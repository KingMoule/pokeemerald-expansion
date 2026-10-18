#ifndef GUARD_SHIFTING_RUINS_H
#define GUARD_SHIFTING_RUINS_H

#include "constants/shifting_ruins.h"

// Shifting Ruins, donjon rogue-lite à la Lufia 2 (wiki section 11),
// construit sur les étages générés de la Pyramide de Combat.

extern bool8 gShiftingRuinsActive;

bool32 InShiftingRuins(void);
u32 ShiftingRuins_GetFloor(void);

// Calculs (exposés pour les tests)
u32 ShiftingRuins_GetWildLevel(u32 floor);
u32 ShiftingRuins_GetTrainerLevel(u32 floor);
u32 ShiftingRuins_GetTrainerPartySize(u32 floor);
bool32 ShiftingRuins_IsLandingFloor(u32 floor);
u32 ShiftingRuins_GetStartMonCount(u32 record);
u32 ShiftingRuins_GetStartTier(u32 record);
enum Species ShiftingRuins_PickStartSpecies(u32 tier, u32 rand);
enum Species ShiftingRuins_PickSpecies(u32 level, u32 rand);
enum Item ShiftingRuins_RollFloorItem(u32 floor, u32 path, u32 rand);
bool32 ShiftingRuins_IsRelic(enum Item item);
void ShiftingRuins_ResetKeptMon(struct Pokemon *mon);
void ShiftingRuins_CreateTrainerParty(struct Pokemon *party, u32 floor, u32 path);
bool32 ShiftingRuins_IsBossFloor(u32 floor);
u32 ShiftingRuins_GetPathLevelBonus(u32 path);
u32 ShiftingRuins_GetBossLevel(u32 floor);
u32 ShiftingRuins_GetBossPartySize(u32 floor);
void ShiftingRuins_CreateBossParty(struct Pokemon *party, u32 floor, u32 path);
u32 ShiftingRuins_GetRecruitCount(void);
enum Species ShiftingRuins_GetRecruitSpecies(u32 index);
enum Species ShiftingRuins_PickRecruit(u32 mask, u32 rand);
void ShiftingRuins_RollBranches(u8 *branches, u32 floor, u32 rand);

// Branchements dans la Pyramide, les combats et les menus
void ShiftingRuins_GenerateWildMon(void);
void ShiftingRuins_SetupTrainerBattle(void);
void ShiftingRuins_SetupWildBattle(void);
void ShiftingRuins_SetupBossBattle(void);
void ShiftingRuins_PlaceRecruiter(struct ObjectEventTemplate *events, u32 count);
bool32 ShiftingRuins_IsRecruiter(u32 localId);
void ShiftingRuins_SetFloorItem(void);
u16 ShiftingRuins_GetPickupItem(void);
void ShiftingRuins_BufferFloorWindowText(u8 *dest);
void ShiftingRuins_GetMapName(u8 *dest);

// Specials
void ShiftingRuins_CheckAccess(void);
void ShiftingRuins_BufferRecord(void);
void ShiftingRuins_Start(void);
void ShiftingRuins_IsActive(void);
void ShiftingRuins_OnFloorResume(void);
void ShiftingRuins_CheckStairs(void);
void ShiftingRuins_HasBranches(void);
void ShiftingRuins_PushPaths(void);
void ShiftingRuins_DescribePath(u32 path, u8 *dest);
void ShiftingRuins_BufferKeptSummary(struct Pokemon *mon, u8 *dest);
void ShiftingRuins_TakePath(void);
void ShiftingRuins_GoDeeper(void);
void ShiftingRuins_StartBossBattle(void);
void ShiftingRuins_EndBossBattle(void);
void ShiftingRuins_MeetRecruiter(void);
void ShiftingRuins_TakeRelic(void);
void ShiftingRuins_EndRun(void);
void ShiftingRuins_CheckDebrief(void);
void ShiftingRuins_GiveRelics(void);
void ShiftingRuins_ChooseKeptMon(void);
void ShiftingRuins_Finish(void);

#endif // GUARD_SHIFTING_RUINS_H
