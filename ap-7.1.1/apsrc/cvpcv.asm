;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      11/12/87/jos - initial file created from DSPAPSRC/cvtcv.asm
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/23/88/jms - add test for cnt=0
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      06/12/89/mtm - fix typo in pseudoC code
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      cvpcv (AP macro) - complex vector plus complex vector
;;          - form a complex vector from the elementwise sum of two complex vectors
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      cvpcv   pf,ic,ainp10,iinp10,ainp20,iinp20,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_cvpcv_\ic is globally unique)
;;      sinp1   =   input vector 1 memory space ('x' or 'y')
;;      ainp10  =   input vector 1 base address
;;      iinp10  =   increment for input vector 1
;;      sinp2   =   input vector 2 memory space ('x' or 'y')
;;      ainp20  =   input vector 2 base address
;;      iinp20  =   increment for input vector 2
;;      sout    =   output vector memory space ('x' or 'y')
;;      aout0   =   output vector base address
;;      iout0   =   increment for output vector
;;      cnt0    =   element count
;;      
;;  DSP MEMORY ARGUMENTS
;;      Access      Description                     Initialization
;;      ------      -----------                     --------------
;;      x:(R_X)+    input vector 1 base address     ainp10
;;      x:(R_X)+    input vector 1 increment        iinp10
;;      x:(R_X)+    input vector 2 base address     ainp20
;;      x:(R_X)+    input vector 2 increment        iinp20
;;      x:(R_X)+    output vector base address      aout0
;;      x:(R_X)+    output vector increment         iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The cvpcv array-processor macro computes C[k]=A[k]+B[k], where each
;;      term is complex.
;;
;;  PSEUDO-C NOTATION
;;      ainp1   =   x:(R_X)+;  
;;      iinp1   =   x:(R_X)+;  
;;      ainp2   =   x:(R_X)+;  
;;      iinp1   =   x:(R_X)+;  
;;      aout    =   x:(R_X)+;  
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;  
;;      
;;      for (n=0;n<cnt;n++) {
;;          sout:aout[n*iout]   = sinp1:ainp1[n*iinp1]   + sinp2:ainp2[n*iinp2];
;;          sout:aout[n*iout+1] = sinp1:ainp1[n*iinp1+1] + sinp2:ainp2[n*iinp2+1];
;;      }
;;      
;;  MEMORY USE 
;;      7 x-memory arguments
;;      sinp1==sinp2:   25 program memory locations
;;      sinp1!=sinp2:   23 program memory locations 
;;      
;;  EXECUTION TIME 
;;      sinp1==sinp2:   cnt*6+21 instruction cycles
;;      sinp1!=sinp2:   cnt*6+20 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      cvpcv (prefix)pf,(instance)ic,
;;          (dspace)sinp1,(input)ainp1,iinp1,
;;          (dspace)sinp2,(input)ainp2,iinp2,
;;          (dspace)sout, (output)aout, iout, cnt
;;      
;;  DSPWRAP C SYNTAX 
;;      ierr    =   DSPAPcvpcv(aS1,iS1,aS2,iS2,aD,iD,n);  /* D[k] = S1[k]+S2[k]
;;          aS1 =   S1 source vector base address (S1 complex)
;;          iS1 =   S1 address increment (2 for successive elements)
;;          aS2 =   S2 source vector base address (S2 complex)
;;          iS2 =   S2 address increment (2 for successive elements)
;;          aD  =   D destination vector base address (D complex)
;;          iD  =   D address increment (2 for successive elements)
;;          n   =   element count (number of complex values) */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tcvpcv'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix 16
;;ax_vec    dc a11aa,a12aa,a21aa,a22aa,a31aa,a32aa,a41aa,a42aa,a51aa,a52aa,a61aa,a62aa
;;bx_vec    dc b11bb,b12bb,b21bb,b22bb,b31bb,b32bb,b41bb,b42bb,b51bb,b52bb,b61bb,b62bb
;;          radix A  
;;      end_xeb
;;      new_xeb out1_vec,7,$777
;;      cvpcv ap,1,x,ax_vec,2,x,bx_vec,2,x,out1_vec,2,3
;;      end_ap  'tcvpcv'
;;      end
;;
;;  SOURCE
;;      /usr/lib/dsp/apsrc/cvpcv.asm
;;      
;;  REGISTER USE  
;;      A       count; real part of vector 1
;;      B       imag part of vector 1
;;      X0 or Y0 real part of vector 2
;;      X1 or Y1 imag part of vector 2
;;      Y1      constant (=1)
;;      R_I1    running pointer to vector 1 terms
;;      N_I1    increment for vector 1
;;      R_I2    running pointer to vector 2 terms
;;      N_I2    increment for vector 2
;;      R_O     running pointer to output vector terms
;;      N_O     increment for output vector
;;      
cvpcv   macro pf,ic,sinp1,ainp10,iinp10,sinp2,ainp20,iinp20,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_cvpcv_\ic\_,ainp1,ainp10
        new_xarg    pf\_cvpcv_\ic\_,iinp1,iinp10
        new_xarg    pf\_cvpcv_\ic\_,ainp2,ainp20
        new_xarg    pf\_cvpcv_\ic\_,iinp2,iinp20
        new_xarg    pf\_cvpcv_\ic\_,aout,aout0
        new_xarg    pf\_cvpcv_\ic\_,iout,iout0
        new_xarg    pf\_cvpcv_\ic\_,cnt,cnt0

        ; get inputs
        move            #>1,Y1
        move            x:(R_X)+,R_I1   ; input vector 1 address
        move            x:(R_X)+,A      ; input vector 1 increment 
        sub     Y1,A    x:(R_X)+,R_I2   ; input vector 2 address to R_I2
        move            x:(R_X)+,B      ; input vector 2 increment
        sub     Y1,B    A,N_I1          ; decremented input vector 1 increment to N_I1
        move            x:(R_X)+,R_O    ; output vector address
        move            x:(R_X)+,A      ; output vector decrement
        sub     Y1,A    B,N_I2          ; decremented input vector 2 increment to N_I2
        move            A,N_O           ; decremented output vector increment to N_O
        move            x:(R_X)+,A      ; count

        ; set up loop and pipelining
        tst     A       (R_O)-N_O       ; position output pointer for first "store i"
        jeq     pf\_cvpcv_\ic\_l2       ; test A to protect against count=0
        move            sout:(R_O),B    ; prefetch item for first "store i"
     
        ; inner loop 
        do      A,pf\_cvpcv_\ic\_l1
            if "sinp1"=="sinp2"
                move            sinp1:(R_I1)+,A     ; fetch r1
                move            sinp2:(R_I2)+,X0    ; fetch r2
                add     X0,A    B,sout:(R_O)+N_O    ; store i
                move            sinp1:(R_I1)+N_I1,B ; fetch i1
                move            sinp2:(R_I2)+N_I2,X1 ; fetch i2
                add     X1,B    A,sout:(R_O)+       ; store r
            else
                if "sinp1"=='x'
                    move            sinp1:(R_I1)+,A         sinp2:(R_I2)+,Y0    
                    add     Y0,A    B,sout:(R_O)+N_O    
                    move            sinp1:(R_I1)+N_I1,B     sinp2:(R_I2)+N_I2,Y1
                    add     Y1,B    A,sout:(R_O)+       
                else
                    move            sinp2:(R_I2)+,X0        sinp1:(R_I1)+,A     
                    add     X0,A    B,sout:(R_O)+N_O    
                    move            sinp2:(R_I2)+N_I2,X1    sinp1:(R_I1)+N_I1,B 
                    add     X1,B    A,sout:(R_O)+       
                endif
            endif
pf\_cvpcv_\ic\_l1
        move    B,sout:(R_O)    ; store last i
pf\_cvpcv_\ic\_l2
        endm

