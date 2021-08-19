;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      07/10/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - changed neg A to tst A in initialization
;;                   - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtv (AP macro) - vector multiply - pointwise multiplication of two vectors
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtv    pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtv_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      sout    =   output vector C memory space ('x' or 'y')
;;      aout0   =   output vector C base address
;;      iout0   =   initial increment for output vector C 
;;      cnt0    =   initial element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector A base address     ainp_a0
;;      x:(R_X)+    input vector A increment        iinp_a0
;;      x:(R_X)+    input vector B base address     ainp_b0
;;      x:(R_X)+    input vector B increment        iinp_b0
;;      x:(R_X)+    output vector C base address    aout0
;;      x:(R_X)+    output vector C increment       iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vtv array-processor macro computes C[k]=A[k]*B[k].
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;  
;;      iinp_a  =   x:(R_X)+;  
;;      ainp_b  =   x:(R_X)+;  
;;      iinp_a  =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] * sinp_b:ainp_b[n*iinp_b];
;;      
;;  MEMORY USE 
;;      7   x-memory arguments
;;      16  program memory locations 
;;            +1 program memory location if sinp_a==sinp_b
;;            +1 program memory location if sinp_b==sout
;;      
;;  EXECUTION TIME 
;;      sinp_a==sinp_b: cnt*6+17 instruction cycles
;;      sinp_a!=sinp_b: cnt*6+16 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vtv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtv(aS1,iS1,aS2,iS2,aD,iD,n);   /* D[k] = S1[k] * S2[k]
;;          aS1 =   Source vector 1 base address
;;          iS1 =   S1 address increment
;;          aS2 =   Source vector 2 base address
;;          iS2 =   S2 address increment
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count                       */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvtv'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;bx_vec    dc 721fff,722fff,723fff,724fff,725fff,726fff,727fff,728fff,729fff,7210ff,7211ff,7212ff
;;          radix   A  
;;      end_xeb
;;      new_xeb out1_vec,6,$777
;;      vtv    ap,1,x,ax_vec,1,x,bx_vec,1,x,out1_vec,1,6
;;      end_ap  'tvtv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtv.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp for testing for cnt==0
;;      X1      cnt
;;      X0,Y0   two input terms    
;;      R_I1    running pointer to B (!) vector terms
;;      N_I1    increment for B vector
;;      R_I2    running pointer to A vector terms
;;      N_I2    increment for A vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      
vtv    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtv_\ic\_,aout,aout0
        new_xarg    pf\_vtv_\ic\_,iout,iout0
        new_xarg    pf\_vtv_\ic\_,cnt,cnt0
     
        ; get inputs 
        ;;  the choice of R_I2 for *A* and R_I1 for *B* is dictated by
        ;;  address register requirements for xy move, below
        move            x:(R_X)+,R_I2   ; input vector A base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector A address increment to N_I2
        move            x:(R_X)+,R_I1   ; input vector B base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector B address increment to N_I1
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,A      ; count

        ; set up loop and pipelining
        tst      A      A,X1            ; squirrel away count for do loop
        jeq      pf\_vtv_\ic\_loop1    ; protect againt count=0

        if "sinp_a"=="sinp_b"
            move        sinp_b:(R_I1)+N_I1,X0    
            move        sinp_a:(R_I2)+N_I2,Y0
        else
            if "sinp_a"=='x'
                move    sinp_a:(R_I2)+N_I2,X0   sinp_b:(R_I1)+N_I1,Y0
            else
                move    sinp_b:(R_I1)+N_I1,X0   sinp_a:(R_I2)+N_I2,Y0    
            endif
        endif          

        ; inner loop 
        do      X1,pf\_vtv_\ic\_loop1
            if "sinp_b"=="sout" 
                mpyr    X0,Y0,A sinp_a:(R_I2)+N_I2,X0
                move            A,sout:(R_O)+N_O   
                move            sinp_b:(R_I1)+N_I1,Y0
            else
                if "sinp_b"=='x'
                    mpyr    X0,Y0,A sinp_a:(R_I2)+N_I2,Y0
                    move            sinp_b:(R_I1)+N_I1,X0   A,sout:(R_O)+N_O
                else
                    mpyr    X0,Y0,A sinp_a:(R_I2)+N_I2,X0
                    move            A,sout:(R_O)+N_O        sinp_b:(R_I1)+N_I1,Y0
                endif
            endif
pf\_vtv_\ic\_loop1
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; at the end of a loop. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; is illegal after the end of a loop.
        endm

