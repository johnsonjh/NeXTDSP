/*
  lock.m
  Copyright 1990, NeXT, Inc.
  Responsibility: David A. Jaffe, Mike Minnick
  
  DEFINED IN: The Music Kit
*/

/* 
  Modification history:

  07/27/90/daj - Created.
  08/10/90/daj - Added thread_policy setting. 
  08/13/90/daj - Added enabling of FIXEDPRI policy for apps running as root
                 (or for apps running in an environment in which FIXEDPRI
		 policy has been enabled.)
  08/17/90/daj - Added cthread_yield when we're "behind".
  08/18/90/daj - Added lock as first thing in separate thread.
  08/20/90/daj - Added _MKGetConductorThreadStress and 
                 _MKSetConductorMaxThreadStress
  09/04/90/daj - Added check for overflow of msg_receive timeout.
  09/06/90/daj - Added additional bit poll to make sure things haven't changed
                 out from under us. Also added terminating of thread from
		 removeTimedEntry to avoid bad situations when someone does
		 a finishPerformance followed by startPerformance while holding
		 the lock!
  09/26/90/daj - Added adjustTimedEntry() if needed after MIDI is received.
                 (Formerly, timedEntry->timeToWait was not being reset unless
		 something is rescheduled for the head of the queue. Thus,
		 the time between the last scheduled event and the incoming
		 MIDI was not being subtracted.)
  09/29/90/daj - Changed to use condition signal to flush old thread.
  10/01/90/daj - Changed resetPriority to always do it.		 
*/

/* This source file should be include by Conductor.m. It includes
   the Conductor code relevant to running the Conductor in a background 
   thread. It should really be called "separateThread.m". */

/* Restrictions on use of locking mechanism:

   See ~david/doc/thread-restrictions
   */

#import <mach.h>
#import <mach_init.h>
// #include <mach/policy.h>
// #include <sys/error.h>

// #import <mach.h>
#import <mach_error.h>
#import	<sys/message.h>
#import <cthreads.h>
#import <libc.h>  /* Has setuid, getuid */
#import "_error.h"
#import "lock_primitives.m"

static rec_mutex_t musicKitLock = NULL;  
static cthread_t musicKitThread = NO_CTHREAD;
static condition_t musicKitAbortCondition = NULL;  

/* Forward declarations */ 
static void adjustTimedEntry(double nextMsgTime);

+ useSeparateThread:(BOOL)yesOrNo
  /* Returns self if successful. It's illegal to change this during a 
     performance. */
{
    if (inPerformance)
      return nil;
    separateThread = yesOrNo;
    return self;
}

+ (cthread_t) performanceThread
  /* In a separate-threaded Music Kit performance, returns the c-thread
     used in that performance.  When the thread is finished, returns
     NO_CTHREAD. */
{
    return musicKitThread;
}

/* The idea here is that Mach over-protects us and only allows us to use
   the fixed priority scheme if it's enabled. But it can only be enabled
   if we're running as root.  So what we do is play along to an extent and
   not to an extent.  That is, we follow the spirit of the law, which is that
   only root processes should use fixed priority; we only allow root priority
   to be used if setThreadPriority: was sent with a value greater than 0 and
   if we're running as root.  We don't follow the letter of the law, however,
   in that we enable the fixed policy for all time.  I suppose we could keep
   a count in a file on /tmp or something, but then there's nothing that 
   prevents another app from coming along and enabling or disabling the
   fixed priority anyway and screwing our count up. */

static float threadPriorityFactor = 0.0;
static BOOL useFixedPolicy = NO;

#define ROOT_UID 0

