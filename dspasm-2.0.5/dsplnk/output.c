#include "lnkdef.h"
#include "lnkdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		fatal --- fatal error handler
*
* synopsis	fatal(str)
*		char *str;	null terminated error message
*
* description	The error message is output and an abortive program
*		exit is generated with a returned non-null error code.
*
**/
fatal(str)
char    *str;
{
	if (Cfile) {
		(void)fprintf (Outfil, "**** %d [%s (Record %d)]: FATAL --- %s\n", Lineno, Cfile->name, Recno, str);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n",
			Cfile->name, Lineno);
#endif
	} else
		(void)fprintf (Outfil, "FATAL --- %s\n", str);
	if (!Err_count)
		exit (INITERR);
	else {
#if !VMS
	        exit (Err_count);
#else
		Vmserr.fld.success  = (unsigned)NO;
		Vmserr.fld.severity = VMS_ERROR;
		Vmserr.fld.message  = (unsigned)Err_count;
		exit (Vmserr.code);
#endif
	}
}

/**
*
* name		error --- error in a line
*
* synopsis	error(str)
*		char *str;	null terminated error message
*
* description	The error message, line, record, and field number
*		of the error, and the filename is output.
*
**/
error(str)
char    *str;
{
	if( Gagerrs ){ /* if errors are inhibited */
		++Gagcnt;
		return;
		}

	if (Cfile) {
		(void)fprintf (Outfil, "**** %d [%s (Record %d)]: ERROR --- %s\n", Lineno, Cfile->name, Recno, str);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n",
			Cfile->name, Lineno);
#endif
	 } else
		(void)fprintf (Outfil, "**** ERROR --- %s\n", str);
        Err_count++;
}

/**
*
* name		error2 --- error in a line with parameter
*
* synopsis	error2(str,param)
*		char *str;	null terminated error message
*		char *param;	null terminated parameter
*
* description	The error message, line, field, and record number
*		of the error, and the filename are all output.
*		Param is output following str, separated by
*		a colon.
*
**/
error2(str,param)
char    *str,*param;
{
	if( Gagerrs ){ /* if errors are inhibited */
		++Gagcnt;
		return;
		}

	if (Cfile) {
		(void)fprintf (Outfil, "**** %d [%s (Record %d)]: ERROR --- %s: %s\n", Lineno, Cfile->name, Recno, str, param);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n",
			Cfile->name, Lineno);
#endif
	} else
		(void)fprintf (Outfil, "**** ERROR --- %s: %s\n", str, param);
        Err_count++;
}

/**
*
* name		warn --- trivial error in a line
*
* synopsis	warn(str)
*		char *str;	null terminated warning message
*
* description	The warning message, line number of the warning, and,
*		if the number of files processed is greater than one,
*		the filename are all output. Warnings are ignored during
*		pass 1. 
*
**/
warn(str)
char    *str;
{
	if( Gagerrs ) /* if errors are gagged */
		return;

	if (Cfile) {
		(void)fprintf (Outfil, "**** %d [%s (Record %d)]: WARNING --- %s\n", Lineno, Cfile->name, Recno, str);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n",
			Cfile->name, Lineno);
#endif
	} else
		(void)fprintf (Outfil, "**** WARNING --- %s\n", str);
	++Warn_count;
}

/**
*
* name		warn2 --- trivial error in a line
*
* synopsis	warn(str,param)
*		char *str;	null terminated warning message
*		char *param;	null terminated parameter
*
* description	The warning message, line number of the warning, and,
*		if the number of files processed is greater than one,
*		the filename are all output. Warnings are ignored during
*		pass 1.  Param is output following str, separated by
*		a colon.
*
**/
warn2(str,param)
char    *str,*param;
{
	if( Gagerrs ) /* if errors are gagged */
		return;

	if (Cfile) {
		(void)fprintf (Outfil, "**** %d [%s (Record %d)]: WARNING --- %s: %s\n", Lineno, Cfile->name, Recno, str, param);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n",
			Cfile->name, Lineno);
#endif
	} else
		(void)fprintf (Outfil, "**** WARNING --- %s: %s\n", str, param);
	++Warn_count;
}

/**
*
* name		cmderr --- process command line error
*
* synopsis	cmderr(str)
*		char *str;	null terminated informative message
*
* description	The message is sent to the standard error output and
*		the program exits.
*
**/
cmderr(str)
char    *str;
{
	(void)printf ("%s: %s\n", Progname, str);
#if LSC
	longjmp (Env, ERR);
#else
	exit (INITERR);
#endif
}

/**
*
* name		cmderr2 --- process command line error with parameter
*
* synopsis	cmderr2(str,param)
*		char *str;	null terminated informative message
*		char *param;	null terminated parameter
*
* description	The message is sent to the standard error output, followed
*		by a colon, space, and the string parameter, after which
*		the program exits.
*
**/
cmderr2(str,param)
char    *str;
char	*param;
{
	(void)printf ("%s: %s: %s\n", Progname, str, param);
#if LSC
	longjmp (Env, ERR);
#else
	exit (INITERR);
#endif
}

