/* 
	OnepoleUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>

@interface OnepoleUG : UnitGenerator
/* OnepoleUG  - from dsp macro /usr/lib/dsp/ugsrc/onepole.asm (see source for details).

   The onepole unit-generator implements a one-pole
   filter section in direct form. 

   You instantiate a subclass of the form 
   OnepoleUG<a><b>, where <a> = space of output and <b> = space of input.
*/

-setInput:aPatchPoint;
/* Sets filter input. */

-setOutput:aPatchPoint;
/* Sets filter output. */

-setB0:(double)val;
/* Sets gain of filter. */

-setA1:(double)val;
/* Sets gain of delayed output sample. */

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible except the
   filter state. */

-clear;
/* Clears internal filter running term. */

-setBrightness:(double)gain forFreq:(double)freq;
/* This is a convenient way to use a onepole filter as a frequency-dependent
   brightness control, as described in Jaffe/Smith, Computer Music Journal
   Vol. 7, No. 2, Summer 1983. 

   You specify the gain at the specified fundamental frequency and the 
   appropriate filter frequency response is selected for you. By keeping
   the gain constant and varying the frequency, you can have a uniform
   amplitude and brightness percept (i.e. a "dynamic level").  

   Note that setting the brightness does not clear the filter state variable.
   You may want to do this in some cases.
*/

@end