+ setThreadPriority:(float)priorityFactor
{
    if (priorityFactor < 0.0 || priorityFactor > 1.0)
      return nil;
    threadPriorityFactor = priorityFactor;
    if (priorityFactor > 0.0) { /* See if we can used fixed priority sched */
	kern_return_t ec;
	processor_set_t	dpset, dpset_priv;
	if (geteuid() != ROOT_UID) {   /* See if we're set-uid-ed to root. */
	    /* Don't set useFixedPolicy to NO here, because we want
	       it to be sticky, since he may no longer be running as root. */
	    return self;          
	}
	/* If we got to here, we're running as root. */
	/* Fix default processor set to take a fixed priority thread. */
	ec = processor_set_default(host_self(), &dpset);
	if (ec == KERN_SUCCESS) {
	    ec = host_processor_set_priv(host_priv_self(), dpset, &dpset_priv);
	    if (ec == KERN_SUCCESS)
	      ec = processor_set_policy_enable(dpset_priv, POLICY_FIXEDPRI);
	    /* If we ever decide to disable it, here's how: 
	       processor_set_policy_disable(dpset_priv,POLICY_FIXEDPRI,TRUE); 
	       */
	    /* If we ever decide to support VERY high priorities, here's how: 
	       thread_max_priority(thread_self(),dpset_priv,30); */
	}
	useFixedPolicy = (ec == KERN_SUCCESS);
    } else useFixedPolicy = NO;
    return self;
}

/* May want to look at this some time: */
#if 0
static BOOL getProcessorSetInfo(port_t privPortSet,int *info)
{
    kern_return_t ec;
    unsigned int count = PROCESSOR_SET_INFO_MAX;
    ec = processor_set_info(privPortSet, PROCESSOR_SET_SCHED_INFO, host_self(),
			    (processor_set_info_t)info,
			    &count);
    if (ec != KERN_SUCCESS) {
	_MKErrorf(MK_machErr,"Can't get processor set scheduling info",
		  mach_error_string(ec));
	return NO;
    }
    return YES;
}
#endif

#define lockIt() rec_mutex_lock(musicKitLock)
#define unlockIt() rec_mutex_unlock(musicKitLock)

void _MKLock(void) 
{ lockIt(); }

void _MKUnlock(void) 
{ unlockIt(); }     

+ lockPerformance
{  
    lockIt();
    if (inPerformance)
      [self adjustTime];
    return self;
}

+ (BOOL)lockPerformanceNoBlock
{
    if (rec_mutex_try_lock(musicKitLock)) {
	if (inPerformance)
	  [self adjustTime];
	return YES;
    }
    return NO;
}

+ unlockPerformance
{
    [_MKClassOrchestra() flushTimedMessages]; /* A no-op if no Orchestra */
    unlockIt();
    return self;
}

static port_name_t appToMKPort;

static BOOL musicKitHasLock(void)
    /* Returns YES if we are in a multi-threaded performance and the
       Music Kit has the lock.  Note that no mutex is needed around this
       function because the function is assumed to be called from either
       within the Music Kit or from the Appkit with the Music Kit lock. 
       I.e. whoever calls this has the Music Kit lock so its return value
       can't change out from under the caller until the caller himself
       releases the lock.
       */
{
    return (musicKitThread != NO_CTHREAD &&       
	    musicKitLock->thread == musicKitThread);
}

static BOOL thingsHaveChanged = NO; /* See below */

static void sendMessageIfNeeded()
{
	/* The way this is currently implemented, an extra context switch
	   gets done. The alternative is to not send the message until 
	   the _MKUnlock(). But this is more complicated (since multiple
	   messages may cancel each other out) and should be left
	   as an optimziation, if needed. */

    kern_return_t ec;
    msg_header_t msg =    {0,                   /* msg_unused */
                           TRUE,                /* msg_simple */
			   sizeof(msg_header_t),/* msg_size */
			   MSG_TYPE_NORMAL,     /* msg_type */
			   0};                  /* Fills in remaining fields */
    cthread_t self = cthread_self();
    if (!separateThread)
      return;

    /*  We send the message only when someone other than the Music Kit has
	the lock. If we're not in performance, the value of musicKitThread
	and musicKitLock->thread will both be NO_CTHREAD. The assumption here
	is that the caller has the lock if we are in performance. */
    if (musicKitThread == NO_CTHREAD ||       /* Not in performance? */
	musicKitLock->thread == musicKitThread) {  /* MK is running */
	return;
    }
      
    thingsHaveChanged = YES; /* Added Sep6,90 by daj */
    msg.msg_local_port = PORT_NULL;
    msg.msg_remote_port = appToMKPort;
    /*  If we ever want to pass a simple message identification to the other 
	thread, we can do it in	msg.msg_id */

    ec = msg_send(&msg, SEND_TIMEOUT, 0);
    if (ec == SEND_TIMED_OUT)
      ;	/* message queue is full, don't need to send another */
    else if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't send Conductor synchronization message.",
		mach_error_string(ec));
}

