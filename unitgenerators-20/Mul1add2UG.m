#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
*/
#import <musickit/musickit.h>
#import "Mul1add2UG.h"
#import "_unitGeneratorInclude.h"
@implementation Mul1add2UG:UnitGenerator
{ /* Instance variables go here */
}


enum args { i1adr, i2adr, aout, i3adr};

#import "mul1add2UGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return YES;
}
#endif _MK_UGOPTIMIZE

-setInput1:aPatchPoint
/* Set first input to specified patchPoint. */
{
    return MKSetUGAddressArg(self,i1adr,aPatchPoint);
}

-setInput2:aPatchPoint
/* Set second input to specified patchPoint. */
{
    return MKSetUGAddressArg(self,i2adr,aPatchPoint);
}

-setInput3:aPatchPoint
/* Set second input to specified patchPoint. */
{
    return MKSetUGAddressArg(self,i3adr,aPatchPoint);
}

-setOutput:aPatchPoint
/* Set output patchPoint of the oscillator. */
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-idleSelf
{
  return [self setAddressArgToSink:aout]; /* Patch output to sink. */
}

@end
