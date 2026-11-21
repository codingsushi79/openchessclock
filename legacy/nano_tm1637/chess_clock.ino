/*
  Arduino Chess Clock  (v2)
  -------------------------
  Hardware:
    - Arduino Uno / Nano
    - 2x TM1637 4-digit 7-segment display (the "clock" kind with a colon)
    - 2x player buttons (big arcade buttons or lever microswitches)
    - 1x mode button (small tactile / 12 mm panel button)
    - 1x passive buzzer
    - 2x LEDs + 220 ohm resistors (shows whose turn it is)

  All buttons are wired between the pin and GND (internal pull-ups are used).

  Timing modes (picked per preset):
    In  = Fischer increment : bonus added to your clock after every move.
    dL  = Simple (US) delay : your clock waits <bonus> seconds each turn
                              before it starts counting down. Your turn
                              LED blinks fast while the delay is running.
    br  = Bronstein delay   : clock runs immediately; after your move you get
                              back the time you used, up to <bonus> seconds.
    --  = Sudden death (no bonus).

  Controls:
    SETUP    : MODE (short) cycles presets. Left display = base time,
               right display = timing mode + bonus seconds.
               Either player button starts the game. Pressing YOUR button
               starts your OPPONENT's clock (so Black presses to start White).
    RUNNING  : Press your button to end your turn. MODE (short) pauses.
    PAUSED   : MODE (short) resumes. The paused side blinks.
               Hold either player button to see move counts ("n 23").
    FLAGGED  : The side that ran out blinks 0:00. Hold a player button to see
               move counts. MODE returns to setup.
    Anywhere : Hold MODE for 1.5 s to reset to setup.

  Display:
    - Normal:      M:SS / MM:SS
    - Under 20 s:  seconds + tenths. On colon-style modules this reads "19:5 "
                   (seconds, colon, tenths). If your module has decimal points
                   instead of a colon, set DISPLAY_HAS_DP = true for " 19.5".

  Low-time warning:
    - One longer beep when a clock drops below LOW_TIME_WARN_MS (10 s).
    - A short tick every second for the last LOW_TIME_TICK_SECS (5) seconds.

  Library: "TM1637" by Avishay Orpaz (Library Manager -> search TM1637Display)
*/

#include <TM1637Display.h>

// ---------- Pins ----------
const uint8_t PIN_L_CLK = 2, PIN_L_DIO = 3;
const uint8_t PIN_R_CLK = 4, PIN_R_DIO = 5;
const uint8_t PIN_BTN_L = 6, PIN_BTN_R = 7, PIN_BTN_MODE = 8;
const uint8_t PIN_BUZZER = 9;
const uint8_t PIN_LED_L = 10, PIN_LED_R = 11;

// ---------- Settings ----------
const bool CLICK_SOUND = false;              // short tick on every move
const bool DISPLAY_HAS_DP = false;           // true if module has decimal points, not a colon
const unsigned long DEBOUNCE_MS = 25;
const unsigned long LONG_PRESS_MS = 1500;
const uint8_t BRIGHTNESS = 5;                // 0..7
const long TENTHS_BELOW_MS = 20000;          // show tenths under this
const long LOW_TIME_WARN_MS = 10000;         // one warning beep (0 = off)
const uint8_t LOW_TIME_TICK_SECS = 5;        // tick each second below this (0 = off)

enum TimingMode { FISCHER, DELAY, BRONSTEIN };

