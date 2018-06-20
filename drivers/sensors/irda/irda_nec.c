#include "irda_raw.h"

/*
 * �ӻ��ͻ������л����������,
 * �����ó�address, data
 */

#define DURATION_BASE  563
#define DURATION_DELTA (DURATION_BASE/2)

#define DURATION_HEAD_LOW    (16*DURATION_BASE)
#define DURATION_HEAD_HIGH   (8*DURATION_BASE)
#define DURATION_REPEAT_HIGH (4*DURATION_BASE)
#define DURATION_DATA_LOW    (1*DURATION_BASE)
#define DURATION_DATA1_HIGH  (3*DURATION_BASE)
#define DURATION_DATA0_HIGH  (1*DURATION_BASE)
#define DURATION_END_LOW     (1*DURATION_BASE)

static int duration_in_margin(int duration, int us) {
	if ((duration > (us - DURATION_DELTA)) && (duration < us + DURATION_DELTA))
		return 1;
	else
		return 0;
}

/*
 * ����ֵ: 0-�õ�����, 1-�õ��ظ���, -1 : ʧ��
 */
int irda_nec_read(int *address, int *data) {
	irda_raw_event event;

	unsigned int val = 0;
	unsigned char byte[4];

	if (ir_event_get(&event))
		return -1;
	/* �������� */
	/* 1. �ж��Ƿ�Ϊ9MS�ĵ����� */
	if (!(event.pol == 0 && duration_in_margin(event.duration, DURATION_HEAD_LOW)))
		return -1; /* ��Ч����δ��ʼ */
	/* 2. ����һ������������ */
	if (ir_event_get_timeout(&event, 10000))
		return -1;
	/* 3. �ж����Ƿ�4.5ms�ĸ�������� 2.25ms�ĸ����� */
	if (event.pol == 1 && duration_in_margin(event.duration, DURATION_REPEAT_HIGH)) {/* 2.25ms�ĸ����� */
		if (ir_event_get_timeout(&event, 1000))
			return -1;
		/* �������� */
		if (!(event.pol == 0 && duration_in_margin(event.duration, DURATION_END_LOW)))
			return -1;  
		return 1;  /* �ظ��� */
	}else if (!(event.pol == 1 && duration_in_margin(event.duration, DURATION_HEAD_HIGH))) {
		return -1;
	}

	/* 4.5ms�ĸ����� */
	/* 4. �ظ�����32λ���� */
	for (int i = 0; i < 32; i++) {
		/* 5. ��0.56ms�ĵ����� */
		if (ir_event_get_timeout(&event, 10000))
			return -1;
		if (!(event.pol == 0 && duration_in_margin(event.duration, DURATION_DATA_LOW)))
			return -1;
		/* 6. ����һ������, �ж����� 0.56ms/1.68ms�ĸ����� */
		if (ir_event_get_timeout(&event, 10000))
			return -1;
		if (event.pol == 1 && duration_in_margin(event.duration, DURATION_DATA1_HIGH)) {
			val |= (1 << i);/* bit 1 */
		} else if (event.pol == 1 && duration_in_margin(event.duration, DURATION_DATA0_HIGH)) {
			/* bit 0 */
		} else {
			return -1;
		}
	}
	if (ir_event_get_timeout(&event, 1000))
		return -1;
	/* �������� */
	if (!(event.pol == 0 && duration_in_margin(event.duration, DURATION_END_LOW)))
		return -1; 
	/* 7. �õ���32λ����, �ж������Ƿ���ȷ */
	*(unsigned int *)&byte = val;
	byte[1] = ~byte[1];
	byte[3] = ~byte[3];

	if (byte[0] != byte[1]) {
		/* ��Щң��������ȫ����NEC�淶 */
		//return -1;
	}
	if (byte[2] != byte[3])
		return -1;
	*address = byte[0];
	*data    = byte[2];
	return 0;
}

void irda_nec_test(void) {
	int address;
	int data;
	int ret;

	irda_init();

	while (!serial_getc_async()) {
		ret = irda_nec_read(&address, &data);
		if (ret == 0) {
			printf("irda_nec_read: address = 0x%x, data = 0x%x\n", address, data);
		} else if (ret == 1) {
			printf("irda_nec_read: repeat code\n");
		}
	}
	irda_exit();
}

