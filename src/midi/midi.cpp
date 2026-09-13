#include "midi.h"
#include "../debug.h"
#include "../audio/audio_engine.h"

// Global variables
bool isConnected = false;
int numberOfNotes = 8;
const int maxNumberOfNotes = 24;
bool noteStatus[24] = {false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
int octave = 48;
int keyPosition = 0;


uint8_t currentScale[24] = {0,2,4,7,9,12,14,16,19,21,24,26,28,31,33,36,38,40,43,45,48,50,52,55}; // pentatonic default
int currentScaleIndex = 9; // Default to pentatonic (matches your default scale)
const char* scaleNames[12] = {
    "Maj Arp", "Min Arp", "Dom 7th", "Maj 7th",
    "Diminish", "Chromatic", "Major", "Minor",
    "Blues", "Pentatonic", "Whole Tone", "Octotonic"
};

int currentKeyIndex = 0; // Default to C (no transposition)
const char* keyNames[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

int currentOctaveNumber = 4; // Default to octave 4 (middle C range)


void onConnected() {
    isConnected = true;
    DEBUG("MIDI Connected");
}

void onDisconnected() {
    isConnected = false;
    DEBUG("MIDI Disconnected");
}

void setupMIDI() {
    // Initialize BLE MIDI
    BLEMidiServer.begin("Orchbeam");

    // Set up callbacks
    BLEMidiServer.setOnConnectCallback(onConnected);
    BLEMidiServer.setOnDisconnectCallback(onDisconnected);

    DEBUG("BLE MIDI Server started");
}

void playNote(int notePosition) {
    int theNote = currentScale[notePosition] + octave + keyPosition;
    // Trigger audio sample
    noteOn(theNote, 100);  // from audio_engine.h
    BLEMidiServer.noteOn(0, theNote, 100);
    // Serial.printf("Note ON: %d\n", theNote);
    noteStatus[notePosition] = true;
}

void stopNote(int notePosition) {
    int theNote = currentScale[notePosition] + octave + keyPosition;
    // Send via BLE (channel 0, note, velocity)
    // Stop audio sample
    noteOff(theNote);  // from audio_engine.h
    BLEMidiServer.noteOff(0, theNote, 100);
    // Serial.printf("Note OFF: %d\n", theNote);
    noteStatus[notePosition] = false;

}

void allNotesOff() {
    // Send All Notes Off control change (CC 123)
    BLEMidiServer.controlChange(0, 123, 0);
    DEBUG("All Notes Off");
}

void checkNote(int notePosition) {
    static int lastStableNote = -1;
    static int lastSeenNote = -1;
    static int noteConfirmCount = 0;
    const int CONFIRM_THRESHOLD = 2;  // Need 2 consistent readings

    if (notePosition == -1) {
        for (int i = 0; i < maxNumberOfNotes; i++) {
            stopNote(i);
        }
        lastStableNote = -1;
        lastSeenNote = -1;
        noteConfirmCount = 0;
        return;
    }

    // If same as current stable note, keep playing it
    if (notePosition == lastStableNote) {
        return;
    }

    // Count consistent readings of the new note
    if (notePosition == lastSeenNote) {
        noteConfirmCount++;
    } else {
        lastSeenNote = notePosition;
        noteConfirmCount = 1;
    }

    // Once we have enough consistent readings, switch to the new note
    if (noteConfirmCount >= CONFIRM_THRESHOLD) {
        // Switch notes
        for (int i = 0; i < numberOfNotes; i++){
            if(i != notePosition){
                if(noteStatus[i]){
                    stopNote(i);
                }
            } else {
                if(!noteStatus[i]){
                    playNote(i);
                }
            }
        }
        lastStableNote = notePosition;
        noteConfirmCount = 0;
    }
}

void changeNoteDensity(int8_t densityNumber) {
    if (densityNumber <= maxNumberOfNotes && densityNumber > 0) {
        numberOfNotes = densityNumber;
        DEBUGF("Note density changed to: %d\n", numberOfNotes);
    }
}

void changeScale(int scaleIndex) {
    // Define all 12 scales (intervals from root note)
    const uint8_t scales[12][24] = {
        // 0: Major arpeggio (1, 3, 5, 8)
        {0,4,7,12,16,19,24,28,31,36,40,43,48,52,55,60,64,67,72,76,79,84,88,91},

        // 1: Minor arpeggio (1, b3, 5, 8)
        {0,3,7,12,15,19,24,27,31,36,39,43,48,51,55,60,63,67,72,75,79,84,87,91},

        // 2: Dominant 7th (1, 3, 5, b7, 8)
        {0,4,7,10,12,16,19,22,24,28,31,34,36,40,43,46,48,52,55,58,60,64,67,70},

        // 3: Major 7th (1, 3, 5, 7, 8)
        {0,4,7,11,12,16,19,23,24,28,31,35,36,40,43,47,48,52,55,59,60,64,67,71},

        // 4: Diminished scale (whole-half pattern: 1, 2, b3, 4, b5, b6, 6, 7)
        {0,2,3,5,6,8,9,11,12,14,15,17,18,20,21,23,24,26,27,29,30,32,33,35},

        // 5: Chromatic scale (all semitones)
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23},

        // 6: Major scale (1, 2, 3, 4, 5, 6, 7, 8)
        {0,2,4,5,7,9,11,12,14,16,17,19,21,23,24,26,28,29,31,33,35,36,38,40},

        // 7: Minor scale (1, 2, b3, 4, 5, b6, b7, 8)
        {0,2,3,5,7,8,10,12,14,15,17,19,20,22,24,26,27,29,31,32,34,36,38,39},

        // 8: Blues scale (1, b3, 4, b5, 5, b7, 8)
        {0,3,5,6,7,10,12,15,17,18,19,22,24,27,29,30,31,34,36,39,41,42,43,46},

        // 9: Pentatonic scale (1, 2, 3, 5, 6, 8)
        {0,2,4,7,9,12,14,16,19,21,24,26,28,31,33,36,38,40,43,45,48,50,52,55},

        // 10: Whole tone scale (all whole steps)
        {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46},

        // 11: Octotonic scale (half-whole pattern: 1, b2, b3, 3, b5, 5, 6, b7)
        {0,1,3,4,6,7,9,10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,33,34}
    };

    if (scaleIndex >= 0 && scaleIndex < 12) {
        // Copy the selected scale to currentScale
        memcpy(currentScale, scales[scaleIndex], sizeof(currentScale));
        currentScaleIndex = scaleIndex;  // Store the current scale index

        DEBUGF("Scale changed to: %s\n", scaleNames[scaleIndex]);
    }
}

void changeKey(int keyIndex) {
    if (keyIndex >= 0 && keyIndex < 12) {
        // keyPosition is the semitone offset from C
        keyPosition = keyIndex;
        currentKeyIndex = keyIndex;

        DEBUGF("Key changed to: %s (transpose +%d semitones)\n",
                      keyNames[keyIndex], keyPosition);
    }
}

void changeOctave(int octaveNumber) {
    if (octaveNumber >= 2 && octaveNumber <= 7) {
        // Convert octave number to MIDI note offset
        // Octave 0 = MIDI 0-11, Octave 1 = 12-23, etc.
        // So Octave 2 = MIDI 24, Octave 3 = 36, Octave 4 = 48...
        octave = octaveNumber * 12;
        currentOctaveNumber = octaveNumber;

        // Kill any sounding notes so the octave change is clean
        allNotesOff();
        stopAllVoices();

        DEBUGF("Octave changed to: %d (MIDI base: %d)\n",
                      octaveNumber, octave);
    }
}
