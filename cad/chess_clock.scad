// =====================================================================
//  Chess clock - rocker lever, Sharp 2.7" memory LCD, Pico, KW11
// =====================================================================
//  Parts (set `part`):
//    "lever"    - the rocker.        Print UPSIDE DOWN (top face on bed).
//    "body"     - main shell.        Print ON ITS LEFT SIDE (left wall on bed), no supports.
//    "plate"    - bottom plate.      Print flat, feet recesses down.
//    "caps"     - 3 front buttons.   Print front face down.
//    "assembly" - everything incl. stand-in electronics (preview only).
//    "assembly_up" - same with the lever tilted the other way.
//
//  Coordinates: X = width (left -> right), Y = depth (front face at Y=0),
//               Z = up (inside floor / top of bottom plate at Z=0).
//
//  Hardware:
//    Pivot      1x M3x16 socket-head screw + M3 nut (nut sits in a hex trap)
//    Plate      4x M3x10 countersunk, self-tapping into corner posts
//    Display    4x M2.5x6 self-tapping
//    KW11       4x M2x12 (2 per switch), self-tapping into slotted ribs
//    Buttons    2x M2x6 (button board), 3x 12x12 mm tactile switches
//    Pico       4x M2x5
//    Magnets    4x 6x2 mm neodymium discs (lever ends + roof)
//    Buzzer     12 mm passive buzzer (optional) in the plate holder
// =====================================================================

part = "assembly";

/* ---------------- main box ---------------- */
W      = 156;        // width
D      = 64;         // depth
wall   = 2.5;
theta  = 5;          // lever tilt each way = roof slope
R      = 80.5;       // roof ridge height above floor
plate_t = 2.5;
cx = W / 2;
yc = D / 2;          // lever / pivot centre line (depth)

/* ---------------- lever ---------------- */
L        = 150;      // lever length
Lw       = 26;       // lever depth (front-back)
Lt       = 7;        // lever thickness
lever_r  = 5;        // plan-view corner radius
pad_t    = 1.2;      // end pads stand proud of the underside
pad_len  = 14;
pivot_drop = 8;      // axle sits this far below the roof ridge
za       = R - pivot_drop;            // axle height
d_b      = pivot_drop * cos(theta);   // axle -> pad underside
bar_z0   = d_b + pad_t;               // axle -> main underside
hub_x = 10; hub_y = 8; hub_r = 4.5;
axle_d   = 3.3;
fing_s   = 55;       // finger distance from centre
fing_x   = 4; fing_y = 5;
fing_below_inner = 12;               // finger tip depth below the inside of the roof (pressed)
tip_z    = d_b - wall - fing_below_inner;   // lever-local tip height

/* ---------------- magnets ---------------- */
mag_d = 6.2; mag_h = 2.1;

/* ---------------- KW11 micro switch (subminiature, 9.5 mm hole pitch) ---------------- */
kw_l = 19.8; kw_t = 6.4; kw_h = 10.2;
kw_pitch = 9.5;
kw_hole_from_bottom = 2.9;
kw_op = 8.8;           // operating position above hole centre line
kw_ot = 0.8;           // over-travel we ask for when the lever is down
kw_slot = 1.5;         // +/- vertical adjustment in the rib
rib_t = 4;

/* ---------------- display: Adafruit 4694, 2.7" Sharp memory LCD ---------------- */
// board 63.5 x 55.88, holes 2.5 mm on 58.42 x 50.8 (from Adafruit's Eagle files)
// Mounted header-up. Active area 58.8 x 35.28 inside a 62.8 x 42.82 glass.
dsp_w = 63.5; dsp_h = 55.88; dsp_pcb = 1.6;
dsp_hole_dx = 58.42; dsp_hole_z = [2.54, 53.34];
dsp_glass_z = [6.88, 49.70];       // glass bottom/top above board bottom edge
act_w = 58.8; act_h = 35.28;
act_bottom_border = 5.54;          // FPC side (bottom). ASSUMPTION: check yours, adjust if the window is off
win_margin = 0.4;
zb = 18;                           // display board bottom edge height
panel_gap = 0.3;                   // glass front to inside of front wall
panel_stack = 1.9;                 // glass + tape thickness above PCB
y_dsp = wall + panel_gap + panel_stack;   // PCB front face

