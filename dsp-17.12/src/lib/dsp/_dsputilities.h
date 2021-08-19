
NOBODY SHOULD BE INCLUDING THIS NOW.  IT WAS ABSORBED INTO _dsp.h


/*
	_dsputilities.h

	Copyright 1988, NeXT, Inc.
  
	This file contains definitions, typedefs, and forward declarations
	used by _DSPUtilities functions.

*/

#ifndef _DSPUTILITIES_
#define _DSPUTILITIES_

#ifndef FILE
#include <stdio.h>
#endif FILE

#ifndef NX_MALLOC
#include <appkit/nextstd.h>
#endif NX_MALLOC

#ifndef isalpha
#include <ctype.h>
#endif isalpha

#define _DSP_MAX_LINE 256	/* max chars per line on input (via fgets) */
#define _DSP_MAX_NAME 80	/* Allow for long paths (getfil.c, geti.c) */
#define _DSP_MAX_CMD 12		/* Max no. chars in a command (indexs.c) */

#define _DSP_EXPANDSIZE 512	/* must be even (for l: data in _DSPLnkRead) */
#define _DSP_EXPANDSMALL 10	/* This one can be left small */

#define _DSP_NULLC 0		/* Null character */
#define _DSP_NOT_AN_INT (0x80000000)
#define _DSP_PATH_DELIM '/'		/* don't ask */

/* ~carl/josprogs */
extern char *_DSPGetSN();
extern char *_DSPGetBody();
extern char *_DSPGetHead();
extern char *_DSPGetTail();
extern char *_DSPIntToChar();
extern char *_DSPRemoveHead();
extern char *_DSPRemoveTail();
extern int *_DSPMakeArray();
extern void _DSPErr();
extern void _DSPGetInputFile();
extern void _DSPGetInputOutputFiles();
extern void _DSPGetOutputFile();
extern void _DSPParseName();
extern void _DSPPutFilter();

/* new since '87 */
extern char *_DSPSkipWhite();
extern char *_DSPSkipToWhite();
extern char *_DSPPadStr();
extern DSP_BOOL _DSPNotBlank();
extern char *_DSPGetLineStr();
extern int _DSPGetIntStr();
extern int _DSPGetIntHexStr();
extern float _DSPGetFloatStr();
extern char *_DSPGetTokStr();
extern FILE *_DSPMyFopen();
extern char *_DSPMakeStr();
extern char *_DSPFirstReadableFile(char *fn,...);

#endif _DSPUTILITIES_

