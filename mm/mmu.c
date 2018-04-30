#include <global_config.h>
#include <assert.h>
#define MMU_FULL_ACCESS     (3)			/* ����Ȩ�� */
#define MMU_DOMAIN(x)       (x)			/* �����ĸ��� */
#define MMU_SPECIAL         (1)			/* ������1 */
#define MMU_CACHE_ENABLE    (1)			/* cache enable */
#define MMU_BUFFER_ENABLE   (1)			/* buffer enable */
#define MMU_CACHE_DISABLE   (0)			/* cache disable */
#define MMU_BUFFER_DISABLE  (0)			/* buffer disable */

#define MMU_COARSE_PAGE     (1)         /* ��ʾ���Ǵ�ҳ������ */
#define MMU_SECTION         (2)         /* ��ʾ���Ƕ������� */
#define MMU_FINE_PAGE		(3)         /* ��ʾ����ϸҳ������ */

#define MMU_LARGE_PAGE 	    (1)         /* ��ʾ���Ǵ�ҳ������ */
#define MMU_SMALL_PAGE		(2)         /* ��ʾ����Сҳ������ */
#define MMU_TINY_PAGE		(3)         /* ��ʾ���Ǽ�Сҳ������ */

#define MMU_COARSE_PAGE_SHIFT		(10)
#define MMU_SECTION_SHIFT			(20)
#define MMU_FINE_PAGE_SHIFT			(12)

#define MMU_LARGE_PAGE_SHIFT 	    (16)
#define MMU_SMALL_PAGE_SHIFT		(12)
#define MMU_TINY_PAGE_SHIFT			(10)

#define MMU_SECTION_SIZE	(1<<MMU_SECTION_SHIFT)
#define MMU_LARGE_PAGE_SIZE	(1<<MMU_LARGE_PAGE_SHIFT)
#define MMU_SMALL_PAGE_SIZE	(1<<MMU_SMALL_PAGE_SHIFT)
#define MMU_TINY_PAGE_SIZE 	(1<<MMU_TINY_PAGE_SHIFT)


typedef struct COARSE_PAGE {
	unsigned int type: 2;
	unsigned int blank1: 2;
	unsigned int special: 1;
	unsigned int domain: 4;
	unsigned int blank0: 1;
	unsigned int base_address: 22;
} COARSE_PAGE;

typedef struct SECTION {
	unsigned int type: 2;
	unsigned int B: 1;
	unsigned int C: 1;
	unsigned int special: 1;
	unsigned int domain: 4;
	unsigned int blank1: 1;
	unsigned int AP: 2;
	unsigned int blank0: 8;
	unsigned int base_address: 12;
} SECTION;

typedef struct FINE_PAGE {
	unsigned int type: 2;
	unsigned int blank1: 2;
	unsigned int special: 1;
	unsigned int domain: 4;
	unsigned int blank0: 3;
	unsigned int base_address: 20;
} FINE_PAGE;

typedef struct LARGE_PAGE {
	unsigned int type: 2;
	unsigned int B: 1;
	unsigned int C: 1;
	unsigned int AP0: 2;
	unsigned int AP1: 2;
	unsigned int AP2: 2;
	unsigned int AP3: 2;
	unsigned int blank0: 4;
	unsigned int base_address: 16;
} LARGE_PAGE;

typedef struct SMALL_PAGE {
	unsigned int type: 2;
	unsigned int B: 1;
	unsigned int C: 1;
	unsigned int AP0: 2;
	unsigned int AP1: 2;
	unsigned int AP2: 2;
	unsigned int AP3: 2;
	unsigned int base_address: 20;
} SMALL_PAGE;

typedef struct TINY_PAGE {
	unsigned int type: 2;
	unsigned int B: 1;
	unsigned int C: 1;
	unsigned int AP: 2;
	unsigned int blank0: 4;
	unsigned int base_address: 22;
} TINY_PAGE;