/* ---------------- front buttons (12x12 tactile on perfboard) ---------------- */
btn_z = 11;
btn_pitch = 20;
btn_w = 12; btn_h = 7;             // hole in front wall
tact_h = 7.3;                      // tactile height incl. stem
cap_proud = 1.2; cap_flange_t = 1.5;
y_pb = wall + cap_flange_t + 0.2 + tact_h;   // perfboard front face
pb_hole_dx = 66;                   // button board screw spacing

/* ---------------- Pico ---------------- */
pico_l = 51; pico_w = 21;
pico_hole_l = 47; pico_hole_w = 11.4;
pico_x = cx + 52;                  // centre
pico_y1 = D - wall - 1.5;          // USB end of the board
pico_standoff = 4;

/* ---------------- corner posts ---------------- */
post_leg = 12;
post_in = post_leg * (2 - sqrt(2)) / 2;     // incentre offset
post_pilot = 2.6;
plate_hole = 3.4;

$fa = 2; $fs = 0.35;      // smooth curves (was a fixed 48 segments)
eps = 0.01;

// =====================================================================
//  helpers
// =====================================================================
function roof_z(x) = R - abs(x - cx) * tan(theta);
// rotate a lever-local point for tilt t (+1 = left end down)
function rot(p, t) = let(a = -t * theta)
    [p[0] * cos(a) + p[1] * sin(a), -p[0] * sin(a) + p[1] * cos(a)];

module rr2(w, h, r) { offset(r = r) offset(delta = -r) square([w, h], center = true); }
module ext_y(y0, h) { translate([0, y0 + h, 0]) rotate([90, 0, 0]) linear_extrude(h) children(); }
module cyl_y(d, y0, h) { translate([0, y0, 0]) rotate([-90, 0, 0]) cylinder(d = d, h = h); }

// ---- derived positions -------------------------------------------------
tipL   = rot([-fing_s, tip_z], 1);            // left finger tip, left end down
sw_x   = [cx + tipL[0], cx - tipL[0]];        // switch centres
sw_hole_z = za + tipL[1] - (kw_op - kw_ot);   // hole centre line
padL   = rot([-(L / 2 - pad_len / 2), d_b], 1);
mag_x  = [cx + padL[0], cx - padL[0]];
fingL_roof = rot([-fing_s, d_b], 1)[0];       // where the finger crosses the roof
fing_slot_x = [cx + fingL_roof, cx - fingL_roof];

echo(str("Switch centres x = ", sw_x, "  hole line z = ", sw_hole_z));
echo(str("Lever top at rest = ", za + bar_z0 + Lt + plate_t, " mm above table"));

// =====================================================================
//  LEVER  (lever-local: origin on the axle, X along the lever, Z up)
// =====================================================================
module lever_bar() {
    hull() {
        linear_extrude(Lt - 1) rr2(L, Lw, lever_r);
        linear_extrude(Lt) rr2(L - 2, Lw - 2, lever_r - 1);
    }
}

module lever() {
    difference() {
        union() {
            translate([0, 0, bar_z0]) lever_bar();
            // end pads (stops + magnet carriers)
            for (s = [-1, 1])
                translate([0, 0, d_b]) linear_extrude(pad_t + eps)
                    intersection() {
                        rr2(L, Lw, lever_r);
                        translate([s * (L / 2 - pad_len / 2), 0]) square([pad_len, Lw], center = true);
                    }
            // pivot hub
            hull() {
                rotate([90, 0, 0]) cylinder(r = hub_r, h = hub_y, center = true);
                translate([-hub_x / 2, -hub_y / 2, 0]) cube([hub_x, hub_y, bar_z0 + 1]);
            }
            // switch fingers
            for (s = [-1, 1]) translate([s * fing_s, 0, 0]) hull() {
                translate([0, 0, tip_z + fing_x / 2]) rotate([90, 0, 0]) cylinder(d = fing_x, h = fing_y, center = true);
                translate([-fing_x / 2, -fing_y / 2, bar_z0]) cube([fing_x, fing_y, 1]);
            }
        }
        rotate([90, 0, 0]) cylinder(d = axle_d, h = hub_y + 2, center = true);
        for (s = [-1, 1]) translate([s * (L / 2 - pad_len / 2), 0, d_b - eps]) cylinder(d = mag_d, h = mag_h + eps);
    }
}

