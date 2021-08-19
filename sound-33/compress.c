/*
 * compress.c
 *	This file is included by convertsound.c!!!!
 *
 * Modification History:
 *	08/10/90/mtm	Added max timeout.
 *	08/15/90/mtm	Restrict encode length to 256, disable timeout for debug.
 *	09/28/90/mtm	Send correct data size to DSP for compression (bug #7909).
 *	10/01/90/mtm	Don't copy sound (bug #10005).
 *	10/11/90/mtm	Only pad to dma size (real fix for bug #10011).
 */

#define	COMPRESS_DMASIZE 2048	/* must be compatible with hostdecompress.asm
				   and hostcompress.asm */
#define	COMPRESS_STREAM_AWAIT_TIMEOUT 1000
#define	COMPRESS_MAX_TIMEOUTS 5
#define COMPRESS_READ_TAG 1
#define COMPRESS_WRITE_TAG 2
#define	BIT_FAITHFUL	1		/* method=1 for bit faithful */

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

/* Modes */
enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS
};

typedef struct {			/* Keep in sync with performsound.c */
    int	originalSize;
    int method;
    int numDropped;
    int encodeLength;
    int reserved;
} compressionSubHeader;

typedef struct {
    int	mode;
    char *read_ptr;
    int read_count;
    int	remaining_bytes;
    int read_done;
    char *block_ptr;
    int block_count;
    compressionSubHeader subheader;
    int sound_header_size;
    port_t cmd_port;
    port_t read_port;
} compress_info_t;

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

static void compress_read_data(void *arg, int tag, void *p, int i)
/*
 * Copy the returned buffer into the sound and request the next
 * buffer using stream control.  If compressing, abort stream when
 * data representing all samples has been received.
 */
{
    compress_info_t *info = (compress_info_t *)arg;
    int size, err;
    int count, code, numBits;
    int numSamples = 0;
    
    size = (i > info->remaining_bytes) ? info->remaining_bytes : i;
    memmove((void *)info->read_ptr, p, size);
    info->read_ptr += size;
    info->read_count += size;
    info->remaining_bytes -= size;
    if (info->remaining_bytes <= 0)
	info->read_done = 1;
    
    if (info->mode == MODE_COMPRESS) {
	numSamples = info->subheader.originalSize / 2;
	/*
	 * Note: dataSize gets truncated leaving a hole of unused
	 * but allocated memory in the sound.  This hole of course goes
	 * away if you write the compressed sound to a file.
	 */
	while (info->read_ptr > info->block_ptr) {
	    if (((info->block_count-1) * info->subheader.encodeLength) >=
		numSamples) {
		info->read_count -= info->read_ptr - info->block_ptr;
		if (info->read_count > info->subheader.originalSize) {
		    info->read_count = info->subheader.originalSize;
#ifdef DEBUG
		    printf("Sound could not be compressed\n");
#endif
		}
		info->read_done = 1;
		break;
	    }
	    code = *info->block_ptr++;
	    numBits = *info->block_ptr++;
	    if ((unsigned)code >= NUM_ENCODES || (unsigned)numBits > 16) {
#ifdef DEBUG
		printf("BOGUS!! block=%d, code=%d, numBits=%d\n",
		       info->block_count, code, numBits);
#endif
		info->read_done = 1;
		break;
	    }
	    
	    count = bytesInBlock(code, numBits, info->subheader.encodeLength);
	    if (count & 1)
		count++;	/* pad to short */
	    info->block_ptr += count;
	    info->block_count++;
	}
    }
    /*
     * Tell the driver to send us data as soon as possible.
     * Normally we get one COMPRESS_DMASIZE chunk each time.
     */
    err = snddriver_stream_control(info->read_port, tag, SNDDRIVER_AWAIT_STREAM);
    err = vm_deallocate(task_self(),(pointer_t)p,i);
}

static int compressDSP(SNDSoundStruct *s1, SNDSoundStruct **s2,
		       int bitFaithful, int dropBits)
