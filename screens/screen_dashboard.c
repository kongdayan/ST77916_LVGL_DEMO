#include "ui.h"

static lv_obj_t *scr   = NULL;
static lv_obj_t *arc_s = NULL;  // 50x50  red
static lv_obj_t *arc_m = NULL;  // 90x90  yellow
static lv_obj_t *arc_l = NULL;  // 130x130 green

static void on_btn_plus(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED_REPEAT) return;
    _ui_arc_increment(arc_l, 6);
    _ui_arc_increment(arc_m, 4);
    _ui_arc_increment(arc_s, 2);
}

static void on_btn_minus(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED_REPEAT) return;
    _ui_arc_increment(arc_l, -6);
    _ui_arc_increment(arc_m, -4);
    _ui_arc_increment(arc_s, -2);
}

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());
    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_info_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_info_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_agent_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_agent_init);
}

static lv_obj_t *make_arc(lv_obj_t *parent, int size, uint32_t color)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_align(arc, LV_ALIGN_CENTER);
    lv_arc_set_value(arc, 50);
    lv_obj_set_style_arc_color(arc, lv_color_hex(color), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    return arc;
}

static lv_obj_t *make_btn(lv_obj_t *parent, int x, int y, const char *label, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_set_x(btn, x);
    lv_obj_set_y(btn, y);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    return btn;
}

void screen_dashboard_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    arc_s = make_arc(scr,  50, 0xFF6666);
    arc_m = make_arc(scr,  90, 0xFFFF66);
    arc_l = make_arc(scr, 130, 0x99CC66);

    make_btn(scr, -100, -100, "+", on_btn_plus);
    make_btn(scr,  100, -100, "-", on_btn_minus);
}

lv_obj_t **screen_dashboard_get_ptr(void) { return &scr; }
