/*	DSPTransfer.c - DSP utilities for data transfer.
	Copyright 1987,1988, by NeXT, Inc.

Modification history:
	07/28/88/jos - Created from _DSPUtilities.c
	12/12/88/jos - Rewrote DSPMKSendSkipArrayTimed() to be atomic and fast
	02/20/89/daj - Fixed timestamp argument in timed BLTs.
		       Added DSPIntToFix24 to -1 arguments in BLTs.
	03/24/89/jos - DSPMKRetValueTimed now always waits forever.
	05/12/89/jos - Added NOP before JMP LOOP_BEGIN in 
		       _DSPMKSendUnitGeneratorWithLooperTimed()
	05/12/89/jos - Brought in DSP{Get,Put}{Int,Float}Array() from 
		       DSPAPUtilities() and removed checking of addresses.
		       AP versions will now have DSPAP prefix.
	01/13/90/jos - Added DSPMKSendArraySkipModeTimed.
		       Moved data arg to 2nd position and added mode arg.
	01/13/90/jos - Added DSPMKSendShortArraySkipTimed() for David.
		       Moved data arg to 2nd position and added mode arg.
	03/19/90/jos - Changed header file: reordered some routines. 
		       Deleted ReadArray prototype. 
		       Changed ShortArray data ptr to short int *!
		       Added DSP_MIN_DMA_{READ,WRITE}_SIZE to dsp.h.
		       Still need to add support for it.
		       Added LJ version of Read/Write Fix24 Array.  
	03/24/90/daj - Fixed extra loopers when breaking up UGs in
		       _DSPMKSendUnitGeneratorWithLooperTimed()
        04/23/90/jos - flushed unsupported entry points.
        04/23/90/jos - fixed arg order in publication of 
		       DSPMKSendArraySkipTimed, DSPMKSendValueTimed,
		       DSPMKSendLongTimed. Old versions kept for MK 1.0.
        04/24/90/jos - fixed arg order in publication of 
		       DSPMKSendValue, DSPMKSendLong,
		       DSPWriteValue, DSPWriteLong,
		       and DSPMKSendArray IN PLACE
        04/25/90/jos - added DSPGetSymbolInLC()
        04/26/90/jos - Since DSPWriteValue and DSPWriteLong were published in
		       1.0, they were reverted, renamed to DSPWrite{}1p0(),
		       and the new correct-arg-order versions have fresh
		       shlib slots. (For 1.0 binary compatibility.)
	04/30/90/jos - Removed "r" prefix from rData, rWordCount, rRepeatCount
	05/01/90/jos - Added DSP memory boundary sensing functions
	05/01/90/jos - added DSPMKClearDSPSoundOutBufferTimed(ts);
*/
	
#ifdef SHLIB
#include "shlib.h"
#endif

#include "dsp/_dsp.h"
#include <nextdev/snd_msgs.h>

#   define NEGATIVE1 (-1 & 0xffffff)

static int ec;			/* Error code */

/************************ UNTIMED TRANSFERS TO THE DSP ***********************/

int DSPMemoryFill(
    DSPFix24 fillConstant,	/* value to use as DSP memory initializer */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* first address within DSP memory to fill */
    int wordCount)		/* number of DSP words to initialize */
{
    int ec;
    ec =DSPMKMemoryFillTimed(DSPMK_UNTIMED,fillConstant,
				memorySpace,startAddress,wordCount);
    return(ec);
}    

int DSPMemoryClear(
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* first address within DSP memory to fill */
    int wordCount)		/* number of DSP words to initialize */
/*
 * Set a block of DSP private RAM to zero.
 * It is equivalent to DSPMemoryFill(0,memorySpace,startAddress,wordCount))
 */
{
    return DSPMKMemoryClearTimed(DSPMK_UNTIMED,memorySpace,startAddress,
				   wordCount);
}

int DSPWriteFix24Array(
    DSPFix24 *data,		/* array to send to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c and <nextdev/snd_msgs.h> */
    return(DSPWriteArraySkipMode(data,memorySpace,startAddress,
			       skipFactor,wordCount,DSP_MODE32));
}

int DSPWriteFix24ArrayLJ(
    DSPFix24 *data,		/* array to send to DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c and <nextdev/snd_msgs.h> */
    return(DSPWriteArraySkipMode(data,memorySpace,startAddress,
			 skipFactor,wordCount,DSP_MODE32_LEFT_JUSTIFIED));
}

int DSPWritePackedArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word written */
    int wordCount)		/* DSP words = byte count / 3 */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,
			       skipFactor,wordCount,DSP_MODE24));
}


int DSPWriteShortArray(
    short int *data,		/* Packed short data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per short written */
    int wordCount)		/* DSP word count = byte count / 2 */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,
			       skipFactor,wordCount,DSP_MODE16));
}


