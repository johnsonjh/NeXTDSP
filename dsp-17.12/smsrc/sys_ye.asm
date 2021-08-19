; y system environment variables
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; Included by /usr/lib/dsp/smsrc/allocsys.asm
;; The space allocation is specified in /usr/lib/dsp/smsrc/memmap.asm (NYE_SYS)
;;
;; *** NOTE *** The number of y system variables (excluding io buffers) 
;; provided for by the NYE_SYS variable in /usr/lib/dsp/smsrc/memmap.asm must 
;; be used here exactly.  Also, the io buffers should be AFTER the y variables.
;; Otherwise the io buffers will not end up at the special addresses
;; needed for modulo storage indexing. For this reason, it is more convenient
;; (and safer) to place all external-memory system variables in x memory
;; so that only io buffers appear here in the external system y-memory pool.
;; Finally, the y io buffers need to be powers of 2 long and allocated in
;; increasing order.
;;
;; *** NOTE *** The default allocation provides only one dma buffer.
;; If two are needed (for simultaneous read-data and write-data for example),
;; you should split the already allocated buffer into 2 pieces of equal size.
;;
;; *** In all cases, however, the length of each dma buffer should equal
;; a multiple of the tick size I_NTICK. The tick is the smallest unit of
;; transfer into or out of a dma buffer by a unit generator.
;;
;; The buffer sizes below are defined in memmap.asm (and config.asm)

        remember 'Do we need to XDEF all the system Y buffer addresses?'

	xdef YB_HMS,YB_DMQ,YB_TMQ
	xdef YB_DMA_W,YB_DMA_W2
	xdef YB_DMA_R,YB_DMA_R2

	symobj YBTOP_HMS,YBTOP_DMQ,YBTOP_TMQ
	symobj YBTOP_DMA_R,YBTOP_DMA_W


sys_ye_ds macro count
	if count>0
		ds count
	endif
	endm

sys_ye_dsm macro count
	if count>0
		dsm count
	endif
	endm

sys_ye_bsc macro count,init
	sys_ye_ds count		; FIXME: Intended "bsc count,init" here
	endm

alloc_hms macro
ye_sys_lc set *		      ; save location counter
YB_HMS0	  sys_ye_bsc NB_HMS,0 ; allocate and clear host message stack
	  org y_e:ye_sys_lc   ; now allocate as modulo storage
YB_HMS	  dsm NB_HMS	      ; allocate host message stack (hms) buffer.
	  if YB_HMS!=YB_HMS0
	       fail 'sys_ye.asm: hms allocation misplaced'
	  endif
YBTOP_HMS set YB_HMS+NB_HMS-1 ; pointer to top (initial) element of hms
	endm

alloc_dmq macro
ye_sys_lc set *	
YB_DMQ0	  sys_ye_bsc NB_DMQ,0 ; clear dsp message queue
	  org y_e:ye_sys_lc
YB_DMQ	  dsm NB_DMQ	      ; Allocate DSP Message Queue (DMQ) storage.
	  if YB_DMQ!=YB_DMQ0  ; Actually, there is no need to make this modulo
	       fail 'sys_ye.asm: DMQ allocation misplaced'
	  endif
YBTOP_DMQ set YB_DMQ+NB_DMQ-1 ; pointer to top (initial) element of dmq
	endm

alloc_dma macro			    ; invocation below (order varies)
; ---
	if READ_DATA
ye_sys_lc set *			    ; save LC to check modulo storage alloc
YB_DMA_R0 sys_ye_bsc NB_DMA_R,0     ; DMA read buffer (NB_DMA* in memmap.asm)
	  org y_e:ye_sys_lc	    ; go back to saved LC
YB_DMA_R  dsm NB_DMA_R/2      	    ; First DMA read buffer
YB_DMA_R2 dsm NB_DMA_R/2      	    ; Address of 2nd DMA read buffer
	else
YB_DMA_R0 equ $2000-1		    ; nonexistent memory (trigger breakpoint)
YB_DMA_R  equ $2000-1		    ; nonexistent memory (trigger breakpoint)
YB_DMA_R2 equ $2000-1		    ; nonexistent memory (trigger breakpoint)
	endif
	if YB_DMA_R!=YB_DMA_R0    ; Make sure modulo storage worked out
	     fail 'sys_ye.asm: DMA read buffer allocation misplaced'
	endif
YBTOP_DMA_R set YB_DMA_R+NB_DMA_R-1 ; pointer to top element of rd bufs
; ---
ye_sys_lc set *	
YB_DMA_W0 sys_ye_bsc NB_DMA_W,0   ; Clear DMA buffers
	  org y_e:ye_sys_lc 
YB_DMA_W  sys_ye_dsm NB_DMA_W/2
YB_DMA_W2 sys_ye_dsm NB_DMA_W/2
	  if YB_DMA_W!=YB_DMA_W0
	       fail 'sys_ye.asm: DMA write buffer allocation misplaced'
	  endif
YBTOP_DMA_W set YB_DMA_W+NB_DMA_W-1 ; pointer to top element of wd bufs
; ---
	  endm

alloc_tmq macro
ye_sys_lc set *	
YB_TMQ0	  sys_ye_bsc NB_TMQ,0 ; Clear Timed Message Queue
	  org y_e:ye_sys_lc
YB_TMQ	  dsm NB_TMQ/2 	      ; Allocate Timed Message Queue (TMQ) storage.
YB_TMQ2	  ds  NB_TMQ/2	      ; YB_TMQ2 used to indicate half-empty to host
	  if YB_TMQ!=YB_TMQ0
	       fail 'sys_ye.asm: TMQ allocation misplaced'
	  endif
YBTOP_TMQ set YB_TMQ+NB_TMQ-1 ; pointer to top (initial) element of tmq
	  endm

;; Allocate HMS, DMQ, TMQ, and DMA buffers in order of increasing size

	if NB_HMS>NB_DMQ
		alloc_dmq
		alloc_hms
	else
		alloc_hms
		alloc_dmq
	endif

	if @MAX(NB_HMS,NB_DMQ)>NB_DMA&&(NB_DMA!=0)
	  fail 'sys_ye.asm: NB_DMQ or NB_HMS > NB_DMA'
	endif

	if NB_DMA<=NB_TMQ+NPE_SYSEP
	  alloc_dma ; Since TMQ last, sum of DMA buffers must not exceed TMQ+EP
	  alloc_tmq ; TMQ *MUST* be last because it splits with SYSEP dispatch
	else
	  fail 'NB_DMA cannot exceed NB_TMQ+NPE_SYSEP'
	endif

