#include <Arduino.h>
#include "scr_st77916.h"
#include <lvgl.h>
#include <ui.h>

void setup()
{
  delay(200);
  Serial.begin(115200);
  scr_lvgl_init();
  ui_init();
}

void loop()
{
  lv_timer_handler();
  vTaskDelay(5);
}
