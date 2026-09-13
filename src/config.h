#pragma once

// Debug configuration
#define DEBUG_ON

// Audio configuration
#define SAMPLE_RATE           44100
#define MAX_POLYPHONY         8
#define MAX_SAMPLES           16
#define MAX_INSTRUMENTS       4           // Maximum number of instruments

// I2S pins for UDA1334A DAC
#define I2S_BCLK_PIN    7
#define I2S_DOUT_PIN    6
#define I2S_WCLK_PIN    5

// SD Card 4 bit pins:
#define SDMMC_CMD   11
#define SDMMC_CLK   12
#define SDMMC_D0    13
#define SDMMC_D1    14
#define SDMMC_D2    9
#define SDMMC_D3    10

// VL53L1X Time-of-Flight sensor (I2C)
#define VL53L1X_SDA_PIN  15  // Purple wire
#define VL53L1X_SCL_PIN  16  // Grey wire

// SPI pins for OLED display
#define DISP_SCK_PIN    18
#define DISP_MOSI_PIN   17
#define DISP_CS_PIN     21
#define DISP_DC_PIN     42
#define DISP_RST_PIN    41

// Rotary encoder with push button (CLK, DT, SW)
#define ENC_CLK_PIN  38
#define ENC_DT_PIN   39
#define ENC_SW_PIN   40

// Audio settings
#define DMA_BUF_LEN     256
#define DMA_NUM_BUF     8

// Global audio settings
extern float sampleVolume;
extern int loadedSamples;
