#include "lnkdef.h"
#include "lnkdcl.h"
#if !LINT
#include <math.h>
#endif
#include <errno.h>
#if (UNIX || VMS) && !NeXT	/* Unix | VAX/VMS C compiler */
#if !LINT
#define HUGE_VAL HUGE
#else
#define HUGE_VAL 0.0
#endif
#endif
#if LSC || MPW
#define HUGE_VAL 99.e999
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

/**
*
* name		eval_ad - evaluate an address
*
* synopsis	er = eval_ad()
*		struct evres *er;	expression result / NULL if error
*
* description	Evaluate the expression as an address. The
*		expression result must be an absolute integer
*		within the range $0 - $FFFF or an error will result.
*
**/
struct evres *
eval_ad()
{
	struct evres *i,*eval();
	long val, acctolong ();

	i = eval();

	if( i != NULL ){
		if( i->etype != INT ){
			free_exp(i);
			error("Expression result must be integer");
			return(NULL);
			}
		val = acctolong (i);
		if( val < 0L || val > 65535L ){
			free_exp(i);
			error("Expression result too large");
			return(NULL);
			}
		i->uval.xvalue.ext = i->uval.xvalue.high = 0L;
		i->uval.xvalue.low = val;
		}
	return(i);
}

/**
*
* name		eval_int - evaluate an integer between $0 - $FFFF
*
* synopsis	er = eval_int()
*		struct evres *er;	expression result
*
* description	eval_int() will allow only absolute integer expressions. The 
*		result must be within the range $0-$FFFF.
**/
struct evres *
eval_int()
{
	struct evres *i,*eval();
	long val, acctolong ();

	i = eval();

	if( i != NULL ){
		if( i->etype != INT ){
			free_exp(i);
			error("Expression result must be integer");
			return(NULL);
			}
		if( i->rel ){
			free_exp(i);
			error("Expression result must be absolute");
			return(NULL);
			}
		val = acctolong (i);
		if( val < -32768L || val > 65535L ){
			free_exp(i);
			error("Expression result too large");
			return(NULL);
			}
		i->uval.xvalue.ext = i->uval.xvalue.high = 0L;
		i->uval.xvalue.low = val;
		}
	return(i);
}

/**
* name		eval --- evaluate expression
*
* synopsis	er = eval()    evaluate expression
*		struct evres *er;	evaluation result, NULL if error
*
* description	Eval will evaluate an expression pointed to by 
*		Optr, allowing for precedence of operators and parenthesis.
*		It will return a pointer to a dynamically allocated 
*		structure containing the results of the expression evaluation.
*		Errors will occur if the expression is missing, invalid,
*		or not terminated by a null or comma.
*
*		eval() will allow floating point or integer (absolute or
*		relocatable) expressions.  The linker version automatically
*		converts floating point values to fractional.
*
*      an expression is constructed like this:
*
*      expr ::=  term 	     |
*		 expr + term |
*                expr - term |
*                expr * term |
*                expr / term |
*                expr % term |	MOD
*                expr | term |	Bitwise OR
*                expr & term |	Bitwise AND
*                expr ^ term |	Bitwise EXOR
*		 expr && term |	Logical AND
*		 expr || term | Logical OR
*		 expr < term |
*		 expr > term |
*		 expr == term |
*		 expr <= term |
*		 expr >= term |
*		 expr != term |
*
*      term ::=  symbol |
*                * |             (* = current pc)
*                constant |
*		 (expr) |
*                [term]		(relocatable term)
*
*	terms can be preceded by any number of unary operators:
*               +       unary plus
*		-	unary minus
*		~	unary binary negate (one's complement)
*		!	unary logical negate
*
*      symbol ::= {valid assembler symbol}
*
*      constant ::= hex constant |
*                   binary constant |
*                   decimal constant |
*		    decimal floating point constant |
*                   ascii constant;
*
*      hex constant ::= $ {hex digits};
*
*      binary constant ::= % { 1 | 0 };
*
*      decimal constant ::= [`] {decimal digits};
*
*      decimal floating point constant ::= {decimal digit(s).decimal digit(s)}
*
*      ascii constant ::= {any printing char};
*
*	Operator precedence is as follows:
*		1. parenthetical expressions
*		2. unary minus
*		3. shift left, shift right
*		4. and, or, eor
*		5. mult, div, mod
*		6. add, subtract
*		7. relational operators
*
* 	Operators of equal precedence are evaluated left to right.
*
*	er->fwd_ref will be set non-zero if the expression contains any
*	forward references.
**/

/**
*
* name		eval - evaluate an integer or floating point expression
*
**/
struct evres *
eval()
{
	struct evres *i;
	struct evres *get_exp();

#if DEBUG 
        printf("Evaluating %s\n",Optr);
#endif
	if( *Optr == EOS ){
		error("Missing expression");
		return(NULL);
		}

	if( (i = get_exp()) == NULL ){	/* there was an error */
		return(NULL);
		}
#if DEBUG
        printf("Result:\n");
	switch (i->etype) {
		case INT:
			(void)printf("absolute integer value = %ld ($%lx)\n",i->uval.xvalue.low,i->uval.xvalue.low);
			break;
		case FPT:
			(void)printf("floating pt value = %g\n",i->uval.fvalue);
			break;
		}
#endif
	/* discard value if external reference or forward ref */
	if( i->secno < 0 || i->fwd_ref )
		switch (i->etype){
			case INT:
				i->uval.xvalue.ext  =
				i->uval.xvalue.high =
				i->uval.xvalue.low  = 0L;
			case FPT:
				i->uval.fvalue = 0.0;
			}

	if( *Optr != EOS && *Optr != ',' ){
		error("Extra characters beyond expression");
		free_exp(i);
		return(NULL);
		}
	return(i);
}

/**
*
* name		free_exp --- free the expression stack 
*
* synopsis	free_exp(ep)
*		struct evres *ep;	expression structure
*
* discussion	Intermediate and final forms of expression evaluation
*		are carried in dynamically allocated structures. This
*		routine will free the memory used by the structure pointed
*		to by ep.
*
**/
free_exp(ep)
struct evres *ep; /* pointer to expression */
{
	if (ep)
		fake_free((char *)ep);	/* free the evaluation result */
}

/**
*
* name		get_exp	--- start expression evaluation
*
* synopsis	er = get_exp()
*		struct evres *er;  evaluation result, NULL if error
*
* description	This routine is used to initialize the expression evaluation
*		by collecting the leftmost term and calling the evaluation
*		loop. This routine can be recursively called in expressions
*		involving parentheticals.
*
**/
static struct evres *
get_exp()
{
        struct evres *left;     /* left term for expression */
	struct evres *get_term(), *do_ops();

#if DEBUG
	(void)printf("get_exp: calling get_term, *Optr = %s\n", Optr);
#endif
        left = get_term();	/* get first part of expression */
	if( left == NULL )	/* hold off, there was an error */
		return(NULL);

#if DEBUG
	(void)printf("get_exp: calling do_ops\n");
#endif
	left = do_ops(left,99);	/* evaluate with lowest precedence */

	if (left && left->rel && left->etype != INT) {
		error("Relative expression must be integer");
		free_exp(left);
		return(NULL);
	}
	return (left);
}

