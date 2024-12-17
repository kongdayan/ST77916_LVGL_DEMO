#include "scr_st77916.h"
#include <lvgl.h>
// #include <demos/lv_demos.h>
#include <ui.h>

void setup()
{
  delay(200);
  Serial.begin(115200);
  scr_lvgl_init();
  // lv_demo_widgets();
  ui_init();

  // lv_demo_benchmark();
  //  lv_demo_music();
}

void loop()
{
  lv_timer_handler();
  vTaskDelay(5);
}
