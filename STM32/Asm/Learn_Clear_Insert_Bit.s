; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

	
; Code
__main
	LDR R0, =0x12345678		;
	LDR R1, =0xFFFFFFFF		;
	
	BFI R1, R0, #4,#8		; Insert 8 bit of R0 register into bit 4 position
	
	LDR R0, =0x000000FF	;
	BFC R0, #4, #4			; Clear 4 bit from bit position index 4 in R0; 0x0000000F
	
STOP
	B STOP
	END