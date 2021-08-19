#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
  04/25/90/daj - Added _exportedPrivateMusicKit.h import
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"
#import "_exportedPrivateMusickit.h"
#import "ConstantUG.h"
@implementation ConstantUG:UnitGenerator
{ 
}

enum args { aout, cnst};

#import "constantUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return YES;
}
#endif _MK_UGOPTIMIZE

-idleSelf
{
    [self setAddressArgToSink:aout];
    return self;
}

-setConstantDSPDatum:(DSPDatum)value
{
    return MKSetUGDatumArg(self,cnst,value);
}

-setConstant:(double)value
{
    return MKSetUGDatumArg(self,cnst,_MKDoubleToFix24(value));
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

@end

