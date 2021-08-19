#ifdef SHLIB
#include "shlib.h"
#endif

/*
	_MKNameTable.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: David Jaffe
  
	DEFINED IN: The Musickit

	Note:
	* back-hashing is optionally supported.
	* the name is owned by the name table (it is copied and freed)
	* a type is associated with the name.

	Note that if back-hashing is not specified (via the _MK_BACKHASH bit),
	the object name is NOT accessible from the object. We use backhashing
	except for things such as pitch names and keywords, where backhashing
	is never done.

	Should convert this to Bertrand's hashtable.
	Also, should NOT copy strings. Or, at least, have a no-copy bit
	that can be set for the strings that exist elsewhere (e.g. the
	ones for the keywords and such) in the program.
*/

/* 
Modification history:

   09/22/89/daj  Made back-hash optional in the interest of saving space.
                 Flushed _MKNameTableAddGlobalNameNoCopy() in light of the 
                 decision to make name table bits (e.g. _MK_NOFREESTRINGBIT) 
		 globally defined.
		 Flushed _MKNameTableAddNameNoCopy() and added a copyIt
		 parameter to _MKNameTableAddName().
  10/06/89/daj - Changed to use hashtable.h version of table.
  12/3/89/daj - Added seed and ranSeed to initTokens list.
  03/5/90/daj - Added check for null name and nil object in public functions.
                Added conversion of name in MKNameObject to legal scorefile
		name and added private global function _MKSymbolize().
   3/06/90/daj - Added _MK_repeat to keyword list.
   4/21/90/daj - Removed unused auto var.
*/

#define HASHSIZE	31
#define BACKHASHSIZE	31
#define HASHFUN(s)	_strhash(s)
#define BACKHASHFUN(obj)   (((unsigned)obj) >> 2) 

#import <ctype.h>
#import <objc/objc.h>
#import <objc/hashtable.h>

extern unsigned 	_strhash();

typedef struct _nameRecord {
    char        *name;
    id          object;
    id		owner;
    unsigned short type;
} nameRecord;

#define NAMESIZE	(sizeof(nameRecord))
#define NAMEDESCR	"{*@@}"

#import "_musickit.h"
#import "_MKNameTable.h" 
#import "_TuningSystem.h"

static unsigned forwardHash(const void *info,const void *data)
{
    return HASHFUN(((nameRecord *)data)->name);
}

static unsigned backwardHash(const void *info,const void *data)
{
    return BACKHASHFUN(((nameRecord *)data)->object);
}

static BOOL anyOwner = NO; /* Changes isEqual behavior */

#define NR(_x) ((nameRecord *)_x)

static int forwardIsEqual(const void *info,const void *data1,
			  const void *data2)
{
    if (!anyOwner)
      if (NR(data1)->owner != NR(data2)->owner)
	return NO;
    return (!(strcmp(NR(data1)->name,NR(data2)->name)));
}

static int backwardIsEqual(const void *info,const void *data1,
			  const void *data2)
{
    return (NR(data1)->object == NR(data2)->object);
}

#undef NR

#import "_ScorefileVar.h"

static nameRecord *cachedRec = NULL;

static nameRecord *getSymbol(void)
{
    nameRecord *nr;
    if (cachedRec) {
	nr = cachedRec;
	cachedRec = NULL;
	return nr;
    }
    _MK_MALLOC(nr,nameRecord,1);
    nr->object = nil;
    return nr;
}

static void giveSymbol(nameRecord *rec)
{
    static id scorefileVarClass = nil;
    if (!scorefileVarClass)
      scorefileVarClass = [_ScorefileVar class];
    if (!rec)
      return;
    if (!(rec->type & _MK_NOFREESTRINGBIT))
      NX_FREE(rec->name);
    if ([rec->object isKindOf:scorefileVarClass] && 
	(!(rec->type & _MK_NOFREESTRINGBIT)))
      [rec->object free];
    if (!cachedRec)
      cachedRec = rec;
    else NX_FREE(rec);
}

static void forwardFree(const void *info, void *data)
{
    giveSymbol((nameRecord *)data);
}

_MKBiHash *_MKNewBiHash(int forwardSize,int backwardSize)
{
    static NXHashTablePrototype forwardPrototype = {
	forwardHash,forwardIsEqual,forwardFree,0};
    static NXHashTablePrototype backwardPrototype = {
	backwardHash,backwardIsEqual,NULL,0};
    _MKBiHash *rtn;
    _MK_MALLOC(rtn,_MKBiHash,1);
    backwardPrototype.free = NXNoEffectFree;
    rtn->hTab = NXCreateHashTable(forwardPrototype,forwardSize,NULL);
    rtn->bTab = NXCreateHashTable(backwardPrototype,backwardSize,NULL);
    return rtn;
}

