#ifdef SHLIB
#include "shlib.h"
#endif

/* 
  Modification history:

  10/06/89/daj - Changed to import _exportedPrivateMusickit.h instead of
                 _musickit.h
  11/21/89/daj - Changed to check referenceCount of noiseLoc in claimNoise.
                 This is needed for new lazy deallocation of synthData.
  02/01/90/daj - Added error message when noise can't be allocated.	 
  04/07/90/daj - Changed to loop over parameters. Also added volume.
  05/14/90/daj - Changed to always set one zero on noteOn when status !=
                 running. This was a bug introduced when the patch was rewritten.
   08/28/90/daj - Changed initialize to init.
  09/02/90/daj - Changed MAXDOUBLE references to noDVal.h way of doing things
  09/29/90/daj - Commented out INLINE_MATH. The 040 exp bug is supposed to be fixed now.
*/
/* Pluck with shared noise */

// #define INLINE_MATH 1 /* Workaround for exp() bug. */
#define MK_INLINE 1
#import <musickit/unitgenerators/unitgenerators.h>
#import "_exportedPrivateMusickit.h"
#import <midi/midi_types.h>
#import "Pluck.h"

@implementation Pluck:SynthPatch
{
    /* Here are the parameters. */
    double freq;                  /* Frequency.   */
    double sustain;               /* Sustain parameter value */
    double ampRel;                /* AmpRel parameter value.*/
    double decay;                 /* Decay parameter value. */
    double bright;                /* Brightness parameter value */
    double amp;                   /* Amplitude parameter value.   */
    double bearing;               /* Bearing parameter value. */
    double baseFreq;              /* Frequency, not including pitch bend  */
    int pitchBend;                /* Modifies freq. */
    double pitchBendSensitivity;  /* How much effect pitchBend has. */
    double velocitySensitivity;   /* How much effect velocity has. */
    int velocity;                 /* Velocity scales bright. */
    int volume;                   /* Midi volume pedal */
    id _reservedPluck1;
    id _reservedPluck2;
    int _reservedPluck3;
    void * _reservedPluck4;
}
/* Plucked string without fine-tuning of pitch, 
   as described in Jaffe/Smith, Computer Music Journal
   Vol. 7, No. 2, Summer 1983. 

   Pluck is a simple physical model of a plucked string. It uses a 
   delay line to represent the string. The lower the pitch, the more delay
   memory it needs. Thus, a passage with many low notes may have problems
   running out of DSP memory. 

   Pluck does dynamic allocation of its delay
   memory. This may result in some loss over time due to DSP memory 
   fragmentation. The sollution to this problem is to preallocate delay memory
   by specifying the lowestFreq in the note passed to +patchTemplateFor:
   However, this feature is not supported in this version of Pluck. Future
   versions will have this feature. 

   This instrument has the following parameters:

   MK_freq       - Frequency. 
   MK_keyNum     - Alternative for frequency.
   MK_lowestFreq - This parameter is used to warn the synthpatch what the 
                   lowest note that may appear in the phrase will be so that
		   it can allocate an appropriate amount of delay memory.
		   It is only used in the first note of a phrase.
   MK_sustain    - On a scale from 0 to 1. 1 means "sustain forever".
   MK_pickNoise  - In seconds (duration of initial pick noise)
   MK_decay      - Time constant of decay or 0 for no decay in units of t60.
                   (That is, this is the time for the note to decay to -60dB.)
   MK_bright     - Brightness of the pluck. On a scale of 0 to 1. 
                   1 is very bright.
   MK_amp        - Amplitude of the pluck. On a scale of 0 to 1.0. Note that
                   resultant amplitude may be lower due to the nature of the 
                   noise used for the attack.
   MK_bearing    - On a scale from -45 to 45.
   MK_ampRel     - Time at end of note (after noteOff) for string to damp to
                   -60dB.
   
   Pluck is quite robust about handling noteUpdates. Most parameters can be
   sent as note updates. This makes it easy to put a slider on parameters.
   Pluck remembers the values you send it so each noteUpdate need not have all
   parameters in it. 

   Note, however, that the parameters MK_bright and MK_amp are applied to the
   attack (the "pick") only. Changing these values in a noteUpdate will have
   no effect until the following note of the phrase.
 */

/*
   This is an atypical SynthPatch in that it allocates its delay memory not 
   as part of itself, but on a note-by-note basis. 
   This is so that memory usage can be optimized. 
*/

