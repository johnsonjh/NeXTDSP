/*
  ScorefileWriter.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
  */

#import "FileWriter.h"

@interface ScorefileWriter : FileWriter
/*  
 * 
 * A ScorefileWriter is an Instrument that realizes Notes by writing
 * them to a scorefile.  Each of the receiver's NoteReceivers 
 * correspond to a Part that will appear in the scorefile.
 * Unlike most Instruments, the ScorefileWriter class doesn't add
 * any NoteReceivers to a newly created object, they must be added by 
 * invoking the addNoteReceiver:. method. 
 * 
 * The names of the Parts represented in the scorefile are taken from the
 * NoteRecievers for which they were created.  You can name a NoteReceiver by
 * calling the MKNameObject() function.
 * 
 * The header of the scorefile always includes a part statement naming the
 * Parts represented in the Score, and a tagRange statement, outlining the
 * range of noteTag values used in the Note statements.
 * 
 * You shouldn't change the name of a data object (such as an
 * Envelope, WaveTable, or NoteReceiver) during a performance involving a
 * ScorefileWriter.
 */
{
    id info; /* The info Note to be written to the file. */
    int _reservedScorefileWriter1;
    int _reservedScorefileWriter2;
    BOOL _reservedScorefileWriter3;
    void *_reservedScorefileWriter4;
}
 
+(char *)fileExtension;
 /* TYPE: Querying; Returns "score", the default file extension for
  * scorefiles.  The string isn't copied.  Note: This method is superceded
  * by the instance method by the same name.  */

-(char *)fileExtension;
 /* TYPE: Querying;
  * Returns "score", the default file extension for score files if the
  * file was set with setFile: or setStream:. Returns "playscore", the
  * default file extension for optimized format score files if the file was
  * set with setOptimizedFile: or setOptimizedStream:.  The string is not
  * copied. */

-setInfo:aNote;
 /* TYPE: Modifying
  * Sets the receiver's info Note, freeing a previously set info Note, if any. 
  * The Note is written, in the scorefile, as an info statement.
  * Returns the receiver.
  */

- info;
 /* TYPE: Querying;
  * Returns the receiver's info Note, as set through setInfo:.
  */

-setInfo:aPartInfo forNoteReceiver:aNoteReceiver;
 /* TYPE: Modifying;
  * Sets aNote as the Note that's written as the info Note for the
  * Part that corresponds to the NoteReceiver aNR.
  * The Part's previously set info Note, if any, is freed.
  * If the receiver is in performance, or if aNR doesn't belong
  * to the receiver, does nothing and returns nil,
  * Otherwise returns the receiver.
  */

- infoForNoteReceiver:aNoteReceiver;
 /* TYPE: Querying;
  * Returns the info Note that's associated with a NoteReceiver
  * (as set through setInfo:forNoteReceiver:).
  */

- initializeFile; 
 /* TYPE: Performing
  * Initializes the scorefile.
  * You never invoke this method; it's invoked automatically just before the
  * receiver writes its first Note to the scorefile.
  */

- finishFile; 
 /* TYPE: Performing;
  * You never invoke this method; it's invoked automatically at the end
  * of a performance. 
  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating;
  * Creates and returns a new ScorefileWriter as a copy of the receiver.
  * The new object copies the receivers NoteReceivers and info Notes.
  * See Instrument copy method.
  */

- realizeNote:aNote fromNoteReceiver:aNoteReceiver; 
 /* TYPE: Performing
  * Realizes aNote by writing it to the scorefile.  The Note statement
  * created from aNote is assigned to the Part that corresponds to
  * aNoteReceiver.
  */

-setFile:(char *)aName;
  /* Sets file and specifies that the data be written in ascii (.score) format.
     See superclass documentation for details. 
   */ 

-setStream:(NXStream *)aStream;
  /* Sets stream and specifies that the data be written in ascii (.score) 
     format. See superclass documentation for details. 
   */ 

-setOptimizedStream:(NXStream *)aStream;
  /* Same as setStream: but specifies that the data be written in optimized 
     scorefile (.playscore) format. */

-setOptimizedFile:(char *)aName;
  /* Same as setFile: but specifies that the data be written in optimized 
     (.playscore) format. */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object to archive.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write:, which archives NoteReceivers.
     Then archives info, isOptimized, and Part info Notes.  */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object from archive.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Note that -init is not sent to newly unarchived objects.
     See write:. */
- awake;
  /* TYPE: Archiving; Gets object ready for use.
     Gets object ready for use. */

@end




