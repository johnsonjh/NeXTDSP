/* This file represents that portion of the Music Kit's private interface
   that the UnitGenerator library depends on. */
/* 
  Modification history:  

  10/12/89/daj - Created.
  11/12/89/daj - Added _MKClassSamples() and _MKClassPartials().
  11/22/89/daj - Added _MKGetEnvelopeNth().
  11/26/89/daj - Added _MKBeginUGBlock() and _MKEndUGBlock().
*/

extern id _MKErrorf(int errorCode, ...); 
    /* Calling sequence like printf, but first arg is error code instead of
       formating info and the formating info is derived from MKGetErrStr(). 
       It's the caller's responsibility
       that the expansion of the arguments using sprintf doesn't
       exceed the size of the error buffer (MK_ERRLEN). Fashions the 
       error message and sends it to MKError(). */

typedef struct __MKClassLoaded { 
    id aClass;
    BOOL alreadyChecked;
} _MKClassLoaded;

extern _MKClassLoaded _MKEnvelopeClass;
extern _MKClassLoaded _MKSamplesClass;
extern _MKClassLoaded _MKPartialsClass;
extern _MKClassLoaded _MKConductorClass;

extern id _MKCheckClassEnvelope() ;
extern id _MKCheckClassConductor();
extern id _MKCheckClassSamples();
extern id _MKCheckClassPartials();

#define _MKClassSamples() \
  ((_MKSamplesClass.alreadyChecked) ? _MKSamplesClass.aClass : \
  _MKCheckClassSamples())

#define _MKClassPartials() \
  ((_MKPartialsClass.alreadyChecked) ? _MKPartialsClass.aClass : \
  _MKCheckClassPartials())

#define _MKClassEnvelope() \
  ((_MKEnvelopeClass.alreadyChecked) ? _MKEnvelopeClass.aClass : \
  _MKCheckClassEnvelope())

#define _MKClassConductor() \
  ((_MKConductorClass.alreadyChecked) ? _MKConductorClass.aClass : \
  _MKCheckClassConductor())

extern DSPFix24 _MKDoubleToFix24(double dval);
extern void _MKBeginUGBlock(id anOrch);
extern void _MKEndUGBlock(void);

extern MKEnvStatus _MKGetEnvelopeNth(id self,int n,double *xPtr,double *yPtr,
				     double *smoothingPtr);

#import <objc/Object.h>
@interface musickitprivatemsgs:Object
+(MKMsgStruct *) _cancelMsgRequest:(MKMsgStruct *)aMsgStructPtr;
+(MKMsgStruct *) _newMsgRequestAtTime:(double )timeOfMsg sel:(SEL )whichSelector to:destinationObject argCount:(int )argCount, ...;
-(void ) _scheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr;
-(MKMsgStruct *) _rescheduleMsgRequest:(MKMsgStruct *)aMsgStructPtr atTime:(double )timeOfNewMsg sel:(SEL )whichSelector to:destinationObject argCount:(int )argCount, ...;
@end


