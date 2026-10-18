#include "global.h"
#include "battle.h"
#include "battle_dynamax.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "random.h"
#include "pokemon_build.h"
#include "constants/battle.h"
#include "constants/battle_set_effect.h"

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

static u32 GetTrunkPointsSpent(const struct BoxPokemon *boxMon)
{
    u32 node, total = 0;
    for (node = 0; node < TREE_TRUNK_NODES; node++)
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

    if (node >= TREE_TRUNK_NODES)
    {
        u32 indexInBranch = (node - TREE_TRUNK_NODES) % TREE_BRANCH_NODES;
        if (indexInBranch == 0)
        {
            if (GetTrunkPointsSpent(boxMon) < MOD_TREE_BRANCH_UNLOCK_POINTS)
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
        if (GetBoxMonTreeRank(boxMon, TREE_TRUNK_NODES + branch * TREE_BRANCH_NODES + TREE_BRANCH_NODES - 1) > 0)
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

u32 GetBattlerTreeEffect(enum BattlerId battler, enum TreeEffect effect)
{
    return GetBoxMonTreeEffect(GetBattlerBoxMon(battler), effect);
}

const struct ElementTree *GetBattlerTree(enum BattlerId battler)
{
    enum Type treeType = GetBattlerBoxMon(battler)->build.treeType;
    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return NULL;
    return &gElementTrees[treeType];
}

// Les bonus de dégâts des arbres s'additionnent entre eux (proposition du wiki, ordre encore ouvert).
uq4_12_t GetTreeDamageModifier(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, enum Type moveType)
{
    const struct BoxPokemon *boxMon = GetBattlerBoxMon(battlerAtk);
    enum Type treeType = boxMon->build.treeType;
    const struct ElementTree *tree;
    u32 bonus = 0;

    if (treeType == TYPE_NONE || treeType >= NUMBER_OF_MON_TYPES)
        return UQ_4_12(1.0);
    tree = &gElementTrees[treeType];

    if (IsBattleMovePhysical(move))
        bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_PHYSICAL_DAMAGE);
    else if (IsBattleMoveSpecial(move))
        bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_SPECIAL_DAMAGE);

    if (moveType == treeType)
    {
        if (IsBattleMovePhysical(move))
            bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_PHYSICAL_DAMAGE);
        else if (IsBattleMoveSpecial(move))
            bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_SPECIAL_DAMAGE);
        if (gBattleMons[battlerAtk].hp * 2 < gBattleMons[battlerAtk].maxHP)
            bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_DAMAGE_LOW_HP);
        if (tree->weather != 0 && HasWeatherEffect() && (gBattleWeather & tree->weather))
            bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_DAMAGE_WEATHER);
        bonus += GetTreeHeartStacks(battlerAtk) * GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_DAMAGE_STACK);
    }

    if (tree->status != 0 && (gBattleMons[battlerDef].status1 & tree->status))
        bonus += GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_DAMAGE_VS_STATUS);

    return UQ_4_12(1.0) + (bonus * UQ_4_12(1.0)) / 1000;
}

// Garde : réduit les dégâts des attaques super efficaces subies (multiplicatif, à part des bonus).
uq4_12_t GetTreeGuardModifier(enum BattlerId battlerDef, uq4_12_t typeEffectiveness)
{
    u32 guard;

    if (typeEffectiveness <= UQ_4_12(1.0))
        return UQ_4_12(1.0);
    guard = min(GetBattlerTreeEffect(battlerDef, TREE_EFFECT_WEAKNESS_GUARD), 1000);
    return UQ_4_12(1.0) - (guard * UQ_4_12(1.0)) / 1000;
}

u32 ApplyTreeRecoilReduction(enum BattlerId battlerAtk, enum Type moveType, u32 recoil)
{
    u32 reduction;

    if (GetBattlerBoxMon(battlerAtk)->build.treeType != moveType)
        return recoil;
    reduction = min(GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_RECOIL), 1000);
    if (reduction == 0)
        return recoil;
    recoil = recoil * (1000 - reduction) / 1000;
    return max(recoil, 1);
}

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

// En points de pourcentage entiers : deux rangs à 0,5 point donnent 1 point.
u32 GetTreeStatusChanceBonus(enum BattlerId battlerAtk, u32 moveEffect)
{
    const struct ElementTree *tree = GetBattlerTree(battlerAtk);

    if (tree == NULL || tree->status == 0 || GetTreeStatusMoveEffect(tree->status) != moveEffect)
        return 0;
    return GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_STATUS_CHANCE) / 10;
}

