#ifdef SHLIB
#include "shlib.h"
#endif
/* Modification history:
   aug 18,90 daj - Renamed local template to aTemplate to avoid confusion. 

   08/28/90/daj - Changed initialize to init.

*/

/* 
  Fm1i is the root of a family of SynthPatches: 

  Fm1i - interpolating osc
  Fm1vi - interpolating osc and vib
  Fm1v - vib no interpolating osc
  Fm1 - no interpolating osc

  Ideally, there would be an _Fm class that was the superclass of both
  Fm1 and Fm1i. But, to minimize the number of classes, we use the following
  hieararchy:

  SynthPatch->Fm1i->Fm1vi->Fm1v

  and  

  SynthPatch->Fm1i->Fm1

  Fm1i contains code to handle vibrato parameters, even though it does not
  have instance variables for vibrato. Instead, it uses private methods
  to invoke the subclass to handle the vibrato.

  Fm1i is a frequency modulation SynthPatch with arbitrary waveforms for  
  carrier and modulator and an interpolating oscillator for the carrier. 
  It supports a wide variety of parameters, including many MIDI parameters. 
  See the FM literature for details of FM synthesis.                       
  (Note that the implementation here is "frequency modulation" rather than 
  "phase modulation" and that the deviation scaling does not follow the    
  frequency envelope -- it is exactly as defined in the literature only    
  when the frequency envelope is at 1 and the vibrato is neither above nor 
  below the pitch.)  */

/* The patch of Fm1vi is  described below. The UnitGenerators are shown 
 * in parenthesis. The non-vib cases are minor variants of this.
 *
 * The vibrato (in Fm1vi and Fm1v only) 
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
 * The modulator (Oscgaf) takes its frequency from the frequency signal and 
 * its FM index from the index envelope generator (Asymp). The output of
 * the modulator is added (Scl1add2) to the frequency signal to produce the 
 * frequency input to the carrier. 
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
#import "Fm1i.h"
  
@implementation Fm1i

#import "_Fm1i.h"

FMDECL(template,ugs);

id _MKSPGetFmNoVibTemplate(_MKSPFMNums *ugs,id oscClass) {
    id aTemplate = [PatchTemplate new];
    /* these UnitGenerators will be instantiated. */
    ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGy class]];
    ugs->indUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->modUG   = [aTemplate addUnitGenerator:[OscgafUGxxyy class]];
    ugs->fmAddUG  = [aTemplate addUnitGenerator:[Scl1add2UGyxy class]];
    ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->oscUG   = [aTemplate addUnitGenerator:oscClass];
    ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
    /* These Patchpoints will be instantiated. */
    ugs->xsig   = [aTemplate addPatchpoint:MK_xPatch];
    ugs->ysig   = [aTemplate addPatchpoint:MK_yPatch];
    /* These methods will be sent to connect the UGs and Patchpoints. */
    [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->indUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->modUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    return aTemplate;
}

