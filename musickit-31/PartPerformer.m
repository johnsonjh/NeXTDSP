#ifdef SHLIB
#include "shlib.h"
#endif

/*
  PartPerformer.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/

/* Modification history:

  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/23/90/daj - Changed to zone API.

*/

#import "_musickit.h"

#import "_Part.h"

#import "PartPerformer.h"
@implementation PartPerformer:Performer
/* PartPerformer performs a Part. */
{
    id nextNote; /* The next note, updated in -perform. */
    id noteSender;/* The one-and-only NoteSender. */
    id part;     /* Part over which we're sequencing. */
    double firstTimeTag; /* The smallest timeTag value considered for
			   performance.  */
    double lastTimeTag;   /* The greatest timeTag value considered for 
			   performance.  */
    id _reservedPartPerformer1;
    id *_reservedPartPerformer2;
    id *_reservedPartPerformer3;
    id _reservedPartPerformer4;
    BOOL _reservedPartPerformer5;
}
/* The simplest Performer, PartPerformer performs a Part. */
#define _list _reservedPartPerformer1    /* Used in performance. */
#define _loc _reservedPartPerformer2   
#define _endLoc _reservedPartPerformer3
#define _scorePerformer _reservedPartPerformer4 

#import "timetagInclude.m"

void _MKSetScorePerformerOfPartPerformer(aPP,aSP)
    PartPerformer *aPP;
    id aSP;
{
    aPP->_scorePerformer = aSP;
}

#define VERSION2 2

+initialize
{
    if (self != [PartPerformer class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

-init
  /* You never send this message. Subclass may implement it but must 
     send [super initialize] before doing its own initialization. Sent
     once when an instance is created. Creates the single noteSender
     and adds the noteSender to the superclass cltn. */
{
    [super init];
    lastTimeTag = MK_ENDOFTIME;
    [self addNoteSender:noteSender = [NoteSender new]];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: then archives firstTimeTag and lastTimeTag.
     Optionally archives part using NXWriteObjectReference().
     */
{
    [super write:aTypedStream];
    NXWriteObjectReference(aTypedStream,part);
    NXWriteTypes(aTypedStream,"dd",&firstTimeTag,&lastTimeTag);
    NXWriteObjectReference(aTypedStream,_scorePerformer);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"PartPerformer") == VERSION2) {
	part = NXReadObject(aTypedStream);
	NXReadTypes(aTypedStream,"dd",&firstTimeTag,&lastTimeTag);
	_scorePerformer = NXReadObject(aTypedStream);
    }
    return self;
}

-awake
  /* Initializes noteSender instance variable. */
{
    [super awake];
    noteSender = [self noteSender];
    return self;
}

-free
  /* If receiver is a member of a ScorePerformer or is active, returns self
     and does nothing. Otherwise frees the receiver. */
{
    if ((status != MK_inactive) || _scorePerformer) 
      return self;
    return [super free];
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
    PartPerformer *newObj = [self copyFromZone:zone];
    newObj->_list = nil;
    newObj->_loc = NULL;
    newObj->_endLoc = NULL;
    newObj->_scorePerformer = nil;
    newObj->noteSender = [newObj noteSender];
    return newObj;
}

-setPart:aPart
  /* Sets Part over which we sequence. 
     If the receiver is active, does nothing and returns nil. Otherwise
     returns self. */
{
    if (status != MK_inactive)
      return nil;
    part = aPart;
    return self;
}

-part
  /* Gets Part over which we sequence. */
{
    return part;
}

-activateSelf
  /* TYPE: Performing
   * Activates the receiver for a performance. The Part is snapshotted at  
   * this time. Any subsequent changes to the Part will not affect the
   * current performance. Returns the receiver. 
   */
{
    double tTag = 0;
    if (!part)
      return nil;
    [part _addPerformanceObj:self];
    _list = [part notes];
    _loc  = NX_ADDRESS(_list);
    _endLoc  = _loc + [_list count];
    nextNote = nil;
    while (_loc != _endLoc) {
	nextNote = *_loc++;
	if ((tTag = [nextNote timeTag]) >= firstTimeTag) 
	  break;
    }
    if (!nextNote) {
 	[_list free];
	_list = nil;
	_loc = _endLoc = NULL;
	[part _removePerformanceObj:self];
	return nil;
    }
//  nextPerform = tTag - firstTimeTag;
    nextPerform = tTag;
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
    [_list  free];
    _list = nil; 
    _loc = _endLoc = NULL;
    [part _removePerformanceObj:self];
    return self;
}

-perform
  /* TYPE: Performing
   * Sends nextNote and specifies the next time to perform.
   * Returns the receiver. You never send this message directly to an instance.
   * Rather, it is invoked by the Conductor.
   * You may override this method, e.g. to modify the note before it is 
   * performed or to modify nextPerform, 
   * but you must send [super perform] to perform the note. 
   */
{
    double t = [nextNote timeTag];
    double tNew;
    [noteSender sendNote:nextNote];
    if ((_loc == _endLoc) || 
	((tNew = [(nextNote = *_loc++) timeTag]) > lastTimeTag))
      [self deactivate];
    else nextPerform = tNew - t;
    return self;
}

@end












