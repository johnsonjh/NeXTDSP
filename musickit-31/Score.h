/*
  Score.h
  Copyright 1988, NeXT, Inc.

  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

@interface Score : Object
/* 
 * A Score is a collection of Part objects.  Scores can be read from and
 * written to a scorefile or midifile, performed with a ScorePerformer,
 * and an be used to record Notes from a ScoreRecorder.
 * 
 * Each Score has an info Note (a mute) that defines, in its parameters,
 * nformation that can be useful in performing or otherwise interpreting
 * the Score.  Typical information includes tempo, DSP headroom (see the
 * Orchestra lass), and sampling rate (the parameters MK_tempo,
 * MK_headroom, and MK_samplingRate are provided to accommodate this
 * utility).
 * 
 * When you read a scorefile into a Score, a Part object is created and
 * added to the Score for each Part name in the file's part statement.
 * If the Score already contains a Part with the same name as a Part in
 * the file, the Notes from the two sources are merged together in the
 * existing Part in the Score.
 * 
 * ScoreFile print statements are printed as the scorefile is read into a
 * Score.  You can set the stream on which the messages are printed by
 * invoking setScorefilePrintStream:.  
 */ 
{
    id parts;                       /* The object's collection of Parts. */
    NXStream *scorefilePrintStream; /* The stream used by scorefile print 
                                       statements. */
    id info;                        /* The object's info Note. */
    void *_reservedScore1;
}
 
 /* METHOD TYPES
  * Creating and freeing a Score
  * Modifying the object
  * Querying the object
  * Reading and writing files
  */
+ new; 
 /* TYPE: Creating
  * Creates object from NXDefaultMallocZone() and sends init to the new 
  * instance. 
  */

- init;
 /* TYPE: Modifying; Initializes the receiver.
  Inits the receiver.  You never invoke this method directly.  A subclass
 implementation should send [super init] before performing its own
 initialization.  The return value is ignored.  
 */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- free;
 /* TYPE: Creating;
 Frees the receiver and its contents.
 */

- freeNotes; 
 /* TYPE: Creating; Frees the receiver's Notes. 
 Removes and frees the Notes contained in the receiver's Parts.
 Also frees the receiver's info Note.  Returns the receiver.
 */

- freeParts; 
 /* TYPE: Modifying;  Removes and frees the receiver's Parts and Notes.
 Removes and frees the receiver's Parts and the Notes contained therein.
 Doesn't free the receiver's info Note.  Parts that are currently
 being performed by a PartPerformer aren't freed.
 Returns the receiver.
 */

- freePartsOnly; 
 /* TYPE: Modifying;  Removes and frees the receiver's Parts.
 Removes and frees the receiver's Parts but doesn't free the Notes contained
 therein.  Parts that are currently being performed by a PartPerformer aren't
 freed.  Returns the receiver.  
 */

- freeSelfOnly; 
 /* TYPE: Creating; Frees the receiver but not its Parts nor their Notes.
 Frees the receiver but not its Parts nor their Notes.  The info Note isn't
 freed.  Returns the receiver.  
 */

- empty; 
 /* TYPE: Modifying; Removes the receiver's Parts.
 Removes the receiver's Parts but doesn't free them.
 Returns the receiver.
  */

- readScorefile:(char * )fileName; 
 /* TYPE: Reading;
 Opens the scorefile named fileName and merges its contents
 with the receiver.  The file is automatically closed.
 Returns the receiver or nil if the file couldn't be read.
 */

- readScorefileStream:(NXStream *)stream; 
 /* TYPE: Reading;
 Reads the scorefile pointed to by stream into the receiver.
 The file must be open for reading; the sender is responsible
 for closing the file.
 Returns the receiver or nil if the file couldn't be read.
 */

- readScorefile:(char * )fileName firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag timeShift:(double)timeShift; 
 /* TYPE Reading;
 The same as readScorefile:, but only those Notes with timeTags
 in the specified range are added to the receiver.  The
 added Notes' timeTags are shifted by timeShift beats.
 Returns the receiver or nil if the file couldn't be read.
 */

- readScorefileStream:(NXStream *)stream firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag timeShift:(double)timeShift; 
 /* TYPE Reading;
 The same as readScorefileStream:, but only those Notes with timeTags
 in the specified range are added to the receiver.  The
 added Notes' timeTags are shifted by timeShift beats.
 Returns the receiver or nil if the file couldn't be read.
 */

- readScorefile:(char * )fileName firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag;
 /* TYPE Reading;
 The same as readScorefile:firstTimeTag:lastTimeTag:timeShift:, but 
 timeShift is assumed to be 0.0. */

- readScorefileStream:(NXStream *)stream firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag;
 /* TYPE Reading;
 The same as readScorefileStream:firstTimeTag:lastTimeTag:timeShift:, 
 but timeShift is assumed to be 0.0. */

- writeScorefile:(char * )aFileName; 
 /* TYPE: Reading;
 Opens the scorefile named fileName and writes the receiver
 to it (the file is overwritten).  The file is automatically closed.
 Returns the receiver or nil if the file couldn't be written.
 */

- writeScorefileStream:(NXStream *)aStream; 
 /* TYPE: Reading;
 Writes the receiver into the scorefile pointed to by stream.
 The file must be open for reading; the sender is responsible for
 closing the file.  Returns the receiver or nil if the file couldn't be
 written.  
 */

- writeScorefile:(char * )aFileName firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag timeShift:(double)timeShift; 
 /* TYPE: Reading;
 The same as writeScorefile:, but only those Notes with timeTags
 in the specified range are written to the file.  The
 written Notes' timeTags are shifted by timeShift beats.
 Returns the receiver or nil if the file couldn't be written.
 */

