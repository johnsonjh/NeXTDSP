; allocusr.asm - user memory allocation macros
;
;; Copyright 1989, NeXT Inc.
;; Author - J.O. Smith
;;
;; -------------------------------------------------------------------------
;; INITIALIZE ALLOCATION POINTERS 
;;     {p,x,y,l}{i,e}_lc      used by user-memory allocation macros 
;; new_{p,x,y,l}{i,e}{,b,cb}:
;;
;; -------------------------------------------------------------------------
;; GENERIC MEMORY BLOCK ALLOCATION
;;
;; NAME
;;   set_blk - Initialize block of x, y, or l memory
;
;; SYNOPSIS
;;   set_blk name,count,value
;;
;; ARGUMENTS
;;   name  = label to attach to first word of storage block
;;   count = number of words of storage to allocate
;;   value = initial value for each word of storage (0 is typical)
;;	     If value = NO_SET, no initial value is explicitly set.
;;
;;   The assembly location counter should already be set to point to the
;;   first word of the new storage block (e.g. using org x:$nnn).
;;
;;   The assembly constant SYMOBJ_P should be 1 when the allocated storage
;;   is to be globally available and 0 when the new data is to be private.
;;
;; CALLED BY
;;   new_xib, new_yib, new_lib, new_xeb, new_yeb, new_leb
;;
set_blk	  macro name,count,value
	  opt now	      ; turn off warnings (name truncated to 4 chars):
ubtst	  set 'abc'++"name"   ; pick off first char of nam (check for "_")
	  if SYMOBJ_P!=0&&!@SCP("name",'')&&ubtst!='abc_'
	       symobj name	   ; export symbol to .lod file symbol table
	       xdef name	   ; export to other sections also
	  endif			   ; symobj _<local-label> fails fatally
	  if "value"=='NO_SET'	   ; NO_SET indicates don't initialize storage
name	       ds count
	  else
name	       bsc count,value	   ; initialize
	  endif
	  opt w			   ; turn warnings back on
	  endm
;
; The following is needed for l memory for reasons discussed below
;
set_lblk  macro name,count,hival,loval
	  opt now	      ; turn off warnings (name truncated to 4 chars):
ubtst	  set 'abc'++"name"   ; pick off first char of nam (check for "_")
	  if SYMOBJ_P!=0&&!@SCP("name",'')&&ubtst!='abc_'
	       symobj name	   ; export symbol to .lod file symbol table
	       xdef name	   ; export to other sections also
	  endif			   ; symobj _<local-label> fails fatally
	  if "hival"=='NO_SET'	   ; NO_SET indicates don't initialize storage
	    if "loval"!='NO_SET'
		fail 'cannot initialize half of l memory and not the other'
	    endif
name	    ds count
	  else
	    opt w			   ; turn warnings back on
	
	   if 0
	    warn 'allocusr: COME GET THIS RIGHT!'
name	    bsc count,(hival<<24)|loval ; should really mask sign bit off loval
	   else
	    if	@int(hival)
hiset0	      set hival
hiset1	      set 0
	    else
hiset0	      set @cvi(@flr(hival))
hiset1	      set @frc(hival-hiset0)
	    endif
	    if	@int(loval)
loset	      set loval
	    else
loset	      set @frc(loval)
	    endif
loset	    set loset+hiset1
name	    bsc count,(hiset0<<24)+(loset&$FFFFFF)
	   endif


	  endif
	  opt w			   ; turn warnings back on
	  endm
