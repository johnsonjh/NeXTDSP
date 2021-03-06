/* 
    errors.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */
/* This file has trace codes as well as error codes used by the
   Music Kit. */

/* Music Kit TRACE codes */
#define MK_TRACEORCHALLOC 1  /* Orchestra allocation information is printed */
#define MK_TRACEPARS 2       /* Application parameters are printed when first
                    encountered. */
#define MK_TRACEDSP 4        /* Music Kit DSP messages are printed. */
#define MK_TRACEMIDI 8       /* MIDI in/out warnings are printed. */
#define MK_TRACEPREEMPT 16   /* SynthInstrument messages are printed for 
                 preempted notes. */
#define MK_TRACESYNTHINS  32 /* SynthInstrument messages are printed. */
#define MK_TRACESYNTHPATCH 64 /* SynthPatch library messages are printed. */
#define MK_TRACEUNITGENERATOR 128 /* UnitGenerator library messages are 
                     printed. */

 /* Tracing.  */
extern unsigned MKSetTrace(int traceCode);
 /* Turns on specified trace bit. */

extern unsigned MKClearTrace(int traceCode);
 /* Turns off specified trace bit. */

extern BOOL MKIsTraced(int traceCode);
 /* Returns whether specified trace bit is on. */

 /* Due to the requirements of real-time The Music Kit uses a different 
    mechanism from that of the Application Kit to do error handling. The 
    following functions impelment that mechanism. 

    Note that it is not guaranteed to be safe to NX_RAISE an error in any 
    performance-oriented class. 
   */

extern void MKSetErrorProc(void (*errProc)(char *msg));
    /* Sets proc to be used when MKError() is called. If errProc is NULL,
       uses the default error proc, which writes to the Music Kit error
       NXStream (see MKSetErrorStream()). errProc takes one string argument. 
       When the *errProc is called in response to a Music Kit error, errno is 
       set to the MKErrno corresponding to the error. If *errProc is invoked in
       response to an application-defined error (see MKError), errno is not
       set; it's up to the application to set it, if desired. 
       */

extern id MKError(char * msg);
    /* Calls the user's error proc (set with MKSetErrorProc), if any, with 
       one argument, the msg. Otherwise, writes the message on the Music
       Kit error stream. (See MKSetErrorStream) Returns nil.
       */

extern void MKSetErrorStream(NXStream *aStream);
    /* Sets the Music Kit error stream. 
       NULL means stderr. The Music Kit initialization sets the error 
       stream to stderr. Note that during a multi-threaded Music Kit 
       performance, errors invoked from the Music Kit thread are not sent
       to the error stream. Use MKSetErrorProc to see them. */

extern NXStream *MKErrorStream(void);
    /* Returns the Music Kit error stream. This is, by default, stderr.  */

/* Errors generated by the Music Kit. You don't generate these yourself. */

#define MK_ERRORBASE 4000    /* 1000 error codes for us start here */

typedef enum _MKErrno {
    MK_musicKitErr = MK_ERRORBASE,
    MK_machErr,
    /* Representation errors */
    MK_cantOpenFileErr ,
    MK_cantCloseFileErr,
    MK_outOfOrderErr,           /* Scorefile parsing/writing error */
    MK_samplesNoResampleErr,
    MK_noMoreTagsErr,
    MK_notScorefileObjectTypeErr,
    /* Synthesis errors */
    MK_orchBadFreeErr,
    MK_synthDataCantClearErr,   /* Synthdata errors */ 
    MK_synthDataLoadErr,
    MK_synthDataReadonlyErr,
    MK_synthInsOmitNoteErr,     /* SynthInstrument errors */
    MK_synthInsNoClass,
    MK_ugLoadErr,               /* UnitGenerator errors. */
    MK_ugBadArgErr,
    MK_ugBadAddrPokeErr,
    MK_ugBadDatumPokeErr,
    MK_ugOrchMismatchErr,
    MK_ugArgSpaceMismatchErr,
    MK_ugNonAddrErr,
    MK_ugNonDatumErr,

    /* Scorefile errors. */
    MK_sfBadExprErr,     /* Illegal constructs */
    MK_sfBadDefineErr,
    MK_sfBadParValErr,
    MK_sfNoNestDefineErr,

    MK_sfBadDeclErr,     /* Missing constructs */
    MK_sfMissingStringErr,
    MK_sfBadNoteTypeErr,
    MK_sfBadNoteTagErr,
    MK_sfMissingBackslashErr,
    MK_sfMissingSemicolonErr,
    MK_sfUndeclaredErr,
    MK_sfBadAssignErr,
    MK_sfBadIncludeErr,
    MK_sfBadParamErr,
    MK_sfNumberErr,
    MK_sfStringErr,
    MK_sfGlobalErr,
    MK_sfCantFindGlobalErr,
    
    MK_sfMulDefErr, /* Duplicate constructs */
    MK_sfDuplicateDeclErr,

    MK_sfNotHereErr,
    MK_sfWrongTypeDeclErr,
    MK_sfBadHeaderStmtErr,
    MK_sfBadStmtErr,

    MK_sfBadInitErr,
    MK_sfNoTuneErr,
    MK_sfNoIncludeErr,
    MK_sfCantFindFileErr,
    MK_sfCantWriteErr,
    MK_sfOutOfOrderErr,
    MK_sfUnmatchedCommentErr,
    MK_sfInactiveNoteTagErr,
    MK_sfCantFindClass,
    MK_sfBoundsErr, 
    MK_sfTypeConversionErr,
    MK_sfReadOnlyErr,
    MK_sfArithErr,
    MK_sfNonScorefileErr,
    MK_sfTooManyErrorsErr,
    
    /* Unit generator library errors. */
    MK_ugsNotSetRunErr,
    MK_ugsPowerOf2Err,
    MK_ugsNotSetGetErr,

    /* Synth patch library errors. */
    MK_spsCantGetMemoryErr,
    MK_spsSineROMSubstitutionErr,
    MK_spsInvalidPartialsDatabaseKeywordErr, 
    MK_spsOutOfRangeErr,
    MK_spsCantGetUGErr,

    /* End marker */
    MK_highestErr,
    /* Reserved from here until MK_maxErr */
    MK_maxErr = (MK_ERRORBASE + 1000)
} MKErrno;

#define MK_sfNonAsciiErr MK_sfNonScorefileErr /* For backwards compatibility */



