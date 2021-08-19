#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
* name		do_op --- process mnemonic
*
* synopsis	yn = do_op(op)
*		int yn;			YES if no error / NO if error
*		struct moper *op;	pointer to mnemonic table entry
*
* description	Called with a pointer to the opcode data structure.
*		Optr points to the beginning of the operand field.
*		Calls appropriate routine to process current instruction
*		according to instruction class.	 Fills in object
*		code data structure.
**/
do_op(op)
struct mnemop *op;
{
	int class = op->class;	/* mnemonic class */
	int xyclass;		/* data transfer class */
	struct amode xsrc;	/* x source */
	struct amode xdst;	/* x dest   */
	struct amode ysrc;	/* y source */
	struct amode ydst;	/* y dest   */
	struct obj objout;	/* object code structure */
	long val[EWLEN];	/* emit data array */
	int rc;			/* return code */

	if( Cspace != PSPACE ){
		error("Runtime space must be P");
		return(NO);
		}

	do_init ();	/* initialize per-line globals */

	objout.count	= 1; /* initialize object code structure */
	objout.opcode	= op->opcode;
	objout.postword = 0;
	objout.opexp = objout.postexp = NULL;

#if SASM
	Sasmobj.count	= 1; /* initialize SASM word count */
#endif

	Optr = Operand; /* point to operand field */

	switch(class){
		case ABBA:			/* A,B or B,A */
			rc = do_abba (&objout);
			break;
		case ACCUMXY:			/* A or B	*/
		case TSTCLASS:
			rc = do_accumxy (class, &objout);
			break;
		case ANDOR:			/* AND, OR */
			rc = do_andor (&objout);
			break;
		case BCLASS:			/* BSET, BCLR, BCHG */
		case BTST:			/* BTST */	/* Rev. C */
			rc = do_bclass (class, &objout);
			break;
		case DIVCLASS:			/* DIV */
			rc = do_divclass (&objout);
			break;
		case DOCLASS:			/* DO instruction     */
			rc = do_doclass (&objout);
			break;
		case ENDDO:			/* ENDDO */
			rc = do_enddo ();
			break;
		case INH:	/* inherent class, no moves allowed */
		case INHNR:	/* inherent class, no moves, no repeats */
		case NOP:	/* NOP instruction */
		case ILLEGAL:	/* ILLEGAL instruction */
			rc = do_inh (class);
			break;
		case JUMP:			/* JMP	*/
		case JCCLASS:			/* Jcc	*/
			rc = do_jump (class, &objout);
			break;
		case JSR:			/* JSR	*/
		case JSCCLAS:			/* JScc */
			rc = do_jsr (class, &objout);
			break;
		case JCLRCLS:			/* JCLR, JSET	*/
		case JSCLR:			/* JSCLR, JSSET */
			rc = do_jmpbit (class, &objout);
			break;
		case LUACLASS:			/* LUA */
			rc = do_luaclass (&objout);
			break;
		case MACLASS:			/* MAC and MACR */
		case MPYCLAS:			/* MPY and MPYR */
			rc = do_maclass (&objout);
			break;
		case MOVCLAS:
		case MOVCCLS:
		case MOVMCLS:			/* MOVE, MOVEC, MOVEM */
			rc = do_move ();
			break;
		case MOVPCLS:			/* MOVEP */
			rc = do_movep (&objout);
			break;
		case RD1:			/* R,D1	  */
			rc = do_rd1 (&objout);
			break;
		case REPCLAS:			/* REP instruction	*/
			rc = do_repclas (&objout);
			break;
		case RTICLASS:			/* RTI */
		case RTSCLASS:			/* RTS */
			rc = do_rtsclass (class);
			break;
		case S9D1:			/* S9,D1     */
		case S10D1:			/* S10,D1   */
			rc = do_s910d1 (class, &objout);
			break;
		case S11D1AB:			/* S11,D1 or A,B or B,A	 */
		case S15D1AB:			/* S15,D1 or A,B or B,A	 */
		case CMPCLASS:
			rc = do_s1115d1ab (class, &objout);
			break;
		case SIMM:			/* short immediate	*/
			rc = do_simm (&objout);
			break;
		case TFRCC:			/* Tcc S11,D1 or Tcc S11,D1 R,R */
			rc = do_tfrcc (&objout);
			break;
		default:
			fatal("Error in mnemonic table");
		}

	if (op->xyclass == VARIES) {	/* adjust rc if xyclass varies */
		xyclass = rc;
		rc = YES;
	} else
		xyclass = op->xyclass;

	if( xyclass )			/* parallel moves allowed */
		if( !*Xmove )
			objout.opcode |= 1L << 21; /* set no-move bit */
		else{
			if( *Ymove )	/* see if fields are swapped */
				Swap = swap_fields ();
			else
				Swap = NO;
			rc = do_xy(&objout,xyclass,&xsrc,&xdst,&ysrc,&ydst);
			if (Swap)	/* swap fields back */
				do_swap ();
			if (class == MOVCCLS && !Ctlreg_flag)
				warn ("No control registers accessed - using MOVE encoding");
			if (class == MOVMCLS && xsrc.space != PSPACE && xdst.space != PSPACE)
				warn ("P space not accessed - using MOVE encoding");
			}
	else if( *Xmove )		/* too many fields specified */
		switch (class) {	/* already flagged these */
			case MOVPCLS:
			case TFRCC:
				break;
			default:
				error("Too many fields specified for instruction");
				rc = NO;
			}

#if SASM
	if (Sasmobj.count > 0) {
		Sasmobj.count = objout.count;
		Sasmobj.opcode = objout.opcode;
		Sasmobj.postword = objout.postword;
	}
#else
	if (Chkdo && !Padreg)	/* reprocessing first DO instruction; */
		return (rc);	/* called recursively from unstk_do */

	if (Dstk && !Padreg){	/* DO loop in progress */
		if (rc && (Prev_flags & PREVDO))
			save_doline (class);	/* save this line for later */
		unstk_do (&objout);	/* check DO loop stack */
		}

