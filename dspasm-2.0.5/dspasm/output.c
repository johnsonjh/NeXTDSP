#include "dspdef.h"
#include "dspdcl.h"

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
char	*str;
{
#if SASM
	Sasmobj.count = 0;
	Sasmobj.opcode = SASMFTL;
#endif
	(void)sprintf(String,"**** %d [%s %d]: FATAL --- %s",
		Lst_lno,Cfname,Line_num,str);
	outlst(String);
	lst_nl();
	if( Lstfil != stdout || Cnlflag ) {
		(void)printf ("%s\n", String);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n", Cfname, Line_num);
#endif
	}
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

#ifdef BIT48
/**
*
* name		p1error --- error in a line on pass 1
*
* synopsis	p1error(str)
*		char *str;	null terminated error message
*
* description	Error message is output along with the current file
*		name and line number. The error count is incremented.
*
**/
p1error(str)
char	*str;
{
#if SASM
	Sasmobj.count = 0;
	Sasmobj.opcode = SASMERR;
#endif
	(void)sprintf(String,"**** %d [%s %d]: ERROR --- %s",
		Lst_lno,Cfname,Line_num,str);
	outlst(String);
	lst_nl();
	if( Lstfil != stdout || Cnlflag ) {
		(void)printf ("%s\n", String);
#if MPW || AZTEC
		(void)printf ("     File '%s' ; line %d\n", Cfname, Line_num);
#endif
	}
	Err_count++;
	Line_err++;
}
#endif

/**
*
* name		error --- error in a line
*
* synopsis	error(str)
*		char *str;	null terminated error message
*
* description	The error message, line number of the error, and,
*		if the number of files processed is greater than one,
*		the filename is output. Errors are ignored during
*		pass 1.
*
**/
error(str)
char	*str;
{
	register char *p;
	int fldno;

#if SASM
	Sasmobj.count = 0;
	Sasmobj.opcode = SASMERR;
#endif
	if( Gagerrs ){ /* if errors are inhibited */
		++Gagcnt;
		return;
		}
#if !PASM
	if( Pass == 2 ){		/* skip errors on pass 1 */
#endif
		(void)sprintf(String,"**** %d [%s %d]: ERROR --- %s",
			Lst_lno,Cfname,Line_num,str);
		if (Optr && (fldno = err_field (Optr)) != 0) {
			p = String + strlen (String);
			if (Chkdo)
				(void)sprintf (p, " (Initial loop instruction)");
			else
				(void)sprintf (p, " (%s field)", Fldmsg[fldno]);
		}
		outlst(String);
		lst_nl();
		if( Lstfil != stdout || Cnlflag ) {
			(void)printf ("%s\n", String);
#if MPW || AZTEC
			(void)printf ("     File '%s' ; line %d\n",
				Cfname, Line_num);
#endif
		}
		Err_count++;
		Line_err++;
#if !PASM
		}
#endif
}

/**
*
* name		error2 --- error in a line with parameter
*
* synopsis	error2(str,param)
*		char *str;	null terminated error message
*		char *param;	null terminated parameter
*
* description	The error message, line number of the error, and,
*		if the number of files processed is greater than one,
*		the filename are all output. Errors are ignored during
*		pass 1. Param is output following str, separated by
*		a colon.
*
**/
error2(str,param)
char	*str,*param;
{
	register char *p;
	int fldno;

#if SASM
	Sasmobj.count = 0;
	Sasmobj.opcode = SASMERR;
#endif
	if( Gagerrs ){ /* if errors are inhibited */
		++Gagcnt;
		return;
		}
#if !PASM
	if( Pass == 2 ){		/* skip errors on pass 1 */
#endif
		(void)sprintf(String,"**** %d [%s %d]: ERROR --- %s: ",
			Lst_lno,Cfname,Line_num,str);
		p = String + strlen (String);
		(void)strncat (p, param, MAXBUF/2);	/* in case param is too big */
		if (Optr && (fldno = err_field (Optr)) != 0) {
			p = String + strlen (String);
			if (Chkdo)
				(void)sprintf (p, " (Initial loop instruction)");
			else
				(void)sprintf (p, " (%s field)", Fldmsg[fldno]);
		}
		outlst(String);
		lst_nl();
		if( Lstfil != stdout || Cnlflag ) {
			(void)printf ("%s\n", String);
#if MPW || AZTEC
			(void)printf ("     File '%s' ; line %d\n",
				Cfname, Line_num);
#endif
		}
		Err_count++;
		Line_err++;
#if !PASM
		}
#endif
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
char	*str;
{
	register char *p;
	int fldno;

#if SASM
	Sasmobj.count = 0;
	Sasmobj.opcode = SASMWRN;
#endif
	if( Gagerrs ) /* if errors are gagged */
		return;
#if !PASM
	if( Pass == 2 && Wrn_flag ){	/* ignore warnings on pass 1 or if flag not set */
#endif
		(void)sprintf(String,"**** %d [%s %d]: WARNING --- %s",
			Lst_lno,Cfname,Line_num,str);
		if (Optr && (fldno = err_field (Optr)) != 0) {
			p = String + strlen (String);
			if (Chkdo)
				(void)sprintf (p, " (Initial loop instruction)");
			else
				(void)sprintf (p, " (%s field)", Fldmsg[fldno]);
		}
		outlst(String);
		lst_nl();
		if( Lstfil != stdout || Cnlflag ) {
			(void)printf ("%s\n", String);
#if MPW || AZTEC
			(void)printf ("     File '%s' ; line %d\n",
				Cfname, Line_num);
#endif
		}
		++Warn_count;
		Line_err++;
#if !PASM
		}
#endif
}

/**
*
* name		msg --- message to listing file
*
* synopsis	msg(str)
*		char *str;	null terminated informative message
*
* description	The message, associated line number and,
*		if the number of files processed is greater than one,
*		the filename are all output. Messages are ignored during
*		pass 1.
*
**/
msg(str)
char	*str;
{
#if !SASM
	if( Gagerrs ) /* if errors are gagged */
		return;
#if !PASM
	if( Pass == 2 ){		/* skip messages on first pass */
#endif
		(void)sprintf(String,"**** %d [%s %d]: %s",
			Lst_lno,Cfname,Line_num,str);
		outlst(String);
		lst_nl();
		if( Lstfil != stdout || Cnlflag ) {
			(void)printf ("%s\n", String);
#if MPW || AZTEC
			(void)printf ("     File '%s' ; line %d\n",
				Cfname, Line_num);
#endif
		}
#if !PASM
		}
#endif
#endif
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
char	*str;
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
char	*str;
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
* name		err_field --- determine parse field where error occured
*
* synopsis	err_field(ptr)
*		char *ptr;	pointer into parsed line
*
* description	Determines whether ptr points into parsed source line.
*		If not, returns zero.  If so, attempts to determine which
*		source field ptr identifies.  Returns the field offset.
*
**/
static
err_field (ptr)
char *ptr;
{
	/* if ptr points outside parsed line buffer, return */
	if (!ptr || ptr < Pline || ptr >= Pline + MAXBUF)
		return (NONE);

	if ( Label != Nullstr &&  ptr >= Label && /* inside Label field */
		ptr < ( Op	!= Nullstr ? Op :
			Operand != Nullstr ? Operand :
			Xmove	!= Nullstr ? Xmove :
			Ymove	!= Nullstr ? Ymove :
			Pline + MAXBUF) )
		return (LBLFIELD);

	if ( Op != Nullstr && ptr >= Op &&	/* inside Opcode field */
		ptr < ( Operand != Nullstr ? Operand :
			Xmove	!= Nullstr ? Xmove :
			Ymove	!= Nullstr ? Ymove :
			Pline + MAXBUF) )
		return (OPCFIELD);
#if !PASM
	if (Swap) {		/* if data move fields swapped, swap back */
		do_swap ();
		Swap = NO;	/* reset flag */
	}
#endif
	if (Operand != Nullstr && ptr >= Operand && /* inside Operand field */
		ptr < ( Xmove != Nullstr ? Xmove :
			Ymove != Nullstr ? Ymove :
			Pline + MAXBUF) )
		return (OPRFIELD);

	if ( Xmove != Nullstr && ptr >= Xmove && /* inside Xmove field */
		ptr < ( Ymove != Nullstr ? Ymove :
			Pline + MAXBUF) )
		return (XMVFIELD);

	if (Ymove != Nullstr && ptr >= Ymove)	/* inside Ymove field */
		return (YMVFIELD);

	return (NONE);
}

/*
 *	print_line --- pretty print input line
 */
void
print_line(ich)
char ich;	/* indicator character */
{
#if !SASM
	int	i;
	char mx = ' ';
	char lc,rc;
	int	fstart;		/* temp field start */
	int	fcstart;	/* folded comment start */
	int	objstart = 0;	/* start of object word */
	char	spc_char(), *strup();

	/* no print on pass 1, if listing inhibited, or list line aborted */
#if !PASM
	if( Pass == 1 )
		return;
#endif
	if( Ilflag || Lflag <=0 || Abortp )
		return;

	if( !Force_prt ){ /* if print line has not been forced */
		/* skip macro expansions */
		if( Macro_exp ){
			mx = MEXPCH;
			if( !Mex_flag )
				return;
			}

		}

	if (*Label && *Op && strlen (Label) > Labwidth) {
		(void)sprintf(String,"%-5u%c%c  ",Lst_lno++,ich,mx);
		outlst(String);
		fstart = 26; /* start of next field */
		if( Pc != Lpc ) /* if load and runtime are not the same */
			fstart += 8;	/* shift everything over */
		if(Cyc_flag)		/* are we cycle counting? */
			fstart += 16;	/* shift everything over */
		outspace(fstart);	/* space to label field */
		outlst(Label);		/* print label */
		lst_nl();		/* new line */
		*Label = EOS;		/* clear label field */
	}

#if !PASM
	(void)sprintf(String,"%-5u%c%c  ",Lst_lno,ich,mx);
	outlst(String);

	fstart = 10; /* start of next field */
	outspace(fstart);

	if( Pc != Lpc ) /* if load and runtime are not the same */
		fstart += 7; /* shift everything over */

	if( Force_fp ){ /* output floating pt values */
		(void)sprintf(String,"%-13.6f",F_value);
		outlst(String);
		}
	else if( Force_int ){ /* output int value */
		objstart = fstart + 1; /* save next object start */
		(void)sprintf(String,"%06lx",P_words[0]&WORDMASK);
		outlst(strup(String));
		}
	else if( P_force ){ /* output pc value */
		objstart = fstart + 8; /* save next object start */
		if( Pc != Lpc ){ /* if load and runtime are not the same */
			lc = spc_char(Loadsp);
			rc = spc_char(Cspace);
			(void)sprintf(String,"%c:%04lx %c:%04lx ",rc,Old_pc&(long)MAXADDR,lc,Old_lpc&(long)MAXADDR);
			outlst(strup(String));
			}
		else{
			rc = spc_char(Cspace);
			(void)sprintf(String,"%c:%04lx ",rc,Old_pc&(long)MAXADDR);
			outlst(strup(String));
			}

		if( P_total){
			(void)sprintf(String,"%06lx",P_words[0]&WORDMASK);
			outlst(strup(String));
			}
		}

	fstart += 16;

	if(Cyc_flag){	/* are we cycle counting? */
		outspace(fstart);
		fstart += 16; /* shift everything over */
		if(Cycles){
			(void)sprintf(String,"[%1d - %8ld]",Cycles,Ctotal);
			outlst(String);
			}
		}
#endif /* PASM */

	if( *Label != EOS){
#if !PASM
		outspace(fstart);
#endif
		outlst(Label);
		}

	if (!Rcom_flag || Fcflag || *Label || *Op || *Operand || *Xmove || *Ymove)
		fstart += Labwidth;
	fcstart = fstart; /* save folded comment start */

	if( *Op != EOS ){
		outspace(fstart);
		outlst(Op);
		}

	if (!Rcom_flag || Fcflag || *Op || *Operand || *Xmove || *Ymove)
		fstart += Opwidth;

	if( *Operand != EOS ){
		outspace(fstart);
		outlst(Operand);
		}

	if (!Rcom_flag || Fcflag || *Operand || *Xmove || *Ymove)
		fstart += Operwidth;

	if( *Xmove != EOS ){
		outspace(fstart);
		outlst(Xmove);
		}

	if (!Rcom_flag || Fcflag || *Xmove || *Ymove)
		fstart += Xwidth;

	if( *Ymove != EOS ){
		outspace(fstart);
		outlst(Ymove);
		}

	if (!Rcom_flag || Fcflag || *Ymove)
		fstart += Ywidth;

	if( *Com != EOS ){
		if( Fcflag ){ /* if comments are supposed to be folded */
			if( P_total <= 1 ){
				lst_nl();
				outspace(fcstart);
				outlst(Com);
				}
			}
		else{
			outspace(fstart);
			outlst(Com);
			}
		}

#if !PASM
	for( i = 1 ; i < P_total ; ++i ){
		lst_nl();
		if (ich == DCEXP) {
			(void)sprintf(String,"     %c   ",ich);
			outlst(String);
		}
		outspace(objstart);
		(void)sprintf(String,"%06lx",P_words[i]&WORDMASK);
		outlst(strup(String));
		if( i == 1 && Fcflag ){ /* handle folded comment lines */
			outspace(fcstart);
			outlst(Com);
			}
		}
#endif
	lst_nl();
#endif /* SASM */
}

/*
 *	print_comm --- pretty print comment or blank line
 */
void
print_comm(ich)
char ich;	/* indicator character */
{
#if !SASM
	char mx = ' ';
	int	fstart;

	/* no print on pass 1, if listing inhibited, or list line aborted */
#if !PASM
	if( Pass == 1 )
		return;
#endif
	if( Ilflag || Lflag <=0 || Abortp )
		return;

	if( *Line == COMM_CHAR && *(Line + 1) == COMM_CHAR )
		return;		/* unreported comment */

	if( !Force_prt ){ /* if print line has not been forced */
		/* skip macro expansions */
		if( Macro_exp ){
			mx = MEXPCH;
			if( Mex_flag == NO )
				return;
			}

		}

#if !PASM
	(void)sprintf(String,"%-5u%c%c",Lst_lno,ich,mx);
	outlst(String);
#endif
	if( *Line != EOS ){
#if !PASM
		fstart = 26; /* start of next field */

		if( Pc != Lpc )		/* if load and runtime are not the same */
			fstart += 7;	/* shift everything over */

		if(Cyc_flag)		/* are we cycle counting? */
			fstart += 16;	/* shift everything over */

		outspace(fstart);
#endif
		outlst(Line);
		}
	lst_nl();
#endif
}

/**
*
* name		header --- print page header
*
**/
static
header()
{
#if !SASM && !PASM
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
	outcol(Testing ? basename(Cfname) : Cfname);
	(void)sprintf(pgstr,"  Page %d\n",Page);
	outcol(pgstr);
	if( Title != NULL ){
		outlst(Title);
		}
	lst_nl();
	if( Subtitle != NULL ){
		outlst(Subtitle);
		}
	lst_nl();
	lst_nl();
#endif
}

/**
*
* name		outcol - output string; check for wrap
*
* synopsis	outcol(str)
*		char *str;	string to be output
*
* description	Outputs the string to the listing.  If the length of the
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

#if !PASM
	len = strlen (str);	/* get length of input string */
	if (len <= Max_col - Start_col + 1 && len + Lst_col > Max_col) {		/* output new line */
		j = Lst_char;	/* remember last character position */
		lst_nl();	/* output newline to listing */
		Lst_char = j;	/* restore character position */
	}
#endif
	outlst (str);		/* output string */
}

/**
*
* name		outlst - output string to listing
*
* synopsis	outlst(str)
*		char *str;	string to be output
*
* description	Outputs the string to the listing. A column count is
*		maintained and if the column count exceeds the printer
*		width, then a newline is output. A line count is maintained,
*		and if the linecount exceeds the maximum printer page length,
*		and if the Formatp flag is set, then the listing is moved to
*		the next page and a header is printed.
*
**/
static
outlst(str)
char *str;
{
	int j;

	if (Cnlflag)		/* compiler flag set */
		return;

#if !SASM && !PASM
	if (Strtlst && Pass == 2 && !Ilflag) {
		Strtlst = NO;
		j = Lst_char;
		top_page ();
		Lst_char = j;
	}
#endif

	while( *str != EOS ){
#if !PASM
		if( Lst_col++ > Max_col && *str != NEWLINE ){
			j = Lst_char;	/* remember last character position */
			lst_nl();	/* output newline to listing */
			Lst_char = j;
			}
#endif
		++Lst_char;
		if( *str == NEWLINE )
			lst_nl();
		else
			(void)fputc(*str,Lstfil);
		++str;
		}
}

/**
*
* name		outdot - output periods to listing
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
* name		outspace - output spaces to listing
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
* name		lst_nl - output newline to listing
*
* description	If listing line count exceeds listing page length, then
*		the listing is moved to the next page. Otherwise, a newline is
*		output to the listing and the line count is incremented.
*
**/
static
lst_nl()
{
	if (Cnlflag)		/* compiler flag set */
		return;

	if( Lst_line >= Max_line )
		page_eject(YES);
	else{
		(void)fputc(NEWLINE,Lstfil);
		++Lst_line;
		Lst_col = 1;
		Lst_char = 1; /* reset listing character */
		}
	left_margin();
}

/**
*
* name		left_margin - adjust left listing margin
*
* description	Outputs required number of blanks to adjust left listing
*		margin.
*
**/
static
left_margin()
{
	while( Lst_col < Start_col ){
		(void)fputc(' ',Lstfil);
		++Lst_col;
		}
}

/**
*
* name		page_eject - move to next output listing page
*
* synopsis	page_eject (newpage)
*		int newpage;		start new page flag
*
* description	The output listing page is ejected by outputting newlines
*		to move to the next listing page. If the Format_flag is
*		set, then a header will be output at the top of the next
*		page.  If the newpage parameter is YES, counters are reset,
*		the page number is incremented, and the top-of-page routine
*		is called.
*
**/
page_eject(newpage)
int newpage;
{
	if (Cnlflag)		/* compiler flag set */
		return;
#if !PASM
	for(; Lst_line <= Lst_p_len ; ++Lst_line )
#endif
		(void)fputc(NEWLINE,Lstfil);
	if (newpage) {
		Lst_col = 1;	/* reset column number */
		Lst_char = 1;	/* reset character position */
		Lst_line = 1;	/* reset line number */
		++Page;		/* increment page # */
#if !PASM
		top_page();	/* do top of page */
#endif
	}
}

/**
*
* name		top_page - format top of source page listing
*
* description	Any blank lines at the top of the page are output and,
*		if the Formflag is set, a listing header is output.
*
**/
static
top_page()
{
	for( Lst_line = 1; Lst_line <= Lst_topm; ++Lst_line)
		(void)fputc(NEWLINE,Lstfil);

	if( Formflag != 0 ){
		header();
		}

}

/**
*
* name		out_prctl - write printer control string to listing file
*
* synopsis	out_prctl (start, len)
*		char *start;	pointer to start of printer control string
*		int len;	length of printer control string
*
* description	This routine sends a print control string to the listing
*		file.  No formatting control is done; the characters in the
*		string are sent as is.
*
**/
out_prctl (start, len)
char *start;
int len;
{
#if !SASM
	register char *p;

	if (!Ilflag)	/* do only if listing not inhibited */
		for (p = start; p < start + len; p++)
			(void)fputc (*p, Lstfil);
#endif
}

/**
*
* name		done_lst - terminate the output listing
*
* description	If the symbol table option is selected, then
*		the Macro, Section, and Symbol tables are output.
*		If the cross reference option is selected, then
*		the symbol cross reference table is output.
*
**/
done_lst()
{
#if !SASM && !PASM
	if (!Loc_flag)
		free_lcl(); /* release storage allocated to local labels */
	(void)sprintf(String,"%-4u Errors\n",Err_count);
	outlst(String);
	if (Lstfil != stdout && !Cflag)
		(void)printf ("%s", String);
	(void)sprintf(String,"%-4u Warnings\n",Warn_count);
	outlst(String);
	if (Lstfil != stdout && !Cflag)
		(void)printf ("%s", String);

	if( Ilflag )		/* if source listing inhibited */
		return;

	if( S_flag ){		/* symbol table option */
		page_eject(YES);/* move to a new page */
		(void)print_defs();	/* print define symbol listing */
		(void)print_macs();	/* print the macro name listing */
		(void)print_secs();	/* print the section name listing */
		}
	(void)print_syms();		/* print symbol table if necessary */
	if( Muflag){		/* memory utilization report option */
		page_eject(YES);/* move to a new page */
		(void)print_mu();	/* print memory utilization report */
	}
	page_eject(NO);		/* force last page out */
	if (Prctl)		/* PRCTL on last line of last file */
		out_prctl (Prctl, Prctl_len);
#endif
}

/*
 *	reset_pflags --- reset force print flags
 */
reset_pflags()
{
	Force_fp =	/* No force of fp unless directed */
	Force_int =	/* No force of ints unless directed */
	Force_prt =	/* No force print unless directed */
	P_force =	/* No force unless bytes emitted */
	Abortp = NO;	/* Init one time stop print */
}

/**
*
* name		print_defs - print define symbol listing
*
* description	Prints a list of the symbols defined in the source listing.
*
**/
static
print_defs()
{
#if !SASM && !PASM
	struct deflist *mp;
	int i,j;
	struct deflist **sort_tab;

	if( Testing ){
		Gagerrs = YES;
		(void)def_rem( "DSPHOST" );	/* remove test defines */
		Gagerrs = NO;
		}

	if( No_defs == 0 )
		return(YES);		/* no defines */

	/* make room for a table to sort them in */
	sort_tab = (struct deflist **)alloc(No_defs * sizeof(struct deflist *));
	if( !sort_tab )
		fatal("Out of memory - cannot sort define symbols");

	/* now fill in the table */
	i = 0;
	for( j = 0 ; j < HASHSIZE ; ++j ){
		for( mp = Dhashtab[j] ; mp!= NULL ; mp = mp->dnext )
			sort_tab[i++] = mp;
		}

	sortdefs(sort_tab,No_defs); /* sort the define symbols	*/

	outlst("Define symbols:\n");
	lst_nl();
	outlst	("Symbol           Definition\n");
	lst_nl();

	for( i = 0 ; i < No_defs ; ++i ){
		String[0] = EOS;
		(void)strncat(String,sort_tab[i]->dname,LSTSYM);
		outlst(String);
		outdot(LSTSYM+2);
		outlst("'");
		outlst(sort_tab[i]->ddef);
		outlst("'\n");
		}

	lst_nl();
	lst_nl();
	free_def();		/* release the definition list */
	free((char *)sort_tab); /* release the sort table */

	return(YES);
#endif /* SASM && PASM */
}

/**
*
* name		sortdefs - sort define symbols
*
**/
static
sortdefs(array,no_defs)
struct deflist **array;
int no_defs;
{
	int cmp_defs ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_defs;
	psort(0,no_defs - 1);
}

/**
*
* name		cmp_defs - compare define symbol names
*
**/
static
cmp_defs (s1, s2)
struct deflist *s1, *s2;
{
	int cmp;

	if ((cmp = strcmp (s1->dname, s2->dname)) != 0)
		return (cmp);
	return (strcmp (s1->ddef, s2->ddef));
}

/**
*
* name		print_macs - print macro name listing
*
* description	Prints a list of the macro's defined in the source listing.
*
**/
static
print_macs()
{
#if !SASM && !PASM
	struct mac *mp;
	int i;
	int no_macros = 0;
	struct mac **sort_tab;

	if( Macro == NULL )
		return(YES);	/* no macros */

	/* count the number of macros and erase their definitions */
	for( mp = Macro ; mp != NULL ; mp = mp->next ){
		++no_macros;
		if( mp->mdef != NULL ){
			free_md(mp->mdef);
			mp->mdef = NULL;
			}
		}
	/* make room for a table to sort them in */
	sort_tab = (struct mac **)alloc(no_macros * sizeof(struct mac *));
	if( sort_tab == NULL )
		fatal("Out of memory - cannot sort macros");

	/* now fill in the table */
	i = 0;
	for( mp = Macro ; mp!= NULL ; mp = mp->next )
		sort_tab[i++] = mp;

	sortmacs(sort_tab,no_macros); /* sort the macro names */

	outlst("Macros:\n");
	lst_nl();
	outlst("Name          Definition       Section\n");
	outlst("                 Line\n");
	lst_nl();

	for( i = 0 ; i < no_macros ; ++i ){
		String[0] = EOS;
		(void)strncat(String,sort_tab[i]->mname,LSTSYM);
		outlst(String);
		outdot(LSTSYM+2);
		(void)sprintf(String,"%-6u",sort_tab[i]->def_line);
		outlst(String);
		if (sort_tab[i]->msect != &Glbsec){
			outspace(LSTSYM+16);
			(void)sprintf(String,"%s",sort_tab[i]->msect->sname);
			outlst(String);
			}
		lst_nl();
		}

	lst_nl();
	lst_nl();
	free_mac();		/* release the macro definition list */
	free((char *)sort_tab); /* release the sort table */

	return(YES);
#endif /* SASM && PASM */
}

/**
*
* name		sortmacs - sort macro names
*
**/
static
sortmacs(array,no_macs)
struct mac **array;
int no_macs;
{
	int cmp_macs ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_macs;
	psort(0,no_macs - 1);
}

/**
*
* name		cmp_macs - compare macro names
*
**/
static
cmp_macs (s1, s2)
struct mac *s1, *s2;
{
	int cmp;

	if ((cmp = strcmp (s1->mname, s2->mname)) != 0)
		return (cmp);
	return (s1->def_line - s2->def_line);
}

/**
*
* name		print_secs - print relocatable section name listing
*
* description	Prints a list of the sections defined in the source listing.
*
**/
static
print_secs()
{
#if !SASM && !PASM
	struct slist *mp;
	int i;
	int no_secs = 0;
	struct slist **sort_tab;

	if( Sect_lst->next == NULL )
		return(YES);	/* no sections */

	/* count the number of sections */
	for( mp = Sect_lst->next ; mp ; mp = mp->next )
		++no_secs;

	/* make room for a table to sort them in */
	sort_tab = (struct slist **)alloc(no_secs * sizeof(struct slist *));
	if( sort_tab == NULL )
		fatal("Out of memory - cannot sort section names");

	/* now fill in the table */
	i = 0;
	for( mp = Sect_lst->next ; mp ; mp = mp->next )
		sort_tab[i++] = mp;

	sortsecs(sort_tab,no_secs); /* sort the section */

	outlst("Relocatable Sections:\n");
	lst_nl();
	outlst("Name\n");
	lst_nl();

	for( i = 0 ; i < no_secs ; ++i ){
		outlst(sort_tab[i]->sname);
		lst_nl();
		}

	lst_nl();
	lst_nl();
	free((char *)sort_tab); /* release the sort table */
	return(YES);
#endif /* SASM && PASM */
}

/**
*
* name		sortsecs - sort section names
*
**/
static
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
* name		cmp_secs - compare section names
*
**/
static
cmp_secs (s1, s2)
struct slist *s1, *s2;
{
	return (strcmp (s1->sname, s2->sname));
}

/**
*
* name		print_syms - print symbol tables
*
* description	If option flags are set, prints a symbol table and
*		symbol cross reference table.
*
**/
static
print_syms()
{
#if !SASM && !PASM
	struct nlist *mp;
	struct symlist *sp;
	int i,j,satt,symcnt;
	struct nlist **sort_tab;
	struct xlist *xs;
	char *strup();

	if( S_flag == NO && Cre_flag == NO )
		return(YES);	/* no symbol table options */
	symcnt = Sym_cnt + (Loc_flag ? Lsym_cnt : 0);
	if( symcnt == 0 ){
		outlst("No symbols used\n");
		return(YES);
		}

	/* if sort table exists, just assign local pointer */
	if (Sym_sort)
		sort_tab = Sym_sort;
	else {
		/* make room for table to sort symbols in */
		sort_tab = (struct nlist **)alloc(symcnt * sizeof(struct nlist *));
		if( !sort_tab )
			fatal("Out of memory - cannot sort symbols");

		/* now fill in the table */
		i = 0;
		for( j = 0 ; j < HASHSIZE ; ++j )
			for( mp = Hashtab[j] ; mp ; mp = mp->next )
				if ((mp->attrib & LOCAL) && !Loc_flag)
					continue;
				else
					sort_tab[i++] = mp;
		/* if doing local labels, get them from local label list */
		if (Loc_flag)
			for (sp = Loclab; sp; sp = sp->next)
				for (mp = sp->sym; mp; mp = mp->next)
					sort_tab[i++] = mp;

		sortsyms(sort_tab,symcnt);	/* sort the symbols */
		Sym_sort = sort_tab;		/* assign global pointer */
	}

	if( S_flag ){ /* symbol table option */
		outlst("Symbols:\n");
		lst_nl();
		outlst("Name             Type    Value         Section           Attributes\n");
		lst_nl();

		for( i = 0 ; i < symcnt ; ++i ){
			satt = sort_tab[i]->attrib;
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
				(void)sprintf(String,"%-13.6f",
					sort_tab[i]->def.fdef);
			else if( satt & INT )
				if( satt & LONG )
					(void)sprintf(String,"%06lx%06lx",
						sort_tab[i]->def.xdef.high,
						sort_tab[i]->def.xdef.low);
				else
					(void)sprintf(String,"%06lx",
						sort_tab[i]->def.xdef.low);
			outlst(strup(String));
			if( sort_tab[i]->sec != &Glbsec ){
				outspace(LSTSYM+24);
				String[0] = EOS;
				(void)strncat(String, sort_tab[i]->sec->sname, LSTSYM);
				outlst(String);
				}
			if( satt & SET ){
				outspace(LSTSYM+LSTSYM+26);
				outlst("SET");
				}
			if( satt & GLOBAL ){
				outspace(LSTSYM+LSTSYM+26);
				outlst("GLOBAL");
				}
			if( satt & LOCAL ){
				outspace(LSTSYM+LSTSYM+26);
				outlst("LOCAL");
				}
			lst_nl();
			}
		lst_nl();
		lst_nl();
		}

	if( Cre_flag ){ /* cross reference listing */
		outlst("Symbol cross-reference listing:\n");
		lst_nl();
		outlst("Name             Line number (* is definition)\n");
		for( i = 0 ; i < symcnt ; ++i ){
			String[0] = EOS;
			(void)strncat (String, sort_tab[i]->name, LSTSYM);
			outlst(String);
			outdot(LSTSYM+2);
			for( xs = sort_tab[i]->xrf ; xs ; xs = xs->next ){
				if( Lst_col + 8 > Max_col ){
					lst_nl();
					outspace(Lst_char + LSTSYM + 1);
					}
				j = Lst_char;
				(void)sprintf(String,"%5u",xs->lnum);
				outlst(String);
				if( xs->rtype == DEFTYPE )
					outlst("*");
				outspace(j + 8);
				}
			lst_nl();
			}
		lst_nl();
		lst_nl();
		}

	if (So_flag == NO) {
		free((char *)sort_tab); /* release the sort table */
		Sym_sort = NULL;
	}
	return(YES);
#endif /* SASM && PASM */
}

#if DEBUG
/**
*
* name		dumphash - dump hash table
*
**/
static
dumphash ()
{
	int j;
	struct nlist *mp;
	struct xlist *xs;

	(void)fprintf (stderr, "\nDumping hash table:\n");
	for( j = 0 ; j < HASHSIZE ; ++j )
		if (!Hashtab[j])
			(void)fprintf (stderr, "\tHashtab[%6d]: NULL\n", j);
		else {
			(void)fprintf (stderr, "\tHashtab[%6d]:\n", j);
			for( mp = Hashtab[j] ; mp ; mp = mp->next ) {
				(void)fprintf (stderr, "\t\tptr = %8lx, name = %s\n", mp, mp->name);
				for( xs = mp->xrf ; xs ; xs = xs->next )
					(void)fprintf (stderr, "\t\t\tptr = %8lx, line = %6d\n", xs, xs->lnum);
			}
		}
}
#endif

/**
*
* name		sortsyms - sort symbol names
*
**/
static
sortsyms(array,no_syms)
struct nlist **array;
int no_syms;
{
	int cmp_syms ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_syms;
	psort(0,no_syms - 1);
}

/**
*
* name		cmp_syms - compare symbol names
*
**/
static
cmp_syms (s1, s2)
struct nlist *s1, *s2;
{
#if !SASM && !PASM
	int cmp;
	double f1, f2, fdiff, vitod();

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	/* order by section */
	if (s1->sec != s2->sec)
		return (s1->sec->secno - s2->sec->secno);

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
#endif
}

/**
*
* name		sortosyms - sort object symbol names
*
**/
static
sortosyms(array,no_syms)
struct nlist **array;
int no_syms;
{
	int cmp_osyms ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_osyms;
	psort(0,no_syms - 1);
}

/**
*
* name		cmp_osyms - compare object symbol names
*
**/
static
cmp_osyms (s1, s2)
struct nlist *s1, *s2;
{
#if !SASM && !PASM
	int cmp;

	/* order by section */
	if (s1->sec != s2->sec)
		return (s1->sec->secno - s2->sec->secno);

	/* order by memory space */
	if ((s1->attrib & MSPCMASK) != (s2->attrib & MSPCMASK))
		return ((s1->attrib & MSPCMASK) < (s2->attrib & MSPCMASK) ?
			-1 : 1);

	/* order by counter type */
	if ((s1->mapping & MCTRMASK) != (s2->mapping & MCTRMASK))
		return ((s1->mapping & MCTRMASK) < (s2->mapping & MCTRMASK) ?
			-1 : 1);

	/* order by memory mapping */
	if ((s1->mapping & MMAPMASK) != (s2->mapping & MMAPMASK))
		return ((s1->mapping & MMAPMASK) < (s2->mapping & MMAPMASK) ?
			-1 : 1);

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	return (0);
#endif
}

/**
*
* name		do_mu - update memory utilization records
*
* synopsis	do_mu (type, size)
*		unsigned type;	definition type
*		unsigned size;	size of memory block
*
* description	Update the memory utilization records if memory
*		utilization reporting is active.  If there are no
*		records yet, or the type has changed or an ORG
*		directive was processed since the last call,
*		call init_mu to create a new set of records.
*
**/
do_mu (type, size)
unsigned type, size;
{
#if !SASM && !PASM
	static curr_type = NONE;

	if (!Muflag)			/* reporting not active */
		return;

	if ((!Rmu && !Lmu) || Mu_org || type != curr_type ||
	    (*Label && type > MUCODE)) {
		init_mu (type);		/* create new record */
		curr_type = type;	/* update current type */
		Mu_org = NO;		/* turn off ORG flag */
	}

	Rmu->length += (long)size;	/* increment length field in rec */
	if (Pc != Lpc)			/* overlay in progress */
		Lmu->length += (long)size;	/* update load record */
#endif
}

/**
*
* name		init_mu - allocate and intialize memory utilization records
*
* synopsis	init_mu (type)
*		unsigned type;	definition type
*
* description	Allocate and initialize a memory utilization record.
*		If an overlay is in progress, allocate and initialize
*		a load time record.  Link the records into the memory
*		utilization chain.
*
**/
static
init_mu (type)
unsigned type;
{
#if !SASM && !PASM
	struct murec *rmu;	/* pointer to runtime memory util. record */
	struct murec *lmu;	/* pointer to load memory util. record */
	int sym_ok;		/* label check flag */
	struct nlist *sp = NULL;/* pointer to symbol table entry */
	char label[MAXSYM+1];	/* label buffer */

	if ((rmu = (struct murec *)alloc (sizeof (struct murec))) == NULL)
		fatal ("Out of memory - cannot allocate memory utilization record");
	rmu->flags = Cspace|type;	/* current mem space and def type */
	rmu->start = Old_pc;		/* old program counter value */
	rmu->length = 0L;		/* initialize length */
	if (*Label) {			/* statement has label */
		Gagerrs = YES;		/* turn off error reporting */
		sym_ok = get_i_sym (Label);	/* check label name */
		Gagerrs = NO;		/* restore previous setting */
		if (sym_ok == YES) {	/* label OK; look for it in table */
			(void)strcpy (label, Label);
			if (Icflag)	/* ignore case */
				(void)strdn (label);
			for (sp = Hashtab[hash (label)]; sp; sp = sp->next)
				if (STREQ (label, sp->name) )
					break;
		}
	}
	rmu->lab = sp;			/* pointer to label record */
	rmu->sec = Csect;		/* save section pointer */
	if (Pc == Lpc) {		/* no overlay active */
		rmu->ov = NULL;
		rmu->next = Rmu;	/* link record into chain */
		Rmu = rmu;
		return;
	}

/*
	if we got here, an overlay is active; alloc. and init. load record
*/

	if ((lmu = (struct murec *)alloc (sizeof (struct murec))) == NULL)
		fatal ("Out of memory - cannot allocate memory utilization record");
	lmu->flags = MUOVRLY|Loadsp|type; /* current mem space and def type */
	lmu->start = Old_lpc;		/* old load counter value */
	lmu->length = 0L;		/* initialize length */
	lmu->lab = sp;			/* pointer to label record */
	lmu->sec = Csect;		/* save section pointer */
	rmu->ov = lmu;			/* point records to each other */
	lmu->ov = rmu;
	rmu->next = Rmu;		/* link records into chain */
	Rmu = rmu;
	lmu->next = Lmu;
	Lmu = lmu;
#endif /* SASM && PASM */
}

/**
*
* name		print_mu - print memory utilization report
*
* description	If option flag is set, prints a memory utilization
*		report sorted by memory space and starting address
*
**/
static
print_mu()
{
#if !SASM && !PASM
	int count = 0;
	struct murec *mu, **mp;
	struct murec **sort_tab;
	int mem;
	long nxtaddr;
	struct murec unused;

	if( !Muflag )
		return(YES);	/* no report requested */

	/* count all the records */
	for (mu = Rmu; mu; mu = mu->next)
		count++;
	for (mu = Lmu; mu; mu = mu->next)
		count++;

	/* allocate sort table, fill it in, sort the records */
	sort_tab = (struct murec **)alloc((count + 1) * sizeof(struct murec *));
	if( !sort_tab )
		fatal("Out of memory - cannot sort symbols");
	if (count) {			/* records to sort */
		mp = sort_tab;		/* point to sort table */
		for (mu = Rmu; mu; mu = mu->next)
			*mp++ = mu;	/* fill in runtime records */
		for (mu = Lmu; mu; mu = mu->next)
			*mp++ = mu;	/* fill in load records */
		sortmu(sort_tab,count); /* sort the records */
	}
	sort_tab[count] = NULL; /* mark end of table */

	/* print the report */

	mp = sort_tab;			/* reset pointer to sort table */
	outlst("                         Memory Utilization Report\n");
	for (mem = XSPACE; mem <= PSPACE; mem <<= 1) {
		lst_nl();
		lst_nl();
		switch (mem) {
			case XSPACE:	outlst ("X"); break;
			case YSPACE:	outlst ("Y"); break;
			case LSPACE:	outlst ("L"); break;
			case PSPACE:	outlst ("P"); break;
		}
		outlst (" Memory\n");
		lst_nl();
		outlst("Start    End     Length    Type      Label             Section           Overlay Address\n");
		nxtaddr = 0L;		/* initialize next address */
		while (*mp && ((*mp)->flags & MSPCMASK) == mem) {
			mu = *mp++;		/* dereference pointer */
			if (mu->length == 0L)	/* empty record */
				continue;
			if (nxtaddr < mu->start) {	/* unused space */
				unused.flags = NONE;
				unused.start = nxtaddr;
				unused.length = mu->start - nxtaddr;
				mu_line (&unused);	/* print unused line */
				nxtaddr += unused.length;
			}
			mu_line (mu);		/* print line */
			nxtaddr += mu->length;	/* recompute next address */
		}
		if (nxtaddr < MAXADDR) {	/* unused space */
			unused.flags = NONE;
			unused.start = nxtaddr; /* save end addr. */
			unused.length = (long)(MAXADDR + 1) - nxtaddr;
			mu_line (&unused);	/* print unused line */
		}
	}

	/* free all memory utilization records */
	for (mu = Rmu; mu; mu = mu->next)
		free ((char *)mu);
	for (mu = Lmu; mu; mu = mu->next)
		free ((char *)mu);
	free((char *)sort_tab);		/* release the sort table */
	return(YES);
#endif /* SASM && PASM */
}

/**
*
* name		mu_line - print memory utilization report line
*
* description	Given a pointer to a memory utilization record,
*		prints the line item information to the listing file
*
**/
static
mu_line (mu)
struct murec *mu;
{
#if !SASM && !PASM
	long endaddr;
	char *strup ();

	endaddr = (mu->start + mu->length) - 1L;
	if ((mu->flags & MUTPMASK) == NONE) {
		(void)sprintf (String, "%04lx     %04lx    %5ld     UNUSED\n",
			mu->start, endaddr, mu->length);
		outlst (strup(String));
	} else {
		(void)sprintf (String, "%04lx     %04lx    %5ld     ",
			mu->start, endaddr, mu->length);
		outlst (strup(String));
		switch (mu->flags & MUTPMASK) {
			case MUCODE:	outlst ("CODE  "); break;
			case MUDATA:	outlst ("DATA  "); break;
			case MUCONST:	outlst ("CONST "); break;
			case MUMOD:	outlst ("MOD   "); break;
			case MUREV:	outlst ("REV   "); break;
		}
		if (mu->lab) {		/* label present */
			outspace (38);
			String[0] = EOS;
			(void)strncat (String, mu->lab->name, LSTSYM);
			outlst (String);
		}
		if (mu->sec != &Glbsec) {	/* section present */
			outspace (40+LSTSYM);
			String[0] = EOS;
			(void)strncat (String, mu->sec->sname, LSTSYM);
			outlst (String);
		}
		if (mu->ov) {		/* overlay record present */
			outspace (42+LSTSYM+LSTSYM);
			switch (mu->ov->flags&MSPCMASK) {
				case XSPACE:	outlst ("X"); break;
				case YSPACE:	outlst ("Y"); break;
				case LSPACE:	outlst ("L"); break;
				case PSPACE:	outlst ("P"); break;
			}
			(void)sprintf (String, ":%04lx(%c)",
				mu->ov->start,
				mu->ov->flags & MUOVRLY ? 'L' : 'R');
			outlst (strup(String));
		}
		outlst ("\n");
	}
#endif /* SASM && PASM */
}

/**
*
* name		sortmu - sort memory utilization records
*
**/
static
sortmu(array,no_mu)
struct murec **array;
int no_mu;
{
	int cmp_mu ();

	Sort_tab = (char **)array;
	Cmp_rtn = cmp_mu;
	psort(0,no_mu - 1);
}

/**
*
* name		cmp_mu - compare memory utilization records
*
**/
static
cmp_mu (s1, s2)
struct murec *s1, *s2;
{
	long diff;

	if ((s1->flags&MSPCMASK) != (s2->flags&MSPCMASK))
		return ((int)((s1->flags&MSPCMASK) - (s2->flags&MSPCMASK)));
	diff = s1->start - s2->start;
	if (diff < 0L)
		return (-1);
	else if (diff > 0L)
		return (1);
	diff = s2->length - s1->length; /* do length descending */
	if (diff < 0L)
		return (-1);
	else if (diff > 0L)
		return (1);
	return (0);
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
#if !SASM && !PASM
	char *strup();

	if (Objfil) {
		Obj_rtype = OBJSTART;
		(void)fprintf (Objfil, "_START %s ", Mod_name);
		if (Lnkmode) {
			Hexstr = "$";	/* set up for hex expressions */
			Lngstr = Nullstr; /* set up for long expressions */
		}
		if (Lnkmode)
			(void)sprintf (Objbuf, "%04x %04x %04x",
				Mod_vers, Mod_rev, Err_count);
		else
			(void)sprintf (Objbuf, "%04x %04x",
				Mod_vers, Mod_rev);
		(void)fprintf (Objfil, "%s\n", strup (Objbuf));
		Objbuf[0] = EOS;
		if (Mod_com) {		/* module header comment */
			if (strlen (Mod_com) > MAXOBJLINE)
				*(Mod_com + MAXOBJLINE) = EOS;
			(void)fprintf (Objfil, "%s", Mod_com);
		}
		if (Lnkmode) {		/* relative mode */
			sect_obj ();	/* write out section records */
			sym_obj ();	/* write out symbol records */
		}
	}
#endif
}

/**
*
* name		done_obj - terminate the object file
*
* description	Write the symbol table or SYMOBJ list info to the
*		object file if required.  If an OMF record has been
*		generated, write the object file buffer and append a
*		newline to it.	Write out the OMF END record.
*
**/
done_obj ()
{
#if !SASM && !PASM
	char *strup();

	if (Objfil) {
		if (!Lnkmode)		/* absolute mode */
			sym_obj ();	/* write out symbols */
		else
			xrf_obj ();	/* write out external references */
		if (Obj_rtype != OBJNULL) {
			(void)fprintf (Objfil, "%s\n", Objbuf);
			if (End_addr < 0L)
				(void)sprintf (Objbuf, "_END");
			else if (End_rel)
				(void)sprintf (Objbuf, "_END %s", Expr);
			else {
				(void)sprintf (Objbuf, "_END %s%04lx",
					Hexstr, End_addr & (long)MAXADDR);
				(void)strup (Objbuf);
			}
			(void)fprintf (Objfil, "%s\n", Objbuf);
		}
	}
#endif
}

/**
*
* name		emit_data --- emit a data record to code file
*
* description	Emits data record to code file.
*
**/
emit_data (val, str)
long val[];
char *str;
{
#if !SASM && !PASM
	static space = NONE;
	static cntr = DEFAULT;
	static map = NONE;
	static long pc = 0L;
	static char *bp = Objbuf;
	char *strup();

	if(Pass==1){
		++*Pc;		/* increment program counters */
		if( Pc != Lpc )
			if( Lbflag )
				*Lpc += (long)BYTEWORD;
			else
				++*Lpc;
		(void)chklc ();
		return;
		}

	val[HWORD] &= WORDMASK; /* mask off high bits */
	val[LWORD] &= WORDMASK; /* mask off high bits */
	if (Loadsp == LSPACE && Lwflag) /* save value for listing */
		save_pword (val[HWORD]);
	save_pword (val[LWORD]);

	if(Objfil) { /* if there is an object file */
		/* new record type, data space, or address */
		if ((Obj_rtype != OBJDATA && Obj_rtype != OBJRDATA) ||
		    space != Loadsp || cntr != Lcntr || map != Lmap ||
		    pc != *Lpc) {
			(void)fprintf (Objfil, "%s\n", Objbuf);
			Obj_rtype = str ? OBJDATA : OBJRDATA;
			space = Loadsp;
			cntr = Lcntr;
			map = Lmap;
			pc = *Lpc;
			bp = Objbuf;
			(void)strcpy (bp, "_DATA ");
			bp += strlen(bp);
			if (Lnkmode) {	/* relative mode */
				(void)sprintf (bp, "%04x ", Csect->secno);
				bp += strlen(bp);
			}
			*bp++ = spc_char (space);
			if (Lnkmode && cntr != DEFAULT)
				*bp++ = ctr_char (cntr);
			if (map)
				*bp++ = map_char (map);
			(void)sprintf (bp, " %04lx", pc & (long)MAXADDR);
			(void)fprintf (Objfil, "%s\n", strup (Objbuf));
			bp = Objbuf;	/* reset buffer pointer */
		/* need to start new line */
		} else if ( (!str && bp + MAXVALSIZE + 1 > Objbuf + MAXOBJLINE) ||
			    (str && bp + strlen (str) + 1 > Objbuf + MAXOBJLINE) ) {
			(void)fprintf (Objfil, "%s\n", Objbuf);
			bp = Objbuf;	/* reset buffer pointer */
		}
		if (!str) {
			if (Loadsp == LSPACE && Lwflag)
				(void)sprintf (bp, "%s%06lx%s%06lx ",
					       Hexstr, val[HWORD],
					       Lngstr, val[LWORD]);
			else
				(void)sprintf (bp, "%s%06lx ",
					       Hexstr, val[LWORD]);
			(void)strup (bp);
		} else
			(void)sprintf (bp, "%s ", str);
		bp += strlen (bp);
		if (Pstart && Cspace == PSPACE) { /* save first instr addr */
			End_addr = *Pc;
			Pstart = NO;
		}
		pc++;	/* increment object program counter */
	}

	++*Pc;		/* increment program counters */
	if( Pc != Lpc )
		if( Lbflag )
			*Lpc += (long)BYTEWORD;
		else
			++*Lpc;
	(void)chklc ();		/* check for overflow */
	Emit = YES;		/* set emit flag */
#endif /* SASM && PASM */
}

/**
*
* name		emit_bdata --- emit block data record to code file
*
* description	Emits a block data record to the object file.
*
**/
emit_bdata (count, val, str, ds)
long count, val[];
char *str;
int ds;
{
#if !SASM && !PASM
	char *bp, *sp, *strup(), spc_char();

	if(Pass==1){
		*Pc += count;	/* increment program counters */
		if( Pc != Lpc )
			if( Lbflag )
				*Lpc += count * (long)BYTEWORD;
			else
				*Lpc += count;
		(void)chklc ();
		return;
		}

	if (!ds) {		/* no value if DS directive */
		val[HWORD] &= WORDMASK; /* mask off high bits */
		val[LWORD] &= WORDMASK; /* mask off high bits */
		if (Loadsp == LSPACE && Lwflag) /* save value for listing */
			save_pword (val[HWORD]);
		save_pword (val[LWORD]);
	}

	if (Objfil) { /* if there is an object file */
		(void)fprintf (Objfil, "%s\n", Objbuf);
		Obj_rtype = str ? OBJBLKDATA : OBJRBLKDATA;
		bp = Objbuf;
		(void)strcpy (bp, "_BLOCKDATA ");
		bp += strlen(bp);
		if (Lnkmode) {	/* relative mode */
			(void)sprintf (bp, "%04x ", Csect->secno);
			bp += strlen(bp);
		}
		sp = bp;	/* save buffer pointer */
		*bp++ = spc_char (Loadsp);
		if (Lnkmode && Lcntr != DEFAULT)
			*bp++ = ctr_char (Lcntr);
		if (Lmap)
			*bp++ = map_char (Lmap);
		(void)sprintf (bp, " %04lx %04lx ", *Lpc, count);
		(void)strup (Objbuf);
		bp += strlen(bp);
		if (!ds)	/* don't generate value if ds directive */
			if (!str) {
				if (Loadsp == LSPACE && Lwflag)
					if (Lnkmode)
						(void)sprintf (bp, "%s%06lx%s%06lx", Hexstr, val[HWORD], Lngstr, val[LWORD]);
					else {
						*sp = 'X';
						(void)sprintf (bp, "%s%06lx",
							       Hexstr,
							       val[HWORD]);
						(void)fprintf (Objfil, "%s\n",
							       Objbuf);
						*sp = 'Y';
						(void)sprintf (bp, "%s%06lx",
							       Hexstr,
							       val[LWORD]);
					}
				else
					(void)sprintf (bp, "%s%06lx",
						       Hexstr, val[LWORD]);
				(void)strup (bp);
			} else
				(void)sprintf (bp, "%s ", str);
		if (Pstart && Cspace == PSPACE) { /* save first instr addr */
			End_addr = *Pc;
			Pstart = NO;
		}
	}

	*Pc += count;	/* increment program counters */
	if( Pc != Lpc )
		if( Lbflag )
			*Lpc += count * (long)BYTEWORD;
		else
			*Lpc += count;
	(void)chklc ();		/* check for overflow */
	Emit = YES;		/* set emit flag */
#endif /* SASM && PASM */
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
#if !SASM && !PASM
	char *strup();

	if (Objfil) { /* if there is an object file */
		Obj_rtype = OBJCOMMENT;
		(void)fprintf (Objfil, "%s\n", Objbuf);
		(void)fprintf (Objfil, "_COMMENT\n");
		(void)fprintf (Objfil, "%s", str);
		Objbuf[0] = EOS;
	}
#endif
}

/**
*
* name		save_pword - save word in print array
*
**/
save_pword(word)
long word;
{
	word &= WORDMASK;	/* mask off high bits */
	if(P_total < P_LIMIT)	/* save word for listing */
		P_words[P_total++] = word;
	P_force = YES;		/* force pc output */
}

/**
*
* name		sect_obj - write section information to object file
*
* description	Scan the section list and write out section records
*		in order.
*
**/
static
sect_obj ()
{
#if !SASM && !PASM
	struct slist *sl;
	char spc_char (), *strup();

	if (!Objfil)		/* no object file */
		return;

	Obj_rtype = OBJSECTION;		/* set new record type */
	for (sl = Sect_lst; sl; sl = sl->next) { /* loop through sections */
		(void)fprintf (Objfil, "%s\n", Objbuf);
		(void)fprintf (Objfil, "_SECTION %s %c ",
			sl->sname, sl->flags & REL ? 'R' : 'A');
		(void)sprintf (Objbuf, "%04x", sl->secno);
		(void)fprintf (Objfil, "%s\n", strup (Objbuf));
		(void)sprintf (Objbuf, "%04lx %04lx %04lx %04lx %04lx %04lx",
			sl->cntrs[XMEM-1][DCNTR][RBANK],
			sl->cntrs[XMEM-1][LCNTR][RBANK],
			sl->cntrs[XMEM-1][HCNTR][RBANK],
			sl->cntrs[YMEM-1][DCNTR][RBANK],
			sl->cntrs[YMEM-1][LCNTR][RBANK],
			sl->cntrs[YMEM-1][HCNTR][RBANK]);
		(void)fprintf (Objfil, "%s\n", strup (Objbuf));
		(void)sprintf (Objbuf, "%04lx %04lx %04lx %04lx %04lx %04lx",
			sl->cntrs[LMEM-1][DCNTR][RBANK],
			sl->cntrs[LMEM-1][LCNTR][RBANK],
			sl->cntrs[LMEM-1][HCNTR][RBANK],
			sl->cntrs[PMEM-1][DCNTR][RBANK],
			sl->cntrs[PMEM-1][LCNTR][RBANK],
			sl->cntrs[PMEM-1][HCNTR][RBANK]);
		(void)strup (Objbuf);
	}
#endif
}

/**
*
* name		sym_obj - write symbol information to object file
*
* description	Sort the symbol table or list and write an OMF SYMBOL record
*		for each section and memory space grouping.
*
**/
static
sym_obj ()
{
#if !SASM && !PASM
	int i,j,k,l,m,secno,mem,ctr,map,symcnt=0;
	struct nlist **sort_tab, *mp;
	struct symlist *so;
	char *bp, spc_char (), *strup();

	if (!Objfil)		/* no object file */
		return;

	if (So_flag || Lnkmode) /* get symbol table count */
		symcnt = Sym_cnt;
	else if (So_cnt)	/* get symbol list count */
		symcnt = So_cnt;
	if (symcnt == 0)	/* no symbols */
		return;

	/* make room for table to sort symbols in */
	sort_tab = (struct nlist **)alloc(symcnt * sizeof(struct nlist *));
	if( !sort_tab )
		fatal("Out of memory - cannot sort symbols");

	/* now fill in the table */
	i = 0;
	if (So_flag || Lnkmode) {
		for( j = 0 ; j < HASHSIZE ; j++ )
			for( mp = Hashtab[j] ; mp ; mp = mp->next )
				sort_tab[i++] = mp;
	} else
		for( j = 0, so = Sym_list; j < So_cnt; j++ ) {
			sort_tab[i++] = so->sym;
			so = so->next;
			}

	if (Lnkmode)
		sortosyms(sort_tab,symcnt);	/* sort the symbols */
	else
		sortsyms(sort_tab,symcnt);

	j = k = l = m = -1;
	Obj_rtype = OBJSYMBOL;	/* set new record type */
	for (i = 0; i < symcnt; i++) {
		mp = sort_tab[i];
		if (mp->attrib & SET || mp->attrib & LOCAL)
			continue;	/* skip set and local labels */
		(void)fprintf (Objfil, "%s\n", Objbuf);
		secno = mp->sec->secno;
		mem = mp->attrib & MSPCMASK;
		ctr = mp->mapping & MCTRMASK;
		map = mp->mapping & MMAPMASK;
		/* new section, space, or map */
		if (secno != j || mem != k || (Lnkmode && ctr != l) || map != m) {
			j = secno;	/* save section */
			k = mem;	/* save memory space */
			l = ctr;	/* save counter */
			m = map;	/* save map */
			(void)strcpy (Objbuf, "_SYMBOL ");
			bp = Objbuf + strlen(Objbuf);
			if (Lnkmode) {	/* relative mode */
				(void)sprintf (bp, "%04x ", secno);
				bp = Objbuf + strlen(Objbuf);
			}
			*bp++ = spc_char (mem);
			if (Lnkmode && ctr)
				*bp++ = ctr_char (ctr);
			if (map)
				*bp++ = map_char (map);
			*bp = EOS;
			(void)fprintf (Objfil, "%s\n", strup (Objbuf));
		}
		(void)fprintf (Objfil, "%-16s ", mp->name);
		if (Lnkmode) {		/* relative mode */
			(void)fputc (mp->attrib & GLOBAL ? 'G' : 'L', Objfil);
			(void)fputc (mp->attrib & REL	 ? 'R' : 'A', Objfil);
		}
		if (mp->attrib & FPT)
			(void)sprintf (Objbuf, "F %-13.6f", mp->def.fdef);
		else
			if (mp->attrib & LONG)
				(void)sprintf (Objbuf, "I %s%06lx%06lx",
					Hexstr, mp->def.xdef.high,
					mp->def.xdef.low);
			else
				(void)sprintf (Objbuf, "I %s%06lx",
					Hexstr, mp->def.xdef.low);
		(void)strup (Objbuf);
	}

	free((char *)sort_tab); /* release the sort table */
#endif /* SASM && PASM */
}

/**
*
* name		xrf_obj - write external reference information to object file
*
* description	Scan the external reference hash table and write an XREF
*		object record for each symbol.
*
**/
static
xrf_obj ()
{
#if !SASM && !PASM
	register i, j;
	struct symref *mp, **sort_tab;
	int secno;
	char spc_char (), *strup();

	if (!Objfil)		/* no object file */
		return;

	if (!Xrf_cnt)		/* no external references */
		return;

	/* make room for table to sort symbols in */
	sort_tab = (struct symref **)alloc(Xrf_cnt * sizeof(struct symref *));
	if( !sort_tab )
		fatal("Out of memory - cannot sort external references");

	/* now fill in the table */
	i = 0;
	for( j = 0 ; j < HASHSIZE ; j++ )
		for( mp = Xrftab[j] ; mp ; mp = mp->next )
			sort_tab[i++] = mp;

	sortxrefs(sort_tab,Xrf_cnt);	/* sort the symbols */

	j = -1;
	Obj_rtype = OBJXREF;	/* set new record type */
	for (i = 0; i < Xrf_cnt; i++) {
		mp = sort_tab[i];
		(void)fprintf (Objfil, "%s\n", Objbuf);
		secno = mp->sec->secno;
		if (secno != j) {	/* new section */
			j = secno;	/* save section */
			(void)fprintf (Objfil, "_XREF %04x\n", secno);
			Objbuf[0] = EOS;/* clear buffer */
		}
		(void)sprintf (Objbuf, "%s", mp->name);
	}

	free((char *)sort_tab); /* release the sort table */
#endif /* SASM && PASM */
}

/**
*
* name		sortxrefs - sort external references
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
	psort(0,no_xrefs - 1);
}

/**
*
* name		cmp_xrefs - compare external references
*
**/
static
cmp_xrefs (s1, s2)
struct symref *s1, *s2;
{
	int cmp;

	/* order by section */
	if (s1->sec != s2->sec)
		return (s1->sec->secno - s2->sec->secno);

	/* order by symbol name */
	if ((cmp = strcmp (s1->name, s2->name)) != 0)
		return (cmp);

	return (0);
}
