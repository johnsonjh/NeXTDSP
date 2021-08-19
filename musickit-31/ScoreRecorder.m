#ifdef SHLIB
#include "shlib.h"
#endif

/*
  ScoreRecorder.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* Modification History:

   03/13/90/daj - Minor changes for new private category scheme.
   03/17/90/daj - Added settable PartRecorderClass
   04/21/90/daj - Small mods to get rid of -W compiler warnings.
   08/27/90/daj - API changes to support zones

*/

#import "_musickit.h"

#import "_Conductor.h"
#import "_PartRecorder.h"
#import "_ScoreRecorder.h"

@implementation ScoreRecorder:Object
  /* A pseudo-recorder that does its work by managing a set of PartRecorders.
   */
{
    id partRecorders; /* A Set of PartRecorders */
    id score;         /* The Score to which we're assigned. */   
    MKTimeUnit timeUnit;
    id partRecorderClass;
    BOOL _reservedScoreRecorder1; /* YES if score is to be archived when the
			  receiver or any object pointing to the receiver
			  is archived. */
    BOOL _reservedScoreRecorder2;
    void *_reservedScoreRecorder3;
}
#define _noteSeen _reservedScoreRecorder2
#define _archiveScore _reservedScoreRecorder1

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

-init {
    [super init];
    timeUnit = MK_second;
    partRecorders = [List new];
    partRecorderClass = [PartRecorder class];
    return self;
}

#define VERSION2 2

+initialize
{
    if (self != [ScoreRecorder class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives partRecorders, timeUnit and partRecorderClass.
     Also optionally archives score using NXWriteObjectReference().
     */
{
    [super write:aTypedStream];
    NX_ASSERT((sizeof(MKTimeUnit) == sizeof(int)),
	      "write: method error.");
    NXWriteObject(aTypedStream,partRecorders);
    NXWriteObjectReference(aTypedStream,score);
    NXWriteTypes(aTypedStream,"i#",&timeUnit,&partRecorderClass);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked with NXReadObject(). 
     */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"ScoreRecorder") == VERSION2) {
	partRecorders = NXReadObject(aTypedStream);
	score = NXReadObject(aTypedStream);
	NXReadTypes(aTypedStream,"i#",&timeUnit,&partRecorderClass);
    }
    return self;
}

-setScore:aScore
  /* Sets score over which we will sequence and creates PartRecorders for
     each Part in the Score. Note that any Parts added to aScore after
     the setScore call will not appear in the performance. */
{
    id aList;
    id *el,newEl;
    unsigned n;
    if (aScore == score)
      return self;
    if (_noteSeen)
      return nil;
    [self freePartRecorders];
    score = aScore;
    if (!aScore)
      return self;
    aList = [aScore parts];
    for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++) {
	[partRecorders addObject:newEl = [partRecorderClass new]];
	[newEl setPart:*el];
	_MKSetScoreRecorderOfPartRecorder(newEl,self);
    }
    [aList free];
    return self;
}

-score
  /* Returns current score. */
{
    return score;
}

