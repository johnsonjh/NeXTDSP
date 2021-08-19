#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		get_space - determine memory space
*
* synopsis	yn = get_space(am,sclass)
*		int yn;			YES if no error / NO if error
*		struct amode *am;	address mode structure
*		int sclass;		allowable memory space class
*
* description	Validates and determines the memory space prefix.
*		If valid, the memory space is marked in am->space and
*		YES is returned. If invalid, an error is generated
*		and NO is returned.
*
**/
get_space(am,sclass)
struct amode *am;
int sclass;
{
	char *toptr;	/* temp Optr */
	int s = NONE;

	toptr = Optr;	/* save old Optr */
	if(*Optr && *(Optr+1) == ':'){
		Optr += 2;
		if ((s = char_spc (*toptr)) == ERR)
			s = NONE;
		}

	am->space = s;	/* save space */

	switch(sclass){
		case NONE:
			if( s == NONE )
				return(YES);
			break;
		case SCLS1:
			if( s == XSPACE )
				return(YES);
			break;
		case SCLS2:
			if( s == YSPACE )
				return(YES);
			break;
		case SCLS3:
			if( s == LSPACE )
				return(YES);
			break;
		case SCLS4:
			if( s == PSPACE )
				return(YES);
			break;
		case SCLS5:
			if( s == XSPACE || s == YSPACE )
				return(YES);
			break;
		case SCLS6:
			if( s == XSPACE || s == YSPACE || s == LSPACE || s == PSPACE || s == NONE )
				return(YES);
			break;
		case SCLS7:
			if( s == XSPACE || s == YSPACE || s == NONE )
				return(YES);
			break;
		case SCLS8:
			if( s == YSPACE || s == NONE )
				return(YES);
			break;
		case SCLS9:
			if( s == XSPACE || s == YSPACE || s == PSPACE || s == NONE )
				return(YES);
			break;
		}

	switch(s){
		case XSPACE:
			error("Illegal memory space specified - X:");
			break;
		case YSPACE:
			error("Illegal memory space specified - Y:");
			break;
		case PSPACE:
			error("Illegal memory space specified - P:");
			break;
		case LSPACE:
			error("Illegal memory space specified - L:");
			break;
		case NONE:
			error("Missing memory space specifier");
			break;
		}
	return(NO);
}

/**
*
* name		init_amode --- determine initial xy addressing mode
*
* synopsis	yn = init_amode(am);
*		int yn;			NO if error
*		struct amode *am;	address mode structure
*
* description	Validates the addressing mode pointed to by Optr. If
*		the addressing mode is valid, am will contain all information
*		concerning the addressing mode, and YES will be returned.
*		If the addressing mode is not valid, or an error occurred,
*		NO will be returned.  Called by do_xy to accomodate
*		address register update in parallel move field.
*
**/
init_amode(am)
struct amode *am;
{
	if( get_mode(NONE,am,RSET18,REA1,LISA,LSI) == NO )
		return(NO);
	/* check for parallel address register update */
	if( (am->space == NONE && am->mode >= POSTINC && am->mode <= PSTDECO) ){
		if( *Optr != EOS ) {
			error("Address mode syntax error - extra characters");
			return(NO);
		}
	} else {
		if( *Optr++ != ',' ){
			error("Address mode syntax error - expected comma");
			return(NO);
			}
	}
	return(YES);
}

/**
*
* name		get_amode --- determine addressing mode
*
* synopsis	yn = get_amode(flags,am,rset,reamode,abmode,imode)
*		int yn;			NO if error
*		int flags;		Bit flags:
*						0 - set if comma follows
*							operand
*						1 - set if operand is a
*							destination field
*		struct amode *am;	address mode structure
*		int rset;		register set allowed for register direct
*					NONE if register direct not allowed
*		int reamode;		REA mode allowed (NONE if none)
*		int abmode;		allowed absolute address modes
*		int imode;		allowed immediated address modes
*
* description	Validates the addressing mode pointed to by Optr. If
*		the addressing mode is valid, am will contain all information
*		concerning the addressing mode, and YES will be returned.
*		If the addressing mode is not valid, or an error occurred,
*		NO will be returned. If cflag is YES, a comma terminator
*		will be verified. Otherwise, EOS terminator will be verified.
*
**/
get_amode(flags,am,rset,reamode,abmode,imode)
int flags;
struct amode *am;
int rset;
int reamode;
int abmode;
int imode;
{
	if( get_mode(flags,am,rset,reamode,abmode,imode) == NO )
		return(NO);
	if( flags & COMMA ){
		if( *Optr++ != ',' ){
			error("Address mode syntax error - expected comma");
			return(NO);
			}
		return(YES);
		}
	if( *Optr == EOS )
		return(YES);

	error("Address mode syntax error - extra characters");
	return(NO);
}

