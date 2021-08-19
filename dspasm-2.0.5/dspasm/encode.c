#include "dspdef.h"
#include "dspdcl.h"

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
* name		f_insert - field insert
*
* synopsis	r = f_insert(base,value,start,width)
*		long r;		result
*		long base;	base value
*		long value;	value to be inserted
*		int start;	start bit # of field
*		int width;	field width
*
* description	Inserts the value into the base starting at
*		bit # start, using a field width of width.
*
**/
static long
f_insert(base,value,start,width)
long base,value;
int start,width;
{
	if (!(width >= 1 && width <= 12))
		fatal ("Field insert failure");

	return ( (base	& ~(~(~((long)0) << width) << start)) |
		 ((value & ~(~((long)0) << width)) << start) );
}

/**
* name		s_insert - string insert
*
* synopsis	s = s_insert(base,value,start,width)
*		char *s;	string result
*		char *base;	base value string
*		char *value;	value string to be inserted
*		int start;	start bit # of field
*		int width;	field width
*
* description	Creates an expression string to insert the value
*		into the base starting at bit # start, using a
*		field width of width.
*
**/
static char *
s_insert(base,value,start,width)
char *base,*value;
int start,width;
{
	char buf[MAXEXP], *s;
	int len;

	if (!(width >= 1 && width <= 12))
		fatal ("String insert failure");

	(void)sprintf (buf, "((%s&~(~(~0<<%d)<<%d))|((%s&~(~0<<%d))<<%d))",
		 base, width, start, value, width, start);
	len = strlen (buf);
	if ((s = (char *)alloc (len + 1)) == NULL)
		fatal ("Out of memory - cannot save expression string");
	return (strcpy (s, buf));
}

/**
*
* name		insert - insert in opcode field or op expression string
*
* synopsis	insert(op,src,start,width)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		int start;		start bit # of field
*		int width;		field width
*
* description	Determines whether to insert source value into opcode
*		field for absolute values or expression string for
*		relocatable values.
*
**/
static
insert(op,src,start,width)
struct obj *op;
struct amode *src;
int start,width;
{
	char *strup ();

	if (!src->expstr)
		op->opcode = f_insert(op->opcode,src->val,start,width);
	else {
		(void)sprintf(String,"$%06lx",op->opcode);
		op->opexp = s_insert(strup(String),src->expstr,start,width);
		free(src->expstr);	/* free old expression string */
	}
}

/**
*
* name		e_CCC - encode control register field
*
* synopsis	ev = e_CCC(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a CCC type field
*
**/
static long
e_CCC(rn)
int rn;
{
	switch(rn){
		case SR:   return(1L);
		case OMR:  return(2L);
		case SP:   return(3L);
		case SSH:  return(4L);
		case SSL:  return(5L);
		case LA:   return(6L);
		case LC:   return(7L);
		default:
			fatal ("CCC encoding failure");
			return ((long)0);
		}

}

/**
*
* name		e_D - encode register field
*
* synopsis	ev = e_D(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a D type field
*
**/
static long
e_D(rn)
int rn;
{
	switch(rn){
		case REGA:  return((long)0);
		case REGB:  return((long)1);
		default:
			fatal("D encoding failure");
			return ((long)0);
		}
}

/**
*
* name		e_DD - encode register field
*
* synopsis	ev = e_DD(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a DD type field
*
**/
static long
e_DD(rn)
int rn;
{
	switch(rn){
		case REGX0:  return(0L);
		case REGX1:  return(1L);
		case REGY0:  return(2L);
		case REGY1:  return(3L);
		default:
			fatal("DD encoding failure");
			return (0L);
		}
}

/**
*
* name		e_DDD - encode register field
*
* synopsis	ev = e_DDD(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a DDD type field
*
**/
static long
e_DDD(rn)
int rn;
{
	switch(rn){
		case REGA0: return(0x00L);
		case REGB0: return(0x01L);
		case REGA2: return(0x02L);
		case REGB2: return(0x03L);
		case REGA1: return(0x04L);
		case REGB1: return(0x05L);
		case REGA:  return(0x06L);
		case REGB:  return(0x07L);
		default:
			fatal("DDD encoding failure");
			return (0L);
		}
}

/**
*
* name		e_DDDDD - encode register field
*
* synopsis	ev = e_DDDDD(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a DDDDD type field
*
**/
static long
e_DDDDD(rn)
int rn;
{
	long e_DD(),e_DDD(),e_NNN(),e_RRR();

	switch(rn){
		case REGX0:
		case REGX1:
		case REGY0:
		case REGY1: return(((long)4)+e_DD(rn));
		case REGA0:
		case REGB0:
		case REGA2:
		case REGB2:
		case REGA1:
		case REGB1:
		case REGA:
		case REGB:  return(((long)8)+e_DDD(rn));
		case REGR0:
		case REGR1:
		case REGR2:
		case REGR3:
		case REGR4:
		case REGR5:
		case REGR6:
		case REGR7: return(((long)0x10)+e_RRR(NONE,rn));
		case REGN0:
		case REGN1:
		case REGN2:
		case REGN3:
		case REGN4:
		case REGN5:
		case REGN6:
		case REGN7: return(((long)0x18)+e_NNN(rn));
		default:
			fatal("DDDDD encoding failure");
			return (0L);
		}
}

