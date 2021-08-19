NOT_DEBUG equ (1)  ; 0 enables simulator; 1 enables real-time use
;  Copyright 1989 by NeXT Inc.
;  Author - Dana Massie 
;      
;  Modification history
;  --------------------
;      06/08/89/dcm - modified from playsquelch.asm
;  ------------------------------ DOCUMENTATION ---------------------------
;  NAME
;      decodemulawsquelch - 
;        -takes a buffer of 8 kHz codec muLaw data and produces 8kHz
;	    codec muLaw data.
;      
;  DESCRIPTION ************************************************************
;   The run-length encoding of silences is formatted in up to 256 byte blocks,
;   with the actual length stored in the second byte.
;    2 bytes are actually used to encode the run; the first (header) byte
;   encodes whether the run contains sample data or zeros, the second
;   contains the length (up to 255). The header byte is also a 
;   synchronization word; if it is not one of two distinct
;   values, then the dsp knows that the data stream is out of synch,
;   and potentially invalid.
;
;    A fixed header of length 8 is simply discarded. This contains
;    the original length of the file, and the threshold used for encoding.
;   
	page 255,255,0,1,1
	include	"dspsound.asm"
;
; Equates here :

; Sleaze Alert!!!
; We really need to keep "Location Counters" for X and Y mem.

muzero		equ	($ff)	; mulaw version of zero
ymemRunFlag	equ	(0) 	; reserve Y mem for runFlag
ymemRunLen	equ	(ymemRunFlag+1)
runbit		equ	(0)
ZeroRunFlag	equ	($55)	; arbitrary flag with lsb==1
SampleRunFlag	equ	($AA)	; arbitrary flag with lsb==0
InitFileFlag	equ	($A5)	; arbitrary flag to indicate sample len word


testpoint	equ	($80)  ; this should be a free location for testpoint


; *************		Main Loop !!   		*****************************
main
; *************		Initialization   	*****************************
;
;	R0	 temp pointer for init
;	R1	temp pointer for init; also used for runlen and runflag ptr
;	R2
;	R3
;	R4	Input Sample Pointer Increment address
;	R5
;	R6
;	R7	unused
;
;	M2,3,4,5 Filter State Vector Size - 1
;	Y0	unused
;	Y1	MuLaw input
;	X0
;	X1
;	A	
;	B	
; *******************************************************************************
;
; Zero out y mem locations for runFlag and runLen
	move	#ymemRunFlag,r1
	clr	a
	move	a,y:(r1)+
	move	a,y:(r1)
; Init for main loop.
	move	#>$8000,y0	; shift constant to move sample to lsb's
	jsr	start
;	
	jsr	getFlag	; init for run-len decode
;
;********* End Initialization *********
;****************************************************************************
; 	|||||||
loop
; 	|||||||
	move	#ymemRunFlag,r1	
	move	#>muzero,a		; source of zero for zero runs
	jclr	#runbit,y:(r1),sampRun	; if (runBit ==0 ) goto sampRun
	jmp	pastsmp			; else
sampRun					; get input data sample
	if NOT_DEBUG
	jsr	getHost			; leaves input data in A1
	jcs	stop
	else
	move	x:$f1,a
	endif
	
pastsmp
	if NOT_DEBUG
	jsr	putHost
	jcc	keepgoing
	jmp	stop
keepgoing
	else
	move	a,x:$f0		; write to this location for output file
	endif

	;  if ( --runLen == 0 ) getHeaderByte; getRunLen;
	move	#ymemRunLen,r1
	nop
	move	y:(r1),b
	move	#>1,a
	sub	a,b	; --runLen
	tst	b
	move	b,y:(r1)	; save decremented value	
	jsle	getFlag	; get preamble for sample run

	nop
	jmp loop

; init subroutine for run-length decode
; uses regs a,b
;
getFlag
	move	#ymemRunFlag,r1	; address for run flag; setup for later
	if NOT_DEBUG
	jsr	getHost			; leaves input data in A1
	jcs	stop	; we really should do an enddo for safety here
	else
	move	x:$f1,a
	endif

	; mask off relevant bits to be safe!
	move	#>$0ff,x0
	and	x0,a

	move	a,y:(r1)+		; save run Flag

	move	#>ZeroRunFlag,b	
	cmp	a,b	; cmp header with zeroRunFlag
	jeq	glen	; zero run flag valid; continue

	move	#>SampleRunFlag,b	
	cmp	a,b
	jeq	glen	; samp run flag valid; continue
	
	move	#>InitFileFlag,b    ; only once per file
	cmp	a,b	
	jne	stop	; disaster deluxe: all flags false!
	
	if NOT_DEBUG
	jsr	getHost ; one byte zero pad after magic flag 
	jsr	getHost ; discard threshold byte 0 msb
	jsr	getHost ; discard threshold byte 1 lsb
	jsr	getHost	; discard sample length - 4 bytes : byte 0
	jsr	getHost ; byte 1
	jsr	getHost ; byte 2
	jsr	getHost ; byte 3 (last byte of length )
	
	jsr	getHost ; 8 bytes for future expansion : 0
	jsr	getHost ; 1
	jsr	getHost ; 2
	jsr	getHost ; 3
	jsr	getHost ; 4
	jsr	getHost ; 5
	jsr	getHost ; 6
	jsr	getHost ; 7
	else
	move	x:$f1,a ; one byte zero pad after magic flag 
	move	x:$f1,a ; discard threshold byte 0 msb
	move	x:$f1,a ; discard threshold byte 1 lsb
	move	x:$f1,a	; discard sample length - 4 bytes : byte 0
	move	x:$f1,a ; byte 1
	move	x:$f1,a ; byte 2
	move	x:$f1,a ; byte 3 (last byte of length )
	
	move	x:$f1,a ; 8 bytes for future expansion : 0
	move	x:$f1,a ; 1
	move	x:$f1,a ; 2
	move	x:$f1,a ; 3
	move	x:$f1,a ; 4
	move	x:$f1,a ; 5
	move	x:$f1,a ; 6
	move	x:$f1,a ; 7
	endif
	jmp	getFlag	; continue to get next flag
glen
	if NOT_DEBUG	; get length of run
	jsr	getHost			; leaves input data in A1
	jcs	stop	; we really should do an enddo for safety here
	else
	move	x:$f1,a
	endif

	; mask off relevant bits to be safe!
	move	#>$0ff,x0
	and	x0,a
	move	a,y:(r1)	; store runLen

	rts
