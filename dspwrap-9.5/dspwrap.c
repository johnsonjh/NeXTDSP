#define DSP_RELEASE 0           /* nonzero if release version */

/* dspwrap.c - wrap a DSP macro (AP or UG types) in a host C function

DESCRIPTION
        See DSPDOC/dspwrap.

Modification history
        11/25/87/jos - Changed ncases=1 case to all y instead of all x
        12/14/87/jos - Changed "mkdsp" to "DSP" everywhere (and in mydsp.h)
        01/15/88/jos - Added code to generate links in MANDIR to man pages
        01/28/88/jos - changed to use dsploadwrap. UG&AP only. Main support
                       taken out. dspload now handles main programs.
        02/20/88/jos - added argtemplate support for dsploadwrap.
        03/21/88/jos - extended argtemplate for dsploadwrap (added arg names).
        08/03/88/jos - changed /bin to /usr/bin
        08/16/88/jos - added -xonly option for x-space-only UG makes
        08/19/88/jos - added -DMUSIC_KIT to asm56000 command line       
        08/30/88/jos - added -local_dir option
        12/01/88/jos - added DSP_RELEASE switch
        12/20/88/jos - added switch -mk to set music-kit mode variable do_mk.
        03/16/89/jos - installed workaround for link to binary bug.
                       For now, we always /ds/dsp/bin version of dsploadwrap.
        03/25/89/jos - added <omit> support and more fields to suppress.
        03/30/89/jos - fixed <omit> support
        04/13/89/jos - made do_c the default so UG ObjC generation works
                       Formerly, do_c was set FALSE for -ug types.
                       How did it ever work?!?
        04/30/89/jos - changed to use of installed libdsp
        04/30/89/jos - we now use installed asm56000 or $DSP/bin/asm56000
        04/30/89/jos - added -dsploadwrapDir option
                       If not used, we use installed dlw or $DSP/bin/dlw
        05/03/89/mtm - added missing dash for dsploadwrapDir argument in
                       USAGE string.  Call dsploadwrap with binary link
                       file name if AP.
        05/15/89/mtm - Remove -mk switch.
        06/06/89/mtm - Removed -noInclude from USAGE string (not implemented)
        06/06/89/mtm - Added generation of function documentation
        06/08/89/mtm - Generate end directive using DSP{AP,MK}_PLI_USR.
        06/12/89/mtm - Removed -noSymbols from USAGE line.
                       (wait until implemented)
        06/15/89/mtm - made fundoc generate prefix as "DSPAP"
                       Bug fix: only generate arg template if do_lnk.
        06/20/89/mtm - force summary file output to current directory.
	08/15/89/mtm - look for bins only in /usr/bin (ignore $DSP).
		       Add -macroDir switch.
		       AP_FUNCTION_DOC ignores PSEUDO-C NOTATION.
		       Disabled stripzero warning message unless _DSPTrace.
		       Implement (address) argument type.
	08/22/89/mtm - Change man page file extension from cat to l (ell)
	08/31/89/mtm - Fix bug passing -trace to dsploadwrap.
		       Fix usage string.
	03/16/90/jos - Added -prefix <string> option to override DSPAP().
	03/16/90/jos - Added "(input)iaddr,(output)oadr" support to arg info.
	05/02/90/jos - Repaired incorrect logic in creating dspace arg list.
	05/23/90/jos - Made (dspace) force the next datum to be an address 
		       (for compatibility with 1.0 UG and AP files).
        06/12/90/jos - Removed call to stripzero in creation of ugargnames.
		       Simultaneously removed '0' suffix on args to 
		       DSPWRAP ARGUMENT INFO in ugsrc/*.asm and apsrc/*.asm.
		       This was a crock that would have been embarrassing.
	08/26/90/jos - added "-stackable" option.

TO DO
        In dspasm: check date of lnk file and don't assemble if up to date
        Make macrotype==UG synonymous with do_spaces and remove extra tests
        "if (macrotype==UG)" in cases loop.
        Create .1 files instead of .cat files using .nf troff directive
*/

#define USAGE "dspwrap [-{ug,ap}] [-no{Doc,Link,C}] [-o<path>] [-trace <n>] [-xonly] [-local <path>] [-dsploadwrapDir <path>] [-macroDir <path>] [-prefixAPcalls <string> [-stackable]] <filename(s)>"

#import <dsp/dsp.h>
#import <stdio.h>
#import <strings.h>
#import <sys/file.h>
#import <appkit/nextstd.h>

#define MAXARGS 100             /* maximum number of unit generator args */
#define MAX_LINE 256		/* max chars per line on input (via fgets) */

/* private functions from libdsp */
extern char *_DSPToLowerStr();
extern char *_DSPCopyStr();
extern int _DSPGetIntStr();
extern char *_DSPGetBody();
extern char *_DSPGetTail();
extern FILE *_DSPMyFopen();
extern char *_DSPGetTokStr();
extern char *_DSPPadStr();
extern char *_DSPSkipWhite();
extern int _DSPNotBlank();
extern char *_DSPRemoveTail();
extern char *_DSPMakeStr();
extern char *_DSPCVS();

