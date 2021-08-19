; ssiplay.asm
;	Read samples from host and send to ssi port.
;	Double buffered ssi output.
;
;	Modification History:
;	04/16/90/mtm	Original coding
;	06/11/90/mtm	Implement dsp-initiated dma from the host.
;
	include	"portdefs.asm"
	opt	mex
	page	132
	
;
;------------------------- Equates
;

READ_BUF	equ	XRAMLO	; start of read buffer
SSI_BUFSIZE	equ	2048	; size of each ssi output buffer

SC_W_REQ	equ	$020002	  ;"Sys call" requesting DMA write on chan 2
DM_W_REQ	equ	$040002	  ;message requesting DMA write on channel 2 

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
crb_init equ    $1b30
                                ; This setting of CRB is determined
                                ; as follows [For more info, see 
                                ; the DSP56000 users manual ....]:
;------------------------------------------------------------------------------
; Bit-name    Bit-value      Comments                                         |
;------------------------------------------------------------------------------
;|  RIE          0           No interrupt on Receiver-full.                   |
;|  TIE          0           No interrupt on Transmitter empty.               |
;|  RE           0           Receiver is not enabled.                         |
;|  TE           1           Transmitter is enabled.                          |
;|  MOD          1           Use Network-mode communication [not Normal-mode].|
;|  GCK          0           Use continuous clock [not gated clock].          |
;|  SYN          1           Transmission and reception are synchronous.      |
;|  FSL          1           Use short frame-sync.                            |
;|   *           0           Unused ......                                    |
;|   *           0           Unused ......                                    |
;|  SCKD         1           Output clock-source.                             |
;|  SCD2         1           Output Frame-sync.                               |
;|  SCD1         0             I'm using neither SC0 nor SC1 for data-        |
;|  SCD0         0             transmission/reception, so leave 'em alone.    |
;|  OF1          0            /                                               |
;|  OF0          0           /                                                |
;------------------------------------------------------------------------------

;
;------------------------- Variable locations
;
x_sConfig	equ	$00ff		;ssi port configuration flags
RCV_CLOCK	equ	0		;  receive bit clock
RCV_FSYNC	equ	1		;  receive frame sync	
y_sFlags	equ	$00ff		;buffer flags
SSI_DONE	equ	0		;  SSI output buffer sent
B_DMA_DONE	equ	1		;  indicates dma complete
B_SYS_CALL	equ	2		;  indicates sys call has been received

x_sPutVal	equ	$00fe		;last value to putSSI
y_sSSICurSamp	equ	$00fe		;current offset in SSIFillBuf

x_sSSIFillBuf	equ	$00fd		;current SSI buffer to fill
y_sSSISendBuf	equ	$00fd		;current SSI buffer to send

x_sSSISaveA0	equ	$00fc		;SSI ISR save A0
x_sSSISaveA1	equ	$00fb		;SSI ISR save A1
x_sSSISaveA2	equ	$00fa		;SSI ISR save A2

y_sSSISaveB0	equ	$00fc		;SSI ISR save B0
y_sSSISaveB1	equ	$00fb		;SSI ISR save B1
y_sSSISaveB2	equ	$00fa		;SSI ISR save B2

x_sSysArg	equ	$00f9		;sys_call arg
y_sDmaSize	equ	$00f9		;dma transfer size

;
;------------------------- Interrupt vectors
;
	org	p:VEC_RESET
	jmp	reset

	org	p:VEC_SSI_TDAT		;SSI transmit data
	jsr	ssiTransmit
	org	p:VEC_SSI_TEXC		;SSI transmit data with exception
	jsr	ssiExcTransmit

	org	p:VEC_W_DONE
	bset	#B_DMA_DONE,y:y_sFlags
	nop

	org	p:VEC_SYS_CALL
	jsr	sys_call

	org	p:VEC_END
	
;
;------------------------- Interrupt service routines
;

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
	bset	#B_SYS_CALL,y:y_sFlags  ;set flag to say we got this
	rti

;
;ssiTransmit - service the SSI transmitter empty interrupt.
;  Depends on r7
;
ssiTransmit
	move	a0,x:x_sSSISaveA0
	move	a1,x:x_sSSISaveA1
	move	a2,x:x_sSSISaveA2
	move	b0,y:y_sSSISaveB0
	move	b1,y:y_sSSISaveB1
	move	b2,y:y_sSSISaveB2
	movep	x:(r7)+,x:SSI_TX	;send sample
	move	#>SSI_BUFSIZE,a
	move	y:y_sSSISendBuf,b
	add	a,b
	move	r7,a
	cmp	a,b
	jgt	_ssiExit
	bset	#SSI_DONE,y:y_sFlags
	bclr	#SSI_TIE,x:SSI_CRB		;disable ssi transmit interrupts
