/* dsploadwrap.c - generate code to load a DSP linker file into the DSP

DESCRIPTION
     See DSPDOC/dsploadwrap

Modification history
	01/28/87/jos - file created
	07/22/88/jos - getlastname returned nothing if last name was whole name
	12/20/88/jos - added switch -mk to set music-kit mode variable do_mk.
	01/07/89/jos - master UG stub file no longer overwritten if it exists.
	05/03/89/mtm - added -ap to USAGE string. Enabled C function generation
		       Pass header file pointer to DSPWriteC.
	05/12/89/mtm - removed do_mk and -mk.
	06/06/89/mtm - added -terse to USAGE string
	06/12/89/mtm - removed -symbols and -localsymobls from USAGE string.
		       (wait until implemented)
	06/15/89/mtm - changed ap interface function file prefix to DSPAP.
	08/17/89/mtm - Implement address arg 'a'.
	08/31/89/mtm - Fix usage string.
	03/15/90/jos - Added -prefixAP support
	03/15/90/jos - Added ugargstr support for "i" & "o"; "a" now "{x,y}a"
	03/15/90/jos - Added argc-- for -argtemplate option!
	03/16/90/jos - added dspFileBody,prefixAP args & proto. to DSPWriteC()
	08/26/90/jos - added "-stackable" option.

*/

#define USAGE "dsploadwrap [-{relative,absolute}] [-{ug,ap}] [-system] [-bootstrap]  [-simulator] [-argtemplate <string>] [-start <address>] [-trace <n>] [-prefixAP <string> [-stackable]] <filename(s)>"

#include <strings.h>
#include <sys/file.h>
#include <appkit/nextstd.h>
#include <dsp/dsp.h>

extern int access();            /* ? Not in <sys/file.h> as man says */
extern int writeUG();		/* from writeUG.c */
extern int DSPWriteC(
    DSPLoadSpec *dsp,
    char *cfn,			/* name of file open on cfp */
    FILE *cfp,			/* C file - assumed open for writing */
    FILE *hfp,			/* header file - assumed open for writing */
    int do_sys,			/* write out system segments */
    int do_boot,		/* boot DSP when executed */
    char *dspFileBody,		/* "foo" part of "foo.dsp" file name */
    char *prefixAP,		/* function name prefix (default = "DSPAP") */
    int stackable);		/* TRUE for "Load", FALSE for "LoadGo" */

/* private functions from libdsp */
extern char *_DSPCopyStr();
extern FILE *_DSPMyFopen();
extern int _DSPError1();
extern char *_DSPToLowerStr();
extern int _DSPErr();
extern int _DSPGetIntStr();
extern char *_DSPGetBody();
extern char *_DSPGetTail();
extern int _DSPLnkRead();
extern int _DSPRelocateUser();
extern int _DSPRelocate();
extern char *_DSPGetTokStr();
extern char *_DSPToUpperStr();

/* private variables from libdsp */
extern int _DSPTrace;

#define DSP_TRACE_DSPLOADWRAP 16

char *getlastname(c)	/* return identifier after last '_'.  NO COPY */
    char *c;
{    
    char *p;
    p = c+strlen(c)-1; /* point to last char */
    while ( *p != '_' && p != c ) p--;
    if (*p == '_') p++; 
    return(p);
}

char *rmvlastname(c)	/* remove string after and including last '_' */
    char *c;
{    
    char *s;
    char *p;
    s = _DSPCopyStr(c);
    p = s+strlen(s)-1;		/* point to last char */
    while ( *p!='_' && p!=s ) p--;
    if (*p=='_') *p='\0';
    return(s);
}

