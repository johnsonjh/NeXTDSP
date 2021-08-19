#ifdef SHLIB
#include "shlib.h"
#endif

/*
  parseScorefile.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/18/89/daj - Changes to accomodate new way of doing parameters (structs 
                 rather than objects).

  09/22/89/daj - Flushed _MKNameTableAddGlobalNameNoCopy() in light of the 
                 decision to make name table bits
		 globally defined. Other changes corresponding to changes
		 in _MKNameTable.

  10/06/89/daj - Changed to use hashtable.h version of _MKNameTable table.
  10/20/89/daj - Added binary scorefile support.
  11/20/89/daj - Changed a few comments and made jmp_buf static.
  12/4/89/daj  - Fixed ran and added seed and ranSeed operators.
  12/8/89/daj  - Added binary scorefile support to include facility.
  12/15/89/daj - Changed binary scorefile magic to one word.
   1/2/90/daj  - Added break; on line 3029. 
   1/8/90/daj  - Changed longjmp to set status to 1.
                 Changed setjmp logic in binaryHeaderStmt() and
		 parseBinaryNote(). 
		 Changed _MKNewScoreInStruct() to not return NULL when the
		 file is empty.
   1/11/90/daj - Changed ascii parser to match lookahead at the start, rather
                 than the end of statements. This makes it possible to have
		 C functions for reading a single note statement.
		 Removed _MK_begin cases in parseScoreNote and 
		 parseBinaryScoreNote.
   3/05/90/daj - Clarified transtab backslash translation. Added translation
                 of backslash, single quote and double quote. Removed 
		 unnecessary binary translation. Made transtab be a function
		 (to allow _ParName.m to access it for backward translation).
   3/06/90/daj - Added "repeat" ScoreFile language extension.		 
   3/10/90/daj - Fixed bug in error reporting for mutes.
   3/10/90/daj - Added "if/else and boolean operators".
   4/21/90/daj - Removed extra MATCH('}') in then clause of endStmt
*/
/* The ASCII parser is split into three levels: lexical analysis, 
   recursive descent expressions, and top-level statements and 
   declarations. Object type, Envelopes and WaveTables declarations 
   are handled by the recursive descent portion of the parser. This is because 
   they are the only declarations that can occur within expressions. 
   All other declarations are handled by the top-level. 

   There is also a binary scorefile format. For the most part, it
   uses its own set of routines. For the binary format, we chose NOT
   to use object archiving. The reason is that this would require the
   entire Score, Part and Notes to be read in as objects. We want to 
   reserve the right to read it in one Note at a time, thus avoiding 
   building the entire object structure. E.g. this is how ScorefilePerformer
   works.

   See binaryScorefile.doc on the musickit source directory for explanation
   of binary scorefile format. 

   */

#import <stdio.h>               /* Contains FILE, etc. */
#import <signal.h>
#import <setjmp.h>
#import <sys/time.h>
#import <string.h>
#import <ctype.h>

#import "_musickit.h"
#import "_MKNameTable.h"
#import "_Note.h"
#import "_ScorefilePerformer.h"
#import "_tokens.h"
#import "_ParName.h"
#import "_ScorefileVar.h"
#import "_TuningSystem.h"
#import "_error.h"

#define INT(x) (short) x

#define FIRST_CHAR_SCOREMAGIC '.'

#import <objc/HashTable.h>

static jmp_buf begin;       /* For long jump. */

/* Lexical analysis. ---------------------------------------------------- */

/* This module does lexical analysis for scorefile reading. */

#define INITIALMAXTOKENLENGTH 100

#define STRTYPE unsigned char           /* Must match streams.h */

static void
error(MKErrno errCode,...);
static void
warning(MKErrno errCode,...);

typedef struct _parseStateStruct {  
    /* Private structure for storing parse state of a given file. */
    struct _parseStateStruct *_backwardsLink;  /* For file stack. */
    short _lookahead;                   /* Next token.*/
    int _lineNo;                        /* Line number in file. */
    int _pageNo;                        /* Page number in file. */
    char * _name;                          /* Name of file, if any */
    _MKParameterUnion _tokenVal;        /* Value. */
    NXStream * _stream;                 /* Stream pointer */
    int _fd;                            /* File descriptor */
} parseStateStruct;

/* All of the following are global for speed -- to avoid indirection and
   passing arguments. */

static _MKScoreInStruct * scoreRPtr = NULL; /* Main structure */
static parseStateStruct * parsePtr = {NULL}; 
/* Current file parse state -- one for each included file. */

static NXStream *scoreStream = NULL; /* Stream ptr of current included file.*/
static short lookahead = 0;         /* Next token */
static	_MKParameterUnion *tokenVal = NULL;/* Current token (union type) */
    
/* The following are for the current token scan. */
static char * tokenBuf = NULL;   /* start of name field */
static char * tokenPtr = NULL;  /* current location in name field */
static unsigned tokenSize = INITIALMAXTOKENLENGTH; /* Size of tokenBuf */
static char * tokenEndBuf = NULL; /* If tokenPtr gets here, token must expand. */
    
static char
  expandTok(c)
char c;
{
#   define EXPANDAMOUNT INITIALMAXTOKENLENGTH 
    int newSize;
    newSize = (tokenSize + EXPANDAMOUNT);
    _MK_REALLOC(tokenBuf,char,newSize);
    tokenPtr = &(tokenBuf[tokenSize]);
    tokenSize = newSize;
    tokenEndBuf = tokenBuf + tokenSize;
    return (*tokenPtr = c);
#   undef EXPANDAMOUNT
}

#define NEXTCHAR() NXGetc(scoreStream)
#define NEXTTCHAR() ((++tokenPtr > tokenEndBuf) ? expandTok(NEXTCHAR()) : \
		     (*tokenPtr = NEXTCHAR()))
#define NEWTOKEN(_c) tokenPtr = tokenBuf; *tokenPtr = _c
#define ENDTOKEN() *(tokenPtr+1) = '\0'; tokenPtr = NULL
#define BACKUP(_c) if (_c != -1) NXUngetc(scoreStream)
#define BACKUPENDTOKEN(_c) *(tokenPtr) = '\0'; tokenPtr = NULL; BACKUP(_c)


#define addLocalSymbol(_name,_obj,_type) \
  _MKNameTableAddName(scoreRPtr->_symbolTable,_name,nil,_obj, \
		      (_type | _MK_BACKHASHBIT),YES)
/* We need backhash bit because we may have to find it later to remove it,
   if it becomes "exported" as a global. */

static unsigned char *errorLoc = NULL;

static const char transtab[][2] = {'b',BACKSPACE, 
				     'f',FORMFEED,
				     'n','\n', 
				     'r',CR, /* carriage return */
				     't',TAB,
				     'v',VT, /* verticl tab (vt) */
				     BACKSLASH,BACKSLASH,
                                     QUOTE,QUOTE, /* single quote */
				     '"','"'}; /* double quote */

const char *_MKTranstab()
{
    return (const char *)transtab;
}

/* transtab maps back-slashified characters appearing in strings read in from a
   scorefile. 

   Example: When parsing a scorefile string, you encounter a back-slash,
   If that backslash is followed by a newline, it's a line continuation.
   Otherwise, it's a back-slashified char. In this case, 

   char *ptr = index((char *)transtab,c) 

   is a pointer to the table entry for that character, if any. If ptr is NULL,
   there is no mapping for that back-slashified char. Otherwise, c is converted
   into the character immediately following c in the table.

   Note that this use assumes that arrays are stored such that the last 
   dimension varies the most rapidly. 
   */


enum {noLongjmp = 0,errorLongjmp,fatalErrorLongjmp,eofLongjmp};

static short
  lexan(void)
{	
    /* lexan is the lexical analyzer. Time spent optimizing this function
       is well-spent, as this is where the bulk of the compute time goes
       in parsing a file. */
    register short c;
    errorLoc = scoreStream->buf_ptr; /* Save location before current token. */
    while ((c=NEXTCHAR()) == ' ' || c == TAB)
      ;
    if ((char)c == (char)EOF) 
      longjmp(begin,eofLongjmp);
    
    /* I follow the conventions defined in the C reference manual
       scanf expectations for number parsing except that a number
       consisting only of an exponent is not allowed. */
    
    if (c == '.') {                  /* Get double number. */
	NEWTOKEN(c);                  
	while (isdigit(c=NEXTTCHAR()))
	  ;
	if (c == 'e' || c == 'E')   
	  if (isdigit(c=NEXTTCHAR()) || c =='+' || c == '-') 
	    while (isdigit(c=NEXTTCHAR())) 
	      ;
	BACKUPENDTOKEN(c);
	tokenVal->rval = atof(tokenBuf);
	return INT(MK_double);
    }			
    if (isdigit(c)) {                 /* Get int or double number. */
	BOOL weGotDouble;
	NEWTOKEN(c);
	if ((c=NEXTTCHAR()) == 'x' || c == 'X') { /* hex */
	    c = NEXTTCHAR();
	    NEWTOKEN(c);               /* Flush 0x */
	    while (isxdigit(c=NEXTTCHAR()))
	      ;
	    BACKUPENDTOKEN(c);
	    sscanf(tokenBuf,"%x",&tokenVal->ival);
	    return INT(MK_int);
	}
	weGotDouble = NO;
	if (isdigit(c))
	  while (isdigit(c=NEXTTCHAR()))
	    ;
	if (c == '.') {  
	    weGotDouble = YES;
	    while (isdigit(c=NEXTTCHAR()))
	      ;
	}
	if (c == 'e' || c == 'E') {  
	    weGotDouble = YES;
	    if (isdigit(c=NEXTTCHAR()) || c =='+' || c == '-') 
	      while (isdigit(c=NEXTTCHAR())) 
		;
	}
	BACKUPENDTOKEN(c);
	if (weGotDouble) { 
	    tokenVal->rval = atof(tokenBuf);
	    return INT(MK_double);
	}
	if (tokenBuf[0] == '0')       /* Octal */
	  sscanf(tokenBuf,"%o",&tokenVal->ival);
	else tokenVal->ival = atoi(tokenBuf);
	return INT(MK_int);
    }                     /* End of number scanner. */
    if (isalpha(c)) {     /* Symbol scanner. */
	unsigned short tok;
	NEWTOKEN(c);
	while (isalnum(c = NEXTTCHAR()) || c == '_')
	  ;
	BACKUPENDTOKEN(c);
	tokenVal->symbol = 
	  _MKNameTableGetObjectForName(scoreRPtr->_symbolTable,tokenBuf,nil,
				       &tok);
	if (tokenVal->symbol /* It's an object */
	    || tok)          /* It's a keyword. */
	  return INT(tok);
	return INT(_MK_undef);
    }                    
    switch (c) {
      case '':
	parsePtr->_pageNo++;
	return lexan();
      case '"':
	c = NEXTCHAR(); /* Don't include leading " in the string. */
	NEWTOKEN(c);
	if (c != '"')
	  do {
	      if (c == BACKSLASH) { /* Line continuation or escape char */
		  if ((c=NEXTTCHAR()) == '\n') {
		      parsePtr->_lineNo++;
		      c = '\0';           /* Just to fool test below */
		      tokenPtr -= 2;      /* Line continuation. */
		  }
		  else if (index((char *)transtab, c))  /* see string.h */
		    *(--tokenPtr) = index((char *)transtab, c)[1];
		  /* Otherwise we pass along the back-slash and its associated 
		     character as string text. We do not bother to convert
		     ASCII numeric escape codes. I can't believe anybody
		     would ever need to do that. (Famous last words.) */
	      }
	      if (c == EOF || c == '\n')
		error(MK_sfMissingStringErr,"\"");
	  } while ( (c=NEXTTCHAR()) != '"'); 
	tokenPtr--;   /* Don't include final " in string but advance input. */
	ENDTOKEN();
	tokenVal->sval = (char *)NXUniqueString(tokenBuf); 
	/* We can't anticipate where the damn thing will end up so we can't
	   do garbage collection. So we make it unique to avoid accumulating
	   garbage. At some point in the future, might want to really move
	   to unique strings system-wide. This makes string compares faster,
	   for example. */
	return INT(MK_string);
      case '$':                         /* Dsp format hex */
	c = NEXTCHAR();                 /* Gobble the $. */
	NEWTOKEN(c);
	while (isxdigit(c=NEXTTCHAR()))
	  ;
	BACKUPENDTOKEN(c);
	sscanf(tokenBuf,"%x",&tokenVal->ival);
	return INT(MK_int);
      case '\n':
	parsePtr->_lineNo++;
	return lexan();
      case BACKSLASH:  
	if ((c=NEXTCHAR()) == '\n')    
	  return lexan();
	else error(MK_sfMissingBackslashErr);
      case '<':
	if ((c=NEXTCHAR()) == '=')
	  return _MK_LEQ;
	else {
	    BACKUP(c);
	    return '<';
	}
      case '>':
	if ((c=NEXTCHAR()) == '=')
	  return _MK_GEQ;
	else {
	    BACKUP(c);
	    return '>';
	}
      case '=':
	if ((c=NEXTCHAR()) == '=')
	  return _MK_EQU;
	else {
	    BACKUP(c);
	    return '=';
	}
      case '!':
	if ((c=NEXTCHAR()) == '=')
	  return _MK_NEQ;
	else {
	    BACKUP(c);
	    return '!';
	}
      case '&':
	if ((c=NEXTCHAR()) == '&')
	  return _MK_AND;
	else {
	    BACKUP(c);
	    return '&';
	}
      case '|':
	if ((c=NEXTCHAR()) == '|')
	  return _MK_OR;
	else {
	    BACKUP(c);
	    return '|';
	}
      case '/': 			/* strip out comments. */
	if ((c=NEXTCHAR()) == '/') {	/* objective C style */
	    while (((c=NEXTCHAR()) != '\n') && (c != EOF))
	      ;
	    parsePtr->_lineNo++;
	}
	else if (c == '*') {	        /* C style */
	    register short nextC = 0;
	    c = NEXTCHAR();             /* Don't count star twice. */
	    while ((c != EOF) && (((c=NEXTCHAR()) != '/') || (nextC != '*'))) {
		nextC = c;
		if (c == '\n') 
		  parsePtr->_lineNo++;
	    }
	}
	else {
	    BACKUP(c);                 /* We've read too far. */
	    return '/';                   /* It's a division. */
	}
	return lexan();
      default:                      /* Single-character operator. */
	if (iscntrl(c)) {
	    warning(MK_sfNonScorefileErr);
	    longjmp(begin,eofLongjmp);
	}
	return c;
    }
}		

#define MATCH(_dummy) lookahead = lexan()
  
static BOOL 
  match(short token)
{
    if (lookahead == token)
      lookahead = lexan();
    else return NO;
    return YES;
}


static char * 
  curToken(void)
{
    /* Used for error reporting. */
    static char charS[2] = {'\0','\0'};
    if (lookahead < INT(_MK_undef)) {
	charS[0] = lookahead;
	return charS;
    }
    if (tokenBuf)   /* This check probably not needed. */ 
      return tokenBuf; 
    return (char *)_MKTokName(lookahead);
}

static void
  insertError(int op)
{
    char s[2];
    char c = (char) op;
    sprintf(s,"%c",c);
    warning(MK_sfMissingStringErr,s);
}

static void matchInsert(short token)
{
    if (match(token))
      return;
    insertError(token);
}

