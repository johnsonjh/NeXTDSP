; *** NeXT NOTES ***
;
; Several things have been changed from the original (by JOS):
;
;	(1) The assembly of vectors here was moved to ~dsp/smsrc/vectors.asm
;
;	(2) All ORG statements were removed so that this is one 
;	    contiguous piece of assembly.  It is positioned by
;	    ~dsp/smsrc/allocsys.asm which includes it.
;
;	(3) Name changes
;		The DE_ prefix was changed to DEGMON_
;		(I use "DE_" to prefix "DSP Error" codes)
;	 	The STACKERR label was changed to DEGMON_STACKERROR
;	 	The DEGMON entry label was changed to DEGMON_BEG
;
;	(4) Comments and code were reformatted to look better after (3,4) above
;
;	(5) Comments were suppressed from listing via changing ';' to ';;'
;
;	(6) Added HCR save/restore because HTIE must cleared and restored.
;	    Otherwise, "DSP messages" will go out while DEGMON is in control
;	    which will wreak havoc.
;
;	(7) "MOVEC SSH,SSL" was changed to "MOVEC SSH,P:DEGMON_FLAG2"

;===========================================================================
;============================= DEGMON . ASM ================================
;===========================================================================
;====DEGenerate ------- Extensible MONitor for the NeXT by ARIEL CORP.======
;=============== (but) =====================================================
;===========================================================================
;======Copyright 1988,89 ARIEL CORP=======ALL RIGHTS RESERVED===============
;===========================================================================
;===========================================================================
;
DEGMON	ident 0,9	;DEGenerate MONitor for DSP56001, NeXT version 1.0
;
;	PC VERSIONS
;	Ver 1.2 16 Feb 89
;	Ver 1.1 12 Dec 88


        include 'ioequ.asm'	;include standard I/O port equates file.
;
;
; This is a short and simple three-function monitor for the NeXT processor 
; card. The last two functions are callable from the NeXT by using Host
; commands.
;
; This version has been adapted for the NeXT. Note that the NeXT has no NMI
; input, so the NMI ISR is solely used as a host command. The RESET vector is 
; is used as a host command. HF2 and HF3 are BOTH set to indicate a breakpoint.
; These changes increase the monitor's size beyond the nominal 64 words.
;
;;The following is a list of the functions incorporated in the monitor:
;;
;;RESET HANDLER		Sets up interrupt modes, etc. Sends the starting 
;;			address and the length of DEGMON to the NeXT via the 
;;			host port. Enters "FREEZE" function to complete the
;;			initialization and execute command-loop. 
;;
;;
;;FREEZE	    (callable)	Halt user program execution
;;			The PC and SR of the interrupted process are saved as
;;			well as HSR and HSX (BOTH levels), then the function 
;;			waits in an endless loop. 
;;			
;;			While in this loop, any two word instruction seq-
;;			uence may be uploaded to 56001 program RAM and
;;			executed. This is done by setting HF1 TRUE or FALSE
;;			and then writing the two words from the NeXT to the 
;;			host port. Note that both words must be written,
;;			even if the second is a NOP.
;;
;;			If HF1 is TRUE after the second word was rcvd, then
;;			DEGMON waits for data to be present in the HRX and
;;			then executes the code. If HF1 is FALSE then
;;			DEGMON waits for the HTX to be empty and then
;;			executes the code. Note that this latter mode
;;			should be used if the uploaded instrs will not be
;;			using the host-port data.
;;			
;;NOTE 1:
;;			The instruction(s) should be position independent, 
;;			such as MOVEP X0,X:<<$FFEB  (X0 --> HTX)
;;			These commands are designed to be used to transfer
;;			data from the host port to a register or memory
;;			location (HF1 SET) or from a register or memory 
;;			location to the host port (HF1 CLEAR). See the 
;;			execution profile below for more info.
;;
;;NOTE 2:
;;			The FREEZE handler is also the TRACE, NMI and SWI 
;;			handler. Therefore, there is no need to waste a 
;;			"normal" host command vector (from $12 up) on another
;;			vector to FREEZE. Since host commands may be used to
;;			activate any interrupt, BUG56 uses the NMI vector to
;;			call the FREEZE exception.			
;;			
;;RUN/TRACE  (callable)	Runs a program beginning at a program counter address
;;			that has been saved in program memory. There is also
;;			a copy of the status register in program RAM. These
;;			may be manipulated by the NeXT by using uploaded instr-
;;			uctions as described above. 
;;			If the the Trace mode bit is set in the copy of the 
;;			SR then only one instruction is executed.
;;
;;	For the NeXT, the Host Command has been changed to HC0;; i.e., the 
;;	RESET vector. This was done to work better with NeXT's existing code.