/* private variables from libdsp */
extern int _DSPTrace;

/* left-over undeclared tidbits */
extern int access();            /* ? Not in <sys/file.h> as man says */
extern int setlinebuf();        /* ? Not in <stdio.h> as man says */
extern int unlink();            /* ? */
extern int symlink();           /* ? */

/* remedial define's (for compiling with pre-dsp-18 libdsp's) */
#ifndef DSP_SYS_REV_C
#define DSP_SYS_VER_C 1
#define DSP_SYS_REV_C 17
#define DSP_PLI_USR_C 0x80
#endif

/* Also see the DSP include directory used in the system() command below */
#define MANDIR "/usr/next/man/man7/" /* manual directory */
#define CATDIR "/usr/next/man/cat7/" /* manual directory */
#define APSUBDIR "/ap/"         /* subpath for ap documentation (NULL ok) */
#define UGSUBDIR "/ug/"         /* subpath for ug documentation (NULL ok) */
#define APOLS "AP_MACROS_SUMMARY" /* AP one-line summary filename */
#define UGOLS "UG_MACROS_SUMMARY" /* UG one-line summary filename */
#define APMLS "AP_CALL_SUMMARY" /* multi-line summary filename */
#define APFUNDOC "AP_FUNCTION_DOC" /* functions documentaion filename */
#define MANSECTION "l"        /* man page suffix */

typedef enum _macrotypecode {UG=0,AP,ODD,NONE} macrotypecode;
typedef enum _argcode 
	{dsp24=0,prefix,instance,dspace,literal,address,input,output} argcode;

char *stripzero(c)              /* strip trailing zero from UG arg name */
    char *c;
{    
    int n;
    n = strlen(c);
    if (c[n-1]!='0') {
	if (_DSPTrace)
	  printf("stripzero: name '%s' does not have trailing '0'\n",c);
    } else
      c[n-1] = '\0';
    return(c);
}

char *my_fgets(s, n, stream)
    char *s;
    int n;
    FILE *stream;
{
    char *r;
    do {
        r = fgets(s, n, stream);
    } while ((strncmp(s,";;<omit>*",7)==0));
    return r;
}

extern int dspasm();            /* defined below */

/*** Global variables (set in main and used by dspasm() below) ***/
char  *local_dir;               /* local include directory, if any */
char  *local_macro_dir;         /* local macro directory, if any */
int do_local = FALSE;           /* use local include files */
int do_local_macro = FALSE;     /* use local macro files */

