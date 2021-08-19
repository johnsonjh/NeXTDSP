; /usr/lib/dsp/smsrc/memmap.asm	- memory-address symbols
;
;; Copyright 1989, NeXT Inc.
;; Author - J. O. Smith
;;
;; Below, 
;;    "internal" means  on-chip dsp memory, and 
;;    "external" means off-chip dsp memory
;;
;; DSP MEMORY MAP
;;    Memory is divided into user memory and system memory.
;;    Memory is also divided into internal and external sections.
;;    Thus, there are four basic sections of p, x, and y memory: 
;;    {user or system} {internal or external} RAM (12 kinds total).
;;
;;    User memory is contiguous in each of the three address spaces x, y, and p.
;;    System memory is split into a low internal block and high external block.
;;
;;    When ONE_MEM is true, external memory is shared among the three banks,
;;    and the high external system blocks (x, y, and p sections)
;;    are concatenated. The memory bounds of these segments (defined below)
;;    provide independent external system x, y, and p blocks; similar
;;    remarks apply to the user external x, y, and p blocks.
;;
;; ---------------------------------------------------------------------------
; MEMORY CONFIGURATION

     if !@DEF(MEM_SIZ)
MEM_SIZ	  set  8192
MEM_OFF	  set  8192 ; external memory address offset
          message 'External 8K memory, 8K base-address OFFSET'
          cobj 'External 8K memory, 8K base-address OFFSET'
     endif

SEG_OFF   set 32768+MEM_OFF ; where x and y are separate banks overlaying  p

; ---------------------------------------------------------------------------
; DEGMON boundaries and size

DEGMON_L  equ I_DEGMON_L  ; Start of degmon - MUST BE INTERNAL P MEMORY
DEGMON_H  equ DEGMON_L+DEGMON_N-1
; ---------------------------------------------------------------------------
; SYSTEM MEMORY RESERVATION SIZES (computed from config.asm)

; The Host-Message-Stack (HMS) and DMA buffers are constrained (modulo storage)
NB_HMS	  equ I_NHMS		   ; Host Message Stack buffer length
NB_TMQ	  equ I_NTMQ		   ; Timed Message Queue buffer length
NB_DMQ	  equ I_NDMQ		   ; DSP Message Queue buffer length

	if READ_DATA
NB_DMA_R  equ I_NDMA*2		   ; Size of DMA read buffer ring (two buffers)
	else
NB_DMA_R    	equ 0			; We'll do this later on
		message 'read-data DMA buffer disabled'
	endif

NB_DMA_W  equ I_NDMA*2		   ; Size of DMA read buffer ring (two buffers)
NB_DMA	  equ NB_DMA_R+NB_DMA_W	   ; Total length of DMA buffer pool

NLI_SYS	  equ  I_NLI_SYS ; No. internal words reserved for system l constants
NXI_SYS	  equ I_NXI_SYS ; No. internal words reserved for system x constants
NYI_SYS	  equ I_NYI_SYS ; No. internal words reserved for system y constants
NPI_SYS	  equ I_NPI_SYS ; No. internal words reserved for system p programs

NPE_SCR	  equ I_NPE_SCR	   ; Maximum length of program in HMS or TMQ
NPE_SYS	  equ I_NPE_SYS	   ; System external p handlers and utilities
NPE_SYSEP equ  I_SYSEP	   ; System dispatch table size (Entry Points)
	
NLE_SYS	  equ I_NLE_SYS	   ; System external l constants
NXE_SYS	  equ I_NXE_SYS	   ; System external x vars (82 currently used:1/24/89)

NYE_SYS	  equ NB_HMS+NB_DMQ+NB_TMQ+NB_DMA ; System external y memory: io bufs
	  if NYE_SYS!=I_NYE_SYS
	    fail 'Computed NYE_SYS different from I_NYE_SYS in config.asm'
	  endif

; ---------------------------------------------------------------------------
; PHYSICAL INTERNAL MEMORY

