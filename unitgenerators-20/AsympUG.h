/* 
	AsympUG.h 
	Copyright 1989, NeXT, Inc.

	This class is part of the Music Kit UnitGenerator Library.
*/
#import <musickit/UnitGenerator.h>
#import <musickit/Conductor.h>
#import <musickit/SynthPatch.h>
#import <musickit/Envelope.h>

@interface AsympUG:UnitGenerator
 /* AsympUG - from dsp macro /usr/lib/dsp/ugsrc/asymp.asm. (see source for details)

   Asymptotic ramper.
	
   You allocate a subclass of the form AsympUG<a>, where <a> = space of output.

   This may be used as a simple ramper but also contains 
   support for sending an envelope to the DSP incrementally. 
   The clockConductor (see Conductor) is used for timing of envelope values.  
   However, -finish is sent based on the arrival of a noteOff to the 
   SynthPatch, which is (usually) managed by another Conductor.

   Asymptotic envelopes consist of a list of (x,y,s) triples, where x and y
   are cartesian coordinate break points and s is a smoothing constant.

   The mathematical interpretation of asymptotic envelopes is as follows:
 * * 	Yn is the target, considered to be in the infinite future
 * *	Xn is the time of the right-hand side of the segment
 * * 	    (i.e. the time to interrupt the trajectory toward Yn)
 * *	Sn is the smoothing constant to get to Yn. If Sn is 0, the point
 * *        is reached immediately. If Sn is 1.0, the point is reached, 
 * *        within about -48dB at the time of the next update. If Sn is 
 * *        larger, the point is not reached within -48dB by the time of the 
 * *        next update. An S value of infinity will cause the envelope to 
 * *        never change value. 
 * *
 * *	The first point, X0, is assumed to be the right-hand side of the 
 * * 	non-existant first segment. 
 * *	Y0 is the initial point (which may or may not be used, depending on 
 * *          the value of the instance variable useInitialvalue (see below)).
 * * 	S0 is ignored. 	

 Asymptotic envelopes have the advantage that if, for some reason, the host
 gets a little bit behind, the envelope will not continue unbounded toward
 disaster. That is, they are "self-limiting". They have the disadvantage of
 not reaching their targets in some cases.
 
 The envelope has a "stick point". When the envelope handler reaches the
 stick point, it does not proceed to the next point until it receives the
 -finish message. If there is no stick point, the -finish message is ignored.
 If the envelope handler has not yet reached the stick point when the -finish
 message is received, the envelope handler proceeds to the first point after
 the stick point and continues from there.

 */
{
    id anEnv;                     /* The envelope, if any. */
    double (*scalingFunc)();      /* scalingFunc is a double-valued scaling 
				     function of two arguments: 
				     the current double value 
				     and the UnitGenerator instance id. 
				     If funPtr is NULL, 
				     the identity mapping is used. */
    int envelopeStatus;           /* Status of last envelope point accessed.
				     I.e. if we just set the target which    
				     is the stickpoint, envelopeStatus is 
				     MK_stickPoint. See <musickit/Envelope.h>. 
				     */
    int arrivalStatus;            /* Status of actual progression on the
				     DSP. I.e. if we have already interrupted
				     the trajectory to the stickpoint,
				     arrivalStatus is MK_stickPoint. 
				     See <musickit/Envelope.h> */
    double timeScale;             /* time scaling. The smoothing values are
				     scaled as well. */
    double releaseTimeScale;      /* For the post-stick-point segment. */ 
    double yScale;                /* Y scale */
    double yOffset;               /* Y offset */
    double targetX;               /* X value of current target. */
    char useInitialValue;         /* Controls how initial value of 
				     envelope is used. */
    int curPt;                    /* Current envelope point. */
    double _reservedAsymp1;
    MKMsgStruct * _reservedAsymp2;
    double _reservedAsymp3;
    double _reservedAsymp4;
    double _reservedAsymp5;
    double _reservedAsymp6;
    double _reservedAsymp7;
    DSPDatum _reservedAsymp8;
}

