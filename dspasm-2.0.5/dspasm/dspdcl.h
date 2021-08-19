/* %W% %G% */	/* SCCS data */

/**
*
* DSPASM external declarations
*
**/

extern char	Fcflag;
extern char	Muflag;
extern char	Mu_org;
extern char	End_flag;
extern char	Unasm_flag;
extern char	Prtc_flag;
extern char	Pmacd_flag;
extern char	Mc_flag;
extern char	Mex_flag;
extern char	Cm_flag;
extern char	Ilflag;
extern char	Cyc_flag;
extern char	Cre_flag;
extern char	Cre_err;
extern char	S_flag;
extern char	So_flag;
extern char	Cex_flag;
extern char	Loc_flag;
extern char	Wrn_flag;
extern char	Gagerrs;
extern char	Abortp;
extern char	Force_prt;
extern char	Ctlreg_flag;
extern char	Emit;
extern char	Rcom_flag;
extern char	Def_flag;
extern char	Swap;
extern char	Chkdo;
extern char	Laerr;
extern char	Dcl_flag;
extern char	Lnkmode;
extern char	Relmode;
extern char	Local;
extern char	Xrf_flag;
extern char	Mi_flag;
extern char	Pack_flag;
extern char	Int_flag;
extern char	Cflag;
extern char	Cnlflag;
extern char	Strtlst;
extern char	Lwflag;
extern char	Icflag;
extern char	Ic_err;
extern char	Lbflag;
extern char	Dex_flag;
extern char	Msw_flag;
extern char	Ro_flag;
extern char	Lo_flag;
extern char	Rpflag;
extern char	Padreg;
extern char	Verbose;
extern char	Testing;

extern int	Sym_cnt;
extern int	Lsym_cnt;
extern int	Xrf_cnt;
extern int	Line_num;
extern int	Real_ln;
extern int	Lst_lno;
extern int	Err_count;
extern int	Warn_count;
extern int	Line_err;
extern int	Gagcnt;
extern int	No_defs;
extern int	Vcount;
extern int	Radix;
extern int	Bad_flags;
extern int	Prev_flags;
extern long	Seed;
extern int	Lflag;
#if VMS
extern union	vmscond Vmserr;
#endif

extern int	Page;
extern int	Lst_col;
extern int	Lst_char;
extern int	Max_col;
extern int	Start_col;
extern int	Lst_line;
extern int	Max_line;
extern int	Lst_p_len;
extern int	Formflag;
extern int	Page_flag;
extern int	Lst_topm;
extern int	Lst_botm;
extern int	Labwidth;
extern int	Opwidth;
extern int	Operwidth;
extern int	Xwidth;
extern int	Ywidth;

extern char	*Title;
extern char	*Subtitle;

extern char	Nullstr[];
extern char	Line[];
extern char	Pline[];
extern char	String[];
extern char	Expr[];
extern char	Curdate[];
extern char	Curtime[];
extern char	*Cfname;
extern char	*Label;
extern char	*Op;
extern char	*Operand;
extern char	*Xmove;
extern char	*Ymove;
extern char	*Com;
extern char	*Optr;
extern char	*Eptr;
extern char	*Hexstr;
extern char	*Lngstr;

#if VAX
extern struct dsc$descriptor_s	Arg_desc;
extern struct dsc$descriptor_s	Abs_desc;
extern struct dsc$descriptor_s	Cc_desc;
extern struct dsc$descriptor_s	Def_desc;
extern struct dsc$descriptor_s	Inc_desc;
extern struct dsc$descriptor_s	Lst_desc;
extern struct dsc$descriptor_s	Mac_desc;
extern struct dsc$descriptor_s	Obj_desc;
extern struct dsc$descriptor_s	Opt_desc;
extern struct dsc$descriptor_s	Src_desc;
extern struct dsc$descriptor_s	Tst_desc;
extern struct dsc$descriptor_s	Vbs_desc;
#else
extern Src_desc;
extern Cc_desc;
#endif

