@ Défis à règles annoncées (mod, wiki sections 13 et 14). Voir src/mod_challenge.c.
@ Après le Panthéon, l'expert du Centre Pokémon d'Ever Grande devient le maître des défis.
EventScript_ChallengeMaster::
	lock
	faceplayer
	goto_if_set FLAG_MOD_CHALLENGES_EXPLAINED, EventScript_Challenge_Choose
	msgbox Text_Challenge_Intro, MSGBOX_DEFAULT
	setflag FLAG_MOD_CHALLENGES_EXPLAINED
EventScript_Challenge_Choose:
	message Text_Challenge_Choose
	waitmessage
	special ModChallenge_PushList
	dynmultistack 0, 0, FALSE, 6, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EventScript_Challenge_Decline
	copyvar VAR_0x8005, VAR_RESULT
	special ModChallenge_BufferRules
	copyvar VAR_0x8006, VAR_RESULT
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto_if_eq VAR_0x8006, TRUE, EventScript_Challenge_Confirm
	msgbox Text_Challenge_Reward, MSGBOX_DEFAULT
EventScript_Challenge_Confirm:
	msgbox Text_Challenge_Confirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_Challenge_Decline
	special ModChallenge_CheckParty
	goto_if_eq VAR_RESULT, FALSE, EventScript_Challenge_NotAllowed
	special ModChallenge_Start
	msgbox Text_Challenge_Begin, MSGBOX_DEFAULT
	closemessage
EventScript_Challenge_Battle:
	setvar VAR_0x8004, SPECIAL_BATTLE_MOD_CHALLENGE
	special DoSpecialTrainerBattle
	waitstate
	special ModChallenge_AfterBattle
	goto_if_eq VAR_RESULT, CHALLENGE_NEXT_BATTLE, EventScript_Challenge_Next
	goto_if_eq VAR_RESULT, CHALLENGE_WON, EventScript_Challenge_Won
	setvar VAR_0x8004, FALSE
	special ModChallenge_Finish
	callnative UpdateFollowingPokemon
	msgbox Text_Challenge_Lost, MSGBOX_DEFAULT
	release
	end

EventScript_Challenge_Next:
	msgbox Text_Challenge_Next, MSGBOX_DEFAULT
	closemessage
	goto EventScript_Challenge_Battle

EventScript_Challenge_Won:
	setvar VAR_0x8004, TRUE
	special ModChallenge_Finish
	callnative UpdateFollowingPokemon
	copyvar VAR_0x8006, VAR_RESULT
	goto_if_eq VAR_0x8006, ITEM_NONE, EventScript_Challenge_WonAgain
	msgbox Text_Challenge_Won, MSGBOX_DEFAULT
	giveitem VAR_0x8006
	msgbox Text_Challenge_Bye, MSGBOX_DEFAULT
	release
	end

EventScript_Challenge_WonAgain:
	msgbox Text_Challenge_WonAgain, MSGBOX_DEFAULT
	release
	end

EventScript_Challenge_NotAllowed:
	msgbox Text_Challenge_NotAllowed, MSGBOX_DEFAULT
	release
	end

EventScript_Challenge_Decline:
	msgbox Text_Challenge_Decline, MSGBOX_DEFAULT
	release
	end

Text_Challenge_Intro:
	.string "So you're the new CHAMPION.\n"
	.string "Then you're ready for my TRIALS.\p"
	.string "Each TRIAL has its own rules, and\n"
	.string "I always tell them up front.\p"
	.string "Build a team that fits the rules.\n"
	.string "My trainers have builds of their\l"
	.string "own, mutations included.\p"
	.string "Lose, and nothing is taken from you.\n"
	.string "Win a TRIAL for the first time, and\l"
	.string "I'll reward you.$"

Text_Challenge_Choose:
	.string "Which TRIAL interests you?$"

Text_Challenge_Reward:
	.string "Clear it for the first time, and\n"
	.string "you'll get a {STR_VAR_2}.$"

Text_Challenge_Confirm:
	.string "Take on the {STR_VAR_1}\n"
	.string "with your current team?$"

Text_Challenge_NotAllowed:
	.string "Your team doesn't follow the rules\n"
	.string "of this TRIAL.\p"
	.string "Change your party and come back.$"

Text_Challenge_Begin:
	.string "Your POKéMON were healed.\n"
	.string "Let the TRIAL begin!$"

Text_Challenge_Next:
	.string "One down! No healing, though.\n"
	.string "Here comes the next trainer!$"

Text_Challenge_Lost:
	.string "That TRIAL got the better of you.\p"
	.string "Your POKéMON were healed.\n"
	.string "Rethink your team and try again!$"

Text_Challenge_Won:
	.string "Splendid! You cleared the\n"
	.string "{STR_VAR_1}!\p"
	.string "Here's your reward.$"

Text_Challenge_WonAgain:
	.string "Splendid! You cleared it again.\n"
	.string "Your POKéMON were healed.$"

Text_Challenge_Bye:
	.string "Your POKéMON were healed.\n"
	.string "Come back for another TRIAL!$"

Text_Challenge_Decline:
	.string "Come back whenever you feel ready.$"
