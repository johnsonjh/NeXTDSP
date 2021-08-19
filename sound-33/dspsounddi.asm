;
;	dspsounddi.asm - dsp-initiated dma in both directions
;	Created from jos example di_dma.asm
;	Copyright 1990 NeXT, Inc.
;
;Typical use:
;
;		include	"dspsounddi.asm"
;	main
;		<user initialization>
;		jsr	start
;	loop
;		jsr	getHost		;called zero or more times
;		<some calculations>
;		jsr	putHost		;called one or more times
;		jcs	exit		;in case host aborts
;		<some loop condition ??>
;		j??	loop		;
;	exit
;		<user shutdown, i.e. ramp down for graceful abort>
;		jmp	stop
;
;The including code must set up LEADPAD and TRAILPAD equates.  Theses should be zero
;if writing to the host and the following if streaming to soundout:
;LEADPAD	equ	2	  ;number of buffers before sound begins
;TRAILPAD	equ	4	  ;number of buffers after sound ends
;This is necessary because the sound driver requires lead pad because it starts soundout
;DMA right away - while the DSP is still producing the first
;real buffer.
;
;
;Modification History
;	07/16/90/mtm	Free up r6, calc buffer start addresses on the fly.
;	07/26/90/mtm	Remove LEADPAD and TRAILPAD equates.
;			Don't include i/o code files.

	include	"portdefs.asm"

;
;------------------------- DSP <--> host communication
;
SC_W_REQ	equ	$020002	  ;"Sys call" requesting DMA write on chan 2
DM_R_REQ	equ	$050001	  ;"DSP message" requesting DMA read on chan 1
DM_W_REQ	equ	$040002	  ;message requesting DMA write on channel 2 

;
;------------------------- Variable locations
;
x_sFlags	equ	$00ff	;flags
B_DMA_DONE	equ	0	;  indicates dma complete
B_SYS_CALL	equ	1	;  indicates sys call has been received
RUNNING		equ	2	;  not the first putHost since boot
y_sDmaSize	equ	$00ff	;dma size sent to us by user (4K or less)

x_sSysArg	equ	$00fe	;sys_call arg
y_sReadPtr	equ	$00fe	;current read buffer pointer

x_sReadBuf	equ	$00fd	;start of read buffer
y_sWriteBuf	equ	$00fd	;start of write buffer

;
;------------------------- Interrupt vectors
;
	org	p:VEC_RESET
	jmp	>reset

	org	p:VEC_R_DONE
	bset	#B_DMA_DONE,x:x_sFlags
	nop

	org	p:VEC_W_DONE
	bset	#B_DMA_DONE,x:x_sFlags
	nop

	org	p:VEC_SYS_CALL
	jsr	>sys_call

	org	p:VEC_END

; sys_call - field a request from the kernel
; A "system call" is a host command followed by one int written to the DSP.
; In the future, the int may specify more ints to follow.
; arg = 24bits = (8,,16) = (op,datum)
; 	where op = 1 for read and 2 for write
;	and datum = 1 for "software fix" needed for DMA chips less than rev. 313
;		The fix tosses the first 16 bytes of the DMA write (to the DSP)
;		into the bit bucket.
;
sys_call
	jclr	#HRDF,x:HSR,sys_call	;buzz until int received
	movep	x:HRX,x:x_sSysArg	;int specifying operation
	bset	#B_SYS_CALL,x:x_sFlags  ;set flag to say we got this
	rti

;
;---------------------- EXPORTED ROUTINES ----------------------
;
	
;
;start - enable interrupts and indicate to host that we are ready.
;  Destroys register A and B and sets the SR.
;
start
	move	#>XRAMLO,a
	move	a1,x:x_sReadBuf		;initialize read buffer
	move	a1,y:y_sReadPtr		;initialize read buffer pointer
	move	y:y_sDmaSize,b
	add	b,a
	move	a1,y:y_sWriteBuf	;initialize write buffer
	move	a1,r7			;initialize write buffer pointer
	clr	a
	move	a,x:x_sFlags		;clear flags
	bset    #HCIE,x:HCR		;host command interrupts
	move	#0,sr			;enable interrupts
	rts

;
;flush - flush the current dma output buffer.
;
flush	clr	a			;pad partial buffer with 0s
	move	r7,b			;current output sample pointer
	move	y:y_sWriteBuf,X0	;write-buffer start address
	sub	x0,b			;subtract x0 from b to get ptr-base
	jle	_done			; if 0, flush is done or was unnecessary
	jsr	putHost			;pad last buffer with 0s
	jmp	flush			;see if that did it
_done
	rts

;
;stop - flush last buffer, signal to host and terminate execution.
;
stop
	clr	a
	if	TRAILPAD
	do	#TRAILPAD,_trail	;send trailing buffers
	endif
	jsr	putHost
	jsr	flush
	nop
_trail
	movep	#>$000018,x:HCR		;indicate we have halted (set HF2 and HF3)
idle	jclr	#HRDF,x:HSR,idle	;buzz forever here
	movep	x:HRX,a			; soak up anything written by host
	jmp	idle			; buzz

;
;putHost - puts a accumulator sample to the output stream. 
; A whole buffer is accumulated before requesting DMA. Blocks until
; DMA is successful. Must be called once for each sample in a
; sample-frame.
; On return, the carry bit indicates that the program should terminate;
; normally it is clear.
;
; This routine destroys the b and x0 registers.
; It also uses and depends on r7.
; Assumes r7 is initially set to point to output buffer.
; External ram is used for buffering.
;
putHost
	btst	#RUNNING,x:x_sFlags
	jcs	_putHost
	bset	#RUNNING,x:x_sFlags
	if	LEADPAD
	do	#LEADPAD,_putHost		;send leading buffers 
	jsr	_putHost
	jsr	flush
	nop
	endif
