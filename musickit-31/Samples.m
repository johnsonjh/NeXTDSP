#ifdef SHLIB
#include "shlib.h"
#endif

/* 
  modification history:

  12/4/89 /daj - Fixed normalization in fillTableLength:scale:.
   3/19/90/daj - Added MKGet/SetSamplesClass().
  03/21/90/daj - Added archiving.
  03/21/90/daj - Small changes to quiet -W compiler warnings.
  08/27/90/daj - Changed to zone API.
*/
#import "_musickit.h"
#import "_scorefile.h"
#import "_error.h"
#import "Samples.h"

#import <soundkit/Sound.h>   

@implementation  Samples : WaveTable
/* The Samples object allows you to use a Sound as data in DSP synthesis.
   The most common use is as a wavetable table for an oscillator.
   You may set the Sound from a Sound object or from a Soundfile using
   setSound: or readSoundfile:.

   Access to the data, is provided by the superclass, WaveTable.
   Samples currently does not provide resampling functionality.
   Hence, it is an error to ask for an array length other than the
   length of the original Sound passed to the object.
   */
{
	id sound;          /* Sound object set by app or internally. */
	char * soundfile; /* Name of soundfile set by app. */
}

static id theSubclass = nil;

BOOL MKSetSamplesClass(id aClass)
{
    if (!_MKInheritsFrom(aClass,[Samples class]))
      return NO;
    theSubclass = aClass;
    return YES;
}

id MKGetSamplesClass(void)
{
    if (!theSubclass)
      theSubclass = [Samples class];
    return theSubclass;
}

#define VERSION2 2

+initialize
{
    if (self != [Envelope class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

-  init
  /* This method is ordinarily invoked only by the superclass when an 
     instance is created. You may send this message to reset the object. */ 

{
    [super init];
    sound = [sound free];
    if (soundfile)
      NX_FREE(soundfile);
    soundfile = NULL;
    return self;
}


- write:(NXTypedStream *) aTypedStream
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"@*",&sound,&soundfile);
    return self;
}

- read:(NXTypedStream *) aTypedStream
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Samples") == VERSION2) 
      NXReadTypes(aTypedStream,"@*",&sound,&soundfile);
    return self;
}

-  free
  /* Frees object and Sound. */
{
    sound = [sound free];
    if (soundfile)
      NX_FREE(soundfile);
    return [super free];
}

- copyFromZone:(NXZone *)zone
  /* Copies receiver. Copies the Sound as well. */
{
    Samples *newObj = [super copyFromZone:zone];
    newObj->soundfile = _MKMakeStr(soundfile);
    newObj->sound = [Sound new];
    if ([newObj->sound copySound:sound] != SND_ERR_NONE) 
      return newObj;
    return nil;
}

- readSoundfile:(char *)aSoundfile 
/* Creates a sound object from the specified file and initializes the
   receiver from the data in that file. Implemented in terms of 
   setSound:. This method creates a Sound object which is owned
   by the receiver. You should not free the Sound. 
   
   Returns self or nil if there's an error. */
{
    id aTmpSound = [Sound newFromSoundfile:aSoundfile];
    if (!aTmpSound)
      return nil;
    if (soundfile)
      NX_FREE(soundfile);
    if (![self setSound:aTmpSound]) {
	[aTmpSound free];
	soundfile = NULL;
	return nil;
    }
    soundfile = _MKMakeStr(aSoundfile);
    return self;
}

- setSound:aSoundObj 
/* Sets the Sound of the Samples object to be aSoundObj.
   aSoundObj must be in 16-bit linear mono format. If not, setSound: returns
   nil. aSoundObj is copied. 

 */
{
    if (!aSoundObj)
      return nil;
    [sound free];
    length = 0; /* This ensures that the superclass recomputes cached buffers. */ 
    sound = nil;
    if (([aSoundObj dataFormat] != SND_FORMAT_LINEAR_16) ||
	([aSoundObj channelCount] != 1))
      return nil; /*** FIXME Eventually convert ***/
    sound = [Sound new];
    if ([sound copySound:aSoundObj] != SND_ERR_NONE) {
	[sound free];
	sound = nil;
	return nil;
    }
    return self;
}

/* 

I favor left shifting because there is a truncation of 8 bits
when going to the dacs.  If you are going straight through and don't
lef t shift, you lose the 8 bits.  I think it is better to assume that
no scaling is going to take place.

As for leaving headroom, I prefer to deal with scaling down when
necessary, rather than scaling up.  This means you are generally
always working in the high order bits by default, leading to naturally
better S/N on output.

Probably we should make it optional (with shifting as the default).
- Mike Mcnabb

It should not be necessary to left-shift 8 bits to convert 16 to 24.  We
download 16-bit data by really writing two bytes per word.  If it really
will be treated as 24 bits, then you have your choice.  Left-shifting is
reasonable (maxamp in 16 => maxamp in 24) but it leaves you no dynamic
range for growth.  If you don't left-shift, you do have to sign extend.
-Julius

Since the Samples object has a scale factor argument in its
fileTableLength:scale: method, you can always 'get there from here'.
Therefore, we can pick one or the other method and anyone can get the
desired effect by scaling. So it's a flip of a coin. I'm currently
doing the shift so this is how I'm leaving it, unless I get argued out
of it.
- David Jaffe

*/