module lever_posed(t) { translate([cx, yc, za]) rotate([0, -t * theta, 0]) lever(); }

// =====================================================================
//  BODY
// =====================================================================
// ---- smooth shell ---------------------------------------------------
// side profile (front view): roof corners rounded, bottom corners square so it sits on the plate
corner_r = 7;        // roof-to-side rounding
edge_r   = 6;        // fillet on every front/back edge (outside); inside follows concentrically
seam_ch  = 0.8;      // small chamfer at the body/plate seam
face_t   = 1.0;      // raised faceplate around display + buttons
fil_steps = 28;

// DGT-style framing: front and back rails rise above the roof, the lever runs in a channel between them
rail_h = 7.5;                 // rail height above the roof (lever top sits ~0.7 mm proud when pressed)
ch_clear = 0.8;               // lever-to-rail clearance each side
ch_w = Lw + 2 * ch_clear;     // channel width
ch_c = 1.5;                   // chamfer on the rail inner edges
ch_steps = 12;

module profile(rr = R) {
    he = rr - cx * tan(theta);
    union() {
        offset(r = corner_r) offset(delta = -corner_r) polygon([[0, 0], [W, 0], [W, he], [cx, rr], [0, he]]);
        square([W, corner_r + 1]);
    }
}
module slice_out(inset) { hull() { offset(delta = -inset) profile(R + rail_h); translate([inset, 0]) square([W - 2 * inset, 1]); } }
module slice_in(inset)  { hull() { offset(delta = -inset) profile(); translate([inset, -5]) square([W - 2 * inset, 1]); } }

// hull of thin slices tracing a quarter-circle fillet on the front and back edges
module outer_raw() {
    hull() for (i = [0 : fil_steps]) {
        a = 90 * i / fil_steps;
        ins = edge_r - edge_r * cos(a);
        dy  = edge_r - edge_r * sin(a);
        ext_y(dy, eps) slice_out(ins);
        ext_y(D - dy - eps, eps) slice_out(ins);
    }
}
footprint_r = edge_r;
module footprint() { offset(r = footprint_r) offset(delta = -footprint_r) square([W, D]); }
module outer() {
    intersection() {
        outer_raw();
        hull() {
            translate([0, 0, -eps]) linear_extrude(eps) offset(delta = -seam_ch) footprint();
            translate([0, 0, seam_ch]) linear_extrude(R + rail_h + 10) footprint();
        }
    }
}
module cavity() {
    ri = edge_r - wall;
    hull() for (i = [0 : fil_steps]) {
        a = 90 * i / fil_steps;
        ins = edge_r - ri * cos(a);
        dy  = edge_r - ri * sin(a);
        ext_y(dy, eps) slice_in(ins);
        ext_y(D - dy - eps, eps) slice_in(ins);
    }
}

// the lever channel: floor = original roof, walls = rails, chamfered top edges
module channel_cut() {
    for (k = [0 : ch_steps]) {
        d = ch_c * k / ch_steps;
        ext_y(yc - ch_w / 2 - d, ch_w + 2 * d) difference() {
            translate([-10, -10]) square([W + 20, 300]);
            rr = k == 0 ? R : R + rail_h - (ch_c - d);
            profile(rr);
            translate([-20, -20]) square([W + 40, 20 + rr - cx * tan(theta) - corner_r]);   // keep the side walls clear of the cut
        }
    }
}

// raised faceplate framing the display and the buttons
face_z = [btn_z - 7, zb + dsp_glass_z[0] + act_bottom_border + act_h + 5];
face_w = act_w + 16;
module faceplate() {
    fh = face_z[1] - face_z[0];
    translate([cx, 0, (face_z[0] + face_z[1]) / 2]) hull() {
        translate([0, -face_t, 0]) rotate([90, 0, 0]) linear_extrude(eps) rr2(face_w - 1.6, fh - 1.6, 4.2);
        translate([0, 0.5, 0]) rotate([90, 0, 0]) linear_extrude(eps) rr2(face_w, fh, 5);
    }
}

