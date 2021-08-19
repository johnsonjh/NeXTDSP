#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
* name		parse_scs --- rough parse of scs line
*
* synopsis	parse_scs (ptrfrm, ptrto)
*		char *ptrfrm;	source pointer
*		char *ptrto;	destination pointer
*
* description	Scan the string pointed to by ptrfrm, looking for
*		a comment character or EOS.  Assign Operand pointer
*		to point to string; assign Com pointer to point to
*		comment if present.
*
**/
parse_scs (ptrfrm, ptrto)
char *ptrfrm, *ptrto;
{
	Operand = ptrto;
	while (*ptrfrm && *ptrfrm != COMM_CHAR)
		*ptrto++ = *ptrfrm++;
	while (isspace (*(ptrto - 1)))	/* find end of previous field */
		--ptrto;
	Xmove = Ymove = ptrto;
	*ptrto++ = EOS;
	Com = ptrto;
	if (*(ptrfrm + 1) != COMM_CHAR)
		while (*ptrfrm)
			*ptrto++ = *ptrfrm++;
	*ptrto = EOS;
}

/**
* name		do_scs --- process structured control statement
*
* synopsis	yn = do_scs (ss)
*		int yn;		NO if error occurs
*		struct scs *ss; pointer to statement header
*
* description	Call the appropriate structured control statement routine,
*		referencing the function pointer in the ss structure.
*		Expand resulting macro definition.
*
**/
do_scs (ss)
struct scs *ss;
{
	struct macexp *expdef, *init_exp ();

	Abortp = Pmacd_flag ? NO : YES;		/* inhibit print if reqd */
	if ((*ss->scsrtn) () == NO) {		/* call scs routine */
		free_md (Scs_mdef);		/* free source lines */
		Scs_mdef = Scs_mend = NULL;
		print_line (' ');	/* print scs line */
		Abortp = YES;			/* inhibit next print */
		return (NO);
	}
	print_line (' ');	/* print scs line */
	expdef = init_exp(Scs_mdef);	/* init expansion */
	exp_mac(expdef);		/* expand code */

	free_md(Scs_mdef);	/* release the scs definition */
	Scs_mdef = Scs_mend = NULL;
	Abortp = YES;		/* inhibit print on last line */
	return(YES);
}

/**
*	scs_break --- process SCS BREAK construct
**/
scs_break ()
{
	struct scslab *lab;
	int found = NO;

	(void)chk_flds (0);
	for (lab = Scs_lab; lab && !found; lab = lab->nxtlab)
		switch (lab->labtype) {
		case SCSENDL:
			scs_savedef (" enddo");
			/* fall through */
		case SCSENDF:
		case SCSUNTL:
		case SCSENDW:
			scs_jmp (lab);
			found = YES;
			break;
		default:
			continue;
		}
	if (!found) {
		warn ("No looping construct found - .BREAK ignored");
		return (NO);
	}
	return (YES);
}

/**
*	scs_continue --- process SCS CONTINUE construct
**/
scs_continue ()
{
	struct scslab *lab;
	int found = NO;

	(void)chk_flds (0);
	for (lab = Scs_lab; lab && !found; lab = lab->nxtlab)
		switch (lab->labtype) {
		case SCSENDL:
		case SCSFOR:
		case SCSREVL:
		case SCSWHL:
			scs_jmp (lab);
			found = YES;
			break;
		default:
			continue;
		}
	if (!found) {
		warn ("No looping construct found - .CONTINUE ignored");
		return (NO);
	}
	return (YES);
}

/**
*	scs_else --- process SCS ELSE construct
**/
scs_else ()
{
	struct scslab *firstlab, *nextlab, *scs_poplbl (), *scs_getlbl ();

	(void)chk_flds (0);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSENDI) {
		Optr = NULL;
		error (".ELSE without associated .IF statement");
		return (NO);
	}
	firstlab = scs_poplbl ();
	nextlab = scs_getlbl (SCSENDI);
	scs_jmp (nextlab);
	scs_pushlbl (nextlab);
	scs_outlbl (firstlab);
	free ((char *)firstlab);
	return (YES);
}

