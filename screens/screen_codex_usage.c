#include "ui.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CURRENT_PCT   27
#define WEEKLY_PCT    73
#define BATTERY_PCT  100

#define CX 180
#define CY 180
#define PI_F 3.14159265f
#define PROGRESS_DOTS 25
#define PROGRESS_SPACING 11

static lv_obj_t *scr = NULL;
static lv_obj_t *panel = NULL;
static bool light_mode = false;

static uint32_t color_bg(void) { return light_mode ? 0xF8FBFF : 0x02050A; }
static uint32_t color_ring(void) { return light_mode ? 0xD8E8FA : 0x0B254D; }
static uint32_t color_tick(void) { return light_mode ? 0x7CAEF6 : 0x1F7AFF; }
static uint32_t color_tick_hot(void) { return light_mode ? 0x1467F2 : 0x1684FF; }
static uint32_t color_text(void) { return light_mode ? 0x061B4D : 0xFFFFFF; }
static uint32_t color_reset(void) { return light_mode ? 0x16264C : 0xFFFFFF; }
static uint32_t color_blue(void) { return light_mode ? 0x075FF0 : 0x1E9BFF; }
static uint32_t color_blue_dim(void) { return light_mode ? 0xD8E7F8 : 0x17446D; }
static uint32_t color_green(void) { return light_mode ? 0x2BA70E : 0x70F52A; }
static uint32_t color_green_dim(void) { return light_mode ? 0xDCEED5 : 0x315C43; }

static void on_gesture(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_indev_wait_release(lv_indev_get_act());

    if (dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM) {
        light_mode = !light_mode;
        if (panel) lv_obj_invalidate(panel);
        return;
    }

    if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT)
        _ui_screen_change(screen_about_get_ptr(),
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, screen_about_init);
}

static void draw_dot(lv_draw_ctx_t *dc, int x, int y, int r, uint32_t color, lv_opa_t opa)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = opa;
    lv_area_t area = { x - r, y - r, x + r, y + r };
    lv_draw_rect(dc, &dsc, &area);
}

static void draw_line(lv_draw_ctx_t *dc, int x1, int y1, int x2, int y2, int w,
                      uint32_t color, lv_opa_t opa)
{
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_hex(color);
    dsc.opa = opa;
    dsc.width = w;
    dsc.round_start = 1;
    dsc.round_end = 1;
    lv_point_t p1 = { x1, y1 };
    lv_point_t p2 = { x2, y2 };
    lv_draw_line(dc, &dsc, &p1, &p2);
}

static void draw_rect(lv_draw_ctx_t *dc, int x1, int y1, int x2, int y2,
                      int radius, uint32_t color, lv_opa_t opa)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = radius;
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = opa;
    lv_area_t area = { x1, y1, x2, y2 };
    lv_draw_rect(dc, &dsc, &area);
}

