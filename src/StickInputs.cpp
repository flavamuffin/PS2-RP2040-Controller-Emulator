#include <Arduino.h>
#include "StickInputs.h"

void StickInputs::Begin()
{
  // Configure RP2040 pins for buttons
  pinMode(TRIANGLE_PIN, INPUT_PULLUP);
  pinMode(SQUARE_PIN,   INPUT_PULLUP);
  pinMode(CIRCLE_PIN,   INPUT_PULLUP);
  pinMode(CROSS_PIN,    INPUT_PULLUP);

  pinMode(UP_PIN,    INPUT_PULLUP);
  pinMode(DOWN_PIN,  INPUT_PULLUP);
  pinMode(LEFT_PIN,  INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);

  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(START_PIN,  INPUT_PULLUP);
}

void StickInputs::Update()
{
  // Read face buttons from GPIO (active-low wiring, so invert the read)
  trianglePressed = !digitalRead(TRIANGLE_PIN);
  squarePressed   = !digitalRead(SQUARE_PIN);
  circlePressed   = !digitalRead(CIRCLE_PIN);
  crossPressed    = !digitalRead(CROSS_PIN);

  upPressed       = !digitalRead(UP_PIN);
  downPressed     = !digitalRead(DOWN_PIN);
  leftPressed     = !digitalRead(LEFT_PIN);
  rightPressed    = !digitalRead(RIGHT_PIN);

  selectPressed   = !digitalRead(SELECT_PIN);
  startPressed    = !digitalRead(START_PIN);

  // Debug: log when any button state changes
  static Ps2Buttons lastButtonStates = {};
  Ps2Buttons currentButtonStates = {};
  currentButtonStates.selectPressed   = selectPressed;
  currentButtonStates.startPressed    = startPressed;
  currentButtonStates.upPressed       = upPressed;
  currentButtonStates.downPressed     = downPressed;
  currentButtonStates.leftPressed     = leftPressed;
  currentButtonStates.rightPressed    = rightPressed;
  currentButtonStates.trianglePressed = trianglePressed;
  currentButtonStates.squarePressed   = squarePressed;
  currentButtonStates.circlePressed   = circlePressed;
  currentButtonStates.crossPressed    = crossPressed;
  if (memcmp(&currentButtonStates, &lastButtonStates, sizeof(Ps2Buttons)) != 0)
  {
    if (Serial) { Serial.printf("[INPUT] sel=%d start=%d up=%d down=%d left=%d right=%d tri=%d sq=%d cir=%d cross=%d\n", currentButtonStates.selectPressed, currentButtonStates.startPressed, currentButtonStates.upPressed, currentButtonStates.downPressed, currentButtonStates.leftPressed, currentButtonStates.rightPressed, currentButtonStates.trianglePressed, currentButtonStates.squarePressed, currentButtonStates.circlePressed, currentButtonStates.crossPressed); }
    lastButtonStates = currentButtonStates;
  }
}

void StickInputs::Snapshot(Ps2Buttons &outButtons)
{
  // Copy current states into outButtons with interrupts disabled to avoid race conditions
  uint32_t irqState = save_and_disable_interrupts();
  outButtons.selectPressed   = selectPressed;
  outButtons.startPressed    = startPressed;
  outButtons.upPressed       = upPressed;
  outButtons.downPressed     = downPressed;
  outButtons.leftPressed     = leftPressed;
  outButtons.rightPressed    = rightPressed;
  outButtons.trianglePressed = trianglePressed;
  outButtons.squarePressed   = squarePressed;
  outButtons.circlePressed   = circlePressed;
  outButtons.crossPressed    = crossPressed;
  restore_interrupts(irqState);
}
