; hmlib_mk.asm - included by hmlib.asm for !AP_MON case (Music Kit Monitor)
;
;
; *****************************************************************************
; *****************************************************************************
; ********************** ROUTINES CALLED BY MKMON ONLY ************************
; *****************************************************************************
; *****************************************************************************
;
; ******************************* CONTROL ************************************

; ================================================================
; set_tinc0 - set current tick increment
; ARGUMENTS (in the order written by the host)
;   hi-order 24-bit word of new tinc
;   lo-order 24-bit word of new tinc
;
set_tinc0 	move y:(R_I1)+,X0	; low-order word
	        move y:(R_I1)+,X1	; high-order word
		move X,l:L_TINC		; set it
	        rts
;
; ******************************* QUERY ************************************
;
; ================================================================
; tmq_room0 - determine free space in timed message queue (in words)
tmq_room0 	jsr measure_tmq_room	; jsrlib.asm ... result in A
		move #DM_TMQ_ROOM,X0	; opcode
		jsr dspmsg
	        rts
; ================================================================
; get_time0 - get current time
;; Current time is sent via normal DSP message. NOT OUT OF BAND.
get_time0 	jsr send_time	; send out the current time
	        rts
; ================================================================
; set_time0 - set current time
; ARGUMENTS (in the order written by the host)
;   hi-order 24-bit word of new time
;   lo-order 24-bit word of new time
;
set_time0 	move y:(R_I1)+,X0	; low-order word
	        move y:(R_I1)+,X1	; high-order word
		move X,l:L_TICK		; set it
	        rts
;
; ******************************* DMA IO ************************************
;
; ================================================================
; clear_dma_hm - init state of sound buffers to turned off and clear condition.
;		 Buffers are not zeroed. Use hm_fill_y.
;
clear_dma_hm0  	move R_I1,x:X_SAVED_R_I1_HMLIB ; only R_I1 needs saving
	     	jsr clear_dma_ptrs ; jsrlib - flush and reset DMA pointers
		jsr wd_buffer_clear ; zero out write-data buffers
		if READ_DATA
		   jsr rd_buffer_clear ; zero out read-data buffers
		endif
		move x:X_SAVED_R_I1_HMLIB,R_I1
	        rts
; ================================================================
; host_rd_on0 - host ready to supply read data
host_rd_on0 	move y:(R_I1)+,A	; channel number
		move #>1,X0		; channel 1
		cmp X0,A		; only channel 1 supported now
		jeq hron_ok
			DEBUG_HALT
hron_ok		bset #B__HOST_RD_ENABLE,x:X_DMASTAT
		rts
; ================================================================
; host_rd_off0 - host has no more read data
host_rd_off0 	move y:(R_I1)+,A	; channel number
		move #>1,X0		; channel 1
		cmp X0,A		; only channel 1 supported now
		jeq hroff_ok
			DEBUG_HALT
hroff_ok        bclr #B__HOST_RD_ENABLE,x:X_DMASTAT
		rts

; Write-data (WD) enables
;
; Before WD is enabled, the write-data buffer ring is freely written
; by the orchestra loop without waiting for buffers to be read by the
; host and without sending DMA requests to the host. When WD is enabled,
; a DMA request goes out immediately for the currently filling buffer.
;
; When a host_wd_off is received to disable WD, the effect is to
; inhibit the generation of future DMA requests (see write_data_buffer_out
; in jsrlib.asm).
;
; ================================================================
; host_wd_on0 - host ready to take write data
host_wd_on0 	move y:(R_I1)+,A	; channel number
		move #>1,X0		; channel 1
		cmp X0,A		; only channel 1 supported now
		jeq hwon_ok
			DEBUG_HALT
hwon_ok		
		move R_I1,x:X_SAVED_R_I1_HMLIB ; only R_I1 not saved
		jsr wd_buffer_clear
	        bset #B__HOST_WD_ENABLE,x:X_DMASTAT
		jscc wd_dma_request
		move x:X_SAVED_R_I1_HMLIB,R_I1
		rts

; ================================================================
; host_wd_off0 - host does not want write data
host_wd_off0 	move y:(R_I1)+,A	; channel number
		move #>1,X0		; channel 1
		cmp X0,A		; only channel 1 supported now
		jeq hwoff_ok
			DEBUG_HALT
hwoff_ok 	bclr #B__HOST_WD_ENABLE,x:X_DMASTAT
		rts
; ================================================================
; dma_wd_ssi_on0 - forward write data to ssi
dma_wd_ssi_on0 	jsr start_ssi_write_data
 		rts
; ================================================================
; dma_wd_ssi_off0 - no write data to ssi
dma_wd_ssi_off0	jsr stop_ssi_write_data
		rts
