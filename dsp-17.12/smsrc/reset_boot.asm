;; ================================================================
; reset_boot - temporary reset handler for downloading dsp system at boot time.
;		This file is 'included' at location p:PLI_USR by allocsys.asm.
;;
;; Copyright 1989, NeXT Inc.
;; Author - J. O. Smith
;;
;; DESCRIPTION
;; This is the bootstrap reset handler executed by the 56001 automatically
;; after loading the first 512 words of mkmon8k.lod in response to a hardware 
;; reset.  The bootstrap reset overrides the normal reset. When executed, it
;; loads the offchip portion of mkmon8k.lod and installs the normal reset 
;; vector. (I.e., location p:1 is changed to point into degmon instead of
;; reset_boot.)  Afterwards, the reset_boot handler is overwritten by user
;; code.
;;
;; The following actions are performed by the bootstrap:
;;     * Tests and clears external ram (write/read/compare $A's, $5's, ramp)
;;     * Downloads external system-initialized external memory 
;;	  and internal data through the host interface.
;;     * Executes normal reset exception handler (host command 0) via 'jmp 0'
;;
;; Modification history
;; 04/23/90/jos - Simplified default boot prog to "jmp >idle_1"
;;
reset_boot ; temporary reset handler for downloading dsp system at boot time.

	nop			; These are sometimes overwritten 
	nop			;   with a JSR to bypass reset on startup
        movec #$300,sr	        ; mask all interrupts during reset

; 	Turn on host interface
        bset #0,x:$FFE8	      	; Set Host Receive-data Interrupt Enable in HCR
        bset #2,x:$FFE8	      	; Set Host Command Interrupt Enable in HCR
	bclr #3,x:$FFE8       	; clear "DSP Busy" flag HF2
	bclr #4,x:$FFE8       	; clear unused flag HF3
        bset #0,x:$FFE0	      	; Configure port B as Host Interface (PBC)

	jsr send_R0		; Write boot-load pointer to host interface

; 	Enable external RAM on older machines:
	bclr #3,x:$FFE1 	; make pc3 a general purpose IO pin
	bset #3,x:$FFE3 	;	 an output
	bclr #3,x:$FFE5 	;	 containing zero.
	movep #0,x:$FFFE	; clear BCR (0 ext mem wait states)

	if USE_TXD_INTERRUPT
; 	Enable TXD (transmit data of SCI serial port) as gen. purpose output
	bclr #1,x:$FFE1 	; make pc1 a general purpose IO pin
	bset #1,x:$FFE3 	;	 an output
	bset #1,x:$FFE5 	;	 containing 1 (0 to interrupt CPU).
	endif

	;*** potential race: HF0 was set to enable this code to execute.
	; The booting program must not clear HF0 until after the memory
	; diagnostic results are sent back (or the dummy 0 is sent back).
 	jclr  #3,x:$FFE9,boot_load_done ; skip boot-load if simulating (!HF0)

;  *** ram diagnostic tests ***
;  If HF1 is set, test external RAM, sending results in R0 below.
;  For either a successful test or no test, 0 is returned.
	move #0,R0		; value to return if not doing mem test
	jsset #4,x:$FFE9,clear_and_test_memory 
	clr B
; ********************** download remainder of system ***********************

; *** Get download specification from the host (libdsp/DSPBoot()) ***
next_segment
	jsr send_R0		; send RAM test results, then final write ptrs
	jsr get_A		; memory space
	tst A
	jeq boot_load_done	; memory space 0 means done
	move A,B
	jsr get_A		; load address to R0
	move A1,R0
	jsr get_A		; word count to X0
	move A1,X0

; *** decode memory space ***

	move #1,A1		; 1 means x memory

	cmp A,B	#2,A1
	jeq x_load	

	cmp A,B	#3,A1
	jeq y_load		; 2 means y memory

	cmp A,B	#4,A1
	jeq l_load		; 3 means l memory

	cmp A,B
	jeq p_load		; 4 means p memory

	jeq boot_load_done	; anything else means no more

; *** memory space load ***

x_load	do X0,xl_loop
		jsr get_A	   ; host data to A1
	        move a1,x:(R0)+	   ; store word where it goes
xl_loop
	jmp next_segment

y_load	do X0,yl_loop
		jsr get_A	   ; host data to A1
	        move a1,y:(R0)+	   ; store word where it goes
