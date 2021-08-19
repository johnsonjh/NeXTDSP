/* 
	OscgUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>

@interface OscgUG:UnitGenerator
/* OscgUG  - from dsp macro /usr/lib/dsp/ugsrc/oscg.asm (see source for details).

   OscgUG<a><b>, where <a> is output space and <b> is table space.
   
   This is a non-interpolating oscillator. That means that its fidelity
   depends on the size of the table (larger tables have lower distortion)
   and the highest frequency represented in the table. For high-quality
   synthesis, an interpolating oscillator is preferable. However, an
   interpolating oscillator is also more expensive. OscgUG is useful
   in cases where density of texture is more important than fidelity of
   individual sounds.
   
   The wavetable length must be a power of 2.
   The wavetable increment must be nonnegative.
   
   */
{
    double _reservedOscg1;
    double _reservedOscg2;
    double _reservedOscg3;
    id _reservedOscg4;
    int tableLength;          /* Or 0 if no table. */
}

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except the
   phase. */

-setInc:(int)anInc;
/* Sets increment as an integer as specified. */

-setFreq:(double)aFreq;
/* Sets oscillator frequency in Hz. If wavetable has not yet been set,
   stores the value for runSelf to use to set the frequency later. */

-setAmp:(double)aAmp;
/* Sets amplitude as specified. */

-setPhase:(double)aPhase;
/* Sets oscillator phase in degrees. If wavetable has not yet been set,
   stores the value for runSelf to use to set the phase later. */


-runSelf;
/* Sets oscillator phase if -setPhase: was called before lookup table was 
   set, and sets oscillator frequency to the last value set with setFreq:.
   If wavetable has not been set, and table space is Y, uses DSP SINE ROM. */

-idleSelf;
/* Deallocates local wave table memory, if any, and patches output to Sink. */

-setOutput:aPatchPoint;
/* Sets output location. */

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

   It functions like setTable:length:, but it defaults to the sine ROM in the 
   DSP if sineROMDefaultOK is YES, the DSP memory for anObj cannot be 
   allocated, and the table memory space of the receiver is Y.
   
   A common use of this method is to pass YES only if the SynthPatch is
   beginning a new phrase (the assumtion is that it is better to keep the
   old wavetable than to use the sine ROM in this case).
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

@end









