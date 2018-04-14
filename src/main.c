#include <stdio.h>
#include <s3c24xx.h>
#include "serial.h"
#include "command.h"
#include "interrupt.h"

//ucosii
#include "ucos_ii.h"
#include "app_cfg.h"

//lcd driver
#include "lcddrv.h"
#include "framebuffer.h"



//ucgui
#include "GUI.h"

//lwip
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/autoip.h"
#include "lwip/igmp.h"
#include "lwip/dns.h"
#include "netif/etharp.h"

//dm9000 driver
#include "dm9000.h"

struct netif dm9000a_netif;

OS_STK  MainTaskStk[MainTaskStkLengh];
OS_STK	Task0Stk[Task0StkLengh];       // Define the Task0 stack

void OSTickISR();
void LWIP_Module_Init(void);
extern err_t ethernetif_init(struct netif *netif);
extern void  ethernetif_input(struct netif *netif,void *ptr);

int main() {
	uart0_init();
	init_irq();

	printf("\n\n************************************************\n");
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
	printf("��ʼ��lwip...\n");
	LWIP_Module_Init();
	EINT_DM9000_Init();
	printf("��������0...\n");
	OSTaskCreate (Task0, (void *)0, &Task0Stk[Task0StkLengh - 1], Task0Prio);
	INT8U  err;
	OSTaskNameSet(Task0Prio, (INT8U *)"Task0", &err);
	printf("��������0���\n");
	httpserver_init();
	while (1){
		//printf("OSMainTask Delay\n");
		OSTimeDly(OS_TICKS_PER_SEC);
	}
	init_Ts();
	MainTask();
	while (1);
}

/*=================================================================================================
description: handle the dm9000a mac frame,actived by dm9000a ISR
argument   : 
             
             
return     : none
=================================================================================================*/
void Task0(void *pdata)
{
    INT8U err;
    void  *q_msg;
    DM_RXPKT *tmp;
    
    pdata=pdata;
    printf("����0����...\n");
    while(1)
    {
       q_msg = OSQPend(rxpkt_q, 0, &err);
	   //printf("����0����\n");
       tmp =(DM_RXPKT *)q_msg;
       if(err==OS_NO_ERR)
       {
          //ethernetif_input() will discard all pkts but IP,ARP,PPPOEDISC,PPPOE,and
          //use (*input) callback function registed in the dm9000a_netif struct
          //in this case the callback is tcpip_input(registed by netif_add() ),
          //the tcpip_input will post mail(queue) to tcpip_thread which is the main
          //loop for tcpip,the tcpip_thread recv the mail and use ethernet_input()
          //to COPY and handle the incoming pkt.
          ethernetif_input(&dm9000a_netif,q_msg);
          //this function was supposed to be called by an ISR,but I made it called by
          //a task. 
       }
       OSMemPut(rxpkt_q_mem,q_msg);
	   //printf("ֹͣ\n");
    }
     
}

/*=================================================================================================
description: call when tcpip_init is done
argument   : @pdata--pointer to  user data area
             
             
return     : none
=================================================================================================*/
int tcpip_init_hook(void *pdata)
{
   pdata = pdata;
   return ERR_OK;
}

void EINT_DM9000_Init(void)
{
   set_irq_handler(EINT4_7, DM9000A_Rx);
   //GPF7=EINT7
   GPFCON &=  ~(3<<14);
   GPFCON |=  (2<<14);
   GPFUP  = 0xff;//GPF0-7��ֹ��������
   EXTINT0 &= ~(7<<28);
   EXTINT0 |= (1<<28);//EINT7�ߵ�ƽ
   EINTMASK &= ~(1<<EINT7);
   INTMSK_set(EINT4_7);
}
/*=================================================================================================
description: lwip module init include(tcpip_init,tcpip_thread,ethernetif_init)
argument   : 
             
             
return     : none
=================================================================================================*/
void LWIP_Module_Init(void)
{
   struct ip_addr ipaddr,netmask,gateway;

   
   //lwip_init();
   tcpip_init(NULL, NULL);
   IP4_ADDR(&ipaddr ,192,168, 11, 3);
   IP4_ADDR(&netmask,255,255,255, 0);
   IP4_ADDR(&gateway,192,168, 11, 1);
   
   netif_add(&dm9000a_netif,&ipaddr,&netmask,&gateway,
             (void *)NULL,
             ethernetif_init,
             tcpip_input);
   netif_set_default(&dm9000a_netif);
   netif_set_up(&dm9000a_netif);
}