- fillTableLength:(int)aLength scale:(double)aScaling 
/* This method fulfills the subclass responsibility required by WaveTable.
   You ordinarily don't send this message yourself.

   Recopies the data from the sound into the dataDSP buffer (defined
   in WaveTable), scaling or normalizing (if aScaling = 0) and resets 
   dataDouble.

   It is currently an error to ask for a length other than an integral
   division of the original data length. If the requested length is an
   integral division, the appropriate number of samples are skipped when
   copying the data into the data buffer.
   Returns self or nil if there's a problem. */
{
    /*** FIXME Eventually allow double and other format Sounds and avoid
      losing precision in this case. ***/
    int originalLength, inc;
    short *data,*end;
    DSPDatum  *newData;
    if (!sound)
      return nil;
    originalLength = [sound sampleCount];
    if (aLength == 0)
      aLength = originalLength;
    inc = originalLength/aLength;
    if (inc * aLength != originalLength) /* Divided evenly */
      return _MKErrorf(MK_samplesNoResampleErr);
    /* The above allows us to down-sample a waveform. If the Sound's size is an
       multiple of the desired length, we can do cheap sampling rate 
       conversion. */

    if (dataDSP) 
      NX_FREE(dataDSP);
    _MK_MALLOC(dataDSP, DSPDatum, aLength);
    if (dataDouble) {
	NX_FREE(dataDouble); 
	dataDouble = NULL;
    }
    length = aLength;
    scaling = aScaling;
    data = (short *)[sound data]; /* Bad cast for RISC architecture */
    end = data + originalLength;
    /* We only compute dataDouble here if scaling is not 1.0. The point is
       that if we have to do scaling, we might as well compute dataDouble
       while we're at it, since we have to do mutliplies anyway.  On the
       other hand, if scaling is 1.0, we don't bother with dataDouble here
       and only create it in superclass on demand. */
    if (scaling == 1.0) {
	newData = dataDSP;
	while (data < end) {
	  *newData++ = (((int) *data) << 8); /* Coercion to int does sign
						extension. */
	  data += inc;
	}
    }
    else {
	double scaler;
	register double *dbl;
	newData = dataDSP;
    	if (aScaling == 0.0) {
	    /* if normalizing, find the maximum amplitude. */
	    short val, maxval;
    	    maxval = 0;   
    	    while (data < end) {
		val = *data;
	        val = (val < 0) ? -val : val;        /* Abs value */
		data += inc;
    	        if (val > maxval) 
		  maxval = val; 
	    }	
	    if (maxval > 24573) {  /* Don't bother if we're close */
		/* 24573 is (0x7fff * .75) */
	        data = (short *)[sound data]; /* Bad cast for RISC architecture */
		while (data < end) {         /* Same as above. */
		    *newData++ = (((int) *data) << 8); 
		    data += inc;
		}
		return self;
	    }
    	    scaler = (1.0 / (double)maxval);  
	}
        else scaler = scaling / (double)(0x7fff); 

	/* This should be rewritten to use fixed point arithmetic and to not
	   fill dataDouble. FIXME */
    	data = (short *)[sound data];
	_MK_MALLOC(dataDouble, double, aLength);
	dbl = dataDouble;
        while (data < end) {
	    *dbl = (*data) * scaler; 
	    data += inc;
    	    *newData++ = _MKDoubleToFix24(*dbl++);
	}
    }
    return self;
}

- _writeScorefileStream:(NXStream *)aStream binary:(BOOL)isBinary
{
    if (isBinary)
      _MKWriteChar(aStream,'\1'); /* Marks it as Sample rather than Partials */
    if (!soundfile) {
	/* Generate file name and write samples. */  
	char * root;
	char * s;
	int fd,i;
	i = 0;
	root = "samples";
	soundfile =  _MKMakeStrcat(root,".snd");
	for (;;) {                         /* Keep trying until success */
	    if ((fd = open(soundfile,O_RDONLY,_MK_PERMS)) == -1) 
	      break;                       /* File doesn't exist. */
	    NX_FREE(soundfile);
	    s = _MKMakeStrcat(root,_MKIntToStringNoCopy(++i));
	    soundfile = _MKMakeStrcat(s,".snd");
	    NX_FREE(s);
	}
	if ([sound writeSoundfile:soundfile] != SND_ERR_NONE) { 
	    NX_FREE(soundfile);                       /* True lossage */
	    soundfile = NULL;
	    /*** FIXME Print sound error code here. ***/ 
	    if (isBinary)
	      _MKWriteString(aStream,"/dev/null");
	    else NXPrintf(aStream,"{\"/dev/null\"}");      /* Not very good */
	    return nil;
	}
    }
    _MKWriteString(aStream,soundfile);
    return self;
}

- writeScorefileStream:(NXStream *)aStream
/* This method is used by the Music Kit to reference the receiver in 
   a scorefile. There are two cases, depending whether the Sound was set
   with readSoundfile: or setSound:.

   If the data was set with readSoundfile, just writes the file name in the
   scorefile. 

   If the data was set with setSound:, generates a new soundfile name of 
   the form "samples<number>.snd",
   writes that soundfile name to the scorefile, and writes a
   soundfile with the auto-generated name on the current directory.
   The soundfile name is guaranteed to be unique on the current directory.
   Also remembers that the file's been written and what its name is so that 
   future references to the Samples object use the same name and don't 
   rewrite the file.

   */
{
    return [self _writeScorefileStream:aStream binary:NO];
}

- _writeBinaryScorefileStream:(NXStream *)aStream
{
    return [self _writeScorefileStream:aStream binary:YES];
}

- sound
/* Returns the Sound object. */
{
    return sound;
}

-(char *) soundfile
/* Returns the name of the soundfile from which the data was 
   obtained, if any. The receiver should not alter this string. 
   If the data was obtained using setSound:,
   returns a NULL. 
   */
{
    return soundfile;
}


@end

