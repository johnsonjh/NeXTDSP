; hmdispatch,asm - dispatch table (for address stability)
;
;; Copyright 1989, NeXT Inc.
;; J.O. Smith
;;
;; Included by allocsys.asm.
;; Associated with hmlib.asm and hmlib_mk.asm
;;
;; Modification history
;; --------------------
;; 04/26/90/jos - Added parametric sound support (four variables)
;;
     xdef hm_first
     xdef hm_say_something,hm_idle
     xdef hm_peek_x
     xdef hm_peek_y,hm_peek_p,hm_peek_r,hm_peek_n
     xdef hm_poke_x,hm_poke_y,hm_poke_p,hm_poke_r,hm_poke_n
     xdef hm_fill_x,hm_fill_y,hm_fill_p
     xdef hm_get_time,hm_set_time,hm_set_tinc
     xdef hm_halt,hm_set_start,hm_go
     xdef hm_tmq_room,hm_tmq_lwm_me,hm_block_on,hm_block_off
     xdef hm_execute,hm_execute_hm
     xdef hm_jsr,hm_save_state,hm_load_state,hm_done_int,hm_done_noint
     xdef hm_clear_dma_hm,hm_dm_off,hm_dm_on,hm_host_r
     xdef hm_host_r_done,hm_host_w,hm_host_rd_on
     xdef hm_host_rd_off,hm_host_wd_on,hm_host_wd_off
     xdef hm_dma_rd_ssi_on,hm_dma_rd_ssi_off,hm_dma_wd_ssi_on,hm_dma_wd_ssi_off
     xdef hm_hm_first,hm_hm_last
     xdef hm_midi_msg,hm_high_srate,hm_low_srate
     xdef hm_blt_x,hm_blt_y,hm_blt_p,hm_sine_test,hm_host_w_dt
     xdef hm_abort,hm_main_done,hm_service_tmq,hm_stderr
     xdef hm_write_data_switch,hm_service_write_data
     xdef hm_host_wd_done,hm_host_rd_done,hm_host_w_swfix,hm_adc_loop
     xdef hm_open_paren,hm_close_paren
     xdef hm_block_tmq_lwm,hm_unblock_tmq_lwm
     xdef hm_set_dma_r_m,hm_set_dma_w_m
     xdef loc_xhmta_return_for_tzm,loc_x_dma_wfp
     xdef hm_last

hm_first	  ;  set *		; Address of first dispatch

; DMA  (r = "read", w = "write", rd = "read data", wd = "write data")

hm_clear_dma_hm	    jmp >clear_dma_hm0	     ;* initialize all dma to none

hm_dm_off	    jmp >dm_off0	     ; turn off DSP messages
hm_dm_on	    jmp >dm_on0		     ; turn on DSP messages
hm_host_r	    jmp >host_r0	     ; host wants a block
hm_host_r_done	    jmp >host_r_done0        ; host got its block
hm_host_w	    jmp >host_w0	     ;* host has a block for us
;hm_host_w_done is not necessary since it's only called by host command

hm_host_rd_on	    jmp >host_rd_on0	     ;* host ready to supply read data
hm_host_rd_off	    jmp >host_rd_off0	     ;* host has no more read data
hm_host_wd_on	    jmp >host_wd_on0	     ;* host ready to take write data
hm_host_wd_off	    jmp >host_wd_off0	     ;* host does not want write data

hm_dma_rd_ssi_on    bset #B__SSI_RD_ENABLE,x:X_DMASTAT
 		    rts		     	     ;* sum in read data from ssi
hm_dma_rd_ssi_off   bclr #B__SSI_RD_ENABLE,x:X_DMASTAT   
		    rts		   	     ;* no read data from ssi (default)
hm_dma_wd_ssi_on    jmp >dma_wd_ssi_on0      ;* forward write data to ssi
hm_dma_wd_ssi_off   jmp >dma_wd_ssi_off0     ;* no write data to ssi

