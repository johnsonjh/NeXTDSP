/* Private functions in DSPObject.c */

int _DSPSetNumber(int i);
/* 
 * Set assigned DSP number for this instance.  Called by the new method.
 */


int _DSPAwaitMsgSendAck(msg_header_t *msg);
/*
 * Read ack message sent to msg->local_port by Mach kernel in response to a 
 * msg_snd. Returns 0 if all is well.
 */


int _DSPAwaitRegs(
    int mask,		/* mask to block on as bits in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 as desired for each 1 mask bit */
    int msTimeLimit);	/* time limit in milliseconds */
/*
 * Block until the specified mask is true in the DSP host interface.
 * Example conditions are (cf. <nextdev/snd_dspreg.h>):
 *  mask=DSP_CVR_REGS_MASK, value=0
 *	Wait for HC bit of DSP host interface to clear
 *  mask=value=DSP_ISR_HF2_REGS_MASK,
 * 	Wait for HF2 bit of DSP host interface to set
 */


int _DSPAwaitBit(
    int bit,		/* bit to block on as bit in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 */
    int msTimeLimit);	/* time limit in milliseconds */
/*
 * Block until the specified bit is true in the DSP host interface.
 * Example conditions are (cf. <nextdev/snd_dspreg.h>):
 *
 * 	bit		value
 * 	--- 		-----
 * DSP_CVR_REGS_MASK	  0	Wait for HC bit of DSP host interface to clear
 * DSP_ISR_HF2_REGS_MASK  1 	Wait for HF2 bit of DSP host interface to set
 *
 */
int _DSPAwaitMsgSendAck(
    msg_header_t *msg);
/*
 * Read ack message sent to msg->local_port by Mach kernel in response to a 
 * msg_snd. Returns 0 if all is well.
 */


int _DSPAwaitRegs(
    int mask,		/* mask to block on as bits in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 as desired for each 1 mask bit */
    int msTimeLimit);	/* time limit in milliseconds */
/*
 * Block until the specified mask is true in the DSP host interface.
 * Example conditions are (cf. <nextdev/snd_dspreg.h>):
 *  mask=DSP_CVR_REGS_MASK, value=0
 *	Wait for HC bit of DSP host interface to clear
 *  mask=value=DSP_ISR_HF2_REGS_MASK,
 * 	Wait for HF2 bit of DSP host interface to set
 */


int _DSPAwaitBit(
    int bit,		/* bit to block on as bit in (ICR,CVR,ISR,IVR) */
    int value,		/* 1 or 0 */
    int msTimeLimit);	/* time limit in milliseconds */
/*
 * Block until the specified bit is true in the DSP host interface.
 * Example conditions are (cf. <nextdev/snd_dspreg.h>):
 *
 * 	bit		value
 * 	--- 		-----
 * DSP_CVR_REGS_MASK	  0	Wait for HC bit of DSP host interface to clear
 * DSP_ISR_HF2_REGS_MASK  1 	Wait for HF2 bit of DSP host interface to set
 *
 */


int _DSPReadDatum(DSPDatum *datumP);
/*
 * Read a single DSP message.
 * Returns nonzero if there is no more data.
 * *** NOTE *** This routine is private because it does not support
 * mapped mode.	 A routine DSPReadDatum() routine can be made which 
 * simply calls DSPReadRX().
 */


int _DSPReadData(DSPDatum *dataP, int *nP);
/*
 * Read back up to *nP DSP messages into array dataP.
 * On input,nP is the maximum number of DSP data words to be read.
 * On output,nP contains the number of DSP data words actually read.
 * Returns nonzero if *nP changes.
 * *** NOTE *** This routine is private because it does not support
 * mapped mode.	 A routine DSPReadData() routine can be made which 
 * simply calls DSPReadRXArray().
 */


int _DSPWriteData(DSPDatum *dataP, int ndata);
/*
 * Write ndata words to the DSP transmit (TX) registers.
 * Returns 0 for success, nonzero if the write fails.
 * *** NOTE *** This routine is private because it does not support
 * mapped mode.	 A routine DSPWriteData() can be made which 
 * simply calls DSPWriteTXArray().
 */


int _DSPWriteRegs(
    int mask,			/* bit mask in (ICR,CVR,ISR,IVR) longword */
    int value);			/* bit values in (ICR,CVR,ISR,IVR) longword */
/*
 * Set DSP host-interface bits to given value.
 * Returns 0 for success, nonzero on error.
 * Example:
 *	_DSPWriteRegs(DSP_ICR_HF0_REGS_MASK,DSP_ICR_HF0_REGS_MASK),
 * sets host flag 0 to 1 and 
 *	_DSPWriteRegs(DSP_ICR_HF0_REGS_MASK,0));
 * clears it.
 *
 * *** NOTE: This routine is private because it does not support mapped mode.
 */


int _DSPPutBit(
    int bit,			/* bit mask in (ICR,CVR,ISR,IVR) longword */
    int value);			/* 1 or 0 */
/*
 * Set DSP host-interface bit to given value.
 * Returns 0 for success, nonzero on error.
 * Example:
 *	_DSPPutBit(DSP_ICR_HF0_REGS_MASK,1),
 * sets host flag 0 to 1 and 
 *	_DSPPutBit(DSP_ICR_HF0_REGS_MASK,0));
 * clears it.
 *
 * *** NOTE: This routine is private because it does not support mapped mode.
 */


