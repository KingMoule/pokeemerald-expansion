#include "global.h"
#include "event_data.h"
#include "test/battle.h"

WILD_BATTLE_TEST("Mod: a level 100 Pokémon gains experience only once a seal raised the cap", s32 exp)
{
    u32 seals;
    PARAMETRIZE { seals = 0; }
    PARAMETRIZE { seals = 1; }
    GIVEN {
        ASSUME(MOD_LEVELS_BEYOND_100);
        VarSet(VAR_MOD_LEVEL_SEALS, seals);
        PLAYER(SPECIES_WOBBUFFET) { Level(100); }
        OPPONENT(SPECIES_CATERPIE) { Level(50); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
        if (seals == 0)
            NOT EXPERIENCE_BAR(player);
        else
            EXPERIENCE_BAR(player, captureGainedExp: &results[i].exp);
    } FINALLY {
        EXPECT_EQ(results[0].exp, 0);
        EXPECT_GT(results[1].exp, 0);
        VarSet(VAR_MOD_LEVEL_SEALS, 0);
    }
}
