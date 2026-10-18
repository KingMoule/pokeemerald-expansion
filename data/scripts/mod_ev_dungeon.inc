@ Donjon d'entraînement des EV (mod), « Training Depths ». Voir src/ev_dungeon.c.
@ Le guide se trouve au Centre Pokémon de Mauville.
EventScript_TrainingDepthsGuide::
	lock
	faceplayer
	special EvDungeon_CheckAccess
	goto_if_eq VAR_RESULT, FALSE, EventScript_TrainingDepths_TooEarly
	goto_if_set FLAG_MOD_EV_DUNGEON_EXPLAINED, EventScript_TrainingDepths_Welcome
	msgbox Text_TrainingDepths_Intro, MSGBOX_DEFAULT
	setflag FLAG_MOD_EV_DUNGEON_EXPLAINED
EventScript_TrainingDepths_Welcome:
	msgbox Text_TrainingDepths_Welcome, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_TrainingDepths_Decline
EventScript_TrainingDepths_ChooseMon:
	msgbox Text_TrainingDepths_ChooseMon, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, EventScript_TrainingDepths_Decline
	special EvDungeon_SetBeneficiary
	goto_if_eq VAR_RESULT, FALSE, EventScript_TrainingDepths_NoEgg
	message Text_TrainingDepths_ChoosePath
	waitmessage
	special EvDungeon_PushPaths
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_DESCRIBE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_TrainingDepths_Decline
	copyvar VAR_0x8005, VAR_RESULT
	setvar VAR_0x8006, 1
	special EvDungeon_PushStartFloors
	goto_if_le VAR_RESULT, 1, EventScript_TrainingDepths_Enter
	message Text_TrainingDepths_ChooseFloor
	waitmessage
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_DESCRIBE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_TrainingDepths_Decline
	copyvar VAR_0x8006, VAR_RESULT
	goto EventScript_TrainingDepths_Enter

EventScript_TrainingDepths_Enter:
	special EvDungeon_Start
	msgbox Text_TrainingDepths_Enter, MSGBOX_DEFAULT
	closemessage
EventScript_TrainingDepths_Floor:
	fadescreen FADE_TO_BLACK
	special EvDungeon_BeginFloor
	fadescreen FADE_FROM_BLACK
	msgbox Text_TrainingDepths_Floor, MSGBOX_DEFAULT
	closemessage
	setvar VAR_0x8004, SPECIAL_BATTLE_EV_DUNGEON
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	special EvDungeon_EndFloor
	goto_if_eq VAR_RESULT, EV_FLOOR_END, EventScript_TrainingDepths_End
	copyvar VAR_0x8007, VAR_RESULT
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto_if_eq VAR_0x8007, EV_FLOOR_NEXT, EventScript_TrainingDepths_Floor
	msgbox Text_TrainingDepths_ContinueOrLeave, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, EventScript_TrainingDepths_Floor
	special EvDungeon_Leave
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto EventScript_TrainingDepths_Bye

EventScript_TrainingDepths_End:
	msgbox gStringVar4, MSGBOX_DEFAULT
EventScript_TrainingDepths_Bye:
	callnative UpdateFollowingPokemon
	msgbox Text_TrainingDepths_Bye, MSGBOX_DEFAULT
	release
	end

EventScript_TrainingDepths_NoEgg:
	msgbox Text_TrainingDepths_NoEgg, MSGBOX_DEFAULT
	goto EventScript_TrainingDepths_ChooseMon

EventScript_TrainingDepths_Decline:
	msgbox Text_TrainingDepths_Decline, MSGBOX_DEFAULT
	release
	end

EventScript_TrainingDepths_TooEarly:
	msgbox Text_TrainingDepths_TooEarly, MSGBOX_DEFAULT
	release
	end

Text_TrainingDepths_TooEarly:
	.string "Below this CENTER lie the TRAINING\n"
	.string "DEPTHS, where POKéMON hone their\l"
	.string "strengths.\p"
	.string "Come back once you have earned\n"
	.string "{STR_VAR_1} BADGES.$"

Text_TrainingDepths_Intro:
	.string "Below this CENTER lie the TRAINING\n"
	.string "DEPTHS.\p"
	.string "Battles out there don't build a\n"
	.string "POKéMON's stats. Training here does.\p"
	.string "Choose one POKéMON to train and a\n"
	.string "path. Each path trains one stat.\l"
	.string "Your whole team fights alongside.\p"
	.string "Every floor won earns EVs, but they\n"
	.string "stay unsecured until you leave.\p"
	.string "You may only leave at rest points.\n"
	.string "Deeper floors are tougher, and pay\l"
	.string "more.\p"
	.string "Lose, and you keep only part of the\n"
	.string "unsecured EVs. EVs earned before\l"
	.string "the visit are always safe.$"

Text_TrainingDepths_Welcome:
	.string "Will you enter the TRAINING DEPTHS?$"

Text_TrainingDepths_ChooseMon:
	.string "Which POKéMON will train?$"

Text_TrainingDepths_NoEgg:
	.string "An EGG can't train here.$"

Text_TrainingDepths_ChoosePath:
	.string "Which path will {STR_VAR_1} take?$"

Text_TrainingDepths_ChooseFloor:
	.string "Your past descents opened shortcuts.\n"
	.string "Start from which floor?$"

Text_TrainingDepths_Enter:
	.string "Your POKéMON were healed.\n"
	.string "Down you go!$"

Text_TrainingDepths_Floor:
	.string "Floor {STR_VAR_1}.\n"
	.string "A trainer blocks the way!$"

Text_TrainingDepths_ContinueOrLeave:
	.string "Go deeper? Choose NO to leave and\n"
	.string "secure your gains.$"

Text_TrainingDepths_Decline:
	.string "Come back whenever you wish to\n"
	.string "train.$"

Text_TrainingDepths_Bye:
	.string "Train hard, and come back soon!$"
