#include "global.h"
#include "battle.h"
#include "battle_dynamax.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "random.h"
#include "battle_scripts.h"
#include "battle_message.h"
#include "battle_script_commands.h"
#include "battle_stat_change.h"
#include "string_util.h"
#include "pokemon_build.h"
#include "constants/battle.h"
#include "constants/battle_set_effect.h"
#include "constants/characters.h"

#include "data/pokemon_trees.h"

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

void SwapBoxMonMoveVariants(struct BoxPokemon *boxMon, u32 slot1, u32 slot2)
{
    u32 variant1 = GetBoxMonMoveVariant(boxMon, slot1);
    SetBoxMonMoveVariant(boxMon, slot1, GetBoxMonMoveVariant(boxMon, slot2));
    SetBoxMonMoveVariant(boxMon, slot2, variant1);
}

static bool32 MoveHasChanceEffect(enum Move move)
{
    u32 i;
    for (i = 0; i < GetMoveAdditionalEffectCount(move); i++)
    {
        if (GetMoveAdditionalEffectById(move, i)->chance != 0)
            return TRUE;
    }
    return FALSE;
}

// Seules les attaques offensives ont des variantes. Incendiaire exige un effet secondaire aléatoire.
bool32 CanMoveHaveVariant(enum Move move, enum MoveVariant variant)
{
    if (move == MOVE_NONE || GetMovePower(move) == 0 || IsBattleMoveStatus(move))
        return FALSE;
    switch (variant)
    {
    case MOVE_VARIANT_FOCUSED:
    case MOVE_VARIANT_THRIFTY:
        return TRUE;
    case MOVE_VARIANT_VOLATILE:
        return MoveHasChanceEffect(move);
    default:
        return FALSE;
    }
}

const u8 *GetMoveVariantName(enum MoveVariant variant)
{
    static const u8 *const sNames[MOVE_VARIANT_COUNT] =
    {
        [MOVE_VARIANT_STANDARD] = COMPOUND_STRING("Standard"),
        [MOVE_VARIANT_FOCUSED]  = COMPOUND_STRING("Focused"),
        [MOVE_VARIANT_THRIFTY]  = COMPOUND_STRING("Thrifty"),
        [MOVE_VARIANT_VOLATILE] = COMPOUND_STRING("Volatile"),
    };
    if (variant >= MOVE_VARIANT_COUNT)
        variant = MOVE_VARIANT_STANDARD;
    return sNames[variant];
}

u32 ApplyMoveVariantToPP(u32 pp, enum MoveVariant variant)
{
    u32 percent;

    switch (variant)
    {
    case MOVE_VARIANT_FOCUSED:
        percent = MOD_VARIANT_FOCUSED_PP;
        break;
    case MOVE_VARIANT_THRIFTY:
        percent = MOD_VARIANT_THRIFTY_PP;
        break;
    default:
        return pp;
    }
    if (pp == 0)
        return 0;
    pp = pp * percent / 100;
    return min(max(pp, 1), 255);
}

u32 CalculateBoxMonMovePP(const struct BoxPokemon *boxMon, enum Move move, u32 ppBonuses, u32 slot)
{
    return ApplyMoveVariantToPP(CalculatePPWithBonus(move, ppBonuses, slot), GetBoxMonMoveVariant(boxMon, slot));
}

u32 CalculateMonMovePP(struct Pokemon *mon, enum Move move, u32 ppBonuses, u32 slot)
{
    return CalculateBoxMonMovePP(&mon->box, move, ppBonuses, slot);
}

// Choix définitif : refusé si l'attaque a déjà une variante ou ne peut pas l'avoir.
// Les PP actuels sont ramenés sous le nouveau maximum.
bool32 TrySetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 slot, enum MoveVariant variant)
{
    enum Move move;
    u32 pp, maxPP;

    if (slot >= MAX_MON_MOVES || GetBoxMonMoveVariant(boxMon, slot) != MOVE_VARIANT_STANDARD)
        return FALSE;
    move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + slot);
    if (!CanMoveHaveVariant(move, variant))
        return FALSE;

    SetBoxMonMoveVariant(boxMon, slot, variant);
    maxPP = CalculateBoxMonMovePP(boxMon, move, GetBoxMonData(boxMon, MON_DATA_PP_BONUSES), slot);
    pp = GetBoxMonData(boxMon, MON_DATA_PP1 + slot);
    if (pp > maxPP)
        SetBoxMonData(boxMon, MON_DATA_PP1 + slot, &maxPP);
    return TRUE;
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

// L'IA simule parfois un autre Pokémon de l'équipe dans gBattleMons sans changer
// gBattlerPartyIndexes (remplaçant prévu, Pokémon qui gênent le blocage de PP) :
// on retrouve alors le bon Pokémon par sa personnalité, que Morphing ne change pas.
static const struct BoxPokemon *GetBattlerBoxMon(enum BattlerId battler)
{
    struct Pokemon *party = GetBattlerParty(battler);
    u32 i, index = gBattlerPartyIndexes[battler];
    u32 personality = gBattleMons[battler].personality;

    if (party[index].box.personality == personality)
        return &party[index].box;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (party[i].box.personality == personality && GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE)
            return &party[i].box;
    }
    return &party[index].box;
}

enum Type GetBattlerMutatedType(enum BattlerId battler, u32 slot)
{
    return GetBoxMonMutatedType(GetBattlerBoxMon(battler), gBattleMons[battler].species, slot);
}

// La variante ne compte que si l'attaque du combat est bien celle du Pokémon
// (pas après Morphing, Copie ou Gribouille en combat).
enum MoveVariant GetBattlerMoveSlotVariant(enum BattlerId battler, u32 slot)
{
    const struct BoxPokemon *boxMon;

    if (slot >= MAX_MON_MOVES)
        return MOVE_VARIANT_STANDARD;
    boxMon = GetBattlerBoxMon(battler);
    if (boxMon->build.moveVariants == 0 || gBattleMons[battler].volatiles.transformed)
        return MOVE_VARIANT_STANDARD;
    if (GetBoxMonData((struct BoxPokemon *)boxMon, MON_DATA_MOVE1 + slot) != gBattleMons[battler].moves[slot])
        return MOVE_VARIANT_STANDARD;
    return GetBoxMonMoveVariant(boxMon, slot);
}

enum MoveVariant GetBattlerMoveVariant(enum BattlerId battler, enum Move move)
{
    u32 slot;

    // Sortie rapide : appelé à chaque calcul de dégâts, IA comprise.
    if (GetBattlerBoxMon(battler)->build.moveVariants == 0 || move == MOVE_NONE)
        return MOVE_VARIANT_STANDARD;
    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        if (gBattleMons[battler].moves[slot] == move)
            return GetBattlerMoveSlotVariant(battler, slot);
    }
    return MOVE_VARIANT_STANDARD;
}

u32 CalculateBattlerMovePP(enum BattlerId battler, enum Move move, u32 ppBonuses, u32 slot)
{
    return ApplyMoveVariantToPP(CalculatePPWithBonus(move, ppBonuses, slot), GetBattlerMoveSlotVariant(battler, slot));
}

uq4_12_t GetMoveVariantPowerModifier(enum BattlerId battler, enum Move move)
{
    switch (GetBattlerMoveVariant(battler, move))
    {
    case MOVE_VARIANT_FOCUSED:
        return UQ_4_12(MOD_VARIANT_FOCUSED_POWER / 100.0);
    case MOVE_VARIANT_THRIFTY:
        return UQ_4_12(MOD_VARIANT_THRIFTY_POWER / 100.0);
    case MOVE_VARIANT_VOLATILE:
        return UQ_4_12(MOD_VARIANT_VOLATILE_POWER / 100.0);
    default:
        return UQ_4_12(1.0);
    }
}

// L'effet appartient à l'une des attaques du Pokémon : on retrouve laquelle par son adresse.
u32 ApplyMoveVariantEffectChance(enum BattlerId battler, const struct AdditionalEffect *additionalEffect, u32 chance)
{
    u32 slot;

    if (chance == 0 || GetBattlerBoxMon(battler)->build.moveVariants == 0)
        return chance;
    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        enum Move move = gBattleMons[battler].moves[slot];
        const struct AdditionalEffect *first;

        if (move == MOVE_NONE || GetMoveAdditionalEffectCount(move) == 0)
            continue;
        first = GetMoveAdditionalEffectById(move, 0);
        if (additionalEffect >= first && additionalEffect < first + GetMoveAdditionalEffectCount(move))
        {
            if (GetBattlerMoveSlotVariant(battler, slot) == MOVE_VARIANT_VOLATILE)
                return chance * MOD_VARIANT_VOLATILE_CHANCE / 100;
            return chance;
        }
    }
    return chance;
}

// ×1,25 lorsque l'attaque est super efficace contre le type pénalisé et
// que l'efficacité totale reste supérieure à ×1. Si l'autre type compense
// la faiblesse, aucune pénalité (cas encore à définir dans le wiki).
uq4_12_t ApplyMutationWeaknessPenalty(enum BattlerId battlerDef, enum Type moveType, uq4_12_t modifier)
{
    const struct BoxPokemon *boxMon;
    enum Type penaltyType;

    // Sorties rapides d'abord : cette fonction est appelée à chaque calcul d'efficacité, IA comprise.
    if (modifier <= UQ_4_12(1.0) || gBattleMons[battlerDef].volatiles.transformed)
        return modifier;
    boxMon = GetBattlerBoxMon(battlerDef);
    if (boxMon->build.mutationType == TYPE_NONE)
        return modifier;

    penaltyType = GetBoxMonMutationPenaltyType(boxMon, gBattleMons[battlerDef].species);
    if (penaltyType == TYPE_NONE || !IS_BATTLER_OF_TYPE(battlerDef, penaltyType))
        return modifier;
    if (GetTypeModifier(moveType, penaltyType) <= UQ_4_12(1.0) || modifier <= UQ_4_12(1.0))
        return modifier;

    return uq4_12_multiply(modifier, UQ_4_12((double)MOD_MUTATION_WEAKNESS_NUM / MOD_MUTATION_WEAKNESS_DEN));
}

// Type d'origine remplacé par la mutation (celui de l'espèce actuelle à mutationSlot).
enum Type GetBoxMonReplacedType(const struct BoxPokemon *boxMon)
{
    enum Species species;

    if (boxMon->build.mutationType == TYPE_NONE)
        return TYPE_NONE;
    species = GetBoxMonData((struct BoxPokemon *)boxMon, MON_DATA_SPECIES);
    return GetSpeciesType(species, GetSpeciesType(species, 0) == GetSpeciesType(species, 1) ? 0 : boxMon->build.mutationSlot);
}

