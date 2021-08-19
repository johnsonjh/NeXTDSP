#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Envelope.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  01/07/90/daj - Changed comments and flushed false conditional comp.
  01/09/90/daj - Fixed minor bug in setStickPoint: -- error checking wasn't
		 correct.
  03/19/90/daj - Added MKSetEnvelopeClass() and MKGetEnvelopeClass().
  03/21/90/daj - Added archiving.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  08/27/90/daj - Changed to zone API.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things

*/
#define MK_INLINE 1
#import "_musickit.h"
#import "_scorefile.h"

#import "Envelope.h"
@implementation  Envelope:Object
{
    double defaultSmoothing;    /* If no Smoothing-array, this is time constant. */
    double samplingPeriod;	/* If no X-array, this is abcissa scale */
    double *xArray;             /* Array of x values, if any. */
    double *yArray;	        /* Arrays of data values */
    double *smoothingArray;           /* Array of time constants. */
    int stickPoint;		/* Index of "steady-state", if any */
    int pointCount;		/* Number of points in envelope */
    void *_reservedEnvelope1;
}
/* The Envelope class suports envelopes.

 */

#define aSound _eReserved1 /* For when I add "data envelopes". */

/* Julius sez:

Tau has a precise meaning.  It is the "time constant".  During tau seconds
you get 1/e of the way to where you're going, or about .37th of the way.
Setting it to 0.2 means you are going 5 time constants in 1 second.  You
determined empirically that 5 or 6 time constants "really gets there".
Still, it is an exponential approach that never quite reaches its target.
The formula

	exp(-t/tau) = epsilon

Can be solved for t to find out how many time constants will get you
to within epsilon times the inital distance.  For example, epsilon = 0.001
gives t around 7.  This means it takes 7 time constants to traverse 99.999%
of the distance.

If we redefine the meaning of tau, we have to give it another name, such
as "relaxation time".  We can never call it the time constant.

That's why we call it "smoothing".
*/

