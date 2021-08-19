;
; compress.asm
;	Compress a sound using Richard Crandall's Parallel Packing
;	technique.  The sound starts with a small subheader.  Each block is
;	one packed channel and starts with an algorithm code.  "Residue" bits
;	are optionally appended to each sample for bit-faithful decompression.
;
;	This is basically a port of the C version in parlib.c.  See that code
;	for more comments.
;
;	Another asm source file must include file this after including i/o code.
;	The SOUND_HEADER equate should also be set up.
;	See hostcompress.asm and ssicompress.asm.
;
; Restrictions:
;	max i/o buf size	5K	(see performsound.c and dspsoundssi.asm)
;	max channelCount	2	from soundfile header
;	max encodeLength	256	from soundfile subheader
;	max numDropped		8	from soundfile subheader
;
;	Subheader method is 1 for bit-faithful, 0 for non-bit-faithful
;
; External Memory Usage:
;	x:$2000 (5120)	buffers used by dspsoundssi.asm
;	x:$3400 (1024)	sample buffer
;	x:$3800	(512)	packed buffer
;	x:$3a00	(512)	left channel residue
;	x:$3c00	(512)	right channel residue
;	p:$3e00		code
;
; Modification History:
;	04/23/90/mtm	Original version.
;	05/11/90/mtm	Added sampleSkip.
;	05/17/90/mtm	Tack residue bits on to each sample.
;	05/25/90/mtm	Don't read numSamples from host.
;	07/18/90/mtm	Change getHost calls to readHost.
;			Change getSSI calls to getSample.
;			Don't include dspsounddi.asm
;	07/26/90/mtm	Eat soundfile header if SOUND_HEADER is set.
;	07/30/90/mtm	Stop after numSamples (host case only).
;	08/06/90/mtm	Sign extend samples when they are read in (host case only).
;	08/12/90/mtm	Implement all algorithms, make maxEncodeLength 256.
;	08/14/90/mtm	Implement REAL_TIME mode.
;	10/01/90/mtm	Host version can read mono files (bug #7909).
;	10/04/90/mtm	Remove stray instruction in eor encode (bozo bug #10001).

;---------------------------------------------------------------
; Equates
;---------------------------------------------------------------

BIT_FAITHFUL		equ	1	; method is 0 for non-bit-faithful
MAX_BUFSIZE		equ	5*1024	; we need at least 3K
MAX_ENCODE_LENGTH	equ	256	; max mono samples in an encode buffer
MAX_BITS		equ	12	; (16 - MinNumDropped) must be >= MAX_BITS
NUM_ENCODES		equ	11


EXTERNAL_CODE	equ	XRAMLO+MAX_BUFSIZE+MAX_ENCODE_LENGTH*5
	
;---------------------------------------------------------------
; Internal l memory
;---------------------------------------------------------------
	org	l:0
curSample	dc	0
numSamples	ds	1
	
;---------------------------------------------------------------
; Internal x memory
;---------------------------------------------------------------
	org	x:2
	
; Compression routines
routines	dc	null,xor,d1,d2,d3,d4,d3_11,d3_22,d4_222,d4_343,d4_101

; Number of unencoded (leading) samples for each routine
leaderCount	dc	0,1,1,2,3,4,3,3,4,4,4

; Multiply factors to implement bit shifting
; See "Fractional and Integer Arithmetic...", Motorola APR3/D, p. 9-11
rightShifts	dc	0,@pow(2,-1),@pow(2,-2),@pow(2,-3),@pow(2,-4)
		dc	@pow(2,-5),@pow(2,-6),@pow(2,-7),@pow(2,-8)
		dc	@pow(2,-9),@pow(2,-10),@pow(2,-11),@pow(2,-12)
		dc	@pow(2,-13),@pow(2,-14),@pow(2,-15)
leftShifts	dc	0,@cvi(@pow(2,1-1)),@cvi(@pow(2,2-1)),@cvi(@pow(2,3-1))
		dc	@cvi(@pow(2,4-1)),@cvi(@pow(2,5-1)),@cvi(@pow(2,6-1))
		dc	@cvi(@pow(2,7-1)),@cvi(@pow(2,8-1)),@cvi(@pow(2,9-1))
		dc	@cvi(@pow(2,10-1)),@cvi(@pow(2,11-1)),@cvi(@pow(2,12-1))
		dc	@cvi(@pow(2,13-1)),@cvi(@pow(2,14-1)),@cvi(@pow(2,15-1))
		
; Variables read from host
headerSize	ds	1	; sound header size if samples from host
channelCount	ds	1	; 1 or 2
method		ds	1	; 1 for bit-faithful, 0 otherwise
numDropped	ds	1	; num bits to right shift off - 4 to 8
encodeLength	ds	1	; depends on numDropped, calculated on host
sampleSkip	ds	1	; 2 for 44K, 4 for 22K hack
dropMask	ds	1	; calculated from numDropped

; Variables set by score
algorithm	ds	1
numBits		ds	1
sign		ds	1

; Array to hold highest bits set by score
maxBits		ds	NUM_ENCODES

; Array to hold total score of each algorithm
scores		ds	NUM_ENCODES

;---------------------------------------------------------------
; External x memory
; External memory buffers start after the dma buffers used by dspsoundssi.asm
;---------------------------------------------------------------
	org	x:XRAMLO+MAX_BUFSIZE
	
; Buffer to hold stereo samples read from input source
sampleBuf	ds	MAX_ENCODE_LENGTH*2

; Buffer to hold packed data to go to host
packedBuf	ds	MAX_ENCODE_LENGTH

; Buffers to hold dropped bits (residue)
leftResidue	ds	MAX_ENCODE_LENGTH
rightResidue	ds	MAX_ENCODE_LENGTH
	
;---------------------------------------------------------------
; Macros
;---------------------------------------------------------------

;---------------------------------------------------------------
; Read an int (32 bits) from the host sent as 2 words
; Returns the low order short in a1
getInt16 macro
	jsr	getHost		; ignore high word
	jsr	getHost
	endm
	
;---------------------------------------------------------------
; Read an int (32 bits) from the host sent as 2 words
; Returns a 48 bit int in a
; Destroys y0, x0, x1
getInt48 macro
	jsr	getHost
	move	a1,y0
	jsr	getHost
	move	a1,x0
	move	#>@cvi(@pow(2,8-1)),x1	; left shift by 8 multiply factor
	mpy	x0,x1,a
	move	y0,a1
	rep	#8
	asr	a
	endm

;---------------------------------------------------------------
; Sign extend 16-bit quatity into 24 bits.
; Destroys b and x1
signx	macro
	move	#$8000,x1
	and	x1,a  a1,b1	 ; test sign bit
	move	b1,a1
	.if	<ne>
		move	#$ff0000,x1
		or	x1,a	 ; clear sign bit
	.endi
	endm

;---------------------------------------------------------------
; Jump to the next score algorithm if maxBit entry > MAX_BITS.
checkMax macro	nextAlgo
	move	x:(r1)+,a	; current maxBit
	cmp	y0,a		; already > max ?
	jgt	nextAlgo	; yes, try next algorithm
	endm

;---------------------------------------------------------------
; Jump to the next score algorithm if not enough samples left.
checkCnt macro	cnt,nextAlgo
	move	lc,a
	move	#>cnt,b		; need at least cnt+1 samples
	cmp	b,a
	jlt	nextAlgo
	endm

;---------------------------------------------------------------
; Code common to end of each score algorithm.
; Take abs of result and or it into current maxbit.
scoreTrailer macro
	abs	a  r2,r0	; restore current data pointer
	lsl	a  x:-(r1),x1	; need sign bit, get current maxBit
	or	x1,a		; save highest bit set
	move	a1,x:(r1)+
	endm

;---------------------------------------------------------------
; or-in the sign bit in y1 if last operation resulted in negative a
setSign macro
	jge	_pos
	neg	a
	or	y1,a		; set sign bit
_pos
	endm

;---------------------------------------------------------------
; This code follows after dspsoundssi.asm code in internal p memory
;---------------------------------------------------------------
	org	p:
	
;---------------------------------------------------------------
; Pack shorts into bits.
; On input:
;   r1	packed shorts (destination, advanced)
;   r2	unpacked shorts (source, advanced)
;   n2	source skip factor
;   y0  number of shorts to pack (destroyed)
;   y1	number of bits per packed short (preserved)
; On output:
;   a1  number of resulting shorts
; Register usage (destroyed):
;   r3	right shift multiply constants
;   n3	right shift amount
;   r4	left shift mulitply constants
;   n4  left shift amount
;   r5	original destination pointer
;   x0	scratch
;   x1	saved packed short
;   y0	scratch
;   b	bit phase
;   a	scratch
pack
	.if	y1 <eq> #0	; return 0 if num bits==0
		rts
	.endi
	move	r1,r5
	move	#rightShifts,r3
	move	#leftShifts,r4
	clr	a  #>16,b	; phase = 16
	move	a1,x1		; clear current packed short
	
	.loop	y0
	move	x:(r2)+n2,a	; get next unpacked short
	sub	y1,b		; phase -= numBits
	jgt	nosplit
	jlt	split
	or	x1,a		; or in current packed short
	move	a1,x:(r1)+	; save packed short
	clr	a  #>16,b	; phase = 16
	move	a1,x1		; clear current packed short
	jmp	continue
split
	abs	b		; phase = numBits - phase
	move	b1,n3
	move	a1,y0
	move	x:(r3+n3),x0
	mpy	y0,x0,a		; right shift (numBits - phase)
	or	x1,a		; or in current packed short
	move	a1,x:(r1)+	; save packed short
	move	#>16,b
	move	n3,x0
	sub	x0,b
	move	b1,n4
	nop
	move	x:(r4+n4),x0
	mpy	y0,x0,a		; left shift (16 - numBits - phase)
	move	a0,x1		; current packed short
	jmp	continue
nosplit
	move	b1,n4
	move	a1,y0
	move	x:(r4+n4),x0
	mpy	y0,x0,a		; left shift (phase - numBits)
	move	a0,a1
	or	x1,a		; or in current packed short
	move	a1,x1
continue
	nop			; LA-1
	nop			; LA
	.endl
	
	.if	b1 <ne> #>16
		move	x1,x:(r1)+	; save final packed word
	.endi
	move	r1,a
	move	r5,b
	sub	b,a		; return packed short count
	rts

;---------------------------------------------------------------
; Read next value from host into a1
readHost
	jclr	#HRDF,x:HSR,readHost
	movep	x:HRX,a
	rts

;---------------------------------------------------------------
; Get parameters from host.  Sets dropMask.
; Destroys r0, n0, x0, x1, a and b
getParams
	if	SOUND_HEADER
		jsr	readHost
		move	#>6,b		; 3 ints (magic, dataLocation, dataSize) are
		sub	b,a		;  read separatetly
		move	a1,x:headerSize
	endif
	jsr	readHost
	move	a1,x:channelCount
	jsr	readHost
	move	a1,x:method
	jsr	readHost
	move	a1,x:numDropped
	move	#leftShifts,r0
	clr	b  a1,n0
	not	b
	move	b1,x0
	move	x:(r0+n0),x1		; left shift multiply factor
	mpy	x0,x1,a
	move	a0,a1
	not	a
	move	a1,x:dropMask		; dropMask = ~((~0)<<dropBits)
	jsr	readHost
	move	a1,x:encodeLength
	jsr	readHost
	move	a1,x:sampleSkip
	rts

	if	SOUND_HEADER
;---------------------------------------------------------------
; Read encodeLength stereo samples from the input source
; HOST version - sign extend samples.
; Compands by numDropped and saves the residue.
; Destroys a, r0, n0, r1, r2, x0, x1, y0, y1, r5, n5, and b
readSamples
	move	#rightShifts,r0
	move	x:numDropped,n0
	move	#leftResidue,r1
	move	#rightResidue,r2
	move	x:dropMask,y0
	move	x:(r0+n0),y1		; right shift multiply factor
	move	#sampleBuf,r0
	move	x:encodeLength,a
	.loop	a1
		getSample
		signx
		move	a1,x1
		and	y0,a
		move	a1,x:(r1)+	; save left residue
		mpy	x1,y1,a
		move	a1,x:(r0)+	; save left sample
		.if	x:channelCount <eq> #>2
			getSample
			signx
			move	a1,x1
			and	y0,a
			move	a1,x:(r2)+	; save right residue
			mpy	x1,y1,a
			move	a1,x:(r0)+	; save right sample
		.endi
		nop
	.endl
	rts
	else
;---------------------------------------------------------------
; Read encodeLength samples from the input source
; SSI version - always read stereo.
; Compands by numDropped and saves the residue.
; Destroys a, r0, n0, r1, r2, x0, x1, y0, y1, r5, n5, and b
readSamples
	move	#rightShifts,r0
	move	x:numDropped,n0
	move	#leftResidue,r1
	move	#rightResidue,r2
	move	x:dropMask,y0
	move	x:(r0+n0),y1		; right shift multiply factor
	move	#sampleBuf,r0
	move	x:encodeLength,a
	move	x:sampleSkip,b
	move	#>4,x1
	cmp	x1,b
	jne	_noSkip
	asl	a			; encodeLength*2 for 22K hack
_noSkip
	.loop	a1
		getSample
		move	a1,x1
		and	y0,a
		move	a1,x:(r1)+	; save left residue
		mpy	x1,y1,a
		move	a1,x:(r0)+	; save left sample
		getSample
		move	a1,x1
		and	y0,a
		move	a1,x:(r2)+	; save right residue
		mpy	x1,y1,a
		move	a1,x:(r0)+	; save right sample
	.endl
	rts
	endif
	
;---------------------------------------------------------------
; Send data from packedBuf to host
; Count passed in a1
; Destroys a, r0, b
sendData
	move	#packedBuf,r0
	.loop	a1
		move	x:(r0)+,a
		jsr	putHost
		nop
	.endl
	rts

	if	*>$200
	warn	'INTERNAL P MEMORY OVERFLOW!!!'
	endif
	
;---------------------------------------------------------------
; This code follows buffers in external memory
;---------------------------------------------------------------
	org	p:EXTERNAL_CODE

;---------------------------------------------------------------
; If trying to run in real-time, use a special version of score
	if	REAL_TIME
;---------------------------------------------------------------
; Run samples through each algorithm and set the winner and
; the minimum number of bits needed to encode the buffer.
; Sample buffer pointer passed in r0
; Destroys a, b, x0, x1, y0, r0, n0, r1, n1, r2
score
	clr	a  x:sampleSkip,n0
	move	a1,x0			; clear maxBits counter

	move	#>1<<(MAX_BITS-1),y0	; MAX_BITS compression result
	move	x:encodeLength,a
	move	#>1,b
	sub	b,a
	.loop	a1
; d1_encode
		move	x:(r0)+n0,a
		move	x:(r0),b
		sub	b,a		; dat[j]-dat[j+1]
		abs	a
		lsl	a		; need sign bit
		or	x0,a		; save highest bit set
		move	a1,x0
	.endl

	move	#>2,a
	move	a1,x:algorithm		; ignored if min == MAX_BITS (see below)

	; Find number of bits set in min number
	move	#>@cvi(@pow(2,(24-MAX_BITS)-1)),x1	; left shift multiply factor
	mpy	x0,x1,a
	move	a0,a1
	.loop	#MAX_BITS
		lsl	a
		jcc	nextBit
		movec	lc,a
		enddo
		jmp	bitCount
nextBit
		nop		; needed because of movec lc,a above
		nop
	.endl
bitCount
	move	a1,x:numBits
	move	#>1,x0
	sub	x0,a		; numBits-1
	jle	zeroBits
	move	a1,n0
	move	#leftShifts,r0
	nop
	move	x:(r0+n0),x1	; left shift multilpy factor
	mpy	x0,x1,b		; if (numBits>0) sign = 1<<(numBits-1)
	move	b0,x0
zeroBits
	move	x0,x:sign
	.if	x:method <eq> #>BIT_FAITHFUL
		move	x:numBits,a
		move	x:numDropped,b
		add	b,a
		move	a1,x:numBits
	.endi
	.if	x:numBits <gt> #>16
		move	#>16,a
		move	a1,x:numBits
		clr	a
		move	a,x:algorithm
	.endi
	rts

;---------------------------------------------------------------
; Not real-time, use full version of score
	else
;---------------------------------------------------------------
; Run samples through each algorithm and set the winner and
; the minimum number of bits needed to encode the buffer.
; Sample buffer pointer passed in r0
; Destroys a, b, x0, x1, y0, r0, n0, r1, n1, r2
score
	move	#maxBits,r1
	clr	a  x:sampleSkip,n0
	rep	#NUM_ENCODES
	move	a1,x:(r1)+		; clear maxBits array

	move	#>1<<(MAX_BITS-1),y0	; MAX_BITS compression result
	move	x:encodeLength,a
	move	#>1,b
	sub	b,a
	.loop	a1
		move	#maxBits+1,r1	; algorithm 0 (null encode) not checked
		move	r0,r2

; XOR encoding
		checkMax d1_encode
		move	x:(r0)+n0,x0	; dat[j]
		move	x:(r0),a	; dat[j+1]
		eor	x0,a  x:-(r1),x1 ; dat[j]^dat[j+1], get current maxBit
		move	#$00FFFF,x0
		and	x0,a		; clear bits 16-24
		or	x1,a  r2,r0	; save highest bit set
					;  restore current data pointer
		move	a1,x:(r1)+
d1_encode
		checkMax d2_encode
		move	x:(r0)+n0,a
		move	x:(r0),b
		sub	b,a		; dat[j]-dat[j+1]
		scoreTrailer		; also restores r2,r0
d2_encode
		checkMax d3_encode
		checkCnt 2,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b		; dat[j+1]*2
		sub	b,a  x:(r0),x0	; dat[j]-dat[j+1]*2
		add	x0,a		; dat[j]-dat[j+1]*2+dat[j+2]
		scoreTrailer
d3_encode
		checkMax d4_encode
		checkCnt 3,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a
		sub	b,a
		sub	b,a  x:(r0)+n0,x0 ; dat[j]-(3*dat[j+1])
		add	x0,a
		add	x0,a
		add	x0,a x:(r0),b	; dat[j]-(3*dat[j+1])+(3*dat[j+2])
		sub	b,a		; dat[j]-(3*dat[j+1])+(3*dat[j+2])-dat[j+3]
		scoreTrailer
d4_encode
		checkMax d3_11_encode
		checkCnt 4,d3_11_encode
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+1]*4
		sub	b,a		; dat[j]-(dat[j+1]*4)
		move	x:(r0)+n0,b
		asl	b		; dat[j+2]*2
		add	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)
		asl	b		; dat[j+2]*4
		add	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+3]*4
		sub	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
					; -(dat[j+3]*4)
		move	x:(r0),b
		add	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
					; -(dat[j+3]*4)+dat[j+4]
		scoreTrailer
