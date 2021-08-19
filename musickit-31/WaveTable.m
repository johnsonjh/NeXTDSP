#ifdef SHLIB
#include "shlib.h"
#endif

/* 
  Modification history:

   04/21/90/daj - Small mods to get rid of -W compiler warnings.
   08/27/90/daj - Changed to zone API.
*/

#import "_musickit.h"
#import "_parameter.h"  /* Has double to fix 24 conversion */
#import "_scorefile.h"
#import "_error.h"
#import "WaveTable.h"

@implementation WaveTable : Object
/* WaveTable is an abstract class inherited by classes which produce or store 
   an array of data to be used as a lookup table in a UnitGenerator.
   Subclasses provided by the Music Kit are

   * * Partials computes a WaveTable given an arrays of harmonic amplitudes, 
   frequency ratios, and phases.

   * * Samples stores a WaveTable of existing samples read in from a Sound 
   object or soundfile.

   The WaveTable class caches multiple formats for the data. This is
   usefuly because it is expensive to recompute the data.
   Access to the data is through one of the "data" methods (-dataDSP, 
   -dataDouble, etc.).  The method
   used depends on the data type needed (type DSPDatum for the DSP
   or type double), the scaling needed, and the length of the array needed.
   The caller should not free nor alter the array of data.

   If necessary, the subclass is called upon to recompute the data.
   The computation of the data is handled by the subclass method 
   fillTableLength:scale:.
*/
{
    int length;	         /* Non-0 if a data table exists, 0 otherwise */
    double scaling;      /* Scaling or 0.0 for normalization. */
    DSPDatum *dataDSP;   /* Computed DSPDatum data */
    double *dataDouble;  /* Computed double data */
    void *_reservedWaveTable1;
}

static void doubleToFix24Array (double *doubleArr, DSPDatum *fix24Arr, int len)
{
    register double *endArr;
    endArr = doubleArr + len;
    while (doubleArr < endArr)
    	*fix24Arr++ = _MKDoubleToFix24(*doubleArr++);
}

static void fix24ToDoubleArray (DSPDatum *fix24Arr, double *doubleArr, int len)
{
    register DSPDatum *endArr;
    endArr = fix24Arr + len;
    while (fix24Arr < endArr)
    	*doubleArr++ = _MKFix24ToDouble(*fix24Arr++);
}