static void matchSemicolon(void)
{
    if (!match(';'))
    	error(MK_sfMissingSemicolonErr);
}


/* Recursive descent expression parsing. ------------------------------ */

/* Variables I store as _ScorefileVar objects. 
   Intermediate values are handled as unions with associated types. There
   are two parallel stacks for the union and type. */

#define SEMITONEOP '~'   /* twiddle */

/* Grammar is as follows:
   
   assign:=catOrFunEval | catOrFunEval = assign   (Right associative)
   catOrFunEval := expr & expr | expr | expr @ expr
   expr := term + term |   term - term |   term
   term := expon * expon |   expon / expon |   expon | expo % expon
   expon:= postfix ^ postfix | postfix ~ postfix |   postfix 
   postfix:= postfix |  postfix dB | postfix 
   prefix:=-factor | factor
   factor:= (assign) | value | ran | envelopeValue | b
   value:= number | stringConstant | envelopeValue | var | waveTableValue
   envelopeValue:= namedEnvelopeDecl | envelopeConstant | envelopeName
   objectValue:= namedObjectDecl | objectConstant | objectName
   waveTableValue:= namedWaveTableDecl | waveTableConstant | waveTableName
   namedEnvelopeDecl:= envDecl name = segEnvelopeConstant |
                       envDecl name = fileEnvelopeConstant
   envDecl:= ENVELOPE
   segEnvelopeConstant:= [ pointlist ]		  
   namedWaveTableDecl:= waveDecl name = WaveTableConstant 
   waveDecl:= WAVETABLE
   WaveTableConstant:= [ valueslist ] | [ {soundfileName} ]		  
   namedObjectDecl:= objDecl name = objConstant 		      
   objDecl:= OBJECT
   objConstant:=[<ClassName> anything]
   pointList:= | (expr,expr) pointList | (expr,expr,expr) pointList 
               | VERTICALBAR pointList 
   valuesList:= | {expr,expr,expr} valuesList 
   */

#define NEWPARVAL(var) _MK_MALLOC(var,_MKParameterUnion,1)
  
#define STACKSIZE 100

static _MKParameterUnion *
parvalStack[STACKSIZE];

static short 
typeStack[STACKSIZE];

static _MKParameterUnion **pValPtr = NULL;
static short *fullStack = typeStack + STACKSIZE - 1;
static short *typePtr = NULL; 

static double *dataX = NULL;
static double *dataY = NULL;
static double *dataZ = NULL; /* Data state, used for envs waveTables. */
static double *dataXEnd = NULL;
static double *dataYEnd = NULL;
static double *dataZEnd = NULL;
static double *dataCurX = NULL;
static double *dataCurY = NULL;
static double *dataCurZ = NULL;

static void expandEnvBuf(double **startPtrPtr,double **curPtrPtr,
			 double **endPtrPtr)
{
#   define EXPANDAMOUNT 10    
    int newSize,curOffset;
    curOffset = *curPtrPtr - *startPtrPtr;
    newSize = *endPtrPtr - *startPtrPtr + EXPANDAMOUNT;
    _MK_REALLOC(*startPtrPtr,double,newSize);
    *endPtrPtr = *startPtrPtr + newSize - 1;
    *curPtrPtr = *startPtrPtr + curOffset;
#   undef EXPANDAMOUNT
}

static void 
  initExpressionParser(void)
{
    static BOOL beenHere = NO;
    _MKParameterUnion * *sp;  
    if (beenHere)
      return;
    beenHere = YES;
    for (sp = &parvalStack[STACKSIZE-1]; sp >= parvalStack; sp--)   
      NEWPARVAL(*sp);   /* The *sp is the pointer to the parameterUnion. */
}

static void
  initScanner(void)
{
    if (tokenBuf) 
      return;
    _MKLinkUnreferencedClasses([_ParName class],[_ScorefileVar class]);
    _MK_MALLOC(tokenBuf,char,INITIALMAXTOKENLENGTH);
    *tokenBuf = '\0';
    tokenEndBuf = INITIALMAXTOKENLENGTH + tokenBuf - 1; 
#   define INITDATASIZE 32
    _MK_MALLOC(dataX,double,INITDATASIZE);
    _MK_MALLOC(dataY,double,INITDATASIZE);
    _MK_MALLOC(dataZ,double,INITDATASIZE);
    dataXEnd = dataX + INITDATASIZE - 1;
    dataYEnd = dataY + INITDATASIZE - 1;
    dataZEnd = dataZ + INITDATASIZE - 1;
#   undef INITDATASIZE
}

static void 
stackPush(_MKParameterUnion *val, short type)
    /* Copies the value of val to stack as well as the type. */
{
    if (typePtr == fullStack)
      error(MK_sfBadExprErr);
    **(++pValPtr) = *val;
    *(++typePtr) = type;
}

static _MKParameterUnion * 
stackPop(short *typeAddr)
{
    if (typePtr < typeStack)
      error(MK_sfBadExprErr);
    *typeAddr = *typePtr--;
    return *pValPtr--;
}

static short
rvalue(_MKParameterUnion **valAddr,short type)
{
    short resultType;
    switch (type) {
      case _MK_typedVar:
      case _MK_untypedVar: 
	resultType = _MKSFVarInternalType((*valAddr)->symbol);
	*valAddr  = _MKSFVarRaw((*valAddr)->symbol);
	break;
      default:
	resultType = type;
    }
    return resultType;
}

/* Expression parsing: Special functions. */

static double 
ran(void)
{
    /* Returns a random number between 0 and 1. */
#   define   RANDOMMAX (double)((long)MAXINT)
    setstate(scoreRPtr->_ranState);
    return ((double)random()) / RANDOMMAX;
}

#define STATESIZEINBYTES 256

static void _setRanSeed(unsigned seed)
{
    initstate(seed,scoreRPtr->_ranState,STATESIZEINBYTES);
}

static char *initRan(void)
{
    char *stateArr;
    _MK_MALLOC(stateArr,char,STATESIZEINBYTES);
    initstate(1,stateArr,STATESIZEINBYTES);
    return stateArr;
}

static int getInt(void);

static void setSeed(void)
{
    MATCH(_MK_seed);
    _setRanSeed(getInt());
}

static void ranSeed(void)
{
    struct timeval tp;
    MATCH(_MK_ranSeed);
    gettimeofday(&tp,NULL);
    _setRanSeed((unsigned)tp.tv_usec);
}

/* Expression parsing: Envelopes */

enum restrictions {noRestriction,noWaveTab,noRecursiveDefines};
static enum restrictions restriction;

static void 
assign(void);	 /* Forward reference needed. */

static void
emit(short t);

static void
emitVar(short t,_MKParameterUnion * tokenVal);

#define SETDATAVAL(_ptr,_val) *++_ptr = _val

static id env(void)
    /* Parse SEG-style envelope and push it onto stack. It is assumed
       that lookahead == '(' when this function is entered. */
{
#   define NOSTICK -1
#   define INITPTR(_cur,_base) _cur = _base - 1
    /* The following only valid before SMOOTHING is set for the current point. */
#   define FIRSTSMOOTHING() (curPoint == 0)
    /* The following only valid after the current point has been processed. */
#   define NPTS() (((int)(dataCurX - dataX)) + 1)
    /* Pointers are incremented before assignment. */
    BOOL varyingSmoothing = NO;
    int curPoint = NOSTICK;
    int stickPoint = NOSTICK;
    double defaultSmoothing = MK_DEFAULTSMOOTHING;
    _MKParameterUnion tmpUnion;
    if (restriction == noRecursiveDefines)
      error(MK_sfNoNestDefineErr,_MKTokNameNoCheck(_MK_envelopeDecl));
    restriction = noRecursiveDefines;
    INITPTR(dataCurX,dataX); /* X point */
    INITPTR(dataCurY,dataY); /* Y point */
    INITPTR(dataCurZ,dataZ); /* Smoothing point */
    while (!match(']')) {
	if (match('(')) {
	    curPoint++;
	    assign();
	    emit(_MK_xEnvValue); 
	    if (!match(','))
	      error(MK_sfMissingStringErr,",");
	    assign();
	    emit(_MK_yEnvValue); 
	    if (lookahead != ')') { /* Smoothing, optional, is present. */
		if (!match(','))
		  error(MK_sfMissingStringErr,",");
		assign();
		emit(_MK_smoothingEnvValue); 
		if (FIRSTSMOOTHING())
		  defaultSmoothing = *dataCurZ;
		else varyingSmoothing = YES;
	    }
	    else {             /* Smoothing not present */
		if (FIRSTSMOOTHING()) {
		    SETDATAVAL(dataCurZ,defaultSmoothing); 
		}
		/* The first point is ignored but must be set in case
		   it's copied below. */
		else {
		    double x = *dataCurZ; 
		    SETDATAVAL(dataCurZ,x); /* Use previous value. */
		}
	    }
	    if (!match(')'))
	      error(MK_sfMissingStringErr,")");
	    match(',');       /* Optional comma. */
	}
	else if (match('|'))
	  stickPoint = curPoint;
	else error(MK_sfNotHereErr,curToken());
    }
    tmpUnion.symbol = [MKGetEnvelopeClass() new];
    [tmpUnion.symbol setPointCount:NPTS()
		     xArray:dataX
		     orSamplingPeriod:0.0 
		     yArray:dataY 
		     smoothingArray:(varyingSmoothing) ? dataZ : NULL 
		     orDefaultSmoothing:defaultSmoothing];
    if (stickPoint != NOSTICK)
      [tmpUnion.symbol setStickPoint:stickPoint];
    emitVar(MK_envelope,&tmpUnion);
    restriction = noRestriction;
    return tmpUnion.symbol;
}

static id samples(void)
{
    _MKParameterUnion tmpUnion;
    tmpUnion.symbol = [MKGetSamplesClass() new];
    [tmpUnion.symbol readSoundfile:tokenVal->sval]; 
    match(lookahead); /* Matches file name */
    emitVar(MK_waveTable,&tmpUnion);
    if (!match('}'))
      error(MK_sfMissingStringErr,"}");
    if (!match(']'))
      error(MK_sfMissingStringErr,"]");
    restriction = noRestriction;
    return tmpUnion.symbol;
}

static id partials(void)
{
#   define NOPOINT -1
#   define INITPTR(_cur,_base) _cur = _base - 1
    /* The following only valid before PHASE is set for the current point. */
#   define FIRSTPHASE() (curPoint == 0)
    /* The following only valid after the current point has been processed. */
#   define NPTS() (((int)(dataCurX - dataX)) + 1)
    int curPoint = NOPOINT;
    BOOL firstTime = YES;
    BOOL varyingPhase = NO;
    double phaseConstant = 0;
    _MKParameterUnion tmpUnion;
    /* Pointers are incremented before assignment. */
    INITPTR(dataCurX,dataX); /* HNum */
    INITPTR(dataCurY,dataY); /* Amp */
    INITPTR(dataCurZ,dataZ); /* Phase */
    while (!match(']')) {
	if (firstTime || match('{')) {
	    firstTime = NO;
	    assign();
	    curPoint++;
	    emit(_MK_hNumWaveValue); 
	    if (!match(','))
	      error(MK_sfMissingStringErr,",");
	    assign();
	    emit(_MK_ampWaveValue); 
	    if (lookahead != '}') { /* Phase, optional, is present. */
		if (!match(','))
		  error(MK_sfMissingStringErr,",");
		assign();
		emit(_MK_phaseWaveValue); 
		if (FIRSTPHASE())
		  phaseConstant = *dataCurZ;
		else varyingPhase = YES;
	    }
	    else {             /* Phase not present */
		if (FIRSTPHASE()) {
		    SETDATAVAL(dataCurZ,phaseConstant);
		}
		else {
		    double x = *dataCurZ;
		    SETDATAVAL(dataCurZ,x); /* Use previous value. */
		}
	    }
	    if (!match('}'))
	      error(MK_sfMissingStringErr,"}");
	    match(',');       /* Optional comma. */
	}
	else error(MK_sfNotHereErr,curToken());
    }
    tmpUnion.symbol = [MKGetPartialsClass() new];
    [tmpUnion.symbol setPartialCount:NPTS()
		     freqRatios:dataX
		     ampRatios:dataY
		     phases:(varyingPhase) ? dataZ : NULL 
		     orDefaultPhase:phaseConstant];
    emitVar(MK_waveTable,&tmpUnion);
    restriction = noRestriction;
    return tmpUnion.symbol;
}

static id wave(void)
    /* Parse wave table and push it onto stack. It is assumed
       that lookahead == '{' when this function is entered. */
{
    if (restriction == noRecursiveDefines)
      error(MK_sfBadDefineErr,_MKTokNameNoCheck(MK_waveTable));
    restriction = noRecursiveDefines;
    if (!match('{'))
      error(MK_sfMissingStringErr,"{");
    if (lookahead == MK_string) 
      return samples();
    else return partials();
}

static short expression(_MKParameterUnion *rtnValPtr);

static id obj(void) 
    /* Parse user-defined object. */ 
{
#   define WILD 0    
    _MKParameterUnion tmpUnion;
    id aClass;
    if (lookahead != _MK_undef)
      error(MK_sfMissingStringErr,_MKTokNameNoCheck(MK_object));
    if (!(aClass = _MK_FINDCLASS(tokenBuf)))
      error(MK_sfCantFindClass,tokenBuf);
    /* The following cases are in case the guy wrote an env or wave out
       as a normal object. SPECIAL-WAVE-ENV-CASE. */
    /*
      Note: It is forbidden to use the character "]" in the description.
      Also, it is forbidden to write a Note, Score or Part as a parameter value
      since these may use ']' in their description.
      */
    if (!strcmp("Envelope",tokenBuf)) {
	MATCH(_MK_undef);
	return env();
    }
    if ((!strcmp("Partials",tokenBuf)) ||
	(!strcmp("Samples",tokenBuf))) {
	MATCH(_MK_undef);
	return wave();
    }
    tmpUnion.symbol = [aClass new];
    /* No MATCH here, because we don't want to read it yet. */
    if (![tmpUnion.symbol respondsTo:@selector(readASCIIStream:)]) {
	for (;;) {
	    lookahead = NEXTCHAR();
	    if (lookahead == ']')
	      break;
	}
	error(MK_notScorefileObjectTypeErr,tokenBuf);
    }
    [tmpUnion.symbol readASCIIStream:scoreStream];
    MATCH(WILD);                          /* Needed to init parser again */
    if (!match(']'))
      error(MK_sfMissingStringErr,"]");
    emitVar(MK_object,&tmpUnion);
    restriction = noRestriction;
    return tmpUnion.symbol;
}

static void declErrCheck(const char *typeS,short declaredType);

