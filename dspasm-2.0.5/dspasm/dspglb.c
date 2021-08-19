#include "dspdef.h"
#include <math.h>		/* for transcendental math functions */
#if MPW
#ifdef log10
#undef log10
extended log10();
#endif
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* DSPASM global variables
*
**/

char	Fcflag = NO;		/* YES = fold trailing comments */
char	Muflag = NO;		/* YES = provide memory utilization report */
char	Mu_org = NO;		/* YES = ORG since last mem. util. update */
char	End_flag = NO;		/* YES = end directive encountered */
char	Unasm_flag = NO;	/* YES = print lines skipped in conditnls */
#if !PASM
char	Prtc_flag = YES;	/* YES = print conditional assem directives */
char	Pmacd_flag = YES;	/* YES = print macro definitions */
char	Mex_flag = NO;		/* YES = print macro expansions */
#else
char	Prtc_flag = NO;		/* YES = print conditional assem directives */
char	Pmacd_flag = NO;	/* YES = print macro definitions */
char	Mex_flag = YES;		/* YES = print macro expansions */
#endif
char	Mc_flag = YES;		/* YES = print macro calls */
char	Cm_flag = YES;		/* YES = save comments with macro defs */
char	Ilflag = NO;		/* YES = Inhibit source listing */
char	Cyc_flag = NO;		/* YES = output cycle count */
char	Cre_flag = NO;		/* YES = cross reference symbol table */
char	Cre_err = NO;		/* YES = CRE used after symbol definition */
char	S_flag = NO;		/* YES = output symbol table */
char	So_flag = NO;		/* YES = write symbol info to object file */
char	Cex_flag = NO;		/* YES = print DC expansions */
char	Loc_flag = NO;		/* YES = output local symbol table */
char	Wrn_flag = YES;		/* YES = print warnings */
char	Gagerrs = NO;		/* YES = inhibit errors */
char	Abortp = NO;		/* YES = do not print current line */
char	Force_prt = NO;		/* YES = force print when Macro_exp != NULL */
char	Ctlreg_flag = NO;	/* YES = control registers accessed */
char	Emit = NO;		/* YES = code/data emitted */
char	Rcom_flag = NO;		/* YES = relative comment spacing on list */
char	Def_flag = NO;		/* YES = DEFINE directives specified */
char	Swap = NO;		/* YES = XY data move fields swapped */
char	Chkdo = NO;		/* YES = reprocessing first DO instruction */
char	Laerr = NO;		/* YES = loop address error */
char	Dcl_flag = NO;		/* YES = using VMS DCL command parsing	*/
#if !PASM
char	Lnkmode = YES;		/* YES = assemble in linking mode */
char	Relmode = YES;		/* YES = assemble in relative mode */
#else
char	Lnkmode = NO;		/* YES = assemble in linking mode */
char	Relmode = NO;		/* YES = assemble in relative mode */
#endif
char	Local = NO;		/* YES = processing local label list */
char	Xrf_flag = NO;		/* YES = XDEF symbols global all sections */
char	Mi_flag = NO;		/* YES = add MACLIB paths to INCLUDE paths */
char	Pack_flag = YES;	/* YES = pack strings in DC directive */
char	Int_flag = NO;		/* YES = enable interrupt vector checking */
char	Cflag = NO;		/* YES = assembler invoked by C compiler */
char	Cnlflag = NO;		/* YES = no listing for compiler code */
char	Strtlst = YES;		/* YES = beginning of listing */
char	Lwflag = YES;		/* YES = support long words in BSC/DC */
char	Icflag = NO;		/* YES = ignore case on symbols */
char	Ic_err = NO;		/* YES = IC used after symbol definition */
char	Lbflag = NO;		/* YES = increment load counter by bytes */
char	Dex_flag = NO;		/* YES = expand DEFINEs within quotes */
char	Msw_flag = YES;		/* YES = warn on incompatible memory space */
char	Ro_flag = NO;		/* YES = runtime location counter overflow */
char	Lo_flag = NO;		/* YES = load location counter overflow */
char	Rpflag = NO;		/* YES = gen. NOP for reg. padding */
char	Padreg = NO;		/* YES = register padding in progress */
char	Verbose = NO;		/* YES = send progress lines to std. error */
char	Testing = NO;		/* YES = in test mode */

