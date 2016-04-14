
#include "game.h"

void think_ai(int ai_id)
{
    AIBrain * ai = AI(ai_id);
    Player * player = PLAYER(ai->player);

    int warior_count = 0;

    // Randomly move our wariors
    for (int i = 1; i < UNIT_COUNT; ++i)
    {
        Unit * unit = UNIT(i);
        if (unit->owner == ai->player && unit->type == UNIT_TYPE_WARIOR)
        {
            warior_count++;

            if (unit->command.type == COMMAND_NONE)
            {
                // Unit is not doing anything
                int x = RANDOM() % MAP_WIDTH;
                int y = RANDOM() % MAP_HEIGHT;

                command_move_to(ai->player, i, x, y);
            }
        }
    }

    // should we build more wariors?
    Unit * flag = UNIT(player->flag);

    if (warior_count < 6 && flag->command.type == COMMAND_NONE)
    {
        //issue_command()
        Vec pos;
        if (find_empty(flag->x, flag->y, &pos))
            command_construct(ai->player, player->flag, pos.x, pos.y, UNIT_TYPE_WARIOR);
    }

}
