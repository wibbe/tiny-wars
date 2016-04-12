#include "punity.c"
#include "game.c"

#define COLOR_BLACK (1)
#define COLOR_WHITE (2)

void init()
{
	printf("Loading...\n");
    CORE->palette.colors[0] = color_make(0x00, 0x00, 0x00, 0x00);
    CORE->palette.colors[1] = color_make(0x00, 0x00, 0x00, 0xff);
    CORE->palette.colors[2] = color_make(0xff, 0xff, 0xff, 0xff);
    CORE->palette.colors[3] = color_make(0x63, 0x9b, 0xff, 0xff);

    CORE->palette.colors_count = 4;
    canvas_clear(1);

	bitmap_load_resource(&game.tilesheet, "tilesheet.png");
    bitmap_load_resource(&game.font.bitmap, "font.png");

    game.font.char_width = 4;
    game.font.char_height = 7;

    CORE->font = &game.font;

    init_game();
}

void step()
{
	if (key_pressed(KEY_ESCAPE)) {
		CORE->running = 0;
	}

    step_game();

    canvas_clear(0);
    draw_game();

    // Draw some performance
    static char buf[256];
    sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
    text_draw(0, 0, buf, 2);
}
