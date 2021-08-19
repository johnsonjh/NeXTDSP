/* 
	ConstantUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
@interface ConstantUG:UnitGenerator
/* ConstantUG  - from dsp macro /usr/lib/dsp/ugsrc/constant.asm (see source for details).

   Outputs a constant.
   
   You allocate one of the subclasses ConstantUG<a>, where <a> is the output 
   space. 

   Note that you ONLY need to use the ConstantUG if the patchpoint you're 
   writing will be overwritten by another UnitGenerator.  If you merely want
   to reference an unchanging constant, you do not need a ConstantUG; just
   allocate a SynthData and use its setToConstant: method.

   An example where the ConstantUG IS needed is in doing a reverberator.
   A global input patch point is published and the reverberator first reads
   this patchpoint, then sets it to 0 with a ConstantUG.  Then any 
   SynthPatches that write to the patchpoint add in to whatever's there.
*/
+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible. */

-idleSelf;
/* Sets output patchpoint to sink. */

-setConstantDSPDatum:(DSPDatum)value;
/* Sets constant value as int. */

-setConstant:(double)value;
/* Sets constant value as double. */

-setOutput:aPatchPoint;
/* Sets output location. */ 

@end





