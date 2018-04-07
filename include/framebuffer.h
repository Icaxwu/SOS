/*
 * FILE: framebuffer.h
 * ��framebuffer�ϻ��㡢���ߡ���ͬ��Բ�������ĺ����ӿ�
 */

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <sys/types.h>

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
void PutPixel(UINT32 x, UINT32 y, UINT16 color);
UINT16 GetPixel(UINT32 x, UINT32 y);
/* 
 * ����Ļ��ɵ�ɫ
 * ���������
 *     color: ��ɫֵ
 *         ����16BPP: color�ĸ�ʽΪ0xAARRGGBB (AA = ͸����),
 *     ��Ҫת��Ϊ5:6:5��ʽ
 *         ����8BPP: colorΪ��ɫ���е�����ֵ��
 *     ����ɫȡ���ڵ�ɫ���е���ֵ
 */
void ClearScr(UINT16 color);
void lcd_set_text_color(unsigned int color);
void lcd_set_background_color(unsigned int color);
#endif /*__FRAMEBUFFER_H__*/