/* #include "tune.m" */

#define noiseLoc _reservedPluck1 
#define delayMem _reservedPluck2
#define delayLen _reservedPluck3
 
static int delay,oneZero,out2Sum,onePole,dSwitch,pp2,pp,allPass,ppy;

+patchTemplateFor:currentNote
    /* In this simple case, always returns the same patch template. */
{
    static id tmpl = nil;
    if (tmpl)
      return tmpl;
    tmpl = [PatchTemplate new];

    /* First we allocate two patchpoints. */
    pp = [tmpl addPatchpoint:MK_xPatch];         /* General purpose */
    ppy = [tmpl addPatchpoint:MK_yPatch]; /* General purpose */
    pp2 = [tmpl addPatchpoint:MK_xPatch]; /* All pass output */

    /* Next we allocate unit generators. 
       Order is critical here, since we reuse patchpoints. Alternatively, 
       we could do a version that uses unordered UGs and 6 patchpoints. */

#   define ADDUG(_factory) [tmpl addUnitGenerator:[_factory class] ordered:YES]
    onePole =       ADDUG(OnepoleUGxx);
    dSwitch =       ADDUG(DswitchtUGyx);
    oneZero =       ADDUG(OnezeroUGxy);
    out2Sum =       ADDUG(Out2sumUGx);
    delay =         ADDUG(DelayUGxxy);
    allPass =       ADDUG(Allpass1UGxx);

#   define MSG(_target,_message,_arg) \
    [tmpl to:_target sel:@selector(_message) arg:_arg]
    MSG(onePole,      setOutput:,pp);
    MSG(dSwitch,      setInput1:,pp);
    MSG(dSwitch,      setOutput:,ppy);
    MSG(oneZero,      setInput:, ppy);
    MSG(oneZero,      setOutput:,pp);
    MSG(delay,        setInput:, pp);
    MSG(delay,        setOutput:,pp);
    MSG(allPass,      setInput:, pp);
    MSG(allPass,      setOutput:,pp2);
    MSG(dSwitch,setInput2:,pp2);
    /* We allow garbage to pump around the loop here. The assumption is that
       the attack will be at least 8 samples long so the garbage will clear
       out. */
    return tmpl;
}

#define SE(_x)        NX_ADDRESS(self->synthElements)[_x]
#define DELAYUG         SE(delay)
#define ONEZERO       SE(oneZero)
#define OUT2SUM       SE(out2Sum)
#define ONEPOLE       SE(onePole)
#define DSWITCH       SE(dSwitch)
#define PP2           SE(pp2)
#define PP            SE(pp)
#define ALLPASS       SE(allPass)

static double t60(double freq,double timeConst)
    /* TimeConst (ampRel or decay) is time constant in units of t(60). 
       See Jaffe/Smith, equations (4), (5) and (19). 
       I'm returning the value for DC to decay in time t(60). */ 
{
    return exp(-6.91/(freq * timeConst));
}

#define PIPE DSPMK_NTICK    /* one-tick pipe delay */

static int tuneIt(double sRate,double freq,double sustain,double *c)
    /* Returns delay length. Also sets coefficient of all pass (*c) and
       resets sustain value. Make sure that the PIPE macro is set 
       correctly. */
{
#   define EPSILON .01         /* See Eq. 15 */
    int Pb;                    /* Delay length */
    double P;                  /* True period. */ 
    double o;                  /* Radian freq. */
    double Pa;                 /* Delay from one-zero filter; */
    double Pc;	               /* Delay from allpass */
    double S;                  /* 1-zero coeff. */
    double extraDelay;            /* Temporary variable */
    double oPc;                /* Optimization to avoid multiplying twice. */
    S = (sustain * .5) + .5;   /* 1-zero coeff. */
    P = sRate/freq;            /* Period */
    o = 2 * M_PI * freq/sRate; /* Radian freq */
    
    /*  Delay from one zero filter. From Eq. 22 */
    Pa = (-1/o) * atan2(-S * sin(o), (1 - S) + S * cos(o)); 
    
    /*  Delay from delay line. Don't round so that Pc, is > 0. */
    extraDelay = PIPE + Pa;
    Pb = P - (extraDelay + EPSILON); /* Eq. 15A */
    Pc = P - (extraDelay + Pb);      /* Eq. 15B */
    oPc = o * Pc;
    *c = sin(.5 * (o - oPc)) / sin(.5 * (o + oPc));   /* Eq. 16 B */
    return Pb;
}

