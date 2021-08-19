#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		install --- add a symbol to the table
*
* synopsis	yn = install(str,val,att,map)
*		int yn;		YES / NO for errors
*		char *str;	null terminated symbol name
*		struct evres *val;	value
*		int att;	attribute flags
*		int map;	mapping flags
*
* description	If the symbol to be installed is not a local label,
*		then the symbol is looked up in the symbol table
*		first. If Pass 1 and it is not found, it is installed.
*		If Pass 2 and the symbol is not found, an error is generated.
*		If it is found, then if Pass 1, the redefinition error
*		attribute flag is set unless the symbol is being assigned
*		with the SET directive or was previously used with the
*		SET directive. If Pass 2, then the value is
*		tested against saved value of the symbol. If different
*		and the symbol is not a SET symbol, then a phasing error
*		has occurred. If the symbol is a SET symbol and the att
*		indicates SET, then the value is changed to the new value.
*		If att does not indicate SET, then an attempt to assign
*		a value to a SET symbol with other than the SET directive
*		mechanism has occurred, and an error is generated.
*
*		The mechanism for local labels is slightly different.
*		A list of the most recent local labels is maintained.
*		When a non-local label is encountered, the list is purged.
*		The rules described above are the same for local labels,
*		with the exception that the label is looked up in the
*		local label list instead of the main symbol table.
*
*		The value of the symbol is contained in the structure pointed
*		to by val. This value may be a floating point value, or an
*		integer value.
*
*		If there is no current section, or if the symbol appears in
*		the XDEF list of the current section, the symbols attribute
*		is set to GLOBAL.
*
**/
install(str,val,att,map)
char	*str;	/* symbol name */
struct evres *val;	/* value */
int	att;	/* symbol attribute flags */
int	map;	/* symbol mapping flags */
{
#if !SASM
	register struct nlist *np;
	struct nlist *lookup();

	if( *str == '_' )	/* local label */
		att |= LOCAL;	/* set local label attribute */

	att |= val->etype;	/* set type flag */

	if( !(att & SET) )	/* not a set symbol */
		att |= val->mspace;	/* set memory space attribute */

	if (att & REL)		/* relocatable symbol */
		Csect->flags |= RELERR; /* set relocatable error flag */

	if( (np = lookup(str,DEFTYPE)) != NULL )/* look for symbol in table */
		/* global returned as local */
		if( np->attrib & GLOBAL &&	/* symbol is global */
		    !(att & SET) &&		/* OK if set symbol */
		    np->sec != Csect &&		/* not defined in curr sec */
		    Csect != &Glbsec &&		/* not in GLOBAL section */
		    !xdef_look(str) )		/* symbol not XDEF */
		    	if ( !(att & SET) )	/* OK if set symbol */
				np = NULL;	/* not the right one */

	if( np ){				/* symbol is in table */

		if( !Macro_exp || Scs_mdef ){
			if( att & LOCAL ){	/* local label */
				if( !Scs_mdef )
					Local = YES;
				}
			else if( Local && Curr_lcl ){	/* non-local label */
				Local = NO;
				Curr_lcl = Curr_lcl->next; /* next list */
				}
			}

		if( (att & SET) != 0 )	/* if this is a set directive */
			return (do_setsym (np, val));	/* do set symbol */

		if( Pass == 1 ){
			if( !(np->attrib & SET) )	/* not a set symbol */
				np->attrib |= RDEFERR;	/* pass 1 set flag */
			return(NO);
			}

		/* to get here, it must be pass 2 and symbol is in table */
		if( np->attrib & RDEFERR ){
			error2("Symbol redefined",str);
			return(NO);
			}

		if( (np->attrib & SET) != 0 ){
			error2("Symbol already used as SET symbol",str);
			return(NO);
			}

		/* see if there is a phasing error */
		if( np->attrib & INT ){
			if( val->etype == INT )
				if( np->def.xdef.low == val->uval.xvalue.low )
					return(YES);
			}
		else if( np->attrib & FPT ){
			if( val->etype == FPT )
				if( np->def.fdef == val->uval.fvalue)
					return(YES);
			}
		error2("Phasing error",str);
		return(NO);
		}
	if( Pass == 2 )		/* symbol not defined on pass 2 */
		return(NO);

	/* install new symbol */
#if DEBUG
	printf("Installing %s as ",str);
	if( val->etype == INT )
		(void)printf("%ld ($%lx)\n",val->uval.xvalue.low,val->uval.xvalue.low);
	else
		(void)printf("%g\n",val->uval.fvalue);
	(void)printf("section: %s\n",Csect->sname);
#endif
	if( Csect == &Glbsec || xdef_look(str) )
		att |= GLOBAL; /* set global for outside section or XDEF */

	if( att & LOCAL ){	/* local label */
		++str;		/* move pointer past underscore */
		if (!Macro_exp)
			++Lsym_cnt;	/* count non-macro local labels */
		}
	else
		++Sym_cnt;	/* count non-local labels */

	np = (struct nlist *)alloc(sizeof(struct nlist));
	if( np == NULL )
		fatal("Out of memory - symbol table full");
	np->name = (char *)alloc(strlen(str)+1);
	if( np->name == NULL )
		fatal("Out of memory - symbol table full");
	(void)strcpy(np->name,str);	/* save name */
	if (Icflag)			/* ignore case */
		(void)strdn (np->name);
	if ( att & INT ){
		np->def.xdef.low  = val->uval.xvalue.low; /* save int value */
		np->def.xdef.high = val->uval.xvalue.high;
		np->def.xdef.ext  = val->uval.xvalue.ext;
		}
	else	/* save floating point value */
		np->def.fdef = val->uval.fvalue; /* save fp value */
	if (val->size == SIZEL) /* check value size */
		att |= LONG;	/* note long value */
	np->attrib = att;	/* save attributes */
	np->mapping = map;	/* save mapping */
	np->sec = Csect;	/* save section ptr */
	np->xrf = NULL;		/* init. pointers */
	np->next = NULL;
	if (!(att & LOCAL))	/* non-local label */
		Curr_lcl = NULL;/* reset local label list */
	if (Last_sym)		/* if there is a valid preceding entry */
		Last_sym->next = np;/* attach to end of symbol list */
	else if( att & LOCAL )	/* local label */
		instlcl(np);	/* symbol belongs in local lbl list */
	else			/* symbol belongs in normal symbol table */
		Hashtab[hash(np->name)] = np;/* hash into symbol table */
	return(YES);
#endif /* SASM */
}

