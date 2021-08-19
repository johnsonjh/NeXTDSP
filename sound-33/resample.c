/*
 * resample.c
 *	This file is included by convertsound.c!!!!
 *
 * Modification History:
 *	08/10/90/mtm	Added max timeout.
 *	09/28/90/mtm	Fixed timeouts.
 *	10/01/90/mtm	Don't copy sound (bug #10005).
 *	10/08/90/mtm	Use different cores for mono and stereo (bug #10407).
 *	10/11/90/mtm	Only pad to dma size (real fix for bug #10011).
 */

#define	RESAMPLE_STREAM_AWAIT_TIMEOUT 1000
#define	RESAMPLE_MAX_TIMEOUTS 5
#define RESAMPLE_READ_TAG 1
#define RESAMPLE_WRITE_TAG 2

#define	RESAMPLE_DMASIZE 1024

typedef struct {
    char *read_ptr;
    int read_count;
    int	remaining_bytes;
    int read_done;
    port_t cmd_port;
    port_t read_port;
} resample_info_t;

static void resample_write_completed(void *arg, int tag)
{
    resample_info_t *info = (resample_info_t *)arg;
    int err;
    if (tag == RESAMPLE_WRITE_TAG) {
    	/* FIXME: this does not seem to flush dsp buffer */
	err = snddriver_dsp_set_flags(info->cmd_port,
				      SNDDRIVER_ICR_HF0,SNDDRIVER_ICR_HF0,
				      SNDDRIVER_HIGH_PRIORITY);
    }
}

static void resample_read_data(void *arg, int tag, void *p, int i)
/*
 * Copy the returned buffer into the sound and request the next
 * buffer using stream control.
 */
{
    resample_info_t *info = (resample_info_t *)arg;
    int size, err;
    
    size = (i > info->remaining_bytes) ? info->remaining_bytes : i;
    memmove((void *)info->read_ptr, p, size);
    info->read_ptr += size;
    info->read_count += size;
    info->remaining_bytes -= size;
    if (info->remaining_bytes <= 0)
	info->read_done = 1;
    /*
     * Tell the driver to send us data as soon as possible.
     * Normally we get one RESAMPLE_DMASIZE chunk each time.
     */
    err = snddriver_stream_control(info->read_port, tag, SNDDRIVER_AWAIT_STREAM);
    err = vm_deallocate(task_self(),(pointer_t)p,i);
}