// Applique « replacedType → newType » si l'espèce possède replacedType et pas déjà newType
// (pas de double type identique : cas encore ouvert dans le wiki, refusé pour l'instant).
bool32 SetBoxMonMutation(struct BoxPokemon *boxMon, enum Type replacedType, enum Type newType)
{
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    enum Type type1 = GetSpeciesType(species, 0);
    enum Type type2 = GetSpeciesType(species, 1);

    if (replacedType == TYPE_NONE || newType == TYPE_NONE || boxMon->build.mutationType != TYPE_NONE)
        return FALSE;
    if (type1 != replacedType && type2 != replacedType)
        return FALSE;
    if (type1 == newType || type2 == newType)
        return FALSE;
    boxMon->build.mutationType = newType;
    boxMon->build.mutationSlot = (type1 == replacedType) ? 0 : 1;
    return TRUE;
}

static bool32 CanEggInheritMutation(struct BoxPokemon *egg, struct BoxPokemon *parent)
{
    struct BoxPokemon copy = *egg;
    copy.build.mutationType = TYPE_NONE;
    return SetBoxMonMutation(&copy, GetBoxMonReplacedType(parent), parent->build.mutationType);
}

static bool32 HasSameMutation(struct BoxPokemon *a, struct BoxPokemon *b)
{
    return a->build.mutationType == b->build.mutationType
        && GetBoxMonReplacedType(a) == GetBoxMonReplacedType(b);
}

// Une seule mutation est transmise, avec sa pénalité ; ni points d'arbre ni maîtrises.
// Même mutation chez les deux parents : garantie. Lineage Charm : garantie, et le parent qui le
// tient l'emporte quand les deux mutations diffèrent. Sinon : MOD_MUTATION_INHERIT_CHANCE %.
void InheritMutation(struct BoxPokemon *egg, struct BoxPokemon *parent1, struct BoxPokemon *parent2)
{
    struct BoxPokemon *parents[2] = {parent1, parent2};
    bool32 compatible[2], charm[2];
    struct BoxPokemon *chosen = NULL;
    u32 i, count = 0;

    for (i = 0; i < 2; i++)
    {
        compatible[i] = CanEggInheritMutation(egg, parents[i]);
        charm[i] = GetBoxMonData(parents[i], MON_DATA_HELD_ITEM) == ITEM_LINEAGE_CHARM;
        count += compatible[i];
    }
    if (count == 0)
        return;

    if (count == 2 && HasSameMutation(parent1, parent2))
        chosen = parent1;
    else if (compatible[0] && charm[0] && !(compatible[1] && charm[1]))
        chosen = parent1;
    else if (compatible[1] && charm[1] && !(compatible[0] && charm[0]))
        chosen = parent2;
    else if (count == 2 && (charm[0] || charm[1]))
        chosen = parents[RandomUniform(RNG_DAYCARE_MUTATION_PICK, 0, 1)];
    else if (charm[0] || charm[1])
        chosen = compatible[0] ? parent1 : parent2;
    else if (RandomPercentage(RNG_DAYCARE_MUTATION_INHERITANCE, MOD_MUTATION_INHERIT_CHANCE))
        chosen = (count == 2) ? parents[RandomUniform(RNG_DAYCARE_MUTATION_PICK, 0, 1)] : (compatible[0] ? parent1 : parent2);

    if (chosen != NULL)
        SetBoxMonMutation(egg, GetBoxMonReplacedType(chosen), chosen->build.mutationType);

    if (MOD_LINEAGE_CHARM_CONSUMED)
    {
        enum Item none = ITEM_NONE;
        for (i = 0; i < 2; i++)
        {
            if (charm[i])
                SetBoxMonData(parents[i], MON_DATA_HELD_ITEM, &none);
        }
    }
}

const struct TreeNode *GetTreeNode(enum Type treeType, u32 node)
{
    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES || node >= MOD_TREE_NODE_COUNT)
        return NULL;
    if (gElementTrees[treeType].nodes[node].maxRank == 0)
        return NULL;
    return &gElementTrees[treeType].nodes[node];
}

// Les points proviennent du niveau, plus d'éventuels points bonus (treePoints), dans la limite du budget.
u32 GetBoxMonTreePointsEarned(struct BoxPokemon *boxMon)
{
    u32 points = GetLevelFromBoxMonExp(boxMon) * MOD_TREE_POINTS_PER_LEVEL + boxMon->build.treePoints;
    return min(points, MOD_TREE_POINT_BUDGET);
}

u32 GetBoxMonTreePointsAvailable(struct BoxPokemon *boxMon)
{
    u32 earned = GetBoxMonTreePointsEarned(boxMon);
    u32 spent = GetBoxMonTreePointsSpent(boxMon);
    return earned > spent ? earned - spent : 0;
}

static bool32 BoxMonHasType(const struct BoxPokemon *boxMon, enum Type type)
{
    enum Species species = GetBoxMonData((struct BoxPokemon *)boxMon, MON_DATA_SPECIES);
    return GetBoxMonMutatedType(boxMon, species, 0) == type || GetBoxMonMutatedType(boxMon, species, 1) == type;
}

static u32 GetGeneralPointsSpent(const struct BoxPokemon *boxMon)
{
    u32 node, total = 0;
    for (node = 0; node < TREE_GENERAL_NODES; node++)
        total += GetBoxMonTreeRank(boxMon, node);
    return total;
}

enum TreeInvestResult CanBoxMonInvestInNode(struct BoxPokemon *boxMon, enum Type treeType, u32 node)
{
    const struct TreeNode *treeNode;

    // Un seul arbre par Pokémon pour l'instant (doubles types : question ouverte du wiki).
    if (boxMon->build.treeType != TYPE_NONE && boxMon->build.treeType != treeType)
        return TREE_INVEST_WRONG_TREE;
    if (boxMon->build.treeType == TYPE_NONE && !BoxMonHasType(boxMon, treeType))
        return TREE_INVEST_WRONG_TREE;

    treeNode = GetTreeNode(treeType, node);
    if (treeNode == NULL)
        return TREE_INVEST_NO_NODE;
    if (GetBoxMonTreeRank(boxMon, node) >= treeNode->maxRank)
        return TREE_INVEST_MAX_RANK;
    if (GetBoxMonTreePointsAvailable(boxMon) == 0)
        return TREE_INVEST_NO_POINTS;

    if (node >= TREE_GENERAL_NODES)
    {
        u32 indexInBranch = (node - TREE_GENERAL_NODES) % TREE_BRANCH_NODES;
        if (indexInBranch == 0)
        {
            if (GetGeneralPointsSpent(boxMon) < MOD_TREE_BRANCH_UNLOCK_POINTS)
                return TREE_INVEST_LOCKED;
        }
        else if (GetBoxMonTreeRank(boxMon, node - 1) < min(MOD_TREE_PARENT_RANKS, GetTreeNode(treeType, node - 1)->maxRank))
        {
            return TREE_INVEST_LOCKED;
        }
    }
    return TREE_INVEST_OK;
}

// Investissement définitif : il n'existe pas de fonction de redistribution.
enum TreeInvestResult TryBoxMonInvestInNode(struct BoxPokemon *boxMon, enum Type treeType, u32 node)
{
    enum TreeInvestResult result = CanBoxMonInvestInNode(boxMon, treeType, node);
    if (result != TREE_INVEST_OK)
        return result;

    boxMon->build.treeType = treeType;
    SetBoxMonTreeRank(boxMon, node, GetBoxMonTreeRank(boxMon, node) + 1);
    return TREE_INVEST_OK;
}

// Acquisition individuelle : MOD_MUTATION_TREE_POINTS points investis et une maîtrise finale.
bool32 CanBoxMonAcquireMutation(const struct BoxPokemon *boxMon)
{
    u32 branch;

    if (boxMon->build.mutationType != TYPE_NONE)
        return FALSE;
    if (GetBoxMonTreePointsSpent(boxMon) < MOD_MUTATION_TREE_POINTS)
        return FALSE;
    for (branch = 0; branch < TREE_BRANCH_COUNT; branch++)
    {
        if (GetBoxMonTreeRank(boxMon, TREE_GENERAL_NODES + branch * TREE_BRANCH_NODES + TREE_BRANCH_NODES - 1) > 0)
            return TRUE;
    }
    return FALSE;
}

// Types vers lesquels on peut muter : les 18 types réels.
bool32 IsMutationTargetType(enum Type type)
{
    return type != TYPE_NONE && type != TYPE_MYSTERY && type < TYPE_STELLAR;
}

// Types ouverts à la mutation : MOD_MUTATION_BASE_TYPES dès le départ, puis un par Mutation Seal
// (VAR_MOD_MUTATION_TYPES pour les types 0 à 15, VAR_MOD_MUTATION_TYPES_2 pour la suite).
bool32 IsMutationTypeOpen(enum Type type)
{
    if (!IsMutationTargetType(type))
        return FALSE;
    if (MOD_MUTATION_BASE_TYPES & (1u << type))
        return TRUE;
    if (type < 16)
        return (VarGet(VAR_MOD_MUTATION_TYPES) >> type) & 1;
    return (VarGet(VAR_MOD_MUTATION_TYPES_2) >> (type - 16)) & 1;
}

void OpenMutationType(enum Type type)
{
    if (!IsMutationTargetType(type))
        return;
    if (type < 16)
        VarSet(VAR_MOD_MUTATION_TYPES, VarGet(VAR_MOD_MUTATION_TYPES) | (1u << type));
    else
        VarSet(VAR_MOD_MUTATION_TYPES_2, VarGet(VAR_MOD_MUTATION_TYPES_2) | (1u << (type - 16)));
}

u32 GetBoxMonTreeEffect(const struct BoxPokemon *boxMon, enum TreeEffect effect)
{
    u32 node, total = 0;
    enum Type treeType = boxMon->build.treeType;

    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = &gElementTrees[treeType].nodes[node];
        if (treeNode->maxRank != 0 && treeNode->effect == effect)
            total += min(GetBoxMonTreeRank(boxMon, node), treeNode->maxRank) * treeNode->valuePerRank;
    }
    return total;
}

// Somme des nœuds de cet effet dont le paramètre vaut exactement param.
static u32 GetBoxMonTreeEffectParam(const struct BoxPokemon *boxMon, enum TreeEffect effect, u32 param)
{
    u32 node, total = 0;
    enum Type treeType = boxMon->build.treeType;

    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = &gElementTrees[treeType].nodes[node];
        if (treeNode->maxRank != 0 && treeNode->effect == effect && treeNode->param == param)
            total += min(GetBoxMonTreeRank(boxMon, node), treeNode->maxRank) * treeNode->valuePerRank;
    }
    return total;
}

// Premier nœud investi de cet effet (pour lire son paramètre), NULL s'il n'y en a pas.
static const struct TreeNode *GetBoxMonTreeEffectNode(const struct BoxPokemon *boxMon, enum TreeEffect effect)
{
    u32 node;
    enum Type treeType = boxMon->build.treeType;

    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return NULL;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = &gElementTrees[treeType].nodes[node];
        if (treeNode->maxRank != 0 && treeNode->effect == effect && GetBoxMonTreeRank(boxMon, node) != 0)
            return treeNode;
    }
    return NULL;
}

u32 GetBattlerTreeEffect(enum BattlerId battler, enum TreeEffect effect)
{
    return GetBoxMonTreeEffect(GetBattlerBoxMon(battler), effect);
}

