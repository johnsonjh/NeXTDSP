/*
  Partials.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>
#import "WaveTable.h"

@interface Partials : WaveTable 
/* 
 * 
 * A Partials object contains arrays that specify the amplitudes,
 * frequency ratios, and initial phases of a set of partials.  This
 * information is used to synthesize a waveform.  The synthesized data is
 * referenced by the methods inherited from WaveTable.
 * 
 * Ordinarily, the frequency ratios are multiplied by the base frequency
 * of the UnitGenerator that uses the Partials object.  Similarly, the
 * amplitude ratios defined in the Partials object are multiplied by the
 * UnitGenerator's amplitude term.
 * 
 */
{
    double *ampRatios;   /* Array of amplitudes. */
    double *freqRatios;  /* Array of frequencies. */
    double *phases;      /* Arrays of initial phases. */
    int partialCount;    /* Number of points in each array */
    double defaultPhase; /* Default phase. */
    double minFreq;      /* Optional frequency minimum. */
    double maxFreq;      /* Optional freq maximum. */
    BOOL _reservedPartials1,_reservedPartials2,_reservedPartials3;
    void *_reservedPartials4;
}
 
 /* METHOD TYPES
  * Creating and freeing a Partials
  * Modifying the object
  * Querying the object
  */

- init; 
 /* TYPE: Modifying; Inits the receiver.
  * Inits the receiver.  You never invoke this method directly.  A
  * subclass implementation should send [super init] before
  * performing its own initialization.  The return value is ignored.  */

-copyFromZone:(NXZone *)zone;
 /* TYPE: Creating; Returns a copy of the receiver.
  * Returns a copy of the receiver with its own copy of arrays. 
  * See also superclass -copy.
  */

- free; 
  /* TYPE: Modifying; Frees the receiver.
     Frees the receiver and all its arrays. */

- setPartialCount:(int)count freqRatios: (double *)fRatios ampRatios: (double *)aRatios phases: (double *)phases orDefaultPhase: (double)defaultPhase;
 /* TYPE: Modifying; Sets amplitude, frequency and phase information.
   This method is used to specify the amplitude and frequency
   ratios and initial phases (in degrees) of a set of partials representing a
   waveform.  If one of the data retrieval methods is called (inherited from 
   the WaveTable object), a wavetable is synthesized and returned.
   The resulting waveform is guaranteed to begin and end 
   at or near the same point only if the partial ratios are integers.

   If phs is NULL, the defPhase value is used for all
   harmonics.  If aRatios or fRatios is NULL, the corresponding value is
   unchanged. The array arguments are copied. */

- (int)partialCount;
 /* TYPE: Querying; Returns the number of partials.
   */

- (double *)freqRatios; 
 /* TYPE: Querying; Returns the frequency ratios array.
   Returns the frequency ratios array directly, without copying it. */

- (double *)ampRatios; 
 /* TYPE: Querying; Returns the amplitude ratios array.
   Returns the amplitude ratios array directly, without copying it nor 
   scaling it. */

- (double)defaultPhase;
 /* Returns the defaultPhase. */
- (double *)phases; 
 /* Returns phase array or NULL if none */

- (int) getPartial:(int)n freqRatio: (double *)fRatio ampRatio:(double *)aRatio
phase: (double *)phase;
 /* TYPE: Querying; Returns the specified partials.
   Get specified partial by reference. n is the zero-based
   index of the partial. If the specified partial is the last value, 
   returns 2. If the specified value is out of bounds, 
   returns -1. Otherwise returns 0.
   The partial amplitude is scaled by the scaling constant.
   */

- fillTableLength:(int)aLength scale:(double)aScaling ;
 /* TYPE: Modifying; Performs additive synthesis.
   Computes the wavetable. Returns self, or nil if an error is found. If 
   scaling is 0.0, the waveform is normalized. This method is sent
   automatically if necessary by the various data-retreival methods 
   (inherited from the WaveTable class).
   The resulting waveform is guaranteed to begin and end 
   at or near the same point only if the partial ratios are integers.
   Currently (release 1.0), only lengths that are a power of 2 are 
   allowed. 
*/

-writeScorefileStream:(NXStream *)aStream;
 /* TYPE: Modifying; Writes the receiver to a scorefile.
   Writes the receiver in scorefile format on the specified stream.
   Returns nil if ampRatios or freqRatios is NULL, otherwise self. */

-setFreqRangeLow:(double)freq1 high:(double)freq2;
 /* TYPE: Modifying; Sets the frequency range.
   Sets the frequency range associated with this timbre. */

-(double)minFreq;
 /* TYPE: Querying; Returns the minimum frequency.
   Returns the minimum fundamental frequency at which this timbre is
   ordinarily used. */

-(double)maxFreq;
 /* TYPE: Querying; Returns the maximum frequency.
   Returns the maximum fundamental frequency at which this timbre is
   ordinarily used. */

-(BOOL)freqWithinRange:(double)freq;
 /* TYPE: Querying; Returns whether freq is within range.
   Returns YES if freq is within the range of fundamental frequencies 
   ordinarily associated with this timbre, as set by setFreqRangeLow:high:. */

-(double)highestFreqRatio;

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Write object.
     You never send this message directly. It's invoked by 
     NXWriteRootObject() */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Read object.
     Note that -init is not sent to newly unarchived objects.
     You never send this message directly. It's invoked by NXReadObject() */

@end



