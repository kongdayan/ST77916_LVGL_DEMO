#include "ui.h"
#include <math.h>

#define CX              180.0f
#define CY              188.0f
#define CUBE_MIN_SCALE   52.0f
#define CUBE_MAX_SCALE  138.0f
#define CUBE_DEFAULT_SCALE ((CUBE_MIN_SCALE + CUBE_MAX_SCALE) * 0.5f)
#define ROT_GAIN          0.012f
#define ZOOM_GAIN         0.75f
#define ROT_DAMPING       0.942f
#define ZOOM_DAMPING      0.885f
#define INERTIA_EPS       0.0008f
#define ZOOM_EPS          0.05f
#define PI_F              3.14159265f
#define NAV_EDGE_PX       46
#define NAV_SWIPE_MIN_PX  54
#define NAV_SWIPE_SLOP_PX 30
#define ZOOM_ARC_R       142.0f
#define ZOOM_ARC_START   315.0f
#define ZOOM_ARC_END     405.0f
#define ZOOM_ARC_HIT_W    26.0f

typedef struct {
    float x;
    float y;
    float z;
} vec3_t;

typedef struct {
    lv_point_t p;
    float z;
} projected_t;

typedef struct {
    int idx;
    float z;
} face_order_t;

static lv_obj_t   *scr = NULL;
static lv_obj_t   *model = NULL;
static lv_obj_t   *zoom_label = NULL;
static lv_timer_t *ticker = NULL;

static float rot_x = -0.48f;
static float rot_y = 0.72f;
static float vel_x = 0.0f;
static float vel_y = 0.0f;
static float cube_scale = CUBE_DEFAULT_SCALE;
static float zoom_vel = 0.0f;

static bool dragging = false;
static bool zooming = false;
static bool nav_candidate = false;
static lv_point_t last_pt;
static lv_point_t press_pt;

