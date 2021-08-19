#ifdef SHLIB
#include "shlib.h"
#endif

/*
  ScorePerformer.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* Modification history:

   03/17/90/daj - Added delegate mechanism. Added settable PartPerformerClass
   04/21/90/daj - Small mods to get rid of -W compiler warnings.
   08/27/90/daj - Changes to support zone API

*/

#import "_musickit.h"

#import "_Conductor.h"
#import "_PartPerformer.h"

#import "ScorePerformer.h"
@implementation ScorePerformer:Object
  /* A pseudo-performer that does its work by managing a set of PartPerformers.
     Many of the methods resemble Performer methods, but they operate by
     doing a broadcast to the contained PartPerformers.

     Note that while ScorePerformer resembles a Performer, it is not identical
     to a Performer and some care must be taken when using it. For example,
     the method -noteSenders returns the NoteSenders of the PartPerformers.
     The ScorePerformer itself does not have any NoteSenders. Thus, to find
     the name of one of these, you have to specify the owner as the
     PartPerformer, not as the ScorePerformer. If you use the NoteSender
     owner method to determine the owner, you won't have any problem.
   */
{
    MKPerformerStatus status; /* The object's status. */
    id partPerformers;/* A Set of PartPerformers. */
    id score;         /* The Score to which we're assigned. */
    double firstTimeTag; /* The smallest timeTag value considered for
			   performance, as last broadcast to the 
			   PartPerformers. */
    double lastTimeTag;   /* The greatest timeTag value considered for 
			   performance, as last broadcast to the 
			   PartPerformers.  */
    double timeShift;	/* The offset time for the object in beats,
			   as last broadcast to the PartPerformers. */
    double duration;    /* The duration of the object in beats,
			   as last broadcast to the PartPerformers. */
    id conductor;     /* Conductor last broadcast to PartPerformers. */
    id delegate;
    id partPerformerClass;
    BOOL _reservedScorePerformer1; /* YES if score is to be archived when the
			  receiver or any object pointing to the receiver
			  is archived. Default is NO. */
    MKMsgStruct * _reservedScorePerformer2;
    void *_reservedScorePerformer3;
}
#define _archiveScore _reservedScorePerformer1
#define _deactivateMsgPtr _reservedScorePerformer2

+new
{
    self = [super allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize];
    return self;
}

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

-init
{
    [super init];
    partPerformers = [List new];
    partPerformerClass = [PartPerformer class];
    _deactivateMsgPtr = NULL;
    duration = MK_ENDOFTIME;
    lastTimeTag = MK_ENDOFTIME;
    status = MK_inactive;
    conductor = [Conductor defaultConductor];
    return self;
}

#define VERSION2 2

