; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu
value DCD 123
	
; Code
__main
    MOV     R0, #10            ; R0 = 10   (n = 10)

    LDR     LR, =return_point  ; 
    B       sum_function       ; 

return_point
    B       .                  ; 

;================================================================
; 
;================================================================
sum_function
    PUSH    {R4, LR}           ;
    MOV     R4, #0             ;
    MOV     R1, #0             ;

calc_loop
    CMP     R1, R0             ;
    BGE     calc_done
    ADD     R4, R4, R1         ;
    ADD     R1, R1, #1         ;
    B       calc_loop

calc_done
    MOV     R0, R4             ;
    POP     {R4, PC}           ;

    END
