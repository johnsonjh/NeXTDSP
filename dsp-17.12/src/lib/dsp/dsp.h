/*
	dsp.h - master include file for the DSP library (-ldsp_s -lsys_s)

	Copyright 1989,1990 by NeXT, Inc.
 
*/

#ifndef DSP_H
#define DSP_H

/* UNIX include files */
#ifndef FILE
#include <stdio.h>
#endif FILE

#ifndef LIBC_H
#include <libc.h>
#endif LIBC_H

#define DSP_EXT_RAM_SIZE 8192	/* External RAM size in 3byte words */
#define DSP_CLOCK_RATE (25.0E6) /* DSP clock frequency */
#define DSP_CLOCK_PERIOD (40.0E-9) /* Cycle time in seconds. */

/* Numeric typedefs */
typedef int DSPMuLaw;
typedef int DSPFix8;
typedef int DSPFix16;
typedef int DSPFix24;
typedef struct _DSPFix48 {
    int high24;		      /* High order 24 bits, right justified */
    int low24;		      /* Low order 24 bits, right justified */
} DSPFix48;

typedef DSPFix16 DSPAddress;
typedef DSPFix24 DSPDatum;
typedef DSPFix48 DSPLongDatum;
typedef DSPFix48 DSPTimeStamp;

#define DSP_I_EPS		0x000001
#define DSP_I_M12DBU16		0x000c66
#define DSP_I_M12DBU24		0x0c6666
#define DSP_I_MAXPOS		0x7fffff
#define DSP_I_MINPOS		0x000001
#define DSP_I_ONEHALF		0x400000
#define DSP_I_EPS		0x000001
#define DSP_I_M12DBU16		0x000c66
#define DSP_I_M12DBU24		0x0c6666
#define DSP_I_MAXPOS		0x7fffff
#define DSP_I_MINPOS		0x000001
#define DSP_I_ONEHALF		0x400000

typedef int DSP_BOOL;
#define DSP_TRUE 1
#define DSP_FALSE 0
#define DSP_NOT_SET 2
#define DSP_MAYBE (-2)		/* TRUE and FALSE defined in nextstd.h */
#define DSP_UNKNOWN (-1)	/* like DSP_{MAYBE,NOT_SET} for adresses */


/****************************** Masks **************************************/

/* Basic DSP data types */
#define DSP_WORD_MASK		0x00FFFFFF /* low-order 24 bits */
#define DSP_SOUND_MASK		0x0000FFFF /* low-order 16 bits */
#define DSP_ADDRESS_MASK	0x0000FFFF /* low-order 16 bits */


/* Bits in the DSP Interrupt Control Register (ICR) */
#define DSP_ICR			0 /* ICR address in host interface */
#define DSP_ICR_RREQ		0x00000001 /* enable host int. on data->host */
#define DSP_ICR_TREQ		0x00000002 /* enable host int. on data<-host */
#define DSP_ICR_UNUSED		0x00000004
#define DSP_ICR_HF0		0x00000008
#define DSP_ICR_HF1		0x00000010
#define DSP_ICR_HM0		0x00000020
#define DSP_ICR_HM1		0x00000040
#define DSP_ICR_INIT		0x00000080

#define DSP_ICR_REGS_MASK	0xFF000000
#define DSP_ICR_RREQ_REGS_MASK	0x01000000
#define DSP_ICR_TREQ_REGS_MASK	0x02000000
#define DSP_ICR_HF0_REGS_MASK	0x08000000
#define DSP_ICR_HF1_REGS_MASK	0x10000000
#define DSP_ICR_HM0_REGS_MASK	0x20000000
#define DSP_ICR_HM1_REGS_MASK	0x40000000
#define DSP_ICR_INIT_REGS_MASK	0x80000000


/* Bits in the DSP Command Vector Register */
#define DSP_CVR			1	   /* address in host interface */
#define DSP_CVR_HV_MASK		0x0000001F /* low-order	 5 bits of CVR */
#define DSP_CVR_HC_MASK		0x00000080 /* HC bit of DSP CVR */

#define DSP_CVR_REGS_MASK	0x00FF0000 /* Regs mask for CVR */
#define DSP_CVR_HV_REGS_MASK	0x001F0000 /* low-order	 5 bits of CVR */
#define DSP_CVR_HC_REGS_MASK	0x00800000 /* Regs mask for HC bit of CVR */


