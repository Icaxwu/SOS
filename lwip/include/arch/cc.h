#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__


/* �������ͷ�ļ� */
#include <string.h>
#include <assert.h>

#define BYTE_ORDER LITTLE_ENDIAN
#define	LWIP_PROVIDE_ERRNO
/* �ṩ��׼������� */
#define LWIP_PROVIDE_ERRNO


/* ������������޹ص��������� */
typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   int     u32_t;
typedef signed     int     s32_t;


/* ����ָ������ */
typedef u32_t mem_ptr_t;


/* �ṹ�� */
#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END


/* ƽ̨��������Ͷ��� */
#define LWIP_DEBUG
#define LWIP_PLATFORM_DIAG(x)	 printf x
#define LWIP_PLATFORM_ASSERT(x)  assert(x)

#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

/* �ٽ����ı���( ʹ��uCOS-II�ĵ������ٽ紦��ʽ ) */
#include "os_cpu.h"
#define SYS_ARCH_DECL_PROTECT(x) u32_t cpu_sr
#define SYS_ARCH_PROTECT(x)      OS_ENTER_CRITICAL()
#define SYS_ARCH_UNPROTECT(x)    OS_EXIT_CRITICAL()

extern unsigned int sys_now(void);
#define LWIP_RAND() sys_now()

#endif /* __ARCH_CC_H__ */
