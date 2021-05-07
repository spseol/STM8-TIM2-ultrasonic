#include "stm8s.h"
#include <stdio.h>

#include "milis.h"
#include "keypad.h"
#include "delay.h"
#include "uart1.h"

#define TI1_PORT GPIOD
#define TI1_PIN  GPIO_PIN_4

#define TRGG_PORT GPIOC
#define TRGG_PIN  GPIO_PIN_5
#define TRGG_ON   GPIO_WriteHigh(TRGG_PORT, TRGG_PIN);
#define TRGG_OFF  GPIO_WriteLow(TRGG_PORT, TRGG_PIN);
#define TRGG_REVERSE GPIO_WriteReverse(TRGG_PORT, TRGG_PIN);


/*----------------------------------------------------------------------------------------------*/



void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);      // taktovat MCU na 16MHz

    init_milis();
    init_keypad();
    init_uart1();

    /*----          trigger setup           ---------*/
    GPIO_Init(TRGG_PORT, TRGG_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);

    /*----           TIM2 setup           ---------*/
    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 0xFFFF );
    /*TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);*/
    TIM2_Cmd(ENABLE);
    GPIO_Init(TI1_PORT, TI1_PIN, GPIO_MODE_IN_FL_NO_IT);  // kanál 1 jako vstup
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

int main(void)
{
    uint16_t time_start = 0;
    uint16_t time_end = 0;
    uint32_t diff;

    setup();
    printf("Start\n");

    while (1) {

        // trigger impulz pro spuštění měření
        TRGG_ON;
        _delay_us(50);
        TRGG_OFF;

        while(TIM2_GetFlagStatus(TIM2_FLAG_CC1) == RESET);  // čekám na náběžnou hranu
        TIM2_ClearFlag(TIM2_FLAG_CC1);                      // smažu vlajku
        time_start = TIM2_GetCapture1();                    // poznamenám si čas

        while(TIM2_GetFlagStatus(TIM2_FLAG_CC2) == RESET);  // čekám na sestupnou hranu
        TIM2_ClearFlag(TIM2_FLAG_CC2);                      // smažu vlajku
        time_end = TIM2_GetCapture2();                      // poznamenám si čas
        
        diff = (time_end - time_start) ;    // délka impoulzu v mikrosekundách
        printf("%lu us\n", diff);
        diff = diff * 340 / 20000;   // FixPoint přepočet na cm 
        printf("%lu cm\n\n", diff);

        _delay_ms(1000);
    }
}



/*-------------------------------  Assert -----------------------------------*/
#include "__assert__.h"
