#include <stdio.h>
#include <s3c24xx.h>
#include "ucos_ii.h"
#include "serial.h"
#include "command.h"

#include "lcddrv.h"
#include "framebuffer.h"

#include "app_cfg.h"
#include "interrupt.h"

#include "GUI.H"


OS_STK  MainTaskStk[MainTaskStkLengh];
OS_STK	Task0Stk[Task0StkLengh];       // Define the Task0 stack

void OSTickISR();

int main() {
	uart0_init();
	init_irq();

	printf("\n\n************************************************\n");
	printf("��ʼ��MMU...\n");
	mmu_init();
	printf("��ʼ��LCD...\n");
	Lcd_Port_Init();						// ����LCD����
	Tft_Lcd_Init(MODE_TFT_16BIT_480272);	// ��ʼ��LCD����
	Lcd_PowerEnable(0, 1);					// ����LCD_PWREN��Ч�������ڴ�LCD�ĵ�Դ
	Lcd_EnvidOnOff(1);						// ʹ��LCD����������ź�
	ClearScr(0x0);							// ����

	printf("��ʼ��uC/OS...\n");
	OSInit ();
	printf("��ʼ��ϵͳʱ��...\n");
	OSTimeSet(0);
	printf("����ϵͳ��ʼ����...\n");
	OSTaskCreate (OSMainTask, (void *)0, &MainTaskStk[MainTaskStkLengh - 1], MainTaskPrio);
	INT8U  err;
	OSTaskNameSet(MainTaskPrio, (INT8U *)"MainTask", &err);
	printf("����uC/OS...\n");
	OSStart ();
	while (1);
}

void OSMainTask(void *pdata) {

#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
	OS_CPU_SR  cpu_sr;
#endif
	enable_irq();
	OS_ENTER_CRITICAL();
	//initial timer for ucos time tick
	init_tick(1000, OSTimeTick);
	OS_EXIT_CRITICAL();
	OSStatInit();

	init_Ts();
	MainTask();
	while (1);
}
