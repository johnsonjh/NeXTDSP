/*	DSPLoad.c - Load user program into running DSP.
	Copyright 1988 NeXT, Inc.
	
	Modification history:
	03/06/88/jos - File created
	04/23/90/jos - Flushed DSPMKLoad()
	04/30/90/jos - Removed "r" prefix from rData, rWordCount, rRepeatCount
	
	*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

int DSPLoadFile(
    char *fn)
    /*
     * Load DSP from the file specified.
     * Equivalent to DSPReadFile followed by DSPLoad.
     *
     */
{
    int ec;
    DSPLoadSpec *ls;
    ec = DSPReadFile(&ls,fn);
    if (ec)
      return(ec);
    return DSPLoad(ls);
}

int DSPLoad(DSPLoadSpec *dspimg)	/* load code or NULL to read default */
{
    int dspack,sysver,sysrev,ec;
    register int curLC;
    register DSPSection *usr;
    register DSPDataRecord *dr;
    int nwords,i,ver,rev,la;
    
    if (!dspimg)
      return _DSPError(EDOM,"DSPLoad: null DSP pointer passed");
    
#if 0
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; *** DSPLoad simulation ***\n\n");
    
    if (_DSPTrace & DSP_TRACE_LOAD) 
      fprintf(stderr,"\tLoading user DSP code, version %d.0(%d).\n",	
	      dspimg->version,dspimg->revision);

    if(ec=DSPCheckVersion(&sysver,&sysrev))
      return _DSPError1(ec,"DSPLoad: DSPCheckVersion() has problems with "
			"DSP system %s",dspimg->module);
    
    if (sysver != dspimg->version || sysrev != dspimg->revision)
      _DSPError1(DSP_EBADVERSION,
		_DSPCat(_DSPCat("DSPLoad: *** WARNING *** Passed DSP "
				"load spec '%s' has version(revision) = ",
				_DSPCat(_DSPCVS(dspimg->version),
				_DSPCat(".0(",_DSPCVS(dspimg->revision)))),
			_DSPCat(") while DSP is running ",
				_DSPCat(_DSPCVS(sysver),
					_DSPCat(".0(",
						_DSPCat(_DSPCVS(sysrev),
							")"))))),
		 dspimg->module);
		
#endif
/******************************** LOAD DSP CODE *****************************/
    
    usr = DSPGetUserSection(dspimg);
    if (!usr) return(_DSPError(DSP_EBADLODFILE,
			       "libdsp/DSPLoad: No user code found "
			       "in DSP struct"));
    
    for (curLC=(int)DSP_LC_X; curLC<DSP_LC_NUM; curLC++) {
	
	if ( dr = usr->data[curLC] ) {
	    
	    la = dr->loadAddress + usr->loadAddress[curLC];
	    nwords = dr->repeatCount * dr->wordCount;
	    if (_DSPTrace & DSP_TRACE_LOAD) 
	      fprintf(stderr,"Loading %d words of user %s memory "
		      "at 0x%X:\n",
		      nwords,DSPLCNames[curLC],la);
	    
	    if (curLC != (int)dr->locationCounter)
	      _DSPError1(DSP_EBADDR,
			 "libdsp/DSPLoad: data record thinks its "
			 "memory segment is %s!",
			 (char *) DSPLCNames[(int)dr->locationCounter]);
	    
	    DSP_UNTIL_ERROR(DSPDataRecordLoad(dr));
	}
    }
    
    /* USER IS LOADED */
    
#if 0
    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; *** User is loaded ***\n");
    
    if (DSPCheckVersion(&ver,&rev))
      DSP_MAYBE_RETURN(_DSPError(DSP_ESYSHUNG,
				 "DSPLoad: DSP is not responding "
				 "after download"));
    
    if (_DSPTrace & DSP_TRACE_LOAD) 
      fprintf(stderr,"\tStart address = 0x%X.\n", dspimg->startAddress);

    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),";; *** Set start address ***\n\n");

#endif

    DSPCall(DSP_HM_SET_START,1,&(dspimg->startAddress));
    
    return(0);
}