/**
*
* name		e_D6 - encode register field
*
* synopsis	ev = e_D6(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a D6 type field
*
**/
static long
e_D6(rn)
int rn;
{
	long e_FFF();

	switch(rn){
		case REGX0:
		case REGX1:
		case REGY0:
		case REGY1:
		case REGA0:
		case REGB0:
		case REGA2:
		case REGB2:
		case REGA1:
		case REGB1:
		case REGA:
		case REGB:
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
		case REGN7: return(e_DDDDD(rn));
		case REGM0:
		case REGM1:
		case REGM2:
		case REGM3:
		case REGM4:
		case REGM5:
		case REGM6:
		case REGM7: return(((long)0x20)+e_FFF(rn));
		case SR:
		case OMR:
		case SP:
		case SSH:
		case SSL:
		case LA:
		case LC:    return(((long)0x38)+e_CCC(rn));
		default:
			fatal("D6 encoding failure");
			return (0L);
		}
}

/**
*
* name		e_DXY - encode register field
*
* synopsis	ev = e_DXY(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a DXY type field
*
**/
static long
e_DXY(rn)
int rn;
{
	switch(rn){
		case REGA:
		case REGB:  return(0L);
		case REGX0: return(4L);
		case REGY0: return(5L);
		case REGX1: return(6L);
		case REGY1: return(7L);
		default:
			fatal("DXY encoding failure");
			return (0L);
		}
}

/**
*
* name		e_EE - encode control register field
*
* synopsis	ev = e_EE(rs,rd)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an EE type field
*
**/
static long
e_EE(rn)
int rn;
{
	switch (rn) {
		case MR:  return (0L);
		case CCR: return (1L);
		case OMR: return (2L);
		default:
			fatal ("EE encoding failure");
			return (0L);
	}
}

/**
*
* name		e_FFF - encode register field
*
* synopsis	ev = e_FFF(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an FFF type field
*
**/
static long
e_FFF(rn)
int rn;
{
	switch(rn){
		case REGM0: return(0L);
		case REGM1: return(1L);
		case REGM2: return(2L);
		case REGM3: return(3L);
		case REGM4: return(4L);
		case REGM5: return(5L);
		case REGM6: return(6L);
		case REGM7: return(7L);
		default:
			fatal("FFF encoding failure");
			return (0L);
		}
}

/**
*
* name		e_LLL - encode register field
*
* synopsis	ev = e_LLL(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an LLL type field
*
**/
static long
e_LLL(rn)
int rn;
{
	switch(rn){
		case REGA10: return (0L);
		case REGB10: return (1L);
		case REGX:   return (2L);
		case REGY:   return (3L);
		case REGA:   return (8L);
		case REGB:   return (9L);
		case REGAB:  return (10L);
		case REGBA:  return (11L);
		default:
			fatal("LLL encoding failure");
			return (0L);
		}
}

/**
*
* name		e_MMM - encode address mode field
*
* synopsis	ev = e_MMM(mode)
*		long ev;	encoded value
*		int mode;	address mode
*
* description	Encodes an MMM type field
*
**/
static long
e_MMM(mode)
int mode;
{
	switch(mode){
		case PSTDECO:  return(0L);
		case PSTINCO:  return(1L);
		case POSTDEC:  return(2L);
		case POSTINC:  return(3L);
		case NOUPDATE: return(4L);
		case INDEXO:   return(5L);
		case LABSOL:
		case LIMMED:   return(6L);
		case PREDEC:   return(7L);
		default:
			fatal("MMM encoding failure");
			return (0L);
		}
}

/**
*
* name		e_NNN - encode register field
*
* synopsis	ev = e_NNN(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an NNN type field
*
**/
static long
e_NNN(rn)
int rn;
{
	switch(rn){
		case REGN0: return(0L);
		case REGN1: return(1L);
		case REGN2: return(2L);
		case REGN3: return(3L);
		case REGN4: return(4L);
		case REGN5: return(5L);
		case REGN6: return(6L);
		case REGN7: return(7L);
		default:
			fatal("NNN encoding failure");
			return (0L);
		}
}

/**
*
* name		e_RRR - encode register field
*
* synopsis	ev = e_RRR(mode,rn)
*		long ev;	encoded value
*		int mode;	addressing mode
*		int rn;		register number
*
* description	Encodes an RRR type field
*
**/
static long
e_RRR(mode,rn)
int mode,rn;
{
	/* handle 2 special cases */
	if( mode == LABSOL )
		return(0L);
	if( mode == LIMMED )
		return(4L);
	/* otherwise default to register number */
	switch(rn){
		case REGR0: return(0L);
		case REGR1: return(1L);
		case REGR2: return(2L);
		case REGR3: return(3L);
		case REGR4: return(4L);
		case REGR5: return(5L);
		case REGR6: return(6L);
		case REGR7: return(7L);
		default:
			fatal("RRR encoding failure");
			return (0L);
		}
}

/**
*
* name		e_S - encode memory addressing
*
* synopsis	ev = e_S(space)
*		long ev;	encoded value
*		int space;	address space
*
* description	Encodes memory space
*
**/
static long
e_S(space)
int space;
{
	if( space == YSPACE )
		return(1L);
	return(0L);
}

