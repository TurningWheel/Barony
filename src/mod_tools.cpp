/*-------------------------------------------------------------------------------

BARONY
File: mod_tools.cpp
Desc: misc modding tools

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/
#include "items.hpp"
#include "mod_tools.hpp"
#include "menu.hpp"
#include "classdescriptions.hpp"

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
GameplayCustomManager gameplayCustomManager;
GameModeManager_t gameModeManager;
const std::vector<std::string> MonsterStatCustomManager::itemStatusStrings =
{
	"broken",
	"decrepit",
	"worn",
	"serviceable",
	"excellent"
};

const std::vector<std::string> MonsterStatCustomManager::shopkeeperTypeStrings =
{
	"equipment",
	"hats",
	"jewelry",
	"books",
	"apothecary",
	"staffs",
	"food",
	"hardware",
	"hunting",
	"general"
};

void GameModeManager_t::Tutorial_t::startTutorial()
{
	isFirstTimeLaunch ? setTutorialMap(std::string("tutorial1.lmp")) : launchHub();
	gameModeManager.setMode(gameModeManager.GameModes::GAME_MODE_TUTORIAL);
	stats[0]->clearStats();
	strcpy(stats[0]->name, "Player");
	stats[0]->sex = static_cast<sex_t>(rand() % 2);
	stats[0]->playerRace = RACE_HUMAN;
	stats[0]->appearance = rand() % NUMAPPEARANCES;
	initClass(0);
}

void GameModeManager_t::Tutorial_t::buttonReturnToTutorialHub(button_t* my)
{
	buttonStartSingleplayer(nullptr);
	gameModeManager.Tutorial.launchHub();
}

void GameModeManager_t::Tutorial_t::buttonRestartTrial(button_t* my)
{
	std::string mapname = map.name;
	if ( mapname.find("Tutorial Hub") == std::string::npos
		&& mapname.find("Tutorial ") != std::string::npos )
	{
		buttonStartSingleplayer(nullptr);
		std::string number = mapname.substr(mapname.find("Tutorial ") + strlen("Tutorial "), 2);
		std::string filename = "tutorial";
		filename.append(std::to_string(stoi(number)));
		filename.append(".lmp");
		gameModeManager.Tutorial.setTutorialMap(filename);
		gameModeManager.Tutorial.dungeonLevel = currentlevel;
		return;
	}
	buttonReturnToTutorialHub(nullptr);
}

void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	node_t* node;
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	subwindow = 1;
	subx1 = xres / 2 - 288;
	subx2 = xres / 2 + 288;
	suby1 = yres / 2 - 160;
	suby2 = yres / 2 + 160;
	button_t* button;

	shootmode = false;
	strcpy(subtext, language[1133]);
	strcat(subtext, language[1134]);
	strcat(subtext, language[1137]);


	// identify all inventory items
	for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		item->identified = true;
	}

	// Restart
	button = newButton();
	strcpy(button->label, language[3957]);
	button->x = subx2 - strlen(language[3957]) * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen(language[3957]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRestartTrial;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// Return to Hub
	button = newButton();
	strcpy(button->label, language[3958]);
	button->x = subx1 + 8;
	button->y = suby2 - 28;
	button->sizex = strlen(language[3958]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonReturnToTutorialHub;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// death hints
	if ( currentlevel / LENGTH_OF_LEVEL_REGION < 1 )
	{
		strcat(subtext, language[1145 + rand() % 15]);
	}

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
}