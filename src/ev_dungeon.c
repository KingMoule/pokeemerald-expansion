#include "global.h"
#include "battle.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "event_data.h"
#include "ev_dungeon.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "random.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "text.h"
#include "constants/battle.h"
#include "constants/opponents.h"

// Donjon d'entraînement des EV, « Training Depths » (wiki section 10).
// Une visite enchaîne des étages, chacun étant un combat de dresseur contre une équipe générée.
// Les EV gagnés restent en attente jusqu'à une sortie, possible seulement aux paliers de repos ;
// une défaite n'en conserve qu'une partie. Les EV acquis avant la visite ne sont jamais touchés.
// Les valeurs sont dans include/config/mod.h (MOD_EVDJ_*). Script : data/scripts/mod_ev_dungeon.inc.

STATIC_ASSERT(MOD_EVDJ_REST_EVERY > 0, EvDungeonRestEveryNotZero);
STATIC_ASSERT(MOD_EVDJ_MAX_FLOOR <= 255, EvDungeonMaxFloorFitsInU8);
STATIC_ASSERT(MOD_EVDJ_PARTY_SIZE_BASE >= 1 && MOD_EVDJ_PARTY_SIZE_BASE <= PARTY_SIZE, EvDungeonPartySizeBase);

// État de la visite en cours. En EWRAM : il survit aux combats, pas à un redémarrage
// (les gains non sécurisés sont alors perdus, comme le veut le wiki).
struct EvDungeonRun
{
    u32 personality;
    u32 otId;
    u16 pending;
    u8 path;
    u8 floor;
    bool8 active;
};

static EWRAM_DATA struct EvDungeonRun sRun = {0};

struct EvDungeonMon
{
    u16 species;
    u8 minLevel;
    u8 maxLevel;
};

// Adversaires de chaque voie, avec la tranche de niveaux où chaque forme apparaît.
static const struct EvDungeonMon sPowerMons[] =
{
    { SPECIES_MACHOP,     1, 27 }, { SPECIES_MACHOKE,   28, 45 }, { SPECIES_MACHAMP,   46, 255 },
    { SPECIES_MAKUHITA,   1, 23 }, { SPECIES_HARIYAMA,  24, 255 },
    { SPECIES_POOCHYENA,  1, 17 }, { SPECIES_MIGHTYENA, 18, 255 },
    { SPECIES_ZIGZAGOON,  1, 19 }, { SPECIES_LINOONE,   20, 255 },
    { SPECIES_ZANGOOSE,  20, 255 },
    { SPECIES_BAGON,      1, 29 }, { SPECIES_SHELGON,   30, 49 }, { SPECIES_SALAMENCE, 50, 255 },
};

static const struct EvDungeonMon sFocusMons[] =
{
    { SPECIES_RALTS,      1, 19 }, { SPECIES_KIRLIA,    20, 29 }, { SPECIES_GARDEVOIR, 30, 255 },
    { SPECIES_MAGNEMITE,  1, 29 }, { SPECIES_MAGNETON,  30, 255 },
    { SPECIES_SLUGMA,     1, 37 }, { SPECIES_MAGCARGO,  38, 255 },
    { SPECIES_SPOINK,     1, 31 }, { SPECIES_GRUMPIG,   32, 255 },
    { SPECIES_ROSELIA,    1, 255 },
    { SPECIES_CHIMECHO,   1, 255 },
};

static const struct EvDungeonMon sEnduranceMons[] =
{
    { SPECIES_WINGULL,    1, 24 }, { SPECIES_PELIPPER,  25, 255 },
    { SPECIES_WHISMUR,    1, 19 }, { SPECIES_LOUDRED,   20, 39 }, { SPECIES_EXPLOUD,   40, 255 },
    { SPECIES_WAILMER,    1, 39 }, { SPECIES_WAILORD,   40, 255 },
    { SPECIES_SPHEAL,     1, 31 }, { SPECIES_SEALEO,    32, 43 }, { SPECIES_WALREIN,   44, 255 },
    { SPECIES_LOTAD,      1, 13 }, { SPECIES_LOMBRE,    14, 29 }, { SPECIES_LUDICOLO,  30, 255 },
    { SPECIES_TORKOAL,    1, 255 },
};

