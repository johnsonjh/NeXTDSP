; beginend.asm - macros called at start/finish of main/macro/orchestra-loop
;;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; There are two main types of program supported on the dsp, each using the
;; same underlying chip-resident system code:
;;
;;   (1) General main programs for the dsp
;;   (2) Orchestra programs
;; 
;; In addition to standard beginning and ending macros for main programs
;; and orchestras, there are begin/end macros for macro definitions and
;; macro bodies.
;;
;; 01/29/90/jos - Changed HF0 meaning from "abort" to "freeze". Use hm_abort to abort.
;; 02/01/90/jos - Added nop at end of beg_orcl conditioned on FAST_AND_LOOSE.
;; 02/12/90/jos - Made nop at end of beg_orcl NOT conditioned on FAST_AND_LOOSE.
;;
;; ============================================================================
;; MAIN PROGRAM CAPS

; beg_main - Macro called at start of every main program.
beg_main  macro mname
	  section USER
	  org p:
	  endm

; end_main - Macro called at end of every main program
end_main macro mname
	if !NEXT_HARDWARE
		DEBUG_HALT
	else
		jsr hm_main_done
	endif
	if !FAST_AND_LOOSE
	  fill_unused_memory ; Set unused memory to a known state at asm time
	endif

	org p_i:	; back to onchip segment (if not there already)
	endsec		; USER (resume global section)
	end START
	endm

; ============================================================================
; AP MAIN PROGRAM CAPS

; beg_ap - Macro called at start of every ap prog.
beg_ap	  macro mname
	  beg_main mname
	  bset #4,x:$FFE8     ; set HF3 => AP busy
	  move #>XLI_USR,R_X  ; only X args initialized (beg_ionly is overkill)
	  nop		      ; give R_X time to settle
	  endm

end_ap	  macro mname	      ; Macro called at end of every main program
	  end_main mname      ; Nothing fancy here
	  endm ; end_ap

; =============================================================================
; ORCHESTRA MAIN PROGRAM CAPS

; beg_orch - Macro called at start of every orchestra prog.
beg_orch  macro mname
;
; 	The orchestra starts up in pause mode.  This means it is necessary
;	to set the tick increment l:L_TINC to I_NTICK in order for time
;	to advance.  It is also necessary to call start_ssi_write_data 
;	and/or start_host_write_data to start sound out i/o.
;
; -------------------------- Begin Orchestra ---------------------------------
;
	  beg_main mname      ; standard startup
;;	  maclib  '/usr/lib/dsp/ugsrc/' ; orchestra macros (music_macros.asm says this)
;;
;;	The following code used to be handled by reset_soft which is no longer
;;	called at the beginnning of every orchestra program.  Now, reset_soft
;;	is called as a part of the bootstrap in reset_boot.asm.
;;
	if ASM_SYS&&!ASM_RESET
; 	  We are in a stand-alone BUG56 load image
;	  Enable external RAM on older machines:
	  bclr #3,x:$FFE1 	; make pc3 a general purpose IO pin
	  bset #3,x:$FFE3 	;	 an output
	  bclr #3,x:$FFE5 	;	 containing zero.
	  movep #0,x:$FFFE	; clear BCR (0 ext mem wait states)
	  movec #>I_DEFOMR,OMR  ; refresh default OMR for ROMs (config.asm)
	endif

	  clr A	      	 	; Zero cumulative tick count
	  move A,l:L_TICK   	; Start at tick 0 (cf. /usr/lib/dsp/smsrc/sys_li.asm)
	  move #>I_NTICK,A0 	; Initialize tick increment
	  move A,l:L_TINC   	;
	  ; the following is taken from /usr/lib/dsp/smsrc/jsrlib.asm
	  move #>YB_DMA_W,X0  	; Write DMA buffer, 1st half
	  move X0,x:X_DMA_WFB 	; Start of initial "write-filling" buffer
	  move X0,x:X_DMA_WFP 	; Corresponding "write-filling" pointer
	  move X0,x:X_DMA_WFN 	; Corresponding "write-fill-next" pointer
	  move #X_DMA_WFP,X0
	  move X0,p:loc_x_dma_wfp ; hmdispatch.asm
	  move #YB_DMA_W2,X0 	; Write DMA buffer, 2nd half
	  move X0,x:X_DMA_WEB 	; Start of initial "write-emptying" buffer
	  endm		      	; beg_orch

end_orch  macro mname	      	; Macro called at end of every main program
	  end_main mname      	; Nothing fancy here
;
; ---------------------------- End Orchestra ---------------------------------
;
	  endm ; end_orch

; =============================================================================
; ORCHESTRA RUN-TIME INITIALIZATION CAPS

beg_ionly macro		      ; begin run-time initialization
;;    Called by the user in a main DSP program to set memory argument
;;    pointers to the current free locations.  This enables invocation
;;    of unit generators or array processor macros before the
;;    orchestra loop.  See also beg_orcl below.	
;;    "Ionly" means "initialization only".
;;
;;    /usr/lib/dsp/ugsrc/test/toscs.asm (when released)
;;    shows an example of its use to initialize a sinewave table at
;;    run-time using the vmov array processor macro. 

	move #next_xi,R_X ; Set up pointer to first x argument
	move #next_yi,R_Y ; Set up pointer to first y argument
	move #next_li,R_L ; Set up pointer to first l argument
	endm