int	Sym_cnt = 0;		/* Number of installed non-local symbols */
int	Lsym_cnt = 0;		/* number of installed local labels	*/
int	Xrf_cnt = 0;		/* external reference count		*/
int	Line_num = 0;		/* current source line number		*/
int	Real_ln = 0;		/* real line num adjusted for continuation */
int	Lst_lno = 0;		/* listing line number			*/
int	Err_count = 0;		/* total number of errors		*/
int	Warn_count = 0;		/* total number of warnings		*/
int	Line_err = 0;		/* errors/warnings per line		*/
int	Gagcnt = 0;		/* errors output while gagged		*/
int	No_defs = 0;		/* total number of define symbols	*/
int	Vcount = 100;		/* default maximum verbose count	*/
int	Radix = 10;		/* default radix for constants		*/
int	Bad_flags = 0;		/* indicates restricted instructions	*/
int	Prev_flags = 0;		/* save area for bad flags		*/
long	Seed = 0L;		/* RND function seed value		*/
#if !PASM
int	Lflag = 0;		/* >0 = produce listing			*/
#else
int	Lflag = 1;		/* >0 = produce listing			*/
#endif
#if VMS
union	vmscond Vmserr;		/* VAX/VMS condition code		*/
#endif

int	Page = 0;		/* Output listing page #		*/
int	Lst_col = 1;		/* current output listing column	*/
int	Lst_char = 1;		/* current output character position	*/
int	Max_col = 80;		/* maximum listing column		*/
int	Start_col = 1;		/* column to start output listing	*/
int	Lst_line = 1;		/* current listing line			*/
int	Max_line = 66;		/* maximum list lines per page		*/
int	Lst_p_len = 66;		/* listing page length			*/
int	Formflag = 1;		/* !=0 means add page headings		*/
int	Page_flag = 1;		/* !=0 means allow page ejects with Page */
int	Lst_topm = 0;		/* number of blank lines at top of page */
int	Lst_botm = 0;		/* number of blank lines bottom of page */
int	Labwidth = DEFLABFW;	/* width of label field			*/
int	Opwidth = DEFOPCFW;	/* width of op field			*/
int	Operwidth = DEFOPRFW;	/* width of operand field		*/
int	Xwidth = DEFXFLDW;	/* width of X field			*/
int	Ywidth = DEFYFLDW;	/* width of Y field			*/

char	*Title = NULL;		/* Program title			*/
char	*Subtitle = NULL;	/* Program sub title			*/

char	Nullstr[1] = {0};	/* null string buffer			*/
char	Line[MAXBUF+1] = {0};	/* input line buffer			*/
char	Pline[MAXBUF+8] = {0};	/* parser line buffer (w/padding room)	*/
char	String[MAXBUF+1] = {0}; /* string buffer			*/
char	Expr[MAXBUF+1] = {0};	/* expression buffer			*/
char	Curdate[9] = {0};	/* current date				*/
char	Curtime[9] = {0};	/* current time				*/
char	*Cfname = Nullstr;	/* current file name for error msgs	*/
char	*Label = Nullstr;	/* label on current line		*/
char	*Op = Nullstr;		/* opcode mnemonic on current line	*/
char	*Operand = Nullstr;	/* operand field on current line	*/
char	*Xmove = Nullstr;	/* X data movement field on current ln	*/
char	*Ymove = Nullstr;	/* Y data movement field on current ln	*/
char	*Com = Nullstr;		/* remainder of line after other fields */
char	*Optr = NULL;		/* pointer into current Operand field	*/
char	*Eptr = NULL;		/* pointer to current expression	*/
char	*Hexstr = Nullstr;	/* pointer to hex expression character	*/
char	*Lngstr = " ";		/* pointer to long expression concat. str. */

#if VMS				/* VMS DCL command line option descriptors */
$DESCRIPTOR(Arg_desc, String);
$DESCRIPTOR(Abs_desc, "ABSOLUTE");
$DESCRIPTOR(Cc_desc,  "CC");
$DESCRIPTOR(Def_desc, "DEFINE");
$DESCRIPTOR(Inc_desc, "INCLUDE");
$DESCRIPTOR(Lst_desc, "LISTING");
$DESCRIPTOR(Mac_desc, "MACLIB");
$DESCRIPTOR(Obj_desc, "OBJECT");
$DESCRIPTOR(Opt_desc, "OPTION");
$DESCRIPTOR(Src_desc, "SOURCE");
$DESCRIPTOR(Tst_desc, "TEST");
$DESCRIPTOR(Vbs_desc, "VERBOSE");
#else
int	Src_desc;		/* fake runtime variables */
int	Cc_desc;
#endif

