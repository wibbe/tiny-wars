
#ifndef GAME_H
#define GAME_H

#include "ini.h"

#define MAP_WIDTH   (80)
#define MAP_HEIGHT  (60)
#define TILE_SIZE   (8)
#define VIEW_WIDTH  (CANVAS_WIDTH / TILE_SIZE)
#define VIEW_HEIGHT (CANVAS_HEIGHT / TILE_SIZE)

#define UNIT_COUNT          (2048)
#define COMMAND_ARG_COUNT   (4)
#define PATH_LENGTH         (8)

#define NO_SPRITE (-1)
#define SPRITE(x, y) (((y) << 16) | (x))
#define SPRITE_X(type) ((type) & 0x00ff)
#define SPRITE_Y(type) ((type) >> 16)

#define CELL(x, y) (&GAME.map.cells[(y) * MAP_WIDTH + (x)])

#define CURSOR_CELL (&GAME.map.cells[(GAME.cursor_y) * MAP_WIDTH + (GAME.cursor_x)])
#define CURSOR_POS GAME.cursor_x, GAME.cursor_y

#define NULL_UNIT (&GAME.units[0])
#define UNIT(id) (id == NO_UNIT ? NULL_UNIT : &GAME.units[id])
#define SELECTED_UNIT UNIT(GAME.selected_unit)
#define UNIT_POS(x, y) UNIT(CELL(x, y)->unit)
#define HAS_UNIT(x, y) (CELL(x, y)->unit != NO_UNIT)
#define NO_UNIT (0)

#define PLAYER_COUNT (4)
#define PLAYER(id) (&GAME.players[id])
#define VIEW_PLAYER PLAYER(GAME.view_player)
#define LOCAL_PLAYER PLAYER(GAME.local_player)
#define NO_PLAYER (-1)

#define RANDOM() random(&GAME.random)

#define AI(id) (&GAME.ai[id])

#define PLAYBACK_FRAME_COUNT (32) // command playback takes 1 seconds for each player


enum Colors {
    COLOR_BLACK = 1,
    COLOR_WHITE,
    COLOR_LIGHT_GRAY,
    COLOR_DARK_GRAY,
    COLOR_GREEN,
    COLOR_PLAYER_1,
    COLOR_PLAYER_2,
    COLOR_PLAYER_3,
    COLOR_PLAYER_4,
    COLOR_COUNT,
};

enum GameStage {
    STAGE_ISSUE_COMMAND,
    STAGE_COMMAND_PLAYBACK,
};

enum UnitType {
    UNIT_TYPE_NONE = 0,
    UNIT_TYPE_PLAYER,
    UNIT_TYPE_WALL,
    UNIT_TYPE_WARIOR,
};

enum UnitAction {
    UNIT_ACTION_NONE = 0,
    UNIT_ACTION_MOVE,
    UNIT_ACTION_BUILD_WALL,
};

enum CommandType {
    COMMAND_NONE = 0,
    COMMAND_CONSTRUCT,
    COMMAND_MOVE_TO,
};

enum PlaybackExecution {
    PLAYBACK_START,
    PLAYBACK_ANIMATE,
};

typedef struct Command {
    int type;
    int progress;   // 0-8
    int unit;
    int x;
    int y;
    int args[COMMAND_ARG_COUNT];
} Command;

typedef struct Unit {
    int type;
    int owner;
    int sprite;
    int x;
    int y;

    int offset_x;
    int offset_y;

    bool is_ready;
    int hit_points;

    bool moving;
    int move_target_x;
    int move_target_y;
    int move_path[PATH_LENGTH];

    int next_free;

    Command command;
} Unit;

typedef struct Cell {
    int sprite;
    bool blocked;
    int unit;
} Cell;

typedef struct {
    Cell cells[MAP_WIDTH * MAP_HEIGHT];
} Map;

typedef struct Player {
    int flag;
    int id;
    bool ai_controlled;
    bool stage_done;
    bool fog_of_war[MAP_WIDTH * MAP_HEIGHT];
} Player;

typedef struct {
    int player;
} AIBrain;

typedef struct {
    Map map;
    Random random;

    Unit units[UNIT_COUNT];
    int first_free_unit;

    Player players[PLAYER_COUNT];
    AIBrain ai[PLAYER_COUNT];

    int player_count;
    int ai_count;

    int view_player;
    int local_player;

    int stage;
    int stage_initiative_player;

    int playback_player_done;
    int playback_player;
    int playback_unit;
    int playback_unit_cmd;
    int playback_frame;

    int selected_unit;
    int selected_action;

    int offset_x;
    int offset_y;

    int cursor_x;
    int cursor_y;

    bool inside_minimap;
    int minimap_x;
    int minimap_y;
} Game;

typedef struct {
    Bitmap tilesheet;
    Font font;
} Res;

extern Game GAME;
extern Res RES;

bool unit_move_to(bool start, int unit_id, int frame);

void command_move_to(int player_id, int unit_id, int x, int y);
bool command_construct(int player_id, int unit_id, int x, int y, int type);

void stop_construct(int player_id, int unit_id);

bool step_move_to(int cmd, int player, int unit, int frame);
bool step_construct(int cmd, int player, int unit, int frame);

bool is_passable(int x, int y);
bool in_view_of_local_player(int x, int y);

bool find_empty(int x, int y, Vec * result);

void reveal_fog_of_war(int player_id, int x, int y);

int astar_compute(int start_x, int start_y, int end_x, int end_y, int * path, int path_length);

void think_ai(int ai_id);

#endif