+initialize
{
    if (self != [ScorePerformer class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives partPerformers,firstTimeTag,lastTimeTag,timeShift,
     duration, and partPerformerClass. Also optionally archives score
     conductor and delegate using NXWriteObjectReference().
     */
{
    [super write:aTypedStream];
    NX_ASSERT((sizeof(MKPerformerStatus) == sizeof(int)),
	      "write: method error.");
    NXWriteObject(aTypedStream,partPerformers);
    NXWriteObjectReference(aTypedStream,score);
    NXWriteTypes(aTypedStream,"dddd#",&firstTimeTag,&lastTimeTag,
		 &timeShift,&duration,&partPerformerClass);
    NXWriteObjectReference(aTypedStream,conductor);
    NXWriteObjectReference(aTypedStream,delegate);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked with NXReadObject(). 
     */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"ScorePerformer") == VERSION2) {
	partPerformers = NXReadObject(aTypedStream);
	score = NXReadObject(aTypedStream);
	NXReadTypes(aTypedStream,"dddd#",&firstTimeTag,&lastTimeTag,
		    &timeShift,&duration,&partPerformerClass);
	conductor = NXReadObject(aTypedStream);
	delegate = NXReadObject(aTypedStream);
    }
    return self;
}

-awake
{
    [super awake];
    if (!conductor)
      conductor=[Conductor defaultConductor];
    status = MK_inactive;
    return self;
}


static void unsetPartPerformers(ScorePerformer *self)
{
    id *el;
    unsigned n;
    for (el = NX_ADDRESS(self->partPerformers), 
	 n = [self->partPerformers count]; n--;el++)
      _MKSetScorePerformerOfPartPerformer(*el,nil);
    self->score = nil;
}

-freePartPerformers
  /* Frees all PartPerformers. Returns self. */
{
    unsetPartPerformers(self);
    [partPerformers freeObjects];
    return self;
}

#define FOREACH() for (el = NX_ADDRESS(partPerformers), n = [partPerformers count]; n--; el++)

-removePartPerformers
  /* Sets score to nil and removes all PartPerformers, but doesn't free them.
     Returns self.
     */
{
    unsetPartPerformers(self);
    [partPerformers empty];
    return self;
}

-score
  /* Returns score. */
{
    return score;
}

-activate
  /* If score is not set or Score contains no parts, returns nil. Otherwise, 
     sends activateSelf, broadcasts activate message to contents, and 
     returns self and sets status to MK_active if any one of the
     PartPerformers returns self.
     */ 
{
    id *el;
    unsigned n;
    if (!score || (![score partCount]) || (![self activateSelf]))
      return nil;
    FOREACH()
      if ([*el activate])
	status = MK_active;
    if (status != MK_active)
      return nil;
    _deactivateMsgPtr = MKCancelMsgRequest(_deactivateMsgPtr);
    _deactivateMsgPtr = [Conductor _afterPerformanceSel:
			 @selector(_deactivate) to:self argCount:0];
    if ([delegate respondsTo:@selector(performerDidActivate:)])
      [delegate performerDidActivate:self];
    return self;
}

-activateSelf
  /* TYPE: Performing; Does nothing; subclass may override for special behavior.
   * Invoked from the \fBactivate\fR method,
   * a subclass can implement
   * this method to perform pre-performance activities.
   * If \fBactivateSelf\fR returns \fBnil\fR, the activation of the 
   * PartPerformers is aborted.
   * The default does nothing and returns the receiver.
   */
{
    return self;
}

-setScore:aScore
  /* Snapshots the score over which we will sequence and creates 
     PartPerformers for each Part in the Score in the same order as the
     corresponding Parts. Note that any Parts added to 
     aScore after -setScore: is sent will not appear in the performance. In
     order to get such Parts to appear, you must send setScore: again. 
      If aScore is not the same as the previously specified score, 
     frees all contained PartPerformers.  The PartPerformers are added
     in the order the corresponding Parts appear in the Score. */
{
    if (aScore == score)
      return self;
    if (status != MK_inactive)
      return nil;
    [self freePartPerformers];
    score = aScore;
    if (!score)
      return self;
    {
	unsigned n;
	id aList = [aScore parts];
	id *el,newEl;
	score = aScore;
	for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++) {
	    [partPerformers addObject:newEl = [partPerformerClass new]];
	    [newEl setPart:*el];
	    _MKSetScorePerformerOfPartPerformer(newEl,self);
	}
	[aList free];
    }
    /* Broadcast current state. */ 
    [self setFirstTimeTag:firstTimeTag];
    [self setLastTimeTag:lastTimeTag];
    [self setDuration:duration];
    [self setTimeShift:timeShift];
    [self setConductor:conductor];
    return self;
}

-deactivateSelf
  /* TYPE: Performing
   * Finalization method sent when receiver is deactivated.
   * You never send the \fBdeactivateSelf\fR message directly to an
   * object; it's invoked by \fBdeactivate\fR.
   * Returns the receiver.
   */
{
    return self;
}  

-pause
  /* Broadcasts activate message to contained PartPerformers. */
{
    [partPerformers makeObjectsPerform:@selector(pause)];
    status = MK_paused;
    if ([delegate respondsTo:@selector(performerDidPause:)])
      [delegate performerDidPause:self];
    return self;
}

-resume
  /* Broadcasts activate message to contained PartPerformers. */
{
    [partPerformers makeObjectsPerform:@selector(resume)];
    status = MK_active;
    if ([delegate respondsTo:@selector(performerDidResume:)])
      [delegate performerDidResume:self];
    return self;
}

-_deactivate
{
    [self deactivateSelf];
    status = MK_inactive;
    _deactivateMsgPtr = MKCancelMsgRequest(_deactivateMsgPtr);
    if ([delegate respondsTo:@selector(performerDidDeactivate:)])
      [delegate performerDidDeactivate:self];
    return self;
}

-deactivate
  /* Sends [self deactivateSelf], broadcasts deactivate message to 
     contained PartPerformers and sets status to MK_inactive. */
{
    [self _deactivate];
    [partPerformers makeObjectsPerform:@selector(deactivate)];
    return self;
}

-setFirstTimeTag:(double) aTimeTag
  /* Broadcast setFirstTimeTag: to contained PartPerformers. */
{ 
    id *el;
    unsigned n;
    firstTimeTag = aTimeTag;
    FOREACH()
      [*el setFirstTimeTag:aTimeTag];
    return self;
}		