/**
*	scs_endf --- process SCS ENDF construct
**/
scs_endf ()
{
	struct scslab *label, *scs_poplbl ();

	(void)chk_flds (0);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSFOR) {
		Optr = NULL;
		error (".ENDF without associated .FOR statement");
		return (NO);
	}
	label = scs_poplbl ();
	scs_jmp (label);
	free ((char *)label);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSENDF) {
		Optr = NULL;
		error (".ENDF without associated .FOR statement");
		return (NO);
	}
	scs_outlbl (label = scs_poplbl ());
	free ((char *)label);
	return (YES);
}

/**
*	scs_endi --- process SCS ENDI construct
**/
scs_endi ()
{
	struct scslab *label, *scs_poplbl ();

	(void)chk_flds (0);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSENDI) {
		Optr = NULL;
		error (".ENDI without associated .IF statement");
		return (NO);
	}
	scs_outlbl (label = scs_poplbl ());
	free ((char *)label);
	return (YES);
}

/**
*	scs_endl --- process SCS ENDL construct
**/
scs_endl ()
{
	struct scslab *label, *scs_poplbl ();

	(void)chk_flds (0);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSENDL) {
		Optr = NULL;
		error (".ENDL without associated .LOOP statement");
		return (NO);
	}
	scs_outlbl (label = scs_poplbl ());
	free ((char *)label);
	return (YES);
}

/**
*	scs_endw --- process SCS ENDW construct
**/
scs_endw ()
{
	struct scslab *label, *scs_poplbl ();

	(void)chk_flds (0);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSWHL) {
		Optr = NULL;
		error (".ENDW without associated .WHILE statement");
		return (NO);
	}
	label = scs_poplbl ();
	scs_jmp (label);
	free ((char *)label);
	if (Scs_lab == NULL || Scs_lab->labtype != SCSENDW) {
		Optr = NULL;
		error (".ENDW without associated .WHILE statement");
		return (NO);
	}
	scs_outlbl (label = scs_poplbl ());
	free ((char *)label);
	return (YES);
}

/**
*	scs_for --- process SCS FOR construct
**/
scs_for ()
{
	struct scsfor fors;
	struct scslab *looplab, *nextlab, *scs_getlbl ();

	if (scs_getfor (&fors) == NO)
		return (NO);
	looplab = scs_getlbl (SCSFOR);
	nextlab = scs_getlbl (SCSENDF);
	(void)sprintf (String, " move %s,a", fors.init.str);
	scs_savedef (String);
	(void)sprintf (String, " move a,%s", fors.cntr.str);
	scs_savedef (String);
	(void)sprintf (String, " move %s,y0", fors.targ.str);
	scs_savedef (String);
	(void)sprintf (String, " move %s,x0", fors.step.str);
	scs_savedef (String);
	scs_outlbl (looplab);
	(void)sprintf (String, " move %s,a", fors.cntr.str);
	scs_savedef (String);
	(void)sprintf (String, " %s x0,a", fors.downto ? "sub":"add");
	scs_savedef (String);
	(void)sprintf (String, " move a,%s", fors.cntr.str);
	scs_savedef (String);
	scs_savedef (" cmp y0,a");
	(void)sprintf (String, " j%s Z_L%05d", fors.downto ? "lt":"gt",
			nextlab->labval);
	scs_savedef (String);
	scs_pushlbl (nextlab);
	scs_pushlbl (looplab);
	return (YES);
}

/**
*	scs_if --- process SCS IF construct
**/
scs_if ()
{
	struct scslab *firstlab, *nextlab, *scs_getlbl ();
	int token;

	firstlab = scs_getlbl (SCSIF);	/* allocate target label */
	nextlab = scs_getlbl (SCSENDI); /* allocate end label */
	if (!((token = scs_eval (firstlab, nextlab)) == SCSTHEN ||
	       token == NONE)) {
		Optr = NULL;
		if (token != ERR)
			error ("Syntax error - invalid statement terminator");
		free ((char *)firstlab);
		free ((char *)nextlab);
		return (NO);
	}
	scs_outlbl (firstlab);		/* output target label */
	free ((char *)firstlab);	/* free target label */
	scs_pushlbl (nextlab);		/* push end label until ENDI */
	return (YES);
}

