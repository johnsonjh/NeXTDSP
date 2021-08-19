/* 
	Simp.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit SynthPatch Library.
*/
#import <musickit/SynthPatch.h>
@interface Simp:SynthPatch
{
  double amp, freq, bearing, phase, velocitySensitivity;
  id waveform;
  int wavelen, volume, velocity;
  int pitchbend;
  double pitchbendSensitivity;  
}

+patchTemplateFor:aNote;
 

-noteOnSelf:aNote;
 

-noteUpdateSelf:aNote;
 

-(double)noteOffSelf:aNote;
 

-noteEndSelf;
 

@end

