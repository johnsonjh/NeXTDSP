#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
*	Macro processing routines
*
**/

/**
*
* name		def_macro - do macro definition
*
**/
def_macro()
{
	int j;
	struct pseuop *psu_look();
	struct mnemop *mne_look();
	struct mac *mp, *mac_look();
	struct macd *get_mdef();
	struct dumname *dalist = NULL;	/* dummy argument list */
	struct dumname *get_dargs();
	char name[MAXSYM+1];
	char *optr;

	optr = Optr;		/* save global pointer */
	Optr = Label;

	if( (j = get_i_sym(Label)) == ERR )
		return(NO);
	if( j == NO ){
		error("Missing macro name");
		return(NO);
		}

	if( *Label == '_' ){
		error2("Invalid macro name",Label);
		return(NO);
		}

	(void)strcpy (name, Label);	/* save macro name */
	Optr = NULL;			/* clear pointer for errors */

	if( Pass == 2 ){	/* see if macro name same as mnemonic */
		if( mne_look(name) != NULL )
			warn("Macro name is the same as existing assembler mnemonic");
		else if( psu_look(name) != NULL )
			warn("Macro name is the same as existing assembler directive");
		}

	if( (mp = mac_look(name)) != NULL )	/* check macro redefinition */
		if( Pass == 1 )
			return(NO);
		else{
			error2("Macro cannot be redefined",name);
			return(NO);
			}

	Optr = optr;		/* restore global pointer */

	if( *Operand == EOS )
		j = 0;		/* no dummy arguments */
	else
		if( (dalist = get_dargs(&j)) == NULL )	/* get dummy args */
			return(NO); /* error occurred */

	if( Pmacd_flag )
		print_line(' '); /* print macro definition header */

#if DEBUG
printf("Installing macro -%s-\n",name);
#endif
	if (!(mp = (struct mac *)alloc(sizeof(struct mac))))
		fatal("Out of memory - cannot save macro definition");
	if (!(mp->mname = (char *)alloc(strlen(name)+1)))
		fatal("Out of memory - cannot save macro definition");
	(void)strcpy(mp->mname,name);/* save name */
	if (Icflag)			/* ignore case */
		(void)strdn(mp->mname);
	mp->msect = Csect;		/* save section */
	mp->def_line = Lst_lno; /* save definition line # */
	mp->mdef = get_mdef(dalist);	/* get definition */
	mp->margs = j;			/* save the number of arguments */
	free_ds(dalist);		/* release dummy arguments */
	mp->next = Macro;		/* put macro into list */
	Macro = mp;
	Abortp = YES;			/* inhibit print on last line */
	return(YES);
}


/**
*
* name		get_mdef - get macro definition
*
* synopsis	dp = get_mdef(dalist)
*		struct macd *dp;	linked list of macro definition
*					NULL if none or error
*		struct dumname *dalist; dummy argument list
*
* description	Reads input lines in, translates dummy arguments, and
*		saves them in a linked list. A pass 1 error will occur
*		if the ENDM directive is missing.
*
**/
static struct macd *
get_mdef(dalist)
struct dumname *dalist;
{
	char *mne,*mapmne();
	struct pseuop *i,*psu_look();
	int macdcnt = 1;	/* nested macro definition counter */
	struct macd *cdef;	/* current macro definition */
	struct macd *odef = NULL;	/* last macro definition */
	struct macd *defst = NULL;	/* start of linked definition list */
	struct macd *tran_mline();

	while( getaline() ){
		Abortp = (Pmacd_flag) ? NO:YES;
		if( parse_line() ){
			if( strlen(Op) < MAXOP && *Op != EOS ){
				mne = mapmne();
				if( (i = psu_look(mne)) != NULL ){
					if( i->ptype == ENDM ){
						if( --macdcnt == 0 ){
#if DEBUG
	(void)printf("MACRO DEFINITION:\n");
	for( odef = defst ; odef != NULL ; odef = odef->mnext )
	(void)printf("-%s-\n",odef->mline);
	(void)printf("END OF DEFINITION\n");
#endif
							dopsulab(NOLABEL);
							(void)chk_flds(0);
							if( Pmacd_flag )
								print_line(MACDEF);
							return(defst);
							}
						}
					else if( i->ptype == MACRO ||
						 i->ptype == DUP   ||
						 i->ptype == DUPA  ||
						 i->ptype == DUPC )
						++macdcnt;
					}
				}
			print_line(MACDEF);
			}
		else
			print_comm(MACDEF);
		if( (cdef = tran_mline(dalist)) != NULL ){ /* if there is anything to save */
			if( defst != NULL ) /* if this is not the first def */
				odef->mnext = cdef;
			else
				defst = cdef;
			odef = cdef; /* mark last definition */
			}
		}
	/* if we got here then the ENDM is missing */
	error("Unexpected end of file - missing ENDM");
	free_md(defst); /* release the macro definition space */
	return(NULL);
}

