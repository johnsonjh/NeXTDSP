/*** UTILITY FUNCTIONS FOR DSP DATA STRUCTURES (MUNG/INIT/PRINT) ***/

/*
 * History
 * 09/26/88/jos - added check on write for disk full
 * 02/10/89/jos - incorporated DSPLoadSpecReadFile.c and DSPLoadSpecWriteFile.c
 * 02/13/89/jos - removed call to _DSPContiguousFree(sym) in DSPSymbolFree
 * 02/14/89/jos - removed excess calls to DSPDataRecordFree in DSPSectionFree
 * 02/17/89/jos - DSPDataRecordMerge: fixed bug preventing l: dr merge
 * 05/24/89/mtm - added fclose
 * 04/23/90/jos - flushed unsupported entry points.
 * 04/30/90/jos - Removed "r" prefix from rData, rWordCount, rRepeatCount
 */

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"
#include <sys/file.h>	/* DSPLoadSpecReadFile(), DSPLoadSpecWriteFile() */

extern int unlink();

static int magicNumber = 0x11111111;  /* Increase this when format changes */

/************************* INITIALIZATION FUNCTIONS **************************/

int _DSPCheckingFWrite(
    int *ptr,
    int size,
    int nitems,
    FILE *stream)
{
    int nw;
    nw = fwrite(ptr, size, nitems, stream);
    if (nw == nitems)
      return(0);
    else
      return(_DSPError(-1,"!!! File system is FULL !!!"));
}


void DSPDataRecordInit(DSPDataRecord *dr)
{
    dr->section = NULL;
    dr->locationCounter = DSP_LC_N;
    dr->loadAddress = 0;
    dr->repeatCount = 0;
    dr->wordCount = 0;
    dr->data = NULL;
    dr->next = NULL;
    dr->prev = NULL;
}


void DSPSectionInit(DSPSection *sec) 
{
    int i;
    sec->name = NULL;
    sec->type = NULL;
    sec->number = DSP_UNKNOWN;
    for (i=0;i<DSP_LC_NUM;i++) {
	sec->loadAddress[i] = 0;
	sec->data[i] = NULL;
	sec->dataEnd[i] = NULL;
	sec->symCount[i] = 0;
	sec->symAlloc[i] = 2048;
	DSP_MALLOC(sec->symbols[i],DSPSymbol,sec->symAlloc[i]);
    }
    for (i=0;i<DSP_LC_NUM_P;i++) {
	sec->fixupCount[i] = 0;
	sec->fixupAlloc[i] = 128;
	DSP_MALLOC(sec->fixups[i],DSPFixup,sec->fixupAlloc[i]);
    }
    sec->xrefCount = 0;
    sec->xrefAlloc = 20;
    DSP_MALLOC(sec->xrefs,char*,sec->xrefAlloc);
}


void DSPLoadSpecInit(DSPLoadSpec *dsp)
{
    int i;
    dsp->module = NULL;
    dsp->type = NULL;
    dsp->version = 0;
    dsp->revision = 0;
    dsp->errorCount = 0;
    dsp->startAddress = 64;
    dsp->comments = NULL;
    dsp->description = NULL;
    dsp->globalSection = NULL;
    dsp->systemSection = NULL;
    dsp->userSection = NULL;
    for (i=0;i<(int)DSP_NSectionTypes;i++) 
      dsp->indexToSection[i] = NULL;
}

void DSPMemMapInit(_DSPMemMap *mm)
/* initialize record fields to NULL */
{
    int i;
    for (i=0;i<DSP_LC_NUM;i++) {
	mm->defaultOffsets[i] = 0;
	mm->userOffsets[i] = 0;
	mm->nOtherOffsets[i] = 0;
	mm->otherOffsets[i] = NULL;
    }
}

/**************************** PRINTING FUNCTIONS *****************************/

void DSPSymbolPrint(DSPSymbol sym)
{
    char *spc,*symptype;
    char symval[100];

    if (sym.type[2]=='F')
      sprintf(symval,"%f",sym.value.f);
    else
      sprintf(symval,"$%X",sym.value.i);
    
    spc = (char *)DSPLCNames[(int)sym.locationCounter];
    printf("\t\t%s:%-22s\t(%s)\t= %s\n",
	   spc,sym.name, sym.type, symval);
}


