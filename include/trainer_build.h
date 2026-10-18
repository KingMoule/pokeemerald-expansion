#ifndef GUARD_TRAINER_BUILD_H
#define GUARD_TRAINER_BUILD_H

// Builds des adversaires (wiki section 13) : arbres, variantes et mutations
// donnés automatiquement aux équipes des dresseurs importants.

enum TrainerBuildTier
{
    TRAINER_BUILD_NONE,     // Dresseurs ordinaires : aucun build.
    TRAINER_BUILD_STANDARD, // Sbires importants : arbre seulement.
    TRAINER_BUILD_STRONG,   // Champions d'arène, rival, chefs d'équipe : arbre et une variante.
    TRAINER_BUILD_ELITE,    // Conseil 4, Maître, revanches des champions, défis : build complet et mutations.
};

enum TrainerBuildTier GetTrainerClassBuildTier(enum TrainerClassID trainerClass);
enum TrainerBuildTier GetTrainerBuildTier(u16 trainerId);
void ApplyMonTrainerBuild(struct Pokemon *mon, enum TrainerBuildTier tier);
bool32 ApplyTrainerMutation(struct Pokemon *party, u32 partySize, u16 trainerId);
void ApplyTrainerPartyBuilds(struct Pokemon *party, u16 trainerId);
u32 GetTrainerMutationCount(void);
u16 GetTrainerMutationTrainerId(u32 index);

#endif // GUARD_TRAINER_BUILD_H
