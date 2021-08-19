/* 
	Add2UG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface Add2UG:UnitGenerator
/* Add2UG  - from dsp macro /usr/lib/dsp/ugsrc/add2.asm (see source for details).

   Outputs the sum of two input signals. 
   
   You allocate one of the subclasses Add2UG<a><b><c>, where <a> is the output 
   space, <b> is the space of the first input and <c> is the space of the
   second input.   This unit generator is faster if <b> is x and <c> is y.

*/

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible */

-setInput1:aPatchPoint;
/* Sets input1 of adder. */

-setInput2:aPatchPoint;
/* Sets input2 of adder. */

-setOutput:aPatchPoint;
/* Sets output of adder. */

@end