/**
*	scs_loop --- process SCS LOOP construct
**/
scs_loop ()
{
	struct scslab *looplab, *scs_getlbl ();
	struct amode op;

	(void)chk_flds (1);
	Optr = Operand;
	if( get_space(&op,SCLS7) == NO )
		return(NO); /* failed */
	if( op.space == NONE ){
		if( get_amode(NONE,&op,RSET17,NONE,NONE,MI) == NO )
			return (NO);
		if( op.mode == RDIRECT && op.reg == SSH) {
			Optr = NULL;
			error("Illegal use of SSH as loop count operand");
			return(NO);
			}
		}
	else /* X: or Y: loop count specified */
		if( get_amode(NONE,&op,NONE,REA1,SA,NONE) == NO )
			return (NO);
	looplab = scs_getlbl (SCSENDL);
	(void)sprintf (String, " do %s,Z_L%05d", Operand, looplab->labval);
	scs_savedef (String);
	scs_pushlbl (looplab);
	return (YES);
}

/**
*	scs_repeat --- process SCS REPEAT construct
**/
scs_repeat ()
{
	struct scslab *label, *scs_getlbl ();

	(void)chk_flds (0);
	label = scs_getlbl (SCSRPT);
	scs_outlbl (label);
	scs_pushlbl (label);
	label = scs_getlbl (SCSUNTL);
	scs_pushlbl (label);
	label = scs_getlbl (SCSREVL);
	scs_pushlbl (label);
	return (YES);
}

/**
*	scs_until --- process SCS UNTIL construct
**/
scs_until ()
{
	struct scslab *looplab, *endlab, *scs_poplbl ();
	int token;

	if (Scs_lab == NULL || Scs_lab->labtype != SCSREVL) {
		Optr = NULL;
		error (".UNTIL without associated .REPEAT statement");
		return (NO);
	}
	endlab = scs_poplbl ();
	scs_outlbl (endlab);
	free ((char *)endlab);		/* free evaluation label */
	if (Scs_lab == NULL || Scs_lab->labtype != SCSUNTL) {
		Optr = NULL;
		error (".UNTIL without associated .REPEAT statement");
		return (NO);
	}
	endlab = scs_poplbl ();
	if (Scs_lab == NULL || Scs_lab->labtype != SCSRPT) {
		Optr = NULL;
		error (".UNTIL without associated .REPEAT statement");
		return (NO);
	}
	looplab = scs_poplbl ();
	if ((token = scs_eval (endlab, looplab)) != NONE) {
		Optr = NULL;
		if (token != ERR)
			error ("Syntax error - invalid statement terminator");
		free ((char *)endlab);
		free ((char *)looplab);
		return (NO);
	}
	scs_outlbl (endlab);		/* output end label */
	free ((char *)endlab);		/* free end label */
	free ((char *)looplab);		/* free loop label */
	return (YES);
}

/**
*	scs_while --- process SCS WHILE construct
**/
scs_while ()
{
	struct scslab *looplab, *firstlab, *nextlab, *scs_getlbl ();
	int token;

	looplab = scs_getlbl (SCSWHL);
	firstlab = scs_getlbl (NONE);
	nextlab = scs_getlbl (SCSENDW);
	scs_outlbl (looplab);
	if (!((token = scs_eval (firstlab, nextlab)) == SCSDO ||
	       token == NONE)) {
		Optr = NULL;
		if (token != ERR)
			error ("Syntax error - invalid statement terminator");
		free ((char *)firstlab);
		free ((char *)nextlab);
		free ((char *)looplab);
		return (NO);
	}
	scs_outlbl (firstlab);
	free ((char *)firstlab);
	scs_pushlbl (nextlab);
	scs_pushlbl (looplab);
	return (YES);
}

/**
* name		scs_eval --- evaluate scs expressions
*
* synopsis	token = scs_eval (firstlab, nextlab)
*		int token;			token at end of expression
*		struct scslab *firstlab;	pointer to first label
*		struct scslab *nextlab;		pointer to next label
*
* description	Evaluate structured control statement expressions.
*		Call routines to generate assembly language statements.
*		Return token at end of expression, ERR on error.
*
**/
static
scs_eval (firstlab, nextlab)
struct scslab *firstlab, *nextlab;
{
	struct scsexp exp;

