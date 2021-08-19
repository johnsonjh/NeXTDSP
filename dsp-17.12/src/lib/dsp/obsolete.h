/* The following are obsolete and are to be deleted in release 1.1 */

int DSPWriteArray(
    DSPFix24 *data,		/* array to send to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int wordCount);		/* from DSP perspective */

int DSPPutArraySkip(
    DSPFix24 *data,		/* array to send to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */

int DSPPutArray(
    DSPFix24 *data,		/* array to send to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int wordCount);		/* from DSP perspective */

int DSPPutPackedArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int wordCount);		/* DSP words = byte count / 3 */

int DSPPutShortArray(
    DSPFix24 *data,		/* Packed short data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int wordCount);		/* DSP word count = byte count / 2 */

int DSPPutByteArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int byteCount);		/* Total number of bytes to transfer */

int DSPWriteIntArrayXY(
    int *intArray,
    DSPMemorySpace memorySpace,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPPutFloatArray(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount);
/*
 * Write a vector of floating-point numbers to a DSP array.
 * Equivalent to DSPFloatToFix24Array() followed by DSPWriteFix24Array().
 */


int DSPWriteFloatArrayXY(
    float *floatArray,
    DSPMemorySpace memorySpace,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPPutDoubleArray(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPGetArraySkip(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */

int DSPGetArray(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int wordCount);		/* from DSP perspective */

int DSPGetArrayPacked(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int wordCount);		/* DSP words = byte count / 3 */

int DSPGetByteArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int byteCount);		/* Same as DSP word count */

int DSPGetShortArray(
    DSPFix24 *data,		/* Packed data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int wordCount);		/* DSP word count = byte count / 2 */

int DSPGetIntArray(
    int *intArray,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPReadIntArrayXY(
    int *intArray,
    DSPMemorySpace memorySpace,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPGetFloatArray(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPReadFloatArrayXY(
    float *floatArray,
    DSPMemorySpace memorySpace,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPGetDoubleArray(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount);

int DSPMKLoad(DSPLoadSpec *dspimg);

int DSPMKSendPauseOrchestraTimed(DSPFix48 *aTimeStampP);

int DSPPutValue( DSPMemorySpace space, int addr, int value);

DSPFix48 *_DSPDoubleIntToFix48(double dval);
/* 
 * Returns, a pointer to a new DSPFix48 
 * with the value as represented by dval.
 * The double is assumed to be between -2^47 and 2^47. 
 */

char *DSPGetImgDirectory(void);

char *DSPGetMusicDirectory(void);  /* DSP_MUSIC_DIRECTORY 'less setenv DSP */

char *DSPGetLocalBinDirectory(void); /* DSP_BIN_DIRECTORY or $DSP/bin */

DSPSection *_DSPGetUserSection(
    DSPLoadSpec *dspStruct);

DSPAddress _DSPGetFirstAddress(
    DSPLoadSpec *dspStruct,
    DSPLocationCounter locationCounter);

DSPAddress _DSPGetLastAddress(
    DSPLoadSpec *dspStruct,
    DSPLocationCounter locationCounter);

void _DSPMemMapInit(_DSPMemMap *mm);
/* 
 * Initialize all fields to NULL.
 */

void _DSPMemMapPrint(_DSPMemMap *mm);


int DSPIsWithSoundOut(void);
/* 
 * Returns nonzero if the DSP is linked to sound out.
 */


int DSPIsMappedOnly(void);
/* 
 * Returns nonzero if DSP host interface is memory mapped to bypass driver.
 */


DSPRegs *DSPGetRegs(void);
/* 
 * Return pointer to the memory-mapped host-interface registers of the DSP.
 * If the DSP is not memory-mapped, NULL is returned. 
 * IMPORTANT: You should be very careful about manipulating these registers
 * when you are also using the Mach driver interface.  It is always safe to 
 * read registers other than RXL, and it may be safe to write registers 
 * sometimes.  However, be aware that the Mach DSP driver may be in the
 * middle of writing a message to the DSP when you do so. If you do not
 * write the memory-mapped host-interface, then Mach messages to the DSP
 * are guaranteed to be "atomic," that is, they cannot be interrupted.
 */


int DSPEnableMappedOnly(void);
/* 
 * Force all communication with the DSP to occur via the memory-mapped 
 * host interface. This function must be called while the DSP is closed.
 * In this mode, DSP interrupts and thus DMA transfers are not possible.
 * It is sometimes used in debugging to see if the problem is in the
 * driver or the user code.  
 */


int DSPDisableMappedOnly(void);
/* 
 * Select normal DSP driver as opposed to (unsupported) memory-mapped IO
 * driver. This function must be called before opening the DSP.
 */


int DSPMappedOnlyIsEnabled(void);
/* 
 * Return true if mapped-only mode is enabled.
 */


int DSPEnableMappedArrayTransfers(void);
/* 
 * Temporary test hack.
 */


int DSPDisableMappedArrayTransfers(void);
/* 
 * Temporary test hack.
 */


int DSPEnableUncheckedMappedArrayTransfers(void);
/* 
 * Temporary test hack.
 */


int DSPDisableUncheckedMappedArrayTransfers(void);
/* 
 * Temporary test hack.
 */


int DSPMKEnableWriteDataCleanup(void);
/* 
 * Temporary test hack.
 */


int DSPMKDisableWriteDataCleanup(void);
/* 
 * Temporary test hack.
 */


int DSPMKWriteDataCleanupIsEnabled(void);
/* 
 * Temporary test hack.
 */

double DSPGetSamplingRate(void);
/*
 * Returns sampling rate assumed by DSP software in Hz.
 */


int _DSPGetNumber(void);
/* 
 * Set assigned DSP number for this instance.
 * Same as DSPGetCurrentDSP().
 */

port_t DSPGetSoundPort(void);

port_t DSPGetWriteDataStreamPort(void);

port_t DSPGetSoundOutStreamPort(void);

port_t DSPGetWriteDataReplyPort(void);


int DSPMapHostInterface(void);	/* Memory-map DSP Host Interface Registers */
/*
 * Memory-map the DSP host interface registers.	 This mode of DSP access
 * was used before the real DSP Mach driver was written.  It is not a 
 * general DSP interface because there is no way for the DSP to interupt
 * the main CPU.
 */

int DSPOpenMapped(void);
/*
 * Open DSP in memory-mapped mode. 
 * No reset or boot is done.
 * DSPGetRegs() can be used to obtain a pointer to the DSP host interface.
 */


int DSPWriteICR(
    int icr);			
/*
 * Write DSP Interrupt Control Register (ICR).
 * Value written from icr is 8 bits, right justified.
 */


int DSPWriteCVR(
    int cvr);			
/*
 * Write DSP Command Vector Register (CVR).
 * Value written from cvr is 8 bits, right justified.
 */


int DSPPutICR(
    int icr); 


int DSPPutCVR(
    int cvr); 


int DSPPutTX(
    int tx); 


int DSPPutTXArray(
    DSPFix24 *data,
    int nwords);


int DSPPutTXArrayB(
    int *data,
    int nwords); 


int DSPGetRXArray(
    int *data,
    int nwords); 


int DSPAwaitIdleState(int msTimeLimit);


int DSPMKSendStartWriteData(void);

int DSPMKSendStopWriteDataTimed(DSPTimeStamp *aTimeStampP);


int DSPPutValue(
    DSPMemorySpace space,
    int addr,
    int value);


int DSPPutLong(
    int addr,
    DSPFix48 *aFix48Val);

int DSPDisableErrorLogging(
    int *old_DSPErrorBlockP);
/* 
 * Turn off DSP error message logging by setting the global variable
 * _DSPErrorBlock to 1.	 The previous state of _DSPErrorBlock is returned
 * in *old_DSPErrorBlockP (unless the pointer is NULL). 
 */


int DSPEnableErrorLogging(
    int *old_DSPErrorBlockP);
/* 
 * Turn on DSP error message logging by setting the global variable
 * _DSPErrorBlock to 1.	 The previous state of _DSPErrorBlock is returned
 * in *old_DSPErrorBlockP (unless the pointer is NULL).
 */


int DSPRestoreErrorLogging(
    int old_DSPErrorBlock);
/* 
 * Restore DSP error message logging according to the value of
 * old_DSPErrorBlock by assigning _DSPErrorBlock = old_DSPErrorBlock.
 */


int DSPRetValueTimed(
    DSPTimeStamp *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value);
/*
 * Send timed peek.  Since we do not know the current time within the
 * DSP, we wait forever for the returned value from the DSP.
 */


int DSPRetValue(
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value);

