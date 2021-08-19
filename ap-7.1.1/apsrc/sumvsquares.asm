;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      09/10/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - change "...\ic\_\l..." to "...\ic\_l..."
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX.
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      sumvsquares (AP macro) - sum of signed vector squares
;;          - sum signed squares of vector elements to a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      sumvsquares     pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro sumvsquares
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_sumvsquares_\ic is globally unique)
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
;;      The sumvsquares array-processor macro computes the sum of the signed squares
;;      of the elements of vector ainp.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      aout = 0;
;;      for (n=0;n<cnt;n++) {
;;          sout:aout += signum(sinp:ainp[n*iinp]) * 
;;              sinp:ainp[n*iinp] * sinp:ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE
;;      4 x-memory arguments
;;      18 program memory locations
;;      
;;  EXECUTION TIME
;;      14 instruction cycles
;;         + 6 DSP instruction cycles for each positive element in vector
;;         + 7 DSP instruction cycles for each negative element in vector
;;      
;;  DSPWRAP ARGUMENT INFO
;;      sumvsquares (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPsumvsquares(A,ia,B,n);    /* sum of vector signed element squares:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'stdmacro'
;;      beg_ap  'tsumvsquares'
;;      symobj  tsumvsquares_vec
;;      beg_yeb
;;tsumvsquares_vec dc -10000,20000,-30000,40000,-50000,60000,-70000,80000,-90000,100000,-110000,120000
;;      end_yeb
;;      new_xeb sum1,1,$777
;;      sumvsquares     ap,1,y,tsumvsquares_vec,1,x,sum1,12
;;      end_ap  'tsumvsquares'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/sumvsquares.asm
;;      
;;  REGISTER USE 
;;      A       accumulator
;;      B       cnt; negative of current element
;;      X0      current element
;;      X1      count
;;      Y0      current element
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     pointer to ouptut term
;;      
sumvsquares     macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_sumvsquares_\ic\_,ainp,ainp0     ; input address arg
        new_xarg    pf\_sumvsquares_\ic\_,iinp,iinp0     ; input increment arg
        new_xarg    pf\_sumvsquares_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_sumvsquares_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1
        move            x:(R_X)+,N_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        clr     A       x:(R_X)+,B      ; cnt goes to B

        ; set up loop and pipelining
        tst      B       B,X1           ; store cnt in X1 too
        jeq      pf\_sumvsquares_\ic\_l3        ; guard against cnt=0
        move             sinp:(R_I1),B  ; move input element A[n] to B

        ; inner loop 
        do      X1,pf\_sumvsquares_\ic\_l2
            neg     B       sinp:(R_I1),X0      ; move same element to X0
            tst     B       sinp:(R_I1)+N_I1,Y0 ; move same element to Y0
            jle     pf\_sumvsquares_\ic\_l1             ; if A[n] < 0,
            move            B1,Y0               ;   move -A[n] to Y0
pf\_sumvsquares_\ic\_l1  
            mac     X0,Y0,A sinp:(R_I1),B       ; square, accumulate
pf\_sumvsquares_\ic\_l2  
        rnd     A
pf\_sumvsquares_\ic\_l3  
        move    A,sout:(R_O)                    ; A=0 when cnt=0
        endm

