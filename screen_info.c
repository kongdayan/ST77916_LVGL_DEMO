#include "ui.h"

static lv_obj_t *scr = NULL;

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());
    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_image_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_image_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_dashboard_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_dashboard_init);
}

static void build_tab_online(lv_obj_t *tab)
{
    lv_obj_t *spinner = lv_spinner_create(tab, 1000, 90);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_set_align(spinner, LV_ALIGN_CENTER);
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_CLICKABLE);
}

static void build_tab_calendar(lv_obj_t *tab)
{
    lv_obj_t *cal = lv_calendar_create(tab);
    lv_calendar_set_today_date(cal, 2024, 12, 17);
    lv_calendar_set_showed_date(cal, 2024, 12);
    lv_calendar_header_arrow_create(cal);
    lv_obj_set_size(cal, 200, 200);
    lv_obj_set_pos(cal, -8, -8);
    lv_obj_set_align(cal, LV_ALIGN_CENTER);
}

static void build_tab_setting(lv_obj_t *tab)
{
    // Wi-Fi row
    lv_obj_t *lbl_wifi = lv_label_create(tab);
    lv_label_set_text(lbl_wifi, "WI-FI");
    lv_obj_set_pos(lbl_wifi, -70, -90);
    lv_obj_set_align(lbl_wifi, LV_ALIGN_CENTER);

    lv_obj_t *sw_wifi = lv_switch_create(tab);
    lv_obj_set_size(sw_wifi, 50, 25);
    lv_obj_set_pos(sw_wifi, 45, -90);
    lv_obj_set_align(sw_wifi, LV_ALIGN_CENTER);

    // Bluetooth row
    lv_obj_t *lbl_bt = lv_label_create(tab);
    lv_label_set_text(lbl_bt, "Bluetooth");
    lv_obj_set_pos(lbl_bt, -59, -61);
    lv_obj_set_align(lbl_bt, LV_ALIGN_CENTER);

    lv_obj_t *sw_bt = lv_switch_create(tab);
    lv_obj_set_size(sw_bt, 50, 25);
    lv_obj_set_pos(sw_bt, 45, -60);
    lv_obj_set_align(sw_bt, LV_ALIGN_CENTER);

    // Volume slider
    lv_obj_t *lbl_vol = lv_label_create(tab);
    lv_label_set_text(lbl_vol, "Volume");
    lv_obj_set_pos(lbl_vol, -63, -30);
    lv_obj_set_align(lbl_vol, LV_ALIGN_CENTER);

    lv_obj_t *slider = lv_slider_create(tab);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_set_size(slider, 150, 10);
    lv_obj_set_pos(slider, 3, 1);
    lv_obj_set_align(slider, LV_ALIGN_CENTER);

    // Auto-scan checkbox
    lv_obj_t *chk = lv_checkbox_create(tab);
    lv_checkbox_set_text(chk, "Enable Auto Scan");
    lv_obj_set_pos(chk, -25, 38);
    lv_obj_set_align(chk, LV_ALIGN_CENTER);
}

void screen_info_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    lv_obj_t *tabview = lv_tabview_create(scr, LV_DIR_LEFT, 50);
    lv_obj_set_size(tabview, 270, 220);
    lv_obj_set_align(tabview, LV_ALIGN_CENTER);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    build_tab_online(lv_tabview_add_tab(tabview, "Online"));
    build_tab_calendar(lv_tabview_add_tab(tabview, "Calendar"));
    build_tab_setting(lv_tabview_add_tab(tabview, "Setting"));
}

lv_obj_t **screen_info_get_ptr(void) { return &scr; }
