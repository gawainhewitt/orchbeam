#pragma once

#include <BLEMidi.h>  // Only include BLE MIDI for now

// Forward declarations
extern bool isConnected;
extern int numberOfNotes;
extern const int maxNumberOfNotes;
extern bool noteStatus[24];
extern int octave;
extern int keyPosition;
extern uint8_t currentScale[24];
extern int currentScaleIndex;
extern const char* scaleNames[12];
extern int currentKeyIndex;
extern const char* keyNames[12];
extern int currentOctaveNumber;

// Function declarations
void setupMIDI();
void playNote(int theNote);
void stopNote(int theNote);
void allNotesOff();
void checkNote(int noteIndex);
void changeNoteDensity(int8_t densityNumber);
void changeScale(int scaleIndex);
void changeKey(int keyIndex);
void changeOctave(int octaveNumber);