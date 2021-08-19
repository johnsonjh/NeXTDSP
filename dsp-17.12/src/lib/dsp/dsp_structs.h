/* DSP DATA STRUCTURES
 * Copyright 1989,1990, by NeXT, Inc. 
 * J.O. Smith
 */

/*
 * These structs provide an in-memory version of a .lod or .lnk file as
 * produced by the Motorola DSP56000/1 assembler.  Either type may also appear
 * in a .dsp file (fast binary format).
 *
 * General support is provided for absolute assembly, and specialized support
 * exists for relative assembly (as needed by the Music Kit and array
 * processing frameworks).  In the relative case, only three "sections" are
 * supported: the "global", "system", and "user" sections.  In the case of
 * absolute assembly (using the -a assembler option) everything appears 
 * in the "global" section (since sections are not used in absolute assembly).
 *
 * For an example of how to manage relocatable DSP modules, see the programming
 * example in /NextDeveloper/Examples/ArrayProcessing/myAP/.
 */

#ifndef _DSPSTRUCTS_
#define _DSPSTRUCTS_

# ifndef FILE
#   include <stdio.h>
# endif FILE

typedef enum _DSPSectionType {DSP_Global=0,DSP_System,DSP_User,
	DSP_NSectionTypes} DSPSectionType;

/* strings from DSPGlobals.c */
extern const char * const DSPSectionNames[]; /* "GLOBAL", "SYSTEM", "USER" */
extern const char * const DSPMemoryNames[];  /* Memory spaces (N,X,Y,L,P) */
extern const char * const DSPLCNames[];	/* Location counters (N,XL,X,XH,...) */
extern int   DSPLCtoMS[];	/* DSP memory code given LC number */

/* enum for DSP memory spaces */
typedef enum _DSPMemorySpace {DSP_MS_N=0, 
			      DSP_MS_X,
			      DSP_MS_Y,
			      DSP_MS_L,
			      DSP_MS_P,
			    DSP_MS_Num} DSPMemorySpace;

/* 
 * The exact layout of this enum is depended upon by _DSPGetMemStr 
 * and _DSPDataRecordMerge (DSPStructMisc.c) 
 */
typedef enum _DSPLocationCounter {DSP_LC_N=0, 
			      DSP_LC_X, DSP_LC_XL, DSP_LC_XH, 
			      DSP_LC_Y, DSP_LC_YL, DSP_LC_YH, 
			      DSP_LC_L, DSP_LC_LL, DSP_LC_LH, 
			      DSP_LC_P, DSP_LC_PL, DSP_LC_PH, 
			    DSP_LC_Num} DSPLocationCounter;

/* NOTE: DSP_LC_P,DSP_LC_PL,DSP_LC_PH MUST BE CONTIGUOUS */

#define DSP_LC_NUM (int)DSP_LC_Num /* no. DSP mem space types (incl. none) */
#define DSP_N_SECTIONS (int)DSP_NSectionTypes

#define DSP_LC_NUM_P 3		/* no. DSP PROGRAM memory spaces */
#define DSP_LC_P_BASE DSP_LC_P	/* First DSP_LC_P memory space. */

/* 
 * DSPDataRecord
 *
 * The DSPDataRecord structure contains the result of reading a _DATA or
 * _BLOCKDATA record in the .lnk/.lod file.  If the owning section is type
 * absolute, there may be a linked list of data blocks, each specifying
 * its own loadAddress parameter. For relative sections, there will be only
 * one data block per memory space (next==NULL), and the loadAddress field
 * is not used. (The load address is specified for each memory space in the
 * DSPSection struct instead of the data block.)  
 * 
 * For a _DATA record,	repeatCount is 1 wordCount gives the number of words
 * in data[]. For long DSP data (type l:), words are stored interleaved: 
 * Hi1,Lo1, Hi2,Lo2, etc., and wordCount equals the number of 24-bit words
 * which is then the number of long words times 2.
 *
 * For a _BLOCKDATA record, wordCount is 1 and repeatCount tells how many
 * times the single word (or two words for l memory) in data is to be 
 * repeated in a memory constant fill.
 */

