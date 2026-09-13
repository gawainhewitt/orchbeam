#pragma once

#include <Arduino.h>

// Rotary encoder with push button (CLK, DT, SW)
void initEncoder();
int encoderGetRotation();      // Net rotation count (clicks) since last reset
void encoderResetRotation();   // Reset the rotation counter to 0
bool encoderButtonPressed();   // True if the button has been pressed (latched)
void encoderClearButton();     // Clear the button-pressed latch
