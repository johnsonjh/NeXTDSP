#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		do_pseudo --- do pseudo op processing
*
* synopsis	yn = do_pseudo(op)
*		int yn;			YES if no error / NO if error
*		struct oper *op;	pointer to pseudo op table entry
*
* description	This is the main pseudo op dispatcher. It takes a pointer
*		to the pseudo op table entry corresponding to the directive
*		found in the current source line, and calls the appropriate
*		routine to handle the directive processing.
**/
do_pseudo(op)
struct pseuop *op;
{
	char *s;
	char *get_i_string();

	dopsulab(op->plclass);

	Optr = Operand;		/* set up Optr */
	switch(op->ptype){
		case BADDR:			/* Baddr directive */
			return (do_baddr ());
		case BSC:			/* Bsc directive */
			return (do_bsc ());
		case BSM:			/* Bsm directive */
			return (do_bsmr (MUMOD));
		case BSR:			/* Bsr directive */
			return (do_bsmr (MUREV));
		case COBJ:			/* Cobj directive */
			if (!get_i_string (Optr, String))
				return (NO);
			emit_comment (String);
			return (chk_flds (1));
		case COMMENT:			/* Comment directive */
			return(do_comment());
		case DC:			/* Dc directive */
			return(do_dc());
		case DEFINE:			/* Define directive */
			(void)do_define(Operand,Xmove,NO);
			return(chk_flds(2));
		case DSTO:			/* Ds directive */
			return(do_ds());
		case DSM:			/* Dsm directive */
			return(do_dsmr(MUMOD));
		case DSR:			/* Dsr directive */
			return(do_dsmr(MUREV));
		case DUP:			/* Dup directive */
			return(do_dup());
		case DUPA:			/* Dupa directive */
			return(do_dupa());
		case DUPC:			/* Dupc directive */
			return(do_dupc());
		case ELSE:			/* Else directive */
			Optr = NULL;
			error("ELSE without associated IF directive");
			return(NO);
		case END:			/* End directive */
			return(do_end());
		case ENDIF:			/* Endif directive */
			Optr = NULL;
			error("ENDIF without associated IF directive");
			return(NO);
		case ENDM:			/* Endm directive */
			Optr = NULL;
			error("ENDM without associated MACRO directive");
			return(NO);
		case ENDSEC:			/* Endsec directive */
			return(do_endsec());
		case EQU:			/* Equate directive */
			return(do_equate());
		case EXITM:			/* Exitm directive */
			if( Macro_exp == NULL ){
				Optr = NULL;
				error("EXITM without associated MACRO directive");
				return(NO);
				}
			Macro_exp->defline = NULL; /* set end of macro expansion */
			return(chk_flds(0));
		case FAIL:			/* Fail directive */
			if( *Operand == EOS )
				error(Nullstr);
			else{
				if ((s = (char *)alloc(MAXBUF+1)) == NULL)
					fatal("Out of memory - cannot allocate fail message");
				if( get_i_string(Operand,s) != NULL ){
					Optr = NULL;
					error(s);
					Optr = Operand;
					}
				free(s);
				}
			return(chk_flds(1));
		case GLOBL:			/* Global directive */
			return(do_global());
		case IDENT:			/* Ident directive */
			return(do_ident());
		case IF:			/* If directive */
			return(do_if());
		case INCLUDE:			/* Include directive */
			(void)do_include();
			return(chk_flds(1));
		case LIST:			/* List directive */
			++Lflag;
			Abortp = YES;		/* don't print directive */
			return(chk_flds(0));
		case LSTCOL:			/* Lstcol directive */
			if( *Operand == EOS ){	/* reset column defaults */
				Labwidth = DEFLABFW;
				Opwidth = DEFOPCFW;
				Operwidth = DEFOPRFW;
				Xwidth = DEFXFLDW;
				Ywidth = DEFYFLDW;
				}
			else{
				(void)chk_flds(1);
				if( do_lstcol() == NO )
					return(NO);
				}
			return(YES);
		case MACLIB:			/* Maclib directive */
			return(do_maclib(Operand,NO));
		case MACRO:			/* Macro directive */
			return(def_macro());
		case MSG:			/* Msg directive */
			if( *Operand == EOS )
				msg(Nullstr);
			else{
				if ((s = (char *)alloc(MAXBUF+1)) == NULL)
					fatal("Out of memory - cannot allocate message");
				if( get_i_string(Operand,s) != NULL ){
					Optr = NULL;
					msg(s);
					Optr = Operand;
					}
				free(s);
				}
			return(chk_flds(1));
		case NOLIST:			/* Nolist directive */
			--Lflag;
			Abortp = YES;		/* don't print directive */
			return(chk_flds(0));
		case OPT:			/* assembler option */
			return(do_option(Operand));
		case ORG:		       /* org */
			(void)do_org();
			return(chk_flds(1));
		case PAGE:			/* Page directive */
			if( *Operand == EOS ){
				if( Page_flag && Pass == 2 &&
				    !Ilflag && Lflag > 0 ) {
					if( !Macro_exp || Mex_flag )
						page_eject(YES);
					Abortp = YES;
					}
				}
			else{
				(void)chk_flds(1);
				if( do_page() == NO )
					return(NO);
				}
			return(YES);
		case PMACRO:			/* Pmacro directive */
			return(do_pmacro());
		case PRCTL:			/* Prctl directive */
			return(do_prctl());
		case RADIX:			/* Radix directive */
			return(do_radix());
		case RDIR:			/* Rdirect directive */
			return(do_rdirect());
		case SECTION:			/* section directive */
			(void)do_sect(Operand);
			return(chk_flds(1));
		case SETD:			/* set or = directive */
			return(do_set());
		case STITLE:			/* Sub-title directive */
			return(do_title(&Subtitle));
		case SYMOBJ:			/* Symobj directive */
			return(do_symobj());
		case TITLE:			/* Title directive */
			return(do_title(&Title));
		case UNDEF:			/* Undef directive */
			(void)do_undef(Operand);
			return(chk_flds(1));
		case WARN:			/* Warn directive */
			if( *Operand == EOS )
				warn(Nullstr);
			else{
				if ((s = (char *)alloc(MAXBUF+1)) == NULL)
					fatal("Out of memory - cannot allocate warn message");
				if( get_i_string(Operand,s) != NULL ){
					Optr = NULL;
					warn(s);
					Optr = Operand;
					}
				free(s);
				}
			return(chk_flds(1));
		case XDEF:			/* Xdef directive */
			return(do_xdef());
		case XREF:			/* Xref directive */
			return(do_xref());
		case NONE:
			return(YES);
		default:
			fatal("Directive select error");
			return(NO);
		}
}

/**
*
* name		dopsulab - do pseudo op label
*
* synopsis	dopsulab(labcls)
*		int labcls;	NOLABEL = error if label present
*				SPECIAL = label handled by pseudo op
*				LABELOK = install label
*
* description	Handles label processing for pseudo ops.
*
**/
dopsulab(labcls)
int labcls;
{
	if( labcls == LABELOK )
		do_label();
	else if( labcls == NOLABEL && *Label != EOS )
		warn("Label field ignored");
}

/**
*
* name		do_define --- perform define directive processing
*
**/
do_define(sname,dstring,cmd)
char *sname,*dstring;
int cmd;
{
	char string[MAXBUF];
	char *get_i_string();

	if( !*sname ){
		error("Missing symbol name");
		return(NO);
		}
	if( get_i_sym(sname) == ERR )
		return(NO);
	if( *sname == '_' ){
		error2("DEFINE symbol must be a global symbol name",sname);
		return(NO);
		}
	if( get_i_string(dstring,string) == NULL ){
		error("Missing definition string");
		return(NO);
		}
	if (def_inst(sname,string,cmd)) {
		Def_flag = YES;
		return (YES);
	} else
		return (NO);
}

/**
*
* name		do_undef --- perform UNDEF directive processing
*
* synopsis	yn = do_undef(sname)
*		int yn;		YES if symbol is UNDEFed, NO otherwise
*		char *sname;	pointer to symbol name
*
* description	Check the validity of the symbol name, then remove it
*		from the DEFINE hash table.
*
**/
static
do_undef(sname)
char *sname;
{
	if( !*sname ){
		error("Missing symbol name");
		return(NO);
		}
	if( get_i_sym(sname) == ERR )
		return(NO);
	if( *sname == '_' ){
		error2("UNDEF symbol must be a global symbol name",sname);
		return(NO);
		}
	return(def_rem(sname));
}

/**
*
* name		do_ident --- perform ident directive processing
*
* description	If label is present, save as module name.  Scan operand
*		field for version and revision numbers.	 Save comment if
*		present.
**/
static
do_ident ()
{
#if !PASM
	static found = NO;
	register char *p;
	struct evres *ev, *eval_int ();

	if (found)		/* already found first IDENT */
		return (YES);

	if (*Label != EOS)	/* save label as module name */
		if (get_i_sym (Label) == ERR)
			return (NO);
		else {
			while (*Label && *Label == '_')
				Label++; /* strip leading underscores */
			(void)strcpy (Mod_name, Label);
		}

	if (*Optr == EOS) {
		error ("IDENT directive must contain version number");
		return (NO);
	}
	for (p = Optr; *p && *p != ','; p++)
		;
	if (!*p) {
		error ("IDENT directive must contain revision number");
		return (NO);
	}
	*p = EOS;	/* replace comma with EOS temporarily */

	if (!(ev = eval_int ()))
		return (NO);
	Mod_vers = (int)ev->uval.xvalue.low; /* save version number */
	free_exp (ev);
	Optr = p + 1;
	if (!(ev = eval_int ()))
		return (NO);
	Mod_rev = (int)ev->uval.xvalue.low; /* save revision number */
	free_exp (ev);
	*p = ',';	/* put comma back */

	if (*Com != EOS) { /* save comment */
		if ( !(Mod_com = (char *)alloc (strlen (Com) + 1)) )
			fatal ("Out of memory - cannot save IDENT comment");
		(void)strcpy (Mod_com, Com);
	}
	found = YES;
	return (chk_flds (1));
#endif /* PASM */
}

