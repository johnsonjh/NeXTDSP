#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		do_xy - process parallel data fields
*
* synopsis	yn = do_xy(op,xyclass,xsrc,xdst,ysrc,ydst)
*		int yn;			NO if error occurs
*		struct obj *op;		object code structure
*		int xyclass;		data transfer class
*		struct amode *xsrc;	pointer to x source
*		struct amode *xdst;	pointer to x dest
*		struct amode *ysrc;	pointer to y source
*		struct amode *ydst;	pointer to y dest
*
* description	Handles data transfer field processing for instructions
*		that allow parallel data movements.
*
**/
do_xy(op,xyclass,xsrc,xdst,ysrc,ydst)
struct obj *op;
int xyclass;
struct amode *xsrc, *xdst, *ysrc, *ydst;
{
	Optr = Xmove; /* set up to examine xmove field */
	if (get_space(xsrc,SCLS6) == NO)
		return (NO);	/* bad space */
	if( xsrc->space == NONE ){
		if( init_amode(xsrc) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* failed to find anything */
			}
		/* check for address register update */
		if( xsrc->mode >= POSTINC && xsrc->mode <= PSTDECO ){
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T50(op,xsrc);
			return(YES);
			}
		if( (xsrc->mode >= NOUPDATE && xsrc->mode <= PREDEC) || (xsrc->mode >= LABSOL && xsrc->mode <= SABSOL) ){
			error("Missing memory space specifier");
			op->count = 2; /* set default word count */
			return(NO);
			}
		if( xsrc->mode == SIMMED )
			return(p_si(xyclass,op,xsrc,xdst,ysrc,ydst));
		if( xsrc->mode == LIMMED )
			return(p_li(xyclass,op,xsrc,xdst,ysrc,ydst));

		/* fall through to register direct processing */
		switch(xsrc->reg){
			case REGX:
			case REGY:
			case REGAB:
			case REGBA:
			case REGA10:
			case REGB10:
				return(p_xyabba(op,xsrc,xdst));
			case REGA:
			case REGB:
				return(p_ab(xyclass,op,xsrc,xdst,ysrc,ydst));
			case REGX0:
			case REGX1:
				return(p_x0x1(xyclass,op,xsrc,xdst,ysrc,ydst));
			case REGY0:
			case REGY1:
			case REGA0:
			case REGB0:
			case REGA1:
			case REGB1:
			case REGA2:
			case REGB2:
			case REGR0:
			case REGR1:
			case REGR2:
			case REGR3:
			case REGR4:
			case REGR5:
			case REGR6:
			case REGR7:
			case REGN0:
			case REGN1:
			case REGN2:
			case REGN3:
			case REGN4:
			case REGN5:
			case REGN6:
			case REGN7:
				return(p_y0_n7(xyclass,op,xsrc,xdst,ysrc,ydst));
			case REGM0:
			case REGM1:
			case REGM2:
			case REGM3:
			case REGM4:
			case REGM5:
			case REGM6:
			case REGM7:
				return(p_m0_m7(xyclass,op,xsrc,xdst));
			case OMR:
			case SR:
			case LA:
			case LC:
			case SSH:
			case SSL:
			case SP:
				return(p_omr_sp(xyclass,op,xsrc,xdst));
			}
		}
	if( get_amode(COMMA,xsrc,NONE,REA1,LSA,NONE) == NO ){
		op->count = 2; /* set default word count */
		return(NO); /* failed to find anything */
		}
	switch( xsrc->space ){
		case XSPACE:
			return(p_xs(xyclass,op,xsrc,xdst,ysrc,ydst));
		case YSPACE:
			return(p_ys(xyclass,op,xsrc,xdst));
		case LSPACE:
			return(p_ls(op,xsrc,xdst));
		case PSPACE:
			return(p_ps(xyclass,op,xsrc,xdst));
		}
	return(NO);
}

/**
*
* name		p_si - process short immediate moves
*
* synopsis	yn = p_si(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with a
*		short immediate as the X source field.
*
**/
static
p_si(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
		return(NO); /* no valid destination */
	switch( set_badflags (xdst->reg) ){
		case SR:
		case LA:
		case LC:
		case SSH:
		case SSL:
		case SP:
			(void)chk_do (2);	/* check if too close to end of loop */
		/* fall through */
		case REGM0:
		case REGM1:
		case REGM2:
		case REGM3:
		case REGM4:
		case REGM5:
		case REGM6:
		case REGM7:
		case OMR:
			if( xyclass != MOVES ){
				error("Instruction does not allow data movement specified");
				return(NO);
				}
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T28(op,xsrc,xdst);
			break;
		case REGA:
		case REGB:
		case REGX0:
		case REGX1:
			if( *Ymove == EOS ){
				enc_T46(op,xsrc,xdst);
				break;
				}
			Optr = Ymove; /* reset Optr */
			if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
				return(NO); /* error trap */
			if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
				return(NO); /* error trap */
			xsrc->mode = LIMMED; /* promote mode to long immediate */
			if( xsrc->force )
				warn("Cannot force short immediate with this parallel move");
			enc_T43a(op,xsrc,xdst,ysrc,ydst);
			break;
		case REGY0:
		case REGY1:
		case REGR0:
		case REGR1:
		case REGR2:
		case REGR3:
		case REGR4:
		case REGR5:
		case REGR6:
		case REGR7:
		case REGN0:
		case REGN1:
		case REGN2:
		case REGN3:
		case REGN4:
		case REGN5:
		case REGN6:
		case REGN7:
		case REGA0:
		case REGB0:
		case REGA1:
		case REGB1:
		case REGA2:
		case REGB2:
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T46(op,xsrc,xdst);
			break;
		default:
			error("Illegal X field destination register specified");
			return(NO);
		}

	return(YES);
}