/**
*
* name		get_mode --- determine addressing mode of *Optr
*
* synopsis	yn = get_mode(flags,am,rset,reamode,abmode,imode)
*		int yn;			NO if error
*		int flags;		Bit flags:
*						0 - set if comma follows
*							operand
*						1 - set if operand is a
*							destination field
*		struct amode *am;	address mode structure
*		int rset;		register set allowed for register direct
*					NONE if register direct not allowed
*		int reamode;		REA mode allowed (NONE if none)
*		int abmode;		absolute mode allowed (NONE if none)
*		int imode;		immediate mode allowed (NONE if none)
*
* description	Validates the addressing mode pointed to by Optr. If
*		the addressing mode is valid, am will contain all information
*		concerning the addressing mode, and YES will be returned.
*		If the addressing mode is not valid, or an error occurred,
*		NO will be returned.
*
**/
get_mode(flags,am,rset,reamode,abmode,imode)
int flags;
struct amode *am;
int rset;
int reamode;
int abmode;
int imode;
{
	int i;

#if DEBUG
printf("get_mode called with rset = %d reamode = %d abmode = %d imode = %d\n",rset,reamode,abmode,imode);
#endif

	am->mode = NONE;	/* init the mode structure */
	am->val = 0L;
	am->force = NONE;
	am->fref = NO;
	am->reg = ERR;

	if( *Optr == EOS ){
		error("Syntax error - missing address mode specifier");
		return(NO);
		}

	if( (i = g_rdirect(rset,am)) == ERR )
		return(NO);
	if( i == YES ){
#if DEBUG
printf("found rdirect addressing mode\n");
#endif
		if (chk_dreg(flags,am->reg) == NO)
			return(NO);
		return(YES);
		}

	if( imode == NONE && reamode == NONE && abmode == NONE ){
		error("Only register direct addressing allowed");
		return(NO);
		}

	if( (i = g_immed(imode,am)) == ERR )
		return(NO);
	if( i == YES ){
#if DEBUG
printf("found immediate addressing mode\n");
#endif
		return(YES);
		}

	if( reamode == NONE && abmode == NONE )
		if( rset == NONE ){
			error("Only immediate addressing allowed");
			return(NO);
			}
		else if( imode == NONE ){
			error("Only register direct addressing allowed");
			return(NO);
			}
		else{
			error("Only immediate and register direct addressing allowed");
			return(NO);
			}

	if( (i = g_reamode(reamode,am)) == ERR )
		return(NO);
	if( i == YES ){
#if DEBUG
printf("found reamode addressing mode\n");
#endif
		/* see if reg. was destination in prior instruction */
		do_regref(am->reg,am->mode);
		return(YES);
		}

	if( (i = g_absol(abmode,am)) == ERR )
		return(NO);
	if( i == YES ){
#if DEBUG
printf("found absol addressing mode\n");
#endif
		return(YES);
		}

	error("Invalid addressing mode"); /* should never get here */
	return(NO);
}

/**
*
* name		g_rdirect - get register direct addressing mode
*
* synopsis	yn = g_rdirect(regset,am)
*		int yn;			NO if no reg found,
*					ERR if invalid reg found
*		int regset;		allowable register set
*		struct amode am;	register number will be put in am->reg
*
* description	Checks for register direct addressing. Returns NO if no valid
*		register name found. Returns YES if valid register found and
*		puts register number in am->reg and RDIRECT in am->mode.
*		Returns ERR if valid register name found, but register is not
*		a valid member of regset, and an error will be output.
*
**/
static
g_rdirect(regset,am)
int regset;
struct amode *am;
{
	int i,test;

	if( (i = get_rnum(&Optr)) == ERR )
		return(NO);

	switch(regset){
		case NONE:
			test = NO;
			break;
		case RSET1:
			test = i == REGA || i == REGB;
			break;
		case RSET2:
			test = i == REGY0 || i == REGY1;
			break;
		case RSET3:
			test = i == REGX0 || i == REGX1 || i == REGA || i == REGB;
			break;
		case RSET4:
			test = i == REGY0 || i == REGY1 || i == REGA || i == REGB;
			break;
		case RSET5:
			test = i == REGX || i == REGY || i == REGA || i == REGB || (i >= REGAB && i <= REGB10);
			break;
		case RSET6:
			test = (i >= REGA && i <= REGM7) || (i >= OMR && i <= SP);
			break;
		case RSET7:
			test = i >= OMR && i <= SP;
			break;
		case RSET8:
			test = i >= REGM0 && i <= REGM1;
			break;
		case RSET9:
			test = (i >= REGA && i <= REGY1) || (i >= REGR0 && i <= REGN7);
			break;
		case RSET10:
			test = i == REGX || i == REGY;
			break;
		case RSET11:
			test = i >= REGX0 && i <= REGY1;
			break;
		case RSET12:
			test = i == REGX || i == REGY || (i >= REGX0 && i <= REGY1);
			break;
		case RSET13:
			test = i == REGA;
			break;
		case RSET14:
			test = i == REGB;
			break;
		case RSET15:
			test = i >= REGA && i <= REGY1;
			break;
		case RSET16:
			test = i >= REGR0 && i <= REGR7;
			break;
		case RSET17:
			test = (i >= REGA && i <= REGM7) || (i >= OMR && i <= SP);
			break;
		case RSET18:
			test = i >= REGX && i <= SP;
			break;
		case RSET19:
			test = i == MR || i == CCR || i == OMR;
			break;
		case RSET20:
			test = i >= REGR0 && i <= REGN7;
			break;
		case RSET21:
			test = i >= REGX && i <= REGY1;
			break;
		case RSET22:		/* Rev. C */
			test = i == REGX0 || i == REGA || i == REGB;
			break;
		case RSET23:		/* Rev. C */
			test = i == REGX0 || i == REGY0 || i == REGY1 || i == REGA || i == REGB;
			break;
		}

	if( test ){
		am->mode = RDIRECT;
		am->reg = i;
		return(YES);
		}

