#include "global.h"
#include "battle_setup.h"
#include "event_data.h"
#include "pokemon_build.h"
#include "trainer_build.h"
#include "test/test.h"
#include "constants/opponents.h"

#define TRUNK_PHYSICAL 0
#define TRUNK_SPECIAL  1
#define FIRE_ASSAULT    4
#define FIRE_COMBUSTION 8
#define FIRE_SOLAR      12

#define OPPONENT(i) (&gParties[B_TRAINER_OPPONENT_A][i])

static void CreateWithMoves(struct Pokemon *mon, enum Species species, u32 level, enum Move m1, enum Move m2, enum Move m3, enum Move m4)
{
    u32 i;
    enum Move moves[MAX_MON_MOVES] = {m1, m2, m3, m4};
    CreateRandomMonWithIVs(mon, species, level, 0);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, moves[i], i);
}

static void SetBadges(u32 count)
{
    u32 i;
    for (i = 0; i < NUM_BADGES; i++)
    {
        if (i < count)
            FlagSet(FLAG_BADGE01_GET + i);
        else
            FlagClear(FLAG_BADGE01_GET + i);
    }
}

// Les tests n'ont pas les vraies équipes (gTrainers de test) : le rang vient de la classe,
// des tables de revanches et des identifiants des défis.
TEST("Mod: trainer build tiers follow the importance of the trainer")
{
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_YOUNGSTER), TRAINER_BUILD_NONE);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_TEAM_MAGMA), TRAINER_BUILD_NONE);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_MAGMA_ADMIN), TRAINER_BUILD_STANDARD);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_LEADER), TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_RIVAL), TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_AQUA_LEADER), TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_ELITE_FOUR), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerClassBuildTier(TRAINER_CLASS_CHAMPION), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerBuildTier(TRAINER_ROXANNE_2), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerBuildTier(TRAINER_JUAN_5), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerBuildTier(TRAINER_STEVEN), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerBuildTier(TRAINER_CHALLENGE_ELEMENT), TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetTrainerBuildTier(TRAINER_CHALLENGE_ENDURANCE_2), TRAINER_BUILD_ELITE);
}

TEST("Mod: a physical Fire opponent spends all its points, Assault branch first")
{
    struct Pokemon mon;
    CreateWithMoves(&mon, SPECIES_ARCANINE, 40, MOVE_FLARE_BLITZ, MOVE_EXTREME_SPEED, MOVE_CRUNCH, MOVE_ROAR);
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_FIRE);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&mon.box), 40 * MOD_TREE_POINTS_PER_LEVEL);
    EXPECT_GE(GetBoxMonTreeRank(&mon.box, TRUNK_PHYSICAL), MOD_TREE_BRANCH_UNLOCK_POINTS);
    EXPECT_GE(GetBoxMonTreeRank(&mon.box, FIRE_ASSAULT), MOD_TREE_PARENT_RANKS);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, FIRE_ASSAULT + 3), 1); // Maîtrise atteinte.
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, FIRE_COMBUSTION), 0);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, FIRE_SOLAR), 0);
    // Variante Focused sur l'attaque la plus forte (Boutefeu, STAB).
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_FOCUSED);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_PP1), CalculateMonMovePP(&mon, MOVE_FLARE_BLITZ, 0, 0));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HP), GetMonData(&mon, MON_DATA_MAX_HP));
}

TEST("Mod: Fire opponents pick the sun or burn branch from their moves")
{
    struct Pokemon mon;
    CreateWithMoves(&mon, SPECIES_CAMERUPT, 40, MOVE_ERUPTION, MOVE_EARTH_POWER, MOVE_SUNNY_DAY, MOVE_WILL_O_WISP);
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_TREE_TYPE), TYPE_FIRE);
    EXPECT_GE(GetBoxMonTreeRank(&mon.box, TRUNK_SPECIAL), MOD_TREE_BRANCH_UNLOCK_POINTS);
    EXPECT_GE(GetBoxMonTreeRank(&mon.box, FIRE_SOLAR), MOD_TREE_PARENT_RANKS);

    CreateWithMoves(&mon, SPECIES_HOUNDOOM, 40, MOVE_FLAMETHROWER, MOVE_DARK_PULSE, MOVE_WILL_O_WISP, MOVE_NASTY_PLOT);
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_STRONG);
    EXPECT_GE(GetBoxMonTreeRank(&mon.box, FIRE_COMBUSTION), MOD_TREE_PARENT_RANKS);
    EXPECT_EQ(GetBoxMonTreeRank(&mon.box, FIRE_SOLAR), 0);
}