_putHost
	move	a,y:(r7)+			;put done! check for buffer full:
	move	y:y_sWriteBuf,b			;write-buffer address
	move	y:y_sDmaSize,x0			;buffer length
	add	x0,b				;add to get last address + 1
	move	r7,x0				;current output sample pointer
	sub	x0,b   y:y_sWriteBuf,x0		;subtract x0 from b to get end+1 - ptr
	jgt	_continue			; if positive, keep cruising, else dma
	;; 
	;; Send a buffer to the host via "DSP-initiated DMA"
	;; 
	move	x0,r7				;reset to buffer start
	bclr	#B_DMA_DONE,x:x_sFlags		;reset end-of-dma indicator bit
_htdeBuzz
	jclr	#HTDE,x:HSR,_htdeBuzz 		;wait until we can write host
	movep	#DM_R_REQ,x:HTX			;send "read request" DSP message
_hf1Buzz
	jclr	#HF1,x:HSR,_hf1Buzz 		;HF1 means DMA is set up to go
	move	y:y_sDmaSize,b
	do	b1,_dmaLoop			;service the DMA transfer
_dmaBuzz
	jclr  	#HTDE,x:HSR,_dmaBuzz		;DMA hardware handshake
	movep  	y:(r7)+,x:HTX			;send values
_dmaLoop
_doneBuzz
	btst	#B_DMA_DONE,x:x_sFlags
	jcs	_endDMA				;terminating host command sets us free
	jclr  	#HTDE,x:HSR,_doneBuzz		;wait until we can write host
	movep	#0,x:HTX			;send zeros until DMA completion 
	jmp	_doneBuzz
_endDMA
	jset	#HF1,x:HSR,_endDMA		;wait for HF1 to go low (should be)
	move	x0,r7				;reset to buffer start for refill
_continue
	btst	#HF0,x:HSR			;set for "abort"
	rts

;
;getHost - returns the next sample from the input stream in register A1.
; When input buffer is empty, blocks until DMA is successful. 
; Must be called once for each sample in a sample-frame.  
;
; This routine destroys the b and x0 registers.
; Also destroys r6.
; Assumes y_sReadPtr is initially set to point to input buffer.
; External ram is used for buffering.
;
getHost
	move   	x:x_sReadBuf,x0			;read-buffer start address
	move 	y:y_sReadPtr,r6			;current pointer
	move	r6,b
	sub	x0,b				;b-x0 == ptr-base
	jgt	nextSamp			;positive if we're working on a buffer
	;; 
	;; Get a buffer from the host via "DSP-initiated DMA"
	;; 
	bclr	#B_DMA_DONE,x:x_sFlags		;reset end-of-dma indicator bit
	bclr	#B_SYS_CALL,x:x_sFlags		;reset "sys-call pending" bit
_htdeBuzz
	jclr	#HTDE,x:HSR,_htdeBuzz 		;wait until we can talk to host
	movep	#DM_W_REQ,x:HTX			;send "write request" DSP message
	move	#SC_W_REQ,x0			;Expected sys_call
_hcBuzz
	btst	#B_SYS_CALL,x:x_sFlags
	jcc	_hcBuzz				;wait until "sys_call" host command
	move	x:x_sSysArg,b 			; single int argument to host command
	cmp	x0,b				; see if it's what we expected.
	jne	_hcBuzz				; if not, ignore it
	bclr	#B_SYS_CALL,x:x_sFlags		;got it
	move	y:y_sDmaSize,b
	do	b1,_dmaLoop			;service the DMA transfer
_dmaBuzz
	jclr	#HRDF,x:HSR,_dmaBuzz		;DMA hardware handshake
	movep	x:HRX,y:(r6)+			;receive values
_dmaLoop
	move	x:x_sReadBuf,x0			;read-buffer start address
	move	x0,r6				;reset to buffer start for consumption
_doneBuzz
	btst	#B_DMA_DONE,x:x_sFlags
	jcs	nextSamp			;terminating host command sets us free
	jclr	#HRDF,x:HSR,_doneBuzz		;if the DMA is still sending to us
	movep	x:HTX,x0			;receive it to drain its pipe
	jmp	_doneBuzz
nextSamp
	move	y:(r6)+,a			;the requested read
	move	x:x_sReadBuf,b			;read-buffer address
	move	y:y_sDmaSize,x0			;buffer length
	add	x0,b				;add to get last address + 1
	move	r6,x0				;current output sample pointer
	move	r6,y:y_sReadPtr
	sub	x0,b   x:x_sReadBuf,x0		;subtract x0 from b to get end+1 - ptr
	jgt	_continue			; if positive, keep cruising, else dma
	move	x0,y:y_sReadPtr			;reset to buffer start to trigger refill
_continue
	rts

;
; reset - first thing executed when DSP boots up
;
reset
	movec   #6,omr			;data rom enabled, mode 2 = "normal"
	bset    #0,x:PBC		;enable host port
	bset	#3,x:PCDDR		;   pc3 is an output asserting 0 to enable
	bclr	#3,x:PCD		;   external DSP ram on very early machines
	movep   #>$000000,x:BCR		;no wait states needed for the external sram
        movep   #>$00B400,x:IPR  	;intr levels: SSI=2, SCI=1, HOST=0
_reset
	jclr	#HRDF,x:HSR,_reset	;get dma buffer size from host user program
	movep	x:HRX,y:y_sDmaSize
beginUsr
	jmp	main

;
;end of dspsounddi.asm
;