/**
*
* name		e_X - encode X type field
*
* synopsis	ev = e_X(space)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an X type field
*
**/
static long
e_X(rn)
int rn;
{
	switch(rn){
		case REGX0:	return(0L);
		case REGX1:	return(1L);
		default:
			fatal("X encoding failure");
			return (0L);
		}
}

/**
*
* name		e_XX - encode register field
*
* synopsis	ev = e_XX(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an XX type field
*
**/
static long
e_XX(rn)
int rn;
{
	switch(rn){
		case REGX0: return(0L);
		case REGX1: return(1L);
		case REGA:  return(2L);
		case REGB:  return(3L);
		default:
			fatal("XX encoding failure");
			return (0L);
		}
}

/**
*
* name		e_Y - encode Y type field
*
* synopsis	ev = e_Y(space)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes a Y type field
*
**/
static long
e_Y(rn)
int rn;
{
	switch(rn){
		case REGY0:	return(0L);
		case REGY1:	return(1L);
		default:
			fatal("Y encoding failure");
			return (0L);
		}
}

/**
*
* name		e_YY - encode register field
*
* synopsis	ev = e_YY(rn)
*		long ev;	encoded value
*		int rn;		register number
*
* description	Encodes an YY type field
*
**/
static long
e_YY(rn)
int rn;
{
	switch(rn){
		case REGY0: return(0L);
		case REGY1: return(1L);
		case REGA:  return(2L);
		case REGB:  return(3L);
		default:
			fatal("YY encoding failure");
			return (0L);
		}
}

/**
*
* name		enc_long - encode instruction extension
*
* synopsis	enc_long(op,am)
*		struct obj *op;		object code structure
*		struct amode *am;	address mode
*		int adj;		DO loop address adjust flag
*
* description	Encodes address extension for instructions with
*		long immediate or long absolute operands, or if
*		operand contains a forward reference.  Adjusts
*		loop address if adj flag set.
*
**/
static
enc_long(op,am,adj)
struct obj *op;
struct amode *am;
int adj;
{
	char *p, *q;

	if (am->mode != LABSOL && am->mode != LIMMED)
		return;
	op->postword = am->val;
	if (am->expstr)
		op->postexp = am->expstr;
	op->count = 2;

	if (!adj)		/* no adjustment necessary */
		return;
	if (op->postword > 0)	/* adjust value */
		--op->postword;
	if ((p = op->postexp) != NULL) { /* reallocate expression string */
		if ((q = (char *)alloc (strlen (p) + 5)) == NULL)
			fatal ("Out of memory - cannot save encoding expression");
		(void)sprintf (q, "%s-1", p);	/* copy expression */
		free (p);			/* free old expression */
		op->postexp = q;		/* save new expression */
	}
}

/**
*
* name		enc_T1 - encode type 1 instruction
*
* synopsis	enc_T1(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T1 instructions.
*
**/
enc_T1(op,src,dst)
struct obj *op;
struct amode *src, *dst;
{
	op->opcode = f_insert(op->opcode|SIMMBO,e_EE(dst->reg),0,2); /* reg */
	insert(op,src,8,8);	/* insert immediate value */
}

/**
*
* name		enc_T2 - encode type 2 instruction
*
* synopsis	enc_T2(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode dst;	dest address mode
*
* description	Encodes T2 instructions.
*
**/
enc_T2(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D(dst->reg),3,1);
	op->opcode = f_insert(opc,e_DXY(src->reg),4,2);
}

/**
*
* name		enc_T4 - encode type 4 instruction
*
* synopsis	enc_T4(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T4 instructions.
*
**/
enc_T4(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D(dst->reg),3,1);
	op->opcode = f_insert(opc,e_RRR(src->mode,src->reg),8,3);
}

/**
*
* name		enc_T5 - encode type 5 instruction
*
* synopsis	enc_T5(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T5 instructions.
*
**/
enc_T5(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_S(src->space),6,1); /* set space bit */
	opc = f_insert(opc,e_RRR(src->mode,src->reg),8,3); /* insert reg */
	opc = f_insert(opc,e_MMM(src->mode),11,3); /* insert mode */
	op->opcode = f_insert(opc,1L,14,1); /* set bits */
}

/**
*
* name		enc_T5a - encode type 5a instruction
*
* synopsis	enc_T5a(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T5a instructions.
*
**/
enc_T5a(op,src)
struct obj *op;
struct amode *src;
{
	op->opcode = f_insert(op->opcode,e_S(src->space),6,1); /* set bit */
	insert(op,src,8,6);	/* insert address */
}

/**
*
* name		enc_T5b - encode type 5b instruction
*
* synopsis	enc_T5b(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T5b instructions.
*
**/
enc_T5b(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D6(src->reg),8,6); /* insert reg code */
	op->opcode = f_insert(opc,3L,14,2);
}

