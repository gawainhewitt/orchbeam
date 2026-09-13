#include "presets.h"
#include "../debug.h"
#include "../midi/midi.h"
#include "../storage/instrument_manager.h"

// Track filenames mapping (moved from switches.cpp)
const char* trackFilenames[12] = {
    "/music/01_Ambient.mp3",
    "/music/02_Robot_Boogie.mp3",
    "/music/03_And_Now.mp3",
    "/music/04_Underwater_Soundscape.mp3",
    "/music/05_Starry_Night.mp3",
    "/music/06_Condor.mp3",
    "/music/07_Dance.mp3",
    "/music/08_Water_Blues.mp3",
    "/music/09_Autumn_Elegy.mp3",
    "/music/10_Olympic_Harmony.mp3",
    "/music/11_Well_Tempered_Mugo.mp3",
    "/music/12_Pentatonia.mp3"
};

// Sounds of Intent level names (for buttons)
const char* soiLevelNames[5] = {
    "Sound Makers",           // Blue button
    "Pattern Makers",         // Green button
    "Motif Makers",           // Orange button
    "Music Makers",           // Purple button
    "Advanced Music Makers"   // Red button
};

// Preset table: [12 tracks][5 colored buttons]
// Using placeholder values for now - you'll fill in actual instrument numbers
// Format: {instrument, octave, scale, divisions, key, name}
static const Preset presetTable[12][5] = {
    // Track 0: 01_Ambient_MUGO
    {
        {12, 3, 5, 8, 0, "Track 1 / Sound Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 1: 02_Robot_Boogie
    {
        {13, 3, 5, 8, 0, "Track 2 / Sound Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 2: 03_AND_NOW_MUGO
    {
        {14, 3, 5, 9, 0, "Track 3 / Sound Makers"},
        {22, 3, 5, 1, 0, "Track 3 / Pattern Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 3: 04_Underwater_Soundscape
    {
        {15, 3, 5, 12, 0, "Track 4 / Sound Makers"},
        {23, 3, 5, 3, 0, "Track 4 / Pattern Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 4: 05_Starry_Night
    {
        {16, 3, 5, 2, 0, "Track 5 / Sound Makers"},
        {24, 3, 5, 7, 0, "Track 5 / Pattern Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 5: 06_Condor
    {
        {17, 3, 5, 2, 0, "Track 6 / Sound Makers"},
        {25, 3, 5, 2, 0, "Track 6 / Pattern Makers"},
        {29, 3, 5, 4, 0, "Track 6 / Motif Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 6: 07_Dance
    {
        {18, 3, 5, 8, 0, "Track 7 / Sound Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {30, 3, 5, 5, 0, "Track 7 / Motif Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 7: 08_Water_Blues
    {
        {19, 3, 5, 6, 0, "Track 8 / Sound Makers"},
        {26, 3, 5, 6, 0, "Track 8 / Pattern Makers"},
        {31, 3, 5, 2, 0, "Track 8 / Motif Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 8: 09_Autumn_Elegy
    {
        {20, 3, 5, 7, 0, "Track 9 / Sound Makers"},
        {27, 3, 5, 3, 0, "Track 9 / Pattern Makers"},
        {32, 3, 5, 2, 0, "Track 9 / Motif Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 9: 10_Olympic_Harmony
    {
        {21, 3, 5, 1, 0, "Track 10 / Sound Makers"},
        {28, 3, 5, 3, 0, "Track 10 / Pattern Makers"},
        {33, 3, 5, 5, 0, "Track 10 / Motif Makers"},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""}
    },
    
    // Track 10: 11_Well_Tempered
    {
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {0, 4, 0, 5, 0, "Major arpeggio"},
        {0, 4, 5, 5, 0, "Chromatic Scale on Flute"}
    },
    
    // Track 11: 12_Pentatonia
    {
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {NO_SOUND, 0, 0, 0, 0, ""},
        {5, 2, 6, 5, 0, "Descending major scale (bass)"},
        {0, 4, 9, 5, 0, "Pentatonic Scale on Flute"}
    }
};

Preset getPreset(int trackIndex, int buttonIndex) {
    // Bounds checking
    if (trackIndex < 0 || trackIndex >= 12) trackIndex = 0;
    if (buttonIndex < 0 || buttonIndex >= 5) buttonIndex = 0;
    
    return presetTable[trackIndex][buttonIndex];
}

bool applyPreset(const Preset& preset, int trackIndex, int buttonIndex) {
    DEBUGF("Applying preset: %s (instrument=%d, octave=%d, scale=%d, divisions=%d, key=%d)\n",
                  preset.name, preset.instrument, preset.octave, preset.scale, preset.divisions, preset.key);
    
    // Check if this is a NO_SOUND preset
    if (preset.instrument == NO_SOUND) {
        DEBUG("NO SOUND available for this combination");
        // We'll add the logic to show which buttons ARE available next
        return false;
    }
    
    // Load instrument
    loadInstrumentForType(preset.instrument);
    
    // Set musical parameters
    changeOctave(preset.octave);
    changeScale(preset.scale);
    
    changeNoteDensity(preset.divisions);
    
    changeKey(preset.key);
    
    DEBUG("Preset applied successfully");
    return true;
}

String getTrackDisplayName(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 12) {
        return "Unknown Track";
    }
    
    // Get the filename (e.g., "01_Ambient_MUGO.mp3")
    String filename = String(trackFilenames[trackIndex]);
    
    // Remove "/music/" prefix if present
    if (filename.startsWith("/music/")) {
        filename = filename.substring(7);
    }
    
    // Remove ".mp3" extension
    if (filename.endsWith(".mp3")) {
        filename = filename.substring(0, filename.length() - 4);
    }
    
    // Remove number prefix (e.g., "01_")
    int underscorePos = filename.indexOf('_');
    if (underscorePos >= 0) {
        filename = filename.substring(underscorePos + 1);
    }
    
    // Replace underscores with spaces
    filename.replace('_', ' ');
    
    // Convert to title case (first letter of each word uppercase)
    bool capitalizeNext = true;
    for (int i = 0; i < filename.length(); i++) {
        if (filename[i] == ' ') {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            filename[i] = toupper(filename[i]);
            capitalizeNext = false;
        } else {
            filename[i] = tolower(filename[i]);
        }
    }
    
    return filename;
}

int getAvailableButtons(int trackIndex, int* availableButtons) {
    if (trackIndex < 0 || trackIndex >= 12 || availableButtons == nullptr) {
        return 0;
    }
    
    int count = 0;
    
    // Check each button to see if it has a valid preset
    for (int buttonIdx = 0; buttonIdx < 5; buttonIdx++) {
        Preset preset = getPreset(trackIndex, buttonIdx);
        if (preset.instrument != NO_SOUND) {
            availableButtons[count] = buttonIdx;
            count++;
        }
    }
    
    return count;
}