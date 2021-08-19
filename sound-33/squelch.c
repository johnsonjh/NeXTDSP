


/*
 * Run-Length Encode Silences in 8 kHz muLaw data;
 * result is 8 kHz muLaw (compresssed)
 */

/* Sleaze alert: the following trims off the nasty zeros at the end of the
 * buffer that SNDRunDSP returns -- there is no efficient way of calculating
 * the true end of buffer (we sure don't want to scan the data again...).
 * Anyway, this tends to work for mulaw data.
 */
static void mulaw_trim_tail(SNDSoundStruct *s)
{
    unsigned char *p = (unsigned char *)data_pointer(s);
    int count;
 
    count = s->dataSize;
    p += count;
    while (!(*--p) && count > 0) count--;
    s->dataSize = count;
    return;
}

static int dspEncodeSquelch(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    static SNDSoundStruct *encodeSquelchCore = 0;
    int	err;
    unsigned char temp [8];	// for preserving the info string 

    char *dstPtr;    
    char *srcPtr = (char *)data_pointer(s1);
    char *t1;
    int srcCount = s1->dataSize;
    int srcWidth = 1;
    int srcBufSize = 2048; // this is pretty arbitrary
    int headersize = s1->dataLocation;
    int negotiation_timeout = -1; //no timeout
    int flush_timeout = 100; // in milliseconds
    int conversion_timeout = 100+ 1000*SNDSampleCount(s1)/s1->samplingRate;
     // in milliseconds
    int dstCount =  (s1->dataSize/128) + s1->dataSize + 32;  // + 32 for safety
    int dstWidth = 1;
    short threshold;

    short *t2 = (short *)data_pointer(*s2);

    if (headersize > (LEADPAD*DMASIZE*dstWidth))
	return SND_ERR_INFO_TOO_BIG;

    if (!encodeSquelchCore) {
	err = findDSPcore("encodemulawsquelch",&encodeSquelchCore);
	if (err) return err;
    }

    // here we insert the squelch file internal header into the data stream
/*
 * The data in a squelch data file are structured;
 * there is a header with extra bytes to allow for future 
 * parameters.
 * 6-12-89 dana c. massie
 *
 * these parameters are also used in the 
 * encodemulawswuelch.asm,
 * decodemulawsquelch.asm
 * mulawcodecsquelch.asm
 * files, and must be changed in parallel with those files.
 */



#define SQUELCH_INTERNAL_HEADER_SIZE (16)
#define SQUELCH_INIT_FILE_FLAG (0xA5)		/* magic flag byte */
#define DEFAULT_THRESHOLD	(100)

    if ((*s2)->dataSize == 0) {
	// use default threshold
	threshold = DEFAULT_THRESHOLD;
    }
    else
	threshold = *t2;
    srcPtr -= SQUELCH_INTERNAL_HEADER_SIZE;
    t1 = srcPtr;
//    bcopy(srcPtr,temp,SQUELCH_INTERNAL_HEADER_SIZE);
    memmove(temp,srcPtr,SQUELCH_INTERNAL_HEADER_SIZE);
    
    srcCount += SQUELCH_INTERNAL_HEADER_SIZE;
    // sleaze alert; crude method to fill this array...
    *t1++ = SQUELCH_INIT_FILE_FLAG;
    *t1++ = 0;
    *((short *) t1) = threshold; // here we need the threshold; 2 bytes
    t1 += 2;
    *((int *) t1) = s1->dataSize; // un-encoded file length; 4 bytes
    
    
    err = SNDRunDSP(encodeSquelchCore,srcPtr,srcCount,srcWidth,srcBufSize,
		    &dstPtr,&dstCount,dstWidth,
		    negotiation_timeout, flush_timeout, conversion_timeout);
    // restore the original info string
//    bcopy(temp,srcPtr,SQUELCH_INTERNAL_HEADER_SIZE);
    memmove(srcPtr, temp, SQUELCH_INTERNAL_HEADER_SIZE);
    
    if (!err) {
        makeIntoSoundStruct(dstPtr,dstCount,dstWidth,headersize,s1,s2);
	// sleaze alert: get rid of buffer tail
	mulaw_trim_tail(*s2);
    }
    return err;
}
/*
 * This routine decodes 8 bit muLaw run-length silence encoded files
 * into 8 bit MuLaw 8 kHz result.
 */
static int dspDecodeMulawSquelch(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    static SNDSoundStruct *decodeSquelchCore = 0;
    int	err;

    char *dstPtr;    
    char *srcPtr = (char *)data_pointer(s1);
    int srcCount = s1->dataSize;
    int srcWidth = 1;
    int srcBufSize = 2048; // this is pretty arbitrary
    int headersize = s1->dataLocation;
    int negotiation_timeout = -1; //no timeout
    int flush_timeout = 100; // in milliseconds
    int conversion_timeout = 100+ 1000*SNDSampleCount(s1)/s1->samplingRate;
     // in milliseconds
    int dstCount =  SNDSampleCount(s1) ;  
    int dstWidth = 1;


    if (headersize > (LEADPAD*DMASIZE*dstWidth))
	return SND_ERR_INFO_TOO_BIG;

    if (!decodeSquelchCore) {
	err = findDSPcore("decodemulawsquelch",&decodeSquelchCore);
	if (err) return err;
    }

    err = SNDRunDSP(decodeSquelchCore,srcPtr,srcCount,srcWidth,srcBufSize,
		    &dstPtr,&dstCount,dstWidth,
		    negotiation_timeout, flush_timeout, conversion_timeout);
    
    if (!err) {
	//sleaze! we should find a better way to trim the tail!!
//	dstCount = SNDSampleCount(s1) + (DMASIZE*LEADPAD);
        makeIntoSoundStruct(dstPtr,dstCount,dstWidth,headersize,s1,s2);
	mulaw_trim_tail(*s2);
    }
    return err;

}
/*
 * This routine decodes 8 bit muLaw run-length silence encoded files
 * into 16 bit Stereo 22050 Hz result.
 */
static int dspUpsampleCodecSquelch(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    static SNDSoundStruct *decodeSquelchCore = 0;
    int	err;

    char *dstPtr;    
    char *srcPtr = (char *)data_pointer(s1);
    int srcCount = s1->dataSize;
    int srcWidth = 1;
    int srcBufSize = 2048; // this is pretty arbitrary
    int headersize = s1->dataLocation;
    int negotiation_timeout = -1; //no timeout
    int flush_timeout = 100; // in milliseconds
    int conversion_timeout = 100+ 1000*SNDSampleCount(s1)/s1->samplingRate;
     // in milliseconds
    int dstCount =  SNDSampleCount(s1) ;  
    int dstWidth = 2;

    dstCount *=  11;
    dstCount /=  dstWidth;
    
    if (headersize > (LEADPAD*DMASIZE*dstWidth))
	return SND_ERR_INFO_TOO_BIG;

    if (!decodeSquelchCore) {
	err = findDSPcore("mulawcodecsquelch",&decodeSquelchCore);
	if (err) return err;
    }

    err = SNDRunDSP(decodeSquelchCore,srcPtr,srcCount,srcWidth,srcBufSize,
		    &dstPtr,&dstCount,dstWidth,
		    negotiation_timeout, flush_timeout, conversion_timeout);
    
    if (!err)
        makeIntoSoundStruct(dstPtr,dstCount,dstWidth,headersize,s1,s2);
    return err;


}
