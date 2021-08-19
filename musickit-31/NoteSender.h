/*
  NoteSender.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

@interface NoteSender : Object 
/* 
 * During a Music Kit performance, Performer objects perform Notes by
 * sending them to one or more Instrument objects.  The NoteSender class
 * defines the Note-sending mechanism used by Performers; each NoteSender
 * object acts as a Note output for a Performer.  Closely related to
 * NoteSender is the NoteReceiver class, which defines the Note-receiving
 * mechanism used by Instruments.  By separating these mechanisms into
 * distinct classes, any Performer can have multiple outputs and any
 * Instrument, multiple inputs.
 * 
 * A NoteSender is added to a Performer through the latter's
 * addNoteSender: method.  While you can create and add NoteSenders
 * yourself, this is typically done automatically by the Performer when
 * it's created.  You can retrieve the object to which a NoteSender has
 * been added by invoking NoteSender's owner method.
 * 
 * To send Notes from a NoteSender to a NoteReceiver, the two objects must be
 * connected.  This is done through the connect: method:
 * 
 *	[aNoteSender connect:aNoteReceiver]
 * 
 * Every NoteSender and NoteReceiver contains a list of connections.  The
 * connect: method adds either object to the other's list; in other
 * words, the NoteReceiver is added to the NoteSender's list and the
 * NoteSender is added to the NoteReceiver's list.  Both NoteReceiver and
 * NoteSender implement connect: as well as disconnect: and disconnect,
 * methods used to sever connections.  A NoteReceiver can be connected to
 * any number of NoteSenders.  Connections can be established and severed
 * during a performance.
 * 
 * NoteSender's sendNote: method defines the Note-sending mechanism.
 * When a NoteSender receives the message sendNote:aNote, it forwards the
 * Note object argument to its NoteReceivers by sending each of them the
 * message receiveNote:aNote.  sendNote: is invoked when the NoteSender's
 * owner performs (or, for NoteFilter, when it realizes) a Note.  You can
 * toggle a NoteSender's Note-sending capability through the squelch and
 * unsquelch methods; a NoteSender won't send any Notes while it's
 * squelched.  A newly created NoteSender is unsquelched.
 * 
 * CF:  NoteReceiver, Performer, NoteFilter
 */ 
{
    id *noteReceivers;   /* Array of connected NoteReceivers. */
    int connectionCount; /* Number of connections. */
    BOOL isSquelched;    /* YES if the object is squelched. */
    id owner;            /* Performer (or NoteFilter) that owns this object. */
    void *_reservedNoteSender1;
    int _reservedNoteSender2;
    BOOL _reservedNoteSender3;
    short _reservedNoteSender4;
    void *_reservedNoteSender5;
}

 /* METHOD TYPES
  * Creating and freeing a NoteSender
  * Modifying the object
  * Querying the object
  * Sending Notes
  */
 
- owner; 
 /* TYPE: Querying; The Performer (or NoteFilter) that owns the receiver.
 * Returns the Performer (or NoteFilter) that owns the receiver.
 */

- disconnect:aNoteReceiver; 
 /* TYPE: Modifying; Disconnects aNoteReceiver from the receiver.
 * Disconnects aNoteReceiver from the receiver.  Returns the receiver.
 */

-(BOOL ) isConnected:aNoteReceiver; 
 /* TYPE: Querying; YES if aNoteReceiver is a connection.
 * Returns YES if aNoteReceiver is connected to the receiver,
 * otherwise returns NO.
 */

- connect:aNoteReceiver; 
 /* TYPE: Modifying; Connects aNoteReceiver to the receiver.
 * Connects aNoteReceiver to the receiver if aNoteReceiver 
 * isKindOf:NoteReceiver. Returns the receiver.
 */

- squelch; 
 /* TYPE: Modifying; Turns off the receiver's Note-sending capability.
 * Squelches the receiver.  While a receiver is squelched it can't send
 * Notes.  Returns the receiver.
 */