/**
*
* name		enc_T5c - encode type 5c instruction
*
* synopsis	enc_T5c(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T5c instructions.
*
**/
enc_T5c(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */
	char *s, buf[MAXEXP];
	char *strup ();

	opc = f_insert(op->opcode,1L,7,1); /* set misc bits */
	if (!src->expstr) {
		opc = f_insert(opc,(long)(src->val >> 8),0,4); /* high imm */
		op->opcode = f_insert(opc,src->val,8,8); /* low immediate */
	} else {
		(void)sprintf(String,"$%06lx",opc); /* convert to string */
		(void)strup(String);
		(void)sprintf(buf,"%s>>8",src->expstr);
		s = s_insert(String,buf,0,4);
		op->opexp = s_insert(s,src->expstr,8,8);
		free(s);
		free(src->expstr);	/* free old expression string */
	}
}

/**
*
* name		enc_T5d - encode type 5d instruction
*
* synopsis	enc_T5c(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	LA address mode
*		struct amode *dst;	LC address mode
*
* description	Encodes T5d instructions.
*
**/
enc_T5d(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	enc_T5(op,src); /* do first part of encoding */
	enc_long(op,dst,YES); /* do second part of encoding */
}

/**
*
* name		enc_T5e - encode type 5e instruction
*
* synopsis	enc_T5e(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	LA address mode
*		struct amode *dst;	LC address mode
*
* description	Encodes T5e instructions.
*
**/
enc_T5e(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	enc_T5a(op,src); /* do first part of encoding */
	enc_long(op,dst,YES); /* do second part of encoding */
}

/**
*
* name		enc_T5f - encode type 5f instruction
*
* synopsis	enc_T5f(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	LA address mode
*		struct amode *dst;	LC address mode
*
* description	Encodes T5f instructions.
*
**/
enc_T5f(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	enc_T5b(op,src); /* do first part of encoding */
	enc_long(op,dst,YES); /* do second part of encoding */
}

/**
*
* name		enc_T5g - encode type 5g instruction
*
* synopsis	enc_T5g(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	LA address mode
*		struct amode *dst;	LC address mode
*
* description	Encodes T5g instructions.
*
**/
enc_T5g(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	enc_T5c(op,src); /* do first part of encoding */
	enc_long(op,dst,YES); /* do second part of encoding */
}

/**
*
* name		enc_T8 - encode type 8 instruction
*
* synopsis	enc_T8(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T8 instructions.
*
**/
enc_T8(op,src)
struct obj *op;
struct amode *src;
{
	op->opcode = f_insert(op->opcode,1L,18,1); /* set bits */
	insert(op,src,0,12);	/* insert address */
}

/**
*
* name		enc_T9 - encode type 9 instruction
*
* synopsis	enc_T9(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T9 instructions.
*
**/
enc_T9(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,1L,7,1); /* set bit */
	opc = f_insert(opc,e_RRR(src->mode,src->reg),8,3); /* insert reg */
	opc = f_insert(opc,e_MMM(src->mode),11,3); /* insert mode encoding */
	opc = f_insert(opc,3L,14,2); /* set bits */
	op->opcode = f_insert(opc,1L,17,1);
	enc_long(op,src,NO); /* handle longs */
}

/**
*
* name		enc_T11 - encode type 11 instruction
*
* synopsis	enc_T11(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T11 instructions.
*
**/
enc_T11(op,src)
struct obj *op;
struct amode *src;
{
	op->opcode = f_insert(op->opcode,1L,18,1); /* set bits */
	insert(op,src,0,12);	/* insert address */
}

/**
*
* name		enc_T12 - encode type 12 instruction
*
* synopsis	enc_T12(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T12 instructions.
*
**/
enc_T12(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */

	opc = (op->opcode >> 12) & ((long)0xf);/* shift condition bits down */
	opc = f_insert((long)(op->opcode|opc),5L,5,3); /* set bits */
	opc = f_insert(opc,e_RRR(src->mode,src->reg),8,3); /* insert register */
	opc = f_insert(opc,e_MMM(src->mode),11,3); /* insert mode encoding */
	op->opcode = f_insert(opc,3L,14,2); /* set bits */
	enc_long(op,src,NO); /* handle longs */
}

/**
*
* name		enc_T14 - encode type 14 instruction
*
* synopsis	enc_T14(op,src,dst,xsrc,xdst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*		struct amode *xsrc;	R source
*		struct amode *xdst;	R dst
*
* description	Encodes T14 instructions.
*
**/
enc_T14(op,src,dst,xsrc,xdst)
struct obj *op;
struct amode *src,*dst,*xsrc,*xdst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(xdst->mode,xdst->reg),0,3); /* insert R dest */
	opc = f_insert(opc,e_D(dst->reg),3,1); /* insert dest */
	opc = f_insert(opc,e_DXY(src->reg),4,3); /* insert source */
	opc = f_insert(opc,e_RRR(xsrc->mode,xsrc->reg),8,3); /* insert R src encoding */
	op->opcode = f_insert(opc,1L,16,1); /* set bits */
}

/**
*
* name		enc_T15 - encode type 15 instruction
*
* synopsis	enc_T15(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T15 instructions.
*
**/
enc_T15(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D(dst->reg),3,1); /* insert dest */
	op->opcode = f_insert(opc,e_DXY(src->reg),4,3); /* insert source */
}

