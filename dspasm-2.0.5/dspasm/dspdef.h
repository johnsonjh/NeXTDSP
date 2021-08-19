/* %W% %G% */	/* SCCS data */

/**
*	DSPDEF - DSP56000 Assembler Definitions
**/

#ifdef	DEBUG
#undef	DEBUG
#endif
#define DEBUG		0		/* Debug flag */

#ifndef LINT
#define LINT		0		/* Lint flag */
#endif

/*
* The following definitions are for Macintosh Lightspeed C since that
* environment does not support command line DEFINEs.  Remove comments
* as necessary.
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
#define AZTEC		0		/* Aztec C (Macintosh) */
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

#ifndef SASM
#define SASM		0		/* Single-line assembler */
#endif

#ifndef PASM
#define PASM		0		/* Pre-assembler */
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

#define YES	((int)1)
#define NO	((int)0)
#define NONE	((int)0)
#define ERR	((int)(-1))
#define DEFAULT ((int)0)

#define ALPHAN(c)	(isalnum(c) || (c=='_'))
#define STREQ(a,b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

#if BSD			/* string redefinitions for BSD */
#define strchr	index
#define strrchr rindex
#endif

#if AZTEC		/* text file routine redefintion for Aztec */
#ifdef fgetc
#undef fgetc
#endif
#define fgetc	agetc
#ifdef fputc
#undef fputc
#endif
#define fputc	aputc
#endif

#define MAXBUF		255	/* longest input line */
#define MAXEXP		512	/* longest expression string */
#define MAXOP		16	/* longest mnemonic */
#define MAXOPT		8	/* longest OPT option */
#define MAXSYM		MAXBUF	/* longest label */
#define MAXSEC		255	/* maximum sections in a module */
#define MAXSINT		65535	/* maximum short integer */
#define LSTSYM		16	/* maximum characters of symbol to display */
#define P_LIMIT		255	/* limit of words collected for listing */
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
#define MAXADDR		0xFFFFL /* maximum memory address */
#define SIZES		BYTEWORD	/* size of DSP short word */
#define SIZEL		(SIZES + SIZES) /* size of DSP long word */
#define SIZED		(SIZES + SIZES) /* size of DSP double word */
#define SIZEA		(SIZEL + 1)	/* size of DSP accumulator word */
#define SIZEI		4	/* long integer size in bytes */
#define SIZEF		8	/* double precision floating point size */
#define DEFLABFW	10	/* default label field width */
#define DEFOPCFW	8	/* default opcode field width */
#define DEFOPRFW	10	/* default operand field width */
#define DEFXFLDW	12	/* default X field width */
#define DEFYFLDW	12	/* default Y field width */
#define PSTART		0x40L	/* start of P memory after interrupts */
#define IOSTART		0xFFC0	/* start of DSP I/O addresses */
#define BUFLEN		2048	/* I/O buffer length */

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
#define BASENAMLEN	78
#endif
#if MAC
#define BASENAMLEN	31
#endif

/*	Character Constants	*/
#define NEWLINE '\n'
#define TAB	'\t'
#define BLANK	' '
#define EOS	'\0'
#define COMM_CHAR ';'	/* comment character */
#define STR_DELIM '\''	/* string delimiter */
#define XTR_DELIM '"'	/* special string delimiter */
#define STR_CONCAT '+'	/* string concatenation character */
#define IFSKIP	'i'	/* marks skipped conditional lines */
#define MACDEF	'm'	/* marks macro definition lines */
#define CONTCH	'\\'	/* line continuation character */
#define MEXPCH	'+'	/* marks macro expansion lines */
#define DCEXP	'd'	/* marks DC directive expansion */
#define SCS_CHAR '.'	/* structured control statement delimiter */

/*	Macro defines		*/
#define MACARG	0x80	/* macro argument indicator */
#define STRTYP	0x81	/* macro string type argument indicator */
#define VALTYP	0x82	/* macro value type argument indicator */
#define HEXTYP	0x84	/* macro hex value type arg indicator */
#define VALCH	'?'	/* value operator */
#define HVALCH	'%'	/* hex value operator */
#define CONCATCH '\\'	/* concatenation operator */
#define STRCH	'"'	/* string operator */

