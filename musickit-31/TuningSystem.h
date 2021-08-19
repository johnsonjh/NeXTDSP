/*   
  TuningSystem.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
  */

#import <objc/Object.h>

#import <musickit/keynums.h>

/* Tuning and freq conversion */

extern double MKKeyNumToFreq(MKKeyNum keyNum);
 /* Convert keyNum to frequency using the installed tuning system.
    Returns MK_NODVAL if keyNum is out of bounds.
    (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

extern MKKeyNum MKFreqToKeyNum(double freq,int *bendPtr,double sensitivity);
 /* Returns keyNum (pitch index) of closest pitch variable to the specified
    frequency . If bendPtr is not NULL, *bendPtr is set to the bend needed to 
    get freq in the context of the specified pitch bend sensitivity. 
    Sensitivity is interpreted such that with a sensitivity of 1.0, 
    a pitch bend of 0 gives a maximum negative displacement of a semitone
    and a bend of 0x3fff gives a maximum positive displacement of a semitone.
    Similarly, a value of 2.0 give a whole tone displacement in either
    direction. MIDI_ZEROBEND gives no displacement. */

extern double MKAdjustFreqWithPitchBend(double freq,int pitchBend,
                    double sensitivity);
 /* Return the result of adjusting freq by the amount specified in
    pitchBend. PitchBend is interpreted in the context of the current
    value of sensitivity. */

extern double MKTranspose(double freq,double semiTonesUp);
 /* Transpose a frequency up by the specified number of semitones. 
    A negative value will transpose the note down. */

@interface TuningSystem : Object
/* 
 * 
 * A TuningSystem object represents a musical tuning system by mapping
 * key numbers to frequencies.  The method setFreq:forKeyNum: defines the
 * frequency value (in hertz) for a specified key number.  To tune a key
 * number and its octaves at the same time, invoke the method
 * setFreq:forKeyNumAndOctaves:.  The frequencies in a TuningSystem
 * object don't have to increase as the key numbers increase -- you can
 * even create a TuningSystem that descends in pitch as the key numbers
 * ascend the scale.  The freqForKeyNum: method retrieves the frequency
 * value of the argument key number.  Such values are typically used to
 * set the frequency of a Note object:
 * 
 * * #import <musickit/keynums.h>
 * * [aNote setPar:MK_freq toDouble:[aTuningSystem freqForKeyNum:c4k]];
 * 
 * The TuningSystem class maintains a master system called the
 * installed tuning system.  By default, the installed tuning system is
 * tuned to 12-tone equal-temperament with a above middle c set to 440
 * Hz.  A key number that doesn't reference a TuningSystem object takes
 * its frequency value from the installed tuning system.  The frequency
 * value of a pitch variable is also taken from the installed system.
 * The difference between key numbers and pitch variables is explained in
 * Chapter 10, "Music."  The entire map of key numbers, pitch variables,
 * and frequency values in the default 12-tone equal-tempered system is
 * given in Appendix G, "Music Tables."
 *
 * You can install your own tuning system by sending the install
 * message to a TuningSystem instance.  Keep in mind that this doesn't
 * install the object itself, it simply copies its key number-frequency
 * map.  Subsequent changes to the object won't affect the installed
 * tuning system (unless you again send the object the install message).
 * 
 * Note that while key numbers can also be used to define pitch for Notes
 * used in MIDI performance, the TuningSystem object has no affect on the
 * precise frequency of a Note sent to a MIDI instrument.  The
 * relationship between key numbers and frequencies on a MIDI instrument
 * is set on the instrument itself. (An application can, of course, use
 * the information in a TuningSystem object to configure the MIDI
 * instrument.)
 * 
 */
{
    id frequencies; /* Storage object of frequencies, indexed by keyNum. */
    void *_reservedTuningSystem1;
}

 /* METHOD TYPES
  * Creating a TuningSystem instance
  * Tuning the object
  * Querying the object
  * Tuning the installed tuning system
  * Querying the installed tuning system
  */

+ new; 
 /* TYPE: Creating; Creates a new TuningSystem object from default zone and
    sends [self init].
  */

- init;
 /* Initializes receiver to 12-tone equal tempered tuning. */

-copyFromZone:(NXZone *)zone;
 /* Copies object and arrays. */

-copy;
 /* Same as [self copyFromZone:[self zone]]; */

-free;
 /* Frees object and internal storage. */

