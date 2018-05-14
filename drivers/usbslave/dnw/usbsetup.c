#include <s3c24x0.h>
#include "def.h"
#include "2440usb.h"
#include "usbmain.h"
#include "usb.h"
#include "usblib.h"
#include "usbsetup.h"

extern S3C24X0_INTERRUPT * intregs;
extern S3C24X0_USB_DEVICE * usbdevregs;
extern S3C24X0_DMAS * dmaregs;

// *** End point information ***
//   EP0: control
//   EP1: bulk in end point
//   EP2: not used
//   EP3: bulk out end point
//   EP4: not used

/* VERY IMPORTANT NOTE */
// Every descriptor size of EP0 should be 8n+m(m=1~7).
// Otherwise, USB will not operate normally because the program
// doesn't prepare the case that the descriptor size is 8n+0.
// If the size of a descriptor is 8n, the 0 length packit should be sent.
// Special thanks to E.S.Choi for reminding me of this USB specification.

/*
 * All following commands will operate only in case
 * - ep0_csr is valid.
 */
#define CLR_EP0_OUT_PKT_RDY() 		usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) | EP0_SERVICED_OUT_PKT_RDY )
#define CLR_EP0_OUTPKTRDY_DATAEND() usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) | (EP0_SERVICED_OUT_PKT_RDY|EP0_DATA_END) )
#define SET_EP0_IN_PKT_RDY() 		usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) | (EP0_IN_PKT_READY) )
#define SET_EP0_INPKTRDY_DATAEND() 	usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) | (EP0_IN_PKT_READY|EP0_DATA_END) )
#define CLR_EP0_SETUP_END() 		usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) | (EP0_SERVICED_SETUP_END) )
#define CLR_EP0_SENT_STALL() 		usbdevregs->EP0_CSR_IN_CSR1_REG =( ep0_csr & (~EP0_WR_BITS) & (~EP0_SENT_STALL) )
#define FLUSH_EP0_FIFO() 			{while(usbdevregs->OUT_FIFO_CNT1_REG) usbdevregs->fifo[0].EP_FIFO_REG;}

U32 ep0State;

extern volatile int isUsbdSetConfiguration;
volatile U8 Rwuen;
volatile U8 Configuration = 1;
volatile U8 AlterSetting;
volatile U8 Selfpwr = TRUE;
volatile U8 device_status;
volatile U8 interface_status;
volatile U8 endpoint0_status;
volatile U8 endpoint1_status;
volatile U8 endpoint3_status;

struct USB_SETUP_DATA descSetup;
struct USB_DEVICE_DESCRIPTOR descDev;
struct USB_CONFIGURATION_DESCRIPTOR descConf;
struct USB_INTERFACE_DESCRIPTOR descIf;
struct USB_ENDPOINT_DESCRIPTOR descEndpt0;
struct USB_ENDPOINT_DESCRIPTOR descEndpt1;
struct USB_CONFIGURATION_SET ConfigSet;
struct USB_INTERFACE_GET InterfaceGet;
struct USB_GET_STATUS StatusGet;   //={0,0,0,0,0};


static const U8 descStr0[] = {
	4, STRING_TYPE, LANGID_US_L, LANGID_US_H, //codes representing languages
};

static const U8 descStr1[] = { //Manufacturer
	(0x14 + 2), STRING_TYPE,
	'S', 0x0, 'y', 0x0, 's', 0x0, 't', 0x0, 'e', 0x0, 'm', 0x0, ' ', 0x0, 'M', 0x0,
	'C', 0x0, 'U', 0x0,
};

static const U8 descStr2[] = { //Product
	(0x2a + 2), STRING_TYPE,
	'S', 0x0, 'E', 0x0, 'C', 0x0, ' ', 0x0, 'S', 0x0, '3', 0x0, 'C', 0x0, '2', 0x0,
	'4', 0x0, '1', 0x0, '0', 0x0, 'X', 0x0, ' ', 0x0, 'T', 0x0, 'e', 0x0, 's', 0x0,
	't', 0x0, ' ', 0x0, 'B', 0x0, '/', 0x0, 'D', 0x0
};