/**
*
* name		tran_mline - translate & save macro definition line
*
* synopsis	np = tran_mline(dalist)
*		struct macd *np;	translated definition
*		struct dumname *dalist; dummy argument list
*
* description	The line in Line is scanned for dummy arguments, and
*		they are replaced by dummy argument markers. Comments
*		are stripped off the line if the Cm_flag is not set.
*		Unreported comments are always stripped off. NULL will
*		be returned if the line is a comment and should not be
*		saved. Otherwise, a pointer to the macro definition structure
*		is returned.
*
**/
static struct macd *
tran_mline(dalist)
struct dumname *dalist;
{
	struct macd *save_mline();
	char tline[MAXBUF+1];	/* translate buffer */
	char *symstrt = NULL;	/* dummy arg ptr */
	int strflag = NO;	/* string flag */
	char *p = Line;
	char *find_dummy();
	char *t;

	t = tline;
	while( YES ){
		if( !strflag ) /* while not processing a string */
			if( !ALPHAN(*p) ){ /* if this isn't an alphanumeric */
/*			if( !*p ||
			    isspace (*p) ||
			    *p == VALCH	 ||
			    *p == HVALCH ||
			    *p == CONCATCH ){
*/
				if( symstrt != NULL ){ /* may be a dummy arg */
					if( symstrt == tline )
						t = find_dummy(symstrt,t,dalist,YES);
					else
						t = find_dummy(symstrt,t,dalist,NO);
					symstrt = NULL;
					}
				if( *p == CONCATCH && *(p + 1) == STRCH ){
					/* escaped double quote */
					p += 2; /* skip over input */
					*t++ = STRCH;
					continue;
					}
				if( *p == COMM_CHAR ){ /* if this is a comment */
					if( *(p+1) == COMM_CHAR ){ /* unreported comment */
						if( t == tline ) /* nothing to save */
							return(NULL);
						*t = EOS;
						return(save_mline(tline));
						}
					if( Cm_flag ){	/* save comments with macros */
						while( (*t++ = *p++) != EOS );
						return(save_mline(tline));
						}
					else{ /* don't save comments */
						while( t != tline && *--t != BLANK && *t != TAB ); /* move back to first non-blank character */
						if( t == tline ) /* nothing to save */
							return(NULL);
						*t = EOS;
						return(save_mline(tline));
						}
					}
				}
			else /* if this is alphanumeric */
/*				if( symstrt == NULL && isalpha(*p) )	*/
				if( symstrt == NULL )
					symstrt = t; /* mark start of next potential dummy arg */

		if( *p == EOS ){ /* end of line */
			if( t == tline && !Cm_flag ) /* nothing to save */
				return(NULL);
			*t = EOS;
			return(save_mline(tline));
			}

		if( *p == STR_DELIM )
			strflag = !strflag;
		if( *p != STRCH || strflag )	/* string op */
			*t++ = *p++;
		else{
			*t++ = STR_DELIM;
			p++;
			}
		}
}

/**
*
* name		find_dummy - find and replace dummy arguments in line
*
* synopsis	np = find_dummy(symst,op,dalist,fflag)
*		char *np	the new end of the line
*		char *symst;	the start of a potential dummy arg
*		char *op;	the old end of line
*		struct dumname *dalist; the dummy argument list
*		int fflag;	YES indicates that symstart = first char of buffer
*
* description	The symbol pointed to by symst is checked to see if it is a
*		dummy argument name. If it is not, then the old end of string
*		value is returned. If it is, then the character preceding the
*		dummy argument is checked for concatenation operators and
*		value operators. The operators (if present) and the dummy
*		argument name are replaced by a special two character
*		sequence:  the first character is the number of the
*		dummy argument with bit 7 set. The second character
*		is a value flag. If VALTYP,
*		then on expansion, the dummy argument is replaced with the
*		value of the argument rather than the string. Otherwise,
*		the flag is set to STRTYP.  If a concatenation operator is
*		present but the symbol is not a dummy argument, the operator
*		byte is replaced by a byte with the high bit set and no
*		macro argument count.  This signals the input routine to
*		throw away the operator byte at expansion time.
*
**/
static char *
find_dummy(symst,op,dalist,fflag)
char *symst,*op;
struct dumname *dalist;
int fflag;
{
	char typflag = STRTYP; /* type of argument */

	if( dalist == NULL ) /* no dummy arguments */
		return(op);

	*op = EOS;	/* put in temp end of string */

	if( strlen(symst) > MAXSYM ) /* too long to be dummy arg */
		return(op);

	do{
		if( STREQ(symst,dalist->daname) ){ /* look for match */
			if( !fflag ){	/* if not the first char of buffer */
				if( *(symst-1) == VALCH ){ /* value op */
					--symst;
					typflag = VALTYP;
					}
				else if( *(symst-1) == HVALCH ){ /* hex op */
					--symst;
					typflag = HEXTYP;
					}
				if( *(symst-1) == CONCATCH ) /* concat op */
					--symst;
				}
			*symst++ = MACARG | dalist->num; /* mark dummy arg location */
			*symst++ = typflag; /* indicate its type */
			return(symst);
			}
	}while( (dalist = dalist->dnext) != NULL );

	if( !fflag && *--symst == CONCATCH )	/* trailing concat operator */
		*symst = MACARG;		/* set high bit w/o count */

	return(op); /* no match */
}

