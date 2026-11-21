# Parts list

## Electronics

| Qty | Part | Notes | Link |
|---|---|---|---|
| 1 | Adafruit SHARP Memory Display Breakout, 2.7" 400×240 (product 4694) | The screen. Reflective, no backlight, like a tournament clock. | https://www.adafruit.com/product/4694 |
| 1 | Raspberry Pi Pico (RP2040) | Any Pico with the standard 51 × 21 mm footprint. No headers needed; wires are soldered straight on. | https://www.adafruit.com/product/4864 · https://sparkfun.com/products/17829 |
| 2 | KW11-3Z micro switch, short hinge lever, 3-pin | Lever sensors. Standard subminiature size: 19.8 × 6.4 × 10.2 mm body, mounting holes 9.5 mm apart. Sold in multi-packs on Amazon, AliExpress and eBay; search "KW11-3Z hinge lever". Roller-lever versions also fit (adjust with the slotted holes). | Search "KW11-3Z" |
| 3 | Tactile switch, 12 × 12 × 7.3 mm, through-hole | Front buttons. The 7.3 mm height matters, because the printed caps are sized for it (`tact_h` in the CAD file if yours differ). Example: Gikfun 12x12x7.3 mm (AE1027). | https://hackster.io/products/buy/65030 |
| 1 | Piezo buzzer PS1240 (12 mm) | Optional. Sits in the holder on the bottom plate. | https://www.adafruit.com/products/160 |
| 1 | Perfboard / protoboard, ≥ 76 × 18 mm | Carries the three tactile switches. Cut a strip. | Any |
| — | Thin hookup wire (26–28 AWG), heat-shrink | | Any |
| 1 | Micro-USB cable + 5 V USB power supply | Powers the clock (the Pico's USB port is at the back). | Any |

## Hardware

| Qty | Part | Used for |
|---|---|---|
| 1 | M3 × 16 socket-head screw + M3 hex nut | Lever pivot (nut sits in the hex pocket on the front bracket) |
| 4 | M3 × 10 countersunk screw | Bottom plate → corner posts (self-tapping into plastic) |
| 4 | M2.5 × 6 screw | Display breakout → bosses |
| 4 | M2 × 12 screw | KW11 switches → ribs (2 per switch) |
| 2 | M2 × 6 screw | Button perfboard → bosses |
| 4 | M2 × 5 screw | Pico → standoffs on the bottom plate |
| 4 | 6 × 2 mm neodymium disc magnets (N42–N52) | Lever end stops / detent: 2 in the lever, 2 in the roof. Example: https://totalelement.com/products/6mm-x-2mm-neodymium-rare-earth-permanent-disc-magnets-n52-150-pack |
| 4 | 10 mm round rubber feet (≤ 0.8 mm recess) | Bottom plate |
| — | Super glue (CA) | Magnets |

A mixed M2 / M2.5 / M3 screw assortment covers all of the screws.

## Printed parts (in `stl/`)

| File | Qty | Orientation | Notes |
|---|---|---|---|
| `chess_clock_body.stl` | 1 | Already placed **on its left side** | ~156 mm tall in that orientation. No supports. Brim recommended. |
| `chess_clock_lever.stl` | 1 | Already placed **upside down** (top face on bed) | No supports. |
| `chess_clock_plate.stl` | 1 | Flat | No supports. |
| `chess_clock_caps.stl` | 1 set (3 caps) | Front face down | II = pause/start, R = reset/previous, M = mode/next. |

Suggested settings: PLA or PETG, 0.2 mm layers, 3–4 walls, 20 % infill. PETG gives the lever pivot a little more toughness.
