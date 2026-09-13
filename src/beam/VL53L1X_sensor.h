#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

// Distance mapping settings
extern int rangeMinimum;     // Minimum distance in mm
extern int rangeMaximum;     // Maximum distance in mm

// Initialize the VL53L1X sensor
void setupVL53L1X();

// Read distance and map to note position
// Returns -1 if no valid reading or in dead zone, 0-23 for note positions
int readVL53L1X();

// Set the working range for the sensor
void setRange(int min, int max);