yl_loop
	jmp next_segment

l_load	do X0,ll_loop		   ; *** l loads read two words per count ***
		jsr get_A	   ; host data to A1
	        move a1,x:(R0)	   ; store word where it goes
		jsr get_A	   ; host data to A1
	        move a1,y:(R0)+	   ; store word where it goes
ll_loop
	jmp next_segment

p_load	do X0,pl_loop
		jsr get_A	   ; host data to A1
	        move a1,p:(R0)+	   ; store word where it goes
pl_loop
	jmp next_segment

boot_load_done
;
; ================================================================
; reset_ - former dsp reset (jmp 0) exception handler
;; 	This former host-command handler is now only called at boot time.
;; 	Formerly, we uses hm_go to start a DSP program from this state.
;;	At present, we are always hard-resetting and rebooting the DSP,
;;	and the former "soft reset" is no longer supported.  The vector
;;	at p:0..1 is now used by DEGMON.
;
reset_
     jsr reset_soft		; Set up default state

; Clear user memory
     move #0,X0			; source of zero
     move #0,X1			; source of zero (X used for long words)
     move #>UNUSED_MARKER,Y0	; source of marker for unused p memory (SWI)

;  ****************************** RAM PRESET *********************************
;  If HF1 is set, clear external RAM.
;  Otherwise, leave it alone.
;  For obvious reasons, when simulating, leave HF1 clear on startup (default)

     jclr #4,x:$FFE9,reset_clear_done

; Clear external user memory (works for ONE_MEM case only)
     if NE_USR>0	      ; It is a real crock to need this test
     move #LE_USR,R0	      ; start address (LE_USR not defined for SEP_MEM)
     move #NE_USR,R1	      ; loop count
     do R1,pec_loop
	  move Y0,p:(R0)+     ; store word where it goes
pec_loop
     endif
	if !ONE_MEM
		fail 'user memory clear not written for multiple memory banks'
	endif
; Clear internal user x memory 
     if NXI_USR>0
     move #XLI_USR,R0
     move #NXI_USR,R1
     do R1,xic_loop
	  move X0,x:(R0)+
xic_loop
     endif
; Clear internal user y memory 
     if NYI_USR>0
     move #YLI_USR,R0
     move #NYI_USR,R1
     do R1,yic_loop
	  move X0,y:(R0)+
yic_loop
     endif
; Clear internal user l memory 
     if NLI_USR>0
     move #LLI_USR,R0
     move #NLI_USR,R1
     do R1,lic_loop
	  move X,l:(R0)+
lic_loop
     endif

; CAN'T Clear internal user p memory 
; because we are running in it right now!
;;     if NPI_USR>0
;;     move #PLI_USR,R0
;;     move #NPI_USR,R1
;;     do R1,pic_loop
;;	  move Y0,p:(R0)+
;;pic_loop
;;     endif

reset_clear_done

;  **************************** DEFAULT PROGRAM *******************************

	move #r_t1,R0		; 1st word of default program (below)
	move #PLI_USR,R1	; Standard user-program starting address
	do #(r_t_end-r_t1),r_defprog_loop ; Install default program
		move p:(R0)+,A
		move A,p:(R1)+
r_defprog_loop

;; Clear stack, configure reset vector for DEGMON, and "return" to idle loop
	clear_sp		; set stack pointer to base value (misc.asm)
     	movec #idle_2,ssh	; "jmp idle_2" after rti (cf. hmlib.asm)
				; DSPBoot() expects the IDLE message
     	movec #0,ssl		; lower execution priority to user level
     	nop			; wait for ssl to soak in
	move #>$BF080,X0	; Get "JSR" opcode (two-word type)
	movem X0,p:0		; Install it
	if DEGMON
	  move #>DEGMON_RUN_LOC,X0 ; DEGMON's "run" command (used by BUG56)
	else
	  move #>reset_soft,X0	; whatever
	endif
	movem X0,p:1
     	rti

;; ================================================================
; Default DSP program after RESET
r_t1
	jmp >idle_1		; Jump silently to idle loop after a reset
r_t_end