/*	Input Modes		*/
#define FILEMODE	1	/* current input is from file */
#define MACXMODE	2	/* current input is from macro expansion */

/*	Opcode Classes (0 < n <= 127)	*/
#define ABBA		1	/* A,B or B,A			*/
#define ACCUM		2	/* A,B no moves allowed		*/
#define ACCUMXY		3	/* A,B moves allowed		*/
#define ANDOR		4	/* AND, OR instructions		*/
#define BCLASS		5	/* Bit instructions		*/
#define BTST		6	/* BTST instruction		*/
#define CMPCLASS	7	/* CMP(M) instructions		*/
#define DIVCLASS	8	/* DIV instruction		*/
#define DOCLASS		9	/* DO instruction		*/
#define ENDDO		10	/* ENDDO instruction		*/
#define ILLEGAL		11	/* ILLEGAL instruction		*/
#define INH		12	/* Inherent no moves allowed	*/
#define INHNR		13	/* Inherent no moves, no repeats allowed */
#define JCCLASS		14	/* Jcc instructions		*/
#define JCLRCLS		15	/* JCLR, JSET instructions	*/
#define JSCCLAS		16	/* JScc instructions		*/
#define JSCLR		17	/* JSCLR, JSSET instructions	*/
#define JSR		18	/* JSR instruction		*/
#define JUMP		19	/* JMP instruction		*/
#define LUACLASS	20	/* LUA instruction		*/
#define MACLASS		21	/* MAC, MACR instructions	*/
#define MOVCCLS		22	/* MOVEC instruction		*/
#define MOVCLAS		23	/* MOVE instruction		*/
#define MOVMCLS		24	/* MOVEM instruction		*/
#define MOVPCLS		25	/* MOVEP instruction		*/
#define MPYCLAS		26	/* MPY, MPYR instructions	*/
#define NOP		27	/* NOP instruction		*/
#define RD1		28	/* R,D1				*/
#define REPCLAS		29	/* Repeat instruction		*/
#define RTICLASS	30	/* RTI instruction		*/
#define RTSCLASS	31	/* RTS instruction		*/
#define S10D1		32	/* S10,D1			*/
#define S11D1		33	/* S11,D1			*/
#define S11D1AB		34	/* S11,D1 or A,B or B,A		*/
#define S15D1AB		35	/* S15,D1 or A,B or B,A		*/
#define S9D1		36	/* S9,D1			*/
#define SIMM		37	/* Short immediate		*/
#define TFRCC		38	/* Tcc				*/
#define TSTCLASS	39	/* TST instruction		*/

/*	XY Classes		*/
#define MOVES	1	/* Move				*/
#define ALL	2	/* All data transfers allowed	*/
#define VARIES	3	/* The transfer type may vary	*/

/*	Cycle Constants		*/
#define EA_CYCLES	2	/* Current effective address cycle count */
#define MVC_CYCLES	4	/* MOVEC extra cycles */
#define MVP_CYCLES	2	/* MOVEP extra cycles */

/*	Operand types */
#define OPSRC	1
#define OPDST	2

/*	Restricted instruction flag masks	*/
#define BADREP		0x0001	/* Instructions following REP */
#define BADRT		0x0002	/* RTI, RTS instructions */
#define BADDO		0x0004	/* DO instruction */
#define BADENDDO	0x0008	/* ENDDO instruction */
#define BADMOVESS	0x0010	/* MOVE from SSH or SSL */
#define BADRTI		0x0020	/* RTI instruction */
#define PREVDO		0x0040	/* Previous DO instruction */
#define BADINT		0x0080	/* Interrupt vector instructions */

/**
*	SASM status values
**/
#define SASMEMP		1L	/* empty line */
#define SASMCOM		2L	/* comment line */
#define SASMWRN		3L	/* warning */
#define SASMERR		4L	/* error */
#define SASMFTL		5L	/* fatal */

/**
*	address mode forcing catagories
**/
#define FORCEL		1	/* forced long			*/
#define FORCES		2	/* forced short			*/
#define FORCEI		3	/* forced IO short		*/

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
*	Memory counter types
**/
#define MCNTRS		3	/* number of location counters */
#define DCNTR		0	/* default location counter */
#define LCNTR		1	/* low memory counter */
#define HCNTR		2	/* high memory counter */