;*** FIXME: Change N*_MEM to N*_RAM everywhere and delete synonym below ***
NE_MEM	  equ  MEM_SIZ	; Size of external memory (assumed same for x,y,p)
NE_RAM	  equ  NE_MEM	; Size of external memory (assumed same for x,y,p)
NI_MEM	  equ  512  	; Size of internal memory (assumed same for x,y,p)
NI_RAM	  equ  NI_MEM   ; Size of internal memory (assumed same for x,y,p)
NXI_RAM	  equ  256
NYI_RAM	  equ  256
NPI_RAM	  equ  512  ; Assume 56001
NXI_ROM	  equ  256
NYI_ROM	  equ  256
NPI_ROM	  equ  0    ; Assume 56001

; X data memory map
XLI_RAM	  equ  0	      ; Lo internal RAM address
XHI_RAM	  equ  NXI_RAM-1      ; Hi internal RAM address
XLI_ROM	  equ  XHI_RAM+1      ; Lo internal ROM address (Mu-law, A-law)
XLI_MLT	  equ  XLI_ROM	      ; Mu-law table base address
XLI_ALT	  equ  XLI_ROM+XLI_MLT/2 ; A-law  table base address
XHI_ROM	  equ  NI_MEM-1	      ; Hi internal ROM address

; Y data memory map
YLI_RAM	  equ  0	      ; Lo internal RAM address
YHI_RAM	  equ  NYI_RAM-1      ; Hi internal RAM address
YLI_ROM	  equ  YHI_RAM+1      ; Lo internal ROM address (full sinewave)
YLI_SIN	  equ  YLI_ROM	      ; Full sinewave table address
YHI_ROM	  equ  NI_MEM-1	      ; Hi internal ROM address

; Program memory map
RESET	  equ  0	      ; Make RESET a symbol for reset start address
PLI_RAM	  equ  0	      ; Lo internal RAM address	    
PHI_RAM	  equ  NPI_RAM-1      ; Hi internal RAM address
PLI_ROM	  equ  PHI_RAM+1      ; Lo internal ROM address
PHI_ROM	  equ  PLI_ROM+NPI_ROM-1   ; Hi internal ROM address

; ---------------------------------------------------------------------------
; PHYSICAL EXTERNAL MEMORY

     if MEM_OFF<NI_MEM
LE_RAM	  equ NI_MEM	      ; low external RAM address
N_SHADOW  equ NI_MEM-MEM_OFF  ; no. ext mem words shadowed by on-chip memory
     else
LE_RAM	  equ MEM_OFF	 ; low external RAM address (for ONEMEM==1 only!!!)
N_SHADOW  equ 0		 ; no. external memory words shadowed by on-chip memory
     endif

HE_RAM	  equ MEM_OFF+NE_MEM-1	   ; high external RAM address
     remember 'shadowed memory not added to top limit'
;    if N_SHADOW==NI_MEM (then HE_RAM=MEM_OFF+NE_MEM+N_SHADOW-1)

; The physical external memory boundaries overlap when
; ONE_MEM is true.  
XLE_RAM	  equ  LE_RAM	      ; Lo external RAM address
XHE_RAM	  equ  HE_RAM	      ; Hi external RAM address 
YLE_RAM	  equ  LE_RAM	      ; Lo external RAM address
YHE_RAM	  equ  HE_RAM	      ; Hi external RAM address 
PLE_RAM	  equ  LE_RAM	      ; Lo external RAM address
PHE_RAM	  equ  HE_RAM	      ; Hi external RAM address 

	  if HE_RAM>=$FFC0
		fail 'ext sys l mem allocated in io map'
	  endif

LE_SEG	equ SEG_OFF		; lower boundary of x/y segmented memory
HE_SEG	equ SEG_OFF+NE_RAM-1	; upper boundary of x/y segmented memory

NXE_SEG	  equ  NE_RAM>>1 		; Size of external SEG, x bank
NYE_SEG	  equ  NE_RAM>>1 		; Size of external SEG, y bank
NLE_SEG	  equ  NE_RAM>>1 		; Size of external SEG, l bank
NPE_SEG	  equ  NE_RAM	 		; Size of external SEG, p overlay

