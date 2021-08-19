/*
  PartRecorder.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import "Instrument.h"
#import "timeunits.h"

@interface PartRecorder : Instrument
/* 
 * 
 * A PartRecorder is an Instrument that realizes Notes by adding copies
 * of them to a Part.  A PartRecorder's Part is set through the setPart:
 * method.  If the Part already contains Notes, the old Notes aren't
 * removed or otherwise affected by recording into the Part -- the
 * recorded Notes are merged in.
 * 
 * Each PartRecorder contains a single NoteReceiver object.  During a
 * performance, a PartPerformer receives Notes from its NoteReceiver,
 * copies them, and then adds them to its Part object.  The PartRecorder
 * gives each Note a new timeTag and, if it's a noteDur, a new duration.
 * The new timeTag reflects the time in the performance that the Note was
 * received by the object.  The timeTag and the duration are computed
 * either as beats or as seconds.
 * 
 * You can create PartRecorders yourself, or you can use a ScoreRecorder
 * object to create a group of them for you.
 * 
 * CF: ScoreRecorder, Part
 * 
 */
{
    MKTimeUnit timeUnit;   /* How time is interpreted. */
    id noteReceiver;       /* The object's single NoteReceiver. */
    id part;               /* The object's Part. */
    id _reservedPartRecorder1;
    BOOL _reservedPartRecorder2;
}

 /* METHOD TYPES
  * Creating and freeing a PartRecorder
  * Modifying the object
  * Querying the object
  * Realizing Notes
  */

- init; 
 /* TYPE: Modifying; Inits the receiver.
  * Inits the receiver by creating and adding its single
  * NoteReceiver.  You never invoke this method directly.  A subclass
  * implementation should send [super init] before performing its
  * own initialization.  The return value is ignored.  */

-setTimeUnit:(MKTimeUnit)aTimeUnit;
 /* See timeunits.h */
-(MKTimeUnit)timeUnit;
 /* See timeunits.h */

- setPart:aPart; 
 /* TYPE: Modifying; Sets aPart as the receiver's Part.
  * Sets aPart as the receiver's Part.  Returns the receiver.  
  */

- part; 
 /* TYPE: Querying; Returns the receiver's Part object. */

- realizeNote:aNote fromNoteReceiver:aNoteReceiver; 
 /* TYPE: Realizing; Adds aNote to the receiver's Part.
  * Copies aNote, computes and sets the new Note's timeTag (and duration
  * if it's a noteDur), and then adds the new Note to the receiver's Part.
  * aNoteReceiver is ignored.  The timeTag and duration computations use
  * the makeTimeTag: and makeDur: methods defined in NoteRecorder.
  * Returns the receiver.  */

-copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Creates and returns a PartRecorder as a copy of the
  * receiver.  Creates and returns a new PartRecorder as a copy of the
  * receiver.  The new object has its own NoteReciever object but adds
  * Notes to the same Part as the receiver.  */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: then archives timeUnit. 
     Optionally archives part using NXWriteObjectReference().
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Note that -init is not sent to newly unarchived objects.
     See write:. */
- awake;
 /* TYPE: Archiving; Gets object ready for use. 
   Gets object ready for use. */

@end




