#include "lnkdef.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* DSPLNK global variables 
*
**/

char	Gagerrs = NO;		/* YES = inhibit errors */
char	Dcl_flag = NO;		/* YES = using VMS DCL command parsing	*/
char	Debug = NO;		/* YES = symbol/line data to load file */
char	Xrf_flag = NO;		/* YES = external reference installed */
char	User_exp = NO;		/* YES = evaluating user expression */
char	Con_flag = YES;		/* YES = constant symbols to map file */
char	Loc_flag = YES;		/* YES = local symbols to map file */
char	Sea_flag = YES;		/* YES = section-by-address to map file */
char	Sen_flag = YES;		/* YES = section-by-name to map file */
char	Syn_flag = YES;		/* YES = symbol-by-name to map file */
char	Syv_flag = YES;		/* YES = symbol-by-value to map file */
char	Cflag = NO;		/* YES = linker invoked by C compiler */
char	Icflag = NO;		/* YES = ignore case on symbols */
char    Verbose = NO;           /* YES = send progress lines to std. error */
char	Testing = NO;		/* YES = in test mode */

int	Pass = 0;               /* Current pass number */
int	Symcnt = 0;		/* Number of installed symbols */
int	Secnt = 0;		/* section count */
int	Sectot = 0;		/* section total */
int	Secno = 0;		/* global section number */
int	Xrfcnt = 0;		/* External reference count		*/
int     Lineno = 0;             /* current line number          	*/
int	Recno = 0;		/* current record number		*/
int	Fldno = 0;		/* current field number			*/
int     Err_count = 0;          /* total number of errors       	*/
int	Warn_count = 0;		/* total number of warnings		*/
int	Gagcnt = 0;		/* errors output while gagged		*/
int     Vcount = 100;           /* default maximum verbose count        */
int	Radix = 10;		/* default radix for constants		*/
long	End_addr = -1L;		/* Starting address of object module	*/
long	Recoff = 0L;		/* Current input module record offset	*/
#if VMS
union	vmscond Vmserr;		/* VAX/VMS condition code		*/
#endif

int	Page = 0;		/* Output listing page #		*/
int	Lst_col = 1;		/* current output listing column	*/
int	Lst_char = 1;		/* current output character position 	*/
int	Max_col = 132;		/* maximum listing column		*/
int	Start_col = 1;		/* column to start output listing 	*/
int	Lst_line = 1;		/* current listing line			*/
int	Max_line = 66;		/* maximum list lines per page		*/
int 	Lst_p_len = 66;		/* listing page length			*/
int	Formflag = 1;		/* !=0 means add page headings		*/
int	Lst_topm = 3;		/* number of blank lines at top of page */
int	Lst_botm = 3;		/*   number of blank lines bottom of page */

char    Field[MAXBUF+1] = {0};  /* input field buffer            	*/
char	String[MAXBUF+1] = {0};	/* string buffer			*/
char	Mapfn[MAXBUF+1] = {0};	/* map file name buffer			*/
char	Curdate[9] = {0};	/* current date				*/
char	Curtime[9] = {0};	/* current time				*/
char    *Optr = NULL;           /* pointer into current expression field*/

#if VMS				/* VMS DCL command line option descriptors */
$DESCRIPTOR(Arg_desc, String);
$DESCRIPTOR(Cc_desc, "CC");
$DESCRIPTOR(Dbg_desc, "DEBUG");
$DESCRIPTOR(Lib_desc, "LIBRARY");
$DESCRIPTOR(Lnk_desc, "LINK");
$DESCRIPTOR(Map_desc, "MAP");
$DESCRIPTOR(Mem_desc, "MEMORY");
$DESCRIPTOR(Noc_desc, "NOCASE");
$DESCRIPTOR(Obj_desc, "OBJECT");
$DESCRIPTOR(Org_desc, "ORIGIN");
$DESCRIPTOR(Tst_desc, "TEST");
$DESCRIPTOR(Vbs_desc, "VERBOSE");
#else
int	Lnk_desc;		/* fake runtime variables */
int	Cc_desc;
#endif

#if LSC
jmp_buf	Env;			/* setjmp/longjmp environment buffer */
#endif

long	Start[MSPACES][MCNTRS] = {	/* Memory start addresses */
			{0L, 0L, 0L},	/* X memory start address */
			{0L, 0L, 0L},	/* Y memory start address */
			{0L, 0L, 0L},	/* L memory start address */
			{0L, 0L, 0L}	/* P memory start address */
			};
int	Mflags[MSPACES][MCNTRS] = {	/* Memory map flags */
			{0, 0, 0}, {0, 0, 0},
			{0, 0, 0}, {0, 0, 0}
			};
long	*Pc = &Start[PMEM-1][DCNTR];	/* Runtime Program Counter */
long	Old_pc = 0L;		/* Previous Runtime Program Counter */
int	Cspace = PSPACE;	/* current runtime memory space	*/
int     Rcntr = DEFAULT;        /* runtime counter type */
int     Rmap = NONE;            /* runtime memory map flag */
long	*Lpc = &Start[PMEM-1][DCNTR];	/* current load memory cntr */
long	Old_lpc = 0L;		/* previous load program counter */
int	Loadsp = PSPACE;	/* current load memory space 	*/
int     Lcntr = DEFAULT;        /* load counter type */
int     Lmap = NONE;            /* load memory map flag */

