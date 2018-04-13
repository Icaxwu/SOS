#define MMU_FULL_ACCESS     (3)			/* ����Ȩ�� */
#define MMU_DOMAIN          (0)			/* �����ĸ��� */
#define MMU_SPECIAL         (1)			/* ������1 */
#define MMU_CACHEABLE       (1)			/* cacheable */
#define MMU_BUFFERABLE      (1)			/* bufferable */

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


#define MUM_TLB_BASE_ADDR	0x30000000

#define PHYSICAL_MEM_ADDR	0x30000000
#define VIRTUAL_MEM_ADDR	0x30000000
#define MEM_MAP_SIZE		0x4000000

#define PHYSICAL_IO_ADDR	0x48000000
#define VIRTUAL_IO_ADDR		0x48000000
#define IO_MAP_SIZE			0x18000000

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

/*
 * ����ҳ��
 */
void create_page_table(void) {
	unsigned long virtuladdr, physicaladdr;
	volatile SECTION *mmu_tlb_base = (volatile SECTION *)MUM_TLB_BASE_ADDR;

	/*
	 * Steppingstone����ʼ�����ַΪ0����һ���ֳ������ʼ���е�ַҲ��0��
	 * Ϊ���ڿ���MMU���������е�һ���ֵĳ���
	 * ��0��1M�������ַӳ�䵽ͬ���������ַ
	 */
	printf("���ڴ���Steppingstoneҳ����\n");
	{
		virtuladdr = 0;
		physicaladdr = 0;
		SECTION sec = {
			.base_address = physicaladdr >> MMU_SECTION_SHIFT,
			.blank0 = 0,
			.AP = MMU_FULL_ACCESS,
			.blank1 = 0,
			.domain = MMU_DOMAIN,
			.special = 1,
			.C = MMU_CACHEABLE,
			.B = MMU_BUFFERABLE,
			.type = MMU_SECTION,
		};
		
		mmu_tlb_base[virtuladdr >> 20] = sec;
	}


	/*
	 * 0x48000000������Ĵ�������ʼ�����ַ��
	 * �������ַ0x48000000��0x5FFFFFFFӳ�䵽�����ַ0x48000000��0x5FFFFFFF�ϣ�
	 */
	printf("���ڴ�������Ĵ���ҳ����\n");
	for (virtuladdr = VIRTUAL_IO_ADDR, physicaladdr = PHYSICAL_IO_ADDR;
			virtuladdr < VIRTUAL_IO_ADDR + IO_MAP_SIZE;
			virtuladdr += MMU_SECTION_SIZE, physicaladdr += MMU_SECTION_SIZE) {

		SECTION sec = {
			.base_address = physicaladdr >> MMU_SECTION_SHIFT,
			.blank0 = 0,
			.AP = MMU_FULL_ACCESS,
			.blank1 = 0,
			.domain = MMU_DOMAIN,
			.special = 1,
			.C = 0,//�ر�cache
			.B = 0,//�ر�buff
			.type = MMU_SECTION,
		};
		mmu_tlb_base[virtuladdr >> 20] = sec;
	}

	/*
	 * SDRAM�������ַ��Χ��0x30000000��0x33FFFFFF��
	 * �������ַ0x30000000��0x33FFFFFFӳ�䵽�����ַ0x30000000��0x33FFFFFF�ϣ�
	 * �ܹ�64M���漰64����������
	 */
	printf("���ڴ���SDRAMҳ����\n");
	for (virtuladdr = VIRTUAL_MEM_ADDR, physicaladdr = PHYSICAL_MEM_ADDR;
			virtuladdr < VIRTUAL_MEM_ADDR + MEM_MAP_SIZE;
			virtuladdr += MMU_SECTION_SIZE, physicaladdr += MMU_SECTION_SIZE) {
	
		SECTION sec = {
			.base_address = physicaladdr >> MMU_SECTION_SHIFT,
			.blank0 = 0,
			.AP = MMU_FULL_ACCESS,
			.blank1 = 0,
			.domain = MMU_DOMAIN,
			.special = 1,
			.C = MMU_CACHEABLE,
			.B = MMU_BUFFERABLE,
			.type = MMU_SECTION,
		};
		mmu_tlb_base[virtuladdr >> 20] = sec;
	}
	{
		//�ر�framebuffer��cache
		SECTION sec = {
			.base_address = 0x33c00000 >> MMU_SECTION_SHIFT,
			.blank0 = 0,
			.AP = MMU_FULL_ACCESS,
			.blank1 = 0,
			.domain = MMU_DOMAIN,
			.special = 1,
			.C = 0,
			.B = 0,
			.type = MMU_SECTION,
		};
		mmu_tlb_base[0x33c00000 >> 20] = sec;
	}
	{
		//����BANK4
		SECTION sec = {
			.base_address = 0x20000000 >> MMU_SECTION_SHIFT,
			.blank0 = 0,
			.AP = MMU_FULL_ACCESS,
			.blank1 = 0,
			.domain = MMU_DOMAIN,
			.special = 1,
			.C = 0,
			.B = 0,
			.type = MMU_SECTION,
		};
		mmu_tlb_base[0x20000000 >> 20] = sec;
		
	}
}

/*
 * ����MMU
 */
void mmu_init(void) {
	unsigned long ttb = 0x30000000;
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
	    "bic    r0, r0, #0x3000\n"          /* ..11 .... .... .... ���V��Iλ */
	    "bic    r0, r0, #0x0300\n"          /* .... ..11 .... .... ���R��Sλ */
	    "bic    r0, r0, #0x0087\n"          /* .... .... 1... .111 ���B/C/A/M */

	    /*
	     * ������Ҫ��λ
	     */
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
