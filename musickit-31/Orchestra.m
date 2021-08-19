#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Orchestra.m
  Copyright 1987, NeXT, Inc.
  Responsibility:David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
  */

/* This is the allocater and manager of DSP resources. CF: UnitGenerator.m,
   SynthPatch.m, SynthData.m and PatchTemplate.m. */ 
   
/* 
Modification history:

  11/10/89/daj - Moved StartSoundOut from -run to -open. The idea is to
                 let soundout fill up the buffers so we don't get random
                 behavior at the start of the score. This should be tested
                 for write data and for write dac.
  11/21/89/daj - Minor changes to support lazy shared data garbage collection.   
  11/27/89/daj - Removed argument from _MKCurSample. Made _previousTime and
                 _previousTimeStamp be instance variables of Orchestra.
  12/3/89/daj  - Moved deviceStatus = MK_devClosed to freeUgs() from 
                 closeOrch() to prevent problems when Conductor's 
                 finishPerformance sends -close recursively. For maximum
                 safety, setting this instance variable is now the last 
                 thing when opening and the first thing when closing.
  12/13/89/daj - Changed _MKCurSample() so that it does NOT return
                 DSPMK_UNTIMED when there's no conductor. This was a bug.
  01/07/89/daj - Changed comments. Made _setSimualtorFilePtr: be private.
                 Made compaction helper functions be in compaction cond
                 compilation. Flushed MIXTEST and some false cond. comp.
  01/10/90/daj - Made changes to accomodate new dspwrap. Adjusted headroom.
  01/16/90/daj - Added conditional compilation to NOT start sound out early.
                 This is an attempt to fix a strange deltaT bug.
  01/25/90/daj - Reimplemented simulator support. Made segmentZero: and
                 segmentSink: accept MK_xData as well as MK_xPatchpoint.
  01/30/90/daj - Broke out the devstatus methods into a separate file for
                 ease of maintaining. New file is orchControl.m. 
  01/31/90/daj - Changed instance variables to be backward-compatable with 1.0
                 header files. This meant making a private "extraVars" struct.
                 Changed loadOrchLoop failure case to just call freeUGs(). 
                 This failure should never happen, anyway.
                 Fixed trace message in allocUnitGenerator.
                 Fixed bug in resoAlloc. It wasn't selecting UnitGenerator
                 times correctly. Got rid of wasTimed in loadOrchLoop(). It
                 was a noop because we're always untimed when loading the
                 orch loop. Added new dspwrap support to compaction code.
                 Flushed obsolete methods 
                 -installSharedObject:for:segment:length and 
                 -installSharedObject:for:segment:.
  02/13/90/daj - Fixed bugs in compaction: I was pre-incrementing piLoop so
                 the space for a freed UnitGenerator wasn't really getting 
                 freed. Reversed order of bltArgs and bltLoop so that 
                 moved messages get sent after the UnitGenerator is fully
                 moved. Changed increment of el to end of loop. Removed -1
                 in bltLoop and bltArgs calls. Changed bltLoop and bltArgs
                 to take address of ug list pointer and set it to NULL (this
                 is a cosmetic change). Added code to break up BLTs that
                 straddle onChip/offChip boundary. Fixed bug in leaper setting.
                 (it was being set to leap to the wrong address.) Tested
                 compaction and it seems to be working well for all cases.

  2/26/90/jos - deleted dummy versions of DSPMKThawOrchestra() and 
                 DSPMKFreezeOrchestra()
  03/23/90/mtm - Added -setOutputDSPCommandsSoundfile: and
                 -(char *)outputDSPCommandsSoundfile
  03/28/90/mtm - Changed above to "setOutputCommandsFile" and 
                 "outputCommandsFile"
  03/28/90/daj - Added read data API support. 
  04/05/90/mmm - Added synchToConductor: from daj experimental Orchestra
  04/05/90/mmm - Added adjustTime hack in all allocation routines.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/25/90/daj - Added check for devStatus == MK_devClosed in 
                 flushTimedMessages
                 Changed _DSPSendArraySkipTimed to DSPMKSendArraySkipTimed
  05/05/90/daj - Made external memory configurable and sensed. This involved 
                 adding several new instance variables.
  06/10/90/daj - Added check in synchTime() for whether Conductor is behind.
                 If Conductor is behind, doesn't synch.
  06/26/90/mtm - Grab conductor thread lock in synchTime().
  10/04/90/daj - Changed _MKCurSample() to return truly untimed if unclocked and
                 stopped.
*/

#import <appkit/Application.h>
#import <dpsclient/dpsNeXT.h> /* Contains NX_FOREVER */
#import "_musickit.h"
#import "_error.h"
#import "_time.h"
#import "_PatchTemplate.h"
#import "_Conductor.h"

#import <stdio.h>               /* Contains FILE, etc. */
#import <objc/HashTable.h>

/* FIXME Consider changing patch List and stack to non-objc linked lists. */

#define COMPACTION 1 /* Set to 0 to turn off compaction. */
#define READDATA 0   /* Set to 1 to turn on read data */

/* Codes returned by memory allocation routines (private) */
#define OK 0
#define TIMEERROR -2
#define BADADDR -1
#define NOMEMORY MAXINT

/* Size of "leaper" (causes orch loop to leap off chip) and "looper" 
   (causes orch loop to return to start of loop) */
#define LEAPERSIZE 3  /* For jumping off-chip */
#define LOOPERSIZE 2

/* The DSP memory map, in orchestra terms, is given by the following: 
   */

/* The following constants deal with the bounds of DSP internal memory. 
   Ulitmately, they should be dynamically obtained from the DSP library.
   When things change enough for them to become variables, we may want to 
   revamp the entire memory map. Note also that the Orchestra assumes 
   overlayed addressing of off-chip memory (i.e. X, Y and P spaces all refer 
   to the same memory location). 
   FIXME */
#define _DEGMON_L     0x000034
#define _XHI_USR      0x0000f5
#define _YHI_USR      0x0000f5
#define _XLI_USR      0x000004
#define _YLI_USR      0x000004
#define _PHI_USR      0x0001ff
#define _PLI_USR      0x000080

/* The following numbers are the default values for the percentage of 
   off-chip memory devoted to X and Y unit generator arguments, respectively. 
   If these values are used for the standard 8k memory configuration, they 
   come out to the same numbers used in the 1.0 release, i.e. 272. They were 
   computed as follows:

   XARGPCDEFAULT = 272/(DSPMK_HE_USR - DSPMK_LE_USR) = 
     (float)272/(0x30d9-0x4313)

*/
  
#define XARGPCDEFAULT 0.063065151884503 
#define YARGPCDEFAULT 0.063065151884503 

/* On-chip P memory -------------------------------------------------------- */
#define ORCHLOOPLOC (self->_bottomOfMemory)
/*  the first user location. */
#define MINPILOOP  (ORCHLOOPLOC)
/*  where the looper jumps to. */
#define MAXPILOOP  (_PHI_USR - LEAPERSIZE)
/*  upper bound of internal p loop. Leaves room for leaper to offchip */
#define DEFAULTONCHIPPATCHPOINTS 11

/* On-chip L memory -------------------------------------------------------- */
#define MINLARG    (MAX(_XLI_USR,_YLI_USR))
/*  lower bound of lArg. */
#define MAXLARG    (MAXXPATCH - self->_onChipPPPartitionSize)
/*  upper bound of lArg. Shares with xArg and yArg. */

/* On-chip X memory ------------------------------------------------------ */
#define MINXPATCH   (MAXLARG + 1)
/*  lower bound of on-chip patch-points partition. */
#define MAXXPATCH   (MIN(_XHI_USR,_YHI_USR))

/*  upper bound of on-chip patch-point partition. Other patch-points offchip */

/* On-chip Y memory ---------------------------------------------------- */
#define MINYPATCH   MINXPATCH
/*  lower bound of on-chip patch-points partition. */
#define MAXYPATCH   MAXXPATCH
/*  upper bound of on-chip patch-point partition. Other patch-points offchip */

/* Off-chip memory (overlayed) --------------------------------------------- */

#define NUMXARGS self->_numXArgs
#define NUMYARGS self->_numYArgs

#define MINXARG     (MAXPELOOP + 1)
/*  x arguments follow XPATCH partition. Shares with l space.  */
#define MAXXARG     (MINXARG + NUMXARGS - 1)
/*  upper bound of xArg partition. Shares with l space. */

#define MINYARG     (MINXARG + NUMXARGS)
/*  y arguments follow YPATCH partition. Shares with l space. */
#define MAXYARG     self->_topOfExternalMemory
/*  upper bound of yArg partition. Shares with l space. */

#define MINPELOOP   self->_bottomOfExternalMemory
/*  lower bound of external pLoop. Shares with PSUBR, overlayed with Y & P. */
#define MAXPELOOP   (MAXYARG - NUMXARGS - NUMYARGS)
/*  upper bound of external pLoop. Shares with PSUBR, overlayed with Y & P. */
#define MINDATA (MINPELOOP + LOOPERSIZE)
#define MINPSUBR    MINDATA
/*  lower bound of pSubr. Shares with PELOOP and overlayed with Y & P. */
#define MAXPSUBR    MAXPELOOP
/*  upper bound of pSubr. Shares with PELOOP and overlayed with Y & P. */
#define MINXDATA    MINDATA
/*  lower bound of xData. xData is off-chip, overlayed with Y & P. */
#define MAXXDATA    MAXPELOOP
/*  upper bound of xData. xData is off-chip, overlayed with Y & P. */
#define MINYDATA    MINDATA
/*  lower bound of yData. yData is off-chip, overlayed with X & P. */
#define MAXYDATA    MAXPELOOP
/*  upper bound of yData. yData is off-chip, overlayed with X & P. */

static int extraRoomAtTopOfOffchipMemory = 0;

int _MKSetTopOfMemory(int val)
    /* This is something Julius wanted for debugging purposes. It allows an
       extra partition to be left at the top of DSP memory. Must be 
       called before the Orchestra is opened. val is the amount of extra
       room to leave at the top of memory. */
{
    extraRoomAtTopOfOffchipMemory = val;
    return 0;
}

#define SINTABLESPACE MK_yData
#define MULAWTABLESPACE MK_xData

#define JUMP 0x0af080  /* Used by leaper */
#define SHORTJUMP 0x0C0000 /* add in short (12 bit) jump address */ 

#import "_SynthData.h"
#import "_UnitGenerator.h"
#import "_SynthPatch.h"
#import "_SharedSynthInfo.h"
#import "_OrchloopbeginUG.h"

#import "_Orchestra.h"
@implementation Orchestra:Object
 /* The Orchestra class is used for managing DSP allocation and control for
    doing sound and music on the DSP. Actually, the Orchestra object supports
    multiple DSPs, although in the basic NeXT configuration, there is only one 
    DSP. 
     
    The Orchestra factory object manages all programs running on all the DSPs. 
    Each instances of the Orchestra class corresponds to a single DSP. We call 
    these instances "orchestra instances" 
    or, simply, "orchestras". We call the sum total of all orchestras the
    "Orchestra". Each orchestra instance is referred to by an integer 
    'orchIndex'. These indecies start at 0. For the basic
    NeXT configuration, orchIndex is always 0.
     
    There are two levels of allocation: SynthPatch allocation and 
    unit generator allocation. SynthPatches are higher-level entities, 
    collections of UnitGenerators. Both levels may be used at the same time.
    */
{
    double computeTime;      /* Runtime of orchestra loop in seconds. */
    double samplingRate;  /* Sampling rate. */
    id stack;      /* Stack of UnitGenerator instances in the order they
                      appear in DSP memory. SynthData instances are not on 
                      this stack. */
    char *outputSoundfile; /* For output sound samples. */
    char *inputSoundfile; /* For output sound samples. */ /* READ DATA */
    char *outputCommandsFile; /* For output DSP commands. */
    id xZero;         /* Special pre-allocated x patch-point that always holds
                         0 and to which nobody ever writes, by convention.  */
    id yZero;         /* Special pre-allocated y patch-point that always holds
                         0 and to which nobody ever writes, by convention.  */
    id xSink;      /* Special pre-allocated x patch-point that nobody ever
                      reads, by convention. */
    id ySink;      /* Special pre-allocated y patch-point that nobody ever
                      reads, by convention. */
    id sineROM;    /* Special read-only SynthData object used to represent
                      the SineROM. */
    id muLawROM;   /* Special read-only SYnthData object used to represent
                      the Mu-law ROM. */
    MKDeviceStatus deviceStatus; /* Status of Orchestra. */
    unsigned short orchIndex;  /* Index of the DSP managed by this instance. */
    char isTimed;    /* Determines whether DSP commands go out timed or not. */
    BOOL useDSP;     /* YES if running on an actual DSP (Default is YES) */
    BOOL soundOut;   /* YES if sound it going to the DACs. */
    BOOL SSISoundOut;
    BOOL isLoopOffChip; /* YES if loop has overflowed off chip. */
    BOOL fastResponse;  /* YES if response latency should be minimized */
    double localDeltaT; /* positive offset in seconds added to out-going 
			   time-stamps */
    short onChipPatchPoints;
    id _reservedOrchestra1;
    int _reservedOrchestra2;
    void *_reservedOrchestra3;
    void  *_reservedOrchestra4;
    id _reservedOrchestra5;
    DSPAddress _reservedOrchestra6;
    DSPAddress _reservedOrchestra7;
    DSPAddress _reservedOrchestra8;
    DSPAddress _reservedOrchestra9;
    DSPAddress *_reservedOrchestra10;
    DSPAddress *_reservedOrchestra11;
    unsigned long _reservedOrchestra12;
    unsigned long _reservedOrchestra13;
    double _reservedOrchestra14;
    double _reservedOrchestra15;
    id _reservedOrchestra16;
    id _reservedOrchestra17;
    DSPFix48 _reservedOrchestra18;
    int _reservedOrchestra19;
    int _reservedOrchestra20;
    int _reservedOrchestra21;
    int _reservedOrchestra22;
    int _reservedOrchestra23;
    int _reservedOrchestra24;
    int _reservedOrchestra25;
    float _reservedOrchestra26;
    float _reservedOrchestra27;
    void *_reservedOrchestra28;
    void *_reservedOrchestra29;
}

