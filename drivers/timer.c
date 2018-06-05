#include <assert.h>
#include <s3c24xx.h>
#include <interrupt.h>
#include <timer.h>
/*
 * ��дTCNTBn,TCMPBn���������ֶ����£������������ֶ����£���д�Ĵ���
 */

void timer_init() {
	TCFG0 = 0;
	TCFG1 = 0;
	TCON  = 0;
	TCFG0 |= (124);		//��ʱ�� 0��1 ��Ԥ��Ƶֵ
	TCFG0 |= (24 << 8);	//��ʱ�� 2��3 �� 4 ��Ԥ��Ƶֵ
}

static void tick_irq_hander();
static unsigned int delta_time = 0;

void init_tick(unsigned int time, void (*handle)()) {
	assert(time <= 0xffff);
	//TCON:��ʱ�����ƼĴ���
	TCON &= ~(1 << 20);		//����
	free_irq(INT_TIMER4);
	TCON &= ~(7 << 20);	//���20~21λ
	TCON |= (1 << 22);		//��ʱ��4��϶ģʽ/�Զ�����
	//TCONB4:��ʱ��4��������Ĵ���

	TCNTB4 = delta_time = time;
	TCON |= (1 << 21);		//��ʱ��4�ֶ�����TCNTB4
	TCON &= ~(1 << 21);		//��ʱ��4ȡ���ֶ�����
	if (handle)
		request_irq(INT_TIMER4, handle);
	else
		request_irq(INT_TIMER4, tick_irq_hander);
	TCON |= (1 << 20);		//����
}

static volatile unsigned long long tick = 0;

int get_sys_tick() {
	return tick;
}

unsigned long long get_system_time_us(void) {
	return delta_time * tick + (delta_time - TCNTO4);
}

unsigned long long delta_time_us(unsigned long long pre, unsigned long long now) {
	return (now - pre);
}

static void tick_irq_hander() {
	tick++;
}

static void (*timer_handle)() = 0;

static void timer_handler() {
	TCON &= ~(1 << 8); //�ر�
	free_irq(INT_TIMER1);
	if (timer_handle)
		timer_handle();
}

void set_timer(unsigned int time, void (*handle)()) {
	assert((100 * time <= 0xffff) && handle);
	if (!handle)
		return;
	TCON &= ~(1 << 8); //�ر�
	free_irq(INT_TIMER1);
	//TCON:��ʱ�����ƼĴ���
	TCFG1 &= ~(15 << 4);
	TCFG1 |= 1 << 4;
	TCON &= ~(15 << 8);	//���8~11λ
	TCON &= ~(1 << 11);		//��ʱ��1����̬

	//TCONB1:��ʱ��1��������Ĵ���
	TCNTB1 = 100 * time;
	TCMPB1 = 0;
	TCON |= (1 << 9);		//��ʱ��1�ֶ�����TCNTB1��TCMPB1
	TCON &= ~(1 << 9);		//��ʱ��1ȡ���ֶ�����
	timer_handle = handle;
	request_irq(INT_TIMER1, timer_handler);
	TCON |= (1 << 8);		//����
}

void close_timer() {
	TCON &= ~(1 << 8); //�ر�
	free_irq(INT_TIMER1);
}

static volatile int delay_end = 0;
static void delay_irq_hander() {
	TCON &= ~(1 << 12); //��ʱ���ر�
	free_irq(INT_TIMER2);
	delay_end = 1;
}
void delay_u(unsigned int delay_time) {
	assert(delay_time <= 0xffff);
	TCON &= ~(1 << 12); //�ر�
	free_irq(INT_TIMER2);

	if (delay_time > 0xffff)
		delay_time = 0xffff;


	//TCON:��ʱ�����ƼĴ���
	TCON &= ~(0x0f << 12);	//���12~11λ
	TCON &= ~(1 << 15);		//��ʱ��2����̬

	//TCONB2:��ʱ��2��������Ĵ���
	TCNTB2 = delay_time;
	TCMPB3 = 0;
	TCON |= (1 << 13);		//��ʱ��2�ֶ�����TCNTB2��TCMPB2
	TCON &= ~(1 << 13);		//��ʱ��2ȡ���ֶ�����
#if 1
	request_irq(INT_TIMER2, delay_irq_hander);
	delay_end = 0;
	TCON |= (1 << 12);		//����
	while (!delay_end);
#else
	//֮�����ṩ���жϰ汾���Ƿ�ֹ�ж����ε�������������
	TCON |= (1 << 12);		//����
	//��ʱ������TCNTO2�޷��������£�����1us�ӳ�
	udelay(1);
	while (TCNTO2);
#endif
	free_irq(INT_TIMER2);
}
