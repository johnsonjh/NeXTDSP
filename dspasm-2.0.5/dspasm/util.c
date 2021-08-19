#include "dspdef.h"
#include "dspdcl.h"
#if MPW
#include <osutils.h>
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif


/*
 *	alloc --- allocate memory word-aligned
 */
void *
alloc(nbytes)
int nbytes;
{
	char *malloc();

	return((void *)malloc((unsigned)nbytes));
}

/**
*
* name		binsrch - binary search
*
* synopsis	p = binsrch (item, tbl, elements, size, cmp)
*		char *p;	returned pointer to table entry
*		char *tbl;	pointer to table
*		int elements;	number of items in table
*		int size;	size of items in table
*		int (*cmp) ();	pointer to comparison routine
*
* description	Performs a binary search for item in table.
*		Comparison routine must return strcmp-compatible
*		values. Returns pointer to the table entry if found.
*		Returns NULL if not found.
*
**/
void *
binsrch (item, tbl, elements, size, cmp)
char *item, *tbl;
int elements, size;
int (*cmp) ();
{
	char *low, *high, *mid;
	int cond;

	low = tbl;
	high = tbl + ((elements - 1) * size);
	while (low <= high) {
		mid = low + ((high - low) / size >> 1) * size;
		if ((cond = (*cmp) (item, mid)) < 0)
			high = mid - size;
		else if (cond > 0)
			low = mid + size;
		else
			return ((void *)mid);
	}
	return((void *)NULL);
}

/**
*
* name		psort - quick sort the pointer array bounded by l and r
*
* synopsis	psort (l, r)
*		int l, r;		left and right bounds of array
*
* description	Sorts a pointer array using a recursive quick sort
*		algorithm.  The comparison routine pointed to by the
*		global variable Cmp_rtn must return strcmp-compatible
*		values.	 The base of the array is stored in the global
*		variable Sort_tab.
*
**/
psort(l, r)
int l, r;
{
	int i, j, piv;
	char *pivot;

	if (l < r) {
		i = l;
		j = r;
		piv = (l + r) / 2;
		pivot = Sort_tab[piv];	/* select center pivot element */
		do {
			while (i < j && (*Cmp_rtn) (Sort_tab[i], pivot) <= 0)
				i++;
			while (j > i && (*Cmp_rtn) (Sort_tab[j], pivot) >= 0)
				j--;
			if (i < j)	/* out of order */
				pswap (i, j);
		} while (i < j);
		if (piv < i && (*Cmp_rtn) (Sort_tab[i], pivot) > 0)
			i--;
		pswap (i, piv);		/* put pivot in correct position */
		if (i - l < r - i) {	/* sort shortest list first */
			psort (l, i - 1);
			psort (i + 1, r);
		} else {
			psort (i + 1, r);
			psort (l, i - 1);
		}
	}
}

/**
*
* name		pswap - swap pointers pointed to by x and y
*
**/
static
pswap(x,y)
int x,y;
{
	char *tmp;

	tmp = Sort_tab[x];
	Sort_tab[x] = Sort_tab[y];
	Sort_tab[y] = tmp;
}

/**
*
* name		hash --- form hash value for string s
*
* synopsis	hv = hash(s)
*		int hv;		hashed value
*		char *s;	string to be hashed
*
* description	Forms a hash value by summing the characters and
*		forming the remainder modulo the array size.
*
**/
hash(s)
char *s;
{
	int	hashval;

	for( hashval = 0; *s != EOS ; )
		hashval += *s++;
	return(hashval % HASHSIZE );
}

/**
*
* name		basename --- find base part of file name
*
* synopsis	ptr = basename(str)
*		char *ptr;	returned pointer to basename
*		char *str;	pointer to complete file path
*
* description	Returns a pointer to the base portion of the file name
*		parameter.  For example, if str = "/usr/include/foo"
*		then ptr will point to "foo".  Returns NULL if the
*		input string is NULL.
*
**/
char *
basename (str)
char *str;
{
	register char *p;

	if (!str)		/* empty input */
		return (NULL);

	for (p = str + strlen (str); p >= str; --p)
#if MSDOS
		if( *p == '\\' || *p == ':')
#endif
#if VMS
		if( *p == ']' || *p == ':')
#endif
#if UNIX
		if( *p == '/' )
#endif
#if MAC
		if (*p == ':')
#endif
			break;

	return (p < str ? str : ++p);
}

