#include "global.h"
#include "daycare.h"
#include "event_data.h"
#include "item.h"
#include "pokemon_build.h"
#include "test/overworld_script.h"
#include "test/test.h"

#define PARENT(i) (&gParties[B_TRAINER_PLAYER][i].box)
#define EGG       (&gParties[B_TRAINER_PLAYER][0].box)

// Comme dans test/daycare.c : les parents sont rangés directement dans la pension.
#define STORE_PARENTS_AND_GET_EGG()                                                              \
    StorePokemonInDaycare(&gParties[B_TRAINER_PLAYER][0], &gSaveBlock1Ptr->daycare.mons[0]);     \
    StorePokemonInDaycare(&gParties[B_TRAINER_PLAYER][0], &gSaveBlock1Ptr->daycare.mons[1]);     \
    RUN_OVERWORLD_SCRIPT( special GiveEggFromDaycare; );

TEST("Mod: two parents with the same mutation always pass it on, without tree points")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 50, gender=MON_MALE;
        givemon SPECIES_CHARMANDER, 50, gender=MON_FEMALE;
    );
    EXPECT(SetBoxMonMutation(PARENT(0), TYPE_FIRE, TYPE_WATER));
    EXPECT(SetBoxMonMutation(PARENT(1), TYPE_FIRE, TYPE_WATER));
    SetBoxMonTreeRank(PARENT(0), 0, 10);
    SET_RNG(RNG_DAYCARE_MUTATION_INHERITANCE, FALSE);
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MUTATION_TYPE), TYPE_WATER);
    EXPECT_EQ(GetBoxMonMutatedType(EGG, SPECIES_CHARMANDER, 0), TYPE_WATER);
    EXPECT_EQ(GetBoxMonTreeRank(EGG, 0), 0);
}

TEST("Mod: a single mutated parent passes its mutation on only by chance")
{
    bool32 inherited;
    PARAMETRIZE { inherited = FALSE; }
    PARAMETRIZE { inherited = TRUE; }
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 50, gender=MON_MALE;
        givemon SPECIES_CHARMANDER, 50, gender=MON_FEMALE;
    );
    EXPECT(SetBoxMonMutation(PARENT(0), TYPE_FIRE, TYPE_WATER));
    SET_RNG(RNG_DAYCARE_MUTATION_INHERITANCE, inherited);
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MUTATION_TYPE), inherited ? TYPE_WATER : TYPE_NONE);
}

TEST("Mod: the Lineage Charm guarantees the mutation, even on the other parent")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 50, gender=MON_MALE;
        givemon SPECIES_CHARMANDER, 50, gender=MON_FEMALE, item=ITEM_LINEAGE_CHARM;
    );
    EXPECT(SetBoxMonMutation(PARENT(0), TYPE_FIRE, TYPE_WATER));
    SET_RNG(RNG_DAYCARE_MUTATION_INHERITANCE, FALSE);
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MUTATION_TYPE), TYPE_WATER);
}

TEST("Mod: with two different mutations, the Lineage Charm holder's one is passed on")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_CHARMANDER, 50, gender=MON_MALE;
        givemon SPECIES_CHARMANDER, 50, gender=MON_FEMALE, item=ITEM_LINEAGE_CHARM;
    );
    EXPECT(SetBoxMonMutation(PARENT(0), TYPE_FIRE, TYPE_WATER));
    EXPECT(SetBoxMonMutation(PARENT(1), TYPE_FIRE, TYPE_GRASS));
    SET_RNG(RNG_DAYCARE_MUTATION_PICK, 0);
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MUTATION_TYPE), TYPE_GRASS);
}

TEST("Mod: a mutation is not passed on when the baby lacks the replaced type")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_BULBASAUR, 50, gender=MON_FEMALE;
        givemon SPECIES_DITTO, 50, item=ITEM_LINEAGE_CHARM;
    );
    EXPECT(SetBoxMonMutation(PARENT(1), TYPE_NORMAL, TYPE_FIRE));
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_BULBASAUR);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MUTATION_TYPE), TYPE_NONE);
}

TEST("Mod: a mutation replaces the matching type of a dual-type baby")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_BULBASAUR, 50, gender=MON_MALE;
        givemon SPECIES_BULBASAUR, 50, gender=MON_FEMALE, item=ITEM_LINEAGE_CHARM;
    );
    EXPECT(SetBoxMonMutation(PARENT(1), TYPE_POISON, TYPE_FIRE));
    STORE_PARENTS_AND_GET_EGG();

    EXPECT_EQ(GetBoxMonMutatedType(EGG, SPECIES_BULBASAUR, 0), TYPE_GRASS);
    EXPECT_EQ(GetBoxMonMutatedType(EGG, SPECIES_BULBASAUR, 1), TYPE_FIRE);
}
