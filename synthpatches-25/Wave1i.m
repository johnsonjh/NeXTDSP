#ifdef SHLIB
#include "shlib.h"
#endif

/* Modification history:
   aug 18,90 daj - Fixed a HORRIBLE bug having to do with templates!
               (In the private template-returning method, I was clobbering
	       Wave1vi's template.)
   08/28/90/daj - Changed initialize to init.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
*/
/* 
  Wave1i is the root of a family of SynthPatches: 

  Wave1i - interpolating osc
  Wave1vi - interpolating osc and vib
  Wave1v - vib no interpolating osc
  Wave1 - no interpolating osc
  DBWave1vi - interpolating osc and vib, wavetable database support

  Ideally, there would be an _Wave class that was the superclass of both
  Wave1 and Wave1i. But, to minimize the number of classes, we use the following
  hieararchy:

  SynthPatch-->Wave1i-->Wave1vi-->Wave1v
                     \         \
                      `->Wave1  `->DBWave1vi

  Wave1i contains code to handle vibrato parameters, even though it does not
  have instance variables for vibrato. Instead, it uses private methods
  to invoke the subclass to handle the vibrato.
*/

/* The patch of Wave1vi is  described below. The UnitGenerators are shown 
 * in parenthesis. The non-vib cases are minor variants of this.
 *
 * The vibrato (in Wave1vi and Wave1v only) 
 * is created by adding (Add2) a periodic component with a random
 * component. The periodic component is created by an oscillator (Oscg).
 * The random component is created by white noise (Snoise) that is
 * low-pass filtered (Onepole). 
 *
 * The vibrato signal is multiplied (Mul1add2) by the output of the frequency
 * envelope generator (Asymp) and the frequency envelope generator is 
 * additionally added. Thus, the vibrato depth is perceptually the same, 
 * even as the frequency is continuously changing. Let us call the resulting 
 * signal the "frequency signal".
 *
 * The carrier (Oscgafi) takes its amplitude envelope from the amplitude
 * envelope generator (Asymp) and sends its output signal to the stereo
 * output sample stream (Out2sum).
 *
 * The communication between UnitGenerators is accomplished by patchpoints.
 * Since the ordering of the UnitGenerators is constrained to be as specified
 * below, two patchpoints suffice for all UnitGenerator communication. 
 * That is, the two patchpoints are reused as temporary storage for each stage
 * in the sample computation.
 */   

/* Written by Mike McNabb and David Jaffe. */
  
#define MK_INLINE 1
#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import "Wave1i.h"
  
@implementation Wave1i

#import "_Wave1i.h"

WAVEDECL(template,ugs);