/**
*
* name		spc_char - convert space to a character
*
**/
char
spc_char(space)
int space; /* which space */
{
	switch( space ){
		case XSPACE: return('X');
		case YSPACE: return('Y');
		case LSPACE: return('L');
		case PSPACE: return('P');
		default:     return('N'); /* just in case */
		}
}

/**
*
* name		spc_off - convert space to counter offset
*
**/
spc_off(space)
int space; /* which space */
{
	switch( space ){
		case XSPACE: return(XMEM);
		case YSPACE: return(YMEM);
		case LSPACE: return(LMEM);
		case PSPACE: return(PMEM);
		default:     return(NONE); /* just in case */
		}
}

/**
*
* name		char_spc - convert character to memory space
*
**/
char_spc (c)
char c;
{
	switch (mapdn (c)) {
		case 'x':
			return (XSPACE);
		case 'y':
			return (YSPACE);
		case 'l':
			return (LSPACE);
		case 'p':
			return (PSPACE);
		case 'n':
			return (NONE);
		default:
			return (ERR);
	}
}

/**
*
* name		ctr_off - convert ctr to counter offset
*
**/
ctr_off(ctr)
int ctr; /* which ctr */
{
	switch( ctr ){
		case DEFAULT: return(DCNTR);
		case LOCTR:   return(LCNTR);
		case HICTR:   return(HCNTR);
		default:      return(ERR); /* just in case */
		}
}

/**
*
* name		ctr_char - convert counter to a character
*
**/
char
ctr_char(ctr)
int ctr; /* which space */
{
	switch( ctr ){
		case DEFAULT: return(' ');
		case LOCTR:   return('L');
		case HICTR:   return('H');
		default:      return(' '); /* just in case */
		}
}

/**
*
* name		char_ctr - convert character to counter
*
**/
char_ctr (c)
char c;
{
	switch (mapdn (c)) {
		case 'l':
			return (LOCTR);
		case 'h':
			return (HICTR);
		default:
			return (ERR);
	}
}

/**
*
* name		map_char - convert map to a character
*
**/
char
map_char(map)
int map; /* which map */
{
	switch( map ){
		case NONE: return(' ');
		case IMAP: return('I');
		case EMAP: return('E');
		case BMAP: return('B');
		default:   return(' '); /* just in case */
		}
}

/**
*
* name		char_map - convert character to map
*
**/
char_map (c)
char c;
{
	switch (mapdn (c)) {
		case 'i':
			return (IMAP);
		case 'e':
			return (EMAP);
		case 'b':
			return (BMAP);
		default:
			return (ERR);
	}
}

/**
*
* name		chk_flds - check for extra fields
*
* synopsis	yn = chk_flds(flds)
*		int yn;		YES / NO if error
*		int flds;	number of valid fields in the set Operand,
*				Xmove, Ymove
*
* description	Issues error if fields that are not valid have characters.
*
**/
chk_flds(flds)
int flds;
{
	if( (flds == 0 && (*Operand != EOS || *Xmove != EOS || *Ymove != EOS)) ||
	    (flds == 1 && (*Xmove != EOS || *Ymove != EOS)) ||
	    (flds == 2 && *Ymove != EOS) ){
		error("Extra fields ignored");
		return(NO);
		}
	return(YES);
}

/**
*
* name		get_string --- collect string
*
* synopsis	ep = get_string(start,to)
*		char *ep;		character past end of string
*		char *start;		start of string
*		char *to;		destination of string
*
* description	Puts a null terminated string in the buffer pointed to
*		by to. Removes the string terminators and accounts for
*		quotes as part of the string. Returns a NULL if
*		string is missing. Warnings will be generated if there
*		is no terminating string delimiter.
*
**/
char *
get_string(start,to)
char *start;
char *to;
{
	register delim;

	if( *start != STR_DELIM && *start != XTR_DELIM ) {
		error ("Syntax error - expected quote");
		return(NULL);
	}
	delim = *start++;

	while ( *start != EOS )
		if( *start == delim )
			if( *(start + 1) == delim ){
				*to++ = *start++;
				++start;
				}
			else if( *(start + 1) == STR_CONCAT &&
				 *(start + 2) == STR_CONCAT ){
				/* string concatenation */
				start += 3; /* move past concatenation and end of first string */
				if( *start != STR_DELIM &&
				    *start != XTR_DELIM ){
					error("Missing string after concatenation operator");
					return(NULL);
					}
				delim = *start++;
				}
			else
				break;
		else
			*to++ = *start++;

	*to = EOS;
	if( *start == delim )
		return(++start);
	else{
		error("Missing quote in string");
		return(NULL);
		}
}

