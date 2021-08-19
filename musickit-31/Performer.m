#ifdef SHLIB
#include "shlib.h"
#endif

 /*
  Performer.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  12/13/89/daj - Fixed deactivate to always reinitialize _performMsgPtr
  01/09/90/daj - Changed pause/resume mechanism to fix bug 4310.
  03/08/90/daj - Fixed bug in _performerBody (lbj).
  03/13/90/daj - Moved private methods to a category.
  03/17/90/daj - Added delegate mechanism.
  03/21/90/daj - Added archiving.
  03/27/90/daj - Added pauseFor:.
  04/06/90/mmm - Added timeOffset instance var to make -time work as advertised,
                 (i.e., time since activation, not including pauses.)
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  07/23/90/daj - Fixed bug in timeOffset setting. 
  08/01/90/daj - Fixed another bug in timeOffset setting. 
  08/14/90/daj - Fixed bug in awake method (wasn't initializing extra instance vars). 
  08/23/90/daj - Changed to use zone API.
  09/26/90/daj & lbj - Added check for [owner inPerformance] in 
                       addNoteSener and check for _noteSeen in 
		       freeNoteSeners. Added missing method inPerformance.
*/

#import "_musickit.h"

#import "_Conductor.h"
#import "_Note.h"
#import "_NoteSender.h"
#import "_Performer.h"

@implementation Performer:Object
/* A Performer produces a series of time-ordered Note
 * objects and initiates their distribution to 
 * a set of Instruments during a Music Kit performance.
 * Performer is an abstract class which managed an List
   of NoteSenders. These NoteSenders are "Note outputs" of the 
   Performer. For convenience, 
   Performers support a subset of the NoteSender connection methods.
   Sending one of the connection messages to a Performer merely 
   broadcasts the message to its NoteSenders.
   The Performer class creates and frees the List for you.

 * Every Performer object is owned by exactly one Conductor.  
 * Unless you set its Conductor
 * by sending it the \fBsetConductor:\fR message, a Performer
 * is owned by the defaultConductor (see the Conductor class).
 * During a performance, the Conductor sends
 * \fBperform\fR messages to the Performer according
 * to requests scheduled by the Performer.
 *
 * \fBperform\fR is the most important method for a Performer.
 * A subclass responsibility, each implementation of the 
 * method should include two activities:  
 *
 *  * It may send a Note object to one of its NoteSenders.
 *  * It must schedule the next invocation of \fBperform\fR. 
 *
 * A Performer usually sends a Note by sending the \fBsendNote:\fR message
 * to one of its NoteSenders.
 * The Note object to be sent can be supplied in any manner:  for example
 * the Performer
 * can read Notes from a file, or from another object, or it can
 * create them itself.
 *
 * The second step, scheduling the next invocation of \fBperform\fR, is 
 * accomplished simply by setting the value of the variable \fBnextPerform\fR.
 * The value of \fBnextPerform\fR is the amount of time, in beats,
 * that the Conductor waits before sending the next 
 * \fBperform\fR message to the Performer.
 * The \fBperform\fR method should only be invoked in this way --
 * an application shouldn't send the \fBperform\fR message itself. 
 * 
 * To use a Performer in a performance,
 * you must first activate it by invoking its
 * \fBactivate\fR method.  This prepares the Performer
 * by first invoking the \fBactivateSelf\fR method and then scheduling
 * the first \fBperform\fR message request.
 * \fBactivateSelf\fR can be overridden to provide 
 * further initialization of the Performer.  For instance,
 * the PartSegment subclass implements \fBactivateSelf\fR
 * to set the value of \fBnextPerform\fR
 * to the timeTag value of its first Note. 
 *
 * The performance begins when the Conductor factory receives the 
 * \fBstartPerformance\fR message.
 * It's legal to activate a Performer after the performance has started.
 *
 * Sending the \fBdeactivate\fR message removes the Performer
 * from the performance and invokes the \fBdeactivateSelf\fR method.
 * This method can be overridden to implement
 * any necessary finalization, such as freeing contained objects.
 * 
 * During a performance, a Performer can be stopped and restarted by 
 * sending it the
 * \fBpaused\fR and \fBresume\fR messages, respectively. 
 * \fBperform\fR messages destined for a paused Performer are suppressed.
 * When a paused Performer is resumed, it recommences
 * performing from the point at which it was stopped.  
 * (Compare this with the \fBsquelch\fR
 * method, inherited from NoteSender, which doesn't suppress
 * \fBperform\fR messages
 * but simply prevents Notes from
 * being sent.)  
 *
 * Each Performer has two instance variables
 * that can adjust its performance time window:
 * \fBtimeShift\fR, and \fBduration\fR.
 * \fBtimeShift\fR and \fBduration\fR set the time, in beats, that the
 * first Note will be sent and the maximum duration of the Performer's
 * performance,
 * respectively.  A Performer is automatically deactivated if its
 * performance extends beyond \fBduration\fR beats.
 *
 * A Performer has a status, represented as one of the
 * following \fBMKPerformerStatus\fR values:
 * 
 *  * \fBMK_inactive\fR.  A deactivated or not-yet-activated Performer.
 *  * \fBMK_active\fR.  An activated, unpaused Performer.
 *  * \fBMK_paused\fR.  The Performer is activated but currently paused.
 *
 * Some messages can only be sent to an inactive (\fBMK_inactive\fR)
 * Performer.  A Performer's status can be queried with the \fBstatus\fR
 * message.  
 */
{
    id conductor;  /* The object's conductor. */
    MKPerformerStatus status; /* The object's status. */
    int performCount;	/* Number of times the \fBperform\fR
			   message has been received. */
    double timeShift;	/* Performance offset time in beats. */
    double  duration;   /* Performance duration in beats. */
    double  time;    /* the time in beats of the current invocation of 
			perform, if any, otherwise, the time in beats of the 
			last invocation of perform. */
    double  nextPerform;/* Amount of time in beats until the next
					* \fBperform\fR message is sent. */
    id noteSenders;    /* Collection of NoteSenders. */
    id delegate;       
    double _reservedPerformer1;
    double _reservedPerformer2;
    MKMsgStruct *_reservedPerformer3;    
    MKMsgStruct *_reservedPerformer4;
    MKMsgStruct *_reservedPerformer5;
    void *_reservedPerformer6;
}			
#import "_Performer.h"

