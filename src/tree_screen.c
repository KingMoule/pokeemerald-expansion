#include "global.h"
#include "battle_main.h"
#include "battle_util.h"
#include "event_data.h"
#include "bg.h"
#include "gpu_regs.h"
#include "item.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "move.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_build.h"
#include "pokemon_icon.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "text.h"
#include "tree_menu.h"
#include "tree_screen.h"
#include "window.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Écran BUILD, ouvert depuis le menu d'équipe (et par le Technique Codex depuis le sac).
// Pages parcourues avec L/R : les onglets de l'arbre (Trunk et les trois branches), Moves, Mutate.
// Select passe au Pokémon suivant de l'équipe.
//  - Arbre : Gauche/Droite (ou A) placent des points en attente ; Start les investit tous après une
//    seule confirmation. Un double type qui n'a pas encore d'arbre le choisit d'abord.
//  - Moves : les quatre attaques avec leur variante, leur puissance et leurs PP. A sur une attaque
//    sans variante compare les variantes possibles et en donne une contre un Technique Codex.
//  - Mutate : les 18 types, ouverts ou scellés, avec les faiblesses avant et après la mutation.

#define TREE_TAB_COUNT (1 + TREE_BRANCH_COUNT)
#define TREE_TAB_NODES 4
#define MUTATE_COLUMNS 6
#define MUTATE_TYPES   18
#define MUTATE_REPLACED_ROW 0xFF
#define TABS_WIDTH 232 // Les onglets vont de x = 4 à 236 ; L/R sont rappelés dans l'aide.

enum
{
    WIN_HEADER,
    WIN_TABS,
    WIN_LIST,
    WIN_DESC,
    WIN_HELP,
};

enum
{
    STATE_INIT,
    STATE_FADE_IN,
    STATE_INPUT,
    STATE_CODEX,
    STATE_CONFIRM_INVEST,
    STATE_CONFIRM_LEAVE,
    STATE_CONFIRM_CODEX,
    STATE_CONFIRM_MUTATE,
    STATE_FADE_OUT,
    STATE_EXIT,
};

enum
{
    SECTION_TREE,
    SECTION_MOVES,
    SECTION_MUTATE,
};

// Couleurs de la palette des fenêtres. 1 à 4 suivent la palette des icônes de boutons.
enum
{
    COL_TRANSPARENT,
    COL_WHITE,
    COL_DARK,
    COL_LIGHT,
    COL_HEADER,
    COL_HEADER_DARK,
    COL_CREAM,
    COL_TAB,
    COL_BROWN,
    COL_RANK,
    COL_PENDING,
    COL_EMPTY,
    COL_LOCKED,
    COL_HIGHLIGHT,
    COL_HELP,
    COL_YELLOW,
};

static const u16 sTreeScreenPalette[16] =
{
    [COL_TRANSPARENT] = RGB2GBA(248, 232, 200),
    [COL_WHITE]       = RGB2GBA(255, 255, 255),
    [COL_DARK]        = RGB2GBA(56, 56, 56),
    [COL_LIGHT]       = RGB2GBA(216, 216, 216),
    [COL_HEADER]      = RGB2GBA(200, 72, 48),
    [COL_HEADER_DARK] = RGB2GBA(120, 32, 16),
    [COL_CREAM]       = RGB2GBA(248, 232, 200),
    [COL_TAB]         = RGB2GBA(216, 192, 160),
    [COL_BROWN]       = RGB2GBA(136, 96, 64),
    [COL_RANK]        = RGB2GBA(232, 96, 48),
    [COL_PENDING]     = RGB2GBA(64, 160, 64),
    [COL_EMPTY]       = RGB2GBA(232, 232, 232),
    [COL_LOCKED]      = RGB2GBA(160, 160, 168),
    [COL_HIGHLIGHT]   = RGB2GBA(255, 240, 200),
    [COL_HELP]        = RGB2GBA(64, 64, 80),
    [COL_YELLOW]      = RGB2GBA(255, 248, 136),
};

static const struct BgTemplate sTreeScreenBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

// Les fenêtres couvrent tout l'écran (600 tuiles, débordent sur le 2e bloc de tuiles, libre ici).
static const struct WindowTemplate sTreeScreenWindowTemplates[] =
{
    [WIN_HEADER] = { .bg = 0, .tilemapLeft = 0, .tilemapTop = 0,  .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 1 },
    [WIN_TABS]   = { .bg = 0, .tilemapLeft = 0, .tilemapTop = 2,  .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 61 },
    [WIN_LIST]   = { .bg = 0, .tilemapLeft = 0, .tilemapTop = 4,  .width = 30, .height = 8, .paletteNum = 15, .baseBlock = 121 },
    [WIN_DESC]   = { .bg = 0, .tilemapLeft = 0, .tilemapTop = 12, .width = 30, .height = 6, .paletteNum = 15, .baseBlock = 361 },
    [WIN_HELP]   = { .bg = 0, .tilemapLeft = 0, .tilemapTop = 18, .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 541 },
    DUMMY_WIN_TEMPLATE,
};

static const u8 sColors_Dark[]       = { COL_WHITE, COL_DARK, COL_LIGHT };
static const u8 sColors_Highlight[]  = { COL_HIGHLIGHT, COL_DARK, COL_LIGHT };
static const u8 sColors_Locked[]     = { COL_WHITE, COL_LOCKED, COL_TRANSPARENT };
static const u8 sColors_LockedHl[]   = { COL_HIGHLIGHT, COL_LOCKED, COL_TRANSPARENT };
static const u8 sColors_Pending[]    = { COL_WHITE, COL_PENDING, COL_TRANSPARENT };
static const u8 sColors_PendingHl[]  = { COL_HIGHLIGHT, COL_PENDING, COL_TRANSPARENT };
static const u8 sColors_Rank[]       = { COL_WHITE, COL_RANK, COL_TRANSPARENT };
static const u8 sColors_RankHl[]     = { COL_HIGHLIGHT, COL_RANK, COL_TRANSPARENT };
static const u8 sColors_Arrow[]      = { COL_HIGHLIGHT, COL_HEADER, COL_TRANSPARENT };
static const u8 sColors_Header[]     = { COL_HEADER, COL_WHITE, COL_HEADER_DARK };
static const u8 sColors_HeaderNum[]  = { COL_HEADER, COL_YELLOW, COL_HEADER_DARK };
static const u8 sColors_TabOn[]      = { COL_WHITE, COL_DARK, COL_LIGHT };
static const u8 sColors_TabOff[]     = { COL_TAB, COL_BROWN, COL_TRANSPARENT };
static const u8 sColors_Cream[]      = { COL_CREAM, COL_DARK, COL_TRANSPARENT };
static const u8 sColors_Status[]     = { COL_WHITE, COL_HEADER_DARK, COL_TRANSPARENT };
static const u8 sColors_Help[]       = { COL_HELP, COL_WHITE, COL_DARK };
static const u8 sColors_Grey[]       = { COL_WHITE, COL_LOCKED, COL_TRANSPARENT };
static const u8 sColors_ChipOpen[]   = { COL_WHITE, COL_DARK, COL_TRANSPARENT };
static const u8 sColors_ChipHl[]     = { COL_HIGHLIGHT, COL_DARK, COL_TRANSPARENT };
static const u8 sColors_ChipSealed[] = { COL_EMPTY, COL_LOCKED, COL_TRANSPARENT };
static const u8 sColors_ChipSealedHl[] = { COL_HIGHLIGHT, COL_LOCKED, COL_TRANSPARENT };
static const u8 sColors_ChipOwn[]    = { COL_TAB, COL_BROWN, COL_TRANSPARENT };

struct TreeScreen
{
    struct BoxPokemon preview; // Copie du Pokémon avec les points en attente appliqués.
    MainCallback returnCallback;
    const u8 *statusText;      // Message affiché à la place de la dernière ligne, jusqu'à la prochaine action.
    u8 pending[MOD_TREE_NODE_COUNT];
    u8 state;
    u8 section;
    u8 tab;
    u8 cursor;
    u8 monSlot;
    u8 iconSpriteId;
    u8 moveCursor;
    u8 variant;                // Variante choisie dans la comparaison du Technique Codex.
    u8 mutateCursor;           // Index dans la grille des types, ou MUTATE_REPLACED_ROW.
    u8 mutateType;             // Dernier index choisi dans la grille.
    u8 replacedSlot;           // Type de l'espèce abandonné (0 ou 1).
    bool8 treeKnown;           // FALSE : un double type sans arbre doit d'abord choisir.
    enum Type treeType;
};

static EWRAM_DATA struct TreeScreen *sTreeScreen = NULL;
static EWRAM_DATA u8 sLastMonSlot = 0;

