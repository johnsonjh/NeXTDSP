#ifdef SHLIB
#include "shlib.h"
#endif

/* 
Modification history:

  09/15/89/daj - Changed to use new fastFft.
  10/02/89/daj - Fixed scaling bug (bug 3670)
  11/20/89/daj - Enabled new fastFft (it was off!). 
  03/09/90/daj - Changed getPartial:... to return 2 if last point 
                 (was returning a bogus enum)
  03/13/90/daj - Moved private method to category
  03/19/90/daj - Added MKGet/SetPartialsClass()
  03/21/90/daj - Added archiving.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/17/90/daj - Added private ability to make amps be floats and freqs be
		 ints to support the timbre data base. Note that archiving, 
		 copying, interpolateBetween: and other such methods are not 
		 supported. The likelyhood of someone calling this methods 
		 on a data base WaveTable is very low.  On the off chance that
		 someone does call them, I added code to convert the object to
		 normal form. However, if someone has a subclass and tries to 
		 access the instance vars directly, he'll get garbage. The
		 likelyhood of this is so low that I'm not worrying about it.
		 Finally, note that you can't mix the float/int form with the 
		 double/double form.  Conclusion:
		 The int/float form is just a hack now. It should be cleaned 
		 up and made public eventually, but we're past the API freeze
		 for 2.0 now.
  08/27/90/daj - Changed to zone API.
*/

#import "_musickit.h"
#import "_scorefile.h"
#import "_Partials.h"

@implementation  Partials:WaveTable
/* Partials, a subclass of Wave, accepts a set of arrays containing
   the amplitude and frequency ratios and initial phases of a set of partials
   representing a waveform.  If one of the getData methods is called 
   (inherited from the Wave object), a wavetable is additively synthesized 
   and returned. 
   
   By "frequency ratios", we mean that when this object is passed to a unit 
   generator, the resulting component frequencies of the waveform will be 
   these numbers times the unit generator's overall frequency value.  
   Similarly, the resulting component amplitudes will be the "amplitude 
   ratios" times the unit generator's overall amplitude term.  */

{
    double *ampRatios, *freqRatios;
    double *phases; 
    int partialCount;			   /* Number of points in each array */
    double defaultPhase;          /* If no phase-array, this is phase */
    double minFreq, maxFreq;      /* optional freq range for this timbre */
    BOOL _reservedPartials1,_reservedPartials2,_reservedPartials3;
    void *_reservedPartials4;
}
#define _ampArrayFreeable _reservedPartials1
#define _freqArrayFreeable _reservedPartials2
#define _phaseArrayFreeable _reservedPartials3
#define _extraVars _reservedPartials4

/* This struct is for instance variables added after the 1.0 instance variable
   freeze. */
typedef struct __extraInstanceVars {
    BOOL dbMode;
} _extraInstanceVars;

#define _dbMode(_self) ((_extraInstanceVars *)_self->_extraVars)->dbMode

#define NORMALFORM(_self) if (_dbMode(_self)) normalform

static void normalform(id obj)
    /* Assumes we've got an obj in dbMode */
{
    register int i;
    double *nFreqs,*nAmps;
    _MK_MALLOC(nFreqs,double,obj->partialCount);
    _MK_MALLOC(nAmps,double,obj->partialCount);
    for (i=0; i<obj->partialCount; i++) {
	nFreqs[i] = (double)(((short *)(obj->freqRatios))[i]);
	nAmps[i] = (double)(((float *)(obj->ampRatios))[i]);
    }
    _dbMode(obj) = NO;
    if (obj->_ampArrayFreeable)
      NX_FREE(obj->ampRatios);
    if (obj->_freqArrayFreeable)
      NX_FREE(obj->freqRatios);
    obj->ampRatios = nAmps;
    obj->freqRatios = nFreqs;
    obj->_freqArrayFreeable = obj->_ampArrayFreeable = YES;
}

static id theSubclass = nil;

