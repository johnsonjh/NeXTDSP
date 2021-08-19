/*  Modification history:

    daj/04/23/90 - Created from _musickit.h 

*/

#define _MK_ERRMSG static char * 

#define _MK_ERRLEN 2048

#import "errors.h"

extern const char * _MKGetErrStr(int errCode);
    /* Returns the error string for the given code or "unknown error" if
       the code is not one of the  MKErrno enums. 
       The string is not copied. Note that some of the strings have printf-
       style 'arguments' embeded. Thus care must be taken in writeing them. */
extern char *_MKErrBuf();
extern id _MKErrorf(int errorCode, ...); 
    /* Calling sequence like printf, but first arg is error code instead of
       formating info and the formating info is derived from MKGetErrStr(). 
       It's the caller's responsibility
       that the expansion of the arguments using sprintf doesn't
       exceed the size of the error buffer (MK_ERRLEN). Fashions the 
       error message and sends it to MKError(). */

