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

// Branches génériques des éléments autres que le Feu (valeurs provisoires, à valider par Guillaume).
// Assaut : attaques physiques de l'élément ; Focus : attaques spéciales ; Statut : infliger et
// exploiter le statut de l'élément ; Météo : profiter de la météo de l'élément ; Garde : encaisser.
// Maîtrises : Cœur (Assaut), Aube (Météo) et statut tenace agissent comme celles du Feu ;
// Focus et Garde ont les leurs (valeurs MOD_TREE_FOCUS_CRIT_STAGES et MOD_TREE_GUARD_FIRST_HIT).
#define BRANCH_ASSAULT(first)                                                                       \
    [first] = {                                                                                     \
        .name = COMPOUND_STRING("Elemental Impact"),                                                \
        .description = COMPOUND_STRING("+1% physical damage\nof its type per rank."),              \
        .effect = TREE_EFFECT_TYPE_PHYSICAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,              \
    },                                                                                              \
    [first + 1] = {                                                                                 \
        .name = COMPOUND_STRING("Recoil Guard"),                                                    \
        .description = COMPOUND_STRING("-3% recoil from moves\nof its type per rank."),            \
        .effect = TREE_EFFECT_TYPE_RECOIL, .maxRank = 10, .valuePerRank = 30,                       \
    },                                                                                              \
    [first + 2] = {                                                                                 \
        .name = COMPOUND_STRING("Desperation"),                                                     \
        .description = COMPOUND_STRING("Below 50% HP, +2% damage\nof its type per rank."),         \
        .effect = TREE_EFFECT_TYPE_DAMAGE_LOW_HP, .maxRank = 10, .valuePerRank = 20,                \
    },                                                                                              \
    [first + 3] = {                                                                                 \
        .name = COMPOUND_STRING("Elemental Heart"),                                                 \
        .description = COMPOUND_STRING("Each hit: +10% damage of\nits type (x3), -5% HP."),  \
        .effect = TREE_EFFECT_TYPE_DAMAGE_STACK, .maxRank = 1, .valuePerRank = 100,                 \
    }

#define BRANCH_FOCUS(first)                                                                         \
    [first] = {                                                                                     \
        .name = COMPOUND_STRING("Elemental Focus"),                                                 \
        .description = COMPOUND_STRING("+1% special damage\nof its type per rank."),               \
        .effect = TREE_EFFECT_TYPE_SPECIAL_DAMAGE, .maxRank = 20, .valuePerRank = 10,               \
    },                                                                                              \
    [first + 1] = {                                                                                 \
        .name = COMPOUND_STRING("Quick Mind"),                                                      \
        .description = COMPOUND_STRING("+0.5% Speed\nper rank."),                                  \
        .effect = TREE_EFFECT_SPEED, .maxRank = 10, .valuePerRank = 5,                              \
    },                                                                                              \
    [first + 2] = {                                                                                 \
        .name = COMPOUND_STRING("Deep Focus"),                                                      \
        .description = COMPOUND_STRING("+1% special damage\nper rank."),                           \
        .effect = TREE_EFFECT_SPECIAL_DAMAGE, .maxRank = 10, .valuePerRank = 10,                    \
    },                                                                                              \
    [first + 3] = {                                                                                 \
        .name = COMPOUND_STRING("Piercing Focus"),                                                  \
        .description = COMPOUND_STRING("Special moves of its type:\n+1 critical-hit stage."),      \
        .effect = TREE_EFFECT_TYPE_SPECIAL_CRIT, .maxRank = 1, .valuePerRank = MOD_TREE_FOCUS_CRIT_STAGES, \
    }

