#include "global.h"
#include "battle.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "event_data.h"
#include "event_scripts.h"
#include "item.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "list_menu.h"
#include "malloc.h"
#include "random.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "shifting_ruins.h"
#include "string_util.h"
#include "trainer_build.h"
#include "text.h"
#include "wild_encounter.h"
#include "constants/battle.h"
#include "constants/battle_frontier.h"
#include "constants/battle_pyramid.h"
#include "constants/items.h"
#include "constants/event_objects.h"
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
// Étape 16 : un gardien barre l'escalier tous les MOD_RUINS_BOSS_EVERY étages et laisse un
// Mutation Seal ; certains escaliers se divisent en deux passages (captures, soins, difficulté
// et récompenses) ; un éclaireur se tient parfois à la place d'un objet et ajoute une espèce
// aux départs suivants, une fois sorti vivant.

STATIC_ASSERT(MOD_RUINS_LANDING_EVERY > 0, RuinsLandingEveryNotZero);
STATIC_ASSERT(MOD_RUINS_MAX_FLOOR <= 255, RuinsMaxFloorFitsInU8);
STATIC_ASSERT(MOD_RUINS_MAX_START_MONS >= 1 && MOD_RUINS_MAX_START_MONS <= PARTY_SIZE, RuinsStartMons);
STATIC_ASSERT(MOD_RUINS_TRAINER_MONS_MAX >= 1 && MOD_RUINS_TRAINER_MONS_MAX <= PARTY_SIZE, RuinsTrainerMons);
STATIC_ASSERT(MOD_RUINS_BOSS_EVERY > 0, RuinsBossEveryNotZero);

enum
{
    BOSS_WAITING,  // Le gardien de l'étage n'a pas encore été vaincu.
    BOSS_FIGHTING, // Combat en cours : le retour sur l'étage est géré par le script de l'escalier.
    BOSS_DEFEATED,
};

