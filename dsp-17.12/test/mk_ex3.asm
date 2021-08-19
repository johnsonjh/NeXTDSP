; mk_ex3.asm
;
; This example was used to get macro ugsrc/dswitcht.asm working.
; After the first breakpoint, inspect the vectors x:xoutvec
; and y:youtvec.  This is the output when the switch is passing 
; scale10*x:xvec1. Single-step one instruction to get past the SWI,
; hit "run" again, and after the second breakpoint, 
; inspect the vectors x:xoutvec and y:youtvec again.  This is the 
; output when the delayed switch is passing scale20*x:xvec2.
;
; ----------------------------------------------------------------------------
; tdswitcht.asm - Test program for ug macro dswitcht.asm
;
; 08/10/89/jos - created
; 08/10/89/jos - verified
;
     define nolist 'list'
     include 'config_standalone'
XY_SPLIT   set 1		; Allow external memory to be half x, half y
				; (by default it is all x in standalone mode)
     include 'music_macros'
     beg_orch 'tdswitcht'
;
	define NTIX '2'		; duration of test

	  beg_xeb 		; input x vectors

i	    set 1
	    symobj xvec1
xvec1	    dup I_NTICK 	; generate length I_NTICK ramp
	       dc 0.01*i
i	       set i+1
	    endm

i	    set 1
	    symobj xvec2
xvec2	    dup I_NTICK 	; generate length I_NTICK ramp = vec1 * 2
	       dc 0.02*i
i	       set i+1
	    endm

	  end_xeb

	  new_xeb xoutvec,I_NTICK,0	; Allocate x output vector

	  new_yeb youtvec,I_NTICK,0	; Allocate y output vector

	  beg_orcl			; begin orchestra loop
		nop_sep 3
;dswitcht  macro pf,ic,sout,aout0,sinp,i1adr0,scale10,i2adr0,scale20,tickdelay0
		define nolist 'list'
		dswitcht orch,1,x,xoutvec,x,xvec1,0.1,xvec2,1.0,1
		nop_sep 3
		dswitcht orch,2,y,youtvec,x,xvec1,0.1,xvec2,1.0,1
		nop_sep 3
		DEBUG_HALT		; Bug56 breakpoint
		nop_sep 3
	  end_orcl			; end of orchestra loop

     end_orch 'tdswitcht'		; end of orchestra main program
