#include "switches.h"
#include "../config.h"
#include "../debug.h"
#include "../storage/instrument_manager.h"
#include "../audio/mp3_streamer.h"  
#include "../presets/presets.h"
#include "../screen/oled.h"
#include "../beam/VL53L1X_sensor.h"  // For rangeMinimum, rangeMaximum

// Transport control state tracking
struct TransportState {
    bool playPressed;
    bool pausePressed;
    bool stopPressed;
    bool loopPressed;
    bool toBeginningPressed;
    unsigned long lastDebounceTime;
    int currentTrack;
    bool isPlaying;
};

static TransportState transport = {false, false, false, false, false, 0, -1, false};
static bool standaloneMode = false;  // true = with tracks ON, false = with tracks OFF
static bool coloredButtonPressed = false;  // true = user has selected a Sounds of Intent level
const unsigned long DEBOUNCE_DELAY = 50; // 50ms debounce
// Display state (accessible to main.cpp)
TrackLevelDisplay trackLevelDisplay = {false, false, "", "", ""};
bool isProcessingButton = false;
// Instrument loading state
static int selectedInstrumentType = -1;  // Currently selected (may not be loaded yet)
static int loadedInstrumentType = -1;    // Currently loaded instrument
static unsigned long lastInstrumentChange = 0;
static bool instrumentNeedsLoading = false;
static bool instrumentIsCurrentlyLoading = false;  //  
const unsigned long INSTRUMENT_SETTLE_TIME = 1000; // 1 second
// Track/preset loading state
static int lastSelectedButton = -1;           // Last colored button pressed
static int pendingTrackForPreset = -1;        // Track waiting to load preset
static unsigned long lastTrackChangeTime = 0;
static bool presetNeedsLoading = false;
static bool presetIsCurrentlyLoading = false;
const unsigned long TRACK_SETTLE_TIME = 1000; // 1 second

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

