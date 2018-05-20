#include <s3c24x0.h>
#include <sys/types.h>
#include <assert.h>
#include <usb/ch9.h>
#include <usb/2440usb.h>
struct g_config_desc_to_send 
{
	struct usb_config_descriptor g_config_desc;
	struct usb_interface_descriptor g_interface_desc;
	struct usb_hid_descriptor g_hid_desc;
	struct usb_endpoint_descriptor g_endpoint_desc;
} __attribute__ ((packed));

/*
 * USB�����������Ķ���
 * ���ص����뱨�����4�ֽڡ�
 * ��һ�ֽڵĵ�3λ������ʾ�����Ƿ��µģ���5λΪ����0�����á�
 * �ڶ��ֽڱ�ʾX��ĵı����������ֽڱ�ʾY��ĸı����������ֽڱ�ʾ
 * ���ֵĸı������������ж϶˵�1��Ӧ��Ҫ��������ĸ�ʽ����ʵ�ʵ�
 * ������ݡ�
 */
static const char gHIDReportDescriptor[52] = {
	//ÿ�п�ʼ�ĵ�һ�ֽ�Ϊ����Ŀ��ǰ׺��ǰ׺�ĸ�ʽΪ��
	//D7~D4��bTag��D3~D2��bType��D1~D0��bSize�����·ֱ��ÿ����Ŀע�͡�

	//����һ��ȫ�֣�bTypeΪ1����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
	//�����һ�ֽ����ݣ�bSizeΪ1����������ֽ����Ͳ�ע���ˣ�
	//�Լ�����bSize���жϡ�
	0x05U, 0x01U,                   /* Usage Page (Generic Desktop)         */

	//����һ���ֲ���bTypeΪ2����Ŀ��˵����������Ӧ�ü�����;�������
	0x09U, 0x02U,                   /* Usage (Mouse)                        */

	//����һ������Ŀ��bTypeΪ0����Ŀ�������ϣ������������0x01��ʾ
	//�ü�����һ��Ӧ�ü��ϡ�����������ǰ������;ҳ����;����Ϊ
	//��ͨ�����õ���ꡣ
	0xA1U, 0x01U,                   /* Collection (Application)             */

	//����һ���ֲ���Ŀ��˵����;Ϊָ�뼯��
	0x09U, 0x01U,                   /*   Usage (Pointer)                    */

	//����һ������Ŀ�������ϣ������������0x00��ʾ�ü�����һ��
	//�����ϣ���;��ǰ��ľֲ���Ŀ����Ϊָ�뼯�ϡ�
	0xA1U, 0x00U,                   /*   Collection (Physical)              */

	//����һ��ȫ����Ŀ��ѡ����;ҳΪ������Button Page(0x09)��
	0x05U, 0x09U,                   /*     Usage Page (Buttons)             */

	//����һ���ֲ���Ŀ��˵����;����СֵΪ1��ʵ��������������
	0x19U, 0x01U,                   /*     Usage Minimum (01)               */

	//����һ���ֲ���Ŀ��˵����;�����ֵΪ3��ʵ����������м���
	0x29U, 0x03U,                   /*     Usage Maximum (03)               */

	//����һ��ȫ����Ŀ��˵�����ص����ݵ��߼�ֵ���������Ƿ��ص��������ֵ����
	//��СΪ0����Ϊ����������Bit����ʾһ�������������СΪ0�����Ϊ1��
	0x15U, 0x00U,                   /*     Logical Minimum (0)              */

	//����һ��ȫ����Ŀ��˵���߼�ֵ���Ϊ1��
	0x25U, 0x01U,                   /*     Logical Maximum (1)              */

	//����һ��ȫ����Ŀ��˵�������������Ϊ������
	0x95U, 0x03U,                   /*     Report Count (3)                 */

	//����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ1��bit��
	0x75U, 0x01U,                   /*     Report Size (1)                  */

	//����һ������Ŀ��˵����3������Ϊ1bit�������������ͳ���
	//��ǰ�������ȫ����Ŀ�����壩������Ϊ���룬
	//����Ϊ��Data,Var,Abs��Data��ʾ��Щ���ݿ��Ա䶯��Var��ʾ
	//��Щ�������Ƕ����ģ�ÿ�����ʾһ����˼��Abs��ʾ����ֵ��
	//��������Ľ�����ǣ���һ��������bit0��ʾ����1��������Ƿ��£�
	//�ڶ���������bit1��ʾ����2���Ҽ����Ƿ��£�������������bit2��ʾ
	//����3���м����Ƿ��¡�
	0x81U, 0x02U,                   /*     Input (Data, Variable, Absolute) */

	//����һ��ȫ����Ŀ��˵������������Ϊ1��
	0x95U, 0x01U,                   /*     Report Count (1)                 */

	//����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ5bit��
	0x75U, 0x05U,                   /*     Report Size (5)                  */

	//����һ������Ŀ�������ã���ǰ������ȫ����Ŀ��֪������Ϊ5bit��
	//����Ϊ1������������Ϊ�����������ص�����һֱ��0����
	//���ֻ��Ϊ�˴���һ���ֽڣ�ǰ������3��bit��������һЩ����
	//���ѣ���������û��ʵ����;�ġ�
	0x81U, 0x01U,                   /*     Input (Constant) for padding     */

	//����һ��ȫ����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
	0x05U, 0x01U,                   /*     Usage Page (Generic Desktop)     */

	//����һ���ֲ���Ŀ��˵����;ΪX��
	0x09U, 0x30U,                   /*     Usage (X)                        */

	//����һ���ֲ���Ŀ��˵����;ΪY��
	0x09U, 0x31U,                   /*     Usage (Y)                        */

	//����һ���ֲ���Ŀ��˵����;Ϊ����
	0x09U, 0x38U,                   /*     Usage (Z)                        */

	//��������Ϊȫ����Ŀ��˵�����ص��߼���С�����ֵ��
	//��Ϊ���ָ���ƶ�ʱ��ͨ���������ֵ����ʾ�ģ�
	//���ֵ����˼���ǣ���ָ���ƶ�ʱ��ֻ�����ƶ�����
	//�����ƶ�ʱ��XֵΪ���������ƶ�ʱ��YֵΪ����
	//���ڹ��֣����������Ϲ�ʱ��ֵΪ����
	0x15U, 0x81U,                   /*     Logical Minimum (-127)           */
	0x25U, 0x7FU,                   /*     Logical Maximum (127)            */

	//����һ��ȫ����Ŀ��˵��������ĳ���Ϊ8bit��
	0x75U, 0x08U,                   /*     Report Size (8)                  */

	//����һ��ȫ����Ŀ��˵��������ĸ���Ϊ3����
	0x95U, 0x03U,                   /*     Report Count (3)                 */

	//����һ������Ŀ����˵��������8bit���������������õģ�
	//����Ϊ��Data,Var,Rel��Data˵�������ǿ��Ա�ģ�Var˵��
	//��Щ�������Ƕ����ģ�����һ��8bit��ʾX�ᣬ�ڶ���8bit��ʾ
	//Y�ᣬ������8bit��ʾ���֡�Rel��ʾ��Щֵ�����ֵ��
	0x81U, 0x06U,                   /*     Input (Data, Variable, Relative) */

	//��������������Ŀ�����ر�ǰ��ļ����á�
	//���ǿ����������ϣ�����Ҫ�����Ρ�bSizeΪ0�����Ժ���û���ݡ�
	0xC0U,                          /*   End Collection (Physical)          */
	0xC0U                           /* End Collection (Application)         */
};

