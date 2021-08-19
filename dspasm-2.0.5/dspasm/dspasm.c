#include "dspdef.h"
#include "dspdcl.h"
#include <signal.h>
#include <time.h>
#if VAXC || LSC || MPW || AZTEC
#include <types.h>
#if MPW
#ifdef String
#undef String
#endif
#endif /* MPW */
#else
#include <sys/types.h>
#endif
#if VMS
#include <ssdef.h>
#include <climsgdef.h>
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		main -- DSP56000 Cross Assembler Main Program
*
**/
#if !LSC
main(argc,argv)		/* DSP56000 Cross Assembler Main Program */
#else
_main(argc,argv)	/* special entry point for Macintosh Lightspeed C */
#endif
int	argc;
char	*argv[];
{
	char *p, *q;
#if BSD || MPW
	int onintr (), onsegv ();
#else
	void onintr (), onsegv ();
#endif
	void sys_date(), sys_time();
	char *basename(), *getenv();
	time_t time ();

#if UNIX || VMS
	(void)signal (SIGSEGV, onsegv); /* set up for fatal signals */
	(void)signal (SIGBUS,  onsegv);
#endif
#if MPW || AZTEC
	(void)sigset (SIGINT, onintr);	/* set up for signals */
#else
	(void)signal (SIGINT, onintr);	/* set up for signals */
#endif
	p = basename (argv[0]);		/* get command base name */
	if ((q = strrchr (p, '.')) != NULL)
		*q = EOS;
#if VMS
	if ((q = strrchr (p, ';')) != NULL)
		*q = EOS;
	Dcl_flag = getenv (p) == NULL;
	Vmserr.code = INITOK;	/* initialize VMS condition code */
#endif
	/* scan for compiler flag on command line */
	Cflag = Cnlflag = get_cflag (argc-1, argv+1);

#if MSDOS
	if (_osmajor > 2)
#endif
		Progname = p;	/* establish program invocation name */

#if MSDOS
	if (!Cflag)		/* display banner */
		(void)fprintf (stderr, "%s  %s\n%s\n",
			Progtitle, Version, Copyright);
#endif

	if(!Dcl_flag && argc < 2)	/* not enough arguments */
		usage ();

#if !LSC && !MSC
	/* process any environment variables */
	if ((p = getenv (Ipenv)) != NULL)
		(void)do_incpath (p);	/* default include path */
	if ((p = getenv (Mlenv)) != NULL)
		(void)do_maclib (p, YES); /* default macro library */
#endif

	Lstfil = stdout;		/* set up default listing file */
	sys_date(Curdate);		/* set up current date */
	sys_time(Curtime);		/* set up current time */

	if (Cflag)
		asm_cfiles (argc, argv);/* assemble C-generated source */
	else
		asm_files (argc, argv); /* assemble normally */

#if LSC
	return (Err_count);
#else
#if !VMS
	exit(Err_count);
#else
	if (Err_count) {
		Vmserr.fld.success  = (unsigned)NO;
		Vmserr.fld.severity = VMS_ERROR;
		Vmserr.fld.message  = (unsigned)Err_count;
	}
	exit (Vmserr.code);
#endif
#endif
}

#if SASM
/**
*
* name		sasm -- DSP56000 Single-line Assembler Main Entry Point
*
**/
struct obj *
sasm (s)		/* DSP56000 Single-line Assembler Main Entry Point */
char *s;
{
	re_init();			/* reinitialize for Pass 2 */
	Lnkmode = Relmode = NO;		/* absolute mode */
	Glbsec.flags &= ~REL;
	Lstfil = stdout;		/* set up default listing file */
	while (*s && isspace (*s))
		s++;			/* skip leading white space */
	if (!*s || *s == COMM_CHAR){
		Sasmobj.count = 0;	/* empty line or comment line */
		if (!*s)
			Sasmobj.opcode = SASMEMP;
		else
			Sasmobj.opcode = SASMCOM;
		}
	else {
		Line[0] = ' ';		/* an instruction line */
		(void)strcpy (Line + 1, s);
		parse_line ();
		(void)process (NO, NO);
	}
	return (&Sasmobj);
}
#endif /* SASM */

/**
*
* name		usage - display usage
*
**/
static
usage ()
{
#if !SASM
#if !MSDOS
	(void)fprintf (stderr, "%s  %s\n%s\n", Progtitle, Version, Copyright);
#else
	if (Cflag)
		(void)fprintf (stderr, "%s  %s\n%s\n",
			Progtitle, Version, Copyright);
#endif
#if !PASM
	(void)fprintf (stderr, "Usage:  %s [-A] [-B[<lodfil>]] [-D<symbol> <string>] [-F<argfil>] [-I<ipath>] [-L[<lstfil>]] [-M<mpath>] [-O<opt>[,<opt>...]] [-V] <srcfil>...\n", Progname);
#else
	(void)fprintf (stderr, "Usage:  %s [-D<symbol> <string>] [-I<ipath>] [-L[<lstfil>]] [-M<mpath>] [-O<opt>[,<opt>...]] <srcfil>...\n", Progname);
#endif
	(void)fprintf (stderr, "where:\n");
#if !PASM
	(void)fprintf (stderr, "        lodfil - optional load file name\n");
#endif
	(void)fprintf (stderr, "        symbol - user-defined symbol\n");
	(void)fprintf (stderr, "        string - string associated with symbol\n");
	(void)fprintf (stderr, "        ipath  - include file directory path\n");
	(void)fprintf (stderr, "        argfil - command line argument file name\n");
	(void)fprintf (stderr, "        lstfil - optional list file name\n");
	(void)fprintf (stderr, "        mpath  - macro library directory path\n");
	(void)fprintf (stderr, "        opt    - assembler option\n");
	(void)fprintf (stderr, "        srcfil - assembler source file(s)\n");
#if LSC
	longjmp (Env, ERR);
#else
	exit (INITERR);
#endif
#endif	/* SASM */
}

/**
*
* name		asm_cfiles - assemble C-generated source
*
**/
static
asm_cfiles (argc, argv)
int argc;
char *argv[];
{
	char *ext;
	char fn[MAXBUF];
	char *strup (), *basename ();

	Comfp = argv+1;		/* set up command argument pointers */
	Comfiles = argc-1;
	if (Dcl_flag) {
		proc_dclo();	/* process VMS DCL command line options */
		if (dcl_getval (&Src_desc) == CLI_ABSENT)
			cmderr("Missing source filename");
	} else {
		proc_cmdo();	/* process command line options */
		if( Comfiles == 0 )	/* must have at least one filename! */
			cmderr("Missing source filename");
	}

	do {			/* loop to assemble C files */

		initialize ();		/* set up initial global values */
		Ilflag = Cnlflag;	/* inhibit listing by default */
		Pack_flag = NO;		/* do not pack strings DC directive */
		Xrf_flag = YES;		/* XDEFs global for all sections */
		Msw_flag = NO;		/* no incompatible space warnings */
		Rpflag = YES;		/* generate NOP register padding */
		cmd_opt ();		/* reset command options */

		if (Verbose)
			(void)fprintf (stderr, "%s: Beginning pass 1\n",
				Progname);

		if (!Dcl_flag)
			(void)strcpy(String,*Comfp++);
		fix_fn(".asm"); /* supply default extension if necessary */
		ext = strrchr (strcpy (fn, String), '.');
		if (Verbose)
			(void)fprintf (stderr, "%s: Opening source file %s\n",
				Progname, String);
		if((Fd = fopen(String,"r")) == NULL)
			cmderr2("Cannot open command line source file",
				String);
		++Cfn;			/* increment file number */
		Cfname = fn;		/* save pointer to filename */

		make_pass();		/* do pass 1 */
#if !PASM
		re_init();		/* reinitialize for Pass 2 */
		Ilflag = Cnlflag;	/* inhibit listing by default */
		Pack_flag = NO;		/* do not pack strings DC directive */
		Xrf_flag = YES;		/* XDEFs global for all sections */
		Msw_flag = NO;		/* no incompatible space warnings */
		Rpflag = YES;		/* generate NOP register padding */
		cmd_opt ();		/* reset command options */

		rewind (Fd);		/* rewind source file */
		if (!Cnlflag){		/* listing file */
			*ext = EOS;
			(void)strcpy (String, fn);
			*ext = '.';
			fix_fn(".lst");
			if((Lstfil = fopen(String,"w")) == NULL)
				cmderr2("Cannot open listing file",
					String);
			}

		*ext = EOS;
		(void)strcpy (String, fn);
		(void)strcpy (Mod_name, basename (String));
		(void)strup (Mod_name);
		*ext = '.';
		fix_fn(Lnkmode ? ".lnk" : ".lod");
		if((Objfil = fopen(String,"w")) == NULL)
			cmderr2("Cannot open object file", String);
		start_obj ();		/* initialize object file */

		/* wait to clear section counters until object file started */
		if (Lnkmode)
			zero_scntrs (); /* clear section counters */

		if (Verbose)
			(void)fprintf (stderr, "%s: Beginning pass 2\n",
				Progname);

		make_pass();		/* do pass 2 */
#endif /* PASM */
		cleanup(NO);		/* general cleanup */
		if (Verbose)
			(void)fprintf (stderr, "%s: Closing file %s\n",
				Progname, Cfname);
		(void)fclose (Fd);	/* close source file */

	} while ( Dcl_flag ? dcl_getval (&Src_desc) != CLI_ABSENT :
			   ++Ccomf <= Comfiles );
}