void DSPDataRecordPrint(DSPDataRecord *dr)
{
    int k,wcm4;
    printf(
 "\n\t%s DATA RECORD: %d*%d = $%X * $%X words at %s:%d:%d = $%X..$%X\n",
	   DSPLCNames[(int)dr->locationCounter], 
	   dr->wordCount,
	   dr->repeatCount, 
	   dr->wordCount,
	   dr->repeatCount, 
	   DSPLCNames[(int)dr->locationCounter], 
	   dr->loadAddress,
	   dr->loadAddress + dr->wordCount * dr->repeatCount - 1,
	   dr->loadAddress,
	   dr->loadAddress + dr->wordCount * dr->repeatCount - 1 );
    wcm4 = dr->wordCount % 4;
    for (k=0;k<dr->wordCount-wcm4;k+=4)
      printf("\t\t$%-9X \t $%-9X \t $%-9X \t $%-9X\n",
	     dr->data[k],
	     dr->data[k+1],
	     dr->data[k+2],
	     dr->data[k+3]);
    printf("\t\t");
    for (k=dr->wordCount-wcm4;k<dr->wordCount;k++)
      printf("$%-9X \t ",dr->data[k]);
    printf("\n");
}


void DSPSectionPrint(DSPSection *section)
{
    int i,j,k,symcount,wcm4;
    printf("\nSection %s\n",section->name);
    printf("	Type   = %s\n",*section->type=='R'?"Relative":"Absolute");
    printf("	Number = %d\n",section->number);
    printf("\n");
    for (j=0;j<DSP_LC_NUM;j++) {
	DSPDataRecord *dr;
	symcount = section->symCount[j];
	printf("    %-2s symbol count = %d",
	       DSPLCNames[j],symcount);
	if (j!=0)
	  printf(" ... load address = $%X\n",
		 section->loadAddress[j]);
	else
	  printf("\n");
	for (k=0;k<symcount;k++) 
	  DSPSymbolPrint(section->symbols[j][k]);
	for (dr=section->data[j];dr!=NULL;dr=dr->next) 
	  DSPDataRecordPrint(dr);
    }
    for (i=0;i<DSP_LC_NUM_P;i++) {
	if (section->fixupCount[i]>0)
	  printf("\nRelocatable symbols encountered for space %s:\n",
		 DSPLCNames[i+(int)DSP_LC_P]);
	for (j=0;j<section->fixupCount[i];j++)
	  printf("\t%s[%d] = loadAddress(%s)+%d	 (%s/%s:%s)\n",
		 DSPLCNames[i+(int)DSP_LC_P],
		 section->fixups[i][j].refOffset,
		 DSPLCNames[(int)section->fixups[i][j].locationCounter],
		 section->fixups[i][j].relAddress,
		 section->name,
		 DSPLCNames[(int)section->fixups[i][j].locationCounter],
		 section->fixups[i][j].name);
	if (0)
	  printf("\tAt offset %d, place &%s = %d\n",
		 section->fixups[i][j].refOffset,
		 section->fixups[i][j].name,
		 section->fixups[i][j].relAddress);
    }
    if (section->xrefCount>0)
      printf("\nExternal symbol references encountered in section %s:\n",
	     section->name);
    for (j=0;j<section->xrefCount;j++)
      printf("\t%s\n",section->xrefs[j]);
}


void DSPLoadSpecPrint(DSPLoadSpec *dsp)
{
    int i;
    DSPSection *section;
    printf(
"\n======================= DSPLoadSpec struct printout ====================\n\n");
    printf("Module %s:\n",dsp->module);
    printf("Type %s:\n",(*dsp->type=='A'?"A[bsolute]":"R[elative]"));
    printf("\tVersion,Rev   = %d,%d\n",dsp->version,dsp->revision);
    printf("\tError count   = %d\n",dsp->errorCount);
    printf("\tStart Address = $%X\n",dsp->startAddress);
    printf("\n");
    if (dsp->description)
      printf("Description:\n%s\n",dsp->description);
    if (dsp->comments)
      printf("Comments:\n%s\n",dsp->comments);
    for (i=0;i<DSP_N_SECTIONS;i++) {
	section = dsp->indexToSection[i];
	if (section==NULL)
	  printf("Section %s not present.\n",DSPSectionNames[i]);
	else
	  DSPSectionPrint(section);
    }
}


