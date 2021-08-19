;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      08/05/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - change neg B to tst B in initialization
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      02/25/88/jms - add OPTIONAL_NOP at end
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtvps (AP macro) - vector multiply plus scalar add
;;          - pointwise multiplication of two vectors plus scalar add
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtvps pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtvps_\ic is globally unique)
;;      sinp_a  =   input vector A memory space ('x' or 'y')
;;      ainp_a0 =   input vector A base address (address of A[1,1])
;;      iinp_a0 =   initial increment for input vector A
;;      sinp_b  =   input vector B memory space ('x' or 'y')
;;      ainp_b0 =   input vector B base address
;;      iinp_b0 =   initial increment for input vector B
;;      sins    =   scalar memory space ('x' or 'y')
;;      ains0   =   scalar address
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
;;      x:(R_X)+    scalar address                  ains0
;;      x:(R_X)+    output vector C base address    aout0
;;      x:(R_X)+    output vector C increment       iout0
;;      x:(R_X)+    element count                   cnt0
;;      
;;  DESCRIPTION
;;      The vtvps array-processor macro computes C[k]=A[k]*B[k]+s.
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  = x:(R_X)+;    
;;      iinp_a  = x:(R_X)+;    
;;      ainp_b  = x:(R_X)+;    
;;      iinp_a  = x:(R_X)+;    
;;      ains    = x:(R_X)+;
;;      aout    = x:(R_X)+;    
;;      iout    = x:(R_X)+;
;;      cnt     = x:(R_X)+;    
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] * sinp_b:ainp_b[n*iinp_b] + sins:sinp;
;;      
;;  MEMORY USE 
;;      8 x-memory arguments
;;      23 program memory locations 
;;      
;;  EXECUTION TIME
;;      cnt*3+22 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vtvps (prefix)pf,(instance)ic,(dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,(dspace)sins,ains,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtvps(aS1,iS1,aS2,iS2,as,aD,iD,n);    /* D[k]=S1[k]*S2[k]+s
;;          aS1 =   Source vector 1 base address
;;          iS1 =   S1 address increment
;;          aS2 =   Source vector 2 base address
;;          iS2 =   S2 address increment
;;          as  =   address of scalar
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count                           */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvtvps'
;;      symobj  ax_vec,by_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;          radix   A  
;;      end_xeb
;;      beg_yeb
;;          radix   16
;;by_vec    dc 741fff,742fff,743fff,744fff,745fff,746fff,747fff,748fff,749fff,7410ff,7411ff,7412ff
;;          radix   A
;;      end_yeb
;;      new_xeb out1_vec,7,$777
;;      new_xeb xscal,1,$123
;;      vtvps    ap,1,x,ax_vec,1,y,by_vec,1,x,xscal,x,out1_vec,1,6
;;      end_ap  'tvtvps'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtvps.asm
;;      
;;  REGISTER USE  
;;      A       accumulator; temp for testing for cnt==0
;;      X1      cnt
;;      X0,Y0   two input terms    
;;      Y1      scalar
;;      R_I1    running pointer to A or B  vector terms
;;      N_I1    increment for A or B vector
;;      R_I2    running pointer to B or A vector terms
;;      N_I2    increment for B or A vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      R_L     pointer to scalar
;;      N_L     temporary storage for R_L     
;;      
vtvps    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtvps_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtvps_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtvps_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtvps_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtvps_\ic\_,ains,ains0
        new_xarg    pf\_vtvps_\ic\_,aout,aout0
        new_xarg    pf\_vtvps_\ic\_,iout,iout0
        new_xarg    pf\_vtvps_\ic\_,cnt,cnt0

        ; get inputs
        move            R_L,N_L             ; squirrel away R_L until endm
;;      When sinp_a = x and sinp_b = y, then the last section of code
;;      in the if..else chains below explicitly specifies x: for the A
;;      vector and y: for the B vector.  The same code can be used without
;;      change if we put here a simple test so that when sinp_a=y and
;;      sinp_b=a, we flip the use of R_I1 and R_I2.  
        if ("sinp_a"!="sinp_b")&&("sinp_a"=='y')
        ; invert normal use of R_I1, R_I2
            move            x:(R_X)+,R_I2   ; input vector A base address to R_I2
            move            x:(R_X)+,N_I2   ; input vector A address increment to N_I2
            move            x:(R_X)+,R_I1   ; input vector B base address to R_I1
            move            x:(R_X)+,N_I1   ; input vector B address increment to N_I1
        else ; this is the normal use of R_I1, R_I2
            move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
            move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
            move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
            move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        endif     
        move            x:(R_X)+,R_L        ; input scalar address to R_L
        move            x:(R_X)+,R_O        ; output address to R_O
        move            x:(R_X)+,N_O        ; output increment to N_O
        move            x:(R_X)+,A          ; count

        ; set up loop and pipelining
        tst     A       A,X1                ; squirrel away count for do loop
        jeq     pf\_vtvps_\ic\_loop1         ; protect againt count=0
        move            sins:(R_L),Y1       ; initialize scalar

        if "sinp_a"=="sinp_b" 
            if "sinp_a"=="sout" ; a = b = c = x or y
                ; more pipeline initialization
                move            sinp_a:(R_I1)+N_I1,Y0
                tfr     Y1,A    sinp_b:(R_I2)+N_I2,X0
                macr    X0,Y0,A sinp_a:(R_I1)+N_I1,Y0
                ; inner loop
                do      X1,pf\_vtvps_\ic\_loop1
                    tfr     Y1,A    A,sout:(R_O)+N_O
                    move            sinp_b:(R_I2)+N_I2,X0
                    macr    X0,Y0,A sinp_a:(R_I1)+N_I1,Y0
            else
                if "sinp_a"=='x' ; a = b = x, c = y
                    ; more pipeline initialization
                    move            sinp_a:(R_I1)+N_I1,X0
                    tfr     Y1,A    sinp_b:(R_I2)+N_I2,Y0
                    macr    X0,Y0,A sinp_b:(R_I2)+N_I2,Y0
                    ; inner loop
                    do      X1,pf\_vtvps_\ic\_loop1
                        tfr     Y1,A    sinp_a:(R_I1)+N_I1,X0   A,sout:(R_O)+N_O
                        macr    X0,Y0,A sinp_b:(R_I2)+N_I2,Y0
                else ; a = b = y, c = x
                    ; more pipeline initialization
                    move            sinp_a:(R_I1)+N_I1,Y0
                    tfr     Y1,A    sinp_b:(R_I2)+N_I2,X0
                    macr    X0,Y0,A sinp_b:(R_I2)+N_I2,X0
                    ; inner loop
                    do      X1,pf\_vtvps_\ic\_loop1
                        tfr     Y1,A    A,sout:(R_O)+N_O        sinp_a:(R_I1)+N_I1,Y0 
                        macr    X0,Y0,A sinp_b:(R_I2)+N_I2,X0
               endif
            endif	
        else
            ; more pipeline initialization
            move            x:(R_I1)+N_I1,X0
            tfr     Y1,A    y:(R_I2)+N_I2,Y0
            macr    X0,Y0,A x:(R_I1)+N_I1,X0    y:(R_I2)+N_I2,Y0
            ; inner loop
            do      X1,pf\_vtvps_\ic\_loop1
                tfr     Y1,A    A,sout:(R_O)+N_O
                macr    X0,Y0,A x:(R_I1)+N_I1,X0    y:(R_I2)+N_I2,Y0
        endif
pf\_vtvps_\ic\_loop1
        move            N_L,R_L   
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.

        endm

