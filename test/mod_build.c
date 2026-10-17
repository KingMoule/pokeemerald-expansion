#include "global.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "test/test.h"

TEST("Mod: a new Pokémon has an empty build")
{
    u32 node, slot;
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 5, 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_NONE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_POINTS), 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MUTATION_TYPE), TYPE_NONE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE_VARIANTS), 0);
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        EXPECT_EQ(GetBoxMonTreeRank(&mon.box, node), 0);
    for (slot = 0; slot < MAX_MON_MOVES; slot++)
        EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, slot), 0);
}

TEST("Mod: tree node ranks are stored independently")
{
    u32 node;
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 5, 0);
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        SetBoxMonTreeRank(&mon.box, node, (node * 7 + 3) % 32);
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        EXPECT_EQ(GetBoxMonTreeRank(&mon.box, node), (node * 7 + 3) % 32);

    // Changing one node must not touch its neighbours.
    SetBoxMonTreeRank(&mon.box, 5, 0);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, 4), (4 * 7 + 3) % 32);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, 5), 0);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, 6), (6 * 7 + 3) % 32);
}

TEST("Mod: tree ranks are capped and spent points are their sum")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 5, 0);
    SetBoxMonTreeRank(&mon.box, 0, 20);
    SetBoxMonTreeRank(&mon.box, 1, 15);
    SetBoxMonTreeRank(&mon.box, 15, 99);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, 15), 31);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&mon.box), 20 + 15 + 31);
    // Out of range nodes are ignored.
    SetBoxMonTreeRank(&mon.box, MOD_TREE_NODE_COUNT, 10);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, MOD_TREE_NODE_COUNT), 0);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&mon.box), 20 + 15 + 31);
}

TEST("Mod: move variants are stored per move slot")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 5, 0);
    SetBoxMonMoveVariant(&mon.box, 0, 1);
    SetBoxMonMoveVariant(&mon.box, 1, 3);
    SetBoxMonMoveVariant(&mon.box, 3, 2);
    SetBoxMonMoveVariant(&mon.box, 1, 0);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), 1);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 1), 0);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 2), 0);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 3), 2);
}

TEST("Mod: build fields survive Set/GetMonData and moving to the box")
{
    u32 treeType = TYPE_FIRE, points = 42, mutationType = TYPE_GRASS, mutationSlot = 1;
    struct Pokemon mon, copy;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 5, 0);
    SetMonData(&mon, MON_DATA_TREE_TYPE, &treeType);
    SetMonData(&mon, MON_DATA_TREE_POINTS, &points);
    SetMonData(&mon, MON_DATA_MUTATION_TYPE, &mutationType);
    SetMonData(&mon, MON_DATA_MUTATION_SLOT, &mutationSlot);
    SetBoxMonTreeRank(&mon.box, 3, 12);
    SetBoxMonMoveVariant(&mon.box, 2, 3);

    BoxMonToMon(&mon.box, &copy);
    EXPECT_EQ(GetMonData(&copy, MON_DATA_TREE_TYPE), TYPE_FIRE);
    EXPECT_EQ(GetMonData(&copy, MON_DATA_TREE_POINTS), 42);
    EXPECT_EQ(GetMonData(&copy, MON_DATA_MUTATION_TYPE), TYPE_GRASS);
    EXPECT_EQ(GetMonData(&copy, MON_DATA_MUTATION_SLOT), 1);
    EXPECT_EQ(GetBoxMonTreeRank(&copy.box, 3), 12);
    EXPECT_EQ(GetBoxMonMoveVariant(&copy.box, 2), 3);
    // The encrypted data is left untouched.
    EXPECT_EQ(GetMonData(&copy, MON_DATA_SPECIES), SPECIES_CHARMANDER);
    EXPECT_EQ(GetMonData(&copy, MON_DATA_SANITY_IS_BAD_EGG), FALSE);
}
