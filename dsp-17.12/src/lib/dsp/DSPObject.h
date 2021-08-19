/* DSPObject.h - Low level DSP access and control functions.
 * Copyright 1989,1990, by NeXT, Inc.
 * J.O. Smith.
 */

/*
 * This file is organized logically with respect to the DSPOpen*()
 * routines in that functions apearing before the open routines must be
 * called before the DSP is opened (or any time), and functions apearing 
 * after the open routines must be (or are typically) called after the 
 * DSP is opened.
 *
 * The functions which depend on the Music Kit DSP monitor have the
 * prefix "DSPMK".  The prefix "DSP" may either be independent of the
 * monitor used, or require the array processing monitor.  Generally,
 * functions with prefix "DSP are monitor-independent unless they involve
 * input/output to or from private DSP memory.  The DSP private RAM is
 * not available on the NeXT bus, so the monitor used must help out.
 * to upgrade your own DSP monitor to support these DSP functions, lift
 * out the I/O support in the array processing monitor.  The sources are
 * distributed online under /usr/lib/dsp/smsrc (system monitor sources).
 */

#import <stdio.h>

extern int DSPGetHostTime(void);
/*
 * Returns the time in microseconds since it was last called.
 */


/*********** Utilities global with respect to all DSP instances **************/


extern int DSPGetDSPCount(void);
/* 
 * Return number of DSPs in current system.
 */


extern int DSPSetCurrentDSP(int newidsp);
/* 
 * Set DSP number.  Calls to functions in this file will act on that DSP.
 * Release 1.0 supports only one DSP: "DSP 0".
 */


extern int DSPGetCurrentDSP(void);
/* 
 * Returns currently selected DSP number.
 */

/*************** Getting and setting "DSP instance variables" ****************/

/*
 * DSP "get" functions do not follow the convention of returning an error code.
 * Instead (because there can be no error), they return the requested value.
 * Each functions in this class has a name beginning with "DSPGet".
 */


extern int DSPGetMessagePriority(void);
/*
 * Return DSP Mach message priority:
 *
 *	 DSP_MSG_HIGH		0
 *	 DSP_MSG_MED		1
 *	 DSP_MSG_LOW		2
 *
 * Only medium and low priorities are used by user-initiated messages.
 * Normally, low priority should be used, and high priority messages
 * will bypass low priority messages enqueued in the driver.  Note,
 * however, that a multi-component message cannot be interrupted once it 
 * has begun.  The Music Kit uses low priority for all timed messages
 * and high priority for all untimed messages (so that untimed messages
 * may bypass any enqueued timed messages).
 *
 */


extern int DSPSetMessagePriority(int pri);
/*
 * Set DSP message priority for future messages sent to the kernel.
 * Can be called before or after DSP is opened.
 */


extern double DSPMKGetSamplingRate(void);
/*
 * Returns sampling rate assumed by DSP software in Hz.
 */


extern int DSPMKSetSamplingRate(double srate);
/*
 * Set sampling rate assumed by DSP software to rate in samples per
 * second (Hz).	 Note that only sampling rates 22050.0 and 44100.0 
 * are supported for real time digital audio output.  Use of other sampling
 * rates implies non-real-time processing or sound-out through the DSP serial
 * port to external hardware.
 */


/*********** Enable/Disable/Query for DSP open-state variables ************/

/* 
 * In general, the enable/disable functions must be called BEFORE the DSP
 * is "opened" via DSPInit(), DSPAPInit(), DSPMKInit(), DSPBoot(), or one of
 * the DSPOpen*() functions.  They have the effect of selecting various open
 * modes for the DSP.  The function which ultimately acts on them is
 * DSPOpenNoBoot() (which is called by the Init and Boot functions above).
 */


extern int DSPGetOpenPriority(void);
/*
 * Return DSP open priority.
 *	0 = low priority (default).
 *	1 = high priority (used by DSP debugger, for example)
 * If the open priority is high when DSPOpenNoBoot is called,
 * the open will proceed in spite of the DSP already being in use.
 * In this case, a new pointer to the DSP owner port is returned.
 * Typically, the task already in control of the DSP is frozen and
 * the newly opening task is a DSP debugger stepping in to look around.
 * Otherwise, the two tasks may confuse the DSP with interleaved commands.
 * Note that deallocating the owner port will give up ownership capability
 * for all owners.
 */


extern int DSPSetOpenPriority(int pri);
/*
 * Set DSP open priority.
 * The new priority has effect when DSPOpenNoBoot is next called.
 *	0 = low priority (default).
 *	1 = high priority (used by DSP debugger, for example)
 */


