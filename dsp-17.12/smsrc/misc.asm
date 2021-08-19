; misc.asm - miscellaneous macros
;
;; Copyright 1989, NeXT Inc.
;; Author - J. O. Smith
;;
;; ============================================================================
fill_unused_memory macro
;
; Set unused memory to a known state.
; Called by end_main (in beginend.asm)
;
		dupc s,'pxyl'
		dupc w,'ie'
		org s\\_\\w:	      ;; no, this is not modem lossage
u\\s\\w\\_usr  equ *+o\\s\\w\\_usr ; next free absolute address
n\\s\\w\\_free equ s\\h\\w\\_usr-u\\s\\w\\_usr+1 ; no. unused words
		if n\\s\\w\\_free<0&&!@def(NO_\\s\\w\\_ARG_CHECK)
			fail 'MEMORY PARTITION OVERFLOW'
n\\s\\w\\_lost equ -n\\s\\w\\_free ; make it easy to read off ovfl amount
		else	; load unused memory with a recognizable marker
		  if n\\s\\w\\_free>0
free_\\s\\w	    ; label for beginning of unused block
		    if "s"=='l'
		      if DBL_BUG
		        ; double load length for l memory case
	 	        bsc 2*n\\s\\w\\_free,UNUSED_MARKER
		      else
		        bsc n\\s\\w\\_free,UNUSED_MARKER<<24|UNUSED_MARKER
		      endif
		    else
		      bsc n\\s\\w\\_free,UNUSED_MARKER
		    endif
		  endif
		endif
		endm
		endm

		if DBL_BUG&&(nli_free>0)
		org l_il:
free_li_low	bsc nli_free,UNUSED_MARKER		
		endif

		if DBL_BUG&&(nle_free>0)
		  warn 'Can''t preset unused le_lo memory: no spare l: counter'
		endif

	if 0	; what we'd like to say is as follows:
;		if DBL_BUG&&(nle_free>0)
;		org l_el:
;free_le_low	bsc nle_free,UNUSED_MARKER		
;		endif
	endif
	endm
; =============================================================================
set_extension macro
        ori #$20,ccr		  ; set extension bit
	endm
; =============================================================================
clear_extension macro
        andi #$DF,ccr		  ; clear extension bit
	endm
; =============================================================================
; The out* series below supports backward compatibility for UG test macros
; =============================================================================
out2	macro pf,ic,ospc,oadr,g0,g1
	out2sum pf,ic,ospc,oadr,g0,g1
	endm
; =============================================================================
outa	macro pf,ic,ospc,oadr
	out2sum pf,ic\a,ospc,oadr,F_ONE,0
	endm
; =============================================================================
outb	macro pf,ic,ospc,oadr
	out2sum pf,ic\b,ospc,oadr,0,F_ONE
	endm
; =============================================================================
outsim	macro pf,ic,ospc,oadr
	out2replace pf,ic\b,ospc,oadr,F_ONE,F_ONE
	endm
; =============================================================================
tick_mult macro name,n ; set name to multiple of I_NTICK >= n
name	set (@CVI((n-1)/I_NTICK)+1)*I_NTICK
	endm
; =============================================================================
abs_adr	macro space,bank,addr,name ; convert to absolute address (if necessary)
name	set addr+o\space\bank\_usr ; space in 'pxyl', bank in 'ie'
	endm
; =============================================================================
lsl_n	  macro arg,n	 ; left-shift arg by n bits. Result in A0.
	  if "arg"=='X1'||"arg"=='x1'
	       move #>(1<<(n-1)),X0
	       mpy X0,X1,A
	  else
	       move #>(1<<(n-1)),X1
	       mpy arg,X1,A
	  endif		      
	  endm
; =============================================================================
mask_host macro ; set interrupt priority mask to 1 to prevent host interrupts
	  ori #1,mr   ; raise level to 1 (lock out host)
	  nop
	  nop
	  nop	; wait for pipeline to clear (yes, you need 8 cycles delay)
	  nop
	  endm