struct Preset { uint8_t minutes; uint8_t bonusSec; TimingMode mode; };
const Preset PRESETS[] = {
  {1,  0,  FISCHER},     // 1 min bullet
  {2,  1,  FISCHER},     // 2+1 bullet
  {3,  0,  FISCHER},     // 3 min blitz
  {3,  2,  FISCHER},     // 3+2 blitz
  {3,  2,  DELAY},       // 3 d2
  {5,  0,  FISCHER},     // 5 min blitz
  {5,  3,  FISCHER},     // 5+3 blitz
  {5,  5,  DELAY},       // G/5 d5 (US blitz)
  {5,  3,  BRONSTEIN},   // 5 br3
  {10, 0,  FISCHER},     // 10 min rapid
  {10, 5,  FISCHER},     // 10+5 rapid
  {15, 10, FISCHER},     // 15+10 rapid
  {25, 5,  DELAY},       // G/25 d5
  {30, 0,  FISCHER},     // 30 min
  {90, 30, FISCHER},     // 90+30 classical
  {90, 30, DELAY},       // G/90 d30
};
const uint8_t NUM_PRESETS = sizeof(PRESETS) / sizeof(PRESETS[0]);
uint8_t presetIdx = 9;                       // default 10+0

// ---------- State ----------
TM1637Display dispL(PIN_L_CLK, PIN_L_DIO);
TM1637Display dispR(PIN_R_CLK, PIN_R_DIO);

enum State { SETUP, RUNNING, PAUSED, FLAGGED };
State state = SETUP;

const uint8_t SIDE_L = 0, SIDE_R = 1, SIDE_NONE = 2;
long remaining[2];                           // ms left per side
uint16_t moves[2];                           // moves completed per side
bool warned[2];                              // low-time warning already given
uint8_t lastTickSec[2];                      // last second we ticked on
uint8_t active = SIDE_NONE;
unsigned long turnUsed = 0;                  // ms used in the current turn (excludes pauses)
unsigned long lastTick = 0;

// ---------- Buttons ----------
enum BtnEvent { EV_NONE, EV_PRESS, EV_SHORT, EV_LONG };

struct Button {
  uint8_t pin;
  bool down;
  bool longFired;
  unsigned long changedAt;
};

Button btnL    = {PIN_BTN_L,    false, false, 0};
Button btnR    = {PIN_BTN_R,    false, false, 0};
Button btnMode = {PIN_BTN_MODE, false, false, 0};

// Registers a change immediately, then ignores bounces for DEBOUNCE_MS.
// EV_PRESS fires on press, EV_SHORT on release of a short press,
// EV_LONG once while held past LONG_PRESS_MS.
BtnEvent updateButton(Button &b) {
  unsigned long now = millis();
  bool raw = digitalRead(b.pin) == LOW;

  if (raw != b.down && now - b.changedAt >= DEBOUNCE_MS) {
    b.down = raw;
    b.changedAt = now;
    if (raw) {
      b.longFired = false;
      return EV_PRESS;
    }
    return b.longFired ? EV_NONE : EV_SHORT;
  }

  if (b.down && !b.longFired && now - b.changedAt >= LONG_PRESS_MS) {
    b.longFired = true;
    return EV_LONG;
  }
  return EV_NONE;
}

// ---------- Display helpers ----------
const uint8_t SEG_BLANK = 0;
const uint8_t CH_I = SEG_E | SEG_F;
const uint8_t CH_n = SEG_C | SEG_E | SEG_G;
const uint8_t CH_d = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
const uint8_t CH_L = SEG_D | SEG_E | SEG_F;
const uint8_t CH_b = SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
const uint8_t CH_r = SEG_E | SEG_G;
const uint8_t CH_DASH = SEG_G;

TM1637Display &displayFor(uint8_t side) {
  return side == SIDE_L ? dispL : dispR;
}

