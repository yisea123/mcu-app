#include "gokit.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 2016 *****/ \
\r\n/******************************************/ \
\r\n");

Ringfifo uart3fifo;
uint32_t SystemTimeCount;
void hw_init(void);
void sw_init(void);
	
int main(void)
{	
		SystemInit();	
		hw_init();
		sw_init();		
		
		while(1) {
				poll_system_timer();
				poll_os_timer();
				poll_uart3_fifo(&uart3fifo);
		}	
}

void hw_init(void)
{
		delay_init(72);
		jtag_set(JTAG_SWD_DISABLE);
		jtag_set(SWD_ENABLE);
		interrupt_priority_init(NVIC_PriorityGroup_2);
		uarts_init();
		watchdog_init(3);  
		timer3_int_init(8, 7000);//9, 7199 ms interrupt	 
		//timer4_int_init(1000-1, 7199);//5s interrupt is max!	
		BOOT_LOG	
		printf_systemrccClocks();		
		led_gpio_init();//PA4
		register_key_event(GPIOA, GPIO_Pin_5, key_gpioA_pin5_callback);
		
		OLED_Init();
		Adc_Init();
		IIC_Init();
		MPU6050_initialize();
		DMP_Init();
		Encoder_Init_TIM2();//use TIME2
		Encoder_Init_TIM4();//use TIME4
		MiniBalance_PWM_Init(7199, 0);
		//timer2_int_init(1000-1, 7199);
		//timer1_int_init(1000-1, 7199);
}

void sw_init(void)
{
	 	my_mem_init(SRAMIN);
		rfifo_init(&uart3fifo);	
		init_system_timer();
		init_os_timer();
		usmart_dev.init();	
		wdTimer = register_system_timer("FeedDogTask", 200, feed_watchdog_task, REPEAT, NULL);	
		ledTimer = register_system_timer("LedFlashTask", TIMERSECOND/10, led_flash_task, REPEAT, &SystemTimeCount);
		timerTest = register_system_timer("TestTask", 10000, test_task_callback, REPEAT, &SystemTimeCount);
		keyTimer = register_system_timer("KeyScanTask", 5, keyevent_detect_task, REPEAT, NULL);
		lcdTimer = register_system_timer("LcdDisplay", 100, lcd_display_task, REPEAT, NULL);
		miniTimer = register_system_timer("MiniBalanceCore", 999999999, minibalance_core_task, REPEAT, NULL);
		printf("Minibalance Init Ok ...\r\n");
}



