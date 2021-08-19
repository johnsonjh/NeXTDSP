#ifdef SHLIB
#include "shlib.h"
#endif

/* Modification history:

  08/28/90/daj - Changed initialize to init.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
*/
#define MK_INLINE 1
#import <musickit/musickit.h>
#import <libc.h>                   /* Has random() */
#import <midi/midi_types.h>
#import <objc/HashTable.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Fm2cnvi.h"

@implementation Fm2cnvi:SynthPatch
  /* FM with interpolating carrier, two cascaded modulators, and noise modulation
     on the cascade modulator, with optional sinusoidal and/or random vibrato.
     See comments in Fm2cnvi.h */
{
  double amp0, amp1, ampAtt, ampRel, freq0, freq1, freqAtt, freqRel,
         bearing, phase, portamento, svibAmp0, svibAmp1, rvibAmp,
         svibFreq0, svibFreq1, bright, cRatio,
         m1Ratio, m1Ind0, m1Ind1, m1IndAtt, m1IndRel, m1Phase,
         m2Ratio, m2Ind0, m2Ind1, m2IndAtt, m2IndRel, m2Phase,
         noise0, noise1, noiseAtt, noiseRel,
         velocitySensitivity, breathSensitivity,
         panSensitivity, afterTouchSensitivity, pitchbendSensitivity;
  id ampEnv, freqEnv, m1IndEnv, m2IndEnv, noiseEnv,
     waveform, m1Waveform, m2Waveform;
  int wavelen, volume, velocity, pan, modulation, breath, aftertouch, pitchbend;
  void *_ugNums;
}

#import "_synthPatchInclude.h"

struct ugNums {
  int svibUG, nvibUG, onepUG, addUG1, incUG, mulUG1, noiseUG, nAmpUG, mulUG2,
      addUG2, ind1UG, mod1UG, ind2UG, mod2UG, addUG3, addUG4, 
      ampUG, oscUG, outUG, xsig1, xsig2, ysig1;
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
  ugs.addUG1  = [template addUnitGenerator:[Add2UGxxy class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mulUG1  = [template addUnitGenerator:[Mul1add2UGxyyx class]];
  ugs.noiseUG = [template addUnitGenerator:[SnoiseUGy class]];
  ugs.nAmpUG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG2  = [template addUnitGenerator:[Mul2UGyxy class]];
  ugs.addUG2  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind2UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod2UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG3  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind1UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod1UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG4  = [template addUnitGenerator:[Add2UGyxy class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG   = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig1  = [template addPatchpoint:MK_xPatch];
  ugs.xsig2  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.svibUG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.nvibUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.onepUG  sel:@selector(setInput:)    arg:ugs.xsig1];
  [template to:ugs.onepUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.addUG1  sel:@selector(setInput1:)   arg:ugs.xsig1];
  [template to:ugs.addUG1  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG1  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput3:)   arg:ugs.xsig1];
  [template to:ugs.mulUG1  sel:@selector(setOutput:)   arg:ugs.xsig2];
  [template to:ugs.nAmpUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.noiseUG sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setInput1:)   arg:ugs.xsig1];
  [template to:ugs.mulUG2  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind2UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod2UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG3  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind1UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod1UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setInput1:)   arg:ugs.xsig2];
  [template to:ugs.addUG4  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  return template;
  }


static id getRanVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  ranVibTemplate = template;
  ranVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.nvibUG  = [template addUnitGenerator:[SnoiseUGx class]];
  ugs.onepUG  = [template addUnitGenerator:[OnepoleUGxx class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mulUG1  = [template addUnitGenerator:[Mul1add2UGxyyx class]];
  ugs.noiseUG = [template addUnitGenerator:[SnoiseUGy class]];
  ugs.nAmpUG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG2  = [template addUnitGenerator:[Mul2UGyxy class]];
  ugs.addUG2  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind2UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod2UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG3  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind1UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod1UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG4  = [template addUnitGenerator:[Add2UGyxy class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG   = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig1  = [template addPatchpoint:MK_xPatch];
  ugs.xsig2  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.nvibUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.onepUG  sel:@selector(setInput:)    arg:ugs.xsig1];
  [template to:ugs.onepUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput3:)   arg:ugs.xsig1];
  [template to:ugs.mulUG1  sel:@selector(setOutput:)   arg:ugs.xsig2];
  [template to:ugs.nAmpUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.noiseUG sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setInput1:)   arg:ugs.xsig1];
  [template to:ugs.mulUG2  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind2UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod2UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG3  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind1UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod1UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setInput1:)   arg:ugs.xsig2];
  [template to:ugs.addUG4  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  return template;
  }


static id getSinVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  sinVibTemplate = template;
  sinVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.svibUG  = [template addUnitGenerator:[OscgUGxy class]];
  ugs.incUG   = [template addUnitGenerator:[AsympUGy class]];
  ugs.mulUG1  = [template addUnitGenerator:[Mul1add2UGxyyx class]];
  ugs.noiseUG = [template addUnitGenerator:[SnoiseUGy class]];
  ugs.nAmpUG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG2  = [template addUnitGenerator:[Mul2UGyxy class]];
  ugs.addUG2  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind2UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod2UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG3  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind1UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod1UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG4  = [template addUnitGenerator:[Add2UGyxy class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG   = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig1  = [template addPatchpoint:MK_xPatch];
  ugs.xsig2  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.svibUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG1  sel:@selector(setInput3:)   arg:ugs.xsig1];
  [template to:ugs.mulUG1  sel:@selector(setOutput:)   arg:ugs.xsig2];
  [template to:ugs.nAmpUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.noiseUG sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setInput1:)   arg:ugs.xsig1];
  [template to:ugs.mulUG2  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind2UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod2UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG3  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind1UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod1UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setInput1:)   arg:ugs.xsig2];
  [template to:ugs.addUG4  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  return template;
  }


