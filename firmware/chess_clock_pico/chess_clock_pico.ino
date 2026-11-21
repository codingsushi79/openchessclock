/*
  Chess Clock - Raspberry Pi Pico + Sharp 2.7" Memory LCD + rocker lever
  ======================================================================
  Board:     Raspberry Pi Pico (RP2040)
  Core:      "Raspberry Pi Pico/RP2040" by Earle F. Philhower (arduino-pico)
  Libraries: Adafruit SHARP Memory Display, Adafruit GFX, Adafruit BusIO
             (all from the Arduino Library Manager)

  Wiring (see WIRING.md for the full table):
    Sharp display  CLK -> GP18   DI -> GP19   CS -> GP17   VIN -> 3V3   GND -> GND
    Lever switches KW11 COM -> GND, NO -> GP2 (left end) / GP3 (right end)
    Buttons        II (pause/start) -> GP6, R (reset/prev) -> GP7, M (mode/next) -> GP8
                   other leg of each button -> GND
    Buzzer         PS1240 piezo: GP15 -> buzzer -> GND

  How it plays (like a DGT 3000):
    - The lever end that is UP belongs to the player whose clock is running.
      Press your end down to stop your clock and start your opponent's.
    - SETUP   : M = next preset, R = previous preset, hold M = sound on/off.
                Start by pressing the lever (Black presses -> White's clock runs)
                or press II to start the side whose lever end is up.
    - RUNNING : II pauses.
    - PAUSED  : II resumes. Hold R (1 s) to end the game and go back to setup.
    - FLAGGED : II or R goes back to setup.

  Timing modes: Fischer increment, simple (US) delay, Bronstein delay, sudden death.

  Core 0 runs the clock logic and reads the lever/buttons; core 1 only draws the
  screen (a full Sharp refresh takes ~50 ms, so it never delays a lever press).
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "clock_types.h"

// ---------------- Pins ----------------
const uint8_t PIN_SHARP_SCK  = 18;
const uint8_t PIN_SHARP_MOSI = 19;
const uint8_t PIN_SHARP_CS   = 17;
const uint8_t PIN_LEVER_L    = 2;    // closes when the LEFT end is pressed down
const uint8_t PIN_LEVER_R    = 3;    // closes when the RIGHT end is pressed down
const uint8_t PIN_BTN_PAUSE  = 6;    // "II"
const uint8_t PIN_BTN_RESET  = 7;    // "R"
const uint8_t PIN_BTN_MODE   = 8;    // "M"
const uint8_t PIN_BUZZER     = 15;

// ---------------- Settings ----------------
const uint8_t  DISPLAY_ROTATION   = 0;      // set to 2 if the picture is upside down
const uint32_t SHARP_SPI_HZ       = 2000000;
const bool     SWAP_LEVER_SIDES   = false;  // set true if pressing left starts the wrong clock
const unsigned long LEVER_DEBOUNCE_MS = 6;
const unsigned long BTN_DEBOUNCE_MS   = 25;
const unsigned long LONG_PRESS_MS     = 1000;
const long TENTHS_BELOW_MS    = 20000;
const long LOW_TIME_WARN_MS   = 10000;      // one warning beep (0 = off)
const uint8_t LOW_TIME_TICK_SECS = 5;       // tick each second below this (0 = off)

#define BLACK 0
#define WHITE 1

Adafruit_SharpMem display(&SPI, PIN_SHARP_CS, 400, 240, SHARP_SPI_HZ);

// ---------------- Presets ----------------
const Preset PRESETS[] = {
  {1,  0,  FISCHER},  {1,  1,  FISCHER},  {2,  1,  FISCHER},
  {3,  0,  FISCHER},  {3,  2,  FISCHER},  {3,  2,  DELAY},
  {5,  0,  FISCHER},  {5,  3,  FISCHER},  {5,  5,  DELAY},   {5, 3, BRONSTEIN},
  {10, 0,  FISCHER},  {10, 5,  FISCHER},  {15, 10, FISCHER},
  {25, 5,  DELAY},    {30, 0,  FISCHER},  {45, 15, FISCHER},
  {60, 30, FISCHER},  {90, 30, FISCHER},  {90, 30, DELAY},
};
const uint8_t NUM_PRESETS = sizeof(PRESETS) / sizeof(PRESETS[0]);
volatile uint8_t presetIdx = 10;            // 10+0

// ---------------- Game state (written by core 0, read by core 1) ----------------
volatile State state = SETUP;

const uint8_t SIDE_L = 0, SIDE_R = 1, SIDE_NONE = 2;
volatile long remaining[2];
volatile uint16_t moves[2];
volatile uint8_t active = SIDE_NONE;
volatile unsigned long turnUsed = 0;        // ms used this turn (pauses excluded)
volatile bool soundOn = true;
volatile unsigned long hintUntil = 0;       // show "hold R to reset" hint until this time

bool warned[2];
uint8_t lastTickSec[2];
unsigned long lastTick = 0;

const Preset &P() { return PRESETS[presetIdx]; }
long bonusMs() { return (long)P().bonusSec * 1000L; }

// ---------------- Sound ----------------
void beep(unsigned int hz, unsigned long ms) { if (soundOn) tone(PIN_BUZZER, hz, ms); }

// ---------------- Lever ----------------
LeverPos leverStable = LEVER_MID;      // last debounced, non-mid position
LeverPos leverCandidate = LEVER_MID;
unsigned long leverCandidateSince = 0;

LeverPos readLeverRaw() {
  bool l = digitalRead(PIN_LEVER_L) == LOW;
  bool r = digitalRead(PIN_LEVER_R) == LOW;
  if (SWAP_LEVER_SIDES) { bool t = l; l = r; r = t; }
  if (l && !r) return LEVER_LEFT_DOWN;
  if (r && !l) return LEVER_RIGHT_DOWN;
  return LEVER_MID;
}

// The player whose end is UP is the one whose clock should run.
uint8_t upSide(LeverPos p) {
  if (p == LEVER_LEFT_DOWN)  return SIDE_R;
  if (p == LEVER_RIGHT_DOWN) return SIDE_L;
  return SIDE_NONE;
}

// Returns true when the lever settles in a new end position. `when` = time of the
// first edge, so debouncing never costs the player time.
bool updateLever(unsigned long &when) {
  unsigned long now = millis();
  LeverPos raw = readLeverRaw();
  if (raw != leverCandidate) { leverCandidate = raw; leverCandidateSince = now; }
  if (leverCandidate != LEVER_MID && leverCandidate != leverStable &&
      now - leverCandidateSince >= LEVER_DEBOUNCE_MS) {
    leverStable = leverCandidate;
    when = leverCandidateSince;
    return true;
  }
  return false;
}

// A lever change is being confirmed: don't charge anyone past its first edge.
bool leverPending() { return leverCandidate != LEVER_MID && leverCandidate != leverStable; }

// ---------------- Buttons ----------------
Button btnPause = {PIN_BTN_PAUSE, false, false, 0};
Button btnReset = {PIN_BTN_RESET, false, false, 0};
Button btnMode  = {PIN_BTN_MODE,  false, false, 0};

BtnEvent updateButton(Button &b) {
  unsigned long now = millis();
  bool raw = digitalRead(b.pin) == LOW;
  if (raw != b.down && now - b.changedAt >= BTN_DEBOUNCE_MS) {
    b.down = raw;
    b.changedAt = now;
    if (raw) { b.longFired = false; return EV_PRESS; }
    return b.longFired ? EV_NONE : EV_SHORT;
  }
  if (b.down && !b.longFired && now - b.changedAt >= LONG_PRESS_MS) {
    b.longFired = true;
    return EV_LONG;
  }
  return EV_NONE;
}

// ---------------- Clock logic ----------------
void loadPreset() {
  long base = (long)P().minutes * 60000L;
  for (uint8_t s = SIDE_L; s <= SIDE_R; s++) {
    remaining[s] = base;
    moves[s] = 0;
    warned[s] = false;
    lastTickSec[s] = 255;
  }
}

void enterSetup() {
  state = SETUP;
  active = SIDE_NONE;
  noTone(PIN_BUZZER);
  loadPreset();
}

// Charge the running clock up to time `now`.
void advanceTo(unsigned long now) {
  if (state != RUNNING) { lastTick = now; return; }
  if ((long)(now - lastTick) <= 0) return;
  unsigned long dt = now - lastTick;
  long charge = (long)dt;
  if (P().mode == DELAY) {           // only time beyond the delay is charged
    long d = bonusMs();
    long before = (long)turnUsed - d;        if (before < 0) before = 0;
    long after  = (long)(turnUsed + dt) - d; if (after < 0) after = 0;
    charge = after - before;
  }
  turnUsed += dt;
  remaining[active] -= charge;
  lastTick = now;

  if (remaining[active] <= 0) {
    remaining[active] = 0;
    state = FLAGGED;
    beep(880, 1500);
  }
}

void lowTimeAlerts() {
  if (state != RUNNING) return;
  long r = remaining[active];
  if (LOW_TIME_WARN_MS > 0 && !warned[active] && r <= LOW_TIME_WARN_MS) {
    warned[active] = true;
    beep(1800, 250);
    return;
  }
  if (LOW_TIME_TICK_SECS > 0 && r <= (long)LOW_TIME_TICK_SECS * 1000L) {
    uint8_t s = (uint8_t)(((unsigned long)r + 999UL) / 1000UL);
    if (s != lastTickSec[active]) { lastTickSec[active] = s; beep(2500, 30); }
  }
}

void startClock(uint8_t side, unsigned long when) {
  if (side == SIDE_NONE) { beep(400, 120); return; }   // lever in the middle
  active = side;
  turnUsed = 0;
  lastTick = when;
  state = RUNNING;
}

// The running player pressed their end at time `when`.
void endTurn(unsigned long when) {
  advanceTo(when);
  if (state != RUNNING) return;      // flagged before the press landed
  uint8_t s = active;
  if (P().mode == FISCHER)        remaining[s] += bonusMs();
  else if (P().mode == BRONSTEIN) remaining[s] += ((long)turnUsed < bonusMs()) ? (long)turnUsed : bonusMs();
  moves[s]++;
  if (remaining[s] > LOW_TIME_WARN_MS) warned[s] = false;
  lastTickSec[s] = 255;
  active = (s == SIDE_L) ? SIDE_R : SIDE_L;
  turnUsed = 0;
}

// ---------------- Core 0: logic ----------------
void setup() {
  pinMode(PIN_LEVER_L, INPUT_PULLUP);
  pinMode(PIN_LEVER_R, INPUT_PULLUP);
  pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  enterSetup();
  leverStable = readLeverRaw();
  lastTick = millis();
}

void loop() {
  unsigned long now = millis();
  unsigned long when = now;
  bool leverMoved = updateLever(when);
  // Charge the running clock up to now - or only up to the lever's first edge
  // while a press is being debounced, so the switch time is exact.
  advanceTo(leverMoved ? when : leverPending() ? leverCandidateSince : now);
  lowTimeAlerts();
  BtnEvent eP = updateButton(btnPause);
  BtnEvent eR = updateButton(btnReset);
  BtnEvent eM = updateButton(btnMode);

  switch (state) {
    case SETUP:
      if (eM == EV_SHORT) { presetIdx = (presetIdx + 1) % NUM_PRESETS; loadPreset(); }
      if (eR == EV_SHORT) { presetIdx = (presetIdx + NUM_PRESETS - 1) % NUM_PRESETS; loadPreset(); }
      if (eM == EV_LONG)  { soundOn = !soundOn; beep(2000, 60); }
      if (leverMoved)          startClock(upSide(leverStable), when);
      else if (eP == EV_PRESS) startClock(upSide(leverStable), now);
      break;

    case RUNNING:
      if (leverMoved && upSide(leverStable) != active) endTurn(when);
      else if (eP == EV_PRESS) { advanceTo(now); if (state == RUNNING) state = PAUSED; }
      break;

    case PAUSED:
      if (eP == EV_PRESS) {
        uint8_t up = upSide(leverStable);
        if (up != SIDE_NONE && up != active) { active = up; turnUsed = 0; }   // lever moved while paused
        lastTick = now;
        state = RUNNING;
      }
      if (eR == EV_LONG)  enterSetup();
      if (eR == EV_SHORT) hintUntil = now + 2000;
      break;

    case FLAGGED:
      if (eP == EV_PRESS || eR == EV_SHORT) enterSetup();
      break;
  }
}

// =====================================================================
// Core 1: drawing
// =====================================================================

// ---- 7-segment digits, drawn like a real clock LCD ----
const uint8_t SEGMAP[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void hseg(int x0, int x1, int yc, int t) {
  int h = t / 2;
  display.fillRect(x0 + h, yc - h, x1 - x0 - 2 * h + 1, t, BLACK);
  display.fillTriangle(x0, yc, x0 + h, yc - h, x0 + h, yc - h + t - 1, BLACK);
  display.fillTriangle(x1, yc, x1 - h, yc - h, x1 - h, yc - h + t - 1, BLACK);
}
void vseg(int xc, int y0, int y1, int t) {
  int h = t / 2;
  display.fillRect(xc - h, y0 + h, t, y1 - y0 - 2 * h + 1, BLACK);
  display.fillTriangle(xc, y0, xc - h, y0 + h, xc - h + t - 1, y0 + h, BLACK);
  display.fillTriangle(xc, y1, xc - h, y1 - h, xc - h + t - 1, y1 - h, BLACK);
}

void drawDigit(int x, int y, int w, int h, int t, uint8_t d) {
  uint8_t m = SEGMAP[d % 10];
  int g = 2;                                   // gap between segments
  int xl = x + t / 2, xr = x + w - 1 - t / 2;
  int yt = y + t / 2, ym = y + h / 2, yb = y + h - 1 - t / 2;
  if (m & 0x01) hseg(xl + g, xr - g, yt, t);   // a
  if (m & 0x02) vseg(xr, yt + g, ym - g, t);   // b
  if (m & 0x04) vseg(xr, ym + g, yb - g, t);   // c
  if (m & 0x08) hseg(xl + g, xr - g, yb, t);   // d
  if (m & 0x10) vseg(xl, ym + g, yb - g, t);   // e
  if (m & 0x20) vseg(xl, yt + g, ym - g, t);   // f
  if (m & 0x40) hseg(xl + g, xr - g, ym, t);   // g
}

const SegFont BIG   = {38, 84, 9, 7, 14, 12};   // "5:00", "19.5"
const SegFont MID   = {32, 76, 8, 6, 12, 11};   // "10:00"
const SegFont SMALL = {24, 58, 6, 4, 10, 9};    // "1:30:00"
const int TIME_MAX_W = 176;                       // each half is 200 px wide

int segTextWidth(const char *s, const SegFont &f) {
  int w = 0;
  for (const char *c = s; *c; c++) {
    if (c != s) w += f.gap;
    w += (*c == ':') ? f.colonW : (*c == '.') ? f.dotW : f.w;
  }
  return w;
}

void drawSegText(int cx, int y, const char *s, const SegFont &f) {
  int x = cx - segTextWidth(s, f) / 2;
  for (const char *c = s; *c; c++) {
    if (c != s) x += f.gap;
    if (*c == ':') {
      int sq = f.t;
      display.fillRect(x + (f.colonW - sq) / 2, y + f.h * 3 / 10 - sq / 2, sq, sq, BLACK);
      display.fillRect(x + (f.colonW - sq) / 2, y + f.h * 7 / 10 - sq / 2, sq, sq, BLACK);
      x += f.colonW;
    } else if (*c == '.') {
      int sq = f.t + 1;
      display.fillRect(x + (f.dotW - sq) / 2, y + f.h - sq, sq, sq, BLACK);
      x += f.dotW;
    } else if (*c >= '0' && *c <= '9') {
      drawDigit(x, y, f.w, f.h, f.t, *c - '0');
      x += f.w;
    } else {
      x += f.w;                                // space
    }
  }
}

// "1:30:00", "45:00", "5:00", "19.5"
void formatTime(long ms, char *out) {
  if (ms < 0) ms = 0;
  unsigned long tenths = ((unsigned long)ms + 99UL) / 100UL;
  if (tenths < (unsigned long)(TENTHS_BELOW_MS / 100)) {
    sprintf(out, "%lu.%lu", tenths / 10, tenths % 10);
    return;
  }
  unsigned long secs = ((unsigned long)ms + 999UL) / 1000UL;
  if (secs >= 3600) sprintf(out, "%lu:%02lu:%02lu", secs / 3600, (secs / 60) % 60, secs % 60);
  else              sprintf(out, "%lu:%02lu", secs / 60, secs % 60);
}

// largest digit size that fits in one half of the screen
const SegFont &fitFont(const char *s) {
  if (segTextWidth(s, BIG) <= TIME_MAX_W) return BIG;
  if (segTextWidth(s, MID) <= TIME_MAX_W) return MID;
  return SMALL;
}

void textCentered(const char *s, int cx, int baseline, const GFXfont *font, uint16_t color) {
  int16_t bx, by; uint16_t bw, bh;
  display.setFont(font);
  display.getTextBounds(s, 0, baseline, &bx, &by, &bw, &bh);
  display.setTextColor(color);
  display.setCursor(cx - (int)bw / 2 - bx, baseline);
  display.print(s);
}

void presetLabel(const Preset &p, char *out) {
  if (p.bonusSec == 0)            sprintf(out, "%u min  sudden death", p.minutes);
  else if (p.mode == FISCHER)     sprintf(out, "%u min + %u s  Fischer", p.minutes, p.bonusSec);
  else if (p.mode == DELAY)       sprintf(out, "%u min  delay %u s", p.minutes, p.bonusSec);
  else                            sprintf(out, "%u min  Bronstein %u s", p.minutes, p.bonusSec);
}

void drawFlag(int x, int y) {                  // little flag, pole at x
  display.fillRect(x, y, 3, 26, BLACK);
  display.fillTriangle(x + 3, y, x + 3, y + 14, x + 20, y + 7, BLACK);
}

void drawScreen() {
  unsigned long now = millis();
  bool blink = (now / 400) % 2;
  State st = state;
  uint8_t act = active;
  const Preset &p = P();
  char buf[40];

  display.clearDisplayBuffer();

  // ---- top strip: running indicators / setup title ----
  if (st == SETUP) {
    sprintf(buf, "PRESET %u / %u", presetIdx + 1, NUM_PRESETS);
    textCentered(buf, 200, 22, &FreeSansBold9pt7b, BLACK);
  } else {
    for (uint8_t s = SIDE_L; s <= SIDE_R; s++) {
      int x0 = s == SIDE_L ? 24 : 224;
      if (s == act && st == RUNNING) {
        bool inDelay = p.mode == DELAY && (long)turnUsed < bonusMs();
        if (!inDelay || (now / 150) % 2) display.fillRoundRect(x0, 8, 152, 10, 5, BLACK);
      } else if (s == act && st == PAUSED) {
        display.drawRoundRect(x0, 8, 152, 10, 5, BLACK);
      }
    }
  }

  // ---- the two clocks ----
  display.drawFastVLine(200, 34, 140, BLACK);
  display.drawFastVLine(201, 34, 140, BLACK);
  for (uint8_t s = SIDE_L; s <= SIDE_R; s++) {
    int cx = s == SIDE_L ? 100 : 300;
    long ms = st == SETUP ? (long)p.minutes * 60000L : remaining[s];
    bool show = true;
    if ((st == PAUSED || st == FLAGGED) && s == act) show = blink;
    if (show) {
      formatTime(ms, buf);
      const SegFont &f = fitFont(buf);
      drawSegText(cx, 40 + (BIG.h - f.h) / 2, buf, f);
    }
    if (st == FLAGGED && s == act) drawFlag(s == SIDE_L ? 8 : 372, 40);
    if (st != SETUP) {
      sprintf(buf, "MOVES %u", moves[s]);
      textCentered(buf, cx, 160, &FreeSansBold9pt7b, BLACK);
    }
  }

  // ---- bottom bar (inverted) ----
  display.fillRect(0, 178, 400, 62, BLACK);
  switch (st) {
    case SETUP:
      presetLabel(p, buf);
      textCentered(buf, 200, 204, &FreeSansBold12pt7b, WHITE);
      textCentered(soundOn ? "R prev   M next   II or lever: start"
                           : "R prev   M next   II start   SOUND OFF",
                   200, 230, &FreeSansBold9pt7b, WHITE);
      break;
    case RUNNING:
      presetLabel(p, buf);
      textCentered(buf, 200, 216, &FreeSansBold12pt7b, WHITE);
      break;
    case PAUSED:
      textCentered("PAUSED", 200, 204, &FreeSansBold12pt7b, WHITE);
      textCentered(now < hintUntil ? "hold R for 1 s to end the game" : "II resume    hold R: new game",
                   200, 230, &FreeSansBold9pt7b, WHITE);
      break;
    case FLAGGED:
      textCentered(act == SIDE_L ? "LEFT FLAG FELL" : "RIGHT FLAG FELL", 200, 204, &FreeSansBold12pt7b, WHITE);
      textCentered("II or R: new game", 200, 230, &FreeSansBold9pt7b, WHITE);
      break;
  }
}

void setup1() {
  SPI.setSCK(PIN_SHARP_SCK);
  SPI.setTX(PIN_SHARP_MOSI);
  display.begin();
  display.setRotation(DISPLAY_ROTATION);
  display.clearDisplay();
}

void loop1() {
  drawScreen();
  display.refresh();          // also toggles VCOM, which the Sharp panel needs regularly
}
