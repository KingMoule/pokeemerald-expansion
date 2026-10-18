#include "global.h"
#include "battle_main.h"
#include "event_data.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
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
#include "tree_screen.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// Écran plein du skill tree : en-tête (Pokémon, arbre, points), onglets Trunk et branches (L/R),
// les quatre nœuds de l'onglet avec une barre de rang, la description et l'aide des boutons.
// Gauche/Droite (ou A) placent des points en attente ; Start les investit tous après une seule
// confirmation. Les points déjà investis restent définitifs.

#define TREE_TAB_COUNT (1 + TREE_BRANCH_COUNT)
#define TREE_TAB_NODES 4

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
    STATE_CONFIRM_INVEST,
    STATE_CONFIRM_LEAVE,
    STATE_FADE_OUT,
    STATE_EXIT,
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
static const u8 sColors_Arrow[]      = { COL_HIGHLIGHT, COL_HEADER, COL_TRANSPARENT };
static const u8 sColors_Header[]     = { COL_HEADER, COL_WHITE, COL_HEADER_DARK };
static const u8 sColors_HeaderNum[]  = { COL_HEADER, COL_YELLOW, COL_HEADER_DARK };
static const u8 sColors_TabOn[]      = { COL_WHITE, COL_DARK, COL_LIGHT };
static const u8 sColors_TabOff[]     = { COL_TAB, COL_BROWN, COL_TRANSPARENT };
static const u8 sColors_Cream[]      = { COL_CREAM, COL_DARK, COL_TRANSPARENT };
static const u8 sColors_Status[]     = { COL_WHITE, COL_HEADER_DARK, COL_TRANSPARENT };
static const u8 sColors_Help[]       = { COL_HELP, COL_WHITE, COL_DARK };

struct TreeScreen
{
    struct BoxPokemon preview; // Copie du Pokémon avec les points en attente appliqués.
    u8 pending[MOD_TREE_NODE_COUNT];
    u8 state;
    u8 tab;
    u8 cursor;
    u8 monSlot;
    u8 iconSpriteId;
    enum Type treeType;
};

static EWRAM_DATA struct TreeScreen *sTreeScreen = NULL;

static struct BoxPokemon *GetTreeScreenBoxMon(void)
{
    return &gParties[B_TRAINER_PLAYER][sTreeScreen->monSlot].box;
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

static const u8 *GetTabName(u32 tab)
{
    if (tab == 0)
        return COMPOUND_STRING("Trunk");
    return gElementTrees[sTreeScreen->treeType].branchNames[tab - 1];
}

static void DrawHeader(void)
{
    u8 text[32];
    u8 *end;
    u32 available = GetBoxMonTreePointsAvailable(&sTreeScreen->preview);
    u32 pendingTotal = GetPendingTotal(sTreeScreen->pending);
    u32 x;

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(COL_HEADER));
    FillWindowPixelRect(WIN_HEADER, PIXEL_FILL(COL_HEADER_DARK), 0, 15, 240, 1);
    GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_NICKNAME, text);
    AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, 32, 0, sColors_Header, TEXT_SKIP_DRAW, text);
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
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static void DrawTabs(void)
{
    u32 tab, x, font = FONT_SMALL, total = 0;

    for (tab = 0; tab < TREE_TAB_COUNT; tab++)
        total += GetStringWidth(FONT_SMALL, GetTabName(tab), 0) + 10;
    if (total > 196)
        font = FONT_SMALL_NARROW;

    FillWindowPixelBuffer(WIN_TABS, PIXEL_FILL(COL_CREAM));
    AddTextPrinterParameterized3(WIN_TABS, FONT_SMALL, 2, 3, sColors_Cream, TEXT_SKIP_DRAW, COMPOUND_STRING("{L_BUTTON}"));
    AddTextPrinterParameterized3(WIN_TABS, FONT_SMALL, 222, 3, sColors_Cream, TEXT_SKIP_DRAW, COMPOUND_STRING("{R_BUTTON}"));
    FillWindowPixelRect(WIN_TABS, PIXEL_FILL(COL_BROWN), 0, 15, 240, 1);

    x = 20;
    for (tab = 0; tab < TREE_TAB_COUNT; tab++)
    {
        const u8 *name = GetTabName(tab);
        u32 width = GetStringWidth(font, name, 0) + 8;
        bool32 selected = (tab == sTreeScreen->tab);

        FillWindowPixelRect(WIN_TABS, PIXEL_FILL(COL_BROWN), x, 2, width, 14);
        FillWindowPixelRect(WIN_TABS, PIXEL_FILL(selected ? COL_WHITE : COL_TAB), x + 1, 3, width - 2, selected ? 13 : 12);
        AddTextPrinterParameterized3(WIN_TABS, font, x + 4, 3, selected ? sColors_TabOn : sColors_TabOff, TEXT_SKIP_DRAW, name);
        x += width + 2;
    }
    CopyWindowToVram(WIN_TABS, COPYWIN_FULL);
}

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

static void DrawList(void)
{
    u32 i, first = GetTabFirstNode(sTreeScreen->tab);
    u8 text[16];
    u8 *end;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(COL_WHITE));
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
        {
            FillWindowPixelRect(WIN_LIST, PIXEL_FILL(COL_HIGHLIGHT), 3, y, 234, 16);
            AddTextPrinterParameterized3(WIN_LIST, FONT_NORMAL, 6, y, sColors_Arrow, TEXT_SKIP_DRAW, COMPOUND_STRING("{RIGHT_ARROW}"));
        }
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
    CopyWindowToVram(WIN_LIST, COPYWIN_FULL);
}

static void BeginDescWindow(void)
{
    FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(COL_WHITE));
    FillWindowPixelRect(WIN_DESC, PIXEL_FILL(COL_BROWN), 0, 0, 240, 1);
}