	if (Int_flag && !Relmode && *Pc < PSTART && Bad_flags & BADINT) {
		Optr = Op;
		error ("Instruction cannot appear in interrupt vector locations");
		rc = NO;
	}

	val[LWORD] = objout.opcode;		/* save code in emit array */
	if (!objout.opexp)
		emit_data(val,(char *)NULL);	/* generate absolute code */
	else {
		emit_data(val,objout.opexp);	/* gen reloc code */
		free (objout.opexp);		/* free expression string */
	}

	if( objout.count == 2 ) {
		if (Prev_flags & BADREP) {
			Optr = Op;
			error ("Cannot repeat two-word instruction");
			rc = NO;
		}
		if (chk_do(0))		/* check loop addresses */
			rc = NO;
		val[LWORD] = objout.postword;	/* save code in emit array */
		if (!objout.postexp)	/* generate second word */
			emit_data(val,(char *)NULL);
		else {
			emit_data(val,objout.postexp);
			free (objout.postexp);	/* free expression string */
		}
	}

	if (Muflag)	/* memory utilization report active */
		do_mu (MUCODE, (unsigned)objout.count);
#endif
	return(rc);
}

/**
*
* name		do_init - initialize per-line global variables
*
**/
static
do_init ()
{
	Ctlreg_flag = NO; /* reset control register flag */

	DestA = DestB = NO; /* initialize duplicate dest. register flags */

#if !SASM
	Laerr = NO;	/* reset loop address error flag */

	/* assign move map to ref map; reinit. move map */
	Regref = Regmov;
	Regmov = 0L;

	/* assign bad flags to prev. flags; reinit. bad flags */
	Prev_flags = Bad_flags;
	Bad_flags = 0;

	/* if scanning interrupts, clear flags when address is even */
	if (Int_flag && *Pc < PSTART && !(*Pc & 1L)) {
		Regref = 0L;
		Prev_flags = 0;
	}
#endif
}

/**
*
* name		save_doline - save first instruction after DO in DO stack
*
* synopsis	save_doline (class)
*		int class;		current instruction class
*
* description	Called from do_op for the first instruction after a DO.
*		Since the first instruction may itself be a DO, the
*		DO stack pointer is adjusted accordingly before the
*		allocation is made.
**/
static
save_doline (class)
int class;
{
	struct dostack *dstk;

	if (class == DOCLASS)	/* current instruction is a DO */
		dstk = Dstk->next;
	else
		dstk = Dstk;
	if (!dstk)
		fatal ("DO stack out of sequence");
	if ((dstk->line = (char *)alloc(strlen(Line) + 1)) == NULL)
		fatal("Out of memory - cannot save initial loop instruction");
	(void)strcpy (dstk->line, Line);
}

/**
*
* name		do_inh - process inherent-class instructions
*
* synopsis	yn = do_inh (class)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*
* description	Performs processing related to inherent-class
*		instructions.
*
**/
static
do_inh (class)
int class;
{
	int rc = YES;	/* return code */

	Optr = Op;	/* set Optr in case of error */
	switch(class){
		case INHNR:  /* inherent class, no moves, no repeats */
			if (Prev_flags & BADREP){
				error("Cannot repeat this instruction");
				rc = NO;
				}
			break;
		case INH:   /* inherent class, no moves allowed */
			if (chk_do (0))
				rc = NO;
			break;
		}
	Optr = Operand; /* reset Optr */
	Ymove = Xmove;	/* move fields, will check later */
	Xmove = Operand;
	Operand = Nullstr;
	return (rc);
}

/**
*
* name		do_rticlass - process RTI/RTS instructions
*
* synopsis	yn = do_rtsclass (class)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*
* description	Performs processing related to RTI/RTS instructions.
*
**/
static
do_rtsclass (class)
int class;
{
	int rc = YES;	/* return code */

	Optr = Op;		/* set Optr in case of error */
	Bad_flags |= BADINT;	/* set bad interrupt instruction flag */
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		rc = NO;
		}
	else if (Prev_flags & (class == RTSCLASS ? BADRT : (BADRT | BADRTI))){
		error ("Instruction cannot appear immediately after control register access");
		rc = NO;
		}
	if (chk_do (0))		/* too close to end of loop */
		rc = NO;
	Optr = Operand;		/* reset Optr */
	Ymove = Xmove;		/* move fields over, check after return */
	Xmove = Operand;
	Operand = Nullstr;
	return (rc);
}

/**
*
* name		do_simm - process short immediate instructions
*
* synopsis	yn = do_simm (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to short immediate
*		instructions.
*
**/
static
do_simm (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( *Optr != '#' )
		error("Immediate operand required");
	else if( get_amode(COMMA,&op1,NONE,NONE,NONE,SI) )
		if( get_amode(NONE,&op2,RSET19,NONE,NONE,NONE) ){
			if (op2.reg == CCR)
				Bad_flags |= BADRTI;
			else if (op2.reg == MR) {
				Bad_flags |= BADENDDO | BADRTI;
				if (chk_do (2))
					return(NO);
				}
			enc_T1(objout,&op1,&op2);
			return(YES);
			}
	return(NO);
}

/**
*
* name		do_andor - process AND,OR instructions
*
* synopsis	xy = do_andor (objout)
*		int xy;			XY data move class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to AND,OR instructions.
*
**/
static
do_andor (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	int xy = NONE;		/* return code */

	if( get_amode(COMMA,&op1,RSET11,NONE,NONE,SI) == NO)
		return(NONE);	/* failed */
	if( op1.mode == SIMMED){
		if( get_amode(NONE,&op2,RSET19,NONE,NONE,NONE) ){
			enc_T1(objout,&op1,&op2);
			switch (op2.reg) {
				case MR:
					Bad_flags |= BADENDDO | BADINT;
					break;
				case CCR:
					Bad_flags |= BADINT;
					break;
				}
			}
		}
	else{
		xy = ALL; /* all xy moves allowed */
		if (!objout->opcode)	/* reset base opcode */
			objout->opcode = ANDBO;
		else
			objout->opcode = ORBO;
		mod_op (OPSRC, op1.reg, &objout->opcode);
		if( get_amode(DEST,&op2,RSET1,NONE,NONE,NONE) )
			mod_op (OPDST, op2.reg, &objout->opcode);
		}
	return (xy);
}

