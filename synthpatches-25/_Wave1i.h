#import "Wave1i.h"

typedef struct __MKSPWAVENums {
  short ampUG, incUG, oscUG, outUG, svibUG, nvibUG, onepUG,
      addUG, mulUG, xsig, ysig;
} _MKSPWAVENums;

/* Don't bother with prefix for macros since they're only in compile-time
   address space. */

#define WAVEDECL(_template,_struct) \
  static id _template = nil;\
  static _MKSPWAVENums _struct = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}

#define WAVENUM(ugname) (((_MKSPWAVENums *)(_ugNums))->ugname)
#define WAVEUG(ugname) NX_ADDRESS(synthElements)[WAVENUM(ugname)]
#define _ugNums _reservedWave1i
#define MIDIVAL(midiControllerValue) \
  ((double)midiControllerValue)/((double)MIDI_MAXDATA)

extern id _MKSPGetWaveNoVibTemplate(_MKSPWAVENums *ugs,id oscClass);
extern id _MKSPGetWaveAllVibTemplate(_MKSPWAVENums *ugs,id oscClass);
extern id _MKSPGetWaveRanVibTemplate(_MKSPWAVENums *ugs,id oscClass);
extern id _MKSPGetWaveSinVibTemplate(_MKSPWAVENums *ugs,id oscClass);

@interface Wave1i(Private)

-(void)_setModWheel:(int)val;
-(void)_setSvibFreq0:(double)val;
-(void)_setSvibFreq1:(double)val;
-(void)_setSvibAmp0:(double)val;
-(void)_setSvibAmp1:(double)val;
-(void)_setRvibAmp:(double)val;
-(void)_setVibWaveform:(id)obj;
-(void)_setVib:(BOOL)setVibWaveform :(BOOL)setVibFreq :(BOOL)setVibAmp 
  :(BOOL)setRandomVib :(BOOL)newPhrase;
-_updateParameters:aNote;
-_setDefaults;

@end
