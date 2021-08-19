#include "dsplib.h"

#if !VMS

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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#if MPW || AZTEC
#include <osutils.h>
#endif

#else	/* VMS */

#include signal
#include types
#include descrip
#include ssdef
#include stsdef
#include time
#include climsgdef
#include stat
#include fab

#endif /* !VMS */

/*
	NAME
		dsplib - DSP56000 librarian

	SYNOPSIS
		lib56000 -command library [files...]

	DESCRIPTION
		This is the DSP56000 librarian program for combining
		DSP56000 relocatable object modules.  It manages any
		number of object modules in a single library file,
		with sufficient information that members may be selectively
		added, extracted, replaced, or deleted from the library.

	OPTIONS
			-a	add named modules to library
			-c	create library with named modules
			-d	delete named modules from library
			-l	list library module info
			-r	replace named modules in library
			-u	update named modules or add at end
			-x	extract named modules from library

	NOTES
		Based on the archive program in the book Software Tools
		in Pascal by Kernighan and Plauger.

	DIAGNOSTICS
		Error messages are sent to the standard error output when
		files cannot be open, or if the program is interrupted.
		The program will complain if files in the input list cannot
		be found.

	AUTHOR
		Tom Cunningham
		Motorola DSP Operation
		6501 Wm. Cannon Dr. W.
		Austin TX 78735-8598
		UUCP: ...!sun!oakhill!tomc

	HISTORY
		1.0	The beginning.
		1.1	Added end-of-module marker (ASCII 0) for linker
			processing.
		1.2	Add VMS DCL command parsing calls
		1.3	Make -u the default if no option specified;
			issue error on -c if library already exists
		1.4	Added interactive mode
		1.5	Put in extra buffering via setvbuf ()
		2.2	Bumped version number for distribution
*/


char *optarg = NULL;			/* command argument pointer */
int optind = 0;				/* command argument index */

static cmd = UPDATE;			/* librarian command */
static char libstr[MAXSTR] = "";	/* library name buffer */
static char *libname = libstr;		/* library name */
static modcnt = 0;			/* number of module names */
static char **modname = NULL;		/* pointer to module name array */
static char modhdr[] = "-h-";		/* library module header */
static char inlib[MAXFILES];		/* in-library flag array */
static errcnt = 0;			/* error count */
static char prompt[] = "> ";		/* command prompt */
static promptlen = 0;			/* length of command prompt */

static char *tmpath = NULL;		/* TMP env. variable pointer */
static char tmpname[MAXSTR] = "";	/* temporary filename buffer */
static char xtmp[] = "libXXXXXX";	/* temporary filename template */

static FILE *lib = NULL;		/* file pointer to library file */
static FILE *mod = NULL;		/* file pointer to module file */
static FILE *tmp = NULL;		/* file pointer to temporary file */

static char *cpbuf = NULL;		/* pointer to copy buffer */
static cpbufsize = 0;			/* copy buffer size */
static char *ibuf = NULL;		/* input buffer */
static ibufsize = 0;			/* input buffer size */
static char *obuf = NULL;		/* output buffer */
static obufsize = 0;			/* output buffer size */

struct command cmdtbl[] = {		/* interactive command table */
	"?",		1,		HELP,
	"add",		1,		ADD,
	"create",	1,		CREATE,
	"delete",	1,		DELETE,
	"exit",		3,		QUIT,
	"extract",	3,		EXTRACT,
	"help",		1,		HELP,
	"list",		1,		LIST,
	"quit",		1,		QUIT,
	"replace",	1,		REPLACE,
	"update",	1,		UPDATE,
	"xtract",	1,		EXTRACT
};
int ctsize = sizeof (cmdtbl) / sizeof (struct command);

#if VMS					/* VMS DCL cmd line opt descriptors */
static char dclbuf[MAXSTR];
$DESCRIPTOR(arg_desc, dclbuf);
$DESCRIPTOR(lib_desc, "LIBRARY");
$DESCRIPTOR(mod_desc, "MODULE");
$DESCRIPTOR(add_desc, "ADD");
$DESCRIPTOR(com_desc, "COMMAND");
$DESCRIPTOR(cre_desc, "CREATE");
$DESCRIPTOR(del_desc, "DELETE");
$DESCRIPTOR(ext_desc, "EXTRACT");
$DESCRIPTOR(lst_desc, "LIST");
$DESCRIPTOR(rep_desc, "REPLACE");
$DESCRIPTOR(upd_desc, "UPDATE");
#endif

static dclflag = NO;			/* VMS DCL flag */
static interactive = NO;		/* interactive command flag */
static intrupt = NO;			/* interrupt flag */
static jmp_buf cmdenv;			/* command mode setjmp env. buffer */

char Progdef[]   = "dsplib";	/* program default name */
char *Progname   = Progdef;	/* pointer to program name */
char Progtitle[] = "Motorola DSP Librarian";
char Version[]   = "Version 3.0";	/* lib version number */
char Copyright[] = "(C) Copyright Motorola, Inc. 1987, 1988, 1989.  All rights reserved.";
#if MSDOS
extern unsigned char _osmajor;
#endif
#if LSC
jmp_buf Env;
#endif

