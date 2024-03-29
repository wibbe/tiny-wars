
#include "game.h"

static const int SPRITE_GRASS_1             = SPRITE(0, 0);
static const int SPRITE_SELECTION           = SPRITE(0, 1);
static const int SPRITE_BUILD_SELECTION     = SPRITE(1, 1);
static const int SPRITE_MOVE_SELECTION      = SPRITE(2, 1);
static const int SPRITE_MOVE_GOAL_MARKER    = SPRITE(3, 1);
static const int SPRITE_MOVE_MARKER         = SPRITE(4, 1);
static const int SPRITE_MOVE_MARKER_INVALID = SPRITE(5, 1);

#define SPRITE_FLAG(player)     SPRITE(player, 7)
#define SPRITE_WARIOR(player)   SPRITE(player, 5)
#define SPRITE_WALL(dir)        SPRITE(dir, 3)
#define SPRITE_FOG_OF_WAR(dir)  SPRITE(dir, 4)
#define SPRITE_HATCH(progress)  SPRITE(progress, 2)

static const Vec OFFSET[8] = {
    {0, 1},
    {1, 1},
    {-1, 1},
    {1, 0},
    {-1, 0},
    {1, -1},
    {-1, -1},
    {0, -1},
};

static const char * COMMAND_NAMES[] = {
    "None",
    "Construct",
    "MoveTo",
};

static const int MAX_HITPOINTS[] = {
    [UNIT_TYPE_PLAYER] = 16,
    [UNIT_TYPE_WARIOR] = 4,
    [UNIT_TYPE_WALL] = 8,
};

Res RES = {0};
Game GAME = {0};


// ##     ## ##    ## #### ########
// ##     ## ###   ##  ##     ##
// ##     ## ####  ##  ##     ##
// ##     ## ## ## ##  ##     ##
// ##     ## ##  ####  ##     ##
// ##     ## ##   ###  ##     ##
//  #######  ##    ## ####    ##


static int alloc_unit(int x, int y, int type, int owner, int hit_points)
{
    if (GAME.first_free_unit == NO_UNIT)
        return NO_UNIT;

    if (HAS_UNIT(x, y))
        return NO_UNIT;

    int id = GAME.first_free_unit;
    Unit * unit = UNIT(id);

    GAME.first_free_unit = unit->next_free;

    memset(unit, 0, sizeof(Unit));

    unit->type = type;
    unit->owner = owner;
    unit->x = x;
    unit->y = y;
    unit->is_ready = false;
    unit->hit_points = hit_points;
    unit->next_free = NO_UNIT;
    unit->moving = false;

    switch (type)
    {
        case UNIT_TYPE_WALL:
            unit->sprite = SPRITE_WALL(0);
            break;

        case UNIT_TYPE_PLAYER:
            ASSERT(owner != -1);
            unit->sprite = SPRITE_FLAG(owner);
            break;

        case UNIT_TYPE_WARIOR:
            ASSERT(owner != -1);
            unit->sprite = SPRITE_WARIOR(owner);
            break;
    }

    CELL(x, y)->unit = id;

    return id;
}

static void free_unit(int id)
{
    Unit * unit = UNIT(id);
    unit->type = UNIT_TYPE_NONE;
    unit->owner = -1;
    unit->next_free = GAME.first_free_unit;
    GAME.first_free_unit = id;

    CELL(unit->x, unit->y)->unit = NO_UNIT;
}

static bool has_unit_type(int x, int y, int type)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
        return false;

    return UNIT_POS(x, y)->type == type;
}

bool is_passable(int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
        return false;

    Cell * cell = CELL(x, y);
    return !cell->blocked && cell->unit == NO_UNIT;
}

bool find_empty(int x, int y, Vec * result)
{
    for (int i = 0; i < 8; ++i)
    {
        if (is_passable(OFFSET[i].x + x, OFFSET[i].y + y))
        {
            if (result != NULL)
                *result = vec_make(OFFSET[i].x + x, OFFSET[i].y + y);

            return true;
        }
    }
    return false;
}

static bool move_unit(int unit_id, int x, int y)
{
    if (is_passable(x, y))
    {
        Unit * unit = UNIT(unit_id);
        CELL(unit->x, unit->y)->unit = NO_UNIT;

        unit->x = x;
        unit->y = y;
        CELL(unit->x, unit->y)->unit = unit_id;

        reveal_fog_of_war(unit->owner, x, y);
        return true;
    }

    return false;
}

void unit_produce(int player_id, int unit_it, int type)
{
    Unit * flag = UNIT(unit_it);
    if (flag->type != UNIT_TYPE_PLAYER)
        return;

    Vec pos;
    if (find_empty(flag->x, flag->y, &pos))
    {
        alloc_unit(pos.x, pos.y, type, player_id, 0);
        command_construct(player_id, unit_it, pos.x, pos.y);
    }
}

static void build_wall(int x, int y)
{
    Cell * cell = CELL(x, y);

    if (cell->unit == NO_UNIT)
    {
        alloc_unit(x, y, UNIT_TYPE_WALL, GAME.local_player, 0);
        update_wall_sprites(x, y);
    }
    else
    {
        Unit * unit = UNIT(cell->unit);
        if (unit->type == UNIT_TYPE_WALL && !unit->is_ready)
        {
            free_unit(cell->unit);
            update_wall_sprites(x, y);
        }
    }
}


// ##     ## ##    ## #### ########    ##     ##  #######  ##     ## ######## ##     ## ######## ##    ## ########
// ##     ## ###   ##  ##     ##       ###   ### ##     ## ##     ## ##       ###   ### ##       ###   ##    ##
// ##     ## ####  ##  ##     ##       #### #### ##     ## ##     ## ##       #### #### ##       ####  ##    ##
// ##     ## ## ## ##  ##     ##       ## ### ## ##     ## ##     ## ######   ## ### ## ######   ## ## ##    ##
// ##     ## ##  ####  ##     ##       ##     ## ##     ##  ##   ##  ##       ##     ## ##       ##  ####    ##
// ##     ## ##   ###  ##     ##       ##     ## ##     ##   ## ##   ##       ##     ## ##       ##   ###    ##
//  #######  ##    ## ####    ##       ##     ##  #######     ###    ######## ##     ## ######## ##    ##    ##