/**
*
* name		instlcl - install local label in symbol list
*
* synopsis	instlcl(np)
*		struct nlist *np;	symbol table entry
*
* description	Install a local label in either the standard local
*		label list or the macro expansion local label list,
*		depending on whether macro expansion is active or not.
*
**/
static
instlcl(np)
struct nlist *np;
{
#if !SASM
	struct symlist *sl;
	static struct symlist *ll;	/* last local label list */

	if (Macro_exp)		/* macro expansion active */
		Macro_exp->lcl = Curr_explcl->list = np;
	else {			/* no macro expansion active */
		if ((sl = (struct symlist *)alloc (sizeof (struct symlist))) == NULL)
		fatal("Out of memory - cannot save local label list");
		if (!Loclab)	/* no local label list started */
			ll = Curr_lcl = Loclab = sl;
		else		/* save current list */
			ll = ll->next = Curr_lcl = sl;
		sl->sym = np;	/* save symbol ptr */
		sl->next = NULL;
	}
#endif
}

/**
*
* name		do_setsym --- process SET symbol
*
* synopsis	yn = do_setsym(np,val)
*		int yn;			YES=is set symbol
*		struct nlist *np;	symbol table entry
*		struct evres *val;	symbol value
*
* description	Resets the SET symbol value as given by val.
*		If np is not a SET symbol an error is issued.
*
**/
static
do_setsym(np,val)
struct nlist *np;
struct evres *val;
{
#if !SASM
	if( np->attrib & SET ){ /* symbol is set symbol */
		np->attrib &= ~TYPEMASK;	/* reset type */
		if(val->etype == INT){		/* assign int value */
			np->attrib |= INT;
			np->def.xdef.low  = val->uval.xvalue.low;
			np->def.xdef.high = val->uval.xvalue.high;
			np->def.xdef.ext  = val->uval.xvalue.ext;
			}
		else if( val->etype == FPT ){	/* assign float value */
			np->attrib |= FPT;
			np->def.fdef = val->uval.fvalue;
			}
		if (val->size == SIZEL)		/* check value size */
			np->attrib |= LONG;	/* note long value */
		return(YES);
		}
	else{
		error2("Symbol cannot be set to new value",np->name);
		return(NO);
		}
#endif
}

