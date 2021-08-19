/* 
 * DSPAPLoadGo.c
 *	Initialize, load, and start an AP macro.
 *
 * Notes:
 *	- When prototypes are implemented for the AP library,
 *	  one should be included for DSPAPLoadGo().
 *
 * Modification History:
 *	6/29/89/mtm - created.
 *	7/22/89/jos - added skipFactor arg to 2 calls of DSPWriteArray().
 *	7/23/89/mtm - changed DSPWriteArray() to DSPWriteFix24Array().
 *		      Made all functions non-static.
 *	8/15/89/mtm - Added some fn externs.  JOS changed .dsp file read.
 *	6/08/90/jos - Changed DSPAwaitIdleState() to DSPAwaitHF3Clear().
 *		      Changed _DSPCat() to DSPCat().
 *		      Changed _DSPGetUserSection() to DSPGetUserSection().
 *	09/01/90/jos - Removed call to AwaitNotBusy() in MacroLoad()
 */
#include <stdlib.h>	/* for getenv() */
#include <dsp/dsp.h>
#import "dsp/DSPAPUtilities.h"

/* private functions from libdsp */
extern int _DSPError1();
extern int _DSPError();
extern int _DSPLnkRead();
extern int _DSPRelocateUser();
extern int _DSPRelocate();

/* From libdsp */
extern int DSPAPTimeLimit;


int DSPAPMacroInit(char *apfnfile, DSPLoadSpec **dspSPP)
/*
 * Load the AP binary file.
 *
 * DSPAPMacroInit - called to download a pre-assembled DSP AP macro.
 * *** NOTE *** By offsetting the load address
 * for spaces P and X, the AP module can be stacked onto a prior
 * set in DSP memory.  This is called "vector function chaining".
 * The end-cap installed afterward should be installed after the
 * stack and not after each one.
 */
{
    int ec;
    DSPSection *user;
    int olddeb;
    char *dspdir;
    char *sysapfnfile;
    char *sysapfnfile2;

    olddeb = DSPErrorLogIsEnabled();
    DSPDisableErrorLog();

    if(ec=_DSPLnkRead(dspSPP,apfnfile,1)) {
	if (apfnfile[0]=='/') {	/* absolute pathname */
	    if (olddeb)
	      DSPEnableErrorLog();
	    return _DSPError1(ec,
			      "DSPAPMacroInit: Could not read file %s",
			      sysapfnfile);
	}	  
	/* Try standard distribution */
	dspdir = DSPGetAPDirectory();
	sysapfnfile = DSPCat(dspdir,apfnfile);
	
	if(ec=_DSPLnkRead(dspSPP,sysapfnfile,1)) {
	    /* Try standard distribution */
	    if (!getenv("DSP")) { /* DSP env var not set */
		if (olddeb)
		  DSPEnableErrorLog();
		return _DSPError1(ec,
				  "DSPAPMacroInit: Could not read file %s",
				  DSPCat(apfnfile,
					 DSPCat(" or ",
						sysapfnfile)));
	    }	  
	    dspdir = DSPCat(DSP_SYSTEM_DIRECTORY,
			    DSP_AP_BIN_DIRECTORY);
	    sysapfnfile2 = DSPCat(dspdir,apfnfile);
	    
	    if(ec=_DSPLnkRead(dspSPP,sysapfnfile2,1)) {
		if (olddeb)
		  DSPEnableErrorLog();
		REMEMBER(Convert to _DSPErrorV here);
		return _DSPError1(ec,
				  "DSPAPMacroInit: Could not read file %s",
				  DSPCat(apfnfile,
					 DSPCat(" or ",
						DSPCat(sysapfnfile,
						       DSPCat(" or ",
						      sysapfnfile2)))));
	    }
	}
    }
    
    /* install USER load offsets in DSP memory */
    if (ec = _DSPRelocateUser(*dspSPP))
      return(_DSPError1(ec,
	"DSPAPMacroInit: Could not obtain load offsets. File=%s",apfnfile));

    /* Make room for "beg_ap" DSP Initialization code */
    user = DSPGetUserSection(*dspSPP);

    user->loadAddress[(int)DSP_LC_P] = DSPAPLoadAddress();

    /* do fixups in p code */
    if (ec = _DSPRelocate(*dspSPP))
      return(_DSPError1(ec,
	 "DSPAPMacroInit: Could not perform fixups for file %s",apfnfile));

    return(ec);
}


int DSPAPMacroLoad(char *fileName, DSPLoadSpec *dsp, int nArgs, int *argValues)
/*
 * Load macro code, end cap, and arguments to the DSP.
 */
{
    int endMain[] = {
        0,      	/* NOP opcode */
	0xAF080, 	/* JMP opcode */
	DSP_HM_MAIN_DONE
    };
    int ec, doneAddress;

    if (ec = DSPLoad(dsp))
        return(_DSPError1(ec,
               "DSPAPMacroLoad: Could not load file %s", fileName));

    doneAddress = DSPGetLastAddress(dsp, DSP_LC_P) + 1;

    if (ec = DSPWriteFix24Array(endMain, DSP_MS_P, doneAddress, 1,
                           sizeof(endMain) / sizeof(int)))
        return(_DSPError1(ec,
	       "DSPAPMacroLoad: Could not load end cap file %s "
	       "to DSP", fileName));

    /* Poke arguments (all assumed to be in X memory) */
    if (ec = DSPWriteFix24Array(argValues, DSP_MS_X, DSPAP_XLI_USR, 1, nArgs))
        return(_DSPError1(ec,
	       "DSPAPMacroLoad: Could not load args for file %s "
	       "to DSP", fileName));
    return(0);
}


int DSPAPMacroGo(char *fileName)
{
    if (DSPStart())
        return(_DSPError1(DSP_EBADDSPFILE,
	       "DSPAPMacroGo:  Could not start DSP after loading "
	       "from file %s", fileName));
    return(0);
}


int DSPAPLoadGo(char *fileName, DSPLoadSpec **dsp, int nArgs, int *argValues)
{
    /* char *dspFile = DSPCat(DSPAPGetLocalAPDirectory(), fileName); */
    char *dspFile = fileName;
    
    if (*dsp == NULL)
        DSP_UNTIL_ERROR(DSPAPMacroInit(dspFile, dsp));
    DSP_UNTIL_ERROR(DSPAPMacroLoad(dspFile, *dsp, nArgs, argValues));
    DSP_UNTIL_ERROR(DSPAPMacroGo(dspFile));
    DSP_UNTIL_ERROR(DSPAPAwaitNotBusy(0));
    return(0);
}