/**
*
* name		enc_T16 - encode type 16 instruction
*
* synopsis	enc_T16(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*
* description	Encodes T16 instructions.
*
**/
enc_T16(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_S(dst->space),6,1); /* insert space */
	opc = f_insert(opc,e_RRR(dst->mode,dst->reg),8,3); /* insert reg */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	op->opcode = f_insert(opc,1L,14,1); /* set bit */
	insert(op,src,0,5);	/* insert bit number */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T16a - encode type 16a instruction	(Rev. C)
*
* synopsis	enc_T16a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*
* description	Encodes T16a instructions.
*
**/
enc_T16a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,0x301L,6,10); /* set bits */
	op->opcode = f_insert(opc,e_D6(dst->reg),8,6); /* insert reg */
	insert(op,src,0,5);	/* insert bit number */
}

/**
*
* name		enc_T17 - encode type 17 instruction
*
* synopsis	enc_T17(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*
* description	Encodes T17 instructions.
*
**/
enc_T17(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */
	char *s;
	char *strup ();

	opc = f_insert(op->opcode,e_S(dst->space),6,1); /* insert space */
	if (!src->expstr)
		opc = f_insert(opc,src->val,0,5); /* insert bit number */
	if (!dst->expstr)
		opc = f_insert(opc,dst->val,8,6); /* insert address */
	op->opcode = opc;		/* save opcode */
	(void)sprintf(String,"$%06lx",opc);	/* convert to string */
	(void)strup(String);
	if (src->expstr && !dst->expstr) {
		op->opexp = s_insert(String,src->expstr,0,5);
		free(src->expstr);	/* free old expression string */
	} else if (dst->expstr && !src->expstr) {
		op->opexp = s_insert(String,dst->expstr,8,6);
		free(dst->expstr);	/* free old expression string */
	} else if (src->expstr && dst->expstr) {
		s = s_insert(String,src->expstr,0,5);
		op->opexp = s_insert(s,dst->expstr,8,6);
		free(src->expstr);	/* free old expression strings */
		free(dst->expstr);
		free (s);
	}
}

/**
*
* name		enc_T18 - encode type 18 instruction
*
* synopsis	enc_T18(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*
* description	Encodes T18 instructions.
*
**/
enc_T18(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set bit */
	enc_T17(op,src,dst);	/* use T17 encoding */
}

/**
*
* name		enc_T19 - encode type 19 instruction
*
* synopsis	enc_T19(op,src,dst,jdst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*		struct amode *jdst;	jump destination address
*
* description	Encodes T19 instructions.
*
**/
enc_T19(op,src,dst,jdst)
struct obj *op;
struct amode *src,*dst,*jdst;
{
	enc_T16(op,src,dst); /* encode same as T16 */
	enc_long(op,jdst,NO); /* save postword */
}

/**
*
* name		enc_T19a - encode type 19a instruction	(Rev. C)
*
* synopsis	enc_T19a(op,src,dst,jdst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*		struct amode *jdst;	jump destination address
*
* description	Encodes T19a instructions.
*
**/
enc_T19a(op,src,dst,jdst)
struct obj *op;
struct amode *src,*dst,*jdst;
{
	enc_T16a(op,src,dst); /* encode same as T16a */
	op->opcode &= ~0x40L; /* clear extraneous bit */
	enc_long(op,jdst,NO); /* save postword */
}

/**
*
* name		enc_T20 - encode type 20 instruction
*
* synopsis	enc_T20(op,src,dst,jdst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*		struct amode *jdst;	jump destination address
*
* description	Encodes T20 instructions.
*
**/
enc_T20(op,src,dst,jdst)
struct obj *op;
struct amode *src,*dst,*jdst;
{
	enc_T17(op,src,dst); /* encode same as T17 */
	enc_long(op,jdst,NO); /* save postword */
}

/**
*
* name		enc_T21 - encode type 21 instruction
*
* synopsis	enc_T21(op,src,dst,jdst)
*		struct obj *op;		object code structure
*		struct amode *src;	bit number
*		struct amode *dst;	operand address mode
*		struct amode *jdst;	jump destination address
*
* description	Encodes T21 instructions.
*
**/
enc_T21(op,src,dst,jdst)
struct obj *op;
struct amode *src,*dst,*jdst;
{
	enc_T18(op,src,dst); /* encode same as T18 */
	enc_long(op,jdst,NO); /* save postword */
}

/**
*
* name		enc_T26 - encode type 26 instruction
*
* synopsis	enc_T26(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T26 instructions.
*
**/
enc_T26(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D6(dst->reg),8,6); /* encode register */
	op->opcode = f_insert(opc,e_S(src->space),16,1); /* encode space */
	insert(op,src,0,6);	/* encode address */
}

/**
*
* name		enc_T26a - encode type 26a instruction
*
* synopsis	enc_T26a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T26a instructions.
*
**/
enc_T26a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_S(src->space),16,1); /* encode space */
	if (dst->space == PSPACE)
		opc = f_insert (opc,1L,6,1);
	else {
		opc = f_insert (opc,e_S(dst->space),6,1);
		opc = f_insert (opc,1L,7,1);
	}
	opc = f_insert(opc,e_RRR(dst->mode,dst->reg),8,3); /* encode reg */
	op->opcode = f_insert(opc,e_MMM(dst->mode),11,3); /* encode mode */
	insert(op,src,0,6);	/* encode address */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T26b - encode type 26b instruction