-copyFromZone:(NXZone *)zone
  /* Copies object. This involves copying firstTimeTag and lastTimeTag. 
     The score of the new object is set with setScore:, creating a new set 
     of partRecorders.
     */
{
    ScoreRecorder *newObj = [super copyFromZone:zone];
    newObj->partRecorders = nil;
    [newObj setScore:score];
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

static void unsetPartRecorders(ScoreRecorder *self)
{
    id *el;
    unsigned n;
    for (el = NX_ADDRESS(self->partRecorders), 
	 n = [self->partRecorders count]; n--;el++)
      _MKSetScoreRecorderOfPartRecorder(*el,nil);
    self->score = nil;
}

-freePartRecorders
  /* Frees all PartRecorders. */
{
    unsetPartRecorders(self);
    [partRecorders freeObjects];
    return self;
}

#define FOREACH() for (el = NX_ADDRESS(partRecorders), n = [partRecorders count]; n--; el++)


-removePartRecorders
  /* Sets score to nil and removes all PartRecorders, but doesn't free them.
     Returns self.
     */
{
    unsetPartRecorders(self);
    [partRecorders empty];
    return self;
}

-free
  /* Frees contained PartRecorders and self. */
{
    if ([self inPerformance])
      return self;
    [self freePartRecorders];
    [partRecorders free];
    return [super free];
}


-(MKTimeUnit)timeUnit
  /* TYPE: Querying; Returns the receiver's recording mode.
   * Returns YES if the receiver is set to do post-tempo recording.
   */
{
    return timeUnit;
}

-setTimeUnit:(MKTimeUnit)aTimeUnit
{
    id *el;
    unsigned n;
    if ([self inPerformance] && (timeUnit != aTimeUnit))
      return nil;
    for (el = NX_ADDRESS(partRecorders), n = [partRecorders count]; n--; el++)
      [*el setTimeUnit:aTimeUnit];
    timeUnit = aTimeUnit;
    return self;
}

-partRecorders
  /* TYPE: Processing
   * Returns a copy of the List of the receiver's PartRecorder collection.
   * The PartRecorders themselves are not copied. It is the sender's
   * responsibility to free the List.
   */
{
    
    return _MKCopyList(partRecorders);
}

-(BOOL)inPerformance
  /* YES if the receiver has received notes for realization during
     the current performance. */
{
    return (_noteSeen);
}    

-firstNote:aNote
  /* You receive this message when the first note is received in a given
     performance session, before the realizeNote:fromNoteReceiver: 
     message is sent. You may override this method to do whatever
     you like, but you should send [super firstNote:aNote]. 
     The default implementation returns self. */
{
    return self;
}
-afterPerformance 
  /* You may implement this to do any cleanup behavior, but you should
     send [super afterPerformance]. Default implementation
     does nothing. It is sent once after the performance. */
{
    return self;
}


-noteReceivers
 /* Creates and returns a List of the PartRecorders' NoteReceivers. The 
    NoteReceivers themselves are not copied. It is the sender's 
    responsibility to free the List. */
{
    id *el;
    unsigned n;
    id aList = [List new];
    IMP addImp = [aList methodFor:@selector(addObject:)];
    for (el = NX_ADDRESS(partRecorders), n = [partRecorders count]; n--; el++)
      (*addImp)(aList,@selector(addObject:),[*el noteReceiver]);
    return aList;
}

#if 0
-setArchiveScore:(BOOL)yesOrNo
 /* Archive part when the receiver or any object pointing to the receiver
    is archived. */  
{
    archiveScore = yesOrNo;
}

-(BOOL)archiveScore
  /* Returns whether part is archived whne the receiver or any object 
   pointing to the receiver is archived. */
{
    return archiveScore;
}
#endif

-partRecorderForPart:aPart
  /* Returns the PartRecorder for aPart, if found. */
{
    id *el;
    unsigned n;
    for (el = NX_ADDRESS(partRecorders), n = [partRecorders count]; n--; el++)
      if ([*el part] == aPart)
	return *el;
    return nil;
}

-setPartRecorderClass:aPartRecorderSubclass
{
    if (!_MKInheritsFrom(aPartRecorderSubclass,[PartRecorder class]))
      return nil;
    partRecorderClass = aPartRecorderSubclass;
    return self;
}

-partRecorderClass
{
    return partRecorderClass;
}

@end


@implementation ScoreRecorder(Private)

-(void)_firstNote:aNote
{
    if (!_noteSeen) {
	[Conductor _afterPerformanceSel:@selector(_afterPerformance) 
       to:self argCount:0];
	[self firstNote:aNote];
	_noteSeen = YES;
    }
}

-_afterPerformance
  /* Sent by conductor at end of performance. Private */
{
    [self afterPerformance];
    _noteSeen = NO;
    return self;
}

@end

