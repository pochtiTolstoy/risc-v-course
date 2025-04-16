#include "fibonachi.h"

uint32_t fibonachi_recursive(uint32_t n) {
    if (n == 0) {
        return 0;
    }
    if ((n == 1) || (n == 2)) {
        return 1;
    }
    return fibonachi_recursive(n - 1) + fibonachi_recursive(n - 2);
}

uint32_t fibonachi_iterative(uint32_t n) {
    if (n <= 1) return n;
    uint32_t prev1 = 1;
    uint32_t prev2 = 0;
    uint32_t res = 0;
    for (uint32_t i = 2; i <= n; ++i) {
        res = prev1 + prev2;
        prev2 = prev1;
        prev1 = res;
    }
    return res;
}