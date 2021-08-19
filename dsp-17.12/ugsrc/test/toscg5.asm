; toscg5.asm - Test program for macro oscg
;
; Generate 512 samples of a bandlimited pulse at frequency Fs*7.99/256
; (increment = 7.99 samples, table size = 256 samples, phase = 0 degrees).
; To verify, start up matlab (alias = ml), and say "dsptest; t; tspec(512);"
;
; 05/06/87/jos - passed. 40dB from 4 cosine peaks to distortion products
; 10/17/87/jos - changed signal vector to x space instead of y.
; 10/17/87/jos - passed xa.
;
toscg5    ident 0,9                ; version, revision
SIMULATOR set 1			   ; No target hardware, pure simulation
XY_SPLIT  set 1			   ; External memory is half x, half y
          include 'music_macros'      ; utility macros
nticks    set 512/I_NTICK          ; number of ticks to compute (512 samples)
mtab      set 255                  ; number of samples in wavetable - 1
ncos      set 4                    ; number of cosines
          beg_orch 'toscg5'        ; standard startup for orchestras
          new_xib xsig,I_NTICK,0   ; allocate waveform vector
ntab      set mtab+1
          beg_xeb                  ; allocate sum_of_cosines table
atab           cosine_sum ntab,ncos
          end_xeb
          beg_orcl                 ; note: 1st sample gets clobbered by out
               nop_sep 3           ; nop's to help find boundary
               brk_orcl nticks     ; stop after nticks ticks
               nop_sep 3           ; nop's to help find boundary
;;             oscg orch,1,sout,out,A0,F0H,F0L,P,0,y,atab0,mtab0
               oscg orch,1,x,xsig,0.5,7,0.99,0,0,x,atab,mtab
               nop_sep 3           ; nop's to help find boundary
               outa orch,1,x,xsig  ; Output signal to DAC channel A
               nop_sep 3           ; nop's to help find boundary
          end_orcl
finish    end_orch 'toscg5'