;;
;; -------------------------------------------------------------------------
;; USER MEMORY ALLOCATION
;;
;; NAME
;;   beg_{x,y,l}{i,e}b - Begin internal or external x, y, or l memory block
;;
;; SYNOPSIS
;;   beg_xib	    ; Start block of internal x memory
;;   beg_yib	    ; Start block of internal y memory
;;   beg_lib	    ; Start block of internal l memory
;;   beg_xeb	    ; Start block of external x memory
;;   beg_yeb	    ; Start block of external y memory
;;   beg_leb	    ; Start block of external l memory
;;
;; ARGUMENTS
;;   None
;;
;; DESCRIPTION
;;   Begin allocation of desired memory block. The current assembly location
;;   counter is switched to point to the next available element of the desired
;;   memory area. 
;;
;;   For example, after a call to beg_xib, which starts
;;   allocation in internal (on-chip) x memory, "define constant" statements
;;   (using the assembler DC directive) load the specified values into
;;   successive elements of x internal memory, beginning with the first
;;   free element. The block is terminated (in this example) by end_xib.
;;
;; EXAMPLE CALL
;;   beg_xib	    ; Start block of internal x memory
;;r1	  dc 1	    ; first value
;;r2	  dc 2	    ; second value
;;	  ...
;;rn	  dc n	    ; nth value
;;   end_xib	    ; terminate block and check for overflow
;;   org p_i	    ; necessary since beg_xib could not save
;;
;; CALLED BY
;;   new_xib, new_yib, new_lib, new_xeb, new_yeb, new_leb
;;   Unit generators, Array-processor macros, user main macros
;;
;; SEE ALSO
;;   end_{x,y,l}{i,e}b - Terminate internal or external x, y, or l memory block
;;
;; BUGS
;;   The DSP56001 assembler allows saving the current assembly location
;;   counter space (@MSP) and value (@LCV), but not which location counter
;;   was in use (e.g. p, pl, ph). Therefore, beg_??b cannot save the counter
;;   so that end_??b can restore it.  Consequently, switching back to
;;   the assembly in progress prior to the beg_??b call must be done
;;   manually by the user.
;;
;; ****************
;;
;; NAME
;;   end_{x,y,l}{i,e}b - End internal or external x, y, or l memory block
;;
;; SYNOPSIS
;;   end_xib	    ; Terminate block of internal x memory
;;   end_yib	    ; Terminate block of internal y memory
;;   end_lib	    ; Terminate block of internal l memory
;;   end_xeb	    ; Terminate block of external x memory
;;   end_yeb	    ; Terminate block of external y memory
;;   end_leb	    ; Terminate block of external l memory
;;
;; ARGUMENTS
;;   None
;;
;; DESCRIPTION
;;   End allocation of desired memory block.
;;   Overflow of the memory block is checked, and an assembler
;;   variable which points to the next available element is updated.
;;
;; CALLED BY
;;   new_xib, new_yib, new_lib, new_xeb, new_yeb, new_leb
;;   Unit generators, Array-processor macros, user main macros
;;
;; SEE ALSO
;;   beg_{x,y,l}{i,e}b - Start internal or external x, y, or l memory block
;;
;; BUGS
;;   end_??b does not switch the assembly location counter back to that
;;   in use (e.g. p, pl, ph) prior to the beg_??b call.	 The user must
;;   do this manually.	(See beg_??b BUGS for an explanation.)
;;
;; ****************
;;
;; NAME
;;   new_{x,y,l}{i,e}{,b} - Allocate internal or external x, y, or l memory
;;
;; SYNOPSIS
;;   new_xi name,value		    ; Allocate one word of internal x memory
;;   new_yi name,value		    ; Allocate one word of internal y memory
;;   new_li name,hival,loval	    ; Allocate one word of internal l memory
;;   new_xe name,value		    ; Allocate one word of external x memory
;;   new_ye name,value		    ; Allocate one word of external y memory
;;   new_le name,hival,loval	    ; Allocate one word of external l memory
;;
;;   new_xib name,count,value	    ; Allocate block of internal x memory
;;   new_yib name,count,value	    ; Allocate block of internal y memory
;;   new_lib name,count,hival,loval ; Allocate block of internal l memory
;;   new_xeb name,count,value	    ; Allocate block of external x memory
;;   new_yeb name,count,value	    ; Allocate block of external y memory
;;   new_leb name,count,hival,loval ; Allocate block of external l memory
;;
;;   new_xibn name,ic,count,value	; Create label of the form name\ic
;;   new_yibn name,ic,count,value	; Create label of the form name\ic
;;   new_libn name,ic,count,hival,loval ; Create label of the form name\ic
;;   new_xebn name,ic,count,value	; Create label of the form name\ic
;;   new_yebn name,ic,count,value	; Create label of the form name\ic
;;   new_lebn name,ic,count,hival,loval ; Create label of the form name\ic
;;
;; ARGUMENTS
;;   name  = label to attach to first word of storage block
;;   ic	   = instance count (usually an integer, but arbitrary text ok)
;;   count = number of words of storage to allocate
;;   value = initial value for each word of storage (0 is typical)
;;	     If value = NO_SET, no initial value is explicitly set.
;;   hival = most-significant word of l memory storage
;;   loval = least-significant word of l memory storage
;;
;;   The assembly constant SYMOBJ_P should be 1 when the allocated storage
;;   is to be globally available and 0 when the new data is to be private.
;;
;; CALLED BY
;;   Unit generators, Array-processor macros, user main macros, 
;;
;; -------------------------------------------------------------------------
;; USER MEMORY ALLOCATION
;;
; USER DATA MEMORY INITIALIZATION (allocation done in /usr/lib/dsp/smsrc/memmap.asm)

     if !@REL()	    ; if not relative-mode assembly, set location counters
	  org p_i:PLI_USR      ; point to beginning of internal user p code
	  org x_i:XLI_USR      ; point to beginning of internal user x data
	  org y_i:YLI_USR      ; point to beginning of internal user y data
	  org l_i:LLI_USR      ; point to beginning of internal user l data
	  org p_e:PLE_USR      ; point to beginning of external user p code
	  org x_e:XLE_USR      ; point to beginning of external user x data
	  org y_e:YLE_USR      ; point to beginning of external user y data
  	  org l_e:LLE_USR      ; point to beginning of external user l data
	  if DBL_BUG
	       org l_ih:LLI_USR ; point to beginning of li data, high-order word
	       org l_il:LLI_USR ; point to beginning of li data, low-order word
	       if NLE_USR>0
	       org l_eh:LLE_USR ; point to beginning of le data, high-order word
	       org l_el:LLE_USR ; point to beginning of le data, low-order word
	       endif
	  endif
     endif

