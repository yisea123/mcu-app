#include "keydetect.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "timer.h"
#include "queue.h"
#include "xmalloc.h"
#include "semphr.h"
#include "led.h"
#include <stdio.h>

unsigned int keyEventTotal = 0;
KEYEVENT keyEvent[10];
/*
* author: yangjianzhou
* function: click_n_double.
* return 0: not action,   1:single click,  2:double click
**/
u8 click_n_double (u8 time, KEYEVENT *key)
{
	  if (KEY(key->port, key->pin) == 0) {  
			key->Forever_count++;//长按标志位未置1
		} else {
			key->Forever_count = 0;
		}
		if (0 == KEY(key->port, key->pin) && 0 == key->flag_key) {
			key->flag_key = 1;	
		}
	  if (0 == key->count_key) {
			if (key->flag_key == 1) {
				key->double_key++;
				key->count_key = 1;	
			}
			if (key->double_key == 2) {
				key->double_key = 0;
				key->count_single = 0;
				return 2;//双击执行的指令
			}
		}
		if (1 == KEY(key->port, key->pin))	{
			key->flag_key = 0;
			key->count_key = 0;
		}			
		if (1 == key->double_key) {
			key->count_single++;
			if (key->count_single > time && key->Forever_count < time) {
				key->double_key = 0;
				key->count_single = 0;	
				return 1;//单击执行的指令
			}
			if (key->Forever_count > time) {
				key->double_key = 0;
				key->count_single = 0;	
			}
		}	
		
		return 0;
}

/*
* author: yangjianzhou
* function: long_press.
* return 0: not action,   1:long press
**/
u8 long_press(KEYEVENT *key)
{	
		if (key->long_press == 0 && KEY(key->port, key->pin) == 0) { 
			key->long_press_count++;
		} else {                       
			key->long_press_count = 0;
		}			
		
		if (key->long_press_count > 300) {//200
			key->long_press = 1;	
			key->long_press_count = 0;
			return 1;
		}			
				
		//if (key->long_press == 1) { 
		//report long press again and again if we have not release key
		if (key->long_press == 1 && KEY(key->port, key->pin) == 1) {//report one time!			
				key->long_press = 0;
		}
		 
		return 0;
}

/*
* author: yangjianzhou
* function: key_scan must be call in every x ms.
* return 0: not action,   1:long press
**/
void key_scan(void)
{	
		u8 tmp, tmp2, i;
	
		for (i = 0; i < keyEventTotal && keyEvent[i].isUse; i++) {
			tmp = click_n_double(50, &(keyEvent[i])); 
			if (tmp == 1) {
				keyEvent[i].callback(KEY_SINGLE);
			} else if (tmp == 2) {
				keyEvent[i].callback(KEY_DOUBLE);
			}
			tmp2 = long_press(&(keyEvent[i]));                   
			if (tmp2 == 1) {
				keyEvent[i].callback(KEY_LONG_PRESS);
			}
		}
}

void key_gpioA_pin5_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin10_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin11_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin12_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin13_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin14_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

void key_gpioE_pin15_callback(KEY_VALUE value)
{
		char key;
	
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
				key = 1;	
				printf("%s %d\r\n", __func__, key);
		} else if (value == KEY_DOUBLE) {
				key = 2;				
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				key = 3;
				printf("%s 3\r\n", __func__);
		}

		//deliver_message(make_message(CMD_RUNNING_CONTROL, &key, 1), miniTimer);			
}

/*
* author: yangjianzhou
* function: register_key_event.
**/
void register_key_event(GPIO_TypeDef* port, uint16_t pin, keyevent_callback callback)
{
		//PA5
		int i;
    	GPIO_InitTypeDef GPIO_InitStructure;
		KEYEVENT *key = NULL;
	
		if (port == GPIOA)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		else if (port == GPIOB)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		else if (port == GPIOC)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		else if (port == GPIOD)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
		else if (port == GPIOE)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		else if (port == GPIOF)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
		else if (port == GPIOG)
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
		else 
			return;
		
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = pin;//GPIO_Pin_5;
    GPIO_Init(port, &GPIO_InitStructure);
		
		if (keyEventTotal < sizeof(keyEvent)/sizeof(keyEvent[0])) {
				for (i=0; i<sizeof(keyEvent)/sizeof(keyEvent[0]); i++) {
						if (keyEvent[i].isUse == 0) {
								break;
						}
				}
				if (i < sizeof(keyEvent)/sizeof(keyEvent[0])) {
						key = &(keyEvent[i]);
						key->isUse = 1;			
						key->port = port;
						key->pin = pin;
						key->callback = callback;
						keyEventTotal++;	
						printf("%s: add key[%d] detect success. keyEventTotal=%d\r\n", __func__, i, keyEventTotal);					
				}
		}
		if (key == NULL) {
				printf("%s: Fail!!!\r\n", __func__);
				return;
		}		
}

void keyevent_detect_task(TimerHandle_t xTimer )
{	
	portTickType xLastWakeTime;

 	printf("%s  enter\r\n", __func__);

	register_key_event(GPIOA, GPIO_Pin_5, key_gpioA_pin5_callback);
	register_key_event(GPIOE, GPIO_Pin_10, key_gpioE_pin10_callback);
	register_key_event(GPIOE, GPIO_Pin_11, key_gpioE_pin11_callback);
	register_key_event(GPIOE, GPIO_Pin_12, key_gpioE_pin12_callback);
	register_key_event(GPIOE, GPIO_Pin_13, key_gpioE_pin13_callback);
	register_key_event(GPIOE, GPIO_Pin_14, key_gpioE_pin14_callback);
	register_key_event(GPIOE, GPIO_Pin_15, key_gpioE_pin15_callback);

	xLastWakeTime = xTaskGetTickCount();	
	while (1) {
		vTaskDelayUntil(&xLastWakeTime, 6/portTICK_PERIOD_MS);	
		key_scan();
	}
}


