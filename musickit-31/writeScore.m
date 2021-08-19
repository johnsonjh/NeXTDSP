#ifdef SHLIB
#include "shlib.h"
#endif

/*
  writeScorefile.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/ 
/* 
Modification history:

  09/22/89/daj - Added _MK_BACKHASH bit or'ed in with type when adding name,
                 to accommodate new way of handling back-hashing. Fixed bug
		 whereby very large table was needlessly created for writing
		 files.

  10/06/89/daj - Changed to use hashtable.h version of table.
  10/20/89/daj - Added binary scorefile support.
  12/15/89/daj - Changed SCOREMAGIC write to conform to one-word magic
  01/09/89/daj - Changed comments.
*/

/*
   Note that the code for writing scorefiles is spread between writeScore.m,
   Note.m, and _ParName.m. This is for reasons of avoiding inter-module
   communication (i.e. minimizing globals). Perhaps the scorefile-writing
   should be more cleanly isolated.
*/

/* See binaryScorefile.doc on the musickit source directory for explanation
   of binary scorefile format. */

#import <objc/HashTable.h>

#import "_musickit.h"
#import "_MKNameTable.h"   
#import "_Note.h"
#import "_tokens.h"
#import "_error.h"
#import "_noteRecorder.h"

static void writeScoreInfo(_MKScoreOutStruct *p,id info)
    /* Writes the Score "info note" */
{
    NXStream *aStream = p->_stream;
    if (!info)
      return;
    if (p->_binaryIndecies) 
      _MKWriteShort(aStream,_MK_info);
    else NXPrintf(aStream,"%s ",_MKTokName(_MK_info));
    _MKWriteParameters(info,aStream,p);
}

#define NO_TAG_YET -1.0

_MKScoreOutStruct *
_MKInitScoreOut(NXStream *fileStream,id owner,id anInfoNote,double timeShift,
		BOOL isNoteRecorder,BOOL binary)
{
    /* Makes new _MKScoreOutStruct for specified file.
       Assumes file has just been opened. */
    _MKScoreOutStruct * p;
    if (!fileStream) 
      return NULL;
    _MK_MALLOC(p,_MKScoreOutStruct,1);
    p->_stream = fileStream;             
    p->_nameTable = _MKNewScorefileParseTable();
    /* We need keyword and such symbols here to make sure there's no
       collission with such symbols when file is written. */
    p->timeTag = NO_TAG_YET; /* Insure first 0 tag is written */
    p->_timeShift = timeShift;
    p->_owner = owner;
    p->_ownerIsNoteRecorder = isNoteRecorder;
    p->isInBody = NO;
#   define DEFAULTINDECIES 8
    p->_binary = binary;
    if (binary) {
	p->_binaryIndecies = [HashTable newKeyDesc:"@" valueDesc:"i"
			    capacity:DEFAULTINDECIES];
	p->_highBinaryIndex = 0;
    } else p->_binaryIndecies = nil;
    if (binary)
      _MKWriteInt(p->_stream,MK_SCOREMAGIC);
    writeScoreInfo(p,anInfoNote);
    return p;
}

#define BINARY(_p) (p->_binary)

static void writePartInfo(_MKScoreOutStruct *p,id aPart,char *partName,
			  id info)
    /* Writes the Part "info note" */
{
    NXStream *aStream = p->_stream;
    if (!info)
      return;
    if (BINARY(p)) {
	_MKWriteShort(aStream,_MK_partInstance);
	_MKWriteShort(aStream,
		      (int)[p->_binaryIndecies valueForKey:(const void *)aPart]);
    }
    else NXPrintf(aStream,"%s ",partName);
    _MKWriteParameters(info,aStream,p);
}