;;===========================================================================
;;
;;The monitor reserves the following resources for itself:
;;
;;The two callable DEGMON functions use the NMI and RESET vectors.
;;
;;The first eight host command vectors are uninitialized and free for your own 
;;use. This means that DEGMON begins at $0034. If you need to use the six 
;;remaining host vectors between $0034 and $003E just reassemble DEGMON 
;;starting at a higher address.
;;
;;Note that since the HOST IPL is set to 2, ALL host commands have the same 
;;IPL. This means that lower-priority (IPL1 or 0) exceptions are masked.
;;Since the HOST IPL also applied to HOST INTERRUPTS for the HOST port data in
;;and out registers, these interrupts should NOT be used when trying to run
;;a program under the BUG56/DEGMON system (may not work very well). 
;;
;;Hardware reset interrupt vector. This enters a short init routine in the
;;beginning of DEGMON, sends the length and location of DEGMON to the NeXT. 
;;It then waits in an endless loop where it handles the uploading of two-word
;;instruction pairs sent from the NeXT. When you are through debugging, be sure
;;to install a reset vector for your program if it is running independently of
;;the monitor!
;;
;;The TRACE, SWI, and "reserved for H/W development" or NMI (at $001E) vectors
;;are all initialized to point to the FREEZE handler.
;;
;;HF3 (host flag 3). This is used so that DEGMON can inform the BUG56 debugger 
;;(running on the NeXT) that it has reached a breakpoint or a return from a 
;;TRACEed instruction. 
;;Otherwise BUG56 would not know whether data being transmitted via the host 
;;port (from the DSP to the NeXT) is something being done by the running 
;;program or an indication that the breakpoint was reached. If your program 
;;SETS this flag bit and then sends data to the NeXT via the host port then
;;incorrect operation of BUG56 will occur.
;;
;;This has been changed for the NeXT: now HF2 and HF3 are BOTH asserted.
;;
;;HF1 (host flag 1). This is used to communicate to the FREEZE routine.
;;
;;HF0 and HF2 are reserved for file I/O for the program being debugged.
;;
;;The STACK ERROR interrupt sets the input port flag at DEGMON_FLAG (in pgm ram)
;;to $00FFFF as a flag that the stack had overflowed. The stack pointer itself
;;is saved at DEGMON_HPD for your inspection, and is then set to zero. The PC
;;and SR at the time of the stack error are lost. The handler then branches into
;;the FREEZE routine.
;;
;;DEGMON uses program memory starting after the some host command vectors
;;(PRAM $0032: DEGMON then starts at $0034). 
;;Note that you can change that by modifying and reassembling DEGMON 
;;Reassembly may also be needed to add additional host command vectors 
;;above the eight that space is allocated for. If host commands are not used 
;;but you are running out of internal program RAM (and you do not want to use
;;external program RAM in order to maximize data memory) then all the 
;;extra  host commands (RUN) may be removed (they  have NOP place holders),
;;and DEGMON may be reassembled.
;;
;;BUG56 does not support placing DEGMON in external program RAM. It is of
;;course not possible to load external program RAM during the 56001's native
;;boot cycle.
;;
;;No data memory is used, anywhere.
;;===========================================================================
;;If the running or single-stepped program does something that keeps
;;host commands from being recognized then BUG56 will assert the NMI signal.
;; (NOT TRUE FOR NeXT)
;;
;;This will ALWAYS work as long as the interrupt vector at $1E is not
;;changed (not true for NeXT, no NMI). If it is changed, BUG56 will time out 
;;and the user must reset the DSP and reload the monitor.
;;
;;The success of this approach requires you to obey a few rules:
;;
;;1) The STOP instruction should not be used. BUG56 will probably have to
;;   reset the DSP if STOP is used.
;;2) The RESET instruction resets the host port IPL. This is recoverable 
;;   by using NMI.
;;3) Avoid IPL 2 for any interrupts in your program. Use IPL 0 or IPL 1. 
;;   For most programs two interrupt levels are sufficient. If you need to
;;   use IPL2, BUG56 may not work correctly, depending on the context. 
;;4) DEGMON may not work correctly if the host NeXT sets DMA mode on in the ICR.
;;   This will not occur when using BUG56 but you should be aware of this 
;;   when developing your own programs.
;;5) Don't change the interupt vectors reserved for DEGMON or disable host 
;;   port interrupts via the IPR bits 10,11.
;;6) Don't change the OMR bits 0 & 1 unless you want to "reboot" the 56001. 
;;7) If you read from or write to the stack using uploaded instructions
;;   be careful of the sequencing to avoid disturbing the stack.
;;
;;	READ SSL from DSP
;;	READ SSH from DSP
;;	modify SSL & SSH within the NeXT.
;;	WRITE SSH to the DSP
;;	WRITE SSL to the DSP
;;
;;   is the correct way to do it. This is because writing to SSH changes the
;;   stack pointer.
;;8) Keep in mind that DEGMON uses one level of stack!
;;   
;;
;;
;;FREEZE:	
;;	DEGMON: Host command initiated. Enter TRACE/NMI/SWI exception
;;		handler, which sets HF3 and waits. 
;;
;;	NeXT:	Set the CVR to $1E (NMI vector) | $80 = $9E
;;		Read and save any data that might have been in RX.
;;		Wait for the CVR bit 8 to be cleared. 
;;		Wait for HF3 to be SET (allow time for DEGMON to reset it).
;;		This means that the program has been frozen. 
;;
;;		If the CVR bit 8 does NOT reset, clear it. Check again for
;;		HF3 being set in case the command activated between the last
;;		time it was checked and the clearing of CVR bit 8. If HF3 is
;;		still not SET, then assert the hardware NMI and check HF3. 
;;		If it still does not SET, the DSP must be reset.
;;		
;;		Once HF3 has been set, DSP56001 registers and memory may be
;;		be written to or read from by following this procedure:
;;
;;		1: SET HF1 for writing to regs or memory.
;;		   CLEAR HF1 for reading, or if no data transmission occurs.
;;
;;		2: Read and save any data that might have been in RX.
;;
;;		3: Write the first instruction word to the host port (TX).
;;
;;		4: Write the second word or a NOP to the host port (TX).
;;
;;		5: If HF1 was SET, write the data word (to place in memory 
;;		   or a Reg) to the host port (TX).
;;
;;		   If HF1 was CLEAR, and you have uploaded an instr that will
;;		   write to the host port, wait for bit 0 in the ISR (RXDF) 
;;		   to be set. Then read and save the memory or register data.
;;
;;
;;RUN/TRACE execution:
;;
;;	DEGMON:	Host command initiated. Data value (if any) in HRX is not
;;		saved in HPD. HF3 is reset.
;;		The trace interrupt bit is set in the unstacked SR by the
;;		HOST for tracing.
;;		
;;	NeXT:	Set the CVR to $0 (RESET vector) | $80 = $80
;;		Read and save any data that might have been in RX.
;;		Wait for the CVR bit 8 to be cleared. 
;;		Now the RUN or TRACE is active.
;;		Wait for HF3 to be SET (allow time for DEGMON to reset it).
;;		This means that the SWI or TRACE has been reached.
;;		Note that "FREEZE" or NMI can be used to halt the program 
;;		if trace mode was not used and no breakpoint has been set.
;;
;;Note that the NeXT should read from RXL LAST and write to TXL LAST. 
;;

	if 0
	  include 'degmon_vectors' ; we use vectors.asm instead
	endif

