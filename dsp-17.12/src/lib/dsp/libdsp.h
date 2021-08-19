/* libdsp.h - external declarations for functions in libdsp_s.a 
 * Copyright (c) 1989 by NeXT Inc.
 *
 * Note that libdsp_s.a depends only on libsys_s.a.
 *
 * Functions with prefix DSPMK work only in conjunction with the 
 * DSP Music Kit Monitor "/usr/lib/dsp/monitor/mkmon8k.dsp", 
 * obtained via DSPMKInit().
 *
 * Functions with prefix DSPAP (see libarrayproc.h) work in conjunction 
 * with the array processing monitor "apmon8k.dsp", obtained via DSPAPInit().
 * 
 * Functions with prefix DSP work with EITHER the array processing or 
 * Music Kit monitors.
 * 
 * Each function returns an integer error code (0 = success, nonzero = error),
 * unless its name is of one of the following forms:
 *
 * 	(1) "DSPGet<v>()" 
 *		Returns <v>, where <v> can be any scalar C type.
 *		Example: rx = DSPGetRX() reads the DSP RX register.
 *
 * 	(2) "DSP<var>IsEnabled()" 
 *		Returns an open-state variable <var> (zero or nonzero).
 * 		Example: if (DSPErrorLogIsEnabled()) ...
 *
 * 	(3) "DSPIs<condition>()" 
 *		Returns <condition> (zero or nonzero).
 * 		Example: if (DSPIsAlive()) ...
 * 
 * For functions which return an error code, the exact nature 
 * of the error is written to the error log file, if it is enabled.
 * (See DSPError.h for error log enabling functions.)
 */

#import <mach.h>

/*** FIXME: replace includes below by 

  #import "no_monitor.h"

  and require user to say

  #import "ap_monitor.h"

  or

  #import "mk_monitor.h"

  to get the rest
*/

#import "DSPControl.h"
#import "DSPMessage.h"
#import "DSPTransfer.h"
#import "DSPStructMisc.h"
#import "DSPConversion.h"
#import "DSPObject.h"
#import "DSPError.h"

/* ============================= DSPLoad.c ================================= */

extern int DSPLoadFile(char *fn);
/*
 * Load DSP from the file specified.
 * Equivalent to DSPReadFile followed by DSPLoad.
 *
 */

extern int DSPLoad(DSPLoadSpec *dspimg);	
/*
 * Load everything in *dspimg to the DSP.
 */


/* =============================== DSPBoot.c =============================== */

extern int DSPBootFile(char *fn);
/*
 * Boot DSP from the file specified.
 * Equivalent to DSPReadFile followed by DSPBoot.
 */


extern int DSPBoot(DSPLoadSpec *system);
/* 
 * Load DSP bootstrap program.
 * DSPBoot closes the DSP if it is open, resets it, and feeds the
 * resident monitor supplied in the struct 'system' to the bootstrapping DSP.
 * If system is NULL, the default resident monitor is supplied to the DSP.
 * On return, the DSP is open.
 */


/* ============================= DSPReadFile.c ============================= */
extern int DSPReadFile(
    DSPLoadSpec **dsppp,
    char *fn);
/*
 * Read in a DSP file (as produced by the assembler in absolute mode).
 * It looks in the system-wide .dsp directory for the given file if 
 * the user's working directory does not contain a readable version of 
 * the file (.lnk, .lod, or .dsp).
 */

/* ================================ _DSPCV.c =============================== */

extern char *DSPFix24ToStr(DSPFix24 datum);
/* 
 * Convert type DSPFix24 to fractional fixed-point string 
 */


extern char *DSPFix48ToSampleStr(DSPFix48 *aTimeStampP);
/* 
 * Convert type DSPFix48 to fractional time-stamp string in samples
 */


extern char *DSPTimeStampStr(DSPFix48 *aTimeStampP);
/* 
 * Convert type DSPFix48 to fractional time-stamp string in samples
 * or the string "<untimed>" if null, or the string "at end of current tick"
 * if time-stamp is zero.
 */

/* ============================= _DSPString.c ============================== */

extern char *DSPCat(char *f1, char *f2);
/* 
 * Concatenate two strings into the returned string.
 * Uses malloc to create the returned string.
 */

/* libdsp is by J. O. Smith */