/**
*
* name		save_mline - save macro definition line
*
* synopsis	md = save_mline(def)
*		struct macd *md;	the saved line
*		char *def;		the definition
*
* description	Saves the definition line in a macd structure. Fatal errors
*		will occur if out of memory.
*
**/
struct macd *
save_mline(def)
char *def;
{
	struct macd *d;

	if (!(d = (struct macd *)alloc(sizeof(struct macd))))
		fatal("Out of memory - cannot save macro definition");
	if (!(d->mline = (char *)alloc(strlen(def)+1)))
		fatal("Out of memory - cannot save macro definition");
	(void)strcpy(d->mline,def);
	d->mnext = NULL;
	return(d);
}

/**
*
* name		get_dargs - get list of dummy arguments
*
* synopsis	dalist = get_dargs(num)
*		struct dumname *dalist;	 linked list of dummy arguments
*					 NULL if error
*		int *num;		ptr to storage of number of args
*
* description	Collects the list of dummy argument names into a linked
*		list. Errors will occur if the symbol names are invalid, if
*		there is a syntax error, or if two dummy arguments are
*		named the same. Fatal errors will occur if memory allocation
*		fails.
*
**/
static struct dumname *
get_dargs(num)
int *num;
{
	char *dname;	/* dummy argument name */
	char *get_sym();
	struct dumname *nd = NULL; /* start of linked list */
	struct dumname *od;
	struct dumname *op;
	int argnum = 0;		/* argument number */

	Optr = Operand;
	while( *Optr != EOS ){
		if( *Optr == '_' ){
			if( nd )
				free_ds(nd);
			error("Syntax error in dummy argument list");
			return(NULL);
			}

		if( !(dname = get_sym()) ){ /* get dummy argument name */
			if( nd )
				free_ds(nd);
			return(NULL);
			}

		if( *Optr != EOS )
			if( *Optr++ != ',' ){
				if( nd )
					free_ds(nd);
				error("Syntax error in dummy argument list");
				return(NULL);
				}

		if( !nd ){
			if (!(od = nd = (struct dumname *)alloc(sizeof(struct dumname))))
				fatal("Out of memory - cannot save macro definition");
			}
		else{
			op = nd;
			do{
				if( STREQ(dname,op->daname) ){
					free_ds(nd);
					error2("Two dummy arguments are the same",dname);
					return(NULL);
					}
				od = op;
				op = op->dnext;
			}while( od->dnext );
			if (!(od = od->dnext = (struct dumname *)alloc(sizeof(struct dumname))))
				fatal("Out of memory - cannot save macro definition");
			}
		od->dnext = NULL;
		if( !(od->daname = (char *)alloc(strlen(dname)+1)) )
			fatal("Out of memory - cannot save macro definition");
		(void)strcpy(od->daname,dname);
		od->num = ++argnum;
		}
	*num = argnum; /* save the number of arguments */
	return(nd);
}

/**
*
* name		free_ds - free list of dummy args
*
* synopsis	free_ds(nd)
*		struct dumname *nd;	head of dummy argument list
*
* description	Frees the dummy argument list pointed to by nd.
*
**/
static
free_ds(nd)
struct dumname *nd;
{
	struct dumname *od;

	while(nd){ /* free dummy arg list */
		if (nd->daname)
			free (nd->daname);
		od = nd->dnext;
		free((char *)nd);
		nd = od;
		}
}

/**
*
* name		free_md - free macro definition list
*
* synopsis	free_md(nd)
*		struct macd *nd;	start of definition list
*
* description	Releases the memory allocated to the macro definition.
*
**/
free_md(nd)
struct macd *nd;
{
	struct macd *od;

