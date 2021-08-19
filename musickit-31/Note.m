#ifdef SHLIB
#include "shlib.h"
#endif

/*** Oft-used Methods to make c funcs

  -conductor
  -_noteOffForNoteDur
  -free
  -unionWith
  -copy
  -timeTag
  -part
  -_setPartLink
  -setNoteTag:
  -noteDur
  -noteType
  -noteTag

***/

/*

  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/18/89/daj - Changed hash table to be non-object variety. Changed parameters
                 to be structs rather than objects. Added C-function access
                 to many methods.
  09/19/89/daj - Changed definition of _ISMKPAR to be a little faster.
  10/06/89/daj - Changed to use new _MKNameTable.
  10/20/89/daj - Added binary scorefile support.
  01/02/90/daj - Flushed possiblyUpdateNoteType()
  03/13/90/daj - Moved private methods to category.
  03/17/90/daj - Fixed bugs in parameter iteration.
  03/19/90/daj - Added MKSetNoteClass() MKGetNoteClass.
                 Changed to use memset to clear _mkPars.
  03/21/90/daj - Added archiving support.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  05/13/90/daj - Simultaneously fixed 2 bugs. The bugs were 1) that unarchived
                 Parts could 'lose' Notes because id of Notes may come in
		 wrong order. 2) that Notes in a Scorefile can get reordered.
		 The fix is to change _reservedNote5 to be an int and use
		 it to store an "orderTag" which is an int assigned by the 
		 Part. This tag is used to disambiguate Notes with the same
		 timeTag. The former use of this field, to hold the 
		 'placeHolder' Notes is implemented as a negative order tag.
		 Note that I changed the semantics of the compare: 
		 method. This is a backward incompatible change.
  08/13/90/daj - Removed extraneous checks for noteClassInited and 
                 parClassInited. Added method +nameOfPar:.
  08/23/90/daj - Changed to zone API.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
*/

#define INT(_x) ((int) _x)

#import <objc/hashtable.h>
#define MK_INLINE 1
#import "_musickit.h"
#import "_tokens.h"
#import "_noteRecorder.h"
#import "_error.h"
#import "_ParName.h"
#import "_MKNameTable.h" 

#import "_Note.h"

@implementation Note:Object 
{
    MKNoteType noteType;    /* The Note's noteType. */
    int noteTag;         /* The Note's noteTag. */
    id performer;      /* Performer object that most recently sent the Note
                          * in performance, if any. */
    id part;            /* The Part that this Note is a member of, if any. */
    double timeTag;     /* Time tag, if any, else MK_ENDOFTIME. */
    void *_reservedNote1;
    unsigned _reservedNote2[MK_MKPARBITVECTS];
    unsigned *_reservedNote3;
    short _reservedNote4;
    int _reservedNote5;
} 

/* METHOD TYPES
 * Creating and freeing a Note
 * Copying and comparing Notes
 * Parameters
 * Timing information
 * Type information
 * Accessing the Note's Part
 * Performance information
 * Displaying the Note
 */

#define _parameters _reservedNote1
#define _mkPars _reservedNote2
#define _appPars _reservedNote3
#define _highAppPar _reservedNote4
#define _orderTag _reservedNote5 
/* _orderTag disambiguates simultaneous notes. If it's negative, 
   it means that the Note is actually slated for deletion. In this case,
   the ordering is the absolute value of _orderTag. */

#if 0
    id _parameters;       /* Set of parameter values. */
    unsigned _mkPars[MK_MKPARBITVECTS]; 
                         /* Bit vectors specifying presence of Music
                          * Kit parameters. */
    unsigned *_appPars;  /* Bit-vector for application-defined parameters. */
    short _highAppPar;  /* Highest bit in \fB_appPars\fR (0 if none). */
#endif


/* Creation, copying, deleting------------------------------------------ */


static id setPar(); /* forward refs */

#define setObjPar(_self,_parameter,_value,_type) \
  setPar(_self,_parameter,_value, _type)
#define setDoublePar(_self,_parameter,_value) \
  setPar(_self,_parameter,& _value, MK_double)
#define setIntPar(_self,_parameter,_value) \
  setPar(_self,_parameter,& _value, MK_int)
#define setStringPar(_self,_parameter,_value) \
  setPar(_self,_parameter, _value, MK_string)

static BOOL isParPresent(); /* forward ref */

#define DEFAULTPARSSETSIZE 5

static BOOL parClassInited = NO;

#define ISPAR(_x) (ISMKPAR(_x) || ISAPPPAR(_x))

#define ISMKPAR(_x) \
((((int)(_x))>((int)MK_noPar))&&(((int)(_x))<((int)MK_privatePars)))

#define ISAPPPAR(_x)  \
((((int)(_x))<=(_MKHighestPar()))&&(((int)(_x))>=((int) _MK_FIRSTAPPPAR)))

#define ISPRIVATEPAR(_x) \
  ((((int)(_x))>((int)MK_privatePars))&&(((int)(_x))<((int)MK_appPars)))

#define _ISMKPAR(_x) ((((int)(_x))>((int)MK_noPar))&&(((int)(_x))<((int)MK_appPars)))

#define _ISPAR(_x) (_ISMKPAR(_x) || ISAPPPAR(_x))

static id theSubclass = nil;

BOOL MKSetNoteClass(id aClass)
{
    if (!_MKInheritsFrom(aClass,[Note class]))
      return NO;
    theSubclass = aClass;
    return YES;
}

id MKGetNoteClass(void)
{
    if (!theSubclass)
      theSubclass = [Note class];
    return theSubclass;
}

+(int)parName:(char *)aName
  /* Returns the par int corresponding to the given name. If a parameter
     with aName does not exist, creates a new one. */
{
    id aPar;
    return _MKGetPar(aName,&aPar);
}    

+(char *)nameOfPar:(int)aPar
  /* Returns the name corresponding to the parameter number given.
     If the parameter number given is not a valid parameter number,
     returns "". Note that the string is not copied. */
{
    if (_MKIsPar(aPar))
      return _MKParNameStr(aPar);
    else return "";
}    

#define DEFAULTNUMPARS 1

#define HASHTABLECACHESIZE 16
static NXHashTable *hashTableCache[HASHTABLECACHESIZE];
static unsigned hashTableCachePtr = 0;

#define PARNUM(_x) ((_MKParameter *)_x)->parNum

static unsigned hashFun(const void *info, const void *data)
{
    return (unsigned)PARNUM(data);
}

static int isEqualFun(const void *info, const void *data1, const void *data2)
{
    return (PARNUM(data1) == PARNUM(data2));
}

static void freeFun(const void *info, void *data)
{
    _MKFreeParameter(data);
}

static NXHashTablePrototype htPrototype = {hashFun,isEqualFun,freeFun,0};

static NXHashTable *allocHashTable(void)
    /* alloc a new table. */
{
    if (hashTableCachePtr) 
      return hashTableCache[--hashTableCachePtr]; 
    else return NXCreateHashTable(htPrototype,DEFAULTNUMPARS,NULL);
}

