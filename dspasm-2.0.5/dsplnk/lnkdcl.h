/* %W% %G% */	/* SCCS data */

/**
*
* DSPASM external declarations
*
**/

extern char	Gagerrs;
extern char	Dcl_flag;
extern char	Debug;
extern char	Xrf_flag;
extern char	User_exp;
extern char	Con_flag;
extern char	Loc_flag;
extern char	Sea_flag;
extern char	Sen_flag;
extern char	Syn_flag;
extern char	Syv_flag;
extern char	Cflag;
extern char	Icflag;
extern char	Verbose;
extern char	Testing;

extern int	Pass;
extern int	Symcnt;
extern int	Secnt;
extern int	Sectot;
extern int	Secno;
extern int	Xrfcnt;
extern int	Lineno;
extern int	Recno;
extern int	Fldno;
extern int	Err_count;
extern int	Warn_count;
extern int	Gagcnt;
extern int	Vcount;
extern int	Radix;
extern long	End_addr;
extern long	Recoff;
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
extern int	Lst_topm;
extern int	Lst_botm;

extern char	Field[];
extern char	String[];
extern char	Mapfn[];
extern char	Curdate[];
extern char	Curtime[];
extern char	*Optr;

#if VAX
extern struct dsc$descriptor_s	Arg_desc;
extern struct dsc$descriptor_s	Cc_desc;
extern struct dsc$descriptor_s	Dbg_desc;
extern struct dsc$descriptor_s	Lib_desc;
extern struct dsc$descriptor_s	Lnk_desc;
extern struct dsc$descriptor_s	Map_desc;
extern struct dsc$descriptor_s	Mem_desc;
extern struct dsc$descriptor_s	Noc_desc;
extern struct dsc$descriptor_s	Obj_desc;
extern struct dsc$descriptor_s	Org_desc;
extern struct dsc$descriptor_s	Tst_desc;
extern struct dsc$descriptor_s	Vbs_desc;
#else
extern Lnk_desc;
extern Cc_desc;
#endif

#if LSC
extern jmp_buf	Env;
#endif

extern long	Start[][MCNTRS];
extern int	Mflags[][MCNTRS];
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

extern int	Comfiles;
extern int	Ccomf;
extern char	**Comfp;
extern FILE	*Fd;
extern struct file *Files;
extern struct file *Cfile;

extern int	Obj_rtype;
extern char	Objbuf[];

extern char	*End_exp;

extern char	*Mod_name;
extern int	Mod_vers;
extern int	Mod_rev;
extern char	*Mod_com;

extern int	(*Cmp_rtn) ();

extern struct slist *Csect;
extern struct smap *Secmap;
extern struct slist *Seclst;
extern struct slist *Secptr;

extern struct slist *Last_sec;
extern struct nlist *Last_sym;
extern struct symref *Last_xrf;

extern char **Sort_tab;
extern int Sortmem;
extern int Sortctr;
extern struct slist **Sec_sort;
extern struct nlist **Sym_sort;
extern struct symref **Xrf_sort;

extern FILE	*Outfil;
extern FILE	*Lodfil;
extern FILE	*Mapfil;
extern FILE	*Memfil;

extern char	Progdef[];
extern char	*Progname;
extern char	Progtitle[];
extern char	Version[];
extern char	Copyright[];
extern char	Gsecname[];
extern char	Modhdr[];
extern int	Modlen;

extern struct rectype Rectab[];
extern int	Rectsize;

extern struct rectype Memtab[];
extern int	Memtsize;

extern struct rectype Opttab[];
extern int	Opttsize;

extern struct nlist *Symtab[];

extern struct slist *Sectab[];

extern struct symref *Xrftab[];

extern struct evres *(*Exop_rtn[]) ();

char *strcpy (), *strncpy ();
char *strcat (), *strncat ();
#if BSD
char *sprintf ();
char *index (), *rindex ();
#else
char *strchr (), *strrchr ();
#endif

double *alloc();	/* declare double for worst case alignment (lint) */
char mapup(), mapdn(), spc_char(), map_char();
char *strup(), *strdn();

#if MSDOS			/* reference DOS version number */
#if MSC
extern unsigned char _osmajor;
#endif
#endif
