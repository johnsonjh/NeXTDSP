; hmlib.asm - Host message library
;
;; Copyright 1989,1990 NeXT Inc.
;; J.O. Smith
;;
;;   Included by allocsys.asm.
;;
;;   These routines comprise the set of host messages available for 
;;   communication from the host to the DSP via host commands.
;;   Host messages are issued via the xhm host command.
;;   See /usr/lib/dsp/smsrc/handlers.asm for the xhm* host-command exception handlers
;;   and the jsr_hm routine that calls these routines.
;;
;;  *** EACH ROUTINE HERE MAY BE CALLED AT HOST INTERRUPT LEVEL ***
;;	The xhm host-command handler calls (in handlers.asm) 
;;	  save_alu,   which saves A2,A1,A0,B2,B1,B0,X1,X0,Y1,Y0,  and
;;	  save_temps, which saves R_I1,R_I2,R_O,N_I1,N_I2,N_O,M_I1,M_I2,M_O
;;      Any other resources needed must be saved and restored explicitly here.
;;
;;  ***	R_I1 MUST BE SAVED AND RESTORED IF SUBROUTINES ELSEWHERE ARE CALLED
;;	SUCH AS IN JSRLIB.ASM.
;;
;;  *** On dismissal here, register R_I1 must point to one before the
;;	first host-message argument written by the host.
;;	This happens naturally when the arguments are consumed sequentially
;;	as y:(R_I1)+ for each argument. Each host message handler must
;;      make this condition true or else a DE_HMARGERR DSP message will
;;	be generated.  The check detects if there is
;;      an error in the number of arguments written by the host OR
;; 	the number consumed by the host message handler.  The checking
;;      of R_I1 is done at label xhm8 in handlers.asm.
;;
;;   Host messages to consider adding:
;;	getMemoryConstants (so memory-map can be changed on the fly)
;;
;; ----------------------------------------------------------------
;;
;;   The host message handlers below can be called in two ways:
;;	  (1) In response to an untimed host message
;;	  (2) By the service_tmq routine to execute a timed host message
;;
;;   Case (1) is at interrupt level, and case (2) is at user level.
;;
;; ----------------------------------------------------------------
;; Modification history
;;
;;  02/26/90/jos - Added timed-zero message support at idle_1 (grep TZM *.asm)
;;  05/04/90/jos - Flushed echo_0 loopback test code
;;  05/04/90/jos - Absorbed halt_0 into hm_halt
;;
;; ----------------------------------------------------------------
;; HMS FORMAT
;;
;; Host Message Stack (HMS) state upon entry to any of the hm routines below:
;;
;;   See handlers.asm
;;
;; ----------------------------------------------------------------
;;
;; TMQ FORMAT
;;
;; Timed Message Queue (TMQ) state upon entry to any of the hm routines below:
;;
;; 		   TMQ_HEAD	   ; first free element (marked)
;;		       0 	   ; Empty message terminates TMQ (0 wd count)
;;		       0	   ; hi-order word of final time stamp = 0
;; x:X_TMQWP -->       0	   ; lo-order word of final time stamp = 0
;;		      <.>	   ; more messages
;;	link -->    TMQ_MEND	   ; non-opcode denoting end of message packet
;;	   	     <arg1>	   ; first argument written by host
;;	   	      ...	   ; other arguments (handler knows how many)
;;	  +          <argN>	   ; last argument or nargs
;;	  -	    <opcode>	   ; can cat opcodes (R_I1 updated in handler!)
;;	  	     <arg1>	   ; first argument written by host
;;	  	      ...	   ; other arguments
;;	R_I1 -->     <argN>	   ; last argument written by host (or nargs)
;;		    <opcode>	   ; host message dispatch address
;;		     <link>	   ; pointer to next msg, 0, or words remaining
;;		  <timeStampHi>	   ; high-order word of absolute time stamp
;;		  <timeStampLo>	   ; low-order word of absolute time stamp
;; x:X_TMQRP -->    TMQ_TAIL	   ; Tail marker
;;
;; NOTES
;; - When the TMQ is empty, x:X_TMQRP + 1 =  x:X_TMQWP, and the TMQ 
;;   consists of only a tail marker, a null message, and a head marker.
;; - If <link> is less than YB_TMQ, it is assumed to be a remaining-word count.
;; - If <link> is 0, opcodes are executed until TMQ_END appears where 
;;   the next opcode would. Multi-opcode execution is more dangerous because
;;   it relies on the corresponding host message handler returning with R_I1
;;   pointing to the next opcode. It is safer but slower to use only one 
;;   opcode per timed message.
;;
;;----------------------------------------------------------------
;
; See hmdispatch.asm for entry points for the routines below.
;
; **************** ROUTINES CALLED BY BOTH APMON AND MKMON *******************
;
; ================================================================
; idle_0 - Place DSP system in the idle state.
;	Called by hm_idle.
;;
;; Forcing idle from the host (via hm_idle) is useful for debugging.
;;   It changes the minimum amount of state necessary to get
;;   the DSP's attention.  After forcing idle, the host may
;;   read whatever state it wants with the DSP stopped.
;;
;;   FIXME: To really do this right, idle_0 should save
;;   all modified state before modifying.
;;
;; If a DMA read is in progress, it is terminated exactly
;; as in hm_host_r_done0 above.  The DM_HOST_R_DONE message
;; before the DM_IDLE message indicates that the DMA was aborted.
;;
idle_0	
	if !AP_MON
	jsr dma_abort		; Let any DMA in progress finish, then disable
	endif
	jsr abort_interrupt	; clear state associated with interrupt
