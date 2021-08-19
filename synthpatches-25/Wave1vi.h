#import <musickit/SynthPatch.h>

/* ------------------------------------------------------------------------ * 
 * Wave1vi is like Wave1i but adds periodic and random vibrato.
 * 
 * See Wave1i.h for a description of the non-vibrato parameters.
 * ------------------------------------------------------------------------ */

#import "Wave1i.h"

@interface Wave1vi:Wave1i
{
    /* Instance variables for the parameters to which the SynthPatch 
       responds. */

    WaveTable *vibWaveform; /* Waveform used for vibrato. */
    double svibAmp0;  /* Vibrato, on a scale of 0 to 1, when modWheel is 0. */
    double svibAmp1;  /* Vibrato, on a scale of 0 to 1, when modWheel is 127.*/
    double svibFreq0; /* Vibrato freq in Hz. when modWheel is 0. */
    double svibFreq1; /* Vibrato freq in Hz. when modWheel is 1. */
    
    double rvibAmp;   /* Random vibrato. On a scale of 0 to 1. */

    int modWheel;     /* MIDI modWheel. Controls vibrato frequency and amp */
}

/* Default parameter values, if corresponding parameter is omitted: 
   vibWaveform - sine wave
   svibAmp0 - 0.0
   svibAmp1 - 0.0
   svibFreq0 - 0.0 Hz.
   svibFreq1 - 0.0 Hz.
    
   rvibAmp - 0.0

   modWheel - vibrato amplitude of svibAmp1 and frequency of svibFreq1 (127)

*/

/* All methods are inherited. */

@end
