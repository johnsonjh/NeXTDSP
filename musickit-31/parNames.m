/*
  parNames.m
  Copyright 1987", NeXT", Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  01/19/90/daj - Added vibWaveform parameter.
*/

/* This list must match that in params.h. */

#define NPARS ((int)_MK_maxPrivPar - (int)MK_noPar)

static const char *const parNames[NPARS] = 
{
    "_noPar",
    "keyPressure", /* These 12 must be together */
    "afterTouch",
    "controlChange",
    "pitchBend",
    "programChange",
    "timeCodeQ", 
    "songPosition",
    "songSelect",
    "tuneRequest",      /* this parameter's value is irrelevant */
    "sysExclusive",
    "chanMode",             /* The value of this is a channel mode message */
    "sysRealTime",         /* The value of this is a real time msg token */

    "basicChan",        /* For channel mode messages */
    "controlVal",
    "monoChans",        /* For mono channel mode message argument */ 

    "velocity",      
    "relVelocity",
    "keyNum",

    "velocitySensitivity",
    "afterTouchSensitivity",
    "modWheelSensitivity",
    "breathSensitivity",
    "footSensitivity",
    "portamentoSensitivity",
    "balanceSensitivity",
    "panSensitivity",
    "expressionSensitivity",
    "pitchBendSensitivity",

    "freq",
    "amp",
    "bearing",
    "bright",
    "portamento",
    
    "waveform",
    "waveLen", 
    "phase",

    "cRatio",		
    "c2Ratio",		
    "c2Amp",		
    "c2Waveform",
    "c2Phase",
    "c3Ratio",		
    "c3Amp",
    "c3Waveform",  
    "c3Phase",
    "m1Ratio",
    "m1Ind",
    "m1Waveform",
    "m1Phase",
    "m2Ratio",
    "m2Ind",
    "m2Waveform",
    "m2Phase",
    "m3Ratio",
    "m3Ind",
    "m3Waveform",
    "m3Phase",
    "m4Ratio",
    "m4Ind",
    "m4Waveform",
    "m4Phase",
    "feedback",
    
    "pickNoise",
    "decay",
    "sustain",
    "lowestFreq",

    "svibFreq",
    "svibAmp",
    "rvibFreq",
    "rvibAmp",
    "indSvibFreq",
    "indSvibAmp", 
    "indRvibFreq",
    "indRvibAmp",
    
    "noiseAmp",
    "noiseFreq",

    "freqEnv",
    "freq0", 
    "freqAtt",	
    "freqRel",
    "ampEnv",
    "amp0",
    "ampAtt",
    "ampRel",
    "bearingEnv",
    "bearing0",
    "brightEnv",
    "bright0",
    "brightAtt",
    "brightRel",
    "waveformEnv",
    "waveform0",
    "waveformAtt",
    "waveformRel",

    "c2AmpEnv",
    "c2Amp0",
    "c2AmpAtt",
    "c2AmpRel",
    "c3AmpEnv",
    "c3Amp0",
    "c3AmpAtt",
    "c3AmpRel",
    "m1IndEnv",
    "m1Ind0",
    "m1IndAtt",
    "m1IndRel",
    "m2IndEnv",
    "m2Ind0",
    "m2IndAtt",	
    "m2IndRel",	
    "m3IndEnv",
    "m3Ind0",
    "m3IndAtt",	
    "m3IndRel",	
    "m4IndEnv",
    "m4Ind0",
    "m4IndAtt",	
    "m4IndRel",

    "svibFreqEnv",
    "svibFreq0",
    "rvibFreqEnv",
    "rvibFreq0",
    "indSvibFreqEnv",
    "indSvibFreq0",
    "indRvibFreqEnv",
    "indRvibFreq0",

    "svibAmpEnv",
    "svibAmp0",	
    "rvibAmpEnv",
    "rvibAmp0",
    "indSvibAmpEnv",
    "indSvibAmp0",
    "indRvibAmpEnv",
    "indRvibAmp0",
    
    "noiseAmpEnv",
    "noiseAmp0",
    "noiseAmpAtt",
    "noiseAmpRel",
    "noiseFreqEnv",
    "noiseFreq0",
    
    "synthPatch",
    "synthPatchCount",
    "midiChan",
    "track",

    "title",
    "samplingRate",
    "headroom",
    "tempo",

    "vibWaveform",

    "sequence",
    "text",
    "copyright",
    "lyric",
    "marker",
    "cuePoint",
    "smpteOffset",
    "timeSignature",
    "keySignature",

    "_illegalPar",
    "_dur",
};

static const int parSyns[] = {
    MK_freq,
    MK_amp,
    MK_bright,
    MK_bearing,
    MK_waveform,
    MK_cRatio,
    MK_amp,
    MK_ampAtt,
    MK_ampRel,
    MK_waveform,
    MK_phase,
    MK_c1Amp,
    MK_c2Amp,
    MK_c3Amp,
    MK_m1Ind,
    MK_m2Ind,
    MK_m3Ind,
    MK_m4Ind,
    MK_svibFreq,
    MK_rvibFreq,
    MK_indSvibFreq,
    MK_indRvibFreq,
    MK_svibAmp,
    MK_rvibAmp,
    MK_indSvibAmp,
    MK_indRvibAmp,
    MK_noiseAmp,
    MK_noiseFreq
};

#define SYNS (sizeof(parSyns)/sizeof(int))

static const char *const parSynNames[SYNS] = {
    "freq1",
    "amp1",
    "bright1",
    "bearing1",
    "waveform1",
    "c1Ratio",
    "c1Amp",
    "c1AmpAtt",
    "c1AmpRel",
    "c1Waveform",
    "phase1",
    "c1Amp1",
    "c2Amp1",
    "c3Amp1",
    "m1Ind1",
    "m2Ind1",
    "m3Ind1",
    "m4Ind1",
    "svibFreq1",
    "rvibFreq1",
    "indSvibFreq1",
    "indRvibFreq1",
    "svibAmp1",
    "rvibAmp1",
    "indSvibAmp1",
    "indRvibAmp1",
    "noiseAmp1",
    "noiseFreq1"
};







