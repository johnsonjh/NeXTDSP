;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;  Modification history
;;  --------------------
;;      07/21/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - changed neg B to tst B in initialization
;;                   - removed "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vts (AP macro) - vector scalar multiply 
;;          - multiply elements of a vector by a scalar
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vts   pf,ic,sinp,ainp0,iinp0,sins,ains0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vts_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address (address of A[1,1])
;;      iinp0   =   initial increment for input vector
;;      sins    =   scalar memory space ('x' or 'y')
;;      ains0   =   scalar address
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
;;      x:(R_X)+    scalar address                  ains0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vts array-processor macro computes C[k]=A[k]*s.
;;
;;  PSEUDO-C NOTATION
;;      ainp    =   x:(R_X)+;
;;      iinp    =   x:(R_X)+;
;;      ains    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = sinp:ainp[n*iinp] * sins:ains;
;;      
;;  MEMORY USE   
;;      6 x-memory arguments
;;      sinp==sout: 29 program memory locations
;;      sinp!=sout: 27 program memory locations
;;      
;;  EXECUTION TIME 
;;      cnt even: cnt*4+27 instruction cycles
;;      cnt odd:  cnt*4+29 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vts (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,
;;          (dspace)sins,ains,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvts(aS,iS,as,aD,iD,n); /* D[k] = S[k] * s
;;          aS  =   Source vector 1 base address
;;          iS  =   S address increment
;;          as  =   address of scalar
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvts'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;          radix   A  
;;      end_xeb
;;      new_xeb xscal,1,$123
;;      new_xeb out1_vec,7,$777
;;      vts   ap,1,x,ax_vec,1,x,xscal,x,out1_vec,1,6
;;      end_ap  'tvts'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vts.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp during initialization
;;      B       accumulator; temp during initialization
;;      X0, X1, Y0, Y1 cnt/2, input vector terms, and input scalar. Exact
;;      usage depends on sinp and sout.
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_I2    pointer to scalar
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      N_X     count even/odd flag
;;      
vts   macro pf,ic,sinp,ainp0,iinp0,sins,ains0,sout,aout0,iout0,cnt0

        ; argument declarations
        new_xarg    pf\_vts_\ic\_,ainp,ainp0
        new_xarg    pf\_vts_\ic\_,iinp,iinp0
        new_xarg    pf\_vts_\ic\_,ains,ains0
        new_xarg    pf\_vts_\ic\_,aout,aout0
        new_xarg    pf\_vts_\ic\_,iout,iout0
        new_xarg    pf\_vts_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,R_I1   ; input vector base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector address increment to N_I1
        move            x:(R_X)+,R_I2   ; input scalar address to R_I2
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        clr     A       x:(R_X)+,B      ; cnt

        ; set up loop and pipelining
        lsr     B                       ; B gets cnt/2 
        jcc     pf\_vts_\ic\_l1
        move            #1,A1           ; A gets "cnt odd" flag
pf\_vts_\ic\_l1     
        move            A1,N_X          ; N_X gets "cnt even/odd" flag

        if "sinp"=="sout"
            move            B1,Y1               ; copy cnt0/2 into Y1 for loop
            move            sinp:(R_I1)+N_I1,X0 ; start pipelining 
            move            sins:(R_I2),Y0      ; Y0 will have scalar throughout  
            mpyr    X0,Y0,A sinp:(R_I1)+N_I1,X1
            tst     B                           ; guard against cnt=0 or cnt=1
            jeq     pf\_vts_\ic\_l3   
            ; inner loop 
            do      Y1,pf\_vts_\ic\_l3
            mpyr    X1,Y0,B sinp:(R_I1)+N_I1,X0 
            move            A,sout:(R_O)+N_O
            mpyr    X0,Y0,A sinp:(R_I1)+N_I1,X1 
            move            B,sout:(R_O)+N_O    

        else
            if "sinp"=='x' 
                move            B1,Y1                   ; copy cnt0/2 into Y1 for loop
                move            sinp:(R_I1)+N_I1,X0     ; start pipelining 
                move            sins:(R_I2),Y0          ; Y0 will have scalar throughout  
                mpyr    X0,Y0,A sinp:(R_I1)+N_I1,X1
                tst     B                               ; guard against cnt=0 or cnt=1
                jeq     pf\_vts_\ic\_l3   
                ; inner loop 
                do      Y1,pf\_vts_\ic\_l3
                    mpyr    X1,Y0,B sinp:(R_I1)+N_I1,X0 A,sout:(R_O)+N_O
                    mpyr    X0,Y0,A sinp:(R_I1)+N_I1,X1 B,sout:(R_O)+N_O    
            else 
                move             B1,X1                  ; copy cnt0/2 into X1 for loop
                move             sinp:(R_I1)+N_I1,Y0    ; start pipelining 
                move             sins:(R_I2),X0         ; X0
                mpyr    X0,Y0,A  sinp:(R_I1)+N_I1,Y1
                tst     B                               ; guard against cnt=0 or cnt=1
                jeq     pf\_vts_\ic\_l3   
                ; inner loop 
                do  X1,pf\_vts_\ic\_l3
                    mpyr    X0,Y1,B A,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,Y0
                    mpyr    X0,Y0,A b,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,Y1
            endif
        endif
                                            ; Loops all end here for any sinp, sout.
pf\_vts_\ic\_l3                           ; Also, jump to here for cnt=0 or cnt=1.
        move    N_X,B                       ; B gets cnt even/odd flag. 
        tst     B    
        jeq     pf\_vts_\ic\_l2   ; if cnt odd (including cnt=1),
        move            A,sout:(R_O)+N_O    ;    store (final) element
pf\_vts_\ic\_l2
        endm

