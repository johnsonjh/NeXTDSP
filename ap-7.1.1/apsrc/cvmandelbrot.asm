;;  Copyright 1987 by NeXT Inc.
;;  Author - Julius Smith and Richard Crandall
;;      
;;  Modification history
;;  --------------------
;;      03/18/88/jos - initial file created from DSPAPSRC/cvconjugate.asm
;;      04/13/88/jos - introduced normalized recursion
;;	05/16/89/mtm - fixed C SYNTAX comment
;;	06/06/89/mtm - expanded tabs in comments.
;;		       removed '??? Program locations' line
;;		       (should be added some time)
;;	07/12/89/mtm - fixed comment bug (output array NOT complex)
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvmandelbrot (AP macro) - complex vector Mandelbrot set generator
;;        - form an integer vector from the elements of a complex vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvmandelbrot pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0,lim0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvmandelbrot_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address
;;      iinp0   =   increment for input vector
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   increment for output vector
;;      cnt0    =   element count
;;      lim0    =   limit on iteration count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector base address       ainp0
;;      x:(R_X)+    input vector increment          iinp0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      x:(R_X)+    iteration limit                 lim0
;;      
;;  DESCRIPTION
;;      The cvmandelbrot array-processor macro computes the number of 
;;      iterations of the formula z = z * z + c required to reach abs(z)>2,
;;      where z and c are complex.  To make this easier in fixed-point, the
;;      actual recursion used is w = 2 * w * w + d until abs(w) > 1,
;;      where w = z/2 and d = c/2.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;  
;;      iinp    =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;      lim     =   x:(R_X)+;  
;;      
;;      for (n=0;n<cnt;n++) {
;;        wr = dr = sinp:ainp[n*iinp];		/* HALF of real part of c */
;;        wi = di = sinp:ainp[n*iinp+1];	/* HALF of imag part of c */
;;        for (k=0;k<lim;k++) {
;;           tr = wr * wr
;;           ti = wi * wi;
;;           if (tr + ti > 1) break;
;;           wi = 4 * wr * wi;
;;           wi += di;
;;           wr = 2 * (tr - ti);
;;           wr += dr;
;;        }
;;        sout:aout[n*iout] = k;
;;      }
;;      
;;  MEMORY USE
;;      6   x-memory arguments
;;      
;;  USAGE RESTRICTIONS
;;      The inner loop must be executed in the interrupt vector area (pc<64)
;;      to obtain 1us per inner loop iteration (otherwise 1.1us).
;;      Define EXECUTE_IN_VECTORS '1' to obtain this case.
;;
;;  EXECUTION TIME PER SAMPLE 
;;      1us or 1.1us per inner loop interation (see above).
;;
;;  DSPWRAP ARGUMENT INFO
;;      cvmandelbrot (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout, (output)aout, iout, cnt, lim
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPcvmandelbrot(aS,iS,aD,iD,n,l); /* Mandelbrot set
;;          aS  =   S source vector base address (S complex)
;;          iS  =   S address increment (2 for successive elements)
;;          aD  =   D destination vector base address
;;          iD  =   D address increment (2 for successive elements)
;;          n   =   element count (number of complex values)
;;          l   =   limit on number of iterations allowed per element   */
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvmandelbrot.asm
;;      
        define pfx "pf\_cvmandelbrot_\ic\_"
	define macpfx """pfx"""

cvmandelbrot macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0,lim0

        ; argument definitions
        new_xarg    macpfx,ainp,ainp0
        new_xarg    macpfx,iinp,iinp0
        new_xarg    macpfx,aout,aout0
        new_xarg    macpfx,iout,iout0
        new_xarg    macpfx,cnt,cnt0
        new_xarg    macpfx,lim,lim0

        ; get inputs
        move            x:(R_X)+,R_I1   ; input vector address
        move            x:(R_X)+,N_I1   ; input vector increment 
        move 	        x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,N_O    ; output vector increment
        move            x:(R_X)+,A      ; count
        move            x:(R_X)+,R_I2   ; limit
     
        tst     A
        jle pfx\loop  ; test A to protect against count=0

        do A,pfx\loop
		move    sinp:(R_I1)+,X0	; dr
		move    sinp:(R_I1)-,Y0	; di
		lua (R_I1)+N_I1,R_I1	; skip by skip factor
		tfr X0,A  X0,X1		; wr
		tfr Y0,B  Y0,Y1		; wi

	        do R_I2,pfx\iloop
		    mpy     Y0,Y0,B	A,X0	; ti, new wr (possibly clipped)
		    mpy     X0,X0,A		; tr
		    add     A,B			; |w|^2
;*		    special_jcc(jec,pfx\no_break)
		    jec >pfx\no_break
			move R_I2,A
			move lc,X0
			sub X0,A
			enddo
			jmp >pfx\aloop
pfx\no_break	    

		    mpy     X0,Y0,B      	; wr * wi
		    asl     B			; can't say addl Y1,B !!!
		    asl	    B 			; 4 * wr * wi
		    add     Y1,B

		    mac     -Y0,Y0,A	B,Y0	; tr - ti, new wi
		    asl     A	     		; can't say addl X1,A !!!
		    add     X1,A		; new wr = 2 * (tr - ti) + dr
pfx\iloop

		move R_I2,A
pfx\aloop
		move A,sout:(R_O)+N_O
pfx\loop
        endm

; special_jcc macro jcc,lab
;	if @def(EXECUTE_IN_VECTORS)
;	    jcc     <lab
;	else
;	    jcc     lab
;	endif

