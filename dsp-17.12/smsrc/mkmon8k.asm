; mkmon8k.asm - Create object file containing Music Kit DSP Monitor

; *** Must manually keep version,rev in synch with verrev.asm ***
mkmon8k	ident 1,17		; NeXT DSP-resident music monitor
;*	define nolist 'list'	; stand back!
NEXT_8K set 1		   	; Assemble for NeXT hardware
ASM_SYS set 1			; include system
	include 'music_macros'	; need 'memmap, lc, allocsys'
	end PLI_USR		; default start address in DSP