int DSPWriteByteArray(
    unsigned char *data,	/* Data to send to DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount)		/* Total number of bytes to transfer */
{	
    return(DSPWriteArraySkipMode((DSPFix24 *)data,
				 memorySpace,startAddress,skipFactor,
				 byteCount,DSP_MODE8));
}


int DSPWriteIntArray(
    int *intArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    int ec;
    if (wordCount<=0)
      return(0);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    /* not needed: ec = DSPIntToFix24Array(fix24Array,intArray,wordCount); */
    ec = DSPWriteArraySkipMode(intArray,memorySpace,startAddress,skipFactor,wordCount,DSP_MODE32);
    return(ec);
}

    
int DSPWriteFloatArray(
    float *floatArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPFloatToFix24Array(floatArray,fix24Array,wordCount);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    ec2 = DSPWriteArraySkipMode(fix24Array,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}

int DSPWriteDoubleArray(
    double *doubleArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPDoubleToFix24Array(doubleArray,fix24Array,wordCount);
    /* old AP version: 
       DSPCheckWriteAddresses(startAddress,skipFactor,wordCount); 
     */
    ec2 = DSPWriteArraySkipMode(fix24Array,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}


int DSPWriteFloatArrayXY(
    float *floatArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPFloatToFix24Array(floatArray,fix24Array,wordCount);
 /* DSPCheckWriteAddressesXY(memorySpace,startAddress,skipFactor,wordCount); */
    ec2 = DSPWriteArraySkipMode(fix24Array,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}


int DSPWriteDoubleArrayXY(
    double *doubleArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    int ec1,ec2;
    DSPFix24 fix24Array[wordCount];
    if (wordCount<=0)
      return(0);
    ec1 = DSPDoubleToFix24Array(doubleArray,fix24Array,wordCount);
 /* DSPCheckWriteAddressesXY(memorySpace,startAddress,skipFactor,wordCount); */
    ec2 = DSPWriteArraySkipMode(fix24Array,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32);
    return(ec2);

    REMEMBER(Clipping ignored)

}

/******************************* Data Record Transfer ************************/

int DSPDataRecordLoad(DSPDataRecord *dr) /* cf. <dsp/dsp_structs.h> */
{
    int i,ms,la,wc,rc,*dp;

    while (dr) {
	
	ms = DSPLCtoMS[(int)dr->locationCounter];
	la = dr->loadAddress+dr->section->loadAddress[(int)dr->locationCounter];
	dp = dr->data;
	rc = dr->repeatCount;
	wc = dr->wordCount;
	
	if (rc == 1) {
	    DSP_UNTIL_ERROR(DSPWriteFix24Array(dp,ms,la,1,wc));
	}
	else
	  if (wc == 1) {
	      DSP_UNTIL_ERROR(DSPMemoryFill(*dp,ms,la,rc));
	  }
	  else
	    for (i=0;i<rc;i++)
	      DSP_UNTIL_ERROR(DSPWriteFix24Array(dp,ms,la,1,wc));

	dr = dr->next;
    }
    return(0);
}


/******************* POKING ONCHIP DSP SYMBOLS *************************/

DSPSymbol *DSPGetSymbol(char *name, DSPSection *sec)
{
    int i,j,k,symcount;
    DSPSymbol *sym;
    for (j=0;j<DSP_LC_NUM;j++) {
	symcount = sec->symCount[j];
	for (k=0;k<symcount;k++) {
	    sym = &sec->symbols[j][k];
	    if (strcmp(sym->name,name)==0)
	      return(sym);
	}
    }
    return (DSPSymbol *)_DSPError1(0,"DSPGetSymbol: "
				   "Could not find symbol '%s'",name);
}

DSPSymbol *DSPGetSymbolInLC(char *name, DSPSection *sec, DSPLocationCounter lc)
{
    int i,k,symcount;
    DSPSymbol *sym;
    symcount = sec->symCount[lc];
    for (k=0;k<symcount;k++) {
	sym = &sec->symbols[lc][k];
	if (strcmp(sym->name,name)==0)
	  return(sym);
    }
    return (DSPSymbol *)_DSPError1(0,"DSPGetSymbol: "
				   "Could not find symbol '%s'",name);
}

int DSPSymbolIsFloat(DSPSymbol *sym)
{
    int isF;
    if (strlen(sym->type)==1)	/* absolute assembly */
      isF = (*sym->type!='I'); 
    else if (strlen(sym->type)>1) /* relative assembly */
      isF = (sym->type[2]!='I');
    return(isF);
}

int DSPGetSymbolAddress(DSPMemorySpace *spacep,
			DSPAddress *addressp,
			char *name,			/* name of symbol */
			DSPSection *sec)
{
    DSPSymbol *sym;
    sym = DSPGetSymbol(name,sec);
    if (!sym)
      return(-1);
    if (DSPSymbolIsFloat(sym))
      return(_DSPError(DSP_EMISC,"DSPGetSymbolAddress: "
			   "Desired symbol is a floating-point "
			   "variable, not a DSP memory location."));
    
    *spacep = DSPLCtoMS[sym->locationCounter];
    if (*spacep == DSP_MS_N)
      return(_DSPError(DSP_EMISC,"DSPGetSymbolAddress: "
			   "Desired symbol is in memory space N "
			   "which is not a DSP memory location."));
    *addressp = (DSPAddress)sym->value.i;
    return(0);
}

int DSPPoke(
    char *name,
    DSPFix24 value,
    DSPLoadSpec *dsp)
{
    int ec;
    DSPSection *usr;
    DSPMemorySpace symMS;
    DSPAddress symAddr;
    if (*dsp->type=='R')	/* shouldn't happen */
      printf("*** DSPPoke: Relative assembly used. It better be relocated!\n");
    usr = DSPGetUserSection(dsp);
    if (ec = DSPGetSymbolAddress(&symMS,&symAddr,name,usr))
      return(ec);
    return DSPWriteValue(value,symMS,symAddr);
}

int DSPPokeFloat(
    char *name,
    float value,
    DSPLoadSpec *dsp)
{
    return(DSPPoke(name,DSPFloatToFix24(value),dsp));
}


/******************** GETTING DSP MEMORY BOUNDARIES **************************/

/* All of these routines return -1 for error */

int DSPGetValueOfSystemSymbol(char *name)
{
    DSPSymbol *dspsym;
    DSPLoadSpec *dspsys = DSPGetSystemImage();

    if (dspsys==NULL)
      return -1;

    dspsym = DSPGetSymbolInLC(name, dspsys->globalSection, DSP_LC_N);

    if (dspsym==NULL)
      return -1;

    if (dspsym->type[0] != 'I')
      _DSPError1(DSP_EWARNING,
		 DSPCat("DSPGetValueOfSystemSymbol: "
			"Unexpected type '%s' for symbol ",name),
		 dspsym->type);

    return dspsym->value.i;
}


DSPAddress DSPGetLowestInternalUserXAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XLI_USR"); }

DSPAddress DSPGetHighestInternalUserXAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XHI_USR"); }

DSPAddress DSPGetLowestInternalUserYAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("YLI_USR"); }

DSPAddress DSPGetHighestInternalUserYAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("YHI_USR"); }

DSPAddress DSPGetLowestInternalUserPAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("PLI_USR"); }

DSPAddress DSPGetHighestInternalUserPAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("PHI_USR"); }

DSPAddress DSPGetLowestExternalUserXAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XLE_USR"); }

DSPAddress DSPGetHighestExternalUserXAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XHE_USR"); }

DSPAddress DSPGetLowestExternalUserYAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("YLE_USR"); }

DSPAddress DSPGetHighestExternalUserYAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("YHE_USR"); }

DSPAddress DSPGetLowestExternalUserPAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("PLE_USR"); }

DSPAddress DSPGetHighestExternalUserPAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("PHE_USR"); }

DSPAddress DSPGetHighestExternalUserAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("HE_USR"); }

DSPAddress DSPGetLowestExternalUserAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("LE_USR"); }

DSPAddress DSPGetLowestDegMonAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("DEGMON_L"); }

DSPAddress DSPGetLowestXYPartitionUserAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XLE_USG"); }

DSPAddress DSPGetHighestXYPartitionXUserAddress(void) 
{ return (DSPAddress)DSPGetValueOfSystemSymbol("XHE_USG"); }

DSPAddress DSPGetHighestXYPartitionYUserAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("YHE_USG"); }

DSPAddress DSPGetHighestXYPartitionUserAddress(void)
{ 
    return MIN(DSPGetHighestXYPartitionXUserAddress(), 
	       DSPGetHighestXYPartitionYUserAddress()); 
}

DSPAddress DSPGetHighestDegMonAddress(void)
{ return (DSPAddress)DSPGetValueOfSystemSymbol("DEGMON_H"); }

DSPAddress DSPMKGetClipCountAddress(void)
{ return (DSPAddress)DSPMK_X_NCLIP; }

/************************** TRANSFERS FROM THE DSP ***************************/

int DSPMKRetValueTimed(
    DSPTimeStamp *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value)
{
    int vallo,valhi;
    int msTimeLimit = 0;		/* in milliseconds */
    int opcode;

    switch(space) {
      case DSP_MS_P:	
	opcode = DSP_HM_PEEK_P;
	break;
      case DSP_MS_X:
	opcode = DSP_HM_PEEK_X;
	break;
      case DSP_MS_Y:
	opcode = DSP_HM_PEEK_Y;
	break;
    default:
	return(_DSPError1(EDOM,
			  "DSPMKRetValueTimed: cannot send memory space: "
			  "%s", (char *) DSPMemoryNames[(int)space]));
    }
    DSP_UNTIL_ERROR(DSPMKCallTimedV(aTimeStampP,opcode,1,address));
    DSP_UNTIL_ERROR(DSPMKFlushTimedMessages());
    
    if (DSPIsSimulatedOnly())
      return(-1);

    if(DSPAwaitUnsignedReply(DSP_DM_PEEK0,&vallo,msTimeLimit))
      return(_DSPError(DSP_ESYSHUNG,
		       "DSPMKRetValueTimed: No reply to timed peek"));
    if(DSPAwaitUnsignedReply(DSP_DM_PEEK1,&valhi,msTimeLimit))
      return(_DSPError(DSP_ESYSHUNG,
		       "DSPMKRetValueTimed: Hi word of timed peek never came"));
    *value = (valhi<<16) | vallo;
    return(0);
}