;===========================================================================
;start of DEGMON. May be changed to another (higher or lower) address to 
;allow more or fewer host commands, or to save program memory. Lowest address
;that is possible for the following ORG statement is P:$26.
;===========================================================================
;*	ORG	P:$0034
;*NeXT*	ORG	P:DEGMON_L	; done by including file allocsys.asm

DEGMON_BEG	; *** START OF DEGMON ***

;The DEGMON machine "registers" (maintained in P-RAM) are: 
;
DEGMON_HPD	EQU	DEGMON_BEG	;possible host-port data
DEGMON_FLAG	EQU	DEGMON_BEG+1	;host port flag byte
DEGMON_HPD2	EQU	DEGMON_BEG+2	;2nd level host-port data
DEGMON_FLAG2	EQU	DEGMON_BEG+3	;2nd level host-port flags
DEGMON_SR	EQU	DEGMON_BEG+4	;stacked SR when FREEZE/RUN used
DEGMON_PC	EQU	DEGMON_BEG+5	;stacked PC when FREEZE/RUN used
DEGMON_IPR	EQU	DEGMON_BEG+6	;saved IPR
DEGMON_HCR	EQU	DEGMON_BEG+7	;saved HCR (jos)
DEGMON_SR2	EQU	DEGMON_BEG+8	;SR that always has Trace bit cleared
;
;Access to these registers is primarily useful to BUG56. The actual memory 
;locations are the actual code for the (following) reset handler. Thus, a
;program should NOT try to re-execute the reset handler!
;
;RESET handler: note that processor IPL is 3 at this time and that the host-
;port has been enabled by the DSP56001's built-in boot program!
;
DEGMON_RESET   	MOVE	#<0,R0		;init R0 with 0. Note that using R0
					;here does not violate our prohibition
					;against using the 56001's registers
					;since the ROM-boot used R0 anyway.