/* Bits in the DSP Interrupt Status Register */
#define DSP_ISR		     2	/* address in host interface */
#define DSP_ISR_RXDF		0x00000001
#define DSP_ISR_TXDE		0x00000002
#define DSP_ISR_TRDY		0x00000004
#define DSP_ISR_HF2		0x00000008
#define DSP_BUSY		0x00000008 /* "DSP Busy"=HF2 */
#define DSP_ISR_HF3		0x00000010
#define DSP_ISR_UNUSED		0x00000020
#define DSP_ISR_DMA		0x00000040
#define DSP_ISR_HREQ		0x00000080

#define DSP_ISR_REGS_MASK	0x0000FF00
#define DSP_ISR_RXDF_REGS_MASK	0x00000100
#define DSP_ISR_TXDE_REGS_MASK	0x00000200
#define DSP_ISR_TRDY_REGS_MASK	0x00000400
#define DSP_ISR_HF2_REGS_MASK	0x00000800
#define DSP_BUSY_REGS_MASK	0x00000800
#define DSP_ISR_HF3_REGS_MASK	0x00001000
#define DSP_ISR_DMA_REGS_MASK	0x00004000
#define DSP_ISR_HREQ_REGS_MASK	0x00008000

/* DSP Interrupt Vector Register */
#define DSP_IVR		     3	/* address in host interface */

#define DSP_UNUSED	     4	/* address in host interface */

/* DSP Receive-Byte Registers */
#define DSP_RXH		     5	/* address in host interface */
#define DSP_RXM		     6	/* address in host interface */
#define DSP_RXL		     7	/* address in host interface */

/* DSP Transmit-Byte Registers */
#define DSP_TXH		     5	/* address in host interface */
#define DSP_TXM		     6	/* address in host interface */
#define DSP_TXL		     7	/* address in host interface */

/* Interesting places in DSP memory */
#define DSP_MULAW_SPACE 1	/* memory space code for mulaw table in DSP */
#define DSP_MULAW_TABLE 256	/* address of mulaw table in X onchip memory */
#define DSP_MULAW_LENGTH 128	/* length  of mulaw table */

#define DSP_ALAW_SPACE 1	/* memory space code for Mu-law table in DSP */
#define DSP_ALAW_TABLE 384	/* address of A-law table */
#define DSP_ALAW_LENGTH 128	/* length  of A-law table */

#define DSP_SINE_SPACE 2	/* memory space code for sine ROM in DSP */
#define DSP_SINE_TABLE 256	/* address of sine table in Y onchip memory */
#define DSP_SINE_LENGTH 256	/* length  of sine table */

/* Host commands (cf. DSP56000/1 DSP User's Manual) */
/* To issue a host command, set CVR to (DSP_CVR_HC_MASK|<cmd>&DSP_CVR_HV_MASK) */

#define DSP_HC_RESET			 (0x0)	   /* RESET host command */
#define DSP_HC_TRACE			 (0x4>>1)  /* TRACE host command */
#define DSP_HC_SWI			 (0x6>>1)  /* SWI host command */
#define DSP_HC_SOFTWARE_INTERRUPT	 (0x6>>1)  /* SWI host command */
#define DSP_HC_ABORT			 (0x8>>1)  /* DEBUG_HALT */
#define DSP_HC_HOST_RD			 (0x24>>1) /* DMA read done */
#define DSP_HC_HOST_R_DONE		 (0x24>>1) /* DMA read done */
#define DSP_HC_EXECUTE_HOST_MESSAGE	 (0x26>>1) /* Used for host messages */
#define DSP_HC_XHM			 (0x26>>1) /* abbreviated version */
#define DSP_HC_DMAWT			 (0x28>>1) /* Terminate DMA write */
#define DSP_HC_DMA_HOST_W_DONE		 (0x28>>1) /* Terminate DMA write */
#define DSP_HC_HOST_W_DONE		 (0x28>>1) /* Terminate DMA write */
#define DSP_HC_KERNEL_ACK		 (0x2A>>1) /* Kernel acknowledge */
#define DSP_HC_EXECUTE			 (0x2C>>1) /* Execute two code words */
#define DSP_HC_XCT			 (0x2C>>1) /* Execute two code words */

#define DSP_MESSAGE_OPCODE(x) (((x)>>16)&0xFF)
#define DSP_MESSAGE_SIGNED_DATUM(x) \
	((int)((x)&0x8000?0xFFFF0000|((x)&0xFFFF):((x)&0xFFFF)))
