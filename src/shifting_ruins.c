#include "global.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "random.h"
#include "script_pokemon_util.h"
#include "shifting_ruins.h"
#include "string_util.h"
#include "text.h"
#include "wild_encounter.h"
#include "constants/battle.h"
#include "constants/battle_frontier.h"
#include "constants/battle_pyramid.h"
#include "constants/items.h"
#include "constants/opponents.h"

// Shifting Ruins, donjon rogue-lite à la Lufia 2 (wiki section 11).
// Une tentative réutilise les étages générés de la Pyramide de Combat, dans un mode à part :
// on part avec un ou plusieurs Pokémon aléatoires de niveau 5, on combat, capture et monte
// en niveau sur place, et les objets rares trouvés restent en attente jusqu'à une sortie
// réussie (palier, Escape Orb ou fond). L'équipe de la campagne attend à l'entrée.
// Toute la tentative vit en EWRAM et la sauvegarde est interdite pendant la tentative :
// un redémarrage ramène à la sauvegarde faite à l'entrée, sans rien perdre de la campagne.
// Les données de la Zone de Combat touchées par les étages sont restaurées à la sortie.
// Valeurs : include/config/mod.h (MOD_RUINS_*). Script : data/scripts/mod_shifting_ruins.inc.

STATIC_ASSERT(MOD_RUINS_LANDING_EVERY > 0, RuinsLandingEveryNotZero);
STATIC_ASSERT(MOD_RUINS_MAX_FLOOR <= 255, RuinsMaxFloorFitsInU8);
STATIC_ASSERT(MOD_RUINS_MAX_START_MONS >= 1 && MOD_RUINS_MAX_START_MONS <= PARTY_SIZE, RuinsStartMons);
STATIC_ASSERT(MOD_RUINS_TRAINER_MONS_MAX >= 1 && MOD_RUINS_TRAINER_MONS_MAX <= PARTY_SIZE, RuinsTrainerMons);

struct ShiftingRuinsRun
{
    struct Pokemon campaignParty[PARTY_SIZE];
    struct BattleFrontier frontierBackup;
    u16 relics[MOD_RUINS_MAX_RELICS];
    u8 campaignPartyCount;
    u8 relicCount;
    u8 floor;
    u8 end;
    u8 keptSlot;
};

static EWRAM_DATA struct ShiftingRuinsRun sRuins = {0};
// Lu par le script de combat (pas de perte d'argent ni de « hors combat » dans les ruines).
EWRAM_DATA bool8 gShiftingRuinsActive = FALSE;

struct RuinsMon
{
    u16 species;
    u8 minLevel;
    u8 maxLevel;
};