void __set_SECTION(unsigned long virtuladdr, unsigned long physicaladdr, unsigned int AP, unsigned int domain, unsigned int C, unsigned int B){
	volatile SECTION *mmu_tlb_base = (volatile SECTION *)MUM_TLB_BASE_ADDR;
	SECTION sec = {
		.base_address = physicaladdr >> MMU_SECTION_SHIFT,
		.blank0 = 0,
		.AP = AP,
		.blank1 = 0,
		.domain = domain,
		.special = 1,
		.C = C,
		.B = B,
		.type = MMU_SECTION,
	};
	mmu_tlb_base[virtuladdr >> 20] = sec;
}
void set_SECTION(unsigned long virtuladdr, unsigned long physicaladdr, unsigned long count, unsigned int AP, unsigned int domain, unsigned int C, unsigned int B){
	unsigned long vaddr = virtuladdr, paddr = physicaladdr;
	assert((vaddr&0xfffff)==0);
	assert((paddr&0xfffff)==0);
	assert(count>0);
	for (int i = 0; i < count; i++) {
		assert(vaddr>=virtuladdr);
		assert(paddr>=physicaladdr);
		__set_SECTION(vaddr, paddr, AP, domain, C, B);
		vaddr += MMU_SECTION_SIZE;
		paddr += MMU_SECTION_SIZE;
	}
}
struct mem_map{
	unsigned long virtuladdr;
	unsigned long physicaladdr;
	unsigned long size;
	unsigned int AP;
	unsigned int domain;
	unsigned int C;
	unsigned int B;
};
/*
 * ����ҳ��
 */
void create_page_table(void) {
	unsigned long virtuladdr, physicaladdr;
	/*
	 * Steppingstone����ʼ�����ַΪ0����һ���ֳ������ʼ���е�ַҲ��0��
	 * Ϊ���ڿ���MMU���������е�һ���ֵĳ���
	 * ��0��1M�������ַӳ�䵽ͬ���������ַ
	 */
	printf("���ڴ���Steppingstoneҳ����\n");
	set_SECTION(0, 0, 1, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_ENABLE, MMU_BUFFER_ENABLE);

	/*
	 * 0x48000000������Ĵ�������ʼ�����ַ��
	 * �������ַ0x48000000��0x5FFFFFFFӳ�䵽�����ַ0x48000000��0x5FFFFFFF�ϣ�
	 */
	printf("���ڴ�������Ĵ���ҳ����\n");
	set_SECTION(VIRTUAL_IO_ADDR, VIRTUAL_IO_ADDR, IO_MAP_SIZE >> MMU_SECTION_SHIFT, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_DISABLE, MMU_BUFFER_DISABLE);
	/*
	 * SDRAM�������ַ��Χ��0x30000000��0x33FFFFFF��
	 * �������ַ0x30000000��0x33FFFFFFӳ�䵽�����ַ0x30000000��0x33FFFFFF�ϣ�
	 * �ܹ�64M���漰64����������
	 */
	printf("���ڴ���SDRAMҳ����\n");
	set_SECTION(VIRTUAL_MEM_ADDR, PHYSICAL_MEM_ADDR, MEM_MAP_SIZE >> MMU_SECTION_SHIFT, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_ENABLE, MMU_BUFFER_ENABLE);
	//�ر�framebuffer��cache
	//set_SECTION(0x33c00000, 0x33c00000, 1, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_DISABLE, MMU_BUFFER_DISABLE);
	//����BANK4
	//TODO:����ȫ
	set_SECTION(0x20000000, 0x20000000, 1, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_DISABLE, MMU_BUFFER_DISABLE);
	set_SECTION(0xfff00000, 0x33f00000, 1, MMU_FULL_ACCESS, MMU_DOMAIN(0), MMU_CACHE_ENABLE, MMU_BUFFER_ENABLE);
}

/*
 * ����MMU
 */
