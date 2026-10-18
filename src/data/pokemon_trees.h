// Arbres élémentaires : General (commun) puis Offense, Defense et Utility propres à chaque type.
// Conception : /mnt/project-files/mod/nouveaux-nodes-par-type.md (validée par Guillaume le 18/10).
// Les valeurs sont en ‱ par rang (100 = 1 %), sauf mention contraire (voir enum TreeEffect).

#define NODE(nodeName, desc, nodeEffect, ranks, value, nodeParam) \
    {                                                             \
        .name = COMPOUND_STRING(nodeName),                        \
        .description = COMPOUND_STRING(desc),                     \
        .effect = nodeEffect,                                     \
        .maxRank = ranks,                                         \
        .valuePerRank = value,                                    \
        .param = nodeParam,                                       \
    }

#define GENERAL                                                                                                     \
    [0] = NODE("Physical Strength", "+1% physical damage\nper rank.", TREE_EFFECT_PHYSICAL_DAMAGE, 10, 100, 0),      \
    [1] = NODE("Special Strength", "+1% special damage\nper rank.", TREE_EFFECT_SPECIAL_DAMAGE, 10, 100, 0),         \
    [2] = NODE("Vitality", "+1% max HP\nper rank.", TREE_EFFECT_MAX_HP, 20, 100, 0),                                 \
    [3] = NODE("Swiftness", "+1% Speed\nper rank.", TREE_EFFECT_SPEED, 10, 100, 0)

#define OFFENSE(n1, n2, n3, n4)  [TREE_NODE_OFFENSE] = n1, [TREE_NODE_OFFENSE + 1] = n2, [TREE_NODE_OFFENSE + 2] = n3, [TREE_NODE_OFFENSE + 3] = n4
#define DEFENSE(n1, n2, n3, n4)  [TREE_NODE_DEFENSE] = n1, [TREE_NODE_DEFENSE + 1] = n2, [TREE_NODE_DEFENSE + 2] = n3, [TREE_NODE_DEFENSE + 3] = n4
#define UTILITY(n1, n2, n3, n4)  [TREE_NODE_UTILITY] = n1, [TREE_NODE_UTILITY + 1] = n2, [TREE_NODE_UTILITY + 2] = n3, [TREE_NODE_UTILITY + 3] = n4

// Nœuds qui reviennent d'un type à l'autre.
#define POWER(typeName)          NODE(typeName " Power", "+1% damage of its " typeName "\nmoves per rank.", TREE_EFFECT_TYPE_DAMAGE, 20, 100, 0)
#define LOW_HP(nodeName, typeName) NODE(nodeName, "Below 50% HP, +2% damage\nof its " typeName " moves per rank.", TREE_EFFECT_TYPE_DAMAGE_LOW_HP, 10, 200, 0)
#define WARD(nodeName, desc, types) NODE(nodeName, desc, TREE_EFFECT_WARD, 20, 125, types)
#define WARD2(nodeName, desc, types) NODE(nodeName, desc, TREE_EFFECT_WARD, 10, 250, types)
#define SE_WARD(nodeName)        NODE(nodeName, "-1.5% damage from super\neffective moves per rank.", TREE_EFFECT_WEAKNESS_GUARD, 20, 150, 0)
#define FIELD_TURNS(nodeName, what, field) NODE(nodeName, "+1 turn to the " what "\nit sets per rank.", TREE_EFFECT_FIELD_TURNS, 3, 1, field)
#define CONTACT(nodeName, verb, status) NODE(nodeName, "+3% chance per rank to " verb "\nattackers that touch it.", TREE_EFFECT_CONTACT_STATUS, 10, 300, status)
#define EMERGENCY(nodeName)      NODE(nodeName, "Once per battle, below 33%\nHP, heals 25% of max HP.", TREE_EFFECT_EMERGENCY_HEAL, 1, 2500, 0)
#define UNBREAKABLE(nodeName)    NODE(nodeName, "Once per battle, halves a\nsuper effective hit.", TREE_EFFECT_FIRST_HIT_GUARD, 1, 5000, TREE_GUARD_SUPER_EFFECTIVE)
#define STURDY(nodeName)         NODE(nodeName, "Once per battle, at full HP,\nsurvives a KO hit with 1 HP.", TREE_EFFECT_ENDURE, 1, 1, 0)
#define FLINCH(nodeName, typeName) NODE(nodeName, "+0.5% flinch chance of its\n" typeName " moves per rank.", TREE_EFFECT_SECONDARY_CHANCE, 20, 50, TREE_SECONDARY_FLINCH)
#define AFFLICT(nodeName, what)  NODE(nodeName, "+0.5% chance to " what "\nper rank.", TREE_EFFECT_STATUS_CHANCE, 20, 50, 0)
#define MOVES_UP(nodeName, desc, family) NODE(nodeName, desc, TREE_EFFECT_MOVE_FAMILY_DAMAGE, 10, 200, family)
#define CONDITION(nodeName, desc, nodeEffect) NODE(nodeName, desc, nodeEffect, 10, 20, 0)
#define HEAL_TURN(nodeName, desc) NODE(nodeName, desc, TREE_EFFECT_WEATHER_HEAL, 10, 50, 0)