/**
*
* name		do_accumxy - process accumxy-class instructions
*
* synopsis	yn = do_accumxy (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to accumxy-class instructions.
*
**/
static
do_accumxy (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/

	if( get_amode(class==TSTCLASS?NONE:DEST,&op1,RSET1,NONE,NONE,NONE) ){
		mod_op (OPDST, op1.reg, &objout->opcode);
		return(YES);
		}
	return(NO);
}

/**
*
* name		do_abba - process abba-class instructions
*
* synopsis	yn = do_abba (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to abba-class instructions.
*
**/
static
do_abba (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( get_amode(COMMA,&op1,RSET1,NONE,NONE,NONE) )
		if( get_amode(DEST,&op2,(op1.reg == REGA) ? RSET14 : RSET13,NONE,NONE,NONE) ){
			mod_op (OPDST, op2.reg, &objout->opcode);
			return(YES);
			}
	return(NO);
}

/**
*
* name		do_s910d1 - process s9d1 and s10d1-class instructions
*
* synopsis	yn = do_s910d1 (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to s9d1 and s10d1-class
*		instructions.
*
**/
static
do_s910d1 (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( get_amode(COMMA,&op1,class==S9D1 ? RSET10:RSET11,NONE,NONE,NONE) == NO )
		return(NO);
	mod_op (OPSRC, op1.reg, &objout->opcode);
	if( get_amode(DEST,&op2,RSET1,NONE,NONE,NONE) == NO )
		return(NO);
	mod_op (OPDST, op2.reg, &objout->opcode);
	return(YES);
}

/**
*
* name		do_s1115d1ab - process s11d1ab and s15d1ab-class instructions
*
* synopsis	yn = do_s1115d1ab (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to s11d1ab-class instructions.
*
**/
static
do_s1115d1ab (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	int flags;		/* address mode flags			*/
	int rset;		/* register set				*/

	if( get_amode(COMMA,&op1,class==S11D1AB?RSET21:RSET15,NONE,NONE,NONE) == NO )
		return(NO); /* failed */
	mod_op (OPSRC, op1.reg, &objout->opcode);
	flags = class == CMPCLASS ? NONE: DEST;
	switch(op1.reg){
		case REGA:
			rset = RSET14;
			break;
		case REGB:
			rset = RSET13;
			break;
		default: /* S11/S15,D1 operands */
			rset = RSET1;
			break;
		}
	if( get_amode(flags,&op2,rset,NONE,NONE,NONE) == NO )
		return(NO);
	mod_op (OPDST, op2.reg, &objout->opcode);
	return(YES);
}

/**
*
* name		do_rd1 - process rd1-class instructions
*
* synopsis	yn = do_rd1 (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to rd1-class instructions.
*
**/
static
do_rd1 (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( get_amode(COMMA,&op1,RSET16,NONE,NONE,NONE) == NO ||
	    get_amode(NONE,&op2,RSET1,NONE,NONE,NONE) == NO )
		return(NO);
	enc_T4(objout,&op1,&op2);
	return(YES);
}

/**
*
* name		do_luaclass - process LUA instruction
*
* synopsis	yn = do_luaclass (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to LUA instruction.
*
**/
static
do_luaclass (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( get_amode(COMMA,&op1,NONE,REA3,NONE,NONE) )
		if( get_amode(DEST,&op2,RSET20,NONE,NONE,NONE) ){
			enc_T54(objout,&op1,&op2);
			return(YES);
			}
	return(NO);
}

/**
*
* name		do_divclass - process DIV instruction
*
* synopsis	yn = do_divclass (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to DIV instruction.
*
**/
static
do_divclass (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/

	if( get_amode(COMMA,&op1,RSET11,NONE,NONE,NONE) )
		if( get_amode(NONE,&op2,RSET1,NONE,NONE,NONE) ){
			enc_T2(objout,&op1,&op2);
			return(YES);
			}
	return(NO);
}

/**
*
* name		do_bclass - process bit-class instructions
*
* synopsis	yn = do_bclass (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to bit-class instructions:
*		BSET, BCLR, BTST, BCHG.
*
**/
static
do_bclass (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	int rc = YES;		/* return code */

	if( get_amode(COMMA,&op1,NONE,NONE,NONE,NI) == NO ){
		rc = NO;	/* failed, but keep going */
		adj_optr();	/* skip over comma */
		}
	if( get_space(&op2,SCLS7) == NO )	/* Rev. C */
		return(NO); /* failed */
	if( op2.space == NONE ){
		if( get_amode(NONE,&op2,RSET17,NONE,NONE,NONE) == NO )
			return(NO); /* failed */
		if( class == BTST ){
			if (op2.reg == SSH && chk_do (2))
				rc = NO;
			}
		else{
			if (!set_badflags (op2.reg))
				rc = NO;
			}
		if( !rc ) /* error on first operand */
			return(NO);
		enc_T16a(objout,&op1,&op2);
		}
	else{
		if( get_amode(NONE,&op2,NONE,REA1,LISA,NONE) == NO ){
			objout->count = 2; /* set default word count */
			return(NO); /* failed */
			}
		if( !rc ){ /* error on first operand */
			objout->count = 2; /* set default word count */
			return(NO);
			}
		switch(op2.mode){
			case SABSOL:
				enc_T17(objout,&op1,&op2);
				break;
			case IABSOL:
				enc_T18(objout,&op1,&op2);
				break;
			case INDEXO:
			case PREDEC:
			case LABSOL:
				Cycles += EA_CYCLES;
			/* fall through */
			default:
				enc_T16(objout,&op1,&op2);
				break;
			}
		}
	return(rc);
}

