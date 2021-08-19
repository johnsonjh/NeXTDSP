; music_macros.asm - standard macros for stand-alone orchestra development
;
;; Copyright 1989, NeXT Inc.
;; Author - J. O. Smith
;;
;; Included by mkmon8k.asm, for example.
;;
	page 255,255,0,1,1	   	; Width, height, topmar, botmar, lmar
	opt nomd,mex,cex,mi,xr,s	; Default assembly options
;;*	opt mu,s,cre,cc		   	; Extra assembly options to consider
	lstcol 9,6,6,9,9	   	; Label, Opcode, Operand, Xmove, Ymove
	include 'verrev.asm'		; cannot appear before ident!
	nolist		   		; Too much junk in the following
	include 'config'	   	; Assembly and run-time config ctl
	include 'include_dirs'		; Specify macro include directories
	maclib  './'		   	; current directory (this is needed!)
	if AP_MON
	  cobj 'APMON8K system services required'
	else
	  cobj 'MKMON8K system services required'
	endif
	include 'defines'	   	; NeXT-defined constants
	include 'dspmsgs'   		; all dsp message and error opcodes
	include 'misc'	   		; Useful macros needed immediately
	include 'memmap'	   	; NeXT memory map
	include 'beginend'	   	; Beginning and ending macros
        section SYSTEM	   	   	; dsp system
	    if ASM_SYS
		include 'allocsys' 	; Allocate and initialize system memory
	    else
		include 'sys_messages'
		if AP_MON
	 	    include 'sys_memory_map_ap'
		else
		    include 'sys_memory_map_mk'
		endif
	    endif
        endsec
	section USER
	     	include 'allocusr' 	; User memory-allocation macros
	endsec

	list			   	; Increment list counter back to 0








