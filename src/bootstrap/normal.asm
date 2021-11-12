;------------------------------------------------------------
;
; ATARI STrinkler "normal" bootstrap by Leonard/Oxygene
; -normal version: depack in place & relocation
;
;------------------------------------------------------------

;	******************************00000000000		; original unpacked

;	HPPPPPPPPPPD00000000000000000000000000000000	; packed just loaded

;	H--------------------------------PPPPPPPPPPD	; move up

;	******************************T0000000RPPPPD	; decrunch & reloc

;	******************************00000000000PPD	; clear BSS

			move.l	4(a7),a6		; PRG basepage

		; update BasePage (for max compatibility)
			move.l	8(a6),a0
			
			move.l	#$12345678,d0		; code section size
			move.l	d0,12(a6)			
			add.l	d0,a0
			move.l	a0,24(a6)			; start of BSS segment
			move.l	#$12345678,28(a6)	; size of BSS segment

			lea		mEnd(pc),a0
			moveq	#(mEnd-mStart)/2-1,d0
copyc:		move.w	-(a0),-(a7)
			dbf		d0,copyc
			
			lea		mEnd(pc),a5
			movea.l	a5,a4
			move.l	#$12345678,d0
			add.l	d0,a5				; size (patched by STrinkler)
			add.l	#$12345678,a4		; end of the big buffer
			
			jmp		(a7)

mStart:
			move.w	-(a5),-(a4)
			subq.l	#2,d0
			bne.s	mStart
			
			move.l	8(a6),a5			; code start address (from basepage)


;------------------------------------------------------------
;
; Shrinkler depacking code by Blueberry/Loonies
;
; The code herein is free to use, in whole or in part,
; modified or as is, for any legal purpose.
;
; No warranties of any kind are given as to its behavior
; or suitability.
;
; Decompress Shrinkler-compressed data produced with the --data option.
;
; A4 = Compressed data
; A5 = Decompressed data destination
;
; Uses 3 kilobytes of space on the stack.
; Preserves D2-D7/A2-A6 and assumes callback does the same.
;
; Decompression code may read one longword beyond compressed data.
; The contents of this longword does not matter.

INIT_ONE_PROB		=	$8000
ADJUST_SHIFT		=	4
SINGLE_BIT_CONTEXTS	=	1
NUM_CONTEXTS		=	1536

; input:
;	a4: packed data
;	a5: output buffer


ShrinklerDecompress:
;	movem.l	d2-d7/a4-a6,-(a7)

;	move.l	a0,a4
;	move.l	a1,a5


	; Init range decoder state
	moveq.l	#0,d2
	moveq.l	#1,d3
;	moveq.l	#1,d4
;	ror.l	#1,d4
	moveq	#-128,d4

	; Init probabilities
	IF NUM_CONTEXTS=1536
	{
		moveq	#96,d6
		lsl.w	#4,d6
	}
	ELSE
	{
		move.l	#NUM_CONTEXTS,d6
	}
.init:	move.w	#INIT_ONE_PROB,-(a7)
	subq.w	#1,d6
	bne.b	.init

	; D6 = 0
.lit:
	; Literal
	addq.b	#1,d6
.getlit:
	bsr	GetBit
	addx.b	d6,d6
	bcc.b	.getlit
	move.b	d6,(a5)+
;	move.w	d6,$ffff8240.w
.switch:
	; After literal
	bsr.b	GetKind
	bcc.b	.lit
	; Reference
	moveq.l	#-1,d6
	bsr.b	GetBit
	bcc.b	.readoffset
.readlength:
	moveq.l	#4,d6
	bsr.b	GetNumber
.copyloop:
	move.b	(a5,d5.l),(a5)+
	subq.l	#1,d7
	bne.b	.copyloop
	; After reference
	bsr.b	GetKind
	bcc.b	.lit
.readoffset:
	moveq.l	#3,d6
	bsr.b	GetNumber
	moveq.l	#2,d5
	sub.l	d7,d5
	bne.b	.readlength

	lea.l	(NUM_CONTEXTS*2+mEnd-mStart)(a7),a7		; back to original a7

;--------------------------------------------------------------
; code relocation (using STrinkler specific relocation table)
;--------------------------------------------------------------
mReloc:		
			move.l	8(a6),a1			; code start address (from basepage)
			move.l	a1,d1
			lea		$1234(a5),a5		; move back to start of relocation table (patched by STrinkler)
.rloop:		move.w	(a5)+,d0
			bmi.s	.over
			bne.s	.ok
			lea		32766(a1),a1
			bra.s	.rloop
.ok:		add.w	d0,a1
			add.l	d1,(a1)
			bra.s	.rloop
.over:		

		; clear BSS
			move.l	24(a6),a0	; start of BSS segment
			move.l	28(a6),d0	; size of BSS segment
.clbss:		clr.w	(a0)+
			subq.l	#2,d0
			bgt.s	.clbss
			move.l	d1,a6

			jmp		(a6)


GetKind:
	; Use parity as context
	move.l	a5,d1
	moveq.l	#1,d6
	and.l	d1,d6
	lsl.w	#8,d6
	bra.b	GetBit

GetNumber:
	; D6 = Number context

	; Out: Number in D7
	lsl.w	#8,d6
.numberloop:
	addq.b	#2,d6
	bsr.b	GetBit
	bcs.b	.numberloop
	moveq.l	#1,d7
	subq.b	#1,d6
.bitsloop:
	bsr.b	GetBit
	addx.l	d7,d7
	subq.b	#2,d6
	bcc.b	.bitsloop
	rts

	; D6 = Bit context

	; D2 = Range value
	; D3 = Interval size
	; D4 = Input bit buffer

	; Out: Bit in C and X

readbit:
	add.b	d4,d4
	bne.b	nonewword
	move.b	(a4)+,d4
	addx.b	d4,d4
nonewword:
	addx.w	d2,d2
	add.w	d3,d3
GetBit:
	tst.w	d3
	bpl.b	readbit

	lea.l	4+SINGLE_BIT_CONTEXTS*2(a7,d6.l),a1
	add.l	d6,a1
	move.w	(a1),d1
	; D1 = One prob

	lsr.w	#ADJUST_SHIFT,d1
	sub.w	d1,(a1)
	add.w	(a1),d1

	mulu.w	d3,d1
	swap.w	d1

	sub.w	d1,d2
	blo.b	.one
.zero:
	; oneprob = oneprob * (1 - adjust) = oneprob - oneprob * adjust
	sub.w	d1,d3
	; 0 in C and X
	rts
.one:
	; onebrob = 1 - (1 - oneprob) * (1 - adjust) = oneprob - oneprob * adjust + adjust
	add.w	#$ffff>>ADJUST_SHIFT,(a1)
	move.w	d1,d3
	add.w	d1,d2
	; 1 in C and X
	rts
mEnd:
	