/**
*
* name		do_doclass - process DO instruction
*
* synopsis	yn = do_doclass (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to DO instruction.
*
**/
static
do_doclass (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	struct dostack *newdo;	/* pointer for allocating stacked loop addresses */
	int rc = YES;		/* return code */

	Optr = Op;		/* set Optr in case of error */
	Bad_flags |= BADINT;	/* set bad interrupt instruction flag */
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		Prev_flags &= ~BADREP;	/* clear flag */
		rc = NO;
		}
	else if (Prev_flags & BADDO){
		error ("Instruction cannot appear immediately after control register access");
		rc = NO;
		}
	if (chk_do (2)) /* too close to end of loop */
		rc = NO;
	Optr = Operand;		/* reset Optr */
	objout->count = 2;	/* default even if syntax error */
	op2.space = NONE;
	if( get_space(&op1,SCLS7) == NO )
		return(NO);	/* failed */
	if( op1.space == NONE ){
		if( get_amode(COMMA,&op1,RSET17,NONE,NONE,MI) == NO ){
			rc = NO;	/* failed, but keep going */
			adj_optr();	/* skip over comma */
			}
		if( op1.mode == RDIRECT && op1.reg == SSH) {
			error("Illegal use of SSH as loop count operand");
			return(NO);
			}
		if( get_amode(NONE,&op2,NONE,NONE,LAS,NONE) == NO )
			return(NO);	/* failed */
		if( Msw_flag && !mem_compat (op2.space, PSPACE) )
			warn("Absolute address involves incompatible memory spaces");
		if( !rc )	/* error on first operand */
			return(NO);
		if( op1.mode == MIMMED )
			enc_T5g(objout,&op1,&op2);
		else
			enc_T5f(objout,&op1,&op2);
		}
	else{ /* X: or Y: loop count specified */
		if( get_amode(COMMA,&op1,NONE,REA1,SA,NONE) == NO ){
			rc = NO;	/* failed, but keep going */
			adj_optr();	/* skip over comma */
			}
		if( get_amode(NONE,&op2,NONE,NONE,LAS,NONE) == NO )
			return(NO);	/* failed */
		if( Msw_flag && !mem_compat (op2.space, PSPACE) )
			warn("Absolute address involves incompatible memory spaces");
		if( !rc )	/* error on first operand */
			return(NO);
		if( op1.mode == SABSOL )
			enc_T5e(objout,&op1,&op2);
		else
			enc_T5d(objout,&op1,&op2);
		}
#if !SASM		/* don't stack for single-line assembler */
	if (Pass == 2 && op2.secno != Csect->secno) {
		error ("DO loop address must be in current section");
		return (NO);
	}
	if (Pass == 2 && !Chkdo){	/* don't stack on first pass */
		Optr = NULL;	/* clear Optr for error processing */
		if (objout->postword <= *Pc + 1L){
			error ("Negative or empty DO loop not allowed");
			return (NO);
			}
		if (Dstk && Dstk->la <= objout->postword){
			error ("Improper nesting of DO loops");
			return (NO);
			}
		if (!(newdo = (struct dostack *)alloc (sizeof (struct dostack))))
			fatal ("Out of memory - DO loop nesting level too deep");
		newdo->la = objout->postword;
		newdo->line = NULL;
		newdo->next = Dstk;
		Dstk = newdo;		/* link in new stack entry */
		Bad_flags |= PREVDO;	/* set previous DO flag */
	}
#endif
	return (rc);
}

/**
*
* name		do_endo - process ENDDO instructions
*
* synopsis	yn = do_enddo ()
*		int yn;			YES if no error / NO if error
*
* description	Performs processing related to ENDDO instruction.
*
**/
static
do_enddo ()
{
	int rc = YES;		/* return code */
#if !SASM
	Optr = Op;		/* set Optr in case of error */
	Bad_flags |= BADINT;	/* set bad interrupt instruction flag */
	if (!Dstk)
		warn ("ENDDO instruction not inside DO loop");
	if (Prev_flags & BADENDDO){
		error ("Instruction cannot appear immediately after control register access");
		rc = NO;
		}
	if (Dstk && !Chkdo && *Pc == Dstk->la - 1L) {
		Laerr = YES;	/* set loop error flag */
		Optr = NULL;	/* reset Optr for error processing */
		error ("ENDDO instruction cannot appear at this point in DO loop");
		rc = NO;
	}
	Optr = Operand;		/* reset Optr */
	Ymove = Xmove;		/* move fields over, check after return */
	Xmove = Operand;
	Operand = Nullstr;
#endif
	return (rc);
}

/**
*
* name		do_repclas - process repeat-class instructions
*
* synopsis	yn = do_repclas (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to the REP instruction.
*
**/
static
do_repclas (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	int rc = YES;		/* return code */

	Optr = Op;		/* set Optr in case of error */
	Bad_flags |= BADREP;	/* set flags */
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		rc = NO;
		}
	if (chk_do (0)) /* too close to end of loop */
		rc = NO;
	Optr = Operand;		/* reset Optr */
	if( get_space(&op1,SCLS7) == NO )
		return(NO);	/* failed */
	if( op1.space == NONE ){
		if( get_amode(NONE,&op1,RSET17,NONE,NONE,MI) == NO )
			return(NO); /* failed */
		switch(op1.mode){
			case MIMMED:
				enc_T5c(objout,&op1);
				break;
			case RDIRECT:
				enc_T5b(objout,&op1);
				break;
			}
		}
	else{ /* X: or Y: loop count specified */
		if( get_amode(NONE,&op1,NONE,REA1,SA,NONE) == NO )
			return(NO); /* failed */
		if( op1.mode == SABSOL )
			enc_T5a(objout,&op1);
		else
			enc_T5(objout,&op1);
		}
	return(rc);
}

