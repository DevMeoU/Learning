; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

; Code
__main
	MOV r0, #0		; start from 0
	MOV r1, #0		; handle
	MOV r4, #10		; count to 10
	B FOR			; Jump to FOR function
	
FOR
	CMP r0, r4		; compare bitween r1, r4
	;BEQ	NEXT		; branch to NEXT
	BGE	NEXT		; more than or equal to
	ADD r1, r1, r0	; add r2 = r2 + r1
	ADD r0, r0, #1	; count up
	B FOR
	
NEXT
	MOV r0, #0		; Assign 0 for r1 register
	
STOP
	B STOP
	END