	if( regset == NONE )
		error("Register direct addressing not allowed");
	else
		error("Invalid register specified");
	return(ERR);
}

/**
*
* name		chk_dreg - perform misc. checks on register direct operands
*
* synopsis	yn = chk_dreg(flags,reg)
*		int yn;			YES if all checks OK, NO otherwise
*		int flags;		Bit flags:
*						0 - set if comma follows
*							operand
*						1 - set if operand is a
*							destination field
*		int reg;		address register number
*
* description	Performs various checks on register direct operands.
*		Sets flag if reg is control register (SR, OMR, SP, LA, etc.).
*		Sets bits in register move map to check for invalid register
*		references in next instruction.	 Calls dup_dest to check
*		for duplicate destination registers in parallel move.
*
**/
static
chk_dreg(flags,reg)
int flags,reg;
{
	/* set control register flag */
	if (!Ctlreg_flag)	/* don't bother if already set */
		Ctlreg_flag = (reg >= REGM0 && reg <= REGM7) ||
			      (reg >= OMR   && reg <= CCR);

	if (flags & DEST){
#if !SASM
		if (reg >= REGR0 && reg <= REGM7)
			/* set destination register move bit */
			Regmov |= (1L << (reg - REGR0));
#endif
		/* check for duplicate destination registers */
		if ( dup_dest (reg) )
			return (NO);
		}

	return(YES);
}

/**
*
* name		do_regref - construct register reference bit map;
*			    compare with register move bit map
*
* synopsis	do_regref(reg,mode)
*		int reg;		address register number
*		int mode;		addressing mode
*
* description	Constructs a register reference bit map based on
*		the register and mode.	Compares reference map with
*		move map.  If comparison is non-zero, issues a warning.
*
**/
static
do_regref(reg,mode)
int reg,mode;
{
	long regmask;	/* register reference mask */
	long basereg;	/* address register bit map */

	regmask = basereg = (1L << (reg - REGR0));
	switch (mode) { /* OR in offset register per addressing mode */
		case PSTINCO:
		case PSTDECO:
		case INDEXO:	regmask |= (basereg << 8);
	}
	switch (mode) { /* OR in modifier register per addressing mode */
		case POSTINC:
		case POSTDEC:
		case PSTINCO:
		case PSTDECO:
		case PREDEC:	regmask |= (basereg << 16);
	}
	if ((Regref & regmask) != 0L)		/* pipeline problem */
		if (Rpflag &&			/* generate NOP */
		    !Chkdo &&			/* not checking DO loop */
		    !Line_err)			/* no previous errors */
			gen_nop ();
		else
			error ("Contents of assigned register in previous instruction not available");
}

/**
*
* name		dup_dest - check for duplicate destination registers
*
* synopsis	yn = dup_dest (reg)
*		int yn;		YES if duplicate register, NO otherwise
*		int reg;	register number to test
*
* description	Checks the input register value in conjunction with the
*		global variables DestA and DestB.  If reg represents
*		registers A, A0, A1, A2, AB, BA, or A10, and DestA is YES, YES
*		is returned; if DestA is NO, DestA is set to YES and NO
*		is returned.  If reg represents registers B, B0, B1, B2, AB,
*		BA, or B10, and DestB is YES, YES is returned; if DestB is NO,
*		DestB is set to YES and NO is returned.	 In all other cases,
*		NO is returned.	 An error is issued if duplicate registers
*		are detected (dup_dest() == YES).
*
**/
static
dup_dest (reg)
int reg;
{
	switch(reg){
		case REGA:
		case REGA0:
		case REGA1:
		case REGA2:
		case REGA10:
			if (DestA) {
				error ("Duplicate destination register not allowed");
				return (YES);
			} else {
				DestA = YES;
				return (NO);
			}
		case REGB:
		case REGB0:
		case REGB1:
		case REGB2:
		case REGB10:
			if (DestB) {
				error ("Duplicate destination register not allowed");
				return (YES);
			} else {
				DestB = YES;
				return (NO);
			}
		case REGAB:
		case REGBA:
			if (DestA || DestB) {
				error ("Duplicate destination register not allowed");
				return (YES);
			} else {
				DestA = YES;
				DestB = YES;
				return (NO);
			}
		default:
			return (NO);
	}
}

/**
*
* name		g_reamode - get address register addressing modes
*
* synopsis	yn = g_reamode(reamode,am)
*		int yn;		YES if found, NO if not found, ERR if invalid
*		int reamode;	allowable address modes
*		struct amode *am;	structure to put results in
*
* description	Checks to see if syntax is valid for register addressing modes.
*		If valid, and addressing mode is allowed,
*		returns YES and puts register # in
*		am->reg. If invalid, returns NO. If valid enough to not be
*		confused with an expression, an error message is
*		output and returns ERR. If valid but not allowed, an error
*		message is output and returns ERR.
*
**/
static
g_reamode(reamode,am)
int reamode;
struct amode *am;
{
	char *toptr;	/* temp Optr */
	char tc;	/* temp */

	toptr = Optr; /* save old Optr */

	if( *Optr == '-' && *(Optr+1) == '(' ){ /* predecrement */
		Optr += 2;
		if( (am->reg = get_ra()) == ERR ){
			Optr = toptr; /* restore old Optr */
			return(NO);
			}

		if( *Optr++ != ')' ){
			error("Address mode syntax error - probably missing ')'");
			return(ERR);
			}
		if( reamode == REA1 ){
			am->mode = PREDEC;
			return(YES);
			}
		else{
			error("Pre-decrement addressing mode not allowed");
			return(ERR);
			}

		}

