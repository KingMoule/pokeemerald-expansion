#include "global.h"
#include "event_data.h"
#include "ev_dungeon.h"
#include "test/overworld_script.h"
#include "test/test.h"
#include "constants/battle.h"

#define MON (&gParties[B_TRAINER_PLAYER][0])

static void SetEv(struct Pokemon *mon, enum Stat stat, u32 value)
{
    SetMonData(mon, MON_DATA_HP_EV + stat, &value);
}

static u32 GetEv(struct Pokemon *mon, enum Stat stat)
{
    return GetMonData(mon, MON_DATA_HP_EV + stat);
}

// Démarre une visite pour le premier Pokémon de l'équipe.
static void StartRun(enum EvDungeonPath path, u32 startFloor)
{
    gSpecialVar_0x8004 = 0;
    EvDungeon_SetBeneficiary();
    gSpecialVar_0x8005 = path;
    gSpecialVar_0x8006 = startFloor;
    EvDungeon_Start();
}

static u32 PlayFloor(u32 outcome)
{
    gSpecialVar_Result = outcome;
    EvDungeon_EndFloor();
    return gSpecialVar_Result;
}

TEST("Mod: Training Depths gains grow at each rest point")
{
    EXPECT_EQ(EvDungeon_GetFloorGain(1), MOD_EVDJ_GAIN_BASE);
    EXPECT_EQ(EvDungeon_GetFloorGain(MOD_EVDJ_REST_EVERY), MOD_EVDJ_GAIN_BASE);
    EXPECT_EQ(EvDungeon_GetFloorGain(MOD_EVDJ_REST_EVERY + 1), MOD_EVDJ_GAIN_BASE + MOD_EVDJ_GAIN_STEP);
    EXPECT_EQ(EvDungeon_GetFloorGain(2 * MOD_EVDJ_REST_EVERY + 1), MOD_EVDJ_GAIN_BASE + 2 * MOD_EVDJ_GAIN_STEP);
}

TEST("Mod: Training Depths opponents grow in number and level")
{
    EXPECT_EQ(EvDungeon_GetEnemyPartySize(1), MOD_EVDJ_PARTY_SIZE_BASE);
    EXPECT_EQ(EvDungeon_GetEnemyPartySize(MOD_EVDJ_REST_EVERY + 1), MOD_EVDJ_PARTY_SIZE_BASE + 1);
    EXPECT_EQ(EvDungeon_GetEnemyPartySize(MOD_EVDJ_MAX_FLOOR), PARTY_SIZE);
    EXPECT_EQ(EvDungeon_GetEnemyLevel(1, 30), 30 - MOD_EVDJ_LEVEL_OFFSET);
    EXPECT_EQ(EvDungeon_GetEnemyLevel(11, 30), 30 - MOD_EVDJ_LEVEL_OFFSET + 10 * MOD_EVDJ_LEVELS_PER_FLOOR);
    EXPECT_EQ(EvDungeon_GetEnemyLevel(1, 2), MIN_LEVEL);
    EXPECT_EQ(EvDungeon_GetEnemyLevel(MOD_EVDJ_MAX_FLOOR, 100), MOD_LEVEL_BASE_CAP);
}

TEST("Mod: Training Depths enemy party matches the floor")
{
    u32 i, floor = MOD_EVDJ_REST_EVERY + 1;
    u32 level = EvDungeon_GetEnemyLevel(floor, 40);

    EvDungeon_CreateEnemyParty(gParties[B_TRAINER_OPPONENT_A], EV_PATH_POWER, floor, 40);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        if (i < EvDungeon_GetEnemyPartySize(floor))
        {
            EXPECT_NE(GetMonData(mon, MON_DATA_SPECIES), SPECIES_NONE);
            EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), level);
            EXPECT_NE(GetMonData(mon, MON_DATA_MOVE1), MOVE_NONE);
        }
        else
        {
            EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_NONE);
        }
    }
}

