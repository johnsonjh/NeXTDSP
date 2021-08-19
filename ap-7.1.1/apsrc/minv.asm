;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn 
;;      
;;  Modification history
;;  --------------------
;;      09/11/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - remove "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;                   - change "...\ic\_\l..." to "...\ic\_l..."
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;	05/19/89/mtm - fix DSPWRAP C SYNTAX
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      minv (AP macro) - vector minimum 
;;          - write minimum of vector elements to a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      minv    pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0; invoke ap macro minv
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_minv_\ic is globally unique)
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
;;      The minv array-processor macro finds the minimum of 
;;      the elements of vector ainp.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;      
;;      aout = +infinity;
;;      for (n=0;n<cnt;n++) {
;;          if (sinp:ainp[n*iinp] < aout)
;;              aout = sinp:ainp[n*iinp];
;;      }
;;      
;;  MEMORY USE 
;;      4 x-memory arguments
;;      28 program memory locations
;;      
;;  EXECUTION TIME 
;;      2*cnt+26 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      minv (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPminv(A,ia,B,n);   /* vector minimum:
;;          A   =   input vector base address in DSP memory
;;          ia  =   A address increment
;;          B   =   address of output
;;          n   =   element count       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'stdmacro'
;;      beg_ap  'tminv'
;;      symobj  tminv_vec
;;      beg_yeb
;;tminv_vec dc -10000,20000,-30000,40000,-50000,60000,-70000,80000,-90000,100000,-110000,120000
;;      end_yeb
;;      new_xeb sum1,1,$777
;;      minv    ap,1,y,tminv_vec,1,x,sum1,12
;;      end_ap  'tminv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/minv.asm
;;      
;;  REGISTER USE 
;;      X0      current element
;;      Y0      current element
;;      Y1      flag for cnt even/odd 
;;      A       accumulator
;;      B       cnt
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input  vector
;;      R_O     running pointer to ouptut term
;;      
minv    macro pf,ic,sinp,ainp0,iinp0,sout,aout0,cnt0

        ; argument declarations
        new_xarg    pf\_minv_\ic\_,ainp,ainp0     ; input address arg
        new_xarg    pf\_minv_\ic\_,iinp,iinp0     ; input increment arg
        new_xarg    pf\_minv_\ic\_,aout,aout0     ; output address arg
        new_xarg    pf\_minv_\ic\_,cnt,cnt0       ; element count arg

        ; get inputs
        move            x:(R_X)+,R_I1
        move            x:(R_X)+,N_I1
        move            x:(R_X)+,R_O    ; output address goes to R_O
        clr     A       x:(R_X)+,B      ; cnt goes to B

        ; set up loop and pipelining
        tst     B       sinp:(R_I1)+N_I1,X0 ; first element to X0, for pipeline setup
        jeq     pf\_minv_\ic\_l4        ; guard against cnt=0
        lsr     B    
        jcc     pf\_minv_\ic\_l1
        move            #1,A1           ; A gets "cnt odd" flag
pf\_minv_\ic\_l1     
        move            A1,Y1           ; Y1 gets "cnt even/odd" flag
        tst     B       X0,A            ; copy first vector element into A
        jeq     pf\_minv_\ic\_l3        ; guard against cnt=1

        ;;  the first of the two cmps in the loop will be a dummy
        ;;  cmp during the first pass through the loop. 
        ; inner loop 
        do     B,pf\_minv_\ic\_l2
            cmp    X0,A      sinp:(R_I1)+N_I1,Y0
            tgt    X0,A
            cmp    Y0,A      sinp:(R_I1)+N_I1,X0
            tgt    Y0,A
pf\_minv_\ic\_l2  
        move           Y1,B
        tst    B    
        jeq    pf\_minv_\ic\_l3         ; if cnt odd, 
        cmp    X0,A                     ;   do odd term
        tgt    X0,A
pf\_minv_\ic\_l3  
        move           A,sout:(R_O)     ; (a contains first element for cnt=1)
pf\_minv_\ic\_l4                        ; jump to here for cnt=0
        endm

