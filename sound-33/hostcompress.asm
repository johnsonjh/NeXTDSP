; hostcompress.asm
; 	See comments in compress.asm.
;	This is the version that reads samples from the host.
;

	opt	mex
	page	132

LEADPAD		equ	0	;number of buffers before sound begins
TRAILPAD	equ	0	;number of buffers after sound ends

	include	"dspsounddi.asm"
	
SOUND_HEADER	equ	1	;sample stream starts with soundfile header
REAL_TIME	equ	0	;not real time, use all algorithms

getSample macro
	jsr	getHost
	endm

	include "compress.asm"

