#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Conductor.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/

/* 
Modification history:

  09/15/89/daj - Added caching of inverse of beatSize. 
  09/19/89/daj - Unraveled inefficient MIN(MAX construct.
  09/25/89/daj - Added check for common case in addConductor.
  01/03/90/daj - Added check for performanceIsPaused in adjustTime and
                 _adjustTimeNoTE:. Made jmp_buf be static.
		 Optimized insertMsgQueue() a bit.
  01/07/90/daj - Changed to use faster time stamps.		 
		 Changed comments. addConductor() name changed to 
		 repositionCond()
  01/31/90/daj - Moved _jmpSet = YES; in unclocked loop to fix bug:
                 Memory exception when empty unclocked conducted performance 
		 following an unclocked performance that wasn't empty. 
		 (bug 4451). Added extraInstanceVars mechanism so that
		 we can forever remain backward compatable.
  02/01/90/daj - Added comments. Added check for inPerformance in
                 MKCancelMsgRequest() to fix bug whereby if finishPerformance
		 is triggered by the repositionCond(), condQueue was nil and
		 you got a memory fault.
  03/13/90/daj - Fixed bugs involving pause/resume and timeOffset 
                 (much thanks to lbj). 
                 Fixed bug in MKRescheduleMsgRequest() whereby 
		 the end of performance was erroneously being triggered.
		 Also added MKRepositionMsgRequest().
  	         Moved private methods to a catagory.
  03/19/90/daj - Added pauseFor:.
  03/23/90 lbj - Fixed pauseFor: -- 
  		Put in a return-if-negative-arg predicate and changed the 
		recipient of the resume msgReq to clockConductor (it was self).
  03/23/90/daj - Added check for isPaused in insertMsgQueue() and -emptyQueue
  03/27/90/daj - Added delegate, against my better judgement, at the request
                 of lbj.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/27/90/daj - Commented out MKSetTime(0) (see changes in time.m) 
  06/10/90/daj - Added _MKAdjustTimeIfNotBehind() for Orchestra's synchTime.
  07/27/90/daj - Added seperate thread mechanism.  See lock.m.
  07/27/90/daj - Moved all checks of the form if (!allConductors) condInit()
                 that were in factory methods into +initialize.
  08/20/90/daj - Added delegate methods for crossing high/low deltaT thresholds.
  08/30/90/daj - Fixed bug that caused empty performance to crash if tempo
                 isn't 60! (Changes to _runSetup). Also changed float compares
		 of MK_ENDOFTIME and ENDOFLIST to be safer.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
  09/29/90/daj - Changed to coincide with new way of doing separate thread 
                 loop. Also fixed bug in forking of thread!

*/

/* This is the Music Kit scheduler. See documentation for details. 
 * Note that, in clocked mode,  all timing is done with a single dpsclient 
 * "timed entry." 
 */

/* In this version, you must use the appkit run loop if you are in clocked 
   mode. You may, however, use the unClocked mode without the 
   appkit. */

#import <sys/time_stamp.h>
#define MK_INLINE 1
#import "_musickit.h"
#import "_time.h"

#import <signal.h>
#import <setjmp.h>
static jmp_buf conductorJmp;       /* For long jump. */
#import <appkit/Application.h>
#import <dpsclient/dpsNeXT.h> /* Contains NX_FOREVER */

#define ENDOFLIST (NX_FOREVER)
#define PAUSETIME (MK_ENDOFTIME - 2.0) /* See ISENDOFTIME below */

/* Macros for safe float compares */
#define ISENDOFLIST(_x) (_x > (ENDOFLIST - 1.0))
#define ISENDOFTIME(_x) (_x > (MK_ENDOFTIME - 1.0))

#define TARGETFREES NO
#define CONDUCTORFREES YES

#define NOTIMEDENTRY NULL

/**** FIXME Might want unclocked loop to call getEvent (or, maybe, schedule
  a timed entry for 'soon'. Ask Trey ***/

static BOOL separateThread = NO;
static void *timedEntry = NOTIMEDENTRY; /* Only used for DPS client mode */
static BOOL inPerformance = NO; /* YES if we're currently in performance. */
static BOOL dontHang = YES;     /* NO if we are expecting asynchronous input, 
				   e.g. MIDI or mouse */
static BOOL _jmpSet;         /* YES if setjmp has been called. */
static BOOL isClocked = YES; /* YES if we should stay synced up to the clock.
				NO if we can run as fast as possible. */
static double startTime = 0; /* Start of performance time. */
static double pauseTime = 0; /* Time last paused. */
static double clockTime = 0.0;    /* Clock time. */
static double oldClockTime = 0;   

static MKMsgStruct *afterPerformanceQueue = NULL; /* end-of-time messages. */
static MKMsgStruct *_afterPerformanceQueue = NULL; /* same but private */
static MKMsgStruct *beforePerformanceQueue = NULL;/* start-of-time messages.*/
static MKMsgStruct *afterPerformanceQueueEnd = NULL; /* end-of-time messages.*/
static MKMsgStruct *_afterPerformanceQueueEnd = NULL; /* same but private */
static MKMsgStruct *beforePerformanceQueueEnd = NULL;/* start-of-time msgs.*/
static BOOL performanceIsPaused = NO; /* YES if the entire performance is
					 paused. */
static id classDelegate = nil;  /* Delegate for the whole class. */

#import "_Conductor.h"
@implementation Conductor:Object 
/* nextMsgTime = (nextbeat - time) * beatSize */
{
    double time;     /* Time in beats, updated (for all
		      * instances) after timed entry fires off. */
    double nextMsgTime; /* Time, in seconds, when next message is scheduled to 
			* be sent by this Conductor.
			*/
    double beatSize;   /* The size of a single beat in seconds. */
    double timeOffset;    /* Performance timeOffset in seconds. */
    BOOL isPaused;      /* \fBYES\fR if this instance is paused. 
			 * Note that pausing all Conductors through the
			 * \fBpause\fR factory
			 * method doesn't set this to \fBYES\fR. */
    id delegate;
    MKMsgStruct *_reservedConductor1;
    id _reservedConductor2;
    id _reservedConductor3;    
    double _reservedConductor4;
    void *_reservedConductor5;
}				

/* METHOD TYPES
 * Creating and freeing Conductors
 * Querying the object
 * Modifying the object
 * Controlling a performance
 * Tempo and timeOffset
 * Requesting messages
 */

static Conductor *curRunningCond = nil; /* Or nil if no running conductor. */
static Conductor *condQueue = nil;   /* Head of conductor queue. */
static Conductor *defaultCond = nil; /* default Conductor. */
static Conductor *clockCond = nil;   /* clock time Conductor. */

#define _msgQueue _reservedConductor1 /* MKMsgStruct Q. Sorted by time. */
#define _condNext _reservedConductor2
#define _condLast _reservedConductor3    /* For linked list of conductors. */
#define _pauseOffset _reservedConductor4 
#define _extraVars _reservedConductor5

/* This struct is for instance variables added after the 1.0 instance variable
   freeze. */
typedef struct __extraInstanceVars {
    double inverseBeatSize;
    double timeWhenOffsetIsDone;
    MKMsgStruct *pauseFor;
    unsigned char archivingFlags;
} _extraInstanceVars;

#define NORMALCOND (unsigned char)0
#define CLOCKCOND (unsigned char)1
#define DEFAULTCOND (unsigned char)2

#define _inverseBeatSize(_self) \
          ((_extraInstanceVars *)_self->_extraVars)->inverseBeatSize
#define _timeWhenOffsetIsDone(_self) \
          ((_extraInstanceVars *)_self->_extraVars)->timeWhenOffsetIsDone
#define _pauseFor(_self) \
          ((_extraInstanceVars *)_self->_extraVars)->pauseFor
#define _archivingFlags(_self) \
          ((_extraInstanceVars *)_self->_extraVars)->archivingFlags

#define VERSION2 2

static id allConductors = nil; /* A List of all conductors. */
static void condInit(void);    /* Forward decl */

+initialize
{
    if (self != [Conductor class])
      return self;
    [self setVersion:VERSION2];
    if (!allConductors)
      condInit();
    return self;
}

static void masterConductorBody();
static double theTimeToWait(double nextMsgTime);

#import "lock.m"

static MKMsgStruct *evalSpecialQueue();