d3_11_encode
		checkMax d3_22_encode
		checkCnt 3,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		move	x:(r0)+n0,b
		add	b,a		; dat[j]-dat[j+1]+dat[j+2]
		move	x:(r0),b
		sub	b,a		; dat[j]-dat[j+1]+dat[j+2]-dat[j+3]
		scoreTrailer
d3_22_encode
		checkMax d4_222_encode
		checkCnt 3,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		sub	b,a		; dat[j]-dat[j+1]-dat[j+1]
		move	x:(r0)+n0,b
		add	b,a		; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]
		add	b,a		; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]+dat[j+2]
		move	x:(r0),b
		sub	b,a		; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]+dat[j+2]
					; -dat[j+3]
		scoreTrailer
d4_222_encode
		checkMax d4_343_encode
		checkCnt 4,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b		; dat[j+1]*2
		sub	b,a		; dat[j]-(dat[j+1]*2)
		move	x:(r0)+n0,b
		asl	b		; dat[j+2]*2
		add	b,a		; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)
		move	x:(r0)+n0,b
		asl	b		; dat[j+3]*2
		sub	b,a		; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)-(dat[j+3]*2)
		move	x:(r0),b
		add	b,a		; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)-(dat[j+3]*2)
					; +dat[j+4]
		scoreTrailer
