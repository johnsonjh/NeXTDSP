;;  Copyright 1989 by NeXT Inc.
;;  Author - Julius Smith
;;
;;  Modification history
;;  --------------------
;;  04/06/87/jos - initial file created
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      patch (UG macro) - patch one signal vector to another
;;
;;  SYNOPSIS
;;      patch pf,ic,sout,aout0,ispc,iadr0
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (s.t. pf\_patch_\ic\_ is globally unique)
;;      sout      = output memory space ('x' or 'y')
;;      aout0     = initial output address in memory sout
;;      ispc      = input memory space ('x' or 'y')
;;      iadr0     = initial input address in memory ispc
;;
;;  DSP MEMORY ARGUMENTS
;;      Arg access      Argument use                 Initialization
;;      ----------      --------------               --------------
;;      x:(R_X)+        address of input signal      iadr0
;;      y:(R_Y)+        address of output signal     aout0
;;
;;  DESCRIPTION
;;      The patch unit-generator simply copies one signal vector over to
;;      another.
;;         
;;  DSPWRAP ARGUMENT INFO
;;      patch (prefix)pf,(instance)ic,(dspace)sout,(output)aout,
;;            (dspace)ispc,(input)iadr
;;
;;  MINIMUM EXECUTION TIME
;;      if ispc==sout, (5+2*I_NTICK)/I_NTICK instruction cycles per sample
;;             == 2.625 instructions/sample (262.5ns) for I_NTICK==8 
;;      if ispc!=sout, (6+I_NTICK)/I_NTICK instruction cycles per sample
;;             == 1.75 instructions/sample (175ns) for I_NTICK==8 
;;
;;  CALLING PROGRAM TEMPLATE
;;      include 'music_macros'        ; utility macros
;;      beg_orch 'tpatch'             ; begin orchestra main program
;;           new_yeb invec,I_NTICK,0  ; Allocate input vector
;;           new_yeb outvec,I_NTICK,0 ; Allocate output vector
;;           beg_orcl                 ; begin orchestra loop
;;                ...                 ; put something in input vector
;;                patch orch,1,y,outvec,y,invec ; invocation
;;                ...                 ; do something interesting with output vector
;;           end_orcl                 ; end of orchestra loop
;;      end_orch 'tpatch'             ; end of orchestra main program
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/patch.asm
;;
;;  ALU REGISTER USE
;;       A = current word in transfer
;;
          beg_def 'patch'          ; standard macro definition-file begin
patch     macro pf,ic,sout,aout0,ispc,iadr0
          beg_mac 'patch'          ; standard macro begin
               new_xarg pf\_patch_\ic\_,iadr,iadr0
               new_yarg pf\_patch_\ic\_,aout,aout0
               move x:(R_X)+,R_I1  ; input address to R_I1
               move y:(R_Y)+,R_O   ; output address to R_O
               if "ispc"=="sout"   ; can't overlap read/write
                    do #I_NTICK,_temp
                         move ispc:(R_I1)+,A
                         move A,sout:(R_O)+
_temp    
               else                ; can overlap read/write
                    move ispc:(R_I1)+,A
                    do #I_NTICK-1,_temp
                         move ispc:(R_I1)+,A A,sout:(R_O)+
_temp               move A,sout:(R_O)+
               endif
          endm


