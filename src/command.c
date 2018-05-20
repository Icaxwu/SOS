#include <sys/types.h>
#include <assert.h>
#include <usb/2440usb.h>
#include "command.h"
#include "ff.h"

#define CMD_MAX_CMD_NUM 50
#define CMD_MAXARGS 10
extern cmd_table *ct_list[];

DIR  dirobj;               // current work dir fof cd
void FileAttr(BYTE attr, char *p) {
	if ((attr & 0x10) == 0x10) {
		sprintf(p, "%5s", "dir :");
	} else {
		sprintf(p, "%5s", "file:");
	}

}
CMD_DEFINE(ls, "ls", "ls") {
	char p_cmd[16], p_arg[32];
	char *p_path, *pfname;
	FRESULT read_res, dir_res;
	DIR  tempdir;
	FILINFO tempfinfo;
	char fdesp[8];
	FRESULT res1 = f_opendir(&dirobj, "/");

	if (res1 == FR_OK)
		if (argc == 1) {
			p_path = "/";
		} else if (argc == 2) {
			p_path = argv[1];
		} else {
			return 1;
		}
	dir_res = f_opendir(&tempdir, p_path);
	if (dir_res != FR_OK) {
		printf("f_opendir failed,path:%s does not exist\n\r", p_path);
		return 1;
	}
	for (;;) {
		read_res = f_readdir(&tempdir, &tempfinfo);
		if ((read_res != FR_OK) || (tempfinfo.fname[0] == 0)) {
			break;
		} else if (tempfinfo.fname[0] == '.') {
			continue;
		} else {
			pfname = tempfinfo.fname;
			FileAttr((tempfinfo.fattrib), fdesp);
			printf("%s   %-15s  %8dbyte\n\r", fdesp, pfname, tempfinfo.fsize);
		}
	}
	return 0;
}

CMD_DEFINE(wav, "wav", "wav") {
	if (argc != 2)
		return 1;
	read_wav_file(argv[1]);
	return 0;
}

CMD_DEFINE(mp3, "mp3", "mp3") {
	if (argc != 2)
		return 1;
	AudioDecode(argv[1]);
	return 0;
}

