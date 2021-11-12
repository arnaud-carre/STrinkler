;------------------------------------------------------------
;
; ATARI STrinkler "normal" bootstrap by Leonard/Oxygene
; -normal version: depack in place & relocation
;
;------------------------------------------------------------

;	******************************00000000000		; original unpacked

;	HPPPPPPPPPPD00000000000000000000000000000000	; packed just loaded


;              a5                              a4
;	           |                               |
;	--------------------------------HPPPPPPPPPPD	; move up

;   a5                              a4
;	|							    |
;	******************************T0000000RPPPPD	; start decrunch

;                                  a5          a4
;	                               |           |
;	******************************T0000000RPPPPD	; start reloc

;                                              a4
;	                                           |
;	******************************0000000000000D	; clear bss

;	******************************0000000000000D	; jmp


mStart:	
		lea		mStart(pc),a5
		movea.l	a5,a4
		move.l	#$12345678,d0			; size (patched by STrinkler)
		add.l	d0,a5					; end of packed data
		add.l	#$12345678,a4			; end of the big buffer
		lea		$4afc(a4),a6			; patched by STrinkler, points on "normal" code (decrunch+reloc)
copym:	move.w	-(a5),-(a4)
		subq.l	#2,d0
		bne.s	copym
		lea		packedData-mStart(a4),a4
		jmp		(a6)
packedData:
