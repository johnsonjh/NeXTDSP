/*
  NoteFilter.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import "Instrument.h"

@interface NoteFilter : Instrument
/*
 * NoteFilter is an abstract class that combines the functionality it
 * inherits from Instrument with the protocol defined in the Performer
 * class.  NoteFilter objects can both receive and send Notes; they're
 * interposed between Performers and Instruments to create a Note
 * processing pipeline.  The subclass responsibility
 * realizeNote:fromNoteReceiver: is passed on to NoteFilter subclasses.
 * Keep in mind that notes must be copied on write or store.  
 */
{
	id noteSenders;     /* Collection of NoteSenders. */
}
 /* METHOD TYPES
  * Creating and freeing a NoteFilter
  * Modifying the object
  * Querying the object
  */

- init;
 /* Creates NoteSenders and sends [super init]. */

- noteSenders; 
 /* TYPE: Querying; Returns a copy of the receiver's List of NoteSenders.
  * Returns a copy of the receiver's List of NoteSenders.  It is the sender's
  * responsibility to free the List.  
  */

-(BOOL ) isNoteSenderPresent:aNoteSender; 
 /* TYPE: Querying; YES if aNoteSender is one of the receiver's NoteSenders.
  * Returns YES if aNoteSender is one of the receiver's NoteSenders.
  * Otherwise returns NO.
  */

- copyFromZone:(NXZone *)zone;
 /* TYPE: Creating; Creates and returns a NoteFilter as a copy of the receiver
  * Creates and returns a NoteFilter as a copy of the receiver.
  * The new object contains copies of the receiver's NoteSenders and
  * NoteReceivers. CF superclass copy
  */

- freeNoteSenders; 
 /* TYPE: Modifying; Removes and frees the receiver's NoteSenders.
  * Removes and frees the receiver's NoteSenders.
  * Returns the receiver.
  */

- removeNoteSenders; 
 /* TYPE: Modifying; Removes all the receiver's NoteSenders.
  * Removes all the receiver's NoteSenders.  Returns the receiver.
  */

- noteSender; 
 /* TYPE: Querying; Returns the receiver's first NoteSender.
  * Returns the receiver's first NoteSender.  This is method should only
  * by invoked if the receiver only contains one NoteSender or if you
  * don't care which NoteSender you get.
  */

- addNoteSender:aNoteSender; 
 /* TYPE: Modifying; Adds aNoteSender to the receiver.
  * Removes aNoteSender from its present owner (if any) and adds it to the
  * receiver.  Returns aNoteSender.  If the receiver is in performance, or if
  * aNoteSender is already a member of the receiver, does nothing and returns
  * nil.  
  */

- removeNoteSender:aNoteSender; 
 /* TYPE: Modifying; Removes aNoteSender from the receiver.
  * Removes aNoteSender from the receiver's List of NoteSenders.
  * Returns aNoteSender.
  * If the receiver is in a performance, does nothing and returns nil.
  */

- free; 
 /* TYPE: Creating; Frees the receiver and its contents
  * Sends freenoteSenders and freenoteReceivers to the receiver
  * then frees the receiver.
  */

- write:(NXTypedStream *) aTypedStream;
   /* TYPE: Archiving; Writes out the object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write: then archives noteSener List. */
- read:(NXTypedStream *) aTypedStream;
   /* TYPE: Archiving; Reads the object. 
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */

@end