	Optr = Operand;		/* set up global operand pointer */
	do {
		if (scs_getexp (&exp) == NO)	/* bad expression */
			return (ERR);
		if (exp.op1.str)		/* need to move operands */
			scs_mvcmp (&exp);	/* gen move/compare code */
		scs_jcc (&exp, firstlab, nextlab);	/* jmp in any case */
		free_scsexp (&exp);		/* free exp. strings */
	} while (exp.cmpd == SCSAND || exp.cmpd == SCSOR);
	return (exp.cmpd);
}

/**
* name		scs_mvcmp --- generate scs move/compare instructions
*
* synopsis	scs_mvcmp (exp)
*		struct scsexp *exp;	pointer to expression structure
*
* description	Examine expression operands and generate macro definition
*		lines for appropriate move/compare of operands.
*
**/
static
scs_mvcmp (exp)
struct scsexp *exp;
{
	if (exp->op1.info.reg == REGA && exp->op2.info.reg == REGX0)
		;	/* do nothing */
	else if (exp->op1.info.reg == REGX0 && exp->op2.info.reg == REGA) {
		scs_savedef (" move a,y0");
		scs_savedef (" tfr x0,a y0,x0");
	} else if (exp->op1.info.reg == REGA) {
		(void)sprintf (String, " move %s,x0", exp->op2.str);
		scs_savedef (String);
	} else if (exp->op2.info.reg == REGA) {
		scs_savedef (" move a,x0");
		(void)sprintf (String, " move %s,a", exp->op1.str);
		scs_savedef (String);
	} else if (exp->op2.info.reg == REGX0) {
		(void)sprintf (String, " move %s,a", exp->op1.str);
		scs_savedef (String);
	} else {
		(void)sprintf (String, " move %s,a", exp->op1.str);
		scs_savedef (String);
		(void)sprintf (String, " move %s,x0", exp->op2.str);
		scs_savedef (String);
	}
	scs_savedef (" cmp x0,a");
}

/**
* name		scs_jcc --- generate conditional jump instruction
*
* synopsis	scs_jcc (exp, firstlab, nextlab)
*		struct scsexp *exp;		pointer to expression
*		struct scslab *firstlab;	pointer to first label
*		struct scslab *nextlab;		pointer to next label
*
* description	Generate conditional jump instruction based on compound
*		operator and label values.  Append to scs definition list.
*
**/
static
scs_jcc (exp, firstlab, nextlab)
struct scsexp *exp;
struct scslab *firstlab, *nextlab;
{
	static char *jstr = " j%s Z_L%05d";

	if (exp->cmpd == SCSOR)
		(void)sprintf (String, jstr, exp->oper->ctext, firstlab->labval);
	else
		(void)sprintf (String, jstr, Cndtab[exp->oper->compl].ctext,
			nextlab->labval);
	scs_savedef (String);
}

/**
* name		scs_jmp --- generate unconditional jump instruction
*
* synopsis	scs_jmp (label)
*		struct scslab *label;		pointer to jump label
*
* description	Generate unconditional jump instruction based on label
*		value.	Append to scs definition list.
*
**/
static
scs_jmp (label)
struct scslab *label;
{
	(void)sprintf (String, " jmp Z_L%05d", label->labval);
	scs_savedef (String);
}

/**
* name		scs_savedef --- save scs macro defintion line
*
* synopsis	scs_savedef (str)
*		char *str;	string to save
*
* description	Allocate macro definition structure.  Save input line
*		in scs macro definition list.
*
**/
static
scs_savedef (str)
char *str;
{
	struct macd *def, *save_mline ();

	def = save_mline (str);
	if (!Scs_mdef)		/* empty list */
		Scs_mdef = Scs_mend = def;
	else {
		Scs_mend->mnext = def;
		Scs_mend = def;
	}
}

/**
* name		scs_getexp --- scan for simple scs expression
*
* synopsis	yn = scs_getexp (exp)
*		int yn;			NO on error
*		struct scsexp *exp;	pointer to scs expression
*
* description	Scan string pointed to by Optr for simple scs expression.
*		Load scs expression structure fields.
*
**/
static
scs_getexp (exp)
struct scsexp *exp;
{
	char *ptr;			/* saved Optr value */
	int rc = YES;			/* return code */
	char *scs_gettok (), mapdn ();
	struct cond *cond_look ();

