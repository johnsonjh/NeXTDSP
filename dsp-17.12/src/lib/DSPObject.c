#define SND_DSP_PROTO_HOSTMSG SND_DSP_PROTO_DSPERR 

#define VERSION_1 0

#define DO_USER_INITIATED_DMA 0	/*** FIXME - make TRUE always***/
#define UI_DMA_CHANDATA_FLUSHED 1 /*** FIXME - make TRUE always ***/

#define MMAP 1

/* move to _dsp.h */
#define _DSPMK_WD_BUF_BYTES 8192 /* vm_page_size */
#define _DSPMK_RD_BUF_BYTES 8192 /* vm_page_size */
#define _DSPMK_LARGE_SO_BUF_BYTES 8192
/* #define _DSPMK_SMALL_SO_BUF_BYTES DSPMK_NB_DMA_W */
#define _DSPMK_WD_TIMEOUT 60000

/*	DSPObject.c - Lowest-level DSP utilities
	Copyright 1989 NeXT, Inc.
	
	Functions for controlling the DSP.
	All DSP control is via the DSP host interface.
	All utilities return 0 for success and a nonzero failure code.
	except functions of the form "DSPIs*()" and "DSPGet*()".
	
	Topics
	DSP device allocation
	DSP memory allocation (AP)
	
Modification history:

  12/28/87/jos - File created
  01/26/88/jos - Changed to global dsp number. Old version = %.0.
  03/08/88/jos - Split _DSPUtilities into DSObject + %
  05/01/88/jos - Changed time-outs to use select()
  07/14/88/jos - Removed DSP_HM_DMA_WD_HOST_ON hm from DSPOpen.
	 	 Sound out to host must be enabled manually now!
  07/25/88/jos - Changed DSPOpen to abort if open fails. This was
		 only needed for running on the Sun w/o DSP doing sim.
		 Ideally, DSPOpenSimulatorFile should work without a
		 DSPOpen, i.e., simulator output and DSP output are
		 orthogonal.  Have to add later if needed.
		 DSPMapHostInterface was similarly changed. It should
		 not be called if the DSP is to be simulated only.
		 See any earlier saved bag file for previous behavior.
  07/25/88/jos - Deleted DSPGetMessageArray() and _DSPA field get/set.
		 Moved many functions to _DSPUtilities.c, reordered
		 functions, added comments, made more "methods" private.
		 Functions of the form "DSPIs*" and "DSPGet*" now return
		 useful values instead of error codes.
  08/07/88/jos - Changed default value of s_mapped_only from 1 to 0
  08/10/88/jos - DSPReadArraySkipMode endgame reworked according to
		 hmlib DMA_R_DONE logic. Now there's only 1 garbage wd.
  08/10/88/jos - Added DSP_IS_SIMULATED_ONLY to _DSPMappedOnlyIsEnabled().
  		 Slightly touched the enable logic of PutTX* and GetRX.
  08/17/88/gk  - Modified setup_stream() for sound device linkage.
  08/17/88/jos - Deleted dmar.channel = 1 in DSP{Get,Put}ArraySkipMode()
  08/21/88/jos - Changed time-outs to new convention.
  08/25/88/jos - Changed DSP buffer size from 512 to 1024 bytes
  08/29/88/jos - Sped up DSPGetTXArray(). Created _DSPOpenWithSoundOut().
  09/08/88/jos - Added DSPRawClose() to _DSPReset().
  09/13/88/jos - Installed word<<8 in DSP{Get,Put}ArraySkipMode()
  09/28/88/jos - Added write of 0 to sound-out in DSPRawClose().
  10/08/88/jos - Added DSPResetSoft() call to DSPClose().
  12/13/88/jos - Always mapping interface irrespective of s_mapped_only.
  12/13/88/jos - Added more instance vars and get/set functions.
  12/13/88/jos - Reworked DSPOpen cases.
  01/05/89/jos - Moved s_mapped_only = 1 from 
		 DSPMapHostInterface() to DSPOpen because mmap
		 is now always done. Since dspmmap in dsp.c clears RREQ,
		 we now set it after the mmap call in DSPOpenNoBoot().
  01/18/89/jos - removed #include <nextdev/sroutevar.h>
  02/23/89/jos - converted to new mach driver
  03/09/89/jos - installed support of DSP reg reading via Mach
  03/09/89/jos - removed wait for HC in DSPWriteICR()
  03/09/89/jos - removed wait for HC and TRDY in DSPWriteCVR()
  03/09/89/jos - installed s_cur_pri = current message priority.
  03/24/89/jos - removed wait for TRDY before HC in writeHostMessage.
  03/30/89/jos - In DSPOpenNoBoot, made message clearer if open failed:
		 "The interlock file was deleted or never created"
  04/03/89/jos - Added _DSPResetTMQ() to DSPRawCloseSaveState().
  04/04/89/jos - Enabled DSP host interface memory mapping
  04/05/89/jos - Fixed bug: s_hostInterface set to null after setup.
  05/12/89/jos - Introduced s_host_msg state variable and zeroed
		 default s_dsp_mode_flags so that the get/put interface
	 	 does not have high-order bit of HTX grabbed by driver
		 as error message code.  This was CMU's problem.
		 Added DSP{Enable,Disable}HostMsg[IsEnabled]().
  05/15/89/jos - Added DSPGet{Sound,HostMessage,DSPMessage,Error}Port()
  05/18/89/jos - moved snd_dsp_proto from *setupSound* to DSPOpenNoBoot
  05/18/89/jos - disabled error message checking in readDatum.
  05/22/89/jos - added s_dsp_chandata and write-data support
  06/05/89/jos - added lock file support. Error stuff moved to _DSPError.
  06/05/89/jos - added DSP open priority support.
  06/08/89/jos - changed default system monitor to AP instead of MK
  06/13/89/mtm - Added _DSPSet{AP,MK}SystemFiles().
  06/13/89/jos - Added support for smaller sound-out buffers.
  06/14/89/jos - Added DSP error message thread
  06/17/89/jos - Lots and lots of changes. Reworked owner ports, wd.
  07/21/89/daj - changed line 1789 to free structs
  12/14/89/jos - reworked s_msgSend() to avoid possibility of blocking. (It 
  		 turns out not to matter because the DSP driver's execution 
		 queue is not checked when repeated msg_receives are sent 
		 awaiting the block to clear.)
  12/18/89/jos - Added "if(s_sound_out)
  		        while( DSPAwaitHF3Clear(_DSP_MACH_DEADLOCK_TIMEOUT));" 
		 to _DSPFlushTMQ(). This slows down throughput a lot, but the
		 possibility of deadlock is made extremely smaller.
  02/12/90/jos - Started conversion from _DSPMachSupport to snddriver fns.
  02/19/90/jos - DSPMK{Freeze,Thaw}Orchestra() added (from DSPControl.c)
		 Note that this changes HF0 from an abort signal (which
	 	 was never used as such) to a freeze signal. We still have
		 the abort untimed message and the old "pause" command which
		 merely halts the advance of the sample counter in the DSP.
  03/13/90/jos - Added DSPGetHF2AndHF3(void).
  03/21/90/jos - _DSPCheckTMQFlush() now flushes on timed-zero messages.
  03/21/90/jos -  DSPMKCallTimed() also flushes on timed-zero messages.
  03/21/90/mtm - added support for dsp commands file
  03/26/90/jos - added single-file read-data support
  04/17/90/jos - added read-data file seek support
  04/17/90/jos - revised ReadArraySkipMode, WriteArraySM, and setupProtocol.
  04/17/90/jos - read-data, when enabled, steals half of write-data buffers.
  04/17/90/jos - flushed Get/Set * BufferCount.  Can only do 1 page anyway?
  04/19/90/mtm - Use SND_FORMAT_DSP_COMMANDS in DSPCloseCommandsFile().
  04/23/90/jos - flushed unsupported entry points.
  04/23/90/jos - changed _DSPSendHm() to _DSPWriteHm()
  05/01/90/jos - added DSPLoadSpec *DSPGetSystemImage(void);
  05/01/90/jos - added DSPMKSoundOutDMASize(void);
  05/01/90/jos - added call to DSPMKFlushTimedMessages() in DSPClose().
  05/14/90/jos - _DSPCheckTMQFlush() no longer flushes on timed-zero messages.
  		 We decided it was worth supporting multicomponent TZMs in DSP.
  10/25/90/jos - Enabled SND_DSP_PROTO_TXD protocol bit.

  END HISTORY

*/

#ifdef SHLIB
#include "shlib.h"
#endif

#import <c.h>		/* for MIN,MAX,ABS,bool,TRUE,FALSE,etc. */
#import <next/cpu.h>
#import <mach.h>
#import <cthreads.h>

/* #import <servers/bootstrap.h> */
#import <sound/accesssound.h>

#import <nextdev/snd_dspreg.h>

#include "dsp/_dsp.h"
#include <sound/sound.h>	/* For write-data output file */

#ifdef SHLIB
#include <sound/sounddriver.h>	/* This will fully replace the following */
#else
#include "snddriver.h"		/*** FIXME ***/
#endif

/* #import <mach_init.h> */

#if (!VERSION_1)
/* 1.02+ unprototyped procedures which trigger -Wimplicit warnings */
extern int mmap();
extern int thread_reply();
#else 
/* 1.0  unprototyped procedures which trigger -Wimplicit warnings */
extern int thread_reply();
extern int unlink();
extern int getpagesize();
extern int mmap();
extern int umask();
extern int getpid();
extern int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *execptfds, struct timeval *timeout);
extern int setlinebuf();
extern void usleep(unsigned int s);
extern void pause(void);
/* Needed before 0.97: extern int open(char *path, int flags, int mode); */

kern_return_t snddriver_dsp_reset (
	port_t		cmd_port,		// valid command port
	int		priority)		// priority of this transaction
{
    return KERN_SUCCESS;
}

kern_return_t snddriver_set_dsp_buffers_per_soundout_buffer (
	port_t		dev_port,		// valid device port
	port_t		owner_port,		// valid owner port
	int		dbpsob)			// so buf size / dsp buf size
{
    return KERN_SUCCESS;
}

#endif

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/notify.h>		/* for SEND_NOTIFY et al. */

#ifndef __FILE_HEADER__
#include <sys/file.h>		/* for fopen(), open() */
#endif

/********************* GLOBAL DSP DATA STRUCTURES ****************************/

/* If this file is the Dsp object, these are its instance variables */
/* True globals are in DSPGlobals.c */

/* !!! ANY CHANGES HERE MAY ALSO AFFECT DSPRawClose() !!! */

static int s_dsp_data_waiting = 0; 	/* read-only to user */
static int *s_dsp_data_0 = 0;		/* ptr to 1st wd of DSP message data */
static int *s_dsp_data_ptr = 0;		/* ptr to next wd of DSP msg data */
static int s_dsp_data_count = 0; 	/* total no. wds of DSP msg data */

static msg_header_t *s_dsprcv_msg = 0;  /* contains unread DSP data */
static msg_header_t *s_dspcmd_msg = 0;  /* contains a re-useable dspcmd msg */
static msg_header_t *s_driver_reply_msg = 0;/* contains a re-useable reply msg */

static int s_cur_pri = DSP_MSG_MED;	/* Current Mach message priority */
static port_t s_sound_dev_port = 0;	/* sound/DSP device port  */
static port_t s_dsp_owner_port = 0;	/* ownership capability */
static port_t s_dsp_hm_port = 0;	/* Host Message port (host to DSP) */
static port_t s_dsp_dm_port = 0;	/* DSP Message port (DSP to host) */
static port_t s_driver_reply_port = 0;	/* Port for replies from driver */
static port_t s_dsp_err_port = 0;	/* DSP Error Message port (to host) */
static cthread_t s_dsp_err_thread=0;/* thread id for s_dsp_err_reader() */
static int s_dsp_err_reader();	/* DSP error thread procedure below */
static port_t s_wd_stream_port = 0;	/* Write-data stream (DSP to host) */
static port_t s_wd_reply_port = 0;	/* Port getting wd msgs */
static port_t s_rd_stream_port = 0;	/* Read-data stream (host to DSP) */
static port_t s_rd_reply_port = 0;	/* Port getting rd msgs */
static int s_msg_read_pending = 0;	/* set when dm read request is out */

static int s_max_block_time = 0; /* maximum time spent blocking on HF3 */
static int s_all_block_time = 0; /* total   time spent blocking on HF3 */

static DSPRegs *s_hostInterface = 0; /* DSP host interface */

#define RXDF (s_hostInterface->isr&1)
#define TXDE (s_hostInterface->isr&2)
#define TRDY (s_hostInterface->isr&4)
#define HF2 (s_hostInterface->isr&8)
#define HC (s_hostInterface->cvr&0x80)

#define TXH (s_hostInterface->data.tx.h)
#define TXM (s_hostInterface->data.tx.m)
#define TXL (s_hostInterface->data.tx.l)

#define RXH (s_hostInterface->data.rx.h)
#define RXM (s_hostInterface->data.rx.m)
#define RXL (s_hostInterface->data.rx.l)

static inline void s_writeTX(void *wp)
{
    register unsigned char *bp = (((unsigned char *)wp)+1);
    TXH = *bp++;
    TXM = *bp++;
    TXL = *bp;
}

static inline int s_readRX(void)
{
    int rx;
    register unsigned char *bp = (unsigned char *)(&rx);
    *bp++ = 0;
    *bp++ = RXH;
    *bp++ = RXM;
    *bp = RXL;
    return rx;
}

static inline void s_writeTXL(unsigned char *bp)
/* First call s_clearTXHM() */
{
    TXL = *bp;
}

static inline unsigned char s_readRXL(void)
{
    unsigned char rx;
    rx = RXL;
    return rx;
}

static inline void s_writeTXML(unsigned short *sp)
/* First call s_clearTXH() */
{
    TXM = *((unsigned char *)sp)++;
    TXL = *((unsigned char *)sp);
}

static inline unsigned short s_readRXML(void)
{
    short rx;
    register unsigned char *bp = (unsigned char *)(&rx);
    *bp++ = RXM;
    *bp = RXL;
    return rx;
}

static inline void s_clearTXH(void)
{
    TXH = 0;
}

static inline void s_clearTXHM(void)
{
    TXH = 0;
    TXM = 0;
}

static inline void s_writeTXMLSigned(short *sp)
{
    register unsigned char c1 = *((unsigned char *)sp)++;
    if (c1 & 0x80)
      TXH = 0xFF;
    else
      TXH = 0;
    TXM = c1;
    TXL = *((unsigned char *)sp);
}

static inline void s_writeTXLSigned(char *bp)
{
    register unsigned char c0 = 0;
    register unsigned char c1 = *(unsigned char *)bp;
    if (c1 & 0x80)
      c0 = 0xFF;
    TXH = c0;
    TXM = c0;
    TXL = c1;
}

static int s_simulated = 0;	/* Set true when simulator stream is open */
static char* s_simulatorFile=0; /* Set to simulator output file name if any */
static FILE* s_simulator_fp=0;	/* Set to simulator output fd if any */

static int s_saving_commands = 0;	/* Set true when commands stream is open */
static char* s_commandsFile=0;  /* Set to commands output file name if any */
static FILE* s_commands_fp=0;	/* Set to commands output fd if any */
static int s_commands_numbytes = 0;     /* Number of bytes written to commands file */

static FILE* s_whofile_fp=0;	/* Set to DSP lock file fd (/tmp/dsp_lock) */
static int s_open_priority=0;	/* 0 = low, 1 => open anyway if in use */

static int s_dsp_count=_DSP_NDSPS; /* No. DSPs, set by _DSPSetDSPCount */
static int s_open = 0;		/* Set true when DSP is open */
#define DSP_IS_SIMULATED_ONLY (s_simulated && !s_saving_commands && !s_open)
#define DSP_IS_SAVING_COMMANDS_ONLY (s_saving_commands && !s_simulated && !s_open)
#define CHECK_ERROR_1 if ( !(s_open||s_simulated||s_saving_commands) ) \
      return(_DSPError(EIO,"Attempted IO on unopened DSP"))
static int s_mapped_only = 0;	/* True for memory-mapped interface only */
static int s_logged = 0;	/* Set true when logging DSP file (FIXME) */
static int s_low_srate=1;	/* Set nonzero for 22KHz, zero for 44KHz */ 
static int s_host_msg=0;	/* Set nonzero to get host message protocol */
static int s_sound_out=0;	/* Set nonzero to link in sound driver */ 

/* Write data */
static int s_write_data=0;	/* Set nz to prepare DSP driver for sound */ 
static int s_stop_write_data=0; /* Tells write-data thread to exit */
static int s_write_data_running=0; /* Write-data thread is running */
static char* s_wd_fn=0;		/* set nz to specify write-data filename */
static FILE* s_wd_fp=0;		/* Set wd file ptr (overrides filename) */
static SNDSoundStruct *s_wd_header=0; /* Hdr for write-data output file */
static int s_wd_sample_count=0; /* Total no. samples written to disk */
static int s_wd_timeout=_DSPMK_WD_TIMEOUT; /* No.ms until giving up */
static int s_no_thread = 0;	/* For disabling thread launch in gdb */
static cthread_t s_wd_thread=0; /* thread id for s_write_data_reader() */
static int s_wd_reader();	/* write-data thread procedure (below) */
static int s_wd_error = 0;	/* write-data thread error code */
static char *s_wd_error_str = 0; /* write-data thread error message */
static int do_wd_cleanup = 1;   /* set nz to permit "abort stream" at end */

/* Read data */
static int s_read_data=0;	/* Set nz to prepare DSP driver for sound */ 
static int s_stop_read_data=0;	/* Tells read-data thread to exit */
static int s_read_data_running=0; /* Read-data thread is running */
static char* s_rd_fn=0;		/* set nz to specify read-data filename */
static int s_rd_fd = -1;	/* read data file descriptor */
static int s_rd_chans;		/* 1 for mono, 2 for stereo, etc. */
static SNDSoundStruct *s_rd_header=0; /* Hdr for read-data input file */
static int s_rd_sample_count=0; /* Total no. samples read from disk and sent */
static cthread_t s_rd_thread=0; /* thread id for s_read_data_writer() */
static int s_rd_writer();	/* read-data thread procedure (below) */
static int s_rd_error = 0;	/* read-data thread error code */
static char *s_rd_error_str = 0; /* read-data thread error message */
static int s_dsp_rd_buf0 = DSPMK_YB_DMA_W2; /* rd DSP buffer address, if any */
static msg_header_t *s_rd_rmsg = 0; /* read-data stream port replies */

static int s_ssi_sound_out=0;	/* Set to enable SSI sound out in DSP MK */
static double s_srate=22050.;	/* Keep in synch w s_low_srate */
static int s_ap_mode=0;		/* Set nonzero to load array processing sys */ 
static int s_dsp_mode_flags = 0; /* passed to sound/dsp driver below */
static int s_dsp_access_flags = 0;
static int s_dsp_buf_wds = (DSPMK_NB_DMA_W >> 1); /* two DMA buffers in wds */
static int s_dsp_record_buf_bytes = _DSPMK_WD_BUF_BYTES; /* in bytes */
static int s_dsp_play_buf_bytes = _DSPMK_RD_BUF_BYTES; /* in bytes */
static short *s_rd_buf = 0;	/* read-data disk buffer */
static int do_mapped_array_reads = 1;  /* set nonzero for mapped array reads */
static int do_mapped_array_writes = 0; /* set nonzero for mapped arr writes */
static int do_unchecked_mapped_array_transfers = 0; /* ignore RXDF and TXDE! */
static int max_rxdf_buzz = 0;	/* maximum no. times rxdf tested before true */
static int max_txde_buzz = 0;	/* maximum no. times txde tested before true */
static int max_hm_buzz = 0;	/* total buzzing for host message write */
static int s_so_buf_bytes = _DSPMK_LARGE_SO_BUF_BYTES; /* Sound-out bytes */
static DSPLoadSpec *s_systemImage; /* Last thing loaded by DSPBoot() */
static char *s_system_link_file = DSP_AP_SYSTEM_0; /* _dsp.h */
static char *s_system_binary_file = DSP_AP_SYSTEM_BINARY_0;
static char *s_system_map_file = DSP_AP_SYSTEM_MAP_FILE_0;
static int s_ap_system = 1;	/* boolean indicator of system type */
static int s_mk_system = 0;	/* boolean indicator of system type */
static char s_devstr[] = "/dev/dsp0";
static int s_idsp = 0;		/* Assigned DSP number */
static int s_dsp_fd = 0;	/* Memory-mapped DSP file descriptor */
static int s_joint_owner = 0;	/* True if we are not the first DSP owner */
static int s_dsp_messages_disabled = 0;
static int ec = 0;		/* error code */
static int s_low_water = 48*1024; /* driver sound buffer refill point */
static int s_high_water = 64*1024; /* driver sound buffer max size */
static int s_stream_configuration = 0;
static int s_frozen = 0;
static int s_small_buffers = 0;

/***** Time stamping utilities *******/

static struct timeval s_timeval; /* used for time-stamping */
static struct timezone s_timezone;
static int s_prvtime = 0;
static int s_curtime = 0;
static int s_deltime = 0;

int DSPGetHostTime(void) 
{
    gettimeofday(&s_timeval, &s_timezone);
    s_curtime = (s_timeval.tv_sec & (long)0x7FF)*1000000 
      + s_timeval.tv_usec;
    if (s_prvtime == 0)
      s_deltime = 0;
    else
      s_deltime = s_curtime - s_prvtime;
    s_prvtime = s_curtime;
    return(s_deltime);
}
    
/***** Utilities which belong global with respect to all DSP instances *******/
    
/*** FIXME: 
      This section should only exist in the Objective C DSP manager object
      (which has not yet been written).  DSPObject.c corresponds
      to one instance of a DSP object, and this function will 
      set the "active" DSP id to the newidsp'th element of an array of DSP 
      id's. 
***/
    
int DSPGetDSPCount(void)
{
    return(_DSP_NDSPS);
}

int DSPSetCurrentDSP(int newidsp)
{
    s_idsp = (newidsp<0?0:(newidsp<s_dsp_count?newidsp:s_dsp_count));
    if ( newidsp != s_idsp)
      return(_DSPError(EINVAL,"DSP number out of range"));
    return(0);
}

int DSPGetCurrentDSP(void)
{
    return(s_idsp);
}

/***************** Boolean state interrogation functions ******************/

/*
 * These functions do not follow the convention of returning an error code.
 * Instead (because there can be no error), they return a boolean value.
 * Each functions in this class has a name beginning with "DSPIs".
 */


int DSPIsOpen(void)
{
    return(s_open);
}


int DSPMKIsWithSoundOut(void)
{
    return(s_sound_out);
}


int DSPDataIsAvailable(void)
/*
 * Returns nonzero if DSP messages are waiting to be read.
 */
{
    if (s_mapped_only) {
	int ec, isr, rxdf = 0;
	if (DSP_IS_SIMULATED_ONLY)
	  rxdf = 0;
	else {
	    if (ec=DSPReadISR(&isr)) {
		_DSPError(ec,"DSPDataIsAvailable: Cannot read ISR");
		return(-1);	/* Error code = invalid data */
	    }
	    rxdf = (isr & DSP_ISR_RXDF) != 0;
	}
	return(rxdf);
    }
    else {
	if (s_dsp_data_waiting)
	  return(TRUE);
	DSPReadMessages(0);	/* FIXME: Flush if thread used later */
	return(s_dsp_data_waiting);
    }
}


int DSPIsSimulated(void)
{
    return(s_simulated);
}

int DSPIsSimulatedOnly(void)
{
    return(DSP_IS_SIMULATED_ONLY);
}

int DSPIsSavingCommands(void)
{
    return(s_saving_commands);
}

int DSPIsSavingCommandsOnly(void)
{
    return(DSP_IS_SAVING_COMMANDS_ONLY);
}


/*************** Getting and setting "DSP instance variables" ****************/

/*
 * DSP "get" functions do not follow the convention of returning an error code.
 * Instead (because there can be no error), they return the requested value.
 * Each functions in this class has a name beginning with "DSPGet".
 */

int DSPGetOpenPriority(void)
{
    return s_open_priority;
}


int DSPSetOpenPriority(int pri)
{
    s_open_priority = pri;
    return 0;
}


int DSPGetMessagePriority(void)
{
    return s_cur_pri;
}


int DSPSetMessagePriority(int pri)
{
    if (pri==DSP_MSG_HIGH)
      s_cur_pri = DSP_MSG_HIGH;
    else if (pri==DSP_MSG_LOW)
      s_cur_pri = DSP_MSG_LOW;
    else if (pri==DSP_MSG_MED)
      s_cur_pri = DSP_MSG_MED;
    else return _DSPError1(DSP_EMISC, "DSPSetMessagePriority: "
			   "Mach message priority %s does not exist",
			   _DSPCVS(pri));
    return 0;
}


DSPRegs *_DSPGetRegs(void)
{
    if (!s_open)
      return(NULL);
    else
      return(s_hostInterface);
}


/*** FIXME: 
  Mapped-only mode will be needed for Ariel board.
  However, it is still something to hide for the main DSP
  because all the sound io and write data become problematic.
  ***/

int DSPEnableMappedOnly(void)
{
#if !MMAP
    return _DSPError(-1,"DSPEnableMappedOnly: Cannot map host interface");
#endif
    s_mapped_only = 1;
    return 0;
}

int DSPDisableMappedOnly(void)
{
    s_mapped_only = 0;
    return(0);
}

int DSPMappedOnlyIsEnabled(void)
{
    return s_mapped_only;
}

/* Old "private" versions */
int _DSPEnableMappedOnly(void) 	  { return DSPEnableMappedOnly();   }
int _DSPDisableMappedOnly(void)	  { return DSPDisableMappedOnly();  }
int _DSPMappedOnlyIsEnabled(void) { return DSPMappedOnlyIsEnabled();}