/**
*
* name		do_ops --- process expression using precedence
*
* synopsis	er = do_ops(left,pprec)
*		struct evres *er;	evaluation result, NULL if error
*		struct evres *left;	left part of expression
*		int pprec;		preceding operator precedence
*
* description	The operator is examined, and if the precedence of the
*		preceding operators is higher or equal to the current 
*		precedence, then the routine returns with nothing done.
*		If the next operator has higher precedence than previous
*		operators, then the right part of the expression is collected.
*		The next operator is examined, and if it has higher precedence
*		than the current operator, then this routine is recursively
*		called with the right term being the left term for the next
*		operator. If the next operator has lower or equal precedence
*		to the current operator, then the left and right terms are
*		combined using the current operator, and the evaluation
*		loop continues.
*
*		Errors will result for divide by zero, shifts by invalid
*		amounts, attempts to use relocatable symbols with operators
*		other than add or subtract, attempts to combine floating point
*		and relocatable quantities, or attempts to use illegal 
*		operators for floating point numbers.
**/
static struct evres *
do_ops(left,pprec)
struct evres *left;	/* left part of expression */
int pprec;		/* precedence of preceding opts */
{
	struct evres *right;	/* right part of expression */
        int     opt;            /* operator */
	int	nxtopt;		/* next operator */
	int	cprec;		/* current precedence */
	struct evres *get_term();

#if DEBUG
	(void)printf("entering do_ops\n");
#endif
        while( (opt = is_op()) != NO ){
                cprec = op_prec(opt);	/* get precedence of current op */
		if( cprec >= pprec )	/* lower precedence */
			break;		/* go do previous ops */
		if( opt == SHFT_R || opt == SHFT_L || opt == EQUL ||
		    opt == LTEQL  || opt == GTEQL  || opt == NEQUL ||
		    opt == LOGAND || opt == LOGOR )
			Optr += 2;
		else
			Optr++;
               	if( (right = get_term()) == NULL ){ /* pickup rightmost side */
			free_exp(left); /* stop for errors */
			return(NULL);
			}

		/* check whether or not to hold off current operation if */
		/* succeeding op has higher precedence */
		if( (nxtopt = is_op()) != NO ) 
			if( op_prec(nxtopt) < cprec ) 
				if( (right = do_ops(right,cprec)) == NULL ){
					free_exp(left); /* stop for error */
					return(NULL);
					}

		/* check memory space compatibility */
		if( mem_compat (left->mspace, right->mspace) )
			left->mspace = right->mspace;
		else{
			error("Expression involves incompatible memory spaces");
			free_exp(right);
			free_exp(left);
			return(NULL);
			}

		/* check operations on relocatable terms */
		if( !Cflag && User_exp && opt != ADD && opt != SUB &&
		    (left->rel || right->rel) ) {
			free_exp(left);
			free_exp(right);
			error("Operation not allowed with relative term");
			return(NULL);
			}

		if ( !(*Exop_rtn[opt]) (left, right) )
			return (NULL);

		if( right->fwd_ref )	/* save forward reference indicator */
			left->fwd_ref = YES;
		if( right->secno < 0 )	/* save external ref indicator */
			left->secno = right->secno;
		free_exp(right);
                }
	return(left);
}

/**
*
* name		is_op --- does Optr point at an expression operator?
*
* synopsis	op = is_op()
*		int op;		which operator, NO if none
*
**/
static
is_op()
{
	int op = NONE;
	char nextch = *(Optr+1);

	switch(*Optr){
		case '-':
			op = SUB;
			break;
		case '*':
			op = MUL;
			break;
		case '/':
			op = DIV;
			break;
		case '%':
			op = MOD;
			break;
		case '^':
			op = EOR;
			break;
		case '@':
			op = BSIZE;
			break;
		case '<':
			if (nextch == '<')
				op = SHFT_L;
			else if (nextch == '=')
				op = LTEQL;
			else
				op = LTHAN;
			break;
		case '>':
			if (nextch == '>')
				op = SHFT_R;
			else if (nextch == '=')
				op = GTEQL;
			else
				op = GTHAN;
			break;
		case '+':
			if (nextch != '+')	/* string concatenation */
				op = ADD;
			break;
		case '=':
			if (nextch == '=')
				op = EQUL;
			break;
		case '!':
			if (nextch == '=')
				op = NEQUL;
			break;
		case '|':
			if (nextch == '|')
				op = LOGOR;
			else
				op = OR;
			break;
		case '&':
			if (nextch == '&')
				op = LOGAND;
			else
				op = AND;
			break;
		default:
			break;
		}

	return(op);
}

/**
*
* name		op_prec --- determine operator precedence
*
* synopsis	pr = op_prec(o)
*		int pr;		precedence value
*		int o;		operator
*
**/
static
op_prec(o)
int o;	/* operator */
{
	switch (o) {
		case MUL:
		case DIV:
		case MOD:
			return (1);
		case ADD:
		case SUB:
			return (2);
		case AND:
		case OR:
		case EOR:
			return (3);
		case SHFT_L:
		case SHFT_R:
			return (4);
		case LTHAN:
		case GTHAN:
		case EQUL:
		case LTEQL:
		case GTEQL:
		case NEQUL:
			return (5);
		case LOGAND:
		case LOGOR:
			return (6);
		default:
			return (0);
	}
}

/**
*
* name		bad_op --- bad expression operator
*
* synopsis	bad_op ()
*
* description	Fatal expression operator error
*
**/
struct evres *
bad_op ()
{
	fatal ("Expression operator failure");
}

