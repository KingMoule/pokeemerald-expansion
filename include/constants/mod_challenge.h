#ifndef GUARD_CONSTANTS_MOD_CHALLENGE_H
#define GUARD_CONSTANTS_MOD_CHALLENGE_H

// Défis à règles annoncées (mod, wiki sections 13 et 14).
#define CHALLENGE_ELEMENT    0 // Tous les Pokémon partagent un type.
#define CHALLENGE_DOUBLE     1 // Combat double.
#define CHALLENGE_VARIETY    2 // Six Pokémon sans type en commun.
#define CHALLENGE_RISING     3 // Pokémon de niveau limité.
#define CHALLENGE_ENDURANCE  4 // Deux combats d'affilée, sans soin ni sac.
#define CHALLENGE_COUNT      5

// Résultat de ModChallenge_AfterBattle (VAR_RESULT).
#define CHALLENGE_NEXT_BATTLE 0
#define CHALLENGE_WON         1
#define CHALLENGE_LOST        2

#endif // GUARD_CONSTANTS_MOD_CHALLENGE_H