/* The following is a minimal structure. We keep it a structure rather than
   a simple double variable to parallel the dpsclient architecture. */
typedef struct _mkOneShotTimedEntry {
    double timeToWait;
} mkTE;

typedef enum _backgroundThreadAction {
    exitThread,
    pauseThread} backgroundThreadAction;

static void emptyAppToMKPort(void);

static void
  killMusicKitThread(void)
{
  /* We have to do this complicated thing because the cthread package doesn't
     support a terminate function. */
  int count;
  if (musicKitThread == NO_CTHREAD)
    return;
  if (musicKitLock->thread == cthread_self()) {/* Must be holding lock to do this */
    /* If we've got the lock, we know that the Music Kit thread is either
       in a msg_receive or waiting for the lock. */
    sendMessageIfNeeded();             /* Get it out of msg_receive */
                                       /* Can't use thread_abort() here
					  because it's possible (unlikely)
					  that thread has not even gotten the
					  initial lock yet! */
    count = musicKitLock->count;       /* Save the count */
    musicKitLock->count = 0;           /* Fudge the rec_mutex for now */
    musicKitLock->thread = NO_CTHREAD;
    while (musicKitThread != NO_CTHREAD) /* Wait for it to be done */
                                        /* condition_wait gives up lock 
					   temporarily */
      condition_wait(musicKitAbortCondition,& musicKitLock->cthread_mutex); 
    musicKitLock->count = count;       /* Fix up the rec_mutex */        
    musicKitLock->thread = cthread_self();
  }
}

static void removeTimedEntry(int arg)
  /* Destroys the timed entry. Made to be compatable with dpsclient version */
{
    switch (arg) {
      case pauseThread:
	adjustTimedEntry(MK_ENDOFTIME);
	break;
      case exitThread:
	if (musicKitThread != NO_CTHREAD && cthread_self() != musicKitThread)  
	  killMusicKitThread();
	break;
      default:
	break;
    }
}

static port_set_name_t conductorPortSet = 0;

/* This is made exactly parallel to dpsclient, in case we ever want to
   export it. */

typedef struct _mkPortInfo {	
    port_name_t thePort;
    void *theArg;
    unsigned msg_size;
    DPSPortProc theFunc;
    int thePriority;            /* Not supported. */
} mkPortInfo;

static mkPortInfo **portInfos = NULL; /* Array of pointers to mkPortInfos */
static int portInfoCount = 0;

/* Will the following scenario work and is it safe?

   Thread A is sitting in a msg_receive on a port set.  
   Thread B removes a port from that port set.

   What is funny about this example is that normally when thread A is in
   the msg_receive, it doesn't have the lock. Therefore, thread B thinks it
   can modify the data. But the one piece of data thread A is still using
   is the port set!

   The alternative is, I guess, the following:

   Thread B sends a message to thread A, thus rousing it from the msg_receive
   and telling thread A to add the port that thread B wanted to add.

   ***THIS IS OK according to Mike DeMoney 18Jun90.  Also according to tech doc on
      msg_receive()***

*/