static const struct EvDungeonMon sResistanceMons[] =
{
    { SPECIES_GEODUDE,    1, 24 }, { SPECIES_GRAVELER,  25, 44 }, { SPECIES_GOLEM,     45, 255 },
    { SPECIES_SHROOMISH,  1, 22 }, { SPECIES_BRELOOM,   23, 255 },
    { SPECIES_CARVANHA,   1, 29 }, { SPECIES_SHARPEDO,  30, 255 },
    { SPECIES_CORPHISH,   1, 29 }, { SPECIES_CRAWDAUNT, 30, 255 },
    { SPECIES_TRAPINCH,   1, 34 }, { SPECIES_VIBRAVA,   35, 44 }, { SPECIES_FLYGON,    45, 255 },
    { SPECIES_ANORITH,    1, 39 }, { SPECIES_ARMALDO,   40, 255 },
};

static const struct EvDungeonMon sDisciplineMons[] =
{
    { SPECIES_DUSKULL,    1, 36 }, { SPECIES_DUSCLOPS,  37, 255 },
    { SPECIES_SHUPPET,    1, 36 }, { SPECIES_BANETTE,   37, 255 },
    { SPECIES_ODDISH,     1, 20 }, { SPECIES_GLOOM,     21, 35 }, { SPECIES_VILEPLUME, 36, 255 },
    { SPECIES_NATU,       1, 24 }, { SPECIES_XATU,      25, 255 },
    { SPECIES_BALTOY,     1, 35 }, { SPECIES_CLAYDOL,   36, 255 },
    { SPECIES_TENTACOOL,  1, 29 }, { SPECIES_TENTACRUEL, 30, 255 },
};

static const struct EvDungeonMon sAgilityMons[] =
{
    { SPECIES_TAILLOW,    1, 21 }, { SPECIES_SWELLOW,   22, 255 },
    { SPECIES_ELECTRIKE,  1, 25 }, { SPECIES_MANECTRIC, 26, 255 },
    { SPECIES_NINCADA,    1, 19 }, { SPECIES_NINJASK,   20, 255 },
    { SPECIES_ZUBAT,      1, 21 }, { SPECIES_GOLBAT,    22, 39 }, { SPECIES_CROBAT,    40, 255 },
    { SPECIES_SWABLU,     1, 34 }, { SPECIES_ALTARIA,   35, 255 },
    { SPECIES_PLUSLE,     1, 255 }, { SPECIES_MINUN,    1, 255 },
    { SPECIES_ABSOL,     25, 255 },
};

static const u8 sText_PathPower[]      = _("POWER (Attack)");
static const u8 sText_PathFocus[]      = _("FOCUS (Sp. Atk)");
static const u8 sText_PathEndurance[]  = _("ENDURANCE (HP)");
static const u8 sText_PathResistance[] = _("RESISTANCE (Defense)");
static const u8 sText_PathDiscipline[] = _("DISCIPLINE (Sp. Def)");
static const u8 sText_PathAgility[]    = _("AGILITY (Speed)");

static const struct
{
    const struct EvDungeonMon *mons;
    u8 count;
    enum Stat stat;
    const u8 *name;
} sPaths[EV_PATH_COUNT] =
{
    [EV_PATH_POWER]      = { sPowerMons,      ARRAY_COUNT(sPowerMons),      STAT_ATK,   sText_PathPower },
    [EV_PATH_FOCUS]      = { sFocusMons,      ARRAY_COUNT(sFocusMons),      STAT_SPATK, sText_PathFocus },
    [EV_PATH_ENDURANCE]  = { sEnduranceMons,  ARRAY_COUNT(sEnduranceMons),  STAT_HP,    sText_PathEndurance },
    [EV_PATH_RESISTANCE] = { sResistanceMons, ARRAY_COUNT(sResistanceMons), STAT_DEF,   sText_PathResistance },
    [EV_PATH_DISCIPLINE] = { sDisciplineMons, ARRAY_COUNT(sDisciplineMons), STAT_SPDEF, sText_PathDiscipline },
    [EV_PATH_AGILITY]    = { sAgilityMons,    ARRAY_COUNT(sAgilityMons),    STAT_SPEED, sText_PathAgility },
};