;send start addr (bits 0..15) and length of DEGMON (bits 16..23) to host (NeXT)
		MOVEP	#DEGMON_BEG+((DEGMON_END-DEGMON_BEG)*65536),X:M_HTX

;now we can use value in R0 to init various registers.
		MOVEP	R0,X:M_BCR	;no wait states in any memory space
		MOVEP	R0,X:M_HCR	;clear the Host Control Register
;
;reset certain DEGMON "machine" registers (why do we need this? - jos)
; These first two are never used to restore state, so they can be ignored
; rather than cleared on reset (jos):
		MOVEM	R0,P:DEGMON_FLAG  ;clear host-port flag reg.
		MOVEM	R0,P:DEGMON_FLAG2 ;clear host-port flag reg, lvl 2.

; These ARE used to restore run state, so they must be cleared
; However, bug56 could do the clear remotely, the way it sets the PC & SR (jos)
		MOVEM	R0,P:DEGMON_IPR   ;clear IPR save reg.
		MOVEM	R0,P:DEGMON_HCR   ;clear HCR save reg. (jos)
		MOVEM	R0,P:DEGMON_SR2   ;clear special copy of SR

;note that the previous four instructions overwrite pgm memory pretty darn 
;near to their own locations! Be careful if you modify this!
;
;NeXT mod: change the RESET vector to point to the DEGMON_RUN command. Note
;that both words must be changed so that we have a JSR. We also have to use A1 
;since there's no way to write immediate data to P memory. Note that the boot
;loader uses this register, anyway.

		MOVE	#>$BF080,A1	;JSR (2-word type)
		MOVEM	A1,P:0		;write the opcode
		MOVE	#>DEGMON_RUN,A1	;get the dest address
		MOVEM	A1,P:1
		bset #3,x:$FFE3		; enable pc3 as output pin
		bclr #3,x:$FFE5		; clear pc3 to enable external RAM
;end of mod
		JMP	<DEGMON_RSTC	;enter main monitor loop
;===========================================================================
;|||||||||||||||||||||||| run a program: |||||||||||||||||||||||||||||||||||
;===========================================================================
;
DEGMON_RUN
		MOVEP	P:DEGMON_HCR,X:M_HCR ; restore HCR (jos)
		MOVEC	SSH,P:DEGMON_FLAG2 ;discard current Pgm Ctr (POP STACK)
		MOVEM	P:DEGMON_PC,SSH	;PUSH new PC on stack
		MOVEM	P:DEGMON_SR,SSL	;put new SR on stack
;we have to clear the IPR if we are tracing: this is co-ordinated by
;BUG56. This is made neccessary by something mentioned in Motorola's 
;Silicon Design Memo Feb 19, 1988, item #2.
		MOVEP	P:DEGMON_IPR,X:M_IPR ;fix the IPR (may be clearing it)

;special code to allow interrupts on while single-stepping: Note that IPL is 2
;at this point in time due to action of 'DEGMON_TRACER' section.
;
		MOVEM	P:DEGMON_SR2,SR	;install same SR as in DEGMON_SR but
					;w/o trace bit set (set up by BUG56)
		NOP			;allow time for an interrupt if SR's
					;I0 and I1 set appropriately.
;this **ONE** NOP is important. The timing is such that the interrupt happens
;during the execution of the RTI so if an interrupt is pending it is serviced
;and the sgl-step execution vectors to the ISR. BUG56 always sets DEGMON_SR2 to
;0 when it is not tracing (sgl stepping). Thus, running a program at full speed
;(which also uses this host command) is not affected.

		RTI			;run or trace. Note that the program
;counter and SR are pulled from the stack. Thus, the starting address
;must have been placed in DEGMON_PC by the NeXT. The SR must also be placed in
;DEGMON_SR before using RUN. To trace one instruction, set the TRACE bit in the
;copy of the SR (bit 13 of DEGMON_SR) 
;
;===========================================================================
;||||||||||||||||| STACK ERROR interrupt handler: ||||||||||||||||||||||||||
;===========================================================================
;
DEGMON_STACKERROR
		MOVE	R7,P:DEGMON_HPD	;save R7 so we can use it as tmp
		MOVE 	#$00FFFF,R7	;init R7 with $FFFF
		MOVE	R7,P:DEGMON_FLAG ;set flag to 00FFFF as error indicator
		MOVE	P:DEGMON_HPD,R7	;restore R7 from P memory
		MOVEM	SP,P:DEGMON_HPD	;save SP contents
		MOVEC	#0,SP		;reset SP to zero to clear error
		JMP	<DEGMON_STK		;and enter normal brkpoint code
