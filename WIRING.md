# Wiring

Everything runs from the Pico's 3.3 V. All switches connect between a GPIO pin and GND
(the firmware turns on the internal pull-ups), so no resistors are needed.

| From | To (Pico pin) | Pico physical pin |
|---|---|---|
| Display **VIN** | 3V3(OUT) | 36 |
| Display **GND** | GND | 38 (any GND) |
| Display **CLK** | GP18 (SPI0 SCK) | 24 |
| Display **DI** | GP19 (SPI0 TX/MOSI) | 25 |
| Display **CS** | GP17 | 22 |
| Display EMD, DISP, EIN, 3V | not connected | — |
| Left KW11 **NO** | GP2 | 4 |
| Right KW11 **NO** | GP3 | 5 |
| Both KW11 **COM** | GND | 3 |
| KW11 NC | not connected | — |
| Button **II** (pause/start) | GP6 | 9 |
| Button **R** (reset/previous) | GP7 | 10 |
| Button **M** (mode/next) | GP8 | 11 |
| Other side of all 3 buttons | GND | 8 or 13 |
| Buzzer (PS1240) | GP15 → buzzer → GND | 20 |

"Left" and "right" are as seen from the front of the clock (the display side).

```
                 Raspberry Pi Pico (top view, USB at the back of the clock)
   GP2  ── left  KW11 NO          GP17 ── display CS
   GP3  ── right KW11 NO          GP18 ── display CLK
   GP6  ── button II              GP19 ── display DI
   GP7  ── button R               GP15 ── buzzer ── GND
   GP8  ── button M               3V3  ── display VIN
   GND  ── KW11 COM x2, button commons, display GND
```

## Tactile switches on the perfboard
12 × 12 tactile switches have 4 legs; the two legs on each side are connected internally.
Use one leg from each side. Run a shared GND wire along the strip, and a separate wire from each
switch to GP6 / GP7 / GP8. Mount the strip with the switch tops facing the front wall. The buttons
sit 20 mm apart, and the two M2 mounting holes are 66 mm apart on the same centre line.

## If something is swapped
- The wrong clock starts when you press the lever: set `SWAP_LEVER_SIDES = true` in the firmware
  (or swap the GP2/GP3 wires).
- The picture is upside down: set `DISPLAY_ROTATION = 2`.
