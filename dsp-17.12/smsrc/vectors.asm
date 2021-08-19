; vectors.asm - DSP 56001 interrupt vector contents
;;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;;   Included by ./allocsys.asm.
;;   These vectors call routines in ./handlers.asm.
;;   They are installed last to avoid forward references to the handlers.
;;   (which triggered complaint from the assembler).
;;
;;   *** WARNING ***
;;   A single two-word instruction used as a fast interrupt
;;   vector will most definitely run the risk of at some point returning
;;   from the fast interrupt to a JSR instruction in the main program code
;;   which will result in the status register interrupt mask bits being updated
;;   as if a long interrupt routine had been invoked.  As I understand it
;;   this bug is in all the 56000 revisions that have been released to date. 
;;   4/12/89
;;
vectors	       ident 0,9	 ; Install two-word interrupt vectors

; ------------------- INTERRUPT VECTORS ----------------

	       if *!=0
	       fail 'attempt to load interrupt vectors other than at p:0'
	       endif

 xdef iv_reset_,iv_stk_err,iv_trace_,iv_swi_,iv_irq_a,iv_irq_b,iv_ssi_rcv
 xdef iv_ssi_rcv_exc,iv_ssi_xmt,iv_ssi_xmt_exc,iv_sci_rcv,iv_sci_rcv_exc
 xdef iv_sci_xmt,iv_sci_idle,iv_sci_timer,iv_nmi,iv_host_rcv
 xdef iv_host_rcv2,iv_host_xmt,iv_host_xmt2,iv_host_cmd,iv_xhm,iv_dhwd,degmon_l

iv_reset_       jsr >reset_boot ; reset vector for offchip boot load
				; after boot, "jsr reset_" used instead
iv_stk_err     	JMP >DEGMON_STACKERROR	;stack overflow error
iv_trace_      	JSR >DEGMON_TRACER	;TRACE interrupt handler
iv_swi_	       	JSR >DEGMON_TRACER	;SWI handler (DEGMON breakpoint)
iv_irq_a       	jsr >abort1 ; external abort
iv_irq_b       	jsr >abort  ; internal abort
iv_ssi_rcv     	jsr >ssi_rcv
iv_ssi_rcv_exc 	jsr >ssi_rcv_exc
iv_ssi_xmt     	jsr >ssi_xmt
iv_ssi_xmt_exc 	jsr >ssi_xmt_exc
iv_sci_rcv     	DEBUG_HALT
		nop
iv_sci_rcv_exc 	DEBUG_HALT
		nop
iv_sci_xmt     	DEBUG_HALT
		nop
iv_sci_idle    	DEBUG_HALT
		nop
iv_sci_timer   	jsr >sci_timer
iv_nmi	 	JSR >DEGMON_TRACER ;TRACE intrpt handler (NMI) - used by BUG56
iv_host_rcv    	movep x:$FFEB,y:(R_HMS)- ; write (circular) Host Message Queue
iv_host_rcv2   	nop
;* iv_host_rcv    movep x:$FFEB,A
;* iv_host_rcv2   movep A,x:$FFEB
iv_host_xmt    	jsr >host_xmt
;* This version gets a relative symbol!: iv_host_xmt2   equ p:iv_host_xmt+1
iv_host_xmt2   	equ iv_host_xmt+1
iv_host_cmd    	jsr >hc_host_r_done  	; Terminate DMA read from host

	       	if (*!=$26) ; make sure * points to first host command vector
DOT_HC		    set *   ; This gets * into listing file
		    fail 'vectors.asm: interrupt handlers off.	*!=$26'
	       	endif

iv_xhm	       	jsr >hc_xhm    		; Execute Host Command ($26)
iv_dhwd        	jsr >hc_host_w_done  	; Terminate DMA write from host ($28)
iv_kernel_ack  	jsr >hc_kernel_ack  	; kernel acknowledge ($2a)
iv_sys_call  	nop		  	; kernel system call ($2c)
		nop

		if *>DEGMON_L
		    fail 'vectors.asm: interrupt handlers run into DEGMON'
		endif

iv_wasted	set DEGMON_L-*		; available for user host commands

		dup iv_wasted
			nop
		endm

degmon_l