/**
*
* name		do_include --- perform include directive processing
*
* description	When an include directive is encountered, the current
*		file status is stacked into a dynamically allocated
*		filelist structure pointed to by F_stack.
**/
static
do_include()
{
	char *get_i_string();
	struct filelist *np;
	FILE *fld;
	struct macd *ip;
	struct dumname *op;
	char str[MAXBUF+1];

	if( get_i_string(Operand,str) == NULL ){
		error("Missing filename");
		return(NO);
		}

	(void)strcpy(String,str);
	fix_fn(".asm"); /* supply default extension if needed */
	if( (fld = fopen(String,"r")) == NULL ){
		for( ip = Iextend ; ip ; ip = ip->mnext ){
			(void)strcpy(String,ip->mline); /* put the -I option string in String */
			(void)strcat(String,str);	/* concatenate the filename */
			fix_fn(".asm"); /* supply default extension */
			if( (fld = fopen(String,"r")) != NULL )
				break;
			}
		if( !ip && Mi_flag ){	/* scan MACLIB paths too */
			for( op = Mextend ; op ; op = op->dnext ){
				(void)strcpy(String,op->daname); /* put the MACLIB string in String */
				(void)strcat(String,str);	/* concatenate the filename */
				fix_fn(".asm"); /* supply default extension */
				if( (fld = fopen(String,"r")) != NULL )
					break;
				}
			}
		}
	if( fld ){
		if (Verbose)
			(void)fprintf (stderr, "%s: Opening include file %s\n", Progname, String);
		}
	else{	/* could not open file */
		error2("Cannot open include file",str);
		return(NO);
		}

#if PASM
	comm_line();	/* comment INCLUDE directive line */
	print_line(' '); /* print the directive line */
	Abortp = YES;
#endif
	if (!(np = (struct filelist *)alloc(sizeof (struct filelist))))
		fatal("Out of memory - cannot save filename");
	np->ifname = Cfname;	/* save current filename */
	np->ifd = Fd;		/* save file descriptor */
	np->fnum = Cfn;		/* save current file number */
	np->iln = Real_ln;	/* save current line number */
	np->prevline = NULL;	/* no previous line */
	np->inext = F_stack;	/* insert into stack */
	F_stack = np;
	++Cfn;			/* bump file number */
	set_cfn(String);	/* setup current filename */
	Fd = fld;		/* setup file descriptor */
	Real_ln = 0;		/* initialize line number */
	push_imode (FILEMODE);	/* change input mode */
	return(YES);
}

/**
*
* name		do_maclib --- perform maclib directive processing
*
* description	When a maclib directive is encountered, the pathname
*		argument in the path argument is put on a linked list
*		of directory names to be scanned when a macro invocation
*		name is not found in the macro lookup tables.
*
**/
do_maclib (path,cmd)
char *path;
int cmd;
{
	struct dumname *np;
	char *get_i_string();

	if (Pass == 2)		/* already have path on second pass */
		return (YES);

	if (cmd)
		(void)strcpy (String, path);
	else
		if( get_i_string(path,String) == NULL ){
			error("Missing pathname");
			return(NO);
			}

	fix_path ();
	if ( !(np = (struct dumname *)alloc(sizeof(struct dumname))) ||
	     !(np->daname = (char *)alloc(strlen(String)+1)) )
		fatal("Out of memory - cannot save MACLIB path");

	(void)strcpy(np->daname,String);
	np->num = cmd;
	np->dnext = Mextend; /* insert into list */
	Mextend = np;

	return(YES);
}

/**
*
* name		do_incpath --- process include file path
*
* description	When a -I command line option is encountered, the pathname
*		argument in the path argument is put on a linked list
*		of directory names to be scanned when an include file
*		name is not found in the current directory.
*
**/
do_incpath (path)
char *path;
{
	struct macd *np;

	if (Pass == 2)		/* already have path on second pass */
		return (YES);

	(void)strcpy (String, path);
	fix_path ();
	if ( !(np = (struct macd *)alloc(sizeof(struct macd))) ||
	     !(np->mline = (char *)alloc(strlen(String)+1)) )
		fatal("Out of memory - cannot save INCLUDE path");

	(void)strcpy(np->mline,String);
	np->mnext = Iextend; /* insert into list */
	Iextend = np;

	return(YES);
}

/**
*
* name		get_cntrs - get location counter base
*
**/
static long *
get_cntrs(s,c,b)
int s; /* space */
int c; /* counter */
int b; /* bank */
{
	if ((s = spc_off (s)) == NONE || (c = ctr_off (c)) == ERR)
		fatal ("Location counter selection failure");
	return (Relmode ? &Csect->cntrs[s-1][c][b] : &Gcntrs[s-1][c][b]);
}

/**
*
* name		do_org --- do ORG directive
*
**/
static
do_org()
{
#if !PASM
	struct evres *rev = NULL;	/* runtime init */
	int rms;			/* runtime memory space */
	int rct;			/* runtime counter type */
	int rmp = NONE;			/* runtime map flag */
	long *rlc;			/* runtime location counter */
	struct evres *lev;		/* load init */
	int lms;			/* load memory space */
	int lct;			/* load counter type */
	int lmp = NONE;			/* load map flag */
	long *llc;			/* load location counter ptr */
	int i, j;
	char c;
	long *get_cntrs ();
	struct evres *eval_ad();

	P_force = YES;

	if( (rms = char_spc(*Optr++)) == ERR || rms == NONE ){
		error("Illegal memory space specified");
		return(NO);
		}

	if( (rct = char_ctr (*Optr)) != ERR )	/* look for counter */
		Optr++;
	else
		rct = DEFAULT;

	switch ( c = mapdn(*Optr) ){	/* look for mapping character */
		case 'i':
		case 'e':
		case 'b':
			Optr++;
			if( *Optr++ != ':' ){
				error("Syntax error in operand field");
				return(NO);
				}
			if( (rmp = char_map(c)) == ERR){
				error("Illegal memory map character");
				return(NO);
				}
			if( rmp == BMAP && rms != PSPACE ){
				error("Bootstrap mapping available only in P memory");
				return(NO);
				}
			break;
		case ':':
			Optr++;
		/* fall through */
		case EOS:
			break;
		default:
			error("Syntax error in operand field");
			return(NO);
		}

	if (Relmode && *Optr != EOS) {
		if (Csect->flags & RELERR) {
			error ("Invalid start of absolute section");
			return (NO);
		}
		for (i = 0; i < MSPACES; i++)
			for (j = 0; j < MCNTRS; j++)
				if (Csect->cntrs[i][j][RBANK]) {
					error ("Invalid start of absolute section");
					Csect->flags |= RELERR;
					return (NO);
				}
		Relmode = NO;		/* reset mode */
		Csect->flags &= ~REL;
	}

	rlc = get_cntrs(rms,rct,RBANK);

	if( *Optr == EOS ){
		Lpc = Pc = rlc; /* load Pc same as runtime */
		Old_lpc = *Lpc; /* save old counter values */
		Old_pc = *Pc;
		Ro_flag = Lo_flag = NO; /* reset overflow flags */
		Loadsp = Cspace = rms;	/* load mem space same as runtime */
		Lcntr = Rcntr = rct;	/* load counter same as runtime */
		Lmap = Rmap = rmp;	/* load map same as runtime */
		Mu_org = YES;
		return(YES);
		}

	if( *Optr == ',' )
		Optr++;
	else{
		if( (rev = eval_ad()) == NULL )
			return(NO);

		if( rev->fwd_ref ){
			error("Expression contains forward references");
			free_exp(rev); /* release expression result */
			return(NO);
			}

		if( *Optr == ',' )
			Optr++;
		else if( *Optr == EOS ){
			*rlc = rev->uval.xvalue.low; /* update counter to new value */
			Lpc = Pc = rlc; /* load Pc same as runtime */
			Old_lpc = *Lpc; /* save old counters */
			Old_pc = *Pc;
			Ro_flag = Lo_flag = NO; /* reset overflow flags */
			Loadsp = Cspace = rms;	/* load mem sp same as run */
			Lcntr = Rcntr = rct;	/* load cntr same as run */
			Lmap = Rmap = rmp;	/* load map same as run */
			Mu_org = YES;
			return(YES);
			}
		}

	if( (lms = char_spc(*Optr++)) == ERR ){
		error("Illegal memory space specified");
		return(NO);
		}

	if( rms == LSPACE ){
		if( lms != LSPACE ){
			error("L space specified for runtime, but not for load");
			return(NO);
			}
		}

	if( lms == LSPACE ){
		if( rms != LSPACE ){
			error("L space specified for load, but not for runtime");
			return(NO);
			}
		}

	if( (lct = char_ctr (*Optr)) != ERR )	/* look for counter */
		Optr++;
	else
		lct = DEFAULT;

	switch ( c = mapdn(*Optr) ){	/* look for mapping character */
		case 'i':
		case 'e':
		case 'b':
			Optr++;
			if( *Optr++ != ':' ){
				error("Syntax error in operand field");
				return(NO);
				}
			if( (lmp = char_map(c)) == ERR){
				error("Illegal memory map character");
				return(NO);
				}
			if( lmp == BMAP && lms != PSPACE ){
				error("Bootstrap mapping available only in P memory");
				return(NO);
				}
			break;
		case ':':
			Optr++;
		/* fall through */
		case EOS:
			break;
		default:
			error("Syntax error in operand field");
			return(NO);
		}

	llc = get_cntrs(lms,lct,LBANK);

	if( *Optr == EOS ){
		if( rev != NULL ){
			*rlc = rev->uval.xvalue.low;
			free_exp(rev);
			}
		Lpc = llc; /* update load Pc */
		Old_lpc = *Lpc;
		Pc = rlc; /* update runtime  */
		Old_pc = *Pc;
		Ro_flag = Lo_flag = NO; /* reset overflow flags */
		Loadsp = lms;	/* update load memory space */
		Cspace = rms;	/* update runtime memory space */
		Lcntr = lct;	/* update load and runtime counter types */
		Rcntr = rct;
		Rmap = rmp;	/* update load and runtime mappings */
		Lmap = lmp;
		Mu_org = YES;
		return(YES);
		}

	if( (lev = eval_ad()) == NULL ){
		if( rev != NULL )
			free_exp(rev);
		return(NO);
		}

	if( lev->fwd_ref ){
		error("Expression contains forward references");
		free_exp(lev); /* release expression result */
		if( rev != NULL )
			free_exp(rev);
		return(NO);
		}

	if( *Optr != EOS ){
		error("Syntax error in operand field - extra characters");
		free_exp(lev);
		if( rev != NULL )
			free_exp(rev);
		return(NO);
		}

	if( rev != NULL ){
		*rlc = rev->uval.xvalue.low; /* update counter to new value */
		free_exp(rev);
		}

	*llc = lev->uval.xvalue.low; /* update load counter */
	free_exp(lev);

	Lpc = llc; /* update load Pc */
	Old_lpc = *Lpc;
	Pc = rlc; /* update runtime */
	Old_pc = *Pc;
	Ro_flag = Lo_flag = NO; /* reset overflow flags */
	Loadsp = lms;	/* update load memory space */
	Cspace = rms;	/* update runtime memory space */
	Lcntr = lct;	/* update load and runtime counter types */
	Rcntr = rct;
	Rmap = rmp;	/* update load and runtime mappings */
	Lmap = lmp;
	Mu_org = YES;
	return(YES);
#endif /* PASM */
}

