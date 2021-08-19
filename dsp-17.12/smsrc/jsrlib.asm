; jsrlib.asm - system subroutines - included by allocsys.asm
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; Modification history:
;; 03/21/90/jos - deleted extra jsset #B__TZM_PENDING,.. at service_write_data1
;; 03/21/90/jos - added "jsset #B__TZM_PENDING,.." at st_buz_lwm
;;		  A TZM while blocking on TMQ empty gave deadlock.
;; 03/21/90/jos - added "jsset #B__ABORTING,.." at st_buz_lwm
;; 03/21/90/jos - Also broke block at st_buz_tm on aborting and cleared TZM.

system_magic dc SYS_MAGIC	; sentinel word checked to detect clobberage

;
; **************** ROUTINES CALLED BY BOTH APMON AND MKMON *******************
;

; ============================================================================
; unwritten_subr - force break on execution of nonexistent routine
unwritten_subr	    ; force break on execution of nonexistent routine
		    move #DE_ILLSUB,X0	; illegal subroutine
		    movec ssh,A1	; location of error detection
		    jsr stderr		; report
		    rts

; ================================================================
; dspmsg.asm - enqueue a DSP message for the host or read-mapped user
;
;; ARGUMENTS:
;;   X0 = message opcode, left-justified
;;   A1 = message word
;;
;; SIDE EFFECTS
;;   X0 is clobbered
;;
;; DESCRIPTION
;;   Place DSP message word into the DSP Message Queue (DMQ).
;;
;;   Note that Host Transmit Interrupt Enable (HTIE) is always
;;   set (to enable host-transmit-data-empty interrupts) because it's 
;;   faster than testing status and, having been called, we're 
;;   guaranteed to have something to send. The interrupt enable
;;   is cleared by the host-xmt interrupt handler when the DMQ
;;   is empty.
;;
;;   DMQ full is detected by read-pointer = write-pointer.
;;   DMQ empty is detected by read-pointer+1 = write-pointer.
;;
;;   When a DMA transfer is in progress from the DSP to the host,
;;   the HTIE enable is skipped since it is already enabled for the
;;   DMA transfer.  During DMA, the host_xmt vector is overridden,
;;   and dsp messages will resume as soon as the vector is restored.
;;
;;   History
;;      2/26/88 - simulated successfully the boot-up message
;;      3/01/88 - DMQWP was not getting updated properly after write
;;      3/10/88 - deadlocked detected: blocking for host when host locked.
;;      4/10/88 - added register save/restore
;;
     remember 'study effect of dsp-to-host dma on HTIE'
;;
dspmsg
	mask_host	      ; Can't let an HTDE interrupt happen now
dspmsg1
	move X1,x:X_DSPMSG_X1
	move B2,x:X_DSPMSG_B2
	move B1,x:X_DSPMSG_B1
	move A1,x:X_DSPMSG_A1
	move B0,x:X_DSPMSG_B0
	move R_O,x:X_DSPMSG_R_O
	move M_O,x:X_DSPMSG_M_O

 	move #>$FFFF,X1     ; 16-bit mask
	and X1,A	    ; ensure only low 16 bits are used (A1 restored)
	or X0,A		    ; install opcode
 	move x:X_DMQRP,X0   ; DMQ read pointer
	move x:X_DMQWP,B    ; DMQ write pointer
	cmp X0,B  B1,R_O    ; Check for DMQ overflow, DMQWP to R_O
	move #NB_DMQ-1,M_O  ; modulo
	jne dspmsg2
;
; *** DMQ is full *** Block until host reads a message, unless aborting
;
	        bset #B__DMQ_FULL,y:Y_RUNSTAT 		; enter full state
	        jset #B__DMQ_LOSE,y:Y_RUNSTAT,dspmsg2	; ok to lose messages
	        jset #B__SIM,y:Y_RUNSTAT,dspmsg2	; simulator
		jsset #B__ABORTING,y:Y_RUNSTAT,abort_now ; abrt => can't block
		lua (R_O)+,R_O		; read ptr points one behind current
dm_buzz		jclr #1,x:$ffe9,dm_buzz	; wait for HTDE in HSR
		move R_O,x:X_DMQRP	; Update incremented DMQRP
		movep y:(R_O)-,x:$FFEB  ; Write HTX, point to new input cell
		bclr #B__DMQ_FULL,y:Y_RUNSTAT ; exit full state
dspmsg2
;
; *** Insert message into DMQ ***
;
	  move A1,y:(R_O)+    ; install word ("move A" will not work)
	  move R_O,x:X_DMQWP  ; update DMQ write pointer

;*	  unmask_host	        ; restore interrupt priority mask (pop SR)
	  ; Since dspmsgs could be called at interrupt level (e.g. by hmlib)
	  ; we can't invoke unmask_host (which doesn't keep a nesting count).
	  ; Therefore, we let the rti below take care of restoring sr.

	  jset #B__DM_OFF,x:X_DMASTAT,dmpathoff
;
;	The DM_OFF bit of the run status register is set whenever
;	DSP messages have been turned off.
;	Operation in this case is to inhibit the turning on of HTIE.
;
	  bset #1,x:$FFE8     ; Set Host Transmit Intrpt Enable (HTIE) in HCR
;	  <zap>		      ; HTDE interrupt happens here.
dmpathoff
	move x:X_DSPMSG_X1,X1
	move x:X_DSPMSG_B2,B2
	move x:X_DSPMSG_B1,B1
	move x:X_DSPMSG_B0,B0
	move x:X_DSPMSG_A1,A1
	move x:X_DSPMSG_R_O,R_O
	move x:X_DSPMSG_M_O,M_O

        rti
	nop ; this is here just to shield breakpoints on stderr (prefetch)

; =============================================================================

	if AP_MON
;
; ********************** ROUTINES CALLED BY APMON ONLY ************************
;
; ================================================================
; main_done - entered via "jmp" when an array proc main program ends
	; Inform host that AP program finished and return to idle loop.
main_done1
	bclr #4,x:$FFE8		; Clear HF3 = "AP Busy" flag in AP mode
				;   Allows polling of busy status
	clr A #DM_MAIN_DONE,X0  ; "main program done" message
	jsr dspmsg		;   Provides interrupt for host on main done.
	clear_sp		; set stack pointer to base value (misc.asm)
	move #0,sr		; clear status register
	jmp idle_1		; jump to idle loop without DM_IDLE message

	endif ; AP_MON

; =============================================================================
; stderr - send a standard dsp error message = error code + status and info
;
; ARGUMENTS
;    X0		= left-justified 8-bit error code (dspmsgs.asm)
;    A1		= additional info (low-order 2 bytes sent)
;
; EXAMPLE CALL
;    move #DE_ERRORCODE,X0
;    move bad_result,A #DE_ERRORCODE,X0
;    jsr stderr
;
stderr	  
	  jsr dspmsg		; deliver the main message
	  jsr abort
;
; ********************** ROUTINES CALLED BY MKMON ONLY ************************
;
	if !AP_MON

; ================================================================
send_time ; send out the current time
	  move #DM_TIME2,X1		; Time 2 message code
	  tfr X1,B  #DM_TIME1,X1	; Time 1 message code
	  move X1,B0 
	  move #DM_TIME0,X0	        ; Time 0 message code

	  if SYS_DEBUG

	  jmp send_time_1