; =============================================================================
unmask_host macro ; set interrupt priority mask to 0
	  andi #$FC,mr		   ; i1:i0 = 0
	  endm
; =============================================================================
nop_sep	  macro num	 ; Generate nop instructions
;	  num = number of nop instructions to generate
	  dup num
	    nop  
	  endm
	  endm 
; ------------------------------ break_on_tick -----------------------------
;
; break_on_tick - Halt orchestra loop when L_TICK >= n_tick*I_NTICK
;
; Since time is incremented at the TOP of the orchestra loop,
; the tick count starts at "1" (l:L_TICK == I_NTICK).
; Therefore, to get exactly n ticks executed in the orchestra loop,
; place "break_on_tick n" at the BOTTOM of the orchestra loop.
;
break_on_tick macro n_tick 
	  clr A l:L_TICK,B	; zero A, current tick to B
	  move #>(n_tick*I_NTICK),A0 ; number of samples desired to A
	  sub B,A	      	; subtract B from A
	  jle END_OLOOP	      	; terminate (see end_orcl macro for label)
	  endm

; ------------------------------ break_on_sample -----------------------------
break_on_sample	macro n_sample 	; Stop orchestra when L_TICK >= n_sample
	  clr A l:L_TICK,B    	; zero A, current tick to B
	  move #>n_sample,A0 	; number of samples desired to A
	  sub B,A	      	; subtract B from A
	  jle END_OLOOP	      	; terminate (see end_orcl macro for label)
	  endm
; ------------------------------ brk_orcl ------------------------------------
brk_orcl  macro n_ticks	      ; Stop orchestra when L_TICK >= n_ticks*I_NTICK
	  break_on_tick n_ticks
	  endm
; -----------------------------------------------------------------------------

set_hf3 macro
	bset #4,x:$FFE8		; Set "TMQ Full"
	endm

clear_hf3 macro
	bclr #4,x:$FFE8		; Clear "TMQ Full"
	endm

set_hf2 macro
	bset #3,x:$FFE8		; Set "DSP Busy"
	endm

clear_hf2 macro
	bclr #3,x:$FFE8		; Clear "DSP Busy"
	endm

jclr_hf1 macro label
	jclr #4,x:$FFE9,label
	endm

jset_hf1 macro label
	jset #4,x:$FFE9,label
	endm

await_hf1 macro
	jclr #4,x:$FFE9,*
	endm

await_hf0 macro
	jclr #3,x:$FFE9,*
	endm

jset_htie macro label
	jset #1,x:$FFE8,label
	endm

bset_htie macro
	bset #1,x:$FFE8
	endm

bclr_htie macro
	bclr #1,x:$FFE8
	endm

jclr_hrdf macro label
	jclr #0,x:$FFE9,label
	endm

jset_hrdf macro label
	jset #0,x:$FFE9,label
	endm

clear_sp macro
	if SYS_DEBUG
       	  movec #2,sp		; clear context stack, leaving room for reset
	else
     	  movec #0,sp		; clear context stack
	endif
	endm

write_htx macro val
	jclr #1,x:$ffe9,*	; wait for HTDE in HSR
	movep val,x:$FFEB  	; Write HTX, point to new input cell
	endm

begin_interrupt_handler macro	; *** CALL BEFORE SAVING ALU REGISTERS ***
	if 0