/**
* name		get_term --- evaluate a single item in an expression
*
* synopsis	t = get_term
*		struct evres *t;	term
*
* description	Returns value of the term pointed to by Optr. Unary
*		minus is accomodated. Errors will result for invalid
*		constants and missing or unknown elements.
*
**/
static struct evres *
get_term()
{
        char    *sym, *tmp;
	char	*get_sym(), *get_string();
	double optof();
	struct nlist *sp,*lkupsym();
	struct evres *p;
	int	i, j;

#if DEBUG
	(void)printf("entering get_term\n");
#endif
#ifdef UNARY_PLUS	/* can't do this because of string concatenation */
        if( *Optr == '+' ){
                Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		return(p);
                }
#endif
        if( *Optr == '-' ){
                Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		do_negate (p);
		return(p);
                }

	if( *Optr == '~' ){
                Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		if (p->rel) {
			error("Operation not allowed with relative term");
			free_exp(p);
			return(NULL);
		}
		if( p->etype == INT ){
			do_onecomp (p);
			return(p);
			}
		else{
			error("Illegal operator for floating point element");
			free_exp(p);
			return(NULL);
			}
		}

	if( *Optr == '!' ){
                Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		if (p->rel) {
			error("Operation not allowed with relative term");
			free_exp(p);
			return(NULL);
		}
		do_not (p);
		p->mspace = NONE;
		return(p);
		}

	if( *Optr == '(' ){	/* parenthetical expression */
                Optr++;
		/* get the expression in the parenthesis */
		if( (p = get_exp()) == NULL ) /* stop for errors */
			return(NULL);
		if( *Optr != ')' ){
			free_exp(p);
			error("Missing ')' in expression");
			return(NULL);
			}
                Optr++;	/* skip right parenthesis */
		return(p);
		}

	if( *Optr == '[' ){	/* relocatable term */
                Optr++;
		/* get the term in the brackets */
		if( (p = get_term()) == NULL ) /* stop for errors */
			return(NULL);
		if( p->etype != INT || Cspace == NONE ){
			free_exp(p);
			error("Invalid relative expression");
			return(NULL);
			}
		/* adjust value */
		i = spc_off (Cspace) - 1;
		j = ctr_off (Rcntr);
		if (i < 0 || j < 0)	/* check offsets */
			fatal ("Space/counter offset failure");
		p->uval.xvalue.low += Csect->start[i][j] +
			 Secmap[Secno].offset[i][j];
		if( *Optr != ']' ){
			free_exp(p);
			error("Missing ']' in expression");
			return(NULL);
			}
                Optr++;	/* skip right bracket */
		return(p);
		}

	if( *Optr == '{' ){	/* user expression */
                Optr++;
		User_exp = YES;	/* set flag */
		/* get the expression in the braces */
		p = get_exp();
		User_exp = NO;	/* reset flag */
		if( p == NULL )	/* stop for errors */
			return(NULL);
		if( *Optr != '}' ){
			free_exp(p);
			error("Missing '}' in expression");
			return(NULL);
			}
                Optr++;		/* skip right brace */
		return(p);
		}

	p = (struct evres *)alloc(sizeof(struct evres));
	if( p == NULL )
		fatal("Out of memory - cannot evaluate expression");
	p->uval.xvalue.ext = p->uval.xvalue.high = p->uval.xvalue.low = 0L;
	p->bcount = p->wcount = 0;
	p->etype = INT;		/* default to integer type */
	p->force = NONE;	/* default to no forcing */
	p->fwd_ref = NO;	/* default to no forward reference */
	p->mspace = NONE;	/* default to memory space of none */
	p->size = SIZES;	/* default word size */
	p->rel = NO;		/* default to absolute */
	p->secno = 0;		/* default to global section */
	i = 0;

        if( *Optr=='%' ||
	    (Radix == 2 && ( *Optr == '0' || *Optr == '1' )) ){ /* binary constant */
		if (*Optr == '%')
                	Optr++;
                while( *Optr == '0' || *Optr == '1' ){
			++i;
                        p->uval.xvalue.low = (p->uval.xvalue.low << 1) +
					     ((*Optr)-'0');
			p->uval.xvalue.high <<= 1;
			p->uval.xvalue.ext  <<= 1;
			do_carry (p);
                	Optr++;
			}
		if( i==0 ){
			free_exp(p);
			error("Binary constant expected");
			return(NULL);
			}
		if (p->uval.xvalue.ext)		/* set appropriate size */
			p->size = SIZEA;
		else if (p->uval.xvalue.high)
			p->size = SIZEL;
		return(p);
                }

	if( *Optr=='$' ||
	    (Radix == 16 && isxdigit(*Optr)) ){	/* hex constant */
		if (*Optr == '$' || *Optr == '0')
	                Optr++;
		if (mapdn(*Optr) == 'x')
			Optr++;
                while( isxdigit(*Optr)){
			++i;
                        if( *Optr > '9' )
                		p->uval.xvalue.low =
					(p->uval.xvalue.low << 4) +
					10 + (mapdn(*Optr)-'a');
                        else
                          	p->uval.xvalue.low =
					(p->uval.xvalue.low << 4) +
					((*Optr)-'0');
			p->uval.xvalue.high <<= 4;
			p->uval.xvalue.ext  <<= 4;
			do_carry (p);
			Optr++;
			}
		if( i == 0 ){
			free_exp(p);
			error("Hex constant expected");
			return(NULL);
			}
		if (p->uval.xvalue.ext)		/* set appropriate size */
			p->size = SIZEA;
		else if (p->uval.xvalue.high)
			p->size = SIZEL;
		return(p);
                }

	if( *Optr=='`' ||
	    (Radix == 10 && (isdigit(*Optr) || *Optr == '.')) ){ /* decimal constant */
		if (*Optr == '`')
			Optr++;
		tmp = Optr; /* remember start of number */
                while( isdigit(*Optr) ){ /* guess integer */
			++i;
                        p->uval.xvalue.low = (p->uval.xvalue.low * 10) + ( (*Optr)-'0');
			p->uval.xvalue.high *= 10;
			p->uval.xvalue.ext  *= 10;
			do_carry (p);
			Optr++;
			}
		if( *Optr == '.' || *Optr == 'e' || *Optr == 'E' ){
			Optr = tmp; /* floating point. Reset pointer */
			p->uval.fvalue = optof();
			p->size = SIZEF;
			p->etype = FPT; /* indicate floating point */
			}
		else if( i == 0 ){
			free_exp(p);
			error("Decimal constant expected");
			return(NULL);
			}
		if (p->etype == INT)
			if (p->uval.xvalue.ext)	/* set appropriate size */
				p->size = SIZEA;
			else if (p->uval.xvalue.high)
				p->size = SIZEL;
		return(p);
                }

	if( *Optr == '*' ){ /* current program counter */
		Optr++;
		p->uval.xvalue.low = Old_pc;
		p->mspace = Cspace;
		return(p);
		} 

	if(*Optr==STR_DELIM || *Optr==XTR_DELIM){   /* literal string */
		if( (Optr = get_string(Optr,String)) == NULL ){
			free_exp(p); /* bad string */
			return(NULL);
			}
		i = strlen(String);
		if( i > 4 )
			i = 4;
		tmp = String;
		for(;i>0;--i)
			p->uval.xvalue.low = (p->uval.xvalue.low << 8) +
					      *tmp++;
		if( strlen(String) > 4 )
			warn("String truncated in expression evaluation");
		return(p);
                }

	if( (sym = get_sym()) == NULL ) {	/* get symbol */
		free_exp (p);			/* there was an error */
		return (NULL);
	}
	if( (sp = lkupsym(sym)) == NULL ) {	/* lookup symbol */
		p->secno = -1;			/* external reference */
		p->fwd_ref = YES;		/* set forward ref flag */
		instlxrf (sym);			/* put in table */
	} else {
		if( sp->attrib & INT ) {	/* integer */
			p->uval.xvalue.ext  = sp->def.xdef.ext;
			p->uval.xvalue.high = sp->def.xdef.high;
			p->uval.xvalue.low  = sp->def.xdef.low;
		} else if( sp->attrib & FPT ) { /* floating point */
			p->uval.fvalue = sp->def.fdef;
			p->etype = FPT;
		}
		if (sp->attrib & LONG)		/* long value */
			p->size = SIZEL;
		p->mspace = sp->attrib & MSPCMASK;/* copy mem space */
		p->rel = (sp->attrib & REL) != 0; /* set rel status */
		if (sp->sec)	/* if symbol is in section */
			p->secno = sp->sec->secno;/* save sect. no. */
	}
	return(p);
}

/**
*
* name		do_add --- add left and right expression operands
*
* synopsis	do_add (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Add left and right parts of expression; check for data
*		type before performing operation.  Set expression type and
*		memory space before returning.
*
**/
struct evres *
do_add (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	double itod ();

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if (!adj_rel (left, right, ADD))
			return (NULL);
		left->uval.xvalue.ext  += right->uval.xvalue.ext;
		left->uval.xvalue.high += right->uval.xvalue.high;
		left->uval.xvalue.low  += right->uval.xvalue.low;
		do_carry (left);
		left->rel = right->rel ? YES : left->rel;
		}
	else{ /* at least one is floating point */
		if (left->rel || right->rel) {
			free_exp(left);
			free_exp(right);
			error("Floating point not allowed in relative expression");
			return(NULL);
		}
		left->uval.fvalue = itod (left) + itod (right);
		left->etype = FPT;
		left->mspace = NONE;
		left->size = SIZEF;
		}
	return (left);
}

/**
*
* name		do_sub --- subtract left and right expression operands
*
* synopsis	do_sub (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Subtract left and right parts of expression; check for data
*		type before performing operation.  Set expression type and
*		memory space before returning.
*
**/
struct evres *
do_sub (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	double itod ();

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if (!adj_rel (left, right, SUB))
			return (NULL);
		do_negate (right);
		(void)do_add (left, right);
		do_negate (right);
		left->rel = right->rel ? YES : left->rel;
		}
	else{ /* at least one is floating point */
		if (left->rel || right->rel) {
			free_exp(left);
			free_exp(right);
			error("Floating point not allowed in relative expression");
			return(NULL);
		}
		left->uval.fvalue = itod (left) - itod (right);
		left->etype = FPT;
		left->mspace = NONE;
		left->size = SIZEF;
		}
	return (left);
}