BOOL MKSetPartialsClass(id aClass)
{
    if (!_MKInheritsFrom(aClass,[Partials class]))
      return NO;
    theSubclass = aClass;
    return YES;
}

id MKGetPartialsClass(void)
{
    if (!theSubclass)
      theSubclass = [Partials class];
    return theSubclass;
}

#define VERSION2 2

+initialize
{
    if (self != [Partials class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

-  init
/* Init frees the data arrays if they have been
   allocated, sets defaultPhase to 0, and calls [super init]. 
   This is invoked when a new object is created. */
{
  [super init]; 
  if (ampRatios && _ampArrayFreeable) 
    {NX_FREE(ampRatios); ampRatios = NULL; _ampArrayFreeable = NO;}
  if (freqRatios && _freqArrayFreeable) 
    {NX_FREE(freqRatios); freqRatios = NULL; _freqArrayFreeable = NO;}
  if (phases && _phaseArrayFreeable) 
    {NX_FREE(phases); phases = NULL; _phaseArrayFreeable = NO;}
  defaultPhase = 0.0;
  if (!_extraVars)
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
  else _dbMode(self) = NO;
  return self;
}

static void putArray(int partialCount,NXTypedStream *aTypedStream,double *arr)
{
    BOOL aBool;
    if (arr) {
	aBool = YES;
	NXWriteType(aTypedStream,"c",&aBool);
	NXWriteArray(aTypedStream,"d", partialCount, arr);
    } else {
	aBool = NO;
	NXWriteType(aTypedStream,"c",&aBool);
    }
}

static void getArray(int partialCount,NXTypedStream *aTypedStream,BOOL *aBool,
		     double **arrPtr)
{
    NXReadType(aTypedStream,"c",aBool);
    if (*aBool) {
	double *arr; /* We do it like this because read: can be called 
			  multiple times. */
	_MK_MALLOC(arr,double,partialCount);
	NXReadArray(aTypedStream,"d", partialCount, arr);
	if (!*arrPtr)
	  *arrPtr = arr;
	else NX_FREE(arr);
    } 
}

- write:(NXTypedStream *) aTypedStream
{
    NORMALFORM(self);
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"iddd",&partialCount,&defaultPhase,&minFreq,
		 &maxFreq);
    putArray(partialCount,aTypedStream,ampRatios);
    putArray(partialCount,aTypedStream,freqRatios);
    putArray(partialCount,aTypedStream,phases);
    return self;
}

- read:(NXTypedStream *) aTypedStream
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Partials") == VERSION2) {
	NXReadTypes(aTypedStream,"iddd",&partialCount,&defaultPhase,&minFreq,
		    &maxFreq);
	getArray(partialCount,aTypedStream,&_ampArrayFreeable,&ampRatios);
	getArray(partialCount,aTypedStream,&_freqArrayFreeable,&freqRatios);
	getArray(partialCount,aTypedStream,&_phaseArrayFreeable,&phases);
    }
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
    return self;
}

-copyFromZone:(NXZone *)zone
  /* Returns a copy of the receiver with its own copy of arrays. */
{
    Partials *newObj = [super copyFromZone:zone];
    NORMALFORM(self);
    newObj->ampRatios = NULL;
    newObj->freqRatios = NULL;
    newObj->phases = NULL;
    [newObj setPartialCount:partialCount freqRatios:freqRatios
   ampRatios:ampRatios phases:phases orDefaultPhase:defaultPhase];
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
    return newObj;
}

-  free
  /* Frees the instance object and all its arrays. */
{
  if (ampRatios && _ampArrayFreeable) 
    {NX_FREE(ampRatios); ampRatios = NULL; _ampArrayFreeable = NO;}
  if (freqRatios && _freqArrayFreeable) 
    {NX_FREE(freqRatios); freqRatios = NULL; _freqArrayFreeable = NO;}
  if (phases && _phaseArrayFreeable) 
    {NX_FREE(phases); phases = NULL; _phaseArrayFreeable = NO;}
  NX_FREE(_extraVars);
  return [super free];
}

