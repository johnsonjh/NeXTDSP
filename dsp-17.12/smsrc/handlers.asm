; Interrupt and host-command exception handlers
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; Make sure every exception handler (which expects to execute in the 
;; background while the user runs) saves and restores any registers
;; it needs to modify.  Also, make sure that if it is interruptible,
;; every possible interrupting handler saves regs in a different place.
;; See hmlib.asm for "sub-host-command" (i.e. "host message") handlers.
;;
; **************** HANDLERS CALLED BY BOTH APMON AND MKMON *******************
;
;; ===========================================================================
    if !DEGMON
swi_	; for SIMULATING, set up a breakpoint on pc==6 (SWI vector)
	move #DE_BREAK,X0	; breakpoint opcode
	movec sp,Y0		; save sp
	movec ssh,A1		; current pc (sp post-decrements)
	movec Y0,sp		; restore sp

	if AP_MON
	  or X0,A		; make a word
	  move A1,x:$ffeb	; write to host interface
swi_buzz jclr #1,x:$ffe9,swi_buzz ; wait until word gets read	
	  jsr abort_now
	else
	  jsr stderr		; send "breakpoint" message
	  jsr abort		; clear state and resume idle loop
	endif
    endif
;; ===========================================================================
; host_rcv - We use fast interrupts for this
;; ===========================================================================
; host_xmt - handler for transmit data empty. Services DSP message Q output.
;
hx_space 	dc 0			; place to save DMA space code
hx_channel 	dc 0			; place to save DMA channel number

host_xmt
;* The following check is suppressed because it dies when simulating
;*	jclr #B__HOST_READ,x:X_DMASTAT,hx_nodma ; detect DMA conflict
;*		DEBUG_HALT	; DMA read always uses fast interrupt
;*hx_nodma
	jsr save_alu	      	; save alu registers
        jsr save_temps	      	; save temporary index registers
	jset #B__DM_OFF,x:X_DMASTAT,hx_dmoff ; Split if DSP Messages are off

	move x:X_DMQRP,R_O	; DMQ read pointer
	move #NB_DMQ-1,M_O	; modulo
	move x:X_DMQWP,A	; DMQ write pointer (initially readpointer+1)
	lua (R_O)+,R_O		; read pointer points one behind current
	move R_O,X0		; Compare to DMQ write pointer
	cmp X0,A y:(R_O),B	; DMQ empty? (readptr catches writeptr)
	jne host_xmt_not_empty  ; If DMQ empty, turn off host_xmt interrupt:
hx_dmoff	bclr #1,x:$FFE8	; Clear Host Transmit Intrpt Enable (HTIE)
		jmp host_xmt_empty
host_xmt_not_empty

	; Since HTDE caused this interrupt, we need not check for it here

; Check for HOST_R_SET1 and, if found, set up DMA (unless in abort mode)
	move #$FF0000,X0	; opcode mask
	and X0,B #DM_HOST_R_SET1,X0 ; host-read setup 1 opcode
	cmp X0,B y:(R_O),A
	jne hx_nowd
	move A1,N_IO		; skip factor for DMA transfer
	move y:(R_O)+,A		; increment R_O
	move y:(R_O),A		; get next word = space and channel number
	move #>$7,X0		; mask for isolating space code
	and X0,A #>$0001F8,X0	; get space code, load channel number mask
	move A1,p:hx_space	; save space code for entering PDMA mode
	move y:(R_O)+,A		; refresh message word
	and X0,A #@pow(2,-3),X0	; channel number << 3, 3-bit right-shifter
	move A,X1  		; prepare to shift
	mpy X1,X0,A y:(R_O),B	; shift it down, get next word = DMA address
	move A1,p:hx_channel	; save for entering PDMA mode
	move B,R_IO		; set up DMA index reg (N_IO setup above)
	move x:X_DMA_R_M,M_IO	; addressing type
	move p:hx_space,X1		; memory space code for write data
	jset #B__ABORTING,y:Y_RUNSTAT,hx_nowd	; nevermind if aborting
 	jsr enter_pdma_r_mode 	   	; below
;
; Clear HTIE for write data (channel 1) to hang the DMA.  When the currently
; filling buffer is ready, HTIE will be set in write_data_buffer_out.  HTIE
; clear does not happen for channel 0 because that is a user-initiated read
; which is always assumed ready to go.  There is no support yet for higher
; numbered channels.
;
	move p:hx_channel,A		; Channel number
	tst A #>1,X0			; Channel 1
	jeq hx_no_htie_clear 		; Channel 0 (RD/WD) leaves HTIE set
	cmp X0,A			; Make sure this is channel 1
	jeq hx_htie_clear
		DEBUG_HALT		; We don't yet support channels > 1
hx_htie_clear
;
; The following block of code pertains only to write-data which is
; channel 1 DMA to host
;
	bset #B__HOST_WD_PENDING,x:X_DMASTAT  ; mark WD pending status
	bclr #1,x:$FFE8		; Clear Host Transmit Intrpt Enable (HTIE)
	move #DM_HOST_R_REQ,X0  ; host-read request for host
	move p:hx_channel,A 	; channel is 1 now, but someday >1 possible
	or X0,A		 	; HOST_R_REQ is what host actually sees
