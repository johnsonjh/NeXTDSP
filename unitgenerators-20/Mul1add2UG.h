/* 
	Mul1add2UG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface Mul1add2UG:UnitGenerator
/* Mul1add2UG  - from dsp macro /usr/lib/dsp/ugsrc/mul1add2.asm (see source for details).

   Outputs the sum of one input signal and the product of two others, i.e,

       out = input1 + (input2 * input3)
   
   You allocate one of the subclasses Mul1add2UG<a><b><c><d>, where <a> is the output 
   space, and <b>, <c>, and <d> are the spaces of the inputs.

	The number of inner loop instructions is:
		spaces:			# instructions:
	    out  in1  in2  in3
	     y    x    y    x		2
	     *    *    y    x		3
	     y    x    *    *		3
		all others 		4

*/

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible */

-setInput1:aPatchPoint;
/* Sets input1 of adder. */

-setInput2:aPatchPoint;
/* Sets input2 of adder. */

-setInput3:aPatchPoint;
/* Sets input3 of adder. */

-setOutput:aPatchPoint;
/* Sets output of adder. */

@end





