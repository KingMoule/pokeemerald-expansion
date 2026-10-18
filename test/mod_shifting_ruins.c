#include "global.h"
#include "event_data.h"
#include "item.h"
#include "pokemon_build.h"
#include "shifting_ruins.h"
#include "test/overworld_script.h"
#include "test/test.h"
#include "constants/battle.h"
#include "constants/items.h"

#define MON(i) (&gParties[B_TRAINER_PLAYER][i])

// Termine une tentative comme le script de retour à l'entrée, en ramenant le Pokémon keptSlot.
static void EndRun(u32 end, u32 keptSlot)
{
    gSpecialVar_0x8004 = end;
    ShiftingRuins_EndRun();
    if (end == RUINS_END_SECURED)
    {
        ShiftingRuins_GiveRelics();
        gSpecialVar_0x8004 = keptSlot;
        ShiftingRuins_ChooseKeptMon();
    }
    ShiftingRuins_Finish();
}

static void AddRelic(enum Item item)
{
    gSpecialVar_0x8000 = item;
    ShiftingRuins_TakeRelic();
}

TEST("Mod: Shifting Ruins difficulty grows with depth")
{
    EXPECT_EQ(ShiftingRuins_GetWildLevel(1), MOD_RUINS_LEVEL_BASE);
    EXPECT_EQ(ShiftingRuins_GetWildLevel(11), MOD_RUINS_LEVEL_BASE + 10 * MOD_RUINS_LEVELS_PER_FLOOR);
    EXPECT_EQ(ShiftingRuins_GetWildLevel(255), MAX_LEVEL);
    EXPECT_EQ(ShiftingRuins_GetTrainerLevel(1), MOD_RUINS_LEVEL_BASE + MOD_RUINS_TRAINER_LEVEL_BONUS);
    EXPECT_EQ(ShiftingRuins_GetTrainerPartySize(1), MOD_RUINS_TRAINER_MONS_BASE);
    EXPECT_EQ(ShiftingRuins_GetTrainerPartySize(MOD_RUINS_LANDING_EVERY), MOD_RUINS_TRAINER_MONS_BASE);
    EXPECT_EQ(ShiftingRuins_GetTrainerPartySize(MOD_RUINS_LANDING_EVERY + 1), MOD_RUINS_TRAINER_MONS_BASE + 1);
    EXPECT_EQ(ShiftingRuins_GetTrainerPartySize(MOD_RUINS_MAX_FLOOR), MOD_RUINS_TRAINER_MONS_MAX);
}

TEST("Mod: Shifting Ruins landings come every few floors")
{
    EXPECT(!ShiftingRuins_IsLandingFloor(1));
    EXPECT(!ShiftingRuins_IsLandingFloor(MOD_RUINS_LANDING_EVERY - 1));
    EXPECT(ShiftingRuins_IsLandingFloor(MOD_RUINS_LANDING_EVERY));
    EXPECT(ShiftingRuins_IsLandingFloor(2 * MOD_RUINS_LANDING_EVERY));
}

TEST("Mod: Shifting Ruins records unlock bigger and stronger starts")
{
    EXPECT_EQ(ShiftingRuins_GetStartMonCount(0), 1);
    EXPECT_EQ(ShiftingRuins_GetStartMonCount(MOD_RUINS_EXTRA_MON_EVERY - 1), 1);
    EXPECT_EQ(ShiftingRuins_GetStartMonCount(MOD_RUINS_EXTRA_MON_EVERY), 2);
    EXPECT_EQ(ShiftingRuins_GetStartMonCount(255), MOD_RUINS_MAX_START_MONS);
    EXPECT_EQ(ShiftingRuins_GetStartTier(0), 0);
    EXPECT_EQ(ShiftingRuins_GetStartTier(MOD_RUINS_TIER_EVERY), 1);
    EXPECT_EQ(ShiftingRuins_GetStartTier(255), 2);
}

TEST("Mod: Shifting Ruins have Pokemon for every level")
{
    u32 level, r;

    for (level = 1; level <= MAX_LEVEL; level++)
    {
        for (r = 0; r < 8; r++)
            EXPECT_NE(ShiftingRuins_PickSpecies(level, r * 7919), SPECIES_NONE);
    }
    // Une forme évoluée n'apparaît pas trop tôt.
    for (r = 0; r < 200; r++)
        EXPECT_NE(ShiftingRuins_PickSpecies(5, r), SPECIES_GARDEVOIR);
}