;
;===========================================================================
;|||||||||| FREEZE / NMI / TRACE / BREAKPOINT interrupt handler: |||||||||||
;===========================================================================
DEGMON_TRACER 	
		MOVEP   X:<<M_HSR,P:DEGMON_FLAG  ;save HSR in P-RAM
		MOVEP	X:<<M_HRX,P:DEGMON_HPD	  ;save host port data (if any)
		MOVEP   X:<<M_HSR,P:DEGMON_FLAG2 ;save HSR (lvl 2) in P-RAM
		MOVEP	X:<<M_HRX,P:DEGMON_HPD2  ;save data (lvl 2) (if any)
		MOVEP	X:<<M_HCR,P:DEGMON_HCR   ;save HCR (jos)
;save the PC,SR, and IPR
		MOVEM 	SSL,P:DEGMON_SR	;save SR
		MOVEM	SSH,P:DEGMON_PC	;save PC (pop stack)
DEGMON_STK	MOVEP	X:M_IPR,P:DEGMON_IPR ;save the IPR value

;execute code to restore the monitor's program context (IPLs, etc)

DEGMON_RSTC	BSET 	#0,X:M_PBC	;activate host port just in case it 
					;had been disabled by other code.
		MOVEP	#$0C00,X:<<M_IPR ;set up host-port intr. priority and
					;disable all other interrupts.
;*		bset #3,x:$FFE3		; enable pc3 as output pin
;*		bclr #3,x:$FFE5		; clear pc3 to enable external RAM

; Set up desired HCR (jos)
;  Bit HCIE to enable host command INTs
;  (want HF2 and HF3 set to indicate we have entered this code after an SWI, 
;	traced instruction, reset sequence, etc.?)
		movep #$1C,x:M_HCR
;*		movep #$4,x:M_HCR
;
;Modify the Mode Register (MR) bits to IPL 2 (mask IPL0,1 INTs) by clearing 
;bit 0 and setting bit 1. Note that these bits are equivalent to bits 8 and 9
;of the Status Register (SR). The IPL is set to 3 after a reset (boot) cycle,
;or after an SWI (breakpoint), trace or stack error interrupt. However we do
;not explicitly know the IPL after an downloaded instruction is executed so we
;carefully set bit 1 then clear bit 0. 
		ORI	#2,MR		;be sure that bit 1 is set. 
		ANDI	#$FE,MR		;clear bit 0 of MR
;
;Wait here for a sequence of two words written by the host NeXT to the 56001 
;host port. Load the words into program memory. After the second word has 
;been placed in program memory, check HF1 for the direction of data transfer. 
;Examine the host port flags as appropriate for the direction, then execute
;the two instruction words.
;
DEGMON_WAIT	JCLR	#M_HRDF,X:<<M_HSR,DEGMON_WAIT
		MOVEP	X:<<M_HRX,P:DEGMON_I0  	;get first instr wd 
DEGMON_WD2	JCLR	#M_HRDF,X:<<M_HSR,DEGMON_WD2 ;wait for host port data
		MOVEP	X:<<M_HRX,P:DEGMON_I1  	;second instr:addr or NOP
;
		JSET	#M_HF1,X:<<M_HSR,DEGMON_WR ;goto wrt code if hf1 set.
;else the user is doing a read function:
;wait for NeXT to have clrd prev data (if any) from host port rcv regs.
DEGMON_RD	JCLR	#M_HTDE,X:<<M_HSR,DEGMON_RD
		JMP	<DEGMON_I0 ;exec instr which reads data from mem or reg
;write fcn: wait for data to be written.
DEGMON_WR	JCLR	#M_HRDF,X:<<M_HSR,DEGMON_WR ;wait for data at host port
;the function itself:
DEGMON_I0	NOP
DEGMON_I1	NOP
		JMP	<DEGMON_RSTC	;top of loop

DEGMON_END	EQU	*		;end of monitor+1
;===========================================================================
;|||||||||||||||||||||| END OF MONITOR |||||||||||||||||||||||||||||||||||||
;===========================================================================

;*	ORG	P:DEGMON_END	;your program begins here

