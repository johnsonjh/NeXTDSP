/* 
    musickit.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */
#ifndef MUSICKIT_H
#define MUSICKIT_H

 /* Include files outside of the Music Kit. */
#import <objc/objc.h>           /* Contains nil, etc. */
#import <streams/streams.h>     /* Contains NXStream, etc. */
#import <math.h>                /* Contains MAXINT, etc. */

#import "noDVal.h"

 /* Music Kit errors. */
#import "errors.h"              /* Error codes, debug flags and functions. */

 /* Music Kit table management.
  *  
  * The Music Kit provides a simple naming mechanism.  There are 5
  * functions provided for manipulating the name of an object. These are
  * declared below.
  * 
  * Names are primarily used when reading and writing scorefiles. For
  * example, when you read a scorefile into a Score object, the Parts that
  * are created are given the names used in the file.  Similarly, when
  * performing a scorefile with a ScorefilePerformer, the NoteSenders are
  * given the part names used in the file.  Envelopes and WaveTables
  * created when reading a scorefile are also given names.
  * 
  * When writing a Score which contains Parts you created in an application,
  * you can explicitly give the Parts names.  If a name you specify is not
  * unique, or if you don't specify any name, one will be automatically
  * generated (a variant of what you supplied). Similarly, when recording to a 
  * scorefile with a ScorefileWriter, you can explicitly provide part names by 
  * naming the corresponding NoteReceivers.
  * 
  * Note that the naming mechanism allows any object, whether or not it is
  * in the Music Kit, to be named. In general, it is the Application's 
  * responsibility to remove the names before freeing the object. 
  * However, as a convenience, the following classes remove the instance name
  * when freeing the instance. Copying an object does not copy its name.
  * 
  * It's illegal to change the name of an object during a performance
  * involving a ScorefileWriter. (Because an object'll get written to the
  * file with the wrong name.) 
  */

extern BOOL MKNameObject(char * name,id object);
 /*
  * Adds the object theObject in the table, with name theName.
  * If the object is already named, does 
  * nothing and returns NO. Otherwise returns YES. Note that the name is copied.
  */

extern const char * MKGetObjectName(id object);
 /* 
  * Returns object name if any. If object is not found, returns NULL. The name
  * is not copied and should not be freed by caller.
  */

extern id MKRemoveObjectName(id object);
 /* Removes theObject from the table, if present. Returns nil. */

extern id MKGetNamedObject(char *name);
 /* Returns the first object found in the name table, with the given name.
    Note that the name is not necessarily unique in the table; there may
    be more than one object with the same name.
   */

 /* These two functions allow you to give an object a name that can be seen
  * by a scorefile. 
  */
extern BOOL MKAddGlobalScorefileObject(id object,char *name);
 /*
  * Adds the object as a global scorefile object, 
  * referenced in the scorefile with the name specified. The name is copied.
  * The object does not become visible to a scorefile unless it explicitly
  * 'imports' it by a getGlobal statement.
  * If there is already a global scorefile object with the specified name, 
  * does nothing and returns NO. Otherwise returns YES. 
  * The type of the object in the scorefile is determined as follows:
  * If object -isKindOf:WaveTable, then the type is MK_waveTable.
  * If object -isKindOf:Envelope, then the type is MK_envelope.
  * Otherwise, the type is MK_object.
  * Note that the global scorefile table is independent of the Music Kit
  * name table. Thus, an object can be named in one and unnamed in the other,
  * or it can be named differently in each.
  */

extern id MKGetGlobalScorefileObject(char *name);
 /* Returns the global scorefile object with the given name. The object may
  * be either one that was added with MKAddGlobalScorefileObject or it
  * may be one that was added from within a scorefile using "putGlobal".
  * Objects accessable to the application are those of type 
  * MK_envelope, MK_waveTable and MK_object. 
  */

extern double MKdB(double dB);          
 /* dB to amp conversion. E.g. MKdB(-60) returns ca. .001 and MKdB(0.0) returns
  * 1.0. */

/* Time functions */
extern double MKGetTime(void) ;
    /* Returns the time in seconds. In a conducted performance (the norm)
     * this is the same as [Conductor time].  */

extern double MKGetDeltaT(void);
    /* Returns the "delta time", in seconds. The meaning of delta time depends
     * on whether the performance is clocked or unclocked. In a clocked 
     * performance, the Conductor tries to stay approximately deltaT seconds 
     * ahead of the devices (e.g. DSP). In an unclocked performance, the 
     * Conductor tries to stay at least deltaT seconds ahead of the devices.
     * Delta time has an effect only if the device is in timed mode. 
     */

