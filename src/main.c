#include <stdio.h>
#include <s3c24xx.h>
#include <interrupt.h>
#include <serial.h>
#include <assert.h>
#include "command.h"

//ucosii
#include "ucos_ii.h"
#include "app_cfg.h"

//lcd driver
#include "lcddrv.h"
#include "framebuffer.h"

//ucgui
#include "GUI.h"

//fs
#include "ff.h"

OS_STK  MainTaskStk[MainTaskStkLengh];
OS_STK	Task0Stk[Task0StkLengh];       // Define the Task0 stack

void OSTickISR();

FATFS fatworkarea;         // Work area (file system object) for logical drives 

void show_bss_info(){
	extern char __bss_start, __bss_end;
    unsigned int size = &__bss_end - &__bss_start;
	printf("BSS��СΪ��%uKB, %uMB\n", size/1024, size/1024/1024);
}
static void initer(void (*init)(), char *msg){
	assert(init);
	if(init){
		if(msg)
			printf(msg);
		init();
	}
}

void OSMainTask(void *pdata) {
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
	OS_CPU_SR  cpu_sr;
#endif

	OS_ENTER_CRITICAL();
	//initial timer for ucos time tick
	init_tick(1000, OSTimeTick);
	OS_EXIT_CRITICAL();

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();		//----ͳ�������ʼ������
#endif

	printf("��ʼ������...\n");
	net_init();

	printf("��������0...\n");
	OSTaskCreate (Task0, (void *)0, &Task0Stk[Task0StkLengh - 1], Task0Prio);
	INT8U  err;
	OSTaskNameSet(Task0Prio, (INT8U *)"Task0", &err);

	init_Ts();
	MainTask2();
	while (1);
}


void Task0(void *pdata) {
	printf("����0����...\n");
	while (1) {
		cmd_loop();
	}
}

void ucosii_init(){
	OSInit ();

	printf("��ʼ��ϵͳʱ��...\n");
	OSTimeSet(0);

	printf("����ϵͳ��ʼ����...\n");
	OSTaskCreate (OSMainTask, (void *)0, &MainTaskStk[MainTaskStkLengh - 1], MainTaskPrio);
	INT8U  err;
	OSTaskNameSet(MainTaskPrio, (INT8U *)"MainTask", &err);

	printf("����uC/OS...\n");
	OSStart ();
}

int main() {
	irq_init();
	Port_Init();
	uart0_init();
	//uart0_interrupt_init();
	printf("\n\n************************************************\n");
	show_bss_info();

	printf("��ʼ��MMU...\n");
	mmu_init();

	printf("��ʼ��TIMER...\n");
	timer_init();

	printf("��ʼ��LCD...\n");
	Lcd_Port_Init();						// ����LCD����
	Tft_Lcd_Init(MODE_TFT_16BIT_480272);	// ��ʼ��LCD����
	Lcd_PowerEnable(0, 1);					// ����LCD_PWREN��Ч�������ڴ�LCD�ĵ�Դ
	Lcd_EnvidOnOff(1);						// ʹ��LCD����������ź�
	ClearScr(0x0);							// ����

	printf("��ʼ��SD������...\n");
	SDI_init();

	printf("��ʼ��fatfs...\n");
	f_mount(0,&fatworkarea);

	printf("ʹ��IRQ...\n");
	enable_irq();

	cmd_loop();

	printf("��ʼ��uC/OS...\n");
	ucosii_init();
	while (1);
}