static const vec3_t cube_vertices[8] = {
    { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f },
    {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
    { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f },
    {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f },
};

static const uint8_t cube_faces[6][4] = {
    { 0, 1, 2, 3 },
    { 4, 7, 6, 5 },
    { 0, 4, 5, 1 },
    { 3, 2, 6, 7 },
    { 1, 5, 6, 2 },
    { 0, 3, 7, 4 },
};

static const uint32_t face_colors[6] = {
    0x2F80ED, 0xEB5757, 0xF2C94C, 0x27AE60, 0x9B51E0, 0x56CCF2,
};

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool is_nav_zone(const lv_point_t *pt)
{
    return pt->x < NAV_EDGE_PX || pt->x > (360 - NAV_EDGE_PX) ||
           pt->y < NAV_EDGE_PX || pt->y > (360 - NAV_EDGE_PX);
}

static bool is_zoom_zone(const lv_point_t *pt)
{
    float dx = (float)pt->x - CX;
    float dy = (float)pt->y - 180.0f;
    float r = sqrtf(dx * dx + dy * dy);
    float deg = atan2f(dy, dx) * 180.0f / PI_F;
    if (deg < 0.0f) deg += 360.0f;
    if (deg < ZOOM_ARC_START) deg += 360.0f;

    return deg >= (ZOOM_ARC_START - 12.0f) &&
           deg <= (ZOOM_ARC_END + 12.0f) &&
           fabsf(r - ZOOM_ARC_R) <= ZOOM_ARC_HIT_W;
}

static float zoom_scale_from_point(const lv_point_t *pt)
{
    float dx = (float)pt->x - CX;
    float dy = (float)pt->y - 180.0f;
    float deg = atan2f(dy, dx) * 180.0f / PI_F;
    if (deg < 0.0f) deg += 360.0f;
    if (deg < ZOOM_ARC_START) deg += 360.0f;
    float pct = (clampf(deg, ZOOM_ARC_START, ZOOM_ARC_END) - ZOOM_ARC_START) /
                (ZOOM_ARC_END - ZOOM_ARC_START);

    return CUBE_MIN_SCALE + pct * (CUBE_MAX_SCALE - CUBE_MIN_SCALE);
}

static void project_cube(projected_t out[8])
{
    float sx = sinf(rot_x), cx = cosf(rot_x);
    float sy = sinf(rot_y), cy = cosf(rot_y);

    for (int i = 0; i < 8; i++) {
        float x = cube_vertices[i].x;
        float y = cube_vertices[i].y;
        float z = cube_vertices[i].z;

        float y1 = y * cx - z * sx;
        float z1 = y * sx + z * cx;
        float x2 = x * cy + z1 * sy;
        float z2 = -x * sy + z1 * cy;

        float perspective = 3.8f / (3.8f + z2);
        out[i].p.x = (lv_coord_t)(CX + x2 * cube_scale * perspective);
        out[i].p.y = (lv_coord_t)(CY + y1 * cube_scale * perspective);
        out[i].z = z2;
    }
}

static void sort_faces(const projected_t points[8], face_order_t order[6])
{
    for (int i = 0; i < 6; i++) {
        float z = 0.0f;
        for (int j = 0; j < 4; j++) z += points[cube_faces[i][j]].z;
        order[i].idx = i;
        order[i].z = z * 0.25f;
    }

    for (int i = 1; i < 6; i++) {
        face_order_t item = order[i];
        int j = i - 1;
        while (j >= 0 && order[j].z > item.z) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = item;
    }
}

static void draw_zoom_rail(lv_draw_ctx_t *dc)
{
    float pct = (cube_scale - CUBE_MIN_SCALE) / (CUBE_MAX_SCALE - CUBE_MIN_SCALE);

    lv_point_t center = { (lv_coord_t)CX, 180 };

    lv_draw_arc_dsc_t rail;
    lv_draw_arc_dsc_init(&rail);
    rail.color = lv_color_hex(0x2A2D34);
    rail.opa = LV_OPA_70;
    rail.width = 8;
    rail.rounded = 1;
    lv_draw_arc(dc, &rail, &center, (uint16_t)ZOOM_ARC_R,
                (uint16_t)ZOOM_ARC_START, (uint16_t)ZOOM_ARC_END);

    lv_draw_arc_dsc_t fill;
    lv_draw_arc_dsc_init(&fill);
    fill.color = lv_color_hex(0xFFFFFF);
    fill.opa = LV_OPA_90;
    fill.width = 8;
    fill.rounded = 1;
    lv_draw_arc(dc, &fill, &center, (uint16_t)ZOOM_ARC_R,
                (uint16_t)ZOOM_ARC_START,
                (uint16_t)(ZOOM_ARC_START + pct * (ZOOM_ARC_END - ZOOM_ARC_START)));

    float knob_deg = (ZOOM_ARC_START + pct * (ZOOM_ARC_END - ZOOM_ARC_START)) * PI_F / 180.0f;
    lv_coord_t kx = (lv_coord_t)(CX + cosf(knob_deg) * ZOOM_ARC_R);
    lv_coord_t ky = (lv_coord_t)(180.0f + sinf(knob_deg) * ZOOM_ARC_R);

    lv_draw_rect_dsc_t knob;
    lv_draw_rect_dsc_init(&knob);
    knob.radius = LV_RADIUS_CIRCLE;
    knob.bg_color = lv_color_hex(0xFFFFFF);
    knob.bg_opa = LV_OPA_COVER;
    lv_area_t knob_area = { kx - 8, ky - 8, kx + 8, ky + 8 };
    lv_draw_rect(dc, &knob, &knob_area);
}

static void draw_cube(lv_draw_ctx_t *dc)
{
    projected_t points[8];
    face_order_t order[6];
    project_cube(points);
    sort_faces(points, order);

    lv_draw_rect_dsc_t face_dsc;
    lv_draw_rect_dsc_init(&face_dsc);
    face_dsc.bg_opa = LV_OPA_80;

    lv_draw_line_dsc_t edge_dsc;
    lv_draw_line_dsc_init(&edge_dsc);
    edge_dsc.color = lv_color_hex(0xEAF2FF);
    edge_dsc.opa = LV_OPA_90;
    edge_dsc.width = 2;
    edge_dsc.round_start = 1;
    edge_dsc.round_end = 1;

    for (int i = 0; i < 6; i++) {
        int f = order[i].idx;
        lv_point_t poly[4];
        for (int j = 0; j < 4; j++) poly[j] = points[cube_faces[f][j]].p;

        face_dsc.bg_color = lv_color_hex(face_colors[f]);
        lv_draw_polygon(dc, &face_dsc, poly, 4);

        for (int j = 0; j < 4; j++) {
            lv_draw_line(dc, &edge_dsc, &poly[j], &poly[(j + 1) % 4]);
        }
    }
}

static void update_zoom_label(void)
{
    if (!zoom_label) return;
    int pct = (int)(((cube_scale - CUBE_MIN_SCALE) * 100.0f) /
                    (CUBE_MAX_SCALE - CUBE_MIN_SCALE) + 0.5f);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(zoom_label, buf);
}

static void on_draw(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_POST_BEGIN) return;
    lv_draw_ctx_t *dc = lv_event_get_draw_ctx(e);

    draw_zoom_rail(dc);
    draw_cube(dc);
}