void DSPMemMapPrint(_DSPMemMap *mm)
{
    int i;
    printf(
"\n======================= _DSPMemMap struct printout ====================\n\n");
    for (i=0;i<DSP_LC_NUM;i++) {
	printf("\tdefaultOffsets[%-2s] = 0x%-6X",
	       DSPLCNames[i],mm->defaultOffsets[i]);
	printf("\tuserOffsets[%-2s] = 0x%-6X\n",
	       DSPLCNames[i],mm->userOffsets[i]);
	if (mm->nOtherOffsets[i]>0) {
	    printf("\tnOtherOffsets[%d] = %d\n",i,mm->nOtherOffsets[i]);
	    printf("\t\tCannot print (unsupported) otherOffsets\n");
	}
    }
}

/****************************** ARCHIVING FUNCTIONS **************************/

/*** writeDSPx ***/

/* TODO: Add proper error return codes (for disk being full, etc.) */

int _DSPWriteString(char *str, FILE *fp)
{
    int len;
    if (!str) str = "(none)";	/* have to write out something (for reader) */
    len = strlen(str)+1;		/* include terminating NULL */
    DSP_UNTIL_ERROR(_DSPCheckingFWrite(&len,sizeof(int),1,fp));
    DSP_UNTIL_ERROR(_DSPCheckingFWrite((int *)str,len,1,fp));
    return(0);
}


int DSPDataRecordWrite(DSPDataRecord *dr, FILE *fp)
{
    DSP_UNTIL_ERROR(_DSPCheckingFWrite((int *)dr,sizeof(DSPDataRecord),1,fp));
    if (dr->wordCount)
      DSP_UNTIL_ERROR(_DSPCheckingFWrite(dr->data,sizeof(int),dr->wordCount,fp));
    if (dr->next)
      return(DSPDataRecordWrite(dr->next,fp));
    return(0);
}


int DSPSymbolWrite(DSPSymbol sym, FILE *fp)
{
    DSP_UNTIL_ERROR(_DSPCheckingFWrite((int *)&sym,sizeof(DSPSymbol),1,fp));
    DSP_UNTIL_ERROR(_DSPWriteString(sym.name,fp));
    return(_DSPWriteString(sym.type,fp));
}


int DSPFixupWrite(DSPFixup fxp, FILE *fp)
{
    DSP_UNTIL_ERROR(_DSPCheckingFWrite((int *)&fxp,sizeof(DSPFixup),1,fp));
    return(_DSPWriteString(fxp.name,fp));
}


int DSPSectionWrite(DSPSection *sec, FILE *fp)
{
    int i,j,k,symcount;
    int symalloc[DSP_LC_NUM],fixupalloc[DSP_LC_NUM_P],xrefalloc;

    for (i=0;i<DSP_LC_NUM;i++)
      symalloc[i] = sec->symAlloc[i];
    for (i=0;i<DSP_LC_NUM_P;i++)
      fixupalloc[i] = sec->fixupAlloc[i];
    xrefalloc = sec->xrefAlloc;

    /* squeeze out any extra space allocation */
    for (i=0;i<DSP_LC_NUM;i++)
      sec->symAlloc[i] = sec->symCount[i];
    for (i=0;i<DSP_LC_NUM_P;i++)
      sec->fixupAlloc[i] = sec->fixupCount[i];
    sec->xrefAlloc = sec->xrefCount;

    /* write out DSPSection struct */
    if(!sec) _DSPError(EINVAL,"DSPSectionWrite: can't pass null pointer");
    DSP_UNTIL_ERROR(_DSPCheckingFWrite((int *)sec,sizeof(DSPSection),1,fp));

    /* restore extra space allocation */
    for (i=0;i<DSP_LC_NUM;i++)
      sec->symAlloc[i] = symalloc[i];
    for (i=0;i<DSP_LC_NUM_P;i++)
      sec->fixupAlloc[i] = fixupalloc[i];
    sec->xrefAlloc = xrefalloc;

    /* write out pointed-to items */
    DSP_UNTIL_ERROR(_DSPWriteString(sec->name,fp));
    DSP_UNTIL_ERROR(_DSPWriteString(sec->type,fp));

    for (i=0;i<DSP_LC_NUM;i++) {
	symcount = sec->symCount[i];
	for (j=0;j<symcount;j++)
	  DSP_UNTIL_ERROR(DSPSymbolWrite(sec->symbols[i][j],fp));
	if (sec->data[i])
	  DSP_UNTIL_ERROR(DSPDataRecordWrite(sec->data[i],fp));
    }

    for (i=0;i<DSP_LC_NUM_P;i++) {
	for (j=0;j<sec->fixupCount[i];j++)
	  DSP_UNTIL_ERROR(DSPFixupWrite(sec->fixups[i][j],fp));
    }

    if (sec->xrefCount>0)
      _DSPError1(DSP_EBADSECTION,
	"External symbol references encountered in section %s",sec->name);

    for (i=0;i<sec->xrefCount;i++)
      DSP_UNTIL_ERROR(_DSPWriteString(sec->xrefs[i],fp));

    return(0);
}