int DSPIsMappedOnly(void)
{
    return(s_mapped_only); /* || DSP_IS_SIMULATED_ONLY); */
}

int _DSPEnableMappedArrayReads(void)
{
#if !MMAP
    return _DSPError(-1,"DSPEnableMappedArrayReads: "
		      "host interface not mapped");
#endif
    do_mapped_array_reads = 1;
    return 0;
}

int _DSPDisableMappedArrayReads(void)
{
    do_mapped_array_reads = 0;
    return 0;
}

int _DSPEnableMappedArrayWrites(void)
{
#if !MMAP
    return _DSPError(-1,"DSPEnableMappedArrayWrites: "
		      "host interface not mapped");
#endif
    do_mapped_array_writes = 1;
    return 0;
}

int _DSPDisableMappedArrayWrites(void)
{
    do_mapped_array_writes = 0;
    return 0;
}

int _DSPEnableMappedArrayTransfers(void)
{
#if !MMAP
    return _DSPError(-1,"DSPEnableMappedArrayTransfers: "
		      "host interface not mapped");
#endif
    do_mapped_array_reads = 1;
    do_mapped_array_writes = 1;
    return 0;
}

int _DSPDisableMappedArrayTransfers(void)
{
    do_mapped_array_reads = 0;
    do_mapped_array_writes = 0;
    return 0;
}

int _DSPEnableUncheckedMappedArrayTransfers(void)
{
#if !MMAP
    return _DSPError(-1,"DSPEnableUncheckedMappedArrayTransfers: "
		      "host interface not mapped");
#endif
    do_unchecked_mapped_array_transfers = 1;
    return 0;
}

int _DSPDisableUncheckedMappedArrayTransfers(void)
{
    do_unchecked_mapped_array_transfers = 0;
    return 0;
}

int DSPMKEnableWriteDataCleanup(void)
{
    do_wd_cleanup = 1;
    return 0;
}

int DSPMKDisableWriteDataCleanup(void)
{
    do_wd_cleanup = 0;
    return 0;
}

int DSPMKWriteDataCleanupIsEnabled(void)
{
    return do_wd_cleanup;
}

FILE *DSPGetSimulatorFP(void)
{
    return(s_simulator_fp);
}

int DSPSetSimulatorFP(fp)
    FILE *fp;
{
    s_simulator_fp = fp;
    return 0;
}

FILE *DSPGetCommandsFP(void)
{
    return(s_commands_fp);
}

int DSPSetCommandsFP(fp)
    FILE *fp;
{
    s_commands_fp = fp;
    return(0);
}

double DSPMKGetSamplingRate(void)
{
    return(s_srate);
}

int DSPMKSetSamplingRate(double srate)
{
    s_srate = srate;
    
    if (srate == DSPMK_LOW_SAMPLING_RATE)
      s_low_srate = 1;
    else {
	/* Assume user is running at some odd rate in DSP if not 44.1KHz */
	s_low_srate = 0;
    }
    
    return 0;
}

/*************** Enable/Disable/Query for DSP state variables ****************/

int DSPEnableHostMsg(void)
{
    s_host_msg = 1;
    return 0;
}

int DSPDisableHostMsg(void)
{
    s_host_msg = 0;
    return 0;
}

int DSPHostMsgIsEnabled(void)
{
    return(s_host_msg);
}

int DSPMKEnableSoundOut(void)
{
    s_sound_out = 1;
    return 0;
}

int DSPMKDisableSoundOut(void)
{
    s_sound_out = 0;
    return 0;
}

int DSPMKSoundOutIsEnabled(void)
{
    return(s_sound_out);
}

/*** FIXME: not supported yet by DSPOpenNoBoot() ***/
int DSPMKEnableSSISoundOut(void)
{
    s_ssi_sound_out = 1;
    return 0;
}

int DSPMKDisableSSISoundOut(void)
{
    s_ssi_sound_out = 0;
    return 0;
}

int DSPMKSSISoundOutIsEnabled(void)
{
    return(s_ssi_sound_out);
}


int DSPMKEnableWriteData(void)
{
    s_write_data = 1;
    return 0;
}

int DSPMKDisableWriteData(void)
{
    s_write_data = 0;
    return 0;
}

int DSPMKWriteDataIsEnabled(void)
{
    return(s_write_data);
}

int DSPMKWriteDataIsRunning(void)
{
    return(s_write_data_running);
}

int DSPMKEnableReadData(void)
{
    s_read_data = 1;
    return 0;
}

int DSPMKDisableReadData(void)
{
    s_read_data = 0;
    return 0;
}

int DSPMKReadDataIsEnabled(void)
{
    return(s_read_data);
}

int DSPMKReadDataIsRunning(void)
{
    return(s_read_data_running);
}

int DSPMKEnableSmallBuffers(void)
{
    s_small_buffers = 1;
    return 0;
}

int DSPMKDisableSmallBuffers(void)
{
    s_small_buffers = 0;
    return 0;
}

int DSPMKSmallBuffersIsEnabled(void)
{
    return s_small_buffers;
}

int _DSPGetNumber(void)
{
    return(s_idsp);
}

int _DSPSetNumber(
    int i)
{
    s_idsp = i;
    return 0;
}

int _DSPOwnershipIsJoint()
{
    return s_joint_owner;
}

/*************** Getting Mach Ports associated with the DSP ****************/

port_t DSPMKGetSoundPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPGetSoundPort: Attempt to access port "
		       "before its creation"));
    return(s_sound_dev_port);
}

port_t DSPGetOwnerPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPGetOwnerPort: Attempt to access port "
		       "before its creation"));
    return(s_dsp_owner_port);
}

port_t DSPGetHostMessagePort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPGetHostMessagePort: Attempt to access port "
		       "before its creation"));
    return(s_dsp_hm_port);
}

port_t DSPGetDSPMessagePort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPGetDSPMessagePort: Attempt to access port "
		       "before its creation"));
    return(s_dsp_dm_port);
}

port_t DSPGetErrorPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPGetErrorPort: Attempt to access port "
		       "before its creation"));
    return(s_dsp_err_port);
}

port_t DSPMKGetWriteDataStreamPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPMKGetWriteDataStreamPort: "
		       "Attempt to access port "
		       "before its creation"));
    return(s_wd_stream_port);
}

port_t DSPMKGetReadDataStreamPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPMKGetReadDataStreamPort: "
		       "Attempt to access port "
		       "before its creation"));
    return(s_rd_stream_port);
}

port_t DSPMKGetWriteDataReplyPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPMKGetWriteDataReplyPort: "
		       "Attempt to access port "
		       "before its creation"));
    return(s_wd_reply_port);
}

port_t DSPMKGetReadDataReplyPort(void)
{
    if (!s_open)
      return(_DSPError(-1, "DSPMKGetReadDataReplyPort: "
		       "Attempt to access port "
		       "before its creation"));
    return(s_rd_reply_port);
}

/****************** Getting and setting DSP system files *********************/

char *DSPGetDSPDirectory(void)
{
    char *dspdir;
    dspdir = getenv("DSP");
    if (dspdir == NULL)
      dspdir = DSP_SYSTEM_DIRECTORY;	/* <dsp/dsp.h> */
    return(dspdir);
}

char *DSPGetImgDirectory(void) 
{
    char *dspimgdir;
    dspimgdir = DSPGetDSPDirectory();
    dspimgdir = DSPCat(dspimgdir,"/img/"); 
    return(dspimgdir);
}	

char *DSPGetAPDirectory(void) 
{
    char *dspimgdir;
    dspimgdir = DSPGetDSPDirectory();
    dspimgdir = DSPCat(dspimgdir,DSP_AP_BIN_DIRECTORY);
    return(dspimgdir);
}	

char *DSPGetSystemDirectory(void) 
{
    char *sysdir;
    sysdir = DSPGetDSPDirectory();
    sysdir = DSPCat(sysdir,"/monitor/");
    return(sysdir);
}

char *DSPGetLocalBinDirectory(void) 
{
    char *lbdir;
    char *dspdir;
    dspdir = getenv("DSP");
    if (dspdir == NULL)
      lbdir = DSP_BIN_DIRECTORY; /* revert to installed binary */
    else
      lbdir = DSPCat(dspdir,"/bin/");
    return(lbdir);
}

DSPLoadSpec *DSPGetSystemImage(void)
{
    return s_systemImage;
}

int DSPSetSystem(DSPLoadSpec *system)
{
    s_systemImage = system;
    if (system->module)
      if (strcmp(system->module,"MKMON8K")==0) {
	  s_ap_system = 0;
	  s_mk_system = 1;
	  s_system_link_file   = DSP_MUSIC_SYSTEM_0; /* _dsp.h */
	  s_system_binary_file = DSP_MUSIC_SYSTEM_BINARY_0;
	  s_system_map_file    = DSP_MUSIC_SYSTEM_MAP_FILE_0;
      } else if (strcmp(system->module,"APMON8K")==0) {
	  s_ap_system = 1;
	  s_mk_system = 0;
	  s_system_link_file   = DSP_AP_SYSTEM_0; /* _dsp.h */
	  s_system_binary_file = DSP_AP_SYSTEM_BINARY_0;
	  s_system_map_file    = DSP_AP_SYSTEM_MAP_FILE_0;
      } else {	     
	  s_ap_system = 0;
	  s_mk_system = 0;
      }		
    return 0;
}	

int DSPMonitorIsAP(void)
{
    return s_ap_system;
}

int DSPMonitorIsMK(void)
{
    return s_mk_system;
}

char *DSPGetSystemBinaryFile(void) 
{
    char *dspdir = DSPGetSystemDirectory(); /* _dsp.h */
    return(DSPCat(dspdir,s_system_binary_file));
}

char *DSPGetSystemLinkFile(void)
{
    char *dspdir = DSPGetSystemDirectory(); /* _dsp.h */
    return(DSPCat(dspdir,s_system_link_file));
    return 0;
}

char *DSPGetSystemMapFile(void)
{
    char *dspdir = DSPGetSystemDirectory(); /* _dsp.h */
    return(DSPCat(dspdir,s_system_map_file));
}

int DSPSetAPSystemFiles(void)
{
    s_system_link_file	 = DSP_AP_SYSTEM_0; /* _dsp.h */
    s_system_binary_file = DSP_AP_SYSTEM_BINARY_0;
    s_system_map_file	 = DSP_AP_SYSTEM_MAP_FILE_0;
    s_ap_system = 1;
    s_mk_system = 0;
    return 0;
}

int DSPSetMKSystemFiles(void)
{
    s_system_link_file	 = DSP_MUSIC_SYSTEM_0; /* dsp.h */
    s_system_binary_file = DSP_MUSIC_SYSTEM_BINARY_0;
    s_system_map_file	 = DSP_MUSIC_SYSTEM_MAP_FILE_0;
    s_ap_system = 0;
    s_mk_system = 1;
    return 0;
}

/***************************** Small Utilities ****************************/

int DSPMKEnableBlockingOnTMQEmptyTimed(DSPFix48 *aTimeStampP)
{
    return DSPMKHostMessageTimed(aTimeStampP,DSP_HM_BLOCK_TMQ_LWM);
}

int DSPMKDisableBlockingOnTMQEmptyTimed(DSPFix48 *aTimeStampP)
/* 
 * Tell the DSP not to block when the Timed Message Queue reaches its
 * "low-water mark."
 */
{
    return DSPMKHostMessageTimed(aTimeStampP,DSP_HM_UNBLOCK_TMQ_LWM);
}

static int s_allocPort(port_t *portP)
/* 
 * Allocate Mach port.
 */
{
    int ec;
    ec = port_allocate(task_self(), portP);
    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"DSPObject.c: port_allocate failed.");
    else 
      return 0;
}

static int s_freePort(port_t *portP)
/* 
 * Deallocate Mach port.
 */
{
    int ec;
    
    if (!portP)
      return 0;
    
    if (*portP) {
	ec = port_deallocate(task_self(), *portP);
	if (ec != KERN_SUCCESS)
	  return(_DSPMachError(ec,"s_freePort: port_deallocate failed."));
	*portP	= 0;
    }
    return 0;
}


static int DSPAwakenDriver(void)
/* 
 * Send empty message to DSP to wake up driver. 
 */
{
    msg_header_t *dspcmd_msg = _DSP_dspcmd_msg(s_dsp_hm_port,
					       s_dsp_dm_port,
					       DSP_MSG_LOW,0);
    ec = msg_send(dspcmd_msg, MSG_OPTION_NONE,0);
    free(dspcmd_msg);
}


/***************************** WriteData Handling ****************************/

int DSPMKGetWriteDataSampleCount(void)
{
    return s_wd_sample_count;
}

int DSPMKGetWriteDataTimeOut(void)
{
    return s_wd_timeout;
}

int DSPMKSetWriteDataTimeOut(
    int to)
{
    s_wd_timeout = to;
    return 0;
}


int DSPMKSetWriteDataFP(
    FILE *fp)
{
    s_wd_fp = fp;
    s_wd_fn = 0;
    return 0;
}


FILE *DSPMKGetWriteDataFP(void)
{
    return s_wd_fp;
}

int DSPMKSetWriteDataFile(char *fn)
{
    s_wd_fn = fn;
    s_wd_fp = 0;
    return 0;
}

char *DSPMKGetWriteDataFile(void)
{
    return s_wd_fn;
}

int DSPMKCloseWriteDataFile(void)
{
    if (s_write_data_running)
      DSPMKStopWriteData();
    if (s_wd_fp)
      fclose(s_wd_fp);
    s_wd_fp = 0;
    return 0;
}

int DSPMKRewindWriteData(void) 
{
    rewind(s_wd_fp);
    return 0;
}

int DSPMKStopWriteDataTimed(DSPTimeStamp *aTimeStampP)
{
    int ec;
    int chan = 1;		/* Sound-out is DSP DMA channel 1 */
    if (s_simulated) 
      fprintf(s_simulator_fp,";; Disable write data from DSP to host\n");
    
    /* It's ok if DSPMK*StopSoundOut*() does this redundantly */
    DSP_UNTIL_ERROR(DSPMKCallTimed(aTimeStampP,DSP_HM_HOST_WD_OFF,1,&chan));
    DSPMKEnableBlockingOnTMQEmptyTimed(aTimeStampP);
    return(ec);
}

static int s_finish_wd()
{
    int ndata;

    /* 
     * Halt write-data thread 
     */
    if (s_write_data_running) {
	s_stop_write_data = 1;
	cthread_join(s_wd_thread);
	s_wd_thread = 0;
    } /* else thread is already dead */

    if (s_wd_error != 0)
      _DSPMachError(ec,DSPCat("DSPObject.c: s_finish_wd: error in reader thread: ",
			       s_wd_error_str));
    s_wd_error = 0;
    s_wd_error_str = 0;

    if (do_wd_cleanup) {	/* conditional due to bugs */
	/*
	 * *** ALL DONE RECORDING WRITE DATA ***
	 * Find out how many samples were recorded.
	 */
	ec = snddriver_stream_nsamples(s_wd_stream_port, &ndata);
	if (ec != KERN_SUCCESS)
	  _DSPMachError(ec,"DSPObject.c: s_finish_wd: "
			"snd_stream_nsamples failed");
	
	ndata >>= 1;
	
	if (_DSPVerbose)
	  fprintf(stderr,"\nTotal length = %d bytes (%d samples)\n",
		  (ndata<<1), ndata);
	
	if ((ndata) != s_wd_sample_count) {
	    _DSPError1(0,"DSPObject.c: s_finish_wd: "
		       "I count total number of samples = %s",
		       _DSPCVS(s_wd_sample_count));
	    _DSPError1(0,"... while the driver counts %s",
		       _DSPCVS(ndata));
	}
    }
    
    /* 
     * Rewrite header to disk to get byte-count right, 
     * then close the write-data file. 
     */
    if (s_wd_header) {
	rewind(s_wd_fp);
	s_wd_header->dataSize = s_wd_sample_count<<1;
	fwrite((char *)s_wd_header, 1, sizeof *s_wd_header, s_wd_fp); 
	if (do_wd_cleanup)
	/*
	 * *** FIXME: The following triggers a malloc_debug(7) complaint 
	 * (attempt to free something already freed) in the next fclose call:
	 *
	 * 	SNDFree(s_rd_header); 
	 */
	 s_rd_header = 0;
    }
    
    if (s_wd_fp && s_wd_fn) {  /* Can't close FP which was passed to us */
	  fclose(s_wd_fp); 
	s_wd_fp = 0;
	if (_DSPVerbose)
	  _DSPError1(0,"Closed write-data output file %s.",s_wd_fn);
    }
    
    s_write_data_running = 0;	/* Already done on wd thread exit  */

    return 0;
}


int DSPMKStopWriteData(void) 
{
    int ec;
    int chan = 1;		/* Sound-out is DSP DMA channel 1 */

    /* It's ok if DSPMKStopSoundOut() does this redundantly */
    DSP_UNTIL_ERROR(DSPCall(DSP_HM_HOST_WD_OFF,1,&chan));
    DSPMKDisableBlockingOnTMQEmptyTimed(NULL);
    ec=DSPHostMessage(DSP_HM_HOST_R_DONE);	/* in case DMA in progress */

    s_finish_wd();
    
    return(ec);
}

static int s_enqueue_record_region(void)
{
    static int tag=0;
    int ec;

    ec = snddriver_stream_start_reading(s_wd_stream_port,NULL,
    	/* no. samples to read */ (s_dsp_record_buf_bytes>>1),
	tag++,0, /* completed */ 1, /* aborted */ 1, 0,0,0,s_wd_reply_port);

    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"s_enqueue_record_region: "
		  "snddriver_stream_start_reading for write-data failed");
    return 0;
}

static int wd_region = 0;
static msg_header_t *s_wd_rmsg = 0;

int DSPMKStartWriteDataTimed(DSPTimeStamp *aTimeStampP)
{
    int chan = 1;		/* DSP write-data and sound-out channel = 1 */
				/* Note that this is NOT DSP_SO_CHAN,
				   but rather a DMA channel no. IN THE DSP */
    if(!s_write_data)
      return _DSPError(DSP_EMISC,"DSPMKStartWriteData:write data not enabled");
    if (s_simulated) 
      fprintf(s_simulator_fp,";; Enable write data from DSP to host\n");
    /* It's ok if DSPMKStartSoundOut() does this redundantly */

    /* 
     * Tell the DSP to block when the Timed Message Queue reaches its
     * "low-water mark."
     */
    DSPMKEnableBlockingOnTMQEmptyTimed(aTimeStampP);

    DSP_UNTIL_ERROR(DSPMKCallTimed(aTimeStampP,DSP_HM_HOST_WD_ON,1,&chan));
    s_stop_write_data = 0; /* watched by s_wd_reader() */

    if (!s_wd_fn && !s_wd_fp)
      s_wd_fn = "dsp_write_data.raw";
    
    /* if file pointer is null, use file name */
    if (!s_wd_fp) {
	/* Get header to use for the write-data output sound file */
	ec = SNDAlloc(&s_wd_header,
		      0 /* data size (we'll have to fix this later) */,
		      SND_FORMAT_LINEAR_16 /* 3 */,
		      s_low_srate? 
		      SND_RATE_LOW /* 22050.0 */ : 
		      SND_RATE_HIGH /* 44100.0 */,
		      2 /* chans */,
		      104 /* info string space to allocate (for 128 bytes) */
		      );
	if (ec)
	  _DSPError(DSP_EMISC, "DSPMKStartWriteData: SNDAlloc for header failed");
	
	s_wd_header->dataSize = 2000000000; /* 2 gigabyte limit! */
	strcpy( s_wd_header->info,
	       "DSP write data written by Music Kit performance");
	
	s_write_data_running = 1;

	s_wd_fp = fopen(s_wd_fn,"w");
	if (_DSPVerbose)
	  fprintf(stderr,"Opened write-data output file %s\n",s_wd_fn);
	if (s_wd_fp == NULL) {
	    s_write_data_running = 0;
	    return _DSPError1(DSP_EMISC,"DSPMKStartWriteDataTimed: "
			      "Could not open write-data output file %s",s_wd_fn);
	}
	/* write header to disk */
	fwrite((char *)s_wd_header, 1, sizeof *s_wd_header, s_wd_fp); 
    }
    
    s_freePort(&s_wd_reply_port);
    if (ec=s_allocPort(&s_wd_reply_port)) return ec;
    
    s_wd_rmsg = _DSP_stream_msg(s_wd_rmsg,s_wd_stream_port,thread_reply(),1);
    
    DSP_UNTIL_ERROR(s_enqueue_record_region());		/* region 1 */
    DSP_UNTIL_ERROR(s_enqueue_record_region());		/* region 2 */
    
    if (_DSPTrace && DSP_TRACE_WRITE_DATA)
      fprintf(stderr,"Entering write-data loop:\n");
    
    if (s_no_thread)
      s_wd_reader(0);
    else {
      s_wd_thread = cthread_fork((cthread_fn_t) s_wd_reader,0);
      cthread_yield();	/* Allow write-data thread to get going */
    }
    return 0;
}


int DSPMKStartWriteData(void) 
{
    return DSPMKStartWriteDataTimed(DSPMK_UNTIMED);
}


int _DSPMKStartWriteDataNoThread(void) 
{
    s_no_thread = 1;
    return DSPMKStartWriteDataTimed(DSPMK_UNTIMED);
}


static int s_wd_reader(int *arg)
/*
 * function which runs in its own thread reading write-data buffers from
 * the DSP.  The argument is unused.
 */
{
    int DSPsndout,ndata,ec;
    int stopping=0;
    /* timeout subdivision vars */
    int timeout=500;		/* timeout we really use in ms */
    int timeout_so_far=0;	/* total timeout used so far */
    if (timeout > s_wd_timeout)	/* take min */
      timeout = s_wd_timeout;	/* do not exceed user-requested timeout */
    
    while (1) {
	short int *data;
	int i,ec;
	
	/*
	 *
	 */
	s_wd_rmsg->msg_size = MSG_SIZE_MAX;
	s_wd_rmsg->msg_local_port = s_wd_reply_port;
	
	ec = msg_receive(s_wd_rmsg, RCV_TIMEOUT, timeout);
	
	/*
	 * NOTE: stdio cannot be used in multiple threads!
	 */

	if (ec != KERN_SUCCESS && ec != RCV_TIMED_OUT) {
	    s_wd_error = ec;
	    s_wd_error_str = "msg_receive 1 failed";
	}

	if (ec == RCV_TIMED_OUT) {
	    if (s_stop_write_data)
	      goto abort_wd;
	    if (s_frozen)
	      continue;		/* keep trying to read DSP buffer */
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_wd_reader: "
		      "data msg_receive timeout\n");
	    timeout_so_far += timeout;
	    if (timeout_so_far<s_wd_timeout) {
		DSPAwakenDriver();
		continue;	/* retry buffer read from DSP */
	    }
	    else {
		s_wd_error = ec;
		s_wd_error_str = "Timed out waiting for write-data buffer.";
	        break;		/* exit write-data thread */
	    }
	}
	else
	  timeout_so_far = 0;	/* reset cumulated timeout */

	if (s_wd_rmsg->msg_id != SND_MSG_RECORDED_DATA) {
	    s_wd_error = ec;
	    s_wd_error_str = "Unexpected msg while expecting SND_MSG_RECORDED_DATA. "
		       "See /usr/include/nextdev/snd_msgs.h(SND_MSG_*)";
	    /* Unexpected msg = _DSPCVS(s_wd_rmsg->msg_id)); */
	    continue;
	}
	
	/* 
	 * Here we have a buffer of write-data to send to disk.
	 */
	
	data = (short int *)((snd_recorded_data_t *)s_wd_rmsg)->recorded_data;

	ndata = 
	  ((snd_recorded_data_t *)s_wd_rmsg)->dataType.msg_type_long_number;
	ndata >>= 1;
	s_wd_sample_count += ndata; /* Total number of words written */
	
	if (_DSPVerbose)
	  fprintf(stderr,"%d ",s_wd_sample_count);

	if (_DSPTrace)
	  fprintf(stderr,"received msgid %d, %d samples\n", s_wd_rmsg->msg_id, 
		  ndata);
	
	fwrite((char *)data, 1, ndata*2, s_wd_fp); /* write to disk */
    	if (_DSPTrace && DSP_TRACE_WRITE_DATA)
      	   fprintf(stderr,"Region received and written.\n");

	/* 
	 * Deallocate the write-data buffer.
	 */
	ec = vm_deallocate(task_self(),
			   (vm_address_t)((snd_recorded_data_t *)s_wd_rmsg)->
			   recorded_data,
			   (vm_size_t)((snd_recorded_data_t *)s_wd_rmsg)->
			   dataType.msg_type_long_number);
	if (ec != KERN_SUCCESS) {
	    s_wd_error = ec;
	    s_wd_error_str =  "write-data buffer deallocate failed";
	}

	/* 
	 * Terminate if so requested.
	 */
  abort_wd:			/* placement here catches abort timeout too */
	if (stopping)
	  break;           /* We just wrote out the last (partial) buffer */

	if (!s_open) {
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_wd_reader: terminating on DSP closed\n");
	    s_stop_write_data = 1;
	}
	
	if (s_stop_write_data) {
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_wd_reader: "
		      "terminating on s_stop_write_data\n");
	    ec = snddriver_stream_control(s_wd_stream_port,0,SND_DC_ABORT);
	    if (ec != KERN_SUCCESS) {
		s_wd_error = ec;
		s_wd_error_str = "snddriver_stream_control(abort) failed";
	    }
    	    stopping = 1;
	    continue;
	}
	
	/* 
	 * Send a request to record the next buffer.
	 */
    	if (_DSPTrace && DSP_TRACE_WRITE_DATA)
      	   fprintf(stderr,"Enqueing region %d.\n",++wd_region);
	if (s_enqueue_record_region()) {
    	  if (_DSPTrace && DSP_TRACE_WRITE_DATA)
      	    fprintf(stderr,"Enqueing of region %d FAILED. "
	    		   "Aborting wd reader.\n",wd_region);
	  break;
	}
    }
    
    s_write_data_running = 0;

    return 0;
}


