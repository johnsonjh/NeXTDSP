/*	DSPControl.c -	Utilities for DSP control from host.
	Copyright 1987,1988, by NeXT, Inc.

Modification history:
	07/28/88/jos - Created from _DSPUtilities.c
	05/12/89/jos - Added DSPEnableHostMsg() to DSPMKInit().
	06/08/89/jos - Brought in paren, TMQ block routines. Added DMA_M pokes.
	07/14/89/daj - Commented out DSP_UNTIL_ERROR in _DSPReadTime.
	07/21/89/daj - Made mkSys be static and cached in DSPMKInit().
	01/22/90/jos - Suppressed untimed parens.
	04/23/90/jos - flushed unsupported entry points.
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"
#include <nextdev/snd_msgs.h> /* for DSP_MSG_LOW */
 

int DSPInit(void) 
{
    return(DSPBoot(NULL));
}


int DSPMKInit(void) 
{
    int ec;
    static DSPLoadSpec *mkSys = NULL;
	
    DSPEnableHostMsg();

    if (!mkSys) { 
	ec = DSPReadFile(&mkSys,DSP_MUSIC_SYSTEM_BINARY_0);
	if(ec)
	  return _DSPError1(ec,"DSPMKInit: Could not read music system '%s' "
			    "for booting the DSP", DSP_MUSIC_SYSTEM_BINARY_0);
    }
    ec = DSPBoot(mkSys);
    if(ec)
      return(_DSPError(ec,"DSPMKInit: Could not boot DSP"));

    return(ec); 
}

int DSPMKInitWithSoundOut(
    int lowSamplingRate)
{
    int ec;

    DSPMKEnableSoundOut();

    if (lowSamplingRate)
      DSPMKSetSamplingRate(DSPMK_LOW_SAMPLING_RATE);
    else
      DSPMKSetSamplingRate(DSPMK_HIGH_SAMPLING_RATE);

    ec = DSPMKInit();
    if(ec)
      return(_DSPError(ec,"DSPMKInitWithSoundOut: Could not init DSP"));
    else
      return(0); 
}

int DSPSetStart(int startAddress)
{
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Set start address \n");
    return(DSPCallV(DSP_HM_SET_START,1,startAddress));
}

int DSPStart()
{
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; GO \n");
    return(DSPHostMessage(DSP_HM_GO));
}

int DSPStartAtAddress(int startAddress)
{
    DSP_UNTIL_ERROR(DSPSetStart(startAddress));
    return(DSPStart());
}

int DSPCheckVersion(
    int *sysver,	   /* system version running on DSP (returned) */
    int *sysrev)	   /* system revision running on DSP (returned) */
{
    int i,dspack=0,verrev=0;
    if (DSPPingVersion(&verrev))
      DSP_MAYBE_RETURN(_DSPError(DSP_ESYSHUNG, "DSPCheckVersion: "
				 "DSP system is not responding."));
    *sysver = (verrev>>8)&0xFF;
    *sysrev = (verrev)&0xFF;
    if (*sysver != DSP_SYS_VER || *sysrev != DSP_SYS_REV)
      _DSPError(DSP_EBADVERSION,
		_DSPCat(_DSPCat("DSPCheckVersion: *** WARNING *** "
				"DSP system version.revision = ",
				_DSPCat(_DSPCVS(*sysver),
					_DSPCat(".0(",_DSPCVS(*sysrev)))),
			_DSPCat(") while this program was compiled assuming ",
				_DSPCat(_DSPCVS(DSP_SYS_VER),
					_DSPCat(".0(",
						_DSPCat(_DSPCVS(DSP_SYS_REV),
							")")
						)))));
		
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; DSPCheckVersion: HM_FIRST\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_HM_FIRST));
    DSP_UNTIL_ERROR(DSPAwaitUnsignedReply(DSP_DM_HM_FIRST,&dspack,
				      DSPDefaultTimeLimit));

    if(DSP_MESSAGE_ADDRESS(dspack)!=DSP_HM_FIRST)
      _DSPFatalError(DSP_EBADVERSION, 
		     /* This is a very rotten situation */
		     "*** DSPCheckVersion: DSP host-message dispatch "
		     "table origin is not the same in this compilation "
		     "as in the DSP.");

    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),
	      ";; DSPCheckVersion: HM_LAST\n");
    DSP_UNTIL_ERROR(DSPHostMessage(DSP_HM_HM_LAST));
    DSP_UNTIL_ERROR(DSPAwaitUnsignedReply(DSP_DM_HM_LAST,&dspack,
				      DSPDefaultTimeLimit));
    if(DSP_MESSAGE_ADDRESS(dspack)!=DSP_HM_LAST)
      _DSPFatalError(DSP_EBADVERSION, 
		     /* This is a wholly flagitious condition */
		     "*** DSPCheckVersion: VERSIONITIS! "
		     "DSP host-message dispatch table end "
		     "is not the same in this compilation as in the DSP.");
    return(0);
}