int DSPMKRetValue(
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value)
{
    return(DSPMKRetValueTimed(&DSPMKTimeStamp0,space,address,value));
}


int DSPReadValue(
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *value)
{
    return(DSPMKRetValueTimed(DSPMK_UNTIMED,space,address,value));
}


DSPFix24 DSPGetValue(
    DSPMemorySpace space,
    DSPAddress address)
{
    int ec, count = 1, skipFactor = 1;
    DSPFix24 datum;

    if (ec = DSPReadArraySkipMode(&datum,space,address,skipFactor,
				  count,DSP_MODE32))
      _DSPError(ec,"DSPGetValue: DSP read failed");
    return datum;
}


int DSPReadArray(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int wordCount)		/* from DSP perspective */
{
    static int warned=0;
    if(!warned) {
	_DSPError(0,"Note: DSPReadArray() has been superceded by "
		  "DSPReadFix24Array() (which has a skipFactor argument)");
	warned=1;
    }
    /* See DSPObject.c */
    return(DSPReadArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,1,
			   wordCount,DSP_MODE32));
}


int DSPReadFix24Array(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c */
    return(DSPReadArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32));
}


int DSPReadFix24ArrayLJ(
    DSPFix24 *data,		/* array to fill from DSP */
    DSPMemorySpace memorySpace, /* from <dsp/dsp_structs.h> */
    DSPAddress startAddress,	/* within DSP memory */
    int skipFactor,		/* 1 means normal contiguous transfer */
    int wordCount)		/* from DSP perspective */
{
    /* See DSPObject.c */
    return(DSPReadArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,
			   skipFactor,wordCount,DSP_MODE32_LEFT_JUSTIFIED));
}


int DSPReadPackedArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per DSP word read */
    int wordCount)		/* DSP words = byte count / 3 */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,
				memorySpace,startAddress,skipFactor,
				wordCount,DSP_MODE24));
}


int DSPReadByteArray(
    unsigned char *data,	/* Data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per byte transferred */
    int byteCount)		/* Same as DSP word count */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,
				memorySpace,startAddress,skipFactor,
				byteCount,DSP_MODE8));
}


int DSPReadShortArray(
    short int *data,		/* Packed data to fill from DSP */
    DSPMemorySpace memorySpace, /* DSP memory space */
    DSPAddress startAddress,	/* DSP start address */
    int skipFactor,		/* DSP index increment per array element */
    int wordCount)		/* DSP word count = byte count / 2 */
{	
    return(DSPReadArraySkipMode((DSPFix24 *)data,memorySpace,startAddress,
			       skipFactor,wordCount,DSP_MODE16));
}


int DSPReadIntArray(
    int *intArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadFix24Array(intArray,memorySpace,startAddress,
			  skipFactor,wordCount);
    ec = DSPFix24ToIntArray(intArray,intArray,wordCount);
    return(ec);
}

    
int DSPReadIntArrayXY(
    int *intArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode(intArray,memorySpace,startAddress,skipFactor,
			wordCount,DSP_MODE32);
    ec = DSPFix24ToIntArray(intArray,intArray,wordCount);
    return(ec);
}

    
int DSPReadFloatArray(
    float *floatArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)floatArray,memorySpace,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToFloatArray((DSPFix24 *)floatArray,floatArray,wordCount);
    return(ec);
}


int DSPReadFloatArrayXY(
    float *floatArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)floatArray,memorySpace,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToFloatArray((DSPFix24 *)floatArray,floatArray,wordCount);
    return(ec);
}


int DSPReadDoubleArray(
    double *doubleArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)doubleArray,memorySpace,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToDoubleArray((DSPFix24 *)doubleArray,doubleArray,wordCount);
    return(ec);
}


int DSPReadDoubleArrayXY(
    double *doubleArray,
    DSPMemorySpace memorySpace,
    DSPAddress startAddress,
    int skipFactor,
    int wordCount)
{
    if (wordCount<=0)
      return(0);
    ec = DSPReadArraySkipMode((DSPFix24 *)doubleArray,memorySpace,startAddress,
			 skipFactor,wordCount,DSP_MODE32);
    if (ec) return(ec);
    ec = DSPFix24ToDoubleArray((DSPFix24 *)doubleArray,doubleArray,wordCount);
    return(ec);
}

/************************ INTERACTIVE DEBUGGING SUPPORT **********************/

int _DSPPrintDatum(
    FILE *fp,
    DSPFix24 word)
{
    unsigned int usword; 
    int sword; 
    float fword;

    usword = word;
    if (word & (1<<23)) 
      usword |= (0xFF << 24); /* ignore overflow */
    sword = usword; /* re-interpret as signed */
    fword = DSPIntToFloat(sword);
    fprintf(fp,"0x%06X = %-8d = %10.8f\n", word,usword,fword);

    return 0;
}


