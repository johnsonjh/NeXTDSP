#ifdef SHLIB
#include "shlib.h"
#endif

/*
  utilities.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/

/* 
Modification history:

  09/15/89/daj - Added caching of Note class. (_MKCheckClassNote())
  10/27/89/daj - Added argument to _MKOpenFileStream to surpress error msg.
  11/15/89/daj - Added caching of Partials class. (_MKCheckClassPartials())
  12/21/89/daj - Changed strstr() calls to isExtensionPresent() in 
                 _MKOpenFileStream(). This insures that the extension is the
		 final extension (i.e. it does the right thing with multiple-
		 extension files).
  01/6/90/daj - Added comments.	Flushed _MKLinkUnreferencedClasses().
  03/13/90/daj - Added "_MK_GLOBAL" to globals for 'ease of grep'.
  03/20/90/daj - Moved _MKInheritsFrom() to here.
  07/16/90/daj - Removed extra sprintfv in _MKErrorf
  07/24/90/daj - Added cthread_set_errno_self in addition to errno setting.
  07/24/90/daj - Added disabling of the error stream for multi-threaded
                 performance. 
  07/24/90/daj - Changed to use _MKSprintf and _MKVsprintf for thread-safety
                 in a multi-threaded Music Kit performance.
  09/02/90/daj - Added MKGetNoDVal() and MKIsNoDVal() 
*/

#import "_musickit.h"
#import "_error.h"
#import "_MKSprintf.h"

#import <cthreads.h>

/* This file should contain only utilities that we always want. I.e. this
   module is always loaded. */

/* globals */

_MK_GLOBAL _MKClassLoaded _MKNoteClass = {0};
_MK_GLOBAL _MKClassLoaded _MKOrchestraClass = {0};
_MK_GLOBAL _MKClassLoaded _MKWaveTableClass = {0};
_MK_GLOBAL _MKClassLoaded _MKEnvelopeClass = {0};
_MK_GLOBAL _MKClassLoaded _MKSamplesClass = {0};
_MK_GLOBAL _MKClassLoaded _MKConductorClass = {0};
_MK_GLOBAL _MKClassLoaded _MKPartialsClass = {0};
_MK_GLOBAL unsigned _MKTraceFlag = 0;

/* A dumb function that causes a reference to its arguments (to fool the
   linker.) */
void _MKLinkUnreferencedClasses()
{
}



/* The following mechanism is to make it so it's fast to check if a class
   is loaded. See the macros in _musickit.h */ 
static id checkClass(_MKClassLoaded *cl,char *className)
    /* Gets and initializes class. There are macros that only invoke
       this when the class isn't initialized yet. */
{
    cl->alreadyChecked = YES;
    cl->aClass = _MK_FINDCLASS(className);
    [cl->aClass initialize]; /* Initialize it now, not later.*/
    return cl->aClass;
}

id _MKCheckClassNote() 
{
    return checkClass(&_MKNoteClass,"Note");
}

id _MKCheckClassPartials() 
{
    return checkClass(&_MKPartialsClass,"Partials");
}

id _MKCheckClassOrchestra() 
{
    return checkClass(&_MKOrchestraClass,"Orchestra");
}

id _MKCheckClassWaveTable() 
{
    return checkClass(&_MKWaveTableClass,"WaveTable");
}

id _MKCheckClassEnvelope() 
{
    return checkClass(&_MKEnvelopeClass,"Envelope");
}

id _MKCheckClassSamples()
{
    return checkClass(&_MKSamplesClass,"Samples");
}

id _MKCheckClassConductor()
{
    return checkClass(&_MKConductorClass,"Conductor");
}


/* Music Kit malloc functions */
char *_MKCalloc(nelem, elsize)
    unsigned nelem, elsize;
{
    void *rtn;
    rtn = calloc(nelem, elsize);
    if (!rtn) {
	fprintf(stderr,"NeXT memory exausted.\n");
	exit(1);
    }
    return rtn;
}

char *_MKMalloc(size)
    unsigned size;
{
    void *rtn;
    rtn = malloc(size);
    if (!rtn) {
	fprintf(stderr,"NeXT memory exausted.\n");
	exit(1);
    }
    return rtn;
}

char *_MKRealloc(ptr,size)
    void *ptr;
    unsigned size;
{
    char *rtn;
    rtn = realloc(ptr,size);
    if (!rtn) {
	fprintf(stderr,"NeXT memory exausted.\n");
	exit(1);
    }
    return rtn;
}

/* Tracing */
/* See musickit.h for details */

unsigned MKSetTrace(int debugFlag)
    /* Set a trace bit */
{
    return (unsigned)(_MKTraceFlag |= debugFlag);
}

unsigned MKClearTrace(int debugFlag)
    /* Clear a trace bit */
{
    return (unsigned)(_MKTraceFlag &= (~debugFlag));
}

BOOL MKIsTraced(int debugFlag)
    /* Check a trace bit */
{
    return (_MKTraceFlag & debugFlag) ? YES : NO;
}

