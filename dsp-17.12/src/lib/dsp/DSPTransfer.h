/* DSPTransfer.h - Functions in libdsp_s.a having to do with data transfer.
 * Copyright 1989,1990, by NeXT, Inc. 
 * J.O. Smith
 */

#if 0

 TERMINOLOGY

    A "load" is an immediate transfer of a DSPLoadSpec struct into DSP memory.
    A load can occur any time after DSPBoot() or DSP{AP,MK,}Init().

    A "write" is an immediate transfer into DSP memory,
    normally used to download user data to the DSP.

    A "read" is the inverse of a "write".   The transfer is immediate,
    without regard for DSP time, if any. 

    A "get" is the same as a "read", except that the return value of the
    function contains the desired datum rather than an error code.

    A "vector" is a contiguous array of words.  A one-dimensional 
    (singly subscripted) C array is an example of a vector.

    An "array" is a not-necessarily-contiguous sequence of words.
    An array is specified by a vector plus a "skip factor".

    A "skip factor" is the number of array elements to advance in an 
    array transfer.  An array with a skip factor of 1 is equivalent to
    a vector.  A skip factor of 2 means take every other element when
    reading from the DSP and write every other element when writing to
    the DSP.  A skip factor of 3 means skip 2 elements between each
    read or write, and so on.

    ----------------------------------------------------------------------

    The following terms pertain primarily to the Music Kit DSP monitor:

    A "send" is a timed transfer into DSP memory.  Functions which do
    "sends" have prefix "DSPMKSend..." or have the form "DSPMK...Timed()".

    A "ret{rieve}" is the inverse of a "send".	

    A "tick" is DSPMK_I_NTICK samples of digital audio produced by one
    iteration of the "orchestra loop" in the DSP.

    An "immediate send" is a send in which the time stamp is 0.	 The global
    variable DSPTimeStamp0 exists for specifying a zero time stamp.  It 
    results in a tick-synchronized write, i.e., occurring at end of
    current tick in the DSP.

    An orchestra must be running on the chip to do either type of send.	 
    Time-stamped DSP directives are always used in the context of the
    Music Kit orchestra.

    If the time stamp pointer is null (DSPMK_UNTIMED), then a send reduces 
    to a write.

    A "BLT" (BLock Transfer) is a move within DSP memory.
    A "BLTB" is a Backwards move within DSP memory.

    A "fill" specifies one value to use in filling DSP memory.

#endif


extern int DSPWriteValue(int value, DSPMemorySpace space, int addr);
/*
 * Write the low-order 24 bits of value to space:addr in DSP memory.
 * The space argument is one of (cf. <dsp/dsp_structs.h>):
 *	DSP_MS_X
 *	DSP_MS_Y
 *	DSP_MS_P
 */


extern int DSPWriteLong(DSPFix48 *aFix48Val, int addr);
/* 
 * Write a DSP double-precision value to l:addr in DSP memory.
 * Equivalent to two calls to DSPWriteValue() for the high-order
 * and low-order words.
 */