; ================================================================
; reset_soft ; reset ALU and DMA state
;
;; DESCRIPTION
;;	reset_soft was originally the first routine called by a freshly loaded
;;	user program.  It resets the running status, dma status, system
;;	variables, M and N registers, system buffers, interrupt priorities,
;;	and host flags.  This was when we tried to avoid rebooting the DSP
;;      between accesses to the DSP.  Due to lack of memory protection, this
;;	convention was abandoned, and now we always reset and reboot the DSP
;;	between accesses.  Therefore, this routine was moved from jsrlib where
;;	it could be called from anywhere, to reset_boot where it can only be
;;	called after a boot-reset.  It can be absorbed into reset_ above
;;	and dispensed with altogether.  reset_soft is also called by 
;;	stand-alone tests (under BUG56) in place of reset_boot.
;;
	xdef reset_soft
reset_soft
	movec #>I_DEFOMR,OMR   	; refresh default OMR for ROMs (config.asm)

; Initialize registers and memory variables used by both APMON and MKMON

; Set Mn to -1 (done by chip reset)
; Set Nn to 1 (not done by chip reset) 
; *** FIXME: don't do this since we do not make assumptions about Nn
	move #<1,n0	      
	move n0,n1
	move n0,n2
	move n0,n3
	move n0,n4
	move n0,n5
	move n0,n6
	move n0,n7

	move M0,x:X_DMA_R_M	; initial M index register for DMA reads
	move M0,x:X_DMA_W_M	; initial M index register for DMA writes

	move #>PLI_USR,X0	; default user start address
	move X0,x:X_START	; current user start address

; Initialize memory variables used only by MKMON

	if !AP_MON

	clr A

; Initialize run status and DMA status
	move A1,x:X_DMASTAT	; clear DMA state register
	move A1,y:Y_RUNSTAT    	; set default running status

; Initialize system L and X variables
	move A,l:L_TICK	      	; zero tick count
	move A,l:L_TINC	      	; zero tick increment (orchestra starts paused)
	move #>I_NCHANS,X0
	move X0,x:X_NCHANS	; No. of audio channels computed
	move A,x:X_NCLIP        ; zero accumulated number of clips
	move A,x:X_MIDI_MSG	; current MIDI message
	move A,x:X_SCI_COUNT	; zero SCI timer count
;*	move A,x:X_SCRATCH<n>	; SCRATCH memory is NOT zeroed
				; handler sine_test depends on this!
	endif ; !AP_MON

	jsr init_buffers	; initialize buffer pointers (jsrlib.asm)
	movep #>I_DEFIPR,x:$FFFF ; set default interrupt priority (config.asm)
	movec #0,ssl	      	; default status reg (all interrupts enabled)
	nop		      	; wait for ssl to dry
	rts
; ================================================================
get_A ; Read word from host interface HRX to A
	jclr #0,x:$FFE9,get_A   ; wait for HRDF in HSR (hif data ready)
	move x:$FFEB,A	        ; get next word from host interface
	rts

; send_R0 Send R0 to host
send_R0	jclr #1,x:$FFE9,send_R0 ; wait for HTDE in HSR of host interface
	move R0,x:$FFEB	   	; send R0 to host
	rts

; ================================================================
clear_and_test_memory  ; verify external RAM while clearing it
		; R0 is set to 0 on success
		;    nonzero (location of problem) otherwise
	bset #4,x:$FFE8	; set HF3 to indicate we are in external RAM test
;
; *** Test image 2 of external memory
;
	remember 'DSP partitioned RAM diagnostics disabled'
	if 0				; *** FIXME ***
	  move #LE_SEG,R6		; low external partitioned RAM start
	  move #>NXE_SEG,X0		; size of x partition = size of y part.
	  move #>1,Y0
	  clr B
	  do X0,cm_loop0
	       add Y0,B
	       move B1,x:(R6)	   ; write counter value into x cell
	       add Y0,B B,A	   ; increment, save x result
	       move B1,y:(R6)	   ; write counter value into y cell
	       move x:(R6),X1	   ; read back count
	       move #6,R0	   ; indicate where we are
	       cmp X1,A
	       jne cm_bad
	       move y:(R6),X1	   ; read back count
	       move #7,R0	   ; indicate where we are
	       cmp X1,B
	       jne cm_bad

	       move #8,R0	   ; indicate where we are
	       add Y0,B		   ; increment
	       move B1,p:(R6)	   ; write counter value into p cell = x cell?
	       move x:(R6),X1	   ; read back count
	       move p:(R6)+,A	   ; read back count (x/y~ floats up), BUMP R6
	       cmp X1,A		   ; make sure x = p here
	       jne cm_bad
