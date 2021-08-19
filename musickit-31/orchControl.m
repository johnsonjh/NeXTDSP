/* This is imported as source by Orchestra.m.
   
   See /usr/include/musickit/devstatus.h */

/* 
  Modification history:
  
  01/30/90/daj - Created from Orchestra.m.
  01/31/90/daj - Changed select() to msg_receive() for uniformity with 
                 the rest of the Music Kit.
  02/15/90/daj - Changed order of events in re-open in -open. 
                 Removed wait for IDLE. 
                 Added reset of time to 0.
                 Added SOFTREOPEN compile switch
  02/16/90/daj - Changed -close to insert less 0s at the end of write data
                 (and soundout). Previously, we inserted many 0s because we 
                 had a libdsp bug in write data, but Julius says this is fixed.
                 Both cases (write data and sound out) need to be tested!
                 TESTME
  03/21/90/daj - Added work-around for pause/resume driver bug. But did not
                 enable the work-around. Set USEFREEZE to 0 to enable 
                 work-around.
  03/23/90/mtm - Added support for DSP commands file in -open and -close.
  03/26/90/mtm - -close no longer sends abort host message when closing
                 DSP commands file.
  03/28/90/mtm - outputDSPCommandsSoundfile -> outputCommandsFile
  03/30/90/mtm - Close commandsfile in -abort.
  03/30/90/daj - Added further support for USEFREEZE == 0
  03/28/90/daj - Added read data API support. 
  03/27/90/mmm - Added adjustOrchTE to this version
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/23/90/daj - Changed callse to _pause to be _pause: to fix bug in 
                 _OrchloopbeginUG.
  04/25/90/daj - Changed arg order in calls to DSPWriteValue, to conform
                 with new libdsp api.
  04/27/90/daj - Added call to DSPEnableErrorFile() conditional upon -DDEBUG.
  04/05/90/daj - Replaced call to DSPMemoryClear() with 
                 DSPMKClearDSPSoundOutBufferTimed(). Replaced call to
                 DSPMKWriteLong() with DSPMKSetTime(). 
		 Added setting of new instance variables 
		 _bottomOfExternalMemory and _topOfExternalMemory as well as 
		 the argument partition sizes.
                 Substituted DSPMKGetClipCountAddress() for DSPMK_X_NCLIP. 
		 Renamed ORCHSYSLOC to ORCHLOOPLOC.
  06/10/90/daj - Changed USEFREEZE to 0
  08/17/90/daj - Added more conditional compilation code to make it so
                 if the pause/resume driver bug gets fixed, then the
		 Orchestra will correctly start on a dime with its buffers
		 full.
  09/26/90/daj - For dsp-18, need to change DSPMKGetClipCountAddress() to 
                 DSPMKGetClipCountXAddress() to correspond to new libdsp
		 API. Changed USEFREEZE to 1.

  10/04/90/daj - Changed USEFREEZE to 0 because pause of sound out causes panic.
  */

#import <sys/message.h>
#import <mach_error.h>

#define SOFTREOPEN 1  /* Set to 1 to NOT do a DSPClose/MKInit on re-open. */

#define USEFREEZE 0 /* Set to 0 if driver bug not fixed for 2.0 */

#if USEFREEZE
/* The following will eventually go in libdsp: */
static int DSPMKPauseSoundOut(void);
static int DSPMKResumeSoundOut(void);
#endif

static void adjustOrchTE(id self,BOOL yesOrNo,BOOL reset);

static void startSoundAndFillBuffers(Orchestra *self)
    /* There are 35 DSP buffers needed to fill up sound out. Each takes 3.2 ms,
       as measured. Thus, the total time to wait is about .1 second.
       */
{
#   define TIMETOWAIT 100 /* milliseconds */
    /* Copied from sleep.c. */
    port_t aPort;
    struct {
        msg_header_t header;
    } null_msg;
#if USEFREEZE
    DSPMKPauseSoundOut();
#endif
    DSPMKStartSoundOut();
    DSPStartAtAddress(ORCHLOOPLOC); /* Starts orchloop running, but without
				       time advancing */
    if (port_allocate(task_self(),&aPort) != KERN_SUCCESS)
      return;
    null_msg.header.msg_local_port = aPort;
    null_msg.header.msg_size = sizeof(null_msg);
    (void)msg_receive((msg_header_t *)&null_msg, RCV_TIMEOUT, TIMETOWAIT);
    (void)port_deallocate(task_self(),aPort);
}

