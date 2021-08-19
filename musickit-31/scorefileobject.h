/* 
    scorefileobject.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */
/* This file describes an abstract interface for supplying your own Objects
   to be read/written from/to Scorefiles. 

   The object may be of any class, but must be able to write itself
   out in ASCII when sent the message -writeASCIIStream:.
   It may write itself any way it wants, as long as it can also read
   itself when sent the message -readASCIIStream:.
   The only restriction on these methods is that the ASCII representation
   should not contain the character ']'.
  */

#import <objc/Object.h>
@interface scorefileObject:Object 
-readASCIIStream:(NXStream *)aStream;
-writeASCIIStream:(NXStream *)aStream;
@end