	if( *Optr != '(' )
		return(NO);
	++Optr;

	if( (am->reg = get_ra()) == ERR ){
		Optr = toptr; /* restore old Optr */
		return(NO);
		}

	if( *Optr == '+' ){
		++Optr;
		if( chk_na(am->reg) == ERR ){
			error("Address mode syntax error - probably missing ')'");
			return(ERR);
			}
		if( *Optr++ != ')' ){
			error("Address mode syntax error - probably missing ')'");
			return(ERR);
			}
		if( *Optr != ',' && *Optr != EOS ){
			error("Address mode syntax error");
			return(ERR);
			}
		if( reamode == REA1 ){
			am->mode = INDEXO;
			return(YES);
			}
		else{
			error("Indexed address mode not allowed");
			return(ERR);
			}
		}
	if( *Optr++ != ')' ){
		error("Address mode syntax error - probably missing ')'");
		return(ERR);
		}

	if( *Optr == EOS || *Optr == ',' ){ /* no update */
		if( reamode == REA1 || reamode == REA2 ) {
			am->mode = NOUPDATE;
			return(YES);
			}
		else {
			error("No-update mode not allowed");
			return(ERR);
			}
		}

	if( *Optr != '+' && *Optr != '-' ){
		error("Address mode syntax error");
		return(ERR);
		}
	tc = *Optr++; /* save character */

	if( *Optr == EOS || *Optr == ',' ){
		if( tc == '+' ){
			am->mode = POSTINC;
			return(YES);
			}
		else {
			am->mode = POSTDEC;
			return(YES);
			}
		}
	if( chk_na(am->reg) == NO ){
		error("Address mode syntax error");
		return(ERR);
		}
	if( *Optr != EOS && *Optr != ',' ){
		error("Address mode syntax error");
		return(ERR);
		}
	if( tc == '+' ){
		am->mode = PSTINCO;
		return(YES);
		}
	if( reamode == REA1 || reamode == REA3 ){
		am->mode = PSTDECO;
		return(YES);
		}
	error("Post-decrement by offset addressing mode not allowed");
	return(ERR);
}

/**
*
* name		g_immed - get immediate addressing mode
*
* synopsis	yn = g_immed(imode,am)
*		int yn;		YES if found, NO if not found, ERR if error
*		int imode;	allowed immediate addressing modes
*
* description	Checks for immediate addressing mode. If found and allowed,
*		the value will be placed in am->val and YES will be
*		returned. If not found, NO will be returned. If the
*		expression has an error, ERR will be returned. If immediate
*		addressing is found but not allowed, an error will be output
*		and ERR will be returned.
*
**/
static
g_immed(imode,am)
int imode;
struct amode *am;
{
	int ftype; /* force type */
	int relext; /* relative/external flag */
	struct evres *ev, *eval_immed();
	char *g_expstr();

	if( *Optr != '#' )
		return(NO);

	if( imode == NONE ){
		error("Immediate addressing mode not allowed");
		while (*Optr && *Optr != ',')
			Optr++;		/* skip to end of subfield */
		return(ERR);
		}

	++Optr;

	if( (ev = eval_immed(imode)) == NULL)
		return(ERR);

	am->val = ev->uval.xvalue.low;		/* save value */
	ftype = am->force = ev->force;		/* save forcing */
	am->fref = ev->fwd_ref; /* save forward ref indicator */
	relext = ev->rel || ev->secno < 0;	/* save for later */
	am->secno = ev->secno;	/* save section number */
	free_exp(ev);		/* free expression structure */

	switch( imode ){
		case LI:
			if( ftype == FORCES ){
				warn("Short immediate cannot be forced");
				}
			am->mode = LIMMED;
			break;
		case MI:
			if( ftype == FORCEL ){
				warn("Long immediate cannot be forced");
				}
			am->mode = MIMMED;
			if( Pass == 2 && !twelvebits(am->val) ){
				error("Immediate value too large");
				return(ERR);
				}
			break;
		case SI:
			if( ftype == FORCEL ){
				warn("Long immediate cannot be forced");
				}
			am->mode = SIMMED;
			if( Pass == 2 && !eightbits(am->val) ){ /* test value size */
				error("Immediate value too large");
				return(ERR);
				}
			break;
		case NI:
			if( ftype == FORCEL ){
				warn("Long immediate cannot be forced");
				}
			am->mode = NIMMED;
			if( Pass == 2 && !wordbits(am->val) ){ /* test value size */
				error("Immediate value too large");
				return(ERR);
				}
			break;
		case LSI:
			if( ftype == FORCEL )
				am->mode = LIMMED;
			else if( ftype == FORCES ){
				am->mode = SIMMED;
				if( am->fref == NO ){
					if( !eightbits(am->val) ){ /* test value size */
						am->mode = LIMMED; /* substitute long immediate */
						warn("Immediate value too large to use short - long substituted");
						}
					}
				else if( Pass == 2 && !eightbits(am->val) ){ /* test value size */
					error("Immediate value too large to use short");
					return(ERR);
					}
				}
			else{
				if( am->fref == NO && !relext ){
					if( eightbits(am->val) ) /* test value size */
						am->mode = SIMMED;
					else
						am->mode = LIMMED;
					}
				else{
					am->mode = LIMMED; /* default to long immediate */
					}
				}
			break;
		}