/**
*
* name		lookup --- find string in symbol table
*
* synopsis	sp = lookup(name,rtyp)
*		struct nlist *sp;	pointer to symbol table entry
*					(NULL if not found)
*		char *name;		null terminated symbol name
*		int rtyp;		reference type
*
* description	If the label is a local label, then the local label list
*		is searched. If not, then the main symbol table is searched
*		for the symbol name. An error will result if the symbol
*		name is not found on pass 2. On pass 2, if the Cre_flag is
*		set, a cross reference listing will be added to the symbol
*		table. The reference type (rtyp) will be preserved in the
*		cross reference listing.
*
**/
struct nlist *
lookup(name,rtyp)
char	*name;
int rtyp;
{
	struct nlist *np;
	struct nlist *lkuplcl(), *srchsym ();
	int	hashval;	/* hash table offset */
	char	symbuf[MAXSYM + 1]; /* symbol name buffer */
#if SASM
	error("Symbols not supported");
	return(NULL);
#else
	if( !Chkdo )		/* if not reprocessing line */
		Lkp_count++;	/* increment lookup count */
	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, name);
		(void)strdn (symbuf);
		name = symbuf;
		}
	if( *name == '_' )	/* local label */
		return(lkuplcl(name,rtyp));

	hashval = hash(name);	/* hash symbol name */
	Last_sym = Hashtab[hashval];	/* save symbol */
	if( (np = srchsym (name)) != NULL ) {
		if( Pass == 2 && Cre_flag && !Chkdo)
			xref_sym (np, rtyp); /* cross ref listing */
		return (np);
		}
	if( !Lnkmode && Pass==2 )
		error2("Symbol undefined on pass 2",name);

	return(NULL);
#endif /* SASM */
}

/**
*
* name		lkuplcl --- find string in local symbol table
*
* synopsis	sp = lkuplcl(name,rtyp)
*		struct nlist *sp;	pointer to symbol table entry
*					(NULL if not found)
*		char *name;		null terminated symbol name
*		int rtyp;		reference type
*
* description	Local labels are maintained in a linked list of structures.
*		This list is searched for the local label name.
*
**/
static struct nlist *
lkuplcl(name,rtyp)
char	*name;
int	rtyp;
{
#if !SASM
	struct nlist *lp, *srchsym ();

	++name; /* move past underscore */

	if( !Macro_exp ){
		if( Curr_lcl == NULL ){ /* no current local label list */
			Last_sym = NULL;
			if( Pass==2 )
				error2("Symbol undefined on pass 2",name);
			return(NULL);
			}
		Last_sym = Curr_lcl->sym;
		if ((lp = srchsym (name)) != NULL) {
			if( Pass == 2 )
				if (Cre_flag && !Chkdo) /* cross ref */
					xref_sym (lp, rtyp);
			return (lp);
			}
		}
	else{	/* macro expansion active */
		if( !Macro_exp->lcl ){ /* no macro exp. local label list */
			Last_sym = NULL;
			if( Pass==2 )
				error2("Symbol undefined on pass 2",name);
			return(NULL);
			}
		Last_sym = Macro_exp->lcl;
		if ((lp = srchsym (name)) != NULL)
			return (lp);
		}
	if( Pass==2 )
		error2("Symbol undefined on pass 2",name);
	return(NULL);
#endif
}

