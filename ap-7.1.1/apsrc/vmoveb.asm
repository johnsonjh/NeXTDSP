;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      09/28/87/jos - initial file created from DSPAPSRC/vmove.asm
;;      09/28/87/jos - passed test/tvmoveb.asm
;;      02/15/88/jms - added OPTIONAL_NOP; moved _aploop outside of if...endif
;;      02/17/88/jms - added protection against cnt=0
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      06/12/89/mtm - rearrange comments to match dspwrap
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vmoveb (AP macro) - vector move backwards 
;;          - copy vector from one location to another, stepping backwards from end
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard dsp macro package
;;      vmoveb   pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0 ; invoke ap macro vmoveb
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vmoveb_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector memory address
;;      iinp0   =   increment for input vector address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector memory address
;;      iout0   =   increment for output vector address
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Source address          ainp0
;;      x:(R_X)+    Source increment        iinp0
;;      x:(R_X)+    Destination address     aout0
;;      x:(R_X)+    Destination increment   iout0
;;      x:(R_X)+    element count           cnt0
;;      
;;  DESCRIPTION
;;      The vmoveb array-processor macro copies the elements of one vector
;;      to another vector of the same length, iterating from the last sample
;;      of each vector back to the first. The purpose of providing this
;;      version of vmove is to allow forward memory to memory transfers
;;      in which the destination memory segment overlaps the first.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      aei     =   ainp + iinp*(cnt-1);    /* end-address for  input vector */
;;      aeo     =   aout + iout*(cnt-1);    /* end-address for output vector */
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[aeo-n*iout] = sinp:ainp[aei-n*iinp];
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      sinp==sout: 28 program memory locations
;;      sinp!=sout: 25 program memory locations
;;      
;;  EXECUTION TIME 
;;      sinp==sout: cnt*2+25 instruction cycles
;;      sinp!=sout: cnt*2+29 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vmoveb (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvmoveb(S,iS,D,iD,n);  /* vector move:
;;          S   =   Source vector base address
;;          iS  =   S address increment
;;          D   =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvmoveb'            ; begin ap main program
;;      new_yeb srcvec,10,$400000   ; allocate input vector
;;      new_yeb outvec,10,0         ; allocate output vector
;;      vmoveb   ap,1,y,srcvec,1,y,outvec,1,10
;;      end_ap  'tvmoveb'            ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vmoveb.asm
;;      
;;  REGISTER USE
;;      X0      element count
;;      X1      temporary storage for increments
;;      A       temporary storage for transfer
;;      B       temporary storage
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vmoveb   macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vmoveb_\ic\_,ainp,ainp0  ; input address arg
        new_xarg    pf\_vmoveb_\ic\_,iinp,iinp0  ; in address increment arg
        new_xarg    pf\_vmoveb_\ic\_,aout,aout0  ; output address arg
        new_xarg    pf\_vmoveb_\ic\_,iout,iout0  ; address increment arg
        new_xarg    pf\_vmoveb_\ic\_,cnt,cnt0    ; element count arg

        ; get inputs
        clr     A       x:(R_X)+,R_I1   ; input address to R_I1
        move            x:(R_X)+,N_I1   ; address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; address increment to N_O
        move            x:(R_X)+,B      ; element count to B

        ; set up loop and pipelining
        tst     B       B,X0            ; element count to X0
        jeq     pf\_vmoveb_\ic\_aploop   ; protect against count = 0

        clr     B       R_I1,A0         ; input start address
        addl    B,A     N_I1,X1         ; rsr on A including A0, input incr
        mac     X1,X0,A                 ; end address plus N_I1 (*2) to A
        addr    B,A                     ; rsr on A including A0
        move            A0,R_I1

        move            R_O,A0          ; output start address
        addl    B,A     N_O,X1          ; rsr on A including A0
        mac     X1,X0,A                 ; end address plus N_O (*2) to A
        addr    B,A     N_I1,B          ; rsr on A including A0, B used below
        move            A0,R_O

        lua             (R_I1)-N_I1,R_I1 
        lua             (R_O)-N_O,R_O

        ; inner loop 
        if "sinp"=="sout"
            do      X0,pf\_vmoveb_\ic\_aploop
                move            sinp:(R_I1)-N_I1,A
                move            A,sout:(R_O)-N_O    ; vector move
        else
            neg      B      N_O,A               ; must negate postincrements
            neg      A      B,N_I1              ;   because (Rn)-Nn can't be
            move            A,N_O               ;   used in a parallel move.
            move            sinp:(R_I1)+N_I1,A  ; make first output valid
            do       X0,pf\_vmoveb_\ic\_aploop
                move            sinp:(R_I1)+N_I1,A A,sout:(R_O)+N_O ; vector move
        endif
pf\_vmoveb_\ic\_aploop    
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

