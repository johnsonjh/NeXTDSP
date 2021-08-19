/*
 * DSPAPUtilities.c
 *
 * Modification History:
 *	05/12/89/mtm - Added NOP opcode to end of initial AP code.
 *	05/31/89/mtm - Added XY partition data transfer routines.
 *	06/05/89/mtm - Added DSPAPGetLocalImagDirectory().
 *	06/21/89/mtm - Changed ap_binaries dir name to apbin.
 *	06/22/89/mtm - Fixed bug in DSPAPInit() call to DSPAPBoot().
 *	06/26/89/mtm - Change hard-coded /apbin/ to DSP_AP_BIN_DIRECTORY.
 *	07/03/89/mtm - Add DSPAPGet{Lowest,Highest}Address[XY]().
 *	07/25/89/mtm - DSPAPBoot() saves static load struct.
 *	08/02/89/mtm - Clean up this header.
 *		       Removed call to DSPRestoreErrorLogging().
 *	08/04/89/mtm - Added call to _DSPEnableMappedArrayReads().
 *	08/08/89/mtm - Added some function externs.  Cast first arg to
 *		       DSPDSP{Read,Write}ArraySkipMode() to (DSPFix24 *).
 *		       Removed call to _DSPEnableMappedArrayReads().
 *	03/12/90/jos - Changed 1st sysapfnfile to apfnfile in DSPAPInitFn.
 *		       If the 1st read failed on an abs path name, the wrong
 *		       file name would be printed as the failing name.
 *	03/12/90/jos - Changed DSPAPInit to DSPAPBoot in error strings
 *		       in DSPAPBoot.  Note that use of _DSPError* implies
 *		       a wrong prefix of "libdsp" for each error message.
 *	08/31/90/jos - rewrote AwaitNotBusy.  Added proc prototypes.
 */

/* See $DSP/lib/grub/DSPWriteC-foo-load-xy-AP-args
   for a version which supports AP args in Y memory as well as X */

#include <stdlib.h>
#include <libc.h>	/* for select() */
#include <c.h>		/* for MIN */
#include <dsp/dsp.h>
#include <nextdev/snd_msgs.h>	/* for DSP_MODE{8,16,24,32} */

#ifndef DSP_DE_ABORT
#define DSP_DE_ABORT DSP_DE_DMA_ABORT
#endif  DSP_DE_ABORT

#ifndef DSP_EABORT
#define DSP_EABORT (DSP_EUSER+1)
#endif  DSP_EABORT

/* private functions from libdsp */
extern int _DSPError1();
extern int _DSPError();
extern int _DSPRelocateUser();
extern int _DSPRelocate();
extern int _DSPLnkRead();
extern int _DSPCVS();
extern char *DSPGetSystemBinaryFile();

/* private mode variables */

static int s_pause_on_error = 0; /* set to hang on error for Bug56 "grab" */
static int ec;			/* error code */

/*  beg_ap (beginend.asm) macro expansion */
#   define DSP_BEG_AP_SIZE 4
    int begAP[DSP_BEG_AP_SIZE] = {  
	0x0AA824,		/* "bset #4,x:$FFE8" opcode (set HF3) */
	0x60F400,		/* "move #<*+1>,R_X" opcode */
	DSPAP_XLI_USR,		/* first x arg address */
	0x000000};		/* "NOP" opcode */
/*	0xBF080,		/* "JSR" opcode */
/*	DSP_HM_RESET_AP};	/* reset entry point for AP prog */



int DSPAPLoadAddress(void) 
{
    return(DSPAP_PLI_USR+DSP_BEG_AP_SIZE);
}


int DSPAPBoot(void)
{
    int ec;
    static DSPLoadSpec *apSys = NULL;

    /* DSPEnableHostMsg();	FIXME */
    DSPSetAPSystemFiles();

    if (!apSys) {
        ec = DSPReadFile(&apSys,DSPGetSystemBinaryFile());
	if(ec)
	  return _DSPError1(ec,"DSPAPBoot: Could read array proc system '%s' "
			    "for booting the DSP",DSPGetSystemBinaryFile());
    }
    ec = DSPBoot(apSys);
    if(ec)
      return _DSPError(ec,"DSPAPBoot: Could not boot DSP");

    return(0);
}


int DSPAPInit(void)
{
    int ec;

    if (ec=DSPAPBoot())
      return ec;

    /* Download initial AP program */

    if (ec=DSPWriteFix24Array(begAP,DSP_MS_P,DSPAP_PLI_USR,1,DSP_BEG_AP_SIZE))
      return(_DSPError(ec,
	"DSPAPInit: Could not load AP init code to DSP"));

    return(0);
}


