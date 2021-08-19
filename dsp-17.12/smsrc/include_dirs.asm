; include_dirs.asm - specify macro include directories
;
; Assumes a symbolic link "/dsp" pointing to the DSP source directory
;
    maclib  '/dsp/smsrc/'
    maclib  '/dsp/umsrc/'
    if AP_MON
	maclib  '/ap/apsrc/'
    else
	maclib  '/dsp/ugsrc/'
    endif

; add last line to avoid asm bug