int DSPIsAlive(void) 
{
    return !DSPPing();
}


int DSPMKIsAlive(void) 
{
    if (!DSPMonitorIsMK())
      return _DSPError(DSP_EMISC,"DSPMKIsAlive: "
		       "DSP is not running the Music Kit monitor");
    return !DSPPing();
}


/******************************** TIMED CONTROL ******************************/

/* 
   Timed messages are used by the music kit.  Time is maintained in the DSP.
   The current time (in samples) is incremented by the tick size
   once each iteration of the orchestra loop on the DSP.  When the orchestra
   loop is initially loaded and started, the time increment is zero so that
   time does not advance.  This is the "paused" state for the DSP orchestra.
   DSPMKPauseOrchestra() will place the orchestra into the paused state at 
   any time.  A DSPMKResumeOrchestra() is necessary to clear the pause.

*/

int DSPMKSetTime(
    DSPFix48 *aTimeStampP)
{
    if (aTimeStampP == DSPMK_UNTIMED)
      return 0;
    return DSPMKCallTimedV(&DSPMKTimeStamp0,DSP_HM_SET_TIME,2,
			DSP_FIX24_CLIP(aTimeStampP->high24),
			DSP_FIX24_CLIP(aTimeStampP->low24));
}

int DSPMKSetTimeFromInts(highTime,lowTime)
    int highTime;
    int lowTime;
{
    return DSPMKCallTimedV(&DSPMKTimeStamp0,DSP_HM_SET_TIME,2,highTime,lowTime);
}

int DSPMKClearTime(void)
{
    return DSPMKCallTimedV(&DSPMKTimeStamp0,DSP_HM_SET_TIME,2,0,0);
}

DSPFix48 *DSPMKGetTime(void)
{
    static DSPFix48 dspTime;
    if(DSPMKReadTime(&dspTime))
    	return NULL;
    else
	return &dspTime;
}

int DSPMKReadTime(DSPFix48 *dspTime)
{
    int i,t0,t1,t2,curTime;
    /* Read the current time in samples */
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Get current time in samples\n");
    DSPHostMessage(DSP_HM_GET_TIME);
//    DSP_UNTIL_ERROR(DSPAwaitUnsignedReply(DSP_DM_TIME0,&t0,10000));
//    DSP_UNTIL_ERROR(DSPAwaitUnsignedReply(DSP_DM_TIME1,&t1,10000));
//    DSP_UNTIL_ERROR(DSPAwaitUnsignedReply(DSP_DM_TIME2,&t2,10000));
    DSPAwaitUnsignedReply(DSP_DM_TIME0,&t0,10000);
    DSPAwaitUnsignedReply(DSP_DM_TIME1,&t1,10000);
    DSPAwaitUnsignedReply(DSP_DM_TIME2,&t2,10000);
    
    dspTime->low24 = ((t1&0xFF)<<16)|(t0&0xFFFF);
    dspTime->high24 = ((t2&0xFFFF)<<8)|((t1>>8)&0xFF);

    if (_DSPTrace)
      printf("DSP Time = 0x%X = %d\n\n",curTime,curTime);

    return(0);
}


