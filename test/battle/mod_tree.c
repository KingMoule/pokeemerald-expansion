#include "global.h"
#include "pokemon_build.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Mod: Physical Strength adds 1% physical damage per rank", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 0, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Physical Strength does not boost special moves", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 0, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: tree damage bonuses add up (Physical Strength + Searing Impact)", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 0, rank); TreeRank(TYPE_FIRE, 4, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_FIRE_PUNCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.4), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Furnace boosts Fire moves below half HP", s16 damage)
{
    u32 hp;
    PARAMETRIZE { hp = 100; }
    PARAMETRIZE { hp = 40; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 6, 10); MaxHP(100); HP(hp); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Ash Hunter boosts damage against a burned target", s16 damage)
{
    u32 status;
    PARAMETRIZE { status = STATUS1_NONE; }
    PARAMETRIZE { status = STATUS1_BURN; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 10, 10); }
        OPPONENT(SPECIES_WOBBUFFET) { Status1(status); }
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Radiance boosts Fire moves in the sun", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 12, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); }
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Ember Body reduces recoil of Fire moves", s16 recoil)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 5, rank); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(9999); MaxHP(9999); }
    } WHEN {
        TURN { MOVE(player, MOVE_FLARE_BLITZ); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player, captureDamage: &results[i].recoil);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].recoil, Q_4_12(0.7), results[1].recoil);
    }
}

SINGLE_BATTLE_TEST("Mod: Lingering Spark raises the burn chance of Ember (10% -> 20% at rank 20)")
{
    PASSES_RANDOMLY(20, 100, RNG_SECONDARY_EFFECT);
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 8, 20); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent);
        STATUS_ICON(opponent, burn: TRUE);
    }
}

SINGLE_BATTLE_TEST("Mod: Long Summer adds turns to the sun summoned by the Pokémon")
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 5; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 13, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); }
        TURN {}
        TURN {}
        TURN {}
        TURN {}
    } SCENE {
        if (rank == 0)
            MESSAGE("The sunlight faded.");
        else
            NOT MESSAGE("The sunlight faded.");
    }
}

SINGLE_BATTLE_TEST("Mod: Vital Warmth heals at the end of each turn in the sun")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 14, 10); MaxHP(200); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SUNNY_DAY); }
    } SCENE {
        MESSAGE("Charmander restored HP using the weather!");
        HP_BAR(player, damage: -10);
    }
}