TEST("Mod: Shifting Ruins rare finds and escape orbs on the floor")
{
    EXPECT_EQ(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PLAIN, 0), ITEM_TECHNIQUE_CODEX);
    EXPECT(ShiftingRuins_IsRelic(ITEM_TECHNIQUE_CODEX));
    EXPECT(ShiftingRuins_IsRelic(ITEM_LINEAGE_CHARM));
    EXPECT(!ShiftingRuins_IsRelic(ITEM_POTION));
    EXPECT(!ShiftingRuins_IsRelic(ITEM_ESCAPE_ORB));
    // Le Lineage Charm n'apparaît qu'après le premier palier.
    EXPECT_NE(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE), ITEM_LINEAGE_CHARM);
    EXPECT_EQ(ShiftingRuins_RollFloorItem(MOD_RUINS_LANDING_EVERY + 1, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE), ITEM_LINEAGE_CHARM);
    EXPECT_EQ(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE), ITEM_ESCAPE_ORB);
    EXPECT(!ShiftingRuins_IsRelic(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PLAIN, 99)));
}

TEST("Mod: Shifting Ruins trainers field a party that matches the floor")
{
    u32 i, floor = MOD_RUINS_LANDING_EVERY + 1;

    ShiftingRuins_CreateTrainerParty(gParties[B_TRAINER_OPPONENT_A], floor, RUINS_PATH_PLAIN);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        if (i < ShiftingRuins_GetTrainerPartySize(floor))
        {
            EXPECT_NE(GetMonData(mon, MON_DATA_SPECIES), SPECIES_NONE);
            EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), ShiftingRuins_GetTrainerLevel(floor));
            EXPECT_NE(GetMonData(mon, MON_DATA_MOVE1), MOVE_NONE);
        }
        else
        {
            EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), SPECIES_NONE);
        }
    }
}

TEST("Mod: a Pokemon brought back from the Shifting Ruins is rebuilt at level 5")
{
    u32 ev = 100;
    enum Item item = ITEM_LEFTOVERS;

    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_CHARMELEON, 40; );
    SetMonData(MON(0), MON_DATA_ATK_EV, &ev);
    SetMonData(MON(0), MON_DATA_HELD_ITEM, &item);
    SetBoxMonTreeRank(&MON(0)->box, 0, 3);
    SetBoxMonMoveVariant(&MON(0)->box, 0, MOVE_VARIANT_FOCUSED);
    EXPECT(SetBoxMonMutation(&MON(0)->box, TYPE_FIRE, TYPE_WATER));

    ShiftingRuins_ResetKeptMon(MON(0));

    EXPECT_EQ(GetMonData(MON(0), MON_DATA_SPECIES), SPECIES_CHARMELEON);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_LEVEL), MOD_RUINS_START_LEVEL);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_ATK_EV), 0);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_HELD_ITEM), ITEM_NONE);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&MON(0)->box), 0);
    EXPECT_EQ(GetBoxMonMoveVariant(&MON(0)->box, 0), MOVE_VARIANT_STANDARD);
    EXPECT_NE(GetMonData(MON(0), MON_DATA_MOVE1), MOVE_NONE);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_HP), GetMonData(MON(0), MON_DATA_MAX_HP));
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_MUTATION_TYPE), TYPE_WATER);
}

TEST("Mod: entering the Shifting Ruins swaps in a level 5 team and restores the campaign after")
{
    u32 level = 0;

    ZeroPlayerPartyMons();
    VarSet(VAR_MOD_RUINS_RECORD, 0);
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; givemon SPECIES_RALTS, 25; );
    gSaveBlock2Ptr->frontier.pyramidBag.itemId[FRONTIER_LVL_50][0] = ITEM_HYPER_POTION;
    gSaveBlock2Ptr->frontier.pyramidBag.quantity[FRONTIER_LVL_50][0] = 1;
    gSaveBlock2Ptr->frontier.lvlMode = FRONTIER_LVL_OPEN;

    ShiftingRuins_Start();
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
    level = GetMonData(MON(0), MON_DATA_LEVEL);
    EXPECT_EQ(level, MOD_RUINS_START_LEVEL);
    EXPECT_EQ(ShiftingRuins_GetFloor(), 1);
    EXPECT_EQ(gSaveBlock2Ptr->frontier.pyramidBag.itemId[FRONTIER_LVL_50][0], ITEM_POKE_BALL);

    EndRun(RUINS_END_LOST, 0);
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_SPECIES), SPECIES_MACHOP);
    EXPECT_EQ(GetMonData(MON(1), MON_DATA_LEVEL), 25);
    EXPECT_EQ(gSaveBlock2Ptr->frontier.pyramidBag.itemId[FRONTIER_LVL_50][0], ITEM_HYPER_POTION);
    EXPECT_EQ((u32)gSaveBlock2Ptr->frontier.lvlMode, FRONTIER_LVL_OPEN);
    EXPECT(!gShiftingRuinsActive);
}