/***************************** ReadData Handling ****************************/

int DSPMKGetReadDataSampleCount(void)
{
    int ndata;
    if (!s_read_data_running)
      return s_rd_sample_count;
      
    ec = snddriver_stream_nsamples(s_rd_stream_port, &ndata);
    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"DSPMKGetReadDataSampleCount: "
			   "snddriver_stream_nsamples failed");
	
    ndata >>= 1;
	
    s_rd_sample_count = ndata;
    
    return s_rd_sample_count;
}

/* Flushed from documented API */
int DSPMKGetReadDataFD(void)
{
    return s_rd_fd;
}

int DSPMKSetReadDataBytePointer(int offset)
{
    return lseek(s_rd_fd,offset,L_SET);
}

int DSPMKIncrementReadDataBytePointer(int offset)
{
    return lseek(s_rd_fd,offset,L_INCR);
}

int DSPMKSetReadDataFile(char *fn)
{
    s_rd_fn = fn;

    if (!s_rd_fn)
      return _DSPError(DSP_EBADFILETYPE,
		       "DSPMKSetReadDataFile: NULL read-data filename");
    
    s_rd_fd = open(s_rd_fn,O_RDONLY,0);
    
    if (s_rd_fd < 0)
      return 
	_DSPError1(-1,"DSPMKSetReadDataFile: "
		   "Could not open read-data output file %s",s_rd_fn);
    
    if (_DSPVerbose)
      fprintf(stderr,"Opened read-data input file %s\n",s_rd_fn);
    
    /* read header */
    
    if (ec=SNDReadHeader(s_rd_fd,&s_rd_header))
      return 
	_DSPMachError(ec,DSPCat("DSPMKSetReadDataFile: "
				 "Failed reading header of read-data file. "
				 "sound library error: ",SNDSoundError(ec)));
    
#define RD(x) s_rd_header->x
    
    if (RD(dataFormat) != SND_FORMAT_LINEAR_16)
      return 
	_DSPError(DSP_EBADFILEFORMAT,"DSPMKSetReadDataFile: "
		   "Read-data file must be 16-bit linear format");
    
    s_rd_chans = RD(channelCount);
    
    return 0;
}

int DSPMKRewindReadData(void) 
{
    int ec;
    if (s_rd_fd)
      lseek(s_rd_fd,0,L_SET);
    else
      return(_DSPError(DSP_EMISC,
		       "Attempt to rewind non-existent read-data stream"));
    ec = SNDReadHeader(s_rd_fd,&s_rd_header); /* move ptr past header */
    if (ec == SND_ERR_NONE)
      return 0;
    else
      return
	_DSPError(DSP_EMISC,
		  "Could not read header after rewinding read-data stream");
}

int DSPMKPauseReadDataTimed(DSPTimeStamp *aTimeStampP) 
{
    int chan = 1;
    return DSPMKCallTimed(aTimeStampP,DSP_HM_HOST_RD_OFF,1,&chan);
}

int DSPMKResumeReadDataTimed(DSPTimeStamp *aTimeStampP) 
{
    int chan = 1;
    return DSPMKCallTimed(aTimeStampP,DSP_HM_HOST_RD_ON,1,&chan);
}

static int s_finish_rd(void)
{
    int ndata;

    /* 
     * Halt read-data thread 
     */
    if (s_read_data_running) {
	s_stop_read_data = 1;
	cthread_join(s_rd_thread);
	s_rd_thread=0;
    } /* else thread is already dead */

    if (s_rd_error != 0)
      _DSPMachError(ec,DSPCat("DSPObject.c: s_finish_rd: "
			      "error in writer thread: ",
			       s_rd_error_str));
    s_rd_error = 0;
    s_rd_error_str = 0;

    /*
     * *** ALL DONE RECORDING READ DATA ***
     * Find out how many samples were recorded.
     */
    ec = snddriver_stream_nsamples(s_rd_stream_port, &ndata);
    if (ec != KERN_SUCCESS)
      _DSPMachError(ec,"DSPObject.c: s_finish_rd: "
		    "sndriver_stream_nsamples failed");
    
    ndata >>= 1;

    s_rd_sample_count = ndata;
    
    if (_DSPVerbose)
      fprintf(stderr,"\nTotal read-data count = %d bytes (%d samples)\n",
	      (ndata<<1), ndata);
    
    if (s_rd_fd >= 0)
      close(s_rd_fd);
    s_rd_fd = -1;

    if (_DSPVerbose)
      _DSPError1(0,"Closed read-data output file %s.",s_rd_fn);
    
    s_read_data_running = 0;	/* Already done on rd thread exit  */

    if (s_rd_buf)
      free(s_rd_buf);

    return 0;
}


int DSPMKStopReadDataTimed(DSPTimeStamp *aTimeStampP)
{
    DSP_UNTIL_ERROR(DSPMKPauseReadDataTimed(aTimeStampP));
    return s_finish_rd();	/* kill thread */
}

int DSPMKStopReadData(void) 
{
    return DSPMKStopReadDataTimed(NULL);
}

static int s_write_two_rd_buffers(void)
/* 
 * Called by DSPMKStartReadDataTimed() to initialize read-data buffers
 * in the DSP
 */
{
    int ec,n;

    n = (s_dsp_buf_wds<<2);	/* two DSP buffers worth, in bytes */
    if (n > s_dsp_play_buf_bytes)
      return _DSPError(DSP_EMISC,"Compile-time Configuration error.\n"
		       "s_dsp_play_buf_bytes must be >= 4*s_dsp_buf_wds");

    read(s_rd_fd,(char *)s_rd_buf, n); /* malloc is below */

    DSPWriteArraySkipMode((DSPFix24 *)s_rd_buf,DSP_MS_Y,s_dsp_rd_buf0,1,
			  s_dsp_buf_wds>>1,DSP_MODE16);

    return 0;
}

static int s_enqueue_play_region(void)
/* Called by the read-data thread to send a buffer of sound to the DSP */
{
    static int tag=0;
    int ec;

    read(s_rd_fd,(char *)s_rd_buf, s_dsp_play_buf_bytes); /* malloc is below */

    ec = snddriver_stream_start_writing(s_rd_stream_port,(void *)s_rd_buf,
	/* no. SAMPLES to write */ (s_dsp_play_buf_bytes>>1), tag++, 
        /* preempt */ 0, /* deallocate */ 0,  /* started */ 0, 
	/* completed */ 1, /* aborted */ 1, 0,0,0,s_rd_reply_port);

    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"s_enqueue_play_region: "
		  "snddriver_stream_start_writing for read-data failed");
    return 0;
}

int DSPMKStartReadDataTimed(DSPTimeStamp *aTimeStampP)
{
    int chan = 1;			/* read-data is DSP DMA channel 1 */

    if (s_simulated) 
      fprintf(s_simulator_fp,";; Enable read data from DSP to host\n");

    s_freePort(&s_rd_reply_port);
    if (ec=s_allocPort(&s_rd_reply_port)) return ec;
    
    s_rd_rmsg = _DSP_stream_msg(s_rd_rmsg,s_rd_stream_port,thread_reply(),1);
    
    if (!s_rd_buf)
      s_rd_buf = (short *)malloc(s_dsp_play_buf_bytes);

    DSP_UNTIL_ERROR(s_write_two_rd_buffers());
    
    if (_DSPTrace)
      fprintf(stderr,"Entering read-data loop:\n");
    
    /* Tell DSP when to start reading */
    if (aTimeStampP != DSPMK_UNTIMED) {
	DSP_UNTIL_ERROR(DSPMKCallTimed(aTimeStampP,DSP_HM_HOST_RD_ON,1,&chan));
    }    

    s_stop_read_data = 0;	/* watched by s_rd_writer() */
    s_read_data_running = 1;

    if (s_no_thread)
      s_rd_writer(0);
    else {
	s_rd_thread = cthread_fork((cthread_fn_t)s_rd_writer,0);
	cthread_yield();	/* Allow read-data thread to get going */
    }
    return 0;
}


int DSPMKStartReadDataPaused(void) 
{
    return DSPMKStartReadDataTimed(DSPMK_UNTIMED);
}


int DSPMKStartReadData(void) 
{
    return DSPMKStartReadDataTimed(&DSPMKTimeStamp0);
}


int _DSPMKStartReadDataNoThread(void) 
{
    s_no_thread = 1;
    return DSPMKStartReadDataTimed(DSPMK_UNTIMED);
}


static int s_rd_writer(int *arg)
/*
 * function which runs in its own thread writing read-data buffers to
 * the DSP.  The argument is unused.
 */
{
    int DSPsndout,ndata,ec;
    int timeout=2000;		/* timeout we use internally in ms */
    
    while (1) {
	short int *data;
	int i,ec;

	/*
	 * Wait for "completed" message on the read-data stream port.
	 */

	s_rd_rmsg->msg_size = MSG_SIZE_MAX;
	s_rd_rmsg->msg_local_port = s_rd_reply_port;
	
	ec = msg_receive(s_rd_rmsg, RCV_TIMEOUT, timeout);
	
	/*
	 * NOTE: stdio cannot be used in multiple threads!
	 */
	
	if (ec != KERN_SUCCESS && ec != RCV_TIMED_OUT) {
	    s_rd_error = ec;
	    s_rd_error_str = "msg_receive 1 failed";
	}

	if (ec == RCV_TIMED_OUT) {
	    if (s_frozen)
	      continue;		/* keep trying to read DSP buffer */
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_rd_writer: "
		      "data msg_receive timeout\n");
	    DSPAwakenDriver();
	    continue;	/* retry buffer read from DSP */
	}

	if (s_rd_rmsg->msg_id != SND_MSG_COMPLETED) {
	    s_rd_error = ec;
	    s_rd_error_str = "Unexpected msg while expecting SND_MSG_COMPLETED"
	      "... See /usr/include/nextdev/snd_msgs.h(SND_MSG_*)";
	    /* Unexpected msg = _DSPCVS(s_rd_rmsg->msg_id)); */
	    continue;
	}
	
	/* 
	 * Terminate if so requested.
	 */
	if (!s_open) {
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_rd_writer: terminating on DSP closed\n");
	    s_stop_read_data = 1;
	}
	
	if (s_stop_read_data) {
	    if (_DSPVerbose)
	      fprintf(stderr,"\ns_rd_writer: "
		      "terminating on s_stop_read_data\n");
	    ec = snddriver_stream_control(s_rd_stream_port,0,SND_DC_ABORT);
	    if (ec != KERN_SUCCESS) {
		s_rd_error = ec;
		s_rd_error_str = "snddriver_stream_control(abort) failed";
	    }
	    break;
	}
	
	/* 
	 * Send a request to play the next buffer.
	 */
	if (ec = s_enqueue_play_region()) {
	    s_rd_error = ec;
	    s_rd_error_str = "Enqueing of read-data region FAILED.";
	    break;
	}
    }
    
    s_read_data_running = 0;

    return 0;
}


/***************************** SoundOut Handling ****************************/

int DSPMKStartSoundOut(void) 
{
    int chan = 1;

    if (!s_sound_out && !s_ssi_sound_out)
      return _DSPError(DSP_EMISC,"DSPMKStartSoundOut: "
		       "DSP link to sound-out not enabled");

    if (s_sound_out) {
	if (s_simulated) 
	  fprintf(s_simulator_fp,";; Enable write data from DSP to host\n");
    
	/* It's ok if DSPMKStartWriteData() does this redundantly */
	DSP_UNTIL_ERROR(DSPCall(DSP_HM_HOST_WD_ON,1,&chan));
    }
    
    if (s_ssi_sound_out)
      return DSPMKStartSSISoundOut();

    return 0;
}

int DSPMKStopSoundOut(void) 
{
    int ec;
    int chan = 1;		/* Sound-out is DSP DMA channel 1 */
    if (s_simulated) 
      fprintf(s_simulator_fp,";; Disable sound-out from DSP to host\n");

    /* It's ok if DSPMKStopWriteData() does this redundantly */
    DSP_UNTIL_ERROR(DSPCall(DSP_HM_HOST_WD_OFF,1,&chan));

    if (s_ssi_sound_out)
      return DSPMKStartSSISoundOut();

    return 0;
}

int DSPMKStartSSISoundOut(void) 
{
    if (!s_ssi_sound_out)
      return _DSPError(DSP_EMISC,"DSPMKStartSSISoundOut: not enabled");
    if (s_simulated) 
      fprintf(s_simulator_fp,
	      ";; Enable write data from DSP to SSI serial port\n");
    return(DSPHostMessage(DSP_HM_DMA_WD_SSI_ON));
}

int DSPMKStopSSISoundOut(void) 
{
    DSPMKDisableSSISoundOut(); /* Set instance variable in DSPObject() */
    if (s_simulated) 
      fprintf(s_simulator_fp,
	      ";; Disable write data from DSP to SSI serial port\n");
    return(DSPHostMessage(DSP_HM_DMA_WD_SSI_OFF));
}


/*********************** OPENING AND CLOSING THE DSP ***********************/

int DSPRawCloseSaveState(void)
{
    int i,ec = 0;
    
    s_open = 0;			/* DSP is officially closed. (See threads) */
    
    _DSPResetTMQ();		/* Clear timed-message-queue buffering */
    
    if (s_wd_thread) { 
	s_finish_wd();
	s_wd_thread = 0;
    }
  
    if (s_rd_thread) { 
	s_finish_rd();
	s_rd_thread = 0;
    }
    if (s_simulated)
      DSPCloseSimulatorFile();
    if (s_saving_commands)
      DSPCloseCommandsFile(NULL);
    
    DSPCloseErrorFP();		/* _DSPError.c */
    
/*? s_dsp_hm_port = 0;		/* Allocated by kernel so we can't free it */
    if ( s_freePort(&s_dsp_hm_port) )
      _DSPError(0,"DSPRawCloseSaveState: Could not free s_dsp_hm_port");

/*
  We cannot free the sound device port because the sound library may
  also think it is the owner of soundout.  When ownership is freed, it
  goes away for everyone else also.  What should be happening is that
  there is never joint ownership.  The negotiation port mechanism will
  eventually result in the passing of the ownership capability from one
  task to another (rather than effectively "copying" ownership as is done
  now).  This, by the way, was the "stealth bug" wherein Stealth could
  not use sound-out after a DSPClose().

*    if ( s_freePort(&s_sound_dev_port) )
*      _DSPError(0,"DSPRawCloseSaveState: Could not free s_sound_dev_port");
*/
    ec = SNDRelease(s_dsp_access_flags,s_sound_dev_port,s_dsp_owner_port);
      
    s_dsp_access_flags = 0;
    s_sound_dev_port = 0;	/* Indicate release */
    s_dsp_owner_port = 0;	/* Indicate release */

    if ( s_freePort(&s_dsp_dm_port) )
      _DSPError(0,"DSPRawCloseSaveState: Could not free s_dsp_dm_port");
    if ( s_freePort(&s_driver_reply_port) )
      _DSPError(0,"DSPRawCloseSaveState: Could not free s_driver_reply_port");
    if ( s_freePort(&s_dsp_err_port) )
      _DSPError(0,"DSPRawCloseSaveState: Could not free s_dsp_err_port");
    
    s_dsp_data_waiting = 0;
    s_dsp_data_ptr = s_dsp_data_0;
    s_dsp_data_count = 0;
    s_msg_read_pending = 0;
    s_wd_stream_port = 0;
    if ( s_freePort(&s_wd_reply_port) )
      _DSPError(0,"DSPRawCloseSaveState: Could not free s_wd_reply_port");
    
    if (_DSPVerbose && s_mk_system)
      fprintf(stderr,"Time spent blocked in msg_send:\n"
	      "\t maximum = %d (msec)\n"
	      "\t total	  = %d (msec)\n",
	      s_max_block_time*0.001,
	      s_all_block_time*0.001);
    
    s_max_block_time = 0;
    s_all_block_time = 0;
    s_prvtime = 0;
    s_curtime = 0;
    s_deltime = 0;

    s_frozen = 0;

    s_systemImage = 0;
    
    DSPCloseWhoFile();
    
    return(ec);
}

int DSPRawClose(void)
{
    int i,ec;
    
    ec =DSPRawCloseSaveState(); /* close DSP without clearing state */
    
    s_mapped_only = 0;
    s_logged = 0;
    s_low_srate = 1;
    s_srate=22050.0;
    s_sound_out=0;
    s_host_msg=0;

    s_dsp_data_ptr = 0;
    s_dsp_data_0 = 0;

    s_write_data=0;
    s_stop_write_data=0;
    s_write_data_running=0;
    s_wd_sample_count=0;
    s_wd_timeout=_DSPMK_WD_TIMEOUT;
    s_wd_fn=0;
    s_wd_fp=0;

    s_read_data=0;
    s_stop_read_data=0;
    s_read_data_running=0;
    s_rd_sample_count=0;
    s_rd_fn=0;
    s_rd_fd=0;

    s_ssi_sound_out=0;
    s_small_buffers = 0;
    s_ap_mode=0;
    s_system_link_file = DSP_AP_SYSTEM_0;
    s_system_binary_file = DSP_AP_SYSTEM_BINARY_0;
    s_system_map_file = DSP_AP_SYSTEM_MAP_FILE_0;
    s_ap_system = 1;
    s_mk_system = 0;
    s_idsp = 0;			/* Assigned DSP number */

    s_prvtime = 0;
    s_curtime = 0;
    s_deltime = 0;
    
    if (s_dspcmd_msg) {
	free(s_dspcmd_msg);
	s_dspcmd_msg = 0;
    }

    if (s_dsprcv_msg) {
	free(s_dsprcv_msg);
	s_dsprcv_msg = 0;
    }

    /* DO NOT FREE s_driver_reply_msg */

    if (!s_wd_rmsg) {
	free(s_wd_rmsg);
	s_wd_rmsg = 0;
    }

    return(ec);
}


int DSPClose(void)		/* close DSP device */
{
    int ec = 0;
    if (!s_open)
      return 0;

    DSPMKFlushTimedMessages();	/* Not that they can get much done! */
    
    if (s_sound_out || s_ssi_sound_out)
      DSPMKStopSoundOut();
    
    return (DSPRawClose());
}


int DSPCloseSaveState(void)		/* close DSP device, saving open state */
{
    int ec = 0;
    if (!s_open)
      return 0;
    
    if (s_sound_out || s_ssi_sound_out)
      DSPMKStopSoundOut();
    
    if (s_write_data)
      DSPMKStopWriteData();
    
    if (s_read_data)
      DSPMKStopReadData();
    
    return (DSPRawCloseSaveState());
}


int _DSPMapHostInterface(void)	/* Memory-map DSP Host Interface Registers */
{
    char *alloc_addr;		/* page address for DSP memory map */
    int sz,i,ec;
    /*** FIXME: Test permissions here and fail gracefully if not enough ***/

    if (s_hostInterface)
      return 0;			/* Already have it */

#if MMAP
    if (!s_dsp_fd)
      s_dsp_fd = open("/dev/dsp",O_RDWR,0);	/* need <sys/file.h> */

    if (s_dsp_fd == -1) {
	s_dsp_fd = 0;
	return(_DSPError(DSP_EMISC,
			 "DSPMapHostInterface: open /dev/dsp failed"));
    }
#endif

    /* If !MMAP, we create a dummy hostInterface which we can write to */
    if (( alloc_addr = (char *)valloc(sz=getpagesize()) ) == NULL)
      return(_DSPError(ENOMEM,"DSPMapHostInterface: memory overflow")); 
    
    s_hostInterface = (DSPRegs *)alloc_addr;
    
#if MMAP
    ec = mmap(alloc_addr,
	      getpagesize(),		/* Must map a full page */
	      PROT_READ|PROT_WRITE,	/* read-write access */
	      MAP_SHARED,		/* shared access (of course) */
	      s_dsp_fd,		/* This device */
	      0);			/* 0 offset */
    /*	 P_DSP);		/* Offset of DSP host i/f from <next/cpu.h> */
    if (ec == -1) {
	s_hostInterface = 0;
	return _DSPError(ec,"DSP Host Interface mmap failed.");
    }
#endif

    return 0;
}


static int s_initMessageFramesAndPorts()
/*
 * Set up ports for communication with the DSP.
 */
{
    int i,ec;
    ec = snddriver_get_dsp_cmd_port(s_sound_dev_port, 
			       s_dsp_owner_port, 
			       &s_dsp_hm_port);
    
    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"s_initMessageFramesAndPorts: "
			   "snddriver_get_dsp_cmd_port failed.");
    

    /* Initialize reuseable receive-data message */
    if (s_dsprcv_msg)
      free(s_dsprcv_msg);
    s_dsprcv_msg = _DSP_dsprcv_msg(s_dsp_hm_port,s_dsp_dm_port);
    
    /* Initialize receive-data initial pointer */
    s_dsp_data_0 = (int *)&(((snd_dsp_msg_t *)s_dsprcv_msg)->data[0]);
    
    /* Initialize reuseable DSP command message */
    if (s_dspcmd_msg)
      free(s_dspcmd_msg);
    s_dspcmd_msg = _DSP_dspcmd_msg(s_dsp_hm_port,s_dsp_dm_port,DSP_MSG_LOW,0);
    
    /* Initialize reuseable driver reply message - DO NOT FREE IT */
    s_driver_reply_msg = _DSP_dspreply_msg(s_driver_reply_port);
    
    return 0;
    
}

static int s_setupProtocol()
/*
 * Set up data streams and DSP driver protocol flags according to 
 * whether sound output and/or write-data is desired.
 */
{
    int ec;
    
    s_dsp_mode_flags = 0;
    
    if (s_host_msg) {
	s_dsp_mode_flags |= SND_DSP_PROTO_HOSTMSG;
	/* s_dsp_mode_flags |= SND_DSP_PROTO_TXD; */
	/* 2nd DSP interrupt to driver */
    }

    if (s_sound_out && s_srate != DSPMK_HIGH_SAMPLING_RATE &&
	s_srate != DSPMK_LOW_SAMPLING_RATE) {
	_DSPError1(DSP_EMISC,
		   "s_setupProtocol: Cannot set up sound-out stream "
		   "when using non-standard sampling rate. "
		   "Changing sampling rate to %s Hz.",
		   _DSPCVS(DSPMK_LOW_SAMPLING_RATE));
	DSPMKSetSamplingRate(DSPMK_LOW_SAMPLING_RATE);
    }

    if (s_mapped_only)
      return 0;
    
    /*
     * Determine DSP DMA buffer sizes for input and output.
     */
    if (s_read_data && (s_write_data || s_sound_out)) {
	s_dsp_buf_wds >>= 1;	/* cut in half for full duplex operation */

	/* FIXME - Tell the DSP the WD buffer size has halved! */
	return _DSPError(DSP_EMISC,"setupProtocol: read-data not ready");
    }

    /*
     * Set up stream into DSP, if read-data enabled.
     */
    if (s_read_data) {

	if (s_sound_out)
	  s_stream_configuration = (s_low_srate ?
				    SNDDRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_22 :
				    SNDDRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_44);
	else 
	  s_stream_configuration = SNDDRIVER_STREAM_TO_DSP;
	ec = snddriver_stream_setup (
				 s_sound_dev_port,
				 s_dsp_owner_port,
				 s_stream_configuration,
				 s_dsp_buf_wds, /* SAMPLES per buffer */
				 2,		    // bytes per sample
				 s_low_water,	    // low water mark
				 s_high_water,	    // high water mark
				 &s_dsp_mode_flags, // modified dsp protocol
				 &s_rd_stream_port);/* returned stream_port */
	if (ec != KERN_SUCCESS)
	  return _DSPMachError(ec,"DSPObject.c: can't setup read-data stream");
    } else
      s_rd_stream_port = 0;

    /*
     * Set up stream out of DSP, if write-data enabled.
     * The "write-data" stream is also used as a handle on the 
     * DSP->soundOut link when there is no read-data or write-data.
     */
    if (s_write_data)
      s_stream_configuration = SNDDRIVER_STREAM_FROM_DSP;
    else if (s_sound_out && !s_read_data)
      s_stream_configuration = (s_low_srate ?
				SNDDRIVER_STREAM_DSP_TO_SNDOUT_22 :
				SNDDRIVER_STREAM_DSP_TO_SNDOUT_44 );
    else s_stream_configuration = 0;
    
    if (s_sound_out) {
	s_so_buf_bytes = (s_small_buffers? 
			  2*s_dsp_buf_wds : _DSPMK_LARGE_SO_BUF_BYTES);
	ec = snddriver_set_sndout_bufsize( 
				   s_sound_dev_port,
				   s_dsp_owner_port,
				   s_so_buf_bytes);
	if (ec != KERN_SUCCESS)
	  return 
	    _DSPMachError(ec,"DSPObject.c: "
		  "snddriver_set_sndout_bufsize() failed");
	/*** FIXME: call snddriver_set_sndout_bufcnt() also ***/
    }

    if (s_stream_configuration) {
	ec = snddriver_stream_setup (
				 s_sound_dev_port,
				 s_dsp_owner_port,
				 s_stream_configuration,
				 s_dsp_buf_wds, /* SAMPLES per buffer */
				 2,		    // bytes per sample
				 s_low_water,	    // low water mark
				 s_high_water,	    // high water mark
				 &s_dsp_mode_flags, // modified dsp protocol
				 &s_wd_stream_port);/* returned stream_port */

	if (ec != KERN_SUCCESS)
	  return _DSPMachError(ec,"DSPObject.c:snddriver_stream_setup failed");
    } else
      s_wd_stream_port = 0;

    /*
     * Set up DSP protocol.
     * This must be one AFTER setting the DMA buffer sizes (?).
     */
    ec = snddriver_dsp_protocol(s_sound_dev_port, s_dsp_owner_port, 
				s_dsp_mode_flags);
    if (ec != KERN_SUCCESS)
      return _DSPMachError(ec,"DSPObject.c: snddriver_dsp_protocol failed.");
    
    return 0;
}

