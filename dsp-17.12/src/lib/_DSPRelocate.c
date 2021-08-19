/*	_DSPRelocate.c - relocate struct of type DSPLoadSpec.
	Copyright 1987,1988 NeXT, Inc.
	
	This function relocates the program memory in the relative
	sections of a DSPLoadSpec data structure (defined in dspstructs.h).
	It assumes that the loadAddress field of each section is set
	to the desired load address for the section (for each memory
	space). For example, _DSPRelocateUser() will do this after a 
	call to _DSPLnkRead().
	
	*** NOTE *** 
	ONLY program memory is fixed up.  To support fixups in data
	memories also, change DSP_LC_NUM_P to DSP_LC_NUM 
	and *+(int)DSP_LC_P to *. (Also, think about it.)
	Watch out for the fact that data records contain either
	constant _BLOCKDATA or non-constant _DATA info.
	
	Modification history:
	12/27/87/jos - File created
	05/08/88/daj - Split _DSPRelocate into 2 functions for use of musickit.
	05/26/89/jos - Added fclose(mmfp)
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

#define ERROR0 "_DSPRelocate: Found multiple data segments in single DSP \n\
memory space (which can happen only for absolute assemblies). \n\
Apparently an absolute section is posing as a relative section."
  
int _DSPReloc(DSPDataRecord *data, DSPFixup *fixups,
    int fixupCount, int *loadAddresses)
/* 
 * dataRec is assumed to be a P data space. Fixes it up in place. 
 * This is a private libdsp method used by _DSPSendUGTimed and
 * _DSPRelocate. 
 */
{
    int errorcode=0;		/* return codes. 0 means success */
    DSPFixup *fixup;
    int spc;
    if (_DSPTrace & DSP_TRACE__DSPRELOCATE) 
      printf("\n_DSPReloc\n");
    if (data->next != NULL)
      _DSPError(errorcode=1,ERROR0);
    for (fixup = fixups; fixupCount--; fixup++) {
	data->data[fixup->refOffset] = loadAddresses[fixup->locationCounter]
	    + fixup->relAddress;
	if (_DSPTrace & DSP_TRACE__DSPRELOCATE) {
	    spc = fixup->locationCounter;
	    printf("\t%s[%d] = loadAddress(%s)+%d  (%s:%s)\n",
		   DSPLCNames[data->locationCounter],
		   fixup->refOffset,DSPLCNames[spc],
		   fixup->relAddress,DSPLCNames[spc],fixup->name);
	}
    }
    return errorcode;
}

int _DSPRelocate(dsp)	
    DSPLoadSpec *dsp;		
    /* relocate DSP struct contents.
       DSPLoadSpec *dsp is produced by _DSPLnkRead() */
{
    int i,j,m;
    DSPSection *sec;
    if (_DSPTrace & DSP_TRACE__DSPRELOCATE) 
      printf("\n_DSPRelocate\n");
    for (i=0;i<DSP_N_SECTIONS;i++) {
	sec = dsp->indexToSection[i];
	if (_DSPTrace & DSP_TRACE__DSPRELOCATE)
	  printf("\tSection %s\n",DSPSectionNames[i]);
	if (sec) 
	  for (m=0;m<DSP_LC_NUM_P;m++) 
	    if (sec->data[m+(int)DSP_LC_P] != NULL) 
	      DSP_UNTIL_ERROR(_DSPReloc(sec->data[m+(int)DSP_LC_P],sec->fixups[m],
				    sec->fixupCount[m],sec->loadAddress));
    }
    if ( _DSPTrace & DSP_TRACE__DSPRELOCATE )
      DSPLoadSpecPrint(dsp);
    return 0;
}


/* ---------------------------------------------------------------------------
/*	_DSPMemMapRead.c - Read load addresses for USER memory from map file.

	_DSPMemMapRead() returns a struct containing the DSP memory-map
	constants found in the DSP system memory-map file 
	$DSP/smsrc/mkmon8k.mem.

	These constants must be the same as those assumed in the file 
	$DSP/smsrc/memmap.asm.	Also, the file $DSP/smsrc/config.asm
	must be configured to assemble for NeXT hardware.  If all this
	is true, then the loadAddress field of each of the nine memory segments
	if the USER section of a DSPLoadSpec struct can be set to the 
	loadAddress values returned by this routine (in the _DSPMemMap struct),
	and a call to _DSPRelocate() will load the USER section into the DSP 
	where it belongs. The routine _DSPRelocateUser() calls this function.
	
*/