static struct BoxPokemon *GetTreeScreenBoxMon(void)
{
    return &gParties[B_TRAINER_PLAYER][sTreeScreen->monSlot].box;
}

static enum Species GetTreeScreenSpecies(void)
{
    return GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_SPECIES);
}

static u32 GetTabFirstNode(u32 tab)
{
    if (tab == 0)
        return 0;
    return TREE_TRUNK_NODES + (tab - 1) * TREE_BRANCH_NODES;
}

static u32 GetPendingTotal(const u8 *pending)
{
    u32 node, total = 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
        total += pending[node];
    return total;
}

// Recompose l'aperçu : copie du Pokémon, puis les points en attente rejoués dans l'ordre des nœuds
// (le tronc avant les branches, chaque nœud après son parent). Un point devenu impossible, parce
// qu'un point du parent a été retiré, est abandonné. Renvoie le nombre de points en attente.
u32 TreePreview_Rebuild(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending)
{
    u32 node, i;

    *preview = *boxMon;
    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        for (i = 0; i < pending[node]; i++)
        {
            if (TryBoxMonInvestInNode(preview, treeType, node) != TREE_INVEST_OK)
                break;
        }
        pending[node] = i;
    }
    return GetPendingTotal(pending);
}

bool32 TreePreview_Add(struct BoxPokemon *preview, enum Type treeType, u8 *pending, u32 node)
{
    if (TryBoxMonInvestInNode(preview, treeType, node) != TREE_INVEST_OK)
        return FALSE;
    pending[node]++;
    return TRUE;
}

// Seuls les points en attente se retirent.
bool32 TreePreview_Remove(struct BoxPokemon *preview, struct BoxPokemon *boxMon, enum Type treeType, u8 *pending, u32 node)
{
    if (pending[node] == 0)
        return FALSE;
    pending[node]--;
    TreePreview_Rebuild(preview, boxMon, treeType, pending);
    return TRUE;
}

// Investit pour de bon les points en attente. Renvoie le nombre de points investis.
u32 TreePreview_Commit(struct BoxPokemon *boxMon, enum Type treeType, const u8 *pending)
{
    u32 node, i, total = 0;

    for (node = 0; node < MOD_TREE_NODE_COUNT; node++)
    {
        for (i = 0; i < pending[node]; i++)
        {
            if (TryBoxMonInvestInNode(boxMon, treeType, node) != TREE_INVEST_OK)
                break;
            total++;
        }
    }
    return total;
}

// Puissance d'une attaque avec sa variante (0 : attaque sans puissance fixe).
u32 BuildScreen_GetVariantPower(enum Move move, enum MoveVariant variant)
{
    u32 percent = 100;

    switch (variant)
    {
    case MOVE_VARIANT_FOCUSED:
        percent = MOD_VARIANT_FOCUSED_POWER;
        break;
    case MOVE_VARIANT_THRIFTY:
        percent = MOD_VARIANT_THRIFTY_POWER;
        break;
    case MOVE_VARIANT_VOLATILE:
        percent = MOD_VARIANT_VOLATILE_POWER;
        break;
    default:
        break;
    }
    return GetMovePower(move) * percent / 100;
}

// Types du Pokémon si le type de l'emplacement replacedSlot devient newType.
void BuildScreen_GetMutatedTypes(enum Species species, u32 replacedSlot, enum Type newType, enum Type *types)
{
    types[0] = GetSpeciesType(species, 0);
    types[1] = GetSpeciesType(species, 1);
    if (types[0] == types[1])
        types[0] = types[1] = newType;
    else
        types[replacedSlot] = newType;
}

// Faiblesses (dégâts ×2 ou plus) d'une paire de types, en bits (1 << type).
u32 BuildScreen_GetWeaknesses(enum Type type1, enum Type type2)
{
    u32 weaknesses = 0;
    enum Type attack;

    for (attack = 0; attack < NUMBER_OF_MON_TYPES; attack++)
    {
        uq4_12_t modifier;

        if (!IsMutationTargetType(attack))
            continue;
        modifier = GetTypeModifier(attack, type1);
        if (type2 != type1)
            modifier = uq4_12_multiply(modifier, GetTypeModifier(attack, type2));
        if (modifier > UQ_4_12(1.0))
            weaknesses |= 1u << attack;
    }
    return weaknesses;
}

// Le n-ième type de la grille Mutate (ordre des types du jeu, sans le type ???).
static enum Type GetMutateGridType(u32 index)
{
    enum Type type;
    u32 n = 0;

    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (!IsMutationTargetType(type))
            continue;
        if (n++ == index)
            return type;
    }
    return TYPE_NONE;
}

static bool32 IsDualTyped(enum Species species)
{
    return GetSpeciesType(species, 0) != GetSpeciesType(species, 1);
}

static u32 CountMoves(void)
{
    u32 slot, count = 0;

    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        if (GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_MOVE1 + slot) != MOVE_NONE)
            count++;
    }
    return count;
}

static enum Move GetCursorMove(void)
{
    return GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_MOVE1 + sTreeScreen->moveCursor);
}

// ---------------------------------------------------------------------------------------------
// Pages

// Nombre de pages de l'arbre : 1 (choix de l'arbre) ou les quatre onglets.
static u32 GetTreePageCount(void)
{
    return sTreeScreen->treeKnown ? TREE_TAB_COUNT : 1;
}

static u32 GetPageCount(void)
{
    return GetTreePageCount() + 2;
}

static u32 GetCurrentPage(void)
{
    switch (sTreeScreen->section)
    {
    case SECTION_TREE:
        return sTreeScreen->treeKnown ? sTreeScreen->tab : 0;
    case SECTION_MOVES:
        return GetTreePageCount();
    default:
        return GetTreePageCount() + 1;
    }
}

static void SetPage(u32 page)
{
    u32 treePages = GetTreePageCount();

    if (page < treePages)
    {
        sTreeScreen->section = SECTION_TREE;
        sTreeScreen->tab = page;
    }
    else if (page == treePages)
    {
        sTreeScreen->section = SECTION_MOVES;
    }
    else
    {
        sTreeScreen->section = SECTION_MUTATE;
    }
}

static const u8 *GetPageName(u32 page)
{
    u32 treePages = GetTreePageCount();

    if (page == treePages)
        return COMPOUND_STRING("Moves");
    if (page == treePages + 1)
        return COMPOUND_STRING("Mutate");
    if (!sTreeScreen->treeKnown)
        return COMPOUND_STRING("Tree");
    if (page == 0)
        return COMPOUND_STRING("Trunk");
    return gElementTrees[sTreeScreen->treeType].branchNames[page - 1];
}

// ---------------------------------------------------------------------------------------------
// Dessin commun

static u8 *AppendTypes(u8 *end, enum Type type1, enum Type type2)
{
    end = StringCopy(end, gTypesInfo[type1].name);
    if (type2 != type1)
    {
        *end++ = CHAR_SLASH;
        end = StringCopy(end, gTypesInfo[type2].name);
    }
    return end;
}

