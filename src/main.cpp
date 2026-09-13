#include <Arduino.h>
#include "./midi/midi.h"
#include "./beam/VL53L1X_sensor.h"
#include "./screen/oled.h"
#include "./audio/audio_engine.h"
#include "./audio/mp3_streamer.h"
#include "./audio/i2s_manager.h"
#include "./storage/sd_manager.h"
#include "./storage/instrument_manager.h"
#include "config.h"
#include "./switches/switches.h"
#include "./debug.h"
#include "./storage/sd_logger.h"

float sampleVolume = 1.0f;
int loadedSamples = 0;


void setup() {
    resetDisplay();  // reset display controller ASAP, before Serial delay/other init

    #ifdef DEBUG_ON
        Serial.begin(115200);
        delay(1000);
    #endif
    DEBUG("Starting Distance-to-Audio project...");

    initI2S();
    initSD();
    initSDLogger();
    initMP3Streamer();
    initVoices();

    xTaskCreatePinnedToCore(
        audioTaskCode,
        "AudioTask",
        4096,
        NULL,
        1,
        &audioTask,
        0
    );

    setupVL53L1X();
    setupOLED();
    setupMIDI();
    setupSwitches();

    readStandaloneSwitch();

    processRotarySwitches();

    pinMode(MIN_DISTANCE_PIN, INPUT);
    pinMode(MAX_DISTANCE_PIN, INPUT);
    pinMode(VOLUME_PIN, INPUT);
    pinMode(BALANCE_PIN, INPUT);

    DEBUG("Setup complete.");
    DEBUGF("Distance range: %d-%d mm\n", rangeMinimum, rangeMaximum);

    // Force initial display update
    bool showPrompt = !isStandaloneMode() && !hasColoredButtonBeenPressed();
    drawOLED(scaleNames[currentScaleIndex],
            keyNames[currentKeyIndex],
            numberOfNotes,
            rangeMinimum/10,
            rangeMaximum/10,
            showPrompt);
}

void setVolume() {
    int volumePot = analogRead(VOLUME_PIN);
    int balancePot = analogRead(BALANCE_PIN);

    static float smoothedVolume = 0.5f;
    static float smoothedBalance = 0.5f;
    const float VOLUME_SMOOTHING = 0.5f;   // 0-1, lower = more smoothing
    const float BALANCE_SMOOTHING = 0.3f;  // Balance can be a bit more responsive

    // Smooth the volume (master level)
    float targetVolume = map(volumePot, 0, 4095, 0, 100) / 100.0f;
    smoothedVolume = smoothedVolume + VOLUME_SMOOTHING * (targetVolume - smoothedVolume);

    // Smooth the balance (0.0 = full MP3, 0.5 = equal mix, 1.0 = full samples)
    float targetBalance = map(balancePot, 0, 4095, 0, 100) / 100.0f;
    smoothedBalance = smoothedBalance + BALANCE_SMOOTHING * (targetBalance - smoothedBalance);

    // Calculate individual volumes using EQUAL-POWER crossfade
    // This keeps perceived loudness more constant across the balance range
    float mp3Balance = cos(smoothedBalance * PI / 2.0f);   // Goes from 1.0 to 0.0
    float sampleBalance = sin(smoothedBalance * PI / 2.0f); // Goes from 0.0 to 1.0

    // Apply master volume to each channel
    float mp3Volume = smoothedVolume * mp3Balance;
    float sampleVolume = smoothedVolume * sampleBalance;

    // Set the volumes
    setMP3Volume(mp3Volume);
    setSampleVolume(sampleVolume);

    // Optional: Debug output (remove or comment out after testing)
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 500) {  // Print every 500ms
        lastDebug = millis();
    }
}

