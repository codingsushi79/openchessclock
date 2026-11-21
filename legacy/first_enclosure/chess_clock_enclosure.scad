// Arduino Chess Clock - parametric enclosure
// ------------------------------------------
// Two parts:
//   part = "shell" : front + top + bottom + sides, open at the back.
//                    Print FRONT FACE DOWN on the bed - no supports needed.
//   part = "back"  : back plate, print flat.
//   part = "both"  : assembled preview (not for printing).
//
// Coordinates (as used on the table):
//   X = width (left/right), Y = height (up), Z = depth (front face at Z=0, back at Z=depth)
//
// MEASURE YOUR PARTS and adjust the values in "Components" before printing.
// Print a test of just the front wall first if you're unsure about the display fit.

part = "both";          // "shell", "back", "both"

// ---------- Box ----------
width   = 190;
height  = 65;
depth   = 75;
wall    = 2.4;
corner_r = 4;           // outer vertical edge rounding

// ---------- Components ----------
// TM1637 0.56" 4-digit module (common 42 x 24 mm PCB)
disp_win_w     = 30.6;  // display body width + clearance
disp_win_h     = 14.6;  // display body height + clearance
disp_body_d    = 7.5;   // how far the display body stands off the PCB
disp_hole_dx   = 37.0;  // PCB mounting hole spacing (X)  -- measure yours
disp_hole_dy   = 19.0;  // PCB mounting hole spacing (Y)  -- measure yours
disp_hole_off_x = 0;    // offset of hole pattern from display centre, if not centred
disp_screw_d   = 1.8;   // pilot for M2 self-tapping screws
disp_boss_d    = 5;

disp_spacing   = 90;    // centre-to-centre distance between the two displays
disp_y         = 28;    // display centre height

led_d          = 5.2;   // 5 mm LED
led_above      = 15;    // LED centre above display centre

// Player buttons on top: 30 mm arcade button -> 30 mm hole (24 mm buttons -> 24)
btn_hole_d     = 30.2;
btn_spacing    = 90;
// Mode button: 12 mm panel-mount momentary (16 mm also common)
mode_hole_d    = 12.2;

// Back plate
usb_w = 13; usb_h = 9;          // USB cutout (Nano Mini-B ~ 8x4, USB-C ~ 9x3.5 plus plug shroud)
usb_x = 30; usb_y = 8;          // cutout centre: from the left edge (viewed from BEHIND) and from the bottom
vent_rows = 3; vent_cols = 6; vent_d = 3; vent_pitch = 6;   // buzzer vent

// Screw posts (M3 self-tapping)
post_size   = 8;
post_pilot  = 2.6;
plate_hole  = 3.4;
plate_t     = 2.4;

$fn = 64;

// ---------- Helpers ----------
module rounded_box(w, h, d, r) {
    // rounded on the 4 edges that run front-to-back
    hull() for (x = [r, w - r], y = [r, h - r]) translate([x, y, 0]) cylinder(r = r, h = d);
}

module post_positions() {
    for (x = [wall + post_size / 2, width - wall - post_size / 2],
         y = [wall + post_size / 2, height - wall - post_size / 2])
        translate([x, y, 0]) children();
}

disp_xs = [width / 2 - disp_spacing / 2, width / 2 + disp_spacing / 2];
btn_xs  = [width / 2 - btn_spacing / 2,  width / 2 + btn_spacing / 2];

// ---------- Shell ----------
module shell() {
    difference() {
        union() {
            difference() {
                rounded_box(width, height, depth, corner_r);
                translate([wall, wall, wall]) cube([width - 2 * wall, height - 2 * wall, depth]);
            }
            // corner screw posts, from front wall to back opening
            post_positions()
                translate([-post_size / 2, -post_size / 2, wall]) cube([post_size, post_size, depth - wall]);
            // display PCB bosses on the inside of the front wall
            for (x = disp_xs, sx = [-1, 1], sy = [-1, 1])
                translate([x + disp_hole_off_x + sx * disp_hole_dx / 2, disp_y + sy * disp_hole_dy / 2, wall])
                    cylinder(d = disp_boss_d, h = disp_body_d - wall);
        }
        // display windows
        for (x = disp_xs)
            translate([x - disp_win_w / 2, disp_y - disp_win_h / 2, -1]) cube([disp_win_w, disp_win_h, wall + 2]);
        // turn LEDs
        for (x = disp_xs)
            translate([x, disp_y + led_above, -1]) cylinder(d = led_d, h = wall + 2);
        // display screw pilots
        for (x = disp_xs, sx = [-1, 1], sy = [-1, 1])
            translate([x + disp_hole_off_x + sx * disp_hole_dx / 2, disp_y + sy * disp_hole_dy / 2, wall - 0.01])
                cylinder(d = disp_screw_d, h = disp_body_d);
        // player buttons in top wall (teardrop, point toward back)
        for (x = btn_xs) translate([x, height - wall - 1, depth / 2]) top_hole(btn_hole_d);
        // mode button
        translate([width / 2, height - wall - 1, depth / 2]) top_hole(mode_hole_d);
        // post pilot holes from the back
        post_positions() translate([0, 0, depth - 14]) cylinder(d = post_pilot, h = 15);
    }
}

// Hole through the top wall. Teardrop shape pointing to +Z (back) so the
// roof of the hole is self-supporting when printed front-face-down.
module top_hole(d) {
    rotate([-90, 0, 0])            // extrude along +Y (up through the top wall)
        linear_extrude(wall + 2)
            intersection() {
                hull() {
                    circle(d = d);
                    // after rotate([-90,0,0]), 2D -Y maps to +Z (toward back)
                    translate([0, -d / 2 * sqrt(2)]) square(0.01, center = true);
                }
                // clip the point so it stays hidden under the button bezel
                translate([-d, -d * 0.55]) square([2 * d, 1.55 * d]);
            }
}

// ---------- Back plate ----------
module back_plate() {
    difference() {
        rounded_box(width, height, plate_t, corner_r);
        // screw holes (countersunk)
        post_positions() {
            translate([0, 0, -1]) cylinder(d = plate_hole, h = plate_t + 2);
            translate([0, 0, plate_t - 1.6]) cylinder(d1 = plate_hole, d2 = 6.4, h = 1.61);
        }
        // USB cutout (measured from the left as seen from behind = from the right in model X)
        translate([width - usb_x - usb_w / 2, usb_y - usb_h / 2, -1])
            cube([usb_w, usb_h, plate_t + 2]);
        // buzzer vent grid
        for (r = [0 : vent_rows - 1], c = [0 : vent_cols - 1])
            translate([width / 2 - (vent_cols - 1) * vent_pitch / 2 + c * vent_pitch,
                       height / 2 - (vent_rows - 1) * vent_pitch / 2 + r * vent_pitch, -1])
                cylinder(d = vent_d, h = plate_t + 2);
    }
}

// ---------- Output ----------
if (part == "shell") {
    shell();                                  // front face already on Z=0: print as-is
} else if (part == "back") {
    // flip so the countersinks face up
    translate([0, height, plate_t]) rotate([180, 0, 0]) back_plate();
} else {
    color("SlateGray") shell();
    color("DimGray", 0.9) translate([0, 0, depth]) back_plate();
}
