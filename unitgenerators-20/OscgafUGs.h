/* 
	OscgafUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>

@interface OscgafUGs:UnitGenerator
/* OscgafUG, superclass for OscgafUG (non-interpolating or "drop-sample") and 
   OscgafiUG (interpolating or "high-quality")

   - from dsp macros /usr/lib/dsp/ugsrc/oscgaf.asm and oscgafi.asm 
   (see source for details).

       OscgafUG<a><b><c><d>, where <a> is the output space, <b> is the
    amplitude input space, <c> is the increment input space, and <d> is
    the table space.
    
       OscgafUG is a class of lookup-table unit generators which includes
    patchpoint arguments for amplitude and frequency control.  That is,
    those parameters are intended to be determined by the output of some
    other unit generator, such as AsympUG.  See the example synthpatch
    FmExamp.m for an example of the use of Oscgaf.
    
       Amplitude control is straightforward.  The output of OscgafUG is
    simply the value of the lookup table times whatever comes in via the
    ampEnvInput patchpoint.  Frequency control is more complicated. The
    signal needed for freqEnvInput is not actually the frequency in Hertz, 
    but the phase increment, which is the amount the lookup table index changes
    during each sample.  This number depends on the desired frequency, the
    length of the lookup table, the sampling rate, and a constant called
    MK_OSCFREQSCALE. MK_OSCFREQSCALE is a power of two which represents
    the maximum possible increment.  Input to freqEnvInput must be divided
    by this number in order to insure that it remains in the 24-bit signal
    range.  The signal is then scaled back up by this number within
    Oscgaf, with a possible additional scaling by the incRatio (see
    below).
    
       A method called incAtFreq: has been provided which takes all the
    above factors into account and returns the increment for a given
    frequency.  The lookup table must be set first, via the -setTable:
    method, since the length of the table must be known to perform the
    calculation.  If more than one Oscgaf is to be controlled by the same
    increment envelope signal (such as in a typical FM patch), they can
    have different frequencies by using the -setIncRatio: method.  Since
    the input increment signal is scaled by MK_OSCFREQSCALE*incRatio
    within Oscgaf, the resulting frequency will be correspondingly
    changed.  The incRatio defaults to 1.0.
    
       The increment scaler can be set directly with -setIncScaler:. This
    simply sets the increment scaler to the value you provide, ignoring
    MK_OSCFREQSCALE, incRatio, etc.
    
       OscgafUG is a non-interpolating oscillator. That means that its
    fidelity depends on the size of the table (larger tables have lower
    distortion) and the highest frequency represented in the table. For
    high-quality synthesis, an interpolating version with the same methods,
    OscgafiUG, is preferable.
    However, an interpolating oscillator is also more expensive. OscgafUG
    is useful in cases where density of texture is more important than
    fidelity of individual sounds.
*/
{
    double _reservedOscgaf1;
    double incRatio;	/* optional multiplier on frequency Scale */
    double _reservedOscgaf2;
    id _reservedOscgaf3;
    int tableLength; /* Or 0 if no table. */
}

+(BOOL)shouldOptimize:(unsigned) arg;
 /* Specifies that all arguments are to be optimized if possible except the
    phase. */

+(int)defaultTableLength:anObj;
 /* Returns a default table length determined by the type of subclass and type
    of argument. */

-setIncScaler:(int)aScaler;
 /* Sets increment directly to an integer as specified. Not normally called
    by the user. */

-setPhase:(double)aPhase;
 /* Sets oscillator phase in degrees. If wavetable has not yet been set,
    stores the value for -runSelf to use to set the phase later. */

-setAmpInput:aPatchPoint;
 /* Sets amplitude envelope input to specified patchPoint. The signal received
    via aPatchPoint serves as a multiplier on the output of the oscillator. */

-setOutput:aPatchPoint;
 /* Set output patchPoint of the oscillator. */

