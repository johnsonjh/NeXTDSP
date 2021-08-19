#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	convertsound.h
 *	Written by Dana Massie and Lee Boynton
 *	Copyright 1988 NeXT, Inc.
 *
 *	Modification History:
 *	04/06/90/mtm	Added SNDCompressSound().
 *	04/08/90/mtm	Comment out "normal_exit:" and "calcHeaderSize()".
 *	07/18/90/mtm	Get rid of static array in findDSPCore().
 *	07/25/90/mtm	Implement SNDCompressSound().
 *	08/08/90/mtm	Add sample rate conversion to SNDConvertSound().
 *	10/08/90/mtm	Add downby2.c back in (bug #10407).
 */

#import <sys/types.h>
#import <stdlib.h>
#import <string.h>
#import "sounddriver.h"
#import "accesssound.h"
#import "utilsound.h"
#import "filesound.h"
#import "convertsound.h"

#define PAGESIZE ((int)vm_page_size)

/*
 * Mulaw to 16 bit linear conversion table
 */
#define	MULAWTABLEN	256
static const short muLaw[MULAWTABLEN] = {
 0x8284, 0x8684, 0x8a84, 0x8e84, 0x9284, 0x9684, 0x9a84, 0x9e84, 
 0xa284, 0xa684, 0xaa84, 0xae84, 0xb284, 0xb684, 0xba84, 0xbe84, 
 0xc184, 0xc384, 0xc584, 0xc784, 0xc984, 0xcb84, 0xcd84, 0xcf84, 
 0xd184, 0xd384, 0xd584, 0xd784, 0xd984, 0xdb84, 0xdd84, 0xdf84, 
 0xe104, 0xe204, 0xe304, 0xe404, 0xe504, 0xe604, 0xe704, 0xe804, 
 0xe904, 0xea04, 0xeb04, 0xec04, 0xed04, 0xee04, 0xef04, 0xf004, 
 0xf0c4, 0xf144, 0xf1c4, 0xf244, 0xf2c4, 0xf344, 0xf3c4, 0xf444, 
 0xf4c4, 0xf544, 0xf5c4, 0xf644, 0xf6c4, 0xf744, 0xf7c4, 0xf844, 
 0xf8a4, 0xf8e4, 0xf924, 0xf964, 0xf9a4, 0xf9e4, 0xfa24, 0xfa64, 
 0xfaa4, 0xfae4, 0xfb24, 0xfb64, 0xfba4, 0xfbe4, 0xfc24, 0xfc64, 
 0xfc94, 0xfcb4, 0xfcd4, 0xfcf4, 0xfd14, 0xfd34, 0xfd54, 0xfd74, 
 0xfd94, 0xfdb4, 0xfdd4, 0xfdf4, 0xfe14, 0xfe34, 0xfe54, 0xfe74, 
 0xfe8c, 0xfe9c, 0xfeac, 0xfebc, 0xfecc, 0xfedc, 0xfeec, 0xfefc, 
 0xff0c, 0xff1c, 0xff2c, 0xff3c, 0xff4c, 0xff5c, 0xff6c, 0xff7c, 
 0xff88, 0xff90, 0xff98, 0xffa0, 0xffa8, 0xffb0, 0xffb8, 0xffc0, 
 0xffc8, 0xffd0, 0xffd8, 0xffe0, 0xffe8, 0xfff0, 0xfff8, 0x0, 
 0x7d7c, 0x797c, 0x757c, 0x717c, 0x6d7c, 0x697c, 0x657c, 0x617c, 
 0x5d7c, 0x597c, 0x557c, 0x517c, 0x4d7c, 0x497c, 0x457c, 0x417c, 
 0x3e7c, 0x3c7c, 0x3a7c, 0x387c, 0x367c, 0x347c, 0x327c, 0x307c, 
 0x2e7c, 0x2c7c, 0x2a7c, 0x287c, 0x267c, 0x247c, 0x227c, 0x207c, 
 0x1efc, 0x1dfc, 0x1cfc, 0x1bfc, 0x1afc, 0x19fc, 0x18fc, 0x17fc, 
 0x16fc, 0x15fc, 0x14fc, 0x13fc, 0x12fc, 0x11fc, 0x10fc, 0xffc, 
 0xf3c, 0xebc, 0xe3c, 0xdbc, 0xd3c, 0xcbc, 0xc3c, 0xbbc, 
 0xb3c, 0xabc, 0xa3c, 0x9bc, 0x93c, 0x8bc, 0x83c, 0x7bc, 
 0x75c, 0x71c, 0x6dc, 0x69c, 0x65c, 0x61c, 0x5dc, 0x59c, 
 0x55c, 0x51c, 0x4dc, 0x49c, 0x45c, 0x41c, 0x3dc, 0x39c, 
 0x36c, 0x34c, 0x32c, 0x30c, 0x2ec, 0x2cc, 0x2ac, 0x28c, 
 0x26c, 0x24c, 0x22c, 0x20c, 0x1ec, 0x1cc, 0x1ac, 0x18c, 
 0x174, 0x164, 0x154, 0x144, 0x134, 0x124, 0x114, 0x104, 
 0xf4, 0xe4, 0xd4, 0xc4, 0xb4, 0xa4, 0x94, 0x84, 
 0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40, 
 0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x8, 0x0
};

/*
 * 16 bit linear to mulaw conversion
 */

#define IMULAWOFFSET	8192
#define	IMULAWTABLEN	16384
#define IMULAWMASK	16383
static unsigned char *iMuLaw = 0;

 struct mu {
    short mu,
	linear;
 };

static int compar(p1, p2)
    struct mu **p1, **p2;
{
    if ((*p1)->linear > (*p2)->linear) return 1;
    else if ((*p1)->linear == (*p2)->linear) return 0;
    else return -1;
}

static void makeIMuLawTab()
{
    int i,j,k, d1, d2;
    struct mu *mutab[256], mus[256];

    iMuLaw = (unsigned char *) malloc(IMULAWTABLEN * sizeof(unsigned char));

    for (i = 0; i < 256; ++i)
    {
	mutab[i] = &mus[i];
	mus[i].mu = i;
	mus[i].linear = muLaw[i] >> 2;
    }
    qsort(mutab, 256, sizeof(struct mu *), compar);

    for (i = 0, j = 0, k = -8192; i < 16384; ++i, ++k)
    {
	if (j < 255)
	{
	    d1 = k - mutab[j]->linear;
	    d2 = mutab[j+1]->linear - k;
	    if (d1 > 0 && d1 > d2)
		++j;
	}
	iMuLaw[i] = mutab[j]->mu;
    }
}

static unsigned char int2Mu(p)
    short p;
{
    p >>= 2; /* scale input; table size is for 14 bit number! */

#if 1
    if (p >= (IMULAWTABLEN/2))
	return iMuLaw[IMULAWTABLEN-1];
    if (p < (-IMULAWTABLEN/2))
	return iMuLaw[0];
    else
#endif
	return iMuLaw[p + IMULAWTABLEN/2];
}


static unsigned char *data_pointer(s)
    SNDSoundStruct *s;
{
    unsigned char *p = (unsigned char *)s;
    p += s->dataLocation;
    return p;
}

static int calcInfoSize(s)
    SNDSoundStruct *s;
{
    int size = strlen(s->info) + 1;
    if (size < 4) size = 4;
    else size = (size + 3) & ~3;
    return size;
}

// This function is currently not usesd
#if 0
static int calcHeaderSize(s)
    SNDSoundStruct *s;
{
    int size = strlen(s->info) + 1;
    if (size < 4) size = 4;
    else size = (size + 3) & 3;
    return(sizeof(SNDSoundStruct) - 4 + size);
}
#endif

static int convertLinearToMulaw(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    int	i, size, err, infosize = calcInfoSize(s1);
    short *src;
    unsigned char *dst;

    size = s1->dataSize / 2;
    err = SNDAlloc(s2,size,SND_FORMAT_MULAW_8,s1->samplingRate,
    					s1->channelCount,infosize);
    if (err) return err;
    src = (short *)data_pointer(s1);
    dst = data_pointer(*s2);
    if (!iMuLaw) makeIMuLawTab();
    for (i = 0; i < size; i++) {
	*dst++ = int2Mu(*src++);
    }
    return SND_ERR_NONE;
}

static int convertMulawToLinear(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    int	i, size, max, err, infosize = calcInfoSize(s1);
    unsigned char *src;
    short *dst;

    max = s1->dataSize;
    size = max * 2;
    err = SNDAlloc(s2,size,SND_FORMAT_LINEAR_16,s1->samplingRate,
    					s1->channelCount,infosize);
    if (err) return err;
    src = data_pointer(s1);
    dst = (short *)data_pointer(*s2);
    for (i = 0; i < max; i++)
	*dst++ = muLaw[*src++];
    return SND_ERR_NONE;
}

static int convertMonoToStereo(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    int	err, i, size, max, infosize = calcInfoSize(s1);

    max = s1->dataSize;
    size = max * 2;
    
    err = SNDAlloc(s2,size,s1->dataFormat,s1->samplingRate,2,infosize);
    if (err) return err;

    if (s1->dataFormat == SND_FORMAT_MULAW_8 ||
				s1->dataFormat == SND_FORMAT_LINEAR_8) {
	unsigned char *src, *dst;
	src = data_pointer(s1);
	dst = data_pointer(*s2);
	for (i=max; i>0; i--) {
	    *dst++ = *src;
	    *dst++ = *src++;
	}
	return SND_ERR_NONE;
    } else if (s1->dataFormat == SND_FORMAT_LINEAR_16 ||
    	       s1->dataFormat == SND_FORMAT_EMPHASIZED) {
	short *src, *dst;
	src = (short *)data_pointer(s1);
	dst = (short *)data_pointer(*s2);
	for (i=max/2; i>0; i--) {
	    *dst++ = *src;
	    *dst++ = *src++;
	}
	return SND_ERR_NONE;
    } else
	return SND_ERR_BAD_FORMAT;
}

/*
 * dsp stuff
 */

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
    return err;
}