	am->expstr = relext ? g_expstr (am) : NULL; /* get expr. string */
	return(YES);
}

/**
*
* name		eval_immed - evaluate/edit immediate value
*
* synopsis	ev = eval_immed(imode)
*		struct evres ev;	evaluation result
*		int imode;		allowed immediate addressing modes
*
* description	Evaluates and checks the immediate expression for valid
*		type based on the imode.  Returns the expression value
*		on success, NULL on error.
*
**/
static struct evres *
eval_immed(imode)
int imode;
{
	struct evres *ev, *eval_word();
	long val;

	if( (ev = eval_word()) == NULL )
		return(NULL);

	if (ev->etype == INT) {
		if (ev->uval.xvalue.low < 0L && imode != LI && imode != LSI) {
			free_exp(ev);
			error("Negative immediate value not allowed");
			return (NULL);
		}
	} else {	/* floating point */
		if (imode != LI && imode != LSI) {
			free_exp(ev);
			error("Floating point value not allowed");
			return (NULL);
		}
		dtof (ev->uval.fvalue, SIZES, ev); /* convert to fractional */
	}

	val = ev->uval.xvalue.low;		/* save value */
	if (!ev->fwd_ref && ev->force != FORCEL && frac_reg () &&
	    (val & (long)SHRTMASK) == 0L)
		/* shift to low byte and clear high bits */
		ev->uval.xvalue.low = (val >> 16) & (long)BYTEMASK;

	return (ev);
}

/**
*
* name		frac_reg - check that destination register is fractional
*
* synopsis	yn = frac_reg ()
*		int yn;		YES if register is fractional; NO otherwise
*
* description	Examines destination register of immediate move.
*		Returns YES if destination is fractional register,
*		NO otherwise.
*
**/
static
frac_reg ()
{
	int rc;
	char *optr;
	int gagerrs;
	struct amode am;

	optr = Optr;		/* save global pointer */
	gagerrs = Gagerrs;
	Gagerrs = YES;		/* turn off error reporting */

	if (*Optr++ != ',')	/* syntax error */
		rc = NO;
	else if ((rc = g_rdirect (RSET15, &am)) != YES)
		rc = NO;

	Optr = optr;		/* restore global pointer */
	Gagerrs = gagerrs;	/* restore error status */

	return (rc);
}

/**
* name		g_absol - get absolute addressing mode
*
* synopsis	yn = g_absol(absmode,am)
*		int yn;		YES if found, NO if not found, ERR if error
*		int absmode;	allowed absolute addressing modes
*		struct amode *am;	structure to put result in
*
* description	Checks for a valid absolute address. If found, the address
*		will be put in am->val and YES will be returned.
*		If an error occurs, ERR will be returned.
*
**/
static
g_absol(absmode,am)
int absmode;
struct amode *am;
{
	int ftype; /* force type */
	int relext; /* relative/external flag */
	struct evres *ev, *eval_ad();
	char *g_expstr();

	if( absmode == NONE ){
		error("Absolute addressing mode not allowed");
		return(ERR);
		}

	if( (ev = eval_ad()) == NULL )
		return(ERR);

	if( Msw_flag && !mem_compat (ev->mspace, am->space) )
		warn("Absolute address involves incompatible memory spaces");
	if (ev->mspace && !am->space)
		am->space = ev->mspace;

	am->val = ev->uval.xvalue.low;	/* save the value */
	ftype = am->force = ev->force;	/* save force */
	am->fref = ev->fwd_ref; /* save forward ref indicator */
	relext = ev->rel || ev->secno < 0; /* save for later */
	am->secno = ev->secno;	/* save section number */
	free_exp(ev);	/* free expression structure */

