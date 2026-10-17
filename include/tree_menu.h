#ifndef GUARD_TREE_MENU_H
#define GUARD_TREE_MENU_H

// Specials de l'écran BUILD : arbre et variantes d'attaques (data/scripts/mod_tree.inc).
void TreeMenu_Begin(void);
void TreeMenu_PushTypeChoices(void);
void TreeMenu_BufferHeader(void);
void TreeMenu_PushNodes(void);
void TreeMenu_CheckNode(void);
void TreeMenu_Invest(void);
void BuildMenu_Begin(void);
void VariantMenu_PushMoves(void);
void VariantMenu_CheckMove(void);
void VariantMenu_PushVariants(void);
void VariantMenu_BufferVariant(void);
void VariantMenu_Apply(void);

#endif // GUARD_TREE_MENU_H