extern int DSPEnableHostMsg(void);
/* 
 * Enable DSP host message protocol.
 * This has the side effect that all unsolicited "DSP messages"
 * (writes from the DSP to the host) are split into two streams.
 * All 24-bit words from the DSP with the most significant bit set
 * are interpreted as error messages and split off to an error port.
 * On the host side, a thread is spawned which sits in msg_receive()
 * waiting for DSP error messages, and it forwards then to the DSP 
 * error log, if enabled.
 */


extern int DSPDisableHostMsg(void);
/* 
 * Disable DSP host message protocol.
 * All writes from the DSP come in on the "DSP message" port.
 * The "DSP errors" port will remain silent.
 */


extern int DSPHostMsgIsEnabled(void);
/* 
 * Return state of HostMsg enable flag.
 */


extern int DSPMKIsWithSoundOut(void);
/* 
 * Returns nonzero if the DSP is linked to sound out.
 */


extern int DSPMKEnableSoundOut(void);
/* 
 * Enable DSP linkage to sound out.
 * When DSP is next opened, it will be linked to sound out.
 */


extern int DSPMKDisableSoundOut(void);
/* 
 * Disable DSP linkage to sound out.
 * When DSP is next opened, it will not be linked to sound out.
 */


extern int DSPMKSoundOutIsEnabled(void);
/* 
 * Return state of SoundOut enable flag.
 */


/* Sound out to the serial port */

extern int DSPMKEnableSSISoundOut(void);
/* 
 * Enable DSP serial port sound out.
 * When DSP is next opened with a Music Kit DSP system, it will be 
 * configured to have SSI sound out.
 */


extern int DSPMKDisableSSISoundOut(void);
/* 
 * Disable DSP serial port sound out.
 * When DSP is next opened with a Music Kit DSP system, the SSI
 * port of the DSP will not be used.
 */


extern int DSPMKSSISoundOutIsEnabled(void);
/* 
 * Return state of serial port SoundOut enable flag.
 */


extern int DSPMKStartSSISoundOut(void);
/*
 * Tell DSP to send sound-out data to the SSI serial port in the DSP.
 * The DSP will block until the SSI port has read the current sound-out
 * buffer.  Sound-out to the SSI can occur simultaneously with sound-out
 * to the host; the DSP simply blocks until both have finished reading.
 */


extern int DSPMKStopSSISoundOut(void);
/*
 * Tell DSP not to send sound-out data to the SSI serial port.
 */


extern int DSPMKEnableSmallBuffers(void);
/* 
 * Enable use of small buffers for DSP sound-out.
 * This is something worth doing when real-time response
 * is desired.	Normally, the sound-out driver uses
 * four 8K byte buffers.  With small buffers enabled,
 * four 1K byte buffers are used.
 */


extern int DSPMKDisableSmallBuffers(void);
/* 
 * Disable use of small buffers for DSP sound-out.
 */


extern int DSPMKSmallBuffersIsEnabled(void);
/* 
 * Return true if small sound-out buffers are enabled.
 */


extern int DSPMKEnableBlockingOnTMQEmptyTimed(DSPFix48 *aTimeStampP);
/* 
 * Tell the DSP to block when the Timed Message Queue is empty.
 * This prevents the possibility of late score information.
 * It is necessary to call DSPMKDisableBlockingOnTMQEmptyTimed()
 * after the last time message is sent to the DSP to enable the 
 * computing of all sound after the time of the last message.
 */


extern int DSPMKDisableBlockingOnTMQEmptyTimed(DSPFix48 *aTimeStampP);
/* 
 * Tell the DSP to NOT block when the Timed Message Queue is empty.
 */

/****************** Getting and setting DSP system files *********************/

/*
 * Get/set various DSP system file names.
 * The default filenames are defined in dsp.h.
 * The "get" versions return an absolute path.
 * The "set" versions take a relative path.
 * The environment variable $DSP must be overwritten appropriately
 * before a "get" version will return what the "set" version set.
 * Or, one could place custom system files in the $DSP directory.
 */


/* Misc. routines for getting various directories. */
char *DSPGetDSPDirectory(void);    /* DSP_SYSTEM_DIRECTORY or $DSP if set */
char *DSPGetSystemDirectory(void); /* <DSPDirectory>/monitor */
char *DSPGetAPDirectory(void);	   /* <DSPDirectory>/DSP_AP_BIN_DIRECTORY */

extern int DSPSetSystem(DSPLoadSpec *system);
/* 
 * Set the DSP system image (called by DSPBoot.c) 
 * If the system name is "MKMON8K" or "APMON8K", then 
 * all system filenames (binary, link, and map) are set accordingly.
 */