#if 0
static double getTime(void)
/* returns the time in seconds.  A 52 bit mantissa in a IEEE double gives us 
   16-17 digits of accuracy.  Taking off six digits for the microseconds in a 
   struct timeval, this leaves 10 digits of seconds,  which is ~300 years.

   The only catch is that this routine must be called at least every 72 min.
   FIXME
*/
{
    struct tsval ts;
    static unsigned int lastStamp = 0;
    static double accumulatedTime = 0.0;
#   define MICRO ((double)0.000001)
#   define WRAP_TIME (((double)0xffffffff) * MICRO)
    kern_timestamp(&ts);
    if (ts.low_val < lastStamp)
	accumulatedTime += WRAP_TIME;
    lastStamp = ts.low_val;
    return accumulatedTime + ((double)ts.low_val) * MICRO;
}
#else 

static double
getTime()
{
    struct tsval ts;
#   define MAX_TIME ((double)0xffffffff)
#   define MICRO ((double)0.000001)
    kern_timestamp(&ts);
    return (((double)ts.high_val) * MAX_TIME + ((double)ts.low_val)) * MICRO;
}
#endif

/* The following implements the delat T high water/low water notification */
static double deltaTThresholdLowPC = .25; /* User sets this */
static double deltaTThresholdHighPC = .75;/* User sets this */

static double deltaTThresholdLow = 0;     
static double deltaTThresholdHigh = 0;    
static BOOL delegateRespondsToThresholdMsgs = NO; 
static BOOL delegateRespondsToLowThresholdMsg = NO;
static BOOL delegateRespondsToHighThresholdMsg = NO;
static BOOL lowThresholdCrossed = NO;

void MKSetLowDeltaTThreshold(double percentage)
{
    deltaTThresholdLowPC = percentage;
    [Conductor _adjustDeltaTThresholds];
}

void MKSetHighDeltaTThreshold(double percentage)
{
    deltaTThresholdHighPC = percentage;
    [Conductor _adjustDeltaTThresholds];
}

static double theTimeToWait(double nextMsgTime)
{
    double t;
    t = getTime();  /* Current time */
    t -= startTime; /* current time relative to the start of performance. */
    t = nextMsgTime - t; /* relative time */
    t = MIN(t, MK_ENDOFTIME); /* clip */
    if (delegateRespondsToThresholdMsgs)
      if (t < deltaTThresholdLow && !lowThresholdCrossed) {
	  if (delegateRespondsToLowThresholdMsg)
	    [classDelegate conductorCrossedLowDeltaTThreshold];
	  lowThresholdCrossed = YES;
      } 
      else if (t > deltaTThresholdHigh && lowThresholdCrossed) {
	  if (delegateRespondsToHighThresholdMsg)
	    [classDelegate conductorCrossedHighDeltaTThreshold];
	  lowThresholdCrossed = NO;
      }
    t = MAX(t,0);
    return t;
}

static void adjustTimedEntry(double nextMsgTime)
    /* The idea here is that we always calibrate by clock time. Therefore
       we can't accumulate errors. We subtract the difference between 
       where we are and where we should be. It is assumed that time is
       already updated. */
{    
    if ((!inPerformance) || (performanceIsPaused) || (musicKitHasLock())
	|| (!isClocked))
      return;  /* No timed entry, s.v.p. */
    if (separateThread) 
      sendMessageIfNeeded();
    else {
	if (timedEntry != NOTIMEDENTRY)
	  DPSRemoveTimedEntry(timedEntry);
	timedEntry = DPSAddTimedEntry(theTimeToWait(nextMsgTime),
				      masterConductorBody,NULL,
				      _MK_DPSPRIORITY);
    }
}

static void
repositionCond(cond,nextMsgTime)
    register Conductor *cond;
    double nextMsgTime;
    /* Enqueue a Conductor (this happens every time a new message is 
       scheduled.)

       cond is the conductor to be enqueued.  nextMsgTime is the next
       post-mapped time that the conductor wants to run.  If we're not in
       performance, just sets cond->nextMsgTime.  Otherwise, enqueues cond at
       the appropriate place. If, after adding the conductor, the head of the
       queue is MK_ENDOFTIME and if we're not hanging, sends
       +finishPerformance. If the newly enqueued conductor is added at the
       head of the list, calls adjustTimedEntry(). */
{
    register Conductor *tmp;
    register double t;
    t = MIN(nextMsgTime,MK_ENDOFTIME);
    t = MAX(t,clockTime);
    cond->nextMsgTime = t;
    if (!inPerformance) 
      return;
    /* remove conductor from doubly-linked list. */
    if (cond == condQueue) { /* It's first */
	if ((!cond->_condNext) ||                /* This check not needed if
						    we always assume 2 
						    conductors */
	    (t < ((Conductor *)cond->_condNext)->nextMsgTime)) { 
	    /* It's us again. */
	    /* We use < to avoid doing an adjustTimedEntry if possible. */
	    /* No need to reposition. */
	    if (!curRunningCond)  /* See comment below */
	      adjustTimedEntry(t);
	    return;                /* No need to check for ENDOFTIME because
				      otherwise, t wouldn't be < nextMsgTime */
	}
	/* Remove us from queue. No need to set pointers in cond because
	   they're going to be set below. */
	condQueue = cond->_condNext;
	if (condQueue)
	  condQueue->_condLast = nil;
    }
    else { 
	/* Remove cond from queue. No need to set pointers in cond because
	   they're going to be set below. */
	if (cond->_condLast)
	  ((Conductor *)cond->_condLast)->_condNext = cond->_condNext;
	if (cond->_condNext)
	  ((Conductor *)cond->_condNext)->_condLast = cond->_condLast;
    }
    /* Now add it. */
    if ((!condQueue)                /* See note above about this check */
	|| (t < condQueue->nextMsgTime)) { /* Add at the start of queue? */
	/* We use < to avoid doing an adjustTimedEntry if possible. */
	/* This can only happen if curRunningCond == self or if
	   nobody's running. In the first case, the timed entry is 
	   added by masterConductorBody. In the second, we add it
	   below. */
	tmp = condQueue;
	condQueue = cond;
	cond->_condNext = tmp;
	cond->_condLast = nil;
	if (tmp)
	  tmp->_condLast = cond;
	if (!curRunningCond)
	  adjustTimedEntry(t); /* Nobody's running and we're not in setup. */
	return;                /* No need to check for ENDOFTIME because
				  otherwise, t wouldn't be < nextMsgTime */
    }
    else {
	for (tmp = condQueue; 
	     (tmp->_condNext && 
	      (((Conductor *)tmp->_condNext)->nextMsgTime <= t)); 
	     tmp = tmp->_condNext)
	  ;
	/* tmp is now first one before us and tmp->_condNext is the first one
	   after us, if any. */
	cond->_condLast = tmp;
	if (tmp->_condNext)
	  ((Conductor *)tmp->_condNext)->_condLast = cond;
	cond->_condNext = tmp->_condNext;
	tmp->_condNext = cond;
    }	
    if (ISENDOFTIME(condQueue->nextMsgTime) &&  
	(dontHang || (!isClocked))) {  /* This is disabled during setup */
	[Conductor finishPerformance];
	return;
    }
}

/* There's a big difference between being paused and waiting for 
   a time offset to finish. 

   Paused: You don't know when you'll resume, but there's a definite
   event (resume message) that resumes you. You do know that beatToClock
   must be called on you before you resume because you can't be the
   currently running conductor (i.e. because you're paused!). So we
   accumulate time in _pauseOffset.  This is so that when we resume, we
   can correct self->time by the amount of time we were paused.

   TimeOffest: You do know when you're offset is over, but you 
   don't have a definite event at that time. So beatToClock reports the
   time from the time when the time offset will be over, rather than using
   the current clockTime. Here we compensate for the offset by subtracting 
   it from self->time, but only once, when we've passed the time when the
   offset is over. 

   There's one more case to consider. If we're paused and we're waiting for
   a time offset to finish, we must take the time we've been paused into
   consideration when figuring the value of beatToClock. That is, we need
   to return the clockTime of the beat that includes both the time offset and
   the time we've been paused. So we add the two here. 
   Similar reasoning requires us to add in _pauseOffset in resume.
*/

