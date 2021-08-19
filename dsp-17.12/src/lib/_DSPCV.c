#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"

/*%-**$$$ BagItem $$$**-%*/
/* cvs.c */
/*########################### cvs.c ###########################*/
/* _DSPCVS.c (_DSPCVS, _DSPCVAS, _DSPCVHS) - convert int to string */

/* #include "dsp/_dsputilities.h" */

char *_DSPCVAS(
    int n,			/* number to be converted */
    int fmt)			/* 0=decimal, 1=hex format */
{
    enum fmts {INT,HEX} thefmt;
    char *intstr;
    int ndig,s,t;
    if (n==_DSP_NOT_AN_INT) 
      return("<not_an_int>");	/* Serious since -_DSP_NOT_AN_INT = _DSP_NOT_AN_INT */
    thefmt = (enum fmts) fmt;
    if (n<0) {
	s = -1;
	n = -n;
    } else
      s = 1;
    for (ndig=1, t=10; ndig<11; ndig++, t *= 10)
      if (n<t) break;
    intstr = (char *) malloc(ndig+2);
    if (intstr==NULL) _DSPErr("cvas: insufficient free storage");
    switch(thefmt) {
      case INT:
	sprintf(intstr," %d",n*s);
	break;
      case HEX:
	sprintf(intstr,"%X",n);
	if (s<0) intstr = _DSPCat("-",intstr);
	break;
      default:
	fprintf(stderr,
	   "*** cvas: unrecognized string format code: %d ***\n",fmt);
    }
    intstr[strlen(intstr)] = '\0';
    return(intstr);
}

char *_DSPCVS(int n)
{
    return(_DSPCVAS(n,0));
}


char *_DSPCVHS(int n)
{
    return(_DSPCVAS(n,1));
}


char *_DSPCVDS(float d)
{
    char *dstr;
    dstr = (char *) malloc(200);
    if (dstr==NULL) _DSPErr("cvds: insufficient free storage");
    sprintf(dstr,"%f",d);
    dstr = realloc(dstr,strlen(dstr)+1);
    if (dstr==NULL) _DSPErr("cvds: realloc failed");
    return(dstr);
}


char *_DSPCVFS(float f)
{
    return(_DSPCVDS((double)f));
}


/*%-**$$$ BagItem $$$**-%*/
/* _DSPIntToChar.c */
/*########################### _DSPIntToChar.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPINTTOCHAR  - Convert digit between 0 and 9 to corresponding character */

char *_DSPIntToChar(int i)
{    
    char *c;

    c = _DSPNewStr(2);
    *c++ = (char) (i + '0');
    *c-- = _DSP_NULLC;
    if(i>9 || i<0) *c = '?';
    return(c);
}

/******************** DSPFix{24,48} --> [Time stamp] string ******************/

#define MAXNUMCHARS 30
static char numStr[MAXNUMCHARS];
static char timeStampStr[MAXNUMCHARS*2+20];

char *DSPFix24ToStr(DSPFix24 datum)
{
    int i;
    double fword;

    /* fword = DSPFix24ToFloat(datum); */
    fword = DSP_INT_TO_FLOAT(datum);
    sprintf(&(numStr[0]),"%10.8f",fword);
    return numStr;
}


char *DSPFix48ToSampleStr(DSPFix48 *aTimeStampP)
{
    if (!aTimeStampP) 
      return "<untimed>";
    sprintf(&(numStr[0]),"%.0f",DSP_FIX48_TO_DOUBLE(aTimeStampP));
    return numStr;
}


char *DSPTimeStampStr(DSPFix48 *aTimeStampP)
{
    double ts;

    if (!aTimeStampP)
      sprintf(&(timeStampStr[0])," <untimed>");
    else if ((ts = DSP_FIX48_TO_DOUBLE(aTimeStampP)) == 0.0)
      sprintf(&(timeStampStr[0])," at end of current tick");
    else
      sprintf(&(timeStampStr[0])," at sample %s = %3f sec at 44.1KHz",
	      DSPFix48ToSampleStr(aTimeStampP),ts/44100.0); 

    return timeStampStr;
}


