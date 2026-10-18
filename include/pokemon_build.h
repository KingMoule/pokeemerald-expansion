#ifndef GUARD_POKEMON_BUILD_H
#define GUARD_POKEMON_BUILD_H

// Accès aux données du mod stockées dans chaque Pokémon (struct PokemonBuild).

u32 GetBoxMonTreeRank(const struct BoxPokemon *boxMon, u32 node);
void SetBoxMonTreeRank(struct BoxPokemon *boxMon, u32 node, u32 rank);
u32 GetBoxMonTreePointsSpent(const struct BoxPokemon *boxMon);
u32 GetBoxMonMoveVariant(const struct BoxPokemon *boxMon, u32 moveSlot);
void SetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 moveSlot, u32 variant);

struct AdditionalEffect;

// Variantes d'attaques : une seule active par attaque, attachée à l'emplacement de l'attaque.
// Elle est remise à zéro quand l'attaque est oubliée ou remplacée, et suit l'attaque quand on la déplace.
enum MoveVariant
{
    MOVE_VARIANT_STANDARD,
    MOVE_VARIANT_FOCUSED,  // Plus de puissance, moins de PP.
    MOVE_VARIANT_THRIFTY,  // Plus de PP, moins de puissance.
    MOVE_VARIANT_VOLATILE, // Plus de chances d'effet secondaire, moins de puissance.
    MOVE_VARIANT_COUNT,
};

void SwapBoxMonMoveVariants(struct BoxPokemon *boxMon, u32 slot1, u32 slot2);
bool32 CanMoveHaveVariant(enum Move move, enum MoveVariant variant);
const u8 *GetMoveVariantName(enum MoveVariant variant);
u32 ApplyMoveVariantToPP(u32 pp, enum MoveVariant variant);
u32 CalculateBoxMonMovePP(const struct BoxPokemon *boxMon, enum Move move, u32 ppBonuses, u32 slot);
u32 CalculateMonMovePP(struct Pokemon *mon, enum Move move, u32 ppBonuses, u32 slot);
bool32 TrySetBoxMonMoveVariant(struct BoxPokemon *boxMon, u32 slot, enum MoveVariant variant);
enum MoveVariant GetBattlerMoveSlotVariant(enum BattlerId battler, u32 slot);
enum MoveVariant GetBattlerMoveVariant(enum BattlerId battler, enum Move move);
u32 CalculateBattlerMovePP(enum BattlerId battler, enum Move move, u32 ppBonuses, u32 slot);
uq4_12_t GetMoveVariantPowerModifier(enum BattlerId battler, enum Move move);
u32 ApplyMoveVariantEffectChance(enum BattlerId battler, const struct AdditionalEffect *additionalEffect, u32 chance);

// Mutations : types effectifs et type portant la pénalité sur les faiblesses.
enum Type GetBoxMonMutatedType(const struct BoxPokemon *boxMon, enum Species species, u32 slot);
enum Type GetBoxMonMutationPenaltyType(const struct BoxPokemon *boxMon, enum Species species);
enum Type GetBattlerMutatedType(enum BattlerId battler, u32 slot);
uq4_12_t ApplyMutationWeaknessPenalty(enum BattlerId battlerDef, enum Type moveType, uq4_12_t modifier);

// Héritage : une mutation passe à l'œuf si le bébé possède naturellement le type remplacé.
enum Type GetBoxMonReplacedType(const struct BoxPokemon *boxMon);
bool32 SetBoxMonMutation(struct BoxPokemon *boxMon, enum Type replacedType, enum Type newType);
void InheritMutation(struct BoxPokemon *egg, struct BoxPokemon *parent1, struct BoxPokemon *parent2);

// Arbres élémentaires.
// Nœuds 0 à 3 : General (commun à tous les types). Nœuds 4 à 7, 8 à 11 et 12 à 15 : les arbres
// Offense, Defense et Utility de l'élément, de quatre nœuds chacun ; chaque nœud exige
// MOD_TREE_PARENT_RANKS rangs dans le précédent (ou son rang maximal s'il est plus bas).
// Le premier nœud d'un arbre exige MOD_TREE_BRANCH_UNLOCK_POINTS points dans General.
#define TREE_GENERAL_NODES     4
#define TREE_BRANCH_COUNT    3
#define TREE_BRANCH_NODES    4

