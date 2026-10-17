#include "global.h"
#include "caps.h"
#include "event_data.h"
#include "pokemon.h"
#include "test/test.h"

TEST("Mod: experience curves are unchanged up to 100 and keep growing up to the absolute max")
{
    u32 rate, level;
    EXPECT_EQ(gExperienceTables[GROWTH_MEDIUM_FAST][100], 1000000);
    EXPECT_EQ(gExperienceTables[GROWTH_ERRATIC][100], 600000);
    EXPECT_EQ(gExperienceTables[GROWTH_FLUCTUATING][100], 1640000);
    for (rate = 0; rate <= GROWTH_SLOW; rate++)
    {
        for (level = 2; level <= MAX_LEVEL; level++)
            EXPECT_GT(gExperienceTables[rate][level], gExperienceTables[rate][level - 1]);
    }
    EXPECT_EQ(gExperienceTables[GROWTH_MEDIUM_FAST][200], 8000000);
}

TEST("Mod: Pokémon above level 100 keep their experience and level")
{
    struct Pokemon mon;
    u32 exp, level = 0;
    PARAMETRIZE { level = 101; }
    PARAMETRIZE { level = 180; }
    PARAMETRIZE { level = MAX_LEVEL; }
    CreateMonWithIVs(&mon, SPECIES_DRAGONITE, 50, 0, OTID_STRUCT_PLAYER_ID, 0);
    exp = gExperienceTables[gSpeciesInfo[SPECIES_DRAGONITE].growthRate][level];
    SetMonData(&mon, MON_DATA_EXP, &exp);
    CalculateMonStats(&mon);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_EXP), exp);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_LEVEL), level);
    EXPECT_EQ(GetLevelFromBoxMonExp(&mon.box), level);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
}

TEST("Mod: the level cap starts at 100 and each seal raises it")
{
    VarSet(VAR_MOD_LEVEL_SEALS, 0);
    EXPECT_EQ(GetCurrentLevelCap(), MOD_LEVEL_BASE_CAP);
    VarSet(VAR_MOD_LEVEL_SEALS, 3);
    EXPECT_EQ(GetCurrentLevelCap(), MOD_LEVEL_BASE_CAP + 3 * MOD_LEVEL_CAP_PER_SEAL);
    VarSet(VAR_MOD_LEVEL_SEALS, 200);
    EXPECT_EQ(GetCurrentLevelCap(), MAX_LEVEL);
    VarSet(VAR_MOD_LEVEL_SEALS, 0);
}

TEST("Mod: Pokémon cannot level up past the current level cap")
{
    struct Pokemon mon;
    u32 exp;
    CreateMonWithIVs(&mon, SPECIES_DRAGONITE, 100, 0, OTID_STRUCT_PLAYER_ID, 0);
    exp = gExperienceTables[gSpeciesInfo[SPECIES_DRAGONITE].growthRate][101];
    SetMonData(&mon, MON_DATA_EXP, &exp);

    VarSet(VAR_MOD_LEVEL_SEALS, 0);
    EXPECT(!TryIncrementMonLevel(&mon));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_LEVEL), 100);

    VarSet(VAR_MOD_LEVEL_SEALS, 1);
    EXPECT(TryIncrementMonLevel(&mon));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_LEVEL), 101);
    VarSet(VAR_MOD_LEVEL_SEALS, 0);
}