-open
  /* Opens device if not already open. 
     Resets orchestra loop if not already reset, freeing all Unit Generators 
     and Synth Patches. Sets deviceStatus to MK_devOpen. Returns nil if some
     problem occurs, else self. Note: In release 0.9, it will not work
     to send open to an already opened, running or stopped Orchestra. 
     */
{
#ifdef DEBUG
    DSPEnableErrorFile("/dev/tty");
#endif
    DSPSetCurrentDSP(orchIndex);
    DSPMKSetSamplingRate(samplingRate);
    switch (deviceStatus) {
      case MK_devClosed: /* Need to open it */
        if (soundOut)
          DSPMKEnableSoundOut();
        if (fastResponse)
          DSPMKEnableSmallBuffers();
        if (outputSoundfile) {
            DSPMKSetWriteDataFile(outputSoundfile); /* Must be before enable */
            DSPMKEnableWriteData();
        }
        if (inputSoundfile) { /* READ DATA */
            DSPMKSetReadDataFile(inputSoundfile); /* Must be before enable */
            DSPMKEnableReadData();
        }
        if (outputCommandsFile)
          DSPOpenCommandsFile(outputCommandsFile);
        if (SSISoundOut)
          DSPMKEnableSSISoundOut();     /* sound out to serial port */
        if (useDSP) {
            if (DSPMKInit()) {
                DSPClose();
                return nil;
            }
        }
        _bottomOfExternalMemory = DSPGetLowestExternalUserAddress();
        _topOfExternalMemory = DSPGetHighestExternalUserAddress() - 
          extraRoomAtTopOfOffchipMemory;
        {
            int memSize = _topOfExternalMemory - _bottomOfExternalMemory;
            if (memSize < 0) {
                _topOfExternalMemory = _bottomOfExternalMemory;
                memSize = 0;
            }
            _numXArgs = _xArgPercentage * memSize; /* Implicit floor() here */
            _numYArgs = _yArgPercentage * memSize; /* Implicit floor() here */ 
        }
        if (_simulatorFile(self)) {
            DSPOpenSimulatorFile(_simulatorFile(self));
            /* start simulator file AFTER bootstrap */
            _simFP = DSPGetSimulatorFP();
        }
        if (outputSoundfile) 
          DSPMKEnableBlockingOnTMQEmptyTimed(DSPMK_UNTIMED);
        /* loadOrchLoop sets devStatus to MK_devOpen if it succeeds.
           If it fails, it does a DSPClose() and sets devStatus to 
           MK_devClosed. */
        if (!loadOrchLoop(self))  
          return nil;
        if (soundOut)  /* Do it here so buffers are full */
          startSoundAndFillBuffers(self);
        break;
      case MK_devStopped:
      case MK_devRunning:
        /* All of the following is an attempt to avoid doing a 
           DSPClose/DSPMKInit. Perhaps this is silly! */
        adjustOrchTE(self,NO,YES);
#if SOFTREOPEN
        /* Reset orchestra loop without doing a DSPClose() first. */
        DSPHostMessage(DSP_HM_IDLE);    /* Jump to infinite loop */
        /* Don't wait for idle. This causes infinite hang if we're in dma 
           mode. On the other hand, if we stop sound out first, the message
           gets stuck behind dma requests (or something like that). In either
           case, we hang infinitely. */ 
        if (soundOut)
          DSPMKStopSoundOut();          /* Stop sending sound-out buffers */
        if (SSISoundOut)
          DSPMKStopSSISoundOut();
        if (outputSoundfile) {
            DSPMKStopWriteData();
            DSPMKRewindWriteData();
        }
        if (inputSoundfile) { /* READ DATA */
            DSPMKStopReadData();
            DSPMKRewindReadData();
        }
        /* Clear sound-out buffers so junk doesn't go out at start of play */
        DSPMKClearDSPSoundOutBufferTimed(DSPMK_UNTIMED);
//      DSPMemoryClear(DSP_MS_Y,DSPMK_YB_DMA_W,DSPMK_NB_DMA_W); 
        freeUGs(self); /* Frees _OrchSysUG as well. */
        /* loadOrchLoop sets devStatus to MK_devOpen if it succeeds.
           If it fails, it does a DSPClose() and sets devStatus to 
           MK_devClosed. */
        if (!loadOrchLoop(self))         
          return nil;
        {   /* Reset time. */
            DSPFix48 zero = {0,0}; 
            DSPMKSetTime(&zero);
        }
        if (soundOut)  /* Do it here so buffers are full */
          startSoundAndFillBuffers(self);
        break;
#else
        [self abort];
        return [self open];
#endif
      case MK_devOpen:
      default:
        break;
    }
    return self;
}

