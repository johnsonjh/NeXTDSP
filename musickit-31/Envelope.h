/*
  Envelope.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
*/

#import <objc/Object.h>

 /* Status for Envelopes. */
typedef enum _MKEnvStatus { 
    MK_noMorePoints = -1,
    MK_noEnvError = 0,
    MK_stickPoint,
    MK_lastPoint} 
MKEnvStatus;

@interface Envelope : Object
/* 
 *
 * The Envelope class provides basic support for creating segmented, or
 * breakpoint, functions.  An Envelope object represents a function as 
 * series of points where each point contains three values: x, y, and 
 * smoothing.  The x and y values locate a value y at a particular time x in 
 * an xy coordinate system. The smoothing value is used in AsympUG, a 
 * UnitGenerator that creates asymptotic envelope fuctions on the DSP, and is 
 * explained below.
 * 
 * There are basically two ways to use an Envelope: as a continuous function 
 * that affects a particular DSP synthesis parameter in a Note, or to return 
 * a discrete value y for a given x.  The former use is demonstrated by 
 * assigning an Envelope to a Note parameter that accepts an Envelope object 
 * as its value (through methods defined in the Note class).  
 * For example, the MK_ampEnv parameter 
 * takes an Envelope and applies it to a Note's amplitude.  When the Note is 
 * synthesized on the DSP, its amplitude follows the shape of the Envelope.
 * 
 * Continuous-function Envelopes are described as having three parts: attack,
 * sustain, and release.  You can set the sustain portion of an Envelope by
 * designating one of its points as the stickpoint.  Everything up to the
 * stickpoint is considered to be the Envelope's attack; everything after the
 * stickpoint is its release.  When the stickpoint is reached during DSP
 * synthesis, the stickpoint's y value is sustained until a noteOff arrives to
 * signal the release.  (Keep in mind that all noteDurs are split into
 * noteOn/noteOff pairs by SynthInstrument).
 * 
 * The other way to use an Envelope is to find a discrete y value for a given 
 * x, as provided in the method lookupYForX:.  If the x value doesn't
 * correspond exactly to a point in the Envelope, the method does a linear
 * interpolation between the surrounding points.  Discrete-value is useful for
 * controlling the way a (constant-valued) parameter evolves over a series of
 * Notes.
 *
 * The Envelope class provides two value-setting methods:
 *
 *    setPointCount:xArray:yArray:
 *
 * and the more complete
 *
 *    setPointCount:xArray:orSamplingPeriod:yArray:
 *      smoothingArray:orDefaultSmoothing:
 *
 * The argument to setPointCount: specifies the number of points in the
 * Envelope; the arguments to xArray:, yArray:, and
 * smoothingArray: are pointers to arrays of x, y, and smoothing values for
 * the Envelope's points.  While you must always supply an array of y values,
 * the same isn't true for x and smoothing.  Rather than provide an x array, 
 * you can specify, in the orSamplingPeriod: argument, a sampling period 
 * that's used as an x increment: The x value of the first point is 0, 
 * successive x values are successive integer multiples of the sampling 
 * period value.  The sampling period option is provided to facilitate the 
 * creation of Envelopes that store sampled data.  Similarly, you can supply 
 * a default smoothing, in orDefaultSmoothing:, rather than a smoothing array.
 * In the presence of both an x array and a sampling period, or both a 
 * smoothing array and a default smoothing, the array takes precedence.
 * 
 * When used in DSP synthesis, an Envelope's y values are usually scaled
 * by a constant value set in a related parameter.  For example, the
 * MK_ampEnv Envelope parameter is scaled by the constant in MK_amp,
 * while the MK_freqEnv Envelope parameter is scaled by the constant in
 * MK_freq.  The range of y values depends on the application. For
 * example, amplitude normally is between 0 and 1, while frequency is
 * normally expressed in Hz and assumes values over the range of human
 * hearing. 
 * 
 * While an Envelopes's x values are normally taken as an absolute time in
 * seconds, the Music Kit provides additional parameters that let you reset 
 * the attack and release durations.  For example, the MK_ampAtt parameter 
 * resets the duration of the attack portion of a Note's amplitude Envelope; 
 * similarly, MK_ampRel resets the release duration.
 * 
 * Smoothing is used by the AsympUG UnitGenerator to specify the amount of 
 * time it takes to rise or fall between the y values of successive points.  
 * For example, the smoothing values of the second point determines how long 
 * it takes to travel from the first y value to the second (the smoothing 
 * value of the first point is always ignored).  
 * The value of smoothing is a double between 0.0 and 1.0,
 * where 0.0 means that the next y value is jumped to immediately, and 1.0 
 * means that it takes the entire duration between points to travel from the 
 * current y value to the next.  A smoothing in excess of 1.0 causes the 
 * AsympUG trajectory to fall short of the next point's y value.  
 * See the AsympUG class for the exact formula used to compute the asymptotic
 *  function.
 *
 * CF: Note, AsympUG
 */
 