static const u8 sText_Floor[]        = _("Floor ");
static const u8 sText_TrainerDefeat[] = _("Good fight! Go deeper, if you dare.");
static const u8 sText_TrainerVictory[] = _("The DEPTHS are not for everyone.");
static const u8 sText_Earned[]       = _(" earned ");
static const u8 sText_EvSuffix[]     = _(" EV.\nUnsecured: ");
static const u8 sText_EvDot[]        = _(" EV.");
static const u8 sText_EvOf[]         = _("'s ");
static const u8 sText_EvColon[]     = _(" EV:\n");
static const u8 sText_Arrow[]        = _(" {RIGHT_ARROW} ");
static const u8 sText_Max[]          = _(" (max ");
static const u8 sText_CloseDot[]     = _(").");
static const u8 sText_TotalEv[]      = _("Total EV:\n");
static const u8 sText_RestPoint[]    = _("A rest point.\nIf you leave now:\p");
static const u8 sText_RestNoHeal[]   = _("\pThe ENDURANCE path offers\nno healing here.");
static const u8 sText_RestHealed[]   = _("\pYour POKéMON were healed.");
static const u8 sText_CapReached[]   = _("\pThe cap is reached. Extra EVs\nwill be lost.");
static const u8 sText_Defeated[]     = _("You were defeated!\n");
static const u8 sText_Keeps[]        = _(" keeps ");
static const u8 sText_KeepsOf[]      = _(" of the\n");
static const u8 sText_UnsecuredEv[]  = _(" unsecured EV.\p");
static const u8 sText_Secured[]      = _("Gains secured!\p");
static const u8 sText_Bottom[]       = _("You reached the deepest floor!\p");

enum Stat EvDungeon_GetPathStat(enum EvDungeonPath path)
{
    return sPaths[path].stat;
}

u32 EvDungeon_GetFloorGain(u32 floor)
{
    return MOD_EVDJ_GAIN_BASE + MOD_EVDJ_GAIN_STEP * ((floor - 1) / MOD_EVDJ_REST_EVERY);
}

u32 EvDungeon_GetEnemyPartySize(u32 floor)
{
    return min(PARTY_SIZE, MOD_EVDJ_PARTY_SIZE_BASE + (floor - 1) / MOD_EVDJ_REST_EVERY);
}

u32 EvDungeon_GetEnemyLevel(u32 floor, u32 partyMaxLevel)
{
    s32 level = (s32)partyMaxLevel - MOD_EVDJ_LEVEL_OFFSET + (s32)(floor - 1) * MOD_EVDJ_LEVELS_PER_FLOOR;
    return max(MIN_LEVEL, min(level, MOD_LEVEL_BASE_CAP));
}

u32 EvDungeon_GetDefeatKeptGain(u32 pending)
{
    return pending * MOD_EVDJ_DEFEAT_KEPT_PCT / 100;
}

bool32 EvDungeon_IsRestFloor(u32 floor)
{
    return floor % MOD_EVDJ_REST_EVERY == 0;
}

// Raccourcis : chaque palier franchi au moins une fois permet de recommencer juste après lui.
// Renvoie le nombre d'étages de départ proposés (le premier est toujours l'étage 1).
u32 EvDungeon_GetUnlockedStartFloors(u32 record)
{
    u32 count = 1 + min(record, MOD_EVDJ_MAX_FLOOR) / MOD_EVDJ_REST_EVERY;
    if (1 + (count - 1) * MOD_EVDJ_REST_EVERY > MOD_EVDJ_MAX_FLOOR)
        count--;
    return count;
}

