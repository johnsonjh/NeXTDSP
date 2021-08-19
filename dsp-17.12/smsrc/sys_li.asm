; l system environment variables
;
; This file is 'included' by /usr/lib/dsp/smsrc/sys_.asm.
; The space allocation is specified in /usr/lib/dsp/smsrc/memmap.asm (NLI_SYS).
;
 	xdef X_ZERO,L_ZERO,Y_ZERO
	if !AP_MON
 	  xdef L_TINC,Y_TINC,L_TICK,Y_TICK,X_TICK
	endif
 	xdef X_DMASTAT,L_STATUS,Y_RUNSTAT

	  org l_i:	; l internal memory
L_ZERO	  dc 0		; double-precision zero
L_STATUS  dc 0		; DMA state,,run status
	if !AP_MON
L_TICK	  dc 0		; current "tick" count
L_TINC	  dc 0		; size of a tick in samples (for incrementing L_TICK)
	endif

; re-allocate l memory x and y overlays to avoid "type" mismatch

	  org x_i:	 ; x internal memory
X_ZERO	  ds 1		 ; x version gives zero in x memory
X_DMASTAT ds 1		 ; DMA state (see sys_xe.asm for bit definitions)
	if !AP_MON
X_TICK	  ds 1		 ; current "tick" count, hi order word
	  ds 1		 ; no name for hi order word of L_TINC
	endif

	  org y_i:	 ; y internal memory
Y_ZERO	  ds 1		 ; y version gives zero in y memory
Y_RUNSTAT ds 1		 ; Run status (everything not associated with DMA)
	if !AP_MON
Y_TICK	  ds 1		 ; current "tick" count, lo order word
Y_TINC	  ds 1		 ; y version serves as single-precision epsilon
	endif

	  org l_i:	 ; in case includer checks up on us








