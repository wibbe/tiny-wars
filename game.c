
#define MAP_WIDTH   (80)
#define MAP_HEIGHT  (60)
#define TILE_SIZE   (8)
#define VIEW_WIDTH  (CANVAS_WIDTH / TILE_SIZE)
#define VIEW_HEIGHT (CANVAS_HEIGHT / TILE_SIZE)

#define UNIT_COUNT      (2048)
#define COMMAND_COUNT   (256)

#define SPRITE(x, y) (((y) << 16) | (x))
#define SPRITE_X(type) ((type) & 0x00ff)
#define SPRITE_Y(type) ((type) >> 16)

#define CELL(x, y) (&game.map.cells[(y) * MAP_WIDTH + (x)])

#define CURSOR_CELL (&game.map.cells[(game.cursor_y) * MAP_WIDTH + (game.cursor_x)])
#define CURSOR_POS game.cursor_x, game.cursor_y

#define NULL_UNIT game.units[0]
#define UNIT(id) (id == NO_UNIT ? &NULL_UNIT : &game.units[id])
#define UNIT_POS(x, y) UNIT(CELL(x, y)->unit)

#define HAS_UNIT(x, y) (CELL(x, y)->unit != NO_UNIT)
#define NO_UNIT (0)

static int SPRITE_GRASS_1   = SPRITE(0, 0);
static int SPRITE_GRASS_2   = SPRITE(1, 0);
static int SPRITE_SELECTION = SPRITE(0, 1);
static int SPRITE_FLAG_1    = SPRITE(0, 7);
static int SPRITE_FLAG_2    = SPRITE(1, 7);
static int SPRITE_FLAG_3    = SPRITE(2, 7);
static int SPRITE_FLAG_4    = SPRITE(3, 7);

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


enum UnitType {
    UNIT_TYPE_NONE = 0,
    UNIT_TYPE_PLAYER,
    UNIT_TYPE_WALL,
};

enum CommandType {
    COMMAND_NONE = 0,
    COMMAND_BUILD_WALL,
};

typedef struct Unit {
    int type;
    int sprite;
    int x;
    int y;
    int next_free;
} Unit;

typedef struct Cell {
    int type;
    int unit;
} Cell;

typedef struct {
    Cell cells[MAP_WIDTH * MAP_HEIGHT];
} Map;

typedef struct Command {
    int type;
    int x;
    int y;
} Command;

typedef struct Player {
    int flag;

    Command commands[COMMAND_COUNT];
    int command_count;
} Player;

typedef struct {
    Map map;

    Unit units[UNIT_COUNT];
    int first_free_unit;

    Player players[4];
    int player_count;
    int current_player;

    Bitmap tilesheet;
    Font font;

    int offset_x;
    int offset_y;

    int cursor_x;
    int cursor_y;
} Game;

static Game game = {0};


static int alloc_unit(int x, int y, int type)
{
    if (game.first_free_unit == NO_UNIT)
        return NO_UNIT;

    if (HAS_UNIT(x, y))
        return NO_UNIT;

    int id = game.first_free_unit;
    Unit * unit = UNIT(id);

    game.first_free_unit = unit->next_free;

    unit->type = type;
    unit->sprite = 0;
    unit->x = x;
    unit->y = y;
    unit->next_free = NO_UNIT;

    CELL(x, y)->unit = id;

    return id;
}

static void free_unit(int id)
{
    Unit * unit = UNIT(id);
    unit->type = UNIT_TYPE_NONE;
    unit->next_free = game.first_free_unit;
    game.first_free_unit = id;

    CELL(unit->x, unit->y)->unit = NO_UNIT;
}

static bool has_unit_type(int x, int y, int type)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
        return false;

    return UNIT_POS(x, y)->type == type;
}

static bool has_wall(int x, int y)
{
    return has_unit_type(x, y, UNIT_TYPE_WALL);
}

static int get_wall_count(int x, int y)
{
    int count = 0;

    if (has_wall(x, y - 1)) count += 1;
    if (has_wall(x + 1, y)) count += 2;
    if (has_wall(x, y + 1)) count += 4;
    if (has_wall(x - 1, y)) count += 8;

    return count;
}

static void update_wall_sprites(int x, int y)
{
    if (has_wall(x, y))
        UNIT_POS(x, y)->sprite = WALL_SPRITES[get_wall_count(x, y)];
}

static void build_wall(int x, int y)
{
    int wall = alloc_unit(x, y, UNIT_TYPE_WALL);
    if (wall == NO_UNIT)
        return;

    update_wall_sprites(x, y);
    update_wall_sprites(x + 1, y);
    update_wall_sprites(x - 1, y);
    update_wall_sprites(x, y + 1);
    update_wall_sprites(x, y - 1);
}

