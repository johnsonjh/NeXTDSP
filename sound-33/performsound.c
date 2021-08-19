/*
 *	performsound.c - recording and playback of sound.
 *	Written by Lee Boynton
 *	Copyright 1988-89 NeXT, Inc.
 *
 *	Modification History:
 *	02/15/90/mtm	Added support for SND_FORMAT_COMPRESSED.
 *			Check for mode optimizable in terminate_performance().
 *			Call terminate_performance() before endFun in
 *			performance_ended().
 *	03/20/90/mtm	Pass dma size to play_configure_dsp_data().
 *	03/21/90/mtm	Added support for SND_FORMAT_DSP_COMMANDS
 *	04/02/90/mtm	Added support for compressed recording from DSP
 *	04/06/90/mtm	Added stubs for playing on DSP, setting compression
 *			options, writing a DSP parameter value, and
 *			getting and setting the filter state.
 *	04/09/90/mtm	#include <mach_init.h>
 *	04/10/90/mtm	Implement SND{Set,Get}Filter() and
 *			SND{Set,Get}CompressionOptions().  Implement playback
 *			of emphasized sounds.
 *	04/11/90/mtm	Added #import <stdlib.h> per OS request.
 *	04/16/90/mtm	Support for playing back on DSP.
 *	04/30/90/mtm	Use multiple read requests rather that stream
 *			awaits during recording.
 *	05/08/90/mtm	Move endFun call back before terminate_performance() call
 *			in performance_ended().  Save sound header in pr field
 *			for use after endFun called (which may have freed sound).
 *	05/11/90/mtm	Implement 22K mono recording from DSP.
 *			Send sampleSkip=4 to DSP for 22K compression hack.
 *	05/17/90/mtm	Fix recording bug that caused called
 *			performance_started() to be called multiple times.
 *	05/18/90/mtm	Revert back to using stream control during recording
 *			so that the SoundKit's SoundMeter works, i.e,
 *			SNDSamplesProcessed() is acurate.
 *	05/23/90/mtm	Use multiple read request rather than stream control
 *			FOR MODE_COMPRESSED_IN ONLY to get around driver bug
 *			that results in missing data with dspsoundssi.asm.
 *	05/24/90/mtm	Initialize free list structs in enqueue_perf_request.
 *	05/25/90/mtm	Bit-faithful bits no longer sent in separate buffer.
 *			Made MODE_COMPRESSED_IN optimizable, and don't send
 *			numSamples to DSP in this mode.
 *	05/29/90/mtm	Release resource in terminate_performance() when only
 *			one request left in queue.
 *	05/30/90/mtm	Back to stream control for compressed case now that
 *			sound driver split stream bug fixed.
 *	06/11/90/mtm	DMA streaming for ssiplay.
 *	06/12/90/mtm	Truncate floats to ints using floor.
 *	06/14/90/mtm	Support dsp-initiated dma to the dsp for ssiplay and
 *			decompression.
 *	06/16/90/mtm	Pad compressed recording to decompression dma size.
 *	07/10/90/mtm	Call SNDReset() to stop soundout in stop_performance().
 *	07/17/90/mtm	SNDDRIVER_DSP_PROTO_SIMPLE now hard-coded as 0.
 *			SNDDRIVER_DSP_PROTO_HOSTMSG now SNDDRIVER_DSP_PROTO_DSPMSG.
 *	07/18/90/mtm	Get rid of static array in findDSPCore() and move static
 *			array to const in record_samples().
 *			Change compress.snd to ssicompress.snd.
 *			Change decompress.snd to sndoutdecompress.snd.
 *	06/29/90/mtm	Implement SNDStartRecordingFile().
 *	08/08/90/mtm	Add sample rate conversion support.
 *	08/13/90/mtm	Verify sample rate and channel count for DSP commands playback.
 *	09/28/90/mtm	Fix 22K mono compression (bug #9881).
 *	10/01/90/mtm	Dont' allow chained playback of compressed sounds (bug #7909).
 *	10/01/90/mtm	Don't pad compression, bump dma to the dsp count
 *			to vm_page_size (bug #10005).
 *	10/02/90/mtm	Check for request_count in SNDWait() (bug #10024).
 *	10/03/90/mtm	Take headerSize mod vm_page_size for dma to the dsp (bug #7912).
 *	10/04/90/mtm	Don't generate timeout error if stream_nsamples OK (bug #10011).
 *	10/08/90/mtm	Use different cores for mono and stereo resample (bug #10407).
 *	10/11/90/mtm	Only pad decompression to dma size
 *                      (real fix for bug #10011).
 *	10/24/90/mtm	Allow recordFD to be 0 (bug #11450).
 */

#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

#import <libc.h>
#import <c.h>
#import <mach.h>
#import <mach_init.h>
#import <cthreads.h>
#import <stdlib.h>
#import <math.h>
#import <string.h>
#import <sys/time_stamp.h>
#import <servers/netname.h>
#import "utilsound.h"
#import "filesound.h"
#import "sounddriver.h"
#import "accesssound.h"
#import "performsound.h"

/*
 * Timeouts
 */
#define REPLY_TIMEOUT (500)
#define SETUP_DELAY (5000)		/* must handle potential kernel pageins */
#define SHUTDOWN_DELAY (250)
#define MIN_PREEMPT_DUR (300)
#define REPLY_BACKLOG (10)		/* about 3 per committed performance */
#define	DSP_COMMANDS_SEND_TIMEOUT (1000)
#define	DSP_COMMANDS_REPLY_TIMEOUT (2000)
#define	COMPRESSED_IN_REPLY_TIMEOUT (1000)

/*
 * Performance modes. 'OUT' modes are for playing, 'IN' modes for recording.
 */
#define MODE_NONE			0
#define MODE_DIRECT_OUT			1
#define MODE_MULAW_CODEC_OUT		2
#define MODE_INDIRECT_OUT		3
#define MODE_SQUELCH_OUT		4
#define MODE_MONO_OUT			5
#define MODE_MONO_BYTE_OUT		6
#define MODE_CODEC_OUT			7
#define MODE_DSP_CORE_OUT		8
#define MODE_ENCAPSULATED_DSP_DATA_OUT	9	/* not used */
#define MODE_DSP_DATA_OUT		10	/* not used */
#define	MODE_COMPRESSED_OUT		11
#define	MODE_DSP_COMMANDS_OUT		12
#define	MODE_DSP_SSI_OUT		13
#define	MODE_DSP_SSI_COMPRESSED_OUT	14
#define	MODE_RESAMPLE_OUT		15

#define IS_PLAY_MODE(mode) (mode < 256)
#define IS_RECORD_MODE(mode) (mode >= 256)

#define MODE_MULAW_CODEC_IN		256
#define MODE_DSP_DATA_IN		257
#define	MODE_COMPRESSED_IN		258
#define	MODE_DSP_MONO22_IN		259

/*
 * Driver priorities used by this file
 */
#define HI_PRI SNDDRIVER_MED_PRIORITY
#define LO_PRI SNDDRIVER_LOW_PRIORITY

extern int kern_timestamp();

/*
 * The performance queue structure
 */
typedef struct _PerfReq {
    struct _PerfReq *prev;
    struct _PerfReq *next;
    int id;
    SNDSoundStruct *sound;
    SNDSoundStruct sndInfo;
    SNDNotificationFun beginFun;
    SNDNotificationFun endFun;
    int tag;
    int priority;
    int preempt;
    int mode;
    int access;
    short status;
    short err;
    int prevFilter;
    int dspOptions;
    int recordFD;
    int startTime;
    int duration;
    port_t dev_port;
    port_t owner_port;
    port_t perf_port;
    void *work_ptr;
    char *work_block_ptr;
    int work_count;
    int work_block_count;
} PerfReq;

#define NULL_REQUEST ((PerfReq *)0)

#define STATUS_FREE 0
#define STATUS_WAITING 1
#define STATUS_PENDING 2
#define STATUS_ACTIVE 3
#define STATUS_ABORTED 4

static int findDSPcore(char *name, SNDSoundStruct **s)
{
    static SNDSoundStruct *lastCore=0;
    static char *lastName = NULL;
    char buf[1024];
    int err;

    if (lastName && !strcmp(lastName,name)) {
	*s = lastCore;
	return SND_ERR_NONE;
    }
    strcpy(buf,"/usr/lib/sound/");
    strcat(buf,name);
#ifdef DEBUG
    strcpy(buf,name);	/* Get dsp code from current directory */
#endif
    strcat(buf,".snd");
    err = SNDReadSoundfile(buf,s);
    if (!err) {
	if (lastCore) SNDFree(lastCore);
	lastCore = *s;
	if (lastName)
	    free(lastName);
	lastName = malloc(strlen(name)+1);
	strcpy(lastName,name);
    }
#ifdef DEBUG
    if (err)
	printf("DSP code not found in current default directory\n");
#endif
    return err;
}

static void sleep_msec(int msec)
{
    port_t x;
    msg_header_t msg;
    int err = port_allocate(task_self(), &x);
    msg.msg_local_port = x;
    msg.msg_size = sizeof(msg_header_t);
    err = msg_receive(&msg, RCV_TIMEOUT, msec);
    port_deallocate(task_self(), x);
}

static int msec_timestamp()
{
    int msec;
    struct tsval now;
    
    kern_timestamp(&now);
    /*
     * This was the old way:
     * usec = (double)now.low_val + ((double)now.high_val * 65536.0 * 65536.0);
     * msec = (int)(usec / 1000.0);
     */
    msec = now.low_val / 1000;
    return msec;
}

static int calc_duration(int mode, SNDSoundStruct *s)
{
    double samp_count;
    
#ifdef DEBUG
    /*printf("Duration timeout disabled\n");
    return 1000*60*5;*/
#endif DEBUG

    if (mode == MODE_COMPRESSED_IN) {
        samp_count = (double)SNDBytesToSamples(s->dataSize, s->channelCount,
	                                       SND_FORMAT_LINEAR_16);
    } else
        samp_count = (double)SNDSampleCount(s);
    if (s->samplingRate && (samp_count > 0.))
	return (int)((samp_count * 1000.) / (double)s->samplingRate);
    else
	return 0;
}

static int calc_sample_count_from_ms(SNDSoundStruct *s, int milliseconds)
{
    if (s->samplingRate)
	return (int)((double)s->samplingRate * (double)milliseconds / 1000.);
    else
	return 0;
}

static int calc_ms_from_sample_count(SNDSoundStruct *s, int sampleCount)
{
    double samp_count = (double)sampleCount;
    if (s->samplingRate && samp_count > 0.)
	return (int)((samp_count * 1000.) / (double)s->samplingRate);
    else
	return 0;
}