/* Definitions for private instance variables */
#define _sysUG _reservedOrchestra1
#define _looper _reservedOrchestra2
#define _availDataMem _reservedOrchestra3
#define _peLoop _reservedOrchestra4
#define _sharedSet _reservedOrchestra5
#define _piLoop _reservedOrchestra6
#define _xArg _reservedOrchestra7
#define _yArg _reservedOrchestra8
#define _lArg _reservedOrchestra9
#define _xPatch _reservedOrchestra10
#define _yPatch _reservedOrchestra11
#define _xPatchAllocBits _reservedOrchestra12
#define _yPatchAllocBits _reservedOrchestra13
#define _headroom _reservedOrchestra14
#define _effectiveSamplePeriod _reservedOrchestra15
#define _orchloopClass _reservedOrchestra16
#define _previousLosingTemplate _reservedOrchestra17
#define _previousTimeStamp _reservedOrchestra18
#define _parenCount _reservedOrchestra19
#define _bottomOfMemory _reservedOrchestra20
#define _bottomOfExternalMemory _reservedOrchestra21
#define _topOfExternalMemory _reservedOrchestra22
#define _onChipPPPartitionSize _reservedOrchestra23
#define _numXArgs _reservedOrchestra24
#define _numYArgs _reservedOrchestra25
#define _xArgPercentage _reservedOrchestra26
#define _yArgPercentage _reservedOrchestra27
#define _simFP _reservedOrchestra28
#define _extraVars _reservedOrchestra29

/* This struct is for instance variables added after the 2.0 instance variable
   freeze. (Actually these could have been real instance variables, but 
   I learned too late that there was NOT an instance var freeze for the
   Music Kit in 2.0. Sigh. Anyway, it provides a mechanism for adding instance
   variables after 2.0.) */
typedef struct __extraInstanceVars {
    double previousTime;
    NXHashTable *sharedGarbage;
    char *simulatorFile;
    id readDataUG;
    id xReadData;
    id yReadData;
    double timeOffset;
    double synchTimeRatio;
    DPSTimedEntry timedEntry;
    BOOL synchToConductor;
} _extraInstanceVars;

/* Macros to access extra instance vars */
#define _previousTime(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->previousTime
#define _sharedGarbage(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->sharedGarbage
#define _simulatorFile(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->simulatorFile
#define _readDataUG(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->readDataUG
#define _xReadData(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->xReadData
#define _yReadData(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->yReadData
#define _timeOffset(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->timeOffset
#define _synchTimeRatio(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->synchTimeRatio
#define _timedEntry(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->timedEntry
#define _synchToConductor(_self) \
  ((_extraInstanceVars *)_self->_extraVars)->synchToConductor
  
/* Some global variables. --------------------------------------- */
static BOOL orchClassInited = NO;
static char isTimedDefault = YES;

#define DEFAULTSRATE         22050.0
#define DEFAULTHEADROOM      .1
#define HEADROOMFUDGE        (-(.1 + .025))

static double samplingRateDefault = DEFAULTSRATE;
static BOOL fastResponseDefault = NO;
static double headroomDefault = DEFAULTHEADROOM;
static double localDeltaTDefault = 0;

/* Memory allocation primitives */
typedef struct _dataMemBlockStruct {
    DSPAddress baseAddr;
    DSPAddress size;
    BOOL isAllocated;
    struct _dataMemBlockStruct *next,*prev;
} dataMemBlockStruct;

#define DATAMEMBLOCKCACHESIZE 32
static dataMemBlockStruct *dataMemBlockCache[DATAMEMBLOCKCACHESIZE];
static unsigned dataMemBlockCachePtr = 0;

static dataMemBlockStruct *allocDataMemBlock(void)
    /* alloc a new structure that keeps track of data memory. */
{
    if (dataMemBlockCachePtr) 
      return dataMemBlockCache[--dataMemBlockCachePtr]; 
    else {
        dataMemBlockStruct *theBlock;
        _MK_MALLOC(theBlock,dataMemBlockStruct,1);
        return theBlock;
    }
}

static void initDataMemBlockCache(void)
    /* init cache of structs that keep track of data memory */
{
    int i;
    for (i=0; i<DATAMEMBLOCKCACHESIZE; i++)
      _MK_MALLOC(dataMemBlockCache[i],dataMemBlockStruct,1);
}

static dataMemBlockStruct *freeDataMemBlock(dataMemBlockStruct *block)
    /* Free a dataMemBlockStruct. Cache it, if possible */
{
    if (block) {
        if (dataMemBlockCachePtr < DATAMEMBLOCKCACHESIZE) {
            dataMemBlockCache[dataMemBlockCachePtr++] = block;
        }
        else NX_FREE(block);
    }
    return NULL;
}

static id allocUG(); /* Forward refs. */
static void givePatchMem();
static void giveDataMem();
static DSPAddress getPatchMem();
static DSPAddress getDataMem();
static DSPAddress getPELoop();
static BOOL givePELoop();

static id *patchTemplates = NULL; /* Array of PatchTemplates */
static int nTemplates = 0;  /* Number of tempaltes about which the orchestra 
                               knows. */
static unsigned short nDSPs = 1;

#define FOREACHORCH(_i) for (_i=0; _i<nDSPs; _i++) if (orchestras[i])

static id defaultOrchloopClass = nil;

static id *orchestras = NULL; 
/* All orchestra instances, indexed by DSP number. (0 based) */

_MK_ERRMSG garbageMsg = "Garbage collecting freed unit generator %s_%d";

static char * orchMemSegmentNames[(int)MK_numOrchMemSegments] = 
{"noSegment","pLoop","pSubr","xArg","yArg","lArg","xData","yData","lData",
   "xPatch","yPatch","lPatch"};

/* Initialization methods --------------------------------------- */

+initialize
  /* Sent once at start-up time. */
{
    static BOOL beenHere = NO;
    if ((self != [Orchestra class]) || (beenHere))
      return self;
    beenHere = YES;
    _MKLinkUnreferencedClasses([UnitGenerator class],[SynthData class],
                               [_OrchloopbeginUG class]);
    NX_ASSERT(((DSP_SINE_SPACE == 2) && (DSP_MULAW_SPACE == 1)),
              "Need to change SINTABLESPACE or MULAWTABLESPACE.");
    defaultOrchloopClass = [_OrchloopbeginUG class];
    nDSPs = (unsigned short)DSPGetDSPCount();
    _MK_CALLOC(orchestras,id,(int)nDSPs);
    return self;
}    

static void classInit()
{
    orchClassInited = YES;
    //    [MKOrchestraClasses() addObject:[Orchestra class]];
    _MKCheckInit();
    initDataMemBlockCache();
}

+alloc
{
    return [self allocFromZone:NXDefaultMallocZone() onDSP:0];
}

+allocFromZone:(NXZone *)zone
  /* We override this method to behave like +new.  Zones are not currently
     supported in Orchestra objects. */
{
    return [self allocFromZone:zone onDSP:0];
}

+allocFromZone:(NXZone *)zone onDSP:(unsigned short)index
{
    Orchestra *orch;
    if (!orchClassInited)
      classInit();
    if (index >= nDSPs)
      return nil;
    if (orchestras[index])
      return orchestras[index];
    orch = [super allocFromZone:zone];
    orch->orchIndex = index;
    return orch;
}

+new
{
    return [[self alloc] init];
}

#if 0
+newOnAllDSPs
  /* Create all orchestras (one per DSP) by sending them the newOnDSP: method
     (see below). Does not claim the DSP device at this time. 
     You can check to see which Orchestra objects are created by using
     the +nthOrchestra: method. Returns the first Orchesra instance. */
{
    unsigned short i;
    id rtn = nil;
    for (i=0; i<nDSPs; i++)  {
        [self newOnDSP:i];
        if (!rtn)
          rtn = orchestras[i];
    }
    return rtn;
}
#endif


#define DEBUG_DELTA_T 0

#if DEBUG_DELTA_T
/* This stuff allows us to tell exactly when stuff is sent to the dsp. 
   It is normally commented out. */

#import <sys/time_stamp.h>

#define MAX_STAMPS      200

static unsigned timeStamps[MAX_STAMPS];
static int numStamps = 0;

static double MKTimeStamps[MAX_STAMPS];
static int numMKStamps = 0;

static unsigned DSPTimeStamps[MAX_STAMPS];
static int numDSPStamps = 0;

static void _timeStamp(void)
{
    struct tsval timeStruct;
    
    if (numStamps == MAX_STAMPS)
      return;
    kern_timestamp(&timeStruct);
    timeStamps[numStamps++] = timeStruct.low_val;
}

void _printTimeStamps(void)
{
    
    int i;
    
    printf("Number of time stamps: %d\n", numStamps);
    if (numStamps == MAX_STAMPS)
      printf("MAY HAVE MISSED SOME TIME STAMPS!!!!\n");
    printf("time\tdelta\n");
    for (i = 0; i < numStamps; i++) {
        if (i != 0)
          printf("\t%d\n", timeStamps[i] - timeStamps[i-1]);
        printf("%u\n", timeStamps[i]);
    }
    printf("Number of DSP time stamps: %d\n", numStamps);
    if (numDSPStamps == MAX_STAMPS)
      printf("MAY HAVE MISSED SOME TIME STAMPS!!!!\n");
    printf("time\tdelta\n");
    for (i = 0; i < numDSPStamps; i++) {
        if (i != 0)
          printf("\t%d\n", DSPTimeStamps[i] - DSPTimeStamps[i-1]);
        printf("%u\n", DSPTimeStamps[i]);
    }
    printf("Number of MK time stamps: %d\n", numStamps);
    if (numMKStamps == MAX_STAMPS)
      printf("MAY HAVE MISSED SOME TIME STAMPS!!!!\n");
    printf("time\tdelta\n");
    for (i = 0; i < numMKStamps; i++) {
        if (i != 0)
          printf("\t%f\n", MKTimeStamps[i] - MKTimeStamps[i-1]);
        printf("%u\n", MKTimeStamps[i]);
    }
}

static void _DSPTimeStamp()
{
    int stamp;
    DSPFix48 aTimeStamp = *_MKCurSample(self);
    if (numDSPStamps == MAX_STAMPS)
      return;
    DSPMKRetValueTimed(&aTimeStamp,DSP_MS_Y,2,&stamp);
    DSPTimeStamps[numStamps++] = stamp;
}

static void _MKTimeStamp()
{
    if (numDSPStamps == MAX_STAMPS)
      return;
    MKTimeStamps[numStamps++] = MKGetTime();
}

#endif

-flushTimedMessages
  /* Flush timed messages. */
{
    if (deviceStatus == MK_devClosed)
      return self;
    DSPSetCurrentDSP(orchIndex);
    DSPMKFlushTimedMessages();
#if DEBUG_DELTA_T
    /* This is normally commented out */
    _DSPTimeStamp();
    _timeStamp();
    _MKTimeStamp();
#endif
    return self;
}

+(unsigned short)DSPCount
  /* Returns the number of DSPs configured on the NeXT machine. The standard
     configuration contains one DSP. This is not necessarily the number of
     DSPs available to your program as other programs may have other DSPs
     assigned. */
{
    return nDSPs;
}

static void setLooper(); /* forward ref */
static int resoAlloc();

static id synthInstruments = nil;

-setOnChipMemoryConfigDebug:(BOOL)debugIt patchPoints:(short)count
  /* Sets configuration of onchip memory
     The default is no debug and 13 on-chip patchpoints. You may need
     to set onchip patchpoints to a smaller number if you are using 
     unit generators with a lot of l arguments. 
     You may not reconfigure when open. Setting count to 0 uses a default
     value. */
{
    int i = onChipPatchPoints;
    if (deviceStatus != MK_devClosed)
      return nil;
    _bottomOfMemory = ((debugIt) ? _PLI_USR : _DEGMON_L);   
    onChipPatchPoints = ((count) ? count : DEFAULTONCHIPPATCHPOINTS);
    if ((MAXLARG-MINLARG) <= 0) 
      if (i)
        onChipPatchPoints = i; /* Don't set it */
    _onChipPPPartitionSize = DSPMK_NTICK * self->onChipPatchPoints;
    return self;
}

-setOffChipMemoryConfigXArg:(float)xPercentage yArg:(float)yPercentage;
{
    if (xPercentage <= 0) 
      xPercentage = XARGPCDEFAULT;
    if (yPercentage <= 0) 
      yPercentage = YARGPCDEFAULT;
    if ((deviceStatus != MK_devClosed) || (xPercentage + yPercentage > 1.0)) 
      return nil;
    _xArgPercentage = xPercentage;
    _yArgPercentage = yPercentage;
    return self;
}

-init
{
    if (_extraVars) /* Already initialized */
      return self;
    [super init];
    _MK_CALLOC(_extraVars,_extraInstanceVars,1);
    deviceStatus = MK_devClosed;
    [self setOnChipMemoryConfigDebug:NO patchPoints:0];
    [self setOffChipMemoryConfigXArg:XARGPCDEFAULT yArg:YARGPCDEFAULT];
    _simFP = NULL;
    orchestras[orchIndex] = self;
    [self setSamplingRate:samplingRateDefault];
    [self setFastResponse:fastResponseDefault];
    [self setHeadroom:headroomDefault];
    [self setLocalDeltaT:localDeltaTDefault];
    [self setTimed:isTimedDefault];     
    [self setSoundOut:YES];
    useDSP = YES;
    return self;
}