static char *makeUGFileName(fileRoot,type)
    char *fileRoot;
    int type; /* 1 == leaf, 2 == master include, 3 == master stub */ 
    /* Makes leaf file class name */
{
#   include <ctype.h>
    int i,len;
    char *rtn,*s1,*s;
    len = strlen(fileRoot);
    if (type != 2)
      NX_MALLOC(rtn,char,len+1+2-1+2); /* + \0 - _ + UG + .m */
    else 
      NX_MALLOC(rtn,char,len+1+9-1+2); /* + \0 - _ + UGInclude + .m */
    for (s = rtn, s1 = fileRoot; *s1 != '_' && *s1;)
      *s++ = *s1++;
    if (!*s1)
      fprintf(stderr,"No underbar found in file name.\n");
    s1++; /* Go past underbar */
    if (type == 1) {
	if (islower(rtn[0]))
	  rtn[0] = toupper(rtn[0]);
	sprintf(s,"UG");
	s += 2;
	while (*s1)   /* Get memory space tail */
	  *s++ = *s1++;
    }
    else if (type == 2) {
	sprintf(s,"UGInclude");
	s += 9;
    }
    else if (type == 3) {
	if (islower(rtn[0]))
	  rtn[0] = toupper(rtn[0]);
	sprintf(s,"UG");
	s += 2;
    }
    sprintf(s,".m");
    s += 2;
    *s = '\0';
    return rtn;
}

static char *ERROR1 =  "dsploadwrap: can't write C function file %s";
static char *ERROR2 =  "dsploadwrap: can't write C header file %s";
static char *ERROR4 =  "dsploadwrap: can't write symbol table file %s";
static char *ERROR8 =  "dsploadwrap: can't write DSP simulator host-interface file %s";

static void writeUGMasterClasses(char *lnkfb,int writeMode,DSPLoadSpec *dsp)
    /* Write leaf class and stub. WriteMode indicates whether there are no
       args. (writeMode == 2 means no 'cases' (space args)) */
{
/* For access(2) */
#define R_OK	4/* test for read permission */
#define W_OK	2/* test for write permission */
#define X_OK	1/* test for execute (search) permission */
#define F_OK	0/* test for presence of file */

    char *masterUGIncludeFileName;
    char *masterUGFileName;
    FILE *fp;
    masterUGIncludeFileName = makeUGFileName(lnkfb,2);
    masterUGFileName = makeUGFileName(lnkfb,3);
    if ((fp=_DSPMyFopen(masterUGIncludeFileName,"w"))==NULL) 
      _DSPError1(EIO,ERROR2,masterUGIncludeFileName);
    else {
	printf(" Writing unit-generator master included file:\t%s\n",
	       masterUGIncludeFileName);
	writeUG(dsp,masterUGIncludeFileName,fp,writeMode);
	fclose(fp);
    }

    /* take original file if it exists */
    if (!access(masterUGFileName,R_OK)) {
	printf("! Master UG file:\t%s ** ALREADY EXISTS **\n"
	       "  Skipping generation of master UG stub.\n",masterUGFileName);
    }
    else {
	if ((fp=_DSPMyFopen(masterUGFileName,"w"))==NULL) 
	  _DSPError1(EIO,ERROR2,masterUGFileName);
	else {
	    printf(" Writing stub of unit-generator master file:\t%s\n",
		   masterUGFileName);
	    /* This is in the file writeUG.c currently. */
	    writeUG(dsp,masterUGFileName,fp,3);
	    fclose(fp);
	}
    }
}
    