/* Error handling */
/* See musickit.h for details */

#import "mkerrors.m"

static void (*errorProc)(char * msg) = NULL;

void MKSetErrorProc(void (*errProc)(char *msg))
    /* Sets proc to be used when MKError() is called. If errProc is NULL,
       uses the default error handler, which writes to stderr. When the
       *errProc is called, errno is set to the MKErrno corresponding to err.
       errProc takes one string argument. */
{
    errorProc = errProc;
}

static NXStream *errorStream = NULL;
static NXStream *stderrStream = NULL;

NXStream *MKErrorStream(void)
    /* Returns the Music Kit error stream */
{
    return errorStream;
}

void MKSetErrorStream(NXStream *aStream) 
    /* Sets the Music Kit error stream. 
       NULL means stderr. The Music Kit initialization sets the error 
       stream to stderr. */
{
    if (aStream) {
	if (errorStream && (errorStream == stderrStream)) {
	    NXClose(stderrStream);
	    stderrStream = NULL;
	}
    }
    else if (!stderrStream) 
      aStream = (stderrStream = NXOpenFile((int)stderr->_file,NX_WRITEONLY));
    errorStream = aStream;
}

/* errno */
#import <stddef.h>  

const char * _MKGetErrStr(int errCode)
    /* Returns the error string for the given code or "unknown error" if
       the code is not a MKErrno. The string is not copied. Note that
       some of the strings have printf-style 'arguments' embeded. */
{
    /*** FIXME Eventually move _errors to an archive file to allow 
      different languages for error messages.  ***/
    const char * msg;
    if (errCode < MK_ERRORBASE || errCode > (int)MK_highestErr)
      return "unknown error";
    errno = errCode;
    cthread_set_errno_self(errCode);
    if (msg = _errors[errCode - MK_ERRORBASE]) 
      return msg;
    return "unknown error";
}

static BOOL errorStreamEnabled = YES;

void _MKDisableErrorStream(void)
{
    errorStreamEnabled = NO;
}

void _MKEnableErrorStream(void)
{
    errorStreamEnabled = YES;
}

id MKError(char * msg)
    /* Calls the user's error proc (set with MKSetErrorProc), if any, with 
       one argument, the msg. Otherwise, writes the message on the Music
       Kit error stream. Returns nil.
       */
{
    if (!msg)
      return nil;
    if (errorProc) {
	errorProc(msg);
	return nil;
    }
    else if (!errorStreamEnabled)
      return nil;
    NXPrintf(MKErrorStream(),msg);
    NXPrintf(errorStream,"\n");
    NXFlush(errorStream);
    return nil;
}

static char _errBuf[_MK_ERRLEN] = "";

char *_MKErrBuf() 
    /* Gets the private error buffer so parseScore can write right into it */
{
    return _errBuf;
}

id _MKErrorf(int errorCode,...)
    /* Calling sequence like printf, but first arg is error code. 
       It's the caller's responsibility
       that the expansion of the arguments using sprintf doesn't
       exceed the size of the error buffer (ERRLEN). Also sets errno. */
{
    const char * fmt;
    va_list ap;
    va_start(ap,errorCode);
    errno = errorCode;
    cthread_set_errno_self(errorCode);
    fmt = _MKGetErrStr(errorCode);
    if (errorProc) {
	_MKVsprintf(_errBuf,fmt,ap);
	MKError(_errBuf);
    }
    else if (!errorStreamEnabled)
      return nil;
    else {
	NXVPrintf(MKErrorStream(),fmt,ap);
	NXPrintf(errorStream,"\n");
	NXFlush(errorStream);
#if 0
	vfprintf(stderr,fmt,ap);
	fprintf(stderr,"\n");
#endif
    }
    va_end(ap);
    return nil;
}



/* Decibels */
/* See musickit.h */
double MKdB(double dbVal)
{
    /* dB to linear conversion */
    return (double) pow(10.0,dbVal/20.0);
}


/* Function to simplify file read/write of files. */

static BOOL extensionPresent(char *filename,char *extension)
    /* Similar to strstr() but looks from the back. */
{
    char *ext = strrchr(filename,'.');
    if (!ext)
      return NO;
    return (strcmp(ext,extension) == 0);
}