static struct usb_device_descriptor g_usb_dev_desc = {
	.bLength 			= USB_DT_DEVICE_SIZE,
	.bDescriptorType 	= USB_DT_DEVICE,
	.bcdUSB 			= 0x0110,
	.bDeviceClass 		= 0,
	.bDeviceSubClass  	= 0,
	.bDeviceProtocol 	= 0,
	.bMaxPacketSize0    = 0x08,
	.idVendor			= 0x8888,
	.idProduct			= 0x6666,
	.iManufacturer		= 1,
	.iProduct			= 2,
	.iSerialNumber		= 3,
	.bNumConfigurations	= 1
};
static struct g_config_desc_to_send g_config_all = {
	.g_config_desc =
	{
		.bLength			= USB_DT_CONFIG_SIZE,
		.bDescriptorType	= USB_DT_CONFIG,

		.wTotalLength		= sizeof(struct usb_config_descriptor)
							+ sizeof(struct usb_interface_descriptor)
							+ sizeof(struct usb_endpoint_descriptor)
							+ sizeof(struct usb_hid_descriptor),
		.bNumInterfaces		= 1,	//��������
		.bConfigurationValue = 1,
		.iConfiguration		= 0,
		.bmAttributes		= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,	//�Լ�����
		.bMaxPower			= 0x32, //����Ϊ100mA
	},
	.g_interface_desc =
	{
		.bLength			= USB_DT_INTERFACE_SIZE,
		.bDescriptorType	= USB_DT_INTERFACE,

		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,	//�ӿڱ��ñ�ţ�һ�㲻�ã���Ϊ0
		.bNumEndpoints		= 1,	//�ӿ�ʹ�õĶ˵�����������0�˵�
		.bInterfaceClass	= USB_CLASS_HID,	//�ýӿ�ʹ�õ���,HID��
		.bInterfaceSubClass	= 1,	//�ýӿ�ʹ�õ�����,֧��BIOS��������������
		.bInterfaceProtocol	= 2,	//�ýӿ�ʹ�õ�Э��,���̴���Ϊ0x01��������Ϊ0x02
		.iInterface			= 0,	//�����ýӿڵ��ַ���������ֵ��0��ʾû��
	},
	.g_hid_desc =
	{
		.bLength			= sizeof(struct usb_hid_descriptor),
		.bDescriptorType	= 0x21,
		.bcdHID				= 0x0110,
		.bCountryCode		= 0x33,
		.bNumDescriptors	= 1,
		.bDescriptorType0	= 0x22,
		.wDescriptorLength	= sizeof(gHIDReportDescriptor),
	},
	.g_endpoint_desc =
	{
		.bLength			= USB_DT_ENDPOINT_SIZE,
		.bDescriptorType	= USB_DT_ENDPOINT,

		.bEndpointAddress	= USB_DIR_IN | EP1,	//endpoint 1 is IN endpoint.
		.bmAttributes		= USB_ENDPOINT_XFER_INT,
		.wMaxPacketSize		= EP1_PKT_SIZE,	//32
		.bInterval			= 0x0A,			//�˵��ѯ��ʱ��
	},
};

