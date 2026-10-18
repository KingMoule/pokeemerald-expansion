#ifndef GUARD_TREE_SCREEN_H
#define GUARD_TREE_SCREEN_H

#include "main.h"
#include "pokemon_build.h"

// Écran BUILD : arbre (un onglet par branche, points en attente validés d'un coup),
// attaques et variantes (Technique Codex), mutation volontaire.
enum BuildScreenPage
{
    BUILD_PAGE_TREE,
    BUILD_PAGE_MOVES,
    BUILD_PAGE_MUTATE,
};

void BuildScreen_Open(u32 monSlot, enum BuildScreenPage page, MainCallback returnCallback);
u32 BuildScreen_GetLastMonSlot(void);

// Logique des points en attente, séparée de l'affichage pour les tests.
// pending[n] : points ajoutés au nœud n, pas encore investis.
u32 TreePreview_Rebuild(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending);
bool32 TreePreview_Add(struct BoxPokemon *preview, enum Type treeType, u8 *pending, u32 node);
bool32 TreePreview_Remove(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending, u32 node);
u32 TreePreview_Commit(struct BoxPokemon *boxMon, enum Type treeType, const u8 *pending);

// Calculs affichés par les pages Moves et Mutate.
u32 BuildScreen_GetVariantPower(enum Move move, enum MoveVariant variant);
void BuildScreen_GetMutatedTypes(enum Species species, u32 replacedSlot, enum Type newType, enum Type *types);
u32 BuildScreen_GetWeaknesses(enum Type type1, enum Type type2);

#endif // GUARD_TREE_SCREEN_H
