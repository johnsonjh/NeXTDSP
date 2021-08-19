/* 
	InterpUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface InterpUG:UnitGenerator
/* InterpUG  - from dsp macro /usr/lib/dsp/ugsrc/interp.asm (see source for details).

   Outputs an interpolation of two input signals, with the blend controlled 
   by a third signal, i.e.,   

       out = input1 + (input2-input1) * control
   
   You allocate one of the subclasses InterpUG<a><b><c><d>, where <a> is the output 
   space, <b> and <c> are the input signal spaces, and <d> is the space of the 
   interpolation control signal.
   This unit generator is 25% faster if <b> is x and <c> is y.

*/

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible */

-setInput1:aPatchPoint;
/* Sets input1 of interpolator. */

-setInput2:aPatchPoint;
/* Sets input2 of interpolator. */

-setInterpInput:aPatchPoint;
/* Sets interpolation signal of interpolator. */

-setOutput:aPatchPoint;
/* Sets output of adder. */

@end





