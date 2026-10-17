#include "global.h"
#include "test/battle.h"

WILD_BATTLE_TEST("Mod: defeating a Pokémon gives experience but no EVs")
{
    s32 gainedExp;

    GIVEN {
        ASSUME(gSpeciesInfo[SPECIES_GEODUDE].evYield_Defense > 0);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); }
        OPPONENT(SPECIES_GEODUDE) { Level(10); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Geodude fainted!");
        EXPERIENCE_BAR(player, captureGainedExp: &gainedExp);
    } THEN {
        EXPECT_GT(gainedExp, 0);
        EXPECT_EQ(GetMonEVCount(&gParties[B_TRAINER_PLAYER][0]), 0);
    }
}
