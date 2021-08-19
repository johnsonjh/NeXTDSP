; ssicompress.asm
; 	See comments in compress.asm.
;	This is the version that reads samples from the DSP SSI port.
;

	opt	mex
	page	132
	
	include	"dspsoundssi.asm"
	
SOUND_HEADER	equ	0	;no soundfile header in sample stream
REAL_TIME	equ	1	;real time, use a subset of the algorithms

getSample macro
	jsr	getSSI
	endm

	include "compress.asm"

