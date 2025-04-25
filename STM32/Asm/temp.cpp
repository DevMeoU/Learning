#include <cstdio>

// compute: dùng inline-asm d? tính a+b và a-b
// - a, b: input
// - sum, diff: con tr? ch?a k?t qu?
void compute(int a, int b, int *sum, int *diff) {
    // Ð? xu?t cho compiler gi? tmp_sum vào r4, tmp_diff vào r5
    register int tmp_sum  asm("r4");
    register int tmp_diff asm("r5");

    __asm__ volatile (
        // --- 1. Assembly template ---
        "ADD %[s], %[x], %[y]\n\t"   // tmp_sum = a + b
        "SUB %[d], %[x], %[y]\n\t"   // tmp_diff = a - b

        // --- 2. Output operands ---
        : [s] "=&r" (tmp_sum),       // %[s] s? là m?t register write-only (& early-clobber)
          [d] "=&r" (tmp_diff)       // %[d] cung v?y

        // --- 3. Input operands ---
        : [x] "r" (a),               // %[x] l?y giá tr? t? C bi?n a vào register
          [y] "r" (b)                // %[y] l?y giá tr? t? C bi?n b vào register

        // --- 4. Clobbers ---
        : "cc"                       // báo compiler: các flag condition (NZCV) s? b? thay d?i
    );

    // Gán k?t qu? tr? l?i memory qua con tr?
    *sum  = tmp_sum;
    *diff = tmp_diff;
}

int main() {
    int a = 10, b = 3;
    int s, d;

    compute(a, b, &s, &d);
    std::printf("a = %d, b = %d ? sum = %d, diff = %d\n", a, b, s, d);
    return 0;
}