- setTo12ToneTempered; 
 /* TYPE: Tuning; Sets the receiver's tuning to 12-tone equal-tempered. */

- install; 
 /* TYPE: Tuning; Installs the receiver's tuning as the installed tuning system.
  * Installs the receiver's tuning as the current tuning system.  The
  * receiver itself isn't installed, only its tuning system; subsequent
  * changes to the receiver won't affect the installed system unless you
  * resend the install message to the receiver.  Returns the receiver.
  */

+ installedTuningSystem; 
 /* Creates a TuningSystem object and tunes it to the installed tuning system.
    Returns the newly created object.  Tuning the returned object won't affect
    the installed TuningSystem.   Defined in terms of 
    -initFromInstalledTuningSystem.
  */

- initFromInstalledTuningSystem;
 /* Initializes a new TuningSystem object to the installed tuning system. */

+(double) freqForKeyNum:(MKKeyNum )aKeyNum; 
 /* TYPE: Querying the i; Returns the installed frequency for aKeyNum.
  * Returns the installed frequency for the key number aKeyNum.  If
  * aKeyNum is out of bounds, returns MK_NODVAL.  
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  * The value returned by this method is the same value as aKeyNum's 
  * analogous pitch variable.
  */

-(double) freqForKeyNum:(MKKeyNum )aKeyNum; 
 /* TYPE: Querying; Returns the receiver's frequency for aKeyNum.
  * Returns the receiver's frequency for the key number aKeyNum.  If
  * aKeyNum is out of bounds, returns MK_NODVAL.
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

- setKeyNum:(MKKeyNum )aKeyNum toFreq:(double)freq;
 /* TYPE: Tuning; Tunes the receiver's aKeyNum key number to freq.
  * Tunes the receiver's aKeyNum key number to freq and returns the
  * receiver.  If aKeyNum is out of bounds, returns MK_NODVAL.
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

+ setKeyNum:(MKKeyNum )aKeyNum toFreq:(double)freq; 
 /* TYPE: Tuning the i; Tunes the installed system's aKeyNum key number to freq.
  * Tunes the installed tuning system's aKeyNum key number to freq and
  * returns the receiver.  If aKeyNum is out of bounds, returns MK_NODVAL.
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  * 
  * Note: If you're making several changes to the installed tuning
  * system, it's more efficient to make the changes in a TuningSystem
  * instance and then send it the install message than it is to repeatedly
  * invoke this method.
  */

- setKeyNumAndOctaves:(MKKeyNum )aKeyNum toFreq:(double)freq;
 /* TYPE: Tuning; Tunes the receiver's aKeyNum and its octaves to octaves of freq.
  * Tunes all the receiver's key numbers with the same pitch class as
  * aKeyNum to octaves of freq such that aKeyNum is tuned to freq.
  * Returns the receiver or nil if aKeyNum is out of bounds.
  */

+ setKeyNumAndOctaves:(MKKeyNum )aKeyNum toFreq:(double)freq;
 /* TYPE: Tuning the i; Tunes the installed system's aKeyNum and its octaves to octaves of freq.
  * Tunes the key numbers in the installed tuning system that are the same
  * pitch class as aKeyNum to octaves of freq such that aKeyNum is tuned
  * to freq.  Returns the receiver or nil if aKeyNum is out of bounds.
  * 
  * Note: If you're making several changes to the installed tuning system,
  * it's more efficient to make the changes in a TuningSystem instance and
  * then send it the install message than it is to repeatedly invoke this
  * method.
  */

+ transpose:(double)semitones; 
 /* TYPE: Tuning the i; Transposes the installed tuning system by semitones half-steps.
  * Transposes the installed tuning system by semitones half-steps.  (The
  * half-step used here is 12-tone equal-tempered.)  If semitones is
  * positive, the transposition is up, if it's negative, the transposition
  * is down.  semitones can be any double value, thus you can transpose
  * the tuning system by increments smaller than a half-step.  Returns the
  * receiver.
  */

- transpose:(double)semitones; 
 /* TYPE: Tuning; Transposes the receiver by semitones half-steps.
  * Transposes the receiver by semitones half-steps.  (The half-step used
  * here is 12-tone equal-tempered.)  If semitones is positive, the
  * transposition is up, if it's negative, the transposition is down.
  * semitones can be any double value, thus you can transpose the receiver
  * by increments smaller than a half-step.  Returns the receiver.
  */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes receiver to archive file. */ 
- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads receiver from archive file. */ 

@end



