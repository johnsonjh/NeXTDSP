/* DSPAPUtilities.h - Array processing utilities */

/* See /NextDeveloper/Examples/DSP/ArrayProcessing/myAP/ to gain 
 * an understanding of these routines.  There is not much doc yet.
 */

/* Simple memory-map constants */

extern int DSPAPGetLowestAddress(void);
extern int DSPAPGetHighestAddress(void);
extern int DSPAPGetHighestXAddressXY(void);
extern int DSPAPGetHighestYAddressXY(void);
extern int DSPAPGetLowestAddressXY(void);
extern int DSPAPGetHighestAddressXY(void);


extern int DSPAPLoadAddress(void); 
/*
 * Return start address of user code in DSP program memory.
 * The address is the start of user memory plus the init code
 * used to set up user execution.  Currently, the init code only
 * resets the memory argument pointer R_X to its starting value.
 * The DSP is currently assumed to have been reset since its last use.
 */


extern int DSPAPBoot(void);
/* 
 * Open and reboot the DSP with the Array Processing monitor.
 * Called by DSPAPInit().
 */


extern int DSPAPInit(void);
/* 
 * Open and reboot the DSP such that it is ready to receive an array
 * processing program.	Calls DSPAPBoot() to load the monitor and then
 * downloads the array processing main program preamble.
 */


extern int DSPAPFree(void); 
/*
 * Free the DSP.
 */


extern int DSPAPEnablePauseOnError(void);
/* 
 * If this function is called prior to DSPAPInit(), then
 * if an abort message is received from the DSP AP monitor, the program 
 * pauses in DSPAwaitNotBusy().  At this point, one would typically
 * run dspabort in another shell to move the DSP into a state compatible 
 * with the Bug56 "grab" button.  Note that when the process 
 * owning the DSP terminates, the DSP will is reset. This pause
 * enable is a convenient way to catch errors and get into the
 * debugger without a lot of return-code checking in your application.
 */


extern int DSPAPDisablePauseOnError(void);
/* 
 * If this function is called prior to DSPAPInit(), then
 * an error message from the DSP AP monitor results in an error
 * return from DSPAPAwaitNotBusy() instead of pausing.
 */


extern int DSPAPPauseOnErrorIsEnabled(void);
/* 
 * Returns true if hanging on DSP error is enabled.
 */


extern int DSPAPMacroInit(char *apfnfile, DSPLoadSpec **dspSPP);
/*
 * Load the AP binary file.
 *
 * DSPAPMacroInit - called to download a pre-assembled DSP AP macro.
 * Note: By offsetting the load address
 * for spaces P and X, the AP module can be stacked onto a prior
 * set in DSP memory.
 * The end-cap installed afterward should be installed after the
 * stack and not after each one.
 */


extern int DSPAPMacroLoad(char *fileName, 
		   DSPLoadSpec *dsp, 
		   int nArgs,
		   int *argValues);
/*
 * Load macro code, end cap, and arguments to the DSP.
 */


extern int DSPAPMacroGo(char *fileName);
/*
 * Start macro.
 */


extern int DSPAPAwaitNotBusy(int msTimeLimit);
/*
 * Wait for DSP message "DSP_DM_MAIN_DONE".
 * Returns 0 if the message is received within the time limit,
 * specified in milliseconds. A zero time limit means forever.
 * Called by AP main programs to await DSP execution.
 */


extern int DSPAPLoadGo(char *fileName, 	
		       DSPLoadSpec **dsp, 
		       int nArgs, 
		       int *argValues);
/*
 * Called by interface function to initialize, start, and await
 * execution of the AP module.
 */


/*****************************************************************************/
/*
 * The following are wrappers for the corresponding "DSP" versions.
 * They exist merely to avoid having to mix the prefixes "DSP" and "DSPAP".
 */

