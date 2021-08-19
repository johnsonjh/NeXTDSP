; x system environment variables
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; This file is 'included' by /usr/lib/dsp/smsrc/allocsys.asm
;; The allocation is in /usr/lib/dsp/smsrc/memmap.asm (NXE_SYS)
;;
;; *** TO ADD AN XE MEMORY VALUE ***
;;
;;    (1) Add it to the xdef list below (IF it should be global outside of sys)
;;    (2) Add it's DC statement below that
;;    (3) Add init code in reset_boot.asm(reset_soft) if it matters.
;;    (4) Increase I_NXE_SYS in config.asm for BOTH AP AND MK CASES!
;;
;; Anything which is accessed by user code (such as beg_orcl code)
;; needs to be XDEF'd.  The XDEF causes it to appear in sys_<?>.asm also.

     xdef X_HMSRP,X_HMSWP,X_START

; *** DATA ***

X_START   	dc $40	 	; Current user start address
X_HMSRP	  dc YBTOP_HMS-1      ; Host Message Stack Read Pointer
X_HMSWP	  dc YBTOP_HMS-1      ; Host Message Stack Write Pointer

X_DSPMSG_X1		dc 0
X_DSPMSG_X0		dc 0
X_DSPMSG_B2		dc 0
X_DSPMSG_B1		dc 0
X_DSPMSG_B0		dc 0
X_DSPMSG_A1		dc 0
X_DSPMSG_R_O		dc 0
X_DSPMSG_M_O		dc 0

X_XHM_R_I1		dc 0	; used by hc_xhm handler

X_DMA_R_M		dc -1	; M register used by DMA reads
X_DMA_W_M		dc -1	; M register used by DMA writes

; *** SAVED REGISTERS (written/restored by host command interrupt handlers) ***
; For interrupt handlers at priorities other than that of host commands, 
; the name segment "SAVED" should be replaced by "FOO_SAVED" where FOO
; is the name of the different priority level. Thus, "X_SAVED..." is short
; for "X_HOST_SAVED...".
; 
X_SAVED_REGISTERS

X_SAVED_R_I1		dc 0      ; etc
X_SAVED_R_I2		dc 0
X_SAVED_R_O		dc 0
X_SAVED_N_I1		dc 0
X_SAVED_N_I2		dc 0
X_SAVED_N_O		dc 0
X_SAVED_M_I1		dc 0
X_SAVED_M_I2		dc 0
X_SAVED_M_O		dc 0
X_SAVED_X1		dc 0
X_SAVED_X0		dc 0
X_SAVED_Y1		dc 0
X_SAVED_Y0		dc 0

X_SAVED_A2		dc 0
X_SAVED_A1		dc 0
X_SAVED_A0		dc 0

X_SAVED_B2		dc 0
X_SAVED_B1		dc 0
X_SAVED_B0		dc 0

X_SAVED_HOST_RCV1	dc 0      ; Saved host_rcv interrupt vector, word 1
X_SAVED_HOST_RCV2	dc 0      ; Saved host_rcv interrupt vector, word 2
X_SAVED_HOST_XMT1	dc 0      ; Saved host_xmt interrupt vector, word 1
X_SAVED_HOST_XMT2	dc 0      ; Saved host_xmt interrupt vector, word 2

X_SAVED_R_HMS		dc 0      ; Saved HMS-buffer index register
X_SAVED_N_HMS		dc 0      ; Saved HMS-buffer increment register
X_SAVED_M_HMS		dc 0      ; Saved HMS-buffer modulo register

	xdef X_DMQRP,X_DMQWP
X_DMQRP	  dc YB_DMQ	      ; DSP Message Queue Read Pointer
X_DMQWP	  dc YB_DMQ	      ; DSP Message Queue Write Pointer

     xdef X_SCRATCH1,X_SCRATCH2
X_SCRATCH1		dc $0	; scratch memory for USER LEVEL ONLY
X_SCRATCH2		dc $0

