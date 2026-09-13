

still not working

im going to check that it still works on the main branch so we can be sure that the sd card works

and then go back to the attempt 

this is the last serial output

​	 Connected!

Starting Distance-to-Audio project...

I2S initialized successfully

Initializing SD card (native ESP-IDF SDMMC driver)...

Pins CLK:12 CMD:11 D0:10 D1:9 D2:14 D3:13

Attempting 4-bit mode @ 10000 kHz...

E (1533) sdmmc_sd: sdmmc_init_sd_scr: send_scr (1) returned 0x107

E (1533) vfs_fat_sdmmc: sdmmc_card_init failed (0x107).

 mount failed: ESP_ERR_TIMEOUT (0x107)

4-bit failed -> retrying in 1-bit mode...

E (1670) sdmmc_sd: sdmmc_init_sd_scr: send_scr (1) returned 0x107

E (1670) vfs_fat_sdmmc: sdmmc_card_init failed (0x107).

 mount failed: ESP_ERR_TIMEOUT (0x107)

SD FAILED to initialize (both 4-bit and 1-bit).

 \- Check card is FAT32-formatted and seated

 \- Verify 4-bit wiring D0-D3 (external pull-ups on breakout)

 \- Verify CMD/CLK connections

sdOpen failed: /sdcard/debug.log (w)

Initializing MP3 streamer...

MP3 buffer allocated: 352800 samples (4.0 seconds)

MP3 mutex created: 0x3fcec894

Initializing VL53L1X Time-of-Flight sensor...

[ 1855][I][esp32-hal-i2c.c:75] i2cInit(): Initialising I2C Master: sda=15 scl=16 freq=400000

VL53L1X sensor initialized successfully

Distance mode: Short (up to 1360mm, best light rejection)



Timing budget: 33ms (~30Hz)



VL53L1X ready - Range: 200-800 mm

=== Display Init Start ===

Reset reason: 1

Display initialized

I NimBLEDevice: BLE Host Task Started

I NimBLEDevice: NimBle host synced.

BLE MIDI Server started

Shift register test ready!

Press buttons to test...

Standalone Mode: ON (backing tracks disabled)

Instrument selected: 4 (waiting to load...)

Note density changed to: 7

Scale changed to: Dom 7th

Key changed to: D (transpose +2 semitones)

Octave changed to: 5 (MIDI base: 60)

Track selected: 0 - /music/01_Ambient.mp3

Setup complete.

Distance range: 200-800 mm

Starting to load instrument type: 4

Loading instrument type: 4

All Notes Off

Created instrument: Guitar (index 0)

sdOpen failed: /sdcard/instruments/guitar/guitarE2.wav (rb)

Failed to open file: /instruments/guitar/guitarE2.wav

Failed to load sample instruments/guitar/guitarE2.wav

Loaded Guitar with 0 samples

Selected instrument: Guitar

Instrument loaded

No suitable sample found for MIDI note 72

No suitable sample found for MIDI note 69

No suitable sample found for MIDI note 66

No suitable sample found for MIDI note 69

No suitable sample found for MIDI note 74

No suitable sample found for MIDI note 72

No suitable sample found for MIDI note 69

No suitable sample found for MIDI note 66

No suitable sample found for MIDI note 69