end_ionly macro
	endm

; =============================================================================
; ORCHESTRA INNER LOOP CAPS

; beg_orcl - Invoked to begin orchestra tick computation loop
;
; -------------------------- Begin Orchestra Loop ----------------------------
;
beg_orcl  macro	    ; Invoked to begin orchestra tick computation loop
	  org p_e:  ; external p mem counter for orchestra tick loop
TICK_LOOP_E         ; entry point of orchestra loop in external memory
	  org p_i:  ; internal p mem counter for orchestra tick loop
pi_active set 1	    ; this goes to zero when we start assembly in external p:
TICK_LOOP	    ; entry point of onchip portion of orchestra loop
;
;    PROBLEM: switching to SYSTEM section changes p: LC value
;    to wherever it's left after sys_pi.asm is loaded (mem diagnostics).
;    Since beg_orch emits code in USER section, it's messy to
;    figure out what absolute org to use. Therefore, we emit orchestra
;    loop overhead code in USER section.
;
;*   	section SYSTEM
;
	if HAVE_SYSTEM

freeze_	jset #3,x:$FFE9,freeze_ ; freeze if requested via HF0 ("abort" in 1.0)
;	
;    check timed message queue
;
	jsset #B__TZM_PENDING,x:X_DMASTAT,loc_xhmta_return_for_tzm
	jclr #B__TMQ_ACTIVE,y:Y_RUNSTAT,bo_TMQ_empty
	jsr hm_service_tmq	; execute timed messages for current tick
bo_TMQ_empty
;
;;
;;    finish off sound-in tick
;;	  <test read-data bit>
;;	  <if read-data happening, see if it's time to switch buffers>
	  remember 'read-data check not yet written in end_orcl'

;;    finish off sound-out tick
;;	write-data buffers are always written. Hence no jsset or jsclr here.
	jsr hm_service_write_data

	else ; don't HAVE_SYSTEM => SIMULATOR => do everything in line
	   move x:X_DMA_WFN,X0	   ; Next dma write-fill pointer (out.asm)
	   move X0,x:X_DMA_WFP	   ; Make it current
	   move x:X_DMA_WEB,A	   ; Address of other half of ring buffer to A
	   cmp X0,A		   ; When time to switch, we get equality
	   jne simulator_no_switch ; below copped from write_data_switch:
	   ; do write_data_switch for SIMULATOR case
	   move x:X_DMA_WFB,X0	   ; current write-data fill buffer (full)
	   move x:X_DMA_WEB,Y0	   ; write-data empty buffer (better be done!)
	   move Y0,x:X_DMA_WFB	   ; new write-data fill buffer
	   move Y0,x:X_DMA_WFP	   ; new write-data fill pointer
	   move X0,x:X_DMA_WEB	   ; new write-data empty buffer
	   bchg #B__WRITE_RING,x:X_DMASTAT ; indicate parity of wd ring
simulator_no_switch
	   clr B x:X_DMA_WFP,R_O   ; current position in dma output buffer
	   move #(NB_DMA_W-1),M_O  ; write-data buffer is modulo ring size
	   rep #(2*I_NTICK)	   ; length of stereo tick
		move B,y:(R_O)+    ; clear it
	   move #-1,M_O            ; always assumed
	   move R_O,x:X_DMA_WFN    ; next write-fill pointer
	endif ; HAVE_SYSTEM

;*	endsec ; SYSTEM	   	   ; resume section USER

; *** SET MEMORY ARGUMENT POINTERS ***
;;
;;    NOTE: All memory argument allocation MUST happen inside orchestra loop! 
;;    Reason: Each UG does its own arg allocation.
;;
x_orca	  equ next_xi	   ; x arg block begins at current free cell
y_orca	  equ next_yi	   ; y arg block begins at current free cell
l_orca	  equ next_li	   ; l arg block BEGINS at current free cell

	  remember 'for backward l alloc, change l_orca equate'
	  ; (so that l arg block ENDS at current free cell)
;*l_orca  equ XHI_RAM	
;
;
;    update sample counter (current time)
;
	  move l:L_TICK,A     ; current tick to A
	  move l:L_TINC,B     ; tick size in samples to B
	  add B,A   #<0,X0    ; increment tick count, get a zero into X0
	  tes X0,A	      ; roll over to zero on overflow (extension set)
	  move A,l:L_TICK     ; save for next time around

	if HAVE_SYSTEM
	  unmask_host
	endif

;    reset arg pointers back to the beginning of the mem arg blocks
;    NOTE: This must be the last thing in the beg_orcl because the
;          Musickit counts on this fact to clobber these 3 instructions. (Daj)
;
	  move #>x_orca,R_X   	; Set up pointer to first x argument
	  move #>y_orca,R_Y   	; Set up pointer to first y argument
	  move #l_orca,R_L    	; Set up pointer to LAST l argument
	  nop			; In case 1st UG uses long mem arg in 1st instruction
	  
	  endm 	; beg_orcl

