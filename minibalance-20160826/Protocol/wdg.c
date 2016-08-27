#include "wdg.h"

//��ʼ���������Ź�
//prer:��Ƶ��:0~7(ֻ�е�3λ��Ч!)
//��Ƶ����=4*2^prer.�����ֵֻ����256!
//rlr:��װ�ؼĴ���ֵ:��11λ��Ч.
//ʱ�����(���):Tout=((4*2^prer)*rlr)/40 (ms).
void watchdog_init(int second) 
{	
		//4*2^prer max = 256 -> prer max = 6;
		int prer = 4;
		u16 rlr = 625*second; //max->1024
	
		//timer out = ((4*2^prer)*rlr)/40 (ms).
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����
		IWDG_SetPrescaler(prer);  //����IWDGԤ��Ƶֵ:����IWDGԤ��ƵֵΪ64
		IWDG_SetReload(rlr);  //����IWDG��װ��ֵ
		IWDG_ReloadCounter();  //����IWDG��װ�ؼĴ�����ֵ��װ��IWDG������
		IWDG_Enable();  //ʹ��IWDG
}

//ι�������Ź�
void watchdog_feed(void)
{   
		IWDG_ReloadCounter();//reload										   
}