int _DSPPrintValue(
    DSPMemorySpace space,
    DSPAddress address)
{
    return _DSPPrintDatum(stderr,DSPGetValue(space,address));
}


int _DSPDump(
    char *name)
{
    int i,j,ec, address, count, skipFactor = 1;
    DSPMemorySpace spc;
    DSPFix24 data[8192];
    char *fn, *spcn;
    FILE *fp;

    if (DSPMKIsWithSoundOut() || DSPMKWriteDataIsEnabled())
      DSPMKStopSoundOut();

    address = 0;
    spc = DSP_MS_Y;

    for (i=0; i<4; i++) {
	switch (i) {
	case 0:			/* X */
	    spc = DSP_MS_X;
	case 1:			/* Y */
	    count = 256;
	    break;
	case 2:			/* L --> external Y */
	    address = 512;
	    count = 8192-512;
	    break;
	case 3:			/* P */
	    count = 512;
	    break;
	}
	if (ec = DSPReadArraySkipMode(data,spc,address,skipFactor,
				      count,DSP_MODE32))
	  _DSPError(ec,"_DSPDump: DSP read failed");
	spcn = (i==2? "E":(char *)DSPMemoryNames[i]);
	fn = _DSPCat(name,_DSPCat(spcn,".ram"));
	fp = _DSPMyFopen(fn,"w");
	for (j=0;j<count;j++) {
	    fprintf(fp,"%c[0x%04X=%-5d] = ",
	    spc,j+address,j+address);
	    _DSPPrintDatum(fp,data[j]);
	}
	fclose(fp);
    }
    return(0);
}


/************************ UNTIMED TRANSFERS WITHIN DSP ***********************/

/* Currently only MK monitor has this service */

int DSPMKBLT(
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(DSPMK_UNTIMED,memorySpace,sourceAddr,1,
			       destinationAddr,1,wordCount));
}

int DSPMKBLTB(
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(DSPMK_UNTIMED,memorySpace,
			   sourceAddr+wordCount-1,NEGATIVE1,
			   destinationAddr+wordCount-1,NEGATIVE1,
			   wordCount));
}

/**************************** TIMED TRANSFERS TO DSP ***********************/

/* (Plus untimed special cases of the timed transfers) */

/* KEEP FOR 1.0 MK BINARY COMPATIBILITY */
int _DSPSendValueTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    int addr,
    int value)
{
    return DSPMKSendValueTimed(aTimeStampP, value, space, addr);
}

int DSPMKSendValueTimed(
    DSPFix48 *aTimeStampP,
    int value,
    DSPMemorySpace space,
    int addr)
{
    DSPAddress opcode;
    DSPFix24 cvalue = DSP_FIX24_CLIP(value);

    if (cvalue != value && ((value|0xFFFFFF) != -1))
      _DSPError1(DSP_EFPOVFL,
		     "DSPMKSendValueTimed: Value 0x%s overflows 24 bits",
		     _DSPCVHS(value));

    if (DSPIsSimulated()) 
      fprintf(DSPGetSimulatorFP(),
	      ";; Send value 0x%X = `%d = %s to %s:$%X %s\n",
	      value,value,DSPFix24ToStr(value),
	      DSPMemoryNames[space],addr,
	      DSPTimeStampStr(aTimeStampP));

    switch(space) {
      case DSP_MS_P:	
	opcode = DSP_HM_POKE_P;
	break;
      case DSP_MS_X:
	opcode = DSP_HM_POKE_X;
	break;
      case DSP_MS_Y:
	opcode = DSP_HM_POKE_Y;
	break;
    default:
	return(_DSPError1(EDOM,
			  "DSPMKSendValueTimed: cannot send memory space: "
			  "%s", (char *) DSPMemoryNames[(int)space]));
    }
    return DSPMKCallTimedV(aTimeStampP,opcode,2,value,addr);
    /* address must be atop value in arg stack */
    /* reason: address takes 2 cycles to set up */
}

int DSPMKSendValue(int value, DSPMemorySpace space, int addr)
{
    return DSPMKSendValueTimed(&DSPMKTimeStamp0,value,space,addr);
}

/* KEEP FOR 1.0 BINARY COMPATIBILITY */
int DSPWriteValue1p0(DSPMemorySpace space, int addr, int value)
{
    return DSPMKSendValueTimed(DSPMK_UNTIMED,value,space,addr);
}

int DSPWriteValue(int value, DSPMemorySpace space, int addr)
{
    return DSPMKSendValueTimed(DSPMK_UNTIMED,value,space,addr);
}

/* KEEP FOR 1.0 BINARY COMPATIBILITY */
int _DSPSendLongTimed(
    DSPFix48 *aTimeStampP,
    int addr,
    DSPFix48 *aFix48Val)
{
    return DSPMKSendLongTimed(aTimeStampP, aFix48Val, addr);
}