	switch( absmode ){
		case LAS:
			if( ftype == FORCES ){
				warn("Short absolute address cannot be forced");
				}
			else if( ftype == FORCEI ){
				warn("I/O short absolute address cannot be forced");
				}
			am->mode = LABSOL;
			break;
		case IA:
			if( ftype == FORCEL ){
				warn("Long absolute address cannot be forced");
				}
			else if( ftype == FORCES ){
				warn("Short absolute address cannot be forced");
				}
			am->mode = IABSOL;
			if( Pass == 2 && !ioaddress(am->val) ){ /* test value size */
				error("Short I/O absolute address too small");
				return(ERR);
				}
			break;
		case MA:
			if( ftype == FORCEL ){
				warn("Long absolute address cannot be forced");
				}
			else if( ftype == FORCEI ){
				warn("I/O short absolute address cannot be forced");
				}
			am->mode = MABSOL;
			if( Pass == 2 && !twelvebits(am->val) ){ /* test value size */
				error("Short absolute address too large");
				return(ERR);
				}
			break;
		case SA:
			if( ftype == FORCEL ){
				warn("Long absolute address cannot be forced");
				}
			else if( ftype == FORCEI ){
				warn("I/O short absolute address cannot be forced");
				}
			am->mode = SABSOL;
			if( Pass == 2 && !sixbits(am->val) ){ /* test value size */
				error("Short absolute address too large");
				return(ERR);
				}
			break;
		case LMA:
			if( ftype == FORCEL )
				am->mode = LABSOL;
			else if( ftype == FORCES ){
				am->mode = MABSOL;
				if( am->fref == NO ){
					if( !twelvebits(am->val) ){ /* test value size */
						am->mode = LABSOL; /* substitute long immediate */
						warn("Absolute address too large to use short - long substituted");
						}
					}
				else if( Pass == 2 && !twelvebits(am->val) ){ /* test value size */
						error("Absolute address too large to use short");
						return(ERR);
						}
				}
			else{
				if( am->fref == NO && !relext ){
					if( twelvebits(am->val) ) /* test value size */
						am->mode = MABSOL; /* short absolute */
					else
						am->mode = LABSOL;
					}
				else{
					am->mode = LABSOL; /* default to long absolute */
					}
				}
			break;
		case LISA:
			if( ftype == FORCEL )
				am->mode = LABSOL;
			else if( ftype == FORCEI ){
				am->mode = IABSOL;
				if( am->fref == NO ){
					if( !ioaddress(am->val) ){ /* test value size */
						am->mode = LABSOL; /* substitute long immediate */
						warn("Absolute address too small to use I/O short - long substituted");
						}
					}
				else if( Pass == 2 && !ioaddress(am->val) ){ /* test value size */
						error("Absolute address too small to use I/O short");
						return(ERR);
						}
				}
			else if( ftype == FORCES ){
				am->mode = SABSOL;
				if( am->fref == NO ){
					if( !sixbits(am->val) ){ /* test value size */
						am->mode = LABSOL; /* substitute long immediate */
						warn("Absolute address too large to use short - long substituted");
						}
					}
				else if( Pass == 2 && !sixbits(am->val) ){ /* test value size */
						error("Absolute address too large to use short");
						return(ERR);
						}
				}
			else{
				if( am->fref == NO && !relext ){
					if( sixbits(am->val) ) /* test value size */
						am->mode = SABSOL; /* short absolute */
					else if( ioaddress(am->val) )
						am->mode = IABSOL; /* short IO address */
					else
						am->mode = LABSOL;
					}
				else{
					am->mode = LABSOL; /* default to long absolute */
					}
				}
			break;
		case ISA:
			if( ftype == FORCEL ){
				if( am->fref == NO ){
					if( sixbits(am->val) ){ /* test value size */
						warn("Long absolute address cannot be forced - substituting short addressing");
						am->mode = SABSOL; /* short absolute */
						}
					else if( ioaddress(am->val) ){
						warn("Long absolute address cannot be forced - substituting I/O short addressing");
						am->mode = IABSOL; /* short IO address */
						}
					else{
						error("Long absolute address cannot be used");
						return(ERR);
						}
					}
				else{
					am->mode = SABSOL; /* default to long absolute */
					if( Pass == 2 )
						if( !sixbits(am->val) ){
							error("Long absolute cannot be used - force short or I/O short");
							return(ERR);
							}
						else
							warn("Long absolute address cannot be forced - substituting short addressing");
					}
				}
			else if( ftype == FORCEI ){
				am->mode = IABSOL;
				if( am->fref == NO ){
					if( !ioaddress(am->val) ){ /* test value size */
						error("Absolute address too small to use I/O short");
						return(ERR);
						}
					}
				else if( Pass == 2 && !ioaddress(am->val) ){ /* test value size */
						error("Absolute address too small to use I/O short");
						return(ERR);
						}
				}
			else if( ftype == FORCES ){
				am->mode = SABSOL;
				if( am->fref == NO ){
					if( !sixbits(am->val) ){ /* test value size */
						error("Absolute address too large to use short");
						return(ERR);
						}
					}
				else if( Pass == 2 && !sixbits(am->val) ){ /* test value size */
						error("Absolute address too large to use short");
						return(ERR);
						}
				}
			else{
				if( am->fref == NO && !relext ){
					if( sixbits(am->val) ) /* test value size */
						am->mode = SABSOL; /* short absolute */
					else if( ioaddress(am->val) )
						am->mode = IABSOL; /* short IO address */
					else{
						error("Absolute address must be either short or I/O short");
						return(ERR);
						}
					}
				else{
					am->mode = SABSOL; /* default to short absolute */
					if( Pass == 2 && !sixbits(am->val) ){ /* default was wrong */
						error("Absolute address contains forward reference - force short or I/O short address");
						return(ERR);
						}
					}
				}
			break;
		case LIA:
			if( ftype == FORCEL )
				am->mode = LABSOL;
			else if( ftype == FORCEI ){
				am->mode = IABSOL;
				if( am->fref == NO ){
					if( !ioaddress(am->val) ){ /* test value size */
						am->mode = LABSOL; /* substitute long immediate */
						warn("Absolute address too small to use I/O short - long substituted");
						}
					}
				else if( Pass == 2 && !ioaddress(am->val) ){ /* test value size */
						error("Absolute address too small to use I/O short");
						return(ERR);
						}
				}
			else if( ftype == FORCES ){
				am->mode = LABSOL; /* substitute long immediate */
				warn("Short absolute address cannot be forced - long substituted");
				}
			else{
				if( am->fref == NO && !relext ){
					if( ioaddress(am->val) )
						am->mode = IABSOL; /* short IO address */
					else
						am->mode = LABSOL;
					}
				else{
					am->mode = LABSOL; /* default to long absolute */
					}
				}
			break;
		case LSA:
			if( ftype == FORCEL )
				am->mode = LABSOL;
			else if( ftype == FORCES ){
				am->mode = SABSOL;
				if( am->fref == NO ){
					if( !sixbits(am->val) ){ /* test value size */
						am->mode = LABSOL; /* substitute long immediate */
						warn("Absolute address too large to use short - long substituted");
						}
					}
				else if( Pass == 2 && !sixbits(am->val) ){ /* test value size */
						error("Absolute address too large to use short");
						return(ERR);
						}
				}
			else if( ftype == FORCEI ){
				am->mode = LABSOL; /* substitute long immediate */
				warn("I/O short absolute address cannot be forced - long substituted");
				}
			else{
				if( am->fref == NO && !relext ){
					if( sixbits(am->val) )
						am->mode = SABSOL; /* short address */
					else
						am->mode = LABSOL;
					}
				else{
					am->mode = LABSOL; /* default to long absolute */
					}
				}
			break;
		}

