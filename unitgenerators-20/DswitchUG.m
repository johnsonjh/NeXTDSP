#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"

#import "DswitchUG.h"
@implementation DswitchUG:UnitGenerator
/* 
  The delayedswitch unit-generator switches from input 1 (scaled) to a 
  input 2 (unscaled) after a delay specified in samples.  The delay
  can be interpreted as the number of samples input 1 is passed to 
  the output.  On each output sample, the delay is decremented by 1.
  Input 1 times the scale factor scale1 is passed to the output as long 
  as delay remains nonnegative. Afterwards, input 2 is passed 
  to the output with no scaling.

  You instantiate a subclass of the form 
  DswitchUG<a><b>, where 
	<a> = space of output	
	<b> = space of inputs
*/	
{
}
enum args { i1adr, scale1, aout, i2adr, switchdelay};

#import "dswitchUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != switchdelay);
}
#endif _MK_UGOPTIMIZE

-idleSelf
  /* Patches output to sink. */
{
    [self setAddressArgToSink:aout];
    return self;
}

-setInput1:aPatchPoint
{
    return MKSetUGAddressArg(self,i1adr,aPatchPoint);
}

-setInput2:aPatchPoint
{
    return MKSetUGAddressArg(self,i2adr,aPatchPoint);
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-setScale1:(double)val
{
    return MKSetUGDatumArg(self,scale1,DSPDoubleToFix24(val));
}

-setDelaySamples:(int)val
  /* Sets delay in samples. */
{
    return MKSetUGDatumArg(self,switchdelay,DSPIntToFix24(val));
}

@end