int DSPLoadSpecWrite(DSPLoadSpec *dsp, FILE *fp)
{
    int mag;
    if(!dsp) return(0);
    if(!fp) return(1);
    mag = magicNumber;
    fwrite(&mag,sizeof(int),1,fp);
    fwrite(dsp,sizeof(DSPLoadSpec),1,fp);
    _DSPWriteString(dsp->module,fp);
    _DSPWriteString(dsp->type,fp);
    _DSPWriteString(dsp->comments,fp);
    _DSPWriteString(dsp->description,fp);
    if (dsp->globalSection)
      DSPSectionWrite(dsp->globalSection,fp);
    if (dsp->systemSection)
      DSPSectionWrite(dsp->systemSection,fp);
    if (dsp->userSection)
      DSPSectionWrite(dsp->userSection,fp);
    return(0);
}


int DSPLoadSpecWriteFile(
    DSPLoadSpec *dspptr,		/* struct containing  DSP load image */
    char *dspfn)			/* file name */
{
    FILE *fp;			/* relocatable link file pointer */
    int ec;
    int dspsize;

    if (_DSPTrace & DSP_TRACE_DSPLOADSPECWRITE) 
      printf("\nDSPLoadSpecWriteFile\n");

    unlink(dspfn);
    fp = _DSPMyFopen(dspfn,"w");
    if (fp==NULL) 
      return(_DSPError1(-1,"DSPLoadSpecWriteFile: could not open %s ",dspfn));

    if (_DSPVerbose)
      printf("\tWriting DSP fast-load file:\t%s\n",dspfn);

    ec = DSPLoadSpecWrite(dspptr,fp);

    fclose(fp);

    if (_DSPVerbose)
      printf("\tFile %s closed.\n",dspfn);

    if (ec) 
      return(_DSPError1(ec,"DSPLoadSpecWriteFile: write failed on %s ",dspfn));

    return(0);
}


/*** readDSPx ***/

char *_DSPContiguousMalloc(unsigned size)
{
    /* FIXME: Cannot implement this until a new bit is placed in each
       struct telling whether it was allocated using malloc (as in 
       _DSPLnkRead() [and anywhere else??] or this routine.
       Each corresponding free routine must test this bit.

       An alternative is to use DSP_CONTIGUOUS_MALLOC in _DSPLnkRead.
       That way the struct is contiguous whether read in from a .lnk/.lod
       file or from a .dsp file.  All frees go away except for DSPLoadSpecFree.
       It frees the single big block allocated for the struct.
    */

    char *ptr;

    ptr = malloc(size);

    if (_DSPTrace & DSP_TRACE_MALLOC)
      fprintf(stderr,
	   "_DSPContiguousMalloc:\t Allocating %d bytes at 0x%X\n",size,ptr);

    return (ptr);
}