static void demolish_wall(int x, int y)
{
    if (!has_wall(x, y))
        return;

    free_unit(CELL(x, y)->unit);

    update_wall_sprites(x + 1, y);
    update_wall_sprites(x - 1, y);
    update_wall_sprites(x, y + 1);
    update_wall_sprites(x, y - 1);
}

void init_game()
{
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i)
    {
        Cell * cell = &game.map.cells[i];
        cell->type = rand() % 2 == 1 ? SPRITE_GRASS_1 : SPRITE_GRASS_2;
        cell->unit = NO_UNIT;
    }

    for (int i = 1; i < UNIT_COUNT; ++i)
    {
        Unit * unit = UNIT(i);
        unit->next_free = i < UNIT_COUNT - 1 ? i + 1 : NO_UNIT;
    }

    // We start counting units on 1, because unit 0 is the null unit.
    game.first_free_unit = 1;
    game.player_count = 2;

    game.players[0].flag = alloc_unit(11, 11, UNIT_TYPE_PLAYER);
    game.players[1].flag = alloc_unit(MAP_WIDTH - 11, MAP_HEIGHT - 11, UNIT_TYPE_PLAYER);

    UNIT(game.players[0].flag)->sprite = SPRITE_FLAG_1;
    UNIT(game.players[1].flag)->sprite = SPRITE_FLAG_2;

    game.cursor_x = VIEW_WIDTH / 2;
    game.cursor_y = VIEW_HEIGHT / 2;
    game.offset_x = 0;
    game.offset_y = 0;
}

void draw_sprite(int x, int y, int sprite)
{
    int sprite_x = SPRITE_X(sprite);
    int sprite_y = SPRITE_Y(sprite);

    if (sprite_y == 7)
    {
        Rect rect = rect_make_size(sprite_x * TILE_SIZE, (sprite_y - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE * 2);
        bitmap_draw(x * TILE_SIZE, (y - 1) * TILE_SIZE, 0, 0, &game.tilesheet, &rect, 0, 0);
    }
    else
    {
        Rect rect = rect_make_size(sprite_x * TILE_SIZE, sprite_y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        bitmap_draw(x * TILE_SIZE, y * TILE_SIZE, 0, 0, &game.tilesheet, &rect, 0, 0);
    }
}

void draw_game()
{
    // Draw map
    for (int y = 0; y < VIEW_HEIGHT; ++y)
        for (int x = 0; x < VIEW_WIDTH; ++x)
        {
            Cell * cell = CELL(game.offset_x + x, game.offset_y + y);
            draw_sprite(x, y, cell->type);
        }

    // Draw units
    for (int i = UNIT_COUNT - 1; i > 0; --i)
    {
        Unit * unit = UNIT(i);
        if (unit->type != UNIT_TYPE_NONE) // Should only draw units that are in the view
            draw_sprite(unit->x - game.offset_x, unit->y - game.offset_y, unit->sprite);
    }

    draw_sprite(game.cursor_x - game.offset_x, game.cursor_y - game.offset_y, SPRITE_SELECTION);

    char buff[256];
    snprintf(buff, 255, "%dx%d - %dx%d", game.cursor_x, game.cursor_y, game.offset_x, game.offset_y);
    text_draw(0, 10, buff, 2);
}

static void step_cursor()
{
    int width = MAP_WIDTH - VIEW_WIDTH;
    int height = MAP_HEIGHT - VIEW_HEIGHT;

    if (key_pressed(KEY_LEFT))
        game.offset_x = clamp(game.offset_x - 1, 0, width);

    if (key_pressed(KEY_RIGHT))
        game.offset_x = clamp(game.offset_x + 1, 0, width);

    if (key_pressed(KEY_UP))
        game.offset_y = clamp(game.offset_y - 1, 0, height);

    if (key_pressed(KEY_DOWN))
        game.offset_y = clamp(game.offset_y + 1, 0, height);

    game.cursor_x = game.offset_x + CORE->mouse_x / TILE_SIZE;
    game.cursor_y = game.offset_y + CORE->mouse_y / TILE_SIZE;
}

void step_game()
{
    step_cursor();

    if (key_down(KEY_LBUTTON) && !has_wall(CURSOR_POS))
    {
        build_wall(CURSOR_POS);
    }

    if (key_down(KEY_RBUTTON) && has_wall(CURSOR_POS))
    {
        demolish_wall(CURSOR_POS);
    }
}
