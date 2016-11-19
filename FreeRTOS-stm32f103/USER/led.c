#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "timer.h"
#include "queue.h"
#include "xmalloc.h"
#include "semphr.h"
#include <stdio.h>

char led1Status = 0;
//SYSTIMER *ledTimer = NULL;

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
	char status;
	
    switch (LEDNUM)
    {
			case LED1:
					status = led1Status;
					break;
			default:
					break;
    }	
		
		return status;
}

void gprs_vcc_gpio_init(void);
void gprs_reset_gpio_init(void);

void led_flash_task(void *argc)
{		
	portTickType xLastWakeTime;
 	printf("%s  enter\r\n", __func__);

	led_gpio_init();
	gprs_vcc_gpio_init();
	gprs_reset_gpio_init();
	
	xLastWakeTime = xTaskGetTickCount();	
	while (1) {
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		vTaskDelayUntil(&xLastWakeTime, 6000/portTICK_PERIOD_MS);	
	}
}

//GPRS_VCC_EN
//#define GPIO_GPRO_VCC_CLK    RCC_APB2Periph_GPIOE
//#define GPIO_GPRO_VCC_PORT   GPIOE
//#define GPIO_GPRO_VCC_PIN    GPIO_Pin_2
//
void gprs_vcc_on( void );
void gprs_vcc_off( void );


void gprs_vcc_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(GPIO_GPRO_VCC_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_GPRO_VCC_PIN;
    GPIO_Init(GPIO_GPRO_VCC_PORT, &GPIO_InitStructure);

    gprs_vcc_on();
}

void gprs_vcc_off( void )
{
	GPIO_SetBits(GPIO_GPRO_VCC_PORT,GPIO_GPRO_VCC_PIN);
}

void gprs_vcc_on( void )
{

	GPIO_ResetBits(GPIO_GPRO_VCC_PORT,GPIO_GPRO_VCC_PIN);

}



void gprs_reset_height( void );
void gprs_reset_low( void );

void gprs_reset_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(GPIO_GPRO_RESET_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_GPRO_RESET_PIN;
    GPIO_Init(GPIO_GPRO_RESET_PORT, &GPIO_InitStructure);

	gprs_reset_height();
	vTaskDelay(400);
    gprs_reset_low();
}

void gprs_reset_height( void )
{
	GPIO_SetBits(GPIO_GPRO_RESET_PORT, GPIO_GPRO_RESET_PIN);
}

void gprs_reset_low( void )
{
	GPIO_ResetBits(GPIO_GPRO_RESET_PORT, GPIO_GPRO_RESET_PIN);
}

void gprs_reset( void )
{
	printf("%s\r\n", __func__);
	gprs_reset_height();
	vTaskDelay(400);
    gprs_reset_low();
}


