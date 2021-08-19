#ifdef SHLIB
#include "shlib.h"
#endif
/* OscgafiUG - interpolating oscillator with amplitude and frequency envelopes. */
/* 
Modification history:

  11/13/89/daj - Simplified defaultTableLength:.

*/
#import <musickit/musickit.h>
#import "_exportedPrivateMusickit.h"
#import <soundkit/Sound.h>
#import "OscgafiUG.h"

@implementation OscgafiUG:OscgafUGs
/* OscgafiUG<a><b><c><d>, where <a> is the output space, <b> is the amplitude
   input space, <c> is the increment input space, and <b> is the table space.

   See documentation for OscgafUGs.

*/

typedef enum _args { aina, atab, inc, ainf, aout, mtab, phs} args;
#import "oscgafiUGInclude.m"

#if 0
+(int)defaultTableLength:anObj
  /* Provides for a power-of-2 table length with a reasonable number of samples for
     the highest component. */
{
  if ([anObj isKindOf:[Samples class]])
    return [[anObj sound] sampleCount];
  else if ([anObj isKindOf:[Partials class]]) {
    switch ((int)ceil([anObj highestFreqRatio] * .0625)) { /* /16 */
      case 0: return 64;    /* no partials? */
      case 1: return 128;   /* 1 to 16 */
      case 2:               /* 17 to 32 */
      case 3:               /* 33 to 48 */
      case 4:               /* 48 to 64 */
	return 256;   
      default:
	return 512;
    }
  }
  else if ([anObj respondsTo:@selector(length)])
    return (int)[anObj length];
  else return 128;
}
#endif

+(int)defaultTableLength:anObj
  /* Provides for a power-of-2 table length */
{
  if ([anObj isKindOf:_MKClassPartials()])
    return 128;
  if ([anObj isKindOf:_MKClassSamples()])
    return [[anObj sound] sampleCount];
  else if ([anObj respondsTo:@selector(length)])
    return (int)[anObj length];
  else return 128;
}

@end
