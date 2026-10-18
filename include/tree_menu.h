#ifndef GUARD_TREE_MENU_H
#define GUARD_TREE_MENU_H

// Logique de l'écran BUILD (src/tree_screen.c) : arbre, variantes d'attaques et mutation.
// VAR_0x8004 : emplacement du Pokémon ; voir src/tree_menu.c pour les autres variables.
void TreeMenu_Begin(void);
void TreeMenu_CheckNode(void);
void TreeMenu_Invest(void);
void BuildMenu_Begin(void);
void VariantMenu_CheckMove(void);
void VariantMenu_Apply(void);
void MutationMenu_Begin(void);
void MutationMenu_Check(void);
void MutationMenu_Apply(void);

#endif // GUARD_TREE_MENU_H