extern int DSPWriteFix24Array(
    DSPFix24 *data,		/* array to write to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */

/* 
 * Write an array of 24-bit words, right-justified in 32 bits, to the DSP, 
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
 * The write is done using 32-bit DMA mode if wordCount is
 * DSP_MIN_DMA_WRITE_SIZE or greater, programmed I/O otherwise.  Note 
 * that the DMA transfer is inherently left-justified, while programmed I/O 
 * is inherently right justified.  For large array transfers, it is more
 * efficient to work with left-justified data, as provided by
 * DSPWriteFix24ArrayLJ().
 *
 * This function is also used to transfer unpacked byte arrays or 
 * unpacked sound arrays to the DSP.  In these cases the data words
 * are right-justified in the 32-bit words of the source array.
 *
 * The memorySpace is one of (see dsp_structs.h):
 *	DSP_MS_X
 *	DSP_MS_Y
 *	DSP_MS_P
 */


extern int DSPWriteFix24ArrayLJ(
    DSPFix24 *data,		/* array to write to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */
/*
 * Same as DSPWriteFix24Array except that the data array is assumed to be 
 * left-justified in 32 bits.
 */


extern int DSPWriteIntArray(
    int *intArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount);
/*
 * Same as DSPWriteFix24Array.  The low-order 24 bits of each int are
 * transferred into each DSP word.
 */


extern int DSPWritePackedArray(
    unsigned char *data,	/* Data to write to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word written */
    int wordCount);		/* DSP words = byte count / 3 */

/* 
 * Write a byte array to the DSP, writing three bytes to
 * each successive DSP word.  Uses 24-bit DMA mode.
 * This is the most compact form of transfer to the DSP.
 */


extern int DSPWriteShortArray(
    short int *data,		/* Packed short data to write to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short written */
    int wordCount);		/* DSP word count = byte count / 2 */

/* 
 * Write a packed array of 16-bit words to the DSP (typically sound data).
 * Uses 16-bit DMA mode.  Each 32-bit word in the
 * source array provides two successive 16-bit samples in the DSP.
 * In the DSP, each 16-bit word is received right-justified in 24 bits,
 * with no sign extension.
 */


extern int DSPWriteByteArray(
    unsigned char *data,	/* Data to write to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount);		/* Total number of bytes to transfer */

/* 
 * Write a packed array of 8-bit words to the DSP (typically microphone data).
 * Uses 8-bit DMA mode.	 Each 32-bit word in the
 * source array provides four successive 8-bit samples to the DSP,
 * right-justified within 24 bits without sign extension.
 * In the DSP, each byte is received right-justified in 24 bits.
 */


extern int DSPWriteFloatArray(
    float *floatArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount);
/*
 * Write a vector of floating-point numbers to a DSP array.
 * Equivalent to DSPFloatToFix24Array() followed by DSPWriteFix24Array().
 */


extern int DSPWriteDoubleArray(
    double *doubleArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount);
/*
 * Write a vector of double-precision floating-point numbers to a DSP array.
 * Equivalent to DSPDoubleToFix24Array() followed by DSPWriteFix24Array().
 */


extern int DSPDataRecordLoad(DSPDataRecord *dr); 
/* 
 * Load data record (as filled from assembler's _DATA record) into DSP.
 * See <dsp/dsp_structs.h> for the struct format.
 */


/* Music Kit versions: timed data transfers to DSP */

extern int DSPMKSendValue(int value, DSPMemorySpace space, int addr);
/*
 * Equivalent to DSPWriteValue() except synchronized to a tick boundary
 * (i.e., executed at the top of the orchestra loop).
 * Equivalent to DSPMKSendValueTimed(DSPMKTimeStamp0,value,space,addr).
 */


extern int DSPMKSendValueTimed(DSPFix48 *aTimeStampP,
			       int value,
			       DSPMemorySpace space,
			       int addr);
/*
 * Set a DSP memory location to a particular value at a particular time.
 */


extern int DSPMKSendLong(DSPFix48 *aFix48Val, int addr);
/*
 * etc.
 */


extern int DSPMKSendLongTimed(DSPFix48 *aTimeStampP, 
			      DSPFix48 *aFix48Val,
			      int addr);


extern int DSPMKSendArraySkipModeTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    DSPMemorySpace space,
    DSPAddress address,
    int skipFactor,
    int count,			/* DSP wordcount */
    int mode);			/* from <nextdev/dspvar.h> */
/*
 * Send an array of data to the DSP at a particular time.
 * The array is broken down into chunks which will fit into the Music Kit
 * DSP monitor's Host Message Stack, and as many timed messages as necessary
 * are sent to transfer the array. 
 *
 * See DSPObject.h, function DSPWriteArraySkipMode() for a description of
 * the various data modes and how they work.
 *
 * When this function is called, timed messages are flushed, and the
 * array transfers are not optimized.  That is, there is no command
 * stream optimization for timed array transfers as there is for
 * other timed host messages.  This means that multiple timed array transfers
 * going out at the same time will be transferred separately rather than being
 * batched.  If the arrays are so small and numerous that this optimization
 * seems warranted, use DSPMKSendValueTimed() instead.
 *
 * This function and its derivatives are intended for timed one-shot transfers
 * such as downloading oscillator wavetables.  DMA is not used, and the entire
 * array is held in the Timed Message Queue within the DSP until the
 * transfer time according to the DSP sample clock arrives.
 * For continuous data transfers into a DSP orchestra, use the "read data"
 * feature in the Music Kit.  The read-data stream can be stopped and
 * started at particular times if desired.
 */


extern int DSPMKSendArraySkipTimed(DSPFix48 *aTimeStampP,
				   DSPFix24 *data,
				   DSPMemorySpace space,
				   DSPAddress address,
				   int skipFactor,
				   int count);
/*
 * Calls DSPMKSendArraySkipModeTimed() with mode == DSP_MODE32.
 */


extern int DSPMKSendArrayTimed(DSPFix48 *aTimeStampP, 
			       DSPFix24 *data,
			       DSPMemorySpace space,
			       DSPAddress address,
			       int count);
/*
 * Calls DSPMKSendArraySkipTimed() with skipFactor equal to 1.
 */


extern int DSPMKSendArray(DSPFix24 *data,
			  DSPMemorySpace space,
			  DSPAddress address,
			  int count);
/*
 * Calls DSPMKSendArrayTimed() with skipFactor == 1 and time stamp == 0.
 */


extern int DSPMKSendShortArraySkipTimed(DSPFix48 *aTimeStampP,
    short int *data,
    DSPMemorySpace space,
    DSPAddress address,
    int skipFactor,
    int count);
/*
 * Calls DSPMKSendArraySkipModeTimed() with mode == DSP_MODE16.
 * Two successive DSP words get left and right 16 bits of each data word.
 * The 16-bit words are received right-justified in each DSP word.
 */


/****************************** DSP MEMORY FILLS *****************************/

/*
 * DSP "memory fills" tell the DSP to rapidly initialize a block of
 * the DSP's private static RAM.
 */

extern int DSPMemoryFill(
    DSPFix24 fillConstant,	/* value to use as DSP memory initializer */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* first address within DSP memory to fill */
    int wordCount);		/* number of DSP words to initialize */
/*
 * Set a block of DSP private RAM to the given fillConstant.
 * The memorySpace is one of (see dsp_structs.h):
 *
 *	DSP_MS_X
 *	DSP_MS_Y
 *	DSP_MS_P
 *
 * corresponding to the three memory spaces within the DSP.
 * The wordCount is in DSP words.  The least-significant 24-bits
 * of the fillConstant are copied into wordCount DSP words, beginning
 * with location startAddress.
 */


extern int DSPMKSendMemoryFill(
    DSPFix24 fillConstant,	/* value to fill memory with */
    DSPMemorySpace space,	/* space of memory fill in DSP */
    DSPAddress address,		/* first address of fill in DSP memory	*/
    int count);			/* number of DSP memory words to fill */
/*
 * Fill DSP memory block space:address#count with given fillConstant.
 * Synchronized to tick boundary.
 */


extern int DSPMKMemoryFillTimed(
    DSPFix48 *aTimeStampP,	/* time to do memory fill in the DSP */
    DSPFix24 fillConstant,
    DSPMemorySpace space,
    DSPAddress address,
    int count);
/*
 * Fill DSP memory block space:address#count with given fillConstant
 * at specified time.
 */


extern int DSPMKMemoryFillSkipTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 fillConstant,
    DSPMemorySpace space,
    DSPAddress address,
    int skip,			/* skip factor in DSP memory */
    int count);
/*
 * Fill DSP memory block space:address+skip*i, i=0 to count-1
 * with given fillConstant at specified time.
 */


extern int DSPMemoryClear(DSPMemorySpace memorySpace,
			  DSPAddress startAddress,
			  int wordCount);
/*
 * Set a block of DSP private RAM to zero.
 * Equivalent to DSPMemoryFill(0,memorySpace,startAddress,wordCount);
 */

extern int DSPMKSendMemoryClear(DSPMemorySpace space,
				DSPAddress address,
				int count);

extern int DSPMKMemoryClearTimed(DSPFix48 *aTimeStampP, 
				 DSPMemorySpace space,
				 DSPAddress address,
				 int count);

/************************* POKING ONCHIP DSP SYMBOLS ************************/


extern DSPSymbol *DSPGetSymbol(char *name, DSPSection *sec);
/*
 * Find symbol within the given DSPSection with the given name. 
 * See <dsp/dsp_structs.h> for the definition of a DSPSection.
 * Equivalent to trying DSPGetSymbolInLC() for each of the 12
 * DSP location counters.
 */


extern DSPSymbol *DSPGetSymbolInLC(char *name, DSPSection *sec, 
				   DSPLocationCounter lc);
/*
 * Find symbol within the given DSPSection and location counter
 * with the given name.  See <dsp/dsp_structs.h> for an lc list.
 */


extern int DSPSymbolIsFloat(DSPSymbol *sym);
/* 
 * Returns TRUE if the DSP assembler symbol is type 'F'.
 */


extern int DSPGetSymbolAddress(
    DSPMemorySpace *spacep,
    DSPAddress *addressp,
    char *name,			/* name of asm56000 asymbol */
    DSPSection *sec);
/*
 * Returns the space and address of symbol with the given name in the 
 * given DSP section.
 */


extern int DSPPoke(char *name, DSPFix24 value, DSPLoadSpec *dsp);
/*
 * Set the value of the DSP symbol with the given name to value (in the DSP).
 */


extern int DSPPokeFloat(char *name, float value, DSPLoadSpec *dsp);
/*
 * Equivalent to DSPPoke(name, DSPFloatToFix24(value), dsp).
 */


/************************** TRANSFERS FROM THE DSP ***************************/

/* 
 * These "from" routines are analogous to the corresponding "to" routines
 * above.  Hence, they are generally not documented when exactly analogous.
 */


extern int DSPMKRetValueTimed(
    DSPTimeStamp *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value);
/*
 * Send a timed peek.  Since we do not know the current time within the
 * DSP, we wait forever for the returned value from the DSP.
 */


extern int DSPMKRetValue(DSPMemorySpace space, 
			 DSPAddress address, 
			 DSPFix24 *value);

extern int DSPReadValue(DSPMemorySpace space,
			DSPAddress address,
			DSPFix24 *value);

DSPFix24 DSPGetValue(DSPMemorySpace space, DSPAddress address);
/*
 * Get DSP memory datum at space:address.
 */


extern int DSPReadFix24Array(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */
/* 
 * Read an array of 24-bit words, right-justified in 32 bits, to the DSP, 
 * reading three bytes to each successive DSP word.
 * The rightmost (least-significant) three bytes of each 32-bit source
 * word go to the corresponding DSP word.  The most significant byte of
 * each source word is ignored.
 *
 * The skip factor specifies the increment for the DSP address register
 * used in the DMA transfer.  A skip factor of 1 means write successive
 * words contiguously in DSP memory.  A skip factor of 2 means skip every
 * other DSP memory word, etc.
 * 
 * The read is done using 32-bit DMA mode if wordCount is
 * DSP_MIN_DMA_READ_SIZE or greater, programmed I/O otherwise.  Note 
 * that DMA transfers are inherently left-justified, while programmed I/O is
 * inherently right justified.  For large array transfers, it is more
 * efficient to work with left-justified data, as provided by
 * DSPReadFix24ArrayLJ().
 * 
 * This function is also used to transfer unpacked byte arrays or 
 * unpacked sound arrays to the DSP.  In these cases the data words
 * are right-justified in the 32-bit words of the source array.
 *
 * The memorySpace is one of (see dsp_structs.h):
 *	DSP_MS_X
 *	DSP_MS_Y
 *	DSP_MS_P
 */


extern int DSPReadFix24ArrayLJ(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount);		/* from DSP perspective */
/*
 * Same as DSPReadFix24Array() except that data is returned 
 * left-justified in 32 bits.
 */


extern int DSPReadIntArray(int *intArray,
			   DSPMemorySpace memorySpace,
			   DSPAddress startAddress,
			   int skipFactor,
			   int wordCount);
/*
 * Same as DSPReadFix24Array() followed by DSPFix24ToIntArray() for 
 * sign extension.
 */


extern int DSPReadPackedArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word read */
    int wordCount);		/* DSP words = byte count / 3 */

extern int DSPReadShortArray(
    short int *data,		/* Packed data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per array element */
    int wordCount);		/* DSP word count = byte count / 2 */

extern int DSPReadByteArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount);		/* Same as DSP word count */