+newOnDSP:(unsigned short)index
  /* Creates an orchestra object corresponding with the orchIndex'th DSP. 
     (DSPs are numbered from 0.) 
     The index must be of a valid DSP. If there is currently an 
     orchestra instance for that index, returns that orchestra.
     */
{
    return [[self allocFromZone:NXDefaultMallocZone() onDSP:index] init];
}

+nthOrchestra:(unsigned short)index
  /* Returns the index'th orchestra or nil if none. */
{
    if (index >= nDSPs)
      return nil;
    return orchestras[index];
}

static id broadcastAndRtn(Orchestra *self,SEL sel)
    /* Does broadcast. Returns nil if any orchs return nil, else self. */
{
    register unsigned short i;
    id rtn = self;
    id tmp;
    FOREACHORCH(i) {
        tmp = [orchestras[i] perform:sel];
        if (!tmp)
          rtn = nil;
    }
    return rtn;
}

+flushTimedMessages
  /* Send all buffered DSP messages immediately for all orchestras. 
     Returns self. */
{
    register unsigned short i;
    FOREACHORCH(i) 
      [orchestras[i] flushTimedMessages];
    return self;
}

+free
  /* Frees all orchestra objects by sending -free to each orchestra object. 
     Returns self. */
{
    broadcastAndRtn(self,@selector(free));
    return self;
}

+open
  /* Sends open to each orchestra object. Returns nil if one of the
     Orchestra returns nil, else self. */
{
    return broadcastAndRtn(self,@selector(open));
}

+run
  /* Sends run to each orchestra object. Returns nil if one of the 
     Orchestras does, else self. */
{
    return broadcastAndRtn(self,@selector(run));
}

+stop
  /* Sends stop to each orchestra object. */
{
    return broadcastAndRtn(self,@selector(stop));
}

#if 0
+step
  /* Sends step to each orchestra object. */
{
    unsigned short i;
    FOREACHORCH(i)
      [orchestras[i] step];
    return self;
}
#endif

+close
  /* Sends close to each orchestra object. Returns nil if one of the 
     Orchestras does, else self. */
{
    return broadcastAndRtn(self,@selector(close));
}

+abort
  /* Sends abort to each orchestra object. Returns nil if one of the 
     Orchestras does, else self. */
{
    return broadcastAndRtn(self,@selector(abort));
}


+setSamplingRate:(double)newSRate
  /* Sets sampling rate (for all orchs). It is illegal to do this while an
     orchestra is open. */
{
    unsigned short i;
    samplingRateDefault = newSRate;
    FOREACHORCH(i)
      [orchestras[i] setSamplingRate:newSRate];
    return self;
}

+setFastResponse:(char)yesOrNo
  /* Sets whether fast response (small sound out buffers) is used (for all
     orchs). It is illegal to do this while an
     orchestra is open */
{
    unsigned short i;
    fastResponseDefault = yesOrNo;
    FOREACHORCH(i)
      [orchestras[i] setFastResponse:fastResponseDefault];
    return self;
}

+setHeadroom:(double)newHeadroom
  /* Sets headroom for all orchs. */
{
    unsigned short i;
    headroomDefault = newHeadroom;
    FOREACHORCH(i)
      [orchestras[i] setHeadroom:newHeadroom];
    return self;
}

+setLocalDeltaT:(double)newLocalDeltaT
  /* Sets localDeltaT for all orchs. */
{
    unsigned short i;
    localDeltaTDefault = newLocalDeltaT;
    FOREACHORCH(i)
      [orchestras[i] setLocalDeltaT:localDeltaTDefault];
    return self;
}


+setTimed:(char)areOrchsTimed
  /* Controls (for all orchs) whether DSP commands are sent timed or untimed. 
     Untimed DSP commands are executed as soon as they are received by the DSP.
     It is permitted to change from timed to untimed during a performance, 
     but it will not work correctly in release 0.9.  */
{
    int  i;
    isTimedDefault = areOrchsTimed;
    FOREACHORCH(i)
      [orchestras[i] setTimed:areOrchsTimed];
    return self;
}

-(char)isTimed
  /* Returns whether DSP commands are sent timed. */
{
    return isTimed;
}

/* Allocation methods for the entire orchestra. ----------------------- */

+allocUnitGenerator:  classObj 
  /* Allocate a unit generator of the specified class
     on the first DSP which has room. */
{
    id rtnVal;
    register unsigned short i;
    FOREACHORCH(i)
      if (rtnVal = [orchestras[i] allocUnitGenerator:classObj])
        return rtnVal;
    return nil;
}

+allocSynthData:(MKOrchMemSegment)segment length:(unsigned )size
  /* Allocate some private data memory on the first DSP which has room. */
{
    id rtnVal;
    register unsigned short i;
    FOREACHORCH(i)
      if (rtnVal = [orchestras[i] allocSynthData:segment length:size])
        return rtnVal;
    return nil;
}

+allocPatchpoint:(MKOrchMemSegment)segment 
  /* Allocate patch memory on first DSP which has room. segment must
     be MK_xPatch or MK_yPatch. */
{
    id rtnVal;
    register unsigned short i;
    FOREACHORCH(i)
      if (rtnVal = [orchestras[i] allocPatchpoint:segment])
        return rtnVal;
    return nil;
}

+allocSynthPatch:aSynthPatchClass
  /* Same as allocSynthPatch:patchTemplate: but uses default template. The default
     template is obtained by sending [aSynthPatchClass defaultPatchTemplate].*/
{
    return [self allocSynthPatch:aSynthPatchClass patchTemplate:
            [aSynthPatchClass defaultPatchTemplate]];
}

+allocSynthPatch:aSynthPatchClass
 patchTemplate:p
  /* Get a SynthPatch on the first DSP which has sufficient resources. */
{
    id rtnVal;
    register int i;
    FOREACHORCH(i)
      if (rtnVal = [orchestras[i] allocSynthPatch:aSynthPatchClass
                  patchTemplate:p])
        return rtnVal;
    return nil;
}

+dealloc:aSynthResource
  /* Deallocates aSynthResource by sending it the dealloc
     message. 
     aSynthResource may be a UnitGenerator, a SynthData or a SynthPatch.
     This method is provided for symmetry with the alloc family
     of methods. */
{
    return [aSynthResource dealloc];
}

-dealloc:aSynthResource
  /* Deallocates aSynthResource by sending it the dealloc
     message. 
     aSynthResource may be a UnitGenerator, a SynthData or a SynthPatch.
     This method is provided for symmetry with the alloc family
     of methods. */
{
    return [aSynthResource dealloc];
}

/* Assorted instance methods and functions. ----------------------------- */

NXHashTable *_MKGetSharedSynthGarbage(Orchestra *self)
{
    return _sharedGarbage(self);
}

-sharedObjectFor:aKeyObj 
 segment:(MKOrchMemSegment)whichSegment 
 length:(int)length
  /* This method returns the SynthData, UnitGenerator or SynthPatch instance 
     representing anObj in the specified segment and increments its reference 
     count, if such an object exists. 
     If not, or if the orchestra is not open, returns nil. 
     anObj is any object associated with the abstract notion of the data.
     The object comparison is done on the basis of aKeyObj's id. */
{
    if (deviceStatus == MK_devClosed)
      return nil;
    return _MKFindSharedSynthObj(_sharedSet,_sharedGarbage(self),aKeyObj,
                                 whichSegment,length);
}

-sharedObjectFor:aKeyObj segment:(MKOrchMemSegment)whichSegment
{
    return [self sharedObjectFor:aKeyObj segment:whichSegment length:0];
}

-sharedObjectFor:aKeyObj 
{
    return [self sharedObjectFor:aKeyObj segment:MK_noSegment length:0];
}

static id installSharedObject(Orchestra *self,
                              id aSynthObj,
                              id aKeyObj,
                              MKOrchMemSegment whichSegment,
                              int length)
    /* This function installs the synthObj into the shared 
       table in the specified segment and sets the reference count to 1.
       Does nothing and returns nil if aKeyObj is already represented for that
       segment. Also returns nil if the orchestra is not open. Otherwise, 
       returns self. 
       aKeyObj is any object associated with the abstract notion of the data.
       aKeyObj is not copied and should not be freed while any shared data 
       associated with it exists. 
       */
{
    if (self->deviceStatus == MK_devClosed)
      return nil;
    if (_MKInstallSharedObject(self->_sharedSet,aSynthObj,aKeyObj,whichSegment,
                               length)) {
        if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
          _MKOrchTrace(self,MK_TRACEORCHALLOC,
                       "Installing shared data %s in segment %s.",
                       [aKeyObj name],[self segmentName:whichSegment]);
        return self;
    }
    return nil;
}

-installSharedSynthDataWithSegmentAndLength:aSynthDataObj
 for:aKeyObj
{
    return installSharedObject(self,aSynthDataObj,aKeyObj,
                               [aSynthDataObj orchAddrPtr]->memSegment, 
                               [aSynthDataObj length]);
}

-installSharedSynthDataWithSegment:aSynthDataObj
 for:aKeyObj
{
    return installSharedObject(self,aSynthDataObj,aKeyObj,
                               [aSynthDataObj orchAddrPtr]->memSegment,0); 
}

-installSharedObject:aSynthObj 
 for:aKeyObj
{
    return installSharedObject(self,aSynthObj,aKeyObj, 
                               MK_noSegment,0);
}

-sineROM
  /* Returns a SynthData object representing the SineROM. You should never
     deallocate this object. */
{
    return sineROM;
}

-muLawROM
  /* Returns a SynthData object representing the MuLawROM. You should never
     deallocate this object. */
{
    return muLawROM;
}

/* READ DATA */
-segmentInputSoundfile:(MKOrchMemSegment)segment
  /* Returns special pre-allocated Patchpoint which always holds 0 and which,
     by convention, nobody ever writes to. This patch-point may not be
     freed. You should not deallocate
     the returned value. Segment should be MK_xPatch or MK_yPatch. */
{
    return ((segment == MK_xPatch) || (segment == MK_xData)) ? 
      _xReadData(self) : _yReadData(self);
}

-segmentZero:(MKOrchMemSegment)segment
  /* Returns special pre-allocated Patchpoint which always holds 0 and which,
     by convention, nobody ever writes to. This patch-point may not be
     freed. You should not deallocate
     the returned value. Segment should be MK_xPatch or MK_yPatch. */
{
    return ((segment == MK_xPatch) || (segment == MK_xData)) ? xZero : yZero;
}

-segmentSink:(MKOrchMemSegment)segment
  /* Returns special pre-allocated Patchpoint from which nobody reads, by
     convention. This patch-point may not be freed. 
     You should not deallocate the returned value. 
     Segment should be MK_xPatch or MK_yPatch. */
{
    return ((segment == MK_xPatch) || (segment == MK_xData)) ? xSink : ySink;
}

-(double)samplingRate
  /* Returns samplingRate. */
{
    return samplingRate;
}

-setSamplingRate:(double)newSRate
  /* Set sampling rate. Only legal when receiver is closed. Returns self
     or nil if receiver is not closed. */ 
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    samplingRate =  newSRate;
    _effectiveSamplePeriod = (1.0 / newSRate) * (1 - _headroom);
    return self;
}

-setFastResponse:(char)yesOrNo
  /* Set whether response is fast. 
     Only legal when receiver is closed. Returns self
     or nil if receiver is not closed. */ 
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    fastResponse = yesOrNo;
    return self;
}

-(char)fastResponse
{
    return fastResponse;
}

-setOutputSoundfile:(char *)file
  /* Sets a file name to which output samples are to be written as a 
     soundfile (the string is copied). In the current release, it
     is not permissable to have an output soundfile and do sound-out at the same
     time. This message is currently ignored if the receiver is not closed. 
     If you re-run the Orchestra, the file is rewritten. To specify that
     you no longer want a file when the Orchestra is re-run, close the Orchestra,
     then send setOutputSoundfile:NULL. 
     
     Note that sending setOutputSoundfile:NULL does not automatically 
     send setSoundOut:YES. You must do this yourself. */
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    if (outputSoundfile) {
        NX_FREE(outputSoundfile);
        outputSoundfile = NULL;
    }
    if (!file)
      return self;
    outputSoundfile = _MKMakeStr(file);
    soundOut = NO;
    [self useDSP:YES];
    return self;
}

-(char *)outputSoundfile
  /* Returns the output soundfile or NULL if none. */
{
    return outputSoundfile;
}

-setOutputCommandsFile:(char *)file
  /* Sets a file name to which DSP commands are to be written as a DSPCommands
     format soundfile.  A copy of the fileName is stored in the instance variable
     outputCommandsFile.
     This message is currently ignored if the receiver is not closed.
     */
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    if (outputCommandsFile) {
        NX_FREE(outputCommandsFile);
        outputCommandsFile = NULL;
    }
    if (!file)
      return self;
    outputCommandsFile = _MKMakeStr(file);
    [self useDSP:YES];
    return self;
}

-(char *)outputCommandsFile
  /* Returns the output soundfile or NULL if none. */
{
    return outputCommandsFile;
}

/* READ DATA */
-setInputSoundfile:(char *)file
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    if (inputSoundfile) {
        NX_FREE(inputSoundfile);
        inputSoundfile = NULL;
    }
    if (!file)
      return self;
    inputSoundfile = _MKMakeStr(file);
    return self;
}