#if LSC
jmp_buf Env;			/* setjmp/longjmp environment buffer */
#endif

long	Gcntrs[MSPACES][MCNTRS][MBANKS] = { /* absolute location counters */
		{ {0L,0L}, {0L,0L}, {0L,0L} },
		{ {0L,0L}, {0L,0L}, {0L,0L} },
		{ {0L,0L}, {0L,0L}, {0L,0L} },
		{ {0L,0L}, {0L,0L}, {0L,0L} }
	};

long	Glbcntrs[MSPACES][MCNTRS][MBANKS-1] = { /* rel. location counters */
		{ {0L}, {0L}, {0L} },
		{ {0L}, {0L}, {0L} },
		{ {0L}, {0L}, {0L} },
		{ {0L}, {0L}, {0L} }
	};

struct	slist Glbsec = {	/* global static section */
			"GLOBAL",	/* section name */
			0,		/* section number */
			REL,		/* default section flags */
			PSPACE,		/* default run flags */
			PSPACE,		/* default load flags */
			Glbcntrs,	/* location counters */
			NULL,		/* no xrefs */
			NULL,		/* no xdefs */
			NULL		/* not in section list */
		};

long	*Pc = NULL;		/* Run Program Counter */
long	Old_pc = 0L;		/* saved runtime program counter */
int	Cspace = PSPACE;	/* current runtime memory space */
int	Rcntr = DEFAULT;	/* runtime counter type */
int	Rmap = NONE;		/* runtime memory map flag */
long	*Lpc = NULL;		/* Load Program Counter */
long	Old_lpc = 0L;		/* saved load program counter */
int	Loadsp = PSPACE;	/* current load memory space */
int	Lcntr = DEFAULT;	/* load counter type */
int	Lmap = NONE;		/* load memory map flag */

long	End_addr = -1L;		/* Starting address of object module */
int	Pstart = YES;		/* P memory start flag */
char	End_rel = NO;		/* End relative address flag */

int	Pass = 0;		/* Current pass #		*/
FILE	*Fd = NULL;		/* Current input file structure */
int	Cfn = 1;		/* Current file number 1...n	*/
int	Comfiles = 0;		/* total number of command line files */
int	Ccomf = 1;		/* current command line file	*/
char	**Comfp = NULL;		/* list of command line filenames */

int	Force_int = 0;		/* force listing of integer number */
int	Force_fp = 0;		/* force listing line to include F_value */
double	F_value;		/* floating pt value of symbol	*/

int	P_force = 0;		/* force listing line to include Old_pc */
int	P_total = 0;		/* current number of words collected	*/
long	P_words[P_LIMIT];	/* words collected for listing	*/

int	Cycles = 0;		/* # of cycles per instruction	*/
long	Ctotal = 0L;		/* # of cycles seen so far */

int	DestA = NO, DestB = NO; /* duplicate destination register flags */

long	Lkp_count = 0L;		/* lookup count */

long	Regmov = 0L;		/* register move map */
long	Regref = 0L;		/* register reference map */

int	Obj_rtype = OBJNULL;	/* object file record type */
char	Objbuf[MAXEXP] = {0};	/* object file line buffer */

char	Mod_name[MAXSYM+1] = {0};	/* object module name */
int	Mod_vers = 0;		/* object module version */
int	Mod_rev = 0;		/* object module revision */
char	*Mod_com = NULL;	/* object module comment */

int	(*Cmp_rtn) () = NULL;	/* pointer to sort compare routine */

char	*Prctl = NULL;		/* pointer to printer control buffer */
int	Prctl_len = 0;		/* length of printer cntrl buffer contents */

char	*Fldmsg[] =	{Nullstr, /* source code field indicator messages */
			 "Label",
			 "Opcode",
			 "Operand",
			 "X data move",
			 "Y data move"
			};

struct macexp *Macro_exp = NULL;	/* macro expansion list */
struct mac *Macro = NULL;		/* macro list */
int Mac_eof = NO;			/* MACLIB end-of-file flag */
struct symref *Mlib;			/* MACLIB check list */