/**
*
* name		adj_rel --- adjust relative term
*
* synopsis	yn = adj_rel (left, right, op)
*		int yn;			YES = relative terms have opp. signs
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*		int op;			operation to perform (ADD/SUB)
*
* description	Examine left and right parts of expression; if one is
*		relative and the other absolute, adjust the resulting
*		term to be relative.  If both are relative, check that
*		each have opposing signs and make the result absolute.
*
**/
static
adj_rel (left, right, op)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
int op;			/* operation to perform */
{
	long lval, rval;

	lval = left->uval.xvalue.low;
	rval = right->uval.xvalue.low;
	if (!left->rel && right->rel)
		left->rel = YES;		/* relative term */
	else if (left->rel && right->rel) {	/* both relative */
		if ( (lval < 0 && rval < 0 && op == ADD) ||
		     (lval > 0 && rval > 0 && op == ADD) ||
		     (lval < 0 && rval > 0 && op == SUB) ||
		     (lval > 0 && rval < 0 && op == SUB) ||
		     (!Cflag && left->secno >= 0 && right->secno >= 0 &&
			left->secno != right->secno) ) {
			error ("Invalid relative expression");
			free_exp (left);
			free_exp (right);
			return (NO);
		}
		left->rel = NO;			/* absolute term */
	}
	return (YES);
}

/**
*
* name		do_mul --- multiply left and right expression operands
*
* synopsis	do_mul (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Multiply left and right parts of expression; check for data
*		type before performing operation.  Set expression type and
*		memory space before returning.
*
**/
struct evres *
do_mul (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	int n,m,sign;
	unsigned int acc;
	int resl[8];
	int frl[8]; /* used to hold fragmented (8 bits each) left value */
	int frr[8]; /* used to hold fragmented (8 bits each) right value */
	double itod();

	if( left->etype == INT && right->etype == INT ) { /* both integer */
		sign=0;
		if(left->uval.xvalue.ext & BBITMASK) {
			do_negate(left);
			sign |= 1;
		}
		if(right->uval.xvalue.ext & BBITMASK) {
			do_negate(right);
			sign |= 2;
		}
		sbig (left->uval.xvalue.ext,
		      left->uval.xvalue.high,
		      left->uval.xvalue.low,
		      frl);
		sbig (right->uval.xvalue.ext,
		      right->uval.xvalue.high,
		      right->uval.xvalue.low,
		      frr);
		acc=0;
		for (n = 0; n < BYTEBITS - 1; n++) {
			for (m = 0; m <= n; m++) {
				acc += frl[m] * frr[n-m];
			}
			resl[n] = acc & BYTEMASK;
			acc >>= BYTEBITS;
		}
		mbig (resl,
		      &left->uval.xvalue.ext,
		      &left->uval.xvalue.high,
		      &left->uval.xvalue.low);
		switch (sign) {
			case 1:
				do_negate(left);
				break;
			case 2:
				do_negate(left);
				/* fall through */
			case 3:
				do_negate(right);
				break;
		}
		if (right->size > left->size)
			left->size = right->size;

	} else {	/* at least one is floating point */

		left->uval.fvalue = itod(left) * itod(right);
		left->etype = FPT;
		left->mspace = NONE;
		left->size = SIZEF;
	}
	return (left);
}

/**
*
* name		do_div --- divide left and right expression operands
*
* synopsis	do_div (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Divide left and right parts of expression; check for data
*		type before performing operation.  Set expression type and
*		memory space before returning.  Check for divide by zero.
*
**/
struct evres *
do_div (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	long lext, lhigh, llow;
	double itod(), fdr;

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if( right->uval.xvalue.ext  == 0L &&
		    right->uval.xvalue.high == 0L &&
		    right->uval.xvalue.low  == 0L ){
			free_exp(right);
			free_exp(left);
			error("Divide by zero");
			return(NULL);
			}
	dvbig(left->uval.xvalue.ext,
	      left->uval.xvalue.high,
	      left->uval.xvalue.low,
	      right->uval.xvalue.ext,
	      right->uval.xvalue.high,
	      right->uval.xvalue.low,
	      &left->uval.xvalue.ext,
	      &left->uval.xvalue.high,
	      &left->uval.xvalue.low,
	      &lext, &lhigh, &llow); 
		}

	else{	/* at least one is floating point */

		fdr = itod (right);
		if (fdr == 0.0){
			free_exp(right);
			free_exp(left);
			error("Divide by zero");
			return(NULL);
			}
		left->uval.fvalue = itod (left) / fdr;
		left->etype = FPT;
		left->mspace = NONE;
		left->size = SIZEF;
		}
	return (left);
}

/**
*
* name		do_mod --- left modulo right expression operands
*
* synopsis	do_mod (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Perform left modulo right of expression;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_mod (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	long lext, lhigh, llow;
	double itod(), fdr;
	double fmod ();

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if( right->uval.xvalue.ext  == 0L &&
		    right->uval.xvalue.high == 0L &&
		    right->uval.xvalue.low  == 0L ){
			free_exp(right);
			free_exp(left);
			error("Divide by zero");
			return(NULL);
			}
	dvbig(left->uval.xvalue.ext,
	      left->uval.xvalue.high,
	      left->uval.xvalue.low,
	      right->uval.xvalue.ext,
	      right->uval.xvalue.high,
	      right->uval.xvalue.low,
	      &lext, &lhigh, &llow,
	      &left->uval.xvalue.ext,
	      &left->uval.xvalue.high,
	      &left->uval.xvalue.low);
		}

	else{	/* at least one is floating point */

		fdr = itod (right);
		if (fdr == 0.0){
			free_exp(right);
			free_exp(left);
			error("Divide by zero");
			return(NULL);
			}
		left->uval.fvalue = fmod (itod (left), fdr);
		left->etype = FPT;
		left->mspace = NONE;
		left->size = SIZEF;
		}
	return (left);
}

/**
*
* name		do_and --- bitwise AND left and right expression operands
*
* synopsis	do_and (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Perform bitwise AND of left and right parts of expression;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_and (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	if( left->etype == INT && right->etype == INT ){ /* both integer */
		left->uval.xvalue.ext  &= right->uval.xvalue.ext;
		left->uval.xvalue.high &= right->uval.xvalue.high;
		left->uval.xvalue.low  &= right->uval.xvalue.low;
		}
	else{
		free_exp(left);
		free_exp(right);
		error("Illegal operator for floating point element");
		return(NULL);
		}
	return (left);
}

/**
*
* name		do_or --- bitwise OR left and right expression operands
*
* synopsis	do_or (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Perform bitwise OR of left and right parts of expression;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_or (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	if( left->etype == INT && right->etype == INT ){ /* both integer */
		left->uval.xvalue.ext  |= right->uval.xvalue.ext;
		left->uval.xvalue.high |= right->uval.xvalue.high;
		left->uval.xvalue.low  |= right->uval.xvalue.low;
		}
	else{
		free_exp(left);
		free_exp(right);
		error("Illegal operator for floating point element");
		return(NULL);
		}
	return (left);
}

/**
*
* name		do_eor --- bitwise XOR left and right expression operands
*
* synopsis	do_eor (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Perform bitwise exclusive-OR of left and right parts of expression;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_eor (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	if( left->etype == INT && right->etype == INT ){ /* both integer */
		left->uval.xvalue.ext  ^= right->uval.xvalue.ext;
		left->uval.xvalue.high ^= right->uval.xvalue.high;
		left->uval.xvalue.low  ^= right->uval.xvalue.low;
		}
	else{
		free_exp(left);
		free_exp(right);
		error("Illegal operator for floating point element");
		return(NULL);
		}
	return (left);
}