X_ABORT_RUNSTAT		dc 0
X_ABORT_DMASTAT		dc 0
X_ABORT_X0		dc 0	; changed in hmlib.asm: host_r_done0
X_ABORT_A1		dc 0	; multiple changes
X_ABORT_SP		dc 0	; 
X_ABORT_SR		dc 0
X_ABORT_HCR		dc 0
X_ABORT_HSR		dc 0
X_ABORT_R_HMS		dc 0
X_ABORT_R_I1		dc 0
X_ABORT_R_IO		dc 0	; what use are these? (can't resume DMA)
X_ABORT_M_IO		dc 0	; they are clobbered in host_r_done0

	if !AP_MON		; AP_MON only needs those above. MK needs rest:

; more saved registers
X_SAVED_SR		dc 0      ; Saved status register
X_SAVED_R_I1_HMLIB	dc 0      ; Saved HMS arg ptr when jsr'ing out of hmlib

     xdef X_NCHANS,X_NCLIP,X_MIDI_MSG
     xdef X_DMA_WFP,X_DMA_WFN,X_DMA_WFB,X_DMA_WEB
     xdef X_TMQRP,X_TMQWP,X_SSIRP
     xdef X_SSIWP,X_DMA_RFB,X_SSI_RFP,X_DMA_REB,X_DMA_REN,X_DMA_REP,X_SCI_COUNT

X_NCHANS  	dc I_NCHANS	; No. of audio channels computed
X_NCLIP   	dc $0		; No. times limit bit set at end of orch loop
X_MIDI_MSG 	dc 0		; Current MIDI message
X_SCI_COUNT	dc 0		; timer interrupt count (cf. sci_timer handler)

X_TMQRP	  dc YB_TMQ	      ; Timed Message Queue Read Pointer (see end_orcl)
X_TMQWP	  dc YB_TMQ	      ; Timed Message Queue Write Pointer (see xhmta)
X_SSIRP	  dc YB_DMA_W	      ; SSI Read Pointer for write-data
X_SSIWP	  dc YB_DMA_R	      ; SSI Write Pointer for read-data 

X_SSI_SAVED_R_I1	dc 0      ; etc
X_SSI_SAVED_M_I1	dc 0      ; etc
X_SSI_SAVED_A0		dc 0      ; etc
	if WRITE_DATA_16_BITS
X_SSI_SAVED_A1		dc 0      ; etc
X_SSI_SAVED_A2		dc 0      ; etc
X_SSI_SAVED_X0		dc 0      ; etc
X_SSI_SAVED_X1		dc 0      ; etc
	endif
X_SSI_PHASE		dc 0      ; FIXME: *temporarily* used for 22KHz -> SSI

; For DSP-to-host DMA ("write-data"):
; The "filling buffer" and "emptying buffer" addresses exchange on every "fill"
X_DMA_WFB dc YB_DMA_W	 ; Current "write-filling" buffer start address
X_DMA_WFP dc YB_DMA_W	 ; Current "write-filling" pointer
X_DMA_WFN dc YB_DMA_W	 ; Next "write-filling" pointer
X_DMA_WEB dc YB_DMA_W2	 ; Current "write-emptying" buffer start address
;*X_SSI_WEP    dc YB_DMA_W2   ; SSI "write-emptying" pointer (host gets R_IO)

; For host-to-DSP DMA ("read-data"):
; The "filling buffer" and "emptying buffer" addresses exchange on every "empty"
X_DMA_RFB dc YB_DMA_R	 ; Current start address of "read-filling" buffer
X_SSI_RFP dc YB_DMA_R	 ; SSI "read-filling" pointer
		remember 'may want RFN pointer here too'
X_DMA_REB dc YB_DMA_R2	 ; Corresponding "read-emptying" buffer
X_DMA_REN dc YB_DMA_R2	 ; Next "read-emptying" pointer
X_DMA_REP dc YB_DMA_R2	 ; Current "read-emptying" pointer

; *** CHECKS ***

TEMP	  set @CVF(I_NDMA)/@CVF(I_NCHANS)
	  if TEMP!=@FLR(TEMP)
	       fail 'Number of channels must divide DMA buffer length'
	  endif

; Taken care of elsewhere:
; X_ZERO  dc 0		 ; Zero (actually h.o.w. of l:L_ZERO. See sys_li)

; Maybe some day:
; X_NDEC  dc I_NDEC	 ; Envelope decimation factor for digital audio


	endif ; !AP_MON





