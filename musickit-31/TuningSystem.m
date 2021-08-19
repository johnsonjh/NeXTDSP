#ifdef SHLIB
#include "shlib.h"
#endif

/*
  TuningSystem.m
  Copyright 1988, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/

/* 
Modification history:

       09/22/89/daj - Removed addReadOnlyVar, which was never called.
                      Changes corresponding to those in _MKNameTable.h.
       10/20/89/daj - Added binary scorefile support.
       11/27/89/daj - Optimized MKAdjustFreqWithPitchBend to check for
                      no-bend case.
       01/02/90/daj - Deleted a comment.		      
       03/21/90/daj - Added archiving.
       04/21/90/daj - Small mods to get rid of -W compiler warnings.
       06/27/90/daj - Fix to setKeyNumAndOctaves:toFreq:.
       08/27/90/daj - Changed to zone API.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
  10/09/90/daj - Changed [super new] to 
                 [super allocFromZone:NXDefaultMallocZone()]
  10/09/90/daj - Added zone specification for installedTuningSystem.
*/

#define MK_INLINE 1
#import "_musickit.h"  
#import "_MKNameTable.h"
#import "_ScorefileVar.h"
#import "_tokens.h"
#import "_TuningSystem.h"
#import <objc/Storage.h>

@implementation TuningSystem:Object
{
    id frequencies; /* Array of frequencies, indexed by keyNum. */
    void *_reservedTuningSystem1;
}
/* Each TuningSystem object manages a mapping from keynumbers to frequencies.
   There are MIDI_NUMKEYS individually tunable elements. The tuning system
   which is accessed by pitch variables is referred to as the "installed
   tuning system". */

#import "equalTempered.m"

static BOOL tuningInited = NO;


static id pitchVars[MIDI_NUMKEYS] = {nil};   /* Mapping from keyNum to freq. 
					  Not necessarily monotonically 
					  increasing. */
  
typedef struct _freqMidiStruct {
    id freqId;
    int keyNum;
} freqMidiStruct;
  
static freqMidiStruct * freqToMidi[MIDI_NUMKEYS];  
/* Used for mapping freq to keyNum. This is kept sorted by frequency. */
  
static int
  freqCmp(p1,p2)
freqMidiStruct **p1, **p2;
{
    /* Function used by qsort below to compare frequencies. */
    double v1,v2;
    v1 = _MKParAsDouble(_MKSFVarGetParameter((*p1)->freqId));
    v2 = _MKParAsDouble(_MKSFVarGetParameter((*p2)->freqId));
    if (v1 < v2) return(-1);
    if (v1 > v2) return(1);
    return(0);
}      

static BOOL dontSort = NO; /* Used to override sort daemon when setting all
			      pitches at once. */

static void
sortPitches(obj)
    id obj;
{
    /* obj is a dummy argument needed by ScorefileVar class. 
       Sorts pitches array. */
    if (dontSort) 
      return;
    qsort(freqToMidi, MIDI_NUMKEYS, sizeof(freqMidiStruct *),freqCmp);
}

MKKeyNum MKFreqToKeyNum(double freq,int *bendPtr,double sensitivity)	
    /* Returns keyNum (pitch index) of closest pitch variable to the specified
       frequency . If bendPtr is not NULL, *bendPtr
       is set to the bend needed to get freq, given the current value
       of the scorefile variable bendAmount.
     */
{	
    /* Do a binary search for target. If bendPtr is not NULL, *bendPtr
       is set to the bend needed to get freq.
     */
#   define PITCH(x) \
    (_MKParAsDouble(_MKSFVarGetParameter(freqToMidi[(x)]->freqId)))

    register int low = 0; 
    register int high = MIDI_MAXDATA;
    register int tmp = MIDI_MAXDATA/2;
    int hit;
    if (!tuningInited)
      _MKCheckInit();
    while (low+1 < high) {
	tmp = (int) floor((double) (low + (high-low)/2));
	if (freq > PITCH(tmp))
	  low = tmp;
	else high = tmp;
    }
    if ((low == MIDI_MAXDATA) || 
	((freq/PITCH(low)) < (PITCH(low+1)/freq)))
      /* See comment below */
      hit = low;
    else hit = (low+1);
    if (bendPtr) {
	double tuningError = 12 * ( ( log(PITCH(hit))/freq) / log(2.0));
	  /* tuning error is in semitones */
	double bendRatio = tuningError/sensitivity;
	bendRatio = MAX(MIN(bendRatio,1.0),-1.0);
	*bendPtr = (int)MIDI_ZEROBEND + (bendRatio * (int)0x1fff);
    }
    return(hit);
}