// Rencontres sauvages et équipes des dresseurs : chaque forme apparaît dans sa tranche de niveaux.
static const struct RuinsMon sRuinsMons[] =
{
    { SPECIES_ZIGZAGOON,   1, 19 }, { SPECIES_LINOONE,    20, 100 },
    { SPECIES_POOCHYENA,   1, 17 }, { SPECIES_MIGHTYENA,  18, 100 },
    { SPECIES_WURMPLE,     1,  9 }, { SPECIES_BEAUTIFLY,  10, 100 }, { SPECIES_DUSTOX,    10, 100 },
    { SPECIES_LOTAD,       1, 13 }, { SPECIES_LOMBRE,     14, 35 }, { SPECIES_LUDICOLO,  36, 100 },
    { SPECIES_SEEDOT,      1, 13 }, { SPECIES_NUZLEAF,    14, 35 }, { SPECIES_SHIFTRY,   36, 100 },
    { SPECIES_TAILLOW,     1, 21 }, { SPECIES_SWELLOW,    22, 100 },
    { SPECIES_WINGULL,     1, 24 }, { SPECIES_PELIPPER,   25, 100 },
    { SPECIES_RALTS,       1, 19 }, { SPECIES_KIRLIA,     20, 29 }, { SPECIES_GARDEVOIR, 30, 100 },
    { SPECIES_SURSKIT,     1, 21 }, { SPECIES_MASQUERAIN, 22, 100 },
    { SPECIES_SHROOMISH,   1, 22 }, { SPECIES_BRELOOM,    23, 100 },
    { SPECIES_SLAKOTH,     1, 17 }, { SPECIES_VIGOROTH,   18, 35 }, { SPECIES_SLAKING,   36, 100 },
    { SPECIES_WHISMUR,     1, 19 }, { SPECIES_LOUDRED,    20, 39 }, { SPECIES_EXPLOUD,   40, 100 },
    { SPECIES_MAKUHITA,    1, 23 }, { SPECIES_HARIYAMA,   24, 100 },
    { SPECIES_ARON,        1, 31 }, { SPECIES_LAIRON,     32, 41 }, { SPECIES_AGGRON,    42, 100 },
    { SPECIES_MEDITITE,    1, 36 }, { SPECIES_MEDICHAM,   37, 100 },
    { SPECIES_ELECTRIKE,   1, 25 }, { SPECIES_MANECTRIC,  26, 100 },
    { SPECIES_GULPIN,      1, 25 }, { SPECIES_SWALOT,     26, 100 },
    { SPECIES_CARVANHA,    1, 29 }, { SPECIES_SHARPEDO,   30, 100 },
    { SPECIES_NUMEL,       1, 32 }, { SPECIES_CAMERUPT,   33, 100 },
    { SPECIES_SPOINK,      1, 31 }, { SPECIES_GRUMPIG,    32, 100 },
    { SPECIES_TRAPINCH,    1, 34 }, { SPECIES_VIBRAVA,    35, 44 }, { SPECIES_FLYGON,    45, 100 },
    { SPECIES_CACNEA,      1, 31 }, { SPECIES_CACTURNE,   32, 100 },
    { SPECIES_SWABLU,      1, 34 }, { SPECIES_ALTARIA,    35, 100 },
    { SPECIES_BARBOACH,    1, 29 }, { SPECIES_WHISCASH,   30, 100 },
    { SPECIES_CORPHISH,    1, 29 }, { SPECIES_CRAWDAUNT,  30, 100 },
    { SPECIES_BALTOY,      1, 35 }, { SPECIES_CLAYDOL,    36, 100 },
    { SPECIES_SHUPPET,     1, 36 }, { SPECIES_BANETTE,    37, 100 },
    { SPECIES_DUSKULL,     1, 36 }, { SPECIES_DUSCLOPS,   37, 100 },
    { SPECIES_SNORUNT,     1, 41 }, { SPECIES_GLALIE,     42, 100 },
    { SPECIES_SPHEAL,      1, 31 }, { SPECIES_SEALEO,     32, 43 }, { SPECIES_WALREIN,   44, 100 },
    { SPECIES_GEODUDE,     1, 24 }, { SPECIES_GRAVELER,   25, 100 },
    { SPECIES_ZUBAT,       1, 21 }, { SPECIES_GOLBAT,     22, 100 },
    { SPECIES_MACHOP,      1, 27 }, { SPECIES_MACHOKE,    28, 100 },
    { SPECIES_SABLEYE,    10, 100 }, { SPECIES_MAWILE,    10, 100 },
    { SPECIES_ZANGOOSE,   15, 100 }, { SPECIES_SEVIPER,   15, 100 },
    { SPECIES_TORKOAL,    15, 100 }, { SPECIES_KECLEON,   15, 100 },
    { SPECIES_ABSOL,      20, 100 }, { SPECIES_TROPIUS,   25, 100 },
    { SPECIES_BAGON,      20, 29 }, { SPECIES_SHELGON,    30, 49 }, { SPECIES_SALAMENCE, 50, 100 },
    { SPECIES_BELDUM,     15, 19 }, { SPECIES_METANG,     20, 44 }, { SPECIES_METAGROSS, 45, 100 },
};

