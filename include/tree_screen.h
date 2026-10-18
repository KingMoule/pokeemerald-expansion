#ifndef GUARD_TREE_SCREEN_H
#define GUARD_TREE_SCREEN_H

// Écran plein du skill tree (BUILD > Tree) : un onglet par branche, points en attente
// validés d'un coup. VAR_0x8004 : emplacement du Pokémon ; VAR_0x8005 : élément de l'arbre.
void TreeScreen_Open(void);

// Logique des points en attente, séparée de l'affichage pour les tests.
// pending[n] : points ajoutés au nœud n, pas encore investis.
u32 TreePreview_Rebuild(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending);
bool32 TreePreview_Add(struct BoxPokemon *preview, enum Type treeType, u8 *pending, u32 node);
bool32 TreePreview_Remove(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending, u32 node);
u32 TreePreview_Commit(struct BoxPokemon *boxMon, enum Type treeType, const u8 *pending);

#endif // GUARD_TREE_SCREEN_H