/* READ DATA */
-(char *)inputSoundfile
  /* Returns the input soundfile or NULL if none. */
{
    return inputSoundfile;
}

-setSimulatorFile:(char *)filename
  /* Sets a file name to which logfile output suitable for the DSP simulator
     is to be written. In the current release a complete log is only available
     when doing sound-out or writing a soundfile.
     This message is currently ignored if the receiver is not closed. 
     If you re-run the Orchestra, the file is rewritten. To specify that
     you no longer want a file when the Orchestra is re-run, close the Orchestra,
     then send setSimulatorFile:NULL.  */
{
    if (deviceStatus != MK_devClosed) 
      return nil;
    if (_simulatorFile(self)) {
        NX_FREE(_simulatorFile(self));
        _simulatorFile(self) = NULL;
    }
    if (!filename)
      return self;
    _simulatorFile(self) = _MKMakeStr(filename);
    return self; 
}

-(char *)simulatorFile
  /* Gets text file being used for DSP log file output, if any. */
{
    return _simulatorFile(self);
}

-setSoundOut:(BOOL)yesOrNo
  /* Controls whether sound is sent to the DACs. The default is YES. 
     It is not permissable to have an output soundfile and do sound-out at the
     same time. Thus, sending setSoundOut:YES also sends 
     setOutputSoundfile:NULL. If the receiver is not closed, this message has 
     no effect.
     */
{
    if (deviceStatus != MK_devClosed)
      return nil;
    soundOut = yesOrNo;
    if (soundOut)
      [self setOutputSoundfile:NULL];
    [self useDSP:YES];
    return self;
}

-(BOOL)soundOut
  /* Returns whether or not sound-out is being used. */
{
    return soundOut;
}

-setSSISoundOut:(BOOL)yesOrNo
  /* Controls whether sound is sent to the SSI port. The default is NO. 
     If the receiver is not closed, this message has no effect.
     */
{
    if (deviceStatus != MK_devClosed)
      return nil;
    SSISoundOut = yesOrNo;
    [self useDSP:YES];
    return self;
}

-(BOOL)SSISoundOut
  /* Returns whether or not sound is being sent to the SSI port. */
{
    return SSISoundOut;
}

/* Methods that do Orchestra control (e.g. open, close, free, etc.) */


static BOOL popResoAndSetLooper(Orchestra *self);
static BOOL popReso();

#define TWO_TO_24   ((double) 16777216.0)
#define TWO_TO_48   (TWO_TO_24 * TWO_TO_24)

static DSPFix48 *doubleIntToFix48UseArg(double dval,DSPFix48 *aFix48)
    /* dval is an integer stored in a double. */
{
    double shiftedDval;
#   define TWO_TO_M24 ((double)5.9604644775390625e-08)
    if (dval < 0) 
      dval = 0;
    if (dval > TWO_TO_48)
      dval = TWO_TO_48;
    shiftedDval = dval * TWO_TO_M24;
    aFix48->high24 = (int)shiftedDval;
    aFix48->low24 = (int)((shiftedDval - aFix48->high24) * TWO_TO_24);
    return aFix48;
}


static void freeUGs(self)
    Orchestra *self;
    /* Free all SynthData and UnitGenerators. */
{
    dataMemBlockStruct *tmp;
    char wasTimed = [self isTimed];
    self->deviceStatus = MK_devClosed;
    [self setTimed:NO];          /* Finalization may generate some
                                    reset code and we don't want it
                                    to go out timed. */
    [synthInstruments makeObjectsPerform:@selector(_disconnectOnOrch:) 
   with:self];
    /* Causes each synthIns to end any residual running voices and deallocate
       its own idle voices so that popReso gets all. */
    popReso(self);                    /* Doesn't reset looper. */
    if ([self->stack lastObject]) {
        _MKErrorf(MK_orchBadFreeErr);
        return;
    }
    if (self->inputSoundfile) /* READ DATA */
      [_readDataUG(self) _free];
    self->_sysUG = [self->_sysUG _free];
    _MKProtectSynthElement(self->xZero,NO);
    [self->xZero dealloc];
    _MKProtectSynthElement(self->yZero,NO);
    [self->yZero dealloc];
    _MKProtectSynthElement(self->ySink,NO);
    [self->ySink dealloc];
    _MKProtectSynthElement(self->xSink,NO);
    [self->xSink dealloc];
    _MKProtectSynthElement(self->sineROM,NO);
    [self->sineROM dealloc];
    _MKProtectSynthElement(self->muLawROM,NO);
    if (self->inputSoundfile) { /* READ DATA */
        _MKProtectSynthElement(_xReadData(self),NO);
        _MKProtectSynthElement(_yReadData(self),NO);
        [_xReadData(self) dealloc];
        [_yReadData(self) dealloc];
    }
    [self->muLawROM dealloc];
    _MKCollectSharedDataGarbage(self,_sharedGarbage(self));
    self->muLawROM = self->sineROM = self->xZero = self->yZero = 
      self->xSink = self->ySink = nil;
    while ((dataMemBlockStruct *)self->_peLoop) {     
        /* Free memory data structure. */
        tmp = ((dataMemBlockStruct *)self->_peLoop)->next; 
        freeDataMemBlock(self->_peLoop);
        self->_peLoop = (void *)tmp;
    }
    self->_sharedSet = _MKFreeSharedSet(self->_sharedSet,
                                        &_sharedGarbage(self));
    self->stack = [self->stack free];
    NX_FREE(self->_xPatch);
    NX_FREE(self->_yPatch);
    self->_xPatch = NULL;
    self->_yPatch = NULL;
    self->_xPatchAllocBits = 0;
    self->_yPatchAllocBits = 0;
    [self setTimed:wasTimed];
}

#if 0
/* Comment this in if needed */
void _MKSetDefaultOrchloopClass(id anOrchSysUGClass)
{
    defaultOrchloopClass = anOrchSysUGClass;
}

void _MKSetOrchloopClass(Orchestra *self,id anOrchSysUGClass)
{
    self->_orchloopClass = anOrchSysUGClass;
}    
#endif

/* Set NOOPS to 3 to insert 3 noops between each unit generator. This is 
   useful for DSP debugging. */
#define NOOPS 0

#ifdef DEBUG
static int noops = NOOPS;
#else 
static int noops = 0;
#endif

void _MKOrchestraSetNoops(int nNoops)
{
    noops = nNoops;
}

#define NOOP 0x0      /* Used to separate unit generators. */

static void insertNoops(self,where)
    Orchestra *self;
    int where;
    /* Inserts NOOPS noops between each unit generator. This is 
       useful for DSP debugging. */
{
    if (!noops)
      return;
    DSPSetCurrentDSP(self->orchIndex);
    if (_MK_ORCHTRACE(self,MK_TRACEDSP))
      _MKOrchTrace(self,MK_TRACEDSP,"inserting %d NOOPs at %d",noops,where);
    DSPMKMemoryFillSkipTimed(_MKCurSample(self),NOOP,DSP_MS_P,where,1,noops);
}

static id loadOrchLoop(self)
    Orchestra *self;
    /* Loads the orchestra loop (a.k.a. _OrchloopbeginUG, 
       a.k.a. orchloopbegin) 
       Also initializes memory data structures, etc. */
{
    int i;
    MKOrchMemStruct reloc;
    dataMemBlockStruct *peLoop,*availDataMem;
    _previousTime(self) = -1;
    self->_previousTimeStamp.high24 = 0;
    self->_previousTimeStamp.low24 = 0;
    if (!self->_orchloopClass)
      self->_orchloopClass = defaultOrchloopClass;
    self->deviceStatus = MK_devOpen;
    self->_sharedSet = _MKNewSharedSet(&_sharedGarbage(self));
    self->stack = [List new];
    self->computeTime = 0;
    self->_piLoop = ORCHLOOPLOC;
    self->_xArg = MINXARG;
    self->_yArg = MINYARG;
    self->_lArg = MINLARG;  
    self->_looper = SHORTJUMP | MINPILOOP; /* This is the start of _sysUG. */
    [self->_orchloopClass _setXArgsAddr:self->_xArg y:self->_yArg l:
     self->_lArg looper:self->_looper]; 
    /* Set argument start addresses to point to correct locations.
       Also set ug to loop */
    _MK_MALLOC(peLoop, dataMemBlockStruct, 1);
    _MK_MALLOC(availDataMem,dataMemBlockStruct,1);
    self->_availDataMem = (void *)availDataMem;
    self->_peLoop = (void *)peLoop;
    availDataMem->baseAddr = MINXDATA;
    availDataMem->size = MAXXDATA - MINXDATA + 1;
    availDataMem->isAllocated = NO;
    (peLoop)->baseAddr = MINPELOOP;
    (peLoop)->size = LOOPERSIZE;
    (peLoop)->isAllocated = YES;
    (peLoop)->next = availDataMem;
    (availDataMem)->prev = peLoop;
    (peLoop)->prev = NULL;
    (availDataMem)->next = NULL;
    _MK_MALLOC(self->_xPatch,DSPAddress,self->onChipPatchPoints);
    _MK_MALLOC(self->_yPatch,DSPAddress,self->onChipPatchPoints);
    self->_xPatch[0] = MINXPATCH;
    self->_yPatch[0] = MINYPATCH;
    for (i = 1; i < self->onChipPatchPoints; i++) {
        self->_xPatch[i] = self->_xPatch[i - 1] + DSPMK_NTICK;
        self->_yPatch[i] = self->_yPatch[i - 1] + DSPMK_NTICK;
    }
    /* Allocate sink first in case we ever decide to have just 1 patchpoint 
       on chip
     */
    self->xSink = [self allocPatchpoint:MK_xPatch];
    self->ySink = [self allocPatchpoint:MK_yPatch];
    self->xZero = [self allocPatchpoint:MK_xPatch];
    [self->xZero clear];
    self->yZero = [self allocPatchpoint:MK_yPatch];
    [self->yZero clear];
    self->sineROM = [SynthData _newInOrch:self index:self->orchIndex 
                   length:DSP_SINE_LENGTH segment:SINTABLESPACE 
                   baseAddr:DSP_SINE_TABLE];
    self->muLawROM = [SynthData _newInOrch:self index:self->orchIndex 
                    length:DSP_MULAW_LENGTH segment:MULAWTABLESPACE 
                    baseAddr:DSP_MULAW_TABLE];
    [self->muLawROM setReadOnly:YES];
    [self->sineROM setReadOnly:YES];
    [self->xZero setReadOnly:YES];
    [self->yZero setReadOnly:YES];
    _MKProtectSynthElement(self->xZero,YES);
    _MKProtectSynthElement(self->yZero,YES);
    _MKProtectSynthElement(self->ySink,YES);
    _MKProtectSynthElement(self->xSink,YES);
    _MKProtectSynthElement(self->sineROM,YES);
    _MKProtectSynthElement(self->muLawROM,YES);
    if (self->inputSoundfile){ /* READ DATA */
        _MKProtectSynthElement(_xReadData(self),YES);
        _MKProtectSynthElement(_yReadData(self),YES);
    }
#if READDATA 
    /* Need something here to selectively create either the stereo or the
       mono Read data UnitGenerator. Or maybe it'll just be one UnitGenerator.
       */
    if (((resoAlloc(self,self->_orchSysUGClass,&reloc) != OK) || 
         self->isLoopOffChip || 
         (!(self->_sysUG = 
            [self->_orchSysUGClass _newInOrch:self index:self->orchIndex
           reloc:&reloc looper:self->_looper]))) ||
        ((inputSoundfile != NULL) &&  /* READ DATA */
         ((resoAlloc(self,_readDataUG(self),&reloc) != OK) || 
          (!(_readDataUG(self) = 
             [SysReadDataUG _newInOrch:self index:self->orchIndex
            reloc:&reloc looper:self->_looper]))))) { /* Should never happen */
        if (self->_sysUG)
          self->_sysUG = [self->_sysUG _free];
        freeUGs(self);
        if (self->useDSP)
          DSPClose();
        self->deviceStatus = MK_devClosed;
        return nil;
    }
#else
    if ((resoAlloc(self,self->_orchloopClass,&reloc) != OK) || 
        self->isLoopOffChip || 
        (!(self->_sysUG = 
           [self->_orchloopClass _newInOrch:self index:self->orchIndex
          reloc:&reloc looper:self->_looper]))) { /* Should never happen */
        freeUGs(self);
        if (self->useDSP)
          DSPClose();
        self->deviceStatus = MK_devClosed;
        return nil;
    }
#endif
    insertNoops(self,reloc.pLoop - noops);
    return self;
}

void _MKOrchAddSynthIns(id anIns)
    /* The Orchestra keeps track of all the SynthInstruments so it can
       clean up at the end of time. */
{
    if (!synthInstruments)
      synthInstruments = [List new];
    [synthInstruments addObject:anIns];
}

void _MKOrchRemoveSynthIns(id anIns)
    /* The Orchestra keeps track of all the SynthInstruments so it can
       clean up at the end of time. */
{
    [synthInstruments removeObject:anIns];
}

-(MKDeviceStatus)deviceStatus
  /* Returns MKDeviceStatus of receiver. */
{
    return deviceStatus;
}

/* Contains the methods -open, -stop, -run, -close and -abort: */ 
#import "orchControl.m"

BOOL _MKOrchLateDeltaTMode(Orchestra *self)
{
    return (self->isTimed == MK_SOFTTIMED);
}

-setTimed:(char)isOrchTimed
  /* Controls (for all orchs) whether DSP commands are sent timed or untimed. 
     The default is timed unless the Conductor is not loaded. 
     It is permitted to change
     from timed to untimed during a performance. (But this won't work in 0.9.) */
{
    isTimed = isOrchTimed;
    return self;
}