/**
*
* name 		outlst - output string to map file
*
* synopsis	outlst(str)
*		char *str;	string to be output
*
* description	Outputs the string to the map file. A column count is
*		maintained and if the column count exceeds the printer
*		width, then a newline is output. A line count is maintained,
*		and if the linecount exceeds the maximum printer page length,
*		and if the Formatp flag is set, then the map file is moved to
*		the next page and a header is printed.
*
**/
static
outlst(str)
char *str;
{
   	static int first = YES;
	int j;

	if (first ) {
		first = NO;
		j = Lst_char;
		top_page ();
		Lst_char = j;
	}

	while( *str != EOS ){
		if( Lst_col++ > Max_col && *str != NEWLINE ){
			j = Lst_char;	/* remember last character position */
			lst_nl();	/* output newline to map file */
			Lst_char = j;
			}
		++Lst_char;
		if( *str == NEWLINE )
			lst_nl();
		else
			(void)fputc(*str,Mapfil);
		++str;
		}
}

/**
*
* name 		outcol - output string; check for wrap
*
* synopsis	outcol(str)
*		char *str;	string to be output
*
* description	Outputs the string to the map file.  If the length of the
*		string would cause a wrap, outputs a new line before printing
*		the string.  If the string would cause a wrap in any case
*		(e.g., even from the left margin), it is output directly.
*
**/
static
outcol(str)
char *str;
{
	int len;		/* length of input string */
	int j;

	len = strlen (str);	/* get length of input string */
	if (len <= Max_col - Start_col + 1 && len + Lst_col > Max_col) {		/* output new line */
		j = Lst_char;	/* remember last character position */
		lst_nl();	/* output newline to map file */
		Lst_char = j;	/* restore character position */
	}
	outlst (str);		/* output string */
}

/**
*
* name 		outdot - output periods to map file
*
* synopsis	outdot(col)
*		int col;	stop column
*
* description	Outputs periods up to the column specified by col.
*
**/
static
outdot(col)
int col;
{
	while( Lst_char < col )
		outlst(".");
}

/**
*
* name 		outspace - output spaces to map file
*
* synopsis	outspace(col)
*		int col;	stop column
*
* description	Outputs spaces up to the column specified by col.
*		If current column is greater than or equal to
*		col, outputs one space.
*
**/
static
outspace(col)
int col;
{
	if( Lst_char >= col )
		outlst(" ");
	else
		while( Lst_char < col )
			outlst(" ");
}

/**
*
* name		lst_nl - output newline to map file
*
* description	If map file line count exceeds map file page length, then
*		the file is moved to the next page. Otherwise, a newline is
*		output to the map file and the line count is incremented.
*
**/
static
lst_nl()
{
	if( Lst_line >= Max_line )
		page_eject(YES);
	else{
		(void)fputc(NEWLINE,Mapfil);
		++Lst_line;
		Lst_col = 1;
		Lst_char = 1; /* reset map file character */
		}
	left_margin();
}

/**
* 
* name		left_margin - adjust left map file margin
*
* description	Outputs required number of blanks to adjust left map file
*		margin.
*
**/
static
left_margin()
{
	while( Lst_col < Start_col ){
		(void)fputc(' ',Mapfil);
		++Lst_col;
		}
}

/**
*
* name		page_eject - move to next output map file page
*
* synopsis	page_eject (newpage)
*		int newpage;		start new page flag
*
* description	The output map file page is ejected by outputting newlines
*		to move to the next map file page. If the Format_flag is
*		set, then a header will be output at the top of the next
*		page.  If the newpage parameter is YES, counters are reset,
*		the page number is incremented, and the top-of-page routine
*		is called.
*
**/
static
page_eject(newpage)
int newpage;
{
	for(; Lst_line <= Lst_p_len ; ++Lst_line )
		(void)fputc(NEWLINE,Mapfil);

	if (newpage) {
		Lst_col = 1;	/* reset column number */
		Lst_char = 1;	/* reset character position */
		Lst_line = 1;	/* reset line number */
		++Page;		/* increment page # */
		top_page();	/* do top of page */
	}
}

/**
*
* name 		top_page - format top of map file
*
* description	Any blank lines at the top of the page are output and,
*		if the Formflag is set, a map file header is output.
*
**/
static
top_page()
{
	for( Lst_line = 1; Lst_line <= Lst_topm; ++Lst_line)
		(void)fputc(NEWLINE,Mapfil);

	if( Formflag != 0 ){
		header();
		}

}		 

/**
*
* name		header --- print page header
*
**/
static
header()
{
	char pgstr[16], *basename();

	left_margin();	/* adjust left margin for first line of page */
	outlst(Progtitle);
	outlst("  ");
	outcol(Testing ? "Version XXXX" : Version);
	outlst("  ");
	outcol(Testing ? "00/00/00" : Curdate);
	outlst("  ");
	outcol(Testing ? "00:00:00" : Curtime);
	outlst("  ");
	outcol(Testing ? basename(Mapfn) : Mapfn);
	(void)sprintf(pgstr,"  Page %d\n",Page);
	outcol(pgstr);
	lst_nl();
	lst_nl();
}

/**
*
* name 		do_map --- generate link map file
*
* description	Generates a link map of sections, symbols, and unresolved
*		externals.
*
**/
do_map ()
{
	if (Sectot == 0) {		/* no sections */
		outlst ("No sections found");
		lst_nl(); lst_nl(); lst_nl();
	} else {
		print_secaddr ();	/* print sections by address */
		print_secname ();	/* print sections by name */
		if (Sec_sort) {
			fake_free((char *)Sec_sort);/* free section sort table */
			Sec_sort = NULL;
		}
	}
	page_eject (YES);
	if (Symcnt == 0) {		/* no symbols */
		outlst ("No symbols found");
		lst_nl(); lst_nl(); lst_nl();
	} else {
		print_symname ();	/* print symbols by name */
		print_symval ();	/* print symbols by value */
		if (Sym_sort) {
			fake_free((char *)Sym_sort);/* free symbol sort table */
			Sym_sort = NULL;
		}
	}
	print_xref ();		/* print unresolved external references */
}

