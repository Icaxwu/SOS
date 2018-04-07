/*
 * FILE: lcddrv.h
 * ����LCD����������ɫ��ȵĵײ㺯���ӿ�
 */

#ifndef __LCDDRV_H__
#define __LCDDRV_H__

#include <sys/types.h>

#define LOWER21BITS(n)  ((n) & 0x1fffff)

#define BPPMODE_1BPP    0x8
#define BPPMODE_2BPP    0x9
#define BPPMODE_4BPP    0xA
#define BPPMODE_8BPP    0xB
#define BPPMODE_16BPP   0xC
#define BPPMODE_24BPP   0xD

#define LCDTYPE_TFT     0x3

#define ENVID_DISABLE   0
#define ENVID_ENABLE    1

#define FORMAT8BPP_5551 0
#define FORMAT8BPP_565  1

#define HSYNC_NORM      0
#define HSYNC_INV       1

#define VSYNC_NORM      0
#define VSYNC_INV       1

#define VDEN_NORM		0
#define VDEN_INV		1

#define BSWP            1
#define HWSWP           1

#define MODE_TFT_8BIT_480272 	 (0x410c)
#define MODE_TFT_16BIT_480272    (0x4110)

#define LCDFRAMEBUFFER 0x33c00000

/*
 * ��ʼ������LCD������
 */
void Lcd_Port_Init(void);


void Tft_Lcd_Init(int type);

/*
 * ���õ�ɫ��
 */
void Lcd_Palette8Bit_Init(void);

/*
 * ����LCD�������Ƿ�����ź�
 * ���������
 * onoff: 
 *      0 : �ر�
 *      1 : ��
 */
void Lcd_EnvidOnOff(int onoff);

/*
 * �����Ƿ����LCD��Դ�����ź�LCD_PWREN
 * ���������
 *     invpwren: 0 - LCD_PWREN��ЧʱΪ��������
 *               1 - LCD_PWREN��ЧʱΪ��ת����
 *     pwren:    0 - LCD_PWREN�����Ч
 *               1 - LCD_PWREN�����Ч
 */
void Lcd_PowerEnable(int invpwren, int pwren);

/*
 * ʹ����ʱ��ɫ��Ĵ��������ɫͼ��
 * ���������
 *     color: ��ɫֵ����ʽΪ0xRRGGBB
 */
void ClearScrWithTmpPlt(UINT32 color);

/*
 * ֹͣʹ����ʱ��ɫ��Ĵ���
 */
void DisableTmpPlt(void);

/*
 * �ı��ɫ��Ϊһ����ɫ
 * ���������
 *     color: ��ɫֵ����ʽΪ0xRRGGBB
 */
void ChangePalette(UINT32 color);


#endif /*__LCDDRV_H__*/
