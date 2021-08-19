#ifdef SHLIB
#include "shlib.h"
#endif

/* This class is just like Fm1vi but overrides the interpolating osc
   with a non-interpolating osc. 

   Modification history:

   08/28/90/daj - Changed initialize to init.
*/

#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Fm1v.h"
#import "_Fm1i.h"
  
@implementation Fm1v

FMDECL(allVibTemplate,allVibUgs);
FMDECL(sinVibTemplate,sinVibUgs);
FMDECL(ranVibTemplate,ranVibUgs);
FMDECL(noVibTemplate,noVibUgs);

+patchTemplateFor:aNote
{
    if (aNote) {
	double svibpc = MKGetNoteParAsDouble(aNote, MK_svibAmp);
	double rvibpc = MKGetNoteParAsDouble(aNote, MK_rvibAmp);
	if (svibpc && rvibpc) {
	    if (!allVibTemplate)
	      allVibTemplate = _MKSPGetFmAllVibTemplate(&sinVibUgs,
						      [OscgafUGxxyy class]);
	    return allVibTemplate;
	}
	else if (rvibpc) {
	    if (!ranVibTemplate)
	      ranVibTemplate = _MKSPGetFmRanVibTemplate(&ranVibUgs,
						      [OscgafUGxxyy class]);
	    return ranVibTemplate;
	}
	else if (svibpc) {
	    if (!sinVibTemplate)
	      sinVibTemplate = _MKSPGetFmSinVibTemplate(&sinVibUgs,
						      [OscgafUGxxyy class]);
	    return sinVibTemplate;
	}
	else {
	    if (!noVibTemplate)
 	      noVibTemplate = _MKSPGetFmNoVibTemplate(&noVibUgs,
						      [OscgafUGxxyy class]);
	    return noVibTemplate;
	    
	}
    }
    if (!allVibTemplate)
      allVibTemplate = _MKSPGetFmAllVibTemplate(&allVibUgs,
						[OscgafUGxxyy class]);
    return allVibTemplate;
}

-init
  /* Sent by this class on object creation and reset. */
{
    [super init];
    if (patchTemplate == allVibTemplate)
      _ugNums = &allVibUgs;
    else if (patchTemplate == ranVibTemplate)
      _ugNums = &ranVibUgs;
    else if (patchTemplate == sinVibTemplate)
      _ugNums = &sinVibUgs;
    else _ugNums = &noVibUgs;
    return self;
}

@end
