#include <Arduino.h>
#undef max
#undef min

#include "device/usbd.h"
#include "system/usb/USB.h"

uint8_t response[64] = {0, 1, 0, 0};
void usbCallback(uint8_t* rxData, uint32_t readLength) {
	Serial.printf("got data %d\n", readLength);
	//send it back
	USB::write(response, 64);
}

void setup() {
	// for our device
	Serial.setPins(21, 17);
	// for devkit
	// Serial.setPins(32+1, 32+2);

	Serial.begin(115200);
	Serial.printf("Starting nrf52 usb tests\n");

	USB::init();
	USB::callback = usbCallback;
}

void loop() {

	Serial.printf("USB TICK! %x %x %x\n", NRF_POWER->USBREGSTATUS, NRF_USBD->INTEN, NRF_USBD->EVENTCAUSE);
	if (tud_suspended()) {
		Serial.println("SUSPENDED!");
		tud_remote_wakeup();
	}

	delay(1000);
}