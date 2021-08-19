#ifdef SHLIB
#include "shlib.h"
#endif

/* This class fills in the vibrato functionality missing from Fm1i. 

   Modification history:

  08/28/90/daj - Changed initialize to init.

*/

#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Fm1vi.h"
#import "_Fm1i.h"
  
@implementation Fm1vi

FMDECL(allVibTemplate,allVibUgs);
FMDECL(sinVibTemplate,sinVibUgs);
FMDECL(ranVibTemplate,ranVibUgs);

+patchTemplateFor:aNote
{
    if (aNote) {
	double svibpc = MKGetNoteParAsDouble(aNote, MK_svibAmp);
	double rvibpc = MKGetNoteParAsDouble(aNote, MK_rvibAmp);
	if (svibpc && rvibpc) {
	    if (!allVibTemplate)
	      allVibTemplate = _MKSPGetFmAllVibTemplate(&allVibUgs,
							[OscgafiUGxxyy class]);
	    return allVibTemplate;
	}
	else if (rvibpc) {
	    if (!ranVibTemplate)
	      ranVibTemplate = _MKSPGetFmRanVibTemplate(&ranVibUgs,
							[OscgafiUGxxyy class]);
	    return ranVibTemplate;
	}
	else if (svibpc) {
	    if (!sinVibTemplate)
	      sinVibTemplate = _MKSPGetFmSinVibTemplate(&sinVibUgs,
							[OscgafiUGxxyy class]);
	    return sinVibTemplate;
	}
	else
	  return [super patchTemplateFor:aNote];
    }
    if (!allVibTemplate)
      allVibTemplate = _MKSPGetFmAllVibTemplate(&allVibUgs,
						[OscgafiUGxxyy class]);
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
    return self;
}

@end

@implementation Fm1vi(Private)

-_setDefaults
  /* Set the instance variables to reasonable default values. We do this
   * after each phrase and upon initialization. This insures that a freshly 
   * allocated SynthPatch will be in a known state. See <musickit/params.h> 
   */
{
    [super _setDefaults];
    svibAmp0 = svibAmp1 = 0.0;        // Periodic vibrato amp
    svibFreq0 = svibFreq1 = 0.0;      // Periodic vibrato freq
    rvibAmp   = 0.0;                  // Random vibrato amplitude
    modWheel = MIDI_MAXDATA;
    return self;
}

-(void)_setModWheel:(int)val
{
    modWheel = val;
}

-(void)_setSvibFreq0:(double)val
{
    svibFreq0 = val;
}

-(void)_setSvibFreq1:(double)val
{
    svibFreq1 = val;
}

-(void)_setSvibAmp0:(double)val
{
    svibAmp0 = val;
}

-(void)_setSvibAmp1:(double)val
{
    svibAmp1 = val;
}

-(void)_setRvibAmp:(double)val
{
    rvibAmp = val;
}

-(void)_setVibWaveform:(id)obj
{
    vibWaveform = obj;
}

-(void)_setVib:(BOOL)setVibWaveform :(BOOL)setVibFreq :(BOOL)setVibAmp 
  :(BOOL)setRandomVib :(BOOL)newPhrase
{
    /* ----------------------------- Vibrato ---------------------------- */
    if (FMNUM(svibUG) >= 0) {	
	if (setVibWaveform) 
	  [FMUG(svibUG) setTable:vibWaveform length:128 defaultToSineROM:YES];
	if (setVibFreq) 
	  [FMUG(svibUG) setFreq:svibFreq0 + (svibFreq1-svibFreq0) * 
	   MIDIVAL(modWheel)];
	if (setVibAmp)
	  [FMUG(svibUG) setAmp:svibAmp0 + (svibAmp1-svibAmp0) * 
	   MIDIVAL(modWheel)];
    }
    if (FMNUM(nvibUG)>=0) {
	if (setRandomVib) {
	    [FMUG(onepUG) setB0:0.004 * rvibAmp]; /* Filter gain (rvibAmp) */
	    [FMUG(onepUG) clear];                 /* Clear filter state */
	    if (newPhrase) {
		[FMUG(onepUG) setA1:-0.9999]; /* Filter feedback coefficient */
		[FMUG(nvibUG) anySeed]; /* each instance has different vib */
	    }
	}
    }
}

@end