#if !LSC
main (argc, argv)
#else
_main (argc, argv)
#endif
int argc;
char *argv[];
{
#if BSD
	int onintr (), onsegv ();
#else
	void onintr (), onsegv ();
#endif
	int c;
	char *p, *q;
	char *basename(), *getenv();
#if VMS
	int nocmd = NO;
#else
	int nocmd = YES;
#endif

/*
	set up for signals, save program name, check for command line options
*/

#if UNIX || VMS
	(void)signal (SIGSEGV, onsegv);
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
	dclflag = getenv (p) == NULL;
#endif
#if MSDOS
	if (_osmajor > 2)
#endif
		Progname = p;

#if LINT || MSDOS
        (void)fprintf (stderr, "%s  %s\n%s\n", Progtitle, Version, Copyright);
#endif

/*
	check for environment variables
*/
#if !VAXC
#if TMP
	if ((tmpath = getenv ("TMP")) == NULL)
		if ((tmpath = getenv ("TEMP")) == NULL)
#endif
#endif
			tmpath = "";	/* empty temporary path */

/*
	allocate copy, input, and output buffers
*/

	for (cpbufsize = MAXBUF; cpbufsize >= MINBUF; cpbufsize >>= 1)
		if ((cpbuf = (char *)alloc (cpbufsize)) != NULL)
			break;
	if (cpbufsize < MINBUF)
		fatal ("cannot allocate copy buffer");
#if !LSC
	for (ibufsize = MAXBUF; ibufsize >= MINBUF; ibufsize >>= 1)
		if ((ibuf = (char *)alloc (ibufsize)) != NULL)
			break;
	if (ibufsize < MINBUF)
		fatal ("cannot allocate input buffer");
	for (obufsize = MAXBUF; obufsize >= MINBUF; obufsize >>= 1)
		if ((obuf = (char *)alloc (obufsize)) != NULL)
			break;
	if (obufsize < MINBUF)
		fatal ("cannot allocate output buffer");
#endif

/*
	check if command mode wanted
*/

	if (!dclflag) {
		if (argc < 2)
			command ();
	} else {
#if VMS
		if (cli$present (&com_desc) == CLI$_PRESENT)
			command ();
#endif
	}

	optarg = NULL;		/* command argument pointer */
	optind = 0;		/* command argument index */
	if (!dclflag) {		/* not using VMS DCL */
		if (argc < 2)	/* not enough arguments */
			usage ();
		while ((c = getopt (argc, argv, "AaCcDdLlRrUuXx?")) != EOF) {
			nocmd = NO;
			switch (c) {	/* save command */
			case 'A':
			case 'a':	cmd = ADD;
					break;
			case 'C':
			case 'c':	cmd = CREATE;
					break;
			case 'D':
			case 'd':	cmd = DELETE;
					break;
			case 'L':
			case 'l':	cmd = LIST;
					break;
			case 'R':
			case 'r':	cmd = REPLACE;
					break;
			case 'U':
			case 'u':	cmd = UPDATE;
					break;
			case 'X':
			case 'x':	cmd = EXTRACT;
					break;
			case '?':	usage ();	/* no return */
					break;
			default:	cmd = UPDATE;
					nocmd = YES;
					break;
			}
		}

		if (optind >= argc)	/* no more args? error */
			usage ();
	} else {
#if VMS
		if (cli$present (&add_desc) == CLI$_PRESENT)
			cmd = ADD;
		else if (cli$present (&cre_desc) == CLI$_PRESENT)
			cmd = CREATE;
		else if (cli$present (&del_desc) == CLI$_PRESENT)
			cmd = DELETE;
		else if (cli$present (&ext_desc) == CLI$_PRESENT)
			cmd = EXTRACT;
		else if (cli$present (&lst_desc) == CLI$_PRESENT)
			cmd = LIST;
		else if (cli$present (&rep_desc) == CLI$_PRESENT)
			cmd = REPLACE;
		else if (cli$present (&upd_desc) == CLI$_PRESENT)
			cmd = UPDATE;
		else {
			cmd = UPDATE;	/* update by default */
			nocmd = YES;
		}
#endif
	}

/*
	save library name, argument vector, and count
*/

	if (dclflag)			/* VMS DCL */
		saveargs ();
	else {
		(void)strcpy (libstr, argv[optind++]);/* save library name */
		fixfn (libstr, ".lib");		/* fix up library name */
		modcnt = argc - optind;		/* save argument count */
		modname = &argv[optind];	/* save argv pointer */
	}

	libinit ();			/* initialize inlib array */

/*
	perform the specified command on the library
*/

	if (nocmd && modcnt <= 0)	/* change default if no modules */
		cmd = LIST;

	switch (cmd) {
	case ADD:			/* add modules to library */
	case CREATE:			/* create lib with named members */
	case REPLACE:			/* replace modules in library */
	case UPDATE:	update ();	/* update modules in library */
			break;
	case DELETE:	delmod ();	/* delete modules from library */
			break;
	case EXTRACT:	extract ();	/* extract modules from library */
			break;
	case LIST:	list ();	/* list modules in library */
			break;
	default:	update ();	/* update by default */
			break;
	}

#if LSC
	return (0);
#else
#if !VMS
	exit (0);
#else
	exit (VMSOK);
#endif
#endif
}


