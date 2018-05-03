/*
 * 1,֧��16λ������/������MP3�Ľ���
 * 2,֧��CBR/VBR��ʽMP3����
 * 3,֧��ID3V1��ID3V2��ǩ����
 * 4,֧�����б�����(MP3�����320Kbps)����
 */
#include <string.h>
#include <assert.h>
#include "mp3common.h"
#include "mp3.h"
#include "ff.h"


#define AUDIO_MIN(x,y)	((x)<(y)? (x):(y))

MP3CTRL * mp3ctrl;	//mp3���ƽṹ��
volatile u8 mp3transferend = 0;	//i2s������ɱ�־
volatile u8 mp3witchbuf = 0;	//i2sbufxָʾ��־


/*
 * ����ID3V1
 * buf:�������ݻ�����(��С�̶���128�ֽ�)
 * pctrl:MP3������
 * ����ֵ:=0,����,����,��ȡʧ��
 */
u8 mp3_id3v1_decode(u8* buf, MP3CTRL *pctrl) {
	ID3V1_Tag *id3v1tag;
	id3v1tag = (ID3V1_Tag*)buf;
	if (strncmp("TAG", (char*)id3v1tag->id, 3) == 0) {
		//��MP3 ID3V1 TAG
		if (id3v1tag->title[0])
			strncpy((char*)pctrl->title, (char*)id3v1tag->title, 30);
		if (id3v1tag->artist[0])
			strncpy((char*)pctrl->artist, (char*)id3v1tag->artist, 30);
	} else
		return 1;
	return 0;
}
/*
 * ����ID3V2
 * buf:�������ݻ�����(��С�̶���128�ֽ�)
 * size:���ݴ�С
 * pctrl:MP3������
 * ����ֵ:=0,����,����,��ȡʧ��
 */
u8 mp3_id3v2_decode(u8* buf, u32 size, MP3CTRL *pctrl) {
	u8* buff = buf;
	assert(size >= sizeof(ID3V2_TagHead));
	ID3V2_TagHead *taghead = (ID3V2_TagHead*)buf;
	buf += sizeof(ID3V2_TagHead);
	if (strncmp("ID3", taghead->id, 3)) {
		//������ID3,mp3�����Ǵ�0��ʼ
		pctrl->datastart = 0;
		return 0;
	}
	//�õ�tag ��С
	//һ���ĸ��ֽڣ���ÿ���ֽ�ֻ��7λ�����λ��ʹ�ú�Ϊ0�����Ը�ʽ����
	//0xxxxxxx 0xxxxxxx 0xxxxxxx 0xxxxxxx
	//�����СʱҪ��0 ȥ�����õ�һ��28 λ�Ķ�������
	u32 tagsize = ((u32)taghead->size[0] << 21)
				  | ((u32)taghead->size[1] << 14)
				  | ((u16)taghead->size[2] << 7)
				  | taghead->size[3];

	pctrl->datastart = tagsize;		//�õ�mp3���ݿ�ʼ��ƫ����
	if (tagsize > size) {
		//tagsize��������bufsize��ʱ��,ֻ��������size��С������
		tagsize = size;
	}

	if (taghead->mversion < 3) {
		printf("not supported mversion!\r\n");
		return 1;
	}
	while (buf < buff + size) {
		ID3V23_FrameHead *framehead = (ID3V23_FrameHead*)(buf);

		//�õ�֡��С
		u32 frame_size = ((u32)framehead->size[0] << 24)
						 | ((u32)framehead->size[1] << 16)
						 | ((u32)framehead->size[2] << 8)
						 | framehead->size[3];

		if (strncmp("TT2", framehead->id, 3) == 0
				|| strncmp("TIT2", framehead->id, 4) == 0) {
			//�ҵ���������֡,��֧��unicode��ʽ!!
			strncpy((char*)pctrl->title, buf + sizeof(ID3V23_FrameHead) + 1, AUDIO_MIN(frame_size - 1, MP3_TITSIZE_MAX - 1));
		}
		if (strncmp("TP1", framehead->id, 3) == 0
				|| strncmp("TPE1", framehead->id, 4) == 0) {
			//�ҵ�����������֡
			strncpy((char*)pctrl->artist, (char*)(buf + sizeof(ID3V23_FrameHead) + 1), AUDIO_MIN(frame_size - 1, MP3_ARTSIZE_MAX - 1));
		}
		buf += frame_size + sizeof(ID3V23_FrameHead);
	}

	return 0;
}

/*
 * ��ȡMP3������Ϣ
 * pname:MP3�ļ�·��
 * pctrl:MP3������Ϣ�ṹ��
 * ����ֵ:0,�ɹ�
 *     ����,ʧ��
 */
