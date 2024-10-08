#include <DHT11_lib.h>

// include custom print header for easier
// c-like serial output string formatting
#include "print.h"

// declare and init DHT11 device
DHT11_DEVICE device(2);

void setup() {
	// turn on built-in LED
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// begin serial output
	// wait until serial is established
	Serial.begin(9600);
	while(!Serial);

	// turn off built-in to indicate serial is established
	digitalWrite(LED_BUILTIN, LOW);

	Serial.println("\n\n-----------------------------------\nBoard started...\n");
	//device.debug();
}

void loop() {
	// collect data every 10 seconds
	delay(10000);
	_print("temp: %d C (%d F)\nhumid: %d%%\n\n",	// print formatting string
		(int)device.readtemp(),			// print temperature in Celsius
		CTOFAHR((int)device.readtemp()), 	// print temperature in Farenheit
		(int)(device.readhumid()*100)		// print humidity in percentage
	);
}
