; dspmsgs.asm - assign integer message codes to dsp messages.
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;;  Included by defines.asm.
;;
msg_ctr	  set 0		 ; dsp messages are opcodes 0 to 127
err_ctr	  set 128	 ; dsp error messages have the MSB on

new_msg	  macro nam	 ; nam = mnemonic for the dsp message opcode
;*	  symobj DM_\nam
DM_\nam	  equ msg_ctr	 ; define the name
msg_ctr	  set msg_ctr+1
	  endm

new_err	  macro nam	 ; nam = mnemonic for the dsp error opcode
;*	  symobj DE_\nam
DE_\nam	  equ err_ctr	 ; define the name
err_ctr	  set err_ctr+1
	  endm

 new_err  BREAK     ; Breakpoint
		    ;	Arg16 = PC of the SWI instruction (ssh)
 new_err  HMARGERR  ; Host message called with the wrong number of arguments
		    ;	or message handler did not consume args correctly.
		    ;	Arg16 = #argsExpected - #argsFound
 new_err  PC	    ; Program Counter,	arg = PC (16 bits)
 new_err  SSH	    ; Status Register,	arg = SR (16 bits)
 new_err  SR	    ; Status Register,	arg = SR (16 bits)
 new_err  LC	    ; Loop Counter,	arg = LC (16 bits)
 new_err  SP	    ; Stack Pointer,	arg = SP (5 bits right justified)
 new_err  TIME0	    ; Tick Time 0,	arg = low-order 16 bits of tick count
 new_err  TIME1	    ; Tick Time 1,	arg = middle 16 bits of tick count
 new_err  TIME2	    ; Tick Time 2,	arg = high-order 16 bits of tick count
 new_err  STATUS0   ; System status word, low 16 bits
 new_err  STATUS1   ; System status word, middle 16 bits
 new_err  STATUS2   ; System status word, high 16 bits
 new_err  ILLSUB    ; Attempt to call a stub subroutine (not yet written)
		    ;	Arg16 = address of stub routine
 new_err  ILLHM	    ; Attempt to jump to nonexistent host message subroutine 
		    ;	Arg16 = offending jump address
 new_err  RESET	    ; DSP has been reset
		    ;	Arg16 = 0
 new_err  DMAWRECK  ; Host requested 2 DMA's without intervening terminator
		    ;	Arg16 = DMA channel number of 2nd DMA request.
 new_err  DHRERR    ; Host initiated a DMA read without turning off DSP msgs
		    ;	Arg16 = 0
 new_err  XHMILL    ; xhm error. Host message type code not recognized.
		    ;	Arg16 = 0
 new_err  DMQOVFL   ; DMQ overflowed. Write pointer overtook read pointer.
		    ;	Arg16 = 0
 new_err  HMSOVFL   ; HMS overflow. Bottom marker overwritten since last xhm.
		    ;	Arg16 = low 2 bytes of what found instead of $7777.
 new_err  HMSUFL    ; HMS underflow. Top marker overwritten since last xhm HC.
		    ;	Arg16 = low 2 bytes of what found instead of $6666.
 new_err  HMSBUSY   ; HMS was busy and one or more host writes happened anyway.
		    ;	Arg16 = R_HMS - x:X_HMSWP (should have been 0)
 new_err  TMQFULL   ; TMQ full. Hold timed messages for a while.
		    ;	Arg16 = Number of free words in TMQ
 new_err  TMQREADY  ; TMQ Ready. Timed messages can be sent again.
		    ;	Arg16 = 0
 new_err  TMQMI	    ; TMQ unblocked with HF2 on.  (service_tmq should NEVER
		    ;   be called within a host-message handler.) Arg16 = 0
 new_err  TMQU	    ; TMQ Underrun. Timed host message came late.
		    ;	Arg16 = current time in samples.
 new_err  TMQTMM    ; TMQ tail mark missing. TMQ probably garbaged.
		    ;	Arg16 = 0
 new_err  TMQHMM    ; TMQ head mark missing. TMQ probably garbaged.
		    ;	Arg16 = low-order 16 bits of what was there instead.
 new_err  TMQTM	    ; TMQ terminator missing. TMQ probably garbaged.
		    ;	Arg16 = 0
 new_err  TMQRWPL   ; TMQ read or write pointer lost. One is out of bounds.
		    ;	Arg16 = tail-head+NB_TMQ (which is negative, impossibly)
 new_err  TMQEOIF   ; TMQ EOF on input file mid-message. Input file invalid?
		    ;	Arg16 = low-order 2 bytes of last word read

		remember 'flush ADM message below'

 new_err  ADMPWE    ; ADM putword routine returned an error. No output file?
		    ;	Arg16 = 0
 new_err  SCROVFL   ; Program passed in execute_msg (hmlib) larger than NPE_SCR
		    ;	Arg16 = size of program attempted
 new_err  SSIWDU    ; SSI write-data underrun
		    ;	Arg16 = 0
 new_err  XMEMARG   ; X memory argument pointer R_X is wrong at orch loop end
		    ;	Arg16 = bad R_X value found at end of orchestra loop
 new_err  YMEMARG   ; Y memory argument pointer R_Y is wrong at orch loop end
		    ;	Arg16 = bad R_Y value found at end of orchestra loop
 new_err  LMEMARG   ; L memory argument pointer R_L is wrong at orch loop end
		    ;	Arg16 = bad R_L value found at end of orchestra loop
 new_err  NO_PROG   ; Control transferred to nonexistent program at p:$40.
		    ;	Arg16 = 0
 new_err  HF2_ON_2  ; "DSP Busy" was already set when DSP needed to set it.
 new_err  ABORT     ; DSP abort signal
 new_err  PLE_SYSMM ; First word of ext sys memory has been clobbered.
		    ;   Arg16 = low-order bits of clobberage at p:system_magic.
 new_err  WFP_BAD   ; write-fill pointer x:X_DMA_WFP is out of range.
		    ;   Arg16 = current value of x:X_DMA_WFP
 new_err  USER_ERR   ; General purpose message for user to self

