#import <musickit/SynthPatch.h>
#import <musickit/WaveTable.h>
#import <musickit/Envelope.h>

@interface Wave1i:SynthPatch
{
    /* Instance variables for the parameters to which the SynthPatch 
       responds. */

    WaveTable *waveform;    /* Carrier waveform */

    Envelope *ampEnv; /* Amplitude envelope. */ 
    double amp0;      /* Amplitude when ampEnv is at 0 */
    double amp1;      /* Amplitude when ampEnv is at 1 */
    double ampAtt;    /* ampEnv attack time or MK_NODVAL for 'not set'. */
    double ampRel;    /* ampEnv release time or MK_NODVAL for 'not set'. */

    Envelope *freqEnv; /* Frequency envelope. */
    double freq0;     /* Frequency when freqEnv is at 0. */
    double freq1;     /* Frequency when freqEnv is at 1. */
    double freqAtt;   /* freqEnv attack time or MK_NODVAL for 'not set'. */
    double freqRel;   /* freqEnv release time or MK_NODVAL for 'not set'. */

    double bearing;   /* Left/right panning. -45 to 45. */

    double portamento;/* Transition time upon rearticulation, in seconds. */

    double phase;     /* Initial phase in radians */

    double velocitySensitivity; /* Sensitivity to velocity. Scale of 0 to 1 */
    double pitchbendSensitivity; /* Sensitivity to pitchBend in semitones. */

    int velocity;     /* MIDI velocity. Boosts or attenuates amplitude. */
    int pitchbend;    /* MIDI pitchBend. Raises or lowers pitch. */
    int volume;       /* MIDI volume pedal. Anything less than full pedal 
			 functions as an attenuation. */

    int wavelen;      /* WaveTable size. Rarely needed. */
    void *_reservedWave1i;
}

/* Default parameter values, if corresponding parameter is omitted: 
   
   waveform - sine wave

   ampEnv - none
   amp0 - 0
   amp1 - 0.1
   ampAtt - not set (use times specified in envelope directly)
   ampRel - not set (use times specified in envelope directly)

   freqEnv - none
   freq0 - 0.0 Hz.
   freq1 - 440.0 Hz.
   freqAtt - not set (use times specified in envelope directly)
   freqRel - not set (use times specified in envelope directly)

   bearing - 0.0

   portamento - not set (use times specified in envelope directly)

   phase - 0.0 radians

   velocitySensitivity - 0.5 of maximum
   pitchbendSensitivity - 3.0 semitones

   velocity - no boost or attenuation (64)
   pitchbend - no bend (MIDI_ZEROBEND -- see <midi/midi_types.h>)
   volume - no attenuation (127)

   wavelen - automatically-selected value
*/

/* The methods are all explained in the class description for SynthPatch */
+patchTemplateFor:currentNote;
-init;
-controllerValues:controllers;
-noteOnSelf:aNote;
-preemptFor:aNote;
-noteUpdateSelf:aNote;
-(double)noteOffSelf:aNote;
-noteEndSelf;

@end
