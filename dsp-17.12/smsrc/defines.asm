; /usr/lib/dsp/smsrc/defines.asm - Symbolic definitions for various assembly constants
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; Modification history
;; 03/22/90/jos - Changed DEBUG_HALT to 'SWI' if !HAVE_SYSTEM
;; 03/22/90/jos - ioequ.asm included for M_CRB etc (SSI handler).
;;		  Could also conditionally not assemble handler if 
;;		  !HAVE_SYSTEM
;;
DBL_BUG 	equ (0) ; set 0 for asm56000 versions>=2.03, 1 for before
TRUE 		equ -1
FALSE 		equ 0

	define OPTIONAL_NOP 'nop' ; used by AP kit for DO loop insurance

; Integer constants
I_MAXPOS  EQU  $7FFFFF	      ; Maximum positive 24-bit fixed-point number (1-)
I_MINPOS  EQU  $000001	      ; Minimum positive 24-bit fixed-point number
I_EPS	  EQU  $000001	      ; Minimum positive 24-bit fixed-point number
I_ONEHALF EQU  $400000	      ; 0.5
I_0DBU24  EQU  $31999	      ; 24-bit amp level equal to 0 dBu (.775Vrms)
I_0DBU16  EQU  $3187	      ; 16-bit 0dBu level (d/a out = 2Vrms at max amp)
I_M12DBU24 EQU $C6666	      ; 24-bit amplitude level equal to -12 dBu
I_M12DBU16 EQU $C66	      ; 16-bit amplitude level equal to -12 dBu

; Floating-point constants
F_PI	  EQU  4*@ATN(1)      ; 3.1415...
F_ONE	  EQU  0.99999988     ; Largest positive float=(2^23-1/2^23)
F_EPS	  EQU  0.00000012     ; Smallest positive float=2^-23=1.1920928955e-07
F_0DBU24  EQU  0.3875	      ; 24-bit amp level equal to 0 dBu (.775Vrms)
F_0DBU16  EQU  0.0015115      ; right-justified 16-bit 0dBu level
F_M12DBU24 EQU 0.0968750      ; 24-bit amplitude level equal to -12 dBu
F_M12DBU16 EQU 0.0003784      ; right-justified 16-bit -12dBu level

; String constants
S_NUL	  EQU  ''	      ; the null string (0)

; *** RUN STATUS ***

;; Y_RUNSTAT	    dc	       0   ; RUN status (declared in sys_li.asm)

     remember 'HMSF and TMQF flags not yet being maintained'

B__HMS_FULL		equ 0	; $000001 Host  Message Stack Full Flag
B__TMQ_LWM_ME		equ 1	; $000002 Enable DM_TMQ_LWM message
B__DMQ_FULL		equ 2	; $000004 Probably blocked at dm_buzz
B__DMQ_LOSE		equ 3	; $000008 Set to allow lost DSP messages

;; B__DMQ_ACTIVE 	equ *	; Test bit 1 (HTIE) of HCR for this
B__TMQ_ACTIVE 		equ 4	; $000010 Set when TMQ has a message in it
B__REALTIME		equ 5	; $000020 Set when trying for real time
B__ALU_SAVED 		equ 6	; $000040 Set when ALU saved for interrupt
B__TMQ_LWM	 	equ 7	; $000080 TMQ is under low-water mark

B__TEMPS_SAVED 		equ 8	; $000100 Set when temps saved for interrupt
;;  B__???????????	equ 9	; $000200 Formerly B__TEMPS_SAVED_ERR
B__SSI_WDU		equ 10	; $000400 SSI WD Underrun (ssi_xmt_exc)
B__SSI_RDO		equ 11	; $000800 SSI RD Overrun (ssi_rcv_exc)
B__BLOCK_TMQ_LWM	equ 12	; $001000 Block on TMQ empty (Formerly ADM)
B__SIM			equ 13	; $002000 Set when in running in SIMULATOR
B__HM_DONE_INT		equ 14	; $004000 Enable interrupt after host msg
B__TRACE_MODE		equ 15	; $008000 Enable trace mode (see swi_)
B__HALF_SRATE		equ 16	; $010000 Set for half sampling rate
			   ; This is only used by ssi xmt handler