void mmu_init(void) {
	unsigned long ttb = (unsigned long)MUM_TLB_BASE_ADDR;;
	//����ҳ��
	printf("���ڴ���ҳ��\n");
	create_page_table();
	printf("ҳ�������\n");
	
	__asm__ (
	    "mov    r0, #0\n"
	    "mcr    p15, 0, r0, c7, c7, 0\n"    /* ʹ��ЧICaches��DCaches */
	    "mcr    p15, 0, r0, c7, c10, 4\n"   /* drain write buffer on v4 */
	    "mcr    p15, 0, r0, c8, c7, 0\n"    /* ʹ��Чָ�����TLB */

	    "mov    r4, %0\n"                   /* r4 = ҳ���ַ */
	    "mcr    p15, 0, r4, c2, c0, 0\n"    /* ����ҳ���ַ�Ĵ��� */

	    "mvn    r0, #0\n"
	    "mcr    p15, 0, r0, c3, c0, 0\n"    /* ����ʿ��ƼĴ�����Ϊ0xFFFFFFFF��
			* ������Ȩ�޼��
			*/
	    /*
	     * ���ڿ��ƼĴ������ȶ�����ֵ������������޸ĸ���Ȥ��λ��
	     * Ȼ����д��
	     */
	    "mrc    p15, 0, r0, c1, c0, 0\n"    /* �������ƼĴ�����ֵ */

	    /* ���ƼĴ����ĵ�16λ����Ϊ��.RVI ..RS B... .CAM
	     * R : ��ʾ����Cache�е���Ŀʱʹ�õ��㷨��
	     *     0 = Random replacement��1 = Round robin replacement
	     * V : ��ʾ�쳣���������ڵ�λ�ã�
	     *     0 = Low addresses = 0x00000000��1 = High addresses = 0xFFFF0000
	     * I : 0 = �ر�ICaches��1 = ����ICaches
	     * R��S : ������ҳ���е�������һ��ȷ���ڴ�ķ���Ȩ��
	     * B : 0 = CPUΪС�ֽ���1 = CPUΪ���ֽ���
	     * C : 0 = �ر�DCaches��1 = ����DCaches
	     * A : 0 = ���ݷ���ʱ�����е�ַ�����飻1 = ���ݷ���ʱ���е�ַ������
	     * M : 0 = �ر�MMU��1 = ����MMU
	     */

	    /*
	     * ���������Ҫ��λ����������Ҫ��������������
	     */
	    /* .RVI ..RS B... .CAM */
	    "bic    r0, r0, #0x1000\n"          /* ...1 .... .... .... ���Iλ */
	    "bic    r0, r0, #0x0300\n"          /* .... ..11 .... .... ���R��Sλ */
	    "bic    r0, r0, #0x0087\n"          /* .... .... 1... .111 ���B/C/A/M */

	    /*
	     * ������Ҫ��λ
	     */
		"orr    r0, r0, #0x2000\n"          /* ..1. .... .... .... �ߵ�ַ�ж������� */
	    "orr    r0, r0, #0x0002\n"          /* .... .... .... ..1. ���������� */
	    "orr    r0, r0, #0x0004\n"          /* .... .... .... .1.. ����DCaches */
	    "orr    r0, r0, #0x1000\n"          /* ...1 .... .... .... ����ICaches */
	    "orr    r0, r0, #0x0001\n"          /* .... .... .... ...1 ʹ��MMU */

	    "mcr    p15, 0, r0, c1, c0, 0\n"    /* ���޸ĵ�ֵд����ƼĴ��� */
	    : /* ����� */
	    : "r" (ttb)
	    : "r0", "r4"
	);
}
void mmu_update(unsigned long ttb){
	__asm__ (
	    "mov    r0, #0\n"
	    "mcr    p15, 0, r0, c7, c7, 0\n"    /* ʹ��ЧICaches��DCaches */
	    "mcr    p15, 0, r0, c7, c10, 4\n"   /* drain write buffer on v4 */
	    "mov    r4, %0\n"                   /* r4 = ҳ���ַ */
	    "mcr    p15, 0, r4, c2, c0, 0\n"    /* ����ҳ���ַ�Ĵ��� */
		"mcr    p15, 0, r0, c8, c7, 0\n"    /* ʹ��Чָ�����TLB */
	    : /* ����� */
	    : "r" (ttb)
	    : "r0", "r4"
	);
}
