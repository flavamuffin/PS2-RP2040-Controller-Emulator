// PS2Controller.h
#ifndef PS2Controller_h
#define PS2Controller_h

#include <hardware/pio.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>
#include <hardware/sync.h>

// Logical button state (true = pressed)
struct Ps2Buttons
{
    bool selectPressed;
    bool startPressed;

    bool upPressed;
    bool rightPressed;
    bool downPressed;
    bool leftPressed;

    bool trianglePressed;
    bool circlePressed;
    bool crossPressed;
    bool squarePressed;
};

// Abstract source of button states (StickInputs)
class IButtonSource
{
	public:
		virtual ~IButtonSource() = default;
		virtual void Snapshot(Ps2Buttons &outButtons) = 0;
};

// Pin configuration for the PS2 controller interface
struct Ps2Pins
{
    uint8_t acknowledge;
    uint8_t command;
    uint8_t attention;
    uint8_t clock;
    uint8_t data;
};


class Ps2Controller
{
	public:
		Ps2Controller();

		void Begin(const Ps2Pins &pins);

		// Attach a button source (StickInputs)
		void SetButtonSource(IButtonSource *source);


  private:
    uint8_t currentFrameBytesBuffer[6];

    // Singleton-style pointer used by the global GPIO IRQ callback
    static Ps2Controller *uniqueInstance;

    // State machine indices
    int smFrameKeeper, smTimeKeeper, smRX, smTX;

    // Pin configuration
    Ps2Pins pins;

    // External button source
    IButtonSource *buttonSource;

    // Frame transaction tracking.
    volatile uint32_t currentFrameAcknowledgements;		// ACK pulses seen in current frame
    volatile bool     frameActive;						// True while the attention pin is low


    // Index of the next byte within the current frame
    volatile uint8_t  currentFrameByteIndex;

    // Internal helpers.
    bool    RXFIFOEmpty();
    uint8_t RxFifoContent();

    void ConfigureInterrupts();
    static void ProcessFrameToggle();
    static void ProcessRxByte();

    static void MakeActiveLowBytes(const Ps2Buttons &buttons, uint8_t &lowByte, uint8_t &highByte);
    static void PrintHexByte(uint8_t value);

    int ConfigureFrameKeeperSM();
    int ConfigureTimeKeeperSM();
    int ConfigureRXSM();
    int ConfigureTXSM();
};

#endif
