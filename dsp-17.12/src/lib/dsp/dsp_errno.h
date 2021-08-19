/*
	dsperrno.h

	Copyright 1989,1990 NeXT, Inc.
  
	This file contains globally unique error codes for the DSP C library.

*/
#ifndef EPERM
#include <sys/errno.h>
#endif	EPERM
extern int errno;

#define DSP_EWARNING 0		/* used to print warning and continue */

#define DSPKIT_ERRORS 6000	/* global error codes for DSP kit */
#define DSP_ERRORS    7000	/* global error codes for DSP C library*/

#define DSP_EBADLA		(DSP_ERRORS+1) /* bad load address */
#define DSP_EBADDR		(DSP_ERRORS+2) /* bad data record */
#define DSP_EBADFILETYPE	(DSP_ERRORS+3) /* bad file type */
#define DSP_EBADSECTION		(DSP_ERRORS+4) /* bad section */
#define DSP_EBADLNKFILE		(DSP_ERRORS+5) /* bad link file */
#define DSP_EBADLODFILE		(DSP_ERRORS+6) /* bad link file */
#define DSP_ETIMEOUT		(DSP_ERRORS+7) /* time out */
#define DSP_EBADSYMBOL		(DSP_ERRORS+8) /* bad symbol */
#define DSP_EBADFILEFORMAT	(DSP_ERRORS+9) /* bad file format */
#define DSP_EBADMEMMAP		(DSP_ERRORS+10) /* invalid DSP memory map */


#define DSP_EMISC		(DSP_ERRORS+11) /* miscellaneous error */
#define DSP_EPEOF		(DSP_ERRORS+12) /* premature end of file */
#define DSP_EPROTOCOL		(DSP_ERRORS+13) /* DSP communication trouble */
#define DSP_EBADRAM		(DSP_ERRORS+14) /* DSP private RAM broken */
#define DSP_ESYSHUNG		(DSP_ERRORS+15) /* DSP system not responding */
#define DSP_EBADDSPFILE		(DSP_ERRORS+16) /* bad .dsp file */
#define DSP_EILLDMA		(DSP_ERRORS+17) /* attempt to write p:$20#2 */
#define DSP_ENOMSG		(DSP_ERRORS+18) /* no DSP messages to read */
#define DSP_EBADMKLC		(DSP_ERRORS+19) /* lc not used by musickit */
#define DSP_EBADVERSION		(DSP_ERRORS+20) /* DSP sys version mismatch */
#define DSP_EDSP		(DSP_ERRORS+21) /* DSP error code */
#define DSP_EILLADDR		(DSP_ERRORS+22) /* Attempt to overwrite sys */
#define DSP_EHWERR		(DSP_ERRORS+23) /* Apparent hardware problem */
#define DSP_EFPOVFL		(DSP_ERRORS+24) /* 24b Fixed-point Overflow */
#define DSP_EHMSOVFL		(DSP_ERRORS+25) /* Host Message Stack Overf. */
#define DSP_EMACH		(DSP_ERRORS+26) /* Error says Mach kernel */
#define DSP_EUSER		(DSP_ERRORS+27) /* User error code */
#define DSP_EABORT		(DSP_ERRORS+28) /* DSP aborted execution */