cm_loop0  
	endif			; *** FIXME ***
;
; *** Test image 1 of external memory
;
	  move #LE_RAM,R6		; low external RAM start address
	  move #>(HE_RAM-LE_RAM+1),X0	; size of external RAM
	  move #>1,Y0
	  clr B
	  do X0,cm_loop
	       add Y0,B
	       move #1,R0	   ; indicate where we are
	       move B1,x:(R6)	   ; write counter value into memory
	       move x:(R6),X1	   ; read back count
	       cmp X1,B
	       jne cm_bad
	       move #2,R0
	       move #$555555,X1	   ; alternating 1,0 pattern
	       move X1,x:(R6)	   ; write pattern into memory
	       move x:(R6),A	   ; read back pattern
	       cmp X1,A
	       jne cm_bad
	       move #3,R0
	       move #$AAAAAA,X1	   ; other alternating 1,0 pattern
	       move X1,x:(R6)	   ; write pattern into memory
	       move x:(R6),A	   ; read back pattern
	       cmp X1,A
	       jne cm_bad
	       if ONE_MEM
		    move #4,R0
		    move y:(R6),A
		    cmp X1,A
		    jne cm_bad
		    move #5,R0
		    movem p:(R6),X1
		    cmp X1,A
		    jne cm_bad
	       else
		    fail 'need to write mem test for y and p ext banks'
	       endif
	       move  #0,X1
	       movem X1,p:(R6)+	; clear memory contents and go on to next
cm_loop	  
	  move #0,R0
	  bclr #4,x:$FFE8 ; clear HF3 to indicate we exited external RAM test
	  rts
cm_bad
	  bclr #4,x:$FFE8 ; clear HF3 to indicate we exited external RAM test
	  opt now
	  enddo		; assembler warns of enddo not inside do loop
	  opt w
	  rts

; ================================================================
; init_buffers - reset Host Message stack, Timed Message Queue, DMA buffers
init_buffers

; DMQ = DSP Message Queue
     move #>YB_DMQ,X0	      ; No markers or error checking for outgoing case
     move X0,x:X_DMQRP	      ; DMQ read pointer
     move #>(YB_DMQ+1),X0     ; If read pointer = write pointer, DMQ blocks
     move X0,x:X_DMQWP	      ; DMQ write pointer

; HMS = Host Message Stack
     move #YBTOP_HMS,R_HMS    ; Host Message Stack (DEcremented by host_rcv)
     move #NB_HMS-1,M_HMS     ; Make it a circular buffer
     move #HMS_TOPMK,X0
     move X0,y:(R_HMS)-	      ; Mark top of HMS (grows down)
     move R_HMS,x:X_HMSRP     ; Initial HMS read pointer
     move R_HMS,x:X_HMSWP     ; Initial HMS write pointer
     move #HMS_BOTMK,X0
     move X0,y:YB_HMS	      ; Mark bottom of HMS

     if !AP_MON

     jsr clear_dma_ptrs	      ; flush and reset DMA pointers

;  If HF1 is set, clear DMA buffers by actually writing zeros into them
;  Leave HF1 zero when simulating.  Note that we can only be here during
;  a bootstrap reset.  Thus, it never comes up whether to set HF1 at any
;  other time for this purpose.

	jsset #4,x:$FFE9,wd_buffer_clear ; zero out write-data buffers
	if READ_DATA
	   jsset #4,x:$FFE9,rd_buffer_clear ; zero out read-data buffers
	endif

; TMQ = Timed Message Queue
     move #TMQ_TAIL,X0	      ; TMQ tail marker
     move X0,y:YB_TMQ	      ; Mark tail of Timed Message Queue (grows up)
     move #YB_TMQ,X0	      ; TMQ base address
     move X0,x:X_TMQRP	      ; Initial TMQ read pointer at tail marker

     move #(YB_TMQ+1),R_I1	; address of 2nd wd of TMQ = null message
     move R_I1,x:X_TMQWP 	; TMQ write pointer -> marker after null message
     move #<0,X0		; write null msg to TMQ
     move X0,y:(R_I1)+		; lo-order word of time stamp
     move X0,y:(R_I1)+		; hi-order word of time stamp
     move X0,y:(R_I1)+		; word count = 0
     move #TMQ_HEAD,X0		; head marker
     move X0,y:(R_I1)		; write it to TMQ

     endif ; !AP_MON

     rts

