#ifndef TDE_NRF528XX_USB_H
#define TDE_NRF528XX_USB_H

#include <Arduino.h>
#include "tusb.h"

typedef void (*USBCallback)(uint8_t* rxData, uint32_t readLength);
typedef void (*USBConnectionCallback)();

class USB {
	public:
	static const struct device* device;
	// static uint8_t rxData[64];
	static USBCallback callback;
	static USBConnectionCallback connectedCallback;
	static USBConnectionCallback disconnectedCallback;

	static void init();
	static void write(uint8_t* data, int len);
};

#endif