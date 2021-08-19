/* 
	DswitchUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>

@interface DswitchtUG : UnitGenerator

/* DswitchtUG  - from dsp macro /usr/lib/dsp/ugsrc/dswitcht.asm (see source for details).

  The DswitchUG switches from input1 (scaled by scale1) to a 
  input2 (scaled by scale2) after a delay specified in samples.  The delay
  can be interpreted as the number of samples input1 is passed to 
  the output.  On each output sample, the delay is decremented by 1.
  Input1 times the scale factor scale1 is passed to the output as long 
  as delay remains nonnegative. Afterwards, input2 is passed 
  to the output with its own scaling.

  You instantiate a subclass of the form 
  DswitchtUG<a><b>, where 
	<a> = space of output	
	<b> = space of inputs
*/	

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except the
   delay counter. */

-setInput1:aPatchPoint;
/* Sets input1 to specified patchPoint. */

-setInput2:aPatchPoint;
/* Sets input2 to specified patchPoint. */

-setOutput:aPatchPoint;
/* Sets output to specified patchPoint. */

-setScale1:(double)val;
/* Sets constant to scale input1 values. */

-setScale2:(double)val;
/* Sets constant to scale input2 values. */

-setDelayTicks:(int)val;
/* Sets delay in ticks (units of DSPMK_NTICK). 
   A negative value will switch immediately to input2. */

-idleSelf;
/* Patches output to sink. */

@end