/**
*
* name		srchsym --- scan list looking for symbol
*
* synopsis	np = srchsym (name)
*		struct nlist *np;	pointer to symbol table entry
*					(NULL if not found)
*		char *name;		null terminated symbol name
*
* description	Scans the symbol list starting at Last_sym, matching symbol
*		identifiers against name.  When a match is found, verifies
*		that the symbol is in the appropriate section or if it is
*		global.	 If inside a section and the symbol is global,
*		check the XREF list.  If found, return.	 Otherwise if the
*		XREF flag is set, save the symbol and keep looking.  If
*		the list is exhausted and no symbol was validated, return
*		the global; else return NULL.
*
**/
struct nlist *
srchsym (name)
char *name;
{
#if !SASM
	struct nlist *np, *sp = NULL;
	int xref = xref_look (name);

	for( np = Last_sym ; np ; np = np->next){

		Last_sym = np;	/* save latest symbol */

		if( !STREQ(name,np->name) ) /* if name is not correct */
			continue;	/* keep looking */

		/* found a matching symbol; now check it out */
		if( xref ||		/* symbol explicitly XREFed */
		    Csect == np->sec )	/* symbol in current section */
			break;

		if( np->attrib & GLOBAL ){	/* found a global symbol */
			if( !sp &&		/* no other global so far */
			    (Csect == &Glbsec  || /* in global section */
			    np->sec == &Glbsec || /* defined in global */
			    Xrf_flag) )		/* recognizing all globals */
				sp = np;	/* save global symbol */
			continue;		/* keep looking */
			}
		}

	if( !np && sp )		/* saved a global symbol */
		np = sp;	/* reset pointer */

	return (np);
#endif
}

/**
*
* name		xref_sym - cross reference symbol
*
* synopsis	xref_sym (np, rtyp)
*		struct nlist *np;	pointer to symbol table entry
*		int rtyp;		reference type
*
* description	Make a cross-reference entry for the symbol table entry np.
*		This routine saves the reference type and line number of
*		the symbol in a linked list anchored to the symbol table
*		entry structure.
*
**/
xref_sym (np, rtyp)
struct nlist *np;
int rtyp;
{
#if !SASM && !PASM
	struct xlist *xs,*is,*os;

	xs = (struct xlist *)alloc(sizeof(struct xlist));
	if( !xs )
		fatal("Out of memory - cannot save cross-reference data");
	xs->lnum = Lst_lno; /* save source line # */
	xs->rtype = rtyp;  /* save reference type */
	xs->next = NULL;
	if( !np->xrf )
		np->xrf = xs;
	else{
		for( is = np->xrf ; is ; is = is->next )
			os = is;
		os->next = xs;
		}
#endif
}

/**
*
* name		instlxrf --- add a symbol to the external reference table
*
* synopsis	instlxrf (name)
*		char *name;		symbol name
*
* description	The symbol name is looked up in the external reference table.
*		If it is already there then nothing else is done and the
*		function returns.  Otherwise the symbol is hashed into the
*		table and put on a doubly-linked list for later reference.
*
**/
instlxrf (name)
char *name;
{
#if !SASM && !PASM
	struct symref *np;
	struct symref *lkupxrf ();

	if ((np = lkupxrf (name)) != NULL)	/* symbol already in table */
		return;

	/* install new symbol */
	if ( (np = (struct symref *)alloc(sizeof(struct symref))) == NULL ||
	     (np->name = (char *)alloc(strlen(name)+1)) == NULL )
		fatal("Out of memory - external reference table full");
	(void)strcpy(np->name,name);	/* save name */
	if (Icflag)			/* ignore case */
		(void)strdn (np->name);
	np->sec = Csect;		/* save section pointer */
	np->next = NULL;
	if (Last_xrf) {			/* if valid preceding entry */
		Last_xrf->next = np;	/* attach to end of symbol list */
		np->prev = Last_xrf;
	} else {
		Xrftab[hash(np->name)] = np;/* hash into symbol table */
		np->prev = NULL;
	}
	Xrf_cnt++;			/* increment count */
#endif
}

/**
*
* name		lkupxrf --- find external reference in symbol table
*
* synopsis	sp = lkupxrf (name)
*		struct symref *sp;	pointer to symbol table entry
*					(NULL if not found)
*		char *name;		null terminated symbol name
*
* description	Hash the symbol name into the symbol table.
*		If found, return pointer to symbol structure.
*		Otherwise, return NULL.
*
**/
struct symref *
lkupxrf (name)
char	*name;
{
#if !SASM && !PASM
	struct symref *np;
	char	symbuf[MAXSYM+1];

	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, name);
		(void)strdn (symbuf);
		name = symbuf;
		}
	Last_xrf = Xrftab[hash(name)];
	for( np = Last_xrf; np; np = np->next){

		Last_xrf = np;	/* save symbol pointer */

		if( STREQ(name,np->name) ) /* found it */
			break;
		}
	return( np );