static
command ()			/* process interactive commands */
{
	int istty = isatty (fileno (stdin));
	char cmdbuf[MAXSTR];

	interactive = YES;
	promptlen = strlen (prompt);
	(void)setjmp (cmdenv);	/* return here on error */

	for (;;) {		/* command loop */
#if VMS
		if (errcnt)
			fputc ('\n', stderr);
#endif
		cmd = UPDATE;
		libstr[0] = tmpname[0] = EOS;
		libname = libstr;
		modname = NULL;
		modcnt = errcnt = 0;
		libinit ();
		if (istty)
			(void)printf ("%s", prompt);
		if (gets (cmdbuf) == NULL)	/* EOF */
#if LSC
			longjmp (Env, 1);
#else
#if !VMS
			exit (0);
#else
			exit (VMSOK);
#endif
#endif
		switch (cmd = parse_cmd (cmdbuf)) {
			case ADD:
			case CREATE:
			case REPLACE:
			case UPDATE:	update ();
					break;
			case DELETE:	delmod ();
					break;
			case EXTRACT:	extract ();
					break;
			case LIST:	list ();
					break;
			case QUIT:
#if LSC
					longjmp (Env, 1);
#else
#if !VMS
					exit (0);
#else
					exit (VMSOK);
#endif
#endif
			case HELP:
					help ();
					break;
			case NONE:
			default:	break;
		}
	}
}


static
update ()			/* update library file */
{
	int i;
	char *mktmp ();

	if (!mktmp (tmpname))
		fatal ("cannot create temporary file name");
	if (!(tmp = libfopen (tmpname, BINWRITE)))
		fatal2 ("cannot open temporary file %s", tmpname);
	if (cmd == CREATE) {	/* see if library exists */
		if (libfopen (libname, BINREAD))
			fatal2 ("library file %s already exists", libname);
	} else {
		if (cmd == ADD)
			if (modcnt <= 0) /* must specify modules to add */
				fatal ("add requires explicit module names");
		if (!(lib = libfopen (libname, BINREAD)))
			fatal2 ("cannot open library file %s", libname);
		replace (lib, tmp);	/* update existing modules  */
		(void)fclose (lib);
		lib = NULL;
	}
	for (i = 0; i < modcnt; i++)	/* add new modules */
		if (inlib[i]) {
			if (cmd == ADD)
				error2 ("%s already in library", modname[i]);
		} else {
			if (cmd == REPLACE)
				error2 ("%s not in library", modname[i]);
			else {
				add (modname[i], tmp);
				inlib[i] = YES;
			}
		}
	(void)fclose (tmp);
	tmp = NULL;
	if (errcnt == 0)		/* no errors */
		fmove (tmpname, libname); /* replace library */
	else
		fatal2 ("fatal errors - %s not altered", libname);
}


static
delmod ()			/* delete modules from library */
{
	char *mktmp ();

	if (modcnt <= 0)	/* must specify modules to delete */
		fatal ("delete requires explicit module names");
	if (!(lib = libfopen (libname, BINREAD)))
		fatal2 ("cannot open library file %s", libname);
	if (!mktmp (tmpname))
		fatal ("cannot create temporary file name");
	if (!(tmp = libfopen (tmpname, BINWRITE)))
		fatal2 ("cannot open temporary file %s", tmpname);
	replace (lib, tmp);	/* update existing modules  */
	notfound ();
	(void)fclose (lib);
	(void)fclose (tmp);
	lib = tmp = NULL;
	if (errcnt == 0)		/* no errors */
		fmove (tmpname, libname); /* replace library */
	else
		fatal2 ("fatal errors - %s not altered", libname);
}


static
extract ()			/* extract modules from library */
{
	char hdr[MAXSTR];	/* module header buffer */
	char name[MAXSTR];	/* module name buffer */
	long size;		/* module size */

	if (!(lib = libfopen (libname, BINREAD)))
		fatal2 ("cannot open library file %s", libname);
	while (gethdr (lib, hdr) != NULL) {	/* scan library for modules */
		(void)sscanf (hdr, "%*s %s %ld", name, &size);
		if (!modarg (name)) {		/* module not in arg list? */
			(void)fseek (lib, size + 1L, SEEK_CUR); /* skip mod */
			continue;		/* keep looking */
		}
		if (!(mod = libfopen (name, BINWRITE))) {
			error2 ("cannot open module file %s", name);
			(void)fseek (lib, size + 1L, SEEK_CUR); /* skip mod */
			continue;
		}
		lcopy (lib, mod, size);		/* copy module contents */
		(void)fgetc (lib);	/* read end-of-module marker */
		(void)fclose (mod);
		mod = NULL;
	}
	(void)fclose (lib);
	lib = NULL;
	notfound ();		/* list arg modules not found in library */
}


static
list ()				/* list modules in library */
{
	char hdr[MAXSTR];	/* module header buffer */
	char name[MAXSTR];	/* module name buffer */
	long size;		/* module size */

	if (!(lib = libfopen (libname, BINREAD)))
		fatal2 ("cannot open library file %s", libname);
	while (gethdr (lib, hdr) != NULL) {	/* scan library for modules */
		(void)sscanf (hdr, "%*s %s %ld", name, &size);
		if (modarg (name))		/* module in argument list? */
			printhdr (hdr);
		(void)fseek (lib, size + 1L, SEEK_CUR);/* skip to next mod */
	}
	(void)fclose (lib);
	lib = NULL;
	notfound ();		/* list arg modules not found in library */
}


static
replace (lfile, tfile)		/* replace/delete modules in library */
FILE *lfile, *tfile;
{
	char hdr[MAXSTR];	/* module header buffer */
	char name[MAXSTR];	/* module name buffer */
	long size;		/* module size */

	while (gethdr (lfile, hdr) != NULL) {	/* scan library for modules */
		(void)sscanf (hdr, "%*s %s %ld", name, &size);
		if (modarg (name)) {		/* module in argument list? */
			if (cmd != DELETE)
				add (name, tfile);	/* add new module */
			(void)fseek (lfile, size + 1L, SEEK_CUR);/* skip */
		} else {				/* just copy module */
			(void)puthdr (tfile, hdr);	/* copy header */
			lcopy (lfile, tfile, size + 1L);/* copy module */
		}
	}
}


