/* %W% %G% */	/* SCCS data */

/**
*	LNKDEF - DSP56000 Linker Definitions
**/

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

/**
*	Environment Definitions (zero here; may be redefined at compile time)
**/

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
#ifndef AZTEC
#define AZTEC           0               /* Aztec C (Macintosh) */
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
*	Global Includes
**/

#include <stdio.h>
#include <ctype.h>
#if VMS
#include <descrip.h>
#endif
#if LSC
#include <setjmp.h>
#endif

/**
*	Constant and Flag Definitions
**/

#define YES     ((int)1)
#define NO      ((int)0)
#define	NONE	((int)0)
#define ANY	((int)0)
#define ERR     ((int)(-1))
#define DEFAULT	((int)0)

#define ALPHAN(c)	(isalnum(c) || (c=='_'))
#define STREQ(a,b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

#if BSD
#define strchr	index
#define strrchr	rindex
#endif

#if AZTEC               /* text file routine redefintion for Aztec */
#ifdef fgetc
#undef fgetc
#endif
#define fgetc   agetc
#ifdef fputc
#undef fputc
#endif
#define fputc   aputc
#endif

#define MAXBUF		255	/* longest input line */
#define MAXEXP		512	/* longest expression string */
#define MAXSYM		MAXBUF	/* longest label */
#define MAXSEC		255	/* maximum sections in a module */
#define LSTSYM		16	/* maximum characters of symbol to display */
#define F_LIMIT		2	/* limit of words for fractional listing */
#define BYTEWORD	3	/* number of bytes to a DSP machine word */
#define BYTEBITS	8	/* number of bits in a byte */
#define WORDBITS	(BYTEWORD * BYTEBITS)	/* bits in DSP word */
#define SHORTBITS	(BYTEBITS * 2)	/* number of bits in a short */
#define LONGBITS	(BYTEBITS * 4)	/* number of bits in a long */
#define LWORDBITS	(WORDBITS * 2)	/* bits in DSP long word */
#define AWORDBITS	(LWORDBITS + BYTEBITS)	/* bits in DSP accum. word */
#define MAXVALSIZE	6	/* longest object file data value */
#define MAXOBJLINE	72	/* line length of object file */
#define MAXEXTLEN	3	/* longest filename extension */
#define MAXADDR		0xFFFFL	/* maximum memory address */
#define SIZES		BYTEWORD	/* size of DSP short word */
#define SIZEL		(SIZES + SIZES)	/* size of DSP long word */
#define SIZEA		(SIZEL + 1)	/* size of DSP accumulator word */
#define SIZEI		4	/* long integer size in bytes */
#define SIZEF		8	/* double precision floating point size */
#define PSTART		0x40L	/* start of P memory after interrupts */

#if !VMS
#define CLI_ABSENT	0		/* fake VMS DCL return value */
#define INITERR		((int)(-1))	/* initialization error return */
#else
#define CLI_ABSENT	CLI$_ABSENT	/* real VMS DCL return value */
#define INITOK		0x18008001L	/* generic VAX/VMS OK status */
#define INITERR		0x18008002L	/* generic VAX/VMS error return */
#define VMS_ERROR	((unsigned)2)	/* VAX/VMS error severity level */
#endif

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

/*      Character Constants     */
#define RETURN	0x0D
#define NEWLINE '\n'
#if AZTEC
#define ENDLINE RETURN
#else
#define ENDLINE NEWLINE
#endif
#define TAB     '\t'
#define BLANK   ' '
#define EOS     '\0'	/* end-of-string */
#define NEWREC	'_'	/* new record indicator */
#define LIBHDR	'-'	/* library header indicator */
#define COMCHAR	';'	/* memory file comment character */
#define	STR_DELIM '\''	/* string delimiter */
#define XTR_DELIM '"'	/* special string delimiter */
#define STR_CONCAT '+'	/* string concatenation character */

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

/**
*	Memory space counter offsets (section)
**/
#define MSPACES		4	/* number of DSP56000 memory spaces */
#ifndef NONE
#define NONE		0	/* no memory space attribute */
#endif
#define XMEM		1
#define YMEM		2
#define LMEM		3
#define PMEM		4

