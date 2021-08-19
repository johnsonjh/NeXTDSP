/*
  Note.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

 /* NoteTag allocation. */
extern unsigned MKNoteTag(void);
extern unsigned MKNoteTags(unsigned n);

 /* Maps MIDI value (such as velocity) onto an amplitude scaler such that 
    64->1.0, 127->10.0, and 0->0. This is primarily designed for scaling 
    amplitude by a value derived from MIDI velocity. */
extern double MKMidiToAmp(int midiValue);

 /* Same as above, but uses sensitivity to control how much effect 
    midiValue has.  */
extern double MKMidiToAmpWithSensitivity(int midiValue, double sensitivity);

 /* Maps an amplitude scaler onto velocity such that
    MKAmpToMidi(MKMidiToAmp(x)) == x
    */
extern int MKAmpToMidi(double amp);

 /* Maps MIDI controller values (e.g. volume pedal) onto an amplitude scaler 
    such that 64->0.1, 127->1.0, and 0->0. */
extern double MKMidiToAmpAttenuation(int midiValue);

 /* Same as above, but uses sensitivity to control how much effect 
    midiValue has.  */
extern double MKMidiToAmpAttenuationWithSensitivity(int midiValue, 
						    double sensitivity);

 /* Maps an amplitude scaler onto velocity such that
    MKAmpAttenuationToMidi(MKMidiToAmpAttenuation(x)) == x
    */
extern int MKAmpAttenuationToMidi(double amp);

typedef enum _MKNoteType {
    MK_noteDur = 257, MK_noteOn, MK_noteOff, MK_noteUpdate, MK_mute} 
MKNoteType;

#import "params.h"

#define MK_MKPARBITVECTS ((((int)MK_appPars-1)/32)+1)

typedef enum _MKDataType {     /* Data types supported by Notes */
    MK_noType = ((int)MK_sysReset + 1),
    MK_double,  
    MK_string,
    MK_int,
    MK_object,
    MK_envelope, 
    MK_waveTable}
MKDataType;

@interface Note : Object
/*
 * 
 * A Note object represents a musical sound or event by describing its 
 * attributes.  This information falls into three categories: 
 * 
 * * parameters 
 * * timing information
 * * type information.
 * 
 * Most of the information in a Note is in its parameters; a Note can
 * have any number of parameters.  A parameter consists of an identifier,
 * a string name, and a value.  The identifier is a unique integer used
 * to catalog the parameter within the Note; the Music Kit defines a
 * number of parameter identifiers such as MK_freq (for frequency) and
 * MK_amp (for amplitude).  The string name is used to identify the
 * parameter in a scorefile.  The string names for the Music Kit
 * parameters are the same as the identifier names, but without the "MK_"
 * prefix.  You can create your own parameter identifiers by passing a
 * name to the parName: class method.  This method returns the identifier
 * associated with the parameter name, creating it if it doesn't already
 * exit.
 * 
 * A parameter's value can be a double, int, char *, an Envelope object,
 * WaveTable object, or other (non-Music Kit) object.  These six
 * parameter value types are represented by the following MKDataType
 * constants:
 * 
 * * MK_double
 * * MK_int
 * * MK_string
 * * MK_envelope
 * * MK_waveTable
 * * MK_object
 * 
 * The method you invoke to set a parameter value depends on the type of
 * the value.  To set a double value, for example, you would invoke the
 * setPar:toDouble: method.  Analogous methods exist for the other data
 * types.
 * 
 * You can retrieve the value of a parameter as any of the parameter data
 * types.  For instance, the parAsInt: method returns an integer
 * regardless of the parameter value's actual type.  The exceptions are
 * in retrieving object information: The parAsEnvelope:, parAsWaveTable:,
 * and parAsObject: messages return nil if the parameter value isn't the
 * specified type.
 * 
 * A Note's parameters are significant only if an object that processes
 * the Note (such as an instance of a subclass of Performer, NoteFilter,
 * Instrument, or SynthPatch) accesses and uses the information.
 * 
 * Timing information is used to perform the Note at the proper time and
 * for the proper duration.  This information is called the Note's
 * timeTag and duration, respectively.  A single Note can have only one
 * timeTag and one duration.  Setting a Note's duration automatically
 * changes its noteType to MK_noteDur, as described below.  TimeTag and
 * duration are measured in beats.
 * 
 * A Note has two pieces of type information, a noteType and a noteTag.
 * A Note's noteType establishes its nature; there are six noteTypes:
 * 
 * * A noteDur represents an entire musical note (a note with a duration).
 * * A noteOn establishes the beginning of a note.
 * * A noteOff establishes the end of a note.
 * * A noteUpdate represents the middle of a note (it updates a sounding note).
 * * A mute makes no sound. 
 * 
 * These are represented by MKNoteType constants:  
 * 
 * * MK_noteDur
 * * MK_noteOn 
 * * MK_noteOff
 * * MK_noteUpdate
 * * MK_mute
 * 
 * The default is MK_mute.
 * 
 * NoteTags are integers used to identify Note objects that are part of
 * the same musical sound or event; in particular, matching noteTags are
 * used to create noteOn/noteOff pairs and to associate noteUpdates with
 * other Notes.  (A noteUpdate without a noteTag updates all the Notes in
 * its Part.)
 * 
 * The C function MKNoteTag() is provided to generate noteTag values that
 * are guaranteed to be unique across your entire application -- you
 * should never create a new noteTag except through this function.  The
 * actual integer value of a noteTag has no significance (the range of
 * noteTag values extends from 0 to 2^32).
 * 
 * Mutes can't have noteTags; if you set the noteTag of such a Note, it
 * automatically becomes a noteUpdate.
 * 
 * Notes are typically added to Part objects.  A Part is a time-ordered
 * collection of Notes.
 * 
 */
{
 	MKNoteType noteType;     /* The object's noteType. */
	int noteTag;             /* The object noteTag. */
	id performer;            /* The Performer object that most recently 
                                    performed this object, if any. */
        id part;                 /* The Part that this object is a member of, 
                                    if any. */
        double timeTag;          /* Time tag, if any, else MK_ENDOFTIME. */
        void *_reservedNote1;
        unsigned _reservedNote2[MK_MKPARBITVECTS];
        unsigned *_reservedNote3;
        short _reservedNote4;
        int _reservedNote5;
}

 /* METHOD TYPES
  * Creating and freeing a Note
  * Modifying the object
  * Querying the object
  * Accessing Parameters
  * Timing information
  * Type information
  * Displaying the Note
  */

