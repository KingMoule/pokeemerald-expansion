#include "global.h"
#include "battle.h"
#include "pokemon_build.h"
#include "test/test.h"

// L'IA place parfois un autre Pokémon de l'équipe dans gBattleMons sans changer
// gBattlerPartyIndexes : sa mutation doit quand même être retrouvée.
TEST("Mod: a simulated switch-in keeps its own mutation")
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    ZeroPlayerPartyMons();
    CreateMon(&party[0], SPECIES_CHARMANDER, 50, 1, OTID_STRUCT_RANDOM_NO_SHINY);
    CreateMon(&party[1], SPECIES_GYARADOS, 50, 2, OTID_STRUCT_RANDOM_NO_SHINY);
    EXPECT(SetBoxMonMutation(&party[1].box, TYPE_FLYING, TYPE_GROUND));

    gBattleTypeFlags = 0;
    gBattlerPositions[B_BATTLER_0] = B_POSITION_PLAYER_LEFT;
    gBattlerPartyIndexes[B_BATTLER_0] = 0;
    PokemonToBattleMon(&party[1], &gBattleMons[B_BATTLER_0]);

    EXPECT_EQ(gBattleMons[B_BATTLER_0].types[1], TYPE_GROUND);
    EXPECT_EQ(GetBattlerMutatedType(B_BATTLER_0, 0), TYPE_WATER);
    EXPECT_EQ(GetBattlerMutatedType(B_BATTLER_0, 1), TYPE_GROUND);
}
