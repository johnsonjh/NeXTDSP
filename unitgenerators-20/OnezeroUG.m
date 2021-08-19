#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
*/

#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"


#import "OnezeroUG.h"
@implementation OnezeroUG:UnitGenerator
  /* One zero filter.
	You instantiate a subclass of the form 
	OnepoleUG<a><b>, where 
	<a> = space of output
	<b> = space of input

      The onezero unit-generator implements a one-zero
      filter section in direct form.  For best performance,
      the input and output signals should be in separate
      memory spaces x or y.

      In pseudo-C notation:

      for (n=0;n<DSPMK_NTICK;n++) {
           in = sinp:ainp[n];
           sout:aout[n] = bb0*in + bb1*s;
           s = in;
      }

      
      */
{
}
enum args { ainp, aout, s, bb0, bb1};

#import "onezeroUGInclude.m"

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

-setB0:(double)val
  /* Sets gain of undelayed sample. */
{
    return MKSetUGDatumArg(self,bb0,DSPDoubleToFix24(val));
}

-setB1:(double)val
  /* Sets gain of delayed sample. */
{
    return MKSetUGDatumArg(self,bb1,DSPDoubleToFix24(val));
}

-clear
  /* Clears filter's state variable. */
{
    return MKSetUGDatumArg(self,s,0);
}

@end
