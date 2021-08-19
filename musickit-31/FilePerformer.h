/*
  FilePerformer.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
*/

#import "Performer.h"

@interface FilePerformer : Performer
/* 
 *
 * During a Music Kit performance, a FilePerformer reads and performs 
 * time-ordered music data from a file on the disk.  An abstract class, 
 * FilePerformer provides common functionality and declares subclass 
 * responsibilities for its two subclasses, MidifilePerformer and 
 * ScorefilePerformer.  
 * 
 * A FilePerformer is associated with a file either by the file's name or 
 * through a stream pointer (NXStream).  If you assoicate a FilePerformer 
 * with a file name (through the setFile: method) the object opens and
 * closes the file for you:  The file is opened for reading when the 
 * FilePerformer receives the activate message and closed when it receives
 * deactivate.  The setFileStream: method associates a FilePerformer
 * with a stream pointer.  In this case, opening and closing the file is the
 * responsibility of the application.  The FilePerformer's stream pointer is 
 * set to NULL after each performance so you must send another
 * setFileStream: message in order to replay the file.  Any number of
 * FilePerformers can perform the same file concurrently.
 * 
 * The FilePerformer class declares two methods as subclass responsibilities:
 * nextNote and performNote:.  nextNote is subclassed to access
 * the next line of information in the file and from it create either a Note
 * object or a timeTag value (for the following Note).  It returns the Note 
 * that it creates, or, in the case of a timeTag, it sets the instance 
 * variable fileTime to represent the current time in the file and returns 
 * nil. performNote: should perform any desired manipulations on the Note 
 * created by nextNote and then pass it as the argument to sendNote: (sent to
 * a NoteSender).  The value returned by performNote: is ignored.
 *
 * FilePerformer defines two timing variables, firstTimeTag and
 * lastTimeTag.  They represent the smallest and largest timeTag values that
 * are considered for performance:  Notes with timeTags that are less than
 * firstTimeTag are ignored; if nextNote creates a timeTag greater
 * than lastTimeTag, the FilePerformer is deactivated. 
 * 
 * A FilePerformer reads and performs one Note at a time.  This allows 
 * efficient performance of arbitrarily large files.  Compare this to the 
 * Score object which reads and processes the entire file before performing 
 * the first Note.  However, unlike the Score object, the FilePerformer 
 * doesn't allow individual timing control over the different Parts 
 * represented in the file; timeShift and duration (inherited from Performer)
 * are applied to the entire file.
 * 
 * Creation of a FilePerformer's NoteSender is left to the subclass.
 * 
 * CF: ScorefilePerformer, Performer 
 */
{
    char *filename;      /* The object's file name, if set. */
    double fileTime;     /* The current time in the file, in beats. */
    NXStream *stream;    /* The object's stream pointer. */
    double firstTimeTag; /* The smallest timeTag value considered for 
                            performance. */
    double lastTimeTag;  /* The largest timeTag value considered for 
                            performance. */
    void *_reservedFilePerformer1;
}
 
 /* METHOD TYPES
  * Creating a FilePerformer
  * Modifying the object
  * Querying the object
  * Performing
  */

- init;
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver by setting both its stream pointer and its 
  * filename to NULL.  You never invoke this method directly.  A subclass 
  * implementation should send [super init] before performing its own 
  * initialization. The return value is ignored.  
  */

-copyFromZone:(NXZone *)zone;
 /* TYPE: Creating; Creates and returns a copy of the receiver.
  * Creates and returns a FilePerformer that contains a copy of the receiver's
  * filename.  The new object's stream pointer is set to NULL.  The new
  * object is given an empty list of NoteSenders. See superclass copy.
  */

- setFile:(char *)aName; 
 /* TYPE: Modifying; Associates the receiver with file aName.
  * Associates the receiver with the file named aName.  The file is opened
  * when the receiver is activated and closed when its deactivated.  If the
  * receiver is active, does nothing and returns nil, otherwise returns the
  * receiver.  
  */

- setStream:(NXStream *)aStream;
 /* TYPE: Modifying; Sets the receiver's stream pointer to aStream.
  * Sets the receiver's stream pointer to aStream.  You must open and
  * close the stream yourself.  If the receiver is active, this does
  * nothing and returns nil, otherwise returns the receiver. 
  */

-(NXStream *) stream; 
 /* TYPE: Querying; Returns the receiver's stream pointer.
  * Returns the receiver's stream pointer, or NULL if it isn't set.
  */