int DSPMKSoundOutDMASize(void)
{
    return s_dsp_buf_wds>>1;	/* Single DMA buffer size in 16-bit words */
}


/******************************* DSP File Locking ****************************/

#include <pwd.h>		/* for getpwuid */
static struct passwd *pw;

int DSPOpenWhoFile(void)
{
    static char *lname;
    time_t tloc;
    
    umask(0); /* Don't CLEAR any filemode bits (arg is backwards!) */
    if (!s_whofile_fp)
      if ((s_whofile_fp=fopen(DSP_WHO_FILE,"w"))==NULL)
	return _DSPError1(0,"*** DSPOpenWhoFile: Could not open %s."
			  " Perhaps the file is write protected."
			  " We will continue without it."
			  " (Mach port ownership, not the lock file,"
			  " is used to arbitrate use of the DSP.)",
			  DSP_WHO_FILE); 
    tloc = time(0);

#ifdef GETPWUID_BUG_FIXED
    if (!lname)
      lname = getlogin();
    if (!lname) {
	    pw = getpwuid(getuid());
	    if (pw)
	      lname = pw->pw_name;
	    else
	      lname = "<user not in /etc/passwd>";
    }
    fprintf(s_whofile_fp,"DSP opened in PID %d by %s on %s\n",
	    getpid(),lname,ctime(&tloc));
#else	   
    fprintf(s_whofile_fp,"DSP opened in PID %d on %s\n",
	    getpid(),ctime(&tloc));
#endif

    fflush(s_whofile_fp);
    return 0;
}      

int DSPCloseWhoFile(void)
{
    time_t tloc;
    fclose(s_whofile_fp);
    s_whofile_fp = 0;
    unlink(DSP_WHO_FILE);
    return 0;
}

char *DSPGetOwnerString(void)
{
    FILE *lockFP;
    char linebuf[_DSP_MAX_LINE]; /* input line buffer */
    
    if ((lockFP=fopen(DSP_WHO_FILE,"r"))==NULL)
      return NULL;
    
    if (fgets(linebuf,_DSP_MAX_LINE,lockFP)==NULL) {
	_DSPError1(-1,"DSPOpenNoBoot: Could not read %s\n",
		   DSP_WHO_FILE);
	fclose(lockFP);
	return NULL;
    }
    fclose(lockFP);
    if (linebuf[strlen(linebuf)-1]=='\n')
      linebuf[strlen(linebuf)-1]='\0';
    return _DSPCopyStr(linebuf);
}


int DSPOpenNoBootHighPriority(void)
{
    DSPSetOpenPriority(1);
    return DSPOpenNoBoot();
}	

/******************************* Mach Interface ******************************/
int notify_switch=1;

static int s_notifyingMsgSend(void)
/*
 * Send Mach message.
 */
{
    int ec;
    int toc;	// time-out count for msg_receive()
    int rpe=0; 	// reply port exists

/* 
 * Normally, messages to send have a null "local port" in order
 * to suppress a reply.  If there is a local port, we use it. 
 */
    if (s_dspcmd_msg->msg_local_port && (s_dspcmd_msg->msg_local_port != thread_reply()))
        _DSPError(DSP_EMACH,"DSPObject.c: s_msgSend: "
	   "Reply port in Mach message not thread_reply() for SEND_NOTIFY reply.");
   
    if (s_dspcmd_msg->msg_local_port)
        rpe=1;
    else
        s_dspcmd_msg->msg_local_port = thread_reply();

    if (notify_switch)
    	ec = msg_send(s_dspcmd_msg, SEND_NOTIFY,0); 
    else
    	ec = msg_send(s_dspcmd_msg, MSG_OPTION_NONE,0); 
    
    if (ec == KERN_SUCCESS)
	return 0;

    if (ec != SEND_WILL_NOTIFY)
        return _DSPError(DSP_EMACH,"DSPObject.c: s_msgSend: "
			 "Did not get will-notify or success from msg_send().");
    

    /*
     * *** FIXME: The following for loop always times out without ever getting the
     * notify message.  We do get several SND messages (ill_msgid 200 => SND_MSG_DSP_MSG)
     * which are probably due to our setting msg_local_port in the message sent when it 
     * otherwise would have been NULL.  (The sound driver suppresses replies when the
     * local port is NULL in the sent message.)  It seems the NOTIFY_MSG_ACCEPTED message
     * is getting lost.
     */
    for(toc=0; toc<10; toc++) {
    
    	_DSP_dsprcv_msg_reset(s_dsprcv_msg,s_dsp_hm_port,s_dspcmd_msg->msg_local_port);

    	ec = msg_receive(s_dsprcv_msg, RCV_TIMEOUT, 100); /* wait 100 ms */
    
    	if (ec == RCV_TIMED_OUT)
	    continue;

	if ( s_dsprcv_msg->msg_id == NOTIFY_MSG_ACCEPTED ) 
	    break;
	else {
	    if ( s_dsprcv_msg->msg_id == NOTIFY_PORT_DELETED )
	    	return _DSPError(DSP_EMACH,"DSPObject.c: s_msgSend: "
	    		"Got NOTIFY_PORT_DELETED message waiting for msg_send() to unblock.");
	    
    	    if (rpe)
      		_DSPError(DSP_EMACH,"_DSPAwaitMsgSendAck: "
			 "Original message had a reply_port, "
			 "and we may have got its msg while waiting for NOTIFY_MSG_ACCEPTED");
    	    if (s_dsprcv_msg->msg_id == SND_MSG_ILLEGAL_MSG)
      		_DSPError1(DSP_EMACH,"_DSPAwaitMsgSendAck: "
			 "Got reply to msg_id %s instead of SEND_NOTIFY",
			 _DSPCVS(((snd_illegal_msg_t *) s_dsprcv_msg)->ill_msgid));
	    else
 	    	_DSPError1(DSP_EMACH,"DSPObject.c: s_msgSend: "
		"msg_id %s in reply not recognized while waiting for NOTIFY_MSG_ACCEPTED. \n"
		";; Look in /usr/include/nextdev/snd_msgs.h for 300 series messages,\n "
		";; and /usr/include/sys/notify.h for 100 series messages.\n"
		";; THROWING THIS MESSAGE AWAY and continuing.",
		_DSPCVS(s_dsprcv_msg->msg_id));
	}
    }
    return 0;
}

static int s_msgSend(void)
/*
 * Send Mach message.
 */
{
    int ec;

    if (s_saving_commands) {
	if (fwrite((void *)s_dspcmd_msg, 1, s_dspcmd_msg->msg_size,
	    s_commands_fp) != s_dspcmd_msg->msg_size)
	    return _DSPError(EIO, "Could not write message to dsp commands file");
	s_commands_numbytes += s_dspcmd_msg->msg_size;
    }
	    
    ec = msg_send(s_dspcmd_msg, SEND_TIMEOUT,_DSP_MACH_SEND_TIMEOUT); 
    
    while (ec == SEND_TIMED_OUT) {
	/* 
	* If we get stuck here, consider the possibility that the DSP is
	* sending error messages that aren't being read.  This will block
	* the DSP driver when its 512-word receive buffer fills up. 
	*/
	DSPGetHostTime();
	ec = msg_send(s_dspcmd_msg, SEND_TIMEOUT,_DSP_MACH_SEND_TIMEOUT); 
	DSPGetHostTime();
	s_all_block_time += s_deltime;
	if (s_deltime > s_max_block_time)
	    s_max_block_time = s_deltime;
    }

    if (ec != KERN_SUCCESS)
	return _DSPMachError(ec,"DSPObject.c: s_oldMsgSend: msg_send failed.");

    return 0;
}

int DSPAlternateReset(void)
{
    int ec;

    if (s_joint_owner)
      return _DSPMachError(ec,"DSPReset: Don't have hm port when joint owner");

    ec = snddriver_dsp_reset(s_dsp_hm_port,s_cur_pri);
    if (ec != KERN_SUCCESS)
      return (_DSPMachError(ec,"DSPAlternateReset: snddriver_dsp_reset failed."));
    return ec;
}

int DSPReset(void)
/* 
 * Reset the DSP.
 * On return, the DSP should be awaiting a 512-word bootstrap program.
 */
{
    int ec;
    port_t neg_port;

    if (!s_joint_owner)
      return DSPAlternateReset();

    ec = snddriver_set_dsp_owner_port(s_sound_dev_port, 
				      s_dsp_owner_port, 
				      &neg_port);
    if (ec != KERN_SUCCESS) {
	_DSPMachError(ec,"DSPReset: "
		      "snddriver_set_dsp_owner (for reset) failed. "
		      "Trying DSPClose().");
	DSPClose();		/* This should also cause a reset */
	ec = DSPOpenNoBoot();
	if (ec)
	  _DSPError(ec,"DSPReset: Could not reset via close either.");
    }
    return ec;
}

/*** FIXME: Describe state variables in comment below ***/

int DSPOpenNoBoot(void)		/* open current DSP */
{
    /*** FIXME: Move outside of single DSP instance ***/
    int ec,ecs;
    port_t neg_port;
    
    /*** DISABLED: malloc_debug(5); ***/

    if (s_open) 
      return(_DSPError(0,"DSPOpenNoBoot: DSP is already open. Returning..."));
    
    if (!s_sound_dev_port) {
	s_dsp_access_flags = SND_ACCESS_DSP;
	if (s_sound_out)
	  s_dsp_access_flags |= SND_ACCESS_OUT;
	ec = SNDAcquire(s_dsp_access_flags, 0, 0, 0, NULL_NEGOTIATION_FUN,
			0, &s_sound_dev_port, &s_dsp_owner_port);
	if (ec != KERN_SUCCESS)
	  return _DSPMachError(ec,"DSPOpenNoBoot: "
			       "SNDAcquire() failed to get the sound device "
			       "and the DSP");
    }
    
    cthread_init();		/* used for write data, dsp errors, abort */
    
    if (ec == KERN_SUCCESS) 
      DSPOpenWhoFile();		/* Tell the world who has the DSP */
    else {			/* could not become owner of DSP */
	/*
	 * Existing DSP owner or negotiation port is returned in neg_port.
	 * Assume it is the DSP owner port and try to continue since there
	 * is no negotiation protocol defined at the time of this writing.
	 * 6/17/89/jos
	 */
	
	/* If we can't open the DSP, we can't own its error log either */
	/* Hence, we fprintf to stderr in this situation */
	
	char *os = DSPGetOwnerString();
	
	if (os)
	  _DSPError(0,os);
	else
	  _DSPError1(-1,"DSPOpenNoBoot: Could not read %s to find out "
		     " what process has the DSP open.",
		     DSP_WHO_FILE);
	
	if (s_open_priority < 1) { /* give up */
	    DSPSetErrorFP(stderr);
	    _DSPMachError(ec,"DSPOpenNoBoot: Could not become DSP owner");
	
	    s_freePort(&s_dsp_owner_port);
	    return DSP_EMACH;
	}
	else {
	    _DSPError(0,"DSPOpenNoBoot: Obtaining joint DSP ownership");
	    if (s_dsp_owner_port == neg_port)
	      return _DSPMachError(ec,"DSPOpenNoBoot: "
				   "libsys/sounddriver_client.c/"
				   "snddriver_set_dsp_owner_port "
				   "did not give us the DSP owner port");
	    else {
		s_freePort(&s_dsp_owner_port);
		s_dsp_owner_port = neg_port; /* This gives us co-ownership */
		s_joint_owner = 1; /* Call _DSPOwnershipIsJoint() to detect */
		return DSP_EMISC;
	    }
	}
    }

    if (s_sound_out) {
	
	neg_port = s_dsp_owner_port;
	ec = snddriver_set_sndout_owner_port(s_sound_dev_port, 
					     s_dsp_owner_port, 
					     &neg_port);
	if (ec != KERN_SUCCESS)
	  return _DSPMachError(ec,"DSPOpenNoBoot: "
			       "Could not obtain sound-out ownership");
    }				/* if (s_sound_out) */
    
    s_hostInterface = NULL;
    
#if MMAP
    /* DSP regs mapped always (for gdb access) */
    if(ec=_DSPMapHostInterface())	
      _DSPError(ecs=ec,"DSPOpenNoBoot: Could map DSP host interface");
    if (!s_mapped_only)
      s_hostInterface->icr = 1; /* dspmmap in dsp.c clears this */
#endif
    
    /* s_dsp_hm_port is allocated by kernel. */
    s_allocPort(&s_dsp_dm_port);
    s_allocPort(&s_dsp_err_port);

    s_allocPort(&s_driver_reply_port);
    
    if (ec=s_initMessageFramesAndPorts())
      _DSPError(ecs=ec,"DSPOpenNoBoot: Could not set up DSP streams");
    
    if(ec=s_setupProtocol())
      _DSPError(ecs=ec,"DSPOpenNoBoot: "
		"Could not set up sound-out stream");
    /* DSPHostMessage(DSP_HM_DMA_WD_HOST_ON); (done in DSPMKInit) */
    
    s_open = 1;
    
    if (!s_simulated)
      s_simulator_fp = NULL;
    if (!s_saving_commands)
      s_commands_fp = NULL;
    
    s_systemImage = NULL;
    
    if (s_host_msg && !s_no_thread && !s_mapped_only)
      s_dsp_err_thread = cthread_fork((cthread_fn_t)s_dsp_err_reader,0);
    
    s_max_block_time = 0;
    s_all_block_time = 0;
    s_prvtime = 0;
    s_curtime = 0;
    s_deltime = 0;

    return(ec);
}


int _DSPOpenMapped(void) 
{
    _DSPEnableMappedOnly();
    return(DSPOpenNoBoot());
}

/************************** SIMULATOR FILE PRINTING *************************/

static char *s_decodeReg(reg,val) 
    int reg,val;
    /* 
     * Return string describing host interface register.
     */
{
    int r,v, hv;
    char *str = _DSPMakeStr(200,NULL);
    char *hcname;
    r = reg & 7;
    if (r!=reg) 
      _DSPError(EDOM,
		"s_decodeReg: DSP host-interface address out of range");
    v = val & 0xFF;
    if (v!=val) 
      _DSPError(EDOM,"s_decodeReg: DSP host-interface byte out of range");
    
    switch (r) {
    case DSP_ICR:
	sprintf(str,"ICR: %s %s %s %s %s %s %s",
		v & DSP_ICR_INIT ? "INIT" : "",
		v & DSP_ICR_HM1	 ? "HM1"  : "",
		v & DSP_ICR_HM0	 ? "HM0"  : "",
		v & DSP_ICR_HF1	 ? "HF1"  : "",
		v & DSP_ICR_HF0	 ? "HF0"  : "",
		v & DSP_ICR_TREQ ? "TREQ" : "",
		v & DSP_ICR_RREQ ? "RREQ" : "");
	break;
    case DSP_CVR:
	
	hv = v & DSP_CVR_HV_MASK;
	
	switch(hv) {
	case DSP_HC_RESET:
	    hcname = "RESET";
	    break;
	case DSP_HC_TRACE:
	    hcname = "TRACE";
	    break;
	case DSP_HC_SOFTWARE_INTERRUPT:
	    hcname = "SW int";
	    break;
	case DSP_HC_EXECUTE_HOST_MESSAGE:
	    hcname = "exec host msg";
	    break;
	case DSP_HC_HOST_W_DONE:
	    hcname = "HOST_W_DONE";
	    break;
	default:
	    hcname = "*** UNKNOWN ***";
	}
	
	sprintf(str,"CVR: $%X, %s Vector=0x%X (hc %d = %s)",
		v,  v & DSP_CVR_HC_MASK ? "HC" : "(NO HC)",
		hv, hv, hcname );
	break;
    case DSP_ISR:
	sprintf(str,"ISR: %s %s %s %s %s %s %s",
		v & DSP_ISR_HREQ ? "HREQ" : "",
		v & DSP_ISR_DMA	 ? "DMA"  : "",
		v & DSP_ISR_HF3	 ? "HF3"  : "",
		v & DSP_ISR_HF2	 ? "HF2"  : "",
		v & DSP_ISR_TRDY ? "TRDY" : "",
		v & DSP_ISR_TXDE ? "TXDE" : "",
		v & DSP_ISR_RXDF ? "RXDF" : "");
	break;
    case DSP_IVR:
	sprintf(str,"IVR: Vector=0x%X",v);
	break;
    case DSP_UNUSED:
	sprintf(str,"*** UNUSED DSP REGISTER *** Contents=0x%X",v);
	break;
    case DSP_TXH:
	sprintf(str,"TXH: $%02X",v);
	break;
    case DSP_TXM:
	sprintf(str,"TXM: $%02X",v);
	break;
    case DSP_TXL:
	sprintf(str,"TXL: $%02X",v);
	break;
    default:
	sprintf(str,"???: $%02X",v);
    }
    
    return(str);
}

static int s_simPrintFNC(fp,reg,val) /* print host reg in simulator format */
    FILE *fp;	/* mach port for simulator file, open for write */
    int reg;	/* host-interface register to write (0:7) */
    int val;	/* least-significant 8 bits written */
{
    int r;
    r = reg & 7;
    if (r!=reg) 
      _DSPError(EDOM,"s_simPrintFNC: "
		"DSP host-interface register out of range");
    r += 8*0; /* or in the r/w~ bit */
    fprintf(fp,"4%01X%02X ",r,val);
    return 0;
}

static int s_simPrintF(fp,reg,val) 
    /* 
     * print host interface reg in simulator format 
     */
    FILE *fp;	/* file pointer for simulator file, open for write */
    int reg;	/* host-interface register to write (0:7) */
    int val;	/* least-significant 8 bits written */
{
    char *regstr;
    s_simPrintFNC(fp,reg,val); /* Print all but comment */
    regstr = s_decodeReg(reg,val);
    fprintf(fp,"\t\t ; %s [%d]\n",regstr,DSPGetHostTime()); /* add comment */
    free(regstr);
    return 0;
}

static int s_simReadRX(word) 
    int word;	/* Actual value of RX read in mapped mode */
    /* 
     * read host interface RX reg in simulator file 
     */
{
    unsigned int usword; 
    int sword; 
    float fword;
    usword = word;
    if (word & (1<<23)) usword |= (0xFF << 24); /* ignore overflow */
    sword = usword; /* re-interpret as signed */
    fword = sword*0.00000011920928955078125; /* 1/2^23 */
    if (s_simulated)
      fprintf(s_simulator_fp,"4D00 4E00 4F00   ; RX : $%06X = `%-8d = %10.8f = %s [%d]\n",
	      word&0xFFFFFF,sword,fword,DSPMessageExpand(word),DSPGetHostTime());
    return 0;
}

static int s_simWriteTX(word) 
    int word;	/* Value of TX to write */
    /* 
     * Record write of host interface TX register to simulator file 
     */
{
    unsigned int usword; 
    int sword,i; 
    float fword;
    
    usword = word;
    if (word & (1<<23)) usword |= (0xFF << 24); /* ignore overflow */
    sword = usword; /* re-interpret as signed */
    fword = sword*0.00000011920928955078125; /* 1/2^23 */
    /* The 4F00 below reads RXL to avoid blocking of host output */
    s_simPrintFNC(s_simulator_fp, DSP_TXH, word>>16);
    s_simPrintFNC(s_simulator_fp, DSP_TXM, (word>>8)&0xFF);
    s_simPrintFNC(s_simulator_fp, DSP_TXL, word&0xFF);
    fprintf(s_simulator_fp,"\t ; TX : $%06X = `%-8d = %10.8f [%d]\n",
	    word,sword,fword,DSPGetHostTime());
    return 0;
}

/*************************** READING DSP REGISTERS ***************************/

static int s_regs,s_icr,s_cvr,s_isr,s_ivr;

static int _DSPUpdateRegBytes()
{
    s_icr = (s_regs >> 24) & 0xFF;
    s_cvr = (s_regs >> 16) & 0xFF;
    s_isr = (s_regs >>	8) & 0xFF;
    s_ivr = (s_regs	 ) & 0xFF;
    return 0;
}


int _DSPReadRegs()
/* 
 * Return first four DSP host interface register bytes (ICR,CVR,ISR,IVR)
 * in *regsP.  Returns 0 on success.  Calls to this routine do not affect
 * the simulator output file because they do not affect the state of the DSP.
 */
{
    int ec;
    
    CHECK_ERROR_1;
    
    if (s_mapped_only && MMAP)
      s_regs = *(int *)s_hostInterface;
    else {

	ec = snddriver_dspcmd_req_condition(s_dsp_hm_port,0,0,
					    s_cur_pri,s_dsp_owner_port);
	/*
	 * Get the reply containing the DSP registers.
	 */
	_DSP_dsprcv_msg_reset(s_dsprcv_msg,s_dsp_hm_port,s_dsp_owner_port);
	
	ec = msg_receive(s_dsprcv_msg, RCV_TIMEOUT, _DSP_MACH_RCV_TIMEOUT);
	
	if (ec == RCV_TIMED_OUT)
	  return _DSPMachError(ec,"_DSPReadRegs: "
				"Timed out reading DSP regs!");

	if (ec != KERN_SUCCESS)
	  return _DSPMachError(ec,"_DSPReadRegs: msg_receive failed.");

	if (s_dsprcv_msg->msg_id != SND_MSG_DSP_COND_TRUE) /* snd_msgs.h */
	  return (_DSPError1(DSP_EMACH,"_DSPReadRegs: "
			     "Unrecognized msg id %s",
			     _DSPCVS(s_dsprcv_msg->msg_id)));

	s_regs = ((snd_dsp_cond_true_t *)s_dsprcv_msg)->value; /* snd_msgs.h */
    }

    _DSPUpdateRegBytes();
    
    return 0;
}

int _DSPPrintRegs()
/* 
 * Print first four DSP host interface register bytes (ICR,CVR,ISR,IVR).
 */
{
    int ec;
    
    ec = _DSPReadRegs();
    
    if (ec)
      return (_DSPError(ec,"_DSPPrintRegs: _DSPReadRegs() failed."));
    
    printf(" icr = 0x%X",s_icr);
    printf(" cvr = 0x%X",s_cvr);
    printf(" isr = 0x%X",s_isr);
    /* printf(" isr = 0x%X",s_isr); */
    printf("\n");
    
    return 0;
}

int DSPReadICR(
    int *icrP)		
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPReadICR:\n");
    
    DSP_UNTIL_ERROR(_DSPReadRegs());
    *icrP = s_icr;
    if (_DSPTrace & DSP_TRACE_DSP) 
      printf("\tICR[%d]	 =  0x%X\n",s_idsp,*icrP);
    return 0;
}


int DSPGetHF0(void)
{
    int icr;
    
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPGetHF0:\n");
    
    DSP_UNTIL_ERROR(DSPReadICR(&icr));
    return((icr&DSP_ICR_HF0)!=0);
}

int DSPGetHF1(void)
{
    int icr;
    
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPGetHF1:\n");
    
    DSP_UNTIL_ERROR(DSPReadICR(&icr));
    return((icr&DSP_ICR_HF1)!=0);
}

int DSPReadCVR(
    int *cvrP)
{
    DSP_UNTIL_ERROR(_DSPReadRegs());
    *cvrP = s_cvr;
    
    if (_DSPTrace & DSP_TRACE_DSP) 
      printf("\tCVR[%d]	 =  0x%X\n",s_idsp,*cvrP);
    
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPReadCVR:\n");
    
    return 0;
}

int DSPReadISR(int *isrP)
{
    DSP_UNTIL_ERROR(_DSPReadRegs());
    *isrP = s_isr;
    if (_DSPTrace & DSP_TRACE_DSP) 
      printf("\tISR[%d]	 =  0x%X\n",s_idsp,*isrP);
    
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPReadISR:\n");
    
    return 0;
}

/*** FIXME: Need global time-out variable that user can set here ***/

int DSPReadRX(DSPFix24 *wordp)
{
    int i,ec;
    if (s_dsp_data_waiting)
      return _DSPReadDatum(wordp);
    if (ec=DSPAwaitData(DSPDefaultTimeLimit))
      return ec;
    return _DSPReadDatum(wordp);
}

int DSPReadRXArray(DSPFix24 *data, int nwords)
{  
    register unsigned char *bp;
    int i,j;
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPReadRXArray:\n");
    
    if (s_mapped_only && MMAP) {
	for (j=0;j<nwords;j++) {
	    while (!RXDF) {
		int i = 0;
		if (i++>1000)	/* 1 second */
		  return _DSPError(DSP_ETIMEOUT,
				   "DSPReadRXArray: Timed out "
				   "waiting for RXDF in DSP");
		select(0,0,0,0,&_DSPOneMillisecondTimer);
	    }
	    data[j] = s_readRX();
	}
    } else {
	for (i=0;i<nwords;i++)
	  if(DSPReadRX(&(data[i])))
	    return(i+1);
    }
    return 0;
}    

/*************************** WRITING DSP REGISTERS ***************************/

int DSPWriteTX(DSPFix24 word)			
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPWriteTX:\n");
    return _DSPWriteDatum(word);
}

int DSPWriteTXArray(DSPFix24 *data, int nwords)
{  
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPWriteTXArray:\n");
    return _DSPWriteData(data,nwords);
}    

int DSPWriteTXArrayB(DSPFix24 *data, int nwords)
{  
    int i;
    DSPFix24 *rdata = (DSPFix24 *) alloca(nwords*sizeof(int));
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSPWriteTXArrayB:\n");
    for (i=0;i<nwords;i++)
      rdata[i] = data[nwords-i-1];
    return _DSPWriteData(rdata,nwords);
}    

int DSPGetICR(void)
{
    int icr;
    DSP_UNTIL_ERROR(DSPReadICR(&icr));
    return icr;
}

