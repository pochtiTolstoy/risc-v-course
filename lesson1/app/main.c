#include "mik32_hal_pcc.h"
#include "mik32_hal_gpio.h"
#include <stddef.h>

/*
 * Данный пример демонстрирует работу с GPIO и PAD_CONFIG.
 * В примере настраивается вывод, который подключенный к светодиоду, в режим GPIO.
 */

#define DASH_LENGTH 300
#define DOT_LENGTH 100
#define ZERO_LENGTH 0
#define UNIT_PAUSE 50 /* pause between dots and dashes within one letter */
#define LETTERS_PAUSE 100
#define MESSAGE_PAUSE 500
#define SPACE_PAUSE 200

#define ALPH_SIZE 27
#define DASH_CODE  '-'
#define DOT_CODE   '.'
#define SPACE_CODE ' '
#define SPACE_ID (ALPH_SIZE - 1)

void SystemClock_Config();
void GPIO_Init();
void display_message();
void display_letter(char);
void blink(int, int);
const char* get_morse_code_from_letter(char);

const char* message = "YADRO RISCV";
const char* itr;
const char* morse_codes[ALPH_SIZE] = {
    ".-",   /* A */
    "-...", /* B */
    "-.-.", /* C */
    "-..",  /* D */
    ".",    /* E */
    "..-.", /* F */
    "--.",  /* G */
    "....", /* H */
    "..",   /* I */
    ".---", /* J */
    "-.-",  /* K */
    ".-..", /* L */
    "--",   /* M */
    "-.",   /* N */
    "---",  /* O */
    ".--.", /* P */
    "--.-", /* Q */
    ".-.",  /* R */
    "...",  /* S */
    "-",    /* T */
    "..-",  /* U */
    "...-", /* V */
    ".--",  /* W */
    "-..-", /* X */
    "-.--", /* Y */
    "--..", /* Z */
    [SPACE_ID] = " "
};

int main()
{
    SystemClock_Config();
    GPIO_Init();

    while (1) {
        display_message();
    }
}

void display_message(void) 
{
    for (itr = message; *itr; ++itr) {
        display_letter(*itr); 
        blink(ZERO_LENGTH, LETTERS_PAUSE);
    }
    blink(ZERO_LENGTH, MESSAGE_PAUSE);
}

void display_letter(char letter) 
{
    const char* morse_code = get_morse_code_from_letter(letter);
    if (!morse_code) return;

    while (*morse_code) {
        switch (*morse_code) {
            case DOT_CODE: 
                blink(DOT_LENGTH, UNIT_PAUSE);
                break;
            case DASH_CODE: 
                blink(DASH_LENGTH, UNIT_PAUSE);
                break;
            case SPACE_CODE:
                blink(ZERO_LENGTH, SPACE_PAUSE);
                break;
        }
        ++morse_code;
    }
}

const char* get_morse_code_from_letter(char letter) 
{
    if (letter == SPACE_CODE) {
        return morse_codes[SPACE_ID];
    } else if (letter >= 'A' && letter <= 'Z') {
        return morse_codes[letter - 'A'];
    }
    return NULL;
}

void blink(int on_time, int off_time) 
{
    if (on_time) {
        HAL_GPIO_WritePin(GPIO_2, GPIO_PIN_7, GPIO_PIN_HIGH);
        HAL_DelayMs(on_time);
    }

    if (off_time) {
        HAL_GPIO_WritePin(GPIO_2, GPIO_PIN_7, GPIO_PIN_LOW);
        HAL_DelayMs(off_time);
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

void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_PCC_GPIO_0_CLK_ENABLE();
    __HAL_PCC_GPIO_1_CLK_ENABLE();
    __HAL_PCC_GPIO_2_CLK_ENABLE();
    __HAL_PCC_GPIO_IRQ_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_NONE;
    HAL_GPIO_Init(GPIO_2, &GPIO_InitStruct);
}
