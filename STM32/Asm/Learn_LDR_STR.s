; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu
st DCD 1,2,3,4,5

; Code
__main
	
	LDR R1, st			;
	LDR R1, =st			;Dia chi cua st[0]
	LDR R3, =4			;
	LDR	R2, [R1]		;
	LDR R2, [R1, R3]	;
	LDR R2, [R1], #4	;
	LDR R8, =0x80000300	;
	
	LDR R0, =0xABCDABCD	;
	LDR R1, =0x20000100	;
	STR R0, [R1]		; Luu gia tri R0 vao gia tri dia chi R1
	STR R2, [R1, #4]	; Luu gia tri R0 vao gia tri dia chi R1
	STR R0, [R1], #4	;
	
STOP
	B STOP
	END