int DSPAPEnablePauseOnError(void)
{
    s_pause_on_error = 1;
    return 0;
}


int DSPAPDisablePauseOnError(void)
{
    s_pause_on_error = 0;
    return 0;
}


int DSPAPPauseOnErrorIsEnabled(void)
{
    return s_pause_on_error;
}


int DSPAPAwaitNotBusy(int msTimeLimit)
{
    int ec,dspack;
    extern int _DSPVerbose;

    while (1) {
	ec = DSPAwaitAnyMessage(&dspack, msTimeLimit);
	if (ec)
	  return _DSPError1(DSP_ETIMEOUT,"DSPAPAwaitNotBusy: Timed out "
			    "waiting for AP program after %s milliseconds",
			    _DSPCVS(msTimeLimit));
	
	if (DSP_MESSAGE_OPCODE(dspack)==DSP_DM_MAIN_DONE) 
	  break;
	else {
	    char *arg;
	    arg = "DSPAPAwaitNotBusy: got unexpected DSP message ";
	    arg = DSPCat(arg,DSPMessageExpand(dspack));
	    arg = DSPCat(arg," while waiting for DSP_DM_MAIN_DONE");
	    _DSPError(DSP_EPROTOCOL,arg);
	}
	if (DSP_MESSAGE_OPCODE(dspack)==DSP_DE_ABORT) {
	    if (_DSPVerbose && s_pause_on_error) {
		fprintf(stderr,"*** DSPAPAwaitNotBusy: DSP has aborted.\n"
			"\tAwaiting (prelaunched) debugger.\n"
			"\tRun dspabort in another Shell or Terminal,  \n"
			"\tthen press the debugger's \"Grab\" button.\n"
			"\tBefore resetting the DSP in the debugger, \n"
			"\tterminate this program.\n");
		select(0,0,0,0,0);
	    } else
	      ec = _DSPError(DSP_EABORT,
			       "DSPAPAwaitNotBusy: DSP has aborted.");
	}
    }
    return ec;
}


int DSPAPFree(void) 
{
    return(DSPClose());
}

/*** FIXME: Flush in 1.1 - obsoleted by DSPAPMacroInit (right?) ***/
int DSPAPInitFn(char *apfnfile, DSPLoadSpec **dspSPP)
{
    int ec;
    DSPSection *user;
    int olddeb;
    char *dspdir;
    char *sysapfnfile;
    char *sysapfnfile2;

    olddeb = DSPErrorLogIsEnabled();
    DSPDisableErrorLog();

#define SYS_TOO DSPCat(apfnfile, DSPCat(" or ", sysapfnfile))
#define ENV_TOO DSPCat(SYS_TOO, DSPCat(" or ", sysapfnfile2))

    if(ec=_DSPLnkRead(dspSPP,apfnfile,1)) {
	if (apfnfile[0]=='/') {	/* absolute pathname */
	    if (olddeb)
	      DSPEnableErrorLog();
	    return _DSPError1(ec,
			      "DSPAPInitFn: Could not read file %s",
			      apfnfile);
	}	  
	/* Try standard distribution */
	dspdir = DSPGetAPDirectory();
	sysapfnfile = DSPCat(dspdir,apfnfile);
	
	if(ec=_DSPLnkRead(dspSPP,sysapfnfile,1)) {
	    /* Try standard distribution */
	    if (!getenv("DSP")) { /* DSP env var not set */
		if (olddeb)
		  DSPEnableErrorLog();
		return 
		  _DSPError1(ec,
			     "DSPAPInitFn: Could not read file %s",SYS_TOO);
	    }	  
	    dspdir = DSPCat(DSP_SYSTEM_DIRECTORY,
			    DSP_AP_BIN_DIRECTORY);
	    sysapfnfile2 = DSPCat(dspdir,apfnfile);
	    
	    if(ec=_DSPLnkRead(dspSPP,sysapfnfile2,1)) {
		if (olddeb)
		  DSPEnableErrorLog();
		REMEMBER(Convert to _DSPErrorV here);
		return 
		  _DSPError1(ec,"DSPAPInitFn: Could not read file %s",ENV_TOO);
	    }
	}
    }
    
    /* install USER load offsets in DSP memory */
    if (ec = _DSPRelocateUser(*dspSPP))
      return(_DSPError1(ec,
	"DSPAPInitFn: Could not obtain load offsets. File=%s",apfnfile));

    /* Make room for "beg_ap" DSP Initialization code */
    user = DSPGetUserSection(*dspSPP);

    user->loadAddress[(int)DSP_LC_P] = DSPAPLoadAddress();

    /* do fixups in p code */
    if (ec = _DSPRelocate(*dspSPP))
      return(_DSPError1(ec,
	 "DSPAPInitFn: Could not perform fixups for file %s",apfnfile));

    return(ec);
}