static void namedDataDecl(_MKToken type)
    /* Parse named data declaration and push resultant data
       onto stack. It is assumed that lookahead is the name of 
       the new data when this function is entered. Type is the
       type of the declaration, i.e. _MK_envelopeDecl or _MK_waveTableDecl or
       _MK_objectDecl. */
{
    id dataObj;
    char * name;
    short tmpType;    
    _MKParameterUnion *tmpUnion;
    _MKToken dataToken = 
      (type == _MK_envelopeDecl) ? MK_envelope : 
	(type == _MK_waveTableDecl) ? MK_waveTable :
	  MK_object;
    declErrCheck(_MKTokName(dataToken),dataToken);
    name = _MKMakeStr(tokenBuf); /* Save name away. */
    MATCH(_MK_undef);         /* Name */
    matchInsert('=');
    switch (type) { 
      case _MK_envelopeDecl: 
	if (lookahead == '[') {
	    MATCH('[');
	    dataObj = env();
	    break;
	}
	else error(MK_sfBadInitErr,_MKTokNameNoCheck(_MK_envelopeDecl));
      case _MK_waveTableDecl:
	if (lookahead == '[') {
	    MATCH('[');
	    dataObj = wave();
	    break;
	}
	else error(MK_sfBadInitErr,_MKTokNameNoCheck(_MK_waveTableDecl));
      default:
	if (lookahead == _MK_objDefStart) {
//	    MATCH('['); /* We've already taken '[' from stream. */
	    dataObj = obj();
	    break;
	}
	else error(MK_sfBadInitErr,_MKTokNameNoCheck(_MK_objectDecl));
    }
    tmpUnion = stackPop(&tmpType);
    tmpType = rvalue(&tmpUnion,tmpType);
    /* The following code creates a unique new name for the Musickit table. 
       If the object is a duplicate, a name is generated of the form
       <oldName><low integer>. */
    addLocalSymbol(name,dataObj,dataToken);       
    MKNameObject(name,dataObj);
    /* Name may collide with other object names but that's ok! It'll get
       resolved when its written out. */
    NX_FREE(name); 
    emitVar(tmpType,tmpUnion);               /* Put data itself 
						back on stack. */
}

/* Expression parsing: Evaluation. */

static void
  unaryEval(_MKParameterUnion * val,short type,short *resultTypeAddr,
	    _MKParameterUnion * rtnVal,short op)
{
    *resultTypeAddr = rvalue(&val,type);
    switch (op) {
      case _MK_xEnvValue:
      case _MK_yEnvValue: 
      case _MK_smoothingEnvValue:
      case _MK_hNumWaveValue:
      case _MK_ampWaveValue:
      case _MK_phaseWaveValue:
	switch (*resultTypeAddr) {
	  case MK_int:
	    rtnVal->rval = (double)val->ival;
	    break;
	  case MK_string: {
	      rtnVal->rval = _MKStringToDouble(val->sval);
	      break;
	  }
	  case MK_double:
	    rtnVal->rval = val->rval;
	    break;
	  default:
	    error(MK_sfBadExprErr);
	}
	switch (op) {
	  case _MK_xEnvValue:
#           define FIRSTDATAX() (dataCurX < dataX) 
	    if ((!FIRSTDATAX()) && (*dataCurX >= rtnVal->rval))
	      error(MK_sfOutOfOrderErr,_MKTokNameNoCheck(_MK_xEnvValue));
	    /* no break here */
	  case _MK_hNumWaveValue:
	    if (dataCurX >= dataXEnd) {
		expandEnvBuf(&dataX,&dataCurX,&dataXEnd);
		expandEnvBuf(&dataY,&dataCurY,&dataYEnd);
		expandEnvBuf(&dataZ,&dataCurZ,&dataZEnd);
	    }
	    SETDATAVAL(dataCurX,rtnVal->rval);
	    break;
	  case _MK_ampWaveValue:
	  case _MK_yEnvValue:
	    SETDATAVAL(dataCurY,rtnVal->rval);
	    break;
	  case _MK_phaseWaveValue:
	  case _MK_smoothingEnvValue:
	    SETDATAVAL(dataCurZ,rtnVal->rval);
	    break;
	}
	break;
      case _MK_uMinus:   
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = -val->rval;
	    break;
	  case MK_int:
	    rtnVal->ival = -val->ival;
	    break;
	  default:
	    error(MK_sfNumberErr);
	}
	break;
      case _MK_dB:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = MKdB(val->rval);
	    break;
	  case MK_int: {
	      double x;
	      *resultTypeAddr = INT(MK_double);
	      x = (double) val->ival; /* This was needed due to gcc bug. */
	      rtnVal->rval = MKdB(x);
	      break;
	  }
	  default:
	    error(MK_sfNumberErr);
	}
	break;
    }
}
 
static void evalAssign(_MKParameterUnion *val1,_MKParameterUnion *val2,
		       short type1,short type2,short *resultTypeAddr,short op,
		       _MKParameterUnion *rtnVal)
{
    short err;
    type2 = rvalue(&val2,type2);        /* Dereference if needed. */
    if (type1 == INT(_MK_untypedVar))
      *resultTypeAddr = type2;
    else if (type1 == INT(_MK_typedVar) )
      *resultTypeAddr = _MKSFVarInternalType(val1->symbol);
    else error(MK_sfBadAssignErr,_MKTokName(type1));
    switch (type2) {
      case MK_envelope:
	err = _MKSetEnvSFVar(val1->symbol,val2->symbol);
	break;
      case MK_object:
	err = _MKSetObjSFVar(val1->symbol,val2->symbol);
	break;
      case MK_waveTable:
	err = _MKSetWaveSFVar(val1->symbol,val2->symbol);
	break;
      case MK_double:
	err = _MKSetDoubleSFVar(val1->symbol,val2->rval);
	break;
      case MK_int:
	err = _MKSetIntSFVar(val1->symbol,val2->ival);
	break;
      case MK_string:
	err = _MKSetStringSFVar(val1->symbol,val2->sval); 
	break;
      default:
	err = NO;
	break;
    }
    if (err) {
	if (err == (short)MK_sfReadOnlyErr)
	  error(MK_sfReadOnlyErr,[val1->symbol name]);
	else
	  error(MK_sfTypeConversionErr);
    }
    *rtnVal = *(_MKSFVarRaw(val1->symbol));
}

static void lookupEnv(_MKParameterUnion *theEnv,short theEnvType,
		      _MKParameterUnion *lookup,short lookupType,short op,
		      _MKParameterUnion *rtnVal)
{ 
    double lookupVal;
    lookupType = rvalue(&lookup,lookupType);  /* Dereference if needed. */
    theEnvType = rvalue(&theEnv,theEnvType);  /* Dereference if needed. */
    if (theEnvType != MK_envelope)
      error(MK_sfBadExprErr);
    switch (lookupType) {
      case MK_double:
	lookupVal = lookup->rval;
	break;
      case MK_int:
	lookupVal = (double)lookup->ival;
	break;
      case MK_string:
	lookupVal = (double)_MKStringToDouble(lookup->sval);
	break;
      default:
	error(MK_sfNumberErr);
    }
    rtnVal->rval = [theEnv->symbol lookupYForX:lookupVal];
    if (MKIsNoDVal(rtnVal->rval))
      error(MK_sfBoundsErr);
}

