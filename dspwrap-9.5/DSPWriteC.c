/*	DSPWriteC.c - 
	write C function from DSPLoadSpec struct.
	Copyright 1988 NeXT, Inc.

Modification history:
	04/17/88/jos - created
	05/05/89/mtm - made it work for array processing
	06/15/89/mtm - changed function name prefix to DSPAP
	06/21/89/mtm - removed underbar from _DSPCat() and
		       DSPGetLastAddress() calls in interface function.
	06/29/89/mtm - modified for new stripped-down interface function.
	03/13/90/jos - changed "dsploadwrap" to "dspwrap" in emmitted comment.
	03/16/90/jos - added prefixAP support and procedure prototype decl.
	03/16/90/jos - added dspFileBody arg to address "FIXME"
	08/26/90/jos - added "stackable" argument.
	08/26/90/jos - removed underbar from _DSPCat() again!

*/

#include <strings.h>
#include <dsp/dsp.h>

/* private functions from libdsp */
extern char *_DSPRemoveTail();
extern int _DSPErr();

/************ ROUTINES IN COMMON WITH _DSPWriteUG.C (MERGE LATER) *************/
#define INT(_x) ((int)_x)
#define FOREACHLC(_i) for (_i = INT(DSP_LC_N); _i < INT(DSP_LC_Num); _i++) 
#define CONDITIONALCOMMA(_i) ((_i == INT(DSP_LC_Num)-1) ? "" : ",")

/* ... */

static int globalSymCount[DSP_LC_Num];

static char isLocalSymbol(symbolPtr)
DSPSymbol *symbolPtr;
{
    return symbolPtr->type[0] == 'L';
}

static int computeGlobalSymCount(symbolsPtr,nSymbols)
    DSPSymbol *symbolsPtr;
    int nSymbols;
{
    int i;
    int count = 0;
    if (!symbolsPtr || nSymbols == 0)
      return 0;
    for (i = 0; i < nSymbols; symbolsPtr++, i++) 
      if (symbolsPtr && !isLocalSymbol(symbolsPtr)) 
	count++;
    return count;
}	

/********** End routines in common with _DSPWriteUG.c (merge later) ***********/
int countGlobalSymbols(user) 
    DSPSection *user;
{
    int i,count=0;
    FOREACHLC(i) 
      globalSymCount[i] = 
	(count += computeGlobalSymCount(user->symbols[i],
					user->symCount[i]));
    return(count);
}

int DSPWriteC(
    DSPLoadSpec *dsp,
    char *cfn,			/* name of file open on cfp */
    FILE *cfp,			/* C file - assumed open for writing */
    FILE *hfp,			/* header file - assumed open for writing */
    int do_sys,			/* write out system segments */
    int do_boot,		/* boot DSP when executed */
    char *dspFileBody,		/* "foo" part of "foo.dsp" file name */
    char *prefixAP,		/* function name prefix (default = "DSPAP") */
    int stackable)		/* TRUE for "Load" else "LoadGo" */
{
    DSPSection *user;
    int ec,nargs;
    char *getenv();
    char *dspfile,*dspstruct;
    char *initloadgofunc,*cfb;

    /* File names */
    cfb = _DSPRemoveTail(cfn);	/* Default = cat("DSPAP",dspFileBody) */
    dspfile = DSPCat(dspFileBody,".dsp");
    dspstruct = DSPCat(cfb,"_struct");
    initloadgofunc = cfb;

    /*	
      The original plan called for writing out a single C function which
      can be compiled to prepare a loadable DSPLoadSpec image in memory.
      To this end, the call

	ec = DSPWriteInclude(dsp,cfp,do_sys,"dsp");

      is under development. 

      However, to save time for now, the DSPLoadSpec image is saved in
      a binary "image" file which gets loaded at run time by an init
      procedure.
    */

    /* Write the C function. */
       
    /* What it will look like (do_boot FALSE and prefixAP == "DSPAP" ): */

#if 0

/*
 * foo.c
 *   This AP interface function was auto-generated by dspwrap.
 */
#include <dsp/dsp.h>

extern int DSPAPLoad[Go]();

int foo(int foo_args)
{
    static DSPLoadSpec *dsp = NULL;
    int argValues[<foo_nargs>];

    argValues[0] = <foo_arg_1_name>;
    argValues[1] = <foo_arg_2_name>;
    ...
    argValues[<foo_nargs] = <foo_arg_nargs_name>;

    return DSPAPLoad[Go]("<foo_file_name>", &dsp, <foo_nargs>, argValues));
}

#endif

    /*** PRINT OUT THE ABOVE C FUNCTION HERE ***/
    
    fprintf(cfp,
            "/*\n"
	    " * %s.c\n"
            " *   This %sAP module interface function was auto-generated by dspwrap.\n"
            " */\n",
	    cfb,(stackable?"STACKABLE ":""));
    fprintf(cfp, "#include <dsp/dsp.h>\n\n");
    fprintf(cfp, "extern int %sLoad%s();\n\n",prefixAP,(stackable?"":"Go"));

    user = DSPGetUserSection(dsp);

    nargs = countGlobalSymbols(user);

    {
	int iarg,i,j,k;
	char *argstr;
	char *proto_argstr;
	char *foo_arg_names[nargs];
	DSPSymbol *sym;

	for (iarg=0,j=0;j<DSP_LC_NUM;j++) { /* loop through symbols */
	    for (k=0;k<user->symCount[j];k++) {
		sym = &user->symbols[j][k];
		if (*sym->type=='G') {
		    if (sym->value.i >= nargs)
			_DSPErr("DSPWriteC: bad symbol address");
		    else
			/* put argument names in address order */
			foo_arg_names[sym->value.i] = sym->name;
		    iarg++;
		}
	    }
	}
	if (iarg!=nargs)
	  _DSPErr("countGlobalSymbols disagrees with global symbol search loop");

	for (i=0,argstr=NULL,proto_argstr=NULL;i<nargs;i++) {
	  argstr = DSPCat(argstr,DSPCat(foo_arg_names[i],","));
	  
	  /* FIXME:  types should be fancy, not just int */
	  proto_argstr = DSPCat(proto_argstr,
	    DSPCat("int ",DSPCat(foo_arg_names[i],",")));
	}
	argstr[strlen(argstr)-1] = '\0'; /* Nevermind last comma */
	proto_argstr[strlen(proto_argstr)-1] = '\0'; /* Nevermind last comma */

	fprintf(cfp,"int %s(%s)\n",initloadgofunc,proto_argstr);
	fprintf(cfp,"{\n");
	fprintf(cfp,"    static DSPLoadSpec *dsp = NULL;\n");
	fprintf(cfp,"    int argValues[%d];\n\n",nargs);
	
	for (j=0;j<nargs;j++)
	  fprintf(cfp,"    argValues[%d] = %s;\n",j,foo_arg_names[j]);
	fprintf(cfp,"\n");
	fprintf(cfp,"    return %sLoad%s(\"%s\", &dsp, %d, argValues);\n", 
		prefixAP, (stackable?"":"Go"), dspfile, nargs);
	fprintf(cfp,"}\n");

	/* Write the header (.h) file that contains function prototype */
    
#if 0
    /* This prototype was auto-generated by dspwrap for foo */

    int foo(int foo_args);
#endif

	/*** GENERATE THE ABOVE HEADER FILE HERE ***/
	fprintf(hfp,
	    "/* This prototype was auto-generated by dspwrap for "
	    "%s */\n", cfb);
	fprintf(hfp, "int %s(%s);\n\n", initloadgofunc, proto_argstr);
    }	/* allocation block */
	
    return(0);
}
