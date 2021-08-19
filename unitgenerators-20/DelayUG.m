#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
  01/16/90/daj - Changed to use new MKSetUGAddressArgToInt()

*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"

#import "DelayUG.h"
@implementation DelayUG:UnitGenerator
/* Delay line. 
	You instantiate a subclass of the form 
	DelayUG<a><b><c>, where 
	<a> = space of output
	<b> = space of input
	<c> = space of delay line
*/	
{
    id memObj;      /* Delay memory */
    int len;   /* Length (LEQ length of memObj). */
}

enum args { ainp, aout, pdel, adel, edel};

#import "delayUGInclude.m"


#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != pdel);
}
#endif _MK_UGOPTIMIZE

-idleSelf
  /* Patches output and delay memory to sink. */
{
    [self setAddressArgToSink:aout];
    [self setDelayMemory:nil];
    return self;
}

-runSelf
  /* Does nothing. */
{
    return self;
}

-setInput:aPatchPoint
  /* Sets input as specified. */
{
    return MKSetUGAddressArg(self,ainp,aPatchPoint);
}

-setOutput:aPatchPoint
  /* Sets output as specified. */
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-setDelayMemory:aDspMemoryObj
  /* If you pass nil, uses sink as the delay memory. It is up to the caller
     to insure the memory is cleared. */
{
    int memObjAddr;
    if (!aDspMemoryObj) {
	MKOrchMemSegment seg;
	DSPMemorySpace spc = [(id)self->isa argSpace:adel];
	if (spc == DSP_MS_X)
	  seg = MK_xPatch;
	else seg = MK_yPatch;
	aDspMemoryObj = [orchestra segmentSink:seg];
    }
    memObj = aDspMemoryObj;
    len = [aDspMemoryObj length];
    [orchestra beginAtomicSection];
    MKSetUGAddressArg(self,adel,aDspMemoryObj);
    memObjAddr = [aDspMemoryObj address];
    MKSetUGAddressArgToInt(self,pdel,memObjAddr);
    MKSetUGAddressArgToInt(self,edel,memObjAddr + len);
    [orchestra endAtomicSection];
    return self;
}

-adjustLength:(int)newLength
  /* If no setDelayMemory: message has been received, returns nil.
     Otherwise, adjusts the delay length as indicated. newLength
     must be LEQ the length of the block of memory specified
     in setDelayMemory:. Otherwise, nil is returned. Note
     that the unused memory in the memory specified in 
     setDelayMemory: is not freed. Resetting the
     length of a running Delay may cause the pointer to go out-of-bounds.
     Therefore, it is prudent to send setPointer: or resetPointer after
     adjustLength:. Also note that when lengthening the delay, you 
     will be bringing in old delayed samples. Therefore, you may
     want to clear the new portion by sending the memory object the
     message -setToConstant:length:offset:. */
{
    if (!memObj || ([memObj length] < newLength))
      return nil;
    len = newLength; 
    MKSetUGAddressArgToInt(self,edel,newLength + [memObj address]);
    return self;
}

-resetPointer
  /* If no setDelayMemory: message has been received, returns nil.
     Else sets pointer to start of memory. This is done automatically
     when setDelayMemory: is sent. */
{
    if (!memObj)
      return nil;
    MKSetUGAddressArgToInt(self,pdel,[memObj address]);
    return self;
}

-setPointer:(int)offset
  /* If no setDelayMemory: message has been received, returns nil.
     Else sets pointer to specified offset. E.g. if offset == 0,
     this is the same as resetPointer. If offset is GEQ the length
     of the memory block, returns nil. */
{
    if (!memObj || (len <= offset))
      return nil;
    MKSetUGAddressArgToInt(self,pdel,offset + [memObj address]);
    return self;
}

-(int)length
  /* Returns the length of the delay. This is always LEQ than the
     length of the memory object. */
{
    return len;
}

@end