static NXHashTable *freeHashTable(NXHashTable *tab)
{
    if (!tab)
      return NULL;
    if (tab) {
        if (hashTableCachePtr < HASHTABLECACHESIZE) {
            /* We only need to free elements if we're not freeing 
               the table. If we do free the table, we free elements in freeFun 
               above */
            _MKParameter *data;
            NXHashState state;
            state = NXInitHashState(tab);
            while (NXNextHashState (tab, &state, (void **)&data))
              _MKFreeParameter(data); /* Free all parameters. */
            NXEmptyHashTable(tab);
            hashTableCache[hashTableCachePtr++] = tab;
        }
        else NXFreeHashTable(tab); 
    }
    return NULL;
}

static _MKParameter *dummyPar = NULL;
static id noteClass = nil;

#define VERSION2 2

static void initNoteClass()
{
    noteClass = [Note class];
    if (!parClassInited)
      parClassInited = _MKParInit();
    dummyPar = _MKNewIntPar(0,MK_noPar);
}

+initialize
{
    if (!noteClass)
      initNoteClass();
    if (self != [Note class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

#define NOTECACHESIZE 8
static id noteCache[NOTECACHESIZE];
static unsigned noteCachePtr = 0;

+newSetTimeTag:(double)aTimeTag
  /* TYPE: Creating; Creates a new Note and sets its timeTag.
   * Creates and initializes a new (mute) Note object 
   * with a timeTag value of \fIaTimeTag\fR.
   * If \fIaTimeTag\fR is \fBMK_ENDOFTIME,\fR the timeTag isn't set.
   * Returns the new object.
   */
{
    if (self == noteClass && noteCachePtr) { 
	/* We initialize reused Notes here. */
	self = noteCache[--noteCachePtr]; 
	_parameters = NULL;
	memset(&(_mkPars[0]), 0, MK_MKPARBITVECTS * sizeof(_mkPars[0])); 
	_highAppPar = 0;
	_appPars = NULL;
    }
    else 
      self = [super allocFromZone:NXDefaultMallocZone()];
    [self initWithTimeTag:aTimeTag];
    return self;
}

-initWithTimeTag:(double)aTimeTag
{
/*    [super init]; Not needed -- we omit in this case, for efficiency */
    noteTag = MAXINT;
    noteType = MK_mute;
    timeTag = aTimeTag;
    return self;
}

-init
{
    return [self initWithTimeTag:MK_ENDOFTIME];
}

+new 
  /* TYPE: Creating; Creates a new Note object.
   * Creates, initializes and returns a new Note object.
   * Implemented as \fBnewSetTimeTag:MK_ENDOFTIME\fR.
   */
{
    return [self newSetTimeTag:MK_ENDOFTIME];
}

#if 0
/* Might want to add this some day */
- empty
{
    freeHashTable((NXHashTable *)_parameters); /* Frees all parameters */
    _parameters = NULL;
    /* Clear bit vectors */
    memset(&(_mkPars[0]), 0, MK_MKPARBITVECTS * sizeof(_mkPars[0])); 
    if (_highAppPar) {
        NX_FREE(_appPars);
        _appPars = NULL;
    }
    _highAppPar = 0;
    return self;
}
#endif

- free
  /* TYPE: Creating; Frees the receiver and its contents.
   * Removes the receiver from its Part, if any, and then frees the
   * receiver and its contents.
   * Envelope and WaveTable objects are not freed.
   */
{
    [part removeNote:self];
    freeHashTable((NXHashTable *)_parameters);
    if (_highAppPar) 
      NX_FREE(_appPars);
    if (((Object *)(self->isa)) == noteClass)
      if (noteCachePtr < NOTECACHESIZE) {
          noteCache[noteCachePtr++] = self;
      } else [super free];
    else [super free];
    return nil;
}


static int nAppBitVects(); /* forward ref */

- copyFromZone:(NXZone *)zone
  /* TYPE: Copying; Returns a new Note as a copy of the receiver.
   * Creates and returns a new Note object as a copy of the receiver.
   * The new Note's parameters, timing information, noteType and noteTag
   * are the same as the receiver's.
   * However, it isn't added to a Part 
   * regardless of the state of the receiver.
   *
   * Note: Envelope and WaveTable objects aren't copied, the new Note shares the
   * receiver's Envelope objects.
   */
{
    Note *newObj;
    _MKParameter *aPar;
    NXHashState state;
    int i, vectorCount;
    newObj = [super copyFromZone:zone];
    newObj->part = nil;

//  newObj->performer = nil; 
    /* E.g. SynthInstrument copies Notes as they're performed. We must copy the
       performer field. Otherwise, it'll be unable to respond correctly to the
       conductor message. */

    if (!_parameters) 
      return newObj;
    state = NXInitHashState((NXHashTable *)_parameters);
    newObj->_parameters =  /* Don't use allocHashtable 'cause we know size */
      NXCreateHashTable(htPrototype,
                        NXCountHashTable((NXHashTable *)_parameters),NULL);
    while (NXNextHashState((NXHashTable *)_parameters,&state,(void **)&aPar))
      NXHashInsert((NXHashTable *)newObj->_parameters,
                   (void *)_MKCopyParameter(aPar));

//  for (i = 0; i < MK_MKPARBITVECTS; i++) newObj->_mkPars[i] = _mkPars[i];
    /* Not needed -- handled by super class copy */

    vectorCount = nAppBitVects(self);
    if (vectorCount) {
        _MK_MALLOC(newObj->_appPars, unsigned, vectorCount);
        for (i = 0; i < vectorCount; i++)
          newObj->_appPars[i] = _appPars[i];
    }
    return newObj;
}

- copy
{
    return [self copyFromZone:[self zone]];
}

-split:(id *)aNoteOn :(id *)aNoteOff
  /* TYPE: Type; Splits the noteDur receiver into two Notes.
   * If receiver isn't a noteDur, returns \fBnil\fR.   Otherwise,
   * creates two new Notes (a noteOn and a noteOff), splits the information
   * in the receiver between the two of them, places the new Notes in the
   * arguments,
   * and returns the receiver
   * (which is neither freed
   * nor otherwise affected).
   *
   * The receiver's parameters are copied into the new noteOn
   * except for \fBMK_relVelocity\fR which, if present
   * in the receiver, is copied into the noteOff.
   * The noteOn takes the receiver's timeTag while the noteOff's
   * timeTag is that of the receiver plus its duration.
   * If the receiver has a noteTag, it's copied into the two new Notes;
   * otherwise a new noteTag is generated for them.
   * The new Notes are added to the Part of the receiver, if any.
   *
   * Note: Envelope objects aren't copied, the new Notes share the
   * receiver's Envelope objects.
   */
{
    *aNoteOff = [(*aNoteOn = [self copyFromZone:NXDefaultMallocZone()]) 
		 _splitNoteDurNoCopy];
    if (*aNoteOff)
      return self;
    return nil;
}

static double getNoteDur(id aNote);



/* Perfomers, Conductors, Parts */


-performer      
  /* TYPE: Perf; Returns the Performer that last sent the Note.
   * Returns the Performer that most recently sent the receiver
   * during performance.  
   */
{
        return performer;
}

-part     
  /* TYPE: Acc; Return the receiver's Part.
   * Returns the Part that the receiver is a member of, or \fBnil\fR if none.
   */
{
    return part;
}

-conductor
  /* TYPE: Perf; Returns the receiver's Performer's Conductor.
   * Returns the Conductor of the receiver's Performer.
   * The Performer is determined by
   * sending \fBperformer\fR to the receiver.
   */
{
    id aCond;
    aCond = [performer conductor];
    return aCond ? aCond : [_MKClassConductor() defaultConductor];
}

- addToPart:aPart
  /* TYPE: Acc; Adds the receiver to \fIaPart\fR.
   * Removes the receiver from the Part that it's currently 
   * a member of and adds it to \fIaPart\fR.
   * Returns the receiver's old Part, if any. 
   */
{
    id oldPart = part;
    [aPart addNote:self];
    return oldPart;
}

- (double)timeTag
  /* TYPE: Timing; Returns the receiver's timeTag.
   * Returns the receiver's timeTag.  If the timeTag isn't set, 
   * returns \fBMK_ENDOFTIME\fR.
   */
{ 
    return timeTag;
}

- (double) setTimeTag:(double)newTimeTag    
  /* TYPE: Timing; Sets the receiver's timeTag.
   * Sets the receiver's timeTag to \fInewTimeTag\fR and returns the old 
   * timeTag, or \fBMK_ENDOFTIME\fR if none. 
   * If \fInewTimeTag\fR is negative, it's clipped to 0.0.
   *
   * Note:  If the receiver is a member of a Part, 
   * it's first removed from the Part,
   * the timeTag is set, and then it's
   * re-added in order to ensure that its ordinal position within the
   * Part is correct.
   */
{ 
    double tmp = timeTag;
    id aPart = part;    /* Save it because remove causes it to be set to nil */
    newTimeTag = MIN(MAX(newTimeTag,0.0),MK_ENDOFTIME); 
    [aPart removeNote:self];
    timeTag = newTimeTag;
    [aPart addNote:self];
    return tmp; 
}

- removeFromPart
  /* TYPE: Acc; Removes the receiver from its Part.
   * Removes the receiver from its Part, if any.
   * Returns the old Part, or \fBnil\fR if none. 
   */
{
    id tmp = part;
    if (part) {
        [part removeNote:self];
        part = nil;
    }
    return tmp;
}

void _MKMakePlaceHolder(Note *aNote)
{
    if (aNote->_orderTag > 0)
      aNote->_orderTag = -aNote->_orderTag;
}

BOOL _MKNoteIsPlaceHolder(Note *aNote)
{
    return (aNote->_orderTag < 0);
}

#ifndef ABS
#define ABS(_x) ((_x < 0) ? -_x : _x)
#endif

int _MKNoteCompare(const void *el1,const void *el2)
    /* This must match code in Note. (Or move this function into Note.) */
{
    Note *id1 = *((Note **)el1);
    Note *id2 = *((Note **)el2);
    double t1,t2;
    if (id1 == id2) 
      return 0;
    if (!id1)      /* Push nils to the end. */
      return 1;
    if (!id2)    
      return -1;
    t1 = id1->timeTag;
    t2 = id2->timeTag;
    if (t2 == t1)   /* Same time */
      return ((ABS(id1->_orderTag)) < (ABS(id2->_orderTag))) ? -1 : 1;
    return (t1 < t2) ? -1 : 1;
}

- (int)compare:aNote  
  /* TYPE: Copying; Compares the receiver with \fBaNote\fR.
   * Compares the receiver with \fIaNote\fR and returns a value as follows:
   *
   *  * If the receiver's timeTag < \fIaNote\fR's timeTag, returns -1.
   *  * If the receiver's timeTag > \fIaNote\fR's timeTag, returns 1.
   *
   * If their timeTags are equal, the two objects are compared 
   * by their order in the Part.
   *
   * This comparison indicates the order in which the
   * two Notes would be stored if they were members of the same Part.
   */
{ 
    /* We must only return 0 in one case: when aNote == the receiver.
       This insures that the binary tree will be correctly maintained. */
    if (![aNote isKindOf:noteClass])
      return -1;
    return _MKNoteCompare(&self,&aNote);
}

/* NoteType, duration, noteTag ---------------------------------------------- */

-(MKNoteType) noteType
  /* TYPE: Type; Returns the receiver's noteType.
   * Returns the receiver's noteType.
   */
{
    return noteType;
}

#define ISNOTETYPE(_x) ((int)_x >= (int)MK_noteDur && (int)_x <= (int)MK_mute)

- (id)setNoteType :(MKNoteType) newNoteType
 /* TYPE: Type; Sets the receiver's noteType to \fInewNoteType\fR.
  * Sets the receiver's noteType to \fBnewNoteType\fR,
  * which must be one of:
  *
  *  * MK_noteDur
  *  * MK_noteOn
  *  * MK_noteOff
  *  * MK_noteUpdate
  *  * MK_mute
  *
  * Returns the receiver or nil if newNoteType is not a legal noteType.
  */
{
    if (ISNOTETYPE(newNoteType)) {
        noteType = newNoteType;
        return self;
    }
    else return nil;
}

-(double) setDur:(double) value
  /* TYPE: Timing; Sets the receiver's duration to \fIvalue\fR.
   * Sets the receiver's duration to \fIvalue\fR beats
   * and sets its 
   * noteType to \fBMK_noteDur\fR.
   * \fIvalue\fR must be positive.
   * Returns \fIvalue\fR.
   */
{       
    if (value < 0)
      return MK_NODVAL;
    noteType = MK_noteDur;
    setDoublePar(self,_MK_dur,value);
    return value;
}

void _MKSetNoteDur(Note *aNote,double dur)
{
    aNote->noteType = MK_noteDur;
    setDoublePar(aNote,_MK_dur,dur);
}

static double getNoteDur(Note *aNote)
{
    if (aNote->noteType != MK_noteDur) 
      return MK_NODVAL;
    return MKGetNoteParAsDouble(aNote,_MK_dur);
}

-(double)dur
  /* TYPE: Timing; Returns the receiver's duration.
   * Returns the receiver's duration, or \fBMK_NODVAL\fR if
   * it isn't set or if the receiver noteType isn't \fBMK_noteDur\fR.
   */
{       
    return getNoteDur(self);
}

- (int)noteTag
 /* TYPE: Type; Returns the receiver's noteTag.
  * Return the receiver's noteTag, or \fBMAXINT\fR if it isn't set.
  */
{
    return noteTag;
}

- setNoteTag:(int)newTag
 /* TYPE: Type; Sets the receiver's noteTag to \fInewTag\fR.
  * Sets the receiver's noteTag to \fInewTag\fR;
  * if the noteType is \fBMK_mute\fR 
  * it's automatically changed to \fBMK_noteUpdate\fR.
  * Returns the receiver.
  */
{
    noteTag = newTag;
    if (noteType == MK_mute)
      noteType = MK_noteUpdate;
    return self;
}

/* Raw setting methods */
void _MKSetNoteType(Note *aNote,MKNoteType aType)
{
    aNote->noteType = aType;
}

void _MKSetNoteTag(Note *aNote,int aTag)
{
    aNote->noteTag = aTag;
}


/* Parameters ------------------------------------------------- */


static BOOL _isMKPar(aPar)
    unsigned aPar;
    /* Returns YES if aPar is a public or private musickit parameter */
{
    return _ISMKPAR(aPar);
}

BOOL _MKIsPar(unsigned aPar)
    /* Returns YES if aPar is a public parameter */
{
    return ISPAR(aPar);
}


#if 0
static BOOL isPublicOrPrivatePar(unsigned aPar)
    /* Returns YES if aPar is a public or private parameter */
{
    return _ISPAR(aPar);
}
#endif

BOOL _MKIsPrivatePar(unsigned aPar)
    /* Returns YES if aPar is a private parameter */
{
    return ISPRIVATEPAR(aPar);
}

/* We represent pars in 2 bit vector arrays, one for MK pars,
   the other for app pars. 
   The 0th MK par is not used so the LSB is always 0 in the MK low-order
   bit vector. The number of MK bit vectors is defined in musickit.h as
   big enough to hold the highest MK par, but not the place-holder
   MK_appPars. 
   */

static BOOL isParPresent(aNote,aPar)
    Note *aNote;
    unsigned aPar;
    /* Returns whether or not param is present. ANote is assumed to be a 
       Note object. */
{
    if (!aNote->_parameters)
      return NO;
    if (_ISMKPAR(aPar))
      return ((aNote->_mkPars[aPar / 32] & 
               (1 << (aPar % 32))) != 0);
    else if (aNote->_highAppPar != 0)
      return ((aPar <= aNote->_highAppPar) && 
            ((aNote->_appPars[aPar / 32 - MK_MKPARBITVECTS] &
             (1 << (aPar % 32))) != 0));
    else return NO;    
}

static int nAppBitVects(self)
    Note *self;
    /* Assumes we have an appBitVect. */
{
    return  (self->_highAppPar) ? 
      (self->_highAppPar - _MK_FIRSTAPPPAR)/32 + 1 : 0;
}

static BOOL setParBit(self,aPar)
    Note *self;
    unsigned aPar;
    /* Returns whether or not param is present and sets bit. */
{
    register unsigned bitVectIndex = aPar / 32;
    register unsigned modBit = aPar % 32;
    if (!self->_parameters)
      self->_parameters = allocHashTable();
    if (_ISMKPAR(aPar)) {
        if (self->_mkPars[bitVectIndex] & (1 << modBit))
           return YES;
        self->_mkPars[bitVectIndex] |= (1 << modBit);
        return NO;
    }
    bitVectIndex -= MK_MKPARBITVECTS;
    if (aPar > self->_highAppPar) {
        int i;
        if (self->_highAppPar != 0)
           _MK_REALLOC(self->_appPars,unsigned,(bitVectIndex + 1));
        else 
           _MK_MALLOC(self->_appPars,unsigned,(bitVectIndex + 1));
        for (i = nAppBitVects(self); i < bitVectIndex; i++)
          self->_appPars[i] = 0; /* Zero out added bit vects. */
        self->_highAppPar = (bitVectIndex + MK_MKPARBITVECTS + 1)
                 * 32 - 1;
        self->_appPars[bitVectIndex] = (1 << modBit);
        return NO;
    }
    if (self->_appPars[bitVectIndex] & (1 << modBit))
      return YES;
    self->_appPars[bitVectIndex] |= (1 << modBit);
    return NO;
}

/* Clears param bit if set. Returns whether param was present. */

static BOOL clearParBit(self,aPar)
    Note *self;
    unsigned aPar;
{
    unsigned thisBitVect = aPar / 32;
    unsigned modBit = aPar % 32;
    if (_isMKPar(aPar)) {
        BOOL wasSet = ((self->_mkPars[thisBitVect] & (1 << modBit)) > 0);
        self->_mkPars[thisBitVect] &= (~(1 << modBit));
        return wasSet;
    }
    if (self->_highAppPar && aPar <= self->_highAppPar) {
        self->_appPars[thisBitVect - MK_MKPARBITVECTS] &= (~(1 << modBit));
        return YES;
    }
    return NO;
}

id MKSetNoteParToDouble(Note *aNote,int par,double value)
{
    if (!_MKIsPar(par))
      return nil;
    return setDoublePar(aNote,par,value);
}

-setPar:(int)par toDouble:(double) value
  /* TYPE: Parameters; Sets parameter \fIpar\fR to \fBdouble\fR \fIvalue\fR.
   * Sets the parameter \fIpar\fR to \fIvalue\fR, which must be a
   * \fBdouble\fR.
   * Returns the receiver.
   */
{
    return MKSetNoteParToDouble(self,par,value);
}

id MKSetNoteParToInt(Note *aNote,int par,int value)
{
    if (!_MKIsPar(par))
      return nil;
    return setIntPar(aNote,par,value);
}

-setPar:(int)par toInt:(int) value
  /* TYPE: Parameters;  Sets parameter \fIpar\fR to \fBint\fR \fIvalue\fR.
   * Set the parameter \fIpar\fR to \fIvalue\fR, which must be an \fBint\fR.
   * Returns the receiver.
   */
{       
    return MKSetNoteParToInt(self,par,value);
}

id MKSetNoteParToString(Note *aNote,int par,char *value)
{
    if (!_MKIsPar(par))
      return nil;
    return setStringPar(aNote,par,value);
}

-setPar:(int)par toString:(char *) value
  /* TYPE: Parameters; Sets parameter \fIpar\fR to a copy of \fIvalue\fR.
   * Set the parameter \fIpar\fR to a copy of \fIvalue\fR, which must be 
   * a \fBchar *\fR.
   * Returns the receiver.
   */
{
    return MKSetNoteParToString(self,par,value);
}

id MKSetNoteParToEnvelope(Note *aNote,int par,id envObj)
{
    if (!_MKIsPar(par))
      return nil;
    return setObjPar(aNote,par,envObj,MK_envelope);
}

-setPar:(int)par toEnvelope:(id)envObj
  /* TYPE: Parameters; Sets parameter \fIpar\fR to the Envelope \fIenvObj\fR.
   * Points the parameter \fIpar\fR to 
   * \fIenvObj\fR (\fBenvObj\fR isn't copied).
   * Scaling and offset information is retained. 
   * Returns the receiver.
   */
{
    return MKSetNoteParToEnvelope(self,par,envObj);
}

id MKSetNoteParToWaveTable(Note *aNote,int par,id waveObj)
{
    if (!_MKIsPar(par))
      return nil;
    return setObjPar(aNote,par,waveObj,MK_waveTable);
}

-setPar:(int)par toWaveTable:(id)waveObj
  /* TYPE: Parameters; Sets parameter \fIpar\fR to the WaveTable \fIwaveObj\fR.
   * Points the parameter \fIpar\fR to 
   * \fIwaveObj\fR (\fBwaveObj\fR isn't copied).
   * Returns the receiver.
   */
{
    return MKSetNoteParToWaveTable(self,par,waveObj);
}


id MKSetNoteParToObject(Note *aNote,int par,id anObj)
{
    if (!_MKIsPar(par))
      return nil;
    return setObjPar(aNote,par,anObj,MK_object);
}

-setPar:(int)par toObject:(id)anObj
  /* TYPE: Parameters; Sets parameter \fIpar\fR to the specified object.
   * Points the parameter \fIpar\fR to 
   * \fIanObj\fR (\fBanObj\fR isn't copied).
   * The object may be of any class, but must be able to write itself
   * out in ASCII when sent the message -writeASCIIStream:.
   * It may write itself any way it wants, as long as it can also read
   * itself when sent the message -readASCIIStream:.
   * The only restriction on these methods is that the ASCII representation
   * should not contain the character ']'.
   * The header file scorefileObject.h is an abstract interface to this
   * protocol.
   * Note that no Music Kit classes implement these methods. The support
   * in Note is provided purely for outside extensibility.
   * Returns the receiver.
   */
{
    return MKSetNoteParToObject(self,par,anObj);
}


#define SETDUMMYPAR(_par) dummyPar->parNum = _par;

/*** FIXME Might be faster to just do the hash here, rather than calling 
  isParPresent? ***/
#define GETPAR(self,_getFun,_noVal) \
if (!self || !isParPresent(self,par)) return _noVal; \
SETDUMMYPAR(par); \
return _getFun(NXHashGet((NXHashTable *)self->_parameters,\
                         (const void *)dummyPar))

double MKGetNoteParAsDouble(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsDouble,MK_NODVAL);
}

-(double)parAsDouble:(int)par
 /* TYPE: Parameters; Returns the value of \fIpar\fR as a \fBdouble\fR.
  * Returns a \fBdouble\fR value converted from the value
  * of the parameter \fIpar\fR. 
  * If the parameter isn't present, returns \fBMK_NODVAL\fR.
  */
{
    return MKGetNoteParAsDouble(self,par);
}

int MKGetNoteParAsInt(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsDouble,MAXINT);
}

-(int)parAsInt:(int)par
 /* TYPE: Parameters; Returns the value of \fIpar\fR as an \fBint\fR.
  * Returns an \fBint\fR value converted from the value
  * of the parameter \fIpar\fR. 
  * If the parameter isn't present, returns \fBMAXINT\fR.
  */
{
    return MKGetNoteParAsInt(self,par);
}

char *MKGetNoteParAsString(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsString,_MKMakeStr(""));
}

