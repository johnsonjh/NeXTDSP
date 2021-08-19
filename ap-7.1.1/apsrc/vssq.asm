;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;
;;  Modification history
;;  --------------------
;;      07/22/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - removed "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vssq (AP macro) - vector signed square 
;;          - multiply each element of a vector by the absolute value of that element
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vssq    pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vssq_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address (address of A[1,1])
;;      iinp0   =   initial increment for input vector
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   initial increment for output vector 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS 
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector base address       ainp0
;;      x:(R_X)+    input vector increment          iinp0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vssq array-processor macro computes C[k]=A[k]*ABS(A[k]).
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp:ainp[n*iinp] * ABS(sinp:ainp[n*iinp]);
;;
;;  MEMORY USE   
;;      5 x-memory arguments
;;      sinp==sout: 32 program memory locations
;;      sinp!=sout: 36 program memory locations
;;      
;;  EXECUTION TIME 
;;                  cnt even        cnt odd
;;      sinp==sout: cnt*3+28        cnt*3+30 instruction cycles
;;      sinp!=sout: cnt*3+34        cnt*3+36 
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vssq (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvssq(aS,iS,aD,iD,n); /* D[k] = S[k] * ABS(S[k])
;;          aS  =   Source vector base address
;;          iS  =   S address increment
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvssq'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;          radix   A  
;;      end_xeb
;;      new_xeb xscal,1,$123
;;      new_xeb out1_vec,7,$777
;;      vssq    ap,1,x,ax_vec,1,x,out1_vec,1,6
;;      end_ap  'tvssq'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vssq.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp during initialization
;;      B       accumulator; temp during initialization
;;      X0, X1, Y0, Y1 cnt/2 and input vector terms. Exact
;;              usage depends on sinp and sout.
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_L     secondary running point to input vector terms, for sinp!=sout 
;;      N_L     secondary increment for input vector, for sinp!=sout 
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      N_Y     storage for R_L
;;      
vssq    macro pf,ic,sinp,ainp0,iinp0,sout,aout0,iout0,cnt0

        ; argument declarations
        new_xarg    pf\_vssq_\ic\_,ainp,ainp0
        new_xarg    pf\_vssq_\ic\_,iinp,iinp0
        new_xarg    pf\_vssq_\ic\_,aout,aout0
        new_xarg    pf\_vssq_\ic\_,iout,iout0
        new_xarg    pf\_vssq_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,R_I1   ; input vector base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        clr A           x:(R_X)+,B      ; cnt to B

        ; set up loop and pipelining
        lsr     B       R_L,N_Y         ; B gets cnt/2, store R_L until end of loop 
                                        ;    (R_L used only for sinp!=sout)
        jcc     pf\_vssq_\ic\_l1
        move            #1,A1           ; A gets "cnt odd" flag
pf\_vssq_\ic\_l1     
        move            A1,N_X          ; N_X gets "cnt even/odd" flag

	; inner loop, more pipeline setup
        if "sinp"=="sout"
            move            sinp:(R_I1)+N_I1,A
            abs     A       A,X0
            move            A,Y0
            mpyr    X0,Y0,A B1,Y1
            tst     B                           ; guard against cnt=0 or cnt=1
            jeq     pf\_vssq_\ic\_l2   
            move            sinp:(R_I1)+N_I1,B  ; complete pipeline startup 
            if "sinp"=='x' 
                move            B,X1
                do      Y1,pf\_vssq_\ic\_l2
                    abs     B       A,sout:(R_O)+N_O
                    move            sinp:(R_I1)+N_I1,A  B,Y1
                    mpyr    X1,Y1,B A,X0
                    abs     A       B,sout:(R_O)+N_O
                    move            sinp:(R_I1)+N_I1,B  A,Y0
                    mpyr    X0,Y0,A B,X1
            else 
                move            B,Y0
                do      Y1,pf\_vssq_\ic\_l2
                    abs     B       A,sout:(R_O)+N_O
                    move            B,X0    sinp:(R_I1)+N_I1,A
                    mpyr    X0,Y0,B A,Y1
                    abs     A       B,sout:(R_O)+N_O
                    move            A,X1    sinp:(R_I1)+N_I1,B
                    mpyr    X1,Y1,A B,Y0
            endif
pf\_vssq_\ic\_l2                        ; Also, jump to here for cnt=0 or cnt=1.
        else
            ; set up address registers (don't disturb cnt/2 in b!)
            move            R_I1,R_L
            move            N_I1,N_L
            move            N_I1,A
            lsl     A       (R_L)+N_L   ; A gets 2*N_I1,
                                        ; R_L starts at *2nd* input datum
            move            A,N_I1      ; R_I1 and R_L will now pick off 
            move            A,N_L       ;    every *other* input datum
            ; set up pipeline   
            move            sinp:(R_I1)+N_I1,A
            abs     A       A,X0
            move            A,Y0
            mpyr    X0,Y0,A B1,Y1
            tst     B                   ; guard against cnt=0 or cnt=1
            jeq     pf\_vssq_\ic\_l3   
            move            sinp:(R_L),B
            do      Y1,pf\_vssq_\ic\_l3
                if "sinp"=='x' 
                    abs     B       sinp:(R_I1),A       A,sout:(R_O)+N_O
                    abs     A       sinp:(R_L)+N_L,X1   B,Y1
                    mpyr    X1,Y1,B sinp:(R_I1)+N_I1,X0 A,Y0
                    mpyr    X0,Y0,A sinp:(R_L),B        B,sout:(R_O)+N_O
                else 
                    abs     B       A,sout:(R_O)+N_O    sinp:(R_I1),A
                    abs     A       B,X1                sinp:(R_L)+N_L,Y1
                    mpyr    X1,Y1,B A,X0                sinp:(R_I1)+N_I1,Y0
                    mpyr    X0,Y0,A B,sout:(R_O)+N_O    sinp:(R_L),B
                endif
pf\_vssq_\ic\_l3                        ; Also, jump to here for cnt=0 or cnt=1.
            move            N_Y,R_L     ; restore R_L
        endif
        move            N_X,B           ; B gets cnt even/odd flag. 
        tst     B
        jeq     pf\_vssq_\ic\_l4        ; if cnt odd (including cnt=1),
        move    A,sout:(R_O)+N_O        ;    store (final) element 
pf\_vssq_\ic\_l4
        endm