/**
*
* name		p_li - process long immediate moves
*
* synopsis	yn = p_li(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with a
*		long immediate as the X source field.
*
**/
static
p_li(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
		return(NO); /* no valid destination */
	Cycles += EA_CYCLES;
	switch( set_badflags (xdst->reg) ){
		case REGM0:
		case REGM1:
		case REGM2:
		case REGM3:
		case REGM4:
		case REGM5:
		case REGM6:
		case REGM7:
			if( xyclass != MOVES ){
				error("Instruction does not allow data movement specified");
				return(NO);
				}
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T32a(op,xsrc,xdst);
			break;
		case SR:
		case LA:
		case LC:
		case SSH:
		case SSL:
		case SP:
			(void)chk_do (2);	/* check if too close to end of loop */
		/* fall through */
		case OMR:
			if( xyclass != MOVES ){
				error("Instruction does not allow data movement specified");
				return(NO);
				}
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T27a(op,xsrc,xdst);
			break;
		case REGA:
		case REGB:
		case REGX0:
		case REGX1:
			if( *Ymove == EOS ){
				enc_T38a(op,xsrc,xdst);
				break;
				}
			Optr = Ymove; /* reset Optr */
			if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
				return(NO); /* error trap */
			if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
				return(NO); /* error trap */
			enc_T43a(op,xsrc,xdst,ysrc,ydst);
			break;
		case REGY0:
		case REGY1:
		case REGR0:
		case REGR1:
		case REGR2:
		case REGR3:
		case REGR4:
		case REGR5:
		case REGR6:
		case REGR7:
		case REGN0:
		case REGN1:
		case REGN2:
		case REGN3:
		case REGN4:
		case REGN5:
		case REGN6:
		case REGN7:
		case REGA0:
		case REGB0:
		case REGA1:
		case REGB1:
		case REGA2:
		case REGB2:
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T38a(op,xsrc,xdst);
			break;
		default:
			error("Illegal X field destination register specified");
			return(NO);
		}

	return(YES);
}

/**
*
* name		p_xyabba - process moves that start with X,Y,AB,BA,A10,B10
*
* synopsis	yn = p_xyabba(op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with an
*		X,Y,AB,BA,A10, or B10 source in the X source field.
*
**/
static
p_xyabba(op,xsrc,xdst)
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( get_space(xdst,SCLS3) == NO )
		return(NO);
	if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
		op->count = 2; /* set default word count */
		return(NO); /* no valid destination */
		}
	switch( xdst->mode ){
		case INDEXO:
		case PREDEC:
		case LABSOL:
			Cycles += EA_CYCLES;
		/* fall through */
		case NOUPDATE:
		case POSTINC:
		case POSTDEC:
		case PSTINCO:
		case PSTDECO:
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				op->count = 2; /* set default word count */
				return(NO);
				}
			enc_T45(op,xsrc,xdst);
			break;
		case SABSOL:
			if( *Ymove != EOS ){
				Optr = Swap ? Xmove : Ymove;
				error("Y move field not allowed");
				return(NO);
				}
			enc_T44(op,xsrc,xdst);
			break;
		default:
			/* should never get here */
			fatal("p_xyabba failure");
			return(NO);
		}

	return(YES);
}


/**
*
* name		p_ab - process moves that start with A,B
*
* synopsis	yn = p_ab(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with an
*		A or B source in the X source field.
*
**/
static
p_ab(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( get_space(xdst,SCLS6) == NO )
		return(NO); /* failed */
	if( xdst->space == NONE ){
		if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
			return(NO); /* no valid destination */
		switch( set_badflags (xdst->reg) ){
			case SR:
			case LA:
			case LC:
			case SSH:
			case SSL:
			case SP:
				(void)chk_do (2);	/* check if too close to end of loop */
			/* fall through */
			case REGM0:
			case REGM1:
			case REGM2:
			case REGM3:
			case REGM4:
			case REGM5:
			case REGM6:
			case REGM7:
			case OMR:
				if( xyclass != MOVES ){
					error("Instruction does not allow data movement specified");
					return(NO);
					}
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T29a(op,xsrc,xdst);
				break;
			case REGA:
			case REGB:
			case REGY0:
			case REGY1:
			case REGR0:
			case REGR1:
			case REGR2:
			case REGR3:
			case REGR4:
			case REGR5:
			case REGR6:
			case REGR7:
			case REGN0:
			case REGN1:
			case REGN2:
			case REGN3:
			case REGN4:
			case REGN5:
			case REGN6:
			case REGN7:
			case REGA0:
			case REGB0:
			case REGA1:
			case REGB1:
			case REGA2:
			case REGB2:
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T49(op,xsrc,xdst);
				break;
			case REGX0:
			case REGX1:
				if( *Ymove == EOS ){
					enc_T49(op,xsrc,xdst);
					break;
					}
				Optr = Ymove; /* reset Optr */
				if( get_space(ysrc,SCLS8) == NO )
					return(NO);
				if( ysrc->space == NONE ){
					if( get_amode(COMMA,ysrc,RSET4,NONE,NONE,LI) == NO ){
						op->count = 2; /* set default word count */
						return(NO); /* error trap */
						}
					if( ysrc->mode == LIMMED ){
						if( get_amode(DEST,ydst,RSET4,NONE,NONE,NONE) == NO )
							return(NO);
						enc_T42a(op,xsrc,xdst,ysrc,ydst);
						break;
						}
					if( get_space(ydst,SCLS2) == NO )
						return(NO);
					if( get_amode(NONE,ydst,NONE,REA1,LAS,NONE) == NO ){
						op->count = 2; /* set default word count */
						return(NO); /* error trap */
						}
					switch( ydst->mode ){
						case INDEXO:
						case PREDEC:
						case LABSOL:
							Cycles += EA_CYCLES;
						}
					enc_T42(op,xsrc,xdst,ysrc,ydst);
					}
				else{
					if( get_amode(COMMA,ysrc,NONE,REA1,LAS,NONE) == NO ){
						op->count = 2; /* set default word count */
						return(NO);
						}
					switch( ysrc->mode ){
						case INDEXO:
						case PREDEC:
						case LABSOL:
							Cycles += EA_CYCLES;
						}
					if( get_amode(DEST,ydst,RSET4,NONE,NONE,NONE) == NO )
						return(NO);
					enc_T42a(op,xsrc,xdst,ysrc,ydst);
					}
				break;
			default:
				error("Illegal X field destination register specified");
				return(NO);
			}
		return(YES);
		}
	else return (xdst_mem (xyclass,op,xsrc,xdst,ysrc,ydst));
}

