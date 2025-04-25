; Khai bao vung ma lenh
	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT __main
		
; Khai bao du lieu

; Code
__main
	PUSH    {LR}             ; preserve return address

    ; Example input: change R0 to test cases
    MOV     R0, #1           ; R0 = 1 (change to 0,2 or >2 to hit default)

    ; Bound check: if R0 > 2, go to default
    CMP     R0, #2
    BHI     default_case

    ; Branch via jump table
    LDR     R1, =jump_table  ; load address of table
    LDR     PC, [R1, R0, LSL #2]

case0  ; R0 == 0
    MOV     R2, #100         ; result = 100
    B       finish_switch

case1  ; R0 == 1
    MOV     R2, #200         ; result = 200
    B       finish_switch

case2  ; R0 == 2
    MOV     R2, #300         ; result = 300
    B       finish_switch

default_case
    MOV     R2, #0xFF        ; default result = 255 (error code)

finish_switch
    POP     {PC}             ; return

    ; Data section for jump table
    AREA    SwitchCase, DATA, READONLY
    ALIGN
jump_table
    DCD     case0, case1, case2   ; table of branch targets

    END                      ; end of file
