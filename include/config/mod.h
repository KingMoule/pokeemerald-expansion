#ifndef GUARD_CONFIG_MOD_H
#define GUARD_CONFIG_MOD_H

// Valeurs provisoires du mod (wiki v0.2). Tous les chiffres d'équilibrage
// sont regroupés ici pour être modifiés sans toucher au reste du code.

// Donjon rogue-lite « Shifting Ruins » (wiki section 11, à la Lufia 2), construit sur les étages
// de la Pyramide de Combat. Valeurs provisoires.
#define MOD_RUINS_REQUIRED_BADGES     1    // Badges requis (Roxanne en parle en donnant le Technique Codex).
#define MOD_RUINS_START_LEVEL         5    // Niveau des Pokémon de départ et du Pokémon ramené.
#define MOD_RUINS_LANDING_EVERY       5    // Un palier de sortie tous les N étages (sortie possible par l'escalier).
#define MOD_RUINS_MAX_FLOOR           50   // Profondeur maximale : sortie forcée et sécurisée en bas.
#define MOD_RUINS_LEVEL_BASE          5    // Niveau des sauvages à l'étage 1...
#define MOD_RUINS_LEVELS_PER_FLOOR    2    // ...puis +N niveaux par étage (100 au plus).
#define MOD_RUINS_TRAINER_LEVEL_BONUS 1    // Niveau des dresseurs : niveau des sauvages + N.
#define MOD_RUINS_TRAINER_MONS_BASE   1    // Pokémon par dresseur à l'étage 1, +1 à chaque palier franchi...
#define MOD_RUINS_TRAINER_MONS_MAX    3    // ...dans cette limite.
#define MOD_RUINS_START_BALLS         5    // Poké Balls dans le sac au départ.
#define MOD_RUINS_START_POTIONS       3    // Potions dans le sac au départ.
#define MOD_RUINS_RELIC_CHANCE        6    // % qu'un objet au sol soit un objet rare (Technique Codex)...
#define MOD_RUINS_RARE_RELIC_CHANCE   1    // ...ou très rare (Lineage Charm), dès le premier palier franchi.
#define MOD_RUINS_ESCAPE_ORB_CHANCE   4    // % qu'un objet au sol soit un Escape Orb (sortie sécurisée).
#define MOD_RUINS_MAX_RELICS          16   // Objets rares en attente au plus pendant une tentative.
// Progression permanente, gagnée en sortant vivant : record = étage le plus profond d'une sortie réussie.
#define MOD_RUINS_EXTRA_MON_EVERY     10   // Un Pokémon de départ de plus par tranche de N étages de record...
#define MOD_RUINS_MAX_START_MONS      3    // ...dans cette limite.
#define MOD_RUINS_TIER_EVERY          10   // Un vivier de départ plus fort par tranche de N étages de record.

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

// Donjon d'entraînement des EV (« Training Depths », wiki section 10). Valeurs provisoires.
#define MOD_EVDJ_REQUIRED_BADGES      4    // Badges requis pour entrer (milieu de campagne).
#define MOD_EVDJ_REST_EVERY           3    // Un palier de repos (sortie possible, soins) tous les N étages.
#define MOD_EVDJ_GAIN_BASE            8    // EV en attente gagnés par étage au départ...
#define MOD_EVDJ_GAIN_STEP            4    // ...plus ce bonus à chaque palier franchi (profondeur = meilleurs gains).
#define MOD_EVDJ_DEFEAT_KEPT_PCT      50   // % des gains non sécurisés conservés en cas de défaite.
#define MOD_EVDJ_LEVEL_OFFSET         5    // Niveau adverse à l'étage 1 : plus haut niveau de l'équipe moins N...
#define MOD_EVDJ_LEVELS_PER_FLOOR     1    // ...puis +N niveaux par étage.
#define MOD_EVDJ_PARTY_SIZE_BASE      2    // Pokémon adverses à l'étage 1, +1 à chaque palier (6 au plus).
#define MOD_EVDJ_ENEMY_IV_BASE        10   // IV adverses : base + étage (31 au plus).
#define MOD_EVDJ_MAX_FLOOR            99   // Profondeur maximale.

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

// Adversaires avec des builds (wiki section 13). Valeurs provisoires.
#define MOD_TRAINER_BUILDS            TRUE // Arbres et variantes pour les dresseurs importants (voir src/trainer_build.c).
#define MOD_TRAINER_BUILD_MIN_BADGES  2    // Pas de build adverse avant N badges (le rival de la Route 110 les annonce).
#define MOD_TRAINER_VARIANT_LEVEL     30   // Champions, rival, chefs d'équipe : une variante dès ce niveau.
#define MOD_TRAINER_MUTATIONS         TRUE // Mutations de src/data/trainer_mutations.h (Conseil 4, Maître, revanches, défis).
#define MOD_TRAINER_FAST_BASE_SPEED   80   // Vitesse de base à partir de laquelle l'IA investit la Vitesse avant les PV.

// Défis à règles annoncées (wiki sections 13 et 14), au Centre Pokémon d'Ever Grande, après le Panthéon.
#define MOD_CHALLENGE_RISING_MAX_LEVEL 50  // « Rising Trial » : niveau maximal des Pokémon du joueur.
#define MOD_CHALLENGE_VARIETY_SIZE     6   // « Variety Trial » : Pokémon requis, sans type en commun.

#endif // GUARD_CONFIG_MOD_H