	exp->op1.str = exp->op2.str = NULL;
	exp->oper = NULL;		/* init. expression fields */
	exp->cmpd = NONE;
	ptr = scs_gettok ();		/* pick up token value */
	if (String[0] != '<') {		/* conditional with operands */
		Optr = String;		/* point Optr to String buffer */
		if (get_space (&exp->op1.info, SCLS9) == NO)
			rc = NO;	/* bad space */
		if (get_mode (NONE, &exp->op1.info, RSET17, REA1, LISA, LSI) == NO)
			rc = NO;	/* bad operand */
		if ((exp->op1.str = (char *)alloc (strlen (String) + 1)) == NULL)
			fatal ("Out of memory - cannot save operand");
		(void)strcpy (exp->op1.str, String);	/* save operand text */
		Optr = ptr;		/* reset Optr */
		ptr = scs_gettok ();	/* get next token */
	}

	if (!(String[0] == '<' && String[3] == '>' && String[4] == EOS)) {
		Optr = NULL;
		error ("Syntax error - invalid conditional operator");
		rc = NO;
	} else {
		String[1] = mapdn (String[1]);	/* convert to lower case */
		String[2] = mapdn (String[2]);
		String[3] = EOS;
		if ((exp->oper = cond_look (&String[1])) == NULL) {
			Optr = NULL;
			error ("Syntax error - invalid conditional operator");
			rc = NO;
		}
	}

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	if ((exp->cmpd = scs_cmpd ()) != ERR) {
		if (exp->op1.str) {
			Optr = NULL;
			error ("Syntax error - missing operand");
			rc = NO;
		}
		if (!rc)		/* error; free up strings */
			free_scsexp (exp);
		return (rc);
	}
	Optr = String;			/* point Optr to String buffer */
	if (get_space (&exp->op2.info, SCLS9) == NO)
		rc = NO;		/* bad space */
	if (get_mode (NONE, &exp->op2.info, RSET17, REA1, LISA, LSI) == NO)
		rc = NO;		/* bad operand */
	if ((exp->op2.str = (char *)alloc (strlen (String) + 1)) == NULL)
		fatal ("Out of memory - cannot save operand");
	(void)strcpy (exp->op2.str, String);	/* save operand text */

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	if ((exp->cmpd = scs_cmpd ()) == ERR) {
		Optr = NULL;
		error ("Syntax error - invalid compound operator");
		rc = NO;
	}
	Optr = ptr;			/* reset Optr */
	if (!rc)
		free_scsexp (exp);	/* free up strings on error */
	return (rc);
}

/**
* name		scs_getfor --- scan for FOR statement fields
*
* synopsis	yn = scs_getfor (fors)
*		int yn;			NO on error
*		struct scsfor *fors;	pointer to for fields
*
* description	Scan string pointed to by Optr for FOR statement fields.
*		Load FOR statement structure fields.
*
**/
static
scs_getfor (fors)
struct scsfor *fors;
{
	char *ptr;			/* saved Optr value */
	int rc = YES;			/* return code */
	char str[MAXBUF+1];		/* temp. string buffer */
	int add = YES;			/* addition flag */
	char *scs_gettok (), *strdn (), mapdn ();

	fors->cntr.str =
	fors->init.str =
	fors->targ.str =
	fors->step.str = NULL;		/* initialize fields */
	Optr = Operand;			/* point Optr to Operand field */
	ptr = scs_gettok ();		/* pick up token value */
	Optr = String;			/* point Optr to String buffer */
	if (get_space (&fors->cntr.info, SCLS9) == NO)
		rc = NO;		/* bad space */
	if (get_mode (NONE, &fors->cntr.info, RSET17, REA1, LISA, NONE) == NO)
		rc = NO;		/* bad operand */
	if ((fors->cntr.str = (char *)alloc (strlen (String) + 1)) == NULL)
		fatal ("Out of memory - cannot save operand");
	(void)strcpy (fors->cntr.str, String);	/* save operand text */

