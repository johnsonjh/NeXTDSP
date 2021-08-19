/* 
	DBWave2vi.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit SynthPatch Library.
*/
#import <musickit/SynthPatch.h>
@interface DBWave2vi:SynthPatch

{
  double amp0, amp1, ampAtt, ampRel, freq0, freq1, freqAtt, freqRel,
         bearing, phase, portamento, svibAmp0, svibAmp1, rvibAmp,
         svibFreq0, svibFreq1, velocitySensitivity, panSensitivity,
         waveformAtt, waveformRel, pitchbendSensitivity;
  id ampEnv, freqEnv, waveform0, waveform1, waveformEnv;
  int wavelen, volume, velocity, modwheel, pan, pitchbend;
  void *_ugNums;
}

+patchTemplateFor:aNote;
   

-noteOnSelf:aNote;
 

-noteUpdateSelf:aNote;
 

-(double)noteOffSelf:aNote;
 

-noteEndSelf;
 

@end

