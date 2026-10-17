#include "global.h"
#include "pokemon_build.h"
#include "test/test.h"

#define TRUNK_PHYSICAL 0
#define FIRE_IMPACT    4
#define FIRE_EMBERS    5
#define FIRE_HEART     7

static void CreateCharmander(struct Pokemon *mon, u32 level)
{
    CreateRandomMonWithIVs(mon, SPECIES_CHARMANDER, level, 0);
}

static void InvestTimes(struct Pokemon *mon, enum Type treeType, u32 node, u32 times)
{
    u32 i;
    for (i = 0; i < times; i++)
        EXPECT_EQ(TryBoxMonInvestInNode(&mon->box, treeType, node), TREE_INVEST_OK);
}

TEST("Mod: tree points come from the level and are capped by the budget")
{
    struct Pokemon mon;
    u32 bonus = 30;
    CreateCharmander(&mon, 50);
    EXPECT_EQ(GetBoxMonTreePointsEarned(&mon.box), 50 * MOD_TREE_POINTS_PER_LEVEL);
    SetMonData(&mon, MON_DATA_TREE_POINTS, &bonus);
    EXPECT_EQ(GetBoxMonTreePointsEarned(&mon.box), min(50 * MOD_TREE_POINTS_PER_LEVEL + 30, MOD_TREE_POINT_BUDGET));
    CreateCharmander(&mon, 100);
    SetMonData(&mon, MON_DATA_TREE_POINTS, &bonus);
    EXPECT_EQ(GetBoxMonTreePointsEarned(&mon.box), MOD_TREE_POINT_BUDGET);
}

TEST("Mod: a Pokémon can only invest in the tree of one of its types")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 50);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_WATER, TRUNK_PHYSICAL), TREE_INVEST_WRONG_TREE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_NONE);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, TRUNK_PHYSICAL), TREE_INVEST_OK);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_FIRE);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, TRUNK_PHYSICAL), 1);
}

TEST("Mod: branches open after enough trunk points, then node by node")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 60);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_IMPACT), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, TRUNK_PHYSICAL, MOD_TREE_BRANCH_UNLOCK_POINTS - 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_IMPACT), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, TRUNK_PHYSICAL, 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_IMPACT), TREE_INVEST_OK);

    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_EMBERS), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, FIRE_IMPACT, MOD_TREE_PARENT_RANKS);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_EMBERS), TREE_INVEST_OK);
}

TEST("Mod: tree nodes stop at their max rank and investing stops without points")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 30);
    InvestTimes(&mon, TYPE_FIRE, TRUNK_PHYSICAL, 20);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, TRUNK_PHYSICAL), TREE_INVEST_MAX_RANK);
    InvestTimes(&mon, TYPE_FIRE, 1, 30 * MOD_TREE_POINTS_PER_LEVEL - 20);
    EXPECT_EQ(GetBoxMonTreePointsAvailable(&mon.box), 0);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, 2), TREE_INVEST_NO_POINTS);
}

TEST("Mod: elements without written branches only have the trunk")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_SQUIRTLE, 60, 0);
    InvestTimes(&mon, TYPE_WATER, TRUNK_PHYSICAL, MOD_TREE_BRANCH_UNLOCK_POINTS);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_WATER, FIRE_IMPACT), TREE_INVEST_NO_NODE);
}

TEST("Mod: a mutation needs enough tree points and a final mastery")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 100);
    InvestTimes(&mon, TYPE_FIRE, TRUNK_PHYSICAL, 20);
    InvestTimes(&mon, TYPE_FIRE, 1, 20);
    InvestTimes(&mon, TYPE_FIRE, FIRE_IMPACT, MOD_TREE_PARENT_RANKS);
    InvestTimes(&mon, TYPE_FIRE, FIRE_EMBERS, MOD_TREE_PARENT_RANKS);
    InvestTimes(&mon, TYPE_FIRE, 6, MOD_TREE_PARENT_RANKS);
    EXPECT_GE(GetBoxMonTreePointsSpent(&mon.box), MOD_MUTATION_TREE_POINTS);
    EXPECT_EQ(CanBoxMonAcquireMutation(&mon.box), FALSE);
    InvestTimes(&mon, TYPE_FIRE, FIRE_HEART, 1);
    EXPECT_EQ(CanBoxMonAcquireMutation(&mon.box), TRUE);
}

TEST("Mod: Vitality and Swiftness raise max HP and Speed")
{
    struct Pokemon plain, trained;
    CreateCharmander(&plain, 100);
    trained = plain;
    SetBoxMonTreeRank(&trained.box, 2, 20);
    SetBoxMonTreeRank(&trained.box, 3, 20);
    {
        u32 fire = TYPE_FIRE;
        SetMonData(&trained, MON_DATA_TREE_TYPE, &fire);
    }
    CalculateMonStats(&plain);
    CalculateMonStats(&trained);
    EXPECT_EQ(GetMonData(&trained, MON_DATA_MAX_HP), GetMonData(&plain, MON_DATA_MAX_HP) * 120 / 100);
    EXPECT_EQ(GetMonData(&trained, MON_DATA_SPEED), GetMonData(&plain, MON_DATA_SPEED) * 110 / 100);
}
