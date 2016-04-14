
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

void command_construct(int player_id, int unit_id, int x, int y, int type)
{
    /*
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

    Unit * unit = UNIT(unit_id);

    unit->command.type = COMMAND_CONSTRUCT;
    unit->command.progress = 0;
    unit->command.unit = type;
    unit->command.x = x;
    unit->command.y = y;
    unit->command.args = {0, length, 0, 0};

    int diff_x = abs(unit->x - x);
    int diff_y = abs(unit->y - y);

    // Do we need to move the unit to the construction sire first?
    //if (diff_x > 1 || diff_y > 1)

    issue_command(unit_id, unit);
    */
}

bool step_move_to(int cmd, int player_id, int unit_id, int frame)
{
    Unit * unit = UNIT(unit_id);

    bool done = unit_move_to(cmd == PLAYBACK_START, unit_id, frame);

    if (done && !unit->moving)
        unit->command.type = COMMAND_NONE;

    return done;
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