u32 GetTreeWeatherTurnsBonus(enum BattlerId battler, u32 weatherFlag)
{
    const struct ElementTree *tree = GetBattlerTree(battler);

    if (tree == NULL || !(tree->weather & weatherFlag))
        return 0;
    return GetBattlerTreeEffect(battler, TREE_EFFECT_WEATHER_TURNS);
}

u32 GetTreeWeatherHealAmount(enum BattlerId battler)
{
    const struct ElementTree *tree = GetBattlerTree(battler);
    u32 heal;

    if (tree == NULL || tree->weather == 0 || !HasWeatherEffect() || !(gBattleWeather & tree->weather))
        return 0;
    heal = GetBattlerTreeEffect(battler, TREE_EFFECT_WEATHER_HEAL);
    if (heal == 0)
        return 0;
    return max(GetNonDynamaxMaxHP(battler) * heal / 1000, 1);
}

// Maîtrises finales actives (wiki section 5) : Cœur (branche Assaut) et Aube (branche Météo).
// Cœur : cumuls perdus au changement de Pokémon. Aube : une fois par combat et par Pokémon.
static EWRAM_DATA u8 sTreeHeartStacks[MAX_BATTLERS_COUNT] = {0};
static EWRAM_DATA u8 sTreeEntryWeatherUsed[NUM_BATTLE_SIDES] = {0}; // Un bit par place dans l'équipe.

// Statuts suivis (Attiser les flammes, Braises tenaces) : qui a infligé le statut de chaque cible.
struct TreeStatusSource
{
    u8 battler;
    u8 partyIndex;
    bool8 valid;
};

enum
{
    TREE_LINGER_NONE,
    TREE_LINGER_WATCHING, // Le statut est en place ; on guette son soin.
    TREE_LINGER_RETURNED, // Soigné puis revenu pour MOD_TREE_LINGER_TURNS tours.
    TREE_LINGER_DONE,
};

struct TreeLinger
{
    u32 status;
    u16 turn;
    u8 state;
};

static EWRAM_DATA struct TreeStatusSource sTreeStatusSource[MAX_BATTLERS_COUNT] = {0};
static EWRAM_DATA struct TreeLinger sTreeLinger[MAX_BATTLERS_COUNT] = {0};
static EWRAM_DATA u8 sTreeLingerUsed[NUM_BATTLE_SIDES] = {0};        // Un bit par place dans l'équipe.
static EWRAM_DATA u8 sTreeFirstWeaknessUsed[NUM_BATTLE_SIDES] = {0}; // Un bit par place dans l'équipe.

void ResetTreeBattleState(void)
{
    memset(sTreeHeartStacks, 0, sizeof(sTreeHeartStacks));
    memset(sTreeEntryWeatherUsed, 0, sizeof(sTreeEntryWeatherUsed));
    memset(sTreeStatusSource, 0, sizeof(sTreeStatusSource));
    memset(sTreeLinger, 0, sizeof(sTreeLinger));
    memset(sTreeLingerUsed, 0, sizeof(sTreeLingerUsed));
    memset(sTreeFirstWeaknessUsed, 0, sizeof(sTreeFirstWeaknessUsed));
}

// Le Pokémon qui entre repart de zéro : cumuls du Cœur perdus, statut suivi oublié
// (la maîtrise « une fois par combat » de l'auteur reste dépensée).
void ResetTreeOnSwitchIn(enum BattlerId battler)
{
    sTreeHeartStacks[battler] = 0;
    sTreeStatusSource[battler].valid = FALSE;
    sTreeLinger[battler].state = TREE_LINGER_NONE;
}

u32 GetTreeHeartStacks(enum BattlerId battler)
{
    return sTreeHeartStacks[battler];
}

// Après une attaque réussie de l'élément de l'arbre : un cumul de plus, contre une part des PV max.
// Une activation par attaque, attaques à coups multiples comprises. Garde Magik évite le coût... et le cumul.
bool32 TryTreeHeartActivation(enum BattlerId battlerAtk, enum Move move)
{
    if (sTreeHeartStacks[battlerAtk] >= MOD_TREE_HEART_MAX_STACKS
     || GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_TYPE_DAMAGE_STACK) == 0
     || GetBattlerBoxMon(battlerAtk)->build.treeType != GetBattleMoveType(move)
     || gBattleStruct->unableToUseMove
     || !IsBattlerAlive(battlerAtk)
     || !IsAnyTargetTurnDamaged(battlerAtk, INCLUDING_SUBSTITUTES)
     || GetBattlerAbility(battlerAtk) == ABILITY_MAGIC_GUARD)
        return FALSE;

    sTreeHeartStacks[battlerAtk]++;
    SetPassiveDamageAmount(battlerAtk, max(GetNonDynamaxMaxHP(battlerAtk) * MOD_TREE_HEART_HP_COST / 100, 1));
    return TRUE;
}

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

