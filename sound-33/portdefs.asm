;
;portdefs.asm - 56k equates
;
;	Modification History:
;	04/16/90/mtm		Added SSI_TDF, SSI_SCKD, SSI_SCD2, SSI_TIE.
;	06/11/90/mtm		Added host command vectors.
;

;
;External memory limits
;

XRAMLO		equ	8192		;Low address of external ram
XRAMSIZE	equ	8192		;External ram size
XRAMHI		equ	XRAMLO+XRAMSIZE	;High address + 1 of external ram

;
;Port/Bus/Interrupt Control Registers
;
			;Port B
PBC	equ $FFE0	;  Control Register
PBDDR	equ $FFE2	;  Data Direction Register
PBD	equ $FFE4	;  Data Register

			;Port C
PCC	equ $FFE1	;  Control Register
PCDDR	equ $FFE3	;  Data Direction Register
PCD	equ $FFE5	;  Data Register

BCR	equ $FFFE	;Bus Control Register
IPR	equ $FFFF	;Interrupt Priority Register

;
;Host Interface 
;
HCR	equ $FFE8	;Host Control Register
HRIE	equ 0		;  Host Receive Interrupt Enable
HTIE	equ 1		;  Host Transmit Interrupt Enable
HCIE	equ 2		;  Host Command Interrupt Enable
HF2	equ 3		;  Host Flag 2 (dsp -> host)
HF3	equ 4		;  Host Flag 3 (dsp -> host)

HSR	equ $FFE9	;Host Status Register
HRDF	equ 0		;  Host Receive Data Full
HTDE	equ 1		;  Host Transmit Data Empty
HCP	equ 2		;  Host Command Pending
HF0	equ 3		;  Host Flag 0 (host -> dsp)
HF1	equ 4		;  Host Flag 1 (host -> dsp)
DMA	equ 7		;  DMA Status

HTX	equ $FFEB	;Host Transmit Register (dsp -> host)
HRX	equ $FFEB	;Host Receive Register (host -> dsp)

;
;SSI
;
SSI_CRA	equ $FFEC
SSI_CRB	equ $FFED
SSI_SCD2 equ 4
SSI_SCKD equ 5
SSI_TIE equ 14
SSI_SR	equ $FFEE
SSI_IF1	equ 1
SSI_TDF	equ 6
SSI_RDF	equ 7
SSI_TX	equ $FFEF
SSI_RX	equ $FFEF

;
;SCI
;
SCR	equ $FFF0
SSR	equ $FFF1
SCCR	equ $FFF2
STX	equ $FFF4
SRX	equ $FFF4

;
;Interrupt Vector locations
;
VEC_RESET	equ	$0000		;reset vector
VEC_STKERR	equ	$0002
VEC_TRACE	equ	$0004
VEC_SWI		equ	$0006
VEC_IRQA	equ	$0008
VEC_IRQB	equ	$000A
VEC_SSI_RDAT	equ	$000C
VEC_SSI_REXC	equ	$000E
VEC_SSI_TDAT	equ	$0010
VEC_SSI_TEXC	equ	$0012
VEC_SCI_RDAT	equ	$0014
VEC_SCI_REXC	equ	$0016
VEC_SCI_TDAT	equ	$0018
VEC_SCI_IDLE	equ	$001A
VEC_SCI_TIMER	equ	$001C
VEC_HW		equ	$001E
VEC_HRX		equ	$0020
VEC_HTX		equ	$0022
VEC_HCMD	equ	$0024
VEC_R_DONE	equ	VEC_HCMD	;host command indicating dma read complete
VEC_W_DONE	equ	$0028		;host command indicating dma write complete
VEC_SYS_CALL	equ	$002C		;host command indicating system-call int coming

;
VEC_END		equ	$0040		;code begins here