static void setOneZeroCoeffs(oneZ,damper,sustain)
    id oneZ;
    double damper,sustain;
{
    [oneZ setB0:damper*(1.0-sustain)*0.5];
    [oneZ setB1:damper*(1.0+sustain)*0.5];
}

#ifndef MAX
#define  MAX(A,B)	((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define  MIN(A,B)	((A) < (B) ? (A) : (B))
#endif
#ifndef ABS
#define  ABS(A)		((A) < 0 ? (-(A)) : (A))
#endif

-_updatePars:aNote noteType:(MKNoteType)noteType 

  /* This handles all parameters which may be set at any time. */
{
    MKPhraseStatus pStatus = [self phraseStatus];
    BOOL needToSetOneZero, newPhrase, needToSetBearing, needToTune;
    double lowestFreq = MK_NODVAL;
    int oldDelayLen,longestDelayLen,par;
    void *state;

    switch (pStatus) {
    	case MK_phraseOn:
	case MK_phraseOnPreempt:
		needToSetBearing = needToTune = newPhrase = YES;
		needToSetOneZero = (status != MK_running);
		break;
	case MK_phraseRearticulate:
		/* If we were previously finishing, need to reset one zero since  
decay changes */
		needToSetOneZero = (status == MK_finishing);
		if (needToSetOneZero)
			needToTune = YES;
		needToSetBearing = NO;
		newPhrase = NO;
		break;
	default: /* Should never happen */
	case MK_phraseUpdate: 

        case MK_phraseOffUpdate:
		needToTune = needToSetBearing = newPhrase =  
needToSetOneZero = NO;
		break;
	case MK_phraseOff:
		needToSetOneZero = YES;
		needToTune = YES; /* When we set one zero, tuning changes. */
		newPhrase = NO;
		needToSetBearing = NO;
		break;
    }
    /* Get parameters */
    state = MKInitParameterIteration(aNote);
    while (par = MKNextParameter(aNote, state))  

      switch (par) { 

	case MK_sustain:
	  sustain = MKGetNoteParAsDouble(aNote,MK_sustain);
	  sustain = MAX(sustain,0);
	  sustain = MIN(sustain,.999);
	  needToSetOneZero = YES;
	  needToTune = YES; /* When we set one zero, tuning changes.  */
	  break;
	case MK_ampRel:
	  ampRel = MKGetNoteParAsDouble(aNote,MK_ampRel);
	  ampRel = MAX(ampRel,0);
	  /* May not need to set it here, since it could be set for later. */
	  break;
	case MK_bearing:
	  bearing = MKGetNoteParAsDouble(aNote,MK_bearing);
	  needToSetBearing = YES;
	  break;
	case MK_freq:
	case MK_keyNum:
	  baseFreq = [aNote freq];
	  needToTune = YES;
	  break;
	case MK_pitchBend:
	  pitchBend = MKGetNoteParAsInt(aNote,MK_pitchBend);
	  needToTune = YES;
	  break;
	case MK_pitchBendSensitivity:
	  pitchBendSensitivity = MKGetNoteParAsDouble(aNote,
						      MK_pitchBendSensitivity);
	  needToTune = YES;
	  break;
	case MK_lowestFreq:
	  lowestFreq = MKGetNoteParAsDouble(aNote,MK_lowestFreq);
	  /* Note that this parameter has no effect when not in a noteOn. */
	  break;
	case MK_decay:
	  decay = MKGetNoteParAsDouble(aNote,MK_decay);
	  decay = MAX(decay,0);
	  if (status != MK_finishing){ /* Decay unused in finishing section */
	         needToSetOneZero = YES;
		 needToTune = YES;   /* When we set one zero, tuning changes.  */
	  }
	  break;
	case MK_amp:
	  amp = MKGetNoteParAsDouble(aNote,MK_amp);
	  break;
	case MK_bright:
	  bright = MKGetNoteParAsDouble(aNote,MK_bright);
	  break;
	case MK_velocity:
	  velocity = MKGetNoteParAsInt(aNote,MK_velocity);
	  break;
	case MK_velocitySensitivity:
	  velocitySensitivity = 

	    MKGetNoteParAsDouble(aNote,MK_velocitySensitivity);
	  break;
	case MK_controlChange: {
	    int controller = MKGetNoteParAsInt(aNote,MK_controlChange);
	    if (controller == MIDI_MAINVOLUME) {
		volume = MKGetNoteParAsInt(aNote,MK_controlVal);
		needToSetBearing = YES; 

	    } 

	    break;
	}
	default:
	  break;
      }
    if (needToTune) { 

	  double srate = [[self orchestra] samplingRate];
 	  double allPassCoefficient;
	  freq = MKAdjustFreqWithPitchBend(baseFreq,pitchBend,
					   pitchBendSensitivity);
	  delayLen = tuneIt(srate,freq,sustain,&allPassCoefficient);
	  if (delayLen < 1) {
	      _MKErrorf(MK_spsOutOfRangeErr,"pitch",MKGetTime());
	      delayLen = 0;
	      return nil;
	  }
	  [ALLPASS setBB0:allPassCoefficient];
	  /* Adjust length */
	  if (newPhrase) {
	      /* See if guy wants to allocate more. */
	      oldDelayLen = 0;
	      if (!MKIsNoDVal(lowestFreq)) {
		  double tmp;
		  longestDelayLen = tuneIt(srate,lowestFreq,sustain,&tmp) + 1;
		  if (longestDelayLen < delayLen)
		    longestDelayLen = 0;
		  /* 1 for good luck (or in case sustain changes!) */
	      } else longestDelayLen = 0;
	  } else {                 /* We've already got delay mem allocated */
	      longestDelayLen = 0;
	      oldDelayLen = [delayMem length];
	      if (oldDelayLen < delayLen) {         /* Not big enough ? */
		  [delayMem dealloc];               /* Free it */
		  delayMem = nil;
	      }
	      else {
#if 0
		  int curLen = [DELAYUG length];      

		  /* If we don't clear out new memory, why should we do it
		     now??? This isn't needed if it's another noteOn because
		     the new noise will fill up the buffer. It could be needed
		     for a noteUpdate but in this case, we take a chance. */
		  if (delayLen > curLen)           /* get rid of old crap */
		    [delayMem setToConstant:0 length:delayLen - curLen offset:
		     curLen];
#endif
		  [DELAYUG adjustLength:delayLen];    /* else adjust it */
	      }
	  }
	  /* Need to alloc? */
	  if (!delayMem) {                               

	      delayMem = [[self orchestra] allocSynthData:MK_yData length:
			  (longestDelayLen) ? longestDelayLen : delayLen];
	      if (!delayMem) {                           /* Alloc failed? */
		  _MKErrorf(MK_spsCantGetMemoryErr,"low pitch",MKGetTime());
		  if (noteType == MK_noteOn) {
		      delayLen = 0;
		      return nil;                          /* Omit note. */
		  }
		  delayLen = oldDelayLen;
		  /* Try and revert to old situation to save hell from 

		     breaking loose. */
		  delayMem = 

		    [[self orchestra] allocSynthData:MK_yData length:delayLen];
		  if (!delayMem) {                /* Now we've really lost */
		      delayLen = 0;
		      return nil;
		  }
	      } 

	      [DELAYUG setDelayMemory:delayMem];
	      if (longestDelayLen)
		[DELAYUG adjustLength:delayLen];
	  }
      }
    /* Now set the filters and such */
    if (needToSetOneZero) {
	double dec;
	if (noteType == MK_noteOff || 

	    ((status == MK_finishing) && noteType == MK_noteUpdate))
	  dec = (ampRel > .00001) ? t60(freq,ampRel) : 0;
	else 

	  dec = (decay == 0) ? 1 : t60(freq,decay);
	setOneZeroCoeffs(ONEZERO,dec,sustain);
    }
    if (noteType == MK_noteOn) {
	double actualAmp = 

	  amp * MKMidiToAmpWithSensitivity(velocity,velocitySensitivity);
	[DSWITCH setScale1:actualAmp];
	[ONEPOLE setBrightness:(bright * 

				((1.0 - velocitySensitivity) + 

				 velocitySensitivity * velocity * .015625)) 

	 /* Velocity/64 */ 

       forFreq:freq];
    }
    if (needToSetBearing)
      [OUT2SUM setBearing:bearing  
scale:MKMidiToAmpAttenuation(volume)];
    return self;
}

