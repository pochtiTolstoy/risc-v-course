#include "epic.h"
#include "gpio.h"
#include "mik32_memory_map.h"
#include "pad_config.h"
#include "power_manager.h"
#include "riscv-irq.h"
#include "scr1_timer.h"
#include "timer32.h"
#include "wakeup.h"

#define SYSTEM_FREQ_HZ 32000000UL

#define PWM_PIN_NUM_3 (0)
#define PWM_PIN_PORT_3 GPIO_0

#define PWM_PIN_NUM_5 (1)
#define PWM_PIN_PORT_5 GPIO_0

#define BTM_PIN_NUM_2 (10)
#define BTM_PIN_PORT_2 GPIO_0

#define BTM_PIN_NUM_1 (8)
#define BTM_PIN_PORT_1 GPIO_0

#define PWM_FREQ_HZ (1000)
#define PWM_PERIOD_TICKS (SYSTEM_FREQ_HZ / PWM_FREQ_HZ) // 32000

#define NUM_WAVES 10 // from 1 to 5 with step 0.5

#define FREQ_SCALE 2 // to avoid floating point

#define BUTTON_COOLDOWN 50000

#define SET_TWO_BIT(REG, NUM, TWO_BITS) \
  do { \
    (REG) = (((REG) & ~(PAD_CONFIG_PIN_M(NUM))) | (PAD_CONFIG_PIN(NUM, TWO_BITS))); \
  } while(0)

#define SCR1_TIMER_GET_TIME()                                                  \
  (((uint64_t)(SCR1_TIMER->MTIMEH) << 32) | (SCR1_TIMER->MTIME))

typedef struct led_t {
  volatile int32_t current_percent;
  volatile int32_t step;
  volatile int32_t frequency;
} led_t;

typedef struct {
  uint32_t pin_num;
  GPIO_TypeDef* pin_port;
  uint32_t curr_state;
  uint32_t prev_state;
  uint32_t pressed;
  uint32_t cooldown;
} button_t;

led_t led_1 = { 
  .current_percent = 0, 
  .step = 1 * FREQ_SCALE, 
  .frequency = 1 * FREQ_SCALE
};

led_t led_2 = { 
  .current_percent = 0, 
  .step = 2 * FREQ_SCALE, 
  .frequency = 2 * FREQ_SCALE
};

button_t button_1 = {
  .pin_num = BTM_PIN_NUM_2, 
  .pin_port = BTM_PIN_PORT_2, 
  .cooldown = BUTTON_COOLDOWN 
};

button_t button_2 = {
  .pin_num = BTM_PIN_NUM_1, 
  .pin_port = BTM_PIN_PORT_1, 
  .cooldown = BUTTON_COOLDOWN 
};

led_t waves_data[NUM_WAVES];

void SystemClock_Config();
void GPIO_Init();
void TMR_Init();
void TMR_PWM_Init();
void delay(uint32_t ms);
void EPIC_trap_handler();
void TMR_PWM_Init();
void update_button(button_t* button);
uint32_t read_button(button_t* button);
void init_waves();
void update_waves();
void update_led_freq(led_t* led);

int main() {
  SystemClock_Config();
  GPIO_Init();
  TMR_Init();
  TMR_PWM_Init();
  init_waves();

  // Включение тактирования EPIC
  PM->CLK_APB_M_SET = PM_CLOCK_APB_M_EPIC_M;
  // Включение прерываний от TIMER32_0
  EPIC->MASK_LEVEL_SET = 1 << (EPIC_LINE_TIMER32_0_S);

  riscv_irq_set_handler(RISCV_IRQ_MEI, EPIC_trap_handler);
  riscv_irq_enable(RISCV_IRQ_MEI);
  riscv_irq_global_enable();
  while (1) {
    update_button(&button_1);
    if (button_1.pressed) {
      update_led_freq(&led_1);
    }
    update_button(&button_2);
    if (button_2.pressed) {
      update_led_freq(&led_2);
    }
  }
}

void update_led_brightness(led_t* led) {
  led->current_percent += led->step;
  if (led->current_percent >= 100 * FREQ_SCALE || led->current_percent <= 0) {
    led->step = -led->step;
    if (led->current_percent < 0) {
      led->current_percent = -led->current_percent;
    }
    if (led->current_percent > 100 * FREQ_SCALE) {
      led->current_percent = (200 * FREQ_SCALE) - led->current_percent;
    }
  }
}

