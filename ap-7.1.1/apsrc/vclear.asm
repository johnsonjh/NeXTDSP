;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      04/09/87/jos - initial file created from template
;;      05/06/87/jos - passed test/tvclear.asm
;;      02/15/88/jms - changed do loop to rep
;;      02/17/88/jms - added protection against cnt = 0
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/27/88/jms - added missing MEMORY USE, EXECUTION TIME
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vclear (AP macro) - vector clear - clear the elements of a vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros'                 ; load standard dsp macro package
;;      vclear    pf,ic,sout,aout0,iout0,cnt0 ; invoke array-processor macro vclear
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vclear_\ic is globally unique)
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial output vector memory address
;;      iout0   =   initial increment for output vector address
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Current output address  aout0
;;      x:(R_X)+    Current increment       iout0
;;      x:(R_X)+    Current element count   cnt0
;;      
;;  DESCRIPTION
;;      The vclear macro sets all of the elements of a vector to zero.
;;
;;  PSEUDO-C NOTATION
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 0;
;;      
;;  MEMORY USE
;;      3 x-memory arguments
;;      9 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt+10 instruction cycles
;;
;;  DSPWRAP ARGUMENT INFO
;;      vclear (prefix)pf,(instance)ic,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvclear(A,i,n);     /* Vector clear:
;;          A   =   Destination vector base address
;;          i   =   A address increment
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvclear'             ; begin ap main program
;;      new_yeb outvec,100,$FF      ; allocate output vector containing $FF
;;      vclear    ap,1,y,outvec,1,100 ; invocation
;;      end_ap  'tvclear'             ; end of ap main program
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vclear.asm
;;      
;;  REGISTER USE
;;      A       0
;;      B       count
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
vclear    macro pf,ic,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vclear_\ic\_,aout,aout0       ; output address arg
        new_xarg    pf\_vclear_\ic\_,iout,iout0       ; address increment arg
        new_xarg    pf\_vclear_\ic\_,cnt,cnt0         ; element count arg

        ; get inputs
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; address increment to N_O
        clr     A       x:(R_X)+,B      ; source of 0 in A; count in B

        ; inner loop 
        tst     B
        jeq     pf\_vclear_\ic\_loop      ; protect against cnt = 0
        rep     B
            move            A,sout:(R_O)+N_O    ; clear output vector
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; after a rep. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after a rep. 
pf\_vclear_\ic\_loop
        endm

