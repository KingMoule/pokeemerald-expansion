// Arbres élémentaires (wiki v0.2, maquette de l'arbre Feu).
// Les valeurs sont en ‰ par rang, sauf mention contraire. Tous les chiffres sont provisoires.

#define TREE_TRUNK                                                                                  \
    [0] = {                                                                                         \
        .name = COMPOUND_STRING("Physical Strength"),                                                  \
        .description = COMPOUND_STRING("+1% physical damage\nper rank."),                     \
        .effect = TREE_EFFECT_PHYSICAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,                   \
    },                                                                                              \
    [1] = {                                                                                         \
        .name = COMPOUND_STRING("Special Strength"),                                                  \
        .description = COMPOUND_STRING("+1% special damage\nper rank."),                      \
        .effect = TREE_EFFECT_SPECIAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,                    \
    },                                                                                              \
    [2] = {                                                                                         \
        .name = COMPOUND_STRING("Vitality"),                                                        \
        .description = COMPOUND_STRING("+1% max HP\nper rank."),                               \
        .effect = TREE_EFFECT_MAX_HP, .maxRank = 20, .valuePerRank = 10,                            \
    },                                                                                              \
    [3] = {                                                                                         \
        .name = COMPOUND_STRING("Swiftness"),                                                        \
        .description = COMPOUND_STRING("+0.5% Speed\nper rank."),                           \
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
            COMPOUND_STRING("Assault"),
            COMPOUND_STRING("Combustion"),
            COMPOUND_STRING("Solar"),
        },
        .status = STATUS1_BURN,
        .weather = B_WEATHER_SUN,
        .nodes =
        {
            TREE_TRUNK,
            // Assaut : attaques physiques Feu, recul et prise de risque.
            [4] = {
                .name = COMPOUND_STRING("Searing Impact"),
                .description = COMPOUND_STRING("+1% physical Fire\ndamage per rank."),
                .effect = TREE_EFFECT_TYPE_PHYSICAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,
            },
            [5] = {
                .name = COMPOUND_STRING("Ember Body"),
                .description = COMPOUND_STRING("-3% recoil from Fire\nmoves per rank."),
                .effect = TREE_EFFECT_TYPE_RECOIL, .maxRank = 10, .valuePerRank = 30,
            },
            [6] = {
                .name = COMPOUND_STRING("Furnace"),
                .description = COMPOUND_STRING("Below 50% HP, +2% Fire\ndamage per rank."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_LOW_HP, .maxRank = 10, .valuePerRank = 20,
            },
            [7] = {
                .name = COMPOUND_STRING("Incandescent Heart"),
                .description = COMPOUND_STRING("Mastery. Rules not yet\ndefined (inactive)."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_STACK, .maxRank = 1, .valuePerRank = 100,
            },
            // Combustion : infliger une brûlure et exploiter les cibles brûlées.
            [8] = {
                .name = COMPOUND_STRING("Lingering Spark"),
                .description = COMPOUND_STRING("+0.5% burn chance\nper rank."),
                .effect = TREE_EFFECT_STATUS_CHANCE, .maxRank = 20, .valuePerRank = 5,
            },
            [9] = {
                .name = COMPOUND_STRING("Fan the Flames"),
                .description = COMPOUND_STRING("+2% damage from burns\nyou inflict per rank."),
                .effect = TREE_EFFECT_STATUS_DAMAGE, .maxRank = 10, .valuePerRank = 20,
            },
            [10] = {
                .name = COMPOUND_STRING("Ash Hunter"),
                .description = COMPOUND_STRING("+1% damage against\nburned targets per rank."),
                .effect = TREE_EFFECT_DAMAGE_VS_STATUS, .maxRank = 10, .valuePerRank = 10,
            },
            [11] = {
                .name = COMPOUND_STRING("Stubborn Embers"),
                .description = COMPOUND_STRING("Mastery. Rules not yet\ndefined (inactive)."),
                .effect = TREE_EFFECT_LINGERING_STATUS, .maxRank = 1, .valuePerRank = 1,
            },
            // Solaire : soleil, puissance conditionnelle et survie.
            [12] = {
                .name = COMPOUND_STRING("Radiance"),
                .description = COMPOUND_STRING("In sun, +1% Fire\ndamage per rank."),
                .effect = TREE_EFFECT_TYPE_DAMAGE_WEATHER, .maxRank = 20, .valuePerRank = 10,
            },
            [13] = {
                .name = COMPOUND_STRING("Long Summer"),
                .description = COMPOUND_STRING("+1 turn to the sun you\nsummon per rank."),
                .effect = TREE_EFFECT_WEATHER_TURNS, .maxRank = 5, .valuePerRank = 1,
            },
            [14] = {
                .name = COMPOUND_STRING("Vital Warmth"),
                .description = COMPOUND_STRING("In sun, heal 0.5% max HP\neach turn per rank."),
                .effect = TREE_EFFECT_WEATHER_HEAL, .maxRank = 10, .valuePerRank = 5,
            },
            [15] = {
                .name = COMPOUND_STRING("Inner Dawn"),
                .description = COMPOUND_STRING("Mastery. Sun on first\nswitch-in (inactive)."),
                .effect = TREE_EFFECT_WEATHER_ON_ENTRY, .maxRank = 1, .valuePerRank = 3,
            },
        },
    },
};

#undef TRUNK_ONLY_TREE
#undef TREE_TRUNK
