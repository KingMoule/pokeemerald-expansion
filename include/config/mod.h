#ifndef GUARD_CONFIG_MOD_H
#define GUARD_CONFIG_MOD_H

// Valeurs provisoires du mod (wiki v0.2). Tous les chiffres d'équilibrage
// sont regroupés ici pour être modifiés sans toucher au reste du code.

// Donjon rogue-lite (étape 9, à la Lufia 2) : nom retenu « Shifting Ruins ».

// Niveaux au-delà de 100 : chaque sceau rapporté du donjon relève le plafond de toute la sauvegarde.
// Désactivé pour l'instant (décision de Guillaume) : niveau 100 maximal et pas d'Ascension Seal en jeu.
#define MOD_LEVELS_BEYOND_100         FALSE
#define MOD_LEVEL_ABSOLUTE_MAX        (MOD_LEVELS_BEYOND_100 ? 250 : 100) // 250 : plafond technique (niveau sur 8 bits).
#define MOD_LEVEL_BASE_CAP            100  // Plafond sans sceau.
#define MOD_LEVEL_CAP_PER_SEAL        10   // Niveaux de plafond gagnés par sceau (provisoire).

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
#define MOD_MUTATION_INHERIT_CHANCE   30   // % de transmission sans Lineage Charm (un parent muté compatible).
#define MOD_LINEAGE_CHARM_CONSUMED    FALSE // Le Lineage Charm est-il consommé à la ponte ? (question ouverte du wiki)

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
