#include "oled.h"
#include <string>
#include "../config.h"
#include "../storage/instrument_manager.h"
#include "../midi/midi.h"
#include "../switches/switches.h"
#include "../debug.h"

// Define the u8g2 object (note: _F_ means full buffer mode)
// 4-wire SPI, software (bit-banged) SPI so we control the exact pins.
// Using SW_SPI avoids any dependency on the hardware SPI peripheral's
// default pin mapping, which is more robust for our custom wiring.
// The RST pin is a genuine hardware reset that u8g2.begin() pulses,
// so no transistor/VCC-switch reset hack is needed.
U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(
    U8G2_R0,
    DISP_SCK_PIN,   // clock
    DISP_MOSI_PIN,  // data / MOSI
    DISP_CS_PIN,    // CS
    DISP_DC_PIN,    // DC / A0
    DISP_RST_PIN    // RST
);

// Hard-reset the display controller as early as possible on boot.
// u8g2.begin() drives a hardware reset pulse on DISP_RST_PIN, which brings
// the controller to a known-good state. Calling this at the very top of
// setup() shortens the window where the panel is powered but not yet reset
// on a quick power cycle, which reduces the boot static artifact.
void resetDisplay() {
    u8g2.begin();
}

void setupOLED() {
    DEBUG("=== Display Init Start ===");

    esp_reset_reason_t reason = esp_reset_reason();
    DEBUGF("Reset reason: %d\n", reason);

    resetDisplay();
    u8g2.setFlipMode(0);

    u8g2.clearBuffer();
    u8g2.sendBuffer();

    delay(100);  // Give display time to process init

    // Turn display ON
    u8g2.setPowerSave(0);

    delay(100);  // Give display time to turn on

    // Draw splash screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(30, 25, "Orchbeam");
    u8g2.drawStr(15, 40, "Starting...");
    u8g2.sendBuffer();

    delay(500);

    DEBUG("Display initialized");
}

void drawOLED(std::string scale, std::string key, int divisions, int min, int max) {
    static bool initialized = false;
    static std::string prevScale = "", prevKey = "";
    static int prevDivisions = -1, prevMin = -1, prevMax = -1;
    static int prevOctave = -1;
    static int prevInstrumentType = -1;
    static bool prevLoading = false;

    // Only redraw if something changed (including octave, the instrument type,
    // and the loading state - all drawn from globals and so must be tracked here).
    // Tracking the instrument type lets the user scan names by turning the knob;
    // tracking the loading state ensures the loading screen appears/clears reliably.
    bool loading = isInstrumentLoading();
    if (scale == prevScale && key == prevKey && divisions == prevDivisions &&
        min == prevMin && max == prevMax && currentOctaveNumber == prevOctave &&
        getSelectedInstrumentType() == prevInstrumentType &&
        loading == prevLoading && initialized) {
        return;
    }

    prevScale = scale;
    prevKey = key;
    prevDivisions = divisions;
    prevMin = min;
    prevMax = max;
    prevOctave = currentOctaveNumber;
    prevInstrumentType = getSelectedInstrumentType();
    prevLoading = loading;
    initialized = true;

    u8g2.clearBuffer();

    if (isInstrumentLoading()) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 30, "Loading");
        u8g2.drawStr(0, 45, "instrument...");
        u8g2.sendBuffer();
        return;
    }

    // Normal display
    u8g2.setFont(u8g2_font_6x10_tr);

    int y = 10;
    int lineHeight = 10;

    // Line 1: Sound (instrument name)
    u8g2.drawStr(0, y, "Sound: ");

    int selectedType = getSelectedInstrumentType();

    if (selectedType >= 0) {
        const char* instrumentName = getInstrumentTypeName(selectedType);
        u8g2.drawStr(42, y, instrumentName);
    } else {
        Instrument* currentInst = getCurrentInstrument();
        if (currentInst && currentInst->isLoaded) {
            u8g2.drawStr(42, y, currentInst->name.c_str());
        } else {
            u8g2.drawStr(42, y, "None");
        }
    }
    y += lineHeight;

    // Show "Loading..." if instrument is being loaded
    if (isInstrumentLoading()) {
        u8g2.drawStr(0, y, "Loading...");
        y += lineHeight;
    }

    // Line 2: Scale
    u8g2.drawStr(0, y, "Scale: ");
    u8g2.drawStr(42, y, scale.c_str());
    y += lineHeight;

    // Line 3: Key and Octave
    u8g2.drawStr(0, y, "Key: ");
    u8g2.drawStr(30, y, key.c_str());
    u8g2.drawStr(60, y, "Oct: ");
    std::string octaveStr = std::to_string(currentOctaveNumber);
    u8g2.drawStr(90, y, octaveStr.c_str());
    y += lineHeight;

    // Line 4: Divisions
    u8g2.drawStr(0, y, "Div: ");
    std::string divisionsStr = std::to_string(divisions);
    u8g2.drawStr(30, y, divisionsStr.c_str());
    y += lineHeight;

    // Line 5: Min and Max
    u8g2.drawStr(0, y, "Min: ");
    std::string minStr = std::to_string(min);
    u8g2.drawStr(30, y, minStr.c_str());
    u8g2.drawStr(60, y, "Max: ");
    std::string maxStr = std::to_string(max);
    u8g2.drawStr(90, y, maxStr.c_str());

    u8g2.sendBuffer();
}