// Description du nœud choisi, puis ce qui l'empêche de monter.
static void DrawDescription(void)
{
    u32 node = GetTabFirstNode(sTreeScreen->tab) + sTreeScreen->cursor;
    const struct TreeNode *treeNode = GetTreeNode(sTreeScreen->treeType, node);
    u8 *end = gStringVar4;

    BeginDescWindow();
    if (treeNode != NULL)
    {
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
        AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 8, 34, sColors_Status, TEXT_SKIP_DRAW, gStringVar4);
    }
    CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
}

static void DrawPrompt(const u8 *question)
{
    ConvertIntToDecimalStringN(gStringVar1, GetPendingTotal(sTreeScreen->pending), STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar4, question);
    BeginDescWindow();
    AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, 8, 2, sColors_Dark, TEXT_SKIP_DRAW, gStringVar4);
    AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 8, 34, sColors_Dark, TEXT_SKIP_DRAW, COMPOUND_STRING("{A_BUTTON} Yes    {B_BUTTON} No"));
    CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
}

static void DrawHelp(void)
{
    FillWindowPixelBuffer(WIN_HELP, PIXEL_FILL(COL_HELP));
    AddTextPrinterParameterized3(WIN_HELP, FONT_SMALL, 3, 2, sColors_Help, TEXT_SKIP_DRAW,
        COMPOUND_STRING("{DPAD_LEFTRIGHT}Point  {L_BUTTON}{R_BUTTON}Branch  {START_BUTTON}Confirm  {B_BUTTON}Back"));
    CopyWindowToVram(WIN_HELP, COPYWIN_FULL);
}

static void DrawAll(void)
{
    DrawHeader();
    DrawTabs();
    DrawList();
    DrawDescription();
}

static void VBlankCB_TreeScreen(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_TreeScreen(void);

static void InitTreeScreen(void)
{
    struct BoxPokemon *boxMon = GetTreeScreenBoxMon();
    enum Species species = GetBoxMonData(boxMon, MON_DATA_SPECIES);

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
    sTreeScreen->iconSpriteId = CreateMonIcon(species, SpriteCB_MonIcon, 16, 8, 0, GetBoxMonData(boxMon, MON_DATA_PERSONALITY));
    gSprites[sTreeScreen->iconSpriteId].oam.priority = 0;

    TreePreview_Rebuild(&sTreeScreen->preview, boxMon, sTreeScreen->treeType, sTreeScreen->pending);
    DrawAll();
    DrawHelp();
    for (u32 win = WIN_HEADER; win <= WIN_HELP; win++)
        PutWindowTilemap(win);
    CopyBgTilemapBufferToVram(0);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_BG0_ON);
    ShowBg(0);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    SetVBlankCallback(VBlankCB_TreeScreen);
    SetMainCallback2(CB2_TreeScreen);
}

static void ChangeTab(s32 delta)
{
    sTreeScreen->tab = (sTreeScreen->tab + TREE_TAB_COUNT + delta) % TREE_TAB_COUNT;
    PlaySE(SE_SELECT);
    DrawTabs();
    DrawList();
    DrawDescription();
}

static void MoveCursor(s32 delta)
{
    sTreeScreen->cursor = (sTreeScreen->cursor + TREE_TAB_NODES + delta) % TREE_TAB_NODES;
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

static void HandleInput(void)
{
    // L et R d'abord : avec l'option L=A, L ajoute aussi A.
    if (JOY_NEW(L_BUTTON))
        ChangeTab(-1);
    else if (JOY_NEW(R_BUTTON))
        ChangeTab(1);
    else if (JOY_REPEAT(DPAD_UP))
        MoveCursor(-1);
    else if (JOY_REPEAT(DPAD_DOWN))
        MoveCursor(1);
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
        DrawPrompt(COMPOUND_STRING("Invest {STR_VAR_1} point(s)?\nThis is permanent."));
        sTreeScreen->state = STATE_CONFIRM_INVEST;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (GetPendingTotal(sTreeScreen->pending) == 0)
        {
            sTreeScreen->state = STATE_FADE_OUT;
            return;
        }
        DrawPrompt(COMPOUND_STRING("Leave without investing\nthe {STR_VAR_1} new point(s)?"));
        sTreeScreen->state = STATE_CONFIRM_LEAVE;
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
            DrawAll();
            sTreeScreen->state = STATE_INPUT;
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
        DrawDescription();
        sTreeScreen->state = STATE_INPUT;
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
    case STATE_CONFIRM_INVEST:
        HandleConfirm(TRUE);
        break;
    case STATE_CONFIRM_LEAVE:
        HandleConfirm(FALSE);
        break;
    case STATE_FADE_OUT:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sTreeScreen->state = STATE_EXIT;
        break;
    case STATE_EXIT:
        if (!gPaletteFade.active)
        {
            // Le script reprend sur « What will you train for {STR_VAR_1}? ».
            GetBoxMonData(GetTreeScreenBoxMon(), MON_DATA_NICKNAME, gStringVar1);
            FreeAndDestroyMonIconSprite(&gSprites[sTreeScreen->iconSpriteId]);
            FreeMonIconPalettes();
            FreeAllWindowBuffers();
            TRY_FREE_AND_SET_NULL(sTreeScreen);
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
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

// Special (waitstate) : ouvre l'écran, puis reprend le script sur la carte en sortant.
void TreeScreen_Open(void)
{
    sTreeScreen = AllocZeroed(sizeof(*sTreeScreen));
    sTreeScreen->monSlot = gSpecialVar_0x8004;
    sTreeScreen->treeType = gSpecialVar_0x8005;
    SetMainCallback2(CB2_OpenTreeScreen);
}