static _DSPMemMap *_DSPMemMapRead(char *mmfn)
/* 
 * Read memory map file.
 * Arg is memorymap filename ($DSP/smsrc/mkmon8k.mem) 
 */
{
    FILE *mmfp;			/* memory map file pointer */
    _DSPMemMap *memmap;		/* memory map struct (dspstructs.h) */
    int i,im;
    char *tok,*c,*record,*rp;
    char linebuf[_DSP_MAX_LINE];	/* input line buffer */
    DSPLocationCounter m;
#   define ERROR1 "_DSPMemMapRead: Memory-map filename %s extension not '.mem'"
#   define ERROR2 "_DSPMemMapRead: Memory-map file %s not found."
#   define ERROR3 "_DSPMemMapRead: Error in memory-map file format:\n\t%s"
#   define ERROR4 "_DSPMemMapRead: Only section USER can be relocated:\n\t%s"
#   define ERROR5 "_DSPMemMapRead: Unrecognized record:\n\t%s"
#   define ERROR6 "_DSPMemMapRead: Memory-map file gives no offset for space %s"

    if (_DSPTrace & DSP_TRACE__DSPMEMMAPREAD) printf("\n_DSPMemMapRead:\n");

    DSP_MALLOC(memmap,_DSPMemMap,1);
    DSPMemMapInit(memmap);
    for (i=0;i<DSP_LC_NUM;i++)
      memmap->userOffsets[i] = DSP_UNKNOWN; /* to flag "not set" by map file */

    if (strcmp((tok=_DSPGetTail(mmfn)),".mem")!=0)
      _DSPError1(DSP_EBADFILETYPE,ERROR1,mmfn);
	
    if ((mmfp=_DSPMyFopen(mmfn,"r"))==NULL) { 
	_DSPError1(ENOENT,ERROR2,mmfn);
	return NULL; 
    }
    
    if (_DSPVerbose) printf("\tReading memory-map file:\t%s\n",mmfn);

    /* READ LINES FROM INPUT FILE */
    while ((fgets(linebuf,_DSP_MAX_LINE,mmfp))!=NULL) {
	rp = _DSPSkipWhite(linebuf);
	if (*rp==';') continue; /* comment line */
	tok = _DSPGetTokStr(&rp);	/* record name */
	if (!tok) continue;	/* strcmp() dies when tok==NULL */
	if (strcmp(tok,"START")==0) { 
	    /* START RECORD: sets load address per space for all sections.
	       Sections loads are concatenated in order of appearance.
	       In our case (only USER section can relocate), START is a noop
	       unless there is no SECTION record in which case the START
	       record serves the same purpose. If both START and SECTION
	       USER appear, the second one to appear overwrites userOffsets
	       specified by the first */
	    goto gulpmspecs;	/* either this or MACRO or annoying function */
	}
	else if (strcmp(tok,"SECTION")==0) { 
	    /* SECTION RECORD: sets load address per space for one section */
	    /* In our case only USER section can relocate */
	    tok=_DSPGetTokStr(&rp);
	    if (strcmp(tok,"USER")!=0) 
	      _DSPError1(DSP_EBADMEMMAP,ERROR4,linebuf);
	    /* Last SECTION spec, regardless of name, determines offsets */
	    goto gulpmspecs;
	}
	else {			/* unknown record type */
	    _DSPError1(DSP_EBADFILEFORMAT,ERROR5,linebuf);
	    continue;		/* ignore current line */
	}
	
      gulpmspecs:		/* Read userOffsets */
	while (*rp) {
	    if ((im=(int)(m=_DSPGetMemStr(&rp,"R")))==(int)DSP_LC_N) break;
	    if ((i=_DSPGetDSPIntStr(&rp))==_DSP_NOT_AN_INT)
	      _DSPError1(DSP_EBADFILEFORMAT,ERROR3,linebuf);
	    else memmap->userOffsets[im] = i; /* im=1-12 (never 0) */
	    /* '\n' not allowed in record as of 1/5/88 */
	    /* You CAN however use multiple SECTION records */
	}
    }

    fclose(mmfp);

    for (i=1;i<DSP_LC_NUM;i++) {
	if(memmap->userOffsets[i]==DSP_UNKNOWN) { /* never specified in .mem file */
	    _DSPError1(DSP_EBADFILEFORMAT,ERROR6,(char *)DSPLCNames[i]);
	    memmap->userOffsets[i]==0; /* good luck! */
	}
	memmap->defaultOffsets[i] = memmap->userOffsets[i]; /* why not? */
    }
	
    if ( _DSPTrace & DSP_TRACE__DSPMEMMAPREAD )
      DSPMemMapPrint(memmap);
    
    return(memmap);
}
/* ---------------------------------------------------------------------------
	_DSPRelocateUser.c - Install relocation offsets for USER DSP memory.

	_DSPRelocateUser() sets the loadAddress field of each USER DSP 
	memory segment (there are 12) to the system-defined load offsets
	defined in the DSP system memory-map file.  The startAddress 
	field is not set, since that is either specified by the END
	statement in the assembler output file or by the default in
	DSPLoadSpecInit.

	To obtain the DSPLoadSpec struct which is passed here as an argument,
	use the function DSPReadFile() to read the struct from a DSP assembler 
	relocatable object file.  _DSPRelocateUser() installs relocation
	offsets in each section of the DSPLoadSpec struct based on the 
	memory-map constants returned by _DSPMemMapRead(). The function 
	_DSPRelocate()must be called to actually perform fixups in the code. 
	Thus, the default loadAddress values installed by this function may
	be overridden by simply overwriting the loadAddress field of
	each section in the DSPLoadSpec struct before calling _DSPRelocate().

	_DSPRelocateUser() returns 0 if successful, nonzero otherwise.
*/

int _DSPRelocateUser(dsp)	/* set up dsp->userSection->loadAddress[i] */
    DSPLoadSpec *dsp;		/* read from .lnk file by _DSPLnkRead() */
{
    int errorcode=0;		/* return code. 0 means success */
    int i,j,m;
    _DSPMemMap *memmap;
    char *mapfile;

    mapfile = DSPGetSystemMapFile();

    if (_DSPTrace & DSP_TRACE__DSPRELOCATEUSER) 
	printf("\n_DSPRelocateUser\n");

    memmap = _DSPMemMapRead(mapfile);
    if (!memmap) 
      return 
	_DSPError1(1,"_DSPRelocateUser: error reading map file %s",mapfile);

    for (m=0;m<DSP_LC_NUM;m++)
      dsp->userSection->loadAddress[m] = memmap->userOffsets[m];

    if ( _DSPTrace & DSP_TRACE__DSPRELOCATEUSER ) {
	_DSPRelocate(dsp);	/* just to see how they came out */
	DSPLoadSpecPrint(dsp);
    }

    return errorcode;
}

