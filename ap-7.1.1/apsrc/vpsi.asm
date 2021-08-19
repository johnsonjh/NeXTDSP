;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      07/21/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - remove "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vpsi (AP macro) - vector scalar add immediate
;;          - add immediate scalar to elements of a vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vpsi  pf,ic,sinp,ainp0,iinp0,ains0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vpsi_\ic is globally unique)
;;      sinp    =   input vector memory space ('x' or 'y')
;;      ainp0   =   input vector base address (address of A[1,1])
;;      iinp0   =   initial increment for input vector
;;      ains0   =   scalar 
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
;;      x:(R_X)+    scalar                          ains0
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vpsi array-processor macro computes C[k]=A[k]+s.
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
;;          sout:aout[n*iout] = sinp:ainp[n*iinp] + ains;
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
;;      vpsi (prefix)pf,(instance)ic,
;;          (dspace)sinp,(input)ainp,iinp,ains,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvpsi(aS,iS,s,aD,iD,n); /* D[k] = S[k] + s
;;          aS  =   Source vector 1 base address
;;          iS  =   S address increment
;;          s   =   scalar
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count               */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvpsi'
;;      symobj  ax_vec
;;      beg_xeb
;;          radix 16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;          radix A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      vpsi ap,1,x,ax_vec,1,$123,x,out1_vec,1,6
;;      end_ap  'tvpsi'
;;      end
;;
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vpsi.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp during initialization
;;      B       accumulator; temp during initialization
;;      X1      cnt/2
;;      Y0      scalar
;;      Y1      flag for cnt even or odd
;;      R_I1    running pointer to input vector terms
;;      N_I1    increment for input vector
;;      R_O     running pointer to ouput vector terms
;;      N_O     increment for output vector
;;      
vpsi  macro pf,ic,sinp,ainp0,iinp0,ains0,sout,aout0,iout0,cnt0

        ; argument declarations
        new_xarg    pf\_vpsi_\ic\_,ainp,ainp0
        new_xarg    pf\_vpsi_\ic\_,iinp,iinp0
        new_xarg    pf\_vpsi_\ic\_,ains,ains0
        new_xarg    pf\_vpsi_\ic\_,aout,aout0
        new_xarg    pf\_vpsi_\ic\_,iout,iout0
        new_xarg    pf\_vpsi_\ic\_,cnt,cnt0

        ; get inputs
        move            x:(R_X)+,R_I1       ; input vector base address to R_I1
        move            x:(R_X)+,N_I1       ; input vector address increment to N_I1
        move            x:(R_X)+,Y0         ; Y0 will have scalar throughout   
        move            x:(R_X)+,R_O        ; output address to R_O
        move            x:(R_X)+,N_O        ; output increment to N_O
        clr     A       x:(R_X)+,B          ; count to B

        ; set up loop and pipelining
        lsr     B                           ; B gets cnt/2 
        jcc     pf\_vpsi_\ic\_l1
        move            #1,A1               ; A gets "cnt odd" flag
pf\_vpsi_\ic\_l1     
        move            B1,X1               ; copy cnt0/2 into X1 for loop
        move            A1,Y1               ; Y1 gets "cnt even/odd" flag
        move            sinp:(R_I1)+N_I1,A  ; start pipelining
        add     Y0,A           
        tst     B                           ; guard against cnt=0 or cnt=1
        jeq     pf\_vpsi_\ic\_l3   
        move            sinp:(R_I1)+N_I1,B  ; continue pipeline initialization

        ; inner loop 
        do      X1,pf\_vpsi_\ic\_l3
            if "sinp"=="sout"
                move            A,sout:(R_O)+N_O
                add     Y0,B    sinp:(R_I1)+N_I1,A
                move            B,sout:(R_O)+N_O   
                add     Y0,A    sinp:(R_I1)+N_I1,B
            else
                if "sinp"=='x'
                    add     Y0,B    sinp:(R_I1)+N_I1,A  A,sout:(R_O)+N_O
                    add     Y0,A    sinp:(R_I1)+N_I1,B  B,sout:(R_O)+N_O   
                else
                    add     Y0,B    A,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,A      
                    add     Y0,A    B,sout:(R_O)+N_O    sinp:(R_I1)+N_I1,B
                endif
            endif
pf\_vpsi_\ic\_l3                          ; jump to here for cnt=0 or cnt=1
        move            Y1,B
        tst     B    
        jeq     pf\_vpsi_\ic\_l2          ; if cnt odd (including cnt=1),
        move            A,sout:(R_O)+N_O    ;   store final element
pf\_vpsi_\ic\_l2
        endm

