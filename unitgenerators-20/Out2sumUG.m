#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
                 Optimized setBearing: slightly.
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"

#import "Out2sumUG.h"
@implementation Out2sumUG:UnitGenerator
/* 
	You instantiate a subclass of the form 
	Out2sumUG<a>, where 
	<a> = space of input
	*/
{
  BOOL _reservedOut2sum1;
  double bearingScale;
}
#define _set _reservedOut2sum1

enum _args { sclA, iadr, sclB};

#import "out2sumUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return YES;
}
#endif _MK_UGOPTIMIZE

-idleSelf
{
    [self setAddressArgToZero:iadr];
    bearingScale = 1.0;
    return self;
}

-runSelf
{
  if (!_set)
    [self setBearing:MK_DEFAULTBEARING scale:1.0]; /* Sets bearing to 0 */
  return self;
}

-setInput:aPatchPoint
{
    return MKSetUGAddressArg(self,iadr,aPatchPoint);
}

-setLeftScale:(double)val
{
    _set = YES;
    return MKSetUGDatumArg(self,sclA,DSPDoubleToFix24(val));
}

-setRightScale:(double)val
{
    _set = YES;
    return MKSetUGDatumArg(self,sclB,DSPDoubleToFix24(val));
}

#define bearingFun1(theta)    fabs(cos(theta))
#define bearingFun2(theta)    fabs(sin(theta))

-setBearing:(double)val
  /* When val is -45, you get the left channel, +45 you get the right channel.
     Val = 90 is the same as val = 0. */  
{
    val = val * M_PI/180.0 + M_PI/4.0;
    MKSetUGDatumArg(self,sclA,DSPDoubleToFix24(bearingScale*bearingFun1(val)));
    MKSetUGDatumArg(self,sclB,DSPDoubleToFix24(bearingScale*bearingFun2(val)));
    _set = YES;
    return self;
}

-setBearing:(double)val scale:(double)aScale
{
  bearingScale = aScale;
  return [self setBearing:val];
}
@end