send_time_with_error
	  move #DE_TIME2,X1		; Time 2 message code
	  tfr X1,B  #DE_TIME1,X1	; Time 1 message code
	  move X1,B0 
	  move #DE_TIME0,X0	        ; Time 0 message code
	  ; Fall through to ...

	  endif ; SYS_DEBUG

send_time_1
	  move l:L_TICK,Y		; Current tick time
	  jsr send_long
	rts

; ================================================================
send_long ; ship out 48-bit datum in Y. X0,B0,B1 contain successive opcodes.

	  move Y0,A			; low-order word
	  jsr dspmsg			; deliver lower 16 bits (X0 ready)

	  move #>@pow(2,-16),X1		; 16-bit right-shift multiplier
	  mpy X1,Y0,A	#>$FF,X1	; low-order 8 bits of middle
	  and X1,A			; bare low byte
	  tfr Y1,A A1,Y0		; upper half to A, low byte to Y0
	  and X1,A			; bare low byte
	  move A1,X0			; low byte back around
	  move #>@pow(2,-16),X1		; Byte-shift-left multiplier
	  mpy X1,X0,A			; upper byte of middle status chunk
	  move A0,A1			; add 24 to -16 in effect
	  or Y0,A	B0,X0		; or in low byte of middle
	  jsr dspmsg			; deliver the message

	  move #>@pow(2,-8),X1		; shift right one byte
	  mpy Y1,X1,A	B1,X0		; upper half
	  jsr dspmsg			; deliver the message

	rts

; ================================================================
start_host_write_data	      ; set up dma sound-out to host
	  bset #B__HOST_WD_ENABLE,x:X_DMASTAT ; enable host WD service
	  rts
; ================================================================
stop_host_write_data	      ; cease dma sound-out to host
	  bclr #B__HOST_WD_ENABLE,x:X_DMASTAT ; disable host WD service
	  rts
; ================================================================
stop_ssi_write_data	 ; cease sound output to ssi port.
	  bclr #B__SSI_WD_ENABLE,x:X_DMASTAT  ; disable ssi out service
	  bclr #B__SSI_WD_RUNNING,x:X_DMASTAT ; indicate need for ptr reset
 	  bclr #12,x:<<M_CRB	      ; clear TE  in SSI control register B
	  bclr #14,x:<<M_CRB	      ; clear TIE in SSI control register B
	  rts
; ================================================================
;; setup_ssi_sound - turn on SSI serial port for 16-bit sound input or output
;;
;; The SSI will be enabled (except for pc3 (sc0) which enables static RAM).
;;
cra_init equ	$4100           ;
                                ; This initial setting of CRA is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
                                ;
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments                                         |
;------------------------------------------------------------------------------
;|  PSR          0           No pre-scaler [divide-by-8] on TX/RX clock.      |
;|  WL1          1            Select a 16-bit word-length.                    |
;|  WL0          0           /                                                |
;|  DC4          0               Frame-rate divider control.                  |
;|  DC3          0              /                                             |
;|  DC2          0             /                                              |
;|  DC1          0            /                                               |
;|  DC0          1           /                                                |
;|  PM7          0                 No pre-scaler on TX/RX clock               |
;|  PM6          0                 [Just going as fast as we can].            |
;|  PM5          0                /                                           |
;|  PM4          0               /                                            |
;|  PM3          0              /                                             |
;|  PM2          0             /                                              |
;|  PM1          0            /                                               |
;|  PM0          0           /                                                |
;------------------------------------------------------------------------------
;
crb_init equ    $0a00 		; This setting of CRB is determined
	                        ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments
;------------------------------------------------------------------------------
;|  RIE          0           No interrupt on Receiver-full.                   |
;|  TIE          0           No interrupt on Transmitter empty.               |
;|  RE           0           Receiver is disabled.                            |
;|  TE           0           Transmitter is not enabled.                      |
;|  MOD          1           Use Network-mode communication [not Normal-mode].|
;|  GCK          0           Use continuous clock [not gated clock].          |
;|  SYN          1           Transmission and reception are synchronous.      |
;|  FSL          0           Use long frame-sync.                             |
;|   *           0           Unused ......                                    |
;|   *           0           Unused ......                                    |
;|  SCKD         0           External clock-source.                           |
;|  SCD2         0           External Frame-sync.                             |
;|  SCD1         0             I'm using neither SC0 nor SC1 for data-        |
;|  SCD0         0             transmission/reception, so leave 'em alone.    |
;|  OF1          0            /                                               |
;|  OF0          0           /                                                |
;------------------------------------------------------------------------------
;
setup_ssi_sound
	bset  #B__REALTIME,y:Y_RUNSTAT ; SSI is a real time device

; Port C Control Register - selects either gen purp I/O or SSI function
pcc_init equ    $1e0 		; This setting of the Port C Control Reg is determined
	                        ; as follows.  All pins are general purpose IO pins
                                ; except the STD, SRD, SCK, and SC2 signals of the SSI.
;------------------------------------------------------------------------------
;|      bclr	#0,x:M_PCC	; PCM 501 select (port i/o)
;|      bclr	#1,x:M_PCC	; SCI TXD (unused)
;|	bclr	#2,x:M_PCC	; SCI SCLK (unused)
;|	bclr	#3,x:M_PCC	; SC0 must be set to zero elsewhere to enable ext RAM
;|
;|	bclr	#4,x:M_PCC	; SSI SC1 (unused)
;|	bset	#5,x:M_PCC	; SSI SC2 (Frame Synch; Left/~Right Clk)
;|	bset	#6,x:M_PCC	; SSI SCK
;|	bset	#7,x:M_PCC	; SSI SRD
;|
;|	bset	#8,x:M_PCC	; SSI STD

	bset	#0,x:M_PCDDR	; pc0 is an output with value
        bset	#0,x:M_PCD	; 1 to set SELECT (for PCM501 D/A mode)

	movep	#cra_init,x:M_CRA	; Set up serial port
	movep	#crb_init,x:M_CRB	; in network mode
	movep   #pcc_init,x:M_PCC	  ; /

     rts

;| --- SSI CRB bits ---
;|   bclr  #0,x:M_CRB	      ; sc0=0 (static RAM en) (nop since x:$ffe1[0]=0)
;|   bclr  #1,x:M_CRB	      ; sc1=0 to clear SELECT (A/D = default mode)
;|   bset  #2,x:M_CRB	      ; make sc0 an output (nop since x:$ffe1[0] = 0).
;|   bclr  #3,x:M_CRB	      ; make sc1 an input
;|   bclr  #4,x:M_CRB	      ; make sc2 an input (-wordClock) [frameSync]
;|   bclr  #5,x:M_CRB	      ; make sck an input (clock)
;|   bxxx  #6,x:M_CRB	      ; not used
;|   bxxx  #7,x:M_CRB	      ; not used

;|   bclr  #8,x:M_CRB         ; frame sync length = word
;|   bset  #9,x:M_CRB	      ; synchronous mode
;|   bclr  #10,x:M_CRB	      ; ungated mode
;|   bclr  #11,x:M_CRB	      ; normal mode (as opposed to network mode)

