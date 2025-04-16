#ifndef USART_PRINT_H
#define USART_PRINT_H

#include <stdint.h>
#include <stddef.h>

#ifndef HAL_H
#define HAL_H
#include "mik32_hal_usart.h"
#include "mik32_hal_scr1_timer.h"
#endif

#define UINT64_BUFFER_SIZE 21
#define FLOAT_BUFFER_SIZE 50
#define TICKS_PER_SECOND 32000000 // 32MHz
#define FLOAT_PRECISION 3
#define FLOAT_EXTENSION_COEF 1000

typedef uint32_t (*function_to_profile)(uint32_t);

void usart_print_uint64(uint64_t value);
void usart_print_float(float value);
char* write_uint64_to_buffer(uint64_t value, char* buffer);
size_t get_uint64_length(uint64_t value);
uint64_t profile_function(function_to_profile f, char* f_name, uint32_t arg);
void compare_profiling_results(uint64_t res1, uint64_t res2);

#endif /* USART_PRINT_H */