#define DSP_CONTIGUOUS_MALLOC( VAR, TYPE, NUM ) \
  if(((VAR) = (TYPE *) _DSPContiguousMalloc((unsigned)(NUM)*sizeof(TYPE) )) == NULL) \
  _DSPError(-1,"malloc: insufficient memory");


int _DSPContiguousFree(char *ptr)
{
    /* FIXME: See _DSPContiguousMalloc() and keep in synch. */

    int nb;

    if (_DSPTrace & DSP_TRACE_MALLOC) {
	nb = malloc_size(ptr);
	fprintf(stderr,
		"_DSPContiguousFree:\t	  Freeing %d bytes at 0x%X\n",nb,ptr);
    }
    free (ptr);

    return (0);
}

int _DSPReadString(char **spp, FILE *fp)
{
    int len,lenr;
    fread(&len,sizeof(int),1,fp);
    /* DSP_CONTIGUOUS_MALLOC(*spp,char,len); */
    *spp = _DSPContiguousMalloc(len*sizeof(char));
    if(*spp == NULL)
      _DSPError(-1,"_DSPReadString: insufficient memory");
    lenr=fread(*spp,sizeof(char),len,fp);
    if (lenr!=len) 
      _DSPError1(-1,"_DSPReadString: fread returned %s",_DSPCVS(lenr));
    return(lenr==len?0:1);
}


int DSPDataRecordRead(
    DSPDataRecord **drpp,
    FILE *fp,
    DSPSection *sp)	/* pointer to section owning this data record */
{
    int nwords;

    if(!*drpp) 
      _DSPError(EINVAL,"DSPDataRecordRead: can't pass null pointer");

    DSP_CONTIGUOUS_MALLOC(*drpp,DSPDataRecord,1);
    fread(*drpp,sizeof(DSPDataRecord),1,fp);
    (*drpp)->section = sp;	/* owning section */

    if (nwords=(*drpp)->wordCount) {
	DSP_CONTIGUOUS_MALLOC((*drpp)->data,int,nwords);
	fread((*drpp)->data,sizeof(int),nwords,fp);
    }
	
    if ((*drpp)->next) {
	DSPDataRecordRead(&((*drpp)->next),fp,sp);
	(*drpp)->next->prev = *drpp;
    }
    return(0);
}


int DSPSymbolRead(DSPSymbol *symp, FILE *fp)
{
    /*	   DSP_CONTIGUOUS_MALLOC(symp,DSPSymbol,1); */	 /* Allocated by caller */
    fread(symp,sizeof(DSPSymbol),1,fp);

    _DSPReadString(&((symp)->name),fp);
    _DSPReadString(&((symp)->type),fp);
    
    return(0);
}


int DSPFixupRead(DSPFixup *fxpp, FILE *fp)
{
    /*	   DSP_CONTIGUOUS_MALLOC(fxpp,DSPFixup,1); */
    fread(fxpp,sizeof(DSPFixup),1,fp);
    _DSPReadString(&((fxpp)->name),fp);
    return(0);
}