/* METHOD TYPES
 * Initializing
 * Creating and freeing a Performer
 * Querying the Performer
 * Accessing the Conductor
 * Accessing time
 * Performing
 * Copying the object
 */

#define VERSION2 2

+initialize
{
    if (self != [Performer class])
      return self;
    [self setVersion:VERSION2];
    _MKCheckInit();
    return self;
}

+new
  /* Create a new instance and sends [self init]. */
{
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Avoid breaking pre-2.0 apps. */
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives noteSender List, timeShift, and duration. Also archives
     conductor and delegate using NXWriteObjectReference().
     */
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"@dd",&noteSenders,&timeShift,&duration);
    NXWriteObjectReference(aTypedStream,conductor);
    NXWriteObjectReference(aTypedStream,delegate);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Performer") == VERSION2) {
	NXReadTypes(aTypedStream,"@dd",&noteSenders,&timeShift,&duration);
	conductor = NXReadObject(aTypedStream);
	delegate = NXReadObject(aTypedStream);
    }
    return self;
}

-awake
{
    [super awake];
    _MK_CALLOC(_extraPerformerVars,_extraPerformerInstanceVars,1);
    if (!conductor)
      conductor=[Conductor defaultConductor];
    status = MK_inactive;
    _performMsgPtr= MKNewMsgRequest(0.0,@selector(_performerBody),self,0);
    return self;
}

#import "noteDispatcherMethods.m"

-removeNoteSender:(id)aNoteSender
  /* If aNoteSender is not owned by the receiver, returns nil.
     Otherwise, removes aNoteSender from the receiver's NoteSender List
     and returns aNoteSender.
     For some subclasses, it is inappropriate for anyone
     other than the subclass instance itself to send this message. 
     It is illegal to modify an active Performer. Returns nil in this case,
     else aNoteSender. */
{
    if ([aNoteSender owner] != self)
      return nil;
    if (status != MK_inactive)
      return nil;
    [noteSenders removeObject:aNoteSender];
    [aNoteSender _setOwner:nil];
    return aNoteSender;
}

-addNoteSender:(id)aNoteSender
  /* If aNoteSender is already owned by the receiver or the receiver is 
     not inactive, returns nil.
     Otherwise, aNoteSender is removed from its owner, the owner
     of aNoteSender is set to self, aNoteSender is added to 
     noteSenders (as the last element) and aNoteSender is returned. 
     For some subclasses, it is inappropriate for anyone
     other than the subclass instance itself to send this message. 
     If you override this method, first forward it to super.
     */
{
    id owner = [aNoteSender owner];
    if ((status != MK_inactive) || /* in performance */
	(owner && (![owner removeNoteSender:aNoteSender])))
        /* owner might be in perf */
	return nil;
    [noteSenders addObject:aNoteSender];
    [aNoteSender _setPerformer:self];
    return aNoteSender;
}