int _DSPSetBit(int bit);	/* bit mask in (ICR,CVR,ISR,IVR) longword */
/*
 * Set DSP host-interface bit.
 * Returns 0 for success, nonzero on error.
 * Example: "_DSPSetBit(DSP_ICR_HF0_REGS_MASK)" sets host flag 0.
 * *** NOTE: This routine is private because it does not support mapped mode.
 */


int _DSPClearBit(int bit);	/* bit mask in (ICR,CVR,ISR,IVR) longword */
/*
 * Clear DSP host-interface bit.
 * Returns 0 for success, nonzero on error.
 * Example: "_DSPSetBit(DSP_ICR_HF0_REGS_MASK)" sets host flag 0.
 * *** NOTE: This routine is private because it does not support mapped mode.
 */


int _DSPWriteDatum(DSPDatum datum);
/*
 * Write one word to the DSP transmit (TX) registers.
 * Returns 0 for success, nonzero if the write fails.
 * *** NOTE *** This routine is private because it does not support
 * mapped mode.	 A routine DSPWriteDatum() can be made which 
 * simply calls DSPWriteTX().
 */


int _DSPStartHmArray(void);
/*
 * Start host message by zeroing the host message buffer pointer hm_ptr.
 */


int _DSPExtendHmArray(DSPDatum *argArray, int nArgs);
/*
 * Add arguments to a host message (for the DSP).
 * Add nArgs elements from argArray to hm_array.
 */


int _DSPExtendHmArrayMode(DSPDatum *argArray, int nArgs, int mode);
/*
 * Add arguments to a host message (for the DSP).
 * Add nArgs elements from argArray to hm_array according to mode.
 * Mode codes are in <nextdev/dspvar.h> and discussed in 
 * DSPObject.h(DSPWriteArraySkipMode).
 */


int _DSPExtendHmArrayB(DSPDatum *argArray, int nArgs);
/*
 * Add nArgs elements from argArray to hm_array in reverse order.
 */


int _DSPFinishHmArray(DSPFix48 *aTimeStampP, DSPAddress opcode);
/*
 * Finish off host message by installing time stamp (if timed) and opcode.
 * Assumes host-message arguments have already been installed in hm_array via 
 * _DSPExtendHmArray().
 */


int _DSPWriteHm(void);
/*
 * Send host message struct to the DSP.
 */


int _DSPWriteHostMessage(int *hm_array, int nwords);
/*
 * Write host message array.
 * See DSPMessage.c for how this array is set up. (Called by _DSPWriteHm().)
 */


int _DSPResetTMQ(void);
/*
 * Reset TMQ buffers to empty state and reset "current time" to 0.
 * Any waiting timed messages in the buffer are lost.
 */


int _DSPFlushTMQ(void) ;
/*
 * Flush current buffer of accumulated timed host messages (all for the
 * same time).
 */


int _DSPOpenMapped(void);
/*
 * Open DSP in memory-mapped mode. 
 * No reset or boot is done.
 * DSPGetRegs() can be used to obtain a pointer to the DSP host interface.
 */


int _DSPEnableMappedOnly(void);
int _DSPDisableMappedOnly(void);
int _DSPMappedOnlyIsEnabled(void);


int _DSPCheckMappedMode(void) ;
/*
 * See if mapped mode would be a safe thing to do.
 */


int _DSPEnterMappedModeNoCheck(void) /* Don't call this directly! */;


int _DSPEnterMappedModeNoPing(void) ;
/*
 * Turn off DSP interrupts.
 */


int _DSPEnterMappedMode(void);
/*
 * Flush driver's DSP command queue and turn off DSP interrupts.
 */


int _DSPExitMappedMode(void);
/*
 * Flush driver's DSP command queue and turn off DSP interrupts.
 */


DSPRegs *_DSPGetRegs(void);

int _DSPMappedOnlyIsEnabled(void);
int _DSPEnableMappedArrayReads(void);
int _DSPDisableMappedArrayReads(void);
int _DSPEnableMappedArrayWrites(void);
int _DSPDisableMappedArrayWrites(void);
int _DSPEnableMappedArrayTransfers(void);
int _DSPDisableMappedArrayTransfers(void);

int _DSPEnableUncheckedMappedArrayTransfers(void);

int _DSPDisableUncheckedMappedArrayTransfers(void);

int _DSPMKStartWriteDataNoThread(void);
/*
 * Same as DSPMKStartWriteData() but using an untimed host message
 * to the DSP.
 */

int _DSPMKStartWriteDataNoThread(void);

int _DSPForceIdle(void);

int _DSPOwnershipIsJoint();
/*
 * Returns TRUE if DSP owner port is held by more than one task.
 */

/* Added 3/30/90/jos from DSPObject.h */

FILE *DSPMKGetWriteDataFP(void);
/* 
 * Get the file-pointer being used for DSP write-data.
 */

int DSPMKSetWriteDataFP(FILE *fp);
/* 
 * Set the file-pointer for DSP write-data to fp.
 * The file-pointer will clear and override any prior specification
 * of write-data filename using DSPMKSetWriteDataFile().
 */