void main(argc,argv) 
    int argc; char *argv[]; 
{
    int iarg,i,j,ndspaceargs,nmargs,nugargs,nspaces,icase,ncases,nbad,errorcode;
    char 
      *asmfn,                   /* input assembly language file */
      *asmfb,                   /* body of asmfn (path and ext removed) */
      *masmfn,                  /* generated main program */
      *masmfb,                  /* body of masmfn (path and ext removed) */
      *olsfn,                   /* one-line summary file */
      *mlsfn,                   /* multi-line summary file */
      *fundocfn,                /* functions documentation file */
      *manpfn,                  /* man page file */
      *lnkfn,                   /* input link file name */
      *dspfn,                   /* input binary link file name */
      *ugargstr,                /* unit-generator argument string */
      *ugargnames,              /* unit-generator ptr-argument names */
      *local_dlw_dir;           /* local dsploadwrap directory, if any */
    FILE *asmfp,*masmfp,*olsfp,*mlsfp,*fundocfp,*manpfp;
    char *tok,*c,*lineptr,*progname=argv[0];
    char *opath=DSPCat(DSPGetDSPDirectory(),"/man/");
    char *apsubdir=APSUBDIR,*ugsubdir=UGSUBDIR;
    char *fundoc_nametok;       /* macro name for function doc */
    char *prefixAP = NULL;        /* prefix for AP function calls */
    char *fundoc_ols;           /* one-line summary for function doc */
    char *fundoc_description;   /* description section for function doc */
    char linebuf[MAX_LINE];        /* input line buffer */
    macrotypecode macrotype = NONE;
    argcode argtype[MAXARGS];   /* UG argument descriptor */
    char *litarg[MAXARGS];      /* UG literal argument strings */
    int verbose = TRUE;
    int sys_clobber = FALSE;    /* if TRUE, allow DSP system overwrites */
    int do_ols = TRUE;          /* if TRUE, write out DSP one-line summary */
    int do_mls = TRUE;          /* if TRUE, write out DSP multi-line summary */
    int do_fundoc = TRUE;       /* if TRUE, write out function documentaion */
    int do_man = TRUE;          /* if TRUE, write out DSP man page */
    int do_lnk = TRUE;          /* assemble DSP main program */
    int do_spaces = DSP_MAYBE;  /* create DSP main for each x/y combination */
    int do_c = TRUE;            /* write out DSP C or ObjC function */
    int do_sym = FALSE;         /* write out symbol table */
    int do_local_dlw = FALSE;   /* use local dsploadwrap */
    int stackable = FALSE;      /* FALSE => DSPAPLoadGo(), else DSPAPLoad() */
    
    argv++; argc--;
    while (argc && **argv=='-') {
        _DSPToLowerStr(++(*argv));
        switch (**argv) {
          case 'd':
            local_dlw_dir = _DSPCopyStr(*++argv); argc--;
            printf("Local dsploadwrap directory set to %s\n",local_dlw_dir);
            do_local_dlw = 1;
            break;
          case 'l':
            local_dir = _DSPCopyStr(*++argv); argc--;
            printf("Local include directory set to %s\n",local_dir);
            do_local = 1;
            break;
          case 'm':
            local_macro_dir = _DSPCopyStr(*++argv); argc--;
            printf("Local macro directory set to %s\n",local_macro_dir);
            do_local_macro = 1;
            break;
          case 'a':
            macrotype = AP;
            break;
          case 't':
            if (--argc)
              _DSPTrace = _DSPGetIntStr(++argv);
            else
              _DSPTrace = -1;
            break;
          case 'u':
            macrotype = UG;
            break;
          case 'n':             /* noDoc, noLink, noC, noSym */
            if (argv[0][2]=='d') do_ols=do_mls=do_fundoc=do_man=FALSE;
            if (argv[0][2]=='l') do_lnk=FALSE;
            if (argv[0][2]=='c') do_c=FALSE;
            if (argv[0][2]=='s') do_sym=FALSE;
            break;
          case 'o':
            opath = _DSPCopyStr(*(++argv)); /* output path for DOCUMENTATION */
            argc--;
            break;
          case 'x':             /* -xonly */
            do_spaces = FALSE;
            break;
          case 'p':             /* -prefixAP */
            prefixAP = _DSPCopyStr(*++argv); argc--;
            break;
          case 's':             /* -stackableAP */
            stackable = TRUE;
            break;
          default:
            printf("Unknown switch -%s\n",*argv);
        }
        --argc; ++argv;
    }

    if(argc == 0){
        printf("\nUsage:\t\t%s *.asm\n\n",USAGE);
        exit(1);
    }

    if (!do_lnk) 
      do_c=do_sym=FALSE;

    if (do_ols) {
        olsfp = NULL;
        /* defer open until after macrotype is determined below */
    }
    if (do_mls) {
        mlsfp = NULL;
        /* defer open until after macrotype is determined below */
    }
    if (do_fundoc) {
        fundocfp = NULL;
        /* defer open until after macrotype is determined below */
    }
    for (iarg=0,nbad=0;iarg<argc;iarg++) {
        asmfn = argv[iarg];
        if (!asmfn)
          break;                /* args miscounted */
        asmfb = _DSPGetBody(asmfn);
        if (strcmp(_DSPGetTail(asmfn),".asm")!=0) {
            printf(" *** Skipping non-.asm file %s ***\n", asmfn);
            continue;
        }
        if ((asmfp=_DSPMyFopen(asmfn,"r"))==NULL) 
          continue;
        else 
          printf("\n Reading source file:\t%s\n",asmfn);

        /* Advance to documentation section */
        if (do_ols||do_man||do_fundoc)
          while (1) {           /* search for ";;  NAME" */
              if (my_fgets(linebuf,MAX_LINE,asmfp)==NULL) {
                  printf(" \";;  NAME\" field missing.\n");
                  printf(" *** Skipping file %s ***\n",asmfn);
                  goto argloopend;
              }
              if (strncmp(linebuf,";;  NAME",7)==0) break;
          }

        /* Read one-line summary and deduce macro type */
        if (do_ols || do_fundoc) {
            char *nametok, *typetok;
            /* Read one-line summary from file */
            if (my_fgets(linebuf,MAX_LINE,asmfp)==NULL) {
                printf(" \";;  NAME\" field missing.\n");
                printf(" *** Skipping file %s ***\n",asmfn);
                goto argloopend;
            }
            lineptr = linebuf;
            nametok = _DSPGetTokStr(&lineptr); /* macro name */
            fundoc_nametok = DSPCat("", nametok);
            nametok = _DSPPadStr(nametok,15);  /* pad string to fixed width */
            /* advance to "(UG macro)" or "(AP macro)" */
            if (macrotype==NONE)
              if (*_DSPSkipWhite(lineptr)=='(') {
                  _DSPToLowerStr(typetok=_DSPGetTokStr(&lineptr));
                  if (strcmp(typetok,"ug")==0) {
                      macrotype = UG;
                      printf(" Macro type assumed is UG.\n");
                  }
                  else if (strcmp(typetok,"ap")==0) {
                      macrotype = AP;
                      printf(" Macro type assumed is AP.\n");
                  }
                  else printf(
                      " *** unknown macro type specified in \"%s\" ***\n",c);
              }
              else {
                  macrotype = UG;       /* "(xx macro)" is optional */
                  printf(" No macro type specification.\n");
                  printf(" Macro type assumed is UG.\n");
              }

   /*** At this point, the macro type is known to be either AP or UG ***/

            while (*lineptr!='-') lineptr++; /* delete "(UG macro)" */
            fundoc_ols = DSPCat("", lineptr);
            if (strncmp("- ", fundoc_ols, 2) == 0)
                fundoc_ols += 2;
            if (do_ols && olsfp==NULL) {        /* output not yet open */
                /* force output to current directory */
                apsubdir = ugsubdir = NULL;
                olsfn = (macrotype==AP?APOLS:UGOLS);
                opath = "./";
                if ((olsfp=_DSPMyFopen(olsfn,"w"))==NULL) continue;
                printf(" Writing oneline summary file:\t%s\n",olsfn);
                setlinebuf(olsfp); /* write strings on stdout UNBUFFERED. */
                fprintf(olsfp,"%s MACROS - SUMMARY\n\n",
                        macrotype==AP?"ARRAY PROCESSING":"UNIT GENERATOR");
            }
            if (do_ols) {
                fprintf(olsfp,nametok); /* output UG macro name */
                fprintf(olsfp,lineptr); /* output one-line summary */
            }
        }

        if (do_man) {           /* Start man page output */
            manpfn = DSPCat(opath,DSPCat(macrotype==AP?apsubdir:ugsubdir,
                         DSPCat(asmfb,DSPCat(".",MANSECTION))));
            if ((manpfp=_DSPMyFopen(manpfn,"w"))==NULL) continue;
            else printf(" Writing man page:\t%s\n",manpfn);
            fprintf(manpfp,"NAME\n");
            fprintf(manpfp,linebuf+3);/* output 1line summary to man page */
        }

        if (macrotype==AP) do_spaces=FALSE; /* only use x array memory */

        /* Advance to DESCRIPTION specification */
        while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                && (strncmp(linebuf,";;",2)==0)
                && (strncmp(linebuf,";;  DESCRIPTION",15)!=0) ) 
            if (do_man)
              fprintf(manpfp,linebuf+(strlen(linebuf)>3?3:2)); /* man page */
        if (do_man)
            fprintf(manpfp,(linebuf+(strlen(linebuf)>3?3:2)));
            
        /* Gather description for function doc */
        if (do_fundoc && macrotype == AP) {
            fundoc_description = "";
            while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                    && (strncmp(linebuf,";;",2)==0)
                    && (strncmp(linebuf,";;  PSEUDO-C NOTATION",21)!=0)) {
                  fundoc_description = DSPCat(fundoc_description,
                                       DSPCat("  ",linebuf+2));
                  if (do_man)
                    fprintf(manpfp,(linebuf+(strlen(linebuf)>3?3:2)));
            }
            if (do_man)
                fprintf(manpfp,(linebuf+(strlen(linebuf)>3?3:2)));
        }

        /* Write function documentation */
        if (do_fundoc && macrotype == AP) {
            if (fundocfp==NULL) {       /* output not yet open */
                fundocfn = DSPCat(opath, DSPCat(apsubdir,APFUNDOC));
                if ((fundocfp=_DSPMyFopen(fundocfn,"w"))==NULL)
                          continue;
                printf(" Writing function documentation file:\t%s\n",fundocfn);
                setlinebuf(fundocfp); /* write strings on stdout UNBUFFERED. */
            }
            fprintf(fundocfp, "DSPAP%s()\n\n", fundoc_nametok);
            fprintf(fundocfp,"    SUMMARY\t%s\n", fundoc_ols);
            fprintf(fundocfp,"    LIBRARY\tlibarrayproc\n\n");
            fprintf(fundocfp,"    SYNOPSIS\n");
            fprintf(fundocfp,"        #import <dsp/arrayproc.h>\n\n");
            fprintf(fundocfp,"        All arguments are of type int.\n\n");
         }
         
        /* Advance to DSPWRAP ARGUMENT INFO specification */
        while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                && (strncmp(linebuf,";;",2)==0)
                && (strncmp(linebuf,";;  DSPWRAP ARGUMENT",20)!=0) ) 
          if (do_man)
            fprintf(manpfp,linebuf+(strlen(linebuf)>3?3:2)); /* man page */

