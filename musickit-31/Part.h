/*
  Part.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

@interface Part : Object
/* 
 * 
 * A Part is a time-ordered collection of Notes that can be edited and
 * performed.  Parts are typically grouped together in a Score.
 * 
 * Notes are ordered within a Part by their timeTag values. The Music Kit
 * does not specify the ordering of Notes with the same timeTag.  To move
 * a Note within a Part, you simply change the Note's timeTag by sending
 * it the setTimeTag: message.  Changing a Note's timeTag causes it to be
 * removed from its Part and then re-added with the new timeTag value.  A
 * Note can belong to only one Part at a time.  When you add a Note to a
 * Part, it's automatically removed from its old Part.
 * 
 * You can add Notes to Part by invoking one of Part's addNote: methods,
 * or you can record Notes into it by using a PartRecorder, a type of
 * Instrument that realizes Notes by adding copies of them to a Part.  A
 * Part is associated with a PartRecorder by sending the setPart: message
 * to the PartRecorder.  Any number of PartRecorders can simultaneously
 * record into the same Part.
 * 
 * A Part can be a source of Notes in a performance by associating it
 * with a PartPerformer (again, through setPart:).  During a performance,
 * the PartPerformer sequences over the Notes in the Part, performing
 * them in order.  While you shouldn't free a Part or any of its Notes
 * while an associated PartPerformer is active, you can add Notes to and
 * remove Notes from the Part at any time.  A PartPerformer creates its
 * own List of the Part's Notes when it receives the setPart: message
 * (but keep in mind that it doesn't make copies of the Notes
 * themselves); changes to the Part made during a performance won't
 * affect the PartPerformer.  This allows a Part to be performed by a
 * PartPerformer and used for recording by a PartRecorder at the same
 * time.
 * 
 * Every Part contains an info Note.  An info Note is a sort of header
 * for the Part and can contain any amount and type of information.  Info
 * Notes are typically used to describe a performance setup -- for
 * example, in can list the default attributes of a particular SynthPatch
 * that's used to realize the Part's Notes.  When the Part's Score is
 * written to a scorefile, the info Note is written in the file's header;
 * this is in distinction to the Part's other Notes, which make up the
 * body of the scorefile.  (To store a Part in a scorefile you must first
 * add it to a Score and then write the Score.)
 * 
 * Parts are commonly given string name identifiers.  The most important
 * use of a Part's name is to identify the Part in a scorefile.  The
 * Music Kit maintains its own name table.  To name an object, call the
 * MKNameObject() C function.  See the section on names in Chapter 10,
 * "Music," for more information.
 * 
 * CF:  PartRecorder, PartPerformer, Note, Score
 */
{
	id score;       /* The Score this object is a member of, if any. */
	id notes;       /* The object's List of Notes. */
	id info;        /* The object's info Note. */
	int noteCount;  /* Number of Notes in the Part. */
	BOOL isSorted;  /* YES if the receiver is sorted. */
	id _reservedPart1;
	id _reservedPart2;
	int _reservedPart3;
}
 
 /* METHOD TYPES
  * Creating and freeing a Part
  * Modifying the object
  * Querying the object
  * Adding, moving, and removing Notes
  * Retrieving Notes
  * Setting Note parameters
  */

+ new; 
 /* TYPE: Creating
  * Creates object from NXDefaultMallocZone() and sends init to the new 
  * instance. 
  */

- sort;
- (BOOL)isSorted;
- notesNoCopy;

- combineNotes; 
 /* TYPE: Adding; Creates and adds a noteDur for each noteOn/noteOff pair in the receiver.
  * Creates and adds a single noteDur for each noteOn/noteOff pair in the
  * receiver.  A noteOn/noteOff pair is identified by pairing a noteOn
  * with the earliest subsequent noteOff that has a matching noteTag.  The
  * parameters from the two Notes are merged in the noteDur.  If the
  * noteOn and its noteOff have different values for the same parameter,
  * the value of the noteOn's parameter takes precedence.  The noteDur's
  * duration is the timeTag difference between the two original Notes.
  * After the noteDur is created and added to the receiver, the noteOn and
  * noteOff are removed and freed.  Returns the receiver.  */

