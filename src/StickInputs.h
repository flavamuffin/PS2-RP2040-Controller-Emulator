#ifndef StickInputs_h
#define StickInputs_h

#include "PS2Controller.h"  // for IButtonSource and PsxButtons definitions

// StickInputs reads buttons via RP2040 GPIOs
class StickInputs : public IButtonSource
{
	private:
		// GPIO pin assignments
		static constexpr uint8_t TRIANGLE_PIN = 26;
		static constexpr uint8_t SQUARE_PIN   = 9;
		static constexpr uint8_t CIRCLE_PIN   = 3;
		static constexpr uint8_t CROSS_PIN    = 4;

		static constexpr uint8_t UP_PIN     = 7;
		static constexpr uint8_t DOWN_PIN   = 8;
		static constexpr uint8_t LEFT_PIN   = 5;
		static constexpr uint8_t RIGHT_PIN  = 6;

		static constexpr uint8_t SELECT_PIN = 12;
		static constexpr uint8_t START_PIN  = 10;

		// Cached button state flags (true = pressed, active-low wiring)
		bool trianglePressed = false;
		bool squarePressed   = false;
		bool circlePressed   = false;
		bool crossPressed    = false;
		bool upPressed       = false;
		bool downPressed     = false;
		bool leftPressed     = false;
		bool rightPressed    = false;
		bool selectPressed   = false;
		bool startPressed    = false;

	public:
		// Initialize I2C expanders and configure GPIOs for face buttons
		void Begin();

		// Read all inputs and update internal boolean states
		void Update();

		// Provide a snapshot of current button states (called by Ps2Controller)
		void Snapshot(Ps2Buttons &outButtons) override;
};

#endif