-(char *) file; 
 /* TYPE: Querying; Returns the name set through setFile:.
  * Returns the receiver's file name, if any.
  */

- activateSelf; 

 /* TYPE: Performing; Prepares the reciever for a performance.
  * Prepares the receiver for a performance by opening the associated file
  * (if necessary) and invoking nextNote until it returns an appropriate
  * Note -- one with a timeTag between firstTimeTag and lastTimeTag,
  * inclusive.  If an appropriate Note isn't found, [self deactivateSelf]
  * is sent.  You never invoke this method; its invoked by the activate
  * method inherited from Performer.  
  */

+(char *)fileExtension;
 /* TYPE: Querying; Returns the default file extension; defined in subclasses.
  * Returns the file name extension for the class.  The default
  * implementation returns NULL.  A subclass may override this method to
  * specify its own file extension.  
  */

+(char **)fileExtensions;
 /* This method is used when several file extensions must be handled.
  * The value returned by this method is a pointer to a null-terminated
  * array of strings, each of which is a valid file extension for files
  * handled by the subclass.  Subclass may override this to specify a
  * default file extension.  The default implementation returns an array
  * with one element equal to [self fileExtension]. */

- perform; 
 /* TYPE: Performing; Gets a Note from the receiver's file and performs it.
  * Gets the next Note from the receiver's file by invoking nextNote, passes
  * it as the argument to performNote:, then sets the value of
  * nextPerform.  You never invoke this method; it's invoked by the
  * receiver's Conductor.  The return value is ignored.  
  */

- performNote:aNote; 
 /* TYPE: Performing; Performs aNote; subclass responsibility.
  * A subclass responsibility expected to manipulate and send aNote which
  * was presumably just read from a file.  You never invoke this method;
  * it's invoked automatically by the perform method.  The return type is
  * ignored.  */

- nextNote; 
 /* TYPE: Performing; Gets next Note from the file; subclass responsibility.
  * A subclass responsibility expected to get the next Note from the file.
  * It should return the Note or nil if the next file entry is a
  * performance time.  In the latter case, fileTime should be updated.
  * You never invoke this method; it's invoked automatically by the
  * perform method.  */

- initializeFile; 
 /* TYPE: Performing; Prepares the file for a performance.
  * A subclass can implement this method to perform any special file
  * initialization.  If nil is returned, the receiver is deactivated.  You
  * never invoke this method; it's invoked automatically by activateSelf.
  * The default implementation does nothing and returns the receiver.  */

- deactivateSelf; 
 /* TYPE: Performing; Cleans up after a performance.
  * Invokes finishFile, closes the receiver's file (if it was set through
  * setFile:), and sets its file pointer to NULL.  You never invoke this
  * method; its invoked automatically when the receiver is deactivated.
  */

- finishFile; 
 /* TYPE: Performing; Wraps up the file after a performance.
  * A subclass can implement this method for post-performance file
  * operations.  You shouldn't close the stream pointer as part of this
  * method.  You never invoke this method; it's invoked automatically by
  * deactivateSelf.  The default implementation does nothing.  The return
  * value is ignored.  */

- setFirstTimeTag:(double)aTimeTag; 
 /* TYPE: Modifying; Sets firstTimeTag to aTimeTag
  * Sets the smallest timeTag considered for performance
  * to aTimeTag.  Returns the receiver.
  * If the receiver is active, does nothing and returns nil.
  */

- setLastTimeTag:(double)aTimeTag; 
 /* TYPE: Modifying; Sets lastTimeTag to aTimeTag
  * Sets the largest timeTag considered for performance
  * to aTimeTag.  Returns the receiver.
  * If the receiver is active, does nothing and returns nil.
  */

-(double) firstTimeTag; 
 /* TYPE: Querying; Returns the receiver's firstTimeTag value. */

-(double) lastTimeTag; 
 /* TYPE: Querying; Returns the receiver's lastTimeTag value. */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes object.
  * You never send this message directly.  
  * Should be invoked with NXWriteRootObject(). 
  * Invokes superclass write: method. Then archives filename, 
  * firstTimeTag, and lastTimeTag. 
  */

- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object.
  * You never send this message directly.  
  * Should be invoked via NXReadObject(). 
  * Note that -init is not sent to newly unarchived objects.
  * See write:. */

@end



