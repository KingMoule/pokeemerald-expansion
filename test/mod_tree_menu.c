#include "global.h"
#include "event_data.h"
#include "item.h"
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

    gSpecialVar_0x8006 = 2; // Vitality
    TreeMenu_CheckNode();
    EXPECT_EQ(gSpecialVar_Result, TREE_INVEST_OK);
    TreeMenu_Invest();
    EXPECT_EQ(GetBoxMonTreeRank(&gParties[B_TRAINER_PLAYER][0].box, 2), 1);
    EXPECT_EQ(GetBoxMonTreePointsAvailable(&gParties[B_TRAINER_PLAYER][0].box), 19);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MAX_HP), hp + hp / 100);

    gSpecialVar_0x8006 = 4; // Searing Impact : il faut 15 points dans le tronc.
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

TEST("Mod: build menu gives a permanent variant to a damaging move only")
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    CreateRandomMonWithIVs(mon, SPECIES_CHARMANDER, 30, 0);
    SetMonMoveSlot(mon, MOVE_FLAMETHROWER, 0);
    SetMonMoveSlot(mon, MOVE_GROWL, 1);
    gSpecialVar_0x8004 = 0;
    BuildMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, TRUE);

    gSpecialVar_0x8007 = 1; // Growl
    VariantMenu_CheckMove();
    EXPECT_NE(gSpecialVar_Result, 0);

    gSpecialVar_0x8007 = 0; // Flamethrower
    VariantMenu_CheckMove();
    EXPECT_EQ(gSpecialVar_Result, 0);
    gSpecialVar_0x8008 = MOVE_VARIANT_VOLATILE;
    VariantMenu_Apply();
    EXPECT_EQ(GetBoxMonMoveVariant(&mon->box, 0), MOVE_VARIANT_VOLATILE);
    VariantMenu_CheckMove();
    EXPECT_NE(gSpecialVar_Result, 0);
}

// Remplit l'arbre Feu jusqu'au seuil de mutation, avec la maîtrise finale de la première branche.
static void TrainForMutation(struct Pokemon *mon)
{
    u32 node;

    mon->box.build.treeType = TYPE_FIRE;
    for (node = 0; node < TREE_TRUNK_NODES; node++)
        SetBoxMonTreeRank(&mon->box, node, 10);
    for (node = TREE_TRUNK_NODES; node < TREE_TRUNK_NODES + TREE_BRANCH_NODES - 1; node++)
        SetBoxMonTreeRank(&mon->box, node, 3);
    SetBoxMonTreeRank(&mon->box, TREE_TRUNK_NODES + TREE_BRANCH_NODES - 1, 1);
}

TEST("Mod: mutation menu needs the tree threshold and a final mastery")
{
    VarSet(VAR_MOD_MUTATION_TYPES, 0);
    VarSet(VAR_MOD_MUTATION_TYPES_2, 0);
    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 100, 0);
    gSpecialVar_0x8004 = 0;
    MutationMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 0);

    TrainForMutation(&gParties[B_TRAINER_PLAYER][0]);
    MutationMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(gSpecialVar_0x8006, TYPE_FIRE);

    // Type ouvert dès le départ : pas besoin de Mutation Seal.
    gSpecialVar_0x8007 = TYPE_WATER;
    MutationMenu_Check();
    EXPECT(gSpecialVar_Result);
    MutationMenu_Apply();
    EXPECT(gSpecialVar_Result);
    EXPECT_EQ(GetBoxMonMutatedType(&gParties[B_TRAINER_PLAYER][0].box, SPECIES_CHARMANDER, 0), TYPE_WATER);

    // Une seule mutation, pour toujours.
    MutationMenu_Begin();
    EXPECT_EQ(gSpecialVar_Result, 0);
}

TEST("Mod: a Mutation Seal opens a sealed type for good")
{
    VarSet(VAR_MOD_MUTATION_TYPES, 0);
    VarSet(VAR_MOD_MUTATION_TYPES_2, 0);
    RemoveBagItem(ITEM_MUTATION_SEAL, CountTotalItemQuantityInBag(ITEM_MUTATION_SEAL));
    EXPECT(IsMutationTypeOpen(TYPE_GRASS));
    EXPECT(!IsMutationTypeOpen(TYPE_DRAGON));
    EXPECT(!IsMutationTypeOpen(TYPE_FAIRY));
    EXPECT(!IsMutationTypeOpen(TYPE_STELLAR));

    CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHARMANDER, 100, 0);
    TrainForMutation(&gParties[B_TRAINER_PLAYER][0]);
    gSpecialVar_0x8004 = 0;
    MutationMenu_Begin();
    gSpecialVar_0x8007 = TYPE_DRAGON;
    MutationMenu_Check();
    EXPECT(!gSpecialVar_Result);

    AddBagItem(ITEM_MUTATION_SEAL, 1);
    MutationMenu_Check();
    EXPECT(gSpecialVar_Result);
    MutationMenu_Apply();
    EXPECT(gSpecialVar_Result);
    EXPECT(!CheckBagHasItem(ITEM_MUTATION_SEAL, 1));
    EXPECT(IsMutationTypeOpen(TYPE_DRAGON));
    EXPECT_EQ(GetBoxMonMutatedType(&gParties[B_TRAINER_PLAYER][0].box, SPECIES_CHARMANDER, 0), TYPE_DRAGON);

    // Les types au-delà de 15 sont gardés dans la seconde variable.
    OpenMutationType(TYPE_FAIRY);
    EXPECT(IsMutationTypeOpen(TYPE_FAIRY));
}
