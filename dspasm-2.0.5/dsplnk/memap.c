#include "lnkdef.h"
#include "lnkdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		do_mem - process the memory relocation file
*
* description	Read the contents of the memory relocation file.
*		For each section record found, look up the section
*		in the section table.  Loop to read in specified
*		addresses.  Reset counter values, set appropriate
*		mapping and relocation flags.
*
**/
do_mem ()
{
	struct file memfile;
	struct rectype *rp, *mem_look ();
	struct slist *sec, *lkupsec ();
	char buf[MAXBUF];
	register char *p, *q;
/*
	The following initialization code is mainly for error handling
	(required for fatal(), error(), etc.).  We borrow the Mapfn buffer
	since it isn't required until the second pass.
*/
	if ((Csect = lkupsec (Gsecname)) == NULL)
		fatal("Cannot find GLOBAL section");
	Fd = Memfil;			/* set up current file pointer */
	Cfile = &memfile;		/* set up pointer to structure */
	Cfile->name = Mapfn;		/* set up pointer to file name */
	Cfile->flags = 0;
	Cfile->cmod = Cfile->modlst = NULL;
	Cfile->next = NULL;
	Lineno = 1;
	Recno = 0;

	while (get_field (MEMANY) != EOF) {	/* loop to read records */

		if (Field[0] == COMCHAR) {
			(void)get_field (MEMCOMMENT);	/* read in comment */
			continue;
		}
			
		Recno++;			/* increment record count */

		for (p = buf, q = Field; *q; p++, q++)
			*p = isupper (*q) ? tolower (*q) : *q;
		*p = EOS;
		if ((rp = mem_look (buf)) == NULL) {
			error2 ("Invalid relocation type field", Field);
			return (NO);
		}

		switch (rp->type) {
			case MEMBASE:
				if (get_field (MEMANY) != 0) {
					error ("Invalid address relocation field");
					return (NO);
				}
				if (!do_addr ((struct slist *)NULL))
					return (NO);
				break;
			case MEMMAP:
				if (!do_mapopt ())
					return (NO);
				break;
			case MEMSECTION:
				if (get_field (MEMANY) != 0) {
					error ("Invalid section name field");
					return (NO);
				}
				if ((sec = lkupsec (Field)) == NULL) {
					error2 ("Section not found", Field);
					return (NO);
				}
				if (get_field (MEMANY) != 0) {
					error ("Invalid address relocation field");
					return (NO);
				}
				if (!do_addr (sec))
					return (NO);
				break;
			case MEMSYMBOL:
				if (!do_symref ())
					return (NO);
				break;
			case MEMSTART:
				if (get_field (MEMANY) != 0) {
					error ("Invalid start address field");
					return (NO);
				}
				if (End_exp)	/*free prev. expression */
					fake_free(End_exp);
				if ((End_exp = (char *)alloc (strlen (Field) + 1)) == NULL)
					fatal("Out of memory - cannot save start address expression");
				(void)strcpy (End_exp, Field);	/* save for later */
				break;
			default:
				fatal ("Relocation type select failure");
		}
	}

	Cfile = NULL;		/* clear current file pointer */
	return (YES);
}

/**
*
* name		do_addr - process memory relocation addresses
*
* synopsis	do_addr (sec)
*		struct slist *sec;	pointer to section
*
* description	Assumes address tuples (memory space, counter, mapping,
*		and value) are contained in the input Field buffer.
*		Loops and scans each tuple, assigning mapping and address
*		values to appropriate section counter.
*
**/
static
do_addr (sec)
struct slist *sec;
{
	int spc, ctr, map;
	long addr;
	char c;
	struct evres *ev, *eval_ad ();

	Optr = Field;			/* set up global pointer */

	while (*Optr) {

		if ((spc = get_space (*Optr++)) == ERR || spc == NONE) {
			error ("Illegal memory space specified");
			return (NO);
		}

		if ((ctr = get_cntr (*Optr)) != ERR)	/* look for counter */
			Optr++;
		else
			ctr = DEFAULT;

		map = NONE;
		switch (c = mapdn (*Optr)) {		/* look for mapping */
			case 'i':
			case 'e':
			case 'b':
				Optr++;
				if (*Optr++ != ':') {
					error ("Syntax error in address field");
					return (NO);
				}
				if ((map = char_map (c)) == ERR) {
					error ("Illegal memory map character");
					return (NO);
				}
				if (map == BMAP && spc != PMEM) {
					error ("Bootstrap mapping available only in P memory");
					return (NO);
				}
				break;
			case ':':
				Optr++;
			/* fall through */
			case EOS:
				break;
			default:
				error ("Syntax error in address field");
				return (NO);
		}

		if ((ev = eval_ad ()) == NULL)	/* evaluate address */
			return (NO);
		addr = ev->uval.xvalue.low;
		free_exp (ev);

		while (*Optr && *Optr != ',')
			Optr++;			/* find comma */
		if (*Optr == ',')		/* skip comma */
			Optr++;

		if (!sec) {		/* use base flags and counters */
			/* set only if not already done on command line */
			if (!(Mflags[spc-1][ctr] & SETCTR)) {
				Mflags[spc-1][ctr] |= map | SETCTR;
				Start[spc-1][ctr] = addr;
			}
		} else {		/* use section counters */
			sec->cflags[spc-1][ctr] |= map | SETCTR;
			sec->start[spc-1][ctr] = 
				sec->cntrs[spc-1][ctr] = addr;
		}
	}
	return (YES);
}