static void DrawHeader(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Species species = GetTreeScreenSpecies();
    u8 text[40];
    u8 *end;
    u32 x;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(COL_HEADER));
    FillWindowPixelRect(WIN_HEADER, PIXEL_FILL(COL_HEADER_DARK), 0, 15, 240, 1);
    GetBoxMonData(boxMon, MON_DATA_NICKNAME, text);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 32, 0, sColors_Header, TEXT_SKIP_DRAW, text);

    switch (sTreeScreen->section)
    {
    case SECTION_TREE:
        if (sTreeScreen->treeKnown)
        {
            u32 available = GetBoxMonTreePointsAvailable(&sTreeScreen->preview);
            u32 pendingTotal = GetPendingTotal(sTreeScreen->pending);

            end = StringCopy(text, gTypesInfo[sTreeScreen->treeType].name);
            StringCopy(end, COMPOUND_STRING(" Tree"));
            AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 100, 0, sColors_HeaderNum, TEXT_SKIP_DRAW, text);
            x = 162;
            AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, x, 0, sColors_Header, TEXT_SKIP_DRAW, COMPOUND_STRING("Points"));
            x += GetStringWidth(FONT_NORMAL, COMPOUND_STRING("Points"), 0) + 4;
            ConvertIntToDecimalStringN(text, available, STR_CONV_MODE_LEFT_ALIGN, 3);
            AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, x, 0, sColors_HeaderNum, TEXT_SKIP_DRAW, text);
            x += GetStringWidth(FONT_NORMAL, text, 0) + 3;
            if (pendingTotal != 0)
            {
                end = StringCopy(text, COMPOUND_STRING("(-"));
                end = ConvertIntToDecimalStringN(end, pendingTotal, STR_CONV_MODE_LEFT_ALIGN, 3);
                StringCopy(end, COMPOUND_STRING(")"));
                AddTextPrinterParameterized3(WIN_HEADER, FONT_SMALL, x, 1, sColors_Header, TEXT_SKIP_DRAW, text);
            }
        }
        else
        {
            ConvertIntToDecimalStringN(text, GetBoxMonTreePointsAvailable(boxMon), STR_CONV_MODE_LEFT_ALIGN, 3);
            AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 162, 0, sColors_Header, TEXT_SKIP_DRAW, COMPOUND_STRING("Points"));
            AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 166 + GetStringWidth(FONT_NORMAL, COMPOUND_STRING("Points"), 0), 0, sColors_HeaderNum, TEXT_SKIP_DRAW, text);
        }
        break;
    case SECTION_MOVES:
    case SECTION_MUTATE:
        end = AppendTypes(text, GetBoxMonMutatedType(boxMon, species, 0), GetBoxMonMutatedType(boxMon, species, 1));
        AddTextPrinterParameterized3(WIN_HEADER, FONT_SMALL, 100, 1, sColors_HeaderNum, TEXT_SKIP_DRAW, text);
        if (sTreeScreen->section == SECTION_MOVES)
        {
            end = StringCopy(text, COMPOUND_STRING("Codex "));
            ConvertIntToDecimalStringN(end, CountTotalItemQuantityInBag(ITEM_TECHNIQUE_CODEX), STR_CONV_MODE_LEFT_ALIGN, 3);
        }
        else
        {
            end = StringCopy(text, COMPOUND_STRING("Seals "));
            ConvertIntToDecimalStringN(end, CountTotalItemQuantityInBag(ITEM_MUTATION_SEAL), STR_CONV_MODE_LEFT_ALIGN, 3);
        }
        AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 236 - GetStringWidth(FONT_NORMAL, text, 0), 0, sColors_Header, TEXT_SKIP_DRAW, text);
        break;
    }
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

// Onglets : les pages de l'arbre, puis Moves et Mutate, séparés par un petit écart.
static void DrawTabs(void)
{
    u32 page, x, font = FONT_SMALL, padding = 8, total = 0, count = GetPageCount(), current = GetCurrentPage();

    for (page = 0; page < count; page++)
        total += GetStringWidth(FONT_SMALL, GetPageName(page), 0) + padding + 2;
    if (total + 4 > TABS_WIDTH)
    {
        font = FONT_SMALL_NARROW;
        total = 0;
        for (page = 0; page < count; page++)
            total += GetStringWidth(font, GetPageName(page), 0) + padding + 2;
        if (total + 4 > TABS_WIDTH)
            padding = 4;
    }

    FillWindowPixelBuffer(WIN_TABS, PIXEL_FILL(COL_CREAM));
    FillWindowPixelRect(WIN_TABS, PIXEL_FILL(COL_BROWN), 0, 15, 240, 1);

    x = 4;
    for (page = 0; page < count; page++)
    {
        const u8 *name = GetPageName(page);
        u32 width = GetStringWidth(font, name, 0) + padding;
        bool32 selected = (page == current);

        if (page == GetTreePageCount())
            x += 4;
        FillWindowPixelRect(WIN_TABS, PIXEL_FILL(COL_BROWN), x, 2, width, 14);
        FillWindowPixelRect(WIN_TABS, PIXEL_FILL(selected ? COL_WHITE : COL_TAB), x + 1, 3, width - 2, selected ? 13 : 12);
        AddTextPrinterParameterized3(WIN_TABS, font, x + padding / 2, 3, selected ? sColors_TabOn : sColors_TabOff, TEXT_SKIP_DRAW, name);
        x += width + 2;
    }
    CopyWindowToVram(WIN_TABS, COPYWIN_FULL);
}

static void DrawRowHighlight(u32 y)
{
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_HIGHLIGHT), 3, y, 234, 16);
    AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 6, y, sColors_Arrow, TEXT_SKIP_DRAW, COMPOUND_STRING("{RIGHT_ARROW}"));
}

static void BeginDescWindow(void)
{
    FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(COL_WHITE));
    FillWindowPixelRect(WIN_DESC, PIXEL_FILL(COL_BROWN), 0, 0, 240, 1);
}

static void PrintDescLine(u32 line, const u8 *colors, const u8 *text)
{
    static const u8 sLineY[] = { 2, 18, 33 };
    u32 font = (line == 0) ? FONT_NORMAL : FONT_SMALL;

    if (GetStringWidth(font, text, 0) > 226)
        font = (line == 0) ? FONT_NARROW : FONT_SMALL_NARROW;
    // Encore trop long (longue liste de faiblesses) : on coupe au dernier mot qui tient.
    if (GetStringWidth(font, text, 0) > 226)
    {
        u8 *copy = gStringVar3;
        u32 i, lastSpace = 0;

        StringCopyN(copy, text, 120);
        copy[120] = EOS;
        for (i = 0; copy[i] != EOS; i++)
        {
            if (copy[i] == CHAR_SPACE)
            {
                copy[i] = EOS;
                if (GetStringWidth(font, copy, 0) + 8 > 226)
                    break;
                copy[i] = CHAR_SPACE;
                lastSpace = i;
            }
        }
        if (lastSpace != 0)
            StringCopy(&copy[lastSpace], COMPOUND_STRING("…"));
        text = copy;
    }
    AddTextPrinterParameterized3(WIN_DESC, font, 8, sLineY[line], colors, TEXT_SKIP_DRAW, text);
}

static void DrawPrompt(const u8 *question)
{
    StringExpandPlaceholders(gStringVar4, question);
    BeginDescWindow();
    AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, 8, 2, sColors_Dark, TEXT_SKIP_DRAW, gStringVar4);
    AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 8, 34, sColors_Dark, TEXT_SKIP_DRAW, COMPOUND_STRING("{A_BUTTON} Yes    {B_BUTTON} No"));
    CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
}

// ---------------------------------------------------------------------------------------------
// Arbre

static void DrawLock(u32 x, u32 y, u32 fill)
{
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_LOCKED), x, y + 6, 7, 6);
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_LOCKED), x + 1, y + 2, 5, 4);
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(fill), x + 2, y + 3, 3, 3);
}

// Barre de rang : un segment par rang (20 au plus), investi, en attente ou vide.
static void DrawRankBar(u32 y, u32 rank, u32 pending, u32 maxRank, bool32 locked)
{
    u32 x = 134, segments = min(maxRank, 20), width = 60 / segments, i;

    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_LOCKED), x - 1, y + 4, segments * width + 1, 8);
    for (i = 0; i < segments; i++)
    {
        u32 color = COL_EMPTY;

        if (locked)
            color = COL_LIGHT;
        else if (i < rank - pending)
            color = COL_RANK;
        else if (i < rank)
            color = COL_PENDING;
        FillWindowPixelRect(WIN_LIST, PIXEL_FILL(color), x + i * width, y + 5, width - 1, 6);
    }
}

static void DrawTreeList(void)
{
    u32 i, first = GetTabFirstNode(sTreeScreen->tab);
    u8 text[16];
    u8 *end;

    for (i = 0; i < TREE_TAB_NODES; i++)
    {
        u32 node = first + i, y = i * 16, rank, pending;
        const struct TreeNode *treeNode = GetTreeNode(sTreeScreen->treeType, node);
        bool32 selected = (i == sTreeScreen->cursor);
        bool32 locked;

        if (treeNode == NULL)
            continue;
        rank = GetBoxMonTreeRank(&sTreeScreen->preview, node);
        pending = sTreeScreen->pending[node];
        locked = (CanBoxMonInvestInNode(&sTreeScreen->preview, sTreeScreen->treeType, node) == TREE_INVEST_LOCKED && rank == 0);
        if (selected)
            DrawRowHighlight(y);
        if (locked)
        {
            DrawLock(14, y, selected ? COL_HIGHLIGHT : COL_WHITE);
            AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 24, y, selected ? sColors_LockedHl : sColors_Locked, TEXT_SKIP_DRAW, treeNode->name);
        }
        else
        {
            AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 24, y, selected ? sColors_Highlight : sColors_Dark, TEXT_SKIP_DRAW, treeNode->name);
        }
        DrawRankBar(y, rank, pending, treeNode->maxRank, locked);

        end = ConvertIntToDecimalStringN(text, rank, STR_CONV_MODE_LEFT_ALIGN, 2);
        *end++ = CHAR_SLASH;
        ConvertIntToDecimalStringN(end, treeNode->maxRank, STR_CONV_MODE_LEFT_ALIGN, 2);
        if (pending != 0)
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 233 - GetStringWidth(FONT_SMALL, text, 0), y + 1, selected ? sColors_PendingHl : sColors_Pending, TEXT_SKIP_DRAW, text);
        else
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 233 - GetStringWidth(FONT_SMALL, text, 0), y + 1, locked ? (selected ? sColors_LockedHl : sColors_Locked) : (selected ? sColors_Highlight : sColors_Dark), TEXT_SKIP_DRAW, text);
    }
}