static
add (name, tfile)		/* add module mname to library */
char *name;
FILE *tfile;
{
	char hdr[MAXSTR];	/* module header buffer */

	if (!(mod = libfopen (name, BINREAD)))
		error2 ("cannot open module file %s", name);

	if (errcnt == 0) {	/* no errors -- add module */
		(void)mkhdr (name, hdr); /* make module header */
		(void)puthdr (tfile, hdr);/* write out header */
		fcopy (mod, tfile);	 /* copy file contents */
		(void)fputc (EOM, tfile);/* write end-of-module marker */
		(void)fclose (mod);
		mod = NULL;
	}
}


static
fmove (sfn, dfn)		/* move file from sfn to dfn */
char *sfn, *dfn;
{
#if VMS
	$DESCRIPTOR (src, sfn);
	$DESCRIPTOR (dst, dfn);
#endif
#if !VMS
	if (rename (sfn, dfn) != 0)
		if (force_rename (sfn, dfn) != 0)
#else
	src.dsc$w_length = strlen (sfn);
	dst.dsc$w_length = strlen (dfn);
	if (lib$rename_file (&src, &dst) != SS$_NORMAL)
#endif
			fatal2 ("cannot rename %s", sfn);

	(void)remove (sfn);	/* delete source if necessary */
}


static
force_rename (old, new)		/* do unqualified rename */
char *old, *new;
{
	static char tmpfn[] = "libXXXXXX";
	char *mktemp ();

	if (rename (old, new) == 0)		/* no existing target */
		return (0);			/* go ahead and rename */

	(void)strcpy (tmpfn, xtmp);		/* copy template */
	if (!mktemp (tmpfn))	/* get temporary name for target file */
		fatal ("cannot create temporary file name");
	if (rename (new, tmpfn) != 0)
		return (-1);	/* couldn't rename target file */
	if (rename (old, new) != 0) {
		if (rename (tmpfn, new) != 0)	/* try to put target back */
			fatal2 ("cannot rename %s", tmpfn);
		return (-1);	/* couldn't rename source file */
	}
	(void)remove (tmpfn);	/* delete the old target file */
	return (0);
}


static
fcopy (sfp, dfp)		/* copy file contents from sfp to dfp */
FILE *sfp, *dfp;
{
	int count;		/* read count */

	clearerr (sfp);
	while ((count = fread (cpbuf, 1, cpbufsize, sfp)) > 0)
		if (fwrite (cpbuf, 1, count, dfp) != count)
			fatal ("error writing file");
	if (ferror (sfp) && !feof (sfp))
		fatal ("error reading file");
}


static
lcopy (sfp, dfp, len)		/* copy len chars from sfp to dfp */
FILE *sfp, *dfp;
long len;
{
	long times;		/* buffers to read */
	int rem;		/* remaining bytes */
	long l;

	times = len / (long)cpbufsize;
	rem = (int)(len % (long)cpbufsize);
	for (l = 0L; l < times; l++)
		if ( fread (cpbuf, 1, cpbufsize, sfp) != cpbufsize ||
		     fwrite (cpbuf, 1, cpbufsize, dfp) != cpbufsize )
			fatal ("file i/o error");
	if ( fread (cpbuf, 1, rem, sfp)  != rem ||
	     fwrite (cpbuf, 1, rem, dfp) != rem )
		fatal ("file i/o error");
}


static
libinit ()		/* perform array initialization, consistency checks */
{
	register i, j;

/*
	clear boolean in-library array
*/

	for (i = 0; i < MAXFILES; i++)
		inlib[i] = NO;

/*
	check for duplicate module names on command line
*/

	for (i = 0; i < modcnt - 1; i++)
		for (j = i + 1; j < modcnt; j++)
			if (strcmp (modname[i], modname[j]) == 0)
				error2 ("duplicate module name %s",
					modname[i]);
}


static FILE *
libfopen (fn, mode)		/* open buffered file */
char *fn, *mode;
{
	FILE *fp;
	char *buf;
	int bufsize;

	if ((fp = fopen (fn, mode)) == NULL)
		return (NULL);
#if !VMS && !LSC && !APOLLO && !GCC && !NeXT
	if (strcmp (mode, BINREAD) == 0) {
		buf = ibuf;
		bufsize = ibufsize;
	} else if (strcmp (mode, BINWRITE) == 0) {
		buf = obuf;
		bufsize = obufsize;
	} else
		return (fp);
	setvbuf (fp, buf, _IOFBF, bufsize);
#endif
	return (fp);
}


static
closefiles ()			/* close any open files; delete temps */
{
	if (lib)
		(void)fclose (lib);
	if (mod)
		(void)fclose (mod);
	if (tmp)
		(void)fclose (tmp);
	if (tmpname)
		(void)remove (tmpname);
}


static char *
mkhdr (name, hdr)		/* create module header for file name */
char *name, *hdr;
{
	char *p, *q;
#if VMS || LSC || MPW || AZTEC
	long filesize ();
#else
	struct stat stbuf;	/* file info buffer */
#endif
	long fsize;
	extern time_t time ();

#if VMS || LSC || MPW || AZTEC
	if ((fsize = filesize (name)) < 0)
#else
	if (stat (name, &stbuf) >= 0)
		fsize = stbuf.st_size;
	else
#endif
		fatal2 ("cannot stat module %s", name);
	p = strlwr (basename (name));
	if ((q = strrchr (p, ';')) != NULL)
		*q = EOS;
	(void)sprintf (hdr, "%s %s %ld %ld",
		modhdr, p, fsize, time ((time_t *)0));
	return (hdr);
}