struct slist *Csect = &Glbsec;		/* current section ptr */
struct slist *Sect_lst = &Glbsec;	/* start of section list chain */
int Secno = 0;				/* section number counter */
struct slist *Last_sec = &Glbsec;	/* end of section list chain */
struct secstk *Sec_stack = NULL;	/* section descriptor stack */

struct fn *F_names = NULL;		/* filename list ptr */
struct filelist *F_stack = NULL;	/* include file stack ptr */
struct inmode *Imode = NULL;		/* top of input mode stack */
struct dostack *Dstk = NULL;		/* pointer to DO loop address stack */

struct nlist *Last_sym	= NULL;		/* last symbol point used in lookup */
struct symlist *Loclab = NULL;		/* start of local label lists */
struct symlist *Curr_lcl = NULL;	/* start of current loc label list */
struct explcl *Exp_lcl = NULL;		/* start of expan. local label list */
struct explcl *Curr_explcl = NULL;	/* current expan. loc label list */
struct explcl *Last_explcl = NULL;	/* last used expan. loc label list */
struct symref *Last_xrf = NULL;		/* last xref point used in lookup */

struct macd *Iextend = NULL;		/* -I command line option strings */
struct dumname *Mextend = NULL;		/* MACLIB directory strings	*/
struct macd *Oextend = NULL;		/* -O command line option strings */

struct fwdref Fhead = {0L, &Fhead};	/* head of forward reference list */
struct fwdref *Fwd = NULL;		/* current forward reference */

struct symlist *Sym_list = NULL;	/* head of SYMOBJ list */
int So_cnt = 0;				/* SYMOBJ count */

struct murec *Lmu = NULL;	/* pointer to load memory utilization list */
struct murec *Rmu = NULL;	/* pointer to runtime mem. utilization list */

char **Sort_tab = NULL;		/* pointer to sort table */

struct nlist **Sym_sort = NULL; /* pointer to symbol sort table */

struct macd *Scs_mdef = NULL;	/* scs expansion list head */
struct macd *Scs_mend = NULL;	/* scs expansion list tail */
unsigned Scs_labcnt = 0;	/* scs label counter */
struct scslab *Scs_lab = NULL;	/* structured control statement label list */
char *Scs_ghdr = "Z_L";		/* scs global label header */
char *Scs_lhdr = "_Z_L";	/* scs local label header */

#if SASM
struct obj Sasmobj;		/* single-line assembler structure */
#endif

FILE	*Objfil = NULL;		/* object file's file pointer */
FILE	*Lstfil = NULL;		/* listing file's file pointer */

#if !PASM
char	Progdef[] = "asm56000"; /* program default name */
char	Progtitle[] = "Motorola DSP56000 Macro Cross Assembler";
char	Version[]   = "Version 3.02";	/* program version number */
#else
char	Progdef[] = "psm56000"; /* program default name */
char	Progtitle[] = "Motorola DSP56000 Pre-assembler";
char	Version[]   = "Version X001a";	/* program version number */
#endif
char	*Progname   = Progdef;	/* pointer to program name */
char	Copyright[] = "(C) Copyright Motorola, Inc. 1987, 1988, 1989.  All rights reserved.";
char	Ipenv[] = "INCPATH";	/* Include path environment variable name */
char	Mlenv[] = "MACLIB";	/* Macro library environment variable name */

/**
*
*	symbol table
*
**/
#if !SASM
struct nlist *Hashtab[HASHSIZE] = {0};	/* symbol table */
#endif

/**
*
*	define table
*
**/
#if !SASM
struct deflist *Dhashtab[HASHSIZE] = {0};   /* define table */
#endif

/**
*
*	external reference table
*
**/
#if !SASM && !PASM
struct symref *Xrftab[HASHSIZE] = {0};
#endif

