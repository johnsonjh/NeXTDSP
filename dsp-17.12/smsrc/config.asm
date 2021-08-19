; config.asm - specify assembly and run-time configuration
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;

DSP_8K	  EQU  1	      ; NeXT hardware, 8K RAMS
DSP_SIM	  EQU  2	      ; DSP56000 simulator (can run systemless)
DSP_32K	  EQU  8	      ; NeXT hardware after upgrade to 32K DSP RAMs

; *** Mnemonics for the various DSP target environments ***

;    Set nonzero to issue TXD interrupt ('040 only) when HF3 clears
     define USE_TXD_INTERRUPT '0'

     define DEGMON '1'
     define SIMULATING '(DSP_TYPE&DSP_SIM)'
     define NEXT_CASE '(DSP_TYPE&DSP_8K||DSP_TYPE&DSP_32K)'
     define NEXT_HARDWARE '(DSP_TYPE==DSP_8K||DSP_TYPE==DSP_32K)'
     define NEXT_UPGRADE '(DSP_TYPE&DSP_32K)'
     define HAVE_SYSTEM '(ASM_SYS||!(DSP_TYPE==DSP_SIM))'

; *** Control of debug info ***

; With I_NPE_SYS==1800,
; $46 free = 72 if SYS_DEBUG is true
; $CA free = 202 if SYS_DEBUG is false

     if !@def(TMQU_MSGS)
TMQU_MSGS set 0		      	; Set nonzero to obtain "TMQ underrun" messages
     endif

     if !@def(FAST_AND_LOOSE)
FAST_AND_LOOSE 	set 1	      ; 1 => skip various safety checks (when debugged)
			      ; e.g. inhibits run-time UG arg pointer checking
			      ; It also inhibits clip-count maintenance.
			      ; Inhibits error checking in handlers:jsr_hm.
     endif

     if !@def(SYS_DEBUG)
SYS_DEBUG set 0		      	; Set nonzero to emit runtime debugging code
     endif			; enables HMS health check in handlers(xhm)
				; enables TMQ underrun messages
				; enables reserving bottom 2 stack locations

     if !@def(UG_DEBUG)
UG_DEBUG set 0	      		; Set nonzero to emit runtime UG debugging code
     endif

     if UG_DEBUG
SYS_DEBUG set 1	      		; UG_DEBUG implies SYS_DEBUG
	msg 'UG_DEBUG is on => extra run-time code in unit generators'
     endif

     if SYS_DEBUG
	msg 'SYS_DEBUG is on => extra debug run-time code in DSP system'
     endif

     if !@def(MSG_REMINDERS)
MSG_REMINDERS set  0	      ; Set nonzero to emit assembly-time reminders
     endif

     if MSG_REMINDERS
	  define remember 'msg'
     else
	  define remember ';'
     endif

     if !@def(NO_MESSAGES)
NO_MESSAGES set  1	      ; Zero means print configuration info
     endif

     if !@def(NO_WARNINGS)
NO_WARNINGS set  0	      ; Zero means print warnings
     endif

     if NO_WARNINGS
	opt now
     endif

     if NO_MESSAGES
;*	  opt now
	  define message ';'
     else
	  define message 'msg'
     endif

; *** Mnemonics for selecting a case ***
; Setting one of these in the source file which includes this one 
; will set DSP_TYPE to what it should be for that case. Otherwise,
; the mnemonic is defined now as the appropriate bit test in DSP_TYPE,
; and it can be used in subsequent code to test what case we're in.

     if !@def(SIMULATOR)  ; Pure simulator (for fast ap,ug development)
       define SIMULATOR '(DSP_TYPE==2)' ; no other bits can be on
     else
DSP_TYPE set DSP_SIM
     endif

     if !@def(NEXT_8K)
       define NEXT_8K '(DSP_TYPE&1==1)'		; Use: if NEXT_8K ...
     else
DSP_TYPE set DSP_8K				; User set NEXT_8K a priori
     endif

     if !@def(NEXT_32K)
       define NEXT_32K '(DSP_TYPE&8==8)'
     else
DSP_TYPE set DSP_32K
     endif

; ** versions including simulator **

     if !@def(SIM_8K)
       define SIM_8K '(DSP_TYPE==DSP_8K+DSP_SIM)'
     else
DSP_TYPE set DSP_8K+DSP_SIM
     endif

     if !@def(SIM_32K)
       define SIM_32K '(DSP_TYPE==DSP_32K+DSP_SIM)'
     else
DSP_TYPE set DSP_32K+DSP_SIM
     endif

; ** DEFAULT CASE **

     if !@def(DSP_TYPE)
DSP_TYPE  set  DSP_8K		      ; Default DSP target environment
     endif

     if !@def(ASM_SYS)
ASM_SYS	  set  0
     endif

     if !@def(ASM_DEGMON)
ASM_DEGMON set 1	 ; want DEGMON except when loading into BUG56
     endif

     if !@def(ASM_RESET)
ASM_RESET set ASM_DEGMON ; want reset_boot except when loading into BUG56
     endif

; *** Choose whether to support relocatable addresses within a 
;	single-word instruction. (As of 4/18/88, NeXT loader
;	software does not --- each relocatable address must 
;	occupy an entire word.)

	if !@def(ANY_FIXUPS)
ANY_FIXUPS set  0	     ; 0 => force long-mode addressing if relocatable
	endif

; *** Choose memory configuration (see memmap.asm) ***

	if !@def(ONE_MEM)
ONE_MEM	set  0    ; 1 => Shared external memory (x, y, and p)
	endif

	if !@def(SEP_MEM)
SEP_MEM	set  0    ; 1 => Separate external memory (x, y, and p)
	endif

 	if !ONE_MEM&&!SEP_MEM
ONE_MEM	  set  1
	  message 'Default memory map = OVERLAY (ONE_MEM=1)'
	  cobj 'Default memory map = OVERLAY (ONE_MEM=1)'
	endif

 	if ONE_MEM&&!@DEF(XY_SPLIT)
XY_SPLIT set  0 ; When true, xe/ye split down the middle. Else all xe.
	endif

 	if XY_SPLIT
	  message 'XY_SPLIT: X and Y memory divided into equal segments'
	  cobj 'XY_SPLIT: X and Y memory divided into equal segments'
	else
	  message '!XY_SPLIT: All external memory is X'
	  cobj '!XY_SPLIT: All external memory is X'
	endif

     if !@def(READ_DATA)
READ_DATA  set  0	        ; Set nonzero to allocate read-data buffers
     endif			; Some day, all buffers will allocate on fly

      if READ_DATA
	message 'READ DATA enabled'
	cobj 'READ DATA enabled'
      else
	message 'READ DATA disabled'
	cobj 'READ DATA disabled'
      endif

     if !@def(SSI_READ_DATA)
SSI_READ_DATA  set  0	        ; Set nonzero to allocate SSI_READ-data buffers
     endif			; Some day, all buffers will allocate on fly

      if SSI_READ_DATA
	message 'SSI_READ DATA enabled'
	cobj 'SSI_READ DATA enabled'
      else
	message 'SSI_READ DATA disabled'
	cobj 'SSI_READ DATA disabled'
      endif

     if !@def(SSI_READ_DATA)
SSI_READ_DATA  set  0	        ; Set nonzero to allocate SSI_READ-data buffers
     endif			; Some day, all buffers will allocate on fly

      if SSI_READ_DATA
	message 'SSI_READ DATA enabled'
	cobj 'SSI_READ DATA enabled'
      else
	message 'SSI_READ DATA disabled'
	cobj 'SSI_READ DATA disabled'
      endif

     if !@def(AP_MON)
AP_MON  set  0		        ; Set nonzero to assemble array proc monitor
     endif

      if AP_MON
	message 'AP MONITOR enabled'
	cobj 'AP MONITOR enabled'
      else
	message 'AP MONITOR disabled'
	cobj 'AP MONITOR disabled'
      endif

     if !@def(WRITE_DATA_16_BITS)
WRITE_DATA_16_BITS  set  1	; 16-bit right-just. or 24-bit sound out format
     endif

     if !@def(DSP_SOUND)
DSP_SOUND set 0			; assume false
     endif

; =============================================================================
cant_have macro cond	; fatal error if cond is true
	if (cond)
		fail " cond is TRUE!"
	endif
	endm
; =============================================================================
must_have macro cond	; fatal error if cond is false
	if !(cond)
		fail " cond is FALSE!"
	endif
	endm

; ******************* System external y memory buffer sizes ******************

I_SYSEP set 200	      		; System exported entry points (don't change!!)
			      	; Must be even and must accomodate hmlib.asm
			      	; dispatch table. Mach kernel uses these.
			      	; Note that each dispatch occupies 2 words.
I_NDMQ	set 32	      		; Size of DSP Message Queue (power of 2)
;*	warn 'HMS size increased for debugging'
I_NHMS	set 64	      		; Host Message Stack length (power of 2)

	if AP_MON

	if READ_DATA
		fail 'cannot use read-data with AP monitor'
	endif

;; Note for below: We really don't want a TMQ at all in the AP_MON case.
;; However, the DMQ and HMS use modulo storage.  Therefore, we must
;; allocate a TMQ size which is a power of 2 greater than I_SYSEP
;; (the dispatch table) and also greater than the maximum of I_NDMQ
;; and I_NHMS.  Currently (6/1/89), there are 56 wasted locations.
;; These may be recovered by increasing I_SYSTEP to equal I_NTMQ and
;; adding 56 new slots at the BEGINNING of the dispatch table (i.e.,
;; currently assigned dispatch addresses MUST NEVER CHANGE).

I_NTMQ  set 256-I_SYSEP    	; Size of Timed Message Q (power of 2 -I_SYSEP)
I_NDMA	set 0	      		; Samples per DMA transfer (2 of these total)
I_NYE_SYS set I_NHMS+I_NDMQ+I_NTMQ+I_NDMA*2 ; Total external y memory use
I_NLI_SYS set  2 ; No. internal words reserved for system l constants
I_NXI_SYS set  0 ; No. internal words reserved for system x constants
I_NYI_SYS set  0 ; No. internal words reserved for system y constants
I_NPI_SYS set  0 ; No. internal words reserved for system p programs
	if SYS_DEBUG
I_NPE_SYS set  900 	; AP System external p handlers and utilities
	else
I_NPE_SYS set  840 	; AP System external p handlers etc.(from 850, 10/12/90)
	endif
I_NLE_SYS set  0 	; AP System external l constants
I_NXE_SYS set  56 	; AP System external x vars (exact)

I_NPE_USR set 0		; No. off-chip words reserved for user code

	else		; not AP_MON => DSP music monitor

	remember 'We rely on driver asking for available space => no overrun'
I_NTMQ  set 1024-I_SYSEP    	; Size of Timed Message Q (power of 2 -I_SYSEP)
	if READ_DATA
I_NDMA	set 256	      		; Samples per DMA transfer (4 of these total)
	else
I_NDMA	set 512	      		; Samples per DMA transfer (2 of these total)
	endif

I_WD_CHAN set 1			; DMA channel code for write data (0 to 63)
I_WD_TYPE set 1			; DMA type (0,1,2,3) => (256,512,1K,2K) buf siz
I_RD_CHAN set 1			; DMA channel code for read data (0 to 63)
I_RD_TYPE set 1			; DMA type (0,1,2,3) => (256,512,1K,2K) buf siz

	must_have (I_WD_TYPE==I_RD_TYPE) ; because I_NDMA sets both sizes

	must_have ((I_WD_TYPE==0&&I_NDMA==256)||(I_WD_TYPE==1&&I_NDMA==512)||(I_WD_TYPE==2&&I_NDMA==1024)||(I_WD_TYPE==3&&I_NDMA==2048))

	if READ_DATA
I_NYE_SYS set I_NHMS+I_NDMQ+I_NTMQ+I_NDMA*4 ; Total external y memory use
	else
I_NYE_SYS set I_NHMS+I_NDMQ+I_NTMQ+I_NDMA*2 ; Total external y memory use
	endif

I_NPE_USR set 512	      ; No. off-chip words reserved for user code

; *** Music system assembly constants ***

I_NSIGS	  set 8		      ; Number of signal vectors in xi, yi each
I_NCHANS  set 2		      ; Number of audio channels assumed in output
;*I_NTICK  set 8	      ; Samples/tick (must be even and divide NB_DMA)
I_NTICK   equ 16 	      ; Needed for TMQ addr exprs (asm bug texpr.asm)
     if I_NTICK%2!=0
	  fail 'defines.asm: I_NTICK must be even'
;	  also, DO #I_NTICK-1 means 65K iterations if I_NTICK==1.
     endif

I_NLI_SYS set  4 ; No. internal words reserved for system l constants
I_NXI_SYS set  0 ; No. internal words reserved for system x constants
I_NYI_SYS set  0 ; No. internal words reserved for system y constants
I_NPI_SYS set  0 ; No. internal words reserved for system p programs

	if SYS_DEBUG
I_NPE_SYS set  1760 	; System external p handlers and utilities
	else
I_NPE_SYS set  1650 	; System external p handlers etc (1632 7/4/89)
	endif
	
I_NLE_SYS set  0 	; System external l constants
I_NXE_SYS set  84 	; System external x vars (exact if WRITE_DATA_16_BITS)

MAX_ONCHIP_PATCHPOINTS  EQU 8 ; no. onchip patchpoints allocated by music kit

	  if I_NCHANS*I_NTICK>I_NDMA
	       fail 'I_NCHANS*I_NTICK cannot exceed I_NDMA'
	  endif

	  if I_NDMA%(I_NCHANS*I_NTICK)!=0
	       fail 'I_NCHANS*I_NTICK must divide I_NDMA'
	  endif

 	endif ; AP_MON

; ******************************* Misc. controls ******************************

I_DEGMON_L set $34 	; Start of degmon - MUST BE INTERNAL P MEMORY
DEGMON_N  equ 76  	; Internal p words used by Ariel debug monitor "degmon"
DEGMON_RUN_LOC 		equ $47	; entry point needed when not assembling degmon
DEGMON_TRACER_LOC	equ $59 ; entry point needed when not assembling degmon

		
SYMOBJ_P      	set 1	      ; 0 => no SYMOBJ in allocusr. 1 => do it
I_NTMQ_LWM     	set I_NTMQ/4  ; Send DM_TMQ_LWM when TMQ is only this full
I_NTMQ_ROOM_HWM set I_NTMQ-I_NTMQ_LWM ; this is what we actually use
I_NPE_SCR set  	1 ; Size of scratch area = maximum length of prog in HMS or TMQ


; Special address constants
I_OUTY	  EQU  $FFFF	      ; Y location mapped to output file for simulator

; Special datum constants
I_DEFIPR  EQU  $243C	      ; Default interrupt priority register (p. 8-10)
			      ; irqa=off,sci=0,hif=1,irqb=ssi=2,-edge
I_DEFOMR  EQU  6	      ; Default operating mode register (p. 9-1)
			      ; normal exp. mode (2) + onchip ROM enabled (4)


; ******************************* Config messages *****************************

     cobj 'Copyright 1989, Next Inc.'

     if NEXT_CASE
	  message 'Assembling for NeXT Hardware'
	  cobj 'Assembled for NeXT Hardware'
     endif

     if SIMULATING
	  message 'Assembling for SIMULATOR'
	  cobj 'Assembled for SIMULATOR'
     endif

     if ASM_SYS
	  message 'Including DSP SYSTEM CODE in assembly'
     else
	  message 'Assembling USER ONLY.  Loading system data but not code'
	  cobj 'USER ONLY'
     endif

	if !@DEF(CONTINUOUS_INFILE)
CONTINUOUS_INFILE set 0	; beginend.asm(beg_orch) and jsrlib.asm(service_infile)
	endif

pi_active set 1		; initially assume assembly in internal p
			; this goes away when you can tell what lc
			; (eg p,ph,pl) is in use. See beg_orcl,
			; end_??b in allocusr.

     if I_NTMQ<0
	  fail 'config.asm: dispatch table size must be less than TMQ length'
     endif
