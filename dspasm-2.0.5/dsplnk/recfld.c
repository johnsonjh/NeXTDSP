#include "lnkdef.h"
#include "lnkdcl.h"
#if AZTEC
#ifdef fgetc
#undef fgetc	/* clear special Aztec definition */
#define fgetc getc
#endif
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
* name          get_record - read in link file record header
*
* synopsis      type = get_record ()
*               int type;	link record type
*
* description   Examines the first character in the field buffer and
*		keeps getting fields until a new record appears.
*		Hashes the record name into the record table and
*		if found returns the record type, otherwise NONE.
*
**/
get_record ()
{
	int fld = 0, rec = OBJNULL;
	struct rectype *rect;
	char *fbp = Field + 1;

	while (Field[0] != NEWREC && (fld = get_field (OBJANY)) == 0)
		;
	if (fld >= 0) {			/* not EOF */
		rect = &Rectab[rechash (fbp)];	/* lookup record type */
		if (rect->name && STREQ (fbp, rect->name)) {
			rec = rect->type;	/* save record type */
			Recno++;		/* increment record number */
		}
	}
	Field[0] = EOS;			/* clear field buffer */
	return (rec);
}


/**
* name          get_field - read in next link file field
*
* synopsis      rc = get_field (type)
*               int rc;		return code
*		int type;	field type
*
* description   Reads from the current input file, skipping whitespace
*		until next nonspace character.  Reads characters into
*		field buffer until next whitespace character.  If EOF
*		encountered, returns EOF.  If current character is a
*		new record character returns YES, otherwise NO.
*
**/
get_field (type)
int type;
{
	register c;
	register char *p;
	long ftell ();

	if (type == MEMCOMMENT) {	/* memory file comment */
		while ((c = fgetc (Fd)) > 0 && c != ENDLINE)
			;
		if (c > 0) {
			Lineno++;	/* increment line number */
			return (NO);
		} else
			return (EOF);
	}

	while ((c = fgetc (Fd)) > 0 && isspace (c))
		if (c == ENDLINE) {	/* new line */
			Lineno++;	/* increment line number */
			if (type == OBJCOMMENT)
				break;	/* quit on comment */
		}

	if (c <= 0 || (type == OBJCOMMENT && c != ENDLINE)) {
		Field[0] = EOS;
		return (EOF);		/* end of file or synch error */
	}

	if (c == (int)NEWREC) {		/* new record */
		if ((Recoff = ftell (Fd)) < 0L)	/* save record offset */
			fatal ("Offset failure");
		--Recoff;			/* adjust offset */
	}

	/* get first comment character */
	if (type == OBJCOMMENT && (c = fgetc (Fd)) == ENDLINE) {
		Field[0] = EOS;
		return (NO);
	}

	/* loop to pick up field value */
	for (p = Field, *p++ = c; (c = fgetc (Fd)) > 0; *p++ = c)
		if ( (type == OBJCOMMENT && c == ENDLINE) ||
		     (type != OBJCOMMENT && isspace (c)) )
			break;
	*p = EOS;			/* null at end of field */

	if (c <= 0 || (type == OBJCOMMENT && c != ENDLINE)) {
		Field[0] = EOS;
		return (EOF);		/* end of file or synch error */
	}

	if (type == OBJCOMMENT) {
		if (*--p == RETURN)	/* look for return character */
			*p = EOS;
		return (NO);
	}


#ifdef VAXCKLUDGE       		/* kludge for VMS VAX C RTL */
	if (fseek (Fd, ftell (Fd) - 1L, SEEK_SET) != 0) {
#else
	if (ungetc (c, Fd) == EOF) {	/* put back last character */
#endif
		Field[0] = EOS;
		return (EOF);		/* I/O error */
	}

	if (Field[0] == NEWREC) {	/* new record */
		Fldno = 0;		/* clear field count */
		return (YES);
	} else {
		Fldno++;		/* increment field count */
		return (NO);
	}
}


/**
* name		get_eof --- read input until EOF
*
* synopsis	get_eof ()
*
* description 	Calls get_field repeatedly until EOF is returned.
*
**/
get_eof ()
{
	while (get_field (OBJANY) >= 0)	/* loop until EOF */
		;
}

/**
* name		get_header --- read library module header
*
* synopsis	get_header (lib, hdr)
*		int yn;		NO on EOF or error; YES otherwise
*		FILE *lib;	file pointer for library
*		char *hdr;	pointer to header buffer
*
* description 	Reads the file lib looking for a library module header.
*		Puts the header in hdr and returns YES on success; NO
*		at EOF or if an error occurs.
*
**/
get_header (lib, hdr)
FILE *lib;
char *hdr;
{
	register char *p;
	int c;

	clearerr (lib);		/* clear error status */
	for (p = hdr;
	     (c = fgetc (lib)) != EOF && c != RETURN && c != NEWLINE;
	     p++)
		*p = c;		/* read header into buffer */
	*p = EOS;
	if (feof (lib))
		return (NO);	/* end of file */
	else if (ferror (lib))
		fatal ("Invalid library module header");
	if (c == RETURN && (c = fgetc (lib)) != EOF && c != NEWLINE)
		ungetc (c, lib);
	if (strncmp (hdr, Modhdr, Modlen) != 0)
		fatal ("Invalid library module header format");
	return (YES);
}

/**
* name          get_start - locate START record in module
*
* synopsis      yn = get_start ()
*               int yn;		YES/NO if errors
*
* description   Reads from the current input file looking for a START
*		record.  If found, reads in version, revision, and
*		comment fields and saves them.  Returns NO on error.
*
**/
get_start ()
{
	int type;
	int errcnt = 0;

/*
	locate START record
*/

	while ((type = get_record ()) != EOF && type != OBJSTART)
		;

	if (type == EOF) {		/* premature EOF */
		error ("No START record");
		return (NO);
	}

	if (Mod_name)			/* already have module name */
		return (YES);

	if (get_field (OBJANY) != 0) {	/* pick up module name */
		error ("Invalid module name field");
		return (NO);
	}
	if ((Mod_name = (char *)alloc (strlen (Field) + 1)) == NULL)
		fatal ("Out of memory - cannot save module name");
	(void)strcpy (Mod_name, Field);

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* version number */
		error ("Invalid version number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &Mod_vers);

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* revision number */
		error ("Invalid revision number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &Mod_rev);

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* error count */
		error ("Invalid error count field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &errcnt);
/*
	if (errcnt > 0 && !Testing) {
		error ("Invalid link module - errors in assembly");
		return (NO);
	}
*/
	if (get_field (OBJCOMMENT) != 0) { 	/* pick up comment */
		error ("Invalid START comment field");
		return (NO);
	}
	if ((Mod_com = (char *)alloc (strlen (Field) + 1)) == NULL)
		fatal ("Out of memory - cannot save module comment");
	(void)strcpy (Mod_com, Field);

	return (YES);
}


/**
* name          get_section - process SECTION link records
*
* synopsis      yn = get_section (ftype)
*               int yn;		YES/NO on errors
*		int ftype;	file type flag
*
* description   Reads from the current input file saving SECTION record
*		fields in a working buffer.  When all fields are collected
*		calls the instlsec function to install the section in
*		the section table if the file type is ANY; otherwise
*		calls lstsec to add to special library search section list.
*
**/
get_section (ftype)
int ftype;
{
	char sname[MAXBUF];
	struct slist buf;
	register i, j;

	if (ftype == LIB)
		Gagerrs = YES;		/* inhibit errors */

	if (get_field (OBJANY) != 0) {	/* pick up section name */
		error ("Invalid section name field");
		Gagerrs = NO;
		return (NO);
	}
	(void)strcpy (sname, Field);
	buf.sname = sname;

	if (get_field (OBJANY) != 0) {	/* read in section type */
		error ("Invalid section type field");
		Gagerrs = NO;
		return (NO);
	}

	if (Field[0] == 'A')		/* absolute section */
		buf.flags = 0;
	else if (Field[0] == 'R')	/* relocatable section */
		buf.flags = REL;	/* set flag */
	else {
		error ("Invalid section type field");
		Gagerrs = NO;
		return (NO);
	}

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* section number */
		error ("Invalid section number field");
		Gagerrs = NO;
		return (NO);
	}
	(void)sscanf (Field, "%x", &buf.secno);

	for (i = 0; i < MSPACES; i++) {	/* read in memory space lengths */
		for (j = 0; j < MCNTRS; j++) {
			if (get_field (OBJANY) != 0 || !fldhex ()) {
				error ("Invalid section counter field");
				Gagerrs = NO;
				return (NO);
			}
			(void)sscanf (Field, "%lx", &buf.cntrs[i][j]);
			buf.start[i][j] = 0L;
			buf.cflags[i][j] = 0;
		}
	}

	Gagerrs = NO;
	if (ftype == LIB) {
		if (!lstsec (&buf))		/* put section on list */
			return (NO);		/* failed */
	} else {
		if (!instlsec (&buf))		/* install section */
			return (NO);		/* could not install */
	}

	return (YES);
}


/**
* name          get_symbol - process SYMBOL link records
*
* synopsis      yn = get_symbol ()
*               int yn;		YES/NO on errors
*
* description   Reads from the current input file saving SYMBOL record
*		fields in a working buffer.  When all fields are collected
*		calls instlsym to install the symbol in the symbol table.
*
**/
get_symbol ()
{
	char name[MAXBUF];
	struct nlist buf;
	int secno, spc;
	int ctr = DEFAULT;
	int map = NONE;
	int i, s, c, m;
	char *p;
	struct evres *ev, *eval();

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* section number */
		error ("Invalid section number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &secno);
	Csect = Secmap[secno].sec;		/* set up current section */

	if (get_field (OBJANY) != 0 || (spc = char_spc (Field[0])) < 0) {
		error ("Illegal memory space specified");
		return (NO);
	}

	if (!Field[1])				/* no counters, mapping */
		Field[2] = EOS;			/* clear next attrib. field */
	else if ((ctr = char_ctr (Field[1])) < 0) {
		if ((map = char_map (Field[1])) < 0) {
			error ("Illegal memory counter specified");
			return (NO);
		} else {			/* good mapping attribute */
			Field[2] = EOS;		/* clear mapping field */
			ctr = DEFAULT;		/* set default counter */
		}
	}

	if (Field[2] && (map = char_map (Field[2])) < 0) {
		error ("Illegal memory map character");
		return (NO);
	}

	Cspace = Loadsp = spc;			/* save memory space */
	Rcntr = Lcntr = ctr;			/* save counter type */
	s = spc_off (spc) - 1;			/* get space offset */
	c = ctr_off (ctr);			/* get counter offset */
	if (c < 0)				/* check offsets */
		fatal ("Counter offset failure");

	if (spc) {				/* valid memory space */
		if ( (m = Csect->cflags[s][c] & MMAPMASK) != 0 ||
		     (m = Mflags[s][c] & MMAPMASK) != 0 )
			map = m;
		Rmap = Lmap = map;
	} else
		Rmap = Lmap = NONE;

	while ((i = get_field (OBJANY)) == 0) {	/* loop to pick up symbols */

		(void)strcpy (name, Field);	/* save symbol name */
		buf.name = name;

		buf.attrib = spc;		/* initialize attributes */

		if (get_field (OBJANY) != 0) {
			error ("Invalid symbol type field");
			return (NO);
		}
		p = Field;
		if (*p == 'L')			/* local symbol */
			;			/* do nothing */
		else if (*p == 'G')		/* global symbol */
			buf.attrib |= GLOBAL;	/* set flag */
		else {
			error ("Invalid symbol type field");
			return (NO);
		}
		p++;				/* move to next subfield */
		if (*p == 'A')			/* absolute symbol */
			;			/* do nothing */
		else if (*p == 'R')		/* relocatable symbol */
			buf.attrib |= REL;	/* set flag */
		else {
			error ("Invalid symbol type field");
			return (NO);
		}
		p++;				/* move to next subfield */
		if (*p == 'I')			/* integer symbol */
			buf.attrib |= INT;	/* set flag */
		else if (*p == 'F')		/* floating point symbol */
			buf.attrib |= FPT;	/* set flag */
		else {
			error ("Invalid symbol type field");
			return (NO);
		}

		if (get_field (OBJANY) != 0) {	/* get symbol value */
			error ("Invalid symbol value field");
			return (NO);
		}
		Optr = Field;
		if ((ev = eval ()) == NULL)
			return (NO);

		if (ev->etype == INT) {
			if (!(buf.attrib & INT)) {
				error ("Invalid symbol value format");
				return (NO);
			}
			buf.def.xdef.ext  = ev->uval.xvalue.ext;
			buf.def.xdef.high = ev->uval.xvalue.high;
			buf.def.xdef.low  = ev->uval.xvalue.low;
			if (ev->size == SIZEL)
				buf.attrib |= LONG;	/* long value */
		} else {	/* assume floating point */
			if (!(buf.attrib & FPT)) {
				error ("Invalid symbol value format");
				return (NO);
			}
			buf.def.fdef = ev->uval.fvalue;
		}
		free_exp (ev);

		if (buf.attrib & REL)		/* rel sym; adjust value */
			buf.def.xdef.low += Csect->start[s][c];

		buf.mapping = ctr | map;	/* set mapping attributes */

		buf.sec = Csect;
		buf.next = NULL;
		(void)instlsym (&buf);		/* install symbol */
	}

	if (i < 0) {		/* EOF */
		error ("Invalid symbol name field");
		return (NO);
	}
	return (YES);
}


/**
* name          get_symbuf - return SYMBOL record buffer
*
* synopsis      sym = get_symbuf ()
*               struct slist *sym;	address of symbol buffer
*
* description   Reads in symbol values from the current input file.
*		Puts the values in a symbol structure and returns it.
*		Returns NULL when a new record or EOF is encountered.
*
**/
struct nlist *
get_symbuf ()
{
	static char name[MAXBUF];
	static struct nlist sym = {name};
	struct slist *sp;
	int i, secno, mem;

	if (!Field[0] || Field[0] == NEWREC) {	/* new record */

		if (Field[0] == NEWREC && get_record () != OBJSYMBOL)
			return (NULL);		/* not sym record */

		if (get_field (OBJANY) != 0 || !fldhex ())
			return (NULL);
		(void)sscanf (Field, "%x", &secno);
		for (i = 0, sp = Seclst; sp && i < secno; i++, sp = sp->next)
			;			/* find section in list */
		if (!sp)			/* couldn't find it */
			return (NULL);
		Csect = sym.sec = sp;		/* save section ptr */

		if (get_field (OBJANY) != 0 ||	/* save memory space */
		    (mem = char_spc (Field[0])) < 0)
			return (NULL);
		Cspace = Loadsp = mem;
	}

	if ((i = get_field (OBJANY)) < 0)	/* get name field */
		return (NULL);
	else if (i > 0)				/* new record */
		return (get_symbuf ());
	(void)strcpy (name, Field);		/* save name */
	if (get_field (OBJANY) != 0)		/* get type field */
		return (NULL);
	sym.attrib = (Field[0] == 'G') ? GLOBAL : 0;
	if (get_field (OBJANY) != 0)		/* skip value field */
		return (NULL);

	/* clear extraneous fields */
	sym.def.xdef.ext = sym.def.xdef.high = sym.def.xdef.low = 0L;
	sym.mapping = 0;
	sym.next = NULL;
	return (&sym);
}


/**
* name          get_data - process DATA link records
*
* synopsis      yn = get_data ()
*               int yn;		YES/NO on errors
*
* description   Reads from the current input file saving DATA record header
*		fields in a working buffer.  When all fields are collected
*		uses section number and memory space to readjust starting
*		address.  Then calls emit_data to output load file record.
*
**/
get_data ()
{
	int secno, spc;
	int ctr = DEFAULT;
	int map = NONE;
	int i, s, c, m;
	long addr, val[EWLEN];
	struct slist *set_absec ();
	struct evres *ev, *eval();

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* section number */
		error ("Invalid section number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &secno);
	Secno = secno;			/* save global section number */

	if (get_field (OBJANY) != 0 || (spc = char_spc (Field[0])) < 0) {
		error ("Illegal memory space specified");
		return (NO);
	}

	if (!Field[1])				/* no counters, mapping */
		Field[2] = EOS;			/* clear next attrib. field */
	else if ((ctr = char_ctr (Field[1])) < 0) {
		if ((map = char_map (Field[1])) < 0) {
			error ("Illegal memory counter specified");
			return (NO);
		} else {			/* good mapping attribute */
			Field[2] = EOS;		/* clear mapping field */
			ctr = DEFAULT;		/* set default counter */
		}
	}

	if (Field[2] && (map = char_map (Field[2])) < 0) {
		error ("Illegal memory map character");
		return (NO);
	}

	Cspace = Loadsp = spc;			/* save memory space */
	Rcntr = Lcntr = ctr;			/* save counter type */
	s = spc_off (spc) - 1;			/* get space offset */
	c = ctr_off (ctr);			/* get counter offset */
	if (s < 0 || c < 0)			/* check offsets */
		fatal ("Space/counter offset failure");

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* address */
		error ("Invalid address field");
		return (NO);
	}
	(void)sscanf (Field, "%lx", &addr);

	Csect = Secmap[secno].sec;		/* get section */
	if (!(Csect->flags & REL))		/* absolute section */
		Csect = set_absec (addr);	/* alloc new section */
	Pc = Lpc = &Csect->cntrs[s][c];		/* set up counter */

	if ( (m = Csect->cflags[s][c] & MMAPMASK) != 0 ||
	     (m = Mflags[s][c] & MMAPMASK) != 0 )
		Rmap = Lmap = m;		/* assign mapping */
	else
		Rmap = Lmap = map;

	while ((i = get_field (OBJANY)) == 0) {	/* loop to pick up values */

		Optr = Field;
		if ((ev = eval ()) == NULL)
			val[EWORD] = val[HWORD] = val[LWORD] = 0L;
		else {
			if (ev->etype == FPT)	/* float; convert to frac */
				dtof (ev->uval.fvalue,
				      Cspace == LSPACE ? SIZEL : SIZES, ev);
			val[EWORD] = ev->uval.xvalue.ext;
			val[HWORD] = ev->uval.xvalue.high;
			val[LWORD] = ev->uval.xvalue.low;
		}
		free_exp (ev);

		emit_data (val);	/* generate object code */
	}

	if (i < 0) {		/* EOF */
		error ("Invalid data value field");
		return (NO);
	}

	return (YES);
}


/**
* name          get_bdata - process BLOCKDATA link records
*
* synopsis      yn = get_bdata ()
*               int yn;		YES/NO on errors
*
* description   Reads from the current input file saving BLOCKDATA record
*		header fields in a working buffer.  When all fields are
*		collected uses section number and memory space to readjust
*		starting address.  Then calls emit_data to output load
*		file record.
*
**/
get_bdata ()
{
	int secno, spc;
	int ctr = DEFAULT;
	int map = NONE;
	int i, s, c, m;
	long addr, count, val[EWLEN];
	struct slist *set_absec ();
	struct evres *ev, *eval();

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* section number */
		error ("Invalid section number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &secno);
	Secno = secno;			/* save global section number */

	if (get_field (OBJANY) != 0 || (spc = char_spc (Field[0])) < 0) {
		error ("Illegal memory space specified");
		return (NO);
	}

	if (!Field[1])				/* no counters, mapping */
		Field[2] = EOS;			/* clear next attrib. field */
	else if ((ctr = char_ctr (Field[1])) < 0) {
		if ((map = char_map (Field[1])) < 0) {
			error ("Illegal memory counter specified");
			return (NO);
		} else {			/* good mapping attribute */
			Field[2] = EOS;		/* clear mapping field */
			ctr = DEFAULT;		/* set default counter */
		}
	}

	if (Field[2] && (map = char_map (Field[2])) < 0) {
		error ("Illegal memory map character");
		return (NO);
	}

	Cspace = Loadsp = spc;			/* save memory space */
	Rcntr = Lcntr = ctr;			/* save counter type */
	s = spc_off (spc) - 1;			/* get space offset */
	c = ctr_off (ctr);			/* get counter offset */
	if (s < 0 || c < 0)			/* check offsets */
		fatal ("Space/counter offset failure");

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* address */
		error ("Invalid address field");
		return (NO);
	}
	(void)sscanf (Field, "%lx", &addr);

	Csect = Secmap[secno].sec;		/* get section */
	if (!(Csect->flags & REL))		/* absolute section */
		Csect = set_absec (addr);	/* alloc new section */
	Pc = Lpc = &Csect->cntrs[s][c];		/* set up counter */

	if ( (m = Csect->cflags[s][c] & MMAPMASK) != 0 ||
	     (m = Mflags[s][c] & MMAPMASK) != 0 )
		Rmap = Lmap = m;		/* assign mapping */
	else
		Rmap = Lmap = map;

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* count */
		error ("Invalid count field");
		return (NO);
	}
	(void)sscanf (Field, "%lx", &count);

	if ((i = get_field (OBJANY)) > 0) {	/* pick up data expression */
		*Pc += count;			/* no value; bump counter */
		return (YES);
	} else if (i < 0) {			/* premature EOF */
		error ("Invalid data value field");
		return (NO);
	}

	Optr = Field;
	if ((ev = eval ()) == NULL)
		val[EWORD] = val[HWORD] = val[LWORD] = 0L;
	else {
		if (ev->etype == FPT)	/* float; convert to frac */
			dtof (ev->uval.fvalue,
			      Cspace == LSPACE ? SIZEL : SIZES, ev);
		val[EWORD] = ev->uval.xvalue.ext;
		val[HWORD] = ev->uval.xvalue.high;
		val[LWORD] = ev->uval.xvalue.low;
	}
	free_exp (ev);

	emit_bdata (count, val);	/* generate object code */

	return (YES);
}


/**
* name          get_xref - process XREF link records
*
* synopsis      yn = get_xref ()
*               int yn;		YES/NO on errors
*
* description   Reads from the current input file saving XREF record
*		fields in a working buffer.  For each name in the field
*		checks if the name is already in the symbol table; if
*		not, calls instlxrf to install the symbol into the external
*		reference table.
*
**/
get_xref ()
{
	int i, secno;
	struct nlist *lkupsym ();

	if (get_field (OBJANY) != 0 || !fldhex ()) {	/* section number */
		error ("Invalid section number field");
		return (NO);
	}
	(void)sscanf (Field, "%x", &secno);
	Secno = secno;				/* save global sec. number */
	Csect = Secmap[secno].sec;		/* get section */

	while ((i = get_field (OBJANY)) == 0)	/* loop to pick up symbols */
		if (!lkupsym (Field))		/* not in symbol table? */
			instlxrf (Field);	/* install symbol */

	if (i < 0) {		/* EOF */
		error ("Invalid symbol name field");
		return (NO);
	}
	return (YES);
}


/**
* name          get_comment - process COMMENT link records
*
* synopsis      yn = get_comment ()
*               int yn;		YES/NO on errors
*
* description   Reads from the current input file and saves the
*		comment field.  On pass 2, writes a new COMMENT
*		record to the load file.
**/
get_comment ()
{
	if (get_field (OBJCOMMENT) != 0) {	/* pick up comment */
		error ("Invalid comment field");
		return (NO);
	}

	if (Pass == 2)
		emit_comment (Field);	/* write comment to load file */

	return (YES);
}


/**
* name          get_end - process END record in link module
*
* synopsis      yn = get_end ()
*               int yn;		YES/NO if END record valid/not valid
*
* description   Reads from the current input file to pick up the
*		end address given in the END link record.  If an END
*		record has already been processed, returns YES.
*		If EOF, also returns YES.  If new record or bad
*		address expression, returns NO.
**/
get_end ()
{
	int fld;
	struct evres *ev, *eval_ad();

	if (End_addr >= 0L)		/* already have end address */
		return (YES);

	if ((fld = get_field (OBJANY)) < 0)	/* get address field */
		return (YES);		/* OK if EOF */

	if (fld > 0) {			/* new record type -- error */
		error ("Invalid END address field");
		return (NO);
	}

	Optr = Field;
	if ((ev = eval_ad ()) == NULL)
		return (NO);

	End_addr = ev->uval.xvalue.low;	/* save address */
	free_exp (ev);

	return (YES);
}


/**
* name          set_absec - return absolute section structure
*
* synopsis      p = set_absec (addr)
*		struct slist *p;	pointer to section structure
*		long addr;		section start address
*
* description   Check length of current section.  If zero, return
*		current section.  Otherwise, allocate new section
*		structure, copy data fields, and link into section
*		chain.
**/
static struct slist *
set_absec (addr)
long addr;
{
	struct slist *sp;
	int s, c;
	register i, j;

	s = spc_off (Loadsp) - 1;		/* get space offset */
	c = ctr_off (Lcntr);			/* get counter offset */
	if (s < 0 || c < 0)			/* check offsets */
		fatal ("Space/counter offset failure");

	for (sp = Csect; sp && sp->secno == Csect->secno; sp = sp->next)
		if (sp->start[s][c] == sp->cntrs[s][c]) { /* sect. not used */
			sp->start[s][c] = sp->cntrs[s][c] = addr;
			return (sp);
		}

	/* allocate new section structure */
	if ((sp = (struct slist *)alloc (sizeof (struct slist))) == NULL)
		fatal ("Out of memory - cannot save absolute section data");
	sp->sname = Csect->sname;		/* save section name */
	sp->secno = Csect->secno;		/* save section number */
	sp->flags = Csect->flags;		/* save section flags */
	for (i = 0; i < MSPACES; i++)		/* clear counters */
		for (j = 0; j < MCNTRS; j++) {
			sp->cflags[i][j] = 0;
			sp->cntrs[i][j] = sp->start[i][j] = 0L;
		}
	sp->start[s][c] = sp->cntrs[s][c] = addr;	/* save address */
	sp->next = Csect->next;			/* link into chain */
	Csect->next = sp;
	Sectot++;				/* increment section total */
	return (sp);
}

/**
*
* name          fldhex - insure field value is a hex number
*
**/
static
fldhex ()
{
	register char *p;

	for (p = Field; *p; p++)
		if (!isxdigit (*p))
			break;
	return (*p ? NO : YES);
}


/**
*
* name		rechash --- hash record type name
*
* synopsis	hv = rechash(s)
*		int hv;		hashed value
*		char *s;	string to be hashed
*
* description	Forms a hash value by summing the characters and
*		forming the remainder modulo the record table size.
*
**/
static
rechash(s)
char *s;
{
        int     hashval;

        for( hashval = 0; *s != EOS ; )
                hashval += *s++;
        return( hashval % Rectsize );
}

/**
* name		chkfile - determine if link file is valid
*
* synopsis	chkfile (lib)
*		int lib;	YES / NO if file is library or not
*
* description 	Checks the first character in the current file to insure file
*		is either a valid link file or a library.  Exits with an
*		error if file is not valid.
*
**/
chkfile (lib)
int lib;
{
	int fchar;		/* first character in file */
#if VMS
	long ftell ();
#endif

	if ( (fchar = fgetc (Fd)) == EOF ||	/* empty file */
#ifdef VAXCKLUDGE		/* kludge for VMS 4.6 VAX C RTL */
	     fseek (Fd, ftell (Fd) - 1L, SEEK_SET) != 0 ||
#else
	     ungetc (fchar, Fd) == EOF ||	/* I/O error */
#endif
	     fchar != (lib ? LIBHDR : NEWREC) ) {	/* bad type */
		(void)printf("%s: Invalid command line link file: %s\n",
			Progname, String);
		exit(INITERR);
	}
}