#if VMS || LSC || MPW || AZTEC
/**
*
* name		filesize - return size of file in bytes
*
* synopsis	count = filesize (name);
*		long count;	count of bytes in file; -1L on error
*		char *name;	name of file
*
* description	This is a kludge to get around a problem with VAX C and RMS
*		file types.  It uses I/O instead of stat() to get the
*		file length, thereby taking into account translations done
*		by RMS.  In this way just about any file can be stored in
*		a library, but it will be a VAX C file when it is extracted!
*
*		Extended to support Macintosh, which apparently does not
*		have stat(), but does have fseek() and ftell().
*
*		Modified with VMS 4.6 to use stat(), but restricts access
*		to native VAX C files, e.g. Stream_LF files with carriage
*		return carriage control.  This was done because apparently
*		the RMS file translation code underlying the standard I/O
*		routines is a moving target.
*
**/
static long
filesize (name)
char *name;
{
#if VMS
	struct stat stbuf;
#else
	FILE *fp;
	long count;
#endif

#if VMS
	if (stat (name, &stbuf) < 0)
		return (-1L);

	if (stbuf.st_fab_rfm != FAB$C_STMLF ||
	    stbuf.st_fab_rat != FAB$M_CR) {
		error ("invalid RMS record format for file ", name);
		return (-1L);		/* invalid file type */
	}
	return ((long)stbuf.st_size);
#else
	if ((fp = libfopen (name, BINREAD)) == NULL)
		return (-1L);

	(void)fseek (fp, 0L, SEEK_END);
	count = ftell (fp);

	(void)fclose (fp);
	return (count);
#endif
}
#endif /* VMS || LSC || MPW || AZTEC */



static char *
gethdr (lfile, hdr)		/* get header info from lib into hdr */
FILE *lfile;
char *hdr;
{
	register char *p;
	int c;

	clearerr (lfile);	/* clear error status */
	for (p = hdr;
	     (c = fgetc (lfile)) != EOF && c != RETURN && c != NEWLINE;
	     p++)
		*p = c;		/* read header into buffer */
	*p = EOS;
	if (feof (lfile))
		return (NULL);	/* end of file */
	else if (ferror (lfile))
		fatal ("cannot read module header");
	if (c == RETURN && (c = fgetc (lfile)) != EOF && c != NEWLINE)
		ungetc (c, lfile);
	if (strncmp (hdr, modhdr, sizeof (modhdr) - 1) != 0)
		fatal ("improper module header format");
	return (hdr);
}


static
puthdr (lfile, hdr)		/* put header info from hdr into lib */
FILE *lfile;
char *hdr;
{
	register char *p;

	for (p = hdr; *p; p++)
		fputc (*p, lfile);
#if MAC || MSDOS
	fputc (RETURN, lfile);
#endif
#if !MAC
	fputc (NEWLINE, lfile);
#endif
}


