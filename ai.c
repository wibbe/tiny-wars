
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
        if (unit->owner == ai->player && unit->type == UNIT_TYPE_WARIOR && unit->is_ready)
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

    // Should we build more wariors?
    Unit * flag = UNIT(player->flag);
    if (warior_count < 6 && flag->command.type == COMMAND_NONE)
    {
        unit_produce(ai->player, player->flag, UNIT_TYPE_WARIOR);
    }

}
