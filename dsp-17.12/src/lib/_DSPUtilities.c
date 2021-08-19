/*	_DSPUtilities.c - Middle-level DSP utilities
	Copyright 1987,1988, by NeXT, Inc.

Miscellaneous DSP utility functions.

Modification history:
	03/08/88/jos - Split _DSPUtilities into DSPObject + %
	04/09/88/jos - DSPStart() no longer clears DSP time because reset_soft
		       is required at the beginning of every user program
		       (to reset HMS properly after HM_GO) and it clears time.
	04/19/88/jos - Added section's load address in DSPDataRecordLoad()
	05/08/88/daj - Added musickit interface
	07/02/88/jos - Added DSP{Send}BLT{Skip}{Timed}
	07/02/88/jos - Changed block alloc to alloca in _DSPCallTimedV()
	07/02/88/jos - Changed DSPPutConstant() to DSPFillMemory().
	07/25/88/jos - Added routines from DSPObject.c.
	07/28/88/jos - Split file up into itself + DSPTransfer.c + DSPMessage.c
		       + DSPControl.c
	07/29/88/jos - Moved remaining contents to _DSPDebug.c
	12/22/88/jos - Inserted contents of _DSPUtilities.c which this replaces
	03/30/89/jos - Fixed warning (added unneeded return to end of getfil)
	04/30/89/jos - Added fix for null string passed to _DSPRemoveHead()
	07/31/89/jos - Created _DSPCV.c and _DSPString.c from routines here
*/

#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"
#include <stdarg.h>

/* Mostly filename stuff */

/* 
  libjos Modification history (before it became _DSPUtilities.c)
  07/02/88/jos - added cvfs, cvds
  08/06/88/jos - _firstreadablefile now returns file if it exists at all.
		 Before it insisted on the extension being one of the list.
  08/08/88/jos - _DSPGetIntStr generalized to hex and octal input ints via strtol.
  01/09/89/jos - changed #include "dsp/_dsputilities.h" to get _dsp.h instead
		 purely for the sake of the new routined _DSPGetMemStr()

*/

static struct strarr { char string[_DSP_MAX_CMD]; } stra[8] = { 0 };

extern int isatty();

