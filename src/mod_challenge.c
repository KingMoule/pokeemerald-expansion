#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "mod_challenge.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "script.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/opponents.h"

// Défis à règles annoncées (wiki sections 13 et 14). Le maître des défis, au Centre Pokémon
// d'Ever Grande, annonce les règles avant chaque défi pour que le joueur prépare son équipe.
// Les adversaires ont des builds complets (rang élite de src/trainer_build.c).
// Une défaite ne coûte ni argent ni retour au Centre ; l'équipe est soignée à la fin du défi.
// La récompense n'est donnée qu'à la première réussite. Valeurs provisoires.

#define MAX_CHALLENGE_STAGES 2

struct ModChallenge
{
    const u8 *name;
    const u8 *rules;
    u16 trainers[MAX_CHALLENGE_STAGES]; // TRAINER_NONE après le dernier combat.
    enum Item reward;
    bool8 noBag;
};

EWRAM_DATA bool8 gModChallengeActive = FALSE;
static EWRAM_DATA u8 sCurrentChallenge = 0;
static EWRAM_DATA u8 sCurrentStage = 0;

static const u8 sText_TrainerDefeat[] = _("A fine showing.\nYou've earned this win.");
static const u8 sText_TrainerVictory[] = _("Rules are rules.\nBetter luck next time!");

static const struct ModChallenge sChallenges[CHALLENGE_COUNT] =
{
    [CHALLENGE_ELEMENT] =
    {
        .name = COMPOUND_STRING("ELEMENT TRIAL"),
        .rules = COMPOUND_STRING("Every POKéMON you bring must share\none type.\pYour opponent is a FIRE expert\nwho fights in the sun."),
        .trainers = { TRAINER_CHALLENGE_ELEMENT, TRAINER_NONE },
        .reward = ITEM_TECHNIQUE_CODEX,
    },
    [CHALLENGE_DOUBLE] =
    {
        .name = COMPOUND_STRING("DOUBLE TRIAL"),
        .rules = COMPOUND_STRING("A DOUBLE BATTLE. You need at least\ntwo POKéMON able to battle.\pYour opponents fight as a pair,\nunder the rain."),
        .trainers = { TRAINER_CHALLENGE_DOUBLE, TRAINER_NONE },
        .reward = ITEM_TECHNIQUE_CODEX,
    },
    [CHALLENGE_VARIETY] =
    {
        .name = COMPOUND_STRING("VARIETY TRIAL"),
        .rules = COMPOUND_STRING("Bring six POKéMON. No two of them\nmay share a type.\pYour opponent's team covers many\ntypes, too."),
        .trainers = { TRAINER_CHALLENGE_VARIETY, TRAINER_NONE },
        .reward = ITEM_LINEAGE_CHARM,
    },
    [CHALLENGE_RISING] =
    {
        .name = COMPOUND_STRING("RISING TRIAL"),
        .rules = COMPOUND_STRING("Every POKéMON you bring must be\nlevel {STR_VAR_3} or lower.\pYour opponent's FIGHTING team\nis no higher, either."),
        .trainers = { TRAINER_CHALLENGE_RISING, TRAINER_NONE },
        .reward = ITEM_TECHNIQUE_CODEX,
    },
    [CHALLENGE_ENDURANCE] =
    {
        .name = COMPOUND_STRING("ENDURANCE TRIAL"),
        .rules = COMPOUND_STRING("Two battles in a row. No healing\nin between, and no items from\lyour BAG during battle.\pPlan for the long haul!"),
        .trainers = { TRAINER_CHALLENGE_ENDURANCE_1, TRAINER_CHALLENGE_ENDURANCE_2 },
        .reward = ITEM_LINEAGE_CHARM,
        .noBag = TRUE,
    },
};

static const u8 sText_Cleared[] = _(" (WON)");

static bool32 IsBattler(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG);
}

