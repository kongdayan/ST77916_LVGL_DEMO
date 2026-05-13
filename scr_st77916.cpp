#include "scr_st77916.h"
#include <esp_display_panel.hpp>

using namespace esp_panel::drivers;

#define TFT_SPI_FREQ_HZ (50 * 1000 * 1000)

static lv_color_t        *disp_draw_buf;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_t        *indev_touchpad;
static BacklightPWM_LEDC *backlight = nullptr;
static LCD               *lcd       = nullptr;
static Touch             *touch     = nullptr;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    LCD *panel = (LCD *)disp->user_data;
    panel->drawBitmap(area->x1, area->y1,
                      area->x2 - area->x1 + 1,
                      area->y2 - area->y1 + 1,
                      (const uint8_t *)color_p);
}

IRAM_ATTR bool onDrawBitmapFinishCallback(void *user_data)
{
    lv_disp_drv_t *drv = (lv_disp_drv_t *)user_data;
    lv_disp_flush_ready(drv);
    return false;
}

#if TOUCH_PIN_NUM_INT >= 0
IRAM_ATTR bool onTouchInterruptCallback(void *user_data)
{
    return false;
}
#endif

void setRotation(uint8_t rot)
{
    if (rot > 3 || lcd == nullptr || touch == nullptr)
        return;

    bool swap, mirX, mirY;
    switch (rot) {
    case 1: swap = true;  mirX = true;  mirY = false; break;
    case 2: swap = false; mirX = true;  mirY = true;  break;
    case 3: swap = true;  mirX = false; mirY = true;  break;
    default: swap = false; mirX = false; mirY = false; break;
    }
    lcd->swapXY(swap);   lcd->mirrorX(mirX);   lcd->mirrorY(mirY);
    touch->swapXY(swap); touch->mirrorX(mirX); touch->mirrorY(mirY);
}

void screen_switch(bool on)
{
    if (backlight == nullptr) return;
    if (on) backlight->on(); else backlight->off();
}

void set_brightness(uint8_t bri)
{
    if (backlight == nullptr) return;
    backlight->setBrightness(bri);
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    Touch *tp = (Touch *)indev_drv->user_data;
    TouchPoint point;
    data->state = LV_INDEV_STATE_RELEASED;

    if (tp->readRawData(1, 0, 0)) {
        if (tp->getPoints(&point, 1) > 0) {
            data->point.x = point.x;
            data->point.y = point.y;
            data->state = LV_INDEV_STATE_PRESSED;
        }
    }
}

static lv_indev_t *indev_init(Touch *tp)
{
    if (tp == nullptr || tp->getPanelHandle() == nullptr) return nullptr;

    static lv_indev_drv_t indev_drv_tp;
    lv_indev_drv_init(&indev_drv_tp);
    indev_drv_tp.type      = LV_INDEV_TYPE_POINTER;
    indev_drv_tp.read_cb   = touchpad_read;
    indev_drv_tp.user_data = (void *)tp;
    return lv_indev_drv_register(&indev_drv_tp);
}

void scr_lvgl_init()
{
    backlight = new BacklightPWM_LEDC(TFT_BLK, true);
    backlight->begin();
    backlight->off();

    BusI2C *touch_bus = new BusI2C(
        TOUCH_PIN_NUM_I2C_SCL, TOUCH_PIN_NUM_I2C_SDA,
        (BusI2C::ControlPanelFullConfig)ESP_PANEL_TOUCH_I2C_CONTROL_PANEL_CONFIG(CST816S));
    touch_bus->configI2C_FreqHz(400000);

    touch = new TouchCST816S(touch_bus, SCREEN_RES_HOR, SCREEN_RES_VER,
                              TOUCH_PIN_NUM_RST, TOUCH_PIN_NUM_INT);
    touch->begin();
#if TOUCH_PIN_NUM_INT >= 0
    touch->attachInterruptCallback(onTouchInterruptCallback, nullptr);
#endif

    BusQSPI *panel_bus = new BusQSPI(
        TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
    panel_bus->configQSPI_FreqHz(TFT_SPI_FREQ_HZ);

    lcd = new LCD_ST77916(panel_bus, SCREEN_RES_HOR, SCREEN_RES_VER, 16, TFT_RST);
    lcd->begin();
    lcd->invertColor(true);
    lcd->setDisplayOnOff(true);

    backlight->on();
    backlight->setBrightness(100);

    const size_t lv_cache_rows = 72;
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(
        lv_cache_rows * SCREEN_RES_HOR * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, SCREEN_RES_HOR * lv_cache_rows);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res   = SCREEN_RES_HOR;
    disp_drv.ver_res   = SCREEN_RES_VER;
    disp_drv.flush_cb  = my_disp_flush;
    disp_drv.draw_buf  = &draw_buf;
    disp_drv.user_data = (void *)lcd;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    lcd->attachDrawBitmapFinishCallback(onDrawBitmapFinishCallback, (void *)disp->driver);

    indev_touchpad = indev_init(touch);
}