/* MACRO ARGUMENT TEMPLATE */
        if (do_lnk) {
            if (strncmp(linebuf,";;  DSP",7)!=0) {
nodwai:         printf(" \";;  DSPWRAP ARGUMENT INFO\" field missing.\n");
                printf(" *** Skipping assembly of file %s ***\n",asmfn);
                printf(" (Documentation generated should be ok)\n");
                goto argloopend;
            }
            my_fgets(linebuf,MAX_LINE,asmfp); /* DSPWRAP argument info line */
            c = linebuf;
            tok=_DSPGetTokStr(&c);      /* DSP macro name */
            if (!tok) goto nodwai; /* above */
            if (strcmp(tok,asmfb)!=0)
              printf(
                  " *** asm filename = /%s/ differs from macro name = /%s/\n",
                  asmfb,tok);

            /* EXTRACT MACRO ARGUMENTS FROM TEMPLATE */
            for (i=0;i<MAXARGS;) { /* arg template not forwarded to doc */
                while (*c==' '||*c=='\t'||*c==','&&*c) c++;
                if (!*c||*c=='\n'||*c==';') { 
                    /* input line used up. Get next: */
                    my_fgets(linebuf,MAX_LINE,asmfp); /* arg info contin */
                    c=linebuf;
                    while (*c==' '||*c=='\t'||*c==';'&&*c) c++;
                }
                if (*c=='(') {
                    tok = _DSPGetTokStr(&c); /* argument type declaration */
                    if (strcmp(tok,"prefix")==0) argtype[i++]=prefix;
                    else if (strcmp(tok,"instance")==0) argtype[i++]=instance;
                    else if (strcmp(tok,"dspace")==0) argtype[i++]=dspace;
                    else if (strcmp(tok,"literal")==0) argtype[i++]=literal;
                    else if (strcmp(tok,"address")==0) argtype[i++]=address;
                    else if (strcmp(tok,"input")==0) argtype[i++]=input;
                    else if (strcmp(tok,"output")==0) argtype[i++]=output;
                }
                else argtype[i++] = dsp24;
                tok = _DSPGetTokStr(&c); /* actual arg mnemonic */
                if (tok==NULL) {i--; break;} /* no more args */
                litarg[i-1]=_DSPToLowerStr(tok);        /* save literal arg */
                if (i>=MAXARGS) {
                    printf(" *** More than %d args found!!! ***\n",i);
                    exit(1);
                }
            }
            nmargs = i;
            printf(" Arg template: ");
            for (i=0;i<nmargs;i++) 
              printf("%s%s",
                     (argtype[i]==dsp24?"(datum)":
                      (argtype[i]==dspace?"(dspace)":
                       (argtype[i]==prefix?"(prefix)":
                        (argtype[i]==instance?"(instance)":
                         (argtype[i]==literal?"(literal)":
                          (argtype[i]==address?"(address)":
                           (argtype[i]==input?"(input)":
                            (argtype[i]==output?"(output)":
                            "!!!unknown!!!")))))))),litarg[i]);
            printf("\n");
        }
        else    /* omit DSPWRAP ARGUMENT INFO field from man page */
          while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                  && _DSPNotBlank(linebuf+2) )
            ;

        /* Append names of all memory-space prefixed UG arguments */
	/* Address arguments are also added */
	/* Note that the (address) qualifier cannot appear when a (dspace)
	   qualifier has been used also --- only when (dspace) is not there. */
        if (do_lnk) {
	    int dspacePending=0;
            ugargnames = "[";
            for (j=0,ndspaceargs=0;j<nmargs;j++) {
		if (argtype[j]==dspace) /* "x" or "y" space spec */
		  dspacePending=1; /* Must force next datum to type (address) */
		else if (argtype[j]==address 
			 || argtype[j]==input 
			 || argtype[j]==output || dspacePending) {
		    ndspaceargs++;
		    ugargnames = DSPCat(ugargnames,
					DSPCat(litarg[j],","));
				       /* DSPCat(stripzero(litarg[j]),",")); */
		    dspacePending=0;
		}
	    }
	    if (ndspaceargs>0)
              ugargnames[strlen(ugargnames)-1]=']'; /* overwrite ',' */
            else 
              ugargnames = "[]";
            ugargnames = DSPCat(ugargnames," "); /* needed separator */
            if (_DSPTrace)
              printf("dspwrap: ugargnames = %s\n",ugargnames);
        }
        
        if ( (do_mls || do_fundoc) && macrotype==AP ) {
            if (do_mls && mlsfp==NULL) {        /* output not yet open */
                mlsfn = DSPCat(opath,DSPCat(apsubdir,APMLS));
                if ((mlsfp=_DSPMyFopen(mlsfn,"w"))==NULL) continue;
                printf(" Writing calling-sequence summary file:\t%s\n",mlsfn);
                setlinebuf(mlsfp); /* write strings on stdout UNBUFFERED. */
                fprintf(mlsfp,
                    "ARRAY PROCESSING C FUNCTION CALLING SEQUENCES\n");
            }
            
            /* Advance to C syntax specification */
            while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                    && (strncmp(linebuf,";;",2)==0)
                    && (strncmp(linebuf,";;  DSPWRAP C SYNTAX",20)!=0) )
              if (do_man)
                fprintf(manpfp,(linebuf+(strlen(linebuf)>3?3:2))); /* manpage */
            if (strncmp(linebuf,";;  DSP",7)!=0)
                printf(" \";;  DSPWRAP C SYNTAX\" field missing.\n");
            else { /* output calling sequence summary */
                if (do_mls) fprintf(mlsfp,"\n");
                while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                        && (strncmp(linebuf,";;",2)==0)
                        && _DSPNotBlank(linebuf+2) ) {
                  if (do_mls)
                      fprintf(mlsfp,(linebuf+8)); /* calling-seq summary */
                  if (do_fundoc)
                      fprintf(fundocfp,"        %s", (linebuf+8));
                  }
            }
            
            /* Write function doc DESCRIPTION section */
            if (do_fundoc && macrotype == AP) {
                fprintf(fundocfp, "\n    DESCRIPTION\n");
                fprintf(fundocfp, "%s", fundoc_description);
                fprintf(fundocfp, "    RETURN\n");
                fprintf(fundocfp, "        Integer error code which is 0 "
                    "on success, nonzero on failure.\n\n\n");
            }
        }
            
