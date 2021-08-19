/*
  WaveTable.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
  */

#import <objc/Object.h>

@interface WaveTable : Object
/* 
 * 
 * A WaveTable contain an array of data that's used by a UnitGenerator
 * object as a lookup table.  The Music Kit provides two subclasses of
 * this absract class: Partials, and Samples.
 * 
 * Access to the data is through one of the data: methods.  The method
 * used depends on the data type needed (type DSPDatum (i.e., for the
 * DSP) or type double), the scaling needed and the length of the array
 * needed.  If necessary, the subclass is called upon to recompute the
 * data.  These methods do not copy the data. Thus, the caller should not
 * free or alter the array of data. The need to recompute is recognized
 * on the basis of whether the length or scaling instance variables have
 * changed.  The subclass can signal that a recomputation is needed by
 * setting length to 0.
 *     
 * The computation of the data is handled by the subclass method
 * fillTableLength:scale:.
 *  
 */
{
    int length;             /* Non-0 if a data table exists, 0 otherwise */
    double scaling;         /* 0.0 = normalization scaling */
    DSPDatum *dataDSP;      /* Loaded or computed 24-bit signed data */
    double *dataDouble;     /* Loaded or computed floating-point data */
    void *_reservedWaveTable1;
}

+ new; 
 /* This is how you make up an empty WaveTable. Uses default malloc zone.
    Also sends -init to the new instance. */

- copyFromZone:(NXZone *)zone;
 /* Copies object and internal data structures. */

- copy;
 /* Same as [self copyFromZone:[self zone]];
  */

- init;
 /* This method is ordinarily invoked only by the superclass when an 
    instance is created. You may send this message to reset the object. 
    A subclass should send [super init] if it overrides this method. 
    */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- free;
 /* Free frees dataDSP and dataDouble then sends [super free].
    It also removes the name, if any, from the Music Kit name table. */

- (int)length;
 /* Length returns the length in samples of the data buffers.  If it is 0,
    neither the DSPDatum or real buffer has been allocated. */

- (double)scaling; 
 /* Scaling returns the current scaling of the data buffers.  If it is 0,
    normalization scaling is specified. Normalization is the default. */

- (DSPDatum *) dataDSPLength:(int)aLength scale:(double)aScaling;
 /* Returns the wavetable as an array of DSPDatums, recomputing 
    the data if necessary at the requested scaling and length. If the 
    subclass has no data, returns NULL. The data should neither be modified
    nor freed by the sender. */
 
- (double *)   dataDoubleLength:(int)aLength scale:(double)aScaling ;
 /* Returns the WaveTable as an array of doubles, recomputing 
    the data if necessary at the requested scaling and length. If the 
    subclass has no data, returns NULL. The data should neither be modified
    nor freed by the sender. */
 
 /* The following methods are minor variations of 
    dataDSPScaling:length: and are implemented in terms of it. 
    They use default or previously specified length, scaling or both.  */
- (DSPDatum *) dataDSP;
- (DSPDatum *) dataDSPLength:(int)aLength;
- (DSPDatum *) dataDSPScale:(double)aScaling;

 /* The following methods are minor variations of 
    dataDoubleScaling:length: and are implemented in terms of it. 
    They use default or previously specified length, scaling or both.  */
- (double *)   dataDouble;
- (double *)   dataDoubleLength:(int)aLength;
- (double *)   dataDoubleScale:(double)aScaling;

- fillTableLength:(int)aLength scale:(double)aScaling ;
 /* This method is a subclass responsibility. It must do the following:

   This method computes the data. It allocates or reuses either (or 
   both) of the data arrays with the specified length and fills it with data, 
   appropriately scaled. 

   If only one of data arrays is computed, frees the other and sets
   its pointer to NULL. If data cannot be computed, 
   returns nil with both buffers freed and set to NULL. 

   Note that the scaling and length instance variables must be set by the 
   subclass' fillTableLength: method. 
 */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving. Writes the receiver to archive file.
     Archives itself by writing its name (using MKGetObjectName()), if any.
     All other data archiving is left to the subclass. 
     */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving. Reads the receiver from archive file.
     Archives itself by reading its name, if any, and naming the
     object using MKGetObjectName(). 
     Note that -init is not sent to newly unarchived objects.
     */
@end



