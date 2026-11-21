#pragma once
#include "Adafruit_GFX.h"
class Adafruit_SharpMem : public Adafruit_GFX { public:
  uint8_t buf[400*240];
  Adafruit_SharpMem(SPIClass*,uint8_t,uint16_t w,uint16_t h,uint32_t=0):Adafruit_GFX(w,h){memset(buf,1,sizeof buf);}
  bool begin(){return true;}
  void drawPixel(int16_t x,int16_t y,uint16_t c){ if(x<0||y<0||x>=width()||y>=height())return; buf[y*400+x]=c?1:0; }
  void clearDisplayBuffer(){memset(buf,1,sizeof buf);}
  void clearDisplay(){clearDisplayBuffer();}
  void refresh(){}
  void save(const char*fn){ FILE*f=fopen(fn,"wb"); fprintf(f,"P5 400 240 255\n"); for(int i=0;i<400*240;i++){unsigned char v=buf[i]?0xC8:0x22; fputc(v,f);} fclose(f);}
};
