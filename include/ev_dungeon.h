#ifndef GUARD_EV_DUNGEON_H
#define GUARD_EV_DUNGEON_H

#include "constants/ev_dungeon.h"

// Donjon d'entraînement des EV, « Training Depths » (wiki section 10).

enum EvDungeonPath
{
    EV_PATH_POWER,      // Attaque
    EV_PATH_FOCUS,      // Attaque spéciale
    EV_PATH_ENDURANCE,  // PV
    EV_PATH_RESISTANCE, // Défense
    EV_PATH_DISCIPLINE, // Défense spéciale
    EV_PATH_AGILITY,    // Vitesse
    EV_PATH_COUNT,
};

// Calculs (exposés pour les tests)
enum Stat EvDungeon_GetPathStat(enum EvDungeonPath path);
u32 EvDungeon_GetFloorGain(u32 floor);
u32 EvDungeon_GetEnemyPartySize(u32 floor);
u32 EvDungeon_GetEnemyLevel(u32 floor, u32 partyMaxLevel);
u32 EvDungeon_GetDefeatKeptGain(u32 pending);
u32 EvDungeon_ClampGain(struct Pokemon *mon, enum Stat stat, u32 gain);
u32 EvDungeon_ApplyGain(struct Pokemon *mon, enum Stat stat, u32 gain);
bool32 EvDungeon_IsRestFloor(u32 floor);
u32 EvDungeon_GetUnlockedStartFloors(u32 record);
void EvDungeon_CreateEnemyParty(struct Pokemon *party, enum EvDungeonPath path, u32 floor, u32 partyMaxLevel);

// Combat (appelé par DoSpecialTrainerBattle)
void EvDungeon_SetupBattle(void);

// Specials
void EvDungeon_CheckAccess(void);
void EvDungeon_SetBeneficiary(void);
void EvDungeon_PushPaths(void);
void EvDungeon_PushStartFloors(void);
void EvDungeon_Start(void);
void EvDungeon_BeginFloor(void);
void EvDungeon_EndFloor(void);
void EvDungeon_Leave(void);

#endif // GUARD_EV_DUNGEON_H
