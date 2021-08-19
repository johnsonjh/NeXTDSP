/*	DSPReadFile.c - load struct of type DSPLoadSpec from .lnk file
	Copyright 1987 NeXT, Inc.

	The Motorola DSP56000/1 assembler (program /usr/next/bin/asm56000)
	generates a relocatable object file with filename extension '.lnk'.
	This function reads such a file and loads the fields of the DSPLoadSpec
	data structure, which is defined in dspstructs.h.  

	If the file extension is '.lod', it is treated as a nonrelocatable
	("absolute") assembly. Only "SYMOBJ" symbols are provided with absolute
	assembly. In this case, all data appears in the globalSection. (For
	absolute assembly, there is no such thing as a section.)

	Modification history:
	12/20/87/jos - File created
	01/12/88/jos - .lod is now supported with only a warning
	01/14/88/jos - Counters *L:,*H:,*: merged to *: for absolute sections
	01/15/88/jos - Added '{' support to _BLOCKDATA, rewrote error handling
	02/02/88/jos - Changed default section for absasm from USER to GLOBAL.
			This was needed because relative assembly where no
			sections are used yields everything in GLOBAL section.
			Now relatively assembled absolute sections and 
			absolutely assembled (sectionless) code go to 
			the same place.	 DSPBoot() (_DSPUtilities.c) wants this.
	02/12/88/jos - Added load of dr->constant0 for l BLOCKDATA records.
	02/19/88/jos - Changed DSPDataBlock to DSPDataRecord and reformatted.
	04/18/88/jos - Added literal support ('[' parsed in relocation spec)
	04/22/88/jos - Added fixup decrement support (for DO loops)
	05/16/88/jos - Flushed preallocation of symbol arrays using seg sizes!
	05/24/88/jos - Added dr->loadAddress to refno in _DSPGetRelIntHexStr.
	07/22/88/jos - Fixed bug in loading of long _BLOCKDATA constant
	07/29/88/jos - Added support of long _DATA constant loads
	10/07/88/jos - Added support for '.dsp' extension as well as '.img'
	06/03/89/jos - Modified warning messages regarding unresolved symbols
	06/03/89/jos - Changed "printf(" to "fprintf(stderr,"
	04/23/90/jos - Flushed _DSPLnkReadMusic()
	04/30/90/jos - Removed "r" prefix from rData, rWordCount, rRepeatCount
	08/24/90/daj/jos - (replace-string "[2]" "[(absasm?0:2)]" nil)
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

int DSPReadFile( DSPLoadSpec **dsppp, char *fn)
{
    int olddeb,ec;
    char *dspdir;
    char *sysfn;

    olddeb = DSPErrorLogIsEnabled();
    DSPDisableErrorLog();
    if(ec=_DSPLnkRead(dsppp,fn,1)) {
	/* User doesn't have it... try to find a system version */
	dspdir = DSPGetSystemDirectory();
	sysfn = DSPCat(dspdir,fn);
	
	if(ec=_DSPLnkRead(dsppp,sysfn,1)) {
	    if (olddeb)
	      DSPEnableErrorLog();
	    return(_DSPError1(ec,
			      "DSPReadFile: Could not read file ./%s",
			      DSPCat(fn,DSPCat(" or ",sysfn))));
	}
    }
    if (olddeb)
      DSPEnableErrorLog();
    return(0);
}