- setPartialCount:	(int)howMany
  freqRatios: (double *)fRatios
  ampRatios: (double *)aRatios
  phases: (double *)phs
  orDefaultPhase: (double)defPhase
  /* This method is used to specify the amplitude and frequency
     ratios and initial phases of a set of partials representing a
     waveform.  If one of the getData methods is called (inherited from 
     the Wave object), a wavetable is additively synthesized and returned.
     In this case, the frequency ratios must not have fractional parts.  
     
     The initial phases are specified in degrees.
     If phs is NULL, the defPhase value is used for all harmonics. 
     If aRatios or fRatios is NULL, the corresponding value is 
     unchanged. The array arguments are copied. */
{
  if (fRatios) {
    if (freqRatios && _freqArrayFreeable) 
      {NX_FREE(freqRatios); freqRatios = NULL; _freqArrayFreeable = NO;}
    _MK_MALLOC(freqRatios,double,howMany);
    if (freqRatios) {
      memmove(freqRatios, fRatios, howMany * sizeof(double));
      _freqArrayFreeable = YES;
    }
  }
  if (aRatios) {
    if (ampRatios && _ampArrayFreeable) 
      {NX_FREE(ampRatios); ampRatios = NULL; _ampArrayFreeable = NO;}
    _MK_MALLOC(ampRatios,double,howMany);
    if (ampRatios) {
      memmove(ampRatios, aRatios, howMany * sizeof(double));
      _ampArrayFreeable = YES;
    }
  }
  if (phs == NULL)
    defaultPhase = defPhase;
  else {
    if (phases && _phaseArrayFreeable) 
      {NX_FREE(phases); phases = NULL; _phaseArrayFreeable = NO;}
    _MK_MALLOC(phases,double,howMany);
    if (phases) {
      memmove(phases, phs, howMany * sizeof(double));
      _phaseArrayFreeable = YES;
    }
  }
  partialCount = howMany;
  length = 0;   /* This ensures a recomputation of the tables. */
  _dbMode(self) = NO;
  return self;
}

#ifndef ABS
#define ABS(_x) ((_x >= 0) ? _x : - _x )
#endif

-interpolateBetween:partials1 :partials2 ratio:(double)value
  /* Assign frequencies and amplitudes to the receiver by interpolating
     between the values in partials1 and partials2. If value is 0,
     you get the values in partials1. If value is 1, you get the values
     in partials2. If either partials1 or partials2 has no amplitude array or 
     no frequency array, the receiver is not affected and nil is returned. 
     The phases of partials1 and partials2 are not used and the phases
     of the receiver, if any, are discarded. */
{
    double *freqs1 = [partials1 freqRatios];  
    double *freqs2 = [partials2 freqRatios];
    double *amps1 = [partials1 ampRatios];  
    double *amps2 = [partials2 ampRatios];
    int np1 = [partials1 partialCount];
    int np2 = [partials2 partialCount];
    double *end1 = freqs1 + np1;
    double *end2 = freqs2 + np2;
    double *freqs;
    double *amps;
    if (!(freqs1 && freqs2 && amps1 && amps2)) 
      return nil;
    if (freqRatios && _freqArrayFreeable) 
      NX_FREE(freqRatios); 
    if (ampRatios && _ampArrayFreeable) 
      NX_FREE(ampRatios); 
    if (phases && _phaseArrayFreeable) {
	NX_FREE(phases); 
	phases = NULL; 
	_phaseArrayFreeable = NO;
    }
    partialCount = np1 + np2; /* Worst case partial count. */
    _MK_MALLOC(freqRatios,double,partialCount);
    _MK_MALLOC(ampRatios,double,partialCount);
    _freqArrayFreeable = YES;
    _ampArrayFreeable = YES;
    freqs = freqRatios;
    amps = ampRatios;
    while ((freqs1 < end1) || (freqs2 < end2)) {
	if ((freqs1 < end1) && (freqs2 < end2) &&  
	    (ABS(*freqs1 - *freqs2) < .001)) { /* The same freq ratio? */
	    *freqs++ = *freqs1++;
	    freqs2++;
	    *amps++ = *amps1 + (*amps2++ - *amps1) * value;
	    amps1++;
	}
	else if ((freqs1 < end1) && 
		 ((freqs2 == end2) || (*freqs1 < *freqs2))) {
	    *freqs++ = *freqs1++;
	    *amps++ = *amps1++ * (1 - value);
	}
	else {
	    *freqs++ = *freqs2++;
	    *amps++ = *amps2++ * value;
	}
    }
    partialCount = freqs - freqRatios;
    length = 0;   /* This ensures a recomputation of the tables. */
    return self;
}