module post_tri2(corner) {   // corner = [right?, back?]
    x0 = corner[0] ? W - wall : wall;
    y0 = corner[1] ? D - wall : wall;
    ox = corner[0] ? 1 : -1;  oy = corner[1] ? 1 : -1;      // outward (into the walls)
    dx = -ox * post_leg;       dy = -oy * post_leg;
    polygon([[x0 + ox, y0 + oy], [x0 + dx, y0 + oy], [x0 + dx, y0], [x0, y0 + dy], [x0 + ox, y0 + dy]]);
}
module post_tri(corner) { linear_extrude(R) post_tri2(corner); }
function post_c(corner) = [corner[0] ? W - wall - post_in : wall + post_in,
                           corner[1] ? D - wall - post_in : wall + post_in];
corners = [[0, 0], [1, 0], [0, 1], [1, 1]];

bt = 3;  // bracket thickness
br_y = [yc - hub_y / 2 - 0.4 - bt, yc + hub_y / 2 + 0.4];   // bracket start y (front, back)

dsp_x0 = cx - dsp_w / 2;
dsp_holes = [for (sx = [-1, 1], z = dsp_hole_z) [cx + sx * dsp_hole_dx / 2, zb + z]];
act_z0 = zb + dsp_glass_z[0] + act_bottom_border;

module body_features() {
    for (c = corners) post_tri(c);
    // pivot brackets
    for (y0 = br_y) hull() {
        translate([cx, y0, za]) rotate([-90, 0, 0]) cylinder(r = 5.5, h = bt);
        translate([cx - 9, y0, za + 2]) cube([18, bt, R]);
    }
    // KW11 ribs (switch screws in from the back, through the switch into the rib)
    for (x = sw_x)
        translate([x - (kw_l + 4) / 2, yc - kw_t / 2 - rib_t, sw_hole_z - kw_hole_from_bottom - 3])
            cube([kw_l + 4, rib_t, R]);
    // display bosses
    for (h = dsp_holes) translate([h[0], 0, h[1]]) cyl_y(5.5, wall - eps, y_dsp - wall + eps);
    // button board bosses
    for (sx = [-1, 1]) translate([cx + sx * pb_hole_dx / 2, 0, btn_z]) cyl_y(6, wall - eps, y_pb - wall + eps);
    // magnet bosses under the roof
    for (i = [0, 1]) translate([mag_x[i], yc, roof_z(mag_x[i])])
        rotate([0, i == 0 ? -theta : theta, 0]) translate([0, 0, -wall - 2.5]) cylinder(d = 10, h = 5);
}