bool in_reach_of_unit(int unit_id, int x, int y)
{
    Unit * unit = UNIT(unit_id);

    int diff_x = abs(unit->x - x);
    int diff_y = abs(unit->y - y);

    return diff_x <= 1 && diff_y <= 1;
}

void unit_move_close_to(int unit_id, int x, int y)
{
    int best_idx = -1;
    int best_length = MAP_WIDTH * MAP_HEIGHT;

    Unit * unit = UNIT(unit_id);

    // Find a suitable build position around the site
    for (int i = 0; i < 8; ++i)
    {
        if (is_passable(x + OFFSET[i].x, y + OFFSET[i].y))
        {
            int length = astar_compute(unit->x, unit->y, x + OFFSET[i].x, y + OFFSET[i].y, unit->move_path, PATH_LENGTH);
            if (length > 0 && length < best_length)
            {
                best_length = length;
                best_idx = i;
            }
        }
    }

    if (best_idx != -1)
    {
        astar_compute(unit->x, unit->y, x + OFFSET[best_idx].x, y + OFFSET[best_idx].y, unit->move_path, PATH_LENGTH);

        unit->moving = true;
        unit->move_target_x = x + OFFSET[best_idx].x;
        unit->move_target_y = y + OFFSET[best_idx].y;
        unit->offset_x = 0;
        unit->offset_y = 0;
    }
}

static bool unit_move_to_next(int unit_id)
{
    Unit * unit = UNIT(unit_id);

    // Stop if we are already there
    if (unit->x == unit->move_target_x && unit->y == unit->move_target_y)
    {
        unit->moving = false;
        return true;
    }

    if (!astar_compute(unit->x, unit->y, unit->move_target_x, unit->move_target_y, unit->move_path, PATH_LENGTH))
    {
        // No solution found
        return true;
    }

    int next_x = unit->move_path[0] % MAP_WIDTH;
    int next_y = unit->move_path[0] / MAP_WIDTH;

    // Initialize movement
    int diff_x = next_x - unit->x;
    int diff_y = next_y - unit->y;

    if (move_unit(unit_id, next_x, next_y))
    {
        unit->offset_x = (diff_x > 0 ? -TILE_SIZE : (diff_x < 0 ? TILE_SIZE : 0));
        unit->offset_y = (diff_y > 0 ? -TILE_SIZE : (diff_y < 0 ? TILE_SIZE : 0));
    }

    return false;
}

bool unit_move_to(bool start, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (start)
    {
        // Just finish directly if we could not find a path forward
        if (!astar_compute(unit->x, unit->y, unit->move_target_x, unit->move_target_y, unit->move_path, PATH_LENGTH))
            return true;

        bool unit_in_view = in_view_of_local_player(unit->x, unit->y) || unit->owner == GAME.local_player;
        bool first_target_in_view = unit->move_path[0] == -1 ? false : in_view_of_local_player(unit->move_path[0] % MAP_WIDTH, unit->move_path[0] / MAP_WIDTH);
        bool second_target_in_view = unit->move_path[1] == -1 ? false : in_view_of_local_player(unit->move_path[1] % MAP_WIDTH, unit->move_path[1] / MAP_WIDTH);
        bool third_target_in_view = unit->move_path[2] == -1 ? false : in_view_of_local_player(unit->move_path[2] % MAP_WIDTH, unit->move_path[2] / MAP_WIDTH);

        // If anything is in view, we need to continue on to the animation stage
        if (unit_in_view || first_target_in_view || second_target_in_view || third_target_in_view)
            return false;

        // Otherwise we just move the player to the correct cells. We need to do both of the moves, so we unveil the fog-of-war correctly
        if (unit->move_path[0] != -1)
            move_unit(unit_id, unit->move_path[0] % MAP_WIDTH, unit->move_path[0] / MAP_WIDTH);
        if (unit->move_path[1] != -1)
            move_unit(unit_id, unit->move_path[1] % MAP_WIDTH, unit->move_path[1] / MAP_WIDTH);
        if (unit->move_path[2] != -1)
            move_unit(unit_id, unit->move_path[2] % MAP_WIDTH, unit->move_path[2] / MAP_WIDTH);

        // Are we done with the move command?
        if (unit->x == unit->move_target_x && unit->y == unit->move_target_y)
            unit->moving = false;
        else
            astar_compute(unit->x, unit->y, unit->move_target_x, unit->move_target_y, unit->move_path, PATH_LENGTH);

        return true;
    }
    else
    {
        // Animate the player
        if (frame % TILE_SIZE == 0 && frame != (UNIT_MOVEMENT_SPEED * TILE_SIZE))
        {
            if (unit_move_to_next(unit_id))
                return true;
        }

        if (frame == (UNIT_MOVEMENT_SPEED * TILE_SIZE))
        {
            astar_compute(unit->x, unit->y, unit->move_target_x, unit->move_target_y, unit->move_path, PATH_LENGTH);

            // Are we at the destination?
            if (unit->x == unit->move_target_x && unit->y == unit->move_target_y)
            {
                unit->moving = false;
                unit->offset_x = 0;
                unit->offset_y = 0;
            }

            return true;
        }
        else
        {
            if (unit->offset_x > 0) unit->offset_x--;
            if (unit->offset_x < 0) unit->offset_x++;
            if (unit->offset_y > 0) unit->offset_y--;
            if (unit->offset_y < 0) unit->offset_y++;
        }
    }

    return false;
}


// ##      ##    ###    ##       ##
// ##  ##  ##   ## ##   ##       ##
// ##  ##  ##  ##   ##  ##       ##
// ##  ##  ## ##     ## ##       ##
// ##  ##  ## ######### ##       ##
// ##  ##  ## ##     ## ##       ##
//  ###  ###  ##     ## ######## ########


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

