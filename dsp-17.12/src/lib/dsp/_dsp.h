 /*
	_dsp.h

	Copyright 1988, NeXT, Inc.
  
	This file contains definitions, typedefs, and forward declarations
	used by libdsp functions.

	Modification history
	07/01/88/jos - prepended '/' to all RELATIVE file names
		       in case DSP environment variable has no trailing '/'
	10/07/88/jos - Changed default DSP directory and filenames for release.
	12/12/89/jos - Changed _DSP_MACH_SEND_TIMEOUT from 0 to 100 ms.
	01/13/90/jos - Introduced MAPPED_ONLY_POSSIBLE macro for ifdef's.
	01/13/90/jos - Introduced SIMULATOR_POSSIBLE macro for ifdef's.
	01/13/90/jos - Introduced TRACE_POSSIBLE macro for ifdef's.
	01/13/90/jos - Removed puzzling '#include "DSPMessage.h"' at EOF (!?)
	04/23/90/jos - Added private "aux dsp structures" from dsp_structs.h.
*/
#ifndef _LIBDSP_
#define _LIBDSP_

#define OLD_DMA_PROTOCOL 0
#define MAPPED_ONLY_POSSIBLE 0
#define SIMULATOR_POSSIBLE 0
#define TRACE_POSSIBLE 0

#define NO_VPRINTF 1

#define REMEMBER(x) /* x */

/*** INCLUDE FILES ***/

#ifndef __FILE_HEADER__
#include <sys/file.h>
#endif	__FILE_HEADER__
/* For access(2) */
#define R_OK	4/* test for read permission */
#define W_OK	2/* test for write permission */
#define X_OK	1/* test for execute (search) permission */
#define F_OK	0/* test for presence of file */

#if 0
	*** NOTE *** math.h allocates 0.0 for use by the built-in 
  	functions such as log.  This allocation screws up the making 
        of the Global data section of a shared library. Thus, math.h
        cannot be included by DSPGlobals.c.
#endif

#ifndef _POLY9
#include <math.h>
#endif _POLY9

#ifndef isalpha
#include <ctype.h>
#endif isalpha

#include <sys/time.h>		/* DSPAwaitData(), DSPMessageGet() */
				/* DSPAwaitUnsignedReply() _DSPAwait*() */
extern int DSPDefaultTimeLimit;

/* {long tv_sec; long tv_usec;} */
extern struct timeval _DSPTenMillisecondTimer;
extern struct timeval _DSPOneMillisecondTimer;

/* NeXT include files */
#ifndef NX_MALLOC
#include <appkit/nextstd.h>
#endif NX_MALLOC
/* Override the one above for greater safety */
/* #define  NX_MALLOC( VAR, TYPE, NUM )				\ */
/*    ((VAR) = (TYPE *) malloc( (unsigned)(NUM)*sizeof(TYPE) ))	 */
#define	 DSP_MALLOC( VAR, TYPE, NUM ) \
  if(((VAR) = (TYPE *) malloc( (unsigned)(NUM)*sizeof(TYPE) )) == NULL) \
  _DSPError(-1,"malloc: insufficient memory");

/* DSP include files */

#ifndef DSP_H
#include "dsp.h"		/* main DSP header file */
#endif DSP_H

/* stuff from _dsputilities.h */
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
#define _DSP_PATH_DELIM '/'	/* don't ask */

#define _DSP_NDSPS 1	/* Number of DSP chips available. FIXME:Sense this  */

/* Host message type codes (OR'd with host message opcode) */
#define _DSP_HMTYPE_UNTIMED 0x880000 /* untimed host message (stack marker) */
#define _DSP_HMTYPE_TIMEDA  0x990000 /* absolutely timed host message */
#define _DSP_HMTYPE_TIMEDR  0xAA0000 /* relatively timed (delta) host message */

/*** FIXME: Flush in 1.1 ***/
#define _DSP_UNTIMED NULL		/* time-stamp NULL means "now" */

/*** SYSTEM TYPE DECLARATIONS NEEDED BY MACROS HEREIN ***/
extern char *getenv();