/**
*
* name		do_shftl --- shift left operand left by right operand bits
*
* synopsis	do_shftl (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Shift left operand left by right operand bits;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_shftl (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	long shift;

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if( (shift = acctolong (right)) < 0L ||
		    shift > (long)AWORDBITS ){
			free_exp(right);
			free_exp(left);
			error("Invalid shift amount");
			return(NULL);
			}
		while (shift--) {
			left->uval.xvalue.low  <<= 1;
			left->uval.xvalue.high <<= 1;
			left->uval.xvalue.ext  <<= 1;
			do_carry (left);
			}
		}
	else{
		free_exp(left);
		free_exp(right);
		error("Illegal operator for floating point element");
		return(NULL);
		}
	return (left);
}

/**
*
* name		do_shftr --- shift left operand right by right operand bits
*
* synopsis	do_shftr (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Arithmetic shift of left operand right by right operand bits;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_shftr (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	long shift;

	if( left->etype == INT && right->etype == INT ){ /* both integer */
		if( (shift = acctolong (right)) < 0L ||
		    shift > (long)AWORDBITS ){
			free_exp(right);
			free_exp(left);
			error("Invalid shift amount");
			return(NULL);
			}
		while (shift--) {
			left->uval.xvalue.low  >>= 1;
			if (left->uval.xvalue.high & 1L)
				left->uval.xvalue.low |= WBITMASK;
			left->uval.xvalue.high >>= 1;
			if (left->uval.xvalue.ext & 1L)
				left->uval.xvalue.high |= WBITMASK;
			left->uval.xvalue.ext  >>= 1;
			if (left->uval.xvalue.ext & 0x40L)
				left->uval.xvalue.high |= BBITMASK;
			}
		}
	else{
		free_exp(left);
		free_exp(right);
		error("Illegal operator for floating point element");
		return(NULL);
		}
	return (left);
}

/**
*
* name		do_logand --- boolean left operand logical AND with right operand
*
* synopsis	do_logand (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand logically ANDed with
*		right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_logand (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	left->uval.xvalue.low = do_test (left) && do_test (right);
	left->uval.xvalue.ext = left->uval.xvalue.high = 0L;
	left->etype = INT; /* convert result to integer */
	left->mspace = NONE;
        left->size = SIZES;
	return (left);
}

/**
*
* name		do_logor --- boolean left operand logical OR with right operand
*
* synopsis	do_logor (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand logically ORed with
*		right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_logor (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	left->uval.xvalue.low = do_test (left) || do_test (right);
	left->uval.xvalue.ext = left->uval.xvalue.high = 0L;
	left->etype = INT; /* convert result to integer */
	left->mspace = NONE;
        left->size = SIZES;
	return (left);
}

/**
*
* name		do_lthan --- boolean left operand less than right operand
*
* synopsis	do_lthan (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand less than right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_lthan (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, LTHAN);
	return (left);
}

/**
*
* name		do_gthan --- boolean left operand greater than right operand
*
* synopsis	do_gthan (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand greater than right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_gthan (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, GTHAN);
	return (left);
}

/**
*
* name		do_equl --- boolean left operand equal to right operand
*
* synopsis	do_equl (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand equal to right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_equl (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, EQUL);
	return (left);
}

/**
*
* name		do_lteql --- boolean left operand less than or equal to
*			     right operand
*
* synopsis	do_lteql (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand less than or equal
*		to right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_lteql (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, LTEQL);
	return (left);
}

/**
*
* name		do_gteql --- boolean left operand greater than or equal to
			     right operand
*
* synopsis	do_gteql (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand greater than or equal to
*		right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_gteql (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, GTEQL);
	return (left);
}

/**
*
* name		do_nequl --- boolean left operand not equal to right operand
*
* synopsis	do_nequl (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Predicate returning left operand not equal to right operand;
*		check for data type before performing operation.  Set
*		expression type and memory space before returning.
*
**/
struct evres *
do_nequl (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	do_compare (left, right, NEQUL);
	return (left);
}

/**
*
* name		do_bsize --- check of expression value size
*
* synopsis	do_bsize (left, right)
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	This routine checks the size of the left expression value
*		based upon the right expression value.
*
**/
struct evres *
do_bsize (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	int ok = YES;
	long val;

	if( left->etype == FPT )	/* float; convert to fractional */
		dtof (left->uval.fvalue, SIZES, left);
	val = left->uval.xvalue.low;

	if( left->etype != INT || right->etype != INT ){
		free_exp(left);
		free_exp(right);
		error("Invalid relative expression");
		return (NULL);
		}

	switch ((int)right->uval.xvalue.low) {
		case NIMMED:
			if (!wordbits (val)) {
				error ("Immediate value too large");
				ok = NO;
			}
			break;
		case SIMMED:
			if (!eightbits (val)) {
				error ("Immediate value too large");
				ok = NO;
			}
			break;
		case MIMMED:
			if (!twelvebits (val)) {
				error ("Immediate value too large");
				ok = NO;
			}
			break;
		case IABSOL:
			if (!ioaddress (val)) {
				error ("I/O address too small");
				ok = NO;
			}
			break;
		case SABSOL:
			if (!sixbits (val)) {
				error ("Address value too large");
				ok = NO;
			}
			break;
		case MABSOL:
			if (!twelvebits (val)) {
				error ("Address value too large");
				ok = NO;
			}
			break;
		default:
			break;
	}

	if (!ok) {
		free_exp (left);
		free_exp (right);
		return (NULL);
	}
	return (left);
}

/**
*
* name		optof - convert ASCII to float
*
* synopsis	double optof()
*		
* description	These functions convert a character string pointed to by Optr
*		to a double-precision floating-point number.  The first
*		unrecognized character ends the conversion.  Atof recognizes
*		a string of digits optionally containing a decimal point,
*		then an optional e or E followed by an optionally signed
*		integer.  This function was originally derived from the
*		Lattice C source library.
**/
static
double optof()
{
	double a=0, b=0, fracmul=1, pwr=0, e, ans;
	int pminus = NO;
	int pflag = NO; /* NO: no e or E encountered  YES: e or E found */
	char digit;
	extern int errno;
#if UNIX
	double pow();
#endif

	/* Optr is pointing to a digit */
	Optr--;
	while ((digit = *++Optr) == '0'); /* skip leading zeros */
	while ('0'<=digit && digit<='9') {
		a = a*10 + (digit - '0');
		digit = *++Optr;
	}

	/* Optr may be pointing to '.' */
	if (*Optr == '.') {
		digit = *++Optr;
		while ('0'<=digit && digit<='9') {
			fracmul = fracmul*0.1;
			b = b + (digit-'0')*fracmul;
			digit = *++Optr;
		}
	}

	/* Optr may be pointing to 'e' or 'E' */
	if (*Optr == 'e' || *Optr == 'E') {
		Optr++;
		/* check sign,if any */
		if (*Optr=='+')
			Optr++;
		else if (*Optr=='-') {
			pminus = YES;
			Optr++;
		}

	/* now, Optr may be pointing to a power digit */
		Optr--;
		while ((digit = *++Optr) == '0'); /* skip leading zeros */
		if ('0'<=digit && digit<='9') {
			pflag = YES; /* there is an exponent number */
			pwr = digit - '0';
			digit = *++Optr;
			while ('0'<=digit && digit<='9') {
				pwr = pwr*10 + (digit - '0');
				digit = *++Optr;
			}
		}
	
	} /* end of if (*Optr == 'e' && *Optr == 'E') { */

	if (!pflag) return(a+b);
	e = pow(10.0,pwr);	

	if (pminus == YES) 
		e = 1.0/e;

	ans = (a+b)*e;

	if (ans >= HUGE_VAL) { /* check overflow again */
		errno = ERANGE;
		ans = HUGE_VAL;
	}

	return(ans);

}

#if AZTEC || GCC
/**
*
*	*** derived from Lattice C library function
*
* name
*	fmod - remainder functions
*
* synopsis
*	double fmod(x, y)
*	double x, y;
*
* description
*	fmod returns x if y is zero, otherwise the number f with the same
*	sign as x, such that x = iy + f for some integer i, and |f| < |x|.
*	This function was adapted from the Lattice C source library.
**/
static double
fmod(x, y)
double x, y;
{ 
	/* x = i*y + f */
	double i, intpart;
	double modf();

	i = x/y;
	i = modf(i, &intpart);

	return( x - intpart*y );

} /* end of fmod() */
#endif

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
	if( val > 0x3fL || val < 0L )
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
	if( val > 0xffL || val < 0L )
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
	if( val > 0xfffL || val < 0L )
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
static
ioaddress(val)
long val;
{
	if( val > 0xffffL || val < 0xffc0L )
		return(NO);

	return(YES);
}

