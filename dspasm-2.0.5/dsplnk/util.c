#include "lnkdef.h"
#include "lnkdcl.h"
#if MPW
#include <osutils.h>
#endif

#if !LINT
static char *SccsID = "%W% %G%";	/* SCCS data */
#endif

 
/*
 *      alloc --- allocate memory word-aligned
 */
double *
alloc(nbytes)
int nbytes;
{
        char *malloc();

#if !LINT
        return((double *)malloc((unsigned)nbytes));
#else
	nbytes = nbytes;
	return((double *)NULL);
#endif
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
double *
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
#if !LINT
                        return ((double *)mid);
#else
                        return ((double *)NULL);
#endif
        }
        return((double *)NULL);
}

/**
*
* name 		psort - quick sort the pointer array bounded by l and r
*
* synopsis	psort (l, r)
*		int l, r;		left and right bounds of array
*
* description	Sorts a pointer array using a recursive quick sort
*		algorithm.  The comparison routine pointed to by the
*		global variable Cmp_rtn must return strcmp-compatible
*		values.  The base of the array is stored in the global
*		variable Sort_tab.
*
**/
/*VARARGS*/
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
				swap (i, j);
		} while (i < j);
		if (piv < i && (*Cmp_rtn) (Sort_tab[i], pivot) > 0)
			i--;
		swap (i, piv);		/* put pivot in correct position */
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
* name 		swap - swap pointers pointed to by x and y
*
**/
static
swap(x,y)
int x,y;
{
	char *tmp;

	tmp = Sort_tab[x];
	Sort_tab[x] = Sort_tab[y];
	Sort_tab[y] = tmp;
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
* name		get_string --- collect string 
*
* synopsis	ep = get_string(start,to)
*		char *ep;		character past end of string
*		char *start;		start of string
*		char *to;		destination of string
*
* description	Puts a null terminated string in the buffer pointed to
*		by to. Removes the string terminators and accounts for
*		single quotes as part of the string. Returns a NULL if
*		string is missing. Warnings will be generated if there
*		is no terminating string delimiter.
*
**/ 
char *
get_string(start,to)
char *start;
char *to;
{
	if( *start != STR_DELIM && *start != XTR_DELIM ) {
		error ("Syntax error in string - expected quote");
		return(NULL);
	}
	start++;

	while ( *start != EOS )
		if( *start == STR_DELIM || *start == XTR_DELIM )
			if( *start == STR_DELIM &&
			    *(start + 1) == STR_DELIM ){
				*to++ = *start++;
				++start;
				}
			else if( *start == XTR_DELIM &&
			    *(start + 1) == XTR_DELIM ){
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
				start++;
				}
			else
				break;
		else
			*to++ = *start++;

	*to = EOS;
	if( *start == STR_DELIM  || *start == XTR_DELIM )
		return(++start);
		
	else{
		error("Missing quote in string");
		return(NULL);
		}
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

	ptr = name;
	if( !isalpha(*Optr) ){
		error("Invalid symbol");
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
* name          get_space - return memory space value
*
* synopsis      space = get_space (c)
*               int space;	memory space value returned
*		char c;		memory space character identifier
*
* description   Perform switch on input character c.  Return corresponding
*		memory space value.  Return -1 on error.
*
**/
get_space (c)
char c;
{
	switch (mapdn (c)) {
		case 'n':
			return (NONE);
		case 'x':
			return (XMEM);
		case 'y':
			return (YMEM);
		case 'l':
			return (LMEM);
		case 'p':
			return (PMEM);
		default:
			return (ERR);
	}
}


/**
* name          get_cntr - return memory counter value
*
* synopsis      cntr = get_cntr (c)
*               int cntr;	memory counter value returned
*		char c;		memory counter character identifier
*
* description   Perform switch on input character c.  Return corresponding
*		memory counter value.  Return -1 on error.
*
**/
get_cntr (c)
char c;
{
	switch (mapdn (c)) {
		case 'l':
			return (LCNTR);
		case 'h':
			return (HCNTR);
		default:
			return (ERR);
	}
}


/*
 *      mapup --- convert a-z to A-Z
 */
char mapup(c)
char c;
{
        return( islower(c) ? toupper(c) : c );
}

/*
 *      mapdn --- convert A-Z to a-z
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
	UnixTimeRec.tm_mon	= (MacTimeRec.month-1);	/* UNIX uses 0-11 not 1-12 */
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