-(char *)parAsString:(int)par
  /* TYPE: Parameters; Returns a copy of the value of \fIpar\fR as a \fBchar *\fR.
  * Returns a \fBchar *\fR converted from a copy of the value
  * of the parameter \fIpar\fR. 
  * If the parameter isn't present, returns a copy of "". 
  */
{
    return MKGetNoteParAsString(self,par);
}

char *MKGetNoteParAsStringNoCopy(Note *aNote,int par)
{
    if (!aNote || !isParPresent(aNote,par)) 
      return (char *)_MKUniqueNull(); 
    SETDUMMYPAR(par);
    return _MKParAsStringNoCopy(NXHashGet((NXHashTable *)aNote->_parameters,
                                          (const void *)dummyPar));
}

-(char *)parAsStringNoCopy:(int)par
  /* TYPE: Parameters; Returns the value of \fIpar\fR as a \fBchar *\fR.
  * Returns a \fBchar *\fR to the value of the
  * parameter \fIpar\fR.
  * You shouldn't delete, alter, or store the value 
  * returned by this method.
  * If the parameter isn't present, returns "". 
  * Note that the strings returned are 'unique' in the sense that
  * they are returned by NXUniqueString(). See hashtable.h for details.
  */
{
    return MKGetNoteParAsStringNoCopy(self,par);
}