#define BRANCH_STATUS(first)                                                                        \
    [first] = {                                                                                     \
        .name = COMPOUND_STRING("Afflict"),                                                         \
        .description = COMPOUND_STRING("+0.5% chance to inflict\nits status per rank."),           \
        .effect = TREE_EFFECT_STATUS_CHANCE, .maxRank = 20, .valuePerRank = 5,                      \
    },                                                                                              \
    [first + 1] = {                                                                                 \
        .name = COMPOUND_STRING("Exploit"),                                                         \
        .description = COMPOUND_STRING("+1% damage per rank vs\ntargets with its status."),        \
        .effect = TREE_EFFECT_DAMAGE_VS_STATUS, .maxRank = 20, .valuePerRank = 10,                  \
    },                                                                                              \
    [first + 2] = {                                                                                 \
        .name = COMPOUND_STRING("Resilience"),                                                      \
        .description = COMPOUND_STRING("+1% max HP\nper rank."),                                   \
        .effect = TREE_EFFECT_MAX_HP, .maxRank = 10, .valuePerRank = 10,                            \
    },                                                                                              \
    [first + 3] = {                                                                                 \
        .name = COMPOUND_STRING("Lingering Curse"),                                                 \
        .description = COMPOUND_STRING("Once per battle, its first\nstatus outlasts a cure."), \
        .effect = TREE_EFFECT_LINGERING_STATUS, .maxRank = 1, .valuePerRank = 1,                    \
    }

#define BRANCH_WEATHER(first)                                                                       \
    [first] = {                                                                                     \
        .name = COMPOUND_STRING("Weather Power"),                                                   \
        .description = COMPOUND_STRING("In its weather, +1% dmg\nof its type per rank."),          \
        .effect = TREE_EFFECT_TYPE_DAMAGE_WEATHER, .maxRank = 20, .valuePerRank = 10,               \
    },                                                                                              \
    [first + 1] = {                                                                                 \
        .name = COMPOUND_STRING("Long Weather"),                                                    \
        .description = COMPOUND_STRING("+1 turn to the weather\nyou summon per rank."),            \
        .effect = TREE_EFFECT_WEATHER_TURNS, .maxRank = 5, .valuePerRank = 1,                       \
    },                                                                                              \
    [first + 2] = {                                                                                 \
        .name = COMPOUND_STRING("Weather Bloom"),                                                   \
        .description = COMPOUND_STRING("In its weather, heal 0.5%\nmax HP each turn per rank."),   \
        .effect = TREE_EFFECT_WEATHER_HEAL, .maxRank = 10, .valuePerRank = 5,                       \
    },                                                                                              \
    [first + 3] = {                                                                                 \
        .name = COMPOUND_STRING("Herald"),                                                          \
        .description = COMPOUND_STRING("Once per battle, calls\nits weather on first entry."),    \
        .effect = TREE_EFFECT_WEATHER_ON_ENTRY, .maxRank = 1, .valuePerRank = 1,                    \
    }

#define BRANCH_GUARD(first)                                                                         \
    [first] = {                                                                                     \
        .name = COMPOUND_STRING("Bulwark"),                                                         \
        .description = COMPOUND_STRING("-0.5% damage from super\neffective moves per rank."),      \
        .effect = TREE_EFFECT_WEAKNESS_GUARD, .maxRank = 20, .valuePerRank = 5,                     \
    },                                                                                              \
    [first + 1] = {                                                                                 \
        .name = COMPOUND_STRING("Thick Hide"),                                                      \
        .description = COMPOUND_STRING("+1% max HP\nper rank."),                                   \
        .effect = TREE_EFFECT_MAX_HP, .maxRank = 10, .valuePerRank = 10,                            \
    },                                                                                              \
    [first + 2] = {                                                                                 \
        .name = COMPOUND_STRING("Last Stand"),                                                      \
        .description = COMPOUND_STRING("Below 50% HP, +2% damage\nof its type per rank."),         \
        .effect = TREE_EFFECT_TYPE_DAMAGE_LOW_HP, .maxRank = 10, .valuePerRank = 20,                \
    },                                                                                              \
    [first + 3] = {                                                                                 \
        .name = COMPOUND_STRING("Unbreakable"),                                                     \
        .description = COMPOUND_STRING("Once per battle, halves a\nsuper effective hit."),         \
        .effect = TREE_EFFECT_FIRST_WEAKNESS_GUARD, .maxRank = 1, .valuePerRank = MOD_TREE_GUARD_FIRST_HIT, \
    }

#define B1 (TREE_TRUNK_NODES)
#define B2 (TREE_TRUNK_NODES + TREE_BRANCH_NODES)
#define B3 (TREE_TRUNK_NODES + 2 * TREE_BRANCH_NODES)

// Assaut / Focus / Garde.
#define TREE_GUARD                                                                                  \
    {                                                                                               \
        .branchNames = { COMPOUND_STRING("Assault"), COMPOUND_STRING("Focus"), COMPOUND_STRING("Guard") }, \
        .nodes = { TREE_TRUNK, BRANCH_ASSAULT(B1), BRANCH_FOCUS(B2), BRANCH_GUARD(B3) },            \
    }

