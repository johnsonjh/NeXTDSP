/*
  Samples.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import "WaveTable.h"

@interface Samples : WaveTable
/*
 * 
 * A Samples object is a type of WaveTable that uses a Sound object (from
 * the Sound Kit) as its data.  The Sound itself can only contain sampled
 * data; each sample in the Sound corresponds to an array entry in the
 * Samples object.  The Sound object can be set directly by invoking the
 * method setSound:, or you can read it from a soundfile, through the
 * readSoundfile: method.
 * 
 * Note that the Samples object currently does not resample (except in
 * one special case, when the sound is evenly divisible by the access
 * length). Except in this special case, the length of the sound must be
 * the same as the length you ask for with the access methods.
 * 
 * Note also that for use with the Music Kit oscillator unit generators,
 * the length must be a power of 2 and must fit in DSP memory (1024 or
 * less is a good length).
 */
{
    id sound;        /* The object's Sound object. */ 
    char *soundfile; /* The name of the soundfile, if the Sound was set
                        through readSoundfile:. */
}

 /* METHOD TYPES
  * Creating and freeing a Samples
  * Modifying the object
  * Querying the object
  */

- init;
 /* TYPE: Modifying; Inits the receiver.
  * Sent automatically when the receiver is created, you can also invoke
  * this method to reset a Samples object.  Sets the receiver's sound
  * variable to nil and soundfile to NULL.  The receiver's previous Sound
  * object, if any, is freed.  A subclass implementation should send
  * [super init].  Returns the receiver.  */

- free;
 /* TYPE: Creating; Frees the receiver.  Frees the receiver and its Sound.
  */

- copyFromZone:(NXZone *)zone;
 /* TYPE: Creating; Creates and returns a Samples object as a copy of the receiver.
  * Creates and returns a new Samples object as a copy of the receiver.  The 
  * receiver's Sound is copied into the new Samples.
  * See also superclass -copy.
  */

- setSound:aSound; 
 /* TYPE: Modifying; Sets the receiver's Sound to a copy of aSound.
  * Sets the receiver's Sound to a copy of aSound (the receiver's current
  * Sound is freed).  aSound must be one-channel, 16-bit linear data.  You
  * shouldn't free the Sound yourself; it's automatically freed when the
  * receiver is freed, initialized, or when a subsequent Sound is set.
  * Returns nil if aSound is in the wrong format, otherwise frees the
  * receiver.  */

- readSoundfile:(char *)aSoundfile;
 /* TYPE: Modifying; Sets the receiver's Sound to the data in aSoundfile.
  * Creates a new Sound object, reads the data from aSoundfile into the
  * object, and then sends setSound: to the receiver with the new Sound as
  * the argument.  You shouldn't free the Sound yourself; it's
  * automatically freed when the receiver is freed, initialized, or when a
  * subsequent Sound is set.  Returns nil if the setSound: message returns
  * nil; otherwise returns the receiver.  */

- sound;
 /* TYPE: Querying; Returns the receiver's Sound object. */

- (char *) soundfile;
 /* TYPE: Querying; Returns the name of the receiver's soundfile, if any.
  * Returns the name of the receiver's soundfile, or NULL if the
  * receiver's Sound wasn't set through readSoundfile:.  The name isn't
  * copied; you shouldn't alter the returned string.  */

- fillTableLength:(int)aLength scale:(double)aScaling ;
 /* TYPE: Modifying; 
  * Copies aLength samples from the receiver's Sound into dataDSP
  * (inherited from WaveTable) and scales the copied data by multiplying it by
  * aScaling.  If aScaling is 0, the data is normalized (it's sized to
  * fit). dataDouble (also from WaveTable) is reset.
  * You ordinarily don't invoke this method; it's invoked from methods
  * defined in WaveTable.
  * Returns self or nil if there's a problem. 

   << Currently, aLength must be the full size of the Sound. >> 
  */

- writeScorefileStream:(NXStream *)aStream;
 /* TYPE: Modifying
  * Writes the receiver in scorefile format to the stream aStream.  A
  * Samples object is written by the name of the soundfile from which its
  * Sound was read, surrounded by braces:
  * 
  *   { "soundfileName" }
  * 
  * If the Sound wasn't set from a soundfile, a soundfile with the
  * unique name "samplesNumber.snd" (where number is added only if
  * needed), is created and the Sound is written to it.  The object
  * remembers if its Sound has been written to a soundfile.  If the
  * receiver couldn't be written to the stream, returns nil, otherwise
  * returns the receiver.
  * 
  */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Write object.
     Archives object by archiving filename and sound object. Note that the
     sound object is archived whether it was created from readSoundfile:
     or from setSound:. We assume that the sound, even if it comes from
     an external source, is an intrinsic part of the object. 
     You never send this message directly.  */

- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Read object.
     Note that -init is not sent to newly unarchived objects.
     You never send this message directly.  */

@end