id MKGetNoteParAsEnvelope(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsEnv,nil);
}

-parAsEnvelope:(int)par
  /* TYPE: Parameters; Returns \fIpar\fR's Envelope object.
   * If \fIpar\fR is an envelope, returns its Envelope object.
   * Otherwise returns \fBnil\fR.
   */
{
    return MKGetNoteParAsEnvelope(self,par);
}

id MKGetNoteParAsWaveTable(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsWave,nil);
}

-parAsWaveTable:(int)par
  /* TYPE: Parameters; Returns \fIpar\fR's WaveTable object.
   * If \fIpar\fR is a waveTable, returns its WaveTable object.
   * Otherwise returns \fBnil\fR.
   */
{
    return MKGetNoteParAsWaveTable(self,par);
}

id MKGetNoteParAsObject(Note *aNote,int par)
{
    GETPAR(aNote,_MKParAsObj,nil);
}

-parAsObject:(int)par
  /* TYPE: Parameters; Returns \fIpar\fR's object.
   * If \fIpar\fR is an object (including an Envelope or WaveTable), 
   * returns its object. Otherwise returns \fBnil\fR.
   */
{
    return MKGetNoteParAsObject(self,par);
}

BOOL MKIsNoteParPresent(Note *aNote,int par)
{
    return isParPresent(aNote,par);
}