idle_2  clr A #DM_IDLE,X0   	; tell host this happened (entry from reset_)
	jsr dspmsg	    	; ok to clobber registers
idle_1  
	if !AP_MON		; busy wait. * THIS IS THE DEFAULT IDLE LOOP *
	jsset #B__TZM_PENDING,x:X_DMASTAT,loc_xhmta_return_for_tzm
	endif
	jmp idle_1	  	; busy wait. * THIS IS THE DEFAULT IDLE LOOP *
;
; ================================================================
dm_off0    	; turn off DSP messages
		bset #B__DM_OFF,x:X_DMASTAT
		move #DM_DM_OFF,X0
dm_off_buzz	jclr #1,x:$FFE9,dm_off_buzz	; wait for HTDE
		move X0,x:$FFEB	; send final DSP message
		bclr #1,x:$FFE8	; Clear Host Transmit Intrpt Enable (HTIE)
		rts
; ================================================================
dm_on0    	; turn on DSP messages
		; Two "garbage words" will be read from the host interface
		; before the ack is seen.  The ack is effectively inserted
		; in front of all pending DSP messages.
		bclr #B__DM_OFF,x:X_DMASTAT
		move #DM_DM_ON,X0
dm_on_buzz	jclr #1,x:$FFE9,dm_on_buzz	; wait for HTDE
		move X0,x:$FFEB	; send message demarcating msg start
		bset #1,x:$FFE8	; Set Host Transmit Intrpt Enable (HTIE)
		rts
; ================================================================
host_r0    	; host wants to read a block of data.
;
; ARGUMENTS (in the order written by the host)
;   space     - memory space of destination address (x=1,y=2,p=4)
;   address   - address of first word
;   increment - skip factor (e.g. 1 means contiguous locations)
;
; Note that the M index register used in the transfer is x:X_DMA_R_M.
; This register defaults to -1 and can be set to any value via a host 
; message.
;
; Host-initiated reads such as this can only happen on channel 0.
; Hence no channel number argument.
;
	move y:(R_I1)+,A		; DMA skip factor (last arg)
	move #DM_HOST_R_SET1,X0		; HOST_R setup word 1
