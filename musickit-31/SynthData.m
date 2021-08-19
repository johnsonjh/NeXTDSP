#ifdef SHLIB
#include "shlib.h"
#endif

/*
  SynthData.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  11/20/89/daj - Minor change for new lazy shared data garbage collection. 
  11/27/89/daj - Removed arg from _MKCurSample.
  01/31/90/daj - Added new method
               -setShortData:(short *)dataArray length:(int)len offset:(int)off
  03/31/90/daj - Moved private methods to category
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/27/90/daj - Changed _DSPSend* to DSPMKSend* 
                 (except for _DSPMKSendUnitGeneratorWithLooperTimed)
*/

#import "_musickit.h"
#import "_SharedSynthInfo.h"
#import "_error.h"
#import "_UnitGenerator.h"
#import "_Orchestra.h"

#import "_SynthData.h"
@implementation SynthData:Object
/* Data memory object. Used to allocate DSP data memory. Also used 
   to allocate patchpoints. You never create instances of SynthData or any 
   of its subclasses directly. They are created automatically by the 
   Orchestra object in response to messages such as allocSynthData:length:. */
{
    id synthPatch;      /* The SynthPatch to which this resource
			     belongs, if any. */
    id orchestra;       /* The orchestra of this SynthElement. */
    unsigned short _reservedSynthElement1;
    unsigned short _reservedSynthElement2;
    id _reservedSynthElement3;
    BOOL _reservedSynthElement4;
    void *_reservedSynthElement5;
    int length;            /* Length of allocated memory. */
    MKOrchAddrStruct orchAddr; /* Contains size, space, etc. */
    BOOL readOnly;       /* Yes if data is not to be written to. */
    MKOrchMemStruct _reservedSynthData1;
}

#define _reso _reservedSynthData1   /* Each instance has its own here. */
/* Used for some special synthdatas like
   zero and sink that should never go away. */


#define ISDATA (orchAddr.memSegment == MK_xData || \
		orchAddr.memSegment == MK_yData)
/* Needed by synthElementMethods.m. We surpress lazy garbage collection for
   patchpoints. The idea is to avoid fragmentation. 
   Note that Pluck depends on this. */

#import "synthElementMethods.m"

-clear
    /* clears memory */
{
    if (readOnly)
      return _MKErrorf(MK_synthDataReadonlyErr);
    if (_MK_ORCHTRACE(orchestra,MK_TRACEDSP))
	_MKOrchTrace(orchestra,MK_TRACEDSP,
		 "Clearing memory block %s_%d.",[self name],
		 self);
    DSPSetCurrentDSP(_orchIndex);
    if (DSPMKMemoryFillSkipTimed(_MKCurSample(orchestra),0,
				 orchAddr.memSpace,orchAddr.address,1,
				 length))  
      return _MKErrorf(MK_synthDataCantClearErr);
    return self;
}

-run
  /* Provided for compatability with UnitGenerator. Does nothing and returns
     self. */
{
    return self;
}

-idle
  /* Provided for compatability with UnitGenerator. Does nothing and returns
     self. */
{
    return self;
}

-(double)finish
  /* Provided for compatability with UnitGenerator. Does nothing and returns 0
     */
{
    return 0;
}

-(int)length
  /* Return size of memory block. */
{
    return length;
}

-(DSPAddress)address
  /* Returns address of memory block. */
{
    return orchAddr.address;
}

-(DSPMemorySpace)memorySpace
  /* Return memory space of this memory block. */
{
    return orchAddr.memSpace;
}

-(MKOrchAddrStruct *)orchAddrPtr
  /* Return addr struct ptr for this memory. The orchAddr is not copied. */
{
    return &orchAddr;
}

#define CONSTANT ((void *)((unsigned)-1)) /* A non-0 impossible pointer */

static id sendPreamble(self,dataArray,len,off,value)
    SynthData *self;
    void *dataArray; /* Or CONSTANT */
    int len;
    int off;
    DSPDatum value; /* Optional. Only supplied if CONSTANT */
{
    if ((!dataArray) || (len + off > self->length))
      return _MKErrorf(MK_synthDataLoadErr);
    if (self->readOnly)
      return _MKErrorf(MK_synthDataReadonlyErr);
    if (_MK_ORCHTRACE(self->orchestra,MK_TRACEDSP)) 
      if (dataArray == CONSTANT)
	_MKOrchTrace(self->orchestra,MK_TRACEDSP,
		     "Loading constant %d into memory block %s_%d.",
		     value,[self name],self);
      else 
	_MKOrchTrace(self->orchestra,MK_TRACEDSP,
		     "Loading array into memory block %s_%d.",
		     [self name],self);
    DSPSetCurrentDSP(self->_orchIndex);
    return self;
}

