#include "ui.h"
#include "sd_card.h"

#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

#define VIDEO_PATH SD_CARD_MOUNT_POINT "/video.rgb"
#define VIDEO_W 360
#define VIDEO_H 360
#define VIDEO_BPP 2
#define VIDEO_FRAME_BYTES (VIDEO_W * VIDEO_H * VIDEO_BPP)
#define VIDEO_FRAME_PERIOD_MS 42

static lv_obj_t *scr = NULL;
static lv_obj_t *img = NULL;
static lv_obj_t *status_label = NULL;
static lv_timer_t *video_timer = NULL;
static FILE *video_file = NULL;
static uint8_t *frame_buf = NULL;
static lv_img_dsc_t frame_dsc;
static uint32_t frame_count = 0;
static bool playback_ready = false;

static void set_status(const char *text)
{
    if (status_label) lv_label_set_text(status_label, text);
}

static void hide_status(void)
{
    if (status_label) lv_label_set_text(status_label, "");
}

static bool read_next_frame(void)
{
    if (!video_file || !frame_buf) return false;

    size_t got = fread(frame_buf, 1, VIDEO_FRAME_BYTES, video_file);
    if (got == VIDEO_FRAME_BYTES) return true;

    fseek(video_file, 0, SEEK_SET);
    got = fread(frame_buf, 1, VIDEO_FRAME_BYTES, video_file);
    return got == VIDEO_FRAME_BYTES;
}

static void close_video(void)
{
    if (video_file) {
        fclose(video_file);
        video_file = NULL;
    }
    playback_ready = false;
}

static bool open_video(void)
{
    close_video();

    if (!sd_card_is_mounted()) {
        set_status("TF card not mounted");
        return false;
    }

    video_file = fopen(VIDEO_PATH, "rb");
    if (!video_file) {
        set_status("Missing /video.rgb");
        return false;
    }

    if (!read_next_frame()) {
        set_status("Invalid video.rgb");
        close_video();
        return false;
    }

    frame_count = 1;
    playback_ready = true;
    hide_status();
    lv_obj_invalidate(img);
    if (video_timer) lv_timer_set_period(video_timer, VIDEO_FRAME_PERIOD_MS);
    return true;
}

static void video_tick(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (!playback_ready) {
        open_video();
        if (!playback_ready) lv_timer_set_period(video_timer, 500);
        return;
    }

    if (read_next_frame()) {
        frame_count++;
        lv_obj_invalidate(img);
    } else {
        set_status("Read error");
        close_video();
        lv_timer_set_period(video_timer, 500);
    }
}

static void on_loaded(lv_event_t *e)
{
    LV_UNUSED(e);
    open_video();
    if (!video_timer) video_timer = lv_timer_create(video_tick, VIDEO_FRAME_PERIOD_MS, NULL);
}

static void on_unloaded(lv_event_t *e)
{
    LV_UNUSED(e);
    if (video_timer) {
        lv_timer_del(video_timer);
        video_timer = NULL;
    }
    close_video();
}

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());

    if (dir == LV_DIR_LEFT)
        _ui_screen_change(screen_about_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_about_init);
    else if (dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_image_get_ptr(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_image_init);
}

void screen_video_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_add_event_cb(scr, on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(scr, on_unloaded, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    if (!frame_buf) {
        frame_buf = (uint8_t *)heap_caps_malloc(VIDEO_FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!frame_buf) {
            frame_buf = (uint8_t *)heap_caps_malloc(VIDEO_FRAME_BYTES, MALLOC_CAP_8BIT);
        }
    }

    memset(&frame_dsc, 0, sizeof(frame_dsc));
    frame_dsc.header.always_zero = 0;
    frame_dsc.header.w = VIDEO_W;
    frame_dsc.header.h = VIDEO_H;
    frame_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    frame_dsc.data_size = VIDEO_FRAME_BYTES;
    frame_dsc.data = frame_buf;

    img = lv_img_create(scr);
    lv_obj_set_size(img, VIDEO_W, VIDEO_H);
    lv_obj_set_pos(img, 0, 0);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(img, on_gesture, LV_EVENT_ALL, NULL);

    if (frame_buf) {
        memset(frame_buf, 0, VIDEO_FRAME_BYTES);
        lv_img_set_src(img, &frame_dsc);
    }

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_align(status_label, LV_ALIGN_CENTER);
    lv_label_set_text(status_label, frame_buf ? "Loading video..." : "No frame memory");
}

lv_obj_t **screen_video_get_ptr(void) { return &scr; }