TEST("Mod: getting out of the Shifting Ruins secures finds, one Pokemon and the record")
{
    u32 floor;
    enum Species ruinsSpecies;

    ZeroPlayerPartyMons();
    VarSet(VAR_MOD_RUINS_RECORD, 0);
    RemoveBagItem(ITEM_TECHNIQUE_CODEX, CountTotalItemQuantityInBag(ITEM_TECHNIQUE_CODEX));
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );

    ShiftingRuins_Start();
    ruinsSpecies = GetMonData(MON(0), MON_DATA_SPECIES);
    for (floor = 1; floor < MOD_RUINS_EXTRA_MON_EVERY; floor++)
        ShiftingRuins_GoDeeper();
    EXPECT_EQ(ShiftingRuins_GetFloor(), MOD_RUINS_EXTRA_MON_EVERY);
    AddRelic(ITEM_TECHNIQUE_CODEX);
    AddRelic(ITEM_TECHNIQUE_CODEX);

    EndRun(RUINS_END_SECURED, 0);
    EXPECT_EQ(gSpecialVar_Result, MON_GIVEN_TO_PARTY);
    EXPECT(gSpecialVar_0x8005);
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    EXPECT_EQ(GetMonData(MON(0), MON_DATA_SPECIES), SPECIES_MACHOP);
    EXPECT_EQ(GetMonData(MON(1), MON_DATA_SPECIES), ruinsSpecies);
    EXPECT_EQ(GetMonData(MON(1), MON_DATA_LEVEL), MOD_RUINS_START_LEVEL);
    EXPECT(CheckBagHasItem(ITEM_TECHNIQUE_CODEX, 2));
    EXPECT_EQ(VarGet(VAR_MOD_RUINS_RECORD), MOD_RUINS_EXTRA_MON_EVERY);

    // La tentative suivante part avec un Pokémon de plus.
    ShiftingRuins_Start();
    EXPECT_EQ(CalculatePlayerPartyCount(), 2);
    EndRun(RUINS_END_LOST, 0);
}

TEST("Mod: falling in the Shifting Ruins loses the run's rare finds")
{
    ZeroPlayerPartyMons();
    VarSet(VAR_MOD_RUINS_RECORD, 0);
    RemoveBagItem(ITEM_LINEAGE_CHARM, CountTotalItemQuantityInBag(ITEM_LINEAGE_CHARM));
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );

    ShiftingRuins_Start();
    ShiftingRuins_GoDeeper();
    AddRelic(ITEM_LINEAGE_CHARM);
    EndRun(RUINS_END_LOST, 0);

    EXPECT_EQ(gSpecialVar_Result, PARTY_SIZE);
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
    EXPECT(!CheckBagHasItem(ITEM_LINEAGE_CHARM, 1));
    EXPECT_EQ(VarGet(VAR_MOD_RUINS_RECORD), 0);
}

TEST("Mod: Shifting Ruins guardians bar the stairs every few floors")
{
    u32 i, floor = MOD_RUINS_BOSS_EVERY;

    EXPECT(!ShiftingRuins_IsBossFloor(1));
    EXPECT(ShiftingRuins_IsBossFloor(MOD_RUINS_BOSS_EVERY));
    EXPECT_EQ(ShiftingRuins_GetBossLevel(floor), ShiftingRuins_GetTrainerLevel(floor) + MOD_RUINS_BOSS_LEVEL_BONUS);

    ShiftingRuins_CreateBossParty(gParties[B_TRAINER_OPPONENT_A], floor, RUINS_PATH_PLAIN);
    for (i = 0; i < ShiftingRuins_GetBossPartySize(floor); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        EXPECT_NE(GetMonData(mon, MON_DATA_SPECIES), SPECIES_NONE);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), ShiftingRuins_GetBossLevel(floor));
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP_IV), MAX_PER_STAT_IVS);
        EXPECT_GT(GetBoxMonTreePointsSpent(&mon->box), 0);
    }
    EXPECT_GT(ShiftingRuins_GetBossPartySize(floor), ShiftingRuins_GetTrainerPartySize(floor));
}