void _MKAddPort(port_name_t aPort, 
		DPSPortProc aHandler,
		unsigned max_msg_size, 
		void *anArg,int priority)
{
    kern_return_t ec;
    mkPortInfo *p;
    if (!separateThread) {
	DPSAddPort(aPort,aHandler,max_msg_size,anArg,priority);
	return;
    }
    if (!allConductors)
      condInit();
    if (inPerformance) 
      ; /* OK to remove port here - See above */
    /* Check and make sure it's not already added. */
    if (portInfoCount == 0) 
      _MK_MALLOC(portInfos,mkPortInfo *,portInfoCount = 1);
    else {
	register int i;
	for (i = 0; i < portInfoCount; i++)
	  if (portInfos[i]->thePort == aPort) 
	    return;
	_MK_REALLOC(portInfos,mkPortInfo *,++portInfoCount);
    }
    _MK_MALLOC(p,mkPortInfo,1);
    portInfos[portInfoCount-1] = p;
    p->thePort = aPort;
    p->theArg = anArg;
    p->msg_size = max_msg_size;
    p->theFunc = aHandler;
    p->thePriority = priority;      /* Not supported yet (or ever) */
    ec = port_set_add(task_self(),conductorPortSet,aPort);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't add Midi port to Conductor port set.",
		mach_error_string(ec));
}

void _MKRemovePort(port_name_t aPort)
{
    kern_return_t ec;
    int i;
    if (!separateThread) {
	DPSRemovePort(aPort);
	return;
    }
    if (!allConductors)
      condInit();
    if (inPerformance) 
      ; /* OK to remove port here - See above */
    for (i = 0; i < portInfoCount; i++)
      if (portInfos[i]->thePort == aPort) {
	  ec = port_set_remove(task_self(),aPort);
	  if (ec != KERN_SUCCESS)
	    _MKErrorf(MK_machErr,"Can't remove Midi port from Conductor port set.",
		      mach_error_string(ec));
	  NX_FREE(portInfos[i]);
	  portInfos[i] = NULL;
	  portInfoCount--;
	  break;
       }
    for (; i < portInfoCount; i++)
      portInfos[i] = portInfos[i + 1];
}

static void initializeBackgroundThread(void)
{
    /* Must be called from App thread. Called once when the Conductor
       is initialized. */
    kern_return_t ec;
    musicKitLock = rec_mutex_alloc();
    musicKitAbortCondition = condition_alloc();
    ec = port_set_allocate(task_self(), &conductorPortSet);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't allocate Conductor port set",
		mach_error_string(ec));
    ec = port_allocate(task_self(), &appToMKPort);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't allocate Conductor synchronization port",
		mach_error_string(ec));
    ec = port_set_add(task_self(),conductorPortSet,appToMKPort);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,
		"Can't add Application synch port to Conductor port set.",
		mach_error_string(ec));
}

static void emptyAppToMKPort(void)
    /* This is called twice, once at the end of the performance and
       once at the beginning. The reason is: We want to make sure it's
       empty at the start of the performance. But we want to empty it as
       quickly as possible to avoid timing differences between midi and 
       conductor (or orchestra and conductor). So we empty it at the end
       to increase the likelyhood that emptying will be quick at the 
       beginning.  
       */
{
    /* We only empty the appToMKPort and not the MIDI port. The reason
       is obscure. We think that Midi empties the port and we're concerned
       about possible timing race at the start of a performance. 
       */
    struct {
        msg_header_t header;
	char data[MSG_SIZE_MAX];
    } msg;
    msg_return_t ret;
    kern_return_t ec;
    ec = port_set_remove(task_self(),appToMKPort);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't remove Conductor synchronization port.",
		mach_error_string(ec));
    do {
	msg.header.msg_size = MSG_SIZE_MAX;
	msg.header.msg_local_port = appToMKPort;
	ret = msg_receive(&msg.header, RCV_TIMEOUT, 0);
    } while (ret == RCV_SUCCESS);
    if (ret != RCV_TIMED_OUT)
      _MKErrorf(MK_machErr,"Error emptying Conductor synchronization port.",
		mach_error_string(ec));
    ec = port_set_add(task_self(),conductorPortSet,appToMKPort);
    if (ec != KERN_SUCCESS)
      _MKErrorf(MK_machErr,"Can't replace Conductor synchronization port.",
		mach_error_string(ec));
}