// Viviers de départ : le premier est toujours ouvert, chaque record en ouvre un plus fort.
static const u16 sStartTier0[] =
{
    SPECIES_ZIGZAGOON, SPECIES_POOCHYENA, SPECIES_WURMPLE, SPECIES_LOTAD, SPECIES_SEEDOT,
    SPECIES_TAILLOW, SPECIES_WINGULL, SPECIES_RALTS, SPECIES_SURSKIT, SPECIES_SHROOMISH,
    SPECIES_SLAKOTH, SPECIES_WHISMUR, SPECIES_MAKUHITA, SPECIES_SKITTY, SPECIES_ARON,
    SPECIES_MEDITITE, SPECIES_ELECTRIKE, SPECIES_GULPIN, SPECIES_NUMEL, SPECIES_SPOINK,
    SPECIES_TRAPINCH, SPECIES_CACNEA, SPECIES_SWABLU, SPECIES_CORPHISH, SPECIES_BALTOY,
};

static const u16 sStartTier1[] =
{
    SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP, SPECIES_ABSOL, SPECIES_ZANGOOSE,
    SPECIES_SEVIPER, SPECIES_SABLEYE, SPECIES_MAWILE, SPECIES_SHUPPET, SPECIES_DUSKULL,
    SPECIES_SNORUNT, SPECIES_SPHEAL, SPECIES_CARVANHA, SPECIES_LILEEP, SPECIES_ANORITH,
};

static const u16 sStartTier2[] =
{
    SPECIES_BAGON, SPECIES_BELDUM, SPECIES_LARVITAR, SPECIES_DRATINI, SPECIES_GIBLE,
};

static const struct
{
    const u16 *species;
    u8 count;
} sStartTiers[] =
{
    { sStartTier0, ARRAY_COUNT(sStartTier0) },
    { sStartTier1, ARRAY_COUNT(sStartTier1) },
    { sStartTier2, ARRAY_COUNT(sStartTier2) },
};

// Objets ordinaires au sol, selon le nombre de paliers franchis. Ils vont dans le sac
// des ruines et ne sortent pas : ils servent pendant la tentative (soins, Poké Balls, PP).
#define RUINS_LOOT_TIER_SIZE 10
static const u16 sCommonLoot[][RUINS_LOOT_TIER_SIZE] =
{
    { ITEM_POTION, ITEM_POTION, ITEM_POTION, ITEM_POKE_BALL, ITEM_POKE_BALL, ITEM_POKE_BALL,
      ITEM_ANTIDOTE, ITEM_PARALYZE_HEAL, ITEM_ETHER, ITEM_REVIVE },
    { ITEM_SUPER_POTION, ITEM_SUPER_POTION, ITEM_POTION, ITEM_GREAT_BALL, ITEM_GREAT_BALL, ITEM_POKE_BALL,
      ITEM_FULL_HEAL, ITEM_ETHER, ITEM_REVIVE, ITEM_RARE_CANDY },
    { ITEM_HYPER_POTION, ITEM_SUPER_POTION, ITEM_SUPER_POTION, ITEM_ULTRA_BALL, ITEM_GREAT_BALL, ITEM_GREAT_BALL,
      ITEM_FULL_HEAL, ITEM_ELIXIR, ITEM_REVIVE, ITEM_RARE_CANDY },
    { ITEM_FULL_RESTORE, ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_ULTRA_BALL, ITEM_ULTRA_BALL, ITEM_GREAT_BALL,
      ITEM_FULL_HEAL, ITEM_ELIXIR, ITEM_MAX_REVIVE, ITEM_RARE_CANDY },
};

static const u8 sText_TrainerDefeat[] = _("The ruins chose you today.");
static const u8 sText_TrainerVictory[] = _("The ruins keep what they take!");
static const u8 sText_FloorWindow[] = _("Shifting Ruins\nFloor ");
static const u8 sText_MapName[] = _("RUINS FLOOR ");
static const u8 sText_YourTeam[] = _("Your team for this run:\n");
static const u8 sText_Comma[] = _(", ");
static const u8 sText_Dot[] = _(".");
static const u8 sText_BroughtBack[] = _("You brought back:\n");
static const u8 sText_Times[] = _(" ×");

// --- Calculs ---------------------------------------------------------------------------

static u32 GetLandingsPassed(u32 floor)
{
    return floor > 0 ? (floor - 1) / MOD_RUINS_LANDING_EVERY : 0;
}