;|   bclr  #12,x:M_CRB	      ; INHIBIT TE  = Transmit Enable
;|   bclr  #13,x:M_CRB	      ; INHIBIT RE  = Receive Enable
;|   bclr  #14,x:M_CRB	      ; INHIBIT TIE = Transmit Interrupt Enable
;|   bclr  #15,x:M_CRB	      ; INHIBIT RIE = Receive Interrupt Enable
;
; ================================================================
; start_ssi_write_data - set up sound output to ssi port.
;;
;; DESCRIPTION
;;   Start write-data output to the synchronous serial interface (SSI).
;;   Write-data is sound output from the DSP.  SSI output is assumed to
;;   go to something like the MetaResearch "Digital Ears".
;;   With little or no modification, this handler can be used to connect
;;   any serial audio device using the 44.1KHz, serial format 
;;   used internally by Sony in their CD players, etc.
;;
;;   After calling this routine, the SSI port is set up but not running.
;;   It will be turned on by write_data_buffer_out when a buffer is actually
;;   ready to go. 
;;
;;   start_ssi_write_data and start_ssi_read_data may be called in any order.
;;   Thus, the ssi registers must be programmed using individual bit 
;;   manipulations. (Or we could AND and OR with the right things.)
;;
start_ssi_write_data	      	; set up sound output to ssi port.
    bset  #B__SSI_WD_ENABLE,x:X_DMASTAT ; enable ssi out in DMA status
    move #>YB_DMA_W,X0	 	; Write-data buffer ring base address
    move X0,x:X_SSIRP	 	; read ptr (really set in write_data_buffer_out)
    remember 'consider zeroing write data buffers when starting WD'
    jsr setup_ssi_sound	; start up SSI in 16-bit sound mode
    rts

; ================================================================
; start_ssi_read_data - set up sound input from ssi serial port.
;;
;; DESCRIPTION
;;   Start read-data input from the synchronous serial interface (SSI).
start_ssi_read_data	 	; set up sound input to ssi port.
     bset  #B__SSI_RD_ENABLE,x:X_DMASTAT ; enable ssi input in DMA status
     move #>YB_DMA_R,X0	 	; Read-data buffer ring base address
     move X0,x:X_SSIWP	 	; write ptr (really set in read_data_buffer_in)
     jsr setup_ssi_sound	; start up SSI in 16-bit sound mode
     remember 'consider zeroing read data buffers when starting RD'
     rts

; ================================================================
; wd_buffer_clear - zero out write-data buffers
; called by hmlib:wd_host_on0 and clear_dma_ptrs below
wd_buffer_clear
	jset #B__SIM,y:Y_RUNSTAT,wdbc_loop
	move #YB_DMA_W,R_I1	; beginning of write-data double-buffer ring
	clr A #>NB_DMA_W,B	; total size of both buffers (cf. sys_ye.asm)
	tst B			; zero is disastrous (and MAY OCCUR)
	jle wdbc_loop
	do B,wdbc_loop	; largest immediate DO count is 12 bits, so use B
	  move A,y:(R_I1)+
wdbc_loop
	rts
; ================================================================
; rd_buffer_clear - zero out read-data buffers
; called by hmlib:rd_host_on0 and clear_dma_ptrs below
rd_buffer_clear
	if READ_DATA
	jset #B__SIM,y:Y_RUNSTAT,rdbc_loop
	move #YB_DMA_R,R_I1	; beginning of read-data double-buffer ring
	clr A #>NB_DMA_R,B	; total size of both buffers (cf. sys_ye.asm)
	tst B			; zero is disastrous (and MAY OCCUR)
	jle rdbc_loop
	do B,rdbc_loop	; largest immediate DO count is 12 bits, so use B
	  move A,y:(R_I1)+
rdbc_loop
	rts
	else
	   jsr unwritten_subr
	endif ; READ_DATA
; =============================================================================
; clear_dma_ptrs - init dma buffer state to the turned off and clear condition
;	      dma buffers are not zeroed. This should be done dynamically.
;	      Called in reset_boot.asm and in hmlib_mk.asm
;
clear_dma_ptrs move #0,X0	   ; 0 => no active DMA (cf. sys_xe.asm)
	       move X0,x:X_DMASTAT ; dma is OFF initially

; For write data:
	       move #>YB_DMA_W,X0  ; Write DMA buffer, 1st half
	       move X0,x:X_DMA_WFB ; Start of initial "write-filling" buffer
	       move X0,x:X_DMA_WFP ; Corresponding "write-filling" pointer
	       move X0,x:X_DMA_WFN ; Corresponding "write-fill-next" pointer
	       move #>YB_DMA_W2,X0 ; Write DMA buffer, 2nd half
	       move X0,x:X_DMA_WEB ; Start of initial "write-emptying" buffer

;*	       move X0,x:X_SSI_WEP ; SSI "write-emptying" pointer

; For read data:
		if READ_DATA
		move #>YB_DMA_R,X0  ; Read DMA buffer, 1st half
		move X0,x:X_DMA_RFB ; Start of initial "read-filling" buffer
		move X0,x:X_SSI_RFP ; SSI "read-filling" pointer
		 remember 'may want RFN pointer here too'
		move #>YB_DMA_R2,X0 ; Read DMA buffer, 2nd half
		move X0,x:X_DMA_REB ; Current "read-emptying" buffer
		move X0,x:X_DMA_REP ; Corresponding "read-emptying" pointer
		rts   
		endif ; READ_DATA
;
;==============================================================================
; service_write_data (dispatched from hmdispatch.asm)
;;
;; DESCRIPTION
;;   Advance write-data fill-pointer and check to see if it's time to 
;;   switch write-data output buffers
;;
service_write_data1
	move x:X_DMA_WFN,X0	   ; Next dma write-fill pointer (out.asm)
	move X0,x:X_DMA_WFP	   ; Make it current
	move x:X_DMA_WEB,A	   ; Address of other half of ring buffer to A
	cmp X0,A		   ; When time to switch, we get equality
	jseq write_data_switch     ; Switch write buffers on equality
;
; *** CLEAR NEXT TICK IN DMA SOUND-OUT BUFFER ***
;
	clr B x:X_DMA_WFP,R_O       ; current position in dma output buffer
	move #(NB_DMA_W-1),M_O 	    ; write-data buffer is modulo ring size
	rep #(2*I_NTICK)	    ; length of stereo tick
	     move B,y:(R_O)+        ; clear it
	move #-1,M_O                ; always assumed
	move R_O,x:X_DMA_WFN        ; next write-fill pointer
	rts
;
;==============================================================================
; write_data_switch - switch write-data buffer-ring halves
;;
;; DESCRIPTION
;;   write_data_switch is called at the two crossover points between the two 
;;   halves of the write-data buffer ring.  See the end_orcl macro in
;;   /usr/lib/dsp/smsrc/beginend.asm for an example call.	 This is also a convenient
;;   time to check for underrun errors and the like (jsr check_errors).
;;
write_data_switch
write_data_switch1
;*	  jsr check_errors	   ; error messages from interrupt level
	  remember '"jsr check_errors" removed because wd underrun too common'
	  ; we hear about it once in ssi_xmt_exc already.
	  jsr write_data_wait	   ; wait until DMA_WEB is traversed by all
	  move x:X_DMA_WFB,X0	   ; current write-data fill buffer (now full)
	  move x:X_DMA_WEB,Y0	   ; write-data empty buffer (better be done!)
	  move Y0,x:X_DMA_WFB	   ; new write-data fill buffer
	  move Y0,x:X_DMA_WFP	   ; new write-data fill pointer
	  move X0,x:X_DMA_WEB	   ; new write-data empty buffer