#if 0
static id addReadOnlyVar(char * name,int val)
{
    /* Add a read-only variable to the global parse table. */
    id rtnVal;
    _MKNameGlobal(name,rtnVal = 
		  _MKNewScorefileVar(_MKNewIntPar(val,MK_noPar),name,NO,YES),
		  _MK_typedVar,YES,YES);
    return rtnVal;
}
#endif


double
MKTranspose(double freq,double semiTonesUp)
    /* Transpose a frequency up by the specified number of semitones. 
       A negative value will transpose the note down. */
{
    return (freq*(pow(2.0, (((double) semiTonesUp) / 12.0))));
}

double 
MKKeyNumToFreq(MKKeyNum keyNum)
    /* Convert keyNum to frequency using the installed tuning system.
       Returns MK_NODVAL if keyNum is out of bounds. */
{
    if ((keyNum < 0) || (keyNum > MIDI_NUMKEYS))
      return MK_NODVAL;
    keyNum = MIDI_DATA(keyNum);
    if (!tuningInited)
      _MKCheckInit();
    return _MKParAsDouble(_MKSFVarGetParameter(pitchVars[(int)keyNum]));
}

double 
MKAdjustFreqWithPitchBend(double freq,int pitchBend,double sensitivity)
    /* Return the result of adjusting freq by the amount specified in
       pitchBend. Sensitivity is in semitones. */
{
#   define SCL ((1.0/(double)MIDI_ZEROBEND))
    double bendAmount;
    if ((pitchBend == MIDI_ZEROBEND) || (pitchBend == MAXINT))
      return freq;
    bendAmount = (pitchBend - (int)MIDI_ZEROBEND) * sensitivity * SCL;
    if (bendAmount)
      return MKTranspose(freq,(double) bendAmount);
    return freq;
}

static id keyNumToId[MIDI_NUMKEYS] = {nil}; /* mapping from keyNum to the id
					  with its name. */

#define BINARY(_p) (_p && _p->_binary) 

BOOL _MKKeyNumPrintfunc(_MKParameter *param,NXStream *aStream,
			_MKScoreOutStruct *p)
{
    /* Used to write keyNum parameters. */
    int i = _MKParAsInt(param);
    if (!tuningInited)
      _MKCheckInit();
    if (param->_uType == MK_envelope)
      return NO;
    if (BINARY(p))
      _MKWriteIntPar(aStream,i);
    else if ((i < 0) || (i > MIDI_NUMKEYS))
      NXPrintf(aStream, "%d", i);
    else NXPrintf(aStream,"%s",[keyNumToId[i] name]);
    return YES;
}    
    
static BOOL writePitches = NO;

void MKWritePitchNames(BOOL usePitchNames)
{
    writePitches = usePitchNames;
}

BOOL _MKFreqPrintfunc(_MKParameter *param,NXStream *aStream,
		      _MKScoreOutStruct *p)
{
    /* Used to write keyNum parameters. */
    double frq;
    char *pitchName;
    int keyNum;
    if ((param->_uType == MK_envelope) || (!writePitches))
      return NO;
    if (!tuningInited)
      _MKCheckInit();
    frq = _MKParAsDouble(param);
    keyNum = MKFreqToKeyNum(frq,NULL,0);
    pitchName = (char *)[pitchVars[keyNum] name];
    if (BINARY(p))
      _MKWriteVarPar(aStream,pitchName);
    else NXPrintf(aStream,"%s",pitchName);
    return YES;
}    

static void install(double *arr) 
{
    register double *p = arr;
    register double *pEnd = arr + MIDI_NUMKEYS;
    register id *idp = pitchVars;
    dontSort = YES;
    while (p < pEnd)
      _MKSetDoubleSFVar(*idp++,*p++);
    dontSort = NO;
    sortPitches(nil);
}