// Double type sans arbre : une ligne par arbre possible, avec ses branches.
static void DrawTreeChoiceList(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Species species = GetTreeScreenSpecies();
    u32 i;
    u8 text[48];
    u8 *end;

    for (i = 0; i < 2; i++)
    {
        enum Type type = GetBoxMonMutatedType(boxMon, species, i);
        bool32 selected = (i == sTreeScreen->cursor);
        u32 branch;

        if (selected)
            DrawRowHighlight(i * 16);
        end = StringCopy(text, gTypesInfo[type].name);
        StringCopy(end, COMPOUND_STRING(" Tree"));
        AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 24, i * 16, selected ? sColors_Highlight : sColors_Dark, TEXT_SKIP_DRAW, text);
        end = text;
        for (branch = 0; branch < TREE_BRANCH_COUNT; branch++)
        {
            if (branch != 0)
                end = StringCopy(end, COMPOUND_STRING(", "));
            end = StringCopy(end, gElementTrees[type].branchNames[branch]);
        }
        AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 104, i * 16 + 1, selected ? sColors_Highlight : sColors_Grey, TEXT_SKIP_DRAW, text);
    }
}

// Description du nœud choisi, puis ce qui l'empêche de monter.
static void DrawTreeDescription(void)
{
    u32 node = GetTabFirstNode(sTreeScreen->tab) + sTreeScreen->cursor;
    const struct TreeNode *treeNode = GetTreeNode(sTreeScreen->treeType, node);
    u8 *end = gStringVar4;

    if (treeNode == NULL)
        return;
    AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, 8, 2, sColors_Dark, TEXT_SKIP_DRAW, treeNode->description);
    gStringVar4[0] = EOS;
    switch (CanBoxMonInvestInNode(&sTreeScreen->preview, sTreeScreen->treeType, node))
    {
    case TREE_INVEST_LOCKED:
        end = StringCopy(end, COMPOUND_STRING("Locked: needs "));
        if (node >= TREE_TRUNK_NODES && (node - TREE_TRUNK_NODES) % TREE_BRANCH_NODES == 0)
        {
            end = ConvertIntToDecimalStringN(end, MOD_TREE_BRANCH_UNLOCK_POINTS, STR_CONV_MODE_LEFT_ALIGN, 2);
            StringCopy(end, COMPOUND_STRING(" Trunk points."));
        }
        else
        {
            const struct TreeNode *parent = GetTreeNode(sTreeScreen->treeType, node - 1);
            end = StringCopy(end, parent->name);
            end = StringCopy(end, COMPOUND_STRING(" rank "));
            end = ConvertIntToDecimalStringN(end, min(MOD_TREE_PARENT_RANKS, parent->maxRank), STR_CONV_MODE_LEFT_ALIGN, 2);
            StringCopy(end, COMPOUND_STRING("."));
        }
        break;
    case TREE_INVEST_MAX_RANK:
        StringCopy(end, COMPOUND_STRING("Max rank reached."));
        break;
    case TREE_INVEST_NO_POINTS:
        StringCopy(end, COMPOUND_STRING("No tree points left."));
        break;
    default:
        break;
    }
    PrintDescLine(2, sColors_Status, gStringVar4);
}

static void DrawTreeChoiceDescription(void)
{
    PrintDescLine(0, sColors_Dark, COMPOUND_STRING("Which tree will it follow?"));
    PrintDescLine(1, sColors_Dark, COMPOUND_STRING("A Pokémon follows a single tree."));
    PrintDescLine(2, sColors_Status, COMPOUND_STRING("The choice is final once a point is invested."));
}

// ---------------------------------------------------------------------------------------------
// Attaques et Technique Codex

static const u8 *const sVariantColors[MOVE_VARIANT_COUNT][2] =
{
    [MOVE_VARIANT_STANDARD] = { sColors_Grey, sColors_LockedHl },
    [MOVE_VARIANT_FOCUSED]  = { sColors_Rank, sColors_RankHl },
    [MOVE_VARIANT_THRIFTY]  = { sColors_Pending, sColors_PendingHl },
    [MOVE_VARIANT_VOLATILE] = { sColors_Status, sColors_Status },
};

static void PrintPowerAndPP(u32 y, u32 power, u32 pp, const u8 *colors)
{
    u8 text[16];
    u8 *end;

    end = StringCopy(text, COMPOUND_STRING("Pow "));
    if (power <= 1) // Puissance variable (Reversal, Low Kick...) ou attaque de statut.
        StringCopy(end, COMPOUND_STRING("-"));
    else
        ConvertIntToDecimalStringN(end, power, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 160, y + 1, colors, TEXT_SKIP_DRAW, text);
    end = StringCopy(text, COMPOUND_STRING("PP "));
    ConvertIntToDecimalStringN(end, pp, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 202, y + 1, colors, TEXT_SKIP_DRAW, text);
}

static void DrawMovesList(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    u32 slot;

    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        enum Move move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + slot);
        enum MoveVariant variant = GetBoxMonMoveVariant(boxMon, slot);
        bool32 selected = (slot == sTreeScreen->moveCursor);
        u32 y = slot * 16;

        if (move == MOVE_NONE)
            continue;
        if (selected)
            DrawRowHighlight(y);
        AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 18, y, selected ? sColors_Highlight : sColors_Dark, TEXT_SKIP_DRAW, GetMoveName(move));
        if (variant != MOVE_VARIANT_STANDARD)
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 108, y + 1, sVariantColors[variant][selected], TEXT_SKIP_DRAW, GetMoveVariantName(variant));
        else if (!CanMoveHaveVariant(move, MOVE_VARIANT_FOCUSED))
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 108, y + 1, selected ? sColors_LockedHl : sColors_Grey, TEXT_SKIP_DRAW, COMPOUND_STRING("No variant"));
        else
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 108, y + 1, selected ? sColors_LockedHl : sColors_Grey, TEXT_SKIP_DRAW, COMPOUND_STRING("-"));
        PrintPowerAndPP(y, BuildScreen_GetVariantPower(move, variant), ApplyMoveVariantToPP(GetMovePP(move), variant), selected ? sColors_Highlight : sColors_Dark);
    }
}

static void AppendVariantEffect(u8 *end, enum MoveVariant variant)
{
    switch (variant)
    {
    case MOVE_VARIANT_FOCUSED:
        StringCopy(end, COMPOUND_STRING(": power 120%, PP 60%."));
        break;
    case MOVE_VARIANT_THRIFTY:
        StringCopy(end, COMPOUND_STRING(": power 85%, PP 150%."));
        break;
    case MOVE_VARIANT_VOLATILE:
        StringCopy(end, COMPOUND_STRING(": power 85%, effect chance x2."));
        break;
    default:
        *end = EOS;
        break;
    }
}

static void DrawMovesDescription(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Move move = GetCursorMove();
    enum MoveVariant variant = GetBoxMonMoveVariant(boxMon, sTreeScreen->moveCursor);
    u8 *end;

    if (move == MOVE_NONE)
        return;
    if (variant != MOVE_VARIANT_STANDARD)
    {
        AppendVariantEffect(StringCopy(gStringVar4, GetMoveVariantName(variant)), variant);
        PrintDescLine(0, sColors_Dark, gStringVar4);
        end = StringCopy(gStringVar4, COMPOUND_STRING("Standard: power "));
        end = ConvertIntToDecimalStringN(end, GetMovePower(move), STR_CONV_MODE_LEFT_ALIGN, 3);
        end = StringCopy(end, COMPOUND_STRING(", PP "));
        end = ConvertIntToDecimalStringN(end, GetMovePP(move), STR_CONV_MODE_LEFT_ALIGN, 3);
        StringCopy(end, COMPOUND_STRING("."));
        PrintDescLine(1, sColors_Grey, gStringVar4);
        PrintDescLine(2, sColors_Status, COMPOUND_STRING("Permanent. Forget the move to reset it."));
    }
    else if (!CanMoveHaveVariant(move, MOVE_VARIANT_FOCUSED))
    {
        PrintDescLine(0, sColors_Dark, COMPOUND_STRING("This move has no variants."));
        PrintDescLine(1, sColors_Grey, COMPOUND_STRING("Only moves with a set power have them."));
    }
    else
    {
        PrintDescLine(0, sColors_Dark, COMPOUND_STRING("No variant yet."));
        if (CheckBagHasItem(ITEM_TECHNIQUE_CODEX, 1))
            PrintDescLine(1, sColors_Dark, COMPOUND_STRING("{A_BUTTON} Compare variants and use a Technique Codex."));
        else
            PrintDescLine(1, sColors_Grey, COMPOUND_STRING("A Technique Codex can give it one."));
    }
}

