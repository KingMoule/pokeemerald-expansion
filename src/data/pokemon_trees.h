// Arbres élémentaires (wiki v0.2, maquette de l'arbre Feu).
// Les valeurs sont en ‰ par rang, sauf mention contraire. Tous les chiffres sont provisoires.

#define TREE_TRUNK                                                                                  \
    [0] = {                                                                                         \
        .name = COMPOUND_STRING("Force physique"),                                                  \
        .description = COMPOUND_STRING("+1 % aux dégâts\nphysiques par rang."),                     \
        .effect = TREE_EFFECT_PHYSICAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,                   \
    },                                                                                              \
    [1] = {                                                                                         \
        .name = COMPOUND_STRING("Force spéciale"),                                                  \
        .description = COMPOUND_STRING("+1 % aux dégâts\nspéciaux par rang."),                      \
        .effect = TREE_EFFECT_SPECIAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,                    \
    },                                                                                              \
    [2] = {                                                                                         \
        .name = COMPOUND_STRING("Vitalité"),                                                        \
        .description = COMPOUND_STRING("+1 % aux PV max\npar rang."),                               \
        .effect = TREE_EFFECT_MAX_HP, .maxRank = 20, .valuePerRank = 10,                            \
    },                                                                                              \
    [3] = {                                                                                         \
        .name = COMPOUND_STRING("Vivacité"),                                                        \
        .description = COMPOUND_STRING("+0,5 % à la Vitesse\npar rang."),                           \
        .effect = TREE_EFFECT_SPEED, .maxRank = 20, .valuePerRank = 5,                              \
    }

// Les éléments sans branches écrites n'ont que le tronc commun.
#define TRUNK_ONLY_TREE { .nodes = { TREE_TRUNK } }

const struct ElementTree gElementTrees[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = TRUNK_ONLY_TREE,
    [TYPE_FIGHTING] = TRUNK_ONLY_TREE,
    [TYPE_FLYING] = TRUNK_ONLY_TREE,
    [TYPE_POISON] = TRUNK_ONLY_TREE,
    [TYPE_GROUND] = TRUNK_ONLY_TREE,
    [TYPE_ROCK] = TRUNK_ONLY_TREE,
    [TYPE_BUG] = TRUNK_ONLY_TREE,
    [TYPE_GHOST] = TRUNK_ONLY_TREE,
    [TYPE_STEEL] = TRUNK_ONLY_TREE,
    [TYPE_WATER] = TRUNK_ONLY_TREE,
    [TYPE_GRASS] = TRUNK_ONLY_TREE,
    [TYPE_ELECTRIC] = TRUNK_ONLY_TREE,
    [TYPE_PSYCHIC] = TRUNK_ONLY_TREE,
    [TYPE_ICE] = TRUNK_ONLY_TREE,
    [TYPE_DRAGON] = TRUNK_ONLY_TREE,
    [TYPE_DARK] = TRUNK_ONLY_TREE,
    [TYPE_FAIRY] = TRUNK_ONLY_TREE,
    [TYPE_FIRE] =
    {
        .branchNames = {
            COMPOUND_STRING("Assaut"),
            COMPOUND_STRING("Combustion"),
            COMPOUND_STRING("Solaire"),
        },
        .status = STATUS1_BURN,
        .weather = B_WEATHER_SUN,
        .nodes =
        {
            TREE_TRUNK,
            // Assaut : attaques physiques Feu, recul et prise de risque.
            [4] = {
                .name = COMPOUND_STRING("Impact ardent"),
                .description = COMPOUND_STRING("+1 % aux dégâts physiques\nFeu par rang."),
                .effect = TREE_EFFECT_TYPE_PHYSICAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,
            },
            [5] = {
                .name = COMPOUND_STRING("Corps de braise"),
                .description = COMPOUND_STRING("-3 % au recul des\nattaques Feu par rang."),
                .effect = TREE_EFFECT_TYPE_RECOIL, .maxRank = 10, .valuePerRank = 30,
            },
            [6] = {
                .name = COMPOUND_STRING("Fournaise"),
                .description = COMPOUND_STRING("Sous 50 % des PV, +2 %\naux dégâts Feu par rang."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_LOW_HP, .maxRank = 10, .valuePerRank = 20,
            },
            [7] = {
                .name = COMPOUND_STRING("Cœur incandescent"),
                .description = COMPOUND_STRING("Maîtrise. Règles encore\nà préciser (inactif)."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_STACK, .maxRank = 1, .valuePerRank = 100,
            },
            // Combustion : infliger une brûlure et exploiter les cibles brûlées.
            [8] = {
                .name = COMPOUND_STRING("Étincelle persistante"),
                .description = COMPOUND_STRING("+0,5 point de chance\nde brûlure par rang."),
                .effect = TREE_EFFECT_STATUS_CHANCE, .maxRank = 20, .valuePerRank = 5,
            },
            [9] = {
                .name = COMPOUND_STRING("Attiser les flammes"),
                .description = COMPOUND_STRING("+2 % aux dégâts des\nbrûlures infligées par rang."),
                .effect = TREE_EFFECT_STATUS_DAMAGE, .maxRank = 10, .valuePerRank = 20,
            },
            [10] = {
                .name = COMPOUND_STRING("Chasseur de cendres"),
                .description = COMPOUND_STRING("+1 % aux dégâts contre\nune cible brûlée par rang."),
                .effect = TREE_EFFECT_DAMAGE_VS_STATUS, .maxRank = 10, .valuePerRank = 10,
            },
            [11] = {
                .name = COMPOUND_STRING("Braises tenaces"),
                .description = COMPOUND_STRING("Maîtrise. Règles encore\nà préciser (inactif)."),
                .effect = TREE_EFFECT_LINGERING_STATUS, .maxRank = 1, .valuePerRank = 1,
            },
            // Solaire : soleil, puissance conditionnelle et survie.
            [12] = {
                .name = COMPOUND_STRING("Rayonnement"),
                .description = COMPOUND_STRING("Sous le soleil, +1 %\naux dégâts Feu par rang."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_WEATHER, .maxRank = 20, .valuePerRank = 10,
            },
            [13] = {
                .name = COMPOUND_STRING("Été prolongé"),
                .description = COMPOUND_STRING("+1 tour de soleil\ninvoqué par rang."),
                .effect = TREE_EFFECT_WEATHER_TURNS, .maxRank = 5, .valuePerRank = 1,
            },
            [14] = {
                .name = COMPOUND_STRING("Chaleur vitale"),
                .description = COMPOUND_STRING("Sous le soleil, +0,5 %\ndes PV max par tour par rang."),
                .effect = TREE_EFFECT_WEATHER_HEAL, .maxRank = 10, .valuePerRank = 5,
            },
            [15] = {
                .name = COMPOUND_STRING("Aube intérieure"),
                .description = COMPOUND_STRING("Maîtrise. Soleil à la première\nentrée (pas encore actif)."),
                .effect = TREE_EFFECT_WEATHER_ON_ENTRY, .maxRank = 1, .valuePerRank = 3,
            },
        },
    },
};

#undef TRUNK_ONLY_TREE
#undef TREE_TRUNK