/**
*
* name		do_jump - process jump-class instructions
*
* synopsis	yn = do_jump (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to jump-class instructions.
*
**/
static
do_jump (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	int rc = YES;		/* return code */

	Optr = Op;		/* set Optr in case of error */
	if (chk_do (0)) /* too close to end of loop */
		rc = NO;
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		Prev_flags &= ~BADREP;	/* clear flag */
		rc = NO;
		}
	Optr = Operand;		/* reset Optr */
	op1.space = NONE;
	if( get_amode(NONE,&op1,NONE,REA1,LMA,NONE) ){
		if( Msw_flag && !mem_compat (op1.space, PSPACE) )
			warn("Absolute address involves incompatible memory spaces");
		if( op1.mode == MABSOL ){
			if (class == JUMP)
				enc_T8(objout,&op1);
			else
				enc_T11(objout,&op1);
			}
		else {
			switch (op1.mode) {
				case LABSOL:
				case INDEXO:
				case PREDEC:
					Cycles += EA_CYCLES;
				}
			if (class == JUMP)
				enc_T9(objout,&op1);
			else
				enc_T12(objout,&op1);
			}
		}
	else {
		objout->count = 2; /* set default word count */
		rc = NO;
		}
	return (rc);
}

/**
*
* name		do_jsr - process jsr-class instructions
*
* synopsis	yn = do_jsr (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to jsr-class instructions.
*
**/
static
do_jsr (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	int rc = YES;		/* return code */

	Optr = Op;		/* set Optr in case of error */
	if (chk_do (0)) /* too close to end of loop */
		rc = NO;
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		Prev_flags &= ~BADREP;	/* clear flag */
		rc = NO;
		}
	Optr = Operand;		/* reset Optr */
	op1.space = NONE;
	if( get_amode(NONE,&op1,NONE,REA1,LMA,NONE) ){
		if( Msw_flag && !mem_compat (op1.space, PSPACE) )
			warn("Absolute address involves incompatible memory spaces");
		if( op1.mode == MABSOL ){
			if (Pass == 2 && Dstk && Dstk->la == op1.val) {
				error ("Subroutine jump to loop address not allowed");
				rc = NO;
				}
			if (class == JSR)
				enc_T8(objout,&op1);
			else
				enc_T11(objout,&op1);
			}
		else {
			switch (op1.mode) {
				case LABSOL:
					if (Pass == 2 && Dstk && Dstk->la == op1.val) {
						error ("Subroutine jump to loop address not allowed");
						rc = NO;
					}
				/* fall through */
				case INDEXO:
				case PREDEC:
					Cycles += EA_CYCLES;
				}
			if (class == JSR)
				enc_T9(objout,&op1);
			else
				enc_T12(objout,&op1);
			}
		}
	else {
		objout->count = 2; /* set default word count */
		rc = NO;
		}
	return (rc);
}

/**
*
* name		do_jmpbit - process jmpbit-class instructions
*
* synopsis	yn = do_jmpbit (class, objout)
*		int yn;			YES if no error / NO if error
*		int class;		mnemonic class
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to various jmpbit-class
*		instructions.
*
**/
static
do_jmpbit (class, objout)
int class;
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	struct amode op3;	/* address mode of operand 3		*/
	int rc = YES;		/* return code */

	Optr = Op;		/* set Optr in case of error */
	if (Prev_flags & BADREP){
		error("Cannot repeat this instruction");
		Prev_flags &= ~BADREP;	/* clear flag */
		rc = NO;
		}
	Optr = Operand;		/* reset Optr */
	objout->count = 2;	/* default even if syntax error */
	if( get_amode(COMMA,&op1,NONE,NONE,NONE,NI) == NO ){
		rc = NO;	/* failed, but keep going */
		adj_optr();	/* skip over comma */
		}
	if( get_space(&op2,SCLS7) == NO )	/* Rev. C */
		return(NO); /* failed */
	if( op2.space == NONE ){
		if( get_amode(COMMA,&op2,RSET17,NONE,NONE,NONE) == NO ){
			rc = NO;	/* failed, but keep going */
			adj_optr();	/* skip over comma */
			}
		if( op2.reg == SSH && chk_do (2) )
			rc = NO;
		}
	else{
		if( get_amode(COMMA,&op2,NONE,REA1,ISA,NONE) == NO ){
			rc = NO;	/* failed, but keep going */
			adj_optr();	/* skip over comma */
			}
		}
	op3.space = NONE;
	if( get_amode(NONE,&op3,NONE,NONE,LAS,NONE) == NO )
		return(NO); /* failed */
	if( Msw_flag && !mem_compat (op3.space, PSPACE) )
		warn("Absolute address involves incompatible memory spaces");
	if( !rc )	/* error on first or second operand */
		return(NO);
	if (Pass == 2 && class == JSCLR && Dstk && Dstk->la == op3.val) {
		error ("Subroutine jump to loop address not allowed");
		return (NO);
	}
	if( op2.mode == RDIRECT )			/* Rev. C */
		enc_T19a(objout,&op1,&op2,&op3);
	else if( op2.mode == SABSOL )
		enc_T20(objout,&op1,&op2,&op3);
	else if( op2.mode == IABSOL )
		enc_T21(objout,&op1,&op2,&op3);
	else {
		enc_T19(objout,&op1,&op2,&op3);
		switch (op2.mode) {
			case INDEXO:
			case PREDEC:
				Cycles += EA_CYCLES;
			}
		}
	return(rc);
}

