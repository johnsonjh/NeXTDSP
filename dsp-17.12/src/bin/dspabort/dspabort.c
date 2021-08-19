/* dspabort.c - read dsp memory

      To link:

       cc -g dspabort.c -o dspabort -ldsp_g

*/

#include <stdio.h>
#include <dsp/dsp.h>
#include <sys/file.h>
#include <sys/mman.h>

extern int mmap();

/* DSP host-interface registers, as accessed in memory-mapped mode */
typedef volatile struct _DSPRegs {
	unsigned char icr;
	unsigned char cvr;
	unsigned char isr;
	unsigned char ivr;
	union {
		unsigned int	receive;
		struct {
			unsigned char	pad;
			unsigned char	h;
			unsigned char	m;
			unsigned char	l;
		} rx;
		unsigned int	transmit;
		struct {
			unsigned char	pad;
			unsigned char	h;
			unsigned char	m;
			unsigned char	l;
		} tx;
	} data;
} DSPRegs;

DSPRegs *getDSPRegs() 
/* 
 * The following function memory-maps the DSP host interface.
 */
{
    char *alloc_addr;	/* page address for DSP memory map */
    int dsp_fd;

    dsp_fd = open("/dev/dsp",O_RDWR);	/* need <sys/file.h> */
    if (dsp_fd == -1) 
      return(NULL);

    if (( alloc_addr = (char *)valloc(getpagesize()) ) == NULL) 
      return(NULL);

    if ( -1 == mmap(alloc_addr,
	 getpagesize(),		/* Must map a full page */
	 PROT_READ|PROT_WRITE,	/* rw access <sys/mman.h> */
	 MAP_SHARED,		/* shared access (of course) */
	 dsp_fd,		/* This device */
	 0) )			/* 0 offset */
      return(NULL);

    return (DSPRegs *)alloc_addr; /* <dsp/dspstructs.h> */
}

void main(argc,argv) 
    int argc; char *argv[]; 
{
    register int newcvr = 0;
    int val = 0,i;
    DSPRegs *r;

    fprintf(stderr,"Telling DSP to abort via host command....\n");

    r = getDSPRegs();

    fprintf(stderr,"Awaiting HC clear....");
    newcvr = 0x80|DSP_HC_ABORT;
    while(r->cvr & 0x80)
      ;
    r->cvr = newcvr;		/* We are in a race with the driver here */
    fprintf(stderr,"ABORT host command issued.\n");

    fprintf(stderr,"Awaiting host flags 2 and 3....");
    while(!(r->isr&0x8) || !(r->isr&0x10)) /* wait until HF2 and HF3 */
      for(i=0; r->isr&1; i++) {
	  fprintf(stderr,"Flushed 0x%X from DSP RX which could mean \"%s\"\n",
		  val, DSPMessageExpand(val));
	  val = r->data.receive;
	  r->icr = 0;		/* in case driver wakes up */
	  if ((i&&255)==0) {
	      fprintf(stderr,
		      "\nPerhaps the DSP thinks it is finishing off a DMA.\n"
		      "Awaiting HC clear to send DSP_HC_HOST_R_DONE....");
	      newcvr = 0x80|DSP_HC_HOST_RD; /* DSP DMA "read" finished */
	      while(r->cvr & 0x80)
		;
	      r->cvr = newcvr;	/* We are in a race with the driver here */
	      fprintf(stderr,"host command issued.\n");
	  }
	  if (i>1024) {
	      fprintf(stderr,"Could not abort the DSP.\n"
		      "Evidently the DSP monitor is damaged or unknown.\n");
	      exit(1);
	  }
      }

      ;
/*
  Note: As each sound-out buffer completes, the driver will wake up and enable
  DSP messages to it by turning on RREQ.
*/
    sleep(1);			/* Wait for sound-out buffers */
    fprintf(stderr,"DSP is in the aborted state. (RETURN to continue):");
    getchar();
    r->icr = 0x80; 	/* INIT host interface and turn off interrupts */
    while (r->icr & 0x80)
      ;
    fprintf(stderr,"Host interface INITed.\n");

    r->icr = 0x8;		/* Set HF0 to proceed DSP routine abort_now */
    
    fprintf(stderr,"HF0 asserted.\n");

    fprintf(stderr,"Awaiting backtrace and DEGMON start address,,length...\n");
    while(!(r->isr&1))
      r->icr = 0;		/* in case driver wakes up */

    val = r->data.receive;	/* Current stack pointer */
    for(i=0;r->isr&1;i++) {
	fprintf(stderr,"Flushed 0x%X from DSP RX which could mean \"%s\"\n",
		val, DSPMessageExpand(val));
	val = r->data.receive;
	r->icr = 0;		/* in case driver wakes up */
    }

    fprintf(stderr,"DEGMON starts at 0x%X and is %d words long.\n",
	    val&0xFFFF,(val>>16)&0xFF);

#ifdef TEST_BUG56_GRAB
    r->icr = 0x18;		/* set HF1 and HF0 */

    fprintf(stderr,"Awaiting BUG56's DEGMON start address,,length...\n");
    while(!(r->isr&1))
      ;
    val = r->data.receive;

    fprintf(stderr,"DEGMON starts at 0x%X and is %d words long.\n",
	    val&0xFFFF,(val>>16)&0xFF);

#endif

    fprintf(stderr,
    "DSP is aborted and awaiting a grab from (a prelaunched) BUG56.\n");

    exit(0);
}
