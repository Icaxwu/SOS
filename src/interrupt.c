#include <interrupt.h>
#include "s3c24xx.h"
#include "ucos_ii.h"

void (*isr_handle_array[50])(void);

void enable_irq(void) {
	__asm__ volatile (
	    "mrs r4,cpsr\n"
	    "bic r4,r4,#0xc0\n"
	    "msr cpsr_c,r4\n"
	    :::"r4"
	);
}
void disable_irq(void) {
	__asm__ volatile (
	    "mrs r4,cpsr\n"
	    "orr r4,r4,#0xc0\n"
	    "msr cpsr_c,r4\n"
	    :::"r4"
	);
}

void INTMSK_set(enum INT_NUM num) {
	INTMSK &= ~(1 << num);
}
void INTMSK_clr(enum INT_NUM num) {
	INTMSK |= (1 << num);
}

void INTSUBMSK_set(enum INT_SUB_NUM num) {
	INTSUBMSK &= ~(1 << num);
}

void INTSUBMSK_clr(enum INT_SUB_NUM num) {
	INTSUBMSK |= (1 << num);
}

void Dummy_isr(void) {
	printf("IRQ HANDLE,ERROR!\n");
	while (1);
}

void init_irq(void) {
	for (int i = 0; i < sizeof(isr_handle_array) / sizeof(isr_handle_array[0]); i++) {
		isr_handle_array[i] = Dummy_isr;
	}
	INTMOD = 0x0;	      //�����ж϶���ΪIRQģʽ
	INTMSK = BIT_ALLMSK;  //�����������ж�
}

void IRQ_Handle(void) {
	unsigned long oft = INTOFFSET;
	//TODO:�ж�Ƕ��ʱ���жϿ�������
	OSIntEnter();
	//�����жϷ������
	isr_handle_array[oft]();
	//���ж�
	ClearPending(INTOFFSET);
	OSIntExit();
}

//TODO:add clr_irq_handler
//TODO:����
void set_irq_handler(int offset, int (*handler)(void)) {
	if (handler != 0)
		isr_handle_array[offset] = handler;
	else
		isr_handle_array[offset] = Dummy_isr;
}

//���ж�
void ClearPending(unsigned oft) {
	//TODO:��ͨ��
	if (oft == (1 << EINT4_7)) {
		//EINT4-7����IRQ4��ע��EINTPEND[3:0]����δ�ã�����Щλд��1���ܵ���δ֪���
		EINTPEND = 1 << 7;
	} else if (oft == (1 << EINT8_23)) {
		//EINT8_23����IRQ5
		EINTPEND = 1 << 11;
	}
	if (oft == INT_ADC) {
		if (SUBSRCPND & (1 << INT_ADC_S))
			SUBSRCPND |= 1 << INT_ADC_S;
		if (SUBSRCPND & (1 << INT_TC))
			SUBSRCPND |= 1 << INT_TC;
	}
	SRCPND = 1 << oft;
	INTPND = 1 << oft;
}
