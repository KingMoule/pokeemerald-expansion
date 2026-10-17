@ Écran d'investissement des points d'arbre (mod), ouvert depuis le menu d'équipe.
@ VAR_0x8004 : emplacement du Pokémon. Voir src/tree_menu.c.
EventScript_TreeMenu::
	lockall
	special TreeMenu_Begin
	goto_if_eq VAR_RESULT, 0, EventScript_TreeMenu_End
	goto_if_eq VAR_RESULT, 1, EventScript_TreeMenu_Loop
	message Text_TreeMenu_ChooseTree
	waitmessage
	special TreeMenu_PushTypeChoices
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_TreeMenu_End
	copyvar VAR_0x8005, VAR_RESULT
EventScript_TreeMenu_Loop:
	special TreeMenu_BufferHeader
	message Text_TreeMenu_Header
	waitmessage
	special TreeMenu_PushNodes
	dynmultistack 0, 0, FALSE, 6, FALSE, VAR_0x8006, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_TreeMenu_End
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

EventScript_TreeMenu_End:
	closemessage
	releaseall
	end

Text_TreeMenu_ChooseTree:
	.string "Which tree should {STR_VAR_1}\n"
	.string "follow? This is permanent.$"

Text_TreeMenu_Header:
	.string "{STR_VAR_1} · {STR_VAR_2} Tree\n"
	.string "Available points: {STR_VAR_3}$"