/* Conductor control ------------------------------------------------ */

-setConductor:(id)aConductor
  /* TYPE: Accessing the Conductor; Sets the receiver's Conductor to \fIaConductor\fR.
   * Sets the receiver's Conductor to \fIaConductor\fR
   * and returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, else self.
   */
{
    if ( (conductor=aConductor) ==nil) 
      aConductor=[Conductor defaultConductor];
    return self;
}

-conductor
  /* TYPE: Accessing the Conductor;  Returns the receiver's Conductor.
   * Returns the receiver's Conductor.
   */
{
    return conductor;
}

/* Activation and deactivation  --------------------------------- */

-activateSelf
  /* TYPE: Performing; Does nothing; subclass may override for special behavior.
   * Invoked from the \fBactivate\fR method,
   * a subclass can implement
   * this method to perform pre-performance activities.
   * In particular, if the subclass needs to
   * alter the initial \fBnextPerform\fR value, it should be 
   * done here (\fBnextPerform\fR is guaranteed to be 0.0 when this
   * method is invoked).
   * If \fBactivateSelf\fR returns \fBnil\fR, the receiver
   * is deactivated.
   * The default does nothing and returns the receiver.
   */
{
    return self;
}

-deactivateSelf
  /* TYPE: Performing; Does nothing; subclass may override for special behavior.
   * Invoked from the \fBdeactivate\fR method,
   * a subclass can implement
   * this method to perform post-performance activities.
   * The value returned by \fBdeactivateSelf\fR is ignored.
   *
   * The default does nothing and returns the receiver.
   */
{
    return self;
}

/* Perform ------------------------------------------------------------ */

-perform	
  /* TYPE: Performing; Subclass responsibility; sends Notes and sets \fBnextPerform\fR.
   * This is a subclass responsibility 
   * expected to send Notes and set the value of the \fBnextPerform\fR
   * variable.
   * The value returned by perform is ignored.
   */
{
    return [self subclassResponsibility:_cmd];
}



-_performerBody
    /* _performerBody is a private method that wraps around the
       subclass's perform method. */
{	
    /* perform before daemon. */
    switch (status != MK_active)  /* This check might be unnecessary? */
      return nil;
    performCount++;
    time = _performMsgPtr->_timeOfMsg - _timeOffset(self) - _pauseOffset;
    _timeOffset(self) += _pauseOffset;
    _pauseOffset = 0;

    [self perform];

    /* Performer perform after daemon. */
    switch (status) {
      case MK_paused: 
	return self;
      case MK_active:
	_performMsgPtr->_timeOfMsg += nextPerform;
	if (_endTime <= _performMsgPtr->_timeOfMsg) /* Duration expired? */
	  break;
	MKScheduleMsgRequest(_performMsgPtr,conductor);
	return self;
      case MK_inactive:  
	/* Subclass perform method may have sent deactivate */
      	return self;
      default:
	break;
    }
    status = MK_paused;
    return [self deactivate];
}

/* Time window variables ------------------------------------------- */

-setTimeShift:(double)shift
  /* TYPE: Accessing time; Delays performance for \fIshift\fR beats.
   * Sets the begin time of the receiver;
   * the receiver's performance is delayed by \fIshift\fR beats.
   * Returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, else self.
   */
{	
    if (status != MK_inactive) 
      return nil;
    timeShift = shift;
    return self;
}		

-setDuration:(double)dur
  /* TYPE: Accessing time;Sets max duration of the receiver to \fIdur\fR beats.
   * Sets the maximum duration of the receiver to \fIdur\fR beats.
   * Returns the receiver.
   * Illegal while the receiver is active. Returns nil in this case, else self.
   */
{
    if (status != MK_inactive) 
      return nil;
    duration = dur;
    return self;
}		


-(double)timeShift 
  /* TYPE: Accessing time; Returns the receiver's performance begin time.
   * Returns the receiver's performance begin time, as set through
   * \fBsetTimeShift:\fR.
   */
{
	return timeShift;
}

-(double)duration 
  /* TYPE: Accessing time; Returns the reciever's performance duration.
   * Returns the receiver's maximum performance duration, as 
   * set through \fBsetDuration:\fR.
   */
{
	return duration;
}

