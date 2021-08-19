/* _libdsp.h - private functions in libdsp_s.a

	01/13/90/jos - Replaced _DSPMessage.h expansion by explicit import.
*/

#import "_DSPTransfer.h"
#import "_DSPObject.h"
#import "_DSPMachSupport.h"
#import "_DSPMessage.h"

/* ============================= _DSPRelocate.c ============================ */

extern _DSPMemMap *_DSPMemMapRead();

extern int _DSPReloc(DSPDataRecord *data, DSPFixup *fixups,
    int fixupCount, int *loadAddresses);
/* 
 * dataRec is assumed to be a P data space. Fixes it up in place. 
 * This is a private libdsp method used by _DSPSendUGTimed and
 * _DSPRelocate. 
 */

extern int _DSPRelocate();
extern int _DSPRelocateUser();

/* ============================= DSPControl.c ============================== */
extern int _DSPCheckMappedMode();
extern int _DSPEnterMappedMode();
extern int _DSPEnterMappedModeNoCheck();
extern int _DSPEnterMappedModeNoPing();
extern int _DSPExitMappedMode();
extern int _DSPReadSSI();
extern int _DSPSetSSICRA();
extern int _DSPSetSSICRB();
extern int _DSPSetStartTimed();
extern int _DSPSetTime();
extern int _DSPSetTimeFromInts();
extern int _DSPSineTest();
extern int _DSPStartTimed();
extern DSPDatum _DSPGetValue();

/* ============================= DSPReadFile.c ============================= */
extern char *_DSPFGetRecord();
extern int _DSPGetIntHexStr6();
extern int _DSPLnkRead();
extern char *_DSPAddSymbol();
extern int _DSPGetRelIntHexStr();
extern char *_DSPUniqueName();

/* ============================ DSPStructMisc.c ============================ */

int _DSPCheckingFWrite( int *ptr, int size, int nitems, FILE *stream);
int _DSPWriteString(char *str, FILE *fp);
int _DSPReadString(char **spp, FILE *fp);
int _DSPFreeString(char *str);
char *_DSPContiguousMalloc(unsigned size);
int _DSPContiguousFree(char *ptr);
void DSPMemMapInit(_DSPMemMap *mm);
void DSPMemMapPrint(_DSPMemMap *mm);

extern char *_DSPContiguousMalloc(unsigned size);
/*
 *	Same as malloc except allocates in one contiguous piece of
 *	memory.	 Calls realloc as necessary to extend the block.
 */


/* ============================ _DSPUtilities.c ============================ */
extern void _DSPErr();
extern char *_DSPFirstReadableFile(char *fn,...);
extern char *_DSPGetBody();
extern int _DSPGetDSPIntStr();
extern char _DSPGetField();
extern int _DSPGetFilter();
extern float _DSPGetFloatStr();
extern char *_DSPGetHead();
extern void _DSPGetInputFile();
extern void _DSPGetInputOutputFiles();
extern int _DSPGetIntHexStr();
extern int _DSPGetIntStr();
extern char *_DSPGetLineStr();
extern void _DSPGetOutputFile();
extern char *_DSPGetSN();
extern char *_DSPGetTail();
extern char *_DSPGetTokStr();
extern int _DSPInInt();
extern int _DSPIndexS();
extern char *_DSPIntToChar();
extern int *_DSPMakeArray();
extern FILE *_DSPMyFopen();
extern char *_DSPPadStr();
extern void _DSPParseName();
extern void _DSPPutFilter();
extern char *_DSPRemoveHead();
extern char *_DSPRemoveTail();
extern int _DSPSaveMatD();
extern int _DSPSezYes();
extern char *_DSPSkipToWhite();
extern char *_DSPSkipWhite();
extern DSP_BOOL _DSPGetFile();
extern DSPLocationCounter _DSPGetMemStr();
extern DSP_BOOL _DSPNotBlank();

/* ============================ DSPConversion.c ============================ */

DSPFix48 *_DSPDoubleIntToFix48UseArg(double dval,DSPFix48 *aFix48P);
/* 
 * The double is assumed to be between -2^47 and 2^47.
 *  Returns, in *aFix48P, the value as represented by dval. 
 *  aFix48P must point to a valid DSPFix48 struct. 
 */

/* ============================= _DSPError.c =============================== */

int _DSPCheckErrorFP(void);
/*
 * Check error file-pointer.
 * If nonzero, return.
 * If zero, open /tmp/dsperrors and return file-pointer for it.
 * Also, write DSP interlock info to dsperrors.
 */


int _DSPErrorV(int errorcode,char *fmt,...);


int _DSPError1(
    int errorcode,
    char *msg,
    char *arg);


int _DSPError(
    int errorcode,
    char *msg);


void _DSPFatalError(
    int errorcode,
    char *msg);


int _DSPMachError(
    int error,
    char *msg);


int _DSPCheckErrorFP(void);
/*
 * Check error file-pointer.
 * If nonzero, return.
 * If zero, open /tmp/dsperrors and return file-pointer for it.
 * Also, write DSP interlock info to dsperrors.
 */


int _DSPErrorV(int errorcode,char *fmt,...);


int _DSPError1(
    int errorcode,
    char *msg,
    char *arg);


int _DSPError(
    int errorcode,
    char *msg);


void _DSPFatalError(
    int errorcode,
    char *msg);


int _DSPMachError(
    int error,
    char *msg);

/* ============================== _DSPCV.c ================================= */

char *_DSPCVAS(
    int n,			/* number to be converted */
    int fmt);			/* 0=decimal, 1=hex format */
/* 
 * Convert integer to decimal or hex string 
 */


char *_DSPCVS(int n);
/* 
 * Convert integer to decimal string 
 */


char *_DSPCVHS(int n);
/* 
 * Convert integer to hex string 
 */


char *_DSPCVDS(float d);
/* 
 * Convert double to hex string 
 */


char *_DSPCVFS(float f);
/* 
 * Convert float to hex string 
 */


char *_DSPIntToChar(int i);
/* 
 * Convert digit between 0 and 9 to corresponding character.
 */

/* ============================ _DSPString.c =============================== */

char *_DSPNewStr(int size);
/*
 * Create string of given total length in bytes.
 */


char *_DSPMakeStr(
    int size,			/* size = total length incl \0 */
    char *init);		/* initialization string */
/* 
 * create new string initialized by given string.
 */


char *_DSPCat(
    char *f1,
    char *f2);
/*
 * Concatenate two strings 
 */


char *_DSPReCat(
    char *f1,
    char *f2);
/*
 * append second string to first via realloc 
 */


char *_DSPCopyStr(char *s);
/*
 * Copy string s into freshly malloc'd storage.
 */


char *_DSPToLowerStr(
    char *s);			/* input string = output string */
/*
 * Convert all string chars to lower case.
 */


char *_DSPToUpperStr(
    char *s);			/* input string = output string */
/*
 * Convert all string chars to upper case.
 */