static id noiseLocObj = nil;  /* Placeholder for table */

static id slaveLists; /* Subserviant shared objects. This is a real hack. */
		 
static id addSlaveList(id theOrch)
{
    List *aList;
    [slaveLists addObject:aList = [List newCount:6]];
    aList->numElements = 6;
    NX_ADDRESS(aList)[0] = theOrch;
    return aList;
}

static id getSlaveListFor(id theOrch)
    /* We have to do this because there may be multiple orchestras. */
{
    int cnt = [slaveLists count];
    id innerList;
    int i;
    for (i=0; i<cnt; i++) {
	innerList = NX_ADDRESS(slaveLists)[i];
	if (NX_ADDRESS(innerList)[0] == theOrch)
	  return innerList;
    }
    return addSlaveList(theOrch);
}

enum {noiseFlt1InIndex = 1,noiseFlt1Index,noiseFlt2InIndex,noiseFlt2Index,
	noiseIndex};

static id claimNoise(Pluck *self)
{
    id slaveList = getSlaveListFor(self->orchestra);
    id noise,noiseFlt1In,noiseFlt2In,noiseFlt1,noiseFlt2;
    if (self->noiseLoc)       /* Shouldn't ordinarily happen. */
      return self->noiseLoc;
    self->noiseLoc = [self->orchestra sharedObjectFor:noiseLocObj];
    if (!self->noiseLoc) {
	/* Need to reinitialize */
	self->noiseLoc = [self->orchestra allocPatchpoint:MK_xPatch];
	/* Now here are the 'slaves' */
	noise = [self->orchestra allocUnitGenerator:[UnoiseUGx class]];
	noiseFlt1In = [self->orchestra allocPatchpoint:MK_xPatch];
	noiseFlt1 = [self->orchestra allocUnitGenerator:
			   [OnezeroUGyx class]];
	noiseFlt2In = [self->orchestra allocPatchpoint:MK_yPatch];
	noiseFlt2 = [self->orchestra allocUnitGenerator:
			   [OnepoleUGxy class]];
	NX_ADDRESS(slaveList)[noiseFlt1InIndex] = noiseFlt1In;
	NX_ADDRESS(slaveList)[noiseFlt2InIndex] = noiseFlt2In;
	NX_ADDRESS(slaveList)[noiseFlt1Index] = noiseFlt1;
	NX_ADDRESS(slaveList)[noiseFlt2Index] = noiseFlt2;
	NX_ADDRESS(slaveList)[noiseIndex] = noise;
	if (!(noise && noiseFlt1In && noiseFlt1 &&
	      noiseFlt2In && noiseFlt2 && self->noiseLoc)) {
	    [noise dealloc];
	    [noiseFlt1In dealloc];
	    [noiseFlt1 dealloc];
	    [noiseFlt2In dealloc];
	    [noiseFlt2 dealloc];
	    [self->noiseLoc dealloc];
	    self->noiseLoc = nil;
	    _MKErrorf(MK_spsCantGetUGErr,"Pluck noise",MKGetTime());
	    return nil;
	}
	/* Connect them */
	[noise setOutput:noiseFlt1In];
	[noiseFlt1 setInput:noiseFlt1In];
	[noiseFlt1 setOutput:noiseFlt2In];
	[noiseFlt1 setB0:.5]; 
	[noiseFlt1 setB1:-.5];
	[noiseFlt2 setInput:noiseFlt2In];
	[noiseFlt2 setOutput:self->noiseLoc];
	[noiseFlt2 setB0:1.0]; 
	[noiseFlt2 setA1:-.99];
	[self->orchestra installSharedObject:self->noiseLoc for:noiseLocObj];
	[noise run];
	[noiseFlt1 run];
	[noiseFlt2 run];
    }
    return self->noiseLoc;
}

