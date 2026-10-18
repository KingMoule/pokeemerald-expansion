#include "global.h"
#include "pokemon_build.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Mod: Focused, Thrifty and Volatile variants change move power", s16 damage)
{
    u32 variant;
    PARAMETRIZE { variant = MOVE_VARIANT_STANDARD; }
    PARAMETRIZE { variant = MOVE_VARIANT_FOCUSED; }
    PARAMETRIZE { variant = MOVE_VARIANT_THRIFTY; }
    PARAMETRIZE { variant = MOVE_VARIANT_VOLATILE; }
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { Moves(MOVE_FLAMETHROWER); MoveVariant(0, variant); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_FLAMETHROWER); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        // La puissance de base est modifiée : les arrondis de la formule de dégâts laissent ±3 %.
        EXPECT_LE(abs(results[1].damage * 100 - results[0].damage * MOD_VARIANT_FOCUSED_POWER), results[0].damage * 3);
        EXPECT_LE(abs(results[2].damage * 100 - results[0].damage * MOD_VARIANT_THRIFTY_POWER), results[0].damage * 3);
        EXPECT_LE(abs(results[3].damage * 100 - results[0].damage * MOD_VARIANT_VOLATILE_POWER), results[0].damage * 3);
    }
}

SINGLE_BATTLE_TEST("Mod: the Volatile variant doubles the burn chance of Ember")
{
    PASSES_RANDOMLY(10 * MOD_VARIANT_VOLATILE_CHANCE / 100, 100, RNG_SECONDARY_EFFECT);
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { Moves(MOVE_EMBER); MoveVariant(0, MOVE_VARIANT_VOLATILE); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_EMBER); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_EMBER, player);
        STATUS_ICON(opponent, burn: TRUE);
    }
}

SINGLE_BATTLE_TEST("Mod: the Focused variant lowers max PP in battle")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { Moves(MOVE_FLAMETHROWER); MoveVariant(0, MOVE_VARIANT_FOCUSED); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_FLAMETHROWER); }
    } THEN {
        EXPECT_EQ(gBattleMons[B_BATTLER_0].pp[0], 15 * MOD_VARIANT_FOCUSED_PP / 100 - 1);
        EXPECT_EQ(CalculateBattlerMovePP(B_BATTLER_0, MOVE_FLAMETHROWER, 0, 0), 15 * MOD_VARIANT_FOCUSED_PP / 100);
    }
}

SINGLE_BATTLE_TEST("Mod: move variants do not apply to the opponent's copy of the move")
{
    GIVEN {
        PLAYER(SPECIES_CHARMANDER) { Moves(MOVE_FLAMETHROWER, MOVE_CELEBRATE); MoveVariant(0, MOVE_VARIANT_FOCUSED); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_FLAMETHROWER); }
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_FLAMETHROWER); }
    } THEN {
        EXPECT_EQ(GetBattlerMoveVariant(B_BATTLER_1, MOVE_FLAMETHROWER), MOVE_VARIANT_STANDARD);
        EXPECT_EQ(GetBattlerMoveVariant(B_BATTLER_0, MOVE_FLAMETHROWER), MOVE_VARIANT_FOCUSED);
    }
}