d4_343_encode
		checkMax d4_101_encode
		checkCnt 4,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		asl	b		; dat[j+1]*2
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+2]*4
		add	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]
		asl	b		; dat[j+3]*2
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]-(dat[j+3]*2)
		move	x:(r0),b
		add	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]-(dat[j+3]*2)+dat[j+4]
		scoreTrailer
d4_101_encode
		checkMax encodeEnd
		checkCnt 4,encodeEnd
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a  (r0)+n0	; dat[j]-dat[j+1]
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]-dat[j+3]
		move	x:(r0),b
		add	b,a		; dat[j]-dat[j+1]-dat[j+3]+dat[j+4]
		scoreTrailer
encodeEnd
		lua	(r0)+n0,r0	; next datum
	.endl

	; Find min number in maxBits array
	move	#maxBits+1,r1		; don't check null encode
	move	#>1<<(MAX_BITS-1),a	; MAX_BITS compression result
	.loop	#NUM_ENCODES-1
		move	x:(r1)+,b
		cmp	a,b
		jge	nextMax
		tfr	b,a		; new min
		movec	lc,x0		; save min algo number
nextMax
		nop
		nop			; needed because of movec lc,x0 above
	.endl
	move	#>NUM_ENCODES,b
	sub	x0,b			; algorithm number
	move	#scores,r0
	move	b1,x:algorithm		; ignored if min == MAX_BITS (see below)
	move	b1,n0
	move	a1,x0			; min number in maxBits array

	; Increment score of this algorithm
	move	x:(r0+n0),a
	move	#>1,b
	add	b,a
	move	a1,x:(r0+n0)

	; Find number of bits set in min number
	move	#>@cvi(@pow(2,(24-MAX_BITS)-1)),x1	; left shift multiply factor
	mpy	x0,x1,a
	move	a0,a1
	.loop	#MAX_BITS
		lsl	a
		jcc	nextBit
		movec	lc,a
		enddo
		jmp	bitCount