/**
*
* name		do_sect --- do section directive
*
* description	Section tag symbols are saved in a linked list of slist
*		structures. On pass 1, when the section directives
*		are processed, the section name is looked up in the
*		section list. If it is found, it is not entered into the
*		table.	If the section list does not contain the
*		indicated section then it is installed in the
*		section list. In either case, the Csect pointer is
*		updated to point at the current section descriptor. On pass 2,
*		if the section is not found in the section list, then an
*		error has occurred. If the section is found in the list,
*		then Csect pointer is updated.	If there exists an
*		active section, then the section ptr is put into the
*		section stack.
*
**/
do_sect(name)
char *name;
{
#if !PASM
	struct slist *sp;
	int	i, j;
	char	buf[MAXSYM+1];

	if( (i = get_i_sym(name)) == ERR )
		return(NO);
	if( i == NO ){
		error("Missing section name");
		return(NO);
		}

	if( *name == '_' || STREQ (name, Glbsec.sname) ){
		error2("Invalid section name",name);
		return(NO);
		}

	if( Icflag ){		/* ignore case */
		(void)strcpy (buf, name);
		(void)strdn (buf);
		name = buf;
		}
	for (sp = Sect_lst->next; sp; sp = sp->next)	/* look for section */
		if( STREQ(name,sp->sname) )
			break;
	if( sp ){		/* found the section in the list */
		save_sec();	/* save current section */
		restore_sec(sp);/* restore new section */
		return(YES);
		}

	if( Pass == 2 ){
		error2("Section not encountered on pass 1",name);
		return(NO);
		}

	/* install new section */
	if (!(sp = (struct slist *)alloc(sizeof(struct slist))))
		fatal("Out of memory - cannot allocate section descriptor");
	save_sec();			/* save current section */
	Csect = sp;			/* set new section */
	if (!(sp->sname = (char *)alloc(strlen(name) + 1)))
		fatal("Out of memory - cannot allocate section descriptor");
	(void)strcpy(sp->sname,name);
	if (Icflag)			/* ignore case */
		(void)strdn (sp->sname);
	if (++Secno > MAXSEC)		/* too many sections */
		fatal("Too many sections in module");
	sp->secno = Secno;		/* fill in section number */
	sp->flags = Relmode ? REL : 0;	/* save relocation mode */
	sp->rflags = sp->lflags = PSPACE;/* init bit flags */
	if (!Lnkmode)			/* absolute mode */
		sp->cntrs = NULL;	/* don't use section counters */
	else {				/* relative -- allocate counters */
		if (!(sp->cntrs = (long (*)[MCNTRS][MBANKS-1])alloc((MSPACES*MCNTRS*(MBANKS-1))*sizeof(long))))
			fatal("Out of memory - cannot allocate section location counters");
		for (i = 0; i < MSPACES; i++)	/* init. location counters */
			for (j = 0; j < MCNTRS; j++)
				sp->cntrs[i][j][RBANK] = 0L;
		Pc = Lpc = &sp->cntrs[PMEM-1][DCNTR][RBANK];
		Old_pc = Old_lpc = 0L;		/* reinit. old counters */
		Ro_flag = Lo_flag = NO;		/* reset overflow flags */
		Cspace = Loadsp = PSPACE;	/* init. mem. space */
		Rcntr = Lcntr = DEFAULT;	/* init. ctr. type */
		Rmap = Lmap = NONE;		/* init. mem. map */
	}
	sp->xdefs = NULL;		/* no xrefs so far */
	sp->xrefs = NULL;		/* no xdefs so far */
	sp->next = NULL;		/* clear next pointer */
	Last_sec = Last_sec->next = sp; /* link into section chain */
	return(YES);
#endif /* PASM */
}

/**
*
* name		do_endsec - perform Endsec directive
*
* description	Performs the Endsec directive processing
*
**/
do_endsec()
{
#if !PASM
	struct secstk *ssp;

	(void)chk_flds (0);

	if( Csect == &Glbsec ){		/* current section is global */
		Optr = NULL;
		error("ENDSEC without associated SECTION directive");
		return(NO);
		}

	if( Sec_stack == NULL )		/* if there are no stacked sections */
		fatal("Section stack underflow");

	ssp = Sec_stack;		/* get section stack ptr */
	restore_sec(ssp->secpt);	/* restore last section */
	Sec_stack = ssp->ssnext;	/* unstack section */
	free((char *)ssp);
	return(YES);
#endif
}

/**
*
* name		save_sec - stack a section description
*
* description	Places the current Csect into the Sec_stack.
*
**/
static
save_sec()
{
#if !PASM
	struct secstk *sp;

	if (!(sp = (struct secstk *)alloc(sizeof(struct secstk))))
		fatal("Out of memory - cannot stack sections");
	sp->secpt = Csect;
	sp->ssnext = Sec_stack;
	Sec_stack = sp;
	if (!Lnkmode)			/* absolute mode */
		return;

	/* save runtime and load flags */
	Csect->rflags = Cspace | Rcntr | Rmap | (Ro_flag ? MOVFMASK : 0);
	Csect->lflags = Loadsp | Lcntr | Lmap | (Lo_flag ? MOVFMASK : 0);
#endif
}

/**
*
* name		restore_sec - restore section context
*
* description	Restores the section context pointed to by sec.
*
**/
static
restore_sec(sec)
struct slist *sec;
{
#if !PASM
	register flags;
	long *get_cntrs();

	Csect = sec;				/* set up new current sect */
	if (!Lnkmode)				/* absolute mode */
		return;
	Relmode = ((sec->flags & REL) != 0);	/* restore mode */

	flags = sec->rflags;			/* get run flags */
	Cspace = flags & MSPCMASK;		/* restore run space */
	Rcntr = flags & MCTRMASK;		/* restore run cntr type */
	Rmap = flags & MMAPMASK;		/* restore run memory type */
	Ro_flag = flags & MOVFMASK;		/* restore run ovrflo flag */
	Pc = get_cntrs (Cspace, Rcntr, RBANK);	/* restore run counter */
	Old_pc = *Pc;				/* restore prev. counter */

	flags = sec->lflags;			/* get load flags */
	Loadsp = flags & MSPCMASK;		/* restore load space */
	Lcntr = flags & MCTRMASK;		/* restore load cntr type */
	Lmap = flags & MMAPMASK;		/* restore load memory type */
	Lo_flag = flags & MOVFMASK;		/* restore load ovrflo flag */
	Lpc = get_cntrs (Loadsp, Lcntr, RBANK); /* restore load counter */
	Old_lpc = *Lpc;				/* restore prev. counter */
#endif
}

/**
*
* name		free_sec - free storage allocated to the section list
*
**/
free_sec ()
{
#if !PASM
	struct slist *sp, *pp;
	struct macd *np, *op;

	sp = Sect_lst->next;		/* skip GLOBAL section */
	while (sp) {
		if (sp->sname)
			free (sp->sname);
		if (sp->cntrs)
			free ((char *)sp->cntrs);
		np = sp->xrefs;
		while (np) {		/* free xrefs */
			op = np->mnext;
			free ((char *)np);
			np = op;
		}
		np = sp->xdefs;
		while (np) {		/* free xdefs */
			op = np->mnext;
			free ((char *)np);
			np = op;
		}
		pp = sp->next;
		free ((char *)sp);
		sp = pp;
	}
	Sect_lst->next = NULL;
	Last_sec = Sect_lst;
	Secno = 0;
#endif /* PASM */
}

/**
*
* name		do_page - input printed listing format
*
* synopsis	yn = do_page()
*		int yn;		YES / NO if error
*
* description	Input the printed listing format and check for consistency.
*
**/
static
do_page()
{
#if !PASM
	int p_width;	/* page width */
	int p_length;	/* page length */
	int blank_top;	/* blank lines at top of page */
	int blank_bot;	/* blank lines at bottom of page */
	int blank_left; /* blank columns at left of page */
	long eval_positive();

	Optr = Operand;
	if( *Optr == ',' )
		p_width = Max_col;
	else{
		if( (p_width = (int)eval_positive()) == ERR )
			return(NO);
		if( p_width < 1 || p_width > MAXBUF ){
			error("Invalid page width specified");
			return(NO);
			}
		}
	if( *Optr == ',' )
		++Optr;		/* skip over comma */
	if( *Optr != EOS ){
		if( *Optr == ',' )
			p_length = Lst_p_len;
		else{
			if( (p_length = (int)eval_positive()) == ERR )
				return(NO);
			if( p_length < 10 || p_length > MAXBUF ){
				error("Invalid page length specified");
				return(NO);
				}
			}
		if( *Optr == ',' )
			++Optr;		/* skip over comma */
		if( *Optr != EOS ){
			if( *Optr == ',' )
				blank_top = Lst_topm;
			else{
				if( (blank_top = (int)eval_positive()) == ERR )
					return(NO);
				}
			if( *Optr == ',' )
				++Optr;		/* skip over comma */
			if( *Optr != EOS ){
				if( *Optr == ',' ){
					blank_bot = p_length - Max_line;
					if( blank_bot < 0 ){
						error("Page length too small to allow default bottom margin");
						return(NO);
						}
					}
				else{
					if( (blank_bot = (int)eval_positive()) == ERR )
						return(NO);
					}
				if( *Optr == ',' )
					++Optr;		/* skip over comma */
				if( *Optr != EOS ){
					if( (blank_left = (int)eval_positive()) == ERR )
						return(NO);
					if( *Optr != EOS ){
						error("Extra characters in operand field");
						return(NO);
						}
					}
				else
					blank_left = Start_col - 1;
				}
			else{
				blank_bot = p_length - Max_line;
				blank_left = Start_col - 1;
				}
			}
		else{
			blank_top = Lst_topm;
			blank_bot = Lst_botm;
			blank_left = Start_col - 1;
			}
		}
	else{
		p_length = Lst_p_len;
		blank_top = Lst_topm;
		blank_bot = Lst_botm;
		blank_left = Start_col - 1;
		}
	if( blank_left >= p_width ){
		error("Left margin exceeds page width");
		return(NO);
		}
	if( blank_top + blank_bot > p_length - 10 ){
		error("Page length too small for specified top and bottom margins");
		return(NO);
		}
	Max_col = p_width;
	Lst_p_len = p_length;
	Lst_topm = blank_top;
	Lst_botm = blank_bot;
	Max_line = p_length - blank_bot;
	Start_col = blank_left + 1;
	return(YES);
#endif /* PASM */
}

