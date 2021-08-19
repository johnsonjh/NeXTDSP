#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"
#import "SnoiseUG.h"

@implementation SnoiseUG:UnitGenerator
/* 
	You instantiate a subclass of the form 
	SnoiseUGUG<a>, where 
	<a> = space of output

      The unoise unit-generator computes uniform pseudo-white noise
      using the linear congruential method for random number generation
      (reference: Knuth, volume II of The Art of Computer Programming).
      Whereas UnoiseUG computes a new random value every sample, SnoiseUG
      computes a new random value every tick (16 samples),
      and is 3 times faster.
      The multiplier used has not been tested for quality.

	*/
{
}
enum args { aout, seed};

#import "snoiseUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != seed);
}
#endif _MK_UGOPTIMIZE

-idleSelf
{
    [self setAddressArgToSink:aout];
    return self;
}

-setSeed:(DSPDatum)seedVal
  /* Sets seed of random sequence. */
{
    return MKSetUGDatumArg(self,seed,seedVal);
}

-anySeed
{
    static int _seedVal = 1;
    _seedVal = ((_seedVal + 1) & DSP_WORD_MASK);
    return MKSetUGDatumArg(self,seed,_seedVal);
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

@end