static u32 GetTotalEvs(struct Pokemon *mon)
{
    u32 i, total = 0;
    for (i = 0; i < NUM_STATS; i++)
        total += GetMonData(mon, MON_DATA_HP_EV + i);
    return total;
}

// Part du gain qui tient sous les plafonds ; l'excédent est perdu (choix par défaut).
u32 EvDungeon_ClampGain(struct Pokemon *mon, enum Stat stat, u32 gain)
{
    u32 current = GetMonData(mon, MON_DATA_HP_EV + stat);
    u32 total = GetTotalEvs(mon);
    u32 roomStat = (current < MAX_PER_STAT_EVS) ? MAX_PER_STAT_EVS - current : 0;
    u32 roomTotal = (total < MAX_TOTAL_EVS) ? MAX_TOTAL_EVS - total : 0;
    return min(gain, min(roomStat, roomTotal));
}

u32 EvDungeon_ApplyGain(struct Pokemon *mon, enum Stat stat, u32 gain)
{
    u32 applied = EvDungeon_ClampGain(mon, stat, gain);
    u32 ev = GetMonData(mon, MON_DATA_HP_EV + stat) + applied;
    SetMonData(mon, MON_DATA_HP_EV + stat, &ev);
    CalculateMonStats(mon);
    return applied;
}

static u32 GetPartyMaxLevel(void)
{
    u32 i, maxLevel = MIN_LEVEL;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG))
            maxLevel = max(maxLevel, GetMonData(mon, MON_DATA_LEVEL));
    }
    return maxLevel;
}

static enum Species PickSpecies(enum EvDungeonPath path, u32 level)
{
    const struct EvDungeonMon *mons = sPaths[path].mons;
    u32 i, eligible = 0, pick;

    for (i = 0; i < sPaths[path].count; i++)
    {
        if (level >= mons[i].minLevel && level <= mons[i].maxLevel)
            eligible++;
    }
    if (eligible == 0)
        return mons[0].species;

    pick = Random() % eligible;
    for (i = 0; i < sPaths[path].count; i++)
    {
        if (level >= mons[i].minLevel && level <= mons[i].maxLevel && pick-- == 0)
            return mons[i].species;
    }
    return mons[0].species;
}

void EvDungeon_CreateEnemyParty(struct Pokemon *party, enum EvDungeonPath path, u32 floor, u32 partyMaxLevel)
{
    u32 i;
    u32 size = EvDungeon_GetEnemyPartySize(floor);
    u32 level = EvDungeon_GetEnemyLevel(floor, partyMaxLevel);
    u32 iv = min(MAX_PER_STAT_IVS, MOD_EVDJ_ENEMY_IV_BASE + floor);

    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < size; i++)
    {
        CreateMonWithIVs(&party[i], PickSpecies(path, level), level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY, iv);
        GiveMonInitialMoveset(&party[i]);
    }
}

void EvDungeon_SetupBattle(void)
{
    EvDungeon_CreateEnemyParty(gParties[B_TRAINER_OPPONENT_A], sRun.path, sRun.floor, GetPartyMaxLevel());
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    InitTrainerBattleParameter();
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_EV_DUNGEON;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)sText_TrainerDefeat;
    TRAINER_BATTLE_PARAM.victoryText = (u8 *)sText_TrainerVictory;
}

static struct Pokemon *GetBeneficiary(void)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE
         && GetMonData(mon, MON_DATA_PERSONALITY) == sRun.personality
         && GetMonData(mon, MON_DATA_OT_ID) == sRun.otId)
            return mon;
    }
    return NULL;
}

static u8 *AppendNumber(u8 *dst, u32 value)
{
    return ConvertIntToDecimalStringN(dst, value, STR_CONV_MODE_LEFT_ALIGN, 5);
}