_ssiExit
 	move	x:x_sSSISaveA0,a0
 	move	x:x_sSSISaveA1,a1
 	move	x:x_sSSISaveA2,a2
	move	y:y_sSSISaveB0,b0
	move	y:y_sSSISaveB1,b1
	move	y:y_sSSISaveB2,b2
	rti
	
;
;ssiExcTransmit - service the SSI transmitter empty with exception interrupt.
;  Depends on r7
;
ssiExcTransmit
	move	a0,x:x_sSSISaveA0
	move	a1,x:x_sSSISaveA1
	move	a2,x:x_sSSISaveA2
	move	b0,y:y_sSSISaveB0
	move	b1,y:y_sSSISaveB1
	move	b2,y:y_sSSISaveB2
	move	x:SSI_SR,a		;clear error
	movep	x:(r7)+,x:SSI_TX	;send sample
	move	#>SSI_BUFSIZE,a
	move	y:y_sSSISendBuf,b
	add	a,b
	move	r7,a
	cmp	a,b
	jgt	_ssiExit
	bset	#SSI_DONE,y:y_sFlags
	bclr	#SSI_TIE,x:SSI_CRB		;disable ssi transmit interrupts
_ssiExit
 	move	x:x_sSSISaveA0,a0
 	move	x:x_sSSISaveA1,a1
 	move	x:x_sSSISaveA2,a2
	move	y:y_sSSISaveB0,b0
	move	y:y_sSSISaveB1,b1
	move	y:y_sSSISaveB2,b2
	rti

;getHost - returns the next sample from the input stream in register A1.
; When input buffer is empty, blocks until DMA is successful. 
; Must be called once for each sample in a sample-frame.  
; On return, the carry bit indicates that the program should terminate.
; Normally it is clear.
;
; This routine destroys the b and x0 registers.
; It also uses and depends on r6. 
; Assumes r6 is initially set to point to input buffer.
; External ram is used for buffering.
;
getHost
	move   #>READ_BUF,x0			;read-buffer start address
	move 	r6,b				;current pointer
	sub	x0,b				;b-x0 == ptr-base
	jgt	nextSamp			;positive if we're working on a buffer
	;; 
	;; Get a buffer from the host via "DSP-initiated DMA"
	;; 
	bclr	#B_DMA_DONE,y:y_sFlags		;reset end-of-dma indicator bit
	bclr	#B_SYS_CALL,y:y_sFlags		;reset "sys-call pending" bit
_htdeBuzz
	jclr	#HTDE,x:HSR,_htdeBuzz 		;wait until we can talk to host
	movep	#DM_W_REQ,x:HTX			;send "write request" DSP message
	move	#SC_W_REQ,x0			;Expected sys_call
_hcBuzz
	btst	#B_SYS_CALL,y:y_sFlags
	jcc	_hcBuzz				;wait until "sys_call" host command
	move	x:x_sSysArg,b 			; single int argument to host command
	cmp	x0,b				; see if it's what we expected.
	jne	_hcBuzz				; if not, ignore it
	bclr	#B_SYS_CALL,y:y_sFlags		;got it

	; HF1 not checked since above host command means only DMA data will follow.

	move	y:y_sDmaSize,b
	do	b1,_dmaLoop			;service the DMA transfer
_dmaBuzz
	jclr	#HRDF,x:HSR,_dmaBuzz		;DMA hardware handshake
	movep	x:HRX,y:(r6)+			;receive values
_dmaLoop
	move	#>READ_BUF,x0			;read-buffer start address
	move	x0,r6				;reset to buffer start for consumption
_doneBuzz
	btst	#B_DMA_DONE,y:y_sFlags
	jcs	nextSamp			;terminating host command sets us free
	jclr	#HRDF,x:HSR,_doneBuzz		;if the DMA is still sending to us
	movep	x:HTX,x0			;receive it to drain its pipe
	jmp	_doneBuzz
nextSamp
	move	y:(r6)+,a			;the requested read
	move	#>READ_BUF,b			;read-buffer address
	move	y:y_sDmaSize,x0			;buffer length
	add	x0,b				;add to get last address + 1
	move	r6,x0				;current output sample pointer
	sub	x0,b   #>READ_BUF,x0		;subtract x0 from b to get end+1 - ptr
	jgt	_continue			; if positive, keep cruising, else dma
	move	x0,r6				;reset to buffer start to trigger refill
_continue
	rts

;
;putSSI - puts the sample in register a1 to the ssi port.
; Double buffered.
; Destroys r5,n5,b
;
putSSI
	move	a1,x:x_sPutVal
	move	y:y_sSSICurSamp,n5
	move	x:x_sSSIFillBuf,r5
	move	n5,b
	move	a1,x:(r5+n5)
	move	#>1,a
	add	a,b
	move	b1,y:y_sSSICurSamp	;curSamp++
	move	#>SSI_BUFSIZE,a
	cmp	b,a
	jgt	putSSIexit		;if (curSamp < bufSize) exit