int DSPGetCVR(void)
{
    int cvr;
    DSP_UNTIL_ERROR(DSPReadCVR(&cvr));
    return cvr;
}

int DSPGetISR(void)
{
    int isr;
    DSP_UNTIL_ERROR(DSPReadISR(&isr));
    return isr;
}

int DSPGetRX(void)
{
    int rx;
    DSP_UNTIL_ERROR(DSPReadRX(&rx));
    return rx;
}

/************************ SIMULATOR STREAM MANAGEMENT ***********************/

int DSPOpenSimulatorFile(char *fn)			
{
    if (!fn) {
	fn = "dsp000.io";
	if (s_idsp>999) return(_DSPError(EDOM,"Too many DSPs for name gen"));
	sprintf(fn,"dsp%03d.io",s_idsp);
	fn[3] += s_idsp;
    }
    if ((s_simulator_fp=fopen(fn,"w"))==NULL)
      return 
	_DSPError1(ENOENT,
		   "_DSPUtilities: Can't open simulator output file '%s'",fn);
    if (_DSPVerbose||_DSPTrace)
      printf("\tWriting simulator output file:\t%s\n",fn);
    
    s_simulated = 1;
    
    setlinebuf(s_simulator_fp); /* turn off buffering */
    fprintf(s_simulator_fp,"delay=2000;\t; *** wait for sim8k.lod reset ***\n");
    fprintf(s_simulator_fp,";; Read a 0 sitting in RX\n");
    s_simReadRX(0);
    fprintf(s_simulator_fp,";; Read DSP_DM_RESET_SOFT (0x10) message\n");
    s_simReadRX(0x100000);
    return 0;
}

int DSPCloseSimulatorFile(void)
{
    int i;
    
    for (i=0;i<20;i++)
      s_simReadRX(-1);		/* Read any waiting messages */
    
    /* fprintf(s_simulator_fp,"6F00#1000\t ;"
       " *** wait a while and then HALT ***\n"); */
    
    /*
      This turns out to be more trouble than it is worth because the orchestra
      has been running while.  It basically restarts the orch loop after an 
      unpredictable running stretch.
      
      fprintf(s_simulator_fp,";; *** HALT for interactive simulation ***\n"
      "6588 663F 67B8	 ; TX : $883FB8 (DSP_HM_HALT)\n"
      "6193		 ; CVR: $93, HC 0x13 (exec host msg)\n\n");
      
      fprintf(s_simulator_fp,";; *** Set time to 0 and GO! ***\n"
      "6588 663F 678A	 ; TX : $883F8A (DSPStart())\n"
      "6193		 ; CVR: $93, HC 0x13 (exec host msg)\n\n");
      */    
    
    fclose(s_simulator_fp);
    s_simulated = 0;
    s_simulator_fp = NULL;		/* DSPGlobals.c */
    if (_DSPVerbose||_DSPTrace)
      printf("\tSimulator output stream closed.\n");
    return 0;
}


int DSPStartSimulatorFP(FILE *fp)
{
    if (fp!=NULL)
      s_simulator_fp=fp;
    if(s_simulator_fp==NULL)
      return(_DSPError(EIO,"DSPStartSimulator: Cannot start. "
		       "No open stream"));
    s_simulated = 1;
    return 0;
}


int DSPStartSimulator(void)
{
    return DSPStartSimulatorFP(s_simulator_fp);
}


int DSPStopSimulator(void)
{
    s_simulated = 0;		/* s_simulator_fp is not changed */
    return 0;
}

/************************ COMMANDS STREAM MANAGEMENT ***********************/

typedef struct {		// Keep in sync with sound library performsound.c
    	int	sampleCount;
	int	dspBufSize;
	int	soundoutBufSize;
	int	reserved;
} commandsSubHeader;

int DSPOpenCommandsFile(
    char *fn)			
{
    int serr;
    SNDSoundStruct dummySoundHeader;
    commandsSubHeader dummySubHeader;
    
    if (s_saving_commands)
        return _DSPError(DSP_EMISC, "Commands file already open");

    if ((s_commands_fp=fopen(fn,"w"))==NULL)
      return(_DSPError1(ENOENT,
			"DSPObject: Can't open commands output file '%s'",fn));
    if (_DSPVerbose||_DSPTrace)
      printf("\tWriting commands output file:\t%s\n",fn);
    
    /* Write dummy header and subheader to soundfile */
    if (fwrite((void *)&dummySoundHeader, sizeof(SNDSoundStruct),
	1, s_commands_fp) != 1)
	return _DSPError(EIO, "Could not write initial header to dsp commands file");
    if (fwrite((void *)&dummySubHeader, sizeof(commandsSubHeader),
	1, s_commands_fp) != 1)
	return _DSPError(EIO, "Could not write initial subheader to"
			      " dsp commands file");
    
    s_commands_numbytes = sizeof(commandsSubHeader);
    s_saving_commands = 1;
    return(0);
}

int DSPCloseCommandsFile(DSPFix48 *endTimeStamp)
{
    static SNDSoundStruct commandsSoundHeader = {
        SND_MAGIC,			/* magic number */
	sizeof(SNDSoundStruct),		/* offset to data */
	0,				/* data size (filled in) */
	SND_FORMAT_DSP_COMMANDS,	/* data format */
	0,			        /* sampling rate (filled in) */
	2				/* channel count */
    };
    commandsSubHeader subheader;
    
    /* Write header and subheader to soundfile */
    if (s_saving_commands) {
        commandsSoundHeader.samplingRate = 
	    (s_srate == DSPMK_HIGH_SAMPLING_RATE ? SND_RATE_HIGH : SND_RATE_LOW);
	commandsSoundHeader.dataSize = s_commands_numbytes;
        rewind(s_commands_fp);
	if (fwrite((void *)&commandsSoundHeader, sizeof(SNDSoundStruct),
	    1, s_commands_fp) != 1)
	    return _DSPError(EIO, "Could not write final header to dsp commands file");
	subheader.sampleCount = (endTimeStamp ? DSPFix48ToInt(endTimeStamp) : 0);
	subheader.dspBufSize = s_dsp_buf_wds;
	subheader.soundoutBufSize = s_so_buf_bytes;
	subheader.reserved = 0;
	if (fwrite((void *)&subheader, sizeof(commandsSubHeader),
	    1, s_commands_fp) != 1)
	    return _DSPError(EIO, "Could not write final subheader to"
	    			  " dsp commands file");
    }
    fclose(s_commands_fp);
    s_saving_commands = 0;
    s_commands_numbytes = 0;
    s_commands_fp = NULL;
    
    if (_DSPVerbose||_DSPTrace)
      printf("\tCommands output stream closed.\n");
    return(0);
}

/* FIXME: is it useful and safe to start and stop commands file? */
int DSPStartCommandsFP(FILE *fp)
{
    if (fp!=NULL)
      s_commands_fp=fp;
    if(s_commands_fp==NULL)
      return(_DSPError(EIO,"DSPStartCommandsFP: Cannot start. "
		       "No open stream"));
    s_saving_commands = 1;
    return(0);
}

int DSPStopCommands(void)
{
    s_saving_commands = 0;		/* s_commands_fp is not changed */
    return(0);
}

/****************** GET/PUT ARRAY TO/FROM DSP HOST INTERFACE *****************/

int DSPWriteArraySkipMode(
    DSPFix24 *data,		/* array to send to DSP (any type ok) */
    DSPMemorySpace memorySpace, /* from <dsp/dspstructs.h> */
    int startAddress,		/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount,		/* from DSP perspective */
    int mode)			/* from <nextdev/dspvar.h> */
{
    int dspack,ec=0;
    int nb,nbdone,nrem;
    int use_dma,use_driver;
    
    if (wordCount <= 0) 
      return 0;
    
#if SIMULATOR_POSSIBLE
    if(s_simulated) 
      fprintf(s_simulator_fp,
	      ";; Load %d words to %s:$%X:%d:$%X:\n",
	      wordCount,DSPMemoryNames[memorySpace],
	      startAddress,skipFactor,
	      startAddress+skipFactor*wordCount-1);
#endif
    
    /* The host-receive interrupt vector (location p:0x20) within 
       the DSP cannot be clobbered */
    
    if (   memorySpace==DSP_MS_P 
	&& startAddress <= 0x21	 
	&& startAddress+wordCount-1 >= 0x20 )
    {
	DSP_MAYBE_RETURN(_DSPError(DSP_EILLDMA,
				    "DSPWriteArraySkipMode: Attempt to clobber"
				    " p:0x20 or p:0x21 refused."));
    }
    
    use_dma = (wordCount >= DSP_MIN_DMA_WRITE_SIZE) && !s_mapped_only && 
      DO_USER_INITIATED_DMA; /*** FIXME: Enable user-initiated DMA ***/

    /*
     * There are two ways to write to the DSP without using DMA.
     * (1) write the memory-mapped DSP registers, bypassing the driver.
     * (2) write using the driver, obtaining mapped writes that way also.
     * Using the driver means DSPWriteRXArray() is called for the transfer,
     * and this only works in 32-bit mode.  Also, if write-data is active,
     * we prefer not to support this even though it should work as long
     * as write-data cannot cause any information to flow to the DSP.
     * Thus, only 32-bit mode transfers to the DSP can occur during write data.
     */

    use_driver = s_write_data || (!do_mapped_array_writes && mode==DSP_MODE32);

    use_driver = use_driver || (!MMAP);

    if (!use_dma) {
	/* Enter pseudo-DMA mode, host-to-DSP */
	if (s_simulated)
	  fprintf(s_simulator_fp,";; DSP_HM_HOST_W: Enter DMA mode\n");

#if MMAP
	if (!use_driver) {
	    /*
	     * MAJOR ASSUMPTION:
	     * There is no output currently going to the DSP,
	     * i.e., the driver is not active wrt the DSP.
	     * Otherwise we might interrupt an operation that
	     * the driver is in the middle of.
	     */
	    register int i,j,*p=data;
	    int dcount,dval;

	    _DSPEnterMappedMode();

	    /* ec = DSPCallV(DSP_HM_HOST_W,
	                     3,(int)memorySpace,startAddress,skipFactor); */

	    /*
	     * Send HOST_W Host Message
	     */

	    i = 0;		/* cumulative Host Message wait count */

	    while (!TXDE) i += 1;
	    TXH = 0;
	    TXM = 0;
	    TXL = memorySpace;

	    while (!TXDE) i += 1;
	    TXH = 0;
	    TXM = (startAddress>>8) & 0xFF;
	    TXL = startAddress & 0xFF;

	    while (!TXDE) i += 1;
	    TXH = 0;
	    TXM = (skipFactor>>8) & 0xFF;
	    TXL = skipFactor & 0xFF;

	    while (!TXDE) i += 1;
	    i = (_DSP_HMTYPE_UNTIMED|DSP_HM_HOST_W);
	    TXH = (i>>16) & 0xFF;
	    TXM = (i>>8) & 0xFF;
	    TXL = i & 0xFF;

	    while (HC) i += 1;
	    while (!TRDY) i += 1;
	    s_hostInterface->cvr = (0x80 | DSP_HC_XHM);

	    /*
	     * Wait until host message is started
	     */
	    while (HC) i += 1;
	    usleep(1);		/* Have 320 ns between !HC and HF2 */
	    while (HF2) i += 1;

	    if (i>max_hm_buzz) {
		max_hm_buzz = i; /* 15 seems typical here */
		if (_DSPVerbose)
		  _DSPError1(0,"DSPWriteArraySkipMode: HM wait-count max "
			     "increased to %s",_DSPCVS(max_hm_buzz));
	    }

	    /*
	     * Send array down
	     */
	    switch (mode) {
	    case DSP_MODE8: {
		register char* c = (char *)data;
		for (i=0,j=wordCount;j;j--) {
		    while (!TXDE) i += 1;
		    s_writeTXLSigned(c++);
		} 
	    } break;
	    case DSP_MODE16: {
		register short* s = (short *)data;
		for (i=0,j=wordCount;j;j--) {
		    while (!TXDE) i += 1;
		    s_writeTXMLSigned(s++);
		} 
	    } break;
	    case DSP_MODE24: {
		register unsigned char* c = (unsigned char *)data;
		unsigned int w;
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--) {
			w = *c++;
			w = (w<<8) | *c++;
			w = (w<<8) | *c++;
			s_writeTX(&w);
		    }
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!TXDE) i += 1;
			w = *c++;
			w = (w<<8) | *c++;
			w = (w<<8) | *c++;
			s_writeTX(&w);
		    } 
		}
	    } break;
	    case DSP_MODE32:
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--)
		      s_writeTX(p++);
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!TXDE) i += 1;
			s_writeTX(p++);
		    } 
		}
		break;
	    case DSP_MODE32_LEFT_JUSTIFIED:
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--) {
			dval = (*p++ >> 8);
			s_writeTX(&dval);
		    }
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!TXDE) i += 1;
			dval = (*p++ >> 8);
			s_writeTX(&dval);
		    } 
		}
		break;
	    default:
		return _DSPError1(EINVAL,"DSPWriteArraySkipMode: "
				  "Unrecognized data mode = %s",_DSPCVS(mode));
		
	    }
	    if (i>max_txde_buzz) {
		max_txde_buzz = i;
		if (_DSPVerbose)
		  _DSPError1(0,"DSPWriteArraySkipMode: TXDE wait-count max "
			     "increased to %s",_DSPCVS(max_txde_buzz));
	    }

	    while (!TRDY) i += 1;

	    /* Exit DMA mode */
	    s_hostInterface->cvr = (0x80 | DSP_HC_HOST_W_DONE);
	    while (HC) i += 1;
	    usleep(1);		/* Have 320 ns between !HC and HF2 */
	    while (HF2) i += 1;

	    _DSPExitMappedMode(); /* Give DSP back to the driver */

	} else {		/* !do_mapped_array_writes */
#else
	if (1) {
#endif
	DWASM_no_map:
	    ec = DSPCallV(DSP_HM_HOST_W,
			  3,(int)memorySpace,startAddress,skipFactor);
	    if (ec)
	      return _DSPError(ec,"DSPWriteArraySkipMode: "
			       "DSPCallV(DSP_HM_HOST_W) failed.");
	
	    ec = DSPWriteTXArray(data,wordCount);

	    if (ec)
	      return _DSPError(ec,"DSPWriteArraySkipMode: "
			       "DSPWriteTXArray failed.");
	    /* Exit DMA mode */
	    if (s_simulated)
	      fprintf(s_simulator_fp,";; Exit DMA mode\n");
	    DSP_UNTIL_ERROR(DSPHostCommand(DSP_HC_HOST_W_DONE));
	}
	
    } else {			/* else use_dma */

	/*
	 * Write the array to the DSP via "user-initiated DMA"
	 */
	int ec;
	int *sdata = 0;
	int *dmadata;
	int bytes_per_sample = 1;
	int protocol = 0;
	int tag = 0;
	
	/*
	 * If the mode is DSP_MODE32 (right-justified), we must
	 * left-justify the data before doing the DMA (sigh).
	 * This is why DSP_MODE32_LEFT_JUSTIFIED was added.
	 */
	if (mode==DSP_MODE32) { /* snd_msgs.h */
	    register int i;
	    register int *s,*d;
	    sdata = (int *)alloca(wordCount * sizeof(int));
	    for (i=wordCount, s=data, d=sdata; i; i--)
	      *d++ = (*s++ << 8);
	    dmadata = sdata;
	} else
	  dmadata = data;
	
#if UI_DMA_CHANDATA_FLUSHED
	/*
	 * "chandata" will not be supported in the driver eventually.
	 * The DSP-side protocol will be assumed unknown by the kernel.
	 */ 
	ec = DSPCallV(DSP_HM_HOST_W,
		      3,(int)memorySpace,startAddress,skipFactor);
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "DSPCallV(DSP_HM_HOST_W) failed.");
#if DO_USER_INITIATED_DMA
	/* Can't even compile because of prototype difference */
	ec = snddriver_dsp_dma_write(s_dsp_hm_port,data,wordCount,mode);
#else
	ec = -1;
#endif
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "snddriver_dsp_dma_write() failed.");
#else	
	ec = _snddriver_libdsp_dma_write(s_dsp_hm_port,startAddress,wordCount,
				     skipFactor,(int)memorySpace,mode,
				     (pointer_t)data);
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "_snddriver_libdsp_dma_write() failed.");
#endif
	
    }
    return 0;
}


static int s_mode_to_width(int mode) {
    int read_width = 0;	
    switch (mode) {
    case DSP_MODE8:
	read_width = 1;
	break;
    case DSP_MODE16:
	read_width = 2;
	break;
    case DSP_MODE24:
    case DSP_MODE32:
    case DSP_MODE32_LEFT_JUSTIFIED:
	read_width = 4;
	break;
    }
    return read_width;
}

int DSPReadArraySkipMode(
    DSPFix24 *data,		/* array to read from DSP (any type ok) */
    DSPMemorySpace memorySpace, /* from <dsp/dspstructs.h> */
    int startAddress,		/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount,		/* from DSP perspective */
    int mode)			/* from <nextdev/dspvar.h> */
{
    int dspack;
    int ec,use_dma,use_driver;

    if (wordCount <= 0) 
      return 0;
    
#if SIMULATOR_POSSIBLE
    if(s_simulated) 
      fprintf(s_simulator_fp,
	      ";; Read %d words from %s:$%X:%d:$%X:\n",
	      wordCount,DSPMemoryNames[memorySpace],
	      startAddress,skipFactor,
	      startAddress+skipFactor*wordCount-1);
#endif
    
    if (s_dsp_data_waiting) {
	_DSPError(0,"DSPReadArraySkipMode: Flushing unread messages/data "
		  "from the DSP");
	DSPFlushMessages();
    }

    use_driver = s_write_data || (!do_mapped_array_reads && mode==DSP_MODE32);

    use_driver = use_driver || (!MMAP);

    use_dma = (use_driver && (wordCount >= DSP_MIN_DMA_READ_SIZE) 
	       && !s_mapped_only && DO_USER_INITIATED_DMA );

    /*
     * There are two ways to read from the DSP without using DMA.
     * (1) read the memory-mapped DSP registers, bypassing the driver.
     * (2) read using the driver, obtaining mapped reads that way too.
     * Using the driver means DSPReadRXArray() is called for the transfer,
     * and this only works in 32-bit mode.  Also, if write-data is active,
     * we CANNOT bypass the driver because we cannot distinguish between
     * write-data and our array transfer by just reading the registers.
     * (Write-data is done using a 16-bit true DMA from the DSP.)
     * The reason to support the limited case (1) is that it is fastest
     * when it can be done.
     */

    DSPClearHF1();		/* Hold off on array transfer until we say */

    if (!(use_dma && !UI_DMA_CHANDATA_FLUSHED)) {
	/* 
	 * Tell the DSP to send the array.
	 */
	
	/* Enter DMA mode, DSP-to-host */
	if (s_simulated)
	  fprintf(s_simulator_fp,
		  ";; DSP_HM_HOST_R: Enter DMA mode, DSP to host\n");
	
	DSP_UNTIL_ERROR(DSPCallV(DSP_HM_HOST_R,
				 3,(int)memorySpace,startAddress,skipFactor));
	
	if (s_simulated)
	  fprintf(s_simulator_fp,
		  ";; Await R_REQ from DSP, possibly after msgs\n");
    
#if DO_USER_INITIATED_DMA
	ec = DSPAwaitUnsignedReply(DSP_DM_HOST_R_REQ,&dspack,
				   DSPDefaultTimeLimit);
	if (ec)
	  return(_DSPError(ec,"DSPReadArraySkipMode: "
			   "Timed out waiting for R_REQ from  DSP"));
	if (dspack != 0)
	  return(_DSPError1(DSP_EMISC,"DSPReadArraySkipMode: "
			    "Instead of channel 0, got an R_REQ from "
			    "DSP on channel %s",_DSPCVS(dspack)));
#else DO_USER_INITIATED_DMA
	/* Cannot do the following because we want s_msg_read_pending
	   to be false after the message is received */
	
	if (s_dsp_data_waiting) {
	    _DSPError(0,"DSPReadArraySkipMode: There is unread DSP data "
		      "in our local buffer... flushing... ");
	    s_dsp_data_waiting = 0; /* DSPFlushMessageBuffer(); */
	} else {
	    /* Read ack from DSP without leaving a dspcmd_req_msg pending */
	    int cnt=0;
	retry:
	    DSPReadMessages(10); /* Do a msg_receive for the waiting data */
	    if (!s_dsp_data_waiting) {
		if (_DSPVerbose)
		  _DSPError(0,"DSPReadArraySkipMode: No DSP_DM_HOST_R_REQ "
			    "after 10 ms!");
		cnt += 1;
		if (cnt > 100)
		  return _DSPError(DSP_ETIMEOUT,
				   "DSPReadArraySkipMode: Giving up "
				   "waiting for DSP_DM_HOST_R_REQ");
		goto retry;
	    }
	    if (s_msg_read_pending)
	      _DSPError(0,"DSPReadArraySkipMode: DSPReadMessages() issued a "
			"data refill request which here we assume it won't");
	    if (s_dsp_data_count != 1)
	      _DSPError1(0,"DSPReadArraySkipMode: "
			 "While looking for DM_HOST_R_REQ, "
			 "received %s words from the DSP instead of 1",
			 _DSPCVS(s_dsp_data_count));
	    dspack = *(s_dsp_data_0 + s_dsp_data_count - 1); /* take last */
	    s_dsp_data_waiting = 0; /* Flush internal DM buffer */
	}
	if (DSP_MESSAGE_OPCODE(dspack)!=DSP_DM_HOST_R_REQ) {
	    char *arg;
	    arg = "DSPReadArraySkipMode: got unexpected DSP message ";
	    arg = DSPCat(arg,DSPMessageExpand(dspack));
	    arg = DSPCat(arg," while waiting for DSP_DM_HOST_R_REQ");
	    _DSPError(DSP_EPROTOCOL,arg);
	}
    } /* done if !(use_dma && !UI_DMA_CHANDATA_FLUSHED) */

	
#endif DO_USER_INITIATED_DMA
    
    /* 
     * At this point, the DSP has been told to send the array
     * (unless DMA is to be used and the old chandata protocol is needed).
     * It remains to read the data from the DSP host interface.
     * This can be done by a DMA transfer or by polled reading
     * of the memory-mapped host interface.
     */

    if (!use_dma) {
	
#if SIMULATOR_POSSIBLE
	if (s_simulated)
	  fprintf(s_simulator_fp,";; Set HF1 and do DMA read, DSP to host\n");
#endif

#if MMAP
	if (!use_driver) {
	    /*
	     * MAJOR ASSUMPTION:
	     * The driver is not active wrt the DSP.
	     * Otherwise we might interrupt an operation that
	     * the driver is in the middle of.
	     */
	    register int i,j,dack;
	    register DSPFix24 *p=(&data[0]);

	    _DSPEnterMappedModeNoPing(); /* Prepare for mapped DSP use */

	    s_hostInterface->icr = 0x10; /* Set HF1 to enable xfer */

	    /* 
	     * Read the array from the DSP via the memory-mapped interface
	     */
	    switch (mode) {
	    case DSP_MODE8: {
		register unsigned char* c = (unsigned char *)data;
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--)
		      *c++ = s_readRXL();
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!RXDF) i += 1;
			*c++ = s_readRXL();
		    } 
		}
	    } break;
	    case DSP_MODE16: {
		register short* s = (short *)data;
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--)
		      *s++ = s_readRXML();
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!RXDF) i += 1;
			*s++ = s_readRXML();
		    } 
		}
	    } break;
	    case DSP_MODE24: {
		register unsigned char* c = (unsigned char *)data;
		register unsigned int w;
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--) {
			w = s_readRX();
			*c++ = (w>>16)&0xff;
			*c++ = (w>>8)&0xff;
			*c++ = w & 0xff;
		    }
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!RXDF) i += 1;
			w = s_readRX();
			*c++ = (w>>16);
			*c++ = (w>>8)&0xff;
			*c++ = w & 0xff;
		    } 
		}
	    } break;
	    case DSP_MODE32:
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--)
		      *p++ = s_readRX();
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!RXDF)
			  i += 1;
			*p++ = s_readRX();
		    }
		}
		break;
	    case DSP_MODE32_LEFT_JUSTIFIED:
		if (do_unchecked_mapped_array_transfers) {
		    for (j=wordCount;j;j--)
		      *p++ = (((unsigned int)s_readRX()) << 8);
		} else {
		    for (i=0,j=wordCount;j;j--) {
			while (!RXDF)
			  i += 1;
			*p++ = (((unsigned int)s_readRX()) << 8);
		    }
		}
		break;
	    default:
		return _DSPError1(EINVAL,"DSPReadArraySkipMode: "
				  "Unrecognized data mode = %s",_DSPCVS(mode));
	    
	    }
	    if (i>max_rxdf_buzz) {
		max_rxdf_buzz = i;
		if (_DSPVerbose)
		  _DSPError1(0,"DSPReadArraySkipMode: RXDF wait-count max "
			     "increased to %s",_DSPCVS(max_rxdf_buzz));
	    }

	    /* Terminate DMA read */
	    s_hostInterface->cvr = (0x80 | DSP_HC_HOST_R_DONE);

	    while (HC) i += 1;
	    usleep(1);
	    while (HF2) i += 1;
	    while (!RXDF) i += 1; /* Flush garbage word in RX */
	    dack = s_readRX();

	    /* Read R_DONE DSP message */
	    while (1) {

		while (!RXDF) i += 1;
		dack = s_readRX();
		
		if (DSP_MESSAGE_OPCODE(dack) == DSP_DM_HOST_R_DONE)
		  break;
		else {
		    char *arg;
		    arg = "DSPReadArraySkipMode: got unexpected DSP message ";
		    arg = DSPCat(arg,DSPMessageExpand(dack));
		    arg = DSPCat(arg," while waiting for DSP_HM_HOST_R_DONE");
		    _DSPError(DSP_EMISC,arg);
		}
	    }

	    /*
	     * If RXDF is on at this point, it is a DSP message, and it
	     * should go to the driver not us:
	     */

	    _DSPExitMappedMode(); /* Give DSP back to the driver */

	    return 0;

	} else {		/* else use_driver */
#else
	if (1) {
#endif
	    /*
	     * Read the data back via the driver, but NOT using DMA
	     */

	    int nread,nrem,kase;
	    int cnt=0, ntoss;

	    DSPSetHF1();	/* Enable "DMA transfer" in the DSP */

	    /* 
	     * DSP_DEF_BUFSIZE is defined in <nextdev/snd_msgs.h> 
	     * In release 1.0, it is 512 words.  Before that it was 32.
	     */
	    if (wordCount >= 2*DSP_DEF_BUFSIZE)
	      nread = (((int)(wordCount/DSP_DEF_BUFSIZE))-1)*DSP_DEF_BUFSIZE;
	    else
	      nread = 0;
	    nrem = wordCount - nread; /* last read size */

	    if (nread > 0)
	      kase = 3; /* Read data to array before "DMA" termination */
	    else {
		if (nrem > DSP_DEF_BUFSIZE)
		  kase = 2;	/* driver and DSPObject read data while DMA */
		else
		  kase = 1;	/* only driver reads data before DMA term. */
	    }

	    switch(kase) {
	    case 1:
		/* only driver reads DSP_DEF_BUFSIZE wds */
		break;
	    case 2:
		if (DSPAwaitData(1000)) { /* fill our buffer too */
		    if (nrem==0)
		      return _DSPError(DSP_EMISC,
				       "DSPReadArraySkipMode: "
				       "0 wordCount impossible");
		    return _DSPError(DSP_EMISC,
				     "DSPReadArraySkipMode: "
				     "DMA read never started after 1 second.");
		}
	    case 3:
		/* nrem: [DSP_DEF_BUFSIZE,2*DSP_DEF_BUFSIZE-1] */
		if(ec=DSPReadRXArray((DSPFix24 *)data,nread)) 
		  return _DSPError1(DSP_ESYSHUNG,
				    "DSPReadArraySkipMode: Array read "
				    "from DSP timed out after %s words",
				    _DSPCVS(ec));
		/* After this, there are 1 to DSP_DEF_BUFSIZE-1 words of 
		   garbage sitting in the driver's buffer, and there
		   are DSP_DEF_BUFSIZE words of unread data in our
		   own buffer pointed to by s_dsp_data_0 */
		break;
	    }
	
	    /* Exit DMA mode in DSP */
	    if (s_simulated)
	      fprintf(s_simulator_fp,
		      ";; Exit DSP-to-Host DMA with an HC_HOST_R_DONE\n");
	    DSP_UNTIL_ERROR(DSPHostCommand(DSP_HC_HOST_R_DONE));
	    DSPClearHF1();
	/*
	 * The DSP has been told to terminate the DMA send on its end.
	 * It uses a fast two-word interrupt handler for HTDE which does 
	 * not care how many words it sends. Therefore, a multiple of the
	 * driver buffer size will always be sent by the DSP.
	 * We have arranged that at most DSP_DEF_BUFSIZE-1 words will
	 * be read beyond the desired read.  These garbage words are
	 * now in the driver buffer.  Our internal buffer is either
	 * empty (for read sizes 1 to DSP_DEF_BUFSIZE words), or 
	 * completely full.  After the HOST_R_DONE,
	 * the DSP sends 2 words followed by all enqueued DSP messages, if any.
	 * The first of the final two words is arbitrary, and the second will
	 * contain the DM_HOST_R_DONE message.  The argument of that message
	 * will be the value of the DMA address register (in the DSP) at
	 * the time of the HOST_R_DONE.  We cannot know what this should
	 * be unless we read back the M register used in the DSP.  If it is
	 * -1, the index read back should be the multiple+1 of the driver's
	 * buffer size (512 in release 1.0, 32 before that) which equals
	 * or exceeds the desired quantity of data, plus 2 words.  Thus,
	 * for a transfer of 512 words from address 0, the DSP pointer should
	 * come back as 1026.  A transfer of length one word would be the same.
	 *
	 * After reading the remaining data, with the DSP out of DMA mode,
	 * there will be 0 to DSP_DEF_BUFSIZE-1 words of garbage to skip.
	 */
	    
	    /*
	     * Read remainder data.
	     */
	    if (nrem>0) {
		DSP_UNTIL_ERROR(DSPReadRXArray((DSPFix24 *)
					       (&(data[nread])), nrem));
		if (!s_dsp_data_waiting) {
		    /* read exactly exhausted buffered data, triggering
		       a refill with timeout 0, and subsequent msg_receive
		       timed out because the dspcmd_req_msg went out
		       just before, and the kernel did not have time to
		       send the data message to the dm_port before the
		       msg_receive.  Below we assume the next data buffer
		       has been read in, so we must explicitly do it now */
		    DSPAwaitData(0);
		}
	    } else {
		/* Same story as when nrem>0, except nread is (always) a 
		   multiple of DSP_DEF_BUFSIZE instead of nrem. */
		DSPAwaitData(0);
	    }
	    
	    if (!s_dsp_data_waiting)
	      return _DSPError(DSP_EPROTOCOL,
			       "DSPReadArraySkipMode: DSP did not send "
			       "DSP_DM_HOST_R_DONE or else it's lost");
	    
	    if (s_dsp_data_count == DSP_DEF_BUFSIZE)
	      s_dsp_data_waiting = 0; /* Flush current buffer */
	    /* else (s_dsp_data_count == 2) as we will find below.
	       (If we did not clear s_dsp_data_waiting, the call to
	       DSPReadMessages() below will be a nop.) */
	reread:
	    DSPReadMessages(10);	/* read a new buffer from driver */
	    if(!s_dsp_data_waiting) {
		if (_DSPVerbose)
		  _DSPError(0,"DSPReadArraySkipMode: "
			    "No DSP_DM_HOST_R_DONE after 10 ms!");
		cnt += 1;
		if (cnt > 100)
		  return _DSPError(DSP_ETIMEOUT,
				   "DSPReadArraySkipMode: Giving up "
				   "waiting for DSP_DM_HOST_R_DONE");
		goto reread;
	    }
	    if (s_dsp_data_count !=2) { /* what we expect */
		if (_DSPVerbose)
		  _DSPError1(DSP_EMISC,"DSPReadArraySkipMode: "
			     "s_dsp_data buffers screwed up. "
			     "Got back %s instead of 2 words for R_DONE. "
			     "Flushing current buffer assuming garbage.",
			     _DSPCVS(s_dsp_data_count));
		s_dsp_data_waiting = 0;	/* Flush mystery buffer */
		goto reread;
	    }

	    /* Read two extra words in host interface pipe (HTX & RX) */
	    if (s_simulated)
	      fprintf(s_simulator_fp,
		      ";; Read garbage word in HTX,RX + ack,,count\n");
	    if(ec=DSPReadRX(&dspack))
	      return(_DSPError(ec,"DSPReadArraySkipMode: "
			       "Could not read back garbage word "
			       "after sending HOST_R_DONE"));
	
	    ec = DSPAwaitUnsignedReply(DSP_DM_HOST_R_DONE,&dspack,
				       DSPDefaultTimeLimit);
	    if(ec)
	      return(_DSPError(ec,"DSPReadArraySkipMode: "
			       "Could not read back next-read-address "
			       "after sending HOST_R_DONE"));
	
	    if (_DSPTrace & DSP_TRACE_HOST_INTERFACE)
	      printf("After HOST_R_DONE, last-addr-read minus expected = %d\n",
		     dspack - (startAddress + wordCount + 2));

	    return(ec);
	    
	} /* use_driver, no dma */

    } else { /* else use_dma ( => use_driver) */

	/*
	 * Read the data back via the driver, USING DMA
	 */

	pointer_t new_data = 0;

#if UI_DMA_CHANDATA_FLUSHED
	/*
	 * "chandata" will not be supported in the driver eventually.
	 * The DSP-side protocol will be assumed unknown by the kernel.
	 */ 
	ec = DSPCallV(DSP_HM_HOST_W,

		      3,(int)memorySpace,startAddress,skipFactor);
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "DSPCallV(DSP_HM_HOST_W) failed.");
	DSPSetHF1();	/* Enable "DMA transfer" in the DSP */