/**
*
* name		p_x0x1 - process moves that start with x0,x1
*
* synopsis	yn = p_x0x1(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with an
*		x0 or x1 source in the X source field.
*
**/
static
p_x0x1(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( get_space(xdst,SCLS9) == NO )
		return(NO); /* failed */
	if( xdst->space == NONE ){
		if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
			return(NO); /* no valid destination */
		switch( set_badflags (xdst->reg) ){
			case SR:
			case LA:
			case LC:
			case SSH:
			case SSL:
			case SP:
				(void)chk_do (2);	/* check if too close to end of loop */
			/* fall through */
			case REGM0:
			case REGM1:
			case REGM2:
			case REGM3:
			case REGM4:
			case REGM5:
			case REGM6:
			case REGM7:
			case OMR:
				if( xyclass != MOVES ){
					error("Instruction does not allow data movement specified");
					return(NO);
					}
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T29a(op,xsrc,xdst);
				break;
			case REGA:
			case REGB:
			case REGX0:
			case REGX1:
			case REGY0:
			case REGY1:
			case REGR0:
			case REGR1:
			case REGR2:
			case REGR3:
			case REGR4:
			case REGR5:
			case REGR6:
			case REGR7:
			case REGN0:
			case REGN1:
			case REGN2:
			case REGN3:
			case REGN4:
			case REGN5:
			case REGN6:
			case REGN7:
			case REGA0:
			case REGB0:
			case REGA1:
			case REGB1:
			case REGA2:
			case REGB2:
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T49(op,xsrc,xdst);
				break;
			default:
				error("Illegal X field destination register specified");
				return(NO);
			}
		return(YES);
		}
	else return (xdst_mem (xyclass,op,xsrc,xdst,ysrc,ydst));
}