-setIncInput:aPatchPoint;
 /* Set frequency envelope input to specified patchPoint. Note that OscgafUG  
   implements a multiplicative frequency envelope. The actual phase increment 
   is the value of the signal received via aPatchPoint multiplied by the 
   IncScaler. To get the proper increment value for a certain frequency, e.g, 
   for use in a frequency envelope generator writing to the incEnvInput, 
   see "incAtFreq:" below. */

-(double)incAtFreq:(double)aFreq;
 /* Returns the phase increment for this unit generator based on aFreq.
   This value is suitable for use as the amplitude to a unit generator whose
   output is being used as the incEnvInput of this unit generator. The lookup 
   table must be set before sending this message.  */

-setIncRatio:(double)aRatio;
 /* This is an alternative to the -setIncScaler: method. 
   The ratio specified here acts as a straight multiplier on the 
   increment scaler, and hence on the frequency.  For example, in an
   FM SynthPatch with one frequency envelope for both carrier and modulator,
   setIncRatio:aValue sent to the modulator is the modulator/carrier 
   frequency ratio.  The ratio defaults to 1.0. */

-setTable:anObj length:(int)aLength;
 /* 
   Sets the lookup table of the oscillator.
   anObj can be a SynthData object or a WaveTable (Partials or Samples).

   First releases its claim on the locally-allocated SynthData, if any. 
   (see below).

   If anObj is a SynthData object, the SynthData object is used directly.

   If anObj is a WaveTable, the receiver first searches in its Orchestra's
   shared object table to see if there is already an existing SynthData based 
   on the same WaveTable, of the same length, and in the required memory
   space. Otherwise, a local SynthData object is created and installed in the
   shared object table so that other unit generators running simultaneously 
   may share it. (This is important since DSP memory is limited.) 
   If the requested size is too large, because there is not sufficient DSP
   memory, smaller sizes are tried. (You can determine what size was used
   by sending the tableLength message.)
   
   Note that altering the contents of a WaveTable will have no effect once it 
   has been installed, even if you call setTable:length: again after 
   modifying the WaveTable. The reason is that the Orchestra's shared data
   mechanism finds the requested object based on its id, rather than its
   contents.

   If anObj is nil, simply releases the locally-allocated SynthData, if any. 
   If the table is not a power of 2, returns nil and generates the error
   MK_ugsPowerOf2Err. 
 */

-setTable:anObj;
 /* Like setTable:length, but uses a default length. */

-setTableToSineROM;
 /* Sets the lookup table to the DSP sine ROM, if address space is Y. 
   Otherwise generates an error. Deallocates local wave table, if any.
  */

-setTable:anObj length:(int)aLength defaultToSineROM:(BOOL)yesOrNo;
 /* This method is provided as a convenience. It tries to do 'the right thing'
   in cases where the table cannot be allocated. 

   If the table can be allocated, it behaves like setTable:length:. If the
   table cannot be allocated, and the table memory space of the receiver is Y,
   sends [self setTableToSineROM]. 
   
   A common use of this method is to pass YES as the argument defaultToSineROM
   only if the SynthPatch is beginning a new phrase (the assumtion is that it 
   is better to keep the old wavetable than to use the sine ROM in this case).
   Another use of this method is to specifically request the sine ROM by 
   passing nil as anObj. If the sine ROM is used, the aLength argument is
   ignored.
   
   Errors:
   If anObj is not nil and the sine ROM is used, generates the error 
   MK_spsSineROMSubstitutionErr. If sineROMDefaultOK is YES but the 
   receiver's table memory space is X, the error MK_spsCantGetMemoryErr 
   is generated.
 */   

-setTable:anObj defaultToSineROM:(BOOL)yesOrNo;
 /* Like setTable:length:defaultToSineROM, but uses a default length. */

-(unsigned)tableLength;
 /* Returns the length of the assigned table 0 if no table is assigned. */

-(double)incRatio;
 /* Returns incRatio. */

-runSelf;
 /* Sets phase if setPhase was called before lookup table was set, and
   sets increment scaler if not already set by a call to -setIncRatio:.
   If wavetable has not been set, and table space is Y, uses dsp sine ROM. */

-idleSelf;
 /* Deallocates local wave table memory, if any, and patches output to Sink. */


@end




  
