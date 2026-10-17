#include "global.h"
#include "item.h"
#include "item_use.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "test/test.h"

static void GiveMoves(struct Pokemon *mon, enum Move m1, enum Move m2, enum Move m3, enum Move m4)
{
    SetMonMoveSlot(mon, m1, 0);
    SetMonMoveSlot(mon, m2, 1);
    SetMonMoveSlot(mon, m3, 2);
    SetMonMoveSlot(mon, m4, 3);
}

TEST("Mod: move variants change max PP and are permanent")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 30, 0);
    GiveMoves(&mon, MOVE_FLAMETHROWER, MOVE_EMBER, MOVE_TACKLE, MOVE_GROWL);

    EXPECT(TrySetBoxMonMoveVariant(&mon.box, 0, MOVE_VARIANT_FOCUSED));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1), 15 * MOD_VARIANT_FOCUSED_PP / 100);
    EXPECT_EQ(CalculateMonMovePP(&mon, MOVE_FLAMETHROWER, 0, 0), 15 * MOD_VARIANT_FOCUSED_PP / 100);
    EXPECT(!TrySetBoxMonMoveVariant(&mon.box, 0, MOVE_VARIANT_THRIFTY));

    EXPECT(TrySetBoxMonMoveVariant(&mon.box, 1, MOVE_VARIANT_THRIFTY));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP2), 25); // Les PP actuels ne montent pas d'eux-mêmes...
    MonRestorePP(&mon);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP2), 25 * MOD_VARIANT_THRIFTY_PP / 100); // ...mais le soin les remplit.
}

TEST("Mod: only damaging moves have variants, and Volatile needs a chance effect")
{
    EXPECT(CanMoveHaveVariant(MOVE_FLAMETHROWER, MOVE_VARIANT_VOLATILE));
    EXPECT(CanMoveHaveVariant(MOVE_TACKLE, MOVE_VARIANT_FOCUSED));
    EXPECT(!CanMoveHaveVariant(MOVE_TACKLE, MOVE_VARIANT_VOLATILE));
    EXPECT(!CanMoveHaveVariant(MOVE_GROWL, MOVE_VARIANT_FOCUSED));
    EXPECT(!CanMoveHaveVariant(MOVE_GROWL, MOVE_VARIANT_THRIFTY));
}

TEST("Mod: a move variant is lost when its move is replaced or forgotten")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 30, 0);
    GiveMoves(&mon, MOVE_FLAMETHROWER, MOVE_EMBER, MOVE_TACKLE, MOVE_SCRATCH);
    SetBoxMonMoveVariant(&mon.box, 0, MOVE_VARIANT_FOCUSED);
    SetBoxMonMoveVariant(&mon.box, 2, MOVE_VARIANT_THRIFTY);

    // Remplacement par un nouvel apprentissage.
    RemoveMonPPBonus(&mon, 2);
    SetMonMoveSlot(&mon, MOVE_FIRE_FANG, 2);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 2), MOVE_VARIANT_STANDARD);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_FOCUSED);

    // L'oubli de la première attaque décale les autres avec leurs variantes.
    SetBoxMonMoveVariant(&mon.box, 1, MOVE_VARIANT_VOLATILE);
    DeleteFirstMoveAndGiveMoveToMon(&mon, MOVE_SLASH);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE1), MOVE_EMBER);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_VOLATILE);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 3), MOVE_VARIANT_STANDARD);
}

TEST("Mod: move variants follow their move when moves are swapped")
{
    struct Pokemon mon;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARMANDER, 30, 0);
    SetBoxMonMoveVariant(&mon.box, 0, MOVE_VARIANT_FOCUSED);
    SwapBoxMonMoveVariants(&mon.box, 0, 3);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_STANDARD);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 3), MOVE_VARIANT_FOCUSED);
}

TEST("Mod: the Mastery Scroll is a party-use item")
{
    EXPECT_EQ(GetItemPocket(ITEM_MASTERY_SCROLL), POCKET_ITEMS);
    EXPECT_EQ(GetItemType(ITEM_MASTERY_SCROLL), ITEM_USE_PARTY_MENU);
    EXPECT(GetItemFieldFunc(ITEM_MASTERY_SCROLL) == ItemUseOutOfBattle_MasteryScroll);
}