/**
*	Memory banks
**/
#define MBANKS		2	/* number of banks */
#define RBANK		0	/* runtime memory */
#define LBANK		1	/* load memory */

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
#define MINFRAC		-8388608L	/* minimum fraction */
#define MAXFRAC		 8388607L	/* maximum fraction */

/*	symbol/section attribute flags	*/
#define XSPACE	0x0001	/* X space symbol/section	*/
#define YSPACE	0x0002	/* Y space symbol/section	*/
#define LSPACE	0x0004	/* L space symbol/section	*/
#define PSPACE	0x0008	/* P space symbol/section	*/
#define SET	0x0010	/* set symbol			*/
#define LOCAL	0x0020	/* local symbol			*/
#define GLOBAL	0x0040	/* global symbol		*/
#define REL	0x0080	/* relocatable symbol/section	*/
#define INT	0x0100	/* integer number		*/
#define FPT	0x0200	/* floating pt number		*/
#define FRC	0x0400	/* fixed pt fractional number	*/
#define LONG	0x0800	/* long value			*/
#define RDEFERR 0x8000	/* redefined symbol		*/
#define RELERR	0x8000	/* relocatable sym/sec error	*/

/*	symbol/section mapping flags	*/
#define LOCTR	0x0010	/* low location counter		*/
#define HICTR	0x0020	/* high location counter	*/
#define IMAP	0x0100	/* internal memory		*/
#define EMAP	0x0200	/* external memory		*/
#define BMAP	0x0400	/* bootstrap memory		*/

/*	memory utilization flags */
#define MUOVRLY 0x0010
#define MUCODE	0x0020
#define MUDATA	0x0040
#define MUCONST 0x0080
#define MUMOD	0x0100
#define MUREV	0x0200

/**
*	Bit masks
**/
#define WORDMASK	0xFFFFFFL	/* DSP word mask */
#define BYTEMASK	0xFF		/* byte mask */
#define SHRTMASK	0xFFFF		/* short integer mask */
#define WBITMASK	0x800000L	/* DSP high bit mask */
#define BBITMASK	0x80		/* byte high bit mask */
#define MSPCMASK	0x000F		/* symbol memory space mask */
#define MUTPMASK	0x03E0		/* memory utilization type mask */
#define MCTRMASK	0x0030		/* location counter type mask */
#define MMAPMASK	0x0700		/* memory mapping mask */
#define MOVFMASK	0x0800		/* location counter overflow mask */
#define TYPEMASK	0x0F00		/* data type mask */

/*	symbol cross reference types */
#define DEFTYPE 1
#define REFTYPE 2

/*	section attribute flags */
#define SHORTS	1	/* short section */

/*	SCS boolean operator/terminator codes	*/
#define SCSAND	1
#define SCSOR	2
#define SCSTHEN 3
#define SCSDO	4

/*	Source field ordinals	*/
#define LBLFIELD	1
#define OPCFIELD	2
#define OPRFIELD	3
#define XMVFIELD	4
#define YMVFIELD	5

#define HASHSIZE 101	/* width of hash table (should be prime) */

#define MOVEMBO		0x000000L	/* MOVEM base opcode		*/
#define MOVECBO		0x040000L	/* MOVEC base opcode		*/
#define SIMMBO		0x0000B8L	/* Short immediate base opcode	*/
#define ANDBO		0x000046L	/* AND base opcode		*/
#define ORBO		0x000042L	/* OR base opcode		*/

