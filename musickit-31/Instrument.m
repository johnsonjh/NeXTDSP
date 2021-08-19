#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Instrument.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* Modification history:

  03/13/90/daj - Changes to support category for private methods.
  03/21/90/daj - Added archiving.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/23/90/daj - Changed to zone API.
  09/26/90/daj & lbj - Added check for [owner inPerformance] in 
                       addNoteReceiver and check for _noteSeen in 
		       freeNoteReceivers.
*/

#import "_musickit.h"

#import "_Conductor.h"
#import "_NoteReceiver.h"
#import "_Instrument.h"

@implementation Instrument:Object
/* Instrument is an abstract superclass that defines the general mechanism
 * for obtaining and realizing Notes during a Music Kit performance.
 * Each subclass of Instrument defines its particular manner of realization
 * by implementing the \fBrealizeNote:\fR or \fBfromNoteReceiver:\fR method.
 * The Instrument subclasses provided by the Music Kit are:
 *
 * .ib
 * .nf
 * \fBSubclass\fR
 * .sp -1v
 * 			\fBRealization\fR
 * MidiOut
 * .sp -1v
 *			Sends MIDI signals to the serial port.
 * NoteFilter
 * .sp -1v
 *			Processes the Note and sends it on.
 * NoteRecorder
 * .sp -1v
 *			Adds the Note to a Score or Part or writes it to a file. 
 * SynthInstrument
 * .sp -1v 	
 * 			Synthesizes a musical note on the DSP.	
 * .iq
 * .fi
 *
 * See the specifications of these classes for details.	
 * 
 * Every Instrument contains a List of NoteReceivers, objects
 * that passively receive Notes during a performance.  
 * When a NoteReceiver receives a Note,
 * \fBrealizeNote:fromNoteReceiver:\fR is sent to its Instrument with the
 * Note as the first argument.  As a convenience, the NoteReceiver's 
 * \fBid\fR is passed as the second argument; a subclass of Instrument may 
 * choose to ignore this information.
 *
 * NoteReceivers are added to and removed from an Instrument through
 * \fBaddNoteReceiver:\fR and \fBremoveNoteReceiver:\fR.
 * These methods are primarily intended to be invoked in the 
 * implementation of an Instrument subclass.
 * For example, every Music Kit subclass of Instrument implements the 
 * \fBinit\fR method to automatically
 * create and add some number of NoteReceivers to each new instance;
 * each new SynthInstrument contains one NoteReceiver by 
 * default, every MidiOut object has 16 (one for each MIDI channel),
 * and so on. 
 *
 * Some operations, such as adding and removing NoteReceivers, are
 * illegal while the Instrument is in performance.  An Instrument
 * is said to be in performance from the time it realizes its
 * first Note until the performance is over. 
 * 
 * CF: NoteReceiver, NoteSender, PerfLink, Performer
 */
{
    id noteReceivers;    /* The object's List of NoteReceivers. */
    BOOL _reservedInstrument1;
    void *_reservedInstrument2;
}
/* METHOD TYPES
 * Creating, initializing, and freeing
 * Modifying the object
 * Querying the object 
 * Performing 
 */

#import "_Instrument.h"


#define VERSION2 2

+initialize
{
    if (self != [Instrument class])
      return self;
    _MKCheckInit();
    [self setVersion:VERSION2];
    return self;
}

+new
  /* Create a new instance and sends [self init]. */
{
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Avoid breaking old apps. */
    return self;
}

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

-init
  /* TYPE: Creating; Initializes the receiver.
   * Initializes the receiver.
   * You never invoke this method directly,
   * it's sent by the superclass when the receiver is created.
   * An overriding subclass method should send \fB[super\ init]\fR
   * before setting its own defaults. 
   */
{
    noteReceivers = [List new];
    return self;
}

-realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* TYPE: Performing; Realizes \fIaNote\fR.
   * Realizes \fIaNote\fR in the manner defined by the subclass.  
   * You never send this message; it's sent to an Instrument
   * as its NoteReceivers receive Note objects.
   */
{
    return self;
}


-firstNote:aNote
  /* TYPE: Performing; Received just before the first Note is realized.
   * You never invoke this method directly; it's sent by the receiver to 
   * itself just before it realizes its first Note.
   * You can subclass the method to perform pre-realization initialization.
   * \fIaNote\fR, the Note that the Instrument is about to realize,
   * is provided as a convenience and can be ignored in a subclass 
   * implementation.  The Instrument isn't considered to be in performance 
   * until after this method returns.
   * The default implementation does nothing and returns the receiver.
   */
{
    return self;
}

- noteReceivers	
  /* TYPE: Querying; Returns a copy of the List of NoteReceivers.
   * Returns a copy of the List of NoteReceivers. The NoteReceivers themselves
   * are not copied.	
   */
{
    return _MKCopyList(noteReceivers);
}

-(BOOL)isNoteReceiverPresent:(id)aNoteReceiver
  /* TYPE: Querying; Returns \fBYES\fR if \fIaNoteReceiver\fR is present.
   * Returns \fBYES\fR if \fIaNoteReceiver\fR is a member of the receiver's 
   * NoteReceiver collection.  Otherwise returns \fBNO\fR.
   */
{
    return ([noteReceivers indexOf:aNoteReceiver] == ((unsigned)-1))? NO : YES;
}

