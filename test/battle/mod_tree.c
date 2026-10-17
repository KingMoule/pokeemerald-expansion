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

SINGLE_BATTLE_TEST("Mod: Elemental Focus boosts special moves of the tree's type", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, 8, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Bulwark reduces super effective damage taken", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_MACHOP) { TreeRank(TYPE_FIGHTING, 12, rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_CONFUSION); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.9), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Bulwark does not reduce neutral damage", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_MACHOP) { TreeRank(TYPE_FIGHTING, 12, rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Toxin's Exploit boosts damage against poisoned targets", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_EKANS) { TreeRank(TYPE_POISON, 9, rank); }
        OPPONENT(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Incandescent Heart stacks Fire damage after each Fire hit, at a cost", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 1; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 7, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER, criticalHit: FALSE); }
        TURN { MOVE(player, MOVE_EMBER, criticalHit: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, player);
        if (rank == 1)
        {
            MESSAGE("Charmander's mastery grows stronger, at a cost!");
            HP_BAR(player);
        }
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Incandescent Heart does nothing without the mastery")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        NOT MESSAGE("Charmander's mastery grows stronger, at a cost!");
    }
}

SINGLE_BATTLE_TEST("Mod: Herald calls the tree's weather on first entry")
{
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, 15, 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Squirtle's mastery calls the weather!");
        MESSAGE("It started to rain!");
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_RAIN);
    }
}

SINGLE_BATTLE_TEST("Mod: Fan the Flames boosts the damage of burns it inflicts", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 9, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WILL_O_WISP); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WILL_O_WISP, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[1].damage, results[0].damage + results[0].damage * 200 / 1000);
    }
}

SINGLE_BATTLE_TEST("Mod: Stubborn Embers brings a cured burn back for one turn")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, 11, 1); }
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_RAWST_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_WILL_O_WISP); }
        TURN { }
        TURN { }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WILL_O_WISP, player);
        MESSAGE("The affliction clings to the opposing Wobbuffet!");
        STATUS_ICON(opponent, burn: TRUE);
        MESSAGE("The opposing Wobbuffet was hurt by its burn!");
        MESSAGE("The lingering affliction left the opposing Wobbuffet.");
        STATUS_ICON(opponent, none: TRUE);
        NOT MESSAGE("The opposing Wobbuffet was hurt by its burn!");
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Mod: Stubborn Embers does nothing without the mastery")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_RAWST_BERRY); }
    } WHEN {
        TURN { MOVE(player, MOVE_WILL_O_WISP); }
        TURN { }
    } SCENE {
        NOT MESSAGE("The affliction clings to the opposing Wobbuffet!");
    }
}

SINGLE_BATTLE_TEST("Mod: Piercing Focus adds a critical-hit stage to special moves of its type")
{
    PASSES_RANDOMLY(1, 8, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_7);
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, 11, 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, player);
        MESSAGE("A critical hit!");
    }
}

SINGLE_BATTLE_TEST("Mod: Unbreakable halves only the first super effective hit of the battle", s16 damage1, s16 damage2)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 1; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { TreeRank(TYPE_PSYCHIC, 15, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_BITE, criticalHit: FALSE); }
        TURN { MOVE(opponent, MOVE_BITE, criticalHit: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, opponent);
        HP_BAR(player, captureDamage: &results[i].damage1);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, opponent);
        HP_BAR(player, captureDamage: &results[i].damage2);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage1, Q_4_12(0.5), results[1].damage1);
        EXPECT_EQ(results[0].damage2, results[1].damage2);
    }
}