-(MKEnvStatus)envelopeStatus;
 /* Status of last envelope point accessed.
   I.e. if we just set the target which    
   is the stickpoint, envelopeStatus is 
   MK_stickPoint. If no envelope, returns MK_noMorePoints. */

-setOutput:aPatchPoint;
  /* Set output of ramper */

-setTargetVal:(double)aVal;
 /* Sets the target value. If the receiver is already 
   processing an envelope, that envelope is not interrupted. The new
   point is simply inserted. */ 

-setCurVal:(double)aVal;
 /* Sets the current envelope value. If the receiver is already 
   processing an envelope, that envelope is not interrupted. The new
   point is simply inserted. */ 

-setRate:(double)aVal;
  /* Sets the rate of the exponential. (1-e^T/tau), where T is sampling
     period and tau is the time constant.
     If the receiver is already 
     processing an envelope, that envelope is not interrupted. The new
     point is simply inserted. */

-setT60:(double)seconds;
  /* Sets the time constant of the exponential. Same as 
     [self setRate:7.0/(seconds*srate)]. */

-preemptEnvelope;
  /* Head to last point of envelope in time specified by 
     MKSetPreemptDuration(). */

-setEnvelope:anEnvelope yScale:(double)yScale yOffset:(double)yOffset
 xScale:(double)xScale releaseXScale:(double)rXScale
 funcPtr:(double(*)())func ;
 /* Initializes envelope handler with the values specified. func is described 
    above.
  */

-resetEnvelope:anEnvelope yScale:(double)yScaleVal yOffset:(double)yOffsetVal
    xScale:(double)xScaleVal releaseXScale:(double)rXScaleVal
    funcPtr:(double(*)())func  transitionTime:(double)transTime;
 /* Like setEnvelope:yScaleVal:yOffset:xScale:releaseXScale:funcPtr:, but
    doesn't bind the first value of the envelope.  
    Additionally allows a transitionTime to be specified. Transition time is
    the absolute time used to get to the second value of the envelope (xScale
    is not used for this first segment). If set to MAXDOUBLE, the time will 
    be the normal time of the first envelope segment. 
 */

-setYScale:(double)yScaleVal yOffset:(double)yOffsetVal;
 /* Resets the scale and offset and updates the running envelope's target. */

-setReleaseXScale:(double)rXScaleVal ;
 /* Sets the envelope's X scaling for the release portion (the portion after
    the stick-point). */

-envelope;
 /* Returns envelope or nil if none. */

-runSelf;
 /* Starts the envelope handler running. */

-abortSelf;
  /* Same as abortEnvelope. */

-idleSelf;
 /* Patches the output to sink. */

-(double)finishSelf;
 /* If envelope is at stick point, proceed. If it is not yet at stick point, 
    head for point after stick pont. If there is no stick point in the envelope,
    has no effect. Also returns time to end of envelope plus a tiny grace 
    time to get to the final value. */

+(BOOL)shouldOptimize:(unsigned) arg;
 /* Specifies that all arguments are to be optimized if possible except the
    current value. */

-abortEnvelope;
 /* Terminates an envelope before it has completed. Also sets anEnv to nil.*/

-setConstant:(double)aVal;
 /* Abort any existing envelope and set the receiver to output a constant. */

 /* The following function simplifies envelope handling. See discussion 
    below and example SynthPatches. */ 

extern void
  MKUpdateAsymp(id asymp, id envelope, double val0, double val1,
		double attDur, double relDur, double portamentoTime,
		MKPhraseStatus status);

@end