-run
  /* If not open, does a [self open].
     If not already running, starts DSP clock. 
     Sets deviceStatus to MK_devRunning. */
{
    switch (deviceStatus) {
      case MK_devClosed:
        if (![self open])
          return nil;
      case MK_devOpen:
        DSPSetCurrentDSP(orchIndex);
        [_sysUG _unpause]; /* Poke orchloopbegin to continue on */
	/* The difference between my _sysUG pause mode and Julius'	
	   freeze mode is that the former generates 0s and the latter does 
	   nothing. */
        /* If we're just doing write data, we haven't done a startAtAddress
           yet. */
        if (!soundOut)     /* Otherwise, done by startSoundAndFillBuffers() */
          DSPStartAtAddress(ORCHLOOPLOC);
#if USEFREEZE
	else DSPMKResumeSoundOut();
#endif
        DSPMKResumeOrchestra(); /* Start time advancing */
        deviceStatus = MK_devRunning;
        if (outputSoundfile)
          DSPMKStartWriteDataTimed(_MKCurSample(self));
        if (inputSoundfile) /* READ DATA */
          DSPMKStartReadDataTimed(_MKCurSample(self));
        /* We do it timed because we don't want to write out the deltaT at
           the beginning. */
        if (SSISoundOut)
          DSPMKStartSSISoundOut();
        /* We don't know of any devices that can start instantaneously so
           we give them deltaT's worth of 0s. */
        adjustOrchTE(self,YES,YES);
        break;
      case MK_devStopped:
#if USEFREEZE
	DSPMKThawOrchestra(); 
        if (soundOut)
         DSPMKResumeSoundOut();
#else
        [_sysUG _unpause]; /* Poke orchloopbegin to continue on */
        if (!soundOut)
          DSPMKThawOrchestra();
        else DSPMKResumeOrchestra();
#endif
        adjustOrchTE(self,YES,NO);
        deviceStatus = MK_devRunning;
        break;
      case MK_devRunning:
      default:
        break;
    }
    return self;
}

-stop
  /* If not open, does a [self open].
     Otherwise, stops DSP clock and sets deviceStatus to MK_devStopped.
     */
{
    switch (deviceStatus) {
      case MK_devClosed:
        return [self open];
      case MK_devOpen:
      case MK_devStopped:
        return self;
      case MK_devRunning:
        DSPSetCurrentDSP(orchIndex);
#if USEFREEZE
	if (soundOut)
	  DSPMKPauseSoundOut();
	DSPMKFreezeOrchestra();
#else
        [_sysUG _pause:self->_looper]; /* Poke orchloop to hardwire jump */
	if (!soundOut)
          DSPMKFreezeOrchestra();
        else DSPMKPauseOrchestra();
#endif
        adjustOrchTE(self,NO,NO);
        deviceStatus = MK_devStopped; 
        return self;
      default:
        break;
    }
    return self;
}

-abort
  /* Closes the receiver immediately, without waiting for all enqueued DSP 
     commands to be executed. This involves freeing all
     unit generators in its unit generator stack, clearing all
     synthpatch allocation lists and releasing the DSP. It is an error
     to free an orchestra with non-idle synthPatches or allocated unit
     generators which are not members of a synthPatch. An attempt to
     do so generates an error. 
     Returns self unless there's some problem
     closing the DSP, in which case, returns nil.
     Closes the commands file (if open) at the current time.
     */
{
    DSPFix48 curTimeStamp;
    
    if (deviceStatus != MK_devClosed) {
        adjustOrchTE(self,NO,YES);
        freeUGs(self);
        DSPSetCurrentDSP(orchIndex);
        if (_simFP) {
            DSPCloseSimulatorFile();
            _simFP = NULL;
        }
        if (outputCommandsFile && DSPIsSavingCommands()) {
            DSPMKReadTime(&curTimeStamp);
            DSPCloseCommandsFile(&curTimeStamp);
        }
        if (!useDSP) 
          return self;
        if (DSPClose()) 
          return nil;
        return self;
    }
    return self;
}

