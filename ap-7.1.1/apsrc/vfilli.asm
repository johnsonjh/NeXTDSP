;;  Copyright 1986 by NeXT Inc.
;;  Author - Julius Smith
;;      
;;  Modification history
;;  --------------------
;;      04/10/87/jos - initial file created from DSPAPSRC/vfill.asm
;;      02/17/88/jms - changed do loop  to rep
;;                   - added protection against cnt = 0 
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      06/12/89/mtm - rearrange comments to match dspwrap
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vfilli (AP macro) - vector fill immediate - fill vector with arg constant
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard dsp macro package
;;      vfilli  pf,ic,cnst0,sout,aout0,iout0,cnt0 ; invoke ap macro vfilli
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vfilli_\ic is globally unique)
;;      cnst0   =   initial fill constant
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial output vector memory address
;;      iout0   =   initial increment for output vector address
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description             Initialization
;;      ------      -----------             --------------
;;      x:(R_X)+    Fill constant           cnst0
;;      x:(R_X)+    Output address          aout0
;;      x:(R_X)+    Output increment        iout0
;;      x:(R_X)+    element count           cnt0
;;      
;;  DESCRIPTION
;;      The vfilli array-processor macro copies the contents of a dsp memory
;;      argument into each element of the specified vector.
;;
;;  PSEUDO-C NOTATION
;;      cnst    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = cnst;
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      10 program memory locations 
;;      
;;  EXECUTION TIME 
;;      cnt+11 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vfilli (prefix)pf,(instance)ic,
;;          cnst,(dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvfilli(c,y,iy,n);    /* vector fill immediate:
;;          c   =   fill-constant (type DSP24)
;;          y   =   destination vector base address in DSP memory
;;          iy  =   y address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tvfilli'           ; begin ap main program
;;      new_yeb outvec,100,0        ; allocate output vector
;;      vfilli ap,1,$a1,y,outvec,1,100
;;      end_ap  'tvfilli'           ; end of ap main program
;;
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vfilli.asm
;;      
;;  REGISTER USE
;;      A       fill constant
;;      B       count
;;      R_I1    pointer to fill term
;;      R_O     running pointer to output terms
;;      N_O     increment for output vector
;;      
vfilli  macro pf,ic,cnst0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vfilli_\ic\_,cnst,cnst0     ; constant address arg
        new_xarg    pf\_vfilli_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_vfilli_\ic\_,iout,iout0     ; address increment arg
        new_xarg    pf\_vfilli_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs
        move            x:(R_X)+,A     ; fill constant to A
        move            x:(R_X)+,R_O   ; fill address to R_O
        move            x:(R_X)+,N_O   ; address increment to N_O
        move            x:(R_X)+,B

        ; set up loop 
        tst     B
        jeq     pf\_vfilli_\ic\_loop    ; protect against cnt = 0

        ; inner loop 
        rep  B
        move            A,sout:(R_O)+N_O        ; vector fill immediate
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; after a rep. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after a rep. 
pf\_vfilli_\ic\_loop
        endm