- (int)partialCount
  /* Returns the number of values in the harmonic data arrays. */
{
    return partialCount;
}

- (double)defaultPhase
  /* Returns phase constant. */
{
    return defaultPhase;
}

- (double *)freqRatios
/* Returns the frequency ratios array directly, without copying it. */
{
    NORMALFORM(self);
    return freqRatios;
}

- (double *)ampRatios
/* Returns the amplitude ratios array directly, without copying it nor 
   scaling it. */
{
    NORMALFORM(self);
    return ampRatios;
}

- (double *)phases
/* Returns the initial phases array directly, without copying it. */
{
    return phases;
}

- (int) getPartial: (int)n
        freqRatio: (double *)fRatio
	ampRatio: (double *)aRatio
        phase: (double *)phs
  /* Get Nth value. 
     If the value is the last value, returns 2. 
     If the value is out of bounds, returns -1. Otherwise returns 0.
     The value is scaled by the scale constant, if non-zero.
     */
{
    NORMALFORM(self);
    if ((n < 0 || n >= partialCount) || (!freqRatios) || (!ampRatios))
      return -1;
    *aRatio = ampRatios[n] * (scaling ? scaling : 1.0); 
    *fRatio = freqRatios[n];
    if (phases == NULL)
      *phs = defaultPhase;
    else
      *phs = phases[n];
    return ((n == partialCount-1) ? 2 : 0);
}

#define DEFAULTTABLELENGTH 256
#define DEG_TO_RADIANS(_aPhase) (_aPhase * (.002777777777777778 * M_PI * 2))

#define TIME_FFT 0

#if TIME_FFT 
#import "timeStamps.m"
#endif

#import "fastFft.c"

static BOOL isPowerOfTwo(int n)
    /* Query whether n is a pure power of 2 */
{
    while (n > 1) {
	if (n % 2) break;
	n >>= 1;
    }
    return (n == 1);
}

