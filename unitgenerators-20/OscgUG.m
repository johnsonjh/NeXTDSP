#ifdef SHLIB
#include "shlib.h"
#endif
/* Simple oscillator with no envelopes */
/* 
  Modification history:
  
  11/13/89/daj - Simplified defaultTableLength:.
  11/21/89/daj - Fixed bug in shared data mechanism: changed table space to
                 table segment.
  11/22/89/daj - Changed to use UnitGenerator C functions for speed.	 
  11/27/89/daj - Propogated MMM's fix into this file. Also added 
                 new method setTableToSineROM. 
  11/28/89/daj - Fixed bug in setTableToSineROM. It wasn't freeing locally
                 allocated table. Also fixed bug in error message reporting
                 (not enough arguments to printf). Also supressed supurfluous
                 printing of substitution error when table is in wrong space.
		 Got rid of superfluous setting of orchestra in -initialize.
		 Changed idleSelf to deallocate via orchestra rather than
		 Orchestra, for efficiency. 
		 Changed idleSelf to return a value. Added return self; to
		 setPhase.
  02/01/90/daj - Replaced obsolete method 
                 -installSharedObject:for:segment:length:
		 with -installSharedSynthDataWithSegmentAndLength:.
  08/28/90/daj - Changed initialize to init.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
  */
#define MK_INLINE 1
#import <musickit/musickit.h>
#import "_exportedPrivateMusickit.h"
#import "_unitGeneratorInclude.h"
#import <soundkit/Sound.h>
#import "OscgUG.h"

@implementation OscgUG:UnitGenerator
  /* OscgUG<a><b>, where <a> is output space and <b> is table space 
     
     oscg (UG macro) - Simplest oscillator with general address mask
     
     This is a non-interpolating oscillator. That means that its fidelity
     depends on the size of the table (larger tables have lower distortion)
     and the highest frequency represented in the table. For high-quality
     synthesis, an interpolating oscillator is preferable. However, an
     interpolating oscillator is also more expensive. OscgUG is useful
     in cases where density of texture is more important than fidelity of
     individual sounds.
     
     The wavetable length must be a power of 2.
     The wavetable increment must be nonnegative.
     
     */
{
    double _reservedOscg1;
    double _reservedOscg2;
    double _reservedOscg3;
    id _reservedOscg4;
    int tableLength;        /* Or 0 if no table. */
}
#define _phase _reservedOscg1
#define _freq _reservedOscg2
#define _mag  _reservedOscg3
#define _table _reservedOscg4

enum _args { atab, mtab, amp, aout, inc, phs};

#import "oscgUGInclude.m"

#if _MK_UGOPTIMIZE 
+(BOOL)shouldOptimize:(unsigned) arg
{
    return (arg != phs);
}
#endif _MK_UGOPTIMIZE

-init
{
    [super init];
    _freq = MK_NODVAL;
    _phase = MK_NODVAL;
    _mag = MK_NODVAL;
    return self;
}

-runSelf
  /* Sets phase if setPhase was called before lookup table was set, and
     sets frequency to the last value set with setFreq.
     If wavetable has not been set, and table space is Y, uses dsp sine rom. */
{
    if (tableLength == 0) {
        DSPMemorySpace tableSpace = [[self class] argSpace:atab];
	if (tableSpace == DSP_MS_Y) {
	    [self setTableToSineROM];
	}
	else {
	    _MKErrorf(MK_ugsNotSetRunErr,"Wavetable",[self name]);
	    return nil;
	}
    }
    if (!MKIsNoDVal(_phase)) {
	[self setPhase: _phase];
	_phase = MK_NODVAL;
    }
    if (!MKIsNoDVal(_freq)) {
	[self setFreq:_freq];
	_freq = MK_NODVAL;
    }
    return self;
}

-freeSelf
  /* Deallocates local wave table memory, if any. */
{
    tableLength = 0;
    if (_table) 
      [orchestra dealloc:_table];
    _table = nil;
    return nil; /* return value is ignored */
}

-idleSelf
  /* Deallocates local wave table memory, if any, and patches output to Sink. */
{
    tableLength = 0;
    if (_table) 
      [orchestra dealloc:_table];
    _table = nil;
    return [self setAddressArgToSink:aout]; /* Patch output to sink. */
}