	while(nd){ /* free definition list */
		if (nd->mline)
			free (nd->mline);
		od = nd->mnext;
		free((char *)nd);
		nd = od;
		}
}

/**
*
* name		free_mac - free list of macros
*
* description	Releases the memory allocated to the macro definition list.
*
**/
free_mac()
{
	struct mac *np = Macro;
	struct mac *op;

	while(np){
		if( np->mdef )
			free_md(np->mdef);
		op = np->next;
		free((char *)np);
		np = op;
		}
	Macro = NULL;
}

/**
*
* name		mac_look - look up macro name
*
* synopsis	mp = mac_look(name)
*		struct mac *mp;		pointer to macro structure
*					NULL if not found
*		char *name;		null terminated macro name
*
* description	Searches the macro table for the name.
*
**/
struct mac *
mac_look(name)
char *name;
{
	struct mac *p;
	char buf[MAXSYM+1]; /* macro name buffer */

	if( Icflag ){		/* ignore case */
		(void)strcpy (buf, name);
		(void)strdn (buf);
		name = buf;
		}
	for( p = Macro ; p ; p = p->next )
		if( STREQ(name,p->mname) &&
		    (p->msect == &Glbsec || p->msect == Csect) )
			break;
	return(p);
}

/**
*
* name		do_dup - perform dup directive
*
* synopsis	yn = do_dup()
*		int yn;		NO if error
*
* description	Handles dup directive.
*
**/
do_dup()
{
	struct evres *er;
	int count;
	struct macd *def;
	struct macexp *expdef;
	struct evres *eval_int();
	struct macexp *init_exp();

	(void)chk_flds(1);
	Abortp = (Pmacd_flag) ? NO:YES; /* inhibit printing if necessary */
	Optr = Operand;
	if( (er = eval_int()) == NULL ){	/* error in evaluation */
		print_line(' '); /* print the DUP directive line */
		skip_def();
		return(NO);
		}
	if( er->etype != INT ){
		error("Count must be an integer value");
		free_exp(er);
		print_line(' '); /* print the DUP directive line */
		skip_def();
		return(NO);
		}
	if( er->fwd_ref ){
		error("Expression contains forward references");
		free_exp(er);
		print_line(' '); /* print the DUP directive line */
		skip_def();
		return(NO);
		}
#if PASM
	comm_line();	/* comment macro invocation line */
#endif
	print_line(' '); /* print the DUP directive line */
	count = (int)er->uval.xvalue.low;
	free_exp(er);
	if( count <= 0 ){ /* skip */
		skip_def();
		return(YES);
		}
	def = get_mdef((struct dumname *)NULL); /* get definition */
	if( def == NULL ){
		Abortp = YES;	/* inhibit print on last line */
		return(NO);	/* missing or null definition */
		}

	while( count-- ){ /* loop for count */
		expdef = init_exp(def); /* init expansion */
		exp_mac (expdef);	/* expand macro */
		}

	Abortp = YES; /* inhibit print on last line */
	free_md(def); /* release the DUP definition */
	return(YES);	/* DUP complete */
}

/**
*
* name		do_dupa - perform dupa directive
*
* synopsis	yn = do_dupa()
*		int yn;		NO if error
*
* description	Handles dupa directive.
*
**/
do_dupa()
{
	struct dumname *da;
	struct dumname *dup_arg();
	int count = 0;
	struct macd *def;
	struct macexp *expdef;
	struct macexp *init_exp();
	struct macd *alist; /* argument list */
	struct macd *get_eargs();

	(void)chk_flds(1);
	Abortp = (Pmacd_flag) ? NO:YES; /* inhibit printing if necessary */
	Optr = Operand;
	if( (da = dup_arg()) == NULL ){ /* error in dummy argument */
		print_line(' '); /* print the DUPA directive line */
		skip_def();
		return(NO);
		}
	if( *Optr == EOS ){ /* no arguments */
		print_line(' '); /* print the DUPA directive line */
		free((char *)da);
		skip_def();
		return(YES);
		}
	if( (alist = get_eargs(&count)) == NULL ){
		print_line(' '); /* error in arg list */
		free((char *)da);
		skip_def();
		return(NO);
		}

#if PASM
	comm_line();	/* comment macro invocation line */
#endif
	print_line(' '); /* print the DUP directive line */

	def = get_mdef(da);	/* get definition */
	free((char *)da);
	if( def == NULL ){
		free_al(alist); /* release the argument list */
		Abortp = YES;	/* inhibit print on last line */
		return(NO);	/* missing or null definition */
		}

	while( alist != NULL ){		/* loop until no more args */
		expdef = init_exp(def); /* init expansion */
		expdef->args = alist;	/* add an argument */
		alist = alist->mnext;	/* reset list ptr */
		expdef->args->mnext = NULL; /* seal off arg list */
		expdef->no_args = 1;	/* set for 1 argument */
		exp_mac (expdef);	/* expand macro */
		}

	Abortp = YES; /* inhibit print on last line */
	free_md(def); /* release the DUPA definition */
	return(YES);
}