#define LANGID_US_L 		    	(0x09)
#define LANGID_US_H 		    	(0x04)
static char g_string_desc0[4] = {
	4,
	USB_DT_STRING,
	LANGID_US_L,
	LANGID_US_H
};
static char g_string_desc1[20] = {
	20,
	USB_DT_STRING,
	'T', 0x00,
	'E', 0x00,
	'S', 0x00,
	'T', 0x00,
	'V', 0x00,
	'E', 0x00,
	'N', 0x00,
	'D', 0x00,
	' ', 0x00,
};
static char g_string_desc2[20] = {
	20,
	USB_DT_STRING,
	'U', 0x00,
	'S', 0x00,
	'B', 0x00,
	'M', 0x00,
	'O', 0x00,
	'U', 0x00,
	'S', 0x00,
	'E', 0x00,
	' ', 0x00,
};
static char g_string_desc3[20] = {
	20,
	USB_DT_STRING,
	'U', 0x00,
	'S', 0x00,
	'B', 0x00,
	'M', 0x00,
	'O', 0x00,
	'U', 0x00,
	'S', 0x00,
	'E', 0x00,
	' ', 0x00,
};

static void usbdev_mouse_reset(void) {
	usbdevregs->EP_INT_EN_REG = 0;
	usbdevregs->USB_INT_EN_REG = 0;
	/* ��ֹ����ģʽ */
	usbdevregs->PWR_REG = PWR_REG_DEFAULT_VALUE;	//disable suspend mode
	usbdevregs->FUNC_ADDR_REG = 0x80;
	/* �˵�0 */
	SET_INDEX(0);
	usbdevregs->INDEX_REG = 0;
	usbdevregs->MAXP_REG = FIFO_SIZE_8;   	//EP0 max packit size = 8
	usbdevregs->EP0_CSR = EP0_SERVICED_OUT_PKT_RDY | EP0_SERVICED_SETUP_END;
	FLUSH_EP0_FIFO();
	/* �˵�1 */
	SET_INDEX(1);
	usbdevregs->MAXP_REG = FIFO_SIZE_64;
	usbdevregs->IN_CSR1_REG = EPI_FIFO_FLUSH | EPI_CDT;
	//usbdevregs->IN_CSR2_REG = EPI_MODE_IN | EPI_IN_DMA_INT_MASK | EPO_ISO; //IN mode, IN_DMA_INT=masked
	//TODO:EPO_ISO��ȥ����ԭ��δ֪
	usbdevregs->IN_CSR2_REG = EPI_MODE_IN | EPI_IN_DMA_INT_MASK; //IN mode, IN_DMA_INT=masked
	usbdevregs->OUT_CSR1_REG = EPO_CDT;
	usbdevregs->OUT_CSR2_REG = EPO_ISO | EPO_OUT_DMA_INT_MASK;

	usbdevregs->EP_INT_REG = EP0_INT | EP1_INT | EP2_INT | EP3_INT | EP4_INT;
	usbdevregs->USB_INT_REG = RESET_INT | SUSPEND_INT | RESUME_INT;

	usbdevregs->EP_INT_EN_REG = EP0_INT | EP1_INT;
	usbdevregs->USB_INT_EN_REG = RESET_INT;

	ep0State = EP0_STATE_INIT;
}