// Comparaison des variantes : Standard (actuelle), puis les trois variantes, impossibles en gris.
static void DrawCodexList(void)
{
    enum Move move = GetCursorMove();
    enum MoveVariant variant;
    u8 text[32];

    for (variant = MOVE_VARIANT_STANDARD; variant < MOVE_VARIANT_COUNT; variant++)
    {
        bool32 selected = (variant == sTreeScreen->variant);
        bool32 possible = (variant == MOVE_VARIANT_STANDARD || CanMoveHaveVariant(move, variant));
        u32 y = variant * 16;
        const u8 *colors;

        if (selected)
            DrawRowHighlight(y);
        if (variant == MOVE_VARIANT_STANDARD)
        {
            StringCopy(StringCopy(text, GetMoveVariantName(variant)), COMPOUND_STRING(" (now)"));
            colors = selected ? sColors_LockedHl : sColors_Grey;
        }
        else
        {
            StringCopy(text, GetMoveVariantName(variant));
            colors = possible ? sVariantColors[variant][selected] : (selected ? sColors_LockedHl : sColors_Grey);
        }
        AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 18, y, colors, TEXT_SKIP_DRAW, text);
        if (possible)
            PrintPowerAndPP(y, BuildScreen_GetVariantPower(move, variant), ApplyMoveVariantToPP(GetMovePP(move), variant), selected ? sColors_Highlight : sColors_Dark);
        else
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 160, y + 1, selected ? sColors_LockedHl : sColors_Grey, TEXT_SKIP_DRAW, COMPOUND_STRING("No added effect"));
        if (variant == MOVE_VARIANT_VOLATILE && possible)
            AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL_NARROW, 108, y + 1, selected ? sColors_Highlight : sColors_Dark, TEXT_SKIP_DRAW, COMPOUND_STRING("Effect x2"));
    }
}

static u8 *AppendSignedDiff(u8 *end, s32 diff)
{
    *end++ = (diff < 0) ? CHAR_HYPHEN : CHAR_PLUS;
    return ConvertIntToDecimalStringN(end, (diff < 0) ? -diff : diff, STR_CONV_MODE_LEFT_ALIGN, 3);
}

static void DrawCodexDescription(void)
{
    enum Move move = GetCursorMove();
    enum MoveVariant variant = sTreeScreen->variant;
    u8 *end;

    end = StringCopy(gStringVar4, COMPOUND_STRING("Give "));
    end = StringCopy(end, GetMoveName(move));
    StringCopy(end, COMPOUND_STRING(" a variant."));
    PrintDescLine(0, sColors_Dark, gStringVar4);
    if (variant == MOVE_VARIANT_STANDARD)
    {
        PrintDescLine(1, sColors_Grey, COMPOUND_STRING("Its current form. Pick a variant below."));
    }
    else
    {
        end = StringCopy(gStringVar4, GetMoveVariantName(variant));
        end = StringCopy(end, COMPOUND_STRING(": "));
        end = AppendSignedDiff(end, (s32)BuildScreen_GetVariantPower(move, variant) - (s32)GetMovePower(move));
        end = StringCopy(end, COMPOUND_STRING(" power, "));
        end = AppendSignedDiff(end, (s32)ApplyMoveVariantToPP(GetMovePP(move), variant) - (s32)GetMovePP(move));
        end = StringCopy(end, COMPOUND_STRING(" PP"));
        if (variant == MOVE_VARIANT_VOLATILE)
            end = StringCopy(end, COMPOUND_STRING(", effect chance x2"));
        StringCopy(end, COMPOUND_STRING("."));
        PrintDescLine(1, sColors_Dark, gStringVar4);
    }
    PrintDescLine(2, sColors_Status, COMPOUND_STRING("Uses 1 Technique Codex. One variant per move."));
}

// ---------------------------------------------------------------------------------------------
// Mutation

static enum Type GetReplacedType(void)
{
    return GetSpeciesType(GetTreeScreenSpecies(), sTreeScreen->replacedSlot);
}

static bool32 IsSpeciesType(enum Type type)
{
    enum Species species = GetTreeScreenSpecies();
    return type == GetSpeciesType(species, 0) || type == GetSpeciesType(species, 1);
}

static void DrawTypeChip(u32 x, u32 y, u32 width, enum Type type, bool32 selected, bool32 own, bool32 open)
{
    const u8 *colors;
    u32 fill, textWidth, font = FONT_SMALL_NARROW;
    const u8 *name = gTypesInfo[type].name;

    if (own)
    {
        fill = COL_TAB;
        colors = sColors_ChipOwn;
    }
    else if (!open)
    {
        fill = selected ? COL_HIGHLIGHT : COL_EMPTY;
        colors = selected ? sColors_ChipSealedHl : sColors_ChipSealed;
    }
    else
    {
        fill = selected ? COL_HIGHLIGHT : COL_WHITE;
        colors = selected ? sColors_ChipHl : sColors_ChipOpen;
    }
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(selected ? COL_HEADER : (open && !own ? COL_DARK : COL_LOCKED)), x, y, width, 14);
    FillWindowPixelRect(WIN_LIST, PIXEL_FILL(fill), x + 1, y + 1, width - 2, 12);
    if (selected)
        FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_HEADER), x + 1, y + 1, width - 2, 1);
    textWidth = GetStringWidth(font, name, 0);
    AddTextPrinterParameterized3(WIN_LIST, font, x + (textWidth < width ? (width - textWidth) / 2 : 0), y + 1, colors, TEXT_SKIP_DRAW, name);
}

static void DrawMutateList(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    u32 spent = GetBoxMonTreePointsSpent(boxMon);
    u32 i, x;
    u8 text[32];
    u8 *end;

    // Ligne du haut : type abandonné, puis les conditions de la mutation.
    AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 6, 1, sColors_Dark, TEXT_SKIP_DRAW, COMPOUND_STRING("Replace"));
    x = 8 + GetStringWidth(FONT_SMALL, COMPOUND_STRING("Replace"), 0);
    DrawTypeChip(x, 1, 50, GetReplacedType(), sTreeScreen->mutateCursor == MUTATE_REPLACED_ROW, FALSE, TRUE);
    AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, x + 54, 1, sColors_Dark, TEXT_SKIP_DRAW, COMPOUND_STRING("with:"));

    if (boxMon->build.mutationType != TYPE_NONE)
    {
        AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL, 168, 1, sColors_Status, TEXT_SKIP_DRAW, COMPOUND_STRING("Mutated"));
    }
    else
    {
        bool32 mastery = CanBoxMonAcquireMutation(boxMon);
        u32 masteryX = 236 - GetStringWidth(FONT_SMALL_NARROW, COMPOUND_STRING("Mastery"), 0);

        // CanBoxMonAcquireMutation demande aussi les points : sans eux, la maîtrise reste en gris.
        AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL_NARROW, masteryX, 1, mastery ? sColors_Pending : (spent >= MOD_MUTATION_TREE_POINTS ? sColors_Status : sColors_Grey), TEXT_SKIP_DRAW, COMPOUND_STRING("Mastery"));
        end = ConvertIntToDecimalStringN(text, min(spent, MOD_MUTATION_TREE_POINTS), STR_CONV_MODE_LEFT_ALIGN, 3);
        *end++ = CHAR_SLASH;
        end = ConvertIntToDecimalStringN(end, MOD_MUTATION_TREE_POINTS, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringCopy(end, COMPOUND_STRING(" pts"));
        AddTextPrinterParameterized3(WIN_LIST, FONT_SMALL_NARROW, masteryX - 6 - GetStringWidth(FONT_SMALL_NARROW, text, 0), 1, spent >= MOD_MUTATION_TREE_POINTS ? sColors_Pending : sColors_Status, TEXT_SKIP_DRAW, text);
    }

    for (i = 0; i < MUTATE_TYPES; i++)
    {
        enum Type type = GetMutateGridType(i);
        u32 column = i % MUTATE_COLUMNS, row = i / MUTATE_COLUMNS;

        DrawTypeChip(2 + column * 39, 17 + row * 16, 38, type, i == sTreeScreen->mutateCursor, IsSpeciesType(type), IsMutationTypeOpen(type));
    }
}