hx_nowd ; Note: No need to check HTDE because it's why we're here
	jset #7,x:$FFE9,hx_nowd	; Wait until DMA bit is clear (ie INIT done)
	jset_hf1 hx_nowd	; misc.asm	; HF1 <=> host DMA in progress
	movep A,x:$FFEB  	; Write Host Transmit Data Register HTX
	move R_O,x:X_DMQRP	; Update incremented DMQRP if not empty
host_xmt_empty
	jsr restore_alu	      	; restore alu registers
        jsr restore_temps      	; restore temporary index registers
	rti

hx_no_htie_clear		; start DMA read for channel 0
	move p:hx_channel,A
	move #DM_HOST_R_REQ,X0  ; host-read request for host
	or X0,A 
	movep A,x:$FFEB  	; Write Host Transmit Data Register HTX
;
	remember 'Come here if DSP blocking on host read bothers you'
;
;	We have just sent an R_REQ to the host. We now have to wait until
;	that interrupt is processed and HF1 is turned on to indicate that
; 	the host interface has been initialized in DMA mode.  For chained
;	transfers, this is efficient because the kernel is sitting in the DMA
;	complete handler reading all the messages it can (i.e., we'll only 
;	sit here a usec or so which can be used in the reg restores below).
; 	For user-requested reads, we have to wait for the DSP interrupt to
;	be serviced which will take tens of microseconds or so = hundreds
;	of wasted DSP instructions!
;
; 	Solution: Write two zeros into the pipe (polling HTDE) and dismiss.
;	When the DMA INIT is finally done, those two words will be clobbered
;	and the desired data will follow behind. The only bad point is that
;	there is in principle a race.  However, we can write those two words
;	immediately behind the R_REQ, and the host cannot beat us:
;
;hx_bz1	jclr #1,x:$FFE9,hx_bz1	; await HTDE
;	movep A,x:$FFEB  	; Write Host Transmit Data Register HTX
;hx_bz2	jclr #1,x:$FFE9,hx_bz2	; await HTDE
;	movep A,x:$FFEB  	; Write Host Transmit Data Register HTX
;
	move R_O,x:X_DMQRP	; Update incremented DMQRP if not empty
	jsr restore_alu	      	; restore alu registers
        jsr restore_temps      	; restore temporary index registers
hx_buz0	  jclr #4,x:$FFE9,hx_buz0 ; wait for HF1 (means host did the DMA INIT)
	rti			; Let's get out of here!

;; ===========================================================================
; enter_pdma_r_mode - Set up "pseudo-DMA read mode".
;	Called by the host_xmt handler.
;	*** ASSUMES IPL SUCH THAT host_xmt CANNOT HAPPEN ***
;
;	Replace the default host_xmt interrupt handler (DSP messages) with
;	one which services a DMA read from the DSP to the host.  The host
;	may use a real DMA or it may simply read RX for the desired number
;	of words.  Arguments are R_IO, N_IO, and M_IO properly set up,
; 	and a memory space code (x,y,l,p = 1,2,3,4) in X1.
;
enter_pdma_r_mode
	bset #B__DM_OFF,x:X_DMASTAT	; turn off DSP messages
	bset #B__HOST_READ,x:X_DMASTAT	; Indicate PDMA read mode in progress
	movem p:iv_host_xmt,X0		; Save host_xmt vector
	move X0,x:X_SAVED_HOST_XMT1	;  = "JSR"
	movem p:iv_host_xmt2,X0		; Save 2nd word of host_xmt vector
	move X0,x:X_SAVED_HOST_XMT2	;  = "host_xmt" handler address
	tfr X1,A			; memory space code (arg)
;*d*	move R_I1,x:X_SAVED_R_I1_DMA	; save this for arg-check when done
	move A1,N_I2			; offset in table below
	move #(eprm_vectab-1),R_I2	; pointer to 1st word of table - 1
eprm_nop nop
eprm1	movem p:(R_I2+N_I2),X0		; 1st word of new host_xmt vector
	movem X0,p:<iv_host_xmt		; Replace host_xmt vector
	movem p:eprm_nop,X0		; 2nd word of new host_xmt vector
	movem X0,p:<iv_host_xmt2	; Drop it in place
	bset #1,x:$FFE8			; Set HTIE in the HCR (redundant)
     	rts				; Exit enter_pdma_r_mode

eprm_vectab
	movep x:(R_IO)+N_IO,x:$FFEB	; DMA read from x data  memory
	movep y:(R_IO)+N_IO,x:$FFEB	; DMA read from y data  memory
	movep x:(R_IO)+N_IO,x:$FFEB	; DMA read from x data  memory (l)
	movep p:(R_IO)+N_IO,x:$FFEB	; DMA read from program memory