static double beatToClock(self,newBeat)
    Conductor *self;
    double newBeat;
    /* Conversion from beat time to clock time.
       This function assumes that self has been adjusted with adjustBeat. */
{
    double x,y;
    if (ISENDOFLIST(newBeat))
      return MK_ENDOFTIME;
    x = _timeWhenOffsetIsDone(self) + self->_pauseOffset;
    y = newBeat - self->time;
    return MAX(y,0.0) * self->beatSize + MAX(x,clockTime);
}

static void adjustBeat(self)
    register Conductor *self;
    /* Given clock time, adjust internal state to reflect current time. */
{
    if (self->isPaused)
      self->_pauseOffset += (clockTime - oldClockTime);
    else if (_timeWhenOffsetIsDone(self) <= clockTime) 
      if (_timeWhenOffsetIsDone(self) != 0) {
	  self->time += (clockTime - _timeWhenOffsetIsDone(self)) * 
	    _inverseBeatSize(self);
	  _timeWhenOffsetIsDone(self) = 0;
      } 
      else 
	self->time += (clockTime - oldClockTime) * _inverseBeatSize(self);
}

static void setTime(t)
    double t;
    /* Adjusts beats of all conductors and resets time. */
{
    register Conductor *cond;
    oldClockTime = clockTime;
    clockTime = t;
    if (curRunningCond) {
  	for (cond = curRunningCond->_condNext; cond; cond = cond->_condNext)
	  adjustBeat(cond);
 	for (cond = curRunningCond->_condLast; cond; cond = cond->_condLast)
	  adjustBeat(cond);
    }
    else for (cond = condQueue; cond; cond = cond->_condNext)
      adjustBeat(cond);
}

static void adjustTime()
/* Normally, the variable time jumps in discrete amounts. However,
   sometimes, as, for example, when an asynchronous event such as
   MIDI or a mouse-click is received, it is desirable to adjust time 
   to reflect the current time.  AdjustTime serves this function.
   Returns the current value of clockTime. */
{
    double time;
    time = getTime() - startTime;
    /* Don't allow it to exceed next scheduled msg. This insures that 
       multiple adjustTimes (e.g. for flurry of MIDI events) don't push 
       scheduled events into the past. (The event loop may favor port action
       over timed entries. Even though it's not obvious, experiments have 
       shown that it's better to do this clip. Otherwise notes are completely
       omitted, whether or not preemption is used (because envelopes are
       "stepped on" out of existance). */
    time = MIN(time,condQueue->nextMsgTime);
    time = MAX(time,clockTime); /* Must be more than previous time. */
    setTime(time);
}

BOOL _MKAdjustTimeIfNotBehind(void)
/* Normally, the variable time jumps in discrete amounts. However,
   sometimes, as, for example, when an asynchronous event such as
   MIDI or a mouse-click is received, it is desirable to adjust time 
   to reflect the current time.  AdjustTime serves this function.
   Returns the current value of clockTime. */
{
    double time;
    time = getTime() - startTime;
    /* Don't allow it to exceed next scheduled msg. This insures that 
       multiple adjustTimes (e.g. for flurry of MIDI events) don't push 
       scheduled events into the past. (The event loop may favor port action
       over timed entries. Even though it's not obvious, experiments have 
       shown that it's better to do this clip. Otherwise notes are completely
       omitted, whether or not preemption is used (because envelopes are
       "stepped on" out of existance). */
    if (time > condQueue->nextMsgTime)
      return NO;
    time = MAX(time,clockTime); /* Must be more than previous time. */
    setTime(time);
    return YES;
}

+(double)time
    /* Returns the time in seconds as viewed by the clock conductor.
       Same as [[Conductor clockConductor] time]. 
       Returns MK_NODVAL if not in
       performance. Use MKIsNoDVal() to check for this return value.  */
{
    if (inPerformance)
      return clockTime;
    return MK_NODVAL;
}

/* The following is a hack that may go away. It was inserted as an 
   emergency measure to get ScorePlayer working for 1.0. - DAJ 
   FIXME */
static void (*pollProc)() = NULL;

void _MKSetPollProc(void (*proc)()) {
    pollProc = proc;
}

static void
unclockedLoop()
    /* Run until done. */
{
    _jmpSet = YES;
    setjmp(conductorJmp);
    if (inPerformance)
      for (;;) {
	  masterConductorBody();
	  if (pollProc)
	    pollProc();
      }
}

/* In addition to the regular message queues, there are several special 
   message queues. E.g. there's one for before-performance messages, one
   for after-performance messages, and so on. These are handled somewhat
   differently. E.g. we don't use back links
   and we cancel just by setting object field to nil. */
static MKMsgStruct *
insertSpecialQueue(sp,queue,queueEnd)
    register MKMsgStruct *sp;
    MKMsgStruct *queue;
    register MKMsgStruct **queueEnd;
    /* insert at end of special msgQueues used for start and end messages */
{
    if (!sp) 
      return queue;
    sp->_onQueue = YES;
    if (*queueEnd) {
	(*queueEnd)->_next = sp;
	(*queueEnd) = sp;
	sp->_next = NULL;
    }
    else {
	sp->_next = NULL;
	*queueEnd = sp;
	queue = sp;
    }
    sp->_conductor = nil; /* nil signals special queue */
    return queue;
}

#define PEEKTIME(pq) (pq)->_timeOfMsg

#define COUNT_MSG_QUEUE_LENGTH 0

static id
insertMsgQueue(sp,self)
    register MKMsgStruct * sp;
    Conductor *self;
    /* inserts in msgQueue and changes timed entry if necessary. */
{
    register double t;
    register MKMsgStruct * tmp;
    if (!sp)
      return nil;
    t = MIN(sp->_timeOfMsg,MK_ENDOFTIME);
    t = MAX(t,self->time);
    sp->_onQueue = YES;
    if ((t < PEEKTIME(self->_msgQueue)) || (!self->_msgQueue->_next)) { 
	sp->_next = self->_msgQueue;
	sp->_prev = NULL;
	sp->_next->_prev = sp;
	self->_msgQueue = sp;
	if (!self->isPaused)
	  repositionCond(self,beatToClock(self,t)); 
	/* Only need to add yourself if this message is the next one. */
    }
    else {
#if 0
	/* Commented out because version below is faster */
	for (tmp = self->_msgQueue; (t >= tmp->_next->_timeOfMsg); 
	     tmp = tmp->_next)
	  ;
	/* insert after tmp */
	sp->_next = tmp->_next;
	sp->_next->_prev = sp;
	tmp->_next = sp;	
	sp->_prev = tmp;
#endif
	for (tmp = self->_msgQueue->_next; (t >= tmp->_timeOfMsg); 
	     tmp = tmp->_next)
	  ;
	/* insert before tmp */
	sp->_next = tmp;
	sp->_prev = tmp->_prev;
	tmp->_prev = sp;	
	sp->_prev->_next = sp;
    }	
    sp->_conductor = self;
#   if COUNT_MSG_QUEUE_LENGTH
    {
	static int maxQueueLen = 0;
	int i;
	for (i = 0, tmp = self->_msgQueue; tmp; tmp = tmp->_next, i++)
	  if (i > maxQueueLen) {
	      fprintf(stderr,"MaxQLen == %d\n",i);
	      maxQueueLen = i;
	  }
    }
#   endif
    return self;
}

/* Why do I call MKMsgStructs "sp" you ask? I don't know!
   Perhaps it means "struct pointer"? */

#define SPCACHESIZE 64  
static MKMsgStruct *spCache[SPCACHESIZE];
static unsigned spCachePtr = 0;

static MKMsgStruct *allocSp()
    /* alloc a new sp. */
{
    MKMsgStruct *sp;
    if (spCachePtr) 
      sp = spCache[--spCachePtr]; 
    else _MK_MALLOC(sp,MKMsgStruct,1);
    return sp;
}

static void freeSp(sp)
    MKMsgStruct * sp;
    /* If cache isn't full, cache sp, else free it. 
       Be careful not to freeSp the same sp twice! */
{
    if (spCachePtr < SPCACHESIZE) 
      spCache[spCachePtr++] = sp;
    else NX_FREE(sp);
}

