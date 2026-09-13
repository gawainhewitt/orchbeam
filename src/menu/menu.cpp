#include "menu.h"
#include "../config.h"
#include "../debug.h"
#include "../encoder/encoder.h"
#include "../screen/oled.h"
#include "../midi/midi.h"
#include "../storage/instrument_manager.h"
#include "../beam/VL53L1X_sensor.h"
#include "../audio/audio_engine.h"

// Menu items
enum MenuItem {
    MENU_INSTRUMENT,
    MENU_SCALE,
    MENU_KEY,
    MENU_OCTAVE,
    MENU_NOTES,
    MENU_VOLUME,
    MENU_RANGE_MIN,
    MENU_RANGE_MAX,
    MENU_COUNT
};

static int currentItem = 0;
static bool editing = false;

// Value bounds and step size for each item
static const int itemMin[MENU_COUNT]  = {0,   0,  0, 2, 1,   0,   0,   0};
static const int itemMax[MENU_COUNT]  = {11, 11, 11, 7, 24, 100, 1000, 1000};
static const int itemStep[MENU_COUNT] = {1,   1,  1, 1, 1,   5,   50,   50};

static int getItemValue(int item) {
    switch (item) {
        case MENU_INSTRUMENT: return getSelectedInstrumentType();
        case MENU_SCALE:      return currentScaleIndex;
        case MENU_KEY:        return currentKeyIndex;
        case MENU_OCTAVE:     return currentOctaveNumber;
        case MENU_NOTES:      return numberOfNotes;
        case MENU_VOLUME:     return (int)(sampleVolume * 100.0f);
        case MENU_RANGE_MIN:  return rangeMinimum;
        case MENU_RANGE_MAX:  return rangeMaximum;
    }
    return 0;
}

static void setItemValue(int item, int value) {
    switch (item) {
        case MENU_INSTRUMENT: selectInstrumentType(value); break;
        case MENU_SCALE:      changeScale(value); break;
        case MENU_KEY:        changeKey(value); break;
        case MENU_OCTAVE:     changeOctave(value); break;
        case MENU_NOTES:      changeNoteDensity(value); break;
        case MENU_VOLUME:     setSampleVolume(value / 100.0f); break;
        case MENU_RANGE_MIN:  setRange(value, rangeMaximum); break;
        case MENU_RANGE_MAX:  setRange(rangeMinimum, value); break;
    }
}

static const char* getItemLabel(int item) {
    switch (item) {
        case MENU_INSTRUMENT: return "Instrument";
        case MENU_SCALE:      return "Scale";
        case MENU_KEY:        return "Key";
        case MENU_OCTAVE:     return "Octave";
        case MENU_NOTES:      return "Notes";
        case MENU_VOLUME:     return "Volume";
        case MENU_RANGE_MIN:  return "Range Min";
        case MENU_RANGE_MAX:  return "Range Max";
    }
    return "";
}

static String getItemValueString(int item) {
    switch (item) {
        case MENU_INSTRUMENT: return getInstrumentTypeName(getSelectedInstrumentType());
        case MENU_SCALE:      return scaleNames[currentScaleIndex];
        case MENU_KEY:        return keyNames[currentKeyIndex];
        case MENU_OCTAVE:     return String(currentOctaveNumber);
        case MENU_NOTES:      return String(numberOfNotes);
        case MENU_VOLUME:     return String((int)(sampleVolume * 100.0f)) + "%";
        case MENU_RANGE_MIN:  return String(rangeMinimum) + "mm";
        case MENU_RANGE_MAX:  return String(rangeMaximum) + "mm";
    }
    return "";
}

static void drawMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);

    if (editing) {
        // Focused edit view
        u8g2.drawStr(0, 10, "> ");
        u8g2.drawStr(12, 10, getItemLabel(currentItem));
        u8g2.drawStr(0, 26, getItemValueString(currentItem).c_str());
        u8g2.drawStr(0, 46, "Rotate: change");
        u8g2.drawStr(0, 56, "Press: confirm");
        u8g2.sendBuffer();
        return;
    }

    // Scrolling list view
    const int VISIBLE = 6;
    int startIndex = currentItem - VISIBLE / 2;
    if (startIndex < 0) startIndex = 0;
    if (startIndex > MENU_COUNT - VISIBLE) startIndex = MENU_COUNT - VISIBLE;

    int y = 10;
    for (int i = startIndex; i < startIndex + VISIBLE && i < MENU_COUNT; i++) {
        String line = (i == currentItem) ? "> " : "  ";
        line += getItemLabel(i);
        line += ": ";
        line += getItemValueString(i);
        u8g2.drawStr(0, y, line.c_str());
        y += 10;
    }
    u8g2.sendBuffer();
}

void initMenu() {
    currentItem = 0;
    editing = false;
    drawMenu();
}

void menuUpdate() {
    int rotation = encoderGetRotation();
    bool button = encoderButtonPressed();
    bool changed = (rotation != 0) || button;

    if (rotation != 0) {
        if (editing) {
            // Change the current item's value
            int value = getItemValue(currentItem) + rotation * itemStep[currentItem];
            value = constrain(value, itemMin[currentItem], itemMax[currentItem]);
            setItemValue(currentItem, value);
        } else {
            // Navigate the list (wraps around)
            currentItem += rotation;
            if (currentItem < 0) currentItem = MENU_COUNT - 1;
            if (currentItem >= MENU_COUNT) currentItem = 0;
        }
        encoderResetRotation();
    }

    if (button) {
        editing = !editing;  // toggle edit mode
        encoderClearButton();
    }

    if (changed) {
        drawMenu();
    }
}
