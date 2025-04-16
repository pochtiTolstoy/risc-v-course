#include "mik32_hal_adc.h"
#include "mik32_hal_usart.h"
#include "mik32_hal_scr1_timer.h"
#include "xprintf.h"

#define ADC_CHANNEL_TOTAL                6U
#define ADC_CONVERSIONS_PER_CHANNEL      2U
#define ADC_RAW_VALUE_MAX                4095U
#define ADC_REF_VOLTAGE_MV               1200U /* mV */
#define MILLIVOLTS_PER_VOLT              1000U
#define SET_TWO_BIT(REG, NUM, TWO_BITS) \
  do { \
    (REG) = (((REG) & ~(PAD_CONFIG_PIN_M(NUM))) | (PAD_CONFIG_PIN(NUM, TWO_BITS))); \
  } while(0)
#define BUZZER_PIN_NUM (0)
#define BUZZER_PIN_PORT GPIO_0
#define LOOP_DELAY_MS 100
#define RISING_TICKS 200
#define LOWER_BOUND 2300
#define UPPER_BOUND 3600
#define TURN_OFF_LOWER_BOUND 3100
#define SMALL_DELTA 50
#define DROP_DELTA 100
#define TURN_OFF_DROP_DELTA 300

typedef enum {
    IDLE,
    RISING,
    ALARM_ON
} alarm_state;

static alarm_state state = IDLE;
static uint32_t rising_counter = 0U;
static uint16_t last_adc = 0U;
static uint16_t adc_value = 0U;
static USART_HandleTypeDef husart0;
ADC_HandleTypeDef hadc;

static void SystemClock_Config();
static void USART_Init();
static void ADC_Init(void);
void GPIO_Init();
static void handle_idle_state();
static void handle_rising_state();
static void handle_alarm_on_state();

int main()
{
    uint32_t integer_part = 0U;
    uint32_t fractional_part = 0U;

    SystemClock_Config();
    GPIO_Init();

    USART_Init();
    ADC_Init();

    while (1)
    {
        for (uint32_t conversion = 0U; conversion < ADC_CONVERSIONS_PER_CHANNEL; conversion++) {
            HAL_ADC_SINGLE_AND_SET_CH(hadc.Instance, ADC_CHANNEL0);
            adc_value = HAL_ADC_WaitAndGetValue(&hadc);
        }
        integer_part = adc_value * ADC_REF_VOLTAGE_MV / ADC_RAW_VALUE_MAX / MILLIVOLTS_PER_VOLT;
        fractional_part = adc_value * ADC_REF_VOLTAGE_MV / ADC_RAW_VALUE_MAX % MILLIVOLTS_PER_VOLT;
        xprintf("ADC[%d]: %04d/%d (%d,%03d V)\r\n", ADC_CHANNEL0, adc_value, ADC_RAW_VALUE_MAX, integer_part, fractional_part);
        switch (state) {
            case IDLE:
                handle_idle_state();
                break;
            case RISING:
                handle_rising_state();
                break;
            case ALARM_ON:
                handle_alarm_on_state();
                break;
        }
        last_adc = adc_value;
        HAL_DelayMs(LOOP_DELAY_MS);
    }
}

static inline void buzzer_on(void) {
    BUZZER_PIN_PORT->OUTPUT |= (1 << BUZZER_PIN_NUM);
}

static inline void buzzer_off(void) {
    BUZZER_PIN_PORT->OUTPUT &= ~(1 << BUZZER_PIN_NUM);
}

static void handle_idle_state()
{
    rising_counter = 0;
    if (adc_value > LOWER_BOUND && adc_value < LOWER_BOUND + DROP_DELTA) {
        xprintf("START RISING\r\n");
        state = RISING;
    }
}

static void handle_rising_state()
{
    if (adc_value <= last_adc - DROP_DELTA || adc_value >= last_adc + DROP_DELTA || adc_value <= LOWER_BOUND) {
        xprintf("STOP RISING\r\n");
        state = IDLE;
    } else {
        ++rising_counter;
        if (rising_counter == RISING_TICKS) {
            if (adc_value > UPPER_BOUND) {
                state = ALARM_ON;
                xprintf("ALARM_ON: trend linear\r\n");
                buzzer_on();
            } else {
                state = IDLE;
                xprintf("IDLE: trend non-linear\r\n");
            }
        }
    }
}

static void handle_alarm_on_state()
{
    if (adc_value + TURN_OFF_DROP_DELTA < last_adc && adc_value <= TURN_OFF_LOWER_BOUND) {
        xprintf("ALARM_OFF: press\r\n");
        buzzer_off();
        state = IDLE;
    }
}

void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;
    HAL_PCC_Config(&PCC_OscInit);
}

static void ADC_Init(void) {
    hadc.Instance = ANALOG_REG;
    hadc.Init.EXTRef = ADC_EXTREF_OFF;
    hadc.Init.EXTClb = ADC_EXTCLB_CLBREF;

    HAL_ADC_Init(&hadc);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    __HAL_PCC_ANALOG_REGS_CLK_ENABLE();

    GPIO_InitStruct.Mode = HAL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIO_1, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_7 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);
}

void USART_Init()
{
    husart0.Instance = UART_0;
    husart0.transmitting = Enable;
    husart0.receiving = Enable;
    husart0.frame = Frame_8bit;
    husart0.parity_bit = Disable;
    husart0.parity_bit_inversion = Disable;
    husart0.bit_direction = LSB_First;
    husart0.data_inversion = Disable;
    husart0.tx_inversion = Disable;
    husart0.rx_inversion = Disable;
    husart0.swap = Disable;
    husart0.lbm = Disable;
    husart0.stop_bit = StopBit_1;
    husart0.mode = Asynchronous_Mode;
    husart0.xck_mode = XCK_Mode3;
    husart0.last_byte_clock = Disable;
    husart0.overwrite = Disable;
    husart0.rts_mode = AlwaysEnable_mode;
    husart0.dma_tx_request = Disable;
    husart0.dma_rx_request = Disable;
    husart0.channel_mode = Duplex_Mode;
    husart0.tx_break_mode = Disable;
    husart0.Interrupt.ctsie = Disable;
    husart0.Interrupt.eie = Disable;
    husart0.Interrupt.idleie = Disable;
    husart0.Interrupt.lbdie = Disable;
    husart0.Interrupt.peie = Disable;
    husart0.Interrupt.rxneie = Disable;
    husart0.Interrupt.tcie = Disable;
    husart0.Interrupt.txeie = Disable;
    husart0.Modem.rts = Disable; //out
    husart0.Modem.cts = Disable; //in
    husart0.Modem.dtr = Disable; //out
    husart0.Modem.dcd = Disable; //in
    husart0.Modem.dsr = Disable; //in
    husart0.Modem.ri = Disable;  //in
    husart0.Modem.ddis = Disable;//out
    husart0.baudrate = 115200;
    HAL_USART_Init(&husart0);
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

  // Green led
  SET_TWO_BIT(PAD_CONFIG->PORT_0_CFG, BUZZER_PIN_NUM, 0);  // Порт общего назначения
  SET_TWO_BIT(PAD_CONFIG->PORT_0_DS, BUZZER_PIN_NUM, 0);   // Нагрузочная способность 2 мА
  SET_TWO_BIT(PAD_CONFIG->PORT_0_PUPD, BUZZER_PIN_NUM, 0); // Резисторы подтяжки отключены
  BUZZER_PIN_PORT->DIRECTION_OUT = 1 << BUZZER_PIN_NUM;
}