/**
* name		get_i_string - get isolated string
*
* synopsis	ep = get_i_string(start,to)
*		char *ep;		character past end of string
*		char *start;		start of string
*		char *to;		destination of string
*
* description	Puts a null terminated string in the buffer pointed
*		to by to. Removes the string terminators and accounts
*		for single quotes as part of the string. Returns a NULL if
*		the string is missing. Warnings will be generated if there
*		is no terminating string delimiter. Any characters after the
*		string will cause an error.
*
**/
char *
get_i_string(start,to)
char *start;
char *to;
{
	char *np;
	char *get_string ();

	if( (np = get_string(start,to)) != NULL ){
		if( *np != EOS ){
			error("Extra characters following string");
			return(NULL);
			}
		else
			return(np);
		}
	else
		return(NULL);
}

/*
*
* name		test_sym - test symbol name
*
* synopsis	yn = test_sym(sp)
*		int yn;		YES if not reserved symbol name
*		char *sp;	ptr to start of symbol name
*
* description	Test to see if symbol name is a reserved symbol name.
*		Returns YES if symbol name is valid (not reserved).
*
**/
test_sym(sp)
char *sp;
{
	char *temp;
#if !PASM
	temp = sp;
	if( get_rnum(&temp) != ERR && !ALPHAN(*temp)){
		error("Reserved name used for symbol name");
		return(NO);
		}
#endif
	return(YES);
}

/*
 *	get_sym --- qualify symbol name. Returns ptr to symbol name.
 */
char *
get_sym()
{
	static char name[MAXSYM+1];
	char *ptr;
	int i = 0;

	if( test_sym(Optr) == NO )
		return(NULL);
	ptr = name;
	if( *Optr == '_' )
		*ptr++ = *Optr++;
	if( !isalpha(*Optr) ){
		error("Symbols must start with alphabetic character");
		return(NULL);
		}

	while( ALPHAN(*Optr) ){
		++i;
		if( i > MAXSYM ){
			error("Symbol name too long");
			return(NULL);
			}
		*ptr++ = *Optr++;
		}
	*ptr = EOS;
	return(name);
}

/**
* name		get_i_sym -- get isolated symbol
*
* synopsis	yn = get_i_sym(start)
*		int yn;		YES if symbol found / NO if none / ERR if error
*		char *start;	start of symbol
*
* description	Looks for an isolated (i.e. null terminated) symbol. Returns
*		YES if a symbol was found, NO if no symbol found. Errors
*		will occur if the symbol does not start with an alpha
*		character or an underscore, if the symbol is too long,
*		or if the symbol does not terminate with a null.
*
**/
get_i_sym(start)
char *start;	/* start of symbol name */
{
	int i = 1;

	if( *start == EOS )
		return(NO);
	if( test_sym(start) == NO )
		return(ERR);
	if( *start == '_' ) /* accept local labels */
		++start;
	if( !isalpha(*start) ){
		error("Symbols must start with alphabetic character");
		return(ERR);
		}
	start++;
	while( ALPHAN(*start) ){
		++i;
		if( i > MAXSYM ){
			error("Symbol name too long");
			return(ERR);
			}
		start++;
		}
	if( *start != EOS ){
		error("Extra characters following symbol name");
		return(ERR);
		}

	return(YES);
}

/*
 *	mapup --- convert a-z to A-Z
 */
char mapup(c)
char c;
{
	return( islower(c) ? toupper(c) : c );
}

/*
 *	mapdn --- convert A-Z to a-z
 */
char mapdn(c)
char c;
{
	return( isupper(c) ? tolower(c) : c );
}

/*
 *	strup --- map all lower case to upper in str
 */
char *
strup (str)
char *str;
{
	register char *p;

	for (p = str; *p; p++)
		*p = mapup (*p);
	return (str);
}

/*
 *	strdn --- map all upper case to lower in str
 */
char *
strdn (str)
char *str;
{
	register char *p;

	for (p = str; *p; p++)
		*p = mapdn (*p);
	return (str);
}