-close
  /* Closes the receiver after waiting for all enqueued DSP commands to be
     executed. This involves freeing all
     unit generators in its unit generator stack, clearing all
     synthpatch allocation lists and releasing the DSP. It is an error
     to free an orchestra with non-idle synthPatches or allocated unit
     generators which are not members of a synthPatch. An attempt to
     do so generates an error. 
     Returns self unless there's some problem
     closing the DSP, in which case, returns nil.
     
     */
{
    if (deviceStatus == MK_devRunning) { /* If not, can't wait for end of time */ 
        /* Wait for end of time */
        DSPFix48 endTimeStamp;
        double endTime;
        int nclip;
        
#       define FOREVER 0
#       define TIMEOUT (FOREVER) /* in seconds */
        
        /* There was a bug: We had to pad the end much too much when doing               
           write data. 
           
           It SHOULD work like this:
           
           The DSP has a buffer, the driver in write data mode has 16 buffers.
           Theoretically the delay is 17 buffers, 512 words of data 
           (256 stereo samples). But there's an extra buffer in the DSP and an
           extra 16 buffers in the driver. So the maximum time needed should be
           35 buffers of stereo samples (35 * 512).
           
           So at 22khz, the delay is .407 seconds and at 44 khz, the delay is              .204 seconds.
           
           For write data, .05 seconds at 22 khz or .025 at 44 khz
           */
        
        double bufferTime;
        if (!isTimed) {
            DSPSetCurrentDSP(orchIndex);
            DSPMKDisableBlockingOnTMQEmptyTimed(DSPMK_UNTIMED);
        } else {
#if 0       /* See comment above. */
            bufferTime = (outputSoundfile) ? 1.5 : .5;
#else
            bufferTime = (((outputSoundfile) ? .05 : .407) * 
                          (22050.0/samplingRate));
#endif  
            endTime = _MKLastTime() + bufferTime;
            doubleIntToFix48UseArg(endTime * samplingRate, &endTimeStamp);
            if (_simFP) 
              if (_MK_ORCHTRACE(self,MK_TRACEDSP))
                _MKOrchTrace(self,MK_TRACEDSP,"End of timed messages queue."
                             "Do timed read of clips.\n");
            DSPSetCurrentDSP(orchIndex);
            DSPMKDisableBlockingOnTMQEmptyTimed(&endTimeStamp);
            if (DSPMKRetValueTimed(&endTimeStamp,DSP_MS_X,
				   DSPMKGetClipCountAddress(),
                                   &nclip))
              if (_MK_ORCHTRACE(self,MK_TRACEDSP))
                _MKOrchTrace(self,MK_TRACEDSP,
                             "Could not send timed peek to orchestra.");
            if (nclip)
              if (_MK_ORCHTRACE(self,MK_TRACEDSP))
                _MKOrchTrace(self,MK_TRACEDSP,
                             "Clipping detected for %d ticks.\n",nclip);
            
            if (outputCommandsFile) {
                if (DSPMKCallTimedV(&endTimeStamp,DSP_HM_HOST_WD_OFF,1,1))
                  if (_MK_ORCHTRACE(self,MK_TRACEDSP))
                    _MKOrchTrace(self,MK_TRACEDSP,
                                 "Could not send timed write data off to orchestra.");
                DSPMKFlushTimedMessages();
                DSPCloseCommandsFile(&endTimeStamp);
            }
        }
    }
    return [self abort];
}

-pauseInputSoundfile
{
    DSPMKPauseReadDataTimed(_MKCurSample(self));
    return self;
}

-resumeInputSoundfile
{
    DSPMKResumeReadDataTimed(_MKCurSample(self));
    return self;
}

#if USEFREEZE
/* The following will eventually go in libdsp */
#import <sound/sounddriver.h>

static int DSPMKPauseSoundOut(void)
{
    snddriver_stream_control(DSPMKGetWriteDataStreamPort(),
			     0,SNDDRIVER_PAUSE_STREAM);
}

static int DSPMKResumeSoundOut(void)
{
    snddriver_stream_control(DSPMKGetWriteDataStreamPort(),
			     0,SNDDRIVER_RESUME_STREAM);
}
#endif