static MKMsgStruct * 
popMsgQueue(msgQueue)
    register MKMsgStruct * *msgQueue;		
    /* Pop and return first element in process queue. 
       msgQueue is a pointer to the process queue head. */
{
    register MKMsgStruct * sp;
    sp = *msgQueue;		/* Pop msgQueue. */
    if (*msgQueue = (*msgQueue)->_next)
      (*msgQueue)->_prev = NULL;
    return(sp);
}

static void
  masterConductorBody()
{
    register Conductor *self;
    register MKMsgStruct  *curProc;
    /* Premable */
    curRunningCond = condQueue;
    setTime(condQueue->nextMsgTime);
    /* Here is the meat of this condutor's performance. */
    self = curRunningCond;
    do {
	curProc = popMsgQueue(&(self->_msgQueue));
	self->time = curProc->_timeOfMsg;
	if (!curProc->_conductorFrees) {
	    curProc->_onQueue = NO;
	    switch (curProc->_argCount) {
	      case 0:
		(*curProc->_methodImp)(curProc->_toObject,curProc->_aSelector);
		break;
	      case 1:
		(*curProc->_methodImp)(curProc->_toObject,curProc->_aSelector,
				       curProc->_arg1);
		break;
	      case 2:
		(*curProc->_methodImp)(curProc->_toObject,curProc->_aSelector,
				       curProc->_arg1,curProc->_arg2);
		break;
	    }
	}
	else {
	    switch (curProc->_argCount) {
	      case 0: 
		[curProc->_toObject perform:curProc->_aSelector];
		break;
	      case 1: 
		[curProc->_toObject perform:curProc->_aSelector with:
		 curProc->_arg1];
		break;
	      case 2: 
		[curProc->_toObject perform:curProc->_aSelector with:
		 curProc->_arg1 with:curProc->_arg2];
		break;
	      default:
		break;
		freeSp(curProc);
	    }
	}
    } while (PEEKTIME(self->_msgQueue)  <= self->time);
    if (!self->isPaused) 
      repositionCond(self,beatToClock(self,PEEKTIME(self->_msgQueue)));
    /* If at the end, repositionCond triggers [Conductor finishPerformance].
       If this occurs, adjustTimedEntry is a noop.
       */
    if (inPerformance)     /* Performance can be ended by repositionCond(). */
      adjustTimedEntry(condQueue->nextMsgTime); 
    curRunningCond = nil;
    [_MKClassOrchestra() flushTimedMessages];
}

static MKMsgStruct *newMsgRequest();

static void condInit(void)
{
    allConductors = [List new];
    defaultCond = [Conductor new];
    clockCond = [Conductor new];
    /* This actually works ok for +new. The first time +new is called,
       it creates defaultCond, clockCond and the new Cond. */
    initializeBackgroundThread();
}

- free
  /* Freeing a conductor is not permitted. This message overrides the free 
     capability. */
{
    return [self doesNotRecognize:_cmd];
}


-_initialize
    /* Private method that initializes the Conductor when it is created
       and after it finishes performing. Sent by self only. Returns self.
       BeatSize is not reset. It retains its previous value. */
{	
    static const char * const errorString = 
      "Conductor's end-of-list was erroneously evaluated (shouldn't happen).\n";
    _pauseFor(self) = MKCancelMsgRequest(_pauseFor(self));
    _timeWhenOffsetIsDone(self) = timeOffset; 
    /* timeOffset is inititialized to 0 because it's an instance var. */
    time = 0;	
    nextMsgTime = 0;
    _pauseOffset = 0;
    isPaused = NO;
    if (!_msgQueue)
      _msgQueue = newMsgRequest(CONDUCTORFREES,ENDOFLIST,@selector(error:),
				self,1,errorString);
    /* If the end-of-list marker is ever sent, it will print an error. 
       (It's a bug if this every happens.) */
    _condLast = _condNext = nil; /* Remove links. We don't want to leave links
			  around because they screw up repositionCond.
			  The links are added at the last minute in
			  _runSetup. */
    return self;
}

+ allocFromZone:(NXZone *)zone {
    if (inPerformance)
      return nil;
    self = [super allocFromZone:zone];
    return self;
}

+ alloc {
    if (inPerformance)
      return nil;
    self = [super alloc];
    return self;
}

+ new {
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    return self;
}

- init
  /* TYPE: Creating; Creates a new Conductor.
   * Creates and returns a new Conductor object with a tempo of
   * 60 beats a minute.
   * If \fBinPerformance\fR is \fBYES\fR, does nothing
   * and returns \fBnil\fR.
   */
{
    /* Initialize instance variables here that are only intiailized upon
       creation. Initialize instance variables that are reinitialized for
       each performance in the _initialize method. */
    id oldLast = [allConductors lastObject];
    if (oldLast == self) /* Attempt to init twice */
      return nil;
    [allConductors addObjectIfAbsent:self];
    if ([allConductors lastObject] != self)
      return nil; /* Attempt to init twice */
    [super init];
    beatSize = 1;
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
    _pauseFor(self) = NULL; 
    _inverseBeatSize(self) = 1;
    _msgQueue = NULL;
    [self _initialize];
    return self;
}

- copy
  /* Same as [[self copyFromZone:[self zone]]. */
{
    return [self copyFromZone:[self zone]];
}

- copyFromZone:(NXZone *)zone
  /* Same as [[self class] allocFromZone:zone] followed by [self init]. */
{
    id obj;
    obj = [[self class] allocFromZone:zone]; 
    [obj init];
    return obj;
}

+adjustTime
  /* TYPE: Modifying; Updates the current time.
   * Updates the factory's notion of the current time.
   * This method should be invoked whenever 
   * an asychronous event (such as a mouse, keyboard, or MIDI
   * event) takes place. The MidiIn object sends adjustTime for you.
   */
{
    if (inPerformance)
      adjustTime();
    return self;
}

static void _runSetup()
  /* Very private function. Makes the conductor list with much hackery. */
{
#   define HACKDECL() BOOL clk = isClocked;BOOL noHng = dontHang
#   define HACK()     dontHang=NO; isClocked=YES; curRunningCond = clockCond
#   define UNHACK() dontHang = noHng; isClocked = clk; curRunningCond = nil
    /* These hacks are to keep repositionCond() from triggering 
       finishPerformance or adding timed entries while sorting list. */ 
    HACKDECL();
    HACK();
    condQueue = clockCond; 
    /* Set head of queue to an arbitrary conductor. Sorting is done by 
       _runSetup. */
    [allConductors makeObjectsPerform:@selector(_runSetup)];
    UNHACK();
}

+ startPerformance
  /* TYPE: Managing; Starts a performance.
   * Starts a Music Kit performance.  All Conductor objects
   * begin at the same time.
   * In clocked mode, the Conductor assumes the use of the Application 
   * object's event loop.
   * If you have not yet sent the -run message to your application, or if 
   * NXApp has not been created, startPerformance does nothing and return nil.
   */
{
    if (isClocked && (!separateThread && ((!NXApp) || (![NXApp isRunning]))))
      return nil;
    if (inPerformance) 
      return self;
    _MKSetConductedPerformance(YES,self);
    inPerformance = YES;   /* Set this before doing _runSetup so that
			      repositionCond() works right. */
    [self _adjustDeltaTThresholds]; /* For automatic notification */
    _runSetup();
    setTime(clockTime = 0); 
    beforePerformanceQueue = evalSpecialQueue(beforePerformanceQueue,
					      &beforePerformanceQueueEnd);
    if ((dontHang || (!isClocked)) && 
	(ISENDOFTIME(condQueue->nextMsgTime))) { 
	[self finishPerformance];
	return self;
    }
    if (!separateThread) 
      setPriority();
    if (!isClocked && !separateThread) {
	timedEntry = NOTIMEDENTRY;
	unclockedLoop();
	return self;
    }
    if (!separateThread) 
      timedEntry = DPSAddTimedEntry(condQueue->nextMsgTime,
				    masterConductorBody,NULL,
				    _MK_DPSPRIORITY);
    startTime = getTime();
    if (separateThread) {
	launchThread();
    }
    return self;
}

+ defaultConductor
  /* TYPE: Querying; Returns the defaultConductor. 
   * Returns the defaultConductor.
   */
{ 	
    return defaultCond;
}

+(BOOL)inPerformance
  /* TYPE: Querying; Returns \fBYES\fR if a performance is in session.
   * Returns \fBYES\fR if a performance is currently taking
   * place.
   */
{
    return inPerformance;
}