extern int DSPReadFloatArray(float *floatArray,
			     DSPMemorySpace memorySpace,
			     DSPAddress startAddress,
			     int skipFactor,
			     int wordCount);

extern int DSPReadDoubleArray(double *doubleArray,
			      DSPMemorySpace memorySpace,
			      DSPAddress startAddress,
			      int skipFactor,
			      int wordCount);

/************************** TRANSFERS WITHIN THE DSP *************************/

extern int DSPMKBLT(DSPMemorySpace memorySpace,
		    DSPAddress sourceAddr,
		    DSPAddress destinationAddr,
		    int wordCount);

extern int DSPMKBLTB(DSPMemorySpace memorySpace,
		     DSPAddress sourceAddr,
		     DSPAddress destinationAddr,
		     int wordCount);

extern int DSPMKBLTSkipTimed(DSPFix48 *timeStamp,
			     DSPMemorySpace memorySpace,
			     DSPAddress srcAddr,
			     DSPFix24 srcSkip,
			     DSPAddress dstAddr,
			     DSPFix24 dstSkip,
			     DSPFix24 wordCount);

extern int DSPMKBLTTimed(DSPFix48 *timeStamp,
			 DSPMemorySpace memorySpace,
			 DSPAddress sourceAddr,
			 DSPAddress destinationAddr,
			 DSPFix24 wordCount);