static u32 GetBattlerTreeEffectParam(enum BattlerId battler, enum TreeEffect effect, u32 param)
{
    return GetBoxMonTreeEffectParam(GetBattlerBoxMon(battler), effect, param);
}

static const struct TreeNode *GetBattlerTreeEffectNode(enum BattlerId battler, enum TreeEffect effect)
{
    return GetBoxMonTreeEffectNode(GetBattlerBoxMon(battler), effect);
}

static enum Type GetBattlerTreeType(enum BattlerId battler)
{
    enum Type treeType = GetBattlerBoxMon(battler)->build.treeType;
    if (treeType >= NUMBER_OF_MON_TYPES)
        return TYPE_NONE;
    return treeType;
}

const struct ElementTree *GetBattlerTree(enum BattlerId battler)
{
    enum Type treeType = GetBattlerTreeType(battler);
    if (treeType == TYPE_NONE)
        return NULL;
    return &gElementTrees[treeType];
}

static bool32 IsTreeWeatherActive(const struct ElementTree *tree, u32 weather)
{
    return tree != NULL && tree->weather != 0 && HasWeatherEffect() && (weather & tree->weather);
}

static bool32 IsTreeTerrainActive(const struct ElementTree *tree)
{
    return tree != NULL && tree->terrain != B_TERRAIN_NONE && gFieldTimers.terrain == tree->terrain;
}

// ---------------------------------------------------------------------------------------------
// État de combat des arbres

// Maîtrises « une fois par combat » : un bit par effet, pour chaque Pokémon de chaque camp.
enum TreeOnce
{
    TREE_ONCE_ENTRY,          // Effet d'entrée (météo, terrain, piège, stat…).
    TREE_ONCE_FIRST_HIT,      // Premier coup amorti (Unbreakable, Ice Face, Mud Coat).
    TREE_ONCE_ENDURE,
    TREE_ONCE_GROUND_IMMUNITY,
    TREE_ONCE_AQUA_RING,
    TREE_ONCE_MOLT,
    TREE_ONCE_TOXIC_DEBRIS,
    TREE_ONCE_STATUS_MOVE_BLOCK,
    TREE_ONCE_BERSERK,
    TREE_ONCE_EMERGENCY_HEAL,
    TREE_ONCE_PAIN_SPLIT,
};

static EWRAM_DATA u16 sTreeOnceUsed[NUM_BATTLE_SIDES][PARTY_SIZE] = {0};
static EWRAM_DATA u8 sTreeHeartStacks[MAX_BATTLERS_COUNT] = {0};
static EWRAM_DATA u8 sTreeChainReady[MAX_BATTLERS_COUNT] = {0};    // Circuit : la dernière attaque était du type.
static EWRAM_DATA u8 sTreeMomentumReady[MAX_BATTLERS_COUNT] = {0}; // Iron Momentum : touché depuis sa dernière attaque du type.
static EWRAM_DATA u8 sTreeFreshEntry[MAX_BATTLERS_COUNT] = {0};    // Phantom Ambush : aucune attaque depuis l'entrée.
static EWRAM_DATA u16 sTreePendingHeal[MAX_BATTLERS_COUNT] = {0};  // Soins dus en fin de tour (Venom Siphon, Dark Recovery).
static EWRAM_DATA u8 sTreeEndTurnStep[MAX_BATTLERS_COUNT] = {0};   // Effets de fin de tour déjà vus ce tour-ci.

// Statuts suivis (Fan the Flames, Venom Siphon) : qui a infligé le statut de chaque cible.
struct TreeStatusSource
{
    u8 battler;
    u8 partyIndex;
    bool8 valid;
};

static EWRAM_DATA struct TreeStatusSource sTreeStatusSource[MAX_BATTLERS_COUNT] = {0};

static bool32 IsTreeOnceUsed(enum BattlerId battler, enum TreeOnce once)
{
    return (sTreeOnceUsed[GetBattlerSide(battler)][gBattlerPartyIndexes[battler] % PARTY_SIZE] >> once) & 1;
}

static void SetTreeOnceUsed(enum BattlerId battler, enum TreeOnce once)
{
    sTreeOnceUsed[GetBattlerSide(battler)][gBattlerPartyIndexes[battler] % PARTY_SIZE] |= 1u << once;
}

// Maîtrise investie et pas encore dépensée dans ce combat.
static bool32 IsTreeOnceReady(enum BattlerId battler, enum TreeEffect effect, enum TreeOnce once)
{
    return GetBattlerTreeEffect(battler, effect) != 0 && !IsTreeOnceUsed(battler, once);
}

void ResetTreeBattleState(void)
{
    memset(sTreeOnceUsed, 0, sizeof(sTreeOnceUsed));
    memset(sTreeHeartStacks, 0, sizeof(sTreeHeartStacks));
    memset(sTreeChainReady, 0, sizeof(sTreeChainReady));
    memset(sTreeMomentumReady, 0, sizeof(sTreeMomentumReady));
    memset(sTreeFreshEntry, 0, sizeof(sTreeFreshEntry));
    memset(sTreePendingHeal, 0, sizeof(sTreePendingHeal));
    memset(sTreeEndTurnStep, 0, sizeof(sTreeEndTurnStep));
    memset(sTreeStatusSource, 0, sizeof(sTreeStatusSource));
}

// Le Pokémon qui entre repart de zéro : cumuls et enchaînements perdus, statut suivi oublié
// (les maîtrises « une fois par combat » restent dépensées).
void ResetTreeOnSwitchIn(enum BattlerId battler)
{
    sTreeHeartStacks[battler] = 0;
    sTreeChainReady[battler] = FALSE;
    sTreeMomentumReady[battler] = FALSE;
    sTreeFreshEntry[battler] = TRUE;
    sTreePendingHeal[battler] = 0;
    sTreeStatusSource[battler].valid = FALSE;
}

u32 GetTreeHeartStacks(enum BattlerId battler)
{
    return sTreeHeartStacks[battler];
}

// ---------------------------------------------------------------------------------------------
// Dégâts

static bool32 IsTreeMoveFamily(enum Move move, enum TreeMoveFamily family)
{
    switch (family)
    {
    case TREE_MOVES_PUNCH:
        return IsPunchingMove(move);
    case TREE_MOVES_BITE:
        return IsBitingMove(move);
    case TREE_MOVES_SOUND:
        return IsSoundMove(move);
    case TREE_MOVES_WIND:
        return IsWindMove(move);
    }
    return FALSE;
}

static bool32 HasRaisedStat(enum BattlerId battler)
{
    u32 stat;
    for (stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
    {
        if (gBattleMons[battler].statStages[stat] > DEFAULT_STAT_STAGE)
            return TRUE;
    }
    return FALSE;
}

// Ordre d'action : pendant les calculs de l'IA, on compare les Vitesses.
static bool32 TreeMovesBeforeTarget(struct DamageContext *ctx)
{
    if (ctx->aiCalc)
        return GetBattlerTotalSpeedStat(ctx->battlerAtk, ctx->abilities[ctx->battlerAtk], ctx->holdEffects[ctx->battlerAtk])
             > GetBattlerTotalSpeedStat(ctx->battlerDef, ctx->abilities[ctx->battlerDef], ctx->holdEffects[ctx->battlerDef]);
    return GetBattlerTurnOrderNum(ctx->battlerAtk) < GetBattlerTurnOrderNum(ctx->battlerDef);
}

static bool32 TreeMovesAfterTarget(struct DamageContext *ctx)
{
    if (ctx->aiCalc)
        return GetBattlerTotalSpeedStat(ctx->battlerAtk, ctx->abilities[ctx->battlerAtk], ctx->holdEffects[ctx->battlerAtk])
             < GetBattlerTotalSpeedStat(ctx->battlerDef, ctx->abilities[ctx->battlerDef], ctx->holdEffects[ctx->battlerDef]);
    return GetBattlerTurnOrderNum(ctx->battlerAtk) > GetBattlerTurnOrderNum(ctx->battlerDef);
}

// Somme des nœuds TYPE_DAMAGE et TYPE_DAMAGE_WEATHER qui s'appliquent à ce type d'attaque
// (paramètre : masque de types, 0 = le type de l'arbre).
static u32 GetTreeTypedBonus(const struct BoxPokemon *boxMon, enum TreeEffect effect, enum Type treeType, enum Type moveType)
{
    u32 node, total = 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = &gElementTrees[treeType].nodes[node];
        u32 types = treeNode->param != 0 ? treeNode->param : (1u << treeType);
        if (treeNode->maxRank != 0 && treeNode->effect == effect && (types & (1u << moveType)))
            total += min(GetBoxMonTreeRank(boxMon, node), treeNode->maxRank) * treeNode->valuePerRank;
    }
    return total;
}