char *DSPAPGetLocalAPDirectory(void) 
{
    char *lbdir;
    char *dspdir;
    dspdir = getenv("DSP");
    if (dspdir == NULL)
      lbdir = DSPGetAPDirectory();
    else
      lbdir = DSPCat(dspdir,DSP_AP_BIN_DIRECTORY);
    return(lbdir);
}

/****************** Get Address functions *********************/

int	DSPAPGetLowestAddress(void)
{
    return(DSPAP_XLE_USR);
}

int	DSPAPGetHighestAddress(void)
{
    return(DSPAP_XHE_USR);
}

int	DSPAPGetLowestAddressXY(void)
{
    return(DSPAP_XLE_USG);
}

int	DSPAPGetHighestXAddressXY(void)
{
    return(DSPAP_XHE_USG);
}

int	DSPAPGetHighestYAddressXY(void)
{
    return(DSPAP_YHE_USG);
}

int	DSPAPGetHighestAddressXY(void)
{
    return(MIN(DSPAPGetHighestXAddressXY(), DSPAPGetHighestYAddressXY()));
}



/********************************* Array Writes ******************************/

/*
 * Note that these "DSPAP" versions are equivalent to their "DSP" prefix 
 * counterparts (in libdsp:DSPTransfer.c) except that the memory-space
 * argument is suppressed. 
 */


int DSPAPWriteFix24Array(
    DSPDatum *data,		/* array to send to DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c and <nextdev/snd_msgs.h> */
    return(DSPWriteArraySkipMode(data,DSP_MS_X,startAddress,
			       skipFactor,wordCount,DSP_MODE32));
}


int DSPAPWritePackedArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word */
    int wordCount)		/* DSP words = byte count / 3 */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
			       skipFactor,wordCount,DSP_MODE24));
}


int DSPAPWriteShortArray(
    short *data,		/* Packed short data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short */
    int wordCount)		/* DSP word count = byte count / 2 */
/* 
 * Send a packed array of 16-bit words to the DSP (typically sound data).
 * Uses 16-bit DMA mode.  Each 32-bit word in the
 * source array provides two successive 16-bit samples in the DSP.
 * In the DSP, each 16-bit word is received right-justified in 24 bits,
 * with no sign extension.
 */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
			       skipFactor,wordCount,DSP_MODE16));
}


int DSPAPWriteByteArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount)		/* Total number of bytes to transfer */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
    			         skipFactor, byteCount,DSP_MODE8));
}


int DSPAPCheckWriteAddresses(int startAddress, int skipFactor, int wordCount)
{
    if (startAddress < DSPAP_XLI_USR)
      return(_DSPError(DSP_EILLADDR,"DSPAPCheckWriteAddresses: "
		    "*** Writing below user onchip DSP memory ***"));
    if (startAddress + (wordCount-1)*skipFactor > DSPAP_XHE_USR)
      return(_DSPError(DSP_EILLADDR,"DSPAPCheckWriteAddresses: "
		    "*** Writing above user DSP memory ***"));
    return(0);
}


int DSPAPCheckWriteAddressesXY(int memorySpace, int startAddress,
			       int skipFactor, int wordCount)
{
    int minAddress = (memorySpace == DSP_MS_X ? DSPAP_XLE_USG : DSPAP_YLE_USG);
    int maxAddress = (memorySpace == DSP_MS_X ? DSPAP_XHE_USG : DSPAP_YHE_USG);

    if (startAddress < minAddress)
      return(_DSPError(DSP_EILLADDR,"DSPAPCheckWriteAddressesXY: "
		    "*** Writing below user external XY DSP memory ***"));
    if (startAddress + (wordCount-1)*skipFactor > maxAddress)
      return(_DSPError(DSP_EILLADDR,"DSPAPCheckWriteAddressesXY: "
		    "*** Writing above user external XY DSP memory ***"));
    return(0);
}