+  new
  /* This is how you make up an empty seg envelope */
{
    self = [super allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Avoid breaking pre-2.0 apps. */
    return self;
}


#define VERSION2 2

+initialize
{
    if (self != [WaveTable class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving. Writes the receiver to archive file.
     Archives itself by writing its name (using MKGetObjectName()), if any.
     All other data archiving is left to the subclass. 
     */
{
    char *str;
    str = (char *)MKGetObjectName(self);
    [super write:aTypedStream];
    NXWriteType(aTypedStream,"*",&str);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving. Reads the receiver from archive file.
     Archives itself by reading its name, if any, and naming the
     object using MKGetObjectName(). 
     */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"WaveTable") == VERSION2) {
	char *str;
	NXReadType(aTypedStream,"*",&str);
	if (str) {
	    MKNameObject(str,self);
	    NX_FREE(str);
	}
    }
    return self;
}

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

- init
/* This method is ordinarily invoked only when an 
   instance is created. 
   A subclass should send [super init] if it overrides this 
   method. */ 
{
    if (dataDSP) {NX_FREE(dataDSP); dataDSP = NULL; }
    if (dataDouble) {NX_FREE(dataDouble); dataDouble = NULL; }
    length = 0;
    scaling = 0.0;
    return self;
}
 
- copyFromZone:(NXZone *)zone
  /* Copies the receiver, setting all cached data arrays to NULL. 
     The scaling and length are copied from the receiver. */
{
    WaveTable *newObj = [super copyFromZone:zone];
    newObj->dataDSP = NULL;
    newObj->dataDouble = NULL;
    return newObj;
}

- copy
{
    return [self copyFromZone:[self zone]];
}

- free
/* Frees cached data arrays then sends [super free].
   It also removes the name, if any, from the Music Kit name table. */
{
    if (dataDSP) 
      NX_FREE(dataDSP);
    if (dataDouble) 
      NX_FREE(dataDouble);
    MKRemoveObjectName(self);
    return [super free];
}

- (int)length
/* Returns the length in samples of the cached data arrays.  If it is 0,
   neither the DSPDatum nor real buffer has been allocated nor computed. */
{
    return length;
}
 
- (double)scaling
/* Scaling returns the current scaling of the data buffers. A value of 0
   indicates normalization scaling. */
{
    return scaling;
}

#define NEEDTOCOMPUTE ((length != aLength) || (scaling != aScaling) || \
		       (length == 0))

- (DSPDatum *)dataDSPLength:(int)aLength scale:(double)aScaling
/* Returns the WaveTable as an array of DSPDatums, recomputing 
   the data if necessary at the requested scaling and length. If the 
   subclass has no data, returns NULL. The data should neither be modified
   nor freed by the sender. */
{
   if (NEEDTOCOMPUTE)
     if (![self fillTableLength:aLength scale:aScaling])
       return NULL;
   if (!dataDSP && dataDouble) {
	_MK_MALLOC(dataDSP, DSPDatum, length);
	if (!dataDSP) return NULL;
	doubleToFix24Array (dataDouble, dataDSP, length);
	} 
   return dataDSP;
}

- (double *)dataDoubleLength:(int)aLength scale:(double)aScaling
/* Returns the WaveTable as an array of doubles, recomputing 
   the data if necessary at the requested scaling and length. If the 
   subclass has no data, returns NULL. The data should neither be modified
   nor freed by the sender. */
{  
   if (NEEDTOCOMPUTE)
     if (![self fillTableLength:aLength scale:aScaling])
       return NULL;
   if (!dataDouble && dataDSP) {
	_MK_MALLOC (dataDouble, double, length);
	if (!dataDouble) return NULL;
	fix24ToDoubleArray (dataDSP, dataDouble, length);
	} 
   return dataDouble;
}

- fillTableLength:(int)aLength scale:(double)aScaling 
/* This method is a subclass responsibility.

   This method computes the data. It allocates or reuses either (or 
   both) of the data arrays with the specified length and fills it with data, 
   appropriately scaled. 

   If only one of the data arrays is computed, the other should be freed
   and its pointer set to NULL. If data cannot be computed, 
   nil should be returned with both arrays freed and their pointers set to 
   NULL. 
*/
{
    return [self subclassResponsibility:_cmd];
}

- (DSPDatum *)dataDSP
/* Returns the WaveTable as an array of DSPDatums
   with the current length and scaling, computing the data if it has
   not been computed yet. Returns NULL if the subclass cannot compute the
   data.  You should neither alter nor free the data. */
{
    return [self dataDSPLength:length scale:scaling];
}

- (double *)dataDouble
/* Returns the WaveTable as an array of doubles, 
   with the current length and scaling, computing the data if it has
   not been computed yet. Returns NULL if the subclass cannot compute the
   data.  You should neither alter nor free the data. */
{
    return [self dataDoubleLength:length scale:scaling];
}

- (DSPDatum *)dataDSPLength:(int)aLength
/* Returns the WaveTable as an array of DSPDatums, recomputing 
   the data if necessary to make the array the requested length.
   Returns NULL if the subclass cannot compute the data.
   You should neither alter nor free the data. */
{
    return [self dataDSPLength:aLength scale:scaling];
}

- (double *)dataDoubleLength:(int)aLength
/* Returns the WaveTable as an array of doubles, recomputing 
   the data if necessary to make the array the requested length.
   Returns NULL if the subclass cannot compute the data.
   You should neither alter nor free the data. */
{
    return [self dataDoubleLength:aLength scale:scaling];
}

- (DSPDatum *)dataDSPScale:(double)aScaling
/* Returns the WaveTable as an array of DSPDatums, recomputing 
   the data if necessary with the requested scaling. 
   Returns NULL if the subclass cannot compute the data.
   You should neither alter nor free the data. */
{
    return [self dataDSPLength:length scale:aScaling];
}

- (double *)dataDoubleScale:(double)aScaling
/* Returns the WaveTable as an array of doubles, recomputing 
   the data if necessary with the requested scaling.
   Returns NULL if the subclass cannot compute the data.
   You should neither alter nor free the data. */
{
    return [self dataDoubleLength:length scale:aScaling];
}

@end