void update_wall_sprites(int x, int y)
{
    #define UPDATE_WALL(x, y) if (has_wall((x), (y))) UNIT_POS((x), (y))->sprite = SPRITE_WALL(get_wall_count((x), (y)));

    UPDATE_WALL(x, y);
    UPDATE_WALL(x + 1, y);
    UPDATE_WALL(x - 1, y);
    UPDATE_WALL(x, y + 1);
    UPDATE_WALL(x, y - 1);
}


// ########  #######   ######       #######  ########    ##      ##    ###    ########
// ##       ##     ## ##    ##     ##     ## ##          ##  ##  ##   ## ##   ##     ##
// ##       ##     ## ##           ##     ## ##          ##  ##  ##  ##   ##  ##     ##
// ######   ##     ## ##   ####    ##     ## ######      ##  ##  ## ##     ## ########
// ##       ##     ## ##    ##     ##     ## ##          ##  ##  ## ######### ##   ##
// ##       ##     ## ##    ##     ##     ## ##          ##  ##  ## ##     ## ##    ##
// ##        #######   ######       #######  ##           ###  ###  ##     ## ##     ##


static int get_fog_of_war_sprite(bool * fog_of_war, int x, int y)
{
    #define HAS_FOG(x, y) (x) >= 0 && (x) < MAP_WIDTH && (y) >= 0 && (y) < MAP_HEIGHT && fog_of_war[(y) * MAP_WIDTH + (x)]

    int count = 0;

    if (HAS_FOG(x, y - 1)) count += 1;
    if (HAS_FOG(x + 1, y)) count += 2;
    if (HAS_FOG(x, y + 1)) count += 4;
    if (HAS_FOG(x - 1, y)) count += 8;

    if (count == 0)
    {
        if (HAS_FOG(x + 1, y - 1)) count = 16;
        if (HAS_FOG(x + 1, y + 1)) count = 17;
        if (HAS_FOG(x - 1, y + 1)) count = 18;
        if (HAS_FOG(x - 1, y - 1)) count = 19;
    }

    #undef HAS_FOG

    return count == 0 ? -1 : SPRITE_FOG_OF_WAR(count);
}

void reveal_fog_of_war(int player_id, int x, int y)
{
    static const bool area[] = {
        0, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 0,
    };

    Player * player = PLAYER(player_id);
    for (int off_y = -3; off_y < 4; ++off_y)
    {
        int ny = y + off_y;

        for (int off_x = -3; off_x < 4; ++off_x)
        {
            int nx = x + off_x;
            int area_idx = (off_y + 3) * 7 + (off_x + 3);

            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT && area[area_idx])
                player->fog_of_war[ny * MAP_WIDTH + nx] = false;
        }
    }
}


//  ######      ###    ##     ## ########
// ##    ##    ## ##   ###   ### ##
// ##         ##   ##  #### #### ##
// ##   #### ##     ## ## ### ## ######
// ##    ##  ######### ##     ## ##
// ##    ##  ##     ## ##     ## ##
//  ######   ##     ## ##     ## ########


void init_game()
{
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i)
    {
        Cell * cell = &GAME.map.cells[i];
        cell->sprite = NO_SPRITE;
        cell->blocked = false;
        cell->unit = NO_UNIT;
    }

    { // Null unit
        NULL_UNIT->type = UNIT_TYPE_NONE;
        NULL_UNIT->owner = NO_PLAYER;
        NULL_UNIT->moving = false;
    }

    for (int i = 1; i < UNIT_COUNT; ++i)
    {
        Unit * unit = UNIT(i);
        unit->type = UNIT_TYPE_NONE;
        unit->owner = NO_PLAYER;
        unit->is_ready = false;
        unit->hit_points = 0;
        unit->moving = false;
        unit->next_free = i < UNIT_COUNT - 1 ? i + 1 : NO_UNIT;
    }

    for (int i = 0; i < PLAYER_COUNT; ++i)
    {
        Player * player = PLAYER(i);
        player->id = i;
        player->flag = NO_UNIT;
        player->stage_done = false;
        player->ai_controlled = false;

        // Hide every part of the map with fog-of-war
        for (int j = 0; j < MAP_WIDTH * MAP_HEIGHT; ++j)
            player->fog_of_war[j] = true;
    }

    for (int i = 0; i < PLAYER_COUNT; ++i)
    {
        AIBrain * ai = AI(i);
        ai->player = NO_PLAYER;
    }

    // We start counting units on 1, because unit 0 is the null unit.
    GAME.first_free_unit = 1;
    GAME.selected_unit = NO_UNIT;
    GAME.player_count = 0;
    GAME.ai_count = 0;
    GAME.cursor_x = VIEW_WIDTH / 2;
    GAME.cursor_y = VIEW_HEIGHT / 2;
    GAME.offset_x = 0;
    GAME.offset_y = 0;
    GAME.stage = STAGE_ISSUE_COMMAND;
    GAME.stage_initiative_player = 0;

    GAME.ui.next_id = 1;
    GAME.ui.current_id = 0;
}