#if MSDOS
/**
*
* name		adjbuf - adjust file buffer
*
* synopsis	adjbuf (fp);
*		FILE *fp;	file pointer
*
* description	Resets the buffer size for the given file.  If space
*		for the new buffer cannot be allocated, or if resetting
*		the buffer size fails, the default buffer is used.
*
**/
adjbuf (fp)
FILE *fp;
{
	char *buf;

	if ((buf = (char *)alloc (BUFLEN)) != NULL)
		setvbuf (fp, buf, _IOFBF, BUFLEN);
}
#endif

/**
*
* name		chklc - check for location counter overflow
*
* synopsis	yn = chklc ();
*		int yn;		YES if overflow, NO otherwise
*
* description	Checks location counter values.	 If overflow detected
*		issues a warning.
*
**/
chklc ()
{
	int rc = NO;

	if (Ro_flag) {
		warn ("Runtime location counter overflow");
		Ro_flag = NO;
		rc = YES;
	} else if ((*Pc &= MAXADDR) < Old_pc)
		Ro_flag = YES;
	if (Pc != Lpc)
		if (Lo_flag) {
			warn ("Load location counter overflow");
			Lo_flag = NO;
			rc = YES;
		} else if ((*Pc &= MAXADDR) < Old_pc)
			Lo_flag = YES;
	return (rc);
}

/**
* name		mem_compat --- check memory space compatibility
*
* synopsis	yn = mem_compat (sp1, sp2)
*		int yn;			YES if compatible, NO otherwise
*		int sp1, sp2;		memory spaces to compare
*
* description	Returns YES if the memory spaces are compatible, otherwise NO.
*
**/
mem_compat (sp1, sp2)
int sp1, sp2;
{
	int rc = YES;

	switch (sp1) {
		case XSPACE:
			if( sp2 == YSPACE || sp2 == PSPACE )
				rc = NO;
			break;
		case YSPACE:
			if( sp2 == XSPACE || sp2 == PSPACE )
				rc = NO;
			break;
		case LSPACE:
			if( sp2 == PSPACE )
				rc = NO;
			break;
		case PSPACE:
			if( sp2 != PSPACE && sp2 != NONE )
				rc = NO;
			break;
		case NONE:
		default:
			rc = YES;
			break;
	}
	return (rc);
}

#if MPW

/* The following routines were adapted from Lightspeed C unixtime.c */

/* This routine returns the time since Jan 1, 1970 00:00:00 GMT */

time_t time(clock)
time_t *clock;
{
	time_t RawTime, GMTtimenow;
	void GetDateTime ();

	GetDateTime (&RawTime);
	GMTtimenow = RawTime-TimeBaseDif+GMTzonedif;

	if (clock)
		*clock = GMTtimenow;

	return (GMTtimenow);
}

struct tm *localtime(clock)
register time_t *clock;
{
DateTimeRec MacTimeRec;
static struct tm UnixTimeRec;
register int dayofyear=0, i;

	time_t RawTime, GMTtimenow;

	if (!clock)
		GetDateTime (&RawTime);

	Secs2Date(clock?(*clock+TimeBaseDif-GMTzonedif):RawTime,&MacTimeRec);

	UnixTimeRec.tm_sec	= (MacTimeRec.second);
	UnixTimeRec.tm_min	= (MacTimeRec.minute);
	UnixTimeRec.tm_hour	= (MacTimeRec.hour);
	UnixTimeRec.tm_mday	= (MacTimeRec.day);
	UnixTimeRec.tm_mon	= (MacTimeRec.month-1); /* UNIX uses 0-11 not 1-12 */
	UnixTimeRec.tm_year	= (MacTimeRec.year-1900);
	UnixTimeRec.tm_wday	= (MacTimeRec.dayOfWeek-1); /* UNIX uses 0-6 not 1-7 */

	for(i=0; i<UnixTimeRec.tm_mon; i++)
	{
	static char monthdays[11] = {31,28,31,30,31,30,31,31,30,31,30};

		/* check for leap year in Feb */
		if ((i==1) && !(MacTimeRec.year % 4) && !(MacTimeRec.year % 100))
			dayofyear++;

		dayofyear += monthdays[i];
	}

	UnixTimeRec.tm_yday = (dayofyear + MacTimeRec.day-1);

	UnixTimeRec.tm_isdst = 0;	/* don't know if daylight savings */

	return(&UnixTimeRec);

}

#endif /* MPW */

#if MPW || AZTEC
getpid ()			/* fake a process ID for MPW and Aztec */
{
	time_t time ();

	return ((unsigned)time(0));
}
#endif /* MPW || AZTEC*/
