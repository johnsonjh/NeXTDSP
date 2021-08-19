;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      08/10/87/jms - initial file created from DSPAPSRC/template
;;      02/15/88/jms - remove "andi #$fe,ccr"; replace ror with lsr, both in initialization 
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtspv (AP macro) - vector scalar multiply-add
;;          - multiply vector times scalar, add another vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtspv pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtspv_\ic is globally unique)
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
;;      The vtspv array-processor macro computes C[k]=A[k]*s + B[k].
;;
;;  PSEUDO-C NOTATION
;;      ainp_a  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      ainp_b  =   x:(R_X)+;
;;      iinp_a  =   x:(R_X)+;
;;      ains    =   x:(R_X)+;
;;      aout    =   x:(R_X)+;
;;      iout    =   x:(R_X)+;
;;      cnt     =   x:(R_X)+;
;;
;;      for (n=0;n<cnt;n++) 
;;          sout:aout[n*iout] = 
;;              sinp_a:ainp_a[n*iinp_a] * sins:sinp + sinp_b:ainp_b[n*iinp_b];
;;      
;;  MEMORY USE 
;;      8     x-memory arguments
;;      35    program memory locations 
;;            +1 program memory location if "sinp_a"=='y'&&"sinp_b"=='x'&&"sout"=='x'
;;            +2 program memory locations if "sinp_b"!="sout"
;;            +2 program memory locations if sinp_a = sinp_b = sout = x or y
;;      
;;  EXECUTION TIME 
;;      cnt*3+33 instruction cycles
;;            +1 instruction cycle if "sinp_a"=='y'&&"sinp_b"=='x'&&"sout"=='x'
;;            +2 instruction cycles if "sinp_b"!="sout"
;;            +2 instruction cycles if cnt odd
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vtspv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,(dspace)sins,ains,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtspv(aS1,iS1,aS2,iS2,as,aD,iD,n);    /* D[k]=S1[k]*s+S2[k]
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
;;      beg_ap  'tvtspv'
;;      symobj  ax_vec,by_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff,717fff,718fff,719fff,7110ff,7111ff,7112ff
;;          radix   A  
;;      end_xeb
;;      beg_yeb
;;          radix   16
;;          by_vec  dc  741fff,742fff,743fff,744fff,745fff,746fff,747fff,748fff,749fff,7410ff,7411ff,7412ff
;;          radix   A
;;      end_yeb
;;      new_xeb out1_vec,7,$777
;;      new_xeb xscal,1,$123
;;      vtspv    ap,1,x,ax_vec,1,y,by_vec,1,x,xscal,x,out1_vec,1,6
;;      end_ap  'tvtspv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtspv.asm
;;      
;;  REGISTER USE  
;;      A,B     accumulator
;;      X1      cnt/2
;;      X0,Y0   input term
;;      Y1      scalar
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      R_L     pointer to scalar; optional running pointer to B vector terms; 
;;      N_L     optional increment for B vector
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      N_X     flag for cnt odd or even
;;      N_Y     temporary storage for R_L
;;      
vtspv    macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtspv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtspv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtspv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtspv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtspv_\ic\_,ains,ains0
        new_xarg    pf\_vtspv_\ic\_,aout,aout0
        new_xarg    pf\_vtspv_\ic\_,iout,iout0
        new_xarg    pf\_vtspv_\ic\_,cnt,cnt0

        ; get inputs 
        move            R_L,N_Y         ;  squirrel away R_L until endm
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,R_L    ; input scalar address to R_L
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        clr A           x:(R_X)+,B      ; count to B

        ; set up loop and pipelining
        lsr     B       sins:(R_L),Y1   ; B gets cnt/2, Y1 will have scalar throughout  
        jcc     pf\_vtspv_\ic\_l1
        move            #1,A1           ; A gets "cnt odd" flag
pf\_vtspv_\ic\_l1    
        move            B1,X1           ; copy cnt0/2 into X1 for loop
        move            A1,N_X          ; N_X gets "cnt even/odd" flag
        move            sinp_a:(R_I1)+N_I1,X0
        move            sinp_b:(R_I2)+N_I2,A
        if "sinp_a"=='y'&&"sinp_b"=='x'&&"sout"=='x'
            move            sinp_a:(R_I1),Y0    ; for one case, Y0 used in inner loop
        endif
        macr    X0,Y1,A sinp_a:(R_I1)+N_I1,X0
        tst     B                       ; guard against cnt=0 or cnt=1
        jeq     pf\_vtspv_\ic\_l2   
        move            sinp_b:(R_I2)+N_I2,B
        if "sinp_b"!="sout"
            move            R_I2,R_L
            move            N_I2,N_L
        endif

        ; inner loop 
        do      X1,pf\_vtspv_\ic\_l2
        if "sinp_b"=="sout"     
            if "sinp_a"=="sout"
                ; sinp_a = sinp_b = sout = x or y
                macr    X0,Y1,B A,sout:(R_O)+N_O       
                move            sinp_a:(R_I1)+N_I1,X0
                move            sinp_b:(R_I2)+N_I2,A      
                macr    X0,Y1,A B,sout:(R_O)+N_O
                move            sinp_a:(R_I1)+N_I1,X0
                move            sinp_b:(R_I2)+N_I2,B
            else
                if "sinp_a"=='x'
                    ; sinp_a = x, sinp_b = sout = y
                    move            A,sout:(R_O)+N_O
                    macr    X0,Y1,B sinp_a:(R_I1)+N_I1,X0   sinp_b:(R_I2)+N_I2,A
                    move            B,sout:(R_O)+N_O
                    macr    X0,Y1,A sinp_a:(R_I1)+N_I1,X0   sinp_b:(R_I2)+N_I2,B
                else
                    ; sinp_a = y, sinp_b = sout = x
                    move            A,sout:(R_O)+N_O
                    macr    Y0,Y1,B sinp_b:(R_I2)+N_I2,A     sinp_a:(R_I1)+N_I1,Y0
                    move            B,sout:(R_O)+N_O
                    macr    Y0,Y1,A sinp_b:(R_I2)+N_I2,B     sinp_a:(R_I1)+N_I1,Y0
                endif
            endif
        else
            if "sinp_b"=='x'
                ; sinp_a = x or y, sinp_b = x, sout = y
                macr    X0,Y1,B sinp_a:(R_I1)+N_I1,X0
                move            sinp_b:(R_L)+N_L,A  A,sout:(R_O)+N_O
                macr    X0,Y1,A sinp_a:(R_I1)+N_I1,X0
                move            sinp_b:(R_L)+N_L,B  B,sout:(R_O)+N_O
            else
                ; sinp_a = x or y, sinp_b = y, sout = x
                macr    X0,Y1,B sinp_a:(R_I1)+N_I1,X0        
                move            A,sout:(R_O)+N_O    sinp_b:(R_L)+N_L,A 
                macr    X0,Y1,A sinp_a:(R_I1)+N_I1,X0
                move            B,sout:(R_O)+N_O    sinp_b:(R_L)+N_L,B
            endif
        endif
pf\_vtspv_\ic\_l2                        ; jump to here for cnt=0 or cnt=1
        move            N_X,B
        tst     B    
        jeq     pf\_vtspv_\ic\_l3        ; if cnt odd (including cnt=1),
        move            A,sout:(R_O)+N_O    ; store final element
pf\_vtspv_\ic\_l3
        move            N_Y,R_L   
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

