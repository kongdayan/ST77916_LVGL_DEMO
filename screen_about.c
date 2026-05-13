#include "ui.h"

static lv_obj_t *scr = NULL;

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());
    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_dashboard_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_dashboard_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_image_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_image_init);
}

void screen_about_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_by = lv_label_create(scr);
    lv_label_set_text(lbl_by, "Designed By");
    lv_obj_set_pos(lbl_by, 0, -35);
    lv_obj_set_align(lbl_by, LV_ALIGN_CENTER);

    lv_obj_t *lbl_name = lv_label_create(scr);
    lv_label_set_text(lbl_name, "Wenyan Kong");
    lv_obj_set_pos(lbl_name, 0, 10);
    lv_obj_set_align(lbl_name, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_44, LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t **screen_about_get_ptr(void) { return &scr; }
