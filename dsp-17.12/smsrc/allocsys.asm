; allocsys.asm - system memory allocation and initialization
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; Included by /usr/lib/dsp/smsrc/music_macros.asm
;;
;; System memory should be allocated and initialized before the user
;; starts allocating. This file should be included before allocusr.asm
;; or not at all if no system code or symbols desired. 
;;
; SYSTEM DATA MEMORY INITIALIZATION (allocation done in /usr/lib/dsp/smsrc/memmap.asm)

     org p_i:0		; point to beginning of internal p memory
     org x_i:XLI_SYS	; point to beginning of internal system x data
     org y_i:YLI_SYS	; point to beginning of internal system y data
     org l_i:LLI_SYS	; point to beginning of internal system l data
     org p_e:PLE_SYS	; point to beginning of external system p code
     org x_e:XLE_SYS	; point to beginning of external system x data
     org y_e:YLE_SYS	; point to beginning of external system y data
     org l_e:LLE_SYS 	; point to beginning of external system l data

; Internal system storage
; -----------------------

	  org l_i:	      ; point to beginning of internal system l data
	  include 'sys_li'    ; Declare and init l internal system vars
	  if (*-LLI_SYS)!=NLI_SYS
		if DBL_BUG&&((*-LLI_SYS)!=2*NLI_SYS)
		   fail 'sys_.asm: memmap.asm/NLI_SYS disagrees with sys_li.asm'
		endif
	  endif

	  org x_i:	      ; Point to beginning of internal system x data
	  if NXI_SYS>0
	     include 'sys_xi'    ; Declare and init x internal system vars
	     symobj NXI_LOST
NXI_LOST     set *-1-XHI_SYS     ; No. of internal words overflow
nxi_free_sys equ -NXI_LOST    ; how much wasted space in partition
	     if NXI_LOST>0
	       fail 'sys_.asm: xi overflow. Increase memmap.asm/NXI_SYS'
	       msg 'Find NXI_LOST above or in symbol table'
	     endif
	  endif

	  org y_i:	      ; point to beginning of internal system y data
	  if NYI_SYS>0
 	     include 'sys_yi'  ; Declare and init y internal system vars
	     symobj NYI_LOST
NYI_LOST     set *-1-YHI_SYS     ; No. of internal words overflow
nyi_free_sys equ -NYI_LOST    ; how much wasted space in partition
	     if NYI_LOST>0
	       fail 'sys_.asm: yi overflow. Increase memmap.asm/NYI_SYS'
	       msg 'Find NYI_LOST in symbol table or above'
	     endif
	  endif

; *** CONSTRAINT: The following segments must form a single contiguous
;	on-chip p segment.
;
	if ASM_DEGMON
	  org p_i:0			; switch to internal program memory
	  include 'vectors.asm'		; load interrupt vectors

	  if *!=DEGMON_L
	     fail 'sys_.asm: Inferred PHE_VECTORS is not equal to DEGMON_L-1'
	  endif

	  org p_i:DEGMON_L		; start of Ariel debugger monitor
	  if DEGMON
	    include 'degmon'
;
;*** Check up on addresses assumed in memmap.asm (needed when degmon is
;    not assembled, such as when assembly product is destined to be loaded
;    into a running BUG56).
;
	    must_have DEGMON_BEG==DEGMON_L
	    must_have DEGMON_END==DEGMON_H+1
	    must_have DEGMON_RUN==DEGMON_RUN_LOC
	    must_have DEGMON_TRACER==DEGMON_TRACER_LOC
	  else
	    dup DEGMON_N		; size of degmon
		nop	; degmon goes here
	    endm
	  endif
	else		; degmon not included:
	  include 'iv_decl'	; declare interrupt vector offsets
		; install vectors that DEGMON will not mind loading:
		org p_i:iv_irq_a
;iv_irq_a
	       	jsr >abort1 ; external abort
;iv_irq_b       	
		jsr >abort  ; internal abort
;iv_ssi_rcv     	
		jsr >ssi_rcv
;iv_ssi_rcv_exc 	
		jsr >ssi_rcv_exc
;iv_ssi_xmt     	
		jsr >ssi_xmt
;iv_ssi_xmt_exc 	
		jsr >ssi_xmt_exc
;iv_sci_rcv 
	    	DEBUG_HALT
		nop
;iv_sci_rcv_exc 
		DEBUG_HALT
		nop
;iv_sci_xmt
	     	DEBUG_HALT
		nop
;iv_sci_idle
	    	DEBUG_HALT
		nop
;iv_sci_timer   	
		jsr >sci_timer

; iv_nmi	JSR >DEGMON_TRACER ;TRACE intrpt handler (NMI) - used by BUG56

		org p_i:iv_host_rcv
;iv_host_rcv 
	   	movep x:$FFEB,y:(R_HMS)- ; write (circular) Host Message Queue
