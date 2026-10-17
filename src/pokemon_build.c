#include "global.h"
#include "pokemon.h"
#include "pokemon_build.h"

#define TREE_RANK_MAX ((1 << MOD_TREE_RANK_BITS) - 1)
#define MOVE_VARIANT_MAX ((1 << MOD_MOVE_VARIANT_BITS) - 1)

STATIC_ASSERT(sizeof(struct PokemonBuild) == 16, PokemonBuildSize);
STATIC_ASSERT(NUMBER_OF_MON_TYPES <= 32, PokemonBuildTypeBits);
STATIC_ASSERT(MOD_MOVE_VARIANT_BITS * MAX_MON_MOVES <= 8, PokemonBuildMoveVariantBits);

// Les rangs sont compactés à MOD_TREE_RANK_BITS bits par nœud, bit de poids faible en premier.
u32 GetBoxMonTreeRank(const struct BoxPokemon *boxMon, u32 node)
{
    u32 bit, rank = 0, i;

    if (node >= MOD_TREE_NODE_COUNT)
        return 0;

    bit = node * MOD_TREE_RANK_BITS;
    for (i = 0; i < MOD_TREE_RANK_BITS; i++, bit++)
    {
        if (boxMon->build.treeRanks[bit / 8] & (1 << (bit % 8)))
            rank |= 1 << i;
    }
    return rank;
}

void SetBoxMonTreeRank(struct BoxPokemon *boxMon, u32 node, u32 rank)
{
    u32 bit, i;

    if (node >= MOD_TREE_NODE_COUNT)
        return;
    if (rank > TREE_RANK_MAX)
        rank = TREE_RANK_MAX;

    bit = node * MOD_TREE_RANK_BITS;
    for (i = 0; i < MOD_TREE_RANK_BITS; i++, bit++)
    {
        if (rank & (1 << i))
            boxMon->build.treeRanks[bit / 8] |= 1 << (bit % 8);
        else
            boxMon->build.treeRanks[bit / 8] &= ~(1 << (bit % 8));
    }
}

u32 GetBoxMonTreePointsSpent(const struct BoxPokemon *boxMon)
{
    u32 node, total = 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        total += GetBoxMonTreeRank(boxMon, node);
    return total;
}

u32 GetBoxMonMoveVariant(const struct BoxPokemon *boxMon, u32 moveSlot)
{
    if (moveSlot >= MAX_MON_MOVES)
        return 0;
    return (boxMon->build.moveVariants >> (moveSlot * MOD_MOVE_VARIANT_BITS)) & MOVE_VARIANT_MAX;
}

void SetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 moveSlot, u32 variant)
{
    u32 shift;

    if (moveSlot >= MAX_MON_MOVES)
        return;
    if (variant > MOVE_VARIANT_MAX)
        variant = MOVE_VARIANT_MAX;

    shift = moveSlot * MOD_MOVE_VARIANT_BITS;
    boxMon->build.moveVariants &= ~(MOVE_VARIANT_MAX << shift);
    boxMon->build.moveVariants |= variant << shift;
}