/* FINISH OFF MAN PAGE */
        if (do_man) {
            FILE *docfp;
            while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                    && (strncmp(linebuf,";;",2)==0) ) {

                /* OMIT fields listed below */
                if (  
                    (strncmp(linebuf,";;  ALU REGISTER USE",20)==0)  
                    || (strncmp(linebuf,";;  REGISTER USE",16)==0) 
                    || (strncmp(linebuf,";;  TEST PROGRAM",16)==0) 
                    || (strncmp(linebuf,";;  MINIMUM EXECUTION TIME",26)==0) 
                   ) {
                    while  ( (my_fgets(linebuf,MAX_LINE,asmfp)!=NULL) 
                            && _DSPNotBlank(linebuf+2) )
                      ;
                    continue;   /* to avoid extra blank line */
                }

                fprintf(manpfp,linebuf+(strlen(linebuf)>3?3:2));
            }
            if (macrotype==AP) /* Look for .doc file and append it */
              if ((docfp=fopen(DSPCat(_DSPRemoveTail(asmfn),".doc"),"r"))!=NULL) {
                  fprintf(manpfp,"DISCUSSION\n");
                  while  ( (my_fgets(linebuf,MAX_LINE,docfp)!=NULL) )
                    fprintf(manpfp,"     %s",linebuf);
                  fclose(docfp);
              }
            fclose(manpfp);     /* MAN PAGE CLOSED */
            /* Create symbolic link to man page in man directories */
            /* linksysdoc(manpfn);      /* below */
        }
        fclose(asmfp);

        if (do_spaces==DSP_MAYBE) do_spaces = (macrotype==UG?TRUE:FALSE);