/**
*
* name		do_lstcol - obtain listing column widths
*
* synopsis	yn = do_lstcol()
*		int yn;		YES / NO if error
*
* description	Input the listing column widths and check for consistency.
*
**/
static
do_lstcol()
{
	int labwidth = Labwidth;	/* Label field width */
	int opwidth = Opwidth;		/* Opcode field width */
	int operwidth = Operwidth;	/* Operand field width */
	int xwidth = Xwidth;		/* X field width */
	int ywidth = Ywidth;		/* Y field width */
	long eval_positive();

	Optr = Operand;
	if( *Optr != ',' ){
		if( (labwidth = (int)eval_positive()) == ERR )
			return(NO);
		if( labwidth < 1 || labwidth > MAXBUF ){
			error("Invalid label field width specified");
			return(NO);
			}
		}
	if( *Optr == ',' )
		++Optr;		/* skip over comma */
	if( *Optr != EOS ){
		if( *Optr != ',' ){
			if( (opwidth = (int)eval_positive()) == ERR )
				return(NO);
			if( opwidth < 1 || opwidth > MAXBUF - labwidth ){
				error("Invalid opcode field width specified");
				return(NO);
				}
			}
		if( *Optr == ',' )
			++Optr;		/* skip over comma */
		if( *Optr != EOS ){
			if( *Optr != ',' ){
				if( (operwidth = (int)eval_positive()) == ERR )
					return(NO);
				if( operwidth < 1 || operwidth > MAXBUF - (labwidth + opwidth) ){
					error("Invalid operand field width specified");
					return(NO);
					}
				}
			if( *Optr == ',' )
				++Optr;		/* skip over comma */
			if( *Optr != EOS ){
				if( *Optr != ',' ){
					if( (xwidth = (int)eval_positive()) == ERR )
						return(NO);
					if( xwidth < 1 || xwidth > MAXBUF - (labwidth + opwidth + operwidth) ){
						error("Invalid X field width specified");
						return(NO);
						}
					}
				if( *Optr == ',' )
					++Optr;		/* skip over comma */
				if( *Optr != EOS ){
					if( (ywidth = (int)eval_positive()) == ERR )
						return(NO);
					if( ywidth < 1 || ywidth > MAXBUF - (labwidth + opwidth + operwidth + xwidth) ){
						error("Invalid Y field width specified");
						return(NO);
						}
					if( *Optr != EOS ){
						error("Extra characters in operand field");
						return(NO);
						}
					}
				}
			}
		}
	Labwidth = labwidth;	/* reset global values */
	Opwidth = opwidth;
	Operwidth = operwidth;
	Xwidth = xwidth;
	Ywidth = ywidth;
	return(YES);
}

/**
*
* name		eval_positive - evaluate non-negative absolute expression
*
* synopsis	i = eval_positive()
*		int i;			evaluation result (ERR if error)
*
* description	Evaluates an expression and checks for a non-negative
*		result. Returns ERR if an error is encountered.
*
**/
static long int
eval_positive()
{
	struct evres *eval_int();
	struct evres *er;	/* evaluation result */
	long val;

	if( (er = eval_int()) == NULL )
		return(ERR);

	else if( (val = er->uval.xvalue.low) < 0L ){
		error("Expression cannot have a negative value");
		val = ERR;	/* error if negative */
		}
	free_exp(er);
	return(val);
}

/**
*
* name		skip_cond - skip conditional statements
*
* synopsis	yn = skip_cond(ctype)
*		int yn;		YES / NO if error
*		int else_flag;	YES = stop on ELSE or ENDIF
*				NO  = stop only on ENDIF
*
* description	Prints line in buffer if Prtc_flag is set.
*		Reads in input lines. Prints if the Unasm_flag is set.
*		Skips until a matching ENDIF  or ELSE directive is found.
*		Accounts for nested conditionals.
*
**/
static
skip_cond(else_flag)
int else_flag;
{
	char	*mapmne();
	char	*mne;
	struct pseuop *i, *psu_look();
	int	ifcnt = 1;

	if( Prtc_flag )
		print_line(' '); /* print conditionals if flag set */

	while( getaline() )
		if( parse_line() ){
			if( *Op != EOS && strlen(Op) < MAXOP ){
				mne = mapmne();
				if( (i = psu_look(mne)) != NULL ){
					if( i->ptype == ENDIF){
						if( --ifcnt == 0 ){
							dopsulab(NOLABEL); /* check for no label */
							(void)chk_flds(0); /* check for extra fields */
							Abortp = ( Prtc_flag == YES) ? NO : YES;
							return(YES);
							}
						}
					else if( i->ptype == ELSE ){
						if( ifcnt == 1 )
							if( else_flag ){
								dopsulab(NOLABEL);
								(void)chk_flds(0);
								return(do_cond(NO));
								}
							else {
								Optr = NULL;
								error("ELSE without associated IF");
								}
						}
					else if ( i->ptype == IF )
						++ifcnt;
					}
				}
			if( Unasm_flag )
				print_line(IFSKIP);
			}
		else{
			if( Unasm_flag )
				print_comm(IFSKIP);
			}

	/* if we got here then the ENDIF must be missing */
	error("Unexpected end of file - missing ENDIF");
	return(NO);
}

/**
*
* name		do_cond - do conditional lines
*
* synopsis	yn = do_cond(else_flag)
*		int yn;			YES / NO if error
*		int else_flag;		YES = stop on ELSE or ENDIF
*					NO  = stop only on ENDIF
*
* description	Reads in input lines and executes them until an
*		ELSE or ENDIF directive is encountered. (See description
*		of else_flag above).
*
**/
static
do_cond(else_flag)
int else_flag;
{
	int i;

	if( Prtc_flag )
		print_line(' '); /* print conditionals if flag set */

	while( getaline() ){
		reset_pflags(); /* reset force print flags */
		if(parse_line()){
			if( (i = process(YES,else_flag)) == ENDIF ){
				dopsulab(NOLABEL); /* check for no label */
				(void)chk_flds(0); /* check for extra fields */
				Abortp = (Prtc_flag) ? NO : YES;
				return(YES);
				}
			if( i == ELSE ){
				dopsulab(NOLABEL); /* check for no label */
				(void)chk_flds(0); /* check for extra fields */
				return(skip_cond(NO));
				}
			print_line(' ');
			}
		else
			print_comm(' ');
		P_total = 0;	/* reset byte count */
		Cycles = 0;	/* and per instruction cycle count */
		}
	error("Unexpected end of file - missing ENDIF");
	return(NO);
}

/**
*
* name		do_if - perform IF directive
*
* synopsis	yn = do_if()
*		int yn;			YES / NO if error
*
* description	Handles all processing for if directive.
*
**/
static
do_if()
{
	struct evres *er;	/* expression result */
	struct evres *eval();
	int	fflag = YES;	/* default condition true */

	(void)chk_flds(1);
	Optr = Operand;
	if( (er = eval()) == NULL )	/* get operand */
		return(NO);

	if( *Optr != EOS ){
		error("Extra characters beyond expression");
		return(NO);
		}

	if( er->fwd_ref ){
		free_exp(er);
		error("Expression contains forward references");
		return(NO);
		}

	if( er->etype != INT ){
		free_exp(er);
		error("Expression result must be integer");
		return(NO);
		}

	if( er->uval.xvalue.low == 0L )
		fflag = NO;
	free_exp(er); /* release expression result */

	if( fflag )
		return(do_cond(YES));
	return(skip_cond(YES));
}

