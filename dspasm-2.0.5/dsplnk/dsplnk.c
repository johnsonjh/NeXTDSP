#include "lnkdef.h"
#include "lnkdcl.h"
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
#include <stat.h>
#include <fab.h>
#endif
#if MSDOS
#include <fcntl.h>
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		main -- DSP56000 Cross Linker Main Program
*
**/
#if !LSC
main(argc,argv)		/* DSP56000 Cross Linker Main Program */
#else
_main(argc,argv)	/* special entry point for Macintosh Lightspeed C */
#endif
int     argc;
char    **argv;
{
	char *p, *q;
#if BSD
	int onintr (), onsegv ();
#else
	void onintr (), onsegv ();
#endif
	void sys_date(), sys_time();
	char *basename(), *getenv();

#if UNIX || VMS
	(void)signal (SIGSEGV, onsegv);	/* set up for fatal signals */
	(void)signal (SIGBUS,  onsegv);
#endif
#if MPW || AZTEC
        (void)sigset (SIGINT, onintr);  /* set up for signals */
#else
        (void)signal (SIGINT, onintr);  /* set up for signals */
#endif
	p = basename (argv[0]);	/* get command base name */
	if ((q = strrchr (p, '.')) != NULL)
		*q = EOS;
#if VMS
	if ((q = strrchr (p, ';')) != NULL)
		*q = EOS;
	Dcl_flag = getenv (p) == NULL;
	Vmserr.code = INITOK;	/* initialize VMS condition code */
#endif
	/* scan for compiler flag on command line */
	Cflag = get_cflag (argc-1, argv+1);

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

	Outfil = stdout;		/* set up default output file */
	sys_date(Curdate);		/* set up current date */
	sys_time(Curtime);		/* set up current time */

        initialize();

	Comfp = argv+1;
	Comfiles = argc-1;
	if (Dcl_flag) {
#if VMS
		proc_dclo();	/* process VMS DCL command line options */
		if (cli$present (&Lnk_desc) == CLI$_ABSENT)
			cmderr ("Missing link filename");
#endif
	} else {
		proc_cmdo();	/* process command line options */
		if( Comfiles == 0 )	/* must have at least one filename! */
			cmderr ("Missing link filename");
	}

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning pass 1\n", Progname);

       	make_pass();		/* do pass 1 */

	fixup ();		/* perform section/symbol fixups */

        re_init();		/* reinitialize for Pass 2 */

	Comfp = argv+1;
	Comfiles = argc-1;
	if (Dcl_flag) {
#if VMS
		proc_dclo();	/* process VMS DCL command line options */
#endif
	} else {
		proc_cmdo();	/* process command line options */
	}

	if (!Lodfil)		/* open default load file */
		default_lod(Comfiles,Comfp);

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning pass 2\n", Progname);

	make_pass();		/* do pass 2 */

	cleanup(NO);		/* general cleanup */

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

/**
*
* name		usage - display usage
*
**/
static
usage ()
{
#if !MSDOS
	(void)fprintf (stderr, "%s  %s\n%s\n", Progtitle, Version, Copyright);
#else
	if (Cflag)
		(void)fprintf (stderr, "%s  %s\n%s\n",
			Progtitle, Version, Copyright);
#endif
	(void)fprintf (stderr, "Usage:  %s [-B[<lodfil>]] [-D] [-F<argfil>] [-L<library>] [-M[<mapfil>]] [-O<mem>[<ctr>][<map>]:<origin>] [-R[<memfil>]] [-V] [<lnkfil>...\n", Progname);
	(void)fprintf (stderr, "where:\n");
	(void)fprintf (stderr, "        lodfil  - optional load file name\n");
	(void)fprintf (stderr, "        argfil  - command line argument file name\n");
	(void)fprintf (stderr, "        library - library file name\n");
	(void)fprintf (stderr, "        mapfil  - optional load map file name\n");
	(void)fprintf (stderr, "        mem     - memory space (X, Y, L, P)\n");
	(void)fprintf (stderr, "        ctr     - location counter (L, H)\n");
	(void)fprintf (stderr, "        map     - memory mapping (I, E, B)\n");
	(void)fprintf (stderr, "        origin  - memory space origin\n");
	(void)fprintf (stderr, "        memfil  - optional memory map file name\n");
	(void)fprintf (stderr, "        lnkfil  - link input file name\n");
#if LSC
        longjmp (Env, ERR);
#else
	exit (INITERR);
#endif
}

/**
*
* name 		proc_cmdo - process command line options
*
**/
static
proc_cmdo()
{
	char *opt;
	char *default_fn(), *basename(), *strup();
	int no_opt, spc, ctr, map;
	char c, *p;

	for ( ; *Comfp && **Comfp == '-'; Comfp++, Comfiles-- ){
		opt = *Comfp; /* get ptr to option */
		++opt; /* skip switch character */

		switch( mapdn(*opt++) ){
			case 'b':
				if (Pass == 1)	/* skip load file on Pass 1 */
					break;
				if( *opt )
					no_opt = NO;
				else{
					no_opt = YES;
					opt = default_fn (Comfiles, Comfp);
					}
				(void)strcpy(String,opt); /* get filename */
				if (no_opt)	/* add default extension */
					fix_fn(".lod");
			       	if((Lodfil = fopen(String,"w")) == NULL)
					cmderr2 ("Cannot open load file", String);
				break;

                        case 'c':
                                if( *opt )
					cmderr ("Illegal command line -C option");
				break;          /* already have it */
			case 'd':
                                if( *opt )
					cmderr ("Illegal command line -D option");
				Debug = YES;
				break;
			case 'f':
				if( !*opt )
					cmderr ("Missing command file name");
				fileargs (opt);	/* get file arguments */
				continue;
			case 'l':
				if (Pass == 2)
					break;	/* skip on Pass 2 */
				if( !*opt )
					cmderr ("Missing library name");
				(void)printf ("%s: Initial library specification ignored: %s\n", Progname, opt);
				break;
			case 'm':
				if (Pass == 1)	/* skip map file on Pass 1 */
					break;
				if( *opt )
					no_opt = NO;
				else{
					no_opt = YES;
					opt = default_fn (Comfiles, Comfp);
					}
				(void)strcpy(String,opt); /* get filename */
				if (no_opt)
					fix_fn(".map"); /* add default extension */
				(void)strcpy (Mapfn, String);	/* save map file name */
			       	if((Mapfil = fopen(String,"w")) == NULL)
					cmderr2 ("Cannot open map file", String);
				break;
			case 'n':
				Icflag = YES;
				break;
			case 'o':
				if( !*opt )
					cmderr ("Missing command line -O option");
				if ((spc = get_space (*opt++) - 1) < 0)
					cmderr ("Invalid memory space specifier");
				if ((ctr = get_cntr (*opt)) != ERR)
					opt++;
				else
					ctr = DEFAULT;
				map = NONE;
				switch (c = mapdn (*opt)) {
					case 'i':
					case 'e':
					case 'b':
						opt++;
						if (*opt++ != ':') 
							cmderr ("Syntax error in command line");
						if ((map = char_map (c)) == ERR) 
							cmderr ("Invalid memory map character");
						if (map == BMAP && spc != PMEM) 
							cmderr ("Bootstrap mapping available only in P memory");
						break;
					case ':':
						opt++;
					/* fall through */
					case EOS:
						break;
					default:
						cmderr ("Syntax error in command line");
				}
				(void)sscanf (opt, "%lx", &Start[spc][ctr]);
				Mflags[spc][ctr] |= map | SETCTR;
				break;
			case 'r':
				if (Pass == 2)	/* skip mem file on Pass 2 */
					break;
				if( *opt )
					no_opt = NO;
				else{
					no_opt = YES;
					opt = default_fn (Comfiles, Comfp);
					}
				(void)strcpy(String,opt); /* get filename */
				if (no_opt) {
					fix_fn(".ctl"); /* add default ext */
					p = strrchr (String, '.');
				}
				(void)strcpy (Mapfn, String);/* save file name*/
			       	if((Memfil = fopen(String,"r")) == NULL)
					if( no_opt && p ){ /* try another */
						*p = EOS;
						fix_fn (".mem");
						Memfil = fopen (String, "r");
						}
				if( !Memfil )
					cmderr2 ("Cannot open memory control file", String);
#if VMS				/* Yet another VMS/RMS/VAXC file record structure kludge */
				(void)tmp_memfil (); /* transfer contents to temporary file */
#endif
				break;
			case 't':
                                if( *opt )
					cmderr ("Illegal command line -T option");
				Testing = YES;
				break;
			case 'v':
				Verbose = YES;
				if( *opt )
					(void)sscanf(opt,"%d",&Vcount);
				break;
			default:
				cmderr2 ("Illegal command line option", opt-2);
			}
		}
}

#if VMS
/**
*
* name 		proc_dclo - process VMS DCL command line options
*
**/
static
proc_dclo()
{
	char c, *p;
	int spc, ctr, map, no_opt;
	char *basename ();
			
	/* reset command line */
	if (cli$dcl_parse(NULL, NULL) != CLI$_NORMAL) 
		cmderr ("Cannot parse command line");
	/* DEBUG option */
	if (cli$present (&Dbg_desc) == CLI$_PRESENT)
		Debug = YES;
	/* MAP option */
	if (Pass == 2 && cli$present (&Map_desc) == CLI$_PRESENT) {
		if (dcl_getval (&Map_desc) == CLI_ABSENT) {
			if (dcl_getval (&Lnk_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL) 
				cmderr ("Cannot parse command line");
			(void)strcpy (String, basename (String));
			if ((p = strrchr (String, '.')) != NULL)
				*p = EOS;	/* strip off extension */
			fix_fn(".map"); /* add default extension */
		}
		(void)strcpy (Mapfn, String);	/* save map file name */
	       	if ((Mapfil = fopen(String,"w")) == NULL) 
			cmderr2 ("Cannot open map file", String);
	}
	/* MEMORY option */
	no_opt = NO;
	if (Pass == 1 && cli$present (&Mem_desc) == CLI$_PRESENT) {
		if (dcl_getval (&Mem_desc) == CLI_ABSENT) {
			if (dcl_getval (&Lnk_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL) 
				cmderr ("Cannot parse command line");
			no_opt = YES;
			(void)strcpy (String, basename (String));
			if ((p = strrchr (String, '.')) != NULL)
				*p = EOS;	/* strip off extension */
			fix_fn(".ctl");	/* add default extension */
		}
		(void)strcpy (Mapfn, String);	/* save mem file name */
	       	if((Memfil = fopen(String,"r")) == NULL)
			if( no_opt ){ /* try another */
				if ((p = strrchr (String, '.')) != NULL)
					*p = EOS;
				fix_fn (".mem");
				Memfil = fopen (String, "r");
				}
		if( !Memfil )
			cmderr2 ("Cannot open memory control file", String);
		(void)tmp_memfil (); /* transfer contents to temporary file */
	}
	/* NOCASE option */
	if (cli$present (&Noc_desc) == CLI$_PRESENT)
		Icflag = YES;
	/* OBJECT option */
	if (Pass == 2 && cli$present (&Obj_desc) == CLI$_PRESENT) {
		if (dcl_getval (&Obj_desc) == CLI_ABSENT) {
			if (dcl_getval (&Lnk_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL) 
				cmderr ("Cannot parse command line");
			(void)strcpy (String, basename (String));
			if ((p = strrchr (String, '.')) != NULL)
				*p = EOS;	/* strip off extension */
			/* add default extension */
			fix_fn(".lod");
		}
	       	if ((Lodfil = fopen(String,"w")) == NULL) 
			cmderr2 ("Cannot open load file", String);
	}
	/* ORIGIN option */
	while (dcl_getval (&Org_desc) != CLI_ABSENT) {
		p = String;
		if ((spc = get_space (*p++) - 1) < 0)
			cmderr ("Invalid memory space specifier");
		if ((ctr = get_cntr (*p)) != ERR)
			p++;
		else
			ctr = DEFAULT;
		map = NONE;
		switch (c = mapdn (*p)) {
			case 'i':
			case 'e':
			case 'b':
				p++;
				if (*p++ != ':') 
					cmderr ("Syntax error in command line");
				if ((map = char_map (c)) == ERR) 
					cmderr ("Invalid memory map character");
				if (map == BMAP && spc != PMEM) 
					cmderr ("Bootstrap mapping available only in P memory");
				break;
			case ':':
				p++;
			/* fall through */
			case EOS:
				break;
			default:
				cmderr ("Syntax error in command line");
		}
		(void)sscanf (p, "%lx", &Start[spc][ctr]);
		Mflags[spc][ctr] |= map | SETCTR;
	}
	/* TEST option */
	if (cli$present (&Tst_desc) == CLI$_PRESENT)
		Testing = YES;
	/* VERBOSE option */
	if (cli$present (&Vbs_desc) == CLI$_PRESENT) {
		Verbose = YES;
		if (dcl_getval (&Vbs_desc) == CLI_ABSENT) {
			if (dcl_getval (&Lnk_desc) == CLI_ABSENT ||
			    cli$dcl_parse(NULL, NULL) != CLI$_NORMAL)
				cmderr ("Cannot parse command line");
		} else
			(void)sscanf (String, "%d", &Vcount);
	}
}
#endif

#if VMS
/**
*
* name		tmp_memfil - open temporary memory/control file
*
* synopsis	tmp_memfil ()
*
* description	This is Yet Another Kludge for VMS to insure that the
*		memory/control file is seekable by the VAX C runtime
*		routines.  It copies the contents of the currently open
*		control file into a temporary file and uses the temporary
*		file in future memory/control operations.
*
**/
static
tmp_memfil ()
{
	FILE *tmp, *tmpfile ();
	int c;

	if (chkfmt (Memfil))
		return;		/* record format OK */
	if ((tmp = tmpfile ()) == NULL)
		cmderr ("Cannot open temporary memory control file");
	if (rewind (Memfil) != 0)
		cmderr ("Cannot rewind memory control file");
	while ((c = fgetc (Memfil)) != EOF)	/* copy the file */
		(void)fputc (c, tmp);
	(void)fclose (Memfil);
	if (rewind (tmp) != 0)
		cmderr ("Cannot rewind memory control file");
	Memfil = tmp;
}
#endif

#if VMS
/**
*
* name		chkfmt - check file record format
*
* synopsis	yn = chkfmt (fd)
*		int yn;		YES if format OK, NO otherwise
*		FILE *fd;	file descriptor
*
* description	Checks the file associated with fd to insure it is the
*		appropriate type for VAX C operations.
*
**/
static
chkfmt (fd)
FILE *fd;
{
	struct stat stbuf;

	if (fstat (fileno (fd), &stbuf) < 0)
		return (NO);
	if (stbuf.st_fab_rfm != FAB$C_STMLF || stbuf.st_fab_rat != FAB$M_CR)
		return (NO);
	return (YES);
}
#endif

/**
*
* name		dcl_getval - get DCL command line value
*
* synopsis	status = dcl_getval (opt)
*		int status;	return status from cli$get_value
*		struct dsc$descriptor_s *opt;	pointer to command line option
*
* description	Calls the VMS DCL routine cli$get_value to return values
*		for the command line option referenced by opt.  The values
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
#if VMS
	unsigned status, len = 0;
	
	status = cli$get_value (opt, &Arg_desc, &len);
	if (status != CLI_ABSENT)
		String[len] = EOS;
	return (status);
#else
	return (*opt);
#endif
}

/**
*
* name		dcl_reset - reset DCL command line
*
* synopsis	yn = dcl_reset ()
*		int yn;		YES/NO for errors
*
* description	Resets the DCL command line so that it may be reparsed.
*		Returns YES if command line is reset, NO on error.
*
**/
static
dcl_reset ()
{
#if VMS
	return (cli$dcl_parse(NULL, NULL) == CLI$_NORMAL);
#else
	return (YES);
#endif
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
}

/**
*
* name		initialize --- init for pass 1
*
* description	Initializes global variables.
*
**/
static
initialize()
{
#if DEBUG
        printf("Initializing\n");
#endif
	Ccomf = 0;
	Cfile = Files;
        Err_count = 0;
	Recno = 0;
	Lineno = Fldno = 1;
	Secnt = Sectot = 0;
	Radix = 10;
	alloc_secmap();
        Pass = 1;
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
#if DEBUG
        printf("Reinitializing\n");
#endif
	Ccomf = 0;
	Cfile = Files;
        Err_count = 0;
	Recno = 0;
	Lineno = Fldno = 1;
	Page = 1;
	Old_pc = Old_lpc = 0L;
	Cspace = Loadsp = PSPACE;
	Radix = 10;
	fake_free((char *)Secmap);	/* free global section map */
	Pass = 2;
}

/**
* name		cleanup --- cleanup after assembly
*
* synopsis	cleanup(err)
*		int err;	indicates whether cleanup due to error
*
* description	Perform general cleanup after assembly.  Clear dangling
*		sections.  Free various lists.  Finish up object and listing
*		files.
*
**/
cleanup (err)
int err;
{
#if DEBUG
        printf("Cleaning up\n");
#endif
	if (Xrfcnt)
		disp_xref ();		/* rpt remaining ext refs */
	if (Lodfil) {
		if (!err)
			done_obj ();	/* finish up load file */
		(void)fclose (Lodfil);
	}
	if (Mapfil) {
		if (!err)
			do_map ();	/* do map file */
		(void)fclose (Mapfil);
	}
	free_sec();	/* free sections */
	free_sym();	/* free symbol table entries */
	free_xrf();	/* free external reference list */
	free_files();	/* free link file list */
}

/**
*
* name		alloc_secmap --- allocate global section map
*
**/
static
alloc_secmap()
{
	register i, j, k;

	if ((Secmap = (struct smap *)alloc (sizeof (struct smap) * MAXSEC)) == NULL) 
		cmderr ("Out of memory - cannot allocate global section map");
	for (i = 0; i < MAXSEC; i++) {	/* clear map entries */
		for (j = 0; j < MSPACES; j++)
			for (k = 0; k < MCNTRS; k++)
				Secmap[i].offset[j][k] = 0L;
		Secmap[i].sec = NULL;
	}
}

/**
* name		make_pass --- perform pass on link input
*
* description	On first pass, process command line files, putting
*		them in a chain and calling the appropriate processing
*		routine depending on whether the file is a library or not.
*		On second pass, revisit the linked files and process
*		again based on the file type.
*
**/
static
make_pass()
{
	struct file *cfile = NULL;
#if DEBUG
        printf("Pass %d\n",Pass);
#endif
	if (Pass == 1) {	/* first pass processing */

		while ( Dcl_flag ? dcl_getval (&Lnk_desc) != CLI_ABSENT :
				   ++Ccomf <= Comfiles ) {
			if (!Dcl_flag)
				(void)strcpy(String,*Comfp++);
			setup_file ();
			if (Cfile->flags & LIB)
				proc_lib ();	/* process library file */
			else
				(void)proc_mod (Cfile->cmod); /* link file */
			if (Verbose)
				(void)fprintf (stderr,
					"%s: Closing %s file %s\n",
					Progname, Cfile->flags & LIB ?
					"library" : "link", Cfile->name);
			(void)fclose (Fd);
		}

	} else {		/* second pass processing */

		if (Lodfil)
			start_obj ();		/* start load file */
		for (Cfile = Files; Cfile; cfile = Cfile, Cfile = Cfile->next) {
			if (Verbose)
				(void)fprintf (stderr,
					"%s: Opening %s file %s\n",
					Progname, Cfile->flags & LIB ?
					"library" : "link", Cfile->name);

			if((Fd = fopen(Cfile->name,BINREAD)) == NULL)
				if (Cfile->flags & LIB)
					fatal("Cannot open library file");
				else
					fatal("Cannot open link file");
#if VMS
			if (!chkfmt (Fd))
				fatal ("Invalid RMS record format");
#endif
			if (Cfile->flags & LIB)
				proc_lib ();	/* process library file */
			else {
				Secmap = Cfile->cmod->secmap;
				(void)proc_mod (Cfile->cmod); /* link file */
			}
			if (Verbose)
				(void)fprintf (stderr,
					"%s: Closing %s file %s\n",
					Progname, Cfile->flags & LIB ?
					"library" : "link", Cfile->name);
			(void)fclose (Fd);
		}
		Cfile = cfile;			/* restore last file info */
	}
}

/**
* name		setup_file - open link file, determine type
*
* synopsis	setup_file ()
*
* description 	Examine filename in String for library flag;
*		if present, set library switch.  Open the file
*		and insure that it is a valid link file.  Set
*		up file info and module data fields.
*
**/
static
setup_file ()
{
	int lib;		/* saved library status */
	struct file *np;	/* file structure pointer */
	struct module *mp;	/* module pointer */

	lib = chklib ();		/* determine library status */

	fix_fn (lib ? ".lib" : ".lnk");	/* add default extension */

	if (Testing)		/* map to lower case if testing */
		(void)strdn (String);

	if (Verbose)
		(void)fprintf (stderr, "%s: Opening %s file %s\n",
			Progname, lib ? "library" : "link", String);

	if((Fd = fopen(String,BINREAD)) == NULL) 
		cmderr2 ("Cannot open link file", String);
#if VMS
	if (!chkfmt (Fd))
		fatal ("Invalid RMS record format");
#endif

	chkfile (lib);		/* make sure file is valid */

	if ( !(np = (struct file *)alloc(sizeof (struct file))) ||
	     !(np->name = (char *)alloc(strlen(String)+1)) ) 
		cmderr ("Out of memory - cannot save file data");

	(void)strcpy(np->name,String);
	np->flags = lib ? LIB : NONE;
	if (lib)
		np->cmod = np->modlst = NULL;
	else {				/* link file; fill in module info */
		if (!(mp = (struct module *)alloc(sizeof (struct module)))) 
			cmderr ("Out of memory - cannot save module data");
		mp->offset = 0L;	/* no library offset */
		mp->secmap = NULL;	/* default to no sections */
		mp->next = NULL;
		np->cmod = mp;
		np->modlst = NULL;
	}
	np->next = NULL;
	if (!Files)			/* link file into chain */
		Files = Cfile = np;
	else
		Cfile = Cfile->next = np;
	Lineno = 1;			/* initialize line number */
}

/**
* name		chklib - determine if link file is a library
*
* synopsis	yn = chklib()
*		int yn;		YES / NO if file is specified as library
*
* description 	Checks the DCL flag to see if doing DCL parsing.
*		If so, checks for library qualifier and returns its
*		presence or absence.  Otherwise, checks string buffer
*		for library switch and if present shifts buffer over
*		and returns YES, else NO.
*
**/
static
chklib()
{
	register char *p, *q;

	if (Dcl_flag) {
#if VMS
		if (cli$present (&Lib_desc) == CLI$_LOCPRES)
			return (YES);
#endif
	} else {
		if (String[0] == '-' &&
		    (String[1] == 'L' || String[1] == 'l')) {
			for (p = String, q = String + 2; *q; p++, q++)
				*p = *q;
			*p = EOS;
			return (YES);
		}
	}
	return (NO);
}

/**
* name		proc_lib - process link library modules
*
* synopsis	yn = proc_lib ()
*		int yn;			YES / NO if errors
*
* description 	During pass 1 check the global count and flags for
*		external references.  While unresolved symbols exist,
*		rewind the current library file and scan it using
*		pass_lib.
*
*		On pass 2, scan the current file module list and process
*		data and block data records in the library module.
*
**/
static
proc_lib ()
{
	struct module *mp;	/* module structure pointer */

	Recno = 0;
	Fldno = 1;
	if (Pass == 1) {

		while (Xrfcnt && Xrf_flag) {	/* outstanding ext. refs. */

			Xrf_flag = NO;		/* reset flag */
			if (fseek (Fd, 0L, SEEK_SET) != 0)
				fatal ("Seek failure");

			pass_lib ();	/* process library */
		}
		Xrf_flag = Xrfcnt ? YES : NO;

	} else {	/* Pass 2 */

		for (mp = Cfile->modlst; mp; mp = mp->next) {
			Cfile->cmod = mp;	/* set up current module */
			Secmap = mp->secmap;	/* current section map */
			(void)proc_mod (mp);	/* process module */
		}
	}
}

/**
* name		pass_lib - first pass process of link library modules
*
* synopsis	yn = pass_lib ()
*		int yn;			YES / NO if errors
*
* description 	During pass 1 while any external references remain
*		scan each module in the library.  Lookup each library module
*		global symbol in the external reference table until
*		the first match is found.  If found, process section
*		and symbol records in the library modules.  Record pertinent
*		module data in the current file module list.
*
**/
static
pass_lib ()
{
	char hdr[MAXBUF];	/* module header buffer */
	long size;		/* module size */
	long offset;		/* file offset */
	int rectype;		/* record type */
	long ftell ();

	Recno = 0;
	Fldno = 1;

	while (Xrfcnt && get_header (Fd, hdr)) {

		(void)sscanf (hdr, "%*s %*s %ld", &size);
		if ((offset = ftell (Fd)) < 0L)	/* save offset */
			fatal ("Offset failure");

		if (!get_start ())		/* get START record */
			continue;		/* error in START record */

		while ((rectype = get_record ()) == OBJSECTION)
			if (!get_section (LIB))
				break;

		if (rectype != OBJSYMBOL) {
			if (fseek (Fd, offset, SEEK_SET) != 0 ||
			    fseek (Fd, size + 1L, SEEK_CUR) != 0)
				fatal ("Seek failure");
			(void)free_seclst ();
			continue;	/* no sym recs in module */
		}

		(void)proc_libmod (offset, size); /* process library module */
		(void)free_seclst ();	/* free section list */
	}
}

/**
* name		proc_libmod - process library input module
*
* synopsis	yn = proc_libmod (offset, size)
*		int yn;			YES / NO if errors
*		long offset;		module offset in library
*		long size;		size of module
*
* description 	Scans the current library module comparing symbol names
*		to outstanding external references.  If a match occurs,
*		rewinds the current module and processes it as a standard
*		link module.
*
**/
static
proc_libmod (offset, size)
long offset, size;
{
	struct nlist *sp, *get_symbuf ();
	struct symref *xp, *lkupxrf ();
	struct module *mp;	/* module structure pointer */

	/* loop until symbol matches external reference */
	while ((sp = get_symbuf ()) != NULL) {

		if ((xp = lkupxrf (sp->name)) == NULL)
			continue;	/* not in external ref. table */

		/* found a matching symbol; now check it out */
		if (STREQ (Csect->sname, xp->sec->sname))
			break;		/* symbol in current section */

		if (sp->attrib & GLOBAL)	/* global symbol */
			break;
	}

	if (!sp) {	/* no external symbol matches in module */
		if (fseek (Fd, offset, SEEK_SET) != 0 ||
		    fseek (Fd, size + 1L, SEEK_CUR) != 0)
			fatal ("Seek failure");
		return (NO);
	}

	if (fseek (Fd, offset, SEEK_SET) != 0)	/* rewind module */
		fatal ("Seek failure");
	if (!(mp = (struct module *)alloc(sizeof (struct module)))) 
		cmderr ("Out of memory - cannot save module data");
	mp->offset = 0L;
	mp->secmap = NULL;
	mp->next = NULL;
	if (Cfile->modlst)		/* link into module chain */
		Cfile->cmod = Cfile->cmod->next = mp;
	else
		Cfile->modlst = Cfile->cmod = mp;

	return (proc_mod (mp));		/* process module */
}

/**
* name		proc_mod - process link input module
*
* synopsis	yn = proc_mod (mp)
*		int yn;			YES / NO if errors
*		struct module *mp;	pointer to link module
*
* description 	On pass 1, process section and symbol records in the
*		link module.  On pass 2, process data and block data
*		records in the link module.
*
**/
static
proc_mod (mp)
struct module *mp;
{
	int rc = YES, first = NO, end = NO;
	int rectype;

	Recno = 0;
	Fldno = 1;
	if (Pass == 1) {

		Field[0] = EOS;			/* clear field buffer */
		if (!get_start ())		/* get START record */
			rc = NO;		/* error in START record */

		if (rc && get_record () == OBJSECTION)	/* global sec rec */
			rc = get_section (ANY);
		else {
			error ("No GLOBAL section record");
			rc = NO;
		}
		while (rc && !end) {		/* loop while status good */
			rectype = get_record ();
			switch (rectype) {
				case OBJSTART:
					error ("Duplicate START record");
					rc = NO;
					break;
				case OBJSECTION:
					rc = get_section (ANY);
					break;
				case OBJSYMBOL:
					rc = get_symbol ();
					break;
				case OBJXREF:
					rc = get_xref ();
					break;
				case OBJCOMMENT:
					rc = get_comment ();
					break;
				case OBJDATA:
				case OBJBLKDATA:
				case OBJLINE:
					Field[0] = EOS;	/* skip on Pass 1 */
					break;
				case OBJEND:
					get_eof ();	/* find end of file */
					end = YES;
					break;
				case OBJNULL:
				default:
					error ("Invalid record type");
					rc = NO;
					break;
			}

			switch (rectype) {	/* save module offset */
				case OBJXREF:
				case OBJCOMMENT:
				case OBJDATA:
				case OBJBLKDATA:
				case OBJLINE:
				case OBJEND:
					if (!first) {	/* save values */
						save_module (mp);
						first = YES;
					}
					break;
				case OBJSTART:
				case OBJSECTION:
				case OBJSYMBOL:
				case OBJNULL:
				default:
					break;
			}
		}
		save_secmap (mp);		/* save section map */
		
	} else {	/* Pass 2 */

		Field[0] = EOS;			/* reset field buffer */
		restore_module (mp);		/* restore module values */
		rc = mp->flags;			/* restore status */
		while (rc && !end)		/* loop while status good */
			switch (get_record ()) {
				case OBJSTART:
					rc = NO;
					break;
				case OBJSECTION:
				case OBJSYMBOL:
				case OBJXDEF:
				case OBJXREF:
				case OBJLINE:
					Field[0] = EOS; /* skip on Pass 2 */
					break;
				case OBJCOMMENT:
					rc = get_comment ();
					break;
				case OBJDATA:
					rc = get_data ();
					break;
				case OBJBLKDATA:
					rc = get_bdata ();
					break;
				case OBJEND:
					rc = get_end ();
					get_eof ();	/* find end of file */
					end = YES;
					break;
				case OBJNULL:
				default:
					rc = NO;
					break;
			}
	}
	if (!end && feof (Fd)) {
		rc = NO;
		if (Pass == 1)
			error ("No END record");
	}
	mp->flags = rc;		/* save error status */
	return (rc);
}

/**
* name		save_secmap - save section map array
*
* synopsis	save_secmap (mp)
*		struct module *mp;	pointer to module structure
*
* description 	Copy the global section map into the module structure.
*
**/
static
save_secmap (mp)
struct module *mp;
{
	struct smap *sm;
	register i, j, k;

	if (!Secnt)	/* no sections encountered */
		return;
	if ((sm = (struct smap *)alloc (Secnt * sizeof (struct smap))) == NULL)
		fatal ("Out of memory - cannot save section map");
	for (i = 0; i < Secnt; i++) {	/* copy section map */
		for (j = 0; j < MSPACES; j++) {
			for (k = 0; k < MCNTRS; k++) {
				sm[i].offset[j][k] = Secmap[i].offset[j][k];
				Secmap[i].offset[j][k] = 0L;
			}
		}
		sm[i].sec = Secmap[i].sec;
		Secmap[i].sec = NULL;	/* clear global entry */
	}
	mp->secmap = sm;		/* save module map */
	Secnt = 0;			/* clear module section count */
}

/**
* name		save_module - save module offset and values
*
* synopsis	save_module (mp)
*		struct module *mp;	pointer to module structure
*
* description 	Copy the record offset and line, field, and record
*		numbers into the module structure for Pass 2.
*
**/
static
save_module (mp)
struct module *mp;
{
	mp->offset = Recoff;
	mp->lineno = Lineno;
	mp->recno = Recno ? Recno - 1 : 0;
}

/**
* name		restore_module - restore module offset and values
*
* synopsis	restore_module (mp)
*		struct module *mp;	pointer to module structure
*
* description 	Seek to saved record offset. Restore line, record,
*		and field numbers.
*
**/
static
restore_module (mp)
struct module *mp;
{
	if (fseek (Fd, mp->offset, SEEK_SET) != 0)
		fatal ("Seek failure");
	Lineno = mp->lineno;
	Recno = mp->recno;
	Fldno = 1;
}

/**
*
* name		fixup - perform section and symbol address fixups
*
* description	Process the memory relocation file if present.
*		Sort the sections by section number.  Move section lengths
*		to a temporary area and move the program counter
*		accumulators to the section counter fields.  Then add
*		the hold area values to the accumulators.  When all
*		sections have been processed scan the symbol table
*		and perform address fixups on all relocatable symbols.
*
**/
static
fixup ()
{
	struct slist *sp, **sort_tab;
	struct nlist *np;
	struct evres *ev, *eval_ad ();
	register i, j, k;
	long tmp[MSPACES][MCNTRS];

	Cfile = NULL;		/* clear current file pointer */

	if (Cflag)			/* processing C files */
		(void)do_cfix ();	/* fixup compiler symbols */

	if (Memfil) {		/* memory relocation file */
		(void)do_mem ();
		(void)fclose (Memfil);
	}

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning section relocation\n",
			Progname);

	if (Sec_sort)		/* already have table */
		sort_tab = Sec_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Sec_sort = (struct slist **)alloc ((Sectot + 1) * sizeof(struct slist *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort sections");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( sp = Sectab[j]; sp; sp = sp->next )
				sort_tab[i++] = sp;
		sort_tab[i] = NULL;	/* seal off list */
	}

	sortsecs (sort_tab, Sectot);	/* sort the sections */

	/* loop to relocate sections */
	for (i = 0; i < Sectot; i++) {
		sp = sort_tab[i];
		if (!(sp->flags & REL))
			continue;	/* only reloc. sections */
		for (j = 0; j < MSPACES; j++) {
			for (k = 0; k < MCNTRS; k++) {
				if (sp->cflags[j][k] & SETCTR)
					continue; /* skip set sections */
				tmp[j][k] = sp->cntrs[j][k]; /* save length */
				/* set up start address */
				sp->cntrs[j][k]=sp->start[j][k]=Start[j][k];
				Start[j][k] += tmp[j][k]; /* fixup address */
			}
		}
	}

	/* free the sort table */
	fake_free((char *)sort_tab);
	Sec_sort = NULL;

	if (Verbose)
		(void)fprintf (stderr, "%s: Beginning symbol relocation\n",
			Progname);

	/* loop to relocate symbols */
	for (i = 0; i < HASHSIZE; i++)
		for (np = Symtab[i]; np; np = np->next) {
			if (!(np->attrib & REL))
				continue;	/* only relocatable */
			if (!(np->attrib & INT)) {
				error2 ("Invalid symbol type", np->name);
				continue;
			}
			j = spc_off (np->attrib & MSPCMASK) - 1;
			if (j < 0)		/* no valid space */
				continue;
			k = ctr_off (np->mapping & MCTRMASK);
			if (k < 0)	/* check counter offset */
				fatal ("Counter offset failure");
			if (j >= 0)	/* valid memory space */
				np->def.xdef.low += np->sec->start[j][k];
		}

	if (End_exp) {		/* adjust start address */
		Optr = End_exp;
		if ((ev = eval_ad ()) == NULL)
			return;
		End_addr = ev->uval.xvalue.low;
		free_exp (ev);
		fake_free(End_exp);
	}
}

/**
*
* name		onsegv - handle segmentation/protection violations
*
* description	Displays a message on the standard error output and
*		exits with the error count.
*
**/
#if BSD
static
#else
static void
#endif
onsegv ()
{
	(void)fprintf (stderr, "\n%s: Fatal segmentation or protection fault\n", Progname);
	(void)fprintf (stderr, "\n%s: Contact Motorola DSP Operation\n", Progname);
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
#if BSD
static
#else
static void
#endif
onintr ()
{
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
	struct tm *tm, *localtime();

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
	struct tm *tm, *localtime();

	clock = time ((time_t *)0);
	tm = localtime(&clock);
	(void)sprintf(p,"%02d:%02d:%02d",
		tm->tm_hour,tm->tm_min,tm->tm_sec);
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
*		after the option list.  If a filename argument cannot
*		be found, it reports a fatal error.
*
**/
static char *
default_fn (argc, argv)
int argc;
char **argv;
{
	static char *p = NULL;
	char *q;
	char *basename ();

	if (p)			/* already found filename */
		return (p);

	while (argc > 0 && **argv == '-') {	/* find first filename */
		argc--;
		argv++;
	}

	if (argc <= 0) 
		cmderr ("Cannot open command line link file");

	q = basename (*argv);
	if ((p = (char *)alloc (strlen (q) + 1)) == NULL)
		fatal ("Out of memory - cannot save filename");

	(void)strcpy (p, q);	/* copy the name out of arg list */
	if ((q = strrchr (p, '.')) != NULL && (q != p && q != p + 1))
		*q = EOS;	/* strip off extension */
	return (p);
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
static
fix_fn(ext)
char *ext;
{
	int extlen;
	char *p,*np,*ep,*basename(),*strup();

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
}

/**
*
* name		default_lod - open default load file
*
* synopsis	default_lod (argc, argv)
*		int argc;	count of command line arguments
*		char **argv;	pointer to command line arguments
*
* description	Scans for the default input file name and opens a load
*		file based on that name.
*
**/
static
default_lod (argc, argv)
int argc;
char **argv;
{
	char *default_fn ();

	if (Dcl_flag) {
		if (dcl_getval (&Lnk_desc) == CLI_ABSENT || !dcl_reset ()) 
			cmderr ("Cannot parse command line");
	} else
		(void)strcpy (String, default_fn (argc, argv));

	fix_fn (".lod");
	if ((Lodfil = fopen (String,"w")) == NULL) 
		cmderr2 ("Cannot open load file", String);
}

/**
*
* name		fileargs - collect arguments from command file
*
* synopsis	fileargs (fn)
*		char *fn;	pointer to command file name
*
* description	Open command file and read linker command arguments.
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
	struct strlst sl, *sp = &sl, *op;

	if (argv && Pass == 2) {	/* already have args */
		Comfp = argv;
		Comfiles = argc;
		return;
	}

       	if ((fp = fopen (fn, "r")) == NULL) 
		cmderr2 ("Cannot open command file", fn);

	sl.str = NULL;
	while (!feof (fp)) {		/* read in args */

		while ((c = fgetc (fp)) != EOF && !(isprint (c) && c != ' '))
			;
		if (c == COMCHAR) {	/* comment; read until end of line */
			while ((c = fgetc (fp)) != EOF && c != NEWLINE)
				;
			continue;
		}
		buf[0] = c;
		for (p = buf + 1, i = 1;
		     (c = fgetc (fp)) != EOF && (isprint (c) && c != ' ');
		     p++, i++)
			*p = c;
		*p = EOS;

		if (!feof (fp)) {	/* allocate string list structure */
			if ((op = (struct strlst *)alloc (sizeof (struct strlst))) == NULL || (op->str = (char *)alloc (i + 1)) == NULL)
				fatal ("Out of memory - cannot save command file arguments");
			(void)strcpy (op->str, buf);
			op->next = NULL;
			sp = sp->next = op;
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

	sp = sl.next;
	*argv = "";			/* empty string */
	for (i = 1; i < argc; i++) {	/* populate the argument vector */
		*(argv + i) = sp->str;
		op = sp;
		sp = sp->next;
		fake_free((char *)op);
	}
	*(argv + i) = NULL;

	Comfp = argv;
	Comfiles = argc;
}

/**
*
* name		do_cfix - fixup special C compiler symbols
*
* synopsis	yn = do_cfix ()
*		int yn;		YES if OK, NO on error
*
* description	Lookup the X and Y global memory space sections.
*		Obtain length from default X and Y counters.
*		Load special compiler symbols with section lengths.
*		Return YES if OK, NO on error.
*
**/
static
do_cfix ()
{
	struct slist *global, *xgbl, *ygbl, *lkupsec();
	struct nlist xsym, *xsize, ysym, *ysize, *lkupsym();
	static char xsecnam[] = "XGBL";
	static char ysecnam[] = "YGBL";
	static char xsymnam[] = "XSIZE";
	static char ysymnam[] = "YSIZE";

	if ((global = lkupsec ("GLOBAL")) == NULL)
		return (NO);

	if ((xsize = lkupsym (xsymnam)) == NULL) {
		xsym.name = xsymnam;
		xsym.def.xdef.ext = xsym.def.xdef.high =
			xsym.def.xdef.low = 0L;
		xsym.attrib = INT | GLOBAL;
		xsym.mapping = NONE;
		xsym.sec = global;
		xsym.next = NULL;
		if (!instlsym (&xsym) ||
		    (xsize = lkupsym (xsymnam)) == NULL)
			return (NO);
	}

	if ((ysize = lkupsym (ysymnam)) == NULL) {
		ysym.name = ysymnam;
		ysym.def.xdef.ext = ysym.def.xdef.high =
			ysym.def.xdef.low = 0L;
		ysym.attrib = INT | GLOBAL;
		ysym.mapping = NONE;
		ysym.sec = global;
		ysym.next = NULL;
		if (!instlsym (&ysym) ||
		    (ysize = lkupsym (ysymnam)) == NULL)
			return (NO);
	}

	if ((xgbl = lkupsec (xsecnam)) != NULL)
		xsize->def.xdef.low += xgbl->cntrs[XMEM-1][DCNTR] +
				       Start[XMEM-1][DCNTR];
	if ((ygbl = lkupsec (ysecnam)) != NULL)
		ysize->def.xdef.low += ygbl->cntrs[YMEM-1][DCNTR] +
				       Start[XMEM-1][DCNTR];

	return (YES);
}

/*
*
* name 		free_files - release link file list
*
* description	Releases all memory allocated to link file list.
*
**/
free_files()
{
	struct file *fp, *op;
	struct module *mp, *xp;

	fp = Files;
	while (fp) {
		if (fp->name)
			fake_free(fp->name);
		mp = fp->modlst;
		while (mp) {
			fake_free((char *)mp->secmap);
			xp = mp->next;
			fake_free((char *)mp);
			mp = xp;
		}
		op = fp->next;
		fake_free((char *)fp);
		fp = op;
	}
	Last_sec = NULL;
	Sectot = 0;
}