char *DSPGetSystemBinaryFile(void);
/* 
 * Get system binary filename.
 * Used by DSPBoot.c.
 */


char *DSPGetSystemLinkFile(void);
/* 
 * Get system linkfile name.
 * Used by _DSPMakeMusicIncludeFiles.c.
 */


char *DSPGetSystemMapFile(void);
/* 
 * Get system linkfile name.
 * Used by _DSPRelocateUser.c.
 */


extern int DSPMonitorIsMK(void);
/*
 * Returns true if the currently set DSP system is "mkmon8k.dsp",
 * otherwise false.
 */


extern int DSPSetMKSystemFiles(void);
/*
 * Set the system binary, link, and map files to the MK world.
 */


extern int DSPMonitorIsAP(void);
/*
 * Returns true if the currently set DSP system is "apmon8k.dsp",
 * otherwise false.
 */


extern int DSPSetAPSystemFiles(void);
/*
 * Set the system binary, link, and map files to the AP world.
 */


/***************************** WriteData Setup *******************************/

/*
 * "Write data" is DSP sound data which is being recorded to disk.
 */


extern int DSPMKSetWriteDataFile(char *fn);
/* 
 * Set the file-name for DSP write-data to fn.
 */


char *DSPMKGetWriteDataFile(void);
/* 
 * Read the file-name being used for DSP write-data.
 */


extern int DSPMKEnableWriteData(void);
/* 
 * Enable DSP write data.
 * When DSP is next opened, stream ports will be set up between the
 * DSP, host, and sound-out such that write-data can be used.
 * 
 * After opening the DSP with DSPMKInit(), call DSPMKStartWriteDataTimed() 
 * (described below) to spawn the thread which reads DSP data to disk.
 *
 *  Bug: Audible sound-out is disabled during write data.
 */


extern int DSPMKDisableWriteData(void);
/* 
 * Disable DSP write data (default).
 */


extern int DSPMKWriteDataIsEnabled(void);
/* 
 * Return state of DSP write-data enable flag.
 */


extern int DSPMKWriteDataIsRunning(void);
/* 
 * Return nonzero if DSP write data thread is still running.
 */


/* 
 * The write-data time-out is the number of milliseconds to wait in 
 * msg_receive() for a write-data sound buffer from the DSP.
 * On time-out, it is assumed that the DSP is not sending any more
 * write-data, and the thread reading write data terminates.
 */


extern int DSPMKGetWriteDataTimeOut(void);
/* 
 * Get number of milliseconds to wait in msg_receive() for a sound buffer
 * from the DSP before giving up.
 */


extern int DSPMKSetWriteDataTimeOut(int to);
/* 
 * Set number of milliseconds to wait in msg_receive() for a sound buffer
 * from the DSP before giving up.  The default is 60 seconds.  It must be
 * made larger if the DSP program might take longer than this to compute
 * a single buffer of data, and it can be set shorter to enable faster
 * detection of a hung DSP program.
 */


extern int DSPMKSoundOutDMASize(void);
/*
 * Returns the size of single DMA transfer used for DSP sound-out
 * 16-bit words.  This may change at run time depending on whether
 * read-data is used or if a smaller buffer size is requested.
 */


extern int DSPMKClearDSPSoundOutBufferTimed(DSPTimeStamp *aTimeStamp);
/*
 * Clears the DSP's sound-out buffer.  Normally, this is unnecessary
 * because the DSP orchestra puts out zeros by default.
 */

/***************************** ReadData Setup *******************************/

/*
 * "Read data" is DSP sound data (stereo or mono) which is being read from 
 * disk and sent to a Music Kit Orchestra running on the DSP.
 */


extern int DSPMKEnableReadData(void);
/* 
 * Enable DSP read data.
 * When the DSP is next opened, a read-data stream to the DSP will be opened.
 */


extern int DSPMKDisableReadData(void);
/* 
 * Disable DSP read data (default).
 */


extern int DSPMKReadDataIsEnabled(void);
/* 
 * Return state of DSP read-data enable flag.
 */


extern int DSPMKSetReadDataFile(char *fn);
/* 
 * Set the read-data file-name to fn.
 */


/*********************** OPENING AND CLOSING THE DSP ***********************/

extern int DSPIsOpen(void);
/* 
 * Returns nonzero if the DSP is open.
 */


DSPLoadSpec *DSPGetSystemImage(void);
/* 
 * Get pointer to struct containing DSP load image installed by DSPBoot().
 * If no system has been loaded, NULL is returned.
 */