/**
*
* name		asm_files - assemble hand-generated source
*
**/
static
asm_files (argc, argv)
int argc;
char *argv[];
{
	char *ext;
	char *strup (), *basename ();

	initialize();			/* set up initial global values */

	Comfp = argv+1;			/* set up command argument pointers */
	Comfiles = argc-1;
	if (Dcl_flag) {
		proc_dclo();	/* process VMS DCL command line options */
		if (dcl_getval (&Src_desc) == CLI_ABSENT)
			cmderr("Missing source filename");
	} else {
		proc_cmdo();	/* process command line options */
		if( Comfiles == 0 )	/* must have at least one filename! */
			cmderr("Missing source filename");
	}

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning pass 1\n", Progname);

	if (!Dcl_flag)
		(void)strcpy(String,*Comfp);
	fix_fn(".asm"); /* add default extension */
	if (Verbose)
		(void)fprintf (stderr, "%s: Opening source file %s\n",
			Progname, String);
	if((Fd = fopen(String,"r")) == NULL)
		cmderr2("Cannot open command line source file", String);

	set_cfn(String);		/* save filename */

	make_pass();			/* do pass 1 */

#if !PASM
	re_init();			/* reinitialize for Pass 2 */
	Comfp = argv+1;
	Comfiles = argc-1;
	if (Dcl_flag) {			/* rescan command line */
		proc_dclo();	/* process VMS DCL command line options */
		(void)dcl_getval (&Src_desc);
	} else {
		proc_cmdo();	/* process command line options */
		(void)strcpy(String,*Comfp);
	}

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning pass 2\n", Progname);

	fix_fn(".asm"); /* add default extension */
	if (Verbose)
		(void)fprintf (stderr, "%s: Opening source file %s\n",
			Progname, String);
	if((Fd = fopen(String,"r")) == NULL)
		cmderr2("Cannot open command line source file", String);

	set_cfn(String);		/* save filename */

	if (Objfil) {
		ext = strrchr (String, '.');
		*ext = EOS;
		(void)strcpy (Mod_name, basename (String));
		(void)strup (Mod_name);
		*ext = '.';
		start_obj ();		/* initialize object file */
	}

	/* wait to clear section counters until object file started */
	if (Lnkmode)
		zero_scntrs ();		/* clear section counters */

	make_pass();			/* do pass 2 */

#endif /* PASM */

	cleanup(NO);			/* general cleanup */
}

/**
*
* name		proc_cmdo - process command line options
*
**/
static
proc_cmdo()
{
#if !SASM
	char *p;
	char *opt;
	int no_opt;
	struct macd *np;
	char *default_fn();

	for ( ; *Comfp && **Comfp == '-'; Comfp++, Comfiles-- ){
		opt = *Comfp; /* get ptr to option */
		++opt; /* skip switch character */

		switch( mapdn(*opt++) ){
#if !PASM
			case 'a':
				if( *opt )
					cmderr("Illegal command line -A option");
				Lnkmode = Relmode = NO;
				Glbsec.flags &= ~REL;
				break;
			case 'b':
				if (Cflag || Pass == 1)
					break;
				if( Objfil )	/* already exists */
					cmderr("Duplicate object file specified");
				if( *opt )
					no_opt = NO;
				else{
					no_opt = YES;
					opt = default_fn (Comfiles, Comfp);
					}
				(void)strcpy(String,opt);
				if (no_opt)	/* add default extension */
					fix_fn(Lnkmode ? ".lnk" : ".lod");
				if((Objfil = fopen(String,"w")) == NULL)
					cmderr2("Cannot open object file", String);
				break;
			case 'c':
				if( *opt )
					cmderr("Illegal command line -C option");
				break;		/* already have it */
#endif
			case 'd':
				if( !*opt )
					cmderr("Missing command line -D option");
				if( --Comfiles == 0 ) /* if no more args */
					cmderr("Command line missing -D definition string");
				p = *++Comfp; /* get the definition line ptr */
				Gagerrs = YES;
				if( do_define(opt,p,YES) == NO )
					cmderr("Illegal command line -D option");
				Gagerrs = NO;
				break;
			case 'f':
				if( !*opt )
					cmderr ("Missing command file name");
				fileargs (opt); /* get file arguments */
				continue;
			case 'i':
				if (Pass == 2)	/* skip on second pass */
					break;
				if( !*opt )
					cmderr("Missing command line -I option");
				Gagerrs = YES;
				if (do_incpath (opt) == NO)
					cmderr("Illegal command line -I option");
				Gagerrs = NO;
				break;
			case 'l':
				if (Cflag) {	/* turn listing back on */
					Cnlflag = Ilflag = NO;
					break;
				}
#if !PASM
				if (Pass == 1)	/* skip listing file on Pass 1 */
					break;
#endif
				if( Lstfil && Lstfil != stdout )
					cmderr("Duplicate listing file specified");
				if( *opt )
					no_opt = NO;
				else{
					no_opt = YES;
					opt = default_fn (Comfiles, Comfp);
					}
				(void)strcpy(String,opt); /* get filename */
				if (no_opt)
					fix_fn(".lst"); /* add default extension */
				if((Lstfil = fopen(String,"w")) == NULL)
					cmderr2("Cannot open listing file", String);
#if MSDOS
				adjbuf (Lstfil);
#endif
				break;
			case 'm':
				if (Pass == 2)	/* skip on second pass */
					break;
				if( !*opt )
					cmderr("Missing command line -M option");
				Gagerrs = YES;
				if (do_maclib (opt, YES) == NO)
					cmderr("Illegal command line -M option");
				Gagerrs = NO;
				break;
#if !PASM
			case 'o':
				if( !*opt )
					cmderr("Missing command line -O option");
				if (!Cflag) {
					Gagerrs = YES;
					if( do_option(opt) == NO )
						cmderr2("Illegal command line -O option", opt);
					Gagerrs = NO;
				} else {	/* hold for later */
					if ( !(np = (struct macd *)alloc(sizeof(struct macd))) ||
					     !(np->mline = (char *)alloc(strlen(opt)+1)) )
						cmderr("Out of memory - cannot save pathname");
					(void)strcpy(np->mline,opt);
					np->mnext = Oextend;
					Oextend = np;
				}
				break;
#endif
			case 't':
				if( *opt )
					cmderr("Illegal command line -T option");
				Testing = YES;
				break;
			case 'v':
				Verbose = YES;
				if( *opt )
					(void)sscanf(opt,"%d",&Vcount);
				break;
			default:
				cmderr2("Illegal command line option",
					opt-2);
				break;
			}
		}
#endif /* SASM */
}