nextBit
		nop		; needed because of movec lc,a above
		nop
	.endl
bitCount
	move	a1,x:numBits
	move	#>1,x0
	sub	x0,a		; numBits-1
	jle	zeroBits
	move	a1,n0
	move	#leftShifts,r0
	nop
	move	x:(r0+n0),x1	; left shift multilpy factor
	mpy	x0,x1,b		; if (numBits>0) sign = 1<<(numBits-1)
	move	b0,x0
zeroBits
	move	x0,x:sign
	.if	x:method <eq> #>BIT_FAITHFUL
		move	x:numBits,a
		move	x:numDropped,b
		add	b,a
		move	a1,x:numBits
	.endi
	.if	x:numBits <gt> #>16
		move	#>16,a
		move	a1,x:numBits
		clr	a
		move	a,x:algorithm
	.endi
	rts
	endif

;---------------------------------------------------------------
; Copy samples, adding residue if bit-faithful.
; Source is r0, skip is n0, destination is r1, residue is r3,
; count is x1.
; Destroys a, x0, x1, r4, n4
copySamples
	.if	x:method <eq> #>BIT_FAITHFUL
		move	#leftShifts,r4
		move	x:numDropped,n4
		nop
		move	x:(r4+n4),x0
		.loop	x1
			move	x:(r0)+n0,x1	; dat[j]
			mpy	x1,x0,a		; dat[j] << numDropped
			move	a0,a1
			move	x:(r3)+,x1
			or	x1,a
			move	a1,x:(r1)+	; dat[j] = (dat[j] << numDropped)
					        ; | residue[k++];
		.endl
	.else
		.loop	x1
			move	x:(r0)+n0,a
			move	a1,x:(r1)+
		.endl
	.endi
	rts