/**
*
* name		do_dupc - perform dupc directive
*
* synopsis	yn = do_dupc()
*		int yn;		NO if error
*
* description	Handles dupc directive.
*
**/
do_dupc()
{
	char *tmp;
	char *charlist; /* argument string */
	char *charptr;	/* current argument character */
	struct dumname *da;
	struct dumname *dup_arg();
	struct macd *def;
	struct macexp *expdef;
	struct macexp *init_exp();
	char *get_string ();

	(void)chk_flds(1);
	Abortp = (Pmacd_flag) ? NO:YES; /* inhibit printing if necessary */
	Optr = Operand;
	if( (da = dup_arg()) == NULL ){ /* error in dummy argument */
		print_line(' '); /* print the DUPC directive line */
		skip_def();
		return(NO);
		}
	if( *Optr == EOS ){ /* no arguments */
		print_line(' '); /* print the DUPC directive line */
		free((char *)da);
		skip_def();
		return(YES);
		}
	if( *Optr == STR_DELIM || *Optr == XTR_DELIM ){
		Optr = get_string(Optr,String);
		if( *Optr != EOS ){
			error("Syntax error in argument list");
			print_line(' '); /* print the DUPC directive line */
			free((char *)da);
			skip_def();
			return(NO);
			}
		}
	else{
		tmp = String;
		while (*tmp)
			*tmp++ = *Optr++;
		}
	if( (charlist = (char *)alloc(strlen(String) + 1)) == NULL )
		fatal("Out of memory - cannot save macro arguments");
	(void)strcpy(charlist,String);
	charptr = charlist;

#if PASM
	comm_line();	/* comment macro invocation line */
#endif
	print_line(' '); /* print the DUP directive line */

	def = get_mdef(da);	/* get definition */
	free((char *)da);
	if( !def ){
		free(charlist); /* release the argument list */
		Abortp = YES;	/* inhibit print on last line */
		return(NO);	/* missing or null definition */
		}

	while( *charptr ){ /* loop until no more args */
		expdef = init_exp(def); /* init expansion */
		/* add an argument */
		if ( ((expdef->args = (struct macd *)alloc(sizeof(struct macd))) == NULL) ||
		     ((tmp = (char *)alloc (2)) == NULL) ) /* must alloc; freed elsewhere */
			fatal("Out of memory - cannot expand macro");
		*tmp = *charptr++;
		*(tmp + 1) = EOS;
		expdef->args->mline = tmp;
		expdef->args->mnext = NULL; /* seal off arg list */
		expdef->no_args = 1;	/* set for 1 argument */
		exp_mac(expdef);	/* expand macro */
		}

	Abortp = YES;	/* inhibit print on last line */
	free(charlist); /* release the argument list */
	free_md(def);	/* release the DUPC definition */
	return(YES);
}

/**
*
* name		dup_arg - get dummy argument for DUPA and DUPC directives
*
* synopsis	da = dup_arg()
*		struct dumname *da;  dummy argument
*					 NULL if error
*
* description	Collects the dummy argument name into a structure.
*		Errors will occur if the symbol name is invalid or missing.
*		Fatal errors will occur if memory allocation
*		fails.
*
**/
static struct dumname *
dup_arg()
{
	char *dname;		/* dummy argument name */
	char *get_sym();
	struct dumname *nd;	/* dummy name structure */

	if( *Operand == EOS ){
		error("Missing argument");
		return(NULL);
		}

	Optr = Operand;
	if( *Optr == '_' ){
		error2("Invalid dummy argument name",Optr);
		return(NULL);
		}

	if( (dname = get_sym()) == NULL ){ /* get dummy argument name */
		return(NULL);
		}

	if( *Optr != EOS )
		if( *Optr++ != ',' ){
			error("Syntax error in dummy argument list");
			return(NULL);
			}

	if (!(nd = (struct dumname *)alloc(sizeof(struct dumname))))
		fatal("Out of memory - cannot save macro definition");
	nd->dnext = NULL;
	if( !(nd->daname = (char *)alloc(strlen(dname)+1)) )
		fatal("Out of memory - cannot save macro definition");
	(void)strcpy(nd->daname,dname);
	nd->num = 1;
	return(nd);
}