/**
*
* name		xdst_mem - process X destination field moves to memory
*
* synopsis	yn = xdst_mem (xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes A, B, X0, or X1 register to memory moves.
*
**/
static
xdst_mem(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( xdst->space == XSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		switch( xdst->mode ){
			case NOUPDATE:
			case POSTINC:
			case POSTDEC:
			case PSTINCO:
				if( *Ymove == EOS ){
					enc_T38(op,xsrc,xdst);
					break;
					}
				Optr = Ymove; /* reset Optr */
				if( get_space(ysrc,SCLS8) == NO )
					return(NO); /* failed */
				if( ysrc->space == NONE ){
					if( get_amode(COMMA,ysrc,RSET23,NONE,NONE,NONE) == NO )	/* Rev. C */
						return(NO);
					switch( ysrc->reg ){
						case REGA:
						case REGB:
							if( get_space(ydst,SCLS8) == NO )
								return(NO);
							if( ydst->space == NONE ){
								if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
									return(NO);
								enc_T43(op,xsrc,xdst,ysrc,ydst);
								}
							else{
								if( get_amode(NONE,ydst,NONE,REA2,NONE,NONE) == NO )
									return(NO);
								if (r_intersect (xdst->reg, ydst->reg))
									return (NO);
								enc_T37(op,xsrc,xdst,ysrc,ydst);
								}
							break;
						case REGY0:
						case REGY1:
							if( get_space(ydst,SCLS2) == NO )
								return(NO);
							if( get_amode(NONE,ydst,NONE,REA2,NONE,NONE) == NO )
								return(NO);
							if (r_intersect (xdst->reg, ydst->reg))
								return (NO);
							enc_T37(op,xsrc,xdst,ysrc,ydst);
							break;
						case REGX0:	/* Rev. C */
							if (xsrc->reg != REGA && xsrc->reg != REGB){
								error ("Invalid addressing mode");
								return (NO);
								}
							if( get_amode(DEST,ydst,RSET1,NONE,NONE,NONE) == NO )
								return(NO);
							enc_T37d(op,xsrc,xdst);
							break;
						}
					}
				else{
					if( get_amode(COMMA,ysrc,NONE,REA2,NONE,NONE) == NO )
						return(NO);
					if (r_intersect (xdst->reg, ysrc->reg))
						return (NO);
					if( get_amode(DEST,ydst,RSET4,NONE,NONE,NONE) == NO )
						return(NO);
					enc_T37b(op,xsrc,xdst,ysrc,ydst);
					}
				break;
			case INDEXO:
			case PREDEC:
				Cycles += EA_CYCLES;
			/* fall through */
			case PSTDECO:			/* Rev. C */
				if( *Ymove == EOS ){
					enc_T38(op,xsrc,xdst);
					break;
					}
				Optr = Ymove;
				if( get_amode(COMMA,ysrc,RSET22,NONE,NONE,NONE) == NO )
					return(NO);
				if( get_amode(NONE,ydst,ysrc->reg==REGX0?RSET1:RSET2,NONE,NONE,NONE) == NO )
					return(NO);
				if (ysrc->reg == REGX0)
					enc_T37d(op,xsrc,xdst);
				else
					enc_T43(op,xsrc,xdst,ysrc,ydst);
				break;
			case LABSOL:			/* Rev. C */
				Cycles += EA_CYCLES;
				if( *Ymove == EOS ){
					enc_T38(op,xsrc,xdst);
					break;
					}
				Optr = Ymove;
				if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
					return(NO);
				if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
					return(NO);
				enc_T43(op,xsrc,xdst,ysrc,ydst);
				break;
			case SABSOL:
				if( *Ymove == EOS ){
					enc_T40(op,xsrc,xdst);
					break;
					}
				xdst->mode = LABSOL; /* promote addressing to long absolute */
				Cycles += EA_CYCLES;
				if( xdst->force )
					warn("Short absolute address cannot be forced - long substituted");
				Optr = Ymove; /* reset Optr */
				if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
					return(NO);
				if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
					return(NO);
				enc_T43(op,xsrc,xdst,ysrc,ydst);
				break;
			default:
				/* should never get here */
				fatal("xdst_mem failure");
			}
		}
	else if( xdst->space == YSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		if( *Ymove != EOS ){
			error("Too many parallel move fields specified");
			op->count = 2; /* set default word count */
			return(NO);
			}
		Ymove = Xmove; /* move fields over */
		Xmove = Nullstr;
		if( xdst->mode == SABSOL )
			enc_T41(op,xsrc,xdst);
		else {
			switch (xdst->mode) {
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				}
			enc_T39(op,xsrc,xdst);
			}
		}
	else if( xdst->space == LSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		switch( xdst->mode ){
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				/* fall through */
				case NOUPDATE:
				case POSTINC:
				case POSTDEC:
				case PSTINCO:
				case PSTDECO:
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						op->count = 2; /* set default word count */
						return(NO);
						}
					enc_T45(op,xsrc,xdst);
					break;
				case SABSOL:
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T44(op,xsrc,xdst);
					break;
			}
		}
	else if( xdst->space == PSPACE ){
		return (xdst_pmem(xyclass,op,xsrc,xdst));
		}
	else{
		error("Illegal X field destination specified");
		return(NO);
		}
	return(YES);
}

/**
*
* name		p_y0_n7 - process moves that start with y0 - n7
*
* synopsis	yn = p_y0_n7(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with an
*		y0,y1,a0,a1,a2,b0,b1,b2,r0-r7,n0-n7
*		source in the X source field.
*
**/
static
p_y0_n7(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( get_space(xdst,SCLS9) == NO )
		return(NO); /* failed */
	if( xdst->space == NONE ){
		if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
			return(NO); /* no valid destination */
		switch( set_badflags (xdst->reg) ){
			case SR:
			case LA:
			case LC:
			case SSH:
			case SSL:
			case SP:
				(void)chk_do (2);	/* check if too close to end of loop */
			/* fall through */
			case REGM0:
			case REGM1:
			case REGM2:
			case REGM3:
			case REGM4:
			case REGM5:
			case REGM6:
			case REGM7:
			case OMR:
				if( xyclass != MOVES ){
					error("Instruction does not allow data movement specified");
					return(NO);
					}
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T29a(op,xsrc,xdst);
				break;
			case REGA:		/* Rev. C */
			case REGB:
				if( *Ymove == EOS ){
					enc_T49(op,xsrc,xdst);
					break;
					}
				if (xsrc->reg != REGY0){
					error ("Invalid addressing mode");
					return (NO);
					}
				Optr = Ymove; /* reset Optr */
				if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
					return(NO); /* no valid destination */
				if( get_space(ydst,SCLS2) == NO )
					return(NO); /* failed */
				if( get_amode(NONE,ydst,NONE,REA1,NONE,NONE) == NO )
					return(NO); /* no valid destination */
				enc_T37d(op,ysrc,ydst);
				break;
			case REGX0:
			case REGX1:
			case REGY0:
			case REGY1:
			case REGR0:
			case REGR1:
			case REGR2:
			case REGR3:
			case REGR4:
			case REGR5:
			case REGR6:
			case REGR7:
			case REGN0:
			case REGN1:
			case REGN2:
			case REGN3:
			case REGN4:
			case REGN5:
			case REGN6:
			case REGN7:
			case REGA0:
			case REGB0:
			case REGA1:
			case REGB1:
			case REGA2:
			case REGB2:
				if( *Ymove != EOS ){
					Optr = Swap ? Xmove : Ymove;
					error("Y move field not allowed");
					return(NO);
					}
				enc_T49(op,xsrc,xdst);
				break;
			default:
				error("Illegal X field destination register specified");
				return(NO);
			}
		}
	else if( xdst->space == XSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		if( *Ymove != EOS ){
			error("Too many parallel move fields specified");
			op->count = 2; /* set default word count */
			return(NO);
			}
		if( xdst->mode == SABSOL )
			enc_T40(op,xsrc,xdst);
		else {
			switch (xdst->mode) {
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				}
			enc_T38(op,xsrc,xdst);
			}
		}
	else if( xdst->space == YSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		if( *Ymove != EOS ){
			error("Too many parallel move fields specified");
			op->count = 2; /* set default word count */
			return(NO);
			}
		Ymove = Xmove; /* move fields over */
		Xmove = Nullstr;
		if( xdst->mode == SABSOL )
			enc_T41(op,xsrc,xdst);
		else {
			switch (xdst->mode) {
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				}
			enc_T39(op,xsrc,xdst);
			}
		}
	else if( xdst->space == PSPACE )
		return (xdst_pmem(xyclass,op,xsrc,xdst));
	else{
		error("Illegal X field destination specified");
		return(NO);
		}
	return(YES);
}