static u8 *AppendTypeList(u8 *end, u32 types)
{
    enum Type type;
    bool32 first = TRUE;

    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (!(types & (1u << type)))
            continue;
        if (!first)
            *end++ = CHAR_SPACE;
        end = StringCopy(end, gTypesInfo[type].name);
        first = FALSE;
    }
    if (first)
        end = StringCopy(end, COMPOUND_STRING("none"));
    return end;
}

// Raison qui empêche la mutation vers le type de la grille, ou NULL.
static const u8 *GetMutationRefusal(enum Type newType)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();

    if (boxMon->build.mutationType != TYPE_NONE)
        return COMPOUND_STRING("It has already mutated. A mutation is forever.");
    if (!CanBoxMonAcquireMutation(boxMon))
        return COMPOUND_STRING("Needs 50 Tree points and a final mastery.");
    if (IsSpeciesType(newType))
        return COMPOUND_STRING("It already has this type.");
    if (!IsMutationTypeOpen(newType) && !CheckBagHasItem(ITEM_MUTATION_SEAL, 1))
        return COMPOUND_STRING("Sealed. A Mutation Seal from the Ruins opens it.");
    return NULL;
}

static void DrawMutateDescription(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Species species = GetTreeScreenSpecies();
    enum Type newType = GetMutateGridType(sTreeScreen->mutateType);
    enum Type now[2], after[2];
    u8 *end;

    now[0] = GetBoxMonMutatedType(boxMon, species, 0);
    now[1] = GetBoxMonMutatedType(boxMon, species, 1);
    if (boxMon->build.mutationType != TYPE_NONE || IsSpeciesType(newType))
    {
        AppendTypes(gStringVar4, now[0], now[1]);
        PrintDescLine(0, sColors_Dark, gStringVar4);
        AppendTypeList(StringCopy(gStringVar4, COMPOUND_STRING("Weak to: ")), BuildScreen_GetWeaknesses(now[0], now[1]));
        PrintDescLine(1, sColors_Dark, gStringVar4);
        PrintDescLine(2, sColors_Status, GetMutationRefusal(newType));
        return;
    }

    BuildScreen_GetMutatedTypes(species, sTreeScreen->replacedSlot, newType, after);
    end = AppendTypes(gStringVar4, now[0], now[1]);
    end = StringCopy(end, COMPOUND_STRING(" {RIGHT_ARROW} "));
    end = AppendTypes(end, after[0], after[1]);
    if (!IsMutationTypeOpen(newType))
        StringCopy(end, COMPOUND_STRING(" (Seal)"));
    PrintDescLine(0, sColors_Dark, gStringVar4);
    AppendTypeList(StringCopy(gStringVar4, COMPOUND_STRING("Now weak to: ")), BuildScreen_GetWeaknesses(now[0], now[1]));
    PrintDescLine(1, sColors_Grey, gStringVar4);
    AppendTypeList(StringCopy(gStringVar4, COMPOUND_STRING("After: ")), BuildScreen_GetWeaknesses(after[0], after[1]));
    PrintDescLine(2, sColors_Dark, gStringVar4);
}

// ---------------------------------------------------------------------------------------------

static void DrawList(void)
{
    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(COL_WHITE));
    switch (sTreeScreen->section)
    {
    case SECTION_TREE:
        if (sTreeScreen->treeKnown)
            DrawTreeList();
        else
            DrawTreeChoiceList();
        break;
    case SECTION_MOVES:
        if (sTreeScreen->state == STATE_CODEX || sTreeScreen->state == STATE_CONFIRM_CODEX)
            DrawCodexList();
        else
            DrawMovesList();
        break;
    case SECTION_MUTATE:
        DrawMutateList();
        break;
    }
    CopyWindowToVram(WIN_LIST, COPYWIN_FULL);
}

static void DrawDescription(void)
{
    BeginDescWindow();
    switch (sTreeScreen->section)
    {
    case SECTION_TREE:
        if (sTreeScreen->treeKnown)
            DrawTreeDescription();
        else
            DrawTreeChoiceDescription();
        break;
    case SECTION_MOVES:
        if (sTreeScreen->state == STATE_CODEX)
            DrawCodexDescription();
        else
            DrawMovesDescription();
        break;
    case SECTION_MUTATE:
        DrawMutateDescription();
        break;
    }
    // Un message (refus, réussite) remplace la dernière ligne jusqu'à la prochaine action.
    if (sTreeScreen->statusText != NULL)
    {
        FillWindowPixelRect(WIN_DESC, PIXEL_FILL(COL_WHITE), 0, 32, 240, 16);
        PrintDescLine(2, sColors_Status, sTreeScreen->statusText);
    }
    CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
}

static void DrawHelp(void)
{
    const u8 *text;

    if (sTreeScreen->state == STATE_CODEX)
        text = COMPOUND_STRING("{DPAD_UPDOWN}Variant  {A_BUTTON}Choose  {B_BUTTON}Back");
    else if (sTreeScreen->section == SECTION_TREE && sTreeScreen->treeKnown)
        text = COMPOUND_STRING("{DPAD_LEFTRIGHT}Point {L_BUTTON}{R_BUTTON}Page {START_BUTTON}Confirm {SELECT_BUTTON}Next {B_BUTTON}Back");
    else if (sTreeScreen->section == SECTION_TREE)
        text = COMPOUND_STRING("{A_BUTTON}Choose  {L_BUTTON}{R_BUTTON}Page  {SELECT_BUTTON}Next  {B_BUTTON}Back");
    else if (sTreeScreen->section == SECTION_MOVES)
        text = COMPOUND_STRING("{A_BUTTON}Codex  {L_BUTTON}{R_BUTTON}Page  {SELECT_BUTTON}Next  {B_BUTTON}Back");
    else
        text = COMPOUND_STRING("{A_BUTTON}Mutate  {L_BUTTON}{R_BUTTON}Page  {SELECT_BUTTON}Next  {B_BUTTON}Back");
    FillWindowPixelBuffer(WIN_HELP, PIXEL_FILL(COL_HELP));
    AddTextPrinterParameterized3(WIN_HELP, FONT_SMALL, 3, 2, sColors_Help, TEXT_SKIP_DRAW, text);
    CopyWindowToVram(WIN_HELP, COPYWIN_FULL);
}

static void DrawAll(void)
{
    DrawHeader();
    DrawTabs();
    DrawList();
    DrawDescription();
    DrawHelp();
}

// ---------------------------------------------------------------------------------------------
// Pokémon affiché

static void CreateIcon(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();

    sTreeScreen->iconSpriteId = CreateMonIcon(GetTreeScreenSpecies(), SpriteCB_MonIcon, 16, 8, 0, GetBoxMonData(boxMon, MON_DATA_PERSONALITY));
    gSprites[sTreeScreen->iconSpriteId].oam.priority = 0;
}

// Arbre suivi, ou à choisir pour un double type ; curseurs remis au début.
static void LoadMon(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Species species = GetTreeScreenSpecies();
    enum Type type1 = GetBoxMonMutatedType(boxMon, species, 0);
    enum Type type2 = GetBoxMonMutatedType(boxMon, species, 1);
    u32 i;

    memset(sTreeScreen->pending, 0, sizeof(sTreeScreen->pending));
    sTreeScreen->statusText = NULL;
    sTreeScreen->cursor = 0;
    sTreeScreen->moveCursor = 0;
    sTreeScreen->replacedSlot = 0;
    if (boxMon->build.treeType != TYPE_NONE)
        sTreeScreen->treeType = boxMon->build.treeType;
    else
        sTreeScreen->treeType = type1;
    sTreeScreen->treeKnown = (boxMon->build.treeType != TYPE_NONE || type1 == type2);
    if (sTreeScreen->tab >= GetTreePageCount())
        sTreeScreen->tab = 0;

    // Grille Mutate : le premier type ouvert que l'espèce n'a pas.
    sTreeScreen->mutateType = 0;
    for (i = 0; i < MUTATE_TYPES; i++)
    {
        enum Type type = GetMutateGridType(i);
        if (IsMutationTypeOpen(type) && !IsSpeciesType(type))
        {
            sTreeScreen->mutateType = i;
            break;
        }
    }
    sTreeScreen->mutateCursor = sTreeScreen->mutateType;
    TreePreview_Rebuild(&sTreeScreen->preview, boxMon, sTreeScreen->treeType, sTreeScreen->pending);
}

