/* dsptest.c - Run simple tests on the DSP

   Link against libdsp.a

*/

#define DO_USER_INITIATED_DMA 0		/*** FIXME ***/

#include <dsp/dsp.h>
#include <nextdev/snd_msgs.h>	/* to define DSP_DEF_BUFSIZE */

extern int rand(void);
extern void exit(int);
extern int _DSPEnableMappedOnly(void);
extern char *_DSPToLowerStr(char *s);
extern int _DSPEnableMappedArrayTransfers(void);
extern int _DSPDisableMappedArrayTransfers(void);
extern int _DSPEnableUncheckedMappedArrayTransfers(void);

DSP_BOOL do_cont = FALSE;	/* if TRUE, continue after errors */

void test_fail(char *msg)
{
    fprintf(stderr,"*** dsptest:%s\n\n",msg);
    if (!do_cont)
      exit(1);
}

DSPFix24 arr1[DSP_EXT_RAM_SIZE];
DSPFix24 arr2[DSP_EXT_RAM_SIZE];

DSPLoadSpec *dspSystem;

int check_readback(size)
    int size;
{
    int i,j;
    for (i=0;i<size;i++) {
	if (arr1[i] != arr2[i]) {
	    fprintf(stderr,"*** at i=%d: sent 0x%X but read back 0x%X\n",
		    i,arr1[i],arr2[i]);
	    if (!do_cont)
	      break;
	}
    }
    if (i != size) {
	if (do_cont) 
	  return(-1);
	else {
	    fprintf(stderr," failed at i=%d.\n",i);
	    DSPClose();
	    exit(1);
	}
    } else 
      fprintf(stderr," ... wins.\n");
    
    for (j=0;j<size;j++) 
      arr2[j] = 0;

    return (i != size);
}

