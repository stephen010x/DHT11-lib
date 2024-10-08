#include <stdint.h>
#include <Arduino.h>
#include <math.h>

#include "DHT11_lib.h"

#define ENABLE_PRINT
#include "print.h"

static_assert(HIGH == 1, "'HIGH' macro mismatch");
static_assert(LOW  == 0, "'LOW' macro mismatch");
#ifndef NAN
static_assert(false, "NaN is not supported");
#endif

// weird endian-ness going on here
// a tale of caution with macros is variable name overlap
#define SETBITOR(ptr, index, n) do{ typeof(index) __i = (index); ((uint8_t*)(ptr))[__i/8] |= (n) << (7-(__i&0x7)); }while(0)
#define SETBITAND(ptr, index, n) do{ typeof(index) __i = (index); ((uint8_t*)(ptr))[__i/8] &= (n) << (7-(__i&0x7)); }while(0)
#define READBIT(ptr, index) ({ typeof(index) __i = (index); ((uint8_t*)(ptr))[__i/8] & (0b1 << (7-(__i&0x7))) ? 1 : 0; })
#define SWAP(a, b) do{ typeof(a) __t = (a); (a) = (b); (b) = __t; }while(0)
#define LENOF(n) (sizeof(n)/sizeof(*(n)))

// consider getting rid of LIVE() in favor of being more explicit
// the ambiguity between micros and millis hardly makes this worth it.
//#define LIVE(start) (micros() - (start))
// actually, just get rid of these in general. The problems caused by
// signed overflow makes these problematic!
//#define LIVEM(start) ((signed long)(millis() - (unsigned long)(start)))
//#define LIVEU(start) ((signed long)(micros() - (unsigned long)(start)))
// consider writing a big int class + a timer class to properly handle
// these in all use cases!

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"


//////////////////////////
// DHT11_WORD METHODS

// returns nonzero if checksum fails
uint8_t DHT11_WORD::checksum() {
	register typeof(raw) &raw = this->raw;
	return raw[0]+raw[1]+raw[2]+raw[3] != this->sum;
}


//////////////////////////
// DHT11_DEVICE METHODS

// C++ be cursed like this.
DHT11_DEVICE::DHT11_DEVICE() : DHT11_DEVICE(DEFAULT_DATA_PIN) {}

DHT11_DEVICE::DHT11_DEVICE(uint8_t dpin) {
	//*this = {0}; // apparently this is not valid code, courtesy of C++
	//this->zerotemp = 0;
	//this->zerohumid = 0;
	this->last_sample = 0;
	this->dpin = dpin;
}

int8_t DHT11_DEVICE::sendstart() {
	// send the start signal
	pinMode(this->dpin, OUTPUT);
	digitalWrite(this->dpin, LOW);
	delay(18);
	digitalWrite(this->dpin, HIGH);
	
	// set to pull high input
	pinMode(this->dpin, INPUT);
	digitalWrite(this->dpin, HIGH);
	return 0;
}

// returns nonzero if not ready
int8_t DHT11_DEVICE::isready() {
	// if called before at least one second since program
	// start, return error.
	// (to prevent calling before DHT passes unstable status
	// if called before SAMPLE_DELAY timeup, return error
	// docs says max polling rate for device is one second!
	if (millis() < STARTUP_DELAY ||
			millis() - this->last_sample < SAMPLE_DELAY)
		return DHT11_ERR_NOT_READY;
	return 0;
}

// makes sure the class stored word is only updated upon success
int8_t DHT11_DEVICE::updateword() {
	DHT11_WORD word = {0};

	// returns error if device not ready
	if (int8_t err = this->isready())
		return err;
	
	// estimated last_sample in case of error return
	// estimated communication time according to docs is 4ms
	// consider replacing with a class that defers these with a deconstructor
	this->last_sample = millis() + 4000;
	
	// send start
	this->sendstart();
	
	// start reading bits (exits sendstart() on HIGH)
	// i = -3 to account for the precursing signal bit flips
	// first bit (HIGH) starts at i==0
	// last bit (LOW) starts at i=80
	for (int8_t i = -3; i <= 80; i++) {
		uint16_t live;
		unsigned long start = micros(); // time rollover during transmission is highly unlikely
		
		// wait for next bit
		while (digitalRead(this->dpin) == ((uint8_t)i)%2) {
			if ((live = micros() - start) > 90)
				return DHT11_ERR_TIMEOUT;}
		
		// store bit in word
		if (i%2 && i >= 0)
			SETBITOR(word.raw, (uint8_t)(i>>1), live>50);
	}
	this->last_sample = millis();

	// check sum
	if (word.checksum())
		return DHT11_ERR_CHECKSUM;

	// adjust and return
	this->word = word;
	this->formatword();
	return DHT11_SUCCESS;
}

