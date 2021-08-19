int _DSPPrintDatum(
    FILE *fp,
    DSPFix24 word);
/*
 * Print DSP datum in decimal, hex, and fractional fixed-point.
 */


int _DSPPrintValue(
    DSPMemorySpace space,
    DSPAddress address);
/*
 * Get DSP memory datum at space:address and print it.
 */


int _DSPDump(char *name);
/*
 * Dump DSP external RAM into files of the form _DSPCat(name,"X.ram") etc.
 */


/*** data arg should be in 2nd position but the shlib is frozen ***/
int _DSPMKSendUnitGeneratorWithLooperTimed(
    DSPFix48 *aTimeStampP, 
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    int count,
    int looperWord);
/*
 * Same as DSPMKSendArrayTimed() but tacks on one extra word which is a
 * DSP instruction which reads "jmp orchLoopStartAddress". Note that
 * code was copied from	 DSPMKSendArraySkipTimed().
 */

/* Flush? */
int _DSPMKSendTwoArraysTimed(
    DSPFix48 *aTimeStampP, 
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data1,
    int count1,
    DSPFix24 *data2,
    int count2);

/* Required for 1.0 MK binary compatibility */

int _DSPSendArraySkipTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    int skipFactor,
    int count);
/*
 * Calls DSPMKSendArraySkipModeTimed() with mode == DSP_MODE32.
 */

int _DSPSendValueTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    int addr,
    int value);

int _DSPSendLongTimed(
    DSPFix48 *aTimeStampP,
    int addr,
    DSPFix48 *aFix48Val);

/******************** GETTING PRIVATE DSP MEMORY ADDRESSES *******************/

DSPAddress _DSPMKGetDMABufferAddress(void);
/* 
 * Returns DSPGetValueOfSystemSymbol("YB_DMA_W").  This is the beginning of
 * the DSP DMA buffer pool used by the Music Kit.  Both sound-out and
 * read-data share this buffer memory.  It is carved up between input and
 * output at DSP open time.  The buffer size in each direction can also be
 * changed at run time.  Each direction is always double-buffered.
 */

DSPAddress _DSPMKGetDMABufferSize(void);
/* 
 * Returns DSPGetValueOfSystemSymbol("NB_DMA").  This is the total size in
 * words of the DSP DMA buffer pool.  Typically it is four times the size of
 * each individual DSP DMA size (two for input and output).
 */

