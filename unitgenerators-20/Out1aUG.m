#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
  04/18/90/mmm - Added missing definition of +shouldOptimize
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"
#import "Out1aUG.h"
@implementation Out1aUG:UnitGenerator
{ /* Instance variables go here */
  BOOL _reservedOut1a1;
}
#define _set _reservedOut1a1

enum args { sclA, iadr};

#import "out1aUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return YES;
}
#endif _MK_UGOPTIMIZE

-idleSelf
{
    [self setAddressArgToZero:iadr];
    return self;
}

-runSelf
{
    if (!_set) [self setScale:0.999999];
    return self;
}

-setInput:aPatchPoint
{
    return MKSetUGAddressArg(self,iadr,aPatchPoint);
}

-setScale:(double)val
{
    _set = YES;
    return MKSetUGDatumArg(self,sclA,DSPDoubleToFix24(val));
}

@end