int _DSPLnkRead(dspptr,lnkfn,sys)/* load DSP struct from .lnk or .lod file */
    DSPLoadSpec **dspptr;		/* struct containing entire DSP load image */
    char *lnkfn;		/* asm56000 output file = linker input file */
    int sys;			/* TRUE to load SYSTEM and GLOBAL variables */
{
    DSPLoadSpec *dsp;		/* struct containing entire DSP load image */
    FILE *lnkfp=NULL;		/* relocatable link file pointer */
    char *tok,*c,*record,*rp,*lnkfb,*lnkfe,*lnkfn2;
    int absasm=0;		/* 0=".lnk", 1=".lod" */

    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) fprintf(stderr,"\n_DSPLnkRead\n");

    DSP_MALLOC(dsp,DSPLoadSpec,1);	/* allocate a dsp struct */
    DSPLoadSpecInit(dsp);		/* initialize record fields to defaults */

    if (!lnkfn)
      return(_DSPError(DSP_EBADLNKFILE,
			   "_DSPLnkRead: input file name is NULL"));

    lnkfn = _DSPFirstReadableFile(lnkfn2=lnkfn,4,".dsp",".img",".lod",".lnk");
    if (!lnkfn)
      return(_DSPError1(DSP_EBADLNKFILE,
	  "_DSPLnkRead: input file %s not found",lnkfn2));

    lnkfe = _DSPGetTail(lnkfn);

    if (strcmp(lnkfe,".dsp")==0 || strcmp(lnkfe,".img")==0)
	return (DSPLoadSpecReadFile(dspptr,lnkfn));

    if (strcmp(lnkfe,".lod")==0)
      absasm = 1;
    else {
	absasm=0;
	if (strcmp(lnkfe,".lnk")!=0) {
	  _DSPError1(DSP_EBADFILETYPE,
				"_DSPLnkRead: Bad DSP file extension "
				"'%s'. Expect '.dsp', '.lod', or '.lnk'."
				" Assuming type '.dsp'", lnkfe);
	  return (DSPLoadSpecReadFile(dspptr,lnkfn)); /* Try it anyway */
      }
    }
	
    if (!lnkfp)
      if ((lnkfp=_DSPMyFopen(lnkfn,"r"))==NULL)
	return(_DSPError1(ENOENT,
			      "_DSPLnkRead: input file %s not found",lnkfn));

    if (_DSPVerbose)
      fprintf(stderr,"\tReading linker file:\t\t%s\n",lnkfn);
	
    /* For absolute assembly, we have to create the GLOBAL section in
       advance, because there is no _SECTION directive to set it up.
     */
    if (absasm) {
	int i,count;
	DSPSection *sec;
	DSP_MALLOC(sec,DSPSection,1);
	DSPSectionInit(sec);
	sec->name   = "GLOBAL";
	sec->type   = "A";	/* absolute (as opposed to relative) */
	sec->number = (int) DSP_Global;
	dsp->indexToSection[sec->number] = sec;
	dsp->globalSection = sec;
	dsp->type = "A";
    } else
	dsp->type = "R";

    /* READ LINES FROM INPUT FILE */
    while ((record=_DSPFGetRecord(lnkfp))!=NULL) {

	/* Process a record.  Types are
	   START       SECTION	 SYMBOL	   DATA
	   BLOCKDATA   XREF	 COMMENT   END
	 */

	rp = record; /* record sits still, rp traverses string */
	tok = _DSPGetTokStr(&rp); /* record name (excluding underbar) */

/* LOAD _SYMBOL RECORD */
	if (strcmp(tok,"SYMBOL")==0) { /* snarf symbol definitions */
	    char *symname;	/* symbol name */
	    char *symtype;	/* symbol type = (L|G)(A|R)(I|F) */
	    int secno;		/* section number */
	    int symno;		/* symbol number */
	    int symmem;		/* memory segment number */
	    static DSPSection *sec; /* DSP section involved */

	    /* _SYMBOL		 <Section number>   <Memory>
	       < <Symbol name>	 <Symbol type>	    <Symbol Value> >
			   . . .
	       For .lod files, first line is without section number.
	     */

	    /* Note that absolutely assembled files have no symbols (yet) */
	    secno = absasm?(int)DSP_Global:_DSPGetIntHexStr(&rp);

	    /* If symbols ever do exist in .lod files, GLOBALs always pass */
	    if ( !absasm && sys==FALSE && secno!=(int)DSP_User ) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD)
		  fprintf(stderr,"Skipping %s _SYMBOL record in file %s\n",
			 DSPSectionNames[secno],lnkfn);
		goto nextRecord;
	    }

	    if (secno==_DSP_NOT_AN_INT) 
	      return(_DSPError1(DSP_EBADLNKFILE,
		 "_DSPLnkRead: no section no. in %s",
		 _DSPReCat(_DSPGetLineStr(&record)," . . .\n")));
	    sec = dsp->indexToSection[secno];
	    if (sec==NULL) {
		return(_DSPError1(DSP_EBADLNKFILE,
		 "_DSPLnkRead: symbol references unknown section in %s",
		 _DSPReCat(_DSPGetLineStr(&record)," . . .\n")));
	    }

	    symmem = (int) _DSPGetMemStr(&rp,sec->type); /* memory type code */

	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      fprintf(stderr,"\n_SYMBOL %d %s\n",secno,DSPLCNames[symmem]);

	    while ((symname=_DSPGetTokStr(&rp))!=NULL) {
		if ( sec->symCount[symmem]
		    >= sec->symAlloc[symmem] ) {
		    sec->symAlloc[symmem] += 100;
		    NX_REALLOC(sec->symbols[symmem],DSPSymbol,
			       sec->symAlloc[symmem]);
		}
		symno = sec->symCount[symmem]++;
		sec->symbols[symmem][symno].name = symname;
		sec->symbols[symmem][symno].locationCounter 
		  = (DSPLocationCounter) symmem;

		symtype = _DSPGetTokStr(&rp);
		sec->symbols[symmem][symno].type = symtype;
		if (symtype[(absasm?0:2)]=='F')
		  sec->symbols[symmem][symno].value.f 
		    = _DSPGetFloatStr(&rp);
		else
		  sec->symbols[symmem][symno].value.i = _DSPGetIntHexStr(&rp);

		if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
		  DSPSymbolPrint(sec->symbols[symmem][symno]);

	    }			/* successive symbols in _SYMBOL record */
	    goto nextRecord;	/* get-record loop */
	}			/* _SYMBOL record */

