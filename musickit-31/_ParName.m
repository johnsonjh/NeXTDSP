#ifdef SHLIB
#include "shlib.h"
#endif

/*
  _ParName.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/

/* This file has the parameter-representation mechanism.

   The term "parameter" is, unfortunately, used loosely for several things.
   This could be cleaned up, but it's all private functions, so it's just
   an annoyance to the maintainer:

   1) An object, of class _ParName, that represents the parameter name. E.g.
   there is only one instance of this object for all frequency parameters.

   2) A low integer that corresponds to the _ParName object. E.g. the constant
   MK_freq is a low integer that represents all frequency parameters.

   3) A string name that corresponds to the _ParName object. E.g. "freq".

   4) A struct called _MKParameter, that represents the particular parameter 
   value.  E.g. there is one _MKParameter for each note containing a frequency.

   The _ParName contains the string name, the low integer, and a function
   (optional) for printing the parameter values in a special way.

   The _MKParameter contains the data, the type of the data, and the
   low integer. There's an array that maps low integers to _ParNames.
   
   Note objects contain an NXHashTable of _MKParameters. CF: Note.m 

   Note that the code for writing scorefiles is spread between writeScore.m,
   Note.m, and _ParName.m. This is for reasons of avoiding inter-module
   communication (i.e. minimizing globals). Perhaps the scorefile-writing
   should be more cleanly isolated.
   */

   

/* 
Modification history:

  _ParName.m:
  09/18/89/daj - Changes to accomodate new way of doing parameters (structs 
                 rather than objects).
  09/22/89/daj - Changes to accomodate changes in _MKNameTable.
  parameter.m:
  09/22/89/daj - Added _MK_BACKHASH bit or'ed in with type when adding name,
                 to accommodate new way of handling back-hashing.
  10/06/89/daj - Type changes for new _MKNameTable implementation.
  10/15/89/daj - Merged _ParName.m and parameter.m and flushed global functions
                 that are no longer needed.
  10/20/89/daj - Added binary scorefile support.
                 Changed writeData() to give anonymous data a default name
		 of the form class-name# where # is an integer.
  02/01/90/daj - Fixed trace message.
                 Fixed bug in anonymous data naming.
		 Added comments.
  03/05/90/daj - Added _MKSymbolize() call in _MKGetPar() to insure that 
                 parameter names are valid scorefile symbols. Changed 
		 scorefile string-printing routine (_MKParWriteStdValueOn)
		 to encode chars such as \n as character escape codes. 
  03/21/90/daj - Small changes to quiet -W compiler warnings.
  08/13/90/daj - Added _MKParNameStr().
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things

*/

#import <objc/HashTable.h>
#import <ctype.h>

#define MK_INLINE 1
#import "_musickit.h"
#import "_tokens.h"
#import "_TuningSystem.h"
#import "_error.h"
#import "_MKNameTable.h"
#import "_Partials.h"
#import "Envelope.h"
#import "_ParName.h"


/* First, here's how parameter names are represented: */

@implementation _ParName:Object
/* This class is used for parameter names.   Each parameter is
   represented by a unique instance of _ParName. They have an optioal
   function which is called when the parameter value is written and
   they have a low integer value, the particular parameter.
   The printfunc allows particular parameters to write in 
   special ways. For example, keyNum writes using the keyNum constants.
   You never instantiate instances of this class directly. 

   This is a private Musickit class.
*/
{
    BOOL (*printfunc)(_MKParameter *param,NXStream *aStream,
		      _MKScoreOutStruct *p);
    /* printfunc is a function for writing the value of the 
       par. See below for details.
       */
    int par;    /* What parameter this is. */
    char * s;   /* Name of parameter */
}

#define BINARY(_p) (_p && _p->_binary) /* writing a binary scorefile? */

static id newParName(char * name,int param)
    /* Make a new one */
{
    register _ParName *self = [_ParName new];
    self->s = _MKMakeStr(name);
    _MKNameGlobal(name,self,_MK_param | _MK_BACKHASHBIT,YES,YES);
    self->par = param;
    self->printfunc = NULL;
    return self;
}

unsigned _MKGetParNamePar(_ParName *self)
    /* Return parameter */
{ 
    return self->par;
}


/* Here's a lot of initialization. */

static _ParName **parIds = NULL; /* Par name array. */

/* Define parameter names */
#import "parNames.m"

