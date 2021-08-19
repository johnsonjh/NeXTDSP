/*  Modification history:

    daj/04/23/90 - Created from _musickit.h 
    08/13/90/daj - Added _MKParNameStr().

*/

typedef union __MKParameterUnion {
    /* Used for Parameters and scoreFile parsing. */
    id symbol;        /* This type is needed for scorefile parsing.
			 Also used for storing envelopes and wavetables. */
    double rval;
    char * sval;
    int ival;
} _MKParameterUnion;

typedef struct __MKParameter 
{
    _MKParameterUnion _uVal;    /* Value. */
    short _uType;               /* Type of union. */
    short parNum;               /* Number of this parameter. */
}
_MKParameter;

#define _MK_FIRSTAPPPAR  (MK_MKPARBITVECTS * 32)

/* _Parameter and _ParName object 'methods' */
extern id _MKParNameObj(int aPar);
extern char *_MKParNameStr(int aPar);
extern const char *_MKUniqueNull();
extern BOOL _MKKeyNumPrintfunc();
extern BOOL _MKParInit();
extern _MKParameter *_MKNewStringPar();
extern _MKParameter *_MKNewDoublePar();
extern _MKParameter *_MKNewIntPar();
extern _MKParameter *_MKNewObjPar();
extern _MKParameter *_MKSetDoublePar();
extern double _MKParAsDouble();
extern _MKParameter *_MKSetIntPar();
extern int _MKParAsInt();
extern _MKParameter *_MKSetStringPar();
extern char *_MKParAsString();
extern _MKParameter *_MKSetObjPar();
extern id _MKParAsObj();
extern id _MKParAsEnv();
extern id _MKParAsWave();
extern _MKParameterUnion *_MKParRaw();
extern char * _MKParAsStringNoCopy();
extern BOOL _MKIsParPublic();
extern _MKParameter * _MKCopyParameter(_MKParameter *aPar);

#import "_scorefile.h"

extern void _MKParWriteStdValueOn(_MKParameter *rcvr,NXStream *aStream,
				  _MKScoreOutStruct *p);
extern void _MKParWriteOn(_MKParameter *rcvr,NXStream *aStream,
			  _MKScoreOutStruct *p);
extern void _MKParWriteValueOn(_MKParameter *rcvr,NXStream *aStream,
			  _MKScoreOutStruct *p);
extern unsigned _MKGetParNamePar(id aParName);
extern void _MKArchiveParOn(_MKParameter *param,NXTypedStream *aTypedStream);
extern void _MKUnarchiveParOn(_MKParameter *param,NXTypedStream *aTypedStream);
extern int _MKHighestPar();
extern id  _MKDummyParameter();
extern BOOL _MKIsPar(unsigned aPar);
extern BOOL _MKIsPrivatePar(unsigned aPar);

typedef enum __MKPrivPar {
    _MK_dur = ((int)MK_privatePars + 1),
    _MK_maxPrivPar /* Must be <= MK_appPars */
}
_MKPrivPar;

extern _MKParameter *_MKFreeParameter(_MKParameter *param);