*
* synopsis	enc_T26b(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T26b instructions.
*
**/
enc_T26b(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T26(op,dst,src); /* swap src and dst */
}

/**
*
* name		enc_T26c - encode type 26c instruction
*
* synopsis	enc_T26c(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T26c instructions.
*
**/
enc_T26c(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T26a(op,dst,src); /* swap src and dst */
}

/**
*
* name		enc_T27 - encode type 27 instruction
*
* synopsis	enc_T27(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T27 instructions.
*
**/
enc_T27(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode|MOVECBO,e_D6(src->reg),0,6); /* src reg */
	opc = f_insert(opc,e_S(dst->space),6,1); /* insert space */
	opc = f_insert(opc,e_RRR(dst->mode,dst->reg),8,3); /* insert reg */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	op->opcode = f_insert(opc,1L,16,1); /* set more bits */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T27a - encode type 27a instruction
*
* synopsis	enc_T27a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T27a instructions.
*
**/
enc_T27a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T27(op,dst,src); /* use T27 encoding */
}

/**
*
* name		enc_T28 - encode type 28 instruction
*
* synopsis	enc_T28(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T28 instructions.
*
**/
enc_T28(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode|MOVECBO,e_D6(dst->reg),0,6); /* dest reg */
	opc = f_insert(opc,1L,7,1); /* set bits */
	op->opcode = f_insert(opc,1L,16,1); /* set more bits */
	insert(op,src,8,8);	/* insert immediate */
}

/**
*
* name		enc_T29 - encode type 29 instruction
*
* synopsis	enc_T29(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T29 instructions.
*
**/
enc_T29(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode|MOVECBO,e_D6(src->reg),0,6); /* src reg */
	opc = f_insert(opc,1L,7,1); /* set bits */
	opc = f_insert(opc,e_D6(dst->reg),8,6); /* insert dst register */
	op->opcode = f_insert(opc,1L,14,1); /* set more bits */
}

/**
*
* name		enc_T29a - encode type 29a instruction
*
* synopsis	enc_T29a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T29a instructions.
*
**/
enc_T29a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T29(op,dst,src); /* use T29 encoding */
}

/**
*
* name		enc_T30 - encode type 30 instruction
*
* synopsis	enc_T30(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T30 instructions.
*
**/
enc_T30(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode|MOVECBO,e_D6(src->reg),0,6); /* src reg */
	opc = f_insert(opc,e_S(dst->space),6,1); /* insert space */
	op->opcode = f_insert(opc,1L,16,1); /* set bits */
	insert(op,dst,8,6);	/* insert address */
}

/**
*
* name		enc_T30a - encode type 30a instruction
*
* synopsis	enc_T30a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T30a instructions.
*
**/
enc_T30a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T30(op,dst,src); /* use T30 encoding */
}

/**
*
* name		enc_T32 - encode type 32 instruction
*
* synopsis	enc_T32(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T32 instructions.
*
**/
enc_T32(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode|MOVECBO,e_D6(src->reg),0,6); /* src reg */
	opc = f_insert(opc,e_S(dst->space),6,1); /* insert space */
	opc = f_insert(opc,e_RRR(dst->mode,dst->reg),8,3); /* dst register */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	op->opcode = f_insert(opc,1L,16,1); /* set more bits */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T32a - encode type 32a instruction
*
* synopsis	enc_T32a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T32a instructions.
*
**/
enc_T32a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T32(op,dst,src); /* use T32 encoding */
}

/**
*
* name		enc_T37 - encode type 37 instruction
*
* synopsis	enc_T37(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T37 instructions.
*
**/
enc_T37(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(xdst->mode,xdst->reg),8,3);/*x addr */
	opc = f_insert(opc,e_MMM(xdst->mode),11,2); /* insert x dst mode */
	opc = f_insert(opc,e_RRR(ydst->mode,ydst->reg),13,2); /* y eff addr */
	opc = f_insert(opc,e_YY(ysrc->reg),16,2); /* insert y src reg */
	opc = f_insert(opc,e_XX(xsrc->reg),18,2); /* insert x src reg */
	opc = f_insert(opc,e_MMM(ydst->mode),20,2); /* y dst mode */
	op->opcode = f_insert(opc,1L,23,1); /* set bit */
}

/**
*
* name		enc_T37a - encode type 37a instruction
*
* synopsis	enc_T37a(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T37a instructions.
*
**/
enc_T37a(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set x write bit */
	enc_T37(op,xdst,xsrc,ysrc,ydst); /* use T37 encoding */
}

/**
*
* name		enc_T37b - encode type 37b instruction
*
* synopsis	enc_T37b(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T37b instructions.
*
**/
enc_T37b(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	op->opcode = f_insert(op->opcode,1L,22,1); /* set y write bit */
	enc_T37(op,xsrc,xdst,ydst,ysrc); /* use T37 encoding */
}

