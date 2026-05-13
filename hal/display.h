#ifndef DISPLAY_H
#define DISPLAY_H

#include "pincfg.h"
#include <lvgl.h>

#define SCREEN_RES_HOR 360
#define SCREEN_RES_VER 360

void display_init(void);
void setRotation(uint8_t rot);
void screen_switch(bool on);
void set_brightness(uint8_t bri);

#endif
