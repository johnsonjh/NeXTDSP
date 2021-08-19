;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      06/22/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - change neg A to tst A in initialization
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX.
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      sumvnolim (AP macro) - vector element sum, no limiting
;;          - sum vector to a scalar, but bypass 56000 ALU limiting
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      sumvnolim pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro sumvnolim
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_sumvnolim_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   initial input vector memory address
;;      iinp0   =   initial increment for input vector address
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   initial address for output scalar
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    Input Vector base address       ainp0
;;      x:(R_X)+    Input Address increment         iinp0
;;      x:(R_X)+    Output                          aout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The sumvnolim array-processor macro computes the sum of the elements
;;      of vector ainp, with no on-chip limiting.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      aout = 0;
;;      for (n=0;n<cnt;n++) {
;;          sout:aout += ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      12 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt*2+13 DSP 
;;      
;;  DSPWRAP ARGUMENT INFO
;;      sumvnolim (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPsumvnolim(A,ia,B,n);   /* vector element sum no limiting:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros'         ; utility macros
;;      beg_ap  'tsumvnolim'         ; begin ap main program
;;      beg_yeb                     ; allocate input vector
;;tsumvnolim_vec dc 1,2,3
;;      end_yeb
;;      new_xeb sum1,1,0            ; allocate space for output
;;      sumvnolim ap,1,y,tsumvnolim_vec,1,x,sum1,3
;;      end_ap  'tsumvnolim'         ; end of ap main program
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/sumvnolim.asm
;;      
;;  REGISTER USE
;;      A       test for cnt == 0 
;;      B       sum of vector elements
;;      X0      current addend
;;      X1      cnt
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     pointer to ouptut term
;;      
sumvnolim macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument definitions
        new_xarg    pf\_sumvnolim_\ic\_,ainp,ainp0       ; input address arg
        new_xarg    pf\_sumvnolim_\ic\_,iinp,iinp0       ; input increment arg
        new_xarg    pf\_sumvnolim_\ic\_,aout,aout0       ; output address arg
        new_xarg    pf\_sumvnolim_\ic\_,cnt,cnt0         ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1
        move            x:(R_X)+,N_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        move            x:(R_X)+,A      ; cnt goes to A

        ; set up loop and pipelining
        clr     B       A,X1            ; copy cnt0 into X1
        tst     A       B,X0            ; clear X0
        jeq     pf\_sumvnolim_\ic\_l1    ; guard against cnt=0

        ; inner loop 
        rep     X1
            add     X0,B    sinp:(R_I1)+N_I1,X0 ; sum elements
        add     X0,B                            ; pick up last one   
pf\_sumvnolim_\ic\_l1      
        move            B1,sout:(R_O)           ; B=0 when cnt=0
        endm

