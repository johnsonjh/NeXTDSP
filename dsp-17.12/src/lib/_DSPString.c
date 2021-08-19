#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

/*%-**$$$ BagItem $$$**-%*/
/* _DSPNewStr.c */
/*########################### _DSPNewStr.c ###########################*/
/* #include "dsp/_dsp.h" */

/* _DSPNEWSTR */

char *_DSPNewStr(int size)
{
    char *cp;

    cp = malloc(size);
    if (cp==NULL) _DSPErr("_DSPNewStr: insufficient free storage");
    *cp = _DSP_NULLC;
    return(cp);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPMakeStr.c */
/*########################### _DSPMakeStr.c ###########################*/
/* _DSPMAKESTR */

/* #include "dsp/_dsp.h" */

char *_DSPMakeStr(
    int size,			/* size = total length incl \0 */
    char *init)			/* initialization string */
{
    char *cp,*bp;

    cp = malloc(size);
    if (cp==NULL) _DSPErr("_DSPMakeStr: insufficient free storage");
    if (init)
      strncpy(cp,init,size-1);
    else
      for (bp=cp;bp<cp+size-1;bp++)
	*bp = ' ';
    *(cp+size-1) = '\0';
    return(cp);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPCat.c */
/*########################### _DSPCat.c ###########################*/
/* #include "dsp/_dsp.h" */

/* _DSPCat */

char *_DSPCat(
    char *f1,
    char *f2)
{   
    char *f12,*cp;

    if ( f1 == NULL ) return(_DSPCat(f2,""));
    if ( f2 == NULL ) return(_DSPCat(f1,"")); /* So you can say _DSPCat(str,0) */
    cp = f12 = (char *) malloc( strlen(f1) + strlen(f2) + 1 );
    if (cp==NULL) _DSPErr("_DSPCat: insufficient free storage");
    while (*cp = *f1++) cp++;
    while (*cp++ = *f2++);	  /* string copy */
    return(f12);
}

/*%-**$$$ BagItem $$$**-%*/
/* DSPCat.c */
/*########################### DSPCat.c ###########################*/
/* DSPCat */

/* concatenate two strings */
char *DSPCat(char *f1, char *f2)
{
    char *f12,*cp;
    return _DSPCat(f1,f2);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPReCat.c */
/*########################### _DSPReCat.c ###########################*/
/* #include "dsp/_dsp.h" */

/* *** WARNING *** _DSPReCat cannot be used with a first argument
	which was not created using malloc. literal strings, e.g. "foo", are
	kept in the text segment instead of the data segment and are
	not writable. The first argument must have been malloc'd !
*/

char *_DSPReCat(
    char *f1,
    char *f2)
{
    char *f12,*cp;
    int len1;

    if ( f2 == NULL ) return(f1);
    if ( f1 == NULL ) return(_DSPCat(f2,"")); /* poor form of call */
    f12 = (char *) realloc(f1, (len1=strlen(f1)) + strlen(f2) + 1 );
    if (f12==NULL) _DSPErr("_DSPReCat: insufficient free storage");
    for (cp = f12+len1; *cp++ = *f2++;); /* copy string 2 to end of string 1 */
    return(f12);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPCopyStr.c */
/*########################### _DSPCopyStr.c ###########################*/

/* _DSPCopyStr */

/* #include "dsp/_dsp.h" */

char *_DSPCopyStr(char *s)
{
    char *c;
    c = malloc(strlen(s)+1);
    if (c==NULL) _DSPErr("_DSPCopyStr: insufficient free storage");
    strcpy(c,s);
    return c;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPToLowerStr.c */
/*########################### _DSPToLowerStr.c ###########################*/
/* _DSPTOLOWERSTR */

/* # include "dsp/_dsp.h" */

char *_DSPToLowerStr(char *s)	/* input string = output string */
{
    char *t=s;
    while(*t) { if(isupper(*t)) *t=tolower(*t); t++; }
    return s;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPToUpperStr.c */
/*########################### _DSPToUpperStr.c ###########################*/
/* _DSPTOUPPERSTR */

/* # include "dsp/_dsp.h" */

char *_DSPToUpperStr(
    char *s)			/* input string = output string */
{
    char *t=s;
    if (!s) return s;
    while(*t) { if(islower(*t)) *t=toupper(*t); t++; }
    return s;
}

