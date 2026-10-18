#include "global.h"
#include "battle_main.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "script_menu.h"
#include "string_util.h"
#include "text.h"
#include "tree_menu.h"

// Écran d'investissement des points d'arbre, ouvert depuis le menu d'équipe.
// Le script EventScript_TreeMenu enchaîne ces specials :
//   VAR_0x8004 : emplacement du Pokémon dans l'équipe.
//   VAR_0x8005 : élément de l'arbre affiché.
//   VAR_0x8006 : dernier nœud choisi (position du curseur).

#define sMonSlot  gSpecialVar_0x8004
#define sTreeType gSpecialVar_0x8005
#define sNode     gSpecialVar_0x8006

enum
{
    TREE_MENU_EXIT,
    TREE_MENU_READY,
    TREE_MENU_CHOOSE_TREE,
};

static struct BoxPokemon *GetTreeMenuBoxMon(void)
{
    return &gParties[B_TRAINER_PLAYER][sMonSlot].box;
}

static void PushTreeMenuItem(const u8 *text, u32 id)
{
    struct ListMenuItem item;
    u8 *name = Alloc(100);

    StringCopy(name, text);
    item.name = name;
    item.id = id;
    MultichoiceDynamic_PushElement(item);
}

// VAR_RESULT : TREE_MENU_READY si l'arbre est connu (déjà suivi ou un seul type),
// TREE_MENU_CHOOSE_TREE si le joueur doit choisir entre les deux types.
void TreeMenu_Begin(void)
{
    struct BoxPokemon *boxMon;
    enum Species species;
    enum Type type1, type2;

    gSpecialVar_Result = TREE_MENU_EXIT;
    sNode = 0;
    if (sMonSlot >= PARTY_SIZE)
        return;
    boxMon = GetTreeMenuBoxMon();
    species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    if (species == SPECIES_NONE || GetBoxMonData(boxMon, MON_DATA_IS_EGG))
        return;

    GetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar1);
    if (boxMon->build.treeType != TYPE_NONE)
    {
        sTreeType = boxMon->build.treeType;
        gSpecialVar_Result = TREE_MENU_READY;
        return;
    }

    type1 = GetBoxMonMutatedType(boxMon, species, 0);
    type2 = GetBoxMonMutatedType(boxMon, species, 1);
    sTreeType = type1;
    gSpecialVar_Result = (type1 == type2) ? TREE_MENU_READY : TREE_MENU_CHOOSE_TREE;
}

void TreeMenu_PushTypeChoices(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    enum Type type1 = GetBoxMonMutatedType(boxMon, species, 0);
    enum Type type2 = GetBoxMonMutatedType(boxMon, species, 1);

    PushTreeMenuItem(gTypesInfo[type1].name, type1);
    PushTreeMenuItem(gTypesInfo[type2].name, type2);
}

// STR_VAR_2 : nom de l'arbre. STR_VAR_3 : points disponibles.
void TreeMenu_BufferHeader(void)
{
    StringCopy(gStringVar2, gTypesInfo[sTreeType].name);
    ConvertIntToDecimalStringN(gStringVar3, GetBoxMonTreePointsAvailable(GetTreeMenuBoxMon()), STR_CONV_MODE_LEFT_ALIGN, 3);
}

// Une ligne par nœud existant : « Nom rang/max », précédée d'un tiret si le nœud est verrouillé.
void TreeMenu_PushNodes(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    u32 node;
    u8 text[64];

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        const struct TreeNode *treeNode = GetTreeNode(sTreeType, node);
        u8 *end = text;

        if (treeNode == NULL)
            continue;
        if (CanBoxMonInvestInNode(boxMon, sTreeType, node) == TREE_INVEST_LOCKED)
            end = StringCopy(end, COMPOUND_STRING("- "));
        end = StringCopy(end, treeNode->name);
        *end++ = CHAR_SPACE;
        end = ConvertIntToDecimalStringN(end, GetBoxMonTreeRank(boxMon, node), STR_CONV_MODE_LEFT_ALIGN, 2);
        *end++ = CHAR_SLASH;
        ConvertIntToDecimalStringN(end, treeNode->maxRank, STR_CONV_MODE_LEFT_ALIGN, 2);
        PushTreeMenuItem(text, node);
    }
}