static void on_input(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        lv_indev_wait_release(indev);
        if (nav_candidate && dir == LV_DIR_LEFT)
            _ui_screen_change(screen_codex_usage_get_ptr(),
                              LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_codex_usage_init);
        else if (nav_candidate && dir == LV_DIR_RIGHT)
            _ui_screen_change(screen_about_get_ptr(),
                              LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_about_init);
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        zooming = is_zoom_zone(&pt);
        nav_candidate = !zooming && is_nav_zone(&pt);
        dragging = !nav_candidate;
        press_pt = pt;
        last_pt = pt;
        vel_x = 0.0f;
        vel_y = 0.0f;
        zoom_vel = 0.0f;
        return;
    }

    if (code == LV_EVENT_PRESSING && dragging) {
        int dx = pt.x - last_pt.x;
        int dy = pt.y - last_pt.y;

        if (zooming) {
            float next_scale = zoom_scale_from_point(&pt);
            zoom_vel = next_scale - cube_scale;
            cube_scale = next_scale;
            update_zoom_label();
        } else {
            float rx = (float)-dy * ROT_GAIN;
            float ry = (float)dx * ROT_GAIN;
            rot_x += rx;
            rot_y += ry;
            vel_x = rx;
            vel_y = ry;
        }

        last_pt = pt;
        lv_obj_invalidate(model);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (nav_candidate && code == LV_EVENT_RELEASED) {
            int total_dx = pt.x - press_pt.x;
            int total_dy = pt.y - press_pt.y;

            if (abs(total_dx) >= NAV_SWIPE_MIN_PX && abs(total_dy) <= NAV_SWIPE_SLOP_PX) {
                lv_indev_wait_release(indev);
                if (total_dx < 0)
                    _ui_screen_change(screen_codex_usage_get_ptr(),
                                      LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_codex_usage_init);
                else
                    _ui_screen_change(screen_about_get_ptr(),
                                      LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_about_init);
            }
        }

        dragging = false;
        zooming = false;
        nav_candidate = false;
    }
}

static void on_tick(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (!dragging) {
        rot_x += vel_x;
        rot_y += vel_y;
        cube_scale = clampf(cube_scale + zoom_vel, CUBE_MIN_SCALE, CUBE_MAX_SCALE);

        vel_x *= ROT_DAMPING;
        vel_y *= ROT_DAMPING;
        zoom_vel *= ZOOM_DAMPING;

        if (fabsf(vel_x) < INERTIA_EPS) vel_x = 0.0f;
        if (fabsf(vel_y) < INERTIA_EPS) vel_y = 0.0f;
        if (fabsf(zoom_vel) < ZOOM_EPS) zoom_vel = 0.0f;
    }

    if (rot_x > PI_F) rot_x -= 2.0f * PI_F;
    if (rot_x < -PI_F) rot_x += 2.0f * PI_F;
    if (rot_y > PI_F) rot_y -= 2.0f * PI_F;
    if (rot_y < -PI_F) rot_y += 2.0f * PI_F;

    update_zoom_label();
    lv_obj_invalidate(model);
}

static void on_loaded(lv_event_t *e)
{
    LV_UNUSED(e);
    if (!ticker) ticker = lv_timer_create(on_tick, 16, NULL);
}

static void on_unloaded(lv_event_t *e)
{
    LV_UNUSED(e);
    if (ticker) {
        lv_timer_del(ticker);
        ticker = NULL;
    }
}

static void on_reset(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    rot_x = -0.48f;
    rot_y = 0.72f;
    vel_x = 0.0f;
    vel_y = 0.0f;
    cube_scale = CUBE_DEFAULT_SCALE;
    zoom_vel = 0.0f;
    update_zoom_label();
    lv_obj_invalidate(model);
}

void screen_3dmodel_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x10131A), LV_PART_MAIN);
    lv_obj_add_event_cb(scr, on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(scr, on_unloaded, LV_EVENT_SCREEN_UNLOADED, NULL);

    model = lv_obj_create(scr);
    lv_obj_set_size(model, 360, 360);
    lv_obj_set_pos(model, 0, 0);
    lv_obj_add_flag(model, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(model, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(model, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(model, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(model, on_draw, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(model, on_input, LV_EVENT_ALL, NULL);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "3D Model");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_pos(title, 0, 18);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    lv_obj_t *reset_btn = lv_btn_create(scr);
    lv_obj_set_size(reset_btn, 58, 30);
    lv_obj_set_pos(reset_btn, 0, 315);
    lv_obj_set_align(reset_btn, LV_ALIGN_TOP_MID);
    lv_obj_add_event_cb(reset_btn, on_reset, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(reset_btn, on_input, LV_EVENT_ALL, NULL);

    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);

    zoom_label = lv_label_create(scr);
    lv_obj_set_style_text_color(zoom_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(zoom_label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_pos(zoom_label, 292, 40);
    update_zoom_label();
}

lv_obj_t **screen_3dmodel_get_ptr(void) { return &scr; }