TEST("Mod: Training Depths gains respect the per-stat and total caps")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );
    SetEv(MON, STAT_ATK, MAX_PER_STAT_EVS - 12);
    EXPECT_EQ(EvDungeon_ClampGain(MON, STAT_ATK, 24), 12);

    SetEv(MON, STAT_ATK, 0);
    SetEv(MON, STAT_HP, MAX_PER_STAT_EVS);
    SetEv(MON, STAT_DEF, MAX_TOTAL_EVS - MAX_PER_STAT_EVS - 6);
    EXPECT_EQ(EvDungeon_ApplyGain(MON, STAT_ATK, 20), 6);
    EXPECT_EQ(GetEv(MON, STAT_ATK), 6);
}

TEST("Mod: leaving the Training Depths at a rest point secures every gain")
{
    u32 i;

    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );
    SetEv(MON, STAT_ATK, 100);
    StartRun(EV_PATH_POWER, 1);
    for (i = 1; i < MOD_EVDJ_REST_EVERY; i++)
        EXPECT_EQ(PlayFloor(B_OUTCOME_WON), EV_FLOOR_NEXT);
    EXPECT_EQ(PlayFloor(B_OUTCOME_WON), EV_FLOOR_REST);
    EXPECT_EQ(GetEv(MON, STAT_ATK), 100); // Rien n'est confirmé avant la sortie.
    EvDungeon_Leave();
    EXPECT_EQ(GetEv(MON, STAT_ATK), 100 + MOD_EVDJ_REST_EVERY * MOD_EVDJ_GAIN_BASE);
    EXPECT_EQ(VarGet(VAR_MOD_EV_DUNGEON_RECORD), MOD_EVDJ_REST_EVERY);
}

TEST("Mod: a defeat in the Training Depths keeps only part of the unsecured gains")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );
    SetEv(MON, STAT_SPEED, 40);
    StartRun(EV_PATH_AGILITY, 1);
    EXPECT_EQ(PlayFloor(B_OUTCOME_WON), EV_FLOOR_NEXT);
    EXPECT_EQ(PlayFloor(B_OUTCOME_WON), EV_FLOOR_NEXT);
    EXPECT_EQ(PlayFloor(B_OUTCOME_LOST), EV_FLOOR_END);
    // Les EV d'avant la visite restent ; seule une partie des gains en attente est gardée.
    EXPECT_EQ(GetEv(MON, STAT_SPEED), 40 + EvDungeon_GetDefeatKeptGain(2 * MOD_EVDJ_GAIN_BASE));
}

TEST("Mod: Training Depths shortcuts open after each rest point reached")
{
    EXPECT_EQ(EvDungeon_GetUnlockedStartFloors(0), 1);
    EXPECT_EQ(EvDungeon_GetUnlockedStartFloors(MOD_EVDJ_REST_EVERY - 1), 1);
    EXPECT_EQ(EvDungeon_GetUnlockedStartFloors(MOD_EVDJ_REST_EVERY), 2);
    EXPECT_EQ(EvDungeon_GetUnlockedStartFloors(3 * MOD_EVDJ_REST_EVERY + 1), 4);
}

TEST("Mod: starting from a shortcut gives the deeper floor's gains")
{
    u32 start = MOD_EVDJ_REST_EVERY + 1;

    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );
    StartRun(EV_PATH_ENDURANCE, start);
    PlayFloor(B_OUTCOME_LOST);
    EXPECT_EQ(GetEv(MON, STAT_HP), EvDungeon_GetDefeatKeptGain(0));
    StartRun(EV_PATH_ENDURANCE, start);
    PlayFloor(B_OUTCOME_WON);
    PlayFloor(B_OUTCOME_WON);
    EXPECT_EQ(PlayFloor(B_OUTCOME_WON), EV_FLOOR_REST);
    EvDungeon_Leave();
    EXPECT_EQ(GetEv(MON, STAT_HP), 3 * EvDungeon_GetFloorGain(start));
}