#endif
}

/**
*
* name		mne_look --- mnemonic lookup
*
* synopsis	mp = mne_look(str)
*		struct mnemop *mp;	pointer to mnemonic table entry
*					NULL if not found.
*		char *str;		null terminated lower case mnemonic
*
* description	Returns pointer to mnemonic table entry if the desired mnemonic
*		is found. If not found, then NULL is returned.
*
**/
struct mnemop *
mne_look(str)
char	*str;
{
#if !PASM
	int mne_cmp ();

	return ((struct mnemop *)binsrch (str,
					 (char *)Mnemonic,
					 Nmne,
					 sizeof (struct mnemop),
					 mne_cmp));
#endif
}

static
mne_cmp (s, m)		/* compare string s with mnemonic in m */
char *s;
struct mnemop *m;
{
	int cmp;

	if ((cmp = strcmp (s, m->mnemonic)) != 0)
		return (cmp);
	return (m->class & BBITMASK ? 1 : 0);	/* for RDIRECT support */
}

/**
*
* name		psu_look --- pseudo op lookup
*
* synopsis	pp = psu_look(str)
*		struct pseuop *pp;	pointer to pseudo table entry.
*					NULL if not found.
*		char *str;		null terminated lower case pseudo op
*
* description	Returns pointer to the pseudo op table entry if found.
*		Returns NULL if not found. If the Movr_flag is set, then
*		the macro table is searched first. If the str is a macro,
*		then this routine will return a NULL.
*
**/
struct pseuop *
psu_look(str)
char	*str;
{
#if !SASM
	struct mac *mac_look();
	int psu_cmp ();

	if( mac_look(str) != NULL )
		return(NULL);

	return ((struct pseuop *)binsrch (str,
					(char *)Pseudo,
					Npse,
					sizeof (struct pseuop),
					psu_cmp));
#endif
}

static
psu_cmp (s, p)		/* compare string s with pseudo op in p */
char *s;
struct pseuop *p;
{
	int cmp;

	if ((cmp = strcmp (s, p->pmnemon)) != 0)
		return (cmp);
	return (p->ptype & BBITMASK ? 1 : 0);	/* for RDIRECT support */
}

/**
*
* name		opt_look --- option lookup
*
* synopsis	op = opt_look(str)
*		struct opt *op;		pointer to option table entry.
*					NULL if not found.
*		char *str;		null terminated lower case option
*
* description	Returns pointer to the option table entry if found.
*		Returns NULL if not found.
*
**/
struct opt *
opt_look(str)
char	*str;
{
#if !SASM && !PASM
	int opt_cmp ();

	return ((struct opt *)binsrch (str,
				       (char *)Option,
				       Nopt,
				       sizeof (struct opt),
				       opt_cmp));
#endif
}

static
opt_cmp (s, o)		/* compare string s with option in o */
char *s;
struct opt *o;
{
	return (strcmp (s, o->omnemon));
}

/**
*
* name		scs_look --- structured control statement lookup
*
* synopsis	s = opt_look(str)
*		struct scs *s;		pointer to scs table entry.
*					NULL if not found.
*		char *str;		null terminated lower case scs verb
*
* description	Returns pointer to the scs table entry if found.
*		Returns NULL if not found.
*
**/
struct scs *
scs_look(str)
char	*str;
{
#if !SASM && !PASM
	int scs_cmp ();

	return ((struct scs *)binsrch (++str,	/* skip leading period */
				       (char *)Scstab,
				       Nscs,
				       sizeof (struct scs),
				       scs_cmp));
#endif
}

static
scs_cmp (s, c)		/* compare string s with verb in c */
char *s;
struct scs *c;
{
	return (strcmp (s, c->scsname));
}

/**
*
* name		cond_look --- structured control statement conditional lookup
*
* synopsis	cnd = cond_look(str)
*		struct cond *cnd;	pointer to cond table entry.
*					NULL if not found.
*		char *str;		null terminated lower case scs cond
*
* description	Returns pointer to the scs cond table entry if found.
*		Returns NULL if not found.
*
**/
struct cond *
cond_look(str)
char	*str;
{
#if !SASM && !PASM
	int cond_cmp ();

	return ((struct cond *)binsrch (str,
				       (char *)Cndtab,
				       Ncnd,
				       sizeof (struct cond),
				       cond_cmp));
#endif
}

