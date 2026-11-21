// Types shared by the sketch. They live in a header (not in the .ino) because the Arduino
// builder generates function prototypes near the top of the sketch, before any type defined
// in the .ino itself, so functions that take or return these types would not compile.
#pragma once
#include <stdint.h>

enum TimingMode : uint8_t { FISCHER, DELAY, BRONSTEIN };
struct Preset { uint8_t minutes; uint8_t bonusSec; TimingMode mode; };

enum State : uint8_t { SETUP, RUNNING, PAUSED, FLAGGED };

enum LeverPos : uint8_t { LEVER_MID, LEVER_LEFT_DOWN, LEVER_RIGHT_DOWN };

enum BtnEvent { EV_NONE, EV_PRESS, EV_SHORT, EV_LONG };
struct Button { uint8_t pin; bool down; bool longFired; unsigned long changedAt; };

struct SegFont { int w, h, t, gap, colonW, dotW; };
