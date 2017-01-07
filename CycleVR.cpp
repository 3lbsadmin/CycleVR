/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
 *********************************************************************/

#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

//#include "src/Adafruit_BluefruitLE/Adafruit_BLE.h"
//#include "src/Adafruit_BluefruitLE/Adafruit_BluefruitLE_SPI.h"
#include "src/Adafruit_BluefruitLE/Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

/*=========================================================================
 APPLICATION SETTINGS

 FACTORYRESET_ENABLE       Perform a factory reset when running this sketch

 Enabling this will put your Bluefruit LE module
 in a 'known good' state and clear any config
 data set in previous sketches or projects, so
 running this at least once is a good idea.

 When deploying your project, however, you will
 want to disable factory reset by setting this
 value to 0.  If you are making changes to your
 Bluefruit LE device via AT commands, and those
 changes aren't persisting across resets, this
 is the reason why.  Factory reset will erase
 the non-volatile memory where config data is
 stored, setting it back to factory default
 values.

 Some sketches that require you to bond to a
 central device (HID mouse, keyboard, etc.)
 won't work at all with this feature enabled
 since the factory reset will clear all of the
 bonding data stored on the chip, meaning the
 central device won't be able to reconnect.
 MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
 MODE_LED_BEHAVIOUR        LED activity, valid options are
 "DISABLE" or "MODE" or "BLEUART" or
 "HWUART"  or "SPI"  or "MANUAL"
 -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    (char *)"0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN,
BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(Serial1, BLUEFRUIT_UART_MODE_PIN);
/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
// A small helper
void error(const __FlashStringHelper *err) {
	Serial.println(err);
	while (1)
		;
}

#define HALL_PIN A2
#define DELTA 200

bool rotation = false;

/**************************************************************************/
/*!
 @brief  Sets up the HW an the BLE module (this function is called
 automatically on startup)
 */
/**************************************************************************/
void setup(void) {

	// required for Flora & Micro
	while (!Serial)
		delay(500);

	Serial.begin(115200);
	Serial.println(F("Adafruit Bluefruit Command Mode Example"));
	Serial.println(F("---------------------------------------"));

	/* Initialise the module */
	Serial.print(F("Initialising the Bluefruit LE module: "));

	if (!ble.begin(VERBOSE_MODE)) {
		error(
				F(
						"Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
	}
	Serial.println(F("OK!"));

	if ( FACTORYRESET_ENABLE) {
		/* Perform a factory reset to make sure everything is in a known state */
		Serial.println(F("Performing a factory reset: "));
		if (!ble.factoryReset()) {
			error(F("Couldn't factory reset"));
		}
	}

	/* Disable command echo from Bluefruit */
	ble.echo(false);

	Serial.println("Requesting Bluefruit info:");
	/* Print Bluefruit information */
	ble.info();

	Serial.println(F("Setting device name to 'VR Exercise Bike': "));
	if (!ble.sendCommandCheckOK(F("AT+GAPDEVNAME=VR Exercise Bike"))) {
		error(F("Could not set device name?"));
	}

	Serial.println(
			F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
	Serial.println(F("Then Enter characters to send to Bluefruit"));
	Serial.println();

	ble.verbose(false);  // debug info is a little annoying after this point!

	/* Wait for connection */
	while (!ble.isConnected()) {
		delay(500);
	}

	// LED Activity command is only supported from 0.6.6
	if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
		// Change Mode LED Activity
		Serial.println(F("******************************"));
		Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
		ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
		Serial.println(F("******************************"));
	}

	pinMode(HALL_PIN, INPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
}

/**************************************************************************/
/*!
 @brief  Checks for user input (via the Serial Monitor)
 */
/**************************************************************************/
bool getUserInput(char buffer[], uint8_t maxSize) {
	// timeout in 100 milliseconds
	TimeoutTimer timeout(100);

	memset(buffer, 0, maxSize);
	while ((!Serial.available()) && !timeout.expired()) {
		delay(1);
	}

	if (timeout.expired())
		return false;

	delay(2);
	uint8_t count = 0;
	do {
		count += Serial.readBytes(buffer + count, maxSize);
		delay(2);
	} while ((count < maxSize) && (Serial.available()));

	return true;
}

/**************************************************************************/
/*!
 @brief  Constantly poll for new command or response data
 */
/**************************************************************************/
void loop(void) {
	// Check for user input
	/*char inputs[BUFSIZE + 1];

	 if (getUserInput(inputs, BUFSIZE)) {
	 // Send characters to Bluefruit
	 Serial.print("[Send] ");
	 Serial.println(inputs);




	 ble.print("AT+BLEUARTTX=");
	 ble.println(inputs);

	 // check response stastus

	 if (!ble.waitForOK()) {
	 Serial.println(F("Failed to send?"));
	 }
	 }*/

	// Check for exercise bike wheel rotation.  Hall Effect Sensor

	///ble.print("AT+BLEUARTTX=");
	//ble.println(2);
	//delay( 500 );
	int signal = analogRead(HALL_PIN);
	Serial.println(signal);
	if (signal > DELTA && rotation == false) {
		rotation = true;
		ble.print("AT+BLEUARTTX=");
		ble.println(2);

		if (!ble.waitForOK()) {
			Serial.println(F("Failed to send?"));
		}
	} else if (signal < DELTA && rotation == true) {
		rotation = false;
	}

	// Check for incoming characters from Bluefruit
	ble.println("AT+BLEUARTRX");
	ble.readline();
	if (strcmp(ble.buffer, "OK") == 0) {
		// no data
		return;
	}
	// Some data was found, its in the buffer
	Serial.print(F("[Recv] "));
	Serial.println(ble.buffer);
	ble.waitForOK();
}