/**
*
*	Mnemonic table
*
**/
#if !PASM
struct mnemop Mnemonic[] = {
/*
MNE		CLASS		XYCLASS		BASE		CYCLES
*/
"abs",		ACCUMXY,	ALL,		0x000026L,	2,
"adc",		S9D1,		ALL,		0x000021L,	2,
"add",		S11D1AB,	ALL,		0x000000L,	2,
"addl",		ABBA,		ALL,		0x000012L,	2,
"addr",		ABBA,		ALL,		0x000002L,	2,
"and",		ANDOR,		VARIES,		0x000000L,	2,
"andi",		SIMM,		NONE,		0x0000b8L,	2,
"asl",		ACCUMXY,	ALL,		0x000032L,	2,
"asr",		ACCUMXY,	ALL,		0x000022L,	2,
"bchg",		BCLASS,		NONE,		0x0b0000L,	4,
"bclr",		BCLASS,		NONE,		0x0a0000L,	4,
"bset",		BCLASS,		NONE,		0x0a0020L,	4,
"btst",		BTST,		NONE,		0x0b0020L,	4,
"clr",		ACCUMXY,	ALL,		0x000013L,	2,
"cmp",		CMPCLASS,	ALL,		0x000005L,	2,
"cmpm",		CMPCLASS,	ALL,		0x000007L,	2,
"div",		DIVCLASS,	NONE,		0x018040L,	2,
"do",		DOCLASS,	NONE,		0x060000L,	6,
"enddo",	ENDDO,		NONE,		0x00008cL,	2,
"eor",		S10D1,		ALL,		0x000043L,	2,
"illegal",	ILLEGAL,	NONE,		0x000005L,	8,	/* Rev. C */
"jcc",		JCCLASS,	NONE,		0x0a0000L,	4,
"jclr",		JCLRCLS,	NONE,		0x0a0080L,	6,
"jcs",		JCCLASS,	NONE,		0x0a8000L,	4,
"jec",		JCCLASS,	NONE,		0x0a5000L,	4,
"jeq",		JCCLASS,	NONE,		0x0aa000L,	4,
"jes",		JCCLASS,	NONE,		0x0ad000L,	4,
"jge",		JCCLASS,	NONE,		0x0a1000L,	4,
"jgt",		JCCLASS,	NONE,		0x0a7000L,	4,
"jhs",		JCCLASS,	NONE,		0x0a0000L,	4,
"jlc",		JCCLASS,	NONE,		0x0a6000L,	4,
"jle",		JCCLASS,	NONE,		0x0af000L,	4,
"jlo",		JCCLASS,	NONE,		0x0a8000L,	4,
"jls",		JCCLASS,	NONE,		0x0ae000L,	4,
"jlt",		JCCLASS,	NONE,		0x0a9000L,	4,
"jmi",		JCCLASS,	NONE,		0x0ab000L,	4,
"jmp",		JUMP,		NONE,		0x080000L,	4,
"jne",		JCCLASS,	NONE,		0x0a2000L,	4,
"jneq",		JCCLASS,	NONE,		0x0a2000L,	4,
"jnn",		JCCLASS,	NONE,		0x0a4000L,	4,
"jnr",		JCCLASS,	NONE,		0x0ac000L,	4,
"jpl",		JCCLASS,	NONE,		0x0a3000L,	4,
"jscc",		JSCCLAS,	NONE,		0x0b0000L,	4,
"jsclr",	JSCLR,		NONE,		0x0b0080L,	6,
"jscs",		JSCCLAS,	NONE,		0x0b8000L,	4,
"jsec",		JSCCLAS,	NONE,		0x0b5000L,	4,
"jseq",		JSCCLAS,	NONE,		0x0ba000L,	4,
"jses",		JSCCLAS,	NONE,		0x0bd000L,	4,
"jset",		JCLRCLS,	NONE,		0x0a00a0L,	6,
"jsge",		JSCCLAS,	NONE,		0x0b1000L,	4,
"jsgt",		JSCCLAS,	NONE,		0x0b7000L,	4,
"jshs",		JSCCLAS,	NONE,		0x0b0000L,	4,
"jslc",		JSCCLAS,	NONE,		0x0b6000L,	4,
"jsle",		JSCCLAS,	NONE,		0x0bf000L,	4,
"jslo",		JSCCLAS,	NONE,		0x0b8000L,	4,
"jsls",		JSCCLAS,	NONE,		0x0be000L,	4,
"jslt",		JSCCLAS,	NONE,		0x0b9000L,	4,
"jsmi",		JSCCLAS,	NONE,		0x0bb000L,	4,
"jsne",		JSCCLAS,	NONE,		0x0b2000L,	4,
"jsneq",	JSCCLAS,	NONE,		0x0b2000L,	4,
"jsnn",		JSCCLAS,	NONE,		0x0b4000L,	4,
"jsnr",		JSCCLAS,	NONE,		0x0bc000L,	4,
"jspl",		JSCCLAS,	NONE,		0x0b3000L,	4,
"jsr",		JSR,		NONE,		0x090000L,	4,
"jsset",	JSCLR,		NONE,		0x0b00a0L,	6,
"lsl",		ACCUMXY,	ALL,		0x000033L,	2,
"lsr",		ACCUMXY,	ALL,		0x000023L,	2,
"lua",		LUACLASS,	NONE,		0x044010L,	4,
"mac",		MACLASS,	ALL,		0x000082L,	2,
"macr",		MACLASS,	ALL,		0x000083L,	2,
"move",		MOVCLAS,	MOVES,		0x000000L,	2,
"movec",	MOVCCLS,	MOVES,		0x040000L,	2,
"movem",	MOVMCLS,	MOVES,		0x070000L,	2,
"movep",	MOVPCLS,	NONE,		0x084000L,	4,
"mpy",		MPYCLAS,	ALL,		0x000080L,	2,
"mpyr",		MPYCLAS,	ALL,		0x000081L,	2,
"neg",		ACCUMXY,	ALL,		0x000036L,	2,
"nop",		NOP,		NONE,		0x000000L,	2,
"norm",		RD1,		NONE,		0x01d815L,	2,
"not",		ACCUMXY,	ALL,		0x000017L,	2,
"or",		ANDOR,		VARIES,		0x000040L,	2,
"ori",		SIMM,		NONE,		0x0000f8L,	2,
"rep",		REPCLAS,	NONE,		0x060020L,	4,
"reset",	INH,		NONE,		0x000084L,	4,
"rnd",		ACCUMXY,	ALL,		0x000011L,	2,
"rol",		ACCUMXY,	ALL,		0x000037L,	2,
"ror",		ACCUMXY,	ALL,		0x000027L,	2,
"rti",		RTICLASS,	NONE,		0x000004L,	4,
"rts",		RTSCLASS,	NONE,		0x00000cL,	4,
"sbc",		S9D1,		ALL,		0x000025L,	2,
"stop",		INH,		NONE,		0x000087L,	0,
"sub",		S11D1AB,	ALL,		0x000004L,	2,
"subl",		ABBA,		ALL,		0x000016L,	2,
"subr",		ABBA,		ALL,		0x000006L,	2,
"swi",		INHNR,		NONE,		0x000006L,	8,
"tcc",		TFRCC,		NONE,		0x020000L,	2,
"tcs",		TFRCC,		NONE,		0x028000L,	2,
"tec",		TFRCC,		NONE,		0x025000L,	2,
"teq",		TFRCC,		NONE,		0x02a000L,	2,
"tes",		TFRCC,		NONE,		0x02d000L,	2,
"tfr",		S15D1AB,	ALL,		0x000001L,	2,
"tge",		TFRCC,		NONE,		0x021000L,	2,
"tgt",		TFRCC,		NONE,		0x027000L,	2,
"ths",		TFRCC,		NONE,		0x020000L,	2,
"tlc",		TFRCC,		NONE,		0x026000L,	2,
"tle",		TFRCC,		NONE,		0x02f000L,	2,
"tlo",		TFRCC,		NONE,		0x028000L,	2,
"tls",		TFRCC,		NONE,		0x02e000L,	2,
"tlt",		TFRCC,		NONE,		0x029000L,	2,
"tmi",		TFRCC,		NONE,		0x02b000L,	2,
"tne",		TFRCC,		NONE,		0x022000L,	2,
"tneq",		TFRCC,		NONE,		0x022000L,	2,
"tnn",		TFRCC,		NONE,		0x024000L,	2,
"tnr",		TFRCC,		NONE,		0x02c000L,	2,
"tpl",		TFRCC,		NONE,		0x023000L,	2,
"tst",		TSTCLASS,	ALL,		0x000003L,	2,
"wait",		INH,		NONE,		0x000086L,	0
};
#endif /* PASM */

