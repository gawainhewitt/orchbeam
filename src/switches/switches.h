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
int decodeSelectedTrackSwitch(uint8_t chip6, uint8_t chip7);
void processRotarySwitches();
bool readStandaloneSwitch();
bool isStandaloneMode();  // Returns true when standalone switch is ON (with tracks)
bool hasColoredButtonBeenPressed();  // Returns true if a colored button has been selected
void resetColoredButtonState();  // Call this when switching modes
int readColoredButtons();
void processColoredButtons();
// Transport control functions
void readTransportControls();
int getCurrentSelectedTrack();

// Display state for track/level info
struct TrackLevelDisplay {
    bool isActive;              // true when we should show track/level display
    bool showError;             // true if no sound available
    String trackName;           // Track display name
    String levelName;           // Level name (e.g., "Sound Makers")
    String availableLevels;     // Comma-separated list of available levels
};

extern TrackLevelDisplay trackLevelDisplay;
extern bool isProcessingButton;  // Prevents main loop from updating display
int getSelectedInstrumentType();
bool isInstrumentLoading();
const char* getInstrumentTypeName(int typeIndex);