typedef struct _DSPDataRecord { /* _BLOCKDATA or _DATA spec from lnk file */
    struct _DSPSection *section; /* Orig owning section when read from file */
    DSPLocationCounter locationCounter; /* mem segment for this data */
    DSPAddress loadAddress;	/* from _{BLOCK}DATA record. 0 for relative. */
    int repeatCount;		/* _BLOCKDATA repeatCount */
    int wordCount;		/* length of *data array in ints */
    int *data;			/*  _DATA array or _BLOCKDATA constant(s) */
    struct _DSPDataRecord *next; /* For linked list (absolute sections only) */
    struct _DSPDataRecord *prev; /* Relative sections always have NULL here  */
} DSPDataRecord;

union DSPSymVal {
    int i;
    float f;
};

typedef struct _DSPSymbol {
    DSPLocationCounter locationCounter; /* one of DSPLCNames above */
    char *name;
    char *type;		    /* (L|G)(A|R)(I|F). E.g., "LRI","GAI","I" */
    union DSPSymVal value;  /* int or float. (Must be last field in struct) */
} DSPSymbol;


/*
 * A "fixup" is the resolution of a relocatable symbol reference.
 * The idea is to provide a way to quickly relocate a DSP section.
 * To relocate a section, each fixup is performed by adding the symbol's
 * section load address to the relAddress of the relocatable symbol
 * (relAddress == relative address in its section). The "fixed up"
 * address is then installed where it is used, refOffset from
 * the beginning of the memory space owning the fixup.
 */

typedef struct _DSPFixup {	/* used for fast relocation */
    DSPLocationCounter locationCounter; /* space of relocatable symbol */
    char *name;			/* name of relocatable symbol (pass 1) */
    int decrement;		/* 1 means relAddress = symbol's val - 1 */
    int refOffset;		/* offset of ref in P, PL, or PH segment */
    DSPAddress relAddress;	/* symbol's address before relocation */
} DSPFixup;

typedef struct _DSPSection {	/* DSP section state from .lnk file. */
/* _SECTION record fields */
    char *name;			/* GLOBAL, SYSTEM, or USER */
    char *type;			/* R (relative) or A (absolute) */
    int number;			/* 0=global, 1=system, 2=user */
    DSPDataRecord *data[DSP_LC_NUM]; /* Sorted absolute data blocks */
    DSPDataRecord *dataEnd[DSP_LC_NUM]; /* point to end of data blocks */
    int symAlloc[DSP_LC_NUM];	   /* no. of symbols allocated in each space */
    int symCount[DSP_LC_NUM];	   /* number of symbols loaded in each space */
    struct _DSPSymbol *(symbols[DSP_LC_NUM]); /* symbol list per mem section */
    int fixupAlloc[DSP_LC_NUM_P]; /* number of fixups allocated per p space */
    int fixupCount[DSP_LC_NUM_P]; /* number of fixups in each p space */
    DSPFixup *fixups[DSP_LC_NUM_P]; /* fix-up array for each p space */
    DSPAddress loadAddress[DSP_LC_NUM]; /* Relocation offset 
					   (unused for absolute) */
    int xrefAlloc;		/* number of xrefs allocated per p space */
    int xrefCount;		/* number of xrefs in each p space */
    char **xrefs;		/* xref array = external symbol references */
} DSPSection;

typedef struct _DSPLoadSpec {	/* All DSP state as loaded from .lnk file. */

/* _START record fields */
    char *module;		/* module thru errorCount are set by _START */
    char *type;			/* "A" => absolute assembly, "R" => relative */
    int version;
    int revision;
    int errorCount;
    DSPAddress startAddress;	/* set by _END directive of object file */
    char *comments;		/* _COMMENT lines from object file */
    char *description;		/* text settable by user program */
/* _SECTION records possible for MK and AP DSP programs */
    DSPSection *globalSection;	/* Also used by (sectionless) .lod files */
    DSPSection *systemSection;
    DSPSection *userSection;
/*  array of section pointers indexed by section number */
    DSPSection *indexToSection[DSP_N_SECTIONS];
} DSPLoadSpec;

#endif	_DSPSTRUCTS_