/*	pseudo op classes (0 < n <= 127)	*/
#define BADDR		1	/* Baddr directive		*/
#define BSC		2	/* Bsc directive		*/
#define BSM		3	/* Bsm directive		*/
#define BSR		4	/* Bsr directive		*/
#define COBJ		5	/* Cobj directive		*/
#define COMMENT		6	/* Comment directive		*/
#define DC		7	/* Dc directive			*/
#define DEFINE		8	/* Define directive		*/
#define DSM		9	/* Dsm directive		*/
#define DSR		10	/* Dsr directive		*/
#define DSTO		11	/* Ds directive			*/
#define DUP		12	/* Dup directive		*/
#define DUPA		13	/* Dupa directive		*/
#define DUPC		14	/* Dupc directive		*/
#define ELSE		15	/* Else directive		*/
#define END		16	/* End directive		*/
#define ENDIF		17	/* Endif directive		*/
#define ENDM		18	/* Endm directive		*/
#define ENDSEC		19	/* Endsec directive		*/
#define EQU		20	/* Equate			*/
#define EXITM		21	/* Exitm directive		*/
#define FAIL		22	/* Fail directive		*/
#define GLOBL		23	/* Global directive		*/
#define IDENT		24	/* Ident directive		*/
#define IF		25	/* If directive			*/
#define INCLUDE		26	/* Include directive		*/
#define LIST		27	/* List directive		*/
#define LSTCOL		28	/* Lstcol directive		*/
#define MACLIB		29	/* Maclib directive		*/
#define MACRO		30	/* Macro directive		*/
#define MSG		31	/* Msg directive		*/
#define NOLIST		32	/* Nolist directive		*/
#define OPT		33	/* assembler option		*/
#define ORG		34	/* Origin			*/
#define PAGE		35	/* Page directive		*/
#define PMACRO		36	/* Pmacro directive		*/
#define PRCTL		37	/* Prctl directive		*/
#define RADIX		38	/* Radix directive		*/
#define RDIR		39	/* Rdirect directive		*/
#define SECTION		40	/* Section directive		*/
#define SETD		41	/* Set or = directive		*/
#define STITLE		42	/* sub-title directive		*/
#define SYMOBJ		43	/* Symobj directive		*/
#define TITLE		44	/* Title directive		*/
#define UNDEF		45	/* Undef directive		*/
#define WARN		46	/* Warn directive		*/
#define XDEF		47	/* Xdef directive		*/
#define XREF		48	/* Xref directive		*/

/* pseudo op label classes */
#define NOLABEL 0	/* no label allowed */
#define SPECIAL 1	/* code for pseudo opt will handle label field */
#define LABELOK 2	/* label allowed */

/* Option classes */
#define CC	1
#define CL	2
#define CM	3
#define CONTC	4
#define CRE	5
#define CEX	6
#define MC	7
#define MD	8
#define MEX	9
#define NOCC	10
#define NOCL	11
#define NOCM	12
#define NOCEX	13
#define NOMC	14
#define NOMD	15
#define NOMEX	16
#define NOUNAS	17
#define NOWRN	18
#define SYMT	19
#define UNAS	20
#define WRN	21
#define FC	22
#define NOFC	23
#define IL	24
#define MU	25
#define LOC	26
#define SO	27
#define RC	28
#define NORC	29
#define INTR	30
#define NOINTR	31
#define XR	32
#define NOXR	33
#define MINC	34
#define NOMINC	35
#define PS	36
#define NOPS	37
#define LW	38
#define NOLW	39
#define IC	40
#define LB	41
#define NOLB	42
#define DEX	43
#define NODEX	44
#define MSW	45
#define NOMSW	46
#define RP	47
#define NORP	48

/* SCS (structured control statement) types */
#define SCSBRK	1
#define SCSELSE 2
#define SCSENDF 3
#define SCSENDI 4
#define SCSENDL 5
#define SCSENDW 6
#define SCSFOR	7
#define SCSIF	8
#define SCSLOOP 9
#define SCSRPT	10
#define SCSUNTL 11
#define SCSWHL	12
#define SCSCONT 13
#define SCSREVL 14

/* Condition code types */
#define CNDCC	1
#define CNDCS	2
#define CNDEC	3
#define CNDEQ	4
#define CNDES	5
#define CNDGE	6
#define CNDGT	7
#define CNDHS	8
#define CNDLC	9
#define CNDLE	10
#define CNDLO	11
#define CNDLS	12
#define CNDLT	13
#define CNDMI	14
#define CNDNE	15
#define CNDNN	16
#define CNDNR	17
#define CNDPL	18

/*
 *	DSP opcode processing
 */

/* register names */