+ new; 
 /* TYPE: Creating; Creates a new mute Note object.
  * Creates, initializes, and returns a new mute Note object.  Implemented
  * as [self newSetTimeTag:MK_ENDOFTIME].  */

+ newSetTimeTag:(double )aTimeTag; 
 /* TYPE: Creating; Creates a new mute Note and sets its timeTag.
  * Creates and initializes a new mute Note object and sets its timeTag to
  * aTimeTag by sending [self initWithTimeTag:aTimeTag]. 
  * Returns  the new object.  */

- initWithTimeTag:(double )aTimeTag;
 /* TYPE: Creating; Sets timeTag as specified and sets type to mute.
  * Subclass should send [super initWithTimeTag:aTimeTag] if it overrides 
  * this method.
  */ 

- init;
 /* TYPE: Creating; Same as [self initWithTimeTag:MK_ENDOFTIME] */

- free; 
 /* TYPE: Creating; Frees the receiver and its contents.
  * Removes the receiver from its Part, if any, and then frees the
  * receiver and its contents.  The contents of object-valued,
  * envelope-valued and wavetable-valued parameters aren't freed.  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Returns a new Note as a copy of the receiver.
  * Creates and returns a new Note object as a copy of the receiver.  The
  * receiver's parameters, timing information, noteType, and noteTag are
  * copied into the new Note.  Object-valued parameters are shared by the
  * two Notes.  The new Note's Part is set to nil.  */

- copy;
 /* Same as [self copyFromZone:[self zone]]; */

- split:(id *)aNoteOn :(id *)aNoteOff; 
 /* TYPE: Creating; Splits the receiver (a noteDur) into a noteOn/noteOff pair.
  * If receiver isn't a noteDur, returns nil.  Otherwise, creates a noteOn
  * and a noteOff, splits the information in the receiver between the two
  * of them (as explained below), and returns the new Notes by reference
  * in the arguments.  The method itself returns the receiver, which is
  * neither freed nor otherwise affected.
  * 
  * All the receiver's parameters are copied into the noteOn except for
  * MK_relVelocity which, if present, is copied into the noteOff.  The
  * noteOn takes the receiver's timeTag while the noteOff's timeTag is
  * that of the receiver plus its duration.  If the receiver has a
  * noteTag, it's copied into both new Notes; otherwise a new noteTag is
  * generated for them.  The new Notes are added to the receiver's Part,
  * if any.
  * 
  * The new noteOn shares the receiver's object-valued parameters.
  */