-(BOOL)isParPresent:(int)par
  /* TYPE: Parameters; Checks for presence of \fIpar\fR.
   * Returns \fBYES\fR if the parameter \fIpar\fR is present in the
   * receiver (i.e. if its value has been set), \fBNO\fR if it isn't.
   */
{
    return isParPresent(self,par);
}

-(MKDataType)parType:(int)par
   /* TYPE: Parameters; Returns the data type of \fIpar\fR.
    * Returns the parameter data type of \fIpar\fR
    * as an \fBMKDataType\fR:
    * 
    *  * MK_double 
    *  * MK_int
    *  * MK_string
    *  * MK_envelope
    *  * MK_waveTable
    *  * MK_object
    * 
    * If the parameter isn't present, returns \fB_MK_undef\fR.
    */
{
    if (!isParPresent(self,par)) 
      return MK_noType;
    SETDUMMYPAR(par);
    return ((_MKParameter *)NXHashGet((NXHashTable *)_parameters,
                                      (const void *)dummyPar))->_uType;
}

-removePar:(int)par
  /* TYPE: Parameters; Removes \fIpar\fR form the receiver.
   * Removes the parameter \fIpar\fR from the receiver.
   * Returns the receiver if the parameter was present, otherwise
   * returns \fBnil\fR.
   */
{
    _MKParameter *aParameter;
    if (!_MKIsPar(par) || !clearParBit(self,par))
      return nil;
    SETDUMMYPAR(par);
    aParameter = (_MKParameter *)NXHashRemove((NXHashTable *)_parameters,
                                              (const void *)dummyPar);
    _MKFreeParameter(aParameter);
    return self;
}