static int calc_play_mode(SNDSoundStruct *s)
{
    int mode = MODE_NONE;
    if (!s || (s->magic != SND_MAGIC))
	return 0;
    switch (s->dataFormat) {
      case SND_FORMAT_LINEAR_16:
      case SND_FORMAT_EMPHASIZED:
	if (s->samplingRate == SND_RATE_LOW || 
	    s->samplingRate == SND_RATE_HIGH) {
	    if (s->channelCount == 2)
		mode = MODE_DIRECT_OUT;
	    else if (s->channelCount == 1)
		mode = MODE_MONO_OUT;
	} else if (s->samplingRate == (int)(floor(SND_RATE_CODEC))) {
	    if (s->channelCount == 1)
		mode = MODE_CODEC_OUT;	/* FIXME: just use resampler now? */
	} else if (s->channelCount == 1 || s->channelCount == 2)
		mode = MODE_RESAMPLE_OUT;
	break;
      case SND_FORMAT_MULAW_8:
	if (s->channelCount == 1) {
	    if (s->samplingRate == (int)(floor(SND_RATE_CODEC)))
		mode = MODE_MULAW_CODEC_OUT;
	}
	break;
      case SND_FORMAT_MULAW_SQUELCH:
	if (s->channelCount == 1) {
	    if (s->samplingRate == (int)(floor(SND_RATE_CODEC)))
		mode = MODE_SQUELCH_OUT;
	}
	break;
      case SND_FORMAT_LINEAR_8:
	if (s->channelCount == 1) {
	    if (s->samplingRate == SND_RATE_LOW)
		mode = MODE_MONO_BYTE_OUT;
	}
	break;
      case SND_FORMAT_INDIRECT:
	mode = MODE_INDIRECT_OUT;
	break;
      case SND_FORMAT_DSP_CORE:
	if ( (s->channelCount == 2) && 
	    (s->samplingRate == SND_RATE_LOW || 
	     s->samplingRate == SND_RATE_HIGH) )
	    mode = MODE_DSP_CORE_OUT;
	break;
      case SND_FORMAT_COMPRESSED:
      case SND_FORMAT_COMPRESSED_EMPHASIZED:
	if ((s->samplingRate == SND_RATE_LOW || 
	     s->samplingRate == SND_RATE_HIGH) &&
	    (s->channelCount == 1 || s->channelCount == 2))
	    mode = MODE_COMPRESSED_OUT;
	break;
      case SND_FORMAT_DSP_COMMANDS:
	if ( (s->channelCount == 2) && 
	    (s->samplingRate == SND_RATE_LOW || 
	     s->samplingRate == SND_RATE_HIGH) )
	    mode = MODE_DSP_COMMANDS_OUT;
	break;
      default:
	break;
    }
    return mode;
}

static int calc_dsp_play_mode(SNDSoundStruct *s)
{
    int mode = MODE_NONE;
    if (!s || (s->magic != SND_MAGIC))
	return 0;
    switch (s->dataFormat) {
      case SND_FORMAT_LINEAR_16:
      case SND_FORMAT_EMPHASIZED:
      case SND_FORMAT_DSP_DATA_16:
	/*
	 * Device must interpret data so channel count
	 * and sample rate are not checked.
	 */
	mode = MODE_DSP_SSI_OUT;
	break;
      case SND_FORMAT_COMPRESSED:
      case SND_FORMAT_COMPRESSED_EMPHASIZED:
	/*
	 * DSP compression code interprets this data so
	 * we must check the channel count.
	 */
	if (s->channelCount == 1 || s->channelCount == 2)
	    mode = MODE_DSP_SSI_COMPRESSED_OUT;
	break;
      default:
	break;
    }
    return mode;
}

static int calc_indirect_mode(SNDSoundStruct *s)
{
    SNDSoundStruct **iBlock = (SNDSoundStruct **)s->dataLocation;
    if (iBlock && *iBlock)
	return (calc_play_mode(*iBlock));
    else
	return MODE_NONE;
}

static int calc_record_mode(SNDSoundStruct *s)
{
    if (!s || (s->magic != SND_MAGIC))
	return 0;
    switch (s->dataFormat) {
      case SND_FORMAT_MULAW_8:
	if (s->channelCount == 1) {
	    if (s->samplingRate == (int)(floor(SND_RATE_CODEC)))
		return MODE_MULAW_CODEC_IN;
	}
	break;
      case SND_FORMAT_DSP_DATA_16:
	if (s->samplingRate == SND_RATE_LOW &&
	    s->channelCount == 1)
	    return MODE_DSP_MONO22_IN;
	else
	    /*
	     * User is free to interprete data in any way, so
	     * channel count and sampling rate are not checked.
	     */
	    return MODE_DSP_DATA_IN;
      case SND_FORMAT_COMPRESSED:
      case SND_FORMAT_COMPRESSED_EMPHASIZED:
	if (s->channelCount == 1 || s->channelCount == 2)
	    return MODE_COMPRESSED_IN;
	break;
      default:
	break;
    }
    return MODE_NONE;
}


static int calc_record_nsamples(PerfReq *pr)
{
    int err, count, width;
    char *p;
    err = SNDGetDataPointer(pr->sound,&p,&count,&width);
    if (err) return 0;
    return pr->sound->dataSize / (width * pr->sound->channelCount);
}

static int calc_play_nsamples(PerfReq *pr)
{
    int delta, count, max_count;
    int now = msec_timestamp();
    
    if (pr->status == STATUS_PENDING || pr->status == STATUS_WAITING) return 0;
    if (pr->status != STATUS_ACTIVE) return -1;
    max_count = SNDSampleCount(pr->sound);
    delta = now - pr->startTime;
    count = calc_sample_count_from_ms(pr->sound,delta);
    return (count > max_count)? max_count : count;
}

static int modeOptimizable(int mode, int last_mode, 
			   SNDSoundStruct *s, SNDSoundStruct *last_s)
{
    switch (mode) {
      case MODE_MULAW_CODEC_OUT:
      case MODE_MULAW_CODEC_IN:
      case MODE_MONO_BYTE_OUT:
      case MODE_CODEC_OUT:
      case MODE_DSP_SSI_OUT:
      case MODE_DIRECT_OUT:
      case MODE_MONO_OUT:
      case MODE_DSP_DATA_IN:
	return (mode == last_mode &&
		s->samplingRate == last_s->samplingRate);
      case MODE_COMPRESSED_IN:
	/* FIXME: must also check compression subheader parameters */
	return (mode == last_mode &&
		s->channelCount == last_s->channelCount);
      case MODE_DSP_MONO22_IN:
	return (mode == last_mode &&
		s->samplingRate == last_s->samplingRate &&
		s->channelCount == last_s->channelCount);
      case MODE_DSP_COMMANDS_OUT:
      case MODE_DSP_CORE_OUT:
      case MODE_DSP_SSI_COMPRESSED_OUT:
      case MODE_COMPRESSED_OUT:
      case MODE_RESAMPLE_OUT:
      default:
	return 0;
    }
}

static int calc_access(int mode)
{
    if (IS_PLAY_MODE(mode)) {
	if (mode == MODE_DIRECT_OUT)
	    return SND_ACCESS_OUT;
	else if (mode == MODE_DSP_SSI_OUT ||
		 mode == MODE_DSP_SSI_COMPRESSED_OUT)
	    return SND_ACCESS_DSP;
	else
	    return (SND_ACCESS_OUT | SND_ACCESS_DSP);
    } else {
	if (mode == MODE_MULAW_CODEC_IN)
	    return SND_ACCESS_IN;
	else if (mode == MODE_DSP_DATA_IN || mode == MODE_COMPRESSED_IN ||
		 mode == MODE_DSP_MONO22_IN)
	    return SND_ACCESS_DSP;
	else
	    return (SND_ACCESS_IN | SND_ACCESS_DSP);
    }
    return 0;
}


static int play_configure_dsp_core(SNDSoundStruct *core, int dmasize, int rate,
				   port_t dev_port, port_t owner_port,
				   port_t *cmd_port)
    /* dspcores now wait for the bufsize to be transmitted to them before starting */
{
    int protocol = 0;
    int config, err;
    port_t stream_port;
    
    if (rate > SND_RATE_LOW)
	config = SNDDRIVER_STREAM_DSP_TO_SNDOUT_44;
    else
	config = SNDDRIVER_STREAM_DSP_TO_SNDOUT_22;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 2,
				 24*1024,
				 32*1024,
				 &protocol,
				 &stream_port);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
    if (err != KERN_SUCCESS) return err;
    err = SNDBootDSP(dev_port, owner_port, core); /* does *not* autorun! */
    if (err != KERN_SUCCESS) return err;
    err = snddriver_get_dsp_cmd_port(dev_port,owner_port,cmd_port);
    return err;
}