// mostly for debugging
void DHT11_DEVICE::graphsig() {
	// returns error if device not ready
	if (int8_t err = this->isready())
		return err;

	// create and zero out graph bit buffer
	struct block{uint8_t _[56];};
	uint8_t buff[56];
	*(block*)&buff = {0};
	
	// send start signal
	this->sendstart();
	// plot signal bits
	for(uint16_t i = 0; i < sizeof(buff)*8; i++) {
		SETBITOR(buff, i, digitalRead(this->dpin));
		delayMicroseconds(3);
	}
	this->last_sample = millis();
	
	// print signal bits
	for(uint16_t i = 0; i < sizeof(buff)*8; i++) {
		Serial.print("signal:");
		Serial.println(READBIT(buff, i));
	}
}

void DHT11_DEVICE::formatword() {
	DHT11_WORD &word = this->word;
	// why are the low bytes fractions of 10???
	this->humid = ((float)word.humid.high + (float)word.humid.low / 10) / 1e2;
	this->temp = ((float)word.temp.high + (float)word.temp.low / 10) * (word.temp.sign ? -1.0 : 1.0);
}

/*float DHT11_DEVICE::readtemp() {
	int8_t err = this->updateword();
	return err == DHT11_SUCCESS || err == DHT11_ERR_NOT_READY ?
		(float)(((int32_t)this->word.temp) - ((int32_t)this->zerotemp) << 8) / 256.0 : NAN;
}*/

// these need to return all nonzero errors except ready errors
float DHT11_DEVICE::readtemp() {
	int8_t err = this->updateword();
	//*debug*/_print("%s, %d\n", DHT11_DEVICE::geterr(err), this->word.temp.high);
	return err == DHT11_SUCCESS || err == DHT11_ERR_NOT_READY ? this->temp : NAN;
}

float DHT11_DEVICE::readhumid() {
	int8_t err = this->updateword();
	//*debug*/_print("%s, %d\n", DHT11_DEVICE::geterr(err), this->word.humid.high);
	return err == DHT11_SUCCESS || err == DHT11_ERR_NOT_READY ? this->humid : NAN;
}

/*int8_t DHT11_DEVICE::calibrate_temp(int16_t zero) {
	this->zerotemp = zero;
	return 0;
}

int8_t DHT11_DEVICE::calibrate_humid(int16_t zero) {
	this->zerohumid = zero;
	return 0;
}*/

const char* DHT11_DEVICE::geterr(int8_t err) {
	char* str;
	switch (err) {
		case DHT11_SUCCESS:
			return "DHT11_SUCCESS";
		case DHT11_ERR_TIMEOUT:
			return "DHT11_ERR_TIMEOUT";
		case DHT11_ERR_CHECKSUM:
			return "DHT11_ERR_CHECKSUM";
		case DHT11_ERR_NOT_READY:
			return "DHT11_ERR_NOT_READY";
		default:
			return "DHT11_UNKNOWN_ERROR";
	}
	return (char*)NULL;
}

void DHT11_DEVICE::debug() {
	_print("Starting DHT11::debug()\n");
	_print("%s\n", DHT11_DEVICE::geterr(this->isready()));
	//delay(2000);
	//this->graphsig();
	delay(2000);
	_print("%s\n", DHT11_DEVICE::geterr(this->updateword()));
	_print("data: ");
	printbuff(this->word.raw, LENOF(this->word.raw));
	_print("\n");
	delay(1234);
	_print("Temp: ");
	Serial.println(this->readtemp());
	delay(1000);
	_print("Humid: ");
	Serial.println(this->readhumid());
	_print("%s\n", DHT11_DEVICE::geterr(this->isready()));
	_print("%s\n", DHT11_DEVICE::geterr(this->isready()));
	delay(1000);
	_print("%s\n", DHT11_DEVICE::geterr(this->isready()));
}

#pragma GCC diagnostic pop
