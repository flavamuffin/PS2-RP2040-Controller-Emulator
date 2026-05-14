// PS2Controller.cpp - new implementation using RX / TX / ACK PIO state machines
#include <Arduino.h>
#include "PS2Controller.h"
#include "PIOSM.h"
#include "hardware/structs/pio.h"


// Static instance pointer used by the GPIO IRQ callback.
Ps2Controller *Ps2Controller::uniqueInstance = nullptr;

Ps2Controller::Ps2Controller() : smFrameKeeper(-1), smTimeKeeper(-1), smRX(-1), smTX(-1), buttonSource(nullptr), currentFrameAcknowledgements(0), frameActive(false), currentFrameByteIndex(0)
{
    pins = { 0, 0, 0, 0, 0 };
}


// --- Frame Keeper state machine setup ---
int Ps2Controller::ConfigureFrameKeeperSM()
{
    // Claim state machine
    int stateMachineIndex = pio_claim_unused_sm(pio0, true);
    if ((stateMachineIndex == -1) && Serial) { Serial.println("[ERROR] Failed to claim state machine for the frame keeper"); }

    // Load program into PIO instruction memory
    int memoryOffset = pio_add_program(pio0, &frame_keeper_program);
    if ((memoryOffset == -1) && Serial) { Serial.println("[ERROR] Failed to load frame keeper into PIO instruction memory"); }

    // Get the configuration to apply to the state machine
    pio_sm_config stateMachineConfig = FrameKeeperStateMachineConfig(memoryOffset);

    // Initialize state machine
    pio_sm_init(pio0, stateMachineIndex, memoryOffset, &stateMachineConfig);

    // Returns the state machine index so the machine can be started later
    return stateMachineIndex;
}


// --- Time Keeper state machine setup ---
int Ps2Controller::ConfigureTimeKeeperSM()
{
    // Claim state machine
    int stateMachineIndex = pio_claim_unused_sm(pio0, true);
    if ((stateMachineIndex == -1) && Serial) { Serial.println("[ERROR] Failed to claim state machine for the time keeper"); }

    // Load program into PIO instruction memory
    int memoryOffset = pio_add_program(pio0, &time_keeper_program);
    if ((memoryOffset == -1) && Serial) { Serial.println("[ERROR] Failed to load time keeper into PIO instruction memory"); }

    // Get the configuration to apply to the state machine
    pio_sm_config stateMachineConfig = TimeKeeperStateMachineConfig(memoryOffset);

    // Initialize state machine
    pio_sm_init(pio0, stateMachineIndex, memoryOffset, &stateMachineConfig);

    // Returns the state machine index so the machine can be started later
    return stateMachineIndex;
}

// --- RX state machine setup (interface_rx) ---
int Ps2Controller::ConfigureRXSM()
{
    // Claim state machine
    int stateMachineIndex = pio_claim_unused_sm(pio0, true);
    if ((stateMachineIndex == -1) && Serial) { Serial.println("[ERROR] Failed to claim state machine for RX program"); }

    // Configures the command, and attention pins as inputs
    pio_sm_set_pindirs_with_mask(pio0, stateMachineIndex, 0, (1u << pins.command) | (1u << pins.attention));

    // Load program into PIO instruction memory
    int memoryOffset = pio_add_program(pio0, &interface_rx_program);
    if ((memoryOffset == -1) && Serial) { Serial.println("[ERROR] Failed to load RX program into PIO instruction memory"); }

    // Get the configuration to apply to the state machine
    pio_sm_config stateMachineConfig = RXStateMachineConfig(memoryOffset, pins.command, pins.attention);

    // Initialize state machine
    pio_sm_init(pio0, stateMachineIndex, memoryOffset, &stateMachineConfig);

    // Returns the state machine index so the machine can be started later
    return stateMachineIndex;
}

// --- TX state machine setup (interface_tx) ---
int Ps2Controller::ConfigureTXSM()
{
    // Claim state machine
    int stateMachineIndex = pio_claim_unused_sm(pio0, true);
    if ((stateMachineIndex == -1) && Serial) { Serial.println("[ERROR] Failed to claim state machine for TX program"); }

    // Configures the acknowledge, data, and clock pins as inputs
    pio_sm_set_pindirs_with_mask(pio0, stateMachineIndex, 0, (1u << pins.acknowledge) | (1u << pins.data) | (1u << pins.clock));

    // Load program into PIO instruction memory
    int memoryOffset = pio_add_program(pio0, &interface_tx_program);
    if ((memoryOffset == -1) && Serial) { Serial.println("[ERROR] Failed to load TX program into PIO instruction memory"); }

    // Get the configuration to apply to the state machine
    pio_sm_config stateMachineConfig = TXStateMachineConfig(memoryOffset, pins.acknowledge, pins.attention, pins.data);

    // Initialize state machine
    pio_sm_init(pio0, stateMachineIndex, memoryOffset, &stateMachineConfig);

    // Returns the state machine index so the machine can be started later
    return stateMachineIndex;
}