;	! We need not save R_I1 for dspmsg because it saves and restores 
;	whatever regs it uses.
	jsr dspmsg			; enqueue read request
	move y:(R_I1)+,X1		; DMA start address
	move y:(R_I1)+,A		; memory space code (first arg)
	move #0,X0			; pure data word (HOST_R_SET2)
	jsr dspmsg
	tfr X1,A #0,X0			; buffer address to A (HOST_R_SET3)
	jsr dspmsg			; enqueue read request
	rts
; ================================================================
; host_r_done0 - host has finished reading block of DSP private memory
;
;	Note that the current DMA channel number is stored in p:hx_channel
;	We can refer to it if we need to know.  There is also p:hx_space.
;
host_r_done0
	jclr #B__HOST_READ,x:X_DMASTAT,hrd_ignore ; Not in pdma mode
			; The simulator can cause this in a case like
			; sound-out (see dspbeep.c). This means the r_done 
			; is lost.

	; ! exit_pdma_r_mode does not modify R_I1 and promises not to
	jsr exit_pdma_r_mode ; (handlers.asm) Restore DSP Message ntrpt handler

	; WD_PENDING is set within host_read_request when a write-data
	; is definitely the current DMA read to the host.  There is nothing
	; that can stop the write-data transfer except this host message.
	; (Actually, the hm_idle host message below will tear it down.)
	; No error termination is provided, so we assume a successful WD.
	;
	bclr #B__HOST_WD_PENDING,x:X_DMASTAT ; Clear pending DMA WD status

	move R_IO,A			; send address pointer as arg
	move #-1,M_IO			; not necessary, but convention
	move #DM_HOST_R_DONE,X0		; load message code
	or X0,A		    		; install opcode with arg
	move A,x:$FFEB			; overwrite 2nd garbage word with ack

	bclr #B__DM_OFF,x:X_DMASTAT	; enable DSP messages
;
;	  Note that the R_DONE ack gets inserted ahead of all pending
;	  DSP messages.  It is preceded by one garbage word in the
;	  host interface.  If a DM_ON message is sent before reading
;	  any words from RX, the DM_ON ack will follow immediately behind
;	  this ack.  Thus, you'll see <garbage-word>,R_DONE,DM_ON,<msg>,<msg>,
;
;*d*	  move x:X_SAVED_R_I1_DMA,R_I1	; to pass arg-check at xhm_host_r
	  ; leave HTIE set in case there are any waiting DSP msgs
hrd_ignore
	  rts  
; ================================================================
; host_w - prepare DSP for DMA to private memory from host.
;
; ARGUMENTS (in the order written by the host)
;   space     - memory space of destination address (x=1,y=2,p=4)
;   address   - address of first word
;   increment - skip factor (e.g. 1 means contiguous locations)
;
;; Note that the M index register used in the transfer is x:X_DMA_R_M.
;; This register defaults to -1 and can be set to any value via a host 
;; message.
;;
;; Additional notes:
;;	Host should wait until the "HC" and "DSP Busy" Flags clear
;;	before starting transfer.  DMA channel 0 is implicit.
;;
;; DESCRIPTION
;;   This is how all DMA transfers are done from the host to the DSP.
;;   After this host message completes (HF2 clears), all writes by the
;;   host to the TX register will go to the destination set up by 
;;   this host message.  Host messages are therefore disabled.
;;   The transfer is terminated via the hc_host_w_done host command which
;;   terminates the routing of TX writes to the destination array
;;   and restores the normal routing of TX to the HMS.
;;
;;   Note: It is not actually necessary to place the DSP host interface
;;   in DMA mode for the "DMA write." Instead, a sequence of programmed 
;;   IO writes to the TX register can be performed with the same result.
;;
;;   Execution of this host command sets a status bit #B__HOST_WRITE. 
;;
;;   DMA writes (host to DSP) use R_HMS since there is no other use for it.
;;   DMA reads use R_IO since R_HMS is then useable for host messages.
;;   Consequently, a simulated DMA write (polling on TXDE) can go 
;;   simultaneously with a true DMA read.
;;
host_w0   ; host wants to write a block of DSP private memory
	  jclr #B__HOST_WRITE,x:X_DMASTAT,dhw_ok ; this would kill saved regs
