/* DSPControl.h - Functions in libdsp_s.a having to do with DSP control.
 * Copyright 1989,1990, by NeXT, Inc. 
 * J.O. Smith
 */

extern int DSPInit(void);
/* 
 * Initialize the DSP with the minimal generic DSP monitor (currently the
 * array processing DSP monitor).
 */


extern int DSPMKInit(void);
/* 
 * Open and reset the DSP such that it is ready to receive a user 
 * orchestra program.  It is the same as DSPBoot(musicKitSystem)
 * followed by starting up sound-out or write-data, if enabled.
 * Also differs from DSPInit() in that "Host Message" protocol is enabled.
 * This protocol implies that DSP messages (any word sent by the DSP
 * to the host outside of a data transfer is a DSP message) with the
 * high-order bit on are error messages, and they are routed to an
 * error port separate from the DSP message port.  Only the Music Kit
 * currently uses this protocol.
 */


extern int DSPMKInitWithSoundOut(int lowSamplingRate);
/* 
 * Open and reset the DSP such that it is ready to receive a user 
 * orchestra program.  Also set up link from DSP to sound-out.
 * If lowSamplingRate is TRUE, the sound-output sampling rate is set
 * to 22KHz, otherwise it is set to 44KHz.
 */


extern int DSPSetStart(DSPAddress startAddress);
/*
 * Set default DSP start address for user program.
 */


extern int DSPStart(void);
/*
 * Initiate execution of currently loaded DSP user program at current
 * default DSP start address.
 */


extern int DSPStartAtAddress(DSPAddress startAddress);
/*
 * Equivalent to DSPSetStart(startAddress) followed by DSPStart().
 */


extern int DSPPingVersionTimeOut(
    int *verrevP,
    int msTimeLimit);
/* 
 * Like DSPPingVersion but allowing specification of a time-out in
 * milliseconds.
 */


extern int DSPPingVersion(int *verrevP);
/* 
 * "Ping" the DSP.  The DSP responds with an "I am alive" message
 * containing the system version and revision.
 * Returns 0 if this reply is received, nonzero otherwise.
 * (version<<8 | revision) is returned in *verrevP.
 */


extern int DSPPingTimeOut(int msTimeLimit);
/* 
 * Like DSPPing but allowing specification of a time-out in
 * milliseconds.
 */


extern int DSPPing(void);
/* 
 * "Ping" the DSP.  The DSP responds with an "I am alive" message.
 * Returns 0 if this reply is received, nonzero otherwise.
 */


extern int DSPCheckVersion(
    int *sysver,	   /* system version running on DSP (returned) */
    int *sysrev);	   /* system revision running on DSP (returned) */
/* 
 * "Ping" the DSP.  The DSP responds with an "I am alive" message
 * containing the DSP system version and revision as an argument.
 * For extra safety, two more messages are sent to the DSP asking for the
 * address boundaries of the DSP host-message dispatch table.  These
 * are compared to the values compiled into the program. 
 * A nonzero return value indicates a version mismatch or a hung DSP.
 * The exact nature of the error will appear in the error log file, if enabled.
 */


extern int DSPIsAlive(void);
/*
 * Ask DSP monitor if it's alive, ignoring system version and revision.
 * "Alive" means anything but hung.  System version compatibility is
 * immaterial.	Use DSPCheckVersion() to check for compatibility between 
 * the loaded DSP system and your compilation.
 */


extern int DSPMKIsAlive(void);
/*
 * Ask DSP monitor if it's alive, and if it's the Music Kit monitor.
 */


extern int DSPMKFreezeOrchestra(void);
/*
 * Place the DSP orchestra into the "frozen" state.  The orchestra loop enters
 * this state when it finishes computing the current "tick" of sound and jumps
 * back to the loop top.  It busy-waits there until DSPMKThawOrchestra() is
 * called.  In the frozen state, DSP device interrupts remain enabled, but no
 * new sound is computed.  Thus, if sound-out is flowing, it will soon
 * under-run.
 */


extern int DSPMKThawOrchestra(void);
/*
 * Release the DSP orchestra from the frozen state.
 */


extern int DSPMKPauseOrchestra(void);
/*
 * Place the DSP orchestra into the paused state.  In this type of pause, the
 * orchestra loop continues to run, emitting sound, but time does not advance.
 * Thus, a better name would be DSPMKPauseTimedMessages().
 */


extern int DSPMKResumeOrchestra(void);
/*
 * Release the DSP orchestra from the paused state.
 */


extern int DSPSetDMAReadMReg(DSPAddress M);
/* 
 * Set the M index register used in DMA reads from DSP to host to M.
 * The default is M = -1 which means linear addressing.
 * The value M = 0 implies bit-reverse addressing, and
 * positive M is one less than the size of the modulo buffer used.
 */


extern int DSPSetDMAWriteMReg(DSPAddress M);
/* 
 * Set the M index register used in DMA writes from host to DSP to M.
 * The default is M = -1 which means linear addressing.
 * The value M = 0 implies bit-reverse addressing, and
 * positive M is one less than the size of the modulo buffer used.
 */


extern int DSPAbort(void);
/* 
 * Tell the DSP to abort.
 */


/******************************** TIMED CONTROL ******************************/
/* 
   Timed messages are used by the music kit.  Time is maintained in the DSP.
   The current time (in samples) is incremented by the tick size DSPMK_I_NTICK
   once each iteration of the orchestra loop on the DSP.  When the orchestra
   loop is initially loaded and started, the time increment is zero so that
   time does not advance.  This is the "paused" state for the DSP orchestra
   (to be distinguished from the "frozen" state in which everything suspends).

*/

extern int DSPMKSetTime(DSPFix48 *aTimeStampP);
/*
 * Set DSP sample time to that contained in *aTimeStampP.
 */


extern int DSPMKClearTime(void);
/*
 * Set DSP sample time to zero.
 */


extern int DSPMKReadTime(DSPFix48 *dspTime);
/*
 * Read DSP sample time.
 */


extern DSPFix48 *DSPMKGetTime(void);
/*
 * Read DSP sample time.  Returns NULL on error instead of error code.
 */


extern int DSPMKEnableAtomicTimed(DSPFix48 *aTimeStampP);
/* 
 * Tell the DSP to begin an atomic block of timed messages.
 */


extern int DSPMKDisableAtomicTimed(DSPFix48 *aTimeStampP);
/* 
 * Terminate an atomic block of timed messages in the DSP TMQ.
 */


extern int DSPMKPauseOrchestraTimed(DSPFix48 *aTimeStampP);
/*
 * Place the orchestra into the paused state at the requested DSP sample time.
 */
