
#include "game.h"

void issue_command(int unit_id, Unit * unit)
{
    log_info("Command (type = %d, x = %d, y = %d) issued on %d by player %d\n", unit->command.type, unit->command.x, unit->command.y, unit_id, unit->owner);
}

void command_move_to(int player_id, int unit_id, int x, int y)
{
    if (!is_passable(x, y))
        return;

    Unit * unit = UNIT(unit_id);

    if (!astar_compute(unit->x, unit->y, x, y, unit->move_path, PATH_LENGTH))
        return;

    unit->command.type = COMMAND_MOVE_TO;
    unit->moving = true;
    unit->move_target_x = x;
    unit->move_target_y = y;
    unit->offset_x = 0;
    unit->offset_y = 0;

    issue_command(unit_id, unit);
}

bool command_construct(int player_id, int unit_id, int x, int y, int type)
{
    if (!is_passable(x, y))
        return false;

    Unit * unit = UNIT(unit_id);
    unit->moving = false;

    int diff_x = abs(unit->x - x);
    int diff_y = abs(unit->y - y);

    // Do we need to move the unit to the construction site first?
    if (diff_x > 1 || diff_y > 1)
    {
        // Find a suitable build position around the site
        int best_idx = -1;
        int best_length = MAP_WIDTH * MAP_HEIGHT;

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
        else
            return false;
    }

    unit->command.type = COMMAND_CONSTRUCT;
    unit->command.x = x;
    unit->command.y = y;
    unit->command.args[0] = alloc_unit(unit->command.x, unit->command.y, type, unit->owner, 0);

    issue_command(unit_id, unit);
    return true;
}

void stop_construct(int unit_id)
{
    Unit * unit = UNIT(unit_id);

    if (unit->command.type != COMMAND_CONSTRUCT)
        return;

    free_unit(unit->command.args[0]);

    unit->command.type = COMMAND_NONE;
    unit->moving = false;
}

bool step_move_to(int cmd, int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (!unit->moving)
        unit->command.type = COMMAND_NONE;

    return true;
}

bool step_construct(int cmd, int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    if (unit->moving)
    {
        return unit_move_to(cmd == PLAYBACK_START, unit_id, frame);
    }
    else
    {
        if (cmd == PLAYBACK_START)
        {
            Unit * new_unit = UNIT(unit->command.args[0]);
            new_unit->hit_points++;

            if (new_unit->hit_points == MAX_HITPOINTS[new_unit->type])
            {
                new_unit->is_ready = true;
                reveal_fog_of_war(unit->owner, unit->command.x, unit->command.y);
                unit->command.type = COMMAND_NONE;
            }
        }
    }

    return true;
}
