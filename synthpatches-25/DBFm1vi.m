#ifdef SHLIB
#include "shlib.h"
#endif

/* 
  Modification history:

  04/23/90/daj - Made this a subclass of Fm1vi.
  04/25/90/daj - Changed timbre string handling to use NoCopy string func
  08/28/90/daj - Changed initialize to init.
*/
#import <musickit/musickit.h>
#import <midi/midi_types.h>
#import <objc/HashTable.h>
#import <musickit/unitgenerators/unitgenerators.h>
#import <appkit/nextstd.h>
#import "DBFm1vi.h"
/* Database of analyzed timbres */
#import "partialsDB.h"

@implementation DBFm1vi

#import "_Fm1i.h"
#import "_synthPatchInclude.h"

+initialize
{
    _MKSPInitializeTimbreDataBase();
    return self;
}

-controllerValues:controllers
  /* Initializes controller variables to current controller values when
     synthpatch is set up. */
{
  if ([controllers isKey:(const void *)MIDI_MAINVOLUME])
    volume = (int)[controllers valueForKey:(const void *)MIDI_MAINVOLUME];
  if ([controllers isKey:(const void *)MIDI_MODWHEEL])
    modWheel = (int)[controllers valueForKey:(const void *)MIDI_MODWHEEL];
  if ([controllers isKey:(const void *)MIDI_BALANCE])
    balance = (int)[controllers valueForKey:(const void *)MIDI_BALANCE];
  if ([controllers isKey:(const void *)MIDI_PAN])
    pan = (int)[controllers valueForKey:(const void *)MIDI_PAN];
  return self;
}

-_setDefaults
  /* Set the static global variables to reasonable values. */
{
    [super _setDefaults];
    waveform0 = nil;
    waveform1 = nil;
    pan       = MAXINT;
    balance   = 127;
    panSensitivity = 1.0;
    balanceSensitivity = 1.0;
    table = NULL;
    return self;
  }

-init
{
  [super init];
  NX_MALLOC(_localTable,DSPDatum,512);
  return self;
}

-freeSelf
{
  NX_FREE(_localTable);
  return self;
}

-_setWaveform:(BOOL)newPhrase str0:(char *)waveStr0 str1:(char *)waveStr1
{
  int interp = balance;
  if sValid(waveStr0) {
    if (*waveStr0=='0')
      waveform0 = _MKSPPartialForTimbre(1 + waveStr0,freq0*cRatio,0);
    else waveform0 = _MKSPPartialForTimbre(waveStr0,freq1*cRatio,0);
  }
  if sValid(waveStr1) {
    if (*waveStr1=='0')
      waveform1 = _MKSPPartialForTimbre(1 + waveStr1,freq0*cRatio,0);
    else waveform1 = _MKSPPartialForTimbre(waveStr1,freq1*cRatio,0);
  }
  if ((waveform0 && !waveform1) || (waveform0==waveform1)) {
    waveform = waveform0;
    table = [waveform0 dataDSPLength: wavelen];
  }
  else if (waveform1 && !waveform0) {
    waveform = waveform1;
    table = [waveform1 dataDSPLength: wavelen];
  }
  else if (waveform0 && waveform1) {
    interp = balance * balanceSensitivity;
    if (interp==0) {
      waveform = waveform0;
      table = [waveform0 dataDSPLength: wavelen];
    }
    else if (interp==127) {
      waveform = waveform1;
      table = [waveform1 dataDSPLength: wavelen];
    }
    else {
      register DSPDatum *t0, *t1, *end;
      int x0, x1;
      double tmp = (double)interp/127.0;
      t0 = [waveform0 dataDSPLength: wavelen];
      t1 = [waveform1 dataDSPLength: wavelen];
      table = _localTable;
      end = table + wavelen;
      while (table<end) { 
	x0 = *t0++;
	x1 = *t1++;
	if (x0 &  0x800000) x0 |= 0xFF000000;
	if (x1 &  0x800000) x1 |= 0xFF000000;
	*table++ = (x0 + (int)((double)(x1-x0)*tmp)) & 0xFFFFFF;
      }
      table = _localTable;
    }
  }
  if (!table || (!waveform0 && waveform1) || (waveform0 && !waveform1) ||
      (interp==0) || (interp==127))
    [FMUG(oscUG) setTable:waveform length:wavelen defaultToSineROM:newPhrase];
  else {
    if (_synthData && ([_synthData length]!=wavelen)) {
      [orchestra dealloc:_synthData];
      _synthData = nil;
    }
    if (!_synthData) 
      _synthData = [orchestra allocSynthData:MK_yData length:wavelen];
    if (_synthData)
      [_synthData setData:table length:wavelen offset:0];
    else if (MKIsTraced(MK_TRACEUNITGENERATOR))
      fprintf(stderr,"Insufficient wavetable memory at time %.3f. \n",MKGetTime());
    [FMUG(oscUG) setTable:_synthData length:wavelen defaultToSineROM:newPhrase];
  }
  return self;
}

