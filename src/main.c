#include "stm8s.h"
#include <stdio.h>

#include "milis.h"
#include "keypad.h"
#include "delay.h"
#include "uart1.h"

#define TI1_PORT GPIOD
#define TI1_PIN  GPIO_PIN_4

#define TRGG_PORT GPIOC
#define TRGG_PIN  GPIO_PIN_7
#define TRGG_ON   GPIO_WriteHigh(TRGG_PORT, TRGG_PIN);
#define TRGG_OFF  GPIO_WriteLow(TRGG_PORT, TRGG_PIN);
#define TRGG_REVERSE GPIO_WriteReverse(TRGG_PORT, TRGG_PIN);

#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_5
#define LED_ON   GPIO_WriteHigh(LED_PORT, LED_PIN);
#define LED_OFF  GPIO_WriteLow(LED_PORT, LED_PIN);
#define LED_REVERSE GPIO_WriteReverse(LED_PORT, LED_PIN);

#define MASURMENT_PERON 444    // maximální celkový čas měření (ms)


/*----------------------------------------------------------------------------------------------*/



void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovat MCU na 16MHz

    init_milis();
    /*init_keypad();*/
    init_uart1();

    GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    /*----          trigger setup           ---------*/
    GPIO_Init(TRGG_PORT, TRGG_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);

    /*----           TIM2 setup           ---------*/
    GPIO_Init(TI1_PORT, TI1_PIN, GPIO_MODE_IN_FL_NO_IT);  // kanál 1 jako vstup

    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 0xFFFF );
    /*TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);*/
    TIM2_Cmd(ENABLE);
    TIM2_ICInit(TIM2_CHANNEL_1,        // nastavuji CH1 (CaptureRegistr1)
            TIM2_ICPOLARITY_RISING,    // náběžná hrana
            TIM2_ICSELECTION_DIRECTTI, // CaptureRegistr1 bude ovládán z CH1
            TIM2_ICPSC_DIV1,           // dělička je vypnutá
            0                          // vstupní filter je vypnutý
        );            
    TIM2_ICInit(TIM2_CHANNEL_2,        // nastavuji CH2 (CaptureRegistr2)
            TIM2_ICPOLARITY_FALLING,   // sestupná hrana
            TIM2_ICSELECTION_INDIRECTTI, // CaptureRegistr2 bude ovládán z CH1
            TIM2_ICPSC_DIV1,           // dělička je vypnutá
            0                          // vstupní filter je vypnutý
        );            

}


/*- https://www.tutorialspoint.com/enum-in-c  -*/
typedef enum
{
    TRGG_START,       // zahájení trigger impoulzu
    TRGG_WAIT,        // čekání na konec trrigger impoulzu
    MEASURMENT_WAIT   // čkání na dokončení měření
} STATE_TypeDef;

int main(void)
{
    uint32_t mtime_led = 0;
    /*uint8_t key_now = 0xFF;*/
    /*uint8_t key_last = 0xFF;*/
    /*uint32_t mtime_key = 0;*/
    uint32_t mtime_ultrasonic = 0;
    uint32_t diff;
    STATE_TypeDef state = TRGG_START;

    setup();
    printf("Start\n");

    while (1) {

        switch (state) {
        case TRGG_START:
            if (milis() - mtime_ultrasonic > MASURMENT_PERON) {
                mtime_ultrasonic = milis();
                TRGG_ON;
                state = TRGG_WAIT;
            }
            break;
        case TRGG_WAIT:
            if (milis() - mtime_ultrasonic > 1) {
                TRGG_OFF;
                // smažu všechny vlajky
                TIM2_ClearFlag(TIM2_FLAG_CC1);
                TIM2_ClearFlag(TIM2_FLAG_CC2); 
                TIM2_ClearFlag(TIM2_FLAG_CC1OF); 
                TIM2_ClearFlag(TIM2_FLAG_CC2OF); 
                state = MEASURMENT_WAIT;
            }
            break;
        case MEASURMENT_WAIT:
             /* detekuji sestupnou hranu ECHO signálu; vzestupnou hranu 
              * detekovat nemusím, zachycení CC1 i CC2 proběhne automaticky  */
            if (TIM2_GetFlagStatus(TIM2_FLAG_CC2) == RESET) {
                TIM2_ClearFlag(TIM2_FLAG_CC1);  // smažu vlajku CC1
                TIM2_ClearFlag(TIM2_FLAG_CC2);  // smažu vlajku CC2

                // délka impulzu v µs 
                diff = (TIM2_GetCapture2() - TIM2_GetCapture1()); 

                printf("%lu us\n", diff);
                //diff = (diff * 340 + 10000)/ 20000; // FixPoint přepočet na cm -- korektně zaokrouhluje 
                diff = (diff * 340)/ 20000; // FixPoint přepočet na cm -- zaokrouhluje vždy dolů
                printf("%lu cm\n\n", diff);

                state = TRGG_START;
            }
            break;
        default:
            state = TRGG_START;
        }
        // pracuji neblokující způsobem, a kromě měření můžu dělat i něco jiného
        if (milis() - mtime_led > 333) {
            mtime_led = milis();
            LED_REVERSE;
        }
        // klávesnice
        /*if (milis() - mtime_key > 55) {*/
        /*    mtime_key = milis();*/
        /*    key_now = check_keypad();*/
        /*    if (  key_last == 0xFF && key_now != 0xFF) {*/
        /*        printf("key: %u\n", key_now);*/
        /*    }*/
        /*    key_last = key_now;*/
        /*}*/
    }
}



/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
