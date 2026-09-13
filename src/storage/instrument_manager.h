#pragma once

#include <Arduino.h>
#include "instrument.h"

#define MAX_INSTRUMENTS 4

extern Instrument instruments[MAX_INSTRUMENTS];
extern int currentInstrument;
extern int loadedInstruments;

// Load a key sample into an instrument
bool loadKeySample(int instrumentIndex, const char* filename, uint8_t rootNote, uint8_t minNote, uint8_t maxNote);

// Create a new instrument
int createInstrument(const char* name);

// Select current instrument
void selectInstrument(int instrumentIndex);

// Get the current instrument
Instrument* getCurrentInstrument();

void loadFlute();
void loadHorn();
void loadBassoon();
void loadOrgan();
void loadGuitar();
void loadBass();
void loadVibraphone();
void loadMarimba();
void loadRhodes();
void loadLatinPercussion();
void loadAfricanPercussion();
void loadBasicDrumKit();

void loadSoundmaker01();
void loadSoundmaker02();
void loadSoundmaker03();
void loadSoundmaker04();
void loadSoundmaker05();
void loadSoundmaker06();
void loadSoundmaker07();
void loadSoundmaker08();
void loadSoundmaker09();
void loadSoundmaker10();

void loadPatternmaker03();
void loadPatternmaker04();
void loadPatternmaker05();
void loadPatternmaker06();
void loadPatternmaker08();
void loadPatternmaker09();
void loadPatternmaker10();

void loadMotifmaker06();
void loadMotifmaker07();
void loadMotifmaker08();
void loadMotifmaker09();
void loadMotifmaker10();

void unloadCurrentInstrument();

void loadInstrumentForType(int typePosition);