int DSPSectionRead(DSPSection **secpp, FILE *fp)
{
    int i,j,k,symcount,symalloc,fixupalloc,fixupcount;

    if(!*secpp) return(0);

    DSP_CONTIGUOUS_MALLOC(*secpp,DSPSection,1);
    fread(*secpp,sizeof(DSPSection),1,fp);
    
    _DSPReadString(&((*secpp)->name),fp);
    _DSPReadString(&((*secpp)->type),fp);

    for (i=0;i<DSP_LC_NUM;i++) {
	symcount = (*secpp)->symCount[i];
	symalloc = (*secpp)->symAlloc[i];
	if (symalloc)
	  DSP_CONTIGUOUS_MALLOC((*secpp)->symbols[i],DSPSymbol,symalloc);
	if (symalloc<symcount)
	  return(_DSPError(DSP_EBADDSPFILE,
	       "DSPSectionRead: symAlloc<symCount"));
	for (j=0;j<symcount;j++)
	  DSPSymbolRead(&((*secpp)->symbols[i][j]),fp);
	if ( (*secpp)->data[i] ) {
	    DSPDataRecord *dr;
	    DSPDataRecordRead(&((*secpp)->data[i]),fp,*secpp);
	    dr = (*secpp)->data[i];
	    while (dr->next)	/* search for last data block in chain */
	      dr = dr->next;
	    (*secpp)->dataEnd[i] = dr; /* install ptr to it */
	}	    
    }

    for (i=0;i<DSP_LC_NUM_P;i++) {
	fixupalloc = (*secpp)->fixupAlloc[i];
	fixupcount = (*secpp)->fixupCount[i];
	if (fixupalloc)
	  DSP_CONTIGUOUS_MALLOC((*secpp)->fixups[i],DSPFixup,fixupalloc);
	if (fixupalloc<fixupcount)
	  return(_DSPError(DSP_EBADDSPFILE,
	       "DSPSectionRead: fixupAlloc<fixupCount"));
	for (j=0;j<fixupcount;j++)
	  DSPFixupRead(&((*secpp)->fixups[i][j]),fp);
    }

    if ((*secpp)->xrefCount>0)
      _DSPError1(DSP_EBADSECTION,
	"Unsupported external symbol references encountered in section %s",
	     (*secpp)->name);

    if ((*secpp)->xrefAlloc)
      DSP_CONTIGUOUS_MALLOC((*secpp)->xrefs,char*,(*secpp)->xrefAlloc);
    for (i=0;i<(*secpp)->xrefCount;i++)
      _DSPReadString(&((*secpp)->xrefs[i]),fp);

    return(0);
}


int DSPLoadSpecRead(DSPLoadSpec **dpp, FILE *fp)
{
    int mag;
    if(!fp) return(1);
    fread(&mag,sizeof(int),1,fp);
    if (mag<magicNumber)
      return(_DSPError(DSP_EBADDSPFILE,
	 "Version mismatch: Binary DSP file is older than this compilation."));
    if (mag>magicNumber)
      return(_DSPError(DSP_EBADDSPFILE,
	 "Version mismatch: Binary DSP file is newer than this compilation."));

    DSP_CONTIGUOUS_MALLOC(*dpp,DSPLoadSpec,1);
    fread(*dpp,sizeof(DSPLoadSpec),1,fp);

    _DSPReadString(&((*dpp)->module),fp);
    _DSPReadString(&((*dpp)->type),fp);
    _DSPReadString(&((*dpp)->comments),fp);
    _DSPReadString(&((*dpp)->description),fp);
    if ((*dpp)->globalSection)
      DSPSectionRead(&((*dpp)->globalSection),fp);
    if ((*dpp)->systemSection)
      DSPSectionRead(&((*dpp)->systemSection),fp);
    if ((*dpp)->userSection)
      DSPSectionRead(&((*dpp)->userSection),fp);
    (*dpp)->indexToSection[0] = (*dpp)->globalSection;
    (*dpp)->indexToSection[1] = (*dpp)->systemSection;
    (*dpp)->indexToSection[2] = (*dpp)->userSection;
    return(0);
}


int DSPLoadSpecReadFile(
    DSPLoadSpec **dspptr,		/* struct containing DSP load image */
    char *dspfn)			/* DSPLoadSpecWriteFile output file */
{
    FILE *fp;			/* relocatable link file pointer */
    int ec;
    int dspsize;

    if (_DSPTrace & DSP_TRACE_DSPLOADSPECREAD) 
      printf("\nDSPLoadSpecReadFile\n");

    fp = fopen(dspfn,"r");
    if (fp==NULL) 
      return(_DSPError1(-1,"DSPLoadSpecReadFile: could not open %s ",dspfn));

    if (_DSPVerbose)
      printf("\tReading DSP fast-load file:\t%s\n",dspfn);

    ec = DSPLoadSpecRead(dspptr,fp);

    fclose(fp);	   /* mtm added fclose 5/24/89 */

    if (_DSPVerbose)
      printf("\tFile %s closed.\n",dspfn);

    if (_DSPTrace & DSP_TRACE_DSPLOADSPECREAD) 
      DSPLoadSpecPrint(*dspptr);

    if (ec) 
      return(_DSPError1(ec,"DSPLoadSpecReadFile: read failed on %s ",dspfn));

    return(0);
}

/****************************** DEALLOCATION *********************************/