#if 0
-step
  /* Execute current tick, increment tick counter and stop. */
{
    [_sysUG step];
}
#endif

-copy
  /* We override this method. Copying is not supported by the Orchestra class.
   */
{
    return [self doesNotRecognize:_cmd];
}

-copyFromZone:(NXZone *)zone
  /* We override this method. Copying is not supported by the Orchestra class.
   */
{
    return [self doesNotRecognize:_cmd];
}

-free
  /* Frees a particular orchestra instance. This involves freeing all
     unit generators in its unit generator stack, clearing all
     synthpatch allocation lists and releasing the DSP. It is an error
     to free an orchestra with non-idle synthPatches or allocated unit
     generators which are not members of a synthPatch. An attempt to
     do so generates an error. 
     */
{
    [self close];
    orchestras[orchIndex] = nil;
    NX_FREE(_extraVars);
    return [super free];
}

-useDSP:(BOOL)useIt
  /* Controls whether or not the output actually goes to the DSP. Has no effect
     if the Orchestra is not closed. The default is YES. 
     This method should not be used in release 0.9. */
{
    if (deviceStatus != MK_devClosed)
      return nil;
    useDSP = useIt;
    return self;
}

-(BOOL)isDSPUsed
  /* If the receiver is not closed, returns YES if the DSP is used.
     If the receiver is closed, returns YES if the receiver is set to use
     the DSP when it is opened. */
{
    return useDSP;
}

FILE *_MKGetOrchSimulator(orch)
    Orchestra *orch;
    /* Returns Simulator file pointer, if any.
       Assumes orchIndex is a valid orchestra. If this is ever made
       non-private, should check for valid orchIndex.  */
{
    return orch->_simFP;
}

static void 
  _traceMsg(simFP,typeOfInfo,fmt,ap)
FILE *simFP;
int typeOfInfo;
char * fmt;
char *ap;
/* See trace: below */
{
    if (_MKTrace() & typeOfInfo) {
        vfprintf(stderr,fmt,ap);
        fprintf(stderr,"\n");
    }
    if (simFP) {
        fprintf(simFP,"; ");
        vfprintf(simFP,fmt,ap);
        fprintf(simFP,"\n");
    }
}

- trace:(int)typeOfInfo msg:(char *)fmt, ...;
/* Arguments are like printf. Writes text, as a comment, to the
   simulator file, if any. Text may not contain new-lines. 
   If the typeOfInfo trace is set, prints info to stderr as well. */
{
    va_list ap;
    va_start(ap,fmt); 
    _traceMsg(_simFP,typeOfInfo,fmt,ap);
    va_end(ap);
    return self;
}

void _MKOrchTrace(Orchestra *orch,int typeOfInfo,char * fmt, ...)
    /* See trace: above */
{
    va_list ap;
    va_start(ap,fmt); 
    _traceMsg(orch->_simFP,typeOfInfo,fmt,ap);
    va_end(ap);
}

DSPFix48 *_MKCurSample(Orchestra *self)
    /* Returns time turned into sample time for use to DSP routines. 
       DeltaT is included in the result. */
{
    if (!orchClassInited) 
      classInit();
    /* Need to differentiate between truly untimed and 'on next tick' 
       untimed. */
    switch (self->deviceStatus) {
      case MK_devClosed:  /* A bug, probably but, play along. */
      case MK_devOpen:    /* Can only do truly untimed pokes when open. */ 
        return DSPMK_UNTIMED;
      case MK_devStopped: /* It's ok to do truly untimed pokes when stopped
			     because we're not running the orch loop */
	if (!self->isTimed)
	  return DSPMK_UNTIMED;
      default:
        break;
    }
    if (self->isTimed) {
        double curTime = MKGetDeltaTTime();
        if (_previousTime(self) != curTime) {
            _previousTime(self) = curTime;
            doubleIntToFix48UseArg((curTime + _timeOffset(self) + 
                                    self->localDeltaT) * 
                                   self->samplingRate, 
                                   &(self->_previousTimeStamp));
        }
    } else {
        self->_previousTimeStamp.high24 = 0;
        self->_previousTimeStamp.low24 = 0;
    }
    return &(self->_previousTimeStamp);
}

-(char *)segmentName:(int)whichSegment
  /* Returns name of the specified OrchMemSegment. */
{
    return (whichSegment < 0 || whichSegment >= MK_numOrchMemSegments) ?
      "invalid" : orchMemSegmentNames[whichSegment];
}

/* Instance methods for UnitGenerator's resource allocation. ------------  */


static void putLeaper(self,leapTo)
    register Orchestra *self;
    int leapTo;
    /* Adds leaper to specified place. A leaper jumps the orchestra loop
       off chip. */
{
    int leaper[LEAPERSIZE]; 
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      if (!self->isLoopOffChip)
        _MKOrchTrace(self,MK_TRACEORCHALLOC,"Moving loop off chip.");
    leaper[0] = NOOP;
    leaper[1] = JUMP;
    leaper[2] = leapTo;
    DSPSetCurrentDSP(self->orchIndex);
    DSPMKSendArraySkipTimed(_MKCurSample(self),leaper,DSP_MS_P,self->_piLoop,
                            1,LEAPERSIZE);
    self->isLoopOffChip = YES;
}

static int biggestFreeDataMemBlockSize();
static DSPAddress peekPELoop();

-(double)headroom
{
    return _headroom - HEADROOMFUDGE;
}

-beginAtomicSection
  /* Marks beginning of a section of DSP commands which are to be sent as 
     a unit. */
{
    if (_parenCount == -1)
      return self;
    if (_parenCount++ == 0) {
        if (_MK_ORCHTRACE(self,MK_TRACEDSP))
          _MKOrchTrace(self,MK_TRACEDSP,"<<< Begin orchestra atomic unit ");
        DSPMKEnableAtomicTimed(_MKCurSample(self));
    }
    return self;
}

-endAtomicSection
  /* Marks end of a section of DSP commands which are to be sent as 
     a unit. */
{
    if (_parenCount == -1)
      return self;
    if (--_parenCount == 0) {
        if (_MK_ORCHTRACE(self,MK_TRACEDSP))
          _MKOrchTrace(self,MK_TRACEDSP,"end orchestra atomic unit.>>> ");
        DSPMKDisableAtomicTimed(_MKCurSample(self));
    }
    else if (_parenCount < 0) 
      _parenCount = 0;
    return self;
}

extern BOOL _MKAdjustTimeIfNotBehind(void);

static void
  synchTime(te,dpsClientTime,self)
DPSTimedEntry te;
double dpsClientTime;
Orchestra *self;
{
    DSPFix48 dspSampleTime;
    double dspTime,hostTime;
    if (![_MKClassConductor() inPerformance])
      return;
    _MKLock();
    if ((self->_parenCount) /* Don't mess with parens */
	|| (!_MKAdjustTimeIfNotBehind())) {
      _MKUnlock();
      return;
    }
    DSPMKReadTime(&dspSampleTime);
    dspTime = DSPFix48ToDouble(&dspSampleTime)/self->samplingRate;
    hostTime = MKGetTime();
    /* one pole filter */
    _synchTimeRatio(self) = (_synchTimeRatio(self) * .8 + 
                             dspTime/hostTime * .2);
    _timeOffset(self) = (_synchTimeRatio(self) - 1) * hostTime;
    _MKUnlock();
}

static void adjustOrchTE(Orchestra *self,BOOL yesOrNo,BOOL reset) {
    if (reset) {
        _timeOffset(self) = 0;
        _synchTimeRatio(self) = 1.0;
    }
    if (yesOrNo && !_timedEntry(self) && _synchToConductor(self)) {
        _timedEntry(self) = DPSAddTimedEntry(5.0,synchTime,(void *)self, 
                                             _MK_DPSPRIORITY);
    }
    else if ((!yesOrNo || !_synchToConductor(self)) && _timedEntry(self)) {
        DPSRemoveTimedEntry(_timedEntry(self));
        _timedEntry(self) = NULL;
    }
}

-setSynchToConductor:(BOOL)yesOrNo
{
    _synchToConductor(self) = yesOrNo;
    if (deviceStatus == MK_devRunning)
      adjustOrchTE(self,yesOrNo,YES);
    return self;
}

-(double)localDeltaT
{
    return localDeltaT;
}

-setLocalDeltaT:(double)value
{
    localDeltaT = value;
    return self;
}

-setHeadroom:(double)headroom
  /* Sets DSP computational headroom. (This only has an effect when you are
     generating sound in real time.) This adjusts the tradeoff between
     maximizing the processing power of the Orchestra on the one hand and
     running a risk of falling out of real time on the other.
     A value of 0 runs a large risk of falling out of real time.
     A value of .5 runs a small risk of falling out of real time but 
     allows for a substantially less powerful Orchestra. 
     Since the UnitGenerator computation time estimates are conservative,
     negative headroom values may sometimes work. The default is .1. 
     
     The effective sampling period is computed as 
     
     sampling period * (1 - headroom).
     
     */
{
    /* We're overly conservative in our UG timings so we fudge here. */
    if (headroom > .99)
      headroom = .99;
    _headroom = headroom + HEADROOMFUDGE;
    _effectiveSamplePeriod = (1.0/samplingRate) * (1 - _headroom);
    return self;
}     

-(MKOrchMemStruct *)peekMemoryResources:(MKOrchMemStruct *)peek
  /* Return the available resources in peek. Note that what is returned 
     is the maximum available of each kind, assuming the memories
     do not overlay. However, XDATA, YDATA
     and PSUBR compete for the same memory. The caller should interpret 
     what is returned with appropriate
     caution. peek must point to a valid MKOrchMemStruct. Returns peek.  */
{
    peek->pLoop = (isLoopOffChip) ? peekPELoop(self) : MAXPILOOP - _piLoop;
    peek->xArg = MAXXARG - _xArg;
    peek->yArg = MAXYARG - _yArg;
    peek->lArg = MAXLARG - _lArg;
    peek->xData = biggestFreeDataMemBlockSize(self,MK_xData);
    peek->pSubr = peek->xData;
    peek->yData = peek->xData;
    return peek;
}

-(unsigned short)index
  /*  Returns the index of the DSP on which this instance is running. */
{
    return orchIndex;
}

-(double)computeTime
  /* Returns the compute time currently used by the orchestra system in 
     seconds per sample. */
{
    return computeTime;
}

id _MKFreeMem(Orchestra *self,MKOrchAddrStruct *mem)
    /* Frees MK_yData, MK_xData, MK_pSubr,
       MK_xSig or MK_ySig memory. */
{
    switch (mem->memSegment) {
      case MK_yData: 
      case MK_xData:
      case MK_pSubr:
        giveDataMem(self,mem->memSegment,mem->address);
        break;
      case MK_xPatch:
      case MK_yPatch:
        givePatchMem(self,mem->memSegment,mem->address);
        break;
    }
    return self;
}

/* Instance methods for SynthPatch alloc/dealloc.------------------------  */

-allocSynthPatch:aSynthPatchClass
  /* Same as allocSynthPatch:patchTemplate: but uses default template. 
     The default
     template is obtained by sending [aSynthPatchClass defaultPatchTemplate].*/
{
    return [self allocSynthPatch:aSynthPatchClass patchTemplate:
            [aSynthPatchClass defaultPatchTemplate]];
}

#define CHECKADJUSTTIME() if (self->isTimed == MK_SOFTTIMED) _MKAdjustTimeIfNecessary()

-allocSynthPatch:aSynthPatchClass patchTemplate:p
  /* Reuse a SynthPatch if possible. Otherwise, build a new one, if 
     possible. If successful, return the new SynthPatch. Otherwise,
     return nil. Note that the ordered collection of objects in the 
     SynthPatch is in the same order as specified in the template. */
{
    id rtnVal;
    if ((!p) || (deviceStatus == MK_devClosed))
      return nil;
    CHECKADJUSTTIME();
    if ((_previousLosingTemplate == p) || /* If we just lost, don't even try */
        (!(rtnVal = _MKAllocSynthPatch(p,aSynthPatchClass,self,orchIndex)))){
        if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
          _MKOrchTrace(self,MK_TRACEORCHALLOC,
                       "allocSynthPatch can't allocate %s",
                       [aSynthPatchClass name]);
        _previousLosingTemplate = p;
        return nil;
    }
    return rtnVal;
}

/* Instance methods for Unit generator alloc/dealloc. ------------------- */

-allocUnitGenerator:  factObj 
  /* Allocate unit generator of the specified class. */
{
    id rtnVal;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator looking for a %s.",[factObj name]);
    rtnVal = allocUG(self,factObj,nil,nil);
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      if (rtnVal)
        _MKOrchTrace(self,MK_TRACEORCHALLOC,
                     "allocUnitGenerator returns %s_%d",[rtnVal name],rtnVal);
    return rtnVal;
}

-allocUnitGenerator:  factObj before:aUnitGeneratorInstance
  /* Allocate unit generator of the specified class before the specified 
     instance. */
{
    id rtnVal;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator looking for a %s before %s_%d", 
                   [factObj name],[aUnitGeneratorInstance name],
                   aUnitGeneratorInstance);
    rtnVal = allocUG(self,factObj,aUnitGeneratorInstance,nil);
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator returns %s_%d", [rtnVal name],rtnVal);
    return rtnVal;
}

-allocUnitGenerator:  factObj after:aUnitGeneratorInstance
  /* Allocate unit generator of the specified class
     after the specified instance. */
{
    id rtnVal;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator looking for a %s after %s_%d", 
                   [factObj name],[aUnitGeneratorInstance name],
                   aUnitGeneratorInstance);
    rtnVal = allocUG(self,factObj,nil,aUnitGeneratorInstance);
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator returns %s_%d",[rtnVal name],rtnVal);
    return rtnVal;
}

