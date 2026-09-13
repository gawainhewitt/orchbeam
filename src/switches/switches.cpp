#include "switches.h"
#include "../config.h"
#include "../debug.h"
#include "../storage/instrument_manager.h"

// Instrument loading state
static int selectedInstrumentType = -1;  // Currently selected (may not be loaded yet)
static int loadedInstrumentType = -1;    // Currently loaded instrument
static unsigned long lastInstrumentChange = 0;
static bool instrumentNeedsLoading = false;
static bool instrumentIsCurrentlyLoading = false;  //  
const unsigned long INSTRUMENT_SETTLE_TIME = 1000; // 1 second

const char* instrumentTypeNames[12] = {
    "Flute", "Horn", "Bassoon", "Organ",
    "Guitar", "Bass", "Vibraphone", "Marimba",
    "Rhodes", "Latin Perc", "African Perc", "Drums"
};

void setupSwitches() {
    
    pinMode(SHIFT_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_DATA_PIN, INPUT_PULLUP);
    
    digitalWrite(SHIFT_LOAD_PIN, HIGH);

    DEBUG("Shift register test ready!");
    DEBUG("Press buttons to test...");
    
}

void readSwitches() {
    const int NUM_CHIPS = 11;
    uint8_t buttonStates[NUM_CHIPS];
    
    // Capture all button states
    digitalWrite(SHIFT_LOAD_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SHIFT_LOAD_PIN, HIGH);
    delayMicroseconds(5);
    
    // Read all bits
    for (int chip = 0; chip < NUM_CHIPS; chip++) {
        buttonStates[chip] = 0;
        
        for (int bit = 0; bit < 8; bit++) {
            int bitValue = digitalRead(SHIFT_DATA_PIN);
            buttonStates[chip] |= (bitValue << (7 - bit));
            
            // Clock pulse
            digitalWrite(SHIFT_CLOCK_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(SHIFT_CLOCK_PIN, LOW);
            delayMicroseconds(10);
        }
    }
    
    // Print button states
    DEBUG("Buttons: ");
    for (int chip = 0; chip < NUM_CHIPS; chip++) {
        for (int bit = 0; bit < 8; bit++) {
            DEBUGF("%s", (buttonStates[chip] & (1 << (7 - bit))) ? "1" : "0");
        }
        if (chip < NUM_CHIPS - 1) DEBUG(" | ");
    }
    DEBUG("");
    
    delay(100);
}

int decodeDivisionsSwitch(uint8_t chip0, uint8_t chip1) {
    // Check chip 0 first (positions 0-7)
    for (int i = 0; i < 8; i++) {
        if (!(chip0 & (1 << i))) {
            return i;
        }
    }
    
    // Check chip 1 (positions 8-11, bits 0-3)
    // Ignore bit 4 which seems to be another switch
    for (int i = 0; i < 4; i++) {
        if (!(chip1 & (1 << i))) {
            return 8 + i;  // Return 8-11
        }
    }
    
    return -1;  // No position selected
}

int decodeTypeSwitch(uint8_t chip5, uint8_t chip4) {
    // Check chip 4 first (positions 0-3, using bits 4-7)
    for (int i = 4; i < 8; i++) {
        if (!(chip4 & (1 << i))) {
            return i - 4;  // Return 0-3
        }
    }
    
    // Check chip 5 (positions 4-11, bits 0-7)
    for (int i = 0; i < 8; i++) {
        if (!(chip5 & (1 << i))) {
            return 4 + i;  // Return 4-11
        }
    }
    
    return -1;  // No position selected
}

int decodeModeSwitch(uint8_t chip3, uint8_t chip4) {
    // Check chip 3 (positions 0-7)
    for (int i = 0; i < 8; i++) {
        if (!(chip3 & (1 << i))) {
            return i;
        }
    }
    
    // Check chip 4 lower bits (positions 8-11, bits 0-3)
    for (int i = 0; i < 4; i++) {
        if (!(chip4 & (1 << i))) {
            return 8 + i;  // Return 8-11
        }
    }
    
    return -1;
}

int decodeTranspositionSwitch(uint8_t chip2, uint8_t chip7) {
    // Positions 0-3: Chip 2 bits in reverse order (3, 2, 1, 0)
    if (!(chip2 & (1 << 3))) return 0;
    if (!(chip2 & (1 << 2))) return 1;
    if (!(chip2 & (1 << 1))) return 2;
    if (!(chip2 & (1 << 0))) return 3;
    
    // Positions 4-7: Chip 2 bits 4-7
    for (int i = 4; i < 8; i++) {
        if (!(chip2 & (1 << i))) {
            return i;
        }
    }
    
    // Positions 8-11: Chip 7 bits 0-3
    for (int i = 0; i < 4; i++) {
        if (!(chip7 & (1 << i))) {
            return 8 + i;
        }
    }
    
    return -1;
}

int decodeOctaveSwitch(uint8_t chip1, uint8_t chip9) {
    // Positions 0-3: Chip 1 bits 4-7
    for (int i = 4; i < 8; i++) {
        if (!(chip1 & (1 << i))) {
            return i - 4;  // Return 0-3
        }
    }
    
    // Positions 4-5: Chip 9 bits 0-1
    for (int i = 0; i < 2; i++) {
        if (!(chip9 & (1 << i))) {
            return 4 + i;  // Return 4-5
        }
    }
    
    return -1;
}

void processRotarySwitches() {
    const int NUM_CHIPS = 10;
    uint8_t buttonStates[NUM_CHIPS];
    
    // Capture all button states
    digitalWrite(SHIFT_LOAD_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SHIFT_LOAD_PIN, HIGH);
    delayMicroseconds(5);
    
    // Read all bits
    for (int chip = 0; chip < NUM_CHIPS; chip++) {
        buttonStates[chip] = 0;
        for (int bit = 0; bit < 8; bit++) {
            int bitValue = digitalRead(SHIFT_DATA_PIN);
            buttonStates[chip] |= (bitValue << (7 - bit));
            digitalWrite(SHIFT_CLOCK_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(SHIFT_CLOCK_PIN, LOW);
            delayMicroseconds(10);
        }
    }
    
    // Decode each switch
    int divisions = decodeDivisionsSwitch(buttonStates[0], buttonStates[1]);
    int typeOfSound = decodeTypeSwitch(buttonStates[5], buttonStates[4]);
    int mode = decodeModeSwitch(buttonStates[3], buttonStates[4]);
    int transposition = decodeTranspositionSwitch(buttonStates[2], buttonStates[7]);
    int octavePos = decodeOctaveSwitch(buttonStates[1], buttonStates[9]);
    
    // Track previous values to detect changes
    static int prevDivisions = -1;
    static int prevMode = -1;
    static int prevTransposition = -1;
    static int prevOctavePos = -1;
    static int prevTypeOfSound = -1;
    
    // Handle instrument type changes
    if (typeOfSound != -1 && typeOfSound != prevTypeOfSound) {
        selectedInstrumentType = typeOfSound;
        lastInstrumentChange = millis();
        instrumentNeedsLoading = true;
        prevTypeOfSound = typeOfSound;
        
        DEBUGF("Instrument selected: %d (waiting to load...)\n", typeOfSound);
    }
    
    // Check if we should START loading (after settle time)
    if (instrumentNeedsLoading && !instrumentIsCurrentlyLoading &&
        (millis() - lastInstrumentChange > INSTRUMENT_SETTLE_TIME)) {
        
        DEBUGF("Starting to load instrument type: %d\n", selectedInstrumentType);
        instrumentIsCurrentlyLoading = true;
        instrumentNeedsLoading = false;
        return;
    }

    // If we're in loading state, do the actual load
    if (instrumentIsCurrentlyLoading && loadedInstrumentType != selectedInstrumentType) {
        DEBUGF("Loading instrument type: %d\n", selectedInstrumentType);
        
        loadInstrumentForType(selectedInstrumentType);
        loadedInstrumentType = selectedInstrumentType;
        
        instrumentIsCurrentlyLoading = false;
        
        DEBUG("Instrument loaded");
    }   
    
    // Update note density when divisions switch changes
    if (divisions != -1 && divisions != prevDivisions) {
        int numNotes = (divisions * 2) + 1;
        changeNoteDensity(numNotes);
        prevDivisions = divisions;
    }
    
    // Update scale when mode switch changes
    if (mode != -1 && mode != prevMode) {
        changeScale(mode);
        prevMode = mode;
    }
    
    // Update key/transposition when transposition switch changes
    if (transposition != -1 && transposition != prevTransposition) {
        changeKey(transposition);
        prevTransposition = transposition;
    }
    
    // Update octave when octave switch changes
    if (octavePos != -1 && octavePos != prevOctavePos) {
        int octaveNum = octavePos + 2;
        changeOctave(octaveNum);
        prevOctavePos = octavePos;
    }
}

int getSelectedInstrumentType() {
    return selectedInstrumentType;
}

bool isInstrumentLoading() {
    // Only report loading during the actual load, not the settle/scan phase, so
    // the user can scan through instrument names by turning the knob without a
    // loading screen. The prevLoading tracking in drawOLED ensures the loading
    // screen appears when the load starts and clears when it finishes.
    bool result = instrumentIsCurrentlyLoading;
    if (result) {
        DEBUG("isInstrumentLoading = TRUE");
    }
    return result;
}

const char* getInstrumentTypeName(int typeIndex) {
    if (typeIndex >= 0 && typeIndex < 12) {
        return instrumentTypeNames[typeIndex];
    }
    return "None";
}
