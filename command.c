
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

static bool move_to_next(int unit_id)
{
    Unit * unit = UNIT(unit_id);

    // Stop if we are already there
    if (unit->x == unit->command.x && unit->y == unit->command.y)
    {
        unit->command.type = COMMAND_NONE;
        return true;
    }

    if (!astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT))
    {
        // No solution found
        return true;
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

    return false;
}

bool step_move_to(int cmd, int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (cmd == PLAYBACK_START)
    {
        // Just finish directly if we could not find a path forward
        if (!astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT))
            return true;

        bool unit_in_view = in_view_of_local_player(unit->x, unit->y);
        bool first_target_in_view = unit->command.args[0] == -1 ? false : in_view_of_local_player(unit->command.args[0] % MAP_WIDTH, unit->command.args[0] / MAP_WIDTH);
        bool second_target_in_view = unit->command.args[1] == -1 ? false : in_view_of_local_player(unit->command.args[1] % MAP_WIDTH, unit->command.args[1] / MAP_WIDTH);

        // If anything is in view, we need to continue on to the animation stage
        if (unit_in_view || first_target_in_view || second_target_in_view)
            return false;

        // Otherwise we just move the player to the correct cells. We need to do both of the moves, so we unveil the fog-of-war correctly
        if (unit->command.args[0] != -1)
            move_unit(unit_id, unit->command.args[0] % MAP_WIDTH, unit->command.args[0] / MAP_WIDTH);
        if (unit->command.args[1] != -1)
            move_unit(unit_id, unit->command.args[1] % MAP_WIDTH, unit->command.args[1] / MAP_WIDTH);

        // Are we done with the move command?
        if (unit->x == unit->command.x && unit->y == unit->command.y)
            unit->command.type = COMMAND_NONE;
        else
            astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT);

        return true;
    }
    else
    {
        // Animate the player
        if (frame == 0 || frame == (PLAYBACK_FRAME_COUNT / 2))
        {
            if (move_to_next(unit_id))
                return true;
        }
        else if (frame == PLAYBACK_FRAME_COUNT)
        {
            astar_compute(unit->x, unit->y, unit->command.x, unit->command.y, unit->command.args, COMMAND_ARG_COUNT);

            // Are we at the destination?
            if (unit->x == unit->command.x && unit->y == unit->command.y)
            {
                unit->command.type = COMMAND_NONE;
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

bool step_construct(int cmd, int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (cmd == PLAYBACK_START)
    {
        if (unit->command.args[0] < unit->command.args[1])
        {
            unit->command.args[0]++;
            unit->command.progress += 8 / unit->command.args[1];
        }
        else
        {
            if (unit->command.args[0] == unit->command.args[1] && is_passable(unit->command.x, unit->command.y))
            {
                alloc_unit(unit->command.x, unit->command.y, unit->command.unit, unit->owner);
                reveal_fog_of_war(unit->owner, unit->command.x, unit->command.y);
                unit->command.type = COMMAND_NONE;
            }
        }
    }

    return true;
}
