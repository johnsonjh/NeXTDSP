/*
  PartPerformer.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import "Performer.h"

@interface PartPerformer : Performer
/* 
 * 
 * A PartPerformer object performs the Notes in a particular Part.  Every
 * PartPerformer has exactly one NoteSender.  A PartPerformer is
 * associated with a Part through its setPart: method.  While a single
 * PartPerformer can only be associated with one Part, any number of
 * PartPerformers can by associated with the same Part.  If you're
 * performing a Score, you can use ScorePerformer to create
 * PartPerformers for you (one for each Part in the Score).
 * 
 * When you activate a PartPerformer (through activateSelf) the object
 * copies its Part's List of Notes (it doesn't copy the Notes
 * themselves).  When it's performed, the PartPerformer sequences over
 * its copy of the List, allowing you to edit the Part (by adding or
 * removing Notes) without disturbing the performance -- changes made to
 * a Part during a performance are not seen by the PartPerformer.
 * However, since only the List of Notes is copied but not the Notes
 * themselves, you should neither alter nor free a Part's Notes during a
 * performance.
 * 
 * The Performer timing variables firstTimeTag, lastTimeTag, beginTime,
 * and duration affect the timing and performance duration of a
 * PartPerformer.  Only the Notes with timeTag values between
 * firstTimeTag and lastTimeTag (inclusive) are performed.  A Note's
 * performance time is computed as its timeTag value minus firstTimeTag
 * plus beginTime.  If the newly computed performance time is greater
 * than duration, the Note is suppressed and the PartPerformer is
 * deactivated.
 * 
 * CF: ScorePerformer, Part
 * 
 */
{
    id nextNote;        /* The next note to perform. */ 
    id noteSender;      /* The object's only NoteSender. */
    id part;            /* The Part associated with this object. */
    double firstTimeTag;
    double lastTimeTag;
    id _reservedPartPerformer1;
    id *_reservedPartPerformer2;
    id *_reservedPartPerformer3;
    id _reservedPartPerformer4;
    BOOL _reservedPartPerformer5;
}

 /* METHOD TYPES
  * Modifying the object
  * Querying the object
  * Performing the object
  */

- init;
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver by creating and adding its single NoteSender.  You
  * never invoke this method directly.  A subclass implementation should send
  * [super init] before performing its own initialization.  The return
  * value is ignored.  
  */

- setPart:aPart; 
 /* TYPE: Modifying; Associates the receiver with aPart.
  * Associates the receiver with aPart.  If the receiver is active, this does
  * nothing and returns nil.  Otherwise returns the receiver.  
  */

- part; 
 /* TYPE: Querying;  Returns the receiver's Part object. */

- activateSelf; 
 /* TYPE: Performing; Activation routine; copies the Part's List of
  * Notes.  Activates the receiver for a performance.  The receiver
  * creates a copy of its Part's List of Notes, sets nextNote to the first
  * Note in the List, and sets nextPerform (an instance variable inherited
  * from Performer that defines the time to pperform nextNote) to the
  * Note's timeTag minus firstTimeTag plus beginTime.

  * You never invoke this method directly; it's invoked as part of the
  * activate method inherited from Performer.  A subclass implementation
  * should send [super activateSelf].  If activateSelf returns nil, the
  * receiver isn't activated.  The default implementation returns nil if
  * there aren't any Notes in the receiver's Note List, otherwise it
  * returns the receiver.  The activate method performs further timing
  * checks.  */

- deactivateSelf; 
 /* TYPE: Performing; Deactivation routine; frees the receiver's Note List.
  * Deactivates the receiver and frees its List of Notes.  You never
  * invoke this method directly; it's invoked as part of the deactivate
  * method inherited from Performer.  The return value is ignored.  */

- perform; 
 /* TYPE: Performing; Performs nextNote.
  * Performs nextNote (by sending it to its NoteSender's connections) and
  * then prepares the receiver for its next Note performance.  It does
  * this by seting nextNote to the next Note in its List and setting
  * nextPerform to that Note's timeTag minus the value of firstTimeTag.
  * You never invoke this method directly; it's automatically invoked by
  * the receiver's Conductor during a performance.  A subclass
  * implementation should send [super perform].  The return value is
  * ignored.  */ 

- setFirstTimeTag:(double )aTimeTag; 
 /* Only Notes within the time span from firstTimeTag to lastTimeTag are
  * included in the performance.  Note that the notes are shifted so that
  * the note that begins at firstTimeTag plays as soon as the Performer is
  * activated. */
- setLastTimeTag:(double )aTimeTag; 
 /* See setFirstTimeTag */
-(double ) firstTimeTag; 
 /* See setFirstTimeTag */
-(double )lastTimeTag; 
 /* See setFirstTimeTag */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Creates and returns a copy of the receiver.
  * Creates and returns a new Instrument as a copy of the receiver.  The
  * new object has its own NoteReceiver collection that contains copies of
  * the receiver's NoteReceivers.  The new NoteReceivers' connections (see
  * the NoteReceiver class) are copied from the NoteReceivers in the
  * receiver. CF superclass copy. */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: then archives firstTimeTag and lastTimeTag.
     Optionally archives part using NXWriteObjectReference().
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     Should be invoked via NXReadObject(). 
     Note that -init is not sent to newly unarchived objects.
     See write:. */

- awake;
 /* TYPE: Archiving; Gets object ready for use. 
   Gets object ready for use. */
@end