#define REGX	   0
#define REGY	   1
#define REGA	   2
#define REGB	   3
#define REGX0	   4
#define REGY0	   5
#define REGX1	   6
#define REGY1	   7
#define REGA0	   8
#define REGB0	   9
#define REGA1	   10
#define REGB1	   11
#define REGA2	   12
#define REGB2	   13
#define REGR0	   14
#define REGR1	   15
#define REGR2	   16
#define REGR3	   17
#define REGR4	   18
#define REGR5	   19
#define REGR6	   20
#define REGR7	   21
#define REGN0	   22
#define REGN1	   23
#define REGN2	   24
#define REGN3	   25
#define REGN4	   26
#define REGN5	   27
#define REGN6	   28
#define REGN7	   29
#define REGM0	   30
#define REGM1	   31
#define REGM2	   32
#define REGM3	   33
#define REGM4	   34
#define REGM5	   35
#define REGM6	   36
#define REGM7	   37
#define REGAB	   38
#define REGBA	   39
#define REGA10	   40
#define REGB10	   41
#define OMR	   42
#define SR	   43
#define LA	   44
#define LC	   45
#define SSH	   46
#define SSL	   47
#define SP	   48
#define MR	   49
#define CCR	   50

/**
*	register sets
**/
#define RSET1	1	/* A,B						*/
#define RSET2	2	/* Y0,Y1					*/
#define RSET3	3	/* X0,X1,A,B					*/
#define RSET4	4	/* Y0,Y1,A,B					*/
#define RSET5	5	/* X,Y,A,B,AB,BA,A10,B10			*/
#define RSET6	6	/* X0,X1,Y0,Y1,A0,A1,A2,A,B0,B1,B2,B,R0-R7,N0-N7 */
#define RSET7	7	/* OMR,SR,LA,LC,SSH,SSL,SP			*/
#define RSET8	8	/* M0-M7					*/
#define RSET9	9	/* A,B,X1,X0,Y1,Y0,R0-R7,N0-N7			*/
#define RSET10	10	/* X,Y						*/
#define RSET11	11	/* X0,X1,Y0,Y1					*/
#define RSET12	12	/* X,Y,X0,X1,Y0,Y1				*/
#define RSET13	13	/* A						*/
#define RSET14	14	/* B						*/
#define RSET15	15	/* A,B,X0,X1,Y0,Y1				*/
#define RSET16	16	/* R0-R7					*/
#define RSET17	17	/* X0,X1,Y0,Y1,A0,A1,A2,A,B0,B1,B2,B,R0-R7,N0-N7,M0-M7,OMR,SR,LA,LC,SSH,SSL,SP */
#define RSET18	18	/* X0,X1,X,Y0,Y1,Y,A0,A1,A2,A,B0,B1,B2,B,AB,BA,R0-R7,N0-N7,M0-M7,OMR,SR,LA,LC,SSH,SSL,SP */
#define RSET19	19	/* MR,CCR,OMR					*/
#define RSET20	20	/* R0-R7,N0-N7					*/
#define RSET21	21	/* A,B,X,Y,X0,X1,Y0,Y1				*/
#define RSET22	22	/* A,B,X0	(Rev. C)			*/
#define RSET23	23	/* X0,Y0,Y1,A,B	(Rev. C)			*/

/* Addressing modes */
#define RDIRECT		1	/* register direct			*/
#define NOUPDATE	2	/* (Ra)	   no update			*/
#define POSTINC		3	/* (Ra)+   postincrement		*/
#define POSTDEC		4	/* (Ra)-   postdecrement		*/
#define PSTINCO		5	/* (Ra)+N  postincrement by offset Na	*/
#define PSTDECO		6	/* (Ra)-N  postdecrement by offset Na	*/
#define INDEXO		7	/* (Ra+N)  indexed by offset Na		*/
#define PREDEC		8	/* -(Ra)   predecrement by 1		*/
#define LIMMED		9	/* long immediate		xxxx	*/
#define MIMMED		10	/* 12 bit short immediate	xxx	*/
#define SIMMED		11	/* 8 bit short immediate	xx	*/
#define NIMMED		12	/* 5 bit short immediate	n	*/
#define LABSOL		13	/* long absolute		@@@@	*/
#define IABSOL		14	/* IO short absolute		@I	*/
#define MABSOL		15	/* 12 bit short absolute	@@@	*/
#define SABSOL		16	/* 6 bit short absolute		@@	*/

/*	register ea types	*/
#define REA1		1	/* NOUPDATE, POSTINC, POSTDEC, PSTINCO, */
				/* PSTDECO, INDEXO, PREDEC */
#define REA2		2	/* NOUPDATE, POSTINC, POSTDEC, PSTINCO */
#define REA3		3	/* POSTINC, POSTDEC, PSTINCO, PSTDECO */

