#include <stdint.h>
#include <stdio.h>

static int control_t, value_2, ptr_1;

static void read_control(void) {
    int value_1 = 8;
		int * ptr_2;
		ptr_2 = (int *)0x20000004;
	
    // Dùng inline assembly d? load giá tr? CONTROL vào control_t
    __asm__ volatile (
        "MRS %0, CONTROL"    /* Move CONTROL register ? %0 */
        : "=r" (control_t)    /* output: write-only register */
        :                      /* no inputs */
        : "cc"                /* clobber condition flags */
    );

    // Sao chép value_1 vào value_2 b?ng asm
    __asm__ volatile (
        "MOV %0, %1"         /* %0 = %1 */
        : "=r" (value_2)      /* output */
        : "r"  (value_1)      /* input */
        :                      /* no clobbers needed */
    );
		
		// copy pointer
		__asm__ volatile (
        "LDR %0, [%1]"         /* %0 = %1 */
        : "=r" (ptr_1)      /* output */
        : "r"  (ptr_2)      /* input */
        :                      /* no clobbers needed */
    );
}

int main(void) {
    int value = 99;

    // Ví d? d?c/ghi b? nh? t?i d?a ch? c? d?nh
    __asm__ volatile ("LDR r1, =0x20001000");
    __asm__ volatile ("LDR r2, =0x20001004");
    __asm__ volatile ("LDR r0, [r1]");
    __asm__ volatile ("LDR r1, [r2]");
    __asm__ volatile ("ADD r0, r0, r1");
    __asm__ volatile ("STR r0, [r2]");

    // Ðua bi?n C 'value' vào r0
    __asm__ volatile (
        "MOV r0, %0"
        : /* no outputs */
        : "r" (value)
        : /* no clobbers */
    );

    read_control();
    printf("CONTROL register = 0x%08X, MOV result = %d\n", control_t, value_2);

    // Vòng vô h?n
    for (;;) ;
    return 0;
}