;; ===========================================================================
; exit_pdma_r_mode - restore interrupt handler for  DSP Message Queue
exit_pdma_r_mode	; called by the host_r_done0 host message
; *** DO NOT HARM R_I1 HERE ***
	bclr #B__HOST_READ,x:X_DMASTAT 	; DMA read no longer in progress
	move x:X_SAVED_HOST_XMT1,X0	; Restore host_xmt exception vector
	movem X0,p:iv_host_xmt		;  which services host messages
	move x:X_SAVED_HOST_XMT2,X0	; Second word of host_xmt vector
	movem X0,p:iv_host_xmt2		; Restore host_xmt exception vector
	rts

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================

;  HOST COMMAND HANDLERS (as opposed to interrupt handlers)
;
;; See /usr/lib/dsp/smsrc/hmlib.asm for the host message routines called 
;; by jsr_hm.
;;
;; HMS FORMAT
;; ----------
;;
;; *** Host message arguments are consumed in the OPPOSITE ORDER
;;     from how they were written. The HMS grows DOWN toward HMS_BOTMK. ***
;;
;;	         <HMS_TOPMK>		; stack boundary
;; x:X_HMSRP -->   <arg1>		; first argument written by host
;;		  ... 		   	; other arguments
;;                 <argN>	  +	; last argument written by host
;;	       <timeStampHi>	   	; absolute time stamp hi-order word
;;	       <timeStampLo>	  -	; time stamp low-order word, if exists
;;      R_I1 --> <messageCode>	   	; time-stamp type and opcode
;; x:X_HMSWP -->   <free>  (grows down) ; first unwritten stack element
;;   R_HMS -------/			; H_RMS also points to free loc
;;	         < ... >		; unused stack region
;; 	       <HMS_BOTMK>		; stack boundary
;;
;; DO NOT USE X_HMSRP to sense number of args (since buffer is modulo).
;; R_I1 points to last arg, and can be clobbered
;;

    remember 'consider setting R_I2 to X_HMSRP or <link>-1 for last arg ptr'

; hc_xhm - Execute Host Command

hc_xhm

; All host interrupt priorities (HCP,HRDF,HTDE) are level 1 so they mask 
; each other. Thus, during the execution of a host command, host messages
; are blocked. Since a pending HC can be overwritten, the "HC" bit of the CVR
; must be checked to make sure it's clear before writing a host command vector.
 
     remember 'xhm is calling save_alu instead of saving only what is used'

     begin_interrupt_handler	; set HF2 and clear HF3 if necessary

     jclr #B__ABORTING,y:Y_RUNSTAT,hc_xhm_noabort
	jsr abort_interrupt	; (jsrlib) only host commands allowed
	rti
hc_xhm_noabort

     if SYS_DEBUG
       jcc hc_xhm_hf2_was_off
	  move #DE_HF2_ON_2,X0 ; error code for "DSP Busy set twice"
	  jsr stderr
hc_xhm_hf2_was_off
     endif

	remember 'handlers.asm: DISABLED HOST-MESSAGE ARG CHECK'
