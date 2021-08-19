/*
  NoteReceiver.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

@interface NoteReceiver :Object
/* 
 * During a Music Kit performance, Instrument objects realize Notes that
 * are sent to them from Performer objects.  The NoteReceiver class
 * defines the Note-receiving mechanism used by Instruments; each
 * NoteReceiver object acts as a Note input for an Instrument.  Closely
 * related to NoteReceiver is the NoteSender class, which defines the
 * Note-sending mechanism used by Performers.  By separating these
 * mechanisms into distinct classes, any Instrument can have multiple
 * inputs and any Performer, multiple outputs.
 * 
 * A NoteReceiver is added to an Instrument through the latter's
 * addNoteReceiver: method.  While an application can create NoteReceivers
 * and add them to an Instrument, this is typically done by the Instrument itself
 * when it's created.  You can retrieve the object to which a NoteReceiver has
 * been added by invoking NoteReceiver's owner method.
 * 
 * To send Notes from a NoteSender to a NoteReceiver, the two objects must be
 * connected.  This is done through the connect: method:
 *
 * 	[aNoteReceiver connect:aNoteSender]
 *
 * Every NoteReceiver and NoteSender contains a list of connections.  The
 * connect: method adds either object to the other's list; in other
 * words, the NoteReceiver is added to the NoteSender's list and the
 * NoteSender is added to the NoteReceiver's list.  Both NoteReceiver and
 * NoteSender implement connect: as well as disconnect: and disconnect,
 * methods used to sever connections.  A NoteReceiver can be connected to
 * any number of NoteSenders.  Connections can be established and severed
 * during a performance.
 *
 * The Note-receiving mechanism is defined in the receiveNote: method.
 * When a NoteReceiver receives the message receiveNote: it sends the
 * message realizeNote:fromNoteReceiver: to its owner, with the received
 * Note as the first argument and its own id as the second.  receiveNote:
 * is automatically invoked when a connected NoteSender sends a Note.
 * You can toggle a NoteReceiver's Note-forwarding capability through the
 * squelch and unsquelch methods; a NoteReceiver ignores the Notes it
 * receives while it's squelched.  A newly created NoteReceiver is
 * unsquelched.
 * 
 * CF:  NoteSender, Instrument
 */
{
    id noteSenders;       /* List of connected NoteSenders. */
    BOOL isSquelched;     /* YES if the object is squelched. */
    id owner;             /* Instrument that owns NoteReceiver. */
    void *_reservedNoteReceiver1;
    void *_reservedNoteReceiver2;
}
 
 /* METHOD TYPES
  * Creating and freeing a NoteReceiver
  * Modifying the object
  * Querying the object
  * Receiving Notes
  */

- owner; 
 /* TYPE: Querying; The Instrument that owns the receiver.
  * Returns the Instrument that owns the receiver.
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

- free; 
 /* TYPE: Creating; Frees the receiver.
  * Disconnects and frees the receiver.
  */

- disconnect:aNoteSender; 
 /* TYPE: Modifying; Disconnects aNoteSender from the receiver.
  * Disconnects aNoteSender from the receiver.  
  * Returns the receiver.
  */

- disconnect; 
 /* TYPE: Modifying; Disconnects all the receiver's NoteSenders.
 * Disconnects all the NoteSenders connected to the receiver.
 * Returns the receiver.
 */

-(BOOL ) isConnected:aNoteSender; 
 /* TYPE: Querying; YES if aNoteSender is a connection.
 * Returns YES if aNoteSender is connected to the receiver,
 * otherwise returns NO.
 */

- connect:aNoteSender; 
 /* TYPE: Modifying; Connects aNoteSender to the receiver.
 * Connects a aNoteSender to the receiver if aNoteSender isKindOf:
 * NoteSender. Returns the receiver.
 */

- squelch; 
 /* TYPE: Modifying; Turns off the receiver's Note-forwarding capability.
 * Squelches the receiver.  While a receiver is squelched it can't send
 * the realizeNote:fromNoteReceiver: message to its owner.
 * Returns the receiver.
 */

- unsquelch; 
 /* TYPE: Modifying; Undoes the effect of the squelch method.
 * Enables the receiver's Note-forwarding capability, undoing
 * the effect of a previous squelch message. 
 * Returns the receiver.
 */

-(BOOL ) isSquelched; 
 /* TYPE: Querying; YES if the receiver is squelched.
 * Returns YES if the receiver is squelched, otherwise returns NO.
 */

- copy; 
 /* TYPE: Copying
    Same as [self copyFromZone:[self zone]] 
  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Returns a new NoteReceiver as a copy of the receiver.
 * Creates and returns a new NoteReceiver with the same
 * connections as the receiver.
 */

-(unsigned)connectionCount;
 /* TYPE: Querying; Returns the number of connected NoteSenders.
 * Returns the number of NoteSenders connected to the receiver. 
 */

- connections;
 /* TYPE: Querying; Returns a List of the connected NoteSenders.
 * Creates and returns a List containing the NoteSenders that are connected
 * to the receiver.  It's the caller's responsibility to free the
 * List.
 */

- receiveNote:aNote; 
 /* TYPE: Receiving; Receives the Note aNote.
 * If the receiver isn't squelched, this sends the message
 * 
 * [owner realizeNote:aNote fromNoteReceiver:self];
 *
 * thereby causing aNote to be realized by the receiver's owner.
 * You never send receiveNote: directly to a NoteReceiver;
 * it's sent as part of a NoteSender's sendNote: method.
 * Returns the receiver, or nil if the receiver is squelched.
 */

- receiveNote:aNote atTime:(double )time; 
 /* TYPE: Receiving; Receives aNote at beat time of the performance.
 * Schedules a request (with aNote's Conductor) for 
 * receiveNote:aNote to be sent to the receiver at time
 * time, measured in beats from the beginning of the
 * performance.  Returns the receiver.
 */

- receiveNote:aNote withDelay:(double )delayTime; 
 /* TYPE: Receiving; Receives aNote after delayTime beats.
 * Schedules a request (with aNote's Conductor) for 
 * receiveNote:aNote to be sent to the receiver at time
 * delayTime, measured in beats from the time this message
 * is received.  Returns the receiver.
 */

- receiveAndFreeNote:aNote withDelay:(double )delayTime; 
 /* TYPE: Receiving; Receives and frees aNote after delayTime beats.
 * Schedules a request (with aNote's Conductor) for 
 * receiveAndFreeNote:aNote to be sent to the receiver at time
 * delayTime, measured in beats from the time this message
 * is received.  Returns the receiver.
 */

- receiveAndFreeNote:aNote; 
 /* TYPE: Receiving; Receives and frees aNote.
 * Sends the message receiveNote:aNote to the receiver and
 * then frees the Note.
 * Returns the receiver.
 */

- receiveAndFreeNote:aNote atTime:(double )time; 
 /* TYPE: Receiving; Receives and frees aNote at beat time.
 * Schedules a request (with aNote's Conductor) for 
 * receiveAndFreeNote:aNote to be sent to the receiver at time
 * time, measured in beats from the beginning of the perfromance.
 * Returns the receiver.
 */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes object.
    You never send this message directly.  
    Should be invoked with NXWriteRootObject(). 
    Archives isSquelched and object name, if any. 
    Also optionally archives elements of NoteSender List and owner using 
    NXWriteObjectReference(). */
- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object.
    You never send this message directly.  
    Should be invoked via NXReadObject(). 
    See write:. */

@end