extern int DSPMKBLTBTimed(DSPFix48 *timeStamp,
			  DSPMemorySpace memorySpace,
			  DSPAddress sourceAddr,
			  DSPAddress destinationAddr,
			  DSPFix24 wordCount);

extern int DSPMKSendBLT(DSPMemorySpace memorySpace,
			DSPAddress sourceAddr,
			DSPAddress destinationAddr,
			DSPFix24 wordCount);

extern int DSPMKSendBLTB(DSPMemorySpace memorySpace,
			 DSPAddress sourceAddr,
			 DSPAddress destinationAddr,
			 DSPFix24 wordCount);


/******************** GETTING DSP MEMORY ADDRESSES **************************/

/*
 * The DSP memory addresses are obtained directly from the DSPLoadSpec
 * struct registered by DSPBoot() as the currently loaded DSP system.
 * The memory boundary symbol names must follow the conventions used
 * in the Music Kit and array processing DSP monitors.  
 * /usr/include/dsp/dsp_memory_map_*.h for a description of the name
 * convention, and see /usr/lib/dsp/monitor/apmon8k.lod for an example
 * system file which properly defines the address-boundary symbols.
 *
 * Note that the DSP symbols relied upon below constitute the set of
 * symbols any new DSP monitor should export for compatibility with libdsp.
 */