bool new_game(const char * map_name, int human_players, int ai_players)
{
    void * map_data;
    size_t map_size;

    init_game();

    map_data = resource_get(map_name, &map_size);
    ini_t * map = ini_parse((const char *)map_data, map_size);

    GAME.player_count = human_players + ai_players;
    GAME.ai_count = ai_players;
    GAME.view_player = 0;
    GAME.local_player = 0;

    random_init(&GAME.random, 0);

    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        char buf[3];
        snprintf(buf, 3, "%02d", y + 1);
        const char * row = ini_get(map, "map", buf);
        if (strlen(row) != MAP_WIDTH)
            goto fail;

        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            switch (row[x])
            {
                case 'x':   // wall
                    UNIT(alloc_unit(x, y, UNIT_TYPE_WALL, -1, MAX_HITPOINTS[UNIT_TYPE_WALL]))->is_ready = true;

                case '.':   // grass
                    CELL(x, y)->sprite = SPRITE_GRASS_1;
                    break;

                case '1':   // player 1-4 start
                case '2':
                case '3':
                case '4':
                    {
                        int player = row[x] - '1';
                        CELL(x, y)->sprite = SPRITE_GRASS_1;
                        PLAYER(player)->flag = alloc_unit(x, y, UNIT_TYPE_PLAYER, player, MAX_HITPOINTS[UNIT_TYPE_PLAYER]);
                        UNIT(PLAYER(player)->flag)->is_ready = true;
                        reveal_fog_of_war(player, x, y);
                    }
                    break;

                default:   // default all cells to grass
                    CELL(x, y)->sprite = SPRITE_GRASS_1;
                    break;
            }
        }
    }

    // Create the first unit for the players
    for (int p = 0; p < GAME.player_count; ++p)
    {
        Player * player = PLAYER(p);
        Unit * flag = UNIT(player->flag);

        Vec pos;
        if (find_empty(flag->x, flag->y, &pos))
        {
            int unit = alloc_unit(pos.x, pos.y, UNIT_TYPE_WARIOR, p, MAX_HITPOINTS[UNIT_TYPE_WARIOR]);
            UNIT(unit)->is_ready = true;
            reveal_fog_of_war(p, pos.x, pos.y);
        }
    }

    // Initialize ai players
    for (int i = 0; i < ai_players; ++i)
    {
        AIBrain * ai = AI(i);
        ai->player = human_players + i;

        PLAYER(ai->player)->ai_controlled = true;

        log_info("AI %d controlling player %d\n", i, ai->player);
    }

    // Update wall sprites
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            update_wall_sprites(x, y);

    ini_free(map);
    return true;

fail:
    ini_free(map);
    return false;
}


// ##     ## ####
// ##     ##  ##
// ##     ##  ##
// ##     ##  ##
// ##     ##  ##
// ##     ##  ##
//  #######  ####


void begin_ui()
{
    GAME.ui.next_id = 1;

    GAME.ui.button_state_old = GAME.ui.button_state;
    GAME.ui.button_state = key_down(KEY_LBUTTON);
}

void end_ui()
{
    if (!GAME.ui.button_state)
        GAME.ui.current_id = 0;
}

static bool ui_cursor_inside(int x, int y, int width, int height)
{
    return CORE->mouse_x >= x && CORE->mouse_x <= (x + width) &&
           CORE->mouse_y >= y && CORE->mouse_y <= (y + height);
}

bool ui_button(int x, int y, Rect sprite, int type, bool enabled, bool down)
{
    int id = GAME.ui.next_id++;
    int width;
    Rect button_rect;

    switch (type)
    {
        case UI_BUTTON_TOOLBAR:
            button_rect = rect_make_size(120, 52, 12, 12);
            width = 12;
            break;

        case UI_BUTTON_NEXT:
            button_rect = rect_make_size(132, 52, 25, 12);
            width = 25;
            break;
    }

    bool mouse_inside = ui_cursor_inside(x, y, width, 12);

    // Did we press down on the button?
    if (enabled && mouse_inside && GAME.ui.current_id == 0 && !GAME.ui.button_state_old && GAME.ui.button_state)
        GAME.ui.current_id = id;

    if ((mouse_inside && GAME.ui.current_id == 0) || GAME.ui.current_id == id || down)
    {
        if (GAME.ui.current_id == id || down)
        {
            button_rect.min_y -= 11;
            button_rect.max_y -= 11;
        }

        if (enabled)
            bitmap_draw(x, y, 0, 0, &RES.tilesheet, &button_rect, 0, 0);
    }

    int sprite_x = (width / 2) - (sprite.max_x - sprite.min_x) / 2;
    int sprite_y = 6 - (sprite.max_y - sprite.min_y) / 2;
    bitmap_draw(x + sprite_x, y + sprite_y, 0, 0, &RES.tilesheet, &sprite, 0, 0);

    // Did we click the button?
    if (enabled && mouse_inside && GAME.ui.current_id == id && GAME.ui.button_state_old && !GAME.ui.button_state)
        return true;

    return false;
}


// ##     ## #### ######## ##      ##
// ##     ##  ##  ##       ##  ##  ##
// ##     ##  ##  ##       ##  ##  ##
// ##     ##  ##  ######   ##  ##  ##
//  ##   ##   ##  ##       ##  ##  ##
//   ## ##    ##  ##       ##  ##  ##
//    ###    #### ########  ###  ###


void focus_view_on(int x, int y)
{
    GAME.offset_x = x - (VIEW_WIDTH / 2);
    GAME.offset_y = y - (VIEW_HEIGHT / 2);
}

bool in_view(int x, int y)
{
    return x > GAME.offset_x && x < (GAME.offset_x + VIEW_WIDTH) && y > GAME.offset_y && y < (GAME.offset_y + VIEW_HEIGHT);
}

bool in_view_of_local_player(int x, int y)
{
    if (!in_view(x, y))
        return false;

    int idx = y * MAP_WIDTH + x;
    if (LOCAL_PLAYER->fog_of_war[idx] == false)
        return true;

    return false;
}


// ########  ########     ###    ##      ## #### ##    ##  ######
// ##     ## ##     ##   ## ##   ##  ##  ##  ##  ###   ## ##    ##
// ##     ## ##     ##  ##   ##  ##  ##  ##  ##  ####  ## ##
// ##     ## ########  ##     ## ##  ##  ##  ##  ## ## ## ##   ####
// ##     ## ##   ##   ######### ##  ##  ##  ##  ##  #### ##    ##
// ##     ## ##    ##  ##     ## ##  ##  ##  ##  ##   ### ##    ##
// ########  ##     ## ##     ##  ###  ###  #### ##    ##  ######


