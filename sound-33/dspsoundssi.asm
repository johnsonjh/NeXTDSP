;
;	dspsoundssi.asm
;	Written by Mike Minnick
;	Inspired by dspsound.asm by Lee Boynton
;	Copyright 1990 NeXT, Inc.
;
;Typical use:
;
;		include	"dspsoundssi.asm"
;	main
;		<user initialization>
;		jsr	start
;	loop
;		jsr	getSSI		;called zero or more times
;		<some calculations>
;		jsr	putHost		;called one or more times
;		jcs	exit		;in case host aborts
;		<some loop condition ??>
;		j??	loop		;
;	exit
;		<user shutdown, i.e. ramp down for graceful abort>
;		jmp	stop
;
;Modification History:
;	04/27/90/mtm	Added SSI_BUF_SIZE.
;			Send DMA read request one buffer in advance.
;	05/04/90/mtm	Disable skipping buffer on underrun.
;	05/07/90/mtm	Re-work dma.
;	05/30/90/mtm	Fix initialization of m7.
;	07/18/90/mtm	Removed getHost.
;	08/06/90/mtm	Use x0 instead of y1 in getSSI.
;

	include	"portdefs.asm"

;
;------------------------- Equates
;
SSI_BUF_SIZE	equ	2048
DM_R_REQ	equ	$050001		;message -> host to request dma

cra_init equ	$4100           ; 16 bit word selected
                                ; This initial setting of CRA is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
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
crb_init equ    $aa00           ;  receive enabled with interrupt
                                ; This setting of CRB is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments                                         |
;------------------------------------------------------------------------------
;|  RIE          1           Interrupt on Receiver-full.                      |
;|  TIE          0           No interrupt on Transmitter empty.               |
;|  RE           1           Receiver is enabled.                             |
;|  TE           0           Transmitter is enabled.                          |
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
shiftin equ $008000	; shift factor for 16 bit input data

;
;------------------------- Variable locations
;
x_sFlags	equ	$00ff		;dspsound flags
DMA_DONE	equ	0		;  indicates that dma is complete
RUNNING		equ	1		;  not first call to putHost
SSI_DONE	equ	2		;  SSI input buffer full
SSI_RUNNING	equ	3		;  getSSI has been called
y_sCurSamp	equ	$00ff		;current offset in buffer

x_sPutVal	equ	$00fe		;last value to putHost
y_sDmaSize	equ	$00fe		;dma size

x_sFillBuf	equ	$00fd		;current buffer to fill
y_sSendBuf	equ	$00fd		;current buffer to send

x_sSSIFillBuf	equ	$00fc		;current SSI buffer to fill
y_sSSIAvailBuf	equ	$00fc		;current SSI buffer to use

x_sSSICurSamp	equ	$00fb		;current offset in SSIAvailBuf
y_sSSIUnderrun	equ	$00fb		;underrun count

x_sSSISaveA0	equ	$00fa		;SSI ISR save A0
x_sSSISaveA1	equ	$00f9		;SSI ISR save A1
x_sSSISaveA2	equ	$00f8		;SSI ISR save A2

y_sSSISaveB0	equ	$00fa		;SSI ISR save B0
y_sSSISaveB1	equ	$00f9		;SSI ISR save B1
y_sSSISaveB2	equ	$00f8		;SSI ISR save B2

x_sSSIExc	equ	$00f7		;SSI receive exception count
y_sSpinEnter	equ	$00f7		;max spins waiting for driver to enter dma mode

;
;------------------------- Interrupt vectors
;
	org	p:VEC_RESET
	jmp	reset
;
	org	p:VEC_SSI_RDAT		;SSI receive data
	jsr	ssiReceive
;
	org	p:VEC_SSI_REXC		;SSI receive data with exception
	jsr	ssiReceiveExc
;
	org	p:VEC_HTX		;host transmit data
	movep	x:(r7)+,x:HTX
;
	org	p:VEC_HCMD		;host command
	jsr	hostCommand
;

	org	p:VEC_END

;
;---------------------- INTERRUPT SERVICE ROUTINES ----------------------
;

