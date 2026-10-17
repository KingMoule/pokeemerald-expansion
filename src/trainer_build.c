#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "trainer_build.h"
#include "constants/abilities.h"
#include "constants/battle_move_effects.h"
#include "constants/opponents.h"
#include "constants/rematches.h"
#include "constants/trainers.h"

// Builds des adversaires (wiki section 13). Les équipes restent celles des fichiers .party ;
// les arbres et les variantes sont ajoutés ici selon le rang du dresseur, et les mutations
// suivent la table de src/data/trainer_mutations.h. L'introduction est graduelle :
// rien avant MOD_TRAINER_BUILD_MIN_BADGES badges, puis les points d'arbre suivent le niveau
// (les branches s'ouvrent vers le milieu de la campagne), les variantes arrivent à
// MOD_TRAINER_VARIANT_LEVEL et les mutations à la fin (Conseil 4, Maître, revanches, défis).

struct TrainerMutation
{
    u16 trainerId;
    u16 species;
    u8 replacedType;
    u8 newType;
};

#include "data/trainer_mutations.h"

#define TREE_RANK_MAX ((1 << MOD_TREE_RANK_BITS) - 1)

static u32 CountBadges(void)
{
    u32 i, count = 0;
    for (i = FLAG_BADGE01_GET; i < FLAG_BADGE01_GET + NUM_BADGES; i++)
    {
        if (FlagGet(i))
            count++;
    }
    return count;
}

static bool32 IsRematchOfSpecialTrainer(u16 trainerId)
{
    u32 i, j;
    for (i = REMATCH_ROXANNE; i < REMATCH_TABLE_ENTRIES; i++)
    {
        for (j = 1; j < REMATCHES_COUNT; j++)
        {
            if (gRematchTable[i].trainerIds[j] == trainerId)
                return TRUE;
        }
    }
    return FALSE;
}

enum TrainerBuildTier GetTrainerClassBuildTier(enum TrainerClassID trainerClass)
{
    switch (trainerClass)
    {
    case TRAINER_CLASS_ELITE_FOUR:
    case TRAINER_CLASS_CHAMPION:
        return TRAINER_BUILD_ELITE;
    case TRAINER_CLASS_LEADER:
    case TRAINER_CLASS_RIVAL:
    case TRAINER_CLASS_AQUA_LEADER:
    case TRAINER_CLASS_MAGMA_LEADER:
        return TRAINER_BUILD_STRONG;
    case TRAINER_CLASS_AQUA_ADMIN:
    case TRAINER_CLASS_MAGMA_ADMIN:
        return TRAINER_BUILD_STANDARD;
    default:
        return TRAINER_BUILD_NONE;
    }
}

enum TrainerBuildTier GetTrainerBuildTier(u16 trainerId)
{
    if (!MOD_TRAINER_BUILDS || trainerId == TRAINER_NONE || trainerId >= TRAINERS_COUNT)
        return TRAINER_BUILD_NONE;
    if (trainerId >= TRAINER_CHALLENGE_FIRST && trainerId <= TRAINER_CHALLENGE_LAST)
        return TRAINER_BUILD_ELITE;
    if (trainerId == TRAINER_STEVEN || IsRematchOfSpecialTrainer(trainerId))
        return TRAINER_BUILD_ELITE;
    return GetTrainerClassBuildTier(GetTrainerClassFromId(trainerId));
}

static bool32 IsPhysicalAttacker(struct Pokemon *mon)
{
    u32 i, physical = 0, special = 0;
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move == MOVE_NONE || IsBattleMoveStatus(move))
            continue;
        if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL)
            physical += GetMovePower(move);
        else
            special += GetMovePower(move);
    }
    if (physical != special)
        return physical > special;
    return GetSpeciesBaseAttack(species) >= GetSpeciesBaseSpAttack(species);
}