-(unsigned)parVector:(unsigned)index
  /* TYPE: Parameters; Checks presence of a number of parameters at once.
   * Returns a bit vector indicating the presence of parameters 
   * identified by integers (\fIindex\fR * 32) through 
   * ((\fIindex\fR  + 1) * 32 - 1). For example,
   *
   * .ib
   * unsigned int parVect = [aNote checkParVector:0];
   * .iq
   *
   * returns the vector for parameters 0-31.
   * An argument of 1 returns the vector for parameters 32-63, etc.
   */
{
    return (index < MK_MKPARBITVECTS) ?  _mkPars[index] :
      (index >= (nAppBitVects(self) + MK_MKPARBITVECTS)) ? 0 :
        _appPars[index - MK_MKPARBITVECTS];
}

-(int)parVectorCount
   /* TYPE: Parameters; Returns the # of bit vectors for parameters.
    * Returns the number of bit vectors (\fBunsigned int\fRs)
    * indicating presence of parameters in the receiver. 
    * 
    * See also \fBcheckParVector:\fR.
    */
{
    return MK_MKPARBITVECTS + nAppBitVects(self);
}

static void copyPars(); /* forward ref */

-copyParsFrom:aNote
  /* TYPE: Copying; Copies parameters and dur (if any) from \fIaNote\fR to receiver.
   * Copies \fIaNote\fR's parameters and duration into
   * the receiver (Envelope and WaveTables and other objects are shared rather than copied).
   * Overwrites such values already present in the receiver.
   * Returns the receiver.
   */
  /* (cf. unionWith) */
{
    copyPars(self,aNote,YES);
    return self;
}

/* Parameter getting methods which provide defaults. --------------- */

#if 0
-(double)amp
  /* TYPE: Parameters; Returns the receiver's amplitude.
   * Returns the value of \fBMK_amp\fR if present. 
   * If not, the return value is derived from \fBMK_velocity\fR thus:
   * 
   *    \fBVel\fR       \fBAmp\fR       \fBMeaning\fR
   *    127     1.0 (almost)    Maximum representable amplitude
   *    64        .1    mezzo-forte
   *    0       0.0     minus infinity dB
   *
   * If \fBMK_velocity\fR is missing, returns \fBMAXDOUBLE\fR.
   */
{
    int velocity;
    double ampVal = [self parAsDouble:MK_amp];
    if (ampVal != MAXDOUBLE) 
      return MIN(MAX(ampVal,0.0),1.0);
    velocity = [self parAsInt:MK_velocity];
    if (velocity == MAXINT)
      return MAXDOUBLE;
    return MKMidiToAmp(velocity);
}

-(int)velocity
  /* TYPE: Parameters; Returns the receiver velocity.
   * Returns the value of the \fBMK_velocity\fR parameter if present.
   * If not, derives a return value from \fBMK_amp\fR (see
   * \fBamp\fR for the conversion table).
   *
   * If \fBamp\fR is absent,returns \fBMAXINT\fR.
   */
{
    double similarPar;
    int midiVel = [self parAsInt:MK_velocity];
    if (midiVel != MAXINT)
      return MAX(0,MIDI_DATA(midiVel));
    if ((similarPar = [self parAsDouble:MK_amp]) != MAXDOUBLE)
      return MKAmpToMidi(similarPar);
    return MAXINT;
}
#endif

-(double)freq
  /* TYPE: Parameters; Returns the frequency of the receiver.
   * Returns the value of \fBMK_freq\fR, if present.  If not,
   * converts and returns the value of \fBMK_keyNum\fR.
   * And if that parameter is missing, returns \fBMK_NODVAL\fR.
   */
{
    int keyNumVal;
    double freqVal = MKGetNoteParAsDouble(self,MK_freq);
    if (!MKIsNoDVal(freqVal))
      return freqVal;
    keyNumVal = MKGetNoteParAsInt(self,MK_keyNum);
    if (keyNumVal == MAXINT)
      return MK_NODVAL;
    return MKKeyNumToFreq(keyNumVal);
}

-(int)keyNum
  /* TYPE: Parameters; Returns the key number of the recevier.
   * Returns the value of \fBMK_keyNum\fR, if present.  If not,
   * converts and returns the value of \fBMK_freq\fR.
   * If \fBMK_freq\fR isn't present, returns \fBMAXINT\fR.
   */
{
    int keyNum;
    keyNum = MKGetNoteParAsInt(self,MK_keyNum);
    if (keyNum != MAXINT)
      return MIDI_DATA(MAX(0,keyNum));
    {
        double freqPar = MKGetNoteParAsDouble(self,MK_freq);
        if (MKIsNoDVal(freqPar))
          return MAXINT;
        return MKFreqToKeyNum(freqPar,NULL,0);
    }
}


/* Writing ------------------------------------------------------------ */

#define BINARY(_p) (_p && _p->_binary)

void _MKWriteParameters(Note *self,NXStream *aStream,_MKScoreOutStruct *p)
{
    _MKParameter *aPar;
    NXHashState state;
    if (self->_parameters) {
        state = NXInitHashState((NXHashTable *)self->_parameters);
        if (BINARY(p)) {
            while (NXNextHashState((NXHashTable *)self->_parameters,
                                   &state,(void **)&aPar))
              _MKParWriteOn(aPar,aStream,p);
        }
        else {
            int parCnt = 0;
            while (NXNextHashState((NXHashTable *)self->_parameters,
                                   &state,(void **)&aPar))
              if (_MKIsParPublic(aPar)) {  /* Private parameters don't print */
#                 if _MK_LINEBREAKS
#                 define PARSPERLINE 5
                  NXPrintf(aStream,", "); 
                  if (++parCnt > PARSPERLINE) {
                      parCnt = 0; 
                      NXPrintf(aStream,"\n\t");
                  }
#                 else
                  if (parCnt++) 
                    NXPrintf(aStream,", "); 
#                 endif
                  _MKParWriteOn(aPar,aStream,p);
              }
        }
    }
    if (BINARY(p))
      _MKWriteInt(aStream,MAXINT);
    else NXPrintf(aStream,";\n");
}

#import <objc/HashTable.h>