-allocUnitGenerator:factObj 
 between:aUnitGeneratorInstance : anotherUnitGeneratorInstance
  /* Allocate unit generator of the specified class between the 
     specified instances. */
{
    id rtnVal;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocUnitGenerator looking for a %s after %s_%d and before %s_%d", 
                   [factObj name],[aUnitGeneratorInstance name],
                   aUnitGeneratorInstance,
                   [anotherUnitGeneratorInstance name],
                   anotherUnitGeneratorInstance);
    rtnVal = allocUG(self,factObj,anotherUnitGeneratorInstance,
                     aUnitGeneratorInstance);
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      if (rtnVal)
        _MKOrchTrace(self,MK_TRACEORCHALLOC,
                     "allocUnitGenerator returns %s_%d",
                     [rtnVal name],rtnVal);
    return rtnVal;
}

/* Instance methods for memory alloc. ---------------------- */

static DSPAddress allocMem(Orchestra *self,MKOrchMemSegment segment,unsigned size);

-allocSynthData:(MKOrchMemSegment)segment length:(unsigned)size
  /* Returns a new SynthData object with the specified length or nil if 
     there's no more memory or if size is 0. This method can be used
     to allocate patch points but the size must be DSPMK_NTICK. */
{
    DSPAddress baseAddr;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocSynthData: looking in segment %s for size %d.",
                   orchMemSegmentNames[segment],size);
    baseAddr = allocMem(self,segment,size);
    if (baseAddr == BADADDR)
      return nil;
    else if (baseAddr == NOMEMORY) {
        if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
          _MKOrchTrace(self,MK_TRACEORCHALLOC,
                       "Allocation failure: No more offchip data memory.");
        return nil;
    }
    else if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   "allocSynthData returns %s %d of length %d.",
                   orchMemSegmentNames[segment],baseAddr,size);
    return [SynthData _newInOrch:self index:orchIndex
          length:size segment:segment baseAddr:baseAddr];
}

-allocPatchpoint:(MKOrchMemSegment)segment 
  /* returns a new patch point object. Segment must be xPatch or yPatch.
     Returns nil if an illegal segment is requested. 
     */
{
    if ((segment != MK_xPatch) && (segment != MK_yPatch)) {
        return nil;
    }
    return [self allocSynthData:segment length:DSPMK_NTICK];
}

/* Garbage collection. -------------------------------------------------- */

static void adjustResources(self,time,reloc)
    register Orchestra *self;
    double time;
    MKOrchMemStruct *reloc;
    /* Pops off stack the UG with given time and relocation. */ 
{
    self->computeTime -= time;
    if (self->isLoopOffChip) 
      self->isLoopOffChip = givePELoop(self,reloc->pLoop - noops);
    else self->_piLoop = reloc->pLoop - noops;
    self->_xArg = reloc->xArg;
    self->_lArg = reloc->lArg;
    self->_yArg = reloc->yArg;
    giveDataMem(self,MK_yData,reloc->yData);
    giveDataMem(self,MK_xData,reloc->xData);
    giveDataMem(self,MK_pSubr,reloc->pSubr);
}

static double getUGComputeTime(Orchestra *self,int pReloc,MKLeafUGStruct *p)
{
    return (((p->reserved1 != MK_2COMPUTETIMES)  /* version 1.0 unit gen */
             || (pReloc < MINPELOOP)) ?          /* it's on-chip */
            p->computeTime
            : p->offChipComputeTime);
}

static void abortAlloc(self,factObj,reloc)
    register Orchestra *self;
    id factObj;
    MKOrchMemStruct *reloc;
    /* Give up */
{
    adjustResources(self,getUGComputeTime(self,reloc->pLoop,
					  [factObj classInfo]),
                    reloc);
    setLooper(self);
}

static void adjustUGInSP(sp,aUG)
    id sp,aUG;
    /* This is what we do when freeing a UnitGenerator that's in a 
       SynthPatch */
{
    [sp _remove:aUG];                     /* Put a hole in the sp */
    _MKDeallocSynthElement(aUG,NO);
    /* Dealloc (if needed) but don't idle, since it's going 
       to be freed anyway. We know it's unshared because we know it's 
       freeable (and hence deallocated.) */
}

void _MKOrchResetPreviousLosingTemplate(Orchestra *self)
    /* After we dealloc or free something, we may win on next template
       so we have to reset _previousLosingTemplate. */
{
    self->_previousLosingTemplate = nil;
}

static void freeUG(self,aUG,aSP)
    register Orchestra *self;
    id aUG;
    id aSP;
    /* Free a UnitGenerator */
{
    double time;
    MKLeafUGStruct *classInfo;
    MKOrchMemStruct *reloc;
    if (aSP)
      adjustUGInSP(aSP,aUG);
    reloc = [aUG _getRelocAndClassInfo:&classInfo];
    time = getUGComputeTime(self,reloc->pLoop,classInfo);
    adjustResources(self,time,reloc);
    self->_previousLosingTemplate = nil;
    [aUG _free];
}

static BOOL popResoAndSetLooper(Orchestra *self)
    /* Pops the UnitGenerator stack and resets the looper. See popReso */
{
    BOOL resetLooper = popReso(self);
    if (resetLooper)
      setLooper(self);
    return resetLooper;
}

static BOOL popReso(self)
    Orchestra *self;
    /* Frees up resources on top of DSP memory stack. This is ordinarily invoked
       automatically by the orchestra instance. Returns YES if something is
       freed.
       */
{
    register id aUG;
    BOOL resetLooper = NO;
    id spHead = nil,spTail = nil,sp;
    while (aUG = [self->stack lastObject]) {
        if (![aUG isFreeable])
          break;
        [self->stack removeLastObject];
        resetLooper = YES;
        sp = [aUG synthPatch];
        if (sp)
          [sp _prepareToFree:&spHead :&spTail];
        if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
          _MKOrchTrace(self,MK_TRACEORCHALLOC,garbageMsg,[aUG name],
                       aUG);
        freeUG(self,aUG,sp);
    }
    [spTail _freeList:spHead];
    return resetLooper;
}

#if COMPACTION 

static void freeUG2(self,aUG,aSP)
    register Orchestra *self;
    id aUG;
    id aSP;
    /* Used when freeing unit generators during compaction. Like 
       freeUG but doesn't change ploop nor argument memory. The
       point here is that we don't need to set these because
       we will just clobber the values later. */
{
    double time;
    MKLeafUGStruct *classInfo;
    MKOrchMemStruct *reloc;
    if (aSP)
      adjustUGInSP(aSP,aUG);
    reloc = [aUG _getRelocAndClassInfo:&classInfo];
    time = getUGComputeTime(self,reloc->pLoop,classInfo);
    self->computeTime -= time;
    giveDataMem(self,MK_yData,reloc->yData);
    giveDataMem(self,MK_xData,reloc->xData);
    giveDataMem(self,MK_pSubr,reloc->pSubr);
    self->_previousLosingTemplate = nil;
    [aUG _free];
}

static void bltArgs(Orchestra *self,MKOrchMemStruct *argBLTFrom,
                    MKOrchMemStruct *argBLTTo,MKOrchMemStruct *reso,
                    id **ugList,id *endOfUGList)
    /* Moves unit generator arguments during compaction. 
       Note that endOfUGList is one past the last UG we'll blt */
{
    DSPFix48 *ts = _MKCurSample(self);
    register id *el;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC)) 
      _MKOrchTrace(self,MK_TRACEORCHALLOC,"Copying arguments.");
    if (reso->xArg && (argBLTFrom->xArg != argBLTTo->xArg))
      DSPMKBLTTimed(ts,DSP_MS_X,argBLTFrom->xArg,
                    argBLTTo->xArg,reso->xArg);
    if (reso->yArg && (argBLTFrom->yArg != argBLTTo->yArg))
      DSPMKBLTTimed(ts,DSP_MS_Y,argBLTFrom->yArg,
                    argBLTTo->yArg,reso->yArg);
    if (reso->lArg && (argBLTFrom->lArg != argBLTTo->lArg)) {
        /* Can't BLT for L space. Need to do x/y separately. */
        DSPMKBLTTimed(ts,DSP_MS_X,argBLTFrom->lArg,argBLTTo->lArg,
                      reso->lArg);
        
        /* JOS/89jul28 */
        DSPMKBLTTimed(ts,DSP_MS_Y,argBLTFrom->lArg,argBLTTo->lArg,
                      reso->lArg);
    }
    for (el = *ugList; (el < endOfUGList); el++) {
        /* Inform UnitGenerator and its patch of the relocation change. */
        [*el moved];             
        [[*el synthPatch] moved:*el];
    }
    *ugList = NULL;
}

static void bltLoop(Orchestra *self,MKOrchMemStruct *loopBLTFrom,
                    MKOrchMemStruct *loopBLTTo,MKOrchMemStruct *reso,
                    id **ugList,id *endOfUGList)
    /* Moves unit generator code during compaction. 
       Note that endOfUGList is one past the last UG we'll blt */
{
    DSPFix48 *ts = _MKCurSample(self);
    register id *el;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC)) 
      _MKOrchTrace(self,MK_TRACEORCHALLOC,"Copying p memory.");
    if (reso->pLoop && (loopBLTFrom->pLoop != loopBLTTo->pLoop))
      DSPMKBLTTimed(ts,DSP_MS_P,loopBLTFrom->pLoop,
                    loopBLTTo->pLoop,reso->pLoop);
    el = *ugList; 
    while (el < endOfUGList)  /* Do fixups */
      _MKFixupUG(*el++,ts);
    *ugList = NULL;
}