; ------------------------------ end_orcl ------------------------------------

; end_orcl - Macro invoked at end of orchestra loop.
end_orcl  macro 
;
	if !HAVE_SYSTEM
;
;	If there is a system, then code within write_data_buffer_out
;	will send the DMA write-data buffer to the simulator output 
;	file.  Otherwise, we write the current tick to sim output here.
;
             jclr #B__SIM,y:Y_RUNSTAT,dma_out_simulator ; skip if not simulator
	     move x:X_DMA_WFP,R_I1	; write-fill pointer of previous tick
	     do #2*I_NTICK,dma_out_simulator
		    move y:(R_I1)+,A	; Outgoing DMA buffer sample
		    move A,y:I_OUTY	; output to y:I_OUTY file
dma_out_simulator
	endif
;
; *** Verify memory argument pointers ***
;
x_orca_l  equ next_xi	   ; x arg block ends before current free cell
y_orca_l  equ next_yi	   ; y arg block ends before current free cell
l_orca_l  equ next_li	   ; l arg block ends before current free cell

	if !FAST_AND_LOOSE
	  jlc orcl_no_overflow
		move x:X_NCLIP,A
		move #>1,Y0
		add Y0,A	; increment total overflow count
		move A,x:X_NCLIP
		andi #$BF,ccr	; clear limit bit
orcl_no_overflow
	  move #>x_orca_l,X0 ; Where x arg ptr should be at end of orch loop
	  move R_X,A	     ; Where it actually is
	  cmp X0,A	     ; Please be right
	  jeq x_orca_ok
		move #DE_XMEMARG,X0
		jsr arg_err
x_orca_ok		
	  move #>y_orca_l,Y0 ; Where y arg ptr should be at end of orch loop
	  move R_Y,A	    ; Where it actually is
	  cmp Y0,A	    ; Please be right
	  jeq y_orca_ok
		move #DE_YMEMARG,X0
		jsr arg_err
y_orca_ok		
	  move #>l_orca_l,X0 ; Where l arg ptr should be at end of orch loop
	  move R_L,A	    ; Where it actually is
	  cmp X0,A	    ; Please be right
	  jeq l_orca_ok
		move #DE_LMEMARG,X0
		jsr arg_err
l_orca_ok		
	endif	; FAST_AND_LOOSE
;
;    measure size of upper segment 
	  org p_e:	      ; assume all external p_e code is in orch loop
	  remember 'If ext p code needed outside of orcl, use p_u or section'
nused_pe  set *-(PLE_USR-OPE_USR) ; number of words issued into upper segment
          if nused_pe>npe_usr
             fail 'external program memory overflow'
	  endif
	  if nused_pe<=0
	     org p_i:
	  endif
ORCL_END  jmp >TICK_LOOP	; Forever loop back for next "tick"
	  remember 'Consider forcing short-mode jump to TICK_LOOP'
	  ;; We force long mode at present to make relative = absolute.
	  ;; The default is short mode for absolute, long mode for relative.
; *** ORCHESTRA LOOP IS CLOSED ***
END_OLOOP  DEBUG_HALT	; Normally the orchestra loop is an infinite loop.
			; However, test facilities such as break_on_sample
			; (misc.asm) and break_on_tick jump to END_OLOOP
			; when some test orchestra program is finished.
	  jmp END_OLOOP ; in case DEBUG_HALT continues

arg_err	  ; We are here if unit-generator memory-arguments got fouled up.
	  if HAVE_SYSTEM
		; X0 has appropriate error code already from above.
	 	jsr hm_stderr ; we're in USER section => stderr not global
	  else
	  	DEBUG_HALT
	  endif
	  jmp arg_err	; in case of a continue

	  org p_i:	; go back and detect overflow of onchip p memory
phi_lost  set *-phi_usr
	  if phi_lost>0
		fail 'internal program memory overflow'
	  endif
;
; -------------------------- End Orchestra Loop ------------------------------
;
	  endm 	; end_orcl

; ============================================================================
; MACRO DEFINITION CAPS

beg_def	  macro mac	 ; beg_def '<mname>' at start of every macro def
;----------------------------- Macro Definition ------------------------------
	  endm ; beg_def

end_def	  macro mac	 ; end_def '<mname>' at end of every macro def
;--------------------------- End Macro Definition ----------------------------
	  endm ; end_def

; ============================================================================
; MACRO BODY CAPS

beg_mac	  macro mac	 ; Macro called at start of every macro invocation
;----------------------------- Macro Body Expansion --------------------------
	  nolist
	  org p:	 ; Switch to code phase
SYMOBJ_P  set 1		 ; requires long name support
	  list
;	  end of beg_mac
	  endm ; beg_mac

end_mac	  macro	    mac	      ; Macro called at end of every macro invocation
;----------------------------- End Macro Expansion ---------------------------
	  endm ; end_mac




