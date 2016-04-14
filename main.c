#include "punity.c"
#include "game.c"
#include "command.c"
#include "ai.c"
#include "astar.c"
#include "lib/ini.c"
#include "lib/index_priority_queue.c"

void init()
{
    log_info("Loading game\n");

    CORE->palette.colors[COLOR_TRANSPARENT] = color_make(0x00, 0x00, 0x00, 0x00);
    CORE->palette.colors[COLOR_BLACK] = color_make(0x00, 0x00, 0x00, 0xff);
    CORE->palette.colors[COLOR_WHITE] = color_make(0xff, 0xff, 0xff, 0xff);
    CORE->palette.colors[COLOR_LIGHT_GRAY] = color_make(0xdb, 0xdb, 0xdb, 0xff);
    CORE->palette.colors[COLOR_DARK_GRAY] = color_make(0xcc, 0xcc, 0xcc, 0xff);
    CORE->palette.colors[COLOR_GREEN] = color_make(0x8d, 0xc4, 0x35, 0xff);

    // Player colors
    CORE->palette.colors[COLOR_PLAYER_1] = color_make(0xff, 0x6a, 0x00, 0xff);
    CORE->palette.colors[COLOR_PLAYER_2] = color_make(0xff, 0xd8, 0x00, 0xff);
    CORE->palette.colors[COLOR_PLAYER_3] = color_make(0x00, 0x94, 0xff, 0xff);
    CORE->palette.colors[COLOR_PLAYER_4] = color_make(0xff, 0x00, 0x6e, 0xff);

    CORE->palette.colors_count = COLOR_COUNT;

    canvas_clear(1);

	bitmap_load_resource(&RES.tilesheet, "tilesheet.png");
    bitmap_load_resource(&RES.font.bitmap, "font.png");

    RES.font.char_width = 4;
    RES.font.char_height = 7;

    CORE->font = &RES.font;

    new_game("menu.map", 1, 1);
}

void step()
{
	if (key_pressed(KEY_ESCAPE))
		CORE->running = 0;

    step_game();

    canvas_clear(0);
    draw_game();

    // Draw some performance
    static char buf[256];
    sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
    text_draw(0, 0, buf, 2);
}
