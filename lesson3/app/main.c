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

#define GREEN_LED_PIN_NUM (0)
#define GREEN_LED_PIN_PORT GPIO_0

#define RED_LED_PIN_NUM (8)
#define RED_LED_PIN_PORT GPIO_0

#define BTM_PIN_NUM_1 (6)
#define BTM_PIN_PORT_1 GPIO_0

#define BTM_PIN_NUM_2 (10)
#define BTM_PIN_PORT_2 GPIO_0


#define SCR1_TIMER_GET_TIME()                                                  \
  (((uint64_t)(SCR1_TIMER->MTIMEH) << 32) | (SCR1_TIMER->MTIME))
#define SYSTEM_FREQ_HZ 32000000UL

#define SET_TWO_BIT(REG, NUM, TWO_BITS) \
  do { \
    (REG) = (((REG) & ~(PAD_CONFIG_PIN_M(NUM))) | (PAD_CONFIG_PIN(NUM, TWO_BITS))); \
  } while(0)

#define LED_STATES_NUM 2

typedef enum {
  TURN_ON = 0,
  TURN_OFF
} LED_STATES;

typedef struct {
  uint32_t pin_num;  
  GPIO_TypeDef* pin_port;
  uint32_t on;
  LED_STATES state;
} led_t;

typedef struct {
  uint32_t pin_num;
  GPIO_TypeDef* pin_port;
  uint32_t curr_state;
  uint32_t prev_state;
  uint32_t pressed;
} button_t;

void SystemClock_Config();
void GPIO_Init();
void delay(uint32_t ms);

void update_button(button_t* button);
uint32_t read_button(button_t*);
void update_led_state(led_t*);
void turn_leds(led_t*, led_t*);
uint32_t is_led_on(const led_t*);
void turn_led_on(led_t*);
void turn_led_off(led_t*);
void blink_led(led_t*);

int main() {
  SystemClock_Config();
  GPIO_Init();
  led_t green_led = {.pin_num = GREEN_LED_PIN_NUM, .pin_port = GREEN_LED_PIN_PORT, .state = TURN_OFF};
  led_t red_led   = {.pin_num = RED_LED_PIN_NUM, .pin_port = RED_LED_PIN_PORT, .state = TURN_OFF};
  button_t button_1 = {.pin_num = BTM_PIN_NUM_1, .pin_port = BTM_PIN_PORT_1, .pressed = 0};
  button_t button_2 = {.pin_num = BTM_PIN_NUM_2, .pin_port = BTM_PIN_PORT_2, .pressed = 0};

  while (1) {
    update_button(&button_1);
    update_button(&button_2);
    if (button_1.pressed) update_led_state(&red_led);
    if (button_2.pressed) update_led_state(&green_led);
    turn_leds(&green_led, &red_led);
    delay(100);
  }
}

void update_button(button_t* button) {
  button->pressed = 0;
  button->curr_state = read_button(button);
  if (button->curr_state != button->prev_state) {
    button->prev_state = button->curr_state;
    if (button->curr_state) button->pressed = 1;
  }
}

uint32_t read_button(button_t* button) { 
  return (button->pin_port->STATE) & (1 << button->pin_num); 
}

void update_led_state(led_t* led) { 
  led->state = (led->state + 1) % LED_STATES_NUM; 
}

void turn_leds(led_t* led_1, led_t* led_2) {
  switch (led_1->state) {
    case TURN_ON:
      turn_led_on(led_1);
      break;
    case TURN_OFF:
      turn_led_off(led_1);
      break;
  }
  switch (led_2->state) {
    case TURN_ON:
      turn_led_on(led_2);
      break;
    case TURN_OFF:
      turn_led_off(led_2);
      break;
  }
}

uint32_t is_led_on(const led_t* led) {
  return ((led->pin_port->OUTPUT) & (1 << led->pin_num));
}

void turn_led_on(led_t* led) {
  led->pin_port->OUTPUT |= (1 << led->pin_num);
}

void turn_led_off(led_t* led) {
  led->pin_port->OUTPUT &= ~(1 << led->pin_num);
}

void blink_led(led_t* led) {
  led->pin_port->OUTPUT ^= (1 << led->pin_num);
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

 /* 
 В документации на PAD указано, что для конфигурации одного порта необходимо
 записать два бита данных. Воспользуемся принципом Read-Modify-Write для этого:
 0. Задаем номер порта NUM.
 1. Определяем положение двух битов и формируем маску c использованием макроса  из pad_config.h
   PAD_CONFIG_PIN_M(NUM)

 2. READ. Читаем значение регистра и обнуляем биты, которые хотим модифицировать    
   (REG & ~PAD_CONFIG_PIN_M(NUM))
 
 3. MODIFY. Задаем новое значение всего регистра с использованием макроса PAD_CONFIG_PIN из pad_config.h
   (REG & ~PAD_CONFIG_PIN_M(NUM)) |  PAD_CONFIG_PIN(NUM, TWO_BITS)
 
 4. WRITE. Записываем изменные данные в регистр аппаратуры. В итоге получаем:  
   REG = (REG & ~PAD_CONFIG_PIN_M(NUM)) | PAD_CONFIG_PIN(NUM, TWO_BITS)

 Попробуйте реализовать макрос 
 SET_TWO_BIT для корректной записи двух битов в регистре и модифицировать код конфигурации PAD, который представлен ниже.
 */

  // Green led
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, GREEN_LED_PIN_NUM, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, GREEN_LED_PIN_NUM, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, GREEN_LED_PIN_NUM, 0); // Резисторы подтяжки отключены
  GREEN_LED_PIN_PORT->DIRECTION_OUT = 1 << GREEN_LED_PIN_NUM;


  // RED led
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, RED_LED_PIN_NUM, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, RED_LED_PIN_NUM, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, RED_LED_PIN_NUM, 0); // Резисторы подтяжки отключены
  RED_LED_PIN_PORT->DIRECTION_OUT = 1 << RED_LED_PIN_NUM;

  // Button
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, BTM_PIN_NUM_1, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, BTM_PIN_NUM_1, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, BTM_PIN_NUM_1, 0); // Резисторы подтяжки отключены
  BTM_PIN_PORT_1->DIRECTION_IN = 1 << BTM_PIN_NUM_1;

  // Button
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, BTM_PIN_NUM_2, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, BTM_PIN_NUM_2, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, BTM_PIN_NUM_2, 0); // Резисторы подтяжки отключены
  BTM_PIN_PORT_2->DIRECTION_IN = 1 << BTM_PIN_NUM_2;
}

void delay(uint32_t ms) {
  uint64_t end_mtimer = SCR1_TIMER_GET_TIME() + ms * (SYSTEM_FREQ_HZ / 1000);
  while (SCR1_TIMER_GET_TIME() < end_mtimer)
    ;
}