/**
*	memory space classes
**/
#define SCLS1		1	/* X:				*/
#define SCLS2		2	/* Y:				*/
#define SCLS3		3	/* L:				*/
#define SCLS4		4	/* P:				*/
#define SCLS5		5	/* X: or Y:			*/
#define SCLS6		6	/* X: or Y: or L: or P: or NONE */
#define SCLS7		7	/* X: or Y: or NONE		*/
#define SCLS8		8	/* Y: or NONE			*/
#define SCLS9		9	/* X: or Y: or P: or NONE	*/

/**
*	absolute addressing catagories
**/
#define LAS		1	/* long absolute @@@@		*/
#define IA		2	/* short IO absolute @I		*/
#define MA		3	/* 12 bit short absolute @@@	*/
#define SA		4	/* 6 bit short absolute @@	*/
#define LMA		5	/* LAS or MA			*/
#define LISA		6	/* LAS or IA or SA		*/
#define ISA		7	/* IA or SA			*/
#define LIA		8	/* LAS or IA			*/
#define LSA		9	/* LAS or SA			*/

/**
*	immediate catagories
**/
#define LI		1	/* long immediate xxxx		*/
#define MI		2	/* 12 bit short immediate xxx	*/
#define SI		3	/* 8 bit short immediate xx	*/
#define NI		4	/* 5 bit short immediate n	*/
#define LSI		5	/* LI or SI			*/

/**
*	flag settings
**/
#define COMMA		0x01	/* check that comma follows operand */
#define DEST		0x02	/* check operand for dup. dest. register */

/**
*
*	evaluation operators
*
**/
#define ADD	1
#define SUB	2
#define MUL	3
#define DIV	4
#define AND	5
#define OR	6
#define EOR	7
#define MOD	8
#define SHFT_L	9
#define SHFT_R	10
#define LTHAN	11
#define GTHAN	12
#define EQUL	13
#define LTEQL	14
#define GTEQL	15
#define NEQUL	16
#define LOGAND	17
#define LOGOR	18

/**
*	functions
**/
#define FDEF	1
#define FEXP	2
#define FINT	3
#define FLCV	4
#define FMSP	5
#define FMAC	6
#define FSCP	7
#define FCVI	8
#define FCVF	9
#define FCVS	10
#define FFRC	11
#define FACS	12
#define FASN	13
#define FATN	14
#define FCOS	15
#define FLOG	16
#define FL10	17
#define FSIN	18
#define FSQT	19
#define FTAN	20
#define FXPN	21
#define FAT2	22
#define FPOW	23
#define FABS	24
#define FSGN	25
#define FLST	26
#define FUNF	27
#define FSNH	28
#define FCOH	29
#define FTNH	30
#define FFLR	31
#define FCEL	32
#define FMIN	33
#define FMAX	34
#define FREL	35
#define FRND	36
#define FLFR	37
#define FLUN	38

/**
* object file record types
**/

#define OBJNULL		0
#define OBJANY		0
#define OBJSTART	1
#define OBJEND		2
#define OBJSECTION	3
#define OBJSYMBOL	4
#define OBJDATA		5
#define OBJBLKDATA	6
#define OBJRDATA	7
#define OBJRBLKDATA	8
#define OBJXDEF		9
#define OBJXREF		10
#define OBJLINE		11
#define OBJCOMMENT	12

/**
* global structures
**/

struct multval {	/* multiple-precision value structure */
		long	ext, high, low;
		};

union udef {
		double	fdef;		/* floating point value */
		struct	multval xdef;	/* multiple-precision value */
		};

struct evres { /* evaluation result */
/*
	NOTE:	If any changes are made to this structure the DSP
		Simulator development team should be notified.	They
		use it in their evaluator code which is called by
		the Single-line Assembler (SASM) to evaluate expressions.
		Any discrepancies will cause inconsistent results
		from SASM!
*/
	int	etype;	/* type of evaluation result */
	int	bcount; /* byte count of characters in string */
	int	wcount; /* word count of evaluated string */
	union{
		double	fvalue; /* floating point value */
		long	*wvalue;/* pointer to string word array */
		struct	multval xvalue; /* multiple-precision value */
	}uval;
	int	force;		/* forced size (long,short,etc.) */
	int	fwd_ref;	/* NO = result contains no forward refs */
	int	mspace;		/* memory space attribute */
	int	size;		/* significant size of value in bytes */
	int	rel;		/* YES = relative value */
	int	secno;		/* section number (0 = global, <0 = ext ref)*/
};

