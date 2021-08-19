#ifdef SHLIB
#include "shlib.h"
#endif

/* This class is just like Wave1i but overrides the interpolating osc
   with a non-interpolating osc. */

/* Modification history:

   08/28/90/daj - Changed initialize to init.
*/

#import <musickit/musickit.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Wave1.h"
#import "_Wave1i.h"
  
@implementation Wave1

WAVEDECL(template,ugs);

+patchTemplateFor:aNote
{
    if (!template)
      template = _MKSPGetWaveNoVibTemplate(&ugs,[OscgafUGxxyy class]);
    return template;
}

-init
  /* Sent by this class on object creation and reset. */
{
    [self _setDefaults]; /* We don't need to send [super init] here */
    _ugNums = &ugs;
    return self;
}

@end