static void intParNames(fromPar,toPar)
    int fromPar,toPar;
    /* Initialize all NeXT parameters */
{
    register unsigned i;
    register _ParName **parId = &(parIds[fromPar]);
    const char * *parNamePtr = (const char **) &(parNames[fromPar]);
    for (i=fromPar; i<toPar; i++) 
      *parId++ = newParName((char *)*parNamePtr++,i);
}

static BOOL keywordsPrintfunc(_MKParameter *parameter,NXStream *aStream,
			      _MKScoreOutStruct *p)
    /* Used to write parameters with keyword-valued arguments. */
{
    int i = _MKParAsInt(parameter);
    if (BINARY(p))
      _MKWriteIntPar(aStream,i);
    else if (!_MK_VALIDTOKEN(i))
      NXPrintf(aStream, "%d", i);
    else NXPrintf(aStream,"%s",_MKTokName(i));
    return YES;
}    

/* For keeping track of application-defined parameters: */
static int parArrSize = (int)MK_appPars;
static int highestPar = _MK_FIRSTAPPPAR - 1; 

id _MKParNameObj(int aPar)
    /* Returns _ParName object of specified parameter. aPar must be a valid 
       parameter */
{
    return parIds[aPar];
}

char *_MKParNameStr(int aPar)
    /* Returns _ParName object of specified parameter. aPar must be a valid 
       parameter */
{
    return parIds[aPar]->s;
}

int _MKHighestPar()
    /* Returns the number of the most recently-created parameter name */
{
    return highestPar;
}

int _MKGetPar(char *aName,_ParName **aPar)
    /* Gets parameter name, installing if necessary. Also returns _ParName 
       object by reference. */
{
    unsigned short type;
    *aPar = _MKGetNamedGlobal(aName,&type);
    if (*aPar)
      return (int)_MKGetParNamePar(*aPar);
    else {
	/* Allocates a new parameter number and sets obj's instance varaible
	   to that number. */
#       define EXPANDAMT 5
	BOOL wasChanged; 
	aName = _MKSymbolize(aName,&wasChanged); /* Make valid sf symbol */
	*aPar = newParName(aName,++highestPar);
	if (wasChanged) /* _MKSymbolize copied the string */
	  NX_FREE(aName);
	if (highestPar >= parArrSize) {
	    _MK_REALLOC(parIds, _ParName *, (highestPar + EXPANDAMT));
	    parArrSize = highestPar + EXPANDAMT;
	}
	parIds[highestPar] = *aPar;
	if (_MKTrace() & MK_TRACEPARS)
	  fprintf(stderr,"Adding new parameter %s\n",(*aPar)->s);
	return highestPar;
    }
}

static void initSyns()
    /* Install synonyms. */
{
    register int i;
    register const char * *parNamePtr = (const char **) &(parSynNames[0]);
    register int *parSynPtr = (int *) &(parSyns[0]);
    _ParName *oldObj,*newObj;
    for (i = 0; i < SYNS; i++) {
	oldObj = _MKParNameObj(*parSynPtr);
	newObj = newParName((char *)*parNamePtr++,*parSynPtr++);
	newObj->printfunc = oldObj->printfunc; 
	/* Same printfunc */
    }
}

/* Avoid propogating multiple copies of the null string */
static const char *uniqueNull = NULL;

const char *_MKUniqueNull()
{
    return uniqueNull;
}

#import <objc/hashtable.h>

#define INT(_x) ((int)_x)

BOOL
_MKParInit()
  /* Sent once from Note. Returns YES. Initializes the world */
{
    _MK_CALLOC(parIds,id,_MK_maxPrivPar);
    intParNames(MK_noPar,MK_privatePars);
    intParNames(MK_privatePars+1,_MK_maxPrivPar);
    initSyns();
    parIds[INT(MK_keyNum)]->printfunc = _MKKeyNumPrintfunc;
    parIds[INT(MK_freq)]->printfunc = _MKFreqPrintfunc;
    parIds[INT(MK_freq0)]->printfunc = _MKFreqPrintfunc;
    parIds[INT(MK_chanMode)]->printfunc = keywordsPrintfunc;
    parIds[INT(MK_sysRealTime)]->printfunc = keywordsPrintfunc;
    uniqueNull = NXUniqueString("");
    return YES;
}


/* Primitives for creating, freeing and copying _MKParameters */

#define CACHESIZE 100
static _MKParameter * _cache[100]; /* Avoid unnecessary malloc/free */
static unsigned _cachePtr = 0;

