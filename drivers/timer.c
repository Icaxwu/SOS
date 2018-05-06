#include <assert.h>
#include <s3c24xx.h>
#include <interrupt.h>


void timer_init() {
	TCFG0 =0;
	TCFG1 =0;
	TCON  =0;
	TCFG0 |= (124);		//��ʱ�� 0��1 ��Ԥ��Ƶֵ //TODO:����̫С����������⣬�����������������
	TCFG0 |= (24 << 8);	//��ʱ�� 2��3 �� 4 ��Ԥ��Ƶֵ
}
void tick_irq_hander();
void init_tick(unsigned int time, void (*handle)()) {
	assert(time <= 0xffff);
	//TCON:��ʱ�����ƼĴ���
	INTMSK_clr(INT_TIMER4);
	TCON &= ~(1 << 20);		//����
	TCON &= (~(7 << 20));	//���20~21λ
	TCON |= (1 << 22);		//��ʱ��4��϶ģʽ/�Զ�����
	TCON |= (1 << 21);		//��ʱ��4�ֶ�����TCNTB4

	//TCONB4:��ʱ��4��������Ĵ���
	TCNTB4 = time;

	TCON |= (1 << 20);		//����
	TCON &= ~(1 << 21);		//��ʱ��4ȡ���ֶ�����

	set_irq_handler(INT_TIMER4, handle);
	INTMSK_set(INT_TIMER4);
}

static volatile int tick = 0;

int get_sys_tick() {
	return tick;
}

void tick_irq_hander() {
	tick ++;
}

static void (*timer_handle)() = 0;

static void timer_handler() {
	if (timer_handle)
		timer_handle();
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
}

void set_timer(unsigned int time, void (*handle)()) {
	assert(100*time <= 0xffff);
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
	//TCON:��ʱ�����ƼĴ���
	TCFG1 &= ~(15<<4);
	TCFG1 |= 1<<4;
	TCON &= (~(15 << 8));	//���8~11λ
	TCON &= ~(1 << 11);		//��ʱ��1����̬
	TCON |= (1 << 9);		//��ʱ��1�ֶ�����TCNTB1��TCMPB1

	//TCONB1:��ʱ��1��������Ĵ���
	TCNTB1 = 100*time;

	TCON |= (1 << 8);		//����
	TCON &= ~(1 << 9);		//��ʱ��1ȡ���ֶ�����

	if (handle)
		timer_handle = handle;
	set_irq_handler(INT_TIMER1, timer_handler);
	INTMSK_set(INT_TIMER1);
}

void close_timer() {
	INTMSK_clr(INT_TIMER1);
	TCON &= ~(1 << 8); //�ر�
}

static volatile int delay_start = 0;
static void delay_irq_hander();

void delay_u(unsigned int delay_time) {
	assert(delay_time <= 0xffff);
	INTMSK_clr(INT_TIMER2);
	TCON &= ~(1 << 12); //�ر�

	if (delay_time > 65535)
		delay_time = 65535;
	delay_start = 1;

	//TCON:��ʱ�����ƼĴ���
	TCON &= (~(15 << 12));	//���8~11λ
	TCON &= ~(1 << 15);		//��ʱ��2����̬
	TCON |= (1 << 13);		//��ʱ��2�ֶ�����TCNTB2��TCMPB2

	//TCONB2:��ʱ��2��������Ĵ���
	TCNTB2 = delay_time;

	TCON |= (1 << 12);		//����
	TCON &= ~(1 << 13);		//��ʱ��2ȡ���ֶ�����

	set_irq_handler(INT_TIMER2, delay_irq_hander);
	//TODO:1������ж�Ƕ��,2�ܿ��ز��������
	INTMSK_set(INT_TIMER2);
	while (delay_start != 0);
	INTMSK_clr(INT_TIMER2);
}

static void delay_irq_hander() {
	

	delay_start = 0;
	INTMSK_clr(INT_TIMER2);
	TCON &= (~(1 << 12)); //��ʱ���ر�
}
