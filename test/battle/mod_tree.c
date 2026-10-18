#include "global.h"
#include "pokemon_build.h"
#include "test/battle.h"

// Indices des nœuds : General 0-3, Offense 4-7, Defense 8-11, Utility 12-15.
#define GENERAL_PHYSICAL 0
#define GENERAL_SPECIAL  1
#define OFFENSE(n)       (TREE_NODE_OFFENSE + (n))
#define DEFENSE(n)       (TREE_NODE_DEFENSE + (n))
#define UTILITY(n)       (TREE_NODE_UTILITY + (n))

// ---------------------------------------------------------------------------------------------
// General et Offense

SINGLE_BATTLE_TEST("Mod: Physical Strength adds 1% physical damage per rank", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, GENERAL_PHYSICAL, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Physical Strength does not boost special moves", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, GENERAL_PHYSICAL, rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: tree damage bonuses add up (Physical Strength + Fire Power)", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 1; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, GENERAL_PHYSICAL, rank * 10); TreeRank(TYPE_FIRE, OFFENSE(0), rank * 20); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_FIRE_PUNCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.3), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Blaze boosts Fire moves below half HP", s16 damage)
{
    u32 hp;
    PARAMETRIZE { hp = 100; }
    PARAMETRIZE { hp = 40; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, OFFENSE(1), 10); MaxHP(100); HP(hp); }
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
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, OFFENSE(2), 10); }
        OPPONENT(SPECIES_WOBBUFFET) { Status1(status); }
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Venomous boosts damage against poisoned targets", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_EKANS) { TreeRank(TYPE_POISON, OFFENSE(1), rank); }
        OPPONENT(SPECIES_WOBBUFFET) { Status1(STATUS1_POISON); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Rain Power boosts Water moves in the rain", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, UTILITY(0), rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_RAIN_DANCE); }
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Iron Fist boosts punching moves", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { TreeRank(TYPE_STEEL, OFFENSE(2), rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_MEGA_PUNCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.2), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Rock Head reduces recoil of all its moves", s16 recoil)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_GEODUDE) { Ability(ABILITY_STURDY); TreeRank(TYPE_ROCK, OFFENSE(1), rank); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(9999); MaxHP(9999); }
    } WHEN {
        TURN { MOVE(player, MOVE_DOUBLE_EDGE); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player, captureDamage: &results[i].recoil);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].recoil, Q_4_12(0.5), results[1].recoil);
    }
}

SINGLE_BATTLE_TEST("Mod: Deep Roots raises the HP drained by its moves", s16 heal)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_BULBASAUR) { TreeRank(TYPE_GRASS, OFFENSE(2), rank); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(9999); MaxHP(9999); }
    } WHEN {
        TURN { MOVE(player, MOVE_GIGA_DRAIN); }
    } SCENE {
        HP_BAR(opponent);
        HP_BAR(player, captureDamage: &results[i].heal);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].heal, Q_4_12(1.5), results[1].heal);
    }
}

SINGLE_BATTLE_TEST("Mod: Incandescent Heart stacks Fire damage after each Fire hit, at a cost", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 1; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, OFFENSE(3), rank); }
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

SINGLE_BATTLE_TEST("Mod: Keen Edge adds a critical-hit stage to Rock moves")
{
    PASSES_RANDOMLY(1, 8, RNG_CRITICAL_HIT);
    GIVEN {
        WITH_CONFIG(B_CRIT_CHANCE, GEN_7);
        PLAYER(SPECIES_GEODUDE) { TreeRank(TYPE_ROCK, OFFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_ROCK_THROW); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_ROCK_THROW, player);
        MESSAGE("A critical hit!");
    }
}

SINGLE_BATTLE_TEST("Mod: Refrigerate turns Normal moves into Ice moves")
{
    GIVEN {
        PLAYER(SPECIES_SNORUNT) { TreeRank(TYPE_ICE, OFFENSE(3), 1); }
        OPPONENT(SPECIES_DRATINI);
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        MESSAGE("It's super effective!");
    }
}