;; ******************* BOOT-TIME ROUTINES USED BY MKMON ONLY ******************

	if !AP_MON

;; ============================================================================
;; sine_test - generate n packets of a test sinusoid at 172.2656 Hz and above
;;	ARGUMENTS
;;		X0 = desired number of packets sent. (1 packet = 1 DMA xfer)
;;
;;	NOTE: This function can only be called as the first thing after
;;	a reset_boot.  If anything else is downloaded, it will get overwritten.
;;
;; 06/06/89/jos - changed "unmask_host" to "jsr abort_interrupt"
;;		  The wd request was going out (in the simulator)
;;		  right away, and because we were at handler level
;;		  (host masked), the double saving of alu/temps was detected.
;;
sine_test
	move X0,x:X_SCRATCH1		; save arg so it will survive reset
;
;  Start up write data
;
        bset #B__HOST_WD_ENABLE,x:X_DMASTAT ; enable write data to host

;*	define RAMP '1'
;*	define CHIRP '0'		; quadratic phase (if !RAMP)

	define RAMP '0'
	define CHIRP '1'		; quadratic phase (if !RAMP)

	mask_host			; it does not work to simply
	jsr wd_dma_request		; turn off DSP messages and then
					; turn them back on (HTIE left off)
	jsr abort_interrupt		; go from handler level to user level

	if RAMP
	  move #0,A			; initial ramp value
	  move A,x:X_SCRATCH2		; saved value
	else
	  move #$100,R_I1		; start address of sine table
	  move R_I1,x:X_SCRATCH2	; saved table address (osc phase)
	  move #>1,N_I1			; initial increment
	  move N_I1,p:P_SCRATCH1
	endif

st_buffer_loop
	move x:X_SCRATCH1,A		; remaining buffer count
	tst A #>1,X0
	jle st_done
	sub X0,A
	move A,x:X_SCRATCH1

	;***TEST***
;	move #DE_USER_ERR,X0		; hack for monitoring events
;	or X0,A
;	jsr dspmsg			; send remaining buffer count
	;***ENDTEST***

	move x:X_DMA_WFP,R_O       	; current write-fill pointer

	if RAMP
	  move x:X_SCRATCH2,A		; ramp value
	  move #>256,Y1			; increment (l.s. byte not shipped)
	  move #0,X0			; reset value
	else
	  move x:X_SCRATCH2,R_I1	; oscillator phase (current table addr)
	  move p:P_SCRATCH1,N_I1	; increment
	  move #$0FF,M_I1		; set for modulo 256 addressing
	endif

	do #I_NDMA/2,st_sineloop	; have stereo interleaved data
	  if RAMP
	    add Y1,A			; increment ramp
	    tes X0,A			; clear on overflow
	  else
	    move y:(R_I1)+N_I1,A	; get next sine value
	    rep #6
	      asr A			; save our ears
	  endif
	  move A,y:(R_O)+		; replace left-channel value in buffer
	  move A,y:(R_O)+		; replace right-channel value in buffer
st_sineloop				; frequency is 44100/256 = 172Hz
	if RAMP
	  move A,x:X_SCRATCH2		; save value
	else
	  move R_I1,x:X_SCRATCH2	; save phase
	endif
	if CHIRP
	  move N_I1,R_I1		; increment phase increment
	  nop
	  lua (R_I1)+,N_I1		; by 1 with modulo wrap-around
	endif
	if !RAMP
	  move N_I1,p:P_SCRATCH1	; save increment
	endif
	move #-1,M_I1			; assumed by routines we'll call below

	move x:X_SCRATCH1,A		; remaining buffer count
	tst A				; test here to avoid last wd out
	jle st_done
	jsr hm_write_data_switch     	; Switch write buffers and ship sound
	jmp st_buffer_loop
st_done
	jsr stop_host_write_data	; Inhibit any more R_REQs
	jsr hm_write_data_switch     	; Enable last DMA
st_buzz	jset #B__HOST_WD_PENDING,x:X_DMASTAT,st_buzz ; wait for last DMA
	jsr idle_0			; at least clear 'B__HOST_WD_ENABLE'
;*	jsr abort			; at least clear 'B__HOST_WD_ENABLE'

	endif ; !AP_MON

P_SCRATCH1	dc $0	; scratch memory for USER LEVEL ONLY