id _MKSPGetFmAllVibTemplate(_MKSPFMNums *ugs,id oscClass) {
    id aTemplate = [PatchTemplate new];
    /* these UnitGenerators will be instantiated. */
    ugs->svibUG  = [aTemplate addUnitGenerator:[OscgUGyy class]];
    ugs->nvibUG  = [aTemplate addUnitGenerator:[SnoiseUGx class]];
    ugs->onepUG  = [aTemplate addUnitGenerator:[OnepoleUGxx class]];
    ugs->vibAddUG  = [aTemplate addUnitGenerator:[Add2UGyxy class]];
    ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
    ugs->indUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->modUG   = [aTemplate addUnitGenerator:[OscgafUGxxyy class]];
    ugs->fmAddUG  = [aTemplate addUnitGenerator:[Scl1add2UGyxy class]];
    ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->oscUG   = [aTemplate addUnitGenerator:oscClass];
    ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
    /* These Patchpoints will be instantiated. */
    ugs->xsig   = [aTemplate addPatchpoint:MK_xPatch];
    ugs->ysig   = [aTemplate addPatchpoint:MK_yPatch];
    /* These methods will be sent to connect the UGs and Patchpoints. */
    [aTemplate to:ugs->svibUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->nvibUG sel:@selector(setOutput:)    arg:ugs->xsig];
    [aTemplate to:ugs->onepUG  sel:@selector(setInput:)    arg:ugs->xsig];
    [aTemplate to:ugs->onepUG  sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->vibAddUG  sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->vibAddUG  sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->vibAddUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->indUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->modUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    return aTemplate;
}

id _MKSPGetFmRanVibTemplate(_MKSPFMNums *ugs,id oscClass) {
    id aTemplate = [PatchTemplate new];
    /* these UnitGenerators will be instantiated. */
    ugs->nvibUG  = [aTemplate addUnitGenerator:[SnoiseUGx class]];
    ugs->onepUG  = [aTemplate addUnitGenerator:[OnepoleUGyx class]];
    ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
    ugs->indUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->modUG   = [aTemplate addUnitGenerator:[OscgafUGxxyy class]];
    ugs->fmAddUG  = [aTemplate addUnitGenerator:[Scl1add2UGyxy class]];
    ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->oscUG   = [aTemplate addUnitGenerator:oscClass];
    ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
    /* These Patchpoints will be instantiated. */
    ugs->xsig   = [aTemplate addPatchpoint:MK_xPatch];
    ugs->ysig   = [aTemplate addPatchpoint:MK_yPatch];
    /* These methods will be sent to connect the UGs and Patchpoints. */
    [aTemplate to:ugs->nvibUG  sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->onepUG  sel:@selector(setInput:)    arg:ugs->xsig];
    [aTemplate to:ugs->onepUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->indUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->modUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    return aTemplate;
}

id _MKSPGetFmSinVibTemplate(_MKSPFMNums *ugs,id oscClass) {
    id aTemplate = [PatchTemplate new];
    /* these UnitGenerators will be instantiated. */
    ugs->svibUG  = [aTemplate addUnitGenerator:[OscgUGyy class]];
    ugs->incUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->mulUG   = [aTemplate addUnitGenerator:[Mul1add2UGyxyx class]];
    ugs->indUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->modUG   = [aTemplate addUnitGenerator:[OscgafUGxxyy class]];
    ugs->fmAddUG  = [aTemplate addUnitGenerator:[Scl1add2UGyxy class]];
    ugs->ampUG   = [aTemplate addUnitGenerator:[AsympUGx class]];
    ugs->oscUG   = [aTemplate addUnitGenerator:oscClass];
    ugs->outUG   = [aTemplate addUnitGenerator:[Out2sumUGx class]];
    /* These Patchpoints will be instantiated. */
    ugs->xsig   = [aTemplate addPatchpoint:MK_xPatch];
    ugs->ysig   = [aTemplate addPatchpoint:MK_yPatch];
    /* These methods will be sent to connect the UGs and Patchpoints. */
    [aTemplate to:ugs->svibUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->incUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->mulUG   sel:@selector(setInput3:)   arg:ugs->xsig];
    [aTemplate to:ugs->mulUG   sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->indUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->modUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->modUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput1:)   arg:ugs->xsig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setInput2:)   arg:ugs->ysig];
    [aTemplate to:ugs->fmAddUG  sel:@selector(setOutput:)   arg:ugs->ysig];
    [aTemplate to:ugs->ampUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setAmpInput:) arg:ugs->xsig];
    [aTemplate to:ugs->oscUG   sel:@selector(setIncInput:) arg:ugs->ysig];
    [aTemplate to:ugs->oscUG   sel:@selector(setOutput:)   arg:ugs->xsig];
    return aTemplate;
}

+patchTemplateFor:aNote
{
    if (!template)
      template = _MKSPGetFmNoVibTemplate(&ugs,[OscgafiUGxxyy class]);
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
    [FMUG(outUG) setInput:FMUG(xsig)];                  // Connect output last. 
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
    [FMUG(incUG) finish];
    [FMUG(indUG) finish];
    /* The value returned by noteOffSelf: is the time for the release portion
     * to complete. We return the value returned by FMUG(ampUG)'s finish 
     * method,
     * i.e. the time in seconds it will take the envelope to 
     * complete. We only really care about the amplitude envelope's finishing
     * (because once it is finished, there is no more sound) so we use its 
     * time. This assumes that the amplitude envelope ends at 0.0. */
    return [FMUG(ampUG) finish];
}

-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    [FMUG(outUG) idle];
    /* Since we only used the FMUG(ampUG)'s finish time above, the other 
     * envelopes may or may not be finished, so we explicitly abort them. */
    [FMUG(incUG) abortEnvelope];
    [FMUG(indUG) abortEnvelope];
    [self _setDefaults];
    return self;
}

-preemptFor:aNote
  /* Sent whenever a running note is being preempted by a new note. */
{
    /* Cause envelope to go quickly to last value.  This is to prevent a
     * click between notes when preempting. (This assumes the amplitude 
     * envelope ends at 0). */
    [FMUG(ampUG) preemptEnvelope]; 
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



@implementation Fm1i(Private)

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
    BOOL newPhrase, setWaveform, setM1Waveform, setM1Ratio, setOutput,
         setRandomVib, setCRatio, setAfterTouch, setVibWaveform, setVibFreq,
         setVibAmp, setPhase, setAmpEnv, setFreqEnv, setM1IndEnv;
    void *state; // For parameter iteration below
    int par;     
    MKPhraseStatus phraseStatus = [self phraseStatus];

    /* Initialize booleans based on phrase status -------------------------- */
    switch (phraseStatus) {
      case MK_phraseOn:          /* New phrase. */
      case MK_phraseOnPreempt:   /* New phrase but using preempted patch. */
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	      setFreqEnv = setM1IndEnv = YES;  // Set everything for new phrase
	break;
      case MK_phraseRearticulate: /* NoteOn rearticulation within phrase. */
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = NO;
	setAmpEnv = setFreqEnv = setM1IndEnv = YES; // Just restart envelopes 
	break;
      case MK_phraseUpdate:       /* NoteUpdate to running phrase. */
      case MK_phraseOff:          /* NoteOff to running phrase. */
      case MK_phraseOffUpdate:    /* NoteUpdate to finishing phrase. */
      default: 
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	      setFreqEnv = setM1IndEnv = NO;  // Only set what's in Note
	break;
    }

    /* Since this SynthPatch supports so many parameters, it would be 
     * inefficient to check each one with Note's isParPresent: method, as
     * we did in Simplicity and Envy. Instead, we iterate over the parameters 
     * in aNote. */

    state = MKInitParameterIteration(aNote);
    while (par = MKNextParameter(aNote, state))  
      switch (par) {          /* Parameters in (roughly) alphabetical order. */
	case MK_afterTouch:
	  afterTouch = MKGetNoteParAsInt(aNote,MK_afterTouch);
	  setAfterTouch = YES;
	  break;
	case MK_afterTouchSensitivity:
	  afterTouchSensitivity = 
            MKGetNoteParAsDouble(aNote,MK_afterTouchSensitivity);
	  setAfterTouch = YES;
          break;
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
	case MK_amp1: // MK_amp is synonym
	  amp1 = MKGetNoteParAsDouble(aNote,MK_amp1);
	  setAmpEnv = YES;
	  break;
	case MK_bearing:
	  bearing = MKGetNoteParAsDouble(aNote,MK_bearing);
	  setOutput = YES;
	  break;
	case MK_bright:
	  bright = MKGetNoteParAsDouble(aNote,MK_bright);
	  setM1IndEnv = YES;
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
	case MK_cRatio:
	  cRatio = MKGetNoteParAsDouble(aNote,MK_cRatio);
	  setCRatio = YES;
	  break;
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
	  freq1 = [aNote freq]; // A special method (see <musickit/Note.h>)
	  setFreqEnv = YES;
	  break;
	case MK_freq0:
	  freq0 = MKGetNoteParAsDouble(aNote,MK_freq0);
	  setFreqEnv = YES;
	  break;
	case MK_m1IndEnv:
	  m1IndEnv = MKGetNoteParAsEnvelope(aNote,MK_m1IndEnv);
	  setM1IndEnv = YES;
	  break;
	case MK_m1IndAtt:
	  m1IndAtt = MKGetNoteParAsDouble(aNote,MK_m1IndAtt);
	  setM1IndEnv = YES;
	  break;
	case MK_m1IndRel:
	  m1IndRel = MKGetNoteParAsDouble(aNote,MK_m1IndRel);
	  setM1IndEnv = YES;
	  break;
	case MK_m1Ind0:
	  m1Ind0 = MKGetNoteParAsDouble(aNote,MK_m1Ind0);
	  setM1IndEnv = YES;
	  break;
	case MK_m1Ind1:
	  m1Ind1 = MKGetNoteParAsDouble(aNote,MK_m1Ind1);
	  setM1IndEnv = YES;
	  break;
	case MK_m1Phase:
	  m1Phase = MKGetNoteParAsDouble(aNote,MK_m1Phase);
	  /* To avoid clicks, we don't allow phase to be set except at the 
	     start of a phrase. Therefore, we don't set setPhase. */
	  break;
	case MK_m1Ratio:
	  m1Ratio = MKGetNoteParAsDouble(aNote,MK_m1Ratio);
	  setM1Ratio = YES;
	  break;
	case MK_m1Waveform:
	  m1Waveform = MKGetNoteParAsWaveTable(aNote,MK_m1Waveform);
	  setM1Waveform = YES;
	  break;
	case MK_phase:
	  phase = MKGetNoteParAsDouble(aNote,MK_phase);
	  /* To avoid clicks, we don't allow phase to be set except at the 
	     start of a phrase. Therefore, we don't set setPhase. */
	  break;
	case MK_pitchBend:
	  pitchbend = MKGetNoteParAsInt(aNote,MK_pitchBend);
	  setFreqEnv = YES;
	  break;
	case MK_pitchBendSensitivity:
	  pitchbendSensitivity = 
	    MKGetNoteParAsDouble(aNote,MK_pitchBendSensitivity);
	  setFreqEnv = YES;
	  break;
	case MK_portamento:
	  portamento = MKGetNoteParAsDouble(aNote,MK_portamento);
	  setM1IndEnv = setAmpEnv = YES;
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
	  setWaveform = setM1Waveform = setM1Ratio = YES; 
	  break;
	default: /* Skip unrecognized parameters */
	  break;
      } /* End of parameter loop. */

	/* -------------------------------- Waveforms --------------------- */
    if (setWaveform)
      [FMUG(oscUG) setTable:waveform length:wavelen defaultToSineROM:newPhrase];
    if (setM1Waveform)
      [FMUG(modUG) setTable:m1Waveform length:wavelen 
       defaultToSineROM:newPhrase];

    /* ------------------------------- Frequency scaling --------------- */
    if (setCRatio)
      [FMUG(oscUG) setIncRatio:cRatio];
    if (setM1Ratio)
      /* Since table lengths may be set automatically (if wavelen is 0),
	 we must account here for possible difference in table lengths 
	 between carrier and modulator. */
      [FMUG(modUG) setIncRatio:m1Ratio * [FMUG(modUG) tableLength] / 
       [FMUG(oscUG) tableLength]];

    /* ------------------------------- Phases -------------------------- */
    if (setPhase) {
	[FMUG(oscUG) setPhase:phase];
	[FMUG(modUG) setPhase:m1Phase];
    }

    /* ------------------------------ Envelopes ------------------------ */
    if (setAmpEnv) 
      MKUpdateAsymp(FMUG(ampUG),ampEnv,amp0,
		    amp1 * 
		    MKMidiToAmpWithSensitivity(velocity,velocitySensitivity),
		    ampAtt,ampRel,portamento,phraseStatus);
    if (setFreqEnv) {
	double fr0, fr1;
	fr0 = MKAdjustFreqWithPitchBend(freq0,pitchbend,pitchbendSensitivity);
	fr1 = MKAdjustFreqWithPitchBend(freq1,pitchbend,pitchbendSensitivity);
	MKUpdateAsymp(FMUG(incUG),freqEnv,
		      [FMUG(oscUG) incAtFreq:fr0], // Convert to osc increment
		      [FMUG(oscUG) incAtFreq:fr1], 
		      freqAtt,freqRel,portamento,phraseStatus);
    }
    if (setM1IndEnv) {
	double FMDeviation = 
	  [FMUG(oscUG) incAtFreq:(m1Ratio * freq1)] * bright;
	/* See literature on FM synthesis for details about the scaling by
	   FMDeviation */
	MKUpdateAsymp(FMUG(indUG), m1IndEnv, 
		      m1Ind0 * FMDeviation, m1Ind1 * FMDeviation,
		      m1IndAtt,m1IndRel,portamento,phraseStatus);
    }

    /* Vibrato is handled by subclass, if any. */
    [self _setVib:setVibWaveform :setVibFreq :setVibAmp :setRandomVib 
     :newPhrase];

    /* ------------------- Bearing, volume and after touch -------------- */
    if (setOutput)
      [FMUG(outUG) setBearing:bearing scale:MKMidiToAmpAttenuation(volume)];
    if (setAfterTouch)
      [FMUG(fmAddUG) setScale:(1-afterTouchSensitivity) + 
       afterTouchSensitivity * MIDIVAL(afterTouch)];

    return self;
}    

