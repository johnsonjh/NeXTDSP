; hostdecompress.asm
;	See comments in decompress.asm
;	This is the version that sends samples to the host.

	opt	mex
	page	132

WRITE_SNDOUT	equ	0

LEADPAD		equ	0	;number of buffers before sound begins
TRAILPAD	equ	0	;number of buffers after sound ends

	include "dspsounddi.asm"
	include "decompress.asm"

