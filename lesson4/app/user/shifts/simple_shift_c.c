#include "my_gpio_config.h"

#define BITS_IN_BYTE 8

void simple_shift_c(uint8_t symbol) {
    *GPIO0_CLR = 1 << STCP_PIN;
    for (int bit_pos = 0; bit_pos < BITS_IN_BYTE; ++bit_pos) {
        if (symbol & (1 << bit_pos)) {
            *GPIO0_SET = 1 << DATA_PIN;
        } else {
            *GPIO0_CLR = 1 << DATA_PIN;
        }
        *GPIO0_SET = 1 << SHCP_PIN;
        *GPIO0_CLR = 1 << SHCP_PIN;
    }
    *GPIO0_SET = 1 << STCP_PIN;
}