static BOOL compactResourceStack(Orchestra *self)
    /* Frees up idle resources in entire DSP memory stack. 
       This is ordinarily invoked automatically by the orchestra instance.
       Returns YES if compaction was accomplished.
       */
{
    /* We start from the bottom of the stack (i.e. nearest the DSP system)
       and work our way up. First we skip over all running UGs, since
       these can't be compacted -- they are already at the bottom of the
       stack. The first time we find a ug that is freeable, we peel back
       the stack as if that ug were the top of stack using freeUG. 
       We also flush p-memory allocation at this point. The idea is that 
       the moved ugs are actually 'reallocated', i.e. we deallocate
       and then allocate again, as if this were a new ug. 
       So it is as if we have a new stack that 
       is growing over the old stack. Thus, for each 
       subsequent freeable ugs, we need not reset the stack, since we
       have already done so. Instead, we merely free any off-chip
       memory that ug has. This is done by the C-function freeUG2.
       The actual moving of the DSP code and argument values is done
       by the UnitGenerator function _MKMoveUGCodeAndArgs(). */
    
    int i;
    unsigned n = [self->stack count];
    id *el = NX_ADDRESS(self->stack); 
    /* check first here to avoid copying List */
    for (i = 0; i < n; i++) 
      if ([*el++ isFreeable])          /* Can we flush this UG? */
        break;
    if (i == n)                      
      return NO;
    {   /* We've got a freeable one. Here we go... */
        id sp,aUG;
        BOOL UGIsOffChip,UGWasOffChip = NO;
        id *pendingArgBLT = NULL; 
        id *pendingLoopBLT = NULL;
        MKOrchMemStruct *ugReso,resoToBLT,newReloc,fromBLT,toBLT, *oldReloc;
        int pLoopNeeds;
        register id aList = _MKCopyList(self->stack); /* Local copy */
        id spHead = nil;
        id spTail = nil;
        [self beginAtomicSection];
        el = &(NX_ADDRESS(aList)[i]);    /* Point into copied list. */
        aUG = *el++;                     /* el is now ready for loop below. */
        [self->stack removeObject:aUG];  /* First we free up aUG. */
        if (sp = [aUG synthPatch])
          [sp _prepareToFree:&spHead :&spTail]; /* won't add twice */
        DSPSetCurrentDSP(self->orchIndex);
        if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC)) {
            _MKOrchTrace(self,MK_TRACEORCHALLOC,
                         "Compacting stack.");
            _MKOrchTrace(self,MK_TRACEORCHALLOC,garbageMsg,[aUG name],aUG);
        }
        /* freeUG sets stacks to what they would be if this were actually
           the top of the stack. */
        UGIsOffChip = (([aUG relocation]->pLoop) >= MINPELOOP);
        if ((!UGIsOffChip) && self->isLoopOffChip) /* Clear offchip loop */
          self->isLoopOffChip = givePELoop(self,MINPELOOP); 
        freeUG(self,aUG,sp);    
        for (i++; i < n; i++) {            /* Starting with the next one... */
            aUG = *el;
            if ([aUG isFreeable]) {         /* Can we flush this UG? */
                if (pendingLoopBLT) 
                  bltLoop(self,&fromBLT,&toBLT,&resoToBLT,&pendingLoopBLT,el);
                if (pendingArgBLT) 
                  bltArgs(self,&fromBLT,&toBLT,&resoToBLT,&pendingArgBLT,el);
                [self->stack removeObject:aUG]; /* Same routine as above.*/ 
                if (sp = [aUG synthPatch])  
                  [sp _prepareToFree:&spHead :&spTail]; 
                if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
                  _MKOrchTrace(self,MK_TRACEORCHALLOC,garbageMsg,[aUG name],
                               aUG);
                freeUG2(self,aUG,sp);      /* But now just free offchip mem */ 
                el++;
            }
            else {                         /* A running UG must be moved */
                ugReso = [aUG resources];
                oldReloc = [aUG relocation];
                pLoopNeeds = ugReso->pLoop + noops;
                /* The following seems confusing because there are several
                   things to keep in mind:
                   * Is the unit generator on chip or off chip before it's
                   moved?
                   * Is the unit generator on chip or off chip after it's
                   moved?
                   * Has the (new) loop spilled off chip yet? 
                   */
                if (self->isLoopOffChip) {  /* We're already off chip */
                    newReloc.pLoop = noops + getPELoop(self,pLoopNeeds);
                    UGIsOffChip = YES;
                }
                else {                      /* We're not yet off chip */
                    /* Can we fit the ug on chip? */
                    if ((self->_piLoop + pLoopNeeds) <= MAXPILOOP) {
                        /* Yes. The UG can be moved on chip */
                        UGIsOffChip = NO; /* We increment piLoop later */
                        newReloc.pLoop = noops + self->_piLoop;
                        /* Are we moving from off chip to on chip? */
                        if (oldReloc->pLoop >= MINPELOOP) {
                            MKLeafUGStruct *p = [aUG classInfo];
                            /* Need to correct compute time. Also need
                               to split up the blt. */
                            self->computeTime -= 
                              (getUGComputeTime(self,oldReloc->pLoop,p) -
                               getUGComputeTime(self,newReloc.pLoop,p));
                            if (pendingLoopBLT && !UGWasOffChip)
                              /* If previous UG was not off chip, we need
                                 to flush BLT here, since BLT can't straddle
                                 chip. */
                              bltLoop(self,&fromBLT,&toBLT,&resoToBLT,
                                      &pendingLoopBLT,el);
                            UGWasOffChip = YES;
                        } else UGWasOffChip = NO;
                    }
                    else { /* We're moving off chip now */
                        /* Add a leaper so orchestra system can straddle
                           on-chip/off-chip boundary. */
                        UGIsOffChip = YES;
                        if (pendingLoopBLT) 
                          bltLoop(self,&fromBLT,&toBLT,&resoToBLT,
                                  &pendingLoopBLT,el);
                        newReloc.pLoop = noops + getPELoop(self,pLoopNeeds);
                        putLeaper(self,newReloc.pLoop);
                    }
                }
                if (!pendingArgBLT) {
                    toBLT.xArg = self->_xArg;
                    toBLT.yArg = self->_yArg;
                    toBLT.lArg = self->_lArg;
                    fromBLT.xArg = oldReloc->xArg;
                    fromBLT.yArg = oldReloc->yArg;
                    fromBLT.lArg = oldReloc->lArg;
                    resoToBLT.xArg = ugReso->xArg; 
                    resoToBLT.yArg = ugReso->yArg;
                    resoToBLT.lArg = ugReso->lArg;
                    pendingArgBLT = el;
                } else {
                    resoToBLT.xArg += ugReso->xArg; 
                    resoToBLT.yArg += ugReso->yArg;
                    resoToBLT.lArg += ugReso->lArg;
                }
                newReloc.xArg = self->_xArg; /* First grab values */
                newReloc.yArg = self->_yArg;
                newReloc.lArg = self->_lArg;
                self->_xArg += ugReso->xArg; /* Now adjust for next reso. */
                self->_yArg += ugReso->yArg;
                self->_lArg += ugReso->lArg;
                if (!pendingLoopBLT) {       
                    toBLT.pLoop = newReloc.pLoop - noops;
                    fromBLT.pLoop = oldReloc->pLoop - noops;
                    resoToBLT.pLoop = ugReso->pLoop + noops; 
                    pendingLoopBLT = el;
                } else resoToBLT.pLoop += (ugReso->pLoop + noops); 
                if (!UGIsOffChip)            
                  self->_piLoop += pLoopNeeds;  /* Adjust the next reso */
                _MKRerelocUG(aUG,&newReloc);
                el++;
            }   /* End of running UG block. */
        }
        if (pendingLoopBLT) 
          bltLoop(self,&fromBLT,&toBLT,&resoToBLT,&pendingLoopBLT,el);
        if (pendingArgBLT) 
          bltArgs(self,&fromBLT,&toBLT,&resoToBLT,&pendingArgBLT,el); 
        [aList free];                        /* Free local copy */
        [spTail _freeList:spHead];           /* Free synthpatches */
        setLooper(self);                     
        [self endAtomicSection];
        
        /* Top of list might have been  freed so we need to set looper
           explicitly. Actually, if we know for sure that the stack has been
           popped, this is not necessary, we could just blt p one word more. */
        
        return YES; 
    } 
}
#else

static BOOL compactResourceStack(Orchestra *self)
    /* Dummy routine for when compaction is disabled */
{
    return NO;
}

#endif

/* Static functions for external memory allocation. -------------------- */

static int biggestFreeDataMemBlockSize(self,segment)
    Orchestra *self;
    MKOrchMemSegment segment; /* We need this argument in non-overlayed case */
{
    /* Returns the size of the biggest free block in the specified segment. */
    int maxSize = 0;
    register dataMemBlockStruct *tmp = self->_availDataMem;
    while (tmp) {
        if (!(tmp->isAllocated))
          maxSize = MAX(tmp->size,maxSize);
        tmp = tmp->prev;
    }
    return maxSize;
}

static void giveDataMem(self,segment,addr)
    Orchestra *self;
    MKOrchMemSegment segment; /* Needed in non-overlayed case. */
    int addr;
{
    /* These are the primitives for managing external memory. Since
       pe memory must be kept contiguous, we allocate pe memory from
       the bottom of the memory segment and keep it contiguous. To do
       this, we keep the pe memory allocated from the standpoint of this
       allocator, even if the unit generator is deallocated. 
       Memory lists are sorted by address with the tail of the list 
       being the highest address. In the structs themselves, the
       address is the base (i.e. lower) address. */
    register dataMemBlockStruct *tmp;
    register dataMemBlockStruct *theBlock;
    if ((addr == BADADDR) || (addr == NOMEMORY))
      return;
    for (tmp = self->_availDataMem; (tmp->prev && (tmp->baseAddr > addr)); 
         tmp = tmp->prev)
      ;
    if ((!(tmp->prev)) ||              /* The first one's the PELoop memory */
        (tmp->baseAddr != addr) ||     /* Wrong address. */
        (!tmp->isAllocated))           /* Unclaimed */
      return;                          /* Caller goofed somehow */
    
    theBlock = tmp;                    /* This is the one to free. */
    theBlock->isAllocated = NO;        /* Unmark it. */
    if (!theBlock->prev->isAllocated) {/* Combine with lower addressed block */
        tmp = theBlock->prev;          
        theBlock->baseAddr = tmp->baseAddr;
        theBlock->size = tmp->size + theBlock->size;
        theBlock->prev = tmp->prev;
        theBlock->prev->next = theBlock;
        /* This is safe 'cause end of list is always allocated */
        freeDataMemBlock(tmp);
    }
    tmp = theBlock->next;
    if (tmp && (!(tmp->isAllocated))) {/* Combine with upper addressed block */
        theBlock->size = theBlock->size + tmp->size;
        theBlock->next = tmp->next;
        if (theBlock->next)            /* Not tail-of-list */ 
          theBlock->next->prev = theBlock;
        else 
          self->_availDataMem = theBlock;/* New tail of list. */
        freeDataMemBlock(tmp);
    }
}

static void
  insertFreeMemStruct(base,size,prev,next)
DSPAddress base;
DSPAddress size;
dataMemBlockStruct *next,*prev;
{
    /* Returns and inits new dataMemBlockStruct between prev and next. */
    register dataMemBlockStruct *newNode;
    newNode = allocDataMemBlock();
    newNode->baseAddr = base;
    newNode->isAllocated = NO;
    newNode->size = size;
    newNode->next = next;
    newNode->prev = prev;
    next->prev = newNode;
    prev->next = newNode;
}    

static DSPAddress getDataMem(self,segment,size)
    Orchestra *self;
    MKOrchMemSegment segment; /* Needed in non-overlayed case. */
    int size;
{
    /* Memory lists are sorted by address with the tail of the list 
       being the highest address. In the structs themselves, the
       address is the base (i.e. lower) address. */
    register dataMemBlockStruct *tmp = self->_availDataMem;
    if (size <= 0)
      return BADADDR;
    while (tmp && (tmp->isAllocated || (tmp->size < size)))
      tmp = tmp->prev;
    if (!tmp)
      return NOMEMORY;
    tmp->isAllocated = YES;
    if (tmp->size > size) {
        insertFreeMemStruct(tmp->baseAddr,tmp->size - size,tmp->prev,tmp);
        tmp->baseAddr += (tmp->size - size);
        tmp->size = size;
    }
    return tmp->baseAddr;
}    

static void givePatchMem(self,segment,addr)
    Orchestra *self;
    MKOrchMemSegment segment;
    DSPAddress addr;
{
    /* Give Patchpoint. Forwards call to giveDataMem if necesary. */
    DSPAddress *sigs;
    register int i;
    if (segment == MK_xPatch)
      sigs = &(self->_xPatch[0]);
    else sigs = &(self->_yPatch[0]);
    if ((addr == NOMEMORY) || (addr == BADADDR))
      return;
    if (addr > ((segment == MK_xPatch) ? MAXXPATCH : MAXYPATCH)) {
        giveDataMem(self,(segment == MK_xPatch) ? MK_xData : MK_yData,addr);
        return;
    }
    for (i = 0; i < self->onChipPatchPoints; i++)
      if (*sigs++ == addr) {
          if (segment == MK_xPatch)
            self->_xPatchAllocBits &= (~(1 << i));
          else self->_yPatchAllocBits &= (~(1 << i));
          return;
      }
}

static DSPAddress getPatchMem(self,segment)
    Orchestra *self;
    MKOrchMemSegment segment;
{
    /* Get Patchpoint. Forwards call to giveDataMem if necesary. */
#   define SIGS ((segment == MK_xPatch) ? self->_xPatch : self->_yPatch)
    register i;
    register unsigned long bVect = (segment == MK_xPatch) ? self->_xPatchAllocBits 
      : self->_yPatchAllocBits;
    for (i = 0; i < self->onChipPatchPoints; i++)
      if ((bVect & (1 << i)) == 0) {              /* If bit is 0, it's free */
          if (segment == MK_xPatch)               /* Set bit */
            self->_xPatchAllocBits |= (1 << i); 
          else self->_yPatchAllocBits |= (1 << i); 
          return SIGS[i];                         /* Return address */
      }
    return getDataMem(self,(segment == MK_xPatch) ? MK_xData : MK_yData,
                      DSPMK_NTICK);                /* Off chip Patchpoint */
}

static DSPAddress getPELoop(self,size)
    Orchestra *self;
    int size;
{
    /* Adjust boundary between PELOOP and PSUBR/XDATA/YDATA memory by adding 
       size. */
    register dataMemBlockStruct *peLoop = self->_peLoop;
    register dataMemBlockStruct *availData = peLoop->next;
    /* We do all calculations as if UG really started at newUGLoc. Then we
       subtract 1 at the end. That is, the new UG is really put at newUGLoc-1
       and the looper is after it. In other words, all UGs are shifted down
       by 1 (due to the return value). Since we allocate 1 at the start, this
       works out ok. */
    /* Base of new block, before compensating for looper. */
    int newUGLoc = availData->baseAddr;
    /* Base of next of availData (if any): */
    int nextAvailData = newUGLoc + availData->size;   
    /* Base of availData after allocation: */
    int newAvailData = newUGLoc + size;  
    if (size <= 0)
      return BADADDR;
    if ((availData->isAllocated) ||                /* Can't use it. */
        (nextAvailData <= newAvailData))           /* Not enough free memory */
      return NOMEMORY;
    availData->size = nextAvailData - newAvailData;/* Shrink availData block.*/
    availData->baseAddr = newAvailData;            /* Update its base addr */
    peLoop->size += size;                     /* Update peLoop size. This is
                                                 the true size (including
                                                 looper). */
    return newUGLoc - LOOPERSIZE; 
    /* Returns location of new unit generator. This is the true base. The
       looper will go after this. We have allocated a block of the specified
       size with LOOPERSIZE words of free storage above the block. The extra
       space is then reclaimed next time and a new word is allocated. 
       Get it? */
}

static DSPAddress peekPELoop(self)
    Orchestra *self;
{
    /* Returns size of available PELOOP stack. */
    dataMemBlockStruct *tmp = ((dataMemBlockStruct *)self->_peLoop)->next;
    if (tmp->isAllocated) 
      return 0;
    else return tmp->size;
}

static BOOL givePELoop(self,freedPEAddr)
    Orchestra *self;
    int freedPEAddr;
{
    /* Adjust boundary between PE and XDATA/YDATA memory by subtracting 
       size. freedPEAddr is the relocation of the peLoop being returned 
       (including the 3 preceeding noops). 
       Returns YES if there is still a non-zero PELoop after
       the stack pop. */
    register dataMemBlockStruct *peLoop = self->_peLoop;
    register dataMemBlockStruct *availData = peLoop->next;
    freedPEAddr += LOOPERSIZE;
    /* We add LOOPERSIZE here because we always want to leave space at 
       the end of peLoop for the LOOPER. All of the calculations below
       work out correctly. E.g. peLoop->size will always be at least 
       LOOPERSIZE. */
    peLoop->size = freedPEAddr - peLoop->baseAddr;
    if (availData->isAllocated)   /* Put a new block in for freed segment */
      insertFreeMemStruct(freedPEAddr,availData->baseAddr - freedPEAddr,peLoop,
                          availData);
    else {                        /* Adjust free list. */
        availData->size += (availData->baseAddr - freedPEAddr);
        availData->baseAddr = freedPEAddr;
    }
    return (peLoop->size > LOOPERSIZE);
}