/* 
	How Envelopes Are Used in the Music Kit SynthPatch Library 

In the Music Kit SynthPatch library, envelopes are specified in the
parameter list as some combination of an Envelope object (a list of
time, value, and optional smoothing values), up to two value-modifying
numbers, and up to two time-scaling numbers.  See
<unitgenerators/AsympUG.h> for details about the smoothing values in
an Envelope.  The parameter names all begin with something descriptive
of their use, such as "amp" or "freq".  The Envelope parameter has the
suffix "Env", e.g., "freqEnv".  The value-modifying parameters have
the suffixes "0" and "1", e.g., "freq0" and "freq1". The time-scaling
parameters have the suffixes "Att" and "Rel", e.g., "freqAtt" and
"freqRel".  In addition, just the descriptive part of the name may be
substituted for the "1"-suffix parameter, e.g. "freq" = "freq1".

	The Envelope and Value-modifying Parameters

The synthpatches have been designed to allow several alternative ways
to use Envelopes, depending on the precise combination of these three
parameters. In the following paragraphs, the term "val0" stands for
any "0"-suffix numeric parameter, "val1" stands for any "1"-suffix
numeric parameter, "valAtt" stands for any "Att"-suffix parameter, and
"valRel" stands for any "Rel"-suffix parameter.

If no Envelope is supplied, the desired value is specified in the
"val" field, (e.g. "freq") and the result is this value applied as a
constant. If an Envelope is supplied but no "val0" or "val1" numbers
(e.g. "freqEnv" is supplied, but no "freq0" nor "freq1"), the Envelope
values are used directly.  If only an Envelope and "val0" are supplied,
the Envelope's y values are used after being added to "val0".  If only
an Envelope and "val1" are supplied, the Envelope's y values are used
after being multipled by "val1".  If an Envelope and both "val0" and "val1"
are supplied, the values used are "val0" plus the difference of "val1" and
"val0" multiplied by the Envelope values.  In other words, the Envelope
specifies the interpolation between the two numeric values.  "Val0"
specifies the value when the Envelope is 0, and "val1" specifies the
value when the Envelope is 1.

In mathematical terms, the formula for an Envelope parameter val is then:
      
    DSP Value(t) = val0 + (val1 - val0) * valEnv(t)
       
where "val0" defaults to 0, "val1" defaults to 1, and "valEnv" defaults to a
constant value of 1 when only "val1" is supplied, and 0 otherwise.

	The Envelope and Time-scaling Parameters

The "valAtt" and "valDec" numeric parameters directly affect the
"attack" and "decay" portions of an envelope, which are considered to
be the portions before and after the stickpoint, respectively.  When
supplied, the relevant portion of the envelope is scaled so that the
total time of that portion is the time specified in the parameter in
seconds.  For example, if valAtt is set to .5, the segments of the
portion of the envelope before the stick point will be proportionally
scaled so that the total time is .5 seconds.  The smoothing values are
also scaled proportionally so that the behavior of time-scaled
envelopes remains consistent.

ValAtt can only be set when an envelope is also supplied in the same note.
However, valDec may be set independently, e.g., in a noteOff where an
envelope was supplied in the preceeding noteOn.

	Phrases 

The Music Kit supports continuity between notes in a phrase.  When two
notes are part of the same phrase (they have the same time tag) or a
sounding note is updated by a noteUpdate, the envelope of the latter
note does not simply interrupt that of the earlier note.  Rather, the
first point of the latter envelope is ignored, and the envelope
proceeds directly to the second point, starting from wherever the
earlier envelope happens to be when the new noteOn occurs.  The time
it takes to do this is, by default, the time of the first segment of
the latter envelope, possibly affected by its "valAtt" parameter.
However, the "portamento" parameter may be used to specify the time
(in seconds) for the transition should take.  All of the x (time)
values of the envelope, except the first, are increased by the amount
needed to make the first segment take the desired amount of time. In
addition, the smoothing value for the first segment is adjusted
appropriately. 

The single "portamento" parameter affects all envelopes which the synthpatch
may be using.

MKUpdateAsymp() may be called with any of its arguments of type double 
"unset". "unset" is indicated by the value MAXDOUBLE. 

Caveat concerning FM:
  With the current (2.0) implementation of the FM family of SynthPatches, the
  amount of modulation (peak frequency deviation) is computed from freq1.
  That means that if you use the convention of putting the frequencies in the
  envelope itself and setting freq1 to 1, the index values will have to be 
  boosted by the fundamental frequency. 
*/













