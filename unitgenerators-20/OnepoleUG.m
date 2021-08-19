#ifdef SHLIB
#include "shlib.h"
#endif
/* 
Modification history:

  11/22/89/daj - Changed to use UnitGenerator C functions instead of methods.
*/
#import <musickit/musickit.h>
#import "_unitGeneratorInclude.h"

#import "OnepoleUG.h"

@implementation OnepoleUG:UnitGenerator
/* One pole filter.
	You instantiate a subclass of the form 
	OnepoleUG<a><b>, where 
	<a> = space of output
	<b> = space of input

      The onepole unit-generator implements a one-pole
      filter section in direct form. In pseudo-C notation:

      for (n=0;n<DSPMK_NTICK;n++) {
           s = bb0*sinp:ainp[n] - aa1*s;
           sout:aout[n] = s;
      }

      */
{
}

enum args { ainp, aout, s, aa1, bb0};

#import "onepoleUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != s);
}
#endif _MK_UGOPTIMIZE

-idleSelf
  /* Sets output to write to sink. */
{
    [self setAddressArgToSink:aout];
    return self;
}

-setInput:aPatchPoint
{
    return MKSetUGAddressArg(self,ainp,aPatchPoint);
}

-setOutput:aPatchPoint
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

-setB0:(double)val
  /* Sets undelayed sample gain. */
{
    return MKSetUGDatumArg(self,bb0,DSPDoubleToFix24(val));
}

-setA1:(double)val
  /* Sets gain of delayed sample. */
{
    return MKSetUGDatumArg(self,aa1,DSPDoubleToFix24(val));
}

-clear
  /* Clears filter's state variable. */
{
    return MKSetUGDatumArg(self,s,0);
}

-setBrightness:(double)gain forFreq:(double)freq
  /* This is a convenient way to use a onepole filter as a frequency-dependent
     brightness control, as described in Jaffe/Smith, Computer Music Journal
     Vol. 7, No. 2, Summer 1983. */
{
#   ifndef MAX
#   define  MAX(A,B)	((A) > (B) ? (A) : (B))
#   endif
#   ifndef MIN
#   define  MIN(A,B)	((A) < (B) ? (A) : (B))
#   endif
#   define LOG10 log(10.0)
    double poleRadius;
    gain = MAX(MIN(gain,.999),0);
    gain = 1 - pow(10.0,-3 * gain)/.999;
    if (gain < .001)
      gain = .001; 
    if (gain > .9995) {
	poleRadius = 0;
	gain = 1.0;
    } else {
#       define TWOPI (M_PI + M_PI)
	double gSquared = pow(gain,2.0);
	double omega = freq/[orchestra samplingRate];
	double piOmega = M_PI * omega;
	double firstTerm,secondTerm,val;
	firstTerm = (1 - gSquared * cos(omega * TWOPI))/(1 - gSquared); 
	secondTerm = 2 * gain * sin(piOmega) * 
	  sqrt(1.0 - gSquared * pow(cos(piOmega),2.0)) / (1 - gSquared);
	val = MIN(firstTerm + secondTerm, firstTerm - secondTerm);
	poleRadius = MAX(val,0.0);
    }
    [self setA1: - poleRadius];
    [self setB0: MIN( (1.0 - poleRadius)/gain, 1.0 ) ];
    return self;
}

@end