int DSPMKSendLongTimed(
    DSPFix48 *aTimeStampP,
    DSPFix48 *aFix48Val,
    int addr)
{
    DSP_UNTIL_ERROR(DSPMKSendValueTimed(aTimeStampP,
				 aFix48Val->high24,
				 DSP_MS_X,addr));

    DSP_UNTIL_ERROR(DSPMKSendValueTimed(aTimeStampP,
				 aFix48Val->low24,
				 DSP_MS_Y,addr));
    return(0);
}

int DSPMKSendLong(DSPFix48 *aFix48Val, int addr)
{
    return DSPMKSendLongTimed(&DSPMKTimeStamp0,aFix48Val,addr);
}

/* KEEP FOR 1.0 BINARY COMPATIBILITY */
int DSPWriteLong1p0(int addr, DSPFix48 *aFix48Val)
{
    return DSPMKSendLongTimed(DSPMK_UNTIMED,aFix48Val,addr);
}

int DSPWriteLong(DSPFix48 *aFix48Val, int addr)
{
    return DSPMKSendLongTimed(DSPMK_UNTIMED,aFix48Val,addr);
}

int DSPMKMemoryFillSkipTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 fillConstant,	/* value to fill memory with */
    DSPMemorySpace space,	/* space of memory fill in DSP */
    DSPAddress address,		/* first address of fill in DSP memory	*/
    int skip,			/* skip factor in DSP memory */
    int wordCount)		/* number of DSP memory words to fill */
{
    DSPAddress opcode;
    if (wordCount <= 0) return(0);
    if (DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),
	      ";; Fill %d words %s:$%X:%d:$%X with $%X %s\n",
	      wordCount,DSPMemoryNames[space],
	      address,skip,address+skip*wordCount-1,fillConstant,
	      DSPTimeStampStr(aTimeStampP));

    switch(space) {
      case DSP_MS_P:	
	opcode = DSP_HM_FILL_P;
	break;
      case DSP_MS_X:
	opcode = DSP_HM_FILL_X;
	break;
      case DSP_MS_Y:
	opcode = DSP_HM_FILL_Y;
	break;
      default:
	return _DSPError1(EDOM,
			  "DSPMKMemoryFillTimed: "
			  "can't fill memory space %s",
			  (char *) DSPMemoryNames[(int)space]);
    }
    /*** FIXME: Add skip factor argument to DSP routines when ready ***/
    if (skip != 1) {
	fprintf(stderr,"DSPMKMemoryFillSkipTimed: "
		"Skip factor not yet implemented\n");
	exit(1);
    }

    return DSPMKCallTimedV(aTimeStampP,opcode,3,wordCount,fillConstant,address);
    /* want wordCount at stack bottom for in-place use */
    /* address must be atop value in arg stack	*/
    /* reason: address takes 2 cycles to set up */
}

int DSPMKMemoryFillTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 fillConstant,
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKMemoryFillSkipTimed(aTimeStampP,fillConstant,
				  space,address,1,count);
}

int DSPMKSendMemoryFill(
    DSPFix24 fillConstant,
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKMemoryFillTimed(&DSPMKTimeStamp0,fillConstant,
				  space,address,count);
}

int DSPMKMemoryClearTimed(
    DSPFix48 *aTimeStampP, 
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKMemoryFillTimed(aTimeStampP,0,space,address,count);
}

int DSPMKSendMemoryClear(
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKMemoryClearTimed(&DSPMKTimeStamp0,space,address,count);
}

int DSPMKClearDSPSoundOutBufferTimed(DSPTimeStamp *aTimeStampP)
{
    return DSPMKMemoryFillTimed(aTimeStampP,0,DSP_MS_Y,DSPMK_YB_DMA_W,
				2*DSPMKSoundOutDMASize());
}

int DSPMKSendArraySkipModeTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 *data,	/* See DSPObject.c(DSPWriteArraySkipMode) for interp */
    DSPMemorySpace space,
    DSPAddress address,
    int skipFactor,
    int count,		/* DSP wdcount (e.g. 1 for each byte in DSP_MODE8) */
    int mode)		/* from <nextdev/dspvar.h> */
{
    register int sim,mapped;
    int nwds;
    
    if (DSPMK_NB_HMS != DSPAP_NB_HMS)
      return _DSPError(DSP_EMISC,"DSPMKSendArraySkipModeTimed: "
		       "DSPMK_NB_HMS != DSPAP_NB_HMS");

#if MAPPED_ONLY_POSSIBLE
    mapped = _DSPMappedOnlyIsEnabled();
#endif

#if SIMULATOR_POSSIBLE
    if (sim = DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),
	      ";; Send %d words to %s:$%X:%d:$%X %s\n",
	      count,DSPMemoryNames[space],
	      address,skipFactor,address+skipFactor*count-1,
	      DSPTimeStampStr(aTimeStampP));
#endif
    
    address--; /* We pass the last address of the buffer to the DSP routine. 
		  So we decrement address now and all works out below. */
    