dhw_bad         move #DE_DMAWRECK,X0	; error code
	        jsr stderr		; issue error
dhw_ok	  bset #B__HOST_WRITE,x:X_DMASTAT ; claim DMA channel

	  move x:X_HMSRP,R_HMS		; Save *POPPED* HMS frame pointer
  	  move R_HMS,x:X_HMSWP		; (pop not executed on return)
	  move R_HMS,x:X_SAVED_R_HMS 	; Save HMS pointers for later restoral
	  move N_HMS,x:X_SAVED_N_HMS	; by host_w_done (below)
	  move M_HMS,x:X_SAVED_M_HMS
	  move y:(R_I1)+,N_HMS		; DMA skip factor (last arg)
	  move y:(R_I1)+,R_HMS		; DMA start address
	  move x:X_DMA_W_M,M_HMS	; indexing type

	  movem p:iv_host_rcv,X0	; Save host_rcv vector
	  move X0,x:X_SAVED_HOST_RCV1	;  = interrupt vector for HMS service
	  movem p:iv_host_rcv2,X0	; Save 2nd word of host_rcv vector
	  move X0,x:X_SAVED_HOST_RCV2	;  = nop normally
	  move y:(R_I1)+,A		; memory space code (first arg)
;*d*	  move R_I1,x:X_SAVED_R_I1_DMA	; save this for arg-check when done
	  move A1,N_I2			; offset
	  move #(*+3),R_I2		; pointer to 1st word of instruction -1
	  jmp >dhw1
dhw_x	  movep x:$FFEB,x:(R_HMS)+N_HMS	; DMA write to x data  memory
dhw_y	  movep x:$FFEB,y:(R_HMS)+N_HMS	; DMA write to y data  memory
dhw_l	  movep x:$FFEB,x:(R_HMS)+N_HMS	; DMA write to x data  memory
dhw_p	  movep x:$FFEB,p:(R_HMS)+N_HMS	; DMA write to program memory
dhw_nop	  nop
dhw1	  movem p:(R_I2+N_I2),X0	; 1st word of new host_rcv vector
	  movem X0,p:<iv_host_rcv	; Replace HMS host_rcv vector
	  movem p:dhw_nop,X0		; 2nd word of new host_rcv vector
	  movem X0,p:<iv_host_rcv2	; Drop it in place
;;	  bset #0,x:<HCR>		; Enable host_rcv interrupts (assumed)
	  ; Host knows we're ready for DMA by when HF2 clears => HC processed.
	  rts
;; ================================================================
; host_w_done  - terminate host-initiated write to DSP private memory
;
; ARGUMENTS: None (The HMS is disabled until this is executed!)
; This routine is called by the hc_host_w_done host command handler.
; It is not really a host message.  As a result, its 
;
;; DESCRIPTION
;;   When the host-to-DSP DMA was host-initiated, we only
;;   revive host message service and free up the DMA channel. 
;;   When the DMA transfer was DSP-initiated, we in addition
;;   clear the read-data sync flag so that the DSP knows it is done.
;;
host_w_done0				; called by hc_host_w_done
	  move R_HMS,A			; DSP message argument below
	  move x:X_SAVED_R_HMS,R_HMS    ; restore HMS service
	  move R_HMS,x:X_HMSWP		; for xhm_done error detection
	  move x:X_SAVED_N_HMS,N_HMS
	  move x:X_SAVED_M_HMS,M_HMS
	  move x:X_SAVED_HOST_RCV1,X0	; Restore host_rcv exception vector
	  movem X0,p:iv_host_rcv	;  which services host messages
	  move x:X_SAVED_HOST_RCV2,X0	; Second word of host_rcv vector
	  movem X0,p:iv_host_rcv2	; Restore host_rcv exception vector
	  bclr #B__HOST_WRITE,x:X_DMASTAT    ; Let go of DMA channel
	  rts  
