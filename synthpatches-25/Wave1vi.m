#ifdef SHLIB
#include "shlib.h"
#endif

/* This class fills in the vibrato functionality missing from Wave1i. */

/* Modification history:

   08/28/90/daj - Changed initialize to init.
*/

#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Wave1vi.h"
#import "_Wave1i.h"
  
@implementation Wave1vi

WAVEDECL(allVibTemplate,allVibUgs);
WAVEDECL(sinVibTemplate,sinVibUgs);
WAVEDECL(ranVibTemplate,ranVibUgs);

+patchTemplateFor:aNote
{
    if (aNote) {
	double svibpc = MKGetNoteParAsDouble(aNote, MK_svibAmp);
	double rvibpc = MKGetNoteParAsDouble(aNote, MK_rvibAmp);
	if (svibpc && rvibpc) {
	    if (!allVibTemplate)
	      allVibTemplate = _MKSPGetWaveAllVibTemplate(&allVibUgs,
							[OscgafiUGxxyy class]);
	    return allVibTemplate;
	}
	else if (rvibpc) {
	    if (!ranVibTemplate)
	      ranVibTemplate = _MKSPGetWaveRanVibTemplate(&ranVibUgs,
							[OscgafiUGxxyy class]);
	    return ranVibTemplate;
	}
	else if (svibpc) {
	    if (!sinVibTemplate)
	      sinVibTemplate = _MKSPGetWaveSinVibTemplate(&sinVibUgs,
							[OscgafiUGxxyy class]);
	    return sinVibTemplate;
	}
	else
	  return [super patchTemplateFor:aNote];
    }
    if (!allVibTemplate)
      allVibTemplate = _MKSPGetWaveAllVibTemplate(&allVibUgs,
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

@implementation Wave1vi(Private)

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
    vibWaveform = nil;
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
    if (WAVENUM(svibUG) >= 0) {	
	if (setVibWaveform) 
	  [WAVEUG(svibUG) setTable:vibWaveform length:128 defaultToSineROM:YES];
	if (setVibFreq) 
	  [WAVEUG(svibUG) setFreq:svibFreq0 + (svibFreq1-svibFreq0) * 
	   MIDIVAL(modWheel)];
	if (setVibAmp)
	  [WAVEUG(svibUG) setAmp:svibAmp0 + (svibAmp1-svibAmp0) * 
	   MIDIVAL(modWheel)];
    }
    if (WAVENUM(nvibUG)>=0) {
	if (setRandomVib) {
	    [WAVEUG(onepUG) setB0:0.004 * rvibAmp]; /* Filter gain (rvibAmp) */
	    [WAVEUG(onepUG) clear];                 /* Clear filter state */
	    if (newPhrase) {
		[WAVEUG(onepUG) setA1:-0.9999]; /* Filter feedback coefficient */
		[WAVEUG(nvibUG) anySeed]; /* each instance has different vib */
	    }
	}
    }
}

@end
