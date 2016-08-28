#include "led.h"
#include "timer.h"

char led1Status = 0;
SYSTIMER *ledTimer = NULL;

void led_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(GPIO_LED1_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_LED1_PIN;
    GPIO_Init(GPIO_LED1_PORT, &GPIO_InitStructure);

    led_off(LED1);
}

void led_on(uint8_t LEDNUM)
{
    switch (LEDNUM)
    {
			case LED1:
					led1Status = 1;
					GPIO_SetBits(GPIO_LED1_PORT,GPIO_LED1_PIN);
					break;
			default:
					break;
    }
}

void led_off(uint8_t LEDNUM)
{
    switch (LEDNUM)
    {
			case LED1:
					led1Status = 0;
					GPIO_ResetBits(GPIO_LED1_PORT,GPIO_LED1_PIN);
					break;
			default:
					break;
    }
}

char get_led_status(uint8_t LEDNUM) 
{
    switch (LEDNUM)
    {
			case LED1:
					return led1Status;
					break;
			default:
					break;
    }	
}


void led_flash_task(void *timer)
{
		SYSTIMER* argc = (SYSTIMER*) timer;
		if (argc->isMessage) {
				if (argc->message) {
					release_message(argc->message);
				} 
				return;
		}	
		
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
}