extern void MKSetDeltaT(double val);
 /* Sets the delta time (scheduler advance) in seconds as indicated. 
  * The delta time is the difference between the Music Kit's time and the
  * assumed time of the MIDI and DSP devices. */

extern double MKGetDeltaTTime(void);
 /* Returns the time the Music Kit thinks the device (e.g. DSP) is at.
  * This is the same as MKGetTime() + MKGetDeltaT(). 
  */

extern double MKSetTime(double newTime);
 /* This is a rarely-used function. It sets time as indicated, but 
  * has no effect (MK_NODVAL is returned -- use MKIsNoDVal() to check for it) 
  * during a conducted performance. It is provided only for non-conducted 
  * performances in which the Orchestra or Midi are in timed mode. 
  */

extern void MKFinishPerformance(void);
 /* This is a rarely-used function.
  * If the performance is conducted (the norm), this is the same as 
  * [Conductor finishPerformance]. Otherwise, it tells Performers and 
  * Instruments the  performance is over. Precisely, it evaluates the 
  * Conductor's "after performance" queue. <<However, note that in the  
  * current release, the Conductor must be linked in to the application; 
  * otherwise this function has no effect.>>
  */

 /* Scorefile reading and writing. */
extern void MKWritePitchNames(BOOL usePitchNames);
 /* Selects whether values of the parameters freq0 and freq are written as 
  * pitch names or as frequencies in Hz. If you write them as pitch names,
  * they are rounded to the nearest pitch. The default is NO. 
  */

extern void MKSetScorefileParseErrorAbort(int cnt);
 /* Sets the number of parser errors to abort on. To never abort,
  * pass MAXINT as the argument. To abort on the first error, pass 1 as the
  * argument. The default is 10. 
  */

 /* Control of Music Kit-created objects. */
extern BOOL MKSetNoteClass(id aNoteSubclass);
 /* When reading a scorefile, processing MIDI, etc., the Music Kit creates
  * Note objects. Use MKSetNoteClass() to substitute your own Note subclass.
  * Returns YES if aNoteSubclass is a subclass of Note. Otherwise returns
  * NO and does nothing. This function does not effect objects returned
  * by [Note new]; these are instances of the Note class, as usual. 
  */

extern id MKGetNoteClass(void);
 /* Returns class set with MKSetNoteClass() or [Note class] if none. */

 /* The following are similar to MKSetNoteClass() and MKGetNoteClass() for
  * other Music Kit classes. */
extern BOOL MKSetPartClass(id aPartSubclass);
extern BOOL MKSetEnvelopeClass(id anEnvelopeSubclass);
extern BOOL MKSetPartialsClass(id aPartialsSubclass);
extern BOOL MKSetSamplesClass(id aSamplesSubclass);
extern id MKGetPartClass(void);
extern id MKGetEnvelopeClass(void);
extern id MKGetPartialsClass(void);
extern id MKGetSamplesClass(void);

 /* Control of voice preemption in DSP synthesis. */
extern double MKGetPreemptDuration(void);
 /* Used to get preemption time used in DSP synththesis. 
  * Default is .006 seconds. */

extern void MKSetPreemptDuration(double seconds);
 /* Used to set preemption time used in DSP synthesis. */
 
#define MK_ENDOFTIME (6000000000.0) /* A long time, but not as long as 
                                       NX_FOREVER */
 
 /*
  * The following magic number appears as the first 4 bytes of the optimized 
  * scorefile (".playscore" file extension). It is used for type checking and 
  * byte ordering information.
  */
#define MK_SCOREMAGIC ((int)0x2e706c61)

/* Music Kit classes. */
#import "Conductor.h"
#import "Envelope.h"
#import "FilePerformer.h"
#import "FileWriter.h"
#import "Instrument.h"
#import "Midi.h"
#import "Note.h"
#import "NoteFilter.h"
#import "NoteReceiver.h"
#import "NoteSender.h"
#import "Orchestra.h"
#import "Part.h"
#import "PartPerformer.h"
#import "PartRecorder.h"
#import "PatchTemplate.h"
#import "Partials.h"
#import "Performer.h"
#import "Samples.h"
#import "Score.h"
#import "ScorePerformer.h"
#import "ScoreRecorder.h"
#import "ScorefilePerformer.h"
#import "ScorefileWriter.h"
#import "SynthData.h"
#import "SynthInstrument.h"
#import "SynthPatch.h"
#import "TuningSystem.h"
#import "UnitGenerator.h"
#import "WaveTable.h"
#import "scorefileobject.h"
#endif MUSICKIT_H



