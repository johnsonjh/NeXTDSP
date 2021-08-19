/*
	Copyright 1987, NeXT Inc.
	private musickit include file.
	This file contains everything used by the Music Kit privately.
	It should probably be broken up for compilation efficiency.
	
*/

/* 
Modification history:

  09/15/89/daj - Added caching of Note class. (_MKClassNote())
  09/22/89/daj - Moved _MKNameTable functions to _MKNameTable.h.
  10/08/89/daj - Changed types for new _MKNameTable implementation.
  11/10/89/daj - Added caching of Partials class. (_MKClassPartials())
  11/26/89/daj - Added _MKBeginUGBlock() and _MKEndUGBlock().
  11/27/89/daj - Removed arg from _MKCurSample.
  12/3/89/daj - Added seed and ranSeed tokens.
  12/22/89/daj - Removed uPlus
  01/08/90/daj - Added name arg to _MKNewScoreInStruct().
  02/26/90/daj - Changes to accomodate new way of doing midiFiles. 
                 Added midifile sys excl support.
  03/05/90/daj - Added macros for escape characters.
   3/06/90/daj - Added _MK_repeat to token list.
   3/13/90/daj - Removed _privatemsgs.h because it doesn't work with the new 
                 compiler. Changed all classes to use catagories instead.
		 Moved many declarations from this file to individual private
		 .h files.
   4/21/90/daj - Added macro _MK_MAKECOMPILERHAPPY to surpress warnings
                 that are unnecessary.
   4/23/90/daj - Moved much of this file to individual .h files and renamed
                 the file _utilities.h
		 The way you now use it is this:
		 First import _musickit.h. This imports musickit.h.
		 Then import any special _*.h files you need.
   7/24/90/daj - Added _MKDisableErrorStream to protect multi-threaded 
                 performance. 
   9/26/90/daj - Changed *cvtToId to objc_getClassWithoutWarning
*/

#ifndef _MKUTILITIES_H
#define _MKUTILITIES_H

#import <sys/file.h>
#import <appkit/nextstd.h>
#import <stdarg.h> 
#import <midi/midi_types.h>
#import "musickit.h"

#define _MK_MAKECOMPILERHAPPY 1

/* These are used to see if a class is loaded */ 
/* These are used to avoid going through the findClass hash every time */

typedef struct __MKClassLoaded { 
    id aClass;
    BOOL alreadyChecked;
} _MKClassLoaded;

#define _MK_GLOBAL

extern _MK_GLOBAL _MKClassLoaded _MKNoteClass;
extern _MK_GLOBAL _MKClassLoaded _MKOrchestraClass;
extern _MK_GLOBAL _MKClassLoaded _MKWaveTableClass;
extern _MK_GLOBAL _MKClassLoaded _MKEnvelopeClass;
extern _MK_GLOBAL _MKClassLoaded _MKSamplesClass;
extern _MK_GLOBAL _MKClassLoaded _MKPartialsClass;
extern _MK_GLOBAL _MKClassLoaded _MKConductorClass;

extern id _MKCheckClassNote() ;
extern id _MKCheckClassOrchestra() ;
extern id _MKCheckClassWaveTable() ;
extern id _MKCheckClassEnvelope() ;
extern id _MKCheckClassSamples();
extern id _MKCheckClassPartials();
extern id _MKCheckClassConductor();

#define _MKClassNote() \
  ((_MKNoteClass.alreadyChecked) ? _MKNoteClass.aClass : \
  _MKCheckClassNote())

#define _MKClassOrchestra() \
  ((_MKOrchestraClass.alreadyChecked) ? _MKOrchestraClass.aClass : \
  _MKCheckClassOrchestra())

#define _MKClassWaveTable() \
  ((_MKWaveTableClass.alreadyChecked) ? _MKWaveTableClass.aClass : \
  _MKCheckClassWaveTable())

#define _MKClassEnvelope() \
  ((_MKEnvelopeClass.alreadyChecked) ? _MKEnvelopeClass.aClass : \
  _MKCheckClassEnvelope())

#define _MKClassSamples() \
  ((_MKSamplesClass.alreadyChecked) ? _MKSamplesClass.aClass : \
  _MKCheckClassSamples())

#define _MKClassPartials() \
  ((_MKPartialsClass.alreadyChecked) ? _MKPartialsClass.aClass : \
  _MKCheckClassPartials())

#define _MKClassConductor() \
  ((_MKConductorClass.alreadyChecked) ? _MKConductorClass.aClass : \
  _MKCheckClassConductor())

extern void _MKLinkUnreferencedClasses();
extern BOOL _MKInheritsFrom(id aFactObj,id superObj);

#define _MKCopyList(_x) [_x copy]

#define BACKSLASH '\\'
#define BACKSPACE '\b'
#define FORMFEED '\f'
#define CR '\r'
#define TAB '\t'
#define NEWLINE '\n'
#define QUOTE '\''
#define VT '\v'

#define _MK_TINYTIME ((double)1.0e-05) /* Must be less than 1/2 a tick. */

#define _MK_LINEBREAKS 0 /* No line breaks within envelopes or notes. */

#define _MK_PERMS 0664 /* RW for owner and group. R for others */ 

#define _MK_DPSPRIORITY 30 /* Almost maximum. Display Postscript priority */

/* Initialization of musickit */
extern void _MKCheckInit();

#import <objc/vectors.h>

extern id objc_getClassWithoutWarning(char *arg);
/* The following finds the class or nil if its not there. */
#define _MK_FINDCLASS(_x) objc_getClassWithoutWarning(_x)
/* Might want to change this to the following: */
// #define _MK_FINDCLASS(_x) ([Object findClass:_x])

/* String functions */
char *_MKMakeStr();
char *_MKMakeStrcat();
char *_MKMakeSubstr();
char *_MKMakeStrRealloc();

/* Conversion */
extern double _MKStringToDouble(char * sVal);
extern int _MKStringToInt(char * sVal);
extern char * _MKDoubleToString(double dVal);
extern char * _MKIntToString(int iVal);
extern char * _MKDoubleToStringNoCopy(double dVal);
extern char * _MKIntToStringNoCopy(int iVal); 
/* See /usr/include/dsp/dsp.h, imported by musickit.h */
extern DSPFix24 _MKDoubleToFix24(double dval);
extern double _MKFix24ToDouble(DSPFix24 ival);
extern int _MKFix24ToInt(DSPFix24 ival);

/* Files */
extern NXStream *_MKOpenFileStream(char * fileName,int *fd,int readOrWrite,
				   char *defaultExtension,BOOL raiseError);

/* Floating point resoulution */
#define _MK_VARRESOLUTION (((double)1.0/(double)44000.0)/(double)2.0)

/* For debugging */
extern void _MKOrchTrace(id orch,int typeOfInfo,char * fmt, ...);
extern unsigned _MKTraceFlag;
#define _MKTrace() _MKTraceFlag

/* Memory alloc */
extern char * _MKMalloc(); /* These will be replaced with NeXT equiv*/
extern char * _MKCalloc(); 
extern char * _MKRealloc();
#define  _MK_MALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) _MKMalloc( (unsigned)(NUM)*sizeof(TYPE) )) 
#define  _MK_REALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) _MKRealloc((char *)(VAR), (unsigned)(NUM)*sizeof(TYPE)))
#define  _MK_CALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) _MKCalloc( (unsigned)(NUM),sizeof(TYPE) )) 

/* For multi-threaded MK performance. */
extern void _MKDisableErrorStream(void);
extern void _MKEnableErrorStream(void);


#endif _MKUTILITIES_H


