/**
*
* name		do_option - do OPT
*
* synopsis	yn = do_option(ptr)
*		int yn;		NO if errors
*		char *ptr;	start of option string
*
* description	Handles all options. Errors will result if option is
*		missing or invalid.
*
**/
do_option(ptr)
char *ptr;
{
#if !PASM
	char lcopt[MAXOPT+1]; /* buffer to hold lower option mnemonic */
	register char *pto;
	register int i;
	struct opt *p;
	struct opt *opt_look();
	int Loc = NO;

	(void)chk_flds(1);
	if( *ptr == EOS ){
		error("Missing option");
		return(NO);
		}

	while( *ptr != EOS ){
		i = 0;
		pto = lcopt;	/* shift option to lower case */
		while( *ptr != EOS && *ptr != ',' ){
			if( i++ >= MAXOPT ){
				error("Illegal option");
				return(NO);
				}
			*pto++ = mapdn(*ptr++);
			}
		*pto = EOS;
		if( *lcopt == EOS ){
			error("Missing option");
			return(NO);
			}
		if( (p = opt_look(lcopt)) == NULL ){
			error("Illegal option");
			return(NO);
			}
		switch(p->otype){
			case CC:
				Cyc_flag = YES;
				Ctotal = 0;
				break;
			case CEX:
				Cex_flag = YES;
				break;
			case CL:
				Prtc_flag = YES;
				break;
			case CM:
				Cm_flag = YES;
				break;
			case CONTC:
				Cyc_flag = YES;
				break;
			case CRE:
				if( Pass == 1 )
					Cre_err = Sym_cnt != 0;
				if( Cre_err ){
					error("CRE option must be used before any label");
					break;
					}
				Cre_flag = YES;
				S_flag = YES; /* set symbol table flag */
				break;
			case DEX:
				Dex_flag = YES;
				break;
			case FC:
				Fcflag = YES;
				break;
			case IC:
				if( Pass == 1)
					Ic_err = Sym_cnt != 0  ||
						 No_defs != 0  ||
						 Macro != NULL ||
						 Sect_lst->next != NULL;
				if( Ic_err ){
					error("IC option must be used before any symbol, section, or macro definition");
					break;
					}
				Icflag = YES;
				break;
			case IL:
				Ilflag = YES;
				break;
			case INTR:
				Int_flag = YES;
				break;
			case LB:
				Lbflag = YES;
				break;
			case LOC:
				Loc = YES;
				break;
			case LW:
				Lwflag = YES;
			case MC:
				Mc_flag = YES;
				break;
			case MD:
				Pmacd_flag = YES;
				break;
			case MEX:
				Mex_flag = YES;
				break;
			case MINC:
				Mi_flag = YES;
				break;
			case MSW:
				Msw_flag = YES;
				break;
			case MU:
				if (Pass == 1)
					break;
				if (Emit) {
					error ("MU option must be used before any code or data generation");
					break;
				}
				Muflag = YES;
				break;
			case NOCC:
				Cyc_flag = NO;
				break;
			case NOCEX:
				Cex_flag = NO;
				break;
			case NOCL:
				Prtc_flag = NO;
				break;
			case NOCM:
				Cm_flag = NO;
				break;
			case NODEX:
				Dex_flag = NO;
				break;
			case NOFC:
				Fcflag = NO;
				break;
			case NOINTR:
				Int_flag = NO;
				break;
			case NOLB:
				Lbflag = NO;
				break;
			case NOLW:
				Lwflag = NO;
				break;
			case NOMC:
				Mc_flag = NO;
				break;
			case NOMD:
				Pmacd_flag = NO;
				break;
			case NOMEX:
				Mex_flag = NO;
				break;
			case NOMINC:
				Mi_flag = NO;
				break;
			case NOMSW:
				Msw_flag = NO;
				break;
			case NOPS:
				Pack_flag = NO;
				break;
			case NORC:
				Rcom_flag = NO;
				break;
			case NORP:
				Rpflag = NO;
				break;
			case NOUNAS:
				Unasm_flag = NO;
				break;
			case NOWRN:
				Wrn_flag = NO;
				break;
			case NOXR:
				Xrf_flag = NO;
				break;
			case PS:
				Pack_flag = YES;
				break;
			case RC:
				Rcom_flag = YES;
				break;
			case RP:
				Rpflag = YES;
				break;
			case SO:
				So_flag = YES;
				break;
			case SYMT:
				S_flag = YES;
				break;
			case UNAS:
				Unasm_flag = YES;
				break;
			case WRN:
				Wrn_flag = YES;
				break;
			case XR:
				Xrf_flag = YES;
				break;
			case NONE:
				break;
			default:
				fatal("Option select error");
			}
		if( *ptr != EOS ) /* skip comma */
			ptr++;
		}
	if (Loc && (Cre_flag || S_flag))
		Loc_flag = YES;

	return(YES);
#endif
}

/**
*
* name		do_rdirect - perform Rdirect directive
*
* description	Performs Rdirect directive processing.
*
**/
static
do_rdirect()
{
#if !PASM
	char *name;		/* pointer to name to be removed */
	struct mnemop *mt;	/* pointer to mnemonic entry */
	struct pseuop *pt;	/* pointer to directive entry */
	char *get_sym ();
	struct mnemop *mne_look ();
	struct pseuop *psu_look ();

	if (Csect != &Glbsec) { /* section active */
		error ("RDIRECT directive not allowed in section");
		return (NO);
	}
	(void)chk_flds(1);
	Optr = Operand;
	if( !*Optr ){
		error("Missing directive name");
		return(NO);
		}
	while( *Optr ){
		if( !(name = get_sym ()) ) /* get mnemonic name */
			return(NO);
		if( (mt = mne_look (name)) != NULL )
			mt->class |= (char)0x80;	/* turn on high bit */
		else if( (pt = psu_look (name)) != NULL )
			pt->ptype |= (char)0x80;	/* turn on high bit */
		else
			error2("Assembler directive or mnemonic not found",name);
		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in directive name list");
				return(NO);
				}
		}
	return(YES);
#endif
}

/**
*
* name		clr_rdirect - clear RDIRECT indicator in tables
*
* description	Clear RDIRECT directive indicator in opcode and pseudo-op
*		tables.
*
**/
clr_rdirect ()
{
#if !PASM
	struct mnemop *mp;
	struct pseuop *op;

	for (mp = Mnemonic; mp < Mnemonic + Nmne; mp++)
		mp->class &= (char)0x7f;	/* clear high bit */
	for (op = Pseudo; op < Pseudo + Npse; op++)
		op->ptype &= (char)0x7f;	/* clear high bit */
#endif
}

/**
*
* name		do_symobj - perform Symobj directive
*
* description	Performs Symobj directive processing.
*
**/
static
do_symobj()
{
#if !PASM
	char *sym;		/* pointer to symbol name */
	struct nlist *sp;	/* pointer to symbol table entry */
	struct symlist *so;	/* pointer to symbol list entry */
	char symbuf[MAXSYM+1];
	char *get_sym ();

	if (Pass == 1)	/* do nothing on first pass */
		return (YES);

	if (So_flag)	/* no effect with SO option */
		return (YES);

	(void)chk_flds(1);
	Optr = Operand;
	if( !*Optr ){
		error("Missing symbol name");
		return(NO);
		}
	while( *Optr ){
		if( !(sym = get_sym ()) ) /* get symbol name */
			return(NO);
		if( Icflag ){		/* ignore case */
			(void)strcpy (symbuf, sym);
			(void)strdn (symbuf);
			sym = symbuf;
			}
		for (sp = Hashtab[hash (sym)]; sp; sp = sp->next)
			if (STREQ (sym, sp->name) )
				break;
		if (sp) {
			if (! (so = (struct symlist *)alloc (sizeof (struct symlist))) )
				fatal ("Out of memory - cannot allocate symbol list");
			so->sym = sp;	/* save pointer to symbol table entry */
			so->next = Sym_list;	/* link into chain */
			Sym_list = so;
			So_cnt++;	/* increment count */
			}
		else
			error2 ("Symbol undefined on pass 2", sym);
		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in symbol name list");
				return(NO);
				}
		}
	return(YES);
#endif
}

/**
*
* name		do_comment - Comment directive
*
* description	Performs Comment directive processing
*
**/
static
do_comment()
{
#if !PASM
	char delim;

	if( *Operand == EOS )
		return(YES); /* no comment delimiter */
	Optr = Operand;
	delim = *Optr++; /* get the delim character */

	print_comm(' '); /* print the comment directive line */
	/* search for directive in the rest of the comment directive line */
	if( strchr(Optr,delim) || strchr(Xmove,delim) || strchr(Ymove,delim) || strchr(Com,delim) ){
		Abortp = YES; /* stop print of comment directive line */
		return(YES);
		}

	while( getaline() ){ /* search following lines for delim */
		print_comm(' '); /* print the line */
		if( strchr(Line,delim) ){
			Abortp = YES; /* stop print on the line */
			return(YES);
			}
		}
	error("Unexpected end of file - missing COMMENT delimiter");
	return(NO);
#endif
}

/**
*
* name		do_end - end directive
*
* description	Performs end directive processing.
*
**/
static
do_end()
{
#if !PASM
	struct evres *ea, *eval_ad();

	(void)chk_flds(1);
	End_flag = YES; /* set flag */
	if( *Operand == EOS )
		return(YES); /* no starting address specified */

	Optr = Operand; /* evaluate ending address */
	if( (ea = eval_ad()) == NULL )
		return(NO);
	if (ea->mspace && ea->mspace != PSPACE) {
		error ("Memory space must be P or NONE");
		return (NO);
	}
	End_addr = ea->uval.xvalue.low;
	End_rel = (ea->rel || ea->secno < 0);	/* relative data */
	free_exp (ea);
	Pstart = NO;
	return(YES);
#endif
}

/**
*
* name		do_global - perform Global directive
*
* description	Performs Global directive processing.
*
**/
static
do_global()
{
#if !PASM
	char *sym;		/* pointer to symbol name */
	struct macd *sp;	/* xdef list */
	char *get_sym ();

	if( Csect == &Glbsec ){
		error("GLOBAL without preceding SECTION directive");
		return(NO);
		}
	if( !*Optr ){
		error("Missing symbol name");
		return(NO);
		}
	(void)chk_flds(1);
	Optr = Operand;
	while( *Optr ){
		if( !(sym = get_sym ()) ) /* get symbol name */
			return(NO);
		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in symbol name list");
				return(NO);
				}
		if( *sym == '_' )
			error2("Local symbol names cannot be used with GLOBAL",sym);
		else if( xref_look(sym) )
			error2("Symbol already defined as XREF",sym);
		else if( xdef_look(sym) )
			error2("Symbol already defined as XDEF",sym);
		else if( glblookup(sym) ){ /* check for any other errors */
			if (!(sp = (struct macd *)alloc(sizeof(struct macd))))
				fatal("Out of memory - cannot save GLOBAL symbols");
			if (!(sp->mline = (char *)alloc( strlen(sym) + 1)))
				fatal("Out of memory - cannot save GLOBAL symbols");
			(void)strcpy(sp->mline,sym); /* save symbol name */
			sp->mnext = Csect->xdefs; /* insert into list */
			Csect->xdefs = sp;
			}
		}
	return(YES);
#endif
}