static bool32 KnowsTreeWeatherMove(struct Pokemon *mon, const struct ElementTree *tree)
{
    u32 i;

    if (tree->weather == 0)
        return FALSE;
    if (tree->weather & B_WEATHER_SUN && GetMonAbility(mon) == ABILITY_DROUGHT)
        return TRUE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move != MOVE_NONE && GetMoveEffect(move) == EFFECT_WEATHER
         && (gBattleWeatherInfo[GetMoveWeatherType(move)].flag & tree->weather))
            return TRUE;
    }
    return FALSE;
}

// Seules les attaques de statut comptent (Feu Follet) : un effet secondaire aléatoire,
// comme la brûlure de Boutefeu, ne fait pas d'un attaquant physique un spécialiste du statut.
static bool32 KnowsTreeStatusMove(struct Pokemon *mon, const struct ElementTree *tree)
{
    u32 i;
    u32 statusEffect = GetTreeStatusMoveEffect(tree->status);

    if (statusEffect == MOVE_EFFECT_NONE)
        return FALSE;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (move != MOVE_NONE && GetMoveEffect(move) == EFFECT_NON_VOLATILE_STATUS && GetMoveNonVolatileStatus(move) == statusEffect)
            return TRUE;
    }
    return FALSE;
}

// Branche suivie : météo si le Pokémon l'invoque, statut s'il l'inflige, sinon l'attaque physique
// pour un attaquant physique et le statut pour un attaquant spécial.
static u32 ChooseBranch(struct Pokemon *mon, const struct ElementTree *tree, bool32 physical)
{
    if (KnowsTreeWeatherMove(mon, tree))
        return 2;
    if (KnowsTreeStatusMove(mon, tree) || !physical)
        return 1;
    return 0;
}

static void InvestUpTo(struct Pokemon *mon, enum Type treeType, u32 node, u32 rank)
{
    while (GetBoxMonTreeRank(&mon->box, node) < rank
        && TryBoxMonInvestInNode(&mon->box, treeType, node) == TREE_INVEST_OK)
        ;
}

static enum Type ChooseTreeType(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Type type1 = GetBoxMonMutatedType(&mon->box, species, 0);
    enum Type type2 = GetBoxMonMutatedType(&mon->box, species, 1);

    // L'élément qui a des branches l'emporte ; à défaut, le premier type.
    if (gElementTrees[type1].branchNames[0] == NULL && gElementTrees[type2].branchNames[0] != NULL)
        return type2;
    return type1;
}

// Répartition automatique : l'attaque principale dans le tronc jusqu'à l'ouverture des branches,
// une branche jusqu'à sa maîtrise, puis le reste du tronc et de la branche.
static void InvestTree(struct Pokemon *mon)
{
    enum Type treeType = ChooseTreeType(mon);
    const struct ElementTree *tree = &gElementTrees[treeType];
    bool32 physical = IsPhysicalAttacker(mon);
    u32 attackNode = physical ? 0 : 1;
    u32 otherAttackNode = physical ? 1 : 0;
    bool32 fast = GetSpeciesBaseSpeed(GetMonData(mon, MON_DATA_SPECIES)) >= MOD_TRAINER_FAST_BASE_SPEED;
    u32 node, i;

    InvestUpTo(mon, treeType, attackNode, MOD_TREE_BRANCH_UNLOCK_POINTS);
    if (tree->branchNames[0] != NULL)
    {
        u32 first = TREE_TRUNK_NODES + ChooseBranch(mon, tree, physical) * TREE_BRANCH_NODES;
        for (i = 0; i < TREE_BRANCH_NODES; i++)
            InvestUpTo(mon, treeType, first + i, MOD_TREE_PARENT_RANKS);
        for (i = 0; i < TREE_BRANCH_NODES; i++)
            InvestUpTo(mon, treeType, first + i, TREE_RANK_MAX);
    }
    InvestUpTo(mon, treeType, attackNode, TREE_RANK_MAX);
    InvestUpTo(mon, treeType, fast ? 3 : 2, TREE_RANK_MAX);
    InvestUpTo(mon, treeType, fast ? 2 : 3, TREE_RANK_MAX);
    InvestUpTo(mon, treeType, otherAttackNode, TREE_RANK_MAX);
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        InvestUpTo(mon, treeType, node, TREE_RANK_MAX);
}