// Bonus offensifs : ils s'additionnent entre eux (règle du wiki), en ‱.
uq4_12_t GetTreeDamageModifier(struct DamageContext *ctx)
{
    enum BattlerId battlerAtk = ctx->battlerAtk;
    enum BattlerId battlerDef = ctx->battlerDef;
    const struct BoxPokemon *boxMon = GetBattlerBoxMon(battlerAtk);
    enum Type treeType = GetBattlerTreeType(battlerAtk);
    const struct ElementTree *tree;
    enum Move move = ctx->move;
    u32 bonus = 0, family;
    uq4_12_t modifier;

    if (treeType == TYPE_NONE)
        return UQ_4_12(1.0);
    tree = &gElementTrees[treeType];

    if (IsBattleMovePhysical(move))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_PHYSICAL_DAMAGE);
    else if (IsBattleMoveSpecial(move))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_SPECIAL_DAMAGE);

    bonus += GetTreeTypedBonus(boxMon, TREE_EFFECT_TYPE_DAMAGE, treeType, ctx->moveType);
    if (IsTreeWeatherActive(tree, ctx->weather))
        bonus += GetTreeTypedBonus(boxMon, TREE_EFFECT_TYPE_DAMAGE_WEATHER, treeType, ctx->moveType);

    if (ctx->moveType == treeType)
    {
        if (gBattleMons[battlerAtk].hp * 2 < gBattleMons[battlerAtk].maxHP)
            bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_TYPE_DAMAGE_LOW_HP);
        if (gBattleMons[battlerAtk].hp * 4 > gBattleMons[battlerAtk].maxHP * 3)
            bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_TYPE_DAMAGE_HIGH_HP);
        bonus += GetTreeHeartStacks(battlerAtk) * GetBoxMonTreeEffect(boxMon, TREE_EFFECT_TYPE_DAMAGE_STACK);
        if (sTreeChainReady[battlerAtk])
            bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_CHAIN_DAMAGE);
        if (sTreeMomentumReady[battlerAtk])
            bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_MOMENTUM_DAMAGE);
    }

    if (GetMoveType(move) == TYPE_NORMAL && ctx->moveType == treeType && treeType != TYPE_NORMAL)
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_NORMAL_TO_TYPE);

    {
        const struct TreeNode *node = GetBoxMonTreeEffectNode(boxMon, TREE_EFFECT_DAMAGE_VS_STATUS);
        if (node != NULL)
        {
            u32 status = node->param != 0 ? node->param : tree->status;
            if (gBattleMons[battlerDef].status1 & status)
                bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_VS_STATUS);
        }
    }

    for (family = TREE_MOVES_PUNCH; family <= TREE_MOVES_WIND; family++)
    {
        if (IsTreeMoveFamily(move, family))
            bonus += GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_MOVE_FAMILY_DAMAGE, family);
    }

    if (ctx->typeEffectivenessModifier >= UQ_4_12(2.0))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_SUPER_EFFECTIVE_DAMAGE);
    else if (ctx->typeEffectivenessModifier < UQ_4_12(1.0) && ctx->typeEffectivenessModifier != UQ_4_12(0.0))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_NOT_VERY_EFFECTIVE_DAMAGE);

    if (GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_MOVING_FIRST) && TreeMovesBeforeTarget(ctx))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_MOVING_FIRST);
    if (GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_MOVING_LAST) && TreeMovesAfterTarget(ctx))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_MOVING_LAST);

    if (gBattleMons[battlerDef].hp * 2 < gBattleMons[battlerDef].maxHP)
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_TARGET_LOW_HP);
    if (IsBattlerAtMaxHp(battlerDef))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_TARGET_FULL_HP);
    if (HasRaisedStat(battlerAtk))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_STAT_RAISED);
    if (gBattleMons[battlerDef].statStages[STAT_ATK] < DEFAULT_STAT_STAGE
     || gBattleMons[battlerDef].statStages[STAT_SPATK] < DEFAULT_STAT_STAGE)
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_TARGET_WEAKENED);
    if (IsBattleMovePhysical(move) && (gBattleMons[battlerAtk].status1 & STATUS1_ANY))
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_PHYSICAL_DAMAGE_STATUSED);
    if (sTreeFreshEntry[battlerAtk])
        bonus += GetBoxMonTreeEffect(boxMon, TREE_EFFECT_DAMAGE_AFTER_ENTRY);

    modifier = UQ_4_12(1.0) + (bonus * UQ_4_12(1.0)) / 10000;

    // Sniper : multiplicateur des coups critiques (×1,5 de base).
    if (ctx->isCrit && GetBoxMonTreeEffect(boxMon, TREE_EFFECT_CRIT_DAMAGE))
        modifier = uq4_12_multiply(modifier, UQ_4_12(1.0) + (GetBoxMonTreeEffect(boxMon, TREE_EFFECT_CRIT_DAMAGE) * UQ_4_12(1.0)) / 15000);

    return modifier;
}

static uq4_12_t ApplyTreeReduction(uq4_12_t modifier, u32 reduction)
{
    if (reduction == 0)
        return modifier;
    reduction = min(reduction, 10000);
    return uq4_12_multiply(modifier, UQ_4_12(1.0) - (reduction * UQ_4_12(1.0)) / 10000);
}

// Premier coup amorti (Unbreakable, Ice Face, Mud Coat) : ce coup est-il concerné ?
static bool32 IsTreeFirstHitMatch(struct DamageContext *ctx, u32 param)
{
    if (param == TREE_GUARD_SUPER_EFFECTIVE)
        return ctx->typeEffectivenessModifier >= UQ_4_12(2.0);
    if (param & TREE_GUARD_PHYSICAL)
        return IsBattleMovePhysical(ctx->move);
    return (param & (1u << ctx->moveType)) != 0;
}

// Réductions défensives : chacune multiplie les dégâts (comme les talents défensifs).
uq4_12_t GetTreeDefenseModifier(struct DamageContext *ctx)
{
    enum BattlerId battlerDef = ctx->battlerDef;
    const struct BoxPokemon *boxMon = GetBattlerBoxMon(battlerDef);
    enum Type treeType = GetBattlerTreeType(battlerDef);
    const struct ElementTree *tree;
    uq4_12_t modifier = UQ_4_12(1.0);
    const struct TreeNode *node;
    u32 i;

    if (treeType == TYPE_NONE || ctx->isSelfInflicted)
        return modifier;
    tree = &gElementTrees[treeType];

    for (i = 0; i < MOD_TREE_NODE_COUNT; i++)
    {
        const struct TreeNode *ward = &tree->nodes[i];
        if (ward->maxRank != 0 && ward->effect == TREE_EFFECT_WARD && (ward->param & (1u << ctx->moveType)))
            modifier = ApplyTreeReduction(modifier, min(GetBoxMonTreeRank(boxMon, i), ward->maxRank) * ward->valuePerRank);
    }
    if (ctx->typeEffectivenessModifier >= UQ_4_12(2.0))
        modifier = ApplyTreeReduction(modifier, GetBoxMonTreeEffect(boxMon, TREE_EFFECT_WEAKNESS_GUARD));
    if (IsBattleMovePhysical(ctx->move))
        modifier = ApplyTreeReduction(modifier, GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_CATEGORY_GUARD, TREE_CATEGORY_PHYSICAL));
    else if (IsBattleMoveSpecial(ctx->move))
        modifier = ApplyTreeReduction(modifier, GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_CATEGORY_GUARD, TREE_CATEGORY_SPECIAL));
    if (IsBattlerAtMaxHp(battlerDef))
        modifier = ApplyTreeReduction(modifier, GetBoxMonTreeEffect(boxMon, TREE_EFFECT_FULL_HP_GUARD));
    if (IsTreeWeatherActive(tree, ctx->weather))
        modifier = ApplyTreeReduction(modifier, GetBoxMonTreeEffect(boxMon, TREE_EFFECT_WEATHER_GUARD));

    node = GetBoxMonTreeEffectNode(boxMon, TREE_EFFECT_FIRST_HIT_GUARD);
    if (node != NULL && !IsTreeOnceUsed(battlerDef, TREE_ONCE_FIRST_HIT) && IsTreeFirstHitMatch(ctx, node->param))
        modifier = ApplyTreeReduction(modifier, node->valuePerRank);

    return modifier;
}

// Fin d'attaque : le premier coup amorti est dépensé par les cibles qu'il a touchées. Le calcul
// des dégâts ne fait que lire l'état, pour que les simulations de l'IA n'y touchent pas.
void UpdateTreeFirstHitGuard(enum BattlerId battlerAtk, enum Move move)
{
    enum BattlerId battler;
    enum Type moveType = GetBattleMoveType(move);

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        const struct TreeNode *node;
        bool32 match;

        if (battler == battlerAtk || IsTreeOnceUsed(battler, TREE_ONCE_FIRST_HIT)
         || !IsBattlerTurnDamaged(battler, INCLUDING_SUBSTITUTES)
         || (node = GetBattlerTreeEffectNode(battler, TREE_EFFECT_FIRST_HIT_GUARD)) == NULL)
            continue;
        if (node->param == TREE_GUARD_SUPER_EFFECTIVE)
            match = (gBattleStruct->moveResultFlags[battler] & MOVE_RESULT_HIGH_EFFECTIVENESS) != 0;
        else if (node->param & TREE_GUARD_PHYSICAL)
            match = IsBattleMovePhysical(move);
        else
            match = (node->param & (1u << moveType)) != 0;
        if (match)
            SetTreeOnceUsed(battler, TREE_ONCE_FIRST_HIT);
    }

}

// Recul en moins : toutes les attaques du Pokémon.
u32 ApplyTreeRecoilReduction(enum BattlerId battlerAtk, enum Type moveType, u32 recoil)
{
    u32 reduction = min(GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_RECOIL_REDUCTION), 10000);

    if (reduction == 0)
        return recoil;
    recoil = recoil * (10000 - reduction) / 10000;
    return max(recoil, 1);
}

// ---------------------------------------------------------------------------------------------
// Statuts et effets secondaires

// Les statuts d'un arbre sont des masques (poison et poison grave, gel et gelure).
u32 GetTreeStatusMoveEffect(u32 status)
{
    if (status & STATUS1_BURN)
        return MOVE_EFFECT_BURN;
    if (status & STATUS1_PSN_ANY)
        return MOVE_EFFECT_POISON;
    if (status & STATUS1_PARALYSIS)
        return MOVE_EFFECT_PARALYSIS;
    if (status & (STATUS1_FREEZE | STATUS1_FROSTBITE))
        return MOVE_EFFECT_FREEZE_OR_FROSTBITE;
    if (status & STATUS1_SLEEP)
        return MOVE_EFFECT_SLEEP;
    return MOVE_EFFECT_NONE;
}

static bool32 IsTreeStatusMoveEffect(u32 status, u32 moveEffect)
{
    switch (moveEffect)
    {
    case MOVE_EFFECT_BURN:
        return (status & STATUS1_BURN) != 0;
    case MOVE_EFFECT_POISON:
    case MOVE_EFFECT_TOXIC:
        return (status & STATUS1_PSN_ANY) != 0;
    case MOVE_EFFECT_PARALYSIS:
        return (status & STATUS1_PARALYSIS) != 0;
    case MOVE_EFFECT_FREEZE:
    case MOVE_EFFECT_FROSTBITE:
        return (status & (STATUS1_FREEZE | STATUS1_FROSTBITE)) != 0;
    case MOVE_EFFECT_SLEEP:
        return (status & STATUS1_SLEEP) != 0;
    default:
        return FALSE;
    }
}

static bool32 IsTreeConfusionMoveEffect(u32 moveEffect)
{
    return moveEffect == MOVE_EFFECT_CONFUSION;
}

// Chance d'effet secondaire en points de pourcentage, ajoutée à celle de l'attaque.
u32 GetTreeSecondaryChanceBonus(enum BattlerId battlerAtk, enum Move move, u32 moveEffect)
{
    const struct ElementTree *tree = GetBattlerTree(battlerAtk);
    const struct BoxPokemon *boxMon;
    u32 bonus = 0, node;

    if (tree == NULL)
        return 0;
    boxMon = GetBattlerBoxMon(battlerAtk);

    // Statut : toutes ses attaques qui peuvent déjà l'infliger.
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = &tree->nodes[node];
        // Un statut précisé par le nœud (Dragon Breath) ne vaut que pour ses attaques du type.
        if (treeNode->maxRank != 0 && treeNode->effect == TREE_EFFECT_STATUS_CHANCE
         && (treeNode->param == 0 || GetBattleMoveType(move) == boxMon->build.treeType)
         && IsTreeStatusMoveEffect(treeNode->param != 0 ? treeNode->param : tree->status, moveEffect))
            bonus += min(GetBoxMonTreeRank(boxMon, node), treeNode->maxRank) * treeNode->valuePerRank;
    }

    // Autres effets : seulement ses attaques du type.
    if (GetBattleMoveType(move) == boxMon->build.treeType)
    {
        bonus += GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_SECONDARY_CHANCE, TREE_SECONDARY_ANY);
        if (moveEffect == MOVE_EFFECT_FLINCH)
            bonus += GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_SECONDARY_CHANCE, TREE_SECONDARY_FLINCH);
        if (IsTreeConfusionMoveEffect(moveEffect))
            bonus += GetBoxMonTreeEffectParam(boxMon, TREE_EFFECT_SECONDARY_CHANCE, TREE_SECONDARY_CONFUSION);
    }
    return bonus / 100;
}

