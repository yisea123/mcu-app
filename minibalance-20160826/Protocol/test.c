#include "test.h"
#include "ostimer.h"

extern uint32_t SystemTimeCount;
SYSTIMER *timerTest = NULL;

void usmart_test(void)
{
		if (get_led_status(LED1)) {
				led_off(LED1);
		} else { 
				led_on(LED1);
		}
		printf("%s\r\n", __func__);
}

void test_task_callback(void *timer)
{
		static unsigned int count = 0;
	
		SYSTIMER* argc = (SYSTIMER*) timer;
		if (argc->isMessage) {
				if (argc->message) {
					printf("%s: msg->data=%s, size=%d, cmd=%d\r\n", __func__,
							argc->message->data, argc->message->size, argc->message->cmd);
					release_message(argc->message);
				} 
				return;
		}	
		
		count++;
		if ( count%1 == 0) {
				count = 0;
				//printf("sysTick=%d\r\n", SystemTimeCount);
		}
}

void test_put_msg(char cmd, char *text)
{
		deliver_message(make_message(cmd, text, strlen(text)), timerTest);
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

void os_timer_callback_test(void *ptmr, void *parg)
{
		printf("%s: %s -SystemTimeCount=%d\r\n", __func__, ((OS_TMR*)ptmr)->OSTmrName, SystemTimeCount);
}

void test_os_timer(void)
{
		unsigned char err;
	
		OS_TMR  *ostimer = create_os_timer(5000,
                      20,
                      OS_TMR_OPT_PERIODIC,
                      os_timer_callback_test,
                      &SystemTimeCount,
                      "osTimerTest",
                      &err);

		if (err == OS_ERR_NONE) {
				printf("%s| create osTimerTest scueess\r\n", __func__);
				start_os_timer(ostimer, &err);
		} else {
				printf("%s| create osTimerTest fail\r\n", __func__);
		}
}