/* LOAD _DATA RECORD */
	/* _DATA <section-number> <space> <start-address> <word1> <word2> ...*/
	if (strcmp(tok,"DATA")==0) {
	    int secno;
	    int space;
	    int i,datum,ndata,odata;
	    int *data;
	    DSPDataRecord *dr;
	    secno = absasm?(int)DSP_Global:_DSPGetIntHexStr(&rp);
	    if ( !absasm && sys==FALSE && secno!=(int)DSP_User ) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD)
		  fprintf(stderr,"skipping %s _DATA record in file %s\n",
			 DSPSectionNames[secno],lnkfn);
		goto nextRecord;
	    }
	    DSP_MALLOC(dr,DSPDataRecord,1);
	    DSPDataRecordInit(dr);
	    dr->section = dsp->indexToSection[secno];
	    dr->locationCounter = _DSPGetMemStr(&rp,dr->section->type);
	    space = (int) dr->locationCounter;
	    dr->loadAddress = _DSPGetIntHexStr(&rp);

	    ndata = _DSP_EXPANDSIZE;
	    DSP_MALLOC(data,int,ndata);
	    odata = 0;
	    while (1) {
		for (i=0;i<_DSP_EXPANDSIZE;i++) {
		    char *sym;
		    rp = _DSPSkipWhite(rp);
		    if (*rp=='{') {
			_DSPGetRelIntHexStr(&rp,dr,odata+i); /* below */
			datum = DSP_UNKNOWN;
		    } else datum=_DSPGetIntHexStr6(&rp); /* read at most 6 chars */
		    /* In the case of long (l:) data, the 12 hex characters 
		       are broken up into two groups of 6.  Thus, two ints
		       are generated for each long datum specification. */
		    if (datum==_DSP_NOT_AN_INT)
		      break;
		    data[odata+i] = datum;
		}
		if (datum==_DSP_NOT_AN_INT) {
		    ndata += (i-_DSP_EXPANDSIZE);
		    break;
		}
		ndata += _DSP_EXPANDSIZE;
		odata += _DSP_EXPANDSIZE;
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
		  fprintf(stderr,"...expanding for DATA block...\n");
		NX_REALLOC(data,int,ndata);
	    }
	    dr->data = data;
	    dr->wordCount = ndata;
	    dr->repeatCount = 1;

	    DSPDataRecordInsert(dr,&(dr->section->data[space]),
			    &(dr->section->dataEnd[space]));

	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      DSPDataRecordPrint(dr);

	    goto nextRecord;	/* get-record loop */
	}			/* _DATA record */