u8 mp3_get_info(FIL *fmp3, u8 *buf, u32 size, MP3CTRL *pctrl) {
	MP3FrameInfo frame_info;
	u32 br;
	u8 res = 0;
	u32 p;
	short samples_per_frame;	//һ֡�Ĳ�������
	u32 totframes;				//��֡��


	//��ʼ����ID3V2/ID3V1�Լ���ȡMP3��Ϣ
	f_lseek(fmp3, 0);
	f_read(fmp3, buf, size, &br);
	mp3_id3v2_decode(buf, br, pctrl);			//����ID3V2����

	f_lseek(fmp3, fmp3->fsize - 128);			//ƫ�Ƶ�����128��λ��
	f_read(fmp3, buf, 128, &br);				//��ȡ128�ֽ�
	mp3_id3v1_decode(buf, pctrl);				//����ID3V1����

	HMP3Decoder decoder = MP3InitDecoder(); 	//MP3���������ڴ�
	f_lseek(fmp3, pctrl->datastart);			//ƫ�Ƶ����ݿ�ʼ�ĵط�
	f_read(fmp3, buf, size, &br);			//��ȡ5K�ֽ�mp3����
	int offset = MP3FindSyncWord(buf, br);		//����֡ͬ����Ϣ

	if (offset < 0 || MP3GetNextFrameInfo(decoder, &frame_info, &buf[offset])) {
		res = 0XFE; //δ�ҵ�ͬ��֡
		printf("δ�ҵ�ͬ��֡\n");
		goto exit;
	}

	printf("%lu bp/s audio MPEG layer %d, %d Hz sample rate,  %d Chanel , outputSamps:%d.\n",
		   frame_info.bitrate, frame_info.layer, frame_info.samprate, frame_info.nChans, frame_info.outputSamps);
	if ((frame_info.layer < 3) || (frame_info.bitrate <= 0) || (frame_info.samprate <= 0)) {
		//((mp3FrameInfo.bitrate>=320000) �����ʵ� inputbuf��СҪ���
		//helixֻ֧�� MP3 ����, .vbr���ļ� �õ���һЩ����Ī������
		printf("bitrate or layer not support!\n");
		res = 1;
		goto exit;
	}
	//�ҵ�֡ͬ����Ϣ��,����һ֡��Ϣ��ȡ����
	p = offset + 4 + 32;
	MP3_FrameVBRI *fvbri = (MP3_FrameVBRI*)(buf + p);
	if (strncmp("VBRI", fvbri->id, 4) == 0) {
		//����VBRI֡(VBR��ʽ)
		if (frame_info.version == MPEG1) {
			//MPEG1,layer3ÿ֡����������1152
			samples_per_frame = 1152;
		} else {
			//MPEG2/MPEG2.5,layer3ÿ֡����������576
			samples_per_frame = 576;
		}
		//�õ���֡��
		totframes = ((u32)fvbri->frames[0] << 24)
					| ((u32)fvbri->frames[1] << 16)
					| ((u16)fvbri->frames[2] << 8)
					| fvbri->frames[3];

		//�õ��ļ��ܳ���
		pctrl->totsec = totframes * samples_per_frame / frame_info.samprate;
	} else {
		//����VBRI֡,�����ǲ���Xing֡(VBR��ʽ)
		if (frame_info.version == MPEG1) {
			p = frame_info.nChans == 2 ? 32 : 17;
			//MPEG1,layer3ÿ֡����������1152
			samples_per_frame = 1152;
		} else {
			p = frame_info.nChans == 2 ? 17 : 9;
			//MPEG2/MPEG2.5,layer3ÿ֡����������576
			samples_per_frame = 576;
		}

		p += offset + 4;
		MP3_FrameXing *fxing = (MP3_FrameXing*)(buf + p);

		if (strncmp("Xing", (char*)fxing->id, 4) == 0 || strncmp("Info", (char*)fxing->id, 4) == 0) { //��Xing֡
			if (fxing->flags[3] & 0X01) {
				//������frame�ֶ�
				//�õ���֡��
				totframes = ((u32)fxing->frames[0] << 24)
							| ((u32)fxing->frames[1] << 16)
							| ((u16)fxing->frames[2] << 8)
							| fxing->frames[3];

				pctrl->totsec = totframes * samples_per_frame / frame_info.samprate; //�õ��ļ��ܳ���
			} else {
				//��������frames�ֶ�
				pctrl->totsec = fmp3->fsize / (frame_info.bitrate / 8);
			}
		} else {
			//CBR��ʽ,ֱ�Ӽ����ܲ���ʱ��
			pctrl->totsec = fmp3->fsize / (frame_info.bitrate / 8);
		}
	}
	pctrl->bitrate = frame_info.bitrate;		//�õ���ǰ֡������
	mp3ctrl->samplerate = frame_info.samprate;	//�õ�������.
	if (frame_info.nChans == 2) {
		//���PCM��������С
		mp3ctrl->outsamples = frame_info.outputSamps;
	} else {
		//���PCM��������С,���ڵ�����MP3,ֱ��*2,����Ϊ˫�������
		mp3ctrl->outsamples = frame_info.outputSamps * 2;
	}
exit:
	MP3FreeDecoder(decoder);//�ͷ��ڴ�
	return res;
}

//�õ���ǰ����ʱ��
void mp3_get_curtime(FIL*fx, MP3CTRL *mp3x) {
	u32 fpos = 0;
	if (fx->fptr > mp3x->datastart) {
		//�õ���ǰ�ļ����ŵ��ĵط�
		fpos = fx->fptr - mp3x->datastart;
	}

	//��ǰ���ŵ��ڶ�������?
	mp3x->cursec = fpos * mp3x->totsec / (fx->fsize - mp3x->datastart);
}