/**
*       Memory counter types
**/
#define MCNTRS          3       /* number of location counters */
#define DCNTR           0       /* default location counter */
#define LCNTR           1       /* low memory counter */
#define HCNTR           2       /* high memory counter */

/**
*	Extended word array offsets
**/
#define EWLEN		3	/* extended word length */
#define LWORD		0	/* low word */
#define HWORD		1	/* high word */
#define EWORD		2	/* extension word */

/**
/*	Fractional constants
**/
#define MINFRAC		0xff800000L	/* minimum fraction */
#define MAXFRAC		0x007fffffL	/* maximum fraction */

/*	symbol/section attribute flags 	*/
#define	XSPACE	0x0001	/* X space symbol/section	*/
#define	YSPACE	0x0002 	/* Y space symbol/section	*/
#define	LSPACE	0x0004 	/* L space symbol/section	*/
#define PSPACE	0x0008	/* P space symbol/section	*/
#define	SET	0x0010	/* set symbol			*/
#define	LOCAL	0x0020	/* local symbol			*/
#define	GLOBAL	0x0040 	/* global symbol		*/
#define REL	0x0080	/* relocatable symbol/section	*/
#define	INT	0x0100	/* integer number		*/
#define	FPT	0x0200	/* floating pt number		*/
#define	FRC	0x0400	/* fixed pt fractional number	*/
#define LONG	0x0800	/* long value			*/
#define	RDEFERR	0x8000	/* redefined symbol 		*/
#define RELERR	0x8000	/* relocatable sym/sec error	*/

/*      symbol/section mapping flags    */
#define LOCTR   0x0010  /* low location counter         */
#define HICTR   0x0020  /* high location counter        */
#define SETCTR	0x0080	/* absolute counter setting	*/
#define IMAP    0x0100  /* internal memory              */
#define EMAP    0x0200  /* external memory              */
#define BMAP    0x0400  /* bootstrap memory             */

/**
*	Bit masks
**/
#define WORDMASK	0xFFFFFFL	/* DSP word mask */
#define BYTEMASK	0xFF		/* byte mask */
#define SHRTMASK	0xFFFF		/* short integer mask */
#define WBITMASK	0x800000L	/* DSP high bit mask */
#define BBITMASK	0x80		/* byte high bit mask */
#define MSPCMASK	0x000F		/* symbol memory space mask */
#define MCTRMASK	0x0030		/* counter type mask */
#define MMAPMASK	0x0700		/* memory mapping mask */
#define TYPEMASK	0x0F00		/* value type mask */

/*	symbol cross reference types */
#define	DEFTYPE	1
#define	REFTYPE 2

/*	section attribute flags	*/
#define	SHORTS	1	/* short section */

#define HASHSIZE 101	/* size of hash table (should be prime) */

/**
*
* 	evaluation operators
*
**/
#define	ADD	1
#define	SUB	2
#define	MUL	3
#define	DIV	4
#define	AND	5
#define	OR	6
#define	EOR	7
#define	MOD	8
#define SHFT_L	9
#define SHFT_R	10
#define	LTHAN	11
#define	GTHAN	12
#define	EQUL	13
#define	LTEQL	14
#define	GTEQL	15
#define	NEQUL	16
#define LOGAND	17
#define LOGOR	18
#define BSIZE	19

/*	special addressing modes	*/
#define	MIMMED		10	/* 12 bit short immediate	xxx	*/
#define	SIMMED		11	/* 8 bit short immediate 	xx	*/
#define	NIMMED		12	/* 5 bit short immediate	n	*/
#define	IABSOL		14	/* IO short absolute		@I	*/
#define	MABSOL		15	/* 12 bit short absolute	@@@	*/
#define	SABSOL		16	/* short absolute		@@	*/


/**
* object and memory file record types
**/

#define OBJNULL		0
#define OBJANY		0
#define MEMANY		0
#define OBJSTART	1
#define MEMSTART	1
#define OBJEND		2
#define OBJSECTION	3
#define MEMSECTION	3
#define OBJSYMBOL	4
#define MEMSYMBOL	4
#define OBJDATA		5
#define OBJBLKDATA	6
#define OBJRDATA	7
#define OBJRBLKDATA	8
#define OBJXDEF		9
#define OBJXREF		10
#define OBJLINE		11
#define OBJCOMMENT	12
#define MEMCOMMENT	13
#define MEMBASE		14
#define MEMMAP		15