static bool32 MonHasType(struct Pokemon *mon, enum Type type)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    return GetBoxMonMutatedType(&mon->box, species, 0) == type || GetBoxMonMutatedType(&mon->box, species, 1) == type;
}

static bool32 ShareOneType(struct Pokemon *party)
{
    u32 i, j, slot;
    struct Pokemon *first = NULL;

    for (i = 0; i < PARTY_SIZE && first == NULL; i++)
    {
        if (IsBattler(&party[i]))
            first = &party[i];
    }
    if (first == NULL)
        return FALSE;

    for (slot = 0; slot < 2; slot++)
    {
        enum Type type = GetBoxMonMutatedType(&first->box, GetMonData(first, MON_DATA_SPECIES), slot);
        bool32 shared = TRUE;
        for (j = 0; j < PARTY_SIZE; j++)
        {
            if (IsBattler(&party[j]) && !MonHasType(&party[j], type))
                shared = FALSE;
        }
        if (shared)
            return TRUE;
    }
    return FALSE;
}

static bool32 NoSharedTypes(struct Pokemon *party)
{
    u32 i, j, slot;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (!IsBattler(&party[i]))
            continue;
        for (slot = 0; slot < 2; slot++)
        {
            enum Type type = GetBoxMonMutatedType(&party[i].box, GetMonData(&party[i], MON_DATA_SPECIES), slot);
            for (j = i + 1; j < PARTY_SIZE; j++)
            {
                if (IsBattler(&party[j]) && MonHasType(&party[j], type))
                    return FALSE;
            }
        }
    }
    return TRUE;
}

static u32 CountBattlers(struct Pokemon *party, bool32 healthyOnly)
{
    u32 i, count = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (IsBattler(&party[i]) && (!healthyOnly || GetMonData(&party[i], MON_DATA_HP) != 0))
            count++;
    }
    return count;
}

static bool32 AllAtMostLevel(struct Pokemon *party, u32 level)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (IsBattler(&party[i]) && GetMonData(&party[i], MON_DATA_LEVEL) > level)
            return FALSE;
    }
    return TRUE;
}

bool32 ModChallenge_IsPartyAllowed(u32 challenge, struct Pokemon *party)
{
    if (CountBattlers(party, FALSE) == 0)
        return FALSE;
    switch (challenge)
    {
    case CHALLENGE_ELEMENT:
        return ShareOneType(party);
    case CHALLENGE_DOUBLE:
        return CountBattlers(party, TRUE) >= 2;
    case CHALLENGE_VARIETY:
        return CountBattlers(party, FALSE) >= MOD_CHALLENGE_VARIETY_SIZE && NoSharedTypes(party);
    case CHALLENGE_RISING:
        return AllAtMostLevel(party, MOD_CHALLENGE_RISING_MAX_LEVEL);
    case CHALLENGE_ENDURANCE:
        return TRUE;
    default:
        return FALSE;
    }
}

u16 ModChallenge_GetTrainer(u32 challenge, u32 stage)
{
    if (challenge >= CHALLENGE_COUNT || stage >= MAX_CHALLENGE_STAGES)
        return TRAINER_NONE;
    return sChallenges[challenge].trainers[stage];
}

enum Item ModChallenge_GetReward(u32 challenge)
{
    return challenge < CHALLENGE_COUNT ? sChallenges[challenge].reward : ITEM_NONE;
}

bool32 ModChallenge_IsCleared(u32 challenge)
{
    return (VarGet(VAR_MOD_CHALLENGES_CLEARED) >> challenge) & 1;
}

void ModChallenge_SetupBattle(void)
{
    u16 trainerId = ModChallenge_GetTrainer(sCurrentChallenge, sCurrentStage);

    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    if (GetTrainerBattleType(trainerId) == TRAINER_BATTLE_TYPE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    InitTrainerBattleParameter();
    TRAINER_BATTLE_PARAM.opponentA = trainerId;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_TrainerDefeat;
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)sText_TrainerVictory;
    CreateTrainerPartyWithBuilds(gParties[B_TRAINER_OPPONENT_A], trainerId);
}

