
#define MAP_WIDTH   (50)
#define MAP_HEIGHT  (40)
#define TILE_SIZE   (8)
#define VIEW_WIDTH  (CANVAS_WIDTH / TILE_SIZE)
#define VIEW_HEIGHT (CANVAS_HEIGHT / TILE_SIZE)
#define UNIT_COUNT  (256)

#define SPRITE(x, y) (((y) << 16) | (x))
#define SPRITE_X(type) ((type) & 0x00ff)
#define SPRITE_Y(type) ((type) >> 16)
#define CELL(x, y) game.map.cells[(y) * MAP_WIDTH + (x)]
#define CURSOR_CELL game.map.cells[(game.cursor_y) * MAP_WIDTH + (game.cursor_x)]

static int SPRITE_GRASS_1         = SPRITE(0, 0);
static int SPRITE_GRASS_2         = SPRITE(1, 0);
static int SPRITE_SELECTION       = SPRITE(0, 1);

/*
 *    1
 *  8 x 2
 *    4
 */
static int WALL_SPRITES[16] = {
    SPRITE(7, 0),      //  0 - center column
    SPRITE(4, 2),     //  1 - top start
    SPRITE(3, 2),     //  2 - right start
    SPRITE(2, 1),     //  3 - bottom left corner
    SPRITE(5, 2),     //  4 - bottom start
    SPRITE(4, 1),     //  5 - top bottom straight
    SPRITE(2, 0),     //  6 - top left corner
    SPRITE(6, 1),    //  7 - top bottom right T
    SPRITE(2, 2),     //  8 - left start
    SPRITE(3, 1),     //  9 - bottom right cornder
    SPRITE(4, 0),     // 10 - left right straight
    SPRITE(6, 0),    // 11 - left right top T
    SPRITE(3, 0),     // 12 - top right cornder
    SPRITE(5, 1),    // 13 - top bottom left T
    SPRITE(5, 0),    // 14 - left right bottom T
    SPRITE(7, 1)      // 15 - center piece
};

typedef struct Unit {
    struct Unit * free;
} Unit;

typedef struct Cell {
    int type;
    bool has_wall;
    int wall_sprite;
    struct Unit * unit;
} Cell;

typedef struct {
    Cell cells[MAP_WIDTH * MAP_HEIGHT];
} Map;

typedef struct {
    Map map;
    Unit units[UNIT_COUNT];
    Unit * free_unit;

    Bitmap tilesheet;

    Font font;

    int offset_x;
    int offset_y;

    int cursor_x;
    int cursor_y;
} Game;

static Game game = {0};


static Cell * cell_at(int x, int y)
{
    return &game.map.cells[y * MAP_WIDTH + x];
}

static int get_wall_count(int x, int y)
{
    int count = 0;
    if (y - 1 >= 0)             count += CELL(x, y - 1).has_wall ? 1 : 0;
    if (x + 1 < MAP_WIDTH)      count += CELL(x + 1, y).has_wall ? 2 : 0;
    if (y + 1 < MAP_HEIGHT)     count += CELL(x, y + 1).has_wall ? 4 : 0;
    if (x - 1 >= 0)             count += CELL(x - 1, y).has_wall ? 8 : 0;
    return count;
}

static void update_wall_sprites(int x, int y)
{
    CELL(x, y).wall_sprite = get_wall_count(x, y);

    if (y - 1 >= 0) CELL(x, y - 1).wall_sprite = get_wall_count(x, y - 1);
    if (x - 1 >= 0) CELL(x - 1, y).wall_sprite = get_wall_count(x - 1, y);
    if (y + 1 < MAP_HEIGHT) CELL(x, y + 1).wall_sprite = get_wall_count(x, y + 1);
    if (x + 1 < MAP_WIDTH)  CELL(x + 1, y).wall_sprite = get_wall_count(x + 1, y);
}

void init_game()
{
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i)
    {
        Cell * cell = &game.map.cells[i];
        cell->type = rand() % 5 == 1 ? SPRITE_GRASS_1 : SPRITE_GRASS_2;
        cell->unit = NULL;
        cell->has_wall = false;
    }

    for (int i = 0; i < UNIT_COUNT; ++i)
    {
        Unit * unit = &game.units[i];

        if (i < UNIT_COUNT - 1)
            unit->free = &game.units[i + 1];
        else
            unit->free = NULL;
    }

    //CELL(10, 10).type = WALL_SPRITES[0];
    //CELL(11, 10).type = WALL_SPRITES[3];

    game.free_unit = &game.units[0];

    game.cursor_x = VIEW_WIDTH / 2;
    game.cursor_y = VIEW_HEIGHT / 2;
    game.offset_x = 0;
    game.offset_y = 0;

    printf("%d, %d\n", SPRITE_X(SPRITE_GRASS_2), SPRITE_Y(SPRITE_GRASS_2));
}

void draw_sprite(int x, int y, int type)
{
    Rect rect = rect_make_size(SPRITE_X(type) * TILE_SIZE, SPRITE_Y(type) * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    bitmap_draw(x * TILE_SIZE, y * TILE_SIZE, 0, 0, &game.tilesheet, &rect, 0, 0);
}

void draw_game()
{
    for (int y = 0; y < VIEW_HEIGHT; ++y)
        for (int x = 0; x < VIEW_WIDTH; ++x)
        {
            Cell cell = CELL(game.offset_x + x, game.offset_y + y);
            draw_sprite(x, y, cell.type);

            if (cell.has_wall)
                draw_sprite(x, y, WALL_SPRITES[cell.wall_sprite]);
        }

    draw_sprite(game.cursor_x - game.offset_x, game.cursor_y - game.offset_y, SPRITE_SELECTION);

    char buff[256];
    snprintf(buff, 255, "%dx%d\n%dx%d", game.cursor_x, game.cursor_y, game.offset_x, game.offset_y);
    text_draw(0, 10, buff, 2);
}

static void step_cursor()
{
    if (key_pressed(KEY_LEFT))
        game.cursor_x = clamp(game.cursor_x - 1, 0, MAP_WIDTH - 1);

    if (key_pressed(KEY_RIGHT))
        game.cursor_x = clamp(game.cursor_x + 1, 0, MAP_WIDTH - 1);

    if (key_pressed(KEY_UP))
        game.cursor_y = clamp(game.cursor_y - 1, 0, MAP_HEIGHT - 1);

    if (key_pressed(KEY_DOWN))
        game.cursor_y = clamp(game.cursor_y + 1, 0, MAP_HEIGHT - 1);

    if (game.cursor_x < game.offset_x)
        game.offset_x -= 1;
    if (game.cursor_x > (game.offset_x + VIEW_WIDTH - 1))
        game.offset_x += 1;

    if (game.cursor_y < game.offset_y)
        game.offset_y -= 1;
    if (game.cursor_y > (game.offset_y + VIEW_HEIGHT - 1))
        game.offset_y += 1;
}

void step_game()
{
    step_cursor();

    if (key_pressed(KEY_SPACE))
    {
        CURSOR_CELL.has_wall = !CURSOR_CELL.has_wall;
        update_wall_sprites(game.cursor_x, game.cursor_y);
    }
}