/**
*
* name		skip_def - skip macro definition
*
* description	Skips the macro definition in cases of syntax errors
*		for the DUP, DUPC, DUPA directives.
*
**/
static
skip_def()
{
	struct macd *def;

	def = get_mdef((struct dumname *)NULL); /* get the definition */
	free_md(def);		/* release the macro def */
	Abortp = YES;		/* suppress printing of last line */
}

/**
*
* name		do_pmacro - perform Pmacro directive
*
* description	Performs pmacro directive processing.
*
**/
do_pmacro()
{
	char *mname; /* macro name */
	struct mac *mp, *mq;
	char *get_sym();

	(void)chk_flds(1);
	Optr = Operand;
	if( !*Optr ){
		error("Missing macro name");
		return(NO);
		}
	while( *Optr ){
		if( *Optr == '_' ){
			error("Invalid macro name");
			return(NO);
			}
		if( !(mname = get_sym()) ) /* get macro name */
			return(NO);
		for( mp = mq = Macro ; mp ; mq = mp, mp = mp->next )
			if( STREQ(mname,mp->mname) )
				break;
		if( !mp )
			error2("Macro not defined",mname);
		else{
			free_md(mp->mdef); /* release memory allocated to def */
			if (mp == Macro)	/* relink macro chain */
				Macro = mp->next;
			else
				mq->next = mp->next;
			free ((char *)mp);	/* free macro header */
			}
		if( *Optr && *Optr++ != ',' ){
			error("Syntax error in macro name list");
			return(NO);
			}
		}
	return(YES);
}

/**
*
* name		mac_lib --- check macro directory list for macro file
*
* synopsis	yn = mac_lib (name)
*		int yn;		YES if macro file found, NO otherwise
*		char *name;	Macro name to find
*
* description	Search the list of macro directories for the named
*		macro file.  If found, save the current file state,
*		open the new file, and treat it like an INCLUDE file.
*		If the file is not found or cannot be opened, return NO.
*
**/
mac_lib (name)
char *name;
{
	char *get_i_string();
	struct filelist *np;
	FILE *fld;
	struct dumname *ip;

	/* make sure we haven't already been here with this macro */
	if (!mlib_chk (name))
		return (NO);

	/* scan the MACLIB directory paths */
	for( ip = Mextend ; ip ; ip = ip->dnext ){
		(void)strcpy(String,ip->daname); /* put the directory string in String */
		(void)strcat(String,name); /* concatenate the filename */
		fix_fn(".asm"); /* supply default extension */
		if( (fld = fopen(String,"r")) != NULL ){
			if (Verbose)
				(void)fprintf (stderr, "%s: Opening macro file %s\n", Progname, String);
			break;
			}
		}
	if( !ip ) /* if end of MACLIB directories */
		return(NO);

	if (!(np = (struct filelist *)alloc(sizeof (struct filelist))))
		fatal("Out of memory - cannot save filename");
	np->ifname = Cfname;	/* save current filename */
	np->ifd = Fd;		/* save file descriptor */
	np->fnum = Cfn;		/* save current file number */
	np->iln = --Real_ln;	/* save current line number - 1 */
	if (!(np->prevline = (char *)alloc (strlen (Line) + 1)))
		fatal ("Out of memory - cannot save previous line");
	(void)strcpy (np->prevline, Line);	/* save previous line */
	np->inext = F_stack;	/* insert into stack */
	F_stack = np;
	++Cfn;			/* bump file number */
	set_cfn(String);	/* setup current filename */
	Fd = fld;		/* setup file descriptor */
	Real_ln = 0;		/* initialize line number */
	--Lst_lno;		/* decrement listing line number */
	push_imode (FILEMODE);	/* change input mode */
	Abortp = YES;		/* don't print this macro line */

	return(YES);
}

/**
*
* name		mlib_chk --- ensure MACLIB called once for given macro
*
* synopsis	yn = mlib_chk (name)
*		int yn;		YES if macro name not found, NO otherwise
*		char *name;	Macro name to find
*
* description	Search list of macro invocation names.	If found,
*		it means a previous MACLIB file did not contain the
*		definition of the given macro, hence the macro is
*		undefined and NO is returned. If the name is not
*		found, it is added to the list and YES is returned.
*
**/
static
mlib_chk (name)
char *name;
{
	struct symref *np;

	np = Mlib;
	while (np && !(STREQ (np->name, name) && np->sec == Csect))
		np = np->next;
	if (np)			/* found */
		return (NO);	/* already processed this macro */

	/* if we got here, the macro name was not found; add it to the list */
	if ( !(np = (struct symref *)alloc (sizeof (struct symref))) ||
	     !(np->name = (char *)alloc (strlen (name) + 1)) )
		fatal ("Out of memory - cannot allocate MACLIB check record");
	(void)strcpy (np->name, name);	/* copy name */
	np->sec = Csect;		/* save section pointer */
	np->next = Mlib;		/* link into chain */
	Mlib = np;
	return (YES);
}