{
    double defaultSmoothing; /* Smoothing for all points (used in the absence
                                of the smoothing array). */
    double samplingPeriod;   /* Constant x-increment (used in the absence of
                                the x array). */
    double *xArray;          /* Array of x (time) values, if any. */
    double *yArray;          /* Arrays of y (data) values */
    double *smoothingArray;  /* Array of smoothing values, if any. */
    int stickPoint;          /* The object's sustain point. */
    int pointCount;          /* Number of points in the object. */
    void *_reservedEnvelope1;

}
 
 /* METHOD TYPES
  * Creating and freeing 
  * Modifying the object
  * Querying the object
  * Writing the object
  */

+ new; 
 /* TYPE: Creating; Creates and returns an empty Envelope. 
  * Creates and returns an empty Envelope. 
  */

- init; 
 /* TYPE: Creating; Inits the receiver.
  * Inits the receiver.  You never invoke this method directly.  
  * A subclass implementation should send [super init] before performing 
  * its own initialization.  The return value is ignored.  
  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

-(int) pointCount; 
 /* TYPE: Querying; Returns the number of points in the receiver. */

- free; 
 /* TYPE: Creating; Frees the receiver and its contents.
  * Frees the receiver and its contents and removes its name, if any, from the
  * Music Kit name table.
  */

- copyFromZone:(NXZone *)zone;
 /* TYPE: Creating; Copies the object. 
  * Copies Envelope and its arrays.
  */

-copy;
 /* TYPE: Creating; Same as [self copyFromZone:[self zone]];
  */

-(double) defaultSmoothing;
 /* TYPE: Querying; Returns the default smoothing, or MK_NODVAL if there's a 
  * smoothing array.  (Use MKIsNoDVal() to check for MK_NODVAL.)
  */
 
-(double) samplingPeriod;
 /* TYPE: Querying; Returns the sampling period, or MK_NODVAL if there's an x 
  * array.   (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

-(int) stickPoint; 
 /* TYPE: Querying; Returns the stickpoint, or MAXINT if none. */

- setStickPoint:(int)sp; 
 /* TYPE: Modifying; Sets the receiver's stickpoint to the sp'th point 
  * (counting the first point as zero.) Sets the receiver's stickpoint to the 
  * sp'th point. Returns the receiver, or nil if sp is out of bounds. 
  */

- setPointCount:(int)n xArray:(double *) xPtr orSamplingPeriod:(double)period yArray:(double *)yPtr smoothingArray:(double *)smoothingPtr orDefaultSmoothing:(double)smoothing;
 /* TYPE: Modifying; Sets the receiver's x, y, and smoothing values.
  * Allocates three arrays of n elements each and copies the values from
  * xPtr, yPtr, and smoothingPtr into them.  If xPtr is
  * NULL, the x array isn't allocated and the receiver's sampling period is
  * set to period (otherwise period is ignored).  Similarly,
  * smoothing is used as the receiver's default smoothing in the absence of
  * smoothingPtr.  If yPtr is NULL, the receiver's y array is
  * unchanged.  Returns the receiver.  
  */