void distanceControls(int& smoothMin, int& smoothMax) {
    // Read pots
    int minPot = analogRead(MIN_DISTANCE_PIN);
    int maxPot = analogRead(MAX_DISTANCE_PIN);

    // Map MIN_DISTANCE_PIN: 0mm to 1000mm (0-100cm)
    int minDistance = map(4095 - minPot, 0, 4095, 0, 1000);

    // Map MAX_DISTANCE_PIN: 0mm to 1000mm (0-100cm)
    int maxDistance = map(4095 - maxPot, 0, 4095, 0, 1000);

    // Ensure max is always greater than min
    if (maxDistance <= minDistance) {
        maxDistance = minDistance + 50;  // Keep at least 50mm gap (was 100mm)
    }

    // Smooth the values (only update if change is significant)
    const int threshold = 10;  // Reduced from 20mm to 10mm for finer control
    if (abs(minDistance - smoothMin) > threshold) {
        smoothMin = minDistance;
    }
    if (abs(maxDistance - smoothMax) > threshold) {
        smoothMax = maxDistance;
    }

    // Set global range variables
    rangeMinimum = smoothMin;
    rangeMaximum = smoothMax;
}

void loop() {
    static int lastNote = -1;
    static int smoothedMinDistance = 500;
    static int smoothedMaxDistance = 1500;

    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 20) {
        // OLD:
        // int noteNumber = readSensor();

        // NEW:
        int noteNumber = readVL53L1X();

        checkNote(noteNumber);

        if (noteNumber != lastNote){
            lastNote = noteNumber;
        }
        lastSensorRead = millis();
    }

    // Update everything else every 50ms (20 times/second - plenty for UI)
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 50) {
        processRotarySwitches();  // This now also updates track selection
        readTransportControls();  //  Check play/pause/stop buttons
        processColoredButtons();

        bool standaloneState = readStandaloneSwitch();

        setVolume();

        int balance = analogRead(BALANCE_PIN);

        distanceControls(smoothedMinDistance, smoothedMaxDistance);

        // Display-only adaptive smoothing: small moves (pot noise) get smoothed
        // hard so the numbers sit solid; large moves (real pot turns) get
        // followed quickly so the display catches up. The actual reading
        // (rangeMinimum/rangeMaximum) is unaffected - this only stabilises the UI.
        static float dispMin = smoothedMinDistance;
        static float dispMax = smoothedMaxDistance;

        const float NOISE_BAND = 55.0f;  // mm - below this treat as noise
        const float SLOW = 0.05f;        // heavy smoothing for noise
        const float FAST = 0.5f;         // light smoothing for real movement

        float diffMin = fabsf(smoothedMinDistance - dispMin);
        float diffMax = fabsf(smoothedMaxDistance - dispMax);
        float kMin = (diffMin > NOISE_BAND) ? FAST : SLOW;
        float kMax = (diffMax > NOISE_BAND) ? FAST : SLOW;

        dispMin += kMin * (smoothedMinDistance - dispMin);
        dispMax += kMax * (smoothedMaxDistance - dispMax);
        int dispMinInt = (int)(dispMin + 0.5f);
        int dispMaxInt = (int)(dispMax + 0.5f);

        // Display logic based on standalone mode
        // Only update display if not currently processing a button press
        if (!isProcessingButton) {
            if (!isStandaloneMode()) {
                // Standalone OFF mode - show track/level display
                if (trackLevelDisplay.isActive) {
                    // Show track and level info
                    drawTrackAndLevel(trackLevelDisplay.showError,
                                    trackLevelDisplay.trackName.c_str(),
                                    trackLevelDisplay.levelName.c_str(),
                                    trackLevelDisplay.availableLevels.c_str(),
                                    dispMinInt,
                                    dispMaxInt);
                } else {
                    // Show "Please choose a Sounds of Intent level" prompt
                    bool showPrompt = true;
                    drawOLED(scaleNames[currentScaleIndex], keyNames[currentKeyIndex], numberOfNotes,
                           dispMinInt/10, dispMaxInt/10, showPrompt);
                }
            } else {
                // Standalone ON mode - show normal display (scale, key, notes, etc.)
                bool showPrompt = false;
                drawOLED(scaleNames[currentScaleIndex], keyNames[currentKeyIndex], numberOfNotes,
                       dispMinInt/10, dispMaxInt/10, showPrompt);
            }
        }

        lastUpdate = millis();
    }

    delay(1);
}