u32 ShiftingRuins_GetWildLevel(u32 floor)
{
    u32 level = MOD_RUINS_LEVEL_BASE + (floor > 0 ? floor - 1 : 0) * MOD_RUINS_LEVELS_PER_FLOOR;
    return min(level, MAX_LEVEL);
}

u32 ShiftingRuins_GetTrainerLevel(u32 floor)
{
    return min(ShiftingRuins_GetWildLevel(floor) + MOD_RUINS_TRAINER_LEVEL_BONUS, MAX_LEVEL);
}

u32 ShiftingRuins_GetTrainerPartySize(u32 floor)
{
    return min(MOD_RUINS_TRAINER_MONS_BASE + GetLandingsPassed(floor), MOD_RUINS_TRAINER_MONS_MAX);
}

bool32 ShiftingRuins_IsLandingFloor(u32 floor)
{
    return floor != 0 && floor % MOD_RUINS_LANDING_EVERY == 0;
}

u32 ShiftingRuins_GetStartMonCount(u32 record)
{
    return min(1 + record / MOD_RUINS_EXTRA_MON_EVERY, MOD_RUINS_MAX_START_MONS);
}

u32 ShiftingRuins_GetStartTier(u32 record)
{
    return min(record / MOD_RUINS_TIER_EVERY, ARRAY_COUNT(sStartTiers) - 1);
}

enum Species ShiftingRuins_PickStartSpecies(u32 tier, u32 rand)
{
    if (tier >= ARRAY_COUNT(sStartTiers))
        tier = ARRAY_COUNT(sStartTiers) - 1;
    return sStartTiers[tier].species[rand % sStartTiers[tier].count];
}

enum Species ShiftingRuins_PickSpecies(u32 level, u32 rand)
{
    u32 i, count = 0;

    for (i = 0; i < ARRAY_COUNT(sRuinsMons); i++)
    {
        if (level >= sRuinsMons[i].minLevel && level <= sRuinsMons[i].maxLevel)
            count++;
    }
    if (count == 0)
        return SPECIES_ZIGZAGOON;
    rand %= count;
    for (i = 0; i < ARRAY_COUNT(sRuinsMons); i++)
    {
        if (level >= sRuinsMons[i].minLevel && level <= sRuinsMons[i].maxLevel && rand-- == 0)
            return sRuinsMons[i].species;
    }
    return SPECIES_ZIGZAGOON;
}

static enum Item RollCommonItem(u32 floor, u32 rand)
{
    u32 tier = min(GetLandingsPassed(floor), ARRAY_COUNT(sCommonLoot) - 1);
    return sCommonLoot[tier][rand % RUINS_LOOT_TIER_SIZE];
}

enum Item ShiftingRuins_RollFloorItem(u32 floor, u32 rand)
{
    u32 roll = rand % 100;
    u32 threshold = MOD_RUINS_RELIC_CHANCE;

    if (roll < threshold)
        return ITEM_TECHNIQUE_CODEX;
    if (GetLandingsPassed(floor) >= 1)
    {
        threshold += MOD_RUINS_RARE_RELIC_CHANCE;
        if (roll < threshold)
            return ITEM_LINEAGE_CHARM;
    }
#if MOD_LEVELS_BEYOND_100
    if (GetLandingsPassed(floor) >= 2)
    {
        threshold += MOD_RUINS_RARE_RELIC_CHANCE;
        if (roll < threshold)
            return ITEM_ASCENSION_SEAL;
    }
#endif
    threshold += MOD_RUINS_ESCAPE_ORB_CHANCE;
    if (roll < threshold)
        return ITEM_ESCAPE_ORB;
    return RollCommonItem(floor, rand / 100);
}

// Objets qu'on ne peut garder qu'en sortant vivant : ils ne vont pas dans le sac des ruines.
bool32 ShiftingRuins_IsRelic(enum Item item)
{
    return item == ITEM_TECHNIQUE_CODEX
        || item == ITEM_LINEAGE_CHARM
#if MOD_LEVELS_BEYOND_100
        || item == ITEM_ASCENSION_SEAL
#endif
        ;
}