NXStream *_MKOpenFileStream(char * fileName,int *fd,int readOrWrite,
			    char *defaultExtension,BOOL errorMsg)
    /* The algorithm is as follows:
       For write: append the extension if it's not already there somewhere.
       For read: look up without extension. If it's no there, append
       extension and try again. */ 
{
#   define OPENFAIL(_x) (_x == -1)
    NXStream *rtnVal = NULL;
    BOOL nameChanged = NO;
    if (readOrWrite == NX_WRITEONLY) {
	if (defaultExtension) {
	    defaultExtension = _MKMakeStrcat(".",defaultExtension);
	    if (!extensionPresent(fileName,defaultExtension)) { 
		nameChanged = YES;                     
		fileName = _MKMakeStrcat(fileName,defaultExtension);
	    }
	    NX_FREE(defaultExtension);
	}
	*fd = creat(fileName,_MK_PERMS);
    }
    else { 
	*fd = open(fileName,O_RDONLY,_MK_PERMS); /* First look with no ext. */
	if (OPENFAIL(*fd) && defaultExtension) {
	    defaultExtension = _MKMakeStrcat(".",defaultExtension);
	    if (!extensionPresent(fileName,defaultExtension)){
		nameChanged = YES;                  /* Add it. */
		fileName = _MKMakeStrcat(fileName,defaultExtension);
	    }
	    NX_FREE(defaultExtension);
	    *fd = open(fileName,O_RDONLY,_MK_PERMS);/* Try with extension */
	}
    }
    if (OPENFAIL(*fd) || ((rtnVal = NXOpenFile(*fd,readOrWrite)) == NULL)) 
      if (errorMsg)
	_MKErrorf(MK_cantOpenFileErr,fileName);
    if (nameChanged)
      NX_FREE(fileName);
    return rtnVal;
}


/* Orchestra set/get */

/* At one time, I was thinking of supporting a List of all Orchestra classes
   so that people can add Orchestras for other hardware. But the changes
   needed to UnitGenerator, SynthPatch, etc. would be quite extensive. It's
   misleading to suggest that we really support other hardware now. Thus
   this function's not supported. */
#if 0

static id orchList = nil;

id MKOrchestraClasses(void)
    /* Returns a List of Orchestra factories. Ordinarily this list contains
	only the Orchestra factory. However you may modify this List to
	add your own Orchestra analog. This List is used for any
	"broadcasts". For example, the Conductor sends +flushTimedMessages
	to each of the elements in the List. */
{
    return orchList ? orchList : (orchList = [List new]);
}
#endif


/* Used by AsympUG, SynthPatch, etc. */
static double preemptDuration = .006;


double MKPreemptDuration(void)
/* Obsolete */
{
    return preemptDuration;
}

/* See musickit.h */
double MKGetPreemptDuration(void)
{
    return preemptDuration;
}

void MKSetPreemptDuration(double val)
{
    preemptDuration = val;
}

BOOL _MKInheritsFrom(id aFactObj,id superObj)
    /* Returns yes if aFactObj inherits from superObj */
{
    id obj = [Object class];
    for (;(aFactObj) && (aFactObj != obj) && (aFactObj != superObj);
	 aFactObj = [aFactObj superClass])
      ;
    return (aFactObj == superObj);
}



char * 
_MKMakeStr(str)
    char *str;
    /* Make a string and copy str into it. Returns 
       the new string. */
{
    char *rtnVal;
    if (!str)
      return NULL;
    _MK_MALLOC(rtnVal,char,strlen(str)+1);
    strcpy(rtnVal,str);
    return rtnVal;
}

char *
_MKMakeStrcat(str1,str2)
    char *str1,*str2;
    /* Makes a new string with str1 followed by str2. */
{
    char *rtnVal;
    if ((!str1) || (!str2))
      return NULL;
    _MK_MALLOC(rtnVal,char,strlen(str1)+strlen(str2)+1);
    strcpy(rtnVal,str1);
    strcat(rtnVal,str2);
    return rtnVal;
}

char *
_MKMakeSubstr(str,startChar,endChar)
    char *str;
    int startChar,endChar;
    /* Makes a new string consisting of a substring from the startChar'th
       character to the endChar'th character. If endChar is greater than
       the length of str, end at the end of the string. */
{
    char *rtnVal;
    register int i,len;
    register char *p,*q;
    if (!str)
      return NULL;
    len = strlen(str);
    endChar = MIN(endChar, len);
    _MK_MALLOC(rtnVal,char,endChar-startChar+2);
    p = str;
    q = rtnVal;
    if (startChar < 1) 
      startChar = 1;
    p += (startChar - 1);
    for (i = startChar; i <= endChar; i++)
      *q++ = *p++;
    *q = '\0';
    return rtnVal;
}

char *
_MKMakeStrRealloc(str, newStrPtr)
    char *str;
    char **newStrPtr;
    /* Assumes newStrPtr is already a valid string and does
       a REALLOC. Returns the new string and sets **newStrPtr
       to that string. */
{
    _MK_REALLOC(*newStrPtr,char,strlen(str)+1);
    strcpy(*newStrPtr,str);
    return *newStrPtr;
}


/* This is here for now, until I can get the inline version to work */

double MKGetNoDVal(void)
  /* Returns the special NaN that the Music Kit uses to signal "no value". */
{
	union {double d; int i[2];} u;
	u.i[0] = _MK_NANHI;
	u.i[1] = _MK_NANLO;
	return u.d;
}

int MKIsNoDVal(double val)
  /* Compares val to see if it is the special NaN that the Music Kit uses
     to signal "no value". */
{
	union {double d; int i[2];} u;
	u.d = val;
	return (u.i[0] == _MK_NANHI); /* Don't bother to check low bits. */
}