#if LSC
extern jmp_buf	Env;
#endif

extern long	Gcntrs[][MCNTRS][MBANKS];

extern long	Glbcntrs[][MCNTRS][MBANKS-1];

extern struct slist Glbsec;

extern long	*Pc;
extern long	Old_pc;
extern int	Cspace;
extern int	Rcntr;
extern int	Rmap;
extern long	*Lpc;
extern long	Old_lpc;
extern int	Loadsp;
extern int	Lcntr;
extern int	Lmap;

extern long	End_addr;
extern int	Pstart;
extern char	End_rel;

extern int	Pass;
extern FILE	*Fd;
extern int	Cfn;
extern int	Comfiles;
extern int	Ccomf;
extern char	**Comfp;

extern int	Force_int;
extern int	Force_fp;
extern double	F_value;

extern int	P_force;
extern int	P_total;
extern long	P_words[];

extern int	Cycles;
extern long	Ctotal;

extern int	DestA, DestB;

extern long	Lkp_count;

extern long	Regmov;
extern long	Regref;

extern int	Obj_rtype;
extern char	Objbuf[];

extern char	Mod_name[];
extern int	Mod_vers;
extern int	Mod_rev;
extern char	*Mod_com;

extern int	(*Cmp_rtn) ();

extern char	*Prctl;
extern int	Prctl_len;

extern char	*Fldmsg[];

extern struct macexp *Macro_exp;
extern struct mac *Macro;
extern int Mac_eof;
extern struct symref *Mlib;

extern struct slist *Csect;
extern struct slist *Sect_lst;
extern int Secno;
extern struct slist *Last_sec;
extern struct secstk *Sec_stack;

extern struct fn *F_names;
extern struct filelist *F_stack;
extern struct inmode *Imode;
extern struct dostack *Dstk;

extern struct nlist *Last_sym;
extern struct symlist *Loclab;
extern struct symlist *Curr_lcl;
extern struct explcl *Exp_lcl;
extern struct explcl *Curr_explcl;
extern struct explcl *Last_explcl;
extern struct symref *Last_xrf;

extern struct macd *Iextend;
extern struct dumname *Mextend;
extern struct macd *Oextend;

extern struct fwdref Fhead;
extern struct fwdref *Fwd;

extern struct symlist *Sym_list;
extern int So_cnt;

extern struct murec *Lmu;
extern struct murec *Rmu;

extern char **Sort_tab;

extern struct nlist **Sym_sort;

extern struct macd *Scs_mdef;
extern struct macd *Scs_mend;
extern unsigned Scs_labcnt;
extern struct scslab *Scs_lab;
extern char *Scs_ghdr;
extern char *Scs_lhdr;

#if SASM
extern struct obj Sasmobj;
#endif

extern FILE	*Objfil;
extern FILE	*Lstfil;

extern char	Progdef[];
extern char	Progtitle[];
extern char	Version[];
extern char	*Progname;
extern char	Copyright[];
extern char	Ipenv[];
extern char	Mlenv[];

extern struct nlist *Hashtab[];

extern struct deflist *Dhashtab[];

extern struct symref *Xrftab[];

extern struct mnemop Mnemonic[];

extern struct pseuop Pseudo[];

extern struct opt Option[];

extern struct evres *(*Exop_rtn[]) ();

extern struct fnc Func[];

extern struct scs Scstab[];

extern struct cond Cndtab[];

extern int Nmne;
extern int Npse;
extern int Nopt;
extern int Nscs;
extern int Ncnd;
extern int Nfnc;

char *strcpy (), *strncpy ();
char *strcat (), *strncat ();
#if BSD
char *sprintf ();
char *index (), *rindex ();
#else
char *strchr (), *strrchr ();
#endif

void *alloc();
void *binsrch();
char mapup(), mapdn(), spc_char(), ctr_char(), map_char();
char *strup(), *strdn();
void print_line(), print_comm();

#if MSDOS		/* reference DOS version number */
#if MSC
extern unsigned char _osmajor;
#endif
#endif