beg_b macro s,w		 ; alloc/init next block in space s, w=i for int, or e
     org s\_\w:
     remember 'beg_b needs to save prior LC (e.g. p_e)'
     if ONE_MEM&&("s"=='l')&&("w"=='e')
	  fail 'beg_b: attempt to allocate external l memory in ONE_MEM case'
     endif
     endm

end_b macro s,w	    ; end block in space S, W=I for int, E for external
	nolist
next_\s\w set *+o\s\w\_usr ; pointer to next free word, absolute
        if next_\s\w>(s\h\w\_usr+1)&&!@def(NO_\s\w\_ARG_CHECK)
	  fail "end_\s\w\b: user memory overflow"
        endif
        if pi_active
	  org p_i:	    ; switch-back to internal assembly
        else
	  org p_e:	    ; switch-back to external assembly
        endif
	list
      endm
;
; --- user memory allocation macros
;
; l memory is not included below because for so long the 56000 assembler
; was broken with respect to them, and special macros for this case appear
; after the general case.  As of version 2.0 of asm56000, the general
; case below should extend properly to l memory. The reason we can't just
; do this is that our work-around meant adding an argument for the low-order
; word.  That is, we pass the upper and lower words of l memory separately
; instead of passing a single 48-bit (which used to be impossible).  If I
; ever get the energy to change all unit-generator macros, all test programs,
; and all array processing macros which use l memory (search for 'new_l'
; everywhere in the universe), and remove the second argument, then we can
; consolidate to the single general case below.  Note that UG arguments will
; change, and dspwrap will probably change.
;
      dupc s,'pxy'  ; loop over memory spaces
      dupc w,'ie'   ; loop over internal and external memory
      symobj next_\\s\\w
next_\\s\\w set s\\l\\w\\_usr ; ptr to next free word, absolute

beg_\\s\\w\\b macro			; start s\\w block
      beg_b s,w				; switch to s\\w ram
      endm

end_\\s\\w\\b macro			; end s\\w block
      end_b s,w
      endm

new_\\s\\w\\b macro name,count,value	; allocate and init next s\\w block
; ------------------------------ Allocate Memory Block ------------------------
      nolist
      beg_\\s\\w\\b
      set_blk name,count,value
      end_\\s\\w\\b
      list
      endm

new_\\s\\w\\bn macro name,ic,count,initval   ; version allowing instance suffix
      new_\\s\\w\\b name\\?ic,count,initval
      endm

new_\\s\\w macro name,value ; allocate and initialize next s\\w location
      new_\\s\\w\\b name,1,value
      endm