;iv_host_rcv2   	
		nop
;iv_host_xmt    	
		jsr >host_xmt
;iv_host_cmd    	
		jsr >hc_host_r_done  	; Terminate DMA read from host

		  org p_i:PLI_SYS
	endif

	  if *!=PLI_SYS
	     warn 'sys_.asm: Inferred DEGMON_H is not equal to PLI_SYS-1'
	  endif
	  if NPI_SYS>0
	    include 'sys_pi'		; Declare and init p internal code
	  endif

;	  Load temporary internal sys prog memory (boot loader, mem tests)
;	  Note that a user module will overwrite bootstrap and memory test code

	  if *!=PLI_USR
	     warn 'sys_.asm: Inferred PHI_SYS is not equal to PLI_USR-1'
	     org p_i:PLI_USR		; beginning of onchip user memory
	  endif

	  if ASM_RESET
   	    include 'reset_boot'	; Boot code for off-chip load
	  endif

	  symobj NPI_BOOT_LOST
NPI_BOOT_LOST  set *-1-PHI_RAM		; No. of internal words overflow
	  if NPI_BOOT_LOST>0		; We spilled off chip
	     fail 'sys_.asm: pi boot overflow. Tighten reset_boot.asm or shift'
	     msg 'Find NPI_BOOT_LOST in symbol table or above'
	  endif

;; ============================================================================
; External system storage
; -----------------------
;; 
;; Include DSP system code, loading it into external p memory.  Next,
;; install external system constants followed by modulo buffers in order
;; of increasing size.	In general, the address of each modulo buffer
;; must be a multiple of the smallest power of 2 equal to or larger than
;; the buffer length.  Here we assume all modulo buffer lengths are a
;; power of 2 by convention. This makes it easy to allocate them as
;; follows:
;; 
;; The first non-existent memory location is a power of 2, and, in
;; general, subtracting a power of 2 from a larger power of 2 leaves a
;; multiple of the smaller power of 2. Therefore, the buffer
;; start-address constraint is satisfied automatically when all buffer
;; lengths are a power of 2 and they are allocated backward from the top
;; of memory in order of decreasing size.
;; 
;; When ONE_MEM is true (shared external memory), external memory contains
;; external system code, "x" external variables, "y" external variables,
;; and the modulo storage buffers, in that order.
;
; we allocate y data before x because it contains IO buffer addresses which
; are used to initialize certain x system variables (DMA initial pointers)

	  org y_e:	      ; point to beginning of ext y data
	  include 'sys_ye'    ; Declare and init ye runtime environment vars
	  symobj NYE_LOST
NYE_LOST  set *-1-YHE_SYS ; No. of ext words overflow by system
nye_free_sys equ -NYE_LOST    ; how much wasted space in partition
	  if NYE_LOST!=0
	     fail 'sys_.asm: y use must exactly = memmap.asm/NYE_SYS'
	     msg 'Find NYE_LOST in symbol table or above'
	  endif

	  org x_e:	      ; point to beginning of ext x data
	  include 'sys_xe'    ; Declare and init xe runtime environment vars
	  symobj NXE_LOST
NXE_LOST  set *-1-XHE_SYS     ; No. of ext words overflow by system
nxe_free_sys equ -NXE_LOST    ; how much wasted space in partition
	  if NXE_LOST>0
	     fail 'sys_.asm: xe overflow. Increase memmap.asm/NXE_SYS'
	     msg 'Find NXE_LOST in symbol table or above'
	  endif

	  org p_e:PLE_SYSEP  	; host message dispatch table
	  include 'hmdispatch'

	  org p_e:PLE_SYS     	; point to beginning of external system p code

	  include 'jsrlib'    	; Declare and init system utility subroutines
	  include 'handlers'  	; Long ntrpt, host cmd, & host msg handlers 
	  include 'hmlib'     	; Declare and init host message handlers

	  symobj NPE_LOST
NPE_LOST  set *-1-PHE_SYS      	; No. of external words overflow by system
npe_free_sys equ -NPE_LOST     	; how much wasted space in partition
	  if NPE_LOST>0
	     fail 'sys_.asm: pe overflow. Increase memmap.asm/NPE_SYS'
	     msg 'Find NPE_LOST in symbol table or above'

	  endif

	  symobj NPE_SYSEP_LOST
NPE_SYSEP_LOST  set *-1-HE_SYS   ; No. of dispatch words overflow by system
npe_sysep_free equ -NPE_SYSEP_LOST  ; how much wasted space in partition
	  if NPE_SYSEP_LOST>0
	     	fail 'sys_.asm: Host Message dispatch table overflow.'
		msg '	Increase config.asm/I_SYSEP by an even number'
		msg '	Make sure former entry points are the same!!'
		msg '	(Add to the beginning of the dispatch table)'
	     	msg '	Find NPE_SYSEP_LOST in symbol table or above'
	  endif
