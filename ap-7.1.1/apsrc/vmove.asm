;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      04/10/87/jos - initial file created from DSPAPSRC/template
;;      11/13/87/jms - added test for count=0; cosmetic changes to documentation
;;      02/15/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      06/12/89/mtm - rearrange comments to match dspwrap
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vmove (AP macro) - vector move - copy vector from one location to another
;;      
;;  SYNOPSIS
;;      include 'ap_macros'      ; load standard dsp macro package
;;      vmove    pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0 ; invoke ap macro vmove
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vmove_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      iinp0   =   initial increment for input vector address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial output vector memory address
;;      iout0   =   initial increment for output vector address
;;      cnt0    =   initial element count
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
;;      The vmove array-processor macro copies the elements of one vector
;;      to another vector of the same length.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = sinp:ainp[n*iinp];
;;      
;;  MEMORY USE
;;      5 x-memory arguments
;;      sinp==sout: 13 program memory locations
;;      sinp!=sout: 12 program memory locations
;;      
;;  EXECUTION TIME
;;      cnt*2+13 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vmove (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvmove(S,iS,D,iD,n);   /* vector move:
;;          S   =   Source vector base address
;;          iS  =   S address increment
;;          D   =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvmove'              ; begin ap main program
;;      new_yeb srcvec,10,$400000   ; allocate input vector
;;      new_yeb outvec,10,0         ; allocate output vector
;;      vmove    ap,1,y,srcvec,1,y,outvec,1,10 
;;      end_ap  'tvmove'              ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vmove.asm
;;      
;;  REGISTER USE
;;      A       temporary storage for transfer
;;      B       count
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vmove    macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
     
        ; argument definitions
        new_xarg    pf\_vmove_\ic\_,ainp,ainp0   ; input address 
        new_xarg    pf\_vmove_\ic\_,iinp,iinp0   ; input address increment 
        new_xarg    pf\_vmove_\ic\_,aout,aout0   ; output address 
        new_xarg    pf\_vmove_\ic\_,iout,iout0   ; output address increment 
        new_xarg    pf\_vmove_\ic\_,cnt,cnt0     ; element count 
     
        ; get inputs
        move            x:(R_X)+,R_I1           ; input address to R_I1
        move            x:(R_X)+,N_I1           ; address increment to N_I1
        move            x:(R_X)+,R_O            ; output address to R_O
        move            x:(R_X)+,N_O            ; address increment to N_O

        ; set up loop and pipelining
        move            x:(R_X)+,B
        tst     B       sinp:(R_I1)+N_I1,A      ; get first input term
        jeq     pf\_vmove_\ic\_aploop            ; protect against count=0

        ; inner loop
        do      B,pf\_vmove_\ic\_aploop
            if "sinp"=="sout"
                move    A,sout:(R_O)+N_O        ; vector move
                move    sinp:(R_I1)+N_I1,A
            else
                move    sinp:(R_I1)+N_I1,A  A,sout:(R_O)+N_O 
            endif
pf\_vmove_\ic\_aploop    
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