struct ShiftingRuinsRun
{
    struct Pokemon campaignParty[PARTY_SIZE];
    struct BattleFrontier frontierBackup;
    u16 relics[MOD_RUINS_MAX_RELICS];
    u16 pendingRecruits; // Recrues rencontrées, acquises à la sortie (bits de sRecruits).
    u8 campaignPartyCount;
    u8 relicCount;
    u8 floor;
    u8 end;
    u8 keptSlot;
    u8 path;            // Passage emprunté pour arriver sur cet étage (RUINS_PATH_*).
    u8 nextPath;        // Passage choisi à l'escalier.
    u8 branches[2];     // Passages proposés par l'escalier de cet étage (RUINS_PATH_PLAIN : aucun).
    u8 bossState;
    u8 recruiterHere:1; // Un éclaireur attend sur cet étage...
    u8 recruiterMet:1;  // ...et le joueur lui a déjà parlé.
    u8 recruiterLocalId;
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

// Espèces que les éclaireurs peuvent ajouter aux départs (1 bit chacune dans VAR_MOD_RUINS_RECRUITS).
static const u16 sRecruits[] =
{
    SPECIES_EEVEE, SPECIES_PIKACHU, SPECIES_RIOLU, SPECIES_GASTLY, SPECIES_ABRA, SPECIES_SCYTHER,
    SPECIES_FEEBAS, SPECIES_MAGNEMITE, SPECIES_SNEASEL, SPECIES_SKARMORY, SPECIES_BULBASAUR,
    SPECIES_CHARMANDER, SPECIES_SQUIRTLE, SPECIES_CHIKORITA, SPECIES_CYNDAQUIL, SPECIES_TOTODILE,
};
STATIC_ASSERT(ARRAY_COUNT(sRecruits) <= 16, RecruitsFitInVar);

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
static const u8 sText_BossDefeat[] = _("The way down is yours. Go on.");
static const u8 sText_BossVictory[] = _("The ruins keep their secrets.");
static const u8 *const sPathNames[RUINS_PATH_COUNT] =
{
    [RUINS_PATH_PLAIN]     = COMPOUND_STRING("Plain stairs"),
    [RUINS_PATH_OVERGROWN] = COMPOUND_STRING("Overgrown"),
    [RUINS_PATH_SPRING]    = COMPOUND_STRING("Spring"),
    [RUINS_PATH_PERILOUS]  = COMPOUND_STRING("Perilous"),
};
// Effet de chaque passage, dans le panneau à côté de la liste. STR_VAR_1 et 2 : valeurs de mod.h.
static const u8 *const sPathDescriptions[RUINS_PATH_COUNT] =
{
    [RUINS_PATH_PLAIN]     = COMPOUND_STRING("The usual way down."),
    [RUINS_PATH_OVERGROWN] = COMPOUND_STRING("Rarer wild POKéMON,\nas if {STR_VAR_1} levels\ndeeper. All their IVs\nare {STR_VAR_2} or more."),
    [RUINS_PATH_SPRING]    = COMPOUND_STRING("A spring heals your\nwhole team on the\nway down."),
    [RUINS_PATH_PERILOUS]  = COMPOUND_STRING("Wild POKéMON, trainers\nand guardian get\n+{STR_VAR_1} levels.\nRare finds x{STR_VAR_2}."),
};
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

bool32 ShiftingRuins_IsBossFloor(u32 floor)
{
    return floor != 0 && floor % MOD_RUINS_BOSS_EVERY == 0;
}

// Niveaux en plus sur un étage atteint par ce passage.
u32 ShiftingRuins_GetPathLevelBonus(u32 path)
{
    return path == RUINS_PATH_PERILOUS ? MOD_RUINS_PERILOUS_LEVELS : 0;
}

u32 ShiftingRuins_GetBossLevel(u32 floor)
{
    return min(ShiftingRuins_GetTrainerLevel(floor) + MOD_RUINS_BOSS_LEVEL_BONUS, MAX_LEVEL);
}

u32 ShiftingRuins_GetBossPartySize(u32 floor)
{
    return min(ShiftingRuins_GetTrainerPartySize(floor) + MOD_RUINS_BOSS_EXTRA_MONS, PARTY_SIZE);
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

// Le passage « Perilous » multiplie les chances d'objets rares, pas celles de l'Escape Orb.
enum Item ShiftingRuins_RollFloorItem(u32 floor, u32 path, u32 rand)
{
    u32 roll = rand % 100;
    u32 factor = (path == RUINS_PATH_PERILOUS) ? MOD_RUINS_PERILOUS_RELICS : 1;
    u32 threshold = MOD_RUINS_RELIC_CHANCE * factor;

    if (roll < threshold)
        return ITEM_TECHNIQUE_CODEX;
    if (GetLandingsPassed(floor) >= 1)
    {
        threshold += MOD_RUINS_RARE_RELIC_CHANCE * factor;
        if (roll < threshold)
            return ITEM_LINEAGE_CHARM;
    }
    if (GetLandingsPassed(floor) >= 2)
    {
        threshold += MOD_RUINS_SEAL_CHANCE * factor;
        if (roll < threshold)
            return ITEM_MUTATION_SEAL;
    }
#if MOD_LEVELS_BEYOND_100
    if (GetLandingsPassed(floor) >= 2)
    {
        threshold += MOD_RUINS_RARE_RELIC_CHANCE * factor;
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
        || item == ITEM_MUTATION_SEAL
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

void ShiftingRuins_CreateTrainerParty(struct Pokemon *party, u32 floor, u32 path)
{
    u32 i;
    u32 size = ShiftingRuins_GetTrainerPartySize(floor);
    u32 level = min(ShiftingRuins_GetTrainerLevel(floor) + ShiftingRuins_GetPathLevelBonus(path), MAX_LEVEL);
    u32 iv = min(MAX_PER_STAT_IVS, 5 + floor / 2);

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < size; i++)
    {
        CreateMonWithIVs(&party[i], ShiftingRuins_PickSpecies(level, Random32()), level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY, iv);
        GiveMonInitialMoveset(&party[i]);
    }
}

// Gardien : une équipe plus grande et plus forte, IV au maximum, avec arbre et variante
// comme un champion d'arène (TRAINER_BUILD_STRONG).
void ShiftingRuins_CreateBossParty(struct Pokemon *party, u32 floor, u32 path)
{
    u32 i;
    u32 size = ShiftingRuins_GetBossPartySize(floor);
    u32 level = min(ShiftingRuins_GetBossLevel(floor) + ShiftingRuins_GetPathLevelBonus(path), MAX_LEVEL);

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < size; i++)
    {
        enum Species species;
        do
        {
            species = ShiftingRuins_PickSpecies(level, Random32());
        } while (i != 0 && GetMonData(&party[i - 1], MON_DATA_SPECIES) == species);
        CreateMonWithIVs(&party[i], species, level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY, MAX_PER_STAT_IVS);
        GiveMonInitialMoveset(&party[i]);
        ApplyMonTrainerBuild(&party[i], TRAINER_BUILD_STRONG);
    }
}

u32 ShiftingRuins_GetRecruitCount(void)
{
    return ARRAY_COUNT(sRecruits);
}

enum Species ShiftingRuins_GetRecruitSpecies(u32 index)
{
    return index < ARRAY_COUNT(sRecruits) ? sRecruits[index] : SPECIES_NONE;
}

// Tire une recrue parmi les bits de mask (SPECIES_NONE si aucune).
enum Species ShiftingRuins_PickRecruit(u32 mask, u32 rand)
{
    u32 i, count = 0;

    for (i = 0; i < ARRAY_COUNT(sRecruits); i++)
        count += (mask >> i) & 1;
    if (count == 0)
        return SPECIES_NONE;
    rand %= count;
    for (i = 0; i < ARRAY_COUNT(sRecruits); i++)
    {
        if (((mask >> i) & 1) && rand-- == 0)
            return sRecruits[i];
    }
    return SPECIES_NONE;
}

// Recrues que l'éclaireur peut encore proposer : ni acquises, ni déjà rencontrées pendant la tentative.
static u32 GetUnmetRecruits(void)
{
    u32 all = (1u << ARRAY_COUNT(sRecruits)) - 1;
    return all & ~(VarGet(VAR_MOD_RUINS_RECRUITS) | sRuins.pendingRecruits);
}

// Escalier ordinaire qui se divise : deux passages différents parmi les trois spéciaux.
void ShiftingRuins_RollBranches(u8 *branches, u32 floor, u32 rand)
{
    u32 first, second;

    branches[0] = branches[1] = RUINS_PATH_PLAIN;
    if (ShiftingRuins_IsLandingFloor(floor) || ShiftingRuins_IsBossFloor(floor) || floor >= MOD_RUINS_MAX_FLOOR)
        return;
    if (rand % 100 >= MOD_RUINS_BRANCH_CHANCE)
        return;
    rand /= 100;
    first = RUINS_PATH_OVERGROWN + rand % (RUINS_PATH_COUNT - 1);
    second = RUINS_PATH_OVERGROWN + (first - RUINS_PATH_OVERGROWN + 1 + (rand / 4) % (RUINS_PATH_COUNT - 2)) % (RUINS_PATH_COUNT - 1);
    branches[0] = first;
    branches[1] = second;
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

    sRuins.bossState = BOSS_WAITING;
    ShiftingRuins_RollBranches(sRuins.branches, sRuins.floor, Random32());
    sRuins.nextPath = RUINS_PATH_PLAIN;
    sRuins.recruiterMet = FALSE;
    sRuins.recruiterLocalId = 0;
    sRuins.recruiterHere = !ShiftingRuins_IsBossFloor(sRuins.floor)
                        && GetUnmetRecruits() != 0
                        && Random() % 100 < MOD_RUINS_RECRUITER_CHANCE;
}

// Passage « Overgrown » : espèces tirées comme plus bas dans les ruines, et de meilleurs IV.
void ShiftingRuins_GenerateWildMon(void)
{
    u32 level = min(ShiftingRuins_GetWildLevel(sRuins.floor) + ShiftingRuins_GetPathLevelBonus(sRuins.path), MAX_LEVEL);
    u32 poolLevel = level;
    u32 i;

    if (sRuins.path == RUINS_PATH_OVERGROWN)
        poolLevel = min(level + MOD_RUINS_OVERGROWN_BONUS, MAX_LEVEL);
    CreateWildMon(ShiftingRuins_PickSpecies(poolLevel, Random32()), level);
    if (sRuins.path == RUINS_PATH_OVERGROWN)
    {
        for (i = 0; i < NUM_STATS; i++)
        {
            u32 iv = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV + i);
            if (iv < MOD_RUINS_OVERGROWN_IV)
            {
                iv = MOD_RUINS_OVERGROWN_IV;
                SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV + i, &iv);
            }
        }
        CalculateMonStats(&gParties[B_TRAINER_OPPONENT_A][0]);
    }
}

