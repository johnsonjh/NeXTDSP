#ifdef SHLIB
#include "shlib.h"
#endif

/* Modification history:

   08/28/90/daj - Changed initialize to init.
*/
#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <objc/HashTable.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Simp.h"

@implementation Simp:SynthPatch
  /* Simple wavetable with no envelopes. See comments in Simp.h */
{ 
  double amp, freq, bearing, phase, velocitySensitivity;
  id waveform;
  int wavelen, volume, velocity;
  int pitchbend;
  double pitchbendSensitivity;  
}

#import "_synthPatchInclude.h"
static int oscUG, outUG, outsig;

+patchTemplateFor:currentNote
    /* Creates and returns a patchTemplate which specifies the
       UnitGenerators and Patchpoints to be used and how they are
       to be interconnected when one of these synthPatches is
       instantiated.  Note that this method only creates the
       specification.  It does not actually instantiate anything.
       This SynthPatch has only one template, but could have variations
       which are returned according to the note parameter values.  */
{
    static id template = nil;
    if (template) return template;
    template = [PatchTemplate new];
    /* These ordered UnitGenerators will be instantiated. */
    oscUG = [template addUnitGenerator:[OscgUGyy class]];
    outUG = [template addUnitGenerator:[Out2sumUGy class]];
    /* These Patchpoints will be instantiated. */
    outsig = [template addPatchpoint:MK_yPatch];
    /* These methods will be sent to connect the UGs and Patchpoints. */
    [template to:oscUG sel:@selector(setOutput:)   arg:outsig];
    return template;
}


-controllerValues:controllers
{
  if ([controllers isKey:(const void *)MIDI_MAINVOLUME])
    volume = (int)[controllers valueForKey:(const void *)MIDI_MAINVOLUME];
  return self;
}


-_setDefaults
  /* Set the static global variables to reasonable values. */
{
    amp     = MK_DEFAULTAMP;
    freq    = MK_DEFAULTFREQ;
    bearing = MK_DEFAULTBEARING;
    phase   = 0.0;
    waveform = nil;
    wavelen = 0;
    volume  = 127;
    velocity = MK_DEFAULTVELOCITY;
    velocitySensitivity = 0.5;
    pitchbend = MIDI_ZEROBEND;
    pitchbendSensitivity = 3;
    return self;
  }


-init
  /* Sent by this class on object creation and reset. */
{
  [self _setDefaults];
  return self;
}


/* ampUG, etc., are really just indexes.  These macros return the objects. */
#define OSC_UG  NX_ADDRESS(synthElements)[oscUG]
#define OUT_UG  NX_ADDRESS(synthElements)[outUG]
#define OUT_SIG NX_ADDRESS(synthElements)[outsig]

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
  BOOL phraseOn = (phraseStatus == MK_phraseOn);
  int controller = intPar(aNote,MK_controlChange,0);
  if ((updateWavePar(aNote,MK_waveform,waveform) |
       updateIntPar(aNote,MK_waveLen,wavelen)) || newPhrase)
    [OSC_UG setTable:waveform length:wavelen defaultToSineROM:newPhrase];
  updateDoublePar(aNote,MK_phase,phase);
  if (phraseOn)
    [OSC_UG setPhase:phase];
  if ((updateDoublePar(aNote,MK_bearing,bearing) |
      ((controller==MIDI_MAINVOLUME) && updateIntPar(aNote,MK_controlVal,volume))) ||
      newPhrase)
    [OUT_UG setBearing:bearing scale:volumeToAmp(volume)];
  if ((updateDoublePar(aNote,MK_amp,amp) |
       updateDoublePar(aNote,MK_velocity,velocity) |
       updateDoublePar(aNote,MK_velocitySensitivity,velocitySensitivity)) || 
      newNote)
    [OSC_UG setAmp:amp*MKMidiToAmpWithSensitivity(velocity,velocitySensitivity)];
  updateIntPar(aNote,MK_pitchBendSensitivity,pitchbendSensitivity);
  if (updateFreq(aNote,freq) || 
      updateIntPar(aNote,MK_pitchBend,pitchbend) || newNote) {
    double fr = MKAdjustFreqWithPitchBend(freq,pitchbend,
					  pitchbendSensitivity);
    [OSC_UG setFreq:fr];
  }
  return self;
}    


-noteOnSelf:aNote
  /* Sent whenever a noteOn is received by the SynthInstrument. */
{
  [self _updateParameters:aNote];
  /* Connect the ouput last. */
  [OUT_UG setInput:OUT_SIG];
  /* Now tell everyone to run. */
  [synthElements makeObjectsPerform:@selector(run)];
  return self;
}


-preemptFor:aNote
  /* Sent whenever a running note is being preempted by a new note. */
{
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
    return 0.0;
}


-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    [OUT_UG idle];
    [self _setDefaults];
    return self;
}

@end