XLE_SEG	  equ  LE_SEG	      		; Lo external SEG address, x bank
XHE_SEG	  equ  LE_SEG+NXE_SEG-1		; Hi external SEG address, x bank
YLE_SEG	  equ  LE_SEG	      		; Lo external SEG address, y bank
YHE_SEG	  equ  LE_SEG+NYE_SEG-1	       	; Hi external SEG address, y bank
LLE_SEG	  equ  LE_SEG	      		; Lo external SEG address, l overlay
LHE_SEG	  equ  LE_SEG+NLE_SEG-1	       	; Hi external SEG address, l overlay
PLE_SEG	  equ  LE_SEG	      		; Lo external SEG address, p overlay
PHE_SEG	  equ  HE_SEG	   		; Hi external SEG address, p overlay

; ---------------------------------------------------------------------------
; INTERNAL SYSTEM MEMORY BOUNDARY POINTERS

LLI_SYS	  equ  0		   ; Lo internal sys l address
LHI_SYS	  equ  NLI_SYS-1	   ; Hi internal sys l address
XLI_SYS	  equ  0		   ; Lo internal sys x address
XHI_SYS	  equ  XLI_SYS+NXI_SYS+NLI_SYS-1   ; Hi internal sys x address
YLI_SYS	  equ  0		   ; Lo internal sys y address
YHI_SYS	  equ  YLI_SYS+NYI_SYS+NLI_SYS-1   ; Hi internal sys y address
PLI_SYS	  equ  DEGMON_H+1	   ; Lo internal sys p address
PHI_SYS	  equ  PLI_SYS+NPI_SYS-1   ; Hi internal sys p address

; ---------------------------------------------------------------------------
; INTERNAL USER MEMORY BOUNDARY POINTERS

; *** THESE ADDRESSES MUST STAY IN SYNCH WITH mkmon8k.mem ***

XLI_USR	  equ  XHI_SYS+1		; Lo internal usr x address
YLI_USR	  equ  YHI_SYS+1		; Lo internal usr y address
PLI_USR	  equ  PHI_SYS+1		; Lo internal user p address
START	  equ  PLI_USR			; Default user start address
PHI_USR	  equ  NPI_RAM-1		; Hi internal usr p address
NPI_USR	  equ  PHI_USR-PLI_USR+1	; No. internal p words for user
	  if !@DEF(NLI_USR)
NLI_USR	  equ  10			; No. internal l words for user (fixed)
	  endif
XHI_USR	  equ  XHI_RAM-NLI_USR		; Hi internal usr x address
YHI_USR	  equ  YHI_RAM-NLI_USR		; Hi internal usr y address
NXI_USR	  equ  XHI_USR-XLI_USR+1	; No. internal x words for user
NYI_USR	  equ  YHI_USR-YLI_USR+1	; No. internal y words for user
LLI_USR	  equ  @CVI(@MAX(XHI_USR,YHI_USR))+1 ; Lo internal usr l address
LHI_USR	  equ  NXI_RAM-1

	  if NYI_USR!=(NYI_RAM-NLI_USR-NLI_SYS)
	       fail 'int y mem err: NYI_USR!=(NYI_RAM-NLI_USR-NLI_SYS)'
	  endif

	  if NXI_USR!=(NXI_RAM-NLI_USR-NLI_SYS)
	       fail 'int X mem err: NXI_USR!=(NXI_RAM-NLI_USR-NLI_SYS)'
	  endif

	  if (LHI_USR-LLI_USR+1)!=NLI_USR
	       fail 'internal l mem confused: (LHI_USR-LLI_USR+1)!=NLI_USR'
	  endif

; ---------------------------------------------------------------------------
; EXTERNAL USER MEMORY BOUNDARY POINTERS

; *** THESE ADDRESSES MUST STAY IN SYNCH WITH mkmon8k.mem ***

; When SEP_MEM is true, we have fully partitioned external memory.
; This configuration is not likely in the
; near future because it only makes sense when there
; is more than 64K words of external memory available.

; ****************** SEP_MEM *******************
     if SEP_MEM
	include 'grub/sepmem.asm'
     endif