static void SwitchToNextMon(void)
{
    u32 slot = sTreeScreen->monSlot, i;

    if (GetPendingTotal(sTreeScreen->pending) != 0)
    {
        PlaySE(SE_BOO);
        sTreeScreen->statusText = COMPOUND_STRING("Confirm or remove the new points first.");
        DrawDescription();
        return;
    }
    for (i = 1; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][(slot + i) % PARTY_SIZE];

        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG))
            break;
    }
    if (i == PARTY_SIZE)
    {
        PlaySE(SE_BOO);
        return;
    }
    PlaySE(SE_SELECT);
    FreeAndDestroyMonIconSprite(&gSprites[sTreeScreen->iconSpriteId]);
    sTreeScreen->monSlot = (slot + i) % PARTY_SIZE;
    LoadMon();
    CreateIcon();
    DrawAll();
}

// ---------------------------------------------------------------------------------------------

static void VBlankCB_TreeScreen(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_TreeScreen(void);

static void InitTreeScreen(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    ScanlineEffect_Stop();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sTreeScreenBgTemplates, ARRAY_COUNT(sTreeScreenBgTemplates));
    InitWindows(sTreeScreenWindowTemplates);
    DeactivateAllTextPrinters();
    LoadPalette(sTreeScreenPalette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(sTreeScreenPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);

    LoadMonIconPalettes();
    LoadMon();
    CreateIcon();
    DrawAll();
    for (u32 win = WIN_HEADER; win <= WIN_HELP; win++)
        PutWindowTilemap(win);
    CopyBgTilemapBufferToVram(0);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_BG0_ON);
    ShowBg(0);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    SetVBlankCallback(VBlankCB_TreeScreen);
    SetMainCallback2(CB2_TreeScreen);
}

static void ChangePage(s32 delta)
{
    u32 count = GetPageCount();

    SetPage((GetCurrentPage() + count + delta) % count);
    sTreeScreen->statusText = NULL;
    PlaySE(SE_SELECT);
    DrawAll();
}

static void MoveTreeCursor(s32 delta)
{
    u32 count = sTreeScreen->treeKnown ? TREE_TAB_NODES : 2;

    sTreeScreen->cursor = (sTreeScreen->cursor + count + delta) % count;
    PlaySE(SE_SELECT);
    DrawList();
    DrawDescription();
}

static void ChangePoints(bool32 add)
{
    u32 node = GetTabFirstNode(sTreeScreen->tab) + sTreeScreen->cursor;
    bool32 changed;

    if (add)
        changed = TreePreview_Add(&sTreeScreen->preview, sTreeScreen->treeType, sTreeScreen->pending, node);
    else
        changed = TreePreview_Remove(&sTreeScreen->preview, GetTreeScreenBoxMon(), sTreeScreen->treeType, sTreeScreen->pending, node);
    if (!changed)
    {
        PlaySE(SE_BOO);
        return;
    }
    PlaySE(SE_BALL_TRAY_BALL);
    DrawHeader();
    DrawList();
    DrawDescription();
}

// L'arbre choisi n'est enregistré qu'avec le premier point investi.
static void ChooseTree(void)
{
    sTreeScreen->treeType = GetBoxMonMutatedType(GetTreeScreenBoxMon(), GetTreeScreenSpecies(), sTreeScreen->cursor);
    sTreeScreen->treeKnown = TRUE;
    sTreeScreen->tab = 0;
    sTreeScreen->cursor = 0;
    TreePreview_Rebuild(&sTreeScreen->preview, GetTreeScreenBoxMon(), sTreeScreen->treeType, sTreeScreen->pending);
    PlaySE(SE_SELECT);
    DrawAll();
}

static void HandleTreeInput(void)
{
    if (JOY_REPEAT(DPAD_UP))
        MoveTreeCursor(-1);
    else if (JOY_REPEAT(DPAD_DOWN))
        MoveTreeCursor(1);
    else if (!sTreeScreen->treeKnown)
    {
        if (JOY_NEW(A_BUTTON))
            ChooseTree();
    }
    else if (JOY_REPEAT(DPAD_RIGHT) || JOY_REPEAT(A_BUTTON))
        ChangePoints(TRUE);
    else if (JOY_REPEAT(DPAD_LEFT))
        ChangePoints(FALSE);
    else if (JOY_NEW(START_BUTTON))
    {
        if (GetPendingTotal(sTreeScreen->pending) == 0)
        {
            PlaySE(SE_BOO);
            return;
        }
        PlaySE(SE_SELECT);
        ConvertIntToDecimalStringN(gStringVar1, GetPendingTotal(sTreeScreen->pending), STR_CONV_MODE_LEFT_ALIGN, 3);
        DrawPrompt(COMPOUND_STRING("Invest {STR_VAR_1} point(s)?\nThis is permanent."));
        sTreeScreen->state = STATE_CONFIRM_INVEST;
    }
}

static void MoveMovesCursor(s32 delta)
{
    u32 count = CountMoves();

    if (count == 0)
        return;
    sTreeScreen->moveCursor = (sTreeScreen->moveCursor + count + delta) % count;
    PlaySE(SE_SELECT);
    DrawList();
    DrawDescription();
}

static void OpenCodex(void)
{
    enum Move move = GetCursorMove();

    if (GetBoxMonMoveVariant(GetTreeScreenBoxMon(), sTreeScreen->moveCursor) != MOVE_VARIANT_STANDARD)
        sTreeScreen->statusText = COMPOUND_STRING("Forget the move to reset its variant.");
    else if (!CanMoveHaveVariant(move, MOVE_VARIANT_FOCUSED))
        sTreeScreen->statusText = COMPOUND_STRING("This move has no variants.");
    else if (!CheckBagHasItem(ITEM_TECHNIQUE_CODEX, 1))
        sTreeScreen->statusText = COMPOUND_STRING("No Technique Codex in the Bag.");
    else
    {
        PlaySE(SE_SELECT);
        sTreeScreen->variant = MOVE_VARIANT_FOCUSED;
        sTreeScreen->state = STATE_CODEX;
        DrawList();
        DrawDescription();
        DrawHelp();
        return;
    }
    PlaySE(SE_BOO);
    DrawDescription();
}

static void HandleMovesInput(void)
{
    if (JOY_REPEAT(DPAD_UP))
        MoveMovesCursor(-1);
    else if (JOY_REPEAT(DPAD_DOWN))
        MoveMovesCursor(1);
    else if (JOY_NEW(A_BUTTON))
        OpenCodex();
}

static void CloseCodex(void)
{
    sTreeScreen->state = STATE_INPUT;
    DrawAll();
}

static void HandleCodexInput(void)
{
    enum Move move = GetCursorMove();

    if (JOY_REPEAT(DPAD_UP) || JOY_REPEAT(DPAD_DOWN))
    {
        s32 delta = JOY_REPEAT(DPAD_UP) ? -1 : 1;
        u32 variant = sTreeScreen->variant;

        // Les variantes impossibles sont sautées.
        do
            variant = (variant + MOVE_VARIANT_COUNT + delta) % MOVE_VARIANT_COUNT;
        while (variant != MOVE_VARIANT_STANDARD && !CanMoveHaveVariant(move, variant));
        sTreeScreen->variant = variant;
        PlaySE(SE_SELECT);
        DrawList();
        DrawDescription();
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (sTreeScreen->variant == MOVE_VARIANT_STANDARD)
        {
            PlaySE(SE_BOO);
            return;
        }
        PlaySE(SE_SELECT);
        StringCopy(gStringVar1, GetMoveName(move));
        StringCopy(gStringVar2, GetMoveVariantName(sTreeScreen->variant));
        DrawPrompt(COMPOUND_STRING("Give {STR_VAR_1} the {STR_VAR_2}\nvariant? This is permanent."));
        sTreeScreen->state = STATE_CONFIRM_CODEX;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseCodex();
    }
}

static void HandleConfirmCodex(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (CheckBagHasItem(ITEM_TECHNIQUE_CODEX, 1)
         && TrySetBoxMonMoveVariant(GetTreeScreenBoxMon(), sTreeScreen->moveCursor, sTreeScreen->variant))
        {
            RemoveBagItem(ITEM_TECHNIQUE_CODEX, 1);
            PlaySE(SE_SUCCESS);
            sTreeScreen->statusText = COMPOUND_STRING("It mastered a new move variant!");
        }
        else
        {
            PlaySE(SE_BOO);
        }
        CloseCodex();
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sTreeScreen->state = STATE_CODEX;
        DrawDescription();
    }
}