extern int DSPGetValueOfSystemSymbol(char *name);
/*
 * Returns the value of the symbol "name" in the DSP system image, or -1 if
 * the symbol is not found or the DSP is not opened.  The requested symbol
 * is assumed to be type "I" and residing in the GLOBAL section under location
 * counter "DSP_LC_N" (i.e., no memory space is associated with the symbol).
 * No fixups are performed, i.e., the symbol is assumed to belong to an 
 * absolute (non-relocatable) section.  See /usr/lib/dsp/monitor/apmon8k.lod 
 * and mkmon8k.lod for example system files which compatibly define system 
 * symbols.
 */

extern DSPAddress DSPGetLowestInternalUserXAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XLI_USR") */

extern DSPAddress DSPGetHighestInternalUserXAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XHI_USR") */

extern DSPAddress DSPGetLowestInternalUserYAddress(void);
/* Returns DSPGetValueOfSystemSymbol("YLI_USR") */

extern DSPAddress DSPGetHighestInternalUserYAddress(void);
/* Returns DSPGetValueOfSystemSymbol("YHI_USR") */

extern DSPAddress DSPGetLowestInternalUserPAddress(void);
/* Returns DSPGetValueOfSystemSymbol("PLI_USR") */

extern DSPAddress DSPGetHighestInternalUserPAddress(void);
/* Returns DSPGetValueOfSystemSymbol("PHI_USR") */

