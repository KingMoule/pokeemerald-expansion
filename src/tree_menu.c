#include "global.h"
#include "battle_main.h"
#include "event_data.h"
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
        [TREE_INVEST_OK]          = COMPOUND_STRING("Investir 1 point ?\nC'est définitif."),
        [TREE_INVEST_WRONG_TREE]  = COMPOUND_STRING("Ce Pokémon suit\ndéjà un autre arbre."),
        [TREE_INVEST_NO_NODE]     = COMPOUND_STRING("Ce nœud n'existe pas."),
        [TREE_INVEST_MAX_RANK]    = COMPOUND_STRING("Ce nœud est déjà\nau rang maximal."),
        [TREE_INVEST_NO_POINTS]   = COMPOUND_STRING("Aucun point d'arbre\ndisponible."),
        [TREE_INVEST_LOCKED]      = COMPOUND_STRING("Ce nœud est encore\nverrouillé."),
    };
    const struct TreeNode *treeNode = GetTreeNode(sTreeType, sNode);
    enum TreeInvestResult result = CanBoxMonInvestInNode(GetTreeMenuBoxMon(), sTreeType, sNode);
    u8 *end = gStringVar4;

    if (treeNode != NULL)
    {
        if (sNode >= TREE_TRUNK_NODES)
        {
            end = StringCopy(end, gElementTrees[sTreeType].branchNames[(sNode - TREE_TRUNK_NODES) / TREE_BRANCH_NODES]);
            end = StringCopy(end, COMPOUND_STRING(" :"));
            *end++ = CHAR_NEWLINE;
        }
        end = StringCopy(end, treeNode->name);
        *end++ = CHAR_PROMPT_CLEAR;
        end = StringCopy(end, treeNode->description);
        *end++ = CHAR_PROMPT_CLEAR;
    }
    StringCopy(end, sRefusals[result]);
    gSpecialVar_Result = result;
}

void TreeMenu_Invest(void)
{
    if (TryBoxMonInvestInNode(GetTreeMenuBoxMon(), sTreeType, sNode) == TREE_INVEST_OK)
        CalculateMonStats(&gParties[B_TRAINER_PLAYER][sMonSlot]);
}