extern int DSPAPWriteFix24Array(
    DSPDatum *data,		/* array to send to DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */
/* 
 * Send an array of 24-bit words, right-justified in 32 bits, to the DSP, 
 * writing three bytes to each successive DSP word.  Uses 32-bit DMA mode.
 * The rightmost (least-significant) three bytes of each 32-bit source
 * word go to the corresponding DSP word.  The most significant byte of
 * each source word is ignored.
 *
 * The skip factor specifies the increment for the DSP address register
 * used in the DMA transfer.  A skip factor of 1 means write successive
 * words contiguously in DSP memory.  A skip factor of 2 means skip every
 * other DSP memory word, etc.
 *
 * This function is also used to transfer unpacked byte arrays or 
 * unpacked sound arrays to the DSP.  In these cases the data words
 * are right-justified in the 32-bit words of the source array.
 *
 */


extern int DSPAPWritePackedArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word */
    int wordCount);		/* DSP words = byte count / 3 */
/* 
 * Send a byte array to the DSP, writing three bytes to
 * each successive DSP word.  Uses 24-bit DMA mode.
 * This is the most compact form of transfer to the DSP.
 */


extern int DSPAPWriteShortArray(
    short *data,		/* Packed short data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short */
    int wordCount);		/* DSP word count = byte count / 2 */
/* 
 * Send a packed array of 16-bit words to the DSP (typically sound data).
 * Uses 16-bit DMA mode.  Each 32-bit word in the
 * source array provides two successive 16-bit samples in the DSP.
 * In the DSP, each 16-bit word is received right-justified in 24 bits,
 * with no sign extension.
 */


extern int DSPAPWriteByteArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount);		/* Total number of bytes to transfer */
/* 
 * Send a packed array of 8-bit words to the DSP (typically microphone data).
 * Uses 8-bit DMA mode.	 Each 32-bit word in the
 * source array provides four successive 8-bit samples to the DSP,
 * right-justified within 24 bits without sign extension.
 */


extern int DSPAPCheckWriteAddressesXY(int memorySpace, 
				      int startAddress,
				      int skipFactor, 
				      int wordCount);


extern int DSPAPWriteIntArray(int *intArray, 
			      int startAddress, 
			      int skipFactor,
			      int wordCount);


extern int DSPAPCheckWriteAddresses(int startAddress, 
				    int skipFactor, 
				    int wordCount);



extern int DSPAPWriteIntArrayXY(int *intArray, 
				int startAddress, 
				int skipFactor,
				int wordCount);


extern int DSPAPWriteFloatArray(float *floatArray, 
				int startAddress, 
				int skipFactor,
				int wordCount);


extern int DSPAPWriteFloatArrayXY(float *floatArray, 
				  int startAddress,
				  int skipFactor, 
				  int wordCount);


extern int DSPAPWriteDoubleArray(double *doubleArray, 
				 int startAddress,
				 int skipFactor,
				 int wordCount);


extern int DSPAPWriteDoubleArrayXY(double *doubleArray,
				   int startAddress, 
				   int skipFactor, 
				   int wordCount);


extern int DSPAPReadArray(
    DSPDatum *data,		/* array to fill from DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */


extern int DSPAPReadFix24Array(
    DSPDatum *data,		/* array to fill from DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */


extern int DSPAPReadPackedArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word */
    int wordCount);		/* DSP words = byte count / 3 */


extern int DSPAPReadByteArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount);		/* Same as DSP word count */


extern int DSPAPReadShortArray(
    short *data,		/* Packed data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short transferred */
    int wordCount);		/* DSP word count = byte count / 2 */


extern int DSPAPReadIntArray(
    int *intArray,
    int startAddress,
    int skipFactor,
    int wordCount);
/*
 * Same as DSPReadFix24Array() followed by DSPFix24ToIntArray() 
 * for sign extension.
 */


extern int DSPAPReadIntArrayXY(
    int *intArray,
    int startAddress,
    int skipFactor,
    int wordCount);
/*
 * Same as DSPReadFix24Array() followed by DSPFix24ToIntArray() 
 * for sign extension.
 */


extern int DSPAPReadFloatArray(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount);


extern int DSPAPReadFloatArrayXY(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount);


extern int DSPAPReadDoubleArray(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount);


extern int DSPAPReadDoubleArrayXY(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount);