void Ps2Controller::ConfigureInterrupts()
{
    // Enable interrupt sources
    pio_set_irq0_source_enabled(pio0, pis_interrupt0, true);
    pio_set_irq0_source_enabled(pio0, pis_interrupt1, true);

    irq_set_exclusive_handler(PIO0_IRQ_0, Ps2Controller::ProcessFrameToggle);
    irq_set_enabled(PIO0_IRQ_0, true);

    // Set the interrupt that occurs when a byte has been received
    pio_interrupt_source_t byteReadyInterruptSource = pio_get_rx_fifo_not_empty_interrupt_source(smRX);
    pio_set_irq1_source_enabled(pio0, byteReadyInterruptSource, true);

    irq_set_exclusive_handler(PIO0_IRQ_1, Ps2Controller::ProcessRxByte);
    irq_set_enabled(PIO0_IRQ_1, true);
}

// Parse PS2 button information into two separate bytes
void Ps2Controller::MakeActiveLowBytes(const Ps2Buttons &buttons, uint8_t &lowByte, uint8_t &highByte)
{
    lowByte = 0xFF;
    highByte = 0xFF;

    // Byte 0 (low): bit0 = Select, bit3 = Start, bit4 = Up, bit5 = Right, bit6 = Down, bit7 = Left
    if (buttons.selectPressed) { lowByte &= static_cast<uint8_t>(~(1u << 0)); }
    if (buttons.startPressed)  { lowByte &= static_cast<uint8_t>(~(1u << 3)); }
    if (buttons.upPressed)     { lowByte &= static_cast<uint8_t>(~(1u << 4)); }
    if (buttons.rightPressed)  { lowByte &= static_cast<uint8_t>(~(1u << 5)); }
    if (buttons.downPressed)   { lowByte &= static_cast<uint8_t>(~(1u << 6)); }
    if (buttons.leftPressed)   { lowByte &= static_cast<uint8_t>(~(1u << 7)); }

    // Byte 1 (high): bit4 = Triangle, bit5 = Circle, bit6 = Cross, bit7 = Square
    if (buttons.trianglePressed) { highByte &= static_cast<uint8_t>(~(1u << 4)); }
    if (buttons.circlePressed)   { highByte &= static_cast<uint8_t>(~(1u << 5)); }
    if (buttons.crossPressed)    { highByte &= static_cast<uint8_t>(~(1u << 6)); }
    if (buttons.squarePressed)   { highByte &= static_cast<uint8_t>(~(1u << 7)); }
}