static int resampleDSP(SNDSoundStruct *s1, SNDSoundStruct **s2, double factor)
/*
 * Sample-rate convert s1 by factor.
 * Largely a rip-off of SNDRunDSP().
 */
{
    static SNDSoundStruct *resample1Core = NULL, *resample2Core = NULL;
    static msg_header_t *reply_msg = 0;
    SNDSoundStruct *core;
    int err, protocol = 0;
    int priority = 1, preempt = 0, low_water = 32*1024, high_water = 32*1024;
    port_t dev_port=PORT_NULL, owner_port=PORT_NULL;
    port_t read_port, write_port, reply_port;
    int req_size, totalSize = 0;
    int bufsize = RESAMPLE_DMASIZE;
    int dmaBytes = RESAMPLE_DMASIZE * 2;
    int headerSize, timeIncrement;
    int negotiation_timeout = -1;
    resample_info_t info;
    int timeoutCount = 0;
    snddriver_handlers_t handlers = { &info, 0, 0, resample_write_completed,
				      0, 0, 0, 0,
				      resample_read_data };
    void *write_ptr;
    int write_count, write_width;
    int read_width;

    /* DMA must start on a page boundry so we send the whole sound,
       including the header.  The DSP is passed the number of bytes
       to ignore (the header size). Also
       Bump write_count up to next dma size multiple for dma to the dsp.
       This memory exists because either map_fd() vm_allocate() was used to
       create the sound and therefore has memory up to the next page size. */

    write_ptr = (void *)s1;
    write_width = read_width = 2;
    totalSize = s1->dataLocation + s1->dataSize;
    if (totalSize % dmaBytes) {
	/* must zero extra memory for resample filter */
	bzero((char *)s1+totalSize, dmaBytes - (totalSize % dmaBytes));
        totalSize = (totalSize + dmaBytes) & ~(dmaBytes - 1);
    }
    write_count = totalSize / write_width;

    if (s1->channelCount == 1) {
	if (!resample1Core) {
	    err = findDSPcore("resample1", &resample1Core);
	    if (err) return err;
	}
	core = resample1Core;
    } else {
	if (!resample2Core) {
	    err = findDSPcore("resample2", &resample2Core);
	    if (err) return err;
	}
	core = resample2Core;
    }
    req_size = (int)(((double)s1->dataSize) * factor) + 1;
    totalSize = s1->dataLocation + req_size;
    /*
     * Pad total size to dmasize multiple.
     */
    if (totalSize % (RESAMPLE_DMASIZE*2)) {
        totalSize = (totalSize + (RESAMPLE_DMASIZE*2)) & ~((RESAMPLE_DMASIZE*2)-1);
	req_size = totalSize - s1->dataLocation;
    }
    err = vm_allocate(task_self(), (pointer_t *)s2, totalSize, 1);
    if (err != KERN_SUCCESS) return SND_ERR_CANNOT_ALLOC;
    memmove(*s2, s1, s1->dataLocation);	/* get header and info */
    (*s2)->dataSize = info.read_count = 0;
    (*s2)->samplingRate = (int)((double)s1->samplingRate * factor);
    info.read_ptr = (char *)data_pointer(*s2);
    info.remaining_bytes = req_size;
    req_size /= read_width;

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
     * Do DSP-initiated DMA in both directions (see simple_dma_support.asm).
     */
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_DMA_STREAM_FROM_DSP,
				 RESAMPLE_DMASIZE, read_width,
				 low_water, high_water,
				 &protocol, &read_port);
    if (err) goto kerr_exit;
    info.read_port = read_port;
    err = snddriver_stream_setup(dev_port, owner_port,
    				 SNDDRIVER_DMA_STREAM_TO_DSP,
				 RESAMPLE_DMASIZE, write_width, 
				 low_water, high_water,
				 &protocol, &write_port);
    if (err) goto kerr_exit;
    err = snddriver_dsp_protocol(dev_port, owner_port, protocol);
    if (err) goto kerr_exit;

    err = port_allocate(task_self(),&reply_port);
    if (err) goto kerr_exit;

    err = snddriver_stream_start_reading(read_port, 0, req_size, RESAMPLE_READ_TAG,
					 0,1,0,0,0,0, reply_port);
    if (err) goto kerr_exit;
					 
    err = SNDBootDSP(dev_port,owner_port,core);
    if (err) goto err_exit;
    /*
     * Send parameters to the DSP.
     */
    headerSize = s1->dataLocation / 2;
    timeIncrement = (int)((1.0 / factor) * ((double)(1<<19)) + 0.4999);
    err = snddriver_dsp_write(info.cmd_port,&bufsize,1,4,SNDDRIVER_MED_PRIORITY);
    err = snddriver_dsp_write(info.cmd_port,&headerSize,1,4,SNDDRIVER_MED_PRIORITY);
    err = snddriver_dsp_write(info.cmd_port,&s1->channelCount,1,4,
                              SNDDRIVER_MED_PRIORITY);
    err = snddriver_dsp_write(info.cmd_port,&timeIncrement,1,4,
                              SNDDRIVER_MED_PRIORITY);
    if (err) goto err_exit;

    err = snddriver_stream_start_writing(write_port,
    					 write_ptr,write_count,
					 RESAMPLE_WRITE_TAG,
					 0,0,
					 0,1,0,0,0,0, reply_port);
    if (err) goto kerr_exit;
    err = snddriver_stream_control(read_port, RESAMPLE_READ_TAG,
				   SNDDRIVER_AWAIT_STREAM);
    if (err != KERN_SUCCESS)
	goto kerr_exit;

    info.read_done = 0;
    while (!info.read_done) {
	reply_msg->msg_size = MSG_SIZE_MAX;
	reply_msg->msg_local_port = reply_port;
	err = msg_receive(reply_msg, RCV_TIMEOUT, RESAMPLE_STREAM_AWAIT_TIMEOUT);
	if (err == RCV_TIMED_OUT) {
	    if (++timeoutCount > RESAMPLE_MAX_TIMEOUTS) {
#ifdef DEBUG
		fprintf(stderr, "Request timed out\n");
#endif
		goto normal_exit;
	    }
	    err = snddriver_stream_control(read_port, RESAMPLE_READ_TAG,
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
