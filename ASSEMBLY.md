# Build guide

## 0. Before you print everything
1. **Test the lever feel first.** Print the lever, and in your slicer cut the body
   so you print only the top ~25 mm (roof, rails, pivot brackets and switch ribs). Assemble the
   pivot, magnets and switches on that piece and check the click.
2. **Check the display window.** The board, its holes and the glass size come from Adafruit's PCB
   files. How much of the glass border is on the bottom (ribbon-cable) side is an
   assumption: 5.5 mm (`act_bottom_border` in `cad/chess_clock.scad`). Hold the display behind the
   printed window; if the pixels sit high or low, change that value and re-export the body.

## 1. Print
See the table at the end of `PARTS.md`. The STLs are already oriented for printing. Re-export from
`cad/chess_clock.scad` with OpenSCAD if you change anything: set `part = "body"` (or `"lever"`,
`"plate"`, `"caps"`) and press F6 then F7. Use `part = "assembly"` / `"section"` to preview.

## 2. Magnets (do this first, glue needs time)
1. Stack all four magnets in one column so you know they attract in a line.
2. Take them off one at a time. Mark the top face of each with a marker before you separate it.
3. Glue two into the lever's end pockets and two into the roof pockets so that each lever magnet
   **attracts** the roof magnet under it. Test with the lever before the glue sets.
4. If the snap is too strong, sand 0.2–0.3 mm off the lever pads to add a gap. If it's too weak,
   use N52 magnets.

## 3. Lever
1. Drop the lever into the channel so the pivot block goes through the centre slot and the two
   fingers go through the side slots.
2. Put the M3 nut into the hex pocket on the **front** bracket (inside the body, reach in from the
   open bottom).
3. Push the M3 × 16 screw in from the **back** (use the round access hole in the back wall),
   through both brackets and the lever, into the nut. Tighten until the lever has no side play
   but still rocks freely, then back off a quarter turn. A drop of thread-locker helps.

## 4. Lever switches (KW11)
1. Solder wires to COM and NO on each switch first (heat-shrink them).
2. Hold each switch against the **back** face of its rib, lever arm up, body below the finger.
3. Screw it on with 2 × M2 × 12 from behind, through the slotted access holes in the back wall.
   The rib holes are slots, so you can slide the switch ±1.5 mm up or down before tightening.
4. Adjust: with the lever end **down**, the switch should have clicked. With it **up**, it
   should be released. Set each switch about 0.5–1 mm past the click point when pressed.

## 5. Display
1. Solder wires to VIN, GND, CLK, DI, CS (see `WIRING.md`). Solder wires directly rather than
   using a header, to keep the space behind the board clear.
2. Peel the protective film and place the board glass-side forward into the window, header edge
   at the **top**.
3. Fix it with 4 × M2.5 × 6 into the bosses. Snug only; don't stress the glass.

## 6. Buttons
1. Solder the three 12 × 12 tactile switches to the perfboard strip, 20 mm apart, and drill two
   2.2 mm holes 66 mm apart on the same line.
2. Put the three printed caps into the front holes from the inside (flange inside).
3. Screw the strip to the two bosses below the display with 2 × M2 × 6. Each cap should rest
   lightly on its switch and click when pressed.

## 7. Pico, buzzer, bottom plate
1. Screw the Pico onto the plate standoffs (4 × M2 × 5) with its USB port toward the back edge.
2. Press the PS1240 buzzer into the ring holder.
3. Solder all wires to the Pico (`WIRING.md`). Leave enough slack to open the plate.
4. Flash the firmware (see `README.md`) and test everything before closing.
5. Fit the plate (the lip locates it) and fix it with 4 × M3 × 10 countersunk screws. Stick on
   the rubber feet.

## 8. First power-up check
- The setup screen shows "PRESET 11 / 19, 10 min sudden death".
- M / R step through the presets. Holding M toggles sound.
- Press the lever: the side whose end is **up** starts running, and the bar above its time is
  filled.
- If the wrong side runs, set `SWAP_LEVER_SIDES = true`. If the image is upside down, set
  `DISPLAY_ROTATION = 2`.