/**
*
* name		do_xref - perform Xref directive
*
* description	Performs the Xref directive processing.
*
**/
static
do_xref()
{
#if !PASM
	char *sn;	/* symbol name */
	struct macd *sp;	/* xref list */
	char *get_sym ();

	if( Csect == &Glbsec ){
		error("XREF without preceding SECTION directive");
		return(NO);
		}
	if( *Operand == EOS ){
		error("Missing symbol name");
		return(NO);
		}
	(void)chk_flds(1);
	Optr = Operand; /* point to symbol names */
	while( *Optr ){
		if( (sn = get_sym()) == NULL )
			return(NO);
		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in symbol name list");
				return(NO);
				}
		if( *sn == '_' )
			error2("Local symbol names cannot be used with XREF",sn);
		else if( xref_look(sn) )
			error2("Symbol already defined as XREF",sn);
		else if( xdef_look(sn) )
			error2("Symbol already defined as XDEF",sn);
		else if( xrlookup(sn) ){ /* check for any other errors */
			if (!(sp = (struct macd *)alloc(sizeof(struct macd))))
				fatal("Out of memory - cannot save XREF symbols");
			if (!(sp->mline = (char *)alloc( strlen(sn) + 1)))
				fatal("Out of memory - cannot save XREF symbols");
			(void)strcpy(sp->mline,sn); /* save symbol name */
			sp->mnext = Csect->xrefs; /* insert into list */
			Csect->xrefs = sp;
			}
		}
	return(YES);
#endif
}

/**
*
* name		do_xdef - perform Xdef directive
*
* description	Performs the Xdef directive processing.
*
**/
static
do_xdef()
{
#if !PASM
	char *sn;	/* symbol name */
	struct macd *sp;	/* xdef list */
	char *get_sym ();

	if( Csect == &Glbsec ){
		error("XDEF without preceding SECTION directive");
		return(NO);
		}

	if( *Operand == EOS ){
		error("Missing symbol name");
		return(NO);
		}
	(void)chk_flds(1);
	Optr = Operand; /* point to symbol names */
	while( *Optr ){
		if( (sn = get_sym()) == NULL )
			return(NO);

		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in symbol name list");
				return(NO);
				}
		if( *sn == '_' )
			error2("Local symbol names cannot be used with XDEF",sn);
		else if( xref_look(sn) )
			error2("Symbol already defined as XREF",sn);
		else if( xdef_look(sn) )
			error2("Symbol already defined as XDEF",sn);
		else if( xdlookup(sn) ){ /* check for any other errors */
			if (!(sp = (struct macd *)alloc(sizeof(struct macd))))
				fatal("Out of memory - cannot save XDEF symbols");
			if (!(sp->mline = (char *)alloc( strlen(sn) + 1)))
				fatal("Out of memory - cannot save XDEF symbols");
			(void)strcpy(sp->mline,sn); /* save symbol name */
			sp->mnext = Csect->xdefs; /* insert into list */
			Csect->xdefs = sp;
			}
		}
	return(YES);
#endif
}

/**
*
* name		xref_look - look up symbol in the xref table
*
* synopsis	xref_look(sn)
*		char *sn;	null-terminated symbol name
*
* description	The current section xref table is searched for sn.
*		Returns YES if found, NO if not.
*
**/
xref_look(sn)
char *sn;
{
#if !PASM
	struct macd *xp;

	if( Csect == &Glbsec )
		return(NO); /* no active section */

	for( xp = Csect->xrefs ; xp != NULL ; xp = xp->mnext ){
		if( STREQ(xp->mline,sn) )
			return(YES);
		}
	return(NO);
#endif
}

/**
*
* name		xdef_look - look up symbol in the xdef table
*
* synopsis	xdef_look(sn)
*		char *sn;	null-terminated symbol name
*
* description	The current section xdef table is searched for sn.
*		Returns YES if found, NO if not.
*
**/
xdef_look(sn)
char *sn;
{
#if !PASM
	struct macd *xp;

	if( Csect == &Glbsec )
		return(NO); /* no active section */

	for( xp = Csect->xdefs ; xp != NULL ; xp = xp->mnext ){
		if( STREQ(xp->mline,sn) )
			return(YES);
		}
	return(NO);
#endif
}

/**
*
* name		glblookup --- special GLOBAL symbol table lookup
*
* synopsis	yn = glblookup(name)
*		int yn;			YES if no error / NO if error
*		char *name;		null terminated symbol name
*
* description	Symbol table is searched for name. If not found
*		then on Pass 2, error is issued and NO is returned.
*
*		If found and symbol is defined global, YES is returned.
*		In this case, on Pass 2, an entry is made in cross ref
*		table if option enabled.
*
**/
static
glblookup(name)
char	*name;
{
#if !PASM
	struct nlist *np;
	char buf[MAXSYM+1];

	if( Icflag ){		/* ignore case */
		(void)strcpy (buf, name);
		(void)strdn (buf);
		name = buf;
		}
	for( np = Hashtab[hash(name)] ; np != NULL ; np = np->next)
		if( STREQ(name,np->name) ){
			if( np->sec == Csect ){ /* symbol defined in current section */
				if( Lnkmode && (np->attrib & SET ) ){
					error2("Set symbol names cannot be used with GLOBAL",name);
					return(NO);
					}
				np->attrib |= GLOBAL;	/* set global flag */
				if( Pass == 2 && Cre_flag && !Chkdo)
					xref_sym (np, REFTYPE);
				return(YES);
				}
			if( np->attrib & GLOBAL ){ /* already defined as global */
				error2("Symbol already defined as global",name);
				return(NO);
				}
			}

	if( !Lnkmode && Pass == 2 )
		error2("Symbol undefined on pass 2",name);
	return(NO);
#endif
}

/**
*
* name		xrlookup --- special XREF symbol table lookup
*
* synopsis	yn = xrlookup(name)
*		int yn;			YES if no error / NO if error
*		char *name;		null terminated symbol name
*
* description	Symbol table is searched for name. If not found
*		then on Pass 2, error is issued. On either pass,
*		YES is returned so that name will be entered into XREF list.
*
*		If found and symbol is defined global, YES is returned.
*		In this case, on Pass 2, an entry is made in cross ref
*		table if option enabled.
*
*		if found and symbol is private section symbol of the current
*		section, error is issued and NO returned.
*
**/
static
xrlookup(name)
char	*name;
{
#if !PASM
	struct nlist *np;
	char buf[MAXSYM+1];

	if( Icflag ){		/* ignore case */
		(void)strcpy (buf, name);
		(void)strdn (buf);
		name = buf;
		}
	for( np = Hashtab[hash(name)] ; np != NULL ; np = np->next)
		if( STREQ(name,np->name) ){
			if( np->attrib & GLOBAL ){ /* global name */
				if( Pass == 2 && Cre_flag && !Chkdo)
					xref_sym (np, REFTYPE);
				return(YES);
				}
			if( np->sec == Csect ){
				error2("Symbol already defined in current section",name);
				return(NO);
				}
			}

	if( !Lnkmode && Pass == 2 )
		error2("Symbol undefined on pass 2",name);
	return(YES);
#endif
}

/**
*
* name		xdlookup --- special XDEF symbol table lookup
*
* synopsis	yn = xdlookup(name)
*		int yn;			YES if no error / NO if error
*		char *name;		null terminated symbol name
*
* description	Symbol table is searched for name. If not found
*		then, on Pass 2 error is issued. On either pass,
*		YES is returned so that name will be entered into XDEF list.
*
*		If found and symbol is defined within current section as
*		global, YES is returned. In this case, on Pass 2, an entry
*		is made in cross ref table if option enabled.
*
*		if found and symbol is private section symbol of the current
*		section, error is issued and NO returned.
*
*		if found and symbol is defined as global but not within
*		the current section, error is issued and NO is returned.
*
**/
static
xdlookup(name)
char	*name;
{
#if !PASM
	struct nlist *np;
	char buf[MAXSYM+1];

	if( Icflag ){		/* ignore case */
		(void)strcpy (buf, name);
		(void)strdn (buf);
		name = buf;
		}
	for( np = Hashtab[hash(name)] ; np != NULL ; np = np->next)
		if( STREQ(name,np->name) ){
			if( np->sec == Csect ){ /* symbol defined in current section */
				if( !(np->attrib & GLOBAL) ){
					error2("Symbol defined in current section before XDEF directive",name);
					return(NO);
					}
				if( Lnkmode && (np->attrib & SET ) ){
					error2("Set symbol names cannot be used with XDEF",name);
					return(NO);
					}
				if( Pass == 2 && Cre_flag && !Chkdo)
					xref_sym (np, REFTYPE);
				return(YES);
				}
			if( np->attrib & GLOBAL ){ /* already defined as global */
				error2("Symbol already defined as global",name);
				return(NO);
				}
			}

	if( Pass == 2 )
		error2("Symbol undefined on pass 2",name);
	return(YES);
#endif
}

/**
* name		free_xrd - release xref and xdef lists
*
* description	Releases any storage allocated to section xref and
*		xdef lists.
*
**/
free_xrd()
{
#if !PASM
	struct macd *rp,*nr;
	struct slist *sp;

	for( sp = Sect_lst->next ; sp ; sp = sp->next ){
		/* erase all xrefs for current section */
		rp = sp->xrefs;
		while(rp){
			nr = rp->mnext;
			free(rp->mline);
			free((char *)rp);
			rp = nr;
			}
		sp->xrefs = NULL;
		/* erase all xdefs for current section */
		rp = sp->xdefs;
		while(rp){
			nr = rp->mnext;
			free(rp->mline);
			free((char *)rp);
			rp = nr;
			}
		sp->xdefs = NULL;
		}
#endif
}