/*
 * Create .lnk file(s) 
 */
        if (!do_lnk) goto argloopend;

        lnkfn = DSPCat(asmfb,".lnk");  /* e.g. dspprog.lnk */

        /* Generate main assembly language prog for each dspace case */
        
        for (i=0,nspaces=0,ncases=1;i<nmargs;i++) 
          if (argtype[i]==dspace) {
              nspaces++;
              ncases *= 2;      /* if do_spaces */
          }
        
        if (!do_spaces) ncases=1; /* Only generate the x memory case */
        
        for (icase=0;icase<ncases;icase++) {
            masmfb = DSPCat(asmfb,"_"); /* Example: onezero -> onezero_xx */
#           define SPACECASE (icase&(1<<j)?"y":"x")
#           define SPACECASEPP (icase&(1<<j++)?"y":"x")
            for (j=0;j<nspaces;j++)
              masmfb = DSPCat(masmfb,SPACECASE);
            for (i=j=0;i<nmargs;i++)
              if (argtype[i]==dspace) litarg[i] = SPACECASEPP;
            lnkfn = DSPCat(masmfb,".lnk");     /* e.g. onezero_xx.lnk */
            masmfn = DSPCat(masmfb,".asm"); /* e.g. onezero_xx.asm */
            if ((masmfp=_DSPMyFopen(masmfn,"w"))==NULL) continue;
            else printf(" Writing main program  :\t%s\n",masmfn);
            fprintf(masmfp,
                    "; %s - Program for assembling macro %s in relocatable form\n;\n",
                    masmfn,asmfn);
            fprintf(masmfp,
                    "%s\t%s ident %d,%d\t\t ; macro shell for %s\n",
                    masmfb,(strlen(masmfb)<8?"\t":""),
                    DSP_SYS_VER_C,DSP_SYS_REV_C,asmfn);
            fprintf(masmfp,"\
ASM_SYS\t\t set 0\t\t\t ; omit assembling DSP system\n\
NO_MESSAGES\t set 1\t\t\t ; omit configuration printout\n\
\t\t include '%s_macros'\t ; utility macros\n\
\t\t section USER\n", macrotype == AP ? "ap" : "music");
            fprintf(masmfp,"\t\t %s %s",asmfb,macrotype==AP?"ap":"orch");
            for (j=0;j<nmargs;j++) 
              if (argtype[j]!=prefix)
                fprintf(masmfp,",%s",
                        ((argtype[j] == dsp24 || argtype[j] == address || 
			 argtype[j] == input || argtype[j] == output )?"0":
                         (argtype[j]==dspace?litarg[j]:
                          (argtype[j]==instance?"1":
                           (argtype[j]==literal?litarg[j]:
                            "!!!unknown!!!")))));

            fprintf(masmfp,"\n\t\t endsec\n"
		    "\t\t end $%x\n", DSP_PLI_USR_C);
            fclose(masmfp);
            
            /* Assemble generated file */
            
            errorcode = dspasm(masmfn,macrotype);       /* below */

            if (errorcode)
              continue;
            
            if (macrotype==AP) { /* Create name.dsp from name_xxx.lnk */
                char *sysstr;
                dspfn = DSPCat(asmfb,".dsp"); /* no '_x...x' suffix */
                sysstr = DSPCat(DSPCat(
                         DSPCat("/usr/bin/dspimg ",lnkfn)," "),dspfn);
                printf("%s\n",sysstr);
                errorcode=system(sysstr);
            }
            
            /* Make ugargstr which is passed to dsploadwrap. 
               Its purpose is to specify the name and space of each 
               macro argument which is an address.  The possible tokens
	       are 
	       		"xa" - x address (16 bits)
	       		"ya" - y address (16 bits)
			"xi" - x memory address used as an input
			"yi" - y memory address used as an input
			"xo" - x memory address used as an output
			"yo" - y memory address used as an output
	       		"a"  - address (16 bits) (inherits last space spec'd)
	       		"d"  - datum (24 bits)

	       Example: "yidxoddxad[ainp,aout,atab]" specifies two DSP address
               arguments, "ainp" and "aout", a third address which is neither 
	       and input or an output, and 4 data args. dsploadwrap 
               uses this string to add an extra type characters 
	       ('XI', 'XO', 'XA', 'YI', 'YO', or 'YA')
               to the symbols having said names.  These type characters
               do NOT specify the space where the argument lives, but rather
               the space into which the argument points (obtained from the
               preceding macro argument.) This extra space info enables
               more careful error checking at run time, allowing detection
               of an attempt to pass an address to a unit generator which 
               points into the wrong space.  For example, trying to plug
               unoise_y into scale_xx would generate an error.
	       
	       An argument of type (address) takes on the last encountered
	       (dspace).  This allows dsploadwrap to treat a datum as an 
	       address without generating a space case for it.  Eg.:
	       (dspace)x,ainp1,datum,(address)ainp2 --> xadxa[ainp1,ainp2]
            */

            if (macrotype==UG) {
		char *ugasp,c='\0';
		int dspacePending=0;
                ugargstr = _DSPMakeStr(nmargs+14,"-argtemplate ");
		ugasp = ugargstr + 13; /* point to end of above init string */

                for (i=0,nugargs=0;i<nmargs;i++) {
		    if (argtype[i]==dspace) { /* "x" or "y" space spec */
			c = *litarg[i]; /* set current memory space */
			*ugasp++ = c;
			dspacePending=1; /* needed to force following datum
					    to type (address) */
		    } 
		    else {
			if (argtype[i]==address) {
			    nugargs++;
			    if (!dspacePending)
			      *ugasp++ = c;
			    *ugasp++ = 'a';
			}
			else	/* catch (dspace){x,y},<name> */
			  if (argtype[i]==dsp24) { /* no (input) or (output) */
			      nugargs++;
			      *ugasp++ = (dspacePending ? 'a' : 'd');
			  }
			  else 
			    if (argtype[i]==input) {
				nugargs++;
				if (!dspacePending)
				  *ugasp++ = c;
				*ugasp++ = 'i';
			    }
			    else
			      if (argtype[i]==output) {
				  nugargs++;
				  if (!dspacePending)
				    *ugasp++ = c;
				  *ugasp++ = 'o';
			      }
			dspacePending=0;
		    }
	      }

                *ugasp = '\0';

                ugargstr = DSPCat(ugargstr,ugargnames);

                if (_DSPTrace)
                  printf("dspwrap: ugargnames = %s\n",ugargnames);
            }
                    
            /* Generate desired output files */

            if (!errorcode&&(do_c||do_sym)) {
                char *sysstr;
                printf(" Generating DSP C function from:\t%s\n",
                  (macrotype == AP ? dspfn : lnkfn));
                printf(
    " ========================= dsploadwrap LOG ========================\n");

                sysstr = (do_local_dlw ? local_dlw_dir : "/usr/bin");
                sysstr = DSPCat(sysstr, "/dsploadwrap ");

                if (_DSPTrace) 
                  sysstr = DSPCat(sysstr,
                                   DSPCat("-trace ",
                                           DSPCat(_DSPCVS(_DSPTrace)," ")));
                if (do_sym)        
                  sysstr = DSPCat(sysstr,"-symbols ");
                if (macrotype==UG) 
                  sysstr = DSPCat(sysstr,DSPCat("-ug ",ugargstr));
                else if (macrotype==AP) 
                  sysstr = DSPCat(sysstr,"-ap");
                sysstr = DSPCat(sysstr," ");
                if (prefixAP && (macrotype == AP)) 
                  sysstr = DSPCat(sysstr,
                                   DSPCat("-prefixAP ",
                                           DSPCat(prefixAP," ")));
                if (stackable && (macrotype == AP)) 
                  sysstr = DSPCat(sysstr,"-stackable ");
                if (macrotype==AP) 
                  sysstr = DSPCat(sysstr,dspfn);
                else
                  sysstr = DSPCat(sysstr,lnkfn);
                fprintf(stderr,"%s\n",sysstr);
                errorcode=system(sysstr);
                printf(
    " ======================= END dsploadwrap OUTPUT ===================\n");
                
                if (errorcode) {
                    printf(" Aborting processing of file %s.\n",asmfn);
                    nbad += 1;
                    goto argloopend;
                }
            }                   /*  if (do_c, etc.) {*/
        }                       /* cases loop */
      argloopend:;
    }                           /* for (iarg=0,nbad=0;iarg<argc;iarg++) { */
        
    if (do_ols) {
        char *snp;
        if (olsfp) {
            fclose(olsfp);
            /* linksysdoc(olsfn); */
        } else
          fprintf(stderr,"dspwrap: One-line summary file was never opened\n");
    }
    if (do_mls) {
        char *snp;
        if (mlsfp) {
            fclose(mlsfp);
            /* linksysdoc(mlsfn); */
        } else if (macrotype==AP)
          fprintf(stderr,"dspwrap: Multi-line summary file was never opened\n");
    }
    if (do_fundoc) {
        char *snp;
        if (fundocfp) {
            fclose(fundocfp);
            /* linksysdoc(mlsfn); */
        } else if (macrotype==AP)
          fprintf(stderr,"dspwrap: Function documentation file was never opened\n");
    }
    if (nbad>0) 
      printf(" Number of input files aborted by errors = %d\n",nbad);

    exit(nbad? 1: 0);
}

