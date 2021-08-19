/* 
    params.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */

/* This file defines the Music Kit Note parameters. You can also create your
   own parameters using the Note method +parName:. When an unrecgnized 
   parameter is found in a scorefile, it is created automatically. 

   These parameters are not recognized by all SynthPatches. You must check
   the spec sheet (or header file) for the individual SynthPatch to determine
   the parameters to which it responds. */

typedef enum _MKPars {     /* Note Parameters. */
    MK_noPar = 0,          /* Begin marker */

    /* MIDI opcodes are represented by the presence of one of the following
       12 parameters, along with the noteType */

    MK_keyPressure,     /* MIDI voice msg. (See MIDI spec) */
    MK_afterTouch,      /* MIDI voice msg */
    MK_controlChange,   /* MIDI voice msg */
    MK_pitchBend,       /* MIDI voice msg. Stored as 14-bit signed quantity, 
                           centered on 0x2000. */
    MK_programChange,   /* MIDI voice msg */ 
    MK_timeCodeQ,       /* MIDI time code, quarter frame */
    MK_songPosition,    /* MIDI system common msg (See MIDI spec) */
    MK_songSelect,      /* MIDI system common msg */
    MK_tuneRequest,     /* MIDI system common message.
                           Significant by its presence alone. Its value is
                           irrelevant. */
    MK_sysExclusive,    /* MIDI system exclusive string (See MIDI Spec) */

    MK_chanMode,        /* MIDI chan mode msg: takes a MKMidiParVal val */
    MK_sysRealTime,     /* MIDI real time msg: takes a MKMidiParVal */ 

    /* The remaining MIDI parameters provide additional data needed to
       represent MIDI messages. */

    MK_basicChan,       /* MIDI basic channel for MIDI mode messages */
    MK_controlVal,      /* MIDI Controller value for MK_controlChange */
    MK_monoChans,       /* An arg for the MIDI monoMode msg arg */

    /* The following are derived from MIDI but are used extensively in 
       Orchestra synthesis as well. Most SynthPatches support them */

    MK_velocity,        /* Key velocity for noteOns -- also used as a 
                           brightness and amp in Orchestra synthesis. */
    MK_relVelocity,     /* Release key velocity. Asociated with noteOffs. */
    MK_keyNum,          /* Key number. Also may be used for Orchestra 
                           synthesis as a substitute for freq. Takes a 
                           MKKeyNum value. */
                  
    MK_velocitySensitivity,
    MK_afterTouchSensitivity,
    MK_modWheelSensitivity,
    MK_breathSensitivity,
    MK_footSensitivity,
    MK_portamentoSensitivity,
    MK_balanceSensitivity,
    MK_panSensitivity,
    MK_expressionSensitivity,
    MK_pitchBendSensitivity,

    /* The following are basic synthesis parameters, which should be 
       supported by all SynthPatch classes, if possible.  */ 

    MK_freq,          /* Frequency of the fundamental. 
                         keyNum is an alternative (see Note class). */
    MK_amp,           /* Amplitude. In the range 0 : 1.0. velocity is an
                         alternative (see Note class). */
    MK_bearing,       /* L/R stereo panning. In the range -45 : 45 */
    MK_bright,        /* Brightness. */
    MK_portamento,    /* Portamento time.  In a phrase, the transition time 
                         to a note from the immediately preceding note. */

    /* The following are supported by oscillator-based SynthPatch classes. */

    MK_waveform,        /* WaveTable used for the oscillator */
    MK_waveLen,         /* Length of wavetable. */
    MK_phase,           /* Initial phase in degrees of wavetable */
    
    /* The following are supported by SynthPatch classes which use
       frequency modulation synthesis.  Parameters are predefined
       for three carriers and four modulators, enough to emulate
       the patches on 4-operator fm synthesizers.  For fm instruments, 
       the brightness parameter is usually a synonym for one or more of 
       the modulator indices. */
       
    MK_cRatio,        /* carrier 1 frequency == (cRatio * freq)
                         c1Ratio is a synonym.  */
    MK_c2Ratio,       /* carrier 2 frequency == (c2Ratio * freq) */
    MK_c2Amp,         /* carrier 2 amplitude  */
    MK_c2Waveform,    /* carrier 2 WaveTable */
    MK_c2Phase,
    MK_c3Ratio,       /* carrier 3 frequency == (c3Ratio * freq) */
    MK_c3Amp,         /* carrier 3 amplitude  */
    MK_c3Waveform,    /* carrier 3 WaveTable */
    MK_c3Phase,
    MK_m1Ratio,       /* modulator 1 frequency == (m1Ratio * freq) */
    MK_m1Ind,         /* modulator 1 modulation index */
    MK_m1Waveform,    /* modulator 1 WaveTable */
    MK_m1Phase,
    MK_m2Ratio,       /* Same as above for second modulator. */
    MK_m2Ind,
    MK_m2Waveform,        
    MK_m2Phase,
    MK_m3Ratio,       /* Same as above for third modulator. */
    MK_m3Ind,
    MK_m3Waveform,
    MK_m3Phase,
    MK_m4Ratio,       /* Same as above for fourth modulator. */
    MK_m4Ind,
    MK_m4Waveform,
    MK_m4Phase,
    MK_feedback,      /* Percentage of modulator feedback. */
    
    /* The following are recognized by the Pluck SynthPatch */

    MK_pickNoise,     /* Duration of attack noise burst in seconds. */
    MK_decay,         /* Frequency-independent decay during entire note. 
                         Specified as time constant to reach -60dB of 
                         original value. The special value 0 means no 
                         decay. */
    MK_sustain,       /* Frequency-dependent sustain.
                         In the range 0 == normal : 1 == maximum. */
    MK_lowestFreq,    /* In the first note of a phrase, this warns the
                         SynthPatch what the lowest note of the phrase is.
                         Some SynthPatches (such as Pluck) need this 
                         information to do appropriate allocation. */

    /* The following vibrato parameters are supported by SynthPatches
       that allow vibrato of various kinds */

    MK_svibFreq,       /* Sinusoidal vibrato rate in hz. */
    MK_svibAmp,        /* Sinusoidal vibrato amplitude in 
                          percentage of fundamental frequency. */
    MK_rvibFreq,       /* Random vibrato rate in hz. */
    MK_rvibAmp,        /* Random vibrato amplitude in
                          percentage of fundamental frequency. */
    MK_indSvibFreq,    /* Sinusoidal fm index vibrato in hz. */
    MK_indSvibAmp,     /* Sinusoidal fm index vibrato amplitude in 
                          percentage of current fm index. */
    MK_indRvibFreq,    /* Random fm index vibrato in hz. */
    MK_indRvibAmp,     /* Random fm index vibrato amplitude in 
                          percentage of current fm index. */
               
    /* A noise generator may play a role in some SynthPatches to create
       special effects or increase realism.  The following predefined 
       parameters are useful in these cases. Note that for SynthPatches where
       the noise generator is the primary source of sound (as in Pluck),
       the standard parameters MK_amp and MK_freq are used instead of 
       these parameters. */

    MK_noiseAmp,     /* Amplitude of a noise generator, 0 to 1 */
    MK_noiseFreq,    /* Frequency of a noise generator in Hz. */
                   
    /* The following are commonly-used envelope parameters, which may be 
       supported by some SynthPatch classes.  Note that there are several
       alternative ways to use envelopes, among them:
       
       * Specify the desired values directly as the y values of the envelope.  
       * Do the same, scaling by a normalized constant scaler
       * Do the same, adding a constant to the results.
       * Normalize the envelope values, and multiply times a constant scaler.
       * Do the same, adding a constant to the results. 
       * Use the envelope to interpolate between two specific values.
      
       The SynthPatches provided with the Music Kit allow all of the above 
       situations for parameters which support envelopes, depending on what
       is supplied in the parameter list.  If a parameter is specified as a
       single value, that value is used.  If an envelope is supplied but no
       value, the envelope values are used directly.  If both are
       supplied, the envelope is scaled by the numeric value to produce the
       resulting values.  If two values and an envelope are supplied, the 
       resulting values are the first value plus the difference of the two 
       values times the envelope values (i.e., interpolation).  In other words
       
           frequency(t) == freq0 + (freq1 - freq0) * envelope(t)
      
       where freq0 defaults to 0, freq1 defaults to 1, and the envelope
       defaults to a constant value of 1.
      
        */ 

    MK_freqEnv,      /* Frequency envelope */
    MK_freq0,        /* Fundamental frequency when the envelope is at 0.
                        MK_freq is frequency when the envelope is at 1. 
                        MK_freq1 is a synonym for MK_freq. */    
    MK_freqAtt,      /* Absolute time of attack portion of envelope */
    MK_freqRel,      /* Absolute time of release portion of envelope */

    MK_ampEnv,       /* Amplitude envelope */
    MK_amp0,         /* Amplitude when the envelope is at 0. 
                        MK_amp is amplitude when the envelope is at 1. 
                        MK_amp1 is a synonym for MK_amp. */
    MK_ampAtt,       /* Absolute time of attack portion of envelope */
    MK_ampRel,       /* Absolute time of release portion of envelope */

    MK_bearingEnv,   /* Bearing envelope */
    MK_bearing0,     /* Bearing when the envelope is at 0.
                        MK_bearing is bearing when envelope is at 1.
                        MK_bearing1 is a synonym for MK_bearing. */

    MK_brightEnv,    /* Brightness envelope */
    MK_bright0,      /* Brightness when the envelope is at 0.
                        MK_brightness is brightness when envelope is at 1.
                        MK_brightness1 is a synonym for MK_brightness. */
    MK_brightAtt,    /* Absolute time of attack portion of envelope */
    MK_brightRel,    /* Absolute time of release portion of envelope */

    MK_waveformEnv,  /* WaveTable interpolation envelope */
    MK_waveform0,    /* WaveTable when the envelope is at 0. 
                        MK_waveform is WaveTable when the envelope is at 1. 
                        MK_waveform1 is a synonym for MK_waveform. */
    MK_waveformAtt,  /* Absolute time of attack portion of envelope */
    MK_waveformRel,  /* Absolute time of release portion of envelope */

    /* Additional parameters needed for fm envelopes. */
    
    MK_c2AmpEnv,     /* Amplitude envelope for second carrier.
                        Defaults to MK_ampEnv. */
    MK_c2Amp0,       /* Amplitude when envelope == 0.
                        MK_c2Amp is amplitude when envelope == 1.
                        MK_c2Amp1 is synonym for MK_c2Amp. */
    MK_c2AmpAtt,     /* Absolute time of attack portion of envelope */
    MK_c2AmpRel,     /* Absolute time of release portion of envelope */
    MK_c3AmpEnv,     /* Amplitude envelope for second carrier.
                        Defaults to MK_ampEnv. */
    MK_c3Amp0,       /* Amplitude when envelope == 0.
                        MK_c3Amp is amplitude when envelope == 1.
                        MK_c3Amp1 is synonym for MK_c3Amp. */
    MK_c3AmpAtt,     /* Absolute time of attack portion of envelope */
    MK_c3AmpRel,     /* Absolute time of release portion of envelope */
    MK_m1IndEnv,     /* Frequency modulation index envelope. */
    MK_m1Ind0,       /* Modulation index when envelope is at 0.
                        MK_m1Ind is index when envelope is at 1.
                        MK_m1Ind1 is synonym for MK_m1Ind. */
    MK_m1IndAtt,     /* Absolute time of attack portion of envelope */
    MK_m1IndRel,     /* Absolute time of release portion of envelope */
    MK_m2IndEnv,     /* Same as above for second modulator. */
    MK_m2Ind0,
    MK_m2IndAtt,    
    MK_m2IndRel,    
    MK_m3IndEnv,     /* etc. */
    MK_m3Ind0,
    MK_m3IndAtt,    
    MK_m3IndRel,    
    MK_m4IndEnv,
    MK_m4Ind0,
    MK_m4IndAtt,    
    MK_m4IndRel,

    /* Additional parameters needed when applying envelopes to vibrato */
               
    MK_svibFreqEnv,
    MK_svibFreq0,       /* MK_svibFreq1 == MK_svibFreq */
    MK_rvibFreqEnv,
    MK_rvibFreq0,       /* MK_rvibFreq1 == MK_rvibFreq */
    MK_indSvibFreqEnv,
    MK_indSvibFreq0,    /* MK_indSvibFreq1 == MK_indSvibFreq */
    MK_indRvibFreqEnv,
    MK_indRvibFreq0,    /* MK_indRvibFreq1 == MK_indRvibFreq */

    MK_svibAmpEnv,
    MK_svibAmp0,        /* MK_svibAmp1 == MK_svibAmp */
    MK_rvibAmpEnv,
    MK_rvibAmp0,        /* MK_rvibAmp1 == MK_rvibAmp */
    MK_indSvibAmpEnv,
    MK_indSvibAmp0,     /* MK_indSvibAmp1 == MK_indSvibAmp */
    MK_indRvibAmpEnv,
    MK_indRvibAmp0,     /* MK_indRvibAmp1 == MK_indRvibAmp */
    
    MK_noiseAmpEnv,
    MK_noiseAmp0,       /* Amplitude of noise generator when envelope is 0.
                           MK_noiseAmp is amplitude when envelope is at 1.
                           MK_noiseAmp1 is synonym for MK_noiseAmp. */
    MK_noiseAmpAtt,     /* Attack and decay values for noiseAmp envelope */
    MK_noiseAmpRel,
    MK_noiseFreqEnv,
    MK_noiseFreq0,      /* Frequency of noise generator when envelope is 0.
                           MK_noiseFreq is frequency when envelope is at 1.
                           MK_noiseFreq1 is synonym for MK_noiseFreq. */
    
    /* The following are currently not supported explicitly by any Musickit 
       class. However, by convention, they may appear in Part info notes to 
       associate information with a Part. */

    MK_synthPatch,        /* A suggested synthPatch class which the 
                             app may want to assign to a SynthInstrument */
    MK_synthPatchCount,   /* A suggested count of manually allocated 
                             SynthPatches of the type specified by
                             MK_synthPatch and the default template. */
    MK_midiChan,          /* A suggested midi channel to which the app may
                             want to connect to playing this part on 
                             MidiOut. */
    MK_track,             /* Track number. Set in Part info when a midifile 
                             is read.*/ 

    /* The following are currently not supported explicitly by any Musickit 
       class. However, by convention, they may appear in Score info notes to 
       associate information with a Score. */

    MK_title,            /* A name for the piece. Used in Score info. */
    MK_samplingRate,     /* Suggested DSP sampling rate to be used. For real-
                            time synthesis with the NeXT hardware, this 
                            must be 44100 or 22050. */
    MK_headroom,         /* Suggested "headroom" on DSP synthesis in the
                            range (-1:1). The headroom determines how 
                            conservative the Orchestra is when allocating 
                            resources. See the Orchestra class for details. */
    MK_tempo,            /* Suggested performance tempo for the default 
                            conductor. When a MIDI file is read, this parameter
                            appears in the score info note if the MIDI file
                            has a tempo specified. */

    /* The following parameters are new in release 2.0: */

    MK_vibWaveform,      /* Sinusoidal vibrato waveform */

    /* The following parameters are used primarily in reading/writing Standard 
       MIDI files. See Standard MIDI file 1.0 Specification for details. */
    MK_sequence,         /* Sequence number may be in the Part info. */  
    MK_text,             /* Any text describing anything. */
    MK_copyright,        /* Copyright notice. May be in Score info */
    MK_lyric,            /* Lyric to be sung */
    MK_marker,           /* Rehearsal letter or section name */
    MK_cuePoint,         /* Description of something happening on film. */
    MK_smpteOffset,      /* SMPTE time at which the track starts. May be in
                            Score info. Encoded as a string of five hex 
                            numbers, separated by spaces. See MIDI file spec */
    MK_timeSignature,    /* Encoded as a string of 4 hex numbers, separated
                            by spaces. See MIDI file spec. */
    MK_keySignature,     /* Encoded as a string of 2 hex numbers, separated
                            by a space. See MIDI file spec. */

    MK_privatePars,      /* Marker for private Music Kit parameter block */
                         /* Parameters MK_privatePars to MK_appPars are 
                            reserved. */
    MK_appPars  = 192    /* End marker. Must be evenly divisible by 32. */
  } MKPar;