_MKBiHash *_MKFreeBiHash(_MKBiHash *aTable)
{
    NXFreeHashTable(aTable->bTab); /* Must be before back hash */
    NXFreeHashTable(aTable->hTab);
    NX_FREE(aTable);
    return NULL;
}

static void importAutoGlobals(_MKBiHash *globalTable,_MKBiHash *localTable)
    /* Adds all the 'auto-import' symbols from globalTable to localTable. The
       strings aren't copied. The type is set to be global.

       'auto-import' symbols are all those that are not of type
       MK_envelope, MK_waveTable or MK_object. (Actually, it's anything
       without the AUTOIMPORT bit on.) */
{
    nameRecord	*data;
    NXHashState	state = NXInitHashState (globalTable->hTab);
    while (NXNextHashState (globalTable->hTab, &state,(void **) &data))
      if (data->type & _MK_AUTOIMPORTBIT)
	_MKNameTableAddName(localTable,data->name,
			    data->owner,data->object,
			    data->type | _MK_NOFREESTRINGBIT,NO);
    /* Shares string */
}

_MKBiHash *_MKNameTableAddName(_MKBiHash *self,char *theName,id theOwner,
			       id theObject,
			       unsigned short theType,BOOL copyIt)
/*
 * Adds the object theObject in the table, with name theName and owner
 * theOwner. If there is already an entry for this name and owner does
 * nothing and returns nil. TheName is not copied. 
 */
{
    nameRecord *symbolRec;
    if (copyIt)
      theName = _MKMakeStr(theName);
    symbolRec = getSymbol();
    symbolRec->name = theName;
    symbolRec->owner = theOwner;
    symbolRec->object = theObject;
    symbolRec->type = theType;
    if (NXHashInsertIfAbsent(self->hTab,symbolRec) != symbolRec) {
	if (!copyIt)
	  symbolRec->type |= _MK_NOFREESTRINGBIT;
	giveSymbol(symbolRec);
	return NULL;
    }
    if (theType & _MK_BACKHASHBIT)
      NXHashInsert(self->bTab,symbolRec);
    return self;
}

id _MKNameTableGetFirstObjectForName(_MKBiHash *self,char *theName)
/*
 * Gets first object with specified name, ignoring owner.
 */
{
    nameRecord *symbolRec;
    nameRecord symbol;
    anyOwner = YES;
    symbol.name = theName;
    symbolRec = NXHashGet(self->hTab, &symbol);
    anyOwner = NO;
    if (symbolRec) 
      return symbolRec->object; 
    else return nil;
}

id _MKNameTableGetObjectForName(_MKBiHash *self,char *theName,id theOwner,
				unsigned short *typeP)
/*
 * If there is an object with this name and owner it is returned, otherwise
 * nil is returned.
 */
{
#   define MASKBITS (_MK_NOFREESTRINGBIT | _MK_AUTOIMPORTBIT | _MK_BACKHASHBIT)
#   define TYPEMASK(_x) (_x & (0xffff & ~(MASKBITS)))

    nameRecord *symbolRec;
    nameRecord symbol;
    symbol.owner = theOwner;
    symbol.name = theName;
    symbolRec = NXHashGet(self->hTab,&symbol);
    if (symbolRec) {
	*typeP = TYPEMASK(symbolRec->type); /* Clear bits */
	return symbolRec->object; 
    }
    else {
	*typeP = 0;
	return nil;
    }
}

char * _MKNameTableGetObjectName(_MKBiHash *table,id theObject,id *theOwner)
/*
 * If theObject has been entered in the table before, this method returns
 * its name and sets theOwner to its owner. Otherwise NULL is returned.
 */
{
    nameRecord symbol;
    nameRecord *symbolRec;
    symbol.object = theObject;
    symbolRec = NXHashGet(table->bTab,&symbol);
    if (symbolRec) {
        *theOwner = symbolRec->owner;
	return symbolRec->name;
    } else return NULL;
}