;
;ssiReceive - service the SSI receiver full interrupt.
;  Depends on r6
;
ssiReceive
	move	a0,x:x_sSSISaveA0
	move	a1,x:x_sSSISaveA1
	move	a2,x:x_sSSISaveA2
	move	b0,y:y_sSSISaveB0
	move	b1,y:y_sSSISaveB1
	move	b2,y:y_sSSISaveB2
	movep	x:SSI_RX,x:(r6)+	;save sample
	move	#>SSI_BUF_SIZE,a
	move	x:x_sSSIFillBuf,b
	add	a,b
	move	r6,a
	cmp	a,b
	jgt	_ssiExit
	move	x:x_sSSIFillBuf,a	;swap buffers
	move	y:y_sSSIAvailBuf,b
	move	a1,y:y_sSSIAvailBuf
	move	b1,x:x_sSSIFillBuf
	move	b1,r6
	bset	#SSI_DONE,x:x_sFlags
_ssiExit
 	move	x:x_sSSISaveA0,a0
 	move	x:x_sSSISaveA1,a1
 	move	x:x_sSSISaveA2,a2
	move	y:y_sSSISaveB0,b0
	move	y:y_sSSISaveB1,b1
	move	y:y_sSSISaveB2,b2
	rti

;
;ssiReceive - service the SSI receiver full interrupt with exception
;  Depends on r6
;
ssiReceiveExc
	move	a0,x:x_sSSISaveA0
	move	a1,x:x_sSSISaveA1
	move	a2,x:x_sSSISaveA2
	move	b0,y:y_sSSISaveB0
	move	b1,y:y_sSSISaveB1
	move	b2,y:y_sSSISaveB2
	move	x:SSI_SR,a		;clear error
	movep	x:SSI_RX,x:(r6)+	;save sample
	move	x:x_sSSIExc,a
	move	#>1,b
	add	b,a
	move	a1,x:x_sSSIExc		;exceptionCount++
	move	#>SSI_BUF_SIZE,a
	move	x:x_sSSIFillBuf,b
	add	a,b
	move	r6,a
	cmp	a,b
	jgt	_ssiExit
	move	x:x_sSSIFillBuf,a	;swap buffers
	move	y:y_sSSIAvailBuf,b
	move	a1,y:y_sSSIAvailBuf
	move	b1,x:x_sSSIFillBuf
	move	b1,r6
	bset	#SSI_DONE,x:x_sFlags
_ssiExit
 	move	x:x_sSSISaveA0,a0
 	move	x:x_sSSISaveA1,a1
 	move	x:x_sSSISaveA2,a2
	move	y:y_sSSISaveB0,b0
	move	y:y_sSSISaveB1,b1
	move	y:y_sSSISaveB2,b2
	rti

;
;hostCommand - service the default host command interrupt.
;  Indicates that a DMA transfer has completed.
;  Sends the next DMA request to the host.
hostCommand
	bclr    #HTIE,x:HCR		;stop host transmit data interrupts
	bset	#DMA_DONE,x:x_sFlags
	
	;Note: it is not necessary to wait for HF1
	;to go low here, the host command implies that
	;the driver is out of DMA mode.

_waitHost
	jclr	#HTDE,x:HSR,_waitHost
	movep	#DM_R_REQ,x:HTX		;send "DSP_dm_R_REQ" to host
	rti
	
;
;---------------------- EXPORTED ROUTINES ----------------------
;

;
;start - enable interrupts and indicate to host that we are ready.
;  Destroys registers A, B and the SR.
;
start
	move	#>XRAMLO,a		;initialize fill and send buffer pointers
	move	a1,x:x_sFillBuf
	move	y:y_sDmaSize,b
	add	b,a
	move    a1,y:y_sSendBuf
	add	b,a
	move	a1,x:x_sSSIFillBuf
	move	a1,r6
	move	#>SSI_BUF_SIZE,b
	add	b,a
	move	a1,y:y_sSSIAvailBuf
	move	y:y_sDmaSize,b
	move	#>1,a
	sub	a,b
	move	b1,m7			;dma buffers accessed modulo dma size
	clr	a
	move	a,x:x_sFlags		;clear flags
	move	a,y:y_sCurSamp		;curSamp = 0
	move	a,x:x_sSSICurSamp
	move	a,y:y_sSSIUnderrun
	move	a,x:x_sSSIExc
	move	a,y:y_sSpinEnter
	bset	#DMA_DONE,x:x_sFlags	;DMA is available
	move	#0,sr			;enable interrupts
	rts
	
;
;getSSI - get next input sample from the SSI serial port
;  Destroys x1, x0, r5, n5, and b
;  Returns sample in a1
;
getSSI
	btst	#SSI_RUNNING,x:x_sFlags
	jcs	_getSSI
	bset	#SSI_RUNNING,x:x_sFlags
	movep	#cra_init,x:SSI_CRA	;Set up SSI serial port
	movep	#crb_init,x:SSI_CRB	;  in network mode, receive interrupt enabled