/**
*
*	pseudo op table
*
**/
#if !SASM
struct pseuop Pseudo[] = {
/* name		type		label class */
#if !PASM
"=",		SETD,		SPECIAL,
"baddr",	BADDR,		NOLABEL,
"bsc",		BSC,		LABELOK,
"bsm",		BSM,		SPECIAL,
"bsr",		BSR,		SPECIAL,
"cobj",		COBJ,		NOLABEL,
"comment",	COMMENT,	NOLABEL,
"dc",		DC,		LABELOK,
#endif
"define",	DEFINE,		NOLABEL,
#if !PASM
"ds",		DSTO,		LABELOK,
"dsm",		DSM,		SPECIAL,
"dsr",		DSR,		SPECIAL,
#endif
"dup",		DUP,		LABELOK,
"dupa",		DUPA,		LABELOK,
"dupc",		DUPC,		LABELOK,
"else",		ELSE,		NOLABEL,
#if !PASM
"end",		END,		NOLABEL,
#endif
"endif",	ENDIF,		NOLABEL,
"endm",		ENDM,		NOLABEL,
#if !PASM
"endsec",	ENDSEC,		NOLABEL,
#endif
"equ",		EQU,		SPECIAL,
"exitm",	EXITM,		NOLABEL,
"fail",		FAIL,		NOLABEL,
#if !PASM
"global",	GLOBL,		NOLABEL,
"ident",	IDENT,		SPECIAL,
#endif
"if",		IF,		NOLABEL,
"include",	INCLUDE,	NOLABEL,
#if !PASM
"list",		LIST,		NOLABEL,
"lstcol",	LSTCOL,		NOLABEL,
#endif
"maclib",	MACLIB,		NOLABEL,
"macro",	MACRO,		SPECIAL,
"msg",		MSG,		NOLABEL,
#if !PASM
"nolist",	NOLIST,		NOLABEL,
"opt",		OPT,		NOLABEL,
"org",		ORG,		NOLABEL,
"page",		PAGE,		NOLABEL,
#endif
"pmacro",	PMACRO,		NOLABEL,
#if !PASM
"prctl",	PRCTL,		NOLABEL,
"radix",	RADIX,		NOLABEL,
"rdirect",	RDIR,		NOLABEL,
"section",	SECTION,	NOLABEL,
#endif
"set",		SETD,		SPECIAL,
#if !PASM
"stitle",	STITLE,		NOLABEL,
"symobj",	SYMOBJ,		NOLABEL,
"title",	TITLE,		NOLABEL,
#endif
"undef",	UNDEF,		NOLABEL,
"warn",		WARN,		NOLABEL,
#if !PASM
"xdef",		XDEF,		NOLABEL,
"xref",		XREF,		NOLABEL,
#endif
"xyzzy",	NONE,		NOLABEL
};
#endif /* SASM */

