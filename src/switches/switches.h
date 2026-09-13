#pragma once

#include <Arduino.h>
#include "../midi/midi.h"

// SN74HC165N

void setupSwitches();
void readSwitches();
int decodeDivisionsSwitch(uint8_t chip0, uint8_t chip1);
int decodeTypeSwitch(uint8_t chip5, uint8_t chip4);
int decodeModeSwitch(uint8_t chip3, uint8_t chip4);
int decodeTranspositionSwitch(uint8_t chip2, uint8_t chip7);
int decodeOctaveSwitch(uint8_t chip1, uint8_t chip9);
void processRotarySwitches();
int getSelectedInstrumentType();
bool isInstrumentLoading();
const char* getInstrumentTypeName(int typeIndex);