/* LOAD _BLOCKDATA RECORD */
	if (strcmp(tok,"BLOCKDATA")==0) {
	    int secno;
	    int space;
	    int i,datum,ndata;
	    int *data;
	    int isLong,nw;
	    DSPDataRecord *dr;

	    secno = absasm?(int)DSP_Global:_DSPGetIntHexStr(&rp);
	    if ( !absasm && sys==FALSE && secno!=(int)DSP_User ) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD)
		  fprintf(stderr,"Skipping %s _BLOCKDATA record in file %s\n",
			 DSPSectionNames[secno],lnkfn);
		goto nextRecord;
	    }
	    DSP_MALLOC(dr,DSPDataRecord,1);
	    dr->section = dsp->indexToSection[secno];
	    dr->locationCounter = _DSPGetMemStr(&rp,dr->section->type);
	    space = (int) dr->locationCounter;
	    dr->loadAddress = _DSPGetIntHexStr(&rp);
	    dr->repeatCount = _DSPGetIntHexStr(&rp);
	    isLong = (space >= (int)DSP_LC_L && space <= (int)DSP_LC_LH);
	    if (isLong)
	      nw = 2;
	    else
	      nw = 1;
	    /* THE FOLLOWING FAILS BECAUSE OF A GCC BUG */
	    /* nw = 1 + (isLong?1:0); */
	    dr->wordCount = nw;
	    DSP_MALLOC(dr->data,int,nw);
	    rp = _DSPSkipWhite(rp);
	    if (*rp=='{') {
		_DSPGetRelIntHexStr(&rp,dr,0);
		dr->data[0] = DSP_UNKNOWN;
	    } else
	      dr->data[0] = _DSPGetIntHexStr6(&rp); /* read at most 6 hex chars */
	    /* The test below circumvents an assembler bug/feature. */
	    /* The DS directive in relative mode issues a null _BLOCKDATA. */
	    /* For example, "foo DS 1" generates "_BLOCKDATA 0 P 0 1" */
	    /* This "feature" is to allow gap-creation in a .lnk file */
	    if (dr->data[0] == _DSP_NOT_AN_INT)
	      ;			/* no data, just skipping memory thank you */
	    else {
		if (isLong) {
		    /* In this case, the first half of the 12-hex-digit
		       constant has been read into dr->data[0], and
		       the second half of the constant is waiting to be
		       read in the input string. */
		    dr->data[1] = _DSPGetIntHexStr(&rp);
		}
		DSPDataRecordInsert(dr,&(dr->section->data[space]),
				    &(dr->section->dataEnd[space]));
	    }

	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      DSPDataRecordPrint(dr);

	    goto nextRecord;	/* get-record loop */
	}			/* _BLOCKDATA record */

/* LOAD _SECTION RECORD */
	if (strcmp(tok,"SECTION")==0) {
	    int i,count;
	    DSPSection *sec;

	    /* _SECTION	  <Section name> <Section type=A|R> <Section number>
		<X default size>      <X low size>	    <X high size>
		<Y default size>      <Y low size>	    <Y high size>
		<L default size>      <L low size>	    <L high size>
		<P default size>      <P low size>	    <P high size>
		For .lod files, there is no section record.
	     */

	    DSP_MALLOC(sec,DSPSection,1);
	    DSPSectionInit(sec);

	    /* read section id */	  
	    sec->name	= _DSPGetTokStr(&rp);
	    sec->type	= _DSPGetTokStr(&rp);
	    sec->number = _DSPGetIntHexStr(&rp);

	    if ( !absasm && sys==FALSE && sec->number!=(int)DSP_User) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD)
		  fprintf(stderr,"Skipping %s _SECTION specification in file %s\n",
			 DSPSectionNames[sec->number],lnkfn);
		goto nextRecord;
	    }

	    /* read resource requirements */	    
/* This is currently disabled because we don't use the information.
   It could be used when loading a _DATA record assembled in relative
   mode.  This would save extra calls to realloc.  However, since we have
   no size info specified in advance for absolutely assembled files,
   this would introduce a second mechanism where one exists now. Thus,
   we throw away segment size information at present.
*/
/* !!!	    sec->segmentSize[0] = 0; /* N segment assoc with no mem space */
	    for (i=1;i<DSP_LC_NUM;i++) {
		count = _DSPGetIntHexStr(&rp);
		if (count==_DSP_NOT_AN_INT) 
		  return(_DSPError(ENOMEM,
		    "_DSPLnkRead: died reading section mem space alloc"));
/* !!!		sec->segmentSize[i] = count; */
	    }

	    /* Initialize pointer to sorted list of data loads */
	    for (i=0;i<DSP_LC_NUM;i++)
		sec->data[i] = NULL;

	    /* INSTALL SECTION IN DSP STRUCT */

	    dsp->indexToSection[sec->number] = sec;

	    switch (sec->number) {
	      case DSP_Global:
		dsp->globalSection = sec;
		break;
	      case DSP_System:
		dsp->systemSection = sec;
		break;
	      case DSP_User:
		dsp->userSection = sec;
		break;
	      default:
		fprintf(stderr,
 "_DSPLnkRead: Only GLOBAL, SYSTEM, and USER sections currently supported.\n");
		goto nextRecord;
	    }


	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) {
		fprintf(stderr,"\nSection %s\n",sec->name); /* print as built */
		fprintf(stderr,"\tType = %s\n",*sec->type=='R'?
		       "Relative":"Absolute");
		fprintf(stderr,"\tNumber = %d\n",sec->number);
		for (i=0;i<DSP_LC_NUM;i++)
		  fprintf(stderr,"\t\tN%s = %d\n",DSPLCNames[i],sec->symAlloc[i]);
	    }

	    goto nextRecord;	/* get-record loop */
	}			/* _SECTION record */