-setInc:(int)anInc
  /* Sets increment as an integer as specified. */
{
    MKSetUGDatumArg(self,inc,DSPIntToFix24(anInc));
    _freq = MK_NODVAL; /* Setting increment invalidates freq */ 
    return self;
}

-setAmp:(double)anAmp
{
    /*  if (anAmp != MAXDOUBLE) */
    MKSetUGDatumArg(self,amp,DSPDoubleToFix24(anAmp));
    return self;
}

-setFreq:(double)aFreq
{
    if (tableLength == 0)
      _freq = aFreq;
    else {
	DSPFix48 aFix48;
	DSPDoubleToFix48UseArg(aFreq * _mag * TWO_TO_M_23, &aFix48);
	MKSetUGDatumArgLong(self,inc,&aFix48);
	_freq = MK_NODVAL;
    }
    return self;
}

-setPhase:(double)aPhase
  /* Sets oscillator phase in degrees. If wavetable has not yet been set,
     stores the value for runSelf to use to set the phase later. */
{
#   define OVER360(_aPhase) _aPhase * .002777777777777778 /* Avoid division */
    if (tableLength == 0)
      _phase = aPhase;
    else {
	DSPFix48 phase48;
	DSPDoubleToFix48UseArg((double)tableLength * OVER360(aPhase) *
			       TWO_TO_M_23, 
			       &phase48);
	_phase = MK_NODVAL;
	return MKSetUGDatumArgLong(self,phs,&phase48);
    }
    return self;
}

-setOutput:aPatchPoint
  /* Set output of osc */
{
    return MKSetUGAddressArg(self,aout,aPatchPoint);
}

#if 0
+(int)defaultTableLength:anObj
{
    if ([anObj isKindOf:[Samples class]])
      return [[anObj sound] sampleCount];
    else if ([anObj isKindOf:[Partials class]]) {
	switch ((int)ceil([anObj highestFreqRatio]/16)) {
	  case 0: return 64;    /* no partials? */
	  case 1: return 256;   /* 1 to 16  */
	}
	return 512;             /* 16 < */
    }
    else if ([anObj respondsTo:@selector(length)])
      return (int)[anObj length];
    else return 512;
}
#endif

+(int)defaultTableLength:anObj
  /* Provides for a power-of-2 table length with a reasonable number 
     of samples for the highest component. */
{
    if ([anObj isKindOf:_MKClassPartials()]) 
      return 256;
    else if ([anObj isKindOf:_MKClassSamples()])
      return [[anObj sound] sampleCount];
    else if ([anObj respondsTo:@selector(length)])
      return (int)[anObj length];
    else return 256;
}

-setTable:anObj
  /* Like setTable:length, but uses a default length. An argument of
     nil will still cause dellocation of any locally allocated dsp memory. */
{
    if (_table)
      [orchestra dealloc:_table];
    _table = nil;
    tableLength = 0;
    if (!anObj) return nil;
    return [self setTable:anObj length:[[self class] defaultTableLength:anObj]];
}