static void setDefaults(Pluck *self)
  /* Sent by this class on object creation and reset. */
{
    self->bright = MK_DEFAULTBRIGHT;
    self->velocitySensitivity = .5;
    self->pitchBendSensitivity = 3.0; /* Changed by DAJ to match other 
					 patches */
    self->velocity = 64;
    self->freq = MK_DEFAULTFREQ;
    self->baseFreq = MK_DEFAULTFREQ;
    self->pitchBend = MIDI_ZEROBEND;
    self->sustain = MK_DEFAULTSUSTAIN;
    self->decay = MK_DEFAULTDECAY;
    self->amp = MK_DEFAULTAMP;
    self->ampRel = .15;
    self->volume = MIDI_MAXDATA;
}

-preemptFor:aNote
{
    /* This is unnecessary but if it's changed, the logic in updateParams
       must change too. */
    [delayMem dealloc];          /* Now free up memory. */
    delayMem = nil;
    [DELAYUG setDelayMemory:nil];
    setDefaults(self);
    return nil;
}

-init
{
    if (!noiseLocObj) { /* Create place-holder. */
	noiseLocObj = [Object new];
	slaveLists = [List new]; /* Mapping from DSP number to 'slave'
				      shared objects. This is an attempt
				      to do something more efficient than
				      making all of them be shared objects.
				      Sigh. */
	addSlaveList(orchestra);
    }
    setDefaults(self);  /* Set default instance variables */
    [DSWITCH setScale2:1.0];
    return claimNoise(self);  /* We need to make sure the noise is claimable */
}