static id writeBinaryNoteAux(Note *self,id aPart,_MKScoreOutStruct *p)
{
    NXStream *aStream = p->_stream;
    _MKWriteShort(aStream,_MK_partInstance);
    _MKWriteShort(aStream,
                  (int)[p->_binaryIndecies valueForKey:(const void *)aPart]);
    switch (self->noteType) {
      case MK_noteDur: {
          double dur = 
            (p->_ownerIsNoteRecorder) ? 
              _MKDurForTimeUnit(self,[p->_owner timeUnit]) : 
                getNoteDur(self);
          _MKWriteShort(aStream,MK_noteDur);
          _MKWriteDouble(aStream,dur);
          _MKWriteInt(aStream,self->noteTag);
          break;
      }
      case MK_noteOn:
      case MK_noteOff:
      case MK_noteUpdate:
        _MKWriteShort(aStream,self->noteType);
        _MKWriteInt(aStream,self->noteTag);
        break;
      case MK_mute:
      default:
        _MKWriteInt(aStream,self->noteType);
        break;
    }
    _MKWriteParameters(self,aStream,p);
    return self;
}

static id writeNoteAux(Note *self,_MKScoreOutStruct *p,
                       NXStream *aStream,const char *partName)
    /* Never invoke this function when writing a binary scorefile */
{
    /* The part should always have a name. This is just in case of lossage. */
    if (!partName)
      partName = "anon";
    NXPrintf(aStream,"%s ",partName);
    NXPrintf(aStream,"(");
    switch (self->noteType) {
      case MK_noteDur: {
          double dur = 
            (p && p->_ownerIsNoteRecorder) ? 
              _MKDurForTimeUnit(self,[p->_owner timeUnit]) : 
                getNoteDur(self);
          NXPrintf(aStream,"%.5f",dur);
          if (self->noteTag != MAXINT) 
            NXPrintf(aStream," %d",self->noteTag);
          break;
      }
      case MK_noteOn:
      case MK_noteOff:
      case MK_noteUpdate:
        NXPrintf(aStream,"%s",_MKTokNameNoCheck(self->noteType));
        if (self->noteTag != MAXINT)
          NXPrintf(aStream," %d",self->noteTag);
        break;
      case MK_mute:
      default:
        NXPrintf(aStream,"%s",_MKTokNameNoCheck(self->noteType));
        break;
    }
    NXPrintf(aStream,") ");
    _MKWriteParameters(self,aStream,p);
    return self;
}

-writeScorefileStream:(NXStream *)aStream
  /* TYPE: Display; Displays the receiver in ScoreFile format.
   * Displays, on \fIaStream\fR, the receiver in ScoreFile format.
   * Returns the receiver.
   */
{
    return writeNoteAux(self,NULL,aStream,MKGetObjectName(part));
}

id _MKWriteNote2(Note *self,id aPart,_MKScoreOutStruct *p)
    /* Used internally for writing notes to scorefiles. */
{
    if (p->_binary)
      return writeBinaryNoteAux(self,aPart,p);
    {
        id tmp;
        return writeNoteAux(self,p,p->_stream,
                            (char *)_MKNameTableGetObjectName(p->_nameTable,
                                                              aPart,&tmp));
    }
}

/* Assorted C functions ----------------------------------------- */

#define VELSCALE 64.0 /* Mike Mcnabb says this is right. */

double 
MKMidiToAmp(int v)
{ 
    /* We do the following map:
       
       VELOCITY  AMP SCALING           MEANING
       ----      -----------           -------
       127      10.0          Maximum amplitude scaling  
       64       1.0 (almost)  No modification of amp
       0         0            minus infinity dB
       */
    if (!v) 
      return 0.0;
    return pow(10.,((double)v-VELSCALE)/VELSCALE);
}

double MKMidiToAmpWithSensitivity(int v, double sensitivity)
{
    return (1 - sensitivity) + sensitivity * MKMidiToAmp(v); 
}

int MKAmpToMidi(double amp)
{ 
    int v;
    if (!amp) 
      return 0.0;
    v = VELSCALE + VELSCALE * log10(amp);
    return MAX(v,0);
}

#define VOLCONST 127.0
#define VOLSCALE 64.0

double 
MKMidiToAmpAttenuation(int v)
{ 
    /* We do the following map:
       
       VELOCITY  AMP SCALING           MEANING
       ----      -----------           -------
       127      1.0           No modification of amp
       64       .1            
       0         0            minus infinity dB
       */
    if (!v) 
      return 0.0;
    return pow(10.,((double)v-VOLCONST)/VOLSCALE);
}

double MKMidiToAmpAttenuationWithSensitivity(int v, double sensitivity)
{
    return (1 - sensitivity) + sensitivity * MKMidiToAmpAttenuation(v); 
}

int MKAmpAttenuationToMidi(double amp)
{ 
    int v;
    if (!amp) 
      return 0.0;
    v = VOLCONST + VOLSCALE * log10(amp);
    return MAX(v,0);
}

static id setPar(self,parNum,value,type)
    Note *self;
    int parNum;
    void *value;
    MKDataType type;
{
    register _MKParameter *aPar;
    if (!self)
      return nil;
    if (setParBit(self,parNum)) { /* Parameter is already present */
        SETDUMMYPAR(parNum);
        aPar = (_MKParameter *)NXHashGet((NXHashTable *)self->_parameters,
                                         (const void *)dummyPar);
        switch (type) {
          case MK_double:
            _MKSetDoublePar(aPar,*((double *)value));
            break;
          case MK_string:
            _MKSetStringPar(aPar,(char *)value);
            break;
          case MK_int:
            _MKSetIntPar(aPar,*((int *)value));
            break;
          default:
            _MKSetObjPar(aPar,(id)value,type);
            break;
        }
    }
    else { /* New parameter */
        switch (type) {
          case MK_int:
            aPar = _MKNewIntPar(*((int *)value),parNum); 
            break;
          case MK_double:
            aPar = _MKNewDoublePar(*((double *)value),parNum); 
            break;
          case MK_string:
            aPar = _MKNewStringPar((char *)value,parNum); 
            break;
          default:
            aPar = _MKNewObjPar((id)value,parNum,type); 
            break;
        }
        NXHashInsert((NXHashTable *)self->_parameters,(void *)aPar);
    }
    return self;
}

static void copyPars(toObj,fromObj,override)
    Note *toObj;
    Note *fromObj;
    BOOL override;
{
    /* Copies parameters from fromObj to toObj. If override
       is YES, the value from fromObj takes parameter. Otherwise, the
       value from toObj takes priority. */
    _MKParameter *aPar;
    NXHashState state;
    if ((!fromObj) || (!toObj) || (![fromObj isKindOf:noteClass]))
      return;
    if (!fromObj->_parameters)
      return;
    state = NXInitHashState((NXHashTable *)fromObj->_parameters);
    while (NXNextHashState((NXHashTable *)fromObj->_parameters,
                           &state,(void **)&aPar))
      if (PARNUM(aPar) != _MK_dur)
        if (override || !isParPresent(toObj,PARNUM(aPar)))
          _MKNoteAddParameter(toObj,aPar);
}

-(unsigned)hash
   /* Notes hash themselves based on their noteTag. */
{
    return noteTag;
}