- performer; 
 /* TYPE: Querying; Returns the Performer that last sent the receiver.
  * Returns the Performer that most recently sent the receiver during a
  * performance.  */

- part; 
 /* TYPE: Querying; Return the receiver's Part.
  * Returns the Part that the receiver is a member of, or nil if none.  */

- conductor; 
 /* TYPE: Querying; Returns the receiver's last Conductor.
  * Returns the Conductor of the Performer that most recently sent the
  * Note in a performance.  If the Note's Performer is nil, then this
  * returns the defaultConductor.  */

- addToPart:aPart; 
 /* TYPE: Modifying; Adds the receiver to aPart.
  * Removes the receiver from the Part that it's currently a member of and
  * adds it to aPart.  Returns the receiver's old Part, if any.  */

-(double ) timeTag; 
 /* TYPE: Timing; Returns the receiver's timeTag.
  * Returns the receiver's timeTag.  If the timeTag isn't set, returns
  * MK_ENDOFTIME.  */

-(double ) setTimeTag:(double )newTimeTag; 
 /* TYPE: Timing; Sets the receiver's timeTag.
  * Sets the receiver's timeTag to newTimeTag and returns the old timeTag,
  * or MK_ENDOFTIME if none.  If newTimeTag is negative, it's clipped to
  * 0.0.
  * 
  * If the receiver is a member of a Part, it's first removed from the
  * Part, its timeTag is set, and then it's re-added to the Part.  This
  * ensures that the receiver's position within its Part is correct.  */

- removeFromPart; 
 /* TYPE: Modifying; Removes the receiver from its Part.
  * Removes the receiver from its Part, if any.  Returns the Part, or nil
  * if none.  */

-(int ) compare:aNote; 
 /* TYPE: Querying; Compares the receiver with aNote.
  * Compares the receiver with aNote and returns a value as follows:
  * 
  * * If the receiver's timeTag < aNote's timeTag, returns -1.
  * * If the receiver's timeTag > aNote's timeTag, returns 1.
  * 
  * If the timeTags are equal, the comparison is by order in the part.
  * 
  * If the Notes are both not in parts or are in different parts, the
  * result is indeterminate.
  * 
  */

-(MKNoteType ) noteType; 
 /* TYPE: Type; Returns the receiver's noteType. */

- setNoteType:(MKNoteType )newNoteType; 
 /* TYPE: Type; Sets the receiver's noteType to newNoteType.
  * Sets the receiver's noteType to newNoteType, one of:
  * 
  * * MK_noteDur
  * * MK_noteOn
  * * MK_noteOff
  * * MK_noteUpdate
  * * MK_mute
  * 
  * Returns the receiver or nil if newNoteType isn't a noteType.
  * */

-(double ) setDur:(double )value; 
 /* TYPE: Timing; Sets the receiver's duration to value.
  * Sets the receiver's duration to value beats and sets its noteType to
  * MK_noteDur.  If value is negative the duration isn't set (but the
  * noteType is still set to noteDur).  Always returns value.  */

-(double ) dur; 
 /* TYPE: Timing; Returns the receiver's duration.
  * Returns the receiver's duration, or MK_NODVAL if it isn't set or if
  * the receiver noteType isn't MK_noteDur.    
  * (Use MKIsNoDVal() to check for MK_NODVAL.)*/

-(int ) noteTag; 
 /* TYPE: Type; Returns the receiver's noteTag.
  * Return the receiver's noteTag, or MAXINT if it isn't set.  */

- setNoteTag:(int )newTag; 
 /* TYPE: Type; Sets the receiver's noteTag to newTag.
  * Sets the receiver's noteTag to newTag; if the noteType is MK_mute it's
  * automatically changed to MK_noteUpdate.  Returns the receiver.  */

+(int ) parName:(char * )aName; 
 /* TYPE: Querying p; Creates (if necessary) and returns the parameter integer for aName.
 */

+(char *) nameOfPar:(int)aPar;
 /* Returns name corresponding to aPar.  E.g. [Note nameOfPar:MK_freq]
    returns "freq".  The string is not copied.
 */

