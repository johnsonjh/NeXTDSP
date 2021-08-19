/* 
	DBWave1vi.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit SynthPatch Library.
*/
#import "Wave1vi.h"
#import <musickit/SynthData.h>

@interface DBWave1vi:Wave1vi
{
  double panSensitivity;
  double balanceSensitivity;
  WaveTable *waveform0;
  WaveTable *waveform1;
  int pan;
  int balance;
  SynthData *_synthData;
  DSPDatum *table;
  DSPDatum *_localTable;
}

/* All methods are inherited. */

@end