+  new
  /* This is how you make up an empty seg envelope */
{
    self = [super allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Attempt to avoid breaking old programs. */
    return self;
}

#define VERSION2 2

+initialize
{
    if (self != [Envelope class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

-initialize 
  /* For backwards compatibility. See below. */
{ 
    return self;
} 

-init
{
    stickPoint = MAXINT;
    samplingPeriod = 1.0;
    defaultSmoothing = MK_DEFAULTSMOOTHING;
    return self;
}

static void putArray(int pointCount,NXTypedStream *aTypedStream,double *arr)
{
    BOOL aBool;
    if (arr) {
	aBool = YES;
	NXWriteType(aTypedStream,"c",&aBool);
	NXWriteArray(aTypedStream,"d", pointCount, arr);
    } else {
	aBool = NO;
	NXWriteType(aTypedStream,"c",&aBool);
    }
}

static void getArray(int pointCount,NXTypedStream *aTypedStream,
		     double **arrPtr)
{
    BOOL aBool;
    NXReadType(aTypedStream,"c",&aBool);
    if (aBool) {
	double *arr; /* We do it like this because read: can be called 
			  multiple times. */
	_MK_MALLOC(arr,double,pointCount);
	NXReadArray(aTypedStream,"d", pointCount, arr);
	if (!*arrPtr)
	  *arrPtr = arr;
	else NX_FREE(arr);
    } 
}

- write:(NXTypedStream *) aTypedStream
{
    char *str;
    [super write:aTypedStream];
    str = (char *)MKGetObjectName(self);
    NXWriteTypes(aTypedStream,"ddii*",&defaultSmoothing,&samplingPeriod,
		 &stickPoint,&pointCount,&str);
    putArray(pointCount,aTypedStream,xArray);
    putArray(pointCount,aTypedStream,yArray);
    putArray(pointCount,aTypedStream,smoothingArray);
    return self;
}

- read:(NXTypedStream *) aTypedStream
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Envelope") == VERSION2) {
	char *str;
	NXReadTypes(aTypedStream,"ddii*",&defaultSmoothing,&samplingPeriod,
		    &stickPoint,&pointCount,&str);
	if (str) {
	    MKNameObject(str,self);
	    NX_FREE(str);
	}
	getArray(pointCount,aTypedStream,&xArray);
	getArray(pointCount,aTypedStream,&yArray);
	getArray(pointCount,aTypedStream,&smoothingArray);
    }
    return self;
}

static id theSubclass = nil;

BOOL MKSetEnvelopeClass(id aClass)
{
    if (!_MKInheritsFrom(aClass,[Envelope class]))
      return NO;
    theSubclass = aClass;
    return YES;
}

id MKGetEnvelopeClass(void)
{
    if (!theSubclass)
      theSubclass = [Envelope class];
    return theSubclass;
}

-copyFromZone:(NXZone *)zone
  /* Returns a copy of the receiver with its own copy of arrays. */
{
    Envelope *newObj = [super copyFromZone:zone];
    newObj->xArray = NULL;
    newObj->yArray = NULL;
    newObj->smoothingArray = NULL;
    [newObj setPointCount:pointCount xArray:xArray orSamplingPeriod:
     samplingPeriod  yArray:yArray smoothingArray:smoothingArray orDefaultSmoothing:defaultSmoothing];
    [newObj setStickPoint:stickPoint];
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

- (int)	pointCount
  /* Returns the number of points in the Envelope. */
{
    return pointCount;
}

-free
    /* Frees self. Removes the name, if any, from the name table. */
{
    if (xArray != NULL)
	NX_FREE(xArray);
    if (yArray != NULL)
	NX_FREE(yArray);
    if (smoothingArray != NULL)
	NX_FREE(smoothingArray);
    MKRemoveObjectName(self);
    return [super free];
}

-(double)defaultSmoothing
  /* If the defaultSmoothing is used (no smoothing points) returns the time constant,
     else MK_NODVAL */
{
    if (smoothingArray) 
      return MK_NODVAL;
    return defaultSmoothing;
}

- (int)	stickPoint
  /* Returns stickPoint or MAXINT if none */
{
    return stickPoint;
}

- setStickPoint:(int) sp
  /* Sets stick point. A stick point of MAXINT means no stick point. 
     Returns nil if sp is out of bounds, else self. Stick point is 0-based. */
{
    if ((sp == MAXINT) || (sp >= 0 && sp < pointCount)) {
	stickPoint = sp;
	return self;
    }
    return nil;
}

-  setPointCount:(int)n 
 xArray:(double *) xPtr 
 yArray:(double *)yPtr 
{
    return [self setPointCount:n 
	  xArray:xPtr
	  orSamplingPeriod:samplingPeriod /* Old value */
	  yArray:yPtr
	  smoothingArray:smoothingArray /* old value */
	  orDefaultSmoothing:defaultSmoothing]; /* old value */
}

-  setPointCount:(int)n 
 xArray:(double *) xPtr 
 orSamplingPeriod:(double)period 
 yArray:(double *)yPtr 
 smoothingArray:(double *)smoothingPtr 
 orDefaultSmoothing:(double)smoothing
  /* Allocates arrays and fills them with values.
     xP or smoothingP may be NULL, in which case the corresponding constant
     value is used. If yP is NULL, the y values are unchanged. */
{
    if (yPtr) {
	if (yArray != NULL)
	  NX_FREE(yArray);
	_MK_MALLOC(yArray,double,n);
	memmove( yArray,yPtr, n * sizeof(double)); /* Copy yPtr to yArray */
    }
    if (xPtr == NULL)
      samplingPeriod = period;
    else  {
	if (xArray != NULL)
	  NX_FREE(xArray);
	_MK_MALLOC(xArray,double,n);
	memmove(xArray, xPtr, n * sizeof(double));
    }
    if (smoothingPtr == NULL)
	defaultSmoothing = smoothing;
    else {
	if (smoothingArray != NULL)
	  NX_FREE(smoothingArray);
	_MK_MALLOC(smoothingArray,double,n);
	memmove(smoothingArray, smoothingPtr, n * sizeof(double));
    }
    pointCount = n;
    return self;
}

- (double)samplingPeriod
  /* If the samplingPeriod is used (no X points) returns the time step, 
     else MK_NODVAL. */
{
    if (xArray != NULL)
      return MK_NODVAL;
    else return samplingPeriod;
}

- (double *)smoothingArray
  /* Returns a pointer to the array of smoothing values or NULL if there are none. 
     The array is not copied. */
{
    if ((pointCount <= 0) && (smoothingArray == NULL)) 
      return NULL;
    return smoothingArray;
}

-(double)releaseDur
{
    if (((stickPoint == MAXINT) || (stickPoint > pointCount - 1)))
      return 0;
    if (xArray) 
      return xArray[pointCount - 1] - xArray[stickPoint];  
    return samplingPeriod * ((pointCount - 1) - stickPoint);
}

-(double)attackDur
{
    int highPt = (((stickPoint == MAXINT) || (stickPoint > pointCount - 1)) ?
		  (pointCount - 1) : stickPoint);
    if (xArray)
      return xArray[highPt] - xArray[0];
    return highPt * samplingPeriod;
}

- (double *)xArray
  /* Returns a pointer to the array of x values or NULL if there are none. 
     The array is not copied. */
{
    if ((pointCount <= 0) && (xArray == NULL)) 
      return NULL;
    return xArray;
}

- (double *)yArray
  /* Returns a pointer to the array of x values or NULL if there are none. 
     The array is not copied. */
{
    if ((pointCount <= 0) && (yArray == NULL)) 
      return NULL;
    return yArray;
}

MKEnvStatus _MKGetEnvelopeNth(Envelope *self,int n,double *xPtr,double *yPtr,
			      double *smoothingPtr)
    /* Private function. Assumes valid n and valid envelope. Used by
       AsympUG. */
{
	*xPtr = (self->xArray) ? self->xArray[n] : (n * self->samplingPeriod);
	*yPtr = self->yArray[n]; 
	*smoothingPtr = ((self->smoothingArray) ? self->smoothingArray[n] :
			 self->defaultSmoothing);
	return ((n == (self->pointCount-1)) ? MK_lastPoint :
		(n == self->stickPoint) ? MK_stickPoint : 
		MK_noEnvError);
}

- (MKEnvStatus)	getNth:(int)n x:(double *)xPtr y:(double *)yPtr 
 smoothing:(double *)smoothingPtr
  /* Get Nth point of X and Y. 
     If the point is the last point, MK_lastPoint. Otherwise,
     if the point is the stickpoint, MK_stickPoint is returned.
     If the point is out of bounds, returns MK_noMorePoints. 
     If some other error occurs, returns MK_noMorePoints.
     Otherwise, returns MK_noEnvError. */
{
	if ((n < 0) || (n >= pointCount) || (!yArray) || 
	    ((!xArray) && (samplingPeriod == 0)))
  	    return MK_noMorePoints;
	return _MKGetEnvelopeNth(self,n,xPtr,yPtr,smoothingPtr);
}


-writeScorefileStream:(NXStream *)aStream
  /* Writes on aStream the following:
     Writes itself in the form:
     (0.0, 0.0, 1.0)(1.0,2.0,1.0)|(1.0,1.0,1.0)
     Returns nil if yArray is NULL, otherwise self. */
{
    int i;
    double xVal;
    double *yP,*xP,*smoothingP;
    if (yArray == NULL) {
	NXPrintf(aStream,"[/* Empty envelope. */]");
	return nil;
    }
    if (xArray == NULL) 
      for (i = 0, xVal = 0, yP = yArray, smoothingP = smoothingArray; i < pointCount; xVal += samplingPeriod) {
	  if (smoothingP == NULL)
	    if (i == 0) 
	      NXPrintf(aStream,"(%.5f, %.5f, %.5f)",xVal,*yP++,
		      defaultSmoothing);
	    else
	      NXPrintf(aStream,"(%.5f, %.5f)",xVal,*yP++);
	  else
	    NXPrintf(aStream,"(%.5f, %.5f, %.5f)",xVal,*yP++,*smoothingP++);
	  if (i == stickPoint)
	    NXPrintf(aStream," | ");
#         if _MK_LINEBREAKS	  
	  if ((++i % 5 == 0) && i < pointCount)
	    NXPrintf(aStream,"\n\t"); 
#         else
	  i++;
#         endif
      }
    else 
      for (i = 0, xP = xArray, yP = yArray, smoothingP = smoothingArray; i < pointCount; ) {
	  if (smoothingP == NULL)
	    if (i == 0)
	      NXPrintf(aStream,"(%.5f, %.5f, %.5f)",*xP++,*yP++,defaultSmoothing);
	    else
	      NXPrintf(aStream,"(%.5f, %.5f)",*xP++,*yP++);
	  else NXPrintf(aStream,"(%.5f, %.5f, %.5f)",*xP++,*yP++,*smoothingP++);
	  if (i == stickPoint)
	    NXPrintf(aStream," | ");
#         if _MK_LINEBREAKS	  
	  if ((++i % 5 == 0) && i < pointCount)
	    NXPrintf(aStream,"\n\t"); 
#         else
	  ++i;
#         endif
      }
    return self;
}

-_writeBinaryScorefileStream:(NXStream *)aStream
  /* Writes on aStream the following:
     Writes itself in the form:
     (0.0, 0.0, 1.0)(1.0,2.0,1.0)|(1.0,1.0,1.0)
     Returns nil if yArray is NULL, otherwise self. */
{
    int i;
    double xVal;
    double *yP,*xP,*smoothingP;
    if (yArray == NULL) {
	_MKWriteChar(aStream,'\0');
	return nil;
    }
    if (xArray == NULL) 
      for (i = 0, xVal = 0, yP = yArray, smoothingP = smoothingArray; 
	   i < pointCount; xVal += samplingPeriod) {
	  if (smoothingP == NULL) {
	      _MKWriteChar(aStream,(i == 0) ? '\3' : '\2');
	      _MKWriteDouble(aStream,xVal);
	      _MKWriteDouble(aStream,*yP++);
	      if (i == 0) 
		_MKWriteDouble(aStream,defaultSmoothing);
	  }
	  else {
	      _MKWriteChar(aStream,'\3');
	      _MKWriteDouble(aStream,xVal);
	      _MKWriteDouble(aStream,*yP++);
	      if (i == 0) 
		_MKWriteDouble(aStream,*smoothingP++);
	  }
	  if (i == stickPoint)
	    _MKWriteChar(aStream,'\1');
	  i++;
      }
    else 
      for (i = 0, xP = xArray, yP = yArray, smoothingP = smoothingArray; 
	   i < pointCount; ) {
	  if (smoothingP == NULL) {
	      _MKWriteChar(aStream,(i == 0) ? '\3' : '\2');
	      _MKWriteDouble(aStream,*xP++);
	      _MKWriteDouble(aStream,*yP++);
	      if (i == 0) 
		_MKWriteDouble(aStream,defaultSmoothing);
	  }
	  else {
	      _MKWriteChar(aStream,'\3');
	      _MKWriteDouble(aStream,*xP++);
	      _MKWriteDouble(aStream,*yP++);
	      _MKWriteDouble(aStream,*smoothingP++);
	  }
	  if (i == stickPoint)
	    _MKWriteChar(aStream,'\1');
	  ++i;
      }
    _MKWriteChar(aStream,'\0');
    return self;
}


-(double)lookupYForX:(double)xVal
  /* Returns, the value at xVal. xVal need not be an 
     actual point of the receiver. If xVal is out of bounds, the 
     beginning (or ending) Y point is returned in *rtnVal. If an error 
     occurs, returns MK_NODVAL */
{
    if (yArray == NULL)
      return MK_NODVAL;
    if (xArray == NULL) {
	if (xVal >= ((pointCount - 1) * samplingPeriod))
	  return yArray[pointCount - 1];
	if (xVal <= 0.0)
	  return *yArray;
	else {
	    int intPart;
	    double fractPart,doubleStep;
	    doubleStep = xVal / samplingPeriod;
	    intPart = (int)doubleStep;
	    fractPart = doubleStep - intPart;
	    return yArray[intPart] + (yArray[intPart + 1] - yArray[intPart]) * fractPart;
	}
    }
    else {
	double *xTmp,*xEnd;
	for (xTmp = xArray, xEnd = xArray + pointCount - 1;
	     *xTmp < xVal && xTmp < xEnd;
	     xTmp++)
	  ;
	if (xTmp == xArray)             /* xVal too small */
	  return *yArray;
	if (*xTmp < xVal) /* xVal too big */
	  return yArray[pointCount - 1]; 
	else {                     /* xVal just right */
	    int i = xTmp - xArray;   
	    double nextX,prevX,nextY,prevY;
	    nextX = *xTmp;
	    prevX = *(xTmp - 1);
	    nextY = yArray[i];
	    prevY = yArray[i - 1];
	    return prevY + ((xVal - prevX)/(nextX - prevX)) * 
	      (nextY - prevY);
	}
    }
    return MK_NODVAL;
}

@end

















