;
; **************************** NON-DMA IO ************************************
; See hmlib.asm for peek routines
;
; ================================================================
; poke_x0 - write a single word into x memory
; ARGUMENTS (in the order written by the host)
;   value     - word to write at address
;   address   - address to poke
poke_x0 	move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
	        move X0,x:(R_I2)	; poke
	        rts
; ================================================================
; poke_y0 - write a single word into y memory
; ARGUMENTS (in the order written by the host)
;   value     - word to write at address
;   address   - address to poke
poke_y0 	move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
	        move X0,y:(R_I2)	; poke
	        rts
; ================================================================
; poke_p0 - write a single word into p memory
; ARGUMENTS (in the order written by the host)
;   value     - word to write at address
;   address   - address to poke
poke_p0 	move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
	        movem X0,p:(R_I2)	; poke
	        rts
; ================================================================
; poke_n0 - multi-word poke, space passed explicitly
; Prior to call, the array to be transferred has been pushed onto the 
; HMS in NATURAL order (i.e. do not push the array on backwards).
;
; ARGUMENTS (in the order written by the host)
;   count     - number of words to poke
;   skip      - skip factor (use positive number)
;   address   - *** last *** address to poke (transfer is in REVERSE order)
;   space     - memory space of poke
	        remember 'poke_n is for compacting TMQ. Put space code in MSB?'
poke_n0 	move #pn_tab,R_O	; table of transfer words
		move y:(R_I1)+,N_O	; space (xylp) = (1234)
		move y:(R_I1)+,R_I2	; last address
		move y:(R_I1)+,N_I2	; skip factor
		move p:(R_O+N_O),X0	; transfer instruction
		move X0,p:pn_xfer_ins	; self-modifying code
		do y:(R_I1)+,pn_loop
			move y:(R_I1)+,A	; get next word
pn_xfer_ins		move A,x:(R_I2)-N_I2	; deposit it
pn_loop
	        rts

pn_tab		nop			; space 0
		move A,x:(R_I2)-N_I2	; space 1 = x
		move A,y:(R_I2)-N_I2	; space 2 = y
		move A,l:(R_I2)-N_I2	; space 3 = l (should not be used)
		move A,p:(R_I2)-N_I2	; space 4 = p
; ================================================================
; fill_x0 - set x memory block to a constant
; ARGUMENTS (in the order written by the host)
;   count     - number of elements
;   value     - word to write at address through address+count-1
;   address   - first address to write
fill_x0	        move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
		do y:(R_I1)+,fill_x0_loop		
		        move X0,x:(R_I2)+	; poke
fill_x0_loop		
	        rts
; ================================================================
; fill_y0 - set y memory block to a constant
fill_y0 	move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
		do y:(R_I1)+,fill_y0_loop		
		        move X0,y:(R_I2)+	; poke
fill_y0_loop		
	        rts
; ================================================================
; fill_p0 - set p memory block to a constant
fill_p0 	move y:(R_I1)+,R_I2	; memory address
	        move y:(R_I1)+,X0	; value to poke
		do y:(R_I1)+,fill_p0_loop		
		        move X0,p:(R_I2)+	; poke
fill_p0_loop		
	        rts

; ================================================================
; blt_x0 - block transfer in x memory
;
; ARGUMENTS (in the order written by the host)
;   count	- number of elements
;   source    	- first address to read
;   sourceskip	- skip factor for source block (1 is typical)
;   dest      	- first address to write
;   destskip	- skip factor for dest block (1 is typical)
;
; NOTES: 
;	If the source and destination blocks overlap, and the skip factors
;	are positive, we must have source>dest.  In other words, normal
;	overlapping block transfers must move to a smaller address.
;
;	However, by setting the skip factors negative and the source and
;	dest address to the last element of the desired blocks, a forward
;	overlapping block transfer can be accomplished.  
;
;	Finally, by setting one skip factor negative and the other positive, a
;	block of DSP memory can be reversed in order.
;
blt_x0 		move y:(R_I1)+,N_I2	; destination skip factor
	        move y:(R_I1)+,R_I2	; destination memory address
	        move y:(R_I1)+,N_O	; source skip factor
	        move y:(R_I1)+,R_O	; source memory address
		do y:(R_I1)+,blt_x0_loop		
		        move x:(R_O)+N_O,X0
		        move X0,x:(R_I2)+N_I2
blt_x0_loop		
	        rts
; ================================================================
; blt_y0 - block transfer in y memory
blt_y0 		move y:(R_I1)+,N_I2	; destination skip factor
	        move y:(R_I1)+,R_I2	; destination memory address
	        move y:(R_I1)+,N_O	; source skip factor
	        move y:(R_I1)+,R_O	; source memory address
		do y:(R_I1)+,blt_y0_loop		
		        move y:(R_O)+N_O,X0
		        move X0,y:(R_I2)+N_I2
