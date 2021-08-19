/* This class is just like Fm1i but overrides the interpolating osc
   with a non-interpolating osc. Thus, it is slightly less expensive than
   Fm1i. */

#import "Fm1i.h"

@interface Fm1:Fm1i
{
}

+patchTemplateFor:aNote;
/* Returns a template using the non-interpolating osc. */

@end