/**
*
* name		do_dc - perform Dc directive
*
* description	Handles Dc directive processing
*
**/
static
do_dc ()
{
#if !PASM
	register i, count = 0;
	struct evres *ev = NULL;
	struct evres *eval ();
	struct evres *eval_string ();
	struct evres *do_frac ();
	long *word, val[EWLEN];

	(void)chk_flds (1);

	if (*Optr == ',' || *Optr == EOS) {
		error ("Missing expression");
		return (NO);
	}

	while (*Optr) { /* scan DC expressions */
		i = 0;
		val[EWORD] = val[HWORD] = val[LWORD] = 0L;
		if (*Optr == ',')	/* no value; emit zero word */
			emit_data (val, (char *)NULL);
		else if (*Optr == STR_DELIM || *Optr == XTR_DELIM) {
			if (!(ev = eval_string ()))
				return (NO);
			for (i = 0, word = ev->uval.wvalue;
			     i < ev->wcount; i++) {
				val[LWORD] = *word++;
				emit_data (val, (char *)NULL);
			}
			free ((char *)ev->uval.wvalue);
		} else {
			if ((ev = eval ()) == NULL)
				return (NO);	/* bad expression */
			if (ev->etype == FPT)	/* floating point */
				dtof (ev->uval.fvalue,
				      Cspace == LSPACE ? SIZEL : SIZES, ev);
			val[EWORD] = ev->uval.xvalue.ext;
			val[HWORD] = ev->uval.xvalue.high;
			val[LWORD] = ev->uval.xvalue.low;
			if (ev->rel || ev->secno < 0)	/* relative data */
				emit_data (val, Expr);
			else
				emit_data (val, (char *)NULL);
		}
		if (ev) {
			free_exp (ev);	/* free expression */
			ev = NULL;
		}
		if (*Optr)
			if (*Optr == ',') {
				Optr++;		/* skip comma */
				if (!*Optr){	/* trailing comma */
					val[EWORD] = val[HWORD] =
						val[LWORD] = 0L;
					emit_data (val, (char *)NULL);
					count++;
					}
			} else {
				error ("Missing comma");
				return (NO);
			}
		count += i ? i : 1;	/* increment word count */
	}
	Emit = YES;		/* set emit flag */
	if (Muflag)		/* memory utilization report active */
		do_mu (MUCONST, (unsigned)count);
	P_force = YES;
	if (Cex_flag) { /* DC expansion option */
		print_line (DCEXP);
		Abortp = YES;
	} else
		P_total = 0;
	return (YES);
#endif /* PASM */
}

/**
*
* name		do_bsc - perform Bsc directive
*
* description	Handles Bsc directive processing
*
**/
static
do_bsc ()
{
#if !PASM
	register char *p;
	struct evres *er, *eval_int (), *eval ();
	long count, val[EWLEN];

	(void)chk_flds(1);
	p = strchr (Optr, ',');
	if (p)		/* isolate count */
		*p = EOS;
	er = eval_int();
	if (p)
		*p = ',';	/* put comma back */
	if (!er)
		return(NO);
	if( er->fwd_ref ) {
		error("Expression contains forward references");
		free_exp (er);
		return(NO);
		}
	count = er->uval.xvalue.low; /* save count */
	free_exp (er);
	if (count <= 0L) {
		error ("Expression must be greater than zero");
		return (NO);
	}

	val[EWORD] = val[HWORD] = val[LWORD] = 0L;
	if (p) {	/* directive contains value */
		Optr = ++p;	/* point to start of value */
		if( !(er = eval()) )
			return (NO);	/* bad expression */
		if (er->etype == FPT)	/* floating point */
			dtof (er->uval.fvalue,
			      Cspace == LSPACE ? SIZEL : SIZES, er);
		val[EWORD] = er->uval.xvalue.ext;
		val[HWORD] = er->uval.xvalue.high;
		val[LWORD] = er->uval.xvalue.low;
		free_exp (er);			/* free expression */
	}

	if (er->rel || er->secno < 0)		/* relative value */
		emit_bdata (count, val, Expr, 0);
	else
		emit_bdata (count, val, (char *)NULL, 0); /* block data to obj file */
	Emit = YES;		/* set emit flag */
	if (Muflag)		/* memory utilization report active */
		do_mu (MUCONST, (unsigned)count);
	return (YES);
#endif
}

/**
*
* name		do_bsmr - perform Bsm/Bsr directive
*
* description	Handles Bsm/Bsr directive processing
*
**/
static
do_bsmr (type)
int type;
{
#if !PASM
	register char *p;
	struct evres *er, ev, *eval_int (), *eval ();
	int label;
	long count, val[EWLEN];

	(void)chk_flds(1);
	if( Relmode ){	/* relative mode */
		error("Directive not allowed in relative mode");
		return(NO);
		}

	p = strchr (Optr, ',');
	if (p)		/* isolate count */
		*p = EOS;
	er = eval_int();
	if (p)
		*p = ',';	/* put comma back */
	if (!er)
		return(NO);
	if( er->fwd_ref ) {
		error("Expression contains forward references");
		free_exp (er);
		return(NO);
		}
	count = er->uval.xvalue.low; /* save count */
	free_exp (er);
	if (count <= 0L) {
		error ("Expression must be greater than zero");
		return (NO);
	}

	if (type == MUMOD || type == MUREV)	/* check if special buffer */
		if (!set_base (type, count)) {
			free_exp (er);
			return (NO);
		}

	val[EWORD] = val[HWORD] = val[LWORD] = 0L;
	if (p) {	/* directive contains value */
		Optr = ++p;	/* point to start of value */
		if( !(er = eval()) )
			return (NO);	/* bad expression */
		if (er->etype == FPT)	/* floating point */
			dtof (er->uval.fvalue,
			      Cspace == LSPACE ? SIZEL : SIZES, er);
		val[EWORD] = er->uval.xvalue.ext;
		val[HWORD] = er->uval.xvalue.high;
		val[LWORD] = er->uval.xvalue.low;
		free_exp (er);			/* free expression */
	}

	/* install label with new pc value */
	label = get_i_sym (Label);
	if (label == ERR)
		return (NO);
	if (label) {
		ev.etype = INT;
		ev.uval.xvalue.ext = ev.uval.xvalue.high = 0L;
		ev.uval.xvalue.low = *Pc;
		ev.mspace = Cspace;
		(void)install(Label,&ev,NONE,Rcntr|Rmap);
	}

	emit_bdata (count, val, (char *)NULL, 0); /* block data to obj file */
	Emit = YES;		/* set emit flag */
	if (Muflag)		/* memory utilization report active */
		do_mu ((unsigned)type, (unsigned)count);
	P_force = YES;		/* force pc output */
	return (YES);
#endif
}

/**
*
* name		do_ds - perform Ds directive
*
* description	Handles Ds directive processing
*
**/
static
do_ds()
{
#if !PASM
	struct evres *er, *eval_int ();
	long count, val[EWLEN];

	(void)chk_flds(1);
	if( Pc != Lpc ){ /* if overlay is in progress */
		error("Overlay generation in progress");
		return(NO);
		}

	if( (er = eval_int()) == NULL )
		return(NO);
	if( er->fwd_ref ) {
		error("Expression contains forward references");
		free_exp(er);
		return(NO);
		}
	if ((count = er->uval.xvalue.low) <= 0L) {
		error ("Expression must be greater than zero");
		free_exp (er);
		return (NO);
	}
	free_exp(er);		/* release structure */

	Emit = YES;		/* set emit flag */
	if (Muflag)		/* memory utilization report active */
		do_mu (MUDATA, (unsigned)count);
	P_force = YES;		/* force pc output */
	val[EWORD] = val[HWORD] = val[LWORD] = 0L;
	if (Lnkmode)		/* relative mode */
		emit_bdata (count, val, (char *)NULL, 1);
	else {
		*Pc += count;		/* increment pc */
		(void)chklc ();		/* check for overflow */
	}
	return(YES);
#endif
}

/**
*
* name		do_dsmr - perform Dsm/Dsr directive
*
* description	Handles Dsm/Dsr directive processing
*
**/
static
do_dsmr(type)
int type;
{
#if !PASM
	struct evres *er, ev, *eval_int ();
	int label;

	(void)chk_flds(1);
	if( Pc != Lpc ){ /* if overlay is in progress */
		error("Overlay generation in progress");
		return(NO);
		}

	if( Relmode ){	/* relative mode */
		error("Directive not allowed in relative mode");
		return(NO);
		}

	if( (er = eval_int()) == NULL )
		return(NO);
	if( er->fwd_ref ) {
		error("Expression contains forward references");
		free_exp(er);
		return(NO);
		}

	if (!set_base (type, er->uval.xvalue.low)) {
		free_exp (er);
		return (NO);	/* couldn't set new base address */
		}

	/* install label with new pc value */
	label = get_i_sym (Label);
	if (label == ERR)
		return (NO);
	if (label) {
		ev.etype = INT;
		ev.uval.xvalue.ext = ev.uval.xvalue.high = 0L;
		ev.uval.xvalue.low = *Pc;
		ev.mspace = Cspace;
		(void)install(Label,&ev,NONE,Rcntr|Rmap);
	}

	Emit = YES;		/* set emit flag */
	if (Muflag)		/* memory utilization report active */
		do_mu ((unsigned)type, (unsigned)er->uval.xvalue.low);
	P_force = YES;		/* force pc output */
	*Pc += er->uval.xvalue.low; /* increment pc */
	free_exp(er);		/* release structure */
	return(YES);
#endif /* PASM */
}

/**
*
* name		do_baddr - perform Baddr directive
*
* description	Handles Baddr directive processing
*
**/
static
do_baddr()
{
#if !PASM
	struct evres *er, *eval_int ();
	int type;

	(void)chk_flds(1);
	if( Pc != Lpc ){ /* if overlay is in progress */
		error("Overlay generation in progress");
		return(NO);
		}

	if( Relmode ){	/* relative mode */
		error("Directive not allowed in relative mode");
		return(NO);
		}

	type = mapdn (*Optr++);		/* get buffer type */
	if (type == 'm')
		type = MUMOD;
	else if (type == 'r')
		type = MUREV;
	else {
		error ("Invalid buffer type");
		return (NO);
	}

	if (*Optr++ != ',') {
		error ("Syntax error in operand field");
		return (NO);
	}

	if( (er = eval_int()) == NULL )
		return(NO);
	if( er->fwd_ref ){
		error("Expression contains forward references");
		free_exp(er);
		return(NO);
		}

	if (!set_base (type, er->uval.xvalue.low)) {
		free_exp (er);
		return (NO);	/* couldn't set new base address */
		}

	P_force = YES;		/* force pc output */
	*Pc += er->uval.xvalue.low; /* increment pc */
	free_exp(er);		/* release structure */
	return(YES);
#endif /* PASM */
}

