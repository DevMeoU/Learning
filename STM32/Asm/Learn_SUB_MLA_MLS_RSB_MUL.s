; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu
BASE EQU 0x203		; const BASE = 0x203
Hello DCB "HelloSonLD\n", 0
a_8_bit DCB 2,3,4,5
b_32_bit DCD 2,100,-10
c_16_bit DCW 100,300,10
s SPACE 200
f FILL 20,0xFF,1
Bin DCB 2_01010100
Oc DCB 8_72
ch DCB 'A'


; Code
__main
	MOV R1, #10
	MOV R2, #2
	MOV R3, #1
	
	; Tru
	SUB R1, #5			; R1 = R1 - 5
	SUB R1, R2			; R1 = R1 - R2
	SUB R1, R2, R3		; R1 = R2 - R3 SUBS tru so nguyen co dau
	;SUB R0, R0, R2, LSL, #2		; LSL: lenh dich trai <=> R0 = R0 - (R2 << 2)
	;ADD R1, R1, R3, LSR, #1		; LSR; lenh dich phai <=> R1 = R1 + (R3 >> 1)
	
	; Nhan
	MUL R1, R2			; R1 = R1 * R2
	MUL R1, R2, R3		; R1 = R2 * R3
	
	; Nhan to hop
	MLA R1, R2, R3, R4		; R1 = (R2 * R3) + R4
	MLS R1, R2, R3, R4		; R1 = R4 - (R2 * R3)
	RSB R1, R2, #3			; R1 = -R2 + 3
	
	
STOP
	B STOP
	END