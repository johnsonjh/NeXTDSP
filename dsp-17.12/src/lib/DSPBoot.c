/*	DSPBoot.c - Reset and boot-load DSP
	Copyright 1988 NeXT, Inc.

Modification history:
	03/06/88/jos - File created from _DSPUtilities.c
	09/10/88/jos - Changed DSP_MAYBE_RETURN to return.
		       Errors now fatal even when simulating.
	03/30/89/jos - Added DSPClose() if DSPIsOpen() on entry
	06/18/89/jos - Always requesting DSP memory diagnostics until 
		       init_bug is fixed.
	04/30/90/jos - Removed "r" prefix from rData, rWordCount, rRepeatCount
	10/31/90/jos - Inhibited DSP memory diagnostics w retry on error.
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

int DSPBootFile(char *fn)
{
    int ec;
    DSPLoadSpec *ls;
    ec = DSPReadFile(&ls,fn);
    if (ec)
      return(ec);
    return DSPBoot(ls);
}


int DSPBoot(DSPLoadSpec *system)
{
    int dspack;			/* message word from DSP */
    int i;
    int stdsys = 0;
    int weAlloc = 0;
    char *sysname;
    int ec;

    if (DSPIsOpen()) {
	_DSPError(0,"DSPBoot: DSP is already open. "
		  "Must close it before rebooting to get into reset state...");
	DSPClose();		/* Can't reboot if not in reset state! */
    }

    if (ec=DSPOpenNoBoot())
      return _DSPError(ec,"DSPBoot: DSPOpenNoBoot() failed");

    /* 
       If BUG56 already has the DSP open in mapped mode, then DSPOpenNoBoot()
       returned successfully, but the DSP is NOT in the reset state as we
       expect.  Therefore, we explicitly call DSPReset() just in case.
     */
       
    if (ec = DSPReset())
      return _DSPError(ec,"DSPBoot: DSPReset() failed");

    DSPFlushMessageBuffer();

    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; *** DSPBoot simulation ***\n\n");

    if (!system) {		/* load music system by default */
	char *sb,*s;
	stdsys=1;
	weAlloc=1;		/* flag deallocation on successful exit */
	sb = DSPGetSystemBinaryFile(); /* find default DSP monitor */
	if(DSPLoadSpecReadFile(&system,sb))	/* if we can't read it */
	  return(_DSPError1(DSP_EBADDSPFILE,
				"DSPBoot: Could not find %s",sb));
	sysname = sb;
	if (_DSPTrace & DSP_TRACE_BOOT) 
	  DSPLoadSpecPrint(system);
    }
    else sysname = system->module;
    if(system->module)
      if(strcmp(system->module,"MKMON8K")==0 ||
	 strcmp(system->module,"APMON8K")==0 ) 
	stdsys=1;
    
    DSPSetSystem(system); /* record what is loaded in the DSP for posterity */

    if (_DSPVerbose && !DSPIsSimulatedOnly()) 
      printf("\tBooting %s version %d.%d.\n",sysname,
	   system->version, system->revision);

    /*** FEED BOOTSTRAP PROG TO THE DSP ***/

    {
	DSPSection *sys,*glob;
	DSPDataRecord *dr;
	int ntotalwords,la,*code,curmem,i;
	int nxi_sys,nxe_sys;
	int nyi_sys,nye_sys;
	int nli_sys;
	int npi_sys,npe_sys;
	
	sys = system->systemSection;
	glob = system->globalSection;
	
	if (!sys) {
	    /* 
	      If the system was assembled in absolute mode instead of relative
	      mode, or if this is a .lnk file with no
	      sections, then code lives in GLOBAL section,
	      and all other sections are empty.	 The typical user will
	      create an absolute assembly, and all his code and data will
	      appear in the GLOBAL section, absolutely relocated.
	      
	     */
	    sys = glob;
	    if (!sys) 
	      return(_DSPError(DSP_EBADLODFILE,
			   "DSPBoot: No system code found in DSP struct"));
	}
	
/************************ LOAD DSP DSP BOOTSTRAP CODE ************************/

	/* The code below talks to DSPSMSRC/reset_boot.asm which is executed
	   by the DSP56000 bootstrap program after loading the on-chip p 
	   memory. */

	/* First load on-chip P memory for the bootstrap ROM program. */
	/* Since section is absolute, only P can occur (no PL or PH) */
	/* (cf. _DSPGetMemStr()) */
	
	curmem = (int) DSP_LC_P; 
	dr = sys->data[curmem];
	
	la = sys->loadAddress[curmem];
	if (la!=0) 
	  return(_DSPError(DSP_EBADLA,
	       "DSPBoot: DSP System p: bootstrap load address not 0"));

	la = dr->loadAddress;
	if (la!=0) 
	  return(_DSPError(DSP_EBADLA,
	     "DSPBoot: DSP System p: vector load address not 0"));
	
	/* Load on-chip program memory */

	npi_sys = dr->wordCount * dr->repeatCount;

	if (npi_sys>512) 
	  return(_DSPError(DSP_EBADDR,
	  "DSPBoot: DSP System bootstrap (first block) overflows 512 words"));

	if (_DSPTrace & DSP_TRACE_BOOT) 
	  printf("Loading %d words of on-chip program memory:\n",npi_sys);

	code = dr->data;	/* first code block */

	if (DSPIsSimulated()) 
	  fprintf(DSPGetSimulatorFP(),"\n;; *** Load bootstrap ***\n");

	for (i=0;i<dr->repeatCount;i++) {
	    if (ec=DSPWriteTXArray(code,dr->wordCount))
	      _DSPError(ec,"DSPBoot: DSPWriteTXArray failed for bootstrap");}

	DSPClearHF1();		/* Inhibit DSP RAM diagnostics */
	DSPSetHF0();		/* Turn off bootstrap load */
	/* We clear HF0 after reading back the memory diagnostic word */

	/* If this is not a standard bootstrap file, then no more p data
	   is waiting to be sent, in which case we can return successfully. */

	if (!dr->next) {
	    if (_DSPVerbose)
	      printf("DSPBoot: Nonstandard single segment DSP program "
		     "loaded successfully.\n");
	    DSPClearHF0(); /* Leave "paused" state (if any) */
	    return(0);
	}

	/* Now the DSP boot ROM has executed a JMP 0 instruction which places
	   execution in our special bootstrap reset handler 
	   $DSP/smsrc/reset_boot.asm.  The first thing it does is
	   send us a copy of R0 (the index register used by the boot ROM
	   to sequence through the 512 onchip program memory locations) */

	/* Read back boot-load pointer */
	if (DSPErrorNo=DSPReadRX(&dspack))
	  if (stdsys)
	    return _DSPError(DSPErrorNo,"DSPBoot: DSPReadRX failed reading R0");

	if (_DSPTrace & DSP_TRACE_BOOT) 
	  printf("DSP boot ROM loaded %d words.\n",dspack);

	if (stdsys && dspack!=npi_sys) {
	    return _DSPError(DSP_EPROTOCOL,
		     "DSPBoot: Bootstrap ROM did not load what we sent");
	}

    retry:
	if (ec=DSPReadRX(&dspack))	/* Read mem diagnostics results */
	  _DSPError(ec,"DSPBoot: DSPReadRX failed for fake mem diag ack");

	if (stdsys && dspack!=0) {
	    fprintf(stderr,
		    "*** Got fake DSP RAM diagnostic code %d instead of 0!\n",
		    dspack);
	    goto retry;		/* assume leading garbage */
	}

	DSPClearHF0();		/* Leave "paused" state (if any) */

	/*
	 * $DSP/smsrc/reset_boot.asm now wants the rest of the system.
	 * Note that three of the following four loads could be calls to 
	 * a single procedure, but some special error checking would be lost.
	 */
	
	/****************** Load off-chip program memory *********************/

	npe_sys = 0;

	while (dr = dr->next) {

	    la = dr->loadAddress;
	    ntotalwords = dr->wordCount * dr->repeatCount;
	    npe_sys += ntotalwords;
	    code = dr->data;	/* first code block */

	    if (_DSPTrace & DSP_TRACE_BOOT) 
	      printf("Loading %d words off-chip program memory at 0x%X:\n",
		     ntotalwords,dr->loadAddress);
	    
	    if(la<512) {
		_DSPError(DSP_EBADDR,
		      "DSPBoot: Expected external and got internal memory");
		npi_sys += ntotalwords;
		npe_sys -= ntotalwords;
		if (_DSPVerbose) 
		  printf(
     "\t(Revised) system internal program memory size (NPI_SYS) = %d words\n",
			 npi_sys);
	    }

	    if (DSPIsSimulated()) {
		fprintf(DSPGetSimulatorFP(),"\n;; *** Load off-chip program memory ***\n");
		fprintf(DSPGetSimulatorFP(),"4F00#200\t ; Allow time for ERAM diagnostics\n");
	    }

	    DSPWriteTX(DSP_MS_P);	/* Send memory space of transfer to DSP */
	    DSPWriteTX(la);	/* Send target address of transfer to DSP */
	    DSPWriteTX(ntotalwords);	/* Send size of transfer to DSP */
	    
	    for (i=0;i<dr->repeatCount;i++) {
		if (ec=DSPWriteTXArray(code,dr->wordCount))
		  _DSPError(ec,"DSPBoot: DSPWriteTXArray failed for pe memory");
	    }
	    if (ec=DSPReadRX(&dspack))	/* Read back boot-load pointer */
	      _DSPError(ec,"DSPBoot: DSPReadRX failed for pe memory");

	    if (dspack-la != ntotalwords) 
	      return _DSPError(DSP_EPROTOCOL,
		"DSPBoot: Failure loading external program memory");
	}

	if (_DSPVerbose) 
	  printf("\tSystem internal program memory size (NPI_SYS) = %d words\n",
	       npi_sys);

	if (_DSPVerbose) 
	  printf("\tSystem external program memory size (NPE_SYS) = %d words\n",
	       npe_sys);

	/********************* Load x data memory *************************/

	curmem = (int) DSP_LC_X;
	dr = sys->data[curmem];
	nxe_sys = nxi_sys = 0;

	while (dr) {
	    ntotalwords = dr->repeatCount * dr->wordCount;
	    /* internal + external memory */
	    
	    if (_DSPTrace & DSP_TRACE_BOOT) 
	      printf("Loading %d words of system x memory at 0x%X:\n",
		     ntotalwords,dr->loadAddress);
	    
	    if (DSPIsSimulated()) 
	      fprintf(DSPGetSimulatorFP(),"\n;; *** Load x memory ***\n");

	    DSPWriteTX(DSP_MS_X);
	    DSPWriteTX(dr->loadAddress);
	    DSPWriteTX(ntotalwords);
	    
	    for (i=0;i<dr->repeatCount;i++) {
		if (ec=DSPWriteTXArray(dr->data,dr->wordCount))
		  _DSPError(ec,"DSPBoot: DSPWriteTXArray failed for x memory");}

	    if (dr->loadAddress > 256) 
	      nxe_sys += ntotalwords;
	    else
	      nxi_sys += ntotalwords;
	    
	    if (ec=DSPReadRX(&dspack)) /* Read back load pointer */
	      _DSPError(ec,"DSPBoot: DSPReadRX failed for x memory");

	    if (dspack - dr->loadAddress != ntotalwords) 
	      return _DSPError(DSP_EPROTOCOL,
			       "DSPBoot: Failure loading x memory");
	    dr = dr->next;
	}

	if (_DSPVerbose) 
	  printf("\tSystem internal x data memory size (NXI_SYS) = %d words\n",
	       nxi_sys);
	if (_DSPVerbose) 
	  printf("\tSystem external x data memory size (NXE_SYS) = %d words\n",
	       nxe_sys);
	
	/********************* Load y data memory *************************/

	curmem = (int) DSP_LC_Y;
	dr = sys->data[curmem];
	nye_sys = nyi_sys = 0;

	while (dr) {
	    ntotalwords = dr->repeatCount * dr->wordCount;
	    /* internal + external memory */
	    
	    if (_DSPTrace & DSP_TRACE_BOOT) 
	      printf("Loading %d words of system y memory at 0x%X:\n",
		     ntotalwords,dr->loadAddress);
	    
	    if (DSPIsSimulated()) 
	      fprintf(DSPGetSimulatorFP(),"\n;; *** Load y memory ***\n");

	    DSPWriteTX(DSP_MS_Y);
	    DSPWriteTX(dr->loadAddress);
	    DSPWriteTX(ntotalwords);
	    
	    for (i=0;i<dr->repeatCount;i++) {
		if (ec=DSPWriteTXArray(dr->data,dr->wordCount))
		  _DSPError(ec,"DSPBoot: DSPWriteTXArray failed for y memory");
	    }

	    if (dr->loadAddress > 256) 
	      nye_sys += ntotalwords;
	    else
	      nyi_sys += ntotalwords;
	    
	    if (ec=DSPReadRX(&dspack)) /* Read back load pointer */
	      _DSPError(ec,"DSPBoot: DSPReadRX failed for y memory"); 

	    if (dspack - dr->loadAddress != ntotalwords) 
	      return _DSPError(DSP_EPROTOCOL,
				"DSPBoot: Failure loading y memory");

	    dr = dr->next;
	}

	if (_DSPVerbose) 
	  printf("\tSystem internal y data memory size (NYI_SYS) = %d words\n",
		 nyi_sys);
	if (_DSPVerbose) 
	  printf("\tSystem external y data memory size (NYE_SYS) = %d words\n",
		 nye_sys);
	
	/********************* Load l data memory *************************/

	curmem = (int) DSP_LC_L;
	dr = sys->data[curmem];
	nli_sys = 0;
	
	while (dr) {
	    ntotalwords = dr->repeatCount * dr->wordCount;
	    nli_sys += ntotalwords/2; /* wordCount is a count of ints! */
	    if (dr->loadAddress+nli_sys>256)
	      _DSPError(DSP_EBADLA,
			 "DSPBoot: Attempt to load offchip l memory");

	    if (_DSPTrace & DSP_TRACE_BOOT) 
	      printf("Loading %d words of on-chip l memory at 0x%X:\n",
		     nli_sys,dr->loadAddress);
	    
	    if (DSPIsSimulated()) 
	      fprintf(DSPGetSimulatorFP(),"\n;; *** Load l memory ***\n");

	    DSPWriteTX(DSP_MS_L);
	    DSPWriteTX(dr->loadAddress);
	    DSPWriteTX(nli_sys);	/* $DSP/smsrc/reset_boot.asm wants #longs */
	    
	    for (i=0;i<dr->repeatCount;i++) {
		if (ec=DSPWriteTXArray(dr->data,dr->wordCount))
		  _DSPError(ec,"DSPBoot: DSPWriteTXArray failed for l memory");}
	      /* $DSP/smsrc/reset_boot.asm takes two words in TX per long */

	    if (ec=DSPReadRX(&dspack)) /* Read back load pointer */
	      _DSPError(ec,"DSPBoot: DSPReadRX failed for l memory");

	    if (dspack-dr->loadAddress != nli_sys) 
	      return _DSPError(DSP_EPROTOCOL,
			"DSPBoot: Failure loading l memory");
	    dr = dr->next;
	}
	
	if (_DSPVerbose) 
	  printf("\tSystem internal l data memory size (NLI_SYS) = "
		 "%d lwords\n", nli_sys);
	
	DSPWriteTX(DSP_MS_N); /* Terminate reset_boot in dsp, cause real reset */
	    
/* SYSTEM IS LOADED AND PRESUMABLY RUNNING */
	if (DSPIsSimulated())  {
	    fprintf(DSPGetSimulatorFP(),"\n;; *** System is loaded ***\n");
	    fprintf(DSPGetSimulatorFP(),"4F00#60000 ; wait for reset\n");
	}

	/* Read "I am alive" message from DSP */
	if (ec=DSPAwaitMessage(DSP_DM_IDLE,10*DSPDefaultTimeLimit))
	  return _DSPError(ec,
			       "DSPBoot: System does not claim to be alive "
			       "after bootstrap.");
	
	while(DSPDataIsAvailable()) {
	    if(!DSPMessageGet(&dspack))
	      _DSPError1(DSP_EPROTOCOL,
			     "DSPBoot: uninterpreted DSP message '%s' found.",
			     DSPMessageExpand(dspack));
	}	
	
	if (system->startAddress) {
	    if (_DSPVerbose) 
	      printf("\tStarting loaded image at location 0x%X.\n",
		     system->startAddress);
	    if (ec=DSPWriteTX(system->startAddress))
	      _DSPError(ec,"DSPBoot: DSPWriteTX failed for start address");
	    if (ec=DSPHostMessage(DSP_HM_GO))
	      _DSPError(ec,"DSPBoot: DSPHostMessage failed for DSP_HM_GO");
	} 

	if (_DSPVerbose) 
	  printf("\tDSP up and running.\n");

	/* We never want to free the system struct, because a pointer
	   to it has been registered with DSPObject.c.	It could be 
	   deallocated there under DSPClose(), but for that we'll
	   have to also register the "weAlloc" variable above which
	   when true gives us deallocation responsibility:

	   if (weAlloc)
	       DSPLoadSpecFree(system);

	   Since DSPControl.c also does the allocation, we really need
	   to pass a "deallocation token" to DSPObject.c 

	 */

	return 0;
    }
}

