; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

; Code
__main
	MOV R0, #0		;
	MOV R1, #1		;
	MOV R2, #1		;
	
	AND R0, R1		; R0 = R0 & R1
	AND R3, R2, R1	; R3 = R2 & R1
	AND R2, R1, #2	; R2 = R1 & 2
	
	ORR	R0, R1		; R0 = R0 | R1
	ORR R4, R1, R2	; R4 = R1 | R2
	
	
STOP
	B STOP
	END