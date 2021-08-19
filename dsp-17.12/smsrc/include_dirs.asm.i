; includes.asm - specify macro include directories
;
    maclib  '/usr/lib/dsp/smsrc/' 	; system macro library
    maclib  '/usr/lib/dsp/umsrc/' 	; utility macro library
    if AP_MON
	maclib  '/usr/lib/dsp/apsrc/' 	; array proc macro lib
    else
	maclib  '/usr/lib/dsp/ugsrc/' 	; unit-generator library
    endif

; add last line to avoid asm bug

