/* Hex-Ball game — three rotating hexagonal rings, one gap each.
 * Touch-drag to spin the ring nearest your finger.
 * Ball passes through a gap → ring breaks and respawns at outer edge.
 * Ball hits a solid side → elastic reflection. */

#include "ui.h"
#include <math.h>

/* ── constants ─────────────────────────────────────────────────────────────── */
#define CX        180.0f       /* screen centre */
#define CY        180.0f
#define SCREEN_R  172.0f       /* usable radius on round display */
#define BALL_R      8.0f
#define SPEED       2.2f
#define NUM_RINGS   3
#define LINE_W      4
#define PIf         3.14159265f
#define SHRINK_SPEED  0.25f      /* px/frame each ring shrinks */
#define MIN_RING_R   28.0f       /* ring auto-breaks below this radius */
#define SPAWN_R     (SCREEN_R + 65.f)  /* fixed outer spawn radius */

static const float      INIT_R[NUM_RINGS]   = { 65.f, 110.f, 155.f };
static const uint32_t   RING_COLOR[NUM_RINGS]= { 0x00BFFF, 0xFF8C00, 0x39FF14 };

/* ── types ──────────────────────────────────────────────────────────────────── */
typedef struct { float x, y, vx, vy; } Ball;

typedef struct {
    float radius, angle;
    int   gap;          /* which side 0-5 is open */
    bool  alive;
    bool  fresh;        /* just respawned — skip break-check this frame */
} Ring;

/* ── state ──────────────────────────────────────────────────────────────────── */
static lv_obj_t   *scr     = NULL;
static lv_obj_t   *game    = NULL;   /* full-screen object, custom draw */
static lv_timer_t *ticker  = NULL;

static Ball  ball;
static Ring  rings[NUM_RINGS];
static int   score = 0;

/* touch drag */
static bool  dragging  = false;
static float drag_ang  = 0.f;
static int   drag_ring = -1;

/* ── geometry helpers ───────────────────────────────────────────────────────── */

static void hex_pt(const Ring *r, int v, float *x, float *y)
{
    float a = r->angle + v * (PIf / 3.f);
    *x = CX + r->radius * cosf(a);
    *y = CY + r->radius * sinf(a);
}

/* squared distance from P to segment AB; closest point → (*ox, *oy) */
static float seg_dsq(float ax, float ay, float bx, float by,
                     float px, float py, float *ox, float *oy)
{
    float dx = bx-ax, dy = by-ay;
    float t  = (dx*dx + dy*dy < 1e-9f) ? 0.f :
               fmaxf(0.f, fminf(1.f, ((px-ax)*dx+(py-ay)*dy)/(dx*dx+dy*dy)));
    *ox = ax + t*dx;  *oy = ay + t*dy;
    float ex = px-*ox, ey = py-*oy;
    return ex*ex + ey*ey;
}

static float vec_len(float x, float y)   { return sqrtf(x*x + y*y); }

