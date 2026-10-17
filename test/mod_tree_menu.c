#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "tree_menu.h"
#include "test/test.h"

TEST("Mod: tree menu picks the only tree of a single-type Pokémon")
{
    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 20, 0);
    gSpecialVar_0x8004 = 0;
    TreeMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(gSpecialVar_0x8005, TYPE_FIRE);
}

TEST("Mod: tree menu asks a dual-type Pokémon to choose, then keeps its tree")
{
    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARIZARD, 50, 0);
    gSpecialVar_0x8004 = 0;
    TreeMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 2);

    gSpecialVar_0x8005 = TYPE_FLYING;
    gSpecialVar_0x8006 = 0;
    TreeMenu_Invest();
    TreeMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(gSpecialVar_0x8005, TYPE_FLYING);
}

TEST("Mod: tree menu invests one point and reports locked nodes")
{
    u32 hp;
    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 20, 0);
    hp = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MAX_HP);
    gSpecialVar_0x8004 = 0;
    gSpecialVar_0x8005 = TYPE_FIRE;

    gSpecialVar_0x8006 = 2; // Vitalité
    TreeMenu_CheckNode();
    EXPECT_EQ(gSpecialVar_Result, TREE_INVEST_OK);
    TreeMenu_Invest();
    EXPECT_EQ(GetBoxMonTreeRank(&gParties[B_TRAINER_PLAYER][0].box, 2), 1);
    EXPECT_EQ(GetBoxMonTreePointsAvailable(&gParties[B_TRAINER_PLAYER][0].box), 19);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MAX_HP), hp + hp / 100);

    gSpecialVar_0x8006 = 4; // Impact ardent : il faut 15 points dans le tronc.
    TreeMenu_CheckNode();
    EXPECT_EQ(gSpecialVar_Result, TREE_INVEST_LOCKED);
    TreeMenu_Invest();
    EXPECT_EQ(GetBoxMonTreeRank(&gParties[B_TRAINER_PLAYER][0].box, 4), 0);
}

TEST("Mod: tree menu does nothing for an egg")
{
    bool32 isEgg = TRUE;
    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 5, 0);
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG, &isEgg);
    gSpecialVar_0x8004 = 0;
    TreeMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 0);
}