/* LOAD _START RECORD */
	if (strcmp(tok,"START")==0) { /* module, version, rev, errors */
	    char *module;	/* name (set by ident statement label) */
	    int version;	/* version  number (ident statement 1st arg) */
	    int revision;	/* revision number (ident statement 2nd arg) */
	    int errorCount;	/* assembly error count (from asm56000) */
	    char *description;

	    module = _DSPGetTokStr(&rp);
	    dsp->module = module;

	    version = _DSPGetIntHexStr(&rp);
	    dsp->version = version;

	    revision = _DSPGetIntHexStr(&rp);
	    dsp->revision = revision;

	    if (!absasm) {
		errorCount = _DSPGetIntHexStr(&rp);
		dsp->errorCount = errorCount;
	    }

	    description = _DSPCopyStr(rp);
	    dsp->description = description;
	
	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      fprintf(stderr,"_START %s %d %d %d \n/%s/\n",
			      dsp->module,dsp->version,dsp->revision,
			      dsp->errorCount,dsp->description);
	    goto nextRecord;
	}

/* LOAD _END RECORD */
	if (strcmp(tok,"END")==0) {
	    /* _END <start-address> */
	    dsp->startAddress = _DSPGetIntHexStr(&rp);
	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      fprintf(stderr,"Start address set to %d\n",dsp->startAddress);
	    goto nextRecord;	/* get-record loop */
	}			/* _END record */

/* LOAD _COMMENT RECORD */
	if (strcmp(tok,"COMMENT")==0) {
	    /* _COMMENT <start-address> */
	    while(*rp++!='\n') ; /* delete leading newline */
	    dsp->comments = _DSPReCat(dsp->comments,DSPCat("\n",_DSPGetLineStr(&rp)));
	    goto nextRecord;	/* get-record loop */
	}			/* _COMMENT record */

/* LOAD _XREF RECORD */
	if (strcmp(tok,"XREF")==0) {
	    char *xrefname;	/* name of extern symbol used in this section */
	    int secno;		/* section number of this section */
	    static DSPSection *sec; /* address of this section */

	    /* _XREF <section number> <name> <name> ... */

	    secno = _DSPGetIntHexStr(&rp);
	    if ( !absasm && sys==FALSE && secno!=(int)DSP_User ) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD)
		  fprintf(stderr,"Skipping %s _XREF record in file %s\n",
			 DSPSectionNames[secno],lnkfn);
		goto nextRecord;
	    }
	    if (secno==_DSP_NOT_AN_INT) {
		return(_DSPError1(DSP_EBADLNKFILE,
		   "_DSPLnkRead: no section no. in %s\n",
			_DSPReCat(_DSPGetLineStr(&record)," . . .\n")));
	    }
	    sec = dsp->indexToSection[secno];
	    if (sec==NULL)
	      return(_DSPError1(DSP_EBADLNKFILE,
		 "_DSPLnkRead: xref references unknown section in \n%s\n",
			 _DSPReCat(_DSPGetLineStr(&record)," . . .\n")));
	    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
	      fprintf(stderr,"\n_XREF %d \n",secno);
	    while ((xrefname=_DSPGetTokStr(&rp))!=NULL) {
		if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
		  fprintf(stderr,"\t%s\n",xrefname);
		if (sec->xrefCount>=sec->xrefAlloc) {
		    sec->xrefAlloc += 20;
		    NX_REALLOC(sec->xrefs,char*,sec->xrefAlloc);
		}
	    }
	    sec->xrefs[sec->xrefCount++] = xrefname;
	    goto nextRecord;	/* get-record loop */
	}			/* _XREF record */

