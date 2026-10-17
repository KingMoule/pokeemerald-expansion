#include "global.h"
#include "battle_setup.h"
#include "event_data.h"
#include "mod_challenge.h"
#include "trainer_build.h"
#include "test/test.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/opponents.h"

#define MON(i) (&gParties[B_TRAINER_PLAYER][i])

static void SetParty(const enum Species *species, u32 count, u32 level)
{
    u32 i;
    ZeroPlayerPartyMons();
    for (i = 0; i < count; i++)
        CreateRandomMonWithIVs(MON(i), species[i], level, 0);
}

static u32 RunChallenge(u32 challenge, bool32 winAll)
{
    u32 result;
    gSpecialVar_0x8005 = challenge;
    ModChallenge_Start();
    do
    {
        gSpecialVar_Result = winAll ? B_OUTCOME_WON : B_OUTCOME_LOST;
        ModChallenge_AfterBattle();
        result = gSpecialVar_Result;
    } while (result == CHALLENGE_NEXT_BATTLE);
    gSpecialVar_0x8004 = (result == CHALLENGE_WON);
    ModChallenge_Finish();
    return gSpecialVar_Result;
}

TEST("Mod: Element Trial needs a type shared by the whole team")
{
    static const enum Species fire[] = {SPECIES_CHARMANDER, SPECIES_PONYTA, SPECIES_MAGCARGO};
    static const enum Species mixed[] = {SPECIES_CHARMANDER, SPECIES_SQUIRTLE};
    SetParty(fire, ARRAY_COUNT(fire), 50);
    EXPECT(ModChallenge_IsPartyAllowed(CHALLENGE_ELEMENT, gParties[B_TRAINER_PLAYER]));
    SetParty(mixed, ARRAY_COUNT(mixed), 50);
    EXPECT(!ModChallenge_IsPartyAllowed(CHALLENGE_ELEMENT, gParties[B_TRAINER_PLAYER]));
}

TEST("Mod: Variety Trial needs six Pokémon with no type in common")
{
    static const enum Species varied[] = {SPECIES_CHARMANDER, SPECIES_SQUIRTLE, SPECIES_BULBASAUR, SPECIES_PIKACHU, SPECIES_MACHOP, SPECIES_ABRA};
    static const enum Species overlap[] = {SPECIES_CHARMANDER, SPECIES_SQUIRTLE, SPECIES_BULBASAUR, SPECIES_PIKACHU, SPECIES_MACHOP, SPECIES_PONYTA};
    SetParty(varied, ARRAY_COUNT(varied), 50);
    EXPECT(ModChallenge_IsPartyAllowed(CHALLENGE_VARIETY, gParties[B_TRAINER_PLAYER]));
    SetParty(overlap, ARRAY_COUNT(overlap), 50);
    EXPECT(!ModChallenge_IsPartyAllowed(CHALLENGE_VARIETY, gParties[B_TRAINER_PLAYER]));
    SetParty(varied, ARRAY_COUNT(varied) - 1, 50);
    EXPECT(!ModChallenge_IsPartyAllowed(CHALLENGE_VARIETY, gParties[B_TRAINER_PLAYER]));
}

TEST("Mod: Rising Trial caps levels and Double Trial needs two fighters")
{
    static const enum Species two[] = {SPECIES_CHARMANDER, SPECIES_SQUIRTLE};
    u32 zero = 0;
    SetParty(two, ARRAY_COUNT(two), MOD_CHALLENGE_RISING_MAX_LEVEL);
    EXPECT(ModChallenge_IsPartyAllowed(CHALLENGE_RISING, gParties[B_TRAINER_PLAYER]));
    EXPECT(ModChallenge_IsPartyAllowed(CHALLENGE_DOUBLE, gParties[B_TRAINER_PLAYER]));
    SetMonData(MON(1), MON_DATA_HP, &zero);
    EXPECT(!ModChallenge_IsPartyAllowed(CHALLENGE_DOUBLE, gParties[B_TRAINER_PLAYER]));
    SetParty(two, ARRAY_COUNT(two), MOD_CHALLENGE_RISING_MAX_LEVEL + 1);
    EXPECT(!ModChallenge_IsPartyAllowed(CHALLENGE_RISING, gParties[B_TRAINER_PLAYER]));
}

TEST("Mod: a Trial rewards only its first clear")
{
    static const enum Species one[] = {SPECIES_CHARMANDER};
    SetParty(one, 1, 50);
    VarSet(VAR_MOD_CHALLENGES_CLEARED, 0);
    EXPECT_EQ(RunChallenge(CHALLENGE_ELEMENT, FALSE), ITEM_NONE);
    EXPECT(!ModChallenge_IsCleared(CHALLENGE_ELEMENT));
    EXPECT_EQ(RunChallenge(CHALLENGE_ELEMENT, TRUE), ModChallenge_GetReward(CHALLENGE_ELEMENT));
    EXPECT(ModChallenge_IsCleared(CHALLENGE_ELEMENT));
    EXPECT_EQ(RunChallenge(CHALLENGE_ELEMENT, TRUE), ITEM_NONE);
    EXPECT(!gModChallengeActive);
    VarSet(VAR_MOD_CHALLENGES_CLEARED, 0);
}

TEST("Mod: Endurance Trial chains two battles and forbids the bag")
{
    gSpecialVar_0x8005 = CHALLENGE_ENDURANCE;
    ModChallenge_Start();
    EXPECT_EQ(VarGet(VAR_MOD_NO_BAG), NO_BAG_IN_BATTLE);
    gSpecialVar_Result = B_OUTCOME_WON;
    ModChallenge_AfterBattle();
    EXPECT_EQ(gSpecialVar_Result, CHALLENGE_NEXT_BATTLE);
    gSpecialVar_Result = B_OUTCOME_WON;
    ModChallenge_AfterBattle();
    EXPECT_EQ(gSpecialVar_Result, CHALLENGE_WON);
    gSpecialVar_0x8004 = TRUE;
    ModChallenge_Finish();
    EXPECT_EQ(VarGet(VAR_MOD_NO_BAG), NO_BAG_RESTRICTION);
    VarSet(VAR_MOD_CHALLENGES_CLEARED, 0);
}

TEST("Mod: every Trial opponent has an elite build")
{
    u32 challenge, stage;
    for (challenge = 0; challenge < CHALLENGE_COUNT; challenge++)
    {
        for (stage = 0; stage < 2; stage++)
        {
            u16 trainerId = ModChallenge_GetTrainer(challenge, stage);
            if (trainerId != TRAINER_NONE)
                EXPECT_EQ(GetTrainerBuildTier(trainerId), TRAINER_BUILD_ELITE);
        }
    }
}
