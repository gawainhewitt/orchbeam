#include <Arduino.h>
#include "./midi/midi.h"
#include "./beam/VL53L1X_sensor.h"
#include "./screen/oled.h"
#include "./audio/audio_engine.h"
#include "./audio/i2s_manager.h"
#include "./encoder/encoder.h"
#include "./menu/menu.h"
#include "./storage/sd_manager.h"
#include "./storage/instrument_manager.h"
#include "config.h"
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
    initEncoder();

    // Static defaults - the physical UI (shift-register switches + pots) has
    // been removed. These values will be driven by the pushbutton encoder UI
    // in a later stage, which will call the same underlying functions.
    selectInstrumentType(0);   // Default instrument: Flute
    changeScale(9);            // Pentatonic
    changeKey(0);              // C
    changeOctave(4);           // Octave 4
    changeNoteDensity(8);      // 8 notes
    setSampleVolume(0.8f);     // Static master volume
    setRange(500, 1500);       // Static beam range (mm)

    DEBUG("Setup complete.");
    DEBUGF("Distance range: %d-%d mm\n", rangeMinimum, rangeMaximum);

    // Show the encoder menu
    initMenu();
}

void loop() {
    static int lastNote = -1;

    // Read the beam and trigger notes
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 20) {
        int noteNumber = readVL53L1X();
        checkNote(noteNumber);
        if (noteNumber != lastNote){
            lastNote = noteNumber;
        }
        lastSensorRead = millis();
    }

    // Update the encoder menu (reads encoder, updates display)
    menuUpdate();

    delay(1);
}