/**
*
* name		do_tfrcc - process conditional transfer-class instructions
*
* synopsis	yn = do_tfrcc (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to conditional transfer-class
*		instructions.
*
**/
static
do_tfrcc (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	struct amode op3;	/* address mode of operand 3		*/
	struct amode op4;	/* address mode of operand 4		*/
	int regset;		/* destination register set		*/
	int rc = YES;		/* return code */

	if( get_amode(COMMA,&op1,RSET15,NONE,NONE,NONE) == NO )
		return(NO); /* failed */
	regset = op1.reg == REGA ? RSET14 :
	         op1.reg == REGB ? RSET13 :
				   RSET1;
	if( get_amode(NONE,&op2,regset,NONE,NONE,NONE) == NO )
		return(NO); /* failed */
	if( *Xmove == EOS )
		enc_T15(objout,&op1,&op2); /* no R,R specified */
	else{ /* R,R specified */
		if( *Ymove != EOS ){
			error("Too many fields specified for instruction");
			return(NO);
			}
		Optr = Xmove; /* reset Optr */
		if( get_amode(COMMA,&op3,RSET16,NONE,NONE,NONE) == NO )
			return(NO); /* failed */
		if( get_amode(DEST,&op4,RSET16,NONE,NONE,NONE) ){
			enc_T14(objout,&op1,&op2,&op3,&op4);
			}
		}
	return(rc);
}

/**
*
* name		do_maclass - process MAC,MPY instructions
*
* synopsis	yn = do_maclass (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs processing related to MAC,MPY instructions.
*
**/
static
do_maclass (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	struct amode op3;	/* address mode of operand 3		*/
	int plus = YES;		/* polarity for MACLASS			*/

	if( *Optr == '+' )
		++Optr;
	else if( *Optr == '-' ){
		++Optr;
		plus = NO;
		}
	if( get_amode(COMMA,&op1,RSET11,NONE,NONE,NONE) )
		if( get_amode(COMMA,&op2,RSET11,NONE,NONE,NONE) )
			if( get_amode(DEST,&op3,RSET1,NONE,NONE,NONE) ){
				(void)mod_mulop(plus, op1.reg, op2.reg, op3.reg, &objout->opcode);
				return(YES);
				}
	return(NO);
}

/**
*
* name		do_move - process MOVE instructions
*
* synopsis	yn = do_move ()
*		int yn;			YES if no error / NO if error
*
* description	Performs processing related to MOVE instructions.
*
**/
static
do_move ()
{
	if( *Ymove != EOS ){
		error("Too many fields specified for instruction");
		return(NO);
		}
	Ymove = Xmove;		/* move all fields over */
	Xmove = Operand;
	Operand = Nullstr;	/* set Operand to null string */
	if( *Xmove == EOS ){
		error("Not enough fields specified for instruction");
		return(NO);
		}
	return(YES);
}

/**
*
* name		do_movep - process MOVEP instruction
*
* synopsis	yn = do_movep (objout)
*		int yn;			YES if no error / NO if error
*		struct obj *objout;	pointer to object code structure
*
* description	Performs all processing related to MOVEP instruction.
*		First checks that appropriate register and mode combinations
*		are used.  Then reconciles operand addressing modes.
*		Finally increments cycle counters and encodes instruction.
*
**/
static
do_movep (objout)
struct obj *objout;
{
	struct amode op1;	/* address mode of operand 1		*/
	struct amode op2;	/* address mode of operand 2		*/
	int failed = NO;
	int rc;
	int op2scls = SCLS5;

	if( *Xmove ){
		error("Too many fields specified for instruction");
		return(NO);
		}
	Ymove = Xmove;		/* move all fields over */
	Xmove = Operand;
	Operand = Nullstr;	/* set operand field to null string */
	if( !*Xmove ){
		error("Not enough fields specified for instruction");
		return(NO);
		}

	if( get_space(&op1,SCLS9) == NO )
		return (NO);	/* failed */
	switch( op1.space ){
		case NONE:
			rc = get_amode(COMMA,&op1,RSET17,NONE,NONE,LI);
			break;
		case PSPACE:
			rc = get_amode(COMMA,&op1,NONE,REA1,LAS,NONE);
			break;
		default:
			rc = get_amode(COMMA,&op1,NONE,REA1,LIA,NONE);
			op2scls = SCLS9;
			break;
		}
	if( !rc ){		/* address mode error */
		objout->count = 2; /* set default word count */
		failed = YES;	/* failed, but keep going */
		adj_optr();	/* skip over comma */
		}
	if( get_space(&op2,op2scls) == NO )
		return (NO);	/* failed */
	if( (op2.space == NONE || op2.space == PSPACE) &&
	    op1.space != XSPACE && op1.space != YSPACE ){
		error("Either source or destination memory space must be X or Y");
		return (NO);	/* failed */
		}
	switch( op2.space ){
		case NONE:
			rc = get_amode(NONE,&op2,RSET17,NONE,NONE,NONE);
			break;
		case PSPACE:
			rc = get_amode(NONE,&op2,NONE,REA1,LAS,NONE);
			break;
		default:
			rc = get_amode(NONE,&op2,NONE,REA1,LIA,NONE);
			break;
		}
	if( !rc ){			/* address mode error */
		objout->count = 2;	/* set default word count */
		return (NO);		/* failed */
		}
	if( failed )			/* error on first operand */
		return (NO);

	/* check DO loop address and other restrictions */
	if (Dstk && (op1.reg == SSH || (op2.reg >= SR && op2.reg <= SP)))
		(void)chk_do (2);	/* check if close to end of loop */
	if (Dstk && (op1.reg == LC || op1.reg == SP || op1.reg == SSL))
		(void)chk_do (1);	/* check if close to end of loop */
	if ((op1.reg == SSH || op1.reg == SSL) && (Prev_flags & BADMOVESS))
		error ("Move from SSH or SSL cannot follow move to SP");
	if (op1.reg == SSH)
		Bad_flags |= BADRT | BADDO | BADENDDO | BADINT;
	switch (op2.reg) {
		case SR:
			Bad_flags |= BADENDDO | BADRTI | BADINT;
			break;
		case LA:
		case LC:
			Bad_flags |= BADDO | BADENDDO | BADINT;
			break;
		case SP:
			Bad_flags |= BADMOVESS | BADINT;
		/* fall through */
		case SSH:
		case SSL:
			Bad_flags |= BADRT | BADDO | BADENDDO | BADINT;
			break;
		}