int DSPMKPauseOrchestra(void)	/* Stop sample-counter at "end of current tick" */
{
    int ec=0;
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Pause orchestra loop\n");
    ec = DSPMKCallTimedV(&DSPMKTimeStamp0,DSP_HM_SET_TINC,2,0,0);
    return(ec);
}


int DSPMKPauseOrchestraTimed(DSPFix48 *aTimeStampP)
{
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),
	      ";; PAUSE at time %s\n",DSPFix48ToSampleStr(aTimeStampP));
    return DSPMKCallTimedV(aTimeStampP,DSP_HM_SET_TINC,2,0,0);
}


int DSPMKResumeOrchestra(void)
{
    int ec=0;
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; Resume orchestra loop\n");
    ec=DSPCallV(DSP_HM_SET_TINC,2,0,DSPMK_I_NTICK);
    return(ec);
}


int _DSPStartTimed(aTimeStampP)
    DSPFix48 *aTimeStampP;
{
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; GO at sample %s\n",DSPFix48ToSampleStr(aTimeStampP));
    return DSPMKCallTimedV(aTimeStampP,DSP_HM_GO,0);
}

int _DSPSetStartTimed(aTimeStampP,startAddress)
    DSPFix48 *aTimeStampP;
    int startAddress;
{
    if (DSPIsSimulated()) 
      if (aTimeStampP)
	fprintf(DSPGetSimulatorFP(),";; Set start address at sample %s\n",
		DSPFix48ToSampleStr(aTimeStampP));
    return DSPMKCallTimedV(aTimeStampP,DSP_HM_SET_START,1,startAddress);
}

int _DSPSineTest(nbufs)
    int nbufs;
{
    if (DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),";; Enable sine test for %d buffers\n", nbufs);
    return DSPCall(DSP_HM_SINE_TEST,1,&nbufs);
}

int _DSPReadSSI(nbufs)
    int nbufs;
{
    if (DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),";; Enable SSI A/D loop for %d buffers\n", nbufs);
    return DSPCall(DSP_HM_ADC_LOOP,1,&nbufs);
}

int _DSPSetSSICRA(cra)		/* Set Control Register A of the SSI */
    int cra;			/* 0x30C for FSL=1, 0x20C for FSL=0 */
{				/* cf. $DSP/smsrc/jsrlib.asm */
    if (DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),";; Set control reg A in SSI to 0x%X\n",cra);
    return DSPWriteValue(cra,DSP_MS_X,0xFFEC);
}

int _DSPSetSSICRB(crb)		/* Set Control Register B of the SSI */
    int crb;			/* 0x30C for FSL=1, 0x20C for FSL=0 */
{				/* cf. $DSP/smsrc/jsrlib.asm */
    if (DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),";; Set control reg B in SSI to 0x%X\n",crb);
    return DSPWriteValue(crb,DSP_MS_X,0xFFED);
}


int DSPMKEnableAtomicTimed(DSPFix48 *aTimeStampP)
{
    if (aTimeStampP != DSPMK_UNTIMED)
         return DSPMKHostMessageTimed(aTimeStampP,DSP_HM_OPEN_PAREN);
    else return 0;
    /* _DSPError(DSP_EPROTOCOL,"DSPMKEnableAtomicTimed: atomic execution "
    			"not supported for untimed messages."); */
}

int DSPMKDisableAtomicTimed(aTimeStampP) 
    DSPFix48 *aTimeStampP;
{
    if (aTimeStampP != DSPMK_UNTIMED)
         return DSPMKHostMessageTimed(aTimeStampP,DSP_HM_CLOSE_PAREN);
    else return 0;
    /* _DSPError(DSP_EPROTOCOL,"DSPMKDisableAtomicTimed: atomic execution "
    			"not supported for untimed messages."); */
}

int DSPSetDMAReadMReg(DSPAddress M)
{
    return DSPCall(DSP_HM_SET_DMA_R_M,1,&M);
}


int DSPSetDMAWriteMReg(DSPAddress M)
{
    return DSPCall(DSP_HM_SET_DMA_W_M,1,&M);
}


int DSPAbort(void)
{
    return DSPHostCommand(DSP_HC_ABORT);
}







