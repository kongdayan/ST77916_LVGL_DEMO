#include "ui.h"

static lv_obj_t *scr = NULL;

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());
    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_video_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_video_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_info_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_info_init);
}

void screen_image_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &ui_img_1539399133);
    lv_obj_set_pos(img, -1, 1);
    lv_obj_set_align(img, LV_ALIGN_CENTER);
    lv_obj_add_flag(img, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t **screen_image_get_ptr(void) { return &scr; }