static const uint8_t *glyph_for(char c)
{
    static const uint8_t space[7] = { 0, 0, 0, 0, 0, 0, 0 };
    static const uint8_t pct[7] = { 0x19, 0x1A, 0x04, 0x08, 0x13, 0x13, 0x00 };
    static const uint8_t colon[7] = { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 };
    static const uint8_t gt[7] = { 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10 };
    static const uint8_t under[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };
    static const uint8_t glyphs[][7] = {
        { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, /* 0 */
        { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* 1 */
        { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }, /* 2 */
        { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E }, /* 3 */
        { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, /* 4 */
        { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, /* 5 */
        { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }, /* 6 */
        { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, /* 7 */
        { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, /* 8 */
        { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }, /* 9 */
        { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* A */
        { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }, /* B */
        { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }, /* C */
        { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }, /* D */
        { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }, /* E */
        { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }, /* F */
        { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }, /* G */
        { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* H */
        { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* I */
        { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C }, /* J */
        { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, /* K */
        { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }, /* L */
        { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }, /* M */
        { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }, /* N */
        { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* O */
        { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }, /* P */
        { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }, /* Q */
        { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }, /* R */
        { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }, /* S */
        { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, /* T */
        { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* U */
        { 0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04 }, /* V */
        { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 }, /* W */
        { 0x11, 0x0A, 0x04, 0x04, 0x04, 0x0A, 0x11 }, /* X */
        { 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x04 }, /* Y */
        { 0x1F, 0x02, 0x04, 0x04, 0x08, 0x10, 0x1F }, /* Z */
    };

    if (c == ' ') return space;
    if (c == '%') return pct;
    if (c == ':') return colon;
    if (c == '>') return gt;
    if (c == '_') return under;
    if (c >= '0' && c <= '9') return glyphs[c - '0'];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return glyphs[10 + c - 'A'];
    return space;
}

static void draw_dot_text(lv_draw_ctx_t *dc, const char *text, int x, int y,
                          int step, int r, uint32_t color, lv_opa_t opa)
{
    while (*text) {
        const uint8_t *g = glyph_for(*text++);
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (g[row] & (1 << (4 - col))) {
                    draw_dot(dc, x + col * step, y + row * step, r, color, opa);
                }
            }
        }
        x += 6 * step;
    }
}

static int dot_text_width(const char *text, int step)
{
    int len = (int)strlen(text);
    return len > 0 ? len * 6 * step - step : 0;
}

static void draw_right_dot_text(lv_draw_ctx_t *dc, const char *text, int right_x, int y,
                                int step, int r, uint32_t color, lv_opa_t opa)
{
    draw_dot_text(dc, text, right_x - dot_text_width(text, step), y, step, r, color, opa);
}

static void draw_right_percent(lv_draw_ctx_t *dc, int pct, int right_x, int y,
                               uint32_t color, lv_opa_t opa)
{
    char digits[5];
    snprintf(digits, sizeof(digits), "%d", pct);

    int digit_step = 4;
    int pct_step = 3;
    int gap = 4;
    int digits_w = dot_text_width(digits, digit_step);
    int pct_w = dot_text_width("%", pct_step);
    int x = right_x - digits_w - gap - pct_w;

    draw_dot_text(dc, digits, x, y, digit_step, 1, color, opa);
    draw_dot_text(dc, "%", x + digits_w + gap, y + 5, pct_step, 1, color, opa);
}

static void draw_centered_dot_text(lv_draw_ctx_t *dc, const char *text, int y,
                                   int step, int r, uint32_t color, lv_opa_t opa)
{
    int w = dot_text_width(text, step);
    draw_dot_text(dc, text, (360 - w) / 2, y, step, r, color, opa);
}

static void draw_dotted_hline(lv_draw_ctx_t *dc, int x1, int x2, int y, uint32_t color, lv_opa_t opa)
{
    for (int x = x1; x <= x2; x += 10) draw_dot(dc, x, y, 1, color, opa);
}

static void draw_progress_dots(lv_draw_ctx_t *dc, int x, int y, int count, int active,
                               int rows, uint32_t active_color, uint32_t inactive_color)
{
    for (int row = 0; row < rows; row++) {
        for (int i = 0; i < count; i++) {
            uint32_t color = i < active ? active_color : inactive_color;
            lv_opa_t opa = i < active ? LV_OPA_COVER : LV_OPA_60;
            draw_dot(dc, x + i * PROGRESS_SPACING, y + row * PROGRESS_SPACING, 2, color, opa);
        }
    }
}

static void draw_outer_ticks(lv_draw_ctx_t *dc)
{
    for (int i = 0; i < 148; i++) {
        float deg = (float)i * 360.0f / 148.0f;
        float rad = deg * PI_F / 180.0f;
        int x = CX + (int)(171.0f * cosf(rad));
        int y = CY + (int)(171.0f * sinf(rad));
        draw_dot(dc, x, y, 1, color_tick(), light_mode ? LV_OPA_50 : LV_OPA_70);
    }

    draw_line(dc, 178, 5, 182, 5, 5, color_tick_hot(), LV_OPA_COVER);
    draw_line(dc, 178, 355, 182, 355, 5, color_tick_hot(), LV_OPA_COVER);
    draw_line(dc, 5, 180, 9, 180, 5, color_tick_hot(), LV_OPA_COVER);
    draw_line(dc, 351, 180, 355, 180, 5, color_tick_hot(), LV_OPA_COVER);
}

static void draw_label_text(lv_draw_ctx_t *dc, const char *text, int x, int y,
                            int w, int h, const lv_font_t *font, uint32_t color)
{
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = lv_color_hex(color);
    dsc.opa = LV_OPA_COVER;
    dsc.font = font;
    dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t area = { x, y, x + w - 1, y + h - 1 };
    lv_draw_label(dc, &dsc, &area, text, NULL);
}

static void draw_clock_icon(lv_draw_ctx_t *dc, int x, int y)
{
    lv_point_t center = { x, y };
    lv_draw_arc_dsc_t dsc;
    lv_draw_arc_dsc_init(&dsc);
    dsc.color = lv_color_hex(color_reset());
    dsc.opa = LV_OPA_COVER;
    dsc.width = 2;
    dsc.rounded = 1;
    lv_draw_arc(dc, &dsc, &center, 7, 0, 360);
    draw_line(dc, x, y, x, y - 5, 2, color_reset(), LV_OPA_COVER);
    draw_line(dc, x, y, x + 4, y + 2, 2, color_reset(), LV_OPA_COVER);
}

static void draw_centered_reset(lv_draw_ctx_t *dc, const char *text, int y)
{
    draw_clock_icon(dc, 58, y + 9);

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = lv_color_hex(color_reset());
    dsc.opa = LV_OPA_COVER;
    dsc.font = &lv_font_montserrat_16;
    dsc.align = LV_TEXT_ALIGN_LEFT;
    lv_area_t area = { 73, y, 315, y + 20 };
    lv_draw_label(dc, &dsc, &area, text, NULL);
}

static void draw_codex_mark(lv_draw_ctx_t *dc)
{
    lv_point_t hex[6];
    for (int i = 0; i < 6; i++) {
        float rad = (30.0f + i * 60.0f) * PI_F / 180.0f;
        hex[i].x = 102 + (lv_coord_t)(20.0f * cosf(rad));
        hex[i].y = 65 + (lv_coord_t)(20.0f * sinf(rad));
    }

    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.bg_color = lv_color_hex(0x0B6CFF);
    fill.bg_opa = LV_OPA_COVER;
    lv_draw_polygon(dc, &fill, hex, 6);

    draw_dot(dc, 94, 57, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 98, 61, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 102, 65, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 98, 69, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 94, 73, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 110, 73, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 114, 73, 2, 0xFFFFFF, LV_OPA_COVER);
    draw_dot(dc, 118, 73, 2, 0xFFFFFF, LV_OPA_COVER);
}

static void draw_battery(lv_draw_ctx_t *dc)
{
    draw_line(dc, 260, 66, 278, 66, 1, 0x1F7AFF, LV_OPA_COVER);
    draw_line(dc, 260, 79, 278, 79, 1, 0x1F7AFF, LV_OPA_COVER);
    draw_line(dc, 260, 66, 260, 79, 1, 0x1F7AFF, LV_OPA_COVER);
    draw_line(dc, 278, 66, 278, 79, 1, 0x1F7AFF, LV_OPA_COVER);
    draw_line(dc, 280, 70, 283, 70, 1, 0x1F7AFF, LV_OPA_COVER);
    draw_line(dc, 280, 75, 283, 75, 1, 0x1F7AFF, LV_OPA_COVER);

    int fill = (BATTERY_PCT + 33) / 34;
    for (int i = 0; i < fill; i++) {
        draw_rect(dc, 263 + i * 5, 69, 266 + i * 5, 76, 1, 0x70F52A, LV_OPA_COVER);
    }

    char buf[6];
    snprintf(buf, sizeof(buf), "%d%%", BATTERY_PCT);
    draw_label_text(dc, buf, 256, 84, 34, 12, &lv_font_montserrat_10, 0xFFFFFF);
}

static void on_draw(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_POST_BEGIN) return;
    lv_draw_ctx_t *dc = lv_event_get_draw_ctx(e);

    draw_rect(dc, 0, 0, 359, 359, LV_RADIUS_CIRCLE, color_bg(), LV_OPA_COVER);

    lv_point_t center = { CX, CY };
    lv_draw_arc_dsc_t ring;
    lv_draw_arc_dsc_init(&ring);
    ring.color = lv_color_hex(color_ring());
    ring.opa = LV_OPA_COVER;
    ring.width = 7;
    ring.rounded = 1;
    lv_draw_arc(dc, &ring, &center, 180, 0, 360);

    draw_outer_ticks(dc);
    draw_dotted_hline(dc, 40, 320, 102, color_tick(), light_mode ? LV_OPA_40 : LV_OPA_70);
    draw_dotted_hline(dc, 28, 332, 203, color_tick(), light_mode ? LV_OPA_40 : LV_OPA_70);
    draw_dotted_hline(dc, 82, 292, 292, color_tick(), light_mode ? LV_OPA_40 : LV_OPA_70);

    draw_codex_mark(dc);
    draw_dot_text(dc, "CODEX", 146, 55, 4, 1, color_text(), LV_OPA_COVER);

    draw_dot_text(dc, "CURRENT", 45, 122, 2, 1, color_blue(), LV_OPA_COVER);
    draw_right_percent(dc, CURRENT_PCT, 319, 116, color_blue(), LV_OPA_COVER);
    draw_progress_dots(dc, 48, 156, PROGRESS_DOTS,
                       (PROGRESS_DOTS * CURRENT_PCT + 50) / 100, 2,
                       color_blue(), color_blue_dim());
    draw_centered_reset(dc, "Resets in 21:59", 180);

    draw_dot_text(dc, "WEEKLY", 45, 221, 2, 1, color_green(), LV_OPA_COVER);
    draw_right_percent(dc, WEEKLY_PCT, 319, 217, color_green(), LV_OPA_COVER);
    draw_progress_dots(dc, 48, 248, PROGRESS_DOTS,
                       (PROGRESS_DOTS * WEEKLY_PCT + 50) / 100, 1,
                       color_green(), color_green_dim());
    draw_centered_reset(dc, "Resets 16:14 on 18 May", 266);

    draw_dot(dc, 100, 307, 3, color_green(), LV_OPA_COVER);
    draw_dot_text(dc, "AGENT ACTIVE", 115, 300, 2, 1, color_green(), LV_OPA_COVER);
}

void screen_codex_usage_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(color_bg()), LV_PART_MAIN);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_ALL, NULL);

    panel = lv_obj_create(scr);
    lv_obj_set_size(panel, 360, 360);
    lv_obj_set_pos(panel, 0, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(panel, on_draw, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(panel, on_gesture, LV_EVENT_ALL, NULL);
}

lv_obj_t **screen_codex_usage_get_ptr(void) { return &scr; }