static void PushItem(const u8 *text, bool32 cleared, u32 id)
{
    struct ListMenuItem item;
    u8 *name = Alloc(32);

    StringCopy(name, text);
    if (cleared)
        StringAppend(name, sText_Cleared);
    item.name = name;
    item.id = id;
    MultichoiceDynamic_PushElement(item);
}

// Liste des défis ; « (WON) » marque ceux déjà réussis.
void ModChallenge_PushList(void)
{
    u32 i;
    for (i = 0; i < CHALLENGE_COUNT; i++)
        PushItem(sChallenges[i].name, ModChallenge_IsCleared(i), i);
}

// VAR_0x8005 : défi choisi. STR_VAR_1 : nom, STR_VAR_2 : récompense, gStringVar4 : règles.
void ModChallenge_BufferRules(void)
{
    u32 challenge = gSpecialVar_0x8005;
    if (challenge >= CHALLENGE_COUNT)
        challenge = 0;
    StringCopy(gStringVar1, sChallenges[challenge].name);
    CopyItemName(sChallenges[challenge].reward, gStringVar2);
    ConvertIntToDecimalStringN(gStringVar3, MOD_CHALLENGE_RISING_MAX_LEVEL, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, sChallenges[challenge].rules);
    gSpecialVar_Result = ModChallenge_IsCleared(challenge);
}

void ModChallenge_CheckParty(void)
{
    gSpecialVar_Result = ModChallenge_IsPartyAllowed(gSpecialVar_0x8005, gParties[B_TRAINER_PLAYER]);
}

void ModChallenge_Start(void)
{
    sCurrentChallenge = gSpecialVar_0x8005 < CHALLENGE_COUNT ? gSpecialVar_0x8005 : 0;
    sCurrentStage = 0;
    gModChallengeActive = TRUE;
    HealPlayerParty();
    VarSet(VAR_MOD_NO_BAG, sChallenges[sCurrentChallenge].noBag ? NO_BAG_IN_BATTLE : NO_BAG_RESTRICTION);
}

// VAR_RESULT en entrée : issue du combat (B_OUTCOME_*). En sortie : CHALLENGE_NEXT_BATTLE,
// CHALLENGE_WON ou CHALLENGE_LOST.
void ModChallenge_AfterBattle(void)
{
    if (gSpecialVar_Result != B_OUTCOME_WON)
    {
        gSpecialVar_Result = CHALLENGE_LOST;
        return;
    }
    sCurrentStage++;
    if (ModChallenge_GetTrainer(sCurrentChallenge, sCurrentStage) != TRAINER_NONE)
        gSpecialVar_Result = CHALLENGE_NEXT_BATTLE;
    else
        gSpecialVar_Result = CHALLENGE_WON;
}

// VAR_0x8004 : TRUE si le défi est réussi. En sortie, VAR_RESULT : objet de récompense
// (ITEM_NONE si le défi était déjà réussi ou s'il est perdu).
void ModChallenge_Finish(void)
{
    bool32 won = gSpecialVar_0x8004;

    gModChallengeActive = FALSE;
    VarSet(VAR_MOD_NO_BAG, NO_BAG_RESTRICTION);
    HealPlayerParty();
    // Le combat a pu réécrire gStringVar1 : le message de victoire nomme le défi.
    StringCopy(gStringVar1, sChallenges[sCurrentChallenge].name);
    gSpecialVar_Result = ITEM_NONE;
    if (won && !ModChallenge_IsCleared(sCurrentChallenge))
    {
        VarSet(VAR_MOD_CHALLENGES_CLEARED, VarGet(VAR_MOD_CHALLENGES_CLEARED) | (1 << sCurrentChallenge));
        gSpecialVar_Result = sChallenges[sCurrentChallenge].reward;
    }
}