id _MKSPGetWaveAllVibTemplate(_MKSPWAVENums *ugs,id oscClass) {
  id aTemplate = [PatchTemplate new];
  /* these UnitGenerators will be instantiated. */
  ugs->svibUG  = [aTemplate addUnitGenerator:[OscgUGyy class]];
  ugs->nvibUG  = [aTemplate addUnitGenerator:[SnoiseUGx class]];
  ugs->onepUG  = [aTemplate addUnitGenerator:[OnepoleUGxx class]];
  ugs->addUG   = [aTemplate addUnitGenerator:[Add2UGyxy class]];
  ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->oscUG   = [aTemplate addUnitGenerator:oscClass]; /* xxyy */
  ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs->xsig = [aTemplate addPatchpoint:MK_xPatch];
  ugs->ysig = [aTemplate addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [aTemplate to:ugs->svibUG  sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->nvibUG  sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->onepUG  sel:@selector(setInput:)    arg:ugs->xsig];
  [aTemplate to:ugs->onepUG  sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->addUG   sel:@selector(setInput1:)   arg:ugs->xsig];
  [aTemplate to:ugs->addUG   sel:@selector(setInput2:)   arg:ugs->ysig];
  [aTemplate to:ugs->addUG   sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
  [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  return aTemplate;
  }

id _MKSPGetWaveRanVibTemplate(_MKSPWAVENums *ugs,id oscClass) {
  id aTemplate = [PatchTemplate new];
  /* these UnitGenerators will be instantiated. */
  ugs->nvibUG  = [aTemplate addUnitGenerator:[SnoiseUGx class]];
  ugs->onepUG  = [aTemplate addUnitGenerator:[OnepoleUGyx class]];
  ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->oscUG   = [aTemplate addUnitGenerator:oscClass]; /* xxyy */
  ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs->xsig = [aTemplate addPatchpoint:MK_xPatch];
  ugs->ysig = [aTemplate addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [aTemplate to:ugs->nvibUG sel:@selector(setOutput:)    arg:ugs->xsig];
  [aTemplate to:ugs->onepUG  sel:@selector(setInput:)    arg:ugs->xsig];
  [aTemplate to:ugs->onepUG  sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
  [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  return aTemplate;
  }

id _MKSPGetWaveSinVibTemplate(_MKSPWAVENums *ugs,id oscClass) {
  id aTemplate = [PatchTemplate new];
  /* these UnitGenerators will be instantiated. */
  ugs->svibUG  = [aTemplate addUnitGenerator:[OscgUGyy class]];
  ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
  ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->oscUG   = [aTemplate addUnitGenerator:oscClass]; /* xxyy */
  ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs->xsig = [aTemplate addPatchpoint:MK_xPatch];
  ugs->ysig = [aTemplate addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [aTemplate to:ugs->svibUG  sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
  [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
  [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
  [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  return aTemplate;
  }

id _MKSPGetWaveNoVibTemplate(_MKSPWAVENums *ugs,id oscClass) {
  id aTemplate = [PatchTemplate new];
  /* these UnitGenerators will be instantiated. */
  ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGy class]];
  ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
  ugs->oscUG   = [aTemplate addUnitGenerator:oscClass]; /* xxyy */
  ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
  /* These Patchpoints will be instantiated. */
  ugs->xsig = [aTemplate addPatchpoint:MK_xPatch];
  ugs->ysig = [aTemplate addPatchpoint:MK_yPatch];
  /* These methods will be sent to connect the UGs and Patchpoints. */
  [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->ysig];
  [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
  [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
  [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
  return aTemplate;
  }

+patchTemplateFor:aNote
{
    if (!template)
      template = _MKSPGetWaveNoVibTemplate(&ugs,[OscgafiUGxxyy class]);
    return template;
}

-init
  /* Sent by this class on object creation and reset. */
{
    [self _setDefaults];
    _ugNums = &ugs;
    return self;
}

-noteOnSelf:aNote
  /* Sent whenever a noteOn is received. First updates the parameters,
   * then connects the carrier to the output. 
   */
{
    [self _updateParameters:aNote];                 // Interpret parameters
    [WAVEUG(outUG) setInput:WAVEUG(xsig)];                  // Connect output last. 
    [synthElements makeObjectsPerform:@selector(run)]; // Make them all run
    return self;
}

-noteUpdateSelf:aNote
  /* Sent whenever a noteUpdate is received by the SynthInstrument. */
{
    return [self _updateParameters:aNote];
}

-(double)noteOffSelf:aNote
  /* Sent whenever a noteOff is received by the SynthInstrument. Returns
   * the amplitude envelope completion time, needed to schedule the noteEnd. */
{   
    [self _updateParameters:aNote];
    [WAVEUG(incUG) finish];
    /* The value returned by noteOffSelf: is the time for the release portion
     * to complete. We return the value returned by WAVEUG(ampUG)'s finish 
     * method,
     * i.e. the time in seconds it will take the envelope to 
     * complete. We only really care about the amplitude envelope's finishing
     * (because once it is finished, there is no more sound) so we use its 
     * time. This assumes that the amplitude envelope ends at 0.0. */
    return [WAVEUG(ampUG) finish];
}

-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    [WAVEUG(outUG) idle];
    /* Since we only used the WAVEUG(ampUG)'s finish time above, the other 
     * envelopes may or may not be finished, so we explicitly abort them. */
    [WAVEUG(incUG) abortEnvelope];
    [self _setDefaults];
    return self;
}

-preemptFor:aNote
  /* Sent whenever a running note is being preempted by a new note. */
{
    /* Cause envelope to go quickly to last value.  This is to prevent a
     * click between notes when preempting. (This assumes the amplitude 
     * envelope ends at 0). */
    [WAVEUG(ampUG) preemptEnvelope]; 
    [self _setDefaults]; /* Reset parameters to defaults. */
    return self;
}

#import <objc/HashTable.h>

-controllerValues:controllers
  /* Sent when a new phrase starts. controllers is a HashTable containing
   * key/value pairs as controller-number/controller-value. Our implementation
   * here ignores all but MIDI_MAINVOLUME and MIDI_MODWHEEL. See 
   * <objc/HashTable.h>, <midi/midi_types.h>, and <musickit/SynthPatch.h>. */
{
#   define CONTROLPRESENT(_key) [controllers isKey:(const void *)_key]
#   define GETVALUE(_key) (int)[controllers valueForKey:(const void *)_key]
    if (CONTROLPRESENT(MIDI_MAINVOLUME))
      volume = GETVALUE(MIDI_MAINVOLUME);
    if (CONTROLPRESENT(MIDI_MODWHEEL))
      [self _setModWheel:GETVALUE(MIDI_MODWHEEL)];
    return self;
}

@end



@implementation Wave1i(Private)

-(void)_setModWheel:(int)val
{
}

-(void)_setSvibFreq0:(double)val
{
}

-(void)_setSvibFreq1:(double)val
{
}

-(void)_setSvibAmp0:(double)val
{
}

-(void)_setSvibAmp1:(double)val
{
}

-(void)_setRvibAmp:(double)val
{
}

-(void)_setVibWaveform:(id)obj
{
}

-(void)_setVib:(BOOL)setVibWaveform :(BOOL)setVibFreq :(BOOL)setVibAmp 
  :(BOOL)setRandomVib :(BOOL)newPhrase
{
}

-_updateParameters:aNote
  /* Updates the SynthPatch according to the information in the note and
   * the note's relationship to a possible ongoing phrase. 
   */
{
    BOOL newPhrase, setWaveform, setOutput,
         setRandomVib, setVibWaveform, setVibFreq,
         setVibAmp, setPhase, setAmpEnv, setFreqEnv;
    void *state; /* For parameter iteration below */
    int par;     
    MKPhraseStatus phraseStatus = [self phraseStatus];

    /* Initialize booleans based on phrase status -------------------------- */
    switch (phraseStatus) {
      case MK_phraseOn:          /* New phrase. */
      case MK_phraseOnPreempt:   /* New phrase but using preempted patch. */
        newPhrase = setWaveform =  setOutput = setRandomVib =  
	  setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	    setFreqEnv = YES;  /* Set everything for new phrase */
	break;
      case MK_phraseRearticulate: /* NoteOn rearticulation within phrase. */
	newPhrase = setWaveform = setOutput = setRandomVib = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = NO;
	setAmpEnv = setFreqEnv = YES; /* Just restart envelopes */
	break;
      case MK_phraseUpdate:       /* NoteUpdate to running phrase. */
      case MK_phraseOff:          /* NoteOff to running phrase. */
      case MK_phraseOffUpdate:    /* NoteUpdate to finishing phrase. */
      default: 
	newPhrase = setWaveform = setOutput = setRandomVib = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	      setFreqEnv = NO;  /* Only set what's in Note */
	break;
      }

    /* Since this SynthPatch supports so many parameters, it would be 
     * inefficient to check each one with Note's isParPresent: method, as
     * we did in Simplicity and Envy. Instead, we iterate over the parameters 
     * in aNote. */

    state = MKInitParameterIteration(aNote);
    while (par = MKNextParameter(aNote, state))  
      switch (par) {          /* Parameters in (roughly) alphabetical order. */
	case MK_ampEnv:
	  ampEnv = MKGetNoteParAsEnvelope(aNote,MK_ampEnv);
	  setAmpEnv = YES;
	  break;
	case MK_ampAtt:
	  ampAtt = MKGetNoteParAsDouble(aNote,MK_ampAtt);
	  setAmpEnv = YES;
	  break;
	case MK_ampRel:
	  ampRel = MKGetNoteParAsDouble(aNote,MK_ampRel);
	  setAmpEnv = YES;
	  break;
	case MK_amp0:
	  amp0 = MKGetNoteParAsDouble(aNote,MK_amp0);
	  setAmpEnv = YES;
	  break;
	case MK_amp1: /* MK_amp is synonym */
	  amp1 = MKGetNoteParAsDouble(aNote,MK_amp1);
	  setAmpEnv = YES;
	  break;
	case MK_bearing:
	  bearing = MKGetNoteParAsDouble(aNote,MK_bearing);
	  setOutput = YES;
	  break;
	case MK_controlChange: {
	    int controller = MKGetNoteParAsInt(aNote,MK_controlChange);
	    if (controller == MIDI_MAINVOLUME) {
		volume = MKGetNoteParAsInt(aNote,MK_controlVal);
		setOutput = YES; 
	    } 
	    else if (controller == MIDI_MODWHEEL) {
		[self _setModWheel:MKGetNoteParAsInt(aNote,MK_controlVal)];
		setVibFreq = setVibAmp = YES;
	    }
	    break;
	}
	case MK_freqEnv:
	  freqEnv = MKGetNoteParAsEnvelope(aNote,MK_freqEnv);
	  setFreqEnv = YES;
	  break;
	case MK_freqAtt:
	  freqAtt = MKGetNoteParAsDouble(aNote,MK_freqAtt);
	  setFreqEnv = YES;
	  break;
	case MK_freqRel:
	  freqRel = MKGetNoteParAsDouble(aNote,MK_freqRel);
	  setFreqEnv = YES;
	  break;
	case MK_freq:
	case MK_keyNum:
	  freq1 = [aNote freq]; /* A special method (see <musickit/Note.h>) */
	  setFreqEnv = YES;
	  break;
	case MK_freq0:
	  freq0 = MKGetNoteParAsDouble(aNote,MK_freq0);
	  setFreqEnv = YES;
	  break;
	case MK_phase:
	  phase = MKGetNoteParAsDouble(aNote,MK_phase);
	  /* To avoid clicks, we don't allow phase to be set except at the 
	     start of a phrase. Therefore, we don't set setPhase. */
	  break;
	case MK_pitchBendSensitivity:
	  pitchbendSensitivity = 
	    MKGetNoteParAsDouble(aNote,MK_pitchBendSensitivity);
	  setFreqEnv = YES;
	  break;
	case MK_pitchBend:
	  pitchbend = MKGetNoteParAsInt(aNote,MK_pitchBend);
	  setFreqEnv = YES;
	  break;
	case MK_portamento:
	  portamento = MKGetNoteParAsDouble(aNote,MK_portamento);
	  setAmpEnv = YES;
	  break;
	case MK_rvibAmp:
	  [self _setRvibAmp:MKGetNoteParAsDouble(aNote,MK_rvibAmp)];
	  setRandomVib = YES;
	  break;
	case MK_svibFreq0:
	  [self _setSvibFreq0:MKGetNoteParAsDouble(aNote,MK_svibFreq0)];
	  setVibFreq = YES;
	  break;
	case MK_svibFreq1:
	  [self _setSvibFreq1:MKGetNoteParAsDouble(aNote,MK_svibFreq1)];
	  setVibFreq = YES;
	  break;
	case MK_svibAmp0:
	  [self _setSvibAmp0:MKGetNoteParAsDouble(aNote,MK_svibAmp0)];
	  setVibAmp = YES;
	  break;
	case MK_svibAmp1:
	  [self _setSvibAmp1:MKGetNoteParAsDouble(aNote,MK_svibAmp1)];
	  setVibAmp = YES;
	  break;
	case MK_vibWaveform:
	  [self _setVibWaveform:MKGetNoteParAsWaveTable(aNote,MK_vibWaveform)];
	  setVibWaveform = YES;
	  break;
	case MK_velocity:
	  velocity = MKGetNoteParAsDouble(aNote,MK_velocity);
	  setAmpEnv = YES;
	  break;
	case MK_velocitySensitivity:
	  velocitySensitivity = 
	    MKGetNoteParAsDouble(aNote,MK_velocitySensitivity);
	  setAmpEnv = YES;
	  break;
	case MK_waveform:
	  waveform = MKGetNoteParAsWaveTable(aNote,MK_waveform);
	  setWaveform = YES;
	  break;
  	case MK_waveLen:
	  wavelen = MKGetNoteParAsInt(aNote,MK_waveLen);
	  setWaveform = YES; 
	  break;
	default: /* Skip unrecognized parameters */
	  break;
      } /* End of parameter loop. */

	/* -------------------------------- Waveforms --------------------- */
    if (setWaveform)
      [WAVEUG(oscUG) setTable:waveform length:wavelen defaultToSineROM:newPhrase];

	/* ------------------------------- Phases -------------------------- */
    if (setPhase)
      [WAVEUG(oscUG) setPhase:phase];

    /* ------------------------------ Envelopes ------------------------ */
    if (setAmpEnv) 
      MKUpdateAsymp(WAVEUG(ampUG),ampEnv,amp0,
		    amp1 * 
		    MKMidiToAmpWithSensitivity(velocity,velocitySensitivity),
		    ampAtt,ampRel,portamento,phraseStatus);
    if (setFreqEnv) {
	double fr0, fr1;
	fr0 = MKAdjustFreqWithPitchBend(freq0,pitchbend,pitchbendSensitivity);
	fr1 = MKAdjustFreqWithPitchBend(freq1,pitchbend,pitchbendSensitivity);
	MKUpdateAsymp(WAVEUG(incUG),freqEnv,
		      [WAVEUG(oscUG) incAtFreq:fr0], /* Convert to osc increment */
		      [WAVEUG(oscUG) incAtFreq:fr1], 
		      freqAtt,freqRel,portamento,phraseStatus);
    }

    /* Vibrato is handled by subclass, if any. */
    [self _setVib:setVibWaveform :setVibFreq :setVibAmp :setRandomVib 
     :newPhrase];

    /* ------------------- Bearing, volume and after touch -------------- */
    if (setOutput)
      [WAVEUG(outUG) setBearing:bearing scale:MKMidiToAmpAttenuation(volume)];

    return self;
}    

-_setDefaults
  /* Set the instance variables to reasonable default values. We do this
   * after each phrase and upon initialization. This insures that a freshly 
   * allocated SynthPatch will be in a known state. See <musickit/params.h> 
   */
{
  waveform = nil;                   /* WaveTables */
  wavelen = 0;                      /* Wave table length */
  phase = 0.0;                      /* Waveform initial phases */
  ampEnv = freqEnv = nil;           /* Envelopes */
  freq0 = 0.0;                      /* Frequency values */
  freq1 = MK_DEFAULTFREQ;           /* 440 Hz. */
  amp0 = 0.0;                       /* Amplitude values */
  amp1 = MK_DEFAULTAMP;             /* 0.1 */
  ampAtt = freqAtt = MK_NODVAL;     /* Attack times (not set) */
  ampRel = freqRel = MK_NODVAL;     /* Release times (not set) */
  bearing = MK_DEFAULTBEARING;      /* 0.0 degrees */
  portamento = MK_NODVAL;           /* Rearticulation skew duration (not set) */
  /* MIDI parameters */
  velocitySensitivity = 0.5;
  velocity = MK_DEFAULTVELOCITY;    
  volume = MIDI_MAXDATA;
  pitchbend = MIDI_ZEROBEND;
  pitchbendSensitivity = 3.0;
  return self;
}

@end
