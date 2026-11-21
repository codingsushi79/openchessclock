#include "Arduino.h"
unsigned long g_ms=0; int g_pins[40]; int g_tones=0; SPIClass SPI;
#include "sketch.inc"
int shot=0;
void run(unsigned long ms){ unsigned long end=g_ms+ms; while(g_ms<end){ g_ms++; loop(); } }
void snap(const char*name){ drawScreen(); char fn[64]; sprintf(fn,"shots/%02d_%s.pgm",shot++,name); display.save(fn); }
void press(int pin, unsigned long hold=80){ g_pins[pin]=LOW; run(hold); g_pins[pin]=HIGH; run(40); }
void lever(int side){ // 0 = press left end down, 1 = right end down
  g_pins[PIN_LEVER_L]=HIGH; g_pins[PIN_LEVER_R]=HIGH; run(15);               // mid travel
  g_pins[side==0?PIN_LEVER_L:PIN_LEVER_R]=LOW; run(20); }
void st(const char*l){ printf("%-38s state=%d act=%d L=%7ld R=%7ld moves=%u/%u tones=%d preset=%u\n",l,state,active,remaining[0],remaining[1],moves[0],moves[1],g_tones,presetIdx); }
int main(){
  setvbuf(stdout,NULL,_IONBF,0); for(int i=0;i<40;i++) g_pins[i]=HIGH;
  g_pins[PIN_LEVER_L]=LOW;                    // lever resting left-end-down
  setup(); setup1(); run(50); st("boot"); snap("setup_10_0");
  while(presetIdx!=7) press(PIN_BTN_MODE); st("5+3 Fischer"); snap("setup_5_3");
  // Black (right player) presses right end -> left (White) clock runs
  lever(1); st("right end down -> left runs");
  run(4000); st("left used 4s"); snap("running");
  lever(0); st("left moved (+3s)");
  run(2500); lever(1); st("right moved after 2.5s");
  press(PIN_BTN_PAUSE); run(3000); st("paused 3s"); snap("paused");
  press(PIN_BTN_RESET); snap("paused_hint");
  press(PIN_BTN_PAUSE); st("resumed");
  // long game: 90+30 (H:MM:SS display)
  press(PIN_BTN_PAUSE); g_pins[PIN_BTN_RESET]=LOW; run(1100); g_pins[PIN_BTN_RESET]=HIGH; run(50); st("pause + long-R -> setup");
  while(presetIdx!=17) press(PIN_BTN_MODE); snap("setup_90_30");
  press(PIN_BTN_PAUSE); st("II start (left end down -> left? )"); run(1000); snap("running_hours");
  // tenths + delay + flag: 1+0 , left flag
  press(PIN_BTN_PAUSE);
  g_pins[PIN_BTN_RESET]=LOW; run(1100); g_pins[PIN_BTN_RESET]=HIGH; run(50);
  while(presetIdx!=0) press(PIN_BTN_MODE);
  int t0=g_tones; lever(0); lever(1); st("1+0 start, right ran ~35ms, left now runs");
  run(45000); snap("tenths"); st("left ~15s");
  run(16000); st("left flagged?"); snap("flagged");
  printf("tones during 1+0 run: %d (expect warn+5 ticks+flag = 7)\n", g_tones-t0);
  // US delay 3 d2
  press(PIN_BTN_RESET); st("flagged + R -> setup"); while(presetIdx!=5) press(PIN_BTN_MODE); st("3 d2 setup");
  lever(0); run(1500); st("right in delay 1.5s (no charge)"); snap("delay");
  run(1000); st("right 2.5s (0.5 charged)");
  lever(1); st("right moved");
  // Bronstein 5 br3
  press(PIN_BTN_PAUSE); g_pins[PIN_BTN_RESET]=LOW; run(1100); g_pins[PIN_BTN_RESET]=HIGH; run(50);
  while(presetIdx!=9) press(PIN_BTN_MODE); lever(0); run(1000); lever(1); st("bronstein: right used ~1s -> back to 300000");
  run(6000); lever(0); st("left used ~6s -> loses ~3s");
  // mid-travel bounce shouldn't count as a move
  g_pins[PIN_LEVER_L]=HIGH; run(3); g_pins[PIN_LEVER_L]=LOW; run(30); st("bounce ignored (no move change)");
}