/*** PRIVATE GLOBAL VARIABLES ***/	/* defined in DSPGlobals.c */
extern int _DSPTrace;
extern int _DSPVerbose;
extern int DSPAPTimeLimit;
extern int _DSPErrorBlock;
extern int _DSPMessagesEnabled;
extern int _DSPMKWriteDataIsRunning;
extern double _DSPSamplingRate;
extern DSPFix48 _DSPTimeStamp0;

#define DSP_MAYBE_RETURN(x) if (DSPIsSimulated()) ; else return(x)

#define DSP_QUESTION(q) /* q */

/*************************** Trace bits ***************************/

#define DSP_TRACE_DSPLOADSPECREAD 1
#define DSP_TRACE_DSPLOADSPECWRITE 2
#define DSP_TRACE_DSPLNKREAD 4
#define DSP_TRACE_FIXUPS 8
#define DSP_TRACE_NOOPTIMIZE 16
#define DSP_TRACE__DSPMEMMAPREAD 32
#define DSP_TRACE__DSPRELOCATE 64
#define DSP_TRACE__DSPRELOCATEUSER 128
#define DSP_TRACE_DSP 256
#define DSP_TRACE_HOST_MESSAGES 256  /* Same as DSP_TRACE_DSP in DSPObject.c */
#define DSP_TRACE_SYMBOLS 512 /* Also def'd in dspmsg/_DSPMakeIncludeFiles.c */
#define DSP_TRACE_HOST_INTERFACE 1024
#define DSP_TRACE_BOOT 2048
#define DSP_TRACE_LOAD 4096
#define DSP_TRACE_UTILITIES 8192
#define DSP_TRACE_TEST 16384
#define DSP_TRACE_DSPWRITEC 32768
#define DSP_TRACE_TMQ 0x10000
#define DSP_TRACE_NOSOUND 0x20000
#define DSP_TRACE_SOUND_DATA 0x40000
#define DSP_TRACE_MALLOC 0x80000
#define DSP_TRACE_MEMDIAG 0x100000 /* DSPBoot.c */
#define DSP_TRACE_WRITE_DATA 0x200000 /* DSPObject.c */

/*************************** Mach-related defines **************************/
/* Mach time-outs are in ms */
#define _DSP_MACH_RCV_TIMEOUT 100
#define _DSP_MACH_DEADLOCK_TIMEOUT 100
#define _DSP_ERR_TIMEOUT 100
#define _DSP_MACH_SEND_TIMEOUT 100
#define _DSP_MACH_FOREVER 1000000000

/*** AUXILIARY DSP STRUCTURES ***/

/* DSP host-interface registers, as accessed in memory-mapped mode */
typedef volatile struct _DSPRegs {
	unsigned char icr;
	unsigned char cvr;
	unsigned char isr;
	unsigned char ivr;
	union {
		struct {
			unsigned char	pad;
			unsigned char	h;
			unsigned char	m;
			unsigned char	l;
		} rx;
		struct {
			unsigned char	pad;
			unsigned char	h;
			unsigned char	m;
			unsigned char	l;
		} tx;
	} data;
} DSPRegs;

typedef struct __DSPMemMap {	/* DSP memory map descriptor */
    /* NeXT MK and AP software makes use of relocation only within the USER 
       section at present. The GLOBAL and SYSTEM sections must be absolute. 
       However, the struct can easily be extended to multiple relocatable 
       sections: */
    int defaultOffsets[DSP_LC_NUM];/* START directive in .mem file: NOT USED */
    int userOffsets[DSP_LC_NUM];   /* SECTION USER in memory-map (.mem) file */
    int nOtherOffsets[DSP_LC_NUM]; /* number of other relocatable sections */
    int *otherOffsets[DSP_LC_NUM]; /* SECTION <whatever>: NOT USED */
				   /* _DSPMemMapRead() will complain if this
				      is needed (and won't malloc it) */
} _DSPMemMap;

#import "_libdsp.h"

#endif _LIBDSP_
