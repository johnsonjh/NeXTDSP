#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* Define directive utilities
*
**/

/**
*
* name		def_lkup
*
* synopsis	dp = def_lkup(name)
*		struct deflist *dp	definition structure if found,
*					NULL if not found.
*		char *name;		null terminated symbol name
*
* description	Looks up name in define table. Returns pointer to define
		structure if found, otherwise returns NULL.
*
**/
struct deflist *
def_lkup(name)
char	*name;
{
	struct deflist *np;
	char symbuf[MAXSYM + 1]; /* symbol name buffer */

	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, name);
		(void)strdn (symbuf);
		name = symbuf;
		}
	for( np = Dhashtab[hash(name)] ; np ; np = np->dnext)
		if( STREQ(name,np->dname) &&
		    (np->dsect == &Glbsec || np->dsect == Csect) )
			break;
	return(np);
}

/**
*
* name		def_inst
*
* synopsis	yn = def_inst(sym,def)
*		int yn;		YES / NO if error
*		char *sym;	null terminated symbol name
*		char *def;	null terminated definition string
*		int cmd;	YES/NO DEFINE is on command line
*
* description	Adds a definition symbol to the table. Error will
*		result if symbol has been previously defined, or if
*		pass 2 and symbol not previously defined.
*
**/
def_inst(sym,def,cmd)
char *sym;	/* symbol name */
char *def;	/* definition string */
int cmd;	/* command line flag */
{
	register struct deflist *np;
	int i;

	if( (np = def_lkup(sym)) != NULL ){
		error2("Re-definition of symbol not allowed",sym);
		return(NO);
		}

	/* enter new symbol */
#if DEBUG
	printf("Define installing %s as -%s-\n",sym,def);
#endif
	if (!(np = (struct deflist *)alloc(sizeof(struct deflist))))
		fatal("Out of memory - define table full");
	if (!(np->dname = (char *)alloc(strlen(sym)+1)))
		fatal("Out of memory - define table full");
	(void)strcpy(np->dname,sym);
	if (Icflag)		/* ignore case */
		(void)strdn (np->dname);
	if (!(np->ddef = (char *)alloc(strlen(def)+1)))
		fatal("Out of memory - define table full");
	(void)strcpy(np->ddef,def);
	np->deflags = cmd;
	np->dsect = Csect;
	i = hash(np->dname);
	np->dnext = Dhashtab[i];
	Dhashtab[i] = np;
	++No_defs; /* count defines */
	return(YES);
}

/**
*
* name		def_rem
*
* synopsis	yn = def_rem(sym)
*		int yn;		YES / NO if error
*		char *sym;	null terminated symbol name
*
* description	Removes a definition symbol from the table. Error will
*		result if symbol has not been previously defined.
*
**/
def_rem(sym)
char *sym;	/* symbol name */
{
	struct deflist *np, *pp;
	int i;
	char symbuf[MAXSYM + 1]; /* symbol name buffer */

	if( Icflag ){		/* ignore case */
		(void)strcpy (symbuf, sym);
		(void)strdn (symbuf);
		sym = symbuf;
		}
	i = hash (sym);
	for( pp = NULL, np = Dhashtab[i];
	     np && !STREQ(sym,np->dname);
	     pp = np, np = np->dnext )
		;
	if( !np ){
		error2("Symbol not previously defined",sym);
		return(NO);
		}

	/* remove symbol */
#if DEBUG
	printf("Define removing %s\n",sym);
#endif
	if (np->dname)
		free (np->dname);
	if (np->ddef)
		free (np->ddef);
	if (!pp)
		Dhashtab[i] = np->dnext;
	else
		pp->dnext = np->dnext;
	free((char *)np);

	--No_defs; /* decrement count of defines */
	return(YES);
}

/*
*
* name		free_def - release define list
*
* description	Releases all memory allocated to define list.
*
**/
free_def()
{
	struct deflist *np, *op;
	int i;

	for ( i = 0 ; i < HASHSIZE ; ++i )
		if( (np = Dhashtab[i]) != NULL ) {
			Dhashtab[i] = NULL;
			while(np){
				/* don't free cmd. line DEFINES if compiler */
				if (Cflag && np->deflags) {
					Dhashtab[i] = np;
					break;
				}
				if (np->dname)
					free (np->dname);
				if (np->ddef)
					free (np->ddef);
				op = np->dnext;
				free((char *)np);
				np = op;
				}
			}
}
