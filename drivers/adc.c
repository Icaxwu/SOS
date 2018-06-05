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
static void adc_init(int channel) {
	/* [15] : ECFLG,  1 = End of A/D conversion
	 * [14] : PRSCEN, 1 = A/D converter prescaler enable
	 * [13:6]: PRSCVL, adc clk = PCLK / (PRSCVL + 1)
	 * [5:3] : SEL_MUX, 000 = AIN 0
	 * [2]   : STDBM
	 * [0]   : 1 = A/D conversion starts and this bit is cleared after the startup.
	 */
	// ѡ��ģ��ͨ����ʹ��Ԥ��Ƶ���ܣ�����A/Dת������ʱ�� = PCLK/(49+1)
	ADCCON = PRESCALE_EN | PRSCVL(49) | ADC_INPUT(channel);
	ADCDLY = 0xff;
	// ���λ[2]����Ϊ��ͨת��ģʽ
	ADCTSC &= ~(1 << 2);
}

int adc_read(int channel) {
	adc_init(channel);
	// ����λ[0]Ϊ1������A/Dת��
	ADCCON |= ADC_START;
	// ��A/Dת��������ʼʱ��λ[0]���Զ���0
	while (ADCCON & ADC_START);
	// ���λ[15]������Ϊ1ʱ��ʾת������
	while (!(ADCCON & ADC_ENDCVT));
	// ��ȡ����
	return (ADCDAT0 & 0x3ff);
}

/*
 * ͨ��A/Dת���������ɱ�������ĵ�ѹֵ
 */
void get_adc(int *vol, int *t, int ch) {
	float v = ((float)adc_read(ch) * 3.3) / 1024.0; // �����ѹֵ
	*vol = v;
	*t = (v - (int)v) * 1000;   // ����С������, �������е�printf�޷���ӡ������
}