/**
*
* name		p_m0_m7 - process moves that start with m0 - m7
*
* synopsis	yn = p_m0_m7(xyclass,op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with an
*		m0-m7 source in the X source field.
*
**/
static
p_m0_m7(xyclass,op,xsrc,xdst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( xyclass != MOVES ){
		error("Instruction does not allow data movement specified");
		return(NO);
		}
	if( *Ymove != EOS ){
		Optr = Swap ? Xmove : Ymove;
		error("Y move field not allowed");
		return(NO);
		}
	if( get_space(xdst,SCLS9) == NO )
		return(NO); /* failed */
	if( xdst->space == NONE ){
		if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
			return(NO); /* no valid destination */
		enc_T29(op,xsrc,xdst);
		}
	else if( xdst->space == XSPACE || xdst->space == YSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		if ( xdst->mode == SABSOL )
			enc_T30(op,xsrc,xdst);
		else {
			switch (xdst->mode) {
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				}
			enc_T32(op,xsrc,xdst);
			}
		if( xdst->space == YSPACE ){
			Ymove = Xmove; /* move fields over */
			Xmove = Nullstr;
			}
		}
	else if( xdst->space == PSPACE )
		return (xdst_pmem(xyclass,op,xsrc,xdst));
	else{
		error("Illegal X field destination specified");
		return(NO);
		}
	return(YES);
}

/**
*
* name		p_omr_sp - process moves that start with omr - sp
*
* synopsis	yn = p_omr_sp(xyclass,op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with an
*		omr_sp source in the X source field.
*
**/
static
p_omr_sp(xyclass,op,xsrc,xdst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( xyclass != MOVES ){
		error("Instruction does not allow data movement specified");
		return(NO);
		}
	if( xsrc->reg == SSH ){
		Bad_flags |= BADRT | BADDO | BADENDDO | BADINT;
		(void)chk_do (2);	/* check if too close to end loop */
		}
	if( (xsrc->reg == SSH || xsrc->reg == SSL) && (Prev_flags & BADMOVESS) )
		error ("Move from SSH or SSL cannot follow move to SP");
	if( *Ymove != EOS ){
		Optr = Swap ? Xmove : Ymove;
		error("Y move field not allowed");
		return(NO);
		}
	if( get_space(xdst,SCLS9) == NO )
		return(NO); /* failed */
	if( xdst->space == NONE ){
		if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
			return(NO); /* no valid destination */
		if(xsrc->reg == SSH && xdst->reg == SSH){
			error("SSH cannot be both source and destination register");
			return(NO);
			}
		enc_T29(op,xsrc,xdst);
		}
	else if( xdst->space == XSPACE || xdst->space == YSPACE ){
		if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
			op->count = 2; /* set default word count */
			return(NO); /* no valid destination */
			}
		if ( xdst->mode == SABSOL )
			enc_T30(op,xsrc,xdst);
		else {
			switch (xdst->mode) {
				case INDEXO:
				case PREDEC:
				case LABSOL:
					Cycles += EA_CYCLES;
				}
			enc_T27(op,xsrc,xdst);
			}
		if( xdst->space == YSPACE ){
			Ymove = Xmove; /* move fields over */
			Xmove = Nullstr;
			}
		}
	else if( xdst->space == PSPACE ){
		if (xdst_pmem(xyclass,op,xsrc,xdst) == NO)
			return (NO);
		}
	else{
		error("Illegal X field destination specified");
		return(NO);
		}
	switch (xsrc->reg) {	/* cannot read LC,SP,SSL at LA-2,LA-1 */
		case LC:
		case SP:
		case SSL:
			if (chk_do (xdst->mode == LABSOL ? 2 : 1))
				return (NO);
			break;
		default:
			break;
		}
	return(YES);
}

/**
*
* name		xdst_pmem - process X destination field moves to P memory
*
* synopsis	yn = xdst_pmem(xyclass,op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with an
*		p space source in the X destination field.
*
**/
static
xdst_pmem(xyclass,op,xsrc,xdst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( xyclass != MOVES ){
		error("Instruction does not allow data movement specified");
		return(NO);
		}
	if( get_amode(NONE,xdst,NONE,REA1,LSA,NONE) == NO ){
		op->count = 2; /* set default word count */
		return(NO); /* no valid destination */
		}
	if( *Ymove != EOS ){
		error("Too many fields specified for instruction");
		op->count = 2; /* set default word count */
		return(NO);
		}
	Cycles += MVC_CYCLES;
	if( xdst->mode == SABSOL )
		enc_T53(op,xsrc,xdst);
	else {
		switch (xdst->mode) {
			case INDEXO:
			case PREDEC:
			case LABSOL:
				Cycles += EA_CYCLES;
			}
		enc_T52(op,xsrc,xdst);
		}
	return(YES);
}

