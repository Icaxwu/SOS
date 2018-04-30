#include <stdio.h>
#include <interrupt.h>
#include <timer.h>
#include "s3c24xx.h"
#include "serial.h"

// ADCCON�Ĵ���
#define PRESCALE_DIS        (0 << 14)
#define PRESCALE_EN         (1 << 14)
#define PRSCVL(x)           ((x) << 6)
#define ADC_INPUT(x)        ((x) << 3)
#define ADC_START           (1 << 0)
#define ADC_ENDCVT          (1 << 15)

// ADCTSC�Ĵ���
#define UD_SEN          (1 << 8)
#define DOWN_INT        (UD_SEN*0)
#define UP_INT          (UD_SEN*1)
#define YM_SEN          (1 << 7)
#define YM_HIZ          (YM_SEN*0)
#define YM_GND          (YM_SEN*1)
#define YP_SEN          (1 << 6)
#define YP_EXTVLT       (YP_SEN*0)
#define YP_AIN          (YP_SEN*1)
#define XM_SEN          (1 << 5)
#define XM_HIZ          (XM_SEN*0)
#define XM_GND          (XM_SEN*1)
#define XP_SEN          (1 << 4)
#define XP_EXTVLT       (XP_SEN*0)
#define XP_AIN          (XP_SEN*1)
#define XP_PULL_UP      (1 << 3)
#define XP_PULL_UP_EN   (XP_PULL_UP*0)
#define XP_PULL_UP_DIS  (XP_PULL_UP*1)
#define AUTO_PST        (1 << 2)
#define CONVERT_MAN     (AUTO_PST*0)
#define CONVERT_AUTO    (AUTO_PST*1)
#define XP_PST(x)       (x << 0)

#define NOP_MODE        0
#define X_AXIS_MODE     1
#define Y_AXIS_MODE     2
#define WAIT_INT_MODE   3


/* ���ý���ȴ��ж�ģʽ��XP_PU,XP_Dis,XM_Dis,YP_Dis,YM_En
 * (1)����S3C2410��λ[8]ֻ��Ϊ0������ֻ��ʹ�������wait_down_int��
 *    ���ȵȴ�Pen Down�жϣ�Ҳ�ȴ�Pen Up�ж�
 * (2)����S3C2440��λ[8]Ϊ0��1ʱ�ֱ��ʾ�ȴ�Pen Down�жϻ�Pen Up�ж�
 */
/* ����"�ȴ��ж�ģʽ"���ȴ������������� */
#define wait_down_int() { ADCTSC = DOWN_INT | XP_PULL_UP_EN | \
	                                   XP_AIN | XM_HIZ | YP_AIN | YM_GND | \
	                                   XP_PST(WAIT_INT_MODE); }
/* ����"�ȴ��ж�ģʽ"���ȴ����������ɿ� */
#define wait_up_int()   { ADCTSC = UP_INT | XP_PULL_UP_EN | XP_AIN | XM_HIZ | \
	                                   YP_AIN | YM_GND | XP_PST(WAIT_INT_MODE); }

/* �����Զ�(����) X/Y������ת��ģʽ */
#define mode_auto_xy()  { ADCTSC = CONVERT_AUTO | XP_PULL_UP_DIS | XP_PST(NOP_MODE); }


static int adc_x = 0, adc_y = 0;
int get_touch_x() {
	return adc_x;
}
int get_touch_y() {
	return adc_y;
}
static void set_touch_x(int x) {
	adc_x = x;
	//printf("x:%d\n", x);
}
static void set_touch_y(int y) {
	adc_y = y;
	//printf("y:%d\n", y);
}

/*
 * INT_TC���жϷ������
 * ��������������ʱ�������Զ�(����) X/Y������ת��ģʽ��
 * �����������ɿ�ʱ������ȴ��ж�ģʽ���ٴεȴ�INT_TC�ж�
 */
static void Isr_Tc(void) {
	if (ADCDAT0 & 0x8000) {
		//printf("Stylus Up\n");
		close_timer();

		set_touch_y(-1);
		set_touch_x(-1);

		//TODO:
		GUI_TOUCH_Exec();
		GUI_TOUCH_Exec();
		wait_down_int();    /* ����"�ȴ��ж�ģʽ"���ȴ������������� */

	} else {
		//printf("Stylus Down\n");

		mode_auto_xy();     /* �����Զ�(����) X/Y������ת��ģʽ */

		/* ����λ[0]Ϊ1������A/Dת��
		 * ע�⣺ADCDLYΪ50000��PCLK = 50MHz��
		 *       Ҫ����(1/50MHz)*50000=1ms֮��ſ�ʼת��X����
		 *       �پ���1ms֮��ſ�ʼת��Y����
		 */
		ADCCON |= ADC_START;
	}
}
static void timer_handle() {
	//TODO:��ʱadcʧ��
	//printf("timer_handle\n");
	mode_auto_xy();
	ADCCON |= ADC_START;
}
/*
 * INT_ADC���жϷ������
 * A/Dת������ʱ�������ж�
 * �ȶ�ȡX��Y����ֵ���ٽ���ȴ��ж�ģʽ
 */
static void Isr_Adc(void) {
	// ����X��Y����ֵ
	set_touch_y((int)(ADCDAT0 & 0x3ff));
	set_touch_x((int)(ADCDAT1 & 0x3ff));

	//TODO:
	GUI_TOUCH_Exec();
	GUI_TOUCH_Exec();
	set_timer(60, timer_handle);
	/* �ж���S3C2410����S3C2440 */
	if ((GSTATUS1 == 0x32410000) || (GSTATUS1 == 0x32410002)) {
		// S3C2410
		wait_down_int();    /* ����"�ȴ��ж�ģʽ"���ȴ����������ɿ� */
	} else {
		// S3C2440
		wait_up_int();      /* ����"�ȴ��ж�ģʽ"���ȴ����������ɿ� */
	}
}

/*
 * ADC�����������жϷ������
 * ����INT_TC��INT_ADC�жϣ��ֱ�������ǵĴ������
 */
void AdcTsIntHandle(void) {
	if (SUBSRCPND & (1 << INT_TC)) {
		Isr_Tc();
	}
	if (SUBSRCPND & (1 << INT_ADC_S)) {
		Isr_Adc();
	}
}

/*
 * ��ʼ��������
 */
void init_Ts(void) {
	set_irq_handler(INT_ADC, AdcTsIntHandle); // ����ADC�жϷ������
	// ʹ��Ԥ��Ƶ���ܣ�����A/Dת������ʱ�� = PCLK/(49+1)
	ADCCON = PRESCALE_EN | PRSCVL(49);

	/* ������ʱʱ�� = (1/3.6864M)*50000 = 13.56ms
	 * �����´��������ٹ�13.56ms�Ų���
	 */
	ADCDLY = 50000;
	wait_down_int();    /* ����"�ȴ��ж�ģʽ"���ȴ������������� */
	INTMSK_set(INT_ADC);// ����ADC���ж�
	INTSUBMSK_set(INT_TC);// ����INT_TC�жϣ��������������»��ɿ�ʱ�����ж�
	INTSUBMSK_set(INT_ADC_S);// ����INT_ADC�жϣ���A/Dת������ʱ�����ж�
}
void close_Ts(void) {
	INTSUBMSK_clr(INT_TC);
	INTSUBMSK_clr(INT_ADC_S);
	INTMSK_clr(INT_ADC);
}

