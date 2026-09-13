#include "VL53L1X_sensor.h"
#include "../config.h"
#include "../debug.h"
#include "../midi/midi.h"

// Create separate I2C bus for sensor
TwoWire I2C_Sensor = TwoWire(1);

// Create sensor object
VL53L1X sensor;

// Range settings
int rangeMinimum = 200;     // Minimum distance in mm
int rangeMaximum = 800;    // Maximum distance in mm

void setupVL53L1X() {
    DEBUG("Initializing VL53L1X Time-of-Flight sensor...");
    
    // Initialize I2C bus on separate pins
    I2C_Sensor.begin(VL53L1X_SDA_PIN, VL53L1X_SCL_PIN, 400000);  // 400kHz
    delay(100);
    
    // Configure sensor to use our I2C bus
    sensor.setBus(&I2C_Sensor);
    sensor.setTimeout(500);
    
    // Initialize sensor
    if (!sensor.init()) {
        DEBUG("ERROR: VL53L1X initialization failed!");
        DEBUG("Check wiring and I2C connections");
        return;
    }
    
    DEBUG("VL53L1X sensor initialized successfully");
    
    // Configure for long range mode (up to 4m)
    sensor.setDistanceMode(VL53L1X::Short);
    DEBUG("Distance mode: Short (up to 1360mm, best light rejection)\n");
    
    // Set measurement timing budget (33ms = ~30Hz update rate)
    // Lower values = faster but less accurate
    // Higher values = slower but more accurate
    sensor.setMeasurementTimingBudget(33000);
    DEBUG("Timing budget: 33ms (~30Hz)\n");
    
    // Start continuous ranging
    sensor.startContinuous(33);
    
    DEBUGF("VL53L1X ready - Range: %d-%d mm\n", rangeMinimum, rangeMaximum);
}

int readVL53L1X() {
    uint16_t distance = sensor.read();
    
    if (sensor.timeoutOccurred()) {
        return -1;
    }
    
    if (distance < rangeMinimum || distance > rangeMaximum) {
        return -1;
    }
    
    // Direct linear mapping - no dead zones
    int workingDistance = rangeMaximum - rangeMinimum;
    float normalizedDistance = (float)(distance - rangeMinimum) / workingDistance;
    int notePosition = (int)(normalizedDistance * numberOfNotes);
    
    // Clamp to valid range
    if (notePosition >= numberOfNotes) {
        notePosition = numberOfNotes - 1;
    }
    if (notePosition < 0) {
        notePosition = 0;
    }
    
    return notePosition;
}

void setRange(int min, int max) {
    int minToSet = min;
    int maxToSet = max;

    if (minToSet >= maxToSet) {
        minToSet = maxToSet - 10;
    }

    if (minToSet < 0) {
        minToSet = 0;
    }

    rangeMinimum = minToSet;
    rangeMaximum = maxToSet;
    
    DEBUGF("Range updated: %d-%d mm\n", rangeMinimum, rangeMaximum);
}