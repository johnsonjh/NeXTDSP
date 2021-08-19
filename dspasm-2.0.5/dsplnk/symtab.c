#include "lnkdef.h"
#include "lnkdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		instlsec --- add a section to the section hash table
*
* synopsis	yn = instlsec (sec)
*		int yn;			YES / NO for errors
*		struct slist *sec;	pointer to section structure
*
* description	Look up sec in the section table.  If it is found,
*		insure that the attributes match.  If the attributes
*		match, add in the memory counter values to the section
*		in the table.  If the attributes do not match, return
*		an error.
*
*		If the section is not found, allocate space for a
*		new section and copy in the section values.  If the
*		section numbers are not sequential or the section
*		number has already been used, return an error.
*		Otherwise, hash the section into the section table
*		and link it into the ordered section list.
*
**/
instlsec (sec)
struct slist *sec;
{
        struct slist *sp, *lkupsec ();
	register i, j;

        if ((sp = lkupsec (sec->sname)) != NULL) { /* section is in table */

		if (sp->flags != sec->flags) {	/* incompatible attributes */
			if (sp->flags & REL)
				error2 ("Section already defined as relocatable", sp->sname);
			else
				error2 ("Section already defined as absolute", sp->sname);
			return (NO);
		}

		Secmap[Secnt].sec = sp;		/* save section pointer */
		for (i = 0; i < MSPACES; i++) {	/* add in section lengths */
			for (j = 0; j < MCNTRS; j++) {
				Secmap[Secnt].offset[i][j] = sp->start[i][j] = sp->cntrs[i][j];
				sp->cntrs[i][j] += sec->cntrs[i][j];
			}
		}
		Secnt++;	/* increment section count */

		return (YES);
	}

	/* install new section */

	if (Secmap[sec->secno].sec) {	/* non-unique section number */
		error ("Invalid section number");
		return (NO);
	}
	if (Secnt != sec->secno) {	/* section synch error */
		error ("Section out of order");
		return (NO);
	}
	if (Secnt == 0 && !STREQ (sec->sname, Gsecname)) {
		error ("Invalid global section");
		return (NO);
	}
	if ( (sp = (struct slist *)alloc (sizeof (struct slist))) == NULL ||
             (sp->sname = (char *)alloc (strlen (sec->sname) + 1)) == NULL )
               	fatal("Out of memory - cannot save section data");
        (void)strcpy (sp->sname, sec->sname);	/* save section name */
	if (Icflag)				/* ignore case */
		(void)strdn (sp->sname);
	Secmap[Secnt++].sec = sp;		/* save section pointer */
	sp->secno = Sectot++;			/* save section number */
	sp->flags = sec->flags;			/* save attributes */
	for (i = 0; i < MSPACES; i++) {		/* save memory lengths */
		for (j = 0; j < MCNTRS; j++) {
			sp->cflags[i][j] = sec->cflags[i][j];
			sp->start[i][j]  = sec->start[i][j];
			sp->cntrs[i][j]  = sec->cntrs[i][j];
		}
	}
	sp->next = NULL;
	if (Last_sec)		/* if there is a valid preceding entry */
		Last_sec->next = sp;/* attach to end of section list */
	else			/* hash section into table */
		Sectab[hash(sec->sname)] = sp;
        return(YES);
}

/**
*
* name		lkupsec --- find section in section table
*
* synopsis	sp = lkupsec (sname)
*		struct slist *sp;	pointer to section table entry
*					(NULL if not found)
*		char *name;		section name
*
* description	Hash the section name into the section hash table.
*		If found, return pointer to section structure.
*		Otherwise, return NULL.
*
**/
struct slist *
lkupsec (sname)
char *sname;
{
        struct slist *np, *op = NULL;
	int hashval;		/* hash table offset */
	char secbuf[MAXSYM+1];	/* section name buffer */

	if( Icflag ){			/* ignore case */
		(void)strcpy (secbuf, sname);
		(void)strdn (secbuf);
		sname = secbuf;
		}
	hashval = hash (sname);		/* hash section name */
	Last_sec = Sectab[hashval];	/* save pointer to section */

        for (np = Last_sec; np; op = np, np = np->next) {
		Last_sec = np;		/* save latest section */
		
                if (!STREQ (sname, np->sname))
			continue; 	/* not found; keep looking */

		if (op) {	/* move entry to front of hash chain */
			op->next = np->next;	/* reconnect links */
			np->next = Sectab[hashval];
			Sectab[hashval] = np;	/* put in front */
		}
		return (np);		/* return section pointer */
	}
        return (NULL);		/* never found it */
}

/**
*
* name		instlsym --- add a symbol to the table
*
* synopsis	yn = instlsym (sym)
*		int yn;			YES / NO for errors
*		struct nlist *sym;	pointer to symbol structure
*
* description	The symbol name is looked up in the symbol table.
*		If the symbol is found and is global and the linker
*		is processing a library, the function returns.
*		Otherwise if the symbol is found a warning is issued.
*		If the symbol is not found the symbol fields are copied into
*		a new symbol structure and the symbol is hashed
*		into the symbol table.
*
**/
instlsym (sym)
struct nlist *sym;
{
        struct nlist *np;
        struct nlist *lkupsym ();
	struct symref *sp, *lkupxrf();