static void
addPitch(keyNumValue,name,oct)
    int keyNumValue;
    char * name,*oct;
{
    /* Add a pitch to the music kit table. */
    id obj;
    char * s1,*s2;
    if (keyNumValue>=MIDI_NUMKEYS)
      return;
    s1 = _MKMakeStrcat(name,oct);
    obj = _MKNewScorefileVar(_MKNewIntPar(keyNumValue,MK_noPar),
			     s2 = _MKMakeStrcat(s1,"k"),NO,YES);
    keyNumToId[keyNumValue] = obj;
    _MKNameGlobal(s2,obj,_MK_typedVar,YES,NO);
    obj = _MKNewScorefileVar(_MKNewDoublePar(0.0,MK_noPar),s1,NO,NO);
    _MKNameGlobal(s1,obj,_MK_typedVar,YES,NO);
    _MKSetScorefileVarPostDaemon(obj,sortPitches);
    pitchVars[keyNumValue] = obj;
}

static void
addAccidentalPitch(keyNumValue,name1,name2,oct1,oct2)
    int keyNumValue;
    char * name1,*name2,*oct1,*oct2;
{
    /* Add an accidental pitch to the musickit table, including
       its enharmonic equivalent as well. */
    id obj1,obj2;
    _MKParameter *tmp;
    char * sharpStr,*flatStr,*sharpKeyStr,*flatKeyStr;
    if (keyNumValue>=MIDI_NUMKEYS)
      return;
    sharpStr = _MKMakeStrcat(name1,oct1);
    flatStr = _MKMakeStrcat(name2,oct2);
    obj1 = _MKNewScorefileVar(_MKNewIntPar(keyNumValue,MK_noPar),
			     sharpKeyStr = _MKMakeStrcat(sharpStr,"k"),NO,YES);
    keyNumToId[keyNumValue] = obj1; /* Only use one in x-ref array */
    obj2 = _MKNewScorefileVar(_MKNewIntPar(keyNumValue,MK_noPar),
			     flatKeyStr = _MKMakeStrcat(flatStr,"k"),NO,YES);
    _MKNameGlobal(sharpKeyStr,obj1,_MK_typedVar,YES,NO);
    _MKNameGlobal(flatKeyStr,obj2,_MK_typedVar,YES,NO);
    obj1 = _MKNewScorefileVar(tmp = _MKNewDoublePar(0.0,MK_noPar),sharpStr,
			      NO,NO);
    obj2 = _MKNewScorefileVar(tmp,flatStr,NO,NO);
    _MKNameGlobal(sharpStr,obj1,_MK_typedVar,YES,YES);
    _MKNameGlobal(flatStr,obj2,_MK_typedVar,YES,YES);
    /* Named object copies its string so can free original. */
    _MKSetScorefileVarPostDaemon(obj1,sortPitches);
    _MKSetScorefileVarPostDaemon(obj2,sortPitches);
    pitchVars[keyNumValue] = obj1;
}

void _MKTuningSystemInit()
    /* Sent by MKInit1() */
{
    int i;
    static const char * octaveNames[] = {"00","0","1","2","3","4","5","6","7",
					   "8","9","10"};
    const char * oct = octaveNames[0]; /* Pointer to const char */
    const char * nOct = octaveNames[1];
    if (tuningInited)
      return; 
    tuningInited = YES;
    i = 0;
    addPitch(i++,"c",oct);       /* No low Bs */
    while (i<MIDI_NUMKEYS) {
	addAccidentalPitch(i++,"cs","df",oct,oct);  
	addPitch(i++,"d",oct); 	          
	addAccidentalPitch(i++,"ds","ef",oct,oct);  
	addAccidentalPitch(i++,"e","ff",oct,oct);  
	addAccidentalPitch(i++,"f","es",oct,oct);  
	addAccidentalPitch(i++,"fs","gf",oct,oct);  
	addPitch(i++,"g",oct);            
	addAccidentalPitch(i++,"gs","af",oct,oct);
	addPitch(i++,"a",oct);
	addAccidentalPitch(i++,"as","bf",oct,oct);
	addAccidentalPitch(i++,"b","cf",oct,nOct);
	addAccidentalPitch(i,"c","bs",nOct,oct);  
	oct = nOct;
	nOct = octaveNames[1 + i/12];
	i++;
    }
    for (i=0; i<MIDI_NUMKEYS; i++) {  /* Init pitch x-ref. */
	_MK_MALLOC(freqToMidi[i], freqMidiStruct, 1);
	freqToMidi[i]->freqId = pitchVars[i];
	freqToMidi[i]->keyNum = i;
    }
    install((double *)equalTempered12);
}