#if DO_USER_INITIATED_DMA
	/* Can't even compile because of prototype difference */
	ec = snddriver_dsp_dma_read(s_dsp_hm_port,&new_data,wordCount,mode);
#else
	ec = -1;
#endif
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "snddriver_dsp_dma_write() failed.");
#else	
	_DSPError(0,"DSPWriteArraySkipMode: DSP will be told a second time "
		  "to send the data array! ***FIXME***");
	ec = _snddriver_libdsp_dma_read(s_dsp_hm_port,startAddress,wordCount,
				     skipFactor,(int)memorySpace,mode,
				     &new_data);
	if (ec)
	  return _DSPError(ec,"DSPWriteArraySkipMode: "
			   "_snddriver_libdsp_dma_read() failed.");
#endif

	/*
	 * Copy data to the user's array
	 */
	s_mode_to_width(mode);
	bcopy((char *)new_data, (char *)data, wordCount*s_mode_to_width(mode));
	vm_deallocate(task_self(), new_data, wordCount*s_mode_to_width(mode));

#if 0
	FIXME: NEED VERSION WHICH ALLOCATES ARRAY, e.g.
	  int DSPReadNewArraySkipMode(
	    DSPFix24 **data,	/* array from DSP ALLOCATED BY KERNEL */
	    DSPMemorySpace memorySpace, /* from <dsp/dspstructs.h> */
	    int startAddress,		/* within DSP memory */
	    int skipFactor,		/* 1 means normal contiguous xfer */
	    int wordCount,		/* from DSP perspective */
            int mode)			/* from <nextdev/dspvar.h> */
#endif	  

	if (mode==DSP_MODE32) {	/* must explicitly right-justify after DMA */
	    register int i;
	    register int *s=data;
	    for (i=wordCount;i;i--)
	      *data++ >>= 8;
	}

	return ec;
    }
}


int _DSPOpenStatePrint()
{
    printf("\nDSP Open State:\n");
    printf("s_sound_dev_port = %d\n",s_sound_dev_port);
    printf("s_dsp_hm_port = %d\n",s_dsp_hm_port);
    printf("s_dsp_dm_port = %d\n",s_dsp_dm_port);
    printf("s_dsp_err_port = %d\n",s_dsp_err_port);
    printf("s_driver_reply_port = %d\n",s_driver_reply_port);
    printf("s_wd_stream_port = %d\n",s_wd_stream_port);
    printf("s_so_buf_bytes = %d\n",s_so_buf_bytes);
    printf("s_dsp_owner_port = %d\n",s_dsp_owner_port);
    
    printf("s_simulated = %d\n",s_simulated);
    printf("s_simulatorFile = %s\n",s_simulatorFile);
    printf("s_simulator_fp = %d\n",s_simulator_fp);
    printf("s_saving_commands = %d\n",s_saving_commands);
    printf("s_commandsFile = %s\n",s_commandsFile);
    printf("s_commands_fp = %d\n",s_commands_fp);
    
    printf("s_dsp_count = %d\n",s_dsp_count);
    printf("s_open = %d\n",s_open);
    printf("s_mapped_only = %d\n",s_mapped_only);
    printf("s_logged = %d\n",s_logged);
    printf("s_low_srate = %d\n",s_low_srate);
    printf("s_sound_out = %d\n",s_sound_out);
    printf("s_write_data = %d\n",s_write_data);
    printf("s_read_data = %d\n",s_read_data);
    printf("s_ssi_sound_out = %d\n",s_ssi_sound_out);
    printf("s_srate = %f\n",s_srate);
    printf("s_ap_mode = %d\n",s_ap_mode);
    
    printf("s_system_link_file = %s\n",s_system_link_file);
    printf("s_system_binary_file = %s\n",s_system_binary_file);
    printf("s_system_map_file = %s\n",s_system_map_file);
    printf("s_devstr = %s\n",s_devstr);
    printf("s_idsp = %d\n\n",s_idsp);
    
    return 0;
}

/*************************** DSP SYNCHRONIZATION ***************************/


int _DSPAwaitMsgSendAck(msg_header_t *msg)
{    
    int ec;
    
    CHECK_ERROR_1;
    
    if (!msg->msg_local_port)
      return (_DSPError(0,"_DSPAwaitMsgSendAck: "
			"no msg_local_port in message for ack"));
    
    _DSP_dsprcv_msg_reset(s_dsprcv_msg,s_dsp_hm_port,msg->msg_local_port);
    ec = msg_receive(s_dsprcv_msg, RCV_TIMEOUT, 10000); /* wait 10 seconds */
    
    if (ec == RCV_TIMED_OUT)
      return (_DSPMachError(ec,"_DSPAwaitMsgSendAck: "
			    "msg_receive timed out after 10 seconds."));
    
    if (ec != KERN_SUCCESS)
      return (_DSPMachError(ec,"_DSPAwaitMsgSendAck: msg_receive failed."));
    
    if (   s_dsprcv_msg->msg_id != SND_MSG_ILLEGAL_MSG
	|| ((snd_illegal_msg_t *)s_dsprcv_msg)->ill_msgid != SND_MSG_DSP_MSG
	|| ((snd_illegal_msg_t *)s_dsprcv_msg)->ill_error != SND_NO_ERROR )
      return (_DSPError1(DSP_EMACH,"_DSPAwaitMsgSendAck: "
			 "msg_id %s in reply not recognized",
			 _DSPCVS(s_dsprcv_msg->msg_id)));
    
    if (((snd_illegal_msg_t *)s_dsprcv_msg)->ill_msgid != msg->msg_id)
      return (_DSPError1(DSP_EMACH,"_DSPAwaitMsgSendAck: "
			 "Got reply to msg_id %s",
			 DSPCat(_DSPCVS(((snd_illegal_msg_t *) s_dsprcv_msg)->ill_msgid),
				 DSPCat(" instead of msg_id ",
					 _DSPCVS(s_dsprcv_msg->msg_id)))));
    
    return 0;
}


int _DSPAwaitRegs(
    int mask,		/* mask to block on as bits in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 as desired for each 1 mask bit */
    int msTimeLimit)	/* time limit in milliseconds */
{    
    int ec,i,isr,desired,retried;
    
    CHECK_ERROR_1;
    
    if (DSP_IS_SIMULATED_ONLY)
      return 0;
    
    if (s_mapped_only) {
	for (i=0;i<msTimeLimit/10;i++) {
	    _DSPReadRegs();
	    if ( s_regs & mask == value )
	      goto dsp_ready;
	    select(0,0,0,0,&_DSPTenMillisecondTimer);
	}
	return(_DSPError1(DSP_ETIMEOUT,
		  "_DSPAwaitRegs: Timed out waiting for reg bits 0x%s ",
		  _DSPCVHS(mask)));
    dsp_ready:
	return 0;
    }
    
    /* reset command message to DSP */
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->header.msg_size = sizeof(snd_dspcmd_msg_t);  /* needed? */
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->header.msg_remote_port = s_dsp_hm_port;
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->header.msg_local_port = PORT_NULL; 
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->header.msg_id = SND_MSG_DSP_MSG;
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->pri = s_cur_pri;
    ((snd_dspcmd_msg_t *)s_dspcmd_msg)->atomic = 0;
   
    /* 
     * Add block spec for desired mask.
     * Note that snddriver_dspcmd_req_condition is very expensive to use here.
     */
    s_dspcmd_msg = _DSP_dsp_condition(s_dspcmd_msg, mask, (value ? mask : 0));
    
    /* add reply message to be sent when mask comes true */
    s_dspcmd_msg = _DSP_dsp_ret_msg(s_dspcmd_msg, s_driver_reply_msg);
    
    /* 
     * Send the condition-wait message.
     * Note that snddriver_dspcmd_req_condition is way too expensive to use here.
     */
    ec = s_msgSend();
    if (ec != KERN_SUCCESS)
      return (_DSPMachError(ec,"_DSPAwaitRegs: s_msgSend failed."));
    
    /*
     * Get the reply we've enqueued.
     */
    retried = 0;
retry:
    s_dsprcv_msg->msg_size = MSG_SIZE_MAX;
    s_dsprcv_msg->msg_local_port = s_driver_reply_port;
    
    ec = msg_receive(s_dsprcv_msg, RCV_TIMEOUT, 
		     (msTimeLimit? msTimeLimit : _DSP_MACH_DEADLOCK_TIMEOUT) );
    
    if (ec == RCV_TIMED_OUT) {
      if (!msTimeLimit)
        goto retry;
      if (retried == 0) {
	retried = 1;
	msTimeLimit = 0;
        goto retry;
      }
      return _DSPError(ec,"_DSPAwaitRegs: timed out waiting for condition");
    }
    
    if (ec != KERN_SUCCESS)
      return (_DSPMachError(ec,"_DSPAwaitRegs: msg_receive failed."));
    
    if (s_dsprcv_msg->msg_id != s_driver_reply_msg->msg_id)
      return (_DSPError1(DSP_EMACH,"_DSPAwaitRegs: "
			 "Unrecognized msg id %s",
			 _DSPCVS(s_dsprcv_msg->msg_id)));
    return 0;
}


int _DSPAwaitBit(
    int bit,		/* bit to block on as bit in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 */
    int msTimeLimit)	/* time limit in milliseconds */
{    
    return _DSPAwaitRegs(bit,(value? bit : 0),msTimeLimit);
}


int DSPAwaitHC(int msTimeLimit)
{
    return _DSPAwaitRegs(DSP_CVR_HC_REGS_MASK,0,msTimeLimit);
}


int DSPAwaitTRDY(int msTimeLimit)
{
    /* The TRDY bit comes on when all HRX data has been processed by the DSP */
    /* It is defined as TXDE && !HRDF */
    return _DSPAwaitRegs((DSP_ISR_TRDY_REGS_MASK),(DSP_ISR_TRDY_REGS_MASK),
			 msTimeLimit);
}


int DSPAwaitHostMessage(int msTimeLimit)
{
    DSP_UNTIL_ERROR(DSPAwaitHC(msTimeLimit));	/* First let HC clear */
    
#if (DSP_BUSY != DSP_ISR_HF2 && DSP_BUSY != DSP_ISR_HF3)
    _DSPFatalError(-1,"DSPAwaitHostMessage: DSP_BUSY != DSP_ISR_HF2 or HF3");
#endif
    
    return _DSPAwaitRegs(DSP_BUSY_REGS_MASK,0,
			 msTimeLimit); /* Assumed in ISR */
}

int DSPAwaitHF3Clear(int msTimeLimit)
{
    return _DSPAwaitRegs(DSP_ISR_HF3_REGS_MASK,0,msTimeLimit);
}


int DSPSetHF0(void)
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; Set HF0\n");
    
    return _DSPSetBit(DSP_ICR_HF0_REGS_MASK);
}


int DSPClearHF0(void)
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; Clear HF0\n");
    
    return _DSPClearBit(DSP_ICR_HF0_REGS_MASK);
}


int DSPSetHF1(void)
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; Set HF1\n");
    
    return _DSPSetBit(DSP_ICR_HF1_REGS_MASK);
}

int DSPClearHF1(void)
{
    if (s_simulated)
      fprintf(s_simulator_fp,";; Clear HF1\n");
    
    return _DSPClearBit(DSP_ICR_HF1_REGS_MASK);
}


int DSPGetHF2(void)
{
    int isr;
    if (s_simulated)
      fprintf(s_simulator_fp,";; Read HF2\n");
    DSP_UNTIL_ERROR(DSPReadISR(&isr));
    return isr & DSP_ISR_HF2;
}

int DSPGetHF3(void)
{
    int isr;
    if (s_simulated)
      fprintf(s_simulator_fp,";; Read HF3\n");
    DSP_UNTIL_ERROR(DSPReadISR(&isr));
    return isr & DSP_ISR_HF3;
}

int DSPGetHF2AndHF3(void)
{
    int isr;
    if (s_simulated)
      fprintf(s_simulator_fp,";; Read HF3\n");
    DSP_UNTIL_ERROR(DSPReadISR(&isr));
    return isr & (DSP_ISR_HF3 || DSP_ISR_HF2);
}

int DSPMKFreezeOrchestra(void) 	/* Freeze orchestra at "end of current tick" */
{
    int ec=0;
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Freeze orchestra loop\n");
    if (s_sound_out)
      snddriver_stream_control(s_wd_stream_port,0,SNDDRIVER_PAUSE_STREAM);
    ec = DSPSetHF0();		/* Pause DSP orchestra loop */
    s_frozen = 1;
    return(ec);
}

int DSPMKThawOrchestra(void) 	/* Freeze orchestra at "end of current tick" */
{
    int ec=0;
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Unfreeze (thaw) orchestra loop\n");
    ec = DSPClearHF0();	/* Unpause DSP orchestra loop */
    if (s_sound_out)
      snddriver_stream_control(s_wd_stream_port,0,SNDDRIVER_RESUME_STREAM);
    s_frozen = 0;
    return(ec);
}

/*************************** DSP BREAKPOINT ***************************/

/* The following will probably move to _DSPDebug.c when it exists */

int DSPBreakPoint(int bpmsg)
/*
 * Process DSP breakpoint 
 */
{
/* maximum number of error messages read back after hitting DSP breakpoint */
#define DSP_MAX_BREAK_MESSAGES 32

    int i;
    if(DSP_MESSAGE_OPCODE(bpmsg)!=DSP_DE_BREAK)
      return(_DSPError1(EINVAL,
			"DSPBreakPoint: Passed invalid DSP breakpoint "
			"message = 0x%s",_DSPCVHS(bpmsg)));
    fprintf(stderr,";;*** DSP BREAKPOINT at address 0x%X ***\n",
	    DSP_MESSAGE_ADDRESS(bpmsg));
    for(i=0;i<DSP_MAX_BREAK_MESSAGES;i++) {
	if (DSPDataIsAvailable()) {
	    DSPReadRX(&bpmsg); /* Read back DSP 'stderr' messages */
	    fprintf(stderr,";;*** %s\n",DSPMessageExpand(bpmsg));
	}
	else
	  goto gotem;
    }	
    fprintf(stderr,"\n;;*** There may be unread DSP messages ***\n\n");
 gotem:
    fprintf(stderr,"\n;; Use dspabort to prepare DSP for Bug56 'grab'"
	    " or kill this process and reset the DSP in Bug56.\n\n");
    if (s_simulated)
      DSPCloseSimulatorFile();
    if (s_saving_commands)
      DSPCloseCommandsFile(NULL);
    pause();
    return 0;
}	

/********************* mach dsp/sound driver primitives ********************/

static int s_dsp_err_reader(arg)
    int *arg;			/* unused argument */
    /*
     * Function which blocks reading error messages from DSP in its own thread.
     * Called once when the DSP is initialized.
     * Returns 0 unless there was a problem checking for errors.
     */
{
    register int r, rsize, i;
    int err_read_pending = 0;	   /* set when err read request is out */
    static msg_header_t *rcv_msg = 0; /* message frame for msg_receive */
    
    if (rcv_msg)
      free(rcv_msg);
    rcv_msg = _DSP_dsprcv_msg(s_dsp_hm_port,s_dsp_dm_port);
    
    if (s_mapped_only)
      return(_DSPError(0,"DSPObject: s_dsp_err_reader: "
		       "no separate error thread in mapped-only mode"));
    
    while (1) {
	if (!s_open)		/* cleared before join and port dealloc */
	  break;
	
	if (s_mapped_only)	/* no can do */
	  break;
	
	if (!err_read_pending) { /* request error messages if necessary */
	    r = snddriver_dspcmd_req_err(s_dsp_hm_port, s_dsp_err_port);
	    err_read_pending = 1;
	    if (r != KERN_SUCCESS)
	      return(_DSPMachError(r,"DSPObject: s_dsp_err_reader: "
				   "snddriver_dspcmd_req_err failed."));
	}
	
	_DSP_dsprcv_msg_reset(rcv_msg,s_dsp_hm_port,s_dsp_err_port);
	r = msg_receive(rcv_msg, RCV_TIMEOUT, _DSP_ERR_TIMEOUT);
	if (r == KERN_SUCCESS) {/* read error messages and print them to log */
	    DSPFix24 errwd;
	    err_read_pending = 0;
	    rsize = ((snd_dsp_msg_t *)rcv_msg)->dataType.msg_type_long_number;
	    for (i=0; i<rsize; i++)  {
		errwd = ((snd_dsp_msg_t *)rcv_msg)->data[i]; 
		if(DSP_MESSAGE_OPCODE(errwd)==DSP_DE_BREAK)
		  DSPBreakPoint(errwd);
		else
		  _DSPError(DSP_EDSP,DSPMessageExpand(errwd));
	    }
	}
	else if (r == RCV_TIMED_OUT) 
	  ; /* normal */
	else if (r == RCV_INVALID_PORT) {
	    if (_DSPVerbose && s_open)
	      _DSPMachError(r,"DSPObject: "
			    "s_dsp_err_reader() error port gone while DSP open");
	    break;
	}
	else
	  return _DSPMachError(r,"DSPObject: s_dsp_err_reader: "
			       "msg_receive failed.");
    }
    return 0;
}