/**
*
* name		do_mac - initialize and perform macro expansion
*
* synopsis	do_mac(mp)
*		struct mac *mp;		macro definition header
*
* description	Collects the macro expansion arguments. Initializes
*		Mac_exp to point to macro expansion.  Calls exp_mac
*		to do the actual macro expansion.
*
**/
do_mac(mp)
struct mac *mp;
{
	struct macexp *init_exp();
	struct macexp *me;	/* expansion structure */
	int args = 0;		/* number of arguments */
	struct macd *get_eargs();

#if DEBUG
	(void)printf("Initializing macro  -%s-\n",mp->mname);
#endif
	(void)chk_flds(1);		/* check fields */
	Abortp = (Mc_flag) ? NO:YES; /* inhibit print of macro call if reqd */
	if( !mp->mdef ){
		print_line(' ');	/* print the macro call line */
		Abortp = YES;		/* inhibit print on last line */
		return(NO);		/* missing or null definition */
		}
	me = init_exp(mp->mdef);
	Optr = Operand;
	if( *Operand ) /* if there are any args */
		if( (me->args = get_eargs(&args)) == NULL ){
			free_me(me);	/* error occurred */
			print_line(' ');/* print the macro call line */
			Abortp = YES;	/* inhibit print on last line */
			return(NO);
			}

	me->no_args = args;	/* save number of expansion arguments */
	if( args > mp->margs )
		warn("Number of macro expansion arguments is greater than definition");
	else if( args < mp->margs )
		warn("Number of macro expansion arguments is less than definition");

#if PASM
	comm_line();		/* comment macro invocation line */
#endif
	print_line(' ');	/* print the macro call line */
	if( !Macro_exp )	/* if no macro expansion active */
		Force_prt = YES;/* then force the print */
	if( *Label )
		do_label();	/* install label in symbol table */
	exp_mac(me);		/* expand the macro definition */
	Abortp = YES;		/* suppress print of last line */
	return(YES);
}

/**
*
* name		init_exp - initialize macro expansion
*
* synopsis	mep = init_exp(dp)
*		struct macexp *mep;	the expansion structure
*		struct macd dp;		the macro definition list ptr
*
* description	Initializes a macro expansion structure.
*
**/
struct macexp *
init_exp(dp)
struct macd *dp;
{
	struct macexp *me;
	struct explcl *el;

	if (!(me = (struct macexp *)alloc(sizeof(struct macexp))))
		fatal("Out of memory - cannot expand macro");
	me->no_args = 0;	/* default for no args */
	me->args = NULL;	/* init expansion args */
	me->defline = dp;	/* init definition line ptr */
	me->prevline = NULL;	/* init prev. def. line ptr */
	me->lc = dp->mline;	/* init character pointer */
	me->curarg = NULL;	/* initialize argument expansion ptr */
	me->eline = Lst_lno;	/* save the expansion line no */
	me->lcl = NULL;		/* default to no local label list */
	if (Macro_exp)		/* macro expansion already in progress */
		me->prevlcl = Curr_explcl;	/* save curr. exp. list */
	else
		me->prevlcl = NULL;
	if (Pass == 1) {	/* allocate new expansion list on Pass 1 */
		if ((el = (struct explcl *)alloc (sizeof (struct explcl))) == NULL)
			fatal("Out of memory - cannot save macro local label list");
		el->list = NULL;	/* no symbols yet */
		el->eslln = Lst_lno;	/* save expansion line number */
		el->lnext = NULL;
		if( Exp_lcl )		/* if expansion local label list */
			Last_explcl = Last_explcl->lnext = el;
		else
			Exp_lcl = Last_explcl = el;
		Curr_explcl = el;	/* set current exp. list pointer */
	} else {		/* Pass 2; reset exp. label list pointers */
		if (!Last_explcl)	/* at beginning of list */
			Curr_explcl = Last_explcl = Exp_lcl;
		else
			Curr_explcl = Last_explcl = Last_explcl->lnext;
		me->lcl = Curr_explcl->list;	/* set exp. label list */
	}
	return(me);
}

