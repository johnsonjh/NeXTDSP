; sndoutdecompress.asm
;	See comments in decompress.asm
;	This is the version that sends samples to soundout.

	opt	mex
	page	132

WRITE_SNDOUT	equ	1
	
	; The sound driver requires lead pad because it starts soundout
	; DMA right away - while the DSP is still producing the first
	; real buffer.  ** Actually this may now be fixed, but it is nice
	; to have leading and trailing zeros anyway. **
	; BUG #7912 - Setting lead pad to zero fixes the problem of sndout
	; aborting with a large file header.
LEADPAD		equ	0	;number of buffers before sound begins
TRAILPAD	equ	4	;number of buffers after sound ends

	include "dspsounddi.asm"
	include "decompress.asm"