static
cond_cmp (s, c)		/* compare string s with cond in c */
char *s;
struct cond *c;
{
	return (strcmp (s, c->ctext));
}

/*
 *	fwdinit - initialize forward reference chain
 */
fwdinit()
{
	struct fwdref *fwd;

	if (Fhead.next != &Fhead) {
		Fwd = Fhead.next;
		while (Fwd != &Fhead) {
			fwd = Fwd;
			Fwd = fwd->next;
			free ((char *)fwd);
		}
	}
	Fhead.ref = 0L;
	Fhead.next = Fwd = &Fhead;
}

/*
 *	fwdreinit - reinitialize forward reference chain
 */
fwdreinit()
{
	Fwd = Fhead.next;
}

/*
 *	fwdmark - mark as containing forward reference
 */
fwdmark()
{
	struct fwdref *newfwd;

	if (!(newfwd = (struct fwdref *)alloc (sizeof (struct fwdref))))
		fatal ("Out of memory - cannot save forward reference data");
	newfwd->ref  = Lkp_count;
	newfwd->next = Fwd->next;
	Fwd->next = newfwd;
	Fwd = newfwd;
#if DEBUG
	(void)fprintf(stderr,"mark: ref = %ld\n", Fwd->ref);
#endif
}

/*
 *	fwdnext - get next forward reference in chain
 */
fwdnext()
{
	if (Fwd == &Fhead)
		return;
	Fwd = Fwd->next;
#if DEBUG
	(void)fprintf(stderr,"next: ref = %ld\n", Fwd->ref);
#endif
}

/*
*
* name		free_sym - release symbol table lists
*
* description	Releases all memory allocated to symbol table list.
*
**/
free_sym()
{
	struct nlist *np, *op;
	struct xlist *xp, *yp;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (np = Hashtab[i]) != NULL ) {
			Hashtab[i] = NULL;
			while(np){
				if (np->name)
					free (np->name);
				xp = np->xrf;
				while(xp){	/* free cross references */
					yp = xp->next;
					free ((char *)xp);
					xp = yp;
					}
				op = np->next;
				free((char *)np);
				np = op;
				}
			}
	Last_sym = NULL;
	Sym_cnt = 0;
}

/**
*
* name		free_lcl - release storage allocated to local labels
*
* description	Releases all dynamic storage allocated to local labels.
*
**/
free_lcl()
{
#if !SASM && !PASM
	struct symlist *np, *op;
	struct nlist *sl,*ns;
	struct explcl *el,*nl;

	/* free entries in standard local label list */
	np = Loclab;
	while (np) {
		sl = np->sym;
		while (sl) {
			free (sl->name);
			ns = sl->next;
			free ((char *)sl);
			sl = ns;
		}
		op = np->next;
		free ((char *)np);
		np = op;
	}
	Loclab = NULL;	/* tidy up, but it shouldn't be used again */
	Lsym_cnt = 0;

	/* free the macro expansion local label list */
	el = Exp_lcl;
	while(el){
		sl = el->list;
		while(sl){
			free (sl->name);
			ns = sl->next;
			free((char *)sl);
			sl = ns;
			}
		nl = el->lnext;
		free((char *)el);
		el = nl;
		}
	Exp_lcl = NULL; /* tidy up, but it shouldn't be used again */
#endif
}

/*
*
* name		free_xrf - release external reference table lists
*
* description	Releases all memory allocated to external reference table list.
*
**/
free_xrf()
{
#if !PASM
	struct symref *np, *op;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (np = Xrftab[i]) != NULL ) {
			Xrftab[i] = NULL;
			while(np){
				if (np->name)
					free (np->name);
				op = np->next;
				free((char *)np);
				np = op;
				}
			}
	Last_xrf = NULL;
	Xrf_cnt = 0;
#endif
}

/*
 *	free_symobj - free the SYMOBJ list
 */
free_symobj()
{
	struct symlist *np, *op;

	np = Sym_list;
	while (np) {
		op = np;
		np = op->next;
		free ((char *)op);
	}
}