- unsquelch; 
 /* TYPE: Modifying; Undoes the effect of the squelch method.
 * Enables the receiver's Note-sending capability, undoing the effect of 
 * a previous squelch message.  Returns the receiver.
 */

-(BOOL ) isSquelched; 
 /* TYPE: Querying; YES if the receiver is squelched.
 * Returns YES if the receiver is squelched, otherwise returns NO.
 */

-(unsigned)connectionCount;
 /* TYPE: Querying; Returns the number of connected NoteReceivers.
 * Returns the number of NoteReceivers connected to the receiver. 
 */

- connections;
 /* TYPE: Querying; Returns a List of the connected NoteReceivers.
 * Creates and returns a List containing the NoteReceivers that are connected
 * to the receiver.  It's the caller's responsibility to free the
 * List.
 */

- free; 
 /* TYPE: Creating; Frees the receiver.
 * Disconnects and frees the receiver.  You can't free a NoteSender that's in 
 * the process of sending a Note.	
 */

+ new; 
 /* TYPE: Creating
  * Creates object from NXDefaultMallocZone() and sends init to the new 
  * instance. 
  */

- init;
 /* TYPE: Creating
  * Initializes object. Subclass should send [super init] when overriding
    this method */

- disconnect; 
 /* TYPE: Modifying; Disconnects all the receiver's NoteReceivers.
 * Disconnects all the NoteReceivers connected to the receiver.
 * Returns the receiver.
 */

-sendAndFreeNote:aNote atTime:(double)time;
 /* TYPE: Sending; Sends and frees aNote at beat time.
 * Schedules a request (with aNote's Conductor) for 
 * sendAndFreeNote:aNote to be sent to the receiver at time
 * time, measured in beats from the beginning of the
 * performance.  Returns the receiver.
 */

-sendAndFreeNote:aNote withDelay:(double)beats;
 /* TYPE: Sending; Sends and frees aNote after delayTime beats.
 * Schedules a request (with aNote's Conductor) for 
 * sendAndFreeNote:aNote to be sent to the receiver at time
 * delayTime measured in beats from the time this message is received.
 * Returns the receiver.
 */

-sendAndFreeNote:aNote;
 /* TYPE: Sending; Immediately sends aNote and then frees it.
 * Sends the message sendNote:aNote
 * to the receiver and then frees aNote,
 * If the receiver is squelched, aNote isn't sent but it is freed.
 * Returns the receiver.
 */

- sendNote:aNote atTime:(double )time; 
 /* TYPE: Sending; Sends aNote at beat time of the performance.
 * Schedules a request (with aNote's Conductor) for 
 * sendNote:aNote to be sent to the receiver at time
 * time, measured in beats from the beginning of the performance.
 * Returns the receiver.
 */

- sendNote:aNote withDelay:(double )delayTime; 
 /* TYPE: Sending; Sends aNote after delayTime beats.
 * Schedules a request (with aNote's Conductor) for 
 * sendNote:aNote to be sent to the receiver at time
 * delayTime measured in beats from the time this message is received.
 * Returns the receiver.
 */

- sendNote:aNote; 
 /* TYPE: Sending; Immediately sends aNote.
 * Sends the message receiveNote:aNote
 * to each NoteReceiver currently connected to the receiver.
 * If the receiver is squelched, the message isn't sent. 
 * Returns the receiver.
 */

- copy; 
 /* TYPE: Copying
    Same as [self copyFromZone:[self zone]] 
  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Returns a new NoteSender as a copy of the receiver.
 * Creates and returns a new NoteSender with the same
 * connections as the receiver.
 */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes object to archive.
    You never send this message directly.  
    Should be invoked with NXWriteRootObject(). 
    Archives isSquelched and object name, if any. 
    Also optionally archives NoteReceiver List and owner using 
    NXWriteObjectReference(). */

- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object from archive.
    You never send this message directly.  
    Should be invoked via NXReadObject(). 
    See write:. */

@end



