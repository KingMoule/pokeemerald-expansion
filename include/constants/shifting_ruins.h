#ifndef GUARD_CONSTANTS_SHIFTING_RUINS_H
#define GUARD_CONSTANTS_SHIFTING_RUINS_H

// Fin d'une tentative dans les Shifting Ruins (ShiftingRuins_EndRun, VAR_0x8004).
#define RUINS_END_NONE    0
#define RUINS_END_SECURED 1 // Sortie réussie (palier, Escape Orb ou fond) : les trouvailles sont acquises.
#define RUINS_END_LOST    2 // Équipe K.O. ou abandon : les trouvailles de la tentative sont perdues.

// Retour sur un étage (ShiftingRuins_OnFloorResume, VAR_RESULT).
#define RUINS_RESUME_NONE 0
#define RUINS_RESUME_WON  1 // Combat de dresseur gagné : la lumière s'agrandit, comme dans la Pyramide.
#define RUINS_RESUME_LOST 2 // Combat perdu : fin de la tentative.

// Escalier d'un étage (ShiftingRuins_CheckStairs, VAR_RESULT).
#define RUINS_STAIRS_NEXT    0 // Étage suivant.
#define RUINS_STAIRS_LANDING 1 // Palier : sortir ou continuer.
#define RUINS_STAIRS_BOTTOM  2 // Fond des ruines : sortie forcée et sécurisée.
#define RUINS_STAIRS_BOSS    3 // Un gardien barre l'escalier : combat avant de pouvoir descendre ou sortir.

// Passages proposés par un escalier qui se divise (ShiftingRuins_PushPaths), pour l'étage suivant.
#define RUINS_PATH_PLAIN     0 // Escalier ordinaire.
#define RUINS_PATH_OVERGROWN 1 // Sauvages plus rares et de meilleure qualité (captures).
#define RUINS_PATH_SPRING    2 // Équipe soignée en descendant.
#define RUINS_PATH_PERILOUS  3 // Adversaires plus forts, objets rares plus fréquents.
#define RUINS_PATH_COUNT     4

#endif // GUARD_CONSTANTS_SHIFTING_RUINS_H
