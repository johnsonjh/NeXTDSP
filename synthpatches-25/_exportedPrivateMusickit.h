/* 
  Modification history:

  10/06/89/daj - Created from _musickit.h.

*/
extern id _MKErrorf(int errorCode, ...); 
    /* Calling sequence like printf, but first arg is error code instead of
       formating info and the formating info is derived from MKGetErrStr(). 
       It's the caller's responsibility
       that the expansion of the arguments using sprintf doesn't
       exceed the size of the error buffer (MK_ERRLEN). Fashions the 
       error message and sends it to MKError(). */

#import <musickit/Partials.h>

@interface Partials(Private)

-_writeBinaryScorefileStream:(NXStream *)aStream;
- _setPartialNoCopyCount: (int)howMany
  freqRatios: (short *)fRatios
  ampRatios: (float *)aRatios
  phases: (double *)phs
  orDefaultPhase: (double)defPhase;

@end