static BOOL getThreadInfo(int *info)
{
    kern_return_t ec;
    unsigned int count = THREAD_INFO_MAX;
    ec = thread_info(thread_self(), THREAD_SCHED_INFO, (thread_info_t)info,
		     &count);
    if (ec != KERN_SUCCESS) {
	_MKErrorf(MK_machErr,"Can't get thread scheduling info",
		  mach_error_string(ec));
	return NO;
    }
    return YES;
}

static BOOL setThreadPriority(int priority)
{
    kern_return_t ec = thread_priority(thread_self(), priority, 0);
    if (ec != KERN_SUCCESS) {
	_MKErrorf(MK_machErr,"Can't set thread priority",
		  mach_error_string(ec));
	return NO;
    }
    return YES;
}

static int oldPriority = MAXINT;

#define INVALID_POLICY -1          /* cf /usr/include/sys/policy.h */
#define QUANTUM 100                /* in ms */

static int oldPolicy = INVALID_POLICY;

static void setPriority(void)
{
    int info[THREAD_INFO_MAX];
    thread_sched_info_t sched_info;
    if (threadPriorityFactor == 0.0 || /* No change */
	!getThreadInfo(info))
      return;
    sched_info = (thread_sched_info_t)info;
    /*
     * Increase our thread priority to our current max priority.
     * (Unless base priority is already greater than max, as can happen 
     * with nice -20!)
     */
    if (useFixedPolicy && (sched_info->policy != POLICY_FIXEDPRI)) {
	oldPolicy = sched_info->policy;
	thread_policy(thread_self(), POLICY_FIXEDPRI, QUANTUM);
    } else oldPolicy = INVALID_POLICY;
    if (sched_info->base_priority < sched_info->max_priority) {
	oldPriority = sched_info->base_priority;
	/* Set it to (max - base) * threadPriorityFactor + base */
	setThreadPriority(((sched_info->max_priority - 
			    sched_info->base_priority) * threadPriorityFactor) 
			  + sched_info->base_priority);
    } else oldPriority = MAXINT; /* No priority to be set. */
}

static void resetPriority(void)
{
    int info[THREAD_INFO_MAX];
    thread_sched_info_t sched_info;
    if (oldPolicy != INVALID_POLICY) /* Reset it only if it was set. */
      thread_policy(thread_self(), oldPolicy, QUANTUM);
    if (oldPriority == MAXINT ||  /* No change */
	!getThreadInfo(info))
      return;
    sched_info = (thread_sched_info_t)info;
    setThreadPriority(oldPriority);
    oldPriority = MAXINT;
}

#define MAXSTRESS 100
static unsigned int threadStress = 0;
static unsigned int maxStress = MAXSTRESS;

void _MKSetConductorThreadMaxStress(unsigned int val)
{
    maxStress = val;
}