;---------------------------------------------------------------
; Compress samples from sampleBuf pointed at by r0 to packedBuf
; Residue buffer passed in r3.
; Destroys a, b, x0, x1, y0, y1, r0, n0, r1, n1, r2, n2, r3, r4, n4, r5
; Returns packed count in a1
compress
	move	r0,r2			; save sample buf pointer
	move	x:algorithm,n1
	move	#routines,r1
	move	x:sampleSkip,n0
	move	x:(r1+n1),r5		; routine address
	move	#leaderCount,r1
	move	n1,x0
	move	x:(r1+n1),x1		; leading samples
	move	#>@cvi(@pow(2,8-1)),y0	; left shift multiply factor
	mpy	x0,y0,a
	move	a0,a1			; shifted algorithm code
	move	#packedBuf,r1
	move	x:numBits,x0
	or	x0,a  x:sign,y1
	move	a1,x:(r1)+		; buffer header - algorithm and numBits
	move	x:encodeLength,a
	sub	x1,a
	move	a1,y0			; number of samples to encode
	move	x:algorithm,a
	tst	a
	jeq	noLeading		; null encode has no leading samples
					; (or rather, ALL leading samples)
	jsr	copySamples		; copy unencoded leading samples
	move	r2,r0			; restore sample buf pointer
