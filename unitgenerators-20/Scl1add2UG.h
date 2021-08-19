/* 
	Scl1add2UG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface Scl1add2UG:UnitGenerator
  /*    Scl1add2UG. From dsp macro /usr/lib/dsp/ugsrc/scl1add2.asm (see source for details)

	You instantiate a subclass of the form Scl1add2UG<a><b><c>, where 
	<a> = space of output
	<b> = space of input1
	<c> = space of input2

      The scl1add2 unit-generator multiplies the first input by a
      scale factor, and adds it to the second input signal to produce a
      third.  The output vector can be the same as an input vector.
      Faster if space of input1 is not the same as the space of input2.
*/

-setInput1:aPatchPoint;
/* Sets input1. This is the input that is scaled. */

-setInput2:aPatchPoint;
/* Sets input2. */

-setOutput:aPatchPoint;
/* Sets output. */

-setScale:(double)val;
/* Sets scaling on input1. */

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible. */

-idleSelf;
  /* Sets output to write to sink. */

@end