void Ps2Controller::PrintHexByte(uint8_t value)
{
    static constexpr char HEX_DIGITS[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    char buf[3];
    buf[0] = HEX_DIGITS[(value >> 4) & 0x0F];
    buf[1] = HEX_DIGITS[value & 0x0F];
    buf[2] = '\0';
    Serial.print(buf);
}

// Configure and start the PIO state machines
void Ps2Controller::Begin(const Ps2Pins &pins)
{
    uniqueInstance = this;
    this->pins = pins;

    // Initialize GPIOs for pio use
    pio_gpio_init(pio0, pins.data);
    pio_gpio_init(pio0, pins.clock);
    pio_gpio_init(pio0, pins.attention);
    pio_gpio_init(pio0, pins.command);
    pio_gpio_init(pio0, pins.acknowledge);

    // Configure the state machines
    smFrameKeeper = ConfigureFrameKeeperSM();
    smTimeKeeper  = ConfigureTimeKeeperSM();
    smRX          = ConfigureRXSM();
    smTX          = ConfigureTXSM();

	// Configure the interrupts
    ConfigureInterrupts();

    // Enables the state machines
    pio_enable_sm_mask_in_sync(pio0, (1u << smFrameKeeper) | (1u << smTimeKeeper) | (1u << smRX) | (1u << smTX));
}

void Ps2Controller::ProcessFrameToggle()
{
    Ps2Controller *self = uniqueInstance;

	// Clears IRQ flags and returns if the static instance pointer is not set
    if (self == nullptr)
    {
        pio_interrupt_clear(pio0, 0);
        pio_interrupt_clear(pio0, 1);
        return;
    }

    // Frame start (Attention pin falling)
    if (pio_interrupt_get(pio0, 0))
    {
        pio_interrupt_clear(pio0, 0);
        self->frameActive  = true;
        self->currentFrameByteIndex = 0;
        self->currentFrameAcknowledgements = 0;
        return;
    }

	// Frame end (Attention pin rising)
    if (pio_interrupt_get(pio0, 1))
    {
        pio_interrupt_clear(pio0, 1);
        self->frameActive = false;
    }
}

bool Ps2Controller::RXFIFOEmpty() { return pio_sm_is_rx_fifo_empty(pio0, static_cast<uint>(smRX)); }

uint8_t Ps2Controller::RxFifoContent()
{
    // Return 0 if the program is not assigned to a state machine
    if (smRX < 0) { return 0; }

    // Returns 0 if there is nothing in the FIFO
    if (pio_sm_is_rx_fifo_empty(pio0, static_cast<uint>(smRX))) { return 0; }

    uint32_t rawValue = pio_sm_get(pio0, static_cast<uint>(smRX));
    uint8_t byteValue = static_cast<uint8_t>((rawValue >> 24) & 0xFFu);

    return byteValue;
}

void Ps2Controller::ProcessRxByte()
{
    Ps2Controller *self = uniqueInstance;
    if (self == nullptr) { return; }

	const uint8_t CONTROLLER_REQUEST_BYTE_VALUE = 0x01;		// The PlayStation sends this value when it to speak to a controller
	const uint8_t POLL_COMMAND_BYTE_VALUE = 0x42;			// The PlayStation sends this value when it wants to poll a digital controller

    const uint8_t DEVICE_MODE_BYTE_VALUE  = 0x41;			// Standard digital pad ID and payload length nibble
    const uint8_t PADDING_BYTE_VALUE = 0x5A;				// Constant padding byte

    // Processes every byte in the RX fifo
    while (!self->RXFIFOEmpty())
    {
        uint8_t byteValue = self->RxFifoContent();
        uint8_t index = self->currentFrameByteIndex;

        // Chooses a response based off of the current position in the frame
        if (index == 0u)
        {
            // First byte must be 0x01 to be acknowledged
            if (byteValue == CONTROLLER_REQUEST_BYTE_VALUE)
            {
            	// Pack the byte into a 32 bit word and pass it to the transmit state machine
                uint32_t packed = static_cast<uint32_t>(~DEVICE_MODE_BYTE_VALUE  & 0xFFu);
                pio_sm_put(pio0, self->smTX, packed);
                self->currentFrameAcknowledgements++;
            }
        }
        else if (index == 1u)
        {
            // Second byte must be 0x42 to be acknowledged
            if (byteValue == POLL_COMMAND_BYTE_VALUE)
            {
            	// Packs the byte into a 32 bit word and passes it to the transmit state machine
                uint32_t packedByte = static_cast<uint32_t>(~PADDING_BYTE_VALUE  & 0xFFu);
                pio_sm_put(pio0, self->smTX, packedByte);
                self->currentFrameAcknowledgements++;
            }
        }
        // Third and fourth bytes are always acknowledged if the first two were
        else if ((self->currentFrameAcknowledgements > 1))
        {
            Ps2Buttons snapshot{};
            if (self->buttonSource != nullptr) { self->buttonSource->Snapshot(snapshot); }

            uint8_t lowByte = 0xFF;
            uint8_t highByte = 0xFF;
            MakeActiveLowBytes(snapshot, lowByte, highByte);

            if (index == 2u)
            {
            	// Packs the byte into a 32 bit word and passes it to the transmit state machine
                uint32_t packedByte = static_cast<uint32_t>(~lowByte  & 0xFFu);
                pio_sm_put(pio0, self->smTX, packedByte);
                self->currentFrameAcknowledgements++;
            }
            else if (index == 3u)
            {
            	// Packs the byte into a 32 bit word and passes it to the transmit state machine
                uint32_t packedByte = static_cast<uint32_t>(~highByte  & 0xFFu);
                pio_sm_put(pio0, self->smTX, packedByte);
                self->currentFrameAcknowledgements++;
            }
        }

        // Any further bytes (index >= 4) are never acknowledged in a digital button poll response
        self->currentFrameByteIndex++;
    }
}

void Ps2Controller::SetButtonSource(IButtonSource *source) { buttonSource = source; }