static id getNoVibTemplate() {
  static id template = nil;
  static struct ugNums ugs  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  if (template) return template;
  template = [PatchTemplate new];
  noVibTemplate = template;
  noVibUgs = &ugs;
  /* these UnitGenerators will be instantiated. */
  ugs.incUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.noiseUG = [template addUnitGenerator:[SnoiseUGy class]];
  ugs.nAmpUG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mulUG2  = [template addUnitGenerator:[Mul2UGyxy class]];
  ugs.addUG2  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind2UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod2UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG3  = [template addUnitGenerator:[Scl1add2UGyyx class]];
  ugs.ind1UG  = [template addUnitGenerator:[AsympUGx class]];
  ugs.mod1UG  = [template addUnitGenerator:[OscgafUGyxyy class]];
  ugs.addUG4  = [template addUnitGenerator:[Add2UGyxy class]];
  ugs.ampUG   = [template addUnitGenerator:[AsympUGx class]];
  ugs.oscUG   = [template addUnitGenerator:[OscgafiUGxxyy class]];
  ugs.outUG   = [template addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs.xsig1  = [template addPatchpoint:MK_xPatch];
  ugs.xsig2  = [template addPatchpoint:MK_xPatch];
  ugs.ysig1 = [template addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [template to:ugs.incUG   sel:@selector(setOutput:)   arg:ugs.xsig2];
  [template to:ugs.nAmpUG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.noiseUG sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setInput1:)   arg:ugs.xsig1];
  [template to:ugs.mulUG2  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.mulUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG2  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG2  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind2UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod2UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod2UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput1:)   arg:ugs.ysig1];
  [template to:ugs.addUG3  sel:@selector(setInput2:)   arg:ugs.xsig2];
  [template to:ugs.addUG3  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ind1UG  sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.mod1UG  sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.mod1UG  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setInput1:)   arg:ugs.xsig2];
  [template to:ugs.addUG4  sel:@selector(setInput2:)   arg:ugs.ysig1];
  [template to:ugs.addUG4  sel:@selector(setOutput:)   arg:ugs.ysig1];
  [template to:ugs.ampUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setAmpInput:) arg:ugs.xsig1];
  [template to:ugs.oscUG   sel:@selector(setIncInput:) arg:ugs.ysig1];
  [template to:ugs.oscUG   sel:@selector(setOutput:)   arg:ugs.xsig1];
  return template;
  }