// Serene Spirit : multiplicateur de toutes les chances d'effet secondaire.
u32 ApplyTreeSecondaryMultiplier(enum BattlerId battlerAtk, u32 chance)
{
    u32 multiplier = GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_SECONDARY_MULTIPLIER);
    if (multiplier == 0)
        return chance;
    return chance + chance * multiplier / 10000;
}

// ---------------------------------------------------------------------------------------------
// Météo, terrain et autres effets posés

static u32 GetTreeFieldTurns(enum BattlerId battler, enum TreeField field)
{
    return GetBattlerTreeEffectParam(battler, TREE_EFFECT_FIELD_TURNS, field);
}

u32 GetTreeWeatherTurnsBonus(enum BattlerId battler, u32 weatherFlag)
{
    const struct ElementTree *tree = GetBattlerTree(battler);

    if (tree == NULL || !(tree->weather & weatherFlag))
        return 0;
    return GetTreeFieldTurns(battler, TREE_FIELD_WEATHER);
}

u32 GetTreeTerrainTurnsBonus(enum BattlerId battler, enum BattleTerrain terrain)
{
    const struct ElementTree *tree = GetBattlerTree(battler);

    if (tree == NULL || tree->terrain != terrain)
        return 0;
    return GetTreeFieldTurns(battler, TREE_FIELD_TERRAIN);
}

u32 GetTreeTailwindTurnsBonus(enum BattlerId battler)
{
    return GetTreeFieldTurns(battler, TREE_FIELD_TAILWIND);
}

u32 GetTreeScreenTurnsBonus(enum BattlerId battler)
{
    return GetTreeFieldTurns(battler, TREE_FIELD_SCREENS);
}

u32 GetTreeWeatherHealAmount(enum BattlerId battler)
{
    const struct ElementTree *tree = GetBattlerTree(battler);
    u32 heal;

    if (!IsTreeWeatherActive(tree, gBattleWeather))
        return 0;
    heal = GetBattlerTreeEffect(battler, TREE_EFFECT_WEATHER_HEAL);
    if (heal == 0)
        return 0;
    return max(GetNonDynamaxMaxHP(battler) * heal / 10000, 1);
}

// ---------------------------------------------------------------------------------------------
// Maîtrises Heart (Incandescent Heart, Dragon Heart) et Sap Surge

// Après une attaque réussie du type de l'arbre : un cumul de plus. Heart coûte une part des PV max
// (Magic Guard évite le coût... et le cumul) ; Sap Surge ne compte que les coups qui absorbent des PV.
bool32 TryTreeHeartActivation(enum BattlerId battlerAtk, enum Move move)
{
    const struct TreeNode *node = GetBattlerTreeEffectNode(battlerAtk, TREE_EFFECT_TYPE_DAMAGE_STACK);

    if (node == NULL
     || sTreeHeartStacks[battlerAtk] >= MOD_TREE_HEART_MAX_STACKS
     || GetBattlerTreeType(battlerAtk) != GetBattleMoveType(move)
     || gBattleStruct->unableToUseMove
     || !IsBattlerAlive(battlerAtk)
     || !IsAnyTargetTurnDamaged(battlerAtk, INCLUDING_SUBSTITUTES))
        return FALSE;

    if (node->param == TREE_STACK_DRAIN)
    {
        if (!MoveHasAdditionalEffect(move, MOVE_EFFECT_ABSORB))
            return FALSE;
        sTreeHeartStacks[battlerAtk]++;
        return FALSE; // Pas de coût, pas de message.
    }

    if (GetBattlerAbility(battlerAtk) == ABILITY_MAGIC_GUARD)
        return FALSE;
    sTreeHeartStacks[battlerAtk]++;
    SetPassiveDamageAmount(battlerAtk, max(GetNonDynamaxMaxHP(battlerAtk) * MOD_TREE_HEART_HP_COST / 100, 1));
    return TRUE;
}

// Fin de chaque attaque du lanceur : Circuit, Iron Momentum, Phantom Ambush.
void UpdateTreeAfterMove(enum BattlerId battlerAtk, enum Move move)
{
    enum Type treeType = GetBattlerTreeType(battlerAtk);
    bool32 typed = (treeType != TYPE_NONE && GetBattleMoveType(move) == treeType && !IsBattleMoveStatus(move));

    sTreeChainReady[battlerAtk] = typed;
    if (typed)
        sTreeMomentumReady[battlerAtk] = FALSE;
    sTreeFreshEntry[battlerAtk] = FALSE;
}

// Iron Momentum : les Pokémon touchés par un adversaire préparent leur prochaine attaque.
void UpdateTreeTargetsHit(enum BattlerId battlerAtk)
{
    enum BattlerId battler;
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (battler != battlerAtk && GetBattlerSide(battler) != GetBattlerSide(battlerAtk)
         && IsBattlerTurnDamaged(battler, INCLUDING_SUBSTITUTES))
            sTreeMomentumReady[battler] = TRUE;
    }
}

// ---------------------------------------------------------------------------------------------
// Effets d'entrée (maîtrises Utility)

static u32 GetTreeBattleWeather(u32 weatherFlags)
{
    if (weatherFlags & B_WEATHER_SUN)
        return BATTLE_WEATHER_SUN;
    if (weatherFlags & B_WEATHER_RAIN)
        return BATTLE_WEATHER_RAIN;
    if (weatherFlags & B_WEATHER_SANDSTORM)
        return BATTLE_WEATHER_SANDSTORM;
    if (weatherFlags & (B_WEATHER_HAIL | B_WEATHER_SNOW))
        return GetConfig(B_SNOW_WARNING) >= GEN_9 ? BATTLE_WEATHER_SNOW : BATTLE_WEATHER_HAIL;
    return BATTLE_WEATHER_COUNT;
}

// ---------------------------------------------------------------------------------------------
// Coups critiques

s32 GetTreeCritStageBonus(enum BattlerId battlerAtk, enum Move move, enum Type moveType)
{
    const struct BoxPokemon *boxMon = GetBattlerBoxMon(battlerAtk);

    // Sortie rapide : appelée à chaque calcul de critique, IA comprise.
    if (boxMon->build.treeType != moveType)
        return 0;
    return GetBoxMonTreeEffect(boxMon, TREE_EFFECT_TYPE_CRIT_STAGE);
}

bool32 TreeAlwaysCrits(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    return (gBattleMons[battlerDef].status1 & STATUS1_PSN_ANY) && GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_MERCILESS) != 0;
}

// ---------------------------------------------------------------------------------------------
// Statuts suivis

static const struct ElementTree *GetBoxMonTree(const struct BoxPokemon *boxMon)
{
    enum Type treeType = boxMon->build.treeType;
    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return NULL;
    return &gElementTrees[treeType];
}

// Appelé chaque fois qu'un statut est infligé à une cible par un autre Pokémon.
void RecordTreeStatusInflicted(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    if (battlerAtk == battlerDef || battlerAtk >= gBattlersCount)
        return;
    sTreeStatusSource[battlerDef].battler = battlerAtk;
    sTreeStatusSource[battlerDef].partyIndex = gBattlerPartyIndexes[battlerAtk];
    sTreeStatusSource[battlerDef].valid = TRUE;
}

static const struct BoxPokemon *GetTreeStatusSourceBoxMon(enum BattlerId battler)
{
    const struct TreeStatusSource *source = &sTreeStatusSource[battler];
    if (!source->valid)
        return NULL;
    return &GetBattlerParty(source->battler)[source->partyIndex].box;
}

// Fan the Flames, Lingering Toxins : dégâts résiduels du statut de l'arbre, s'il vient de ce Pokémon.
// Venom Siphon, Night Terrors : la source, si elle est sur le terrain, récupère des PV en fin de tour.
u32 ApplyTreeStatusDamageBonus(enum BattlerId battler, u32 status, u32 damage)
{
    const struct TreeStatusSource *source = &sTreeStatusSource[battler];
    const struct BoxPokemon *sourceMon = GetTreeStatusSourceBoxMon(battler);
    const struct ElementTree *tree;
    u32 bonus, siphon;

    if (sourceMon == NULL || (tree = GetBoxMonTree(sourceMon)) == NULL || !(tree->status & status))
        return ApplyTreeIndirectGuard(battler, damage);
    bonus = GetBoxMonTreeEffect(sourceMon, TREE_EFFECT_STATUS_DAMAGE);
    siphon = GetBoxMonTreeEffect(sourceMon, TREE_EFFECT_STATUS_SIPHON);
    if (siphon != 0 && gBattlerPartyIndexes[source->battler] == source->partyIndex && IsBattlerAlive(source->battler))
        sTreePendingHeal[source->battler] += max(GetNonDynamaxMaxHP(source->battler) * siphon / 10000, 1);
    return ApplyTreeIndirectGuard(battler, damage + damage * bonus / 10000);
}

// ---------------------------------------------------------------------------------------------
// Fin de tour (maîtrises Defense et Utility)

static void SetTreeNodeNameBuffer(const struct TreeNode *node)
{
    if (node != NULL)
        StringCopy(gBattleTextBuff3, node->name);
    else
        gBattleTextBuff3[0] = EOS;
}

static bool32 CanTreeHeal(enum BattlerId battler)
{
    return !gBattleMons[battler].volatiles.healBlockTimer && !IsBattlerAtMaxHp(battler);
}

static const u8 *SetTreeHeal(enum BattlerId battler, enum TreeEffect effect, u32 amount)
{
    SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, effect));
    SetHealAmount(battler, max(amount, 1));
    return BattleScript_TreeNodeHeal;
}

static const u8 *CureTreeStatus(enum BattlerId battler, enum TreeEffect effect)
{
    if (gBattleMons[battler].status1 & STATUS1_SLEEP)
        TryDeactivateSleepClause(battler, gBattlerPartyIndexes[battler]);
    gBattleMons[battler].status1 = 0;
    gBattleMons[battler].volatiles.nightmare = FALSE;
    BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[battler].status1);
    MarkBattlerForControllerExec(battler);
    SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, effect));
    gBattleScripting.battler = battler;
    return BattleScript_TreeStatusCure;
}

static bool32 TryTreeChance(enum RandomTag tag, u32 chance)
{
    if (chance == 0)
        return FALSE;
    if (chance >= 10000)
        return TRUE;
    return RandomChance(tag, chance, 10000);
}

static enum BattlerId GetTreePainSplitTarget(enum BattlerId battler)
{
    enum BattlerId foe = GetOppositeBattler(battler);
    if (!IsBattlerAlive(foe))
        foe = GetPartnerBattler(foe);
    return IsBattlerAlive(foe) ? foe : MAX_BATTLERS_COUNT;
}