noLeading
	jsr	(r5)			; do the compression

	move	x:encodeLength,a
	move	x:algorithm,b
	tst	b
	jeq	no_packing		; no packing for null encode
	
	.if	x:method <eq> #>BIT_FAITHFUL
		move	#leftShifts,r4
		move	x:numDropped,n4
		move	r2,r0
		move	x:(r4+n4),x1
		.loop	y0
			move	x:(r0),x0
			mpy	x0,x1,a
			move	a0,a1
			move	x:(r3)+,x0
			or	x0,a
			move	a1,x:(r0)+n0
		.endl
	.endi

	; r1 - packed destination
	; r2 - unpacked, coded source
	; y0 - num to pack
	move	x:sampleSkip,n2
	move	x:numBits,y1
	jsr	pack

	; Total packed count is 1 (coded algo num and numbits) +
	; number of leading samples + packed count.

	move	x:algorithm,n4
	move	#leaderCount,r4
	move	#>1,x0
	move	x:(r4+n4),b
	add	x0,b
	add	b,a
no_packing
	rts

;---------------------------------------------------------------
; Compression algorithm routines
; On entry:
;	r0	sample buffer
;	n0	sample skip
;	r1	packed buffer
;	y0	number of samples to encode
;	y1	sign bit
;---------------------------------------------------------------