static any_t separateThreadLoop(any_t unusedArg)
    /* This is the main loop for a separate-threaded performance. */
{
    struct {
        msg_header_t header;
	char data[MSG_SIZE_MAX];
    } msg;
    msg_return_t ret;
    /*
     * When timeToWait is <= MIN_TIMEOUT (in milliseconds), we
     * avoid a context switch by doing a msg_receive() with a zero
     * timeout.  According to Doug Mitchell this is special-cased in the
     * kernel to just poll your ports.
     */
#   define	MIN_TIMEOUT	3
    int timeout;
    double timeToWait;
    BOOL yield;
    lockIt();                /* Must be the first thing in this function */
    _MKDisableErrorStream(); /* See note below */
    emptyAppToMKPort(); /* See comment above */
    setPriority();
    threadStress = 0;
    while (inPerformance) { 
	/* finishPerformance can be called from within the musickit thread
	   or from the appkit thread.  In both cases, inPerformance gets
	   set to NO. In the appkit thread case, we also send a message
	   to kick the musickit thread.
	   */
	/**************************** GOODNIGHT ***************************/
	timeToWait = (performanceIsPaused ? MK_ENDOFTIME : 
		      (isClocked ? theTimeToWait(condQueue->nextMsgTime) : 0.0));
	if (timeToWait >= (MAXINT/1000))
	  timeout = MAXINT;  /* Don't wrap around. */
	else timeout = (int)(timeToWait * 1000);
	if (timeout <= MIN_TIMEOUT) {
	    timeout = 0;
	    if (yield = (threadStress++ > maxStress))
	      threadStress = 0;
	} else {
	    yield = NO;
	    threadStress = 0;
	}
	msg.header.msg_size = MSG_SIZE_MAX;
	msg.header.msg_local_port = conductorPortSet;
	_MKEnableErrorStream(); /* See note below */
	unlockIt();
	if (yield) 
	  cthread_yield();
	ret = msg_receive(&msg.header, RCV_TIMEOUT, timeout);
	lockIt();
	if (!inPerformance)
	    break;	/* may have been set during unlocked period above */
	/**************************** IT'S MORNING! *************************/
	/* If the desire is to exit the thread, this will be 
	   accomplished by the setting of inPerformance to false.
	   If the desire is to pause the thread, this will be
	   accomplished by the setting of timedEntry to MK_ENDOFTIME.
	   If the desire is to reposition the thread, this will be
	   accomplished by the setting of timedEntry to the new 
	   time. */
	_MKDisableErrorStream(); /* See note below */
	if (ret == RCV_TIMED_OUT) {
	    /* The following 2 lines added Sep6,90 by daj. They are necessary
	       because things could change between the time we return from the
	       msg_receive and the time we get the lock. */
	    if (thingsHaveChanged)  /* It's a "kick-me". Go back to loop */
	      thingsHaveChanged = NO;
	    else masterConductorBody();
	}
	else if (ret != RCV_SUCCESS)
	  _MKErrorf(MK_machErr,mach_error_string(ret));
	else if (msg.header.msg_local_port == appToMKPort) 
	  thingsHaveChanged = NO;
	  /* This can happen if there is more than one message in the
	       appToMKPort. This probably should be optimized to make it
	       so that the port accepts only one message. */
	else { 
	    register mkPortInfo *p;
	    if (portInfoCount == 1) { /* Special case for common case */
		p = portInfos[0];      /* Assume it's the right one (safe?) */
		(p->theFunc)(&msg.header, p->theArg);
	    }
	    else { /* Find port and send to it. */
		register int i;
		for (i = 0; i<portInfoCount; i++)
		  if (portInfos[i]->thePort == msg.header.msg_local_port) {
		      p = portInfos[i];
		      (p->theFunc)(&msg.header, p->theArg);
		      break; /* Stop looking */
		  }
	    }
	}
    }
    emptyAppToMKPort();
    resetPriority();
    musicKitThread = NO_CTHREAD;
    condition_signal(musicKitAbortCondition);
    unlockIt(); 

#if 0
    /* This is a kludge to keep thread from exiting */
    msg.header.msg_size = MSG_SIZE_MAX;
    msg.header.msg_local_port = PORT_NULL;
    ret = msg_receive(&msg.header, RCV_TIMEOUT, MAXINT);
#endif
    
    /* cthread_exit(0) implicitly done here. */
}

/* Re: _MKDisableErrorStream/_MKEnableErrorStream,
   It's kind of gross to call these functions ever time through the loop,
   even though they're very cheap.
   The alternative, however, is to drag the Conductor into every Music Kit
   app by having the error routines poll the Conductor if it's in 
   a performance (using _MKMusicKitHasLock).  I don't know which is worse.
*/

static void launchThread(void)
{
    lockIt(); /* Make sure thread has had a chance to finish. */
    cthread_detach(musicKitThread = 
		   cthread_fork(separateThreadLoop,(any_t) 0));
    unlockIt();
    cthread_yield(); /* Give it a chance to run. */
}

