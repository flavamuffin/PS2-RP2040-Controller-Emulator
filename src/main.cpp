#include <Arduino.h>
#include "StickInputs.h"
#include "PS2Controller.h"

static constexpr int SERIAL_BAUD_RATE = 115200;     // Baud rate used for the serial connection
static constexpr int SERIAL_MAX_WAIT_TIME = 500;    // The maximum amount of time (in milliseconds) to wait for the serial connection to initialize before giving up
static constexpr int SERIAL_RETRY_DELAY = 10;       // The amount of milliseconds to wait before retrying to make a serial connection

Ps2Pins pins { 19, 27, 18, 20, 28 };   // acknowledge, command, attention, clock, data

StickInputs stick;
Ps2Controller ps2;

void setup()
{
	// Waits a moment for a possible serial connection
	Serial.begin(SERIAL_BAUD_RATE);
	while (!Serial && millis() < SERIAL_MAX_WAIT_TIME) { delay(SERIAL_RETRY_DELAY); }

	// Starts sampling button states
	stick.Begin();

	// Configures the interface that connects to the PlayStation
	ps2.Begin(pins);
	ps2.SetButtonSource(&stick);

	Serial.println("PS2 PIO digital pad ready.");
}

// Keeps the internally stored button states up to date
void loop() { stick.Update(); }