; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

	
; Code
__main
	MOV R2, #2		;
	MOV R1, #1
	
	LSL R1, #2		; R1 << 2
	LSR R2, #1		; R2 >> 1
	
	MOV R1, #2_00101110
	ROR R2, R1, #1 	; Dich phai
	ROL R3, R1, #1	; Dich trai
	
STOP
	B STOP
	END