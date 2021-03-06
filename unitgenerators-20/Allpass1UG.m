#ifdef SHLIB
#include "shlib.h"
#endif

/* This is a stub of the class you fill in. Generated by dsploadwrap.*/
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"
#import "Allpass1UG.h"
@implementation Allpass1UG:UnitGenerator
{}
  /* First order all pass filter.
	You instantiate a subclass of the form 
	Allpass1UG<a><b>, where 
	<a> = space of output
	<b> = space of input

      The allpass1 unit-generator implements a one-pole, one-zero
      allpass filter section in direct form. 

	The transfer function implemented is

		bb0 + 1/z
	H(z) =	---------
		1 + bb0/z

	In pseudo-C notation:

      for (n=0;n<I_NTICK;n++) {
         t = sinp:ainp[n] - bb0*s;
         sout:aout[n] = bb0*t + s;
         s = t;
      }

      */

enum args { ainp, bb0, aout, s};

#import "allpass1UGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != s);
}
#endif _MK_UGOPTIMIZE

-idleSelf
  /* Patches output to sink. */
{
    [self setAddressArgToSink:aout];
    return self;
}

-setInput:aPatchPoint
{
    return MKSetUGAddressArg(self,ainp,aPatchPoint);
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-setBB0:(double)val
  /* Sets coefficient of undelayed input and delayed output. */
{
    return MKSetUGDatumArg(self,bb0,DSPDoubleToFix24(val));
}

-clear
  /* Clears filter's state variable. */
{
    return MKSetUGDatumArg(self,s,0);
}

@end 
