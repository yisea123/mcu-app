#ifndef _LED_H
#define _LED_H

#include <stdio.h>
#include <stm32f10x.h>

#define GPIO_LED1_CLK    RCC_APB2Periph_GPIOA
#define GPIO_LED1_PORT   GPIOA
#define GPIO_LED1_PIN    GPIO_Pin_4

#define GPIO_LED2_CLK    RCC_APB2Periph_GPIOB
#define GPIO_LED2_PORT   GPIOB
#define GPIO_LED2_PIN    GPIO_Pin_14

#define GPIO_LED3_CLK    RCC_APB2Periph_GPIOB
#define GPIO_LED3_PORT   GPIOB
#define GPIO_LED3_PIN    GPIO_Pin_11

#define GPIO_LED4_CLK    RCC_APB2Periph_GPIOB
#define GPIO_LED4_PORT   GPIOB
#define GPIO_LED4_PIN    GPIO_Pin_1



#define LED1             0X01
#define LED2             0X02
#define LED3             0X04
#define LED4             0X08


extern void led_gpio_init(void);
extern void led_on(uint8_t LEDNUM);
extern void led_off(uint8_t LEDNUM);
extern char get_led_status(uint8_t LEDNUM);
extern void led_flash_task(void *argc);

#endif



