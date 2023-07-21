#include "USB.h"

#include <nrfx_power.h>

#include "class/hid/hid_device.h"
#include "device/usbd.h"

#define USBD_STACK_SIZE (3 * configMINIMAL_STACK_SIZE / 2) * (CFG_TUSB_DEBUG ? 2 : 1)

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_HID 0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 0x02
#define USB_INTERFACE_DESCRIPTOR_TYPE 0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE 0x05

#define HID_DESCRIPTOR_TYPE 0x21

#define CUSTOMHID_SIZ_CONFIG_DESC 41
#define CUSTOMHID_SIZ_REPORT_DESC 31

// uint8_t USB::rxData[64] = { 0 };
USBCallback USB::callback = nullptr;
USBConnectionCallback USB::connectedCallback = nullptr;
USBConnectionCallback USB::disconnectedCallback = nullptr;
StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

	.idVendor = 0x1231,
	.idProduct = 0x3321,
	.bcdDevice = 0x0200,

	.iManufacturer = 0x01,
	.iProduct = 0x02,
	.iSerialNumber = 0x03,

	.bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
const uint8_t* tud_descriptor_device_cb(void) {
	return (const uint8_t*)&desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] = {
	// TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_EP_BUFSIZE, HID_REPORT_ID(1))
	0x06, 0xFF, 0x00,  // USAGE_PAGE (Vendor-Defined)
	0x09, 0x01,           // USAGE (Vendor-Defined)
	0xA1, 0x01,           // COLLECTION (Application)

	0x85, 0x01,     // REPORT_ID (1)

	0x09, 0x03,           // USAGE (Vendor-Defined)
	0x15, 0x00,           // LOGICAL_MINIMUM (0)
	0x26, 0xFF, 0x00,  // LOGICAL_MAXIMUM (255)
	0x95, 0x08,           // REPORT_COUNT (8)
	0x75, 0x3E,           // REPORT_SIZE (62)
	0x81, 0x02,           // Input (Data, Variable, Absolute)

	0x09, 0x04,     // USAGE (Vendor-Defined)
	0x91, 0x02,     // Output (Data, Variable, Absolute)

	0x09, 0x05,     // USAGE (Vendor-Defined)
	0xB1, 0x02,     // Feature (Data, Variable, Absolute)
	0xc0,         //   END_COLLECTION
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
	return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_HID, ITF_NUM_TOTAL };

uint8_t const desc_configuration[] = {
	0x09,                               /* bLength: Configuration Descriptor size */
	USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
	CUSTOMHID_SIZ_CONFIG_DESC,
	/* wTotalLength: Bytes returned */
	0x00,
	0x01, /* bNumInterfaces: 1 interface */
	0x01, /* bConfigurationValue: Configuration value */
	0x00, /* iConfiguration: Index of string descriptor describing
				 the configuration*/
	0xA0, /* bmAttributes: Bus powered */
	0xFA, /* MaxPower 500 mA: this current is used for detecting Vbus */

	/************** Descriptor of Custom HID interface ****************/
	/* 09 */
	0x09,                           /* bLength: Interface Descriptor size */
	USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: Interface descriptor type */
	0x00,                           /* bInterfaceNumber: Number of Interface */
	0x00,                           /* bAlternateSetting: Alternate setting */
	0x02,                           /* bNumEndpoints */
	0x03,                           /* bInterfaceClass: HID */
	0x01,                           /* bInterfaceSubClass : 1=BOOT, 0=no boot */
	0x00,                           /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
	0,                               /* iInterface: Index of string descriptor */
	/******************** Descriptor of Custom HID HID ********************/
	/* 18 */
	0x09,                 /* bLength: HID Descriptor size */
	HID_DESCRIPTOR_TYPE, /* bDescriptorType: HID */
	0x10,                 /* bcdHID: HID Class Spec release number */
	0x01,
	0x00,                       /* bCountryCode: Hardware target country */
	0x01,                       /* bNumDescriptors: Number of HID class descriptors to follow */
	0x22,                       /* bDescriptorType */
	CUSTOMHID_SIZ_REPORT_DESC, /* wItemLength: Total length of Report descriptor */
	0x00,
	/******************** Descriptor of Custom HID endpoints ******************/
	/* 27 */
	0x07,                          /* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

	0x81, /* bEndpointAddress: Endpoint Address (IN) */
	0x03, /* bmAttributes: Interrupt endpoint */
	0x40, /* wMaxPacketSize: 64 Bytes max */
	0x00,
	0x01, /* bInterval: Polling Interval (1 ms) */
	/* 34 */
	0x07,                          /* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

	0x01, /* bEndpointAddress: Endpoint Address (OUT) */
	0x03, /* bmAttributes: Interrupt endpoint */
	0x40, /* wMaxPacketSize: 64 Bytes max */
	0x00,
	0x01, /* bInterval: Polling Interval (1 ms) */
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
	(void)index;  // for multiple configurations
	return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
const char* string_desc_arr[] = {
	(char[]){ 0x09, 0x04 },      // 0: is supported language is English (0x0409)
	"nrf52usbtest",              // 1: Manufacturer
	"nrf52usbtest device",      // 2: Product
	"123123",  // 3: Serials, should use chip ID
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
	(void)langid;

	uint8_t chr_count;

	if (index == 0) {
		memcpy(&_desc_str[1], string_desc_arr[0], 2);
		chr_count = 1;
	}
	else {
		// Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
		// https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

		if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
			return nullptr;
		}

		const char* str = string_desc_arr[index];

		// Cap at max char
		chr_count = (uint8_t)strlen(str);
		if (chr_count > 31) {
			chr_count = 31;
		}

		// Convert ASCII string into UTF-16
		for (uint8_t i = 0; i < chr_count; i++) {
			_desc_str[1 + i] = str[i];
		}
	}

	// first byte is length (including header), second byte is string type
	_desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

	return _desc_str;
}

