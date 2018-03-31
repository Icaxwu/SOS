/*
 * FILE: framebuffer.c
 * ʵ����framebuffer�ϻ��㡢���ߡ���ͬ��Բ�������ĺ���
 */
#include <ctype.h>
#include "framebuffer.h"

extern unsigned int fb_base_addr;
extern unsigned int bpp;
extern unsigned int xsize;
extern unsigned int ysize;
static unsigned int text_color=0x0;
static unsigned int background_color=0xffffff;
/* 
 * ����
 * ���������
 *     x��y : ��������
 *     color: ��ɫֵ
 *         ����16BPP: color�ĸ�ʽΪ0xAARRGGBB (AA = ͸����),
 *     ��Ҫת��Ϊ5:6:5��ʽ
 *         ����8BPP: colorΪ��ɫ���е�����ֵ��
 *     ����ɫȡ���ڵ�ɫ���е���ֵ
 */
void PutPixel(UINT32 x, UINT32 y, UINT32 color)
{
	UINT16 *addr = (UINT16 *)fb_base_addr + (y * xsize + x);
	*addr = (UINT16) color;
}
UINT32 GetPixel(UINT32 x, UINT32 y)
{
	UINT16 *addr = (UINT16 *)fb_base_addr + (y * xsize + x);
	return *addr;
}

/* 
 * ����Ļ��ɵ�ɫ
 * ���������
 *     color: ��ɫֵ
 *         ����16BPP: color�ĸ�ʽΪ0xAARRGGBB (AA = ͸����),
 *     ��Ҫת��Ϊ5:6:5��ʽ
 *         ����8BPP: colorΪ��ɫ���е�����ֵ��
 *     ����ɫȡ���ڵ�ɫ���е���ֵ
 */
void ClearScr(UINT32 color)
{   
    UINT32 x,y;
    
    for (y = 0; y < ysize; y++)
        for (x = 0; x < xsize; x++)
            PutPixel(x, y, color);
}
#define FONTDATAMAX 2048

void lcd_set_text_color(unsigned int color){
	text_color=color;
}
void lcd_set_background_color(unsigned int color){
	background_color=color;
}