static int usererr(code,msg)
    int code;
    char *msg;
{
    fprintf(stderr,"\t %s\n\t (user error code = %d)\n\n",msg,code);
    perror("(Last UNIX-recorded error)");
    if (code<=0) exit(1);
    return code;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetBody.c */
/*########################### _DSPGetBody.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETBODY */

char *_DSPGetBody(fn)
char *fn;
{
    return( _DSPRemoveTail(_DSPRemoveHead(fn)) );
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetField.c */
/*########################### _DSPGetField.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETFIELD */

char _DSPGetField(infile, string, tklist,lstr)
FILE *infile; char *string, *tklist; int lstr; 
{   int nstr; 
    DSP_BOOL leading, comment; 
    char c;
    leading = TRUE;	/* Begin as if in white space between tokens */
    comment = FALSE;
    nstr=1;		/* Size of token so far */
    *string = '\0';
    while( (c=getc(infile)) != EOF){
	if(nstr>=lstr)
	{
	    *string = '\0'; 
	    fprintf(stderr,"_DSPGetField: token '%s' too long\n",string); 
	    return(c);
	}
	if('A' <= c && c <= 'Z') c += 'a' - 'A';
	if(c=='#')comment = TRUE;
	if(comment && c=='\n') comment = FALSE;
	if(comment)
	  continue;
	if(leading && (c==' ' || c=='\n' || c=='\t'))continue;
/*	printf("index(%s,%c)=%d\n",tklist,c,index(tklist,c));	*/
	if (index(tklist,c)) {*string++ = c; nstr++; leading = FALSE; continue;}
	    else if ( !leading )
	    {
		*string = '\0'; 
		/* printf("_DSPGetField: returning /%s/\n",(string-nstr+1)); */
		return(c);
	    }
    }
    return(EOF);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetFile.c */
/*########################### _DSPGetFile.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETFILE.C */

/* get a file for input or output */

DSP_BOOL _DSPGetFile(fpp, mode, name, dname)
    FILE **fpp; char *mode, *name, *dname;
{
    DSP_BOOL exists;
    
    if (name == NULL) name = _DSPNewStr(_DSP_MAX_NAME);
    if (dname == NULL) dname = _DSPNewStr(_DSP_MAX_NAME);
    /* We use while instead of if below because for some reason a null filename
       can be opened for reading as if it exists! */
    while ( name == NULL || *name == _DSP_NULLC )	/* ask for a filename */
    {
	if(*mode == *"r") fprintf(stderr," Input file (%s):\n",dname);
	else fprintf(stderr," Output file (%s):\n",dname);
	
	name = _DSPGetSN(name,_DSP_MAX_NAME); /* This gets will create name if NULL */
	_DSPParseName(name, dname); /* fill in name from default name */
    }
    
    if (*mode == *"a") 
    {
	*fpp = fopen(name,mode);
	return(TRUE);
    }
    
    if ( exists = !( (*fpp = fopen(name,"r")) == NULL) )
    {
	if (*mode == *"r") return(TRUE);
	else if (strncmp(name,"/dev/",5) ==0 )
	{
	    *fpp = fopen(name,mode);
	    return(TRUE);
	}
	else
	{
	    if (*mode != *"w") 
	      _DSPErr("_DSPGetFile: requested mode not r,w, or a");
	    fprintf(stderr,"File %s already exists. Replace?",name);
	    if(_DSPSezYes())
	    {
		*fpp = fopen(name,mode);
		return(TRUE);
	    }
	    else 
	    { 
		strcpy(dname,name);
		*name = _DSP_NULLC; 
		return( _DSPGetFile(fpp,mode,name,dname) ); 
	    }
	}
    }
    else	/* file not there */
      if (*mode == *"w")
      {
	  *fpp = fopen(name, mode);
	  return(TRUE);
      }
      else
      {
	  if (*mode != *"r") _DSPErr("_DSPGetFile: file mode not r,w, or a");
	  fprintf(stderr,"File %s not found.\n", name);
	  strcpy(dname, name);
	  *name = _DSP_NULLC;
	  return( _DSPGetFile(fpp,mode,name,dname) );
      }
    return(TRUE);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetFloatStr.c */
/*########################### _DSPGetFloatStr.c ###########################*/
/* _DSPGETFLOATSTR */

/* #include "dsp/_dsputilities.h" */

float _DSPGetFloatStr(s)		/* get float from string */
    char **s;			/* input string is chopped to float++ */
{
    float f;
    char *p,*t;
    t = *s;
    while (!isdigit(*t)&&*t!='-'&&*t) t++; /* skip to beginning of float */
    if (!*t) {
	*s=t;			/* no token */
	return _DSP_NOT_AN_INT; /* no token */
    }
    p=t;
    while (isdigit(*p)||*p=='e'||*p=='E'||*p=='.'||*p=='-') p++;
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    sscanf(t,"%f",&f);
    return f;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetFilter.c */
/*########################### _DSPGetFilter.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGetFilter.C */

/* Get a filter file */

# define NULLSTATEMENT printf("")	/* This should not be necessary */
# define ABORT(s) {fprintf(stderr,s); return(FALSE);} NULLSTATEMENT
# define COMPLAIN(s) {fprintf(stderr,s); return(TRUE);} NULLSTATEMENT

# define NSTR 100
# define NCMDS 5	/* Number of commands available */

static char cmd[NSTR], ctkl[27];
static char val[NSTR], vtkl[40];

# define SETINT(name) \
    {	if(_DSPGetField(Ffile, val, vtkl,NSTR)==EOF) \
	    COMPLAIN(("Dangling parameter (no value): %s\n",cmd)); \
	*name = atoi(val); \
    } NULLSTATEMENT

static int sp_fillc(c,ncP,Ffile)
    float *c;
    int *ncP;
    FILE *Ffile;
{
    int i;
    for (i = 0; i < *ncP; i++)
    {
	if(_DSPGetField(Ffile, &(val[0]), vtkl, (int) NSTR)==EOF) {
	    fprintf(stderr,
		    "_DSPGetFilter: coefficient array truncated to %d\n",i);
	    return(TRUE);
	}
	sscanf(&(val[0]),"%f",&(c[i]));
    }
    return(0);
}

/* ========================================================= */
int _DSPGetFilter(name,dname,ncmax,nic,noc,ic,oc)
char *name, *dname; int ncmax, *nic, *noc; float *ic, *oc;
{
    struct strarr cmds[NCMDS];
    FILE *Ffile;
    int i,icmd;

    *nic = *noc = 1;
    ic[0] = oc[0] = 1.0;	/* default filter is speaker wire */
    for(i=1; i<ncmax; i++) ic[i] = oc[i] = 0.0;

#   define LOADC(i,s) strcpy(cmds[i].string,s)
    LOADC(0,"nicoeffs"); LOADC(1,"nocoeffs"); LOADC(2,"icoeffs");  
    LOADC(3,"ocoeffs");	 LOADC(4,"type");

    strcpy(ctkl,"abcdefghijklmnopqrstuvwxyz");
    strcpy(vtkl,"abcdefghijklmnopqrstuvwxyz0123456789+-.");

    if(*name == '\0')
      fprintf(stderr," Filter");  /* Help out the prompt */
    if ( !_DSPGetFile(&Ffile, "r", name, dname) ) 
      exit(0);
    if( (_DSPGetField(Ffile, cmd, ctkl,NSTR) == EOF) 
	|| strcmp(cmd,"filter") != 0 )
	    ABORT("_DSPGetFilter: Input not a filter file\n");

   while(_DSPGetField(Ffile, cmd, ctkl,NSTR)!=EOF)
	{
again:	icmd=_DSPIndexS(cmds,cmd,NCMDS); 
/* if (icmd>=0)fprintf(stderr,"Got cmd %d = %s\n",icmd,cmds[icmd].cname); */
	if (icmd == -1) 
	{ 
	    fprintf(stderr,"_DSPGetFilter: Unknown command: %s\n",cmd);
	    continue;
	}
	if (icmd < -1) 
	{ 
	    fprintf(stderr,"_DSPGetFilter: %s is ambigous. Type more of it:",cmd);
	    _DSPGetSN(cmd,NSTR);
	    goto again;
	}

	switch(icmd)
	{
	case 0: SETINT(nic);   break;
	case 1: SETINT(noc);   break;
	case 2: sp_fillc(ic,nic,Ffile); break;
	case 3: sp_fillc(oc,noc,Ffile); break;
	case 4: if(_DSPGetField(Ffile,val,vtkl,NSTR)!=EOF 
		   && strcmp(val,"tdl")!=0)
	  fprintf(stderr,"_DSPGetFilter: Only type TDL filters supported\n"); 
		break;
	default:
		_DSPErr("Dryrot at _DSPGetFilter switch"); 
		break;
	}
    }
    if (*nic <= 0) *nic = 1;
    if (*noc <= 0) *noc = 1; /* however, */
    if (oc[0] != 1.0) ABORT("_DSPGetFilter: oc[0] must ALWAYS be unity!!\n");
    return(TRUE);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetHead.c */
/*########################### _DSPGetHead.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETHEAD */

char *
_DSPGetHead(fn) /* return pointer to string equal to fn up to and  */
char *fn;		/* including the last '/'.			   */ 
{
    char *h,*e,*o;

    e = NULL;
    for (h = fn; *h != _DSP_NULLC; h++)
	 if (*h == _DSP_PATH_DELIM)	  /* '/' for UNIX, '\' for MSDOS */
	   e = h;
    if (!e) return(h);		/* "" */
    o = malloc(e - fn + 2);
    if (o == NULL) _DSPErr("_DSPGetHead: no free storage"); 
    strncpy(o,fn,e - fn + 1); /* strcpyn does not terminate o with _DSP_NULLC !!! */
    *(o + (e - fn) + 1) = _DSP_NULLC;
    return(o);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetInputFile.c */
/*########################### _DSPGetInputFile.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETINPUTFILE */

void _DSPGetInputFile(ipp,din)
FILE **ipp; char *din;
{
    char iname[_DSP_MAX_NAME];
    iname[0] = '\0';		/* Force _DSPGetFile to prompt for filenames */
    if (isatty(0))		/* Get input */ 
	{
	    if (din == NULL) din = _DSPCat("testin.dat",0);
	    if (!_DSPGetFile(ipp,"r",iname,din))exit(0);
	    fprintf(stderr," Reading: %s\n", iname);
	    /* Read header here if there is one */
	} 
	else *ipp=stdin;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetIntHexStr.c */
/*########################### _DSPGetIntHexStr.c ###########################*/
/* _DSPGETINTHEXSTR */

/* #include "dsp/_dsputilities.h" */

int _DSPGetIntHexStr(s)		/* get integer from hex string */
    char **s;			/* input string is chopped to int++ */
{
    int i;
    char *p,*t;
/*  t = *s; */
/*  while (!isxdigit(*t)&&*t!='-'&&*t) t++; /* skip to beginning of hex int */

    /* skip to beginning of hex int */
    for (t = *s; !isxdigit(*t) && *t!='-' && *t; t++) 
      if (!isspace(*t) && *t != '$') {
	  printf("_DSPGetIntHexStr: *** Found spurious character ");
	  printf("%c scanning for hex integer in string:\n\t%s\n",*t,*s);
      }
    if (!*t) {
	*s=t;			/* no token */
	return _DSP_NOT_AN_INT; /* no token */
    }
    p=t;
    while (isxdigit(*p)||*p=='-') p++;	/* t->token, p->token++ */
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    sscanf(t,"%X",&i);
    return i;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetIntStr.c */
/*########################### _DSPGetIntStr.c ###########################*/
/* _DSPGETINTSTR */

/* #include "dsp/_dsputilities.h" */

int _DSPGetIntStr(s)		/* get integer or _DSP_NOT_AN_INT from string */
    char **s;			/* input string is chopped to int++ */
{
    int i;
    char *p,*t;
    t = *s;
    while (!isdigit(*t)&&*t!='-'&&*t) t++; /* skip to beginning of int */
    if (!*t) {
	*s=t;			/* no token */
	return _DSP_NOT_AN_INT;
    }
    p=t;

    while (isdigit(*p)||isxdigit(*p)||*p=='-'||*p=='x'||*p=='X') 
      p++; /* t->token, p->token++ */
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    i = strtol(t,NULL,0);	/* sscanf(t,"%d",&i); */
    return i;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetInputOutputFiles.c */
/*########################### _DSPGetInputOutputFiles.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETINPUTOUTPUTFILES */

void _DSPGetInputOutputFiles(ipp,opp,din,don)
FILE **ipp, **opp;
char *din, *don;
{
    _DSPGetInputFile(ipp,din);
    _DSPGetOutputFile(opp,don);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetLineStr.c */
/*########################### _DSPGetLineStr.c ###########################*/
/* _DSPGETLINESTR */

/* #include "dsp/_dsputilities.h" */

char *_DSPGetLineStr(s)		/* get line from string, including '\n' */
    char **s;			/* input string is chopped to line++ */
{
    int i;
    char *p,*t;
    p = t = *s;			/* ptr to 1st char */
    while (*t&&*t!='\n') t++;	/* skip to newline */
    if (!*t) {
	*s=t;			/* no newline, point s to empty string */
	return NULL;
    }
    *s = ++t;			/* remainder string */
    t = _DSPMakeStr(t-p,p);		/* returned string containing line */
    return t;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetOutputFile.c */
/*########################### _DSPGetOutputFile.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETOUTPUTFILE */

void _DSPGetOutputFile(opp,don)
FILE **opp; char *don;
{
    char oname[_DSP_MAX_NAME];
    oname[0] = '\0';	/* Force _DSPGetFile to prompt for filenames */
    if (isatty(1))		/* Get output */
	{
	    if (don == NULL) 
	      don = _DSPCat("testout.dat",0);
	    if(!_DSPGetFile(opp,"w",oname,don)) 
	      exit(0);
	    fprintf(stderr," Writing: %s\n", oname);
	    /* Read header here if there is one */
	} 
	else *opp=stdout;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetSN.c */
/*########################### _DSPGetSN.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETSN */

char *
_DSPGetSN(g,ng) /* get string up to \n from /dev/tty */
char *g; int ng;	/* At most ng chars are read */
{
#define MAXC 100

	char *s; int nstr, lstr; register c; register char *cs;
	FILE *in;

	in = fopen("/dev/tty", "r");
	if(g == NULL) { s = _DSPNewStr(MAXC); lstr = MAXC; }
	    else { s = g; lstr = ng;}
	nstr = 0; 
	cs = s;
	while ((c = getc(in)) != '\n' && c >= 0 && ++nstr < lstr ) *cs++ = c;
	*cs = '\0';
	fclose(in);
	return(s);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetTail.c */
/*########################### _DSPGetTail.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPGETTAIL */

char *_DSPGetTail(fn) /* return pointer to string equal to fn after 1st "." */
char *fn;	   /* The '.' is appended to the front of the extension	 */
{
    char *n,*o;

    if (fn == NULL) _DSPErr("_DSPGetTail: NULL pointer passed");
/* printf("_DSPGetTail: /%s/ = _DSPRemoveHead(/%s/)\n",_DSPRemoveHead(fn),fn); */
    fn = _DSPRemoveHead(fn);
    while ((*fn!=_DSP_NULLC)&&(*fn!='.'))  fn++;
    if (*fn == _DSP_NULLC) return(fn);	/* "" */
    o=n=(char *)malloc(strlen(fn)+1);	/* fn is pointing at the . */
    if(n==NULL) _DSPErr("_DSPGetTail: no more free storage");
    while(*fn!=_DSP_NULLC) *n++ = *fn++;
    *n = _DSP_NULLC;
/* printf("_DSPGetTail: returning /%s/\n",o); */
    return(o);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetTokStr.c */
/*########################### _DSPGetTokStr.c ###########################*/
/* _DSPGETTOKSTR */

/* #include "dsp/_dsputilities.h" */

char *_DSPGetTokStr(s)		/* get token or NULL from string */
    char **s;			/* input string is chopped to token++ */
{
    char *p,*t;
    if (!s || !*s || !**s) return NULL;
    t = *s;			    /* beginning of string */
    while (*t && !isalpha(*t) ) t++; /* skip to first alpha */
    if (!*t) {	/* no token found. Return NULL and "" */
	*s=t;
	return NULL;
    }
    p=t;
    while (isalnum(*p)||*p=='_') p++;	/* t->token, p->token++ */
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    return t;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPIndexS.c */
/*########################### _DSPIndexS.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPINDEXS */

/* _DSPIndexS: For matching abbreviated keywords to list of full keywords.
 * Return index (<= nstr) in string array stra at which string str appears;
 * -1 if not found, -n if there were n >1 matches. 
 *
 * Uses	   struct strarr { char string[_DSP_MAX_CMD] } stra[]; 
 */

#define NOTFOUND	-1

int _DSPIndexS(stra, str, nstr)
struct strarr *stra;
register char *str;
int nstr;
{
    register int i, lstr; 
    int nhits, ihit;
   
    if (stra == NULL) _DSPErr("_DSPIndexS: NULL pointer passed for keyword table");
    if (str == NULL) _DSPErr("_DSPIndexS: NULL pointer passed for search key");
    nhits = 0;
    lstr = strlen(str);
    for(i=0; i<nstr; i++)
    {
/* fprintf(stderr,"_DSPIndexS: comparing /%s/ to /%s/\n",str,stra[i].string); */
	if (strncmp(stra[i].string,str,lstr)==0)
	{
	    nhits += 1;
	    ihit = i;
	}
    }
    if (nhits == 0) return(NOTFOUND);
    if (nhits > 1) return(-nhits);
/*fprintf(stderr,"_DSPIndexS: Unique hit of %s on %s\n", str, stra[ihit].string); */
    return(ihit);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPInInt.c */
/*########################### _DSPInInt.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPININT */

int _DSPInInt(def, name)
int def; char *name;
{
    int val; 
    char *line;
    DSP_BOOL white;

    fprintf(stderr,_DSPCat(name,"(=%d):"),def);
    line = _DSPGetSN(0,0);
    white = TRUE;
    val = atoi(line);
    while(*line != _DSP_NULLC)	if( isdigit(*line++) ) white = FALSE; 
    if(white)
    {
	fprintf(stderr,_DSPCat(name," not changed\n"));
	return(def);
    }
    else 
    {
	fprintf(stderr,_DSPCat(name, " set to %d\n"),val);
	return(val);
    }
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPMakeArray.c */
/*########################### _DSPMakeArray.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPMAKEARRAY */

int *
_DSPMakeArray(size)
int size;
{
    int *b;
    b=(int *)malloc(size);
    if (b==NULL) _DSPErr("_DSPMakeArray: no free storage");
    return(b);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPMyFopen.c */
/*########################### _DSPMyFopen.c ###########################*/
/* _DSPMYFOPEN */

/* #include "dsp/_dsputilities.h" */

FILE *_DSPMyFopen(fn,mode)
    char *fn, *mode;
{
    FILE *fp;
    fp = fopen(fn,mode);
    if (fp==NULL)
      fprintf(stderr,"_DSPMyFopen: *** Can't open %s file: %s ***\n",
	      ((*mode=='r')?"input":"output"),fn);
    return fp;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPNotBlank.c */
/*########################### _DSPNotBlank.c ###########################*/
/* _DSPNOTBLANK */

/* #include "dsp/_dsputilities.h" */

DSP_BOOL _DSPNotBlank(s)		/* test string for whitespace only */
    char *s;			/* input string */
{
    return *_DSPSkipWhite(s)!='\0';
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPPadStr.c */
/*########################### _DSPPadStr.c ###########################*/
/* _DSPPADSTR */

char *_DSPMakeStr();		/* this lib */
#include <stdio.h>

char *_DSPPadStr(s,n)  /* pad string to fixed width, right-filled with blanks */
    char *s;			/* input string */
    int n;			/* desired minimum width */
{
    char *p;
    int i;
    p = _DSPMakeStr(n,s);		/* null filled or truncated */
    if (strlen(s)>=n) fprintf(stderr,
      "_DSPPadStr: field width = %d while string length = %d\n",n,strlen(s));
    else for (i=strlen(s)+1,s=p+strlen(s); i<n ; i++) *s++ = ' ';
    return p;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPParseName.c */
/*########################### _DSPParseName.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPPARSENAME */

/* Return filename consisting of that which   
 * is specified in name plus any extra fields 
 * present in the default name dname.	      
 * A full name is head/body.tail or	      
 * directory-path/name.extension if you will. 
 * There are a few peculiarities in overriding defaults. 
 * To specify a filename with no tail, one must include
 * a `.' after the name which signifies a null tail. To
 * get a filename followed by a single `.', type two dots
 * and for two, type three, etc. 
 * Finally, to get rid of a default directory, type ./ followed
 * by carriage return.
 */

void _DSPParseName(name, dname) 
char *name, *dname;		
{
    char *nhead, *nbody, *ntail, *dhead, *dbody, *dtail;
    char *ohead, *obody, *otail; 
    char *oname, *t;

 /* fprintf(stderr,"_DSPParseName gets name = /%s/, dname = /%s/\n",name,dname); */
    if (*dname == _DSP_NULLC) return;
    if (*name == _DSP_NULLC) { strcpy(name,dname); return; }

    nhead = _DSPGetHead(name);
    dhead = _DSPGetHead(dname);
    nbody = _DSPGetBody(name);
    dbody = _DSPGetBody(dname);
    ntail = _DSPGetTail(name);
    dtail = _DSPGetTail(dname);

# define E(name) (strlen(name)!=0)		/* `Exists' */

    if(E(dhead) && !E(nhead)) ohead = _DSPCat(dhead,0); else ohead = _DSPCat("",nhead);
    if(E(dbody) && !E(nbody)) obody = _DSPCat(dbody,0); else obody = _DSPCat("",nbody);
    if(E(dtail) && !E(ntail)) otail = _DSPCat(dtail,0); else otail = _DSPCat("",ntail); 
/* Special hack for trailing `.' */
    for(t=otail; *t == '.'; t++);
    if(*otail != _DSP_NULLC && (t-otail) == strlen(otail) ) otail++;

/* printf("_DSPParseName: head = /%s/, body = /%s/, tail = /%s/\n",ohead,obody,otail); */
    oname = _DSPCat(ohead, _DSPCat(obody, _DSPCat(otail, NULL)));
    strcpy(name, oname);
    if(strlen(name) == 0)
    fprintf(stderr," _DSPParseName: Warning: Your filename is null which means `.'\n");
 /* fprintf(stderr,"_DSPParseName returns with name = /%s/, dname = /%s/\n",name,dname); */
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPPutFilter.c */
/*########################### _DSPPutFilter.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPPUTFILTER */

void _DSPPutFilter(name, dname, ni, no, ic, oc)
char *name, *dname; int ni, no;
float *ic, *oc;
{
    FILE *fp;
    int i,j;

    if (dname == NULL || *dname == _DSP_NULLC) dname = (char *)_DSPCat("test.flt",0);
    fprintf(stderr,"Filter");
    if(!_DSPGetFile(&fp,"w",name,dname)) exit(0);
    fprintf(stderr," Writing: %s\n", name);
    if (no>1) fprintf(fp,"# IIR filter\n");
	else fprintf(fp,"# FIR filter\n");
    fprintf(fp,"# To see its response, type\n"); 
    fprintf(fp,"#\n#	 filter filterfile < i.dat | plotf\n#\n");
    fprintf(fp,"# while logged in at the VT55 DECscope\n");
    fprintf(fp,"# where i.dat is an impulse (1,0,0,0,0,...)\n");
    fprintf(fp,"# and filterfile is this file\n#\n");
    fprintf(fp,"filter;\n");
    fprintf(fp,"NIcoeffs = %d\n",ni);
    fprintf(fp,"NOcoeffs = %d\n",no);

    fprintf(fp,"ICoeffs = \n");
    for(i=0;i<ni;i++)
    {
	fprintf(fp," %-17.10e", ic[i]);
	if (i%4 == 0) fprintf(fp,"\n");
    }

    fprintf(fp,"OCoeffs = \n");
    for(i=0;i<no;i++)
    {
	fprintf(fp," %-17.10e", oc[i]);
	if (i%4 == 0) fprintf(fp,"\n");
    }
    fclose(fp);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPRemoveHead.c */
/*########################### _DSPRemoveHead.c ###########################*/
/* #include "dsp/_dsputilities.h" */
    
/* _DSPREMOVEHEAD */

char *
_DSPRemoveHead(fn)	/* return pointer to string equal to fn after	*/
char *fn;		/* the last '/'. If there is no '/',		*/ 
{			/* return the whole string			*/
    char *h,*e,*o;
    int len;

    if (!fn)
      return(fn);

    len = 0;
    e = NULL;
    for (h = fn; *h != _DSP_NULLC; h++)
	 if (*h == _DSP_PATH_DELIM)	  /* '/' for UNIX, '\' for MSDOS */
	   e = h;

    if (!e) return(_DSPCat(fn,0));
    for (h = e+1; *h != _DSP_NULLC; h++) len++ ;
    o = malloc(len+1);
    if (o == NULL) _DSPErr("_DSPRemoveHead: no free storage"); 
    strncpy(o,e+1,len+1); /* strcpyn does get the _DSP_NULLC this time */ 
    return(o);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPRemoveTail.c */
/*########################### _DSPRemoveTail.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPREMOVETAIL */

char *
_DSPRemoveTail(fn)  /* return pointer to string equal to fn up to 1st "." */
char *fn;	   /* The '.' is not appended to the string.		 */
{		   /*  If there is no "." return the whole string	 */
    char *m,*head;

    if (!fn)
      return(fn);

    head = _DSPGetHead(fn);
    fn = _DSPRemoveHead(fn);
    for (m=fn; (*m!=_DSP_NULLC)&&(*m!='.') ; m++);
    *m = _DSP_NULLC;
    return( _DSPCat(head,fn) );
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPSaveMatD.c */
/*########################### _DSPSaveMatD.c ###########################*/
/* _DSPSaveMatD.c - Save double matrix in matlab binary ".mat" format */

#include <stdio.h> 

int _DSPSaveMatD(mrows,ncols,imagf,name,rpart,ipart,fp)
	int mrows,ncols,imagf;
	char *name;
	double rpart[],ipart[];
	FILE *fp;
{
	int wtflg;
/* 
 * NAME
 *	_DSPSaveMatD - for writing PC-MATLAB data file
 *
 * DESCRIPTION
 *	_DSPSaveMatD writes out a 
 *	Arguments:
 *	mrows	- number of rows in matrix
 *	ncols	- number of columns in matrix
 *	imagf	- imaginary flag; 0 for no imaginary part or 1 if the 
 *		  data has an imaginary part
 *	name	- string holding the matrix name.
 *	rpart	- real part of matrix (mrows x ncols double precision 
 *		  elements stored column wise)
 *	ipart	- imaginary part of matrix (only used if imagf = 1)
 *	fp	- file pointer of output file
 *
 *	PC-MATLAB is capable of reading more than one matrix 
 *	on a file.
 *
 *
 * RETURN
 *
 *	wtflg	- write flag; 0 = good write, 1 = error during file write
 *
 * 20 byte header = type, mrows, ncols, imagf, namlen (32-bit integers)
 *
 */
	int mn,i,j,nw,nl,type;
	char *bp;
	int buf[5];
	int trace;
	type=1000; /* 1000 + text flag= 0 for numeric or 1 for text matrix data */
/* int _DSPSaveMatD(mrows,ncols,imagf,name,rpart,ipart,fp) */
/*	savemat_(&type,&mrows,&ncols,&imagf,&namelen,name,
		     rpart, ipart, &lunit, &irec, &wtflg); */
	mn =mrows*ncols;
	if (trace > 4) {
	  fprintf(stderr,"\t _DSPSaveMatD: Called on %d samples\n",mn);
	  for (i=0;i<10;i++) 
	    fprintf(stderr,"\t r[%d]=%f ",i,rpart[i]); fprintf(stderr,"\n"); }

/* Prepare header */
	buf[i=0]=type;	buf[++i]=mrows; buf[++i]=ncols; buf[++i]=imagf;
	buf[++i]=nl=strlen(name)+1; /* Add 1 to count the terminating null */
	wtflg = 0; /* write succeeded indication */
/* Write header */
	nw = fwrite(buf,sizeof(*buf),++i,fp);
	if (nw != i) 
	  usererr(wtflg=1,
		  _DSPCat("savemat: could not write header of file:",name));
/* Write name */
	if (trace > 4) fprintf(stderr,"\t matrix name is %s = %o\n",name,name);
/*   nw = fwrite(name,sizeof(*name),nl+1,fp);
 *	if (nw != nl+1) usererr(wtflg=1,"savemat: could not write name");
 */
	bp=name;
	for (i=0;i<nl;i++) putc(*bp++,fp); /* No trailing anything */

/* Write real data */
/*
	nw = fwrite(rpart,1,8*mn,fp);
	if (nw != 8*mn) usererr(wtflg=1,"savemat: could not write realpart");

	bp = (char *) rpart;
	for(i=0;i<mn;i++) for(j=7;j>=0;j--) fwrite(*(bp+8*i+j),1,1,fp);
 */
    nw = fwrite(rpart,sizeof(*rpart),mn,fp);
    if (nw != mn) usererr(wtflg=1,"savemat: could not write realpart");

/* Write imaginary data */
	if (imagf == 1) {
	    nw = fwrite(ipart,sizeof(*ipart),mn,fp); /* Write imag part */
	    if (nw != mn) usererr(wtflg=1,"savemat: could not write imagpart");
	}
	return wtflg;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPSezYes.c */
/*########################### _DSPSezYes.c ###########################*/
/* #include "dsp/_dsputilities.h" */

/* _DSPSEZYES */

int _DSPSezYes()
{
    char *reply;

/*    CLRBFI;		/* Flush type-ahead  (I don't trust it) */
    reply = _DSPGetSN(NULL);
    if(*reply=='Y' || *reply=='y') return(TRUE);
	else return(FALSE);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPSkipToWhite.c */
/*########################### _DSPSkipToWhite.c ###########################*/
/* _DSPSKIPTOWHITE */

char *_DSPSkipToWhite(s)		/* skip to first white space */
    char *s;			/* input string */
{
    while(*s!='\t'&&*s!=' '&&*s!='\n'&&*s) s++;
    return s;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPSkipWhite.c */
/*########################### _DSPSkipWhite.c ###########################*/
/* _DSPSKIPWHITE */

char *_DSPSkipWhite(s)		/* skip past white space */
    char *s;			/* input string */
{
    while((*s=='\t'||*s==' '||*s=='\n')&&*s) s++;
    return s;
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPFirstReadableFile.c */
/*#################### _DSPFirstReadableFile.c ######################*/

#ifndef __FILE_HEADER__
#include <sys/file.h>
#endif	__FILE_HEADER__
/* For access(2) */
#define R_OK	4/* test for read permission */
#define W_OK	2/* test for write permission */
#define X_OK	1/* test for execute (search) permission */
#define F_OK	0/* test for presence of file */

/* _DSPFirstReadableFile - return first readable file in list, else NULL */
/* file = _DSPFirstReadableFile(filename,Nexts,ext1,ext2,...,extN); */

char *_DSPFirstReadableFile(char *fn,...)  {
    va_list ap;
    char* fb;			/* file body */
    char* fe;			/* file extension */
    char* nfe;			/* next extension to try */
    char* nfn;			/* next name to try */
    int argno = 0;
    int curArg;
    int nExts;
    int i;
    extern int access();

    va_start(ap,fn);

    if (!fn) {
	nfn = fn;
	goto exit_funnel;
    }
    fb = _DSPRemoveTail(fn);
    fe = _DSPGetTail(fn);

    /* take original file if it exists */
    if (access(fn,R_OK)!=-1 ) {
	nfn = fn;
	goto exit_funnel;
    }

    /* take first existing file using the supplied extensions */
    nExts = va_arg(ap,int);
    for (i=0;i<nExts;i++) {
      nfe = va_arg(ap,char*);
      nfn = _DSPCat(fb,nfe);
      if ( access(nfn,R_OK)!=-1 ) 
	goto exit_funnel;
    }
    nfn = NULL;

  exit_funnel:
    va_end(ap);
    return(nfn);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetMemStr.c */
/*#################### _DSPGetMemStr.c ######################*/

DSPLocationCounter _DSPGetMemStr(s,type) /* get DSP memory type from string or DSP_LC_N */
    char **s;		/* input string is lopped */
    char *type;		/* "A" for absolute section, "R" for relative */
{
    char *t;
    int space,ctr;
    DSPLocationCounter m;
    t = _DSPGetTokStr(s);		/* {X,Y,L,P}{,L,H}{I,E,B} */
    if (!t) return(DSP_LC_N);
    t = _DSPToUpperStr(t);
    if (*t=='N') return(DSP_LC_N);
    else if (*t=='X') space=0;
    else if (*t=='Y') space=1;
    else if (*t=='P') space=3;
    else if (*t=='L') space=2;
    else return DSP_LC_N;
    t++;
    /* Note that {I(nternal),E(xternal),B(ootstrap)} are ignored if present */
    if (*type=='A'||*t=='\0'||*t=='I'||*t=='E'||*t=='B') ctr=0;
    else if (*t=='L') ctr=1;
    else if (*t=='H') ctr=2;
    else fprintf(stderr," *** DSP mem space string %s unexpected ***\n",--t);
    m = (DSPLocationCounter) (space*3+ctr+1);
    /* detect mem spec and return proper enum */
    return(m);
}

/*%-**$$$ BagItem $$$**-%*/
/* _DSPGetDSPIntStr.c */
/*#################### _DSPGetDSPIntStr.c ######################*/
/* _DSPGETDSPINTSTR */

int _DSPGetDSPIntStr(s)		/* get integer from string in DSP format */
    char **s;			/* input string is chopped to int++ */
{
    int i,sgn;
    char *p,*t,r,*tsave;
    t = *s;
    while (!isdigit(*t) && *t!='-' && *t!='%' && *t!='`' && *t!='$' && *t) 
      t++; /* skip to beginning of <int> || %<int> || `<int> || $<int> */
    if (!*t) {
	*s=t;			/* no token */
	return _DSP_NOT_AN_INT;
    }
    sgn = 1;
    if (*t=='-') {		/* minus sign appears before radix indicator */
	sgn = -1;
	t++;
    }
    if (!isdigit(*t)) {
	r = *t++;		/* radix spec */
	if (sgn == -1)
	  *--t = '-';		/* install minus sign for sscanf use */
    }
    else r='`';			/* default radix is decimal */
    p=t;			/* t will point to token, p to token "++" */
    if (r == '$')		/* hexadecimal number */
      while (isxdigit(*p)) p++;
    else if (r == '`')		/* decimal number */
      while (isdigit(*p)) p++;
    else if (r == '%')		/* binary number */
      while (*p == '1' || *p == '0') p++;
    *s = p;			/* remainder string */
    t = _DSPMakeStr(p-t+1,t);	/* finished token */
    t = _DSPToUpperStr(t);		/* upper casify */
    tsave = _DSPCopyStr(t);

    if (r == '$') {		/* hexadecimal number */
	sscanf(t,"%X",&i);
    }
    else if (r == '`') {	/* decimal number */
	sscanf(t,"%d",&i);
    }
    else if (r == '%') {	/* binary number */
	i=0;
	if (sgn<0) t++;
	do {
	    i <<= 1;
	    if (*t == '1') i += 1; 
	    else if (*t != '0') 
	      fprintf(stderr,"_DSPGetDSPIntStr: bogus binary number: %s",
		      tsave);
	} while (*++t);
	if (sgn<0) i = ~i + 1;	/* two's complement */
    }
    return i;
}


