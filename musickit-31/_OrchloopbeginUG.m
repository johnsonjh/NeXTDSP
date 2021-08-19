#ifdef SHLIB
#include "shlib.h"
#endif

/*
  OrchSysUG.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  11/10/89/daj - In order to implement filling up the sound out buffers
                 before starting to play, did the following:
		 Recompiled orchloopbeginIncludeUG.m. 
		 Added looper: argument to _setXArgsAddr:y:l:looper.
		 Added _unpause method. 
  03/13/90/daj - Changed name to _OrchloopbeginUG.
  04/23/90/daj - Got rid of instance variable and added arg to _pause:
                 to fix bug.
  04/25/90/daj - Changed arg order in calls to DSPWriteValue, to conform
                 with no libdsp api.
  05/01/90/jos - Removed "r" prefix from rData and rWordCount

*/
#import "_musickit.h"
#import "_OrchloopbeginUG.h"
@implementation _OrchloopbeginUG:UnitGenerator
/* This is a unit generator which contains the orchestra system.
   It's derived from beg_orcl in dsp/smsrc/beginend.asm.
 */
{
}
#import "_SynthElement.h"
#import "orchloopbeginUGInclude.m"

#define LOOPER_JUMP(_arrSize) (_arrSize - 1)

+_setXArgsAddr:(int)xArgsAddr y:(int)yArgsAddr l:(int)lArgsAddr 
 looper:(int)looperWord
{
    MKLeafUGStruct *info = [self classInfo]; 
    DSPDataRecord *dRec = info->data[(int)DSP_LC_P]; 
    int *pData = dRec->data;         /* The data array */
    int arrSize = dRec->wordCount;   

#   define ADDR(_x) (_x << 8)
#   define XARG_ADDR arrSize - 5
#   define YARG_ADDR arrSize - 3
#   define LARG_MOVE arrSize - 2
#   define LMOVEOP ((unsigned)0x320000)

    /* Fix up arg pointers */
    pData[XARG_ADDR] = xArgsAddr; 
    pData[YARG_ADDR] = yArgsAddr;
    pData[LARG_MOVE] = LMOVEOP | ADDR(lArgsAddr); 
    pData[LOOPER_JUMP(arrSize)] = looperWord; /* always loop back. */
    return self;
}

-_unpause
{
    MKLeafUGStruct *info = [self classInfo]; 
    DSPDataRecord *dRec = info->data[(int)DSP_LC_P]; 
    int arrSize = dRec->wordCount;   
    int address = LOOPER_JUMP(arrSize) + relocation.pLoop;
#   define NOOP 0x0      
    DSPWriteValue(NOOP,DSP_MS_P,address);
    return self;
}

-_pause:(int)looperWord
{
    MKLeafUGStruct *info = [self classInfo]; 
    DSPDataRecord *dRec = info->data[(int)DSP_LC_P]; 
    int arrSize = dRec->wordCount;   
    int address = LOOPER_JUMP(arrSize) + relocation.pLoop;
    DSPWriteValue(looperWord,DSP_MS_P,address);
    return self;
}

@end

