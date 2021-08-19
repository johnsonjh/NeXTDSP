
/*
 * This file is included by convertsound.c!!!!
 *
 * The C version is cheaper, worse-sounding algorithm than the dsp version.
 *
 * Modification History:
 *	07/18/90/mtm	Get rid of static data in interpolate4to11().
 */

static int dspUpsampleCodec(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
    static SNDSoundStruct *mulawCodecCore = 0;
    int	err;

    char *dstPtr;    
    int dstWidth = 2;
    int dstCount =  (s1->dataSize*11)/dstWidth;
    char *srcPtr = (char *)data_pointer(s1);
    int srcCount = s1->dataSize;
    int srcWidth = 1;
    int srcBufSize = 2048; // this is pretty arbitrary
    int headersize = s1->dataLocation;
    int negotiation_timeout = -1; //no timeout
    int flush_timeout = 100; // in milliseconds
    int conversion_timeout = 100+ 1000*SNDSampleCount(s1)/s1->samplingRate;
     // in milliseconds

    if (headersize > (LEADPAD*DMASIZE*dstWidth))
	return SND_ERR_INFO_TOO_BIG;

    if (!mulawCodecCore) {
	err = findDSPcore("mulawcodec",&mulawCodecCore);
	if (err) return err;
    }
    err = SNDRunDSP(mulawCodecCore,srcPtr,srcCount,srcWidth,srcBufSize,
		    &dstPtr,&dstCount,dstWidth,
		    negotiation_timeout, flush_timeout, conversion_timeout);
    if (!err)
        makeIntoSoundStruct(dstPtr,dstCount,dstWidth,headersize,s1,s2);
    return err;
}

static void interpolate4to11(register u_char *ip, register short *op,
			     int channel, int nchan,
			     short *saved_i4, short *saved_i5, short *saved_i6,
			     short *saved_i7, short *saved_i8)
{
    register int val;
    static short i0=0, i1=0, i2=0, i3=0, i4=0, i5=0, i6=0, i7=0, i8=0;

    ip += channel;	/* offset into channel */
    op += channel;
    i3 = muLaw[*ip];
    ip += nchan;
    i2 = muLaw[*ip];
    ip += nchan;
    i1 = muLaw[*ip];
    ip += nchan;
    i0 = muLaw[*ip];

	i8 = saved_i8[channel];
	i7 = saved_i7[channel];
	i6 = saved_i6[channel];
	i5 = saved_i5[channel];
	i4 = saved_i4[channel];


    /* gain scaling test: sum of magnitude of coeffs=6247 dec */
    /* gain scaling test: sum of coeffs=  4017 dec */
	val = (
		  (short) (-468) * i4
		+ (short) (1753) * i5
		+ (short) (3197) * i6
		+ (short) (-647) * i7
		+ (short) (182) * i8
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 5944 dec */
    /* gain scaling test: sum of coeffs=  4048 dec */
	val = (
		  (short) (173) * i3
		+ (short) (-599) * i4
		+ (short) (3573) * i5
		+ (short) (1250) * i6
		+ (short) (-349) * i7
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 4857 dec */
    /* gain scaling test: sum of coeffs=  4093 dec */
	val = (
		  (short) (-103) * i3
		+ (short) (356) * i4
		+ (short) (4036) * i5
		+ (short) (-279) * i6
		+ (short) (83) * i7
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 6386 dec */
    /* gain scaling test: sum of coeffs=  3996 dec */
	val = (
		  (short) (-568) * i3
		+ (short) (2262) * i4
		+ (short) (2752) * i5
		+ (short) (-632) * i6
		+ (short) (172) * i7
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 5478 dec */
    /* gain scaling test: sum of coeffs=  4074 dec */
	val = (
		  (short) (140) * i2
		+ (short) (-479) * i3
		+ (short) (3858) * i4
		+ (short) (778) * i5
		+ (short) (-223) * i6
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 5478 dec */
    /* gain scaling test: sum of coeffs= 4074  dec */
	val = (
		  (short) (-223) * i2
		+ (short) (778) * i3
		+ (short) (3858) * i4
		+ (short) (-479) * i5
		+ (short) (140) * i6
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 6386 dec */
    /* gain scaling test: sum of coeffs=  3960 dec */
	val = (
		  (short) (172) * i1
		+ (short) (-632) * i2
		+ (short) (2752) * i3
		+ (short) (2262) * i4
		+ (short) (-568) * i5
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 4857 dec */
    /* gain scaling test: sum of coeffs=  4093 dec */
	val = (
		  (short) (83) * i1
		+ (short) (-279) * i2
		+ (short) (4036) * i3
		+ (short) (356) * i4
		+ (short) (-103) * i5
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 5944 dec */
    /* gain scaling test: sum of coeffs=  4048 dec */
	val = (
		  (short) (-349) * i1
		+ (short) (1250) * i2
		+ (short) (3573) * i3
		+ (short) (-599) * i4
		+ (short) (173) * i5
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 6247 dec */
    /* gain scaling test: sum of coeffs= 4017  dec */
	val = (
		  (short) (182) * i0
		+ (short) (-647) * i1
		+ (short) (3197) * i2
		+ (short) (1753) * i3
		+ (short) (-468) * i4
		) >> 12;
	if (val > 32767) val = 32767;
	else 
	    if (val < -32767) val = -32767;
	*op = val;
	op += nchan;

    /* gain scaling test: sum of magnitude of coeffs= 4096!!! dec 
     * we don't need this multiply!
	val = (
		  (short) (4096) * i2
		) >> 12;
	*op = val;
    */
	*op = i2;

	saved_i8[channel] = i4;
	saved_i7[channel] = i3;
	saved_i6[channel] = i2;
	saved_i5[channel] = i1;
	saved_i4[channel] = i0;
}

static int upsampleCodec(SNDSoundStruct *s1, SNDSoundStruct **s2)
{
#   define MAXCHAN 16
    short saved_i4[MAXCHAN], 
          saved_i5[MAXCHAN],
	  saved_i6[MAXCHAN],
	  saved_i7[MAXCHAN],
	  saved_i8[MAXCHAN];
    int	err, i, j, size, max, nchan, infosize = calcInfoSize(s1);
    unsigned char *src;
    short *dst;
    nchan = s1->channelCount;
    if (nchan < 0 || nchan > MAXCHAN) return SND_ERR_BAD_CHANNEL;
    size = s1->dataSize*11/2;
    err = SNDAlloc(s2,size,SND_FORMAT_LINEAR_16,SND_RATE_LOW,nchan,infosize);
    if (err) return err;
    src = data_pointer(s1);
    dst = (short *)data_pointer(*s2);
    max = s1->dataSize / (4*nchan);
    for (i = max; i>0; i--) {
	for (j=nchan; j>0; j--)
	    interpolate4to11(src, dst, j, nchan,
			     saved_i4, saved_i5, saved_i6, saved_i7, saved_i8);
	src += 4*nchan;
	dst += 11*nchan;
    }
    return SND_ERR_NONE;
}