	am->expstr = relext ? g_expstr (am) : NULL; /* get expr. string */
	return(YES);
}

/**
*
* name		wordbits - check value bits in word size
*
* synopsis	yn = wordbits(val)
*		int yn;		YES if wordsize-1 bits or less / NO if not
*		long val;	value to be tested
*
**/
static
wordbits(val)
long val;
{
	if( val >= (long)WORDBITS || val < 0L )
		return(NO);

	return(YES);
}

/**
*
* name		sixbits - check value 6 bit size
*
* synopsis	yn = sixbits(val)
*		int yn;		YES if 6 bits or less / NO if not
*		long val;	value to be tested
*
**/
static
sixbits(val)
long val;
{
	if( val > (long)0x3fL || val < (long)0L )
		return(NO);

	return(YES);
}

/**
*
* name		eightbits - check value is only 8 bit
*
* synopsis	yn = eightbits(val)
*		int yn;		YES if 8 bits or less / NO if not
*		long val;	value to be tested
*
**/
static
eightbits(val)
long val;
{
	if( val > (long)0xffL || val < (long)0L )
		return(NO);

	return(YES);
}

/**
*
* name		twelvebits - check value is only 12 bit
*
* synopsis	yn = twelvebits(val)
*		int yn;		YES if 12 bits or less / NO if not
*		long val;	value to be tested
*
**/
static
twelvebits(val)
long val;
{
	if( val > (long)0xfffL || val < (long)0L )
		return(NO);

	return(YES);
}

/**
*
* name		ioaddress - check value is valid I/O address
*
* synopsis	yn = ioaddress(val)
*		int yn;		YES if valid io address / NO if not
*		long val;	value to be tested
*
**/
ioaddress(val)
long val;
{
	if( val > (long)0xffffL || val < (long)0xffc0L )
		return(NO);

	return(YES);
}

/**
* name		get_ra --- return address register number, adv Optr if found
*
* synopsis	r = get_ra()
*		int r;	reg # if found, ERR if not found
*
* description	Returns register number and advances Optr
*		if R0-R7 is found, otherwise
*		returns ERR.
*
**/
static
get_ra()
{
	int reg;

	if( (reg = get_rnum(&Optr)) == ERR )
		return(ERR);

	if( reg >= REGR0 && reg <= REGR7 )
		return(reg);

	return(ERR);
}

/**
* name		chk_na - check offset register
*
* synopsis	r = chk_na(ra)
*		int r;	YES/ NO if error
*		int ra; address register number
*
* description	If N0-N7 found, confirms that N register number
*		matches R register number. Issues error and returns
*		NO if they don't match. If only N is found, returns YES.
*		Advances Optr.
*
**/
static
chk_na(ra)
int ra;
{
	int reg;

	if( (reg = get_rnum(&Optr)) == ERR ){
		if( *Optr == 'N' || *Optr == 'n' ){
			++Optr;
			return(YES);
			}
		return(NO);
		}

	if( reg >= REGN0 && reg <= REGN7 ) {
		if( reg - REGN0 == ra - REGR0 )
			return(YES);
		error("Offset register number must be the same as address register number");
		return(NO);
		}

	return(NO);
}