nextRecord:
	NX_FREE(record);
    }				/* get-record loop */
    fclose(lnkfp);
    
    /* .lnk file read. Pass 1 complete. */
    /* Now resolve relocatable symbol refs (Pass 2) */
    {
	int i,j,k,l,m,addr;
	int spc;
	DSPSection *sec;

	/* First merge data records into contiguous blocks */
	/* In relative mode this is guaranteed to produce one data record */
	/* for each location counter.  This assumption is checked below. */

	for (i=0;i<DSP_N_SECTIONS;i++) {
	    sec = dsp->indexToSection[i];
	    if (sec!=NULL)
	      for (j=0;j<DSP_LC_NUM;j++)
		DSPDataRecordMerge(sec->data[j]);
	}

	if ( _DSPTrace & DSP_TRACE_DSPLNKREAD ) {
	    fprintf(stderr,
"------------------- _DSPLnkRead: Finished merging data records -----------\n");
	    DSPLoadSpecPrint(dsp);
	    fprintf(stderr,
"------------------- _DSPLnkRead: Starting pass 2 for fixups --------------\n");
	}

	for (i=0;i<DSP_N_SECTIONS;i++) {
	    sec = dsp->indexToSection[i];
	    if (sec!=NULL) {
		if (sec->xrefCount > 0) {
		    fprintf(stderr,"\n*** Warning - Fixups for %d XREFs (relative-"
			   "section cross-references) not generated ***\n",
			   sec->xrefCount);
		    fprintf(stderr,"
These are probably just undefined symbols in your DSP source.
Otherwise, you are trying to reference a relocatable symbol which lives
in a different section than the one being processed (so-called XREFs).
XREFs are not supported.\n");

		    /* To do this, a symbol table search like the below must
		       be done for each section. NeXT DSP software does not
		       need the XREF feature because only the USER section
		       is relocatable. */
		}
		if (_DSPTrace & DSP_TRACE_FIXUPS) 
		  fprintf(stderr,"\nRelocatable symbol fixups for %s section:\n",
			 DSPSectionNames[i]);
		for (m=0;m<DSP_LC_NUM_P;m++) {
		    if (sec->fixupCount[m]<=0) continue;
		    if (sec->data)
		      if (sec->data[m+(int)DSP_LC_P]->next!=NULL)
			_DSPError(DSP_EBADLNKFILE,
			      DSPCat("_DSPLnkRead assumes relocatable references ",
				  DSPCat("happen only in relocatable sections\n",
				      "which implies only one data segment")));
		    for (j=0;j<sec->fixupCount[m];j++) {
			char *symname = sec->fixups[m][j].name;
			/* Find symbol in THIS SECTION'S sym tab (NO XREF!) */
			for (k=0;k<DSP_LC_NUM;k++)
			  for (l=0;l<sec->symCount[k];l++){
			      if (strcmp(symname,
					 sec->symbols[k][l].name)==0) {
				  spc=(int)sec->symbols[k][l].locationCounter;
				  if(spc!=k)fprintf(stderr,DSPCat("*** FOUND SPACE %d ",
				    "SYMBOL IN SYM TAB FOR SPACE %d ***\n"),
						   spc,k);
				  addr = sec->symbols[k][l].value.i;
				  goto foundSymbol;
			      }
			  }
			fprintf(stderr,"\n*** Could not resolve symbol '%s' ***\n",
			       symname);
		      foundSymbol:
			sec->fixups[m][j].relAddress = 
			  sec->fixups[m][j].decrement?addr-1:addr;
			sec->fixups[m][j].locationCounter = (DSPLocationCounter)spc;

			/* Do relocation according to current loadAddress */
			sec->data[m+(int)DSP_LC_P]->data[sec->fixups[m][j].refOffset]
			= sec->loadAddress[(int)sec->fixups[m][j].locationCounter] 
			  + sec->fixups[m][j].relAddress;

			if (_DSPTrace & DSP_TRACE_FIXUPS) 
			  fprintf(stderr,"\t%s[%d] = loadAddress(%s)+%d	 (%s/%s:%s)\n",
			     DSPLCNames[m+(int)DSP_LC_P],
			     sec->fixups[m][j].refOffset,
			     DSPLCNames[(int)sec->fixups[m][j].locationCounter],
			     sec->fixups[m][j].relAddress,
			     DSPSectionNames[i],DSPLCNames[spc],symname);
		    }
		}
	    }
	}

	if ( _DSPTrace & DSP_TRACE_DSPLNKREAD ) {
	    fprintf(stderr,
"---------------------- _DSPLnkRead: Finished reading input file -----------\n");
	    DSPLoadSpecPrint(dsp);
	    fprintf(stderr,
"---------------------- _DSPLnkRead: Returning ---------------------\n");
	}
    }

    *dspptr = dsp;

    return(0);
}