extern int DSPRawCloseSaveState(void);
/*
 * Close the DSP device without trying to clean up things in the DSP,
 * and without clearing the open state (so that a subsequent open
 * will be with the same modes).
 * This function is normally only called by DSPCloseSaveState, but it is nice
 * to have interactively from gdb when the DSP is wedged.
 */


extern int DSPRawClose(void);
/*
 * Close the DSP device without trying to clean up things in the DSP.
 * This function is normally only called by DSPClose, but it is nice
 * to have interactively from gdb when the DSP is known to be hosed.
 */


extern int DSPClose(void);
/*
 * Close the DSP device (if open). If sound-out DMA is in progress, 
 * it is first turned off which leaves the DSP in a better state.
 * Similarly, SSI sound-out from the DSP, if running, is halted.
 */


extern int DSPCloseSaveState(void);
/*
 * Same as DSPClose(), but retains all enabled features such that
 * a subsequent DSPBoot() or DSPOpenNoBoot() will come up in the same mode.
 * If sound-out DMA is in progress, it is first turned off.
 * If SSI sound-out is running, it is halted.
 */


extern int DSPOpenWhoFile(void);
/*
 * Open DSP "who file" (unless already open) and log PID and time of open.
 * This file is read to find out who has the DSP when an attempt to
 * access the DSP fails because another task has ownership of the DSP
 * device port.  The file is removed by DSPClose().  If a task is
 * killed before it can delete the who file, nothing bad will happen.
 * It will simply be overwritten by the next open.
 */


extern int DSPCloseWhoFile(void);
/*
 * Close and delete the DSP lock file.
 */


char *DSPGetOwnerString(void);
/*
 * Return string containing information about the current task owning
 * the DSP.  An example of the returned string is as follows:
 * 
 *	DSP opened in PID 351 by me on Sun Jun 18 17:50:46 1989
 *
 * The string is obtained from the file /tmp/dsp_lock and was written
 * when the DSP was initialized.  If the DSP is not in use, or if there
 * was a problem reading the lock file, NULL is returned.
 * The owner string is returned without a newline.
 */


extern int DSPOpenNoBootHighPriority(void);
/*
 * Open the DSP at highest priority.
 * This will normally only be called by a debugger trying to obtain
 * ownership of the DSP when another task has ownership already.
 */ 


extern int DSPOpenNoBoot(void);
/*
 * Open the DSP in the state implied by the DSP state variables.
 * If the open is successful or DSP is already open, 0 is returned.
 * After DSPOpenNoBoot, the DSP is open in the reset state awaiting
 * a bootstrap program download.  Normally, only DSPBoot or a debugger
 * will ever call this routine.	 More typically, DSPInit() is called 
 * to open and reboot the DSP.
 */


extern int DSPReset(void);
/* 
 * Reset the DSP.
 * The DSP must be open.
 * On return, the DSP should be awaiting a 512-word bootstrap program.
 */


/****************************** SoundOut Handling ****************************/

extern int DSPMKStartSoundOut(void);
/*
 * Tell the DSP to begin sending sound-out packets which were linked
 * to the sound-out hardware by calling DSPMKEnableSoundOut().
 * The DSP must have been initialized via DSPMKInit() already.
 */


extern int DSPMKStopSoundOut(void);
/*
 * Tell DSP to stop sending sound-out packets.
 */


/***************************** WriteData Handling ****************************/

extern int DSPMKStartWriteDataTimed(DSPTimeStamp *aTimeStampP);
/*
 * Tell the DSP to start sending sound-out requests to the DSP driver when a
 * buffer of sound-out data is ready in the DSP.  A thread is spawned which
 * blocks in msg_receive() until each record region is received, and the
 * buffers are written to disk in the file established by
 * DSPMKSetWriteDataFile().  A second effect of this function is that the 
 * DSP will now block until the driver reads each sound-out buffer.
 * This function must be called after the DSP is initialized by DSPMKInit().
 *
 * If sound-out is also requested (via DSPMKEnableSoundOut()), each buffer
 * will be played immediately after it is written to disk.  In this case,
 * there is no need to also call DSPMKStartSoundOut().
 *
 * Bug: Audible sound-out is disabled during write data.
 * Since this should be fixed in the future, do not call both 
 * DSPMKEnableSoundOut() and DSPMKEnableWriteData() if you want
 * the same behavior in future releases.
 */


extern int DSPMKStartWriteData(void);
/*
 * Equivalent to DSPMKStartWriteDataTimed(DSPMK_UNTIMED);
 */