// Appelé pour chaque Pokémon, encore et encore, tant qu'un script est renvoyé.
const u8 *TryTreeEndTurnEffect(enum BattlerId battler)
{
    u32 value;
    const struct TreeNode *node;

    if (!IsBattlerAlive(battler) || GetBattlerTreeType(battler) == TYPE_NONE)
        return NULL;

    // Venom Siphon, Night Terrors, Dark Recovery.
    if (sTreePendingHeal[battler] != 0)
    {
        u32 heal = sTreePendingHeal[battler];
        sTreePendingHeal[battler] = 0;
        if (CanTreeHeal(battler))
            return SetTreeHeal(battler, GetBattlerTreeEffect(battler, TREE_EFFECT_STATUS_SIPHON) ? TREE_EFFECT_STATUS_SIPHON : TREE_EFFECT_KO_HEAL, heal);
    }

    if (sTreeEndTurnStep[battler] == 0)
    {
        sTreeEndTurnStep[battler]++;
        value = GetBattlerTreeEffect(battler, TREE_EFFECT_END_TURN_HEAL);
        if (value != 0 && CanTreeHeal(battler))
            return SetTreeHeal(battler, TREE_EFFECT_END_TURN_HEAL, GetNonDynamaxMaxHP(battler) * value / 10000);
    }

    if (gBattleMons[battler].hp * 3 < gBattleMons[battler].maxHP
     && IsTreeOnceReady(battler, TREE_EFFECT_EMERGENCY_HEAL, TREE_ONCE_EMERGENCY_HEAL)
     && CanTreeHeal(battler))
    {
        SetTreeOnceUsed(battler, TREE_ONCE_EMERGENCY_HEAL);
        return SetTreeHeal(battler, TREE_EFFECT_EMERGENCY_HEAL,
                           GetNonDynamaxMaxHP(battler) * GetBattlerTreeEffect(battler, TREE_EFFECT_EMERGENCY_HEAL) / 10000);
    }

    if (gBattleMons[battler].hp * 2 < gBattleMons[battler].maxHP
     && IsTreeOnceReady(battler, TREE_EFFECT_AQUA_RING_ONCE, TREE_ONCE_AQUA_RING))
    {
        SetTreeOnceUsed(battler, TREE_ONCE_AQUA_RING);
        if (!gBattleMons[battler].volatiles.aquaRing)
        {
            gBattleMons[battler].volatiles.aquaRing = TRUE;
            SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, TREE_EFFECT_AQUA_RING_ONCE));
            return BattleScript_TreeAquaRing;
        }
    }

    if (sTreeEndTurnStep[battler] == 1)
    {
        sTreeEndTurnStep[battler]++;
        node = GetBattlerTreeEffectNode(battler, TREE_EFFECT_STATUS_CURE_END_TURN);
        if (node != NULL && (gBattleMons[battler].status1 & STATUS1_ANY)
         && (node->param == 0 || (HasWeatherEffect() && (gBattleWeather & node->param)))
         && TryTreeChance(RNG_TREE_STATUS_CURE, GetBattlerTreeEffect(battler, TREE_EFFECT_STATUS_CURE_END_TURN)))
            return CureTreeStatus(battler, TREE_EFFECT_STATUS_CURE_END_TURN);
    }

    // Molt : statut et baisses de stats, une fois, dès qu'il y a quelque chose à muer.
    if (IsTreeOnceReady(battler, TREE_EFFECT_MOLT, TREE_ONCE_MOLT))
    {
        u32 stat;
        bool32 lowered = FALSE;
        for (stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
        {
            if (gBattleMons[battler].statStages[stat] < DEFAULT_STAT_STAGE)
                lowered = TRUE;
        }
        if (lowered || (gBattleMons[battler].status1 & STATUS1_ANY))
        {
            SetTreeOnceUsed(battler, TREE_ONCE_MOLT);
            for (stat = STAT_ATK; stat < NUM_BATTLE_STATS; stat++)
            {
                if (gBattleMons[battler].statStages[stat] < DEFAULT_STAT_STAGE)
                    gBattleMons[battler].statStages[stat] = DEFAULT_STAT_STAGE;
            }
            if (gBattleMons[battler].status1 & STATUS1_ANY)
                return CureTreeStatus(battler, TREE_EFFECT_MOLT);
            SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, TREE_EFFECT_MOLT));
            gBattleScripting.battler = battler;
            return BattleScript_TreeMolt;
        }
    }

    if (sTreeEndTurnStep[battler] == 2)
    {
        sTreeEndTurnStep[battler]++;
        if (gBattleMons[battler].volatiles.confusionTimer
         && TryTreeChance(RNG_TREE_CONFUSION_SNAP, GetBattlerTreeEffect(battler, TREE_EFFECT_CONFUSION_SNAP)))
        {
            gBattleMons[battler].volatiles.confusionTimer = 0;
            SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, TREE_EFFECT_CONFUSION_SNAP));
            return BattleScript_TreeConfusionSnap;
        }
    }

    if (gBattleMons[battler].hp * 4 < gBattleMons[battler].maxHP
     && IsTreeOnceReady(battler, TREE_EFFECT_PAIN_SPLIT_ONCE, TREE_ONCE_PAIN_SPLIT))
    {
        enum BattlerId foe = GetTreePainSplitTarget(battler);
        if (foe != MAX_BATTLERS_COUNT && gBattleMons[foe].hp > gBattleMons[battler].hp)
        {
            s32 share = (gBattleMons[battler].hp + gBattleMons[foe].hp) / 2;
            SetTreeOnceUsed(battler, TREE_ONCE_PAIN_SPLIT);
            gBattleStruct->passiveHpUpdate[battler] = gBattleMons[battler].hp - share;
            gBattleStruct->passiveHpUpdate[foe] = gBattleMons[foe].hp - share;
            gBattlerTarget = foe;
            SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, TREE_EFFECT_PAIN_SPLIT_ONCE));
            return BattleScript_TreePainSplit;
        }
    }

    sTreeEndTurnStep[battler] = 0;
    return NULL;
}

// ---------------------------------------------------------------------------------------------
// Précision, vitesse, priorité et types

u32 ApplyTreeAccuracy(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, u32 calc)
{
    u32 bonus = GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_ACCURACY);

    if (IsBattleMoveStatus(move))
        bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_STATUS_MOVE_ACCURACY);
    calc += calc * bonus / 10000;
    if (battlerAtk != battlerDef && GetBattlerSide(battlerAtk) != GetBattlerSide(battlerDef))
        calc -= calc * min(GetBattlerTreeEffect(battlerDef, TREE_EFFECT_FOE_ACCURACY_DOWN), 10000) / 10000;
    return calc;
}

bool32 TreeMoveNeverMisses(enum BattlerId battlerAtk, enum Move move)
{
    return GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_NEVER_MISS) != 0
        && GetBattleMoveType(move) == GetBattlerTreeType(battlerAtk);
}

u32 ApplyTreeFieldSpeed(enum BattlerId battler, u32 speed, u32 weather)
{
    const struct ElementTree *tree = GetBattlerTree(battler);
    u32 bonus;

    if (tree == NULL || (bonus = GetBattlerTreeEffect(battler, TREE_EFFECT_FIELD_SPEED)) == 0)
        return speed;
    if (IsTreeWeatherActive(tree, weather) || IsTreeTerrainActive(tree))
        speed += speed * bonus / 10000;
    return speed;
}

bool32 TreeGivesPriority(enum BattlerId battler, enum Move move)
{
    return GetBattlerTreeEffect(battler, TREE_EFFECT_PRIORITY_FULL_HP) != 0
        && IsBattlerAtMaxHp(battler)
        && GetMoveType(move) == GetBattlerTreeType(battler);
}

enum Type GetTreeAteType(enum BattlerId battler)
{
    enum Type treeType = GetBattlerTreeType(battler);

    if (treeType == TYPE_NONE || treeType == TYPE_NORMAL || GetBattlerTreeEffect(battler, TREE_EFFECT_NORMAL_TO_TYPE) == 0)
        return TYPE_NONE;
    return treeType;
}

// Scrappy (Normal et Combat contre Spectre), Grounding Blow (Sol contre Vol et lévitation).
bool32 TreeMoveIgnoresImmunity(enum BattlerId battlerAtk, enum Type moveType, enum Type defType)
{
    if (defType == TYPE_GHOST && (moveType == TYPE_NORMAL || moveType == TYPE_FIGHTING))
        return GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_HIT_GHOST) != 0;
    if (defType == TYPE_FLYING && moveType == TYPE_GROUND)
        return GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_HIT_AIRBORNE) != 0;
    return FALSE;
}

bool32 IsTreeGroundImmune(enum BattlerId battlerDef)
{
    return IsTreeOnceReady(battlerDef, TREE_EFFECT_GROUND_IMMUNITY_ONCE, TREE_ONCE_GROUND_IMMUNITY);
}

// Magnet Rise : dépensé quand l'attaque Sol échoue contre lui.
void SpendTreeGroundImmunity(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Type moveType)
{
    if (moveType == TYPE_GROUND && battlerAtk != battlerDef && IsTreeGroundImmune(battlerDef))
        SetTreeOnceUsed(battlerDef, TREE_ONCE_GROUND_IMMUNITY);
}

// ---------------------------------------------------------------------------------------------
// Chances et effets secondaires

bool32 TryTreeChanceEffect(enum BattlerId battler, enum TreeEffect effect, u32 tag)
{
    return TryTreeChance(tag, GetBattlerTreeEffect(battler, effect));
}

bool32 TryTreeMaxMultiHit(enum BattlerId battlerAtk)
{
    return TryTreeChanceEffect(battlerAtk, TREE_EFFECT_MULTI_HIT_MAX, RNG_TREE_MULTI_HIT);
}

bool32 TryTreeResistVolatile(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    return battlerAtk != battlerDef && GetBattlerSide(battlerAtk) != GetBattlerSide(battlerDef)
        && TryTreeChanceEffect(battlerDef, TREE_EFFECT_FLINCH_CONFUSION_RESIST, RNG_TREE_RESIST);
}

s32 ApplyTreeDrainBonus(enum BattlerId battlerAtk, s32 heal)
{
    return heal + heal * (s32)GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_DRAIN_BONUS) / 10000;
}

u32 ApplyTreeHealingMoveBoost(enum BattlerId battler, u32 heal)
{
    return heal + heal * GetBattlerTreeEffect(battler, TREE_EFFECT_HEALING_MOVE_BOOST) / 10000;
}

u32 ApplyTreeIndirectGuard(enum BattlerId battler, u32 damage)
{
    u32 guard = min(GetBattlerTreeEffect(battler, TREE_EFFECT_INDIRECT_GUARD), 10000);

    if (guard == 0 || damage == 0)
        return damage;
    return max(damage - damage * guard / 10000, 1);
}

void SetTreeNodeNameForEffect(enum BattlerId battler, enum TreeEffect effect)
{
    SetTreeNodeNameBuffer(GetBattlerTreeEffectNode(battler, effect));
}

static EWRAM_DATA bool8 sTreeSynchronizePending = FALSE;

bool32 TreeHasSynchronize(enum BattlerId battler, enum BattlerId battlerAtk)
{
    return battlerAtk < gBattlersCount && GetBattlerSide(battler) != GetBattlerSide(battlerAtk)
        && GetBattlerTreeEffect(battler, TREE_EFFECT_SYNCHRONIZE) != 0;
}