/*
*****************************************************************************
*
*	JAY NORWOOD'S LONG (48-BIT) ARITHMETIC ROUTINES
*
*****************************************************************************
*/

double itod(p) /* INTEGER TO DOUBLE */
struct evres *p;
{ /* p contains 3,6 or 7 byte integer value.  This will return double float equivalent. */
   double rv,litod(),vitod(),aitod();
   if (p->etype==FPT) rv=p->uval.fvalue;
   else{
     switch (p->size){
         case 3: rv= litod(p->uval.xvalue.low);break;
         case 6: rv= vitod(p->uval.xvalue.high,p->uval.xvalue.low);break;
/*
         case 7: rv= aitod(p->uval.xvalue.ext,p->uval.xvalue.high,p->uval.xvalue.low);break;
*/
         default:  rv=0.0; break; /* error condition */
         }
      }
   return(rv);
}

#ifdef BIT48
dtoi(dv,size,p) /* DOUBLE TO INTEGER */
double dv;
int size;
struct evres *p;
{ /* return 3,6 or 7 byte integer value equivalent to double float dv value */
     long dtoli();
     p->uval.xvalue.ext = p->uval.xvalue.high = p->uval.xvalue.low = 0L;
     switch (size){
         case 3: p->uval.xvalue.low=dtoli(dv);
                 if (p->uval.xvalue.low&0x800000l){
                    p->uval.xvalue.high=0xffffffl; /* sign extend */
                    p->uval.xvalue.ext=0xffl;
                    }
                 break;
         case 6: dtovi(dv,&p->uval.xvalue.high,&p->uval.xvalue.low);
                 if (p->uval.xvalue.high&0x800000l)
                    p->uval.xvalue.ext=0xffl; /* sign extend */
                 break;
/*
         case 7: dtoai(dv,&p->uval.xvalue.ext,
                          &p->uval.xvalue.high,
                          &p->uval.xvlaue.low);
                 break;
*/
         }
	if (p->uval.xvalue.ext != 0L && p->uval.xvalue.ext != 0xFFL)
		p->size = SIZEA;
	else if (p->uval.xvalue.high != 0L &&
		 p->uval.xvalue.high != 0xFFFFFFL)
		p->size = SIZEL;
	else
		p->size = SIZES;
	p->etype = INT;
}
#endif

#ifdef BIT48
double ftod(p) /* FRACTIONAL TO DOUBLE */
struct evres *p;
{ /* p contains 3,6 or 7 byte integer value.  This will return double float equivalent. */
   double rv,lftod(),vftod();
   if (p->etype==FPT) rv=p->uval.fvalue;
   else{
     switch (p->size){
         case 3: rv= lftod(p->uval.xvalue.low);break;
         case 6: rv= vftod(p->uval.xvalue.high,p->uval.xvalue.low);break;
/*
         case 7: rv= aftod(p->uval.xvalue.ext,p->uval.xvalue.high,p->uval.xvalue.low);break;
*/
         default:  rv=0.0; break; /* error condition */
         }
      }
   return(rv);
}
#endif

dtof(dv,size,p) /* DOUBLE TO FRACTIONAL */
double dv;
int size;
struct evres *p;
{ /* return 3,6 or 7 byte integer value equivalent to double float dv value */
     long dtolf();
     p->uval.xvalue.ext = p->uval.xvalue.high = p->uval.xvalue.low = 0L;
     switch (size){
         case 3: p->uval.xvalue.low=dtolf(dv);
                 if (p->uval.xvalue.low&0x800000l){
                    p->uval.xvalue.high=0xffffffl; /* sign extend */
                    p->uval.xvalue.ext=0xffl;
                    }
                 break;
         case 6: dtovf(dv,&p->uval.xvalue.high,&p->uval.xvalue.low);
                 if (p->uval.xvalue.high&0x800000l)
                    p->uval.xvalue.ext=0xffl; /* sign extend */
                 break;
/*
         case 7: dtoaf(dv,&p->uval.xvalue.ext,
                          &p->uval.xvalue.high,
                          &p->uval.xvlaue.low);
                 break;
*/
         }
	if (p->uval.xvalue.ext != 0L && p->uval.xvalue.ext != 0xFFL)
		p->size = SIZEA;
	else if (p->uval.xvalue.high != 0L &&
		 p->uval.xvalue.high != 0xFFFFFFL)
		p->size = SIZEL;
	else
		p->size = SIZES;
	p->etype = INT;
}


long acctolong(p)
struct evres *p;
{ /* extends 24 bit value in low with partial value from high. */
   long rv;
   rv=p->uval.xvalue.low | (p->uval.xvalue.high<<24);
   return (rv);
}


static
do_negate(p)
struct evres *p;
{  /* negate the value */
   if (p->etype==FPT) p->uval.fvalue = -(p->uval.fvalue);
   else{
      do_onecomp(p);
      p->uval.xvalue.low+=1;
      do_carry(p);
      }
}

static
do_onecomp(p)
struct evres *p;
{  /* bitwise ones complement */
      p->uval.xvalue.ext= (~(p->uval.xvalue.ext))&0xffl;
      p->uval.xvalue.high= (~(p->uval.xvalue.high))&0xffffffl;
      p->uval.xvalue.low= (~(p->uval.xvalue.low))&0xffffffl;
}

static
do_not(p)
struct evres *p;
{  /* logical not operation */
   p->uval.xvalue.low=!(do_test(p));
   p->uval.xvalue.ext = p->uval.xvalue.high =0;
   p->etype=INT;
   p->mspace = NONE;
}

static
do_test(p)
struct evres *p;
{  /* returns -1 negative +1 positive 0 zero */
   int i;
   if (p->etype== FPT){
      i=p->uval.fvalue!=0.0;
      if (i&&p->uval.fvalue<0.0)i= -1;
      }
   else{
      i= ((p->uval.xvalue.ext!=0)||
          (p->uval.xvalue.high!=0)||
          (p->uval.xvalue.low!=0));
      if (i&&(p->uval.xvalue.ext&0x80)) i= -1;
       }
   return (i);
}

static
do_carry(p)
struct evres *p;
{
   /* propagate carry through low -> high -> ext */
   int carry;
   carry=(p->uval.xvalue.low>>24)&0xffl;
   p->uval.xvalue.low&=0xffffffl;
   p->uval.xvalue.high+=carry;
   carry=(p->uval.xvalue.high>>24)&0xffl;
   p->uval.xvalue.high&=0xffffffl;
   p->uval.xvalue.ext+=carry;
   p->uval.xvalue.ext&=0xffl;
}

static
do_compare(left,right,opt)
struct evres *left,*right;
int opt;
{  /* compares left to right returns 0 if equal 1 if left>right -1 if left<right */
   int i,j,signl,signr,neg,result;
   double fdl,fdr;
   double itod();

   i=(left->etype!=FPT);
   j=(right->etype!=FPT);
   signl=(i)?!!(left->uval.xvalue.ext&0x80):left->uval.fvalue<0.0;
   signr=(j)?!!(right->uval.xvalue.ext&0x80):right->uval.fvalue<0.0;
   neg=signl?-1:1;
   if (signl!=signr) result=neg;
	else if( i && j ){ /* both integer */
      if (left->uval.xvalue.ext != right->uval.xvalue.ext){
         result=(left->uval.xvalue.ext > right->uval.xvalue.ext)?neg:-neg;
         }
      else if (left->uval.xvalue.high != right->uval.xvalue.high){
         result=(left->uval.xvalue.high > right->uval.xvalue.high)?neg:-neg;
         }
      else if (left->uval.xvalue.low != right->uval.xvalue.low){
         result=(left->uval.xvalue.low > right->uval.xvalue.low)?neg:-neg;
         }
      else result=0;
      }
	else{ /* at least one is floating point. convert both to float */
      fdr=itod(right);
      fdl=itod(left);
      if (fdl>fdr) result=1;
      else if (fdl<fdr) result= -1;
      else result=0;
      }
   switch (opt){
			case LTHAN:left->uval.xvalue.low=result== -1;break;
			case GTHAN:left->uval.xvalue.low=result==1;break;
			case EQUL:left->uval.xvalue.low=result==0;break;
			case LTEQL:left->uval.xvalue.low=result!=1;break;
			case GTEQL:left->uval.xvalue.low=result!= -1;break;
			case NEQUL:left->uval.xvalue.low=result!=0;break;
      }
	left->etype =INT; /* convert result to integer */
   left->uval.xvalue.ext=left->uval.xvalue.high=0;
	left->mspace = NONE;
   left->size=3;
}