/* The following constants must match those in "dspsound.asm" */
#define DMASIZE 	(2048)
#define LEADPAD		(2)
#define TRAILPAD	(2)	/* not currently used */

#define READ_TAG 1
#define WRITE_TAG 2

typedef struct { 
    int timeout;
    int flush_timeout;
    char *read_ptr;
    int read_count;
    int read_done;
    port_t cmd_port;
} runDSP_data_t;

 static void runDSP_write_completed(void *arg, int tag)
{
    runDSP_data_t *data = (runDSP_data_t *)arg;
    int err;
    if (tag == WRITE_TAG) {
	err = snddriver_dsp_set_flags(data->cmd_port,
					SNDDRIVER_ICR_HF0,SNDDRIVER_ICR_HF0,
						SNDDRIVER_MED_PRIORITY);
	data->timeout = data->flush_timeout;
    }
}

 static void runDSP_read_data(void *arg, int tag, void *p, int size)
{
    runDSP_data_t *data = (runDSP_data_t *)arg;
    if (tag == READ_TAG) {
	data->read_ptr = (char *)p;
	data->read_count = size;
	data->read_done = 1;
    }
}

int SNDRunDSP(SNDSoundStruct *core,
		  char *write_ptr,
		  int write_count,
		  int write_width,
		  int write_buf_size,
		  char **read_ptr,
		  int *read_count,
		  int read_width,
		  int negotiation_timeout,
		  int flush_timeout,
		  int conversion_timeout)
{
    static msg_header_t *reply_msg = 0;
    int err, protocol = 0;
    int flushed;
    int priority = 1, preempt = 0, low_water = 32*1024, high_water = 32*1024;
    port_t dev_port=PORT_NULL, owner_port=PORT_NULL;
    port_t read_port, write_port, reply_port;
    int req_size = *read_count+(DMASIZE*LEADPAD);
    int bufsize = 2048; //? should follow vm_page_size -- change dsp program!
    runDSP_data_t data =  { -1, flush_timeout, 0, 0, 0, 0 };

    snddriver_handlers_t handlers = { &data, 0, 
    		0, runDSP_write_completed, 0, 0, 0, 0, runDSP_read_data};
    if (!reply_msg)
	reply_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
    err = SNDAcquire(SND_ACCESS_DSP,priority,preempt, negotiation_timeout,
    				NULL_NEGOTIATION_FUN, (void *)0,
					&dev_port, &owner_port);
    if (err) return err;
    err = snddriver_get_dsp_cmd_port(dev_port,owner_port,
    						&data.cmd_port);
    if (err) goto kerr_exit;
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_STREAM_FROM_DSP,
				 DMASIZE, read_width,
				 low_water, high_water,
				 &protocol, &read_port);
    if (err) goto kerr_exit;
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_STREAM_TO_DSP,
				 write_buf_size, write_width, 
				 low_water, high_water,
				 &protocol, &write_port);
    if (err) goto kerr_exit;
    err = snddriver_dsp_protocol(dev_port, owner_port, protocol);
    if (err) goto kerr_exit;

    err = port_allocate(task_self(),&reply_port);
    if (err) goto kerr_exit;

    err = snddriver_stream_start_reading(read_port, 0, req_size, READ_TAG,
					 	0,1,0,0,0,0, reply_port);
    if (err) goto kerr_exit;
					 
    err = SNDBootDSP(dev_port,owner_port,core);
    if (err) goto err_exit;

    err = snddriver_dsp_write(data.cmd_port,&bufsize,1,4,
    						SNDDRIVER_MED_PRIORITY);
    if (err) goto err_exit;

    err = snddriver_stream_start_writing(write_port,
    					 (void *)write_ptr,write_count,
					 WRITE_TAG,
					 0,0,
					 0,1,0,0,0,0, reply_port);
    if (err) goto kerr_exit;

    data.timeout = -1;
    data.flush_timeout = flush_timeout;
    data.read_done = 0;
    flushed = 0;
    while (!data.read_done) {
	reply_msg->msg_size = MSG_SIZE_MAX;
	reply_msg->msg_local_port = reply_port;
	if (data.timeout > 0)
	    err = msg_receive(reply_msg, RCV_TIMEOUT, data.timeout);
	else if (conversion_timeout < 0)
	    err = msg_receive(reply_msg, MSG_OPTION_NONE, 0);
	else
	    err = msg_receive(reply_msg, RCV_TIMEOUT, conversion_timeout);
	if (err != KERN_SUCCESS) {
	    if (err == RCV_TIMED_OUT && !flushed) {
		err = snddriver_stream_control(read_port,READ_TAG, 
						SNDDRIVER_ABORT_STREAM);
		if (err != KERN_SUCCESS) goto kerr_exit;
		flushed = 1;
	    } else {
		err = (err == RCV_TIMED_OUT)? SND_ERR_TIMEOUT : SND_ERR_KERNEL;
		goto err_exit;
	    }
	} else {
	    err = snddriver_reply_handler(reply_msg,&handlers);
	    if (err != KERN_SUCCESS) goto kerr_exit;
	}
    }
    *read_ptr = data.read_ptr;
    *read_count = data.read_count / read_width;
 //normal_exit:
    err = SNDRelease(SND_ACCESS_DSP,dev_port,owner_port);
    return err;
 kerr_exit:
     err = SND_ERR_KERNEL;
 err_exit:
    SNDRelease(SND_ACCESS_DSP,dev_port,owner_port);
    return err;
}

