#ifndef GUARD_CONSTANTS_EV_DUNGEON_H
#define GUARD_CONSTANTS_EV_DUNGEON_H

// Résultat d'un étage des Training Depths, renvoyé dans VAR_RESULT par EvDungeon_EndFloor.
#define EV_FLOOR_NEXT 0 // Étage suivant, sans possibilité de sortir.
#define EV_FLOOR_REST 1 // Palier de repos : continuer ou sortir.
#define EV_FLOOR_END  2 // Défaite ou fond du donjon : la visite est terminée.

#endif // GUARD_CONSTANTS_EV_DUNGEON_H