-addNoteReceiver:(id)aNoteReceiver
  /* TYPE: Modifying; Adds \fIaNoteReceiver\fR to the receiver.
   * Removes \fIaNoteReceiver\fR from its current owner and adds it to the 
   * receiver. 
   * You can't add a NoteReceiver to an Instrument that's in performance.
   * If the receiver is in a performance, this message is ignored and nil is
   * returned. Otherwise \fIaNoteReceiver\fR is returned.
   */
{
    id owner = [aNoteReceiver owner];
    if (_noteSeen ||  /* in performance */
	(owner && (![owner removeNoteReceiver:aNoteReceiver]))) 
        /* owner might be in perf */
      return nil;
    [noteReceivers addObject:aNoteReceiver];
    [aNoteReceiver _setOwner:self];
    return aNoteReceiver;
}

-removeNoteReceiver:(id)aNoteReceiver
  /* TYPE: Modifying; Removes \fIaNoteReceiver\fR from the receiver.
   * Removes \fIaNoteReceiver\fR from the receiver and returns it
   * (the NoteReceiver) or \fBnil\fR if it wasn't owned by the receiver.
   * You can't remove a NoteReceiver from an Instrument that's in
   * performance. Returns nil in this case.
   */ 
{
    if ([aNoteReceiver owner] != self)
      return nil;
    if (_noteSeen)
      return nil;
    [noteReceivers removeObject:aNoteReceiver];
    [aNoteReceiver _setOwner:nil];
    return aNoteReceiver;
}

-free
  /* TYPE: Creating; Frees the receiver and its NoteReceivers.
   * Removes and frees the receiver's NoteReceivers and then frees the
   * receiver itself.  
   * The NoteReceiver's connections are severed (see the PerfLink class).
   * This message is ignored if the receiver is in a performance. In this
   * case self is returned; otherwise nil is returned.
   */
{
    if (_noteSeen)
      return self;
    [self freeNoteReceivers];
    [noteReceivers free];
    return [super free];
}

-freeNoteReceivers
  /* TYPE: Creating; Frees the receiver's NoteReceivers.
   * Removes and frees the receiver's NoteReceivers.
   * The NoteReceiver's connections are severed (see the PerfLink class).
   * Returns the receiver. 
   */
{
    id aList;
    if (_noteSeen)
      return nil;
    aList = _MKCopyList(noteReceivers);
    [noteReceivers makeObjectsPerform:@selector(disconnect)];
    [self removeNoteReceivers];
    [aList freeObjects];  /* Split this up because elements may try
				  and remove themselves from noteReceivers
				  when they are freed. */
    [aList free];
    return self;
}

-removeNoteReceivers
  /* TYPE: Modifying; Removes all the receiver's NoteReceivers.
   * Removes all the receiver's NoteReceivers but neither disconnects
   * nor frees them. Returns the receiver.
   */
{
    /* Need to use seq because we may be modifying the cltn. */
    register id *el;
    IMP selfRemoveNoteReceiver;
    unsigned n;
    if (!noteReceivers)
      return self;
    selfRemoveNoteReceiver = [self methodFor:@selector(removeNoteReceiver:)];
#   define SELFREMOVENOTERECEIVER(x)\
    (*selfRemoveNoteReceiver)(self, @selector(removeNoteReceiver:), (x))
    for (el = NX_ADDRESS(noteReceivers), n = [noteReceivers count]; n--; el++)
      SELFREMOVENOTERECEIVER(*el++);
    return self;
}

-(BOOL)inPerformance
  /* TYPE: Querying; Returns \fBYES\fR if first Note has been seen.
   * Returns \fBNO\fR if the receiver has yet to receive a Note object.
   * Otherwise returns \fBYES\fR.
   */
{
    return (_noteSeen);
}    

-afterPerformance 
  /* TYPE: Performing; Sent after performance is finished.
   * You never invoke this method; it's automatically
   * invoked when the performance is finished.
   * A subclass can implement this method to do post-performance
   * cleanup.  The default implementation does nothing and
   * returns the receiver.
   */
{
    return self;
}

-copyFromZone:(NXZone *)zone
  /* TYPE: Creating; Creates and returns a copy of the receiver.
   * Creates and returns a new Instrument as a copy of the receiver.  
   * The new object has its own NoteReceiver collection that contains
   * copies of the receiver's NoteReceivers.  The new NoteReceivers'
   * connections (see the PerfLink class) are copied from 
   * those in the receiver.
   */
{
    Instrument *newObj = [super copyFromZone:zone];
    id *el,newEl;
    unsigned n;
    newObj->_noteSeen = NO;
    newObj->noteReceivers = [List newCount:n = [noteReceivers count]];
    for (el = NX_ADDRESS(noteReceivers); n--;el++) 
      [newObj addNoteReceiver:newEl = [*el copy]];
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

-noteReceiver
  /* TYPE: Querying; Returns the receiver's first NoteReceiver.
   * Returns the first NoteReceiver in the receiver's List.
   * This is particularly useful for Instruments that have only
   * one NoteReceiver.
   */
{
    return [noteReceivers objectAt:0];
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives noteReceiver List. */
{
    [super write:aTypedStream];
    NXWriteObject(aTypedStream,noteReceivers);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Instrument") == VERSION2) 
      noteReceivers = NXReadObject(aTypedStream);
    return self;
}

@end

@implementation Instrument(Private)

-_realizeNote:aNote fromNoteReceiver:aNoteReceiver
  /* Private */
{
    if (!_noteSeen) {
	[Conductor _afterPerformanceSel:@selector(_afterPerformance) 
       to:self argCount:0];
	[self firstNote:aNote];
	_noteSeen = YES;
    }
    return [self realizeNote:aNote fromNoteReceiver:aNoteReceiver];
}

-_afterPerformance
  /* Sent by conductor at end of performance. Private */
{
    [self afterPerformance];
    _noteSeen = NO;
    return self;
}

@end