; NON-DMA IO
hm_peek_x	    jmp >peek_x0	;* read single word of x memory
hm_peek_y	    jmp >peek_y0	; read single word of y memory
hm_peek_p	    jmp >peek_p0	; read single word of p memory
hm_peek_r	    jmp >unwritten_subr	; read DSP register
hm_peek_n	    jmp >unwritten_subr	; peek multiple words
hm_poke_x	    jmp >poke_x0	;* write single word of x memory
hm_poke_y	    jmp >poke_y0	;* write single word of y memory
hm_poke_p	    jmp >poke_p0	;* write single word of p memory
hm_poke_r	    jmp >unwritten_subr	; write DSP register
hm_poke_n	    jmp >poke_n0	;* poke multiple words
hm_fill_x	    jmp >fill_x0	;* set x memory block to a constant
hm_fill_y	    jmp >fill_y0	;* set y memory block to a constant
hm_fill_p	    jmp >fill_p0	;* set p memory block to a constant
hm_blt_x	    jmp >blt_x0		;* move x memory block
hm_blt_y	    jmp >blt_y0		;* move y memory block
hm_blt_p	    jmp >blt_p0		;* move p memory block

; CONTROL				';*' means done
; -------
hm_say_something    jmp >say_something0	;* request "I am alive" DSP message
hm_echo		    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_idle		    jmp >idle_0		;* abort execution and chase tail
hm_reset_soft	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_reset_ipr	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_get_time	    jmp >get_time0	;* get current value of tick counter
hm_set_time	    jmp >set_time0	;* set current value of tick counter
hm_set_tinc	    jmp >set_tinc0	;* set current value of tick increment
hm_set_break	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_clear_break	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_set_start	    jmp >set_start_0	;* set start address
hm_go		    jmp >go_0		;* go at start address
hm_step		    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_tmq_room	    jmp >tmq_room0	;* determine no. of free words in TMQ
hm_tmq_lwm_me	    bset #B__TMQ_LWM_ME,y:Y_RUNSTAT
		    rts			;* enable message on TMQ low-water mark
hm_hms_room	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_block_on 	    bset #B__DMQ_LOSE,y:Y_RUNSTAT
	            rts			;* set run status to allow blocking
hm_block_off	    bclr #B__DMQ_LOSE,y:Y_RUNSTAT
	            rts			;* set run status to stop blocking
hm_done_int         bset #B__HM_DONE_INT,y:Y_RUNSTAT
	       	    rts			;* interrupt host when host msg done
hm_done_noint       bclr #B__HM_DONE_INT,y:Y_RUNSTAT
		    rts			;* interrupt on host msg done
hm_trace_on	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_trace_off	    jmp >unwritten_subr	;* AVAILABLE SLOT
hm_execute	    jmp >execute_0	;* execute arbitrary DSP instruction
hm_execute_hm	    jmp >execute_hm0	; execute host message directly
hm_jsr		    jmp >jsr_0		;* execute arbitrary subroutine
hm_save_state	    jmp >unwritten_subr	; host wants a total state dump
hm_load_state	    jmp >unwritten_subr	; host wants to swap in new state
hm_hm_first	    jmp >hostm_first0	;* return start address of this table
hm_hm_last	    jmp >hostm_last0	;* return end address of this table
hm_midi_msg	    jmp >midi_msg0	;* receive MIDI message from host
hm_high_srate	    bclr #B__HALF_SRATE,y:Y_RUNSTAT
	            rts			;* select high sampling rate
hm_low_srate	    bset #B__HALF_SRATE,y:Y_RUNSTAT
	            rts			;* select low sampling rate

hm_host_wd_done	    rts			; obsolete. Not used at all in new
		    nop			; protocol.  Called in old protocol
					; but not actually needed.
					; host_w_done clears pending always

hm_host_rd_done	    jmp >host_rd_done0	; Tell DSP last DMA was an RD buffer

hm_halt		    DEBUG_HALT		; Force break (fall into DEGMON)
		    rts