/* The following synonym parameters are defined for the sake of clarity.
   When envelopes are used, the "1" suffix parameter is used to indicate
   the value when the envelope is at 1.  The "0" suffix parameter indicates
   the value when the envelope is at 0.  */
   
#define MK_freq1    MK_freq
#define MK_amp1        MK_amp
#define MK_bright1    MK_brightness
#define MK_bearing1    MK_bearing
#define MK_waveform1    MK_waveform
#define MK_c1Ratio    MK_cRatio
#define MK_c1Amp    MK_amp
#define MK_c1AmpAtt    MK_ampAtt
#define MK_c1AmpRel    MK_ampRel
#define MK_c1Waveform    MK_waveform
#define MK_c1Phase    MK_phase
#define MK_c1Amp1    MK_c1Amp
#define MK_c2Amp1    MK_c2Amp
#define MK_c3Amp1    MK_c3Amp
#define MK_m1Ind1    MK_m1Ind
#define MK_m2Ind1    MK_m2Ind
#define MK_m3Ind1    MK_m3Ind
#define MK_m4Ind1    MK_m4Ind
#define MK_svibFreq1    MK_svibFreq
#define MK_rvibFreq1    MK_rvibFreq
#define MK_indSvibFreq1 MK_indSvibFreq
#define MK_indRvibFreq1    MK_indRvibFreq
#define MK_svibAmp1    MK_svibAmp
#define MK_rvibAmp1    MK_rvibAmp
#define MK_indSvibAmp1     MK_indSvibAmp
#define MK_indRvibAmp1    MK_indRvibAmp
#define MK_noiseAmp1    MK_noiseAmp
#define MK_noiseFreq1    MK_noiseFreq

