;;  Copyright 1987 by NeXT Inc.
;;  Author - John Strawn
;;      
;;  Modification history
;;  --------------------
;;      08/11/87/jms - initial file created from DSPAPSRC/template
;;      02/17/88/jms - added OPTIONAL_NOP
;;      02/23/88/jms - cosmetic changes to code and documentation
;;      03/29/88/jms - fix bugs in CALLING DSP PROGRAM TEMPLATE
;;      
;;  ------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      vtsmv (AP macro) - vector times scalar minus vector
;;          - multiply vector times scalar, subtract another vector
;;      
;;  SYNOPSIS
;;      include 'ap_macros' ; load standard DSP macro package
;;      vtsmv pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0
;;      
;;  MACRO ARGUMENTS
;;      pf      =   global label prefix (any text unique to invoking macro)
;;      ic      =   instance count (such that pf\_vtsmv_\ic is globally unique)
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
;;      The vtsmv array-processor macro computes C[k]=A[k]*s - B[k].
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
;;              sinp_a:ainp_a[n*iinp_a] * sins:sinp - sinp_b:ainp_b[n*iinp_b];
;;      
;;  MEMORY USE  
;;      8 x-memory arguments
;;      31 program memory locations
;;          + 1 program memory location if sinp_a==sinp_b==sinp_c
;;      
;;  EXECUTION TIME 
;;      cnt*3+32 instruction cycles
;;      
;;  DSPWRAP ARGUMENT INFO
;;      vtsmv (prefix)pf,(instance)ic,
;;          (dspace)sinp_a,(input)ainp_a,iinp_a,
;;          (dspace)sinp_b,(input)ainp_b,iinp_b,
;;          (dspace)sins,ains,
;;          (dspace)sout,(output)aout,iout,cnt
;;      
;;  DSPWRAP C SYNTAX
;;      ierr    =   DSPAPvtsmv(aS1,iS1,aS2,iS2,as,aD,iD,n);   /* D[k]=S1[k]*s-S2[k]
;;          aS1 =   Source vector 1 base address
;;          iS1 =   S1 address increment
;;          aS2 =   Source vector 2 base address
;;          iS2 =   S2 address increment
;;          as  =   address of scalar
;;          aD  =   Destination vector base address
;;          iD  =   D address increment
;;          n   =   element count                         */
;;      
;;  CALLING DSP PROGRAM TEMPLATE
;;      include 'ap_macros' 
;;      beg_ap  'tvtsmv'
;;      symobj  ax_vec,bx_vec
;;      beg_xeb
;;          radix   16
;;ax_vec    dc 711fff,712fff,713fff,714fff,715fff,716fff
;;bx_vec    dc 721fff,722fff,723fff,724fff,725fff,726fff
;;          radix   A  
;;      end_xeb
;;      new_xeb xscal,1,$123000
;;      new_xeb out1_vec,6,$777
;;      vtsmv   ap,1,x,ax_vec,1,x,bx_vec,1,x,xscal,x,out1_vec,1,6
;;      end_ap  'tvtsmv'
;;      end
;;      
;;  SOURCE
;;      /usr/lib/dsp/apsrc/vtsmv.asm
;;      
;;  REGISTER USE  
;;      A,B     accumulator
;;      X0,Y0,Y1 input terms and scalar
;;      X0,Y0   -0.5 and input term---see long comment in code
;;      R_I1    running pointer to A vector terms
;;      N_I1    increment for A vector
;;      R_I2    running pointer to B vector terms
;;      N_I2    increment for B vector
;;      R_L     pointer to scalar
;;      N_L     temporary storage for R_L
;;      R_O     running pointer to C vector terms
;;      N_O     increment for C vector
;;      
vtsmv   macro pf,ic,sinp_a,ainp_a0,iinp_a0,sinp_b,ainp_b0,iinp_b0,sins,ains0,sout,aout0,iout0,cnt0

        ; argument definitions
        new_xarg    pf\_vtsmv_\ic\_,ainp_a,ainp_a0
        new_xarg    pf\_vtsmv_\ic\_,iinp_a,iinp_a0
        new_xarg    pf\_vtsmv_\ic\_,ainp_b,ainp_b0
        new_xarg    pf\_vtsmv_\ic\_,iinp_b,iinp_b0
        new_xarg    pf\_vtsmv_\ic\_,ains,ains0
        new_xarg    pf\_vtsmv_\ic\_,aout,aout0
        new_xarg    pf\_vtsmv_\ic\_,iout,iout0
        new_xarg    pf\_vtsmv_\ic\_,cnt,cnt0

        ; get inputs
        move            R_L,N_L         ;  squirrel away R_L until endm
        move            x:(R_X)+,R_I1   ; input vector A base address to R_I1
        move            x:(R_X)+,N_I1   ; input vector A address increment to N_I1
        move            x:(R_X)+,R_I2   ; input vector B base address to R_I2
        move            x:(R_X)+,N_I2   ; input vector B address increment to N_I2
        move            x:(R_X)+,R_L    ; input scalar address to R_L
        move            x:(R_X)+,R_O    ; output address to R_O
        move            x:(R_X)+,N_O    ; output increment to N_O
        move            x:(R_X)+,B      ; count to B

        ; set up loop and pipelining
        ;;  The difference in the two parts of this if...else block is 
        ;;  simply that X1 and Y1 are switched, which is necessary for
        ;;  x/y moves in the loop below.  This same if...else test will,
        ;;  not suprisingly, be encountered later.
        if "sinp_a"=='y'&&("sinp_b"=='x'||"sinp_b"!="sout")
            move            sins:(R_L),X1   ; load scalar into X1; 
            tst     B       sinp_a:(R_I1)+N_I1,Y1
        else
            move            sins:(R_L),Y1   ; load scalar into Y1; 
            tst     B       sinp_a:(R_I1)+N_I1,X1
        endif
        jeq  pf\_vtsmv_\ic\_l2              ; protect againt count=0
        ; continue to set up pipelining, prepare to test for cnt=1
        ;;  The difference in *this* if...else block is simply that
        ;;  X0 and Y0 are switched.
        if "sinp_a"=='x'&&"sinp_b"=='y'
            move    #>$800000,X0
            move    #>1,Y0
            sub     Y0,B    sinp_b:(R_I2)+N_I2,Y0   ; B gets cnt-1
        else
            move    #>$800000,Y0
            move    #>1,X0
            sub     X0,B    sinp_b:(R_I2)+N_I2,X0
        endif

        ; generate first output term
        mpy     X1,Y1,A
        macr    X0,Y0,A
        ;;  Those last two lines are tricky. The goal is to 
        ;;  generate V1*s-V2, with s and V1 in X1 and Y1. The 
        ;;  mpy of X1,Y1 should be clear.  Now consider the 
        ;;  case when V2 is in Y0, and $800000 is in X0. 
        ;;  Normally multiplying by $8...  would be equivalent 
        ;;  to a multiply by -0.5.  But here, the macr 
        ;;  multiplies V2 by what is in effect a -1 in X0.  I 
        ;;  say "in effect" because the 56000 chip 
        ;;  automatically does a multiply by 2 after every 
        ;;  multiply.  This "compensates" for the "fractional 
        ;;  multiply."  So the result of the macr, since 
        ;;  $800000 is in one of X0 or Y0, is to add  the 
        ;;  negative of the V2 term into the product V1*s, 
        ;;  which is already in  accumulator A.  QED.
        
        ; finish set up pipelining, test for b==1
        if "sinp_a"=='y'&&("sinp_b"=='x'||"sinp_b"!="sout")
            move            sinp_a:(R_I1)+N_I1,Y1
            else
            move            sinp_a:(R_I1)+N_I1,X1
        endif
        if "sinp_a"=='x'&&"sinp_b"=='y'
            tst     B       sinp_b:(R_I2)+N_I2,Y0
        else
            tst     B       sinp_b:(R_I2)+N_I2,X0
        endif
        jeq     pf\_vtsmv_\ic\_l1           ; protect against cnt==1

        ; inner loop 
        do      B,pf\_vtsmv_\ic\_l1         ; this loops cnt-1 times 
            if "sinp_a"=="sinp_b"
                if "sinp_a"=="sout"
                    mpy     X1,Y1,A A,sout:(R_O)+N_O
                    macr    X0,Y0,A sinp_b:(R_I2)+N_I2,X0
                    move            sinp_a:(R_I1)+N_I1,X1
                else
                    if "sinp_a"=='x'
                        mpy     X1,Y1,A sinp_a:(R_I1)+N_I1,X1   A,sout:(R_O)+N_O
                    else
                        mpy     X1,Y1,A A,sout:(R_O)+N_O        sinp_a:(R_I1)+N_I1,Y1
                    endif
                macr    X0,Y0,A sinp_b:(R_I2)+N_I2,X0
                endif
            else
                mpy     X1,Y1,A A,sout:(R_O)+N_O  
                if "sinp_a"=='x'
                    macr    X0,Y0,A sinp_a:(R_I1)+N_I1,X1   sinp_b:(R_I2)+N_I2,Y0
                else
                    macr    X0,Y0,A sinp_b:(R_I2)+N_I2,X0   sinp_a:(R_I1)+N_I1,Y1     
                endif
            endif
pf\_vtsmv_\ic\_l1                       ; (jump to here for cnt==1)
        move    A,sout:(R_O)+N_O         ; store last output term
pf\_vtsmv_\ic\_l2
        move    N_L,R_L             
        OPTIONAL_NOP    ; from defines.asm, to prevent the macro from ending
                        ; by loading R_L. If you remove the nop, or if
                        ; you redefine OPTIONAL_NOP to be just ' ', then the 
                        ; macro loaded next might begin with an op code which
                        ; uses R_L without waiting for pipelining to finish.
        endm