void main(argc,argv) 
    int argc; char *argv[]; 
{
    int iarg,i,j,nbad,errorcode,count,nSpaceArgs;
    char 
      *lnkfn,			/* input link file name <*/
      *cfn,			/* output c file name (USER) */
      *hfn,			/* output h file name (USER) */
      *leafUGFileName,		/* output m file name (UG) */
      *symfn,			/* output symbol table file name */
      *simfn,			/* output simulator file name */
      *lnkfe,			/* lnkfn extension */
      *lnkfb,			/* lnkfn body (filename extension removed) */
      *prefixAP=NULL,		/* prefix to replace "DSPAP" for AP calls */
      *ugargstr=NULL;		/* unit-generator arg template (dspwrap) */
    typedef enum _DSPSrcType 
      {NOST=0,			/* not specified */
	 UG,			/* unit generator main .lnk file (dspwrap) */
	 AP,			/* array processor main .lnk file (dspwrap) */
	 USER			/* user's own main .lnk or .lod file */
       } DSPSrcType;
    typedef enum _DSPAsmType 
      {NOAT=0,
	 LNK,			/* relative assembly ASM56000 output */
	 LOD			/* absolute assembly (USER only) */
       } DSPAsmType;
    DSPSrcType srctype = NOST;	/* unknown */
    DSPAsmType asmtype = NOAT;	/* unknown  */
    int start_address = -1;	/* if non-negative, overrides usual */
    int do_boot = FALSE;	/* if TRUE, generate a bootstrap load */
    int do_sys = FALSE;		/* if TRUE, allow DSP system overwrites */
    int do_sym = FALSE;		/* if TRUE, write out symbol table */
    int do_lsym = FALSE;	/* if TRUE, include local symbols in sym tab */
    int do_sim = FALSE;		/* if TRUE, write out host-interface file */
    int stackable = FALSE;	/* see dspwrap.c */
    FILE *cfp,*hfp,*symfp,*simfp;
    DSPLoadSpec *dsp;		/* symbol table and memory image on DSP */

    while (--argc && **(++argv)=='-') {
	_DSPToLowerStr(++(argv[0]));
	switch (*argv[0]) {
	  case 'a':
	    if (argv[0][1]=='p') {				/* -ap */
		srctype = AP;
		if (asmtype==LOD) _DSPErr("AP must be relative assembly");
		asmtype = LNK;
	    }
	    else if (argv[0][1]=='b')				/* -absolute */
	      srctype = USER;	/* LOD or LNK ok */
	    else if (argv[0][1]=='r') {			     /* -argtemplate */
		ugargstr = (++argv)[0];
		argc--;
	    }
	    else printf("\ndsploadwrap: ambiguous switch: %s\n",argv[0]);
	    break;
	  case 'b':						/* -boot */
	    do_boot = do_sys = TRUE;
	    break;
	  case 'l':						/* -local */
	    do_sym = do_lsym = TRUE;
	    break;
	  case 'u':
	    srctype = UG;					/* -ug */
	    if (asmtype==LOD) _DSPErr("incompatible options");
	    asmtype = LNK;
	    break;
	  case 'p':						/* -prefixAP */
	    prefixAP = (++argv)[0];
	    argc--;
	    break;
	  case 'r':						/* -relative */
	    asmtype = LNK;
	    break;
	  case 's':
	    if (argv[0][1]=='i') do_sim=TRUE;			/* -simulate */
	    else if (argv[0][1]=='y') {
		if (argv[0][2]=='m') do_sym=TRUE;		/* -symbols */
		else do_sys = TRUE;				/* -system */
	    } else if (argv[0][1]=='t') {
		if (argv[0][3]=='c')				/* -stackable*/
		  stackable = TRUE;
		else
		  start_address = _DSPGetIntStr(++argv); 	/* -start */
	    }
	    else printf("\ndsploadwrap: Ambiguous switch: %s\n",argv[0]);
	    break;
	  case 't':
	    _DSPTrace = (--argc)? _DSPGetIntStr(++argv): DSP_TRACE_DSPLOADWRAP;
	    if (_DSPTrace == 0)
	      _DSPTrace = DSP_TRACE_DSPLOADWRAP;
	    break;
	  default:
	    printf("Unknown switch -%s\n",argv[0]);
	}
    }

    if(argc == 0){
	printf("\nUsage:\t%s *.lnk\n\n",USAGE);
	exit(1);
    }

    if (!prefixAP)
      prefixAP = "DSPAP";

    for (iarg=0,nbad=0;iarg<argc;iarg++) {
	lnkfn = argv[iarg];
	if (!lnkfn) break;
	lnkfb = _DSPGetBody(lnkfn);
	lnkfe = _DSPGetTail(lnkfn);
	if (asmtype==NOAT)
	  if (strcmp(lnkfe,".lod")==0) 
	    asmtype = LOD;
	  else
	    asmtype = LNK;

	if (srctype==UG) {
	    cfn = NULL;
	    leafUGFileName = makeUGFileName(lnkfb,1);
	} else if (srctype==AP) {
	    cfn = DSPCat(prefixAP,DSPCat(rmvlastname(lnkfb),".c"));
	    hfn = DSPCat(prefixAP,DSPCat(rmvlastname(lnkfb),".h"));
	    leafUGFileName = NULL;
	} else if (srctype==USER) {
	    cfn = DSPCat("DSP",DSPCat(rmvlastname(lnkfb),".c"));
	    hfn = DSPCat("DSP",DSPCat(rmvlastname(lnkfb),".h"));
	    leafUGFileName = NULL;
	} else /* (srctype==NOST) */ {
	    printf("Relocatable input file assumed to be GENERAL PURPOSE\n");
	    srctype = USER;
	}
	symfn = do_sym?DSPCat(lnkfb,".sym"):NULL; /* NULL => skip symbol table */
	simfn = do_sim?DSPCat(lnkfb,".io"):NULL;  /* NULL => skip sim file */

	if ( (asmtype==LOD) && (leafUGFileName||symfn) )
	  printf(" Can't write header or symbol files from .lod file\n");

	/* load .lnk or .lod file (SYSTEM is absolute) */
	if (errorcode = _DSPLnkRead(&dsp,lnkfn,do_sys))
	  goto errout;

	/* set USER load offsets */
	if (srctype == UG)
	    DSPSetMKSystemFiles();
	else /* default is AP monitor */
	    DSPSetAPSystemFiles();
	if (errorcode = _DSPRelocateUser(dsp))
	  goto errout;

	/* do fixups in p,pl,ph code */
	if (errorcode = _DSPRelocate(dsp))
	  goto errout;

	if(ugargstr!=NULL) { /* Process unit generator argument symbols */
	    DSPSection *section;
	    DSPSymbol *sym;
	    int i,j,k;
	    char *c;
	    char *argt = ugargstr;
	    char argt1[] = "NN";
	    char lastSpace = 'N';
	    char *symname,*namep;
	    for ( c=ugargstr; *c && *c!='['; c++)  ;
	    if (*c++ != '[') 
	      _DSPErr("dsploadwrap: error in ugargstr from dspwrap");

	    /* 
	     * Find symname in symbols.  Add type char to type field.
	     * Argument types are currently "GRI".  Datum args will 
	     * remain the same.  Address args become type "GRIX" or "GRIY".
	     * Input address args become type "GRIXI" or "GRIYI".
	     * Output address args become type "GRIXO" or "GRIYO".
	     */
	    section = dsp->userSection;
	    nSpaceArgs = 0;
	    while ( (symname=_DSPGetTokStr(&c)) != NULL) {
		nSpaceArgs++;
		while (*argt == 'd') argt++; /* Find type from ugargstr */
		if (*argt == 'a') {
		    if (lastSpace == 'N')
		      printf("dsploadwrap: no last space for address arg\n");
		    argt1[0] = lastSpace;
		    argt1[1] = *argt++;
		} else {
		    argt1[0] = lastSpace = *argt++; /* memory space */
		    argt1[1] = *argt++; /* type (Address,Input, or Output) */
		}
		_DSPToUpperStr(argt1); /* DSP symbol type chars are u.c. */
		for (j=0;j<DSP_LC_NUM;j++) { /* loop through symbols */
		    for (k=0;k<section->symCount[j];k++) {
			sym = &section->symbols[j][k];
			namep = getlastname(sym->name);
			if (namep && *namep && strcmp(namep,symname)==0) {
			    sym->type = DSPCat(sym->type,argt1);
			    if (_DSPTrace & DSP_TRACE_DSPLOADWRAP) {
				printf("dsploadwrap: Appending type chars "
				       "%s to symbol %s:\n", argt1,symname);
				DSPSymbolPrint(*sym);
			    }
			}
		    }
		}
	    }
	}
	
	if (_DSPTrace & DSP_TRACE_DSPLOADWRAP) {
	    printf("After reading and relocating the link file:\n");
	    DSPLoadSpecPrint(dsp);
	}
	
	/* OUTPUT C FUNCTION (normal DSP programs or AP macros) */

	if (cfn!=NULL) {
	    if ((cfp=_DSPMyFopen(cfn,"w"))==NULL) 
	      _DSPError1(EIO,ERROR1,cfn);
	    else if ((hfp=_DSPMyFopen(hfn,"w"))==NULL)
	      _DSPError1(EIO,ERROR2,hfn);
	    else {
		printf(" Writing C function file:\t%s\n",cfn);
		printf(" Writing C header file:\t%s\n",hfn);
		/* DO IT (do_sys and boot make a big difference!) */
		/* remember: user mem loc 0 becomes xe:$2000 */
		if (errorcode = DSPWriteC(dsp,cfn,cfp,hfp,do_sys,do_boot,
					  lnkfb,prefixAP,stackable))
		  _DSPError1(errorcode,
				 "dsploadwrap: Could not write C file",cfn);
		fclose(cfp);
		fclose(hfp);
	    }
	}
	
	/* OUTPUT SYMBOL TABLE */
	if (symfn!=NULL) {
	    if ((symfp=_DSPMyFopen(symfn,"w"))==NULL)
	      _DSPError1(EIO,ERROR4,symfn);
	    else {
		printf(" Writing symbol table file:\t%s\n",symfn);
		printf("%sncluding LOCAL symbols\n",do_lsym?" I":" Not i");
		/* DO IT (REMEMBER TO USE do_lsym) */
		/* errorcode = DSPWriteSymbols(dsp,symfp,do_sys,do_lsym); */
		fclose(symfp);
	    }
	}
	
	/* 
	  Consider just doing a _DSPStartLog in the C function above.
	  That would be run-time generation of the host-interface file.
	  It probably wants a simple switch in the code for that, like
	  a global boolean.  Here we want to generate it directly right
	  now.
	*/
	
	/* OUTPUT DSP SIMULATOR HOST-INTERFACE FILE */
	if (simfn!=NULL) {
	    if ((simfp=_DSPMyFopen(simfn,"w"))==NULL)
	      _DSPError1(EIO,ERROR8,simfn);
	    else {
		printf(" Writing DSP simulator host-interface file:\t%s\n",
		       simfn);
		/* DO IT */
		fclose(simfp);
	    }
	}

	/* OUTPUT M FILE (UG macros) */
	if (leafUGFileName!=NULL && ugargstr!=NULL) {
	    FILE *fp;
	    if (nSpaceArgs==0) 
	      /* No "cases" = memory space arguments */
	      writeUGMasterClasses(lnkfb,2,dsp);
	    else {
		/* Have memory address arguments */
		for(i=0;i<strlen(ugargstr);i++)
		  if(ugargstr[i]=='y') break;
		if (i==strlen(ugargstr))  /* Only do it on all x's */
		  writeUGMasterClasses(lnkfb,1,dsp);
		if ((fp=_DSPMyFopen(leafUGFileName,"w"))==NULL) 
		  _DSPError1(EIO,ERROR2,leafUGFileName);
		printf(" Writing unit-generator leaf class file:\t%s\n",
		       leafUGFileName);
		writeUG(dsp,leafUGFileName,fp,0);
		fclose(fp);
	    }
	}
	
    errout:
	if (errorcode) {
	    printf("Error code for file %s = %x\n",
		   lnkfn,errorcode);
	    nbad += 1;
	    continue;
	}
    }

    if (nbad>0) {
	printf(" Number of input files having nonzero error code = %d\n",
	       nbad);
	exit(1);
    }
    exit(0);
}