#define VERSION2 2

+initialize
{
    if (self != [TuningSystem class])
      return self;
    if (!tuningInited)
      _MKCheckInit();
    [self setVersion:VERSION2];
    return self;
}


+new
{
    self = [super allocFromZone:NXDefaultMallocZone()];
    [self init];
    return self;
}

-init
  /* Returns a new tuning system initialized to 12-tone equal tempered. */
{
    [super init];
    if (!tuningInited)
      _MKCheckInit();
    frequencies = [Storage newCount:MIDI_NUMKEYS elementSize:8
		description:"d"];
    [self setTo12ToneTempered];
    return self;
}

- write:(NXTypedStream *) aTypedStream
{
    [super write:aTypedStream];
    NXWriteObject(aTypedStream,frequencies);
    return self;
}

- read:(NXTypedStream *) aTypedStream
{
    [super write:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"TuningSystem") == VERSION2) 
      frequencies = NXReadObject(aTypedStream);
    return self;
}

-setTo12ToneTempered
  /* Sets receiver to 12-tone equal-tempered tuning. */
{
    memcpy((double *)[frequencies elementAt:0],equalTempered12,
	   MIDI_NUMKEYS * sizeof(double)); 
    /* Copy from equalTempered12 to us. */
    return self;
}

-install
  /* Installs the receiver as the current tuning system. Note, however,
     that any changes to the contents of the receiver are not automatically
     installed. You must send install again in this case. */
{
    install((double *)[frequencies elementAt:0]);
    return self;
}

-free
{
    [frequencies free];
    return [super free];
}