/**
*
* name 		print_secaddr - print sections by address
*
* description	Prints a list of the sections defined in the link file
*		sorted by memory space and address.
*
**/
static
print_secaddr()
{
	struct slist *mp;
	register i, j, k;
	int nosec;
	long start, len, end, lastend;
	struct slist **sort_tab;
	char *strup ();

	if( Sectot == 0 )
		return; 	/* no sections */

	if (!Sea_flag)
		return;		/* listing not wanted */

	if (Sec_sort)		/* already have table */
		sort_tab = Sec_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Sec_sort = (struct slist **)alloc((Sectot+1) * sizeof(struct slist *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort section names");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( mp = Sectab[j]; mp; mp = mp->next )
				sort_tab[i++] = mp;
		sort_tab[i] = NULL;	/* seal off list */
	}

	outlst("                        Section Link Map by Address");
	lst_nl();

	for (i = 0; i < MSPACES; i++) {

		for (j = 0; j < MCNTRS; j++) {

			sortasec(sort_tab,Sectot,i,j);	/* sort sections */

			nosec = YES;
			end = -1L;
			lst_nl();
			lst_nl();
	                switch (i+1) {
				case XMEM:    outlst ("X"); break;
	        	        case YMEM:    outlst ("Y"); break;
				case LMEM:    outlst ("L"); break;
		                case PMEM:    outlst ("P"); break;
			}
			outlst (" Memory ");
	                switch (j) {
				case DEFAULT: outlst ("(default)"); break;
	        	        case LCNTR:   outlst ("(low)"); break;
				case HCNTR:   outlst ("(high)"); break;
			}
			lst_nl();
			lst_nl();
                	outlst("Start    End     Length    Section");
			lst_nl();

			for (k = 0; k < Sectot; k++) {
				mp = sort_tab[k];
				start = mp->start[i][j];
				len = mp->cntrs[i][j] - start;
				if (len == 0)
					continue;
				lastend = end;
				end = (start + len) - 1L;
				nosec = NO;
				(void)sprintf (String, "%04lx     %04lx    %5ld     ",
					start, mp->cntrs[i][j] - 1L, len);
		                outlst (strup(String));
				String[0] = EOS;
				(void)strncat (String, mp->sname, LSTSYM);
				outlst (String);
				if (!(mp->flags & REL)) {
					outspace (30 + LSTSYM);
					outlst ("Abs");
				}
				if (start <= lastend) {
					outspace (30 + LSTSYM);
					outlst ("*Overlap*");
				}
				lst_nl();
			}
			if (nosec) {
				lst_nl();
				outlst ("  No sections");
				lst_nl();
			}
		}
	}
	lst_nl();
	lst_nl();
	lst_nl();
}

/**
*
* name 		sortasec - sort sections by address
*
**/
static
sortasec(array,no_secs,mem,ctr)
struct slist **array;
int no_secs,mem,ctr;
{
	int cmp_asec ();

	Sort_tab = (char **)array;
	Sortmem = mem;
	Sortctr = ctr;
	Cmp_rtn = cmp_asec;
	psort(0, no_secs - 1);
}

/**
*
* name 		cmp_asec - compare sections by address
*
**/
static
cmp_asec (s1, s2)
struct slist *s1, *s2;
{
	if (s1->start[Sortmem][Sortctr] < s2->start[Sortmem][Sortctr])
		return (-1);
	else if (s1->start[Sortmem][Sortctr] > s2->start[Sortmem][Sortctr])
		return (1);
	return (strcmp (s1->sname, s2->sname));
}

/**
*
* name 		print_secname - print sections by name
*
* description	Prints a list of the sections defined in the link file
*		sorted by name.
*
**/
static
print_secname()
{
	struct slist *mp;
	register i, j, k;
	int nomem;
	long start, len, end;
	struct slist **sort_tab;
	char *strup ();

	if( Sectot == 0 )
		return; 	/* no sections */

	if (!Sen_flag)
		return;		/* listing not wanted */

	if (Sec_sort)		/* already have table */
		sort_tab = Sec_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Sec_sort = (struct slist **)alloc(Sectot * sizeof(struct slist *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort section names");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( mp = Sectab[j]; mp; mp = mp->next )
				sort_tab[i++] = mp;
		sort_tab[i] = NULL;	/* seal off list */
	}

	sortnsec(sort_tab,Sectot);	/* sort the sections */

	outlst("                         Section Link Map by Name");
	lst_nl();
	lst_nl();
	lst_nl();
	outlst("Section                Memory       Start    End     Length");
	lst_nl();

	for (i = 0; i < Sectot; i++) {
		if (!(sort_tab[i]->flags & REL)){	/* absolute section */
			i = print_absec (&sort_tab[i], i);
			continue;
		}
		mp = sort_tab[i];
		String[0] = EOS;
		(void)strncat (String, mp->sname, LSTSYM);
		outlst(String);
		nomem = YES;
		for (j = 0; j < MSPACES; j++) {
			for (k = 0; k < MCNTRS; k++) {
				start = mp->start[j][k];
				len = mp->cntrs[j][k] - start;
				if (!len)
					continue;
				end = (start + len) - 1L;
				nomem = NO;
				outspace (LSTSYM + 8);
		                switch (j+1) {
					case XMEM:	outlst ("X"); break;
	        		        case YMEM:	outlst ("Y"); break;
					case LMEM:	outlst ("L"); break;
		                	case PMEM:	outlst ("P"); break;
				}
		                switch (k) {
					case DEFAULT:	outlst (" default");
							break;
	        		        case LCNTR:	outlst (" low");
							break;
					case HCNTR:	outlst (" high");
							break;
				}
				outspace (LSTSYM + 21);
				(void)sprintf (String, "%04lx     %04lx    %5ld",
					start, end, len);
		                outlst (strup(String));
				lst_nl();
			}
		}
		if (nomem) {		/* no memory allocation */
			outspace (LSTSYM + 8);
			outlst ("None");
			lst_nl();
		}
	}
	lst_nl();
	lst_nl();
	lst_nl();
}

/**
*
* name 		print_absec - print absolute section by name
*
* description	Prints absolute section info sorted by memory address. 
*
**/
static
print_absec (tab, offset)
struct slist **tab;
int offset;
{
	char *sname;
	int cnt, nomem, i, j, k;
	long start, len, end;
	struct slist *sp;
	char *strup ();

	String[0] = EOS;
	(void)strncat (String, tab[0]->sname, LSTSYM);
	outlst(String);
	outspace (LSTSYM + 2);
	outlst ("Abs");
	nomem = YES;

	for (cnt = 1, sname = tab[0]->sname;
	     tab[cnt] && STREQ (sname, tab[cnt]->sname);
	     cnt++)
		;			/* count absolute section entries */

	for (i = 0; i < MSPACES; i++) {
		for (j = 0; j < MCNTRS; j++) {
			sortasec (tab, cnt, i, j);	/* sort sections */
			for (k = 0; k < cnt; k++) {
				sp = tab[k];
				start = sp->start[i][j];
				len = sp->cntrs[i][j] - start;
				if (!len)
					continue;
				end = (start + len) - 1L;
				nomem = NO;
				outspace (LSTSYM + 8);
		                switch (i+1) {
					case XMEM:	outlst ("X"); break;
	        		        case YMEM:	outlst ("Y"); break;
					case LMEM:	outlst ("L"); break;
		                	case PMEM:	outlst ("P"); break;
				}
		                switch (j) {
					case DEFAULT:	outlst (" default");
							break;
	        		        case LCNTR:	outlst (" low");
							break;
					case HCNTR:	outlst (" high");
							break;
				}
				outspace (LSTSYM + 21);
				(void)sprintf (String, "%04lx     %04lx    %5ld",
					start, end, len);
		                outlst (strup(String));
				lst_nl();
			}
		}
	}
	if (nomem) {		/* no memory allocation */
		outspace (LSTSYM + 8);
		outlst ("None");
		lst_nl();
	}
	return (offset + (cnt - 1));
}

/**
*
* name 		sortnsec - sort sections by name
*
**/
static
sortnsec(array,no_secs)
struct slist **array;
int no_secs;
{
	int cmp_nsec ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_nsec;
	psort(0, no_secs - 1);
}

/**
*
* name 		cmp_nsec - compare sections by name
*
**/
static
cmp_nsec (s1, s2)
struct slist *s1, *s2;
{
	return (strcmp (s1->sname, s2->sname));
}

/**
*
* name 		sortsecs - sort sections by section number
*
**/
sortsecs(array,no_secs)
struct slist **array;
int no_secs;
{
	int cmp_secs ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_secs;
	psort(0, no_secs - 1);
}

/**
*
* name 		cmp_secs - compare sections by section number
*
**/
static
cmp_secs (s1, s2)
struct slist *s1, *s2;
{
	return (s1->secno - s2->secno);
}

/**
*
* name 		print_symname - print symbol table listing by name
*
* description	Prints a list of the symbols defined in the link file
*		sorted by name.
*
**/
static
print_symname()
{
	struct nlist *mp;
	int i,j,satt;
	struct nlist **sort_tab;
	char *strup();

	if( Symcnt == 0 )		/* no symbols */
		return;

	if (!Syn_flag)
		return;			/* listing not wanted */

	if (Sym_sort)
		sort_tab = Sym_sort;	/* already have sorted table */
	else {
		/* make room for table to sort symbols in */
		sort_tab = Sym_sort = (struct nlist **)alloc(Symcnt * sizeof(struct nlist *));
		if( !sort_tab )
			fatal("Out of memory - cannot sort symbols");
	
		/* now fill in the table */
		i = 0;
		for( j = 0 ; j < HASHSIZE ; ++j )
			for( mp = Symtab[j] ; mp ; mp = mp->next )
				sort_tab[i++] = mp;
	}

	sortnsym(sort_tab,Symcnt); 	/* sort the symbols */

	outlst("                          Symbol Listing by Name");
	lst_nl();
	lst_nl();
	lst_nl();
	outlst("Name             Type    Value            Section           Attributes");
	lst_nl();

	for( i = 0 ; i < Symcnt ; ++i ){
		satt = sort_tab[i]->attrib;
		if (!Con_flag && !(satt & MSPCMASK))
			continue;	/* no constants wanted */
		if (!Loc_flag && !(satt & GLOBAL))
			continue;	/* no locals wanted */
		String[0] = EOS;
		(void)strncat (String, sort_tab[i]->name, LSTSYM);
		outlst(String);
		outdot(LSTSYM+2);
		if( satt & FPT )
			outlst("fpt");
		else if ( satt & INT )
			outlst("int");
		else
			outlst("frc");
		outspace(LSTSYM+8);
		if( satt & XSPACE )
			outlst("X:");
		else if( satt & YSPACE )
			outlst("Y:");
		else if( satt & PSPACE )
			outlst("P:");
		else if( satt & LSPACE )
			outlst("L:");
		else 
			outlst("  ");
		if( satt & FPT )
			(void)sprintf(String,"%-13.6f",sort_tab[i]->def.fdef);
		else if ( satt & INT )
			if( satt & LONG )
				(void)sprintf(String,"%06lx%06lx",
					sort_tab[i]->def.xdef.high,
					sort_tab[i]->def.xdef.low);
			else
				(void)sprintf(String,"%06lx",
					sort_tab[i]->def.xdef.low);
		outlst(strup(String));
		if( sort_tab[i]->sec != NULL ){
			outspace(LSTSYM+27);
			String[0] = EOS;
			(void)strncat(String, sort_tab[i]->sec->sname, LSTSYM);
			outlst(String);
			}
		if( satt & SET ){
			outspace(LSTSYM+LSTSYM+29);
			outlst("SET");
			}
		if( satt & GLOBAL ){
			outspace(LSTSYM+LSTSYM+29);
			outlst("GLOBAL");
			}
		if( satt & LOCAL ){
			outspace(LSTSYM+LSTSYM+29);
			outlst("LOCAL");
			}
		lst_nl();
		}
	lst_nl();
	lst_nl();
	lst_nl();
}

/**
*
* name 		sortnsym - sort symbols by name
*
**/
static
sortnsym(array,no_syms)
struct nlist **array;
int no_syms;
{
	int cmp_nsym ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_nsym;
	psort(0,no_syms - 1);
}

/**
*
* name 		cmp_nsym - compare symbols by name
*
**/
static
cmp_nsym (s1, s2)
struct nlist *s1, *s2;
{
	int cmp;
	double f1, f2, fdiff, vitod();

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	/* order by section name */
	if (s1->sec != s2->sec)
		if (!s1->sec)
			return (-1);
		else if (!s2->sec)
			return (1);
		else return (strcmp (s1->sec->sname, s2->sec->sname));

	/* order by value */
	if (s1->attrib & FPT)
		f1 = s1->def.fdef;
	else
		f1 = vitod (s1->def.xdef.high, s1->def.xdef.low);
	if (s2->attrib & FPT)
		f2 = s2->def.fdef;
	else
		f2 = vitod (s2->def.xdef.high, s2->def.xdef.low);
	fdiff = f1 - f2;
	if (fdiff < 0.0)
		return (-1);
	else if (fdiff > 0.0)
		return (1);
	return (0);
}

/**
*
* name 		print_symval - print symbol table listing by value
*
* description	Prints a list of the symbols defined in the link file
*		sorted by value.
*
**/
static
print_symval()
{
	struct nlist *mp;
	int i,j,k,satt,hlen;
	struct nlist **sort_tab;
	char *heading;
	char *strup();

	if( Symcnt == 0 )		/* no symbols */
		return;

	if (!Syv_flag)
		return;			/* listing not wanted */

	if (Sym_sort)
		sort_tab = Sym_sort;	/* already have sorted table */
	else {
		/* make room for table to sort symbols in */
		sort_tab = Sym_sort = (struct nlist **)alloc(Symcnt * sizeof(struct nlist *));
		if( !sort_tab )
			fatal("Out of memory - cannot sort symbols");
	
		/* now fill in the table */
		i = 0;
		for( j = 0 ; j < HASHSIZE ; ++j )
			for( mp = Symtab[j] ; mp ; mp = mp->next )
				sort_tab[i++] = mp;
	}

	sortvsym(sort_tab,Symcnt); 	/* sort the symbols */

	outlst("                          Symbol Listing by Value");
	lst_nl();
	lst_nl();
	lst_nl();
	heading = "Value            Name              ";
	hlen = strlen (heading);
	do
		outlst(heading);
	while (Lst_col + hlen < Max_col);
	lst_nl();

	j = 0;
	for( i = 0 ; i < Symcnt ; ++i ){
		satt = sort_tab[i]->attrib;
		if (!Con_flag && !(satt & MSPCMASK))
			continue;	/* no constants wanted */
		if (!Loc_flag && !(satt & GLOBAL))
			continue;	/* no locals wanted */
		if( satt & FPT )
			(void)sprintf(String,"%-13.6f",sort_tab[i]->def.fdef);
		else if ( satt & INT )
			if( satt & LONG )
				(void)sprintf(String,"%06lx%06lx",
					sort_tab[i]->def.xdef.high,
					sort_tab[i]->def.xdef.low);
			else
				(void)sprintf(String,"%06lx",
					sort_tab[i]->def.xdef.low);
		outlst(strup(String));
		outspace(j+18);
		String[0] = EOS;
		(void)strncat (String, sort_tab[i]->name, LSTSYM);
		outlst(String);
		k = strlen (String);
		k = (k > LSTSYM) ? 2 : (LSTSYM + 2) - k;
		if (Lst_col + k + hlen>= Max_col){
			j = 0;
			lst_nl();
			}
		else{
			j += hlen;
			outspace(j+1);
			}
		}
	lst_nl();
	lst_nl();
	lst_nl();
}

/**
*
* name 		sortvsym - sort symbols by value
*
**/
static
sortvsym(array,no_syms)
struct nlist **array;
int no_syms;
{
	int cmp_vsym ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_vsym;
	psort(0,no_syms - 1);
}

/**
*
* name 		cmp_vsym - compare symbols by value
*
**/
static
cmp_vsym (s1, s2)
struct nlist *s1, *s2;
{
	double f1, f2, fdiff, vitod();
	int cmp;

	/* order by value */
	if (s1->attrib & FPT)
		f1 = s1->def.fdef;
	else
		f1 = vitod (s1->def.xdef.high, s1->def.xdef.low);
	if (s2->attrib & FPT)
		f2 = s2->def.fdef;
	else
		f2 = vitod (s2->def.xdef.high, s2->def.xdef.low);
	fdiff = f1 - f2;
	if (fdiff < 0.0)
		return (-1);
	else if (fdiff > 0.0)
		return (1);

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	return (0);
}

/**
*
* name 		sortosym - sort object symbol names
*
**/
static
sortosym(array,no_syms)
struct nlist **array;
int no_syms;
{
	int cmp_osym ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_osym;
	psort(0,no_syms - 1);
}

/**
*
* name 		cmp_osym - compare object symbol names
*
**/
static
cmp_osym (s1, s2)
struct nlist *s1, *s2;
{
	int cmp;

	/* order by section number */
	if (s1->sec != s2->sec)
		if (!s1->sec)
			return (-1);
		else if (!s2->sec)
			return (1);
		else return (s1->sec->secno < s2->sec->secno ? -1 : 1);

	/* order by memory space */
	if ((s1->attrib & MSPCMASK) != (s2->attrib & MSPCMASK))
		return ((s1->attrib & MSPCMASK) < (s2->attrib & MSPCMASK) ?
			-1 : 1);

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	return (0);
}

/**
*
* name 		disp_xref - display remaining external references
*
* description	Displays a list of unresolved external references.
*
**/
disp_xref ()
{

	struct symref *xp;
	register i, j;
	struct symref **sort_tab;

	if( Xrfcnt == 0 )
		return; 	/* no unresolved references */

	if (Xrf_sort)		/* already have sort table */
		sort_tab = Xrf_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Xrf_sort = (struct symref **)alloc(Xrfcnt * sizeof(struct symref *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort unresolved references");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( xp = Xrftab[j]; xp; xp = xp->next )
				sort_tab[i++] = xp;

		sortxrefs(sort_tab,Xrfcnt); /* sort the sections */
	}

	(void)fprintf(Outfil,"\nUnresolved Externals:\n");

	for( i = 0 ; i < Xrfcnt ; ++i )
		(void)fprintf(Outfil,"%s\n",sort_tab[i]->name);

	(void)fprintf(Outfil,"\n");

	if (!Mapfil) {			/* no map file */
		fake_free((char *)sort_tab); /* release the sort table */
		Xrf_sort = NULL;
	}
}

/**
*
* name 		print_xref - print remaining external references
*
* description	Print a list of unresolved external references.
*
**/
print_xref ()
{

	struct symref *xp;
	register i, j;
	struct symref **sort_tab;

	if( Xrfcnt == 0 )
		return; 	/* no unresolved references */

	if (Xrf_sort)		/* already have sort table */
		sort_tab = Xrf_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Xrf_sort = (struct symref **)alloc(Xrfcnt * sizeof(struct symref *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort unresolved references");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( xp = Xrftab[j]; xp; xp = xp->next )
				sort_tab[i++] = xp;

		sortxrefs(sort_tab,Xrfcnt); /* sort the sections */
	}

	page_eject (YES);
	outlst ("Unresolved Externals:");
	lst_nl();
	lst_nl();

	for (i = 0; i < Xrfcnt; i++) {
		outlst (sort_tab[i]->name);
		lst_nl();
	}
	lst_nl();
	lst_nl();
	lst_nl();
}

/**
*
* name 		sortxrefs - sort unresolved externals
*
**/
static
sortxrefs(array,no_xrefs)
struct symref **array;
int no_xrefs;
{
	int cmp_xrefs ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_xrefs;
	psort(0, no_xrefs - 1);
}

/**
*
* name 		cmp_xrefs - compare external names
*
**/
static
cmp_xrefs (s1, s2)
struct symref *s1, *s2;
{
	return (strcmp (s1->name, s2->name));
}

/**
*
* name		start_obj - write START record to object file
*
* description	
*
**/
start_obj ()
{
	char *strup();

	if (!Lodfil)		/* no object file */
		return;

	Obj_rtype = OBJSTART;
	(void)fprintf (Lodfil, "_START %s ", Mod_name);
	(void)sprintf (Objbuf, "%04x %04x", Mod_vers, Mod_rev);
	(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
	Objbuf[0] = EOS;
	if (Mod_com) {		/* module header comment */
		if (strlen (Mod_com) > MAXOBJLINE)
			*(Mod_com + MAXOBJLINE) = EOS;
		(void)fprintf (Lodfil, "%s", Mod_com);
	}
}

/**
*
* name 		done_obj - terminate the object file
*
* description	If debug mode is on, write the symbol table info to the
*		object file.  If an OMF record has been
*		generated, write the object file buffer and append a
*		newline to it.  Write out the OMF END record.
*
**/
done_obj ()
{
	char *strup();

	if (!Lodfil)		/* no object file */
		return;

	if (Obj_rtype != OBJNULL) {
		if (!(Obj_rtype == OBJRDATA ||
		      Obj_rtype == OBJRBLKDATA))
			(void)strup (Objbuf);
		(void)fprintf (Lodfil, "%s\n", Objbuf);
		if (End_addr < 0L)
			(void)sprintf (Objbuf, "_END");
		else
			(void)sprintf (Objbuf, "_END %04lx",
				End_addr & (long)MAXADDR);
		(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
	}
}

/**
*
* name		emit_data --- emit a data word to code file
*
* description	Emits data word to code file.
*
**/
emit_data (val)
long val[];
{
	static space = NONE;
	static map = NONE;
	static long pc = 0L;
	static char *bp = Objbuf;
	char spc_char(), *strup();

	Old_pc = *Pc;		/* save current location counters */
	Old_lpc = *Lpc;
	val[HWORD] &= WORDMASK;	/* mask off high bits */
	val[LWORD] &= WORDMASK;	/* mask off high bits */
	if(Lodfil) { /* if there is an object file */

		/* new record type, data space, or address */
		if (Obj_rtype != OBJDATA || space != Loadsp ||
		    map != Lmap || pc != *Lpc) {
			if (!(Obj_rtype == OBJRDATA ||
			      Obj_rtype == OBJRBLKDATA))
				(void)strup (Objbuf);
			(void)fprintf (Lodfil, "%s\n", Objbuf);
			Obj_rtype = OBJDATA;
			space = Loadsp;
			map = Lmap;
			pc = *Lpc;
			bp = Objbuf;
			(void)strcpy (Objbuf, "_DATA ");
			bp += strlen(bp);
			*bp++ = spc_char (space);
			if (map)
				*bp++ = map_char (map);
			(void)sprintf (bp, " %04lx", pc & (long)MAXADDR);
			(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
			bp = Objbuf;	/* reset buffer pointer */
		/* need to start new line */
		} else if (bp + MAXVALSIZE + 1 > Objbuf + MAXOBJLINE) {
			(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
			bp = Objbuf;	/* reset buffer pointer */
		}
		if (Loadsp == LSPACE)
			(void)sprintf (bp, "%06lx %06lx ", val[HWORD],
				val[LWORD]);
		else
			(void)sprintf (bp, "%06lx ", val[LWORD]);
		(void)strup (bp);
		bp += strlen (bp);
		pc++; /* increment local program counter */
	}

	++*Pc;		/* increment program counters */
	if ((*Pc &= MAXADDR) < Old_pc)
		error ("Location counter overflow");
	if( Pc != Lpc ){
		++*Lpc;
		if ((*Lpc &= MAXADDR) < Old_lpc)
			error ("Location counter overflow");
		}
}

/**
*
* name		emit_bdata --- emit block data record to code file
*
* description	Emits a block data record to the object file.
*
**/
emit_bdata (count, val)
long count, val[];
{
	char *bp, *sp, *strup(), spc_char();

	Old_pc = *Pc;		/* save current location counters */
	Old_lpc = *Lpc;
	val[HWORD] &= WORDMASK;	/* mask off high bits */
	val[LWORD] &= WORDMASK;	/* mask off high bits */
	if (Lodfil) { /* if there is an object file */
		if (!(Obj_rtype == OBJRDATA || Obj_rtype == OBJRBLKDATA))
			(void)strup (Objbuf);
		(void)fprintf (Lodfil, "%s\n", Objbuf);
		Obj_rtype = OBJBLKDATA;
		bp = Objbuf;
		(void)strcpy (Objbuf, "_BLOCKDATA ");
		bp += strlen(bp);
		sp = bp;	/* save buffer pointer */
		*bp++ = spc_char (Loadsp);
		if (Lmap)
			*bp++ = map_char (Lmap);
		if (Loadsp == LSPACE) {
#ifdef BIT48
			(void)sprintf (bp, " %04lx %04lx %06lx %06lx",
				*Lpc, count, val[HWORD], val[LWORD]);
#else
			*sp = 'X';
			(void)sprintf (bp, " %04lx %04lx %06lx",
				*Lpc, count, val[HWORD]);
			(void)fprintf (Lodfil, "%s\n", Objbuf);
			*sp = 'Y';
			(void)sprintf (bp, " %04lx %04lx %06lx",
				*Lpc, count, val[LWORD]);
#endif
		} else
			(void)sprintf (bp, " %04lx %04lx %06lx",
				*Lpc, count, val[LWORD]);
	}

	*Pc += count;	/* increment program counters */
	if ((*Pc &= MAXADDR) < Old_pc)
		error ("Location counter overflow");
	if( Pc != Lpc ){
		*Lpc += count;
		if ((*Lpc &= MAXADDR) < Old_lpc)
			error ("Location counter overflow");
		}
}

/**
*
* name		emit_comment --- emit comment record to code file
*
* description	Emits a comment record to the object file.
*
**/
emit_comment (str)
char *str;
{
	char *strup();

	if (!Lodfil)	/* no object file */
		return;

	Obj_rtype = OBJCOMMENT;
	if (!(Obj_rtype == OBJRDATA || Obj_rtype == OBJRBLKDATA))
		(void)strup (Objbuf);
	(void)fprintf (Lodfil, "%s\n", Objbuf);
	(void)fprintf (Lodfil, "_COMMENT\n");
	(void)fprintf (Lodfil, "%s", str);
	Objbuf[0] = EOS;
}

/**
*
* name 		sym_obj - write symbol information to object file
*
* description	Sort the symbol table and write a SYMOBJ record
*		for each section and memory space grouping.
*
**/
static
sym_obj ()
{
	int i,j,k,secno,mem,map;
	struct nlist **sort_tab, *mp;
	char *bp, spc_char (), *strup();

	if (!Lodfil)		/* no object file */
		return;

	/* make room for table to sort symbols in */
	sort_tab = Sym_sort = (struct nlist **)alloc(Symcnt * sizeof(struct nlist *));
	if( !sort_tab )
		fatal("Out of memory - cannot sort symbols");

	/* now fill in the table */
	i = 0;
	for( j = 0 ; j < HASHSIZE ; j++ )
		for( mp = Symtab[j] ; mp ; mp = mp->next )
			sort_tab[i++] = mp;

	sortosym(sort_tab,Symcnt); 	/* sort the symbols */

	j = k = -1;
	if (!(Obj_rtype == OBJRDATA || Obj_rtype == OBJRBLKDATA))
		(void)strup (Objbuf);
	Obj_rtype = OBJSYMBOL;	/* set new record type */
	for (i = 0; i < Symcnt; i++) {
		mp = sort_tab[i];
		if (!(mp->attrib & GLOBAL))
			continue;	/* skip section local labels */
		(void)fprintf (Lodfil, "%s\n", Objbuf);
		secno = mp->sec ? mp->sec->secno : 0;
		mem = mp->attrib & MSPCMASK;
		map = mp->mapping & MMAPMASK;
		if (secno != j || mem != k) {	/* new section or space */
			j = secno;	/* save section */
			k = mem;	/* save memory space */
			(void)strcpy (Objbuf, "_SYMBOL ");
			bp = Objbuf + strlen(Objbuf);
			*bp++ = spc_char (mem);
			if (map)
				*bp++ = map_char (map);
			*bp = EOS;
			(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
		}
		(void)fprintf (Lodfil, "%-16s ", mp->name);
		if (mp->attrib & FPT)
			(void)sprintf (Objbuf, "F %-13.6f", mp->def.fdef);
		else
			if (mp->attrib & LONG)
				(void)sprintf (Objbuf, "I 06lx %06lx",
					mp->def.xdef.high,
					mp->def.xdef.low);
			else
				(void)sprintf (Objbuf, "I %06lx",
					mp->def.xdef.low);
		(void)strup (Objbuf);
	}

	if (!Mapfil)			/* no map file */
		fake_free((char *)sort_tab); /* release the sort table */
}

/**
*
* name 		sect_obj - write section information to object file
*
* description	Scan the section list and write out section records
*		in order.
*
**/
static
sect_obj ()
{
	struct slist *sl, **sort_tab;
	char spc_char (), *strup();
	register i, j;

	if (!Lodfil)		/* no object file */
		return;

	if (Sec_sort)		/* already have table */
		sort_tab = Sec_sort;
	else {
		/* make room for a table to sort them in */
		sort_tab = Sec_sort = (struct slist **)alloc(Sectot * sizeof(struct slist *));
		if( sort_tab == NULL )
			fatal("Out of memory - cannot sort section names");
	
		/* now fill in the table */
		i = 0;
		for (j = 0; j < HASHSIZE; j++)
			for( sl = Sectab[j]; sl; sl = sl->next )
				sort_tab[i++] = sl;
		sort_tab[i] = NULL;	/* seal off list */
	}

	sortnsec(sort_tab,Sectot);	/* sort the sections */

	Obj_rtype = OBJSECTION;		/* set new record type */
	if (!(Obj_rtype == OBJRDATA || Obj_rtype == OBJRBLKDATA))
		(void)strup (Objbuf);
	(void)fprintf (Lodfil, "%s\n", Objbuf);
	for (i = 0; i < Sectot; i++) {
		sl = sort_tab[i];
		(void)fprintf (Lodfil, "%s\n", strup (Objbuf));
		(void)fprintf (Lodfil, "_SECTION %s %c ",
			sl->sname, sl->flags & REL ? 'R' : 'A');
		(void)sprintf (Objbuf, "%04x", sl->secno);
                (void)fprintf (Lodfil, "%s\n", strup (Objbuf));
                (void)sprintf (Objbuf, "%04lx %04lx %04lx %04lx %04lx %04lx",
			sl->cntrs[XMEM-1][DCNTR], sl->cntrs[XMEM-1][LCNTR],
			sl->cntrs[XMEM-1][HCNTR], sl->cntrs[YMEM-1][DCNTR],
			sl->cntrs[YMEM-1][LCNTR], sl->cntrs[YMEM-1][HCNTR]);
                (void)fprintf (Lodfil, "%s\n", strup (Objbuf));
                (void)sprintf (Objbuf, "%04lx %04lx %04lx %04lx %04lx %04lx",
			sl->cntrs[LMEM-1][DCNTR], sl->cntrs[LMEM-1][LCNTR],
			sl->cntrs[LMEM-1][HCNTR], sl->cntrs[PMEM-1][DCNTR],
			sl->cntrs[PMEM-1][LCNTR], sl->cntrs[PMEM-1][HCNTR]);
		(void)strup (Objbuf);
	}
	if (!Mapfil)				/* no map file */
		fake_free((char *)sort_tab);	/* release the sort table */
}