TEST("Mod: Shifting Ruins stairs split into two different passages")
{
    u32 rand;
    u8 branches[2];

    for (rand = 0; rand < 400; rand++)
    {
        ShiftingRuins_RollBranches(branches, 1, rand);
        if (rand % 100 < MOD_RUINS_BRANCH_CHANCE)
        {
            EXPECT_NE(branches[0], RUINS_PATH_PLAIN);
            EXPECT_NE(branches[1], RUINS_PATH_PLAIN);
            EXPECT_NE(branches[0], branches[1]);
            EXPECT_LT(branches[0], RUINS_PATH_COUNT);
            EXPECT_LT(branches[1], RUINS_PATH_COUNT);
        }
        else
        {
            EXPECT_EQ(branches[0], RUINS_PATH_PLAIN);
        }
        // Ni sur un palier ni devant un gardien.
        ShiftingRuins_RollBranches(branches, MOD_RUINS_LANDING_EVERY, rand);
        EXPECT_EQ(branches[0], RUINS_PATH_PLAIN);
        ShiftingRuins_RollBranches(branches, MOD_RUINS_BOSS_EVERY, rand);
        EXPECT_EQ(branches[0], RUINS_PATH_PLAIN);
    }
}

TEST("Mod: the Perilous passage is tougher and richer, and Mutation Seals lie deep")
{
    u32 deep = 2 * MOD_RUINS_LANDING_EVERY + 1;

    EXPECT_EQ(ShiftingRuins_GetPathLevelBonus(RUINS_PATH_PLAIN), 0);
    EXPECT_EQ(ShiftingRuins_GetPathLevelBonus(RUINS_PATH_PERILOUS), MOD_RUINS_PERILOUS_LEVELS);
    EXPECT_NE(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE), ITEM_TECHNIQUE_CODEX);
    EXPECT_EQ(ShiftingRuins_RollFloorItem(1, RUINS_PATH_PERILOUS, MOD_RUINS_RELIC_CHANCE), ITEM_TECHNIQUE_CODEX);
    EXPECT(ShiftingRuins_IsRelic(ITEM_MUTATION_SEAL));
    EXPECT_NE(ShiftingRuins_RollFloorItem(MOD_RUINS_LANDING_EVERY + 1, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE + MOD_RUINS_RARE_RELIC_CHANCE), ITEM_MUTATION_SEAL);
    EXPECT_EQ(ShiftingRuins_RollFloorItem(deep, RUINS_PATH_PLAIN, MOD_RUINS_RELIC_CHANCE + MOD_RUINS_RARE_RELIC_CHANCE), ITEM_MUTATION_SEAL);
}

TEST("Mod: a scout met in the Shifting Ruins adds a species to later starts")
{
    u32 i;
    enum Species recruit = SPECIES_NONE;

    ZeroPlayerPartyMons();
    VarSet(VAR_MOD_RUINS_RECORD, 0);
    VarSet(VAR_MOD_RUINS_RECRUITS, 0);
    EXPECT_EQ(ShiftingRuins_PickRecruit(0, 5), SPECIES_NONE);
    EXPECT_EQ(ShiftingRuins_PickRecruit(1 << 3, 5), ShiftingRuins_GetRecruitSpecies(3));
    RUN_OVERWORLD_SCRIPT( givemon SPECIES_MACHOP, 30; );

    // Un éclaireur rencontré pendant une tentative perdue ne compte pas.
    ShiftingRuins_Start();
    ShiftingRuins_MeetRecruiter();
    EndRun(RUINS_END_LOST, 0);
    EXPECT_EQ(VarGet(VAR_MOD_RUINS_RECRUITS), 0);

    ShiftingRuins_Start();
    ShiftingRuins_MeetRecruiter();
    EXPECT(gSpecialVar_Result);
    ShiftingRuins_MeetRecruiter();
    EXPECT(!gSpecialVar_Result);
    EndRun(RUINS_END_SECURED, PARTY_SIZE);
    EXPECT(gSpecialVar_0x8006);
    for (i = 0; i < ShiftingRuins_GetRecruitCount(); i++)
    {
        if ((VarGet(VAR_MOD_RUINS_RECRUITS) >> i) & 1)
        {
            EXPECT_EQ(recruit, SPECIES_NONE);
            recruit = ShiftingRuins_GetRecruitSpecies(i);
        }
    }
    EXPECT_NE(recruit, SPECIES_NONE);
    VarSet(VAR_MOD_RUINS_RECRUITS, 0);
}