int DSPReadMessages(
    int msTimeLimit)
{
    register int r, rsize, i, j, ec;
    
    CHECK_ERROR_1;
    
    if (s_dsp_data_waiting)
      return 0;		/* Cannot read until existing data consumed */
    
#if MMAP
    if (s_mapped_only) {
	i = 0;
	s_dsp_data_ptr = s_dsp_data_0;
	while (i<128 && RXDF) {		/* 128 is defined in snd_msgs.h */
	    i++;
	    *s_dsp_data_ptr++ = s_readRX();
	    /* select(0,0,0,0,&_DSPTenMillisecondTimer); */
	}
	s_dsp_data_count = s_dsp_data_ptr - s_dsp_data_0;
	s_dsp_data_waiting = (s_dsp_data_count>0);
	goto DSPReadMessages_exit;
    }
#endif
    
    if (!s_msg_read_pending) {
	r = snddriver_dspcmd_req_msg(s_dsp_hm_port, s_dsp_dm_port);
	s_msg_read_pending = 1;
	if (r != KERN_SUCCESS)
	  return(_DSPMachError(r,"DSPReadMessages: "
			       "snddriver_dspcmd_req_msg failed."));
    }
    
 retry:
    _DSP_dsprcv_msg_reset(s_dsprcv_msg,s_dsp_hm_port,s_dsp_dm_port);
    r = msg_receive(s_dsprcv_msg, RCV_TIMEOUT, msTimeLimit);
    if (r == KERN_SUCCESS) {
	if (s_dsprcv_msg->msg_id != SND_MSG_RET_DSP_MSG) {
	    _DSPError1(DSP_EMISC,"got msg %s instead of SND_MSG_RET_DSP_MSG", 
		       _DSPCVS(s_dsprcv_msg->msg_id));
	    if (s_dsprcv_msg->msg_id == SND_MSG_ILLEGAL_MSG) {
		_DSPError1(DSP_EMISC,"s_msgSend ack SND_MSG_ILLEGAL_MSG "
			  "to msg %s\n",_DSPCVS(
			  ((snd_illegal_msg_t *)s_dsprcv_msg)->ill_msgid));
		_DSPError(DSP_EMISC,"We'll retry the msg_receive . . . \n");
		goto retry;
	    }	    
	    else
	      return(DSP_EMACH);
	}
	s_dsp_data_ptr = s_dsp_data_0;
	s_dsp_data_count = 
	  ((snd_dsp_msg_t *)s_dsprcv_msg)->dataType.msg_type_long_number;
	s_dsp_data_waiting = (s_dsp_data_count>0);
	s_msg_read_pending = 0;
    } else if (r != RCV_TIMED_OUT) 
      return(_DSPMachError(r,"DSPReadMessages: msg_receive failed."));
    else
      s_dsp_data_waiting = 0;
    
 DSPReadMessages_exit:
    return(!s_dsp_data_waiting);
}


int DSPFlushMessages(void)
{
    do {
	s_dsp_data_waiting = 0;
	DSPReadMessages(0);	/* 89jul13/jos - from default time-out */
    } while (s_dsp_data_waiting);
    /* s_dsp_data_count is zero here */
    return 0;
}

int DSPFlushMessageBuffer(void)
{
    s_dsp_data_waiting = 0;
    return 0;
}


int _DSPReadDatum(DSPFix24 *datumP)
{
    if (!s_dsp_data_waiting)
      DSPReadMessages(0);    /* 89jul13/jos was _DSP_MACH_RCV_TIMEOUT*10    */
    
    if (s_dsp_data_waiting) {
	*datumP = *s_dsp_data_ptr++;
	if (s_dsp_data_ptr >=  s_dsp_data_0 + s_dsp_data_count) {
	    s_dsp_data_waiting = 0;
	    DSPReadMessages(0); /* initiate next read, but don't wait for it */
	}
    }
    else 
      return(-1);
    
    /* The purpose of explicit reads is to synch with HM_HOST_R_DONE */
    if (s_simulated)
      s_simReadRX(*datumP);
    
    if (_DSPTrace & DSP_TRACE_DSP) 
      printf("\tRX [%d]	 =  0x%X\n",s_idsp,*datumP);
    
    return 0;
}


int _DSPReadData(DSPFix24 *dataP, int *nP)
{
    int i,n;
    DSPFix24 d;
    
    n = *nP;
    for (i=0; i<n; i++)
    {
	if (_DSPReadDatum(&d))
	  break;
	*dataP++ = d;
    }
    *nP = i;
    return (*nP != n);
}


int DSPAwaitData(int msTimeLimit)
{
    int i;
    if (s_mapped_only) {
	while (DSPDataIsAvailable() == 0) {
	    if (i++>msTimeLimit/10 && msTimeLimit>0)
	      return(_DSPError(DSP_ETIMEOUT,
			       "DSPAwaitData: Timed out waiting "
			       "for RXDF in DSP"));
	    select(0,0,0,0,&_DSPTenMillisecondTimer);
	}		
	return 0;
    }
    
    if (DSP_IS_SIMULATED_ONLY)
      return(1);			/* simulate time-out */
    
    if (s_dsp_data_waiting)
      return 0;
    
    return DSPReadMessages(msTimeLimit? msTimeLimit : _DSP_MACH_FOREVER);
}


