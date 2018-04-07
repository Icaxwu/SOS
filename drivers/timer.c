#include <s3c24xx.h>
#include "interrupt.h"

void delay_irq_hander();

void init_tick(int time, void (*handle)()) {
	//��ʱ�����ƼĴ��� 0 ��TCFG0��
	TCFG0 |= (24 << 8); //��ʱ�� 2��3 �� 4 ��Ԥ��Ƶֵ
	//��ʱ�����ƼĴ��� 1 ��TCON��
	TCON &= (~(7 << 20)); //���20~21λ
	TCON |= (1 << 22); //��ʱ�� 4����̬
	TCON |= (1 << 21); //��ʱ�� 4�ֶ����� TCNTB4
	//TCONB4:��ʱ�� 4  ��������Ĵ���
	TCNTB4 = time;

	TCON |= (1 << 20); //����
	TCON &= ~(1 << 21); //��ʱ�� 4 ȡ���ֶ�����

	set_irq_handler(INT_TIMER4, handle);
	INTMSK_set(INT_TIMER4);
}
static volatile int tick = 0;
int get_sys_tick() {
	return tick;
}
void delay_irq_hander() {
	tick ++;
}
static void (*timer_handle)() = 0;
void timer_handler() {
	if (timer_handle)
		timer_handle();
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
}
void set_timer(int time, void (*handle)()) {
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
	//��ʱ�����ƼĴ��� 0 ��TCFG0��
	TCFG0 |= (24); //��ʱ�� 0��1 ��Ԥ��Ƶֵ
	//��ʱ�����ƼĴ��� 1 ��TCON��
	TCON &= (~(15 << 8)); //���8~11λ
	TCON |= (0 << 11); //��ʱ�� 1��϶ģʽ���Զ����أ�
	TCON |= (1 << 9); //��ʱ�� 1�ֶ����� TCNTB1
	//TCONB1:��ʱ�� 1  ��������Ĵ���
	TCNTB1 = 1000 * time;

	TCON |= (1 << 8); //����
	TCON &= ~(1 << 9); //��ʱ�� 1 ȡ���ֶ�����

	if (handle)
		timer_handle = handle;
	set_irq_handler(INT_TIMER1, timer_handler);
	INTMSK_set(INT_TIMER1);
}
void close_timer() {
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
}