;---------------------------------------------------------------
; Null encode - just copy samples
; NOTE: this also implies no unpacking, i.e. 16 bits data.
; In practice this never happens because the minimum number of
; dropped bits is 4, so another alogorithm is bound to win
; on 12 bits.  Null encode is useful only for debugging.
null
	.loop	y0
		move	x:(r0)+n0,a
		move	a1,x:(r1)+
	.endl
	rts
	
;---------------------------------------------------------------
; xor encode - exclusive or
; dat[j] ^= dat[i+1]
xor
	.loop	y0
		move	x:(r0)+n0,x0	; dat[j]
		move	x:(r0),a	; dat[j+1]
		eor	x0,a  (r0)-n0	; dat[j]^dat[j+1]
		move	#$00FFFF,x0
		and	x0,a		; clear bits 16-24
		move	a1,x:(r0)+n0
	.endl
	rts

;---------------------------------------------------------------
; d1 encode - first differences
; dat[j] = dat[j] - dat[j+1]
d1
	.loop	y0
		move	x:(r0)+n0,a
		move	x:(r0),b
		sub	b,a  (r0)-n0	; dat[j]-dat[j+1]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d2 encode - second differences
; dat[j] = dat[j] - dat[j+1]*2 + dat[j+2]
d2
	.loop	y0
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b		; dat[j+1]*2
		sub	b,a  x:(r0)-n0,x0 ; dat[j]-dat[j+1]*2
		add	x0,a  (r0)-n0	; dat[j]-dat[j+1]*2+dat[j+2]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d3 encode - third differences
; dat[j] = dat[j]-(3*dat[j+1])+(3*dat[j+2])-dat[j+3]
d3
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a
		sub	b,a
		sub	b,a  x:(r0)+n0,x0 ; dat[j]-(3*dat[j+1])
		add	x0,a
		add	x0,a
		add	x0,a x:(r0),b	; dat[j]-(3*dat[j+1])+(3*dat[j+2])
		sub	b,a  r5,r0 	; dat[j]-(3*dat[j+1])+(3*dat[j+2])-dat[j+3]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d4 encode - fourth differences
; dat[j] = dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)-(dat[j+3]*4)+dat[j+4]
d4
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+1]*4
		sub	b,a		; dat[j]-(dat[j+1]*4)
		move	x:(r0)+n0,b
		asl	b		; dat[j+2]*2
		add	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)
		asl	b		; dat[j+2]*4
		add	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+3]*4
		sub	b,a		; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
					; -(dat[j+3]*4)
		move	x:(r0),b
		add	b,a  r5,r0	; dat[j]-(dat[j+1]*4)+(dat[j+2]*2)+(dat[j+2]*4)
					; -(dat[j+3]*4)+dat[j+4]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d3_11 encode - third differences, alternate coefficients
; dat[j] = dat[j]-dat[j+1]+dat[j+2]-dat[j+3]
d3_11
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		move	x:(r0)+n0,b
		add	b,a		; dat[j]-dat[j+1]+dat[j+2]
		move	x:(r0),b
		sub	b,a  r5,r0	; dat[j]-dat[j+1]+dat[j+2]-dat[j+3]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d3_22 encode - third differences, alternate coefficients