	/* now reconcile the modes of the two operands */
	if( op1.mode == IABSOL && op2.mode == IABSOL ){ /* both short IO */
		if( op1.force && op2.force ) /* if both are forced */
			warn("Cannot force I/O short addressing for source and destination");
		if( op1.space == PSPACE )
			op1.mode = LABSOL; /* convert to long absolute */
		else
			op2.mode = LABSOL;
		}

	/* neither is IO short */
	else if( op1.mode != IABSOL && op2.mode != IABSOL ){

		/* neither is absolute */
		if( op1.mode != LABSOL && op2.mode != LABSOL ){
			error("I/O short addressing must be used for either source or destination");
			return (NO);
			}

		/* both are absolute */
		if( op1.mode == LABSOL && op2.mode == LABSOL ){

			/* both are forced long */
			if( op1.force && op2.force ){
				error("I/O short addressing must be used for either source or destination");
				return (NO);
				}

			/* first operand forced long */
			if( op1.force ){
				/* default second to IO short */
				op2.mode = IABSOL;
				if( op2.space == PSPACE ||
				    (Pass == 2 && !ioaddress(op2.val)) ){
					error("I/O short addressing must be used for either source or destination");
					objout->count = 2; /* set up to avoid phasing errors */
					return (NO);
					}
				}

			/* second operand forced long */
			else if( op2.force ){
				/* default first to IO short */
				op1.mode = IABSOL;
				if( op1.space == PSPACE ||
				    (Pass == 2 && !ioaddress(op1.val)) ){
					error("I/O short addressing must be used for either source or destination");
					objout->count = 2; /* set up to avoid phasing error */
					return (NO);
					}
				}

			/* neither operand is forced */
			else{
				op1.mode = IABSOL; /* default */
				if( Pass == 2 ){
					/* guessed wrong */
					if( !ioaddress(op1.val) ){
						if( !ioaddress(op2.val) ){
							error("I/O short addressing must be used for either source or destination");
							objout->count = 2; /* set up to avoid phasing error */
							return (NO);
							}
						else{
							op2.mode = IABSOL;
							op1.mode = LABSOL;
							}
						}
					}
				}
			}

		/* only first operand uses absolute */
		else if( op1.mode == LABSOL ){
			if( op1.force || op1.space == PSPACE ){
				error("I/O short addressing must be used for either source or destination");
				return (NO);
				}
			op1.mode = IABSOL; /* convert to IO short */
			if( Pass == 2 && !ioaddress(op1.val) ){
				error("I/O short addressing must be used for either source or destination");
				return (NO);
				}
			}

		/* only second operand uses absolute */
		else if( op2.mode == LABSOL ){
			if( op2.force || op2.space == PSPACE ){
				error("I/O short addressing must be used for either source or destination");
				return (NO);
				}
			op2.mode = IABSOL; /* convert to IO short */
			if( Pass == 2 && !ioaddress(op2.val) ){
				error("I/O short addressing must be used for either source or destination");
				return (NO);
				}
			}
		}

	/* adjust cycles counts if necessary */
	if (Cyc_flag) {
		if ( op1.space == PSPACE || op2.space == PSPACE)
			Cycles += MVP_CYCLES;
		else if (! (op1.space == NONE || op2.space == NONE) )
			if (op1.mode == IABSOL)
				switch (op2.mode) {
					case INDEXO:
					case PREDEC:
					case LABSOL:
						Cycles += EA_CYCLES;
					}
			else if (op2.mode == IABSOL)
				switch (op1.mode) {
					case INDEXO:
					case PREDEC:
					case LIMMED:
					case LABSOL:
						Cycles += EA_CYCLES;
					}
		}

	if( op1.mode == IABSOL )
		if ( op2.mode == RDIRECT)
			enc_T26(objout,&op1,&op2);
		else
			enc_T26a(objout,&op1,&op2);
	else
		if ( op1.mode == RDIRECT)
			enc_T26b(objout,&op1,&op2);
		else
			enc_T26c(objout,&op1,&op2);

	return (YES);
}

/**
*
* name		unstk_do - unstack DO loop addresses
*
* synopsis	unstk_do (objout);
*
* description	Clears the DO stack of addresses lower than the current
*		program counter value.	Uses count field in obj structure
*		to adjust for two-word instructions.  Also checks the first
*		loop instruction against the last loop instruction for
*		pipeline violations.
*
**/
static
unstk_do (objout)
struct obj *objout;
{
	long pc;		/* program counter value */
	struct dostack *olddo;	/* pointer to freed loop address value */

	pc = *Pc;		/* get current program counter value */
	if (objout->count > 1)	/* two-word instruction */
		pc++;

	while (Dstk && pc >= Dstk->la) {	/* clear DO stack */
		if (Dstk->line)
			reproc_line (objout);	/* reprocess first instr. */
		olddo = Dstk;
		Dstk = olddo->next;
		free ((char *)olddo);
	}
}

/**
*
* name		reproc_line - reprocess first DO loop instruction line
*
* synopsis	reproc_line (objout)
*
* description	Reprocesses initial DO loop instruction line saved in
*		the DO stack.  Preserves register and previous instruction
*		flags; calls process() via indirect recursion to parse and
*		and process line.
*
**/
static
reproc_line (objout)
struct obj *objout;
{
	long savreg;			/* saved register sets */
	int flags;			/* saved flags */

	save_pword(objout->opcode);	/* save words for print */
	if (objout->count > 1)
		save_pword(objout->postword);

	Ctotal += Cycles;		/* bump cycle count */
	print_line(' ');		/* print this line */
	Abortp = YES;			/* abort subsequent print */

