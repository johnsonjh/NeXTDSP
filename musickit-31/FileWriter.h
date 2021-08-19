/*
  FileWriter.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
*/

#import "Instrument.h"
#import "timeunits.h"

@interface FileWriter : Instrument
/* 
 * A FileWriter is an Instrument that realizes Notes by writing them to a
 * file on the disk.  An abstract class, FileWriter provides common
 * functionality for the Music Kit subclasses ScorefileWriter and
 * MidifileWriter.
 * A FileWriter is associated with a file, either by the file's name
 * or through a stream pointer (NXStream *).  If you assoicate a
 * FileWriter with a file name (through the setFile: method) the object
 * opens and closes the file for you: The file is opened for writing when
 * the object first receives the realizeNote: message and closed after
 * the performance.  A FileWriter remembers its file name between
 * performances, but the file is overwritten each time it's opened.
 * 
 * The setStream: method points the FileWriter's stream instance variable
 * to the given stream pointer.  In this case, opening and closing the
 * stream is the responsibility of the application.  After each
 * performance, stream is set to NULL.
 * 
 * The subclass responsibility realizeNote:fromNoteReceiver:,
 * inherited from Instrument, is passed on to the FileWriter subclasses.
 * Two other methods, initializeFile and finishFile, can be redefined in
 * a subclass, although neither must be.  initializeFile is invoked just
 * before the first Note is written to the file and should perform any
 * special initialization such as writing a file header.  finishFile is
 * invoked after each performance and should perform any post-performance
 * cleanup.  The values returned by initializeFile and finishFile are
 * ignored.
 * 
 */
{
    MKTimeUnit timeUnit; /* Time unit used to interpret a Note's
                            timetag and duration. */
    char *filename;      /* The object's file name (if the file was set 
                            through setFile:). */
    NXStream *stream;    /* The object's stream pointer. */
    double timeShift;    /* Optional timetag value shift. */
    int _reservedFileWriter1;
}
 
/* METHOD TYPES
 * Creating a FileWriter   
 * Modifying the object
 * Querying the object
 * Performing 
 */
 
- init; 
 /* TYPE: Modifying; Initializes the receiver.
  * Initializes the receiver by setting both stream and filename to NULL.
  * You never invoke this method directly.  A subclass implementation
  * should send [super init] before performing its own
  * initialization.  The return value is ignored.  */

-setTimeUnit:(MKTimeUnit)aTimeUnit;
 /* See timeunits.h */
-(MKTimeUnit)timeUnit;
 /* See timeunits.h */

+(char *)fileExtension;
 /* TYPE: Querying; Returns the default file extension; defined in subclasses.
  * Returns the file name extension for the class.  The default
  * implementation returns NULL.  A subclass may override this method to
  * specify its own file extension.  
  */

-(char *)fileExtension;
 /* TYPE: Querying; Returns default file extension for files managed by
  * the subclass. The default implementation just invokes the
  * fileExtension method.  A subclass can override this to provide a
  * fileExtension on an instance- by-instance basis. For example
  * ScorefileWriter returns a different default extension for binary
  * format scorefiles. */

- setFile:(char *)aName; 
 /* TYPE: Modifying; Associates the receiver with the file aName.
  * Associates the receiver with the file aName.  The file is opened when
  * the first Note is realized (written to the file) and closed at the end
  * of the performance.  If the receiver is already in a performance, this
  * does nothing and returns nil, otherwise returns the receiver.  */

- setStream:(NXStream *)aStream; 
 /* TYPE: Modifying; Points the receiver's stream pointer to aStream.
  * Points the receiver's stream pointer to aStream.  You must open and
  * close the stream yourself.  If the receiver is already in a
  * performance, this does nothing and returns nil, otherwise returns the
  * receiver.  */

-(NXStream *) stream; 
 /* TYPE: Querying; Returns the receiver's stream pointer.
  * Returns the receiver's stream pointer, or NULL if it isn't set.  The
  * receiver's stream pointer is set to NULL after each performance.  */

- copyFromZone:(NXZone *)zone; 
 /* TYPE: Creating; Returns a copy of the receiver with NULL file name and stream pointer.
  * Creates and returns a copy of the receiver.  The new object's file
  * name and stream pointer are both set to NULL.  See superclass copy. */

-(char *) file; 
 /* TYPE: Querying; Returns the name set through setFile:.
  * Returns the receiver's file name, if any. 
  */

- finishFile; 
 /* TYPE: Performing; Cleans up after a performance.
  * This can be overridden by a subclass to perform post-performance
  * activities.  However, the implementation shouldn't close the
  * receiver's stream pointer.  You never send the finishFile message
  * directly to a FileWriter; it's invoked automatically after each
  * performance.  The return value is ignored.  */

- initializeFile; 
 /* TYPE: Performing; Prepares the file for writing.
  * This can be overriden by a subclass to perform file initialization, such as
  * writing a file header.  You never send the initializeFile message
  * directly to a FileWriter; it's invoked from the firstNote: method.  The
  * return value is ignored.  
  */

- firstNote:aNote; 
 /* TYPE: Performing; You never invoke this method.
  * You never invoke this method; it's invoked automatically just before the
  * receiver writes its first Note.  It opens stream to
  * the receiver's filename (if set)
  * and then sends initializeFile to the receiver.  
  */

- afterPerformance; 
 /* TYPE: Performing; You never invoke this method.
  * You never invoke this method; it's invoked automatically just after a
  * performance.  It closes the receiver's stream (if the receiver opened it
  * itself in the firstNote: method) and sets it to NULL.  
  */

- (double)timeShift;
 /* Returns value of current timeShift. */
- setTimeShift:(double)timeShift;
 /* Sets a constant value to be added to Notes' times when they are written
    out to the file. It's up to the subclass to use this value. */

- write:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Writes object to archive file.
  * You never send this message directly.  
  * Should be invoked with NXWriteRootObject(). 
  * Invokes superclass write: method. Then archives timeUnit, filename, 
  * and timeShift. 
  */
- read:(NXTypedStream *) aTypedStream;
 /* TYPE: Archiving; Reads object from archive file.
  * You never send this message directly.  
  * Note that -init is not sent to newly unarchived objects.
  * Should be invoked via NXReadObject(). 
  * See write:. */

@end