;; 
;; The problem here is that we don't know if word sitting
;; in HRX is last arg of current HM or 1st arg of
;; next HM.  Need to lock out 1st arg until HM sets
;; HF2 or delay 1st arg on host side until HF2 (which
;; driver could miss!)  For the time being, the host-command
;; is not issued until TRDY is true.
;
;*; Make sure any host message arguments made it to safety
;*     jclr_hrdf hc_xhm_trdy
;*hc_xhm_trdy_ck
;*     movep x:$FFEB,y:(R_HMS)- ; write (circular) Host Message Queue
;*     jset_hrdf hc_xhm_trdy_ck
;*hc_xhm_trdy

	if SYS_DEBUG
     move R_HMS,x:X_HMSWP     	; Save R_HMS to check up on host (we're busy!)
	endif

     jsr save_alu	      	; overkill for timed case
     jsr save_temps	      	; overkill for timed case

;; ----------------- CHECK HMS HEALTH ---------------------
;; Note that overrun or underrun by more than one element will
;; clobber BOTH BOTMK and TOPMK.
;;
     if SYS_DEBUG

     move p:system_magic,A    ; Marker at start of external system memory
     move #SYS_MAGIC,X0	      ; What we expect
     cmp X0,A		      ; Make sure it is still there
     jeq hc_xhm_sys_ok
	  move #DE_PLE_SYSMM,X0 ; error code
	  jsr stderr	      ; send bad news
hc_xhm_sys_ok
     move y:YB_HMS,A	      ; Stack bottom (growth limit)
     move #HMS_BOTMK,X0	      ; Stack-bottom marker
     cmp X0,A		      ; Make sure it is still there
     jeq hc_xhm_stk_bot_ok	      
	  move #DE_HMSOVFL,X0 ; error code for HMS overflow
	  jsr stderr	      ; send bad news
hc_xhm_stk_bot_ok
     move y:YBTOP_HMS,A	      ; Stack top (growth origin)
     move #HMS_TOPMK,X0	      ; Stack-top marker
     cmp X0,A		      ; Make sure it is still there
     jeq hc_xhm_stk_top_ok	      
	  move #DE_HMSUFL,X0  ; error code for HMS underflow
	  jsr stderr	      ; send bad news
hc_xhm_stk_top_ok

     endif ; SYS_DEBUG

;; ----------------- BEGIN REAL WORK ---------------------
     lua (R_HMS)+,R_I1	      ; (pop) Point R_I1 to message code word
     move #$FF0000,X0	      ; Upper-byte mask
     move y:(R_I1),A	      ; Get message type to A1, R_I1 -> Opcode
     and X0,A #>HM_UNTIMED,X0 A,Y0 ; Mask out lower two bytes = opcode
     tfr Y0,B #>$FFFF,Y0      ; code word to B, lower two-byte mask to Y0
     and Y0,B		      ; Opcode now in B  
     move B1,y:(R_I1)	      ; Overwrite previous message code word
     cmp X0,A #>HM_TIMEDA,X0  ; See if message is untimed
     jeq xhmut		      ; If so, jump to untimed case
  if AP_MON
     move #DE_XHMILL,X0	      ; So sorry.
     move B,A		      ; Place unrecognized opcode in A for stderr
     jsr stderr		      ; Not a recognized host message type
     jmp xhm_done	      ; delete stack frame and clear "busy"
xhmut	  ; untimed host message execution. R_I1 points to opcode
     jsr jsr_hm		      ; jsr to address pointed to by R_I1
     jmp xhm_done	      ; on return, R_I1 points to first arg + 1
		; Note that the above instruction is an expensive nop
		; when SYS_DEBUG is false, but I can't make myself take it out.
  else ; AP_MON
     cmp X0,A #>HM_TIMEDR,X0  ; See if message is timed absolute
     jeq xhmta		      ; If so, jump to timed absolute case
     cmp X0,A		      ; See if message is timed relative
     jeq xhmtr		      ; If so, jump to timed relative case
     move #DE_XHMILL,X0	      ; So sorry.
     move B,A		      ; Place unrecognized opcode in A for stderr
     jsr stderr		      ; Not a recognized host message type
     jmp xhm_done	      ; delete stack frame and clear "busy"

xhmut	  ; untimed host message execution. R_I1 points to opcode
     jsr jsr_hm		      ; jsr to address pointed to by R_I1
     jmp xhm_done	      ; on return, R_I1 points to first arg + 1

xhmtr   ; Timed-relative version of xhm. R_I1 points to time stamp.
;	  Convert from relative to absolute time stamp and do 
;	  absolutely timed case.
	jsr unwritten_subr
        jmp xhm_done	      ; delete stack frame and clear "busy"

; xhmta	- timed-absolute host message
;;
;; DESCRIPTION
;;   Queue timed host message by copying HMS frame to TMQ.
;;	  
;;   On entry, R_I1 points to the 16-bit message opcode with the 
;;   time-stamp-type byte having been zeroed.
;;
;;   Any late or out-of-order messages are appended to the untimed
;;   message queue (UTMQ).  All messages in the UTMQ are executed 
;;   at the end of the current tick computation, before the TMQ.
;;
;;   The TMQ is assumed locked.	 The TMQ is emptied by service_tmq (called by
;;   the end_orcl macro (beginend.asm)) which runs at user level (priority 0).
;;   Since we are processing a host command (priority 1), the user can't run. 
;;
;;   Note that the TMQ always turns itself off when it detects the
;;   empty condition (to save empty detection time).
;;   Therefore, we must always turn it on.
;;
;;----------------------------------------------------------------
;;
;; TMQ FORMAT (see hmlib.asm)
;;
;;----------------------------------------------------------------
;;
xhmta
     move x:X_TMQWP,R_O		; Pointer to first unused element of TMQ (head)
     move #(NB_TMQ-1),M_O	; Make it a modulo pointer
  if SYS_DEBUG
     move y:(R_O),A		; should be first word of null message
     tst A  			; which is a zero time stamp
     jeq xhtma_head_ok	
	  move #DE_TMQHMM,X0	; "TMQ head marker missing" (obsolete name)
	  jsr stderr  
xhtma_head_ok  
  endif ; SYS_DEBUG
     clr B y:(R_I1)+,N_I1	; Save opcode for insertion in TMQ
     move y:(R_I1)+,B0		; Low word of time stamp
     move y:(R_I1),B1		; High word of time stamp
     tst B			; zero time stamp means "timed-zero message"
     jne xhmta_nonzero_ts
 if 1
     remember 'Bypassing timed-zero message handling code. Enqueing!'
     clr A l:L_TICK,B		; Current time
     tst B #>1,A0		; Make sure it is not zero
     jne xhmta_ctnz
       add A,B			;   (zero time stamp in TMQ will turn it off)
xhmta_ctnz
     move B1,y:(R_I1)-		; High word of time stamp
     move B0,y:(R_I1)+		; Low word of time stamp
     jmp xhmta_nonzero_ts
 endif
;; *** FIXME: Make this work with the PlayNote example *** !!!
;; Here we have a "timed-zero message" to process.
;; Timed messages with a time-stamp of 0 are special.
;; They are like "untimed messages" except that they are constrained to
;; execute on "tick" boundaries.  The way we handle that here is to 
;; set a "timed-zero message pending" flag and return to the orchestra loop.
;; At the top of the next tick, the flag will be noticed and control will pass
;; back here.  Then we'll execute the message.
;;
;; CAVEAT: We return to the orchestra loop with interrupts turned off and
;; HF2 set. This means the orchestra loop cannot block waiting for something 
;; from the host, because nothing can get in.  In particular, we cannot
;; block on TMQ empty.  Any place that might block has to check to make sure
;; a timed-zero message is not pending, i.e. #B__TZM_PENDING,x:X_DMASTAT == 0.
;; Blocking for DMA IO is ok AS LONG AS the driver ignores the state of HF2.
;; Note that the only time we would block on TMQ empty is when processing
;; timed messages which is on a tick boundary.
;;
	bset #B__TZM_PENDING,x:X_DMASTAT ; Post message as pending current tick
	move N_I1,y:(R_I1)	; Make untimed msg by writing opcode over tshi
;;        lua (R_HMS)+,R_HMS      ; (pop) eliminate evidence of time stamp
;;        move R_HMS,x:X_HMSWP    ; Used to check no. args used (HMSWP += 2)
	move R_I1,x:X_XHM_R_I1  ; Need HMS pointer when we return
	jsr restore_alu		; Restore the world the way it was
	jsr restore_temps	;   but LEAVE HOST INTERRUPTS TURNED OFF.
	rts			; Return to orchestra loop to finish tick.
				; Use rts to stay at interrupt level, NOT RTI!
xhmta_return_for_tzm		; jsr here from top of tick loop on TZM_PENDING
	jsr save_alu		; Save new state
	jsr save_temps		; 
	clr A x:X_XHM_R_I1,R_I1 ; Restore HMS pointer (points to opcode)
	movec A1,ssl		; clear return ipl (rest of sr not looked at)
	bclr #B__TZM_PENDING,x:X_DMASTAT ; done
	jmp xhmut		; This works if EACH TZM IS FLUSHED
				; There CANNOT be another message on the HMS!
				; If there is, you'll see an HM_ARGERR
xhmta_nonzero_ts
     bset #B__TMQ_ACTIVE,y:Y_RUNSTAT ; Tell world TMQ has stuff in it
     move l:L_TICK,A		; Current time
     cmp A,B			; get sign of B-A = delay time
     jge xhtma_ontime		; negative means underrun: time stamp in past
	if TMQU_MSGS
	     move #DE_TMQU,X0	; TMQ underrun error code (current time in A)
	     move y:Y_TICK,A	; Current time at underrun (low-order word)
	     jsr dspmsg		; Complain, then fall through to UTMQ
				; ASSUME stderr DOES NOT ALTER R_I1 OR N_I1!!!
		;***TEST***
		move l:L_TICK,A		; Current time
		tst A
		jsne abort
		;***ENDTEST***
	endif
        move l:L_TICK,B		; Change time stamp to current time
xhtma_ontime
     move x:X_HMSRP,A		; Pointer to first argument on HMS stack 
     move R_I1,Y0		; Pointer to high word of time stamp
     sub Y0,A	(R_I1)+		; Number of arguments = HMS to TMQ copy length
     move B0,y:(R_O)+		; Transfer low word of time stamp to TMQ
     move B1,y:(R_O)+		; Transfer high word of time stamp to TMQ
     move R_O,N_O		; Save R_O so we can come back & install link
     move (R_O)+		; skip link (need pre-increment mode)
     move N_I1,y:(R_O)+		; install opcode
;;
;;  *** HERE IS WHERE THE HMS ARGUMENTS GET COPIED OVER TO THE TMQ ***
;;
     tst A			; If no arguments, A is zero
     jle xhmta0			;   and a "DO 0" means "DO 65K"
     do A1,xhmta0		; loop over stack frame
	  move y:(R_I1)+,Y0	
	  move Y0,y:(R_O)+	; Transfer HMS frame to TMS (opcode and args)
xhmta0	
     move N_O,R_I2		; Point to next-TM link word
     clr A #TMQ_MEND,X0	
     move R_O,y:(R_I2)		; Make link word of installed message valid
     move X0,y:(R_O)+		; Install TMQ message-end marker
     move R_O,x:X_TMQWP		; Update TMQ write-pointer
     move A,y:(R_O)+		; Zero time stamp (lo) [null message]
     move A,y:(R_O)+		; Zero time stamp (hi) [null message]
     move A,y:(R_O)+		; Zero link word [null message]
     move #TMQ_HEAD,X0		; TMQ head marker
     move X0,y:(R_O)		; Install TMQ head marker
     move #-1,M_O		; Fall through to xhm_done case

  endif ; AP_MON

xhm_done	 
;
;	Bit B__HOST_WRITE is set when doing a DMA into the DSP.
;	(See hmlib:(host_w0))
;	The error checking below would see that R_HMS is
;	wrong and, worse, overwrite R_HMS to "pop" the current host
;	message frame off of the host-message stack. Therefore,
;	if entering this mode (i.e., we are wrapping up the host message
;	hm_host_w), we skip the checking and resetting of R_HMS.
;
	jset #B__HOST_WRITE,x:X_DMASTAT,xhm_host_w

	if SYS_DEBUG
	  move x:X_HMSWP,X0	; Get write pointer = saved R_HMS on xhm entry
	  move R_HMS,A		; Current write register
	  sub X0,A		; Make sure they're still equal
	  jeq xhm8		; If not (took host rcv ntrpt or lost R_HMS)
	    move #DE_HMSBUSY,X0	;   complain, sending
	    jsr stderr		;   R_HMS - x:X_HMSWP to host
	endif ; SYS_DEBUG
xhm8    move x:X_HMSRP,R_HMS	; Read pointer. Points to first arg.
	if SYS_DEBUG
	  move R_HMS,x:X_HMSWP	; R_HMS = write pointer
	endif
xhm_host_w
	remember 'still checking for arg error even when SYS_DEBUG is false'
	move x:X_HMSRP,Y0		; Read pointer. Points to first arg.
	lua (R_I1)-,R_I1		; make R_I1 point to first arg also
	move R_I1,A
	sub Y0,A			; A = #ArgsExpected - #ArgsFound
	jeq xhm_noargerr
		move #DE_HMARGERR,X0	; inform host
		jsr stderr
xhm_noargerr

	jclr #B__HM_DONE_INT,y:Y_RUNSTAT,hc_noint ; see if "done" msg required
	   clr A #DM_HM_DONE,X0		; "host message done" message
	   jsr dspmsg
hc_noint

	if !AP_MON
;;	Set HF3 ("TMQ Full") if next message could overflow the TMQ
;;	TMQ blocking can be avoided
;;	by verifying (via the hm_tmq_room host message) that there will be
;;	at least NB_HMS-1 free words in the TMQ after writing the next timed
;;	message.
;;	
	bset #B__TMQ_FULL,y:Y_RUNSTAT
	set_hf3			; guaranteed to be fully BETWEEN host messages
	jsr measure_tmq_margin	; result in A is positive for success
	jle xhm_tmq_full	; 	otherwise, TMQ is nearly full
		bclr #B__TMQ_FULL,y:Y_RUNSTAT
		clear_hf3
xhm_tmq_full
	endif ; !AP_MON

	jsr restore_alu
	jsr restore_temps

	end_interrupt_handler

	rti

;
; ********************* HANDLERS CALLED BY MKMON ONLY ************************
;
	if !AP_MON
;; ================================================================
; SSI_RCV - ssi receive data interrupt handler
;
ssi_rcv
	jclr #B__SSI_RD_ENABLE,x:X_DMASTAT,turn_off_ssi_rcv ; ssi not expected

	move R_I1,x:X_SSI_SAVED_R_I1  ; save R_I1
	move M_I1,x:X_SSI_SAVED_M_I1  ; save M_I1

	move x:X_SSIWP,R_I1     ; SSI write ptr (read-data, filling)
	move #NB_DMA_W-1,M_I1   ; Make it modulo like DMA write buffers
	nop
	movep x:$ffef,y:(R_I1)+ ; deposit input sample to input buffer

ssircv_done
	move R_I1,x:X_SSIWP      	; update ssi write pointer
	move x:X_SSI_SAVED_R_I1,R_I1  	; restore user usage of R_I1
	move x:X_SSI_SAVED_M_I1,M_I1  	; restore user usage of M_I1
	rti

turn_off_ssi_rcv
        bclr #13,x:<<$FFED	      ; clear RE  in SSI control register B
        bclr #15,x:<<$FFED	      ; clear RIE in SSI control register B
	rti

;; ================================================================
ssi_rcv_exc
	bset #B__SSI_RDO,y:Y_RUNSTAT  ; indicate read-data overrun
	jmp ssi_rcv     ; try normal service for want of better idea
     rti

;; ================================================================
; ssi_xmt.asm - ssi transmit data interrupt handler
;;
;; DESCRIPTION
;;   The ssi_xmt handler simply outputs the current word to the SSI port.
;;   No checking of buffer pointers is done.  write_data_wait is responsible
;;   for blocking until the current emptying buffer is traversed by the SSI,
;;   and start_ssi_write_data initializes the buffer pointer x:X_SSIRP.
;

ssi_prv		dc 0	; saved output word

ssi_xmt

	jclr #B__SSI_WD_ENABLE,x:X_DMASTAT,turn_off_ssi_xmt ; ssi not expected

	move R_I1,x:X_SSI_SAVED_R_I1  ; save R_I1
	move M_I1,x:X_SSI_SAVED_M_I1  ; save M_I1
	move A0,x:X_SSI_SAVED_A0

	move x:X_SSIRP,R_I1      ; SSI read ptr (write-data, emptying)
	move #NB_DMA_W-1,M_I1    ; Make it modulo like DMA write buffers

	; the following block of code should probably go away along
	; with the labels ssixmt_full_srate, ssixmt_done, ssi_prv, X_SSI_PHASE
	jclr #B__HALF_SRATE,y:Y_RUNSTAT,ssixmt_full_srate
	bchg #0,x:X_SSI_PHASE	; toggle phase bit
	jcc ssixmt_full_srate	; if formerly clear, do 2nd phase
	movep p:ssi_prv,x:$ffef ; repeat word to ssi xmt (TX) [MSB's taken]
	jmp ssixmt_done		;   next time

ssixmt_full_srate

	move y:(R_I1)+,A0    	; next sample to send out

 ; !!! TEST HACK To insert some bits into serial output stream !!!
 ; remove this or no audio data will go out of ssi!!!
 ;***	move  #$a5a5a5,a0
 ; !!! end of test hack 


	if WRITE_DATA_16_BITS	; then left-shift each sample one byte
;;
;;	   This crock is needed because the host interface in 16-bit mode
;;	   takes the least-significant bits while the SSI in 16-bit mode
;;	   takes the most-significant bits 
;;
	   move X0,x:X_SSI_SAVED_X0
	   move X1,x:X_SSI_SAVED_X1
	   move A2,x:X_SSI_SAVED_A2
	   move A1,x:X_SSI_SAVED_A1

  	   move A0,X1		; next sample to send out
	   move #>@pow(2,-16),X0 ; one-byte left shift = two-byte right-shift
	   mpy X0,X1,A 		; not mpyr which rounds A1, clearing A0, not A0

	   move x:X_SSI_SAVED_X0,X0
	   move x:X_SSI_SAVED_X1,X1
	   move x:X_SSI_SAVED_A2,A2
	   move x:X_SSI_SAVED_A1,A1
	endif ; WRITE_DATA_16_BITS 

	move A0,p:ssi_prv	; save for next phase in srate/2 mode
	movep A0,x:$ffef     	; next word to ssi xmt (TX) [MSB's taken]

ssixmt_done
	move R_I1,x:X_SSIRP      	; update ssi read pointer
	move x:X_SSI_SAVED_A0,A0
	move x:X_SSI_SAVED_R_I1,R_I1  	; restore user usage of R_I1
	move x:X_SSI_SAVED_M_I1,M_I1  	; restore user usage of M_I1

	rti

turn_off_ssi_xmt
        bclr #12,x:<<$FFED	      ; clear TE  in SSI control register B
        bclr #14,x:<<$FFED	      ; clear TIE in SSI control register B
	rti
;; ================================================================

; ssi_xmt_exc.asm - ssi transmit underrun data interrupt handler
;
ssi_xmt_exc
	bset #B__SSI_WDU,y:Y_RUNSTAT  ; indicate write-data underrun
	jmp ssi_xmt     ; try normal service for want of better idea
;; ================================================================
; sci_timer - increment timer count.  
;	See also enable_sci_timer and disable_sci_timer in jsrlib.asm
;
sci_timer	move R_IO,p:st_saved_R_IO
		move x:X_SCI_COUNT,R_IO
		nop
		lua (R_IO)+,R_IO
		nop
		move R_IO,x:X_SCI_COUNT
		move p:st_saved_R_IO,R_IO
		do #4,st_loop
		nop		; fill out to 64 cycles handler time
st_loop
		rti

; timer state variables
st_saved_R_IO	dc 0		; reg save

	endif ; !AP_MON

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================

; DMA write termination

hc_host_w_done
	jsr save_alu
	jsr save_temps
;*	jsr flush_hrx
;*   This race is resolved by waiting for TRDY before issuing HC
	jsr host_w_done0
	jsr restore_alu
	jsr restore_temps
	rti

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================

; DMA read termination

hc_host_r_done
        begin_interrupt_handler
	jsr save_alu
	jsr save_temps
	jsr host_r_done0
	jsr restore_alu
	jsr restore_temps
        end_interrupt_handler
	rti

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================
hc_kernel_ack  ; kernel acknowledge.
	  ; used to tell host when previous host-command is finished.
hcka_buzz jclr #1,x:$FFE9,hcka_buzz	; wait for HTDE to come true	
	move #DM_KERNEL_ACK,X0
	move X0,x:$FFEB			; write to host interface
	rti

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================

; User host command handlers

hc_user
     jsr unwritten_subr
     DEBUG_HALT
     rti

;; ================================================================
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;;
;;		 SUBROUTINES CALLED AT INTERRUPT LEVEL
;;
;; ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;; ================================================================

; save_alu - Save A, B, X, and Y registers in data ALU
save_alu
     bset #B__ALU_SAVED,y:Y_RUNSTAT 	; mark ALU as saved
     jcc sa_ok				; carry set => already saved
	  DEBUG_HALT			; break
sa_ok
     move A2,x:X_SAVED_A2
     move A1,x:X_SAVED_A1
     move A0,x:X_SAVED_A0
     move B2,x:X_SAVED_B2
     move B1,x:X_SAVED_B1
     move B0,x:X_SAVED_B0
     move X1,x:X_SAVED_X1
     move X0,x:X_SAVED_X0
     move Y1,x:X_SAVED_Y1
     move Y0,x:X_SAVED_Y0
     rts
; =============================================================================
; restore_alu - Restore A, B, X, and Y registers in data ALU
restore_alu
     bclr #B__ALU_SAVED,y:Y_RUNSTAT
     jcs ra_ok
	  DEBUG_HALT
ra_ok
     move x:X_SAVED_A2,A2
     move x:X_SAVED_A1,A1
     move x:X_SAVED_A0,A0
     move x:X_SAVED_B2,B2
     move x:X_SAVED_B1,B1
     move x:X_SAVED_B0,B0
     move x:X_SAVED_X1,X1
     move x:X_SAVED_X0,X0
     move x:X_SAVED_Y1,Y1
     move x:X_SAVED_Y0,Y0
     rts
; =============================================================================
; save_temps - Save temporary address registers
save_temps
     bset #B__TEMPS_SAVED,y:Y_RUNSTAT	; mark temps as saved
     jcc st_ok				; carry set => already saved
	  DEBUG_HALT			; break
st_ok
     move R_I1,x:X_SAVED_R_I1 
     move R_I2,x:X_SAVED_R_I2 
     move R_O,x:X_SAVED_R_O   
     move N_I1,x:X_SAVED_N_I1 
     move N_I2,x:X_SAVED_N_I2 
     move N_O,x:X_SAVED_N_O   
     move M_I1,x:X_SAVED_M_I1 
     move M_I2,x:X_SAVED_M_I2 
     move M_O,x:X_SAVED_M_O   
     rts
; =============================================================================
; restore_temps - Restore temporary address registers
restore_temps		      ; Restore temporary address registers
     bclr #B__TEMPS_SAVED,y:Y_RUNSTAT
     jcs rt_ok
	  DEBUG_HALT
rt_ok
     move x:X_SAVED_R_I1,R_I1
     move x:X_SAVED_R_I2,R_I2
     move x:X_SAVED_R_O,R_O
     move x:X_SAVED_N_I1,N_I1
     move x:X_SAVED_N_I2,N_I2
     move x:X_SAVED_N_O,N_O
     move x:X_SAVED_M_I1,M_I1
     move x:X_SAVED_M_I2,M_I2
     move x:X_SAVED_M_O,M_O
     rts
;; ================================================================
flush_hrx	; Process any pending incoming data from HRX
		; Since host commands have higher priority than
		; HRDF when both are pending, there can be up to
		; two words waiting (HRX and RX) when the host command 
		; vector is taken.
;* This race is now resolved by waiting for TRDY before issuing HC from host
		if 1
			DEBUG_HALT
		else
		 	jclr #0,x:$ffe9,no_hrx   ; Flush HRX,TX before HC
			movep x:$FFEB,y:(R_HMS)- ; Simulate HRDF interrupt
			jmp flush_hrx		 ; HC got in before two args?
no_hrx			rts
		endif
; =============================================================================
; |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;
;    The remaining routines assume save_alu and save_temps have been called.
;
; |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
; =============================================================================
; jsr_hm  - host message (or timed message) jump to subroutine
;;
;; ARGUMENTS
;;   R_I1      - contains jsr address
;;
;; SIDE EFFECTS
;;   R_I1      - points to first argument 
;; 
;; This routine is called by the host-command handler xhm or xhmf, or by the
;; timed message dispatcher service_tmq.
;; It makes sure the jsr address is a legal dispatch address in hmlib.asm,
;; advances R_I1, and jsr's to the jsr address.
;; 
;; If untimed, (R_I1)+ points to the last arg written by the host on the HMS.
;; x:X_HMSRP points to first arg written by the host onto the HMS.
;; If timed, R_I1 is an index into the TMQ and there is no last arg pointer.
;; In this case, (R_I1)+ points to the two-word time stamp, followed by args.
;; The convention is that if the number of args is not fixed and known, the
;; first arg is the number of args.  Otherwise, the number of arguments
;; in a TMQ frame can be deduced from the link pointer (see TMQ format in 
;; handlers.asm) or by searching for the message terminator TMQ_MEND.
;; In the former case, watch out for buffer wrap-around (the TMQ is modulo
;; storage).
;;
jsr_hm				   ; host-command jsr dispatch
   if SYS_DEBUG||!FAST_AND_LOOSE
     move y:(R_I1),A		   ; JSR address to A1 (R_I1 incr. below)
     move #>hm_first,X0	   	   ; address of first system library dispatch
     cmp x0,A  #>hm_last,X0	   ; see if address is too small
     jlt ill_hm
     cmp x0,A			   ; see if address is too large
     jge ill_hm
     if (hm_first%2)==0	   	   ; if all dispatches are even addresses
	  jset #0,y:(R_I1)+,ill_hm ; make sure jsr address is even
     else			   ; otherwise
	  jclr #0,y:(R_I1)+,ill_hm ; make sure it's odd
     endif     
   else
     move y:(R_I1)+,A		   ; JSR address to A1
   endif
     move A1,p:>(*+3)	 ; Poke it into the following JSR instruction
     jsr >0		 ; This calls the routine (or RESET if we missed ha ha)
     rts		 ; Hopefully we get back here
ill_hm
     move #DE_ILLHM,X0	 ; error code plus illegal opcode in A (dspmsgs.asm)
     jsr stderr
     rts