blt_y0_loop		
	        rts
; ================================================================
; blt_p0 - block transfer in p memory
blt_p0 		move y:(R_I1)+,N_I2	; destination skip factor
	        move y:(R_I1)+,R_I2	; destination memory address
	        move y:(R_I1)+,N_O	; source skip factor
	        move y:(R_I1)+,R_O	; source memory address
		do y:(R_I1)+,blt_p0_loop		
		        move p:(R_O)+N_O,X0
		        move X0,p:(R_I2)+N_I2
blt_p0_loop		
	        rts
; ================================================================
; sine_test0 - perform sine test
sine_test0 	move y:(R_I1)+,X0	; duration of test in output buffers
		if ASM_RESET
		  jmp sine_test		; resets back to idle loop when done
		else
		  jmp unwritten_subr
		endif

; ================================================================
; host_w_dt0 - host write deferred termination (read-data only)
		remember 'need to add support for channel number argument'
host_w_dt0 	move y:(R_I1)+,X0	; channel number
		bset #B__HOST_RD_OFF_PENDING,x:X_DMASTAT
		rts

; ================================================================
; host_w_swfix0 - same as host_w0 but with software fix for DMA problem
		; into DSP.  Fix is to throw away the 1st chunk (4 words)
		; of the transfer.

host_w_swfix0 	jsr host_w0		; set up the normal way first.
		movem p:iv_host_rcv,X0	; host_rcv vector, word 1
		movem X0,p:hws_v1	; save it here.  Word 2 is a nop
		clr A #>$BF080,X0
		movem X0,p:iv_host_rcv	; overridden host_rcv vector, word 1
		movem A,p:iv_host_rcv2	; overridden host_rcv vector, word 2
		move #>4,X0		; initial value of countdown
		move X0,p:hws_cnt
		
		rts

hws_v1		dc 0 			; place for host_rcv interrupt vector
hws_cnt		dc 0 			; word count

; ----------------------------------------------------------------
hws_swfix	; temporary DMA receive interrupt handler for throwing
		; away the first chunk (four words) of incoming DMA data

		move A2,x:X_SAVED_A2
		move A1,x:X_SAVED_A1
		move A0,x:X_SAVED_A0
		move X0,x:X_SAVED_X0

		movep x:$FFEB,X0	; toss DMA word into bit bucket
		move p:hws_cnt,A	; countdown four words of this
		move #>1,X0
		sub X0,A
		move A,p:hws_cnt
		jgt hws_rts		; done when zero is reached

		move p:hws_v1,X0	; switch from this handler to normal
		move X0,p:iv_host_rcv	; restore fast DMA interrupt vector
		clr A 
		move A,p:iv_host_rcv2	; second word is a nop
hws_rts
		move x:X_SAVED_A2,A2
		move x:X_SAVED_A1,A1
		move x:X_SAVED_A0,A0
		move x:X_SAVED_X0,X0
		rts

; ================================================================
; midi_msg0 - Receive MIDI message from host.
; 		Running status assumed expanded.
; 		System exclusive not supported.
; ARGUMENT
;   msg       - MIDI message (left-justified if less than 3 bytes)
midi_msg0 	move y:(R_I1)+,X0	; MIDI message
		move X0,x:X_MIDI_MSG	; install in mail box
	        rts
;;
;; To be done by consumer of MIDI message (e.g. MidiReader unit generator):
;;	        clr A #DM_MIDI_MSG,X0
;;		jsr dspmsg
;;
;;	???     should we turn on the 0x80 bit in the opcode for the DSP
;;		driver's benefit?
;;
; ================================================================
; host_rd_done0 - Tell DSP last DMA was an RD buffer
; ARGUMENT
;   chan       - read data channel whose request was satisfied
host_rd_done0 	move y:(R_I1)+,X0	; read data channel (always 1 for now)
		bclr #B__HOST_RD_PENDING,x:X_DMASTAT ; should be per channel
		bclr #B__RD_BLOCKED,y:Y_RUNSTAT ; should be per channel
		bchg #B__READ_RING,x:X_DMASTAT ; should be per channel
			; We can change the above to B__RBUF_PARITY
		rts
; ================================================================
; open_paren0 - begin atomic TMQ block
;
open_paren0	bset #B__TMQ_ATOMIC,x:X_DMASTAT 
		bset #B__TMQ_ACTIVE,y:Y_RUNSTAT ; So we can block if need be
		rts
; ================================================================
; block_tmq_lwm0 - begin blocking when the TMQ is empty
;
block_tmq_lwm0	bset #B__BLOCK_TMQ_LWM,y:Y_RUNSTAT 
		bset #B__TMQ_ACTIVE,y:Y_RUNSTAT ; So we can block if need be
		rts
