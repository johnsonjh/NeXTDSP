/* 
	SnoiseUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/

#import <musickit/UnitGenerator.h>

@interface SnoiseUG : UnitGenerator
/* 
  SnoiseUG - from dsp macro /usr/lib/dsp/ugsrc/snoise.asm (see source for details).

  You instantiate a subclass of the form SnoiseUG<a>, where 
  <a> = space of output.

  SnoiseUG computes uniform pseudo-white noise using the linear congruential 
  method for random number generation (reference: Knuth, volume II of The Art 
  of Computer Programming).   Whereas UnoiseUG computes a new random value
  every sample, SnoiseUG computes a new random value every tick (16 samples),
  and is 3 times faster.
  
  */
-idleSelf;
/* Sets output to sink. */

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except seed. */

-setSeed:(DSPDatum)seedVal;
/* Sets seed of random sequence. This is the current value and thus is changed
   by the unit generator itself. */

-anySeed;
/* Sets seed of random sequence to a new seed, never before used by previous
   invocations of anySeed. Useful, for insuring that different
   noise generators generate different noise. */

-setOutput:aPatchPoint;
/* Sets output as specified. */

@end