#define TREE_NODE_OFFENSE    (TREE_GENERAL_NODES)
#define TREE_NODE_DEFENSE    (TREE_GENERAL_NODES + TREE_BRANCH_NODES)
#define TREE_NODE_UTILITY    (TREE_GENERAL_NODES + 2 * TREE_BRANCH_NODES)

enum TreeBranch
{
    TREE_BRANCH_OFFENSE,
    TREE_BRANCH_DEFENSE,
    TREE_BRANCH_UTILITY,
};

// Valeurs des nœuds : ‱ par rang (100 = 1 %, 10000 = 100 %) sauf mention contraire. Les chances aussi.
// « Ses attaques du type » = les attaques du type de l'arbre.
enum TreeEffect
{
    TREE_EFFECT_NONE,
    // General
    TREE_EFFECT_PHYSICAL_DAMAGE,      // Dégâts des attaques physiques.
    TREE_EFFECT_SPECIAL_DAMAGE,       // Dégâts des attaques spéciales.
    TREE_EFFECT_MAX_HP,               // PV max.
    TREE_EFFECT_SPEED,                // Vitesse.
    // Offense
    TREE_EFFECT_TYPE_DAMAGE,          // Dégâts de ses attaques du type (param : masque de types, 0 = le type de l'arbre).
    TREE_EFFECT_TYPE_DAMAGE_LOW_HP,   // Dégâts de ses attaques du type sous 50 % de PV.
    TREE_EFFECT_TYPE_DAMAGE_HIGH_HP,  // Dégâts de ses attaques du type au-dessus de 75 % de PV.
    TREE_EFFECT_TYPE_DAMAGE_WEATHER,  // Dégâts sous la météo de l'arbre (param : masque de types, 0 = le type de l'arbre).
    TREE_EFFECT_DAMAGE_VS_STATUS,     // Dégâts contre une cible sous statut (param : STATUS1_*, 0 = le statut de l'arbre).
    TREE_EFFECT_MOVE_FAMILY_DAMAGE,   // Dégâts d'une famille d'attaques (param : enum TreeMoveFamily).
    TREE_EFFECT_SUPER_EFFECTIVE_DAMAGE,   // Dégâts des coups super efficaces.
    TREE_EFFECT_NOT_VERY_EFFECTIVE_DAMAGE, // Dégâts des coups peu efficaces.
    TREE_EFFECT_DAMAGE_MOVING_FIRST,  // Dégâts quand il agit avant la cible.
    TREE_EFFECT_DAMAGE_MOVING_LAST,   // Dégâts quand il agit après la cible.
    TREE_EFFECT_DAMAGE_TARGET_LOW_HP, // Dégâts contre une cible sous 50 % de PV.
    TREE_EFFECT_DAMAGE_TARGET_FULL_HP, // Dégâts contre une cible à PV pleins.
    TREE_EFFECT_DAMAGE_STAT_RAISED,   // Dégâts tant qu'une de ses stats est augmentée.
    TREE_EFFECT_DAMAGE_TARGET_WEAKENED, // Dégâts contre une cible dont l'Attaque ou l'Attaque spéciale est baissée.
    TREE_EFFECT_PHYSICAL_DAMAGE_STATUSED, // Dégâts physiques tant qu'il a un statut.
    TREE_EFFECT_DAMAGE_AFTER_ENTRY,   // Dégâts de sa première attaque après son entrée.
    TREE_EFFECT_RECOIL_REDUCTION,     // Recul en moins (toutes ses attaques).
    TREE_EFFECT_CRIT_DAMAGE,          // Dégâts des coups critiques en plus (5000 : ×1,5 devient ×2).
    TREE_EFFECT_MULTI_HIT_MAX,        // Chance que ses attaques à coups multiples frappent le maximum de fois.
    TREE_EFFECT_DRAIN_BONUS,          // PV absorbés en plus.
    TREE_EFFECT_SELF_DROP_IGNORE,     // Chance que ses attaques ne baissent pas ses propres stats.
    TREE_EFFECT_TYPE_DAMAGE_STACK,    // Maîtrise « Heart » : dégâts par cumul (param : TREE_STACK_*).
    TREE_EFFECT_BERSERK,              // Maîtrise : +1 Attaque et Attaque spéciale la première fois sous 50 % de PV.
    TREE_EFFECT_TYPE_CRIT_STAGE,      // Maîtrise : niveaux de critique de ses attaques du type (valeur = niveaux).
    TREE_EFFECT_CHAIN_DAMAGE,         // Maîtrise : dégâts de ses attaques du type dès la 2e d'affilée.
    TREE_EFFECT_MOMENTUM_DAMAGE,      // Maîtrise : dégâts de sa prochaine attaque du type après avoir été touché.
    TREE_EFFECT_NORMAL_TO_TYPE,       // Maîtrise : ses attaques Normal prennent le type de l'arbre, + dégâts.
    TREE_EFFECT_HIT_GHOST,            // Maîtrise : ses attaques Normal et Fighting touchent les Ghost.
    TREE_EFFECT_HIT_AIRBORNE,         // Maîtrise : ses attaques Ground touchent les Pokémon en l'air.
    TREE_EFFECT_NEVER_MISS,           // Maîtrise : ses attaques du type ne ratent jamais.
    TREE_EFFECT_PRIORITY_FULL_HP,     // Maîtrise : +1 de priorité à ses attaques du type à PV pleins.
    TREE_EFFECT_MERCILESS,            // Maîtrise : toujours critique contre une cible empoisonnée.
    TREE_EFFECT_MOXIE,                // Maîtrise : +1 à sa meilleure stat offensive quand il met un adversaire K.O.
    TREE_EFFECT_TYPE_DRAIN,           // Maîtrise : ses attaques du type absorbent une part des dégâts.
    // Defense
    TREE_EFFECT_WARD,                 // Dégâts subis en moins (param : masque des types d'attaque).
    TREE_EFFECT_WEAKNESS_GUARD,       // Dégâts subis en moins contre les attaques super efficaces.
    TREE_EFFECT_CATEGORY_GUARD,       // Dégâts subis en moins (param : TREE_CATEGORY_*).
    TREE_EFFECT_FULL_HP_GUARD,        // Dégâts subis en moins à PV pleins.
    TREE_EFFECT_WEATHER_GUARD,        // Dégâts subis en moins sous la météo de l'arbre.
    TREE_EFFECT_INDIRECT_GUARD,       // Dégâts indirects en moins (statut, pièges, météo).
    TREE_EFFECT_CONTACT_STATUS,       // Chance d'infliger un statut à qui l'attaque au contact (param : STATUS1_*).
    TREE_EFFECT_CONTACT_WEAKEN,       // Chance de baisser l'Attaque de qui l'attaque au contact.
    TREE_EFFECT_CONTACT_DAMAGE,       // Qui l'attaque au contact perd cette part de ses PV max.
    TREE_EFFECT_STAT_DROP_IGNORE,     // Chance d'ignorer une baisse de stat infligée par l'adversaire.
    TREE_EFFECT_STATUS_CURE_END_TURN, // Chance de guérir son statut en fin de tour (param : météo B_WEATHER_*, 0 = toujours).
    TREE_EFFECT_STATUS_CURE_SWITCH,   // Chance de guérir son statut en sortant.
    TREE_EFFECT_STATUS_AVOID,         // Chance d'éviter un statut infligé par l'adversaire.
    TREE_EFFECT_SWITCH_OUT_HEAL,      // PV max récupérés en sortant.
    TREE_EFFECT_DISABLE_ON_HIT,       // Chance de bloquer l'attaque qui l'a touché.
    TREE_EFFECT_FIRST_HIT_GUARD,      // Maîtrise : premier coup amorti une fois par combat (param : TREE_GUARD_*).
    TREE_EFFECT_EMERGENCY_HEAL,       // Maîtrise : une fois par combat, sous 33 % de PV, soigne cette part des PV max.
    TREE_EFFECT_ENDURE,               // Maîtrise : une fois par combat, à PV pleins, survit à un coup fatal.
    TREE_EFFECT_GROUND_IMMUNITY_ONCE, // Maîtrise : la première attaque Ground qui le vise échoue.
    TREE_EFFECT_AQUA_RING_ONCE,       // Maîtrise : Aqua Ring la première fois sous 50 % de PV.
    TREE_EFFECT_MOLT,                 // Maîtrise : une fois par combat, efface ses baisses de stats et son statut.
    TREE_EFFECT_TOXIC_DEBRIS,         // Maîtrise : le premier coup physique subi pose Toxic Spikes chez l'adversaire.
    TREE_EFFECT_SYNCHRONIZE,          // Maîtrise : renvoie le statut qu'un adversaire lui inflige.
    TREE_EFFECT_STATUS_MOVE_BLOCK,    // Maîtrise : la première attaque de statut adverse qui le vise échoue.
    TREE_EFFECT_LAST_HAUNT,           // Maîtrise : mis K.O. par un coup direct, l'attaquant perd cette part de ses PV max.
    // Utility
    TREE_EFFECT_STATUS_CHANCE,        // Chance d'infliger un statut avec ses attaques qui le peuvent (param : STATUS1_*, 0 = celui de l'arbre).
    TREE_EFFECT_STATUS_DAMAGE,        // Dégâts résiduels en plus du statut de l'arbre qu'il a infligé.
    TREE_EFFECT_SECONDARY_CHANCE,     // Chance d'effet secondaire de ses attaques du type (param : TREE_SECONDARY_*).
    TREE_EFFECT_SECONDARY_MULTIPLIER, // Chances d'effet secondaire de toutes ses attaques, en multiplicateur.
    TREE_EFFECT_ADDED_DEF_DROP,       // Chance que ses attaques du type baissent la Défense de la cible.
    TREE_EFFECT_FIELD_TURNS,          // Tours en plus pour ce qu'il pose (valeur = tours ; param : TREE_FIELD_*).
    TREE_EFFECT_FIELD_SPEED,          // Vitesse sous la météo ou sur le terrain de l'arbre.
    TREE_EFFECT_WEATHER_HEAL,         // PV max récupérés en fin de tour sous la météo de l'arbre.
    TREE_EFFECT_END_TURN_HEAL,        // PV max récupérés en fin de tour.
    TREE_EFFECT_STATUS_MOVE_ACCURACY, // Précision en plus de ses attaques de statut (en points : 100 = +1).
    TREE_EFFECT_ACCURACY,             // Précision de toutes ses attaques, en multiplicateur.
    TREE_EFFECT_FOE_ACCURACY_DOWN,    // Précision des attaques adverses qui le visent, en moins.
    TREE_EFFECT_FLINCH_CONFUSION_RESIST, // Chance de résister à la peur et à la confusion.
    TREE_EFFECT_CONFUSION_SNAP,       // Chance de sortir de la confusion à chaque tour.
    TREE_EFFECT_HEALING_MOVE_BOOST,   // Soins de ses attaques de soin en plus.
    TREE_EFFECT_STATUS_SIPHON,        // PV max récupérés quand une cible qu'il a affligée subit son statut.
    TREE_EFFECT_KO_HEAL,              // PV max récupérés quand il met un adversaire K.O.
    TREE_EFFECT_MONEY_BONUS,          // Argent gagné en combat en plus.
    TREE_EFFECT_PICKUP,               // Chance de trouver un objet après un combat.
    TREE_EFFECT_WEATHER_ON_ENTRY,     // Maîtrise : la météo de l'arbre à la première entrée.
    TREE_EFFECT_TERRAIN_ON_ENTRY,     // Maîtrise : le terrain de l'arbre à la première entrée.
    TREE_EFFECT_HAZARD_ON_ENTRY,      // Maîtrise : un piège chez l'adversaire à la première entrée (param : TREE_HAZARD_*).
    TREE_EFFECT_TAILWIND_ON_ENTRY,    // Maîtrise : Tailwind à la première entrée.
    TREE_EFFECT_SCREEN_ON_ENTRY,      // Maîtrise : Reflect à la première entrée.
    TREE_EFFECT_STAT_ON_ENTRY,        // Maîtrise : à la première entrée, baisse une stat des adversaires (param : STAT_*)
                                      // ou, avec TREE_STAT_SELF_BEST, augmente sa meilleure stat offensive.
    TREE_EFFECT_PAIN_SPLIT_ONCE,      // Maîtrise : une fois par combat, sous 25 % de PV, Pain Split avec l'adversaire en face.
    TREE_EFFECT_COUNT,
};

