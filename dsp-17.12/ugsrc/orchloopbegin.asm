;; Copyright 1989 by NeXT Inc.
;; Author - Julius Smith
;;
;; Modification history
;; --------------------
;; 04/12/88/jos - initial file created from /usr/lib/dsp/ugsrc/template
;; 02/05/89/jos - removed jsr to reset_soft. Now MK must boot or HM_RESET_SOFT
;;
;;------------------------------ DOCUMENTATION ---------------------------
;;  NAME
;;      orchloopbegin (UG macro) - begin DSP orchestra loop
;;
;;  SYNOPSIS
;;      orchloopbegin pf,ic
;;
;;  MACRO ARGUMENTS
;;      pf        = global label prefix (any text unique to invoking macro)
;;      ic        = instance count (pf\_orchloopbegin\ic\_ globally unique)
;;
;;  DSP MEMORY ARGUMENTS
;;      None
;;
;;  DESCRIPTION
;;      The orchloopbegin unit-generator contains the intial segment of the 
;;      DSP orchestra program, including the beginning of the orchestra
;;      loop.  It includes the following system functions:
;;  
;;                1. increment the tick counter
;;                2. execute ready timed messages
;;                3. update read-data pointers and DMA
;;                4. update write-data pointers and DMA
;;  
;;      These functions are inherited from the beg_orcl macro 
;;      in /usr/lib/dsp/smsrc/beginend.asm (used also for unit-generator development
;;      in a stand-alone-assembly mode.
;;  
;;  DSPWRAP ARGUMENT INFO
;;      orchloopbegin (prefix)pf,(instance)ic
;;
;;  SOURCE
;;      /usr/lib/dsp/ugsrc/orchloopbegin.asm
;;
;;  SEE ALSO
;;      /usr/lib/dsp/smsrc/beginend.asm(beg_orcl) - invoked by orchloopbegin
;;
;;#define DSP_MAX_ONCHIP_PATCHPOINTS 8

orchloopbegin macro pf,ic

;!!!    jsr >reset_soft         ; initialize the world (same as beg_orch???)

        beg_orcl                ; start orchestra loop (beginend.asm)

      endm