TEST("Mod: opponent variants arrive gradually")
{
    struct Pokemon mon;
    CreateWithMoves(&mon, SPECIES_ARCANINE, MOD_TRAINER_VARIANT_LEVEL - 1, MOVE_FLARE_BLITZ, MOVE_EXTREME_SPEED, MOVE_CRUNCH, MOVE_ROAR);
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_STRONG);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_STANDARD);
    EXPECT_GT(GetBoxMonTreePointsSpent(&mon.box), 0);

    CreateWithMoves(&mon, SPECIES_ARCANINE, 60, MOVE_FLARE_BLITZ, MOVE_EXTREME_SPEED, MOVE_CRUNCH, MOVE_ROAR);
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_STANDARD);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_STANDARD);

    // Rang élite : Focused sur la plus forte, Volatile sur les attaques à effet secondaire.
    ApplyMonTrainerBuild(&mon, TRAINER_BUILD_ELITE);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 0), MOVE_VARIANT_FOCUSED);
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 2), MOVE_VARIANT_VOLATILE); // Mâchouille : baisse de Défense.
    EXPECT_EQ(GetBoxMonMoveVariant(&mon.box, 3), MOVE_VARIANT_STANDARD); // Hurlement : attaque de statut.
}

TEST("Mod: no opponent build before the second badge")
{
    ZeroEnemyPartyMons();
    CreateWithMoves(OPPONENT(0), SPECIES_ARCANINE, 40, MOVE_FLARE_BLITZ, MOVE_EXTREME_SPEED, MOVE_CRUNCH, MOVE_ROAR);
    SetBadges(MOD_TRAINER_BUILD_MIN_BADGES - 1);
    ApplyTrainerPartyBuilds(gParties[B_TRAINER_OPPONENT_A], TRAINER_ROXANNE_2);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&OPPONENT(0)->box), 0);
    SetBadges(MOD_TRAINER_BUILD_MIN_BADGES);
    ApplyTrainerPartyBuilds(gParties[B_TRAINER_OPPONENT_A], TRAINER_ROXANNE_2);
    EXPECT_EQ(GetBoxMonTreePointsSpent(&OPPONENT(0)->box), 40 * MOD_TREE_POINTS_PER_LEVEL);
    SetBadges(0);
}

TEST("Mod: the Champion's Gyarados is mutated to Water/Ground before its tree")
{
    struct Pokemon *gyarados = OPPONENT(1);
    ZeroEnemyPartyMons();
    CreateWithMoves(OPPONENT(0), SPECIES_MILOTIC, 58, MOVE_RECOVER, MOVE_SURF, MOVE_ICE_BEAM, MOVE_TOXIC);
    CreateWithMoves(gyarados, SPECIES_GYARADOS, 56, MOVE_DRAGON_DANCE, MOVE_EARTHQUAKE, MOVE_HYPER_BEAM, MOVE_SURF);
    EXPECT(ApplyTrainerMutation(gParties[B_TRAINER_OPPONENT_A], PARTY_SIZE, TRAINER_WALLACE));
    EXPECT_EQ(GetBoxMonMutatedType(&gyarados->box, SPECIES_GYARADOS, 0), TYPE_WATER);
    EXPECT_EQ(GetBoxMonMutatedType(&gyarados->box, SPECIES_GYARADOS, 1), TYPE_GROUND);
    EXPECT_EQ(GetMonData(OPPONENT(0), MON_DATA_MUTATION_TYPE), TYPE_NONE);
    // Une équipe sans l'espèce prévue n'a pas de mutation.
    EXPECT(!ApplyTrainerMutation(gParties[B_TRAINER_OPPONENT_A], PARTY_SIZE, TRAINER_SIDNEY));
}