static void evalAfterQueues()
    /* Calls evalSpecialQueue for both the private and public after-performance
       queues. */
{
   _afterPerformanceQueue = evalSpecialQueue(_afterPerformanceQueue,
					     &_afterPerformanceQueueEnd);
   afterPerformanceQueue = evalSpecialQueue(afterPerformanceQueue,
					    &afterPerformanceQueueEnd);
}

+finishPerformance
  /* TYPE: Modifying; Ends the performance.
   * Stops the performance.  All enqueued messages are  
   * flushed and the \fBafterPerformance\fR message is sent
   * to the factory.  Returns \fBnil\fR if it was in performance, nil
   * otherwise.
   *
   * If the performance doesn't hang,
   * the factory automatically sends the \fBfinishPerformance\fR
   * message to itself when the message queue is exhausted.
   * It can also be sent by the application to terminate the
   * performance prematurely.
   */
{	
    double lastTime;
    if (!inPerformance) {
	evalAfterQueues(); /* This is needed for MKFinishPerformance() */
	return nil;
    }
    performanceIsPaused = NO;
    _MKSetConductedPerformance(NO,self);
    inPerformance = NO; /* Must be set before -emptyQueue is sent */
    [allConductors makeObjectsPerform:@selector(emptyQueue)];
    if (separateThread)
      removeTimedEntry(exitThread);
    else if (timedEntry != NOTIMEDENTRY)
      DPSRemoveTimedEntry(timedEntry);
    if (!separateThread)
      resetPriority();
    timedEntry = NOTIMEDENTRY;
    condQueue = nil;
    lastTime = clockTime;
    setTime(clockTime = 0.0);
    //   MKSetTime(0.0); /* Handled by _MKSetConductedPerformance now */
    oldClockTime = lastTime;
    [allConductors makeObjectsPerform:@selector(_initialize)];
    evalAfterQueues();
    if (_jmpSet) {
	_jmpSet = NO;     
	longjmp(conductorJmp, 0);
    } 
    else _jmpSet = NO;
    return self;
}


+pausePerformance
  /* TYPE: Controlling; Pauses a performance.
   * Pauses all Conductors.  The performance is resumed when
   * the factory receives the \fBresume\fR message.
   * The factory object's notion of the current time is suspended
   * until the performance is resumed.
   * It's illegal to pause an unclocked performance. Returns nil in this
   * case, otherwise returns the receiver.
   * Note that pausing a performance does not pause MidiIn, nor does it
   * pause any Instruments that have their own clocks. (e.g. MidiOut, and
   * the Orchestra).
   */
{	
   if ((!inPerformance)  || performanceIsPaused)
     return self;
   if (!isClocked)
     return nil;	
   pauseTime = getTime();
   if (separateThread)
     removeTimedEntry(pauseThread);
   else if (timedEntry != NOTIMEDENTRY)
     DPSRemoveTimedEntry(timedEntry);
   timedEntry = NOTIMEDENTRY;
   performanceIsPaused = YES;
   return self;
}

+(BOOL)isPaused
  /* TYPE: Querying; \fBYES\fR if performance is paused.
   * Returns \fBYES\fR if the performance is paused.
   */
{
    return performanceIsPaused;
}

+resumePerformance
  /* TYPE: Controlling; Unpauses a paused performance.
   * Resumes a paused performance.
   * When the performance resumes, the notion of the
   * current time will be the same as when the factory
   * received the \fBpause\fR message --
   * time stops while the performance
   * is paused. It's illegal to resume a performance that's not clocked.
   * Returns nil in this case, otherwise returns the receiver.
   */
{	
    if ((!inPerformance)  || (!performanceIsPaused))
      return self;
    if (!isClocked)
      return nil;
    performanceIsPaused = NO;
    startTime += (getTime() - pauseTime);
    /* We use cur-start to get the time since the start of the performance
       with all pauses removed. Thus by increasing startTime by the
       paused time, we remove the effect of the pause. */
    adjustTimedEntry(condQueue->nextMsgTime); 
    return self;
}

+ currentConductor
  /* TYPE: Querying; Returns the Conductor that's sending a message, if any.
   * Returns the Conductor that's in the process
   * of sending a message, or \fBnil\fR if no message
   * is currently being sent.
   */
{
    return curRunningCond;
}

+ setFinishWhenEmpty:(BOOL)yesOrNo
  /* TYPE: Modifying; Sets \fBBOOL\fR for continuing with empty queues.
   * If \fIyesOrNo\fR is \fBNO\fR (the default), the performance
   * is terminated when all the Conductors' message queues are empty.
   * If \fBYES\fR, the performance continues until the \fBfinishPerformance\fR
   * message is sent.
   */
{
    dontHang = yesOrNo;
    return self;
}


+ setClocked:(BOOL)yesOrNo 
  /* TYPE: Modifying; Establishes clocked or unclocked performance.  
   * If \fIyesOrNo\fR is \fBYES\fR, messages are dispatched
   * at specific times.  If \fBNO\fR, they are sent 
   * as quickly as possible.
   * It's illegal to invoke this method while the performance is in progress.
   * Returns nil in this case, else self.
   * Initialized to \fBYES\fR.  
   */
{	
    if (inPerformance && 
	((yesOrNo && (!isClocked)) || (!yesOrNo) && isClocked))
      return nil;
    isClocked = yesOrNo;
    return self;
}

+(BOOL)isEmpty
  /* TYPE: Querying; \fBYES\fR if in performance and all queues are empty.
   * \fBYES\fR if the performance is active and all message queues are empty. 
   * (This can only happen if \fBsetFinishWhenEmpty:NO\fR was sent.)
   */
{
    return ((!dontHang) && (inPerformance) && 
	    (!condQueue || ISENDOFTIME(condQueue->nextMsgTime)));
}

+(BOOL)finishWhenEmpty
  /* TYPE: Querying; \fBYES\fR if performance continues despite empty queues.
   * Returns \fBNO\fR if the performance is terminated when the
   * queues are empty, \fBNO\fR if the performance continues.
   */
{
    return dontHang;
}

+(BOOL) isClocked
  /* TYPE: Querying; \fBYES\fR if performance is clocked, \fBNO\fR if not.
   * Returns \fBYES\fR if messages are sent at specific times,
   * \fBNO\fR if they are sent as quickly as possible.
   */
{	
    return isClocked;
}

-(BOOL)isPaused 
  /* Returns YES if the receiver is paused. */
{
    return isPaused;
}

-pause
  /* TYPE: Controlling; Pauses the receiver.
   * Pauses the performance of the receiver.
   * The effect on the receiver is restricted to
   * the present performance;
   * paused Conductors are automatically resumed at the end of each
   * performance.
   * Returns the receiver.
   */
{
    if (self == clockCond)
      return nil;
    if (isPaused)
      return self;
    isPaused = YES;
    repositionCond(self,PAUSETIME);
    if ([delegate respondsTo:@selector(conductorDidPause:)])
      [delegate conductorDidPause:self];
    return self;
}

-resume
  /* TYPE: Controlling; Resumes a paused receiver.
   * Resumes the receiver.  If the receiver isn't currently paused
   * (if it wasn't previously sent the \fBpause\fR message),
   * this has no effect.
   */
{
    if (!isPaused)
      return self;
    isPaused = NO;
    if (_timeWhenOffsetIsDone(self) != 0)
      _timeWhenOffsetIsDone(self) += _pauseOffset;
    _pauseOffset = 0; 
    repositionCond(self,beatToClock(self,PEEKTIME(_msgQueue)));
    _pauseFor(self) = MKCancelMsgRequest(_pauseFor(self));
    if ([delegate respondsTo:@selector(conductorDidResume:)])
      [delegate conductorDidResume:self];
    return self;
}

-pauseFor:(double)seconds
{
    /* Pause it if it's not already paused. */
    if (seconds <= 0.0 || ![self pause]) 
      return nil;
    if (_pauseFor(self))/* Already doing a "pauseFor"? */
      MKRepositionMsgRequest(_pauseFor(self),clockTime + seconds);
    else {             /* New "pauseFor". */
	_pauseFor(self) = MKNewMsgRequest(clockTime + seconds,
					  @selector(resume),self,0);
	MKScheduleMsgRequest(_pauseFor(self), clockCond); 
    }
    return self;
}

