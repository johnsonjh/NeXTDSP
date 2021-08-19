/* 
	Out2sumUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/

#import <musickit/UnitGenerator.h>

@interface Out2sumUG : UnitGenerator
/* Out2sumUG - from dsp macro /usr/lib/dsp/ugsrc/out2sum.asm (see source for details).

   Out2sum writes its input signal to both channels of the stereo output 
   sample stream of the DSP, adding into that stream. 
   The stream is cleared before each DSP tick (each orchestra program 
   iteration). Out2sum also provides individual scaling on each output channel.
   The method setBearing: allows a convenient way to control the proportion
   of the signal sent to each channel.

   You instantiate a subclass of the form 
   Out2sumUG<a>, where <a> = space of input

   */
{
  BOOL _reservedOut2sum1; 
  double bearingScale;
}

+(BOOL)shouldOptimize:(unsigned) arg;
/* Specifies that all arguments are to be optimized if possible. */

-setLeftScale:(double)val;
/* Sets scaling for left channel. */ 

-setRightScale:(double)val;
/* Sets scaling for right channel. */ 

-setBearing:(double)val;
/* As a convenience, you can set both scaleA and scaleB with a single 
   message. 

   When val is 0, the signal is equally distributed between the two channels.
   When val is -45, you get the left channel, +45 you get the right channel.
   Val = 90 is the same as val = 0. */  

-setBearing:(double)val scale:(double)aScale;
/* Same as setBearing:, but including an overall amp scaling factor independent
   of bearing. */

-runSelf;
/* If bearing has not been set, sets it to 0. */

-idleSelf;
/* Since Out2sum has no outputs, it idles itself by patching its inputs
   to zero. Thus, an idle Out2sum makes no sound. */ 

-setInput:aPatchPoint;
/* Sets input patch point. */
@end