_getSSI
	clr	b  x:x_sSSICurSamp,a
	cmp	b,a
	jgt	_ssiAvail
	btst	#SSI_DONE,x:x_sFlags	;detect underrun
	jcc	_waitAvail
	move	y:y_sSSIUnderrun,b
	move	#>1,a
	add	a,b
	clr	a  b1,y:y_sSSIUnderrun
	;;;bclr	#SSI_DONE,x:x_sFlags	;skip a buffer - DISABLED
_waitAvail
	btst	#SSI_DONE,x:x_sFlags	;wait for input buffer available
	jcc	_waitAvail
	bclr	#SSI_DONE,x:x_sFlags
_ssiAvail
	move	a1,n5
	move	#>1,b
	add	b,a			;curSamp++
	move	#>SSI_BUF_SIZE,b
	cmp	a,b
	jgt	_ssiExit
	clr	a			;curSamp=0
_ssiExit
	move	a1,x:x_sSSICurSamp
	move	y:y_sSSIAvailBuf,r5
	move	#>shiftin,x0
	move	x:(r5+n5),x1
	mpy	x1,x0,a			;scale input
	rts
	
;
;flush - flush the current dma output buffer.
;
flush
	move	x:x_sPutVal,a
	move	y:y_sCurSamp,b
	tst	b
	jeq	_flush1			;if (curSamp != 0)
	jsr	putHost			;   pad last buffer
	jmp	flush
_flush1
	rts

;
;stop - flush last buffer, signal to host and terminate execution.
;
stop
	clr	a
	jsr	putHost	
	jsr	flush
_trail					;wait for final dma to complete
	btst	#DMA_DONE,x:x_sFlags
	jcc	_trail
	bclr    #HTIE,x:HCR		;stop host transmit data interrupts
_ackEnd
	jset	#HF1,x:HSR,_ackEnd	;wait for HF1 to go low
	movep	#>$000018,x:HCR		;indicate that we have halted
_grave
	jclr	#HRDF,x:HSR,_grave
	movep	x:HRX,a			;eat shit
	jmp	_grave			;and kind of tune out the world


;
;putHost - puts a sample to the output stream. 
; Note that this must be called once for each sample in a
; sample-frame.
; On return, the carry bit indicates that the program should terminate;
; normally it is clear.
;
;Initiates DMA and swaps buffers when current buffer is full.
;
;This routine destroys the b register, and may affect a0 and a2.
;It also uses and depends on r7.
;It also destroys r5 and n5.
;
putHost
	move	a,x:x_sPutVal
	btst	#RUNNING,x:x_sFlags
	jcs	_putHost
_waitHost
	jclr	#HTDE,x:HSR,_waitHost
	movep	#DM_R_REQ,x:HTX		;send "DSP_dm_R_REQ" to host
	bset    #HCIE,x:HCR		;start host command interrupts
	bset	#RUNNING,x:x_sFlags	
_putHost
	move	x:x_sPutVal,a
	move	y:y_sCurSamp,n5
	move	x:x_sFillBuf,r5
	move	n5,b
	move	a,x:(r5+n5)
	move	#>1,a
	add	a,b
	move	b,y:y_sCurSamp		;curSamp++
	move	y:y_sDmaSize,a
	cmp	b,a
	jgt	_exit			;if (curSamp < bufSize) exit
_waitDone				;wait for previous dma to complete
	btst	#DMA_DONE,x:x_sFlags
	jcc	_waitDone
	bclr	#DMA_DONE,x:x_sFlags	;be sure we know we are through
	move	x:x_sFillBuf,r7		;swap buffers
	move	y:y_sSendBuf,b
	move	b1,x:x_sFillBuf
	clr	b  r7,y:y_sSendBuf
	move	b,y:y_sCurSamp		;curSamp = 0
	move	#>1,a
_waitEnter
	add	a,b
	jclr	#HF1,x:HSR,_waitEnter	;wait for HF1 to go high (driver in DMA mode)
	move	y:y_sSpinEnter,a
	cmp	a,b
	jle	_startXmit
	move	b1,y:y_sSpinEnter	;save max dma enter spin count
_startXmit
	bset    #HTIE,x:HCR		;start host transmit data interrupts
_exit
	move	x:x_sPutVal,a
	btst	#HF0,x:HSR
	rts

;
;reset - initialize and start program from a hard reset (i.e. just 
; bootstrapped).
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
;end of dspsoundssi.asm
;
