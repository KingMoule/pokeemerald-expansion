#include "global.h"
#include "pokemon_build.h"
#include "test/test.h"

#define GENERAL_PHYSICAL 0
#define FIRE_POWER     4
#define FIRE_BLAZE     5
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
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_WATER, GENERAL_PHYSICAL), TREE_INVEST_WRONG_TREE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_NONE);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, GENERAL_PHYSICAL), TREE_INVEST_OK);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_FIRE);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, GENERAL_PHYSICAL), 1);
}

TEST("Mod: branches open after enough General points, then node by node")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 60);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_POWER), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, GENERAL_PHYSICAL, MOD_TREE_BRANCH_UNLOCK_POINTS - 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_POWER), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, GENERAL_PHYSICAL, 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_POWER), TREE_INVEST_OK);

    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_BLAZE), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_FIRE, FIRE_POWER, MOD_TREE_PARENT_RANKS);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_FIRE, FIRE_BLAZE), TREE_INVEST_OK);
}

TEST("Mod: tree nodes stop at their max rank and investing stops without points")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 30);
    InvestTimes(&mon, TYPE_FIRE, GENERAL_PHYSICAL, 10);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, GENERAL_PHYSICAL), TREE_INVEST_MAX_RANK);
    InvestTimes(&mon, TYPE_FIRE, 2, 30 * MOD_TREE_POINTS_PER_LEVEL - 10);
    EXPECT_EQ(GetBoxMonTreePointsAvailable(&mon.box), 0);
    EXPECT_EQ(TryBoxMonInvestInNode(&mon.box, TYPE_FIRE, 3), TREE_INVEST_NO_POINTS);
}

TEST("Mod: other elements open their branches like the Fire tree")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_SQUIRTLE, 60, 0);
    InvestTimes(&mon, TYPE_WATER, GENERAL_PHYSICAL, MOD_TREE_BRANCH_UNLOCK_POINTS - 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_WATER, FIRE_POWER), TREE_INVEST_LOCKED);
    InvestTimes(&mon, TYPE_WATER, GENERAL_PHYSICAL, 1);
    EXPECT_EQ(CanBoxMonInvestInNode(&mon.box, TYPE_WATER, FIRE_POWER), TREE_INVEST_OK);
}

TEST("Mod: a mutation needs enough tree points and a final mastery")
{
    struct Pokemon mon;
    CreateCharmander(&mon, 100);
    InvestTimes(&mon, TYPE_FIRE, GENERAL_PHYSICAL, 10);
    InvestTimes(&mon, TYPE_FIRE, 2, 20);
    InvestTimes(&mon, TYPE_FIRE, FIRE_POWER, MOD_TREE_PARENT_RANKS);
    InvestTimes(&mon, TYPE_FIRE, FIRE_BLAZE, MOD_TREE_PARENT_RANKS);
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
    SetBoxMonTreeRank(&trained.box, 3, 10);
    {
        u32 fire = TYPE_FIRE;
        SetMonData(&trained, MON_DATA_TREE_TYPE, &fire);
    }
    CalculateMonStats(&plain);
    CalculateMonStats(&trained);
    EXPECT_EQ(GetMonData(&trained, MON_DATA_MAX_HP), GetMonData(&plain, MON_DATA_MAX_HP) * 120 / 100);
    EXPECT_EQ(GetMonData(&trained, MON_DATA_SPEED), GetMonData(&plain, MON_DATA_SPEED) * 110 / 100);
}

TEST("Mod: every element has Offense, Defense and Utility branches of four nodes")
{
    enum Type type;
    u32 branch;
    for (type = TYPE_NORMAL; type <= TYPE_FAIRY; type++)
    {
        if (type == TYPE_MYSTERY || type == TYPE_STELLAR)
            continue;
        for (branch = 0; branch < TREE_BRANCH_COUNT; branch++)
        {
            EXPECT(gTreeBranchNames[branch] != NULL);
            EXPECT(GetTreeNode(type, TREE_GENERAL_NODES + branch * TREE_BRANCH_NODES) != NULL);
            EXPECT(GetTreeNode(type, TREE_GENERAL_NODES + branch * TREE_BRANCH_NODES + TREE_BRANCH_NODES - 1) != NULL);
        }
    }
}
