@ Shifting Ruins (mod), donjon rogue-lite à la Lufia 2. Voir src/shifting_ruins.c.
@ Le guide se trouve au Centre Pokémon de Rustboro. Les étages sont ceux de la Pyramide de Combat,
@ dont les scripts renvoient ici quand une tentative est en cours.

EventScript_ShiftingRuinsGuide::
	lock
	faceplayer
	special ShiftingRuins_CheckAccess
	goto_if_eq VAR_RESULT, FALSE, EventScript_ShiftingRuins_TooEarly
	goto_if_set FLAG_MOD_RUINS_EXPLAINED, EventScript_ShiftingRuins_Welcome
	msgbox Text_ShiftingRuins_Intro, MSGBOX_DEFAULT
	setflag FLAG_MOD_RUINS_EXPLAINED
EventScript_ShiftingRuins_Welcome:
	special ShiftingRuins_BufferRecord
	goto_if_eq VAR_RESULT, 0, EventScript_ShiftingRuins_AskEnter
	msgbox Text_ShiftingRuins_Record, MSGBOX_DEFAULT
EventScript_ShiftingRuins_AskEnter:
	msgbox Text_ShiftingRuins_AskEnter, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_ShiftingRuins_Decline
	msgbox Text_ShiftingRuins_SaveFirst, MSGBOX_DEFAULT
	closemessage
	call Common_EventScript_SaveGame
	goto_if_eq VAR_RESULT, 0, EventScript_ShiftingRuins_Decline
	special ShiftingRuins_Start
	callnative UpdateFollowingPokemon
	msgbox Text_ShiftingRuins_Enter, MSGBOX_DEFAULT
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	playse SE_WARP_IN
	warp MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR, 1, 1
	waitstate
	release
	end

EventScript_ShiftingRuins_Decline:
	msgbox Text_ShiftingRuins_Decline, MSGBOX_DEFAULT
	release
	end

EventScript_ShiftingRuins_TooEarly:
	msgbox Text_ShiftingRuins_TooEarly, MSGBOX_DEFAULT
	release
	end

@ Retour à l'entrée après une tentative (appelé par l'arrivée au Centre Pokémon de Rustboro).
ShiftingRuins_EventScript_CheckDebrief::
	special ShiftingRuins_CheckDebrief
	call_if_ne VAR_RESULT, RUINS_END_NONE, ShiftingRuins_EventScript_SetDebrief
	return

ShiftingRuins_EventScript_SetDebrief:
	setvar VAR_TEMP_1, 1
	return

ShiftingRuins_EventScript_Debrief::
	lockall
	setvar VAR_TEMP_1, 0
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_FaceRight
	waitmovement 0
	special ShiftingRuins_CheckDebrief
	goto_if_eq VAR_RESULT, RUINS_END_LOST, ShiftingRuins_EventScript_DebriefLost
	msgbox ShiftingRuins_Text_WelcomeBack, MSGBOX_DEFAULT
	special ShiftingRuins_GiveRelics
	goto_if_eq VAR_RESULT, FALSE, ShiftingRuins_EventScript_ChooseKept
	playfanfare MUS_OBTAIN_ITEM
	message gStringVar4
	waitmessage
	waitfanfare
	waitbuttonpress
ShiftingRuins_EventScript_ChooseKept:
	msgbox ShiftingRuins_Text_ChooseKept, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	special ShiftingRuins_ChooseKeptMon
	goto_if_eq VAR_RESULT, TRUE, ShiftingRuins_EventScript_Finish
	msgbox ShiftingRuins_Text_ConfirmNoKept, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ShiftingRuins_EventScript_ChooseKept
ShiftingRuins_EventScript_Finish:
	special ShiftingRuins_Finish
	callnative UpdateFollowingPokemon
	goto_if_eq VAR_RESULT, MON_GIVEN_TO_PARTY, ShiftingRuins_EventScript_KeptToParty
	goto_if_eq VAR_RESULT, MON_GIVEN_TO_PC, ShiftingRuins_EventScript_KeptToPC
	goto_if_eq VAR_RESULT, MON_CANT_GIVE, ShiftingRuins_EventScript_KeptNoRoom
	goto ShiftingRuins_EventScript_Record