extern DSPAddress DSPGetLowestExternalUserXAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XLE_USR") */

extern DSPAddress DSPGetHighestExternalUserXAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XHE_USR") */

extern DSPAddress DSPGetLowestExternalUserYAddress(void);
/* Returns DSPGetValueOfSystemSymbol("YLE_USR") */

extern DSPAddress DSPGetHighestExternalUserYAddress(void);
/* Returns DSPGetValueOfSystemSymbol("YHE_USR") */

extern DSPAddress DSPGetLowestExternalUserPAddress(void);
/* Returns DSPGetValueOfSystemSymbol("PLE_USR") */

extern DSPAddress DSPGetHighestExternalUserPAddress(void);
/* Returns DSPGetValueOfSystemSymbol("PHE_USR") */

extern DSPAddress DSPGetHighestExternalUserAddress(void);
/* Returns DSPGetValueOfSystemSymbol("HE_USR") */

extern DSPAddress DSPGetLowestExternalUserAddress(void);
/* Returns DSPGetValueOfSystemSymbol("LE_USR") */

DSPAddress DSPGetLowestXYPartitionUserAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XLE_USG") */

DSPAddress DSPGetHighestXYPartitionXUserAddress(void);
/* Returns DSPGetValueOfSystemSymbol("XHE_USG") */

DSPAddress DSPGetHighestXYPartitionYUserAddress(void);
/* Returns DSPGetValueOfSystemSymbol("YHE_USG") */

DSPAddress DSPGetHighestXYPartitionUserAddress(void);
/* Returns MIN(DSPGetHighestXYPartitionXUserAddress(), 
 *	       DSPGetHighestXYPartitionYUserAddress());
 */

extern DSPAddress DSPGetLowestDegMonAddress(void);
/* Returns DSPGetValueOfSystemSymbol("DEGMON_L") */

extern DSPAddress DSPGetHighestDegMonAddress(void);
/* Returns DSPGetValueOfSystemSymbol("DEGMON_H") */

extern DSPAddress DSPMKGetClipCountAddress(void);
/* 
 * Returns DSPGetValueOfSystemSymbol("X_NCLIP").  This is the address of the
 * location in DSP X memory used to store a cumulative count of "clips". A
 * "clip" occurs when an oversized value is moved from the DSP ALU to DSP
 * memory.  The clip value is reset to zero when the DSP is rebooted.
 * A standard usage is to do a timed peek on this location at the end of
 * a performance (using DSPMKRetValueTimed()).
 */