_MKScoreOutStruct *
_MKWritePartDecl(id aPart, _MKScoreOutStruct * p,id aPartInfo)
{
    /* File must be open before this function is called. 
       Gets partName from global table. 
       If the partName collides with any symbols written to the
       file, a new partName of the form partName<low integer> is formed.
       If p has no name, generates a new name of the form 
       <part><low integer>.  If p is NULL, returns NULL, else p. */
    BOOL newName;
    unsigned short iTmp;
    id tmp;
    char * partName;
    if (!p)
      return NULL;
    partName = _MKNameTableGetObjectName(p->_nameTable,aPart,&tmp);
    if (partName)
      return p;                            /* Already declared */

    /* If we've come here, we already know that the object is not named in the
       local table. */
    partName = (char *)MKGetObjectName(aPart);     /* Get object's name */
    newName = 
      ((partName == NULL) ||       /* No name */
       (_MKNameTableGetObjectForName(p->_nameTable,partName,nil,&iTmp) != nil)
       /* Name exists */
       );
    if (newName) {      /* anonymous object or name collission */
	id hashObj;
	if (!partName)
	  partName = (char *)_MKTokNameNoCheck(_MK_part); 
	/* Root of anonymous name */
	partName = _MKUniqueName(_MKMakeStr(partName),p->_nameTable,aPart,
				 &hashObj);
    }
    if (BINARY(p)) {
	_MKWriteShort(p->_stream,_MK_part);
	_MKWriteString(p->_stream,partName);
	[p->_binaryIndecies insertKey:(const void *)aPart
       value:(void *)(++(p->_highBinaryIndex))];
    }
    else 
      NXPrintf(p->_stream,"%s %s;\n",_MKTokNameNoCheck(_MK_part),partName);
    _MKNameTableAddName(p->_nameTable,partName,nil,aPart,
			_MK_partInstance | _MK_BACKHASHBIT,YES);
    writePartInfo(p,aPart,partName,aPartInfo);
    if (newName)
      NX_FREE(partName);
    return p;
}

_MKScoreOutStruct *_MKFinishScoreOut(_MKScoreOutStruct * p,
				     BOOL writeEnd)
{
    /* Frees struct pointed to by argument. Does not
       close file. Returns NULL. */
    if (p) {
	if (writeEnd) {
	    if (!(p->isInBody)) 
	      if (BINARY(p)) {
		  _MKWriteShort(p->_stream,_MK_begin);
		  _MKWriteShort(p->_stream,_MK_end);
	      }
	      else {
		  NXPrintf(p->_stream,"\n\n%s;\n\n",
			   _MKTokNameNoCheck(_MK_begin));
		  NXPrintf(p->_stream,"%s;\n",_MKTokNameNoCheck(_MK_end));
	      }
	    else {
	      if (BINARY(p)) {
		  _MKWriteShort(p->_stream,_MK_end);
		  _MKWriteShort(p->_stream,0); /* Parser likes to read 
						  4 bytes */
	      }
	      else 
		NXPrintf(p->_stream,"%s;\n",_MKTokNameNoCheck(_MK_end));
	  }
	}
	_MKFreeScorefileTable(p->_nameTable);
	NXFlush(p->_stream);
	NX_FREE(p);
    }
    return NULL;
}

_MKScoreOutStruct *
_MKWriteNote(id aNote, id aPart, _MKScoreOutStruct * p)
{
    /* If p is NULL, return NULL. Else write note, adding timeTag if 
       necessary.
       If timeTag is out of order, error. */
    double timeTag;
    if (!p)
      return NULL;
    if (!(p->isInBody)) {
	if (BINARY(p))
	  _MKWriteShort(p->_stream,_MK_begin);
	else NXPrintf(p->_stream,"\n\n%s;\n\n",_MKTokNameNoCheck(_MK_begin));
	p->isInBody = YES;
    }
    timeTag = ((p->_ownerIsNoteRecorder) ? 
	       _MKTimeTagForTimeUnit(aNote,[p->_owner timeUnit]) :
	       ([aNote timeTag] + p->_timeShift));
    if (timeTag < 0)
      timeTag = 0;
    if (timeTag > p->timeTag) {
	if (BINARY(p)) {
	    double t = timeTag;
#           define MAX_FLOAT_TIME ((float)2.5) /* Minutes */
	    /* At a sampling rate of 44100, we can encode 6 minutes with
	       sample-level accuracy in a float. At 48000, this goes down
	       to 5.8 minutes. For safety, we divide this by 2.
	       For more than MAX_FLOAT_TIME, we start to lose precision. 
	       Therefore, we never write more
	       than a MAX_FLOAT_TIME relative time to a binary scorefile. */
	    if (p->timeTag == NO_TAG_YET)  
	      p->timeTag = 0;
	    t -= p->timeTag; /* Make it relative */
	    while (t > MAX_FLOAT_TIME) {
		_MKWriteShort(p->_stream,_MK_time);
		_MKWriteFloat(p->_stream,MAX_FLOAT_TIME);
		t -= MAX_FLOAT_TIME;
	    }
	    _MKWriteShort(p->_stream,_MK_time);
	    _MKWriteFloat(p->_stream,(float)t);
	}
	else 
	  NXPrintf(p->_stream,"%s %.5f;\n",_MKTokNameNoCheck(_MK_time),
		   timeTag);
	p->timeTag = timeTag;
    }
    else if (timeTag < p->timeTag) 
      _MKErrorf(MK_outOfOrderErr,timeTag,p->timeTag);
    if (aNote) 
      _MKWriteNote2(aNote,aPart,p);
    return p;
}