char *_DSPUniqueName(stub)	/* return DSPCat(stub,"<num>") where num is unique */
    char *stub;
{
    static int cnt = 0;
    return(DSPCat(stub,_DSPCVS(cnt)));
}
    
char *_DSPAddRelSymbol(sec,lc,symname,symtype,ival,fval) 
    DSPSection *sec;				/* create symbol table entry */
    DSPLocationCounter lc;
    char *symname;
    char *symtype;
    int	  ival;
    float fval;
{
    int mem = (int) lc;
    int symno;

    if ( sec->symCount[mem] >= sec->symAlloc[mem] ) {
	sec->symAlloc[mem] += 100;
	NX_REALLOC(sec->symbols[mem],DSPSymbol,
		   sec->symAlloc[mem]);
    }
    symno = sec->symCount[mem]++;
    sec->symbols[mem][symno].name = symname;
    sec->symbols[mem][symno].locationCounter = lc;
    
    sec->symbols[mem][symno].type = symtype;
    if(strlen(symtype)!=3)
      fprintf(stderr, "*** _DSPAddRelSymbol: "
	      "Expecting relative assembly only\n");
    if (symtype[2]=='F')
      sec->symbols[mem][symno].value.f = fval;
    else
      sec->symbols[mem][symno].value.i = ival;
    
    if (_DSPTrace & DSP_TRACE_DSPLNKREAD) 
      DSPSymbolPrint(sec->symbols[mem][symno]);

    return (symname);

}
    
int _DSPGetRelIntHexStr(rpp,dr,refno) /* get relocatable int from hex string */
    char **rpp;			/* string containing input character stream */
				/* **rpp=='{', 1st char of a relocatable ref */
    DSPDataRecord *dr;		/* data block to be loaded */
    int refno;			/* offset of datum within dr */
