#include "usart_print.h"

USART_HandleTypeDef husart0;

void usart_print_uint64(uint64_t value) {
    char buffer[UINT64_BUFFER_SIZE];
    if (write_uint64_to_buffer(value, buffer)) {
        HAL_USART_Print(&husart0, buffer, USART_TIMEOUT_DEFAULT);
    }
}

void usart_print_float(float value) {
    char buffer[FLOAT_BUFFER_SIZE];
    char* output_iterator = buffer;

    // handle negative value
    if (value < 0) {
        *output_iterator++ = '-';
        value = -value;
    }

    // check max integer part of value
    if (value > (float)UINT64_MAX) {
        HAL_USART_Print(&husart0, "Error: integer part overflow\r\n", USART_TIMEOUT_DEFAULT);
        return;
    }

    // print integer part of value
    uint64_t integer_part = (uint64_t)value;
    output_iterator = write_uint64_to_buffer(integer_part, output_iterator);
    if (output_iterator == NULL) {
        return;
    }

    // print fractional part of value
    value -= (float)integer_part;
    uint32_t fractional_part = (uint32_t) (value * FLOAT_EXTENSION_COEF);
    if (fractional_part != 0) {
        *output_iterator++ = '.';
        int fractional_digits = get_uint64_length((uint64_t) fractional_part);
        int leading_zeros = FLOAT_PRECISION - fractional_digits;

        // write leading zeros
        for (int i = 0; i < leading_zeros; ++i) {
            *output_iterator++ = '0';
        }
        output_iterator = write_uint64_to_buffer(fractional_part, output_iterator);
        if (output_iterator == NULL) {
            return;
        }
    }
    HAL_USART_Print(&husart0, buffer, USART_TIMEOUT_DEFAULT);
}

char* write_uint64_to_buffer(uint64_t value, char* buffer) {
    size_t value_length = get_uint64_length(value);
    buffer[value_length] = '\0';
    for (int i = value_length - 1; i >= 0; --i) {
        buffer[i] = (value % 10) + '0';
        value /= 10;
    }
    return buffer + value_length;
}

size_t get_uint64_length(uint64_t value) {
    if (value == 0) return 1;
    size_t length = 0;
    while (value) {
        ++length;
        value /= 10;
    }
    return length;
}

uint64_t profile_function(function_to_profile f, char* f_name, uint32_t arg) {
    uint64_t start_mtime, end_mtime;
    uint64_t diff_mtime;
    uint32_t function_result;
    float time_in_seconds;

    // exec function
    start_mtime = __HAL_SCR1_TIMER_GET_TIME();
    function_result = f(arg);
    end_mtime = __HAL_SCR1_TIMER_GET_TIME();

    // calculate time difference
    diff_mtime = end_mtime - start_mtime;
    time_in_seconds = (float)(diff_mtime) / TICKS_PER_SECOND;

    // print function output
    usart_print_uint64(function_result);
    HAL_USART_Print(&husart0, "\r\n", USART_TIMEOUT_DEFAULT);

    // print time in ticks
    usart_print_uint64(diff_mtime);
    HAL_USART_Print(&husart0, "\r\n", USART_TIMEOUT_DEFAULT);

    // print time in seconds
    usart_print_float(time_in_seconds);
    HAL_USART_Print(&husart0, "\r\n\r\n", USART_TIMEOUT_DEFAULT);

    return diff_mtime;
}

void compare_profiling_results(uint64_t res1, uint64_t res2) {
    uint64_t diff_mtime = (res1 > res2) ? res1 - res2 : res2 - res1;
    float time_in_seconds = (float)(diff_mtime) / TICKS_PER_SECOND;

    HAL_USART_Print(&husart0, "Difference in ticks: ", USART_TIMEOUT_DEFAULT);
    usart_print_uint64(diff_mtime);
    HAL_USART_Print(&husart0, "\r\n", USART_TIMEOUT_DEFAULT);

    HAL_USART_Print(&husart0, "Difference in seconds: ", USART_TIMEOUT_DEFAULT);
    usart_print_float(time_in_seconds);
    HAL_USART_Print(&husart0, "\r\n", USART_TIMEOUT_DEFAULT);
}
