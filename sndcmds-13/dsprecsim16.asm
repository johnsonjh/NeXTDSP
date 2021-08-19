; ================================================================
; This dsp 56000 assembler code will bring in 16 bit data from the SSI
; serial input and feed it to the host using simple DMA mode.
; 
; modified by dana massie 7-10-89
; 
;; setup_ssi_sound - turn on SSI serial port for 16-bit sound input or output
;;
;; The SSI will be enabled (except for pc3 (sc0) which enables static RAM).
;;
;cra_init equ	$6100           ; 24 bit word selected
cra_init equ	$4100           ; 16 bit word selected
                                ; This initial setting of CRA is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments                                         |
;------------------------------------------------------------------------------
;|  PSR          0           No pre-scaler [divide-by-8] on TX/RX clock.      |
;|  WL1          1            Select a 24-bit word-length.                    |
;|  WL0          1           /                                                |
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
;crb_init equ    $3a00           ; transmit, receive enabled
crb_init equ    $2a00           ;  receive enabled
                                ; This setting of CRB is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments
;------------------------------------------------------------------------------
;|  RIE          0           No interrupt on Receiver-full.                   |
;|  TIE          0           No interrupt on Transmitter empty.               |
;|  RE           1           Receiver is enabled.                            |
;|  TE           1           Transmitter is enabled.                          |
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
;shiftin equ $00800000	; shift factor for 24 bit input data
shiftin equ $008000	; shift factor for 16 bit input data

	include "ioequ.asm"
	include	"dspsound.asm"

main
	move	#>XRAMLO,r5		;
	move	r5,x:1			;
	move	#>XRAMLO+DMASIZE,r6	;use external RAM for double buffers
	move	r6,x:0			;
	move	#>DMASIZE-1,m5		;
	move	#>DMASIZE-1,m6		;
	move	#1,n6			;
	move	#1,n5			;
	move	#>shiftin,y1		;scale factor to produce 16 bit output
	jsr	start			;
	movep	#cra_init,x:Mo_CRA	; Set up serial port
	movep	#crb_init,x:Mo_CRB	; 	in network mode
	do	#DMASIZE,prime2		;prime the first buffer
prime					;
	jclr	#Mo_RDF,x:Mo_SR,prime ;
	movep	x:Mo_RX,x:(r5)+	;


prime2					;
					;
loop					;while (1) {
	move	x:1,r6			;
	move	x:0,r5			;
	move	r6,x:0			;    swap buffers
	move	r5,x:1			;
beginBuf				;
;	jclr	#Mo_RDF,x:Mo_SR,_beg2	;
;	movep	x:Mo_RX,x:(r5)+	;
;_beg2					;
;	jclr	#Mo_HTDE,x:Mo_HSR,beginBuf	;    start DMA
;	movep	#DM_R_REQ,x:Mo_HTX		;
;_ackBegin				;
;	jclr	#Mo_RDF,x:Mo_SR,_ack2	;
;	movep	x:Mo_RX,x:(r5)+	;
;_ack2					;
;	jclr	#Mo_HF1,x:Mo_HSR,_ackBegin	;
;					;
	do	#DMASIZE,senddone	;    send the output buffer
send					;
	jclr	#Mo_RDF,x:Mo_SR,_foo	; if receive data register full,
	movep	x:Mo_RX,x:(r5)+		; save data

_foo					;
	jclr	#Mo_HTDE,x:Mo_HSR,send	;
	move	x:(r6)+,x1		;
	mpy	x1,y1,a			;
	move	a,x:Mo_HTX			;    
senddone				;
					;
;_prodDMA				;    handshake the DMA
;	btst	#DMA_DONE,x:x_sFlags	;
;	jcs	endDMA			;
;	jclr	#Mo_RDF,x:Mo_SR,_foo	;
;	movep	x:Mo_RX,x:(r5)+	;
;_foo					;
;	jclr	#Mo_HTDE,x:Mo_HSR,_prodDMA	;
;	movep	#0,x:Mo_HTX		;
;	jmp	_prodDMA		;
;endDMA					;
;	bclr	#DMA_DONE,x:x_sFlags	;
;_ackEnd					;
;	jclr	#Mo_RDF,x:Mo_SR,_foo	;
;	movep	x:Mo_RX,x:(r5)+	;
;_foo					;
;	jset	#Mo_HF1,x:Mo_HSR,_ackEnd	;
					;
finin					;    finish filling the input buffer
	move	x:1,b			;
	move	r5,a			;
	cmp	a,b			;
	jeq	loop_end		;
_foo					;
	jclr	#Mo_RDF,x:Mo_SR,_foo	;
	movep	x:Mo_RX,x:(r5)+	;


	jmp	finin			;}
loop_end				;
	jset	#Mo_HF0,x:Mo_HSR,stop		;
	jmp	loop			;


