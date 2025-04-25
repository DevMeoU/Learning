; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu
N DCD 10

; Code
__main
	
	MOV r0, #0		; Bien tang 2
	LDR r1, N		;
	MOV r2, #0		; Tong cac so chan
	
LoopTongcacsochanbehonN
	CMP r1, r0	;
	BHI Benduoi
	B Thoat

Benduoi
	ADD r2, r0	; r2 = r2 + r0
	ADD r0, #2	; r0 = r0 + 2
	B LoopTongcacsochanbehonN

Thoat
	
STOP
	B STOP
	END