-_setDefaults
  /* Set the instance variables to reasonable default values. We do this
   * after each phrase and upon initialization. This insures that a freshly 
   * allocated SynthPatch will be in a known state. See <musickit/params.h> 
   */
{
    waveform = m1Waveform = nil;      // WaveTables
    wavelen = 0;                      // Wave table length
    phase = m1Phase = 0.0;            // Waveform initial phases
    ampEnv = freqEnv = m1IndEnv = nil;// Envelopes
    freq0 = 0.0;                      // Frequency values
    freq1 = MK_DEFAULTFREQ;           // 440 Hz.
    amp0 = 0.0;                       // Amplitude values
    amp1 = MK_DEFAULTAMP;             // 0.1
    m1Ind0 = 0.0;                     // FM index values
    m1Ind1 = MK_DEFAULTINDEX;         // 1.0
    ampAtt = freqAtt = m1IndAtt = MK_NODVAL;  // Attack times (not set)
    ampRel = freqRel = m1IndRel = MK_NODVAL;  // Release times (not set)
    bright = 1.0;                     // A multiplier on index
    bearing = MK_DEFAULTBEARING;      // 0.0 degrees
    cRatio = MK_DEFAULTCRATIO;        // Carrier frequency scaler.
    m1Ratio = MK_DEFAULTMRATIO;       // Modulator frequency scaler.
    portamento = MK_NODVAL;           // Rearticulation skew duration (not set)
      /* MIDI parameters */
    velocity = MK_DEFAULTVELOCITY;    
    volume = afterTouch = MIDI_MAXDATA;
    afterTouchSensitivity = velocitySensitivity = 0.5;
    pitchbend = MIDI_ZEROBEND;
    pitchbendSensitivity = 3.0;
    return self;
}


@end