extern int DSPMKGetWriteDataSampleCount(void);
/* 
 * Get number of samples written to disk since write-data was initialized.
 */


extern int DSPMKStopWriteDataTimed(DSPTimeStamp *aTimeStampP);
/*
 * Tell DSP not to generate write-data requests.
 * If write-data is going to disk, it does NOT tell the write-data thread
 * to exit.  This must be the case since only the DSP knows when to turn
 * off write-data. Call DSPMKStopWriteData() to halt the write-data thread,
 * or let it time-out and abort on its own, (cf. DSPMKSetWriteDataTimeOut()).
 * Note that as far as the DSP is concerned, there is no difference between
 * write-data and sound-out.  Thus, calling this function will also turn off
 * sound-out, if it was enabled, whether or not write data was specifically
 * enabled.  A byproduct of this function is that the DSP stops blocking 
 * until each sound-out buffer is read by the driver. The timed start/stop
 * write-data functions can be used to write out specific sections of
 * a Music Kit performance, running as fast as possible (silently, throwing
 * away sound output buffers) during intervals between the stop and start
 * times.
 */


extern int DSPMKStopWriteData(void);
/*
 * Same as DSPMKStopWriteDataTimed(aTimeStampP) but using an untimed
 * host message to the DSP.  Called by DSPMKStopSoundOut().
 * If write-data is going to disk, also tells write-data thread
 * to exit.  See DSPMKStopWriteDataTimed() above.
 */


extern int DSPMKRewindWriteData(void);
/*
 * Rewind write-data to beginning of file.
 * DSPMKStopWriteData() must have been called first to terminate
 * the thread which actively writes the file.
 * After this, write-data can be resumed by DSPMKStartWriteData{Timed}().
 */

extern int DSPMKCloseWriteDataFile(void);
/*
 * Close the write-data file.
 * DSPMKStopWriteData() is called automatically if write-data is running.
 * This function is called by DSPClose(), so it is normally not used
 * unless the file is needed as an input before the DSP is next closed and
 * reopened.
 */

/***************************** ReadData Handling ****************************/

extern int DSPMKStartReadDataTimed(DSPTimeStamp *aTimeStampP);
/*
 * Start read-data flowing from disk to the DSP.
 * This function must be called after the DSP is initialized by DSPMKInit()
 * with read-data enabled by DSPMKEnableReadData(), and with
 * the input disk file having been specified by DSPMKSetReadDataFile().
 * The first two buffers of read-data are sent to the DSP immediately,
 * and a timed message is sent to the DSP saying when to start consumption.
 * A thread is spawned which blocks in msg_send() until each buffer is taken.
 * The DSP will request buffer refills from the driver as needed.
 * There is no timeout as there is for write data because the thread knows 
 * how much data to expect from the file, and it waits forever trying to give 
 * buffers to the DSP.  
 *
 * A second effect of this function is that the DSP orchestra program will 
 * begin blocking on read-data underrun.  There are two read-data buffers in 
 * the DSP, so blocking tends not happen if the host is able to convey sound 
 * from the disk to the DSP in real time or better, on average.
 *
 * If aTimeStamp == DSPMK_UNTIMED, the read-data is started in the PAUSED
 * state.  A subsequent DSPMKResumeReadDataTimed() is necessary to tell the
 * DSP when to begin consuming the read data.  The time-stamp can be set to 
 * contain zero which means start read data immediately in the DSP.
 *
 * Note that DSPMKStartReadDataTimed(ts) is equivalent to
 * DSPMKStartReadDataPaused() followed by DSPMKResumeReadDataTimed(ts).
 */


extern int DSPMKStartReadDataPaused(void);
/*
 * Equivalent to DSPMKStartReadDataTimed(DSPMK_UNTIMED);
 */


extern int DSPMKStartReadData(void);
/*
 * Equivalent to DSPMKStartReadDataTimed(&DSPMKTimeStamp0);
 * Read-data starts in the DSP immediately.
 */


extern int DSPMKGetReadDataSampleCount(void);
/* 
 * Get number of samples sent to the DSP since read-data was started.
 */


extern int DSPMKPauseReadDataTimed(DSPTimeStamp *aTimeStampP); 
/* 
 * Tell the DSP to stop requesting read-data buffer refills at the
 * specified time.  When this happens, the read-data stream going from
 * disk to the DSP will block.
 * 
 * This function and its "resume" counterpart provide
 * a way to save disk space in the read-data file when there are stretches
 * of time in the Music Kit performance during which no read-data is needed.
 * Silence in the read-data file can be squeezed out, and the pause/resume
 * functions can be used to read in the sound sections only when needed.
 */