#define DSP_MESSAGE_UNSIGNED_DATUM(x) ((x)&0xFFFF)
#define DSP_MESSAGE_ADDRESS(x) DSP_MESSAGE_UNSIGNED_DATUM(x)
#define DSP_ERROR_OPCODE_INDEX(x) ((x) & 0x7F)	 /* Strip MSB on 8-bit field */
#define DSP_IS_ERROR_MESSAGE(x) ((x) & 0x800000) /* MSB on in 24 bits */
#define DSP_IS_ERROR_OPCODE(x) ((x) & 0x80) /* MSB on in 8 bits */

#define DSP_START_ADDRESS DSP_PLI_USR /* cf. dsp_messages.h */

/* Be sure to enclose in {} when followed by an else */
#define DSP_UNTIL_ERROR(x) if (DSPErrorNo=(x)) \
  return(_DSPError(DSPErrorNo,"Aborting"))

/**** Include files ****/

#import "dsp_structs.h"		/* DSP struct declarations */
#import "dsp_errno.h"		/* Error codes for DSP C functions */
#import "dsp_messages.h"	/* DSP messages and host messages to DSP */
#import "dsp_memory_map_mk.h"	/* Music Kit Monitor memory map constants */
#import "dsp_memory_map_ap.h"	/* Array Processing Monitor memory map */
#import "libdsp.h"		/* Function prototypes for libdsp functions */

/* DSP System version and revision codes */
#if (DSPAP_SYS_VER != DSPMK_SYS_VER || DSPAP_SYS_REV != DSPMK_SYS_REV)
	*** VERSIONITIS! ***
	MK and AP system versions must be kept identical because
	there is shared functionality between the two DSP monitors.
#else
#	define DSP_SYS_VER DSPAP_SYS_VER
#	define DSP_SYS_REV DSPAP_SYS_REV
#endif

/* maximum number of elements writable to host message stack in DSP */
#if (DSPMK_NB_HMS!=DSPAP_NB_HMS)
	Size of HMS in AP case differs from MK case.
	This means the larger case is wasted in certain cases.
	Chase down all references to DSP_NB_HMS and decide on which case 
	to use.	 You could interrogate the currently loaded system if you
	have to do both.
#endif
#define DSP_NB_HMS MIN(DSPMK_NB_HMS,DSPAP_NB_HMS)
#define DSP_MAX_HM (DSP_NB_HMS-2) /* Leave room in HMS for begin/end marks */

#define DSPMK_UNTIMED NULL	/* Denotes untimed, not tick-synchronized */

/*** GLOBAL VARIABLES ***/	/* defined in DSPGlobals.c */
extern int DSPErrorNo;
extern DSPTimeStamp DSPMKTimeStamp0; /* Tick-synchronized, untimed */

/* Numerical conversion */
#define DSP_TWO_TO_24   ((double)16777216.0)
#define DSP_TWO_TO_M_24 ((double)5.960464477539063e-08)
#define DSP_TWO_TO_23   ((double)8388608.0)
#define DSP_TWO_TO_M_23 ((double)1.192092895507813e-7)
#define DSP_TWO_TO_48   ((double)281474976710656.0)
#define DSP_TWO_TO_M_48 ((double)3.552713678800501e-15)

#define DSP_INT_TO_FLOAT(x) ((((float)(x))*((float)DSP_TWO_TO_M_23)))
#define DSP_FIX24_TO_FLOAT(x) ((((float)(x))*((float)DSP_TWO_TO_M_23)))
#define DSP_INT_TO_DOUBLE(x) ((((double)(x))*(DSP_TWO_TO_M_23)))
#define DSP_FIX48_TO_DOUBLE(x) ((double)((x)->high24)*(DSP_TWO_TO_24) + ((double) (x)->low24))
#define DSP_FLOAT_TO_INT(x) ((int)(((double)(x))*((double)DSP_TWO_TO_23)+0.5))
#define DSP_DOUBLE_TO_INT(x) ((int)(((double)(x))*(DSP_TWO_TO_23)+0.5))

/* Max positive DSP float = (1-1/2^23) */
#define DSP_F_MAXPOS ((double)(1.0-DSP_TWO_TO_M_23))
#define DSP_ONE DSP_F_MAXPOS