_MKBiHash *_MKNameTableRemoveName(_MKBiHash *table,char *theName,id theOwner)
/*
 * Removes the entry associated to (theName x theOwner) if any.
 */
{
    nameRecord symbol;
    nameRecord *symbolRec1,*symbolRec2;
    symbol.name = theName;
    symbol.owner = theOwner;
    symbolRec1 = NXHashRemove(table->hTab, &symbol);
    if (symbolRec1) 
      symbolRec2 = NXHashRemove(table->bTab, symbolRec1);
    else return NULL;
    giveSymbol(symbolRec1);
    return table;
}

_MKBiHash *_MKNameTableRemoveObject(_MKBiHash *table,id theObject)
/*
 * Removes theObject from the table.
 */
{
    nameRecord symbol;
    nameRecord *symbolRec1,*symbolRec2;
    symbol.object = theObject;
    symbolRec2 = NXHashRemove(table->bTab, &symbol);
    if (symbolRec2)
      symbolRec1 = NXHashRemove(table->hTab, symbolRec2);
    else return NULL;
    giveSymbol(symbolRec1);
    return table;
}


/* Routines to check and convert to C symbols for writing to score files. */

#define isIllegalFirstCChar(_c) (!isalpha(*sym) && (*sym != '_'))
#define isIllegalCChar(_c) (!isalnum(*sym) && (*sym != '_'))

static BOOL isCSym(register char *sym)
{
    if (isIllegalFirstCChar(*sym))
      return NO;
    while (*++sym) 
      if (isIllegalCChar(*sym))
	return NO;
    return YES;
}

static void convertToCSym(register char *sym,register char *newSym)
    /* newSym is assumed to be allocated to be at least the length of sym.
       Returns newSym. */
{
    *newSym++ = (isIllegalFirstCChar(*sym)) ? '_' : *sym;
    while (*++sym) 
      *newSym++ = (isIllegalCChar(*sym)) ? '_' : *sym;
    *newSym = '\0';
}

char *_MKSymbolize(char *sym,BOOL *wasChanged)
    /* Converts sym to new symbol, returns the new symbol (malloced),and
       sets *wasChanged to YES. If sym is already a symbol, does not malloc,
       returns sym, and sets *wasChagned to NO. */
{
    char *newSym;
    if (isCSym(sym)) {
	*wasChanged = NO;
	return sym;
    }
    _MK_MALLOC(newSym,char,strlen(sym)+1);
    convertToCSym(sym,newSym);
    *wasChanged = NO;
    return newSym;
}


/* Higher level interface and specific Music Kit use of name tables.  */

#import <objc/vectors.h>	
#define INT(x) (int) x
 
#import "TuningSystem.h"
#import "Note.h"
#import "_ParName.h"

/* We keep two name tables, one for parsing (private, flat) 
   and one for writing (public, hierarchical). There are also private flat
   local tables used for parsing. */

static _MKBiHash *globalParseNameTable;
static _MKBiHash *mkNameTable = NULL;

id MKRemoveObjectName(id object)
/*
 * Removes theObject from the table. Returns nil. 
 */
{
    if (!mkNameTable)
      _MKCheckInit();
    _MKNameTableRemoveObject(mkNameTable,object);
    return nil;
} 

BOOL MKNameObject(char * name,id object)
/*
 * Adds the object theObject in the table, with name theName.
 * If the object is already named, does 
 * nothing and returns NO. Otherwise returns YES. Note that the name is copied.
 */
{
    /* Always sets BACKHASH bit. */
    BOOL wasChanged;
    BOOL rtnVal;
    if (!mkNameTable)
      _MKCheckInit();
    if (!name)
      return NO;
    name = _MKSymbolize(name,&wasChanged); /* Convert to valid sf name */
    rtnVal = 
      (_MKNameTableAddName(mkNameTable,name,object,object,_MK_BACKHASHBIT,YES)
       != NULL);
    if (wasChanged)
      NX_FREE(name);
    return rtnVal;
}

const char * MKGetObjectName(id object)
/*
 * Returns object name if any. If object is not found, returns NULL. The name
 * is not copied and should not be freed or altered by caller.
 */
{
    id owner;
    if (!mkNameTable || !object)
      return NULL;
    return _MKNameTableGetObjectName(mkNameTable,object,&owner);
}

id MKGetNamedObject(char *name)
/* Returns the first object found in the name table, with the given name.
   Note that the name is not necessarily unique in the table; there may
   be more than one object with the same name.
*/
{
    if (!mkNameTable || !name)
      return nil;
    return _MKNameTableGetFirstObjectForName(mkNameTable,name);
}