        if ((np = lkupsym (sym->name)) != NULL)	{ /* look for sym in table */
		if (sym->attrib & GLOBAL)	/* symbol we have is global */
			if (Cfile->flags & LIB)	/* processing a library */
				return (YES);	/* already there */
			else {
				warn2 ("Duplicate symbol", sym->name);
				return (YES);
			}
		if (sym->sec == np->sec) {	/* duplicate local symbol */
			error2 ("Duplicate local symbol", sym->name);
			return (YES);
		}
	}
        /* install new symbol */
	if ( (np = (struct nlist *)alloc(sizeof(struct nlist))) == NULL ||
	     (np->name = (char *)alloc(strlen(sym->name)+1)) == NULL )
                fatal("Out of memory - symbol table full");
        (void)strcpy(np->name,sym->name);	/* save name */
	if( Icflag )				/* ignore case */
		(void)strdn (np->name);
	if( sym->attrib & INT ){
        	np->def.xdef.ext  = sym->def.xdef.ext; /* save int value */
        	np->def.xdef.high = sym->def.xdef.high;
        	np->def.xdef.low  = sym->def.xdef.low;
		}
	else if( sym->attrib & FPT )
		np->def.fdef = sym->def.fdef; /* save fp value */
	np->attrib = sym->attrib;	/* save attributes */
	np->mapping = sym->mapping;	/* save mapping */
	np->sec = sym->sec;		/* save section pointer */
	np->next = NULL;
	Symcnt++;			/* increment symbol count */
	if (Last_sym)			/* if valid preceding entry */
		Last_sym->next = np;	/* attach to end of symbol list */
	else
		Symtab[hash(sym->name)] = np;/* hash into symbol table */
	if ((sp = lkupxrf (sym->name)) != NULL) /* lookup in ext refs */
		rmxrf (sp);		/* remove from table */
	return (YES);
}

/**
*
* name		lkupsym --- find name in symbol table
*
* synopsis	np = lkupsym (name)
*		struct nlist *np;	pointer to symbol table entry
*					(NULL if not found)
*		char *name;		null terminated symbol name
*
* description	Hash the symbol name into the symbol table.
*		If found, return pointer to symbol structure.
*		Otherwise, return NULL.
*
**/
struct nlist *
lkupsym (name)
char    *name;
{
        struct nlist *np, *sp = NULL;
	int	hashval;	/* hash table offset */
	char	symbuf[MAXSYM+1]; /* symbol name buffer */

	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, name);
		(void)strdn (symbuf);
		name = symbuf;
		}
	hashval = hash(name);	/* hash symbol name */
	Last_sym = Symtab[hashval];	/* save symbol */
        for( np = Last_sym ; np ; np = np->next){

		Last_sym = np;	/* save latest symbol */

                if( !STREQ(name,np->name) ) /* if name is not correct */
			continue; /* keep looking */

		if( Csect == np->sec )
			break;		/* found it */

		if( np->attrib & GLOBAL )
			sp = np;	/* save global */
		}

        if( !np && sp )         /* saved a global symbol */
                np = sp;        /* reset pointer */

	return (np);
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
		Xrftab[hash(name)] = np;/* hash into symbol table */
		np->prev = NULL;
	}
	Xrfcnt++;			/* increment count */
	Xrf_flag = YES;			/* set flag */
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
* description	Hash the symbol name into the external reference table.
*		If found, return pointer to symbol structure.
*		Otherwise, return NULL.
*
**/
struct symref *
lkupxrf (name)
char    *name;
{
        struct symref *np;
	int	hashval;	/* hash table offset */
	char	symbuf[MAXSYM+1]; /* symbol name buffer */

	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, name);
		(void)strdn (symbuf);
		name = symbuf;
		}
	hashval = hash(name);	/* hash symbol name */
	Last_xrf = Xrftab[hashval];
        for( np = Last_xrf; np; np = np->next){

		Last_xrf = np;	/* save symbol pointer */

                if( STREQ(name,np->name) ) /* found it */
			break;
		}
        return( np );
}

/**
*
* name		rmxrf --- remove external reference from symbol table
*
* synopsis	rmxrf (np)
*		struct symref *np;	pointer to symbol table entry
*
* description	Remove the external reference pointed to by np from
*		the external reference table.
*
**/
rmxrf (np)
struct symref *np;
{
	if (np->prev)
		np->prev->next = np->next;
	else
		Xrftab[hash (np->name)] = np->next;
	if (np->next)
		np->next->prev = np->prev;
	fake_free(np->name);			/* free memory */
	fake_free((char *)np);
	--Xrfcnt;				/* decrement count */
}