- setPar:(int )par toDouble:(double )value; 
 /* TYPE: Setting; Sets parameter par to value, a double.
  * Sets the parameter par to value, which must be a double.
  * Returns the receiver.  
  */

- setPar:(int )par toInt:(int )value; 
 /* TYPE: Setting;  Sets parameter par to value, an int.
  * Sets the parameter par to value, which must be an int.  Returns the
  * receiver.  */

- setPar:(int )par toString:(char * )value; 
 /* TYPE: Setting; Sets parameter par to a copy of value, a char *.
  * Set the parameter par to a copy of value, which must be a char\ *.
  * Returns the receiver.  */
 
- setPar:(int )par toEnvelope:envObj; 
 /* TYPE: Setting; Sets parameter par to the Envelope envObj.
  * Sets the parameter par to envObj, an Envelope object.
  * Returns the receiver.  
  */

- setPar:(int )par toWaveTable:waveObj; 
 /* TYPE: Setting; Sets parameter par to the WaveTable waveObj.
  * Sets the parameter par to waveObj, a WaveTable object.
  * Returns the receiver.  
  */

- setPar:(int )par toObject:anObj; 
 /* TYPE: Setting; Sets parameter par to the object anObj.
  * Sets the parameter par to the object anObj.  The object's class must
  * implement the methods writeASCIIStream: and readASCIIStream: (in order
  * to be written to a scorefile).  An object's ASCII representation
  * shouldn't contain the character ']'.  Returns the receiver.
  * 
  * None of the Music Kit classes implement readASCIIStream: or
  * writeASCIIStream: so you can't use this method to set a parameter to a
  * Music Kit object (you should invoke the setPar:toEnvelope: or
  * setPar:toWaveTable: to set the value of a parameter to an Envelope or
  * WaveTable object).  This method is provided to support extensions to the 
  * Music Kit allowing you to set the value of a parameter to an instance of 
  * your own class.
  */

-(double ) parAsDouble:(int )par; 
 /* TYPE: Accessing P; Returns the value of par as a double.
  * Returns a double value converted from the value of the parameter par.
  * If the parameter isn't present, returns MK_NODVAL. 
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

-(int ) parAsInt:(int )par; 
 /* TYPE: Accessing P; Returns the value of par as an int.
  * Returns an int value converted from the value of the parameter par.
  * If the parameter isn't present, returns MAXINT.  */

-(char * ) parAsString:(int )par; 
 /* TYPE: Accessing P;Returns a copy of the value of par as a char *.
  * Returns a char * converted from a copy of the value of the parameter
  * par.  If the parameter isn't present, returns a copy of "".  */

-(char * ) parAsStringNoCopy:(int )par; 
 /* TYPE: Accessing P; Returns the value of par as a char *.
  * Returns a char * to the value of the parameter par.  You shouldn't
  * delete or alter the value returned by this method.  If the parameter
  * isn't present, returns "".  */

- parAsEnvelope:(int )par; 
 /* TYPE: Accessing P; Returns par's value if it's an Envelope.
  * Returns the Envelope value of par.  If par isn't present or if its
  * value isn't an Envelope, returns nil.  */

- parAsWaveTable:(int )par; 
 /* TYPE: Accessing P; Returns par's value if it's a WaveTable.
  * Returns the WaveTable value of par.  If par isn't present or if it's
  * value isn't an WaveTable, returns nil.  */

- parAsObject:(int )par; 
 /* TYPE: Accessing P; Returns par's value if it's an object.
  * Returns the object value of par.  If par isn't present or if its value
  * isn't an object, returns nil.  (This method will return Envelope and
  * WaveTable objects).  */

-(BOOL ) isParPresent:(int )par; 
 /* TYPE: Accessing P; YES if par is present in the receiver.
  * Returns YES if the parameter par is present in the receiver, NO if it
  * isn't.  */

-(MKDataType ) parType:(int )par; 
 /* TYPE: Accessing P; Returns the data type of par.
  * Returns the parameter data type of par as one of the six MKDataTypes
  * listed in the class description above.  If the parameter isn't
  * present, returns MK_noType.  */

- removePar:(int )par; 
 /* TYPE: Accessing P; Removes par from the receiver.
  * Removes the parameter par from the receiver.  Returns the receiver if
  * the parameter is present, otherwise returns nil.  */