static void Ep1Handler(void) {
	return;
}

extern void handle_hid_class(struct usb_ctrlrequest ctrlreq);

struct usbdev_struct usbdev = {
	.dev_desc = &g_usb_dev_desc,
	.config_desc = &g_config_all.g_config_desc,
	.interface_desc = &g_config_all.g_interface_desc,
	.hid_desc = &g_config_all.g_hid_desc,
	.endpoint_desc[0] = &g_config_all.g_endpoint_desc,
	.endpoint_desc[1] = NULL,
	.endpoint_desc[2] = NULL,
	.endpoint_desc[3] = NULL,
	.string_desc[0] = &g_string_desc0,
	.string_desc[1] = &g_string_desc1,
	.string_desc[2] = &g_string_desc2,
	.string_desc[3] = &g_string_desc3,
	.string_desc_size[0] = sizeof(g_string_desc0),
	.string_desc_size[1] = sizeof(g_string_desc1),
	.string_desc_size[2] = sizeof(g_string_desc2),
	.string_desc_size[3] = sizeof(g_string_desc3),
	.config_all = &g_config_all,
	.config_all_size = sizeof(g_config_all),
	.report_desc = &gHIDReportDescriptor,
	.report_desc_size = sizeof(gHIDReportDescriptor),
	.ep_handler[0] = Ep1Handler,
	.ep_handler[1] = NULL,
	.ep_handler[2] = NULL,
	.ep_handler[3] = NULL,
	.reset = usbdev_mouse_reset,
	.handle_class = handle_hid_class,
};
