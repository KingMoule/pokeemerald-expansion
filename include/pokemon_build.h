#ifndef GUARD_POKEMON_BUILD_H
#define GUARD_POKEMON_BUILD_H

// Accès aux données du mod stockées dans chaque Pokémon (struct PokemonBuild).

u32 GetBoxMonTreeRank(const struct BoxPokemon *boxMon, u32 node);
void SetBoxMonTreeRank(struct BoxPokemon *boxMon, u32 node, u32 rank);
u32 GetBoxMonTreePointsSpent(const struct BoxPokemon *boxMon);
u32 GetBoxMonMoveVariant(const struct BoxPokemon *boxMon, u32 moveSlot);
void SetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 moveSlot, u32 variant);

// Mutations : types effectifs et type portant la pénalité sur les faiblesses.
enum Type GetBoxMonMutatedType(const struct BoxPokemon *boxMon, enum Species species, u32 slot);
enum Type GetBoxMonMutationPenaltyType(const struct BoxPokemon *boxMon, enum Species species);
enum Type GetBattlerMutatedType(enum BattlerId battler, u32 slot);
uq4_12_t ApplyMutationWeaknessPenalty(enum BattlerId battlerDef, enum Type moveType, uq4_12_t modifier);

#endif // GUARD_POKEMON_BUILD_H