/* ================================================================ */

/* DSPASM */
int dspasm(asmfn,macrotype)
    char *asmfn;
    macrotypecode macrotype;
{
/* For access(2) */
#define R_OK    4/* test for read permission */
#define W_OK    2/* test for write permission */
#define X_OK    1/* test for execute (search) permission */
#define F_OK    0/* test for presence of file */

    char *asmfb,*lnkfn,*lstfn, *asmStr;

    int errorcode=0;
    asmfb = _DSPGetBody(asmfn);
    lnkfn = DSPCat(asmfb,".lnk");
    lstfn = DSPCat(asmfb,".lst");

/* TEMPORARY HACK. When there is time, do the true "make" thing of
   checking to see that the .lnk file date is later than the .asm
   file date before skipping assembly.
*/
    /* take original file if it exists */
    if (!access(lnkfn,R_OK)) {
        printf(" DSP linker output file:\t%s ** ALREADY EXISTS **\n",lnkfn);
        printf(" Skipping assembly of:\t%s\n",asmfn);
        return errorcode;
    }
    printf(" Assembling DSP program:\t%s\n",asmfn);
    printf(
" ======================== DSP ASSEMBLY LOG ====================\n");

    asmStr = "/usr/bin/asm56000 -b -l ";

    if (do_local)
      asmStr = DSPCat(asmStr,DSPCat("-I",DSPCat(local_dir," ")));
    else
      asmStr = DSPCat(asmStr,"-I/usr/lib/dsp/smsrc/ ");
    if (do_local_macro)
      asmStr = DSPCat(asmStr,DSPCat("-M",DSPCat(local_macro_dir," ")));

    asmStr = DSPCat(asmStr, asmfn);
    printf("%s\n",asmStr);
    errorcode=system(asmStr);

    printf(
" ==================== END DSP ASSEMBLER OUTPUT ================\n");
    if (errorcode) {
        printf("\n *** Error(s) assembling %s ***\n",asmfn);
        unlink(lnkfn);
    }
    else unlink(lstfn);
    return errorcode;
}

