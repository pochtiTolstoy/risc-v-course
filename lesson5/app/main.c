/*
 * Данный пример демонстрирует работу с GPIO и PAD_CONFIG.
 * В примере настраивается вывод, который подключенный к светодиоду, в режим
 * GPIO.
 */

#include "gpio.h"
#include "mik32_memory_map.h"
#include "pad_config.h"
#include "power_manager.h"
#include "scr1_timer.h"
#include "wakeup.h"

#define LED_PIN_NUM (7)
#define LED_PIN_PORT GPIO_2

#define SCR1_TIMER_GET_TIME()                                                  \
  (((uint64_t)(SCR1_TIMER->MTIMEH) << 32) | (SCR1_TIMER->MTIME))
#define SYSTEM_FREQ_HZ 32000000UL
void SystemClock_Config();
void GPIO_Init();

void delay(uint32_t ms) __attribute__((section(".ram_delay"), noinline));

// Constructors
static void setup_global1(void) __attribute__ ((constructor(101)));
static void setup_global2(void) __attribute__ ((constructor(102)));

// Destructors
static void destroy_global1(void) __attribute__ ((destructor(101)));
static void destroy_global2(void) __attribute__ ((destructor(102)));

// Global to hold current timestamp
static volatile uint64_t timestamp = 0;

// Values for tracing and observing initialization
static int global_value_with_init = 42;
static uint32_t global_u32_value_with_init = 0xa1a2a3a4ul;
static uint64_t global_u64_value_with_init = 0xb1b2b3b4b5b6b7b8ull;
static float    global_f32_value_with_init = 3.14;
static double   global_f64_value_with_init = 1.44;
static uint16_t global_u16_value_with_init = 0x1234;
static uint8_t  global_u8a_value_with_init = 0x42;
static uint8_t  global_u8b_value_with_init = 0x43;
static uint8_t  global_u8c_value_with_init = 0x44;
static uint8_t  global_u8d_value_with_init = 0x45;

static int global_value_bss1;
static int global_value_bss2;


static char global_str2[] = "global_str2 bla1 bla2 bla3 1234567890 1234567890 bla1 bla2 bla3 1234567890";
static char global_str3[] = "global_str3 bla1 bla2 bla3 1234567890 1234567890 bla1 bla2 bla3 1234567890";
static char global_str4[] = "global_str4 bla1 bla2 bla3 1234567890 1234567890 bla1 bla2 bla3 1234567890";
static char* global_str5 =  "global_str5 bla1 bla2 bla3 1234567890 1234567890 bla1 bla2 bla3 1234567890";

// Values to observe contructor/destructor changes
static unsigned int global_value1_with_constructor = 1;
static unsigned int global_value2_with_constructor = 2;
 
const int global_const = 228;

void setup_global2(void) {
    global_value1_with_constructor |= 0x200;
    global_value2_with_constructor |= 0x200;
}

void setup_global1(void) {
    global_value1_with_constructor |= 0x100000;
    global_value2_with_constructor |= 0x100000;
}

void destroy_global2(void) {
    global_value1_with_constructor &= ~0x200;
    global_value2_with_constructor &= ~0x200;
}

void destroy_global1(void) {
    global_value1_with_constructor &= ~0x100000;
    global_value2_with_constructor &= ~0x100000;
}

int main() {
  SystemClock_Config();
  GPIO_Init();

   char local_str1[] =
      "local_str1 bla1 bla2 bla3 1234567890 1234567890 bla1 bla2 bla3 "
      "1234567890 1234567890 bla1 bla2 bla3 1234567890 1234567890";
  local_str1[0] = '1';
  global_str2[0] = '2';
  global_str3[0] = '3';

  global_str3[0] = global_str5[11];
  global_str2[1] = local_str1[0];
  global_str3[1] = global_str4[11];
  global_value_bss1 += 1;
  global_value_bss2 += 2;
  
  global_u8c_value_with_init++;
  global_u32_value_with_init = 0xa1a2a3a4ul;
  global_u64_value_with_init++;
  global_f32_value_with_init++;
  global_u8b_value_with_init++;
  global_f64_value_with_init++;
  global_u8d_value_with_init++;
  global_u16_value_with_init++;
  global_u8a_value_with_init++;

  global_str2[sizeof(global_str2) - 2] = '1';

  while (1) {
    // toggle LED_PIN
    global_value_with_init++;
    LED_PIN_PORT->OUTPUT ^= (1 << LED_PIN_NUM);
    delay(100);
  }
}

void SystemClock_Config(void) {
  WU->CLOCKS_SYS &=
      ~(0b11 << WU_CLOCKS_SYS_OSC32M_EN_S); // Включить OSC32M и HSI32M
  WU->CLOCKS_BU &=
      ~(0b11 << WU_CLOCKS_BU_OSC32K_EN_S); // Включить OSC32K и LSI32K

  // Поправочный коэффициент HSI32M
  WU->CLOCKS_SYS = (WU->CLOCKS_SYS & (~WU_CLOCKS_SYS_ADJ_HSI32M_M)) |
                   WU_CLOCKS_SYS_ADJ_HSI32M(128);
  // Поправочный коэффициент LSI32K
  WU->CLOCKS_BU = (WU->CLOCKS_BU & (~WU_CLOCKS_BU_ADJ_LSI32K_M)) |
                  WU_CLOCKS_BU_ADJ_LSI32K(8);

  // Автоматический выбор источника опорного тактирования
  WU->CLOCKS_SYS &= ~WU_CLOCKS_SYS_FORCE_32K_CLK_M;

  // ожидание готовности
  while (!(PM->FREQ_STATUS & PM_FREQ_STATUS_OSC32M_M))
    ;

  // переключение на тактирование от OSC32M
  PM->AHB_CLK_MUX = PM_AHB_CLK_MUX_OSC32M_M | PM_AHB_FORCE_MUX_UNFIXED;
  PM->DIV_AHB = 0;   // Задать делитель шины AHB.
  PM->DIV_APB_M = 0; // Задать делитель шины APB_M.
  PM->DIV_APB_P = 0; // Задать делитель шины APB_P.
}

void GPIO_Init() {

  /**< Включить  тактирование GPIO_0 */
  PM->CLK_APB_P_SET = PM_CLOCK_APB_P_GPIO_0_M;

  /**< Включить  тактирование GPIO_1 */
  PM->CLK_APB_P_SET = PM_CLOCK_APB_P_GPIO_1_M;

  /**< Включить  тактирование GPIO_2 */
  PM->CLK_APB_P_SET = PM_CLOCK_APB_P_GPIO_2_M;

  /**< Включить  тактирование схемы формирования прерываний GPIO */
  PM->CLK_APB_P_SET = PM_CLOCK_APB_P_GPIO_IRQ_M;

  // первая функция (порт общего назначения);
  PAD_CONFIG->PORT_0_CFG |= 0 << (LED_PIN_NUM * 2);

  // нагрузочная способность 2 мА;
  PAD_CONFIG->PORT_0_DS |= 0 << (LED_PIN_NUM * 2);

  // резисторы подтяжки отключены
  PAD_CONFIG->PORT_0_PUPD |= 0 << (LED_PIN_NUM * 2);

  // Установка направления выводов как выход.
  GPIO_2->DIRECTION_OUT = 1 << LED_PIN_NUM;
}

__attribute__((section(".ram_delay"), noinline))
void delay(uint32_t ms) {
  uint64_t end_mtimer = SCR1_TIMER_GET_TIME() + ms * (SYSTEM_FREQ_HZ / 1000);
  while (SCR1_TIMER_GET_TIME() < end_mtimer)
    ;
}