enum TreeMoveFamily
{
    TREE_MOVES_PUNCH,
    TREE_MOVES_BITE,
    TREE_MOVES_SOUND,
    TREE_MOVES_WIND,
};

#define TREE_STACK_EVERY_HIT   0 // Heart : chaque coup du type, contre des PV.
#define TREE_STACK_DRAIN       1 // Sap Surge : chaque coup qui absorbe des PV, sans coût.

#define TREE_CATEGORY_PHYSICAL 0
#define TREE_CATEGORY_SPECIAL  1

#define TREE_GUARD_SUPER_EFFECTIVE 0          // Premier coup super efficace.
#define TREE_GUARD_PHYSICAL        (1u << 31) // Premier coup physique.
// Sinon : masque des types d'attaque.

enum TreeSecondary
{
    TREE_SECONDARY_ANY,
    TREE_SECONDARY_FLINCH,
    TREE_SECONDARY_CONFUSION,
};

enum TreeField
{
    TREE_FIELD_WEATHER,
    TREE_FIELD_TERRAIN,
    TREE_FIELD_TAILWIND,
    TREE_FIELD_SCREENS,
};

enum TreeHazard
{
    TREE_HAZARD_STEALTH_ROCK,
    TREE_HAZARD_TOXIC_SPIKES,
    TREE_HAZARD_STICKY_WEB,
};