/**
*
* name		p_xs - process moves that start with x space
*
* synopsis	yn = p_xs(xyclass,op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes parallel moves that start with an
*		x space source in the X source field.
*
**/
static
p_xs(xyclass,op,xsrc,xdst,ysrc,ydst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	switch( xsrc->mode ){
		case NOUPDATE:
		case POSTINC:
		case POSTDEC:
		case PSTINCO:
			if( get_space(xdst,NONE) == NO )
				return(NO); /* failed */
			if( xdst->space == NONE ){
				if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
					return(NO);
				switch( set_badflags (xdst->reg) ){
					case REGM0:
					case REGM1:
					case REGM2:
					case REGM3:
					case REGM4:
					case REGM5:
					case REGM6:
					case REGM7:
						if( xyclass != MOVES ){
							error("Instruction does not allow data movement specified");
							return(NO);
							}
						if( *Ymove != EOS ){
							Optr = Swap ? Xmove : Ymove;
							error("Y move field not allowed");
							return(NO);
							}
						enc_T32a(op,xsrc,xdst);
						break;
					case SR:
					case LA:
					case LC:
					case SSH:
					case SSL:
					case SP:
						(void)chk_do (2);	/* check if too close to end of loop */
						/* fall through */
					case OMR:
						if( xyclass != MOVES ){
							error("Instruction does not allow data movement specified");
							return(NO);
							}
						if( *Ymove != EOS ){
							Optr = Swap ? Xmove : Ymove;
							error("Y move field not allowed");
							return(NO);
							}
						enc_T27a(op,xsrc,xdst);
						break;
					case REGA:
					case REGB:
					case REGX0:
					case REGX1:
						if (!xdst_reg (op,xsrc,xdst,ysrc,ydst))
							return (NO);
						break;
					case REGY0:
					case REGY1:
					case REGR0:
					case REGR1:
					case REGR2:
					case REGR3:
					case REGR4:
					case REGR5:
					case REGR6:
					case REGR7:
					case REGN0:
					case REGN1:
					case REGN2:
					case REGN3:
					case REGN4:
					case REGN5:
					case REGN6:
					case REGN7:
					case REGA0:
					case REGB0:
					case REGA1:
					case REGB1:
					case REGA2:
					case REGB2:
						if( *Ymove != EOS ){
							Optr = Swap ? Xmove : Ymove;
							error("Y move field not allowed");
							return(NO);
							}
						enc_T38a(op,xsrc,xdst);
						break;
					default:
						error("Illegal X field destination register specified");
						return(NO);
					}
				}
			break;
		case INDEXO:
		case PREDEC:
		case LABSOL:
			Cycles += EA_CYCLES;
		/* fall through */
		case PSTDECO:
			if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
				return(NO);
			switch( set_badflags (xdst->reg) ){
				case REGM0:
				case REGM1:
				case REGM2:
				case REGM3:
				case REGM4:
				case REGM5:
				case REGM6:
				case REGM7:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T32a(op,xsrc,xdst);
					break;
				case SR:
				case LA:
				case LC:
				case SSH:
				case SSL:
				case SP:
					(void)chk_do (2);	/* check if too close to end of loop */
					/* fall through */
				case OMR:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T27a(op,xsrc,xdst);
					break;
				case REGA:
				case REGB:
				case REGX0:
				case REGX1:
					if( *Ymove == EOS ){
						enc_T38a(op,xsrc,xdst);
						break;
						}
					Optr = Ymove; /* reset Optr */
					if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
						return(NO); /* failed */
					if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
						return(NO);
					enc_T43a(op,xsrc,xdst,ysrc,ydst);
					break;
				case REGY0:
				case REGY1:
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
				case REGN0:
				case REGN1:
				case REGN2:
				case REGN3:
				case REGN4:
				case REGN5:
				case REGN6:
				case REGN7:
				case REGA0:
				case REGB0:
				case REGA1:
				case REGB1:
				case REGA2:
				case REGB2:
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T38a(op,xsrc,xdst);
					break;
				default:
					error("Illegal X field destination register specified");
					return(NO);
				}
			break;
		case SABSOL:
			if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
				return(NO);
			switch( set_badflags (xdst->reg) ){
				case SR:
				case LA:
				case LC:
				case SSH:
				case SSL:
				case SP:
					(void)chk_do (2);	/* check if too close to end of loop */
				/* fall through */
				case REGM0:
				case REGM1:
				case REGM2:
				case REGM3:
				case REGM4:
				case REGM5:
				case REGM6:
				case REGM7:
				case OMR:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T30a(op,xsrc,xdst);
					break;
				case REGA:
				case REGB:
				case REGX0:
				case REGX1:
					if( *Ymove == EOS ){
						enc_T40a(op,xsrc,xdst);
						break;
						}
					Optr = Ymove; /* reset Optr */
					if( get_amode(COMMA,ysrc,RSET1,NONE,NONE,NONE) == NO )
						return(NO); /* failed */
					if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
						return(NO);
					if( xsrc->force )
						warn("Short absolute address cannot be forced - long substituted");
					xsrc->mode = LABSOL;
					Cycles += EA_CYCLES;
					enc_T43a(op,xsrc,xdst,ysrc,ydst);
					break;
				case REGY0:
				case REGY1:
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
				case REGN0:
				case REGN1:
				case REGN2:
				case REGN3:
				case REGN4:
				case REGN5:
				case REGN6:
				case REGN7:
				case REGA0:
				case REGB0:
				case REGA1:
				case REGB1:
				case REGA2:
				case REGB2:
					if( *Ymove != EOS ){
						Optr = Swap ? Xmove : Ymove;
						error("Y move field not allowed");
						return(NO);
						}
					enc_T40a(op,xsrc,xdst);
					break;
				default:
					error("Illegal X field destination register specified");
					return(NO);
				}
			break;
		}
	return(YES);
}