module body_cuts() {
    channel_cut();
    // display window with a 45-degree bevel on the front
    translate([cx, 0, act_z0 + act_h / 2]) {
        ww = act_w + 2 * win_margin; wh = act_h + 2 * win_margin;
        hull() {
            translate([0, wall - 1.2, 0]) rotate([90, 0, 0]) linear_extrude(eps) rr2(ww, wh, 1);
            translate([0, -face_t - eps, 0]) rotate([90, 0, 0]) linear_extrude(eps) rr2(ww + 2.4 + 2 * face_t, wh + 2.4 + 2 * face_t, 1 + 1.2 + face_t);
        }
        translate([0, wall + 1, 0]) rotate([90, 0, 0]) linear_extrude(wall + face_t + 3) rr2(ww, wh, 1);
    }
    // button holes
    for (i = [-1, 0, 1]) translate([cx + i * btn_pitch, wall + 1, btn_z]) rotate([90, 0, 0])
        linear_extrude(wall + face_t + 3) rr2(btn_w, btn_h, 2);
    // hub slot (between the brackets)
    translate([cx - 6.5, yc - hub_y / 2 - 0.4, za]) cube([13, hub_y + 0.8, 20]);
    // finger slots
    for (x = fing_slot_x) translate([x - 4, yc - fing_y / 2 - 0.6, za - 15]) cube([8, fing_y + 1.2, 30]);
    // roof magnet pockets
    for (i = [0, 1]) translate([mag_x[i], yc, roof_z(mag_x[i])])
        rotate([0, i == 0 ? -theta : theta, 0]) translate([0, 0, -mag_h]) cylinder(d = mag_d, h = mag_h + 2);
    // pivot: clearance through both brackets, nut trap on the front bracket's front face
    translate([cx, br_y[0] - 1, za]) rotate([-90, 0, 0]) cylinder(d = 3.4, h = 2 * bt + hub_y + 3);
    translate([cx, br_y[0] - eps, za]) rotate([-90, 0, 0]) rotate([0, 0, 30]) cylinder(d = 6.4, h = 2.5, $fn = 6);
    // KW11 slotted pilot holes
    for (x = sw_x, dx = [-1, 1]) hull() for (dz = [-kw_slot, kw_slot])
        translate([x + dx * kw_pitch / 2, yc - kw_t / 2 - rib_t - 1, sw_hole_z + dz]) rotate([-90, 0, 0]) cylinder(d = 1.8, h = rib_t + 2);
    // display pilots (run 1.8 mm into the front wall)
    for (h = dsp_holes) translate([h[0], 0, h[1]]) cyl_y(2.1, wall - 1.8, 10);
    // button board pilots
    for (sx = [-1, 1]) translate([cx + sx * pb_hole_dx / 2, 0, btn_z]) cyl_y(1.8, wall + 0.5, 20);
    // corner post pilots
    for (c = corners) translate([post_c(c)[0], post_c(c)[1], -eps]) cylinder(d = post_pilot, h = 14);
    // screwdriver access through the back wall: KW11 screws (slotted like the ribs) and the pivot bolt
    for (x = sw_x, dx = [-1, 1]) hull() for (dz = [-kw_slot, kw_slot])
        translate([x + dx * kw_pitch / 2, D - wall - 1, sw_hole_z + dz]) rotate([-90, 0, 0]) cylinder(d = 4.5, h = wall + 2);
    translate([cx, D - wall - 1, za]) rotate([-90, 0, 0]) cylinder(d = 6, h = wall + 2);
    // USB opening in the back wall
    translate([pico_x - 6, D - wall - 1, pico_standoff + 1 + 1.4 - 4.5]) cube([12, wall + 2, 9]);
}

module body() {
    difference() {
        union() {
            difference() { outer(); cavity(); }
            intersection() { outer(); body_features(); }
            faceplate();
        }
        body_cuts();
    }
}

// =====================================================================
//  BOTTOM PLATE  (sits under the body, Z from -plate_t to 0)
// =====================================================================
pico_holes = [for (sx = [-1, 1], sy = [-1, 1])
    [pico_x + sx * pico_hole_w / 2, pico_y1 - pico_l / 2 + sy * pico_hole_l / 2]];
buzz = [28, yc];

module plate() {
    difference() {
        union() {
            hull() {   // 0.6 mm shadow gap to the body, chamfered bottom edge
                translate([0, 0, -plate_t]) linear_extrude(eps) offset(delta = -0.6 - seam_ch) footprint();
                translate([0, 0, -plate_t + seam_ch]) linear_extrude(plate_t - seam_ch) offset(delta = -0.6) footprint();
            }
            // locating lip just inside the walls
            linear_extrude(1.5) difference() {
                offset(delta = -0.3) translate([wall, wall]) square([W - 2 * wall, D - 2 * wall]);
                offset(delta = -1.9) translate([wall, wall]) square([W - 2 * wall, D - 2 * wall]);
                for (c = corners) offset(r = 0.4) post_tri2(c);
            }
            // Pico standoffs
            for (h = pico_holes) translate([h[0], h[1], 0]) cylinder(d = 5, h = pico_standoff);
            // buzzer holder
            translate([buzz[0], buzz[1], 0]) difference() { cylinder(d = 15.4, h = 4); cylinder(d = 12.4, h = 5); }
        }
        for (c = corners) translate([post_c(c)[0], post_c(c)[1], -plate_t - eps]) {
            cylinder(d = plate_hole, h = plate_t + 1);
            cylinder(d1 = 6.4, d2 = plate_hole, h = 1.6);
        }
        for (h = pico_holes) translate([h[0], h[1], -plate_t + 0.6]) cylinder(d = 1.7, h = 10);
        for (a = [0 : 90 : 270]) translate([buzz[0], buzz[1], -plate_t - 1]) rotate(a + 45) translate([3, 0, 0]) cylinder(d = 2.5, h = 5);
        for (sx = [0, 1], sy = [0, 1]) translate([sx ? W - 22 : 22, sy ? D - 12 : 12, -plate_t - eps]) cylinder(d = 10.5, h = 0.8);
    }
}