new_\\s\\w\\cb macro name,count,value ; allocate constrained s\\w location
_temp set *
      dsm count		      ; allocate modulo storage (skipping and wasting)
_lost set (*-_temp)-count
      new_\\s\\w\\b name,lost,0		; allocate lost portion
      new_\\s\\w\\b name,count,value	; use standard allocation
      endm

      endm ; dupc s,'pxy'

      endm ; dupc w,'ie'

; --- l user allocation

      dupc w,'ie'

new_l\w\bn macro name,ic,count,hival,loval ; version allowing instance suffix
      new_l\w\b name\?ic,count,hival,loval
      endm

	  symobj next_l\w
next_l\w  set ll\w\_usr ; pointer to next free word, absolute

new_l\w\b macro nam,count,hival,loval 	; alloc and init next l block
	  nolist		   	; who wants to see this cruft?
	  beg_b l,w		   	; switch to {li,le} ram
	  if DBL_BUG
nam	    ds count
	    dup count
	      org l_\w\h:
	      dc hival
	      org l_\w\l:
	      dc loval
	    endm
	  else
	    set_lblk nam,count,hival,loval
 	  endif
	  end_b l,w
	  list
	  endm

new_l\w	  macro name,hival,loval ; allocate and init next internal l location
	  new_l\w\b name,1,hival,loval
	  endm

	  endm ; dupc
;;
;;
;; -------------------------------------------------------------------------
;; UNIT-GENERATOR MEMORY-ARGUMENT ALLOCATION
;;
;; A Unit-Generator's (UG) memory arguments are normally allocated one word at a
;; time by the UG macro itself in internal memory.  The name generated for these
;; memory arguments has the form:
;;
;; main_<OuterUG>_<ic>_<NextInnerUG>_<ic>_...._<InnerMostUG>_<ic>_<SymbolName>
;;
;; where <OuterUG> is the unit-generator macro invoked by the main dsp
;; program, <NextInnerUG> is invoked within <OuterUG>, and <InnerMostUG> 
;; is invoked at the innermost macro nesting level where the memory argument
;; is being allocated. The "instance count" <ic> is an integer chosen by each
;; UG caller to make this occurrence of the UG macro unique.
;;
;; The name is passed in two parts in order to allow (1) innermost UG to 
;; specify <SymbolName> instead of the UG's caller, (2) <SymbolName> to be last.
;; The assembler will not let you prepend a macro argument - only append. 
;; Therefore, if the allocating UG specifies <SymbolName>, it cannot be 
;; appended to the rest of the name (which is derived from UG macro arguments).
;;
;; The following example program illustrates the intended use of these macros:
;;
;; UgB	     macro from,to,ic		; Innermost unit generator
;;	     new_xarg from\to\ic,X2,-1	; Symbol name is main_UgA_1_UgB_1_X2
;;	    ...				; Unit-generator guts
;;	     endm
;;	     
;; UgA	     macro from,to,ic		; Outermost unit generator
;;	     new_xarg from\to\ic,X1,-1	; Symbol name is main_UgA_1_X1
;;	    ...				; Unit-generator guts
;;	     UgB from\to\ic,UgB_,1_	; Invoke nested UG macro
;;	    ...				; Unit-generator guts
;;	     endm
;;	     
;;	     org p:0			; main program
;;	     UgA main_,UgA_,1_		; UgA <pf>_,<ic>_,<callee-instance>_
;;	     end 0
;;
new_xarg  macro nam1,nam2,value		; allocate 1 argument, name in 2 pieces
	  nolist
	  new_xib nam1\nam2,1,value
	  list
	  endm

new_yarg  macro nam1,nam2,value		; allocate 1 argument, name in 2 pieces
	  nolist
	  new_yib nam1\nam2,1,value
	  list
	  endm

new_larg  macro nam1,nam2,hival,loval	; allocate 1 argument, name in 2 pieces
	  nolist
	  new_lib nam1\nam2,1,hival,loval
	  list
	  endm

new_del	  macro spc,bname,ename,size    ; allocate delay line.
	  new_\spc\eb bname,size,0
;* ename  equ spc:bname+size  ; FAILS! ename defined by EQU does not relocate
	  org spc\_e:
ename			      ; This apparently works, but I don't like it
	  endm



