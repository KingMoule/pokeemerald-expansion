#include "global.h"
#include "pokemon_build.h"
#include "test/battle.h"

// Exemple du wiki : Feu/Combat devient Plante/Combat (le type Combat est conservé).

TEST("Mod: a mutation replaces the chosen type of a dual-type species")
{
    struct Pokemon mon;
    u32 mutationType = TYPE_GRASS, slot = 0;
    ASSUME(GetSpeciesType(SPECIES_COMBUSKEN, 0) == TYPE_FIRE);
    ASSUME(GetSpeciesType(SPECIES_COMBUSKEN, 1) == TYPE_FIGHTING);
    CreateRandomMonWithIVs(&mon, SPECIES_COMBUSKEN, 20, 0);
    SetMonData(&mon, MON_DATA_MUTATION_TYPE, &mutationType);
    SetMonData(&mon, MON_DATA_MUTATION_SLOT, &slot);
    EXPECT_EQ(GetBoxMonMutatedType(&mon.box, SPECIES_COMBUSKEN, 0), TYPE_GRASS);
    EXPECT_EQ(GetBoxMonMutatedType(&mon.box, SPECIES_COMBUSKEN, 1), TYPE_FIGHTING);
    EXPECT_EQ(GetBoxMonMutationPenaltyType(&mon.box, SPECIES_COMBUSKEN), TYPE_FIGHTING);
}

TEST("Mod: a mutation replaces the only type of a single-type species")
{
    struct Pokemon mon;
    u32 mutationType = TYPE_WATER, slot = 0;
    ASSUME(GetSpeciesType(SPECIES_CHARMANDER, 0) == TYPE_FIRE);
    ASSUME(GetSpeciesType(SPECIES_CHARMANDER, 1) == TYPE_FIRE);
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 20, 0);
    SetMonData(&mon, MON_DATA_MUTATION_TYPE, &mutationType);
    SetMonData(&mon, MON_DATA_MUTATION_SLOT, &slot);
    EXPECT_EQ(GetBoxMonMutatedType(&mon.box, SPECIES_CHARMANDER, 0), TYPE_WATER);
    EXPECT_EQ(GetBoxMonMutatedType(&mon.box, SPECIES_CHARMANDER, 1), TYPE_WATER);
    EXPECT_EQ(GetBoxMonMutationPenaltyType(&mon.box, SPECIES_CHARMANDER), TYPE_WATER);
}

SINGLE_BATTLE_TEST("Mod: mutation adds x1.25 to weaknesses of the kept type (Psychic x2 -> x2.5)", s16 damage)
{
    bool32 mutated;
    PARAMETRIZE { mutated = FALSE; }
    PARAMETRIZE { mutated = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_COMBUSKEN) { if (mutated) Mutation(TYPE_GRASS, 0); }
    } WHEN {
        TURN { MOVE(player, MOVE_CONFUSION); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.25), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: mutation turns a x4 weakness into x5 (Flying vs Grass/Fighting)", s16 damage)
{
    bool32 mutated;
    PARAMETRIZE { mutated = FALSE; }
    PARAMETRIZE { mutated = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_COMBUSKEN) { if (mutated) Mutation(TYPE_GRASS, 0); }
    } WHEN {
        TURN { MOVE(player, MOVE_PECK); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        // Normal : Feu/Combat subit x2. Muté : Plante/Combat subit x4, puis x1,25.
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(2.5), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: weaknesses of the new type of a dual-type mon are not penalized (Fire vs Grass)", s16 damage)
{
    bool32 mutated;
    PARAMETRIZE { mutated = FALSE; }
    PARAMETRIZE { mutated = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_COMBUSKEN) { if (mutated) Mutation(TYPE_GRASS, 0); }
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        // Normal : Feu/Combat subit x0,5. Muté : Plante/Combat subit x2, sans pénalité.
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(4.0), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: a single-type mon is penalized on the weaknesses of its new type", s16 damage)
{
    bool32 mutated;
    PARAMETRIZE { mutated = FALSE; }
    PARAMETRIZE { mutated = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CHARMANDER) { if (mutated) Mutation(TYPE_WATER, 0); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDER_SHOCK); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        // Normal : Feu subit x1. Muté : Eau subit x2, puis x1,25.
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(2.5), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: STAB follows the mutated type", s16 damage)
{
    bool32 mutated;
    PARAMETRIZE { mutated = FALSE; }
    PARAMETRIZE { mutated = TRUE; }
    GIVEN {
        PLAYER(SPECIES_COMBUSKEN) { if (mutated) Mutation(TYPE_GRASS, 0); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_VINE_WHIP); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.5), results[1].damage);
    }
}