;*	  move X0,x:X_SSI_WEP	   ; new write-data empty pointer for SSI
				   ; (host uses R_IO for a WEP)
	  bchg #B__WRITE_RING,x:X_DMASTAT ; indicate parity of wd ring

	  jsr write_data_buffer_out 

	  rts
;==============================================================================
; write_data_wait - wait until WD readers (SSI and/or HOST) finish reading.
;	Called by write_data_switch.
;
;; DESCRIPTION
;;   Here is where we block waiting for the emptying write-data buffer
;;   to make it to safety. Either or both of the host and the SSI may be taking
;;   the write data, and we must wait for both of them to get their data.
;;   Typically, the SSI is nearly in real time, and the host is much faster
;;   than real time. Thus, we normally wait a while for the SSI, but should
;;   rarely, if ever, wait for the host to get its data.
;;   
;;   Note: we always wait until each DMA request is satisfied before sending
;;   out another. In other words, there is never more than one pending DMA
;;   request in each direction.
;;

write_data_wait

	; WD DMA req pending? If not, either WD is disabled or
	; host has taken all past buffers (see hmlib:host_host_r_done0)
	; If yes, block till host reads the WD-emptying buffer (WEB).

	; If SSI is reading, block till SSI reads WEB also.

	; Note that overrun of the read-pointers is not detected.
	; The read-pointer can get as much as a whole buffer ahead before 
	; losing.  The modulo addressing is the reason overrun is undetectable.

; ** HERE IS WHERE WE SPEND A LOT OF TIME BLOCKED AWAITING WRITE-DATA **
;    (assuming we are running ahead of real time)
;
; State table (WD = B__HOST_WD_PENDING bit) (DM = DSP Message)
;
; WDP HTIE	Meaning
; --- ----	-------
;  0    0 	no DMA, no DMs
;  0    1 	DMs in progress
;  1    0	DMA is pending and is hung waiting for the buffer to be filled
;  1    1 	Active DMA
;
; Here we want to block if we are in state (1,1) since this is the DMA-out
; of the previously filled buffer (not the one filled just now).  That DMA was
; actually started (by blocking if necessary until HTIE could be set to 
; start the transfer) when the newly filled buffer began filling.

wdw_block_host
	bset #B__BLOCK_WD_FINISH,y:Y_RUNSTAT 	; indicate blocked status
 	jclr #1,x:$FFE8,wdw_no_block_host 	; Wait until HCR(HTIE) == 0
	jclr #B__HOST_WD_PENDING,x:X_DMASTAT,wdw_no_block_host ; or ~pending
	jmp wdw_block_host
wdw_no_block_host
	bclr #B__BLOCK_WD_FINISH,y:Y_RUNSTAT 	; clear blocked status

; Join also on SSI reads.
; Note: The SSI block-read is sensed automatically.
; We could do automatic block transfer sensing for the host also, thus
; eliminating the need for the DSP_HM_HOST_R_DONE message from the host.
; However, automatic sensing can fail to detect overrun of the read
; (which should never happen in this case).  It seems there is
; plenty of time for the host to tell the DSP
; it has taken the block.  Perhaps another good reason is that presently
; the host can assume no new HOST_R_REQ can come in until it
; has sent the "DSP_HM_HOST_R_DONE" host message.  If we automatically
; sensed the transfer, the next R_REQ could race with the DMA complete
; interrupt in the host and possibly increase the number of states in the
; kernel.

wdw_block_ssi		     	; block until ssi reads its write-data buffer
	jclr #B__SSI_WD_PENDING,x:X_DMASTAT,wdw_no_ssi_wd
	move x:X_DMA_WFB,A	; Beginning of WD fill buffer (DMA_WFB)
	move x:X_SSIRP,X0	; SSI read ptr in WD emptying buffer (DMA_WEB)
	sub X0,A #>NB_DMA_W/2,X0 ; number of words to go for SSI read pointer
	jle wdw_unblock_ssi	; SSI has read whole emptying buffer
	sub X0,A 		; (done_position - current_position) - length
	jle wdw_block_ssi	;   positive if we've wrapped => unblock
wdw_unblock_ssi
	bclr #B__SSI_WD_PENDING,x:X_DMASTAT ; clear pending status
wdw_no_ssi_wd
	rts
; =============================================================================
; read_and_write_data - called when both read-data and write-data are enabled
read_and_write_data ; called when both read-data and write-data are enabled
	  move #DE_DMAWRECK,X0	   ; error code for this situation
	  jsr stderr		   ; add PC, SR, time, etc. and ship it
	  rts
; =============================================================================
; read_data_wait - wait until all RD fillers (SSI and/or HOST) finish writing
;
; TODO: If deferred termination is pending, when current buffer is consumed, 
; read-data is turned off before the next W_REQ goes out.
;
read_data_wait
	  remember 'write read_data_wait from write_data_wait after debugging'
	  rts
; =============================================================================
read_data_request
	remember 'This has to take a channel number from 1 to 16'
	remember 'send parity of ring buffer.'
;;
;;* The following code is tested and works for channel 1:
;*	clr A #>I_RING_PARITY,X0	; the assigned bit
;*	btst #B__WRITE_RING<chan>,x:X_DMASTAT ; parity of wd ring <FOR CHAN>
;*	tcs X0,A
;*	move #DM_HOST_W_REQ,X0  ; host-read request for host
;*	or X0,A 
;*	move <dma channel number>,X0
	jsr dspmsg
	rts
; =============================================================================
; write_data_buffer_out - prompt write-data takers to read new DMA_WEB
;
;; DESCRIPTION
;;   This is called by write_data_switch when a write-data buffer has been 
;;   filled and needs to be sent out to the host or the ssi or both.
;
write_data_buffer_out
;
; Start SSI output if necessary
;
	  jclr #B__SSI_WD_ENABLE,x:X_DMASTAT,wdbo_no_ssi_wd ; must be enabled
	  bset #B__SSI_WD_PENDING,x:X_DMASTAT ; mark pending status
	  jset #B__SSI_WD_RUNNING,x:X_DMASTAT,wdbo_ssi_running ; read the below
	  bset #B__SSI_WD_RUNNING,x:X_DMASTAT ; once started, do not reset
	  move x:X_DMA_WEB,A1	   ; new write-data empty buffer
	  move A1,x:X_SSIRP	   ; point SSI to buffer beginning
	  bset #14,x:M_CRB	   ; SET TIE = Transmit Interrupt Enable
	  bset #12,x:M_CRB	   ; SET TE  = Transmit Enable
;;	  <zap>			   ; First WEB word at SSIRP grabbed by ssi_xmt
;;	  <zap>			   ; Second word grabbed to fill double buffer
wdbo_ssi_running
	  remember 'check that conditions set by ssi wd turn-on still hold'
	  ;; e.g. check SSI control bits, compare WEB to SSIRP, etc.