const char *_MKGetGlobalName(id object)
/*
 * Returns object name if any. If object is not found, returns NULL. The name
 * is not copied and should not be freed or altered by caller.
 */
{
    id owner;
    if (!globalParseNameTable)
      return NULL;
    return _MKNameTableGetObjectName(globalParseNameTable,object,&owner);
}

void _MKNameGlobal(char * name,id dataObj,unsigned short type,BOOL autoImport,
		   BOOL copyIt)
    /* Copies name */
{
    if (!globalParseNameTable)
      _MKCheckInit();
    if (autoImport)
      type |= _MK_AUTOIMPORTBIT;
    _MKNameTableAddName(globalParseNameTable,name,nil,dataObj,type,copyIt);
}

id _MKGetNamedGlobal(char * name,unsigned short *typeP)
{
    if (!globalParseNameTable)
      return nil;
    return _MKNameTableGetObjectForName(globalParseNameTable,name,nil,typeP);
}

BOOL MKAddGlobalScorefileObject(id object,char *name)
/*
 * Adds the object theObject in the table, referenced in the scorefile
 * with the name specified. The name is copied.
 * If there is already a global scorefile object 
 * with that name, does nothing and returns NO. Otherwise returns YES. 
 * The object does not become visible to scorefiles unless they explicitly
 * do a call of getGlobal.  
 * The type of the object in the scorefile is determined as follows:
 * * If object isKindOf:[WaveTable class], then the type is MK_waveTable.
 * * If object isKindOf:[Envelope class], then the type is MK_envelope.
 * * Otherwise, the type is MK_object.
 */
{
    unsigned short type;
    if (!object || !name)
      return NO;
    type = ([object isKindOf:[Envelope class]]) ? MK_envelope : 
	    ([object isKindOf:[WaveTable class]]) ? MK_waveTable : MK_object;
    if (!globalParseNameTable)
      _MKCheckInit();
    _MKNameTableAddName(globalParseNameTable,name,nil,object,
			type | _MK_BACKHASHBIT,YES);
    return YES;
}

id MKGetGlobalScorefileObject(char *name)
/* Returns the global scorefile object with the given name. The object may
   be either one that was added with MKAddGlobalScorefileObject or it
   may be one that was added by the scorefile itself using "putGlobal".
   Objects accessable to the application are those of type 
   MK_envelope, MK_waveTable and MK_object. An attempt to return some other
   object will return nil.
   Note that the name is not necessarily registered with the Music Kit
   name table.
 */
{
    unsigned short typeP;
    id obj;
    if (!name)
      return nil;
    obj = _MKGetNamedGlobal(name,&typeP);
    switch (typeP) {
      case MK_envelope:
      case MK_waveTable:
      case MK_object:
	return obj;
      default:
	return nil;
    }
}


/* The following is a list of the key words recognized by the scorefile
   parser. */

static const int keyWordsArr[] = {
    /* note types */
    MK_noteDur,MK_mute,MK_noteOn,MK_noteOff,MK_noteUpdate, 
    /* Midi pars */
    MK_resetControllers,MK_localControlModeOn,MK_localControlModeOff,
    MK_allNotesOff,MK_omniModeOff,MK_omniModeOn,MK_monoMode,MK_polyMode,
    MK_sysClock,MK_sysStart,MK_sysContinue,MK_sysStop,MK_sysActiveSensing,
    MK_sysReset,
    /* Other keywords */
    _MK_part,_MK_doubleVarDecl,_MK_stringVarDecl,_MK_tune,
    _MK_intVarDecl,_MK_to,_MK_begin,_MK_end,_MK_include,_MK_comment,
    _MK_endComment,_MK_print,_MK_time,_MK_dB,_MK_ran,
    _MK_envelopeDecl,_MK_waveTableDecl,_MK_objectDecl,_MK_noteTagRange,
    _MK_envVarDecl,_MK_waveVarDecl,_MK_varDecl,_MK_objVarDecl,_MK_info,
    _MK_putGlobal,_MK_getGlobal,_MK_seed,_MK_ranSeed,_MK_repeat,
    _MK_if,_MK_else,_MK_while,_MK_do
    };
    
/* The following define some assumed defaults used as a guess at Set sizes.
   All such Sets are expandable, however. */