// gStringVar4 : description du nœud, puis la question ou la raison du refus.
// VAR_RESULT : TREE_INVEST_OK ou la raison du refus (enum TreeInvestResult).
void TreeMenu_CheckNode(void)
{
    static const u8 *const sRefusals[] =
    {
        [TREE_INVEST_OK]          = COMPOUND_STRING("Invest 1 point?\nThis is permanent."),
        [TREE_INVEST_WRONG_TREE]  = COMPOUND_STRING("This Pokémon already\nfollows another tree."),
        [TREE_INVEST_NO_NODE]     = COMPOUND_STRING("This node doesn't exist."),
        [TREE_INVEST_MAX_RANK]    = COMPOUND_STRING("This node is already\nat max rank."),
        [TREE_INVEST_NO_POINTS]   = COMPOUND_STRING("No tree points\navailable."),
        [TREE_INVEST_LOCKED]      = COMPOUND_STRING("This node is\nstill locked."),
    };
    const struct TreeNode *treeNode = GetTreeNode(sTreeType, sNode);
    enum TreeInvestResult result = CanBoxMonInvestInNode(GetTreeMenuBoxMon(), sTreeType, sNode);
    u8 *end = gStringVar4;

    if (treeNode != NULL)
    {
        if (sNode >= TREE_TRUNK_NODES)
        {
            end = StringCopy(end, gElementTrees[sTreeType].branchNames[(sNode - TREE_TRUNK_NODES) / TREE_BRANCH_NODES]);
            end = StringCopy(end, COMPOUND_STRING(":"));
            *end++ = CHAR_NEWLINE;
        }
        end = StringCopy(end, treeNode->name);
        *end++ = CHAR_PROMPT_CLEAR;
        end = StringCopy(end, treeNode->description);
        *end++ = CHAR_PROMPT_CLEAR;
    }
    // Nœud verrouillé : dire ce qui l'ouvre.
    if (result == TREE_INVEST_LOCKED && (sNode - TREE_TRUNK_NODES) % TREE_BRANCH_NODES == 0)
    {
        end = StringCopy(end, COMPOUND_STRING("Locked. Needs "));
        end = ConvertIntToDecimalStringN(end, MOD_TREE_BRANCH_UNLOCK_POINTS, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringCopy(end, COMPOUND_STRING(" points\nin the first four nodes."));
    }
    else if (result == TREE_INVEST_LOCKED)
    {
        const struct TreeNode *parent = GetTreeNode(sTreeType, sNode - 1);
        end = StringCopy(end, COMPOUND_STRING("Locked. Needs "));
        end = StringCopy(end, parent->name);
        *end++ = CHAR_NEWLINE;
        end = StringCopy(end, COMPOUND_STRING("at rank "));
        end = ConvertIntToDecimalStringN(end, min(MOD_TREE_PARENT_RANKS, parent->maxRank), STR_CONV_MODE_LEFT_ALIGN, 2);
        StringCopy(end, COMPOUND_STRING(" first."));
    }
    else
    {
        StringCopy(end, sRefusals[result]);
    }
    gSpecialVar_Result = result;
}

void TreeMenu_Invest(void)
{
    if (TryBoxMonInvestInNode(GetTreeMenuBoxMon(), sTreeType, sNode) == TREE_INVEST_OK)
        CalculateMonStats(&gParties[B_TRAINER_PLAYER][sMonSlot]);
}

// Variantes d'attaques, depuis le même écran BUILD.
//   VAR_0x8007 : emplacement de l'attaque choisie.
//   VAR_0x8008 : variante choisie.

#define sMoveSlot gSpecialVar_0x8007
#define sVariant  gSpecialVar_0x8008

enum
{
    VARIANT_MENU_OK,
    VARIANT_MENU_ALREADY_SET,
    VARIANT_MENU_NO_VARIANT,
};

static enum Move GetVariantMenuMove(void)
{
    return GetBoxMonData(GetTreeMenuBoxMon(), MON_DATA_MOVE1 + sMoveSlot);
}

// VAR_RESULT : TRUE si le Pokémon peut ouvrir l'écran BUILD (ni œuf ni emplacement vide).
void BuildMenu_Begin(void)
{
    struct BoxPokemon *boxMon;

    gSpecialVar_Result = FALSE;
    if (sMonSlot >= PARTY_SIZE)
        return;
    boxMon = GetTreeMenuBoxMon();
    if (GetBoxMonData(boxMon, MON_DATA_SPECIES) == SPECIES_NONE || GetBoxMonData(boxMon, MON_DATA_IS_EGG))
        return;
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, gStringVar1);
    sMoveSlot = 0;
    gSpecialVar_Result = TRUE;
}