waitSend				;wait for previous send to complete
	btst	#SSI_DONE,y:y_sFlags
	jcc	waitSend
	bclr	#SSI_DONE,y:y_sFlags
	move	x:x_sSSIFillBuf,r7	;swap buffers
	move	y:y_sSSISendBuf,b
	move	b1,x:x_sSSIFillBuf
	clr	b  r7,y:y_sSSISendBuf
	move	b1,y:y_sSSICurSamp	;curSamp = 0
	bset	#SSI_TIE,x:SSI_CRB	;enable ssi transmit interrupts
putSSIexit
	move	x:x_sPutVal,a
	rts

;
;reset - initialize and start program from a hard reset (i.e. just 
; bootstrapped).  Falls through to main loop.
;
reset
	; It appears that this must be done before port C initialization below
	movep	#cra_init,x:SSI_CRA	;Set up SSI serial port
	movep	#crb_init,x:SSI_CRB

        movec   #6,OMR			;data rom enabled, mode 2
	bset    #0,x:PBC		;host port

	bset	#3,x:PCDDR		;   pc3 is an output with value
	bclr	#3,x:PCD		;   zero to enable the external ram
	movep   #>$000000,x:BCR		;no wait states on the external sram
        movep   #>$00B400,x:IPR  	;intr levels: SSI=2, SCI=1, HOST=0
getDmaSize
	jclr	#HRDF,x:HSR,getDmaSize	;get dma size from host
	movep	x:HRX,y:y_sDmaSize
getConfig
	jclr	#HRDF,x:HSR,getConfig
	movep	x:HRX,x:x_sConfig	;get ssi config from host
	btst	#RCV_CLOCK,x:x_sConfig
	jcc	config1
	bclr	#SSI_SCKD,x:SSI_CRB
config1
	btst	#RCV_FSYNC,x:x_sConfig
	jcc	config2
	bclr	#SSI_SCD2,x:SSI_CRB
config2

	bset	#0,x:PCDDR	; pc0 is an output with value
        bset	#0,x:PCD	; 1 to set SELECT (for PCM501 D/A mode)

	; Port C Control Register - selects either gen purp I/O or SSI function
	bclr	#0,x:PCC	; bit 0 for pcm 501 select
	bset	#1,x:PCC	; SCI TXD (unused)
	bset	#2,x:PCC	; SCI SCLK (unused)
	bclr	#3,x:PCC	; SC0 must be zero to enable ram
	bset	#4,x:PCC	; SSI SC1 (unused)
	bset	#5,x:PCC	; SSI SC2 (Frame Synch; Left/~Right Clk)
	bset	#6,x:PCC	; SSI SCK
	bset	#7,x:PCC	; SSI SRD
	bset	#8,x:PCC	; SSI STD

	move	#>@cvi(@pow(2,8-1)),y1	;left shift multiply factor
	move	#>READ_BUF,a		;initialize read pointer
	move	a1,r6
	move	y:y_sDmaSize,b		;initialize fill and send buffer pointers
	add	b,a
	move	a1,x:x_sSSIFillBuf
	move	#>SSI_BUFSIZE,b
	add	b,a
	move    a1,y:y_sSSISendBuf
	clr	a
	move	a,y:y_sFlags		;clear flags
	move	a,y:y_sSSICurSamp	;curSamp = 0
	bset	#SSI_DONE,y:y_sFlags	;force initial buffer fill
	bset    #HCIE,x:HCR		;enable host command interrupts
	move	#0,sr			;enable interrupts

	; set up modulo addressing for sine wave output
;;;     move #$100,r4           ; pointer to sine buf (move *,r4 is 2 cycles)
;;;     movec #$0FF,m4          ; modulo 256 (movec *,m4 is 1 cycle)
	nop

	;does not help
	clr	a
	move	#XRAMLO,r0
	move	#>XRAMSIZE,b
	.loop	b1
		move	a1,x:(r0)+
	.endl

loop
	jsr	getHost
;;;	move	y:(r4)+,a
	move	a1,y0			;left justify 16 bit data
	mpy	y0,y1,a
	move	a0,a1
	jsr	putSSI
	jmp	loop

;;;FIXME: need to flush buffers
stop					;wait for previous send to completed
	btst	#SSI_DONE,y:y_sFlags
	jcc	stop
	move	y:y_sSSICurSamp,a
	tst	a
	jeq	halt
	move	x:x_sSSIFillBuf,r7	;send last buffer
	.loop	a1
lastSend
	jclr	#SSI_TDF,x:SSI_SR,lastSend
	movep	x:(r7)+,x:SSI_TX
	nop
	.endl
halt
	movep	#>$000018,x:HCR		;indicate that we have halted
grave
	jclr	#HRDF,x:HSR,grave
	movep	x:HRX,a			;eat stray input
	jmp	grave
