#ifdef SHLIB
#include "shlib.h"
#endif

/*
  tokenNames.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  12/3/89/daj - Added seed and ranSeed.
  12/22/89/daj - Removed uPlus
  01/08/90/daj - Flushed false condtional compilation.
   3/06/90/daj - Added "repeat" to token list.
*/

#import "_musickit.h"
#import "_tokens.h"

/* Names of tokens ------------------------------------------------------ */

/* This must match up with the enum typedef in scorefile.h. */

#define NTOKENS ((int)_MK_highestToken - (int)_MK_undef + 1)

static const char *const tokenNames[NTOKENS]={
    "undefined",
    "noteDur",
    "noteOn",
    "noteOff",
    "noteUpdate",
    "mute",
    "resetControllers",
    "localControlModeOn",
    "localControlModeOff",
    "midiAllNotesOff",
    "omniModeOff",
    "omniModeOn",
    "monoMode",
    "polyMode",
    "sysClock",
    "sysUndefined0xf9",
    "sysStart",
    "sysContinue",
    "sysStop",
    "sysUndefined0xfd",
    "sysActiveSensing",
    "sysReset",

    "no type",
    "double value",
    "string value",
    "int value",
    "object value",
    "envelope value",
    "waveTable value",

    "parameter",
    "object definition",
    "variable",
    "untyped variable",
    "unary minus",
    "int",
    "double",
    "string",
    "var",
    "env",
    "wave",
    "obj",
    "envelope",
    "waveTable",
    "object",
    "include",
    "print",
    "t",
    "part",
    "partInstance",
    "scoreInstance",
    "BEGIN",
    "END",
    "comment",
    "endComment",
    "to",
    "tune",
    "ok",
    "noteTagRange",
    "dB",
    "ran",
    "data file",
    "x envelope point",
    "y envelope point",
    "envelope smoothing",
    "waveTable harmonic number",
    "waveTable amplitude",
    "waveTable phase",
    "envelope lookup",
    "info",
    "putGlobal",
    "getGlobal",
    "seed",
    "ranSeed",
    "LEQ",
    "GEQ",
    "EQU",
    "NEQ",
    "OR",
    "AND",
    "repeat",
    "if",
    "else",
    "while",
    "do",
    "_highestToken"
  };

const char * _MKTokName(tok)
    int tok;
    /* This may be used to write names of _MKTokens, MKDataTypes,
       MKMidiPars and MKNoteTypes. */
{
   if (((int)tok >= (int)_MK_undef) && ((int)tok <= (int)_MK_highestToken))
     return tokenNames[(int)tok - (int)_MK_undef];
   return "invalid";
}

const char * _MKTokNameNoCheck(tok)
    int tok;
{
    return tokenNames[(int)tok - (int)_MK_undef];
}












































