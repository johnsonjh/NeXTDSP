#ifdef SHLIB
#include "shlib.h"
#endif

/* 
  Modification history:

  11/25/89/mmm - Fixed bug in wavelens.
  04/25/90/daj - Changed name of updateStringPar to updateStringParNoCopy
  08/28/90/daj - Changed initialize to init.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things

*/
#define MK_INLINE 1

#import "musickit/musickit.h"
#import <midi/midi_types.h>
#import <objc/HashTable.h>
#import "musickit/unitgenerators/unitgenerators.h"
#import "DBWave2vi.h"
/* Database of analyzed timbres */
#import "partialsDB.h"

@implementation DBWave2vi:SynthPatch
  /* Dual-wavetable with interpolating oscillators and dynamic
     interpolation between them, with optional
     sinusoidal and/or random vibrato.  Wavetables may be loaded from
     the timbre database.      See comments in DBWave2vi.h */
{
  double amp0, amp1, ampAtt, ampRel, freq0, freq1, freqAtt, freqRel,
         bearing, phase, portamento, svibAmp0, svibAmp1, rvibAmp,
         svibFreq0, svibFreq1, velocitySensitivity, panSensitivity,
         waveformAtt, waveformRel, pitchbendSensitivity;
  id ampEnv, freqEnv, waveform0, waveform1, waveformEnv;
  int wavelen, volume, velocity, modwheel, pan, pitchbend;
  void *_ugNums;
}

#import "_synthPatchInclude.h"

struct ugNums {
  int ampUG, incUG, oscUG1, oscUG2, outUG, svibUG, nvibUG, onepUG,
      addUG, mulUG, intUG, mixUG, xsig, ysig1, ysig2;
};
static struct ugNums *noVibUgs, *sinVibUgs, *ranVibUgs, *allVibUgs;

static id noVibTemplate = nil;   /* Template with no vibrato */
static id sinVibTemplate = nil;  /* Template with sinusoidal vibrato */
static id ranVibTemplate = nil;  /* Template with random vibrato */
static id allVibTemplate = nil;  /* Template with both vibratos */