// Normal: M:SS / MM:SS, rounded up so 0:00 only appears at the flag.
// Under TENTHS_BELOW_MS: seconds + tenths.
void showTime(TM1637Display &d, long ms, bool visible) {
  if (!visible) { d.clear(); return; }
  if (ms < 0) ms = 0;
  uint8_t seg[4];

  unsigned long tenths = ((unsigned long)ms + 99UL) / 100UL;
  if (tenths < (unsigned long)(TENTHS_BELOW_MS / 100)) {
    uint8_t s = tenths / 10;
    uint8_t t = tenths % 10;
    if (DISPLAY_HAS_DP) {                    // " 19.5"
      seg[0] = SEG_BLANK;
      seg[1] = s >= 10 ? d.encodeDigit(s / 10) : SEG_BLANK;
      seg[2] = d.encodeDigit(s % 10) | 0x80;
      seg[3] = d.encodeDigit(t);
    } else {                                 // "19:5 "
      seg[0] = s >= 10 ? d.encodeDigit(s / 10) : SEG_BLANK;
      seg[1] = d.encodeDigit(s % 10) | 0x80;
      seg[2] = d.encodeDigit(t);
      seg[3] = SEG_BLANK;
    }
    d.setSegments(seg);
    return;
  }

  unsigned long secs = ((unsigned long)ms + 999UL) / 1000UL;
  unsigned long mm = secs / 60;
  if (mm > 99) mm = 99;
  uint8_t ss = secs % 60;

  seg[0] = mm >= 10 ? d.encodeDigit(mm / 10) : SEG_BLANK;
  seg[1] = d.encodeDigit(mm % 10) | 0x80;    // colon (or DP after digit 2)
  seg[2] = d.encodeDigit(ss / 10);
  seg[3] = d.encodeDigit(ss % 10);
  d.setSegments(seg);
}

// Two letters + right-aligned 2-digit number, e.g. "In 5", "dL30", "br 3".
void showTagged(TM1637Display &d, uint8_t c0, uint8_t c1, uint8_t num) {
  uint8_t seg[4];
  seg[0] = c0;
  seg[1] = c1;
  seg[2] = num >= 10 ? d.encodeDigit((num / 10) % 10) : SEG_BLANK;
  seg[3] = d.encodeDigit(num % 10);
  d.setSegments(seg);
}

void showMode(TM1637Display &d, const Preset &p) {
  if (p.bonusSec == 0) {
    uint8_t seg[4] = {CH_DASH, CH_DASH, CH_DASH, CH_DASH};
    d.setSegments(seg);
    return;
  }
  switch (p.mode) {
    case FISCHER:   showTagged(d, CH_I, CH_n, p.bonusSec); break;
    case DELAY:     showTagged(d, CH_d, CH_L, p.bonusSec); break;
    case BRONSTEIN: showTagged(d, CH_b, CH_r, p.bonusSec); break;
  }
}

// "n 23", "n123"
void showMoves(TM1637Display &d, uint16_t n) {
  if (n > 999) n = 999;
  uint8_t seg[4];
  seg[0] = CH_n;
  seg[1] = n >= 100 ? d.encodeDigit(n / 100) : SEG_BLANK;
  seg[2] = n >= 10 ? d.encodeDigit((n / 10) % 10) : SEG_BLANK;
  seg[3] = d.encodeDigit(n % 10);
  d.setSegments(seg);
}

// ---------- Game logic ----------
const Preset &P() { return PRESETS[presetIdx]; }
long bonusMs() { return (long)P().bonusSec * 1000L; }

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

void startClock(uint8_t side) {
  active = side;
  turnUsed = 0;
  state = RUNNING;
  lastTick = millis();
  if (CLICK_SOUND) tone(PIN_BUZZER, 3000, 15);
}

void endTurn() {
  switch (P().mode) {
    case FISCHER:
      remaining[active] += bonusMs();
      break;
    case BRONSTEIN:
      remaining[active] += ((long)turnUsed < bonusMs()) ? (long)turnUsed : bonusMs();
      break;
    case DELAY:
      break;                                 // delay time was never charged
  }
  moves[active]++;

  // Re-arm the low-time warning if the bonus lifted us back above it.
  if (remaining[active] > LOW_TIME_WARN_MS) warned[active] = false;
  lastTickSec[active] = 255;

  active = (active == SIDE_L) ? SIDE_R : SIDE_L;
  turnUsed = 0;
  if (CLICK_SOUND) tone(PIN_BUZZER, 3000, 15);
}

