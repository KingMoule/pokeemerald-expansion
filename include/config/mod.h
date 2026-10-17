#ifndef GUARD_CONFIG_MOD_H
#define GUARD_CONFIG_MOD_H

// Valeurs provisoires du mod (wiki v0.2). Tous les chiffres d'équilibrage
// sont regroupés ici pour être modifiés sans toucher au reste du code.

// Arbres élémentaires
#define MOD_TREE_NODE_COUNT           16   // Nœuds stockés par Pokémon : 4 du tronc + 3 branches de 4 (maquette Feu).
#define MOD_TREE_RANK_BITS            5    // Bits par nœud : rang maximal stockable de 31.
#define MOD_TREE_POINT_BUDGET         100  // Budget total de points d'arbre (illustratif).
#define MOD_TREE_BRANCH_UNLOCK_POINTS 15   // Points dans le tronc pour ouvrir les branches.
#define MOD_TREE_PARENT_RANKS         5    // Rangs requis dans le nœud parent pour ouvrir le suivant.
#define MOD_TREE_POINTS_PER_LEVEL     1    // Points d'arbre gagnés par niveau (provisoire, question ouverte du wiki).

// EV
#define MOD_BATTLE_EV_GAINS           FALSE // Les combats normaux ne donnent plus d'EV (donjon d'entraînement à la place).
#define MOD_EV_CAP_PER_STAT           252   // Plafond par statistique (exemple du wiki, 255 au plus).
#define MOD_EV_CAP_TOTAL              510   // Plafond total (exemple du wiki).

// Mutations
#define MOD_MUTATION_TREE_POINTS      50   // Points investis requis pour acquérir soi-même une mutation.
#define MOD_MUTATION_WEAKNESS_NUM     125  // Pénalité sur les faiblesses : NUM / DEN, soit ×1,25.
#define MOD_MUTATION_WEAKNESS_DEN     100

// Variantes d'attaques
#define MOD_MOVE_VARIANT_BITS         2    // Bits par attaque : 0 = standard, 1 à 3 = variantes.
// Valeurs en % de la valeur normale de l'attaque (wiki : avantage et contrepartie, chiffres à fixer).
#define MOD_VARIANT_FOCUSED_POWER     120  // Concentrée : plus de puissance...
#define MOD_VARIANT_FOCUSED_PP        60   // ...moins de PP.
#define MOD_VARIANT_THRIFTY_POWER     85   // Économe : moins de puissance...
#define MOD_VARIANT_THRIFTY_PP        150  // ...plus de PP.
#define MOD_VARIANT_VOLATILE_POWER    85   // Incendiaire, généralisée : moins de puissance...
#define MOD_VARIANT_VOLATILE_CHANCE   200  // ...plus de chances d'effet secondaire (brûlure, gel, recul...).

#endif // GUARD_CONFIG_MOD_H