// « Nom's Attack EV: 120 → 150 (max 252). » puis « Total EV: 300 → 330 (max 510). »
static u8 *AppendEvPreview(u8 *dst, struct Pokemon *mon, u32 gain)
{
    enum Stat stat = sPaths[sRun.path].stat;
    u32 current = GetMonData(mon, MON_DATA_HP_EV + stat);
    u32 total = GetTotalEvs(mon);
    u32 applied = EvDungeon_ClampGain(mon, stat, gain);

    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    dst = StringAppend(dst, gStringVar1);
    dst = StringAppend(dst, sText_EvOf);
    dst = StringAppend(dst, gStatNamesTable[stat]);
    dst = StringAppend(dst, sText_EvColon);
    dst = AppendNumber(dst, current);
    dst = StringAppend(dst, sText_Arrow);
    dst = AppendNumber(dst, current + applied);
    dst = StringAppend(dst, sText_Max);
    dst = AppendNumber(dst, MAX_PER_STAT_EVS);
    dst = StringAppend(dst, sText_CloseDot);
    *dst++ = CHAR_PROMPT_CLEAR;
    *dst = EOS;
    dst = StringAppend(dst, sText_TotalEv);
    dst = AppendNumber(dst, total);
    dst = StringAppend(dst, sText_Arrow);
    dst = AppendNumber(dst, total + applied);
    dst = StringAppend(dst, sText_Max);
    dst = AppendNumber(dst, MAX_TOTAL_EVS);
    dst = StringAppend(dst, sText_CloseDot);
    if (applied < gain)
        dst = StringAppend(dst, sText_CapReached);
    return dst;
}

static u8 *BufferPendingPreview(u8 *dst, struct Pokemon *mon)
{
    return AppendEvPreview(dst, mon, sRun.pending);
}

