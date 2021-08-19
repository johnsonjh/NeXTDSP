/* 
	UnoiseUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/

#import <musickit/UnitGenerator.h>

@interface UnoiseUG : UnitGenerator
/* 
  UnoiseUG - from dsp macro /usr/lib/dsp/ugsrc/unoise.asm (see source for details).

  You instantiate a subclass of the form UnoiseUG<a>, where 
  <a> = space of output.

  UnoiseUG computes uniform pseudo-white noise using the linear congruential 
  method for random number generation (reference: Knuth, volume II of The Art 
  of Computer Programming).
  
  */
-idleSelf;
/* Sets output to sink. */

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except seed. */

-setSeed:(DSPDatum)seedVal;
/* Sets seed of random sequence. This is the current value and thus is changed
   by the unit generator itself. */

-setOutput:aPatchPoint;
/* Sets output as specified. */

@end






