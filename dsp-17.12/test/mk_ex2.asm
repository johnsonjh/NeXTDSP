; mk_ex2.asm - Test program for unit generator macro oscgafi
;
; Generate a test sinusoid.  When the program halts in Bug56,
; select DSPeek from the Tools menu, and set the range to nsamps
; (below) and the origin to y:$3800 (which is symbol YB_DMA_W, 
; the first of the two DMA sound-out buffers).
; You should see a plot of the sinusoidal output waveform (1.1 cycles).
; Note that every other sample is zero because the output buffer
; is stereo, and we are only sending data to channel A.
;
; Note: When a DMA buffer is filled, it gets divided by 256 
; to right-justify the output data for the 16-bit mode DMA transfer.
; This scaling happens at the same time DMA requests go out.
; You know when a DMA request happens because Bug56
; posts an alert panel when the host port is written with the DSP
; message "$3A00, $50001" (which means set up a DMA for the NEXT buffer).
; If you look at the DMA buffer AFTER a DMA request has gone out  
; over the host port, it will be too small to see with DSPeek.  The size
; of a stereo DMA buffer is NB_DMA_W/2 (512 in version 1.0).  To view the
; output data in the DMA buffer, use no more than NB_DMA_W/4 (256 in 1.0)
; MONAURAL samples in order to see it before scaling by 1/256.

; Usage:
;	asm56000 -A -B -L -OS,SO -I/usr/lib/dsp/smsrc/ mk_ex2
;	open /NextDeveloper/Apps/Bug56.app
;	<File / Load & erase symbols> mk_ex2.lod
;	<run>
;
mk_ex2  ident 0,0		; version, revision (arbitrary)
	include 'config_standalone'
;*	define nolist 'list'	; get absolutely everything into listing file
	include 'music_macros'	; utility macros

nsamps	set 256			; number of samples to compute (at most NB_DMA_W/4!)
nsintab equ 256			; wavetable length (sine table in ROM)
srate	equ 44100.0		; samples per second
amp	equ 0.5			; carrier amplitude
inc	equ 1.1			; table increment: Freq = inc*srate/nsintab
frqscl	equ 1.0/256.0		; inc scaler, to allow increments up to 256
incscl	equ 256.0/@pow(2.,23.)	; inc scaler, to allow increments up to 256

	beg_orch 'mk_ex2'	; standard startup for orchestras

	new_xib xsig,I_NTICK,0		; allocate waveform vector
	new_xib xamp,I_NTICK,amp 	; allocate amplitude vector
	new_xib xfrq,I_NTICK,inc*frqscl ; allocate increment vector

	beg_orcl
		nop_sep 3	; nop's to help find boundary
;;		oscgafi pf,ic,sout,aout0,sina,aina0,sinf,ainf0,inc0,
;;			phs0h,phs0l,stab,atab0,mtab0
		oscgafi orch,1,x,xsig,x,xamp,x,xfrq,incscl,0,0,y,YLI_SIN,$FF
		nop_sep 3	   	; nop's to help find boundary
		outa orch,1,x,xsig 	; Output signal to DAC channel A
		nop_sep 3	   	; nop's to help find boundary
		break_on_sample nsamps	; stop after nsamps samples (misc.asm)
	end_orcl
finish	end_orch 'mk_ex2'