static u8 *AppendVariantStats(u8 *end, enum MoveVariant variant)
{
    u32 power, other;

    switch (variant)
    {
    case MOVE_VARIANT_FOCUSED:
        power = MOD_VARIANT_FOCUSED_POWER;
        other = MOD_VARIANT_FOCUSED_PP;
        break;
    case MOVE_VARIANT_THRIFTY:
        power = MOD_VARIANT_THRIFTY_POWER;
        other = MOD_VARIANT_THRIFTY_PP;
        break;
    case MOVE_VARIANT_VOLATILE:
        power = MOD_VARIANT_VOLATILE_POWER;
        other = MOD_VARIANT_VOLATILE_CHANCE;
        break;
    default:
        return end;
    }
    end = StringCopy(end, COMPOUND_STRING("Power "));
    end = ConvertIntToDecimalStringN(end, power, STR_CONV_MODE_LEFT_ALIGN, 3);
    end = StringCopy(end, variant == MOVE_VARIANT_VOLATILE ? COMPOUND_STRING("%,\nadded effect chance ") : COMPOUND_STRING("%,\nPP "));
    end = ConvertIntToDecimalStringN(end, other, STR_CONV_MODE_LEFT_ALIGN, 3);
    return StringCopy(end, COMPOUND_STRING("%."));
}

// Une ligne par attaque : « Nom » ou « Nom (Variante) ».
void VariantMenu_PushMoves(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    u32 slot;
    u8 text[64];

    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        enum Move move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + slot);
        enum MoveVariant variant = GetBoxMonMoveVariant(boxMon, slot);
        u8 *end;

        if (move == MOVE_NONE)
            continue;
        end = StringCopy(text, GetMoveName(move));
        if (variant != MOVE_VARIANT_STANDARD)
        {
            end = StringCopy(end, COMPOUND_STRING(" ("));
            end = StringCopy(end, GetMoveVariantName(variant));
            StringCopy(end, COMPOUND_STRING(")"));
        }
        PushTreeMenuItem(text, slot);
    }
}

// VAR_RESULT : VARIANT_MENU_OK, ou la raison du refus avec son message dans gStringVar4.
void VariantMenu_CheckMove(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    enum Move move = GetVariantMenuMove();
    enum MoveVariant variant = GetBoxMonMoveVariant(boxMon, sMoveSlot);
    u8 *end = StringCopy(gStringVar4, GetMoveName(move));

    if (variant != MOVE_VARIANT_STANDARD)
    {
        end = StringCopy(end, COMPOUND_STRING(" uses the "));
        end = StringCopy(end, GetMoveVariantName(variant));
        end = StringCopy(end, COMPOUND_STRING(" variant.\n"));
        end = AppendVariantStats(end, variant);
        gSpecialVar_Result = VARIANT_MENU_ALREADY_SET;
    }
    else if (!CanMoveHaveVariant(move, MOVE_VARIANT_FOCUSED))
    {
        StringCopy(end, COMPOUND_STRING(" has no variants."));
        gSpecialVar_Result = VARIANT_MENU_NO_VARIANT;
    }
    else
    {
        gSpecialVar_Result = VARIANT_MENU_OK;
    }
}

void VariantMenu_PushVariants(void)
{
    enum Move move = GetVariantMenuMove();
    enum MoveVariant variant;

    for (variant = MOVE_VARIANT_STANDARD + 1; variant < MOVE_VARIANT_COUNT; variant++)
    {
        if (CanMoveHaveVariant(move, variant))
            PushTreeMenuItem(GetMoveVariantName(variant), variant);
    }
}

// gStringVar4 : effet de la variante, puis la question de confirmation.
void VariantMenu_BufferVariant(void)
{
    u8 *end = StringCopy(gStringVar4, GetMoveVariantName(sVariant));

    end = StringCopy(end, COMPOUND_STRING(": "));
    end = AppendVariantStats(end, sVariant);
    *end++ = CHAR_PROMPT_CLEAR;
    end = StringCopy(end, COMPOUND_STRING("Give this variant to\n"));
    end = StringCopy(end, GetMoveName(GetVariantMenuMove()));
    StringCopy(end, COMPOUND_STRING("? This is permanent."));
}

void VariantMenu_Apply(void)
{
    TrySetBoxMonMoveVariant(GetTreeMenuBoxMon(), sMoveSlot, sVariant);
}

// Mutation volontaire, depuis le même écran BUILD : 50 points investis et une maîtrise finale
// (CanBoxMonAcquireMutation). Seuls les types ouverts sont accessibles ; un type encore scellé
// demande un Mutation Seal, consommé, qui l'ouvre pour toute la sauvegarde.
//   VAR_0x8006 : type d'origine remplacé.
//   VAR_0x8007 : nouveau type.

#define sReplacedType gSpecialVar_0x8006
#define sNewType      gSpecialVar_0x8007

enum
{
    MUTATION_MENU_REFUSED,
    MUTATION_MENU_READY,
    MUTATION_MENU_CHOOSE_REPLACED,
};

