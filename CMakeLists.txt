SET(SOURCES screens/ui_Home.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_Number.c)

add_library(ui ${SOURCES})