SINGLE_BATTLE_TEST("Mod: Scrappy lets Normal moves hit Ghost types")
{
    GIVEN {
        PLAYER(SPECIES_RATTATA) { TreeRank(TYPE_NORMAL, OFFENSE(3), 1); }
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Mod: Grounding Blow lets Ground moves hit Flying types")
{
    GIVEN {
        PLAYER(SPECIES_SANDSHREW) { TreeRank(TYPE_GROUND, OFFENSE(3), 1); }
        OPPONENT(SPECIES_PIDGEY);
    } WHEN {
        TURN { MOVE(player, MOVE_MUD_SLAP); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MUD_SLAP, player);
        HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Mod: Gale Wings gives priority to Flying moves at full HP")
{
    GIVEN {
        PLAYER(SPECIES_PIDGEY) { TreeRank(TYPE_FLYING, OFFENSE(3), 1); Speed(10); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_GUST); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GUST, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
    }
}

SINGLE_BATTLE_TEST("Mod: Swift Swim raises Speed in the rain")
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, UTILITY(2), rank); Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(110); }
    } WHEN {
        TURN { MOVE(player, MOVE_RAIN_DANCE); }
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_RAIN_DANCE, player);
        if (rank == 0)
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
        }
        else
        {
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, player);
            ANIMATION(ANIM_TYPE_MOVE, MOVE_CELEBRATE, opponent);
        }
    }
}

SINGLE_BATTLE_TEST("Mod: Moxie raises its best attacking stat after a KO")
{
    GIVEN {
        PLAYER(SPECIES_POOCHYENA) { TreeRank(TYPE_DARK, OFFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_BITE); SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet fainted!");
        MESSAGE("Poochyena's Moxie activated!");
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Mod: Soul Siphon drains a quarter of the damage of its Ghost moves", s16 damage, s16 heal)
{
    PARAMETRIZE { }
    GIVEN {
        PLAYER(SPECIES_GASTLY) { TreeRank(TYPE_GHOST, OFFENSE(3), 1); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SHADOW_BALL); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
        MESSAGE("Gastly restored HP with Soul Siphon!");
        HP_BAR(player, captureDamage: &results[i].heal);
    } FINALLY {
        EXPECT_EQ(results[0].heal, -results[0].damage / 4);
    }
}

// ---------------------------------------------------------------------------------------------
// Defense

SINGLE_BATTLE_TEST("Mod: Steam Veil reduces Water damage taken", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, DEFENSE(0), rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.75), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Thick Bark reduces super effective damage taken", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_BULBASAUR) { TreeRank(TYPE_GRASS, DEFENSE(0), rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.7), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Thick Bark does not reduce neutral damage", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 20; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_BULBASAUR) { TreeRank(TYPE_GRASS, DEFENSE(0), rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Unbreakable halves only the first super effective hit of the battle", s16 damage1, s16 damage2)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 1; }
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { TreeRank(TYPE_STEEL, DEFENSE(3), rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_EMBER, criticalHit: FALSE); }
        TURN { MOVE(opponent, MOVE_EMBER, criticalHit: FALSE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, opponent);
        HP_BAR(player, captureDamage: &results[i].damage1);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, opponent);
        HP_BAR(player, captureDamage: &results[i].damage2);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage1, Q_4_12(0.5), results[1].damage1);
        EXPECT_EQ(results[0].damage2, results[1].damage2);
    }
}

SINGLE_BATTLE_TEST("Mod: Multiscale reduces damage taken at full HP", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_DRATINI) { TreeRank(TYPE_DRAGON, DEFENSE(2), rank); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.7), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Mod: Sturdy survives a KO hit once at full HP")
{
    GIVEN {
        PLAYER(SPECIES_GEODUDE) { TreeRank(TYPE_ROCK, DEFENSE(3), 1); MaxHP(10); HP(10); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_WATER_GUN); }
    } SCENE {
        HP_BAR(player);
        MESSAGE("Geodude endured the hit!");
    } THEN {
        EXPECT_EQ(player->hp, 1);
    }
}

SINGLE_BATTLE_TEST("Mod: Magnet Rise makes the first Ground move fail")
{
    GIVEN {
        PLAYER(SPECIES_PIKACHU) { TreeRank(TYPE_ELECTRIC, DEFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_MUD_SLAP); }
        TURN { MOVE(opponent, MOVE_MUD_SLAP); }
    } SCENE {
        MESSAGE("It doesn't affect Pikachu…");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MUD_SLAP, opponent);
        HP_BAR(player);
    }
}

SINGLE_BATTLE_TEST("Mod: Dark Veil makes the first status move aimed at it fail")
{
    GIVEN {
        PLAYER(SPECIES_POOCHYENA) { TreeRank(TYPE_DARK, DEFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_GLARE); }
        TURN { MOVE(opponent, MOVE_GLARE); }
    } SCENE {
        MESSAGE("Poochyena avoided the attack!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GLARE, opponent);
        STATUS_ICON(player, paralysis: TRUE);
    }
}

SINGLE_BATTLE_TEST("Mod: Flame Body may burn attackers that touch it")
{
    PASSES_RANDOMLY(30, 100, RNG_TREE_CONTACT);
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, DEFENSE(2), 10); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
        MESSAGE("Charmander's Flame Body activated!");
        STATUS_ICON(opponent, burn: TRUE);
    }
}

