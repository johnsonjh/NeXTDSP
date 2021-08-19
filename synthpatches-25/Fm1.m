#ifdef SHLIB
#include "shlib.h"
#endif

/* This class is just like Fm1i but overrides the interpolating osc
   with a non-interpolating osc. 

   Modification history:


    08/28/90/daj - Changed initialize to init.
*/


#import <musickit/musickit.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Fm1.h"
#import "_Fm1i.h"
  
@implementation Fm1

FMDECL(template,ugs);

+patchTemplateFor:aNote
{
    if (!template)
      template = _MKSPGetFmNoVibTemplate(&ugs,[OscgafUGxxyy class]);
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
