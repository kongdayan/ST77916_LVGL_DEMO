#include "ui.h"

#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH must be 16"
#endif
#if LV_COLOR_16_SWAP != 1
    #error "LV_COLOR_16_SWAP must be 1"
#endif

void ui_init(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(
        disp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        true,
        LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);

    screen_dashboard_init();
    lv_disp_load_scr(*screen_dashboard_get_ptr());
}