/*
 *  Read relocatable symbol reference from input stream and install the
 *  corresponding fixup in the fixup table.
 */
{
    DSPSection *sec;
    int pmem,fixupno;
    int decrement;
    char *sym;
    sec = dr->section;
    if (*sec->type!='R')
      _DSPError(DSP_EBADLNKFILE,
 "_DSPLnkRead: Attempt to load relative assembly into non-relative section");
    if (*(*rpp)++ != '{')
      return _DSPError1(DSP_EBADLNKFILE,
	"_DSPLnkRead: illegal .lnk input format:\n\t%s\nExpect '{name}@0'",
	    --*rpp);
    if (**rpp=='[') { /* literal: e.g. {[$00000025]}@0 => $25+relocation */
	int val;
	*rpp += 1;		/* skip over '[' */
	val = _DSPGetIntHexStr(rpp);
	sym = _DSPAddRelSymbol(sec,DSP_LC_P,_DSPUniqueName("Literal"),
			    "LRI",val,0.0);
	if (**rpp!=']') 
	  _DSPError1(DSP_EBADLNKFILE,
	 "\n*** Possible parse error in {[literal]}@0-1 specification: %s\n",
		  DSPCat(DSPCat("{",sym),*rpp));
	*rpp +=1;  /* skip over ']' */
    }
    else
      sym = _DSPGetTokStr(rpp); /* pre-existing symbol */
    if (**rpp!='}') 
      _DSPError1(DSP_EBADLNKFILE,
 "\n*** Possible parse error in {relocatableSymbol}@0 spec ***\n\t%s\n",
		  DSPCat(DSPCat("{",sym),*rpp));
    *rpp +=1;  /* skip over '}' */

    if (**rpp!='@') 
      _DSPError1(DSP_EBADLNKFILE,
 "\n*** Possible parse error in {relocatableSymbol}@0 spec ***\n\t%s\n",
		  DSPCat(DSPCat("{",sym),*rpp));
    *rpp +=1;  /* skip over '@' */

    if (**rpp!='0') 
      _DSPError1(DSP_EBADLNKFILE,
 "\n*** Possible parse error in {relocatableSymbol}@0 spec ***\n\t%s\n",
		  DSPCat(DSPCat("{",sym),*rpp));
    *rpp +=1;  /* skip over '0' */

    if (**rpp=='-')  { /* "{relocSym}@0-1" is supported for DO loops */
	*rpp +=1;  /* skip over '-' */
	decrement = _DSPGetIntHexStr(rpp);
	if (decrement != 1)
	  _DSPError1(DSP_EBADLNKFILE,
	  "\n Only subtraction of 1 supported in {relocSymb}@0-1 SPEC\n\t%s\n",
		  DSPCat(DSPCat("{",sym),*rpp));
	decrement = 1;
    }
    else
      decrement = 0;

    *rpp = _DSPSkipToWhite(*rpp);

    /* record name and offset of relocatable symbol ref */
    pmem = (int) dr->locationCounter - (int)DSP_LC_P;
    if (sec->fixupCount[pmem]>=sec->fixupAlloc[pmem]) {
	sec->fixupAlloc[pmem] += 20;
	NX_REALLOC(sec->fixups[pmem],DSPFixup,
		   sec->fixupAlloc[pmem]);
    }
    fixupno = sec->fixupCount[pmem]++;
    sec->fixups[pmem][fixupno].name = sym;
    sec->fixups[pmem][fixupno].decrement = decrement;
    sec->fixups[pmem][fixupno].refOffset = refno + dr->loadAddress;
    sec->fixups[pmem][fixupno].relAddress = DSP_UNKNOWN;
    /* we find relocation address in sym tab on pass 2 */

    return(0);
}


/* _DSPGetIntHexStr6 - get up to 6 digits worth of hex string.
   Needed to support long DSP constants (48 bits) 
*/

/* #include "_dsputilities.h" */

int _DSPGetIntHexStr6(s)		/* get integer from hex string */
    char **s;			/* input string is chopped to int++ */
{
    int i;
    char *p,*t;
/*  t = *s; */
/*  while (!isxdigit(*t)&&*t!='-'&&*t) t++; /* skip to beginning of hex int */

    /* skip to beginning of hex int */
    for (t = *s; !isxdigit(*t) && *t!='-' && *t; t++) 
      if (!isspace(*t) && *t != '$') {
	  fprintf(stderr,"_DSPGetIntHexStr6: *** Found spurious character ");
	  fprintf(stderr,"%c scanning for hex integer in string:\n\t%s\n",*t,*s);
      }
    if (!*t) {
	*s=t;			/* no token */
	return _DSP_NOT_AN_INT; /* no token */
    }
    p=t;
    if (*p == '-') p++;
    for(i=0; i<6 && isxdigit(*p); i++,p++); /* t->token, p->token++ */
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    sscanf(t,"%X",&i);
    return i;
}

char *_DSPFGetRecord (lnkfp)	/* get next record from .lnk or .lod file */
				/* a record is of the form "_%s\n%s\n..."_ */
    FILE *lnkfp;		/* linker input file pointer */
{
    char *tok,*c,*lineptr,*record=NULL;
    char linebuf[_DSP_MAX_LINE];	/* input line buffer */

    /* SKIP LINES FROM INPUT FILE UNTIL '_' APPEARS IN COLUMN 1 */

    while (TRUE) {
	if (fgets(linebuf,_DSP_MAX_LINE,lnkfp)==NULL) return NULL;
	if (linebuf[0]=='_') break; /* get to start of a record */
    }

    record = _DSPCopyStr(linebuf);	/* first line of record */

    /* CONCATENATE LINES FROM INPUT FILE UNTIL '_' APPEARS IN COLUMN 1 */
    
    while (TRUE) {
	char c;
	c=getc(lnkfp);		/* peek ahead 1 char */
	ungetc(c,lnkfp);
	if (c=='_') return record;
	if (fgets(linebuf,_DSP_MAX_LINE,lnkfp)==NULL) return record;
	/* if (c==';') continue;	/* strip comment */
	record = _DSPReCat(record,linebuf); /* next line of record */
    }
}