static _MKParameter * newPar(parNum)
    int parNum;
    /* Returns new _MKParameter struct */
{
    register _MKParameter *param;
    if (_cachePtr > 0)
      param = _cache[--_cachePtr];
    else _MK_MALLOC(param,_MKParameter,1);
    param->parNum = parNum;
    return param;
}

#define DEBUG_CACHE 0

_MKParameter *_MKFreeParameter(_MKParameter *param)
  /* Frees object and string datum, if any. */
{
    if (!param)
      return NULL;
#   if DEBUG_CACHE 
    {
	int i;
	for (i=0; i<_cachePtr; i++) 
	  if (_cache[i] == param) {
	    fprintf(stderr,"Attempt to free freed parameter.\n");
	    return NULL;
	}
    }
#   endif
    if (_cachePtr < CACHESIZE) 
      _cache[_cachePtr++] = param;
    else NX_FREE(param);
    return NULL;
}

BOOL _MKIsParPublic(param)
    _MKParameter *param;
    /* Returns whether this is a public parameter. */
{
    return _MKIsPar(parIds[param->parNum]->par);
}

_MKParameter *_MKCopyParameter(_MKParameter *param)
    /* Creates a copy of the _MKParameter */
{
    _MKParameter *newOne;
    if (!param)
      return NULL;
    newOne = newPar(param->parNum);
    newOne->_uVal = param->_uVal;
    newOne->_uType = param->_uType;
    return newOne;
}


/* All of the following return a new _MKParameter struct initialized as 
   specified */

_MKParameter * _MKNewDoublePar(value,parNum)
    double value;
    int parNum;
{
    register _MKParameter *param = newPar(parNum);
    param->_uType = MK_double;
    param->_uVal.rval = value;
    return param;
}

_MKParameter * _MKNewStringPar(value,parNum)
    char * value;
    int parNum;
{
    register _MKParameter *param = newPar(parNum);
    param->_uType = MK_string;
    param->_uVal.sval = (char *)NXUniqueString(value);
    return param;
}

_MKParameter * _MKNewIntPar(value,parNum)
    int value;
    int parNum;
{
    register _MKParameter *param = newPar(parNum);
    param->_uType = MK_int;
    param->_uVal.ival = value;
    return param;
}

_MKParameter * _MKNewObjPar(value,parNum,type)
    id value;
    int parNum;
    _MKToken type;
{
    register _MKParameter *param = newPar(parNum);
    param->_uType = type;
    param->_uVal.symbol = value;
    return param;
}


/* All of the following set the parameter struct to the value indicated */

_MKParameter * _MKSetDoublePar(param,value)
    _MKParameter *param;
    double value;
    /* Set the Parameter to type and double and assign doubleVal. */
{
    param->_uType = MK_double;
    param->_uVal.rval = value;
    return param;
}

_MKParameter * _MKSetIntPar(param,value)
    _MKParameter *param;
    int value;
    /* Set the Parameter to type int and assign intVal. */
{
    param->_uType = MK_int;
    param->_uVal.ival = value;
    return param;
}

_MKParameter * _MKSetStringPar(param,value)
    _MKParameter *param;
    char * value;
    /* Set the Parameter to type string and assign a copy of stringVal. */
{
    param->_uVal.sval = (char *)NXUniqueString(value);
    param->_uType = MK_string;
    return param;
}

_MKParameter * _MKSetObjPar(param,value,type)
    _MKParameter *param;
    id value;
    _MKToken type;
    /* Sets obj field and type */
{
    param->_uVal.symbol = value;
    param->_uType = type;
    return param;
}

static double firstEnvValue(param)
    _MKParameter *param;
  /* If receiver is not type MK_envelope, returns MK_NODVAL. Otherwise,
     returns the first value of the envelope multiplied by yScale plus
     yOffset. If the receiver is envelope typed but the object is not
     an envelope, returns MK_NODVAL. */
{
    double rtn,dummy,dummy2;
    id env;
    if (param->_uType == MK_envelope)
      env = param->_uVal.symbol;
    else return MK_NODVAL;
    if ((![env isKindOf:_MKClassEnvelope()]) || 
	([env getNth:0 x:&dummy y:&rtn smoothing:&dummy2] <MK_noEnvError))
      return MK_NODVAL;
    return rtn;
}