/**
*
*	OPT option table
*
**/
#if !SASM && !PASM
struct opt Option[] = {
/* name		type */
"cc",		CC,
"cex",		CEX,
"cl",		CL,
"cm",		CM,
"contc",	CONTC,
"cre",		CRE,
"dex",		DEX,
"fc",		FC,
"ic",		IC,
"il",		IL,
"intr",		INTR,
"lb",		LB,
"loc",		LOC,
"lw",		LW,
"mc",		MC,
"md",		MD,
"mex",		MEX,
"mi",		MINC,
"msw",		MSW,
"mu",		MU,
"nocc",		NOCC,
"nocex",	NOCEX,
"nocl",		NOCL,
"nocm",		NOCM,
"nodex",	NODEX,
"nofc",		NOFC,
"nointr",	NOINTR,
"nol3",		NOLB,
"nolw",		NOLW,
"nomc",		NOMC,
"nomd",		NOMD,
"nomex",	NOMEX,
"nomi",		NOMINC,
"nomsw",	NOMSW,
"nops",		NOPS,
"norc",		NORC,
"norp",		NORP,
"nou",		NOUNAS,
"now",		NOWRN,
"noxr",		NOXR,
"ps",		PS,
"rc",		RC,
"rp",		RP,
"s",		SYMT,
"so",		SO,
"u",		UNAS,
"w",		WRN,
"xr",		XR,
"xyzzy",	NONE
};
#endif /* SASM && PASM */

#if !SASM
/**
*
*	Expression operator routine table
*
**/
extern struct evres	*bad_op (),	*do_add (),	*do_sub (),
			*do_mul (),	*do_div (),	*do_and (),
			*do_or (),	*do_eor (),	*do_mod (),
			*do_shftl (),	*do_shftr (),	*do_gthan (),
			*do_lthan (),	*do_equl (),	*do_gteql (),
			*do_lteql (),	*do_nequl (),	*do_logand (),
			*do_logor ();

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
do_logor
};
#endif