/**
*
* name		lstsec --- add a section to the library section list
*
* synopsis	yn = lstsec (sec)
*		int yn;			YES / NO for errors
*		struct slist *sec;	pointer to section structure
*
* description	Allocate space for a new section and copy in the
*		section values.  Link the section into the library
*		module section list.
*
**/
lstsec (sec)
struct slist *sec;
{
        struct slist *sp;
	register i, j;

	if ( (sp = (struct slist *)alloc (sizeof (struct slist))) == NULL ||
             (sp->sname = (char *)alloc (strlen (sec->sname) + 1)) == NULL )
               	fatal("Out of memory - cannot save library section");
        (void)strcpy (sp->sname, sec->sname);	/* save section name */
	sp->secno = sec->secno;			/* save section number */
	sp->flags = sec->flags;			/* save attributes */
	for (i = 0; i < MSPACES; i++)		/* clear memory lengths */
		for (j = 0; j < MCNTRS; j++) {
			sp->cflags[i][j] = 0;
			sp->start[i][j] = sp->cntrs[i][j] = 0L;
		}
	sp->next = NULL;
	if (Secptr)		/* if there is a valid preceding entry */
		Secptr = Secptr->next = sp;/* attach to end of section list */
	else			/* start section list */
		Secptr = Seclst = sp;
        return(YES);
}

/**
*
* name		free_seclst --- free library section list
*
* synopsis	yn = free_seclst ()
*		int yn;			YES / NO for errors
*
* description	Free space allocated to library module section list.
*
**/
free_seclst ()
{
        struct slist *sp, *op;

	sp = Seclst;
	while (sp) {
		fake_free(sp->sname);
		op = sp;
		sp = op->next;
		fake_free((char *)op);
	}
	Secptr = Seclst = NULL;
        return(YES);
}

/*
*
* name 		free_sec - release section table lists
*
* description	Releases all memory allocated to section table list.
*
**/
free_sec()
{
	struct slist *sp, *op;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (sp = Sectab[i]) != NULL ) {
			Sectab[i] = NULL;
			while(sp){
				if (sp->sname)
					fake_free(sp->sname);
				op = sp->next;
				fake_free((char *)sp);
				sp = op;
				}
			}
	Last_sec = NULL;
	Sectot = 0;
}

/*
*
* name 		free_sym - release symbol table lists
*
* description	Releases all memory allocated to symbol table list.
*
**/
free_sym()
{
	struct nlist *np, *op;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (np = Symtab[i]) != NULL ) {
			Symtab[i] = NULL;
			while(np){
				if (np->name)
					fake_free(np->name);
				op = np->next;
				fake_free((char *)np);
				np = op;
				}
			}
	Last_sym = NULL;
	Symcnt = 0;
}

/*
*
* name 		free_xrf - release external reference table lists
*
* description	Releases memory allocated to external reference table list.
*
**/
free_xrf()
{
	struct symref *np, *op;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (np = Xrftab[i]) != NULL ) {
			Xrftab[i] = NULL;
			while(np){
				if (np->name)
					fake_free(np->name);
				op = np->next;
				fake_free((char *)np);
				np = op;
				}
			}
	Last_xrf = NULL;
	Xrfcnt = 0;
}

/**
*
* name		hash --- form hash value for string s
*
* synopsis	hv = hash(s)
*		int hv;		hashed value
*		char *s;	string to be hashed
*
* description	Forms a hash value by summing the characters and
*		forming the remainder modulo the array size.
*
**/
hash(s)
char *s;
{
        int     hashval;

        for( hashval = 0; *s != EOS ; )
                hashval += *s++;
        return( hashval % HASHSIZE );
}

/**
*
* name		mem_look --- lookup memory file record
*
* synopsis	mp = mem_look(str)
*		struct rectype *mp;	pointer to memory record table entry
*					NULL if not found.
*		char *str;		null terminated lower case mnemonic
*
* description	Returns pointer to memory file table entry if the desired
*		record name is found. If not found, then NULL is returned.
*
**/
struct rectype *
mem_look(str)
char    *str;
{
	double *binsrch ();
	int rec_cmp ();

	return ((struct rectype *)binsrch (str,
					  (char *)Memtab,
					  Memtsize,
					  sizeof (struct rectype),
					  rec_cmp));
}

/**
*
* name		opt_look --- lookup map file option record
*
* synopsis	mp = opt_look(str)
*		struct rectype *op;	pointer to map option table entry
*					NULL if not found.
*		char *str;		null terminated lower case mnemonic
*
* description	Returns pointer to map option table entry if the desired
*		record name is found. If not found, then NULL is returned.
*
**/
struct rectype *
opt_look(str)
char    *str;
{
	double *binsrch ();
	int rec_cmp ();

	return ((struct rectype *)binsrch (str,
					  (char *)Opttab,
					  Opttsize,
					  sizeof (struct rectype),
					  rec_cmp));
}

static
rec_cmp (s, r)		/* compare string s with record name in r */
char *s;
struct rectype *r;
{
	return (strcmp (s, r->name));
}

