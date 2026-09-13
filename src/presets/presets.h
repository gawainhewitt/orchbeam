#pragma once

#include <Arduino.h>

// Special value to indicate no sound available for this combination
#define NO_SOUND -1

// Preset configuration for a track + colored button combination
struct Preset {
    int instrument;      // -1 = no sound, 0+ = valid instrument number
    int octave;          // 2-7 (octave number)
    int scale;           // 0-11 (scale/mode index)
    int divisions;       // 0-11 (maps to 1,3,5,7,9,11,13,15,17,19,21,23 notes)
    int key;             // 0-11 (transposition, 0=C, 1=C#, etc.)
    const char* name;    // Preset name for debugging (e.g., "Soundmaker01 sounds")
};

// Track filenames (12 tracks)
extern const char* trackFilenames[12];

// Sounds of Intent level names
extern const char* soiLevelNames[5];

// Get preset for a given track and colored button
// trackIndex: 0-11, buttonIndex: 0-4 (blue, green, orange, purple, red)
Preset getPreset(int trackIndex, int buttonIndex);

// Apply a preset (loads instrument, sets scale, octave, divisions, key)
// Returns true if successful, false if NO_SOUND
bool applyPreset(const Preset& preset, int trackIndex, int buttonIndex);

// Convert track filename to display name (e.g., "01_Ambient_MUGO.mp3" -> "Ambient Mugo")
String getTrackDisplayName(int trackIndex);

// Get list of available button indices for a track (returns count, fills availableButtons array)
// availableButtons should be an array of size 5
int getAvailableButtons(int trackIndex, int* availableButtons);