- fillTableLength:(int)aLength scale:(double)aScaling 
  /* Computes the wavetable from the data provided by the
     setN: method.  Returns self, or nil if an error is found. If 
     scaling is 0.0, the waveform is normalized. This method is sent
     automatically if necessary by the various getData: methods 
     (inherited from the Wave class) used to access the resulting
     wavetable. The resulting waveform is guaranteed to begin and end 
     at or near the same point only if the partials are harmonically 
     related. */
{
    int i; 
    double cosPhase;
    double sinPhase;
    double tmp;
    register double *dataEnd,*dataPtr;
    int indexVal,halfLength;

#   if TIME_FFT
    _timeStamp();
#   endif

    if (!ampRatios || !freqRatios || (partialCount <= 0))
      return nil;
    if (aLength == 0)
      if (length == 0)
	aLength = DEFAULTTABLELENGTH;
      else aLength = length;
    if (!isPowerOfTwo(aLength)) {
	MKError("Partials currently supports table sizes of powers of 2 only.");
	/*** FIXME ***/
	return nil;
    }
    if (!dataDouble || (length != aLength)) {
    	if (dataDouble) {
	    NX_FREE(dataDouble); 
	    dataDouble = NULL;
	}
	_MK_CALLOC(dataDouble, double, aLength);
    }
    length = aLength;
    if (dataDSP) {NX_FREE(dataDSP); dataDSP = NULL;}
    halfLength = length / 2;
    bzero(dataDouble,length * sizeof(double));
    if (!phases) {
	cosPhase = cos(DEG_TO_RADIANS(defaultPhase)-M_PI_2) * .5; 	
	sinPhase = sin(DEG_TO_RADIANS(defaultPhase)-M_PI_2) * .5; 	
	/* We subtract M_PI_2 so that a zero phase means sine and a PI/2 
	   phase means cosine. */
    }
    if (_dbMode(self))
      for (i = 0; i<partialCount; i++) { 	
	  indexVal = ((short *)freqRatios)[i];
	  if (indexVal == 0) {
	      /* Value at n=0 must be real */
	      dataDouble[indexVal] = ((float *)ampRatios)[i];
	      dataDouble[length - indexVal] = 0;
	  } else if (indexVal < halfLength) {
	      if (phases) {
		  tmp = DEG_TO_RADIANS(phases[i])-M_PI_2;
		  cosPhase = cos(tmp) * .5;
		  sinPhase = sin(tmp) * .5;
	      }
	      dataDouble[indexVal] = ((float *)ampRatios)[i] * cosPhase;
	      dataDouble[length - indexVal] = ((float *)ampRatios)[i] *  sinPhase;
	  }
      }
    else for (i = 0; i<partialCount; i++) { 	
	indexVal = freqRatios[i];
	if (indexVal == 0) {
	    /* Value at n=0 must be real */
	    dataDouble[indexVal] = ampRatios[i];
	    dataDouble[length - indexVal] = 0;
	} else if (indexVal < halfLength) {
	    if (phases) {
		tmp = DEG_TO_RADIANS(phases[i])-M_PI_2;
		cosPhase = cos(tmp) * .5;
		sinPhase = sin(tmp) * .5;
	    }
	    dataDouble[indexVal] = ampRatios[i] * cosPhase;
	    dataDouble[length - indexVal] = ampRatios[i] *  sinPhase;
	}
    }
    fftinv_hermitian_to_real(dataDouble,length);
    scaling = aScaling;
    if (scaling == 0.0) { /* Normalize */
	for (dataPtr = dataDouble, dataEnd = dataDouble + length;
	     dataPtr < dataEnd; dataPtr++) 
	  if ((tmp = ABS(*dataPtr)) > aScaling) 
	    aScaling = tmp;
	aScaling = 1.0/aScaling;
    }
    if (aScaling != 1.0) 
      for (dataPtr = dataDouble, dataEnd = dataDouble + length; 
	   dataPtr < dataEnd;)
	*dataPtr++ = *dataPtr * aScaling;

#   if TIME_FFT
    _timeStamp();
#   endif

    return self;
}

-writeScorefileStream:(NXStream *)aStream
  /* Writes on aStream the following:
     {1.0, 0.3, 0.0}{2.0,.1,0.0}{3.0,.01,0.0}
     Returns nil if ampRatios or freqRatios is NULL, otherwise self. */
{
    int i;
    double *aRatios,*fRatios,*phs;
    NORMALFORM(self);
    if ((freqRatios == NULL) || (ampRatios == NULL)) {
	NXPrintf(aStream,"{1.0,0,0}");
	return nil;
    }
    i = 0; 
    fRatios = freqRatios;
    aRatios = ampRatios; 
    phs = phases; 
    while (i < partialCount) {
	if (phs == NULL)
	  if (i == 0)
	    NXPrintf(aStream,"{%.5f,%.5f,%.5f}",*fRatios++,*aRatios++,
		     defaultPhase);
	  else
	    NXPrintf(aStream,"{%.5f, %.5f}",*fRatios++,*aRatios++);
	else NXPrintf(aStream,"{%.5f, %.5f,%.5f}",*fRatios++,*aRatios++,
		      *phs++);
#       if _MK_LINEBREAKS
	if ((++i % 5 == 0) && i < partialCount)
	  NXPrintf(aStream,"\n\t");
#       else
	i++;
#       endif
    }
    return self;
}