wdbo_no_ssi_wd
;
; Write-data DMA is started up is as follows.
;
; We first block until B__HOST_WD_PENDING is 1, HTIE is 0, and HF1 is 1.
; This means the DMA for the current buffer has been set up and is waiting to
; go.  We set HTIE to start this DMA.  Next, we enqueue a HOST_R_REQ for the
; NEXT buffer to be filled.  When the HOST_R_REQ message is delivered (in the
; host_xmt handler), that DMA will be set up (setting B__HOST_WD_PENDING)
; with HTIE off.  As long as we are in the current DMA, HTIE will remain on.
; B__HOST_WD_PENDING is logically B__HOST_READ for channel 0.  If a DMA on
; another channel is in progress, B__HOST_WD_PENDING will not be set, and we
; will continue to wait. B__HOST_WD_PENDING is cleared in the hm_host_r_done
; handler.
;
	if WRITE_DATA_16_BITS
; First, we need to shift the buffer just filled:
	move x:X_DMA_WEB,R_I1
	move #1,N_I1
	move #@pow(2,-8),X0	; for shifting right 8 bits
	move y:(R_I1),Y0
	do #I_NDMA,wdbo_shift
;
; We cannot do a mpyr below, because a $7FFF,,1xx will wrap around to $8000
;
		mpy X0,Y0,A y:(R_I1+N_I1),Y0
		move A,y:(R_I1)+
wdbo_shift
	endif ; WRITE_DATA_16_BITS

; Referring to the state table in write_data_wait, we now want to wait
; explicitly for state (1,0) in which the DMA is started but hung awaiting
; HTIE to be set to enable the transfer.  In addition, we await the host
; flag HF1 which the kernel sets when it has completed the INIT of the
; host interface in DMA mode. 

	jset #B__SIM,y:Y_RUNSTAT,wdbo_unbuzz ; don't block in simulator

	; if not B__HOST_WD_ENABLE and not B__HOST_WD_PENDING, don't block
	jset #B__HOST_WD_ENABLE,x:X_DMASTAT,wdbo_buzz
	jset #B__HOST_WD_PENDING,x:X_DMASTAT,wdbo_buzz
	jmp wdbo_unbuzz

wdbo_buzz
	bset #B__BLOCK_WD_START,y:Y_RUNSTAT ; indicate blocked status
    if !SYS_DEBUG
	jclr #B__HOST_WD_PENDING,x:X_DMASTAT,wdbo_buzz 	; need DMA pending
	jset #1,x:$FFE8,wdbo_buzz  ; HTIE must be clear => DMA not active
				   ; and DSP messages are off
	jclr #4,x:$FFE9,wdbo_buzz  ; wait for HF1     *** 02/24/89/jos ***
    else
	jset #B__HOST_WD_PENDING,x:X_DMASTAT,wdbo_gotp 	; need DMA pending
	  bset #B__BLOCK_WD_PENDING,y:Y_RUNSTAT ; indicate reason for blocking
	  jmp wdbo_buzz
wdbo_gotp
	bclr #B__BLOCK_WD_PENDING,y:Y_RUNSTAT
	jclr #1,x:$FFE8,wdbo_gotc  ; HTIE must be clear => DMA not active
	  bset #B__BLOCK_WD_HTIE,y:Y_RUNSTAT ; indicate reason for blocking
	  jmp wdbo_buzz
wdbo_gotc
	  bclr #B__BLOCK_WD_HTIE,y:Y_RUNSTAT
	  ; *** 02/24/89/jos CHANGED FROM HF0 ***
	  jset #4,x:$FFE9,wdbo_unbuzz ; wait for HF1 => host did DMA INIT
	  bset #B__BLOCK_WD_HF1,y:Y_RUNSTAT ; indicate reason for blocking
	  jmp wdbo_buzz
    endif ; !SYS_DEBUG

wdbo_unbuzz
	bclr #B__BLOCK_WD_HF1,y:Y_RUNSTAT
	bclr #B__BLOCK_WD_START,y:Y_RUNSTAT ; clear blocked status
;
; We are in DMA mode. DSP messages are off and a DMA read is hanging
; for the previously filled buffer.  Start the DMA transfer:
;
	bset #1,x:$FFE8 	   ; Set HTIE in HCR to enable DMA transfer
;
; Enque a DMA transfer request for the buffer we are starting to fill now.
; When this message goes out (in turn after the current DMA completes), we 
; will go back into hung DMA mode [state (1,0)] until the currently filling
; buffer is full.  At that time, we will arrive here again and the transfer
; to the host will be enabled.
;
; *** Tell host to pick up write-data buffer, if write-data is enabled ***
;
	jsset #B__HOST_WD_ENABLE,x:X_DMASTAT,wd_dma_request

wdbo_no_host_wd

	  jclr #B__SIM,y:Y_RUNSTAT,wdbo_done ; do following only for SIMULATOR
		bclr #14,x:M_CRB	; Nevermind SSI transmit interrupts
		bclr #12,x:M_CRB	; (Note: x:X_SSIRP is ADVANCED already)
		bclr #B__SSI_WD_PENDING,x:X_DMASTAT  ; clear SSI pending status
	        move x:X_DMA_WEB,R_I1	; Emptying buffer (not x:X_SSIRP!)
		move #NB_DMA_W-1,M_I1   ; Make it modulo for the heck of it
	        do #I_NDMA,dma_out_simulator
		    move y:(R_I1)+,A	; Outgoing DMA buffer sample
		    move A,y:I_OUTY	; WD output is to y:I_OUTY file
dma_out_simulator			; With SSI transmit interrupts off,
	        move R_I1,x:X_SSIRP	;   this will satisfy write_data_wait
		move #-1,M_I1    	; Assumed by the world
wdbo_done  rts	    
;
; =============================================================================
; wd_dma_request - request DMA output of currently filling write-data buffer
;
; First, we make sure DSP messages are turned off because we can't let a
; host_xmt interrupt happen until all three words are are written to the DMQ.
; We can be called from (1) write_data_buffer_out, in which case DSP messages
; are off and a DMA read (DSP to host) has just been enabled, or from (2)
; the host_wd_on handler in hmlib in which case DSP messages may be on
; but the host_xmt exception cannot happen until after the handler exits,
; or from (3) the sine_test handler which explicitly turns off DSP messages.
;
wd_dma_request
        jset  #B__SIM,y:Y_RUNSTAT,wdr_ok	; don't block if simulating
	jset #B__DM_OFF,x:X_DMASTAT,wdr_ok	; if DSP messages are off
	move sr,A 				; or host is locked out
	move #>$300,X0
	and X0,A
	jne wdr_ok				; then we're ok
		DEBUG_HALT			; else we can be interrupted
wdr_ok
	move #>1,A			; DMA skip factor
	move #DM_HOST_R_SET1,X0		; HOST_R setup word 1
	jsr dspmsg			; enqueue

	move #>((I_WD_CHAN<<3)+SPACE_Y),A ; channel, memory space code
	move #0,X0			; pure data word (HOST_R_SET2)
	jsr dspmsg

	move x:X_DMA_WFB,A		; DMA start address
	move #0,X0			; pure data word (HOST_R_SET3)
	jsr dspmsg			; enqueue read request

 	rts
