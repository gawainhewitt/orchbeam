#pragma once

#include <U8g2lib.h>
#include <string>

// Change from U8X8 to U8G2
extern U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2;

const int firstTab = 0;
const int secondTab = 7;
const int position1 = 0;
const int position2 = 1;
const int position3 = 2;
const int position4 = 3;
const int position5 = 4;
const int position6 = 5;

extern std::string storedScale;
extern std::string storedKey;
extern int storedDivisions;
extern int storedMin;
extern int storedMax;

void resetDisplay();
void setupOLED();
void drawOLED(std::string scale, std::string key, int divisions, int min, int max);