void SetTreeSynchronizePending(bool32 pending)
{
    sTreeSynchronizePending = pending;
}

bool32 IsTreeSynchronizePending(void)
{
    return sTreeSynchronizePending;
}

// Misty Veil : un statut infligé par un adversaire peut être évité.
bool32 TryTreeAvoidStatus(enum BattlerId battlerAtk, enum BattlerId battlerDef)
{
    if (battlerAtk == battlerDef || battlerAtk >= gBattlersCount || GetBattlerSide(battlerAtk) == GetBattlerSide(battlerDef)
     || !TryTreeChanceEffect(battlerDef, TREE_EFFECT_STATUS_AVOID, RNG_TREE_STATUS_AVOID))
        return FALSE;
    SetTreeNodeNameForEffect(battlerDef, TREE_EFFECT_STATUS_AVOID);
    return TRUE;
}

// Sturdy et Endure : à PV max, survit une fois à un coup fatal.
bool32 TryTreeEndure(enum BattlerId battlerDef)
{
    if (!IsBattlerAtMaxHp(battlerDef) || !IsTreeOnceReady(battlerDef, TREE_EFFECT_ENDURE, TREE_ONCE_ENDURE))
        return FALSE;
    SetTreeOnceUsed(battlerDef, TREE_ONCE_ENDURE);
    return TRUE;
}

// Dark Veil : la première attaque de statut d'un adversaire qui le vise échoue.
bool32 TryTreeStatusMoveBlock(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move)
{
    if (!IsBattleMoveStatus(move) || battlerAtk == battlerDef || GetBattlerSide(battlerAtk) == GetBattlerSide(battlerDef)
     || !IsTreeOnceReady(battlerDef, TREE_EFFECT_STATUS_MOVE_BLOCK, TREE_ONCE_STATUS_MOVE_BLOCK))
        return FALSE;
    SetTreeOnceUsed(battlerDef, TREE_ONCE_STATUS_MOVE_BLOCK);
    return TRUE;
}

// Natural Cure et Aerial Retreat : en quittant le terrain.
void TryTreeSwitchOutEffects(enum BattlerId battler)
{
    struct Pokemon *mon = &GetBattlerParty(battler)[gBattlerPartyIndexes[battler]];
    u32 heal;

    if (gBattleMons[battler].hp == 0 || GetBattlerTreeType(battler) == TYPE_NONE)
        return;

    if ((gBattleMons[battler].status1 & STATUS1_ANY)
     && TryTreeChanceEffect(battler, TREE_EFFECT_STATUS_CURE_SWITCH, RNG_TREE_STATUS_CURE))
    {
        if (gBattleMons[battler].status1 & STATUS1_SLEEP)
            TryDeactivateSleepClause(battler, gBattlerPartyIndexes[battler]);
        gBattleMons[battler].status1 = 0;
        SetMonData(mon, MON_DATA_STATUS, &gBattleMons[battler].status1);
    }

    heal = GetBattlerTreeEffect(battler, TREE_EFFECT_SWITCH_OUT_HEAL);
    if (heal != 0 && gBattleMons[battler].hp < gBattleMons[battler].maxHP)
    {
        u32 hp = min(gBattleMons[battler].hp + max(GetNonDynamaxMaxHP(battler) * heal / 10000, 1), gBattleMons[battler].maxHP);
        gBattleMons[battler].hp = hp;
        SetMonData(mon, MON_DATA_HP, &hp);
    }
}

// Thief's Cut, Fortune : le meilleur bonus de l'équipe du joueur.
u32 ApplyTreeMoneyBonus(u32 money)
{
    u32 i, bonus = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct BoxPokemon *boxMon = &gParties[B_TRAINER_PLAYER][i].box;
        if (GetBoxMonData(boxMon, MON_DATA_SPECIES_OR_EGG) != SPECIES_NONE && GetBoxMonData(boxMon, MON_DATA_SPECIES_OR_EGG) != SPECIES_EGG)
            bonus = max(bonus, GetBoxMonTreeEffect(boxMon, TREE_EFFECT_MONEY_BONUS));
    }
    return money + money * bonus / 10000;
}

bool32 TryTreePickup(struct BoxPokemon *boxMon)
{
    u32 chance = GetBoxMonTreeEffect(boxMon, TREE_EFFECT_PICKUP);
    return chance != 0 && RandomChance(RNG_TREE_PICKUP, chance, 10000);
}

// ---------------------------------------------------------------------------------------------
// Fin d'attaque : contact, coups reçus, KO et vol de vie

static enum Stat GetTreeBestAttackingStat(enum BattlerId battler)
{
    return gBattleMons[battler].spAttack > gBattleMons[battler].attack ? STAT_SPATK : STAT_ATK;
}

static EWRAM_DATA u8 sTreeMoveEndStep = 0;

enum
{
    TREE_MOVEEND_CONTACT_STATUS,
    TREE_MOVEEND_CONTACT_WEAKEN,
    TREE_MOVEEND_CONTACT_DAMAGE,
    TREE_MOVEEND_DISABLE,
    TREE_MOVEEND_TOXIC_DEBRIS,
    TREE_MOVEEND_BERSERK,
    TREE_MOVEEND_LAST_HAUNT,
    TREE_MOVEEND_TYPE_DRAIN,
    TREE_MOVEEND_DEF_DROP,
    TREE_MOVEEND_COUNT,
    // Après l'annonce des K.O., comme Moxie
    TREE_MOVEEND_MOXIE = TREE_MOVEEND_COUNT,
    TREE_MOVEEND_KO_HEAL,
    TREE_MOVEEND_FOES_FAINTED_COUNT,
};

static const u8 *TryTreeMoveEndStep(struct BattleCalcValues *cv, u32 step)
{
    enum BattlerId atk = cv->battlerAtk, def = cv->battlerDef;
    bool32 defHit = IsBattlerTurnDamaged(def, EXCLUDING_SUBSTITUTES);
    bool32 atkAlive = IsBattlerAlive(atk);
    const struct TreeNode *node;
    u32 value;

    switch (step)
    {
    case TREE_MOVEEND_CONTACT_STATUS:
        node = GetBattlerTreeEffectNode(def, TREE_EFFECT_CONTACT_STATUS);
        if (node != NULL && defHit && atkAlive
         && !CanBattlerAvoidContactEffects(atk, def, cv->abilities[atk], cv->holdEffects[atk], cv->move))
        {
            enum MoveEffect moveEffect = GetTreeStatusMoveEffect(node->param);
            if (moveEffect != MOVE_EFFECT_NONE
             && CanSetNonVolatileStatus(def, atk, cv->abilities[def], cv->abilities[atk], moveEffect, CHECK_TRIGGER)
             && TryTreeChanceEffect(def, TREE_EFFECT_CONTACT_STATUS, RNG_TREE_CONTACT))
            {
                gEffectBattler = atk;
                gBattleScripting.battler = def;
                gBattleScripting.moveEffect = moveEffect;
                SetTreeNodeNameBuffer(node);
                return BattleScript_TreeContactStatus;
            }
        }
        break;
    case TREE_MOVEEND_CONTACT_WEAKEN:
        if (defHit && atkAlive && GetBattlerTreeEffect(def, TREE_EFFECT_CONTACT_WEAKEN)
         && !CanBattlerAvoidContactEffects(atk, def, cv->abilities[atk], cv->holdEffects[atk], cv->move)
         && gBattleMons[atk].statStages[STAT_ATK] > MIN_STAT_STAGE
         && TryTreeChanceEffect(def, TREE_EFFECT_CONTACT_WEAKEN, RNG_TREE_CONTACT))
        {
            SetStatChange(atk, STAT_ATK, -1);
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_CONTACT_WEAKEN);
            return BattleScript_TreeStatChange;
        }
        break;
    case TREE_MOVEEND_CONTACT_DAMAGE:
        value = GetBattlerTreeEffect(def, TREE_EFFECT_CONTACT_DAMAGE);
        if (value != 0 && defHit && atkAlive && cv->abilities[atk] != ABILITY_MAGIC_GUARD
         && !CanBattlerAvoidContactEffects(atk, def, cv->abilities[atk], cv->holdEffects[atk], cv->move))
        {
            SetPassiveDamageAmount(atk, max(GetNonDynamaxMaxHP(atk) * value / 10000, 1));
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_CONTACT_DAMAGE);
            return BattleScript_TreeHurtAttacker;
        }
        break;
    case TREE_MOVEEND_DISABLE:
        if (defHit && atkAlive && GetBattlerTreeEffect(def, TREE_EFFECT_DISABLE_ON_HIT)
         && GetMoveEffect(cv->move) != EFFECT_FUTURE_SIGHT
         && gBattleMons[atk].volatiles.disabledMove == MOVE_NONE
         && gChosenMove != MOVE_STRUGGLE
         && GetActiveGimmick(atk) != GIMMICK_DYNAMAX
         && TryTreeChanceEffect(def, TREE_EFFECT_DISABLE_ON_HIT, RNG_TREE_DISABLE))
        {
            gBattleMons[atk].volatiles.disabledMove = gChosenMove;
            gBattleMons[atk].volatiles.disableTimer = B_DISABLE_TIMER;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gChosenMove);
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_DISABLE_ON_HIT);
            return BattleScript_TreeDisable;
        }
        break;
    case TREE_MOVEEND_TOXIC_DEBRIS:
        if (defHit && IsBattleMovePhysical(cv->move) && !gBattleStruct->isSkyBattle
         && gSideTimers[GetBattlerSide(atk)].toxicSpikesAmount < 2
         && IsTreeOnceReady(def, TREE_EFFECT_TOXIC_DEBRIS, TREE_ONCE_TOXIC_DEBRIS))
        {
            SetTreeOnceUsed(def, TREE_ONCE_TOXIC_DEBRIS);
            SaveBattlerTarget(gBattlerTarget);
            SaveBattlerAttacker(gBattlerAttacker);
            gBattlerAttacker = def;
            gBattlerTarget = atk;
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_TOXIC_DEBRIS);
            return BattleScript_TreeToxicDebris;
        }
        break;
    case TREE_MOVEEND_BERSERK:
        if (defHit && IsBattlerAlive(def) && gBattleMons[def].hp * 2 < gBattleMons[def].maxHP
         && IsTreeOnceReady(def, TREE_EFFECT_BERSERK, TREE_ONCE_BERSERK))
        {
            SetTreeOnceUsed(def, TREE_ONCE_BERSERK);
            SetStatChange(def, STAT_ATK, 1);
            SetStatChange(def, STAT_SPATK, 1);
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_BERSERK);
            return BattleScript_TreeStatChange;
        }
        break;
    case TREE_MOVEEND_LAST_HAUNT:
        value = GetBattlerTreeEffect(def, TREE_EFFECT_LAST_HAUNT);
        if (value != 0 && defHit && !IsBattlerAlive(def) && atkAlive && cv->abilities[atk] != ABILITY_MAGIC_GUARD)
        {
            SetPassiveDamageAmount(atk, max(GetNonDynamaxMaxHP(atk) * value / 10000, 1));
            gBattleScripting.battler = def;
            SetTreeNodeNameForEffect(def, TREE_EFFECT_LAST_HAUNT);
            return BattleScript_TreeHurtAttacker;
        }
        break;
    case TREE_MOVEEND_TYPE_DRAIN:
        value = GetBattlerTreeEffect(atk, TREE_EFFECT_TYPE_DRAIN);
        if (value != 0 && defHit && atkAlive && CanTreeHeal(atk) && gBattleStruct->moveDamage[def] > 0
         && GetBattleMoveType(cv->move) == GetBattlerTreeType(atk))
            return SetTreeHeal(atk, TREE_EFFECT_TYPE_DRAIN, gBattleStruct->moveDamage[def] * value / 10000);
        break;
    case TREE_MOVEEND_DEF_DROP:
        if (defHit && atkAlive && IsBattlerAlive(def)
         && GetBattleMoveType(cv->move) == GetBattlerTreeType(atk)
         && gBattleMons[def].statStages[STAT_DEF] > MIN_STAT_STAGE
         && TryTreeChanceEffect(atk, TREE_EFFECT_ADDED_DEF_DROP, RNG_TREE_DEF_DROP))
        {
            SetStatChange(def, STAT_DEF, -1);
            gBattleScripting.battler = atk;
            SetTreeNodeNameForEffect(atk, TREE_EFFECT_ADDED_DEF_DROP);
            return BattleScript_TreeStatChange;
        }
        break;
    }
    return NULL;
}

