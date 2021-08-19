; config_standalone -included by unit-generator and array-processing-macro
; 		     test programs.  Sets things up so that assembly
;		     includes system monitor, no degmon, and no reset code.
;		     This makes the assembly loadable into a running Bug56.
ASM_SYS	   set 1		; want monitor code
ASM_RESET  set 0		; degmon will be preloaded by Bug56
ASM_DEGMON set 0		; degmon will be preloaded by Bug56

; override normal runtime halt action by one convenient with Bug56
	define DEBUG_HALT 'SWI' ; SINGLE WORD (--->abort)
DEBUG_HALT_OVERRIDDEN set 1
