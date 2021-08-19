#ifdef SHLIB
#include "shlib.h"
#endif

/*
  PartRecorder.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* Modification History:

   03/13/90/daj - Minor changes for new private category scheme.
   04/21/90/daj - Small mods to get rid of -W compiler warnings.
   08/27/90/daj - Changed to zone API.

*/

#import "_musickit.h"
#import "_noteRecorder.h"
#import "_Conductor.h"
#import "_Instrument.h"
#import "_Part.h"
#import "_ScoreRecorder.h"

#import "PartRecorder.h"
@implementation PartRecorder:Instrument
  /* A simple class which records notes to a part. That is, it acts as
     an Instrument that realizes notes by adding them to its Part.
     */
{
    MKTimeUnit timeUnit;
    id noteReceiver; /* The single instance of noteReceiver. */
    id part;
    id _reservedPartRecorder1;
    BOOL _reservedPartRecorder2; /* YES if part is to be archived when the
			  receiver or any object pointing to the receiver
			  is archived. */
}
#define _archivePart _reservedPartRecorder2
#define _scoreRecorder _reservedPartRecorder1 /* The scoreRecorder owning this 
				       PartRecorder, if any. */
#import "noteRecorderMethods.m"

#define VERSION2 2

+initialize
{
    if (self != [PartRecorder class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: then archives timeUnit. 
     Optionally archives part using NXWriteObjectReference().
     */
{
    [super write:aTypedStream];
    NX_ASSERT((sizeof(MKTimeUnit) == sizeof(int)),"write: method error.");
    NXWriteType(aTypedStream,"i",&timeUnit);
    NXWriteObjectReference(aTypedStream,part);
    NXWriteObjectReference(aTypedStream,_scoreRecorder);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"PartRecorder") == VERSION2) {
	NXReadType(aTypedStream,"i",&timeUnit);
	part = NXReadObject(aTypedStream);
	_scoreRecorder = NXReadObject(aTypedStream);
    }
    return self;
}

-awake
  /* Initializes noteSender instance variable. */
{
    [super awake];
    noteReceiver = [self noteReceiver];
    return self;
}

-copyFromZone:(NXZone *)zone
{
    PartRecorder *newObj = [super copyFromZone:zone];
    newObj->noteReceiver = [newObj noteReceiver];
    return newObj;
}

-free
  /* If receiver is a member of a ScoreRecorder or is in performance, 
     returns self and does nothing. Otherwise frees the receiver. */
{
    if ([self inPerformance] || _scoreRecorder) 
      return self;
    return [super free];
}

-init
  /* TYPE: Creating
   * This message is sent to when a new instance is created.
   * The default implementation returns the receiver and creates a single
   * NoteReceiver.
   * A subclass
   * implementation should first send \fB[super init]\fR.
   */
{
    [super init];
    [self addNoteReceiver:noteReceiver = [NoteReceiver new]];
    timeUnit = MK_second;
    return self;
}

#if 0
-setArchivePart:(BOOL)yesOrNo
 /* Archive part when the receiver or any object pointing to the receiver
    is archived. */  
{
    archivePart = yesOrNo;
}

-(BOOL)archivePart
 /* Dont archive part when the receiver or any object pointing to the 
    receiver is archived. */  
{
    return archivePart;
}
#endif

void _MKSetScoreRecorderOfPartRecorder(aPR,aSR)
    PartRecorder *aPR;
    id aSR;
{
    aPR->_scoreRecorder = aSR;
}

-setPart:aPart
  /* Sets Part to which notes are sent. */
{
    part = aPart;
    return self;
}

-part
{
    return part;
}

-_realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* Private */
{
    if (!_noteSeen) {
	[Conductor _afterPerformanceSel:@selector(_afterPerformance) 
       to:self argCount:0];
	[_scoreRecorder _firstNote:aNote];
	[part _addPerformanceObj:self];
	[self firstNote:aNote];
	_noteSeen = YES;
    }
    return [self realizeNote:aNote fromNoteReceiver:aNoteReceiver];
}

-_afterPerformance
  /* Sent by conductor at end of performance. Private */
{
    [part _removePerformanceObj:self];
    [self afterPerformance];
    _noteSeen = NO;
    return self;
}

-realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* Copies the note, adjusting its timetag and possibly adjusting its 
     duration according to tempo, then sends addNote: to the Part. */
{
    aNote = [aNote copyFromZone:NXDefaultMallocZone()];
    [aNote setTimeTag:_MKTimeTagForTimeUnit(aNote,timeUnit)];
    if ([aNote noteType] == MK_noteDur) 
      [aNote setDur:_MKDurForTimeUnit(aNote,timeUnit)];
    [part addNote:aNote];
    return self;
}

@end