int linksysdoc(manpfn)
    char *manpfn;               /* filename of man page to be installed */
{
    char *manpfnnp;             /* man page filename, no path */
    char *sysmanpfn;            /* system man page filename */
    char *syscatpfn;            /* system man page filename, cat version */
    char *fnbody;               /* filename body, no extension or path */
    int ec;                     /* error code */
    fnbody = _DSPGetBody(manpfn);
    manpfnnp = DSPCat(fnbody,DSPCat(".",MANSECTION));
    sysmanpfn = DSPCat(MANDIR,manpfnnp);
    syscatpfn = DSPCat(CATDIR,manpfnnp);
    if((ec=unlink(sysmanpfn)) && ec!=ENOENT)
      perror(" Could not delete previous link to man page");
    if(symlink(manpfn,sysmanpfn)) {
        perror(" symlink failed on man page");
        printf(" Could not link manual page %s to directory %s\n",
               manpfn,sysmanpfn);
    }
    if((ec=unlink(syscatpfn)) && ec!=ENOENT)
      perror(" Could not delete previous link to man/cat page");
    if(symlink(manpfn,syscatpfn)) {
        perror(" symlink failed on man/cat page");
        printf(" Could not link manual page %s to %s\n",
               manpfn,syscatpfn);
    }
    return(ec);
}