// Rappelé tant qu'un script est renvoyé ; chaque effet n'est essayé qu'une fois par cible.
const u8 *TryTreeMoveEndEffect(struct BattleCalcValues *cv)
{
    if (cv->battlerAtk != cv->battlerDef && GetBattlerSide(cv->battlerAtk) != GetBattlerSide(cv->battlerDef)
     && !gSpecialStatuses[cv->battlerAtk].attackerInParty)
    {
        while (sTreeMoveEndStep < TREE_MOVEEND_COUNT)
        {
            const u8 *script = TryTreeMoveEndStep(cv, sTreeMoveEndStep++);
            if (script != NULL)
                return script;
        }
    }
    sTreeMoveEndStep = 0;
    return NULL;
}

static bool32 HasTreeKnockedOutFoe(enum BattlerId battlerAtk)
{
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        if (GetBattlerSide(battler) != GetBattlerSide(battlerAtk)
         && IsBattlerTurnDamaged(battler, EXCLUDING_SUBSTITUTES) && !IsBattlerAlive(battler))
            return TRUE;
    }
    return FALSE;
}

static const u8 *TryTreeFoesFaintedStep(struct BattleCalcValues *cv, u32 step)
{
    enum BattlerId atk = cv->battlerAtk;
    u32 value;

    switch (step)
    {
    case TREE_MOVEEND_MOXIE:
        if (!NoAliveMonsForEitherParty()
         && GetBattlerTreeEffect(atk, TREE_EFFECT_MOXIE)
         && gBattleMons[atk].statStages[GetTreeBestAttackingStat(atk)] < MAX_STAT_STAGE)
        {
            SetStatChange(atk, GetTreeBestAttackingStat(atk), 1);
            gBattleScripting.battler = atk;
            SetTreeNodeNameForEffect(atk, TREE_EFFECT_MOXIE);
            return BattleScript_TreeStatChange;
        }
        break;
    case TREE_MOVEEND_KO_HEAL:
        value = GetBattlerTreeEffect(atk, TREE_EFFECT_KO_HEAL);
        if (value != 0 && CanTreeHeal(atk))
            return SetTreeHeal(atk, TREE_EFFECT_KO_HEAL, GetNonDynamaxMaxHP(atk) * value / 10000);
        break;
    }
    return NULL;
}

// Mod : Moxie et Dark Recovery, une fois les K.O. annoncés.
const u8 *TryTreeFoesFaintedEffect(struct BattleCalcValues *cv)
{
    if (sTreeMoveEndStep < TREE_MOVEEND_COUNT)
        sTreeMoveEndStep = TREE_MOVEEND_COUNT;
    if (IsBattlerAlive(cv->battlerAtk) && !gSpecialStatuses[cv->battlerAtk].attackerInParty
     && HasTreeKnockedOutFoe(cv->battlerAtk))
    {
        while (sTreeMoveEndStep < TREE_MOVEEND_FOES_FAINTED_COUNT)
        {
            const u8 *script = TryTreeFoesFaintedStep(cv, sTreeMoveEndStep++);
            if (script != NULL)
                return script;
        }
    }
    sTreeMoveEndStep = 0;
    return NULL;
}

// ---------------------------------------------------------------------------------------------
// Effets d'entrée : météo, terrain, piège, Tailwind, Reflect, stats

static const u8 *TryTreeEntryWeather(enum BattlerId battler, const struct ElementTree *tree)
{
    u32 weather = GetTreeBattleWeather(tree->weather);

    if (weather == BATTLE_WEATHER_COUNT || (gBattleWeather & gBattleWeatherInfo[weather].flag))
        return NULL;
    if (TryChangeBattleWeather(battler, weather, ABILITY_NONE) != WEATHER_FAILURE_SUCCESS)
        return NULL;
    gBattleStruct->weatherDuration = MOD_TREE_ENTRY_WEATHER_TURNS + GetTreeWeatherTurnsBonus(battler, gBattleWeather);
    gBattleScripting.battler = battler;
    gBattleScripting.animArg1 = gBattleWeatherInfo[weather].animation;
    return BattleScript_TreeWeatherOnEntry;
}

static const u8 *TryTreeEntryHazard(enum BattlerId battler, u32 hazard)
{
    enum BattlerId foe = GetOppositeBattler(battler);
    enum BattleSide side = GetBattlerSide(foe);

    switch (hazard)
    {
    case TREE_HAZARD_STEALTH_ROCK:
        if (IsHazardOnSide(side, HAZARDS_STEALTH_ROCK))
            return NULL;
        PushHazardTypeToQueue(side, HAZARDS_STEALTH_ROCK);
        break;
    case TREE_HAZARD_TOXIC_SPIKES:
        if (gSideTimers[side].toxicSpikesAmount >= 2)
            return NULL;
        if (gSideTimers[side].toxicSpikesAmount == 0)
            PushHazardTypeToQueue(side, HAZARDS_TOXIC_SPIKES);
        gSideTimers[side].toxicSpikesAmount++;
        break;
    case TREE_HAZARD_STICKY_WEB:
        if (IsHazardOnSide(side, HAZARDS_STICKY_WEB))
            return NULL;
        PushHazardTypeToQueue(side, HAZARDS_STICKY_WEB);
        gSideTimers[side].stickyWebBattlerId = battler;
        gSideTimers[side].stickyWebBattlerSide = GetBattlerSide(battler);
        break;
    default:
        return NULL;
    }
    gBattleCommunication[MULTISTRING_CHOOSER] = hazard;
    SaveBattlerAttacker(gBattlerAttacker);
    SaveBattlerTarget(gBattlerTarget);
    gBattlerAttacker = battler;
    gBattlerTarget = foe;
    return BattleScript_TreeHazardOnEntry;
}

static const u8 *TryTreeEntryStat(enum BattlerId battler, u32 stat)
{
    enum BattlerId foe;
    bool32 any = FALSE;

    if (stat == TREE_STAT_SELF_BEST)
    {
        stat = GetTreeBestAttackingStat(battler);
        if (gBattleMons[battler].statStages[stat] >= MAX_STAT_STAGE)
            return NULL;
        SetStatChange(battler, stat, 1);
        return BattleScript_TreeStatChange;
    }
    for (foe = 0; foe < gBattlersCount; foe++)
    {
        if (GetBattlerSide(foe) != GetBattlerSide(battler) && IsBattlerAlive(foe))
        {
            SetStatChange(foe, stat, -1);
            any = TRUE;
        }
    }
    return any ? BattleScript_TreeStatChange : NULL;
}

const u8 *TryTreeEntryEffect(enum BattlerId battler)
{
    const struct ElementTree *tree = GetBattlerTree(battler);
    static const u8 sEntryEffects[] =
    {
        TREE_EFFECT_WEATHER_ON_ENTRY, TREE_EFFECT_TERRAIN_ON_ENTRY, TREE_EFFECT_HAZARD_ON_ENTRY,
        TREE_EFFECT_TAILWIND_ON_ENTRY, TREE_EFFECT_SCREEN_ON_ENTRY, TREE_EFFECT_STAT_ON_ENTRY,
    };
    const struct TreeNode *node = NULL;
    enum BattleSide side = GetBattlerSide(battler);
    const u8 *script = NULL;
    u32 i;

    if (tree == NULL || !IsBattlerAlive(battler) || IsTreeOnceUsed(battler, TREE_ONCE_ENTRY))
        return NULL;
    for (i = 0; i < ARRAY_COUNT(sEntryEffects) && node == NULL; i++)
        node = GetBattlerTreeEffectNode(battler, sEntryEffects[i]);
    if (node == NULL)
        return NULL;
    SetTreeOnceUsed(battler, TREE_ONCE_ENTRY);
    gBattleScripting.battler = battler;

    switch (node->effect)
    {
    case TREE_EFFECT_WEATHER_ON_ENTRY:
        if (tree->weather != 0)
            script = TryTreeEntryWeather(battler, tree);
        break;
    case TREE_EFFECT_TERRAIN_ON_ENTRY:
        if (TryChangeBattleTerrain(battler, tree->terrain))
        {
            gFieldTimers.terrainTimer = MOD_TREE_ENTRY_TERRAIN_TURNS + GetTreeTerrainTurnsBonus(battler, tree->terrain);
            script = BattleScript_TreeTerrainOnEntry;
        }
        break;
    case TREE_EFFECT_HAZARD_ON_ENTRY:
        script = TryTreeEntryHazard(battler, node->param);
        break;
    case TREE_EFFECT_TAILWIND_ON_ENTRY:
        if (!(gSideStatuses[side] & SIDE_STATUS_TAILWIND))
        {
            gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
            gSideTimers[side].tailwindTimer = MOD_TREE_ENTRY_TAILWIND_TURNS + GetTreeTailwindTurnsBonus(battler);
            SaveBattlerAttacker(gBattlerAttacker);
            gBattlerAttacker = battler;
            script = BattleScript_TreeTailwindOnEntry;
        }
        break;
    case TREE_EFFECT_SCREEN_ON_ENTRY:
        if (TrySetReflect(battler))
        {
            gSideTimers[side].reflectTimer = MOD_TREE_ENTRY_SCREEN_TURNS + GetTreeScreenTurnsBonus(battler);
            SaveBattlerAttacker(gBattlerAttacker);
            gBattlerAttacker = battler;
            script = BattleScript_TreeScreenOnEntry;
        }
        break;
    case TREE_EFFECT_STAT_ON_ENTRY:
        script = TryTreeEntryStat(battler, node->param);
        break;
    default:
        break;
    }

    if (script != NULL)
    {
        gBattleScripting.battler = battler;
        SetTreeNodeNameBuffer(node);
    }
    return script;
}