; dat[j] = dat[j]-dat[j+1]-dat[j+1]+dat[j+2]+dat[j+2]-dat[j+3]
d3_22
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		sub	b,a		; dat[j]-dat[j+1]-dat[j+1]
		move	x:(r0)+n0,b
		add	b,a		; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]
		add	b,a		; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]+dat[j+2]
		move	x:(r0),b
		sub	b,a  r5,r0	; dat[j]-dat[j+1]-dat[j+1]+dat[j+2]+dat[j+2]
					; -dat[j+3]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d4_222 encode - fourth differences, alternate coefficients
; dat[j] = dat[j]-(dat[j+1]*2)+(dat[j+2]*2)-(dat[j+3]*2)+dat[j+4]
d4_222
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		asl	b		; dat[j+1]*2
		sub	b,a		; dat[j]-(dat[j+1]*2)
		move	x:(r0)+n0,b
		asl	b		; dat[j+2]*2
		add	b,a		; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)
		move	x:(r0)+n0,b
		asl	b		; dat[j+3]*2
		sub	b,a		; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)-(dat[j+3]*2)
		move	x:(r0),b
		add	b,a  r5,r0	; dat[j]-(dat[j+1]*2)+(dat[j+2]*2)-(dat[j+3]*2)
					; +dat[j+4]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d4_343 encode - fourth differences, alternate coefficients
; dat[j] = dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)-dat[j+3]-(dat[j+3]*2)+dat[j+4]
d4_343
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]
		asl	b		; dat[j+1]*2
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)
		move	x:(r0)+n0,b
		asl	b
		asl	b		; dat[j+2]*4
		add	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]
		asl	b		; dat[j+3]*2
		sub	b,a		; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]-(dat[j+3]*2)
		move	x:(r0),b
		add	b,a  r5,r0	; dat[j]-dat[j+1]-(dat[j+1]*2)+(dat[j+2]*4)
					; -dat[j+3]-(dat[j+3]*2)+dat[j+4]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts
	
;---------------------------------------------------------------
; d4_101 encode - fourth differences, alternate coefficients
; dat[j] = dat[j]-dat[j+1]-dat[j+3]+dat[j+4]
d4_101
	.loop	y0
		move	r0,r5
		move	x:(r0)+n0,a
		move	x:(r0)+n0,b
		sub	b,a  (r0)+n0	; dat[j]-dat[j+1]
		move	x:(r0)+n0,b
		sub	b,a		; dat[j]-dat[j+1]-dat[j+3]
		move	x:(r0),b
		add	b,a  r5,r0	; dat[j]-dat[j+1]-dat[j+3]+dat[j+4]
		setSign
		move	a1,x:(r0)+n0
	.endl
	rts

;---------------------------------------------------------------
; Main loop
main
	clr	a  #scores,r1
	rep	#NUM_ENCODES
	move	a1,x:(r1)+	; clear scores array

	jsr	getParams
	jsr	start
	if	SOUND_HEADER
		getInt16		; ignore magic
		getInt16		; ignore dataLocation
		getInt48		; dataSize
		move	x:channelCount,y1
		rep	y1
		asr	a
		move	a,l:numSamples
		move	x:headerSize,y1
		.loop	y1
			jsr	getHost		; eat rest of sound header
			nop
		.endl
	endif
loop
	jsr	readSamples
	
	; Compress and send the first channel
	move	#sampleBuf,r0
	jsr	score
	move	#sampleBuf,r0
	move	#leftResidue,r3
	jsr	compress
	jsr	sendData
	
	; Compress and send the second channel
	.if	x:channelCount <eq> #>2
		move	#sampleBuf+1,r0
		jsr	score
		move	#sampleBuf+1,r0
		move	#rightResidue,r3
		jsr	compress
		jsr	sendData
	.endi

	if	SOUND_HEADER
		; Increment sample count
		move	x:encodeLength,y0
		move	#0,y1
		move	l:curSample,a
		add	y,a		; 48 bit curSample + encodeLength
		move	a,l:curSample
		move	l:numSamples,b
		cmp	b,a
		.if	<ge>
			jsr	stop	; flush and terminate
		.endi
	endif
	
	jmp	loop

	if	*>$4000
	warn	'EXTERNAL MEMORY OVERFLOW!!!'
	endif