void Ep0Handler(void) {
	static int ep0SubState;
	usbdevregs->INDEX_REG = 0;
	U8 ep0_csr = usbdevregs->EP0_CSR_IN_CSR1_REG;

	DbgPrintf("<0:%x]\n", ep0_csr);

	//DATAEND interrupt(ep0_csr==0x0) will be ignored
	//because ep0State==EP0_STATE_INIT when the DATAEND interrupt is issued.

	if (ep0_csr & EP0_SETUP_END) {
		// Host may end GET_DESCRIPTOR operation without completing the IN data stage.
		// If host does that, SETUP_END bit will be set.
		// OUT_PKT_RDY has to be also cleared because status stage sets OUT_PKT_RDY to 1.
		DbgPrintf("[SETUP END]\n");
		CLR_EP0_SETUP_END();
		if (ep0_csr & EP0_OUT_PKT_READY) {
			FLUSH_EP0_FIFO(); //(???)
			//I think this isn't needed because EP0 flush is done automatically.
			CLR_EP0_OUT_PKT_RDY();
		}

		ep0State = EP0_STATE_INIT;
		return;
	}

	//I think that EP0_SENT_STALL will not be set to 1.
	if (ep0_csr & EP0_SENT_STALL) {
		DbgPrintf("[STALL]\n");
		CLR_EP0_SENT_STALL();
		if (ep0_csr & EP0_OUT_PKT_READY) {
			CLR_EP0_OUT_PKT_RDY();
		}

		ep0State = EP0_STATE_INIT;
		return;
	}


	/*
	OUT_PKT_RDY:�������һ�����ݰ��� FIFO �к��� USB ��λ��һ��
	MCU �� FIFO �ж�ȡ������λӦ�ñ� MCU �����д"0"��
	*/
	//�ж��Ƿ�������µ����ݰ�
	if ((ep0_csr & EP0_OUT_PKT_READY)) {
		//��8���ֽڵ����ư�
		RdPktEp0((U8 *)&descSetup, EP0_PKT_SIZE);
		PrintEp0Pkt((U8 *)(&descSetup)); //DEBUG
		//�ж���������
		//note:bmRequestType�����ж���������
		switch (descSetup.bRequest) {
			case GET_DESCRIPTOR:
				switch (descSetup.bValueH) {
					case DEVICE_TYPE:
						DbgPrintf("[GET_DESCRIPTOR:DEVICE_TYPE]\n");
						CLR_EP0_OUT_PKT_RDY();
						ep0State = EP0_STATE_GD_DEV_0;
						break;
					case CONFIGURATION_TYPE:
						DbgPrintf("[GET_DESCRIPTOR:CONFIGURATION_TYPE]\n");
						CLR_EP0_OUT_PKT_RDY();
						if ((descSetup.bLengthL + (descSetup.bLengthH << 8)) > 0x9)
							//bLengthH should be used for bLength=0x209 at WIN2K.
							ep0State = EP0_STATE_GD_CFG_0; //for WIN98,WIN2K
						else
							ep0State = EP0_STATE_GD_CFG_ONLY_0; //for WIN2K
						break;
					case STRING_TYPE:
						DbgPrintf("[GET_DESCRIPTOR:STRING_TYPE]\n");
						CLR_EP0_OUT_PKT_RDY();
						switch (descSetup.bValueL) {
							case 0:
								ep0State = EP0_STATE_GD_STR_I0;
								break;
							case 1:
								ep0State = EP0_STATE_GD_STR_I1;
								break;
							case 2:
								ep0State = EP0_STATE_GD_STR_I2;
								break;
							default:
								DbgPrintf("[UE:STRI?]\n");
								break;
						}
						ep0SubState = 0;
						break;
					case INTERFACE_TYPE:
						DbgPrintf("[GET_DESCRIPTOR:STRING_TYPE]\n");
						CLR_EP0_OUT_PKT_RDY();
						ep0State = EP0_STATE_GD_IF_ONLY_0; //for WIN98
						break;
					case ENDPOINT_TYPE:
						DbgPrintf("[GET_DESCRIPTOR:ENDPOINT_TYPE]\n");
						CLR_EP0_OUT_PKT_RDY();
						switch (descSetup.bValueL & 0xf) {
							case 0:
								ep0State = EP0_STATE_GD_EP0_ONLY_0;
								break;
							case 1:
								ep0State = EP0_STATE_GD_EP1_ONLY_0;
								break;
							default:
								DbgPrintf("[UE:GDE?]\n");
								break;
						}
						break;
					default:
						DbgPrintf("[UE:GD?]\n");
						break;
				}
				break;
			case SET_ADDRESS:
				DbgPrintf("[SET_ADDRESS:%d]\n", descSetup.bValueL);
				usbdevregs->FUNC_ADDR_REG = descSetup.bValueL | 0x80;
				CLR_EP0_OUTPKTRDY_DATAEND(); //Because of no data control transfers.
				ep0State = EP0_STATE_INIT;
				break;
			case SET_CONFIGURATION:
				DbgPrintf("[SET_CONFIGURATION]\n");
				ConfigSet.ConfigurationValue = descSetup.bValueL;
				CLR_EP0_OUTPKTRDY_DATAEND(); //Because of no data control transfers.
				ep0State = EP0_STATE_INIT;
				isUsbdSetConfiguration = 1;
				break;
				//////////////////////// For chapter 9 test ////////////////////
			case CLEAR_FEATURE:
				switch (descSetup.bmRequestType) {
					case DEVICE_RECIPIENT:
						if (descSetup.bValueL == 1)
							Rwuen = FALSE;

						break;

					case ENDPOINT_RECIPIENT:
						if (descSetup.bValueL == 0) {
							if ((descSetup.bIndexL & 0x7f) == 0x00) {
								StatusGet.Endpoint0 = 0;
							}
							if ((descSetup.bIndexL & 0x8f) == 0x81) {         // IN  Endpoint 1
								StatusGet.Endpoint1 = 0;
							}
							if ((descSetup.bIndexL & 0x8f) == 0x03) {        // OUT Endpoint 3
								StatusGet.Endpoint3 = 0;
							}
						}

						break;

					default:
						break;
				}
				CLR_EP0_OUTPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				break;
			case GET_CONFIGURATION:
				CLR_EP0_OUT_PKT_RDY();
				ep0State = EP0_CONFIG_SET;
				break;
			case GET_INTERFACE:
				CLR_EP0_OUT_PKT_RDY();
				ep0State = EP0_INTERFACE_GET;
				break;
			case GET_STATUS:
				switch (descSetup.bmRequestType) {
					case (0x80):
						CLR_EP0_OUT_PKT_RDY();
						StatusGet.Device = ((U8)Rwuen << 1) | (U8)Selfpwr;
						ep0State = EP0_GET_STATUS0;
						break;
					case (0x81):
						CLR_EP0_OUT_PKT_RDY();
						StatusGet.Interface = 0;
						ep0State = EP0_GET_STATUS1;
						break;
					case (0x82):
						CLR_EP0_OUT_PKT_RDY();
						if ((descSetup.bIndexL & 0x7f) == 0x00) {
							ep0State = EP0_GET_STATUS2;
						}

						if ((descSetup.bIndexL & 0x8f) == 0x81) {
							ep0State = EP0_GET_STATUS3;
						}

						if ((descSetup.bIndexL & 0x8f) == 0x03) {
							ep0State = EP0_GET_STATUS4;
						}
						break;
					default:
						break;
				}
				break;
			case SET_DESCRIPTOR:
				CLR_EP0_OUTPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				break;
			case SET_FEATURE:
				switch (descSetup.bmRequestType) {
					case DEVICE_RECIPIENT:
						if (descSetup.bValueL == 1)
							Rwuen = TRUE;
						break;
					case ENDPOINT_RECIPIENT:
						if (descSetup.bValueL == 0) {
							if ((descSetup.bIndexL & 0x7f) == 0x00) {
								StatusGet.Endpoint0 = 1;
							}
							if ((descSetup.bIndexL & 0x8f) == 0x81) {
								StatusGet.Endpoint1 = 1;
							}
							if ((descSetup.bIndexL & 0x8f) == 0x03) {
								StatusGet.Endpoint3 = 1;
							}
						}
						break;
					default:
						break;
				}
				CLR_EP0_OUTPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				break;
			case SET_INTERFACE:
				InterfaceGet.AlternateSetting = descSetup.bValueL;
				CLR_EP0_OUTPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				break;
			case SYNCH_FRAME:
				ep0State = EP0_STATE_INIT;
				break;
			default:
				DbgPrintf("[UE:SETUP=%x]\n", descSetup.bRequest);
				CLR_EP0_OUTPKTRDY_DATAEND(); //Because of no data control transfers.
				ep0State = EP0_STATE_INIT;
				break;
		}
	}

	switch (ep0State) {
		case EP0_STATE_INIT:
			break;
			/*
			�����豸���������ܳ� 8+8+2 ���ֽ�
			�˵�0 ������С8�ֽڣ����Ҫ�����η���
			ÿ�η�����ɣ�mcu ��λ IN_PKT_RDY(packet ready)
			USB�������ʱ,USB��IN_PKT_RDY����
			��������һ�����ݰ���IN_PKT_RDY��λ��ͬʱ��Ҫ�� DATA_END ��λ
			*/
		/* GET_DESCRIPTOR:DEVICE */
		case EP0_STATE_GD_DEV_0:
			//TODO:�ڵ�һ�λ�ȡ�豸������ֻ�÷���һ�����ݰ�
			DbgPrintf("[GD_DEV_0]\n");
			WrPktEp0((U8 *)&descDev + 0, 8); //EP0_PKT_SIZE
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_DEV_1;
			break;
		case EP0_STATE_GD_DEV_1:
			DbgPrintf("[GD_DEV_1]\n");
			WrPktEp0((U8 *)&descDev + 0x8, 8);
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_DEV_2;
			break;
		case EP0_STATE_GD_DEV_2:
			DbgPrintf("[GD_DEV_2]\n");
			WrPktEp0((U8 *)&descDev + 0x10, 2); //8+8+2=0x12
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;

		/* GET_DESCRIPTOR:CONFIGURATION+INTERFACE+ENDPOINT0+ENDPOINT1 */
		//Windows98 gets these 4 descriptors all together by issuing only a request.
		//Windows2000 gets each descriptor seperately.
		/*
			��ȡ������������һ��ȫ����ȡ(�����������µĽӿ����ö˵�������)

			�������ĳ���(wLength)����ʵ�ʷ��ص����ݳ���(SendLength)��
			���ҷ��ص����ݰ������Ƕ˵���������������ʱ����Ҫ����0�������ݰ������򲻷���0�������ݰ�
			���������������ģ���� host �ĽǶ�������Ҫ���� 18 �ֽڣ������� setup �׶Σ�
			��ʵ�ʵ����ݴ����� data �׶Σ�����������һ�� data �׶Σ�
		transaction 1:
			host ���� in token��device �յ��󷢳� 8 �ֽ����ݣ��� 0 ~ 7 �ֽڣ���
		transaction 2:
			host �յ����ݺ󣬼������� in token��device �յ��󷢳� 8 �ֽ����ݣ��� 8 ~ 15 �ֽڣ���
			���ˣ�device �Ѱ� 16 �ֽ�����ȫ��������ϣ��� host Ҫ����� 18 �ֽڣ����յ��� 16 �ֽڣ�
			��Ȼ host ��Ϊ����û�д����꣬���ǣ�
		transaction 3:
			host �������� in token��device �յ� in token���ã������ڴ�ͣס��
			����Ϊ���� device Ӧ�������Ӧ��������� in token���ã�����������ɣ���ô host ���������Σ�
			Ȼ�󱨴�������Ȼ���С����֪ͨ host �����Ѿ�����أ�Ϊ�ˣ�USB Э��涨��device ���뷢һ��
			zero length packet��host �յ��󣬾ͻ�֪��û�и���������ˡ�
		*/
		case EP0_STATE_GD_CFG_0://��ȡ����������
			DbgPrintf("[GD_CFG_0]\n");
			WrPktEp0((U8 *)&descConf + 0, 8); //EP0_PKT_SIZE
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_CFG_1;
			break;
		case EP0_STATE_GD_CFG_1://��ȡ����������+�ӿ�������
			DbgPrintf("[GD_CFG_1]\n");
			WrPktEp0((U8 *)&descConf + 8, 1);
			WrPktEp0((U8 *)&descIf + 0, 7);
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_CFG_2;
			break;
		case EP0_STATE_GD_CFG_2://��ȡ�ӿ�������+�˵�1������
			DbgPrintf("[GD_CFG_2]\n");
			WrPktEp0((U8 *)&descIf + 7, 2);
			WrPktEp0((U8 *)&descEndpt0 + 0, 6);
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_CFG_3;
			break;
		case EP0_STATE_GD_CFG_3://��ȡ�˵�1������+�˵�3������
			DbgPrintf("[GD_CFG_3]\n");
			WrPktEp0((U8 *)&descEndpt0 + 6, 1);
			WrPktEp0((U8 *)&descEndpt1 + 0, 7); //2440�Ķ˵�3������ַ
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_CFG_4;
			break;
		case EP0_STATE_GD_CFG_4://֪ͨ host �����Ѿ����
			DbgPrintf("[GD_CFG_4]\n");
			//zero length data packit
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* ��������Ļ�ȡ���������� */
		/* GET_DESCRIPTOR:CONFIGURATION ONLY */
		case EP0_STATE_GD_CFG_ONLY_0:
			DbgPrintf("[GD_CFG_ONLY_0]\n");
			WrPktEp0((U8 *)&descConf + 0, 8); //EP0_PKT_SIZE
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_CFG_ONLY_1;
			break;
		case EP0_STATE_GD_CFG_ONLY_1:
			DbgPrintf("[GD_CFG_ONLY_1]\n");
			WrPktEp0((U8 *)&descConf + 8, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* GET_DESCRIPTOR:INTERFACE ONLY */
		case EP0_STATE_GD_IF_ONLY_0:
			DbgPrintf("[GD_IF_ONLY_0]\n");
			WrPktEp0((U8 *)&descIf + 0, 8);
			SET_EP0_IN_PKT_RDY();
			ep0State = EP0_STATE_GD_IF_ONLY_1;
			break;
		case EP0_STATE_GD_IF_ONLY_1:
			DbgPrintf("[GD_IF_ONLY_1]\n");
			WrPktEp0((U8 *)&descIf + 8, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* GET_DESCRIPTOR:ENDPOINT 1 ONLY */
		case EP0_STATE_GD_EP0_ONLY_0:
			DbgPrintf("[GD_EP0_ONLY_0]\n");
			WrPktEp0((U8 *)&descEndpt0 + 0, 7);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* GET_DESCRIPTOR:ENDPOINT 3 ONLY */
		case EP0_STATE_GD_EP1_ONLY_0:
			DbgPrintf("[GD_EP1_ONLY_0]\n");
			WrPktEp0((U8 *)&descEndpt1 + 0, 7); //2440�Ķ˵�3������ַ
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		case EP0_INTERFACE_GET:
			WrPktEp0((U8 *)&InterfaceGet + 0, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* GET_DESCRIPTOR:STRING *//* �ַ��������� */
		case EP0_STATE_GD_STR_I0:
			DbgPrintf("[GD_STR_I0]\n");
			WrPktEp0((U8 *)descStr0, 4);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			ep0SubState = 0;
			break;
		case EP0_STATE_GD_STR_I1:
			DbgPrintf("[GD_STR_I1_%d]\n", ep0SubState);
			if ((ep0SubState * EP0_PKT_SIZE + EP0_PKT_SIZE) < sizeof(descStr1)) {
				WrPktEp0((U8 *)descStr1 + (ep0SubState * EP0_PKT_SIZE), EP0_PKT_SIZE);
				SET_EP0_IN_PKT_RDY();
				ep0State = EP0_STATE_GD_STR_I1;
				ep0SubState++;
			} else {
				WrPktEp0((U8 *)descStr1 + (ep0SubState * EP0_PKT_SIZE),
						 sizeof(descStr1) - (ep0SubState * EP0_PKT_SIZE));
				SET_EP0_INPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				ep0SubState = 0;
			}
			break;
		case EP0_STATE_GD_STR_I2:
			DbgPrintf("[GD_STR_I2_%d]\n", ep0SubState);
			if ((ep0SubState * EP0_PKT_SIZE + EP0_PKT_SIZE) < sizeof(descStr2)) {
				WrPktEp0((U8 *)descStr2 + (ep0SubState * EP0_PKT_SIZE), EP0_PKT_SIZE);
				SET_EP0_IN_PKT_RDY();
				ep0State = EP0_STATE_GD_STR_I2;
				ep0SubState++;
			} else {
				DbgPrintf("[E]\n");
				WrPktEp0((U8 *)descStr2 + (ep0SubState * EP0_PKT_SIZE),
						 sizeof(descStr2) - (ep0SubState * EP0_PKT_SIZE));
				SET_EP0_INPKTRDY_DATAEND();
				ep0State = EP0_STATE_INIT;
				ep0SubState = 0;
			}
			break;
		/* �������� */
		case EP0_CONFIG_SET:
			WrPktEp0((U8 *)&ConfigSet + 0, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		/* ״̬ */
		case EP0_GET_STATUS0:
			WrPktEp0((U8 *)&StatusGet + 0, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		case EP0_GET_STATUS1:
			WrPktEp0((U8 *)&StatusGet + 1, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		case EP0_GET_STATUS2:
			WrPktEp0((U8 *)&StatusGet + 2, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		case EP0_GET_STATUS3:
			WrPktEp0((U8 *)&StatusGet + 3, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		case EP0_GET_STATUS4:
			WrPktEp0((U8 *)&StatusGet + 4, 1);
			SET_EP0_INPKTRDY_DATAEND();
			ep0State = EP0_STATE_INIT;
			break;
		default:
			DbgPrintf("UE:G?D\n");
			break;
	}
}

void PrintEp0Pkt(U8 *pt) {
	DbgPrintf("[RCV:");
	for (int i = 0; i < EP0_PKT_SIZE; i++)
		DbgPrintf("%02x,", pt[i]);
	DbgPrintf("]\n");
}

void InitDescriptorTable(void) {
	//Standard device descriptor
	descDev.bLength = 0x12;			//EP0_DEV_DESC_SIZE=0x12 bytes
	descDev.bDescriptorType = DEVICE_TYPE;
	descDev.bcdUSBL = 0x10;
	descDev.bcdUSBH = 0x01;			//Ver 1.10
	descDev.bDeviceClass = 0xFF;	//0x0
	descDev.bDeviceSubClass = 0x0;
	descDev.bDeviceProtocol = 0x0;
	descDev.bMaxPacketSize0 = 0x8;
	descDev.idVendorL = 0x45;
	descDev.idVendorH = 0x53;
	descDev.idProductL = 0x34;
	descDev.idProductH = 0x12;
	descDev.bcdDeviceL = 0x00;
	descDev.bcdDeviceH = 0x01;
	descDev.iManufacturer = 0x1;	//index of string descriptor
	descDev.iProduct = 0x2;			//index of string descriptor
	descDev.iSerialNumber = 0x0;
	descDev.bNumConfigurations = 0x1;

	//Standard configuration descriptor
	descConf.bLength = 0x9;
	descConf.bDescriptorType = CONFIGURATION_TYPE;
	descConf.wTotalLengthL = 0x20;		//<cfg desc>+<if desc>+<endp0 desc>+<endp1 desc>
	descConf.wTotalLengthH = 0;
	descConf.bNumInterfaces = 1;
	descConf.bConfigurationValue = 1;	//��������
	descConf.iConfiguration = 0;
	descConf.bmAttributes = CONF_ATTR_DEFAULT | CONF_ATTR_SELFPOWERED; //�Թ���
	descConf.maxPower = 25;				//draws 50mA current from the USB bus.

	//Standard interface descriptor
	descIf.bLength = 0x9;
	descIf.bDescriptorType = INTERFACE_TYPE;
	descIf.bInterfaceNumber = 0x0;
	descIf.bAlternateSetting = 0x0; //�ӿڱ��ñ�ţ�һ�㲻�ã���Ϊ0
	descIf.bNumEndpoints = 2;		//�ӿ�ʹ�õĶ˵���#������0�˵�
	descIf.bInterfaceClass = 0xff;	//�ýӿ�ʹ�õ���
	descIf.bInterfaceSubClass = 0x0;//�ýӿ�ʹ�õ�����
	descIf.bInterfaceProtocol = 0x0;//�ýӿ�ʹ�õ�Э��
	descIf.iInterface = 0x0;		//�����ýӿڵ��ַ���������ֵ��0��ʾû��

	//Standard endpoint0 descriptor
	descEndpt0.bLength = 0x7;
	descEndpt0.bDescriptorType = ENDPOINT_TYPE;
	descEndpt0.bEndpointAddress = 1 | EP_ADDR_IN;	//endpoint 1 is IN endpoint.
	descEndpt0.bmAttributes = EP_ATTR_BULK;
	descEndpt0.wMaxPacketSizeL = EP1_PKT_SIZE;		//64
	descEndpt0.wMaxPacketSizeH = 0x0;
	descEndpt0.bInterval = 0x0;						//�˵��ѯ��ʱ��

	//Standard endpoint1 descriptor
	descEndpt1.bLength = 0x7;
	descEndpt1.bDescriptorType = ENDPOINT_TYPE;
	descEndpt1.bEndpointAddress = 3 | EP_ADDR_OUT;	//endpoint 3 is OUT endpoint.
	descEndpt1.bmAttributes = EP_ATTR_BULK;
	descEndpt1.wMaxPacketSizeL = EP3_PKT_SIZE;		//64
	descEndpt1.wMaxPacketSizeH = 0x0;
	descEndpt1.bInterval = 0x0;						//�˵��ѯ��ʱ��
}