static void
eval(_MKParameterUnion *val1,_MKParameterUnion *val2,short type1,short type2,
     short *resultTypeAddr,short op,_MKParameterUnion *rtnVal)
{
    _MKParameterUnion v1,v2;
    /* Do type conversion. */
    type1 = rvalue(&val1,type1);  /* Dereference if needed. */
    type2 = rvalue(&val2,type2); 
    v1 = *val1;
    v2 = *val2;
    if (type1 == type2)
      *resultTypeAddr = type1;
    else if (type1 == INT(MK_double) && type2 == INT(MK_int)) {
	*resultTypeAddr = INT(MK_double);
	v2.rval = (double) v2.ival;
    }
    else if (type2 == INT(MK_double) && type1 == INT(MK_int)) {
	*resultTypeAddr = INT(MK_double);
	v1.rval = (double) v1.ival;
    }
    switch (op) {
      case '&':
	break;
      case _MK_EQU:
      case _MK_NEQ:
	if (type2 == INT(MK_string) && type1 == INT(MK_string))
	  break;
	/* else no break. I.e. for equality between strings, we just 
	   compare characters. Otherwise, we do the "usual conversion"
	   of strings to numbers */
      default:
	if (type2 == INT(MK_string)) {
	    if (type1 == INT(MK_double)) {
		*resultTypeAddr = INT(MK_double);
		v2.rval =  _MKStringToDouble(v2.sval);
		if (MKIsNoDVal(v2.rval))
		  error(MK_sfNumberErr);
	    }
	    else if (type1 == INT(MK_int)) {
		*resultTypeAddr = INT(MK_int);
		v2.ival =  _MKStringToInt(v2.sval);
		if (v2.ival == MAXINT)
		  error(MK_sfNumberErr);
	    }
	    else if (type1 != INT(MK_string))
 	      error(MK_sfStringErr,_MKTokName(type1));
	}
	else if (type1 == INT(MK_string)) {
	    if (type2 == INT(MK_double)) {
		*resultTypeAddr = INT(MK_double);
		v1.rval =  _MKStringToDouble(v1.sval);
		if (MKIsNoDVal(v1.rval))
		  error(MK_sfNumberErr);
	    }
	    else if (type2 == INT(MK_int)) {
		*resultTypeAddr = INT(MK_int);
		v1.ival =  _MKStringToInt(v1.sval);
		if (v1.ival == MAXINT)
		  error(MK_sfNumberErr);
	    }
	    else if (type2!= INT(MK_string))
	      error(MK_sfStringErr,_MKTokName(type2));
	}
    }         /* End of strings to numbers block. */
    switch (op) {
      case '+':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval + v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival + v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_GEQ:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval >= v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival >= v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_LEQ:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval <= v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival <= v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '>':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval > v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival > v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '<':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval < v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival < v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_EQU:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval == v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival == v2.ival;
	    break;
	  case MK_string:
	    *resultTypeAddr = INT(MK_int);
	    rtnVal->ival = (strcmp(v1.sval,v2.sval) == 0) ? 1 : 0;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_NEQ:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval != v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival != v2.ival;
	    break;
	  case MK_string:
	    *resultTypeAddr = INT(MK_int);
	    rtnVal->ival = (strcmp(v1.sval,v2.sval) == 0) ? 0 : 1;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_AND:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval && v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival && v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case _MK_OR:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval || v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival || v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '^':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = pow((double)v1.rval,(double)v2.rval);
	    break;
	  case MK_int:
	    rtnVal->ival = (int) pow((double)v1.ival ,(double) v2.ival);
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case SEMITONEOP:
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval * pow(2.0,v2.rval/12.0);
	    break;
	  case MK_int:
	    rtnVal->ival = (int)((double)v1.ival * 
				 pow(2.0,((double)v2.ival)/12.0));
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '-':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval - v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival - v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '*':
	switch (*resultTypeAddr) {
	  case MK_double:
	    rtnVal->rval = v1.rval * v2.rval;
	    break;
	  case MK_int:
	    rtnVal->ival = v1.ival * v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '/':
	switch (*resultTypeAddr) {
	  case MK_double:
	    if (v2.rval == 0.0) 
	      error(MK_sfArithErr);
	    rtnVal->rval = v1.rval / v2.rval;
	    break;
	  case MK_int:            
	    if (v2.ival == 0) 
	      error(MK_sfArithErr);
	    if (v1.ival % v2.ival) {
		rtnVal->rval = ((double) v1.ival) / ((double)v2.ival);
		*resultTypeAddr = INT(MK_double);
	    }
	    else rtnVal->ival = v1.ival / v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '%':
	switch (*resultTypeAddr) {
	  case MK_double:           /* All mod of doubles returns integer. */
	    if (v2.rval == 0.0) 
	      error(MK_sfArithErr);
	    rtnVal->ival = ((int)v1.rval) % ((int)v2.rval);
	    *resultTypeAddr = INT(MK_int);
	    break;
	  case MK_int:            
	    if (v2.ival == 0) 
	      error(MK_sfArithErr);
	    rtnVal->ival = v1.ival % v2.ival;
	    break;
	  default:
 	    error(MK_sfNumberErr);
	} break;
      case '&': {
	  if (type1 == INT(MK_int))
	    v1.sval = _MKIntToStringNoCopy(v1.ival);
	  else if (type1 == INT(MK_double))
	    v1.sval = _MKDoubleToStringNoCopy(v1.rval);
	  else if (type1 != INT(MK_string))
	    error(MK_sfStringErr);
	  if (type2 == INT(MK_int))
	    v2.sval = _MKIntToStringNoCopy(v2.ival);
	  else if (type2 == INT(MK_double))
	    v2.sval = _MKDoubleToStringNoCopy(v2.rval);
	  else if (type2 != INT(MK_string))
	    error(MK_sfStringErr,_MKTokName(type2));
	  *resultTypeAddr = INT(MK_string);
	  rtnVal->sval = _MKMakeStrcat(v1.sval,v2.sval);
	  break;
      }
    }
}

/* Expression parsing: Emit. */

static void
emitVar(short t,_MKParameterUnion * tokenVal)
{
    switch(t) {
      case MK_double: case MK_string: case MK_int: case MK_envelope:
      case _MK_typedVar: case _MK_untypedVar:  case MK_object:
      case MK_waveTable:
	stackPush(tokenVal,t);
	break;
      default:
	error(MK_sfBadExprErr);
	break;
    }
}

static void
emit(short t)
{
    _MKParameterUnion *val1,*val2;
    _MKParameterUnion valResult;
    short type1,type2,resultType;
    val2 = stackPop(&type2);
    switch(t) {
      case '+': case '-': case '*': case '/': case '&': case '^': case '%':
      case _MK_GEQ: case _MK_LEQ: case _MK_EQU: case _MK_NEQ:
      case '>': case '<':
      case _MK_AND: case _MK_OR:
      case SEMITONEOP:
	val1 = stackPop(&type1);
	eval(val1,val2,type1,type2,&resultType,t,&valResult);
	stackPush(&valResult, resultType);
	break;
      case '=':
	val1 = stackPop(&type1);
	evalAssign(val1,val2,type1,type2,&resultType,t,&valResult);
	stackPush(&valResult, resultType);
	break;
      case _MK_lookupEnv:
	val1 = stackPop(&type1);
	lookupEnv(val1,type1,val2,type2,t,&valResult);
	stackPush(&valResult, MK_double);
	break;
      case _MK_hNumWaveValue:
      case _MK_ampWaveValue:
      case _MK_phaseWaveValue:
      case _MK_xEnvValue:
      case _MK_yEnvValue:
      case _MK_smoothingEnvValue:
	unaryEval(val2,type2,&resultType,&valResult,t);
	/* These represent 'end of expression' so we don't push the 
	   value back on the stack. */
	break;
      case _MK_uMinus:   
      case _MK_dB:
	unaryEval(val2,type2,&resultType,&valResult,t);
	stackPush(&valResult,resultType);
	break;
      default:
	error(MK_sfBadExprErr,_MKTokName(t));
	break;
    }
}

/* Expression parsing: Syntax. */

static void
factor(void) 
{ 
    switch(lookahead) {
      case '(': 	
	MATCH('('); assign(); 
	if (!match(')'))
	  error(MK_sfMissingStringErr,")");
	break; 	
      case '[':                  
	MATCH('[');
	if (lookahead == '(')   /* unnamed seg envelope constant */
	  env();
	else if (lookahead == '{') /* unnamed wave table constant */
	  wave();
	else 
	  obj();
	break;
      case _MK_waveTableDecl:     
      case _MK_objectDecl:     
      case _MK_envelopeDecl:  {
	  static void namedDataDecl(); /* forward ref. */
	  short type;
	  type = lookahead;
	  MATCH(lookahead);
	  namedDataDecl(type);
	  break;
      }
      case MK_double: 
      case MK_string: 
      case MK_int: 
      case _MK_typedVar: 
      case _MK_untypedVar: 
      case MK_object:
      case MK_envelope:               
      case MK_waveTable:
	emitVar(lookahead, tokenVal); 
	MATCH(lookahead);       
	break;
      case _MK_time:                 /* Time is special. */
	tokenVal->rval = scoreRPtr->timeTag; 
	emitVar(MK_double, tokenVal);
	MATCH(lookahead);
	break;
      case _MK_ran:               /* Ran is treated like a constant */
	tokenVal->rval = ran();
	emitVar(MK_double, tokenVal);
	MATCH(lookahead);
	break;
      case MK_localControlModeOn:
      case MK_localControlModeOff:
      case MK_allNotesOff:
      case MK_omniModeOff:
      case MK_omniModeOn:
      case MK_monoMode:
      case MK_polyMode:
      case MK_tuneRequest:
      case MK_sysClock:
      case MK_sysStart:
      case MK_sysContinue:
      case MK_sysStop:
      case MK_sysActiveSensing:
      case MK_sysReset:
	tokenVal->ival = INT(lookahead);
	emitVar(MK_int,tokenVal); 
	MATCH(lookahead);
	break;
      case _MK_undef:
	error(MK_sfUndeclaredErr,_MKTokNameNoCheck(_MK_typedVar),tokenBuf);
      default: 	 
	error(MK_sfBadExprErr);
    } 
}

static void
unary(void)
    /* To omit one level of recursion, the prefix and postfix operators
       are on the same precedence level. The way I used to do this,
       wasn't right because -4dB evaluated to -(4dB), not (-4)dB, as it
       should. The current implementation is not general. It doesn't handle
       mixing unary operators nor multiple unary plus. 
       E.g. -+4, +-4, and ++4 don't work. But -4dB works correctly, as does
       ---4dB. */
{  
    switch (lookahead) {
      case '-': {
	  MATCH('-');
	  if (lookahead != '-') {
	      factor();
	      emit(_MK_uMinus);
	  }
	  else {
	      int minusCount;    /* Handle multiple '-' */
	      for (minusCount = 1; match('-'); minusCount++)
		;
	      factor();
	      if (minusCount % 2) 
		emit(_MK_uMinus);
	  }
	  break;
      }
      case '+':
	while (match('+'))
	  ;
      default:
	factor();
	break;
    }
    while (match(_MK_dB))
      emit(_MK_dB);
}

static void 
expon(void)
{  
    short t;
    unary();     
    for (;;)
      switch (lookahead) {
	case SEMITONEOP:
	case '^': 
	  t = lookahead;
	  MATCH(lookahead); unary(); emit(t);
	  continue;
	default:
	  return;
      }
}

static void
term(void)
{  
    short t;
    expon();
    for (;;)
      switch (lookahead) {
	case '*': case '/': case '%':
	  t = lookahead;
	  MATCH(lookahead); expon(); emit(t);
	  continue;
	default:
	  return;
      }
}

static void
expr(void)
{
    short t;
    term();
    for(;;) 
      switch (lookahead) {
	case '+': case '-':
	  t = lookahead;
	  MATCH(lookahead); term(); emit(t);
	  continue;
	default: 
	  return;
      }
}

static void
catOrFun(void)
{
    short t;
    expr();
    for(;;) 
      switch (lookahead) {
	case '&': case '@':
	  t = lookahead;
	  MATCH(lookahead); expr(); emit(t);
	  continue;
	default: 
	  return;
      }
}

static void
comparisonEval(void)
{
    short t;
    catOrFun();
    for(;;) 
      switch (lookahead) {
	case _MK_LEQ: case _MK_GEQ: case _MK_EQU: case _MK_NEQ: 
	case '<': case '>':
	  t = lookahead;
	  MATCH(lookahead); catOrFun(); emit(t);
	  continue;
	default: 
	  return;
      }
}

static void 
booleanEval(void)
{
    short t;
    comparisonEval();
    for(;;) 
      switch (lookahead) {
	case _MK_AND: case _MK_OR: 
	  t = lookahead;
	  MATCH(lookahead); comparisonEval(); emit(t);
	  continue;
	default: 
	  return;
      }
}

static void 
assign(void)
{
    booleanEval();                /* This matches assign to expr
				     and relies on eval to catch it. */
    if (lookahead == '=')
    {
	MATCH('=');
	assign();      /* Right associativity.*/
	emit('=');
    }
}

/* Expression parsing: Interface to statement and declaration level. */


#define EMPTYSTACK()      typePtr = typeStack-1; pValPtr = parvalStack-1
#define STACKERRORCHECK() if (typePtr != typeStack-1) error(MK_sfBadExprErr)

static short
expression(_MKParameterUnion *rtnValPtr)
    /* Returns type of expression. This is a top-level parser function. */
{
    short rtnType;
    _MKParameterUnion *p;
    EMPTYSTACK();
    assign();
    p = stackPop(&rtnType);
    STACKERRORCHECK();
    rtnType = rvalue(&p,rtnType);
    *rtnValPtr = *p;
    return rtnType;
}    

static double
getDouble(void)
    /* Returns double. This is a top-level parser function. */
{
    _MKParameterUnion val;
    switch (expression(&val)) {
      case MK_int:
	val.rval = val.ival;
	break;
      case MK_string: {
	  val.rval = _MKStringToDouble(val.sval);
	  break;
      }
      case MK_double:
	break;
      default:
	error(MK_sfNumberErr);
    }
    return val.rval;
}

static int 
getInt(void)
    /* Returns int, truncated. This is a top-level parser function. */
{
    _MKParameterUnion val;
    switch (expression(&val)) {
      case MK_double:
	val.ival = val.rval;
	break;
      case MK_string: {
	  val.ival = _MKStringToInt(val.sval);
	  break;
      }
      case MK_int:
	break;
      default:
	error(MK_sfNumberErr);
    }
    return val.ival;
}

static int 
getBOOL(void)
    /* Returns int, truncated. This is a top-level parser function. */
{
    _MKParameterUnion val;
    switch (expression(&val)) {
      case MK_double:
	val.ival = (val.rval == 0) ? 0 : 1;
	break;
      case MK_string: {
	  val.ival = (_MKStringToInt(val.sval) == 0) ? 0 : 1;
	  break;
      }
      case MK_int:
	val.ival = (val.ival == 0) ? 0 : 1;
	break;
      default:
	error(MK_sfNumberErr);
    }
    return val.ival;
}


/* Error handling. ------------------------------------------------------ */

#define BINARY(_p) scoreRPtr->_binary

static BOOL shutUp = NO;

static char *
_errorMsg(MKErrno errCode,char *ap)
    /* Subsidiary error string function. Gets error and appends line number 
       info */
{
    char * s;
    const char * fmt = _MKGetErrStr(errCode);
    if (shutUp) 
      return NULL;
    s = _MKErrBuf();
    if (errCode == MK_sfNonScorefileErr)  /* This one's special */
      *s = '\0';
    else if (BINARY(scoreRPtr)) /* Binary files have no 'line number' */
      sprintf(s,"%s: ",parsePtr->_name);
    else 
      sprintf(s,"%s, pg %d, line %d: ", parsePtr->_name,
	      parsePtr->_pageNo, parsePtr->_lineNo);
    vsprintf(s + strlen(s),fmt,ap);
    return s;
}

static int errAbort = 10;

void MKSetScorefileParseErrorAbort(int cnt)
    /* Sets the number of parser errors to abort on. To never abort,
       pass MAXINT as the argument. To abort on the first error, pass 1 as the
       argument. The default is 10. */
{
    if (cnt < 1) 
      cnt = 1;
    errAbort = cnt;
}

#define TOOMANYERRORS (scoreRPtr->_errCount >= errAbort)

static char *
_warning(BOOL potentiallyFatal,MKErrno errCode,char *ap)
    /* Create error string with context */
{
    register STRTYPE *p,*q;
    int i,msgLen;
    char *errS,*s;
    if (shutUp) 
      return NULL;    
    errS = _errorMsg(errCode,ap);
    s = errS;                /* Keep pointer to buffer */
    if (BINARY(scoreRPtr))
      return s;
    msgLen = strlen(errS);
    errS = errS + msgLen;
//  *errS++ = '\n';
    /* Search backwards for most recent 'newline'. */
    for (p = errorLoc; p > scoreStream->buf_base;)
      if (*p-- == '\n') 
	break;
    if (p < scoreStream->buf_base)
      p = scoreStream->buf_base;
#   define LOOKAHEAD 10 
#   define ABORTSIZE 34 /* In case this error puts us over threshold. */
#   define EXTRA (8 + ABORTSIZE) /* 3 '/n's, one NULL and 4 for good luck */ 
    if (((scoreStream->buf_ptr - p) * 2 + msgLen + LOOKAHEAD + EXTRA) >= 
	_MK_ERRLEN) 
      p = scoreStream->buf_ptr + 
	(msgLen + LOOKAHEAD + EXTRA - _MK_ERRLEN)/2;
    if ((p == scoreStream->buf_base) && (*p != '\n'))
      sprintf(errS++,"%c",'\n');
    else ++p; /* We're one behind (see above loop) */
    q = p;
    while (p < errorLoc)
      sprintf(errS++,"%c",*p++);
    sprintf(errS++,"\n");
    for (p = q; p < errorLoc; p++)    /* Formating. */
      sprintf(errS++,"%c",' ');
    /* Write next LOOKAHEAD characters of buffer */
    for (i = MIN(scoreStream->buf_ptr - errorLoc + scoreStream->buf_left,
		 LOOKAHEAD); 
	 ((i > 0) && (*p != '\n')); 
	 i--) 
      sprintf(errS++,"%c",*p++);
    if (potentiallyFatal)
      if (scoreRPtr->_errCount != MAXINT)
	if (++scoreRPtr->_errCount >= errAbort) {
	    sprintf(errS,"\n%s",_MKGetErrStr(MK_sfTooManyErrorsErr));
	}
    return s;
}

static void
errorMsg(MKErrno errCode,...)
{
    /* Write message with niether long jump nor context. 
       Calling sequence like printf. */
    va_list ap;
    va_start(ap,errCode);
    MKError(_errorMsg(errCode,ap));
    va_end(ap);
}

static void
warning(MKErrno errCode,...)
{
    /* Write warning without long jump. Calling sequence like printf. */
    va_list ap;
    va_start(ap,errCode);
    MKError(_warning(NO,errCode,ap));
    va_end(ap);
}

static void
error(MKErrno errCode,...)
{
    /* Try and recover from runtime error. Calling sequence like printf. */
    va_list ap;
    va_start(ap,errCode);
    MKError(_warning(YES,errCode,ap));
    va_end(ap);
    if (TOOMANYERRORS || BINARY(scoreRPtr))  
      /* Can't recover gracefully from binary errors */
      longjmp(begin,fatalErrorLongjmp);    /* Go into hyperspace */
    shutUp = YES;
    while (lookahead != ';') /* Look for semi-colon */
      MATCH(lookahead);
    shutUp = NO;
    restriction = noRestriction;
    longjmp(begin,errorLongjmp);
}

/* Saving and restoring state. ---------------------------------------- */

static void loadFromStruct(register _MKScoreInStruct * scoreP)
{
    register parseStateStruct * parseP;
    parseP = (parseStateStruct *) scoreP->_parsePtr;
    if (!parseP) {
	parsePtr = NULL;
	return;
    }
    parsePtr = parseP;
    scoreStream = parseP->_stream;
    lookahead = parseP->_lookahead;
    tokenVal = &parseP->_tokenVal;
}

static void storeInStruct(register _MKScoreInStruct * scoreP,
			  register parseStateStruct * parseP)
{
    scoreP->_parsePtr = (void *) parseP;
    if (!parseP)
      return;
    parseP->_lookahead = lookahead;
    parseP->_stream = scoreStream;
    return;
}

#define TOPLEVELFILE -1

static parseStateStruct * 
initParsePtr(NXStream *aStream,int fd,char *name)
{
    /* Do all kinds of things to get ready for the first parse. */
    int c; /* ungetc takes an int */
    _MK_MALLOC(parsePtr,parseStateStruct,1);
    parsePtr->_stream = aStream;
    parsePtr->_fd = fd;
    parsePtr->_backwardsLink = NULL;
    tokenVal = &(parsePtr->_tokenVal);
    parsePtr->_lineNo = 1;
    parsePtr->_pageNo = 1;
    parsePtr->_name = _MKMakeStr(name);
    scoreStream = aStream;
    c = NXGetc(aStream);
    if ((fd != TOPLEVELFILE) && ((char)c == FIRST_CHAR_SCOREMAGIC)) {
	errorMsg(MK_sfNotHereErr,".playscore file");
	/* Can't mix ASCII and binary formats */
	NXUngetc(aStream);
	NX_FREE(parsePtr->_name);
	NX_FREE(parsePtr);
	return NULL;
    }
    /* Note: even if we didn't need to check the first char, we'd need
       to do a getc/ungetc to initialize the stream for error reporting. */
    NXUngetc(aStream);
    lookahead = ';';   /* So that first statement is gobbled correctly */
    tokenPtr = NULL;
    return parsePtr;
}

#define ISINCLUDEFILE(_parseP) (_parseP->_backwardsLink)

static parseStateStruct * 
initScoreParsePtr(char * scorefilePath)
{
    /* Used for include files */
    NXStream *aStream;
    int fd;
    /* FIXME Or do something smart with dates here */
    aStream = _MKOpenFileStream(scorefilePath,&fd,NX_READONLY,
			       _MK_BINARYSCOREFILEEXT,NO);
    if (!aStream)
      aStream = _MKOpenFileStream(scorefilePath,&fd,NX_READONLY,
				  _MK_SCOREFILEEXT,NO);
    if (!aStream)
      return NULL;
    parsePtr = initParsePtr(aStream,fd,scorefilePath);
    if (!parsePtr)
      return NULL;
    return parsePtr;
}

static parseStateStruct * 
popFileStack(void)
{
    /* Close file, free this parsePtr record and pop file stack. */
    if (!parsePtr)
      return NULL;
    if (ISINCLUDEFILE(parsePtr)) { /* Don't close top level file. */
	NXClose(parsePtr->_stream);
	close(parsePtr->_fd);
    }
    NX_FREE(parsePtr->_name);
    scoreRPtr->_parsePtr = (void *)parsePtr->_backwardsLink;   
    /* Pop file stack.*/
    NX_FREE(parsePtr);
    loadFromStruct(scoreRPtr);
    return parsePtr;
}

_MKScoreInStruct * _MKFinishScoreIn(_MKScoreInStruct *scorefileRPtr)
{
    /* Finish and free up score file input structure. If the parse is currently
       in an include file, closes all files on the stack, except the
       top level file.  Returns NULL. */
    scoreRPtr = scorefileRPtr;
    if (!scoreRPtr) 
      return NULL;
    parsePtr = (parseStateStruct *) scoreRPtr->_parsePtr;
    while (parsePtr)
      parsePtr = popFileStack();
    _MKFreeScorefileTable(scoreRPtr->_symbolTable);
    [scoreRPtr->_noteTagTable free];
    [scoreRPtr->_aNote free];
    if (scoreRPtr->_freeStream) {
	NXFlush(scoreRPtr->printStream);
	NXClose(scoreRPtr->printStream);
    }
    NX_FREE(scoreRPtr);
    return NULL;
}

/* Binary scorefile primitives. ------------------------------------ */

#define abortBinary() longjmp(begin,eofLongjmp)

#define CHECKEOS() if (NXAtEOS(parsePtr->_stream)) abortBinary()

static short getBinaryShort(void)
{
    short rtn;
    NXRead(parsePtr->_stream, &rtn, sizeof(short));
    CHECKEOS();
    return rtn;
}

static int getBinaryInt(void)
{
    int rtn;
    NXRead(parsePtr->_stream, &rtn, sizeof(int));
    CHECKEOS();
    return rtn;
}

static double getBinaryDouble(void)
{
    double rtn;
    NXRead(parsePtr->_stream, &rtn, sizeof(double));
    CHECKEOS();
    return rtn;
}

static float getBinaryFloat(void)
{
    float rtn;
    NXRead(parsePtr->_stream, &rtn, sizeof(float));
    CHECKEOS();
    return rtn;
}

static char *getBinaryString(BOOL install)
    /* If install is NO, we're running "fast and dangerous" */
{
    register int c = (int)NXGetc(parsePtr->_stream);
    NEWTOKEN(c);
    do {
	CHECKEOS();
    } while ( (c=NEXTTCHAR()) != '\0'); 
    ENDTOKEN();
    if (install)
      tokenVal->sval = (char *)NXUniqueString(tokenBuf); 
    /* We can't anticipate where the damn thing will end up so we can't
       do garbage collection. So we make it unique to avoid accumulating
       garbage. At some point in the future, might want to really move
       to unique strings system-wide. This makes string compares faster,
       for example. */
    return tokenVal->sval = tokenBuf;
}

static id nullObject = nil;

static id getNullObject(void)
    /* Special null object, since nil can't be used in Lists */ 
{
    if (!nullObject)
      nullObject = [Object new];
    return nullObject;
}

static id getBinaryIndexedObject(void)
    /* Get index and indexed object */
{
    register short index = getBinaryShort();
    tokenVal->symbol = 
      NX_ADDRESS(scoreRPtr->_binaryIndexedObjects)[index-1];
    if (tokenVal->symbol == nullObject)
      tokenVal->symbol = nil;
    return tokenVal->symbol;
}

#if 0
static id getBinaryIndexedObjectForIndex(short index)
    /* Same as above but index is passed in */
{
    tokenVal->symbol = 
      NX_ADDRESS(scoreRPtr->_binaryIndexedObjects)[index-1];
    if (tokenVal->symbol == nullObject)
      tokenVal->symbol = nil;
    return tokenVal->symbol;
}
#endif

static short getBinarySymbol(void)
    /* Read token and do lookup in symbol table. Set tokenVal as 
       a side-effect */
{
    unsigned short tok;
    register int c = (int)NXGetc(parsePtr->_stream);
    CHECKEOS();
    NEWTOKEN(c);
    while ((c = NEXTTCHAR()) != '\0')
      CHECKEOS();
    ENDTOKEN();
    tokenVal->symbol = 
      _MKNameTableGetObjectForName(scoreRPtr->_symbolTable,tokenBuf,nil,
				   &tok);
    if (tokenVal->symbol || tok)        /* It's an object or keyword */
      lookahead = INT(tok);
    else lookahead = INT(_MK_undef);
    return lookahead;
}

static double getBinaryVarValue(void)
{
    register _MKParameter *par;
    getBinarySymbol();
    par = _MKSFVarGetParameter(tokenVal->symbol);
    return _MKParAsDouble(par);
}

static id getBinaryWaveTableDecl(void)
{
    register int c;
    id rtn;
    c = NXGetc(parsePtr->_stream);
    CHECKEOS();
    if (c == '\0') { /* Partials */
#       define INITPTR(_cur,_base) _cur = _base - 1
#       define FIRSTPHASE() (curPoint == 0)
#       define NPTS() (((int)(dataCurX - dataX)) + 1)
	int curPoint = NOPOINT;
	BOOL varyingPhase = NO;
	double phaseConstant = 0;
	/* Pointers are incremented before assignment. */
	INITPTR(dataCurX,dataX); /* HNum */
	INITPTR(dataCurY,dataY); /* Amp */
	INITPTR(dataCurZ,dataZ); /* Phase */
	for (;;) {
	    curPoint++;
	    c = NXGetc(parsePtr->_stream);
	    CHECKEOS();
	    if (c == '\0')
	      break;
	    if (dataCurX >= dataXEnd) {
		expandEnvBuf(&dataX,&dataCurX,&dataXEnd);
		expandEnvBuf(&dataY,&dataCurY,&dataYEnd);
		expandEnvBuf(&dataZ,&dataCurZ,&dataZEnd);
	    }
	    SETDATAVAL(dataCurX,getBinaryDouble());
	    SETDATAVAL(dataCurY,getBinaryDouble());
	    if (c == '\3') {
		SETDATAVAL(dataCurZ,getBinaryDouble());
		if (FIRSTPHASE())
		  phaseConstant = *dataCurZ;
		else varyingPhase = YES;
	    }
	    else if (FIRSTPHASE())
	      SETDATAVAL(dataCurZ,phaseConstant);
	    else {
		double x = *dataCurZ;
		SETDATAVAL(dataCurZ,x); /* Use previous value. */
	    }
	}
	rtn = [MKGetPartialsClass() new];
	[rtn setPartialCount:NPTS()
       freqRatios:dataX
       ampRatios:dataY
       phases:(varyingPhase) ? dataZ : NULL 
       orDefaultPhase:phaseConstant];
    }
    else { /* Samples */
	rtn = [MKGetSamplesClass() new];
	[rtn readSoundfile:getBinaryString(NO)];
    }
    return rtn;
}

static id getBinaryEnvelopeDecl(void)
{
    register int c;
    id rtn;
#   define NOSTICK -1
#   define INITPTR(_cur,_base) _cur = _base - 1
    /* The following only valid before SMOOTHING is set for the current point. */
#   define FIRSTSMOOTHING() (curPoint == 0)
    /* The following only valid after the current point has been processed. */
    BOOL varyingSmoothing = NO;
    int curPoint = NOSTICK;
    int stickPoint = NOSTICK;
    double defaultSmoothing = MK_DEFAULTSMOOTHING;
    /* Pointers are incremented before assignment. */
    INITPTR(dataCurX,dataX); /* HNum */
    INITPTR(dataCurY,dataY); /* Amp */
    INITPTR(dataCurZ,dataZ); /* Phase */
    for (;;) {
	c = NXGetc(parsePtr->_stream);
	CHECKEOS();
	if (c == '\1') {
	    stickPoint = curPoint;
	    continue;
	}
	if (c == '\0')
	  break;
	curPoint++;
	if (dataCurX >= dataXEnd) {
	    expandEnvBuf(&dataX,&dataCurX,&dataXEnd);
	    expandEnvBuf(&dataY,&dataCurY,&dataYEnd);
	    expandEnvBuf(&dataZ,&dataCurZ,&dataZEnd);
	}
	SETDATAVAL(dataCurX,getBinaryDouble());
	SETDATAVAL(dataCurY,getBinaryDouble());
	if (c == '\3') {
	    SETDATAVAL(dataCurZ,getBinaryDouble());
	    if (FIRSTSMOOTHING())
	      defaultSmoothing = *dataCurZ;
	    else varyingSmoothing = YES;
	}
	else if (FIRSTSMOOTHING())
	  SETDATAVAL(dataCurZ,defaultSmoothing);
	else {
	    double x = *dataCurZ;
	    SETDATAVAL(dataCurZ,x); /* Use previous value. */
	}
    }
    rtn = [MKGetEnvelopeClass() new];
    [rtn setPointCount:curPoint + 1
   xArray:dataX
   orSamplingPeriod:0.0 
   yArray:dataY 
   smoothingArray:(varyingSmoothing) ? dataZ : NULL 
   orDefaultSmoothing:defaultSmoothing];
    if (stickPoint != NOSTICK)
      [rtn setStickPoint:stickPoint];
    return rtn;
}

static id getBinaryObjectDecl(void)
{
#   define WILD 0    
    id rtn;
    id aClass;
    register int c;
    if (!(aClass = _MK_FINDCLASS(getBinaryString(NO))))
      errorMsg(MK_sfCantFindClass,tokenBuf);
    /* The following cases are in case the guy wrote an env or wave out
       as a normal object. SPECIAL-WAVE-ENV-CASE. */
    /*
      Note: It is forbidden to use the character "]" in the description.
      Also, it is forbidden to write a Note, Score or Part as a parameter value
      since these may use ']' in their description.
      */
    if (!strcmp("Envelope",tokenBuf)) {
	return getBinaryEnvelopeDecl();
    }
    if ((!strcmp("Partials",tokenBuf)) ||
	(!strcmp("Samples",tokenBuf))) {
	return getBinaryWaveTableDecl();
    }
    if (aClass)
      rtn = [aClass new];
    if (!aClass || ![rtn respondsTo:@selector(readASCIIStream:)]) {
	do {
	    c = NXGetc(parsePtr->_stream);
	    CHECKEOS();
	    if (lookahead == ']')
	      break;
	} while (lookahead != ']');
	errorMsg(MK_notScorefileObjectTypeErr,tokenBuf);
	return getNullObject();
    }
    [rtn readASCIIStream:scoreStream];
    c = NXGetc(parsePtr->_stream);
    CHECKEOS();
    if (c != ']')
      error(MK_sfMissingStringErr,"]"); /* Fatal error */
    return rtn;
}

static void addIndexedObject(id obj)
{
    [scoreRPtr->_binaryIndexedObjects addObject:obj];
}

static id getBinaryDataDecl(_MKToken declType)
{
    register char *name = getBinaryString(NO);
    register id obj;
    if (strlen(name))
      name = _MKMakeStr(name);
    switch (declType) {
      case _MK_waveTableDecl:
	obj = getBinaryWaveTableDecl();
	break;
      case _MK_envelopeDecl:
	obj = getBinaryEnvelopeDecl();
	break;
      default:
	obj = getBinaryObjectDecl();
	break;
    }
    if (strlen(name)) {
	addLocalSymbol(name,obj,MK_waveTable);
	MKNameObject(name,obj);
	addIndexedObject(obj);
	NX_FREE(name);
    }
    return obj;
}

static short addParameter(char *name);

static short getAppPar(void)
{
    getBinarySymbol();
    if (lookahead == INT(_MK_undef))
      return addParameter(tokenBuf);
    else return _MKGetParNamePar(tokenVal->symbol);
    /*** FIXME BINARY Need to check for non-params here. ***/ 
}

static void getBinaryParameters(id aNote)
{
    unsigned short parHdr[2];
#   define PARAM parHdr[0]
#   define TYPE  parHdr[1]
#   define HANDLEAPPPAR() if (PARAM == MK_appPars) PARAM = getAppPar()
    for (;;) {
	NXRead(parsePtr->_stream, parHdr, sizeof(unsigned short) * 2);
	if (PARAM == MAXSHORT) /* High order bits of MAXINT */
	  break;
	CHECKEOS();
	switch (TYPE) {
	  case MK_string: {
	      register char *str = getBinaryString(YES);
	      HANDLEAPPPAR();
	      MKSetNoteParToString(aNote,PARAM,str);
	      break;
	  }
	  case _MK_typedVar: {
	      register double dbl = getBinaryVarValue();
	      HANDLEAPPPAR();
	      MKSetNoteParToDouble(aNote,PARAM,dbl);
	      break;
	  }
	  case MK_int: {
	      register int i = getBinaryInt();
	      HANDLEAPPPAR();
	      MKSetNoteParToInt(aNote,PARAM,i);
	      break;
	  }
	  case MK_double: {
	      register double dbl = getBinaryDouble();
	      HANDLEAPPPAR();
	      MKSetNoteParToDouble(aNote,PARAM,dbl);
	      break;
	  }
	  case MK_waveTable: 
	    getBinaryIndexedObject();
	    HANDLEAPPPAR();
	    if (tokenVal->symbol)
	      MKSetNoteParToWaveTable(aNote,PARAM,tokenVal->symbol);
	    break;
	  case MK_object:
	    getBinaryIndexedObject();
	    HANDLEAPPPAR();
	    if (tokenVal->symbol)
	      MKSetNoteParToObject(aNote,PARAM,tokenVal->symbol);
	    break;
	  case MK_envelope:
	    getBinaryIndexedObject();
	    HANDLEAPPPAR();
	    if (tokenVal->symbol)
	      MKSetNoteParToEnvelope(aNote,PARAM,tokenVal->symbol);
	    break;
	  case _MK_waveTableDecl:
	    {
		register id tmp = getBinaryDataDecl(_MK_waveTableDecl);
		HANDLEAPPPAR();
		if (tmp)
		  MKSetNoteParToWaveTable(aNote,PARAM,tmp);
		break;
	    }
	  case _MK_objectDecl:
	    {
		register id tmp = getBinaryDataDecl(_MK_objectDecl);
		HANDLEAPPPAR();
		if (tmp)
		  MKSetNoteParToObject(aNote,PARAM,tmp);
		break;
	    }
	  case _MK_envelopeDecl:
	    {
		register id tmp = getBinaryDataDecl(_MK_envelopeDecl);
		HANDLEAPPPAR();
		if (tmp)
		  MKSetNoteParToEnvelope(aNote,PARAM,tmp);
		break;
	    }
	  default:
	    break;
	}
    }
}


/* Declaration processing. ------------------------------------------- */

static void snarfCommas(void) 
{
    while (lookahead==',') 
      MATCH(',');
}

static void 
declErrCheck(const char *typeS,short declaredType)
{
    if (lookahead == INT(_MK_undef))
	return;
    else if (lookahead == INT(declaredType))
      error(MK_sfDuplicateDeclErr,tokenBuf);
    else if (lookahead > INT(_MK_undef))
      error(MK_sfWrongTypeDeclErr,tokenBuf,typeS);
    else 
      error(MK_sfBadDeclErr,curToken());
}

static id installPart(char *name)
{
    id part = [scoreRPtr->_owner _newFilePartWithName:name];
    addLocalSymbol(name,part,_MK_partInstance);
    return part;
}

static void
partDecl(void)
{	
    MATCH(_MK_part);
    do {    
	if (lookahead == INT(_MK_partInstance)) {
	    MATCH(_MK_partInstance);
	    /* We allow duplicate declarations here.*/
	    continue;   
	}
	declErrCheck(_MKTokNameNoCheck(_MK_part),0);
	installPart(tokenBuf);
	MATCH(lookahead);
    }  while (match(','));
}

static void
binaryPartDecl(void)
{	
    getBinarySymbol();
    if (lookahead == INT(_MK_partInstance)) {  /* Already declared */
	/* I think this is ok -- it's a merge of existing parts. */
	return;
    }
    /* We don't need a declErrCheck here. It actually doesn't matter what
       we call the part. The collissions are resolved when it's written to
       a file. */
    [scoreRPtr->_binaryIndexedObjects addObject:installPart(tokenBuf)];
    return;
}

/* Globals */

/* See /ds/david/doc/globals.doc */

static void installGlobalLocally(id obj,unsigned short type)
    /* Get global name and put obj on local table with that name. 
       Assumes name is undefined in local table. Also assumes that the type 
       found in table is the same as 'type'. Finally, assumes that obj is a 
       global on table. */
{
    const char *s = _MKGetGlobalName(obj);
    _MKNameTableAddName(scoreRPtr->_symbolTable,(char *)s,
			nil,obj,
			type | (_MK_NOFREESTRINGBIT | _MK_BACKHASHBIT) ,NO);
    /* Probably don't really need backhash here, but can't hurt. */
}

static void installLocalGlobally(char *s,id anObj,unsigned short type)
    /* s is any name. 
       Put local symbol on global table. 
       It's up to the caller to insure that anObj is of a type that's allowed
       to be global, that anObj is not yet global, and that anObj is already
       on the local table. */
{
    /* First remove from table as local. */
    _MKNameTableRemoveObject(scoreRPtr->_symbolTable,anObj); 
    /* Then install in global table. */ 
    _MKNameGlobal(s,anObj,type,NO,YES); 
    /* Now reinstall in local table as a global. */
    installGlobalLocally(anObj,type);      
}

static void putGlobal()
{ 
    unsigned short tok;
    MATCH(lookahead);
    do {
	if (lookahead == _MK_undef)
	  error(MK_sfUndeclaredErr,"symbol",curToken());
	if (_MKGetNamedGlobal(tokenBuf,&tok))
	  error(MK_sfMulDefErr,curToken(),_MKTokName((int)tok));
	switch (lookahead) { 
	  case _MK_typedVar:
	  case _MK_untypedVar:
	  case MK_object:
	  case MK_waveTable:
	  case MK_envelope:
	    break;
	  default:
	    error(MK_sfGlobalErr,_MKTokName(lookahead));
	}
	installLocalGlobally(tokenBuf,tokenVal->symbol,
			     (unsigned short)lookahead);
	MATCH(lookahead);
    } while (match(','));
}

static void getGlobal()
{ 
    id obj;
    unsigned short tok;
    short type;
    MATCH(lookahead);
    do {
	switch (lookahead) { 
	  case _MK_doubleVarDecl: /* Optional decl type */
	  case _MK_stringVarDecl: 
	  case _MK_intVarDecl:  
	  case _MK_waveVarDecl:
	  case _MK_envVarDecl:
	  case _MK_objVarDecl:
	    type = _MK_typedVar;
	    MATCH(lookahead);
	    break;
	  case _MK_varDecl:       
	    type = _MK_untypedVar;
	    MATCH(lookahead);
	    break;
	  case _MK_objectDecl:
	    type = MK_object;
	    MATCH(lookahead);
	    break;
	  case _MK_waveTableDecl:
	    type = MK_waveTable;
	    MATCH(lookahead);
	    break;
	  case _MK_envelopeDecl:
	    type = MK_envelope; 
	    MATCH(lookahead);
	    break;
	  case _MK_typedVar:     /* Otherwise, it's the global itself. */
	  case _MK_untypedVar:
	  case MK_object:
	  case MK_waveTable:
	  case MK_envelope:
	    type = lookahead;
	    break;
	  default:
	    error(MK_sfGlobalErr,curToken());
	}
	if (lookahead != _MK_undef)
	  error(MK_sfMulDefErr,curToken(),_MKTokName(lookahead));
	if (!(obj = _MKGetNamedGlobal(tokenBuf,&tok)))
	  error(MK_sfCantFindGlobalErr,tokenBuf);
	if (tok != (unsigned short) type)
	  error(MK_sfMulDefErr,tokenBuf,_MKTokName((int)tok));
	installGlobalLocally(obj,(unsigned short)type);
	MATCH(lookahead);       
    } while (match(','));
}



static void
varDecl(void)
{
    id aScorefileVar;
    _MKParameter *aPar;
    BOOL isUntyped = NO;
    short varType = lookahead;
    short dataType = MK_noType;
    _MKParameterUnion tmpUnion;
    MATCH(lookahead);
    do {
	declErrCheck(_MKTokNameNoCheck(_MK_typedVar),_MK_typedVar);
	declErrCheck(_MKTokNameNoCheck(_MK_untypedVar),_MK_untypedVar);
	switch (varType) {
	  case _MK_doubleVarDecl:
	    aPar = _MKNewDoublePar(0.0,MK_noPar);
	    break;
	  case _MK_stringVarDecl:
	    aPar = _MKNewStringPar("",MK_noPar);
	    break;
	  case _MK_varDecl:
	    isUntyped = YES;
	    /* No break here */
	  case _MK_intVarDecl: 
	    aPar = _MKNewIntPar(0,MK_noPar);
	    break;
	  default:
	    if (dataType == MK_noType)
	      dataType = ((varType == _MK_envVarDecl) ? MK_envelope :
			  (varType == _MK_waveVarDecl) ? MK_waveTable:
			  MK_object);
	    aPar = _MKNewObjPar(nil,MK_noPar,dataType);
	    break;
	}
	aScorefileVar = _MKNewScorefileVar(aPar,tokenBuf,isUntyped,NO);
	lookahead = (isUntyped) ? _MK_untypedVar : _MK_typedVar;
	addLocalSymbol(tokenBuf,aScorefileVar,lookahead);
	tokenVal->symbol = aScorefileVar;
	expression(&tmpUnion); /* Possible initialization value */
    }  while (match(','));
}  

static void
tune(void)
{
    double val;
    MATCH(_MK_tune);
    /* There are two forms of the tune statement. */
    if (lookahead != INT(_MK_typedVar)) { 
	_MKParameterUnion tmpUnion;
	switch (expression(&tmpUnion)) {  /* Arg is semitones to retune all */
	  case MK_double:
	    val = tmpUnion.rval;
	    break;
	  case MK_int:
	    val = (double)tmpUnion.ival;
	    break;
	  default:
	    error(MK_sfNoTuneErr,curToken());
	}
	[TuningSystem transpose:val];
    }
    else {
	id var = tokenVal->symbol;
	short keyNum;
	MATCH(lookahead);                 /* Arg is pitch to retune */
	keyNum = _MKFindPitchVar(var);
	if (keyNum >= MIDI_NUMKEYS)
	  error(MK_sfNoTuneErr);
	match('=');
	val = getDouble();
	[TuningSystem setKeyNumAndOctaves:keyNum toFreq:val];
    }
}


/* Control structure --------------------------------------------------- */

enum {thenClause, elseClause, repeatClause, whileClause, doClause }; 

typedef struct _progStructure {
    struct _progStructure *next;
    int clause;
    int location;
    int lineNo;
    int pageNo;
    short count; /* Only used for iteration */
    short lookahead;
    parseStateStruct *parseState;
} progStructure;

static void popStructureStack(void)
{
    register progStructure *tmp;
    tmp = ((progStructure *)scoreRPtr->_repeatStack);
    if (!tmp)
      return;
    scoreRPtr->_repeatStack = tmp->next;
    NX_FREE(tmp);
}

static progStructure *addProgStruct(void)
{
    progStructure *tmp;
    progStructure *newHead;
    tmp = (progStructure *)scoreRPtr->_repeatStack;
    _MK_MALLOC(newHead,progStructure,1);
    scoreRPtr->_repeatStack = newHead;
    newHead->next = tmp;
    return newHead;
}

/*** FIXME 
  Can handle single then or else exprs by check after every statement to see
  if it's the special one-statement case.
  Maybe error recovery should stop on ';' OR '}'?
  What about recovering from bad structure? The problem is that there are
  also WaveTables and such to be fooled by. A good fix for this latter 
  problem is to make '[{' and '}]' be new symbols in the lexical 
  analyzer. This will make '}' and '{' uniquely recognizable as structure
  blocks ***/

static void saveLoc(progStructure *p)
{
    p->lineNo = parsePtr->_lineNo;
    p->pageNo = parsePtr->_pageNo;
    p->location = NXTell(scoreStream);
    p->parseState = parsePtr; /* We save this for error checking. */
    p->lookahead = lookahead;
}

static void restoreLoc(progStructure *p)
{
    if (p->parseState != parsePtr)
      error(MK_sfBadIncludeErr,"}","{");
    NXSeek(scoreStream,p->location,NX_FROMSTART);
    lookahead = p->lookahead;
    parsePtr->_lineNo = p->lineNo;
    parsePtr->_pageNo = p->pageNo;
}

static void skipBlock(char openChar,char closeChar)
    /* Assumes first brace has been snarfed. Matches last closeChar. */
{
    /* FIXME This could be sped up for the case when we're in an iterative
       loop by arranging to store location in file.  */
    int braceCount = 1;
    do {
	if (openChar == lookahead) {
	    braceCount++; 
	} 
	else if (closeChar == lookahead) {
	    --braceCount;
	} 
	MATCH(lookahead);
    } while (braceCount);
}

static BOOL
  endBlock(void)
{
    progStructure *head = (progStructure *)scoreRPtr->_repeatStack;
    if (!head) {          /* Unmatched } */
	lookahead = ';';  /* Help error recovery */
	error(MK_sfMissingStringErr,"{");
    }
    switch (head->clause) {
      case elseClause: /* End of a taken else block */
	popStructureStack();
	break;
      case thenClause: /* End of a taken 'if' or 'else if' block */
	MATCH('}');
	while (match(_MK_else) && (head->clause != elseClause)) {
	    if (match(_MK_if)) {
		if (!match('(')) 
		  warning(MK_sfMissingStringErr,"(");
		skipBlock('(',')');
	    } 
	    else head->clause = elseClause;
	    if (!match('{')) 
	      warning(MK_sfMissingStringErr,"{");
	    skipBlock('{','}');
	}
	popStructureStack();
	return NO;
      case repeatClause:  /* End of a repeat block */
	if (--head->count > 0) {          /* More iteration */
	    /* This puts us back at the '{' */
	    restoreLoc(head);
	    match('{');  /* Use lower-case match because { may be missing */
	    return NO;
	}
	else 
	  popStructureStack();
	break;
      case doClause:
	MATCH('}');
	if (!match(_MK_while))
	  error(MK_sfMissingStringErr,"while");
	if (getBOOL()) {   /* Go back */
	    restoreLoc(head);
	    match('{');
	    return NO;
	}
	else {
	    popStructureStack();
	    return YES;
	}
	break;
      case whileClause: {  /* End of a while block */
	  progStructure temp;
	  saveLoc(&temp);
	  restoreLoc(head);
	  if (!getBOOL()) {
	      restoreLoc(&temp);
	      popStructureStack();
	  } 
	  else {
	      match('{');
	      return NO;
	  }
	  break;
      }
      default:
	lookahead = ';';  /* Help error recovery */
	error(MK_sfMissingStringErr,"'if' or 'repeat'");
	/* error never returns */
    }
    MATCH('}');
    return NO;
}

static void 
  whileStmt(void)
{
    progStructure *newHead = addProgStruct();
    BOOL doIt;
    MATCH(_MK_while);
    saveLoc(newHead);
    if (lookahead != '(')
      error(MK_sfMissingStringErr,"(");
    newHead->clause = whileClause;
    doIt = getBOOL(); 
    if (!match('{'))
      warning(MK_sfMissingStringErr,"{");
    if (!doIt) {
	skipBlock('{','}');  /* Skip false while block */
	popStructureStack();
    }
}

static void 
  doStmt(void)
{
    progStructure *newHead = addProgStruct();
    MATCH(_MK_do);
    newHead->clause = doClause;
    saveLoc(newHead);
    if (!match('{'))
      warning(MK_sfMissingStringErr,"{");
}

static void 
  repeat(void)
{
    progStructure *newHead = addProgStruct();
    MATCH(_MK_repeat);
    newHead->count = getInt();
    newHead->clause = repeatClause;
    saveLoc(newHead);
    if (!match('{')) 
      warning(MK_sfMissingStringErr,"{");
    if (newHead->count < 1) {
	skipBlock('{','}');  /* Skip false repeat block */
	popStructureStack();
    }
}

static void
  ifStmtAux(progStructure *head)
{
    BOOL doIt;
    MATCH(_MK_if);
    if (lookahead != '(')
      error(MK_sfMissingStringErr,"(");
    doIt = getBOOL(); 
    if (!match('{')) 
      warning(MK_sfMissingStringErr,"{");
    if (doIt)
      head->clause = thenClause;
    else {
	/* Skip to else portion, if any. */
	skipBlock('{','}');
	if (lookahead == _MK_else) { /* else or else if */
	    MATCH(_MK_else);
	    if (lookahead == _MK_if) {
		ifStmtAux(head);
		return;
	    }
	    else if (!match('{'))    /* Missing '{' */
	      warning(MK_sfMissingStringErr,"{");
	    head->clause = elseClause;
	} 
	else { /* No else clause */
	    popStructureStack(); /* Get rid of this if/then struct */
	}
    }
}

static void
  ifStmt(void)
{
    progStructure *head = addProgStruct();
    ifStmtAux(head);
}

static void
  elseStmt(void)
/* If this is called, it is an error */ 
{
    error(MK_sfMissingStringErr,"if clause");
}




static void
comment(void)
{
    MATCH(_MK_comment);
    shutUp = YES;
    while (lookahead != INT(_MK_endComment))
      MATCH(lookahead);
    MATCH(_MK_endComment);
    shutUp = NO;
}  

static void
dataDecl(_MKToken type)
/* Type is one of _MK_envelopeDecl or _MK_waveTableDecl or _MK_objectDecl */
{
    short rtnType;
    MATCH(type);
    do {
	/* Named objects, envelopes and waveTables are the only declarations 
	   that are handled
	   by the recursive descent portion of the parser. This is because
	   they are the only declarations that can occur within expressions. 
	   */
	EMPTYSTACK();           
	namedDataDecl(type);
	stackPop(&rtnType);   /* namedDataDecl pushes the env on the stack */
	STACKERRORCHECK();
    }
    while (match(','));
}

static short addParameter(char *name)
    /* Assumes name's token is currently _MK_undef with regard to the local
       table. */
{
    id aParName;
    short i;
    i = _MKGetPar(name,&aParName);
    installGlobalLocally(aParName,_MK_param); 
    return i;
}

#if 0
static void
parListDecl(void)
{
    BOOL skipped;
    id aParList;
    int param;
    _MKToken t;
    _MKParameterUnion tmpUnion;
    MATCH(_MK_parList);
    declErrCheck(_MKTokNameNoCheck(_MK_parList),_MK_parListInstance);
    aParList = [_ParList new];
    addLocalSymbol(tokenBuf,aParList,_MK_parListInstance);
    MATCH(_MK_undef);
    matchInsert('=');
    while (lookahead == INT(_MK_param) || lookahead == INT(_MK_undef)) {
	if (lookahead == INT(_MK_undef))
	  param = addParameter(tokenBuf);
	else param = _MKGetParNamePar(tokenVal->symbol);
	MATCH(lookahead);
	if (lookahead == ':') {
	    MATCH(':');
	    switch (t = expression(&tmpUnion)) {
	      case MK_double:
		skipped = ([aParList setPar:param toDouble:tmpUnion.rval] == 
			  nil);
		break;
	      case MK_int:
		skipped = ([aParList setPar:param toInt:tmpUnion.ival] == nil);
		break;
	      case MK_string: 
		skipped = ([aParList setPar:param toString:tmpUnion.sval]
			   == nil);
		break;
	      case MK_envelope:
		skipped = ([aParList setPar:param toEnvelope:tmpUnion.symbol]
			   == nil);
		break;
	      case MK_waveTable:
		skipped = ([aParList setPar:param toWaveTable:tmpUnion.symbol]
			   == nil);
		break;
	      case MK_object:
		skipped = ([aParList setPar:param toObject:tmpUnion.symbol]
			   == nil);
		break;
	      default:
		error(MK_sfBadParValErr);
	    }
	}
	else skipped = ([aParList setPar:param toInt:0] == nil);
	if (skipped)
	  warning(MK_sfOmittingDupParErr);
	match(',');
    }
}

static void
apply(void)
{
    id aPart,aList;
    MATCH(_MK_apply);
    if (lookahead != INT(_MK_parListInstance))
      error(MK_sfUndeclaredErr,_MKTokNameNoCheck(_MK_parListInstance),
	    curToken());
    aList = tokenVal->symbol;
    MATCH(lookahead);
    if (!match(_MK_to))
      error(MK_sfMissingStringErr,_MKTokNameNoCheck(_MK_to));
    if (lookahead != INT(_MK_partInstance))
      error(MK_sfUndeclaredErr,_MKTokNameNoCheck(_MK_partInstance),curToken());
    aPart = tokenVal->symbol;
    MATCH(_MK_partInstance);
    if (lookahead == ';') { /* This is the default case. */
	[aPart _setReadParList: aList noteType:MK_noteOn];
	[aPart _setReadParList: aList noteType:MK_noteDur];
    }
    while (match('(')) {    /* He specified a type. */
	[aPart _setReadParList: aList noteType:lookahead];
	MATCH(lookahead);
	if (match(')')) 
	  break;
	match(',');         /* Can have multiple types. */
    }
}  
#endif

static void
include(void)
{
    _MKParameterUnion tmpUnion;
    parseStateStruct * backwardsLink;
    char * s;
    MATCH(_MK_include);
    if (expression(&tmpUnion) != INT(MK_string))
      /*** FIXME Maybe make this have to be a constant. */
      error(MK_sfCantFindFileErr,"");
    /* Lookahead stored in struct should be semicolon here. */
    storeInStruct(scoreRPtr,parsePtr);
    backwardsLink = parsePtr;     /* Save value in case of error below */
    if (!initScoreParsePtr(s = tmpUnion.sval)) {
	parsePtr = backwardsLink; /* Restore value. */
	scoreRPtr->_parsePtr = (void *)parsePtr;
	loadFromStruct(scoreRPtr);
	errorMsg(MK_sfCantFindFileErr,s);
    }
    else 
      parsePtr->_backwardsLink = backwardsLink; /* Stack link. */
}  

static void
print(void)
{
    short t;
    _MKParameterUnion tmpUnion;
    MATCH(_MK_print);
    while (lookahead != ';') {
	t = expression(&tmpUnion);
	if (scoreRPtr->printStream)
	  switch (t) {
	    case MK_double: 
	      NXPrintf(scoreRPtr->printStream,"%f",tmpUnion.rval); 
	      break;
	    case MK_int:
	      NXPrintf(scoreRPtr->printStream,"%d",tmpUnion.ival); 
	      break;
	    case MK_string: 
	      NXPrintf(scoreRPtr->printStream,tmpUnion.sval); 
	      break;
	    case MK_object:
	    case MK_waveTable:
	    case MK_envelope: {
		char * s = (char *)MKGetObjectName(tmpUnion.symbol);
		if (s) 
		  NXPrintf(scoreRPtr->printStream,"%s %s = ",
			   _MKTokNameNoCheck(t),s);
		[tmpUnion.symbol writeScorefileStream:scoreRPtr->printStream];
		break;
	    }
	    default:
	      error(MK_sfCantWriteErr,curToken());
	  }
	match(',');
    }
    if (scoreRPtr->printStream)
      NXFlush(scoreRPtr->printStream);
}

static void clipTagRange(void)
{
    /* If application asks for a range bigger than MAXRANGE, clip. 
       This is to avoid the "growing noteTag syndrome". */
#   define MAXRANGE ((int)0xffffff)
    if ((scoreRPtr->_fileHighTag - scoreRPtr->_fileLowTag) > MAXRANGE)
      scoreRPtr->_fileHighTag = scoreRPtr->_fileLowTag + MAXRANGE;
}

static void binaryNoteTagRange(void)
{
    scoreRPtr->_fileLowTag = getBinaryInt();
    scoreRPtr->_fileHighTag = getBinaryInt();
    clipTagRange();
}

static void noteTagRange(void)
{
    MATCH(_MK_noteTagRange);
    matchInsert('=');
    scoreRPtr->_fileLowTag = getInt();
    if (!match(_MK_to))
      error(MK_sfMissingStringErr,_MKTokNameNoCheck(_MK_to));
    scoreRPtr->_fileHighTag = getInt();
    if (scoreRPtr->_fileHighTag < scoreRPtr->_fileLowTag)
      error(MK_sfOutOfOrderErr,_MKTokNameNoCheck(_MK_noteTagRange));
    clipTagRange();
}

static void
body(void)
{
    int i = scoreRPtr->_fileHighTag - scoreRPtr->_fileLowTag + 1;
    scoreRPtr->isInBody = YES;
    if (i > 0)
      scoreRPtr->_newLowTag = MKNoteTags(i);
}

static void setPar(id aNote,short aPar)
{
    /* Get expression and set specified parameter in aNote. Might want to make
       this a macro. */
    _MKParameterUnion tmpUnion;
    switch (expression(&tmpUnion)) {
      case MK_string: 
	MKSetNoteParToString(aNote,aPar,tmpUnion.sval);
	break;
      case MK_int: 
	MKSetNoteParToInt(aNote,aPar,tmpUnion.ival);
	break;
      case MK_double: 
	MKSetNoteParToDouble(aNote,aPar,tmpUnion.rval);
	break;
      case MK_waveTable:
	MKSetNoteParToWaveTable(aNote,aPar,tmpUnion.symbol);
	break;
      case MK_object:
	MKSetNoteParToObject(aNote,aPar,tmpUnion.symbol);
	break;
      case MK_envelope:
	MKSetNoteParToEnvelope(aNote,aPar,tmpUnion.symbol);
	break;
      default: error(MK_sfBadParValErr);
    }
}

static void setInfo(id aNote,id infoOwner)
{
    [infoOwner _setInfo:aNote];
    [aNote free]; /* _setInfo: does a copy. */
}

static void
  partOrScoreInfo(BOOL isPartInfo)
{
    register id aNote;
    id target;
    short j;
    target = (isPartInfo) ? tokenVal->symbol : scoreRPtr->_owner;
    aNote = [MKGetNoteClass() new];
    MATCH(declarator);
    snarfCommas();
    if (lookahead == '(') {
	while ((lookahead != ')') && (lookahead != ';'))
	  MATCH(lookahead); /* Snarf notetype if any. (Not really 
			       allowed, but
			       we'll be lenient here. */
	match(')');
    }
    snarfCommas();
    while (lookahead != ';') {
	if (lookahead != INT(_MK_param))
	  if (lookahead == INT(_MK_undef)) 
	    j = addParameter(tokenBuf);
	  else error(MK_sfBadParamErr);
	else 
	  j = _MKGetParNamePar(tokenVal->symbol);
	MATCH(lookahead);
	if (lookahead != ':') 
	  error(MK_sfMissingStringErr,":");
	MATCH(':');         
	setPar(aNote,j);
	snarfCommas();
    }
    setInfo(aNote,target);
}

static void
  binaryPartOrScoreInfo(BOOL isPartInfo)
{
    register id aNote;
    id target;
    if (isPartInfo) 
      getBinaryIndexedObject();
    target = (isPartInfo) ? tokenVal->symbol : scoreRPtr->_owner;
    aNote = [MKGetNoteClass() new];
    getBinaryParameters(aNote);
    setInfo(aNote,target);
}

static void
headerStmt(void)
{	
    matchSemicolon();
    for (;;)
      switch (lookahead) {
	case _MK_noteTagRange: 
	  noteTagRange(); 
	  return;
	case _MK_part:
	  partDecl(); 
	  return;
	case _MK_partInstance:
	  partOrScoreInfo(YES);
	  return;
	case _MK_info:
	  partOrScoreInfo(NO);
	  return;
	case _MK_doubleVarDecl: 
	case _MK_stringVarDecl: 
	case _MK_intVarDecl:  
	case _MK_waveVarDecl:
	case _MK_objVarDecl:
	case _MK_envVarDecl:
	case _MK_varDecl:
	  varDecl(); 
	  return;
	case _MK_objectDecl:
	case _MK_envelopeDecl:
	case _MK_waveTableDecl:
	  dataDecl(lookahead); 
	  return;
	case _MK_begin:
	  MATCH(_MK_begin); 
	  body();
	  return;
	case _MK_comment: 
	  comment(); 
	  return;
	case _MK_include:	
	  include(); 
	  return;
	case _MK_putGlobal:
	  putGlobal();
	  return;
	case _MK_getGlobal:
	  getGlobal();
	  return;
	case _MK_seed:
	  setSeed();
	  return;
	case _MK_ranSeed:
	  ranSeed();
	  return;
	case _MK_tune:
	  tune(); 
	  return;
	case _MK_print:
	  print(); 
	  return;
	case _MK_typedVar:
	case _MK_untypedVar: {
	    _MKParameterUnion tmpUnion;
	    expression(&tmpUnion); 
	    return;
	}
	  /* The following 5 cases break rather than return */
	case _MK_repeat:
	  repeat();
	  break;
	case _MK_if:
	  ifStmt();
	  break;
	case '}':
	  if (endBlock())    /* returns YES if we should match semicolon */
	    return;
	  break;
	case _MK_while:
	  whileStmt();
	  break;
	case _MK_do:
	  doStmt();
	  break;
	case _MK_else:
	  elseStmt(); /* This triggers an error */
	  return;
	case 0:              /* Eof. */
	  return;
	case _MK_endComment:
	  error(MK_sfUnmatchedCommentErr);
	default:
	  error(MK_sfBadHeaderStmtErr,curToken());
      }
}

static void
binaryHeaderStmt(void)
{	
    lookahead = getBinaryShort();
    switch (lookahead) {
      case _MK_noteTagRange: 
	binaryNoteTagRange(); 
	break;
      case _MK_part:
	binaryPartDecl(); 
	break;
      case _MK_partInstance:
	binaryPartOrScoreInfo(YES);
	break;
      case _MK_info:
	binaryPartOrScoreInfo(NO);
	break;
      case _MK_begin:
	body();
	return;
      case 0:
	return;
      default:
	error(MK_sfNonScorefileErr,""); /* Fatal error */
	return;
    }
    return;
}

static BOOL isBinaryScorefile(NXStream *aStream)
{
    static parseStateStruct foo;
    parsePtr = &foo;
    parsePtr->_stream = aStream;
    if (getBinaryInt() != MK_SCOREMAGIC) {
	errorMsg(MK_sfNonScorefileErr);
	return NO;
    }
    parsePtr = NULL;
    return YES;
}

_MKScoreInStruct *
_MKNewScoreInStruct(NXStream *aStream,id owner,NXStream *printStream,
		    BOOL mergeParts,char *name)
    /* Assumes aStream is a pointer to a stream open for read.
       Creates a _MKScoreInStruct and parses the file's header. 
       Returns a pointer to the new _MKScoreInStruct. 
       Owner is an object that must meet the following restrictions:
       1. It must respond to _newFilePartWithName:(char *)name by
       creating an object corresponding to name. This object corresponds to
       a part declaration in the file. It may actually be a Part object or it 
       may be any object that responds to _setInfo:.
       2. It must respond to _setInfo:.  
       3. If mergeParts is YES, it must respond to -_elements by returning a 
       List of the objects which it contains (i.e. those created in response 
       to _newFilePartWithName:.

       FIXME 
       To make this more palatable:
       Change _setInfo: to setInfoMerge: in both Score and Part. 
       Change _newFilePartWithName: to newScorefilePartWithName: or
          newObjectWithName:.
       Make mergeParts be private or change _elements to ??.
       */
{
    BOOL binary;
    if ((!aStream) || (!owner))
      return NULL;
    initScanner();
    {
	/* Decide what kind of file it is */
	int c = NXGetc(aStream);
	if (!NXAtEOS(aStream)) /* We got something */
	  if (c == FIRST_CHAR_SCOREMAGIC) { /* Look like it might be binary */
	      NXUngetc(aStream);
	      if (!isBinaryScorefile(aStream))
		return NULL;
	      binary = YES;
	  }
	  else {
	      NXUngetc(aStream);
	      initExpressionParser();
	      binary = NO;
	  }
	else binary = YES; /* EOS. Pretend it's binary in this case. */
    }
    _MK_MALLOC(scoreRPtr,_MKScoreInStruct,1);
    scoreRPtr->_binary = binary;
    if (!printStream) {                 /* Use stderr */
	printStream = NXOpenFile((int)stderr->_file,NX_WRITEONLY);
	scoreRPtr->_freeStream = (printStream != NULL);
    }
    else scoreRPtr->_freeStream = NO;
    if (binary) 
      scoreRPtr->_binaryIndexedObjects = [List new];
    scoreRPtr->printStream = printStream;
    scoreRPtr->_ranState = initRan();
    scoreRPtr->_noteTagTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    scoreRPtr->timeTag = 0;
    scoreRPtr->_fileHighTag = -1;
    scoreRPtr->_fileLowTag = 0;
    scoreRPtr->_errCount = 0;
    scoreRPtr->_symbolTable = _MKNewScorefileParseTable();
    scoreRPtr->_aNote = (id )nil;
    parsePtr = initParsePtr(aStream,TOPLEVELFILE,
			    ((name) ? name : "Scorefile"));
    /* We decided NOT to merge the same Part name when you read a scorefile 
       into a Score. For ScorefilePerformer, we currently are merging Parts.
       But this is maybe the wrong thing. It might be better to 'clean up'
       the ScorefilePerformer after performing. FIXME. */
    if (mergeParts)
    {       
	id aList;
	char * aName;
	aList = [owner _elements];
	if (aList) {
	    id *el;
	    unsigned n;
	    for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++)
	      if (aName = (char *)MKGetObjectName(*el))
		addLocalSymbol(aName,*el,_MK_partInstance);
	}
	[aList free];
    }
    storeInStruct(scoreRPtr,parsePtr);
    scoreRPtr->isInBody = NO;
    scoreRPtr->_owner = owner;
    return scoreRPtr;
}

void _MKParseScoreHeader(_MKScoreInStruct *scorefileRPtr)
    /* Parse entire header */
{
    while ((!_MKParseScoreHeaderStmt(scorefileRPtr)) && 
	   (scorefileRPtr->timeTag != MK_ENDOFTIME))
      ;
}

BOOL _MKParseScoreHeaderStmt(_MKScoreInStruct *scorefileRPtr)
    /* Parse next header stmt, if any. Returns YES if body is reached.
       Returns NO and sets timeTag to MK_ENDOFTIME if file terminated for
       some other reason.
     */
{
    scoreRPtr = scorefileRPtr;
    switch (setjmp(begin)) {
      case errorLongjmp:         /* Non-fatal error */
      case noLongjmp:
	break;
      case fatalErrorLongjmp:   /* Too many errors or binary error */
	scoreRPtr->_errCount = MAXINT; /* Signal app. */
	while (ISINCLUDEFILE(parsePtr))
	  popFileStack();
	scoreRPtr->timeTag = MK_ENDOFTIME;
	storeInStruct(scoreRPtr,parsePtr);
	return scoreRPtr->isInBody;
      case eofLongjmp:            /* EOF */
	if (ISINCLUDEFILE(parsePtr))
	  popFileStack();
	else {
	    scoreRPtr->timeTag = MK_ENDOFTIME;
	    storeInStruct(scoreRPtr,parsePtr);
	    return scoreRPtr->isInBody;
	}
	break;
    }
    if (scoreRPtr->isInBody)
      return YES;
    if (BINARY(scoreRPtr)) 
      binaryHeaderStmt();
    else headerStmt();
    storeInStruct(scoreRPtr,parsePtr);
    return scoreRPtr->isInBody;
}

/* Statement processing. -------------------------------------------- */


static double
  getTime(double curT)
{	
    BOOL relativeTime;
    double val;
    MATCH(_MK_time);
    if (relativeTime = (lookahead == '+')) 
      MATCH('+');
    val = getDouble();
    if (relativeTime) 
      return val + curT;
    if (curT > val)
      error(MK_outOfOrderErr,val,curT);
    return val;
}

static int
  getNoteTag(BOOL binary,MKNoteType noteType)
{
    int fileTag;
    if (binary) {
	fileTag = getBinaryInt();
	if (fileTag == MAXINT)
	  return MAXINT;
    } else {
	match(',');
	fileTag = getInt();
    }
    if ((fileTag <= scoreRPtr->_fileHighTag) &&
	(fileTag >= scoreRPtr->_fileLowTag)) 
      return fileTag + scoreRPtr->_newLowTag - scoreRPtr->_fileLowTag;
    /* Lookup noteTag. If found, use mapping. Otherwise, if noteOff
       or noteOn generate error. */
    {
	int newTag = (int)[scoreRPtr->_noteTagTable valueForKey:
			   (void *)fileTag]; 
	if (newTag)
	  return newTag;
#if 0
	else if (!binary && 
		 (noteType == MK_noteUpdate || noteType == MK_noteOff)) 
	  error(MK_sfInactiveNoteTagErr,_MKTokNameNoCheck(noteType));
#endif
	newTag = MKNoteTag();
	[scoreRPtr->_noteTagTable insertKey:(const void *)fileTag value:
	 (void *)newTag];
	return newTag;
    }
}

static id parseScoreNote(void)
{
    register id aNote;
    id aPart;
    short j;
    matchSemicolon();
    for (;;) {
 	switch (lookahead) {
	  case _MK_end:
	    MATCH(_MK_end);
	    if (ISINCLUDEFILE(parsePtr))
	      popFileStack();
	    else {
		storeInStruct(scoreRPtr,parsePtr);
		scoreRPtr->timeTag = MK_ENDOFTIME;
		return nil;
	    }
	    break;
	  case _MK_doubleVarDecl: 
	  case _MK_stringVarDecl: 
	  case _MK_intVarDecl:  
	  case _MK_waveVarDecl:
	  case _MK_envVarDecl:
	  case _MK_objVarDecl:
	  case _MK_varDecl:
	    varDecl();
	    break;
	  case _MK_objectDecl:
	  case _MK_waveTableDecl:
	  case _MK_envelopeDecl:
	    dataDecl(lookahead);
	    break;
	  case _MK_seed:
	    setSeed();
	    break;
	  case _MK_ranSeed:
	    ranSeed();
	    break;
	  case _MK_include:
	    include(); 
	    continue;           /* Semicolon is snarfed by include. */
	  case _MK_print:
	    print(); 		/* See note above. */
	    break;
	  case _MK_tune:
	    tune(); 
	    break;
	  case _MK_comment:	  
	    comment();
	    break;
	    /* The following three cases continue rather than break */
	  case _MK_repeat:
	    repeat();
	    continue;
	  case _MK_if:
	    ifStmt();
	    continue;
	  case '}':
	    if (endBlock())    /* returns YES if we should match semicolon */
	      break;
	    continue;
	  case _MK_while:
	    whileStmt();
	    continue;
	  case _MK_do:
	    doStmt();
	    continue;
	  case _MK_else:
	    elseStmt(); /* This is an error */
	    break;
	  case _MK_time:
	    scoreRPtr->timeTag = getTime(scoreRPtr->timeTag);
	    storeInStruct(scoreRPtr,parsePtr);
	    return (id )nil;
	  case _MK_putGlobal:
	    putGlobal();
	    break;
	  case _MK_getGlobal:
	    getGlobal();
	    break;
	  case _MK_typedVar:
	  case _MK_untypedVar:
	    assign();
	    break;
	  case _MK_undef:
	    error(MK_sfUndeclaredErr,_MKTokNameNoCheck(_MK_part),tokenBuf);
	    break;
	  case _MK_endComment:
	    error(MK_sfUnmatchedCommentErr);
	    break;
	  case _MK_partInstance:
	    aPart = tokenVal->symbol;
	    MATCH(_MK_partInstance);
	    [scoreRPtr->_aNote free];
	    scoreRPtr->_aNote = aNote = 
	      [MKGetNoteClass() newSetTimeTag:scoreRPtr->timeTag];
	    snarfCommas();
	    if (lookahead != '(') 
	      error(MK_sfBadNoteTypeErr);
	    MATCH('(');
	    j = lookahead;
	    switch (lookahead) {
	      case MK_noteOn:
	      case MK_noteOff:
		MATCH(lookahead);
		if (lookahead == ')')
		  error(MK_sfBadNoteTagErr);
		_MKSetNoteType(aNote,j);
		_MKSetNoteTag(aNote,getNoteTag(NO,j));
		break;
	      case MK_noteUpdate:
		_MKSetNoteType(aNote,j);
		MATCH(lookahead);
		if (lookahead != ')') /* Tags are optional here. */
		  _MKSetNoteTag(aNote,getNoteTag(NO,j));
		break;
	      case MK_mute:
		/* Note type is, by default, mute */
		MATCH(lookahead);
		if (lookahead != ')')
		  error(MK_sfMissingStringErr,")");
		break;
	      default: 
		{
		    double x;
		    x =  getDouble();
		    _MKSetNoteDur(aNote,MAX(x,0));
		    if (lookahead != ')')
		      _MKSetNoteTag(aNote,getNoteTag(NO,MK_noteDur));
		}
		break;
	    }
	    MATCH(')');             /* Closing paren of noteType */
	    snarfCommas();
#if 0
	    aList = [aPart _getReadParList:j]; /* j is notetype */
	    if (aList) {
		register id *aPar;
		unsigned i = [aList count];
		BOOL defaults = NO;
		defaults = NO;
		for (aPar = NX_ADDRESS(aList); i--; aPar++) {
		    switch (lookahead) {
		      case _MK_param:
		      case ';':
		      case 0:
		      case _MK_undef:
			defaults = YES; /* file list shorter than parlist */
			break;
		      default:
			break;
		    }
		    if (defaults)                 /* Parameter was omitted */
		      _MKNoteAddParameter((id)aNote,*aPar); /* copies *aPar */
		    else setPar(aNote,(*aPar)->parNum);/* Uses file value */
		    if (!match(','))
		      break;
		    snarfCommas();
		}
	    }
#endif
	    while (lookahead != ';') {
		if (lookahead != INT(_MK_param))
		  if (lookahead == INT(_MK_undef)) 
		    j = addParameter(tokenBuf);
		  else error(MK_sfBadParamErr);
		else 
		  j = _MKGetParNamePar(tokenVal->symbol);
		MATCH(lookahead);
		if (lookahead != ':') 
		  error(MK_sfMissingStringErr,":");
		MATCH(':');         
		setPar(aNote,j);
		snarfCommas();
	    }
	    storeInStruct(scoreRPtr,parsePtr);
	    scoreRPtr->part = aPart;  /* return these by reference*/
	    return aNote;
	  default: 
	    error(MK_sfBadStmtErr,curToken());
	    break;
	}
	matchSemicolon();
    }
}

static id 
  parseBinaryScoreNote(void)
/* Returns the 
   current note or nil if the current statement in the file is a time setting.
   If EOF is reached, the timeTag field of scorefileRPtr is set to 
   MK_ENDOFTIME and _MKParseScoreNote returns nil. 
   Otherwise, the timeTag field is set to the current time. The part
   field of scorefileRPtr is set to the Part of the current note.
   The note returned should be copied on memory. 
   It is 'owned' by the _MKScoreInStruct pointer. */
{
    register id aNote;
    id aPart;
    lookahead = getBinaryShort();
    switch (lookahead) {
      case _MK_end:
	scoreRPtr->timeTag = MK_ENDOFTIME;
	/*** FIXME Or set a flag here and don't clobber timeTag. ***/ 
	return nil;
      case _MK_time:
	scoreRPtr->timeTag += ((double)getBinaryFloat());
	return nil;
      case _MK_partInstance: {
	  unsigned short tok;
	  getBinaryIndexedObject();      /* Sets tokenVal->symbol */
	  aPart = tokenVal->symbol;
	  [scoreRPtr->_aNote free]; 
	  scoreRPtr->_aNote = aNote = 
	    [MKGetNoteClass() newSetTimeTag:scoreRPtr->timeTag];
	  tok = getBinaryShort();
	  switch (tok) {
	    case MK_noteOn:
	    case MK_noteOff:
	    case MK_noteUpdate:
	      _MKSetNoteType(aNote,tok);
	      _MKSetNoteTag(aNote,getNoteTag(YES,tok));
	      break;
	    case MK_mute: /* Notes are type mute by default. */
	      break;
	    case MK_noteDur:
	      _MKSetNoteDur(aNote,getBinaryDouble()); /* Also sets type */
	      _MKSetNoteTag(aNote,getNoteTag(YES,MK_noteDur));
	      break;
	  }
	  getBinaryParameters(aNote);
	  scoreRPtr->part = aPart;  /* return part by reference*/
	  return aNote;
      }
      default:
	error(MK_sfNonScorefileErr,"");
    }
}    

id 
_MKParseScoreNote(_MKScoreInStruct * scorefileRPtr)
/* Returns the current note or nil if the current statement in the file is a 
   time setting. If EOF is reached, the timeTag field of scorefileRPtr is 
   set to MK_ENDOFTIME and _MKParseScoreNote returns nil. 
   Otherwise, the timeTag field is set to the current time. The part
   field of scorefileRPtr is set to the Part of the current note.
   The note returned should be copied on memory. 
   It is 'owned' by the _MKScoreInStruct pointer. */
{
    if (!scorefileRPtr || !scorefileRPtr->_parsePtr) 
      return (id )nil;
    if (scorefileRPtr->timeTag == MK_ENDOFTIME)
      return nil; /* This check is just for safety. It can only happen if the
		     caller doesn't quit after seeing the first ENDOFTIME. */

    scoreRPtr = scorefileRPtr;
    loadFromStruct(scoreRPtr);
    switch (setjmp(begin)) {
      case errorLongjmp:         /* Non-fatal error */
      case noLongjmp:
	break;
      case fatalErrorLongjmp:   /* Too many errors or binary error */
	scoreRPtr->_errCount = MAXINT; /* Signal app. */
	while (ISINCLUDEFILE(parsePtr))
	  popFileStack();
	scoreRPtr->timeTag = MK_ENDOFTIME;
	storeInStruct(scoreRPtr,parsePtr);
	return nil;
      case eofLongjmp:            /* EOF */
	if (ISINCLUDEFILE(parsePtr))
	  popFileStack();
	else {
	    scoreRPtr->timeTag = MK_ENDOFTIME;
	    storeInStruct(scoreRPtr,parsePtr);
	    return nil;
	}
	break;
    }
    if (BINARY(scoreRPtr)) 
      return parseBinaryScoreNote();
    else return parseScoreNote();
}


/* File-finding stuff */
#if 0    
/* Don't include this stuff until it's fixed */

/*** FIXME This stuff needs to be updated to handle binary files. 
  Also, see changes in utilities.m */

#import <string.h>
#import <stdlib.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/file.h>

static char *getHomeDirectory()
{
    static char *homeDirectory = NULL;
    struct passwd  *pw;
    if (!homeDirectory) {  /* Only do it the first time. It's expensive. */
	pw = getpwuid(getuid());
	if (pw && (pw->pw_dir) && (*pw->pw_dir)) {
	    _MK_MALLOC(homeDirectory,char,strlen(pw->pw_dir)+1);
	    strcpy(homeDirectory,pw->pw_dir);
	}
    }
    return homeDirectory;
}

#define HOMESCOREDIR "/Library/Music/Scores/"
#define LOCALSCOREDIR "/LocalLibrary/Music/Scores/"
#define SYSTEMSCOREDIR "/NextLibrary/Music/Scores/"

/**** FIXME Add this to parseScore.m, Score.m (read only) 
  ScorefilePerformer.m (read only) ***/

NXStream *_MKFindScorefile(int *fd,char *name)
{
    NXStream *rtnVal;
    *fd = MKFindScorefile(name);
    if ((OPENFAIL(*fd)) || ((rtnVal = NXOpenFile(*fd,readOrWrite)) == NULL)) 
      _MKErrorf(MK_cantOpenFileErr,fileName);
    return rtnVal;
}

int MKFindScorefile(char *name)
    /* This function opens a scorefile in read-only mode. The search
       proceeds as follows:
       
       If name is a path beginning with '/', name is used as an absolute path.
       Otherwise, name is relative and the following directories are searched
       in the order given:

       1. The current working directory.
       2. ~/Library/Music/Scores/
       3. /LocalLibrary/Music/Scores/
       4. /NextLibrary/Music/Scores/

       Returns a file descriptor or -1 if the file can't be opened.
       */
{
    int siz;
    int fd;
    char *p;
    char *fileName;
    int addExt = 0;
    if (!name) 
      return -1;
    if (!strstr(name,".score"))  /* Ext not there */
      addExt = 1;
    siz = strlen(name) + 100;    /* Enough for LOCALSCOREDIR and extension */
    _MK_MALLOC(fileName,char,siz);
    strcpy(fileName,name);
    if (addExt)
      strcat(fileName,".score");
    fd = open(fileName,O_RDONLY,_MK_PERMS); 
    if (fd != -1) {
	NX_FREE(fileName);
	return fd;
    }
    if (name[0]!='/') { /* There's hope */
	if (p = getHomeDirectory()) {
	    siz += strlen(p);
	    _MK_REALLOC(fileName,char,siz); 
	    strcpy(fileName,p);
	    strcat(fileName,HOMESCOREDIR);
	    strcat(fileName,name);
	    if (addExt)
	      strcat(fileName,".score");
	    fd = open(fileName,O_RDONLY,_MK_PERMS);
	    if (fd != -1) {
		NX_FREE(fileName);
		return fd;
	    }
	}
	
	strcpy(fileName,LOCALSCOREDIR);
	strcat(fileName,name);
	if (addExt)
	  strcat(fileName,".score");
	fd = open(fileName,O_RDONLY,_MK_PERMS); 
	if (fd != -1) {
	    NX_FREE(fileName);
	    return fd;
	}
	strcpy(fileName,SYSTEMSCOREDIR);
	strcat(fileName,name);
	if (addExt)
	  strcat(fileName,".score");
	fd = open(fileName,O_RDONLY,_MK_PERMS); 
    }
    NX_FREE(fileName);
    return fd;
}

#endif



