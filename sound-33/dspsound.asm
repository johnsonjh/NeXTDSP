;
;	dspsound.asm - complex dma mode version
;	Written by Lee Boynton
;	Copyright 1988 NeXT, Inc.
;
;Typical use:
;
;		include	"dspsound.asm"
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

	include	"portdefs.asm"

;
;------------------------- Equates
;
LEADPAD		equ	2		;number of buffers before sound begins
TRAILPAD	equ	4		;number of buffers after sound ends
DM_R_REQ	equ	$050001		;message -> host to request dma

;
;------------------------- Variable locations
;
x_sFlags	equ	$00ff		;dspsound flags
DMA_DONE	equ	0		;  indicates that dma is complete
RUNNING		equ	1		;  not the first getHost since boot
y_sCurSamp	equ	$00ff		;current offset in buffer

x_sPutVal	equ	$00fe		;last value to putHost
y_sDmaSize	equ	$00fe		;dma size

;
;------------------------- Interrupt vectors
;
	org	p:$0000
	jmp	reset
;
	org	p:VEC_R_DONE
	bset	#DMA_DONE,x:x_sFlags
;

;
;---------------------- EXPORTED ROUTINES ----------------------
;

	org	p:$0040

;
;start - enable interrupts and indicate to host that we are ready.
;  Destroys register A1 and sets the SR.
;
start
	move	#>XRAMLO,r7
	clr	a
	move	a,x:x_sFlags		;clear flags
	move	a,y:y_sCurSamp		;curSamp = 0;
	bset    #HCIE,x:HCR		;host command interrupts
	move	#0,sr			;enable interrupts
	rts
;
;flush - flush the current dma output buffer.
;
flush
	move	x:x_sPutVal,a
	move	y:y_sCurSamp,b
	tst	b
	jeq	_flush1			;if (curSamp != 0) {
	jsr	putHost			;   pad last buffer
	jmp	flush
_flush1
	rts

;
;stop - send trailing buffers, signal to host and terminate execution.
;
stop
	clr	a
	do	#TRAILPAD,_trail	; send a trailing buffers 
	jsr	putHost			;
	jsr	flush
	nop
_trail
	movep	#>$000018,x:HCR		;indicate that we have halted
_grave
	jclr	#HRDF,x:HSR,_grave
	movep	x:HRX,a			;eat shit
	jmp	_grave			;and kind of tune out the world



;
;putHost - puts a sample to the output stream. Does not return until
; successful. Note that this must be called once for each sample in a
; sample-frame.
; On return, the carry bit indicates that the program should terminate;
; normally it is clear.
;
;This routine destroys the b register, and may affect a0 and a2.
;It also uses and depends on r7.
;
putHost
	move	a,x:x_sPutVal
	btst	#RUNNING,x:x_sFlags
	jcs	_putHost
	bset	#RUNNING,x:x_sFlags	;
	do	#LEADPAD,_putHost	; send leading buffers 
	jsr	_putHost			;
	jsr	flush
	nop
_putHost
	move	a,x:(r7)+
	move	y:y_sCurSamp,b
	move	#>1,a			;
	add	a,b			;
	move	b,y:y_sCurSamp		;curSamp++
	move	y:y_sDmaSize,a
	cmp	b,a
	jgt	_exit			;if (curSamp < bufSize) exit
	move	#>XRAMLO,r7
_beginBuf
	jclr	#HTDE,x:HSR,_beginBuf
	movep	#DM_R_REQ,x:HTX		;    send "DSP_dm_R_REQ" to host
_ackBegin
	jclr	#HF1,x:HSR,_ackBegin	;    wait for HF1 to go high
	move	y:y_sDmaSize,b
	do	b,_prodDMA
_send
	jclr	#HTDE,x:HSR,_send
	movep	x:(r7)+,x:HTX			;    send values
_prodDMA
	btst	#DMA_DONE,x:x_sFlags
	jcs	_endDMA
	jclr	#HTDE,x:HSR,_prodDMA
	movep	#0,x:HTX		;send garbage until noticed
	jmp	_prodDMA
_endDMA
	bclr	#DMA_DONE,x:x_sFlags	;be sure we know we are through
_ackEnd
	jset	#HF1,x:HSR,_ackEnd	;wait for HF1 to go low
	move	#>XRAMLO,r7
	clr	b
	move	b,y:y_sCurSamp		;curSamp = 0
_exit
	move	x:x_sPutVal,a
	btst	#HF0,x:HSR		;
	rts				;

;
;getHost - returns the next sample from the input stream in register a1.
; Waits until there is a sample available.
;
getHost
	jset	#HF0,x:HSR,_exit	;
	jclr	#HRDF,x:HSR,getHost
	movep	x:HRX,a
_exit
	btst	#HF0,x:HSR		;
	rts

;
;reset - initialize and start program from a hard reset (i.e. just 
; bootstrapped). Note that this routine just drops through to the user
; code (this file is assumed to be included
;
reset
        movec   #6,OMR			;data rom enabled, mode 2
	bset    #0,x:PBC		;host port
	movep   #>$0001F7,x:PCC		;both serial ports (SC0 not available)
	bset	#3,x:PCDDR		;   pc3 is an output with value
	bclr	#3,x:PCD		;   zero to enable the external ram
	movep   #>$000000,x:BCR		;no wait states on the external sram
        movep   #>$00B400,x:IPR  	;intr levels: SSI=2, SCI=1, HOST=0
_reset
	jclr	#HRDF,x:HSR,_reset
	movep	x:HRX,y:y_sDmaSize	;get dma buffer size from host
beginUsr
	jmp	main

;
;end of dspsound.asm
;