int _MKParAsInt(param)
    _MKParameter *param;
    /* Get the current value as an integer. If the receiver is envelope-valued,
       returns truncated firstEnvValue(param). If the receiver is
       waveTable-valued, returns MAXINT.
       */
{
    switch (param->_uType) {
      case MK_double:	
	return ((int) param->_uVal.rval);
      case MK_string:	
	return (_MKStringToInt(param->_uVal.sval));
      case MK_int: 	
	return param->_uVal.ival;
      case MK_envelope:
	return (int)firstEnvValue(param);
      default: 
	break;
    }
    return MAXINT;
}

double _MKParAsDouble(param)
    _MKParameter *param;
    /* Get the current value as a double. If the receiver is envelope-valued,
       returns firstEnvValue(param). If the receiver is waveTable-valued
       returns MK_NODVAL. */
{
    switch (param->_uType) {
      case MK_double:
	return param->_uVal.rval;
      case MK_string:	
	return (_MKStringToDouble(param->_uVal.sval));
      case MK_int:
 	return ((double) param->_uVal.ival);
      case MK_envelope:
	return (double)firstEnvValue(param);
      default:
	break;
    }
    return MK_NODVAL;
}

char * _MKParAsString(param)
    _MKParameter *param;
    /* Returns a copy of the datum as a string. If the receiver is envelope-
       valued, returns a description of the envelope. */
{
    switch (param->_uType) {
      case MK_double:	 
	return _MKDoubleToString(param->_uVal.rval);
      case MK_int:	
	return _MKIntToString(param->_uVal.ival);
      case MK_string:
	return _MKMakeStr(param->_uVal.sval);
      case MK_envelope:
	return _MKMakeStr(" (envelope) "); /*** FIXME ***/
      case MK_waveTable:
	return _MKMakeStr(" (waveTable) ");  /*** FIXME ***/
      default:
	return _MKMakeStr([param->_uVal.symbol name]); /*** FIXME ***/
    }
    return _MKMakeStr("");
}

char * _MKParAsStringNoCopy(param)
    _MKParameter *param;
    /* Returns the value as a string , but does
       not copy that string. Application should not
       delete the returned value and should not rely on its
       staying around for longer than the Parameter object.
       This method is provided as a speed optimzation. 
       */
{
    switch (param->_uType) {
      case MK_double:	 
	return _MKDoubleToStringNoCopy(param->_uVal.rval);
      case MK_int:	
	return _MKIntToStringNoCopy(param->_uVal.ival);
      case MK_string:
	return param->_uVal.sval;
      case MK_envelope:
	return " (envelope) ";   /*** FIXME ***/
      case MK_waveTable:
	return " (waveTable) ";  /*** FIXME ***/
      default:
	return _MKMakeStr([param->_uVal.symbol name]); /*** FIXME ***/
    }
    return "";
}

id _MKParAsEnv(param)
    _MKParameter *param;
  /* Returns envelope if any, else nil. The envelope is not copied. */
{
    if (param->_uType == MK_envelope)
      return param->_uVal.symbol;
    return nil;
}

id _MKParAsObj(param)
    _MKParameter *param;
  /* If type is MK_envelope, MK_waveTable or MK_object, returns the object.
     Otherwise, returns nil. */
{
    switch (param->_uType) {
      case MK_envelope:
      case MK_waveTable:
      case MK_object:
	return param->_uVal.symbol;
      default:
	return nil;
    }
}

id _MKParAsWave(param)
    _MKParameter *param;
  /* Returns waveTable if any, else nil. The waveTable is not copied. */
{
    if (param->_uType == MK_waveTable)
      return param->_uVal.symbol;
    else return nil;
}

_MKParameterUnion *_MKParRaw(param)
    _MKParameter *param;
    /* Returns the raw _MKParameterUnion *. */
{
    return &param->_uVal;
}


/* The following is used for printing parameters to scorefiles and such */

static void parNameWriteValueOn(_ParName *parNameObj,NXStream *aStream,
			      _MKParameter *aVal,_MKScoreOutStruct *p)
{
    if (!parNameObj->printfunc || (!parNameObj->printfunc(aVal,aStream,p)))
      _MKParWriteStdValueOn(aVal,aStream,p);
}

void _MKParWriteOn(_MKParameter *param,NXStream *aStream,
		   _MKScoreOutStruct *p)
    /* Called by Note's _MKWriteParameters() */
{	
    register _ParName *self = parIds[param->parNum];
    if (_MKIsPrivatePar(self->par))
      return;
    if (BINARY(p)) {
	BOOL appPar = (self->par >= (int)(_MK_FIRSTAPPPAR));
	if (appPar) 
	  _MKWriteShort(aStream,MK_appPars);
	else _MKWriteShort(aStream,self->par);
	parNameWriteValueOn(self,aStream,param,p);
	if (appPar) 
	  _MKWriteString(aStream,self->s); 
    }
    else {
	NXPrintf(aStream, "%s:",self->s); 
	parNameWriteValueOn(self,aStream,param,p);
    }
}

