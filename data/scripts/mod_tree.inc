@ Écran BUILD (mod), ouvert depuis le menu d'équipe : arbre élémentaire et variantes d'attaques.
@ VAR_0x8004 : emplacement du Pokémon. Voir src/tree_menu.c.
EventScript_BuildMenu::
	lockall
	special BuildMenu_Begin
	goto_if_eq VAR_RESULT, FALSE, EventScript_BuildMenu_End
EventScript_BuildMenu_Hub:
	message Text_BuildMenu_Hub
	waitmessage
	dynmultipush Text_BuildMenu_Tree, 0
	dynmultipush Text_BuildMenu_Moves, 1
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, 0, EventScript_TreeMenu
	goto_if_eq VAR_RESULT, 1, EventScript_VariantMenu
EventScript_BuildMenu_End:
	closemessage
	releaseall
	end

@ Arbre élémentaire
EventScript_TreeMenu:
	special TreeMenu_Begin
	goto_if_eq VAR_RESULT, 0, EventScript_BuildMenu_End
	goto_if_eq VAR_RESULT, 1, EventScript_TreeMenu_Loop
	message Text_TreeMenu_ChooseTree
	waitmessage
	special TreeMenu_PushTypeChoices
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_BuildMenu_Hub
	copyvar VAR_0x8005, VAR_RESULT
EventScript_TreeMenu_Loop:
	special TreeMenu_BufferHeader
	message Text_TreeMenu_Header
	waitmessage
	special TreeMenu_PushNodes
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_0x8006, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_BuildMenu_Hub
	copyvar VAR_0x8006, VAR_RESULT
	special TreeMenu_CheckNode
	goto_if_ne VAR_RESULT, 0, EventScript_TreeMenu_Refused
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_TreeMenu_Loop
	special TreeMenu_Invest
	playse SE_SUCCESS
	waitse
	goto EventScript_TreeMenu_Loop

EventScript_TreeMenu_Refused:
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto EventScript_TreeMenu_Loop

@ Variantes d'attaques, consultation depuis BUILD : seul le Technique Codex en donne une.
EventScript_VariantMenu:
	message Text_VariantMenu_ViewMoves
	waitmessage
	special VariantMenu_PushMoves
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_0x8007, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_BuildMenu_Hub
	copyvar VAR_0x8007, VAR_RESULT
	special VariantMenu_CheckMove
	goto_if_ne VAR_RESULT, 0, EventScript_VariantMenu_Info
	msgbox Text_VariantMenu_NeedCodex, MSGBOX_DEFAULT
	goto EventScript_VariantMenu

EventScript_VariantMenu_Info:
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto EventScript_VariantMenu

@ Technique Codex utilisé depuis le sac. L'objet n'est retiré qu'après confirmation.
EventScript_TechniqueCodex::
	lockall
	special BuildMenu_Begin
	goto_if_eq VAR_RESULT, FALSE, EventScript_TechniqueCodex_NoEffect
EventScript_TechniqueCodex_ChooseMove:
	message Text_VariantMenu_ChooseMove
	waitmessage
	special VariantMenu_PushMoves
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_0x8007, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_BuildMenu_End
	copyvar VAR_0x8007, VAR_RESULT
	special VariantMenu_CheckMove
	goto_if_ne VAR_RESULT, 0, EventScript_TechniqueCodex_Refused
	message Text_VariantMenu_ChooseVariant
	waitmessage
	special VariantMenu_PushVariants
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_TechniqueCodex_ChooseMove
	copyvar VAR_0x8008, VAR_RESULT
	special VariantMenu_BufferVariant
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_TechniqueCodex_ChooseMove
	special VariantMenu_Apply
	removeitem ITEM_TECHNIQUE_CODEX
	playse SE_SUCCESS
	waitse
	msgbox Text_VariantMenu_Learned, MSGBOX_DEFAULT
	goto EventScript_BuildMenu_End

EventScript_TechniqueCodex_Refused:
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto EventScript_TechniqueCodex_ChooseMove

EventScript_TechniqueCodex_NoEffect:
	msgbox Text_VariantMenu_NoEffect, MSGBOX_DEFAULT
	goto EventScript_BuildMenu_End

Text_BuildMenu_Hub:
	.string "What will you train for\n"
	.string "{STR_VAR_1}?$"

Text_BuildMenu_Tree:
	.string "Tree$"

Text_BuildMenu_Moves:
	.string "Moves$"

Text_TreeMenu_ChooseTree:
	.string "Which tree should {STR_VAR_1}\n"
	.string "follow? This is permanent.$"

Text_TreeMenu_Header:
	.string "{STR_VAR_1} · {STR_VAR_2} Tree\n"
	.string "Available points: {STR_VAR_3}$"

Text_VariantMenu_ViewMoves:
	.string "Check the variant of\n"
	.string "which move?$"

Text_VariantMenu_NeedCodex:
	.string "No variant yet. A Technique\n"
	.string "Codex can give it one.$"

Text_VariantMenu_Learned:
	.string "{STR_VAR_1} mastered a new\n"
	.string "move variant!$"

Text_VariantMenu_NoEffect:
	.string "It won't have any effect.$"

Text_VariantMenu_ChooseMove:
	.string "Choose a move to give\n"
	.string "a variant.$"

Text_VariantMenu_ChooseVariant:
	.string "Choose a variant.\n"
	.string "Only one per move.$"