ShiftingRuins_EventScript_KeptToParty:
	msgbox ShiftingRuins_Text_KeptToParty, MSGBOX_DEFAULT
	goto ShiftingRuins_EventScript_Record

ShiftingRuins_EventScript_KeptToPC:
	msgbox ShiftingRuins_Text_KeptToPC, MSGBOX_DEFAULT
	goto ShiftingRuins_EventScript_Record

ShiftingRuins_EventScript_KeptNoRoom:
	msgbox ShiftingRuins_Text_KeptNoRoom, MSGBOX_DEFAULT
ShiftingRuins_EventScript_Record:
	goto_if_eq VAR_0x8005, FALSE, ShiftingRuins_EventScript_Bye
	msgbox ShiftingRuins_Text_NewStart, MSGBOX_DEFAULT
ShiftingRuins_EventScript_Bye:
	msgbox ShiftingRuins_Text_Bye, MSGBOX_DEFAULT
	releaseall
	end

ShiftingRuins_EventScript_DebriefLost:
	msgbox ShiftingRuins_Text_WelcomeBackLost, MSGBOX_DEFAULT
	special ShiftingRuins_Finish
	callnative UpdateFollowingPokemon
	goto ShiftingRuins_EventScript_Bye

@ Étages (appelés depuis data/maps/BattleFrontier_BattlePyramidFloor/scripts.inc).
ShiftingRuins_EventScript_FloorOnResume::
	pyramid_settrainers
	special ShiftingRuins_OnFloorResume
	goto_if_eq VAR_RESULT, RUINS_RESUME_LOST, ShiftingRuins_EventScript_RunLost
	goto_if_eq VAR_RESULT, RUINS_RESUME_WON, ShiftingRuins_EventScript_LightUp
	end

ShiftingRuins_EventScript_LightUp:
	setvar VAR_TEMP_D, 1
	end

ShiftingRuins_EventScript_Retire::
ShiftingRuins_EventScript_RunLost::
	setvar VAR_0x8004, RUINS_END_LOST
	special ShiftingRuins_EndRun
ShiftingRuins_EventScript_WarpOut:
	pyramid_updatelight 0, PYRAMID_LIGHT_SET_RADIUS
	warpsilent MAP_RUSTBORO_CITY_POKEMON_CENTER_1F, 10, 2
	waitstate
	end

ShiftingRuins_EventScript_Stairs::
	lockall
	special ShiftingRuins_CheckStairs
	goto_if_eq VAR_RESULT, RUINS_STAIRS_BOTTOM, ShiftingRuins_EventScript_Bottom
	goto_if_eq VAR_RESULT, RUINS_STAIRS_LANDING, ShiftingRuins_EventScript_Landing
ShiftingRuins_EventScript_GoDeeper:
	special ShiftingRuins_GoDeeper
	setvar VAR_RESULT, 0
	warp MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR, 1, 1
	waitstate
	end

ShiftingRuins_EventScript_Landing:
	msgbox ShiftingRuins_Text_Landing, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, ShiftingRuins_EventScript_Secured
	closemessage
	goto ShiftingRuins_EventScript_GoDeeper

ShiftingRuins_EventScript_Bottom:
	msgbox ShiftingRuins_Text_Bottom, MSGBOX_DEFAULT
ShiftingRuins_EventScript_Secured::
	closemessage
	setvar VAR_0x8004, RUINS_END_SECURED
	special ShiftingRuins_EndRun
	goto ShiftingRuins_EventScript_WarpOut

ShiftingRuins_EventScript_FindRelic::
	lock
	special ShiftingRuins_TakeRelic
	playfanfare MUS_OBTAIN_ITEM
	message ShiftingRuins_Text_FoundRelic
	waitmessage
	waitfanfare
	msgbox ShiftingRuins_Text_RelicPending, MSGBOX_DEFAULT
	pyramid_hideitem
	release
	end

@ Escape Orb utilisé depuis le sac des ruines (src/item_use.c).
ShiftingRuins_EventScript_EscapeOrb::
	lockall
	msgbox ShiftingRuins_Text_EscapeOrb, MSGBOX_DEFAULT
	goto ShiftingRuins_EventScript_Secured

