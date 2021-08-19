;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/06/87/jos - initial file created
;;  04/08/87/jos - passed test/tscale.asm
;;  10/20/87/jos - changed macro arguments to new conventions.
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      scale (UG macro) - scale a signal vector by a scalar using mpyr
;;
;;  SYNOPSIS
;;      scale pf,ic,sout,aout0,sinp,ainp0,ginp0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (pf\_scale_\ic\_ globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      sinp      = input memory space ('x' or 'y')
;;      ainp0     = initial input address in memory sinp
;;      ginp0     = initial scale factor [-1.0 to 1.0-2^(-23)]
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input signal      ainp0
;;      y:(R_Y)+        address of output signal     aout0
;;      y:(R_Y)+        scale factor                 ginp0
;;
;;  DESCRIPTION
;;      The scale unit-generator simply copies one signal vector over to
;;      another, multiplying by a scale factor.  The output vector can 
;;      be the same as the input vector.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      scale (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;            (dspace)sinp,(input)ainp,ginp
;;
;;  MAXIMUM EXECUTION TIME
;;      78 DSP clock cycles for one "tick" which equals 16 audio samples
;;      (sinp==sout).
;;
;;  MINIMUM EXECUTION TIME
;;      48 DSP clock cycles for one "tick" (sinp!=sout).
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tscale'             ; begin orchestra main program
;;           new_yeb invec,I_NTICK,0  ; Allocate input vector
;;           new_yeb outvec,I_NTICK,0 ; Allocate output vector
;;           beg_orcl                 ; begin orchestra loop
;;                ...                 ; put something in input vector
;;                scale orch,1,y,outvec,y,invec,0.2 ; invocation
;;                ...                 ; do something interesting with output vector
;;           end_orcl                 ; end of orchestra loop
;;      end_orch 'tscale'             ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/scale.asm
;;
;;  ALU REGISTER USE
;;       Y1  = scale factor
;;       X0  = current input word if sinp==x, else unused
;;       Y0  = current input word if sinp==y, else unused
;;       A   = current output word 
;;
scale macro pf,ic,sout,aout0,sinp,ainp0,ginp0
        new_xarg pf\_scale_\ic\_,ainp,ainp0
        new_yarg pf\_scale_\ic\_,aout,aout0
        new_yarg pf\_scale_\ic\_,ginp,ginp0
        move x:(R_X)+,R_I1      ; input address to R_I1
        move y:(R_Y)+,R_O       ; output address to R_O
        move y:(R_Y)+,Y1        ; scale factor to Y1
        if "sinp"=="sout"       ; target and dest spaces the same
          move sinp:(R_I1)+,X1
          mpyr X1,Y1,A
          do #I_NTICK-1,pf\_scale_\ic\_tl
            move sinp:(R_I1)+,X1
            mpyr X1,Y1,A A,sout:(R_O)+
pf\_scale_\ic\_tl
        else ; sinp!=sout:
          if "sinp"=='x'
               define pf\_scale_\ic\_inwd 'X0'
          else
               define pf\_scale_\ic\_inwd 'Y0'
          endif
          move sinp:(R_I1)+,pf\_scale_\ic\_inwd
          mpyr pf\_scale_\ic\_inwd,Y1,A sinp:(R_I1)+,pf\_scale_\ic\_inwd
          do #I_NTICK-1,pf\_scale_\ic\_tl
   mpyr pf\_scale_\ic\_inwd,Y1,A sinp:(R_I1)+,pf\_scale_\ic\_inwd A,sout:(R_O)+
pf\_scale_\ic\_tl
      endif
      move A,sout:(R_O)+  ; flush out last sample in pipe
      endm