+ clockConductor
  /* TYPE: Querying; Returns the clockConductor. 
   * Returns the clockConductor.
   */
{ 	
    return clockCond;
}

-(double)setBeatSize:(double)newBeatSize
  /* TYPE: Tempo; Sets the tempo by resizing the beat.
   * Sets the tempo by changing the size of a beat to \fInewBeatSize\fR,
   * measuredin seconds.  The default beat size is 1.0 (one beat per
   * second).
   * Attempts to set the beat size of the clockConductor are ignored. 
   * Returns the previous beat size.
   */
{
    register double oldBeatSize;
    oldBeatSize = beatSize;
    if (self == clockCond)
      return oldBeatSize;
    beatSize  = newBeatSize;
    _inverseBeatSize(self) = 1.0/beatSize;
    if (curRunningCond != self && !isPaused) 
      repositionCond(self,beatToClock(self,PEEKTIME(self->_msgQueue)));
    return oldBeatSize;
}

-(double)setTempo:(double)newTempo
  /* TYPE: Tempo; Sets the tempo in beats per minute.
   * Sets the tempo to \fInewTempo\fR, measured in beats per minute.
   * Implemented as \fB[self\ setBeatSize:\fR(60.0/\fInewTempo\fR)\fB]\fR.
   * Attempts to set the tempo of the clockConductor are ignored. 
   * Returns the previous tempo.
   */
{
    return 60.0 / [self setBeatSize: (60.0 / newTempo)];
}

-(double)setTimeOffset:(double)newTimeOffset
  /* TYPE: Tempo; Sets the receiver's timeOffset value in seconds.
   * Sets the number of seconds into the performance
   * that the receives waits before it begins processing
   * its message queue.  Notice that \fInewtimeOffset\fR is measured
   * in seconds -- it's not affected by the receiver's
   * tempo.
   * Attempts to set the timeOffset of the clockConductor are ignored. 
   * Returns the previous timeOffset.
   */
{
    register double oldTimeOffset;
    oldTimeOffset = timeOffset;
    if (self == clockCond)
      return oldTimeOffset;
    timeOffset  = newTimeOffset;
    if (inPerformance)
      _timeWhenOffsetIsDone(self) = newTimeOffset - oldTimeOffset + clockTime;
    else _timeWhenOffsetIsDone(self) = newTimeOffset;
    if (curRunningCond != self && !isPaused) 
      repositionCond(self,beatToClock(self,PEEKTIME(self->_msgQueue)));
    return oldTimeOffset;
}

-(double)beatSize
  /* TYPE: Tempo; Returns the receiver's beat size in seconds.
   * Returns the size of the receiver's beat, in seconds.
   */
{
    return beatSize;
}

-(double)tempo
  /* TYPE: Tempo; Returns the receiver's tempo in beats per minute.
   * Returns the receiver's tempo in beats per minute.
   */
{
    return 60.0 * _inverseBeatSize(self);
}

-(double)timeOffset
  /* TYPE: Tempo; Returns the receiver's timeOffset.
   * Returns the receiver's timeOffset value.
   */
{
    return timeOffset;
}


-sel:(SEL)aSelector to:(id)toObject withDelay:(double)deltaT 
 argCount:(int)argCount, ...;
/* TYPE: Requesting; Requests \fBaSelector\fR to be sent to \fBtoObject\fR.
 * Schedules a request for the receiver
 * to send the message \fIaSelector\fR to the
 * object \fItoObject\fR at time \fIdeltaT\fR beats from now.
 * \fIargCount\fR specifies the number of arguments to 
 * \fIaSelector\fR followed by the arguments themselves,
 * separated by commas (up to two arguments are allowed).
 * Returns the receiver unless argCount is too high, in which case returns nil.
 */
{
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    va_end(ap);	
    return insertMsgQueue(newMsgRequest(CONDUCTORFREES,self->time + deltaT,
					aSelector,toObject,argCount,arg1,arg2),
			  self);
}

-sel:(SEL)aSelector to:(id)toObject atTime:(double)t
 argCount:(int)argCount, ...;
/* TYPE: Requesting; Requests \fIaSelector\fR to be sent to \fItoObject\fR.
 * Schedules a request for the receiver to send 
 * the message \fIaSelector\fR to the object \fItoObject\fR at
 * time \fIt\fR beats into the performance (offset by the
 * receiver's timeOffset).
 * \fIargCount\fR specifies the number of arguments to 
 * \fIaSelector\fR followed by the arguments themselves,
 * seperated by commas (up to two arguments are allowed).
 * Returns the receiver unless argCount is too high, in which case returns 
 * nil. 
 */
{
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    va_end(ap);	
    return insertMsgQueue(newMsgRequest(CONDUCTORFREES,t,aSelector,toObject,
				 argCount,arg1,arg2),
			  self);
}

- _runSetup
  /* Private method that inits a Conductor to run. */
{
    if (ISENDOFLIST(PEEKTIME(_msgQueue)))
      nextMsgTime = MK_ENDOFTIME;
    else {
	nextMsgTime = PEEKTIME(_msgQueue) * beatSize + timeOffset;
	nextMsgTime = MIN(nextMsgTime,MK_ENDOFTIME);    
    }
    repositionCond(self,nextMsgTime);
    return self;
}

-(double)predictTime:(double)beatTime
  /* TYPE: Querying; Returns predicted time corresponding to beat.
   * Returns the time, in tempo-mapped time units,
   * corresponding to a specified beat time.  In our Conductor, this method
   * just assumes the tempo is a constant between now and beatTime.  That
   * is, it keeps no record of how tempo has changed, nor does it know the
   * future; it just computes based on the current time and tempo.  
   * More sophisticated tempo mappings an be added by subclassing
   * conductor. See "Ensemble Aspects of Computer Music", by David Jaffe,
   * CMJ (etc.) for details of tempo mappings. 
   */
{
    return beatToClock(self,beatTime);
}    

-(double)time
  /* TYPE: Querying; Returns the receiver's notion of the current time.
   * Returns the receiver's notion of the current time
   * in beats.
   */
{	
    return self->time;
}

-emptyQueue
  /* TYPE: Requesting; Flushes the receiver's message queue.
   * Flushes the receiver's message queue and returns self.
   */
{
    register MKMsgStruct *curProc;
    while (!ISENDOFLIST(PEEKTIME(_msgQueue))) {
	curProc = popMsgQueue(&(_msgQueue)); 
	if (curProc) /* This test shouldn't be needed */
	  if (curProc->_conductorFrees) 
	    freeSp(curProc);
	  else curProc->_onQueue = NO;
    }
    if (!isPaused)
      repositionCond(self,MK_ENDOFTIME);
    return self;
}

-(BOOL)isCurrentConductor
  /* TYPE: Querying; \fBYES\fR if the receiver is sending a message.
   * Returns \fBYES\fR if the receiver
   * is currently sending a message.
   */
{
    return (curRunningCond == self);
}    

/* The following functions and equivalent methods
   give the application more control over 
   scheduling. In particular, they allow scheduling requests to be
   unqueued. They work as follows:

   MKNewMsgRequest() creates a new MKMsgStruct.
   MKScheduleMsgRequest() schedules up the request.
   MKCancelMsgRequest() cancels the request.

   Unless a MKCancelMsgRequest() is done, it is the application's 
   responsibility to NX_FREE the structure. On the other hand,
   if a  MKCancelMsgRequest() is done, the application must relinquish
   ownership of the MKMsgStruct and should not NX_FREE it.
   */

static void freeSp();

