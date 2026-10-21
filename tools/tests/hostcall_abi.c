/* Regression: the ground-asset cut returns an aggregate through x8. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern uint64_t dm_guest_call_regs(void *fn, uint64_t *regs);
typedef struct { uint64_t words[20]; } Result;

__attribute__((noinline)) Result target(uint64_t a, uint64_t b, uint64_t c, uint64_t d,
                                      uint64_t e, uint64_t f, uint64_t g, uint64_t h)
{
    Result out;
    for (int i = 0; i < 20; i++)
        out.words[i] = a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h + i;
    return out;
}

__attribute__((noinline)) double floating(double a, double b) { return a*3 + b; }

int main(void)
{
    _Alignas(16) uint64_t regs[100] = {0};
    struct { uint64_t before; Result value; uint64_t after; } out = {
        .before = 0x12345678, .after = 0x87654321
    };
    for (int i = 0; i < 8; i++)
        regs[i] = i + 1;
    regs[8] = (uintptr_t)&out.value;
    dm_guest_call_regs((void *)target, regs);
    for (int i = 0; i < 20; i++)
        if (out.value.words[i] != 204 + i)
            return 1;
    if (out.before != 0x12345678 || out.after != 0x87654321)
        return 2;

    double a = 1.5, b = 2.25, result;
    memcpy((char *)regs + 256, &a, 8);
    memcpy((char *)regs + 272, &b, 8);
    dm_guest_call_regs((void *)floating, regs);
    memcpy(&result, (char *)regs + 256, 8);
    if (result != 6.75)
        return 3;
    puts("PASS: eight integer arguments, x8 aggregate result with guards, SIMD arguments/result");
    return 0;
}
