; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

; Code
__main
	MOV r0, #1		;
	MOV r1, #2		;
	MOV r2, #1		;
	
	LDR r3, =Other	;
	;BX r3			; Nhay toi mot thanh ghi chua dia chi
	;B Other			;
	BL Other		;
	
	CMP r0, #100	;
	BEQ BangNhau	;
	BNE KhacNhau	;
	
BangNhau
	MOV r2, #100	; Co Z len 1

KhacNhau
	MOV r2, #999	;
	
Other
	MOV r2, #69		;

STOP
	B STOP
	END