void _MKParWriteValueOn(_MKParameter *param,NXStream *aStream,
			_MKScoreOutStruct *p)
    /* Private function used by ScorefileVars to write their values */
{	
    parNameWriteValueOn(parIds[param->parNum],aStream,param,p);
}

/*
  Strategy for writing objects:
  If we're not writing a scorefile, just print object definition.
  If a name isn't in there and the obj isn't named, name it.
  If a name isn't in the local table and the obj is named, 
  it needs to have a decl generated.
  If a name is in there and the obj's name is not pointing to the same object,
  make a new name to point to the new obj.
  */

static char *genAnonName(id obj)
    /* Generate default anonymous name */
{
    return _MKMakeStrcat([obj name],"1");
}

static void writeObj(id dataObj,NXStream *aStream,_MKToken declToken,BOOL
		     binary)
{
    if (!binary)
      NXPrintf(aStream,"[");
    if (declToken == MK_object) {
	if (![dataObj respondsTo:@selector(writeASCIIStream:)]) { 
	    _MKErrorf(MK_notScorefileObjectTypeErr,[dataObj name]);
	    if (binary)
	      _MKWriteChar(aStream,'\0');
	    else NXPrintf(aStream,"]");
	    return;  
	}
	NXPrintf(aStream,"%s ",[dataObj name]);
	[dataObj writeASCIIStream:aStream];        
    }
    else 
      if (binary)
	[dataObj _writeBinaryScorefileStream:aStream];
      else {
	  [dataObj writeScorefileStream:aStream];        
	  NXPrintf(aStream,"]");
      }
}

static void writeData(NXStream *aStream,_MKScoreOutStruct *p,
		      id dataObj,int type)
{
    id hashObj;
    char * name;
    BOOL binary = BINARY(p);
    BOOL changedName = NO;
    unsigned short tmp;
    _MKToken declToken;
    switch (type) {
      case MK_envelope:
	declToken = _MK_envelopeDecl;
	break;
      case MK_waveTable:
	declToken = _MK_waveTableDecl;
	break;
      default:
	declToken = _MK_objectDecl;
	break;
    }
    if (binary) {
	int val = (int)[p->_binaryIndecies valueForKey:(void *)dataObj];
	if (val) {
	    _MKWriteShort(aStream,type);
	    _MKWriteShort(aStream,val);
	    return;
	}
    }
    if (!p) {         /* We're not writing a scorefile so don't give 
			 it a name. */
	if (binary) {
	    _MKWriteShort(aStream,type);
	    _MKWriteChar(aStream,'\0');
	}
	writeObj(dataObj,aStream,declToken,binary);
	return;
    }
    name = (char *)MKGetObjectName(dataObj);
    if (!name) {
	name = genAnonName(dataObj);
	MKNameObject(name,dataObj);
	NX_FREE(name);
	name = (char *)MKGetObjectName(dataObj);
    }
    /* If we've gotten here, it's named and we're writing a scorefile. */
    hashObj = _MKNameTableGetObjectForName(p->_nameTable,name,nil,&tmp);
    if (hashObj && (hashObj != dataObj)) {     /* Resolve name collissions. */
	changedName = YES;
	name = _MKUniqueName(_MKMakeStr(name),p->_nameTable,dataObj,&hashObj);
    }
    if (hashObj == dataObj)          /* It's already declared in file. */
      NXPrintf(aStream,name);        /* Just write name. 
					(If we got here, we must be 
					writing an ascii file) */
    else {                           /* It's not been declared yet. */    
	if (binary) {
	    _MKWriteShort(aStream,declToken);
	    _MKWriteString(aStream,name);
	    [p->_binaryIndecies insertKey:(const void *)dataObj 
	   value:(void *)(++(p->_highBinaryIndex))];
	}
	else
	  NXPrintf(aStream,"%s %s = ",_MKTokName(declToken),name);
	writeObj(dataObj,aStream,declToken,binary);
	_MKNameTableAddName(p->_nameTable,name,nil,dataObj,
			    type | _MK_BACKHASHBIT,YES);
    }
    if (changedName)
      NX_FREE(name);
}