-setTable:anObj length:(int)aLength
  /* Sets the lookup table of the unit generator.
     The argument can be a WaveTable (Partials or Samples), or SynthData object.
     In the former case, an existing SynthData is first searched for which is
     based on the same WaveTable, of the same length, and in the required memory
     space. Otherwise, a local SynthData object is created and installed so that
     other unit generators running simultaneously may share it, in order to
     conserve dsp memory. In the latter case, the unit generator is simply
     set up to use the supplied SynthData. Note that altering the contents
     of a WaveTable will have no effect once it has been installed, even
     if you call setTable:length: again after modifying the WaveTable. The 
     reason is that this method uses the Orchestra sharedObject mechanism to
     determine if the WaveTable is already represented on the DSP. Since the
     Orchestra uses the isSame: method, it is the object's id that determines
     its identity, not its contents.
     */
{
    MKOrchMemSegment tableSegment = 
      ([[self class] argSpace:atab] == DSP_MS_X) ? MK_xData : MK_yData;
    id synthData = nil;
    if (_table) { 
	[orchestra dealloc:_table];
	_table = nil;
	tableLength = 0; /* mmm Nov 28, 89 */
    }
    if (!anObj) return nil;
    if (!aLength || (aLength==MAXINT)) 
      aLength = [[self class] defaultTableLength:anObj];
    /* Currently, Samples objects cannot do resampling on the fly. */
    if ([anObj isKindOf:[WaveTable class]]) {
	if ([anObj isKindOf:[Samples class]])
	  aLength = [[anObj sound] sampleCount];
	/* First see if memory with this table in it already exists. */
	synthData = [orchestra sharedObjectFor:anObj segment:tableSegment 
		   length:aLength];
	if (!synthData) {
	    /* If we can't allocate the memory, try halving the length (up to a 
	       point) */
	    while (!(synthData = [orchestra allocSynthData:tableSegment
				length:aLength]) && (aLength >= 64)) {
		aLength >>= 1;
		if (MKIsTraced(MK_TRACEUNITGENERATOR))
		  fprintf(stderr,"Insufficient wavetable memory at time %.3f. \n"
			  "Trying smaller table length %d.\n",MKGetTime(), aLength);
	    }
	    if (synthData) {
		[synthData setData:[anObj dataDSPLength: aLength]
	       length:aLength offset:0];
		[orchestra installSharedSynthDataWithSegmentAndLength:
		 synthData for:anObj];
	    }
	    else if (MKIsTraced(MK_TRACEUNITGENERATOR)) /* daj */
	      fprintf(stderr,"Insufficient wavetable memory at time %.3f. \n",MKGetTime());
	}
	_table = synthData;
    }
    else if ([anObj isKindOf:[SynthData class]]) {
	synthData = anObj;
	if (aLength > [synthData length])  /* Moved by DAJ */
	  aLength = [synthData length];
    }
    if (synthData) {
	/* do a little error checking here on the length. */
	if (!_MKUGIsPowerOf2(aLength)) {
	    /* The following statement added by DAJ */
	    if (anObj != synthData) /* If user alloced, let it be */
	      [synthData dealloc];  /* Release our claim on it. */
	    _MKErrorf(MK_ugsPowerOf2Err,[self name]);
	    return nil;
	}
	/* Set some relevant instance variables. */
	tableLength = aLength;
	_mag = (double)tableLength / [orchestra samplingRate];
	/* Finally, actually do the dsp setting. */
	MKSetUGDatumArg(self,mtab,tableLength - 1);
	return MKSetUGAddressArg(self,atab,synthData);
    }
    return nil; 
}

-setTableToSineROM
  /* Sets the lookup table of the unit generator to the dsp sine ROM, 
     if address space is Y. Otherwise, generates an error.
     */
{
    if ([[self class] argSpace:atab] == DSP_MS_X) {
	if (MKIsTraced(MK_TRACEUNITGENERATOR))
	  fprintf(stderr,"X-space oscgaf cannot use sine ROM at time %.3f. \n",
		  MKGetTime());
	_MKErrorf(MK_spsCantGetMemoryErr,"",MKGetTime());
	return nil; /* added by Daj */
    }
    if (_table) {  /* Added by DAJ. */
	[orchestra dealloc:_table];
	_table = nil;
    }
    tableLength = DSP_SINE_LENGTH;
    _mag = ((double)DSP_SINE_LENGTH) / [orchestra samplingRate];
    MKSetUGDatumArg(self,mtab,DSP_SINE_LENGTH-1);
    return MKSetUGAddressArg(self,atab,[orchestra sineROM]);
}

-setTable:anObj length:(int)aLength defaultToSineROM:(BOOL)sineROMDefaultOK
  /* Like setTable:length:, but defaults to the sine ROM in the dsp if dsp memory
     for the given table cannot be allocated, and if the table memory space
     of the unit generator instance is Y.  */
{
    id success;
    if (!(success=[self setTable:anObj length:aLength]) && sineROMDefaultOK) {
	success = [self setTableToSineROM];
	if (anObj && /* If anObj is nil (user-specified sine ROM), don't warn */
	    success) 
	  _MKErrorf(MK_spsSineROMSubstitutionErr, MKGetTime());
    }
    return success;
}

-setTable:anObj defaultToSineROM:(BOOL)sineROMDefaultOK
  /* Like setTable:, but defaults to the sine ROM in the dsp if dsp memory
     for the given table cannot be allocated, and if the table memory space
     of the unit generator instance is Y.  */
{
    /* setTable:length will eventually get the default length from the subclass,
       so we don't need to do anything special. */
    return [self setTable:anObj length:0 defaultToSineROM:sineROMDefaultOK];
}

-(unsigned)tableLength
{
    return tableLength;
}

@end
