# Display: why SPI

We use a 128×64 SSD1309 OLED on a **4-wire SPI** interface.

## Why SPI not I2C

The I2C SSD1309 module had no hardware RESET pin. On quick power
off/on cycles the controller could wedge and the panel stayed blank — a
hardware limitation that software could not clear. We briefly worked
around it with a PNP transistor switching the display's VCC under
firmware, but the SPI module's genuine `RST` pin makes that whole hack
unnecessary: `u8g2.begin()` pulses hardware reset every boot, so the
controller always comes up in a known-good state.

## Pin mapping

| Signal | GPIO |
|--------|------|
| SCK  | 18 |
| MOSI | 17 |
| CS   | 21 |
| DC   | 42 |
| RST  | 41 |

(SCK/MOSI reuse the old I2C pins; GPIO 48 is avoided because it drives
the onboard LED, which would flicker with the clock.)

## Remaining quirk

A brief static/flicker can still appear on a quick power cycle. It's a
physical charge-pump transient, self-clears in well under a second, and
the display always recovers — it never hangs like the old I2C blank
screen. Moved the reset to the top of `setup()` which reduced it to a
quick flicker; the rest is considered acceptable.