// Combat de dresseur dans les ruines : un combat ordinaire (expérience, évolutions) contre
// une équipe générée ; l'apparence et les répliques du dresseur restent celles de la Pyramide.
void ShiftingRuins_SetupTrainerBattle(void)
{
    VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
    MarkApproachingPyramidTrainersAsBattled();
    ShiftingRuins_CreateTrainerParty(gParties[B_TRAINER_OPPONENT_A], sRuins.floor, sRuins.path);
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_RUINS;
    TRAINER_BATTLE_PARAM.opponentB = 0;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_TrainerDefeat;
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)sText_TrainerVictory;
}

// Gardien (DoSpecialTrainerBattle, SPECIAL_BATTLE_RUINS_BOSS) : combat ordinaire, sans PYRAMID.
void ShiftingRuins_SetupBossBattle(void)
{
    VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
    ShiftingRuins_CreateBossParty(gParties[B_TRAINER_OPPONENT_A], sRuins.floor, sRuins.path);
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    InitTrainerBattleParameter();
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_RUINS;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_BossDefeat;
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)sText_BossVictory;
}

void ShiftingRuins_SetupWildBattle(void)
{
    VarSet(VAR_TEMP_E, 0);
}

void ShiftingRuins_SetFloorItem(void)
{
    enum Item item = ShiftingRuins_RollFloorItem(sRuins.floor, sRuins.path, Random32());

    gSpecialVar_0x8000 = item;
    gSpecialVar_0x8001 = 1;
    gSpecialVar_Result = ShiftingRuins_IsRelic(item);
}