- writeScorefileStream:(NXStream *)aStream firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag timeShift:(double)timeShift; 
 /* 
 The same as writeScorefileStream:, but only those Notes with timeTags
 in the specified range are written to the file.  The
 written Notes' timeTags are shifted by timeShift beats.
 Returns the receiver or nil if the file couldn't be written.
 */

- writeScorefile:(char * )aFileName firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag;
 /* Same as writeScorefileStream:firstTimeTag:lastTimeTag: but timeShift 
    assumed to be 0.0. */

- writeScorefileStream:(NXStream *)aStream firstTimeTag:(double )firstTimeTag lastTimeTag:(double )lastTimeTag;
 /* Same as writeScorefileStream:firstTimeTag:lastTimeTag:timeShift: but
    timeShift assumed to be 0.0. */

-writeOptimizedScorefile:(char *)aFileName;
-writeOptimizedScorefileStream:(NXStream *)aStream;
-writeOptimizedScorefile:(char *)aFileName firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag;
- writeOptimizedScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double )firstTimeTag 
 lastTimeTag:(double )lastTimeTag ;
-writeOptimizedScorefile:(char *)aFileName firstTimeTag:(double)firstTimeTag 
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift;
- writeOptimizedScorefileStream:(NXStream *)aStream 
 firstTimeTag:(double )firstTimeTag 
 lastTimeTag:(double )lastTimeTag 
 timeShift:(double)timeShift; 
 /* 
 These are the same as the analagous writeScorefile methods, except that they
 write the Scorefile in optimized format.
 */

- readMidifile:(char *)aFileName firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag timeShift:(double) timeShift;
- readMidifileStream:(NXStream *) aStream firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag timeShift:(double) timeShift;
- readMidifile:(char *)aFileName firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag;
- readMidifileStream:(NXStream *) aStream firstTimeTag:(double) firstTimeTag
    lastTimeTag:(double) lastTimeTag;
-readMidifile:(char *)fileName;
-readMidifileStream:(NXStream *)aStream;
 /* Reads a Standard Midifile as described in the NeXT Reference Manual. */

-writeMidifile:(char *)aFileName firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift;
-writeMidifileStream:(NXStream *)aStream firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag timeShift:(double)timeShift;
-writeMidifile:(char *)aFileName firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag;
-writeMidifileStream:(NXStream *)aStream firstTimeTag:(double)firstTimeTag
 lastTimeTag:(double)lastTimeTag;
-writeMidifileStream:(NXStream *)aStream ;
-writeMidifile:(char *)aFileName;
 /* Writes a Standard Midifile as described in the NeXT Reference Manual. */

-(unsigned ) noteCount; 
 /* TYPE: Querying;
   Returns the number of Notes in all the receiver's Parts.
   */

- addPart:aPart; 
 /* TYPE: Modifying;
   Adds aPart to the receiver.  The Part is first removed from the Score
   that it's presently a member of, if any.  Returns aPart, or nil
   if it's already a member of the receiver.  
   */

- removePart:aPart; 
 /* TYPE: Modifying;
   Removes aPart from the receiver.  Returns aPart 
   or nil if it wasn't a member of the receiver.
   */

- shiftTime:(double )shift; 
 /* TYPE: Modifying;
   Shifts the timeTags of all receiver's Notes by shift beats.
   Returns the receiver.
   */

-(BOOL ) isPartPresent:aPart; 
 /* TYPE: Querying;
   Returns YES if aPart has been added to the receiver,
   otherwise returns NO.
   */

- midiPart:(int )aChan; 
  /* TYPE: Querying.
     Returns the first Part with a MK_midiChan info parameter equal to
     aChan, if any. aChan equal to 0 corresponds to the Part representing
     MIDI system and channel mode messages. */

-(unsigned )partCount;
 /* TYPE: Querying;
   Returns the number of Part contained in the receiver.
   */

- parts;
 /* TYPE: Querying
   Creates and returns a List containing the 
   receiver's Parts.  The Parts themselves
   aren't copied. It is the sender's repsonsibility to free the List. 
   */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating;
   Creates and returns a new Score as a copy of the receiver.
   The receiver's Part, Notes, and info Note are all copied.
   */

- copy;
 /* Returns [self copyFromZone:[self zone]] */

-setInfo:aNote;
 /* TYPE: Modifying;
   Sets the receiver's info Note to a copy of aNote.  The receiver's
   previous info Note is removed and freed.
   */

-info;
 /* TYPE: Querying;
   Returns the receiver's info Note.
   */

-setScorefilePrintStream:(NXStream *)aStream;
 /* TYPE: Modifying;
   Sets the stream used by ScoreFile print statements to
   aStream.  Returns the receiver.
   */  

-(NXStream *)scorefilePrintStream;
 /* TYPE: Querying;
   Returns the receiver's ScoreFile print statement stream.
   */

- write:(NXTypedStream *)aTypedStream;
 /* TYPE: Archiving; Writes object.
   You never send this message directly.  
   Should be invoked with NXWriteRootObject(). 
   Archives Notes and info. Also archives Score using 
   NXWriteObjectReference(). */
- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object.
   You never send this message directly.  
   Should be invoked via NXReadObject(). 
   Note that -init is not sent to newly unarchived objects.
   See write:. */
- awake;
 /* TYPE: Archiving; Maps noteTag.
   Maps noteTags as represented in the archive file onto a set that is
   unused in the current application. This insures that the integrity
   of the noteTag is maintained. The noteTags of all Parts in the Score are 
   considered part of a single noteTag space. */ 

@end