#ifdef BIT48
static
do_normalize(left,right)
struct evres *left,*right;
{
/* normalizes fractional quantities by shifting left */
/* Otherwise it just changes the size field to match */
if (left->size!=right->size){
   if ((left->etype==FRC)&&(left->size==3)){
      left->uval.xvalue.high=left->uval.xvalue.low;
      left->uval.xvalue.low=0l;
      }
   else if((right->etype==FRC)&&(right->size==3)){
      right->uval.xvalue.high=right->uval.xvalue.low;
      right->uval.xvalue.low=0l;
      }
   if (right->size>left->size) left->size=right->size;
   else right->size=left->size;
   }
}
#endif

#ifdef BIT48
static
comp_big(ext,high,low) 
long *ext,*high,*low;
{  /* bitwise ones complement */
      *ext= (~(*ext))&0xffl;
      *high= (~(*high))&0xffffffl;
      *low= (~(*low))&0xffffffl;
}
#endif

#ifdef BIT48
static
negate_big(ext,high,low) 
long *ext,*high,*low;
{  /* negate the value */
      comp_big(ext,high,low);
      low+=1;
      carry_big(ext,high,low);
}
#endif

#ifdef BIT48
static
carry_big(ext,high,low) 
long *ext,*high,*low;
{
   /* propagate carry through low -> high -> ext */
   int carry;
   carry=(*low>>24)&0xffl;
   *low&=0xffffffl;
   *high+=carry;
   carry=(*high>>24)&0xffl;
   *high&=0xffffffl;
   *ext+=carry;
   *ext&=0xffl;
}
#endif

dvbig(divdext,divdhigh,divdlow,divrext,divrhigh,divrlow,
            quoext,quohigh,quolow,remext,remhigh,remlow)
long divdext,divdhigh,divdlow,divrext,divrhigh,divrlow,
            *quoext,*quohigh,*quolow,*remext,*remhigh,*remlow;
{
   /* unsigned divide or modulo */
   int at_q,divrlen;
   int temp[8],mult[8],q[8],divd[8],divr[8];
   /* first test for divisor greater than dividend */
   if (ucomp_big(divrext,divrhigh,divrlow,divdext,divdhigh,divdlow)>0){
      *remext=divdext; /* if so remainder = dividend  and quotient=0 */
      *remhigh=divdhigh;
      *remlow=divdlow;
      *quoext= *quohigh= *quolow=0l;
      }
   else{
      sbig(0l,0l,0l,q);
      sbig(0l,0l,0l,temp);
      sbig(0l,0l,0l,mult);
      sbig(divdext,divdhigh,divdlow,divd);
      sbig(divrext,divrhigh,divrlow,divr);
      divrlen=len_big(divr);
      for (at_q=len_big(divd)-2;at_q>=0;at_q--){
         block_copy(temp,temp+1,divrlen-1);
         temp[0]=divd[at_q];
         q[at_q]= goes_into(divr,temp,divrlen,mult);
         }
      mbig(q,quoext,quohigh,quolow); /* quotient */
      mbig(temp,remext,remhigh,remlow); /* remainder */
      
      }
}

static
ucomp_big(lext,lhigh,llow,rext,rhigh,rlow)
long lext,lhigh,llow,rext,rhigh,rlow;
{
   /* compares left to right returns 0 if equal 1 if left>right -1 if left<right */
   int result,neg;
   neg=1;
      if (lext != rext){
         result=(lext > rext)?neg:-neg;
         }
      else if (lhigh != rhigh){
         result=(lhigh > rhigh)?neg:-neg;
         }
      else if (llow != rlow){
         result=(llow > rlow)?neg:-neg;
         }
      else result=0;
      return (result);
}

static
sbig(ext,high,low,rv)
long ext,high,low;
int *rv;
{
   rv[7]=0;
   rv[6]=ext;
   rv[5]=high>>16;
   rv[4]=(high>>8)&0xff;
   rv[3]=high&0xff;
   rv[2]=(low>>16);
   rv[1]=(low>>8)&0xff;
   rv[0]=low&0xff;
}

static
mbig(from,ext,high,low)
long *ext,*high,*low;
int *from;
{
   *ext=from[6];
   *high=from[5];
   *high<<=8;
   *high|=from[4];
   *high<<=8;
   *high|=from[3];
   *low=from[2];
   *low<<=8;
   *low|=from[1];
   *low<<=8;
   *low|=from[0];
}

static
block_copy(from,to,size)
int *from,*to,size;
{
   while (--size>=0){
      to[size]=from[size];
      }
}

static
goes_into(what_goes,into_what,len,mult)
int *what_goes,*into_what,*mult,len;
{
   int times,i;
   unsigned int wordtemp;

   i=len-1;
   while (what_goes[i]==0){
      i--;
      if (i<0) return (0); /* division by 0 */
      }
   wordtemp=into_what[i];
   times=  wordtemp/what_goes[i];
   do{
      simple_mult(times,what_goes,mult,len);
      times--;
      }
   while(!try_subtract(mult,into_what,len));

   do{
      times++;
      simple_mult(times,what_goes,mult,len);
      }
   while(try_subtract(mult,into_what,len));
   times--;
   simple_mult(times,what_goes,mult,len);
   really_subtract(mult,into_what,len);
   return(times);
}

static
simple_mult(times,what,res,len)
int times,*what,*res,len;
{
   unsigned int acc,i;
   acc=0;
   for (i=0;i<len;i++){
      acc+=times*what[i];
      res[i]=acc&0xff;
      acc>>=8;
      }
}

static
try_subtract(into,from,len)
int *from,*into;
int len;
{
   int i,acc;
   acc=0;
   for (i=0;i<len;i++){
      acc=from[i]-into[i]-acc;
      acc=(acc<0); /* borrow generated? */
      }
   return (acc==0); /* returns 1 if result of subtract is positive */
}

static
really_subtract(into,from,len)
int *from,*into;
int len;
{
   int i,acc;
   acc=0;
   for (i=0;i<len;i++){
      acc=from[i]-into[i]-acc;
      from[i]=acc&0xff;
      acc=(acc<0); /* borrow generated? */
      }
}

static
len_big(bignum)
int *bignum;
{  /* returns length of integer in bytes */
   int i;
   for (i=6;(i>=0) && (bignum[i]==0);i--);
   return (i+2);
}

double litod(value)	/* long integer to double */
long value;
{
	double fvalue;
	long neg;

	neg = value & WBITMASK;
	if (neg)
		value = ((~value) & WORDMASK) + 1L;
	fvalue = value;	
	return (neg ? -fvalue : fvalue);
}

#ifdef BIT48
long dtoli(value)	/* double to long integer */
double value;
{
	long ivalue = value;

	return (ivalue & WORDMASK);
}
#endif

double vitod(hivalue,lovalue)	/* very long integer to double */
long hivalue, lovalue;
{
	double value;
	int neg = NO;
	long carry;

	if (hivalue == 0L && lovalue == 0L)
		return (0.0);

	if (hivalue & WBITMASK) {
		neg = YES;
		hivalue = (~hivalue) & WORDMASK;
		lovalue = ((~lovalue) & WORDMASK) + 1L;
		carry = (lovalue >> WORDBITS) & BYTEMASK;
		lovalue &= WORDMASK;
		hivalue += carry;
		hivalue &= WORDMASK;
	}

	value = hivalue;
	value *= WBITMASK << 1;
	value += lovalue;
	return (neg ? -value : value);
}