static void makeIntoSoundStruct(char *p, int size, int width, int header_size,
				SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    int nbytes = size*width;
    SNDSoundStruct *s;
    char *h = (char *)(*s2);
    char *free = p;
    int offset = (DMASIZE*LEADPAD*width) - header_size;
    p += offset;
    memmove(p,(char *)s1,header_size); //get the info string and padding
    memmove(p,h,sizeof(SNDSoundStruct)-4); // get the header itself
    s = (SNDSoundStruct *)p;
    s->dataLocation = header_size;
    s->dataSize = nbytes - offset - header_size;
    if (offset >= PAGESIZE) {
	int pageOffset = (int)p % PAGESIZE;
	int free_bytes = nbytes - offset - pageOffset;
	vm_deallocate(task_self(),(pointer_t)free,free_bytes);
    }
    *s2 = s;
}

/*
 * Put included files containing static functions right here.
 */

#include "upsamplecodec.c"
#include "squelch.c"
#include "downby2.c"
#include "compress.c"
#include "resample.c"

/*
 * Exported routines.
 */

unsigned char SNDMulaw(short n)
{
    if (!iMuLaw) makeIMuLawTab();
    return int2Mu(n);
}

short SNDiMulaw(unsigned char m)
{
    return muLaw[m];
}

int SNDConvertSound(SNDSoundStruct *s1, SNDSoundStruct **s)
{
    SNDSoundStruct *s2;
    SNDSoundStruct *dummyCore;
    if (!s1 ||  s1->magic != SND_MAGIC)
	return SND_ERR_NOT_SOUND;
    if (!s ||  !(s2 = *s) || s2->magic != SND_MAGIC)
	return SND_ERR_BAD_CONFIGURATION;
    if ( s1->samplingRate == (int)SND_RATE_CODEC &&
	     s1->dataFormat == SND_FORMAT_MULAW_8 &&
		 s2->samplingRate == (int)SND_RATE_LOW &&
		     s2->dataFormat == SND_FORMAT_LINEAR_16) {
	if (s1->channelCount == 1 && s2->channelCount == 2)
	    return dspUpsampleCodec(s1,s);
	else if (s1->channelCount == s2->channelCount)
	    return upsampleCodec(s1,s);
    } else if ( s1->samplingRate == (int)SND_RATE_CODEC &&
	     s1->dataFormat == SND_FORMAT_MULAW_8 &&
		 s2->samplingRate == (int)SND_RATE_CODEC &&
		     s2->dataFormat == SND_FORMAT_MULAW_SQUELCH) {
	if (s1->channelCount == 1 && s2->channelCount == 1)
	    return dspEncodeSquelch(s1,s);
	else
	    return SND_ERR_NOT_IMPLEMENTED;
    }
     else if ( s1->samplingRate == (int)SND_RATE_CODEC &&
	     s1->dataFormat == SND_FORMAT_MULAW_SQUELCH &&
		 s2->samplingRate == (int)SND_RATE_CODEC &&
		     s2->dataFormat == SND_FORMAT_MULAW_8) {
	if (s1->channelCount == 1 && s2->channelCount == 1)
	    return dspDecodeMulawSquelch(s1,s);
	else
	    return SND_ERR_NOT_IMPLEMENTED;
    }
     else if ( s1->samplingRate == (int)SND_RATE_CODEC &&
	     s1->dataFormat == SND_FORMAT_MULAW_SQUELCH &&
		 s2->samplingRate == (int)SND_RATE_LOW &&
		     s2->dataFormat == SND_FORMAT_LINEAR_16) {
	if (s1->channelCount == 1 && s2->channelCount == 2)
	    return dspUpsampleCodecSquelch(s1,s);
	else
	    return SND_ERR_NOT_IMPLEMENTED;
    }
    else if (   s1->dataFormat == s2->dataFormat &&
		    s1->samplingRate == s2->samplingRate ) {
		if (s1->channelCount == 1 && s2->channelCount == 2)
		    return convertMonoToStereo(s1,s);
		else
		    return SND_ERR_NOT_IMPLEMENTED;
    }
    /*
     * Only run downby2 filter if DSP sample rate conversion not available.
     */
    else if ((findDSPcore("resample1", &dummyCore) != SND_ERR_NONE) &&
	     s1->dataFormat == SND_FORMAT_LINEAR_16 &&
	     s2->dataFormat == SND_FORMAT_LINEAR_16 &&
	     s1->samplingRate == SND_RATE_HIGH   &&
	     s2->samplingRate == SND_RATE_LOW ) {
	if (s1->channelCount == s2->channelCount )
	    return downsampleBy2(s1,s);
	else
	    return SND_ERR_NOT_IMPLEMENTED;
    }
    else if (s1->dataFormat == SND_FORMAT_MULAW_8 &&
    		s2->dataFormat == SND_FORMAT_LINEAR_16 &&
		s1->samplingRate == s2->samplingRate) {
	return convertMulawToLinear(s1,s);
    } else if (s2->dataFormat == SND_FORMAT_MULAW_8 &&
    		s1->dataFormat == SND_FORMAT_LINEAR_16 &&
		s1->samplingRate == s2->samplingRate) {
	return convertLinearToMulaw(s1,s);
    }
    else if ((s1->dataFormat == SND_FORMAT_LINEAR_16 ||
    	     s1->dataFormat == SND_FORMAT_EMPHASIZED) &&
	     s2->dataFormat == s1->dataFormat &&
	     s1->samplingRate != s2->samplingRate) {
	 return resampleDSP(s1, s, 
	                   (double)s2->samplingRate / (double)s1->samplingRate);
    }
    return SND_ERR_NOT_IMPLEMENTED;
}

int SNDCompressSound(SNDSoundStruct *s1, SNDSoundStruct **s2,
		     int bitFaithful, int dropBits)
{
    /*
     * dspCompress compresses or decompresses based on the format code
     * of s1.  Channel count equal 1 or 2 is the only restriction.
     */
    if (s1->channelCount != 1 && s1->channelCount != 2)
	return SND_ERR_BAD_CHANNEL;
    return compressDSP(s1, s2, bitFaithful, dropBits);
}