extern int DSPMKResumeReadDataTimed(DSPTimeStamp *aTimeStampP);
/* 
 * Tell the DSP to resume read-data at the specified time.
 */


extern int DSPMKReadDataIsRunning(void);
/* 
 * Return nonzero if DSP read data thread is still running.
 * "Paused" is considered a special case of "running" since the
 * thread which spools data to the DSP is still alive.
 */


extern int DSPMKStopReadData(void);
/*
 * Tell DSP to stop read-data consumption, if active, and tell the host
 * read-data thread to exit.  See also DSPMKPauseReadDataTimed() above.
 */


extern int DSPMKRewindReadData(void);
/*
 * Rewind read-data to beginning of file.
 * The read-data thread should be paused or stopped during this operation.
 */


extern int DSPMKSetReadDataBytePointer(int offset);
/*
 * Move read-data file pointer to given offset in bytes.
 * Returns file pointer in bytes from beginning of file after the seek
 * or -1 if an error occurs.
 * The read-data thread should be paused or stopped during this operation.
 */

extern int DSPMKIncrementReadDataBytePointer(int offset);
/*
 * Move read-data file pointer to given offset from current location in bytes.
 * Returns file pointer in bytes from beginning of file after the seek
 * or -1 if an error occurs.
 * The read-data thread should be paused or stopped during this operation.
 */


/********************** READING/WRITING DSP HOST FLAGS ***********************/

extern int DSPSetHF0(void);
/*
 * Set bit HF0 in the DSP host interface.
 * In the context of the music kit or array processing kit, HF0 is set 
 * by the driver to indicate that the host interface is initialized in DMA
 * mode.
 */


extern int DSPClearHF0(void);
/*
 * Clear bit HF0 in the DSP host interface.
 */


extern int DSPGetHF0(void);
/* 
 * Read state of HF0 flag of ICR in DSP host interface.
 */


extern int DSPSetHF1(void);
/*
 * Set bit HF1 in the DSP host interface.
 * In the context of the music kit or array processing kit, HF1 is not used.
 */


extern int DSPClearHF1(void);
/*
 * Clear bit HF1 in the DSP host interface.
 */


extern int DSPGetHF1(void);
/* 
 * Read state of HF1 flag of ICR in DSP host interface.
 */


extern int DSPGetHF2(void);
/*
 * Return nonzero if bit HF2 in the DSP host interface is set, otherwise FALSE.
 * In the context of the music kit or array processing kit, HF2 is set during 
 * the execution of a host message.
 */


extern int DSPGetHF3(void);
/*
 * Return nonzero if bit HF3 in the DSP host interface is set, otherwise FALSE.
 * HF3 set in the context of the music kit implies the Timed Message Queue
 * in the DSP is full.  For array processing, it means the AP program is still
 * executing on the DSP.
 */


extern int DSPGetHF2AndHF3(void);
/*
 * Return nonzero if bits HF2 and HF3 in the DSP host interface are set, 
 * otherwise FALSE.  The Music Kit and array processing monitors set
 * both bits to indicate that the DSP has aborted.
 */

/****************** READING/WRITING HOST-INTERFACE REGISTERS *****************/


extern int DSPReadICR(int *icrP);		
/* 
 * Read DSP Interrupt Control Register (ICR).
 * value returned in *icrP is 8 bits, right justified.
 */


extern int DSPGetICR(void);
/*
 * Return ICR register of the DSP host interface.
 */


extern int DSPReadCVR(int *cvrP);
/* 
 * Read DSP Command Vector Register (CVR).
 * value returned in *cvrP is 8 bits, right justified.
 */


extern int DSPGetCVR(void);
/*
 * Return CVR register of the DSP host interface.
 */


extern int DSPHostCommand(int cmd);
/*
 * Issue DSP "host command". The low-order 5 bits of cmd are sent.
 * There are 32 possible host commands, with 18 predefined by Motorola.
 */


extern int DSPReadISR(int *isrP);
/*
 * Read DSP Interrupt Status Register (ISR).
 * value returned in *isrP is 8 bits, right justified.
 */


extern int DSPGetISR(void);
/*
 * Return ISR register of the DSP host interface.
 */


extern int DSPReadRX(DSPFix24 *wordp);
/*
 * Read next word sent by DSP from DSP Receive Byte Registers.
 * Value returned in *wordp is 24 bits, right justified.
 * Return value is 0 for success, nonzero if there's nothing to read
 * after waiting DSPDefaultTimeLimit.  RXDF must be true
 * before the read will occur.  Note that it is an "error" for RXDF
 * not to be true.  Call DSPAwaitData(msTimeLimit) before calling
 * DSPReadRX() in order to await RXDF indefinitely.  The time-out used
 * here may change in the future, and may become infinity.
 */