const u8 gTreeGeneralName[] = _("General");

const u8 *const gTreeBranchNames[TREE_BRANCH_COUNT] =
{
    [TREE_BRANCH_OFFENSE] = COMPOUND_STRING("Offense"),
    [TREE_BRANCH_DEFENSE] = COMPOUND_STRING("Defense"),
    [TREE_BRANCH_UTILITY] = COMPOUND_STRING("Utility"),
};

const struct ElementTree gElementTrees[NUMBER_OF_MON_TYPES] =
{
    [TYPE_FIRE] =
    {
        .status = STATUS1_BURN,
        .weather = B_WEATHER_SUN,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Fire"),
                    LOW_HP("Blaze", "Fire"),
                    NODE("Ash Hunter", "+2% damage per rank vs\nburned targets.", TREE_EFFECT_DAMAGE_VS_STATUS, 10, 200, 0),
                    NODE("Incandescent Heart", "Each Fire hit: +10% Fire\ndamage (x3), costs 5% HP.", TREE_EFFECT_TYPE_DAMAGE_STACK, 1, 1000, TREE_STACK_EVERY_HIT)),
            DEFENSE(WARD("Steam Veil", "-1.25% damage from Water\nmoves per rank.", TREE_TYPES(TYPE_WATER)),
                    WARD2("Magma Crust", "-2.5% damage from Ground\nand Rock moves per rank.", TREE_TYPES(TYPE_GROUND, TYPE_ROCK)),
                    CONTACT("Flame Body", "burn", STATUS1_BURN),
                    EMERGENCY("Rekindle")),
            UTILITY(AFFLICT("Lingering Spark", "burn"),
                    FIELD_TURNS("Long Summer", "sun", TREE_FIELD_WEATHER),
                    NODE("Fan the Flames", "+5% damage from burns\nit inflicts per rank.", TREE_EFFECT_STATUS_DAMAGE, 10, 500, 0),
                    NODE("Inner Dawn", "Once per battle, calls\nthe sun on first entry.", TREE_EFFECT_WEATHER_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_WATER] =
    {
        .weather = B_WEATHER_RAIN,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Water"),
                    LOW_HP("Torrent", "Water"),
                    NODE("Full Tide", "Above 75% HP, +1.5% damage\nof its Water moves per rank.", TREE_EFFECT_TYPE_DAMAGE_HIGH_HP, 10, 150, 0),
                    NODE("Berserk Tide", "First time below 50% HP:\n+1 Attack and Sp. Atk.", TREE_EFFECT_BERSERK, 1, 1, 0)),
            DEFENSE(WARD("Root Ward", "-1.25% damage from Grass\nmoves per rank.", TREE_TYPES(TYPE_GRASS)),
                    WARD2("Insulation", "-2.5% damage from Electric\nmoves per rank.", TREE_TYPES(TYPE_ELECTRIC)),
                    NODE("Hydration", "In rain, +5% chance per rank\nto cure its status each turn.", TREE_EFFECT_STATUS_CURE_END_TURN, 10, 500, B_WEATHER_RAIN),
                    NODE("Aqua Ring", "First time below 50% HP,\nwraps itself in Aqua Ring.", TREE_EFFECT_AQUA_RING_ONCE, 1, 1, 0)),
            UTILITY(NODE("Rain Power", "In rain, +1% damage of its\nWater moves per rank.", TREE_EFFECT_TYPE_DAMAGE_WEATHER, 20, 100, 0),
                    FIELD_TURNS("Long Rain", "rain", TREE_FIELD_WEATHER),
                    NODE("Swift Swim", "In rain, +2% Speed\nper rank.", TREE_EFFECT_FIELD_SPEED, 10, 200, 0),
                    NODE("Drizzle", "Once per battle, calls\nthe rain on first entry.", TREE_EFFECT_WEATHER_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_GRASS] =
    {
        .weather = B_WEATHER_SUN,
        .terrain = B_TERRAIN_GRASSY,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Grass"),
                    LOW_HP("Overgrow", "Grass"),
                    NODE("Deep Roots", "+5% HP drained by its\nmoves per rank.", TREE_EFFECT_DRAIN_BONUS, 10, 500, 0),
                    NODE("Sap Surge", "Each draining hit: +10%\nGrass damage (x3), no cost.", TREE_EFFECT_TYPE_DAMAGE_STACK, 1, 1000, TREE_STACK_DRAIN)),
            DEFENSE(SE_WARD("Thick Bark"),
                    WARD2("Thick Fat", "-2.5% damage from Fire\nand Ice moves per rank.", TREE_TYPES(TYPE_FIRE, TYPE_ICE)),
                    NODE("Natural Cure", "+5% chance per rank to cure\nits status when switching out.", TREE_EFFECT_STATUS_CURE_SWITCH, 10, 500, 0),
                    EMERGENCY("Regrowth")),
            UTILITY(NODE("Spore Craft", "+1% accuracy of its status\nmoves per rank.", TREE_EFFECT_STATUS_MOVE_ACCURACY, 20, 100, 0),
                    FIELD_TURNS("Long Terrain", "Grassy Terrain", TREE_FIELD_TERRAIN),
                    NODE("Chlorophyll", "In sun or Grassy Terrain,\n+2% Speed per rank.", TREE_EFFECT_FIELD_SPEED, 10, 200, 0),
                    NODE("Grassy Surge", "Once per battle, sets Grassy\nTerrain on first entry.", TREE_EFFECT_TERRAIN_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_ELECTRIC] =
    {
        .status = STATUS1_PARALYSIS,
        .terrain = B_TERRAIN_ELECTRIC,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Electric"),
                    CONDITION("Volt Rush", "+2% damage per rank when\nit moves before the target.", TREE_EFFECT_DAMAGE_MOVING_FIRST),
                    NODE("Static Shock", "+2% damage per rank vs\nparalyzed targets.", TREE_EFFECT_DAMAGE_VS_STATUS, 10, 200, 0),
                    NODE("Circuit", "From its 2nd Electric move\nin a row, +20% damage.", TREE_EFFECT_CHAIN_DAMAGE, 1, 2000, 0)),
            DEFENSE(WARD("Grounding Wire", "-1.25% damage from Ground\nmoves per rank.", TREE_TYPES(TYPE_GROUND)),
                    NODE("Insulated", "-1% damage from special\nmoves per rank.", TREE_EFFECT_CATEGORY_GUARD, 10, 100, TREE_CATEGORY_SPECIAL),
                    CONTACT("Static", "paralyze", STATUS1_PARALYSIS),
                    NODE("Magnet Rise", "Once per battle, the first\nGround move aimed at it fails.", TREE_EFFECT_GROUND_IMMUNITY_ONCE, 1, 1, 0)),
            UTILITY(AFFLICT("Afflict", "paralyze"),
                    FIELD_TURNS("Long Terrain", "Electric Terrain", TREE_FIELD_TERRAIN),
                    NODE("Surge Surfer", "On Electric Terrain,\n+2% Speed per rank.", TREE_EFFECT_FIELD_SPEED, 10, 200, 0),
                    NODE("Electric Surge", "Once per battle, sets Electric\nTerrain on first entry.", TREE_EFFECT_TERRAIN_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_ICE] =
    {
        .status = STATUS1_FREEZE | STATUS1_FROSTBITE,
        .weather = B_WEATHER_HAIL | B_WEATHER_SNOW,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Ice"),
                    CONDITION("Shatter", "+2% damage of super\neffective hits per rank.", TREE_EFFECT_SUPER_EFFECTIVE_DAMAGE),
                    NODE("Icicle Volley", "+10% chance per rank that\nmulti-hit moves hit max times.", TREE_EFFECT_MULTI_HIT_MAX, 10, 1000, 0),
                    NODE("Refrigerate", "Its Normal moves become\nIce moves, +10% damage.", TREE_EFFECT_NORMAL_TO_TYPE, 1, 1000, 0)),
            DEFENSE(SE_WARD("Frost Armor"),
                    WARD2("Glacier Wall", "-2.5% damage from Fighting\nand Rock moves per rank.", TREE_TYPES(TYPE_FIGHTING, TYPE_ROCK)),
                    NODE("Snow Cloak", "In hail or snow, -2% damage\ntaken per rank.", TREE_EFFECT_WEATHER_GUARD, 10, 200, 0),
                    NODE("Ice Face", "Once per battle, halves\nthe first physical hit.", TREE_EFFECT_FIRST_HIT_GUARD, 1, 5000, TREE_GUARD_PHYSICAL)),
            UTILITY(NODE("Afflict", "+0.25% chance to freeze\nper rank.", TREE_EFFECT_STATUS_CHANCE, 20, 25, 0),
                    FIELD_TURNS("Long Snow", "snow", TREE_FIELD_WEATHER),
                    HEAL_TURN("Ice Body", "In hail or snow, heals 0.5%\nof max HP each turn per rank."),
                    NODE("Snow Warning", "Once per battle, calls\nthe snow on first entry.", TREE_EFFECT_WEATHER_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_GROUND] =
    {
        .weather = B_WEATHER_SANDSTORM,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Ground"),
                    NODE("Sand Force", "In sandstorm, +2% damage of\nGround, Rock, Steel per rank.", TREE_EFFECT_TYPE_DAMAGE_WEATHER, 10, 200, TREE_TYPES(TYPE_GROUND, TYPE_ROCK, TYPE_STEEL)),
                    CONDITION("Aftershock", "+2% damage per rank when\nit moves after the target.", TREE_EFFECT_DAMAGE_MOVING_LAST),
                    NODE("Grounding Blow", "Its Ground moves hit Flying\nand levitating foes.", TREE_EFFECT_HIT_AIRBORNE, 1, 1, 0)),
            DEFENSE(WARD("Mud Ward", "-1.25% damage from Water\nmoves per rank.", TREE_TYPES(TYPE_WATER)),
                    WARD2("Root Guard", "-2.5% damage from Grass\nand Ice moves per rank.", TREE_TYPES(TYPE_GRASS, TYPE_ICE)),
                    NODE("Solid Footing", "-1% damage from physical\nmoves per rank.", TREE_EFFECT_CATEGORY_GUARD, 10, 100, TREE_CATEGORY_PHYSICAL),
                    NODE("Mud Coat", "Once per battle, halves the\nfirst Water or Grass hit.", TREE_EFFECT_FIRST_HIT_GUARD, 1, 5000, TREE_TYPES(TYPE_WATER, TYPE_GRASS))),
            UTILITY(NODE("Mudslinger", "+0.5% added effect chance of\nits Ground moves per rank.", TREE_EFFECT_SECONDARY_CHANCE, 20, 50, TREE_SECONDARY_ANY),
                    FIELD_TURNS("Long Sandstorm", "sandstorm", TREE_FIELD_WEATHER),
                    NODE("Sand Rush", "In sandstorm, +2% Speed\nper rank.", TREE_EFFECT_FIELD_SPEED, 10, 200, 0),
                    NODE("Sand Stream", "Once per battle, calls a\nsandstorm on first entry.", TREE_EFFECT_WEATHER_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_ROCK] =
    {
        .weather = B_WEATHER_SANDSTORM,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Rock"),
                    NODE("Rock Head", "-5% recoil damage\nper rank.", TREE_EFFECT_RECOIL_REDUCTION, 10, 500, 0),
                    NODE("Sniper", "+5% critical hit damage\nper rank.", TREE_EFFECT_CRIT_DAMAGE, 10, 500, 0),
                    NODE("Keen Edge", "Its Rock moves get +1\ncritical-hit stage.", TREE_EFFECT_TYPE_CRIT_STAGE, 1, 1, 0)),
            DEFENSE(SE_WARD("Solid Rock"),
                    NODE("Sand Armor", "In sandstorm, -2% damage\ntaken per rank.", TREE_EFFECT_WEATHER_GUARD, 10, 200, 0),
                    NODE("Thick Hide", "+1.5% max HP\nper rank.", TREE_EFFECT_MAX_HP, 10, 150, 0),
                    STURDY("Sturdy")),
            UTILITY(FLINCH("Rockfall", "Rock"),
                    FIELD_TURNS("Long Sandstorm", "sandstorm", TREE_FIELD_WEATHER),
                    HEAL_TURN("Desert Rest", "In sandstorm, heals 0.5% of\nmax HP each turn per rank."),
                    NODE("Stealth Rock", "Once per battle, sets Stealth\nRock on first entry.", TREE_EFFECT_HAZARD_ON_ENTRY, 1, 1, TREE_HAZARD_STEALTH_ROCK)),
        },
    },
    [TYPE_STEEL] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Steel"),
                    CONDITION("Heavy Metal", "+2% damage per rank when\nit moves after the target.", TREE_EFFECT_DAMAGE_MOVING_LAST),
                    MOVES_UP("Iron Fist", "+2% damage of punching\nmoves per rank.", TREE_MOVES_PUNCH),
                    NODE("Iron Momentum", "After taking a hit, its next\nSteel move deals +30%.", TREE_EFFECT_MOMENTUM_DAMAGE, 1, 3000, 0)),
            DEFENSE(WARD("Heatproof", "-1.25% damage from Fire\nmoves per rank.", TREE_TYPES(TYPE_FIRE)),
                    WARD2("Bedrock Plating", "-2.5% damage from Fighting\nand Ground moves per rank.", TREE_TYPES(TYPE_FIGHTING, TYPE_GROUND)),
                    NODE("Clear Body", "+5% chance per rank to ignore\na stat drop from foes.", TREE_EFFECT_STAT_DROP_IGNORE, 10, 500, 0),
                    UNBREAKABLE("Unbreakable")),
            UTILITY(FLINCH("Polished Edge", "Steel"),
                    FIELD_TURNS("Long Screens", "screens", TREE_FIELD_SCREENS),
                    NODE("Self-Repair", "Heals 0.5% of max HP\neach turn per rank.", TREE_EFFECT_END_TURN_HEAL, 10, 50, 0),
                    NODE("Shield Wall", "Once per battle, sets\nReflect on first entry.", TREE_EFFECT_SCREEN_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_PSYCHIC] =
    {
        .terrain = B_TERRAIN_PSYCHIC,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Psychic"),
                    CONDITION("Neuroforce", "+2% damage of super\neffective hits per rank.", TREE_EFFECT_SUPER_EFFECTIVE_DAMAGE),
                    CONDITION("Focused Mind", "+2% damage per rank while\none of its stats is raised.", TREE_EFFECT_DAMAGE_STAT_RAISED),
                    NODE("Mind Break", "+30% chance its Psychic\nmoves trigger added effects.", TREE_EFFECT_SECONDARY_CHANCE, 1, 3000, TREE_SECONDARY_ANY)),
            DEFENSE(WARD("Shadow Ward", "-1.25% damage from Dark\nand Ghost moves per rank.", TREE_TYPES(TYPE_DARK, TYPE_GHOST)),
                    WARD2("Bug Ward", "-2.5% damage from Bug\nmoves per rank.", TREE_TYPES(TYPE_BUG)),
                    NODE("Mental Barrier", "-5% indirect damage\nper rank.", TREE_EFFECT_INDIRECT_GUARD, 10, 500, 0),
                    NODE("Synchronize", "A status a foe inflicts\non it is passed back.", TREE_EFFECT_SYNCHRONIZE, 1, 1, 0)),
            UTILITY(NODE("Bewilder", "+0.5% confusion chance of\nits Psychic moves per rank.", TREE_EFFECT_SECONDARY_CHANCE, 20, 50, TREE_SECONDARY_CONFUSION),
                    FIELD_TURNS("Long Terrain", "Psychic Terrain", TREE_FIELD_TERRAIN),
                    FIELD_TURNS("Screen Craft", "screens", TREE_FIELD_SCREENS),
                    NODE("Psychic Surge", "Once per battle, sets Psychic\nTerrain on first entry.", TREE_EFFECT_TERRAIN_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_FAIRY] =
    {
        .terrain = B_TERRAIN_MISTY,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Fairy"),
                    CONDITION("Charm Offensive", "+2% damage per rank vs foes\nwith lowered Atk or Sp. Atk.", TREE_EFFECT_DAMAGE_TARGET_WEAKENED),
                    MOVES_UP("Enchanting Voice", "+2% damage of sound\nmoves per rank.", TREE_MOVES_SOUND),
                    NODE("Pixilate", "Its Normal moves become\nFairy moves, +10% damage.", TREE_EFFECT_NORMAL_TO_TYPE, 1, 1000, 0)),
            DEFENSE(WARD("Iron Ward", "-1.25% damage from Poison\nand Steel moves per rank.", TREE_TYPES(TYPE_POISON, TYPE_STEEL)),
                    NODE("Misty Veil", "+5% chance per rank to avoid\na status from foes.", TREE_EFFECT_STATUS_AVOID, 10, 500, 0),
                    NODE("Cute Charm", "+3% chance per rank to lower\nthe Attack of foes touching it.", TREE_EFFECT_CONTACT_WEAKEN, 10, 300, 0),
                    EMERGENCY("Moonlight Blessing")),
            UTILITY(NODE("Pixie Dust", "+0.5% added effect chance of\nits Fairy moves per rank.", TREE_EFFECT_SECONDARY_CHANCE, 20, 50, TREE_SECONDARY_ANY),
                    FIELD_TURNS("Long Terrain", "Misty Terrain", TREE_FIELD_TERRAIN),
                    NODE("Healing Aura", "+3% healing from its\nhealing moves per rank.", TREE_EFFECT_HEALING_MOVE_BOOST, 10, 300, 0),
                    NODE("Misty Surge", "Once per battle, sets Misty\nTerrain on first entry.", TREE_EFFECT_TERRAIN_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_POISON] =
    {
        .status = STATUS1_PSN_ANY,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Poison"),
                    NODE("Venomous", "+2% damage per rank vs\npoisoned targets.", TREE_EFFECT_DAMAGE_VS_STATUS, 10, 200, 0),
                    CONDITION("Predator", "+2% damage per rank vs\ntargets below 50% HP.", TREE_EFFECT_DAMAGE_TARGET_LOW_HP),
                    NODE("Merciless", "Always lands critical hits\non poisoned targets.", TREE_EFFECT_MERCILESS, 1, 1, 0)),
            DEFENSE(WARD("Earth Ward", "-1.25% damage from Ground\nmoves per rank.", TREE_TYPES(TYPE_GROUND)),
                    WARD2("Mind Ward", "-2.5% damage from Psychic\nmoves per rank.", TREE_TYPES(TYPE_PSYCHIC)),
                    CONTACT("Poison Point", "poison", STATUS1_POISON),
                    NODE("Toxic Debris", "Once per battle, the first\nphysical hit sets Toxic Spikes.", TREE_EFFECT_TOXIC_DEBRIS, 1, 1, 0)),
            UTILITY(AFFLICT("Afflict", "poison"),
                    NODE("Lingering Toxins", "+5% damage from poison\nit inflicts per rank.", TREE_EFFECT_STATUS_DAMAGE, 10, 500, 0),
                    NODE("Venom Siphon", "Heals 0.5% of max HP per rank\nwhen its poison hurts a foe.", TREE_EFFECT_STATUS_SIPHON, 10, 50, 0),
                    NODE("Toxic Spikes", "Once per battle, sets Toxic\nSpikes on first entry.", TREE_EFFECT_HAZARD_ON_ENTRY, 1, 1, TREE_HAZARD_TOXIC_SPIKES)),
        },
    },
    [TYPE_BUG] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Bug"),
                    NODE("Tinted Lens", "+5% damage of not very\neffective hits per rank.", TREE_EFFECT_NOT_VERY_EFFECTIVE_DAMAGE, 10, 500, 0),
                    NODE("Swarm Strikes", "+10% chance per rank that\nmulti-hit moves hit max times.", TREE_EFFECT_MULTI_HIT_MAX, 10, 1000, 0),
                    NODE("Swarm", "Below 50% HP, its Bug\nmoves deal +30% damage.", TREE_EFFECT_TYPE_DAMAGE_LOW_HP, 1, 3000, 0)),
            DEFENSE(WARD("Fireproof Shell", "-1.25% damage from Fire\nmoves per rank.", TREE_TYPES(TYPE_FIRE)),
                    WARD2("Chitin", "-2.5% damage from Flying\nand Rock moves per rank.", TREE_TYPES(TYPE_FLYING, TYPE_ROCK)),
                    NODE("Shed Skin", "+3% chance per rank to cure\nits status each turn.", TREE_EFFECT_STATUS_CURE_END_TURN, 10, 300, 0),
                    NODE("Molt", "Once per battle, sheds its\nstatus and lowered stats.", TREE_EFFECT_MOLT, 1, 1, 0)),
            UTILITY(NODE("Compound Eyes", "+0.5% accuracy of its moves\nper rank.", TREE_EFFECT_ACCURACY, 20, 50, 0),
                    NODE("Stinging", "+1% added effect chance of\nits Bug moves per rank.", TREE_EFFECT_SECONDARY_CHANCE, 10, 100, TREE_SECONDARY_ANY),
                    NODE("Leech", "+5% HP drained by its\nmoves per rank.", TREE_EFFECT_DRAIN_BONUS, 10, 500, 0),
                    NODE("Sticky Web", "Once per battle, sets Sticky\nWeb on first entry.", TREE_EFFECT_HAZARD_ON_ENTRY, 1, 1, TREE_HAZARD_STICKY_WEB)),
        },
    },
    [TYPE_FLYING] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Flying"),
                    MOVES_UP("Gale Force", "+2% damage of wind\nmoves per rank.", TREE_MOVES_WIND),
                    NODE("Reckless Dive", "-5% recoil damage\nper rank.", TREE_EFFECT_RECOIL_REDUCTION, 10, 500, 0),
                    NODE("Gale Wings", "At full HP, its Flying\nmoves get +1 priority.", TREE_EFFECT_PRIORITY_FULL_HP, 1, 1, 0)),
            DEFENSE(WARD("Storm Shield", "-1.25% damage from Electric\nmoves per rank.", TREE_TYPES(TYPE_ELECTRIC)),
                    WARD2("Frost and Stone", "-2.5% damage from Ice\nand Rock moves per rank.", TREE_TYPES(TYPE_ICE, TYPE_ROCK)),
                    NODE("Aerial Retreat", "Heals 2% of max HP per rank\nwhen switching out.", TREE_EFFECT_SWITCH_OUT_HEAL, 10, 200, 0),
                    UNBREAKABLE("Sky Guard")),
            UTILITY(FLINCH("Buffeting Wind", "Flying"),
                    FIELD_TURNS("Long Tailwind", "Tailwind", TREE_FIELD_TAILWIND),
                    NODE("Roost Mastery", "+3% healing from its\nhealing moves per rank.", TREE_EFFECT_HEALING_MOVE_BOOST, 10, 300, 0),
                    NODE("Herald of Winds", "Once per battle, sets\nTailwind on first entry.", TREE_EFFECT_TAILWIND_ON_ENTRY, 1, 1, 0)),
        },
    },
    [TYPE_DARK] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Dark"),
                    MOVES_UP("Strong Jaw", "+2% damage of biting\nmoves per rank.", TREE_MOVES_BITE),
                    CONDITION("Opportunist", "+2% damage per rank vs\ntargets below 50% HP.", TREE_EFFECT_DAMAGE_TARGET_LOW_HP),
                    NODE("Moxie", "Knocking out a foe raises\nits best attacking stat.", TREE_EFFECT_MOXIE, 1, 1, 0)),
            DEFENSE(WARD("Fist Ward", "-1.25% damage from Fighting\nmoves per rank.", TREE_TYPES(TYPE_FIGHTING)),
                    WARD2("Silk Ward", "-2.5% damage from Bug\nand Fairy moves per rank.", TREE_TYPES(TYPE_BUG, TYPE_FAIRY)),
                    NODE("Rough Skin", "Foes touching it lose 1.25%\nof their max HP per rank.", TREE_EFFECT_CONTACT_DAMAGE, 10, 125, 0),
                    NODE("Dark Veil", "Once per battle, the first\nstatus move aimed at it fails.", TREE_EFFECT_STATUS_MOVE_BLOCK, 1, 1, 0)),
            UTILITY(FLINCH("Menace", "Dark"),
                    NODE("Thief's Cut", "+5% prize money\nper rank.", TREE_EFFECT_MONEY_BONUS, 10, 500, 0),
                    NODE("Dark Recovery", "Heals 2% of max HP per rank\nwhen it knocks out a foe.", TREE_EFFECT_KO_HEAL, 10, 200, 0),
                    NODE("Intimidate", "Once per battle, lowers foes'\nAttack on first entry.", TREE_EFFECT_STAT_ON_ENTRY, 1, 1, STAT_ATK)),
        },
    },
    [TYPE_FIGHTING] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Fighting"),
                    MOVES_UP("Iron Knuckles", "+2% damage of punching\nmoves per rank.", TREE_MOVES_PUNCH),
                    NODE("Second Breath", "+5% chance per rank its moves\ndon't lower its own stats.", TREE_EFFECT_SELF_DROP_IGNORE, 10, 500, 0),
                    NODE("Unerring Strikes", "Its Fighting moves\nnever miss.", TREE_EFFECT_NEVER_MISS, 1, 1, 0)),
            DEFENSE(WARD("Sky Ward", "-1.25% damage from Flying\nmoves per rank.", TREE_TYPES(TYPE_FLYING)),
                    WARD2("Clarity Ward", "-2.5% damage from Psychic\nand Fairy moves per rank.", TREE_TYPES(TYPE_PSYCHIC, TYPE_FAIRY)),
                    NODE("Toughened", "-1% damage from physical\nmoves per rank.", TREE_EFFECT_CATEGORY_GUARD, 10, 100, TREE_CATEGORY_PHYSICAL),
                    STURDY("Endure")),
            UTILITY(NODE("Pressure Points", "+0.5% chance per rank its\nFighting moves lower Defense.", TREE_EFFECT_ADDED_DEF_DROP, 20, 50, 0),
                    NODE("Iron Resolve", "+5% chance per rank to resist\nflinching and confusion.", TREE_EFFECT_FLINCH_CONFUSION_RESIST, 10, 500, 0),
                    NODE("Guts", "With a status, +3% physical\ndamage per rank.", TREE_EFFECT_PHYSICAL_DAMAGE_STATUSED, 10, 300, 0),
                    NODE("Battle Cry", "Once per battle, lowers foes'\nDefense on first entry.", TREE_EFFECT_STAT_ON_ENTRY, 1, 1, STAT_DEF)),
        },
    },
    [TYPE_DRAGON] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Dragon"),
                    NODE("Draconic Will", "+5% chance per rank its moves\ndon't lower its own stats.", TREE_EFFECT_SELF_DROP_IGNORE, 10, 500, 0),
                    CONDITION("Tyrant", "+2% damage per rank vs\ntargets at full HP.", TREE_EFFECT_DAMAGE_TARGET_FULL_HP),
                    NODE("Dragon Heart", "Each Dragon hit: +10% Dragon\ndamage (x3), costs 5% HP.", TREE_EFFECT_TYPE_DAMAGE_STACK, 1, 1000, TREE_STACK_EVERY_HIT)),
            DEFENSE(WARD("Frost Scales", "-1.25% damage from Ice\nmoves per rank.", TREE_TYPES(TYPE_ICE)),
                    WARD2("Wyrm Scales", "-2.5% damage from Dragon\nand Fairy moves per rank.", TREE_TYPES(TYPE_DRAGON, TYPE_FAIRY)),
                    NODE("Multiscale", "At full HP, -3% damage\ntaken per rank.", TREE_EFFECT_FULL_HP_GUARD, 10, 300, 0),
                    UNBREAKABLE("Unbreakable")),
            UTILITY(NODE("Wyrm's Vigor", "Heals 0.25% of max HP\neach turn per rank.", TREE_EFFECT_END_TURN_HEAL, 20, 30, 0),
                    NODE("Dragon Breath", "+1% paralysis chance of its\nDragon moves per rank.", TREE_EFFECT_STATUS_CHANCE, 10, 100, STATUS1_PARALYSIS),
                    NODE("Self-Mastery", "+5% chance per rank to snap\nout of confusion each turn.", TREE_EFFECT_CONFUSION_SNAP, 10, 500, 0),
                    NODE("Dragon's Presence", "Once per battle, lowers foes'\nSp. Atk on first entry.", TREE_EFFECT_STAT_ON_ENTRY, 1, 1, STAT_SPATK)),
        },
    },
    [TYPE_GHOST] =
    {
        .status = STATUS1_ANY,
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Ghost"),
                    NODE("Hex Mastery", "+2% damage per rank vs\ntargets with any status.", TREE_EFFECT_DAMAGE_VS_STATUS, 10, 200, STATUS1_ANY),
                    NODE("Phantom Ambush", "+3% damage of its first move\nafter entering per rank.", TREE_EFFECT_DAMAGE_AFTER_ENTRY, 10, 300, 0),
                    NODE("Soul Siphon", "Its Ghost moves drain 25%\nof the damage dealt.", TREE_EFFECT_TYPE_DRAIN, 1, 2500, 0)),
            DEFENSE(WARD("Spirit Ward", "-1.25% damage from Ghost\nand Dark moves per rank.", TREE_TYPES(TYPE_GHOST, TYPE_DARK)),
                    NODE("Ethereal", "-1% damage from physical\nmoves per rank.", TREE_EFFECT_CATEGORY_GUARD, 10, 100, TREE_CATEGORY_PHYSICAL),
                    NODE("Cursed Body", "+3% chance per rank to disable\nthe move that hit it.", TREE_EFFECT_DISABLE_ON_HIT, 10, 300, 0),
                    NODE("Last Haunt", "If knocked out by a hit, the\nattacker loses 25% max HP.", TREE_EFFECT_LAST_HAUNT, 1, 2500, 0)),
            UTILITY(NODE("Haunting", "+1% accuracy of its status\nmoves per rank.", TREE_EFFECT_STATUS_MOVE_ACCURACY, 20, 100, 0),
                    NODE("Eerie Aura", "-0.5% accuracy of foes'\nmoves aimed at it per rank.", TREE_EFFECT_FOE_ACCURACY_DOWN, 10, 50, 0),
                    NODE("Night Terrors", "Heals 0.5% of max HP per rank\nwhen its status hurts a foe.", TREE_EFFECT_STATUS_SIPHON, 10, 50, 0),
                    NODE("Pain Split", "Once per battle, below 25%\nHP, splits HP with the foe.", TREE_EFFECT_PAIN_SPLIT_ONCE, 1, 1, 0)),
        },
    },
    [TYPE_NORMAL] =
    {
        .nodes =
        {
            GENERAL,
            OFFENSE(POWER("Normal"),
                    NODE("Brawler's Grit", "With a status, +3% physical\ndamage per rank.", TREE_EFFECT_PHYSICAL_DAMAGE_STATUSED, 10, 300, 0),
                    MOVES_UP("Echo", "+2% damage of sound\nmoves per rank.", TREE_MOVES_SOUND),
                    NODE("Scrappy", "Its Normal and Fighting\nmoves hit Ghost types.", TREE_EFFECT_HIT_GHOST, 1, 1, 0)),
            DEFENSE(WARD("Iron Will", "-1.25% damage from Fighting\nmoves per rank.", TREE_TYPES(TYPE_FIGHTING)),
                    NODE("Thick Hide", "+1.5% max HP\nper rank.", TREE_EFFECT_MAX_HP, 10, 150, 0),
                    NODE("Steady", "+5% chance per rank to ignore\na stat drop from foes.", TREE_EFFECT_STAT_DROP_IGNORE, 10, 500, 0),
                    EMERGENCY("Second Wind")),
            UTILITY(NODE("Serene Spirit", "+2.5% added effect chance of\nits moves per rank (x).", TREE_EFFECT_SECONDARY_MULTIPLIER, 20, 250, 0),
                    NODE("Pickup", "+1% chance per rank to find\nan item after a battle.", TREE_EFFECT_PICKUP, 10, 100, 0),
                    NODE("Fortune", "+5% prize money\nper rank.", TREE_EFFECT_MONEY_BONUS, 10, 500, 0),
                    NODE("Rally Cry", "Once per battle, raises its\nbest attacking stat on entry.", TREE_EFFECT_STAT_ON_ENTRY, 1, 1, TREE_STAT_SELF_BEST)),
        },
    },
};

#undef HEAL_TURN
#undef CONDITION
#undef MOVES_UP
#undef AFFLICT
#undef FLINCH
#undef STURDY
#undef UNBREAKABLE
#undef EMERGENCY
#undef CONTACT
#undef FIELD_TURNS
#undef SE_WARD
#undef WARD2
#undef WARD
#undef LOW_HP
#undef POWER
#undef UTILITY
#undef DEFENSE
#undef OFFENSE
#undef GENERAL
#undef NODE