static
printhdr (hdr)			/* display module header info */
char *hdr;
{
	char name[MAXSTR];	/* module name buffer */
	long size;		/* module size */
	time_t clock;		/* module time */
	struct tm *tm;		/* time field structure */
	struct tm *localtime ();

	(void)sscanf (hdr, "%*s %s %ld %ld", name, &size, &clock);
	tm = localtime (&clock);
	(void)printf ("%-*s %10ld    %02d/%02d/%02d    %02d:%02d:%02d\n",
		MNAMLEN, name, size,
		tm->tm_mon+1, tm->tm_mday, tm->tm_year,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}


static
modarg (name)			/* see if name is in argument list */
char *name;
{
	int i, found = NO;
	char modbuf[MAXSTR];
	char *basename ();
#if VMS
	char *p;
#endif

	if (modcnt <= 0)	/* no modules specified */
		return (YES);

	for (i = 0; !found && i < modcnt; i++) {
		(void)strcpy (modbuf, basename (modname[i]));
#if VMS
		if ((p = strrchr (modbuf, ';')) != NULL)
			*p = EOS;
#endif
		if (strcmp (name, modbuf) == 0)
			found = inlib[i] = YES;
	}
	return (found);
}


static
notfound ()			/* check arg list for modules not in lib */
{
	int i;

	for (i = 0; i < modcnt; i++)
		if (!inlib[i])		/* module not in library */
			error2 ("%s not in library", modname[i]);
}


static
fixfn (fn, ext)			/* add ext to fn if necessary */
char *fn, *ext;
{
	int extlen, lower = NO;
	char extbuf[EXTLEN+2];
	char *p, *np, *ep, *op, *basename();

	extlen = strlen (ext);		/* get extension length */
	p = basename (fn);		/* get file base name */
	np = fn + strlen (fn);		/* compute end of string */
	ep = strrchr (fn, '.');		/* find file extension */

	if( !ep || ep < p ){		/* no extension supplied */
		(void)strncpy (extbuf, ext, EXTLEN+2);
		for (op = p; *op; op++)	/* check for lowercase in filename */
			if (islower (*op)) {
				lower = YES;
				break;
			}
		if (!lower)		/* convert extension to upper case */
			for (op = extbuf; *op; op++)
				if (islower (*op))
					*op = toupper (*op);
		(void)strcpy (np, extbuf);	/* copy extension */
		ep = np;
		np += extlen;
	}

	if (np - p > BASENAMLEN) {	/* filename too long -- truncate */
		np = p + (BASENAMLEN - extlen);
		p = np;
		while (*ep && np < p + EXTLEN + 1)
			*np++ = *ep++;
		*np = EOS;
	}
}


static char *
basename (str)			/* return base part of file name in str */
char *str;
{
	register char *p;

	if (!str)		/* empty input */
		return (NULL);

	for (p = str + strlen (str); p >= str; --p)
#if MSDOS
		if( *p == '\\' || *p == ':')
#endif
#if VMS
		if( *p == ']' || *p == ':')
#endif
#if UNIX
		if( *p == '/' )
#endif
#if MAC
		if( *p == ':' )
#endif
			break;

	return (p < str ? str : ++p);
}


static char *
strlwr (str)			/* convert chars in str to lower case */
char *str;
{
	register char *p;

	for (p = str; *p; p++)
		if (isupper (*p))
			*p = tolower (*p);
	return (str);
}

#if BSD
static
#else
static void
#endif
onintr ()			/* clean up from signal */
{
	intrupt = YES;
	fatal ("Interrupted");
}


#if BSD
static
#else
static void
#endif
onsegv ()			/* handle segmentation/protection faults */
{
	(void)fprintf (stderr, "%s: Fatal segmentation or protection fault\n", Progname);
	(void)fprintf (stderr, "%s: Contact Motorola DSP Operation\n", Progname);
#if LSC
	longjmp (Env, ERR);
#else
#if !VMS
	exit (1);
#else
	exit (VMSERR);
#endif
#endif
}

static
parse_cmd (cmdstr)		/* parse interactive command line */
char *cmdstr;
{
	struct module modlst, *mp, *xp;
	char *cp, **np, *p;
	int cmdtype;
	char buf[MAXSTR];

#if MAC
	/* Strip off prompt that MPW puts into buffer */
	if (strncmp (cmdstr, prompt, promptlen) == 0)
		cmdstr += promptlen;
#endif
	for (cp = cmdstr; *cp && isspace (*cp); cp++)
		;			/* skip leading whitespace */
	if (!*cp)
		return (NONE);		/* no command present */
	for (p = buf; *cp && !isspace (*cp); p++, cp++)
		*p = isupper (*cp) ? tolower (*cp) : *cp;
	*p = EOS;
	if ((cmdtype = getcmd (buf)) == NONE)
		return (NONE);		/* invalid command */

	while (*cp && isspace (*cp))
		cp++;			/* skip over whitespace */
	if (!*cp)			/* no library name present */
		switch (cmdtype) {
			case HELP:
			case QUIT:	return (cmdtype);
			default:
					error ("command requires library name");
					return (NONE);
		}
	for (p = libstr; *cp && !isspace (*cp); p++, cp++)
		*p = *cp;		/* copy library name */
	*p = EOS;
	fixfn (libstr, ".lib");		/* fix up library name */

/*
	scan command line and put module names in a linked list
*/

	modlst.name = NULL;
	modlst.next = NULL;
	mp = &modlst;
	modcnt = 0;
	for (;;) {
		while (*cp && isspace (*cp))
			cp++;		/* skip over whitespace */
		if (!*cp)
			break;		/* end of command line */
		for (p = buf; *cp && !isspace (*cp); p++, cp++)
			*p = *cp;	/* copy module name into buffer */
		*p = EOS;
		if ((xp = (struct module *)alloc (sizeof (struct module))) == NULL || (p = (char *)alloc (strlen (buf) + 1)) == NULL)
			fatal ("cannot allocate module structure");
		(void)strcpy (p, buf);
		xp->name = p;
		mp = mp->next = xp;
		mp->next = NULL;
		modcnt++;
	}

/*
	allocate a pointer array and copy the module name pointers
*/

	if (!modcnt)
		modname = NULL;
	else {
		if ((modname = (char **)alloc (sizeof (char *) * modcnt)) == NULL)
			fatal ("cannot allocate module vector");
		for (mp = modlst.next, np = modname; mp; np++) {
			*np = strlwr (mp->name);
			xp = mp;
			mp = mp->next;
			free ((char *)xp);
		}
	}

	return (cmdtype);
}


static
getcmd (cmdstr)			/* lookup interactive command type */
char *cmdstr;
{
	register i;
	register struct command *cp;
	int cmdlen = strlen (cmdstr);

	for (i = 0; i < ctsize; i++) {
		cp = &cmdtbl[i];
		if (strncmp (cmdstr, cp->name, cmdlen) == 0)
			if (cmdlen >= cp->unique)
				return (cp->type);
			else {
				error2 ("ambiguous command %s", cmdstr);
				return (NONE);
			}
	}
	error2 ("invalid command %s", cmdstr);
	return (NONE);
}


static
saveargs ()			/* save VMS DCL command arguments */
{
#if VMS
	$DESCRIPTOR (instr, NULL);
	$DESCRIPTOR (outstr, NULL);
	unsigned context;
	struct module modlst, *mp, *xp;
	char **np, *p;
	char xfnbuf[MAXSTR];

	dcl_getval (&lib_desc);			/* get library name */
	(void)strcpy (libstr, dclbuf);		/* and save it */
	fixfn (libstr, ".lib");			/* fix up library name */

/*
	scan DCL command line and put module names in a linked list
*/

	modlst.name = NULL;
	modlst.next = NULL;
	mp = &modlst;
	modcnt = 0;
	while (dcl_getval (&mod_desc) != CLI$_ABSENT)
		if (cmd == DELETE || cmd == LIST || cmd == EXTRACT) {
			if ((xp = (struct module *)alloc (sizeof (struct module))) == NULL || (p = (char *)alloc (strlen (dclbuf) + 1)) == NULL)
				fatal ("cannot allocate module structure");
			(void)strcpy (p, dclbuf);
			xp->name = p;
			mp = mp->next = xp;
			mp->next = NULL;
			modcnt++;
		} else {
			instr.dsc$a_pointer  = dclbuf;
			instr.dsc$w_length   = strlen (dclbuf);
			outstr.dsc$a_pointer = xfnbuf;
			outstr.dsc$w_length  = MAXSTR;
			context = 0;
	                while (lib$find_file (&instr, &outstr, &context)
			    & STS$M_SUCCESS) {
				for (p = xfnbuf; *p && !isspace (*p); p++)
					;
				*p = EOS;	/* strip trailing blanks */
				if ((xp = (struct module *)alloc (sizeof (struct module))) == NULL ||
				    (xp->name = (char *)alloc (strlen (xfnbuf) + 1)) == NULL)
					fatal ("cannot allocate module structure");
				strcpy (xp->name, xfnbuf);
				mp = mp->next = xp;
				mp->next = NULL;
				modcnt++;
			}
			lib$find_file_end (&context);
		}

/*
	allocate a pointer array and copy the module name pointers
*/

	if ((modname = (char **)alloc (sizeof (char *) * modcnt)) == NULL)
		fatal ("cannot allocate module vector");
	for (mp = modlst.next, np = modname; mp; np++) {
		*np = strlwr (mp->name);	/* convert to lower case */
		xp = mp;
		mp = mp->next;
		free ((char *)xp);
	}
#endif
}


getopt (argc, argv, optstring)	/* get option letter from argv */
int argc;
char *argv[];
char *optstring;
{
	register char c;
	register char *place;
	static char *scan = NULL;
	extern char *index();

	optarg = NULL;

	if (scan == NULL || *scan == '\0') {
		if (optind == 0)
			optind++;
	
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		if (strcmp(argv[optind], "--")==0) {
			optind++;
			return(EOF);
		}
	
		scan = argv[optind]+1;
		optind++;
	}

	c = *scan++;
	place = index(optstring, c);

	if (place == NULL || c == ':') {
		(void)fprintf(stderr, "%s: unknown option -%c\n", argv[0], c);
		return('?');
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			optarg = scan;
			scan = NULL;
		} else if (optind < argc) {
			optarg = argv[optind];
			optind++;
		} else {
			(void)fprintf( stderr, "%s: -%c argument missing\n", 
					argv[0], c);
			return( '?' );
		}
	}

	return(c);
}