CMD_DEFINE(nes, "nes", "nes") {
	if (argc != 2)
		return 1;
	if (InfoNES_Load(argv[1])) {
		printf("\033[31m��Ϸ����ʧ��\033[0m\n");
		return 1;
	}
	InfoNES_Main();
	return 0;
}
CMD_DEFINE(usbdebug, "usbdebug", "usbdebug") {
	#if USB_DEBUG == 1
	DbgPrintf("show");
	#endif
	return 0;
}
CMD_DEFINE(usbmouse, "usbmouse", "usbmouse") {
#if 1
	while (1) {
		U8 Buf[4]={0,0,0,0};
		switch(getc()){
			case 'a':
				Buf[1] = -1;	//����һ�������ƶ�һ����λ��
				break;
			case 'd':
				Buf[1] = 1;		//����һ�������ƶ�һ����λ��
				break;
			case 'w':
				Buf[2] = -1;	//����һ�������ƶ�һ����λ��
				break;
			case 's':
				Buf[2] = 1;		//����һ�������ƶ�һ����λ��
				break;
			case 'j':
				Buf[0] |= 0x01;	//D0Ϊ������
				break;
			case 'k':
				Buf[0] |= 0x02;	//D1Ϊ����Ҽ�
				break;
			case 'q':
			case 'Q':
				return 0;
				break;
			default:
				break;
		}
		usb_send_init(EP1, Buf, sizeof(Buf));
		usb_send_message(EP1);
	}
#endif
	return 0;
}
CMD_DEFINE(delay_u, "delay_u", "delay_u") {
	if (argc < 2)
		return 1;
	if(argc == 2) {
		delay_u(simple_strtoul(argv[1], NULL, 10));
	}else if(argc == 3) {
		for(int i = 0; i < simple_strtoul(argv[1], NULL, 10); i++){
			delay_u(simple_strtoul(argv[2], NULL, 10));
		}
	}else{
		return 1;
	}
	return 0;
}
CMD_DEFINE(usbslave,
		   "usbslave - get file from host(PC)",
		   "[loadAddress] [wait] \n"
		   "\"wait\" is 0 or 1, 0 means for return immediately, not waits for the finish of transferring") {
#if 0
	//TODO:��ý��ļ����ص��ļ�ϵͳ��
	extern int download_run;
	extern volatile U32 dwUSBBufBase;
	extern volatile U32 dwUSBBufSize;

	int wait = 1;
#define BUF_SIZE (1024*1024)
	/* download_runΪ1ʱ��ʾ���ļ�������USB Host���͹���dnwָ����λ��
	 * download_runΪ0ʱ��ʾ���ļ������ڲ���argv[2]ָ����λ��
	 * Ҫ���س����ڴ棬Ȼ��ֱ������ʱ��Ҫ����download_run=1����Ҳ������������ֵ�����
	 */
	//����0x3000000�����ҳ������download_run = 0ȷ�����ص�ַ��ȷ������������λ�����õĵ�ַ
	download_run = 0;//Ĭ������λ��������ַ�ʹ�С
	if (argc == 2) {
		//dwUSBBufBase = kmalloc(BUF_SIZE);
		dwUSBBufBase = 0x30a00000;
		if (!dwUSBBufBase) {
			printf("malloc memory error!\n");
			return 1;
		}
		wait = (int)simple_strtoul(argv[1], NULL, 16);
		dwUSBBufSize = BUF_SIZE;
	} else {
		return 1;
	}
	usb_init_slave();
	int size = usb_receive(dwUSBBufBase, dwUSBBufSize, wait);
	assert(size > 0 && size <= BUF_SIZE);
#endif
	return 0;
}
CMD_DEFINE(help, "help", "help") {
	for (int i = 0; ct_list[i] != NULL; i++) {
		printf("%-20s:\t-%s\n", ct_list[i]->name, ct_list[i]->usage);
	}
	return 0;
}
#define CMD_ENTRY(x) & ct_##x
cmd_table *ct_list[] = {
	CMD_ENTRY(help),
	CMD_ENTRY(ls),
	CMD_ENTRY(wav),
	CMD_ENTRY(mp3),
	CMD_ENTRY(nes),
	CMD_ENTRY(usbslave),
	CMD_ENTRY(delay_u),
	CMD_ENTRY(usbdebug),
	CMD_ENTRY(usbmouse),
	NULL
};
cmd_table *search_cmd(char *name) {
	for (int i = 0; ct_list[i] != NULL; i++) {
		if (strcmp(ct_list[i]->name, name) == 0) {
			return ct_list[i];
		}
	}
	return NULL;
}
int run_command(char *cmd, int flag) {
	char *str = cmd;
	char *argv[CMD_MAXARGS + 1] = {0};	/* NULL terminated	*/
	int argc = 0;
	int cmdlen = strlen(cmd);
	for (int i = 0; i < cmdlen; i++) {
		if (str[i] != ' ' && i != 0) {
			continue;
		} else {
			while (str[i] == ' ') {
				str[i] = '\0';
				i++;
			}
			if (i < cmdlen) {
				argv[argc] = &str[i];
				argc++;
				if (argc == CMD_MAXARGS + 1)
					return -1;
			} else
				break;
		}
	}
	cmd_table *pct = search_cmd(argv[0]);
	if (pct) {
		pct->cmd(pct, argc, argv);
	} else {
		printf("%s:command not found\n", argv[0]);
		return 0;
	}
	return 1;
}
static int get_str(char *buf, int len) {
	int i;
	for (i = 0; i < len - 1; i++) {
		char c = getc();
		//xshell �س�����\r\n
		if (c == '\r') {
			getc();

			if (i == 0) {
				return -1;
			} else {
				printf("\n");
				buf[i] = '\0';
				break;
			}
		} else if (c == '\b') {
			if (i > 0) { //ǰ�����ַ�
				putc(c);
				i = i - 2;
			} else { //ǰ��û���ַ�
				i = i - 1;
			}
		} else {
			putc(c);
			buf[i] = c;
		}
	}
	return 1;
}
int cmd_loop() {
	char buf[100] = {0};
	while (1) {
		printf("\nOS>");
		if (get_str(buf, 100) == -1)
			continue;
		run_command(buf, 1);
	}
}