/**
*
* name		do_symref - process unresolved symbol declarations
*
* synopsis	yn = do_symref ()
*		int yn;		YES/NO depending on errors
*
* description	Reads a SYMBOL type record from the memory file.  Sets
*		up a symbol table entry and installs the new symbol.  If
*		the symbol already exists an error is issued.
*
**/
static
do_symref ()
{
	struct nlist buf, *lkupsym ();
	struct evres *ev, *eval ();
	int spc = NONE, ctr = DEFAULT, map = NONE;
	register char *p;
	char c, name[MAXBUF];

	if (get_field (MEMANY) != 0) {
		error ("Invalid symbol name field");
		return (NO);
	}
	if (lkupsym (Field) != NULL) {
		error2 ("Symbol already defined", Field);
		return (NO);
	}
	(void)strcpy (name, Field);		/* save symbol name */
	buf.name = name;

	if (get_field (MEMANY) != 0) {
		error ("Invalid symbol value field");
		return (NO);
	}

	Optr = Field;

	for (p = Optr; *p && p <= Optr + 3 && *p != ':'; p++)
		;		/* scan for memory space */

	if (*p == ':') {	/* found memory space */

		if ((spc = get_space (*Optr++)) == ERR) {
			error ("Illegal memory space specified");
			return (NO);
		}

		if ((ctr = get_cntr (*Optr)) != ERR)	/* look for counter */
			Optr++;
		else
			ctr = DEFAULT;

		map = NONE;
		switch (c = mapdn (*Optr)) {		/* look for mapping */
			case 'i':
			case 'e':
			case 'b':
				Optr++;
				if (*Optr++ != ':') {
					error ("Syntax error in address field");
					return (NO);
				}
				if ((map = char_map (c)) == ERR) {
					error ("Illegal memory map character");
					return (NO);
				}
				if (map == BMAP && spc != PMEM) {
					error ("Bootstrap mapping available only in P memory");
					return (NO);
				}
				break;
			case ':':
				Optr++;
			/* fall through */
			case EOS:
				break;
			default:
				error ("Syntax error in address field");
				return (NO);
		}
	}

	if ((ev = eval ()) == NULL)
		return (NO);

	buf.attrib = spc | GLOBAL | (ev->etype & TYPEMASK);
	if (buf.attrib & FPT)
		buf.def.fdef = ev->uval.fvalue;
	else {
		buf.def.xdef.ext  = ev->uval.xvalue.ext;
		buf.def.xdef.high = ev->uval.xvalue.high;
		buf.def.xdef.low  = ev->uval.xvalue.low;
	}
	if (ev->size == SIZEL)
		buf.attrib |= LONG;		/* set long attribute */
	free_exp (ev);
	buf.mapping = ctr | map;		/* set mapping attributes */
	buf.sec = Csect;			/* set global section */
	buf.next = NULL;

	(void)instlsym (&buf);			/* install symbol */
	return (YES);
}

/**
*
* name		do_mapopt - process map listing file records
*
* synopsis	yn = do_mapopt ()
*		int yn;		YES/NO depending on errors
*
* description	Reads a MAP type record from the memory file.
*		Interprets format and control options for linker
*		map output file.
*
**/
static
do_mapopt ()
{
	if (get_field (MEMANY) != 0) {
		error ("Invalid MAP record field");
		return (NO);
	}

	(void)strdn (Field);
	if (STREQ (Field, "page")) {
		if (!do_page ())
			return (NO);
	} else if (STREQ (Field, "opt")) {
		if (!do_opt ())
			return (NO);
	} else {
		error ("Invalid MAP record field");
		return (NO);
	}

	return (YES);
}


/**
*
* name 		do_page - input map file format
*
* synopsis	yn = do_page()
*		int yn;		YES / NO if error
*
* description 	Input the map file format and check for consistency.
*
**/
static
do_page()
{
	int p_width;	/* page width */
	int p_length;	/* page length */
	int blank_top;	/* blank lines at top of page */
	int blank_bot;	/* blank lines at bottom of page */
	int blank_left;	/* blank columns at left of page */
	long eval_positive();

	if (get_field (MEMANY) != 0) {
		error ("Invalid MAP page field");
		return (NO);
	}
	Optr = Field;
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
	struct evres *er, *eval_int();
	long val;

	if( (er = eval_int()) == NULL )
		return(ERR);

	if( (val = er->uval.xvalue.low) < 0L ){
		error("Expression cannot have a negative value");
		val = ERR;
		}

	free_exp(er);
	return(val);
}


/**
*
* name 		do_opt - set map file output options
*
* synopsis	yn = do_opt()
*		int yn;		YES / NO if error
*
* description 	Set options for display of map file sections.
*
**/
static
do_opt()
{
	struct rectype *rp, *opt_look ();
	char buf[MAXBUF];
	register char *p, *q;

	if (get_field (MEMANY) != 0) {
		error ("Invalid MAP option field");
		return (NO);
	}

	q = Field;
	while (*q) {
		for (p = buf; *q && *q != ','; p++, q++)
			*p = isupper (*q) ? tolower (*q) : *q;
		*p = EOS;
		if (*q)
			q++;	/* skip comma */
		if ((rp = opt_look (buf)) == NULL) {
			error2 ("Invalid MAP option", buf);
			return (NO);
		}

		switch (rp->type) {
			case MAPNOCON:
				Con_flag = NO;
				break;
			case MAPNOLOC:
				Loc_flag = NO;
				break;
			case MAPNOSEA:
				Sea_flag = NO;
				break;
			case MAPNOSEN:
				Sen_flag = NO;
				break;
			case MAPNOSYN:
				Syn_flag = NO;
				break;
			case MAPNOSYV:
				Syv_flag = NO;
				break;
			default:
				fatal ("Map option select failure");
		}
	}

	return (YES);
}