static char *
mktmp (tmpnm)		/* generate temporary file name */
char *tmpnm;
{
	static char tmpfn[] = "libXXXXXX";
	register char *p;
	char *mktemp ();

	(void)strcpy (tmpfn, xtmp);
	(void)strcpy (tmpnm, tmpath);
	for (p = tmpnm; *p; p++)
		;
	if (p > tmpnm)
		--p;
#if UNIX
	if (*p && *p != '/')
		*++p = '/';
#endif
#if MSDOS
	if (*p && *p != ':' && *p != '\\')
		*++p = '\\';
#endif
#if VMS
	if (*p && *p != ':' && *p != ']')
		*++p = ':';
#endif
#if MAC
	if (*p && *p != ':')
		*++p = ':';
#endif
	if (p > tmpnm)
		*++p = EOS;

	if (!mktemp (strcat (tmpnm, tmpfn)))
		fatal ("cannot create temporary file name");

	return (tmpnm);
}


#if LSC
/**
*
* name		mktemp - create temporary file name
*
* synopsis	fn = mktemp (template)
*		char *fn;		pointer to temporary file name
*		char *template;		template for temporary file name
*
* description	Creates a temporary file name based on template.  Template
*		has the form yyyXXXXXX, where yyy is arbitrary text and
*		the literal XXXXXX substring is replaced by a letter
*		and a five digit number.  If a duplicate identifier is
*		formed in the same process, the routine substitutes the
*		next alphabetic character in sequence for the leading letter.
*		Returns pointer to revised template on success, NULL if
*		template is badly formed or other error.
*
**/
static char *
mktemp (template)
char *template;
{
	static char oldfn[MAXSTR] = "";
	static char subpat[] = "XXXXXX";
	char *p, *subp;
	char tbuf[32];
	long t;
	int i;
	extern time_t time ();

	if ((subp = strchr (template, subpat[0])) == NULL)
		return (NULL);

	if (strcmp (subp, subpat) != 0)
		return (NULL);

	(void)time ((time_t *)&t);
	if (t < 0L)
		t *= -1L;
	sprintf (tbuf, "%05ld", t);
	for (i = strlen (subpat) - 1, p = tbuf + strlen (tbuf);
	     i > 0;
	     i--, p--)
		;
	i = 'a';
	do {
		sprintf (subp, "%c%s", i++, p);
	} while (i <= 'z' && strcmp (oldfn, template) == 0);
	if (i > 'z')
		return (NULL);

	strcpy (oldfn, template);
	return (template);
}
#endif


#if VMS
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
*		are returned in the global dclbuf array.  The length returned
*		from cli$get_value is used to terminate the string.
*
**/
static
dcl_getval (opt)
struct dsc$descriptor_s *opt;
{
	unsigned status, len = 0;
	
	status = cli$get_value (opt, &arg_desc, &len);
	if (status != CLI$_ABSENT)
		dclbuf[len] = EOS;
	return (status);
}
#endif


#if VMS
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
	return (cli$dcl_parse(NULL, NULL) == CLI$_NORMAL);
}
#endif