struct nlist { /* basic symbol table entry */
	char	*name;		/* symbol name */
	union udef def;		/* symbol value */
	int	attrib;		/* attribute flags */
	int	mapping;	/* mapping flags */
	struct slist *sec;	/* section descriptor ptr */
	struct xlist *xrf;	/* cross reference list ptr */
	struct nlist *next;	/* next entry in chain */
};

struct slist { /* basic relocatable section table entry */
	char	*sname;		/* section name */
	int	secno;		/* section number */
	int	flags;		/* section bit flags */
	int	rflags;		/* runtime bit flags */
	int	lflags;		/* load bit flags */
	long	(*cntrs)[MCNTRS][MBANKS-1]; /* location counters */
	struct	macd *xrefs;	/* list of xref symbols */
	struct	macd *xdefs;	/* list of xdef symbols */
	struct	slist *next;	/* next entry in chain */
};

struct secstk { /* section stack */
	struct slist *secpt;	/* stacked section descriptor */
	struct secstk *ssnext;	/* next in chain */
};

struct xlist {	/* an entry in the cross reference listing */
	int	lnum;	/* line number */
	int	rtype;	/* reference type */
	struct	xlist *next;	/* next in chain */
};

struct mnemop {	 /* an entry in the mnemonic table */
	char	*mnemonic;	/* its name */
	char	class;		/* its class */
	char	xyclass;	/* its XY move class */
	long	opcode;		/* its base opcode */
	int	cycles;		/* its base # of cycles */
};

struct pseuop {	  /* an entry in the pseudo op table */
	char	*pmnemon;	/* its name */
	char	ptype;		/* its type */
	char	plclass;	/* its label class */
};

struct opt {   /* an entry in the option table */
	char	*omnemon;	/* its name */
	char	otype;		/* its type */
};

struct fnc {   /* an entry in the function table */
	char	*funcname;	/* its name */
	char	ftype;		/* its type */
	double	(*rtn)();	/* pointer to transcendental routine */
};

struct scs {   /* an entry in the structured control statement table */
	char	*scsname;	/* its name */
	char	scstype;	/* its type */
	int	(*scsrtn)();	/* pointer to scs routine */
};

struct deflist { /* basic define table entry */
	char	*dname;		/* symbol */
	char	*ddef;		/* replacement string */
	int	deflags;	/* define flags */
	struct slist *dsect;	/* pointer to section */
	struct deflist *dnext ; /* next entry in chain */
};

struct filelist {	/* include file stack	*/
	char	*ifname;	/* file name */
	FILE	*ifd;		/* file structure */
	int	fnum;		/* file number */
	int	iln;		/* last line number */
	char	*prevline;	/* pointer to previous line */
	struct filelist *inext; /* next entry in chain */
};

struct fn {	/* file name list */
	char *fnam;		/* filename */
	struct fn *fnnext;	/* next entry in chain */
};

struct mac {	/* macro definition header */
	char *mname;	/* macro name */
	struct slist *msect; /* section ptr */
	int def_line;	/* definition line number */
	int margs;	/* number of arguments */
	struct macd *mdef; /* definition */
	struct mac *next;  /* next in chain */
};

struct macd {	/* macro definition */
	char *mline;	/* definition line */
	struct macd *mnext;	/* next in line */
};

struct dumname {	/* macro dummy argument linked list */
	char *daname;	/* dummy argument name */
	int num;	/* number in order */
	struct dumname *dnext;	/* next in chain */
};

struct macexp {		/* macro expansion linked list */
	int no_args;		/* number of expansion arguments */
	struct macd *defline;	/* pointer to current definition line */
	struct macd *prevline;	/* pointer to previous definition line */
	char *lc;		/* current character within line */
	struct macd *args;	/* ptr to linked list of expansion args */
	char *curarg;		/* current argument under expansion */
	int eline;		/* line number of expansion */
	struct nlist *lcl;	/* expansion local label list pointer */
	struct explcl *prevlcl; /* previous expansion local label list */
	struct macexp *next;	/* next in chain */
};