// À la création des objets de l'étage : l'éclaireur prend la place du premier objet.
void ShiftingRuins_PlaceRecruiter(struct ObjectEventTemplate *events, u32 count)
{
    u32 i;

    sRuins.recruiterLocalId = 0;
    if (!sRuins.recruiterHere)
        return;
    for (i = 0; i < count; i++)
    {
        if (events[i].localId != LOCALID_NONE && events[i].graphicsId == OBJ_EVENT_GFX_ITEM_BALL)
        {
            events[i].graphicsId = OBJ_EVENT_GFX_EXPERT_M;
            // Les gabarits copiés des carrés de la Pyramide portent déjà le script de l'objet :
            // le remplacer ici, sinon l'éclaireur donne un objet quand on lui parle.
            events[i].script = ShiftingRuins_EventScript_Recruiter;
            sRuins.recruiterLocalId = events[i].localId;
            return;
        }
    }
}

bool32 ShiftingRuins_IsRecruiter(u32 localId)
{
    return InShiftingRuins() && sRuins.recruiterLocalId != 0 && localId == sRuins.recruiterLocalId;
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
    u32 recruits = VarGet(VAR_MOD_RUINS_RECRUITS);
    u32 pools = maxTier + 1 + (recruits != 0);
    u8 *str = gStringVar4;

    memcpy(sRuins.campaignParty, gParties[B_TRAINER_PLAYER], sizeof(sRuins.campaignParty));
    sRuins.campaignPartyCount = gPartiesCount[B_TRAINER_PLAYER];
    memcpy(&sRuins.frontierBackup, &gSaveBlock2Ptr->frontier, sizeof(sRuins.frontierBackup));
    sRuins.relicCount = 0;
    sRuins.floor = 1;
    sRuins.end = RUINS_END_NONE;
    sRuins.keptSlot = PARTY_SIZE;
    sRuins.pendingRecruits = 0;
    sRuins.path = RUINS_PATH_PLAIN;
    gShiftingRuinsActive = TRUE;

    str = StringCopy(str, sText_YourTeam);
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
    for (i = 0; i < count; i++)
    {
        enum Species species;
        // Un tirage par Pokémon dans un des viviers ouverts (les recrues forment un vivier de plus), sans doublon.
        do
        {
            u32 pool = Random() % pools;
            if (pool > maxTier)
                species = ShiftingRuins_PickRecruit(recruits, Random());
            else
                species = ShiftingRuins_PickStartSpecies(pool, Random());
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

    // Combat contre le gardien : le script de l'escalier reprend la main (ShiftingRuins_EndBossBattle).
    if (sRuins.bossState == BOSS_FIGHTING)
    {
        gSpecialVar_Result = RUINS_RESUME_NONE;
        return;
    }
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
    if (ShiftingRuins_IsBossFloor(sRuins.floor) && sRuins.bossState != BOSS_DEFEATED)
        gSpecialVar_Result = RUINS_STAIRS_BOSS;
    else if (sRuins.floor >= MOD_RUINS_MAX_FLOOR)
        gSpecialVar_Result = RUINS_STAIRS_BOTTOM;
    else if (ShiftingRuins_IsLandingFloor(sRuins.floor))
        gSpecialVar_Result = RUINS_STAIRS_LANDING;
    else
        gSpecialVar_Result = RUINS_STAIRS_NEXT;
}

// VAR_RESULT : TRUE si l'escalier se divise (ShiftingRuins_PushPaths).
void ShiftingRuins_HasBranches(void)
{
    gSpecialVar_Result = (sRuins.branches[0] != RUINS_PATH_PLAIN);
}

void ShiftingRuins_PushPaths(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sRuins.branches); i++)
    {
        struct ListMenuItem item;
        u8 *name = Alloc(40);

        StringCopy(name, sPathNames[sRuins.branches[i]]);
        item.name = name;
        item.id = sRuins.branches[i];
        MultichoiceDynamic_PushElement(item);
    }
    gDynamicMultichoiceDescribe = ShiftingRuins_DescribePath;
}