- splitNotes; 
 /* TYPE: Adding; Splits the receiver's noteDurs into noteOn/noteOff pairs.
  * Splits the receiver's noteDurs into noteOn/noteOff pairs.  The
  * original noteDur's type is set to noteOn and a noteOff is created (and
  * added) to complement it.  The original parameters and noteTag are
  * divided between the two Notes as described in Note's split:: method.
  * Returns the receiver.  */

- addToScore:newScore; 
 /* TYPE: Modifying; Adds the receiver to newScore.
  * Removes the receiver from its present Score, if any, and adds it to
  * newScore.  Implemented as [newScore addPart:self].  Returns the
  * receiver.  */

- removeFromScore; 
 /* TYPE: Modifying; Removes the receiver from its present Score, if any.
  * Removes the receiver from its present Score, if any.  Implemented as
  * [score removePart:self].  Returns the receiver, or nil if it isn't
  * part of a Score.  */

- init; 
 /* TYPE: Modifying; Inits the receiver.
  * Inits the receiver.  You never invoke this method directly.  A
  * subclass implementation should send [super init] before
  * performing its own initialization.  The return value is ignored.  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- free; 
 /* TYPE: Creating; Frees the receiver and its Notes.
  * Frees the receiver and its Notes (including the info Note).  Removes
  * the receiver's name from the name table.  Returns the receiver.  If
  * the receiver has an active PartPerformer associated with it, this does
  * nothing.  Always returns nil.  */

- freeNotes; 
 /* TYPE: Adding; Removes and frees the receiver's Notes.
  * Removes and frees the receiver's Notes (including the info Note) and
  * sets notes to nil.  Returns the receiver.
  * 
  * You shouldn't invoke this method if the receiver has an active
  * PartPerformer associated with it.  */

- freeSelfOnly; 
 /* TYPE: Creating; Frees the receiver but not its Notes.
  * Frees the receiver but not its Notes.  */

- firstTimeTag:(double)firstTimeTag lastTimeTag:(double)lastTimeTag;
 /* TYPE: Retr; Creates and returns a List of a portion of the receiver's Notes.
  * Creates and returns a List of the receiver's Notes that have timeTag
  * values between firstTimeTag and lastTimeTag (inclusive).  The Notes
  * aren't copied.  The sender is responsible for freeing the List.
  * 
  * The List returned by this method is useful as the argument in messages
  * such as addNotes: (sent to another Part), addNotes:timeShift:, and
  * removeNotes:.  */

- addNote:aNote; 
 /* TYPE: Adding; Adds aNote to the receiver.
  * Removes aNote from its present Part, if any, and adds it to the
  * receiver.  Creates the receiver's notes if it's currently nil (this
  * can only happen if freeNotes was previously invoked).  Returns aNote's
  * old Part, or nil if none.  */

- addNoteCopy:aNote; 
 /* TYPE: Adding; Adds a copy of aNote to the receiver.
  * Copies aNote and adds the new Note to the receiver by invoking
  * addNote:.  Returns the new Note.  */

- removeNote:aNote; 
 /* TYPE: Add; Removes aNote from the receiver.
  * Removes aNote from the receiver.  Returns aNote or nil if not found.
  * 
  * You shouldn't invoke this method if the receiver has an active
  * PartPerformer associated with it.  */

- removeNotes:aList; 
 /* TYPE: Adding; Removes, from the receiver, the Notes in aList.
  * Removes all the Notes the receiver has in common with aList.  Returns
  * the receiver.  CF: firstTimeTag:lastTimeTag: */

- addNoteCopies:aList timeShift:(double )shift; 
 /* TYPE: Adding; Adds timeTag-shifted copies of aList's Notes to the receiver.
  * Copies the Notes in aList, shifts the copies' timeTags by shift beats,
  * and then adds the copies to the receiver (by invoking addNote: for
  * each Note).  Returns the receiver, or nil if aList is nil.  CF:
  * firstTimeTag:lastTimeTag: */

- addNotes:aList timeShift:(double )shift; 
 /* TYPE: Adding; Adds timeTag-shifted Notes from aList to the receiver.
  * For each Note in aList, removes the Note from its present Part (if
  * any), shifts its timeTag by shift beats, and adds it to the receiver
  * (by invoking addNote: for each Note).  Returns the receiver, or nil if
  * aList is nil.  CF: firstTimeTag:lastTimeTag: */

