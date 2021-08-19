#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"
#import "ScaleUG.h"
@implementation ScaleUG:UnitGenerator
{ /* Instance variables go here */
}

/* 
   Scale.
	You instantiate a subclass of the form 
	ScaleUG<a><b>, where 
	<a> = space of output
	<b> = space of input

      The scale unit-generator simply copies one signal vector over to
      another, multiplying by a scale factor.  The output vector can 
      be the same as the input vector.
*/
enum args { ainp, aout, ginp};

#import "scaleUGInclude.m"

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

-setInput:aPatchPoint
{
    return MKSetUGAddressArg(self,ainp,aPatchPoint);
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-setScale:(double)val
  /* Sets scaling. val is assumed between -1 and 1. */
{
    return MKSetUGDatumArg(self,ginp,DSPDoubleToFix24(val));
}

@end