/*
 * Optimization is omitted here because
 * since the array transfers are already large, we may lose more in the
 * extra handling than we gain from the message combining.  It is important
 * to optimize this function for speed, and skipping optimization gives the
 * most direct path to the output.
 */
    DSPMKFlushTimedMessages();	/* Flush since we're not optimizing here */

/* The following is taken out as an optimization because the Music Kit
   always has parens around its loads */
/*  if (aTimeStampP)
      DSPMKOpenParenTimed(aTimeStampP); /* Don't allow TMQ underrun */

    while (count > 0) {
	nwds = MIN(count,DSPAP_NB_HMS - 9);
	/* The - 9 above is 
	   4 for arguments nwds,skipFactor,address, and space (below).
	   Room (3 words) for accompanying timed host msg already reserved 
	   by DSP in the DM_TMQ_ROOM reply.  We will write nwds+4+3 words
	   to the HMS, and this cannot exceed DSPAP_NB_HMS-2 (HMS size minus
	   2 for the begin-mark and end-mark in the HMS). Hence the 9 above.
	 */

#if SIMULATOR_POSSIBLE
	if (sim)
	  fprintf(DSPGetSimulatorFP(),
		  ";; %d-word Timed Message Data to TX %s\n",
		  nwds,DSPTimeStampStr(aTimeStampP));
#endif

#if TRACE_POSSIBLE
	if (_DSPTrace & DSP_TRACE_TMQ)
	  printf("msg = %3d words --> TMQ\n",nwds);
#endif

	_DSPStartHmArray();
	_DSPExtendHmArrayMode(data,nwds,mode);
	_DSPExtendHmArray(&nwds,1);
	_DSPExtendHmArray(&skipFactor,1);
	address += nwds;
	_DSPExtendHmArray(&address,1);
	_DSPExtendHmArray(&((DSPFix24)space),1);
	_DSPFinishHmArray(aTimeStampP,DSP_HM_POKE_N);
	if(DSPErrorNo=_DSPWriteHm())
	  return(DSPErrorNo);

	count -= nwds;
	data += nwds;
    }

/*  if (aTimeStampP)
      DSPMKCloseParenTimed(aTimeStampP); */
    return DSPErrorNo;
}

/* KEEP FOR 1.0 BINARY COMPATIBILITY */
int _DSPSendArraySkipTimed(
    DSPFix48 *aTimeStampP,
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    int skipFactor,
    int count)
{
    return DSPMKSendArraySkipModeTimed(aTimeStampP,data,space,address, 
    				      skipFactor,count,DSP_MODE32);
}


int DSPMKSendArraySkipTimed(
    DSPFix48 *aTimeStampP,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    DSPMemorySpace space,
    DSPAddress address,
    int skipFactor,
    int count)
{
    return DSPMKSendArraySkipModeTimed(aTimeStampP,data,space,address, 
    				      skipFactor,count,DSP_MODE32);
}
    
int DSPMKSendArrayTimed(
    DSPFix48 *aTimeStampP, 
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKSendArraySkipModeTimed(aTimeStampP,data,space,address,1,
				       count,DSP_MODE32);
}

int DSPMKSendShortArraySkipTimed(
    DSPFix48 *aTimeStampP,
    short int *data,  /* 2 DSP words get left and right 16 bits of data word */
    DSPMemorySpace space,
    DSPAddress address,
    int skipFactor,
    int count)
{
    return DSPMKSendArraySkipModeTimed(aTimeStampP,(DSPFix24 *)data,
				      space,address, 
    				      skipFactor,count,DSP_MODE16);
}
    
/* FIXME - order should be (timeStamp,data,space,addr,count) */
int _DSPMKSendUnitGeneratorWithLooperTimed(
    DSPFix48 *aTimeStampP, 
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    int count,
    int looperWord)
{
    register int sim,mapped;
    int z,nwds,skipFactor=1;
    
    mapped = _DSPMappedOnlyIsEnabled();

    if (sim = DSPIsSimulated())
      fprintf(DSPGetSimulatorFP(),
	      ";; Send %d words plus 'jmp 0x%X' word to %s:$%X:$%X %s\n",
	      count,looperWord & 0xfff,DSPMemoryNames[space],
	      address,address+count-1+1,
	      DSPTimeStampStr(aTimeStampP));
    
    address--; /* We pass the last address of the buffer to the DSP routine. 
		  So we decrement address now and everything works out below.*/

    DSPMKFlushTimedMessages();	/* Flush to preserve order since no opt */

/*  if (aTimeStampP)
      DSPMKOpenParenTimed(aTimeStampP); /* Don't allow TMQ underrun */

    while (count > 0) {
	/* See DSPMKSendArraySkipTimed: */
	nwds = MIN(count,DSPAP_NB_HMS - 9);	
	if (sim)
	  fprintf(DSPGetSimulatorFP(),
		  ";; %d-word Timed Message Data to TX %s\n",
		  nwds,DSPTimeStampStr(aTimeStampP));
	if (_DSPTrace & DSP_TRACE_TMQ)
	  printf("msg = %3d words --> TMQ\n",nwds);

	_DSPStartHmArray();
	_DSPExtendHmArray(data,nwds);
	if (count - 2 <= nwds) {  
	    z = 0;
	    _DSPExtendHmArray(&z,1); 
	    /* Need NOP if last UG ends with DO loop */
	    _DSPExtendHmArray(&looperWord,1);
	    nwds += 2;	/* account for new words */
	}
	_DSPExtendHmArray(&nwds,1);
	_DSPExtendHmArray(&skipFactor,1);
	address += nwds;
	_DSPExtendHmArray(&address,1);
	_DSPExtendHmArray(&((DSPFix24)space),1);
	_DSPFinishHmArray(aTimeStampP,DSP_HM_POKE_N);
	if(DSPErrorNo=_DSPWriteHm())
	  return(DSPErrorNo);

	count -= nwds;
	data += nwds;
	
    }

/*  if (aTimeStampP)
      DSPMKCloseParenTimed(aTimeStampP); */

    return DSPErrorNo;
}


