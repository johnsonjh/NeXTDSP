NOT_DEBUG equ (1)  ; 0 enables simulator; 1 enables real-time use

DMASIZE	equ	4096	;;;This should use a variable instead!!! LRB

; NOTE!! This Version uses only P memory space, instead of P and X and Y
; this is to assist in veryifying the operation of booter.asm
;
;5-18-89
;;  Copyright 1989 by NeXT Inc.
;;  Author - Dana Massie 
;;      
;;  Modification history
;;  --------------------
;;      04/10/89/dcm - modified from run.asm
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      encodeRun - 
;;        -takes a buffer of 8 kHz codec muLaw data, performs an
;;	estimate of the amplitude of the signal, and run-length
;;	encodes the signal that falls below a given threshold (silences)
;;	Also, passes the first 5 bytes thru unaltered. This is to allow
;;	easy storage of the length of the un-encoded signal.
;;      
;;  DESCRIPTION ************************************************************
;;   THe amplitude estimator is a recursive magnitude estimate.
;;
;;   The first byte in the encoded data file is a 
;;   InitFileFlag ($A5)	; arbitrary flag to indicate sample len word
;;   This is followed by 4 bytes (one int) holding the length in bytes
;;   of the un-encoded signal.  This is used to calculate the 
;;   actual duration of the decoded signal to set the correct time-out
;;   values for playback.
;;   The run-length encoding of silences is formatted in up to 256 byte blocks,
;;   with the actual length stored in the second byte.
;    2 bytes are actually used to encode the run; the first (header) byte
;;   encodes whether the run contains sample data or zeros, the second
;;   contains the length (up to 255). The header byte is also a 
;;   synchronization word; if it is not one of two distinct
;;   values, then the dsp knows that the data stream is out of synch,
;;   and potentially invalid.
;;
;;   Note!  The calling (host) routine is responsible for maintaining
;;   a count of how many bytes of data are actually produced on output!
;;   The total size is dependent on the length of silences, and it is
;;   easier for the host to count this than to send a count up after the
;;   sample data have been consumned.
;;   
;;  MEMORY USE
;;      
;;  IPORTANT EXTENSIONS TO BE IMPLEMENTED
;;   There is a great discussion of speech/non-speech decisions in
;;   "Digital Processing of Speech Signals) by Rabiner and Schafer
;;   Prentice-Hall.  They advise using a combination of 
;;   rms energy (or magnitude) and zero-crossing rate.  If the zero-crossing
;;   rate exceeds a given threshold, then an unvioced fricative is probably
;;   starting.  This helps in detecting fricatives that are below the
;;   energy threshold, which is apparently faily common.  This routine
;;   does not use this.
;;

	page 255,255,0,1,1
	include	"dspsound.asm"
;
; Equates here :
XLI_MLT		equ	(256)	; mulaw table address in rom
runbit		equ	(0)
ZeroRunFlag	equ	($55)	; arbitrary flag with lsb==1
SampleRunFlag	equ	($AA)	; arbitrary flag with lsb==0
InitFileFlag	equ	($A5)	; arbitrary flag to indicate sample len word

testpoint	equ	($80)  ; this should be a free location for testpoint

msbShift	equ	($8000)
lsbShift	equ	($80)

SAMPLE_RATE	equ	(8000.0)
RISE_SECONDS	equ	(0.01)	; not exactly seconds...
RISE_SAMPLES	equ	(SAMPLE_RATE*RISE_SECONDS)
DECAY_SECONDS	equ	(0.1)	; not exactly seconds...
rc		equ	(1.0-1.0/(SAMPLE_RATE*RISE_SECONDS))
dc		equ	(1.0-1.0/(SAMPLE_RATE*DECAY_SECONDS))

; use p mem only; for debugging
;	org	x:$0
xmemsrc
avMagNM1	dc	0	; reserve for running average magnitude
riseCons	dc	rc
decayCons	dc	dc
threshold	dc	(150<<8) ;  overwritten by threshold from host
xmemend
xmemlen	equ	(xmemend-xmemsrc)
xmemdst	equ	(0)	; destination adress for xmem data 
; the following are the new "synthetic" labels we need
xavMagNM1	equ	xmemdst
xriseCons	equ	xmemdst+1
xdecayCons	equ	xmemdst+2