-setLastTimeTag:(double) aTimeTag
  /* Broadcast setLastTimeTag: to contained PartPerformers. */
{ 
    id *el;
    unsigned n;
    FOREACH()
      [*el setLastTimeTag:aTimeTag];
    lastTimeTag = aTimeTag;
    return self;
}		

-(double)firstTimeTag  
  /* TYPE: Accessing time
   * Returns the value of the receiver's \fBfirstTimeTag\fR variable.
   */
{
    return firstTimeTag;
}

-(double)lastTimeTag 
  /* TYPE: Accessing time
   * Returns the value of the receiver's \fBlastTimeTag\fR variable.
   */
{
    return lastTimeTag;
}


-setTimeShift:(double) aTimeShift
  /* Broadcast setTimeShift: to contained PartPerformers. */
{ 
    id *el;
    unsigned n;
    FOREACH()
      [*el setTimeShift:aTimeShift];
    timeShift = aTimeShift;
    return self;
}		


-setDuration:(double) aDuration
  /* Broadcast setDuration: to contained PartPerformers. */
{ 
    id *el;
    unsigned n;
    duration = aDuration;
    FOREACH()
      [*el setDuration:aDuration];
    return self;
}		


-(double)timeShift 
  /* TYPE: Accessing time
   * Returns the value of the receiver's \fBtimeShift\fR variable.
   */
{
	return timeShift;
}

-(double)duration 
  /* TYPE: Accessing time
   * Returns the value of the receiver's \fBduration\fR variable.
   */
{
	return duration;
}

-copyFromZone:(NXZone *)zone
  /* Copies object. This involves copying firstTimeTag and lastTimeTag. 
     The score of the new object is set with setScore:, creating a new set 
     of partPerformers. */
{
    ScorePerformer *newObj = [super copyFromZone:zone];
    newObj->partPerformers = nil;
    [newObj setScore:score];
    newObj->_deactivateMsgPtr = NULL;
    newObj->status = MK_inactive;
#if 0
    /* This happens automatically */
    newObj->lastTimeTag = lastTimeTag; 
    newObj->firstTimeTag = firstTimeTag;
    newObj->timeShift = timeShift;
    newObj->duration = duration;
    newObj->conductor = conductor;
    newObj->score = score;
#endif
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

-free
  /* Frees contained PartPerformers and self. */
{
    if (status != MK_inactive)
      return self;
    [self freePartPerformers];
    [partPerformers free];
    return [super free];
}

-setConductor:aConductor
  /* Broadcasts setConductor: to contained PartPerformers. */
{
    if ( (conductor=aConductor) ==nil) 
      aConductor=[Conductor defaultConductor];
    if (status == MK_inactive)
      conductor = aConductor;
    else return nil;
    [partPerformers makeObjectsPerform:@selector(setConductor:) with:
     aConductor];
    return self;
}

-partPerformerForPart:aPart
  /* Returns the PartPerformer for aPart, if found. */
{
    id *el;
    unsigned n;
    FOREACH()
      if ([*el part] == aPart)
	return *el;
    return nil;
}

-partPerformers
  /* TYPE: Processing
   * Returns a copy of the List of the receiver's PartPerformer collection.
   * The PartPerformers themselves are not copied. It is the sender's
   * responsibility to free the List.
   */
{
    
    return _MKCopyList(partPerformers);
}

-noteSenders
  /* TYPE: Processing
     Returns a List of the sender's PartPerformers' NoteSenders. 
     It's the caller's responsibility to free the List. */
{
    id *el;
    unsigned n;
    id aList = [List new];
    IMP addImp = [aList methodFor:@selector(addObject:)];
    FOREACH()
      (*addImp)(aList,@selector(addObject:),[*el noteSender]);
    return aList;
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

-setPartPerformerClass:aPartPerformerSubclass
{
    if (!_MKInheritsFrom(aPartPerformerSubclass,[PartPerformer class]))
      return nil;
    partPerformerClass = aPartPerformerSubclass;
    return self;
}

-partPerformerClass
{
    return partPerformerClass;
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

#if 0
-setArchiveScore:(BOOL)yesOrNo
 /* Archive score when the receiver or any object pointing to the receiver
    is archived. */  
{
    archiveScore = yesOrNo;
}

-(BOOL)archiveScore
  /* Returns whether Score is archived when the receiver or any object 
   pointing to the receiver is archived. */
{
    return archiveScore;
}
#endif

@end

