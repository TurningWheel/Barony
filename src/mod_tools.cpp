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

void GameModeManager_t::Tutorial_t::readFromFile()
{
	levels.clear();
	if ( PHYSFS_getRealDir("/data/tutorial_scores.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/tutorial_scores.json");
		inputPath.append("/data/tutorial_scores.json");

		FILE* fp = fopen(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		rapidjson::FileReadStream is(fp, buf, sizeof(buf));
		fclose(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("levels") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}
		int version = d["version"].GetInt();

		for ( rapidjson::Value::ConstMemberIterator level_itr = d["levels"].MemberBegin(); level_itr != d["levels"].MemberEnd(); ++level_itr )
		{
			Tutorial_t::Level_t level;
			level.filename = level_itr->name.GetString();
			level.title = level_itr->value["title"].GetString();
			level.description = level_itr->value["desc"].GetString();
			level.completionTime = level_itr->value["completion_time"].GetUint();
			levels.push_back(level);
		}

		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
}

void GameModeManager_t::Tutorial_t::writeToDocument()
{
	if ( levels.empty() )
	{
		printlog("[JSON]: Could not write tutorial scores to file due to empty levels vector.");
		return;
	}

	if ( !PHYSFS_getRealDir("/data/tutorial_scores.json") )
	{
		printlog("[JSON]: Error: Could not locate json file /data/tutorial_scores.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/data/tutorial_scores.json");
	inputPath.append("/data/tutorial_scores.json");

	FILE* fp = fopen(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}
	char buf[65536];
	rapidjson::FileReadStream is(fp, buf, sizeof(buf));
	fclose(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	//rapidjson::Document d;
	//d.SetObject();
	//CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
	//rapidjson::Value levelsObj(rapidjson::kObjectType);
	//CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
	//for ( int i = 0; i < kNumTutorialLevels && i < levels.size(); ++i )
	//{
	//	std::string filename = std::string("tutorial" + std::to_string(i + 1)); // non-zero starting index for filenames
	//	rapidjson::Value level(rapidjson::kObjectType);
	//	level.AddMember("title", rapidjson::Value(levels.at(i).title.c_str(), d.GetAllocator()), d.GetAllocator());
	//	level.AddMember("desc", rapidjson::Value(levels.at(i).description.c_str(), d.GetAllocator()), d.GetAllocator());
	//	level.AddMember("completion_time", rapidjson::Value(levels.at(i).completionTime), d.GetAllocator());

	//	CustomHelpers::addMemberToSubkey(d, "levels", filename, level);
	//}
	for ( int i = 0; i < kNumTutorialLevels && i < levels.size(); ++i )
	{
		std::string filename = std::string("tutorial" + std::to_string(i + 1)); // non-zero starting index for filenames
		d["levels"][filename.c_str()]["completion_time"].SetUint(levels.at(i).completionTime);
	}

	writeToFile(d);
}

void GameModeManager_t::Tutorial_t::Menu_t::open()
{
	bWindowOpen = true;
	windowScroll = 0;

	// create window
	subwindow = 1;
	subx1 = xres / 2 - 420;
	subx2 = xres / 2 + 420;
	suby1 = yres / 2 - 300;
	suby2 = yres / 2 + 300;
	strcpy(subtext, "Hall of Trials");

	// close button
	button_t* button = newButton();
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

	// upload window button
	button = newButton();
	strcpy(button->label, "upload workshop content");
	button->x = subx2 - 40 - strlen(button->label) * TTF12_WIDTH;
	button->y = suby1;
	button->sizex = strlen(button->label) * TTF12_WIDTH + 16;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// start tutorial hub
	button = newButton();
	strcpy(button->label, "launch tutorial hub");
	button->sizex = 25 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->x = subx2 - (button->sizex + 16);
	button->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}