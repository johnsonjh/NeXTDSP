/*
  ScorefilePerformer.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
  */

#import "FilePerformer.h"

@interface ScorefilePerformer : FilePerformer
/* 
 * ScorefilePerformers are used to perform scorefiles.  When the object
 * is activated, it reads the file's header and creates a NoteSender for
 * each (unique) member of the part statement.  A NoteSender is given the
 * same name as the Parts for which it was created.
 * 
 * A ScorefilePerformer also has an info Note which it fashions from the
 * info statement in the file and defines a stream on which scorefile
 * print statements are printed.
 * 
 * During a performance, a ScorefilePerformer reads successive Note and
 * time statements from the file.  When it reaches the end of the file,
 * the ScorefilePerformer is deactivated.
 */
{
    NXStream *scorefilePrintStream; /* The stream used for 
                                the scorefile's print
                                statements.*/
    id info; /* Score info Note for the file. */
    void *_reservedScorefilePerformer1;
    id _reservedScorefilePerformer2;
}
 
 /* METHOD TYPES
  * Creating a ScorefilePerformer
  * Modfiying the object
  * Querying the object
  * Performing the object
  */

- init;
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver.  You never invoke this method directly.  A
  * subclass implementation should send [super initialize] before
  * performing its own initialization.  The return value is ignored.
  */

+(char *)fileExtension;
 /* TYPE: Querying;
  * Returns a pointer to ".score", the default soundfile extension.
  * The string isn't copied.
  */

+(char **)fileExtensions;
 /* Returns a null-terminated array of the default file extensions
  * recognized by ScorefilePerformer instances. This array consists of
  * "score" and "playscore".  This method is used by the FilePerformer
  * class.  The string is not copied. */

- info;
 /* TYPE: Querying;
  * Returns the receiver's info Note, fashioned from an info statement
  * in the header of the scorefile.
  */

- initializeFile; 
 /* TYPE: Performing
  * You never invoke this method; it's invoked automatically by
  * selfActivate (just before the file is performed).  It reads the
  * scorefile header and creates NoteSender objects for each member of the
  * file's part statements.  It also creates info Notes from the file's
  * Score and Part info statements and adds them to itself and its Parts.
  * If the file can't be read, or the scorefile parser encounters too many
  * errors, the receiver is deactivated.
  */

- finishFile; 
 /* TYPE: Peforming
  * You never invoke this method; it's invoked automatically by
  * deactivateSelf.  Performs post-performance cleanup of the scorefile
  * parser.
  */

-(NXStream *)scorefilePrintStream;
 /* TYPE: Querying
  * Returns the receiver's scorefile print statement stream.
  */

- nextNote; 
 /* TYPE: Performing
  * Reads the next Note or time statement from the body of the scorefile.
  * Note statements are turned into Note objects and returned.  If its a
  * time statement that's read, fileTime is set to the statement's value
  * and nil is returned.
  * 
  * You never invoke this method; it's invoked automatically by the
  * perform method.  If you override this method, you must send 
  * [super nextNote].
  */

- infoForNoteSender:aNoteSender; 
 /* TYPE: Querying
  * Returns the info Note of the Part associated with the 
  * NoteSender aNoteSender.  If aNoteSender isn't 
  * a contained in the receiver, returns nil.
  */

- performNote:aNote; 
 /* TYPE: Performing;
  * Sends aNote to the appropriate NoteSender
  * You never send performNote: directly to a ScorefilePerformer;
  * it's invoked by the perform method.  
  */

-free;
 /* TYPE: Creating;
  * Frees the receiver, its NoteSenders, and its info Note.  If the
  * receiver is active, this does nothing and returns self. Otherwise,
  * returns nil
  */

- copyFromZone:(NXZone *)zone;
 /* TYPE: Creating;
  * Creates and returns a new ScorefilePerformer as a copy of the
  * receiver.  The info receiver's info Note is also copied.
  */

- write:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Invokes superclass write:, which archives NoteSenders.
     Then archives info and part infos gleaned from the Scorefile. */
- read:(NXTypedStream *) aTypedStream;
  /* TYPE: Archiving; Reads object.
     You never send this message directly.  
     Should be invoked via NXReadObject(). 
     Note that -init is not sent to newly unarchived objects.
     See write:. */

@end