#define TREE_STAT_SELF_BEST 0xFF

#define TREE_TYPES(...) (TREE_TYPES_(__VA_ARGS__, TYPE_NONE, TYPE_NONE, TYPE_NONE))
#define TREE_TYPES_(a, b, c, ...) ((1u << (a)) | ((b) != TYPE_NONE ? 1u << (b) : 0) | ((c) != TYPE_NONE ? 1u << (c) : 0))

struct TreeNode
{
    const u8 *name;
    const u8 *description;
    enum TreeEffect effect;
    u8 maxRank; // 0 = nœud absent.
    u16 valuePerRank;
    u32 param;
};

struct ElementTree
{
    u32 status;  // Statut associé à l'élément (STATUS1_*), 0 si aucun.
    u32 weather; // Météo associée à l'élément (B_WEATHER_*), 0 si aucune.
    enum BattleTerrain terrain; // Terrain associé à l'élément (B_TERRAIN_*), B_TERRAIN_NONE si aucun.
    struct TreeNode nodes[MOD_TREE_NODE_COUNT];
};

extern const u8 *const gTreeBranchNames[TREE_BRANCH_COUNT];
extern const u8 gTreeGeneralName[];

enum TreeInvestResult
{
    TREE_INVEST_OK,
    TREE_INVEST_WRONG_TREE,    // L'élément n'est pas un type du Pokémon, ou un autre arbre est déjà suivi.
    TREE_INVEST_NO_NODE,       // Ce nœud n'existe pas dans cet arbre.
    TREE_INVEST_MAX_RANK,
    TREE_INVEST_NO_POINTS,
    TREE_INVEST_LOCKED,        // Prérequis non remplis.
};