/*
 * Compress or decompress on DSP depending on mode of s1.
 * Largely a rip-off of SNDRunDSP().
 */
{
    static SNDSoundStruct *decompressCore = NULL;
    static SNDSoundStruct *compressCore = NULL;
    static msg_header_t *reply_msg = 0;
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
    SNDSoundStruct *core;
    int err, protocol = 0;
    int priority = 1, preempt = 0, low_water = 32*1024, high_water = 32*1024;
    port_t dev_port=PORT_NULL, owner_port=PORT_NULL;
    port_t read_port, write_port, reply_port;
    int req_size, totalSize = 0;
    int bufsize = COMPRESS_DMASIZE;
    int dmaBytes = COMPRESS_DMASIZE * 2;
    int headerSize, sampleSkip, encodeLen, encodeSize;
    int negotiation_timeout = -1;
    compress_info_t info;
    int timeoutCount = 0;
    snddriver_handlers_t handlers = { &info, 0, 0, 0,
    				      0, 0, 0, 0,
				      compress_read_data };
    void *write_ptr;
    int write_count, write_width;
    int read_width;
    compressionSubHeader *subheader = NULL;

    if (s1->dataFormat == SND_FORMAT_COMPRESSED ||
	s1->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED)
	info.mode = MODE_DECOMPRESS;
    else if (s1->dataFormat == SND_FORMAT_LINEAR_16 ||
	     s1->dataFormat == SND_FORMAT_EMPHASIZED)
	info.mode = MODE_COMPRESS;
    else
	return SND_ERR_BAD_FORMAT;

    /* DMA must start on a page boundry so we send the whole sound,
       including the header.  The DSP is passed the number of bytes
       to ignore (the header size). Also
       Bump write_count up to next dma size multiple for dma to the dsp.
       This memory exists because either map_fd() vm_allocate() was used to
       create the sound and therefore has memory up to the next page size. */

    write_ptr = (void *)s1;
    write_width = read_width = 2;
    totalSize = s1->dataLocation + s1->dataSize;
    if (totalSize % dmaBytes)
        totalSize = (totalSize + dmaBytes) & ~(dmaBytes - 1);
    write_count = totalSize / write_width;

    if (info.mode == MODE_DECOMPRESS) {
	if (!decompressCore) {
	    err = findDSPcore("hostdecompress", &decompressCore);
	    if (err) return err;
	}
	core = decompressCore;
	if (s1->dataSize <= sizeof(compressionSubHeader))
	    return SND_ERR_BAD_SIZE;
	subheader = (compressionSubHeader *)data_pointer(s1);
	req_size = subheader->originalSize / read_width;
	err = vm_allocate(task_self(), (pointer_t *)s2,
			  subheader->originalSize + s1->dataLocation, 1);
	if (err != KERN_SUCCESS) return SND_ERR_CANNOT_ALLOC;
	memmove(*s2, s1, s1->dataLocation);	/* get header and info */
	(*s2)->dataSize = info.read_count = 0;
	(*s2)->dataFormat = (s1->dataFormat == SND_FORMAT_COMPRESSED_EMPHASIZED ?
			   SND_FORMAT_EMPHASIZED : SND_FORMAT_LINEAR_16);
	info.read_ptr = (char *)data_pointer(*s2);
	info.remaining_bytes = subheader->originalSize;
    } else {	/* MODE_COMPRESS */
	if (!compressCore) {
	    err = findDSPcore("hostcompress", &compressCore);
	    if (err) return err;
	}
	core = compressCore;
	/*
	 * dataSize must be a multiple of the encode length.
	 */
	encodeLen = 256; /*bestEncodeLength[dropBits];*/
	encodeSize = encodeLen * s1->channelCount * write_width;
	s1->dataSize = (s1->dataSize / encodeSize) * encodeSize;
	if (s1->dataSize <= 0)
	    return SND_ERR_BAD_SIZE;
	req_size = s1->dataSize / read_width;
	err = vm_allocate(task_self(), (pointer_t *)s2,
			  s1->dataSize + s1->dataLocation, 1);
	if (err != KERN_SUCCESS) {
	    err = SND_ERR_CANNOT_ALLOC;
	    goto err_exit;
	}
	memmove(*s2, s1, s1->dataLocation);	/* get header and info */
	(*s2)->dataSize = sizeof(compressionSubHeader);
	(*s2)->dataFormat = (s1->dataFormat == SND_FORMAT_EMPHASIZED ?
			     SND_FORMAT_COMPRESSED_EMPHASIZED : SND_FORMAT_COMPRESSED);
	subheader = (compressionSubHeader *)data_pointer(*s2);
	subheader->originalSize = s1->dataSize;
	if (bitFaithful)
	    subheader->method = BIT_FAITHFUL;
	if (dropBits < 4)
	    dropBits = 4;
	else if (dropBits > 8)
	    dropBits = 8;
	subheader->numDropped = dropBits;
	subheader->encodeLength = encodeLen;
	subheader->reserved = 0;
	info.subheader = *subheader;
	info.sound_header_size = (*s2)->dataLocation;
	info.read_ptr = (char *)data_pointer(*s2) + sizeof(compressionSubHeader);
	info.block_ptr = info.read_ptr;
	info.block_count = 0;
	info.read_count = sizeof(compressionSubHeader);
	info.remaining_bytes = s1->dataSize - sizeof(compressionSubHeader);
    }
    if (!reply_msg)
	reply_msg = (msg_header_t *)malloc(MSG_SIZE_MAX);
    err = SNDAcquire(SND_ACCESS_DSP, priority, preempt, negotiation_timeout,
		     NULL_NEGOTIATION_FUN, (void *)0,
		     &dev_port, &owner_port);
    if (err) goto err_exit;
    err = snddriver_get_dsp_cmd_port(dev_port,owner_port,
				     &info.cmd_port);
    if (err) goto kerr_exit;
    /*
     * Do DSP-initiated DMA in both directions (see dspsounddi.asm).
     */
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_DMA_STREAM_FROM_DSP,
				 COMPRESS_DMASIZE, read_width,
				 low_water, high_water,
				 &protocol, &read_port);
    if (err) goto kerr_exit;
    info.read_port = read_port;
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_DMA_STREAM_TO_DSP,
				 COMPRESS_DMASIZE, write_width, 
				 low_water, high_water,
				 &protocol, &write_port);
    if (err) goto kerr_exit;
    err = snddriver_dsp_protocol(dev_port, owner_port, protocol);
    if (err) goto kerr_exit;

    err = port_allocate(task_self(),&reply_port);
    if (err) goto kerr_exit;

    err = snddriver_stream_start_reading(read_port, 0, req_size, COMPRESS_READ_TAG,
					 0,0,0,0,0,0, reply_port);
    if (err) goto kerr_exit;
					 
    err = SNDBootDSP(dev_port,owner_port,core);
    if (err) goto err_exit;
    /*
     * Send parameters to the DSP.
     */
    headerSize = s1->dataLocation / 2;
    err = snddriver_dsp_write(info.cmd_port,&bufsize,1,4,SNDDRIVER_MED_PRIORITY);
    err = snddriver_dsp_write(info.cmd_port,&headerSize,1,4,SNDDRIVER_MED_PRIORITY);
    err = snddriver_dsp_write(info.cmd_port,&s1->channelCount,1,4,SNDDRIVER_MED_PRIORITY);
    if (err) goto err_exit;
    if (info.mode == MODE_COMPRESS) {
	err = snddriver_dsp_write(info.cmd_port,&subheader->method,1,4,
				  SNDDRIVER_MED_PRIORITY);
	err = snddriver_dsp_write(info.cmd_port,&subheader->numDropped,1,4,
				  SNDDRIVER_MED_PRIORITY);
	err = snddriver_dsp_write(info.cmd_port,&subheader->encodeLength,1,4,
				  SNDDRIVER_MED_PRIORITY);
	sampleSkip = s1->channelCount;
	err = snddriver_dsp_write(info.cmd_port,&sampleSkip,1,4,SNDDRIVER_MED_PRIORITY);
	if (err) goto err_exit;
    }

    err = snddriver_stream_start_writing(write_port,
    					 write_ptr,write_count,
					 COMPRESS_WRITE_TAG,
					 0,0,
					 0,0,0,0,0,0, reply_port);
    if (err) goto kerr_exit;
    err = snddriver_stream_control(read_port, COMPRESS_READ_TAG, SNDDRIVER_AWAIT_STREAM);
    if (err != KERN_SUCCESS)
	goto kerr_exit;

    info.read_done = 0;
    while (!info.read_done) {
	reply_msg->msg_size = MSG_SIZE_MAX;
	reply_msg->msg_local_port = reply_port;
	err = msg_receive(reply_msg, RCV_TIMEOUT, COMPRESS_STREAM_AWAIT_TIMEOUT);
	if (err == RCV_TIMED_OUT) {
	    if (++timeoutCount > COMPRESS_MAX_TIMEOUTS) {
#ifdef DEBUG
		fprintf(stderr, "Request timed out\n");
#endif
		goto normal_exit;
	    }
	    err = snddriver_stream_control(read_port, COMPRESS_READ_TAG,
					   SNDDRIVER_AWAIT_STREAM);
	    if (err != KERN_SUCCESS)
		goto kerr_exit;
	} else if (err != KERN_SUCCESS)
	    goto kerr_exit;
	else {
	    err = snddriver_reply_handler(reply_msg,&handlers);
	    if (err != KERN_SUCCESS) goto kerr_exit;
	    timeoutCount = 0;
	}
    }

 normal_exit:
    (*s2)->dataSize = info.read_count;
    err = SNDRelease(SND_ACCESS_DSP,dev_port,owner_port);
    return err;
 kerr_exit:
    (*s2)->dataSize = info.read_count;
    SNDRelease(SND_ACCESS_DSP,dev_port,owner_port);
    return SND_ERR_KERNEL;
 err_exit:
    (*s2)->dataSize = info.read_count;
    SNDRelease(SND_ACCESS_DSP,dev_port,owner_port);
    return err;
}