; In the case of ONE_MEM, all system external memory is shared.
; In the system section, first comes program, then x variables, then y.
; There is no external l memory in this case. All modulo io buffers
; are allocated in y memory (to make it easy to allocate them on 
; power-of-2 boundaries without wasting storage).
; See /usr/lib/dsp/smsrc/allocsys.asm for further details. 
;
; Ext mem
; -------
; usr p
; usr x
; usr y
; usr l (none)
; sys p
; sys x
; sys y
; sys l (none)
;
; ****************** ONE_MEM *******************

     if ONE_MEM

; ----- EXTERNAL USER MEMORY SIZES -----

; All external sys memory 
NAE_SYS	equ NXE_SYS+NYE_SYS+NPE_SYS+NPE_SYSEP+NLE_SYS+N_SHADOW 

NPE_USR	equ I_NPE_USR			; No. off-chip words for user code

	if XY_SPLIT
NXE_USR	equ (NE_MEM-NPE_USR-NAE_SYS)>>1	; No. off-chip words for user x data
NYE_USR	equ NXE_USR			; No. off-chip words for user y data
	else
NXE_USR	equ NE_MEM-NPE_USR-NAE_SYS	; No. off-chip words for user x data
NYE_USR	equ 0				; No. off-chip words for user y data
	endif
NLE_USR	equ 0				; No. off-chip words for user l data