static id getAllVibTemplate() {
  static id template = nil;
  static struct ugNums ugs;
  if (template) return template;
  template = [PatchTemplate new];
  allVibTemplate = template;
  allVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.svibUG  = [template addUnitGenerator:[OscgUGyy class]];
  ugs.nvibUG  = [template addUnitGenerator:[SnoiseUGx class]];
  ugs.onepUG  = [template addUnitGenerator:[OnepoleUGxx class]];
  ugs.addUG   = [template addUnitGenerator:[Add2UGyxy class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG   = [template addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG1  = [template addUnitGenerator:[OscgafiUGyxyy class]];
  ugs.oscUG2  = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.intUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mixUG   = [template addUnitGenerator:[InterpUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  ugs.ysig2 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.svibUG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.nvibUG  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.onepUG  sel:@selector(setInput:)    arg:ugs.xsig];
  [template to:ugs.onepUG  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.addUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.addUG   sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG   sel:@selector(setInput3:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG1  sel:@selector(setOutput:)   arg:ugs.ysig2];
  [template to:ugs.oscUG2  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG2  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG2  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.intUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mixUG   sel:@selector(setInput2:)   arg:ugs.ysig2];
  [template to:ugs.mixUG   sel:@selector(setInterpInput:) arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  return template;
  }


static id getRanVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  ranVibTemplate = template;
  ranVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.nvibUG  = [template addUnitGenerator:[SnoiseUGx class]];
  ugs.onepUG  = [template addUnitGenerator:[OnepoleUGyx class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG   = [template addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG1  = [template addUnitGenerator:[OscgafiUGyxyy class]];
  ugs.oscUG2  = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.intUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mixUG   = [template addUnitGenerator:[InterpUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  ugs.ysig2 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.svibUG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.nvibUG  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.onepUG  sel:@selector(setInput:)    arg:ugs.xsig];
  [template to:ugs.onepUG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.addUG   sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG   sel:@selector(setInput3:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG1  sel:@selector(setOutput:)   arg:ugs.ysig2];
  [template to:ugs.oscUG2  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG2  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG2  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.intUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mixUG   sel:@selector(setInput2:)   arg:ugs.ysig2];
  [template to:ugs.mixUG   sel:@selector(setInterpInput:) arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  return template;
  }


static id getSinVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  sinVibTemplate = template;
  sinVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.svibUG  = [template addUnitGenerator:[OscgUGyy class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG   = [template addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG1  = [template addUnitGenerator:[OscgafiUGyxyy class]];
  ugs.oscUG2  = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.intUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mixUG   = [template addUnitGenerator:[InterpUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  ugs.ysig2 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.svibUG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG   sel:@selector(setInput3:)   arg:ugs.xsig];
  [template to:ugs.mulUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG1  sel:@selector(setOutput:)   arg:ugs.ysig2];
  [template to:ugs.oscUG2  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG2  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG2  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.intUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mixUG   sel:@selector(setInput2:)   arg:ugs.ysig2];
  [template to:ugs.mixUG   sel:@selector(setInterpInput:) arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  return template;
  }

static id getNoVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  noVibTemplate = template;
  noVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.incUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG1  = [template addUnitGenerator:[OscgafiUGyxyy class]];
  ugs.oscUG2  = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.intUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mixUG   = [template addUnitGenerator:[InterpUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  ugs.ysig2 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG1  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG1  sel:@selector(setOutput:)   arg:ugs.ysig2];
  [template to:ugs.oscUG2  sel:@selector(setAmpInput:) arg:ugs.xsig];
  [template to:ugs.oscUG2  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG2  sel:@selector(setOutput:)   arg:ugs.xsig];
  [template to:ugs.intUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setInput1:)   arg:ugs.xsig];
  [template to:ugs.mixUG   sel:@selector(setInput2:)   arg:ugs.ysig2];
  [template to:ugs.mixUG   sel:@selector(setInterpInput:) arg:ugs.ysig1];
  [template to:ugs.mixUG   sel:@selector(setOutput:)   arg:ugs.xsig];
  return template;
  }

+initialize
{
    _MKSPInitializeTimbreDataBase();
    return self;
}

+patchTemplateFor:aNote
    /* Creates and returns a patchTemplate which specifies the
       UnitGenerators and Patchpoints to be used and how they are
       to be interconnected when one of these synthPatches is
       instantiated.  Note that this method only creates the
       specification.  It does not actually instantiate anything.
       This SynthPatch has only one template, but could have variations
       which are returned according to the note parameter values.  */
{
  if (aNote) {
    double svibpc = doublePar(aNote,MK_svibAmp,.01);
    double rvibpc = doublePar(aNote,MK_rvibAmp,.01);
    if (svibpc && rvibpc)
      return getAllVibTemplate();
    else if (rvibpc)
      return getRanVibTemplate();
    else if (svibpc)
      return getSinVibTemplate();
    else
      return getNoVibTemplate();
  }
  else return getAllVibTemplate();
}


-controllerValues:controllers
  /* Initializes controller variables to current controller values when
     synthpatch is set up. */
{
  if ([controllers isKey:(const void *)MIDI_MAINVOLUME])
    volume = (int)[controllers valueForKey:(const void *)MIDI_MAINVOLUME];
  if ([controllers isKey:(const void *)MIDI_MODWHEEL])
    modwheel = (int)[controllers valueForKey:(const void *)MIDI_MODWHEEL];
  if ([controllers isKey:(const void *)MIDI_PAN])
    pan = (int)[controllers valueForKey:(const void *)MIDI_PAN];
  return self;
}


-_setDefaults
  /* Set the static global variables to reasonable values. */
{
    amp0    = 0.0;
    amp1    = MK_DEFAULTAMP;
    ampAtt  = MK_NODVAL;
    ampRel  = MK_NODVAL;
    freq0   = 0.0;
    freq1   = MK_DEFAULTFREQ;
    freqAtt = MK_NODVAL;
    freqRel = MK_NODVAL;
    bearing = MK_DEFAULTBEARING;
    phase   = 0.0;
    wavelen = 0;
    volume  = 127;
    portamento = MK_NODVAL;
    waveform0 = nil;
    waveform1 = nil;
    waveformEnv = nil;
    waveformAtt = MK_NODVAL;
    waveformRel = MK_NODVAL;
    ampEnv   = nil;
    freqEnv  = nil;
    svibAmp0  = 0.0;
    svibAmp1  = 0.0;
    svibFreq0 = 0.0;
    svibFreq1 = 0.0;
    rvibAmp   = 0.0;
    modwheel = 127;
    pan       = MAXINT;
    velocity = MK_DEFAULTVELOCITY;
    velocitySensitivity = 0.5;
    panSensitivity = 1.0;
    pitchbendSensitivity = 3;
    pitchbend = MIDI_ZEROBEND;
    return self;
  }

#define NUM(ugname) (((struct ugNums *)(self->_ugNums))->ugname)
#define UG(ugname) NX_ADDRESS(synthElements)[NUM(ugname)]

-init
  /* Sent by this class on object creation and reset. */
{
  [self _setDefaults];
  if (patchTemplate == allVibTemplate)
    _ugNums = allVibUgs;
  else if (patchTemplate == ranVibTemplate)
    _ugNums = ranVibUgs;
  else if (patchTemplate == sinVibTemplate)
    _ugNums = sinVibUgs;
  else if (patchTemplate == noVibTemplate)
    _ugNums = noVibUgs;
  return self;
}

-_updateParameters:aNote
  /* Updates the synthPatch according to which note parameter values are present
     in the note and the note's relationship to a possible ongoing phrase. 
     The macros of the form "updateXXPar" lookup the value of the specified
     parameter in the current note, and set the given variable if the value is
     valid (i.e., the parameter was actually set in this note).  The macros return
     true if the variable was set and false if it was not set.  Boolean logic
     is then used to determine whether or not to actually send out updates.
     */
{
  MKPhraseStatus phraseStatus = [self phraseStatus];
  BOOL newPhrase = (phraseStatus<=MK_phraseOnPreempt);
  BOOL newNote   = (phraseStatus<=MK_phraseRearticulate);
  BOOL newFreq = updateDoublePar(aNote,MK_freq0,freq0) |
                 updateFreq(aNote,freq1);
  BOOL phraseOn = (phraseStatus == MK_phraseOn);
  int controller = intPar(aNote,MK_controlChange,0);
  char *waveStr0 = "";
  char *waveStr1 = "";
  double fr0, fr1;
  if (newPhrase) 
    updateIntPar(aNote,MK_waveLen,wavelen);
  updateDoublePar(aNote,MK_phase,phase);
  if (phraseOn) {
    [UG(oscUG1) setPhase:phase];
    [UG(oscUG2) setPhase:phase];
  }
  if ((updateDoublePar(aNote,MK_bearing,bearing) |
       ((controller==MIDI_PAN) && updateIntPar(aNote,MK_controlVal,pan)) |
       updateDoublePar(aNote,MK_panSensitivity,panSensitivity) |
       ((controller==MIDI_MAINVOLUME) && updateIntPar(aNote,MK_controlVal,volume))) ||
      newPhrase) {
    if (iValid(pan)) bearing = panSensitivity * 45.0 * (double)(pan-64)/64.0;
    [UG(outUG) setBearing:bearing scale:volumeToAmp(volume)];
  }
  if (((updateWavePar(aNote,MK_waveform0,waveform0) ||
	updateStringParNoCopy(aNote,MK_waveform0,waveStr0)) |
       (updateWavePar(aNote,MK_waveform1,waveform1) ||
	updateStringParNoCopy(aNote,MK_waveform1,waveStr1))) || newPhrase) {
    if sValid(waveStr0) {
      if (*waveStr0=='0')
	waveform0 = _MKSPPartialForTimbre(1 + waveStr0,freq0,0);
	else waveform0 = _MKSPPartialForTimbre(waveStr0,freq1,0);
    }
    if sValid(waveStr1) {
      if (*waveStr1=='0')
	waveform1 = _MKSPPartialForTimbre(1 + waveStr1,freq0,0);
	else waveform1 = _MKSPPartialForTimbre(waveStr1,freq1,0);
    }
    if (newPhrase && (!waveform0 || !waveform1))   /* sine ROM in use? */
      wavelen = 256;
    [UG(oscUG1) setTable:waveform1 length:wavelen defaultToSineROM:newPhrase];
    [UG(oscUG2) setTable:waveform0 length:wavelen defaultToSineROM:newPhrase];
  }
  updateDoublePar(aNote,MK_portamento,portamento);
  updateIntPar(aNote,MK_pitchBendSensitivity,pitchbendSensitivity);
  if ((newFreq |
       updateEnvPar(aNote,MK_freqEnv,freqEnv) |
       updateIntPar(aNote,MK_pitchBend,pitchbend) |
       updateDoublePar(aNote,MK_freqAtt,freqAtt) |
       updateDoublePar(aNote,MK_freqRel,freqRel)) || newNote) {
    if (freq0>0)
      fr0 = MKAdjustFreqWithPitchBend(freq0,pitchbend,
				      pitchbendSensitivity);
    else fr0 = 0;
    if (freq1>0)
      fr1 = MKAdjustFreqWithPitchBend(freq1,pitchbend,
				      pitchbendSensitivity);
    else fr1 = 0;
    MKUpdateAsymp(UG(incUG),freqEnv,
		  [UG(oscUG1) incAtFreq:fr0], [UG(oscUG1) incAtFreq:fr1],
		  freqAtt,freqRel,portamento,phraseStatus);
  }
  if ((updateEnvPar(aNote,MK_waveformEnv,waveformEnv) |
       updateDoublePar(aNote,MK_waveformAtt,waveformAtt) |
       updateDoublePar(aNote,MK_waveformRel,waveformRel)) || newNote)
    MKUpdateAsymp(UG(intUG),waveformEnv,0,.999999,
		  waveformAtt,waveformRel,portamento,phraseStatus);
  if (NUM(svibUG)>=0) {
    BOOL modChange = ((controller==MIDI_MODWHEEL) && 
		      updateIntPar(aNote,MK_controlVal,modwheel));
    double modPc = (double)modwheel/127.0;
    id vibWave = MKGetNoteParAsWaveTable(aNote,MK_vibWaveform);
    if (vibWave)
      [UG(svibUG) setTable:vibWave length:128 defaultToSineROM:YES];
    else if (newPhrase)
      [UG(svibUG) setTableToSineROM];
    if ((updateDoublePar(aNote,MK_svibFreq0,svibFreq0) |
	 updateDoublePar(aNote,MK_svibFreq1,svibFreq1) | modChange) || newPhrase)
      [UG(svibUG) setFreq:svibFreq0+(svibFreq1-svibFreq0)*modPc];
    if ((updateDoublePar(aNote,MK_svibAmp0,svibAmp0) |
	 updateDoublePar(aNote,MK_svibAmp1,svibAmp1) | modChange) || newPhrase)
      [UG(svibUG) setAmp:svibAmp0+(svibAmp1-svibAmp0)*modPc];
  }
  if (NUM(nvibUG)>=0) {
    if (updateDoublePar(aNote,MK_rvibAmp,rvibAmp) || newPhrase) {
      [UG(onepUG) setB0:.004*rvibAmp];
      [UG(onepUG) clear];
      if (newPhrase) {
	[UG(onepUG) setA1:-.9999];
	[UG(nvibUG) anySeed];
      }
    }
  }
  if ((updateEnvPar(aNote,MK_ampEnv,ampEnv) |
       updateDoublePar(aNote,MK_amp0,amp0) | 
       updateDoublePar(aNote,MK_amp1,amp1) |
       updateDoublePar(aNote,MK_velocity,velocity) |
       updateDoublePar(aNote,MK_velocitySensitivity,velocitySensitivity) |
       updateDoublePar(aNote,MK_ampAtt,ampAtt) |
       updateDoublePar(aNote,MK_ampRel,ampRel)) || newNote) {
    double amp = amp1 * MKMidiToAmpWithSensitivity(velocity,velocitySensitivity);
    MKUpdateAsymp(UG(ampUG),ampEnv,amp0,amp,ampAtt,ampRel,portamento,phraseStatus);
  }
  return self;
}    


-noteOnSelf:aNote
  /* Sent whenever a noteOn is received by the SynthInstrument. */
{
  /* Interpret all the parameters and send the necessary updates. */
  [self _updateParameters:aNote];
  /* Connect the ouput last. */
  [UG(outUG) setInput:UG(xsig)];
  /* Now tell everyone to run. */
  [synthElements makeObjectsPerform:@selector(run)];
  return self;
}


-preemptFor:aNote
  /* Sent whenever a running note is being preempted by a new note. */
{
  [UG(ampUG) preemptEnvelope];
  [self _setDefaults];
  return self;
}


-noteUpdateSelf:aNote
  /* Sent whenever a noteUpdate is received by the SynthInstrument. */
{
    return [self _updateParameters:aNote];
}


-(double)noteOffSelf:aNote
  /* Sent whenever a noteOff is received by the SynthInstrument. Returns
     the amplitude envelope completion time, needed to schedule the noteEnd. */
{   
    [self _updateParameters:aNote];
    /* The finish method returns the time it will take the envelope to complete,
       i.e., the time after the stickpoint if there is one, otherwise the
       remaining overall envelope time. */
    [UG(incUG) finish];
    [UG(intUG) finish];
    return [UG(ampUG) finish];
}

-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    [UG(outUG) idle]; 
    [UG(incUG) abortEnvelope];
    [UG(intUG) abortEnvelope];
    [self _setDefaults];
    return self;
}

@end