extern int DSPGetRX(void);
/*
 * Return RX register of the DSP host interface.
 */


extern int DSPReadRXArray(DSPFix24 *data, int nwords);
/*
 * Read next nwords words sent by DSP from DSP Receive Byte Registers.
 * Return value is 0 for success, nonzero if an element could not be read
 * after trying for 10 seconds.	 If the read could not be complete, the
 * number of elements successfully read + 1 is returned.
 */


extern int DSPWriteTX(DSPFix24 word);
/*
 * Write word into DSP transmit byte registers.
 * Low-order 24 bits are written from word.
 */


extern int DSPWriteTXArray(
    DSPFix24 *data,
    int nwords);
/* 
 * Feed array to DSP transmit register.
 */


extern int DSPWriteTXArrayB(
    DSPFix24 *data,
    int nwords);
/*
 * Feed array *backwards* to DSP TX register 
 */


/***************** READ/WRITE ARRAY FROM/TO DSP HOST INTERFACE ***************/

/* For DMA array transfers to/from DSP */

extern int DSPWriteArraySkipMode(
    DSPFix24 *data,		/* array to send to DSP (any type ok) */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    int startAddress,		/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount,		/* from DSP perspective */
    int mode);			/* from <nextdev/dspvar.h> */
/* 
 * Send an array of bytes to the DSP.
 * The mode is one of (cf. dspvar.h):
 *
 *	DSP_MODE8
 *	DSP_MODE16
 *	DSP_MODE24
 *	DSP_MODE32
 *
 * Mode DSP_MODE8 maps successive bytes from the source byte array to 
 * successive words in the DSP.	 Each byte is right justified in the 
 * 24-bit DSP word.
 *
 * Mode DSP_MODE16 maps successive byte pairs from the source byte array to 
 * successive words in the DSP.	 Each 16-bit word from the source is right 
 * justified in the 24-bit DSP word.
 *
 * Mode DSP_MODE24 maps successive byte trios from the source byte array to 
 * successive words in the DSP.	 Each 24-bit word from the source occupies
 * a full 24-bit DSP word.
 *
 * Mode DSP_MODE32 maps the least significant three bytes from each four-byte
 * word in the source array to successive words in the DSP.  Each 32-bit word 
 * from the source specifies a full 24-bit DSP word.
 *
 * The skip factor specifies the increment for the DSP address register
 * used in the DMA transfer.  A skip factor of 1 means write successive
 * words contiguously in DSP memory.  A skip factor of 2 means skip every
 * other DSP memory word, etc.
 */


extern int DSPReadArraySkipMode(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    int startAddress,		/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount,		/* from DSP perspective */
    int mode);			/* DMA mode from <nextdev/dspvar.h> */
/* 
 * Receive an array of bytes from the DSP.
 * Operation is analogous to that of DSPWriteArraySkipMode().
 *
 * Note: In order to relieve pressure on the host side,
 * the DSP blocks from the time the host is interrupted
 * to say that reading can begin and the time the host tells
 * the DSP that the host interface has been initialized in DMA
 * mode.  Therefore, this routine should not be used to read DSP
 * memory while an orchestra is running.
 */


/*************************** DSP SYNCHRONIZATION ***************************/


extern int DSPAwaitHC(int msTimeLimit);
/*
 * Wait for "HC bit" to clear. 
 * The HC clears when the next instruction to be executed in the DSP
 * is the first word of the host command interrupt vector.
 */


extern int DSPAwaitTRDY(int msTimeLimit);
/*
 * Wait for "TRDY bit" to set. 
 */


extern int DSPAwaitHF3Clear(int msTimeLimit);
/*
 * Wait for HF3 = "TMQ full" or "AP Busy" bit to clear. 
 */


/*** FIXME: Move to DSPMessage.h ***/

extern int DSPAwaitHostMessage(int msTimeLimit);
/*
 * Wait for currently executing host message to finish.
 */


/******************************** DSP MESSAGES *******************************/

/* 
 * Any unsolicited word written by the DSP to the host (via RX) 
 * is defined as a "DSP Message".
 */


extern int DSPBreakPoint(int dsp_bp_msg);
/*
 * Process a breakpoint generated by DSP software.  A "breakpoint" is just a
 * "DSP message" with an op-code of 0x80.
 * It currently just prints the DSP breakpoint message, reads any messages
 * trying to get out of the DSP, and pauses so that the Ariel debugger (Bug56)
 * can be used to see what's going on before anything else happens.
 */