// Assaut / Focus / Météo.
#define TREE_WEATHER(weatherName, weatherFlags)                                                     \
    {                                                                                               \
        .branchNames = { COMPOUND_STRING("Assault"), COMPOUND_STRING("Focus"), COMPOUND_STRING(weatherName) }, \
        .weather = weatherFlags,                                                                    \
        .nodes = { TREE_TRUNK, BRANCH_ASSAULT(B1), BRANCH_FOCUS(B2), BRANCH_WEATHER(B3) },          \
    }

// Assaut / Statut / troisième branche (Garde ou Météo).
#define TREE_STATUS_GUARD(statusName, statusFlags)                                                  \
    {                                                                                               \
        .branchNames = { COMPOUND_STRING("Assault"), COMPOUND_STRING(statusName), COMPOUND_STRING("Guard") }, \
        .status = statusFlags,                                                                      \
        .nodes = { TREE_TRUNK, BRANCH_ASSAULT(B1), BRANCH_STATUS(B2), BRANCH_GUARD(B3) },           \
    }

#define TREE_STATUS_WEATHER(statusName, statusFlags, weatherName, weatherFlags)                     \
    {                                                                                               \
        .branchNames = { COMPOUND_STRING("Assault"), COMPOUND_STRING(statusName), COMPOUND_STRING(weatherName) }, \
        .status = statusFlags,                                                                      \
        .weather = weatherFlags,                                                                    \
        .nodes = { TREE_TRUNK, BRANCH_ASSAULT(B1), BRANCH_STATUS(B2), BRANCH_WEATHER(B3) },         \
    }

const struct ElementTree gElementTrees[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = TREE_GUARD,
    [TYPE_FIGHTING] = TREE_GUARD,
    [TYPE_FLYING]   = TREE_GUARD,
    [TYPE_POISON]   = TREE_STATUS_GUARD("Toxin", STATUS1_PSN_ANY),
    [TYPE_GROUND]   = TREE_WEATHER("Sandstorm", B_WEATHER_SANDSTORM),
    [TYPE_ROCK]     = TREE_WEATHER("Sandstorm", B_WEATHER_SANDSTORM),
    [TYPE_BUG]      = TREE_GUARD,
    [TYPE_GHOST]    = TREE_GUARD,
    [TYPE_STEEL]    = TREE_GUARD,
    [TYPE_WATER]    = TREE_WEATHER("Rain", B_WEATHER_RAIN),
    [TYPE_GRASS]    = TREE_STATUS_WEATHER("Spores", STATUS1_SLEEP, "Sunlight", B_WEATHER_SUN),
    [TYPE_ELECTRIC] = TREE_STATUS_WEATHER("Static", STATUS1_PARALYSIS, "Storm", B_WEATHER_RAIN),
    [TYPE_PSYCHIC]  = TREE_GUARD,
    [TYPE_ICE]      = TREE_STATUS_WEATHER("Frost", STATUS1_FREEZE | STATUS1_FROSTBITE, "Hail", B_WEATHER_HAIL | B_WEATHER_SNOW),
    [TYPE_DRAGON]   = TREE_GUARD,
    [TYPE_DARK]     = TREE_GUARD,
    [TYPE_FAIRY]    = TREE_GUARD,
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
                .description = COMPOUND_STRING("Each Fire hit: +10% Fire\ndamage (x3), costs 5% HP."),
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
                .description = COMPOUND_STRING("Once per battle, its first\nburn outlasts a cure."),
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
                .description = COMPOUND_STRING("Once per battle, calls\nthe sun on first entry."),
                .effect = TREE_EFFECT_WEATHER_ON_ENTRY, .maxRank = 1, .valuePerRank = 3,
            },
        },
    },
};

#undef TREE_STATUS_WEATHER
#undef TREE_STATUS_GUARD
#undef TREE_WEATHER
#undef TREE_GUARD
#undef B1
#undef B2
#undef B3
#undef BRANCH_GUARD
#undef BRANCH_WEATHER
#undef BRANCH_STATUS
#undef BRANCH_FOCUS
#undef BRANCH_ASSAULT
#undef TREE_TRUNK