// Invoked when device is mounted
void tud_mount_cb(void) {
	Serial.println("USB Mounted");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
	Serial.println("USB Unmounted");
}

void tud_suspend_cb(bool remote_wakeup_en) {
	(void)remote_wakeup_en;
	Serial.println("USB Suspended");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
	Serial.println("USB Resumed");
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t
tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
int cbcount = 0;

void tud_hid_set_report_cb(
	uint8_t itf,
	uint8_t report_id,
	hid_report_type_t report_type,
	uint8_t const* buffer,
	uint16_t bufsize
) {
	if (cbcount > 10) {
		Serial.println("GOT DAT!");
		cbcount = 0;
	}
	cbcount++;
	if (USB::callback) {
		USB::callback((uint8_t*)buffer, bufsize);
	}
}

[[noreturn]] void usb_device_task(void* param) {
	auto res = tud_init(0);
	Serial.println(res ? "USB Initialized" : "USB not ready!");
	// RTOS forever loop
	while (true) {
		tud_task();
		delay(1);
	}
}

// tinyusb function that handles power event (detected, ready, removed)
// We must call it within SD's SOC event handler, or set it as power event handler if SD is not enabled.
extern "C" void tusb_hal_nrf_power_event(uint32_t event);

//--------------------------------------------------------------------+
// Forward USB interrupt events to TinyUSB IRQ Handler
//--------------------------------------------------------------------+
extern "C" void USBD_IRQHandler(void) {
	#if CFG_SYSVIEW
	SEGGER_SYSVIEW_RecordEnterISR();
	#endif

	tud_int_handler(0);

	#if CFG_SYSVIEW
	SEGGER_SYSVIEW_RecordExitISR();
	#endif
}

// Init usb hardware when starting up. Softdevice is not enabled yet
static void usb_hardware_init() {
	// Priorities 0, 1, 4 (nRF52) are reserved for SoftDevice
	// 2 is highest for application
	NVIC_SetPriority(USBD_IRQn, 2);

	// USB power may already be ready at this time -> no event generated
	// We need to invoke the handler based on the status initially
	uint32_t usb_reg = NRF_POWER->USBREGSTATUS;

	// Power module init
	const nrfx_power_config_t pwr_cfg = { 0, 0 };
	nrfx_power_init(&pwr_cfg);

	// Register tusb function as USB power handler
	const nrfx_power_usbevt_config_t config = {
		.handler = (nrfx_power_usb_event_handler_t)(void*)tusb_hal_nrf_power_event
	};
	nrfx_power_usbevt_init(&config);

	nrfx_power_usbevt_enable();

	if (usb_reg & POWER_USBREGSTATUS_VBUSDETECT_Msk) {
		tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_DETECTED);
	}

	if (usb_reg & POWER_USBREGSTATUS_OUTPUTRDY_Msk) {
		tusb_hal_nrf_power_event(NRFX_POWER_USB_EVT_READY);
	}
}

void USB::init() {
	Serial.printf("Initializing USB : %s\n");
	usb_hardware_init();
	Serial.println("Initialized hardware usb");
	xTaskCreateStatic(
		usb_device_task,
		"usbd",
		USBD_STACK_SIZE,
		nullptr,
		configMAX_PRIORITIES - 1,
		usb_device_stack,
		&usb_device_taskdef
	);
}

void USB::write(uint8_t* data, int len) {
	if (!tud_hid_ready()) {
		Serial.printf("usb not ready!\n");
		return;
	}

	// Serial.printf("sending usb data\n");
	// for (int i = 0; i < len; i++) Serial.print(data[i], HEX);
	// Serial.println();

	if (tud_hid_report(0, data, len)) {
		//Serial.println("SENT!");
	}
	else {
		Serial.println("FAILED TO SEND!");
	}
}