static
usage ()			/* display usage on stderr, exit nonzero */
{
#if !MSDOS
        (void)fprintf (stderr, "%s  %s\n%s\n", Progtitle, Version, Copyright);
#endif
	(void)fprintf (stderr, "Usage:  %s -command library [files...]\n",
		Progname);
	(void)fprintf (stderr, "where -command is one of the following:\n");
	(void)fprintf (stderr, "        -a        add named modules to library\n");
	(void)fprintf (stderr, "        -c        create library with named modules\n");
	(void)fprintf (stderr, "        -d        delete named modules from library\n");
	(void)fprintf (stderr, "        -l        list library module info\n");
	(void)fprintf (stderr, "        -r        replace named modules in library\n");
	(void)fprintf (stderr, "        -u        update named modules or add at end\n");
	(void)fprintf (stderr, "        -x        extract named modules from library\n");
#if LSC
	longjmp (Env, ERR);
#else
#if !VMS
	exit (1);
#else
	exit (VMSERR);
#endif
#endif
}


static
help ()			/* display help message */
{
	(void)printf ("Usage: command library [files...]\n");
	(void)printf ("where  command is one of the following:\n");
	(void)printf ("    add     - add named modules to library\n");
	(void)printf ("    create  - create library with named modules\n");
	(void)printf ("    delete  - delete named modules from library\n");
	(void)printf ("    extract - extract named modules from library\n");
	(void)printf ("    help    - display this message\n");
	(void)printf ("    list    - list library module info\n");
	(void)printf ("    quit    - exit librarian\n");
	(void)printf ("    replace - replace named modules in library\n");
	(void)printf ("    update  - update named modules or add at end\n");
}


static double *
alloc (bytes)
int bytes;
{
	char *malloc ();

#if !LINT
	return ((double *)malloc ((unsigned)bytes));
#else
	bytes = bytes;
	return ((double *)NULL);
#endif
}


#if MPW

/* The following routines were adapted from Lightspeed C unixtime.c */

/* This routine returns the time since Jan 1, 1970 00:00:00 GMT */

time_t time(clock)
time_t *clock;
{
	time_t RawTime, GMTtimenow;
	void GetDateTime ();

	GetDateTime (&RawTime);
	GMTtimenow = RawTime-TimeBaseDif+GMTzonedif;

	if (clock)
		*clock = GMTtimenow;
		
	return (GMTtimenow);
}

struct tm *localtime(clock)
register time_t *clock;
{
DateTimeRec MacTimeRec;
static struct tm UnixTimeRec;
register int dayofyear=0, i;

	time_t RawTime, GMTtimenow;

	if (!clock)
		GetDateTime (&RawTime);

	Secs2Date(clock?(*clock+TimeBaseDif-GMTzonedif):RawTime,&MacTimeRec);

	UnixTimeRec.tm_sec	= (MacTimeRec.second);
	UnixTimeRec.tm_min	= (MacTimeRec.minute);
	UnixTimeRec.tm_hour	= (MacTimeRec.hour);
	UnixTimeRec.tm_mday	= (MacTimeRec.day);
	UnixTimeRec.tm_mon	= (MacTimeRec.month-1);	/* UNIX uses 0-11 not 1-12 */
	UnixTimeRec.tm_year	= (MacTimeRec.year-1900);
	UnixTimeRec.tm_wday	= (MacTimeRec.dayOfWeek-1); /* UNIX uses 0-6 not 1-7 */

	for(i=0; i<UnixTimeRec.tm_mon; i++)
	{
	static char monthdays[11] = {31,28,31,30,31,30,31,31,30,31,30};

		/* check for leap year in Feb */
		if ((i==1) && !(MacTimeRec.year % 4) && !(MacTimeRec.year % 100))
			dayofyear++;
			
		dayofyear += monthdays[i];
	}
	
	UnixTimeRec.tm_yday = (dayofyear + MacTimeRec.day-1);

	UnixTimeRec.tm_isdst = 0;	/* don't know if daylight savings */

	return(&UnixTimeRec);

}

#endif /* MPW */

#if MPW || AZTEC
getpid ()			/* fake a process ID for MPW and Aztec */
{
	time_t time ();

	return ((unsigned)time(0));
}
#endif /* MPW || AZTEC*/

static
error (str)
char *str;
{
	errcnt++;
	if (!interactive)
		(void)fprintf (stderr, "%s: ", Progname);
	(void)fprintf (stderr, "%s\n", str);
}


static
error2 (fmt, str)
char *fmt, *str;
{
	errcnt++;
	if (!interactive)
		(void)fprintf (stderr, "%s: ", Progname);
	(void)fprintf (stderr, fmt, str);
	(void)fprintf (stderr, "\n");
}


static
fatal (str)
char *str;
{
	(void)closefiles ();
#if AZTEC
	SysBeep ();
#else
	(void)fprintf (stderr, "\007");
#endif
	(void)error (str);
	if (interactive && !intrupt)
		longjmp (cmdenv, -1);
#if LSC
	longjmp (Env, ERR);
#else
#if !VMS
	exit (1);
#else
	exit (VMSERR);
#endif
#endif
}


static
fatal2 (fmt, str)
char *fmt, *str;
{
	(void)closefiles ();
#if AZTEC
	SysBeep ();
#else
	(void)fprintf (stderr, "\007");
#endif
	(void)error2 (fmt, str);
	if (interactive && !intrupt)
		longjmp (cmdenv, -1);
#if LSC
	longjmp (Env, ERR);
#else
#if !VMS
	exit (1);
#else
	exit (VMSERR);
#endif
#endif
}