/* Some default parameter values */

#define MK_DEFAULTAMP         0.1
#define MK_DEFAULTSMOOTHING    1.0     /* Used by Envelope */
#define MK_DEFAULTBRIGHT     0.5     
#define MK_DEFAULTFREQ              440.0
#define MK_DEFAULTPORTAMENTO     0.1
#define MK_DEFAULTCRATIO     1.0
#define MK_DEFAULTMRATIO     1.0
#define MK_DEFAULTINDEX        2.0
#define MK_DEFAULTSVIBFREQ     4.0
#define MK_DEFAULTSVIBAMP     0.01
#define MK_DEFAULTRVIBFREQ    15.0
#define MK_DEFAULTRVIBAMP    0.008
#define MK_DEFAULTBEARING       0.0
#define MK_DEFAULTSUSTAIN       0.0     /* Used by Pluck */
#define MK_DEFAULTDECAY         0.0     /* Used by Pluck */
#define MK_DEFAULTVELOCITY      64

typedef enum _MKMidiParVal {  /* The order of these is taken from MIDI spec. */
    /* These correspond to MIDI channel mode message values. 
       These are arguments for the parameter MK_chanMode. */
    MK_resetControllers = 262,
    MK_localControlModeOn, 
    MK_localControlModeOff,
    MK_allNotesOff,    
    MK_omniModeOff,
    MK_omniModeOn,
    MK_monoMode,
    MK_polyMode,
    /* These correspond to MIDI system realtime message values. 
       These are arguments for the parameter MK_sysRealTime */
    MK_sysClock,
    MK_sysUndefined0xf9,
    MK_sysStart,
    MK_sysContinue,
    MK_sysStop,
    MK_sysUndefined0xfd,
    MK_sysActiveSensing,
    MK_sysReset}
MKMidiParVal;



