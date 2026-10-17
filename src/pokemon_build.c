#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "pokemon.h"
#include "pokemon_build.h"

#define TREE_RANK_MAX ((1 << MOD_TREE_RANK_BITS) - 1)
#define MOVE_VARIANT_MAX ((1 << MOD_MOVE_VARIANT_BITS) - 1)

STATIC_ASSERT(sizeof(struct PokemonBuild) == 16, PokemonBuildSize);
STATIC_ASSERT(NUMBER_OF_MON_TYPES <= 32, PokemonBuildTypeBits);
STATIC_ASSERT(MOD_MOVE_VARIANT_BITS * MAX_MON_MOVES <= 8, PokemonBuildMoveVariantBits);
STATIC_ASSERT(MOD_EV_CAP_PER_STAT <= 255, EvCapPerStatFitsInU8);
STATIC_ASSERT(MOD_EV_CAP_TOTAL <= MOD_EV_CAP_PER_STAT * NUM_STATS, EvCapTotalReachable);

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

// Une mutation remplace le type mutationSlot de l'espèce actuelle.
// Un Pokémon à un seul type prend le nouveau type comme unique type.
enum Type GetBoxMonMutatedType(const struct BoxPokemon *boxMon, enum Species species, u32 slot)
{
    enum Type type1 = GetSpeciesType(species, 0);
    enum Type type2 = GetSpeciesType(species, 1);
    enum Type mutationType = boxMon->build.mutationType;

    if (mutationType == TYPE_NONE)
        return slot == 0 ? type1 : type2;
    if (type1 == type2 || slot == boxMon->build.mutationSlot)
        return mutationType;
    return slot == 0 ? type1 : type2;
}

// Double type : la pénalité porte sur les faiblesses du type conservé.
// Type unique : elle porte sur les faiblesses du nouveau type.
enum Type GetBoxMonMutationPenaltyType(const struct BoxPokemon *boxMon, enum Species species)
{
    enum Type type1 = GetSpeciesType(species, 0);
    enum Type type2 = GetSpeciesType(species, 1);

    if (boxMon->build.mutationType == TYPE_NONE)
        return TYPE_NONE;
    if (type1 == type2)
        return boxMon->build.mutationType;
    return boxMon->build.mutationSlot == 0 ? type2 : type1;
}

static const struct BoxPokemon *GetBattlerBoxMon(enum BattlerId battler)
{
    return &GetBattlerParty(battler)[gBattlerPartyIndexes[battler]].box;
}

enum Type GetBattlerMutatedType(enum BattlerId battler, u32 slot)
{
    return GetBoxMonMutatedType(GetBattlerBoxMon(battler), gBattleMons[battler].species, slot);
}

// ×1,25 lorsque l'attaque est super efficace contre le type pénalisé et
// que l'efficacité totale reste supérieure à ×1. Si l'autre type compense
// la faiblesse, aucune pénalité (cas encore à définir dans le wiki).
uq4_12_t ApplyMutationWeaknessPenalty(enum BattlerId battlerDef, enum Type moveType, uq4_12_t modifier)
{
    enum Type penaltyType;

    if (gBattleMons[battlerDef].volatiles.transformed)
        return modifier;

    penaltyType = GetBoxMonMutationPenaltyType(GetBattlerBoxMon(battlerDef), gBattleMons[battlerDef].species);
    if (penaltyType == TYPE_NONE || !IS_BATTLER_OF_TYPE(battlerDef, penaltyType))
        return modifier;
    if (GetTypeModifier(moveType, penaltyType) <= UQ_4_12(1.0) || modifier <= UQ_4_12(1.0))
        return modifier;

    return uq4_12_multiply(modifier, UQ_4_12((double)MOD_MUTATION_WEAKNESS_NUM / MOD_MUTATION_WEAKNESS_DEN));
}