static void MoveMutateCursor(s32 dx, s32 dy)
{
    s32 cursor = sTreeScreen->mutateCursor;
    bool32 dual = IsDualTyped(GetTreeScreenSpecies());

    if (cursor == MUTATE_REPLACED_ROW)
    {
        if (dy > 0)
            cursor = sTreeScreen->mutateType;
        else if (dx != 0)
        {
            // Gauche/Droite sur la ligne du haut : l'autre type de l'espèce.
            sTreeScreen->replacedSlot ^= 1;
            PlaySE(SE_SELECT);
            DrawList();
            DrawDescription();
            return;
        }
        else
            return;
    }
    else if (dy < 0 && cursor < MUTATE_COLUMNS)
    {
        if (!dual)
            return;
        cursor = MUTATE_REPLACED_ROW;
    }
    else
    {
        s32 column = cursor % MUTATE_COLUMNS + dx, row = cursor / MUTATE_COLUMNS + dy;

        if (row < 0 || row >= MUTATE_TYPES / MUTATE_COLUMNS)
            return;
        column = (column + MUTATE_COLUMNS) % MUTATE_COLUMNS;
        cursor = row * MUTATE_COLUMNS + column;
        sTreeScreen->mutateType = cursor;
    }
    sTreeScreen->mutateCursor = cursor;
    if (cursor == MUTATE_REPLACED_ROW)
        sTreeScreen->statusText = COMPOUND_STRING("{DPAD_LEFTRIGHT} Choose the type it gives up.");
    else
        sTreeScreen->statusText = NULL;
    PlaySE(SE_SELECT);
    DrawList();
    DrawDescription();
}

static void TryMutate(void)
{
    enum Type newType = GetMutateGridType(sTreeScreen->mutateType);
    enum Type after[2];
    const u8 *refusal;

    if (sTreeScreen->mutateCursor == MUTATE_REPLACED_ROW)
    {
        MoveMutateCursor(1, 0);
        return;
    }
    refusal = GetMutationRefusal(newType);
    if (refusal != NULL)
    {
        PlaySE(SE_BOO);
        sTreeScreen->statusText = refusal;
        DrawDescription();
        return;
    }
    PlaySE(SE_SELECT);
    BuildScreen_GetMutatedTypes(GetTreeScreenSpecies(), sTreeScreen->replacedSlot, newType, after);
    AppendTypes(gStringVar2, after[0], after[1]);
    GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_NICKNAME, gStringVar1);
    if (IsMutationTypeOpen(newType))
        DrawPrompt(COMPOUND_STRING("Mutate {STR_VAR_1} into {STR_VAR_2}?\nThis is forever."));
    else
        DrawPrompt(COMPOUND_STRING("Use a Mutation Seal: {STR_VAR_2}?\nThis is forever."));
    sTreeScreen->state = STATE_CONFIRM_MUTATE;
}

static void HandleConfirmMutate(void)
{
    if (JOY_NEW(A_BUTTON))
    {
        enum Type newType = GetMutateGridType(sTreeScreen->mutateType);

        // Même logique que l'ancien menu (src/tree_menu.c) : Mutation Seal consommé si le type est scellé.
        sTreeScreen->state = STATE_INPUT;
        gSpecialVar_Result = FALSE;
        gSpecialVar_0x8004 = sTreeScreen->monSlot;
        gSpecialVar_0x8006 = GetReplacedType();
        gSpecialVar_0x8007 = newType;
        if (GetMutationRefusal(newType) == NULL)
            MutationMenu_Apply();
        if (gSpecialVar_Result)
        {
            PlaySE(SE_EXP_MAX);
            sTreeScreen->statusText = COMPOUND_STRING("It mutated!");
        }
        else
        {
            PlaySE(SE_BOO);
        }
        DrawAll();
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sTreeScreen->state = STATE_INPUT;
        DrawDescription();
    }
}

static void HandleMutateInput(void)
{
    if (JOY_REPEAT(DPAD_UP))
        MoveMutateCursor(0, -1);
    else if (JOY_REPEAT(DPAD_DOWN))
        MoveMutateCursor(0, 1);
    else if (JOY_REPEAT(DPAD_LEFT))
        MoveMutateCursor(-1, 0);
    else if (JOY_REPEAT(DPAD_RIGHT))
        MoveMutateCursor(1, 0);
    else if (JOY_NEW(A_BUTTON))
        TryMutate();
}

static void HandleInput(void)
{
    // L et R d'abord : avec l'option L=A, L ajoute aussi A.
    if (JOY_NEW(L_BUTTON))
        ChangePage(-1);
    else if (JOY_NEW(R_BUTTON))
        ChangePage(1);
    else if (JOY_NEW(SELECT_BUTTON))
        SwitchToNextMon();
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (GetPendingTotal(sTreeScreen->pending) == 0)
        {
            sTreeScreen->state = STATE_FADE_OUT;
            return;
        }
        ConvertIntToDecimalStringN(gStringVar1, GetPendingTotal(sTreeScreen->pending), STR_CONV_MODE_LEFT_ALIGN, 3);
        DrawPrompt(COMPOUND_STRING("Leave without investing\nthe {STR_VAR_1} new point(s)?"));
        sTreeScreen->state = STATE_CONFIRM_LEAVE;
    }
    else
    {
        if (sTreeScreen->statusText != NULL && (JOY_NEW(DPAD_ANY)))
            sTreeScreen->statusText = NULL;
        switch (sTreeScreen->section)
        {
        case SECTION_TREE:
            HandleTreeInput();
            break;
        case SECTION_MOVES:
            HandleMovesInput();
            break;
        case SECTION_MUTATE:
            HandleMutateInput();
            break;
        }
    }
}

static void HandleConfirm(bool32 invest)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (invest)
        {
            TreePreview_Commit(GetTreeScreenBoxMon(), sTreeScreen->treeType, sTreeScreen->pending);
            CalculateMonStats(&gParties[B_TRAINER_PLAYER][sTreeScreen->monSlot]);
            memset(sTreeScreen->pending, 0, sizeof(sTreeScreen->pending));
            TreePreview_Rebuild(&sTreeScreen->preview, GetTreeScreenBoxMon(), sTreeScreen->treeType, sTreeScreen->pending);
            PlaySE(SE_SUCCESS);
            sTreeScreen->state = STATE_INPUT;
            DrawAll();
        }
        else
        {
            PlaySE(SE_SELECT);
            sTreeScreen->state = STATE_FADE_OUT;
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        sTreeScreen->state = STATE_INPUT;
        DrawDescription();
    }
}

static void CB2_TreeScreen(void)
{
    switch (sTreeScreen->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
            sTreeScreen->state = STATE_INPUT;
        break;
    case STATE_INPUT:
        HandleInput();
        break;
    case STATE_CODEX:
        HandleCodexInput();
        break;
    case STATE_CONFIRM_INVEST:
        HandleConfirm(TRUE);
        break;
    case STATE_CONFIRM_LEAVE:
        HandleConfirm(FALSE);
        break;
    case STATE_CONFIRM_CODEX:
        HandleConfirmCodex();
        break;
    case STATE_CONFIRM_MUTATE:
        HandleConfirmMutate();
        break;
    case STATE_FADE_OUT:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTreeScreen->state = STATE_EXIT;
        break;
    case STATE_EXIT:
        if (!gPaletteFade.active)
        {
            MainCallback callback = sTreeScreen->returnCallback;

            sLastMonSlot = sTreeScreen->monSlot;
            FreeAndDestroyMonIconSprite(&gSprites[sTreeScreen->iconSpriteId]);
            FreeMonIconPalettes();
            FreeAllWindowBuffers();
            TRY_FREE_AND_SET_NULL(sTreeScreen);
            SetMainCallback2(callback);
            return;
        }
        break;
    }
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

static void CB2_OpenTreeScreen(void)
{
    InitTreeScreen();
    sTreeScreen->state = STATE_FADE_IN;
}

// Dernier Pokémon affiché (Select change de Pokémon), pour revenir au menu d'équipe sur lui.
u32 BuildScreen_GetLastMonSlot(void)
{
    return sLastMonSlot;
}

// Ouvre l'écran BUILD sur la page demandée.
void BuildScreen_Open(u32 monSlot, enum BuildScreenPage page, MainCallback returnCallback)
{
    sTreeScreen = AllocZeroed(sizeof(*sTreeScreen));
    sTreeScreen->monSlot = monSlot;
    sTreeScreen->returnCallback = returnCallback;
    switch (page)
    {
    case BUILD_PAGE_TREE:
        sTreeScreen->section = SECTION_TREE;
        break;
    case BUILD_PAGE_MOVES:
        sTreeScreen->section = SECTION_MOVES;
        break;
    case BUILD_PAGE_MUTATE:
        sTreeScreen->section = SECTION_MUTATE;
        break;
    }
    SetMainCallback2(CB2_OpenTreeScreen);
}
