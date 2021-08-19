#ifdef SHLIB
#include "shlib.h"
#endif

/*
  conversion.c
  Copyright 1988, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: objc.h
*/
/* 
Modification history:

  01/08/90/daj - Added comments.
  07/24/90/daj - Changed to use _MKSprintf and _MKVsprintf for thread-safety
                 in a multi-threaded Music Kit performance.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
*/

#import <ctype.h>
#define MK_INLINE 1
#import "_musickit.h"
#import "_MKSprintf.h"

int
_MKStringToInt(char * s)
{
    /* Convert string to int by scanning for a contained string.
       If none, returns MAXINT. */
    int i;
    char *p = s;
    if (!s)
      return MAXINT;
    i = atoi(s);
    if (i == 0) {   /* Make sure there's really a number. */
	while (*p == ' ' || *p == '\t') p++;
	if (*p == '0') 
	  return 0;
	else return MAXINT;
    }
    return i;
}

double
_MKStringToDouble(char * s)
{
    /* Convert string to double by scanning for number. If none,
       returns MK_NODVAL. */
    double x;
    char *p = s;
    if (!s)
      return MK_NODVAL;
    x = atof(s);
    if (x == 0.0) {   /* Make sure there's really a number. */
	while (*p == ' ' || *p == '\t') p++;
	if ((*p == '0') || ((*p == '.') && isdigit(*++p)))
	  return 0.0;
	else return MK_NODVAL;
    }
    return x;
}

char *
  _MKDoubleToString(double x)
{
    /* Converts double to string. Allocates a new string and writes number into 
       it. */
    char *rtn;
#   define PRECISION 6 
#   define DECIMALPOINT 1
#   define FRACTIONALPART (PRECISION + DECIMALPOINT)
    int len;
    int iPart;
    iPart = (int)x;
    if (iPart > 0)
      len = (int)log10((double)iPart) + 1 + FRACTIONALPART;
    else if (iPart < 0)
      len = (int)log10((double)-iPart) + 2 + FRACTIONALPART; /* minus sign */
    else len = 1 + FRACTIONALPART;  /* i == 0 */
    _MK_MALLOC(rtn,char,len+1);
    _MKSprintf(rtn,"%f",x);
    return rtn;
}

char *
_MKIntToString(int i)
{
    /* Converts int to string. Allocates a new string and writes number into 
       it. */
    char *rtn;
    int len;
    if (i > 0)
      len = (int)log10((double)i) + 1; 
    else if (i < 0)
      len = (int)log10((double)-i) + 2; /* + 1 for minus sign */
    else len = 1;                     /* i == 0 */
    _MK_MALLOC(rtn,char,len+1);
    _MKSprintf(rtn,"%d",i);
    return rtn;
}

#define MAXLEN 80

static char numStr[MAXLEN] = ""; /* Used for returning numbers as strings
					when the string is not copied. */
char *
_MKDoubleToStringNoCopy(double x)
{
    /* Converts double to string. Allocates a new string and writes number 
       into it. Number is assumed to be representable in at most 80
       characters. */
    _MKSprintf(numStr,"%f",x);
    return numStr;
}

char *
_MKIntToStringNoCopy(int i)
{
    /* Converts int to string. Allocates a new string and writes number into 
       it. Number is assumed to be representable in at most 80
       characters. */
    _MKSprintf(numStr,"%d",i);
    return numStr;
}

int _MKFix24ToInt(DSPFix24 ival)
{
    /* Converts Fix24 to int. Included here so that libdsp doesn't always
       have to be loaded. */
    return (ival &  0x800000) ? ival | 0xFF000000 /* sign extend */ : ival;
}

double _MKFix24ToDouble(DSPFix24 ival)
{
    /* Converts Fix24 to double. Included here so that libdsp doesn't always
       have to be loaded. */
    double dval;
    if (ival &  0x800000)
      ival |= 0xFF000000; /* sign extend */
    dval = (double) ival;
    dval *= 0.00000011920928955078125; /* 1/2^23 */
    return dval;
}

DSPFix24 _MKDoubleToFix24(double dval)
{
    /* Converts double to Fix24. Included here so that libdsp doesn't always
       have to be loaded. */
    register int ival;
    if (dval > DSP_F_MAXPOS)  /* cf. dsp.h */
      dval = DSP_F_MAXPOS;
    else if (dval < DSP_F_MAXNEG) 
      dval = DSP_F_MAXNEG;
    ival = DSP_DOUBLE_TO_INT(dval);
    return ival & DSP_WORD_MASK;    /* strip off sign extension, if any */
}



