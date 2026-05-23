#include "ui.h"

static lv_obj_t *scr = NULL;

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());
    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_codex_usage_get_ptr(),
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, screen_codex_usage_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_video_get_ptr(),
                          LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_video_init);
}

void screen_about_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x02050A), LV_PART_MAIN);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_size(panel, 250, 130);
    lv_obj_center(panel);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(panel, on_gesture, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_by = lv_label_create(scr);
    lv_label_set_text(lbl_by, "Designed by");
    lv_obj_set_style_text_color(lbl_by, lv_color_hex(0x7CAEF6), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_by, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(lbl_by, 0, -44);
    lv_obj_set_align(lbl_by, LV_ALIGN_CENTER);

    lv_obj_t *lbl_name = lv_label_create(scr);
    lv_label_set_text(lbl_name, "Wenyan Kong");
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_pos(lbl_name, 0, -6);
    lv_obj_set_align(lbl_name, LV_ALIGN_CENTER);

    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "Codex Usage Watch");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x70F52A), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(lbl_hint, 0, 42);
    lv_obj_set_align(lbl_hint, LV_ALIGN_CENTER);
}

lv_obj_t **screen_about_get_ptr(void) { return &scr; }