// Le Pokémon ramené revient au niveau 5, sans l'expérience, les attaques, les EV, l'arbre
// ni les variantes acquis dans les ruines. Il garde son espèce, ses IV, sa nature, son talent
// et une éventuelle mutation (les mutations sont permanentes).
void ShiftingRuins_ResetKeptMon(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 level = MOD_RUINS_START_LEVEL;
    u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][level];
    u32 i, zero = 0;
    enum Item noItem = ITEM_NONE;

    SetMonData(mon, MON_DATA_EXP, &exp);
    SetMonData(mon, MON_DATA_LEVEL, &level);
    SetMonData(mon, MON_DATA_HELD_ITEM, &noItem);
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_EV + i, &zero);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &zero);
        SetMonData(mon, MON_DATA_PP1 + i, &zero);
    }
    SetMonData(mon, MON_DATA_PP_BONUSES, &zero);
    memset(mon->box.build.treeRanks, 0, sizeof(mon->box.build.treeRanks));
    mon->box.build.treeType = TYPE_NONE;
    mon->box.build.treePoints = 0;
    mon->box.build.moveVariants = 0;
    GiveMonInitialMoveset(mon);
    CalculateMonStats(mon);
    HealPokemon(mon);
}

void ShiftingRuins_CreateTrainerParty(struct Pokemon *party, u32 floor)
{
    u32 i;
    u32 size = ShiftingRuins_GetTrainerPartySize(floor);
    u32 level = ShiftingRuins_GetTrainerLevel(floor);
    u32 iv = min(MAX_PER_STAT_IVS, 5 + floor / 2);

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < size; i++)
    {
        CreateMonWithIVs(&party[i], ShiftingRuins_PickSpecies(level, Random32()), level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY, iv);
        GiveMonInitialMoveset(&party[i]);
    }
}

// --- État de la tentative --------------------------------------------------------------

bool32 InShiftingRuins(void)
{
    return gShiftingRuinsActive && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_FLOOR;
}

u32 ShiftingRuins_GetFloor(void)
{
    return sRuins.floor;
}

static void SetupFloor(void)
{
    u32 i;

    // Les gabarits d'étage, les couleurs et les rencontres de la Pyramide tournent sur ses 7 étages.
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = (sRuins.floor - 1) % FRONTIER_STAGES_PER_CHALLENGE;
    for (i = 0; i < ARRAY_COUNT(gSaveBlock2Ptr->frontier.pyramidRandoms); i++)
        gSaveBlock2Ptr->frontier.pyramidRandoms[i] = Random();
    gSaveBlock2Ptr->frontier.pyramidTrainerFlags = 0;
    gBattleOutcome = 0;
}

void ShiftingRuins_GenerateWildMon(void)
{
    u32 level = ShiftingRuins_GetWildLevel(sRuins.floor);
    CreateWildMon(ShiftingRuins_PickSpecies(level, Random32()), level);
}

// Combat de dresseur dans les ruines : un combat ordinaire (expérience, évolutions) contre
// une équipe générée ; l'apparence et les répliques du dresseur restent celles de la Pyramide.
void ShiftingRuins_SetupTrainerBattle(void)
{
    VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
    MarkApproachingPyramidTrainersAsBattled();
    ShiftingRuins_CreateTrainerParty(gParties[B_TRAINER_OPPONENT_A], sRuins.floor);
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_RUINS;
    TRAINER_BATTLE_PARAM.opponentB = 0;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_TrainerDefeat;
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)sText_TrainerVictory;
}

void ShiftingRuins_SetupWildBattle(void)
{
    VarSet(VAR_TEMP_E, 0);
}

void ShiftingRuins_SetFloorItem(void)
{
    enum Item item = ShiftingRuins_RollFloorItem(sRuins.floor, Random32());

    gSpecialVar_0x8000 = item;
    gSpecialVar_0x8001 = 1;
    gSpecialVar_Result = ShiftingRuins_IsRelic(item);
}

