/*
	DSPLIB - DSP Librarian Definitions
*/


/*
	General-purpose definitions
*/

#define YES		1
#define NO		0
#define NONE		0
#define ERR		(-1)
#define EOS		'\0'		/* end-of-string constant */
#define EOM		'\0'		/* end-of-module constant */
#define RETURN		0x0D		/* ASCII carriage return */
#define NEWLINE		0x0A		/* ASCII line feed */
#ifdef	DEBUG
#undef	DEBUG
#endif
#define DEBUG		0		/* Debug flag */
#ifndef LINT
#define LINT		0		/* Lint flag */
#endif

/*
* The following definitions are for Macintosh since that environment
* does not support command line DEFINEs; remove comments as necessary.
*/
/*
#define MAC		1
#define LSC		1
#define M68020		1
*/

/*
	Environment Definitions (zero here; may be redefined at compile time)
*/

#ifndef MSDOS
#define MSDOS		0		/* MS-DOS operating system */
#endif
#ifndef VMS
#define VMS		0		/* VMS operating system */
#endif
#ifndef ATT
#define ATT		0		/* AT&T Unix */
#endif
#ifndef BSD
#define BSD		0		/* Berkeley Unix */
#endif
#ifdef UNIX
#undef UNIX
#endif
#define UNIX		(ATT || BSD)	/* generic Unix operating system */
#ifndef MAC
#define MAC		0		/* Macintosh operating system */
#endif
#ifndef APOLLO
#define APOLLO		0		/* Apollo system */
#endif

#ifndef AZTEC
#define AZTEC		0		/* Aztec C compiler */
#endif
#ifndef LATTICE
#define LATTICE		0		/* Lattice C compiler */
#endif
#ifndef MSC
#define MSC		0		/* Microsoft C compiler */
#endif
#ifndef VAXC
#define VAXC		0		/* VAX C compiler */
#endif
#ifndef LSC
#define LSC		0		/* Lightspeed C compiler */
#endif
#ifndef MPW
#define MPW		0		/* MPW C (Macintosh) */
#endif
#ifndef GCC
#define GCC		0		/* GNU C compiler */
#endif
#ifndef PCC
#define PCC		0		/* Portable C (generic Unix) */
#endif

#ifndef I8086
#define I8086		0		/* Intel 8086/88 CPU */
#endif
#ifndef M68010
#define M68010		0		/* Motorola 68010 CPU */
#endif
#ifndef M68020
#define M68020		0		/* Motorola 68020 CPU */
#endif
#ifndef VAX
#define VAX		0		/* DEC VAX CPU */
#endif

/**
*       Global Includes
**/

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

/*
	Length constant definitions
*/

#define MAXSTR		256		/* maximum string size + 1 */
#define MAXFILES	2000		/* maximum files in library */
#define MNAMLEN		15		/* module name display length */
#define EXTLEN		3		/* filename extension length */
#define MINBUF		512		/* minimum copy buffer size */
#define MAXBUF		8192		/* maximum copy buffer size */

/*	Base filename lengths	*/
#if MSDOS
#define BASENAMLEN	12
#endif
#if ATT
#define BASENAMLEN	14
#endif
#if BSD
#define BASENAMLEN	255
#endif
#if VMS
#define	BASENAMLEN	78
#endif
#if MAC
#define BASENAMLEN	31
#endif

/*
	Command constant definitions
*/

#define ADD		1		/* add modules to library */
#define CREATE		2		/* create lib with named modules */
#define	DELETE		3		/* delete modules from library */
#define	EXTRACT		4		/* extract modules from library */
#define	LIST		5		/* list modules in library */
#define REPLACE		6		/* replace modules in library */
#define UPDATE		7		/* update named mods or add at end */
#define QUIT		8		/* exit librarian */
#define HELP		9		/* interactive help */

/*
	File seek definitions
*/

#ifndef SEEK_SET
#define SEEK_SET	0		/* seek from beginning of file */
#define SEEK_CUR	1		/* seek from current pos. in file */
#define SEEK_END	2		/* seek from end of file */
#endif

/*
	File open modes
*/

#if MSC || LSC
#define BINREAD		"rb"		/* binary read */
#define BINWRITE	"wb"		/* binary write */
#else
#define BINREAD		"r"
#define BINWRITE	"w"
#endif

struct module {		/* module list structure */
	char *name;
	struct module *next;
};

struct command {	/* interactive command table */
	char *name;
	int unique;
	int type;
};

#if VMS
#define VMSOK		0x18008001L
#define VMSERR		0x18008002L
#define remove	delete
#endif

#if BSD || AZTEC
extern char *index (), *rindex (), *sprintf ();
#define strchr	index
#define strrchr rindex
#define remove	unlink
#else
extern char *strchr (), *strrchr ();
#define index	strchr
#define rindex	strrchr
#endif

#if MPW || AZTEC
typedef unsigned long time_t;
#endif
#if LSC
#define time_t unsigned long
#endif

#if MPW
/* Adapted from Lightspeed C unixtime.c */

/* The Macintosh keeps a raw seconds count that begins at
	00:00:00 January 1, 1904 local time,
	UNIX uses 00:00:00 January 1, 1970 GMT */

/* seconds difference between EST & GMT time zones */
#define GMTzonedif	(5*60*60)

#define	TMacbaseyr	1904
#define	TUNIXbaseyr	1970

/* number of leap days between the two years -- Mac base was a leap year! */
#define	TLpD	((TUNIXbaseyr-TMacbaseyr-1)/4)

/* TimeBaseDif is the number of seconds between Mac and UNIX time (GMT) */
#define TimeBaseDif	((((TUNIXbaseyr-TMacbaseyr)*365)+TLpD)*24*60*60)

/* End of Lightspeed defines */
#endif /* MPW */

#if MPW || AZTEC
/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};
#endif /* MPW || AZTEC */

#if MSDOS
extern _fmode;				/* file mode flag */
#endif
char *mkhdr ();				/* make library header routine */
char *gethdr ();			/* get library header routine */
char *basename ();			/* return file base name routine */
char *strlwr ();			/* convert string to lower case */
double *alloc ();			/* max. align for lint */
FILE *libfopen ();			/* special buffered file open */
extern char *malloc (), *strcpy (), *strncpy (), *strcat (), *mktemp ();
extern char *gets ();
extern long ftell (), lseek ();
extern char Progdef[];
extern char *Progname;
extern char Progtitle[];
extern char Version[];
extern char Copyright[];
extern jmp_buf Env;