extern const struct ElementTree gElementTrees[NUMBER_OF_MON_TYPES];

const struct TreeNode *GetTreeNode(enum Type treeType, u32 node);
u32 GetBoxMonTreePointsEarned(struct BoxPokemon *boxMon);
u32 GetBoxMonTreePointsAvailable(struct BoxPokemon *boxMon);
enum TreeInvestResult CanBoxMonInvestInNode(struct BoxPokemon *boxMon, enum Type treeType, u32 node);
enum TreeInvestResult TryBoxMonInvestInNode(struct BoxPokemon *boxMon, enum Type treeType, u32 node);
bool32 CanBoxMonAcquireMutation(const struct BoxPokemon *boxMon);
bool32 IsMutationTargetType(enum Type type);
bool32 IsMutationTypeOpen(enum Type type);
void OpenMutationType(enum Type type);
u32 GetBoxMonTreeEffect(const struct BoxPokemon *boxMon, enum TreeEffect effect);
u32 GetBattlerTreeEffect(enum BattlerId battler, enum TreeEffect effect);
const struct ElementTree *GetBattlerTree(enum BattlerId battler);

// Effets des arbres en combat.
struct DamageContext;
uq4_12_t GetTreeDamageModifier(struct DamageContext *ctx);
uq4_12_t GetTreeDefenseModifier(struct DamageContext *ctx);
void UpdateTreeFirstHitGuard(enum BattlerId battlerAtk, enum Move move);
u32 ApplyTreeRecoilReduction(enum BattlerId battlerAtk, enum Type moveType, u32 recoil);
u32 GetTreeStatusMoveEffect(u32 status);
u32 GetTreeSecondaryChanceBonus(enum BattlerId battlerAtk, enum Move move, u32 moveEffect);
u32 ApplyTreeSecondaryMultiplier(enum BattlerId battlerAtk, u32 chance);
u32 GetTreeWeatherTurnsBonus(enum BattlerId battler, u32 weatherFlag);
u32 GetTreeTerrainTurnsBonus(enum BattlerId battler, enum BattleTerrain terrain);
u32 GetTreeTailwindTurnsBonus(enum BattlerId battler);
u32 GetTreeScreenTurnsBonus(enum BattlerId battler);
u32 GetTreeWeatherHealAmount(enum BattlerId battler);
void ResetTreeBattleState(void);
void ResetTreeOnSwitchIn(enum BattlerId battler);
u32 GetTreeHeartStacks(enum BattlerId battler);
bool32 TryTreeHeartActivation(enum BattlerId battlerAtk, enum Move move);
void UpdateTreeAfterMove(enum BattlerId battlerAtk, enum Move move);
void UpdateTreeTargetsHit(enum BattlerId battlerAtk);
const u8 *TryTreeEntryEffect(enum BattlerId battler);
u32 ApplyTreeAccuracy(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, u32 calc);
bool32 TreeMoveNeverMisses(enum BattlerId battlerAtk, enum Move move);
u32 ApplyTreeFieldSpeed(enum BattlerId battler, u32 speed, u32 weather);
bool32 TreeGivesPriority(enum BattlerId battler, enum Move move);
enum Type GetTreeAteType(enum BattlerId battler);
bool32 TreeMoveIgnoresImmunity(enum BattlerId battlerAtk, enum Type moveType, enum Type defType);
bool32 IsTreeGroundImmune(enum BattlerId battlerDef);
void SpendTreeGroundImmunity(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Type moveType);
bool32 TryTreeChanceEffect(enum BattlerId battler, enum TreeEffect effect, u32 tag);
bool32 TryTreeMaxMultiHit(enum BattlerId battlerAtk);
bool32 TryTreeResistVolatile(enum BattlerId battlerAtk, enum BattlerId battlerDef);
s32 ApplyTreeDrainBonus(enum BattlerId battlerAtk, s32 heal);
u32 ApplyTreeHealingMoveBoost(enum BattlerId battler, u32 heal);
u32 ApplyTreeIndirectGuard(enum BattlerId battler, u32 damage);
void SetTreeNodeNameForEffect(enum BattlerId battler, enum TreeEffect effect);
bool32 TreeHasSynchronize(enum BattlerId battler, enum BattlerId battlerAtk);
void SetTreeSynchronizePending(bool32 pending);
bool32 IsTreeSynchronizePending(void);
bool32 TryTreeAvoidStatus(enum BattlerId battlerAtk, enum BattlerId battlerDef);
bool32 TryTreeEndure(enum BattlerId battlerDef);
bool32 TryTreeStatusMoveBlock(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move);
void TryTreeSwitchOutEffects(enum BattlerId battler);
u32 ApplyTreeMoneyBonus(u32 money);
bool32 TryTreePickup(struct BoxPokemon *boxMon);
struct BattleCalcValues;
const u8 *TryTreeMoveEndEffect(struct BattleCalcValues *cv);
const u8 *TryTreeFoesFaintedEffect(struct BattleCalcValues *cv);
const u8 *TryTreeEndTurnEffect(enum BattlerId battler);
s32 GetTreeCritStageBonus(enum BattlerId battlerAtk, enum Move move, enum Type moveType);
bool32 TreeAlwaysCrits(enum BattlerId battlerAtk, enum BattlerId battlerDef);
void RecordTreeStatusInflicted(enum BattlerId battlerAtk, enum BattlerId battlerDef);
u32 ApplyTreeStatusDamageBonus(enum BattlerId battler, u32 status, u32 damage);

#endif // GUARD_POKEMON_BUILD_H