u16 ShiftingRuins_GetPickupItem(void)
{
    return RollCommonItem(sRuins.floor, Random32());
}

void ShiftingRuins_BufferFloorWindowText(u8 *dest)
{
    dest = StringCopy(dest, sText_FloorWindow);
    ConvertIntToDecimalStringN(dest, sRuins.floor, STR_CONV_MODE_LEFT_ALIGN, 3);
}

void ShiftingRuins_GetMapName(u8 *dest)
{
    dest = StringCopy(dest, sText_MapName);
    ConvertIntToDecimalStringN(dest, sRuins.floor, STR_CONV_MODE_LEFT_ALIGN, 3);
}

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

// --- Specials --------------------------------------------------------------------------

// VAR_RESULT : TRUE si le joueur a assez de badges. STR_VAR_1 : nombre requis. STR_VAR_2 : écart des paliers.
void ShiftingRuins_CheckAccess(void)
{
    ConvertIntToDecimalStringN(gStringVar1, MOD_RUINS_REQUIRED_BADGES, STR_CONV_MODE_LEFT_ALIGN, 1);
    ConvertIntToDecimalStringN(gStringVar2, MOD_RUINS_LANDING_EVERY, STR_CONV_MODE_LEFT_ALIGN, 2);
    gSpecialVar_Result = (CountBadges() >= MOD_RUINS_REQUIRED_BADGES);
}

// STR_VAR_1 : record. STR_VAR_2 : Pokémon au départ. VAR_RESULT : record (0 = jamais sorti).
void ShiftingRuins_BufferRecord(void)
{
    u32 record = VarGet(VAR_MOD_RUINS_RECORD);

    ConvertIntToDecimalStringN(gStringVar1, record, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, ShiftingRuins_GetStartMonCount(record), STR_CONV_MODE_LEFT_ALIGN, 1);
    gSpecialVar_Result = record;
}

static bool32 PartyHasSpecies(u32 count, enum Species species)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == species)
            return TRUE;
    }
    return FALSE;
}

// Met l'équipe de la campagne de côté, tire l'équipe de départ, prépare le sac des ruines
// et le premier étage. gStringVar4 : composition de l'équipe.
void ShiftingRuins_Start(void)
{
    u32 i, record = VarGet(VAR_MOD_RUINS_RECORD);
    u32 count = ShiftingRuins_GetStartMonCount(record);
    u32 maxTier = ShiftingRuins_GetStartTier(record);
    u8 *str = gStringVar4;

    memcpy(sRuins.campaignParty, gParties[B_TRAINER_PLAYER], sizeof(sRuins.campaignParty));
    sRuins.campaignPartyCount = gPartiesCount[B_TRAINER_PLAYER];
    memcpy(&sRuins.frontierBackup, &gSaveBlock2Ptr->frontier, sizeof(sRuins.frontierBackup));
    sRuins.relicCount = 0;
    sRuins.floor = 1;
    sRuins.end = RUINS_END_NONE;
    sRuins.keptSlot = PARTY_SIZE;
    gShiftingRuinsActive = TRUE;

    str = StringCopy(str, sText_YourTeam);
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    for (i = 0; i < count; i++)
    {
        enum Species species;
        // Un tirage par Pokémon dans un des viviers ouverts, sans doublon.
        do
        {
            species = ShiftingRuins_PickStartSpecies(Random() % (maxTier + 1), Random());
        } while (PartyHasSpecies(i, species));
        CreateMonWithIVs(&gParties[B_TRAINER_PLAYER][i], species, MOD_RUINS_START_LEVEL, Random32(), OTID_STRUCT_PLAYER_ID, USE_RANDOM_IVS);
        GiveMonInitialMoveset(&gParties[B_TRAINER_PLAYER][i]);
        if (i != 0)
            str = StringAppend(str, sText_Comma);
        str = StringAppend(str, GetSpeciesName(species));
    }
    StringAppend(str, sText_Dot);
    gPartiesCount[B_TRAINER_PLAYER] = count;

    // Le sac des ruines est celui de la Pyramide (niveau 50), restauré à la sortie.
    gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_50;
    gSaveBlock2Ptr->frontier.challengeStatus = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    for (i = 0; i < PYRAMID_BAG_ITEMS_COUNT; i++)
    {
        gSaveBlock2Ptr->frontier.pyramidBag.itemId[FRONTIER_LVL_50][i] = ITEM_NONE;
        gSaveBlock2Ptr->frontier.pyramidBag.quantity[FRONTIER_LVL_50][i] = 0;
    }
    AddPyramidBagItem(ITEM_POKE_BALL, MOD_RUINS_START_BALLS);
    AddPyramidBagItem(ITEM_POTION, MOD_RUINS_START_POTIONS);
    SetupFloor();
}