int	Comfiles = 0;		/* total number of command line files */
int	Ccomf = 0;		/* current command line file */
char	**Comfp = NULL;		/* list of command line filenames */
FILE    *Fd = NULL;	        /* Current input file pointer */
struct file *Files = NULL;	/* link file list pointer */
struct file *Cfile = NULL;	/* Current input file structure */

int	Obj_rtype = OBJNULL;		/* object file record type */
char	Objbuf[MAXOBJLINE+1] = {0};	/* object file line buffer */

char	*End_exp = NULL;	/* start address expression string pointer */

char	*Mod_name = NULL;	/* object module name */
int	Mod_vers = 0;		/* object module version */
int	Mod_rev = 0;		/* object module revision */
char	*Mod_com = NULL;	/* object module comment */

int	(*Cmp_rtn) () = NULL;	/* pointer to sort compare routine */

struct slist *Csect = NULL;		/* current section ptr */
struct smap *Secmap = NULL;		/* section map array */
struct slist *Seclst = NULL;		/* section library lookup list */
struct slist *Secptr = NULL;		/* pointer into sec. lib. list */

struct slist *Last_sec = NULL;		/* last section found in lookup */
struct nlist *Last_sym	= NULL;		/* last symbol found in lookup */
struct symref *Last_xrf	= NULL;		/* last xref found in lookup */

char **Sort_tab = NULL;		/* pointer to quicksort table */
int Sortmem = 0;		/* memory space to sort */
int Sortctr = 0;		/* memory counter to sort */
struct slist **Sec_sort = NULL;	/* pointer to section sort table */
struct nlist **Sym_sort = NULL;	/* pointer to symbol sort table */
struct symref **Xrf_sort = NULL;/* pointer to external reference sort table */

FILE	*Outfil = NULL;		/* output file's file pointer */
FILE    *Lodfil = NULL;		/* load file's file pointer */
FILE    *Mapfil = NULL;		/* map file's file pointer */
FILE	*Memfil = NULL;		/* memory file's file pointer */

char	Progdef[] = "lnk56000";	/* program default name */
char	*Progname   = Progdef;	/* pointer to program name */
char	Progtitle[] = "Motorola DSP56000 Cross Linker";
char	Version[]   = "Version 3.01";	/* linker version number */
char	Copyright[] = "(C) Copyright Motorola, Inc. 1987, 1988, 1989.  All rights reserved.";
char	Gsecname[]= "GLOBAL";	/* global section name */
char	Modhdr[] = "-h-";	/* library module header */
int	Modlen = sizeof (Modhdr) - 1;

/**
*
* 	record name table
*
**/

struct rectype Rectab[] = {
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"DATA",		OBJDATA,
	"COMMENT",	OBJCOMMENT,
	"SYMBOL",	OBJSYMBOL,
	"SECTION",	OBJSECTION,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"LINE",		OBJLINE,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"BLOCKDATA",	OBJBLKDATA,
	"START",	OBJSTART,
	"NULL",		OBJNULL,
	"NULL",		OBJNULL,
	"END",		OBJEND,
	"XREF",		OBJXREF
};

int Rectsize = sizeof (Rectab) / sizeof (struct rectype);

/**
*
* 	memory file record name table
*
**/

struct rectype Memtab[] = {
	"base",		MEMBASE,
	"map",		MEMMAP,
	"section",	MEMSECTION,
	"start",	MEMSTART,
	"symbol",	MEMSYMBOL
};

int Memtsize = sizeof (Memtab) / sizeof (struct rectype);

/**
*
* 	map file record option table
*
**/

struct rectype Opttab[] = {
	"noconst",	MAPNOCON,
	"nolocal",	MAPNOLOC,
	"nosecaddr",	MAPNOSEA,
	"nosecname",	MAPNOSEN,
	"nosymname",	MAPNOSYN,
	"nosymval",	MAPNOSYV
};

int Opttsize = sizeof (Opttab) / sizeof (struct rectype);

/**
*
* 	symbol table
*
**/

struct nlist *Symtab[HASHSIZE] = {0};

/**
*
* 	section table
*
**/

struct slist *Sectab[HASHSIZE] = {0};

/**
*
* 	external reference table
*
**/

struct symref *Xrftab[HASHSIZE] = {0};

/**
*
*	Expression operator routine table
*
**/
extern struct evres
*bad_op (),
*do_add (),
*do_sub (),
*do_mul (),
*do_div (),
*do_and (),
*do_or (),
*do_eor (),
*do_mod (),
*do_shftl (),
*do_shftr (),
*do_gthan (),
*do_lthan (),
*do_equl (),
*do_gteql (),
*do_lteql (),
*do_nequl (),
*do_logand (),
*do_logor (),
*do_bsize ();

struct evres *(*Exop_rtn[])() = {
bad_op,
do_add,
do_sub,
do_mul,
do_div,
do_and,
do_or,
do_eor,
do_mod,
do_shftl,
do_shftr,
do_lthan,
do_gthan,
do_equl,
do_lteql,
do_gteql,
do_nequl,
do_logand,
do_logor,
do_bsize
};

