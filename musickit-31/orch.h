/* 
    orch.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */
#import <dsp/dsp.h>             /* Contains DSPAddress, etc. */

typedef enum _MKOrchMemSegment { /* Memory segments for Orchestra */
    /* Memory segments may be on or off chip unless otherwise indicated */
    MK_noSegment = 0,            /* Illegal segment. */
    MK_pLoop,                    /* Orchestra loop P memory. */
    MK_pSubr,                    /* P subroutine memory (off-chip only) */
    MK_xArg,                     /* X argument memory. 
                                    (currently only on-chip) */
    MK_yArg,                     /* Y argument memory. 
                                    (currently only on-chip) */
    MK_lArg,                     /* L argument memory. (on-chip only) */
    MK_xData,                    /* X data memory (off-chip only) */
    MK_yData,                    /* Y data memory (off-chip only, except sin
                                    table rom) */
    MK_lData,                    /* L data memory (currently unused). */
    MK_xPatch,                   /* X patchpoints */
    MK_yPatch,                   /* Y patchpoints */
    MK_lPatch,                   /* L patchpoints (currently unused). */
    MK_numOrchMemSegments        /* End marker */ 
  } MKOrchMemSegment;

typedef struct _MKOrchMemStruct { /* Used to represent relocation as well
                     as memory usage of UnitGenerators. */ 
    unsigned xArg;   /* x unit generator memory arguments */
    unsigned yArg;   /* y */
    unsigned lArg;   /* l */
    unsigned pLoop;  /* program memory that's part of the main orch loop */
    unsigned pSubr;  /* program memory subroutines */ 
    unsigned xData;  /* Also used for xPatch memory */
    unsigned yData;  /* Also used for yPatch memory */
    unsigned lData;  /* Currently unused. */
} MKOrchMemStruct;

typedef struct _MKOrchAddrStruct { /* Used to represent orchestra addresses. */
    DSPAddress address;            /* Absolute address of symbol. */
    DSPMemorySpace memSpace;       /* In low-level DSP terms. */
    MKOrchMemSegment memSegment;   /* In higher-level Orchestra terms. */
    int orchIndex;                 /* Which DSP. */
} MKOrchAddrStruct;

typedef enum _MKSynthStatus { /* Status for SynthPatches and UnitGenerators. */
    MK_idle,                  /* Writing to sink (nowhere). */ 
    MK_running,               /* The meaning of this is defined by the ug */
    MK_finishing,             /* The meaning of this is defined by the ug */
  } MKSynthStatus;