void lowTimeAlerts() {
  long r = remaining[active];
  if (LOW_TIME_WARN_MS > 0 && !warned[active] && r <= LOW_TIME_WARN_MS) {
    warned[active] = true;
    tone(PIN_BUZZER, 1800, 250);
    return;
  }
  if (LOW_TIME_TICK_SECS > 0) {
    uint8_t s = (uint8_t)(((unsigned long)r + 999UL) / 1000UL);   // r <= a few s here
    if (r <= (long)LOW_TIME_TICK_SECS * 1000L && s != lastTickSec[active]) {
      lastTickSec[active] = s;
      tone(PIN_BUZZER, 2500, 30);
    }
  }
}

void tick() {
  unsigned long now = millis();
  if (state == RUNNING) {
    unsigned long dt = now - lastTick;
    long charge = (long)dt;

    if (P().mode == DELAY) {
      // Only the part of this turn beyond the delay is charged.
      long d = bonusMs();
      long before = (long)turnUsed - d;  if (before < 0) before = 0;
      long after  = (long)(turnUsed + dt) - d;  if (after < 0) after = 0;
      charge = after - before;
    }
    turnUsed += dt;
    remaining[active] -= charge;

    if (remaining[active] <= 0) {
      remaining[active] = 0;
      state = FLAGGED;
      tone(PIN_BUZZER, 880, 1500);
    } else {
      lowTimeAlerts();
    }
  }
  lastTick = now;
}

void render() {
  static unsigned long lastRender = 0;
  unsigned long now = millis();
  if (now - lastRender < 50) return;         // ~20 fps
  lastRender = now;

  bool blinkOn = (now / 400) % 2;
  bool blinkFast = (now / 125) % 2;

  if (state == SETUP) {
    showTime(dispL, (long)P().minutes * 60000L, true);
    showMode(dispR, P());
  } else if ((state == PAUSED || state == FLAGGED) && (btnL.down || btnR.down)) {
    showMoves(dispL, moves[SIDE_L]);
    showMoves(dispR, moves[SIDE_R]);
  } else {
    for (uint8_t side = SIDE_L; side <= SIDE_R; side++) {
      bool visible = true;
      if ((state == PAUSED || state == FLAGGED) && side == active) visible = blinkOn;
      showTime(displayFor(side), remaining[side], visible);
    }
  }

  bool inDelay = state == RUNNING && P().mode == DELAY && (long)turnUsed < bonusMs();
  bool ledOn = inDelay ? blinkFast : true;
  digitalWrite(PIN_LED_L, state == RUNNING && active == SIDE_L && ledOn);
  digitalWrite(PIN_LED_R, state == RUNNING && active == SIDE_R && ledOn);
}

// ---------- Arduino ----------
void setup() {
  pinMode(PIN_BTN_L, INPUT_PULLUP);
  pinMode(PIN_BTN_R, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_LED_L, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  dispL.setBrightness(BRIGHTNESS);
  dispR.setBrightness(BRIGHTNESS);

  enterSetup();
}

void loop() {
  tick();

  BtnEvent eL = updateButton(btnL);
  BtnEvent eR = updateButton(btnR);
  BtnEvent eM = updateButton(btnMode);

  if (eM == EV_LONG) enterSetup();

  switch (state) {
    case SETUP:
      if (eM == EV_SHORT) {
        presetIdx = (presetIdx + 1) % NUM_PRESETS;
        loadPreset();
      }
      if (eL == EV_PRESS)      startClock(SIDE_R);  // left pressed -> right's clock runs
      else if (eR == EV_PRESS) startClock(SIDE_L);
      break;

    case RUNNING:
      if (eM == EV_SHORT) state = PAUSED;
      else if (eL == EV_PRESS && active == SIDE_L) endTurn();
      else if (eR == EV_PRESS && active == SIDE_R) endTurn();
      break;

    case PAUSED:
      if (eM == EV_SHORT) state = RUNNING;   // tick() keeps lastTick fresh, so no time jump
      break;

    case FLAGGED:
      if (eM == EV_SHORT) enterSetup();
      break;
  }

  render();
}