Rect rect_from_sprite(int sprite)
{
    int sprite_x = SPRITE_X(sprite);
    int sprite_y = SPRITE_Y(sprite);
    return rect_make_size(sprite_x * TILE_SIZE, sprite_y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void draw_sprite(int x, int y, int offset_x, int offset_y, int sprite)
{
    int sprite_x = SPRITE_X(sprite);
    int sprite_y = SPRITE_Y(sprite);
    int real_x = 0;
    int real_y = 0;
    Rect rect;

    if (sprite_y == 7)
    {
        rect = rect_make_size(sprite_x * TILE_SIZE, (sprite_y - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE * 2);

        real_x = (x - GAME.offset_x) * TILE_SIZE + offset_x;
        real_y = (y - 1 - GAME.offset_y) * TILE_SIZE + offset_y;
    }
    else
    {
        rect = rect_from_sprite(sprite);
        real_x = (x - GAME.offset_x) * TILE_SIZE + offset_x;
        real_y = (y - GAME.offset_y) * TILE_SIZE + offset_y;
    }
    bitmap_draw(real_x, real_y, 0, 0, &RES.tilesheet, &rect, 0, 0);
}

void draw_construct(Unit * unit, int id)
{
    int x = (unit->x - GAME.offset_x) * TILE_SIZE;
    int y = (unit->y - GAME.offset_y) * TILE_SIZE;

    float t = (float)unit->hit_points / MAX_HITPOINTS[unit->type];
    int progress = (int)round(t * 8);

    draw_sprite(unit->x, unit->y, unit->offset_x, unit->offset_y, SPRITE_HATCH(progress));
    rect_draw(rect_make_size(x, y + TILE_SIZE - 1, progress, 1), COLOR_PLAYER_1 + unit->owner);
    //rect_draw(rect_make_size(x + 2 + progress, y + TILE_SIZE - 4, 4 - progress, 2), COLOR_LIGHT_GRAY);
}

void draw_move_to(Unit * unit, int id, bool selected)
{
    if (selected)
    {
        for (int i = 0; i < PATH_LENGTH; ++i)
        {

            int idx = unit->move_path[i];
            if (idx == -1)
                continue;

            int x = idx % MAP_WIDTH;
            int y = idx / MAP_WIDTH;

            draw_sprite(x, y, 0, 0, i < 3 ? SPRITE_MOVE_MARKER : SPRITE_MOVE_MARKER_INVALID);
        }
    }

    draw_sprite(unit->move_target_x, unit->move_target_y, 0, 0, SPRITE_MOVE_GOAL_MARKER);
}

void draw_selected_unit(Unit * unit, int id)
{
    if (unit->moving)
        draw_move_to(unit, id, true);
}

void draw_unit(Unit * unit, int id)
{
    draw_sprite(unit->x, unit->y, unit->offset_x, unit->offset_y, unit->sprite);

    if (unit->owner == GAME.view_player)
    {
        if (unit->moving && unit->command.type == COMMAND_MOVE_TO)
            draw_move_to(unit, id, false);

        if (!unit->is_ready)
            draw_construct(unit, id);

        /*
        switch (unit->command.type)
        {
            case COMMAND_CONSTRUCT:
                break;
        }
        */
    }
}

void draw_game()
{
    begin_ui();

    // Start by making sure the offset is within the map
    GAME.offset_x = clamp(GAME.offset_x, 0, MAP_WIDTH - VIEW_WIDTH);
    GAME.offset_y = clamp(GAME.offset_y, 0, MAP_HEIGHT - VIEW_HEIGHT);

    // Draw map
    for (int y = 0; y < VIEW_HEIGHT; ++y)
        for (int x = 0; x < VIEW_WIDTH; ++x)
        {
            Cell * cell = CELL(GAME.offset_x + x, GAME.offset_y + y);
            draw_sprite(GAME.offset_x + x, GAME.offset_y + y, 0, 0, cell->sprite);
        }

    // Draw units
    for (int i = UNIT_COUNT - 1; i > 0; --i)
    {
        Unit * unit = UNIT(i);
        if (unit->type != UNIT_TYPE_NONE) // Should only draw units that are in the view
            draw_unit(unit, i);
    }

    if (GAME.selected_unit != NO_UNIT)
    {
        Unit * unit = UNIT(GAME.selected_unit);

        draw_selected_unit(unit, GAME.selected_unit);
        draw_sprite(unit->x, unit->y, 0, 0, SPRITE_SELECTION);
    }

    {

        if (GAME.selected_action == UNIT_ACTION_BUILD_WALL)
        {
            draw_sprite(CURSOR_POS, 0, 0, SPRITE_BUILD_SELECTION);
        }
        else if (GAME.selected_unit != NO_UNIT)
        {
            Unit * cursor_unit = UNIT_POS(GAME.cursor_x, GAME.cursor_y);

            if (cursor_unit == NULL_UNIT)
            {
                draw_sprite(CURSOR_POS, 0, 0, SPRITE_MOVE_SELECTION);
            }
            else if (cursor_unit->owner == GAME.local_player &&
                     (cursor_unit->type == UNIT_TYPE_WALL || cursor_unit->type == UNIT_TYPE_PLAYER) &&
                     cursor_unit->hit_points < MAX_HITPOINTS[cursor_unit->type])
                draw_sprite(CURSOR_POS, 0, 0, SPRITE_BUILD_SELECTION);
        }
    }

    // Draw fog-of-war
    bool * fog_of_war = VIEW_PLAYER->fog_of_war;
    for (int y = 0; y < VIEW_HEIGHT; ++y)
        for (int x = 0; x < VIEW_WIDTH; ++x)
        {
            int px = GAME.offset_x + x;
            int py = GAME.offset_y + y;
            int idx = py * MAP_WIDTH + px;

            if (fog_of_war[idx])
            {
                draw_sprite(px, py, 0, 0, SPRITE_FOG_OF_WAR(0));
            }
            else
            {
                int sprite = get_fog_of_war_sprite(fog_of_war, px, py);
                if (sprite >= 0)
                    draw_sprite(px, py, 0, 0, sprite);
            }
        }

    // Draw interface
    int ui_w = MAP_WIDTH + 28;
    int ui_h = MAP_HEIGHT + 2;
    int ui_x = CANVAS_WIDTH - ui_w;
    int ui_y = CANVAS_HEIGHT - ui_h;

    rect_draw(rect_make_size(ui_x + 1, ui_y + 1, ui_w - 1, ui_h - 1), COLOR_UI_FACE);
    rect_draw(rect_make_size(ui_x + 2, ui_y, ui_w - 2, 1), COLOR_UI_HIGHLIGHT);
    rect_draw(rect_make_size(ui_x, ui_y + 2, 1, ui_h - 2), COLOR_UI_HIGHLIGHT);

    // Draw corner
    Rect rect = rect_make_size(157, 61, 3, 3);
    bitmap_draw(ui_x, ui_y, 0, 0, &RES.tilesheet, &rect, 0, 0);

    bool is_in_issue_cmd = GAME.stage == STAGE_ISSUE_COMMAND;

    // End turn
    if (ui_button(ui_x + 2, ui_y + 2, rect_make_size(109, 56 - (is_in_issue_cmd ? 0 : 8), 11, 8), UI_BUTTON_NEXT, is_in_issue_cmd, false))
        player_done();

    // Build walls
    if (ui_button(ui_x + 2, ui_y + 15, rect_from_sprite(SPRITE_WALL(0)), UI_BUTTON_TOOLBAR, is_in_issue_cmd, GAME.selected_action == UNIT_ACTION_BUILD_WALL))
    {
        if (GAME.selected_action == UNIT_ACTION_BUILD_WALL)
            GAME.selected_action = UNIT_ACTION_NONE;
        else
            GAME.selected_action = UNIT_ACTION_BUILD_WALL;
    }

    // Produce warior
    if (ui_button(ui_x + 15, ui_y + 15, rect_from_sprite(SPRITE_WARIOR(GAME.local_player)), UI_BUTTON_TOOLBAR, is_in_issue_cmd, UNIT(LOCAL_PLAYER->flag)->command.type == COMMAND_CONSTRUCT))
    {
        if (UNIT(LOCAL_PLAYER->flag)->command.type == COMMAND_CONSTRUCT)
            stop_construct(LOCAL_PLAYER->flag);
        else
            unit_produce(GAME.local_player, LOCAL_PLAYER->flag, UNIT_TYPE_WARIOR);
    }

    // Draw mini map
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            int px = CANVAS_WIDTH - MAP_WIDTH + x;
            int py = CANVAS_HEIGHT - MAP_HEIGHT + y;

            int map_idx = y * MAP_WIDTH + x;
            int canvas_idx = py * CANVAS_WIDTH + px;

            u8 color = 0;

            if (((x == GAME.offset_x || x == (GAME.offset_x + VIEW_WIDTH - 1)) && y >= GAME.offset_y && y < (GAME.offset_y + VIEW_HEIGHT)) ||
                ((y == GAME.offset_y || y == (GAME.offset_y + VIEW_HEIGHT - 1)) && x >= GAME.offset_x && x < (GAME.offset_x + VIEW_WIDTH)))
            {
                color = COLOR_WHITE;
            }
            else if (!fog_of_war[map_idx])
            {
                color = COLOR_GREEN;

                if (HAS_UNIT(x, y))
                {
                    Unit * unit = UNIT_POS(x, y);
                    switch (unit->type)
                    {
                        case UNIT_TYPE_WALL:
                            color = COLOR_LIGHT_GRAY;
                            break;

                        case UNIT_TYPE_WARIOR:
                        case UNIT_TYPE_PLAYER:
                            color = COLOR_PLAYER_1 + unit->owner;
                            break;
                    }
                }
            }

            if (color != 0)
                CORE->canvas->pixels[canvas_idx] = color;
        }
    }

    if (GAME.selected_unit != NO_UNIT)
    {
        char buff[256];
        snprintf(buff, 255, "ID:%d X:%d Y:%d O:%d HP:%d CMD:%s", GAME.selected_unit, SELECTED_UNIT->x, SELECTED_UNIT->y, SELECTED_UNIT->owner, SELECTED_UNIT->hit_points, COMMAND_NAMES[SELECTED_UNIT->command.type]);
        text_draw(0, CANVAS_HEIGHT - 8, buff, 2);
    }

    end_ui();
}


//  ######  ######## ######## ########     ####  ######   ######  ##     ## ########     ######  ##     ## ########
// ##    ##    ##    ##       ##     ##     ##  ##    ## ##    ## ##     ## ##          ##    ## ###   ### ##     ##
// ##          ##    ##       ##     ##     ##  ##       ##       ##     ## ##          ##       #### #### ##     ##
//  ######     ##    ######   ########      ##   ######   ######  ##     ## ######      ##       ## ### ## ##     ##
//       ##    ##    ##       ##            ##        ##       ## ##     ## ##          ##       ##     ## ##     ##
// ##    ##    ##    ##       ##            ##  ##    ## ##    ## ##     ## ##          ##    ## ##     ## ##     ##
//  ######     ##    ######## ##           ####  ######   ######   #######  ########     ######  ##     ## ########


static void step_cursor()
{
    int width = MAP_WIDTH - VIEW_WIDTH;
    int height = MAP_HEIGHT - VIEW_HEIGHT;

    if (key_pressed(KEY_LEFT))
        GAME.offset_x = clamp(GAME.offset_x - 1, 0, width);

    if (key_pressed(KEY_RIGHT))
        GAME.offset_x = clamp(GAME.offset_x + 1, 0, width);

    if (key_pressed(KEY_UP))
        GAME.offset_y = clamp(GAME.offset_y - 1, 0, height);

    if (key_pressed(KEY_DOWN))
        GAME.offset_y = clamp(GAME.offset_y + 1, 0, height);

    GAME.cursor_x = GAME.offset_x + CORE->mouse_x / TILE_SIZE;
    GAME.cursor_y = GAME.offset_y + CORE->mouse_y / TILE_SIZE;
}

void player_done()
{
    LOCAL_PLAYER->stage_done = true;
    GAME.selected_unit = NO_UNIT;
}

static void issue_unit_order(int x, int y)
{
    if (SELECTED_UNIT->type == UNIT_TYPE_WARIOR)
    {
        Unit * clicked_unit = UNIT_POS(x, y);

        if (clicked_unit->owner == GAME.local_player || clicked_unit->type == UNIT_TYPE_NONE)
        {
            // Own unit or nothing

            if (clicked_unit->type == UNIT_TYPE_WALL || clicked_unit->type == UNIT_TYPE_PLAYER)
            {
                if (clicked_unit->hit_points < MAX_HITPOINTS[clicked_unit->type])
                    command_construct(GAME.local_player, GAME.selected_unit, x, y);
            }
            else
            {
                command_move_to(GAME.local_player, GAME.selected_unit, x, y);
            }
        }
        else
        {
            // Enemy
        }
    }
}

static void step_player_minimap(int x, int y)
{
    if (key_down(KEY_LBUTTON))
    {
        if (key_pressed(KEY_LBUTTON))
        {
            GAME.inside_minimap = true;
            GAME.minimap_x = x;
            GAME.minimap_y = y;

            if (x < GAME.offset_x || x > (GAME.offset_x + VIEW_WIDTH) ||
                y < GAME.offset_y || y > (GAME.offset_y + VIEW_HEIGHT))
                focus_view_on(x, y);
        }

        if (GAME.inside_minimap)
        {
            GAME.offset_x += x - GAME.minimap_x;
            GAME.offset_y += y - GAME.minimap_y;
            GAME.minimap_x = x;
            GAME.minimap_y = y;
        }
    }

    if (key_pressed(KEY_RBUTTON) && GAME.selected_unit != NO_UNIT)
    {
        GAME.inside_minimap = true;
        issue_unit_order(x, y);
    }
}

static void step_player_world()
{
    // Issue commands to units
    if (key_pressed(KEY_RBUTTON))
    {
        switch (GAME.selected_action)
        {
            case UNIT_ACTION_BUILD_WALL:
                {
                    build_wall(CURSOR_POS);
                }
                break;

            case UNIT_ACTION_NONE:
                {
                    if (GAME.selected_unit != NO_UNIT)
                    {
                        GAME.inside_minimap = false;
                        issue_unit_order(CURSOR_POS);
                    }
                }
                break;
        }
    }
}

static void step_player()
{
    if (VIEW_PLAYER->stage_done)
        return;

    int hover_unit_id = CELL(GAME.cursor_x, GAME.cursor_y)->unit;
    Unit * hover_unit = UNIT(hover_unit_id);

    bool inside_minimap = CORE->mouse_x >= (CANVAS_WIDTH - MAP_WIDTH) && CORE->mouse_y >= (CANVAS_HEIGHT - MAP_HEIGHT);

    // Select units if we are autside of the minimap
    if (!inside_minimap && key_pressed(KEY_LBUTTON))
    {
        GAME.inside_minimap = false;

        if (hover_unit->owner == GAME.view_player && hover_unit->is_ready)
        {
            GAME.selected_unit = hover_unit_id;
            GAME.selected_action = UNIT_ACTION_NONE;
        }
        else
            GAME.selected_unit = NO_UNIT;
    }

    // Only the local player can issue commands
    if (GAME.view_player != GAME.local_player)
        return;

    // Issue command in the real world or from the minimap
    if (inside_minimap)
        step_player_minimap(CORE->mouse_x - (CANVAS_WIDTH - MAP_WIDTH), CORE->mouse_y - (CANVAS_HEIGHT - MAP_HEIGHT));
    else
        step_player_world();

    if (key_pressed(KEY_B) && SELECTED_UNIT->type == UNIT_TYPE_PLAYER)
    {
    }

    if (key_pressed(KEY_W))
        GAME.selected_action = UNIT_ACTION_BUILD_WALL;
    if (key_pressed(KEY_M))
        GAME.selected_action = UNIT_ACTION_MOVE;

    if (key_pressed(KEY_SPACE))
        player_done();
}

static void switch_player()
{
    if (key_pressed(KEY_1))
    {
        GAME.view_player = 0;
        GAME.selected_unit = NO_UNIT;
    }

    if (key_pressed(KEY_2) && GAME.player_count >= 2)
    {
        GAME.view_player = 1;
        GAME.selected_unit = NO_UNIT;
    }

    if (key_pressed(KEY_3) && GAME.player_count >= 3)
    {
        GAME.view_player = 2;
        GAME.selected_unit = NO_UNIT;
    }

    if (key_pressed(KEY_4) && GAME.player_count >= 4)
    {
        GAME.view_player = 3;
        GAME.selected_unit = NO_UNIT;
    }
}

static void step_issue_commands()
{
    step_player();
    switch_player();

    // When the local player is finished with the commands, step the ai
    if (LOCAL_PLAYER->stage_done)
    {
        for (int i = 0; i < GAME.ai_count; ++i)
        {
            AIBrain * ai = AI(i);
            if (!PLAYER(ai->player)->stage_done)
            {
                think_ai(i);
                PLAYER(ai->player)->stage_done = true;
            }
        }
    }

    bool all_done = true;
    for (int p = 0; p < GAME.player_count; ++p)
        all_done &= PLAYER(p)->stage_done;

    if (all_done)
    {
        GAME.stage = STAGE_UNIT_MOVEMENT;
        GAME.playback_frame = -1;
        GAME.playback_player = GAME.stage_initiative_player;
        GAME.playback_unit = 1;
        GAME.playback_unit_cmd = PLAYBACK_START;
        GAME.playback_player_done = 0;

        //log_info("Start playback with player %d\n", GAME.playback_player);
    }
}


//  ######  ######## ######## ########     ##     ##  #######  ##     ## ######## ##     ## ######## ##    ## ########
// ##    ##    ##    ##       ##     ##    ###   ### ##     ## ##     ## ##       ###   ### ##       ###   ##    ##
// ##          ##    ##       ##     ##    #### #### ##     ## ##     ## ##       #### #### ##       ####  ##    ##
//  ######     ##    ######   ########     ## ### ## ##     ## ##     ## ######   ## ### ## ######   ## ## ##    ##
//       ##    ##    ##       ##           ##     ## ##     ##  ##   ##  ##       ##     ## ##       ##  ####    ##
// ##    ##    ##    ##       ##           ##     ## ##     ##   ## ##   ##       ##     ## ##       ##   ###    ##
//  ######     ##    ######## ##           ##     ##  #######     ###    ######## ##     ## ######## ##    ##    ##


static void step_unit_movement()
{
    // Have we cycled through all players?
    if (GAME.playback_player_done >= GAME.player_count)
    {
        GAME.stage = STAGE_COMMAND_PLAYBACK;
        GAME.playback_frame = 0;
        GAME.playback_player = GAME.stage_initiative_player;
        GAME.playback_unit = 1;
        GAME.playback_unit_cmd = PLAYBACK_START;
        GAME.playback_player_done = 0;

        return;
    }

    // Start movement
    if (GAME.playback_frame == -1)
    {
        for (int i = 1; i < UNIT_COUNT; ++i)
        {
            Unit * unit = UNIT(i);
            if (unit->moving && unit->owner == GAME.playback_player)
                unit->stage_movement_done = unit_move_to(true, i, 0);
        }
    }

    GAME.playback_frame++;

    // Animate movement
    for (int i = 1; i < UNIT_COUNT; ++i)
    {
        Unit * unit = UNIT(i);
        if (unit->moving && !unit->stage_movement_done && unit->owner == GAME.playback_player)
            unit->stage_movement_done = unit_move_to(false, i, GAME.playback_frame);
    }

    // Have we run through the all units for the current player?
    if (GAME.playback_frame == (UNIT_MOVEMENT_SPEED * TILE_SIZE))
    {
        GAME.playback_player = (GAME.playback_player + 1) % GAME.player_count;
        GAME.playback_player_done++;
        GAME.playback_frame = -1;
    }
}


//  ######  ######## ######## ########     ########  ##          ###    ##    ## ########     ###     ######  ##    ##
// ##    ##    ##    ##       ##     ##    ##     ## ##         ## ##    ##  ##  ##     ##   ## ##   ##    ## ##   ##
// ##          ##    ##       ##     ##    ##     ## ##        ##   ##    ####   ##     ##  ##   ##  ##       ##  ##
//  ######     ##    ######   ########     ########  ##       ##     ##    ##    ########  ##     ## ##       #####
//       ##    ##    ##       ##           ##        ##       #########    ##    ##     ## ######### ##       ##  ##
// ##    ##    ##    ##       ##           ##        ##       ##     ##    ##    ##     ## ##     ## ##    ## ##   ##
//  ######     ##    ######## ##           ##        ######## ##     ##    ##    ########  ##     ##  ######  ##    ##


static void step_next_player()
{
    GAME.playback_player_done++;
    GAME.playback_player = (GAME.playback_player + 1) % GAME.player_count;
    GAME.playback_unit = 1;
    GAME.playback_unit_cmd = PLAYBACK_START;
    GAME.playback_frame = 0;

    //log_info("Next player %d (%d/%d)\n", GAME.playback_player, GAME.playback_player_done, GAME.player_count);

    // If we have stept through all players, resume with the issue-command stage
    if (GAME.playback_player_done >= GAME.player_count)
    {
        //log_info("Playback done\n");
        GAME.stage = STAGE_ISSUE_COMMAND;

        GAME.view_player = GAME.local_player;
        GAME.stage_initiative_player = (GAME.stage_initiative_player + 1) % GAME.player_count;

        for (int p = 0; p < GAME.player_count; ++p)
        {
            Player * player = PLAYER(p);
            player->stage_done = false;
        }
    }
}

static bool step_unit_commands(int cmd)
{
    switch (UNIT(GAME.playback_unit)->command.type)
    {
        case COMMAND_MOVE_TO:
            return step_move_to(cmd, GAME.playback_player, GAME.playback_unit, GAME.playback_frame);

        case COMMAND_CONSTRUCT:
            return step_construct(cmd, GAME.playback_player, GAME.playback_unit, GAME.playback_frame);

        default:
            return true;
    }
}

static void step_commands()
{
    while (UNIT(GAME.playback_unit)->owner != GAME.playback_player && GAME.playback_unit < UNIT_COUNT)
        GAME.playback_unit++;

    if (GAME.playback_unit >= UNIT_COUNT)
    {
        step_next_player();
    }
    else
    {
        if (step_unit_commands(GAME.playback_unit_cmd))
        {
            //log_info("Unit %d done\n", GAME.playback_unit);

            // if command is finished move along to the next unit
            GAME.playback_unit++;
            GAME.playback_frame = 0;
            GAME.playback_unit_cmd = PLAYBACK_START;
        }
        else
        {
            // If command is not finished, either continue on to the animation phase, or step the animation
            switch (GAME.playback_unit_cmd)
            {
                case PLAYBACK_START:
                    {
                        //log_info("Animate unit %d\n", GAME.playback_unit);
                        Unit * unit = UNIT(GAME.playback_unit);

                        // Move view if unit is not in it, this should only be done if the unit is in combat
                        if (!in_view(unit->x, unit->y) && unit->owner == GAME.view_player)
                            focus_view_on(unit->x, unit->y);

                        GAME.playback_unit_cmd = PLAYBACK_ANIMATE;
                        GAME.playback_frame = 0;
                    }
                    break;

                case PLAYBACK_ANIMATE:
                    GAME.playback_frame++;
                    break;
            }
        }
    }
}

static void step_stage()
{
    switch (GAME.stage)
    {
        case STAGE_ISSUE_COMMAND:
            step_issue_commands();
            break;

        case STAGE_UNIT_MOVEMENT:
            step_unit_movement();
            break;

        case STAGE_COMMAND_PLAYBACK:
            step_commands();
            break;
    }
}

void step_game()
{
    step_cursor();
    step_stage();
}