void EPIC_trap_handler() {
  if (EPIC->RAW_STATUS & (1 << EPIC_LINE_TIMER32_0_S)) {
    update_led_brightness(&led_1);
    update_led_brightness(&led_2);
    update_waves();

    // update ocr
    TIMER32_1->CHANNELS[0].OCR = (led_1.current_percent * PWM_PERIOD_TICKS) / (100 * FREQ_SCALE);
    TIMER32_1->CHANNELS[1].OCR = (led_2.current_percent * PWM_PERIOD_TICKS) / (100 * FREQ_SCALE);

    // clear int
    TIMER32_0->INT_CLEAR = TIMER32_INT_OVERFLOW_M;
    EPIC->CLEAR = EPIC_LINE_TIMER32_0_S;
  }
}

void update_button(button_t* button) {
  button->pressed = 0;
  if (button->cooldown != 0) {
    --button->cooldown;
    return;
  }
  button->curr_state = read_button(button);
  if (button->curr_state != button->prev_state) {
    button->prev_state = button->curr_state;
    if (button->curr_state) {
      button->pressed = 1;
      button->cooldown = BUTTON_COOLDOWN;
    }
  }
}

uint32_t read_button(button_t* button) { 
  return (button->pin_port->STATE) & (1 << button->pin_num); 
}

void update_led_freq(led_t* led) {
  uint8_t new_wave_ind = led->frequency;
  if (new_wave_ind >= NUM_WAVES) {
    new_wave_ind = 0;
  }
  *led = waves_data[new_wave_ind];
}

void init_waves() {
  for (int i = 0; i < NUM_WAVES; ++i) {
    waves_data[i] = (led_t) { 
        .current_percent = 0, 
        .step = i + 1, 
        .frequency = i + 1 
    };
  }
}

void update_waves() {
  for (int i = 0; i < NUM_WAVES; ++i) {
    update_led_brightness(&waves_data[i]);
  }
}

void TMR_PWM_Init() {
  // Подключене пина к TIMER32_1_CH1
  PAD_CONFIG->PORT_0_CFG |= 2 << (PWM_PIN_NUM_3 * 2);

  // подключение пина 5 к TIMER32_1_CH2
  PAD_CONFIG->PORT_0_CFG |= 2 << (PWM_PIN_NUM_5 * 2);

  // Включение тактирования TIMER32_1
  PM->CLK_APB_P_SET = PM_CLOCK_APB_P_TIMER32_1_M;
  TIMER32_1->ENABLE = 0;
  TIMER32_1->TOP = PWM_PERIOD_TICKS;
  TIMER32_1->PRESCALER = 0;
  TIMER32_1->CONTROL =
      TIMER32_CONTROL_MODE_UP_M | TIMER32_CONTROL_CLOCK_PRESCALER_M;
  TIMER32_1->INT_MASK = 0;
  TIMER32_1->INT_CLEAR = 0xFFFFFFFF;

  TIMER32_1->CHANNELS[0].OCR = 0;
  TIMER32_1->CHANNELS[0].CNTRL =
      TIMER32_CH_CNTRL_MODE_PWM_M | TIMER32_CH_CNTRL_ENABLE_M;
  TIMER32_1->CHANNELS[1].OCR = 0;
  TIMER32_1->CHANNELS[1].CNTRL =
      TIMER32_CH_CNTRL_MODE_PWM_M | TIMER32_CH_CNTRL_ENABLE_M;
  TIMER32_1->ENABLE = 1;
}

void TMR_Init() {
  // Включение тактирования TIMER32_0
  PM->CLK_APB_M_SET = PM_CLOCK_APB_M_TIMER32_0_M;
  TIMER32_0->ENABLE = 0;
  TIMER32_0->TOP = PWM_PERIOD_TICKS * 5; 
  TIMER32_0->PRESCALER = 0;
  TIMER32_0->CONTROL =
      TIMER32_CONTROL_MODE_UP_M | TIMER32_CONTROL_CLOCK_PRESCALER_M;
  TIMER32_0->INT_MASK = 0;
  TIMER32_0->INT_CLEAR = 0xFFFFFFFF;
  TIMER32_0->ENABLE = 1;
  // Включение прерывания по переполнению
  TIMER32_0->INT_MASK = TIMER32_INT_OVERFLOW_M;
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

  // Button
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, button_1.pin_num, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, button_1.pin_num, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, button_1.pin_num, 0); // Резисторы подтяжки отключены
  button_1.pin_port->DIRECTION_IN = 1 << button_1.pin_num;

  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, button_2.pin_num, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, button_2.pin_num, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, button_2.pin_num, 0); // Резисторы подтяжки отключены
  button_2.pin_port->DIRECTION_IN = 1 << button_2.pin_num;
}

void delay(uint32_t ms) {
  uint64_t end_mtimer = SCR1_TIMER_GET_TIME() + ms * (SYSTEM_FREQ_HZ / 1000);
  while (SCR1_TIMER_GET_TIME() < end_mtimer)
    ;
}