- copyParsFrom:aNote; 
 /* TYPE: Accessing p; Copies parameters from aNote into the receiver.
  * Copies aNote's parameters into the receiver.  Object-valued parameters
  * are shared by the two Notes.  Returns the receiver.  */

-(double ) freq; 
 /* TYPE: Accessing P; Special method that returns the receiver's frequency.
  * If MK_freq is present, returns its value.  Otherwise, gets the
  * frequency that correponds to MK_keyNum according to the installed
  * tuning system (see the TuningSystem class).  If MK_keyNum isn't
  * present, returns MK_NODVAL. (Use MKIsNoDVal() to check for MK_NODVAL.)
  * The correpondence between key numbers and
  * frequencies for the default tuning system is given in Appendix F,
  * "Music Tables." 
  */

-(int ) keyNum; 
 /* TYPE: Accessing P; Special method that returns the receiver's key number.
  * If MK_keyNum is present, returns its value.  Otherwise, gets the
  * frequency that correponds to MK_freq according to the installed tuning
  * system (see the TuningSystem class).  If MK_freq isn't present,
  * returns MAXINT.  The correpondence between key numbers and
  * frequencies for the default tuning system is given in Appendix F,
  * "Music Tables."  */

- writeScorefileStream:(NXStream *)aStream; 
 /* TYPE: Modifying; Writes the receiver in scorefile format.
  * Writes the receiver to the (open) stream aStream using scorefile
  * format.  Returns the receiver.  */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object to archive file.
     You never send this message directly. Should be invoked via 
     NXWriteRootObject().
     Archives parameters, noteType, noteTag, and timeTag. Also archives
     performer and part using MKWriteObjectReference(). */

- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object from archive file.
     You never send this message directly.  
     Reads Note back from archive file. Note that the noteTag is NOT mapped
     onto a unique note tag. This is left up to the Part or Score with which
     the Note is unarchived. If the Note is unarchived directly with 
     NXReadObject(), then the handling of the noteTag is left to the 
     application.
   */

-(int)parVectorCount;
-(unsigned)parVector:(unsigned)index;
 /* TYPE: Parameters; Checks presence of a number of parameters at once.
 * Returns a bit vector indicating the presence of parameters 
 * identified by integers (index * 32) through 
 * ((index + 1) * 32 - 1). For example,
 *
 * .ib
 * unsigned int parVect = [aNote checkParVector:0];
 * .iq
 *
 * returns the vector for parameters 0-31.
 * An argument of 1 returns the vector for parameters 32-63, etc.
 *
 * parVectorCount gives the number of parVectors. For example, if the
 * highest parameter is 65, parVectorCount returns 3.
 */

extern void *MKInitParameterIteration(id aNote);
extern int MKNextParameter(id aNote,void *aState);
 /* These functions provide iteration over the parameters of a Note. 
 * Usage:
 *
 *  void *aState = MKInitParameterIteration(aNote);
 *  int par;
 *  while ((par = MKNextParameter(aNote,aState)) != MK_noPar) {
 *        select (par) {
 *          case freq0: 
 *            something;
 *            break;
 *          case amp0:
 *            somethingElse;
 *            break;
 *          default: // Skip unrecognized parameters
 *            break;
 *        }}
 *
 *  It is illegal to reference aState after MKNextParameter has returned
 *  MK_noPar.
 */

 /* Functions that are equivalent to above methods, for speed. */
extern id MKSetNoteParToDouble(id aNote,int par,double value);
extern id MKSetNoteParToInt(id aNote,int par,int value);
extern id MKSetNoteParToString(id aNote,int par,char *value);
extern id MKSetNoteParToEnvelope(id aNote,int par,id envObj);
extern id MKSetNoteParToWaveTable(id aNote,int par,id waveObj);
extern id MKSetNoteParToObject(id aNote,int par,id anObj);
extern double MKGetNoteParAsDouble(id aNote,int par);
extern int MKGetNoteParAsInt(id aNote,int par);
extern char *MKGetNoteParAsString(id aNote,int par);
extern char *MKGetNoteParAsStringNoCopy(id aNote,int par);
extern id MKGetNoteParAsEnvelope(id aNote,int par);
extern id MKGetNoteParAsWaveTable(id aNote,int par);
extern id MKGetNoteParAsObject(id aNote,int par);
extern BOOL MKIsNoteParPresent(id aNote,int par);

@end