;
; **************************** NON-DMA IO ************************************
; See also hmlib_mk.asm

peek_common 	tfr A,B
		and X0,A #DM_PEEK0,X0
		jsr dspmsg		; send low-order two bytes
		move B,X0 #>@pow(2,-16),Y0 ; right-shift 16 places
		mpy X0,Y0,A #>$FF,X0	; top byte of A1 to low byte, 16b mask
		and X0,A #DM_PEEK1,X0
		jsr dspmsg		; send high-order byte
	        rts
; ================================================================
; peek_x0 - read a single word from x memory
;
; ARGUMENTS
;   address   - address of peek
;
peek_x0 	move y:(R_I1)+,R_I2	; memory address
		move #>$FFFF,X0		; 16-bit mask
		move x:(R_I2),A1	; peek's return value
		jmp peek_common
; ================================================================
; peek_y0 - read a single word from y memory
;
; ARGUMENTS
;   address   - address of peek
;
peek_y0 	move y:(R_I1)+,R_I2	; memory address
		move #>$FFFF,X0		; 16-bit mask
		move y:(R_I2),A1	; peek's return value
		jmp peek_common
; ================================================================
; peek_p0 - read a single word from p memory
;
; ARGUMENTS
;   address   - address of peek
;
peek_p0 	move p:(R_I1)+,R_I2	; memory address
		move #>$FFFF,X0		; 16-bit mask
		move p:(R_I2),A1	; peek's return value
		jmp peek_common
; ******************************* CONTROL ************************************
; ================================================================
; say_something0 - request "I am alive" message from DSP
say_something0 	move #>sys_ver,X0		; system version
		move #>@pow(2,-16),X1		; 1 byte leftshift = 2 byte rsh
		mpy X0,X1,A #>sys_rev,X0 	; system revision
		move A0,A1			; result in A0
		or X0,A				; install sys_rev
	   	move #DM_IAA,X0			; load message code
		jsr dspmsg			; send it along
		rts
; ===============================================================
; set_start_0 - set execution start address to arg1
;; ARGUMENT
;;   16 bit start address, right-justified in 24 bits
set_start_0     move X0,x:X_SAVED_X0
		move y:(R_I1)+,X0
		move X0,x:X_START
		move x:X_SAVED_X0,X0
		rti
; ===============================================================
; go_0 - start execution at start_address (written above)
;;
;; ARGUMENTS	none
;;
;;		Reset the system stack and start user at (x:X_START).
;;
;;		Enough cleanup is done here so that the user program
;;		need not begin with a reset_soft.  Normally, after a
;;		host message, the code following label xhm_done in 
;;		/usr/lib/dsp/smsrc/handlers.asm is executed.  The code here
;;		attempts to arrive at the same state.
;;
go_0	       	move #0,sp		; clear stack
		clear_sp		; set stack ptr to base val (misc.asm)
		move x:X_HMSRP,R_HMS	; Read pointer. Points to first arg.
		move R_HMS,x:X_HMSWP	; R_HMS = write pointer
		bclr #B__ALU_SAVED,y:Y_RUNSTAT   ; We'll not restore ALU
		bclr #B__TEMPS_SAVED,y:Y_RUNSTAT ; nor temp regs (handlers.asm)
		jclr #B__HM_DONE_INT,y:Y_RUNSTAT,go_noint ; "done" msg req'd?
		   clr A #DM_HM_DONE,X0		; "host message done" message
		   jsr dspmsg