#define NUMKEYWORDS (sizeof(keyWordsArr)/sizeof(int))
#define NUMOCTAVES (128/12)
#define NUMPITCHVARS (NUMOCTAVES * (9 + 12)) /* 9 enharmnonic equivalents */
#define NUMKEYVARS (NUMPITCHVARS)
#define NUMOTHERMUSICKITVARS 9 /* Other than KEYVARS and PITCHVARS */
#define NUMMUSICKITVARS (NUMKEYVARS + NUMPITCHVARS + NUMOTHERMUSICKITVARS)
#define NAPPVARSGUESS 15 /* Per file. (This includes parts and envelopes)  */
#define NFILESGUESS 2 /* Num scoreFiles being read at once or sequentially. */
#define GLOBALTABLESIZE (NUMMUSICKITVARS)
#define GLOBALTABLEBACKHASHSIZE (BACKHASHSIZE)
#define LOCALTABLESIZE (NAPPVARSGUESS + GLOBALTABLESIZE)
#define LOCALTABLEBACKHASHSIZE (BACKHASHSIZE)

/* Re. _MK_NOFREESTRINGBIT below:
   Actually, this is not needed, since we never free the elements of the
   global table. But I left it in anyway, to highlight that the string is 
   in the text segment and that horrible things will happen if it's freed */

static id addReadOnlyVar(char * name,int val)
{
    /* Add a read-only variable to the global parse table. */
    id rtnVal;
    _MKNameGlobal(name,rtnVal = 
		  _MKNewScorefileVar(_MKNewIntPar(val,MK_noPar),name,NO,YES),
		  _MK_typedVar | _MK_NOFREESTRINGBIT,YES,NO);
    return rtnVal;
}

static void
initKeyWords()
{
    /* Init the symbol table used to store key words for score file
       parsing. */
    static BOOL inited = NO;
    int i,tok;
    if (inited)
      return;
    addReadOnlyVar("NO",0); 
    addReadOnlyVar("YES",1);
    _MKNameGlobal("PI",
		  _MKNewScorefileVar(_MKNewDoublePar(M_PI,MK_noPar),"PI",NO,
				     YES),
		  (unsigned short)_MK_typedVar | _MK_NOFREESTRINGBIT,YES,NO);
    for (i=0; i<NUMKEYWORDS; i++) {
	tok = (int)(keyWordsArr[i]);
	_MKNameGlobal((char *)_MKTokName(tok),nil,
		      (unsigned short)tok | _MK_NOFREESTRINGBIT,YES,NO);
    }
    inited = YES;
}

void _MKCheckInit(void)
    /* */
{
    if (globalParseNameTable)  /* Been here? */
      return;
    MKSetErrorStream(NULL);
    /* We don't try and use the Appkit error mechanism. It's not well-suited
       to real-time. */
//  NXRegisterErrorReporter( MK_ERRORBASE, MK_ERRORBASE+1000,_MKWriteError);
    globalParseNameTable = _MKNewBiHash(GLOBALTABLESIZE,
					 GLOBALTABLEBACKHASHSIZE);
    mkNameTable = _MKNewBiHash(0,0);
    [[Note new] free]; /* Force initialization. Must be after table creation.*/
    _MKTuningSystemInit();
}

_MKBiHash *
  _MKNewScorefileParseTable(void)
{
    /* Initialize a local symbol table for a new score file to be parsed. 
       Global symbols are not included here. */
    _MKBiHash *localTable = 
      _MKNewBiHash(LOCALTABLESIZE,LOCALTABLEBACKHASHSIZE);
    initKeyWords();     /* Add key words to global symbol table. */
    importAutoGlobals(globalParseNameTable,localTable);
    return localTable;
}

char *_MKUniqueName(char *name,_MKBiHash *table,id anObject,id *hashObj)
    /* Name is assumed malloced. anObject may be nil. This routine
       makes sure that name is not in the table. If it is in the table,
       a new name of the form <oldName><int> is generated. */
{
#   define FIRSTDIGIT 0
    int i;
    int nDigits = 1, nextPower = 10;
    int newSize = strlen(name) + nDigits; 
    unsigned short typeP;
    _MK_REALLOC(name,char,newSize+1);     /* Now expand (1 for NULL) */
    for (i = FIRSTDIGIT; ; i++) { 
	if (i >= nextPower) {                 /* Make more room */
	    newSize++;
	    _MK_REALLOC(name,char,newSize+1); /* 1 for NULL */
	    nextPower *= 10;
	    nDigits++;
	}
	sprintf(&(name[newSize - nDigits]),"%d",i); 
	*hashObj = _MKNameTableGetObjectForName(table,name,nil,&typeP);
	/* Name unused or we found it? */
	if ((!*hashObj) || (*hashObj == anObject)) 
	  return name;
    }
}