	Optr = ptr;		/* reset Optr */
	ptr = scs_gettok ();	/* get next token */
	if (!(String[0] == '=' && String[1] == EOS)) {
		Optr = NULL;
		error ("Syntax error - invalid assignment operator");
		rc = NO;
	}

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	Optr = String;			/* point Optr to String buffer */
	if (get_space (&fors->init.info, SCLS9) == NO)
		rc = NO;		/* bad space */
	if (get_mode (NONE, &fors->init.info, RSET17, REA1, LISA, LSI) == NO)
		rc = NO;		/* bad operand */
	if ((fors->init.str = (char *)alloc (strlen (String) + 1)) == NULL)
		fatal ("Out of memory - cannot save operand");
	(void)strcpy (fors->init.str, String);	/* save operand text */

	Optr = ptr;		/* reset Optr */
	ptr = scs_gettok ();	/* get next token */
	(void)strdn (strcpy (str, String));
	if (!(STREQ (str, "to") || (add = strcmp (str, "downto")) == 0)) {
		Optr = NULL;
		error ("Syntax error - expected keyword TO or DOWNTO");
		rc = NO;
	}
	if (add)
		fors->downto = NO;
	else
		fors->downto = YES;

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	Optr = String;			/* point Optr to String buffer */
	if (get_space (&fors->targ.info, SCLS9) == NO)
		rc = NO;		/* bad space */
	if (get_mode (NONE, &fors->targ.info, RSET17, REA1, LISA, LSI) == NO)
		rc = NO;		/* bad operand */
	if ((fors->targ.str = (char *)alloc (strlen (String) + 1)) == NULL)
		fatal ("Out of memory - cannot save operand");
	(void)strcpy (fors->targ.str, String);	/* save operand text */

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	(void)strdn (strcpy (str, String));
	if (String[0] == EOS || STREQ (str, "do") ) {
		(void)strcpy (String, "#>1");	/* set up default step value */
		if ((fors->step.str = (char *)alloc (strlen (String) + 1)) == NULL)
			fatal ("Out of memory - cannot save operand");
		(void)strcpy (fors->step.str, String); /* save operand text */
		fors->step.info.mode = LIMMED;
		fors->step.info.val = 1L;
		if (!rc)
			free_scsfor (fors);
		return (rc);
	}

	if (!STREQ (str, "by")) {
		Optr = NULL;
		error ("Syntax error - expected keyword BY");
		rc = NO;
	}

	Optr = ptr;			/* reset Optr */
	ptr = scs_gettok ();		/* get next token */
	Optr = String;			/* point Optr to String buffer */
	if (get_space (&fors->step.info, SCLS9) == NO)
		rc = NO;		/* bad space */
	if (get_mode (NONE, &fors->step.info, RSET17, REA1, LISA, LSI) == NO)
		rc = NO;		/* bad operand */
	if ((fors->step.str = (char *)alloc (strlen (String) + 1)) == NULL)
		fatal ("Out of memory - cannot save operand");
	(void)strcpy (fors->step.str, String);	/* save operand text */

	Optr = ptr;		/* reset Optr */
	ptr = scs_gettok ();	/* get next token */
	(void)strdn (strcpy (str, String));
	if (String[0] != EOS && !STREQ (str, "do")) {
		Optr = NULL;
		error ("Syntax error - expected keyword DO");
		rc = NO;
	}

	Optr = ptr;		/* reset Optr */
	if (!rc)
		free_scsfor (fors);
	return (rc);
}

/**
* name		scs_gettok --- isolate scs token
*
* synopsis	ptr = scs_gettok ()
*		char *ptr;	pointer to next character after token
*
* description	Using global Optr, skip white space; copy text to next
*		white space into string buffer; return new Optr.

*
**/
static char *
scs_gettok ()
{
	register char *p;

	while (*Optr && isspace (*Optr))
		Optr++;
	for (p = String; *Optr && !isspace (*Optr); p++, Optr++)
		*p = *Optr;
	*p = EOS;
	return (Optr);
}