void main(argc,argv) 
    int argc; char *argv[]; 
{
    int ec,i,j;
    double dsptime;
    DSPFix48 dspt;
    DSP_BOOL do_sim = FALSE;	/* if TRUE, write out host-interface file */
    int kase = 0;		/* for sequencing through test cases */
    int ver=0,rev=0;
    int dtr,dtw;
    double rate;
    int r_oh,w_oh;
    extern int DSPDefaultTimeLimit;
    extern int _DSPVerbose;
    extern int _DSPTrace;
 
    DSPDefaultTimeLimit=0;	/* Wait forever. (Factory sees 1 second time out!) */
    
    DSPEnableErrorFile("/dev/tty"); 
    _DSPDisableMappedArrayTransfers();	/* change default to not */

    while (--argc && **(++argv) == '-') {
	_DSPToLowerStr(++(*argv));
	switch (**argv) {
	case 'k':
	    do_cont=TRUE;	/* -k */
	    break;
	case 'v':
	    _DSPVerbose=!_DSPVerbose;	/* -verbose (toggle) */
	    break;
	case 's':
	    do_sim=TRUE;	/* -simulate */
	    break;
	case 'm':
	    if (*(argv[0]+1)=='o') { /* -mo */
		_DSPEnableMappedOnly();
		fprintf(stderr,"Executing DSP tests in MAPPED-ONLY mode.\n");
	    } else {		/* -m<any> */
		_DSPEnableMappedArrayTransfers();	/* -arrays_mapped */
		fprintf(stderr,"Using MEMORY-MAPPED array transfers.\n");
	    }
	    if (*(argv[0]+1)=='u') { /* -mu */
		_DSPEnableUncheckedMappedArrayTransfers(); 
		fprintf(stderr,"Using UNCHECKED (!) MEMORY-MAPPED "
			"array transfers.\n");
	    }
	    break;
	case 't':		/* -trace n */
	    _DSPTrace = (--argc)? strtol((char *)(*(++argv)),(int)NULL,0) : -1;
	    fprintf(stderr,"_DSPTrace set to 0x%X.\n",_DSPTrace);
	    break;
	default:
	    fprintf(stderr,"Unknown switch -%s\n",*argv);
	    exit(1);
	}
    }

    if (do_sim)
      if(DSPErrorNo=DSPOpenSimulatorFile("dsptest.io"))
	test_fail("Could not open simulator output file:dsptest.io");

    /* *** NEED THIS TO GET ERROR MESSAGES ***	DSPEnableHostMsg(); */

 loopBack:
    
    if (kase==0) {
	fprintf(stderr,"Booting DSP with AP monitor\n");
	if (DSPReadFile(&dspSystem,DSP_AP_SYSTEM_BINARY_0))
	  test_fail("DSPReadFile() failed for AP system.");
    } else {
	fprintf(stderr,"Booting DSP with MK monitor\n");
	if (DSPReadFile(&dspSystem,DSP_MUSIC_SYSTEM_BINARY_0))
	  test_fail("DSPReadFile() failed for music system.");
    }

    if (DSPBoot(dspSystem))
      test_fail("DSPBoot(dspSystem) failed.");

    if(DSPCheckVersion(&ver,&rev))
      test_fail("DSPCheckVersion() test failed.");
    else
      fprintf(stderr,"DSP is running system %d.0(%d).\n",ver,rev);

#if 0
    if(_DSPReadTime(&dspt))
      fprintf(stderr,"Could not read DSP time.\n");
    else {
	dsptime = DSPFix48ToDouble(&dspt);
	fprintf(stderr,"DSP time = %d samples = %f seconds = %f minutes.\n",
	       (int)dsptime,dsptime*(1.0/44100.0),
	       dsptime*((1.0/44100.0)/60.0));
    }
#endif

    for (i=0;i<DSP_EXT_RAM_SIZE;i++) arr1[i] = (i-2048);

    /* --- Internal X memory test --- */

    fprintf(stderr,"Unit internal X memory test [x:%d#1]\n",DSPAP_XLI_USR);

    if (DSPWriteIntArray(arr1,DSP_MS_X,DSPAP_XLI_USR,1,1))
      test_fail("Could not write internal X memory");

    DSPGetHostTime();
    if (DSPWriteIntArray(arr1,DSP_MS_X,DSPAP_XLI_USR,1,1))
      test_fail("Could not write internal X memory");
    w_oh = DSPGetHostTime();

    if (DSPReadIntArray(arr2,DSP_MS_X,DSPAP_XLI_USR,1,1))
      test_fail("Could not read internal X memory");

    DSPGetHostTime();
    if (DSPReadIntArray(arr2,DSP_MS_X,DSPAP_XLI_USR,1,1))
      test_fail("Could not read internal X memory");
    r_oh = DSPGetHostTime();

    check_readback(1);

    if (_DSPVerbose)
      fprintf(stderr,
	      " \t DSPWriteIntArray overhead time is %d microseconds\n",
	      w_oh);

    if (_DSPVerbose)
      fprintf(stderr,
	      "\t  DSPReadIntArray overhead time is %d microseconds\n",r_oh);

    fprintf(stderr,"Internal X memory test [x:%d#%d]\n",
	    DSPAP_XLI_USR,DSPAP_NXI_USR);

    if (DSPWriteIntArray(arr1,DSP_MS_X,DSPAP_XLI_USR,1,DSPAP_NXI_USR))
      test_fail("Could not write internal X memory");

    if (DSPReadIntArray(arr2,DSP_MS_X,DSPAP_XLI_USR,1,DSPAP_NXI_USR))
      test_fail("Could not read internal X memory");

    check_readback(DSPAP_NXI_USR);

    /* --- External X memory test --- */

    fprintf(stderr,"External X memory test [x:%d#%d]\n",
	    DSPAP_XLE_USR,DSPAP_NXE_USR);

    DSPGetHostTime();
    if (DSPWriteIntArray(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write external X memory");
    dtw = DSPGetHostTime();

    DSPGetHostTime();
    if (DSPReadIntArray(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read external X memory");
    dtr = DSPGetHostTime();

    check_readback(DSPAP_NXE_USR);

    if (_DSPVerbose)
      fprintf(stderr,"\t DSPWriteIntArray rate is %d kWords per second\n",
	      1000*DSPAP_NXE_USR/dtw);

    if (_DSPVerbose)
      fprintf(stderr,"\t DSPReadIntArray rate is %d kWords per second\n",
	      1000*DSPAP_NXE_USR/dtr);

    /* --- Internal Y memory test --- */

    fprintf(stderr,"Internal Y memory test [y:%d#%d]\n",
	    DSPAP_YLI_USR,DSPAP_NYI_USR);

    for (i=0;i<DSP_EXT_RAM_SIZE;i++) arr1[i] &= 0xFFFFFF; /* no sign ext */

    if (DSPWriteFix24Array(arr1,DSP_MS_Y,DSPAP_YLI_USR,1,DSPAP_NYI_USR))
      test_fail("Could not write internal Y memory");

    if (DSPReadFix24Array(arr2,DSP_MS_Y,DSPAP_YLI_USR,1,DSPAP_NYI_USR))
      test_fail("Could not read internal Y memory");

    check_readback(DSPAP_NYI_USR);


    /* --- External Y memory test --- */

    fprintf(stderr,"External Y memory test [y:%d#%d]\n",
	    DSPAP_YLE_USR,DSPAP_NYE_USR);

    if (DSPWriteFix24Array(arr1,DSP_MS_Y,DSPAP_YLE_USR,1,DSPAP_NYE_USR))
      test_fail("Could not write external Y memory");

    if (DSPReadFix24Array(arr2,DSP_MS_Y,DSPAP_YLE_USR,1,DSPAP_NYE_USR))
      test_fail("Could not read external Y memory");

    check_readback(DSPAP_NYE_USR);


    /* --- External Y memory test, left-justified 1 --- */

    fprintf(stderr,"External left-justified 1 X memory test [x:%d#%d]\n",
	    DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write external X memory left-justified 1");

    if (DSPReadFix24ArrayLJ(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read external X memory left-justified 1");

    for (i=0;i<DSPAP_NXE_USR;i++) arr1[i] <<= 8;

    check_readback(DSPAP_NXE_USR);


    /* --- External X memory test, left-justified 2 --- */

    /* Array 1 is left-justified... send it that way */

    fprintf(stderr,"External left-justified 2 X memory test [x:%d#%d]\n",
	    DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWriteFix24ArrayLJ(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write external X memory left-justified 2");

    if (DSPReadFix24ArrayLJ(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read external X memory left-justified 2");

    check_readback(DSPAP_NXE_USR);

    /* restore to RJ, no sign */
    for (i=0;i<DSP_EXT_RAM_SIZE;i++) arr1[i] = (i-2048) & 0xFFFFFF;

    /* --- Internal P memory test --- */

    fprintf(stderr,"Internal P memory test [p:%d#%d]\n",
	    DSPAP_PLI_USR,DSPAP_NPI_USR);

    if (DSPWriteFix24Array(arr1,DSP_MS_P,DSPAP_PLI_USR,1,DSPAP_NPI_USR))
      test_fail("could not write internal P memory");

    if (DSPReadFix24Array(arr2,DSP_MS_P,DSPAP_PLI_USR,1,DSPAP_NPI_USR))
      test_fail("could not read internal P memory");

    check_readback(DSPAP_NPI_USR);


    /* --- External P memory test --- */

    fprintf(stderr,"External P memory test [p:%d#%d]\n",
	    DSPAP_PLE_USR,DSPAP_NPE_USR);

    if (DSPWriteFix24Array(arr1,DSP_MS_P,DSPAP_PLE_USR,1,DSPAP_NPE_USR))
      test_fail("could not write external P memory");

    if (DSPReadFix24Array(arr2,DSP_MS_P,DSPAP_PLE_USR,1,DSPAP_NPE_USR))
      test_fail("could not read external P memory");

    check_readback(DSPAP_NPE_USR);

#if DSPAP_NXE_USG

    /* --- External partitioned X memory test --- */

    fprintf(stderr,"External X/Y partitioned memory test [x:%d#%d] [y:%d#%d]\n",
	    DSPAP_XLE_USG,DSPAP_NXE_USG, DSPAP_YLE_USG,DSPAP_NYE_USG);

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USG,1,DSPAP_NXE_USG))
      test_fail("Could not write external X memory partitioned segment");

    if (DSPWriteFix24Array(arr1+DSPAP_NXE_USG,DSP_MS_Y,DSPAP_YLE_USG,1,
			DSPAP_NYE_USG))
      test_fail("Could not write external Y memory partitioned segment");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USG,1,DSPAP_NXE_USG))
      test_fail("Could not read external X memory partitioned segment");;

    if (DSPReadFix24Array(arr2+DSPAP_NXE_USG,DSP_MS_Y,DSPAP_YLE_USG,1,
			DSPAP_NYE_USG))
      test_fail("Could not read external Y memory partitioned segment");;

    check_readback(DSPAP_NXE_USG+DSPAP_NYE_USG);

    if (DSPAP_NXE_USG==0)
      fprintf(stderr,"*** NOTE *** X/Y partition test is meaningless because\n"
      "\tthe loaded DSP system completely fills the user X partition\n");

#endif

    /* --- magic sizes test --- */

    if(DSPAP_XLE_USR < DSP_DEF_BUFSIZE) {
	fprintf(stderr,"Default DSP buffer size = %d while external DSP AP "
		"memory room = %d.\n Aborting.\n",
		DSP_DEF_BUFSIZE,DSPAP_XLE_USR);
	exit(1);
    }
	
    /* 513 */

    fprintf(stderr,"Fence-post buffer-size test [x:%d#%d]\n",
	    DSPAP_XLE_USR,DSP_DEF_BUFSIZE+1); /* already did singleton */

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSP_DEF_BUFSIZE+1))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSP_DEF_BUFSIZE+1))
      test_fail("Could not read external X memory");

    check_readback(DSP_DEF_BUFSIZE+1);


    /* 512 */

    fprintf(stderr,"Fence-post buffer-size test [x:%d#%d]\n",
	    DSPAP_XLE_USR,DSP_DEF_BUFSIZE); /* already did singleton */

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSP_DEF_BUFSIZE))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSP_DEF_BUFSIZE))
      test_fail("Could not read external X memory");

    check_readback(DSP_DEF_BUFSIZE);


    /* 1025 */

    fprintf(stderr,"Fence-post buffer-size test [x:%d#%d]\n",
	    DSPAP_XLE_USR,2*DSP_DEF_BUFSIZE+1); /* already did singleton */

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,2*DSP_DEF_BUFSIZE+1))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,2*DSP_DEF_BUFSIZE+1))
      test_fail("Could not read external X memory");

    check_readback(2*DSP_DEF_BUFSIZE+1);


    /* 1024 */

    fprintf(stderr,"Fence-post buffer-size test [x:%d#%d]\n",
	    DSPAP_XLE_USR,2*DSP_DEF_BUFSIZE); /* already did singleton */

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,2*DSP_DEF_BUFSIZE))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,2*DSP_DEF_BUFSIZE))
      test_fail("Could not read external X memory");

    check_readback(2*DSP_DEF_BUFSIZE);


    fprintf(stderr,"Fence-post buffer-size test [x:%d#%d]\n",
	    DSPAP_XLE_USR,3*DSP_DEF_BUFSIZE); /* already did singleton */

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,3*DSP_DEF_BUFSIZE))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,3*DSP_DEF_BUFSIZE))
      test_fail("Could not read external X memory");

    check_readback(3*DSP_DEF_BUFSIZE);


    /*** Non-32-bit-mode tests ***/

    /* +++ Fill test array with random bits +++ */

    for (i=0;i<DSP_EXT_RAM_SIZE;i++)
      arr1[i] = rand();


    /* --- External X memory PACKED 16-BIT TRANSFER test --- */

    fprintf(stderr,"External X memory packed 16-bit mode test [x:%d#%d]\n",
	   DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWriteShortArray((short *)arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write 16-bit data to external X memory");

    if (DSPReadShortArray((short *)arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read 16-bit data from external X memory");

    check_readback(DSPAP_NXE_USR/2);


    /* --- External X memory PACKED 8-BIT TRANSFER test --- */


    fprintf(stderr,"External X memory packed 8-bit mode test [x:%d#%d]\n",
	   DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWriteByteArray((unsigned char *)arr1,
			  DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write 8-bit data to external X memory");

    if (DSPReadByteArray((unsigned char *)arr2,
			 DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read 8-bit data from external X memory");

    check_readback(DSPAP_NXE_USR/4);


    /* --- External X memory UNPACKED 24-BIT TRANSFER test --- */

    for (i=0;i<DSP_EXT_RAM_SIZE;i++)
      arr1[i] &= 0xFFFFFF;

    fprintf(stderr,"External X memory unpacked 24-bit transfer test "
	    "[x:%d#%d]\n", DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWriteFix24Array(arr1,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write external X memory");

    if (DSPReadFix24Array(arr2,DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read external X memory");

    check_readback(DSPAP_NXE_USR);

    /* --- External X memory PACKED 24-BIT TRANSFER test --- */

    for (i=0;i<DSP_EXT_RAM_SIZE;i++)
      arr1[i] = rand();

    fprintf(stderr,"External X memory packed 24-bit transfer test [x:%d#%d]\n",
	   DSPAP_XLE_USR,DSPAP_NXE_USR);

    if (DSPWritePackedArray((unsigned char *)arr1,
			    DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not write external X memory");

    if (DSPReadPackedArray((unsigned char *)arr2,
			   DSP_MS_X,DSPAP_XLE_USR,1,DSPAP_NXE_USR))
      test_fail("Could not read external X memory");

    check_readback(DSPAP_NXE_USR*3/4);

 egress:

    DSPClose();

    if (kase==0) {
	kase = 1;
	fprintf(stderr,"-------------------\n");
	/* FIXME: Need DSPAP vs DSPMK variables before
	   goto loopBack; */
    }
}

