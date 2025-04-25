; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

; Code
__main
	; Nhom lenh dieu kien
	MOV r3, #-1;
	MOV r4, #-2;
	ADDS r3, r4, r3 ; ADDS cho phep cong so am
	
	LDR r3, =0x7B000000;
	LDR r4, =0x30000000;
	ADDS r5, r4, r3

	MOV r7, #1	;
	SUBS r7, r7, #1	;


	MOV r1, #1		;
	MOV r2, #2		;
	
	CMP r1, #1
	ADDEQ r0, r1, r2		;
	
	CMP r1, r2
	ADDGT r0, r1, r2		; r1 >= r2		r0 = r1 + r2
	SUBLE r0, r2, r1		; r1 < r2		r0 = r2 - r1
	
	; if (r0 == 1) || (r0 == 2) r1 = r1 + 5
	;MOV r0, #1		;
	MOV r0, #2
	
	TEQ r0, #1		;
	TEQNE r0, r2	;
	ADDEQ r1, r1, #5	;
	
	; N: Negative flag { MOV r3, #-1; MOV r4, #-2; ADDS r3, r4, r3; }  
	; Z: Zero flag
	; C: Carry or borrow flag
	; V: Overload flag { LDR r3, =0x7B000000; LDR r4, =0x30000000; ADDS r5, r4, r3}
		
STOP
	B STOP
	END