-_updateParameters:aNote
  /* Updates the SynthPatch according to the information in the note and
   * the note's relationship to a possible ongoing phrase. 
   */
{
    BOOL newPhrase, setWaveform, setM1Waveform, setM1Ratio, setOutput,
         setRandomVib, setCRatio, setAfterTouch, setVibWaveform, setVibFreq,
         setVibAmp, setPhase, setAmpEnv, setFreqEnv, setM1IndEnv;
    void *state; /* For parameter iteration below */
    int par;     
    MKPhraseStatus phraseStatus = [self phraseStatus];
    char *waveStr0 = "";
    char *waveStr1 = "";

    /* Initialize booleans based on phrase status -------------------------- */
    switch (phraseStatus) {
      case MK_phraseOn:          /* New phrase. */
      case MK_phraseOnPreempt:   /* New phrase but using preempted patch. */
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	      setFreqEnv = setM1IndEnv = YES;  /* Set everything for new phrase */
	break;
      case MK_phraseRearticulate: /* NoteOn rearticulation within phrase. */
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = NO;
	setAmpEnv = setFreqEnv = setM1IndEnv = YES; /* Just restart envelopes  */
	break;
      case MK_phraseUpdate:       /* NoteUpdate to running phrase. */
      case MK_phraseOff:          /* NoteOff to running phrase. */
      case MK_phraseOffUpdate:    /* NoteUpdate to finishing phrase. */
      default: 
	newPhrase = setWaveform = setM1Waveform = setM1Ratio = 
	  setOutput = setRandomVib = setCRatio = setAfterTouch = 
	    setVibWaveform = setVibFreq = setVibAmp = setPhase = setAmpEnv = 
	      setFreqEnv = setM1IndEnv = NO;  /* Only set what's in Note */
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
	case MK_afterTouch:
	  afterTouch = MKGetNoteParAsInt(aNote,MK_afterTouch);
	  setAfterTouch = YES;
	  break;
	case MK_afterTouchSensitivity:
	  afterTouchSensitivity = 
            MKGetNoteParAsDouble(aNote,MK_afterTouchSensitivity);
	  setAfterTouch = YES;
          break;
	case MK_panSensitivity:
	  panSensitivity = MKGetNoteParAsDouble(aNote,MK_panSensitivity);
	  setOutput = YES;
	  break;
	case MK_balanceSensitivity:
	  balanceSensitivity = MKGetNoteParAsDouble(aNote,MK_balanceSensitivity);
	  setWaveform = YES;
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
		modWheel = MKGetNoteParAsInt(aNote,MK_controlVal);
		setVibFreq = setVibAmp = YES;
	    }
	    else if (controller == MIDI_PAN) {
		pan = MKGetNoteParAsInt(aNote,MK_controlVal);
		setOutput = YES;
	    }
	    else if (controller == MIDI_BALANCE) {
		balance = MKGetNoteParAsInt(aNote,MK_controlVal);
		setWaveform = YES;
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
	  rvibAmp = MKGetNoteParAsDouble(aNote,MK_rvibAmp);
	  setRandomVib = YES;
	  break;
	case MK_svibFreq0:
	  svibFreq0 = MKGetNoteParAsDouble(aNote,MK_svibFreq0);
	  setVibFreq = YES;
	  break;
	case MK_svibFreq1:
	  svibFreq1 = MKGetNoteParAsDouble(aNote,MK_svibFreq1);
	  setVibFreq = YES;
	  break;
	case MK_svibAmp0:
	  svibAmp0 = MKGetNoteParAsDouble(aNote,MK_svibAmp0);
	  setVibAmp = YES;
	  break;
	case MK_svibAmp1:
	  svibAmp1 = MKGetNoteParAsDouble(aNote,MK_svibAmp1);
	  setVibAmp = YES;
	  break;
	case MK_vibWaveform:
	  vibWaveform = MKGetNoteParAsWaveTable(aNote,MK_vibWaveform);
	  setVibWaveform = YES;
	  break;
	case MK_velocity:
	  velocity = MKGetNoteParAsDouble(aNote,MK_velocity);
	  setAmpEnv = YES;
	  setM1IndEnv = YES;
	  break;
	case MK_velocitySensitivity:
	  velocitySensitivity = 
	    MKGetNoteParAsDouble(aNote,MK_velocitySensitivity);
	  setAmpEnv = YES;
	  setM1IndEnv = YES;
	  break;
	case MK_waveform0:
	  waveform0 = MKGetNoteParAsWaveTable(aNote,MK_waveform0);
	  if (waveform0==nil)
	    waveStr0 = MKGetNoteParAsStringNoCopy(aNote,MK_waveform0);
	  setWaveform = YES;
	  break;
	case MK_waveform1:
	  waveform1 = MKGetNoteParAsWaveTable(aNote,MK_waveform1);
	  if (waveform1==nil)
	    waveStr1 = MKGetNoteParAsStringNoCopy(aNote,MK_waveform1);
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
      [self _setWaveform:newPhrase str0:waveStr0 str1:waveStr1];
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
		      [FMUG(oscUG) incAtFreq:fr0], /* Convert to osc increment */
		      [FMUG(oscUG) incAtFreq:fr1], 
		      freqAtt,freqRel,portamento,phraseStatus);
    }
    if (setM1IndEnv) {
	double FMDeviation = 
	  [FMUG(oscUG) incAtFreq:(m1Ratio * freq1)] * bright *
	    ((1.0-velocitySensitivity) + 
	     velocitySensitivity * (double)velocity/MK_DEFAULTVELOCITY);
	/* See literature on FM synthesis for details about the scaling by
	   FMDeviation */
	MKUpdateAsymp(FMUG(indUG), m1IndEnv, 
		      m1Ind0 * FMDeviation, m1Ind1 * FMDeviation,
		      m1IndAtt,m1IndRel,portamento,phraseStatus);
    }

    if (FMNUM(svibUG)>=0) {
      double modPc = (double)modWheel/127.0;
      if (setVibWaveform)
	[FMUG(svibUG) setTable:vibWaveform length:128 defaultToSineROM:YES];
      else if (newPhrase)
	[FMUG(svibUG) setTableToSineROM];
      if (setVibFreq)
	[FMUG(svibUG) setFreq:svibFreq0+(svibFreq1-svibFreq0)*modPc];
      if (setVibAmp)
	[FMUG(svibUG) setAmp:svibAmp0+(svibAmp1-svibAmp0)*modPc];
    }
    if ((FMNUM(nvibUG)>=0) && (setRandomVib || newPhrase)) {
      [FMUG(onepUG) setB0:.004*rvibAmp];
      [FMUG(onepUG) clear];
      if (newPhrase) {
	[FMUG(onepUG) setA1:-.9999];
	[FMUG(nvibUG) anySeed];
      }
    }

    /* ------------------- Bearing, volume and after touch -------------- */
    if (setOutput) {
      if (iValid(pan)) bearing = panSensitivity * 45.0 * (double)(pan-64)/64.0;
      [FMUG(outUG) setBearing:bearing scale:MKMidiToAmpAttenuation(volume)];
    }
    if (setAfterTouch)
      [FMUG(fmAddUG) setScale:(1-afterTouchSensitivity) + 
       afterTouchSensitivity * (double)afterTouch/127.0];
    return self;
}    

@end