static void sfWriteStrPar(register NXStream *aStream,register char *str)
    /* Print str to aStream such that, when read in again as a scorefile,
       it will look identical. */
{
    char c;
    char *tmp;
#   define WRITECHAR(_c) c = (_c); NXWrite(aStream,&c,1)
    WRITECHAR('"');
    for (tmp = str; *str; str++)
      switch (*str) {
	case BACKSPACE:  case FORMFEED:  case NEWLINE:  case CR:  case TAB:
	case VT:  case BACKSLASH:  case QUOTE:  case '"':
	  NXWrite(aStream,tmp,str - tmp); /* Write string up to here. */
	  tmp = str + 1;                /* update tmp */
	  /* Translate char. */
	  WRITECHAR(BACKSLASH);
	  WRITECHAR(rindex((char *)_MKTranstab(),*str)[-1]);
	  break;
	default:
	  break;
      }
    NXWrite(aStream,tmp,str - tmp);  /* Write remainder of string. */
    WRITECHAR('"');
}

void _MKParWriteStdValueOn(_MKParameter *param,NXStream *aStream,
			   _MKScoreOutStruct *p)
    /* Private method that writes value in a standard way.
       If the_ParName for this parameter has a non-NULL print function,
       that function is used instead. That function may call 
       _MKParWriteStdValueOn, if desired. */
{	
    switch (param->_uType) { 
      case MK_double:
	if (BINARY(p))
	  _MKWriteDoublePar(aStream,_MKParAsDouble(param));
	else NXPrintf(aStream,"%.5f",_MKParAsDouble(param));
	break;
      case MK_string:
	if (BINARY(p))
	  _MKWriteStringPar(aStream,_MKParAsString(param));
	else sfWriteStrPar(aStream,_MKParAsString(param));
	break;
      case MK_envelope:
      case MK_waveTable:
      case MK_object:
	writeData(aStream,p,param->_uVal.symbol,param->_uType);
	break;
      default:
      case MK_int:
	if (BINARY(p)) 
	  _MKWriteIntPar(aStream,_MKParAsInt(param));
	else NXPrintf(aStream,"%d",_MKParAsInt(param));
	break;
    }
}

void _MKArchiveParOn(_MKParameter *param,NXTypedStream *aTypedStream) 
{
    BOOL isMKPar = (param->parNum<(int)(_MK_FIRSTAPPPAR)); /* YES if MK par */
    NXWriteType(aTypedStream,"c",&isMKPar);
    if (isMKPar) /* Write parameter number */
      NXWriteType(aTypedStream,"s",&param->parNum);
    else        /* Write parameter name */
      NXWriteType(aTypedStream,"*",&(parIds[param->parNum]->s));
    NXWriteType(aTypedStream,"s",&param->_uType);
    switch (param->_uType) { 
      case MK_double:
	NXWriteType(aTypedStream,"d", &param->_uVal.rval);
	break;
      case MK_string:
	NXWriteType(aTypedStream,"%", &param->_uVal.sval);
	break;
      case MK_envelope:
      case MK_waveTable:
      case MK_object:
	NXWriteType(aTypedStream,"@",&param->_uVal.symbol);
	break;
      default:
      case MK_int:
	NXWriteType(aTypedStream,"i",&param->_uVal.ival);
	break;
    }
}

void _MKUnarchiveParOn(_MKParameter *param,NXTypedStream *aTypedStream) 
{
    BOOL isMKPar;
    char *strVar;
    NXReadType(aTypedStream,"c",&isMKPar);
    if (isMKPar) /* Write parameter number */
      NXReadType(aTypedStream,"s",&param->parNum);
    else {       /* Write parameter name */
	id aParNameObj;
	NXReadType(aTypedStream,"*",&strVar);
	param->parNum = _MKGetPar(strVar,&aParNameObj);
	NX_FREE(strVar);
    }
    NXReadType(aTypedStream,"s",&param->_uType);
    switch (param->_uType) { 
      case MK_double:
	NXReadType(aTypedStream,"d", &param->_uVal.rval);
	break;
      case MK_string:
	NXReadType(aTypedStream,"%", &param->_uVal.sval);
	break;
      case MK_envelope:
      case MK_waveTable:
      case MK_object:
	NXReadType(aTypedStream,"@", &param->_uVal.symbol);
	break;
      default:
      case MK_int:
	NXReadType(aTypedStream,"i", &param->_uVal.ival);
	break;
    }
}

@end




















