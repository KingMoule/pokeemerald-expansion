#ifndef GUARD_POKEMON_BUILD_H
#define GUARD_POKEMON_BUILD_H

// Accès aux données du mod stockées dans chaque Pokémon (struct PokemonBuild).

u32 GetBoxMonTreeRank(const struct BoxPokemon *boxMon, u32 node);
void SetBoxMonTreeRank(struct BoxPokemon *boxMon, u32 node, u32 rank);
u32 GetBoxMonTreePointsSpent(const struct BoxPokemon *boxMon);
u32 GetBoxMonMoveVariant(const struct BoxPokemon *boxMon, u32 moveSlot);
void SetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 moveSlot, u32 variant);

#endif // GUARD_POKEMON_BUILD_H