/* ring index whose radius is closest to dist, within 35 px */
static int nearest_ring(float dist)
{
    int best = -1; float bd = 35.f;
    for (int i = 0; i < NUM_RINGS; i++) {
        if (!rings[i].alive) continue;
        float d = fabsf(rings[i].radius - dist);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

/* ── physics ────────────────────────────────────────────────────────────────── */

static void respawn(int idx)
{
    rings[idx].radius = SPAWN_R;
    rings[idx].angle  = (float)lv_rand(0, 628) / 100.f;
    rings[idx].gap    = (int)lv_rand(0, 5);
    rings[idx].alive  = true;
    rings[idx].fresh  = true;
}

static void physics_step(void)
{
    /* ── shrink all rings inward ── */
    for (int i = 0; i < NUM_RINGS; i++) {
        if (!rings[i].alive) continue;
        rings[i].radius -= SHRINK_SPEED;
        if (rings[i].radius < MIN_RING_R) {
            score++;
            respawn(i);
        }
    }

    for (int i = 0; i < NUM_RINGS; i++) rings[i].fresh = false;

    float prev_x = ball.x, prev_y = ball.y;
    ball.x += ball.vx;
    ball.y += ball.vy;

    /* ── bounce off circular screen edge ── */
    float bx = ball.x - CX, by = ball.y - CY;
    float bd = vec_len(bx, by);
    if (bd + BALL_R > SCREEN_R && bd > 0.f) {
        float nx = bx/bd, ny = by/bd;
        float dot = ball.vx*nx + ball.vy*ny;
        ball.vx -= 2.f*dot*nx;
        ball.vy -= 2.f*dot*ny;
        float push = bd + BALL_R - SCREEN_R;
        ball.x -= nx*push;  ball.y -= ny*push;
    }

    /* ── bounce off solid hexagon sides ── */
    for (int i = 0; i < NUM_RINGS; i++) {
        if (!rings[i].alive) continue;
        for (int s = 0; s < 6; s++) {
            if (s == rings[i].gap) continue;
            float ax, ay, bxx, byy, cx, cy;
            hex_pt(&rings[i], s,       &ax,  &ay);
            hex_pt(&rings[i], (s+1)%6, &bxx, &byy);
            float d2 = seg_dsq(ax, ay, bxx, byy, ball.x, ball.y, &cx, &cy);
            if (d2 < BALL_R * BALL_R) {
                float nx = ball.x-cx, ny = ball.y-cy;
                float nl = vec_len(nx, ny) + 1e-9f;
                nx /= nl;  ny /= nl;
                float dot = ball.vx*nx + ball.vy*ny;
                if (dot < 0.f) {
                    ball.vx -= 2.f*dot*nx;
                    ball.vy -= 2.f*dot*ny;
                }
                float pen = BALL_R - sqrtf(d2);
                ball.x += nx*pen;  ball.y += ny*pen;
            }
        }
    }

    /* ── break detection: ball crossed ring radius → passed through gap ── */
    float pd = vec_len(prev_x-CX, prev_y-CY);
    float cd = vec_len(ball.x -CX, ball.y -CY);
    for (int i = 0; i < NUM_RINGS; i++) {
        if (!rings[i].alive || rings[i].fresh) continue;
        float r = rings[i].radius;
        if ((pd < r && cd >= r) || (pd > r && cd <= r)) {
            rings[i].alive = false;
            score++;
            respawn(i);
        }
    }
}

/* ── drawing ────────────────────────────────────────────────────────────────── */

static void on_draw(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_POST_BEGIN) return;
    lv_draw_ctx_t *dc = lv_event_get_draw_ctx(e);

    /* hexagon rings */
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.width = LINE_W;  ld.round_start = 1;  ld.round_end = 1;

    for (int i = 0; i < NUM_RINGS; i++) {
        if (!rings[i].alive) continue;
        ld.color = lv_color_hex(RING_COLOR[i]);
        for (int s = 0; s < 6; s++) {
            if (s == rings[i].gap) continue;
            float ax, ay, bx, by;
            hex_pt(&rings[i], s,       &ax, &ay);
            hex_pt(&rings[i], (s+1)%6, &bx, &by);
            lv_point_t p1 = { (lv_coord_t)ax, (lv_coord_t)ay };
            lv_point_t p2 = { (lv_coord_t)bx, (lv_coord_t)by };
            lv_draw_line(dc, &ld, &p1, &p2);
        }
    }

    /* ball */
    lv_draw_rect_dsc_t rd;
    lv_draw_rect_dsc_init(&rd);
    rd.radius = LV_RADIUS_CIRCLE;
    rd.bg_color = lv_color_white();
    rd.bg_opa   = LV_OPA_COVER;
    lv_area_t ba = {
        (lv_coord_t)(ball.x - BALL_R), (lv_coord_t)(ball.y - BALL_R),
        (lv_coord_t)(ball.x + BALL_R), (lv_coord_t)(ball.y + BALL_R),
    };
    lv_draw_rect(dc, &rd, &ba);

    /* score */
    char buf[24];
    lv_snprintf(buf, sizeof(buf), "Score: %d", score);
    lv_draw_label_dsc_t lbl;
    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.font  = LV_FONT_DEFAULT;
    lv_area_t la = { 10, 8, 170, 26 };
    lv_draw_label(dc, &lbl, &la, buf, NULL);
}

/* ── input ──────────────────────────────────────────────────────────────────── */

static void on_input(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    /* ── navigation via swipe ── */
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        lv_indev_wait_release(lv_indev_get_act());
        if (dir == LV_DIR_LEFT)
            _ui_screen_change(screen_dashboard_get_ptr(),
                              LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, screen_dashboard_init);
        else if (dir == LV_DIR_RIGHT)
            _ui_screen_change(screen_about_get_ptr(),
                              LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, screen_about_init);
        return;
    }

    /* ── ring rotation via drag ── */
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    float tx = (float)pt.x, ty = (float)pt.y;
    float ang = atan2f(ty - CY, tx - CX);

    if (code == LV_EVENT_PRESSED) {
        dragging  = true;
        drag_ang  = ang;
        drag_ring = nearest_ring(vec_len(tx - CX, ty - CY));
    } else if (code == LV_EVENT_PRESSING && dragging && drag_ring >= 0) {
        float delta = ang - drag_ang;
        if (delta >  PIf) delta -= 2.f * PIf;
        if (delta < -PIf) delta += 2.f * PIf;
        rings[drag_ring].angle += delta;
        drag_ang = ang;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        dragging  = false;
        drag_ring = -1;
    }
}

/* ── timer & lifecycle ──────────────────────────────────────────────────────── */

static void on_tick(lv_timer_t *t)
{
    LV_UNUSED(t);
    physics_step();
    lv_obj_invalidate(game);
}

static void on_loaded(lv_event_t *e)
{
    LV_UNUSED(e);
    if (!ticker) ticker = lv_timer_create(on_tick, 16, NULL);
}

static void on_unloaded(lv_event_t *e)
{
    LV_UNUSED(e);
    if (ticker) { lv_timer_del(ticker); ticker = NULL; }
}

/* ── public API ─────────────────────────────────────────────────────────────── */

static void game_reset(void)
{
    ball.x  = CX;  ball.y  = CY;
    ball.vx = SPEED * cosf(PIf / 6.f);
    ball.vy = SPEED * sinf(PIf / 6.f);
    score   = 0;
    for (int i = 0; i < NUM_RINGS; i++) {
        rings[i].radius = INIT_R[i];
        rings[i].angle  = i * (PIf / 9.f);
        rings[i].gap    = i * 2;          /* gaps on different sides */
        rings[i].alive  = true;
        rings[i].fresh  = false;
    }
}

void screen_agent_init(void)
{
    scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_add_event_cb(scr, on_loaded,   LV_EVENT_SCREEN_LOADED,   NULL);
    lv_obj_add_event_cb(scr, on_unloaded, LV_EVENT_SCREEN_UNLOADED, NULL);

    /* full-screen transparent object: draws game + handles input */
    game = lv_obj_create(scr);
    lv_obj_set_size(game, 360, 360);
    lv_obj_set_pos(game, 0, 0);
    lv_obj_clear_flag(game, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(game, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(game, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(game, on_draw,  LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(game, on_input, LV_EVENT_ALL, NULL);

    game_reset();
}

lv_obj_t **screen_agent_get_ptr(void) { return &scr; }