bool32 TryTreeWeatherOnEntry(enum BattlerId battler)
{
    const struct ElementTree *tree = GetBattlerTree(battler);
    enum BattleSide side = GetBattlerSide(battler);
    u32 slotBit = 1u << gBattlerPartyIndexes[battler];
    u32 weather;

    if (tree == NULL || tree->weather == 0 || (sTreeEntryWeatherUsed[side] & slotBit)
     || GetBattlerTreeEffect(battler, TREE_EFFECT_WEATHER_ON_ENTRY) == 0 || !IsBattlerAlive(battler))
        return FALSE;
    sTreeEntryWeatherUsed[side] |= slotBit;

    weather = GetTreeBattleWeather(tree->weather);
    if (weather == BATTLE_WEATHER_COUNT || (gBattleWeather & gBattleWeatherInfo[weather].flag))
        return FALSE;
    if (TryChangeBattleWeather(battler, weather, ABILITY_NONE) != WEATHER_FAILURE_SUCCESS)
        return FALSE;

    gBattleStruct->weatherDuration = MOD_TREE_ENTRY_WEATHER_TURNS + GetTreeWeatherTurnsBonus(battler, gBattleWeather);
    gBattleScripting.battler = battler;
    gBattleScripting.animArg1 = gBattleWeatherInfo[weather].animation;
    return TRUE;
}

// Focus : coups critiques des attaques spéciales de l'élément.
u32 GetTreeCritStageBonus(enum BattlerId battlerAtk, enum Move move, enum Type moveType)
{
    const struct BoxPokemon *boxMon;

    // Sorties rapides : appelée à chaque calcul de critique, IA comprise.
    if (!IsBattleMoveSpecial(move))
        return 0;
    boxMon = GetBattlerBoxMon(battlerAtk);
    if (boxMon->build.treeType != moveType)
        return 0;
    return GetBoxMonTreeEffect(boxMon, TREE_EFFECT_TYPE_SPECIAL_CRIT);
}

static bool32 IsTreeFirstWeaknessReady(enum BattlerId battlerDef)
{
    return (sTreeFirstWeaknessUsed[GetBattlerSide(battlerDef)] & (1u << gBattlerPartyIndexes[battlerDef])) == 0
        && GetBattlerTreeEffect(battlerDef, TREE_EFFECT_FIRST_WEAKNESS_GUARD) != 0;
}

// Garde : le premier coup super efficace subi dans le combat est amorti (multiplicatif, comme Bulwark).
uq4_12_t GetTreeFirstWeaknessModifier(enum BattlerId battlerDef, uq4_12_t typeEffectiveness)
{
    u32 reduction;

    if (typeEffectiveness <= UQ_4_12(1.0) || !IsTreeFirstWeaknessReady(battlerDef))
        return UQ_4_12(1.0);
    reduction = min(GetBattlerTreeEffect(battlerDef, TREE_EFFECT_FIRST_WEAKNESS_GUARD), 100);
    return UQ_4_12(1.0) - (reduction * UQ_4_12(1.0)) / 100;
}

// Fin d'attaque : la Garde est dépensée par les cibles touchées par un coup super efficace.
// Le calcul des dégâts ne fait que lire l'état, pour que les simulations de l'IA n'y touchent pas.
void UpdateTreeFirstWeaknessGuard(enum BattlerId battlerAtk)
{
    enum BattlerId battler;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (battler == battlerAtk || !IsTreeFirstWeaknessReady(battler))
            continue;
        if (IsBattlerTurnDamaged(battler, INCLUDING_SUBSTITUTES)
         && (gBattleStruct->moveResultFlags[battler] & MOVE_RESULT_HIGH_EFFECTIVENESS))
            sTreeFirstWeaknessUsed[GetBattlerSide(battler)] |= 1u << gBattlerPartyIndexes[battler];
    }
}

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
    const struct ElementTree *tree;
    u32 status, slotBit;
    enum BattleSide side;

    if (battlerAtk == battlerDef || battlerAtk >= gBattlersCount)
        return;
    sTreeStatusSource[battlerDef].battler = battlerAtk;
    sTreeStatusSource[battlerDef].partyIndex = gBattlerPartyIndexes[battlerAtk];
    sTreeStatusSource[battlerDef].valid = TRUE;

    // Braises tenaces : seulement le premier statut de l'élément infligé dans le combat.
    tree = GetBattlerTree(battlerAtk);
    side = GetBattlerSide(battlerAtk);
    slotBit = 1u << gBattlerPartyIndexes[battlerAtk];
    if (tree == NULL || (sTreeLingerUsed[side] & slotBit)
     || GetBattlerTreeEffect(battlerAtk, TREE_EFFECT_LINGERING_STATUS) == 0)
        return;
    status = gBattleMons[battlerDef].status1 & tree->status;
    if (status == 0)
        return;
    if (status & STATUS1_SLEEP)
        status = STATUS1_SLEEP;
    sTreeLingerUsed[side] |= slotBit;
    sTreeLinger[battlerDef].status = status;
    sTreeLinger[battlerDef].state = TREE_LINGER_WATCHING;
}

