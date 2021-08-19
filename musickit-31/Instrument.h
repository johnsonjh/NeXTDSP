/*
  Instrument.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>
#import "NoteReceiver.h"

@interface Instrument : Object
/* 
 * 
 * Instrument is an abstract class that defines the general mechanism for
 * obtaining and realizing Notes during a Music Kit performance.  Each
 * subclass of Instrument defines its particular manner of realization by
 * implementing realizeNote:fromNoteReceiver:.
 * 
 * Every Instrument contains a List of NoteReceivers, objects that
 * receive Notes during a performance.  Each subclass of Instrument
 * should implement its init method to automatically create and add
 * some number of NoteReceivers to a newly created instance.  When a
 * NoteReceiver receives a Note (through the receiveNote: method), it
 * causes realizeNote:fromNoteReceiver: to be sent to its Instrument with
 * the Note as the first argument and the NoteReceiver's id as the second
 * argument.
 * 
 * An Instrument is considered to be in performance from the time it
 * realizes its first Note until the peformance is over.
 * 
 * The Instrument subclasses provided by the Music Kit are:
 * 
 * Subclass                    Realization
 * 
 * NoteFilter           Processes the Note and sends it on.
 * NoteRecorder         Adds the Note to a Part or writes it to a file. 
 * SynthInstrument      Synthesizes a musical sound on the DSP.    
 *
 * CF:  NoteReceiver
 */
{
    id noteReceivers; /* The object's List of NoteReceivers. */
    BOOL _reservedInstrument1;
    void *_reservedInstrument2;
}
 /* METHOD TYPES
  * Creating and freeing an Instrument
  * Modifying the object
  * Querying the object 
  * Performing 
  */

+ new; 
 /* TYPE: Creating
  * Creates object from NXDefaultMallocZone() and sends init to the new 
  * instance. 
  */

- init; 
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver.  You never invoke this method directly.  A
  * subclass implementation should send [super init] before
  * performing its own initialization.  The return value is ignored.  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- realizeNote:aNote fromNoteReceiver:aNoteReceiver; 
 /* TYPE: Performing; Realizes aNote.  You never invoke this method.
  * Realizes aNote in the manner defined by the subclass.  aNoteReceiver
  * is the NoteReceiver that received aNote.  The default implementation
  * does nothing.  You never invoke this method; it's automatically
  * invoked as the receiver's NoteReceivers receive Notes.  The return
  * value is ignored.  Keep in mind that notes must be copied on write or
  * store.  */
 
- firstNote:aNote; 
 /* TYPE: Performing; Received just before the first Note is realized.
  * You never invoke this method; it's invoked just before the receiver
  * realizes its first Note.  A subclass can implement this method to
  * perform pre-realization initialization.  aNote, the Note that the
  * Instrument is about to realize, is provided as a convenience and can
  * be ignored in a subclass implementation.  The receiver is considered
  * to be in performance after this method returns.  The return value is
  * ignored.  */

- noteReceivers; 
 /* TYPE: Querying; Returns a copy of the receiver's List of NoteReceivers.
  * Returns a copy of the receiver's List of NoteReceivers.  The
  * NoteReceivers themselves aren't copied.  It's the sender's
  * responsibility to free the List.  */

-(BOOL ) isNoteReceiverPresent:aNoteReceiver; 
 /* TYPE: Querying; Returns YES if the receiver contains aNoteReceiver.
  * Returns YES if aNoteReceiver is in the receiver's NoteReceiver List.
  * Otherwise returns NO.  */

- addNoteReceiver:aNoteReceiver; 
 /* TYPE: Modifying; Adds aNoteReceiver to the receiver.
  * Adds aNoteReceiver to the receiver, first removing it from it's
  * current Instrument, if any.  If the receiver is in performance, does
  * nothing and returns nil, otherwise returns aNoteReceiver.  */

- removeNoteReceiver:aNoteReceiver; 
 /* TYPE: Modifying; Removes aNoteReceiver from the receiver.
  * Removes aNoteReceiver from the receiver's NoteReceiver List.  If the
  * receiver is in performance, does nothing and returns nil, otherwise
  * returns aNoteReceiver.  */

- free; 
 /* TYPE: Creating; Frees the receiver and its NoteReceivers.
  * Sends freeNoteReceivers to self and then frees the receiver.  If the
  * receiver is in performance, does nothing and returns the receiver,
  * otherwise returns nil.  */

- freeNoteReceivers; 
 /* TYPE: Modifying; Removes and frees the receiver's NoteReceivers.
  * Disconnects, removes, and frees the receiver's NoteReceivers.  Returns
  * the receiver.  */

- removeNoteReceivers; 
 /* TYPE: Modifying; Removes all the receiver's NoteReceivers.
  * Removes all the receiver's NoteReceivers but neither disconnects nor
  * frees them. Returns the receiver.  */

-(BOOL ) inPerformance;
 /* TYPE: Querying; Returns YES if the receiver is in performance.
  * Returns YES if the receiver is in performance (it has received its first
  * Note).  Otherwise returns NO.  
  */

- afterPerformance; 
 /* TYPE: Performing; Invoked automatically after the performance is finished.
  * You never invoke this method; it's automatically invoked when the
  * performance is finished.  A subclass can implement this method to do
  * post-performance cleanup.  The default implementation does nothing;
  * the return value is ignored.  */

- copy; 
 /* TYPE: Copying
    Same as [self copyFromZone:[self zone]] 
  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Creates and returns a copy of the receiver.
  * Creates and returns a new Instrument as a copy of the receiver.  The
  * new object has its own NoteReceiver collection that contains copies of
  * the receiver's NoteReceivers.  The new NoteReceivers' connections (see
  * the NoteReceiver class) are copied from the NoteReceivers in the
  * receiver.  */

- noteReceiver; 
 /* TYPE: Querying; Returns the receiver's first NoteReceiver.
  * Returns the first NoteReceiver in the receiver's List.  This is useful
  * for Instruments that have only one NoteReceiver.  */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes out the object.
  * You never send this message directly.  Should be invoked with
  * NXWriteRootObject().  Archives noteReceiver List. */

- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads the object. 
  * You never send this message directly.  
  * Should be invoked via NXReadObject(). 
  * Note that -init is not sent to newly unarchived objects.
  * See write:. */

@end