int _DSPFreeString(char *str)
{
    if (!str) return(0);
    _DSPContiguousFree(str);
    return(0);
}


int DSPDataRecordFree(DSPDataRecord *dr)
{
    if (!dr) return(0);
    if (dr->wordCount)
      _DSPContiguousFree((char *)(dr->data));
    if (dr->next)
      DSPDataRecordFree(dr->next);
    _DSPContiguousFree((char *)dr);
    return(0);
}


int DSPSymbolFree(DSPSymbol *sym)
{
    if (!sym) return(0);
    _DSPFreeString(sym->name);
    _DSPFreeString(sym->type);
    /* _DSPContiguousFree(sym); (Done by caller since mallocs were fused) */
    return(0);
}


int DSPFixupFree(DSPFixup *fxp)
{
    if (!fxp) return(0);
    _DSPFreeString(fxp->name);
    _DSPContiguousFree((char *)fxp);
    return(0);
}


int DSPSectionFree(DSPSection *sec)
{
    int i,j,k;

    if(!sec) return(0);

    _DSPFreeString(sec->name);
    _DSPFreeString(sec->type);

    for (i=0;i<DSP_LC_NUM;i++) {

	for (j=0;j<sec->symCount[i];j++) /* free individual symbols */
	  DSPSymbolFree(&(sec->symbols[i][j]));

	if (sec->symbols[i])	/* free array of symbol pointers */
	  _DSPContiguousFree((char *)(sec->symbols[i]));

	if (sec->data[i])	/* free array of symbol pointers */
	  DSPDataRecordFree(sec->data[i]);	/* recursive free */
    }

    for (i=0;i<DSP_LC_NUM_P;i++) {
	for (j=0;j<sec->fixupCount[i];j++)
	  DSPFixupFree(&sec->fixups[i][j]);
	if (sec->fixups[i])
	  _DSPContiguousFree((char *)(sec->fixups[i]));
    }

    if (sec->xrefCount>0)
      _DSPError1(DSP_EBADSECTION,
	 "External symbol references encountered in section %s",sec->name);

    for (i=0;i<sec->xrefCount;i++)
      _DSPFreeString(sec->xrefs[i]);

    if (sec->xrefs)
	_DSPContiguousFree((char *)(sec->xrefs));

    _DSPContiguousFree((char *)sec);

    return(0);
}


int DSPLoadSpecFree(DSPLoadSpec *dsp)
{
    if(!dsp) 
      return(0);

    /*** FIXME ***/
    return(0);

    _DSPFreeString(dsp->module);
    _DSPFreeString(dsp->type);
    _DSPFreeString(dsp->comments);
    _DSPFreeString(dsp->description);
    DSPSectionFree(dsp->globalSection);
    DSPSectionFree(dsp->systemSection);
    DSPSectionFree(dsp->userSection);
    _DSPContiguousFree((char *)dsp);
    return(0);
}

/******************************* MISCELLANEOUS *******************************/

DSPSection *DSPGetUserSection(DSPLoadSpec *dspStruct)
{
    DSPSection *user;

    /* 
      If the user was assembled in absolute mode,
      or if this is a .lnk file with no
      sections, then code lives in GLOBAL section,
      and all other sections are empty.
     */
	
    if (*dspStruct->type == 'A')
      user = dspStruct->globalSection;
    else
      user = dspStruct->userSection;
    return(user);
}

DSPAddress DSPGetFirstAddress(DSPLoadSpec *dspStruct,
			      DSPLocationCounter locationCounter)
{
    DSPAddress firstAddress;
    DSPSection *user;
    DSPDataRecord *dr;

    user = DSPGetUserSection(dspStruct);
    dr = user->data[(int)locationCounter];
    firstAddress = user->loadAddress[(int)locationCounter] + dr->loadAddress;
    return(firstAddress);
}