-  setPointCount:(int)n xArray:(double *) xPtr yArray:(double *)yPtr ;
 /* TYPE: Modifying; Sets the receiver's x and y values.
  * Allocates two arrays of n elements each and copies the values from
  * xPtr and yPtr into them.  If xPtr is
  * NULL, the receiver's x array is unchanged. Similarly, if yPtr 
  * is NULL, the receiver's y array is unchanged. 
  * In either of these cases, it is the sender's responsibility to insure that 
  * the new value of pointCount is the same as that of the old array.
  * Returns the receiver.  
  */

-(double *) yArray;
 /* TYPE: Querying; Returns a pointer to the receiver's y array, or NULL if 
  * none.
  */

-(double *) xArray;
 /* TYPE: Querying; Returns a pointer to the receiver's x array, or NULL if 
  * none. 
  */

-(double *) smoothingArray;
 /* TYPE: Querying; Returns a pointer to the receiver's smoothing array, or 
  * NULL if none. 
  */

-(MKEnvStatus) getNth:(int)n x:(double *)xPtr y:(double *)yPtr smoothing:(double *)smoothingPtr; 
 /* TYPE: Querying; Returns all info for the n'th point in the receiver.
  * Returns, by reference, the x, y, and smoothing values for the n'th point in
  * the receiver (n is zero-based).  The method's return value is an
  * error code that depends on the position of the n'the point:
  *
  * 
  * * Position                          Code
  * * last point in the receiver        MK_lastPoint
  * * stickpoint                        MK_stickPoint
  * * point out of bounds               MK_noMorePoints
  * * any other point                   MK_noEnvError
  * 
  *
  * If the receiver's y array is NULL, or its x array is NULL and
  * its sampling period is 0, then MK_noMorePoints is returned.
  * */

- writeScorefileStream:(NXStream *)aStream; 
 /* TYPE: Writing; Writes the receiver to the open stream aStream.
  * Writes the receiver to the stream aStream in scorefile format.  The
  * stream must already be open.  The points in the receiver are written out, 
  * in order, as (x, y, smoothing), where each value is written
  * as a real number.  The stickpoint is followed by a vertical bar ("|").
  * For example, a simple, three-point envelope describing an arch might look 
  * like this (the second point is the stickpoint):
  *
  * (0.0, 0.0, 0.0) (0.3, 1.0, 0.05) | (0.5, 0.0, 0.2)
  * 
  * Returns nil if the receiver's y array is NULL.  Otherwise returns
  * the receiver.
  */

-(double)lookupYForX:(double)xVal;
 /* TYPE: Querying; Returns the y value for xVal, interpolating as needed.
  * Returns the y value that corresponds to xVal.  If xVal doesn't
  * fall cleanly on one of the receiver's points, the return value is
  * computed as a linear interpolation between the nearest points on either
  * side of xVal.  If xVal is out of bounds, returns
  * the first or last y value, depending on which boundary was exceeded.
  * If the receiver's y array is NULL, returns MK_NODVAL.
  * (Use MKIsNoDVal() to check for MK_NODVAL.)
  */

-(double)releaseDur;
 /* TYPE: Querying; Returns the duration of the release portion of the 
  * receiver. Returns the duration of the release portion of the receiver (the 
  * difference between the x value of the stickpoint and the x value of the
  * final point).  Returns 0 if the receiver doesn't have a stickpoint (or if 
  * the stickpoint is out of bounds) 
  */

-(double)attackDur;
 /* TYPE: Querying; Returns the duration of the attack portion of the receiver.
  * Returns the duration of the attack portion of the receiver.  In other 
  * words, the difference between the x value of the first point and the x 
  * value of the stickpoint.  If the receiver doesn't have a stickpoint (or 
  * if the stickpoint is out of bounds), returns the duration of the entire 
  * Envelope.  
  */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Write object.
  * You never send this message directly. It's invoked by 
  * NXWriteRootObject(). Writes data and object name (using MKGetObjectName())
  * if any. 
  */

- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Read object.
  * You never send this message directly. It's invoked by NXReadObject().
  * Note that -init is not sent to newly unarchived objects.
  * Reads data and sets object name (using MKNameObject()), if any. 
  */

@end