int _DSPWriteData(DSPFix24 *dataP, int ndata)
{
    int i,j,ec,word;
    
    CHECK_ERROR_1;		/* make sure DSP is open or simulated */
    
    /* If memory-mapped only, write to DSP in mapped mode */
#if MMAP
    if (s_mapped_only) {
	for (j=0;j<ndata;j++) {
	    word = dataP[j];
	    while (!TXDE) {
		int i = 0;
		if (i++>100)	/* 1 second */
		  return _DSPError1(DSP_ETIMEOUT,
				    "_DSPWriteData(data[%s): Timed out "
				    "waiting for TXDE in DSP",
				    DSPCat(_DSPCVS(j),
					    DSPCat("]=(0x",
						    _DSPCVHS(word))));
		select(0,0,0,0,&_DSPTenMillisecondTimer);
	    }
	    s_writeTX(&word);
	}
    } else {
#else
    if (1) {
#endif
	/*
	  The data is sent down atomically to shut out DMA completes
	  while it is trickling in.  A DMA complete INITs the host interface
	  which would clear any data in the DSP host interface pipe.
	  */
	_DSP_dspcmd_msg_reset(s_dspcmd_msg,
			      s_dsp_hm_port, PORT_NULL, s_cur_pri, 1);
	s_dspcmd_msg = _DSP_dsp_data(s_dspcmd_msg, 
				     (pointer_t)dataP, sizeof(int), ndata);
	ec = s_msgSend();
	if (ec != KERN_SUCCESS)
	  return (_DSPMachError(ec,"_DSPWriteData: s_msgSend failed."));
    }
    
    /* If memory-mapped only, write to DSP in simulator file */
    if (s_simulated) {
	fprintf(s_simulator_fp,
		";; _DSPWriteData: writing %d = 0x%X words to TX\n",
		ndata,ndata);
	for (i=0;i<ndata;i++) 
	  s_simWriteTX(dataP[i]); /* write to simulator file */
	fprintf(s_simulator_fp,"\n");
    }
    
    return 0;
}

int _DSPWriteRegs(int mask, int value)
{
    int ec,oicr,ocvr;
    
    CHECK_ERROR_1;
    
    if (s_simulated) {
	_DSPReadRegs();
	oicr = s_icr;
	ocvr = s_cvr;
	s_regs |= (mask & value);
	_DSPUpdateRegBytes();
	if (oicr != s_icr)
	  DSP_UNTIL_ERROR(s_simPrintF(s_simulator_fp, DSP_ICR, s_icr));
	if (ocvr != s_cvr)
	  DSP_UNTIL_ERROR(s_simPrintF(s_simulator_fp, DSP_CVR, s_cvr));
	fprintf(s_simulator_fp,"\n");
    }
    
    if (s_mapped_only && MMAP)
      *(int *)s_hostInterface |= (mask & value);
    else {
	_DSP_dspcmd_msg_reset(s_dspcmd_msg,
			      s_dsp_hm_port, 
			      thread_reply(), /* request an ack message */
			      s_cur_pri, 0);
	
	s_dspcmd_msg = _DSP_dsp_host_flag(s_dspcmd_msg, mask, value);
	
	ec = s_msgSend();
	
	if (ec != KERN_SUCCESS)
	  return (_DSPMachError(ec,"_DSPWriteRegs: s_msgSend failed."));
	
	ec = _DSPAwaitMsgSendAck(s_dspcmd_msg);
	
	if (ec)
	  return (_DSPError(ec,"_DSPWriteRegs: _DSPAwaitMsgSendAck failed."));
    }
    
    return 0;
}


int _DSPPutBit( int bit, int value)
{
    return _DSPWriteRegs(bit,(value? bit : 0));
}


int _DSPSetBit(int bit)
{
    return _DSPWriteRegs(bit,bit);
}


int _DSPClearBit(int bit)
{
    return _DSPWriteRegs(bit,0);
}


int _DSPWriteDatum(DSPFix24 datum)
{
    DSPFix24 w;
    w = datum & DSP_WORD_MASK;
    if (w != datum)
      _DSPError(EDOM,"_DSPWriteDatum: most-significant byte not 0");
    
    if (_DSPTrace & DSP_TRACE_HOST_INTERFACE)
      printf("\tTX*[%d]	 <-- 0x%X\n", s_idsp,datum);
    
    return(_DSPWriteData(&datum,1));
}


static msg_header_t *s_addMsgData(
	  msg_header_t	*msg,		// message frame to add request to
	  int		*data,		// data to add
	  int		eltsize,	// 1, 2, or 4 byte data
	  int		nelts)		// number of elements of data to send
{
    int i;
    if (s_simulated) {
	fprintf(s_simulator_fp,
		";; s_addMsgData: writing %d = 0x%X words to TX\n",
		nelts,nelts);
	for (i=0;i<nelts;i++) 
	  s_simWriteTX(data[i]); /* write to simulator file */
    }
    
    return _DSP_dsp_data(msg,(pointer_t)data,eltsize,nelts);
}


int _DSPWriteHostMessage(
    int *hm_array,
    int nwords)
{
    static int hm_mask  = (DSP_CVR_HC_REGS_MASK) | 
      ((DSP_ISR_TRDY_REGS_MASK | DSP_ISR_HF3_REGS_MASK | DSP_ISR_HF2_REGS_MASK));
    static int hm_flags = (DSP_ISR_TRDY_REGS_MASK);
    int tshi,tslo;
    register int hm_type,ec;
    
    if (!s_open)
      return(_DSPError(-1,"_DSPWriteHostMessage: Can't talk to closed DSP"));
    
    if (nwords > DSP_MAX_HM)	/* dsp.h */
      return(_DSPError(DSP_EHMSOVFL,
		       DSPCat(DSPCat("_DSPWriteHostMessage: "
				       "Host message total length = ",
				       _DSPCVS(nwords)),
			       DSPCat(" while maximum is ",
				       _DSPCVS(DSP_MAX_HM)))));
    /* DSP host message type code */
    hm_type = hm_array[nwords-1] & 0xFF0000;
    if (hm_type != _DSP_HMTYPE_UNTIMED) {
	tshi = hm_array[nwords-3];
	tslo = hm_array[nwords-2];
    }
    
#if SIMULATOR_POSSIBLE
    if (s_simulated) {
	DSPTimeStamp ts,*tsp;
	int i;
	if (hm_type == _DSP_HMTYPE_UNTIMED) {
	    tsp = 0;
	} else {
	    tsp = &ts;
	    ts.high24 = tshi;
	    ts.low24 = tslo;
	}
	fprintf(s_simulator_fp,";; _DSPWriteHostMessage: "
		"Await hc~,hf3~,hf2~,trdy == 0x%X in 0x%X,\n"
		";;     and send length %d = 0x%X host msg %s\n",
		hm_flags,hm_mask,nwords,nwords,DSPTimeStampStr(tsp));
	/* write to simulator file */
	for (i=0;i<nwords;i++) 
	  s_simWriteTX(hm_array[i]);
	s_simPrintF(s_simulator_fp, DSP_CVR, DSP_CVR_HC_MASK|DSP_HC_XHM);
	fprintf(s_simulator_fp,"\n");
    }
#endif

/*  s_cur_pri = (hm_type==_DSP_HMTYPE_UNTIMED)? DSP_MSG_MED : DSP_MSG_LOW; */
    
    if (hm_type==_DSP_HMTYPE_UNTIMED)
      s_cur_pri = DSP_MSG_MED;	/* Untimed messages go around TMQ */
    else /* timed absolute or relative */
	if (hm_type==_DSP_HMTYPE_TIMEDA && (tshi == 0 && tslo == 0))
	  s_cur_pri = DSP_MSG_MED; /* Timed-0 messages also jump around TMQ */
	else
	  s_cur_pri = DSP_MSG_LOW;	/* True timed messages enqueue */
    
    
    /* 
      Reset dsp command message fields. Host Message must be ATOMIC.
      This is because a DMA complete will send an hc_host_r_done
      host message to the DSP.	It this happens while we are in the
      middle of a host message, all arguments written to the DSP
      so far will be lost.  Host messages could be made interruptible,
      but at the price of not being able to check up on the
      host message handler to see that it consumed the precise number
      of arguments it should have.  If we assumed the interrupting HM
      is always correct (only the OS can do it, and only hm_host_r_done
      can occur this way at present) then we could actually arrange to
      keep the error checking with some rewrite in jsrlib.asm et al.
      */
    _DSP_dspcmd_msg_reset(s_dspcmd_msg,
			  s_dsp_hm_port, PORT_NULL, s_cur_pri, 1);
    
    /* 
      Wait on HF3 now since any long-term block must occur at the beginning
      of the message.	*** AN ATOMIC MESSAGE MUST NOT BLOCK AFTER IT
      HAS SENT ANYTHING TO THE DSP.  IT CAN ONLY BLOCK AT THE BEGINNING. ***
      This is because sound-out DMA packets cannot be terminated while
      blocked. At the end of a sound-out DMA, the hc_host_r_done host-command
      must be issued, and HF1 should be cleared.  If an atomic message
      is in progress, neither will occur, so the DSP will continue sending
      garbage in DMA mode while the driver has left DMA mode.
      
      Here we also wait for HC and HF2 because during a host message
      execution, HF3 is cleared so that both HF2 and HF3 can imply "abort".
      
      We block until TRDY since we need it too.
      */
    
    s_dspcmd_msg = _DSP_dsp_condition(s_dspcmd_msg,hm_mask,hm_flags);
    s_dspcmd_msg = _DSP_dsp_data(s_dspcmd_msg, (pointer_t)hm_array, 
				 sizeof(int), nwords);
    
    /* Issue "Execute host message" host command to DSP */
    s_dspcmd_msg = _DSP_dsp_condition(s_dspcmd_msg,hm_mask,hm_flags);
    s_dspcmd_msg = _DSP_dsp_host_command(s_dspcmd_msg, DSP_HC_XHM);
    
    /* Send atomic mach message containing full DSP host message */
    ec = s_msgSend();
    
    s_cur_pri = DSP_MSG_MED;	/* revert to normal priority */
    
    if (ec)
      return _DSPMachError(ec,"_DSPWriteHostMessage: s_msgSend failed.");
    else
      return 0;
}


/*************************** DSP Host Commands ***************************/


int DSPHostCommand(int cmd)			
{
    static int hm_mask  = (DSP_CVR_HC_REGS_MASK) | 
      ((DSP_ISR_TRDY_REGS_MASK 
	| DSP_ISR_HF3_REGS_MASK 
	| DSP_ISR_HF2_REGS_MASK));

    static int hm_flags = (DSP_ISR_TRDY_REGS_MASK);

    int cvrVal,ec;			/* DSP command vector register */
    
    if (cmd!=(cvrVal=cmd&DSP_CVR_HV_MASK))
      _DSPError(DSP_EMISC,"DSPHostCommand: MSBs of CVR host vector not 0\n");
    
    if (s_simulated)
      fprintf(s_simulator_fp,
	      ";; DSPHostCommand(0x%X):\n",cmd);
    
    if (s_mapped_only) {
	cvrVal |= DSP_CVR_HC_MASK; /* Or "HC" bit with command code */
	
	ec = _DSPWriteRegs( 0xFF << 16 , (cvrVal & 0xFF) << 16 );
	if (ec && _DSPVerbose)
	  _DSPError1(ec,"DSPHostCommand(0x%s): Could not write CVR",
		     _DSPCVHS(cmd));
    } else {
	
	if (s_simulated) {
	    s_simPrintF(s_simulator_fp, DSP_CVR, DSP_CVR_HC_MASK | cvrVal);
	    fprintf(s_simulator_fp,"\n");
	}
	
	_DSP_dspcmd_msg_reset(s_dspcmd_msg,
			      s_dsp_hm_port, PORT_NULL, s_cur_pri, 1);
	
	/* New 89jul20/jos */
	s_dspcmd_msg = _DSP_dsp_condition(s_dspcmd_msg,hm_mask,hm_flags);
	s_dspcmd_msg = _DSP_dsp_host_command(s_dspcmd_msg, cmd);
	
	/* Send mach message */
	if (ec = s_msgSend())
	  return (_DSPMachError(ec,"_DSPHostCommand: s_msgSend failed."));
    }
    return(ec);
}


/****************************** DSPMessage.c ********************************/

/*	DSPMessage.c - Utilities for messages between host and DSP
	Copyright 1987,1988, by NeXT, Inc.

There are two types of message:

	DSP  Message - message from DSP to host (single 24-bit word)
	Host Message - message from host to DSP (several TX args + host cmd)

Modification history:
	07/28/88/jos - Created from _DSPUtilities.c
	08/19/88/jos - Added host message ioctl to DSPCall{V}.
	08/20/88/jos - rewrote DSPHostMessage* et al for atomic host msgs.
	08/21/88/jos - added msTimeLimit (in milliseconds) to every "Await" fn
	08/21/88/jos - changed DSPDataAvailable to DSPDataIsAvailable.
	08/21/88/jos - converted to procedure prototypes.
	08/13/88/gk  - changed _DSPSendHmStruct() to _DSPWriteHm() using write.
		       KEEP DSPMessage.h UP TO DATE !!!!
	11/20/89/daj - changed _DSPCallTimedV() to write arguments backwards 
		       onto TMQ. (To match DSPMKCallTimed())
		       Also replaced DSP_MALLOC with alloca() for speed. (see
		       // DAJ comments.
	02/17/89/jos - added check to _DSPWriteHm() to return if DSP not open
	02/19/89/jos - moved DSPDataIsAvailable and DSPAwaitData to DSPObject
	03/23/89/jos - placed call to DSPReadErrors in _DSPWriteHM().
	03/24/89/jos - rewrote hm_array usage for faster service
	06/19/89/jos - flushed curTime for better optimization
	06/19/89/jos - flushed s_mapped_only support in DSPMKFlushTimed...
	06/19/89/jos - took out s_simulated tests below DSPMKFlushTimed...
*/

#include "dsp/_dsp.h"
// #include <nextdev/dspvar.h>	/* DSPHostCommand() */
// #include <sys/stropts.h>	/* DSPDataIsAvailable() */
#include <sys/time.h>		/* DSPAwaitData(), DSPMessageGet() */
				/*     DSPAwaitUnsignedReply() */
#include <stdarg.h>

/**************************** DSP MESSAGES *********************************/

int DSPMessagesOff(void)
/* 
 * Turn off DSP messages at the source.
 */
{

    /* Turn off DSP messages. The DSP will not try to send any more. */
    if (s_simulated)
      fprintf(s_simulator_fp,";; Turn off DSP messages\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_DM_OFF));
    
    if (s_mapped_only) {
	if (s_simulated)
	  fprintf(s_simulator_fp,";; Await ack for DSP messages off\n");
	/* Read up to two pending DSP messages in the host interface regs. */
	DSP_UNTIL_ERROR(DSPAwaitMessage(DSP_DM_DM_OFF,DSPDefaultTimeLimit));
    }

    s_dsp_messages_disabled = 1;

    return 0;
}

int DSPMessagesOn(void)
/* 
 * Enable DSP messages.
 */
{

    s_dsp_messages_disabled = 0;
    if (s_simulated)
      fprintf(s_simulator_fp,";; Turn on DSP messages\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_DM_ON));
    if (s_mapped_only) {
	if (s_simulated)
	  fprintf(s_simulator_fp,";; Await ack for DSP messages on\n");
	DSP_UNTIL_ERROR(DSPAwaitMessage(DSP_DM_DM_ON,DSPDefaultTimeLimit));
    }
    return 0;
}


int DSPMessageGet(int *msgp)
/*
 * Return a single DSP message in *msgp, if one is waiting,
 * otherwise it returns the DSP error code DSP_ENOMSG.
 * The DSP message returned in *msgp is 24 bits, right justified.
 */
{
    if (s_dsp_messages_disabled)
      return(_DSPError(DSP_EPROTOCOL,
		       "DSPGetMessage: DSP Messages are turned off"));

 readMsg:
    if (DSPDataIsAvailable()) {
	_DSPReadDatum(msgp); /* Read back DSP message */
/***FIXME == DSPReadRX(msgp) for DSP messages only (errors stripped out) */
	if (_DSPTrace & DSP_TRACE_DSP) 
	  printf("DSP message = %s\n",DSPMessageExpand(*msgp));
	return 0;
    }
    if (_DSPTrace & DSP_TRACE_DSP) 
      printf("DSPGetMessage: DSPDataIsAvailable returns FALSE\n");
    return(DSP_ENOMSG);
}


int DSPAwaitAnyMessage(
    int *dspackp,		/* returned DSP message */
    int msTimeLimit)		/* time-out in milliseconds */
/*
 * Await any message from the DSP.
 */
{
    int ec;
    DSPMKFlushTimedMessages();
    if(ec=DSPAwaitData(msTimeLimit))
      return(_DSPError(ec,"DSPAwaitAnyMessage: DSPAwaitData() failed."));
    if(ec=DSPMessageGet(dspackp))
      return(_DSPError(ec,"DSPAwaitAnyMessage: "
			   "DSPMessageGet() failed after DSPAwaitData() "
			   "returned successfully."));
    return 0;
}

int DSPAwaitUnsignedReply(
    DSPAddress opcode,	       /* opcode of expected DSP message */
    DSPFix24 *datum,	       /* datum of  expected DSP message (returned) */
    int msTimeLimit)	       /* time-out in milliseconds */
/* 
 * Wait for specific DSP message containing an unsigned datum.
 */
{
    int dspack;

    while (1) {
	/* SimulatorOnly or CommandsOnly => error */
	DSP_UNTIL_ERROR(DSPAwaitAnyMessage(&dspack,msTimeLimit)); 
	if (DSP_MESSAGE_OPCODE(dspack)==opcode)
	  break;
	else {
	    char *arg;
	    arg = "DSPAwaitUnsignedReply: got unexpected DSP message ";
	    arg = DSPCat(arg,DSPMessageExpand(dspack));
	    arg = DSPCat(arg," while waiting for ");
	    arg = DSPCat(arg,DSPMessageExpand(opcode<<16));
	    _DSPError(DSP_EMISC,arg);
	}
    }
    *datum = DSP_MESSAGE_UNSIGNED_DATUM(dspack);
    if (s_simulated)
      fprintf(s_simulator_fp,";; DSP reply = %s\n\n",DSPMessageExpand(dspack));
    return 0;
}

int DSPAwaitSignedReply(
    DSPAddress opcode,	    /* opcode of expected DSP message */
    int *datum,		    /* datum of	 expected DSP message (returned) */
    int msTimeLimit)	    /* time-out in milliseconds */
/* 
 * Wait for specific DSP message containing a signed datum.
 */
{
    int ec;
    ec = DSPAwaitUnsignedReply(opcode,datum,msTimeLimit);
    *datum = DSP_MESSAGE_SIGNED_DATUM(*datum);
    return(ec);
}

int DSPAwaitMessage(
    DSPAddress opcode,		/* opcode of expected DSP message */
    int msTimeLimit)		/* time-out in milliseconds */
/* 
 * Return succesfully on specified DSP message 
 */
{
    int dspack;			/* unneeded opcode */
    return(DSPAwaitUnsignedReply(opcode,&dspack,msTimeLimit));
}

int _DSPForceIdle(void) 
{
    if (s_simulated) 
      fprintf(s_simulator_fp,";; Force DSP into IDLE state \n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_IDLE));
    return(DSPAwaitMessage(DSP_DM_IDLE,DSPDefaultTimeLimit));
}

/****************************** HOST MESSAGES *******************************/

/* See $DSP/doc/host-messages for documentation */

/* This struct is used to send a host message to the DSP atomically */

static int hm_array[DSP_MAX_HM*2]; /* times 2 so ovfl less likely to kill */
static int hm_ptr;

int DSPHostMessage(int msg)		
/* 
 * Issue untimed DSP "host message" (minus args) by issuing "xhm" 
 * host command.
 */
{
    return(DSPMKHostMessageTimed(DSPMK_UNTIMED,msg));
}

int DSPMKHostMessageTimedFromInts(
    int msg,	      /* Host message opcode. */
    int hiwd,	      /* High word of time stamp. */
    int lowd)	      /* Lo   word of time stamp. */
{
    DSPFix48 aTimeStamp = {hiwd,lowd};
    return(DSPMKHostMessageTimed(&aTimeStamp,msg));
}    

/************************** TIMED HOST MESSAGES *****************************/

int DSPMKHostMessageTimed(DSPFix48 *aTimeStampP, int msg)
{
    int ec;

    if (_DSPTrace & DSP_TRACE_HOST_MESSAGES)
      printf("Host message = 0x%X %s\n",msg,DSPTimeStampStr(aTimeStampP));
    if (s_simulated)
      fprintf(s_simulator_fp,
	      ";; DSPMKHostMessageTimed(0x%X) %s:\n",msg,
	      DSPTimeStampStr(aTimeStampP));
    if ( msg < DSP_HM_FIRST || msg > DSP_HM_LAST )
      return(_DSPError1(EDOM,"DSPMKHostMessageTimed: "
			    "opcode = 0x%s is too large",
			    _DSPCVHS(msg)));
    if (s_mapped_only) {

	/* put time stamp */
	if (aTimeStampP!=DSPMK_UNTIMED) {
	    DSP_UNTIL_ERROR(DSPWriteTX(DSP_FIX24_CLIP(aTimeStampP->high24))); 
	    DSP_UNTIL_ERROR(DSPWriteTX(DSP_FIX24_CLIP(aTimeStampP->low24))); 
	}

	/* put opcode */
	DSP_UNTIL_ERROR(DSPWriteTX((aTimeStampP==DSPMK_UNTIMED?
			     _DSP_HMTYPE_UNTIMED :_DSP_HMTYPE_TIMEDA)
			     | msg ));

	/* issue host command */
	ec = DSPHostCommand(DSP_HC_XHM); /* xhm host command */

    } else
      ec = DSPMKCallTimed(aTimeStampP,msg,0,0);
    
    if (ec)
      _DSPError1(ec,
		 "DSPMKHostMessageTimed: Could not issue host message 0x%s",
		 DSPCat(_DSPCVHS(msg),DSPTimeStampStr(aTimeStampP)));
    return(ec);
}


int _DSPStartHmArray(void)
{
    hm_ptr = 0;
    return 0;
}


int _DSPExtendHmArray(DSPFix24 *argArray, int nArgs)
{
    int i;
    for (i=0; i<nArgs; i++)  hm_array[hm_ptr++] = argArray[i];
    return 0;
}

int _DSPExtendHmArrayMode(DSPDatum *argArray, int nArgs, int mode)
/*
 * Add arguments to a host message (for the DSP).
 * Add nArgs elements from argArray to hm_array.
 * Mode codes are in <nextdev/dspvar.h> and discussed in 
 * DSPObject.h(DSPWriteArraySkipMode).
 */
{
    int i,j;
    switch (mode) {
    case DSP_MODE8: {
	register unsigned char* c = (unsigned char *)argArray;
	for (i=0,j=nArgs;j;j--)
	  hm_array[hm_ptr++] = *c++;
    } break;
    case DSP_MODE16: {
	register short* s = (short *)argArray;
	for (i=0,j=nArgs;j;j--)
	  hm_array[hm_ptr++] = *s++;
    } break;
    case DSP_MODE24: {
	register unsigned char* c = (unsigned char *)argArray;
	register unsigned int w;
	for (i=0,j=nArgs;j;j--) {
	    w = *c++;
	    w = (w<<8) | *c++;
	    w = (w<<8) | *c++;
	    hm_array[hm_ptr++] = w;
	}
    } break;
    case DSP_MODE32: {
	register DSPFix24* p = (DSPFix24 *)argArray;
	for (i=0,j=nArgs;j;j--)
	  hm_array[hm_ptr++] = *p++;
    } break;
    case DSP_MODE32_LEFT_JUSTIFIED: {
	register DSPFix24* p = (DSPFix24 *)argArray;
	for (i=0,j=nArgs;j;j--)
	  hm_array[hm_ptr++] = (*p++ >> 8);
    } break;
    default: 
	return _DSPError1(EINVAL,"_DSPExtendHmArrayMode: "
			  "Unrecognized data mode = %s",_DSPCVS(mode));
    }
    return 0;
}

int _DSPExtendHmArrayB(DSPFix24 *argArray, int nArgs)
{
    int i;
    for (i=nArgs-1; i>=0; i--)	hm_array[hm_ptr++] = argArray[i];
    return 0;
}


int _DSPFinishHmArray(DSPFix48 *aTimeStampP, DSPAddress opcode)
{
    int timed = ( aTimeStampP == DSPMK_UNTIMED ? 0 : 1 );
    int pfx = ( timed ? _DSP_HMTYPE_TIMEDA : _DSP_HMTYPE_UNTIMED );

    if ( opcode < DSP_HM_FIRST ||
	 opcode > DSP_HM_LAST )
      return(_DSPError1(EDOM,
			    "DSPHostMessage: opcode = 0x%s is out of bounds",
			    _DSPCVHS(opcode)));
    
    if (timed) {		/* install time stamp */
	hm_array[hm_ptr++] = DSP_FIX24_CLIP(aTimeStampP->high24); 
	hm_array[hm_ptr++] = DSP_FIX24_CLIP(aTimeStampP->low24); 
    }

    hm_array[hm_ptr++] = pfx | opcode; /* host message type and opcode */

    return 0;
}


int _DSPWriteHm(void)
{
    int ec,nb,nbd,hmtype,op;
    DSPFix48 aTimeStamp;

    if (s_mapped_only) {
	if (ec = DSPWriteTXArray(hm_array,hm_ptr))
	  return(_DSPError(DSP_EMISC,"_DSPWriteHm: host msg args failed"));
	if (ec = DSPHostCommand(DSP_HC_XHM)) /* xhm host command */
	  return(_DSPError(DSP_EMISC,"_DSPWriteHm: host command failed"));
    } else {
	if (ec = _DSPWriteHostMessage(hm_array, hm_ptr))
	  return _DSPError1(ec,
			    "_DSPWriteHm: _DSPWriteHostMessage failed for "
			    "message 0x%s", _DSPCVHS(hm_array[hm_ptr-1]));
    }
    return 0;
}
    

int _DSPCallTimedMaybeB(
    DSPFix48 *aTimeStampP,
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray,
    int reverse)
/*
 * Low-level routine for sending timed host messages to the DSP.
 * Called when the buffer of timed messages for the same time stamp
 * is flushed.
 */
{
    int ec,i;
    
    hm_ptr = 0;		/* no host message arguments yet written */

    if (reverse)
      _DSPExtendHmArrayB(argArray,nArgs);
    else
      _DSPExtendHmArray(argArray,nArgs);

    _DSPFinishHmArray(aTimeStampP,hm_opcode);

    return(_DSPWriteHm());
}


int DSPCall(
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray)
/*
 * Send an untimed host message to the DSP.
 */
{
    return(_DSPCallTimedMaybeB(DSPMK_UNTIMED,hm_opcode,nArgs,argArray,FALSE));
}	


int DSPCallB(
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray)
/*
 * Same as DSPCall() except that the argArray is sent in reverse
 * order to the DSP.
 */
{
    return(_DSPCallTimedMaybeB(DSPMK_UNTIMED,hm_opcode,nArgs,argArray,TRUE));
}	


int _DSPCallTimedFlush(
    DSPFix48 *aTimeStampP,
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray)
/*
 * Send a timed host message without accumulating messages for
 * the same time into a single buffer before sending.
 */
{
    return(_DSPCallTimedMaybeB(aTimeStampP,hm_opcode,nArgs,argArray,FALSE));
}	

int _DSPCallTimedFlushB(
    DSPFix48 *aTimeStampP,
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray)
/*
 * Same as DSPMKCallTimed() except that the argArray is sent in reverse
 * order to the DSP.
 */
{
    return(_DSPCallTimedMaybeB(aTimeStampP,hm_opcode,nArgs,argArray,TRUE));
}	


int _DSPCallTimedFlushV(DSPFix48 *aTimeStampP,DSPAddress hm_opcode,
			int nArgs,...)
/*
 * Usage is _DSPCallTimedFlushV(aTimeStampP,hm_opcode,N,arg1,...,ArgN);
 * Same as _DSPCallTimedFlush() except that a variable number of arguments 
 * is specified explicitly (using stdarg) rather than being passed in an
 * array.
 */
{
    va_list ap;
    int i,ec;

    va_start(ap,nArgs);

    if (s_mapped_only) {
	DSPFix24 *argArray;
	argArray = (DSPFix24 *)alloca(nArgs * sizeof(DSPFix24));
/*	DSP_MALLOC(argArray,DSPFix24,nArgs);  DAJ */
	for (i=0;i<nArgs;i++)
	  argArray[i] = va_arg(ap,DSPFix24);
	va_end(ap);
	return(_DSPCallTimedMaybeB(aTimeStampP,hm_opcode,nArgs,argArray,FALSE));
    } else {

	/* Don't call _DSPCallTimedMaybeB() and avoid an array copy */

	for (hm_ptr=0;hm_ptr<nArgs;) hm_array[hm_ptr++] = va_arg(ap,DSPFix24);
	_DSPFinishHmArray(aTimeStampP,hm_opcode);
	return(_DSPWriteHm());
    }
}


int DSPCallV(DSPAddress hm_opcode,int nArgs,...)
/*
 * Usage is int DSPCallV(hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as DSPCall() except that a variable number of host message arguments 
 * is specified explicitly (using stdarg) rather than being passed in an
 * array.
 */
{
    va_list ap;
    int i,ec;

    va_start(ap,nArgs);

    if (s_mapped_only) {
	DSPFix24 *argArray;
	argArray = (DSPFix24 *)alloca(nArgs * sizeof(DSPFix24));
//	DSP_MALLOC(argArray,DSPFix24,nArgs); // DAJ
	for (i=0;i<nArgs;i++)
	  argArray[i] = va_arg(ap,DSPFix24);
	va_end(ap);
	return(_DSPCallTimedMaybeB(DSPMK_UNTIMED,hm_opcode,nArgs,argArray,FALSE));
    } else {
	/* Don't call _DSPCallTimedMaybeB() and avoid an array copy */

	for (hm_ptr=0;hm_ptr<nArgs;) hm_array[hm_ptr++] = va_arg(ap,DSPFix24);
	va_end(ap);
	_DSPFinishHmArray(DSPMK_UNTIMED,hm_opcode);
	return(_DSPWriteHm());
    }
}


/********************** COMBINED TIMED HOST MESSAGES ************************/

/* We are seeing TMQ overflow. */
#define TMQ_FUDGE 0
#define TMQ_GUARD_ROOM 100 
static int minTMQRoom = 0; /* Lower bound on currently available TMQ room */

/* 
 * The purpose of combining timed host messages having the same time
 * into a single host message is to reduce the control bandwidth to the DSP.
 */

#define FILLER_FUDGE 2		/*** FIXME ***/
#define TIMED_MSG_FILLER (4+FILLER_FUDGE)
/* 2 for time stamp of host msg (opcode included already)+2 for topmk,botmk */
#define TIMED_MSG_BUF_SIZE (DSP_NB_HMS - TIMED_MSG_FILLER)

static int timedMsg[TIMED_MSG_BUF_SIZE];

#define FIRST_TIMED_WD (&timedMsg[0])
#define TIMED_WD_2 (&timedMsg[1])
#define LAST_TIMED_WD (&timedMsg[TIMED_MSG_BUF_SIZE - 1])

static int *curTimedWd = FIRST_TIMED_WD;
static int *timedArrEnd = LAST_TIMED_WD;
static int TMQMessageCount = 0;
static DSPFix48 curTimeStamp = {0,0};

#define TIMED_MSG_IS_EMPTY() (curTimedWd == FIRST_TIMED_WD)
#define TIMED_MSG_IS_NOT_EMPTY() (curTimedWd > FIRST_TIMED_WD)
#define TIMED_MSG_BUFFER_SIZE() (curTimedWd - FIRST_TIMED_WD)


int _DSPResetTMQ(void) 
{
    curTimedWd = FIRST_TIMED_WD;
    timedArrEnd = LAST_TIMED_WD;
    TMQMessageCount = 0;
    curTimeStamp.high24 = 0;
    curTimeStamp.low24 = 0;
    return 0;
}

int _DSPFlushTMQ(void) 
{
    if (TIMED_MSG_IS_EMPTY()) 
      return 0;

    /* Don't include first opcode. It is sent below. */
    if (TIMED_MSG_BUFFER_SIZE() > DSP_NB_HMS-2)
      _DSPFatalError(DSP_EPROTOCOL,
		    "_DSPFlushTMQ: Accumulated timed messages overflow HMS");

    if(!s_sound_out) {
        int logstate = DSPErrorLogIsEnabled();
	DSPDisableErrorLog();
        while(DSPAwaitHF3Clear(20*_DSP_MACH_DEADLOCK_TIMEOUT))
	  ;
	if (logstate)
	  DSPEnableErrorLog();
    }

#if 0				/* FIXME: Revive for Ariel board */
    if (s_mapped_only) {
	/* wait until TMQ can accept a write */
	DSPAwaitHF3Clear(0);
	DSP_UNTIL_ERROR(DSPWriteTXArrayB(TIMED_WD_2,TIMED_MSG_BUFFER_SIZE()-1));
	DSP_UNTIL_ERROR(DSPMKHostMessageTimed(&curTimeStamp,*FIRST_TIMED_WD));
    } else {
	DSP_UNTIL_ERROR(_DSPCallTimedMaybeB(&curTimeStamp,
					    *FIRST_TIMED_WD, 
					    TIMED_MSG_BUFFER_SIZE()-1,
					    TIMED_WD_2,DSP_TRUE));
    }
#else
    DSP_UNTIL_ERROR(_DSPCallTimedMaybeB(&curTimeStamp,*FIRST_TIMED_WD, 
					TIMED_MSG_BUFFER_SIZE()-1,
					TIMED_WD_2,DSP_TRUE));
#endif

    curTimedWd = FIRST_TIMED_WD;	       /* Reset ptr */
    TMQMessageCount = 0;
    return 0;
}

int DSPMKFlushTimedMessages(void)
/* 
 * Flush all combined timed messages for the current time. 
 * You must call this if you are sending updates to the DSP 
 * asynchronously (e.g. in response to MIDI or mouse events 
 * as opposed to via the musickit Conductor).  It should
 * also be called after all timed messages have been sent.
 */
{
    if (!TIMED_MSG_IS_EMPTY())
      DSP_UNTIL_ERROR(_DSPFlushTMQ());
    return 0;
}


#define TWO_TO_24   ((double) 16777216.0)

static double s_fix48ToDouble(register DSPFix48 *aFix48P)
{
    if (!aFix48P)
      return -1.0; /* FIXME or some other value */
    return ((double) aFix48P->high24) * TWO_TO_24 + (double) aFix48P->low24;
}


#define BACKUP_ERROR \
    _DSPError1(DSP_EMISC, "_DSPCheckTMQFlush: Warning:" \
	       "Attempt to move current time backwards from %s", \
	       DSPCat(DSPCat("current sample ", \
			     _DSPCVDS(s_fix48ToDouble(&curTimeStamp))), \
		      DSPCat(" to sample ", \
			     _DSPCVDS(s_fix48ToDouble(aTimeStampP)))))
    /* NO ERROR RETURN (warning only) */

int _DSPCheckTMQFlush(DSPFix48 *aTimeStampP, int nArgs)
/* 
 * Flush Timed Message buffer if the new message is timed for later
 * or if the new message won't fit because of the limited HMS size 
 * in the DSP.
 */
{
    register int newTimeH,newTimeL;
    register int curTimeH,curTimeL;

    /* Note: This routine gets called a LOT, and should be optimized */

    if (aTimeStampP == DSPMK_UNTIMED)
      return 0;

    curTimeH = curTimeStamp.high24;
    curTimeL = curTimeStamp.low24;
    newTimeH = aTimeStampP->high24;
    newTimeL = aTimeStampP->low24;

    /*
     * See if time has advanced, and if so, flush timed messages.
     */
    if (newTimeH == curTimeH) { /* Typical case (24b active timestamp)*/
	if (newTimeL<curTimeL && (newTimeL != 0 || newTimeH != 0)) /* TZM ok */
	  BACKUP_ERROR;
	else if (newTimeL>curTimeL) {
	    DSPMKFlushTimedMessages();
	    curTimeStamp = *aTimeStampP; /* Remember new time */
	} 
    } else { /* Have to deal with whole 48-bit time-stamps */
	if (newTimeH<curTimeH && (newTimeL != 0 || newTimeH != 0)) /* TZM ok */
	  BACKUP_ERROR;
	else {
	    DSPMKFlushTimedMessages();
	    curTimeStamp = *aTimeStampP; /* Remember new time */
	} 
    }

    /* 
     * Flush if current message overflows maximum message size to DSP 
     */
    if ((nArgs + curTimedWd) > LAST_TIMED_WD)
      /* Check if there's room. Since curTimedWd is preincremented, the
	 value nArgs + curTimedWd is 1 too big. But we need to save room
	 for the opcode, so this works out right. */
      if (TIMED_MSG_IS_EMPTY())	 /* First msg. Flushing won't help! */
	return(_DSPError(E2BIG,
			 "_DSPCheckTMQFlush: Too many host message args "
			 "to fit in existing HMS size on DSP"));
      else DSPMKFlushTimedMessages(); /* Make room for new msg by flushing */

    return 0;
}


int DSPMKCallTimed(
    DSPFix48 *aTimeStampP,
    DSPAddress hm_opcode,
    int nArgs,
    DSPFix24 *argArray)
/*
 * Enqueue a timed host message for the DSP.  If the time stamp of the
 * host message is greater than that of the host messages currently
 * in the timed message buffer, the buffer is flushed before the
 * new message is enqueued.  If the timed stamp is equal to those
 * currently in the buffer, it is appended to the buffer.  It is an
 * error for the time stamp to be less than that of the current
 * timed message buffer.  When the DSP Timed Message Queue is full,
 * this routine will block.
 */
{
    int i;

    if (aTimeStampP == DSPMK_UNTIMED)
      return(DSPCall(hm_opcode,nArgs,argArray));

    DSP_UNTIL_ERROR(_DSPCheckTMQFlush(aTimeStampP,nArgs));

    *curTimedWd++ = hm_opcode;	/* Install opcode of new message in buffer */
    /* 
       Load array backwards so that when combined by DSPMKFlushTimedMessages
       and written out backwards to the DSP, timed messages will be executed
       in the same order as when they are not combined. 
    */
    for (i = nArgs-1; i >= 0; i--)
	*curTimedWd++ = argArray[i];

    TMQMessageCount += 1;

    if (_DSPTrace & DSP_TRACE_NOOPTIMIZE)
      DSPMKFlushTimedMessages(); /* Flush for clarity in simulator file */

    if (aTimeStampP->high24 == 0 && aTimeStampP->low24 == 0) /* TZM */
      DSPMKFlushTimedMessages(); /* TZM must be alone on HMS! */

    return 0;
}
 

int _DSPCallTimedV(DSPFix48 *aTimeStampP,int hm_opcode,int nArgs,...)
/*
 * Usage is int _DSPCallTimedV(aTimeStampP,hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as _DSPCallTimed() except that a variable number of host message 
 * arguments is specified explicitly in the argument list (using stdarg) 
 * rather than being passed in an array.
 */
{
    va_list ap;
    int i;

    va_start(ap,nArgs);

    if (aTimeStampP == DSPMK_UNTIMED) {
	DSPFix24 *argArray;
	argArray = (DSPFix24 *)alloca(nArgs * sizeof(DSPFix24));
//	DSP_MALLOC(argArray,DSPFix24,nArgs); // DAJ
	for (i=0;i<nArgs;i++)
	  argArray[i] = va_arg(ap,DSPFix24);
	return(DSPCall(hm_opcode,nArgs,argArray));
    }

    DSP_UNTIL_ERROR(_DSPCheckTMQFlush(aTimeStampP,nArgs));

    *curTimedWd++ = hm_opcode;	/* Install opcode of new message in buffer */

    /* This was changed by DAJ. We need to write the arguments backwards
       into the TMQ buffer. */
    curTimedWd += nArgs;
    for (i = nArgs-1; i >= 0; i--)
	*--curTimedWd = va_arg(ap,int); /* Install hm args in msg buffer */
    curTimedWd += nArgs;		/* Set it to first loc after msg. */

    va_end(ap);

    TMQMessageCount += 1;

    if (_DSPTrace & DSP_TRACE_NOOPTIMIZE)
      DSPMKFlushTimedMessages(); /* Flush for clarity in simulator file */

    return 0;
}

int DSPMKCallTimedV(DSPFix48 *aTimeStampP,int hm_opcode,int nArgs,...)
/*
 * Usage is int _DSPCallTimedV(aTimeStampP,hm_opcode,nArgs,arg1,...,ArgNargs);
 * Same as _DSPCallTimed() except that a variable number of host message 
 * arguments is specified explicitly in the argument list (using stdarg) 
 * rather than being passed in an array.
 */
{
    va_list ap;
    int i;

    va_start(ap,nArgs);

    if (aTimeStampP == DSPMK_UNTIMED) {
	DSPFix24 *argArray;
	argArray = (DSPFix24 *)alloca(nArgs * sizeof(DSPFix24));
//	DSP_MALLOC(argArray,DSPFix24,nArgs); // DAJ
	for (i=0;i<nArgs;i++)
	  argArray[i] = va_arg(ap,DSPFix24);
	return(DSPCall(hm_opcode,nArgs,argArray));
    }

    DSP_UNTIL_ERROR(_DSPCheckTMQFlush(aTimeStampP,nArgs));

    *curTimedWd++ = hm_opcode;	/* Install opcode of new message in buffer */

    /* This was changed by DAJ. We need to write the arguments backwards
       into the TMQ buffer. */
    curTimedWd += nArgs;
    for (i = nArgs-1; i >= 0; i--)
	*--curTimedWd = va_arg(ap,int); /* Install hm args in msg buffer */
    curTimedWd += nArgs;		/* Set it to first loc after msg. */

    va_end(ap);

    TMQMessageCount += 1;

    if (_DSPTrace & DSP_TRACE_NOOPTIMIZE)
      DSPMKFlushTimedMessages(); /* Flush for clarity in simulator file */

    return 0;
}
 
/************************************ Ping ***********************************/

int DSPPingVersionTimeOut( int *verrevP, int msTimeLimit)
{
    if (s_simulated) 
      fprintf(s_simulator_fp,";; DSPPingVersion: HM_SAY_SOMETHING\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_SAY_SOMETHING));
    if (DSP_IS_SIMULATED_ONLY) return 0;
    if(DSPAwaitUnsignedReply(DSP_DM_IAA,verrevP,msTimeLimit))
      DSP_MAYBE_RETURN(_DSPError(DSP_ESYSHUNG,
				 "DSPPing: DSP system is not responding."));
    return 0;
}

int DSPPingVersion(int *verrevP)
{
    return DSPPingVersionTimeOut(verrevP,DSPDefaultTimeLimit);
}

int DSPPingTimeOut(int msTimeLimit)
{
    int verrev=0;
    return DSPPingVersionTimeOut(&verrev,msTimeLimit);
}

int DSPPing(void)
{
    int verrev=0;
/*   return DSPPingVersion(&verrev);  Need optimization... hence repeat code */
    if (s_simulated) 
      fprintf(s_simulator_fp,";; DSPPing: HM_SAY_SOMETHING\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_SAY_SOMETHING));
    if (DSP_IS_SIMULATED_ONLY) 
      return 0;
    if(DSPAwaitUnsignedReply(DSP_DM_IAA,&verrev,DSPDefaultTimeLimit))
      DSP_MAYBE_RETURN(_DSPError(DSP_ESYSHUNG,
				 "DSPPing: DSP system is not responding."));
    return 0;
}

/**************************** Memory mapped mode ****************************/

static int s_mm_lock = 0; /* lock variable */
static int s_mm_old_mapped = 0;
static int s_mm_old_icr = 0;
static DSPRegs *s_dsp_regs; /* DSP host interface (<nextdev/dspreg.h>) */


int _DSPCheckMappedMode(void) 
{

    if (DSPMKWriteDataIsEnabled())
      return _DSPError(DSP_EPROTOCOL,"_DSPCheckMappedMode: "
			"Cannot do this when write-data is enabled");

    if (s_mm_lock)
      return _DSPError(DSP_EPROTOCOL,"_DSPCheckMappedMode: "
			"Already in mapped mode!");

    return 0;
}


int _DSPEnterMappedModeNoCheck(void) /* Don't call this directly! */
{
    s_mm_old_mapped = s_mapped_only;

    s_mapped_only = 1;

    s_mm_old_icr = s_dsp_regs->icr;
    if (s_mm_old_icr != 1) {
	_DSPError1(0,"_DSPEnterMappedModeNoCheck: "
		   "Driver not in expected state. "
		   "icr = 0x%s. ",_DSPCVHS(s_mm_old_icr));
    }
    /* 
     * Turn off RREQ so that RXDF will not wake up the kernel 
     */
    s_dsp_regs->icr = 0; /* This hopefully won't hurt a bit */
    s_mm_lock = 1;
    return 0;
}


int _DSPEnterMappedModeNoPing(void) 
{
    s_dsp_regs = _DSPGetRegs();

    if (_DSPCheckMappedMode())
      return _DSPError(DSP_EPROTOCOL,"_DSPEnterMappedModeNoPing: Aborting");

    _DSPEnterMappedModeNoCheck();

    return 0;
}

int _DSPEnterMappedMode(void) 
{
    int ec;
    static int s_mm_old_pri = 0;

    s_dsp_regs = _DSPGetRegs();

    if (_DSPCheckMappedMode())
      return _DSPError(DSP_EPROTOCOL,"_DSPEnterMappedMode: Aborting");

    s_mm_old_pri = DSPGetMessagePriority(); /* Mach message priority */

    DSPSetMessagePriority(DSP_MSG_LOW);	/* Lowest Mach message priority */

    ec = DSPPingTimeOut(3000);	/* Flush driver message queue */
    if (ec)
      return _DSPError(ec,"_DSPEnterMappedMode: DSPPingTimeOut(3000) failed "
			"(3-second time-out)");

    DSPSetMessagePriority(s_mm_old_pri);  /* Restore Mach message priority */

    s_mm_old_icr = s_dsp_regs->icr;
    if (s_mm_old_icr != 1) {
	_DSPError1(0,"_DSPEnterMappedMode: "
		   "Driver not in expected state. "
		   "icr = 0x%s. ",_DSPCVHS(s_mm_old_icr));
    }

    _DSPEnterMappedModeNoCheck();

    return 0;
}

int _DSPExitMappedMode(void) 
{
    if (!s_mm_lock)
      return _DSPError(DSP_EPROTOCOL,"_DSPExitMappedMode: "
			"Mapped mode never entered!");

    s_mm_lock = 0; /* lock variable */

    if (!s_mm_old_mapped)
      _DSPDisableMappedOnly();

    s_dsp_regs->icr = s_mm_old_icr;

    return 0;
}