SINGLE_BATTLE_TEST("Mod: Rough Skin hurts attackers that touch it")
{
    GIVEN {
        PLAYER(SPECIES_POOCHYENA) { TreeRank(TYPE_DARK, DEFENSE(2), 10); }
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(160); HP(160); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, opponent);
        MESSAGE("Poochyena's Rough Skin activated!");
        HP_BAR(opponent, damage: 20);
    }
}

SINGLE_BATTLE_TEST("Mod: Clear Body may ignore a stat drop from a foe")
{
    PASSES_RANDOMLY(50, 100, RNG_TREE_STAT_DROP_IGNORE);
    GIVEN {
        PLAYER(SPECIES_MAGNEMITE) { TreeRank(TYPE_STEEL, DEFENSE(2), 10); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_GROWL); }
    } SCENE {
        MESSAGE("Magnemite's Clear Body kept its stats from being lowered!");
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Mod: Misty Veil may avoid a status from a foe")
{
    PASSES_RANDOMLY(50, 100, RNG_TREE_STATUS_AVOID);
    GIVEN {
        PLAYER(SPECIES_CLEFAIRY) { TreeRank(TYPE_FAIRY, DEFENSE(1), 10); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_GLARE); }
    } SCENE {
        MESSAGE("Clefairy's Misty Veil kept it from being afflicted!");
    } THEN {
        EXPECT_EQ(player->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Mod: Synchronize passes a status back to the foe")
{
    GIVEN {
        PLAYER(SPECIES_DROWZEE) { TreeRank(TYPE_PSYCHIC, DEFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_GLARE); }
    } SCENE {
        STATUS_ICON(player, paralysis: TRUE);
        MESSAGE("Drowzee's Synchronize activated!");
        STATUS_ICON(opponent, paralysis: TRUE);
    }
}

SINGLE_BATTLE_TEST("Mod: Last Haunt hurts the attacker that knocks it out")
{
    GIVEN {
        PLAYER(SPECIES_GASTLY) { TreeRank(TYPE_GHOST, DEFENSE(3), 1); HP(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { MaxHP(200); HP(200); }
    } WHEN {
        TURN { MOVE(opponent, MOVE_BITE); SEND_OUT(player, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_BITE, opponent);
        MESSAGE("Gastly's Last Haunt activated!");
        HP_BAR(opponent, damage: 50);
    }
}

SINGLE_BATTLE_TEST("Mod: Toxic Debris sets Toxic Spikes after the first physical hit")
{
    GIVEN {
        PLAYER(SPECIES_EKANS) { TreeRank(TYPE_POISON, DEFENSE(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("Ekans's Toxic Debris activated!");
    } THEN {
        EXPECT_EQ((u32)gSideTimers[B_SIDE_OPPONENT].toxicSpikesAmount, 1);
    }
}

SINGLE_BATTLE_TEST("Mod: Rekindle heals once below a third of max HP")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, DEFENSE(3), 1); MaxHP(100); HP(30); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
        TURN { }
    } SCENE {
        MESSAGE("Charmander restored HP with Rekindle!");
        HP_BAR(player, damage: -25);
        NOT MESSAGE("Charmander restored HP with Rekindle!");
    }
}

SINGLE_BATTLE_TEST("Mod: Molt sheds its status once")
{
    GIVEN {
        PLAYER(SPECIES_CATERPIE) { TreeRank(TYPE_BUG, DEFENSE(3), 1); Status1(STATUS1_PARALYSIS); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Caterpie's Molt cured its status!");
    } THEN {
        EXPECT_EQ(player->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Mod: Aqua Ring wraps it in Aqua Ring below half HP")
{
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, DEFENSE(3), 1); MaxHP(100); HP(40); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Squirtle's Aqua Ring activated!");
    } THEN {
        EXPECT(player->volatiles.aquaRing);
    }
}

SINGLE_BATTLE_TEST("Mod: Mental Barrier reduces indirect damage", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_ABRA) { TreeRank(TYPE_PSYCHIC, DEFENSE(2), rank); MaxHP(160); HP(160); Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        HP_BAR(player, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[1].damage, results[0].damage / 2);
    }
}

SINGLE_BATTLE_TEST("Mod: Aerial Retreat heals when switching out")
{
    GIVEN {
        PLAYER(SPECIES_PIDGEY) { TreeRank(TYPE_FLYING, DEFENSE(2), 10); MaxHP(100); HP(50); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { SWITCH(player, 1); }
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 70);
    }
}

// ---------------------------------------------------------------------------------------------
// Utility

SINGLE_BATTLE_TEST("Mod: Lingering Spark raises the burn chance of Ember (10% -> 20% at rank 20)")
{
    PASSES_RANDOMLY(20, 100, RNG_SECONDARY_EFFECT);
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, UTILITY(0), 20); }
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
    PARAMETRIZE { rank = 3; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, UTILITY(1), rank); }
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

SINGLE_BATTLE_TEST("Mod: Fan the Flames boosts the damage of burns it inflicts", s16 damage)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { TreeRank(TYPE_FIRE, UTILITY(2), rank); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_WILL_O_WISP); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WILL_O_WISP, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[1].damage, results[0].damage + results[0].damage * 5000 / 10000);
    }
}

SINGLE_BATTLE_TEST("Mod: Ice Body heals at the end of each turn in the snow")
{
    GIVEN {
        PLAYER(SPECIES_SNORUNT) { TreeRank(TYPE_ICE, UTILITY(2), 10); MaxHP(200); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_SNOWSCAPE); }
    } SCENE {
        MESSAGE("Snorunt restored HP using the weather!");
        HP_BAR(player, damage: -10);
    }
}

SINGLE_BATTLE_TEST("Mod: Self-Repair heals at the end of each turn")
{
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { TreeRank(TYPE_STEEL, UTILITY(2), 10); MaxHP(200); HP(100); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Registeel restored HP with Self-Repair!");
        HP_BAR(player, damage: -10);
    }
}

SINGLE_BATTLE_TEST("Mod: Drizzle calls the rain on first entry")
{
    GIVEN {
        PLAYER(SPECIES_SQUIRTLE) { TreeRank(TYPE_WATER, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Squirtle's Drizzle activated!");
        MESSAGE("It started to rain!");
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_RAIN);
    }
}

SINGLE_BATTLE_TEST("Mod: Grassy Surge sets Grassy Terrain on first entry")
{
    GIVEN {
        PLAYER(SPECIES_BULBASAUR) { TreeRank(TYPE_GRASS, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Bulbasaur's Grassy Surge activated!");
    } THEN {
        EXPECT_EQ(gFieldTimers.terrain, B_TERRAIN_GRASSY);
    }
}

SINGLE_BATTLE_TEST("Mod: Stealth Rock mastery sets Stealth Rock on first entry")
{
    GIVEN {
        PLAYER(SPECIES_GEODUDE) { TreeRank(TYPE_ROCK, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Geodude's Stealth Rock activated!");
    } THEN {
        EXPECT(IsHazardOnSide(B_SIDE_OPPONENT, HAZARDS_STEALTH_ROCK));
    }
}

SINGLE_BATTLE_TEST("Mod: Shield Wall sets Reflect on first entry")
{
    GIVEN {
        PLAYER(SPECIES_REGISTEEL) { TreeRank(TYPE_STEEL, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Registeel's Shield Wall activated!");
    } THEN {
        EXPECT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_REFLECT);
    }
}

SINGLE_BATTLE_TEST("Mod: Herald of Winds sets Tailwind on first entry")
{
    GIVEN {
        PLAYER(SPECIES_PIDGEY) { TreeRank(TYPE_FLYING, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Pidgey's Herald of Winds activated!");
    } THEN {
        EXPECT(gSideStatuses[B_SIDE_PLAYER] & SIDE_STATUS_TAILWIND);
    }
}

SINGLE_BATTLE_TEST("Mod: Intimidate mastery lowers the foe's Attack on first entry")
{
    GIVEN {
        PLAYER(SPECIES_POOCHYENA) { TreeRank(TYPE_DARK, UTILITY(3), 1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { }
    } SCENE {
        MESSAGE("Poochyena's Intimidate activated!");
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
    }
}

SINGLE_BATTLE_TEST("Mod: Healing Aura boosts its healing moves", s16 heal)
{
    u32 rank;
    PARAMETRIZE { rank = 0; }
    PARAMETRIZE { rank = 10; }
    GIVEN {
        PLAYER(SPECIES_CLEFAIRY) { TreeRank(TYPE_FAIRY, UTILITY(2), rank); MaxHP(200); HP(1); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_RECOVER); }
    } SCENE {
        HP_BAR(player, captureDamage: &results[i].heal);
    } FINALLY {
        EXPECT_EQ(results[0].heal, -100);
        EXPECT_EQ(results[1].heal, -130);
    }
}