/**
*
* name		xdst_reg - process X memory moves to A, B, X0, or X1 registers
*
* synopsis	yn = xdst_reg(op,xsrc,xdst,ysrc,ydst)
*		int yn;		NO if error
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*		struct amode *ysrc;	y field source mode
*		struct amode *ydst;	y field dest mode
*
* description	Processes moves from X memory to A, B, X0, or X1 registers.
*
**/
static
xdst_reg(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	if( *Ymove == EOS ){
		enc_T38a(op,xsrc,xdst);
		return(YES);
		}
	Optr = Ymove; /* reset Optr */
	if( get_space(ysrc,SCLS8) == NO )
		return(NO); /* failed */
	if( ysrc->space == NONE ){
		if( get_amode(COMMA,ysrc,RSET4,NONE,NONE,NONE) == NO )
			return(NO); /* failed */
		switch( ysrc->reg ){
			case REGA:
			case REGB:
				if( get_space(ydst,SCLS8) == NO )
					return(NO);
				if( ydst->space == NONE ){
					if( get_amode(NONE,ydst,RSET2,NONE,NONE,NONE) == NO )
						return(NO);
					enc_T43a(op,xsrc,xdst,ysrc,ydst);
					}
				else{
					if( get_amode(NONE,ydst,NONE,REA2,NONE,NONE) == NO )
						return(NO);
					if (r_intersect (xsrc->reg, ydst->reg))
						return (NO);
					enc_T37a(op,xsrc,xdst,ysrc,ydst);
					}
				break;
			case REGY0:
			case REGY1:
				if( get_space(ydst,SCLS2) == NO )
					return(NO);
				if( get_amode(NONE,ydst,NONE,REA2,NONE,NONE) == NO )
					return(NO);
				if (r_intersect (xsrc->reg, ydst->reg))
					return (NO);
				enc_T37a(op,xsrc,xdst,ysrc,ydst);
				break;
			}
		}
	else{
		if( get_amode(COMMA,ysrc,NONE,REA2,NONE,NONE) == NO )
			return(NO);
		if (r_intersect (xsrc->reg, ysrc->reg))
			return (NO);
		if( get_amode(DEST,ydst,RSET4,NONE,NONE,NONE) == NO )
			return(NO);
		enc_T37c(op,xsrc,xdst,ysrc,ydst);
		}
	return (YES);
}

/**
*
* name		p_ys - process moves that start with y space
*
* synopsis	yn = p_ys(xyclass,op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with a
*		y space source in the X source field.
*
**/
static
p_ys(xyclass,op,xsrc,xdst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( *Ymove != EOS ){
		error("Too many fields specified for instruction");
		return(NO);
		}
	Ymove = Xmove; /* move fields over */
	Xmove = Nullstr;

	switch( xsrc->mode ){
		case INDEXO:
		case PREDEC:
		case LABSOL:
			Cycles += EA_CYCLES;
		/* fall through */
		case NOUPDATE:
		case POSTINC:
		case POSTDEC:
		case PSTINCO:
		case PSTDECO:
			if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
				return(NO);
			switch( set_badflags (xdst->reg) ){
				case REGM0:
				case REGM1:
				case REGM2:
				case REGM3:
				case REGM4:
				case REGM5:
				case REGM6:
				case REGM7:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					enc_T32a(op,xsrc,xdst);
					break;
				case SR:
				case LA:
				case LC:
				case SSH:
				case SSL:
				case SP:
					(void)chk_do (2);	/* check if too close to end of loop */
				/* fall through */
				case OMR:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					enc_T27a(op,xsrc,xdst);
					break;
				case REGA:
				case REGB:
				case REGX0:
				case REGX1:
				case REGY0:
				case REGY1:
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
				case REGN0:
				case REGN1:
				case REGN2:
				case REGN3:
				case REGN4:
				case REGN5:
				case REGN6:
				case REGN7:
				case REGA0:
				case REGB0:
				case REGA1:
				case REGB1:
				case REGA2:
				case REGB2:
					enc_T39a(op,xsrc,xdst);
					break;
				default:
					error("Illegal X field destination register specified");
					return(NO);
				}
			break;
		case SABSOL:
			if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
				return(NO);
			switch( set_badflags (xdst->reg) ){
				case SR:
				case LA:
				case LC:
				case SSH:
				case SSL:
				case SP:
					(void)chk_do (2);	/* check if too close to end of loop */
				/* fall through */
				case REGM0:
				case REGM1:
				case REGM2:
				case REGM3:
				case REGM4:
				case REGM5:
				case REGM6:
				case REGM7:
				case OMR:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					enc_T30a(op,xsrc,xdst);
					break;
				case REGA:
				case REGB:
				case REGX0:
				case REGX1:
				case REGY0:
				case REGY1:
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
				case REGN0:
				case REGN1:
				case REGN2:
				case REGN3:
				case REGN4:
				case REGN5:
				case REGN6:
				case REGN7:
				case REGA0:
				case REGB0:
				case REGA1:
				case REGB1:
				case REGA2:
				case REGB2:
					enc_T41a(op,xsrc,xdst);
					break;
				default:
					error("Illegal X field destination register specified");
					return(NO);
				}
			break;
		}
	return(YES);
}