static u32 GetMoveScore(struct Pokemon *mon, enum Move move)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Type moveType = GetMoveType(move);
    u32 score = GetMovePower(move);

    if (moveType == GetBoxMonMutatedType(&mon->box, species, 0) || moveType == GetBoxMonMutatedType(&mon->box, species, 1))
        score = score * 3 / 2;
    return score;
}

// Variante Focused sur l'attaque la plus forte ; au rang élite, Volatile sur les autres attaques
// à effet secondaire aléatoire.
static void SetVariants(struct Pokemon *mon, enum TrainerBuildTier tier)
{
    u32 i, best = MAX_MON_MOVES, bestScore = 0;
    u32 ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        if (CanMoveHaveVariant(move, MOVE_VARIANT_FOCUSED) && GetMoveScore(mon, move) > bestScore)
        {
            best = i;
            bestScore = GetMoveScore(mon, move);
        }
    }
    if (best != MAX_MON_MOVES)
        TrySetBoxMonMoveVariant(&mon->box, best, MOVE_VARIANT_FOCUSED);

    if (tier >= TRAINER_BUILD_ELITE)
    {
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (i != best)
                TrySetBoxMonMoveVariant(&mon->box, i, MOVE_VARIANT_VOLATILE);
        }
    }

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        enum Move move = GetMonData(mon, MON_DATA_MOVE1 + i);
        u32 pp = CalculateMonMovePP(mon, move, ppBonuses, i);
        SetMonData(mon, MON_DATA_PP1 + i, &pp);
    }
}

void ApplyMonTrainerBuild(struct Pokemon *mon, enum TrainerBuildTier tier)
{
    u32 hp;

    if (tier == TRAINER_BUILD_NONE || GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
        return;

    InvestTree(mon);
    if (tier >= TRAINER_BUILD_ELITE
     || (tier >= TRAINER_BUILD_STRONG && GetMonData(mon, MON_DATA_LEVEL) >= MOD_TRAINER_VARIANT_LEVEL))
        SetVariants(mon, tier);

    // Les arbres changent les PV max et la Vitesse : le Pokémon entre en combat soigné.
    CalculateMonStats(mon);
    hp = GetMonData(mon, MON_DATA_MAX_HP);
    SetMonData(mon, MON_DATA_HP, &hp);
}

// La mutation va au dernier Pokémon de l'espèce indiquée (l'as de l'équipe en général).
bool32 ApplyTrainerMutation(struct Pokemon *party, u32 partySize, u16 trainerId)
{
    u32 i;
    s32 slot;

    for (i = 0; i < ARRAY_COUNT(sTrainerMutations); i++)
    {
        if (sTrainerMutations[i].trainerId != trainerId)
            continue;
        for (slot = partySize - 1; slot >= 0; slot--)
        {
            if (GetMonData(&party[slot], MON_DATA_SPECIES) == sTrainerMutations[i].species)
                return SetBoxMonMutation(&party[slot].box, sTrainerMutations[i].replacedType, sTrainerMutations[i].newType);
        }
    }
    return FALSE;
}

u32 GetTrainerMutationCount(void)
{
    return ARRAY_COUNT(sTrainerMutations);
}

u16 GetTrainerMutationTrainerId(u32 index)
{
    return index < ARRAY_COUNT(sTrainerMutations) ? sTrainerMutations[index].trainerId : TRAINER_NONE;
}

void ApplyTrainerPartyBuilds(struct Pokemon *party, u16 trainerId)
{
    enum TrainerBuildTier tier = GetTrainerBuildTier(trainerId);
    u32 i;

    if (tier == TRAINER_BUILD_NONE || CountBadges() < MOD_TRAINER_BUILD_MIN_BADGES)
        return;

    // La mutation d'abord : l'arbre suit alors les types après mutation.
    if (tier >= TRAINER_BUILD_ELITE && MOD_TRAINER_MUTATIONS)
        ApplyTrainerMutation(party, PARTY_SIZE, trainerId);
    for (i = 0; i < PARTY_SIZE; i++)
        ApplyMonTrainerBuild(&party[i], tier);
}
