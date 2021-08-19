;; mk_ex1.asm - Test program for unit generator macro ramp
;;
;; Generate samples of a ramp.  When the program halts in Bug56, open an X
;; memory "space editor", set loLimit to xsig (=4), and look for the numbers
;; 0 through $F in the ouput vector x:4..$13.
;;
;; Usage:
;;	asm56000 -A -B -L -OS,SO -I/usr/lib/dsp/smsrc/ mk_ex1
;; 		(or, simply "dspasm mk_ex1", to use the shell script provided)
;;	open /NextDeveloper/Apps/Bug56.app
;;	<File / Load & erase symbols> mk_ex1.lod
;;	<sstep> or <run>
;;
mk_ex1  ident 0,0		; version, revision (arbitrary)
	include 'config_standalone' ; in this directory
;*	define nolist 'list'	; get absolutely everything into listing file
	include 'music_macros'	; utility macros (in /usr/lib/dsp/smsrc)

	beg_orch 'mk_ex1'	; standard startup for orchestras

	new_xib xsig,I_NTICK,0	; allocate waveform vector

	beg_orcl
		nop_sep 3	; nop's which help visual delineation
;; (arg. mnem.)	ramp pf,ic,sout,aout0,val0h,val0l,slp0h,slp0l 
	      	ramp orch,1,x,xsig,0,0,1,0	; slope == (1,0)
		nop_sep 3
		outa orch,1,x,xsig 	; Output signal to DAC channel A
		nop_sep 3
		break_on_tick 1		; halt after one tick of output
		nop_sep 3
	end_orcl
finish	end_orch 'mk_ex1'





