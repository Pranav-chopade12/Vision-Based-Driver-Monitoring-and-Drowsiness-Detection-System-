#include "drowsiness.h"

static char state = 'W';

void delay_ms(uint32_t ms)
{
    for(uint32_t i=0;i<ms*4000;i++)
    {
        __NOP();
    }
}

/* LEDs */
void LED_Green_ON(void)
{
    GPIOG->BSRR = (1<<13);
}

void LED_Green_OFF(void)
{
    GPIOG->BSRR = (1<<(13+16));
}

void LED_Red_ON(void)
{
    GPIOG->BSRR = (1<<14);
}

void LED_Red_OFF(void)
{
    GPIOG->BSRR = (1<<(14+16));
}

void LEDs_OFF(void)
{
    LED_Green_OFF();
    LED_Red_OFF();
}

/* Buzzer */
void Buzzer_ON(void)
{
    GPIOB->BSRR = (1<<0);
}

void Buzzer_OFF(void)
{
    GPIOB->BSRR = (1<<(0+16));
}

void Drowsiness_Init(void)
{
    /* GPIOA GPIOB GPIOG Clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;


    /* PG13 PG14 Output */
    GPIOG->MODER &= ~((3U<<26)|(3U<<28));
    GPIOG->MODER |=  ((1U<<26)|(1U<<28));

    /* PB0 Output */
    GPIOB->MODER &= ~(3U<<0);
    GPIOB->MODER |=  (1U<<0);


    LEDs_OFF();
    Buzzer_OFF();
}

char GetDriverState(void)
{
    return state;
}

void Drowsiness_Task(void)
{
    static uint32_t tick = 0;

    if(USART1->SR & USART_SR_RXNE)
    {
        char rx = USART1->DR;

        if(rx=='W' || rx=='D' || rx=='A')
        {
            state = rx;
        }
    }

    if(state=='W')
    {
        LEDs_OFF();
        Buzzer_OFF();
    }
    else if(state=='D')
    {
        LED_Green_OFF();

        if((tick%500)<250)
        {
            LED_Red_ON();
            //Buzzer_ON();
        }
        else
        {
            LED_Red_OFF();
           // Buzzer_OFF();
        }
    }
    else if(state=='A')
    {
        if((tick%100)<50)
        {
            LED_Red_ON();
            LED_Green_OFF();
            Buzzer_ON();
        }
        else
        {
            LED_Red_OFF();
            LED_Green_ON();
            Buzzer_OFF();
        }
    }

    delay_ms(1);

    tick++;

    if(tick>10000)
    {
        tick=0;
    }
}


void Send_State_To_ESP32(char state)
{
    uint8_t packet[3] = {0xAA, state, 0x55};
    uint32_t timeout;

    for(int i = 0; i < 3; i++)
    {
        timeout = 10000;
        while(!(USART1->SR & (1 << 7)) && timeout--)
        {
            if(timeout == 0) return;
        }
        USART1->DR = packet[i];
    }

    timeout = 10000;
    while(!(USART1->SR & (1 << 6)) && timeout--);
}