/* Max negative DSP float = -2^23 */
#define DSP_F_MAXNEG (float) -1.0 

#define DSP_FIX24_CLIP(_x) (((int)_x) & 0xFFFFFF)

#define DSPMK_LOW_SAMPLING_RATE 22050.0
#define DSPMK_HIGH_SAMPLING_RATE 44100.0

#ifndef REMEMBER
#define REMEMBER(x) /* x */
#endif

#define DSPMK_NTICK DSPMK_I_NTICK

/*** FILE NAMES ***/
/*** These filenames must stay in synch with those in $DSP/Makefile.config ***/

#define DSP_SYSTEM_DIRECTORY "/usr/lib/dsp/"
#define DSP_BIN_DIRECTORY "/usr/bin/"
#define DSP_AP_BIN_DIRECTORY "/apbin/"
#define DSP_INSTALL_ROOT "/usr/lib/dsp/"
#define DSP_ERRORS_FILE "/tmp/dsperrors"
#define DSP_WHO_FILE "/tmp/dsp.who"

extern char *DSPGetDSPDirectory();	/* /usr/lib/dsp or $DSP if $DSP set */
extern char *DSPGetSystemDirectory();	/* /usr/lib/dsp/monitor|$DSP/monitor */
extern char *DSPGetImgDirectory();	/* /usr/lib/dsp/img or $DSP/img */
extern char *DSPGetAPDirectory();	/* /usr/lib/dsp/imgap or $DSP/imgap */
extern char *DSPGetMusicDirectory();	/* DSP_MUSIC_DIRECTORY */
extern char *DSPGetLocalBinDirectory(); /* /usr/bin or $DSP/bin */

/* 
   Convert Y-space address in DSP "XY memory partition" 
   (where X and Y memories are each 4K long addressed from 0xA000)
   into an equivalent address in the "overlaid memory partition"
   (where memory addresses are irrespective of space).
   *** NOTE: This macro depends on there being 8K words of DSP static RAM ***
   Behaves like a function returning int.
*/
#define DSPMapPMemY(ya) ((int) ((ya) & 0x7FFF))

/* 
   Convert X-space address in DSP "XY memory partition" 
   into an equivalent address in the "overlaid memory partition"
   *** NOTE: This macro depends on there being 8K words of DSP static RAM ***
   Behaves like a function returning int.
*/
#define DSPMapPMemX(xa) ((int) (((xa)|0x1000) & 0x7FFF))

/* 
   File names below are ALL assumed to exist in the directory returned by
   DSPGetSystemDirectory(): 
*/

/* Default AP and MK monitors installed with the system */

/* dsp0 */
#define DSP_MUSIC_SYSTEM_BINARY_0 "/mkmon8k.dsp" /* DSPObject.c DSPBoot.c */
#define DSP_MUSIC_SYSTEM_0 "/mkmon8k.lnk"	/* DSPObject.c DSPReadFile.c */
#define DSP_MUSIC_SYSTEM_MAP_FILE_0 "/mkmon8k.mem" /* DSPObject.c and 
						     _DSPRelocateuser.c */
#define DSP_AP_SYSTEM_BINARY_0 "/apmon8k.dsp"
#define DSP_AP_SYSTEM_0 "/apmon8k.lnk"
#define DSP_AP_SYSTEM_MAP_FILE_0 "/apmon8k.mem"

#define DSP_32K_MUSIC_SYSTEM_BINARY_0 "/mkmon32k.dsp"
#define DSP_32K_MUSIC_SYSTEM_0 "/mkmon32k.lnk"
#define DSP_32K_MUSIC_SYSTEM_MAP_FILE_0 "/mkmon32k.mem"

#define DSP_32K_AP_SYSTEM_BINARY_0 "/apmon32k.dsp"
#define DSP_32K_AP_SYSTEM_0 "/apmon32k.lnk"
#define DSP_32K_AP_SYSTEM_MAP_FILE_0 "/apmon32k.mem"

/* Number of words above which array-reads from the DSP use DMA */
#define DSP_MIN_DMA_READ_SIZE 1024

/* Number of words above which array-writes to the DSP use DMA */
#define DSP_MIN_DMA_WRITE_SIZE 1024

/* New mode for more efficient DMA transfers (cf. <nextdev/snd_msgs.h>) */
#define DSP_MODE32_LEFT_JUSTIFIED 6

#endif DSP_H