-(int) status
  /* TYPE: Querying; Returns the receiver's status.
   * Returns the receiver's status as one of the
   * following values:
   *
   *  *   	\fBStatus\fR	\fBMeaning\fR
   *  *		MK_inactive	between performances
   *  *		MK_active	in performance
   *  * 	MK_paused	in performance but currently paused
   *
   * A performer's status is set as a side effect of 
   * methods such as \fBactivate\fR and \fBpause\fR.
   */
{
    return (int) status;
}

-(int) performCount
  /* TYPE: Querying; Returns the number of Notes the receiver has performed.
   * Returns the number Notes the receiver has performed in the
   * current performance.  Does this by counting the number of
   * \fBperform\fR messages it has received. 
   */
{
    return performCount;
}

/* Activation and deactivation ------------------------------------------ */

-activate
  /* TYPE: Performing; Prepares the receiver for a performance.
   * If the receiver isn't \fBMK_inactive\fR, immediately returns the receiver;
   * Otherwise 
   * prepares the receiver for a performance by 
   * setting \fBnextPerform\fR to 0.0, invoking \fBactivateSelf\fR,
   * scheduling the first \fBperform\fR message request with the Conductor,
   * and setting the receiver's status to \fBMK_active\fR.
   * If a subclass needs to alter the initial value of 
   * \fBnextPerform\fR, it should do so in its implementation
   * of the \fBactivateSelf\fR method.
   * Returns the receiver.
   */
{
    double condTime;
    if (status != MK_inactive) 
      return self;
    if (duration <= 0)
      return nil;
    nextPerform = 0;
    if (![self activateSelf])
      return nil;
    performCount = 0;
    condTime = [conductor time];
    self->time = 0.0;
    _timeOffset(self) = condTime + timeShift;
    _performMsgPtr->_timeOfMsg = _timeOffset(self) + nextPerform;
    _endTime = _timeOffset(self) + duration;
    _endTime = MIN(_endTime,  MK_ENDOFTIME);
    if (_endTime <= _performMsgPtr->_timeOfMsg)
      return nil;
    MKScheduleMsgRequest(_performMsgPtr,conductor);
    status = MK_active;
    _deactivateMsgPtr = [Conductor _afterPerformanceSel:
			 @selector(deactivate) to:self argCount:0];
    if ([delegate respondsTo:@selector(performerDidActivate:)])
      [delegate performerDidActivate:self];
    return self;
}

-deactivate
  /* TYPE: Performing; Removes the receiver from the performance.
   * If the receiver's status is already \fBMK_inactive\fR, this
   * does nothing and immediately returns the receiver.
   * Otherwise removes the receiver from the performance, invokes
   * \fBdeactivateSelf\fR, and sets the receiver's status
   * to \fBMK_inactive\fR.
   * Returns the receiver.
   */
{
    if (status == MK_inactive)
      return self;
    _performMsgPtr = MKCancelMsgRequest(_performMsgPtr);
    _pauseForMsgPtr = MKCancelMsgRequest(_pauseForMsgPtr);
    _performMsgPtr = MKNewMsgRequest(0.0,@selector(_performerBody),self,0);
    _deactivateMsgPtr = MKCancelMsgRequest(_deactivateMsgPtr);
    [self deactivateSelf];
    status = MK_inactive;
    if ([delegate respondsTo:@selector(performerDidDeactivate:)])
      [delegate performerDidDeactivate:self];
    return self;
}

/* Creation ------------------------------------------------------- */

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

-init
  /* TYPE: Initializing; Initializes the receiver.
   * Initializes the receiver.
   * You never invoke this method directly,
   * it's sent by the superclass upon creation.
   * An overriding subclass method must send \fB[super\ init]\fR
   * before setting its own defaults. 
   */
{
    noteSenders = [List new];
    _MK_CALLOC(_extraPerformerVars,_extraPerformerInstanceVars,1);
    conductor=[Conductor defaultConductor];
    duration = MK_ENDOFTIME;
    _endTime = MK_ENDOFTIME;
    status = MK_inactive;
    _performMsgPtr= MKNewMsgRequest(0.0,@selector(_performerBody),self,0);
    return self;
}


/* Changing status during a performance ---------------------------------- */

-pause   
  /* TYPE: Performing; Suspends the the receiver's performance.
   * If the receiver is \fBMK_active\fR, this changes its 
   * status to \fBMK_paused\fR, suspends its performance, 
   * and returns the receiver.  
   * Otherwise does nothing and returns the receiver.
   *
   * If you want to free a paused Performer during a performance,
   * you should first send it the \fBdeactivate\fR message.
   */
{  
    if (status == MK_inactive || status == MK_paused) 
      return self;
    _performMsgPtr = MKCancelMsgRequest(_performMsgPtr);
    _pauseOffset -= [conductor time];
    status = MK_paused;
    if ([delegate respondsTo:@selector(performerDidPause:)])
      [delegate performerDidPause:self];
    return self;	
}