int DSPAPWriteIntArray(int *intArray, int startAddress, int skipFactor,
		       int wordCount)
{
    int ec;
    if (wordCount<=0)
      return(0);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    /* not needed: ec = DSPIntToFix24Array(fix24Array,intArray,wordCount); */
    ec = DSPWriteArraySkipMode((DSPFix24 *)intArray,DSP_MS_X,startAddress,
    			       skipFactor,wordCount,DSP_MODE32);
    return(ec);
}

    
int DSPAPWriteIntArrayXY(int *intArray, int startAddress, int skipFactor,
			 int wordCount)
{
    int ec;
    if (wordCount<=0)
      return(0);
    /* DSPCheckWriteAddressesXY(DSP_MS_X,startAddress,skipFactor,wordCount);*/
    /* not needed: ec = DSPIntToFix24Array(fix24Array,intArray,wordCount); */
    ec = DSPWriteArraySkipMode((DSPFix24 *)intArray,DSP_MS_X,startAddress,
    			       skipFactor,wordCount,DSP_MODE32);
    return(ec);
}


int DSPAPWriteFloatArray(float *floatArray, int startAddress, int skipFactor,
			 int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPFloatToFix24Array(floatArray,fix24Array,wordCount);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    ec2 = DSPWriteArraySkipMode(fix24Array,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}

int DSPAPWriteFloatArrayXY(float *floatArray, int startAddress,
			   int skipFactor, int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPFloatToFix24Array(floatArray,fix24Array,wordCount);
 /* DSPCheckWriteAddressesXY(DSP_MS_X,startAddress,skipFactor,wordCount); */
    ec2 = DSPWriteArraySkipMode(fix24Array,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}

int DSPAPWriteDoubleArray(double *doubleArray, int startAddress,
			  int skipFactor,int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPDoubleToFix24Array(doubleArray,fix24Array,wordCount);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    ec2 = DSPWriteArraySkipMode(fix24Array,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}


int DSPAPWriteDoubleArrayXY(double *doubleArray,
			    int startAddress, int skipFactor, int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPDoubleToFix24Array(doubleArray,fix24Array,wordCount);
 /* DSPCheckWriteAddressesXY(DSP_MS_X,startAddress,skipFactor,wordCount); */
    ec2 = DSPWriteArraySkipMode(fix24Array,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}


/********************************* Array Reads *******************************/

int DSPAPReadArray(
    DSPDatum *data,		/* array to fill from DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c */
    return(DSPReadArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32));
}

int DSPAPReadFix24Array(
    DSPDatum *data,		/* array to fill from DSP */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c */
    return(DSPReadArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
			   skipFactor,wordCount,DSP_MODE32));
}

int DSPAPReadPackedArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word */
    int wordCount)		/* DSP words = byte count / 3 */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
    				skipFactor,wordCount,DSP_MODE24));
}

int DSPAPReadByteArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount)		/* Same as DSP word count */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
    				skipFactor,byteCount,DSP_MODE8));
}

int DSPAPReadShortArray(
    short *data,		/* Packed data to fill from DSP */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short transferred */
    int wordCount)		/* DSP word count = byte count / 2 */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,DSP_MS_X,startAddress,
			       skipFactor,wordCount,DSP_MODE16));
}

int DSPAPReadIntArray(
    int *intArray,
    int startAddress,
    int skipFactor,
    int wordCount)
/*
 * Same as DSPReadFix24Array() followed by DSPFix24ToIntArray() for sign extension.
 */
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadFix24Array((DSPFix24 *)intArray,DSP_MS_X,startAddress,
			  skipFactor,wordCount);
    ec = DSPFix24ToIntArray(intArray,intArray,wordCount);
    return(ec);
}

int DSPAPReadIntArrayXY(
    int *intArray,
    int startAddress,
    int skipFactor,
    int wordCount)
/*
 * Same as DSPReadFix24Array() followed by DSPFix24ToIntArray() for sign extension.
 */
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)intArray,DSP_MS_X,startAddress,
    			      skipFactor,wordCount,DSP_MODE32);
    ec = DSPFix24ToIntArray(intArray,intArray,wordCount);
    return(ec);
}

    
int DSPAPReadFloatArray(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)floatArray,DSP_MS_X,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToFloatArray((DSPFix24 *)floatArray,floatArray,wordCount);
    return(ec);
}


int DSPAPReadFloatArrayXY(
    float *floatArray,
    int startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)floatArray,DSP_MS_X,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToFloatArray((DSPFix24 *)floatArray,floatArray,wordCount);
    return(ec);
}

int DSPAPReadDoubleArray(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)doubleArray,DSP_MS_X,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToDoubleArray((DSPFix24 *)doubleArray,doubleArray,wordCount);
    return(ec);
}


int DSPAPReadDoubleArrayXY(
    double *doubleArray,
    int startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)doubleArray,DSP_MS_X,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToDoubleArray((DSPFix24 *)doubleArray,doubleArray,wordCount);
    return(ec);
}