/**
* map file option record types
**/

#define MAPNOCON	1
#define MAPNOLOC	2
#define MAPNOSEA	3
#define MAPNOSEN	4
#define MAPNOSYN	5
#define MAPNOSYV	6

/**
* file type flags
**/

#define LIB		0x0002

/**
* global structures
**/

struct multval {	/* multiple-precision value structure */
		long	ext, high, low;
		};

union udef {
		double	fdef;		/* floating point value */
		struct	multval	xdef; 	/* multiple-precision value */
		};

struct evres { /* evaluation result */
/*
	NOTE:	If any changes are made to this structure the DSP
		Simulator development team should be notified.  They
		use it in their evaluator code which is called by
		the Single-line Assembler (SASM) to evaluate expressions.
		Any discrepancies will cause inconsistent results
		from SASM!
*/
	int	etype;	/* type of evaluation result */
	int	bcount;	/* byte count of characters in string */
	int	wcount;	/* word count of evaluated string */
	union{
		double	fvalue;	/* floating point value */
		long	*wvalue;/* pointer to string word array */
		struct	multval	xvalue;	/* multiple-precision value */
	}uval;
	int	force;		/* forced size (long,short,etc.) */
	int	fwd_ref;	/* NO = result contains no forward refs */
	int	mspace;		/* memory space attribute */
	int	size;		/* significant size of value in bytes */
	int	rel;		/* YES = relative value */
	int	secno;		/* section number (0 = global, <0 = ext ref)*/
};

struct nlist { /* basic symbol table entry */
        char    *name;		/* symbol name */
	union udef def;		/* symbol value */
	int	attrib;		/* attribute flags */
        int     mapping;        /* mapping flags */
	struct slist *sec;	/* section descriptor ptr */
        struct nlist *next; 	/* next entry in chain */
};

struct slist { /* basic relocatable section table entry */
	char 	*sname;		/* section name */
	int	secno;		/* section number */
	int	flags;		/* bit flags */
	int	cflags[MSPACES][MCNTRS];/* counter flags */
	long	start[MSPACES][MCNTRS];	/* memory start address */
	long	cntrs[MSPACES][MCNTRS];	/* location counters */
	struct	slist *next;	/* next entry in chain */
};

struct smap {	/* section map structure */
	long	offset[MSPACES][MCNTRS];/* section memory offsets */
	struct slist *sec;	/* pointer to section info */
};

struct file {	/* file list */
	char *name;		/* filename */
	int flags;		/* attribute flags */
	struct module *cmod;	/* pointer to current module data */
	struct module *modlst;	/* head of module data list */
	struct file *next;	/* next entry in chain */
};

struct module {	/* module info */
	long offset;		/* module offset */
	int lineno;		/* saved line number */
	int recno;		/* saved record number */
	int flags;		/* module flags */
	struct smap *secmap;	/* section mapping table */
	struct module *next;	/* next in chain */
};

struct rectype {	/* link record type structure */
	char *name;		/* record type name */
	int type;		/* record type */
};

struct symref {		/* symbol reference data */
	char *name;		/* pointer to symbol name */
	struct slist *sec;	/* pointer to section where referenced */
	struct symref *prev;	/* previous entry in chain */
	struct symref *next;	/* next entry in chain */
};

struct symlist {	/* symbol table entry list */
	struct nlist *sym;	/* pointer to symbol table entry */
	struct symlist *next;
};

struct strlst {		/* string list entry */
	char *str;		/* string pointer */
	struct strlst *next;	/* pointer to next entry */
};

#if VMS
union vmscond{		/* VAX VMS condition code */
	unsigned long code;
	struct {
		unsigned		: 0;		/* alignment field */
		unsigned success	: 1;
		unsigned severity	: 2;
		unsigned message	: 12;
		unsigned unique		: 1;
		unsigned facility	: 12;
		unsigned noprint	: 1;
		unsigned control	: 3;
	} fld;
};
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