; switch to p mem only; for debugging
;	org	y:$0

ymemsrc
runFlag	dc	(0) ; reserve Y mem for runFlag
ymemend
ymemlen	equ	(ymemend-ymemsrc)
ymemdst	equ	(0)
yrunFlag	equ	ymemdst

; switch to p mem only; for debugging
;	org	y:XRAMLO+DMASIZE+1	; choose external mem for run buf
;runBuf	ds	254
;bufEnd	dc	0	; we just need the end address of runBuf

runBuf	equ	XRAMLO+DMASIZE+1	; choose external mem for run buf
bufEnd	equ	runBuf+254	; we just need the end address of runBuf


; ************************************************************************
;  runTest(): uses  x0,y0 a,b r0,r1,r2,r4
;  input: magnitude estimate in register A
;  leaves run flag in register B upon exit
runTest	macro   
	move	#>ZeroRunFlag,b
	move	#>threshold,r2
	move	#>SampleRunFlag,x0
	move	x:(r2),y0	; get threshold out of mem
	nop
	cmp	y0,a	; if (magEstimate(ip, bufend)  >  threshold )
	tgt	x0,b	; over write with SAMPLE_RUN_MAGIC_FLAG if GT
	endm
; ************************************************************************



; ************************************************************************
; magEstimate; mag estimator macro:
; uses regs:
; a,b x0,x1 y0,y1 r1,r0,r2
; mulaw data enter in reg A
; avMag stored in x:(r0), r0=#avMagNM1
; also left in reg A upon exit
; *****************************
; translate mulaw to linear
; (we could simplify mulaw translation for positive data only,
; but we would like to do an agc here eventually,
; which wants both polarities of signal.)
magEstimate macro mavMagNM1,mriseCons,mdecayCons
;	arguments; 
; 	avMagNM1 - address of running average magnitude
;	riseCons - rise time constant address
;	decayCons - decay time constant address
	tfr	a,b	#>$7f,x0
	and	x0,a	#XLI_MLT,x1
	add	x1,a	b,x0
	move	a1,r6
	move	#>$8000,y1
	mpy	x0,y1,b
	move	b0,b1
	lsl	b	x:(r6),a
	neg	a	a,b
	tcs	b,a
	; lin data are in a1 !!
;      /* this recursive estimator for average magnitude has unity dc gain */
;      if (avMag < linmag)
;	avMag = linmag + (avMagNM1 - linmag) * rise;
;      else
;	avMag = linmag + (avMagNM1 - linmag) * decay;
	move	#mavMagNM1,r0	; avMagNM1 
	abs	a		; linmag here into A
	move	a,x1	; temp save for linmag 
	move	x:(r0),b	; get avMagNM1 into reg B

	move	#>mriseCons,r1	; get rise constant address
	move	#>mdecayCons,r2	; get decay constant address

	sub	a,b	; (avMagNM1 - linmag) => B
	; if linmag > avmag, then we choose rise constant
	move	B,x0 	; (avMagNM1 - linmag) => x0
	tgt	x1,a	r2,r1	; x1 -> a is just a dummy transfer

	nop
	move	x:(r1),x1
	nop
	mac	x0,x1,a		;linmag + (avMagNM1 - linmag) * rise
	move	a,x:(r0)	; avMagNM1 = avMag;
	endm	; mag estimate done!


; *************		Main 		****************************
	org	p
main
;*********  Initialization  *********

; first move data from p mem to x mem 
; this is a debug stategy to avoid using the complex loader
	move	#xmemsrc,r0	; source data address
	move	#xmemdst,r1	; destination address 
	do	#xmemlen,move_xdat
	movem	p:(r0)+,x0
	move	x0,x:(r1)+
move_xdat

; move data from p mem to y mem 
; this is a debug stategy to avoid using the complex loader
	move	#ymemsrc,r0	; source data address
	move	#ymemdst,r1	; destination address 
	do	#ymemlen,move_ydat
	movem	p:(r0)+,x0
	move	x0,y:(r1)+
move_ydat

	jsr	start
	opt	mex
	
; 8 bytes of internal squelch header pass thru here
; byte #
; 0 = magic = 4A%
; 1 = 0
; 2-3  Threshold (grab!)
; 4-8 un-encoded file size; pass thru
	
	move	#>runBuf,r3
	move	#>threshold,r2
	; 
	jsr	getHost			; leaves input data in A1
	jsr	putHost	; send magic
	jcs	stop
	
    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; send 0
	jcs	stop