Text_ShiftingRuins_TooEarly:
	.string "I guide trainers into the SHIFTING\n"
	.string "RUINS. Their halls change on every\l"
	.string "visit.\p"
	.string "Come back once you hold {STR_VAR_1}\n"
	.string "or more GYM BADGES.$"

Text_ShiftingRuins_Intro:
	.string "I guide trainers into the SHIFTING\n"
	.string "RUINS. Their halls change on every\l"
	.string "visit.\p"
	.string "Your team waits here with me. You\n"
	.string "go in with a random POKéMON at\l"
	.string "level 5 and a small BAG.\p"
	.string "Battle, catch and grow your team\n"
	.string "on the way down.\p"
	.string "Rare finds, like TECHNIQUE CODICES,\n"
	.string "are only yours if you get out.\p"
	.string "You can leave every {STR_VAR_2} floors,\n"
	.string "or with an ESCAPE ORB.\p"
	.string "Fall, and every rare find of that\n"
	.string "run is lost.\p"
	.string "When you get out, bring back one\n"
	.string "POKéMON. It returns to level 5 and\l"
	.string "forgets what it learned inside.\p"
	.string "The deeper you get out from, the\n"
	.string "better your next start.$"

Text_ShiftingRuins_Record:
	.string "Your best escape was from floor\n"
	.string "{STR_VAR_1}. You'll start with\l"
	.string "{STR_VAR_2} POKéMON.$"

Text_ShiftingRuins_AskEnter:
	.string "Will you enter the SHIFTING RUINS?$"

Text_ShiftingRuins_SaveFirst:
	.string "First, save your adventure. Nothing\n"
	.string "can be saved inside the ruins.$"

Text_ShiftingRuins_Enter:
	.string "Your team will be safe with me.\n"
	.string "Here are your partners for this run.$"

Text_ShiftingRuins_Decline:
	.string "The ruins will wait. They always do.$"

ShiftingRuins_Text_WelcomeBack:
	.string "You made it out! Everything you\n"
	.string "found in there is yours.$"

ShiftingRuins_Text_ChooseKept:
	.string "Choose one POKéMON to bring back.\n"
	.string "It will return to level 5.$"

ShiftingRuins_Text_ConfirmNoKept:
	.string "Leave without bringing a POKéMON\n"
	.string "back?$"

ShiftingRuins_Text_KeptToParty:
	.string "{STR_VAR_1} is back to level 5\n"
	.string "and joins your team.$"

ShiftingRuins_Text_KeptToPC:
	.string "{STR_VAR_1} is back to level 5.\n"
	.string "It was sent to the PC.$"

ShiftingRuins_Text_KeptNoRoom:
	.string "There's no room left for\n"
	.string "{STR_VAR_1}…\p"
	.string "It went back into the ruins.$"

ShiftingRuins_Text_NewStart:
	.string "Getting out from floor {STR_VAR_2}…\n"
	.string "Impressive!\p"
	.string "Next time, you'll start with more\n"
	.string "or stronger POKéMON.$"

ShiftingRuins_Text_Bye:
	.string "Here is your team. Come back any\n"
	.string "time. The ruins will have shifted.$"

ShiftingRuins_Text_WelcomeBackLost:
	.string "You were carried out of the ruins…\p"
	.string "The rare finds of this run were\n"
	.string "lost in there.$"

ShiftingRuins_Text_Landing:
	.string "Floor {STR_VAR_1}. Daylight comes down\n"
	.string "from above.\p"
	.string "Leave the ruins now and keep\n"
	.string "your finds?$"

ShiftingRuins_Text_Bottom:
	.string "This is the heart of the ruins.\n"
	.string "There is nowhere deeper to go.\p"
	.string "A path leads back to daylight.$"

ShiftingRuins_Text_FoundRelic:
	.string "{PLAYER} found a {STR_VAR_2}!$"

ShiftingRuins_Text_RelicPending:
	.string "It's only yours if you make it out\n"
	.string "of the ruins.$"

ShiftingRuins_Text_EscapeOrb:
	.string "{PLAYER} used the ESCAPE ORB.\n"
	.string "Daylight rushed in!$"