MKMsgStruct *
MKCancelMsgRequest(MKMsgStruct *aMsgStructPtr)
    /* Cancels MKScheduleMsgRequest() request and frees the structure. 
       Returns NULL.
       */
{
    if (!aMsgStructPtr)
      return NULL;
    if (!aMsgStructPtr->_conductor) { /* Special queue */
	/* Special queue messages are cancelled differently. Here we just
	   se the _toObject field to nil and leave the struct in the list. */
	if (!aMsgStructPtr->_toObject) /* Already canceled? */
	  return NULL;
	aMsgStructPtr->_toObject = nil;
	if (aMsgStructPtr->_onQueue) 
	  aMsgStructPtr->_conductorFrees = YES;
	else freeSp(aMsgStructPtr);
	return NULL;
    }
    if (aMsgStructPtr->_onQueue) { /* Remove from ordinary queue */
	aMsgStructPtr->_next->_prev = aMsgStructPtr->_prev;
	if (aMsgStructPtr->_prev)
	  aMsgStructPtr->_prev->_next = aMsgStructPtr->_next;
	else {
	    Conductor *conductor = aMsgStructPtr->_conductor;
	    conductor->_msgQueue = aMsgStructPtr->_next;
	    if ((curRunningCond != conductor) && !conductor->isPaused) {
		/* If our conductor is the running conductor, then 
		   repositionCond will be called by him so there's no need 
		   to do it here. */
		BOOL wasHeadOfQueue;
		double nextTime = beatToClock(conductor,
					      PEEKTIME(conductor->_msgQueue));
		wasHeadOfQueue = (conductor == condQueue);
		/* If we're the head of the queue, then the message we've
		   just deleted is enqueued to us with a timed entry. We've
		   got to do an adjustTimedEntry. */
		repositionCond(conductor,nextTime); 
		/* Need to check for inPerformance below because
		   repositionCond() can call finishPerformance. */
		/* If curRunningCond is non-nil, adjustTimedEntry will be 
		   done for us by masterConductorBody. */
		if (inPerformance && !curRunningCond && wasHeadOfQueue)
		  adjustTimedEntry(condQueue->nextMsgTime);
	    }
	}
    }
    freeSp(aMsgStructPtr);
    return NULL;
}

MKMsgStruct *
MKNewMsgRequest(double timeOfMsg,SEL whichSelector,id destinationObject,
		int argCount,...)
    /* Creates a new msgStruct to be used with MKScheduleMsgRequest. 
       args may be ids or ints. The struct returned by MKNewMsgRequest
       should not be altered in any way. Its only use is to pass to
       MKCancelMsgRequest() and MKScheduleMsgRequest(). */
{
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount);
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    va_end(ap);
    return newMsgRequest(TARGETFREES,timeOfMsg,whichSelector,destinationObject,
			 argCount,arg1,arg2);
}	


void MKScheduleMsgRequest(MKMsgStruct *aMsgStructPtr, id conductor)
    /* Reschedule the specified msg. */
{	
    if (aMsgStructPtr && conductor && (!aMsgStructPtr->_onQueue))
      insertMsgQueue(aMsgStructPtr,conductor);
}

MKMsgStruct *MKRescheduleMsgRequest(MKMsgStruct *aMsgStructPtr,id conductor,
				    double timeOfNewMsg,SEL whichSelector,
				    id destinationObject,int argCount,...)
    /* Reschedules the MKMsgStruct pointed to by aMsgStructPtr as indicated.
       If aMsgStructPtr is non-NULL and points to a message request currently
       in the queue, first cancels that request. Returns a pointer to the
       new MKMsgStruct. This function is equivalent to the following
       three function calls:
       MKNewMsgRequest() // New one
       MKScheduleMsgRequest() // New one 
       MKCancelMsgRequest() // Old one

       Note that aMsgStructPtr may be rescheduled with a different conductor,
       destinationObject and arguments, 
       than those used when it was previously scheduled.
       */
{
    MKMsgStruct *newMsgStructPtr;
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount);
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    va_end(ap);
    newMsgStructPtr = newMsgRequest(TARGETFREES,timeOfNewMsg,
				    whichSelector,destinationObject,
				    argCount,arg1,arg2);
    MKScheduleMsgRequest(newMsgStructPtr,conductor);
    MKCancelMsgRequest(aMsgStructPtr); /* A noop if already canceled. */
    return aMsgStructPtr = newMsgStructPtr;
}

MKMsgStruct *MKRepositionMsgRequest(MKMsgStruct *aMsgStructPtr,
				    double timeOfNewMsg)
{
    MKMsgStruct *newMsgStructPtr = newMsgRequest(TARGETFREES,timeOfNewMsg,
						    aMsgStructPtr->_aSelector,
						    aMsgStructPtr->_toObject,
						    aMsgStructPtr->_argCount,
						    aMsgStructPtr->_arg1,
						    aMsgStructPtr->_arg2);
    MKScheduleMsgRequest(newMsgStructPtr,aMsgStructPtr->_conductor);
    MKCancelMsgRequest(aMsgStructPtr);
    return aMsgStructPtr = newMsgStructPtr;
}

static MKMsgStruct *
newMsgRequest(doesConductorFree,timeOfMsg,whichSelector,destinationObject,
		argCount,arg1,arg2)
    BOOL doesConductorFree;
    double timeOfMsg;
    SEL whichSelector;
    id destinationObject;
    int argCount;
    id arg1,arg2;
    /* Create a new msg struct */
{
    MKMsgStruct * sp;
    sp = allocSp();
    if ((sp->_conductorFrees = doesConductorFree) == TARGETFREES)
      sp->_methodImp = [destinationObject methodFor:whichSelector];
    sp->_timeOfMsg = timeOfMsg;
    sp->_aSelector = whichSelector;
    sp->_toObject = destinationObject;
    sp->_argCount = argCount;
    sp->_next = NULL;
    sp->_prev = NULL;
    sp->_conductor = nil;
    sp->_onQueue = NO;
    switch (argCount) {
      default: 
	freeSp(sp);
	return NULL;
      case 2: 
	sp->_arg2 = arg2;
      case 1: 
	sp->_arg1 = arg1;
      case 0:
	break;
    }
    return(sp);
}	

+(MKMsgStruct *)afterPerformanceSel:(SEL)aSelector 
 to:(id)toObject 
 argCount:(int)argCount, ...;
/* TYPE: Requesting; Sends \fIaSelector\fR to \fItoObject\fR after performance.
 * Schedules a request for the factory object
 * to send the message \fIaSelector\fR to the object
 * \fItoObject\fR immediately after the performance ends, regardless
 * of how it ends.
 * \fBargCount\fR specifies the number of arguments to 
 * \fBaSelector\fR followed by the arguments themselves,
 * seperated by commas.  Up to two arguments are allowed.
 * AfterPerfomance messages are sent in the order they were enqueued.
 * Returns a pointer to an \fBMKMsgStruct\fR that can be
 * passed to \fBcancelMsgRequest:\fR.
 */
{
    id arg1,arg2;
    MKMsgStruct *sp;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    afterPerformanceQueue = 
      insertSpecialQueue(sp = newMsgRequest(CONDUCTORFREES,MK_ENDOFTIME,
					    aSelector,toObject,argCount,
					    arg1,arg2),
			 afterPerformanceQueue,&afterPerformanceQueueEnd);
    va_end(ap);
    return(sp);
}

+(MKMsgStruct *)beforePerformanceSel:(SEL)aSelector 
 to:(id)toObject 
 argCount:(int)argCount, ...;
/* TYPE: Requesting; Sends \fIaSelector\fR to \fItoObject\fR before performance.
 * Schedules a request for the factory object
 * to send the message \fIaSelector\fR to the object
 * \fItoObject\fR immediately before performance begins.
 * \fBargCount\fR specifies the number of arguments to 
 * \fBaSelector\fR followed by the arguments themselves,
 * seperated by commas.  Up to two arguments are allowed.
 * Messages requested through this method will be sent
 * before any other messages. beforePerformance messages are sent in the
 * order they were enqueued.
 * Returns a pointer to an \fBMKMsgStruct\fR that can be
 * passed to \fBcancelMsgRequest:\fR.
 */
{
    id arg1,arg2;
    MKMsgStruct *sp;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    beforePerformanceQueue = 
      insertSpecialQueue(sp = newMsgRequest(CONDUCTORFREES,0.0,
					    aSelector,toObject,argCount,
					    arg1,arg2),
			 beforePerformanceQueue,&beforePerformanceQueueEnd);
    va_end(ap);
    return(sp);
}

static MKMsgStruct *evalSpecialQueue(queue,queueEnd)
    MKMsgStruct *queue;
    MKMsgStruct **queueEnd;
    /* Sends all messages in the special queue, e.g. afterPerformanceQueue.
     */
{
    register MKMsgStruct *curProc;

    while (queue) {
	curProc = popMsgQueue(&(queue));
	if (curProc == *queueEnd)
	  *queueEnd = NULL;
	switch (curProc->_argCount) {
	  case 0: 
	    [curProc->_toObject perform:curProc->_aSelector];
	    break;
	  case 1: 
	    [curProc->_toObject perform:curProc->_aSelector with:curProc->_arg1];
	    break;
	  case 2: 
	    [curProc->_toObject perform:curProc->_aSelector with:curProc->_arg1
		 with:curProc->_arg2];
	    break;
	  default: 
	    break;
	}
	if (curProc->_conductorFrees)
	  freeSp(curProc);
    }
    return NULL;
}


- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Archives beatSize and timeOffset. Also archives whether this
     was the clockConductor or defaultConductor.
     */
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"ddc",&beatSize,&timeOffset,
		 &(_archivingFlags(self)));
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked with NXReadObject(). 
     See write: and finishUnarchiving.
     */
{
    [super read:aTypedStream];
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
    if (NXTypedStreamClassVersion(aTypedStream,"Conductor") == VERSION2) {
	NXReadTypes(aTypedStream,"ddc",&beatSize,&timeOffset,
		    &(_archivingFlags(self)));
    }
    return self;
}

-finishUnarchiving
  /* If the unarchived object was the clockConductor, frees the new object
     and returns the clockConductor. Otherwise, if there is a performance
     going on, frees the new object and returns the defaultConductor. 
     Otherwise, if the unarchived object was the defaultConductor, sets the 
     defaultConductor's beatSize and timeOffset from the unarchived object,
     frees the new object and returns the defaultConductor.
     Otherwise, the new unarchived Conductor is added to the Conductor
     list and nil is returned. */
{
    if (_archivingFlags(self) == CLOCKCOND) {
	[super free];
	return clockCond;
    } else if (inPerformance) {
	[super free];
	return defaultCond;
    } else if (_archivingFlags(self) == DEFAULTCOND) {
	[defaultCond setBeatSize:beatSize];
	[defaultCond setTimeOffset:timeOffset];
	[super free];
	return defaultCond;
    } 
    [allConductors addObject:self];
    _inverseBeatSize(self) = 1.0/beatSize;
    [self _initialize];
    return nil;
}

+setDelegate:object
{
    delegateRespondsToLowThresholdMsg = 
      [object respondsTo:@selector(conductorCrossedLowDeltaTThreshold)];
    delegateRespondsToHighThresholdMsg = 
      [object respondsTo:@selector(conductorCrossedHighDeltaTThreshold)];
    delegateRespondsToThresholdMsgs = (delegateRespondsToLowThresholdMsg ||
				       delegateRespondsToHighThresholdMsg);
    classDelegate = object;
    return self;
}

+delegate 
{
    return classDelegate;
}

-setDelegate:object
{
    delegate = object;
    return self;
}

-delegate
{
    return delegate;
}

/* Needed to get around a compiler bug FIXME */
static double getNextMsgTime(Conductor *aCond)
{
    return aCond->nextMsgTime;
}

@end

@implementation Conductor(Private)


+(MKMsgStruct *)_afterPerformanceSel:(SEL)aSelector 
 to:(id)toObject 
 argCount:(int)argCount, ...;
/* 
  Same as afterPerformanceSel:to:argCount: but ensures that message will
  be sent before any of the messages enqueued with that method. Private
  to the musickit.
*/
{
    id arg1,arg2;
    MKMsgStruct *sp;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    _afterPerformanceQueue = 
      insertSpecialQueue(sp = newMsgRequest(CONDUCTORFREES,MK_ENDOFTIME,
					    aSelector,toObject,argCount,
					    arg1,arg2),
			 _afterPerformanceQueue,&_afterPerformanceQueueEnd);
    va_end(ap);
    return(sp);
}

+(MKMsgStruct *)_newMsgRequestAtTime:(double)timeOfMsg
  sel:(SEL)whichSelector to:(id)destinationObject
  argCount:(int)argCount, ...;
/* TYPE: Requesting; Creates and returns a new message request.
 * Creates and returns message request but doesn't schedule it.
 * The return value can be passed as an argument to the
 * \fB_scheduleMsgRequest:\fR and \fB_cancelMsgRequest:\fR methods.
 *
 * You should only invoke this method if you need greater control
 * over scheduling (for instance if you need to cancel a message request)
 * than that afforded by the \fBsel:to:atTime:argCount:\fR and
 * \fBsel:to:withDelay:argCount:\fR methods.
 */
{
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    va_end(ap);
    return newMsgRequest(TARGETFREES,timeOfMsg,whichSelector,destinationObject,
			 argCount,arg1,arg2);
}

-(void)_scheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr
  /* TYPE: Requesting; Schedules a message request with the receiver.
   * Sorts the message request \fIaMsgStructPtr\fR
   * into the receiver's message queue.  \fIaMsgStructPtr\fR is 
   * a pointer to an \fBMKMsgStruct\fR, such as returned by
   * \fB_newMsgRequestAtTime:sel:to:argCount:\fR.
   */
{
    if (aMsgStructPtr && (!aMsgStructPtr->_onQueue))
      insertMsgQueue(aMsgStructPtr,self);
}

+(void)_scheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr
  /* Same as _scheduleMsgRequest: but uses clock conductor. 
   */
{
    if (aMsgStructPtr && (!aMsgStructPtr->_onQueue))
      insertMsgQueue(aMsgStructPtr,clockCond);
}

-(MKMsgStruct *)_rescheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr
  atTime:(double)timeOfNewMsg sel:(SEL)whichSelector
  to:(id)destinationObject argCount:(int)argCount, ...;
  /* TYPE: Requesting; Reschedules a message request with the receiver.
   * Redefines the message request \fIaMsgStructPtr\fR according
   * to the following arguments and resorts
   * it into the receiver's message queue.
   * \fIaMsgStructPtr\fR is 
   * a pointer to an \fBMKMsgStruct\fR, such as returned by
   * \fB_newMsgRequestAtTime:sel:to:argCount:\fR.
   */

/* Same as MKReschedule */
{
    id arg1,arg2;
    va_list ap;
    va_start(ap,argCount); 
    arg1 = va_arg(ap,id);
    arg2 = va_arg(ap,id);
    return MKRescheduleMsgRequest(aMsgStructPtr,self,timeOfNewMsg,
    		whichSelector,destinationObject,argCount,arg1,
				  arg2);
    va_end(ap);
}


+(MKMsgStruct *)_cancelMsgRequest:(MKMsgStruct *)aMsgStructPtr
  /* TYPE: Requesting; Cancels the message request \fIaMsgStructPtr\fR.
   * Removes the message request pointed to by
   * \fIaMsgStructPtr\fR. 
   * Notice that this is a factory method -- you don't have to
   * know which queue the message request is on to cancel it.
   * \fIaMsgStructPtr\fR is 
   * a pointer to an \fBMKMsgStruct\fR, such as returned by
   * \fB_newMsgRequestAtTime:sel:to:argCount:\fR.
   */
{
    return MKCancelMsgRequest(aMsgStructPtr);
}

+(double)_adjustTimeNoTE:(double)desiredTime     
  /* TYPE: Modifying; Sets the current time to \fIdesiredTime\fR.
   * Sets the factory's notion of the current time to
   * \fIdesiredTime\fR.  \fIdesiredTime\fR is clipped
   * to a value not less than the time that the previous message
   * was sent and not greater than that of the
   * next scheduled message.
   * Returns the adjusted time.
   */
{
    double t;
    if (!inPerformance || performanceIsPaused)
      return clockTime;
    /*** FIXME Should maybe do a gettimeofday and compare with the time
      we've been handed. If they're different, we need to 
      adjust startTime (subtract difference) and then adjust timed entry.
      ***/
    t = MIN(desiredTime,getNextMsgTime(condQueue));
    t = MAX(t,clockTime);
    setTime(t);
    return clockTime;
}

+(double)_getLastTime
  /* See time.m: _MKLastTime(). */ 
{
    return (clockTime != 0.0) ? clockTime : oldClockTime;
}

+_adjustDeltaTThresholds
{
    double dt = -MKGetDeltaT();
    deltaTThresholdLow = dt * (1 - deltaTThresholdLowPC);
    deltaTThresholdHigh = dt * (1 - deltaTThresholdHighPC);
    if (deltaTThresholdLow > deltaTThresholdHigh)
      deltaTThresholdLow = deltaTThresholdHigh;
}

@end













