; =============================================================================
; service_tmq		 ; execute timed messages for current tick
;;			 ; called once per tick loop at user level
;;
;; DESCRIPTION
;;
;;   While (l:L_TICK equals time-stamp of next timed message) {
;;	  lock TMQ
;;	  point R_I1 to opcode
;;	  Execute the timed message
;;	  pop top frame of TMQ
;;	  unlock TMQ
;;   }
;;
;;   See /usr/lib/dsp/smsrc/hmlib.asm for TMQ format.
;;
;; BUGS
;;   The entire message is executed with the host masked which means
;;   no host messages or DSP messages can get through.  If something 
;;   goes wrong in executing the message, debugging is difficult.  
;;   (We could use the IRQA interrupt to reset the IPR for this.)
;;
;;   To allow host messages to come in, we could use a status bit to indicate
;;   that the TMQ is in use, and the xhmta handler could block when it
;;   needs to move a message from the HMS to the TMQ which would overwrite an
;;   unread message.  Important note: if this is implemented, it is critical
;;   that HF3 not be turned on in the interior of a host message.  This is
;;   because host messages are atomic, and the higher priority DMA completion
;;   host command sent by the driver cannot get in, and this means HF1 will
;;   never clear, and deadlock will result.  See misc.asm at the macro
;;   begin_interrupt_handler for more details.  The suggested solution is
;;   to only set HF3 when dismissing from an xhm host message (copy the
;;   bit from bit #B__TMQ_FULL,y:Y_RUNSTAT).  Clearing HF3, however, should
;;   be done as soon as possible and may be asynchronous. 
;;
	remember 'Can we flush link/count and use TMQ_MEND only?'
;	Link is good for independently popping off message from Q
;	rather than depending on each hm handler to leave ptrs right.
;	Maybe use count only for this.

service_tmq1 ; execute timed messages for current tick

	  jclr #B__TMQ_ACTIVE,y:Y_RUNSTAT,st_TMQ_empty
	  remember 'TMQ active check happening twice, as is time stamp compare'

service_tmq2
	  mask_host		   ; set interrupt pri mask to stifle host
	  move #NB_TMQ-1,M_I1	   ; TMQ is a modulo buffer
	  move x:X_TMQRP,R_I1	   ; TMQ read pointer (points to tail mark)
	  move R_I1,R_I2	   ; nop
st_check_tmq			   ; see if TMQ has something to execute
	if SYS_DEBUG
	  move y:(R_I1),B	   ; TMQ tail mark to B1. R_I1 -> time stamp.
	  move #TMQ_TAIL,X0	   ; What tail should look like
	  cmp X0,B		   ; Check for clobberage
	  jeq st_tm_ok
	       move #DE_TMQTMM,X0  ;	No TMQ tail
	       jsr stderr	   ;	Complain
st_tm_ok  
	endif
	  clr B	(R_I1)+		   ; Get B2 clear. B will hold time stamp.
	  move y:(R_I1)+,B0	   ; low-order word of time stamp
	  move y:(R_I1)+,B1	   ; high-order word of time stamp
	  tst B			   ; check for TMQ empty
	  jne st_TMQ_not_empty	   ; ZERO TIME-STAMP DENOTES EMPTY TMQ:
				   ; (since such a msg would have gone to UTMQ)
	       bclr #B__TMQ_ACTIVE,y:Y_RUNSTAT ; turn off TMQ
	     if SYS_DEBUG
	       move y:(R_I1)+,B    ; link word
	       tst B		   ; must be zero in null msg terminating TMQ.
	       jne st_nme	   ; nonzero = null message error.
	       move #TMQ_HEAD,X0   ; make sure head marker is in place too
	       move y:(R_I1)+,B	   ; should be head
	       cmp X0,B
	       jeq st_tmq_done				    
st_nme		 tfr B,A #DE_TMQHMM,X0 ; TMQ head mark missing or link fouled
		 if 1		   ; FIXME: temporary while UTMQ not existent
			lua (R_I1)-,R_I1 ; zero time stamp means do it now
			jmp st_TMQ_not_empty ; note that HMM not detected also
		 endif
		 jsr stderr
	     endif ; SYS_DEBUG
		 jmp st_tmq_done
st_TMQ_not_empty
;
; *** HERE IS WHERE THE TIME STAMP IS LOOKED AT AND UNDERRUN IS DEALT WITH ***
;
; The message is executed when the tick time (in samples) is greater than
; or equal to the time stamp (in samples).  If the tick time exceeds the
; time stamp by a whole tick or more, an underrun error message is generated.
;
	move l:L_TICK,A	   	; tick count (already updated for next tick)
	cmp A,B #>I_NTICK,X0	; form sign of B-A = timeStamp-TICK
	jgt st_tmq_done		; NOPE, next thing to do still in future

	if TMQU_MSGS
	    jeq st_xct_tm	   ; If not TMQ underrun, process the message
	    move B0,Y0		   ; save time stamp in B to Y (could use l:)
	    clr B B1,Y1
	    move X0,B0		   ; tick size to B as long
	    sub B,A  Y1,B 	   ; A = previous TICK
	    move Y0,B0
	    cmp A,B		   ; B-A = timeStamp-TICK+NTICK
	    jgt st_xct_tm	   ; Previous tick saw message in the future
	      move #DE_TMQU,X0     ;   else UNDERRUN (timeStamp<TICK)
	      move y:Y_TICK,A	   ; Current time at underrun (low-order word)
	      jsr dspmsg
	 endif ; TMQU_MSGS
st_xct_tm			   ; Execute next item in TMQ

	  clr A y:(R_I1)+,X0	   ; next-message link, or wd-count-left, or 0
	  move X0,R_O		   ; link word to R_O in case it's a link
	  move #>YB_TMQ,A  	   ; TMQ start address
	  cmp X0,A		   ; start address - (link|count)
	  remember 'IGNORING LINKS because have_link code still not written'
; !!!	  jle st_have_link	   ; link >= start address, count always less

st_no_link     move y:(R_I1),X0	   ; Op code or message terminator TMQ_MEND
	       move #TMQ_MEND,A	   ; check for message terminator
	       cmp X0,A		   ; if not there, assume opcode
	       jeq st_nolink_done
		    jsr jsr_hm	   ; execute message (handlers.asm)
		    jmp st_no_link ; keep going until terminator TMQ_END seen
st_nolink_done
	       move #TMQ_TAIL,X0   ; advance tail over executed messages
	       move X0,y:(R_I1)	   ; overwrite TMQ_MEND with TMQ_TAIL
	       move R_I1,x:X_TMQRP ; update TMQ read pointer to new tail

st_did_one
	;
	; Even though we are never called at interrupt level,
	; host-interface interrupts are disabled.  Thus, there
	; is not much point in calling check_tmq_full after each
	; message in order to post an early HF3 clear. On the
	; other hand, if deadlock is a problem, calling it here
	; could help.

		; *** LOOP BACK POINT ***
	       jmp st_check_tmq 	; go check for another timed message

st_have_link
	  remember 'no link support at present'
	  jsr unwritten_subr
	  jmp st_did_one

st_tmq_done
	;
	; Check to see if the TMQ is full.
	;
	  jsr check_tmq_full  	   ; unblock timed messages if there's room
	;
	; Now, HF3 is SET if the TMQ is full, CLEAR if it is not full, and
	; #B__TMQ_FULL,y:Y_RUNSTAT is SET to HF3
	;
	; 
	; ************************* BLOCKING LOOPS ***************************
	;
	; Enable host interrupts so host messages can come in.
	; Can block within parens or in low water.
	;
	; HOWEVER, HF2 will be set if a timed-zero message comes in.
	; We must watch for this and execute all timed-zero messages
	; as they come in.
	;
	; We also inhibit blocking if the ABORTING bit is set.
	;
	move #-1,M_I1		   ; M registers assumed -1 always
	unmask_host		   ; restore interrupt priority mask (pop SR)

	; *** PARENTHESES BLOCKING ***
	jclr #B__TMQ_ATOMIC,x:X_DMASTAT,st_not_atomic
	; Here we are in an atomic block, and the host must send us more
	; timed messages up to the close_paren message.
	;; However, if a "timed-zero message" is pending, it can't get in,
	;; so we must clear that up now.  Since we are at a tick boundary,
	;; (because "jsr hm_service_tmq" only appears at the top of the orch.
	;; loop), the TZM is legal to do now.
st_buz_tm 
	jsset #B__TZM_PENDING,x:X_DMASTAT,loc_xhmta_return_for_tzm
	;; Also refrain from blocking if we're aborting:
	jsset #B__ABORTING,y:Y_RUNSTAT,abort_now	; abort => can't block
	jset #B__TMQ_FULL,y:Y_RUNSTAT,st_not_atomic ; no point waiting if full
	jclr #B__TMQ_ACTIVE,y:Y_RUNSTAT,st_buz_tm ; wait for new timed msg
	jmp service_tmq2	   ; keep processing TMQ until close_paren seen
st_not_atomic

st_TMQ_empty
	;
	; *** LOW-WATER-MARK BLOCKING (Actually blocking on empty) ***
	;
	jset #B__TMQ_ACTIVE,y:Y_RUNSTAT,st_no_block ; have a future msg q'd
st_buz_lwm
	jclr #B__BLOCK_TMQ_LWM,y:Y_RUNSTAT,st_no_block
	;; Here we want more host messages in the TMQ to avoid underrun.
	;; Clear any waiting timed-zero message:
	jsset #B__TZM_PENDING,x:X_DMASTAT,loc_xhmta_return_for_tzm
	;; Refrain from blocking if we're aborting:
	jsset #B__ABORTING,y:Y_RUNSTAT,abort_now	; abort => can't block
	jclr #B__TMQ_ACTIVE,y:Y_RUNSTAT,st_buz_lwm ; wait for new timed msg
	jmp service_tmq2 ; give new msg a chance to be executed
st_no_block

	rts

; =============================================================================
; check_tmq_full - Determine if Timed Message Queue is too full.
;; Note: it is not sufficient to declare full when there is room for 
;; one maximum message or less because
;; a write may already be in progress to the HMS.  That is, at the time
;; we turn on HF3, a max length message may be coming for sure.  Thus,
;; there should be at least two times NB_HMS as margin.
;
check_tmq_full
	jsr measure_tmq_margin		; result in A = # TMQ data words free
	jle st_tmq_full			; 	otherwise, TMQ is nearly full
	  clear_hf3
	  bclr #B__TMQ_FULL,y:Y_RUNSTAT
	  jcc ct_exit ; if TMQ_FULL bit was already clear, skip the below
	  if USE_TXD_INTERRUPT
	    txd_interrupt			; Wake up driver if necessary
	  else
	    ; Normally we rely on sound-out requests to wake up the
	    ; DSP driver so it can notice the TMQ is no longer full.
	    ; If sound output is to the SSI port only, we have to send
	    ; a message to wake up the kernel:
	    ; *** NOTE *** If message on TMQ low-water mark feature is enabled
	    ; (see check_tmq_lwm below) then this is not needed.
	    jset #B__HOST_WD_ENABLE,x:X_DMASTAT,ct_exit
	    jclr #B__SSI_WD_ENABLE,x:X_DMASTAT,ct_exit
	      clr A #DM_KERNEL_ACK,X0 ; Wake up Mach
	      jsr dspmsg
	  endif
	  jmp ct_exit
st_tmq_full
	bset #B__TMQ_FULL,y:Y_RUNSTAT
	set_hf3
ct_exit
	rts

measure_tmq_margin ; result in A is positive when there is "enough" TMQ room.
		   ; called above and in handlers.asm for the host message.
	jsr measure_tmq_room		; result in A = # TMQ data words free
	move #>2*NB_HMS,X0		; large margin
	sub X0,A			; must be positive (allow for opcode)
	rts

; =============================================================================
measure_tmq_room ;  Compute number of free DATA words in tmq (result in A)
		 ;  You can send a single message to the TMQ if the data part
		 ;  (that which is written to TX) does not exceed room.
		 ;  The opcode (which is the last thing written to TX)
		 ;  is not counted (i.e. room for it is reserved here).
		 ;  The purpose of reporting DATA room only is to suppress
		 ;  details of the TMQ format should it change.
		 ;  Registers X0, A, and B are used.
		 ;  Result is in A.  It can be negative.
	move  x:X_TMQWP,X0	; TMQ write pointer (points to null message)
	clr B x:X_TMQRP,A	; TMQ read pointer (points to tail mark)
	sub X0,A #>NB_TMQ,X0	; Compute rp - wp = number of free words
	tlt X0,B		;   mod out buffer length
	add B,A	#>9,X0		;   (since TMQ is a ring)
	if SYS_DEBUG
	jge ct_nowrap		; negative means we're lost in space
		move #DE_TMQRWPL,X0  ; read or write pointer garbaged?
	        jsr stderr	; never returns
ct_nowrap
	endif
	sub X0,A		; rp-wp-4(nullmsg&hmk)-5(ts,op,link,MEND)
	jclr #B__TMQ_LWM_ME,y:Y_RUNSTAT,ct_no_lwm_ck
;;
;; See if TMQ is nearly empty.
;; If so, set the bit and issue DM_TMQ_LWM message if that feature is enabled,
;; and then disable it. (The host enables each LWM notification individually.)
;;
	move #>I_NTMQ_ROOM_HWM,Y1 ; TMQ free-space HWM
	cmp Y1,A #DM_TMQ_LWM,X0	; #wds_free - #fs_hwm = (+) if too much FS
	jle ctl_high_water	; 			(+) => "low water"
	  bset #B__TMQ_LWM,y:Y_RUNSTAT ; indicate below low-water-mark status
	  jsset #B__TMQ_LWM_ME,y:Y_RUNSTAT,dspmsg
	  bclr #B__TMQ_LWM_ME,Y:Y_RUNSTAT	; *** MUST ENABLE EACH MSG ***
	  rts
ctl_high_water
	bclr #B__TMQ_LWM,y:Y_RUNSTAT ; indicate above low-water-mark status
	rts

ct_no_lwm_ck
	rts
;; ================================================================
	endif ; !AP_MON
;; ================================================================
;
; **************** MORE ROUTINES CALLED BY BOTH APMON AND MKMON ***************
;
;; ================================================================
; abort - abort DSP execution
;;	  DMA in progress is completed. (A DMA read gets an abort symbol.)
;;	  Queued up DSP messages are sent out.
;;	  Execution stack is rolled back to 0.
;;	  Host interrupts are enabled.
;; 	  Host communication status is reset.
;;	  Falls into monitor mode awaiting debugger
;;
abort1
	bset #B__EXTERNAL_ABORT,x:X_DMASTAT 	; called via hmdispatch
	jset #B__ABORTING,y:Y_RUNSTAT,ab_nosave ; Don't save regs twice!
abort						; called internally
	bset #B__ABORTING,y:Y_RUNSTAT ; Inhibit DMA requests (host_xmt)

; Save everything we are about to change
	move X0,x:X_ABORT_X0	; save X0 for later inspection
	move A1,x:X_ABORT_A1	; save A1 for later inspection
	movec sp,A1
	move A1,x:X_ABORT_SP
	movec sr,A1
	move A1,x:X_ABORT_SR
	move x:$FFE8,A1	; HCR
	move A1,x:X_ABORT_HCR
	move x:$FFE9,A1	; HSR
	move A1,x:X_ABORT_HSR
	move R_HMS,x:X_ABORT_R_HMS
	move R_IO,x:X_ABORT_R_IO
	move M_IO,x:X_ABORT_M_IO
	move R_I1,x:X_ABORT_R_I1
	move x:X_DMASTAT,A1
	move A1,x:X_ABORT_DMASTAT
	move y:Y_RUNSTAT,A1
	move A1,x:X_ABORT_RUNSTAT
ab_nosave
	jsr dma_abort		; Let any DMA in progress finish, then disable
ab_buzz	jset_htie ab_buzz 	; let DSP messages out
	jsr abort_now		; never returns. We JSR to leave stack trail.

; ================================================================
; dma_abort - Let any DMA in progress finish, then return.
;	Called by abort(handlers.asm) & reset_soft(jsrlib.asm) (hence on boot)

dma_abort
		; First we must enable interrupts so that the DMA termination
		; host command can get through

		jsr abort_interrupt	; clear state associated with interrupt

        	jclr  #B__SIM,y:Y_RUNSTAT,dmaa_ck_r ; SWI if simulating
			SWI
;
;	Wait until DMA completes
;
dmaa_ck_r	jclr #B__HOST_READ,x:X_DMASTAT,dmaa_r_done ; check DMA read
		jclr_hf1 dmaa_r_done ; misc.asm	; HF1 <=> host DMA in progress
;*		bset_htie			; Is this needed sometimes?
		jmp dmaa_ck_r			; keep going until host_r_done
dmaa_r_done 	jsset #B__HOST_READ,x:X_DMASTAT,host_r_done0 ; if hf1 cleared
dmaa_buzz	jset_hf1 dmaa_buzz	    	; if host_r_done only

	    if READ_DATA
		fail '*** FIXME *** Make rd case analogous to wd case'
	    endif ; READ_DATA

	    rts

; ============================================================================
; abort_interrupt - clear state associated with servicing an interrupt.
;; 	Call this routine to abort from interrupt level, e.g., during
;; 	the execution of a host message.  Called by idle_0 and abort
abort_interrupt
	bclr #B__ALU_SAVED,y:Y_RUNSTAT  ; We'll not restore ALU
	bclr #B__TEMPS_SAVED,y:Y_RUNSTAT ; nor temp regs (handlers.asm)
	bclr #3,x:$FFE8    		; clear "DSP Busy" flag
        move x:X_HMSRP,R_HMS 		; reset HMS write pointer back
        move R_HMS,x:X_HMSWP 		;   to read pointer to avoid arg error
        unmask_host   	   		; allow interrupts
	rts

;; ================================================================
; abort_now - abort DSP execution
;;	  Falls into monitor mode awaiting debugger
;;
abort_now  			; "abort" entry point is in hmdispatch.asm
	movec #$300,sr 		; disable interrupts to freeze things until
				; debugger (BUG56) arrives on the scene
	move x:$ffe8,A 		; get HCR
	move #>$18,X0	        ; set HF2 and HF3 to indicate we're done

	define DRIVER_RESETS_ON_ABORT '1'
	if DRIVER_RESETS_ON_ABORT
	jset #B__EXTERNAL_ABORT,x:X_DMASTAT,an_external ; e.g. by dspabort
	  move #>$10,X0		; *** FIXME: Driver will reset DSP if HF2&HF3!!
	  bclr #B__EXTERNAL_ABORT,x:X_DMASTAT ; allow dspabort to get in now
	  or X0,A		; Just set HF3 to hang driver as if "busy"
	  move A1,x:$ffe8
	  move #DE_ABORT,A
	  write_htx A		; Hopefully this will get through to user
	  movec #$0,sr 		; enable host commands (want external abort)
	  jsr an_hang		; Leave evidence on the stack
an_hang	  jmp an_hang		; Wait for HC_ABORT, e.g. by dspabort
an_external
	endif

	or X0,A
	move A1,x:$ffe8		; This tells the Mach world we've aborted

;; Wait for HF0 (HF0 alone means "abort DSP").  This is asserted by dspabort
;; when driver is put to sleep by setting ICR to 0.  Without this it is a race
;; between the driver and dspabort for the following output.  Note that there
;; is still the possibility that the driver will come back to life, set ICR to
;; 1, and intercept output below.  For this reason, the driver needs to be
;; modified (in the device interrupt loop: snd_dspdev.c near line 705)
;; to ignore DSP input (and in fact abort) when it sees HF2 and HF3 both set.

	await_hf0
	jset_hf1 an_send_start	; BUG56 sets HF0 and HF1 together

; Send stack pointer
	clr A #DE_SP,X0		; SP message code
	movec sp,A1		; SP to A1 for message
	or X0,A A,B		; install SP message code, and save SP in B
	write_htx A1

; Send PC backtrace
	tst B B,Y1		; Compare SP to 0, save original SP to Y1
	jle an_no_bt		; SP must be positive for backtrace to exist
	clr A #DE_PC,X0		; PC message code
an_bt	movec ssh,A1		; PC to A1, right justified (*pop*)
	or X0,A			; install "PC" message code
	write_htx A1		; send PC backtrace component
	move sp,B		; current sp to B
	tst B
	jgt an_bt		; send ssh until stack pointer is 0
an_no_bt
	move Y1,sp		; restore original SP
	write_htx #DEGMON_L+((DEGMON_H-DEGMON_L+1)*65536) ; for dspabort
	await_hf1		; HF0 and HF1 indicates debugger has control
	await_hf0		; Make sure this is still on
; send start address (bits 0..15) and length of DEGMON (bits 16..23) to BUG56:
an_send_start
	write_htx #DEGMON_L+((DEGMON_H-DEGMON_L+1)*65536)
	movec #0,sr  		; Allow host interrupts only to debugger
	move #>$BF080,X0	; Get "JSR" opcode (two-word type)
	movem X0,p:iv_swi_	; Install it at SWI vector
	if DEGMON
	  move #>DEGMON_TRACER_LOC,X0 ; DEGMON's SWI handler
	else
	  move #>abort,X0	; What we use
	endif
	movem X0,p:(iv_swi_+1)
	jsr DEGMON_TRACER_LOC	; Fall into DEGMON_TRACER (breakpoint)