#if !SASM
/**
*
*	Function table
*
**/
struct fnc Func[] = {
/* name		type		rtn	*/
"abs",		FABS,		fabs,
"acs",		FACS,		acos,
"asn",		FASN,		asin,
"at2",		FAT2,		atan2,
"atn",		FATN,		atan,
"cel",		FCEL,		ceil,
"coh",		FCOH,		cosh,
"cos",		FCOS,		cos,
"cvf",		FCVF,		NULL,
"cvi",		FCVI,		NULL,
"cvs",		FCVS,		NULL,
"def",		FDEF,		NULL,
"exp",		FEXP,		NULL,
"flr",		FFLR,		floor,
"frc",		FFRC,		NULL,
"int",		FINT,		NULL,
"l10",		FL10,		log10,
"lcv",		FLCV,		NULL,
"lfr",		FLFR,		NULL,
"log",		FLOG,		log,
"lst",		FLST,		NULL,
"lun",		FLUN,		NULL,
"mac",		FMAC,		NULL,
"max",		FMAX,		NULL,
"min",		FMIN,		NULL,
"msp",		FMSP,		NULL,
#if MPW || AZTEC
"pow",		FPOW,		power,
#else
"pow",		FPOW,		pow,
#endif
"rel",		FREL,		NULL,
"rnd",		FRND,		NULL,
"scp",		FSCP,		NULL,
"sgn",		FSGN,		NULL,
"sin",		FSIN,		sin,
"snh",		FSNH,		sinh,
"sqt",		FSQT,		sqrt,
"tan",		FTAN,		tan,
"tnh",		FTNH,		tanh,
"unf",		FUNF,		NULL,
"xpn",		FXPN,		exp
};
#endif

#if !SASM && !PASM
/**
*
*	Structured control statement table
*
**/
extern	scs_break (),	scs_else (),	scs_endf (),	scs_endi (),
	scs_endl (),	scs_endw (),	scs_for (),	scs_if (),
	scs_loop (),	scs_repeat (),	scs_until (),	scs_while (),
	scs_continue ();

struct scs Scstab[] = {
/* scsname	scstype		scsrtn	*/
"break",	SCSBRK,		scs_break,
"continue",	SCSCONT,	scs_continue,
"else",		SCSELSE,	scs_else,
"endf",		SCSENDF,	scs_endf,
"endi",		SCSENDI,	scs_endi,
"endl",		SCSENDL,	scs_endl,
"endw",		SCSENDW,	scs_endw,
"for",		SCSFOR,		scs_for,
"if",		SCSIF,		scs_if,
"loop",		SCSLOOP,	scs_loop,
"repeat",	SCSRPT,		scs_repeat,
"until",	SCSUNTL,	scs_until,
"while",	SCSWHL,		scs_while
};
#endif

#if !SASM && !PASM
/**
*
*	Condition code table
*
**/
struct cond Cndtab[] = {
/* ctext	ctype		comp	*/
Nullstr,	NONE,		NONE,
"cc",		CNDCC,		CNDCS,
"cs",		CNDCS,		CNDCC,
"ec",		CNDEC,		CNDES,
"eq",		CNDEQ,		CNDNE,
"es",		CNDES,		CNDEC,
"ge",		CNDGE,		CNDLT,
"gt",		CNDGT,		CNDLE,
"hs",		CNDHS,		CNDLO,
"lc",		CNDLC,		CNDLS,
"le",		CNDLE,		CNDGT,
"lo",		CNDLO,		CNDHS,
"ls",		CNDLS,		CNDLC,
"lt",		CNDLT,		CNDGE,
"mi",		CNDMI,		CNDPL,
"ne",		CNDNE,		CNDEQ,
"nn",		CNDNN,		CNDNR,
"nr",		CNDNR,		CNDNN,
"pl",		CNDPL,		CNDMI
};
#endif

/**
*
*	table size definitions
*
**/
#if !PASM
int Nmne = sizeof(Mnemonic) / sizeof(struct mnemop);
#endif
#if !SASM
int Npse = sizeof(Pseudo)   / sizeof(struct pseuop);
#if !PASM
int Nopt = sizeof(Option)   / sizeof(struct opt);
int Nscs = sizeof(Scstab)   / sizeof(struct scs);
int Ncnd = sizeof(Cndtab)   / sizeof(struct cond);
#endif
#endif
#if !SASM
int Nfnc = sizeof(Func)	    / sizeof(struct fnc);
#endif
