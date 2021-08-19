#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
*/
#import <musickit/musickit.h>
#import "Scl2add2UG.h"
#import "_unitGeneratorInclude.h"
@implementation Scl2add2UG:UnitGenerator
{ /* Instance variables go here */
}
/*    Scl2add2.

	You instantiate a subclass of the form Scl1add2UG<a><b><c>, where 
	<a> = space of output
	<b> = space of input1
	<c> = space of input2

      The scl2add2 unit-generator multiplies two input signals
      times constant scalers then adds them together to produce a
      third.  The output vector can be the same as an input vector.
      Inner loop is two instructions if space of input1 is "x" and
      space of input2 is "y", otherwise three instructions.

*/

enum args { i1adr, i1gin, aout, i2adr, i2gin};

#import "scl2add2UGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return YES;
}
#endif _MK_UGOPTIMIZE

-idleSelf
  /* Sets output to write to sink. */
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
  /* Sets scaling of input1. val is assumed between -1 and 1. */
{
    return MKSetUGDatumArg(self,i1gin,DSPDoubleToFix24(val));
}

-setScale2:(double)val
  /* Sets scaling of input1. val is assumed between -1 and 1. */
{
    return MKSetUGDatumArg(self,i2gin,DSPDoubleToFix24(val));
}

@end
