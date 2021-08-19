#ifdef SHLIB
#include "shlib.h"
#endif

/* 
Modification history:

  01/08/90/daj - Added comments.

*/

/* This file supports the writing of binary ("optimized") scorefiles. These
   files have the extension .playscore. See the file binaryScorefile.doc
   on the musickit source directory */

#import "_musickit.h"
#import "_tokens.h"
#import "_scorefile.h"

void _MKWriteIntPar(NXStream *aStream,int anInt)
{
    short aType = MK_int;
    NXWrite(aStream,&aType,sizeof(aType));
    NXWrite(aStream,&anInt,sizeof(anInt));
}

void _MKWriteDoublePar(NXStream *aStream,double aDouble)
{
    short aType = MK_double;
    NXWrite(aStream,&aType,sizeof(aType));
    NXWrite(aStream,&aDouble,sizeof(aDouble));
}

void _MKWriteStringPar(NXStream *aStream,char *aString)
{
    short aType = MK_string;
    NXWrite(aStream,&aType,sizeof(aType));
    NXWrite(aStream,aString,strlen(aString)+1);
}

void _MKWriteVarPar(NXStream *aStream,char *aString)
{
    short aType = _MK_typedVar;
    NXWrite(aStream,&aType,sizeof(aType));
    NXWrite(aStream,aString,strlen(aString)+1);
}

void _MKWriteInt(NXStream *aStream,int anInt)
{
    NXWrite(aStream,&anInt,sizeof(anInt));
}

void _MKWriteShort(NXStream *aStream,short anShort)
{
    NXWrite(aStream,&anShort,sizeof(anShort));
}

void _MKWriteDouble(NXStream *aStream,double aDouble)
{
    NXWrite(aStream,&aDouble,sizeof(aDouble));
}

void _MKWriteFloat(NXStream *aStream,float aFloat)
{
    NXWrite(aStream,&aFloat,sizeof(aFloat));
}

void _MKWriteChar(NXStream *aStream,char aChar)
{
    NXWrite(aStream,&aChar,sizeof(aChar));
}

void _MKWriteString(NXStream *aStream,char *aString)
{
    NXWrite(aStream,aString,strlen(aString)+1);
}



