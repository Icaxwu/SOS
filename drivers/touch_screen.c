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
void wait_down_int() {
	ADCTSC = DOWN_INT | XP_PULL_UP_EN | XP_AIN | XM_HIZ | YP_AIN | YM_GND |  XP_PST(WAIT_INT_MODE);
}
/* ����"�ȴ��ж�ģʽ"���ȴ����������ɿ� */
void wait_up_int()   {
	ADCTSC = UP_INT | XP_PULL_UP_EN | XP_AIN | XM_HIZ | YP_AIN | YM_GND | XP_PST(WAIT_INT_MODE);
}

/* �����Զ�(����) X/Y������ת��ģʽ */
void mode_auto_xy() {
	ADCTSC = CONVERT_AUTO | XP_PULL_UP_DIS | XP_PST(NOP_MODE);
}


static int g_ts_x = 0;
static int g_ts_y = 0;
static int g_ts_pressure = 0;
volatile int g_ts_data_valid = 0;
static int test_x_array[16];
static int test_y_array[16];


int get_touch_x() {
	return g_ts_x;
}
int get_touch_y() {
	return g_ts_y;
}

void report_ts_xy(int x, int y, int pressure) {
	//printf("x = %08d, y = %08d\n", x, y);
	if (g_ts_data_valid == 0) {
		g_ts_x = x;
		g_ts_y = y;
		g_ts_pressure = pressure;
		g_ts_data_valid = 1;
		//TODO:
		//GUI_TOUCH_Exec();
		//GUI_TOUCH_Exec();
	}
}

void ts_read_raw(int *px, int *py, int *ppressure) {
	while (g_ts_data_valid == 0);
	*px = g_ts_x;
	*py = g_ts_y;
	*ppressure = g_ts_pressure;
	g_ts_data_valid = 0;
}

int ts_read_raw_asyn(int *px, int *py, int *ppressure) {
	if (g_ts_data_valid == 0)
		return -1;
	*px = g_ts_x;
	*py = g_ts_y;
	*ppressure = g_ts_pressure;
	g_ts_data_valid = 0;
	return 0;
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
		report_ts_xy(-1, -1, 0);
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
	mode_auto_xy();
	ADCCON |= ADC_START;
}

#define ABS(x) (((x)>0)?(x):(-(x)))
/*
 * INT_ADC���жϷ������
 * A/Dת������ʱ�������ж�
 * �ȶ�ȡX��Y����ֵ���ٽ���ȴ��ж�ģʽ
 */
static void Isr_Adc(void) {
	int x = ADCDAT0 & 0x3ff;
	int y = ADCDAT1 & 0x3ff;

	static int adc_cnt = 0;
	static int adc_x = 0;
	static int adc_y = 0;
	static int pre_adc_x = 0;
	static int pre_adc_y = 0;
	/* ����ADC�ж�ʱ, TS����"�Զ�����ģʽ" */

	/* ֻ����"�ȴ��ж�ģʽ"�²ſ���ʹ��ADCDAT0'BIT 15���жϴ�����״̬ */
	wait_up_int();      /* ����"�ȴ��ж�ģʽ"���ȴ����������ɿ� */

	//TODO
	udelay(5);
	if (ADCDAT0 & 0x8000) {
		adc_cnt = 0;
		adc_x = 0;
		adc_y = 0;
		close_timer();
		report_ts_xy(-1, -1, 0);
		wait_down_int();
		return;
	}
#if 0
	//abs > 1000 ,���̫С����ѭ��������������������
	if ((adc_cnt > 0)
			&& ((ABS(adc_x / adc_cnt - x) > 1000) || (ABS(adc_y / adc_cnt - y) > 1000))) {
		mode_auto_xy();/* ����"�Զ�����"ģʽ */
		ADCCON |= (1 << 0);/* ����ADC */
		return;
	}
#endif
	/* ��1������ADC��:
	 *   a. Ҫ��������N��, ���N������, ��ƽ��ֵ���ϱ�
	 *   b. �õ�N�����ݺ�, ������TIMER
	 */

	adc_x += x;
	adc_y += y;

	test_x_array[adc_cnt] = pre_adc_x = x;
	test_y_array[adc_cnt] = pre_adc_y = y;

	adc_cnt++;

	if (adc_cnt == 16) {
		adc_x >>= 4;
		adc_y >>= 4;
		report_ts_xy(adc_x, adc_y, 1);

		adc_cnt = 0;
		adc_x = 0;
		adc_y = 0;

		/* ������ʱ�����ٴζ�ȡ���� */
		/* ������TS����"�ȴ��ж�ģʽ" */
		wait_up_int();
		set_timer(10, timer_handle);
	} else {
		mode_auto_xy();/* ����"�Զ�����"ģʽ */
		ADCCON |= (1 << 0);/* ����ADC */
	}
}

/*
 * ADC�����������жϷ������
 * ����INT_TC��INT_ADC�жϣ��ֱ�������ǵĴ������
 */
void AdcTsIntHandle(void) {
	if (get_subsrcpnd() & (1 << INT_TC)) {
		Isr_Tc();
	}
	if (get_subsrcpnd() & (1 << INT_ADC_S)) {
		Isr_Adc();
	}
}

/*
 * ��ʼ��������
 */
void init_Ts(void) {
	// ʹ��Ԥ��Ƶ���ܣ�����A/Dת������ʱ�� = PCLK/(49+1)
	ADCCON = PRESCALE_EN | PRSCVL(49);

	/* ������ʱʱ�� = (1/3.6864M)*50000 = 13.56ms
	 * �����´��������ٹ�13.56ms�Ų���
	 */
	ADCDLY = 50000;
	set_subint(INT_TC);// ����INT_TC�жϣ��������������»��ɿ�ʱ�����ж�
	set_subint(INT_ADC_S);// ����INT_ADC�жϣ���A/Dת������ʱ�����ж�
	request_irq(INT_ADC, AdcTsIntHandle); // ����ADC�жϷ������
	wait_down_int();    /* ����"�ȴ��ж�ģʽ"���ȴ������������� */
}
void close_Ts(void) {
	clr_subint(INT_TC);
	clr_subint(INT_ADC_S);
	free_irq(INT_ADC);
}