static const u8 sText_MutationSealed[] = _(" (sealed)");

// VAR_RESULT : MUTATION_MENU_READY (type remplacé connu), MUTATION_MENU_CHOOSE_REPLACED (double type)
// ou MUTATION_MENU_REFUSED avec la raison dans gStringVar4.
void MutationMenu_Begin(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);
    enum Type type1 = GetSpeciesType(species, 0);
    enum Type type2 = GetSpeciesType(species, 1);

    gSpecialVar_Result = MUTATION_MENU_REFUSED;
    if (boxMon->build.mutationType != TYPE_NONE)
    {
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("{STR_VAR_1} has already mutated.\nA mutation is forever."));
        return;
    }
    if (!CanBoxMonAcquireMutation(boxMon))
    {
        ConvertIntToDecimalStringN(gStringVar2, MOD_MUTATION_TREE_POINTS, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("To mutate, {STR_VAR_1} needs\n{STR_VAR_2} points in its Tree…\pand a final mastery in one of\nits branches."));
        return;
    }
    sReplacedType = type1;
    gSpecialVar_Result = (type1 == type2) ? MUTATION_MENU_READY : MUTATION_MENU_CHOOSE_REPLACED;
}

void MutationMenu_PushReplacedTypes(void)
{
    enum Species species = GetBoxMonData(GetTreeMenuBoxMon(), MON_DATA_SPECIES);

    PushTreeMenuItem(gTypesInfo[GetSpeciesType(species, 0)].name, GetSpeciesType(species, 0));
    PushTreeMenuItem(gTypesInfo[GetSpeciesType(species, 1)].name, GetSpeciesType(species, 1));
}

// Tous les types réels sauf ceux de l'espèce, les types encore scellés marqués comme tels.
void MutationMenu_PushNewTypes(void)
{
    enum Species species = GetBoxMonData(GetTreeMenuBoxMon(), MON_DATA_SPECIES);
    enum Type type;
    u32 pass;
    u8 text[32];

    // Types ouverts d'abord, puis les types scellés.
    for (pass = 0; pass < 2; pass++)
    {
        for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
        {
            if (!IsMutationTargetType(type) || type == GetSpeciesType(species, 0) || type == GetSpeciesType(species, 1))
                continue;
            if ((IsMutationTypeOpen(type) ? 0 : 1) != pass)
                continue;
            StringCopy(text, gTypesInfo[type].name);
            if (pass == 1)
                StringAppend(text, sText_MutationSealed);
            PushTreeMenuItem(text, type);
        }
    }
}

// VAR_RESULT : TRUE si la mutation est possible, gStringVar4 = question ; sinon FALSE et la raison.
void MutationMenu_Check(void)
{
    StringCopy(gStringVar2, gTypesInfo[sReplacedType].name);
    StringCopy(gStringVar3, gTypesInfo[sNewType].name);
    gSpecialVar_Result = FALSE;
    if (!IsMutationTypeOpen(sNewType) && !CheckBagHasItem(ITEM_MUTATION_SEAL, 1))
    {
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("The {STR_VAR_3} type is still sealed.\pA MUTATION SEAL from the SHIFTING\nRUINS can open it for good."));
        return;
    }
    if (IsMutationTypeOpen(sNewType))
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("{STR_VAR_1} will trade {STR_VAR_2}\nfor {STR_VAR_3}.\pIts weaknesses will hurt more.\nThis is forever. Mutate?"));
    else
        StringExpandPlaceholders(gStringVar4, COMPOUND_STRING("Use a MUTATION SEAL to open\nthe {STR_VAR_3} type for good?\p{STR_VAR_1} will trade {STR_VAR_2}\nfor {STR_VAR_3}.\pIts weaknesses will hurt more.\nThis is forever. Mutate?"));
    gSpecialVar_Result = TRUE;
}

// Applique la mutation ; un type scellé consomme un Mutation Seal et s'ouvre pour de bon.
void MutationMenu_Apply(void)
{
    struct BoxPokemon *boxMon = GetTreeMenuBoxMon();
    bool32 sealed = !IsMutationTypeOpen(sNewType);

    gSpecialVar_Result = FALSE;
    if (sealed && !CheckBagHasItem(ITEM_MUTATION_SEAL, 1))
        return;
    if (!SetBoxMonMutation(boxMon, sReplacedType, sNewType))
        return;
    if (sealed)
    {
        RemoveBagItem(ITEM_MUTATION_SEAL, 1);
        OpenMutationType(sNewType);
    }
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][sMonSlot]);
    gSpecialVar_Result = TRUE;
}