-copyFromZone:(NXZone *)zone
  /* Returns a copy of receiver. */
{
    TuningSystem *newObj = [super copyFromZone:zone];
    newObj->frequencies = [Storage newCount:MIDI_NUMKEYS elementSize:8
			 description:"d"];
    memcpy((double *)[newObj->frequencies elementAt:0],
	   (double *)[frequencies elementAt:0],
	   MIDI_NUMKEYS * sizeof(double)); 
    return newObj;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

+installedTuningSystem
  /* Returns a new TuningSystem set to the values of the currently installed
     tuning system. Note, however, that this is a copy of the current values.
     Thus, altering the returned object does not alter the current values
     unless the -install message is sent. */
{
    return [[super allocFromZone:NXDefaultMallocZone()] initFromInstalledTuningSystem];
}

-initFromInstalledTuningSystem
{
    register int i;
    register double *p;
    register id *pit;
    [super init];
    if (!tuningInited)
      _MKCheckInit();
/*  frequencies = [Storage newCount:MIDI_NUMKEYS elementSize:sizeof(double)
	       description:"d"];
*/
    frequencies = [Storage newCount:MIDI_NUMKEYS elementSize:8
	       description:"d"];
    p = (double *)[frequencies elementAt:0];
    for (i=0, pit = pitchVars; i<MIDI_NUMKEYS; i++)
      *p++ = _MKParAsDouble(_MKSFVarGetParameter(*pit++));
    return self;
}

+(double)freqForKeyNum:(MKKeyNum)aKeyNum
  /* Returns freq For specified keyNum in the installed tuning system.
     or MK_NODVAL if keyNum is illegal . */ 
{
    if (!tuningInited)
      _MKCheckInit();
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return MK_NODVAL;
    return _MKParAsDouble(_MKSFVarGetParameter(pitchVars[(int)aKeyNum]));
}

-(double)freqForKeyNum:(MKKeyNum)aKeyNum
  /* Returns freq For specified keyNum in the receiver or MK_NODVAL if the
     keyNum is illegal . */ 
{
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return MK_NODVAL;
    return (*((double *)[frequencies elementAt:aKeyNum]));
}

-setKeyNum:(MKKeyNum)aKeyNum toFreq:(double)freq
  /* Sets frequency for specified keyNum in the receiver. Note that the 
     change is not installed. */
{
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return nil;
    (*((double *)[frequencies elementAt:aKeyNum])) = freq;
    return self;
}

+setKeyNum:(MKKeyNum)aKeyNum toFreq:(double)freq
  /* Sets frequency for specified keyNum in the installed tuning system.
     Note that if several changes are going to be made at once, it is more
     efficient to make the changes in a TuningSystem instance and then send
     the install message to that object. 
     Returns self or nil if aKeyNum is out of bounds. */
{
    if (!tuningInited)
      _MKCheckInit();
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return nil;
    _MKSetDoubleSFVar(pitchVars[(int)aKeyNum],freq);
    return self;
}

-setKeyNumAndOctaves:(MKKeyNum)aKeyNum toFreq:(double)freq
  /* Sets frequency for specified keyNum and all its octaves in the receiver.
     Returns self or nil if aKeyNum is out of bounds. */
{       
    register int i;
    register double fact;
    double *arr = (double *)[frequencies elementAt:0];
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return nil;
    for (fact = 1.0, i = aKeyNum; i >= 0; i -= 12, fact *= .5) 
      arr[i] = freq * fact;
    for (fact = 2.0, i = aKeyNum + 12; i < MIDI_NUMKEYS; i += 12, fact *= 2.0)
      arr[i] = freq * fact;
    return self;
}

+setKeyNumAndOctaves:(MKKeyNum)aKeyNum toFreq:(double)freq
  /* Sets frequency for specified keyNum and all its octaves in the installed
     tuning system.
     Note that if several changes are going to be made at once, it is more
     efficient to make the changes in a TuningSystem instance and then send
     the install message to that object. 
     Returns self or nil if aKeyNum is out of bounds. */
{	
    register int i;
    register double fact;
    if ((aKeyNum < 0) || (aKeyNum > MIDI_NUMKEYS))
      return nil;
    if (!tuningInited)
      _MKCheckInit();
    dontSort = YES;
    for (fact = 1.0, i = aKeyNum; i >= 0; i -= 12, fact *= .5)
	_MKSetDoubleSFVar(pitchVars[i],freq * fact);
    for (fact = 2.0, i = aKeyNum + 12; i < MIDI_NUMKEYS; i += 12, fact *= 2.0)
	_MKSetDoubleSFVar(pitchVars[i],freq * fact);
    dontSort = NO;
    sortPitches(nil);
    return self;
}

int _MKFindPitchVar(id aVar)
    /* Returns keyNum corresponding to the specified pitch variable or
       MAXINT if none. */
{
    register int i;
    register id *pitch = pitchVars;
    _MKParameter *aPar;
    if (!aVar) 
      return MAXINT;
    aPar = _MKSFVarGetParameter(aVar); 
    for (i = 0; i < MIDI_NUMKEYS; i++)
      if (aPar == _MKSFVarGetParameter(*pitch++)) 
	return i; /* We must do it this way because of enharmonic equivalents*/
    return MAXINT;
}    

+transpose:(double)semitones
  /* Transposes the installed tuning system by the specified amount.
     If semitones is positive, the installed tuning system is transposed
     up. If semitones is negative, the installed tuning system is transposed
     down. Semitones may be fractional.
     */
{
    register int i;
    register id *p = pitchVars;
    double fact = pow(2.0,semitones/12.0);
    dontSort = YES;
    if (!tuningInited)
      _MKCheckInit();
    for (i=0; i<MIDI_NUMKEYS; i++, p++) 
      _MKSetDoubleSFVar(*p,_MKParAsDouble(_MKSFVarGetParameter(*p)) * fact);
    dontSort = NO;
    sortPitches(nil);
    return self;
}

-transpose:(double)semitones
  /* Transposes the receiver by the specified amount.
     If semitones is positive, the receiver is transposed up.
     If semitones is negative, the receiver is transposed down.
     */
{
    register double fact = pow(2.0,semitones/12.0);
    register double *p = (double *)[frequencies elementAt:0];
    register double *pEnd = p + MIDI_NUMKEYS; 
    while (p < pEnd)
      *p++ *= fact;
    return self;
}

@end

