-(BOOL)isEqual:aNote
   /* Notes are considered 'equal' if their noteTags are the same. */
{
   return [aNote isKindOf:noteClass] && (((Note *)aNote)->noteTag == noteTag);
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Archives parameters, noteType, noteTag, and timeTag. Also archives
     performer and part using MKWriteObjectReference(). This object should
     be written with NXWriteRootObject(). */
{
    NXHashState state;
    _MKParameter *aPar;
    int parCount;

    [super write:aTypedStream];
    
    /* First archive the note's type, noteTag, timeTag, and number of pars */
    if (_parameters)
      parCount = NXCountHashTable((NXHashTable *)_parameters);
    else parCount = 0;
    NXWriteTypes(aTypedStream,"iiidi", &noteType, &noteTag, &parCount, 
                 &timeTag,&_orderTag);
    NXWriteObjectReference(aTypedStream,performer);
    NXWriteObjectReference(aTypedStream,part);
    
    if (!_parameters)
      return self;

    /* Archive pars */
    state = NXInitHashState((NXHashTable *)_parameters);
    while (NXNextHashState((NXHashTable *)_parameters,&state,
                           (void **)&aPar)) 
      _MKArchiveParOn(aPar,aTypedStream);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Unarchives object. See write:.  */
{
    _MKParameter aPar;
    int parCount,i;

    /* First check version */

    [super read:aTypedStream];
    
    if (NXTypedStreamClassVersion(aTypedStream,"Note") == VERSION2) {
        /* First unarchive the note's type, noteTag, timeTag, 
           and number of pars */
        NXReadTypes(aTypedStream,"iiidi", &noteType, &noteTag, &parCount, 
                    &timeTag,&_orderTag);
        performer = NXReadObject(aTypedStream);
        part = NXReadObject(aTypedStream);
        
        if (parCount) /* Don't use allocHashtable 'cause we know size */
          _parameters =  NXCreateHashTable(htPrototype,parCount,NULL);
        for (i=0; i<parCount; i++) {
            _MKUnarchiveParOn(&aPar,aTypedStream); /* Writes into aPar */
            _MKNoteAddParameter(self,&aPar);
        }
    }
    return self;
}

#if 0
-(int)parameterCount
{
    if (!aNote->_parameters)
      return 0;
    return (int)NXCountHashTable(aNote->_parameters);
}
#endif

static unsigned highestTag = 0; /* Has the current highestTag. 1-based. */

unsigned MKNoteTag(void)
{
    /* Return a new noteTag or generate an error and return
       MAXINT if no more noteTags. */
    if (highestTag < MAXINT)
      return ++highestTag;
    else _MKErrorf(MK_noMoreTagsErr);
    return MAXINT;
}

unsigned MKNoteTags(unsigned n)
{
    /* Return the first of a block of n noteTags or generate an error
       and return MAXINT if no more noteTags or n is 0. */
    int base = highestTag + 1;
    if (((int)MAXINT) - ((int)n) >= highestTag) { 
        highestTag += n;
        return base;
    }
    _MKErrorf(MK_noMoreTagsErr);
    return MAXINT;
}

void _MKNoteAddParameter(Note *self,_MKParameter *aPar)
   /* Allows a Parameter object to be added directly. 
      The object is first copied. Returns nil. */
{
    aPar = _MKCopyParameter(aPar);
    setParBit(self,PARNUM(aPar));
    aPar = (_MKParameter *)NXHashInsert((NXHashTable *)self->_parameters,
                                        (const void *)aPar);
    _MKFreeParameter(aPar);
}

void _MKNoteShiftTimeTag(Note *aNote, double timeShift)
    /* Assumes timeTag is valid */
{
    aNote->timeTag += timeShift;
}

static NXHashState *cachedHashState = NULL;

void *MKInitParameterIteration(Note *aNote)
    /* Call this to start an iteration over the parameters of a Note.
       Usage:

    void *aState = MKInitParameterIteration(aNote);
    int par;
    while ((par = MKNextParameter(aNote,aState)) != MK_noPar) {
        select (par) {
          case freq0: 
            something;
            break;
          case amp0:
            somethingElse;
            break;
          default: // Skip unrecognized parameters
            break;
        }}
    */
{
    NXHashState *aState;
    if (!aNote || !aNote->_parameters)
      return NULL;
    if (cachedHashState) {
        aState = cachedHashState;
        cachedHashState = NULL;
    }
    else _MK_MALLOC(aState,NXHashState,1);
    *aState = NXInitHashState((NXHashTable *)aNote->_parameters);
    return (void *)aState;
}

int MKNextParameter(Note *aNote,void *aState)
{
    id aPar;
    if (!aNote || !aState) 
      return MK_noPar;
    if (NXNextHashState((NXHashTable *)aNote->_parameters,
                        (NXHashState *) aState,
                        (void **)&aPar))
      return PARNUM(aPar);
    else {
        if (cachedHashState) { /* Cache is full */
            NX_FREE(aState);
        } else cachedHashState = (NXHashState *)aState; /* Cache it */
        return MK_noPar;
    }
}

/* FIXME Needed due to a compiler bug */
static void setNoteOffFields(Note *aNoteOff,int aNoteTag,id aPerformer)
{
    aNoteOff->noteTag = aNoteTag;
    aNoteOff->performer = aPerformer;
    aNoteOff->part = nil;
    aNoteOff->noteType = MK_noteOff;
}

@end


@implementation Note(Private)

-_unionWith:aNote
    /* Copies parameters from aNote to the receiver. For 
       parameters which are already present in the receiver, the
       value from the receiver is used. (cf. -copyFrom:) */
{
    copyPars(self,aNote,NO);
    return self;
}

-_noteOffForNoteDur
  /* If the receiver isn't a noteDur, returns \fBnil\fR. Otherwise, returns
   * the noteOff created according to the rules described in
   * \fB-split::\fR.
   */
 {
    Note *aNoteOff;
    if (noteType != MK_noteDur) 
      return nil;
    aNoteOff = [noteClass newSetTimeTag:timeTag+getNoteDur(self)];
    if (noteTag == MAXINT) 
      noteTag = MKNoteTag(); 
    setNoteOffFields(aNoteOff,noteTag,performer);
    if (isParPresent(self,MK_relVelocity))
      MKSetNoteParToInt(aNoteOff,MK_relVelocity,
                        MKGetNoteParAsInt(self,MK_relVelocity));
    return aNoteOff;
}

-_splitNoteDurNoCopy
  /* Same as \fB-_noteOffForNoteDur\fR except the receiver's \fBnoteType\fR 
   * is set to \fBMK_noteOn\fR.
   */
{
    Note *aNoteOff = [self _noteOffForNoteDur];
    noteType = MK_noteOn;
    return aNoteOff;
}

-(void)_setPerformer:anObj
  /* Private method sent by NoteSender class.  */
{
        performer = anObj;
}

- _setPartLink:aPart order:(int)orderTag
  /* Private method used for interface with Part class. */
{
    id oldPart;
    oldPart = part;
    part = aPart;
    if (aPart)
      _orderTag = orderTag;
    return oldPart;
}

@end

