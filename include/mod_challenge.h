#ifndef GUARD_MOD_CHALLENGE_H
#define GUARD_MOD_CHALLENGE_H

#include "constants/mod_challenge.h"

// Défis à règles annoncées (wiki sections 13 et 14), proposés après le Panthéon.

extern bool8 gModChallengeActive;

// Calculs (exposés pour les tests)
bool32 ModChallenge_IsPartyAllowed(u32 challenge, struct Pokemon *party);
u16 ModChallenge_GetTrainer(u32 challenge, u32 stage);
enum Item ModChallenge_GetReward(u32 challenge);
bool32 ModChallenge_IsCleared(u32 challenge);
void ModChallenge_Describe(u32 challenge, u8 *dest);

// Combat (battle_special.c)
void ModChallenge_SetupBattle(void);

// Specials
void ModChallenge_PushList(void);
void ModChallenge_BufferRules(void);
void ModChallenge_CheckParty(void);
void ModChallenge_Start(void);
void ModChallenge_AfterBattle(void);
void ModChallenge_Finish(void);

#endif // GUARD_MOD_CHALLENGE_H