// =====================================================================
//  BUTTON CAPS  (built at their installed position, front face at y = -(face_t + cap_proud))
// =====================================================================
cap_labels = ["II", "R", "M"];   // pause, reset, cycle
module cap(i) {
    difference() {
        union() {
            translate([0, wall, 0]) rotate([90, 0, 0]) linear_extrude(wall + face_t + cap_proud) rr2(btn_w - 0.6, btn_h - 0.6, 1.8);
            translate([0, wall + cap_flange_t, 0]) rotate([90, 0, 0]) linear_extrude(cap_flange_t) rr2(btn_w + 3, btn_h + 3, 2);
        }
        translate([0, -face_t - cap_proud + 0.6, 0]) rotate([90, 0, 0])
            linear_extrude(0.6 + eps) text(cap_labels[i], size = 3.6, halign = "center", valign = "center",
                                     font = "Liberation Sans:style=Bold");
    }
}
module caps_installed() { for (i = [0 : 2]) translate([cx + (i - 1) * btn_pitch, 0, btn_z]) cap(i); }

// =====================================================================
//  STAND-IN ELECTRONICS (for checking fit only)
// =====================================================================
module kw11(x) {
    z0 = sw_hole_z - kw_hole_from_bottom;
    translate([x - kw_l / 2, yc - kw_t / 2, z0]) cube([kw_l, kw_t, kw_h]);
    translate([x - kw_l / 2, yc - kw_t / 2, z0 - 4]) cube([kw_l, kw_t, 4]);      // terminals + wires
}
module display_model() {
    translate([dsp_x0, y_dsp, zb]) cube([dsp_w, dsp_pcb, dsp_h]);
    translate([cx - 31.4, wall + panel_gap, zb + dsp_glass_z[0]]) cube([62.8, panel_stack, dsp_glass_z[1] - dsp_glass_z[0]]);
    translate([cx - 11.5, y_dsp + dsp_pcb, zb + 53.34 - 1.3]) cube([23, 10, 2.6]);     // header + wires
    translate([dsp_x0 + 4, y_dsp + dsp_pcb, zb + 8]) cube([dsp_w - 8, 3, 40]);           // parts on the back
}
module buttons_model() {
    translate([cx - 38, y_pb, btn_z - 9]) cube([76, 1.6, 18]);
    for (i = [-1, 0, 1]) translate([cx + i * btn_pitch - 6, y_pb - tact_h, btn_z - 6]) cube([12, tact_h, 12]);
}
module pico_model() {
    translate([pico_x - pico_w / 2, pico_y1 - pico_l, pico_standoff]) cube([pico_w, pico_l, 1]);
    translate([pico_x - 4, pico_y1 - 5, pico_standoff + 1]) cube([8, 6.3, 2.8]);      // USB socket
}

module electronics() {
    color("SeaGreen") { kw11(sw_x[0]); kw11(sw_x[1]); }
    color("DarkSlateGray") display_model();
    color("Olive") buttons_model();
    color("ForestGreen") pico_model();
}

// =====================================================================
//  OUTPUT
// =====================================================================
if (part == "lever") {
    translate([0, 0, bar_z0 + Lt]) rotate([180, 0, 0]) lever();      // upside down
} else if (part == "body") {
    rotate([0, -90, 0]) body();                                       // left wall on the bed
} else if (part == "plate") {
    translate([0, 0, plate_t]) plate();
} else if (part == "caps") {
    for (i = [0 : 2]) translate([i * 20, 0, face_t + cap_proud]) rotate([90, 0, 0]) cap(i);
} else if (part == "section" || part == "section_up") {
    t = part == "section" ? 1 : -1;
    intersection() {
        translate([-10, yc, -10]) cube([W + 20, D, 120]);     // cut away the front half
        union() {
            color("Gainsboro") body();
            color("DimGray") plate();
            color("SteelBlue") lever_posed(t);
            electronics();
        }
    }
} else if (part == "assembly" || part == "assembly_up") {
    t = part == "assembly" ? 1 : -1;
    color("Gainsboro", 0.55) body();
    color("DimGray") plate();
    color("SteelBlue") lever_posed(t);
    color("Orange") caps_installed();
    electronics();
}
