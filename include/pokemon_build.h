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
// Nœuds 0 à 3 : tronc commun. Nœuds 4 à 7, 8 à 11 et 12 à 15 : trois branches
// de quatre nœuds, chacun exigeant MOD_TREE_PARENT_RANKS rangs dans le précédent.
// Le premier nœud d'une branche exige MOD_TREE_BRANCH_UNLOCK_POINTS points dans le tronc.
#define TREE_TRUNK_NODES     4
#define TREE_BRANCH_COUNT    3
#define TREE_BRANCH_NODES    4

enum TreeEffect
{
    TREE_EFFECT_NONE,
    TREE_EFFECT_PHYSICAL_DAMAGE,      // ‰ de dégâts des attaques physiques.
    TREE_EFFECT_SPECIAL_DAMAGE,       // ‰ de dégâts des attaques spéciales.
    TREE_EFFECT_MAX_HP,               // ‰ de PV max.
    TREE_EFFECT_SPEED,                // ‰ de Vitesse.
    TREE_EFFECT_TYPE_PHYSICAL_DAMAGE, // ‰ de dégâts des attaques physiques de l'élément.
    TREE_EFFECT_TYPE_RECOIL,          // ‰ de recul en moins pour les attaques de l'élément.
    TREE_EFFECT_TYPE_DAMAGE_LOW_HP,   // ‰ de dégâts de l'élément sous 50 % des PV.
    TREE_EFFECT_STATUS_CHANCE,        // ‰ (en points) de chance d'infliger le statut de l'élément.
    TREE_EFFECT_DAMAGE_VS_STATUS,     // ‰ de dégâts contre une cible portant le statut de l'élément.
    TREE_EFFECT_TYPE_DAMAGE_WEATHER,  // ‰ de dégâts de l'élément sous la météo de l'élément.
    TREE_EFFECT_WEATHER_TURNS,        // Tours de météo en plus quand ce Pokémon l'invoque.
    TREE_EFFECT_WEATHER_HEAL,         // ‰ des PV max récupérés en fin de tour sous la météo.
    TREE_EFFECT_TYPE_SPECIAL_DAMAGE,  // ‰ de dégâts des attaques spéciales de l'élément.
    TREE_EFFECT_WEAKNESS_GUARD,       // ‰ de dégâts en moins contre les attaques super efficaces.
    TREE_EFFECT_TYPE_DAMAGE_STACK,    // ‰ de dégâts de l'élément par cumul, gagné après chaque attaque de l'élément.
    TREE_EFFECT_WEATHER_ON_ENTRY,     // Invoque la météo de l'élément à la première entrée.
    // Effets dont les règles restent à préciser dans le wiki : sans effet pour l'instant.
    TREE_EFFECT_STATUS_DAMAGE,
    TREE_EFFECT_LINGERING_STATUS,
    TREE_EFFECT_INACTIVE_MASTERY,     // Maîtrise générique des autres éléments.
};

struct TreeNode
{
    const u8 *name;
    const u8 *description;
    enum TreeEffect effect;
    u8 maxRank; // 0 = nœud absent.
    u16 valuePerRank;
};

struct ElementTree
{
    const u8 *branchNames[TREE_BRANCH_COUNT];
    u32 status;  // Statut associé à l'élément (STATUS1_*), 0 si aucun.
    u32 weather; // Météo associée à l'élément (B_WEATHER_*), 0 si aucune.
    struct TreeNode nodes[MOD_TREE_NODE_COUNT];
};

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
u32 GetBoxMonTreeEffect(const struct BoxPokemon *boxMon, enum TreeEffect effect);
u32 GetBattlerTreeEffect(enum BattlerId battler, enum TreeEffect effect);
const struct ElementTree *GetBattlerTree(enum BattlerId battler);

// Effets des arbres en combat.
uq4_12_t GetTreeDamageModifier(enum BattlerId battlerAtk, enum BattlerId battlerDef, enum Move move, enum Type moveType);
uq4_12_t GetTreeGuardModifier(enum BattlerId battlerDef, uq4_12_t typeEffectiveness);
u32 ApplyTreeRecoilReduction(enum BattlerId battlerAtk, enum Type moveType, u32 recoil);
u32 GetTreeStatusMoveEffect(u32 status);
u32 GetTreeStatusChanceBonus(enum BattlerId battlerAtk, u32 moveEffect);
u32 GetTreeWeatherTurnsBonus(enum BattlerId battler, u32 weatherFlag);
u32 GetTreeWeatherHealAmount(enum BattlerId battler);
void ResetTreeBattleState(void);
void ResetTreeHeartStacks(enum BattlerId battler);
u32 GetTreeHeartStacks(enum BattlerId battler);
bool32 TryTreeHeartActivation(enum BattlerId battlerAtk, enum Move move);
bool32 TryTreeWeatherOnEntry(enum BattlerId battler);

#endif // GUARD_POKEMON_BUILD_H