hm_sine_test	    jmp >sine_test0	;* initiate test sinusoid
hm_host_w_dt	    jmp >host_w_dt0	;* request RD defferred termination
hm_host_w_swfix	    jmp >host_w_swfix0	;* toss 1st chunk of DMA transfer
hm_adc_loop	    jmp >unwritten_subr	;* A/D conversion via serial port
hm_open_paren 	    jmp >open_paren0	;* begin atomic TMQ block
hm_close_paren 	    bclr #B__TMQ_ATOMIC,x:X_DMASTAT ;* end atomic TMQ block
		    rts
hm_block_tmq_lwm    jmp >block_tmq_lwm0
hm_unblock_tmq_lwm  bclr #B__BLOCK_TMQ_LWM,y:Y_RUNSTAT
		    rts			;* disable blocking
hm_set_dma_r_m      jmp >set_dma_r_m0   ;* set M index register for DMA read
hm_set_dma_w_m      jmp >set_dma_w_m0   ;* set M index register for DMA write

hm_last	 	    ; set *		; Address of last host message dispatch

; PADDING FROM WHICH NEW HOST MESSAGES SHOULD BE TAKEN
; (i.e. subtract 1 from count below for each new host message above
; in order to keep subroutine entry points stable).

	dup 11
	dc 0	; for jmp opcode
	dc 0	; for long jump address
	endm

; PARAMETRIC SOUND SUPPORT
; ------------------------
loc_sound_par_1	dc 0
loc_sound_par_2	dc 0
loc_sound_par_3	dc 0
loc_sound_par_4	dc 0

; INDIRECT JUMPS USED BY ASM CODE
; -------------------------------
loc_xhmta_return_for_tzm	; jsr here from top of tick loop on TZM_PENDING
		if !AP_MON
		    jmp >xhmta_return_for_tzm
		else
		    dc 0
		    dc 0
		endif


; SYSTEM VARIABLE LOCATIONS COMPILED INTO C SOFTWARE
; --------------------------------------------------
loc_x_dma_wfp	
		if !AP_MON
		    dc X_DMA_WFP
		else
		    dc 0
		endif

loc_unused	dc 0	; Want to use an even number of locations

	
; SUBROUTINE ENTRY POINTS COMPILED INTO C SOFTWARE
; ------------------------------------------------
	if AP_MON
hm_abort	    	jmp >abort_now
hm_stderr	    	jmp >stderr
hm_service_tmq	    	jmp >unwritten_subr
hm_write_data_switch 	jmp >unwritten_subr
hm_main_done	    	jmp >main_done1
hm_reset_ap	    	jmp >unwritten_subr
hm_service_write_data 	jmp >unwritten_subr

	else	; !AP_MON:

hm_abort	    	jmp >abort1
hm_stderr	    	jmp >stderr
hm_service_tmq	    	jmp >service_tmq1
hm_write_data_switch   	jmp >write_data_switch1
hm_main_done	    	jmp >unwritten_subr
hm_reset_ap	    	jmp >unwritten_subr
hm_service_write_data  	jmp >service_write_data1

	endif	; !AP_MON

; SOUND KIT BOOTSTRAP PROGRAM LOCATIONS (6 WORDS)
; -----------------------------------------------
;
; The following gets loaded by the sound library (C functions with prefix
; "SND") into external memory before feeding a "DSP core" .snd file to the
; DSP. This bootstrap loaded then loads booter.asm (in sound library) which
; in turn loads general user DSP code. It assumes r0 is set to zero and x0 has 
; the count on entry
;
sound_booter_p_mem_loop
	dc	$06c400
	dc	$003ffe
	dc	$0aa980
	dc	$003ffc
	dc	$08586b
	dc	$0c0000
;
;Which really is:
;
;	org	p:XRAMHI-6
;p_mem_loop
;	do	x0,_done
;_get
;	jclr	#HRDF,x:HSR,_get
;	movep	x:HRX,p:(r0)+
;_done
;	jmp	reset

; This is the highest location in external memory.

