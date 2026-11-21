#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
typedef bool boolean; typedef uint8_t byte;
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define pgm_read_word(a) (*(const uint16_t*)(a))
#define pgm_read_dword(a) (*(const uint32_t*)(a))
#define pgm_read_pointer(a) (*(void* const*)(a))
#define LOW 0
#define HIGH 1
#define INPUT_PULLUP 2
#define OUTPUT 1
#define INPUT 0
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef _swap_int16_t
#endif
class __FlashStringHelper;
extern unsigned long g_ms; extern int g_pins[40]; extern int g_tones;
inline unsigned long millis(){return g_ms;}
inline int digitalRead(uint8_t p){return g_pins[p];}
inline void digitalWrite(uint8_t p,int v){g_pins[p]=v;}
inline void pinMode(uint8_t,int){}
inline void tone(uint8_t,unsigned,unsigned long=0){g_tones++;}
inline void noTone(uint8_t){}
inline void delay(unsigned long){}
#define radians(d) ((d)*0.017453292519943295)
#define degrees(r) ((r)*57.29577951308232)
struct String { std::string s; String(const char*c=""):s(c){} unsigned int length() const {return s.size();} const char* c_str() const {return s.c_str();} };
#include "Print.h"
struct SPIClass { void setSCK(int){} void setTX(int){} void begin(){} };
extern SPIClass SPI;
