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
	strcpy(stats[0]->name, "Player");
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