-noteOnSelf:aNote
  /* Sent whenever a noteOn is received by the SynthPatch. 
     We allocate our delay memory here. */
{
    int attSamps,i;
    double pickNoise;
    if (status == MK_idle)
      if (!(noiseLoc = claimNoise(self)))
	return nil;
    if (![self _updatePars:aNote noteType:MK_noteOn])
      return nil;
    [ONEPOLE clear];
    [ONEPOLE setInput:noiseLoc];
    pickNoise = MKGetNoteParAsDouble(aNote,MK_pickNoise);
    if (!MKIsNoDVal(pickNoise))
      attSamps = (int)(pickNoise * [orchestra samplingRate]);
    else attSamps = delayLen + PIPE + 2; /* 2 for 'good luck' */ 
    if (attSamps < (i = delayLen + PIPE + 2))
      attSamps = i;
    [ONEZERO clear]; /* There could be old large samples in the loop. */
    [DSWITCH setDelayTicks:attSamps/DSPMK_NTICK + 1];
    [OUT2SUM setInput:PP];
    [synthElements makeObjectsPerform:@selector(run)];
    return self;
}

-noteEndSelf
  /* Sent when patch goes from finalDecay to idle. */
{
    /* The following is just in case the note is so short that the noise
       hasn't stopped yet. */
    [OUT2SUM idle];
    [DSWITCH setDelayTicks:-1];/* Make sure it's reading input2 now. */
    [delayMem dealloc];          /* Now free up memory. */
    delayMem = nil;
    [DELAYUG setDelayMemory:nil];
    if (noiseLoc) {              /* Upon creation, this is a noop. */
	id slaveList;
	int i = [noiseLoc referenceCount];
	[noiseLoc dealloc];
	noiseLoc = nil;
	if (i == 1) {
	    slaveList = getSlaveListFor(orchestra);
	    [NX_ADDRESS(slaveList)[noiseFlt1Index] dealloc];
	    [NX_ADDRESS(slaveList)[noiseFlt2Index] dealloc];
	    [NX_ADDRESS(slaveList)[noiseFlt1InIndex] dealloc];
	    [NX_ADDRESS(slaveList)[noiseFlt2InIndex] dealloc];
	    [NX_ADDRESS(slaveList)[noiseIndex] dealloc];
	}
    }
    setDefaults(self);  /* Set default instance variables */
    return self;
}

-noteUpdateSelf:aNote
{
    if (![self _updatePars:aNote noteType:MK_noteUpdate])
      [self noteEndSelf];
    return self;
}

-(double)noteOffSelf:aNote
  /* Sent when patch goes from running to finalDecay */
{
    if (![self _updatePars:aNote noteType:MK_noteOff])
      [self noteEndSelf];
    return ampRel;
}

#import <objc/HashTable.h>

-controllerValues:controllers
  /* Sent when a new phrase starts. controllers is a HashTable containing
   * key/value pairs as controller-number/controller-value. Our implementation
   * here ignores all but MIDI_MAINVOLUME.  */
{
#   define CONTROLPRESENT(_key) [controllers isKey:(const void *)_key]
#   define GETVALUE(_key) (int)[controllers valueForKey:(const void *)_key]
    if (CONTROLPRESENT(MIDI_MAINVOLUME))
      volume = GETVALUE(MIDI_MAINVOLUME);
    return self;
}

@end