/**
*
* name		get_rnum - get register number
*
* synopsis	rn = get_rnum(ptr)
*		int rn;		register number (ERR if not identified)
*		char **ptr;	ptr to start of register name
*
* description	The register pointed to by ptr is identified and ptr
*		is advanced past the register name. The register number
*		is returned. If the register name is not identified, ptr
*		is left unchanged.
*
**/
get_rnum(ptr)
char **ptr;
{
	register char *tp;	/* save ptr */
	register char c;	/* save char */

	tp = *ptr;

	switch(mapdn (*(*ptr)++)){
		case 'a':
			if (!ALPHAN (**ptr))
				return(REGA);
			switch (mapdn (*(*ptr)++)) {
				case  '0':
					if (!ALPHAN (**ptr))
						return(REGA0);
					break;
				case  '2':
					if (!ALPHAN (**ptr))
						return(REGA2);
					break;
				case  'b':
					if (!ALPHAN (**ptr))
						return(REGAB);
					break;
				case  '1':
					if (!ALPHAN (**ptr))
						return(REGA1);
					if (*(*ptr)++ == '0' && !ALPHAN (**ptr))
						return(REGA10);
					break;
			}
			break;
		case 'b':
			if (!ALPHAN (**ptr))
				return(REGB);
			switch (mapdn (*(*ptr)++)) {
				case  '0':
					if (!ALPHAN (**ptr))
						return(REGB0);
					break;
				case  '2':
					if (!ALPHAN (**ptr))
						return(REGB2);
					break;
				case  'a':
					if (!ALPHAN (**ptr))
						return(REGBA);
					break;
				case  '1':
					if (!ALPHAN (**ptr))
						return(REGB1);
					if (*(*ptr)++ == '0' && !ALPHAN (**ptr))
						return(REGB10);
					break;
			}
			break;
		case 'x':
			if (!ALPHAN (**ptr))
				return(REGX);
			switch (*(*ptr)++) {
				case  '0':
					if (!ALPHAN (**ptr))
						return(REGX0);
					break;
				case  '1':
					if (!ALPHAN (**ptr))
						return(REGX1);
					break;
			}
			break;
		case 'y':
			if (!ALPHAN (**ptr))
				return(REGY);
			switch (*(*ptr)++) {
				case  '0':
					if (!ALPHAN (**ptr))
						return(REGY0);
					break;
				case  '1':
					if (!ALPHAN (**ptr))
						return(REGY1);
					break;
			}
			break;
		case 'r':
			c = *(*ptr)++;
			if (c >= '0' && c <= '7' && !ALPHAN (**ptr))
				return (REGR0 + (c - '0'));
			break;
		case 'n':
			c = *(*ptr)++;
			if (c >= '0' && c <= '7' && !ALPHAN (**ptr))
				return (REGN0 + (c - '0'));
			break;
		case 'm':
			c = *(*ptr)++;
			if (c >= '0' && c <= '7' && !ALPHAN (**ptr))
				return (REGM0 + (c - '0'));
			if (mapdn (c) == 'r' && !ALPHAN (**ptr))
				return (MR);
			break;
		case 'o':
			if (mapdn (*(*ptr)++) == 'm' &&
			    mapdn (*(*ptr)++) == 'r' &&
			    !ALPHAN (**ptr))	return (OMR);
			break;
		case 'c':
			if (mapdn (*(*ptr)++) == 'c' &&
			    mapdn (*(*ptr)++) == 'r' &&
			    !ALPHAN (**ptr))	return (CCR);
			break;
		case 's':
			switch (mapdn (*(*ptr)++)) {
				case 'r':
					if (!ALPHAN (**ptr))
						return(SR);
					break;
				case 'p':
					if (!ALPHAN (**ptr))
						return(SP);
					break;
				case 's':
					switch (mapdn (*(*ptr)++)) {
						case 'h':
							if (!ALPHAN (**ptr))
								return(SSH);
							break;
						case 'l':
							if (!ALPHAN (**ptr))
								return(SSL);
							break;
					}
					break;
			}
			break;
		case 'l':
			switch (mapdn (*(*ptr)++)) {
				case 'a':
					if (!ALPHAN (**ptr))
						return(LA);
					break;
				case 'c':
					if (!ALPHAN (**ptr))
						return(LC);
					break;
			}
			break;
	}

	*ptr = tp;	/* restore old ptr */
	return(ERR);
}

/**
* name		g_expstr - get expression string
*
* synopsis	expstr = g_exptr (am)
*		char *expstr;		expression string
*		struct amode *am;	address mode structure
*
* description	Allocates space for the current expression string
*		and returns a pointer to it.  Adds sizing field
*		based on address mode.
*
**/
static char *
g_expstr (am)
struct amode *am;
{
	char *expstr;
	int size;

	if (!Eptr || !Expr[0])		/* no expression string */
		return (NULL);

	/* determine size operand */
	switch (am->mode) {
		case NIMMED:
		case SIMMED:
		case MIMMED:
		case IABSOL:
		case SABSOL:
		case MABSOL:
			size = am->mode;
			break;
		default:
			size = 0;
			break;
	}

	/* allocate extra space for size operand */
	if ((expstr = (char *)alloc (strlen (Expr) + 5)) == NULL)
		fatal ("Out of memory - cannot save expression string");
	(void)sprintf (expstr, "%s@%d", Expr, size);
	return (expstr);
}

/**
*
* name		gen_nop - generate NOP instruction for register padding
*
* synopsis	gen_nop ()
*
* description	Pushes a NOP instruction to the object and listing files
*		to provide padding for address register pipeline access.
*
**/
static
gen_nop ()
{
	char *optr, *label, *op, *operand, *xmove, *ymove, *com;
	char line[MAXBUF+1];
	int cycles;

	optr = Optr;			/* save field pointers */
	label = Label;
	op = Op;
	operand = Operand;
	xmove = Xmove;
	ymove = Ymove;
	com = Com;
	(void)strcpy (line, Line);	/* save raw input line */
	cycles = Cycles;		/* save cycle count */
	(void)strcpy (Line, "\tnop");	/* set up line */
	Padreg = YES;			/* set flag */
	(void)parse_line ();		/* parse line */
	(void)process (NO,NO);		/* process line */
	print_line ('p');		/* print line */
	Padreg = NO;			/* reset flag */
	(void)strcpy (Line, line);	/* restore input line */
	(void)parse_line ();		/* re-parse input line */
	Optr = optr;			/* restore field pointers */
	Label = label;
	Op = op;
	Operand = operand;
	Xmove = xmove;
	Ymove = ymove;
	Com = com;
	Old_pc = *Pc;			/* reset saved counters */
	Old_lpc = *Lpc;
	Cycles = cycles;		/* restore cycle count */
	Lst_lno++;			/* increment listing line number */
	P_total = 0;			/* clear word count */
}