#ifdef BIT48
dtovi(value,hivalue,lovalue)	/* double to very long integer */
double value;
long *hivalue,*lovalue;
{
   long hilong,lolong;
   int complement,carry;
   double lodubl,hidubl;

   if (complement=(value<0.0)){
      value= -value;
      }
   hidubl=value/0x1000000l;
   hilong=((long)hidubl)&0xffffffl; /* upper portion of return value */
   hidubl=hilong*0x1000000l;
   lodubl=value-hidubl;
   lolong=((long)lodubl)&0xffffffl;
   if (complement){
      lolong=~lolong&0xffffffl;
      hilong=~hilong&0xffffffl;
      lolong+=1L;
      carry=(lolong>>WORDBITS)&0xffl;
      hilong+=carry;
      lolong&=0xffffffl;
      hilong&=0xffffffl;
      }
   *hivalue=hilong;
   *lovalue=lolong;
}
#endif

#ifdef BIT48
double lftod(value)	/* long fractional to double */
long value;
{
   static long upb=~0x7fffffl;
   double fvalue;
   value&=0xffffffl;
   if(value&0x800000l) value|=upb; /* sign extend upper bits */
   fvalue=((double)value)/((double)0x800000l);
   return(fvalue);
}
#endif

long dtolf(value)	/* double to long fractional */
double value;
{
   long retv;
   double d,f;
   if (value < -1.0) {
      warn ("Expression value outside fractional domain");
      return (MINFRAC);
   }
   if (value >= 1.0) {
      warn ("Expression value outside fractional domain");
      return (MAXFRAC);
   }
   d = value*0x800000l; /* scale up value */
   if (value<0.0) d+= -0.5;
   else d+= 0.5;  /* round result */
   retv = d;
   f= d - retv; /* fractional remainder of return value */
   if (f==0.0) retv&= ~1l; /* do convergent rounding */
   if (retv>0x7fffffl) retv=0x7fffffl; /* maximum positive value */
   return (retv&0xffffffl);
}

#ifdef BIT48
double vftod(hivalue,lovalue)	/* very long fractional to double */
long hivalue,lovalue;
{/* converts very long data in form of two 24 bit long values to double */
   static long scaleconst=0x800000l;
   double fvalue; /* intermediate return double value */
   int carry;
   int complement;
   if(hivalue&scaleconst){
      lovalue= ((~lovalue)&0xffffffl)+1l;
      carry= !!(lovalue&0x1000000l);/* 1 or 0 for carry */
      lovalue&=0xffffffl;
      hivalue= ((~hivalue)&0x7fffffl)+carry;
      complement=1;
      }
   else{
      hivalue&= 0xffffffl;
      lovalue&= 0xffffffl;
      complement=0;
      }
   fvalue=(((double)hivalue)/((double)scaleconst))+
           (((double)lovalue) / (((double)0x1000000l)*((double)scaleconst)));

   if (complement) fvalue= -fvalue;
   return(fvalue);
}
#endif

dtovf(value,hivalue,lovalue)	/* double to very long fractional */
double value;
long *hivalue,*lovalue;
{  /* returns a very long (two 24 bit words) value representing the double value */
   long retv,lolong;
   int complement,carry;
   double lodubl,hidubl;
   if (value< -1.0){
      warn ("Expression value outside fractional domain");
      *hivalue=0x800000l;
      *lovalue=0l;
      return;
      }
   if (value>=1.0){
      warn ("Expression value outside fractional domain");
      *hivalue=0x7fffffl;
      *lovalue=0xffffffl;
      return;
      }
   if (complement=(value<0)){
      value= -value;
      }
   hidubl=(value*0x800000l); /* scale up value */
   retv=((long)hidubl); /* upper portion of return value */
   lodubl=hidubl-((double)(retv));
   lolong=((long)(0x1000000l*lodubl))&0xffffffl;
   if (complement){
      carry=!lolong;
      lolong=(carry)?0l:(~lolong)&0xffffffl;
      retv=(~retv)+carry;
      }
   *hivalue=retv&0xffffffl;
   *lovalue=lolong;
}

#ifdef BIT48
double aftod(extension,hivalue,lovalue)	/* accum. fractional to double */
unsigned long extension,hivalue,lovalue;
{/* converts very long data in form of two 24 bit long values to double */
   double fvalue; /* intermediate return double value */
   static long scaleconst=0x800000l; /* scaling mode divider */
   int complement;
   int carry;
   /* check sign of accumulator and sign extend if negative */
   if (extension==0x80l && hivalue==0l &&lovalue==0l){
      /* special case won't work with 2' complement */
         fvalue=(double)-256.0;
      }
   else{
      complement=extension&0x80;
      hivalue|= (extension<<24);
      if(complement){
         if (lovalue){
            lovalue=((~lovalue)&0xffffffl)+1l;
            carry=0;
            }
         else carry=1;
         hivalue= ((~hivalue)&0xffffffffl)+carry;
         }
      else lovalue&= 0xffffffl;

      fvalue=(((double)hivalue)/((double)scaleconst))+
              (((double)lovalue) / (((double)0x1000000l)*((double)scaleconst)));

      if (complement) fvalue= -fvalue;
      }
   return (fvalue);
}
#endif

#ifdef BIT48
dtofa(value,extension,hivalue,lovalue)	/* double to accum. fractional */
double value;
long *extension,*hivalue,*lovalue;
{  /* returns a very long (two 24 bit words) value representing the double value */
   long retv,lolong;
   int carry,complement;
   double lodubl,hidubl;
   if (value<= -256.0){
      *extension=0x80l;
      *hivalue=0l;
      *lovalue=0l;
      return;
      }
   if (value>=256.0){
      *extension=0x7fl;
      *hivalue=0xffffffl;
      *lovalue=0xffffffl;
      return;
      }
   if (complement=(value<0)){
      value= -value;
      }
   hidubl=value*0x800000l;
   retv=(long)hidubl;
   lodubl=hidubl-((double)(retv));
   lolong=((long)(0x1000000l*lodubl))&0xffffffl;
   if (complement){
      carry=!lolong;
      lolong=(carry)?0l:(~lolong)&0xffffffl;
      retv=(~retv)+carry;
      }
   *hivalue=retv&0xffffffl;
   *lovalue=lolong;
   *extension=(retv>>24)&0xffl;
}
#endif

#ifdef BIT48
actd(ext,val1,val2,ret1,ret2)
long ext,val1,val2,*ret1,*ret2;
{  /* returns decimal integer value of accumulator in two longs */
   /* Accumulator contains valid data in 8 bit ext and lower 24 bits of val1,val2 */
   long hvext,hvhigh,hvlow,lvext,lvhigh,lvlow;
   dvbig(ext,val1,val2,0l,0x3bl,0x9aca00l,&hvext,&hvhigh,&hvlow,&lvext,&lvhigh,&lvlow);
   *ret2=(lvhigh*0x1000000l)+lvlow; /*low order bits */
   *ret1=(hvhigh*0x1000000l)+hvlow;
}
#endif

#ifdef BIT48
vltd(val1,val2,ret1,ret2)
long val1,val2,*ret1,*ret2;
{  /* returns decimal equivalent of value in long register */
   /* Long registers contain valid bits in lower 24 bits of val1 and val2 */
   /* Accumulator contains valid data in 8 bit ext and lower 24 bits of val1,val2 */
   long hvext,hvhigh,hvlow,lvext,lvhigh,lvlow;
   dvbig(0l,val1,val2,0l,0x3bl,0x9aca00l,&hvext,&hvhigh,&hvlow,&lvext,&lvhigh,&lvlow);
   *ret2=(lvhigh*0x1000000l)+lvlow; /*low order bits */
   *ret1=(hvhigh*0x1000000l)+hvlow;
}
#endif