- empty; 
 /* TYPE: Adding; Removes the receiver's Notes but doesn't free them.
  * Removes the receiver's Notes but doesn't free them.  Returns the receiver.
  */

- shiftTime:(double )shift; 
 /* TYPE: Adding; Shifts the receiver's Notes by shift beats.
  * Shifts the timeTags of the receiver's Notes by shift beats.
  * Implemented in terms of addNotes:timeShift:.  Notice that this means
  * the Notes are removed and then readded to the Part.  Returns the
  * receiver.  */

-(unsigned ) noteCount;
 /* TYPE: Querying; Returns the number of Notes in the receiver. */

-(BOOL ) containsNote:aNote; 
 /* TYPE: Querying; YES if the receiver contains aNote.
  * Returns YES if the receiver contains aNote, otherwise NO.  
  */

-(BOOL ) isEmpty; 
 /* TYPE: Querying; YES if the receiver contains no Notes.
  * Returns YES if the receiver contains no Notes, otherwise
  * NO.
  */

- atTime:(double )timeTag; 
 /* TYPE: Retr; Returns the first Note with a timeTag of timeTag.
  * Returns the (first) Note with a timeTag of timeTag, or nil if none.
  * Invokes Note's compareNotes: method if the receiver contains more than
  * one such Note.  */

- atOrAfterTime:(double )timeTag; 
 /* TYPE: Retr; Returns the first Note with a timeTag equal to or greater than timeTag.
  * Returns the first Note with a timeTag equal to or greater than
  * timeTag, or nil if none.  */

- nth:(unsigned )n; 
 /* TYPE: Retr; Returns the nth Note in the receiver.
  * Returns the nth Note (0-based), or nil if n is negative or greater
  * than the receiver's Note count.  */

- atOrAfterTime:(double )timeTag nth:(unsigned )n; 
 /* TYPE: Retr; Returns the nth Note with the appropriate timeTag.
  * Returns the nth Note with a timeTag equal to or greater than timeTag,
  * or nil if none.  */

- atTime:(double )timeTag nth:(unsigned )n; 
 /* TYPE: Retr; Returns the nth Note with a timeTag of timeTag.
  * Returns the nth Note with a timeTag of timeTag. 
  */

- next:aNote; 
 /* TYPE: Retr; Returns the Note immediately following aNote.
  * Returns the Note immediately following aNote, or nil if aNote isn't a
  * member of the receiver, or if it's the receiver's last Note.  For
  * greater efficiency, you should create a List from notes and call the
  * NX_ADDRESS() C function.  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Creates and returns a new Part as a copy of the receiver.
  * Creates and returns a new Part as a copy of the receiver.  The new
  * Part contains copies of recevier's Notes (including the info Note), is
  * added to the same Score as the receiver, but isn't given the
  * receiver's name (if any).  */

- copy;
 /* Returns [self copyFromZone:[self zone]] */

- notes;
 /* TYPE: Retr; Creates and returns a List of the receiver's Notes.
  * Creates and returns a List of the receiver's Notes.  The Notes themselves
  * aren't copied.  The sender is responsible for freeing the List.
  */

- score; 
 /* TYPE: Querying; Returns the receiver's Score.
  * Returns the Score the receiver is a member of, or nil if none.
  */

- info; 
 /* TYPE: Querying; Returns the receiver's info Note.
  * Returns the receiver's info Note.  
  */

- setInfo:aNote; 
 /* TYPE: Modifying; Sets the receiver's info Note to aNote.
  * Sets the receiver's info Note to aNote.  Returns the receiver.  
  */

- write:(NXTypedStream *)aTypedStream;
   /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives Notes, info and name, if any (using MKGetObjectName()). 
     Also optionally archives Score using NXWriteObjectReference(). 
 */
- read:(NXTypedStream *) aTypedStream;
   /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Sets object name (using MKNameObject()), if any.
     Note that -init is not sent to newly unarchived objects.
     See write:. */
- awake;
 /* TYPE: Archiving; Maps noteTag.
   Maps noteTags as represented in the archive file onto a set that is
   unused in the current application. This insures that the integrity
   of the noteTag is maintained. If the Part is unarchived as part of a 
   Score, the noteTags of all Parts in the Score are considered part of a 
   single noteTag space. */ 

@end