static int play_configure_dsp_data(SNDSoundStruct *core, int dmasize,
				   int width, int rate,
				   int low_water, int high_water,
				   port_t dev_port, port_t owner_port,
				   port_t *stream_port)
{
    int protocol = 0;
    int config, err;
    port_t cmd_port;
    
    if (rate > SND_RATE_LOW)
	config = SNDDRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_44;
    else
	config = SNDDRIVER_STREAM_THROUGH_DSP_TO_SNDOUT_22;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 width,
				 low_water,
				 high_water,
				 &protocol,
				 stream_port);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
    if (err != KERN_SUCCESS) return err;
    err = SNDBootDSP(dev_port, owner_port, core);
    err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
    if  (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_write(cmd_port,&dmasize,1,4,HI_PRI);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_configure_dma_dsp_data(SNDSoundStruct *core, int dmasize,
				   int width, int rate,
				   int low_water, int high_water,
				   port_t dev_port, port_t owner_port,
				   port_t *stream_port)
{
    int protocol = 0;
    int config, err;
    port_t cmd_port;
    
    if (rate > SND_RATE_LOW)
	config = SNDDRIVER_DMA_STREAM_THROUGH_DSP_TO_SNDOUT_44;
    else
	config = SNDDRIVER_DMA_STREAM_THROUGH_DSP_TO_SNDOUT_22;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 width,
				 low_water,
				 high_water,
				 &protocol,
				 stream_port);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
    if (err != KERN_SUCCESS) return err;
    err = SNDBootDSP(dev_port, owner_port, core);
    err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
    if  (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_write(cmd_port,&dmasize,1,4,HI_PRI);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_configure_dsp_commands(int dmasize,
				       int width, int rate,
				       int low_water, int high_water,
				       port_t dev_port, port_t owner_port,
				       port_t *stream_port)
{
    int protocol = SNDDRIVER_DSP_PROTO_DSPMSG;
    int config, err;
    port_t cmd_port;
    
    if (rate > SND_RATE_LOW)
	config = SNDDRIVER_STREAM_DSP_TO_SNDOUT_44;
    else
	config = SNDDRIVER_STREAM_DSP_TO_SNDOUT_22;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 width,
				 low_water,
				 high_water,
				 &protocol,
				 stream_port);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
    if  (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_reset(cmd_port, HI_PRI);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_configure_dsp_ssi(SNDSoundStruct *core, int dmasize,
				  int width, int rate,
				  int low_water, int high_water,
				  port_t dev_port, port_t owner_port,
				  port_t *stream_port)
{
    int protocol = 0;
    int config, err;
    port_t cmd_port;
    
    config = SNDDRIVER_DMA_STREAM_TO_DSP;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 width,
				 low_water,
				 high_water,
				 &protocol,
				 stream_port);
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
    if (err != KERN_SUCCESS) return err;
    err = SNDBootDSP(dev_port, owner_port, core);
    if (err) return SND_ERR_CANNOT_PLAY;
    err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
    if  (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_write(cmd_port,&dmasize,1,4,HI_PRI);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_configure_direct(int rate, port_t dev_port,
				 port_t owner_port, port_t *stream_port)
{
    int protocol = 0;
    int config, err;
    int dmasize = vm_page_size / 2;
    
    if (rate > SND_RATE_LOW)
	config = SNDDRIVER_STREAM_TO_SNDOUT_44;
    else
	config = SNDDRIVER_STREAM_TO_SNDOUT_22;
    err = snddriver_stream_setup(dev_port, owner_port,
				 config,
				 dmasize,
				 2,
				 256*1024,
				 512*1024,
				 &protocol,
				 stream_port);
    return err;
}

typedef struct {		/* keep in sync with dsp library DSPObject.c */
    int	sampleCount;
    int	dspBufSize;
    int	soundoutBufSize;
    int	reserved;
} commandsSubHeader;

#define	DECOMPRESS_DMA_SIZE	2048
#define	RESAMPLE_DMA_SIZE	1024

static int play_configure(int mode, SNDSoundStruct *s,port_t dev_port,
			  port_t owner_port, port_t *play_port,
			  int dspOptions)
{
    int err;
    int rate = s->samplingRate;
    int dmasize = vm_page_size / 2;
    int headerSize, timeIncrement;
    SNDSoundStruct *core, **iBlock;
    port_t cmd_port;
    commandsSubHeader *subheader;
    
    switch (mode) {
      case MODE_DIRECT_OUT:
	return play_configure_direct(rate,dev_port,owner_port,play_port);
      case MODE_MULAW_CODEC_OUT:
	err = findDSPcore("mulawcodec",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	return play_configure_dsp_data(core,dmasize,1,rate,48*1024,64*1024,
				       dev_port,owner_port,play_port);
      case MODE_INDIRECT_OUT:
	iBlock = (SNDSoundStruct **)s->dataLocation;
	if (iBlock && *iBlock)
	    return play_configure(calc_play_mode(*iBlock), *iBlock,
				  dev_port,owner_port,play_port,dspOptions);
	return SND_ERR_UNKNOWN;
      case MODE_SQUELCH_OUT:
	err = findDSPcore("mulawcodecsquelch",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	return play_configure_dsp_data(core,dmasize,1,rate,48*1024,64*1024,
				       dev_port,owner_port,play_port);
      case MODE_MONO_OUT:
	err = findDSPcore("mono",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	return play_configure_dsp_data(core,dmasize,2,rate,512*1024,768*1024,
				       dev_port,owner_port,play_port);
      case MODE_MONO_BYTE_OUT:
	err = findDSPcore("monobyte",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	return play_configure_dsp_data(core,dmasize,1,rate,64*1024,96*1024,
				       dev_port,owner_port,play_port);
      case MODE_CODEC_OUT:
	err = findDSPcore("codec",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	return play_configure_dsp_data(core,dmasize,2,rate,48*1024,64*1024,
				       dev_port,owner_port,play_port);
      case MODE_DSP_CORE_OUT:
	/* dmasize = get_dmasize_from_header */
	return play_configure_dsp_core(s,dmasize,rate,
				       dev_port,owner_port,play_port);
      case MODE_COMPRESSED_OUT:
	dmasize = DECOMPRESS_DMA_SIZE;
	err = findDSPcore("sndoutdecompress",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = play_configure_dma_dsp_data(core,dmasize,2,rate,48*1024,64*1024,
					  dev_port,owner_port,play_port);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	/*
	 * DMA must start on a page boundry so the whole sound is sent -
	 * tell the dsp how many words to ignore (the sound header).
	 */
	headerSize = (s->dataLocation % vm_page_size) / 2;
	err = snddriver_dsp_write(cmd_port,&headerSize,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port,&s->channelCount,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	return SND_ERR_NONE;
      case MODE_RESAMPLE_OUT:
	dmasize = RESAMPLE_DMA_SIZE;	/* FIXME: currently ignored by
					   resample.asm */
	if (s->channelCount == 1)
	    err = findDSPcore("resample1",&core);
	else
	    err = findDSPcore("resample2",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = play_configure_dma_dsp_data(core,dmasize,2,rate,48*1024,64*1024,
					  dev_port,owner_port,play_port);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	/*
	 * DMA must start on a page boundry so the whole sound is sent -
	 * tell the dsp how many words to ignore (the sound header).
	 */
	headerSize = (s->dataLocation % vm_page_size) / 2;
	err = snddriver_dsp_write(cmd_port,&headerSize,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port,&s->channelCount,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	/*
	 * Always resample to 44K for playback.
	 */
	timeIncrement = (int)(((double)(1<<19)) * 
	 	             (((double)s->samplingRate)/SND_RATE_HIGH) + 0.5);
	err = snddriver_dsp_write(cmd_port,&timeIncrement,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	return SND_ERR_NONE;
      case MODE_DSP_COMMANDS_OUT:
	subheader = (commandsSubHeader *)((char *)s + s->dataLocation);
	dmasize = subheader->dspBufSize;
	/* FIXME: should set soundout dma size from subheader */
	return play_configure_dsp_commands(dmasize,2,rate,0,0,
					   dev_port,owner_port,play_port);
      case MODE_DSP_SSI_OUT:
	dmasize = 1024;
	err = findDSPcore("ssiplay",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = play_configure_dsp_ssi(core,dmasize,2,rate,48*1024,64*1024,
				     dev_port,owner_port,play_port);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port,&dspOptions,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	return SND_ERR_NONE;
      case MODE_DSP_SSI_COMPRESSED_OUT:
	dmasize = DECOMPRESS_DMA_SIZE;
	/* FIXME: currently not implemented */
	err = findDSPcore("ssidecompress",&core);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = play_configure_dsp_ssi(core,dmasize,2,rate,48*1024,64*1024,
				     dev_port,owner_port,play_port);
	if (err != SND_ERR_NONE) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port,&dspOptions,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port,&s->channelCount,1,4,HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	return SND_ERR_NONE;
      case MODE_ENCAPSULATED_DSP_DATA_OUT:
      case MODE_DSP_DATA_OUT:
      default:
	return SND_ERR_BAD_CONFIGURATION;
    }
}

#define	DSP_LOW_WATER		(512*1024)
#define	DSP_HIGH_WATER		(768*1024)
#define	CODEC_LOW_WATER		(48*1024)
#define	CODEC_HIGH_WATER	(64*1024)
#define	DSP_DMA_SIZE		(vm_page_size/2)
#define	DSP_MONO22_DMA_SIZE	1024	/* hard-coded in derecord22m.asm */
#define	CODEC_DMA_SIZE		256
#define	COMPRESS_DMA_SIZE	512

static int record_configure(int mode, SNDSoundStruct *s,port_t dev_port, 
			    port_t owner_port, port_t *record_port)
{
    int err, protocol = 0;
    SNDSoundStruct *core;
    
    switch (mode) {
      case MODE_MULAW_CODEC_IN:
	err = snddriver_stream_setup(dev_port, owner_port,
				     SNDDRIVER_STREAM_FROM_SNDIN,
				     CODEC_DMA_SIZE,
				     1,
				     CODEC_LOW_WATER,
				     CODEC_HIGH_WATER,
				     &protocol,
				     record_port);
	return err? SND_ERR_CANNOT_CONFIGURE : SND_ERR_NONE;
      case MODE_DSP_DATA_IN:
	err = snddriver_stream_setup(dev_port, owner_port,
				     SNDDRIVER_STREAM_FROM_DSP,
				     DSP_DMA_SIZE,
				     2,
				     DSP_LOW_WATER,
				     DSP_HIGH_WATER,
				     &protocol,
				     record_port);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = findDSPcore("dsprecord",&core);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = SNDBootDSP(dev_port,owner_port,core);
	return err? SND_ERR_CANNOT_CONFIGURE : SND_ERR_NONE;
      case MODE_COMPRESSED_IN:
	err = snddriver_stream_setup(dev_port, owner_port,
				     SNDDRIVER_STREAM_FROM_DSP,
				     COMPRESS_DMA_SIZE,
				     2,
				     DSP_LOW_WATER,
				     DSP_HIGH_WATER,
				     &protocol,
				     record_port);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = findDSPcore("ssicompress",&core);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = SNDBootDSP(dev_port,owner_port,core);
	return err? SND_ERR_CANNOT_CONFIGURE : SND_ERR_NONE;
      case MODE_DSP_MONO22_IN:
	err = snddriver_stream_setup(dev_port, owner_port,
				     SNDDRIVER_STREAM_FROM_DSP,
				     DSP_MONO22_DMA_SIZE,
				     2,
				     DSP_LOW_WATER,
				     DSP_HIGH_WATER,
				     &protocol,
				     record_port);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = snddriver_dsp_protocol(dev_port,owner_port,protocol);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = findDSPcore("derecord22m",&core);
	if (err) return SND_ERR_CANNOT_CONFIGURE;
	err = SNDBootDSP(dev_port,owner_port,core);
	return err? SND_ERR_CANNOT_CONFIGURE : SND_ERR_NONE;
      default:
	return SND_ERR_BAD_CONFIGURATION;
    }
}

/*
 * Low level performance support
 */

static int play_samples(int tag, SNDSoundStruct *s, port_t stream_port,
			port_t reply_port)
{
    int err, size, totalSize, headerSize, pages;
    int dmaBytes = DECOMPRESS_DMA_SIZE * 2;
    char *p = (char *)s;

    if (s->dataFormat == SND_FORMAT_LINEAR_16   ||
	s->dataFormat == SND_FORMAT_DSP_DATA_16 ||
        s->dataFormat == SND_FORMAT_EMPHASIZED  ||
        s->dataFormat == SND_FORMAT_COMPRESSED  ||
	s->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED)
	size = s->dataSize / 2;
    else
	size = s->dataSize;
    /*
     * Formats that use DMA writes to DSP send the sound header with the
     * data because the buffer must be page-aligned.  The total size must
     * be bumped to the next dma size.  This is OK because the sound was
     * created with either vm_allocate() or map_fd() and therefore has
     * memory up to the next page size.
     */
    if (s->dataFormat == SND_FORMAT_COMPRESSED ||
	s->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED) {
	headerSize = s->dataLocation;
	if (headerSize >= vm_page_size) {
	    pages = headerSize / vm_page_size;
	    headerSize -= pages * vm_page_size;
	    p += pages * vm_page_size;
	}
	totalSize = s->dataSize + headerSize;
	if (totalSize % dmaBytes)
	    totalSize = (totalSize + dmaBytes) & ~(dmaBytes - 1);
	size = totalSize / 2;	/* 16-bit samples */
    } else
	p += s->dataLocation;
    if (!size) return SND_ERR_CANNOT_PLAY;

    err = snddriver_stream_start_writing(stream_port,
    					 (void *)p,
					 size,
					 tag,
					 0,0,
					 1,1,0,0,0,0,
					 reply_port);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_resamples(int tag, SNDSoundStruct *s, port_t stream_port,
			  port_t reply_port)
/* Plays MODE_RESAMPLE_OUT */
{
    int err, size, totalSize, headerSize, pages;
    int dmaBytes = RESAMPLE_DMA_SIZE * 2;
    char *p = (char *)s;

    /*
     * Formats that use DMA writes to DSP send the sound header with the
     * data because the buffer must be page-aligned.  The total size must
     * be bumped to the next dma size.  This is OK because the sound was
     * created with either vm_allocate() or map_fd() and therefore has
     * memory up to the next page size.
     */
    headerSize = s->dataLocation;
    if (headerSize >= vm_page_size) {
	pages = headerSize / vm_page_size;
	headerSize -= pages * vm_page_size;
	p += pages * vm_page_size;
    }
    totalSize = s->dataSize + headerSize;
    if (totalSize % dmaBytes) {
	/* Must zero extra memory for resample filter */
	bzero(p+totalSize, dmaBytes - (totalSize % dmaBytes));
	totalSize = (totalSize + dmaBytes) & ~(dmaBytes - 1);
    }
    size = totalSize / 2;	/* 16-bit samples */
    if (!size) return SND_ERR_CANNOT_PLAY;

    err = snddriver_stream_start_writing(stream_port,
    					 (void *)p,
					 size,
					 tag,
					 0,0,
					 1,1,0,0,0,0,
					 reply_port);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static int play_indirect_samples(int tag, SNDSoundStruct *s,
				 port_t stream_port, port_t reply_port)
{
    SNDSoundStruct *s2, **iBlock = (SNDSoundStruct **)s->dataLocation;
    int err, size, width; 
    int first, last, region_count = 0;
    char *ptr;
    
    if (!*iBlock) return SND_ERR_CANNOT_PLAY;
    
    first = 1;
    last = 0;
    err = snddriver_stream_control(stream_port,tag,SNDDRIVER_PAUSE_STREAM);
    if (err) return SND_ERR_KERNEL;
    while(s2 = *iBlock++) {
	SNDGetDataPointer(s2,&ptr,&size,&width);
	region_count++;
	if (!(*iBlock)) last = 1;
	err = snddriver_stream_start_writing(stream_port,
					     (void *)ptr,
					     size,
					     tag,
					     0,0,
					     first,last,0,0,0,0,
					     reply_port);
	if (err) return SND_ERR_KERNEL;
	first = 0;
    }
    err = snddriver_stream_control(stream_port,tag,SNDDRIVER_RESUME_STREAM);
    return err? SND_ERR_KERNEL : SND_ERR_NONE;
}

static int play_dsp_core(int tag, SNDSoundStruct *s,
			 port_t cmd_port, port_t reply_port)
{
    int err;
    int dmasize = vm_page_size / 2;	/* FIXME: get from header of sound */
    err = snddriver_dspcmd_req_condition(cmd_port,
    					 SNDDRIVER_ISR_HF2, 0,
    					 LO_PRI, reply_port);
    if (err) return SND_ERR_CANNOT_PLAY;
    err = snddriver_dspcmd_req_condition(cmd_port,
    					 SNDDRIVER_ISR_HF2, SNDDRIVER_ISR_HF2,
    					 LO_PRI, reply_port );
    if (err != KERN_SUCCESS) return err;
    err = snddriver_dsp_write(cmd_port,&dmasize,1,4,HI_PRI);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

static s_kill_dsp_commands_thread;
typedef struct {
    port_t cmd_port;
    msg_header_t *message;
    int size;
} dsp_commands_struct;

static any_t dsp_commands_thread(any_t args)
{
    int err;
    int count = 0;
    msg_header_t *msg;
    dsp_commands_struct *info;
    
    info = (dsp_commands_struct *)args;
    msg = info->message;
    while (count < info->size) {
        msg->msg_remote_port = info->cmd_port;
        msg->msg_local_port = PORT_NULL;
        err = msg_send(msg, SEND_TIMEOUT, DSP_COMMANDS_SEND_TIMEOUT);
	while (err == SEND_TIMED_OUT) {
	    if (s_kill_dsp_commands_thread)
	        break;
            err = msg_send(msg, SEND_TIMEOUT, DSP_COMMANDS_SEND_TIMEOUT);
	}
#ifdef DEBUG
	if (err != KERN_SUCCESS)
            printf("dsp commands thread msg_send error %d\n", err);
#endif
        if (s_kill_dsp_commands_thread)
	    break;
        count += msg->msg_size;
        msg = (msg_header_t *) ((char *)msg + msg->msg_size);
    }
    free(args);
    cthread_exit(0);
    return NULL;
}

static int play_dsp_commands(int tag, SNDSoundStruct *s,
			     port_t cmd_port, port_t reply_port)
{
    dsp_commands_struct *args;
    int err;
    
    args = (dsp_commands_struct *)malloc(sizeof(dsp_commands_struct));
    if (!args)
        return SND_ERR_KERNEL;
    
    args->cmd_port = cmd_port;
    args->message = (msg_header_t *) ((char *)s + s->dataLocation +
				      sizeof(commandsSubHeader));
    args->size = s->dataSize - sizeof(commandsSubHeader);
    s_kill_dsp_commands_thread = FALSE;
    cthread_detach(cthread_fork(dsp_commands_thread, (any_t)args));
    err = snddriver_dspcmd_req_msg(cmd_port, reply_port);
    return err? SND_ERR_CANNOT_PLAY : SND_ERR_NONE;
}

typedef struct {			/* Keep in sync with decompression.asm */
    int	originalSize;
    int method;
    int numDropped;
    int encodeLength;
    int reserved;
} compressionSubHeader;

#define	BIT_FAITHFUL	1		/* method=1 for bit faithful */

static int record_samples(PerfReq *pr, port_t reply_port, int dmasize)
{
    int err;
    int tag = pr->tag;
    port_t stream_port = pr->perf_port, cmd_port;
    int sampleSkip;
    compressionSubHeader *subheader = NULL;

#if 0
    /* Currently, compress.asm has a max encode length of 256 */
    static const short bestEncodeLength[] = {
	64,	/* shift 0 - currently not used */
	64,	/* shift 1 - currently not used */
	128,	/* shift 2 - currently not used */
	128,	/* shift 3 - currently not used */
	256,	/* shift 4 */
	256,	/* shift 5 */
	512,	/* shift 6 */
	512,	/* shift 7 */
	512	/* shift 8 */
	};
#endif

    pr->work_ptr = (void *)((char *)pr->sound + pr->sound->dataLocation);
    pr->work_count = pr->sound->dataSize;
    pr->sound->dataSize = 0;
    if (!pr->work_count) return SND_ERR_CANNOT_RECORD;
    if (pr->mode == MODE_COMPRESSED_IN) {
        if (pr->work_count <= sizeof(compressionSubHeader))
	    return SND_ERR_CANNOT_RECORD;
        subheader = (compressionSubHeader *)pr->work_ptr;
	subheader->originalSize = pr->work_count;
	if (subheader->method)
	    subheader->method = BIT_FAITHFUL;
	if (subheader->numDropped < 4)
	    subheader->numDropped = 4;
	else if (subheader->numDropped > 8)
	    subheader->numDropped = 8;
	subheader->encodeLength = 256; /*bestEncodeLength[subheader->numDropped];*/
	
	/* Max encodeLength for 22K hack is 128 */
	if ((pr->sound->samplingRate == SND_RATE_LOW) &&
	    subheader->encodeLength > 128)
	    subheader->encodeLength = 128;
	
	subheader->reserved = 0;

	/* Write subheader if recording to a file */
	if (pr->recordFD >= 0)
	    if (write(pr->recordFD, (char *)pr->work_ptr, sizeof(compressionSubHeader))
		!= sizeof(compressionSubHeader))
		return SND_ERR_CANNOT_WRITE;

        pr->work_ptr = (void *) ((char *)pr->work_ptr + sizeof(compressionSubHeader));
	pr->work_block_ptr = (char *)pr->work_ptr;
	pr->work_count -= sizeof(compressionSubHeader);
	pr->work_block_count = 0;
	pr->sound->dataSize = sizeof(compressionSubHeader);
    }
    err = snddriver_stream_start_reading(stream_port,
					 0,
					 (pr->access & SND_ACCESS_DSP ?
					  pr->work_count/2 : pr->work_count),
					 tag,
					 1,1,0,0,0,0,
					 reply_port);
#if DEBUG
    if (err)
	printf("record_samples received error %d\n", err);
#endif
    if (err) return SND_ERR_CANNOT_RECORD;
    
    /*
     * FIXME: parameters are written even in the optimized case (dsp already
     * running).  The DSP does not read them - they queue up in the driver.
     */
    if (pr->access & SND_ACCESS_DSP) {
	err = snddriver_get_dsp_cmd_port(pr->dev_port,pr->owner_port, 
					 &cmd_port);
	if  (err != KERN_SUCCESS) return SND_ERR_CANNOT_RECORD;
	err = snddriver_dsp_write(cmd_port,&dmasize,1,4,HI_PRI);
	if (err != KERN_SUCCESS) return SND_ERR_CANNOT_RECORD;
	if (pr->mode == MODE_COMPRESSED_IN) {
	    err = snddriver_dsp_write(cmd_port,&pr->sound->channelCount,1,4,HI_PRI);
	    err = snddriver_dsp_write(cmd_port,&subheader->method,1,4,HI_PRI);
	    err = snddriver_dsp_write(cmd_port,&subheader->numDropped,1,4,HI_PRI);
	    err = snddriver_dsp_write(cmd_port,&subheader->encodeLength,1,4,HI_PRI);
	    sampleSkip = (pr->sound->samplingRate == SND_RATE_HIGH ? 2 : 4);
	    err = snddriver_dsp_write(cmd_port,&sampleSkip,1,4,HI_PRI);
	    if (err != KERN_SUCCESS) return SND_ERR_CANNOT_RECORD;
        }
    }
    return err? SND_ERR_CANNOT_RECORD : SND_ERR_NONE;
}


/*
 * The performance queue. Contains both play and record requests. Several
 * entries may be active at one time.
 */

static volatile PerfReq *perf_q_head = 0, *perf_q_tail = 0, *free_list = 0;
static int request_count = 0, request_max = 0, next_id = 1;
static mutex_t q_lock = 0;
static condition_t q_changed=0;

static int enqueue_perf_request(int mode,
				SNDSoundStruct *s,
				int tag,
				int priority,
				int preempt,
				SNDNotificationFun beginFun,
				SNDNotificationFun endFun,
				int dspOptions, int fd)
{
    int err, i = request_max;
    PerfReq *pr, *npr;
    port_t junk_port;
    
    if (free_list) {
	for (i=0; i<request_max;i++) {
	    if (free_list[i].status == STATUS_FREE)
		break;
	}
    }
    if (i == request_max) {
	int j;
	PerfReq *old_q = (PerfReq *)free_list, *new_q;
    	request_max = request_max? 2*request_max : 4;
	new_q = (PerfReq *)calloc(1,request_max*sizeof(PerfReq));
	if (!new_q) {
	    return SND_ERR_KERNEL;
	}
	free_list = (PerfReq *)new_q;
	pr = (PerfReq *)perf_q_head;
	j = 0;
	while (pr) {
	    npr = &new_q[j];
	    *npr = *pr;
	    if (pr->next)
		npr->next = &new_q[j+1];
	    if (pr->prev)
		npr->prev = &new_q[j-1];
	    pr = pr->next;
	    j++;
	}
	perf_q_head = &new_q[0];
	perf_q_tail = i? &new_q[i-1] : NULL_REQUEST;
	if (old_q)
	    free(old_q);
    }
    pr = (PerfReq *)(&free_list[i]);
    pr->dev_port = PORT_NULL;
    err = SNDAcquire(0,0,0,-1,0,0,&pr->dev_port,&junk_port);
    if (err) return err;
    pr->id = next_id++;
    pr->mode = mode;
    pr->access = calc_access(mode);
    pr->sound = s;
    
    /* Copy sound header in case sound is freed in endFun */
    pr->sndInfo = *s;
    
    pr->tag = tag;
    pr->priority = priority;
    pr->preempt = preempt;
    pr->beginFun = beginFun;
    pr->endFun = endFun;
    pr->startTime = 0;
    pr->duration = calc_duration(mode, s);
    pr->status = STATUS_WAITING;
    pr->err = SND_ERR_NONE;
    pr->dspOptions = dspOptions;
    pr->recordFD = fd;
    if (!perf_q_tail) {
	pr->prev = pr->next = NULL_REQUEST;
	perf_q_head = perf_q_tail = pr;
    } else {
	pr->prev = (PerfReq *)perf_q_tail;
	pr->next = NULL_REQUEST;
	perf_q_tail->next = pr;
	perf_q_tail = pr;
    }
    request_count++;
    return SND_ERR_NONE;
}

static void dequeue_perf_request(PerfReq *pr)
{
    if (perf_q_head == pr) {
	perf_q_head = pr->next;
	if (perf_q_tail == pr) perf_q_tail = NULL_REQUEST;
	if (pr->next) pr->next->prev = NULL_REQUEST;
    } else {
	pr->prev->next = pr->next;
	if (perf_q_tail == pr)
	    perf_q_tail = pr->prev;
	else
	    pr->next->prev = pr->prev;
    }
    request_count--;
    pr->status = STATUS_FREE;
}

static PerfReq *findRequestForAccess(int access)
{
    PerfReq *pr = (PerfReq *)perf_q_head;
    while (pr)
	if (pr->access & access)
	    return pr;
	else
	    pr = pr->next;
    return NULL_REQUEST;
}

static PerfReq *findRequestForTag(int tag)
{
    PerfReq *pr = (PerfReq *)perf_q_head;
    while (pr)
	if (pr->tag == tag)
	    return pr;
	else
	    pr = pr->next;
    return NULL_REQUEST;
}

static PerfReq *reverseFindRequestForTag(int tag)
{
    PerfReq *pr = (PerfReq *)perf_q_tail;
    while (pr)
	if (pr->tag == tag)
	    return pr;
	else
	    pr = pr->prev;
    return NULL_REQUEST;
}

#if 0
/* NOT CURRENTLY USED */
static PerfReq *findNextRequest(PerfReq *cur_pr, int mode, int status)
{
    PerfReq *pr = cur_pr->next;
    while (pr) {
	if ((pr->status == status) && (pr->mode  == mode))
	    return pr;
	else
	    pr = pr->next;
    }
    return NULL_REQUEST;
}
#endif

/*
 * Performance configuration, initiation and message reply handling
 */

static port_t reply_port = 0;
static msg_header_t *reply_msg = 0;
static int pending_count = 0;

static void terminate_performance(PerfReq *cur);

static int configure_performance(PerfReq *pr)
{
    if (IS_PLAY_MODE(pr->mode))
	return play_configure(pr->mode,pr->sound,pr->dev_port,
			      pr->owner_port,&(pr->perf_port),
			      pr->dspOptions);
    else
	return record_configure(pr->mode,pr->sound,pr->dev_port,
				pr->owner_port,&(pr->perf_port));
}

static int deconfigure_performance(PerfReq *pr)
{
    int err=SND_ERR_NONE, protocol = 0;
    switch (pr->mode) {
      case MODE_DSP_CORE_OUT:
#ifdef DEBUG
	printf("Fix this leak!\n");
#endif
	break;
      default:
	/* free the stream */
	err = snddriver_stream_setup(pr->dev_port, pr->owner_port,
				     0,
				     0,
				     0,
				     0,
				     0,
				     &protocol,
				     &pr->perf_port); 
	break;
    }
    return err;
}

static int perform(PerfReq *pr, port_t reply_port)
{
    int err;
    port_t cmd_port;
    
    /* Turn on the lowpass filter if playing an emphasized sound */
    if (IS_PLAY_MODE(pr->mode) &&
        (pr->mode != MODE_DSP_SSI_OUT) &&
        (pr->mode != MODE_DSP_SSI_COMPRESSED_OUT) &&
        (pr->sound->dataFormat == SND_FORMAT_EMPHASIZED ||
	 pr->sound->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED)) {
	err = SNDGetFilter(&pr->prevFilter);
	if (err) return err;
	err = SNDSetFilter(1);
	if (err) return err;
    }
    
    switch (pr->mode) {
      case MODE_COMPRESSED_OUT:
      case MODE_MULAW_CODEC_OUT:
      case MODE_DIRECT_OUT:
      case MODE_SQUELCH_OUT:
      case MODE_MONO_OUT:
      case MODE_MONO_BYTE_OUT:
      case MODE_CODEC_OUT:
      case MODE_DSP_SSI_OUT:
      case MODE_DSP_SSI_COMPRESSED_OUT:
	return play_samples(pr->tag,pr->sound,pr->perf_port,reply_port);
      case MODE_RESAMPLE_OUT:
	return play_resamples(pr->tag,pr->sound,pr->perf_port,reply_port);
      case MODE_INDIRECT_OUT:
	return play_indirect_samples(pr->tag,pr->sound,
				     pr->perf_port,reply_port);
      case MODE_MULAW_CODEC_IN:
      case MODE_DSP_DATA_IN:
	return record_samples(pr,reply_port,DSP_DMA_SIZE);
      case MODE_DSP_MONO22_IN:
	return record_samples(pr,reply_port,DSP_MONO22_DMA_SIZE);
      case MODE_COMPRESSED_IN:
	return record_samples(pr,reply_port,COMPRESS_DMA_SIZE);
      case MODE_DSP_CORE_OUT:
	err = play_dsp_core(pr->tag,pr->sound,pr->perf_port,reply_port);
	return err;
      case MODE_DSP_COMMANDS_OUT:
	err = snddriver_get_dsp_cmd_port(pr->dev_port, pr->owner_port,
					 &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	return play_dsp_commands(pr->tag,pr->sound,cmd_port,reply_port);
      default:
	break;
    }
    return SND_ERR_NONE;
}

static int flush_performance(PerfReq *pr)
{
    int err, mode = pr->mode;
    port_t cmd_port;
    
    if (mode == MODE_INDIRECT_OUT)
	mode = calc_indirect_mode(pr->sound);
    switch (mode) {
      case MODE_SQUELCH_OUT:
      case MODE_MULAW_CODEC_OUT:
      case MODE_MONO_OUT:
      case MODE_MONO_BYTE_OUT:
      case MODE_CODEC_OUT:
      case MODE_COMPRESSED_OUT:
      case MODE_DSP_COMMANDS_OUT:
      case MODE_RESAMPLE_OUT:
	err = snddriver_get_dsp_cmd_port(pr->dev_port, pr->owner_port,
					 &cmd_port);
	if  (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_set_flags(cmd_port, SNDDRIVER_ICR_HF0, 
				      SNDDRIVER_ICR_HF0, HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	sleep_msec(250); /* wait for dma to drain */
	break;
      case MODE_DSP_CORE_OUT:
	err = snddriver_dsp_set_flags(pr->perf_port,
				      SNDDRIVER_ICR_HF0, 
				      SNDDRIVER_ICR_HF0, HI_PRI);
	if  (err != KERN_SUCCESS) return err;
	sleep_msec(250); /* wait for dma to drain */
      default:
	break;
    }
    
    /* Note: you cannot look at pr->sound here because it may have
       been SNDFree()ed by endFun */
    
    /* Return lowpass filter to previous state if finished playing an emphasized sound */
    if (IS_PLAY_MODE(pr->mode) &&
        (pr->mode != MODE_DSP_SSI_OUT) &&
        (pr->mode != MODE_DSP_SSI_COMPRESSED_OUT) &&
        (pr->sndInfo.dataFormat == SND_FORMAT_EMPHASIZED ||
	 pr->sndInfo.dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED)) {
	err = SNDSetFilter(pr->prevFilter);
	if (err) return err;
    }
    
    return SND_ERR_NONE;
}

static void recover_pending_requests(PerfReq *pr, int mode)
{
    while (pr) {
	if (pr->status == STATUS_PENDING && pr->mode == mode)
	    pr->status = STATUS_WAITING;
	pr = pr->next;
    }
}

static int access_in_use(PerfReq *pr, int required_access, port_t dev_port)
{
    while (pr) {
	if (pr->access & required_access && pr->dev_port == dev_port)
	    return 1;
	pr = pr->prev;
    }
    return 0;
}

static int initiate_performance()
{
    PerfReq *pr = (PerfReq *)perf_q_head;
    int required_access, err;
    port_t required_port;
    
    while (1) {
	while (pr && pr->status != STATUS_WAITING)
	    pr = pr->next;
	if (!pr) return SND_ERR_NONE; /* nothing to do */
	
	required_access = pr->access;
	required_port = pr->dev_port;
	if (access_in_use(pr->prev,required_access,required_port)) {
	    PerfReq *prev = pr->prev;
	    while (prev && ((required_access != prev->access) ||
			    (required_port != prev->dev_port)))
		prev = prev->prev;
	    if (!prev || !modeOptimizable(pr->mode, prev->mode, 
					  pr->sound, prev->sound) ||
		(pending_count*3 > REPLY_BACKLOG))
		return SND_ERR_NONE;
	    pr->perf_port = prev->perf_port;
	    pr->owner_port = prev->owner_port;
	    pr->status = STATUS_PENDING;
	    pr->startTime = msec_timestamp();
	    err = perform(pr, reply_port);
	    if (err) {
		pr->err = err;
		pr->status = STATUS_ACTIVE;
#if DEBUG
		printf("err %d in initiate_performance\n", err);
#endif
		return err;
	    } else
		pending_count++;
	} else {
	    err = SNDAcquire(required_access, pr->priority, pr->preempt,
			     -1, (SNDNegotiationFun)0, (void *)0,
			     &pr->dev_port, &pr->owner_port);
	    if (err != SND_ERR_NONE) return err;
	    pr->status = STATUS_PENDING;
	    pr->startTime = msec_timestamp();
	    err = configure_performance(pr);
	    if (err != KERN_SUCCESS) {
		pr->err = err = SND_ERR_CANNOT_CONFIGURE;
		pr->status = STATUS_ACTIVE;
		return err;
	    } else {
		err = perform(pr, reply_port);
		if (err) {
		    deconfigure_performance(pr);
		    pr->err = err;
		    pr->status = STATUS_ACTIVE;
		    return err;
		} else
		    pending_count++;
	    }
	}
    }
}

static int can_preempt(int mode, int priority)
{
    PerfReq *pr = (PerfReq *)perf_q_head;
    int required_access = calc_access(mode);
    int pri = priority + 1;
    while (pr) {
	if ((pr->access & required_access) && (pr->priority > pri))
	    return 0;
	pr = pr->next;
    }
    return 1;
}

static void preempt_requests(int mode)
{
    SNDSoundStruct *s;
    SNDNotificationFun fun;
    PerfReq *pr = (PerfReq *)perf_q_head;
    int err, tag, required_access = calc_access(mode);
    while (pr) {
	if (pr->access & required_access) {
	    if (pr->status == STATUS_ACTIVE) {
		if (pr->duration > MIN_PREEMPT_DUR) {
		    recover_pending_requests(pr->next,pr->mode);
		    pr->err = SND_ERR_ABORTED;
		    err = deconfigure_performance(pr);
		    err = SNDRelease(pr->access, pr->dev_port, pr->owner_port);
		    pending_count = 0;
		    fun = pr->endFun;
		    s = pr->sound;
		    tag = pr->tag;
		    dequeue_perf_request(pr);
		    mutex_unlock(q_lock);
		    if (fun)
			(*fun)(s,tag,SND_ERR_ABORTED);
		    mutex_lock(q_lock);
		}
	    } else {
		fun = pr->endFun;
		s = pr->sound;
		tag = pr->tag;
		if (pr->status == STATUS_PENDING) {
		    pending_count--;
		    if (!pending_count)
			err = deconfigure_performance(pr);
		}
		dequeue_perf_request(pr);
		mutex_unlock(q_lock);
		if (fun)
		    (*fun)(s,tag,SND_ERR_ABORTED);
		mutex_lock(q_lock);
	    }
	}
	pr = pr->next;
    }
}    

static int doNotStart = 0;

static int start_performance(int mode, SNDSoundStruct *s, int tag,
			     int priority, int preempt, 
			     SNDNotificationFun beginFun,
			     SNDNotificationFun endFun,
			     int dspOptions, int fd)
{
    int err;
    mutex_lock(q_lock);
    if (findRequestForTag(tag)) {
	mutex_unlock(q_lock);
	return SND_ERR_BAD_TAG;
    }
    if (preempt) {
	if (can_preempt(mode,priority))
	    preempt_requests(mode);
	else {
	    mutex_unlock(q_lock);
	    return SND_ERR_CANNOT_PLAY;
	}
    }
    err = enqueue_perf_request(mode,s,tag,priority,preempt,beginFun,endFun,dspOptions,fd);
    if (!err && !doNotStart) {
	err = initiate_performance();
	if (err) {
	    PerfReq *pr = findRequestForTag(tag);
	    if (pr) dequeue_perf_request(pr);
	}
    }
    mutex_unlock(q_lock);
    return err;
}

static void terminate_performance(PerfReq *cur)
{
    PerfReq *next = NULL_REQUEST;
    int err = SND_ERR_NONE;
    
    if (cur->status == STATUS_WAITING)
	dequeue_perf_request(cur);
    else {
	if (!cur->err) {
	    next = cur->next;
	    while (next) {
		if (next->dev_port == cur->dev_port &&
		    next->mode == cur->mode &&
		    (next->status == STATUS_PENDING ||
		     next->status == STATUS_WAITING) )
		    break;
		else
		    next = next->next;
	    }
	}
	
	/* Note: you cannot look at cur->sound here because it may have
	   been SNDFree()ed by endFun */
	
	/* Also, there must be at least 2 pending requests to optimize */
	
	if (!next || (pending_count == 1) ||
	    !modeOptimizable(next->mode, cur->mode, next->sound, &cur->sndInfo)) {
	    if (!cur->err)
		flush_performance(cur);
	    err = deconfigure_performance(cur);
	    err = SNDRelease(cur->access, cur->dev_port, cur->owner_port);
	    pending_count = 1;
	}
	dequeue_perf_request(cur);
	pending_count--;
	if (perf_q_head)
	    err = initiate_performance();
    }
#ifdef DEBUG
    if (err)
        printf("Error %d in terminate_performance\n", err);
#endif
}

static void stop_performance(int tag, int err)
{
    PerfReq *pr;
    SNDNotificationFun fun;
    SNDSoundStruct *s;
    
    mutex_lock(q_lock);
    pr = findRequestForTag(tag);
    if (!pr) {
	mutex_unlock(q_lock);
	return;
    }
    switch (pr->mode) {
      case MODE_MULAW_CODEC_IN:
      case MODE_DSP_DATA_IN:
      case MODE_DSP_MONO22_IN:
      case MODE_COMPRESSED_IN:
	pr->status = STATUS_ABORTED;
	pr->err = SND_ERR_ABORTED;
	if (err != SND_ERR_TIMEOUT) {
	    while (pr && pr->status != STATUS_FREE) {
		condition_wait(q_changed,q_lock);
		pr = findRequestForTag(tag);
	    }
	    mutex_unlock(q_lock);
	} else {
	    recover_pending_requests(pr->next,pr->mode);
	    terminate_performance(pr);
	    fun = pr->endFun;
	    s = pr->sound;
	    mutex_unlock(q_lock);
	    if (fun)
		(*fun)(s,tag,err);
	    condition_signal(q_changed);
	}
	break;
      case MODE_DSP_COMMANDS_OUT:
	s_kill_dsp_commands_thread = TRUE;
	/* fall through */
      default:
	pr->err = err;
	recover_pending_requests(pr->next,pr->mode);
	/*
	 * Force soundout to stop.  This is necessary if access has
	 * been reserved because SNDRelease() will not deallocate owner port
	 * (which is the usual way of stopping soundout).
	 */
	if (pr->access & SND_ACCESS_OUT)
	    SNDReset(pr->access, pr->dev_port, pr->owner_port);
	terminate_performance(pr);
	fun = pr->endFun;
	s = pr->sound;
	mutex_unlock(q_lock);
	if (fun)
	    (*fun)(s,tag,err);
	condition_signal(q_changed);
	break;
    }
}

static void performance_ended(void *junk, int tag);

static void check_performance()
    /* take action on one thing maximum per call */
{
    static int start_delay = 10;
    SNDNotificationFun fun;
    SNDSoundStruct *s;
    int err, now = msec_timestamp(), tag=0;
    int reply_timeout;
    PerfReq *pr;
    int nsamples = 0;

    mutex_lock(q_lock);
    pr = (PerfReq *)perf_q_head;
    while (pr) {
	if (pr->status == STATUS_ACTIVE) {
	    start_delay = 10;
	    if (pr->mode == MODE_DSP_COMMANDS_OUT)
	        reply_timeout = DSP_COMMANDS_REPLY_TIMEOUT;
	    else if (pr->mode == MODE_COMPRESSED_IN)
		reply_timeout = COMPRESSED_IN_REPLY_TIMEOUT;
	    else
	        reply_timeout = REPLY_TIMEOUT;
	    if ((now - pr->startTime - reply_timeout) > pr->duration) {
		tag = pr->tag;
#ifdef DEBUG
		printf("Active request timed out : %d\n",tag);
		printf(" ...duration = %d, start time = %d, now = %d\n",
		       pr->duration, pr->startTime, now );
		/*printf("Active request termination disabled\n");
		return;*/
#endif
		/* NOTE: this call actually returns the number of bytes
		   processed, not the number of samples */
		err = snddriver_stream_nsamples(pr->perf_port, &nsamples);
#ifdef DEBUG	
		if (err)
		    printf("snddriver_stream_nsamples returned error %d\n", err);
#endif
		/* This works around dsp/driver endgame problems */
		if (!err && (nsamples >= pr->sound->dataSize)) {
#ifdef DEBUG
		    printf("performence ended normally because nsamples > dataSize\n");
#endif
		    mutex_unlock(q_lock);
		    performance_ended(NULL, tag);
		    return;
		}
		pr->err = SND_ERR_TIMEOUT;
		pr->status = STATUS_ABORTED;
		mutex_unlock(q_lock);
		stop_performance(tag, SND_ERR_TIMEOUT);
		return;
	    } else if (IS_RECORD_MODE(pr->mode)) {
		err = snddriver_stream_control(pr->perf_port,pr->tag,
					       SNDDRIVER_AWAIT_STREAM);
	    }
	} else if (pr->status == STATUS_PENDING && pr == perf_q_head) {
	    if (!start_delay--) {
		start_delay = 10;
		tag = pr->tag;
#ifdef DEBUG
		printf("Pending request timed out : %d\n",tag);
		/*printf("Pending request termination disabled\n");
		return;*/
#endif
		pr->err = SND_ERR_TIMEOUT;
		mutex_unlock(q_lock);
		stop_performance(tag, SND_ERR_TIMEOUT);
		return;
	    }
	} else if (pr->status == STATUS_ABORTED) {
	    start_delay = 10;
	    fun = pr->endFun;
	    err = pr->err;
	    s = pr->sound;
#ifdef DEBUG
	    printf("Aborted request removed : %d\n",tag);
#endif
	    recover_pending_requests(pr->next,pr->mode);
	    terminate_performance(pr);
	    mutex_unlock(q_lock);
	    if (fun)
		(*fun)(s,tag,err);
	    condition_signal(q_changed);
	    return;
	}
	pr = pr->next;
    }
    mutex_unlock(q_lock);
}


static void performance_started(void *junk, int tag)
{
    PerfReq *pr;
    int delay_of_first_buffer;
    int err, now;
    SNDNotificationFun fun;
    SNDSoundStruct *s;
    int dmasize = vm_page_size / 2;
    
    mutex_lock(q_lock);
    pr = findRequestForTag(tag);
    if (!pr || pr->status == STATUS_ABORTED) {
	mutex_unlock(q_lock);
	return;
    }
    delay_of_first_buffer = calc_ms_from_sample_count(pr->sound,dmasize);
    pr->status = STATUS_ACTIVE;
    now = msec_timestamp();
    /*
     * This was used at one time:
     * pr->startTime = now - delay_of_first_buffer;
     */
    pr->startTime = now;
#ifdef DEBUG
    /*printf("perf %d started at %d\n",tag,now);*/
#endif
    if (IS_RECORD_MODE(pr->mode)) {
	err = snddriver_stream_control(pr->perf_port,pr->tag,
				       SNDDRIVER_AWAIT_STREAM);
    }
    fun = pr->beginFun;
    err = pr->err;
    s = pr->sound;
    mutex_unlock(q_lock);
    if (fun)
	(*fun)(s,tag,err);
}

static void performance_ended(void *junk, int tag)
{
    PerfReq *pr;
    int err;
    SNDNotificationFun fun;
    SNDSoundStruct *s;
    
    mutex_lock(q_lock);
    pr = findRequestForTag(tag);
    if (!pr) {
	mutex_unlock(q_lock);
	return;
    }
#ifdef DEBUG
    /*printf("perf %d ended at %d (started at %d)\n",tag,msec_timestamp(),pr->startTime);*/
#endif
    if (pr->recordFD >= 0) {
	lseek(pr->recordFD, 0L, 0);
	pr->err = SNDWriteHeader(pr->recordFD, pr->sound);
    }
    fun = pr->endFun;
    err = pr->err;
    s = pr->sound;
    doNotStart=1;
    mutex_unlock(q_lock);
    if (fun)
	(*fun)(s,tag,err);
    mutex_lock(q_lock);
    pr = findRequestForTag(tag);
    terminate_performance(pr);
    doNotStart=0;
    mutex_unlock(q_lock);
    condition_signal(q_changed);
}

/* Parallel micro-algorithm codes */
enum {
    NULL_ENCODE,
    XOR_ENCODE,
    D1_ENCODE,
    D2_ENCODE,
    D3_ENCODE,
    D4_ENCODE,
    D3_11_ENCODE,
    D3_22_ENCODE,
    D4_222_ENCODE,
    D4_343_ENCODE,
    D4_101_ENCODE,
    NUM_ENCODES
    };

static int bytesInBlock(int code, int numBits, int encodeLength)
    /* Returns the number of bytes required to encode a block using numBits per sample. */
{
    int tokenbytes = 0, packshorts = 0;
    
    switch (code) {
      case NULL_ENCODE: tokenbytes = 0; packshorts = encodeLength; break;
      case XOR_ENCODE: tokenbytes = 2; packshorts = encodeLength-1; break;
      case D1_ENCODE: tokenbytes = 2; packshorts = encodeLength-1; break;
      case D2_ENCODE: tokenbytes = 4; packshorts = encodeLength-2; break;
      case D3_ENCODE: tokenbytes = 6; packshorts = encodeLength-3; break;
      case D4_ENCODE: tokenbytes = 8; packshorts = encodeLength-4; break;
      case D3_11_ENCODE: tokenbytes = 6; packshorts = encodeLength-3; break;
      case D3_22_ENCODE: tokenbytes = 6; packshorts = encodeLength-3; break;
      case D4_222_ENCODE: tokenbytes = 8; packshorts = encodeLength-4; break;
      case D4_343_ENCODE: tokenbytes = 8; packshorts = encodeLength-4; break;
      case D4_101_ENCODE: tokenbytes = 8; packshorts = encodeLength-4; break;
    }
    return(tokenbytes + (numBits*packshorts+7)/8);
}

static void performance_read_data(void *junk, int tag, void *p, int i)
{
    char *ptr;
    int remaining_bytes, size, err, done = 0;
    PerfReq *pr;
    compressionSubHeader *subheader = NULL;
    int count, code, numBits;
    int numSamples = 0;
#ifdef DEBUG
    static int curTag = 0;
#endif
    
    mutex_lock(q_lock);
    pr = findRequestForTag(tag);
    if (pr && pr->status) {
	switch (pr->mode) {
	  case MODE_COMPRESSED_IN:
	    subheader = (compressionSubHeader *)((char *)pr->sound
						 + pr->sound->dataLocation);
	    numSamples = subheader->originalSize/2;
	    /* Fall through */
	  case MODE_MULAW_CODEC_IN:
	  case MODE_DSP_DATA_IN:
	  case MODE_DSP_MONO22_IN:
#ifdef DEBUG
	    if (curTag && (curTag != tag)) {
		printf("Lost data for tag %d\n", curTag);
		/*pause();*/
	    }
	    curTag = tag;
#endif
	    ptr = (char *)pr->work_ptr;
	    remaining_bytes = pr->work_count;
	    size = (i > remaining_bytes)? remaining_bytes : i;
	    if (pr->recordFD >= 0) {
		if (write(pr->recordFD, (char *)p, size) != size)
		    pr->err = SND_ERR_CANNOT_WRITE;
	    } else {
		memmove(ptr,p,size);
		ptr += size;
	    }
	    remaining_bytes -= size;
	    pr->work_ptr = (void *)ptr;
	    pr->work_count = remaining_bytes;
	    pr->sound->dataSize += size;
	    done = (pr->status == STATUS_ABORTED || !remaining_bytes);
	    
	    /* FIXME: for compressed to work with recording to file, you have
	       keep state about where you are rather that using pointers.
	       This would work fine for the non-compressed case too. */

	    if (pr->mode == MODE_COMPRESSED_IN) {
		/*
		 * Note: dataSize gets truncated leaving a hole of unused
		 * but allocated memory in the sound.  This hole of course goes
		 * away if you write the compressed sound to a file.
		 */
		while (ptr > pr->work_block_ptr) {
		    if (((pr->work_block_count-1) * subheader->encodeLength) >=
			numSamples) {
			pr->sound->dataSize -= ptr - pr->work_block_ptr;
			if (pr->sound->dataSize > subheader->originalSize) {
			    pr->sound->dataSize = subheader->originalSize;
#ifdef DEBUG
			    printf("Sound could not be compressed\n");
#endif
			}
			done = TRUE;
			break;
		    }
		    code = *pr->work_block_ptr++;
		    numBits = *pr->work_block_ptr++;
		    if ((unsigned)code >= NUM_ENCODES || (unsigned)numBits > 16) {
#ifdef DEBUG
			printf("BOGUS!! block=%d, code=%d, numBits=%d\n",
			       pr->work_block_count, code, numBits);
#endif
			done = TRUE;
			break;
		    }
		    
		    count = bytesInBlock(code, numBits, subheader->encodeLength);
		    if (count & 1)
			count++;	/* pad to short */
		    pr->work_block_ptr += count;
		    pr->work_block_count++;
		}
	    }
	    err = snddriver_stream_control(pr->perf_port,pr->tag,
					   SNDDRIVER_AWAIT_STREAM);
	    break;
	  default:
	    break;
	}    
    }
    err = vm_deallocate(task_self(),(pointer_t)p,i);
    mutex_unlock(q_lock);
    if (done) {
#ifdef DEBUG
	curTag = 0;
#endif
	performance_ended(junk,tag);
    }
}

static void performance_condition_true(void *junk, u_int mask, u_int bits,
				       u_int value)
{
    PerfReq *pr;
    int tag;
    
    mutex_lock(q_lock);
    pr = findRequestForAccess(SND_ACCESS_DSP);
    if (pr) {
	tag = pr->tag;
	switch (pr->status) {
	  case STATUS_ACTIVE:
	    mutex_unlock(q_lock);
	    performance_ended(junk,tag);
	    return;
	  case STATUS_PENDING:
	    mutex_unlock(q_lock);
	    performance_started(junk,tag);
	    return;
	}
    }
    mutex_unlock(q_lock);
}

#define	OPCODE_MASK	0xf0000
#define	OPCODE_IDLE	0xf0000
#define OPCODE_PEEK0	0xd0000

static void performance_dsp_message(void *junk, int *data, int size)
{
    PerfReq *pr;
    int tag, i, opcode, err;
    int gotIdle = FALSE, gotPeek0 = FALSE;
    port_t cmd_port;
    
    mutex_lock(q_lock);
    pr = findRequestForAccess(SND_ACCESS_DSP);
    if (pr) {
	tag = pr->tag;
	for (i = 0; i < size; i++) {
	    opcode = data[i] & OPCODE_MASK;
	    if (opcode == OPCODE_IDLE)
	        gotIdle = TRUE;
	    else if (opcode == OPCODE_PEEK0)
	        gotPeek0 = TRUE;
	}
	err = snddriver_get_dsp_cmd_port(pr->dev_port, pr->owner_port,
				         &cmd_port);
#ifdef DEBUG
	if (err) printf("snddriver_get_dsp_cmd_port returned %d\n", err);
#endif
	switch (pr->status) {
	  case STATUS_ACTIVE:
	    mutex_unlock(q_lock);
	    if (gotPeek0)
		performance_ended(junk,tag);
	    else {
		err = snddriver_dspcmd_req_msg(cmd_port, reply_port);
#ifdef DEBUG
		if (err) printf("snddriver_dspcmd_req_msg returned %d\n", err);
#endif
	    }
	    return;
	  case STATUS_PENDING:
	    mutex_unlock(q_lock);
	    if (gotIdle)
		performance_started(junk,tag);
	    err = snddriver_dspcmd_req_msg(cmd_port, reply_port);
#ifdef DEBUG
	    if (err) printf("snddriver_dspcmd_req_msg returned %d\n", err);
#endif
	    return;
	}
    }
    mutex_unlock(q_lock);
}

static any_t perform_reply_thread(any_t args)
{
    snddriver_handlers_t handlers = {
	(void *)0, 0, 
	performance_started,
	performance_ended,
	0, 0, 0, 0, 
	performance_read_data,
	performance_condition_true,
	performance_dsp_message,
	0 };
    int err;
    while (1) {
	reply_msg->msg_size = MSG_SIZE_MAX;
	reply_msg->msg_local_port = reply_port;
	err = msg_receive(reply_msg, RCV_TIMEOUT, REPLY_TIMEOUT);
	if (err == KERN_SUCCESS) {
	    err = snddriver_reply_handler(reply_msg,&handlers);
	} else if (err == RCV_TIMED_OUT) {
	    check_performance();
	} else {
#ifdef DEBUG
	    printf("perform_reply_thread msg_receive error : %d\n",err);
#endif
	}
    }
    return NULL;
}

static int initialize()
{
    int err;
    static int initialized=0;
    if (initialized) return 0;
    initialized = 1;
    q_lock = mutex_alloc();
    mutex_init(q_lock);
    q_changed = condition_alloc();
    condition_init(q_changed);
    err = port_allocate(task_self(),&reply_port);
    if (err != KERN_SUCCESS)  return SND_ERR_KERNEL;
    err = port_set_backlog(task_self(),reply_port,REPLY_BACKLOG);
    if (err != KERN_SUCCESS)  return SND_ERR_KERNEL;
    reply_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
    if (!reply_msg)  return SND_ERR_KERNEL;
    cthread_fork(perform_reply_thread, (any_t)NULL);
    return SND_ERR_NONE;
}

/*
 * Exported routines
 */

int SNDStartPlaying(SNDSoundStruct *s, int tag, int priority, int preempt,
		    SNDNotificationFun beginFun, SNDNotificationFun endFun)
{
    int err, mode;
    
    if (err = initialize())
	return err;
    mode = calc_play_mode(s);
    if (!mode)
	return SND_ERR_CANNOT_PLAY;
    err = start_performance(mode,s,tag,priority,preempt,beginFun,endFun,0,-1);
    return err;
}

int SNDStartPlayingDSP(SNDSoundStruct *s, int tag, int priority, int preempt,
		       SNDNotificationFun beginFun, SNDNotificationFun endFun,
		       int playOptions)
{
    int err, mode;
    
    if (err = initialize())
	return err;
    mode = calc_dsp_play_mode(s);
    if (!mode)
	return SND_ERR_CANNOT_PLAY;
    err = start_performance(mode,s,tag,priority,preempt,beginFun,endFun,playOptions,-1);
    return err;
}

int SNDStop(int tag)
{
    int err = initialize();
    
    if (err) return err;
    if (!tag)
	return SND_ERR_BAD_TAG; /* FIXME: zero tag should stop everything? */
    stop_performance(tag,SND_ERR_ABORTED);
    return SND_ERR_NONE;
}

int SNDWait(int tag)
{
    int err = initialize();
    
    if (err) return err;
    mutex_lock(q_lock);
    if (tag) {
	PerfReq *pr = reverseFindRequestForTag(tag);
	if (pr)
	    while (pr && pr->status != STATUS_FREE) {
		condition_wait(q_changed,q_lock);
		pr = reverseFindRequestForTag(tag);
	    }
    } else {
	if (perf_q_head && request_count)
	    while (perf_q_head)
		condition_wait(q_changed,q_lock);
    }
    mutex_unlock(q_lock);
    return err;
}

int SNDStartRecording(SNDSoundStruct *s, int tag, int priority, int preempt,
		      SNDNotificationFun beginFun, SNDNotificationFun endFun)
{
    int err, mode;
    
    if (err = initialize())
	return err;
    mode = calc_record_mode(s);
    if (!mode)
	return SND_ERR_CANNOT_RECORD;
    err = start_performance(mode,s,tag,priority,preempt,beginFun,endFun,0,-1);
    return err;
}

int SNDStartRecordingFile(char *fileName, SNDSoundStruct *s,
			  int tag, int priority, int preempt,
			  SNDNotificationFun beginFun, SNDNotificationFun endFun)
{
    int err, mode, fd;
    
    if (err = initialize())
	return err;

    /* NOTE: When recording from the dsp the soundfile will retain the
       SND_FORMAT_DSP_DATA_16 format.  The format must be changed to
       SND_FORMAT_LINEAR_16 to be played back. */

    /* NOTE: s->dataSize must specify the number of bytes to record, but the
       sound does not have to have this memory allocated.  If recording compressed,
       enough data must be allocated for the compression subheader.  In fact, you
       should call SNDAlloc() to get the default compression parameters.
       (This is bogus since we don't export the subheader size.) */

    /* Actually, you should probably do the normal SNAlloc() with the dataSize you
       want and let vm get allocated - there is almost no overhead if you don't
       write to it.  You can then SNDFree() the sound after recording is done. */

    mode = calc_record_mode(s);
    if (!mode)
	return SND_ERR_CANNOT_RECORD;
    if ((fd = creat(fileName, 0644)) == -1)
	return SND_ERR_BAD_FILENAME;
    if (err = SNDWriteHeader(fd, s))
	return err;
    err = start_performance(mode,s,tag,priority,preempt,beginFun,endFun,0,fd);
    return err;
}

int SNDSamplesProcessed(int tag)
{
    int samples = -1;
    int err = initialize();
    
    if (err) return err;
    mutex_lock(q_lock);
    if (tag) {
	PerfReq *pr = findRequestForTag(tag);
	if (pr) {
	    if (IS_RECORD_MODE(pr->mode))
		samples = calc_record_nsamples(pr);
	    else
		samples = calc_play_nsamples(pr);
	}
    }
    mutex_unlock(q_lock);
    return samples;
}

int SNDModifyPriority(int tag, int new_priority)
{
    int err = initialize();
    
    if (err) return err;
    err = SND_ERR_BAD_TAG;
    mutex_lock(q_lock);
    if (tag) {
	PerfReq *pr = findRequestForTag(tag);
	if (pr) {
	    pr->priority = new_priority;
	    err = SND_ERR_NONE;
	}
    }
    mutex_unlock(q_lock);
    return err;
}

int SNDSetVolume(int left, int right)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_set_volume(dev_port,left,right);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    else
	return SND_ERR_NONE;
}

int SNDGetVolume(int *left, int *right)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_get_volume(dev_port,left,right);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    else
	return SND_ERR_NONE;
}

int SNDSetMute(int speakerOn)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    boolean_t speaker, lowpass, zerofill;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_get_device_parms(dev_port,&speaker,&lowpass,&zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    if ((speakerOn && speaker) || (!speakerOn && !speaker))
	return SND_ERR_NONE;
    err = snddriver_set_device_parms(dev_port,!speaker, lowpass, zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    return SND_ERR_NONE;
}

int SNDGetMute(int *speakerOn)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    boolean_t speaker, lowpass, zerofill;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_get_device_parms(dev_port,&speaker,&lowpass,&zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    *speakerOn = speaker? 1:0;
    return SND_ERR_NONE;
}

int SNDSetCompressionOptions(SNDSoundStruct *s, int bitFaithful, int dropBits)
{
    compressionSubHeader *subheader = NULL;
    
    if (!s || (s->magic != SND_MAGIC))
	return SND_ERR_NOT_SOUND;
    if (s->dataFormat != SND_FORMAT_COMPRESSED &&
        s->dataFormat != SND_FORMAT_COMPRESSED_EMPHASIZED)
	return SND_ERR_BAD_FORMAT;
    if (s->dataSize < sizeof(compressionSubHeader))
        return SND_ERR_BAD_SIZE;
    subheader = (compressionSubHeader *)((char *)s + s->dataLocation);
    subheader->method = bitFaithful ? 1 : 0;
    if (dropBits < 4)
        dropBits = 4;
    else if (dropBits > 8)
        dropBits = 8;
    subheader->numDropped = dropBits;
    return SND_ERR_NONE;
}

int SNDGetCompressionOptions(SNDSoundStruct *s, int *bitFaithful, int *dropBits)
{
    compressionSubHeader *subheader = NULL;
    
    if (!s || (s->magic != SND_MAGIC))
	return SND_ERR_NOT_SOUND;
    if (s->dataFormat != SND_FORMAT_COMPRESSED &&
        s->dataFormat != SND_FORMAT_COMPRESSED_EMPHASIZED)
	return SND_ERR_BAD_FORMAT;
    if (s->dataSize < sizeof(compressionSubHeader))
        return SND_ERR_BAD_SIZE;
    subheader = (compressionSubHeader *)((char *)s + s->dataLocation);
    *bitFaithful = subheader->method;
    *dropBits = subheader->numDropped;
    return SND_ERR_NONE;
}

int SNDUpdateDSPParameter(int value)
{
    /* FIXME */
    return SND_ERR_NOT_IMPLEMENTED;
}

int SNDSetFilter(int filterOn)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    boolean_t speaker, lowpass, zerofill;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_get_device_parms(dev_port,&speaker,&lowpass,&zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    if ((filterOn && lowpass) || (!filterOn && !lowpass))
	return SND_ERR_NONE;
    err = snddriver_set_device_parms(dev_port, speaker, !lowpass, zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    return SND_ERR_NONE;
}

int SNDGetFilter(int *filterOn)
{
    port_t dev_port=PORT_NULL, owner_port;
    int err;
    boolean_t speaker, lowpass, zerofill;
    if (err = initialize())
	return err;
    err = SNDAcquire(0,0,0,-1,0,0,&dev_port,&owner_port);
    if (err) return err;
    err = snddriver_get_device_parms(dev_port,&speaker,&lowpass,&zerofill);
    if (err)
	return (err == RCV_TIMED_OUT || err == SEND_TIMED_OUT)? 
	    SND_ERR_TIMEOUT : SND_ERR_KERNEL;
    *filterOn = lowpass ? 1 : 0;
    return SND_ERR_NONE;
}