DSPAddress DSPGetLastAddress(DSPLoadSpec *dspStruct, 
			     DSPLocationCounter locationCounter)
{
    DSPAddress firstAddress;
    DSPAddress lastAddress;
    DSPAddress lastAddressBelow;
    DSPSection *user;
    DSPDataRecord *dr;

    user = DSPGetUserSection(dspStruct);
    
    dr = user->data[(int)locationCounter];
    firstAddress = DSPGetFirstAddress(dspStruct,locationCounter);
    lastAddress = firstAddress + dr->wordCount - 1;
    if (dr->next)
      _DSPError(DSP_EBADLNKFILE,
     "DSPGetLastAddress: Not intended for multi-segment (absolute) assemblies");
    return(lastAddress);
}


int DSPDataRecordInsert(DSPDataRecord *dr,
			DSPDataRecord **head,
			DSPDataRecord **tail) 
{
    dr->next = NULL;
    dr->prev = NULL;
    if (!*head) {
	*head = dr; /* first data block */
	*tail = dr; /* last data block */
    } else {
	DSPDataRecord *sb;
	sb = *tail;
	/* Sort by start address of data block */
	do {
	    if (sb->loadAddress <= dr->loadAddress) {
		/* insert dr after sb */
		if (sb->next)
		  (sb->next)->prev = dr;
		else /* nil next means we have a new tail */
		  *tail = dr;
		dr->next = sb->next;
		dr->prev = sb;
		sb->next = dr;
		return(0);
	    }
	} while (sb=sb->prev);
	/* block failed to place. Therefore it's first */
	(*head)->prev = dr;
	dr->next = *head;
	*head = dr;
    }
    return(0);
}


int DSPDataRecordMerge(DSPDataRecord *dr)
/* 
 * Merge contiguous, sorted dataRecords within a DSP memory space.
 * Argument is a pointer to the first data record in a linked list.
 */
{
    DSPDataRecord *dr1,*dr2;
    int la1,la2;		/* load addresses */
    int nw1,nw2;		/* word counts */
    int rp1,rp2;		/* repeat factors */
    int *dt1,*dt2;		/* data pointers */
    int i,nwr;
    
    if (!dr) return(0);
    dr1 = dr;			/* first data block */

    while (dr2=dr1->next) {

	la1 = dr1->loadAddress;
	la2 = dr2->loadAddress;
	nw1 = dr1->wordCount;
	nw2 = dr2->wordCount;
	rp1 = dr1->repeatCount;
	rp2 = dr2->repeatCount;
	dt1 = dr1->data;
	dt2 = dr2->data;

	/* Compute number of words really spanned by data record */
	nwr = (dr1->locationCounter >= DSP_LC_L && 
	       dr1->locationCounter <= DSP_LC_LH) ? /* if L memory */
		 nw1 >> 1 :	/* address range = wordcount / 2 */
		   nw1;		/* else it's wordcount */

	/* Skip merge if data not contiguous */
	if(la1 + nwr*rp1 != la2)
	    goto skipMerge;

	/* Merge adjacent memory fills if fill-constants are the same */
	if (rp1>1 && rp2>1	/* both are fill constants */
	    && ((nw1==1 && nw2==1) /* for 24-bit (x,y, or p) memory */
	       || (nw1==2 && nw2==2)) /* or 48-bit (l) memory */
	    && dt1[0] == dt2[0] /* same constant */
	    && (nw1 == 1 || dt1[1] == dt2[1]) )
	  {
	      dr1->repeatCount += dr2->repeatCount;
	      goto finishMerge;
	  }	    

	/* The above case is the only one in which we merge data blocks
	   involving a repeatCount > 1.	 We don't expand constant 
	   fills into general data arrays. */

	if (rp1>1 || rp2>1) goto skipMerge;	/* no way to combine */

	/* Here, both repeat factors are 1 (rp1==1, rp2==1) */

	NX_REALLOC(dt1,int,nw1+nw2);

	dr1->data = dt1;

	for (i=0;i<nw2;i++)
	  dt1[nw1+i] = dt2[i]; /* copy data array */

	dr1->wordCount += nw2;

      finishMerge:
	dr1->next = dr2->next;
	if (dr1->next)
	  dr1->next->prev = dr1;
	dr2->next = NULL;	/* prevent recursive deallocation beyond dr2 */
	DSPDataRecordFree(dr2);
	goto continueMerge;
      skipMerge:
	dr1 = dr1->next;
      continueMerge:
	;
    }
    return(0);
}