-setData:(DSPDatum *)dataArray length:(int)len offset:(int)off
    /* Load data and check size. Offset is shift from start of memory block. */
{
    if (!sendPreamble(self,dataArray,len,off))
	return nil;
    if (DSPMKSendArraySkipTimed(_MKCurSample(orchestra),dataArray,
				orchAddr.memSpace,orchAddr.address + off,
				1,len)) 
      return _MKErrorf(MK_synthDataLoadErr);
    return self;
}

-setData:(DSPDatum *)dataArray 
    /* Same as above, but uses our size as the array length. */
{
    return [self setData:dataArray length:length offset:0];
}

-setShortData:(short *)dataArray length:(int)len offset:(int)off
    /* Load data and check size. Offset is shift from start of memory block. */
{
    if (!sendPreamble(self,dataArray,len,off))
	return nil;
    if (DSPMKSendShortArraySkipTimed(_MKCurSample(orchestra),dataArray,
				     orchAddr.memSpace,orchAddr.address + off,
				     1,len)) 
      return _MKErrorf(MK_synthDataLoadErr);
    return self;
}

-setShortData:(short *)dataArray 
{
    return [self setShortData:dataArray length:length offset:0];
}

-setToConstant:(DSPDatum)value length:(int)len offset:(int)off
    /* Load data and check size. Offset is shift from start of memory block. */
{
    if (!sendPreamble(self,CONSTANT,len,off,value))
	return nil;
    if (DSPMKMemoryFillSkipTimed(_MKCurSample(orchestra),value,
				 orchAddr.memSpace,orchAddr.address + off,
				 1,len)) 
      return _MKErrorf(MK_synthDataLoadErr);
    return self;
}

-setToConstant:(DSPDatum)value
  /* Same as above but sets entire memory block to specified value. */
{
    return [self setToConstant:value length:length offset:0];
}

-(BOOL)readOnly
  /* Returns YES if the receiver is read-only. */
{
    return readOnly;
}

-setReadOnly:(BOOL)readOnlyFlag
  /* Sets whether the receiver is read-only. Default is no. Anyone can
     change a readOnly object to read-write by first calling setReadOnly:.
     Thus readOnly is more of a "curtesy flag".
     The exception is the Sine ROM, the MuLaw ROM and the "zero" patchpoints.
     These are protected. An attempt to make them read-write is ignored.*/
{
    if (_protected)
      return self;
    readOnly = readOnlyFlag;
    return self;
}


-(BOOL)isAllocated     
  /* Returns YES */
{
    return YES;
}

-(int)referenceCount
{
    if (_sharedKey)
      return _MKGetSharedSynthReferenceCount(_sharedKey);
    return 1;
}

@end

@implementation SynthData(Private)

-(MKOrchMemStruct *) _resources     
  /* return pointer to memory requirements of this unit generator. */
{
    return &_reso;
}
-_deallocAndAddToList
    /* For memory, we do not keep a class-wide free list. Instead, we
       give the contained memory back to the Orchestra's free list. */
{
    _MKFreeMem(orchestra,&orchAddr);
    [super free];
    return nil;
}

+(id)_newInOrch:(id)anOrch index:(unsigned short)whichDSP 
 length:(int)size segment:(MKOrchMemSegment)whichSegment 
 baseAddr:(DSPAddress)addr
{
//    self = [super allocFromZone:NXDefaultMallocZone()];
    self = [super new];
    orchestra = anOrch;
    _orchIndex = whichDSP;
    _reso.pLoop = _reso.pSubr = 
      _reso.xArg = _reso.lArg = 
	_reso.yArg = 0;
    length = size;
    switch (whichSegment) {
      case MK_xData:
      case MK_xPatch:
	_reso.xData = size;
        _reso.yData = 0;	
	orchAddr.memSpace = DSP_MS_X;
	break;
      case MK_yData:
      case MK_yPatch:   /* We use this class for patchpoint also. */
        _reso.xData = 0;	
	_reso.yData = size;
	orchAddr.memSpace = DSP_MS_Y;
	break;    
    }
    orchAddr.memSegment = whichSegment;  
    orchAddr.address = addr;      
    orchAddr.orchIndex = whichDSP;
    return self;
}

#import "_synthElementMethods.m"

@end



