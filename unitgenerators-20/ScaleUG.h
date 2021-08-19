/* 
	ScaleUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface ScaleUG:UnitGenerator
/* 
  ScaleUG - from dsp macro /usr/lib/dsp/ugsrc/scale.asm (see source for details).

  You instantiate a subclass of the form 
  ScaleUG<a><b>, where <a> = space of output and <b> = space of input.

  The scale unit-generator simply copies one signal vector over to
  another, multiplying by a scale factor.  The output patchpoint can 
  be the same as the input patchpoint.
*/

-setInput:aPatchPoint;
/* Sets input patchpoint. */

-setOutput:aPatchPoint;
/* Sets output patchpoint. */

-setScale:(double)val;
/* Sets scale factor. */

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible. */

@end