/**
*
* name		proc_dclo - process VMS DCL command line options
*
**/
static
proc_dclo()
{
#if !SASM
#if VMS
	char *p;
	struct macd *np;
	struct dumname *op;
	int lflag = NO;
	char *basename ();

	/* reset command line */
	if (cli$dcl_parse(NULL, NULL) != CLI$_NORMAL)
		cmderr ("Cannot parse command line");
#if !PASM
	/* ABSOLUTE option */
	if (cli$present (&Abs_desc) == CLI$_PRESENT) {
		Lnkmode = Relmode = NO;
		Glbsec.flags &= ~REL;
	}
#endif
	/* DEFINE option */
	while (dcl_getval (&Def_desc) != CLI_ABSENT) {
		if ((p = strchr (String, '=')) == NULL)
			cmderr ("Illegal command line DEFINE option");
		*p++ = EOS;
		Gagerrs = YES;
		if (do_define (String, p, YES) == NO)
			cmderr ("Illegal command line DEFINE option");
		Gagerrs = NO;
	}
	/* INCLUDE option */
	if (Pass == 1) {	/* only process on first pass */
		while (dcl_getval (&Inc_desc) != CLI_ABSENT) {
			Gagerrs = YES;
			if (do_incpath (String) == NO)
				cmderr("Illegal command line INCLUDE option");
			Gagerrs = NO;
		}
	}
	/* LISTING option */
#if !PASM
	if (!Cflag && Pass == 2	 && cli$present (&Lst_desc) == CLI$_PRESENT) {
#else
	if (cli$present (&Lst_desc) == CLI$_PRESENT) {
#endif
		if (dcl_getval (&Lst_desc) == CLI_ABSENT) {
			if (dcl_getval (&Src_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL)
				cmderr ("Cannot parse command line");
			(void)strcpy (String, basename (String));
			if ((p = strrchr (String, '.')) != NULL)
				*p = EOS;	/* strip off extension */
			fix_fn(".lst"); /* add default extension */
		}
		if ((Lstfil = fopen(String,"w")) == NULL)
			cmderr2("Cannot open listing file", String);
	} else if (Cflag)	/* turn listing back on */
		Cnlflag = Ilflag = NO;
	/* MACLIB option */
	if (Pass == 1) {	/* only process on first pass */
		while (dcl_getval (&Mac_desc) != CLI_ABSENT) {
			Gagerrs = YES;
			if (do_maclib (String, YES) == NO)
				cmderr("Illegal command line MACLIB option");
			Gagerrs = NO;
		}
	}
#if !PASM
	/* OBJECT option */
	if (!Cflag && Pass == 2	 && cli$present (&Obj_desc) == CLI$_PRESENT) {
		if (dcl_getval (&Obj_desc) == CLI_ABSENT) {
			if (dcl_getval (&Src_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL)
				cmderr ("Cannot parse command line");
			(void)strcpy (String, basename (String));
			if ((p = strrchr (String, '.')) != NULL)
				*p = EOS;	/* strip off extension */
			/* add default extension */
			fix_fn(Lnkmode ? ".lnk" : ".lod");
		}
		if ((Objfil = fopen(String,"w")) == NULL)
			cmderr2("Cannot open object file: %s", String);
	}
#endif
	/* OPTION option */
	while (dcl_getval (&Opt_desc) != CLI_ABSENT) {
		Gagerrs = YES;	/* stop error output */
		if (do_option(String) == NO)
			cmderr2("Illegal command line OPTION option",
				String);
		Gagerrs = NO; /* release error output */
		if (!Cflag) {
			Gagerrs = YES;
			if( do_option(String) == NO )
				cmderr2("Illegal command line OPTION option: %s", String);
			Gagerrs = NO;
		} else {	/* hold for later */
			if ( !(np = (struct macd *)alloc(sizeof(struct macd))) ||
			     !(np->mline = (char *)alloc(strlen(String)+1)) )
				cmderr("Out of memory - cannot save pathname");
			(void)strcpy(np->mline,String);
			np->mnext = Oextend;
			Oextend = np;
		}
	}
	/* TEST option */
	if (cli$present (&Tst_desc) == CLI$_PRESENT)
		Testing = YES;
	/* VERBOSE option */
	if (cli$present (&Vbs_desc) == CLI$_PRESENT) {
		Verbose = YES;
		if (dcl_getval (&Vbs_desc) == CLI_ABSENT) {
			if (dcl_getval (&Src_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL)
				cmderr ("Cannot parse command line");
		} else
			(void)sscanf (String, "%d", &Vcount);
	}
	if (Cflag)		/* compiler flag set */
		if (lflag)
			Cnlflag = NO;	/* explicit listing */
		else
			Ilflag = YES;	/* inhibit listing */
#endif /* VMS */
#endif /* SASM */
}

/**
*
* name		dcl_getval - get DCL command line value
*
* synopsis	status = dcl_getval (opt)
*		int status;	return status from cli$get_value
*		struct dsc$descriptor_s *opt;	pointer to command line option
*
* description	Calls the VMS DCL routine cli$get_value to return values
*		for the command line option referenced by opt.	The values
*		are returned in the global String array.  The length returned
*		from cli$get_value is used to terminate the string.
*
**/
static
dcl_getval (opt)
#if VMS
struct dsc$descriptor_s *opt;
#else
int *opt;
#endif
{
#if !SASM
#if VMS
	unsigned status, len = 0;

	status = cli$get_value (opt, &Arg_desc, &len);
	if (status != CLI_ABSENT)
		String[len] = EOS;
	return (status);
#else
	return (*opt);
#endif
#endif /* SASM */
}

/**
*
* name		cmd_opt --- process command line -O options
*
* synopsis	cmd_opt ()
*
* description	Processes the options in the command line -O linked list.
*
**/
static
cmd_opt ()
{
	struct macd *np;

	Gagerrs = YES;
	for (np = Oextend; np; np = np->mnext)
		if( do_option(np->mline) == NO ) {
			(void)printf("%s: Illegal command line %s option: %s\n", Progname, Dcl_flag ? "OPTION" : "-O", np->mline);
#if LSC
			longjmp (Env, ERR);
#else
			exit(INITERR);
#endif
		}
	Gagerrs = NO;
}

/**
*
* name		default_fn - get default filename
*
* synopsis	fn = default_fn (argc, argv)
*		char *fn;	pointer to default filename
*		int argc;	count of command line arguments
*		char **argv;	pointer to command line arguments
*
* description	Takes a pointer to and count of the command line arguments
*		and returns a pointer to the first filename
*		after the option list.	If a filename argument cannot
*		be found, it reports a fatal error.
*
**/
static char *
default_fn (argc, argv)
int argc;
char **argv;
{
#if !SASM
	static char *p = NULL;
	char *q, c;
	char *basename ();

	if (p)			/* already found filename */
		return (p);

	while (argc > 0 && **argv == '-') {	/* find first filename */
		c = *(*argv + 1);		/* save switch character */
		--argc;
		++argv;
		switch (mapdn (c)) {		/* skip extra fields */
			case 'd':
				--argc;
				++argv;
		}
	}

	if (argc <= 0)
		cmderr("Cannot open command line source file");

	q = basename (*argv);
	if ((p = (char *)alloc (strlen (q) + 1)) == NULL)
		fatal ("Out of memory - cannot save filename");

	(void)strcpy (p, q);	/* copy the name */
	if ((q = strrchr (p, '.')) != NULL && (q != p && q != p + 1))
		*q = EOS;	/* strip off extension */
	return (p);
#endif /* SASM */
}

/**
*
* name		fix_fn - fix filename extension
*
* synopsis	fix_fn(ext)
*		char *ext;	default extension
*
* description	Examines the filename in String. If it has an extension then
*		the filename is saved unchanged. If it has no
*		extension, the default one is added.
*
**/
fix_fn(ext)
char *ext;
{
#if !SASM
	int extlen;
	char *p,*np,*ep,*basename();

	extlen = strlen (ext);		/* get extension length */
	p = basename (String);		/* get file base name */
	np = String + strlen (String);	/* compute end of string */
	ep = strrchr (String, '.');	/* find file extension */

	if( !ep || ep < p ){		/* no extension supplied */
		(void)strcpy(np,ext);
		ep = np;
		np += extlen;
		}

	if (np - p > BASENAMLEN) {	/* filename too long -- truncate */
		np = p + (BASENAMLEN - extlen);
		p = np;
		while (*ep && np < p + MAXEXTLEN + 1)
			*np++ = *ep++;
		*np = EOS;
	}
#endif /* SASM */
}

/**
*
* name		fix_path - fix pathname
*
* description	Examines the path in String, and appends an appropriate
*		delimiter character if required.
*
**/
fix_path ()
{
#if !SASM
	register i;
	register char *p;

	if ((i = strlen (String)) <= 0)
		return;

	p = &String[i-1];
#if MSDOS
	if (*p != '\\' && *p != ':')
		*++p = '\\';
#endif
#if VMS
	if (*p != ']' && *p != ':')
		*++p = ':';
#endif
#if UNIX
	if (*p != '/')
		*++p = '/';
#endif
#if MAC
	if (*p != ':')
		*++p = ':';
#endif
	*++p = EOS;
#endif /* SASM */
}

/**
*
* name		set_cfn --- set up current filename pointer
*
* synopsis	yn = set_cfn(str)
*		int yn;		YES/NO
*		char *str;	file name
*
* description	If Pass 1, the filename is saved in a linked list of
*		fn structures. If Pass 2, the filename is looked
*		up in the list. On either pass, the global variable
*		Cfname is initialized to point to the filename.
*		Fatal errors will occur if there is insufficient
*		memory to save the filename or if the filename was
*		not encountered on Pass 1.
*
**/
set_cfn(str)
char *str;
{
#if !SASM
	struct fn *np;
	struct fn *ep;

	if (Testing)	/* map to lower case if testing */
		(void)strdn (str);
	if( Pass == 1 ){
		if (!(np = (struct fn *)alloc(sizeof (struct fn))))
			fatal("Out of memory - cannot save filename");
		if (!(np->fnam = (char *)alloc(strlen(str)+1)))
			fatal("Out of memory - cannot save filename");
		(void)strcpy(np->fnam,str);
		Cfname = np->fnam;
		np->fnnext = NULL;
		if( F_names ){
			ep = F_names;
			while( ep->fnnext ) /* find end of list */
				ep = ep->fnnext;
			ep->fnnext = np; /* put filename at end */
			}
		else
			F_names = np;	/* first time to save filename */
		}
	else{
		np = F_names;	/* point to start of filename list */
		while( !STREQ(str,np->fnam) )
			if( !(np = np->fnnext) )
				fatal("File not encountered on pass 1");
		Cfname = np->fnam;
		}
#endif /* SASM */
}

/**
*
* name		fileargs - collect arguments from command file
*
* synopsis	fileargs (fn)
*		char *fn;	pointer to command file name
*
* description	Open command file and read assembler command arguments.
*		Allocate new argument vector and fill in pointers to names.
*		On pass 2, simply set up global vector pointer and
*		count, then return.
*
**/
static
fileargs (fn)
char *fn;
{
	static argc = 0;
	static char **argv = NULL;
	FILE *fp;
	int c;
	char buf[MAXBUF];
	register char *p;
	register i;
	struct macd sl, *sp = &sl, *op;

	if (argv && Pass == 2) {	/* already have args */
		Comfp = argv;
		Comfiles = argc;
		return;
	}

	if ((fp = fopen (fn, "r")) == NULL)
		cmderr2 ("Cannot open command file: %s", fn);

	sl.mline = NULL;
	while (!feof (fp)) {		/* read in args */

/*		while ((c = fgetc (fp)) != EOF && !isgraph (c))		*/
		while ((c = fgetc (fp)) != EOF && !(isprint (c) && c != ' '))
			;
		if (c == COMM_CHAR) {	/* comment; read until end of line */
			while ((c = fgetc (fp)) != EOF && c != NEWLINE)
				;
			continue;
		}
		buf[0] = c;
		for (p = buf + 1, i = 1;
/*		     (c = fgetc (fp)) != EOF && isgraph (c);		*/
		     (c = fgetc (fp)) != EOF && (isprint (c) && c != ' ');
		     p++, i++)
			*p = c;
		*p = EOS;

		if (!feof (fp)) {	/* allocate string list structure */
			if ((op = (struct macd *)alloc (sizeof (struct macd))) == NULL || (op->mline = (char *)alloc (i + 1)) == NULL)
				fatal ("Out of memory - cannot save command file arguments");
			(void)strcpy (op->mline, buf);
			op->mnext = NULL;
			sp = sp->mnext = op;
			argc++;
		}
	}

	(void)fclose (fp);
/*
	allocate two extra vectors, one for the start and one for end.
	That way the command processing loop can skip the first one and
	the last entry can be NULL.
*/
	argc++;		/* count beginning dummy argument */
	if ((argv = (char **)alloc (sizeof (char *) * (argc + 1))) == NULL)
		fatal ("Out of memory - cannot allocate argument vector");

	sp = sl.mnext;
	*argv = Nullstr;		/* empty string */
	for (i = 1; i < argc; i++) {	/* populate the argument vector */
		*(argv + i) = sp->mline;
		op = sp;
		sp = sp->mnext;
		free ((char *)op);
	}
	*(argv + i) = NULL;

	Comfp = argv;
	Comfiles = argc;
}

/**
*
* name		get_cflag --- scan command line for compiler flag
*
* synopsis	yn = get_cflag (argc, argv)
*		int yn;		YES/NO compiler flag found
*		int argc;	count of command line arguments
*		char **argv;	pointer to command line arguments
*
* description	Takes a pointer to and count of the command line arguments.
*		Scans command line looking for compiler flag.  Returns
*		YES if found, NO otherwise.
*
**/
static
get_cflag (argc, argv)
int argc;
char **argv;
{
#if !SASM
	if (Dcl_flag) {
		if (dcl_getval (&Cc_desc) != CLI_ABSENT)
			return (YES);
	} else {
		while (argc > 0 && **argv == '-') {	/* scan args */
			if (mapdn (*(*argv + 1)) == 'c')
				return (YES);
			--argc;
			++argv;
		}
	}
	return (NO);
#endif /* SASM */
}

/**
*
* name		initialize --- init for pass 1
*
* description	Initializes global variables.  Initializes forward
*		reference file.
*
**/
static
initialize()
{
#if !SASM
#if DEBUG
	printf("Initializing\n");
#endif
	Pass	  = 1;
	Real_ln = Lst_lno = 0;
	Err_count = Warn_count = 0;
	Lkp_count = 0L;
	Lflag	= 1;
	Ctotal	= 0;
	Page	= 1;
	No_defs = 0;
	End_addr = -1L;
	if (Cflag) {
		Strtlst	  = YES;	/* start listing */
	} else {
		Cfn	  = 1;		/* reset file counts */
		Ccomf	  = 1;
	}

	Relmode = Lnkmode;
	Glbsec.flags |= Relmode ? REL : 0;
	Glbsec.rflags = Glbsec.lflags = PSPACE;
	zero_gcntrs ();		/* clear global absolute counters */
	if (!Lnkmode) {		/* absolute mode */
		Int_flag = YES; /* do vector checking by default */
		Pc = Lpc = &Gcntrs[PMEM-1][DCNTR][RBANK];
	} else {
		Int_flag = NO;	/* no vector checking */
		zero_scntrs (); /* clear section counters */
		Pc = Lpc = &Glbsec.cntrs[PMEM-1][DCNTR][RBANK];
	}
	Old_pc = Old_lpc = 0L;
	Cspace = Loadsp = PSPACE;
	Rcntr = Lcntr = DEFAULT;
	Rmap = Lmap = NONE;
	Regref = Regmov = 0L;
	Curr_lcl = Loclab;
	Exp_lcl = Curr_explcl = Last_explcl = NULL;
	P_total = 0;
	Radix = 10;
	Seed = 0L;

#if !PASM
	Prtc_flag = YES;
	Pmacd_flag = YES;
	Mex_flag = NO;
#else
	Prtc_flag = NO;
	Pmacd_flag = NO;
	Mex_flag = YES;
#endif
	Msw_flag = Cflag ? NO : YES;

	Cm_flag =
	Mc_flag =
	Wrn_flag =
	Lwflag = YES;

	Fcflag =
	Unasm_flag =
	Cyc_flag =
	Cex_flag =
	Mex_flag =
	Rcom_flag =
	Int_flag =
	Xrf_flag =
	Emit =
	Local =
	Icflag =
	Lbflag =
	Dex_flag =
	Rpflag =
	End_flag = NO;

	fwdinit();	/* forward ref init */
#endif /* SASM */
}

/**
* name		re_init --- initialize for pass 2
*
* description	Initializes global variables for pass 2.
*		Initializes the forward reference file for pass 2.
*
**/
static
re_init()
{
#if !PASM
#if DEBUG
	printf("Reinitializing\n");
#endif
	if (Cflag) {
		Strtlst	  = YES;	/* start listing */
	} else {
		Cfn	  = 1;		/* reset file counts */
		Ccomf	  = 1;
	}
	Real_ln = Lst_lno = 0;
	Err_count = Warn_count= 0;
	Lkp_count = 0L;
	Relmode = (Glbsec.flags & REL) != 0;
	Glbsec.rflags = Glbsec.lflags = PSPACE;
	zero_gcntrs();	/* zero absolute counters */
	if (!Lnkmode) { /* absolute mode */
		Int_flag = YES;
		Pc = Lpc = &Gcntrs[PMEM-1][DCNTR][RBANK];
	} else {
		Int_flag = NO;
		Pc = Lpc = &Glbsec.cntrs[PMEM-1][DCNTR][RBANK];
	}
	Old_pc = Old_lpc = 0L;
	Cspace = Loadsp = PSPACE;
	Rcntr = Lcntr = DEFAULT;
	Rmap = Lmap = NONE;
	Regref = Regmov = 0L;
	P_total = 0;
	Radix = 10;
	Seed = 0L;

	Msw_flag = Cflag ? NO : YES;

	Prtc_flag =
	Cm_flag =
	Pmacd_flag =
	Mc_flag =
	Wrn_flag =
	Lwflag = YES;

	Fcflag =
	Unasm_flag =
	Cyc_flag =
	Cex_flag =
	Mex_flag =
	Rcom_flag =
	Int_flag =
	Xrf_flag =
	Emit =
	Local =
	Icflag =
	Lbflag =
	Dex_flag =
	Rpflag =
	End_flag = NO;

	Lflag	= 1;
	Ctotal	= 0;
	Page	= 1;
	No_defs = 0;
	Curr_lcl = Loclab;
	Curr_explcl = Exp_lcl;
	Last_explcl = NULL;	/* used as flag to start second pass */
#if !SASM
	fwdreinit();
	free_def();		/* free DEFINE list */
	free_mac();		/* free macro definitions */
	free_mc();		/* free MACLIB check list */
	clr_rdirect();
	clr_scslbl();
	while( Csect != &Glbsec )	/* clean up any left over sections */
		(void)do_endsec();
	free_xrd();		/* release xref and xdef lists */
#endif
	Pass	= 2;		/* wait to set Pass until reinitialized */
#endif
}

/**
* name		cleanup --- cleanup after assembly
*
* synopsis	cleanup(err)
*		int err;	indicates whether cleanup due to error
*
* description	Perform general cleanup after assembly.	 Clear dangling
*		sections.  Free various lists.	Finish up object and listing
*		files.
*
**/
cleanup (err)
int err;
{
#if !SASM
#if DEBUG
	printf("Cleaning up\n");
#endif
#if !PASM
	if( Csect != &Glbsec ){		/* remove dangling sections */
		if (!err)
			error("Unexpected end of file - missing ENDSEC directive");
		while( Csect != &Glbsec )
			(void)do_endsec();
		}
	free_xrd();		/* release storage for xref and xdef lists */
#endif
	if (!Cflag)
		free_al(Iextend);	/* release include directory paths */
	free_ml();			/* release MACLIB directory paths */
	free_mc();			/* release MACLIB check list */
	Iextend = NULL;
	Mextend = NULL;
	Mlib = NULL;
#if !PASM
	if (Objfil){
		if (!err)
			done_obj();	/* finish up object file */
		(void)fclose (Objfil);
		}
	/* clear labels here so errors will make it to the listing */
	clr_scslbl();			/* clear any stacked SCS labels */
	if (!err)
		done_lst();		/* finish up listing */
#endif
	if (Lstfil && Lstfil != stdout)
		(void)fclose (Lstfil);
#if !LSC
	if (Cflag) {
#endif
		free_lcl();	/* release local label lists */
		free_def();	/* free DEFINE list */
		free_mac();	/* free macro definitions */
		free_sym();	/* free symbol table entries */
		free_sec();	/* free sections */
		free_xrf();	/* free external reference list */
		if (Sym_sort)	/* free any remaining sort tables */
			free ((char *)Sym_sort);
		free_symobj();	/* free SYMOBJ list */
#if !LSC
	}
#endif
#endif	/* SASM */
}

/**
*
* name		zero_gcntrs - zero global location counters
*
**/
static
zero_gcntrs()
{
	register i, j;

	for (i = 0; i < MSPACES; i++)
		for (j = 0; j < MCNTRS; j++)
			Gcntrs[i][j][RBANK] = Gcntrs[i][j][LBANK] = 0L;
}

/**
*
* name		zero_scntrs - zero section location counters
*
**/
static
zero_scntrs()
{
	struct slist *sp;
	register i, j;

	for (sp = Sect_lst; sp; sp = sp->next) {

		for (i = 0; i < MSPACES; i++)
			for (j = 0; j < MCNTRS; j++)
				sp->cntrs[i][j][RBANK] = 0L;
	}
}

/**
* name		make_pass --- perform pass on source code
*
* description	Reads in each line of source, and if not a
*		comment line, processes it. At the end of the current
*		file, checks for any stacked files (for include
*		file processing).
*
**/
static
make_pass()
{
#if !SASM
#if DEBUG
	printf("Pass %d\n",Pass);
#endif
	while( getaline() ){
		reset_pflags(); /* reset force print flags */
		if(parse_line()){
			(void)process(NO,NO);
			print_line(' ');
			}
		else
			print_comm(' ');
		P_total = 0;	/* reset byte count */
		Cycles = 0;	/* and per instruction cycle count */
		}
#endif /* SASM */
}

/**
* name		unstack_f --- restore file (if any) from the include stack
*
* synopsis	yn = unstack_f()
*		int yn;		YES/NO
*
* description	Checks to see if there are any files in the include
*		file stack, and if there are, unstacks the filename,
*		filedescriptor, filenumber, and line number and then
*		frees the stack space.
**/
static
unstack_f()
{
#if !SASM
	struct filelist *np;

	if( F_stack == NULL )
		return(NO);		/* no files stacked */
	Cfname = F_stack->ifname;	/* restore filename */
	Fd = F_stack->ifd;		/* restore file descriptor */
	Cfn = F_stack->fnum;		/* restore filenumber */
	Real_ln = F_stack->iln;		/* restore line number */
	if (F_stack->prevline) {	/* MACLIB file? restore prev. line */
		if (!Imode || Imode->mode == FILEMODE) { /* just copy line */
			(void)strcpy (Line, F_stack->prevline);
			Mac_eof = YES;	/* set MACLIB EOF flag */
		} else {		/* restore from macro expansion */
			Macro_exp->defline = Macro_exp->prevline;
			Macro_exp->lc = Macro_exp->defline->mline;
		}
		free (F_stack->prevline);	/* free storage */
	}
	np = F_stack;
	F_stack = np->inext;
	free((char *)np);			/* release block */
	if (pop_imode () != FILEMODE)	/* restore input mode */
		fatal ("Input mode stack out of sequence");
	return(YES);
#endif /* SASM */
}

/**
* name		getaline - get next source input line
*
* synopsis	yn = getaline()
*		int yn;		YES / NO if end of source
*
* description	Reads in source lines from command line files, include
*		files, and macro expansions. Returns NO when end of source
*		(macro or file) is encountered.
*
**/
getaline()
{
#if !SASM
	if( End_flag ) /* encountered end directive */
		return(NO);

	if( Imode && Imode->mode == MACXMODE ) /* macro expansion active */
		if( Macro_exp && !Macro_exp->defline )	/* end of expansion */
			return(NO);

	if( ++Lst_lno <= 0 )	/* increment listing line number */
		fatal("Too many lines in source file");

	if( readaline() == YES ){
		if( Macro_exp == NULL ) /* if this is not a macro expansion */
			Line_num = ++Real_ln;	/* set starting line number */
		return(YES);
		}

	while( unstack_f() == YES )	/* loop in case EOF but more files */
		if( readaline() == YES ){
			if( Macro_exp == NULL ) /* if this is not a macro expansion */
				Line_num = ++Real_ln;	/* set starting line number */
			return(YES);
			}

	if (Cflag)	/* end of the line for C files */
		return (NO);

	while ( Dcl_flag ? dcl_getval (&Src_desc) != CLI_ABSENT : ++Ccomf <= Comfiles ){
		if (!Dcl_flag)
			(void)strcpy(String,*++Comfp);
		fix_fn(".asm"); /* supply default extension if necessary */
		if (Verbose)
			(void)fprintf (stderr, "%s: Opening source file %s\n",
				Progname, String);
		if((Fd = fopen(String,"r")) == NULL)
			cmderr2("Cannot open command line source file",
				String);
		++Cfn;			/* increment file number */
		set_cfn(String);	/* save filename */
		Real_ln = 0;		/* reset line number */
		if( readaline() == YES ){ /* if file is not empty */
			if( Macro_exp == NULL ) /* if this is not a macro expansion */
				Line_num = ++Real_ln;	/* set starting line number */
			return(YES);
			}
		}
	return(NO);	/* must be end of source */
#endif /* SASM */
}

/**
* name		readaline --- collect an input line.
*
* synopsis	yn = readaline()
*		int yn;		YES if line / NO if end of file
*
* description	Collects an input line from the source file, including
*		any possible continuation lines. Any define directive
*		translations are applied to the line. Tabs are expanded
*		to spaces, and the input line is terminated with a null.
*		Lines that only contain spaces are converted to null lines.
*		If the end of file is encountered, the file is closed,
*		and the local label list is flushed.
*
**/
static
readaline()
{
#if !SASM
	register char *p = Line;
	int	c;
	char	ch;
	char	*do_def();
	char	*symstrt;	/* start of define symbol */
	int	col = 0;	/* current source column */
	int	strflag = NO;	/* indicates string processing in progress */
	int	cnt = 0;	/* count of symbol tokens on line */
	int	i;
	long	ftell ();

	if (Mac_eof) {		/* end of MACLIB file; already have line */
		Mac_eof = NO;	/* reset flag */
		return (YES);
	}

	symstrt = NULL;
	while( (c = next_char()) != EOF ){

		ch = (char)c & 0x7f; /* make sure bit 7 is 0 */
		++col;

		if( !strflag )
			if( !ALPHAN(ch) ){	/* apply define translations */
				if( symstrt != NULL ){
					if (Def_flag)
						p = do_def(symstrt,p,++cnt);
					symstrt = NULL;
					}
				}
			else
				if( symstrt == NULL && isalpha(ch) )
					symstrt = p;

		if( ch == NEWLINE ){	/* end of line */
			*p = EOS;
			if( p == Line )
				return(YES);	/* just an empty line */
			else if( *(p-1) == CONTCH ){ /* continuation line */
				if( Macro_exp == NULL ) /* if this is not a macro expansion */
					++Real_ln;	/* inc line number */
				--p;		/* ignore continuation chars */
				col = 0;	/* reset column number */
				}
			else{
				return(chk_bline());
				}
			}
		else if( ch == TAB ){	/* tab */
			for( i = 8 - ((col - 1 ) % 8) ; i > 0 ; --i ){
				++col;
				if( p - Line < MAXBUF )
					*p++ = ' ';
				else{
					error("Line too long");
					*p = EOS;
					while( (c = fgetc(Fd)) != EOF &&
						c != NEWLINE )
						; /* loop until end of line */
					break;
					}
				}
			}
		else{
			if( p - Line < MAXBUF ){
				if( ch == STR_DELIM && !Dex_flag )
					strflag = !strflag;
				*p++ = ch;
				}
			else{
				error("Line too long");
				*p = EOS;
				while( (c = fgetc(Fd)) != EOF && c != NEWLINE )
					;	/* loop until end of line */
				break;
				}
			}
		}

	if( p != Line ){	/* if last line was not terminated by cr */
		if( symstrt != NULL && Def_flag)
			p = do_def(symstrt,p,++cnt);
		*p = EOS;
		return(chk_bline());
		}

	if (!Cflag) {
		if (Verbose)
			(void)fprintf (stderr, "%s: Closing file %s\n",
				Progname, Cfname);
		(void)fclose(Fd);	/* end of file */
	}
	return(NO);
#endif /* SASM */
}

/**
*
* name		next_char - get next input stream character
*
* synopsis	ch = next_char()
*		int ch;		next input stream character (EOF if end
*				of input file)
*
* description	Returns next input stream character. If a macro expansion
*		is active, returns the next macro expansion character.
*		If no macro expansion is active,
*		returns the next character from the current input file.
*
**/
static
next_char()
{
#if !SASM
	static char numarg[16]; /* buffer for value type arguments */
	int i;
	int nextch;
	int argnum, argtyp;
	struct macd *oa;
	struct evres *er,*eval_int();

	if( !Imode || Imode->mode == FILEMODE ) /* no macro exp. active */
		return(fgetc(Fd));

	/* macro expansion still active; loop until valid character found */

	for( ;; ){
		if( Macro_exp->curarg ){ /* currently expanding argument */
			nextch = *Macro_exp->curarg++;
			if( nextch )
				return(nextch); /* if not end of expanded argument */
			else
				Macro_exp->curarg = NULL; /* end of argument. Kill off argument expansion */
			}
		nextch = *Macro_exp->lc++;
		if( (nextch & MACARG) == 0 ) /* not an argument */
			if( nextch )
				return(nextch);
			else { /* end of line. set up next */
				Macro_exp->prevline = Macro_exp->defline;
				/* reset next definition line ptr */
				Macro_exp->defline = Macro_exp->defline->mnext;
				if (Macro_exp->defline)
					Macro_exp->lc = Macro_exp->defline->mline;
				return(NEWLINE); /* return fake newline */
				}

		/* must be a macro argument */
		argnum = nextch & 0x7F; /* find argument number */
		if( argnum == 0 )
			continue; /* skip trailing concatenation operator */
		/* if dummy arg number exceeds expansion args then skip */
		if( argnum > Macro_exp->no_args ){
			Macro_exp->lc += 2;
			continue;
		}
		/* look up expansion argument */
		oa = Macro_exp->args;
		for( i = 1 ; i != argnum ; ++i )
			oa = oa->mnext;
		argtyp = (*Macro_exp->lc++) & 0xFF;
		if( (argtyp & STRTYP) == STRTYP ) /* string arg */
			Macro_exp->curarg = oa->mline;
		else{ /* value type of argument */
			Optr = oa->mline; /* evaluate the argument */
			er = eval_int();
			if( !er ){
				error("Macro value substitution failed");
				*numarg = EOS; /* put in dummy null string */
				}
			else{
				(void)sprintf(numarg,argtyp == VALTYP ?
					"%ld" : "%lx",er->uval.xvalue.low);
				Macro_exp->curarg = numarg;
				}
			free_exp(er);
			}
		}
#endif /* SASM */
}

/**
*
* name		push_imode - push new input mode on mode stack
*
* synopsis	push_imode (mode)
*		int mode;	new input mode
*
* description	Puts a new input mode onto the input mode stack.
*
**/
push_imode (mode)
int mode;
{
	struct inmode *ip;

	if (!(ip = (struct inmode *)alloc (sizeof (struct inmode))))
		fatal ("Out of memory - cannot push input mode stack");
	ip->mode = mode;
	ip->next = Imode;
	Imode = ip;
}

/**
*
* name		pop_imode - pop input mode off mode stack
*
* synopsis	mode = pop_imode ()
*		int mode;	mode popped from stack
*
* description	Pops top mode from input mode stack and returns it.
*		Returns zero if stack empty.
*
**/
pop_imode ()
{
	int mode;
	struct inmode *ip;

	if (!Imode)
		return (0);
	mode = Imode->mode;	/* save mode */
	ip = Imode;
	Imode = ip->next;
	free ((char *)ip);	/* free space */
	return (mode);
}

/**
*
* name		chk_bline - check for blank line
*
* description	Checks the input Line for all blanks. If no non-blank
*		characters are found, then set *Line = EOS. Always returns
*		YES.
*
**/
static
chk_bline()
{
	char *p = Line;

	for( ; *p ; ++p )
		if( !isspace(*p) )
			return(YES); /* good line */
	*Line = EOS;	/* blank line */
	return(YES);
}

/**
*
* name		parse_line --- split input line into fields.
*
* synopsis	n = parse_line()
*		int n;		0 if empty or comment line, 1 otherwise
*
* description	Splits input line into label, op, opxy, operand, xmove,
*		ymove, and comment fields. Each field is left as
*		a null terminated string.
*
**/
parse_line()
{
	register char *ptrfrm = Line;
	register char *ptrto = Pline;
	char	*endptr;
	char	*end_field();

	if( *ptrfrm == COMM_CHAR || *ptrfrm == EOS )
		return(0);	/* a comment line */

	Label = ptrto;
	endptr = end_field(ptrfrm);
	while( ptrfrm != endptr )
		*ptrto++ = *ptrfrm++;
	*ptrto++ = EOS;

	while( isspace(*ptrfrm) ) ++ptrfrm;

	Op = ptrto;
	if( *ptrfrm != COMM_CHAR ){
		endptr = end_field(ptrfrm);
		while( ptrfrm != endptr )
			*ptrto++ = *ptrfrm++;
		}
	*ptrto++ = EOS;

	while( isspace(*ptrfrm) ) ++ptrfrm;
#if !SASM && !PASM
	if (*Op == SCS_CHAR) {	/* structured control statement */
		parse_scs (ptrfrm, ptrto);
		return (1);
	}
#endif
	Operand = ptrto;
	if( *ptrfrm != COMM_CHAR ){
		endptr = end_field(ptrfrm);
		while( ptrfrm != endptr )
			*ptrto++ = *ptrfrm++;
		}
	*ptrto++ = EOS;

	while( isspace(*ptrfrm) ) ++ptrfrm;

	Xmove = ptrto;
	if( *ptrfrm != COMM_CHAR ){
		endptr = end_field(ptrfrm);
		while( ptrfrm != endptr )
			*ptrto++ = *ptrfrm++;
		}
	*ptrto++ = EOS;

	while( isspace(*ptrfrm) ) ++ptrfrm;

	Ymove = ptrto;
	if( *ptrfrm != COMM_CHAR ){
		endptr = end_field(ptrfrm);
		while( ptrfrm != endptr )
			*ptrto++ = *ptrfrm++;
		}
	*ptrto++ = EOS;

	while( isspace(*ptrfrm) ) ++ptrfrm;

	Com = ptrto;
	if( *(ptrfrm + 1) != COMM_CHAR )	/* check for unreported comm */
		while( *ptrfrm != EOS )
			*ptrto++ = *ptrfrm++;
	*ptrto = EOS;

#if DEBUG
	printf("Label---%s-\n",Label);
	printf("Op------%s-\n",Op);
	printf("Operand-%s-\n",Operand);
	printf("Xmove---%s-\n",Xmove);
	printf("Ymove---%s-\n",Ymove);
	printf("Comment-%s-\n",Com);
#endif
	return(1);
}

/**
* name		end_field --- find end of field
*
* synopsis	np = end_field(ptr)
*		char *np;	end of field
*		char *ptr;	start of field
*
* description	Returns ptr to the end of a field. The field
*		separators are blanks unless the blank is embedded
*		in a string.
*
**/
static char *
end_field(ptr)
register char *ptr;
{
	while( *ptr && !isspace (*ptr) ){
		if( *ptr == STR_DELIM )
			while (*++ptr && *ptr != STR_DELIM )
				;
		else if( *ptr == XTR_DELIM )
			while (*++ptr && *ptr != XTR_DELIM )
				;
		ptr++;
		}
	return(ptr);
}

/**
*
* name		do_def --- apply any define directive translations
*
* synopsis	np = do_def(stptr,endptr,cnt)
*		char *np;	end of translated string
*		char *stptr;	start of possible define symbol
*		char *endptr;	end of possible define symbol
*		int cnt;	count of symbols on line
*
* description	If the define symbol is valid, the symbol is
*		looked up in the define table, and if found, the
*		define translation is copied over the symbol.
*		The new end of string is returned. An error will
*		occur if the define translation would cause the
*		line buffer to be overflowed.
*
**/
static char *
do_def(stptr,endptr,cnt)
char *stptr,*endptr;
int cnt;
{
#if !SASM
	struct deflist *def_lkup();
	struct deflist *dp;
	char *p,*defp;
	static undef = NO;

	if( *stptr == '_' )
		return(endptr); /* illegal define symbol */

	*endptr = EOS;		/* put in temp end of string */

	if( strlen(stptr) > MAXSYM )
		return(endptr); /* string too long to be define symbol */

	if (undef) {
		undef = NO;	/* don't expand the symbol */
		return (endptr);
	}

	p = stptr;
	undef = ((cnt == 1 && isspace(Line[0])) ||
		 (cnt == 2 && !isspace(Line[0]))) &&
		(*p   == 'U' || *p == 'u') &&
		(*++p == 'N' || *p == 'n') &&
		(*++p == 'D' || *p == 'd') &&
		(*++p == 'E' || *p == 'e') &&
		(*++p == 'F' || *p == 'f') &&
		*++p == EOS;

	if( (dp = def_lkup(stptr)) != NULL ){
		defp = dp->ddef;
#if DEBUG
printf("Looking up -%s-\n",stptr);
printf("Definition is -%s-\n",defp);
#endif
		if( Line - (stptr + strlen(defp)) >= MAXBUF ){
			error2("Re-definition would overflow line",stptr);
			return(endptr);
			}
		while( *defp )
			*stptr++ = *defp++;
		return(stptr);
		}

	return(endptr);
#endif /* SASM */
}

/**
* name		process --- determine mnemonic class and act on it
*
* synopsis	tt = process(cond_flag,else_flag)
*		int tt;			if cond_flag == YES then tt = indicates
*					terminating condition (ELSE or ENDIF)
*					if cond_flag == NO then tt = NO;
*		int cond_flag;		YES means processing conditional
*					assembly
*		int else_flag;		terminate conditional assembly on an
*					ELSE directive
*
* description	Main dispatcher for source lines. The program
*		counter at the start of the source line (Old_pc)
*		is initialized. If the line contains only a label,
*		then the label is installed using the current pc.
*		If the cond_flag is set, then the op type is examined.
*		If it is an ENDIF directive, then processing is halted.
*		If it is an ELSE directive and the else_flag is YES, then
*		processing is halted.
*		Otherwise, the Op is looked up in the mnemonic and
*		pseudo op tables and dispatched if found. An error
*		will result if the mnemonic is not valid.
*
**/
process(cond_flag,else_flag)
int cond_flag,else_flag;
{
	char	*mne, *mapmne();
	struct mnemop *i, *mne_look();
	struct pseuop *j, *psu_look();
	struct scs *k, *scs_look();
	struct mac *mp, *mac_look();

	if (Verbose && Lst_lno % Vcount == 0)
		(void)fprintf (stderr, "%s: Processing line %d in file %s (%d)\n", Progname, Line_num, Cfname, Lst_lno);
#if !PASM
	if( !Chkdo || Padreg ){		/* if not reprocessing line */
		Old_pc = *Pc;	/* setup 'old' runtime program counter */
		Old_lpc = *Lpc; /* setup 'old' load program counter */
		}
	Optr = NULL;		/* initialize Operand pointer */
	Line_err = 0;		/* clear line error count */

	if( !*Op ){		/* no mnemonic */
		do_label();	/* pickup any label if present */
		return(NO);
		}
#endif
#if !SASM
	if( (mp = mac_look(Op)) != NULL ){	/* see if this is a macro */
		(void)do_mac(mp);		/* yes -- expand macro */
		return(NO);
		}
#endif
	if( strlen(Op) < MAXOP ){	/* within op/pseudo-op lengths */
		mne = mapmne();		/* shift mnemonic to lower case */
#if !PASM
		if( (i = mne_look(mne)) != NULL ){	/* opcode mnemonic */
			if( !Chkdo )
				do_label(); /* pick up any label */
			Cycles = i->cycles;
			(void)do_op(i);
			Ctotal += Cycles;
			return(NO);
			}
#endif
#if !SASM && !PASM
		if (*Op == SCS_CHAR){	/* structured control statement */
			if( (k = scs_look(mne)) != NULL ){
				do_label(); /* pick up any label */
				(void)do_scs(k);
				return(NO);
				}
			}
#endif
#if !SASM
		if( (j = psu_look(mne)) != NULL ){	/* pseudo-op */
			if( cond_flag == YES && j->ptype == ELSE && else_flag == YES )
				return(ELSE);
			else if( cond_flag == YES && j->ptype == ENDIF )
				return(ENDIF);
			(void)do_pseudo(j);
			return(NO);
			}
#endif
		}
#if !SASM
	if( mac_lib(Op) )	/* too long or not found; MACLIB macro? */
		return(NO);
#endif
#if !PASM
	error2("Unrecognized mnemonic",Op);
#endif
	return(NO);
}

/**
*
* name		mapmne - map mnemonic to lower case
*
* synopsis	m = mapmne()
*		char *m;	lower case mnemonic
*
* description	Maps the Op field to lower case.
*
**/
char *
mapmne()
{
	static char mne[MAXOP+1]; /* buffer to hold lower case mnemonic */
	register char *pto;
	register char *pfr;

	pto = mne;	/* shift mnemonic to lower case */
	pfr = Op;
	while( (*pto++ = mapdn(*pfr++)) != EOS );
	return(mne);
}

/**
*
* name		do_label --- install label if present
*
* description	Install label (if present) into symbol table
*		using Pc and Cspace.
*
**/
do_label()
{
#if !SASM
	struct evres ev;

	if( get_i_sym(Label) == YES ){
		ev.etype = INT;
		ev.uval.xvalue.ext = ev.uval.xvalue.high = 0L;
		ev.uval.xvalue.low = *Pc;
		ev.size = SIZES;
		ev.mspace = Cspace;
		(void)install(Label,&ev,(Relmode?REL:NONE),Rcntr|Rmap);
		}
#endif
}

/**
*
* name		onsegv - handle segmentation/protection violations
*
* description	Displays a message on the standard error output and
*		exits with the error count.
*
**/
#if BSD || MPW
static
#else
static void
#endif
onsegv ()
{
	(void)fprintf (stderr, "%s: Fatal segmentation or protection fault\n", Progname);
	(void)fprintf (stderr, "%s: Contact Motorola DSP Operation\n", Progname);
	if (!Err_count)
		exit (INITERR);
	else {
#if !VMS
		exit (Err_count);
#else
		Vmserr.fld.success  = (unsigned)NO;
		Vmserr.fld.severity = VMS_ERROR;
		Vmserr.fld.message  = (unsigned)Err_count;
		exit (Vmserr.code);
#endif
	}
}

/**
*
* name		onintr - handle keyboard interrupt
*
* description	Displays a message on the standard error output and
*		exits with the error count.
*
**/
#if BSD || MPW
static
#else
static void
#endif
onintr ()
{
	if (!Cflag)
		(void)fprintf (stderr, "\n%s: Interrupted\n", Progname);
#if LSC
	longjmp (Env, ERR);
#else
	if (!Err_count)
		exit (INITERR);
	else {
#if !VMS
		exit (Err_count);
#else
		Vmserr.fld.success  = (unsigned)NO;
		Vmserr.fld.severity = VMS_ERROR;
		Vmserr.fld.message  = (unsigned)Err_count;
		exit (Vmserr.code);
#endif
	}
#endif
}

/**
*
* name		sys_date - get system date
*		sys_time - get system time
*
* synopsis	sys_date(p);
*		sys_time(p);
*		char *p;		pointer to target string
*
* description	These functions get the current date or time from the
*		system and return as YY-MM-DD (for date) and HH:MM:SS
*		(for time).
*
**/
static void
sys_date(p)
char *p;
{
	time_t time (), clock;
	struct tm *tm, *localtime ();

	clock = time ((time_t *)0);
	tm = localtime(&clock);
	(void)sprintf(p,"%02d-%02d-%02d",
		tm->tm_year,tm->tm_mon+1,tm->tm_mday);
}

static void
sys_time(p)
char *p;
{
	time_t time (), clock;
	struct tm *tm, *localtime ();

	clock = time ((time_t *)0);
	tm = localtime(&clock);
	(void)sprintf(p,"%02d:%02d:%02d",
		tm->tm_hour,tm->tm_min,tm->tm_sec);
}
