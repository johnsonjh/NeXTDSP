/*
 * Convert sampling rate from 44 to 22 kHz (decimate by 2), using
 * simplest fir filter with 3 points: .25 .5 .25
 * This filter does not have unity gain.  It does use saturation
 * arithmetic.
 * Sleaze Alert:
 * This is not a high quality convertor.
 * We have a high-quality convertor, but it is so slow in software,
 * that it only seemed worthwhile to use this simple filter,
 * and wait for a dsp version of the high quality decimator.
 *
 * Modification History:
 *	10/15/90/mtm	Change points to .25 .5 .25 (bug #10775).
 */
static void Skip2(  short *src, short *dst, int size, int nChans );


static int downsampleBy2(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    int err, srcSize, dstSize, nChans, infosize = calcInfoSize(s1);

    /* divide by two because target size is decimated by 2 */
    dstSize = s1->dataSize/2;
    srcSize = s1->dataSize;
    nChans = s1->channelCount;

    err = SNDAlloc(s2,dstSize,SND_FORMAT_LINEAR_16,SND_RATE_LOW,
    					s1->channelCount,infosize);
    if (err) return err;

    if (s1->dataFormat == SND_FORMAT_LINEAR_16) {
	unsigned char *src, *dst;

	src = data_pointer(s1);
	dst = data_pointer(*s2);
	Skip2((short *) src, (short *) dst, srcSize, nChans ); 

	return SND_ERR_NONE;
    } else
	return SND_ERR_BAD_FORMAT;
}
static void Skip2(  short *src, short *dst, int size, int nChans )
{
    int   j,
	  k,
	  nsampframes,
	  val;
    /*
     * Simplest fir filter with 3 points: .25 .5 .25
     */
    
    /* First convert from bytes to samples; i prefer sample frames. */
    nsampframes = size /( sizeof(short) * nChans );
    
    /* lose first and last samps to make indexing easier */
    src+=nChans;
    nsampframes -= 1;
    for ( j = 0; j< (nsampframes>>1); j++)
    {
	/* Loop over each channel. */
	for( k=0; k < nChans; k++)
	{
	    val =  *(src - nChans) / 4;
	    val += *src / 2;
	    val += *(src + nChans) / 4;
	    if (val > 32767) val = 32767;
	    if (val < -32768) val = -32768;
	    *dst++ = (short) val;
	    src++;
	}
	src += nChans; /* skip every other input sample */
    }
}
