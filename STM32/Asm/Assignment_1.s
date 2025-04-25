; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu
N EQU 10

; Code
__main
	
	MOV r0, #0		;
	LDR r1, =N		;
	
Tinhtong
	ADD r0, r0, 1	; r0 = r0 + 1
	SUBS r1, r1, #1	; r1 = r1 - 1
	CMP r1, #0		;
	BGT Tinhtong	;
	
STOP
	B STOP
	END