int decodeSelectedTrackSwitch(uint8_t chip6, uint8_t chip7) {
    // Positions 0-3: Chip 6 bits 0-3
    for (int i = 0; i < 4; i++) {
        if (!(chip6 & (1 << i))) {
            return i;
        }
    }
    
    // Positions 4-7: Chip 7 bits 4-7
    for (int i = 4; i < 8; i++) {
        if (!(chip7 & (1 << i))) {
            return i;  // Returns 4-7
        }
    }
    
    // Positions 8-11: Chip 6 bits 4-7
    for (int i = 4; i < 8; i++) {
        if (!(chip6 & (1 << i))) {
            return 4 + i;  // Returns 8-11
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
    int selectedTrack = decodeSelectedTrackSwitch(buttonStates[6], buttonStates[7]);
    
    // Track previous values to detect changes
    static int prevDivisions = -1;
    static int prevMode = -1;
    static int prevTransposition = -1;
    static int prevOctavePos = -1;
    static int prevTypeOfSound = -1;
    static int prevSelectedTrack = -1;
    
    // Only allow instrument/musical parameter changes when standalone mode is ON
    if (isStandaloneMode()) {
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
    
    // Handle track selection changes (always active regardless of mode)
    if (selectedTrack != -1 && selectedTrack != prevSelectedTrack) {
        // Stop any currently playing track
        if (transport.isPlaying) {
            DEBUG("Stopping current track due to track change");
            stopMP3Stream();
            transport.isPlaying = false;
        }
        
        transport.currentTrack = selectedTrack;
        DEBUGF("Track selected: %d - %s\n", selectedTrack, trackFilenames[selectedTrack]);
        
        // If in backing track mode and a button was previously selected, trigger preset reload
if (!isStandaloneMode() && lastSelectedButton >= 0) {
    // Check if preset is available
    Preset preset = getPreset(selectedTrack, lastSelectedButton);
    
    // UPDATE DISPLAY IMMEDIATELY
    trackLevelDisplay.isActive = true;
    trackLevelDisplay.trackName = getTrackDisplayName(selectedTrack);
    trackLevelDisplay.levelName = String(soiLevelNames[lastSelectedButton]);
    
    if (preset.instrument == NO_SOUND) {
        // Show error immediately
        trackLevelDisplay.showError = true;
        
        // Build available levels string
        int availableButtons[5];
        int count = getAvailableButtons(selectedTrack, availableButtons);
        
        trackLevelDisplay.availableLevels = "";
        for (int i = 0; i < count; i++) {
            if (i > 0) trackLevelDisplay.availableLevels += ", ";
            trackLevelDisplay.availableLevels += String(soiLevelNames[availableButtons[i]]);
        }
        
        DEBUG("Preset not available for this track");
    } else {
        // Valid preset - show normally and set up delayed loading
        trackLevelDisplay.showError = false;
        trackLevelDisplay.availableLevels = "";
        
        pendingTrackForPreset = selectedTrack;
        lastTrackChangeTime = millis();
        presetNeedsLoading = true;
        DEBUGF("Will load preset for button %d after settle time\n", lastSelectedButton);
    }
}
        
        prevSelectedTrack = selectedTrack;
    }
    
    // Handle preset loading for track changes (only in backing track mode)
    if (!isStandaloneMode()) {
        // Check if we should START loading preset (after settle time)
        if (presetNeedsLoading && !presetIsCurrentlyLoading &&
            (millis() - lastTrackChangeTime > TRACK_SETTLE_TIME)) {
            
            DEBUGF("Starting to load preset for track %d, button %d\n", 
                         pendingTrackForPreset, lastSelectedButton);
            
            // Get the preset
            Preset preset = getPreset(pendingTrackForPreset, lastSelectedButton);
            
            // Update display state
            trackLevelDisplay.isActive = true;
            trackLevelDisplay.trackName = getTrackDisplayName(pendingTrackForPreset);
            trackLevelDisplay.levelName = String(soiLevelNames[lastSelectedButton]);
            
            if (preset.instrument == NO_SOUND) {
                // Show error
                trackLevelDisplay.showError = true;
                
                // Build available levels string
                int availableButtons[5];
                int count = getAvailableButtons(pendingTrackForPreset, availableButtons);
                
                trackLevelDisplay.availableLevels = "";
                for (int i = 0; i < count; i++) {
                    if (i > 0) trackLevelDisplay.availableLevels += ", ";
                    trackLevelDisplay.availableLevels += String(soiLevelNames[availableButtons[i]]);
                }
                
                DEBUG("Preset not available for this track\n");
                presetNeedsLoading = false;
            } else {
                // Valid preset - set loading flag
                trackLevelDisplay.showError = false;
                trackLevelDisplay.availableLevels = "Loading...";
                presetIsCurrentlyLoading = true;
                presetNeedsLoading = false;
            }
            
            return;  // Exit to allow display update
        }
        
        // If we're in loading state, do the actual load
        if (presetIsCurrentlyLoading) {
            DEBUGF("Loading preset for track %d\n", pendingTrackForPreset);
            
            Preset preset = getPreset(pendingTrackForPreset, lastSelectedButton);
            applyPreset(preset, pendingTrackForPreset, lastSelectedButton);
            
            trackLevelDisplay.availableLevels = "";
            presetIsCurrentlyLoading = false;
            
            DEBUG("Preset loaded\n");
        }
    }
}

void readTransportControls() {

    if (isStandaloneMode()) {
        return;  // Exit early if in standalone mode
    }

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
    
    // Check transport buttons (all use active LOW logic - 0 = pressed)
    bool playNow = !(buttonStates[8] & (1 << 1));    // Chip 8, bit 1
    bool pauseNow = !(buttonStates[8] & (1 << 4));   // Chip 8, bit 4
    bool stopNow = !(buttonStates[9] & (1 << 2));    // Chip 9, bit 2
    bool loopNow = !(buttonStates[8] & (1 << 3));    // Chip 8, bit 3
    bool toBeginningNow = !(buttonStates[9] & (1 << 5));
    
    // Debouncing
    unsigned long currentTime = millis();
    
    // Play button - starts from the beginning, OR resumes if currently paused.
    if (playNow && !transport.playPressed && (currentTime - transport.lastDebounceTime > DEBOUNCE_DELAY)) {
        transport.playPressed = true;
        transport.lastDebounceTime = currentTime;
        
        if (mp3Paused) {
            // Resume from the pause point rather than restarting
            DEBUG("PLAY pressed - Resuming playback");
            resumeMP3();
            transport.isPlaying = true;
        } else if (transport.currentTrack >= 0 && transport.currentTrack < 12) {
            // Start playing the currently selected track from the beginning
            DEBUGF("PLAY pressed - Starting track %d: %s\n", 
                         transport.currentTrack, trackFilenames[transport.currentTrack]);
            startMP3Stream(trackFilenames[transport.currentTrack]);
            transport.isPlaying = true;
        } else {
            DEBUG("PLAY pressed but no valid track selected");
        }
    } else if (!playNow && transport.playPressed) {
        transport.playPressed = false;
    }
    
    // Pause button - toggles pause/resume (resumes from the same spot).
    if (pauseNow && !transport.pausePressed && (currentTime - transport.lastDebounceTime > DEBOUNCE_DELAY)) {
        transport.pausePressed = true;
        transport.lastDebounceTime = currentTime;
        
        if (mp3Paused) {
            DEBUG("PAUSE pressed - Resuming playback");
            resumeMP3();
        } else {
            DEBUG("PAUSE pressed - Pausing playback");
            pauseMP3();
        }
    } else if (!pauseNow && transport.pausePressed) {
        transport.pausePressed = false;
    }
    
    // Stop button
    if (stopNow && !transport.stopPressed && (currentTime - transport.lastDebounceTime > DEBOUNCE_DELAY)) {
        transport.stopPressed = true;
        transport.lastDebounceTime = currentTime;
        
        DEBUG("STOP pressed - Stopping playback");
        stopMP3Stream();
        transport.isPlaying = false;
    } else if (!stopNow && transport.stopPressed) {
        transport.stopPressed = false;
    }

    // Loop button - toggles looping of the current/pending backing track.
    if (loopNow && !transport.loopPressed && (currentTime - transport.lastDebounceTime > DEBOUNCE_DELAY)) {
        transport.loopPressed = true;
        transport.lastDebounceTime = currentTime;
        setMP3Looping(!mp3Looping);
    } else if (!loopNow && transport.loopPressed) {
        transport.loopPressed = false;
    }

    // To Beginning button - restarts the current track from the start.
    if (toBeginningNow && !transport.toBeginningPressed && (currentTime - transport.lastDebounceTime > DEBOUNCE_DELAY)) {
        transport.toBeginningPressed = true;
        transport.lastDebounceTime = currentTime;
        
        if (transport.currentTrack >= 0 && transport.currentTrack < 12) {
            DEBUGF("TO BEGINNING pressed - Restarting track %d: %s\n", 
                         transport.currentTrack, trackFilenames[transport.currentTrack]);
            startMP3Stream(trackFilenames[transport.currentTrack]);
            transport.isPlaying = true;
        } else {
            DEBUG("TO BEGINNING pressed but no valid track selected");
        }
    } else if (!toBeginningNow && transport.toBeginningPressed) {
        transport.toBeginningPressed = false;
    }
}

int getCurrentSelectedTrack() {
    return transport.currentTrack;
}

int readColoredButtons() {
    // Only check colored buttons when in standalone mode OFF (backing tracks enabled)
    if (standaloneMode) {
        return -1;  // Don't check buttons when standalone is ON
    }
    
    const int NUM_CHIPS = 10;
    uint8_t buttonStates[NUM_CHIPS];
    
    // Capture all button states
    digitalWrite(SHIFT_LOAD_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SHIFT_LOAD_PIN, HIGH);
    delayMicroseconds(5);
    
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
    
    // Check each colored button (active LOW - 0 = pressed)
    if (!(buttonStates[8] & (1 << 2))) return 0;  // Blue
    if (!(buttonStates[8] & (1 << 5))) return 1;  // Green
    if (!(buttonStates[8] & (1 << 0))) return 2;  // Orange
    if (!(buttonStates[9] & (1 << 6))) return 3;  // Purple
    if (!(buttonStates[9] & (1 << 4))) return 4;  // Red
    
    return -1;  // No button pressed
}

void processColoredButtons() {
    static int lastButton = -1;
    static bool buttonWasPressed = false;
    static unsigned long lastPressTime = 0;
    const unsigned long DEBOUNCE_DELAY = 200;
    
    int currentButton = readColoredButtons();
    unsigned long currentTime = millis();
    
    if (currentButton != -1 && !buttonWasPressed && 
        (currentTime - lastPressTime > DEBOUNCE_DELAY)) {
        
        isProcessingButton = true;
        
        buttonWasPressed = true;
        lastButton = currentButton;
        lastPressTime = currentTime;
        coloredButtonPressed = true;
        
        DEBUGF("\n=== Colored Button Pressed: %s ===\n", soiLevelNames[currentButton]);
        
        // Get current track
        int currentTrack = transport.currentTrack;
        if (currentTrack < 0 || currentTrack >= 12) {
            DEBUG("WARNING: No valid track selected, defaulting to track 0");
            currentTrack = 0;
        }
        
        DEBUGF("Current track: %d (%s)\n", currentTrack + 1, trackFilenames[currentTrack]);
        
        // Get preset
        Preset preset = getPreset(currentTrack, currentButton);
        
        // Update display state
        trackLevelDisplay.isActive = true;
        trackLevelDisplay.trackName = getTrackDisplayName(currentTrack);
        trackLevelDisplay.levelName = String(soiLevelNames[currentButton]);
        
        // Check if valid preset
        if (preset.instrument == NO_SOUND) {
            // Show error
            trackLevelDisplay.showError = true;
            
            // Build available levels string
            int availableButtons[5];
            int count = getAvailableButtons(currentTrack, availableButtons);
            
            trackLevelDisplay.availableLevels = "";
            for (int i = 0; i < count; i++) {
                if (i > 0) trackLevelDisplay.availableLevels += ", ";
                trackLevelDisplay.availableLevels += String(soiLevelNames[availableButtons[i]]);
            }
            
            DEBUG("Preset not available for this combination\n");
        } else {
            // Show "Loading..."
            trackLevelDisplay.showError = false;
            trackLevelDisplay.availableLevels = "Loading...";
            
            // Force display update
            drawTrackAndLevel(trackLevelDisplay.showError, 
                 trackLevelDisplay.trackName.c_str(),
                 trackLevelDisplay.levelName.c_str(),
                 trackLevelDisplay.availableLevels.c_str(),
                 rangeMinimum,
                 rangeMaximum);
            
            // Wait for OLED to refresh
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Load the instrument
            bool success = applyPreset(preset, currentTrack, currentButton);
            
            // Clear "Loading..."
            trackLevelDisplay.availableLevels = "";
            
            DEBUG("===================================\n");
        }

        lastSelectedButton = currentButton;
        
        isProcessingButton = false;
    }
    else if (currentButton == -1 && buttonWasPressed) {
        buttonWasPressed = false;
    }
}

bool readStandaloneSwitch() {
    const int NUM_CHIPS = 10;
    uint8_t buttonStates[NUM_CHIPS];
    
    // Capture all button states
    digitalWrite(SHIFT_LOAD_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SHIFT_LOAD_PIN, HIGH);
    delayMicroseconds(5);

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

    bool switchState = !(buttonStates[8] & (1 << 7)); // Chip 8, Bit 7
    
    // Track mode changes
    static bool lastState = false;
    static bool initialized = false;  // ADD THIS
    
    // Initialize on first read OR detect changes
    if (!initialized || switchState != lastState) {  // CHANGE THIS LINE
        initialized = true;  // ADD THIS
        standaloneMode = !switchState;   
        
        if (standaloneMode) {
            DEBUG("Standalone Mode: ON (backing tracks disabled)");  
        } else {
            DEBUG("Standalone Mode: OFF (backing tracks enabled)");  
            coloredButtonPressed = false;  
            trackLevelDisplay.isActive = false;  
        }
        
        lastState = switchState;
    }
    
    return standaloneMode;
}

bool isStandaloneMode() {
    return standaloneMode;
}

bool hasColoredButtonBeenPressed() {
    return coloredButtonPressed;
}

void resetColoredButtonState() {
    coloredButtonPressed = false;
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