; ----- SHARED EXTERNAL USER MEMORY -----
LE_USR	  equ  LE_RAM			; lower bound of all external user mem
PLE_USR	  equ  LE_USR			; convention is to allocate pe 1st
PHE_USR	  equ  PLE_USR+NPE_USR-1
XLE_USR	  equ  PHE_USR+1		; xe
XHE_USR	  equ  XLE_USR+NXE_USR-1
YLE_USR	  equ  XHE_USR+1		; ye
YHE_USR	  equ  YLE_USR+NYE_USR-1
LLE_USR	  equ  YHE_USR+1		; le (can't have)
LHE_USR	  equ  LLE_USR+NLE_USR-1
HE_USR	  equ  LHE_USR			; upper bound of all external user mem
NE_USR	  equ  HE_USR-LE_USR+1		; total count of all external user mem
	  must_have NLE_USR==0

; ----- EXTERNAL USER PARTITIONED X/Y SEGMENT MEMORY -----

	if NE_USR<NXE_SEG	; only enough for Y partition, none left for x
NYE_USG 	equ NE_USR	; size of y bank (appears first) 
NXE_USG 	equ 0		; size of x bank
	else
NYE_USG 	equ NYE_SEG	; size of y bank = physical size
NXE_USG 	equ NE_USR-NYE_SEG ; size of x bank = physical minus system
	endif
NLE_USG equ NXE_USG		; min(x.y)
NPE_USG equ NPE_USR		; unchanged

LE_USG	equ  LE_SEG		; low user segment begin

PLE_USG	equ  LE_USG		; p looks the same in this partition
PHE_USG	equ  PLE_USG+NPE_USG-1

XLE_USG	equ  LE_USG		; user xe segment
XHE_USG	equ  XLE_USG+NXE_USG-1

YLE_USG	equ  LE_USG		; user ye segment
YHE_USG equ  YLE_USG+NYE_USG-1

LLE_USG	  equ  LE_USG		; user le segment
LHE_USG	  equ  LLE_USG+NLE_USG-1

; ----- SHARED EXTERNAL SYSTEM MEMORY -----
HE_SYS	  equ  HE_RAM			  ; upper bound of all external sys mem
LE_SYS	  equ  HE_SYS-NAE_SYS+N_SHADOW+1  ; lower bound of all external sys mem
NE_SYS	  equ  HE_SYS-LE_SYS+1		  ; total count of all external sys mem
PLE_SYS	  equ  LE_SYS			  ; Lo external system p address
PHE_SYS	  equ  PLE_SYS+NPE_SYS-1	  ; Hi external system p address
XLE_SYS	  equ  PHE_SYS+1		  ; Lo external system x address
XHE_SYS	  equ  XLE_SYS+NXE_SYS-1	  ; Hi external system x address
YLE_SYS	  equ  XHE_SYS+1		  ; Lo external system y address
YHE_SYS	  equ  YLE_SYS+NYE_SYS-1	  ; Hi external system y address
LLE_SYS	  equ  YHE_SYS+1		  ; Lo external system l address
LHE_SYS	  equ  LLE_SYS+NLE_SYS-1	  ; Hi external system l address
PLE_SYSEP equ  HE_SYS-NPE_SYSEP+1	  ; System entry point table
PHE_SYSEP equ  HE_SYS			  ; System entry point table last word

	  must_have NLE_SYS==0		  ; because external memory is shared
	  must_have YHE_SYS==HE_RAM-NPE_SYSEP
	  must_have LHE_USR==(PLE_SYS-1)||(XY_SPLIT&&LHE_USR==(PLE_SYS-2))
	  ; redundant check. Latter case happens when NAE_SYS is odd.

     endif

; ----- USER MEMORY OFFSETS -----

	  if @REL()
OPI_USR	  set PLI_USR	      ; offset for internal user p code, relative asm
OXI_USR	  set XLI_USR	      ; offset for internal user x data
OYI_USR	  set YLI_USR	      ; offset for internal user y data
OLI_USR	  set LLI_USR	      ; offset for internal user l data
OPE_USR	  set PLE_USR	      ; offset for external user p code
OXE_USR	  set XLE_USR	      ; offset for external user x data
OYE_USR	  set YLE_USR	      ; offset for external user y data
OLE_USR	  set LLE_USR	      ; offset for external user l data
	  else
OPI_USR	  set 0		      ; for absolute assembly, offset is built in
OXI_USR	  set 0		      
OYI_USR	  set 0		      
OLI_USR	  set 0		      
OPE_USR	  set 0		      
OXE_USR	  set 0		      
OYE_USR	  set 0		      
OLE_USR	  set 0		      
	  endif

; --- LOWER CASE VERSIONS ---

     define pli_usr 'PLI_USR'
     define phi_usr 'PHI_USR'
     define npi_usr 'NPI_USR'
     define xli_usr 'XLI_USR'
     define xhi_usr 'XHI_USR'
     define nxi_usr 'NXI_USR'
     define yli_usr 'YLI_USR'
     define yhi_usr 'YHI_USR'
     define nyi_usr 'NYI_USR'
     define lli_usr 'LLI_USR'
     define lhi_usr 'LHI_USR'
     define nli_usr 'NLI_USR'
     define ple_usr 'PLE_USR'
     define npe_usr 'NPE_USR'
     define phe_usr 'PHE_USR'
     define xle_usr 'XLE_USR'
     define xhe_usr 'XHE_USR'
     define nxe_usr 'NXE_USR'
     define yle_usr 'YLE_USR'
     define yhe_usr 'YHE_USR'
     define nye_usr 'NYE_USR'
     define lle_usr 'LLE_USR'
     define lhe_usr 'LHE_USR'
     define nle_usr 'NLE_USR'
     define opi_usr 'OPI_USR'
     define oxi_usr 'OXI_USR'
     define oyi_usr 'OYI_USR'
     define oli_usr 'OLI_USR'
     define ope_usr 'OPE_USR'
     define oxe_usr 'OXE_USR'
     define oye_usr 'OYE_USR'
     define ole_usr 'OLE_USR'

; ----- CONSISTENCY CHECKS -----

;    8K external RAM occupies addresses 8K to 16K-1
;    because address line A13 of 8K RAMs is 'chip enable'

     if MEM_SIZ==8192&&MEM_OFF==0&&!SIMULATOR
	  warn 'Expected 8K RAM to require 8K address offset'
     endif
 
     DUPA MEM,XI,XE,YI,YE,PI,PE,LI,LE
        IF (N\MEM\_SYS<0)||(N\MEM\_USR<0)     ; CAN HAPPEN FOR COMPUTED CASES
	FAIL 'NEGATIVE MEMORY PARTITION SIZE'
	ENDIF
     ENDM