void ShiftingRuins_IsActive(void)
{
    gSpecialVar_Result = InShiftingRuins();
}

// Au retour sur un étage (après un combat ou un menu) : résultat du dernier combat.
void ShiftingRuins_OnFloorResume(void)
{
    u32 outcome = gBattleOutcome & ~B_OUTCOME_LINK_BATTLE_RAN;

    gBattleOutcome = 0;
    if (outcome == B_OUTCOME_LOST || outcome == B_OUTCOME_DREW || outcome == B_OUTCOME_FORFEITED)
        gSpecialVar_Result = RUINS_RESUME_LOST;
    else if (outcome == B_OUTCOME_WON && (gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        gSpecialVar_Result = RUINS_RESUME_WON;
    else
        gSpecialVar_Result = RUINS_RESUME_NONE;
}

// Escalier de l'étage : VAR_RESULT = RUINS_STAIRS_*. STR_VAR_1 : étage actuel.
void ShiftingRuins_CheckStairs(void)
{
    ConvertIntToDecimalStringN(gStringVar1, sRuins.floor, STR_CONV_MODE_LEFT_ALIGN, 3);
    if (sRuins.floor >= MOD_RUINS_MAX_FLOOR)
        gSpecialVar_Result = RUINS_STAIRS_BOTTOM;
    else if (ShiftingRuins_IsLandingFloor(sRuins.floor))
        gSpecialVar_Result = RUINS_STAIRS_LANDING;
    else
        gSpecialVar_Result = RUINS_STAIRS_NEXT;
}

void ShiftingRuins_GoDeeper(void)
{
    if (sRuins.floor < MOD_RUINS_MAX_FLOOR)
        sRuins.floor++;
    SetupFloor();
}

// Objet rare ramassé (VAR_0x8000) : mis de côté jusqu'à la sortie. STR_VAR_2 : son nom.
void ShiftingRuins_TakeRelic(void)
{
    CopyItemName(gSpecialVar_0x8000, gStringVar2);
    if (sRuins.relicCount < MOD_RUINS_MAX_RELICS)
        sRuins.relics[sRuins.relicCount++] = gSpecialVar_0x8000;
}

// VAR_0x8004 : RUINS_END_SECURED ou RUINS_END_LOST.
void ShiftingRuins_EndRun(void)
{
    sRuins.end = gSpecialVar_0x8004;
    if (sRuins.end == RUINS_END_LOST)
        sRuins.relicCount = 0;
}

// À l'entrée des ruines : VAR_RESULT = fin de la tentative à raconter (RUINS_END_*).
void ShiftingRuins_CheckDebrief(void)
{
    if (gShiftingRuinsActive && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
        gSpecialVar_Result = sRuins.end;
    else
        gSpecialVar_Result = RUINS_END_NONE;
}

// Donne les objets rares rapportés. VAR_RESULT : TRUE s'il y en avait. gStringVar4 : la liste.
void ShiftingRuins_GiveRelics(void)
{
    u32 i, j;
    u8 *str = StringCopy(gStringVar4, sText_BroughtBack);
    bool32 first = TRUE;

    gSpecialVar_Result = (sRuins.relicCount != 0);
    for (i = 0; i < sRuins.relicCount; i++)
    {
        u32 count = 0;
        enum Item item = sRuins.relics[i];

        if (item == ITEM_NONE)
            continue;
        for (j = i; j < sRuins.relicCount; j++)
        {
            if (sRuins.relics[j] == item)
            {
                count++;
                sRuins.relics[j] = ITEM_NONE;
            }
        }
        AddBagItem(item, count);
        if (!first)
            str = StringAppend(str, sText_Comma);
        first = FALSE;
        CopyItemName(item, gStringVar3);
        str = StringAppend(str, gStringVar3);
        if (count > 1)
        {
            str = StringAppend(str, sText_Times);
            str = ConvertIntToDecimalStringN(str, count, STR_CONV_MODE_LEFT_ALIGN, 2);
        }
    }
    StringAppend(str, sText_Dot);
    sRuins.relicCount = 0;
}

// Après ChoosePartyMon (VAR_0x8004) : Pokémon à ramener. VAR_RESULT : TRUE si choisi. STR_VAR_1 : son nom.
void ShiftingRuins_ChooseKeptMon(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];

    sRuins.keptSlot = PARTY_SIZE;
    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 >= PARTY_SIZE
     || GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE
     || GetMonData(mon, MON_DATA_IS_EGG))
        return;
    sRuins.keptSlot = gSpecialVar_0x8004;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    gSpecialVar_Result = TRUE;
}

// Fin de la tentative : rend l'équipe et les données de la Zone de Combat, ajoute le Pokémon
// ramené et enregistre le record. VAR_RESULT : MON_GIVEN_TO_PARTY, MON_GIVEN_TO_PC,
// MON_CANT_GIVE ou PARTY_SIZE (aucun). VAR_0x8005 : TRUE si un départ meilleur est débloqué.
// STR_VAR_1 : nom du Pokémon ramené. STR_VAR_2 : étage atteint.
void ShiftingRuins_Finish(void)
{
    struct Pokemon kept;
    bool32 hasKept = (sRuins.end == RUINS_END_SECURED && sRuins.keptSlot < PARTY_SIZE);
    u32 oldRecord = VarGet(VAR_MOD_RUINS_RECORD);

    if (hasKept)
        kept = gParties[B_TRAINER_PLAYER][sRuins.keptSlot];

    memcpy(gParties[B_TRAINER_PLAYER], sRuins.campaignParty, sizeof(sRuins.campaignParty));
    gPartiesCount[B_TRAINER_PLAYER] = sRuins.campaignPartyCount;
    memcpy(&gSaveBlock2Ptr->frontier, &sRuins.frontierBackup, sizeof(sRuins.frontierBackup));

    gSpecialVar_Result = PARTY_SIZE;
    if (hasKept)
    {
        enum NationalDexOrder dexNum = SpeciesToNationalPokedexNum(GetMonData(&kept, MON_DATA_SPECIES));

        ShiftingRuins_ResetKeptMon(&kept);
        GetMonData(&kept, MON_DATA_NICKNAME, gStringVar1);
        StringGet_Nickname(gStringVar1);
        gSpecialVar_Result = GiveCapturedMonToPlayer(&kept);
        if (gSpecialVar_Result != MON_CANT_GIVE)
        {
            GetSetPokedexFlag(dexNum, FLAG_SET_SEEN);
            GetSetPokedexFlag(dexNum, FLAG_SET_CAUGHT);
        }
    }

    ConvertIntToDecimalStringN(gStringVar2, sRuins.floor, STR_CONV_MODE_LEFT_ALIGN, 3);
    gSpecialVar_0x8005 = FALSE;
    if (sRuins.end == RUINS_END_SECURED && sRuins.floor > oldRecord)
    {
        VarSet(VAR_MOD_RUINS_RECORD, sRuins.floor);
        gSpecialVar_0x8005 = ShiftingRuins_GetStartMonCount(sRuins.floor) > ShiftingRuins_GetStartMonCount(oldRecord)
                          || ShiftingRuins_GetStartTier(sRuins.floor) > ShiftingRuins_GetStartTier(oldRecord);
    }

    sRuins.end = RUINS_END_NONE;
    sRuins.keptSlot = PARTY_SIZE;
    gShiftingRuinsActive = FALSE;
}