B__TMQ_FULL		equ 17	; $020000 Timed Message Queue Full Flag
B__BLOCK_WD_START	equ 18	; $040000 Set when blocking at wdbo_buzz
B__BLOCK_WD_FINISH	equ 19	; $080000 Set when blocking at wd_block_host
B__BLOCK_WD_PENDING 	equ 20	; $100000 if blocked till DMA WD PENDING
B__BLOCK_WD_HTIE	equ 21	; $200000 if blocked till HTIE clear
B__BLOCK_WD_HF1		equ 22	; $400000 if blocked till HF1 set
B__ABORTING		equ 23	; $800000 if aborting (inhibits DMA req's)

; *** DMA STATE ***

;; X_DMASTAT needs to be a short (6bit) absolute address so that the JSSET
;;	     instruction will work

;; X_DMASTAT   dc 0	        ; Initial DMA state (defined in sys_li.asm)
B__HOST_READ	 	equ 0	; $000001 dma host read in progress
B__READ_DATA	 	equ 1	; $000002 dma read-data in progress
B__READ_RING	 	equ 2	; $000004 dma read-data ring state
B__HOST_WRITE	 	equ 3	; $000008 dma host write in progress

B__WRITE_DATA	 	equ 4	; $000010 dma write-data in progress
B__WRITE_RING	 	equ 5	; $000020 dma write-data ring state
B__HOST_RD_ENABLE 	equ 6	; $000040
B__SSI_RD_ENABLE 	equ 7	; $000080

B__HOST_WD_ENABLE 	equ 8	; $000100 write data goes to host
B__SSI_WD_ENABLE 	equ 9	; $000200 write data to DSP ssi port
B__HOST_WD_PENDING 	equ 10 	; $000400 DMA WD request pending
B__HOST_RD_PENDING 	equ 11 	; $000800 DMA RD request pending

B__SSI_WD_PENDING 	equ 12  ; $001000 SSI WD request pending
B__SSI_RD_PENDING 	equ 13  ; $002000 SSI RD request pending
B__SSI_RD_RUNNING 	equ 14  ; $004000 1 after write_data_buf._out 
B__SSI_WD_RUNNING 	equ 15  ; $008000 1 after write_data_buf._out 

	if MSG_REMINDERS
	    msg 'RD and WD BLOCKED status bits not yet being maintained'
	endif
B__WD_BLOCKED		equ   16  ; $010000 WD blocked for host|ssi
B__RD_BLOCKED		equ   17  ; $020000 RD blocked for host|ssi
B__DM_OFF		equ   18  ; $040000 DSP messages are turned off

	remember 'need to add such bits for each WD and RD channel'
B__HOST_WD_OFF_PENDING 	equ 	19	; $080000 Enabled to turn off 
				     	; write-data after the
				     	; current buffer gets out.
B__HOST_RD_OFF_PENDING 	equ 	20	; $100000 Same for read-data
B__TMQ_ATOMIC		equ	21	; $200000 Atomic Timed Messages
B__TZM_PENDING	 	equ	22	; $400000 for Timed-Zero msg
B__EXTERNAL_ABORT	equ 	23	; $800000 if abort is via host message
					; this is needed until the driver stops
					; resetting the DSP on HF2 & HF3

; MNEMONICS FOR LOCATION COUNTERS
;;
;;  Loc ctr    Use in x, y, l, and p mem     Use in l if DBL_BUG
;;  -------    ---------------------------   --------------------
;;  Low	       Not used			     Not used
;;  Default    On-chip segment		     On-chip l segment, high-order word
;;  High       Off-chip segment		     On-chip l segment, low-order word
;;
	  define p_i 'p'	   ; p internal memory lc
	  define p_e 'ph'	   ; p external memory lc
	  define p_u 'pl'	   ; p memory lc free for user
	  define x_i 'x'	   ; x internal
	  define x_e 'xh'	   ; x external
	  define x_u 'xl'	   ; x memory lc free for user
	  define y_i 'y'	   ; y internal
	  define y_e 'yh'	   ; y external
	  define y_u 'yl'	   ; y memory lc free for user
	  define l_i  'l'	   ; l internal
	  define l_e 'lh'	   ; l external
	  define l_u  'll'	   ; l memory lc free for user
	  if DBL_BUG
	     define l_ih 'xl'	   ; l internal, high-order word
	     define l_il 'yl'	   ; l internal, low-order word
	     define l_eh 'sorry'   ; l external, high-order word
	     define l_el 'sorry'   ; l external, low-order word
	  else
	  endif

; MNEMONICS FOR MEMORY SPACE NUMERIC CODES
	define SPACE_X '1'
	define SPACE_Y '2'
	define SPACE_L '3'
	define SPACE_P '4'

; MNEMONICS FOR ADDRESS ALU REGISTERS
	  define R_X   'r0'   ; x arg block pointer
	  define R_Y   'r4'   ; y arg block pointer
	  define R_I1  'r1'   ; 1st input signal pointer
	  define R_I2  'r5'   ; 2nd input signal pointer
	  define R_L   'r2'   ; l arg block pointer
;*** FIXME: delete from here
	  define P_L   '+'    ; l arg post-increment/decrement for arg use
	  define B_L   '-'    ; incr/decr for going BACKWARDS through l args
;*** FIXME: delete to here
	  define R_O   'r6'   ; output signal pointer
	  define R_HMS 'r3'   ; host message stack write pointer
;*** R_IO was called R_DMA before ioequ.asm started being used:
	  define R_IO  'r7'   ; DMA read-or-write pointer

	  define N_X   'n0'   ; x arg block pointer
	  define N_Y   'n4'   ; y arg block pointer
	  define N_I1  'n1'   ; 1st input signal pointer
	  define N_I2  'n5'   ; 2nd input signal pointer
	  define N_L   'n2'   ; l arg block pointer
	  define N_O   'n6'   ; output signal pointer
	  define N_HMS 'n3'   ; host message stack write pointer
;*** N_IO was called N_DMA before ioequ.asm started being used:
	  define N_IO  'n7'   ; DMA read-or-write pointer

	  define M_X   'm0'   ; x arg block pointer
	  define M_Y   'm4'   ; y arg block pointer
	  define M_I1  'm1'   ; 1st input signal pointer
	  define M_I2  'm5'   ; 2nd input signal pointer
	  define M_L   'm2'   ; l arg block pointer
	  define M_O   'm6'   ; output signal pointer
	  define M_HMS 'm3'   ; host message stack write pointer
	  define M_IO  'm7'   ; DMA read-or-write pointer
;*** M_DMA is defined in Motorola's ioequ.asm as the DMA bit of the HSR

; MISC. DEFINITIONS 
	  define TMQ_HEAD   '$111111'	; marks word at TMQ write pointer
	  define SYS_MAGIC  '$222222'	; marks first word of ext sys memory
	  define TMQ_TAIL   '$333333'	; marks word behind TMQ read pointer

; changed 7/4/89 so that when service_tmq overwrites message separator
; TMQ_MEND with tail marker TMQ_TAIL, there will be no difference. The
; purpose of this is to allow rolling back TMQRP in the debugger without
; having to fix up the TMQ.
	  define TMQ_MEND   '$333333'	; indicates end of timed message
;*	  define TMQ_MEND   '$555555'	; indicates end of timed message

	  define TMQ_NOLINK '0'		; TMQ_MEND better be correct!
	  define HMS_TOPMK  '$666666'	; marks top (origin) of HM stack
	  define HMS_BOTMK  '$777777'	; marks bottom (limit) of HM stack
	  define HM_UNTIMED '$880000'	; untimed host message (stack marker)
	  define HM_TIMEDA  '$990000'	; absolute timed host message
	  define HM_TIMEDR  '$AA0000'	; relative timed host message

; Maybe some day
; I_NDEC  set 2		      ; Samples per envelope update (decimation factor)

	define SWI_OPCODE '6' ; SWI = break
	define UNUSED_MARKER '6' ; SWI = break

define_debug_halt macro
 	    if HAVE_SYSTEM
	      define DEBUG_HALT 'JSR <iv_irq_b' ; SINGLE WORD (--->abort)
 	    else
	      define DEBUG_HALT 'SWI'
	    endif
	endm

	if (!@def(DEBUG_HALT_OVERRIDDEN))
	    define_debug_halt
	else
	  if (!DEBUG_HALT_OVERRIDDEN)
	    define_debug_halt
	  endif
	endif
	define I_RING_PARITY '$8000'	; location of bit in hm_host_w_req arg

	include 'ioequ.asm'

; Missing entry points from hmlib_mk.asm

	if AP_MON

	define set_tinc0 'unwritten_subr'
	define tmq_room0 'unwritten_subr'
	define get_time0 'unwritten_subr'
	define set_time0 'unwritten_subr'
	define clear_dma_hm0 'unwritten_subr'
	define host_rd_on0 'unwritten_subr'
	define host_rd_off0 'unwritten_subr'
	define host_wd_on0 'unwritten_subr'
	define host_wd_off0 'unwritten_subr'
	define dma_rd_ssi_on0 'unwritten_subr'
	define dma_rd_ssi_off0 'unwritten_subr'
	define dma_wd_ssi_on0 'unwritten_subr'
	define dma_wd_ssi_off0 'unwritten_subr'
	define poke_x0 'unwritten_subr'
	define poke_y0 'unwritten_subr'
	define poke_p0 'unwritten_subr'
	define poke_n0 'unwritten_subr'
	define fill_x0 'unwritten_subr'
	define fill_y0 'unwritten_subr'
	define fill_p0 'unwritten_subr'
	define blt_x0 'unwritten_subr'
	define blt_y0 'unwritten_subr'
	define blt_p0 'unwritten_subr'
	define sine_test0 'unwritten_subr'
	define host_w_dt0 'unwritten_subr'
	define host_w_swfix0 'unwritten_subr'
	define midi_msg0 'unwritten_subr'
	define host_rd_done0 'unwritten_subr'

	define ssi_rcv 'unwritten_subr'
	define ssi_rcv_exc 'unwritten_subr'
	define ssi_xmt 'unwritten_subr'
	define ssi_xmt_exc 'unwritten_subr'
	define sci_timer 'unwritten_subr'

	define open_paren0 'unwritten_subr'
	define close_paren0 'unwritten_subr'

	define block_tmq_lwm0 'unwritten_subr'
	define unblock_tmq_lwm0 'unwritten_subr'

	endif ; AP_MON