	savreg = Regmov;		/* save registers */
	flags = Bad_flags;		/* save flags */
	Chkdo = YES;			/* set flag */
	(void)strcpy (Line, Dstk->line);/* set up line */
	if (parse_line())		/* parse line */
		(void)process(NO,NO);	/* reprocess line */
	Chkdo = NO;			/* reset flag */
	Bad_flags = flags;		/* restore flags */
	Regmov = savreg;		/* restore registers */

	Ctotal -= Cycles;		/* readjust cycle total */
	Cycles = 0;			/* clear cycle count */
}

/**
*
* name		chk_do - check DO loop address range
*
* synopsis	err = chk_do (range);
*		int err;	Loop error flag value
*		int range;	Range from loop address to check
*
* description	Compares current program counter with value at top of
*		DO loop stack, adjusted for range parameter.  Returns
*		YES and issues an error if pc is within loop address range.
*		Returns NO otherwise.
*
**/
chk_do (range)
int range;
{
	long la;		/* loop address at top of stack */
	long pc;		/* program counter value */
	register i;
	char *ptr;		/* saved Optr */

	if (!Dstk)		/* not in loop */
		return (NO);
	if (Chkdo)		/* just checking initial DO instruction */
		return (NO);
	la = Dstk->la;		/* get loop address at top of stack */
	pc = *Pc;		/* get current program counter value */
	for (i = range; i >= 0; i--)
		if (pc == (la - (long)i))
			break;
	if (i >= 0 && !Laerr) { /* pc within loop address range */
		Laerr = YES;	/* set loop error flag */
		ptr = Optr;
		Optr = NULL;	/* reset Optr for error processing */
		switch (range) {
			case 0: error ("Instruction cannot appear at last address of a DO loop");
				break;
			case 1: error ("Instruction cannot appear within last 2 words of a DO loop");
				break;
			case 2: error ("Instruction cannot appear within last 3 words of a DO loop");
		}
		Optr = ptr;	/* restore Optr */
	}
	return (Laerr);		/* return current setting of error flag */
}

/**
*
* name		adj_optr - adjust op pointer
*
* synopsis	adj_optr ()
*
* description	If the global op pointer Optr is valid, non-null, and
*		points to a comma, increment it.
*
**/
static
adj_optr ()
{
	if (Optr && *Optr && *Optr == ',')
		Optr++;
}

/**
*
* name		mod_op - modify opcode based on sdflag, reg, and base
*			 opcode value
*
* synopsis	mod_op (sdflag, reg, opcode)
*		int sdflag;	opcode source/destination flag
*		int reg;	source/destination register number
*		long *opcode;	pointer to opcode
*
* description	Modifies opcode based on reg number and whether reg
*		is source or destination.  Uses the base opcode value to
*		resolve source register ambiguities.
*
**/
static
mod_op (sdflag, reg, opcode)
int sdflag, reg;
long *opcode;
{
	long mask;

	if (sdflag == OPDST)
		mask = (reg == REGA ? (long)0 : (long)8);
	else
		switch (reg) {
			case REGX:
				mask = 0x20L;
				break;
			case REGY:
				mask = 0x30L;
				break;
			case REGX0:
				mask = 0x40L;
				break;
			case REGY0:
				mask = 0x50L;
				break;
			case REGX1:
				mask = 0x60L;
				break;
			case REGY1:
				mask = 0x70L;
				break;
			case REGA:
			case REGB:
				mask = *opcode & ((long)7);
				switch ((int)mask){
			/* TFR	*/	case 0x01:
			/* CMP	*/	case 0x05:
			/* CMPM */	case 0x07:
						mask = 0L;
						break;
			/* ADD	*/	case 0x00:
			/* SUB	*/	case 0x04:
						mask = 0x10L;
						break;
				}
		}
	*opcode |= mask;
}

/**
*
* name		mod_mulop - modify multiply instruction encoding
*
* synopsis	yn = mod_mulop (plus, reg1, reg2, reg3, opcode)
*		int yn;		YES if registers OK, NO otherwise
*		int plus;	sign flag
*		int reg1;	First source register number
*		int reg2;	Second source register number
*		int reg3;	Destination register number
*		long *opcode;	Pointer to opcode value
*
* description	Insures that register combinations on multiply instructions
*		are valid.  Returns YES if register pair is valid, NO
*		otherwise.  Generates an error if registers are invalid.
*		Also updates opcode based on valid register combinations.
*
**/
static
mod_mulop (plus, reg1, reg2, reg3, opcode)
int plus, reg1, reg2, reg3;
long *opcode;
{
	long mask = 0L;

	switch (reg1) {
		case REGX0:
			switch (reg2) {
				case REGX0:
					break;
				case REGX1:
					mask = 0x20L;
					break;
				case REGY0:
					mask = 0x50L;
					break;
				case REGY1:
					mask = 0x40L;
					break;
				default:
					error ("Invalid register combination");
					return (NO);
			}
			break;
		case REGX1:
			switch (reg2) {
				case REGX0:
					mask = 0x20L;
					break;
				case REGY0:
					mask = 0x60L;
					break;
				case REGY1:
					mask = 0x70L;
					break;
				default:
					error ("Invalid register combination");
					return (NO);
			}
			break;
		case REGY0:
			switch (reg2) {
				case REGX0:
					mask = 0x50L;
					break;
				case REGX1:
					mask = 0x60L;
					break;
				case REGY0:
					mask = 0x10L;
					break;
				case REGY1:
					mask = 0x30L;
					break;
				default:
					error ("Invalid register combination");
					return (NO);
			}
			break;
		case REGY1:
			switch (reg2) {
				case REGX0:
					mask = 0x40L;
					break;
				case REGX1:
					mask = 0x70L;
					break;
				case REGY0:
					mask = 0x30L;
					break;
				default:
					error ("Invalid register combination");
					return (NO);
			}
			break;
		default:	/* should not get here */
			fatal ("mulreg failure");
			return (NO);
	}

	if (reg3 == REGB)
		mask |= 8L;
	if (!plus)
		mask |= 4L;
	*opcode |= mask;

	return (YES);
}