go_noint
		move x:X_START,ssh	; This sets sp to 1
		move #>0,ssl		; *** LEAVE INTERRUPTS ENABLED ***
		bclr #4,x:$FFE8		; Clear (HF3 = "TMQ full" or "AP busy")
		bclr #3,x:$FFE8 	; Clear (HF2 = "HC in progress")
		rti			; User is top level now
; ================================================================
; execute_0 - execute arbitrary instruction in arg1,arg2

	      if 1
execute_0       jsr unwritten_subr	; need the code space
	      else
xqt_0		dc 0
xqt_1		dc 0
		rts
execute_0       ; execute arbitrary instruction in arg1,arg2
; ARGUMENTS (in the order written by the host)
;   word1   - first word of instruction to execute
;   word2   - second word of instruction to execute
		move X0,x:X_SAVED_X0
	        move y:(R_I1)+,X0		; 2nd word
	        move X0,p:xqt_0
	        move y:(R_I1)+,X0		; 1st word
	        move X0,p:xqt_1
		jsr xqt_0			; do it
		move x:X_SAVED_X0,X0
	        rts
	        rts
	      endif
; ================================================================
; execute_hm0 - execute top frame of HMS or next frame of TMQ
;
; ARGUMENTS (in the order written by the host)
;   program text (relocatable)
;   number of words of program in the message
;
;; DESCRIPTION
;;   The purpose of this message is to allow execution
;;   of the host message as a program. 
;;   The code must be relocatable. For example,
;;   it cannot have any DO loops because DO loops require an
;;   absolute address for the loop termination.

scratch_prog_area   ; place to put program from message queue
	       dup NPE_SCR    ; memmap.asm
	       nop	      ; 
	       endm
	       rts	      ; in case we forget
	       
execute_hm0    ; execute top frame of host message stack
	       move #scratch_prog_area,R_O
	       move #>NPE_SCR,X0	; maximum program size in words
	       move y:(R_I1)+,A		; number of words in program
	       cmp X0,A			; see if it fits
	       jle eh_ps_ok
		    move #DE_SCROVFL,X0
		    jsr stderr			       
eh_ps_ok
	       do A1,eh_copy_loop
		    move x:(R_I1)+,X0
		    movem X0,p:(R_O)+
eh_copy_loop
	       	movem p:rts_instr,X0
	       	movem X0,p:(R_O)+   ; overwrite "execute_hm" with "rts"
	       	jmp scratch_prog_area
rts_instr	rts
;; ================================================================
; jsr_0 - execute arbitrary subroutine
; ARGUMENTS
;   address   - address of DSP subroutine to execute
jsr_0 		move X0,x:X_SAVED_X0
		move y:(R_I1),X0	; JSR address to X0
	        move X0,p:>(*+3)	; Poke it into the following "JSR"
	         jsr >0	 ; This calls the routine (or RESET if we missed ha ha)
		move x:X_SAVED_X0,X0
		rts
; ================================================================
; hostm_first0 - return first host-message dispatch address
hostm_first0 	move #>hm_first,A1
	        move #DM_HM_FIRST,X0
		jsr dspmsg
	        rts
; ================================================================
; hostm_last0 - return last host-message dispatch address
hostm_last0 	move #>hm_last,A1
	        move #DM_HM_LAST,X0
		jsr dspmsg
	        rts

; ================================================================
; set_dma_r_m0 - poke M register used by all DMA read transfers (to host)
set_dma_r_m0
		move y:(R_I1)+,X0	; M register to use
		move X0,x:X_DMA_R_M
		rts

; ================================================================
; set_dma_w_m0 - poke M register used by all DMA write transfers (to DSP)
set_dma_w_m0
		move y:(R_I1)+,X0	; M register to use
		move X0,x:X_DMA_W_M
		rts

; ================================================================

	if !AP_MON
		include 'hmlib_mk'
	endif ; !AP_MON (end of hmlib_mk include)



