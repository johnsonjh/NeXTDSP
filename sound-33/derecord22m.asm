;;; derecord22m.asm
;;; DSP code to listen to serial port and send data in DMA mode.
;;; Author: Robert D. Poor, NeXT Technical Support
;;; Copyright 1989 NeXT, Inc.  Next, Inc. is furnishing this software
;;; for example purposes only and assumes no liability for its use.
;;;
;;; Edit history (most recent edits first)
;;;
;;; 05/11/90/mtm	Converted back to use dspsound.asm.
;;; 04-Apr-90 Mike Minnick: cheap 22K mono version - returns every 4th sample
;;;			    (derecord22m.asm)
;;; 11-Sep-89 Rob Poor: Converted to use dspstream.asm rather than
;;;	dspsound.asm, also eliminating need for portdefs.asm
;;; 07-Sep-89 Rob Poor: Created (based on code by Lee Boynton)
;;;
;;; End of edit history

	include	"dspsound.asm"

DMASIZE		equ	1024
BUFSIZE		equ	DMASIZE*4

main
	jsr	start
	move	#>XRAMLO,r5		;
	move	r5,x:1			;
	move	#>XRAMLO+BUFSIZE,r6	;use external RAM for double buffers
	move	r6,x:0			;
	move	#>BUFSIZE-1,m5		;
	move	#>BUFSIZE-1,m6		;
	move	#4,n6			;send every 4th sample
	move	#1,n5			;
	move	#>$008000,y1		;scale factor for 16 bit output
	movep   #>$0001F7,x:PCC		;both serial ports (SC0 not available)
	movep	#$4100,x:SSI_CRA	; Set up serial port
	movep	#$2a00,x:SSI_CRB	; 	in network mode
	;; most properly, this would be the place to enable interrupts...
	move	#>BUFSIZE,a
	do	a,prime2		;prime the first buffer
prime					;
	jclr	#SSI_RDF,x:SSI_SR,prime ;
	movep	x:SSI_RX,x:(r5)+		;
prime2					;
					;
loop					;while (1) {
	move	x:1,r6			;
	move	x:0,r5			;
	move	r6,x:0			;    swap buffers
	move	r5,x:1			;
beginBuf				;
	jclr	#SSI_RDF,x:SSI_SR,_beg2	;
	movep	x:SSI_RX,x:(r5)+	;
_beg2					;
	jclr	#HTDE,x:HSR,beginBuf	;    start DMA
	movep	#DM_R_REQ,x:HTX		;
_ackBegin				;
	jclr	#SSI_RDF,x:SSI_SR,_ack2	;
	movep	x:SSI_RX,x:(r5)+	;
_ack2					;
	jclr	#HF1,x:HSR,_ackBegin	;
	
	move	#>DMASIZE,a		;
	do	a,senddone		;    send the output buffer
send					;
	jclr	#SSI_RDF,x:SSI_SR,_foo	;
	movep	x:SSI_RX,x:(r5)+	;
_foo					;
	jclr	#HTDE,x:HSR,send	;
	move	x:(r6)+n6,x1		;send every 4th sample
	mpy	x1,y1,a			;
	move	a,x:HTX			;    
senddone				;
					;
_prodDMA				;    handshake the DMA
	btst	#DMA_DONE,x:x_sFlags	;
	jcs	endDMA			;
	jclr	#SSI_RDF,x:SSI_SR,_foo	;
	movep	x:SSI_RX,x:(r5)+	;
_foo					;
	jclr	#HTDE,x:HSR,_prodDMA	;
	movep	#0,x:HTX		;
	jmp	_prodDMA		;
endDMA					;
	bclr	#DMA_DONE,x:x_sFlags	;
_ackEnd					;
	jclr	#SSI_RDF,x:SSI_SR,_foo	;
	movep	x:SSI_RX,x:(r5)+	;
_foo					;
	jset	#HF1,x:HSR,_ackEnd	;
					;
finin					;    finish filling the input buffer
	move	x:1,b			;
	move	r5,a			;
	cmp	a,b			;
	jeq	loop_end		;
_foo					;
	jclr	#SSI_RDF,x:SSI_SR,_foo	;
	movep	x:SSI_RX,x:(r5)+	;
	jmp	finin			;}
loop_end				;
	jset	#HF0,x:HSR,stop	;
	jmp	loop			;