extern int DSPReadMessages(int msTimeLimit);
/*
 * Read messages from DSP.
 * Returns 0 if DSP messages were read by msTimeLimit milliseconds.
 * A 0 msTimeLimit means don't wait if there are no messages waiting
 * from the DSP.
 */


extern int DSPFlushMessages(void);
/*
 * Flush any unread messages from the DSP.
 */


extern int DSPFlushMessageBuffer(void);
/*
 * Flush any DSP messages cached internally.
 * Same as DSPFlushMessages() except that the DSP
 * is not checked for more messages.  Anything
 * queued up in the driver buffer will stay there.
 */


extern int DSPDataIsAvailable(void);
/*
 * Return nonzero if DSP has one or more pending DSP messages waiting in the
 * DSP host interface.	For this to work, RREQ in the ICR of the DSP host
 * interface must be set.
 */


extern int DSPAwaitData(int msTimeLimit);
/*
 * Block until DSP has one or more pending DSP messages.
 * An msTimeLimit of zero means wait forever.
 * Returns 0 when data available, nonzero if
 * no data available before time-out.
 */


/******************************* Port fetching *******************************/

/*
 * In all these routines for obtaining Mach ports,
 * the DSP must be opened before asking for the ports.
 */

port_t DSPGetOwnerPort(void);
/* 
 * Get port conveying DSP and sound-out ownership capability.
 */


port_t DSPGetHostMessagePort(void);
/* 
 * Get port used to send "host messages" to the DSP.
 */


port_t DSPGetDSPMessagePort(void);
/* 
 * Get port used to send "DSP messages" from the DSP to the host.
 */


port_t DSPGetErrorPort(void);
/* 
 * Get port used to send "DSP error messages" from the DSP to the host.
 * Error messages on this port are enabled by DSPEnableHostMsg().
 */


port_t DSPMKGetSoundPort(void);
/* 
 * Get sound device port.
 */


port_t DSPMKGetWriteDataStreamPort(void);
/* 
 * Get stream port used to receive "DSP write data" buffers from the DSP.
 */


port_t DSPMKGetSoundOutStreamPort(void);
/* 
 * Get stream port used to convey "sound out" buffers from the DSP
 * to the stereo DAC.
 */

port_t DSPMKGetWriteDataReplyPort(void);
/* 
 * Get reply port used to receive status information on "DSP write data" 
 * buffers transfers from the DSP.
 */

/************************ SIMULATOR FILE MANAGEMENT **************************/

extern int DSPIsSimulated(void);
/* 
 * Returns nonzero if the DSP is simulated.
 */


extern int DSPIsSimulatedOnly(void);
/* 
 * Returns nonzero if the DSP simulator output is open but the DSP device 
 * is not open.	 This would happen if DSPOpenSimulatorFile() were called
 * without opening the DSP.
 */


FILE *DSPGetSimulatorFP(void);
/*
 * Returns file pointer used for the simulator output file, if any.
 */


extern int DSPOpenSimulatorFile(char *fn);			
/* 
 * Open simulator output file fn.
 */


extern int DSPStartSimulator(void);
/*
 * Initiate simulation mode, copying DSP commumications to the simulator
 * file pointer.
 */


extern int DSPStartSimulatorFP(FILE *fp);
/*
 * Initiate simulation mode, copying DSP commumications to the file pointer fp.
 * If fp is NULL, the previously set fp, if any, will be used.
 */


extern int DSPStopSimulator(void);
/*
 * Clear simulation bit, halting DSP command output to the simulator file.
 */


extern int DSPCloseSimulatorFile(void);
/* 
 * Close simulator output file.
 */

/*********************** DSP COMMANDS FILE MANAGEMENT ************************/

extern int DSPIsSavingCommands(void);
/* 
 * Returns nonzero if a "DSP commands file" is open.
 */

extern int DSPIsSavingCommandsOnly(void);
/* 
 * Returns nonzero if the DSP commands file is open but the DSP device 
 * is not open.	 This would happen if DSPOpenCommandsFile() were called
 * without opening the DSP.
 */

extern int DSPOpenCommandsFile(char *fn);
/*
 * Opens a "DSP Commands file" which will receive all Mach messages
 * to the DSP.  The filename suffix should be ".snd".  This pseudo-
 * sound file can be played by the sound library.
 */

extern int DSPCloseCommandsFile(DSPFix48 *endTimeStamp);
/*
 * Closes a "DSP Commands file".  The endTimeStamp is used by the
 * sound library to terminate the DSP-sound playback thread.
 */
