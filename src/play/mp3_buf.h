#ifndef __MP3_BUF_H__
#define __MP3_BUF_H__
#define FRAMEBUF_NUM 20
#define OUTPUT_BUFFER_SIZE	2304	//һ֡��1152����
void framebuf_init();
void framebuf_exit();
void framebuf_start();
unsigned short *get_next_framebuf();
unsigned short *get_current_framebuf();

#endif