-pauseFor:(double)beats
{
    if (beats <= 0.0) 
      return nil;
    [self pause];
    if (_pauseForMsgPtr)/* Already doing a "pauseFor"? */
      MKRepositionMsgRequest(_pauseForMsgPtr,[conductor time] + beats);
    else {             /* New "pauseFor". */
	_pauseForMsgPtr = MKNewMsgRequest([conductor time] + beats,
					  @selector(resume),self,0);
	MKScheduleMsgRequest(_pauseForMsgPtr,conductor);
    }
    return self;
}

-resume
  /* TYPE: Performing; Resumes the receiver's performance.
   * If the receiver is paused, this changes its status to \fBMK_active\fR,
   * resumes its performance and returns the receiver.
   * Otherwise does nothing and returns the receiver.
   */
{
    double resumeTime;
    if (status != MK_paused)
      return self;
    _pauseOffset += [conductor time];
    resumeTime = nextPerform + self->time + _timeOffset(self) + _pauseOffset;
    if (resumeTime > _endTime)
      return nil;
    _performMsgPtr = 
      MKRescheduleMsgRequest(_performMsgPtr,conductor,resumeTime,
			     @selector(_performerBody),self,0);
    _pauseForMsgPtr = MKCancelMsgRequest(_pauseForMsgPtr);
    status = MK_active;
    if ([delegate respondsTo:@selector(performerDidResume:)])
      [delegate performerDidResume:self];
    return self;
}

/* Copy ---------------------------------------------------------------- */


static id copyFields(Performer *self,Performer *newObj)
  /* Same as copy but doesn't copy NoteSenders. */
{
    newObj->timeShift = self->timeShift;
    newObj->duration = self->duration;
    newObj->time = newObj->nextPerform = 0;
    _timeOffset(newObj) = 0;
    newObj->_deactivateMsgPtr = NULL;
    newObj->_performMsgPtr=
      MKNewMsgRequest(0.0,@selector(_performerBody),newObj,0);
    newObj->status = MK_inactive;
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

-copyFromZone:(NXZone *)zone;
  /* TYPE: Copying: Returns a copy of the receiver.
   * Creates and returns a new inactive Performer as
   * a copy of the receiver.  
   * The new object has the same \fBtimeShift\fR and 
   * \fBduration\fR values as the reciever. Its
   * \fBtime\fR and \fBnextPerform\fR variables 
   * are set to 0.0. It has its own noteSenders which contains
   * copies of the values in the receiver's collection. The copies are 
   * added to the collection by addNoteSender:. 
   */
{
    Performer *newObj = [super copyFromZone:zone];
    id *el;
    unsigned n;
    newObj = copyFields(self,newObj);
    newObj->noteSenders = [List newCount:n = [noteSenders count]];
    for (el = NX_ADDRESS(noteSenders); n--; el++) 
      [newObj addNoteSender:[*el copy]];
    return newObj;
}


-free
  /* TYPE: Creating
   * This invokes \fBfreeContents\fR and then frees the receiver
   * and its NoteSenders. This message is ignored if the receiver is not
   * inactive. In this case, returns self; otherwise returns nil.
   */
{
    if (status != MK_inactive)
      return self;
    NX_FREE(_performMsgPtr);
    [self freeNoteSenders];
    [noteSenders free];
    return [super free];
}

-(double)time
/* TYPE: Accessing time; Returns the receiver's latest performance time.
Returns the time, in beats, that the receiver last received the \fBperform\fR
message.  If the receiver is inactive, returns \fBMK_ENDOFTIME\fR.  The return
value is measured from the beginning of the performance and doesn't include any
time that the receiver has been paused.  
*/
{
    return (status != MK_inactive) ? self->time : MK_ENDOFTIME;
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

/* FIXME Needed due to a compiler bug. */
static void setNoteSenders(Performer *newObj,id aList)
{
    newObj->noteSenders = aList;
}

-(BOOL)inPerformance
{
    return status != MK_inactive;
}

@end

@implementation Performer(Private)

-_copyFromZone:(NXZone *)zone
{
    /* This is like copyFromZone: except that the NoteSenders are not copied.
       Instead, a virgin empty List is given. */
    Performer *newObj = [super copyFromZone:zone];
    newObj = copyFields(self,newObj);
    setNoteSenders(newObj,[List new]);
    return newObj;
}

@end