/**
*	free_scsexp --- free structured control statement expression strings
**/
static
free_scsexp (exp)
struct scsexp *exp;
{
	if (exp->op1.str)
		free (exp->op1.str);	/* free first operand string */
	if (exp->op2.str)
		free (exp->op2.str);	/* free second operand string */
}

/**
*	free_scsfor --- free for statement operator strings
**/
static
free_scsfor (fors)
struct scsfor *fors;
{
	if (fors->cntr.str)
		free (fors->cntr.str);	/* free counter string */
	if (fors->init.str)
		free (fors->init.str);	/* free init. string */
	if (fors->targ.str)
		free (fors->targ.str);	/* free target string */
	if (fors->step.str)
		free (fors->step.str);	/* free step string */
}

/**
* name		scs_cmpd --- return boolean operator/terminator code
*
* synopsis	code = scs_cmpd ()
*		int code;		code for scs terminator/compound ops
*
* description	Examine string buffer for scs compound operators.  Return
*		corresponding boolean or terminator code.
*
**/
static
scs_cmpd ()
{
	char str[MAXBUF+1], *strdn ();

	if (String[0] == EOS)
		return (NONE);
	(void)strdn (strcpy (str, String));
	if (STREQ (str, "and"))
		return (SCSAND);
	if (STREQ (str, "or"))
		return (SCSOR);
	if (STREQ (str, "then"))
		return (SCSTHEN);
	if (STREQ (str, "do"))
		return (SCSDO);
	return (ERR);
}

/**
* name		scs_getlbl --- allocate scs label structure
*
* synopsis	label = scs_getlbl (type)
*		struct scslab *label;	pointer to label structure returned
*		int type;		scs type
*
* description	Allocate and initialize an scs label structure.
*
**/
static struct scslab *
scs_getlbl (type)
int type;
{
	struct scslab *label;

	if ((label = (struct scslab *)alloc (sizeof (struct scslab))) == NULL)
		fatal ("Out of memory - cannot allocate scs label");
	label->labval = Scs_labcnt++;
	label->labtype = type;
	label->nxtlab = NULL;
	return (label);
}

/**
* name		scs_pushlbl --- push a label onto the label stack
*
* synopsis	scs_pushlbl (label)
*		struct scslab *label;	pointer to label to be pushed
*
* description	Push the label structure onto the scs label stack.
*
**/
static
scs_pushlbl (label)
struct scslab *label;
{
	label->nxtlab = Scs_lab;
	Scs_lab = label;
}

/**
* name		scs_poplbl --- pop a label off the label stack
*
* synopsis	label = scs_poplbl ()
*		struct scslab *label;	pointer to label popped off stack
*
* description	Pop a label structure off of the scs label stack.
*
**/
static struct scslab *
scs_poplbl ()
{
	struct scslab *label;

	label = Scs_lab;
	if (Scs_lab)
		Scs_lab = Scs_lab->nxtlab;
	return (label);
}

/**
* name		scs_outlbl --- outputs a label to the scs definition list
*
* synopsis	scs_outlbl (label)
*		struct scslab *label;	label to be output
*
* description	Constructs an assembler label line from the input label
*		structure and appends it to the scs macro definition list.
*
**/
static
scs_outlbl (label)
struct scslab *label;
{
	(void)sprintf (String, "Z_L%05d", label->labval);
	scs_savedef (String);
}

/**
*	clr_scslbl --- clear the SCS label stack
**/
clr_scslbl ()
{
	char *ptr;
	struct scslab *label, *scs_poplbl ();

	ptr = Optr;	/* save global pointer */
	Optr = NULL;	/* clear global pointer */
	while ((label = scs_poplbl ()) != NULL) {
		if (Pass == 2)
			switch (label->labtype) {
			case SCSENDF:
				error ("Unexpected end of file - missing .ENDF");
				break;
			case SCSENDI:
				error ("Unexpected end of file - missing .ENDI");
				break;
			case SCSENDL:
				error ("Unexpected end of file - missing .ENDL");
				break;
			case SCSUNTL:
				error ("Unexpected end of file - missing .UNTIL");
				break;
			case SCSENDW:
				error ("Unexpected end of file - missing .ENDW");
				break;
			}
		free ((char *)label);
	}
	Optr = ptr;
	Scs_labcnt = 0;
}