+patchTemplateFor:aNote
{
  if (aNote) {
    double svibpc = doublePar(aNote, MK_svibAmp,.01);
    double rvibpc = doublePar(aNote, MK_rvibAmp,.01);
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
    modulation = (int)[controllers valueForKey:(const void *)MIDI_MODWHEEL];
  if ([controllers isKey:(const void *)MIDI_BREATH])
    breath = (int)[controllers valueForKey:(const void *)MIDI_BREATH];
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
    waveform = nil;
    ampEnv   = nil;
    freqEnv  = nil;
    svibAmp0  = 0.0;
    svibAmp1  = 0.0;
    svibFreq0 = 0.0;
    svibFreq1 = 0.0;
    rvibAmp   = 0.0;
    modulation = 127;
    bright   = 1.0;
    cRatio   = MK_DEFAULTCRATIO;
    m1Ratio  = MK_DEFAULTMRATIO;
    m1Ind0   = 0.0;
    m1Ind1   = MK_DEFAULTINDEX;
    m1IndAtt = MK_NODVAL;
    m1IndRel = MK_NODVAL;
    m1Phase  = 0.0;
    m1IndEnv = nil;
    m1Waveform = nil;
    m2Ratio  = 2;
    m2Ind0   = 0.0;
    m2Ind1   = MK_DEFAULTINDEX;
    m2IndAtt = MK_NODVAL;
    m2IndRel = MK_NODVAL;
    m2Phase  = 0.0;
    m2IndEnv = nil;
    m2Waveform = nil;
    noise0   = 0.0;
    noise1   = .007; 
    noiseEnv = nil;
    noiseAtt = MK_NODVAL;
    noiseRel = MK_NODVAL;
    breath    = 127;
    aftertouch = 127;
    velocity = MK_DEFAULTVELOCITY;
    pan       = MAXINT;
    velocitySensitivity = 0.5;
    panSensitivity = 1.0;
    breathSensitivity = 0.5;
    afterTouchSensitivity = 0.5;
    pitchbend = MIDI_ZEROBEND;
    pitchbendSensitivity = 3;
    return self;
 }


# define NUM(ugname) (((struct ugNums *)(self->_ugNums))->ugname)
# define UG(ugname) NX_ADDRESS(synthElements)[NUM(ugname)]

-init
{
  [self _setDefaults]; /* Don't need [super init] here */
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
  BOOL newWavelen = updateIntPar(aNote,MK_waveLen,wavelen);
  BOOL phraseOn = (phraseStatus == MK_phraseOn);
  int controller = intPar(aNote,MK_controlChange,0);
  BOOL newFreq = updateDoublePar(aNote,MK_freq0,freq0) |
                 updateFreq(aNote,freq1);
  double fr0, fr1;
  if ((updateWavePar(aNote,MK_waveform,waveform) | newWavelen) || newPhrase)
    [UG(oscUG) setTable:waveform length:wavelen defaultToSineROM:newPhrase];
  updateDoublePar(aNote,MK_phase,phase);
  if (phraseOn)
    [UG(oscUG) setPhase:phase];
  if ((updateDoublePar(aNote,MK_bearing,bearing) |
       ((controller==MIDI_PAN) && updateIntPar(aNote,MK_controlVal,pan)) |
       updateDoublePar(aNote,MK_panSensitivity,panSensitivity) |
       ((controller==MIDI_MAINVOLUME) && updateIntPar(aNote,MK_controlVal,volume))) ||
      newPhrase) {
    if (iValid(pan)) bearing = panSensitivity * 45.0 * (double)(pan-64)/64.0;
    [UG(outUG) setBearing:bearing scale:volumeToAmp(volume)];
  }
  updateDoublePar(aNote,MK_portamento,portamento);
  if (newNote) {
    updateDoublePar(aNote,MK_velocity,velocity);
    updateDoublePar(aNote,MK_velocitySensitivity,velocitySensitivity);
  }
  if ((updateEnvPar(aNote,MK_ampEnv,ampEnv) |
       updateDoublePar(aNote,MK_amp0,amp0) | 
       updateDoublePar(aNote,MK_amp1,amp1) |
       updateDoublePar(aNote,MK_ampAtt,ampAtt) |
       updateDoublePar(aNote,MK_ampRel,ampRel)) || newNote) {
    double amp = amp1 * MKMidiToAmpWithSensitivity(velocity,velocitySensitivity);
    MKUpdateAsymp(UG(ampUG),ampEnv,amp0,amp,ampAtt,ampRel,portamento,phraseStatus);
  }
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
		  [UG(oscUG) incAtFreq:fr0], [UG(oscUG) incAtFreq:fr1],
		  freqAtt,freqRel,portamento,phraseStatus);
  }
  if (NUM(svibUG)>=0) {
    BOOL modChange = ((controller==MIDI_MODWHEEL) && 
		      updateIntPar(aNote,MK_controlVal,modulation));
    double modPc = (double)modulation/127.0;
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
  if (updateDoublePar(aNote,MK_cRatio,cRatio) || newPhrase)
    [UG(oscUG) setIncRatio:cRatio];
  if ((updateWavePar(aNote,MK_m1Waveform,m1Waveform) | newWavelen) || newPhrase)
    [UG(mod1UG) setTable:m1Waveform length:wavelen defaultToSineROM:newPhrase];
  updateDoublePar(aNote,MK_m1Phase,m1Phase);
  if (phraseOn)
    [UG(mod1UG) setPhase:m1Phase];
  if ((updateDoublePar(aNote,MK_m1Ratio,m1Ratio) |
       newWavelen) || newPhrase)
    /* must account here for possible difference in table lengths */
    [UG(mod1UG) setIncRatio:(m1Ratio*[UG(mod1UG) 
				      tableLength]/[UG(oscUG) tableLength])];
  if ((updateEnvPar(aNote,MK_m1IndEnv,m1IndEnv) |
       updateDoublePar(aNote,MK_m1Ind0,m1Ind0) | 
       updateDoublePar(aNote,MK_m1Ind1,m1Ind1) |
       updateDoublePar(aNote,MK_m1IndAtt,m1IndAtt) |
       updateDoublePar(aNote,MK_m1IndRel,m1IndRel) |
       updateDoublePar(aNote,MK_bright,bright)) || newNote) {
    double deviation = [UG(oscUG) incAtFreq: (m1Ratio * freq1)] * bright *
                       ((1.0-velocitySensitivity) + 
                        velocitySensitivity * 
			(double)velocity/MK_DEFAULTVELOCITY);
    MKUpdateAsymp(UG(ind1UG), m1IndEnv, m1Ind0*deviation, m1Ind1*deviation,
		  m1IndAtt,m1IndRel,portamento,phraseStatus);
  }
  if ((updateWavePar(aNote,MK_m2Waveform,m2Waveform) | newWavelen) || newPhrase)
    [UG(mod2UG) setTable:m2Waveform length:wavelen defaultToSineROM:newPhrase];
  updateDoublePar(aNote,MK_m2Phase,m2Phase);
  if (phraseOn)
    [UG(mod2UG) setPhase:m2Phase];
  if ((updateDoublePar(aNote,MK_m2Ratio,m2Ratio) | newWavelen) || newPhrase)
    /* must account here for possible difference in table lengths */
    [UG(mod2UG) setIncRatio:(m2Ratio*[UG(mod2UG) 
				      tableLength]/[UG(oscUG) tableLength])];
  if ((updateEnvPar(aNote,MK_m2IndEnv,m2IndEnv) |
       updateDoublePar(aNote,MK_m2Ind0,m2Ind0) | 
       updateDoublePar(aNote,MK_m2Ind1,m2Ind1) |
       updateDoublePar(aNote,MK_m2IndAtt,m2IndAtt) |
       updateDoublePar(aNote,MK_m2IndRel,m2IndRel) |
       updateDoublePar(aNote,MK_bright,bright)) || newNote) {
    double deviation = [UG(oscUG) incAtFreq: (m2Ratio * freq1)] * bright;
    MKUpdateAsymp(UG(ind2UG), m2IndEnv, m2Ind0*deviation, m2Ind1*deviation,
		  m2IndAtt,m2IndRel,portamento,phraseStatus);
  }
  if ((updateEnvPar(aNote,MK_noiseAmpEnv,noiseEnv) |
       updateDoublePar(aNote,MK_noiseAmp0,noise0) | 
       updateDoublePar(aNote,MK_noiseAmp1,noise1) |
       updateDoublePar(aNote,MK_noiseAmpAtt,noiseAtt) |
       updateDoublePar(aNote,MK_noiseAmpRel,noiseRel)) || newNote) {
    if (newPhrase) 
      [UG(noiseUG) setSeed:random()&0xFFFFFF];
    MKUpdateAsymp(UG(nAmpUG),noiseEnv,noise0,noise1,noiseAtt,noiseRel,
		  portamento,phraseStatus);
  }
  if ((((controller==MIDI_BREATH) && updateIntPar(aNote,MK_controlVal,breath)) |
       updateDoublePar(aNote,MK_breathSensitivity,breathSensitivity)) || newPhrase) {
    [UG(addUG2) setScale:(1.0-breathSensitivity) + 
                         breathSensitivity * (double)breath/127.0];
  }
  if ((updateIntPar(aNote,MK_afterTouch,aftertouch) |
       updateDoublePar(aNote,MK_afterTouchSensitivity,afterTouchSensitivity)) || 
      newPhrase)
    [UG(addUG3) setScale:(1.0-afterTouchSensitivity) +
                         afterTouchSensitivity * (double)aftertouch/127.0];
  return self;
}    


-noteOnSelf:aNote
  /* Sent whenever a noteOn is received by the SynthInstrument. */
{
  /* Interpret all the parameters and send the necessary updates. */
  [self _updateParameters:aNote];
  /* Connect the ouput last. */
  [UG(outUG) setInput:UG(xsig1)];
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
    [UG(ind1UG) finish];
    [UG(ind2UG) finish];
    [UG(nAmpUG) finish];
    return [UG(ampUG) finish];
}


-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    [UG(outUG) idle]; 
    [UG(incUG) abortEnvelope];
    [UG(ind1UG) abortEnvelope];
    [UG(ind2UG) abortEnvelope];
    [UG(nAmpUG) abortEnvelope];
    [self _setDefaults];
    return self;
}

@end