/* Special hack for atomically piggybacking nops to UG code in music kit */
/* FIXME - Flush? Does anyone use it? */
/* FIXME - order should be (timeStamp,data,space,addr,count) */
int _DSPMKSendTwoArraysTimed(
    DSPFix48 *aTimeStampP, 
    DSPMemorySpace space,
    DSPAddress address,
    DSPFix24 *data1,
    int count1,
    DSPFix24 *data2,
    int count2)
{
    DSPFix24 *data;
    int ec,count,i;
    count = count1 + count2;
    data = (DSPFix24 *) alloca( count * sizeof(DSPFix24) );
    for (i=0;i<count1;i++)
      data[i] = data1[i];
    for (i=0;i<count2;i++)
      data[count1+i] = data2[i];
    ec = DSPMKSendArraySkipModeTimed(aTimeStampP,data,space,address,1,
				     count,DSP_MODE32);
    return(ec);
}

int DSPMKSendArray(
    DSPFix24 *data,		/* DSP gets rightmost 24 bits of each word */
    DSPMemorySpace space,
    DSPAddress address,
    int count)
{
    return DSPMKSendArraySkipModeTimed(&DSPMKTimeStamp0,data,space,address,1,
				       count,DSP_MODE32);
}

/************************ TIMED TRANSFERS WITHIN DSP ***********************/

int DSPMKBLTSkipTimed(
    DSPFix48 *timeStamp,
    DSPMemorySpace memorySpace,
    DSPAddress srcAddr,
    DSPFix24 srcSkip,
    DSPAddress dstAddr,
    DSPFix24 dstSkip,
    int wordCount)
{
    DSPAddress opcode;
    
    if (wordCount <= 0) return(0);
    if (DSPIsSimulated()) {
	int sourceSkip, destinationSkip;	  /* DAJ */
	sourceSkip = DSPFix24ToInt(srcSkip);	  /* These may be negative. */
	destinationSkip = DSPFix24ToInt(dstSkip); 
	fprintf(DSPGetSimulatorFP(),
		";; BLT %d words from %s:$%X:%d:$%X to %s:$%X:%d:$%X\n",
		wordCount,
		DSPMemoryNames[memorySpace],
		srcAddr,
		sourceSkip,
		srcAddr+wordCount*sourceSkip-1,
		DSPMemoryNames[memorySpace],
		dstAddr,
		destinationSkip,
		dstAddr+wordCount*destinationSkip-1);
    }
    switch(memorySpace) {
      case DSP_MS_P:	
	opcode = DSP_HM_BLT_P;
	break;
      case DSP_MS_X:
	opcode = DSP_HM_BLT_X;
	break;
      case DSP_MS_Y:
	opcode = DSP_HM_BLT_Y;
	break;
      default:
	return(_DSPError1(EDOM,
	   "DSPMKBLTSkipTimed: cannot BLT memory space: %s",
			  (char *)DSPMemoryNames[(int)memorySpace]));
    }

    return (DSPMKCallTimedV(timeStamp,opcode,5,wordCount,
			  srcAddr,srcSkip,dstAddr,dstSkip));
}

int DSPMKBLTTimed(
    DSPFix48 *timeStamp,
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(timeStamp,memorySpace,sourceAddr,1,
			       destinationAddr,1,wordCount));
}

int DSPMKBLTBTimed(
    DSPFix48 *timeStamp,
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(timeStamp,memorySpace,
			   sourceAddr+wordCount-1,NEGATIVE1,
			   destinationAddr+wordCount-1,NEGATIVE1,
			   wordCount));
}

int DSPMKSendBLT(
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(&DSPMKTimeStamp0,memorySpace,sourceAddr,1,
			       destinationAddr,1,
			       wordCount));
}

int DSPMKSendBLTB(
    DSPMemorySpace memorySpace,
    DSPAddress sourceAddr,
    DSPAddress destinationAddr,
    int wordCount)
{
    return(DSPMKBLTSkipTimed(&DSPMKTimeStamp0,memorySpace,
			   sourceAddr+wordCount-1,-1,
			   destinationAddr+wordCount-1,-1,
			   wordCount));
}

