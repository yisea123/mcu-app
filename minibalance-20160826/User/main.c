#include "gokit.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 2016 *****/ \
\r\n/******************************************/ \
\r\n");

extern u8 Flag_Stop;
Ringfifo uart3fifo;
uint32_t SystemTimeCount;
SYSTIMER *timer1 = NULL, *wdTimer = NULL;
void printf_systemrccClocks(void);
void hw_init(void);
void sw_init(void);

void poll_uart3_fifo(Ringfifo *mfifo) 
{
		char ch;
		static char pre = 0; 
		if (rfifo_len(mfifo) > 0) {
				if (rfifo_get(mfifo, &ch, 1) == 1) {
						if (pre != ch)
							handle_remote_control(ch);
						pre = ch;
				}
		}
}

int main(void)
{	
		SystemInit();	
		hw_init();
		sw_init();		
		
		while(1) {
				poll_system_timer();
				poll_uart3_fifo(&uart3fifo);
		}	
}

void key_gpioA_pin5_callback(KEY_VALUE value)
{
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		
		if (value == KEY_SINGLE) {
			  Flag_Stop = !Flag_Stop;
				printf("%s 1\r\n", __func__);
		} else if (value == KEY_DOUBLE) {
				Flag_Stop = !Flag_Stop;
				printf("%s 2\r\n", __func__);
		} else if (value == KEY_LONG_PRESS) {
				printf("%s 3\r\n", __func__);
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
		timer3_int_init(9, 7199);//ms interrupt	 
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

void led_flash_callback(void *timer)
{
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
}

void keyevent_detect(void *timer)
{
		key_scan();
}

void feed_watchdog_peroid(void *timer)
{
		watchdog_feed();
}

void usmart_test(void)
{
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		printf("%s\r\n", __func__);
}

void msecond_callback(void *timer)
{
		static unsigned int count = 0;
		count++;
		if ( count%500 == 0) {
				count = 0;
				printf("sysTick=%d\r\n", SystemTimeCount);
		}
}

void sw_init(void)
{
		rfifo_init(&uart3fifo);	
		init_system_timer();
		wdTimer = register_system_timer("feedDog", 400, feed_watchdog_peroid, REPEAT, NULL);
		usmart_dev.init();	
		timer1 = register_system_timer("LedFlash", TIMERSECOND/10, led_flash_callback, REPEAT, &SystemTimeCount);
		//register_system_timer("mSecond", 1, msecond_callback, REPEAT, &SystemTimeCount);
		register_system_timer("KeyScan", 7, keyevent_detect, REPEAT, NULL);
		register_system_timer("LcdDisplay", 100, poll_led_display, REPEAT, NULL);
		register_system_timer("miniCore", 5, poll_minibalance_core, REPEAT, NULL);
		printf("Minibalance Init Ok ...\r\n");
}

void printf_systemrccClocks(void)
{
		uint8_t SYSCLKSource;

		RCC_ClocksTypeDef  SystemRCC_Clocks;
		printf("System start...\r\n");
		SYSCLKSource = RCC_GetSYSCLKSource();
		if (SYSCLKSource==0x04)
			printf("SYSCLKSource is HSE\r\n");
		else if (SYSCLKSource==0x00)
		  printf ("SYSCLKSource is HSI\r\n");
		else if (SYSCLKSource==0x08)
			printf("SYSCLKSource is PL!\r\n");
		
		RCC_GetClocksFreq(&SystemRCC_Clocks);
		printf("SYS clock =%dMHz \r\n",(uint32_t)SystemRCC_Clocks.SYSCLK_Frequency/1000000);
		printf("HCLK clock =%dMHz \r\n",(uint32_t)SystemRCC_Clocks.HCLK_Frequency/1000000);
		printf("PCLK1 clock =%dMHz \r\n",(uint32_t)SystemRCC_Clocks.PCLK1_Frequency/1000000);
		printf("PCLK2_clock =%dMHz \r\n",(uint32_t)SystemRCC_Clocks.PCLK2_Frequency/1000000);	
		printf("SADCCLK_Frequencyclock =%dMHz \r\n",(uint32_t)SystemRCC_Clocks.ADCCLK_Frequency/1000000);
}


