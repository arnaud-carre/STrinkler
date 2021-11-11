;------------------------------------------------------------
;
; 	ATARI STrinkler special relocation code
; 	Specific relocation data more suited to Shrinkler 16bits contexts
;	Leonard/Oxygene
;
;------------------------------------------------------------

; input: 
;	a6:	code start
;	a5: end of depacked EXE

mReloc:		
			movea.l	a6,a1
			move.l	a6,d1
			lea		.datar(pc),a5
.rloop:		move.w	(a5)+,d0
			bmi.s	.over
			bne.s	.ok
			lea		32766(a1),a1
			bra.s	.rloop
.ok:		add.w	d0,a1
			add.l	d1,(a1)
			bra.s	.rloop
.over:		jmp		(a6)
.datar:
; here STrinkler puts all w16 relocation offsets
