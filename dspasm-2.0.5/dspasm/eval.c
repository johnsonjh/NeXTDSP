#include "dspdef.h"
#include "dspdcl.h"
#if !LINT
#include <math.h>
#endif
#include <errno.h>
#if (UNIX || VMS) && !NeXT /* Unix | VAX/VMS C compiler */
#ifdef HUGE_VAL
#undef HUGE_VAL
#endif
#if !LINT
#define HUGE_VAL HUGE
#else
#define HUGE_VAL 0.0
#endif
#endif
#if LSC || MPW
#ifdef HUGE_VAL
#undef HUGE_VAL
#endif
#define HUGE_VAL 99.e999
#endif
#if MPW
#ifdef log10
#undef log10
#endif
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
* description	eval_ad() will allow an expression to be preceded by an
*		optional '>' or '<' to force long or short addressing. The
*		expression result must be an absolute or relocatable integer
*		within the range $0 - $FFFF or an error will result.
*
*		er->force is only set when the expression is preceded
*		by a '<','<<',or '>'.
*			<  = FORCES (force short addressing)
*			<< = FORCEI (force IO short addressing)
*			>  = FORCEL (force long addressing)
*
*		er->fwd_ref will be set non-zero if the expression contains
*		forward references.
*
**/
struct evres *
eval_ad()
{
	struct evres *i,*eval();
	int ftype = NONE;
	long val, acctolong ();

	if(*Optr=='<'){
		ftype = FORCES;
		++Optr;
		if( *Optr == '<' ){	/* check for */
			ftype = FORCEI; /* IO short address force */
			++Optr;
			}
		}
	else if(*Optr=='>'){
		ftype = FORCEL;
		++Optr;
		}
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
		if( ftype == FORCEI && i->secno < 0 )	/* ext. reference */
			i->uval.xvalue.low = IOSTART;
		i->force = ftype; /* save force type */
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
*
*		er->fwd_ref will be set non-zero if the expression contains
*		forward references.
*
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
*
* name		eval_byte - evaluate an 8-bit quantity between $0 - $FF
*
* synopsis	er = eval_byte()
*		struct evres *er;	expression result
*
* description	eval_byte() will allow only absolute 8-bit expressions. The
*		result must be within the range $0-$FF.
*
*		er->fwd_ref will be set non-zero if the expression contains
*		forward references.
*
**/
struct evres *
eval_byte()
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
		if( val < -128L || val > 255L ){
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
* name		eval_word - evaluate a word between $0 - $FFFFFF
*
* synopsis	er = eval_word()
*		struct evres *er;	expression result
*
* description	eval_word() will allow an expression to be preceded by an
*		optional '>' or '<' to force long or short words. The
*		expression result must be an absolute or relocatable integer
*		within the range $0 - $FFFFFF OR a floating point number.
*		A floating point number is allowed for later conversion
*		to a fractional value.
*
*		er->force is only set when the expression is preceded
*		by a '<','<<',or '>'.
*			<  = FORCES (force short addressing)
*			<< = FORCEI (force IO short addressing)
*			>  = FORCEL (force long addressing)
*
*		er->fwd_ref will be set non-zero if the expression contains
*		forward references.
*
**/
struct evres *
eval_word()
{
	struct evres *i,*eval();
	int ftype = NONE;
	long val, acctolong ();

	if(*Optr=='<'){
		ftype = FORCES;
		++Optr;
		}
	else if(*Optr=='>'){
		ftype = FORCEL;
		Optr++;
		}

	i = eval();

	if( i != NULL ){
		if( i->etype == INT ){
			val = acctolong (i);
			if( val < -8388608L || val > 16777215L ){
				free_exp(i);
				error("Expression result too large");
				return(NULL);
				}
			i->uval.xvalue.ext = i->uval.xvalue.high = 0L;
			if( ftype == FORCEI && i->secno < 0 ) /* extern ref */
				i->uval.xvalue.low = IOSTART;
			else
				i->uval.xvalue.low = val;
			}
		i->force = ftype; /* save force type */
		}
	return(i);
}

/**
*
* name		eval_string - evaluate a character string
*
* synopsis	er = eval_string()
*		struct evres *er;	expression result
*
* description	eval_string evaluates the character string returned by
*		get_string. The characters in the string are counted and
*		word space is dynamically allocated for the character values.
*
*		For single character strings, the function puts the value
*		of the character into the low byte of a 24-bit word.
*		For multiple character strings, eval_string puts character
*		triples into whole words; if characters remain, they are
*		left aligned in the final word of the resulting character
*		array.	A pointer to the array and a count of words is
*		returned in the evres structure.  It is the caller's
*		responsibility to free the space allocated for the word array.
*
**/
struct evres *
eval_string()
{
	struct evres *es;
	register char *p;
	int count, mem;
	long *word;
	char *get_string ();
	union wb {
		char byte[sizeof (long)];
		long word;
	} *wbyte;

/*
	get the string and count the characters in it
*/

	if ((Optr = get_string (Optr, String)) == NULL)
		return (NULL);
	count = strlen (String);

/*
	allocate memory for evaluation structure and word array
*/

	if ((es = (struct evres *)alloc (sizeof (struct evres))) == NULL)
		fatal ("Out of memory - cannot evaluate string");
	mem = Pack_flag ? (count ? ((count - 1) / BYTEWORD) + 1 : 1) :
			  (count ? count : 1);
	if ((word = (long *)alloc (mem * sizeof (long))) == NULL)
		fatal ("Out of memory - cannot evaluate string");
	es->etype = INT;
	es->bcount = count;
	es->wcount = mem;
	es->uval.wvalue = word;
	es->force = es->fwd_ref = es->mspace = NONE;

	if (count <= 1) {	/* empty or single-character string */
		*word = count ? (long)String[0] : (long)0;
		return (es);
	}

	if (!Pack_flag) {	/* don't pack characters */
		for (p = String; *p; p++)
			*word++ = *p;
		return (es);
	}

/*
	initialize word union; loop to pick up characters in string
*/

	wbyte = (union wb *)word;
	wbyte->word = 0L;
#if M68010 || M68020 || LINT
	for (p = String, count = 1; *p; p++, count++) {
		if (count > BYTEWORD) {
			(++wbyte)->word = 0L;
			count = 1;
		}
		wbyte->byte[count] = *p;
	}
#endif
#if I8086 || VAX
	for (p = String, count = BYTEWORD - 1; *p; p++, count--) {
		if (count < 0) {
			(++wbyte)->word = 0L;
			count = BYTEWORD - 1;
		}
		wbyte->byte[count] = *p;
	}
#endif
	return (es);
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
*		relocatable) expressions.
*
*      an expression is constructed like this:
*
*      expr ::=	 term	     |
*		 expr + term |
*		 expr - term |
*		 expr * term |
*		 expr / term |
*		 expr % term |	MOD
*		 expr | term |	Bitwise OR
*		 expr & term |	Bitwise AND
*		 expr ^ term |	Bitwise EXOR
*		 expr && term | Logical AND
*		 expr || term | Logical OR
*		 expr < term |
*		 expr > term |
*		 expr == term |
*		 expr <= term |
*		 expr >= term |
*		 expr != term |
*
*      term ::=	 symbol |
*		 * |		 (* = current pc)
*		 function |
*		 constant |
*		 (expr)
*
*	terms can be preceded by any number of unary operators:
*		+	unary plus
*		-	unary minus
*		~	unary binary negate (one's complement)
*		!	unary logical negate
*
*      symbol ::= {valid assembler symbol}
*
*      function ::= @ {valid function name}
*
*      constant ::= hex constant |
*		    binary constant |
*		    decimal constant |
*		    decimal floating point constant |
*		    ascii constant;
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
*	Operators of equal precedence are evaluated left to right.
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
	Eptr = Expr;			/* initialize expression pointer */
	*Eptr++ = '{';			/* bracket user expression */
	if( (i = get_exp()) == NULL ){	/* there was an error */
		Expr[0] = EOS;
		Eptr = NULL;
		return(NULL);
		}
	*Eptr++ = '}';			/* bracket user expression */
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
	if( *Optr != EOS && *Optr != ',' ){
		error("Extra characters beyond expression");
		free_exp(i);
		Expr[0] = EOS;
		Eptr = NULL;
		return(NULL);
		}

	/* discard value if external reference or forward ref on pass 1 */
	if( i->secno < 0 || (i->fwd_ref && Pass == 1) )
		switch (i->etype){
			case INT:
				i->uval.xvalue.ext  =
				i->uval.xvalue.high =
				i->uval.xvalue.low  = 0L;
			case FPT:
				i->uval.fvalue = 0.0;
			}

	*Eptr = EOS;		/* terminate expression string */
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
		free((char *)ep);	/* free the evaluation result */
}

/**
*
* name		get_exp --- start expression evaluation
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
	struct evres *left;	/* left term for expression */
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
	left = do_ops(left,99); /* evaluate with lowest precedence */

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
	int	opt;		/* operator */
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
		    opt == LOGAND || opt == LOGOR ){
			*Eptr++ = *Optr++;	/* save operator */
			*Eptr++ = *Optr++;
			}
		else
			*Eptr++ = *Optr++;

		/* logical op short-circuit code; commented out for now */
#ifdef SHORT_CIRCUIT
		if (opt == LOGAND || opt == LOGOR){
			left->mspace = NONE;
			if (left->etype != INT){
				left->etype = INT;
				left->uval.xvalue.low = left->uval.fvalue;
				}
			if ( (opt == LOGAND && !left->uval.xvalue.low) ||
			     (opt == LOGOR  &&	left->uval.xvalue.low) )
				return (left);
			}
#endif

		if( (right = get_term()) == NULL ){ /* pick up right side */
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
		if( !Cflag && opt != ADD && opt != SUB &&
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
	char	*sym;
	char	*tmp, *tmp2;
	char	*get_sym();
	char	*get_string();
	int	i;
	double optof();
	struct nlist *sp,*lookup();
	struct evres *p,*get_func();

#if DEBUG
	(void)printf("entering get_term\n");
#endif
#ifdef UNARY_PLUS	/* can't do this because of string concatenation */
	if( *Optr == '+' ){
		*Eptr++ = *Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		return(p);
		}
#endif
	if( *Optr == '-' ){
		*Eptr++ = *Optr++;
		if( (p = get_term()) == NULL )
			return(NULL); /* error occurred */
		do_negate (p);
		return(p);
		}

	if( *Optr == '~' ){
		*Eptr++ = *Optr++;
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
		*Eptr++ = *Optr++;
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
		*Eptr++ = *Optr++;
		/* get the expression in the parenthesis */
		if( (p = get_exp()) == NULL ) /* stop for errors */
			return(NULL);
		if( *Optr != ')' ){
			free_exp(p);
			error("Missing ')' in expression");
			return(NULL);
			}
		*Eptr++ = *Optr++;	/* skip right parenthesis */
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
			*Eptr++ = *Optr++;
		while( *Optr == '0' || *Optr == '1' ){
			++i;
			p->uval.xvalue.low = (p->uval.xvalue.low << 1) +
					     ((*Optr)-'0');
			p->uval.xvalue.high <<= 1;
			p->uval.xvalue.ext  <<= 1;
			do_carry (p);
			*Eptr++ = *Optr++;
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
	    (Radix == 16 && isxdigit(*Optr)) ){ /* hex constant */
		if (*Optr == '$' || (*Optr == '0' && *(Optr+1) != EOS))
			*Eptr++ = *Optr++;
		if (mapdn(*Optr) == 'x')
			*Eptr++ = *Optr++;
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
			*Eptr++ = *Optr++;
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
			*Eptr++ = *Optr++;
		tmp = Optr; /* remember start of number */
		tmp2 = Eptr;
		while( isdigit(*Optr) ){ /* guess integer */
			++i;
			p->uval.xvalue.low = (p->uval.xvalue.low * 10) + ( (*Optr)-'0');
			p->uval.xvalue.high *= 10;
			p->uval.xvalue.ext  *= 10;
			do_carry (p);
			*Eptr++ = *Optr++;
			}
		if( *Optr == '.' || *Optr == 'e' || *Optr == 'E' ){
			Optr = tmp; /* floating point. Reset pointer */
			Eptr = tmp2;
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
			if (p->uval.xvalue.ext) /* set appropriate size */
				p->size = SIZEA;
			else if (p->uval.xvalue.high)
				p->size = SIZEL;
		return(p);
		}

	if( *Optr == '*' ){ /* current program counter */
		Optr++;
		p->uval.xvalue.low = Old_pc;
		p->mspace = Cspace;
		p->rel = Relmode;
		p->secno = Csect->secno;
		save_term(p);
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
		(void)strncpy(Eptr,String,i);
		Eptr += i;
		tmp = String;
		for(;i>0;--i)
			p->uval.xvalue.low = (p->uval.xvalue.low << 8) +
					     *tmp++;
		if( strlen(String) > 4 )
			warn("String truncated in expression evaluation");
		p->size = SIZEI;
		return(p);
		}

	if(*Optr=='@'){ /* function */
		Optr++;
		if( (p = get_func(p)) == NULL )
			return(NULL);
		save_term(p);
		return(p);
		}

	if( (sym = get_sym()) != NULL ) { /* a symbol */
		if( (sp = lookup(sym,REFTYPE)) != NULL) {
			if( sp->attrib & INT ) { /* integer */
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
			p->secno = sp->sec->secno;	/* save sect. no. */
			if (p->rel && sp->attrib & LOCAL)
				save_term (p);		/* save value */
			else {
				(void)strcpy (Eptr, sym); /* save symbol */
				Eptr += strlen (Eptr);
			}
#if SASM
		} else {		/* no forward refs. in SASM */
			free_exp(p);
			p = NULL;
		}
#else
		} else if(Pass==1) {	/* forward ref here */
			fwdmark();
			p->fwd_ref = YES; /* indicate forward ref */
			(void)strcpy (Eptr, sym); /* save symbol */
			Eptr += strlen (Eptr);
		}

		if(Pass==2) {
			if(Fwd->ref==Lkp_count) {
				p->fwd_ref = YES; /* indicate forward ref */
				fwdnext();
			}
			if (!sp) {	/* symbol not found */
				if (!Lnkmode) { /* absolute mode */
					free_exp(p);
					p = NULL;
				} else {	/* relative mode */
					p->secno = -1;	/* unknown section */
					(void)strcpy (Eptr, sym);
					Eptr += strlen (Eptr);
					instlxrf (sym); /* put in table */
				}
			}
		}
#endif
		return(p);
	}
	free_exp(p);	/* none of the above */
	return(NULL);

}

/**
*
* name		save_term - save term value in expression buffer
*
* synopsis	save_term (p)
*		struct evres *p;	input expression
*
* description	Saves a term value in the expression buffer
*		for later use by the code generation routines.
*
**/
static
save_term (p)
struct evres *p;
{
	char *strup ();

	if (p->rel)
		*Eptr++ = '[';		/* bracket relocatable value */
	if (p->etype == INT) {		/* integer value */
		switch (p->size) {
			case SIZES:
				(void)sprintf (Eptr, "$%06lx",
					p->uval.xvalue.low & WORDMASK);
			case SIZEL:
				(void)sprintf (Eptr, "$%06lx%06lx",
					p->uval.xvalue.high & WORDMASK,
					p->uval.xvalue.low  & WORDMASK);
			default:	/* default to standard long int */
				(void)sprintf (Eptr, "$%08lx",
					p->uval.xvalue.low);
		}
	} else				/* floating point value */
		(void)sprintf (Eptr, "-13.6f", p->uval.fvalue);
	Eptr += strlen (strup (Eptr));	/* readjust expression pointer */
	if (p->rel)
		*Eptr++ = ']';		/* bracket relocatable value */
}

/**
*
* name		get_func - evaluate built-in function
*
* synopsis	f = get_func (p)
*		struct evres *f;	pointer to function result
*		struct evres *p;	input expression
*
* description	Searches for the function string pointed to by Optr;
*		switches to appropriate evaluation logic for function.
*		Returns result of function call if successful, otherwise
*		NULL pointer.
*
**/
static struct evres *
get_func (p)
struct evres *p;
{
	char	fbuf[4];
	int	fnc_cmp();
	int	i = 0;
	struct	fnc *f;
	struct	evres	*do_flkup(),*do_fexp(),*do_fint(),*do_flcv(),
			*do_flst(),*do_fmsp(),*do_fscp(),*do_fcv(),*do_fcvs(),
			*do_fsgn(),*do_fmax(),*do_fmath(),*do_fmath2(),
			*do_frel(),*do_frnd();

	while (ALPHAN (*Optr) && i < sizeof (fbuf) - 1)
		fbuf[i++] = mapdn (*Optr++);
	fbuf[i] = EOS;
#if DEBUG
	(void)fprintf (stderr, "fbuf = %s\n", fbuf);
#endif
	if( *Optr++ != '(' ){
		free_exp(p);
		error("Missing '(' for function");
		return(NULL);
		}

	f = (struct fnc *)binsrch (fbuf,	/* look for function name */
				   (char *)Func,
				   Nfnc,
				   sizeof (struct fnc),
				   fnc_cmp);
	if (!f){
		error2("Invalid function name",fbuf);
		free_exp(p);
		return(NULL);
		}

	switch( f->ftype ){
		case FDEF:
		case FMAC:
			if ((p = do_flkup (p, f->ftype)) == NULL)
				return (NULL);
			break;
		case FEXP:
			if ((p = do_fexp (p)) == NULL)
				return (NULL);
			break;
		case FINT:
			if ((p = do_fint (p)) == NULL)
				return (NULL);
			break;
		case FLCV:
			if ((p = do_flcv (p)) == NULL)
				return (NULL);
			break;
		case FLST:
			if ((p = do_flst (p)) == NULL)
				return (NULL);
			break;
		case FREL:
			if ((p = do_frel (p)) == NULL)
				return (NULL);
			break;
		case FMSP:
			if ((p = do_fmsp (p)) == NULL)
				return (NULL);
			break;
		case FSCP:
			if ((p = do_fscp (p)) == NULL)
				return (NULL);
			break;
		case FCVI:
		case FCVF:
		case FFRC:
		case FUNF:
		case FLFR:
		case FLUN:
			if ((p = do_fcv (p, f->ftype)) == NULL)
				return (NULL);
			break;
		case FCVS:
			if ((p = do_fcvs (p)) == NULL)
				return (NULL);
			break;
		case FSGN:
			if ((p = do_fsgn (p)) == NULL)
				return (NULL);
			break;
		case FMIN:
		case FMAX:
			if ((p = do_fmax (p, f->ftype)) == NULL)
				return (NULL);
			break;
		case FRND:
			if ((p = do_frnd (p)) == NULL)
				return (NULL);
			break;
		case FABS:
		case FACS:
		case FASN:
		case FATN:
		case FCEL:
		case FCOH:
		case FCOS:
		case FFLR:
		case FLOG:
		case FL10:
		case FSIN:
		case FSNH:
		case FSQT:
		case FTAN:
		case FTNH:
		case FXPN:
			if ((p = do_fmath (p, f->rtn)) == NULL)
				return (NULL);
			break;
		case FAT2:
		case FPOW:
			if ((p = do_fmath2 (p, f->rtn)) == NULL)
				return (NULL);
			break;
		}
	if( *Optr++ != ')' ){
		free_exp(p);	/* release structure */
		error("Extra characters in function argument or missing ')' for function");
		return(NULL);
		}
	if( p->secno < 0 ){	/* external reference */
		free_exp(p);	/* release structure */
		error("External reference not allowed in function");
		return(NULL);
		}
	return(p);
}

static
fnc_cmp (s, f)		/* compare string s with function name in f */
char *s;
struct fnc *f;
{
	return (strncmp (s, f->funcname, strlen (f->funcname)));
}

/**
*
* name		do_flkup --- return whether symbol/macro is defined or not
*
* synopsis	ev = do_flkup (p, type)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*		int type;		function type
*
* description	Sets ivalue in p to 1 if symbol/macro has been defined,
*		0 otherwise.  Returns p if OK, NULL on error.
*
**/
static struct evres *
do_flkup (p, type)
struct evres *p;
int type;
{
	char *tmp, *get_sym();
	struct nlist *sp, *lookup();
	struct mac *mac_look();

#if SASM
	error("Symbols/macros not supported in single-line assembler");
	return(NULL);
#else
	if( (tmp = get_sym()) == NULL ){
		free_exp(p);
		return(NULL);
		}
	Gagerrs = YES;
	if (type == FDEF) {
		sp = lookup (tmp, REFTYPE);
		if (Pass == 1) {
			if (sp)
				p->uval.xvalue.low = 1L;
			else			/* forward reference */
				fwdmark ();
		} else {			/* must be Pass 2 */
			if (Fwd->ref == Lkp_count)
				fwdnext ();
			else
				p->uval.xvalue.low = 1L;
		}
	} else {
		if (mac_look(tmp))
			p->uval.xvalue.low = 1L;
	}
	Gagerrs = NO;
	p->etype = INT;
	p->mspace = NONE;
	p->size = SIZES;
	return (p);
#endif /* SASM */
}

/**
*
* name		do_fexp --- check if expression has errors
*
* synopsis	ev = do_fexp (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to 1 if expression in parentheses
*		does not contain errors, 0 otherwise.  Returns p.
*
**/
static struct evres *
do_fexp (p)
struct evres *p;
{
	struct evres *tp, *get_exp ();

	Gagerrs = YES; /* stop errors */
	Gagcnt = 0; /* clear gag error count */
	p->uval.xvalue.low = 0L;
	/* get the expression in parentheses */
	if( (tp = get_exp()) != NULL ){
		if( Gagcnt == 0 )
			p->uval.xvalue.low = 1L;
		}
	Gagerrs = NO; /* release errors */
	free_exp(tp); /* get rid of expression */
	p->etype = INT;
	p->mspace = NONE;
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_fint --- checks if input is integer
*
* synopsis	ev = do_fint (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to 1 if expression is an integer, 0
*		otherwise.  Returns p if OK, NULL on error.
*
**/
static struct evres *
do_fint (p)
struct evres *p;
{
	struct evres *get_exp ();

	/* get the expression in parenthesis */
	free_exp(p); /* throw away old structure */
	if( (p = get_exp()) == NULL )
		return(NULL);
	p->uval.xvalue.low = p->etype == INT ? 1L : 0L;
	p->etype = INT;
	p->mspace = NONE;
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_flcv --- return location counter value
*
* synopsis	ev = do_flcv (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Returns current location counter value:
*		if arg is R returns runtime location counter value; if arg
*		is L returns loadtime location counter value; otherwise
*		error.	Returns p if OK, NULL on error.
*
**/
static struct evres *
do_flcv (p)
struct evres *p;
{
	if( *Optr == 'R' || *Optr == 'r' ){
		p->uval.xvalue.low = Old_pc; /* get counter val */
		p->mspace = Cspace;
		}
	else if( *Optr == 'L' || *Optr == 'l' ){
		p->uval.xvalue.low = Old_lpc; /* get counter value */
		p->mspace = Loadsp;
		}
	else{
		error("Illegal function argument");
		free_exp(p);
		return(NULL);
		}
	p->rel = Relmode;		/* set mode */
	p->etype = INT;			/* set type to integer */
	p->size = SIZES;
	++Optr;				/* adjust op pointer */
	return (p);
}

/**
*
* name		do_flst --- return listing counter value
*
* synopsis	ev = do_flst (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to current listing counter value.
*
**/
static struct evres *
do_flst (p)
struct evres *p;
{
	p->uval.xvalue.low = (long)Lflag;	/* assign counter value */
	p->etype = INT;			/* set type to integer */
	p->mspace = NONE;		/* no memory space attribute */
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_frel --- return assembler operating mode
*
* synopsis	ev = do_frel (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to 1 if assembler in relative mode,
*		0 otherwise.
*
**/
static struct evres *
do_frel (p)
struct evres *p;
{
	p->uval.xvalue.low = Lnkmode ? 1L : 0L; /* set predicate */
	p->etype = INT;			/* set type to integer */
	p->mspace = NONE;		/* no memory space attribute */
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_fmsp --- return memory space of expression
*
* synopsis	ev = do_fmsp (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p depending on memory space of arg.
*		Returns p if OK, NULL on error.
*
**/
static struct evres *
do_fmsp (p)
struct evres *p;
{
	struct evres *get_exp ();

	/* get the expression in parenthesis */
	free_exp(p); /* throw away old structure */
	if( (p = get_exp()) == NULL )
		return(NULL);
	switch( p->mspace ){
		case NONE:
			p->uval.xvalue.low = 0;
			break;
		case XSPACE:
			p->uval.xvalue.low = 1;
			break;
		case YSPACE:
			p->uval.xvalue.low = 2;
			break;
		case LSPACE:
			p->uval.xvalue.low = 3;
			break;
		case PSPACE:
			p->uval.xvalue.low = 4;
			break;
		}
	p->etype = INT;
	p->mspace = NONE;
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_fscp --- return whether strings compare
*
* synopsis	ev = do_fscp (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to 1 if string in first arg of function
*		is a match for second string arg; otherwise returns 0.
*		Returns p if OK, NULL on error (missing arg).
*
**/
static struct evres *
do_fscp (p)
struct evres *p;
{
	char *get_string(),*tmp;

	if( (Optr = get_string(Optr,String)) == NULL ){
		error("Missing string");
		free_exp(p);
		return(NULL);
		}
	if( *Optr++ != ',' ){
		error("Missing comma");
		free_exp(p);
		return(NULL);
		}
	if( (tmp = (char *)alloc(strlen(String)+1)) == NULL )
		fatal("Out of memory - cannot compare strings");
	(void)strcpy(tmp,String);
	if( (Optr = get_string(Optr,String)) == NULL ){
		free(tmp);
		free_exp(p);
		error("Missing string");
		return(NULL);
		}
	p->uval.xvalue.low = (long)STREQ(String,tmp);
	free(tmp);
	p->etype = INT;			/* set type to integer */
	p->mspace = NONE;		/* no memory space attribute */
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_fcv --- return converted values
*
* synopsis	ev = do_fcv (p, type)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*		int type;		function type
*
* description	Performs conversion of int-to-float, float-to-int, and
*		float-to-frac.	Returns value in p uvalue union.
*		Returns p if OK, NULL on error.
*
**/
static struct evres *
do_fcv (p, type)
struct evres *p;
int type;
{
	struct evres *get_exp();
	double itod (), ftod ();

	free_exp(p); /* throw away old structure */
	/* get the expression in the parenthesis */
	if( (p = get_exp()) == NULL ) /* stop for errors */
		return(NULL);
	if (p->rel) {		/* relative expression */
		error ("Relative expression not allowed");
		free_exp (p);
		return (NULL);
	}
	switch (type) {		/* determine conversion type */
		case FCVI:
			if (p->etype == FPT)
				dtoi (p->uval.fvalue, SIZEL, p);
			break;
		case FCVF:
			if (p->etype == INT)
				p->uval.fvalue = itod (p);
			p->etype = FPT;
			p->mspace = NONE;
			break;
		case FFRC:
		case FLFR:
			if (p->etype == INT)
				p->uval.fvalue = itod (p);
			p->etype = FPT;
			p->mspace = NONE;
			dtof (p->uval.fvalue,
			      type == FFRC ? SIZES : SIZEL, p);
			break;
		case FUNF:
		case FLUN:
			if (p->etype == FPT)
				dtoi (p->uval.fvalue,
				      type == FUNF ? SIZES : SIZEL, p);
			p->uval.fvalue = ftod (p);
			p->etype = FPT;
			p->mspace = NONE;
			break;
		default:
			error("Illegal function argument");
			free_exp(p);
			return (NULL);
	}
	return (p);
}

/**
*
* name		do_fcvs --- convert memory attribute of expression
*
* synopsis	ev = do_fcvs (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets the memory space attribute of p to that given by
*		the function argument. Returns p if OK, NULL on error
*		(missing or invalid attribute, bad expression syntax).
*
**/
static struct evres *
do_fcvs (p)
struct evres *p;
{
	int i;
	struct evres *get_exp();

	free_exp(p); /* throw away old structure */
	if ((i = char_spc (*Optr++)) == ERR) {
		error ("Invalid memory space attribute");
		return (NULL);
	}
	if (*Optr++ != ',') {
		error ("Missing comma");
		return (NULL);
	}
	/* get the expression */
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	p->mspace = i; /* set to new memory space */
	return (p);
}

/**
*
* name		do_fsgn --- return sign of argument
*
* synopsis	ev = do_fsgn (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Sets ivalue in p to 1 if expression is positive, -1 if
*		negative, 0 if zero.  Returns p if OK, NULL on error.
*
**/
static struct evres *
do_fsgn (p)
struct evres *p;
{
	struct evres *get_exp();
	double d,itod();

	free_exp(p); /* throw away old structure */
	/* get the expression in the parenthesis */
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	if (p->etype == INT)
		d = itod(p);		/* convert to double */
	else
		d = p->uval.fvalue;
	if (d < 0.0) {
		p->uval.xvalue.low = 1L;
		do_negate (p);
	} else if (d > 0.0)
		p->uval.xvalue.low = 1L;
	else
		p->uval.xvalue.low = 0L;
	p->etype = INT;
	p->mspace = NONE;
	p->size = SIZES;
	return (p);
}

/**
*
* name		do_fmax --- return min/max function value
*
* synopsis	ev = do_fmax (p, type)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*		int type;		function type (min or max)
*
* description	Returns min or max of input expressions based on
*		type.
*
**/
static struct evres *
do_fmax (p, type)
struct evres *p;
int type;
{
	struct evres *get_exp();
	double m,d,itod();

	free_exp(p); /* throw away old structure */
	/* get the expression */
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	if (p->rel) {		/* relative expression */
		error ("Relative expression not allowed");
		free_exp (p);
		return (NULL);
	}
	if( p->etype == INT )			/* convert to double */
		m = itod (p);
	else
		m = p->uval.fvalue;
	while (*Optr == ',') {
		Optr++;
		free_exp (p);			/* free expression */
		if( (p = get_exp()) == NULL ){	/* stop for errors */
			free_exp(p);
			return(NULL);
			}
		if( p->etype == INT )		/* convert to double */
			d = itod (p);
		else
			d = p->uval.fvalue;

		if (type == FMIN) {
			if (d < m)
				m = d;
		} else {
			if (d > m)
				m = d;
		}
	}
	p->uval.fvalue = m;
	p->etype = FPT;
	p->mspace = NONE;
	p->size = SIZEF;
	return (p);
}

/**
*
* name		do_frnd --- return random function value
*
* synopsis	ev = do_frnd (p)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*
* description	Returns a random number in the range 0.0 to 1.0.
*
**/
static struct evres *
do_frnd (p)
struct evres *p;
{
	if (Seed == 0L)
		Seed = (long)getpid ();
	/* compute new random value */
	Seed = (25173L * Seed + 13849L) % (long)(MAXSINT + 1);
	p->uval.fvalue = Seed;			/* get the random number */
	p->uval.fvalue /= (MAXSINT + 1);	/* scale it */
	p->etype = FPT;
	p->mspace = NONE;
	p->size = SIZEF;
	return (p);
}

/**
*
* name		do_fmath --- return math function value
*
* synopsis	ev = do_fmath (p, rtn)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*		double (*rtn) ();	pointer to math routine
*
* description	Calls external math routines to perform common transcendental
*		math functions.	 Returns p if OK, NULL on error (domain or
*		range errors).
*
**/
static struct evres *
do_fmath (p, rtn)
struct evres *p;
double (*rtn) ();
{
	struct evres *get_exp();
	double d, itod ();
	extern errno;

	free_exp(p); /* throw away old structure */
	/* get the expression */
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	if (p->rel) {		/* relative expression */
		error ("Relative expression not allowed");
		free_exp (p);
		return (NULL);
	}
	if( p->etype == INT )	/* convert to double */
		d = itod (p);
	else
		d = p->uval.fvalue;
	errno = 0;		/* clear error */
	p->uval.fvalue = (*rtn) (d);
	if (errno) {	/* error occured */
		if (errno == EDOM)
			error ("Argument outside function domain");
		if (errno == ERANGE)
			error ("Function result out of range");
		free_exp (p);
		return (NULL);
		}
	p->etype = FPT;
	p->mspace = NONE;
	p->size = SIZEF;
	return (p);
}

/**
*
* name		do_fmath2 --- return math function value
*
* synopsis	ev = do_fmath2 (p, rtn)
*		struct evres *ev;	pointer to result
*		struct evres *p;	pointer to input expression
*		double (*rtn) ();	pointer to math routine
*
* description	Calls external math routines to perform common transcendental
*		math functions requiring two double arguments.	Returns p
*		if OK, NULL on error (domain or range errors).
*
**/
static struct evres *
do_fmath2 (p, rtn)
struct evres *p;
double (*rtn) ();
{
	struct evres *get_exp();
	double d,d2,itod();
	extern errno;

	free_exp(p); /* throw away old structure */
	/* get the expression */
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	if (p->rel) {		/* relative expression */
		error ("Relative expression not allowed");
		free_exp (p);
		return (NULL);
	}
	if( p->etype == INT )	/* convert to double */
		d = itod (p);
	else
		d = p->uval.fvalue;
	free_exp (p);		/* free expression */
	if (*Optr++ != ',') {
		error ("Missing comma");
		return (NULL);
	}
	if( (p = get_exp()) == NULL ){ /* stop for errors */
		free_exp(p);
		return(NULL);
		}
	if( p->etype == INT )	/* convert to double */
		d2 = itod (p);
	else
		d2 = p->uval.fvalue;
	errno = 0;		/* clear error */
	p->uval.fvalue = (*rtn) (d, d2);
	if (errno) {	/* error occured */
		if (errno == EDOM)
			error ("Argument outside function domain");
		if (errno == ERANGE)
			error ("Function result out of range");
		free_exp (p);
		return (NULL);
		}
	p->etype = FPT;
	p->mspace = NONE;
	p->size = SIZEF;
	return (p);
}

#if VAXC	/* another VAX kludge; floor() doesn't work with G_FLOAT */
double
floor (d)	/* return largest integer less than or equal to d */
double d;
{
	double i, f, modf();

	f = modf (d, &i);
	if (i < 0.0 && f != 0.0)
		i -= 1.0;
	return (i);
}
#endif

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
		if (!adj_rel (left, right))
			return (NULL);
		left->uval.xvalue.ext  += right->uval.xvalue.ext;
		left->uval.xvalue.high += right->uval.xvalue.high;
		left->uval.xvalue.low  += right->uval.xvalue.low;
		do_carry (left);
		}
	else{ /* at least one is floating point */
		if (left->rel || right->rel){
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
		do_negate (right);
		(void)do_add (left, right);
		}
	else{ /* at least one is floating point */
		if (left->rel || right->rel){
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
* synopsis	yn = adj_rel (left, right)
*		int yn;			YES = relative terms have opp. signs
*		struct evres *left;	left part of expression
*		struct evres *right;	right part of expression
*
* description	Examine left and right parts of expression; if one is
*		relative and the other absolute, adjust the resulting
*		term to be relative.  If both are relative, check that
*		each have opposing signs and make the result absolute.
*
**/
static
adj_rel (left, right)
struct evres *left;	/* left part of expression */
struct evres *right;	/* right part of expression */
{
	long lval, rval;

	if (!left->rel && right->rel)
		left->rel = YES;		/* relative term */
	else if (left->rel && right->rel) {	/* both relative */
		lval = acctolong(left);
		rval = acctolong(right);
		if ( (lval < 0L && rval < 0L) ||
		     (lval > 0L && rval > 0L) ) {
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
*		memory space before returning.	Check for divide by zero.
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
* name		optof - convert ASCII operand to float
*
* synopsis	double optof()
*
* description	These functions convert a character string pointed to by Optr
*		to a double-precision floating-point number.  The first
*		unrecognized character ends the conversion.  Atof recognizes
*		a string of digits optionally containing a decimal point,
*		then an optional e or E followed by an optionally signed
*		integer.  Originally adapted from the Lattice C library.
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
		*Eptr++ = digit;
		digit = *++Optr;
	}

	/* Optr may be pointing to '.' */
	if (*Optr == '.') {
		*Eptr++ = digit;
		digit = *++Optr;
		while ('0'<=digit && digit<='9') {
			fracmul = fracmul*0.1;
			b = b + (digit-'0')*fracmul;
			*Eptr++ = digit;
			digit = *++Optr;
		}
	}

	/* Optr may be pointing to 'e' or 'E' */
	if (*Optr == 'e' || *Optr == 'E') {
		*Eptr++ = *Optr++;
		/* check sign,if any */
		if (*Optr=='+')
			*Eptr++ = *Optr++;
		else if (*Optr=='-') {
			pminus = YES;
			*Eptr++ = *Optr++;
		}

	/* now, Optr may be pointing to a power digit */
		Optr--;
		while ((digit = *++Optr) == '0'); /* skip leading zeros */
		if ('0'<=digit && digit<='9') {
			pflag = YES; /* there is an exponent number */
			pwr = digit - '0';
			*Eptr++ = digit;
			digit = *++Optr;
			while ('0'<=digit && digit<='9') {
				pwr = pwr*10 + (digit - '0');
				*Eptr++ = digit;
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
	double i, intpart;

	i = x/y;
	i = modf(i, &intpart);

	return( x - intpart*y );

}
#endif

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

long dtoli(value)	/* double to long integer */
double value;
{
	long ivalue = value;

	return (ivalue & WORDMASK);
}

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
   if (retv>(long)0x7fffffl) retv=0x7fffffl; /* maximum positive value */
   return (retv&0xffffffl);
}


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
double aftod(extension,hivalue,lovalue) /* accum. fractional to double */
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

#if MPW
extended
log10 (val)	/* special base 10 log for MPW */
double val;
{
	return (log(val)/log(10.0));
}
#endif
