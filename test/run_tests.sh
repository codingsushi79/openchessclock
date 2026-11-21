#!/usr/bin/env bash
# Runs the clock firmware on your computer (no Pico needed): scripted games check the
# timing math, and every screen is saved as an image in test/shots/.
# Needs: g++, git, and Python 3 with Pillow (only for the PNG contact sheet).
set -euo pipefail
cd "$(dirname "$0")"

if [ ! -d gfx ]; then
  git clone -q --depth 1 https://github.com/adafruit/Adafruit-GFX-Library gfx
fi
mkdir -p shots
cp ../firmware/chess_clock_pico/chess_clock_pico.ino sketch.inc
g++ -std=gnu++17 -O1 -w -DARDUINO=100 -Istubs -Igfx/src -o clock_sim harness.cpp gfx/src/Adafruit_GFX.cpp
./clock_sim

python3 - <<'EOF' || echo "(install Pillow to get shots/contact_sheet.png)"
from PIL import Image
import glob
fs = sorted(glob.glob('shots/*.pgm'))
ims = [Image.open(f).convert('L') for f in fs]
W, H, cols = 400, 240, 2
rows = (len(ims) + 1) // 2
sheet = Image.new('L', (cols * (W + 12) + 12, rows * (H + 12) + 12), 255)
for i, im in enumerate(ims):
    sheet.paste(im, (12 + (i % cols) * (W + 12), 12 + (i // cols) * (H + 12)))
sheet.save('shots/contact_sheet.png')
print('wrote shots/contact_sheet.png')
EOF
