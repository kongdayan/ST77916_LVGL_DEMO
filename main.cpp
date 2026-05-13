#include <Arduino.h>
#include "display.h"
#include <lvgl.h>
#include "ui.h"

void setup()
{
  delay(200);
  Serial.begin(115200);
  display_init();
  ui_init();
}

void loop()
{
  lv_timer_handler();
  vTaskDelay(5);
}