/**
*
* name		enc_T37c - encode type 37c instruction
*
* synopsis	enc_T37c(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T37c instructions.
*
**/
enc_T37c(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,1L,15,1); /* set x write bit */
	op->opcode = f_insert(opc,1L,22,1); /* set y write bit */
	enc_T37(op,xdst,xsrc,ydst,ysrc); /* use T37 encoding */
}

/**
*
* name		enc_T37d - encode type 37d instruction	(Rev. C)
*
* synopsis	enc_T37d(op,xsrc,xdst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*
* description	Encodes T37d instructions.
*
**/
enc_T37d(op,xsrc,xdst)
struct obj *op;
struct amode *xsrc,*xdst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(xdst->mode,xdst->reg),8,3);/*x addr */
	opc = f_insert(opc,e_MMM(xdst->mode),11,3); /* insert x dst mode */
	opc = f_insert(opc,e_S(xdst->space),15,1); /* insert space */
	opc = f_insert(opc,e_D(xsrc->reg),16,1); /* insert x src/y dst reg */
	op->opcode = f_insert(opc,1L,19,1); /* set bit */
}

/**
*
* name		enc_T38 - encode type 38 instruction
*
* synopsis	enc_T38(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T38 instructions.
*
**/
enc_T38(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */
	long val;	/* temp */

	opc = f_insert(op->opcode,e_RRR(dst->mode,dst->reg),8,3); /* addr */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	opc = f_insert(opc,(val = e_DDDDD(src->reg)),16,3); /* low src reg */
	opc = f_insert(opc,(long)(val >> 3),20,2); /* insert high src reg */
	op->opcode = f_insert(opc,1L,22,1); /* set bit */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T38a - encode type 38a instruction
*
* synopsis	enc_T38a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T38a instructions.
*
**/
enc_T38a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T38(op,dst,src); /* use T38 encoding */
}

/**
*
* name		enc_T39 - encode type 39 instruction
*
* synopsis	enc_T39(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T39 instructions.
*
**/
enc_T39(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,19,1); /* set bit */
	enc_T38(op,src,dst); /* use T38 encoding */
}

/**
*
* name		enc_T39a - encode type 39a instruction
*
* synopsis	enc_T39a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T39a instructions.
*
**/
enc_T39a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T39(op,dst,src); /* use T39 encoding */
}

/**
*
* name		enc_T40 - encode type 40 instruction
*
* synopsis	enc_T40(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T40 instructions.
*
**/
enc_T40(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */
	long val;	/* temp */

	opc = f_insert(op->opcode,(val = e_DDDDD(src->reg)),16,3);
	opc = f_insert(opc,(long)(val >> 3),20,2); /* insert high src reg */
	op->opcode = f_insert(opc,1L,22,1); /* set bit */
	insert(op,dst,8,6);	/* insert address */
}

/**
*
* name		enc_T40a - encode type 40a instruction
*
* synopsis	enc_T40a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T40a instructions.
*
**/
enc_T40a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T40(op,dst,src); /* use T40 encoding */
}

/**
*
* name		enc_T41 - encode type 41 instruction
*
* synopsis	enc_T41(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T41 instructions.
*
**/
enc_T41(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,19,1); /* set bit */
	enc_T40(op,src,dst); /* use T40 encoding */
}

/**
*
* name		enc_T41a - encode type 41a instruction
*
* synopsis	enc_T41a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T41a instructions.
*
**/
enc_T41a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T41(op,dst,src); /* use T41 encoding */
}

/**
*
* name		enc_T42 - encode type 42 instruction
*
* synopsis	enc_T42(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T42 instructions.
*
**/
enc_T42(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(ydst->mode,ydst->reg),8,3);
	opc = f_insert(opc,e_MMM(ydst->mode),11,3); /* encode y addr. mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	opc = f_insert(opc,e_YY(ysrc->reg),16,2); /* insert y src. reg. */
	opc = f_insert(opc,e_X(xdst->reg),18,1); /* insert x dst. reg. */
	opc = f_insert(opc,e_D(xsrc->reg),19,1); /* insert x src. reg. */
	op->opcode = f_insert(opc,1L,20,1); /* set bit */
	enc_long(op,ydst,NO); /* handle longs */
}

/**
*
* name		enc_T42a - encode type 42a instruction
*
* synopsis	enc_T42a(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T42a instructions.
*
**/
enc_T42a(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T42(op,xsrc,xdst,ydst,ysrc); /* use T42 encoding */
}

/**
*
* name		enc_T43 - encode type 43 instruction
*
* synopsis	enc_T43(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T43 instructions.
*
**/
enc_T43(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(xdst->mode,xdst->reg),8,3);/* x dst */
	opc = f_insert(opc,e_MMM(xdst->mode),11,3); /* encode x addr. mode */
	opc = f_insert(opc,e_Y(ydst->reg),16,1); /* insert y dst. reg. */
	opc = f_insert(opc,e_D(ysrc->reg),17,1); /* insert y src. reg. */
	opc = f_insert(opc,e_XX(xsrc->reg),18,2); /* insert x src. reg. */
	op->opcode = f_insert(opc,1L,20,1); /* set bit */
	enc_long(op,xdst,NO); /* handle longs */
}