static const struct BoxPokemon *GetTreeStatusSourceBoxMon(enum BattlerId battler)
{
    const struct TreeStatusSource *source = &sTreeStatusSource[battler];
    if (!source->valid)
        return NULL;
    return &GetBattlerParty(source->battler)[source->partyIndex].box;
}

// Attiser les flammes : dégâts résiduels du statut de l'élément, si c'est ce Pokémon qui l'a infligé.
u32 ApplyTreeStatusDamageBonus(enum BattlerId battler, u32 status, u32 damage)
{
    const struct BoxPokemon *source = GetTreeStatusSourceBoxMon(battler);
    const struct ElementTree *tree;
    u32 bonus;

    if (source == NULL || (tree = GetBoxMonTree(source)) == NULL || !(tree->status & status))
        return damage;
    bonus = GetBoxMonTreeEffect(source, TREE_EFFECT_STATUS_DAMAGE);
    return damage + damage * bonus / 1000;
}

static enum MoveEffect GetTreeLingerMoveEffect(u32 status)
{
    if (status & STATUS1_BURN)
        return MOVE_EFFECT_BURN;
    if (status & STATUS1_POISON)
        return MOVE_EFFECT_POISON;
    if (status & STATUS1_TOXIC_POISON)
        return MOVE_EFFECT_TOXIC;
    if (status & STATUS1_PARALYSIS)
        return MOVE_EFFECT_PARALYSIS;
    if (status & STATUS1_FREEZE)
        return MOVE_EFFECT_FREEZE;
    if (status & STATUS1_FROSTBITE)
        return MOVE_EFFECT_FROSTBITE;
    return MOVE_EFFECT_SLEEP;
}

// Braises tenaces, en fin de tour : si le statut suivi a été soigné alors que la cible est restée
// en combat, il revient pour MOD_TREE_LINGER_TURNS tours puis s'éteint. Pas contre une immunité,
// ni si un autre statut a pris sa place. Renvoie TRUE quand un message doit s'afficher
// (gBattleCommunication[MULTISTRING_CHOOSER] : 0 = retour, 1 = fin).
bool32 TryTreeLingeringStatus(enum BattlerId battler)
{
    struct TreeLinger *linger = &sTreeLinger[battler];
    u32 *status1 = &gBattleMons[battler].status1;

    if (!IsBattlerAlive(battler))
        return FALSE;

    switch (linger->state)
    {
    case TREE_LINGER_WATCHING:
        if (*status1 & linger->status)
            return FALSE;
        linger->state = TREE_LINGER_DONE;
        if ((*status1 & STATUS1_ANY)
         || !CanSetNonVolatileStatus(sTreeStatusSource[battler].battler, battler, ABILITY_NONE, GetBattlerAbility(battler),
                                     GetTreeLingerMoveEffect(linger->status), CHECK_TRIGGER))
            return FALSE;
        if (linger->status & STATUS1_SLEEP)
            *status1 |= STATUS1_SLEEP_TURN(MOD_TREE_LINGER_TURNS + 1);
        else
            *status1 |= linger->status;
        linger->state = TREE_LINGER_RETURNED;
        linger->turn = gBattleTurnCounter;
        gBattleCommunication[MULTISTRING_CHOOSER] = 0;
        break;
    case TREE_LINGER_RETURNED:
        if (!(*status1 & linger->status))
        {
            linger->state = TREE_LINGER_DONE;
            return FALSE;
        }
        if (gBattleTurnCounter - linger->turn < MOD_TREE_LINGER_TURNS)
            return FALSE;
        *status1 &= ~(linger->status | STATUS1_TOXIC_COUNTER);
        linger->state = TREE_LINGER_DONE;
        gBattleCommunication[MULTISTRING_CHOOSER] = 1;
        break;
    default:
        return FALSE;
    }

    gBattleScripting.battler = battler;
    BtlController_EmitSetMonData(battler, B_COMM_TO_CONTROLLER, REQUEST_STATUS_BATTLE, 0, sizeof(*status1), status1);
    MarkBattlerForControllerExec(battler);
    return TRUE;
}
