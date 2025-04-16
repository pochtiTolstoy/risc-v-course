#ifndef MY_GPIO_CONFIG_H
#define MY_GPIO_CONFIG_H

#include <stdint.h>
#include "gpio.h"
#include "mik32_memory_map.h"
#include "pad_config.h"
#include "power_manager.h"
#include "scr1_timer.h"
#include "wakeup.h"

#define SCR1_TIMER_GET_TIME()                                                  \
  (((uint64_t)(SCR1_TIMER->MTIMEH) << 32) | (SCR1_TIMER->MTIME))
#define SYSTEM_FREQ_HZ 32000000UL

#define SET_TWO_BIT(REG, NUM, TWO_BITS) (REG = (REG & ~PAD_CONFIG_PIN_M(NUM)) | PAD_CONFIG_PIN(NUM, TWO_BITS))

#define DATA_PIN (8) 
#define DATA_PIN_PORT GPIO_0

#define STCP_PIN (0) // STORE
#define STCP_PIN_PORT GPIO_0 

#define SHCP_PIN (10) //SHIFT
#define SHCP_PIN_PORT GPIO_0 

#define GPIO0_SET ((volatile uint32_t* )(GPIO_0_BASE_ADDRESS + GPIO_SET))
#define GPIO0_CLR ((volatile uint32_t* )(GPIO_0_BASE_ADDRESS + GPIO_CLEAR))

void SystemClock_Config();
void GPIO_Init();
void delay(uint32_t ms);

#endif /* MY_GPIO_CONFIG_H */