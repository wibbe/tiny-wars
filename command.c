
#include "game.h"

Command command_move_to(Unit * unit, int x, int y)
{
    Command cmd = {
        .type = COMMAND_MOVE_TO,
        .x = x,
        .y = y,
    };

    astar_compute(unit->x, unit->y, cmd.x, cmd.y, cmd.args, COMMAND_ARG_COUNT);
    return cmd;
}

Command command_construct(int x, int y, int type)
{
    int length = 0;
    switch (type)
    {
        case UNIT_TYPE_WALL:
            length = 2;
            break;

        case UNIT_TYPE_WARIOR:
            length = 4;
            break;
    }

    Command cmd = {
        .type = COMMAND_CONSTRUCT,
        .progress = 0,
        .unit = type,
        .x = x,
        .y = y,
        .args = {0, length, 0, 0},
    };
    return cmd;
}

static void move_to_next(int unit_id)
{
    Unit * unit = UNIT(unit_id);

    // Stop if we are already there
    if (unit->x == unit->command.x && unit->y == unit->command.y)
    {
        unit->command.type = COMMAND_NONE;
        return;
    }

    if (!astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT))
    {
        // No solution found
        unit->command.type = COMMAND_NONE;
        return;
    }

    int next_x = unit->command.args[0] % MAP_WIDTH;
    int next_y = unit->command.args[0] / MAP_WIDTH;

    // Initialize movement
    int diff_x = next_x - unit->x;
    int diff_y = next_y - unit->y;

    if (move_unit(unit_id, next_x, next_y))
    {
        unit->offset_x = (diff_x > 0 ? -TILE_SIZE : (diff_x < 0 ? TILE_SIZE : 0));
        unit->offset_y = (diff_y > 0 ? -TILE_SIZE : (diff_y < 0 ? TILE_SIZE : 0));
    }
}

void step_move_to(int player_id, int unit_id, int frame)
{
    if (frame == 0)
    {
        move_to_next(unit_id);
    }
    else if (frame == (PLAYBACK_FRAME_COUNT / 2))
    {
        move_to_next(unit_id);
    }
    else if (frame == PLAYBACK_FRAME_COUNT - 1)
    {
        Unit * unit = UNIT(unit_id);
        astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT);

        // Done
        if (unit->x == unit->command.x && unit->y == unit->command.y)
        {
            unit->command.type = COMMAND_NONE;
            unit->offset_x = 0;
            unit->offset_y = 0;
        }
    }
    else
    {
        //if (frame % 2 == 0)
        {
            Unit * unit = UNIT(unit_id);

            if (unit->offset_x > 0) unit->offset_x--;
            if (unit->offset_x < 0) unit->offset_x++;
            if (unit->offset_y > 0) unit->offset_y--;
            if (unit->offset_y < 0) unit->offset_y++;
        }
    }
}

void step_construct(int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (frame == 0 && unit->command.args[0] < unit->command.args[1])
    {
        unit->command.args[0]++;
        unit->command.progress += 8 / unit->command.args[1];
    }
    else if (frame == PLAYBACK_FRAME_COUNT - 1)
    {
        if (unit->command.args[0] == unit->command.args[1] && is_passable(unit->command.x, unit->command.y))
        {
            alloc_unit(unit->command.x, unit->command.y, unit->command.unit, unit->owner);
            unit->command.type = COMMAND_NONE;
        }
    }
}
