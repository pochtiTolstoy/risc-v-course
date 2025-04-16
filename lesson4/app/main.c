#include "my_gpio_config.h"
#include "shifts.h"
#include "stddef.h"

const uint8_t binary_symbols[] = {
  0b00000001,
  0b00000000,
  0b11111100,
  0b01100000,
  0b11011010,
  0b11110010,
  0b01100110,
  0b10110110,
  0b10111110,
  0b11100000,
  0b11111110, 
  0b11110110
};

int main(void) {
  SystemClock_Config();
  GPIO_Init();

  size_t binary_symbol_id = 0;
  size_t binary_symbols_size = sizeof(binary_symbols) / sizeof(binary_symbols[0]);
  while (1) {
    shift_asm(binary_symbols[binary_symbol_id++]);
    if (binary_symbol_id == binary_symbols_size) {
      binary_symbol_id = 0;
    }
    delay(600);
  }
  return 0;
}
