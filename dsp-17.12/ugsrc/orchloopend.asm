;; Copyright 1989 by NeXT Inc.
;; Author - Julius Smith
;;
;; Modification history
;; --------------------
;; 04/12/88/jos - initial file created from /usr/lib/dsp/ugsrc/template
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      orchloopend (UG macro) - end DSP orchestra loop
;;
;;  SYNOPSIS
;;      orchloopend pf,ic
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (pf\_orchloopend\ic\_ globally unique)
;;
;;  DSP MEMORY ARGUMENTS
;;      None
;;
;;  DESCRIPTION
;;      The orchloopend unit-generator contains the final segment of the 
;;      DSP orchestra program, including the endning of the orchestra
;;      loop.   It simply jumps back to the top of the orchestra loop.
;;
;;      If doing static orchestra assembly (as opposed to module assembly
;;      for the Music kit), and if debugging is enabled (by placing the
;;      line "UG_DEBUG set 1" in the assembly source before including 
;;      music_macros), each memory-argument pointer
;;      is checked every tick to make sure it is in the right place.
;;  
;;      These functions are inherited from the end_orcl macro 
;;      in /usr/lib/dsp/smsrc/beginend.asm (used also for unit-generator development
;;      in a stand-alone-assembly mode.
;;  
;;  DSPWRAP ARGUMENT INFO
;;      orchloopend (prefix)pf,(instance)ic
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/orchloopend.asm
;;

orchloopend macro pf,ic

        end_orcl                ; end orchestra loop (beginend.asm)

      endm