-setFreqRangeLow:(double)freq1 high:(double)freq2
  /* Sets the frequency range associated with this timbre. */
{
  minFreq = freq1;
  maxFreq = freq2;
  return self;
}

-(double)maxFreq
  /* Returns the maximum fundamental frequency at which this timbre is
     ordinarily used. */
{
  return maxFreq;
}

-(double)minFreq
  /* Returns the minimum fundamental frequency at which this timbre is
     ordinarily used. */
{
  return minFreq;
}

-(BOOL)freqWithinRange:(double)freq
  /* Returns YES if freq is within the range of fundamental frequencies 
     ordinarily associated with this timbre. */
{
  return ((minFreq <= freq) && (freq <= maxFreq));
}

-(double)highestFreqRatio
  /* Returns the highest (i.e., largest absolute value) freqRatio.  
     Useful for optimizing lookup table sizes. */
{
  int i;
  double ratio, maxRatio = 0;
  for (i=0; i<partialCount; i++) {
    if (((ratio = ABS(freqRatios[i])) > maxRatio) && (ampRatios[i] != 0.0))
      maxRatio = ratio;
  }
  return maxRatio;
}
    

@end


@implementation Partials(Private)

-_writeBinaryScorefileStream:(NXStream *)aStream
  /* Writes on aStream the following:
     {1.0, 0.3, 0.0}{2.0,.1,0.0}{3.0,.01,0.0}
     Returns nil if ampRatios or freqRatios is NULL, otherwise self. */
{
    int i;
    double *aRatios,*fRatios,*phs;
    _MKWriteChar(aStream,'\0'); /* Marks it as a partials rather than samples 
				 */
    if ((freqRatios == NULL) || (ampRatios == NULL)) {
	_MKWriteChar(aStream,'\2');
	_MKWriteDouble(aStream,1.0);
	_MKWriteDouble(aStream,1.0);
	return nil;
    }
    i = 0; 
    fRatios = freqRatios;
    aRatios = ampRatios; 
    phs = phases; 
    while (i < partialCount) {
	if (phs == NULL) {
	    _MKWriteChar(aStream,(i == 0) ? '\3' : '\2');
	    _MKWriteDouble(aStream,*fRatios++);
	    _MKWriteDouble(aStream,*aRatios++);
	    if (i == 0)
	      _MKWriteDouble(aStream,defaultPhase);
	}
	else {
	    _MKWriteChar(aStream,'\3');
	    _MKWriteDouble(aStream,*fRatios++);
	    _MKWriteDouble(aStream,*aRatios++);
	    _MKWriteDouble(aStream,*phs++);
	}
	i++;
    }
    _MKWriteChar(aStream,'\0');
    return self;
}


- _setPartialNoCopyCount: (int)howMany
  freqRatios: (short *)fRatios
  ampRatios: (float *)aRatios
  phases: (double *)phs
  orDefaultPhase: (double)defPhase
  /* Same as setPartialCount:freqRatios:ampRatios:phases:orDefaultPhase
     except that the array arguments are not copied or freed. */
{
  if (fRatios) {
    if (freqRatios && _freqArrayFreeable) 
      {NX_FREE(freqRatios); freqRatios = NULL;}
    freqRatios = (double *)fRatios;
    _freqArrayFreeable = NO;
  }
  if (aRatios) {
    if (ampRatios && _ampArrayFreeable) 
      {NX_FREE(ampRatios); ampRatios = NULL;}
    ampRatios = (double *)aRatios;
    _ampArrayFreeable = NO;
  }
  if (phs == NULL)
    defaultPhase = defPhase;
  else {
    if (phases && _phaseArrayFreeable) 
      {NX_FREE(phases); phases = NULL;}
    phases = phs;
    _phaseArrayFreeable = NO;
  }
  partialCount = howMany;
  length = 0;   /* This ensures a recomputation of the tables. */
  ((_extraInstanceVars *)_extraVars)->dbMode = YES;
  return self;
}

@end