/**
*
* name		p_ls - process moves that start with l space
*
* synopsis	yn = p_ls(op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with an
*		l space source in the X source field.
*
**/
static
p_ls(op,xsrc,xdst)
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( *Ymove != EOS ){
		error("Too many fields specified for instruction");
		return(NO);
		}
	if( xsrc->mode == SABSOL ){
		if( get_amode(DEST,xdst,RSET5,NONE,NONE,NONE) == NO )
			return(NO);
		enc_T44a(op,xsrc,xdst);
		}
	else{
		switch (xsrc->mode) {
			case INDEXO:
			case PREDEC:
			case LABSOL:
				Cycles += EA_CYCLES;
			}
		if( get_amode(DEST,xdst,RSET5,NONE,NONE,NONE) == NO )
			return(NO);
		enc_T45a(op,xsrc,xdst);
		}
	return(YES);
}

/**
*
* name		p_ps - process moves that start with p space
*
* synopsis	yn = p_ps(xyclass,op,xsrc,xdst)
*		int yn;		NO if error
*		int xyclass;	MOVES if special moves are allowed
*		struct obj *op; object code structure
*		struct amode *xsrc;	x field source mode
*		struct amode *xdst;	x field dest mode
*
* description	Processes parallel moves that start with an
*		p space source in the X source field.
*
**/
static
p_ps(xyclass,op,xsrc,xdst)
int xyclass;
struct obj *op;
struct amode *xsrc,*xdst;
{
	if( *Ymove != EOS ){
		error("Too many fields specified for instruction");
		return(NO);
		}
	Cycles += MVC_CYCLES;
	switch( xsrc->mode ){
		case INDEXO:
		case PREDEC:
		case LABSOL:
			Cycles += EA_CYCLES;
		/* fall through */
		case NOUPDATE:
		case POSTINC:
		case POSTDEC:
		case PSTINCO:
		case PSTDECO:
		case SABSOL:
			if( get_amode(DEST,xdst,RSET18,NONE,NONE,NONE) == NO )
				return(NO);
			switch( set_badflags (xdst->reg) ){
				case SR:
				case LA:
				case LC:
				case SSH:
				case SSL:
				case SP:
					(void)chk_do (2);	/* check if too close to end of loop */
				/* fall through */
				case REGM0:
				case REGM1:
				case REGM2:
				case REGM3:
				case REGM4:
				case REGM5:
				case REGM6:
				case REGM7:
				case OMR:
				case REGA:
				case REGB:
				case REGX0:
				case REGX1:
				case REGY0:
				case REGY1:
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
				case REGN0:
				case REGN1:
				case REGN2:
				case REGN3:
				case REGN4:
				case REGN5:
				case REGN6:
				case REGN7:
				case REGA0:
				case REGB0:
				case REGA1:
				case REGB1:
				case REGA2:
				case REGB2:
					if( xyclass != MOVES ){
						error("Instruction does not allow data movement specified");
						return(NO);
						}
					if( xsrc->mode == SABSOL)
						enc_T53a(op,xsrc,xdst);
					else
						enc_T52a(op,xsrc,xdst);
					break;
				default:
					error("Illegal X field destination register specified");
					return(NO);
				}
			break;
		}
	return(YES);
}

/**
*
* name		r_intersect - check for register set intersection
*
* synopsis	yn = r_intersect (reg1, reg2)
*		int yn;		YES if duplicate register, NO otherwise
*		int reg1, reg2; registers to be compared
*
* description	Checks whether the address registers in an XY data move
*		belong to the same set.	 If reg1 and reg2 are both in
*		R0-R3, YES is returned; otherwise NO.  If reg1 and reg2
*		are both in R4-R7, YES is returned; otherwise NO.  An
*		error is issued if both registers belong to the same set.
*
**/
static
r_intersect (reg1, reg2)
int reg1, reg2;
{
	switch (reg1) {
		case REGR0:
		case REGR1:
		case REGR2:
		case REGR3:
			switch (reg2) {
				case REGR0:
				case REGR1:
				case REGR2:
				case REGR3:
					error ("Invalid XY address register specification");
					return (YES);
				default:
					return (NO);
			}
		case REGR4:
		case REGR5:
		case REGR6:
		case REGR7:
			switch (reg2) {
				case REGR4:
				case REGR5:
				case REGR6:
				case REGR7:
					error ("Invalid XY address register specification");
					return (YES);
				default:
					return (NO);
			}
		default:
			return (NO);
	}
}

/**
*
* name		set_badflags - set restricted instruction/register flags
*
* synopsis	set_badflags (reg)
*		int reg;	restricted register to check
*
* description	Sets global Bad_flags to allow validation of register usage
*		in subsequent instruction.  Returns reg.
*
**/
set_badflags (reg)
int reg;
{
	switch (reg) {
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
		default:
			break;
	}
	return (reg);
}

/**
*
* name		swap_fields - swap X and Y move fields if necessary
*
* synopsis	yn = swap_fields ()
*		int yn;			NO if no swap
*
* description	Checks the X and Y data move fields to insure they are
*		in the proper order for the assembler to analyze them
*		(X followed by Y).  If a reference to Y memory is found
*		in the X move field, or a reference to X memory is found
*		in the Y move field, the field pointers are swapped and
*		YES is returned.  Otherwise, NO is returned.
*
**/
swap_fields ()
{
	register char *p;

	for (p = Xmove; *p; p++)
		if ((*p == 'Y' || *p == 'y') && *++p == ':') {
			do_swap ();
			return (YES);
		}
	for (p = Ymove; *p; p++)
		if ((*p == 'X' || *p == 'x') && *(p + 1) == ':') {
			do_swap ();
			return (YES);
		}
	return (NO);
}

/**
*	do_swap - swap X and Y data move field pointers
**/
do_swap ()
{
	char *p;

	p = Xmove;
	Xmove = Ymove;
	Ymove = p;
}
