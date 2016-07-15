
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

bool command_construct(int player_id, int unit_id, int x, int y)
{
    Unit * to_construct = UNIT_POS(x, y);
    Unit * unit = UNIT(unit_id);

    if (to_construct->owner != player_id)
        return false;

    unit->command.type = COMMAND_CONSTRUCT;
    unit->command.x = x;
    unit->command.y = y;
    unit->moving = false;
    issue_command(unit_id, unit);

    if (!in_reach_of_unit(unit_id, unit->command.x, unit->command.y))
        unit_move_close_to(unit_id, unit->command.x, unit->command.y);

    return true;
}

void stop_construct(int unit_id)
{
    Unit * unit = UNIT(unit_id);

    if (unit->command.type != COMMAND_CONSTRUCT)
        return;

    Cell * cell = CELL(unit->command.x, unit->command.y);
    if (cell->unit != NO_UNIT)
        free_unit(cell->unit);

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

    if (cmd == PLAYBACK_START)
    {
        bool can_reach = in_reach_of_unit(unit_id, unit->command.x, unit->command.y);

        if (can_reach && !unit->moving)
        {
            // If we are in reach of the other unit, construct it.

            Unit * new_unit = UNIT_POS(unit->command.x, unit->command.y);

            if (new_unit->hit_points < MAX_HITPOINTS[new_unit->type])
            {
                new_unit->hit_points++;

                if (new_unit->hit_points == MAX_HITPOINTS[new_unit->type])
                {
                    new_unit->is_ready = true;
                    reveal_fog_of_war(unit->owner, unit->command.x, unit->command.y);
                    unit->command.type = COMMAND_NONE;
                }
            }
        }
        else if (!can_reach)
        {
            // If we are to far away from the unit, we need to walk up to it
            unit_move_close_to(unit_id, unit->command.x, unit->command.y);
        }
    }

    return true;
}