/**
*
* name		enc_T43a - encode type 43a instruction
*
* synopsis	enc_T43a(op,xsrc,xdst,ysrc,ydst)
*		struct obj *op;		object code structure
*		struct amode *xsrc;	X source address mode
*		struct amode *xdst;	X dest address mode
*		struct amode *ysrc;	Y source address mode
*		struct amode *ydst;	Y dest address mode
*
* description	Encodes T43a instructions.
*
**/
enc_T43a(op,xsrc,xdst,ysrc,ydst)
struct obj *op;
struct amode *xsrc,*xdst,*ysrc,*ydst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T43(op,xdst,xsrc,ysrc,ydst); /* use T43 encoding */
}

/**
*
* name		enc_T44 - encode type 44 instruction
*
* synopsis	enc_T44(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T44 instructions.
*
**/
enc_T44(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_LLL(src->reg),16,4); /* source register */
	op->opcode = f_insert(opc,1L,22,1); /* set bit */
	insert(op,dst,8,6);	/* insert address */
}

/**
*
* name		enc_T44a - encode type 44a instruction
*
* synopsis	enc_T44a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T44a instructions.
*
**/
enc_T44a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T44(op,dst,src); /* use T44 encoding */
}

/**
*
* name		enc_T45 - encode type 45 instruction
*
* synopsis	enc_T45(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T45 instructions.
*
**/
enc_T45(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(dst->mode,dst->reg),8,3); /* addr */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	opc = f_insert(opc,e_LLL(src->reg),16,4); /* insert source register */
	op->opcode = f_insert(opc,1L,22,1); /* set bit */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T45a - encode type 45a instruction
*
* synopsis	enc_T45a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T45a instructions.
*
**/
enc_T45a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T45(op,dst,src); /* use T45 encoding */
}

/**
*
* name		enc_T46 - encode type 46 instruction
*
* synopsis	enc_T46(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T46 instructions.
*
**/
enc_T46(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_DDDDD(dst->reg),16,5);
	op->opcode = f_insert(opc,1L,21,1); /* set bit */
	insert (op,src,8,8);	/* insert immediate */
}

/**
*
* name		enc_T49 - encode type 49 instruction
*
* synopsis	enc_T49(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	dest address mode
*
* description	Encodes T49 instructions.
*
**/
enc_T49(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_DDDDD(dst->reg),8,5); /* dest. reg. */
	opc = f_insert(opc,e_DDDDD(src->reg),13,5); /* insert src. reg. */
	op->opcode = f_insert(opc,1L,21,1); /* set bit */
}

/**
*
* name		enc_T50 - encode type 50 instruction
*
* synopsis	enc_T50(op,src)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*
* description	Encodes T50 instructions.
*
**/
enc_T50(op,src)
struct obj *op;
struct amode *src;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_RRR(src->mode,src->reg),8,3); /* addr */
	opc = f_insert(opc,e_MMM(src->mode),11,2); /* insert mode */
	op->opcode = f_insert(opc,0x81L,14,8); /* set bits */
}

/**
*
* name		enc_T52 - encode type 52 instruction
*
* synopsis	enc_T52(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T52 instructions.
*
**/
enc_T52(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D6(src->reg),0,6); /* insert src. reg. */
	opc = f_insert(opc,1L,7,1); /* set bit */
	opc = f_insert(opc,e_RRR(dst->mode,dst->reg),8,3); /* effect. addr */
	opc = f_insert(opc,e_MMM(dst->mode),11,3); /* insert mode */
	opc = f_insert(opc,1L,14,1); /* set bits */
	op->opcode = f_insert(opc,7L,16,3); /* set more bits */
	enc_long(op,dst,NO); /* handle longs */
}

/**
*
* name		enc_T52a - encode type 52a instruction
*
* synopsis	enc_T52a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T52a instructions.
*
**/
enc_T52a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T52(op,dst,src); /* use T52 encoding */
}

/**
*
* name		enc_T53 - encode type 53 instruction
*
* synopsis	enc_T53(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T53 instructions.
*
**/
enc_T53(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_D6(src->reg),0,6); /* insert src. reg. */
	op->opcode = f_insert(opc,7L,16,3); /* set bits */
	insert(op,dst,8,6);	/* insert address */
}

/**
*
* name		enc_T53a - encode type 53a instruction
*
* synopsis	enc_T53a(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T53a instructions.
*
**/
enc_T53a(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	op->opcode = f_insert(op->opcode,1L,15,1); /* set write bit */
	enc_T53(op,dst,src); /* use T53 encoding */
}

/**
*
* name		enc_T54 - encode type 54 instruction
*
* synopsis	enc_T54(op,src,dst)
*		struct obj *op;		object code structure
*		struct amode *src;	source address mode
*		struct amode *dst;	destination address mode
*
* description	Encodes T54 instructions.
*
**/
enc_T54(op,src,dst)
struct obj *op;
struct amode *src,*dst;
{
	long opc;	/* opcode */

	opc = f_insert(op->opcode,e_DDDDD(dst->reg),0,4); /* dst. reg. */
	opc = f_insert(opc,e_RRR(src->mode,src->reg),8,3); /* eff. addr. */
	op->opcode = f_insert(opc,e_MMM(src->mode),11,2); /* insert mode */
}
