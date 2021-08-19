;;  ap_ex1.asm - Test program for ap macro vpv.asm (vector plus vector)
;;
;; Add two vectors together.  When the program halts in Bug56, open an X
;; memory "space editor", set loLimit to out (=$200E), and look for the numbers
;; 8,8,8,8,8,8,8 in the ouput vector x:out#7.
;;
;; Usage:
;;	asm56000 -A -B -L -OS,SO -I/usr/lib/dsp/smsrc/ ap_ex1
;; 		(or, simply "dspasm ap_ex1", to use the shell script provided)
;;	open /NextDeveloper/Apps/Bug56.app
;;	<File / Load & erase symbols> ap_ex1.lod
;;	<run>
;;
;*	define nolist 'list'	; get absolutely everything into listing file
	include 'config_standalone' ; in this directory
        include 'ap_macros' 	; typically in /usr/lib/dsp/smsrc/

	beg_ap 'tvpv'		; begin array processing test program

        beg_xeb			; begin x external block
in1       dc 1,2,3,4,5,6,7	; input vector 1
        end_xeb

        beg_xeb
in2       dc 7,6,5,4,3,2,1	; input vector 2
        end_xeb

        new_xeb out,7,$777	; output vector, initialized with $777's

        vpv ap,1,x,in1,1,x,in2,1,x,out,1,7	; vector plus vector

	DEBUG_HALT		; stop. inspect x:out#7

        end_ap 'tvpv'

        end