struct explcl {		/* macro expansion local label list */
	int eslln;	/* line number of expansion */
	struct nlist *list;	/* list of local labels */
	struct explcl *lnext;	/* next in chain */
};

struct fwdref {		/* forward reference structure */
	long ref;	/* lookup count for forward reference */
	struct fwdref *next;	/* pointer to next fwd. ref. structure */
};

struct obj{		/* structure for encoded instruction */
/*
	NOTE:	If any changes are made to this structure the DSP
		Simulator development team should be notified.	It is
		the interface structure for passing information between
		the Single-line Assembler (SASM) and the Simulator.
		Any discrepancies will cause inconsistent results
		from SASM!

		The SASM interface is currently defined as follows:
		SASM is called with a pointer to a string containing
		an instruction sequence.  SASM returns with a pointer to
		a structure of type obj.  This structure contains the
		encoded instruction and a word count.  Normally, the word
		count is either 1 or 2; if something other than an instruction
		is encountered, or an error occurs, the word count is 0.
		In the latter case, a status value is placed in the opcode
		field of the obj structure.
*/
	int count;		/* word count (1 or 2; 0 on error for SASM) */
	long opcode;		/* base opcode (status on SASM error) */
	long postword;		/* post word */
	char *opexp;		/* opcode expression string */
	char *postexp;		/* post word expression string */
};

struct amode{		/* addressing mode structure */
	int space;	/* memory space */
	int mode;	/* address mode */
	int fref;	/* if mode is absolute or immediate, indicates */
			/* forward refs occurred in expression evaluation */
	int force;	/* forced type (long, short, etc.) */
	long val;	/* address if mode is SABSOL or LABSOL */
			/* value if mode is SIMMED or LIMMED */
	int reg;	/* address register if mode is indexed */
			/* register # if mode is RDIRECT */
	int secno;	/* section number (0 = global, <0 = ext ref) */
	char *expstr;	/* expression string */
};

struct dostack{		/* DO loop instruction stack */
	long la;		/* stacked loop address */
	char *line;		/* next line after DO instruction */
	struct dostack *next;
};

struct symref {		/* symbol reference data */
	char *name;		/* pointer to symbol name */
	struct slist *sec;	/* pointer to section where referenced */
	struct symref *prev;	/* previous entry in chain */
	struct symref *next;	/* next entry in chain */
};

struct symlist{		/* symbol table entry list */
	struct nlist *sym;	/* pointer to symbol table entry */
	struct symlist *next;
};

struct scslab {		/* scs label entry */
	unsigned labval;	/* label value */
	char	labtype;	/* label type */
	struct scslab *nxtlab;	/* next label */
};

struct scsop {		/* scs operand structure */
	char *str;		/* operand string */
	struct amode info;	/* operand info */
};

struct cond {		/* condition code table entry */
	char *ctext;		/* condition text */
	char ctype;		/* condition type */
	char compl;		/* condition complement */
};

struct scsexp {		/* scs expression structure */
	struct scsop op1;	/* first operand */
	struct cond *oper;	/* conditional operator (ptr into tbl) */
	struct scsop op2;	/* second operand */
	int cmpd;		/* compound operator type */
};

struct scsfor {		/* scs for statement structure */
	struct scsop cntr;	/* for loop counter */
	struct scsop init;	/* for loop initializer */
	struct scsop targ;	/* for loop target value */
	struct scsop step;	/* for loop step value */
	int downto;		/* downto flag */
};

struct murec{		/* memory utilization record */
	int flags;		/* attribute flags */
	long start;		/* start address */
	long length;		/* block length */
	struct nlist *lab;	/* pointer to associated label record */
	struct slist *sec;	/* pointer to associated section record */
	struct murec *ov;	/* pointer to overlay record */
	struct murec *next;	/* pointer to next record in chain */
};

struct inmode{		/* input mode structure */
	int mode;		/* input mode (file, macro, etc.) */
	struct inmode *next;	/* next on stack */
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

#define TMacbaseyr	1904
#define TUNIXbaseyr	1970

/* number of leap days between the two years -- Mac base was a leap year! */
#define TLpD	((TUNIXbaseyr-TMacbaseyr-1)/4)

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