static void PushItem(const u8 *text, u32 id)
{
    struct ListMenuItem item;
    u8 *name = Alloc(32);

    StringCopy(name, text);
    item.name = name;
    item.id = id;
    MultichoiceDynamic_PushElement(item);
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

// VAR_RESULT : TRUE si le joueur a assez de badges. STR_VAR_1 : nombre requis.
void EvDungeon_CheckAccess(void)
{
    ConvertIntToDecimalStringN(gStringVar1, MOD_EVDJ_REQUIRED_BADGES, STR_CONV_MODE_LEFT_ALIGN, 1);
    gSpecialVar_Result = (CountBadges() >= MOD_EVDJ_REQUIRED_BADGES);
}

// VAR_0x8004 : emplacement choisi dans l'équipe. VAR_RESULT : FALSE si annulé ou œuf.
void EvDungeon_SetBeneficiary(void)
{
    struct Pokemon *mon;

    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return;
    mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return;

    sRun.personality = GetMonData(mon, MON_DATA_PERSONALITY);
    sRun.otId = GetMonData(mon, MON_DATA_OT_ID);
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    gSpecialVar_Result = TRUE;
}

void EvDungeon_PushPaths(void)
{
    u32 i;
    for (i = 0; i < EV_PATH_COUNT; i++)
        PushItem(sPaths[i].name, i);
}

// VAR_RESULT : nombre d'étages de départ débloqués (raccourcis). Le choix n'est proposé
// (et la liste préparée) que s'il y en a plusieurs.
void EvDungeon_PushStartFloors(void)
{
    u32 i, count = EvDungeon_GetUnlockedStartFloors(VarGet(VAR_MOD_EV_DUNGEON_RECORD));
    u8 text[16];

    gSpecialVar_Result = count;
    if (count <= 1)
        return;
    for (i = 0; i < count; i++)
    {
        u32 floor = 1 + i * MOD_EVDJ_REST_EVERY;
        u8 *end = StringCopy(text, sText_Floor);
        AppendNumber(end, floor);
        PushItem(text, floor);
    }
}

// VAR_0x8005 : voie choisie. VAR_0x8006 : étage de départ.
void EvDungeon_Start(void)
{
    sRun.path = min(gSpecialVar_0x8005, EV_PATH_COUNT - 1);
    sRun.floor = max(1, min(gSpecialVar_0x8006, MOD_EVDJ_MAX_FLOOR));
    sRun.pending = 0;
    sRun.active = TRUE;
    HealPlayerParty();
}

// STR_VAR_1 : numéro de l'étage.
void EvDungeon_BeginFloor(void)
{
    ConvertIntToDecimalStringN(gStringVar1, sRun.floor, STR_CONV_MODE_LEFT_ALIGN, 3);
}

static void EndRun(void)
{
    sRun.active = FALSE;
    sRun.pending = 0;
    HealPlayerParty();
}

static void RecordFloor(u32 floor)
{
    if (floor > VarGet(VAR_MOD_EV_DUNGEON_RECORD))
        VarSet(VAR_MOD_EV_DUNGEON_RECORD, floor);
}

// À appeler après le combat (VAR_RESULT : issue du combat).
// VAR_RESULT en sortie : enum EvDungeonFloorResult. gStringVar4 : message à afficher.
void EvDungeon_EndFloor(void)
{
    struct Pokemon *mon = GetBeneficiary();
    enum Stat stat = sPaths[sRun.path].stat;
    u8 *dst = gStringVar4;

    *dst = EOS;
    if (mon == NULL)
    {
        EndRun();
        gSpecialVar_Result = EV_FLOOR_END;
        return;
    }

    if (gSpecialVar_Result != B_OUTCOME_WON)
    {
        u32 kept = EvDungeon_GetDefeatKeptGain(sRun.pending);

        GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
        dst = StringAppend(dst, sText_Defeated);
        dst = StringAppend(dst, gStringVar1);
        dst = StringAppend(dst, sText_Keeps);
        dst = AppendNumber(dst, kept);
        dst = StringAppend(dst, sText_KeepsOf);
        dst = AppendNumber(dst, sRun.pending);
        dst = StringAppend(dst, sText_UnsecuredEv);
        AppendEvPreview(dst, mon, kept);
        EvDungeon_ApplyGain(mon, stat, kept);
        EndRun();
        gSpecialVar_Result = EV_FLOOR_END;
        return;
    }

    u32 gain = EvDungeon_GetFloorGain(sRun.floor);
    sRun.pending = min(sRun.pending + gain, 0xFFFF);
    RecordFloor(sRun.floor);

    if (sRun.floor >= MOD_EVDJ_MAX_FLOOR)
    {
        // Fond du donjon : la visite se termine et sécurise tout.
        dst = StringAppend(dst, sText_Bottom);
        BufferPendingPreview(dst, mon);
        EvDungeon_ApplyGain(mon, stat, sRun.pending);
        EndRun();
        gSpecialVar_Result = EV_FLOOR_END;
        return;
    }

    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    dst = StringAppend(dst, gStringVar1);
    dst = StringAppend(dst, sText_Earned);
    dst = AppendNumber(dst, gain);
    *dst++ = CHAR_SPACE;
    *dst = EOS;
    dst = StringAppend(dst, gStatNamesTable[stat]);
    dst = StringAppend(dst, sText_EvSuffix);
    dst = AppendNumber(dst, sRun.pending);
    dst = StringAppend(dst, sText_EvDot);

    if (EvDungeon_IsRestFloor(sRun.floor))
    {
        *dst++ = CHAR_PROMPT_CLEAR;
        *dst = EOS;
        dst = StringAppend(dst, sText_RestPoint);
        dst = BufferPendingPreview(dst, mon);
        if (sRun.path == EV_PATH_ENDURANCE)
        {
            StringAppend(dst, sText_RestNoHeal);
        }
        else
        {
            HealPlayerParty();
            StringAppend(dst, sText_RestHealed);
        }
        gSpecialVar_Result = EV_FLOOR_REST;
    }
    else
    {
        gSpecialVar_Result = EV_FLOOR_NEXT;
    }
    sRun.floor++;
}

// Sortie à un palier : tous les gains en attente sont confirmés. gStringVar4 : récapitulatif.
void EvDungeon_Leave(void)
{
    struct Pokemon *mon = GetBeneficiary();
    u8 *dst = StringCopy(gStringVar4, sText_Secured);

    if (mon != NULL)
    {
        BufferPendingPreview(dst, mon);
        EvDungeon_ApplyGain(mon, sPaths[sRun.path].stat, sRun.pending);
    }
    EndRun();
}