void ShiftingRuins_DescribePath(u32 path, u8 *dest)
{
    if (path >= RUINS_PATH_COUNT)
        path = RUINS_PATH_PLAIN;
    if (path == RUINS_PATH_PERILOUS)
    {
        ConvertIntToDecimalStringN(gStringVar1, MOD_RUINS_PERILOUS_LEVELS, STR_CONV_MODE_LEFT_ALIGN, 2);
        ConvertIntToDecimalStringN(gStringVar2, MOD_RUINS_PERILOUS_RELICS, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(dest, sPathDescriptions[path]);
        return;
    }
    ConvertIntToDecimalStringN(gStringVar1, MOD_RUINS_OVERGROWN_BONUS, STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, MOD_RUINS_OVERGROWN_IV, STR_CONV_MODE_LEFT_ALIGN, 2);
    StringExpandPlaceholders(dest, sPathDescriptions[path]);
}

// VAR_0x8004 : passage choisi.
void ShiftingRuins_TakePath(void)
{
    if (gSpecialVar_0x8004 == sRuins.branches[0] || gSpecialVar_0x8004 == sRuins.branches[1])
        sRuins.nextPath = gSpecialVar_0x8004;
}

// VAR_RESULT : passage emprunté (RUINS_PATH_SPRING : équipe soignée).
void ShiftingRuins_GoDeeper(void)
{
    sRuins.path = sRuins.nextPath;
    if (sRuins.floor < MOD_RUINS_MAX_FLOOR)
        sRuins.floor++;
    SetupFloor();
    if (sRuins.path == RUINS_PATH_SPRING)
        HealPlayerParty();
    gSpecialVar_Result = sRuins.path;
}

void ShiftingRuins_StartBossBattle(void)
{
    sRuins.bossState = BOSS_FIGHTING;
}

// Après le combat contre le gardien. VAR_RESULT : TRUE si gagné ; le Mutation Seal laissé
// par le gardien est mis de côté (STR_VAR_2 : son nom).
void ShiftingRuins_EndBossBattle(void)
{
    u32 outcome = gBattleOutcome & ~B_OUTCOME_LINK_BATTLE_RAN;

    gBattleOutcome = 0;
    gSpecialVar_Result = (outcome == B_OUTCOME_WON);
    sRuins.bossState = gSpecialVar_Result ? BOSS_DEFEATED : BOSS_WAITING;
    if (gSpecialVar_Result)
    {
        gSpecialVar_0x8000 = ITEM_MUTATION_SEAL;
        ShiftingRuins_TakeRelic();
    }
}

// L'éclaireur : VAR_RESULT = TRUE à la première rencontre, STR_VAR_1 : l'espèce recommandée.
void ShiftingRuins_MeetRecruiter(void)
{
    u32 i, unmet;
    enum Species species;

    gSpecialVar_Result = FALSE;
    if (sRuins.recruiterMet)
        return;
    unmet = GetUnmetRecruits();
    species = ShiftingRuins_PickRecruit(unmet, Random());
    if (species == SPECIES_NONE)
        return;
    for (i = 0; i < ARRAY_COUNT(sRecruits); i++)
    {
        if (sRecruits[i] == species)
            sRuins.pendingRecruits |= 1u << i;
    }
    sRuins.recruiterMet = TRUE;
    StringCopy(gStringVar1, GetSpeciesName(species));
    gSpecialVar_Result = TRUE;
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
    {
        sRuins.relicCount = 0;
        sRuins.pendingRecruits = 0;
    }
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
    ShiftingRuins_BufferKeptSummary(mon, gStringVar4);
    gSpecialVar_Result = TRUE;
}

// Ce que le Pokémon ramené garde (nature, talent, IV, mutation), puis la question de confirmation.
void ShiftingRuins_BufferKeptSummary(struct Pokemon *mon, u8 *dest)
{
    static const u8 *const sIvLabels[NUM_STATS] =
    {
        [STAT_HP]    = COMPOUND_STRING("IVs  HP "),
        [STAT_ATK]   = COMPOUND_STRING("  Atk "),
        [STAT_DEF]   = COMPOUND_STRING("  Def "),
        [STAT_SPEED] = COMPOUND_STRING("  Spe "),
        [STAT_SPATK] = COMPOUND_STRING("SpA "),
        [STAT_SPDEF] = COMPOUND_STRING("  SpD "),
    };
    // Ordre d'affichage : PV, Attaque, Défense, puis Attaque Spé., Défense Spé., Vitesse.
    static const u8 sIvOrder[NUM_STATS] = { STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED };
    enum Type mutation = mon->box.build.mutationType;
    u8 *end = StringCopy(dest, gStringVar1);
    u32 i;

    end = StringCopy(end, COMPOUND_STRING(" keeps its "));
    end = StringCopy(end, gNaturesInfo[GetNature(mon)].name);
    end = StringCopy(end, COMPOUND_STRING(" nature\nand "));
    end = StringCopy(end, gAbilitiesInfo[GetMonAbility(mon)].name);
    end = StringCopy(end, COMPOUND_STRING(".\p"));
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 stat = sIvOrder[i];

        if (i == 3)
            *end++ = CHAR_NEWLINE;
        end = StringCopy(end, sIvLabels[stat]);
        end = ConvertIntToDecimalStringN(end, GetMonData(mon, MON_DATA_HP_IV + stat), STR_CONV_MODE_LEFT_ALIGN, 2);
    }
    *end++ = CHAR_PROMPT_CLEAR;
    if (mutation != TYPE_NONE)
    {
        end = StringCopy(end, COMPOUND_STRING("Its "));
        end = StringCopy(end, gTypesInfo[mutation].name);
        end = StringCopy(end, COMPOUND_STRING(" mutation stays.\n"));
    }
    StringCopy(end, COMPOUND_STRING("Bring it back at level 5?"));
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
    // Recrues rencontrées : acquises pour les départs suivants. VAR_0x8006 : TRUE s'il y en a, STR_VAR_3 : leurs noms.
    gSpecialVar_0x8006 = FALSE;
    if (sRuins.end == RUINS_END_SECURED && sRuins.pendingRecruits != 0)
    {
        u32 i;
        u8 *str = gStringVar3;

        VarSet(VAR_MOD_RUINS_RECRUITS, VarGet(VAR_MOD_RUINS_RECRUITS) | sRuins.pendingRecruits);
        *str = EOS;
        for (i = 0; i < ARRAY_COUNT(sRecruits); i++)
        {
            if (!((sRuins.pendingRecruits >> i) & 1))
                continue;
            if (gSpecialVar_0x8006)
                str = StringAppend(str, sText_Comma);
            str = StringAppend(str, GetSpeciesName(sRecruits[i]));
            gSpecialVar_0x8006 = TRUE;
        }
    }
    sRuins.pendingRecruits = 0;

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