Clear HF3 ("TMQ full" or "AP busy") and set HF2 ("executing host message")
because HF3 and HF2 together mean "DSP aborted".  Bit B__TMQ_FULL of
y:Y_RUNSTAT retains whether HF3 was set.  We must change both bits
ATOMICALLY because HF2&HF3 => "abort", and !(HF2|HF3) means the host can
write messages.  I have seen a hanging bug (cf. bug #11394) where HF3
clears momentarily and the driver writes down the ints of a host message,
but then cannot write the xhm host command ($13) which sends the message.
Since these are in an atomic message together, the HOST_R_DONE host command
($12) at the end of a DMA read by the host cannot get in, even though it's
higher priority than the blocked host message, because it cannot break in
on an atomic message that's in progress.  This was not fatal before Oct '90
because we did not wait for HF1 to clear in host_xmt before sending out a
message.  We had to start waiting for HF1 to clear because otherwise it was
possible for the INIT which accompanies the clearing of HF1 after a DMA
read to flush the R_REQ (next DMA-read request: $50001) written to the RX
register by write_data_buffer_out.  Thus, waiting on HF1 can result in
deadlock.  It is therefore essential that HF3 not turn on inside of an
atomic message.  Note: The TMQ is processed at user level, but currently the
host is masked out.  If a "TMQ lock" is ever implemented instead,
HF3 will again be able to turn on in the middle of a host message.
	endif
	if AP_MON
	  clear_hf3	; Clear "AP main program executing
	  set_hf2	; "Host message executing"
	else
	move A1,x:X_SAVED_A1	; Save A1
	move x:$FFE8,A1		; Read HCR to A1
	move A1,x:X_SAVED_A2	; Save in memory; we won't clobber A2
	bclr #4,x:X_SAVED_A2	; Clear HF3 in memory version
	bset #3,x:X_SAVED_A2	; Set HF2 in memory version
	move x:X_SAVED_A2,A1	; Obtain newly prepared HCR in A1
	move A1,x:$FFE8		; Post it
	move x:X_SAVED_A1,A1	; Restore A1
	endif
 	endm

end_interrupt_handler macro	; *** CALL AFTER RESTORING ALU REGISTERS ***
;; 
;; Our goal is to clear HF2 and copy bit B__TMQ_FULL of y:Y_RUNSTAT to HF3.
;; We must do this atomically.  If we clear HF2 first, the host may see
;; the 80ns window in which both HF2 and HF3 are off and start sending
;; a host message (very unlikely, yes).  (It does not matter that host
;; interrupts are disabled here, the message will simply block until it
;; gets in.)  If we set HF3 before clearing HF2, there is an 80ns window
;; in which the DSP looks aborted.  This is even more unlikely to be detected,
;; but there it is.  It would be awfully nice if Motorola could add a few 
;; more host flags (there are 3 unused bits in the HCR and 2 in the HSR).
;;
	move x:$FFE8,A1		; Read HCR to A1, saved A1 lies in memory
	move A1,x:X_SAVED_A2	; Save in memory; we won't clobber A2
	bclr #3,x:X_SAVED_A2	; Clear HF2 in memory version
	jclr #B__TMQ_FULL,y:Y_RUNSTAT,_eih_tmq_not_full ; Test for "TMQ full"
	  bset #4,x:X_SAVED_A2	; Set HF3 ("TMQ full") in new HCR
_eih_tmq_not_full
	move x:X_SAVED_A2,A1	; Obtain newly prepared HCR in A1
	move A1,x:$FFE8		; Post it, avoiding race with abort signal
	move x:X_SAVED_A1,A1	; Restore A1
	move A2,x:X_SAVED_A2	; Restore A2
	if 0			; postpone this questionable one to post-2.0
	  txd_interrupt		; Wake up driver if necessary
	else
	  remember 'should issue TXD interrupt if interrupt handler is long'
	  remember 'save current time in begin_... and measure duration here'
	endif
     endm

set_txd macro
	bset #1,x:$FFE5 	; can interrupt CPU using TXD (if enabled)
	endm

clear_txd macro
	bclr #1,x:$FFE5 	; can interrupt CPU using TXD (if enabled)
	endm

txd_interrupt macro
	if USE_TXD_INTERRUPT
	clear_txd
;; On chaos (mminnick's 040 machine), we know 56 nop's is enough and 48 isn't
	rep #64
	  nop
	set_txd
	endif
	endm
