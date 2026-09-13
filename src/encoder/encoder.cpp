#include "encoder.h"
#include "../config.h"
#include "../debug.h"

// ---------------------------------------------------------------------------
// Classic rotary encoder state machine (the well-known "ttable" decode).
// It only reports a step when a full detent quadrature sequence is detected,
// and it resets cleanly on illegal/bounce transitions - so it's robust against
// contact bounce and gives one count per detent with reliable direction.
// ---------------------------------------------------------------------------

#define DIR_NONE 0x00
#define DIR_CW   0x10
#define DIR_CCW  0x20

#define R_START     0x3
#define R_CW_BEGIN  0x1
#define R_CW_NEXT   0x0
#define R_CW_FINAL  0x2
#define R_CCW_BEGIN 0x6
#define R_CCW_NEXT  0x4
#define R_CCW_FINAL 0x5

static const unsigned char ttable[8][4] = {
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},                // R_CW_NEXT
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_BEGIN,  R_START},                // R_CW_BEGIN
    {R_CW_NEXT,  R_CW_FINAL,  R_CW_FINAL,  R_START | DIR_CW},       // R_CW_FINAL
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},                // R_START
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},                // R_CCW_NEXT
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_FINAL, R_START | DIR_CCW},      // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_BEGIN, R_CCW_BEGIN, R_START},                // R_CCW_BEGIN
    {R_START,    R_START,     R_START,     R_START}                 // ILLEGAL
};

static volatile unsigned int encState = R_START;
static volatile int rotationCount = 0;

// Button press latch (updated from ISR)
static volatile bool buttonPressed = false;
static volatile unsigned long lastButtonTime = 0;

// ISR for CLK/DT: advance the state machine. Note the pin order - DT is the
// high bit and CLK the low bit, matching the ttable decode above.
void IRAM_ATTR encoderISR() {
    unsigned char pinstate = (digitalRead(ENC_DT_PIN) << 1) | digitalRead(ENC_CLK_PIN);
    encState = ttable[encState & 0x07][pinstate];
    if (encState & DIR_CW) {
        rotationCount++;
    } else if (encState & DIR_CCW) {
        rotationCount--;
    }
}

// ISR for the push button (active LOW). Debounced in software.
void IRAM_ATTR encoderSWISR() {
    unsigned long now = millis();
    if (now - lastButtonTime > 50) {
        lastButtonTime = now;
        buttonPressed = true;
    }
}

void initEncoder() {
    pinMode(ENC_CLK_PIN, INPUT_PULLUP);
    pinMode(ENC_DT_PIN, INPUT_PULLUP);
    pinMode(ENC_SW_PIN, INPUT_PULLUP);

    encState = R_START;

    attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_DT_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_SW_PIN), encoderSWISR, FALLING);

    DEBUG("Encoder initialized");
}

int encoderGetRotation() {
    return rotationCount;
}

void encoderResetRotation() {
    rotationCount = 0;
}

bool encoderButtonPressed() {
    return buttonPressed;
}

void encoderClearButton() {
    buttonPressed = false;
}