; * means not in use
 new_msg  ILLDSPMSG  ;* illegal (never sent...defined to detect bogus messages)
 new_msg  KERNEL_ACK ;kernel acknowledge, arg = 0, used by hc_kernel_ack
 new_msg  HOST_W_DONE  ; host_w done,  arg = final value of R_HMS 
 new_msg  HOST_R_DONE  ; host_r done,  arg = final value of R_IO
 new_msg  HOST_W_REQ   ; Read-data ready,  arg16 = channel number
 new_msg  HOST_R_REQ   ; Write-data ready, arg16 = channel number
		       ; Channel 1 also has ring parity bit I_RING_PARITY
 new_msg  DM_OFF    ; DSP messages have been turned off, and this is the last
 new_msg  DM_ON     ; DSP messages have been turned off, and this is the last
 new_msg  DM_MIDI_MSG ; Last MIDI message has been consumed. OK to send next.
 new_msg  HOST_R_SET1 ; HOST_R setup word 1. Arg(16) = <skip factor>
 ; This is followed by
 ; 	HOST_R setup word 2: Arg(7,6,3) = <xxx><chan><space>
 ; 	HOST_R setup word 3: Arg(16) = <buffer address>
 new_msg  TMQ_LWM    ; TMQ wants more messages. arg16 = #words free
;-----------------  ; *** Messages above are used by kernel. DO NOT CHANGE ***
 new_msg  HM_DONE    ; host message done
 new_msg  MAIN_DONE  ;* main program done
 new_msg  PEEK0	     ; Peek value 0	arg = low 2 bytes of peek value
 new_msg  PEEK1	     ; Peek value 1	arg = top byte of peek value
 new_msg  IDLE	     ; Sent from idle loop
 new_msg  NOT_IN_USE ; Formerly RESET_SOFT.  *** AVAILABLE MESSAGE SLOT ***
 new_msg  IAA	     ; "I am alive",	arg = version,revision (8,8 bits)
 new_msg  PC	     ; Program Counter,	arg = PC (16 bits)
 new_msg  SSH	     ; Status Register,	arg = SR (16 bits)
 new_msg  SR	     ; Status Register,	arg = SR (16 bits)
 new_msg  LC	     ; Loop Counter,	arg = LC (16 bits)
 new_msg  SP	     ; Stack Pointer,	arg = SP (5 bits right justified)
 new_msg  TIME0	     ; Tick Time 0,	arg = low-order 16 bits of tick count
 new_msg  TIME1	     ; Tick Time 1,	arg = middle 16 bits of tick count
 new_msg  TIME2	     ; Tick Time 2,	arg = high-order 16 bits of tick count
 new_msg  STATUS0    ; System status word, low 16 bits
 new_msg  STATUS1    ; System status word, middle 16 bits
 new_msg  STATUS2    ; System status word, high 16 bits
 new_msg  HMS_ROOM   ; No. words free in HMS, arg16 = number of words free
 new_msg  TMQ_ROOM   ; No. words free in TMQ, arg16 = number of words free
 new_msg  SSI_WDU    ; SSI write-data underrun. (one message per reset) Arg = 0
 new_msg  HM_FIRST   ; Address of first host-message dispatch
 new_msg  HM_LAST    ; Address of last host-message dispatch
 new_msg  USER_MSG   ; General purpose message for user to self