/**
*
* name		exp_mac - expand macro definition
*
* synopsis	exp_mac (expdef)
*		struct macexp *expdef;	macro expansion structure
*
* description	Expands the macro definition by reading the stored lines
*		and processing them.  Links the macro expansion structure
*		into the expansion linked list.	 Stacks and unstacks any
*		ongoing macro expansion.
*
**/
exp_mac (expdef)
struct macexp *expdef;
{
	expdef->next = Macro_exp;	/* stack current expansion */
	Macro_exp = expdef;
	push_imode (MACXMODE);		/* change input mode */
	while( getaline() ){		/* while more definition lines */
		reset_pflags();		/* reset force print flags */
		if( parse_line() ){	/* parse line */
			(void)process(NO,NO);
			print_line(' ');
			}
		else
			print_comm(' ');
		P_total = 0;		/* reset byte count */
		Cycles = 0;		/* and per instruction cycle count */
		}
	if (pop_imode () != MACXMODE)	/* restore input mode */
		fatal ("Input mode stack out of sequence");
	if (expdef->prevlcl)		/* previous local label list */
		Curr_explcl = expdef->prevlcl;
	else
		Curr_explcl = Last_explcl;
	Macro_exp = Macro_exp->next;
	free_me (expdef);		/* unstack expansion */
}

/**
*
* name		get_eargs - get macro expansion args
*
* synopsis	al = get_eargs(np)
*		struct macd *al;	linked list of arguments
*		int *np;		number of arguments
*
* description	Collects the expansion arguments into a linked list.
*
**/
struct macd *
get_eargs(np)
int *np;
{
	int args = 0;	/* number of arguments */
	struct macd *al = NULL; /* argument list */
	struct macd *od = NULL;
	char *tmp;
	char *get_string();

	while( *Optr ){
		++args;
		if( *Optr == STR_DELIM || *Optr == XTR_DELIM ){
			/* string argument */
			if ((Optr = get_string(Optr,String)) == NULL)
				return (NULL);
			}
		else{	/* non-string argument */
			tmp = String;
			while( *Optr ){
				if( *Optr == ',' )
					break;
				*tmp++ = *Optr++;
				}
			*tmp = EOS;
			}
		if( !al ){ /* handle first argument */
			al = (struct macd *)alloc(sizeof(struct macd));
			od = al;
			}
		else{
			od->mnext = (struct macd *)alloc(sizeof(struct macd));
			od = od->mnext;
			}

		if( !od )
			fatal("Out of memory - cannot expand macro");

		od->mnext = NULL;
		if (!(od->mline = (char *)alloc(strlen(String)+1))) /* save argument */
			fatal("Out of memory - cannot expand macro");
		(void)strcpy(od->mline,String);
		if( *Optr )
			if( *Optr++ != ',' ){
				error("Syntax error in macro argument list");
				*np = 0;
				free_al(al);
				return(NULL);
				}
		}
	*np = args;
	return(al);
}

/**
*
* name		free_me - release macro expansion structure
*
* synopsis	free_me(me)
*		struct macexp *me;	structure to be freed
*
* description	Releases all allocated memory associated with the macro
*		expansion.
*
**/
free_me(me)
struct macexp *me;
{
	free_al(me->args);	/* release the argument list */
	free((char *)me);	/* release the expansion structure */
}

/**
*
* name		free_al - free expansion argument list
*
* synopsis	free_al(lp)
*		struct macd *lp;	start of list
*
* description	Frees memory allocated to the expansion argument list.
*
**/
free_al(lp)
struct macd *lp;
{
	struct macd *np;

	while( lp ){	/* free argument list */
		if (lp->mline)
			free (lp->mline);
		np = lp->mnext;
		free((char *)lp);
		lp = np;
		}
}

/**
*
* name		free_ml - free macro library list
*
* synopsis	free_ml()
*
* description	Frees the macro library list.
*
**/
free_ml()
{
	struct dumname *nd, *od;

	nd = Mextend;
	Mextend = NULL;
	while(nd){ /* free macro library list */
		if (Cflag && nd->num){
			Mextend = nd;
			break;
			}
		if (nd->daname)
			free (nd->daname);
		od = nd->dnext;
		free((char *)nd);
		nd = od;
		}
}

/**
*
* name		free_mc - free MACLIB check list
*
* description	Releases the memory allocated to the MACLIB macro check list.
*
**/
free_mc()
{
	struct symref *np = Mlib;
	struct symref *op;

	while(np){
		if( np->name )
			free(np->name);
		op = np->next;
		free((char *)np);
		np = op;
		}
	Mlib = NULL;
}