/**
*
* name		set_base - set base address for modulo/rev-carry buffers
*
* synopsis	yn = set_base (type, size)
*		int yn;		YES/NO depending on errors
*		int type;	Type of buffer to allocate
*		long size;	Size of buffer
*
* description	Sets the program counter to the base of the specified
*		buffer given its type and size.
*
**/
static
set_base(type,size)
int type;
long size;
{
#if !PASM
	int shift;
	long base, pc;

	if (size <= 0L) {
		error ("Storage block size must be greater than zero");
		return (NO);
	}
	if (type == MUMOD) {
		if (size < 2L || size > 32768L) {
			error ("Storage block size out of range");
			return (NO);
		}
	} else if (type == MUREV) {
		for (base = size; base && !(base & (long)1); base >>= 1)
			;		/* check if power of two */
		if (base > 1L)
			warn ("Storage block size not a power of 2");
	}
	/* establish new base address */
	for (base = 1L, shift = 0; base && base < size; base <<= 1)
		shift++;		/* determine next power of 2 */
	if (!base) {
		error ("Storage block too large");
		return (NO);
	}
	base = ~(~((long)0) << shift);		/* create base address mask */
	for (pc = *Pc; pc & base; pc++) /* bump pc to next valid base addr. */
		;
	if ((pc &= (long)MAXADDR) >= *Pc) {
		*Pc = pc;		/* got a good base address */
		Old_pc = *Pc;		/* set old pc for listing */
	} else {
		error ("Storage block too large");
		return (NO);
	}
	return (YES);
#endif /* PASM */
}

/**
*
* name		do_prctl - perform Prctl directive
*
* description	Handles Prctl directive processing
*
**/
static
do_prctl ()
{
	register char *p, *s = String;
	struct evres *ev = NULL;
	struct evres *eval_byte ();
	int c;
	char *get_string ();
#if VMS
	long ftell ();
#endif

	if (Pass == 1 || Ilflag)	/* ignore if pass 1 or list inhibit */
		return (YES);

	if (Lstfil == stdout) {		/* ignore if no -L option */
		warn ("PRCTL directive ignored - no explicit listing file");
		return (NO);
	}

	(void)chk_flds (1);
	while (Optr) {	/* scan expressions */
		p = strchr (Optr, ',');
		if (Optr == p) {	/* no value -- zero byte */
			*s++ = '\0';
			if (!*(p + 1)) {/* trailing comma with no value */
				*s++ = '\0';
				p = NULL;
			}
		} else if (*Optr == STR_DELIM || *Optr == XTR_DELIM) {
			if (!(p = get_string (Optr, s)))
				return (NO);
			s += strlen (s);
		} else {
			if (p)		/* comma after expression */
				*p = EOS;
			ev = eval_byte ();	/* evaluate byte expression */
			if (p)
				*p = ',';	/* put comma back */
			if (ev)
				*s++ = (char)ev->uval.xvalue.low;
			else
				return (NO);	/* bad expression */
		}
		if (ev) {
			free_exp (ev);
			ev = NULL;
		}
		Optr = p ? ++p : NULL;
	}
	c = fgetc (Fd);			/* get next character in input */
	if (c == EOF && !F_stack)	/* last line in last file */
		save_prctl (String, s - String);/* save string for latter */
	else {
#if !VMS
		(void)ungetc ((char)c, Fd);	/* put character back */
#else	/* kludge for VMS 4.6 VAX C RTL */
		(void)fseek (Fd, ftell (Fd) - 1L, SEEK_SET);
#endif
		out_prctl (String, s - String); /* write out string */
	}
	Abortp = YES;			/* abort next call to print line */
	return (YES);
}

/**
*
* name		save_prctl - save Prctl control string
*
* synopsis	save_prctl (start, len)
*		char *start;	pointer to start of string
*		int len;	length of string
*
* description	This routine is called when a PRCTL directive is the last
*		line in the last input file to be assembled.  In this case,
*		we want to save the control string until all error summaries,
*		symbol table data, and cross-reference information have been
*		output.	 Then the routine done_lst will write the control
*		string to the listing file.
*
**/
static
save_prctl (start, len)
char *start;
int len;
{
	register char *p;

	if (!(Prctl = (char *)alloc (Prctl_len = len)))
		fatal ("Out of memory - cannot save PRCTL control string");
	for (p = Prctl; p < Prctl + Prctl_len; p++, start++)
		*p = *start;	/* copy control string */
}

/**
*
* name		do_equate - perform Equate directive
*
* description	Handles Equate directive processing
*
**/
static
do_equate ()
{
	int space=NONE;
	struct evres *ev; /* evaluation result */
	struct evres *eval(), *eval_ad();

	(void)chk_flds (1);

	if(*Label==EOS){
		error("EQU requires label");
		return(NO);
		}
	if( get_i_sym(Label) == ERR )
		return(NO);

	if( *(Optr + 1) == ':' ){	/* explicit memory specifier */
		if( (space = char_spc (*Optr)) == ERR || space == NONE){
			error("Invalid memory space attribute");
			return(NO);
			}
		Optr += 2;		/* move over to expression */
		}

	if( (ev = space ? eval_ad() : eval()) == NULL )
		return(NO);

	if( ev->fwd_ref ){
		error("Expression contains forward references");
		free_exp(ev);
		return(NO);
		}

	if( space )		/* explicit memory space specifier */
		if( !ev->mspace )
			ev->mspace = space;
		else if( Msw_flag && !mem_compat (ev->mspace, space) )
			warn("Expression involves incompatible memory spaces");

	(void)install(Label,ev,ev->rel?REL:NONE,NONE);

	if( ev->etype == INT ){
		if( ev->size == SIZES ){
			P_words[0] = ev->uval.xvalue.low;
			P_total = 1;
			}
		else {
			P_words[0] = ev->uval.xvalue.high;
			P_words[1] = ev->uval.xvalue.low;
			P_total = 2;
			}
		Force_int = YES;
		}
	else if( ev->etype == FPT ){
		Force_fp = YES;
		F_value = ev->uval.fvalue;
		}

	free_exp(ev);
	return(YES);
}

/**
*
* name		do_set - perform Set directive
*
* description	Handles Set directive processing
*
**/
static
do_set ()
{
	struct evres *ev; /* evaluation result */
	struct evres *eval();

	(void)chk_flds (1);

	if(*Label==EOS){
		error("SET requires label");
		return(NO);
		}
	if( get_i_sym(Label) == ERR )
		return(NO);

	if( (ev = eval()) == NULL )
		return(NO);
	if( ev->fwd_ref ){
		error("Expression contains forward references");
		free_exp(ev);
		return(NO);
		}

	(void)install(Label,ev,SET,NONE);

	if( ev->etype == INT ){
		if( ev->size == SIZES ){
			P_words[0] = ev->uval.xvalue.low;
			P_total = 1;
			}
		else {
			P_words[0] = ev->uval.xvalue.high;
			P_words[1] = ev->uval.xvalue.low;
			P_total = 2;
			}
		Force_int = YES;
		}
	else if( ev->etype == FPT ){
		Force_fp = YES;
		F_value = ev->uval.fvalue;
		}

	free_exp(ev);
	return(YES);
}

/**
*
* name		do_radix - perform Radix directive
*
* description	Handles Radix directive processing
*
**/
static
do_radix ()
{
#if !PASM
	struct evres *ev; /* evaluation result */
	struct evres *eval_int();
	int radix;

	if( (ev = eval_int()) == NULL )
		return(NO);

	if( ev->fwd_ref ){
		error("Expression contains forward references");
		free_exp(ev);
		return(NO);
		}

	radix = (int)ev->uval.xvalue.low;
	free_exp(ev);
	switch (radix) {
		case  2:
		case 10:
		case 16:
			Radix = radix;
			return(YES);
		default:
			error("Invalid radix expression");
			return(NO);
		}
#endif
}

/**
*
* name		do_title - perform Title/Subtitle directive
*
* description	Handles Title/Subtitle directive processing
*
**/
do_title (tptr)
char **tptr;
{
#if !PASM
	char *get_i_string ();

	if (Pass == 1)
		return (YES);	/* do nothing on first pass */

	if( *Operand == EOS ) { /* clear if no title/subtitle specified */
		if (*tptr) {
			free (*tptr);
			*tptr = NULL;
		}
		Abortp = YES;
		return(YES);
		}

	if( get_i_string(Operand,String) ){
		(void)chk_flds(1);
		if (*tptr)
			free (*tptr);	/* free previous title/subtitle */
		if( !(*tptr = (char *)alloc(strlen(String)+1)) )
			fatal("Out of memory - cannot save title/subtitle");
		(void)strcpy(*tptr,String);
		Abortp = YES;
		return(YES);
		}

	return(NO);
#endif
}

#if PASM
/**
*
* name		comm_line --- comment directive line
*
* description	Comments the opcode field of the directive line
*		in order to preserve the label in pre-assembler
*		output.
*
**/
comm_line()
{
	static char comment[] = {"; "};
	char line[MAXBUF+8+sizeof(comment)];
	register char *p, *q;

	for (p = Label, q = line; *p; p++, q++)
		*q = *p;	/* copy label */
	*q++ = EOS;
	*q++ = comment[0];	/* add comment filler */
	*q++ = comment[1];
	for (p++;
	     p < Pline + (sizeof(line) - (sizeof(comment) - 1));
	     p++, q++)
		*q = *p;	/* copy rest of line */
	for (p = line, q = Pline;
	     p < line + (sizeof(line) - (sizeof(comment) - 1));
	     p++, q++)
		*q = *p;	/* copy line back to buffer */
	/* adjust pointers */
	Operand += sizeof (comment) - 1;
	Xmove	+= sizeof (comment) - 1;
	Ymove	+= sizeof (comment) - 1;
	Com	+= sizeof (comment) - 1;
	if (Optr && *Optr && Optr > Op)
		Optr += sizeof (comment) - 1;
	if (Eptr && *Eptr && Eptr > Op)
		Eptr += sizeof (comment) - 1;
}
#endif
