/* 
	DelayUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/

#import <musickit/UnitGenerator.h>

@interface DelayUG : UnitGenerator
/* DelayUG  - from dsp macro /usr/lib/dsp/ugsrc/delay.asm (see source for details).

	You instantiate a subclass of the form 
	DelayUG<a><b><c>, where 
	<a> = space of output
	<b> = space of input
	<c> = space of delay line

	DelayUG is useful for flanging, reverberation, plucked string 
	synthesis, etc.
*/	
{
    id memObj;      /* Delay memory */
    int len;        /* Currently used length (must be <= length of memObj) */
}
+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except the
   delay pointer. */

-setInput:aPatchPoint;
/* Sets input patchpoint as specified. */
-setOutput:aPatchPoint;
/* Sets output patchpoint as specified. */

-setDelayMemory:aSynthData;
/* Sets the delay memory to aSynthData.
   If you pass nil, uses sink as the delay memory. It is up to the caller
   to insure the memory is cleared. */

-adjustLength:(int)newLength;
/* If no setDelayMemory: message has been received, returns nil.
   Otherwise, adjusts the delay length as indicated. newLength
   must be <= the length of the block of memory specified
   in setDelayMemory:. Otherwise, nil is returned. Note
   that the unused memory in the memory specified in 
   setDelayMemory: is not freed. Resetting the
   length of a running Delay may cause the pointer to go out-of-bounds.
   Therefore, it is prudent to send setPointer: or resetPointer after
   adjustLength:. Also note that when lengthening the delay, you 
   will be bringing in old delayed samples. Therefore, you may
   want to clear the new portion by sending the memory object the
   message -setToConstant:length:offset:. */

-setPointer:(int)offset;
/* If no setDelayMemory: message has been received, returns nil.
   Else sets pointer to specified offset. E.g. if offset == 0,
   this is the same as resetPointer. If offset is GEQ the length
   of the memory block, returns nil. */

-resetPointer;
/* If no setDelayMemory: message has been received, returns nil.
   Else sets pointer to start of memory. This is done automatically
   when setDelayMemory: is sent. */

-(int)length;
/* Returns the length of the delay currently in use. This is always <= than the
   length of the memory object. */

-runSelf;
/* Does nothing. */
-idleSelf;
/* Patches output and delay memory to sink. */


@end









