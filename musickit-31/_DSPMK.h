/* Frozen prototypes of all private libdsp functions used by Music Kit */

extern int _DSPError(int errorcode, char *msg);

extern int _DSPMKSendUnitGeneratorWithLooperTimed(
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

extern int _DSPReloc(DSPDataRecord *data, DSPFixup *fixups,
    int fixupCount, int *loadAddresses);
/* 
 * dataRec is assumed to be a P data space. Fixes it up in place. 
 * This is a private libdsp method used by _DSPSendUGTimed and
 * _DSPRelocate. 
 */


/* The following three could be changed to use the non-underbar (DSPMK)
   versions, but an argument order change is required. */
extern int _DSPSendArraySkipTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    int skipFactor,
    int count);
/*
 * Calls DSPMKSendArraySkipModeTimed() with mode == DSP_MODE32.
 */

extern int _DSPSendValueTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    int addr,
    int value);

extern int _DSPSendLongTimed(
    DSPFix48 *aTimeStampP,
    int addr,
    DSPFix48 *aFix48Val);