; Grab threshold here!
    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; send threshold byte 0 (msb)
	jcs	stop
	; pack data into threshold destination
	move	#>msbShift,x0
	move	a1,y0
	mpy	x0,y0,a
	move	a0,y1	; y1 is temp for assembly of threshold



    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; send threshold byte 1 (lsb)
	jcs	stop
	
	move	#>lsbShift,x0
	move	a1,y0
	mpy	x0,y0,a
	move	a0,a1
	or	y1,a	; y1 is temp for assembly of threshold
	move	a1,x:(r2)	; save threshold!
	
; pass filesize next
    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; leaves input data in A1
	jsr	putHost	; pass filesize
	jcs	stop

; pass 8 pad bytes for futore expansion next
    	jsr	getHost			; byte 0
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 1
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 2
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 3
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 4
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 5
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 6
	jsr	putHost	; pass filesize
	jcs	stop

    	jsr	getHost			; byte 7
	jsr	putHost	; pass filesize
	jcs	stop

	if NOT_DEBUG ; get  input sample
	jsr	getHost			; leaves input data in A1
	jcs	stop	; we should never quit after only one sample!
	else
	move	x:$f1,a		; debug input of sample data
	endif
;  ******
; delay line would go here if need be...
	magEstimate xavMagNM1,xriseCons,xdecayCons ; av mag is in reg A now...
;  ******
	runTest	; leaves Run Flag in register B
;  ******
	move	#yrunFlag,r0
	nop
	move	b1,y:(r0)	; save magic run flag
;  ******************************************************************
;  ********* End Initialization *********

; 	|||||||
runLoop
; 	|||||||

;    while (ip < bufend){
;	if ( runFlag != (temp = runTest( ip++, bufend, threshold )) 
;           || (rCnt >= runMax))
	move	#yrunFlag,r0	; redundant!

	if NOT_DEBUG	; get next sample
	jsr	getHost			; leaves input data in A1
	jcs	stop
	else
	move	x:$f1,a		; debug input of sample data
	endif
;  ****** save input data; if zeros, we discard later (on output)
	move	a1,y:(r3)+	; must test for end of buf below!
;  ******
	magEstimate xavMagNM1,xriseCons,xdecayCons
;  ******
	runTest	; leave runFlag: result of (magEstimate() > threshold ) in  B
;  ******
	move	#yrunFlag,r0
	move	b,x1		; temp storage of new runFlag
	move	y:(r0),a ; get old runFlag
	cmp	a,b
	; if not equal, we have a transition
	jne	sendRun
;  	********************************
; test for (rCnt >= runMax)
	move	#>bufEnd,b	; runBuf end address + 1 
	move	r3,A		; r3 = current output pointer
	cmp	a,b
	jgt	runLoop

sendRun
	move	#>runBuf,r2
	move	y:(r0),a ; get old runFlag

	; do putHosts until we eat up the current buffer!
	; old runFlag should be in A

	if NOT_DEBUG
 	jsr	putHost	; send runFlag
	jcs	stop
	else
	move	a,x:$f0		; write to this location for output file
	endif

	move	r2,b	; b is buf start address
	move	r3,a	; a is current outputPointer
	sub	b,a	; outputPointer - start = runLen
	if NOT_DEBUG
 	jsr	putHost	; send runFlag
	jcs	stop
	else
	move	a,x:$f0		; write to this location for output file
	endif
	
	jset	#0,y:(r0),nosamps ; runFlag lsb set means zero run
	move	x1,y:(r0)	; store new runFlag here!
	move	r3,b		; load B reg for output loop test

doit	move	y:(r2)+,a
	if NOT_DEBUG
	jsr	putHost	; send runFlag
	jcs	stop
	else
	move	a,x:$f0		; write to this location for output file
	endif
	move	r3,b		; load B reg; b = loop end
	move	r2,a		; a = outputPointer
	nop
	cmp	b,a	;if ( a = outputPointer < b = loop end )
	jlt	doit	;   continue loop
nosamps	move	#>runBuf,r3	; reset output pointer 
	move	x1,y:(r0)	; store new runFlag here!
	jmp	runLoop