static void setLooper(self)
    Orchestra *self;
{
    /* Returns address where looper is (or should be). Assumes looper
       is inited. */
    unsigned int looper[2];
    DSPAddress addr = (self->isLoopOffChip) ?
      ((dataMemBlockStruct *)self->_peLoop)->next->baseAddr - LOOPERSIZE : 
        self->_piLoop;
    if (_MK_ORCHTRACE(self,MK_TRACEDSP))
      _MKOrchTrace(self,MK_TRACEDSP,"Adding looper.");
    DSPSetCurrentDSP(self->orchIndex);
    looper[0] = NOOP; /* Jos says you need a noop before the looper to 
                         insure that there's no problem if final word of 
                         unit generator is a jump. */
    looper[1] = self->_looper;
    DSPMKSendArraySkipTimed(_MKCurSample(self),(DSPFix24 *)&looper[0],
                            DSP_MS_P,addr,1,LOOPERSIZE);
}


static DSPAddress allocMemAux(self,segment,size)
    register Orchestra *self;
    MKOrchMemSegment segment;
    int size;
{
    /* Memory alloc auxiliary routine. */
    switch (segment) {
      case MK_pSubr:
      case MK_xData:
      case MK_yData:
        return getDataMem(self,segment,size);
      case MK_xPatch:
      case MK_yPatch:
        if (size != DSPMK_NTICK) 
          return BADADDR;
        return getPatchMem(self,segment);
      case MK_pLoop:
        return BADADDR;
      default: 
        return BADADDR;
    }
}

static DSPAddress allocMem(Orchestra *self,MKOrchMemSegment segment,unsigned size)
{
    /* Allocate off-chip memory of specified size in indicated segment 
       and returns address in specified segment. */
    id *templPtr;
    id aPatch,deallocatedPatches;
    DSPAddress rtnVal;     
    unsigned i;
    CHECKADJUSTTIME();
    if (self->deviceStatus == MK_devClosed)
      return NOMEMORY;
    if (size == 0)
      return BADADDR;
    if ((rtnVal = allocMemAux(self,segment,size)) != NOMEMORY)
      return rtnVal;
    /* Now look if there's some garbage to collect in shared table. */
    if (_MKCollectSharedDataGarbage(self,_sharedGarbage(self)))
      if ((rtnVal = allocMemAux(self,segment,size)) != NOMEMORY)
        return rtnVal;
    /* Now look if there's some free memory in a deallocated syntpatch */
    i = nTemplates;
    templPtr = patchTemplates;
    while (i--) {
        deallocatedPatches = _MKDeallocatedSynthPatches(*templPtr,self->orchIndex);
        if (aPatch = [deallocatedPatches lastObject]) /* peek */
          if ([aPatch _usesEMem:segment])
            /* See comment in SynthPatch.m */
            while (aPatch = [deallocatedPatches removeLastObject]) {
                [aPatch _free];            /* Deallocate some UGs. */
                if (popResoAndSetLooper(self)) /* Free some UGs. */
                  if ((rtnVal = allocMemAux(self,segment,size)) 
                      != NOMEMORY)
                    return rtnVal;
            }
        templPtr++;  /* Try next template. */
    }
    rtnVal = allocMemAux(self,segment,size);
    if (rtnVal == NOMEMORY) {
        if (compactResourceStack(self))
          rtnVal = allocMemAux(self,segment,size);
    }
    return rtnVal;
}

/* Functions for unit generator allocation. ---------------------- */

#define CONSERVATIVE_UG_TIMING_COMPENSATION (DSP_CLOCK_PERIOD * 3)

static int resoAlloc(self,factObj,reloc)
    register Orchestra *self;
    id factObj;
    MKOrchMemStruct *reloc;
    /* Returns, in reloc, the relocation info matching the reso request. 
       Returns 0 (OK) if successful, TIMEERROR if unsuccessful because 
       of a lack of available runtime,
       else an MKOrchMemSegment indicating what was in short supply.
       If a non-0 value is returned, the
       reloc contents is not valid and should be ignored. This is 
       used by the UnitGenerator class to allocate space for the new
       instance. */
{
    BOOL leapOffChip = NO;
    double time;
    int pLoopNeeds;
    int rtnVal;
    register MKOrchMemStruct *reso;
    MKLeafUGStruct *classInfo = [factObj classInfo];
    reso = &classInfo->reso;
    time = getUGComputeTime(self,
			    ((self->isLoopOffChip) ? 
                             MAXPELOOP : /* any  old off-chip address */
                             (reso->pLoop + self->_piLoop)), /* figure it */
                            classInfo);
    if ((reloc->xData = allocMem(self,MK_xData,reso->xData)) == 
        NOMEMORY) 
      return (int)MK_xData;
    if ((reloc->yData = allocMem(self,MK_yData,reso->yData)) == 
        NOMEMORY) {
        giveDataMem(self,MK_xData,reloc->xData);
        return (int)MK_yData;
    }
    if ((reloc->pSubr = allocMem(self,MK_pSubr,reso->pSubr)) == 
        NOMEMORY) {
        giveDataMem(self,MK_xData,reloc->xData);
        giveDataMem(self,MK_yData,reloc->yData);
        return (int)MK_pSubr;
    }
    reloc->xArg = self->_xArg;
    reloc->yArg = self->_yArg;
    reloc->lArg = self->_lArg;
    pLoopNeeds = reso->pLoop + noops;
    if (self->isLoopOffChip) 
      reloc->pLoop = noops + getPELoop(self,pLoopNeeds);
    else {  
        reloc->pLoop = noops + self->_piLoop;
        if ((self->_piLoop + pLoopNeeds) <= MAXPILOOP) 
          self->_piLoop += pLoopNeeds;
        else {
            reloc->pLoop = noops + getPELoop(self,pLoopNeeds);
            leapOffChip = YES;
            /* Add a leaper so orchestra system can straddle on-chip/off-chip 
               boundary. */
        }
    }
    rtnVal = ((reloc->pLoop == NOMEMORY) ? (int)MK_pLoop : 
              (self->soundOut && 
               ((self->computeTime + time >= self->_effectiveSamplePeriod))) ? 
              TIMEERROR :
              ((self->_xArg += reso->xArg) >= MAXXARG) ? (int)MK_xArg :
              ((self->_lArg += reso->lArg) >= MAXLARG) ? (int)MK_lArg :
              ((self->_yArg += reso->yArg) >= MAXYARG) ? (int)MK_yArg : OK);
    /* >= because xArg, etc. point to the NEXT available location. */ 
    if (rtnVal)
      {   /* Undo effect of resoAlloc. */
          if (rtnVal != (int)MK_pLoop)
            if (self->isLoopOffChip)  /* See if we're moving back on chip. */
              self->isLoopOffChip = givePELoop(self,reloc->pLoop - noops); 
            else self->_piLoop = reloc->pLoop - noops; /* Wind back. */
          self->_xArg = reloc->xArg;
          self->_lArg = reloc->lArg;
          self->_yArg = reloc->yArg;
          giveDataMem(self,MK_yData,reloc->yData);
          giveDataMem(self,MK_xData,reloc->xData);
          giveDataMem(self,MK_pSubr,reloc->pSubr);
          return rtnVal;
      }
    if (leapOffChip)
      putLeaper(self,reloc->pLoop);
    self->computeTime += time;
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC)) {
        _MKOrchTrace(self,MK_TRACEORCHALLOC,
                     "Reloc: pLoop %d, xArg %d, yArg %d, lArg %d, xData %d, yData %d, pSubr %d",
                     reloc->pLoop,reloc->xArg,reloc->yArg,reloc->lArg,
                     reloc->xData,reloc->yData,reloc->pSubr);
        _MKOrchTrace(self,MK_TRACEORCHALLOC,
                     "Reso: pLoop %d, xArg %d, yArg %d, lArg %d, xData %d, yData %d, pSubr %d, time %e",
                     pLoopNeeds,reso->xArg,reso->yArg,reso->lArg,
                     reso->xData,reso->yData,reso->pSubr,time);
    }
    return OK;
}


#define KEEPTRYING 0
#define LOSE 1
#define WIN 2

static id getUG(self,factObj,beforeObj,afterObj,optionP)
    Orchestra *self;
    id factObj,beforeObj,afterObj;
    unsigned *optionP;
{
    /* Returns a unit generator if one is around. Does not create one. */
    id rtnVal;
    if (afterObj) 
      if (beforeObj)
        rtnVal = [factObj _allocFirstAfter:afterObj before:beforeObj 
                list:self->orchIndex];
      else 
        rtnVal = [factObj _allocFirstAfter:afterObj list:self->orchIndex];
    else if (beforeObj)
      rtnVal = [factObj _allocFirstBefore:beforeObj list:self->orchIndex];
    else rtnVal = [factObj _allocFromList:self->orchIndex];
    if (rtnVal) {
        *optionP = WIN;
        return rtnVal;
    }
    *optionP = KEEPTRYING;
    return nil;
}


static void allocError(self,allocErr)
    Orchestra *self;
    int allocErr;
{
    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                   (allocErr == OK) ?
                   "Allocation failure. DSP error." :
                   (allocErr == TIMEERROR) ? 
                   "Allocation failure. Not enough computeTime." : 
                   "Allocation failure. Not enough %s memory.",
                   orchMemSegmentNames[allocErr]);
}

static id 
  allocUG(self,factObj,beforeObj,afterObj)
register Orchestra *self;
id factObj,beforeObj,afterObj;
{
    /* Self is the orchestra instance. FactObj is the factory of the
       unit generator requested. beforeObj and afterObj, if specified,
       are used to limit the search. They should be unit generator
       instances. The algorithm used is as follows:
       
       Here's the algorithm for ug alloc:
       
       Deallocated ug of correct type? If so, use it.
       Otherwise, is there a ug of the correct type in a deallocated synth patch?
       If so, use it.
       Otherwise, pop stack. Enough resources for new ug?
       If so, make it.
       Otherwise, do compaction. Enough resources for new ug?
       If so, make it.
       Otherwise, fail.
       */
    id *templPtr;
    id aPatch,rtnVal,deallocatedPatches;     
    unsigned i,option;
    CHECKADJUSTTIME();
    if (self->deviceStatus == MK_devClosed)
      return nil;
    rtnVal = getUG(self,factObj,beforeObj,afterObj,&option);
    if (option == WIN) 
      return rtnVal;
    else if (option == LOSE)
      return nil;
    /* Now look if there's one in an deallocated syntpatch */
    i = nTemplates;
    templPtr = patchTemplates;
    while (i--) {
        deallocatedPatches = _MKDeallocatedSynthPatches(*templPtr,self->orchIndex);
        if ([deallocatedPatches lastObject]) /* peek */
          if (_MKIsClassInTemplate(*templPtr,factObj))
            while (aPatch = [deallocatedPatches removeLastObject]) {
                [aPatch _free];        /* Free up some UGs. */
                rtnVal = getUG(self,factObj,beforeObj,afterObj,&option);
                if (option == WIN) 
                  return rtnVal;
                else if (option == LOSE)
                  return nil;
            }
        templPtr++;  /* Try next template. */
    }
    {   /* Make a new one. */
#   if _MK_MAKECOMPILERHAPPY
        int allocErr = 0;
#   else
        int allocErr;
#   endif
        MKOrchMemStruct reloc;
        popResoAndSetLooper(self);
        if (!beforeObj) 
          /* We always add at the top of the stack. 
             So if we've made it to here, we know
             there can be no object after us. */    
          if ((allocErr = resoAlloc(self,factObj,&reloc)) == OK)
            if (!(rtnVal = 
                  [factObj _newInOrch:self index:self->orchIndex reloc:
                   &reloc looper:self->_looper])) 
              abortAlloc(self,factObj,&reloc);
        if (!rtnVal) {
            if (!compactResourceStack(self)) {
                if (beforeObj) {
                    if (_MK_ORCHTRACE(self,MK_TRACEORCHALLOC))
                      _MKOrchTrace(self,MK_TRACEORCHALLOC,
                                   "Allocation failure: Can't allocate before specified ug.");
                }
                else
                  allocError(self,allocErr);
                return nil;
            }
            if ((allocErr = resoAlloc(self,factObj,&reloc)) == OK)
              if (!(rtnVal = 
                    [factObj _newInOrch:self index:self->orchIndex reloc:
                     &reloc looper:self->_looper])) 
                abortAlloc(self,factObj,&reloc);
            if (!rtnVal) {
                allocError(self,allocErr);
                return nil;
            }
        }
        [self->stack addObject:rtnVal];
        insertNoops(self,reloc.pLoop - noops);
        /* We add the noops after the UG (in time) but positioned in memory
           before the UG. This is guaranteed to be safe, since we know we have
           a valid looper until we add the noops. */
        return rtnVal;
    }
    return nil;
}

@end
  
/* Private Orchestra interface */
@implementation Orchestra(Private)

+(id *)_addTemplate:aNewTemplate 
  /* The Orchestra keeps a list of SynthPatches indexed by PatchTemplates.
     PatchTemplate uses this to tell the Orchestra about each new Template. */
{
    static int curArrSize = 0;
    id *deallocatedPatches;
    int i;
#   define ARREXPAND 5 
    if (nTemplates == 0) {
        _MK_MALLOC(patchTemplates,id,ARREXPAND);
        curArrSize = ARREXPAND;
    }
    else if (nTemplates == curArrSize) {
        curArrSize = nTemplates + ARREXPAND;
        _MK_REALLOC(patchTemplates,id,curArrSize);
    }
    _MK_MALLOC(deallocatedPatches,id,nDSPs);
    for (i = 0; i < nDSPs; i++)
      deallocatedPatches[i] = [List new];
    patchTemplates[nTemplates++] = aNewTemplate;
    return deallocatedPatches;
}

@end

