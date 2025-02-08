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
#include "draw.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "ui/Field.hpp"
#include "ui/Image.hpp"
#ifndef EDITOR
#include "ui/MainMenu.hpp"
#include "shops.hpp"
#include "interface/ui.hpp"
#include "ui/GameUI.hpp"
#include "playfab.hpp"
#endif
#include "init.hpp"
#include "ui/LoadingScreen.hpp"
#include <thread>
#include <future>
#include <fstream>

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
BaronyRNG MonsterStatCustomManager::monster_stat_rng;
GameplayCustomManager gameplayCustomManager;
GameModeManager_t gameModeManager;
ItemTooltips_t ItemTooltips;
GlyphRenderer_t GlyphHelper;
ScriptTextParser_t ScriptTextParser;
#ifdef USE_THEORA_VIDEO
VideoManager_t VideoManager[MAXPLAYERS];
#endif
#ifndef NINTENDO
IRCHandler_t IRCHandler;
#endif // !NINTENDO
StatueManager_t StatueManager;
DebugTimers_t DebugTimers;

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

void GameModeManager_t::Tutorial_t::startTutorial(std::string mapToSet)
{
	if ( mapToSet.compare("") == 0 )
	{
		launchHub();
	}
	else
	{
		if ( mapToSet.find(".lmp") == std::string::npos )
		{
			mapToSet.append(".lmp");
		}
		setTutorialMap(mapToSet);
	}
	gameModeManager.setMode(gameModeManager.GameModes::GAME_MODE_TUTORIAL);
	stats[0]->clearStats();
	strcpy(stats[0]->name, "Player");
	stats[0]->sex = static_cast<sex_t>(local_rng.rand() % 2);
	stats[0]->playerRace = RACE_HUMAN;
	stats[0]->stat_appearance = local_rng.rand() % NUMAPPEARANCES;
	client_classes[0] = CLASS_WARRIOR;
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

		int tutorialNum = stoi(number);
		if ( tutorialNum > 0 && tutorialNum <= gameModeManager.Tutorial.kNumTutorialLevels )
		{
			gameModeManager.Tutorial.onMapRestart(tutorialNum);
		}
		return;
	}
	buttonReturnToTutorialHub(nullptr);
}

#ifdef EDITOR
void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	return;
}
void GameModeManager_t::Tutorial_t::createFirstTutorialCompletedPrompt()
{
	return;
}
#else
void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	MainMenu::openGameoverWindow(0, true);
}

void GameModeManager_t::Tutorial_t::createFirstTutorialCompletedPrompt()
{
	MainMenu::tutorialFirstTimeCompleted();
}
#endif

//TODO: NX PORT: Update for the Switch?
void GameModeManager_t::Tutorial_t::readFromFile()
{
	levels.clear();
	if ( PHYSFS_getRealDir("/data/tutorial_strings.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/tutorial_strings.json");
		inputPath.append("/data/tutorial_strings.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

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
			levels.push_back(level);
		}
		Menu.windowTitle = d["window_title"].GetString();
		Menu.defaultHoverText = d["default_hover_text"].GetString();
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
	else
	{
		return;
	}

	if ( PHYSFS_getRealDir(tutorialScoresFilename.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(tutorialScoresFilename.c_str());
		inputPath.append(tutorialScoresFilename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("levels") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());

			// recreate this file, corrupted?
			printlog("[JSON]: File %s corrupt, recreating...", inputPath.c_str());
			d.Clear();
			d.SetObject();
			CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
			CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(FirstTimePrompt.showFirstTimePrompt));
			CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(firstTutorialCompleted));

			rapidjson::Value levelsObj(rapidjson::kObjectType);
			CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
			for ( auto it = levels.begin(); it != levels.end(); ++it )
			{
				rapidjson::Value level(rapidjson::kObjectType);
				level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
				CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
			}
			writeToFile(d);
			return;
		}
		int version = d["version"].GetInt();

		this->FirstTimePrompt.showFirstTimePrompt = d["first_time_prompt"].GetBool();
		if ( d.HasMember("first_tutorial_complete") )
		{
			this->firstTutorialCompleted = d["first_tutorial_complete"].GetBool();
		}

		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			if ( d["levels"].HasMember(it->filename.c_str()) && d["levels"][it->filename.c_str()].HasMember("completion_time") )
			{
				it->completionTime = d["levels"][it->filename.c_str()]["completion_time"].GetUint();
			}
		}
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
	else
	{
		printlog("[JSON]: File %s does not exist, creating...", tutorialScoresFilename.c_str());

		rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(true));
		CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(false));

		this->FirstTimePrompt.showFirstTimePrompt = true;
		this->firstTutorialCompleted = false;

		rapidjson::Value levelsObj(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			rapidjson::Value level(rapidjson::kObjectType);
			level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
			CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
		}
		writeToFile(d);
	}
}

void GameModeManager_t::Tutorial_t::writeToDocument()
{
	if ( levels.empty() )
	{
		printlog("[JSON]: Could not write tutorial scores to file due to empty levels vector.");
		return;
	}

	if ( !PHYSFS_getRealDir(tutorialScoresFilename.c_str()) )
	{
		printlog("[JSON]: Error file %s does not exist", tutorialScoresFilename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(tutorialScoresFilename.c_str());
	inputPath.append(tutorialScoresFilename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}
	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	if ( !d.HasMember("version") || !d.HasMember("levels") )
	{
		printlog("[JSON]: File %s corrupt, recreating...", inputPath.c_str());
		d.Clear();
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(FirstTimePrompt.showFirstTimePrompt));
		CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(firstTutorialCompleted));

		rapidjson::Value levelsObj(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(d, "levels", levelsObj);
		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			rapidjson::Value level(rapidjson::kObjectType);
			level.AddMember("completion_time", rapidjson::Value(it->completionTime), d.GetAllocator());
			CustomHelpers::addMemberToSubkey(d, "levels", it->filename, level);
		}
	}
	else
	{
		d["first_time_prompt"].SetBool(this->FirstTimePrompt.showFirstTimePrompt);
		if ( !d.HasMember("first_tutorial_complete") )
		{
			CustomHelpers::addMemberToRoot(d, "first_tutorial_complete", rapidjson::Value(false));
		}
		d["first_tutorial_complete"].SetBool(this->firstTutorialCompleted);

		for ( auto it = levels.begin(); it != levels.end(); ++it )
		{
			d["levels"][it->filename.c_str()]["completion_time"].SetUint(it->completionTime);
		}
	}

	writeToFile(d);
}

void GameModeManager_t::Tutorial_t::Menu_t::open()
{
	// deprecated
	assert(0);
	return;
}

void GameModeManager_t::Tutorial_t::Menu_t::onClickEntry()
{
	if ( this->selectedMenuItem == -1 )
	{
		return;
	}
	buttonStartSingleplayer(nullptr);
	gameModeManager.setMode(GameModeManager_t::GAME_MODE_TUTORIAL_INIT);
	if ( gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt )
	{
		gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
		gameModeManager.Tutorial.writeToDocument();
	}
	gameModeManager.Tutorial.startTutorial(gameModeManager.Tutorial.levels.at(this->selectedMenuItem).filename);
	steamStatisticUpdate(STEAM_STAT_TUTORIAL_ENTERED, ESteamStatTypes::STEAM_STAT_INT, 1);
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::createPrompt()
{
	return;
	//bWindowOpen = true;
	//showFirstTimePrompt = false;
	//if ( !title_bmp )
	//{
	//	return;
	//}

	//// create window
	//subwindow = 1;
	//subx1 = xres / 2 - ((0.75 * title_bmp->w / 2) + 52);
	//subx2 = xres / 2 + ((0.75 * title_bmp->w / 2) + 52);
	//suby1 = yres / 2 - ((0.75 * title_bmp->h / 2) + 88);
	//suby2 = yres / 2 + ((0.75 * title_bmp->h / 2) + 88);
	//strcpy(subtext, "");

	//Uint32 centerWindowX = subx1 + (subx2 - subx1) / 2;

	//button_t* button = newButton();
	//strcpy(button->label, Language::get(3965));
	//button->sizex = strlen(Language::get(3965)) * 10 + 8;
	//button->sizey = 20;
	//button->x = centerWindowX - button->sizex / 2;
	//button->y = suby2 - 28 - 24;
	//button->action = &buttonPromptEnterTutorialHub;
	//button->visible = 1;
	//button->focused = 1;

	//button = newButton();
	//strcpy(button->label, Language::get(3966));
	//button->sizex = strlen(Language::get(3966)) * 12 + 8;
	//button->sizey = 20;
	//button->x = centerWindowX - button->sizex / 2;
	//button->y = suby2 - 28;
	//button->action = &buttonSkipPrompt;
	//button->visible = 1;
	//button->focused = 1;
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::drawDialogue()
{
	return;
	/*if ( !bWindowOpen )
	{
		return;
	}
	Uint32 centerWindowX = subx1 + (subx2 - subx1) / 2;

	SDL_Rect pos;
	pos.x = centerWindowX - 0.75 * title_bmp->w / 2;
	pos.y = suby1 + 4;
	pos.w = 0.75 * title_bmp->w;
	pos.h = 0.75 * title_bmp->h;
	SDL_Rect scaled;
	scaled.x = 0;
	scaled.y = 0;
	scaled.w = title_bmp->w * 0.75;
	scaled.h = title_bmp->h * 0.75;
	drawImageScaled(title_bmp, nullptr, &pos);
	
	ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(Language::get(3936)) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 13, makeColorRGB(255, 255, 0), Language::get(3936));
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(Language::get(3967)) * TTF12_WIDTH) / 2, suby2 + 8 - TTF12_HEIGHT * 11, Language::get(3967));
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(Language::get(3967)) * TTF12_WIDTH) / 2 - TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 11, Language::get(3968));*/
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::buttonSkipPrompt(button_t* my)
{
	gameModeManager.Tutorial.FirstTimePrompt.doButtonSkipPrompt = true;
	gameModeManager.Tutorial.writeToDocument();
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::buttonPromptEnterTutorialHub(button_t* my)
{
	gameModeManager.Tutorial.Menu.selectedMenuItem = 0; // set the tutorial hub to enter.
	gameModeManager.Tutorial.Menu.onClickEntry();
	gameModeManager.Tutorial.writeToDocument();
}

void GameModeManager_t::CurrentSession_t::SeededRun_t::setup(std::string _seedString)
{
	if ( _seedString == "" )
	{
		seed = 0;
		seedString = "";
		return;
	}
	int num = atoi(_seedString.c_str());
	if ( num == 0 )
	{
		seed = djb2Hash(const_cast<char*>(_seedString.c_str()));
	}
	else
	{
		seed = num;
	}
	seedString = _seedString;
}

void GameModeManager_t::CurrentSession_t::SeededRun_t::reset()
{
	seed = 0;
	seedString = "";
}

bool GameModeManager_t::allowsSaves()
{
	if ( currentMode == GAME_MODE_DEFAULT )
	{
		return true;
	}
	else if ( currentMode == GAME_MODE_CUSTOM_RUN )
	{
		return true;
	}
	return false;
}

void GameModeManager_t::setMode(const GameModes mode)
{
	currentMode = mode;
}

bool GameModeManager_t::allowsStatisticsOrAchievements(const char* achName, int statIndex)
{
	if ( currentMode == GAME_MODE_CUSTOM_RUN && currentSession.challengeRun.isActive()
		&& currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		if ( achName )
		{
			if ( !strcmp(achName, "BARONY_ACH_BLOOM_PLANTED") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_DUNGEONSEED") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_GROWTH_MINDSET") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_REAP_SOW") )
			{
				return true;
			}
			if ( !strcmp(achName, "BARONY_ACH_SPROUTS") )
			{
				return true;
			}
		}
		else if ( statIndex >= 0 )
		{
			switch ( statIndex )
			{
				case SteamStatIndexes::STEAM_STAT_DUNGEONSEED:
					return true;
				default:
					break;
			}
		}
		return false;
	}
	if ( currentMode == GAME_MODE_DEFAULT || currentMode == GAME_MODE_TUTORIAL
		|| currentMode == GAME_MODE_CUSTOM_RUN 
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsHiscores()
{
	if ( currentMode == GAME_MODE_CUSTOM_RUN && currentSession.challengeRun.isActive()
		&& currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		return false;
	}
	if ( currentMode == GAME_MODE_DEFAULT || currentMode == GAME_MODE_CUSTOM_RUN
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::isFastDeathGrave()
{
	if ( currentMode == GAME_MODE_TUTORIAL || currentMode == GAME_MODE_TUTORIAL_INIT )
	{
		return true;
	}
	if ( currentMode == GAME_MODE_CUSTOM_RUN )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsGlobalHiscores()
{
	if ( currentMode == GAME_MODE_DEFAULT )
	{
		if ( currentSession.seededRun.seed == 0 )
		{
			return true;
		}
	}
	else if ( (currentMode == GAME_MODE_CUSTOM_RUN
		|| currentMode == GAME_MODE_CUSTOM_RUN_ONESHOT) )
	{
		return true;
	}
	return false;
}

bool GameModeManager_t::allowsBoulderBreak()
{
	if ( currentMode != GAME_MODE_TUTORIAL )
	{
		return true;
	}
	return false;
}

std::vector<std::string> GameModeManager_t::CurrentSession_t::SeededRun_t::prefixes;
std::vector<std::string> GameModeManager_t::CurrentSession_t::SeededRun_t::suffixes;

void GameModeManager_t::CurrentSession_t::SeededRun_t::readSeedNamesFromFile()
{
	const std::string filename = "data/seed_names.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[10000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("prefixes") || !d.HasMember("suffixes") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	prefixes.clear();
	suffixes.clear();
	for ( auto it = d["prefixes"].Begin(); it != d["prefixes"].End(); ++it )
	{
		prefixes.push_back(it->GetString());
	}
	for ( auto it = d["suffixes"].Begin(); it != d["suffixes"].End(); ++it )
	{
		suffixes.push_back(it->GetString());
	}
}

#ifndef NINTENDO
bool IRCHandler_t::readFromFile()
{
	if ( PHYSFS_getRealDir("/data/twitchchat.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/twitchchat.json");
		inputPath.append("/data/twitchchat.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}
		auth.oauth = "";
		auth.chatroom = "";
		auth.username = "";
		if ( d.HasMember("oauth_key") )
		{
			auth.oauth = d["oauth_key"].GetString();
		}
		if ( d.HasMember("username") )
		{
			auth.username = d["username"].GetString();
		}
		if ( d.HasMember("channel") )
		{
			auth.chatroom = d["channel"].GetString();
		}

		if ( !auth.oauth.compare("") || !auth.chatroom.compare("") || !auth.username.compare("") )
		{
			printlog("[JSON]: Error in one or more data values. Check syntax and try again.");
			return false;
		}
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		return true;
	}
	return false;
}

void IRCHandler_t::disconnect()
{
	if ( net_ircsocketset )
	{
		SDLNet_TCP_DelSocket(net_ircsocketset, net_ircsocket);
	}
	net_ircsocketset = nullptr;
	if ( net_ircsocket )
	{
		SDLNet_TCP_Close(net_ircsocket);
	}
	net_ircsocket = nullptr;
	bSocketConnected = false;
}

bool IRCHandler_t::connect()
{
	if ( SDLNet_ResolveHost(&ip, "irc.chat.twitch.tv", 6667) == -1 )
	{
		return false;
	}
	bSocketConnected = false;
	if ( !readFromFile() )
	{
		return false;
	}
	if ( !(net_ircsocket = SDLNet_TCP_Open(&ip)) )
	{
		return false;
	}
	net_ircsocketset = SDLNet_AllocSocketSet(1);
	if ( !net_ircsocketset )
	{
		return false;
	}
	SDLNet_TCP_AddSocket(net_ircsocketset, net_ircsocket);
	bSocketConnected = true;

	std::string data = "PASS oauth:" + auth.oauth + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	data = "NICK " + auth.username + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	data = "JOIN #" + auth.chatroom + "\r\n";
	packetSend(data);
	SDL_Delay(1);
	return true;
}

int IRCHandler_t::packetSend(std::string data)
{
	if ( !bSocketConnected )
	{
		return -1;
	}
	int sentBytes = SDLNet_TCP_Send(net_ircsocket, data.data(), data.length());
	return sentBytes;
}

int IRCHandler_t::packetReceive()
{
	if ( !bSocketConnected )
	{
		return 0;
	}

	if ( SDLNet_CheckSockets(net_ircsocketset, 0) )
	{
		if ( SDLNet_SocketReady(net_ircsocketset) )
		{
			std::fill(recvBuffer.begin(), recvBuffer.end(), '\0');
			int receiveLen = SDLNet_TCP_Recv(net_ircsocket, &recvBuffer[0], MAX_BUFFER_LEN);
			if ( receiveLen <= 0 )
			{
				printlog("[IRCHandler]: Error in packetReceive: %s", SDLNet_GetError());
				return 0;
			}
			return receiveLen;
		}
	}
	return 0;
}

void IRCHandler_t::run()
{
	if ( !bSocketConnected )
	{
		return;
	}

	while ( int receiveLen = packetReceive() )
	{
		// check incoming messages.
		std::string msg(recvBuffer.cbegin(), recvBuffer.cend());
		handleMessage(msg);
	}
}

void IRCHandler_t::handleMessage(std::string& msg)
{
#ifndef EDITOR
	if ( intro )
	{
		return;
	}
	if ( msg.length() <= 1 )
	{
		return;
	}
	msg = msg.substr(0, msg.find("\r\n"));
	printlog("Recv: %s", msg.c_str());

	if ( msg.find("PING :tmi.twitch.tv") != std::string::npos )
	{
		packetSend("PING :tmi.twitch.tv\r\n");
		return;
	}

	std::string msgPrefix = "PRIVMSG #" + auth.chatroom + " :";
	auto findMsg = msg.find(msgPrefix);
	if ( findMsg != std::string::npos )
	{
		if ( msg.find("!") != std::string::npos )
		{
			std::string user = msg.substr(1, msg.find("!") - 1);
			std::string formattedMsg = msg.substr(msgPrefix.length() + findMsg);
			messagePlayer(clientnum, MESSAGE_MISC, "IRC: [@%s]: %s", user.c_str(), formattedMsg.c_str());
		}
		return;
	}
#endif
}
#endif // !NINTENDO

Uint32 ItemTooltips_t::itemsJsonHashRead = 0;
const Uint32 ItemTooltips_t::kItemsJsonHash = 1748555711;
void ItemTooltips_t::readItemsFromFile()
{
	printlog("loading items...\n");
	if ( !PHYSFS_getRealDir("items/items.json") )
	{
		printlog("[JSON]: Error: Could not find file: items/items.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/items/items.json");
	inputPath.append("/items/items.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	const int bufSize = 360000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);
	if ( !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	int itemsRead = 0;

	tmpItems.clear();

	for ( rapidjson::Value::ConstMemberIterator item_itr = d["items"].MemberBegin(); 
		item_itr != d["items"].MemberEnd(); ++item_itr )
	{
		tmpItem_t t;
		t.internalName = item_itr->name.GetString();
		t.itemId = item_itr->value["item_id"].GetInt();
		t.fpIndex = item_itr->value["first_person_model_index"].GetInt();
		t.tpIndex = item_itr->value["third_person_model_index"].GetInt();
		if ( item_itr->value.HasMember("third_person_model_index_short") )
		{
			t.tpShortIndex = item_itr->value["third_person_model_index_short"].GetInt();
		}
		t.gold = item_itr->value["gold_value"].GetInt();
		t.weight = item_itr->value["weight_value"].GetInt();
		t.itemLevel = item_itr->value["item_level"].GetInt();
		t.category = item_itr->value["item_category"].GetString();
		t.equipSlot = item_itr->value["equip_slot"].GetString();

		for ( rapidjson::Value::ConstValueIterator pathArray_itr = item_itr->value["item_images"].Begin();
			pathArray_itr != item_itr->value["item_images"].End(); 
			++pathArray_itr )
		{
			t.imagePaths.push_back(pathArray_itr->GetString());
		}

		if ( item_itr->value.HasMember("stats") )
		{
			for ( rapidjson::Value::ConstMemberIterator stat_itr = item_itr->value["stats"].MemberBegin(); 
				stat_itr != item_itr->value["stats"].MemberEnd(); ++stat_itr )
			{
				t.attributes[stat_itr->name.GetString()] = stat_itr->value.GetInt();
			}
		}

		if ( item_itr->value.HasMember("tooltip") )
		{
			if ( item_itr->value["tooltip"].HasMember("type") )
			{
				t.tooltip = item_itr->value["tooltip"]["type"].GetString();
			}
		}

		if ( item_itr->value.HasMember("icon_label_path") )
		{
			t.iconLabelPath = item_itr->value["icon_label_path"].GetString();
		}

		tmpItems.push_back(t);
		++itemsRead;
	}

	printlog("[JSON]: Successfully read %d items from '%s'", itemsRead, inputPath.c_str());

	//itemValueTable.clear();
	//itemValueTableByCategory.clear();
	Uint32 shift = 0;
	Uint32 hash = 0;
	for ( int i = 0; i < NUMITEMS && i < itemsRead; ++i )
	{
		assert(i == tmpItems[i].itemId);
		items[i].level = tmpItems[i].itemLevel;
		items[i].value = tmpItems[i].gold;
		items[i].weight = tmpItems[i].weight;
		items[i].fpindex = tmpItems[i].fpIndex;
		items[i].index = tmpItems[i].tpIndex;
		items[i].indexShort = tmpItems[i].tpShortIndex;
		items[i].tooltip = tmpItems[i].tooltip;
		items[i].attributes.clear();
		items[i].attributes = tmpItems[i].attributes;
		if ( i == SPELL_ITEM )
		{
			items[i].variations = 1;
		}
		else
		{
			items[i].variations = tmpItems[i].imagePaths.size();
		}
		list_FreeAll(&items[i].images);
		items[i].images.first = NULL;
		items[i].images.last = NULL;
		for ( int j = 0; j < tmpItems[i].imagePaths.size(); ++j )
		{
			//auto s = static_cast<string_t*>(list_Node(&items[i].images, j)->element);
			//assert(!strcmp(s->data, tmpItems[i].imagePaths[j].c_str()));

			string_t* string = (string_t*)malloc(sizeof(string_t));
			const size_t len = 64;
			string->data = (char*)malloc(sizeof(char) * len);
			memset(string->data, 0, sizeof(char) * len);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[i].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			stringCopy(string->data, tmpItems[i].imagePaths[j].c_str(), len - 1, tmpItems[i].imagePaths[j].size());
		}
		if ( tmpItems[i].category.compare("WEAPON") == 0 )
		{
			items[i].category = WEAPON;
		}
		else if ( tmpItems[i].category.compare("ARMOR") == 0 )
		{
			items[i].category = ARMOR;
		}
		else if ( tmpItems[i].category.compare("AMULET") == 0 )
		{
			items[i].category = AMULET;
		}
		else if ( tmpItems[i].category.compare("POTION") == 0 )
		{
			items[i].category = POTION;
		}
		else if ( tmpItems[i].category.compare("SCROLL") == 0 )
		{
			items[i].category = SCROLL;
		}
		else if ( tmpItems[i].category.compare("MAGICSTAFF") == 0 )
		{
			items[i].category = MAGICSTAFF;
		}
		else if ( tmpItems[i].category.compare("RING") == 0 )
		{
			items[i].category = RING;
		}
		else if ( tmpItems[i].category.compare("SPELLBOOK") == 0 )
		{
			items[i].category = SPELLBOOK;
		}
		else if ( tmpItems[i].category.compare("TOOL") == 0 )
		{
			items[i].category = TOOL;
		}
		else if ( tmpItems[i].category.compare("FOOD") == 0 )
		{
			items[i].category = FOOD;
		}
		else if ( tmpItems[i].category.compare("BOOK") == 0 )
		{
			items[i].category = BOOK;
		}
		else if ( tmpItems[i].category.compare("THROWN") == 0 )
		{
			items[i].category = THROWN;
		}
		else if ( tmpItems[i].category.compare("SPELL_CAT") == 0 )
		{
			items[i].category = SPELL_CAT;
		}
		else
		{
			items[i].category = GEM;
		}

		items[i].item_slot = ItemEquippableSlot::NO_EQUIP;
		if ( tmpItems[i].equipSlot.compare("mainhand") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON;
		}
		else if ( tmpItems[i].equipSlot.compare("offhand") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD;
		}
		else if ( tmpItems[i].equipSlot.compare("gloves") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES;
		}
		else if ( tmpItems[i].equipSlot.compare("cloak") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK;
		}
		else if ( tmpItems[i].equipSlot.compare("boots") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS;
		}
		else if ( tmpItems[i].equipSlot.compare("torso") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE;
		}
		else if ( tmpItems[i].equipSlot.compare("amulet") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET;
		}
		else if ( tmpItems[i].equipSlot.compare("ring") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING;
		}
		else if ( tmpItems[i].equipSlot.compare("mask") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK;
		}
		else if ( tmpItems[i].equipSlot.compare("helm") == 0 )
		{
			items[i].item_slot = ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM;
		}

		hash += (Uint32)((Uint32)items[i].weight << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)items[i].value << (shift % 32)); ++shift;
		hash += (Uint32)((Uint32)items[i].level << (shift % 32)); ++shift;
		/*{
			auto pair = std::make_pair(items[i].value, i);
			auto lower = std::lower_bound(itemValueTable.begin(), itemValueTable.end(), pair,
				[](const auto& lhs, const auto& rhs) {
					return lhs < rhs;
			});
			itemValueTable.insert(lower, pair);
		}
		{
			auto pair = std::make_pair(items[i].value, i);
			auto lower = std::lower_bound(itemValueTableByCategory[items[i].category].begin(), 
				itemValueTableByCategory[items[i].category].end(), pair,
				[](const auto& lhs, const auto& rhs) {
					return lhs < rhs;
				});
			itemValueTableByCategory[items[i].category].insert(lower, pair);
		}*/
	}

	itemsJsonHashRead = hash;
	if ( itemsJsonHashRead != kItemsJsonHash )
	{
		printlog("[JSON]: Notice: items.json unknown hash, achievements are disabled: %d", itemsJsonHashRead);
	}
	else
	{
		printlog("[JSON]: items.json hash verified successfully.");
	}

	spellItems.clear();

	int spellsRead = 0;
	for ( rapidjson::Value::ConstMemberIterator spell_itr = d["spells"].MemberBegin();
		spell_itr != d["spells"].MemberEnd(); ++spell_itr )
	{
		spellItem_t t;
		t.internalName = spell_itr->name.GetString();
		t.name = spell_itr->value["spell_name"].GetString();
		t.id = spell_itr->value["spell_id"].GetInt();
		t.spellTypeStr = spell_itr->value["spell_type"].GetString();
		t.spellType = SPELL_TYPE_DEFAULT;
		if ( t.spellTypeStr == "PROJECTILE" )
		{
			t.spellType = SPELL_TYPE_PROJECTILE;
		}
		else if ( t.spellTypeStr == "AREA" )
		{
			t.spellType = SPELL_TYPE_AREA;
		}
		else if ( t.spellTypeStr == "SELF" )
		{
			t.spellType = SPELL_TYPE_SELF;
		}
		else if ( t.spellTypeStr == "SELF_SUSTAIN" )
		{
			t.spellType = SPELL_TYPE_SELF_SUSTAIN;
		}
		else if ( t.spellTypeStr == "PROJECTILE_SHORT_X3" )
		{
			t.spellType = SPELL_TYPE_PROJECTILE_SHORT_X3;
		}

		for ( rapidjson::Value::ConstValueIterator arr_itr = spell_itr->value["effect_tags"].Begin();
			arr_itr != spell_itr->value["effect_tags"].End(); ++arr_itr )
		{
			t.spellTagsStr.push_back(arr_itr->GetString());
			if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "DAMAGE" )
			{
				t.spellTags.insert(SPELL_TAG_DAMAGE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "STATUS" )
			{
				t.spellTags.insert(SPELL_TAG_STATUS_EFFECT);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "UTILITY" )
			{
				t.spellTags.insert(SPELL_TAG_UTILITY);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "HEALING" )
			{
				t.spellTags.insert(SPELL_TAG_HEALING);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "CURE" )
			{
				t.spellTags.insert(SPELL_TAG_CURE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "BASIC_HIT_MESSAGE" )
			{
				t.spellTags.insert(SPELL_TAG_BASIC_HIT_MESSAGE);
			}
			else if ( t.spellTagsStr[t.spellTagsStr.size() - 1] == "TRACK_SPELL_HITS" )
			{
				t.spellTags.insert(SPELL_TAG_TRACK_HITS);
			}
		}

		t.spellbookInternalName = spell_itr->value["spellbook_internal_name"].GetString();
		t.magicstaffInternalName = spell_itr->value["magicstaff_internal_name"].GetString();

		for ( int i = 0; i < NUMITEMS; ++i )
		{
			if ( items[i].category != SPELLBOOK && items[i].category != MAGICSTAFF )
			{
				continue;
			}
			if ( t.spellbookInternalName == tmpItems[i].internalName )
			{
				t.spellbookId = i;
			}
			if ( t.magicstaffInternalName == tmpItems[i].internalName )
			{
				t.magicstaffId = i;
			}
		}

		spellNameStringToSpellID[t.internalName] = t.id;
		assert(spellItems.find(t.id) == spellItems.end()); // check we haven't got duplicate key
		spellItems.insert(std::make_pair(t.id, t));
		++spellsRead;
	}
	printlog("[JSON]: Successfully read %d spells from '%s'", spellsRead, inputPath.c_str());

	// validation against old items.txt
	/*for ( int i = 0; i < NUMITEMS; ++i )
	{
		assert(i == tmpItems[i].itemId);
		assert(items[i].level == tmpItems[i].itemLevel);
		assert(items[i].value == tmpItems[i].gold);
		assert(items[i].weight == tmpItems[i].weight);
		assert(items[i].fpindex == tmpItems[i].fpIndex);
		assert(items[i].index == tmpItems[i].tpIndex);
		if ( i != SPELL_ITEM )
		{
			assert(items[i].variations == tmpItems[i].imagePaths.size());
		}
		for ( int j = 0; j < items[i].variations; ++j )
		{
			auto s = static_cast<string_t*>(list_Node(&items[i].images, j)->element);
			assert(!strcmp(s->data, tmpItems[i].imagePaths[j].c_str()));
		}
		assert(items[i].index == tmpItems[i].tpIndex);
		assert(!strcmp(itemNameStrings[i + 2], tmpItems[i].itemName.c_str()));
	}*/
}

void ItemTooltips_t::readItemLocalizationsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/lang/item_names.json") )
	{
		printlog("[JSON]: Error: Could not find file: lang/item_names.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/lang/item_names.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemLocalizationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/lang/item_names.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 17); // 131kb
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( d.HasMember("items") )
	{
		if ( forceLoadBaseDirectory )
		{
			itemNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator items_itr = d["items"].MemberBegin();
			items_itr != d["items"].MemberEnd(); ++items_itr )
		{
			if ( items_itr->value.HasMember("name_identified") )
			{
				itemNameLocalizations[items_itr->name.GetString()].name_identified = items_itr->value["name_identified"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: item '%s' has no member 'name_identified'!", items_itr->name.GetString());
			}
			if ( items_itr->value.HasMember("name_unidentified") )
			{
				itemNameLocalizations[items_itr->name.GetString()].name_unidentified = items_itr->value["name_unidentified"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: item '%s' has no member 'name_unidentified'!", items_itr->name.GetString());
			}
		}
	}

	if ( d.HasMember("spell_names") )
	{
		if ( forceLoadBaseDirectory )
		{
			spellNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator spell_itr = d["spell_names"].MemberBegin();
			spell_itr != d["spell_names"].MemberEnd(); ++spell_itr )
		{
			if ( spell_itr->value.HasMember("name") )
			{
				spellNameLocalizations[spell_itr->name.GetString()] = spell_itr->value["name"].GetString();
			}
			else
			{
				printlog("[JSON]: Warning: spell '%s' has no member 'name'!", spell_itr->name.GetString());
			}
		}
	}

	printlog("[JSON]: Successfully read %d item names, %d spell names from '%s'", itemNameLocalizations.size(), spellNameLocalizations.size(), inputPath.c_str());
	assert(itemNameLocalizations.size() == (NUMITEMS));
	assert(spellNameLocalizations.size() == (NUM_SPELLS - 1)); // ignore SPELL_NONE

	// apply localizations
	for ( int i = 0; i < NUMITEMS; ++i )
	{
		items[i].setIdentifiedName("default_identified_item_name");
		items[i].setUnidentifiedName("default_unidentified_item_name");
	}
	for ( auto& item : tmpItems )
	{
		if ( item.itemId >= WOODEN_SHIELD && item.itemId < NUMITEMS )
		{
			items[item.itemId].setIdentifiedName(itemNameLocalizations[item.internalName].name_identified);
			items[item.itemId].setUnidentifiedName(itemNameLocalizations[item.internalName].name_unidentified);
		}
	}
	for ( auto& spell : spellItems )
	{
		spell.second.name = spellNameLocalizations[spell.second.internalName];
	}

	/*for ( auto i : itemValueTable )
	{
		printlog("itemValueTable %4d | %s", items[i.second].value, items[i.second].getIdentifiedName());
	}
	for ( int cat = 0; cat < NUMCATEGORIES; ++cat )
	{
		for ( auto i : itemValueTableByCategory[cat] )
		{
			printlog("itemValueTableByCategory %2d | %4d | %s", cat,
				items[i.second].value, items[i.second].getIdentifiedName());
		}
	}*/
}

void ItemTooltips_t::readBookLocalizationsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/lang/book_names.json") )
	{
		printlog("[JSON]: Error: Could not find file: lang/book_names.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/lang/book_names.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readBookLocalizationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/lang/book_names.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 13);
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( d.HasMember("book_names") )
	{
		if ( forceLoadBaseDirectory )
		{
			bookNameLocalizations.clear();
		}
		for ( rapidjson::Value::ConstMemberIterator books_itr = d["book_names"].MemberBegin();
			books_itr != d["book_names"].MemberEnd(); ++books_itr )
		{
			bookNameLocalizations[books_itr->name.GetString()] = books_itr->value.GetString();
		}
	}

	printlog("[JSON]: Successfully read %d book names from '%s'", bookNameLocalizations.size(), inputPath.c_str());
}

#ifndef EDITOR
void ItemTooltips_t::readTooltipsFromFile(bool forceLoadBaseDirectory)
{
	if ( !PHYSFS_getRealDir("/items/item_tooltips.json") )
	{
		printlog("[JSON]: Error: Could not find file: items/items.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/items/item_tooltips.json");
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readTooltipsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append("/items/item_tooltips.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	constexpr uint32_t buffer_size = (1 << 20); // 1mb
	if ( fp->size() >= buffer_size )
	{
		printlog("[JSON]: Error: file size is too large to fit in buffer! %s", inputPath.c_str());
		FileIO::close(fp);
		return;
	}

	static char buf[buffer_size];
	const int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);

	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: json file does not define a complete object. Is there a syntax error? %s", inputPath.c_str());
		return;
	}

	if ( !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	if ( forceLoadBaseDirectory )
	{
		adjectives.clear();
	}
	if ( d.HasMember("adjectives") )
	{
		for ( rapidjson::Value::ConstMemberIterator adj_itr = d["adjectives"].MemberBegin();
			adj_itr != d["adjectives"].MemberEnd(); ++adj_itr )
		{
			std::map<std::string, std::string> m;
			for ( rapidjson::Value::ConstMemberIterator inner_itr = adj_itr->value.MemberBegin();
				inner_itr != adj_itr->value.MemberEnd(); ++inner_itr )
			{
				m[inner_itr->name.GetString()] = inner_itr->value.GetString();
			}
			adjectives[adj_itr->name.GetString()] = m;
		}
	}

	if ( d.HasMember("default_text_colors") )
	{
		defaultHeadingTextColor = makeColor( 
			d["default_text_colors"]["heading"]["r"].GetInt(), 
			d["default_text_colors"]["heading"]["g"].GetInt(), 
			d["default_text_colors"]["heading"]["b"].GetInt(),
			d["default_text_colors"]["heading"]["a"].GetInt());
		defaultIconTextColor = makeColor(
			d["default_text_colors"]["icons"]["r"].GetInt(),
			d["default_text_colors"]["icons"]["g"].GetInt(),
			d["default_text_colors"]["icons"]["b"].GetInt(), 
			d["default_text_colors"]["icons"]["a"].GetInt());
		defaultDescriptionTextColor = makeColor(
			d["default_text_colors"]["description"]["r"].GetInt(),
			d["default_text_colors"]["description"]["g"].GetInt(), 
			d["default_text_colors"]["description"]["b"].GetInt(),
			d["default_text_colors"]["description"]["a"].GetInt());
		defaultDetailsTextColor = makeColor(
			d["default_text_colors"]["details"]["r"].GetInt(), 
			d["default_text_colors"]["details"]["g"].GetInt(), 
			d["default_text_colors"]["details"]["b"].GetInt(), 
			d["default_text_colors"]["details"]["a"].GetInt());
		defaultPositiveTextColor = makeColor(
			d["default_text_colors"]["positive_color"]["r"].GetInt(),
			d["default_text_colors"]["positive_color"]["g"].GetInt(),
			d["default_text_colors"]["positive_color"]["b"].GetInt(),
			d["default_text_colors"]["positive_color"]["a"].GetInt());
		defaultNegativeTextColor = makeColor(
			d["default_text_colors"]["negative_color"]["r"].GetInt(),
			d["default_text_colors"]["negative_color"]["g"].GetInt(),
			d["default_text_colors"]["negative_color"]["b"].GetInt(),
			d["default_text_colors"]["negative_color"]["a"].GetInt());
		defaultStatusEffectTextColor = makeColor(
			d["default_text_colors"]["status_effect"]["r"].GetInt(),
			d["default_text_colors"]["status_effect"]["g"].GetInt(),
			d["default_text_colors"]["status_effect"]["b"].GetInt(),
			d["default_text_colors"]["status_effect"]["a"].GetInt());
		defaultFaintTextColor = makeColor(
			d["default_text_colors"]["faint_text"]["r"].GetInt(),
			d["default_text_colors"]["faint_text"]["g"].GetInt(),
			d["default_text_colors"]["faint_text"]["b"].GetInt(),
			d["default_text_colors"]["faint_text"]["a"].GetInt());
	}

	if ( forceLoadBaseDirectory )
	{
		templates.clear();
	}
	if ( d.HasMember("templates") )
	{
		for ( rapidjson::Value::ConstMemberIterator template_itr = d["templates"].MemberBegin();
			template_itr != d["templates"].MemberEnd(); ++template_itr )
		{
			if ( !template_itr->value.IsArray() )
			{
				printlog("[JSON]: Error: template entry for template %s did not have [] format!", template_itr->name.GetString());
			}
			else
			{
				std::string template_name = template_itr->name.GetString();
				if ( templates.find(template_name) != templates.end() )
				{
					templates[template_name].clear();
				}
				for ( auto lines = template_itr->value.Begin();
					lines != template_itr->value.End(); ++lines )
				{
					templates[template_name].push_back(lines->GetString());
				}
			}
		}
	}

	if ( forceLoadBaseDirectory )
	{
		tooltips.clear();
	}
	std::unordered_set<std::string> tagsRead;

	if ( d.HasMember("tooltips") )
	{
		for ( rapidjson::Value::ConstMemberIterator tooltipType_itr = d["tooltips"].MemberBegin();
			tooltipType_itr != d["tooltips"].MemberEnd(); ++tooltipType_itr )
		{
			ItemTooltip_t tooltip;
			tooltip.setColorHeading(this->defaultHeadingTextColor);
			tooltip.setColorDescription(this->defaultDescriptionTextColor);
			tooltip.setColorDetails(this->defaultDetailsTextColor);
			tooltip.setColorPositive(this->defaultPositiveTextColor);
			tooltip.setColorNegative(this->defaultNegativeTextColor);
			tooltip.setColorStatus(this->defaultStatusEffectTextColor);
			tooltip.setColorFaintText(this->defaultFaintTextColor);

			if ( tooltipType_itr->value.HasMember("icons") )
			{
				if ( !tooltipType_itr->value["icons"].IsArray() )
				{
					printlog("[JSON]: Error: 'icons' entry for tooltip %s did not have [] format", tooltipType_itr->name.GetString());
				}
				else
				{
					for ( auto icons = tooltipType_itr->value["icons"].Begin();
						icons != tooltipType_itr->value["icons"].End(); ++icons )
					{
						// you need to FindMember() if getting objects from an array...
						auto textMember = icons->FindMember("text");
						auto iconPathMember = icons->FindMember("icon_path");
						if ( !textMember->value.IsString() || !iconPathMember->value.IsString() )
						{
							printlog("[JSON]: Error: Icon text or path was not string!");
							continue;
						}

						tooltip.icons.push_back(ItemTooltipIcons_t(iconPathMember->value.GetString(), textMember->value.GetString()));

						Uint32 color = this->defaultIconTextColor;
						if ( icons->HasMember("color") && icons->FindMember("color")->value.HasMember("r") )
						{
							// icons->FindMember("color")->value.isObject() always returning true?? so check for "r" member instead
							color = makeColor(
								icons->FindMember("color")->value["r"].GetInt(),
								icons->FindMember("color")->value["g"].GetInt(),
								icons->FindMember("color")->value["b"].GetInt(),
								icons->FindMember("color")->value["a"].GetInt());
						}
						tooltip.icons[tooltip.icons.size() - 1].setColor(color);
						if ( icons->HasMember("conditional_attribute") )
						{
							tooltip.icons[tooltip.icons.size() - 1].setConditionalAttribute(icons->FindMember("conditional_attribute")->value.GetString());
						}
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("description") )
			{
				if ( tooltipType_itr->value["description"].IsString() )
				{
					//printlog("[JSON]: Found template string '%s' for tooltip '%s'", tooltipType_itr->value["description"].GetString(), tooltipType_itr->name.GetString());
					if ( templates.find(tooltipType_itr->value["description"].GetString()) != templates.end() )
					{
						tooltip.descriptionText = templates[tooltipType_itr->value["description"].GetString()];
					}
					else
					{
						printlog("[JSON]: Error: Could not find template tag '%s'", tooltipType_itr->value["description"].GetString());
					}
				}
				else
				{
					for ( auto descriptions = tooltipType_itr->value["description"].Begin();
						descriptions != tooltipType_itr->value["description"].End(); ++descriptions )
					{
						tooltip.descriptionText.push_back(descriptions->GetString());
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("details") )
			{
				if ( !tooltipType_itr->value["details"].IsArray() )
				{
					printlog("[JSON]: Error: 'details' entry for tooltip '%s' did not have [] format!", tooltipType_itr->name.GetString());
				}
				else
				{
					for ( auto details_itr = tooltipType_itr->value["details"].Begin();
						details_itr != tooltipType_itr->value["details"].End(); ++details_itr )
					{
						for ( auto keyValue_itr = details_itr->MemberBegin();
							keyValue_itr != details_itr->MemberEnd(); ++keyValue_itr )
						{
							tagsRead.insert(keyValue_itr->name.GetString());
							std::vector<std::string> detailEntry;
							if ( keyValue_itr->value.IsString() )
							{
								//printlog("[JSON]: Found template string '%s' for tooltip '%s'", keyValue_itr->value.GetString(), tooltipType_itr->name.GetString());
								if ( templates.find(keyValue_itr->value.GetString()) != templates.end() )
								{
									detailEntry = templates[keyValue_itr->value.GetString()];
								}
								else
								{
									printlog("[JSON]: Error: Could not find template tag '%s'", keyValue_itr->value.GetString());
								}
							}
							else
							{
								for ( auto detailTag = keyValue_itr->value.Begin();
									detailTag != keyValue_itr->value.End(); ++detailTag )
								{
									detailEntry.push_back(detailTag->GetString());
								}
							}
							tooltip.detailsText[keyValue_itr->name.GetString()] = detailEntry;
							tooltip.detailsTextInsertOrder.push_back(keyValue_itr->name.GetString());
						}
					}
				}
			}

			if ( tooltipType_itr->value.HasMember("size") )
			{
				if ( tooltipType_itr->value["size"].HasMember("min_width") )
				{
					tooltip.minWidths["default"] = tooltipType_itr->value["size"]["min_width"].GetInt();
				}
				else
				{
					tooltip.minWidths["default"] = 0;
				}
				if ( tooltipType_itr->value["size"].HasMember("max_width") )
				{
					tooltip.maxWidths["default"] = tooltipType_itr->value["size"]["max_width"].GetInt();
				}
				else
				{
					tooltip.maxWidths["default"] = 0;
				}
				if ( tooltipType_itr->value["size"].HasMember("max_header_width") )
				{
					tooltip.headerMaxWidths["default"] = tooltipType_itr->value["size"]["max_header_width"].GetInt();
				}
				else
				{
					tooltip.headerMaxWidths["default"] = 0;
				}

				if ( tooltipType_itr->value["size"].HasMember("item_overrides") )
				{
					for ( auto itemOverride_itr = tooltipType_itr->value["size"]["item_overrides"].MemberBegin();
						itemOverride_itr != tooltipType_itr->value["size"]["item_overrides"].MemberEnd(); ++itemOverride_itr )
					{
						if ( itemOverride_itr->value.HasMember("min_width") )
						{
							tooltip.minWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["min_width"].GetInt();
						}
						if ( itemOverride_itr->value.HasMember("max_width") )
						{
							tooltip.maxWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["max_width"].GetInt();
						}
						if ( itemOverride_itr->value.HasMember("max_header_width") )
						{
							tooltip.headerMaxWidths[itemOverride_itr->name.GetString()] = itemOverride_itr->value["max_header_width"].GetInt();
						}
					}
				}
			}

			tooltips[tooltipType_itr->name.GetString()] = tooltip;
		}
	}

	printlog("[JSON]: Successfully read %d item tooltips from '%s'", tooltips.size(), inputPath.c_str());
	/*for ( auto tmp : tagsRead )
	{
		printlog("%s", tmp.c_str());
	}*/
}

std::string& ItemTooltips_t::getItemStatusAdjective(Uint32 itemType, Status status)
{
	if ( itemType >= ARTIFACT_ORB_BLUE && itemType <= ARTIFACT_ORB_GREEN )
	{
		if ( adjectives.find("jewelry_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["jewelry_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["jewelry_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["jewelry_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["jewelry_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["jewelry_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( itemType == TOOL_SENTRYBOT || itemType == TOOL_SPELLBOT
		|| itemType == TOOL_GYROBOT || itemType == TOOL_DUMMYBOT )
	{
		if ( adjectives.find("tinkering_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["tinkering_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["tinkering_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["tinkering_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["tinkering_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["tinkering_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == ARMOR 
		|| items[itemType].category == WEAPON
		|| items[itemType].category == MAGICSTAFF
		|| items[itemType].category == TOOL
		|| items[itemType].category == THROWN
		|| itemType == POTION_EMPTY )
	{
		if ( adjectives.find("equipment_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["equipment_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["equipment_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["equipment_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["equipment_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["equipment_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == AMULET
		|| items[itemType].category == RING
		|| items[itemType].category == GEM )
	{
		if ( adjectives.find("jewelry_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["jewelry_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["jewelry_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["jewelry_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["jewelry_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["jewelry_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == SCROLL
		|| items[itemType].category == SPELLBOOK
		|| items[itemType].category == BOOK )
	{
		if ( adjectives.find("book_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["book_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["book_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["book_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["book_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["book_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == FOOD )
	{
		if ( adjectives.find("food_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["food_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["food_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["food_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["food_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["food_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	else if ( items[itemType].category == POTION )
	{
		if ( adjectives.find("potion_status") == adjectives.end() )
		{
			return defaultString;
		}
		switch ( status )
		{
			case BROKEN:
				return adjectives["potion_status"]["broken"];
				break;
			case DECREPIT:
				return adjectives["potion_status"]["decrepit"];
				break;
			case WORN:
				return adjectives["potion_status"]["worn"];
				break;
			case SERVICABLE:
				return adjectives["potion_status"]["serviceable"];
				break;
			case EXCELLENT:
				return adjectives["potion_status"]["excellent"];
				break;
			default:
				return defaultString;
		}
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemBeatitudeAdjective(Sint16 beatitude)
{
	if ( adjectives.find("beatitude_status") == adjectives.end() )
	{
		return defaultString;
	}

	if ( beatitude > 0 )
	{
		return adjectives["beatitude_status"]["blessed"];
	}
	else if ( beatitude < 0 )
	{
		return adjectives["beatitude_status"]["cursed"];
	}
	else
	{
		return adjectives["beatitude_status"]["uncursed"];
	}
}

std::string& ItemTooltips_t::getProficiencyLevelName(Sint32 proficiencyLevel)
{
	if ( adjectives.find("proficiency_levels") == adjectives.end() )
	{
		return defaultString;
	}

	if ( proficiencyLevel >= SKILL_LEVEL_LEGENDARY )
	{
		return adjectives["proficiency_levels"]["legend"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_MASTER )
	{
		return adjectives["proficiency_levels"]["master"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_EXPERT )
	{
		return adjectives["proficiency_levels"]["expert"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_SKILLED )
	{
		return adjectives["proficiency_levels"]["skilled"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_BASIC )
	{
		return adjectives["proficiency_levels"]["basic"];
	}
	else if ( proficiencyLevel >= SKILL_LEVEL_NOVICE )
	{
		return adjectives["proficiency_levels"]["novice"];
	}
	else
	{
		return adjectives["proficiency_levels"]["none"];
	}
}

bool ItemTooltips_t::bIsSpellDamageOrHealingType(spell_t* spell)
{
	if ( !spell )
	{
		return false;
	}
	if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING) != spellItems[spell->ID].spellTags.end()
		|| spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_DAMAGE) != spellItems[spell->ID].spellTags.end() )
	{
		return true;
	}
	return false;
}

bool ItemTooltips_t::bSpellHasBasicHitMessage(const int spellID)
{
	if ( spellItems.find(spellID) != spellItems.end() )
	{
		auto& entry = spellItems[spellID];
		if ( entry.spellTags.find(SPELL_TAG_BASIC_HIT_MESSAGE) != entry.spellTags.end() )
		{
			return true;
		}
	}
	return false;
}

int ItemTooltips_t::getSpellDamageOrHealAmount(const int player, spell_t* spell, Item* spellbook, const bool excludePlayerStats)
{
#ifdef EDITOR
	return 0;
#else
	if ( !spell )
	{
		return 0;
	}
	node_t* rootNode = spell->elements.first;
	spellElement_t* elementRoot = nullptr;
	if ( rootNode )
	{
		elementRoot = (spellElement_t*)(rootNode->element);
	}
	int damage = 0;
	int mana = 0;
	int heal = 0;
	spellElement_t* primaryElement = nullptr;
	if ( elementRoot )
	{
		node_t* primaryNode = elementRoot->elements.first;
		mana = elementRoot->mana;
		heal = mana;
		if ( primaryNode )
		{
			primaryElement = (spellElement_t*)(primaryNode->element);
			if ( primaryElement )
			{
				damage = primaryElement->damage;
			}
		}
		if ( player >= 0 && players[player] )
		{
			int bonus = 0;
			if ( spellbook && itemCategory(spellbook) == MAGICSTAFF )
			{
				// no modifier.
			}
			else
			{
				if ( spellbook && itemCategory(spellbook) == SPELLBOOK )
				{
					bonus = getSpellbookBonusPercent(
						excludePlayerStats ? nullptr : players[player]->entity, 
						excludePlayerStats ? nullptr : stats[player], 
						spellbook);
				}
				damage += (damage * (bonus * 0.01 
					+ getBonusFromCasterOfSpellElement(
						excludePlayerStats ? nullptr : players[player]->entity,
						excludePlayerStats ? nullptr : stats[player], primaryElement, spell ? spell->ID : SPELL_NONE)));
				heal += (heal * (bonus * 0.01 
					+ getBonusFromCasterOfSpellElement(
						excludePlayerStats ? nullptr : players[player]->entity,
						excludePlayerStats ? nullptr : stats[player], primaryElement, spell ? spell->ID : SPELL_NONE)));
			}
		}
		if ( spell->ID == SPELL_HEALING || spell->ID == SPELL_EXTRAHEALING )
		{
			damage = heal;
		}
	}
	return damage;
#endif
}

std::string ItemTooltips_t::getSpellDescriptionText(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell || spellItems.find(spell->ID) == spellItems.end() )
	{
		return defaultString;
	}
	std::string templateName = "template_desc_";
	templateName += spellItems[spell->ID].internalName;

	if ( templates.find(templateName) == templates.end() )
	{
		return defaultString;
	}

	std::string str;
	for ( auto it = templates[templateName].begin();
		it != templates[templateName].end(); ++it )
	{
		str += *it;
		if ( std::next(it) != ItemTooltips.templates[templateName].end() )
		{
			str += '\n';
		}
	}
	return str;
#endif
}

std::string& ItemTooltips_t::getIconLabel(Item& item)
{
#ifndef EDITOR
	return tmpItems[item.type].iconLabelPath;
#endif
}

std::string ItemTooltips_t::getSpellIconText(const int player, Item& item, const bool compendiumTooltipIntro)
{
#ifndef EDITOR
	spell_t* spell = nullptr;
	
	if ( itemCategory(&item) == SPELLBOOK )
	{
		spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
	}
	else if ( itemCategory(&item) == MAGICSTAFF )
	{
		for ( auto& s : spellItems )
		{
			if ( s.second.magicstaffId == item.type )
			{
				spell = getSpellFromID(s.first);
				break;
			}
		}
	}
	else
	{
		spell = getSpellFromItem(player, &item, false);
	}
	if ( !spell || spellItems.find(spell->ID) == spellItems.end() )
	{
		return defaultString;
	}
	std::string templateName = "template_icon_";
	templateName += spellItems[spell->ID].internalName;

	if ( templates.find(templateName) == templates.end() )
	{
		return defaultString;
	}

	std::string str;
	for ( auto it = templates[templateName].begin();
		it != templates[templateName].end(); ++it )
	{
		str += *it;
		if ( std::next(it) != ItemTooltips.templates[templateName].end() )
		{
			str += '\n';
		}
	}

	if ( spellItems[spell->ID].internalName == "spell_summon" )
	{
		int numSummons = 1;
		if ( !compendiumTooltipIntro )
		{
			if ( (statGetINT(stats[player], players[player]->entity)
				+ stats[player]->getModifiedProficiency(PRO_MAGIC)) >= SKILL_LEVEL_EXPERT )
			{
				numSummons = 2;
			}
		}
		char buf[128];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), str.c_str(), numSummons);
		str = buf;
	}
	else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING) != spellItems[spell->ID].spellTags.end()
		|| spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_DAMAGE) != spellItems[spell->ID].spellTags.end() )
	{
		char buf[128];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), str.c_str(), getSpellDamageOrHealAmount(player, spell, &item, compendiumTooltipIntro));
		str = buf;
	}

	return str;
#else
	return std::string("");
#endif
}

real_t ItemTooltips_t::getSpellSustainCostPerSecond(int spellID)
{
	real_t cost = 0.0;
	switch ( spellID )
	{
		case SPELL_REFLECT_MAGIC:
			cost = 6.0;
			break;
		case SPELL_LEVITATION:
			cost = 0.6;
			break;
		case SPELL_INVISIBILITY:
			cost = 1.0;
			break;
		case SPELL_LIGHT:
			cost = 15.0;
			break;
		case SPELL_VAMPIRIC_AURA:
			cost = 0.33;
			break;
		case SPELL_AMPLIFY_MAGIC:
			cost = 0.25;
			break;
		default:
			break;
	}
	return cost;
}

std::string& ItemTooltips_t::getSpellTypeString(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell )
	{
		return defaultString;
	}
	switch ( spellItems[spell->ID].spellType )
	{
		case SPELL_TYPE_AREA:
			return adjectives["spell_strings"]["spell_type_area"];
			break;
		case SPELL_TYPE_PROJECTILE:
			return adjectives["spell_strings"]["spell_type_projectile"];
			break;
		case SPELL_TYPE_SELF:
			return adjectives["spell_strings"]["spell_type_self"];
			break;
		case SPELL_TYPE_SELF_SUSTAIN:
			return adjectives["spell_strings"]["spell_type_self_sustain"];
			break;
		case SPELL_TYPE_PROJECTILE_SHORT_X3:
			return adjectives["spell_strings"]["spell_type_projectile_3x"];
			break;
		case SPELL_TYPE_DEFAULT:
		default:
			return defaultString;
			break;
	}
#endif
}

std::string ItemTooltips_t::getCostOfSpellString(const int player, Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	spell_t* spell = getSpellFromItem(player, &item, false);
	if ( !spell )
	{
		return defaultString;
	}
	char buf[64];
	memset(buf, 0, sizeof(buf));
	if ( spell->ID == SPELL_DOMINATE )
	{
		std::string templateName = "template_spell_cost_dominate";
		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
	}
	else if ( spell->ID == SPELL_DEMON_ILLUSION )
	{
		std::string templateName = "template_spell_cost_demon_illusion";
		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
	}
	else
	{
		std::string templateName = "template_spell_cost";
		real_t sustainCostPerSecond = getSpellSustainCostPerSecond(spell->ID);
		if ( sustainCostPerSecond > 0.01 )
		{
			templateName = "template_spell_cost_sustained";
		}

		std::string str;
		for ( auto it = templates[templateName].begin();
			it != templates[templateName].end(); ++it )
		{
			str += *it;
			if ( std::next(it) != ItemTooltips.templates[templateName].end() )
			{
				str += '\n';
			}
		}
		snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
		if ( players[player] && players[player]->entity )
		{
			if ( sustainCostPerSecond > 0.01 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					getCostOfSpell(spell, players[player]->entity), sustainCostPerSecond);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell, players[player]->entity));
			}
		}
		else
		{
			if ( sustainCostPerSecond > 0.01 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell), sustainCostPerSecond);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getCostOfSpell(spell));
			}
		}
	}
	return buf;
#endif
}

node_t* ItemTooltips_t::getSpellNodeFromSpellID(int spellID)
{
	node_t* spellImageNode = nullptr;
	if ( spellID >= NUM_SPELLS || spellID < SPELL_NONE )
	{
		return nullptr;
	}

	if ( arachnophobia_filter )
	{
		if ( spellID == SPELL_SPIDER_FORM )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_FORM);
		}
		else if ( spellID == SPELL_SPRAY_WEB )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_WEB);
		}
		else
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
		}
	}
	else
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, spellID);
	}
	return spellImageNode;
}

std::string ItemTooltips_t::getSpellIconPath(const int player, Item& item, int spellID)
{
#ifdef EDITOR
	return "items/images/null.png";
#else
	node_t* spellImageNode = nullptr;
	if ( itemCategory(&item) == MAGICSTAFF )
	{
		spell_t* spell = nullptr;
		for ( auto& s : spellItems )
		{
			if ( s.second.magicstaffId == item.type )
			{
				spell = getSpellFromID(s.first);
				break;
			}
		}
		if ( spell )
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, spell->ID);
		}
		else
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, 0);
		}
	}
	else if ( itemCategory(&item) == SPELLBOOK )
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, getSpellIDFromSpellbook(item.type));
	}
	else if ( item.type == TOOL_SPELLBOT )
	{
		spellImageNode = list_Node(&items[SPELL_ITEM].images, item.status < EXCELLENT ? SPELL_FORCEBOLT : SPELL_MAGICMISSILE);
	}
	else if ( item.type == SPELL_ITEM )
	{
		if ( spellID > SPELL_NONE )
		{
			spellImageNode = getSpellNodeFromSpellID(spellID);
		}
		else
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( spell )
			{
				spellImageNode = getSpellNodeFromSpellID(spell->ID);
			}
			else
			{
				spellImageNode = getSpellNodeFromSpellID(SPELL_NONE);
			}
		}
	}
	if ( spellImageNode )
	{
		string_t* string = (string_t*)spellImageNode->element;
		if ( string )
		{
			return string->data;
		}
	}
	return "items/images/null.png";
#endif
}

std::string& ItemTooltips_t::getItemPotionAlchemyAdjective(const int player, Uint32 itemType)
{
#ifdef EDITOR
	return defaultString;
#else
	if ( adjectives.find("potion_alchemy_types") == adjectives.end() )
	{
		return defaultString;
	}
	if ( clientLearnedAlchemyIngredients[player].find(itemType) == clientLearnedAlchemyIngredients[player].end() )
	{
		return adjectives["potion_alchemy_types"]["unknown"];
	}
	else if ( GenericGUI[player].isItemBaseIngredient(itemType) )
	{
		return adjectives["potion_alchemy_types"]["base_ingredient"];
	}
	else if ( GenericGUI[player].isItemSecondaryIngredient(itemType) )
	{
		return adjectives["potion_alchemy_types"]["secondary_ingredient"];
	}
	else
	{
		return adjectives["potion_alchemy_types"]["no_ingredient"];
	}
#endif
}

std::string& ItemTooltips_t::getItemPotionHarmAllyAdjective(Item& item)
{
#ifdef EDITOR
	return defaultString;
#else
	if ( adjectives.find("potion_ally_damage") == adjectives.end() )
	{
		return defaultString;
	}

	if ( items[item.type].hasAttribute("POTION_TYPE_GOOD_EFFECT") 
		|| items[item.type].hasAttribute("POTION_TYPE_HEALING") /*item.doesPotionHarmAlliesOnThrown()*/ )
	{
		return adjectives["potion_ally_damage"]["no_harm_ally"];
	}
	else
	{
		return adjectives["potion_ally_damage"]["harm_ally"];
	}
#endif
}

std::string& ItemTooltips_t::getItemProficiencyName(int proficiency)
{
	if ( adjectives.find("proficiency_types") == adjectives.end() )
	{
		return defaultString;
	}

	switch ( proficiency )
	{
		case PRO_SWORD:
			return adjectives["proficiency_types"]["sword"];
		case PRO_AXE:
			return adjectives["proficiency_types"]["axe"];
		case PRO_MACE:
			return adjectives["proficiency_types"]["mace"];
		case PRO_POLEARM:
			return adjectives["proficiency_types"]["polearm"];
		case PRO_UNARMED:
			return adjectives["proficiency_types"]["unarmed"];
		case PRO_SHIELD:
			return adjectives["proficiency_types"]["shield"];
		case PRO_RANGED:
			return adjectives["proficiency_types"]["ranged"];
		default:
			return defaultString;
	}
}

std::string& ItemTooltips_t::getItemSlotName(ItemEquippableSlot slotname)
{
	switch ( slotname )
	{
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET:
			return adjectives["equipment_slot_types"]["amulet"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING:
			return adjectives["equipment_slot_types"]["ring"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE:
			return adjectives["equipment_slot_types"]["breastpiece"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM:
			return adjectives["equipment_slot_types"]["helm"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS:
			return adjectives["equipment_slot_types"]["boots"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES:
			return adjectives["equipment_slot_types"]["gloves"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK:
			return adjectives["equipment_slot_types"]["cloak"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK:
			return adjectives["equipment_slot_types"]["mask"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON:
			return adjectives["equipment_slot_types"]["mainhand"];
		case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD:
			return adjectives["equipment_slot_types"]["offhand"];
		default:
			break;
	}
	return adjectives["equipment_slot_types"]["unknown"];
}

std::string& ItemTooltips_t::getItemStatShortName(const char* attr)
{
    const std::string attribute = attr;
	if ( attribute == "STR" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "DEX" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "CON" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "INT" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "PER" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "CHR" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	else if ( attribute == "AC" )
	{
		return adjectives["stat_short_name"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemStatFullName(const char* attr)
{
    const std::string attribute = attr;
	if ( attribute == "STR" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "DEX" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "CON" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "INT" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "PER" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "CHR" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	else if ( attribute == "AC" )
	{
		return adjectives["stat_long_name"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemEquipmentEffectsForIconText(std::string& attribute)
{
	if ( adjectives["equipment_effects_icon_text"].find(attribute) != adjectives["equipment_effects_icon_text"].end() )
	{
		return adjectives["equipment_effects_icon_text"][attribute];
	}
	return defaultString;
}

std::string& ItemTooltips_t::getItemEquipmentEffectsForAttributesText(std::string& attribute)
{
	if ( adjectives["equipment_effects_attributes_text"].find(attribute) != adjectives["equipment_effects_attributes_text"].end() )
	{
		return adjectives["equipment_effects_attributes_text"][attribute];
	}
	return defaultString;
}

Sint32 getStatAttributeBonusFromItem(const int player, Item& item, std::string& attribute)
{
#ifndef EDITOR
	Sint32 stat = 0;
	bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
	if ( item.beatitude >= 0 || cursedItemIsBuff )
	{
		stat += items[item.type].attributes[attribute];
	}
	stat += (cursedItemIsBuff ? abs(item.beatitude) : item.beatitude);
	return stat;
#else
	return 0;
#endif
}

void ItemTooltips_t::formatItemIcon(const int player, std::string tooltipType, Item& item, std::string& str, int iconIndex, std::string& conditionalAttribute, Frame* parentFrame)
{
#ifndef EDITOR
	auto itemTooltip = tooltips[tooltipType];
	static Stat itemDummyStat(0);
	static char buf[1024];
	memset(buf, 0, sizeof(buf));

	bool compendiumTooltip = false;
	bool compendiumTooltipIntro = false;
	if ( parentFrame && !strcmp(parentFrame->getName(), "compendium") )
	{
		compendiumTooltip = true;
		if ( intro )
		{
			compendiumTooltipIntro = true;
		}
	}

	if ( conditionalAttribute.find("magicstaff_") != std::string::npos )
	{
		if ( str == "" )
		{
			str = getSpellIconText(player, item, compendiumTooltipIntro);
		}
		return;
	}
	else if ( conditionalAttribute.find("SPELL_") != std::string::npos )
	{
		if ( conditionalAttribute == "SPELL_ICON_MANACOST" )
		{
			str = getCostOfSpellString(player, item);
		}
		else if ( conditionalAttribute == "SPELL_ICON_EFFECT" )
		{
			str = getSpellIconText(player, item, compendiumTooltipIntro);
		}
		return;
	}
	else if ( conditionalAttribute.find("SPELLBOOK_") != std::string::npos )
	{
		if ( conditionalAttribute == "SPELLBOOK_SPELLINFO_LEARNED" )
		{
			str = getSpellIconText(player, item, compendiumTooltipIntro);
			return;
		}
		else if ( conditionalAttribute == "SPELLBOOK_SPELLINFO_UNLEARNED" )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( spell )
			{
				snprintf(buf, sizeof(buf), str.c_str(), spell->getSpellName());
			}
		}
		else if ( conditionalAttribute == "SPELLBOOK_CAST_BONUS"
			&& items[item.type].hasAttribute(conditionalAttribute) )
		{
			int spellBookBonusPercent = 0;
			spellBookBonusPercent += getSpellbookBonusPercent(
				compendiumTooltipIntro ? nullptr : players[player]->entity, 
				compendiumTooltipIntro ? nullptr : stats[player], &item);
			spellBookBonusPercent *= ((items[item.type].attributes["SPELLBOOK_CAST_BONUS"]) / 100.0);

			int spellID = getSpellIDFromSpellbook(item.type);
			if ( spellItems.find(spellID) == spellItems.end() )
			{
				return;
			}
			SpellItemTypes spellType = spellItems[spellID].spellType;

			if ( spellItems[spellID].spellTags.find(SPELL_TAG_DAMAGE) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_damage_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_damage_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_damage_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_HEALING) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_heal_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_heal_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_heal_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_STATUS_EFFECT) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_duration_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_duration_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_duration_bonus"].end() )
					{
						str += '\n';
					}
				}
			}
			else if ( spellItems[spellID].spellTags.find(SPELL_TAG_CURE) != spellItems[spellID].spellTags.end() )
			{
				str = "";
				for ( auto it = ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].begin();
					it != ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].end(); ++it )
				{
					str += *it;
					if ( std::next(it) != ItemTooltips.templates["template_spellbook_icon_cureailment_bonus"].end() )
					{
						str += '\n';
					}
				}
				int bonusSeconds = 10 * ((spellBookBonusPercent * 4) / 100.0); // 25% = 10 seconds, 50% = 20 seconds.
				snprintf(buf, sizeof(buf), str.c_str(), bonusSeconds);
				str = buf;
				return;
			}

			snprintf(buf, sizeof(buf), str.c_str(), spellBookBonusPercent);
		}
		else
		{
			return;
		}
		str = buf;
		return;
	}
	else if ( tooltipType.find("tooltip_tool_bomb") != std::string::npos )
	{
		if ( conditionalAttribute.find("BOMB_ATK") != std::string::npos )
		{
			int baseDamage = items[item.type].attributes["BOMB_ATK"];
			int baseSpellDamage = 0;
			if ( item.type == TOOL_FREEZE_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr, compendiumTooltipIntro);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr, compendiumTooltipIntro);
			}
			int bonusFromPER = std::max(0, statGetPER(stats[player], players[player]->entity)) * items[item.type].attributes["BOMB_DMG_PER_MULT"];
			if ( compendiumTooltipIntro )
			{
				bonusFromPER = 0;
			}
			bonusFromPER /= 100;
			snprintf(buf, sizeof(buf), str.c_str(), baseDamage + bonusFromPER + baseSpellDamage);
			str = buf;
		}
		return;
	}
	else if ( item.type == TOOL_SENTRYBOT || item.type == TOOL_SPELLBOT
		|| item.type == TOOL_GYROBOT || item.type == TOOL_DUMMYBOT )
	{
		switch ( item.type )
		{
			case TOOL_SENTRYBOT: itemDummyStat.type = SENTRYBOT; break;
			case TOOL_SPELLBOT: itemDummyStat.type = SPELLBOT; break;
			case TOOL_GYROBOT: itemDummyStat.type = GYROBOT; break;
			case TOOL_DUMMYBOT: itemDummyStat.type = DUMMYBOT; break;
			default:
				break;
		}
		Entity::tinkerBotSetStats(&itemDummyStat, item.status);
		if ( conditionalAttribute.find("TINKERBOT_RANGEDATK") != std::string::npos )
		{
			int baseDamage = items[CROSSBOW].attributes["ATK"] + 1;
			int statDMG = itemDummyStat.PER + itemDummyStat.DEX;
			int skillBonus = SKILL_LEVEL_MASTER / 20;
			snprintf(buf, sizeof(buf), str.c_str(), baseDamage + statDMG + skillBonus);
			str = buf;
		}
		else if ( conditionalAttribute.find("TINKERBOT_MAGICATK") != std::string::npos )
		{
			int spellID = item.status == EXCELLENT ? SPELL_MAGICMISSILE : SPELL_FORCEBOLT;
			int spellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(spellID), nullptr, compendiumTooltipIntro);
			snprintf(buf, sizeof(buf), str.c_str(), spellDamage);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_HPAC" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.MAXHP, itemDummyStat.CON);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_HP" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.MAXHP);
			str = buf;
		}
		else if ( conditionalAttribute == "TINKERBOT_AC" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), itemDummyStat.CON);
			str = buf;
		}
		return;
	}
	else if ( conditionalAttribute == "EFF_MONOCLE_APPRAISE" )
	{
		int appraisalMult = 200;
		if ( item.beatitude > 0 )
		{
			appraisalMult = 400;
		}

		snprintf(buf, sizeof(buf), str.c_str(), appraisalMult);
		str = buf;
		return;
	}
	else if ( conditionalAttribute.compare("") != 0 && items[item.type].hasAttribute(conditionalAttribute) )
	{
		if ( conditionalAttribute == "STR" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "DEX" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "CON" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "INT" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "PER" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute == "CHR" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute.c_str()).c_str());
		}
		else if ( conditionalAttribute.find("EFF_") != std::string::npos )
		{
			if ( conditionalAttribute == "EFF_PARALYZE" )
			{
				if ( item.type == TOOL_BEARTRAP )
				{
					snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes["EFF_PARALYZE"] / TICKS_PER_SECOND);
				}
			}
			else if ( conditionalAttribute == "EFF_LIFESAVING" )
			{
				if ( item.type == AMULET_LIFESAVING )
				{
					real_t restore = 0.5;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						restore += 0.5 * std::min(2, abs(item.beatitude));
					}
					restore *= 100.0;
					snprintf(buf, sizeof(buf), str.c_str(), (int)restore);
				}
			}
			else if ( conditionalAttribute == "EFF_BLEEDING" )
			{
				if ( item.type == TOOL_BEARTRAP )
				{
					snprintf(buf, sizeof(buf), str.c_str(), items[item.type].attributes["EFF_BLEEDING"] / TICKS_PER_SECOND);
				}
			}
			else if ( conditionalAttribute == "EFF_REGENERATION" )
			{
				if ( item.type == RING_REGENERATION )
				{
					int healring = std::min(2, std::max((shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude) + 1, 1));
					snprintf(buf, sizeof(buf), str.c_str(), healring,
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else
				{
					int healring = 1;
					snprintf(buf, sizeof(buf), str.c_str(), healring,
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute == "EFF_MP_REGENERATION" )
			{
				int manaring = 1;
				snprintf(buf, sizeof(buf), str.c_str(), manaring,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_CLOAK_GUARDIAN1" )
			{
				real_t res = 0.75;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					res = std::max(0.25, 0.75 - 0.25 * (abs(item.beatitude)));
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(res * 100))),
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_CLOAK_GUARDIAN2" )
			{
				real_t res = 0.5;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					res = std::max(0.25, 0.5 - 0.25 * (abs(item.beatitude)));
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(res * 100))),
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_MARIGOLD1"
				|| conditionalAttribute == "EFF_MARIGOLD1_HUNGER" )
			{
				int foodMod = (svFlags & SV_FLAG_HUNGER) ? 5 : 3;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					foodMod += 3 * std::min(2, (int)abs(item.beatitude));
				}
				snprintf(buf, sizeof(buf), str.c_str(), foodMod,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_RING_RESOLVE" )
			{
				int effect = 10;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					effect += 10 * std::min(2, (int)abs(item.beatitude));
				}
				snprintf(buf, sizeof(buf), str.c_str(), effect,
					getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
			else if ( conditionalAttribute == "EFF_COLDRESIST" )
			{
				if ( item.type == HAT_WARM )
				{
					real_t coldMultiplier = 1.0;
					if ( !(players[player]->entity && players[player]->entity->effectShapeshift != NOTHING) )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							coldMultiplier = std::max(0.0, 0.5 - 0.25 * (abs(item.beatitude)));
						}
						else
						{
							coldMultiplier = 0.50;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)((100 - (int)(coldMultiplier * 100))),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute == "EFF_TECH_GOGGLES1" )
			{
				real_t speedFactor = 1.0;
				if ( item.type == MASK_TECH_GOGGLES )
				{
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						speedFactor = std::min(speedFactor + (1 + abs(item.beatitude)) * 0.5, 3.0);
					}
					else
					{
						speedFactor = speedFactor + 0.5;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)((speedFactor - 1.0) * 100));
			}
			else if ( conditionalAttribute == "EFF_EYEPATCH" )
			{
				int bonus = 0;
				if ( item.type == MASK_EYEPATCH )
				{
					bonus = 2;
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						bonus += abs(item.beatitude);
					}
					else if ( item.beatitude < 0 )
					{
						bonus = 2;
					}
				}
				bonus = std::max(-6, std::min(bonus, 4));
				snprintf(buf, sizeof(buf), str.c_str(), bonus);
			}
			else if ( conditionalAttribute == "EFF_STRAFE" )
			{
				double backpedalMultiplier = 0.25;
				if ( item.type == HAT_BANDANA )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						backpedalMultiplier += 0.5 * (1 + abs(item.beatitude)) * 0.25;
						backpedalMultiplier = std::min(0.75, backpedalMultiplier);
					}
					else
					{
						backpedalMultiplier += 0.5 * (1 + abs(item.beatitude)) * 0.25;
						backpedalMultiplier = std::min(0.75, backpedalMultiplier);
					}
				}
				int multBackpedal = 100 * (backpedalMultiplier - 0.25);
				snprintf(buf, sizeof(buf), str.c_str(), multBackpedal);
			}
			else if ( conditionalAttribute == "EFF_MASK_GOLDEN" )
			{
				int equipmentBonus = 100;
				if ( item.type == MASK_GOLDEN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						equipmentBonus -= 50 * (1 + abs(item.beatitude));
						equipmentBonus = std::max(-50, equipmentBonus);
					}
					else
					{
						equipmentBonus -= 50 * (abs(item.beatitude));
						equipmentBonus = std::max(0, equipmentBonus);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100 - equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_PIPE" )
			{
				int chance = 0;
				if ( item.type == MASK_PIPE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance = std::min(25 + (10 * abs(item.beatitude)), 50);
					}
					else
					{
						chance = std::min(25 + (10 * abs(item.beatitude)), 50);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), chance);
			}
			else if ( conditionalAttribute == "EFF_HOOD_APPRENTICE" )
			{
				int chance = 0;
				if ( item.type == HAT_HOOD_APPRENTICE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance = std::min(30 + (10 * abs(item.beatitude)), 50);
					}
					else
					{
						chance = std::min(30 + (10 * abs(item.beatitude)), 50);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), chance);
			}
			else if ( conditionalAttribute == "EFF_HOOD_ASSASSIN" )
			{
				int bonus = 0;
				if ( item.type == HAT_HOOD_ASSASSIN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(4 + (2 * abs(item.beatitude)), 8);
					}
					else
					{
						bonus = 4;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), bonus);
			}
			else if ( conditionalAttribute == "EFF_HOOD_WHISPERS" )
			{
				/*int bonus = 0;
				if ( item.type == HAT_HOOD_WHISPERS )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						bonus = std::min(50 + (10 * abs(item.beatitude)), 100);
					}
					else
					{
						bonus = 50;
					}
				}*/

				int val = ((compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20)) + 2) * 2; // backstab dmg
				if ( skillCapstoneUnlocked(player, PRO_STEALTH) )
				{
					val *= 2;
				}

				real_t equipmentModifier = 0.0;
				real_t bonusModifier = 1.0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentModifier += (std::min(50 + (10 * abs(item.beatitude)), 100)) / 100.0;
				}
				else
				{
					equipmentModifier = 0.5;
					bonusModifier = 0.5;
				}
				val = ((val * equipmentModifier) * bonusModifier);

				snprintf(buf, sizeof(buf), str.c_str(), val);
			}
			else if ( conditionalAttribute == "EFF_THORNS" )
			{
				int dmg = 0;
				if ( item.type == MASK_MOUTHKNIFE )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						dmg = (1 + abs(item.beatitude)) * 2;
					}
					else
					{
						dmg = -2 * (1 + abs(item.beatitude));
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), abs(dmg));
			}
			else if ( conditionalAttribute == "EFF_CHEF" )
			{
				real_t foodMult = 1.0;
				if ( item.type == HAT_CHEF )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						if ( svFlags & SV_FLAG_HUNGER )
						{
							foodMult += 0.2 + abs(item.beatitude) * 0.1;
						}
						else
						{
							foodMult += 0.5 + abs(item.beatitude) * 0.25;
						}
					}
					else
					{
						foodMult += 0.2;
					}
					foodMult = std::max(0.2, foodMult);
				}
				snprintf(buf, sizeof(buf), str.c_str(), (int)(foodMult * 100) - 100);
			}
			else if ( conditionalAttribute == "EFF_CHEF2" )
			{
				int chance = 0;
				if ( item.type == HAT_CHEF )
				{
					chance = 20;
					bool cursedChef = false;
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						chance -= 5 * abs(item.beatitude);
						chance = std::max(10, chance);
					}
					else
					{
						chance -= 5 * abs(item.beatitude);
						chance = std::max(10, chance);
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100.0 / chance);
			}
			else if ( conditionalAttribute == "EFF_INSPIRATION" )
			{
				int inspiration = 0;
				if ( item.type == HAT_LAURELS
					|| item.type == HAT_TURBAN
					|| item.type == HAT_CROWN )
				{
					if ( item.beatitude >= 0 )
					{
						inspiration = std::min(300, 25 + (item.beatitude * 25));
					}
					else if ( shouldInvertEquipmentBeatitude(stats[player]) )
					{
						inspiration = std::min(300, 25 + (abs(item.beatitude) * 25));
					}
					else
					{
						inspiration = 25;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), inspiration);
			}
			else if ( conditionalAttribute == "EFF_CELEBRATION" )
			{
				int hpMod = 0;
				if ( item.type == HAT_CROWN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						hpMod += std::min(50, ((20 + 10 * (abs(item.beatitude)))));
					}
					else
					{
						hpMod = 20;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), hpMod);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_REGEN" )
			{
				int regen = 0;
				if ( item.type == HAT_LAURELS )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						regen = 1 + abs(item.beatitude) * 1;
					}
					else
					{
						regen = 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 2 * regen);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_TRAPRESIST" )
			{
				int resist = 0;
				if ( item.type == HAT_TURBAN )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						resist = std::min(100, 50 + abs(item.beatitude) * 25);
					}
					else
					{
						resist = 50;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), resist);
			}
			else if ( conditionalAttribute == "EFF_FOLLOWER_DMGRESIST" )
			{
				int resist = 0;
				if ( item.type == HAT_CROWNED_HELM )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						resist = std::min(50, 20 + abs(item.beatitude) * 10);
					}
					else
					{
						resist = 20;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), resist);
			}
			else if ( conditionalAttribute == "EFF_SPRIG" )
			{
				real_t mult = 0.0;
				if ( item.type == MASK_GRASS_SPRIG )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						mult = std::min(1.25 + (0.25 * abs(item.beatitude)), 2.0);
					}
					else
					{
						mult = 1.25;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), ((1.0 / mult)));
			}
			else if ( conditionalAttribute == "EFF_SKILL_MELEE_STEEL" )
			{
				int equipmentBonus = 0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * 5);
				}
				else
				{
					equipmentBonus += 5;
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_SKILL_MELEE_ARTIFACT" )
			{
				int equipmentBonus = 0;
				if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				{
					equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * 10);
				}
				else
				{
					equipmentBonus += 10;
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_BOUNTY" )
			{
				int equipmentBonus = 0;
				if ( item.type == HAT_BOUNTYHUNTER )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						if ( abs(item.beatitude) >= 2 )
						{
							equipmentBonus += 2;
						}
						else
						{
							equipmentBonus += 1;
						}
					}
					else
					{
						equipmentBonus += 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus);
			}
			else if ( conditionalAttribute == "EFF_RANGED_DISTANCE" )
			{
				int dropOffModifier = 0;
				if ( item.type == HAT_BYCOCKET )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						dropOffModifier = std::min(3, 1 + abs(item.beatitude));
					}
					else
					{
						dropOffModifier = 1;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), dropOffModifier);
			}
			else if ( conditionalAttribute == "EFF_RANGED_FIRERATE" )
			{
				int equipmentBonus = 0;
				if ( item.type == HAT_BYCOCKET )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						equipmentBonus -= std::min(30, 10 + 10 * abs(item.beatitude));
					}
					else
					{
						equipmentBonus -= 30;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), -equipmentBonus);
			}
			else if ( conditionalAttribute.find("EFF_SKILL_") != std::string::npos )
			{
				int skill = std::stoi(conditionalAttribute.substr(strlen("EFF_SKILL_"), std::string::npos));
				int equipmentBonus = 0;
				if ( (skill == PRO_TRADING && item.type == MASK_GOLDEN)
					|| (skill == PRO_LEADERSHIP && item.type == HAT_PLUMED_CAP)
					|| (skill == PRO_RANGED && item.type == HAT_BOUNTYHUNTER)
					|| (skill == PRO_STEALTH && item.type == HAT_HOOD_WHISPERS)
					|| (skill == PRO_SPELLCASTING && (item.type == HAT_CIRCLET || item.type == HAT_CIRCLET_WISDOM))
					|| (skill == PRO_ALCHEMY && item.type == MASK_HAZARD_GOGGLES) )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						equipmentBonus += std::min(Stat::maxEquipmentBonusToSkill, (1 + abs(item.beatitude)) * 10);
					}
					else
					{
						equipmentBonus += 10;
					}
				}

				std::string skillName = "";
				for ( auto s : Player::SkillSheet_t::skillSheetData.skillEntries )
				{
					if ( s.skillId == skill )
					{
						skillName = s.name;
						break;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), equipmentBonus, skillName.c_str());
			}
			else if ( conditionalAttribute == "EFF_BOULDER_RES" )
			{
				real_t mult = 1.0;
				if ( item.type == HAT_TOPHAT )
				{
					mult = 0.0;
				}
				else if ( item.type == HELM_MINING )
				{
					mult = 0.5;
					bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats[player]);
					if ( item.beatitude >= 0 || cursedItemIsBuff )
					{
						mult -= 0.25 * abs(item.beatitude);
						mult = std::max(0.0, mult);
					}
					else
					{
						mult = 0.5;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), 100 - (int)(mult * 100));
			}
			else if ( conditionalAttribute.find("EFF_PWR") != std::string::npos )
			{
				real_t bonus = 0.0;
				if ( conditionalAttribute == "EFF_PWR" )
				{
					if ( item.type == HAT_CIRCLET
						|| item.type == HAT_CIRCLET_WISDOM )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.05 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.05;
						}
					}
					else if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else if ( conditionalAttribute == "EFF_PWR_DMG" )
				{
					if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
				else if ( conditionalAttribute == "EFF_PWR_HEAL" )
				{
					if ( item.type == HAT_MITER || item.type == HAT_HEADDRESS )
					{
						if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
						{
							bonus += (0.10 + (0.05 * abs(item.beatitude)));
						}
						else
						{
							bonus = 0.1;
						}
					}
					snprintf(buf, sizeof(buf), str.c_str(), (int)(bonus * 100),
						getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
				}
			}
			else if ( conditionalAttribute.find("EFF_ARTIFACT_") != std::string::npos )
			{
				real_t amount = 0.0;
				real_t percent = getArtifactWeaponEffectChance(item.type, *stats[player], &amount);
				if ( conditionalAttribute == "EFF_ARTIFACT_SWORD" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_AXE" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, (amount * 100) - 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_MACE" )
				{
					amount = amount / MAGIC_REGEN_TIME;
					snprintf(buf, sizeof(buf), str.c_str(), 100 * amount);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_SPEAR" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else if ( conditionalAttribute == "EFF_ARTIFACT_BOW" )
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount * 100);
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), percent, amount);
				}
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), getItemEquipmentEffectsForIconText(conditionalAttribute).c_str());
			}
		}
		else if ( conditionalAttribute == "AC" )
		{
			Sint32 AC = item.armorGetAC(stats[player]);
			snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName("AC").c_str());
		}
		else
		{
			return;
		}
		str = buf;
		return;
	}

	if ( tooltipType.find("tooltip_armor") != std::string::npos
		|| tooltipType.find("tooltip_offhand") != std::string::npos
		|| tooltipType.find("tooltip_ring") != std::string::npos )
	{
		Sint32 AC = item.armorGetAC(stats[player]);
		snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName("AC").c_str());
	}
	else if ( tooltipType.find("tooltip_mace") != std::string::npos
		|| tooltipType.find("tooltip_sword") != std::string::npos
		|| tooltipType.find("tooltip_whip") != std::string::npos
		|| tooltipType.find("tooltip_polearm") != std::string::npos
		|| tooltipType.find("tooltip_thrown") != std::string::npos
		|| tooltipType.find("tooltip_boomerang") != std::string::npos
		|| tooltipType.find("tooltip_gem") != std::string::npos
		|| tooltipType.find("tooltip_ranged") != std::string::npos
		|| tooltipType.find("tooltip_quiver") != std::string::npos
		|| tooltipType.compare("tooltip_tool_pickaxe") == 0 
		)
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.find("tooltip_axe") != std::string::npos )
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		atk += 1;
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.compare("tooltip_food_tin") == 0 )
	{
		std::string cookingMethod, protein, sides;
		item.foodTinGetDescription(cookingMethod, protein, sides);
		snprintf(buf, sizeof(buf), str.c_str(), protein.c_str());
	}
	else if ( tooltipType.find("tooltip_potion") != std::string::npos )
	{
		if ( items[item.type].hasAttribute("POTION_TYPE_HEALING") )
		{
			if ( item.type == POTION_HEALING || item.type == POTION_EXTRAHEALING || item.type == POTION_RESTOREMAGIC )
			{
				int healthVal = item.potionGetEffectHealth(players[player]->entity, stats[player]);

				if ( item.type == POTION_HEALING )
				{
					const int statBonus = compendiumTooltipIntro ? 0 : (2 * std::max(0, statGetCON(stats[player], players[player]->entity)));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_EXTRAHEALING )
				{
					const int statBonus = compendiumTooltipIntro ? 0 : (4 * std::max(0, statGetCON(stats[player], players[player]->entity)));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_RESTOREMAGIC )
				{
					const int statBonus = std::min(30, 2 * (compendiumTooltipIntro ? 0 : (std::max(0, statGetINT(stats[player], players[player]->entity)))));
					healthVal += statBonus;
				}

				snprintf(buf, sizeof(buf), str.c_str(), healthVal);
			}
			else if ( item.type == POTION_BOOZE )
			{
				if ( iconIndex == 1 )
				{
					auto oldBeatitude = item.beatitude;
					item.beatitude = std::max((Sint16)0, item.beatitude);
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND, 
						item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
					item.beatitude = oldBeatitude;
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth(players[player]->entity, stats[player]));
				}
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth(players[player]->entity, stats[player]));
			}
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_DMG") )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDamage(players[player]->entity, stats[player]));
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_GOOD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND, 
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_BAD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / TICKS_PER_SECOND,
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
	}
	else if ( tooltipType.find("tooltip_tool_beartrap") != std::string::npos )
	{
		const int atk = 10 + 3 * (item.status + item.beatitude);
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.find("tooltip_scroll") != std::string::npos )
	{
		if ( conditionalAttribute == "SCROLL_LABEL" )
		{
			if ( compendiumTooltip )
			{
				snprintf(buf, sizeof(buf), str.c_str(), "???"); // hide labels in compendium
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), item.getScrollLabel());
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}
	str = buf;
#endif
}

void ItemTooltips_t::formatItemDescription(const int player, std::string tooltipType, Item& item, std::string& str)
{
	if ( tooltipType.find("tooltip_spell_") != std::string::npos )
	{
		str = getSpellDescriptionText(player, item);
	}
	else if ( item.type == TOOL_SENTRYBOT || item.type == TOOL_SPELLBOT
		|| item.type == TOOL_GYROBOT || item.type == TOOL_DUMMYBOT )
	{
		if ( item.status == BROKEN )
		{
			str = "";
			for ( auto it = ItemTooltips.templates["template_tinkerbot_broken_description"].begin();
				it != ItemTooltips.templates["template_tinkerbot_broken_description"].end(); ++it )
			{
				str += *it;
				if ( std::next(it) != ItemTooltips.templates["template_tinkerbot_broken_description"].end() )
				{
					str += '\n';
				}
			}
		}
	}
	return;
}

void ItemTooltips_t::formatItemDetails(const int player, std::string tooltipType, Item& item, std::string& str, std::string detailTag, Frame* parentFrame)
{
#ifndef EDITOR
	if ( !stats[player] )
	{
		str = "";
		return;
	}
	if ( players[player] && !players[player]->isLocalPlayer() )
	{
		str = "";
		return;
	}

	bool compendiumTooltip = false;
	bool compendiumTooltipIntro = false;
	if ( parentFrame && !strcmp(parentFrame->getName(), "compendium") )
	{
		compendiumTooltip = true;
		if ( intro )
		{
			compendiumTooltipIntro = true;
		}
	}

	auto itemTooltip = ItemTooltips.tooltips[tooltipType];

	memset(buf, 0, sizeof(buf));

	if ( tooltipType.find("tooltip_armor") != std::string::npos 
		|| tooltipType.find("tooltip_offhand") != std::string::npos
		|| tooltipType.find("tooltip_amulet") != std::string::npos
		|| tooltipType.find("tooltip_ring") != std::string::npos )
	{
		if ( detailTag.compare("armor_base_ac") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("AC") ? items[item.type].attributes["AC"] : 0	);
		}
		else if ( detailTag.compare("armor_shield_bonus") == 0 )
		{
			bool excludeSkill = compendiumTooltipIntro;
			if ( tooltipType.find("tooltip_offhand") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					stats[player]->getActiveShieldBonus(false, excludeSkill, &item),
					getItemProficiencyName(PRO_SHIELD).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					stats[player]->getPassiveShieldBonus(false, excludeSkill),
					getItemProficiencyName(PRO_SHIELD).c_str(),
					stats[player]->getActiveShieldBonus(false, excludeSkill, &item),
					getItemProficiencyName(PRO_SHIELD).c_str());
			}
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude, 
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("shield_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_SHIELD) / 10);
			int durabilityBonus = skillLVL * 10;
			if ( itemCategory(&item) == ARMOR )
			{
				durabilityBonus *= 2;
			}
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("shield_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("knuckle_skill_modifier") == 0 )
		{
			int atk = compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(PRO_UNARMED) / 20)); // 0 - 5
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("knuckle_knockback_modifier") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				items[item.type].hasAttribute("KNOCKBACK") ? items[item.type].attributes["KNOCKBACK"] : 0);
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), (!compendiumTooltipIntro && stats[player]) ? statGetSTR(stats[player], players[player]->entity) : 0);
		}
		else if ( detailTag.compare("ring_unarmed_atk") == 0 )
		{
			int atk = 1 + (shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_UNARMED) / 20);
			int durabilityBonus = skillLVL * 20;
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("weapon_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("equipment_fragile_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("FRAGILE") ? -items[item.type].attributes["FRAGILE"] : 0);
		}
		else if ( detailTag.compare("equipment_stat_bonus") == 0 )
		{
			std::vector<std::string> statNames = { "STR", "DEX", "CON", "INT", "PER", "CHR" };
			int baseStatBonus = 0;
			int beatitudeStatBonus = 0;
			bool found = false;
			for ( auto& stat : statNames )
			{
				if ( items[item.type].hasAttribute(stat) )
				{
					found = true;
					baseStatBonus = items[item.type].attributes[stat];
					beatitudeStatBonus = getStatAttributeBonusFromItem(player, item, stat) - baseStatBonus;

					snprintf(buf, sizeof(buf), str.c_str(), baseStatBonus, stat.c_str(),
						beatitudeStatBonus, stat.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
					break;
				}
			}
			if ( !found )
			{
				return;
			}
		}
		else if ( detailTag.find("EFF_") != std::string::npos )
		{
			if ( adjectives["equipment_effects_attributes_text"].find(detailTag)
				== adjectives["equipment_effects_attributes_text"].end() )
			{
				return;
			}

			if ( detailTag == "EFF_FEATHER" )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					items[item.type].hasAttribute(detailTag) ? items[item.type].attributes["EFF_FEATHER"] : 0,
					getItemEquipmentEffectsForAttributesText(detailTag).c_str());
			}
			else if ( detailTag == "EFF_WARNING" )
			{
				int beatitude = shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude;
				int radius = std::max(3, 11 + 5 * beatitude);
				snprintf(buf, sizeof(buf), str.c_str(), radius, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else if ( detailTag == "EFF_HOOD_WHISPERS" )
			{
				//int val = (stats[player]->getModifiedProficiency(PRO_STEALTH) / 20 + 2) * 2; // backstab dmg
				//if ( skillCapstoneUnlocked(player, PRO_STEALTH) )
				//{
				//	val *= 2;
				//}

				//real_t equipmentModifier = 0.0;
				//real_t bonusModifier = 1.0;
				//if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
				//{
				//	equipmentModifier += (std::min(50 + (10 * abs(item.beatitude)), 100)) / 100.0;
				//}
				//else
				//{
				//	equipmentModifier = 0.5;
				//	bonusModifier = 0.5;
				//}
				//val = ((val * equipmentModifier) * bonusModifier);

				std::string skillName = "";
				for ( auto s : Player::SkillSheet_t::skillSheetData.skillEntries )
				{
					if ( s.skillId == PRO_STEALTH )
					{
						skillName = s.name;
						break;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), skillName.c_str());
			}
			else if ( detailTag == "EFF_SILKEN_BOW" )
			{
				int baseBonus = 5;
				int chanceBonus = 0;
				if ( item.type == HAT_SILKEN_BOW )
				{
					if ( item.beatitude >= 0 || shouldInvertEquipmentBeatitude(stats[player]) )
					{
						baseBonus = 3 + 1 * std::min(5, abs(item.beatitude));
						if ( !compendiumTooltipIntro )
						{
							chanceBonus += std::min(10, (stats[player]->getModifiedProficiency(PRO_LEADERSHIP)
								+ std::max(0, 3 * statGetCHR(stats[player], players[player]->entity))) / 10);
						}

						if ( baseBonus + chanceBonus > 15 )
						{
							chanceBonus -= (baseBonus + chanceBonus) - 15;
						}
					}
					else
					{
						baseBonus = 1;
					}
				}

				std::string skillName = "";
				for ( auto s : Player::SkillSheet_t::skillSheetData.skillEntries )
				{
					if ( s.skillId == PRO_LEADERSHIP )
					{
						skillName = s.name;
						break;
					}
				}
				snprintf(buf, sizeof(buf), str.c_str(), baseBonus,
					chanceBonus, skillName.c_str(), getItemStatShortName("CHR").c_str());
			}
			else
			{
				return;
			}
		}
		else if ( detailTag.compare("equipment_on_cursed_sideeffect") == 0
			|| detailTag.compare("ring_on_cursed_sideeffect") == 0 
			|| detailTag.compare("armor_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("artifact_armor_on_degraded") == 0 )
		{
			int statusModifier = std::max(DECREPIT, item.status) - 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_gem") != std::string::npos )
	{
		int proficiency = PRO_RANGED;
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("thrown_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), (!compendiumTooltipIntro && stats[player]) ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0);
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 10);
			snprintf(buf, sizeof(buf), str.c_str(), skillLVL,
				getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag == "EFF_FEATHER" )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute(detailTag) ? items[item.type].attributes["EFF_FEATHER"] : 0,
				getItemEquipmentEffectsForAttributesText(detailTag).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_tool_beartrap") != std::string::npos )
	{
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("beartrap_degrade_on_use_cursed") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 100, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("beartrap_degrade_on_use") == 0 )
		{
			int chanceDegrade = 0;
			switch ( item.status )
			{
				case SERVICABLE:
					chanceDegrade = 4;
					break;
				case WORN:
					chanceDegrade = 10;
					break;
				case DECREPIT:
					chanceDegrade = 25;
					break;
				default:
					break;
			}
			snprintf(buf, sizeof(buf), str.c_str(),	chanceDegrade, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				item.beatitude * 3,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("on_degraded") == 0 )
		{
			int statusModifier = item.status * 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_thrown") != std::string::npos
		|| tooltipType.find("tooltip_boomerang") != std::string::npos )
	{
		int proficiency = PRO_RANGED;
		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude,
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("thrown_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), (!compendiumTooltipIntro && stats[player]) ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0);
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 20);
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * thrownDamageSkillMultipliers[std::min(skillLVL, 5)] - 100),
				getItemProficiencyName(proficiency).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_mace") != std::string::npos
		|| tooltipType.find("tooltip_axe") != std::string::npos
		|| tooltipType.find("tooltip_sword") != std::string::npos
		|| tooltipType.find("tooltip_polearm") != std::string::npos
		|| tooltipType.find("tooltip_whip") != std::string::npos
		|| tooltipType.compare("tooltip_tool_pickaxe") == 0
		|| tooltipType.find("tooltip_ranged") != std::string::npos
		|| tooltipType.find("tooltip_quiver") != std::string::npos )
	{
		int proficiency = PRO_SWORD;
		if ( tooltipType.find("tooltip_mace") != std::string::npos )
		{
			proficiency = PRO_MACE;
		}
		else if ( tooltipType.find("tooltip_axe") != std::string::npos )
		{
			proficiency = PRO_AXE;
		}
		else if ( tooltipType.find("tooltip_sword") != std::string::npos )
		{
			proficiency = PRO_SWORD;
		}
		else if ( tooltipType.find("tooltip_polearm") != std::string::npos )
		{
			proficiency = PRO_POLEARM;
		}
		else if ( tooltipType.find("tooltip_ranged") != std::string::npos
			|| tooltipType.compare("tooltip_whip") == 0 )
		{
			proficiency = PRO_RANGED;
		}

		if ( detailTag.compare("weapon_base_atk") == 0 )
		{
			if ( proficiency == PRO_AXE )
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] + 1 : 0);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					items[item.type].hasAttribute("ATK") ? items[item.type].attributes["ATK"] : 0);
			}
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude, 
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("on_degraded") == 0 )
		{
			int statusModifier = item.status - 3;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("artifact_weapon_on_degraded") == 0 )
		{
			int statusModifier = (item.status - 3) * 2;
			snprintf(buf, sizeof(buf), str.c_str(), statusModifier, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("weapon_skill_modifier") == 0 )
		{
			if ( proficiency == PRO_POLEARM )
			{
				//int weaponEffectiveness = -8 + (stats[player]->PROFICIENCIES[proficiency] / 3); // -8% to +25%
				int weaponEffectiveness = -25 + compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(proficiency) / 2)); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
			else
			{
				int weaponEffectiveness = -25 + compendiumTooltipIntro ? 0 : ((stats[player]->getModifiedProficiency(proficiency) / 2)); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
		}
		else if ( detailTag.compare("weapon_skill_modifier_range") == 0 )
		{
			real_t variance = 20;
			real_t baseSkillModifier = 50.0; // 40-60 base
			ItemType itemType = item.type;
			Entity::setMeleeDamageSkillModifiers(compendiumTooltipIntro ? nullptr : players[player]->entity, 
				nullptr, proficiency, baseSkillModifier, variance, &itemType);
			real_t lowest = baseSkillModifier - (variance / 2) + (compendiumTooltipIntro ? 0 : stats[player]->getModifiedProficiency(proficiency) / 2.0);
			lowest = std::min(100.0, std::max(0.0, lowest));
			real_t highest = std::min(100.0, lowest + variance);

			snprintf(buf, sizeof(buf), str.c_str(), (int)lowest, (int)highest, getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			if ( item.type == TOOL_WHIP )
			{
				int atk = (stats[player] ? statGetDEX(stats[player], players[player]->entity) : 0);
				atk += (stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
				atk = std::min(atk / 2, atk);
				if ( compendiumTooltipIntro )
				{
					atk = 0;
				}
				snprintf(buf, sizeof(buf), str.c_str(), atk);
			}
			else if ( proficiency == PRO_RANGED )
			{
				snprintf(buf, sizeof(buf), str.c_str(), (!compendiumTooltipIntro && stats[player]) ? statGetDEX(stats[player], players[player]->entity) : 0);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), (!compendiumTooltipIntro && stats[player]) ? statGetSTR(stats[player], players[player]->entity) : 0);
			}
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(proficiency) / 20);
			int durabilityBonus = skillLVL * 20;
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_bonus_exp") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				items[item.type].hasAttribute("BONUS_SKILL_EXP") ? items[item.type].attributes["BONUS_SKILL_EXP"] : 0,
				getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("equipment_fragile_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("FRAGILE") ? -items[item.type].attributes["FRAGILE"] : 0);
		}
		else if ( detailTag.compare("weapon_ranged_armor_pierce") == 0 )
		{
			int statChance = std::min(std::max((stats[player] ? statGetPER(stats[player], players[player]->entity) : 0) / 2, 0), 50); // 0 to 50 value.
			if ( compendiumTooltipIntro )
			{
				statChance = 0;
			}
			statChance += (items[item.type].hasAttribute("ARMOR_PIERCE") ? items[item.type].attributes["ARMOR_PIERCE"] : 0);
			snprintf(buf, sizeof(buf), str.c_str(), statChance);
		}
		else if ( detailTag.compare("weapon_ranged_quiver_augment") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemSlotName(ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD).c_str());
		}
		else if ( detailTag.compare("weapon_ranged_rate_of_fire") == 0 )
		{
			int rof = (items[item.type].hasAttribute("RATE_OF_FIRE") ? items[item.type].attributes["RATE_OF_FIRE"] : 0);
			if ( rof > 0 )
			{
				rof -= 100;
				rof *= -1;
				snprintf(buf, sizeof(buf), str.c_str(), rof);
			}
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_potion") != std::string::npos )
	{
		if ( detailTag.compare("default") == 0 || detailTag.compare("potion_additional_effects") == 0 )
		{
			return;
		}
		else if ( detailTag.compare("potion_polymorph_duration") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum(players[player]->entity, stats[player]) / (60 * TICKS_PER_SECOND),
				item.potionGetEffectDurationMaximum(players[player]->entity, stats[player]) / (60 * TICKS_PER_SECOND) );
		}
		else if ( detailTag.compare("potion_restoremagic_bonus") == 0 )
		{
			if ( stats[player] && statGetINT(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), std::min(30, 2 * std::max(0, statGetINT(stats[player], players[player]->entity))));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_healing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 2 * std::max(0, statGetCON(stats[player], players[player]->entity)));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_extrahealing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 && !compendiumTooltipIntro )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 4 * std::max(0, statGetCON(stats[player], players[player]->entity)));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_on_blessed") == 0 )
		{
			if ( item.type == POTION_CUREAILMENT )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					item.potionGetEffectDurationRandom(players[player]->entity, stats[player]) / TICKS_PER_SECOND, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else if ( item.type == POTION_WATER )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 20 * item.beatitude, getItemBeatitudeAdjective(item.beatitude).c_str());
			}
			else
			{
				return;
			}
		}
		else if ( detailTag.compare("potion_on_cursed") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("alchemy_details") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemPotionAlchemyAdjective(player, item.type).c_str());
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.beatitude, getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("potion_damage") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), BASE_THROWN_DAMAGE);
		}
		else if ( detailTag.compare("potion_multiplier") == 0 )
		{
			int skillLVL = compendiumTooltipIntro ? 0 : (stats[player]->getModifiedProficiency(PRO_ALCHEMY) / 20);
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * potionDamageSkillMultipliers[std::min(skillLVL, 5)] - 100), 
				getItemPotionHarmAllyAdjective(item).c_str());
		}
	}
	else if ( tooltipType.compare("tooltip_tool_lockpick") == 0 )
	{
		if ( detailTag.compare("lockpick_chestsdoors_unlock_chance") == 0 )
		{
			int chance = stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 2; // lockpick chests/doors
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) == SKILL_LEVEL_LEGENDARY )
			{
				chance = 100;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_chests_scrap_chance") == 0 )
		{
			int chance = std::min(100, stats[player]->getModifiedProficiency(PRO_LOCKPICKING) + 50);
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 10)))); // disarm arrow traps
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_BASIC )
			{
				chance = 0;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_automaton_disarm") == 0 )
		{
			int chance = 0;
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) >= SKILL_LEVEL_EXPERT )
			{
				chance = 100; // lockpick automatons
			}
			else
			{
				chance = (100 - 100 / (static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 20 + 1))); // lockpick automatons
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_skeletonkey") == 0 )
	{
		Sint32 PER = statGetPER(
			compendiumTooltipIntro ? nullptr : stats[player], 
			compendiumTooltipIntro ? nullptr : players[player]->entity);
		if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->getModifiedProficiency(PRO_LOCKPICKING) / 10)))); // disarm arrow traps
			if ( stats[player]->getModifiedProficiency(PRO_LOCKPICKING) < SKILL_LEVEL_BASIC )
			{
				chance = 0;
			}
			if ( compendiumTooltipIntro )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_decoy") == 0 )
	{
		if ( detailTag.compare("tool_decoy_range") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), decoyBoxRange);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_food") != std::string::npos )
	{
		if ( detailTag.compare("food_puke_chance") == 0 )
		{
			int pukeChance = item.foodGetPukeChance(nullptr);
			real_t chance = 100;
			if ( item.status == BROKEN )
			{
				chance = 100.0;
			}
			else if ( pukeChance == 100 )
			{
				chance = 0.0;
			}
			else
			{
				chance *= (1.0 / std::max(1, pukeChance));
			}
			snprintf(buf, sizeof(buf), str.c_str(), 
				static_cast<int>(chance), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("food_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_spellbook") != std::string::npos )
	{
		if ( detailTag.compare("spellbook_cast_bonus") == 0 )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( !spell ) { return; }

			int intBonus = (statGetINT(
				compendiumTooltipIntro ? nullptr : stats[player], 
				compendiumTooltipIntro ? nullptr : players[player]->entity) * 0.5);
			real_t mult = ((items[item.type].attributes["SPELLBOOK_CAST_BONUS"]) / 100.0);
			intBonus *= mult;
			int beatitudeBonus = (mult * getSpellbookBonusPercent(
				compendiumTooltipIntro ? nullptr : players[player]->entity, 
				compendiumTooltipIntro ? nullptr : stats[player], &item)) - intBonus;

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
			}
			else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_CURE)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["duration"];
			}
			else if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["duration"];
			}

			snprintf(buf, sizeof(buf), str.c_str(),
				intBonus, damageOrHealing.c_str(), getItemStatShortName("INT").c_str(),
				beatitudeBonus, damageOrHealing.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("spellbook_cast_success") == 0 )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( !spell ) { return; }

			int spellcastingAbility = getSpellcastingAbilityFromUsingSpellbook(spell, players[player]->entity, stats[player]);
			int chance = ((10 - (spellcastingAbility / 10)) * 20 / 3.0); // 33% after rolling to fizzle, 66% success
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spellbook_extramana_chance") == 0 )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( !spell ) { return; }

			int spellcastingAbility = getSpellcastingAbilityFromUsingSpellbook(spell, players[player]->entity, stats[player]);
			int chance = (10 - (spellcastingAbility / 10)) * 10;
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spellbook_magic_requirement") == 0 )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( !spell ) { return; }

			int skillLVL = std::min(100, stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity));
			if ( !playerLearnedSpellbook(player, &item) && (spell && spell->difficulty > skillLVL) )
			{
				str.insert((size_t)0, 1, '^'); // red line character
			}

			if ( spell )
			{
				snprintf(buf, sizeof(buf), str.c_str(), spell->difficulty, getProficiencyLevelName(spell->difficulty).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0, getProficiencyLevelName(0).c_str());
			}
		}
		else if ( detailTag.compare("spellbook_magic_current") == 0 )
		{
			spell_t* spell = getSpellFromID(getSpellIDFromSpellbook(item.type));
			if ( !spell ) { return; }

			int skillLVL = std::min(100, stats[player]->getModifiedProficiency(PRO_MAGIC) + statGetINT(stats[player], players[player]->entity));
			if ( !playerLearnedSpellbook(player, &item) && (spell && spell->difficulty > skillLVL) )
			{
				str.insert((size_t)0, 1, '^'); // red line character
			}
			Sint32 INT = stats[player] ? statGetINT(stats[player], players[player]->entity) : 0;
			Sint32 skill = stats[player] ? stats[player]->getModifiedProficiency(PRO_MAGIC) : 0;
			Sint32 total = std::min(SKILL_LEVEL_LEGENDARY, INT + skill);
			snprintf(buf, sizeof(buf), str.c_str(), INT + skill, getProficiencyLevelName(INT + skill).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_spell_item") == 0 )
	{
		if ( detailTag.compare("spell_damage_bonus") == 0 )
		{
			spell_t* spell = getSpellFromItem(player, &item, false);
			if ( !spell ) { return; }

			//int totalDamage = getSpellDamageOrHealAmount(player, spell, nullptr);
			Sint32 oldINT = stats[player]->INT;
			stats[player]->INT = 0;

			int baseDamage = getSpellDamageOrHealAmount(-1, spell, nullptr, compendiumTooltipIntro);

			real_t bonusEquipPercent = 100.0 * getBonusFromCasterOfSpellElement(players[player]->entity, stats[player], nullptr, spell ? spell->ID : SPELL_NONE);

			stats[player]->INT = oldINT;

			real_t bonusINTPercent = 100.0 * getBonusFromCasterOfSpellElement(players[player]->entity, stats[player], nullptr, spell ? spell->ID : SPELL_NONE);
			bonusINTPercent -= bonusEquipPercent;

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
			}
			snprintf(buf, sizeof(buf), str.c_str(), damageOrHealing.c_str(), baseDamage, damageOrHealing.c_str(), 
				bonusINTPercent, damageOrHealing.c_str(), getItemStatShortName("INT").c_str(), bonusEquipPercent, damageOrHealing.c_str());
		}
		else if ( detailTag.compare("spell_cast_success") == 0 )
		{
			int spellcastingAbility = std::min(std::max(0, stats[player]->getModifiedProficiency(PRO_SPELLCASTING)
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = ((10 - (spellcastingAbility / 10)) * 20 / 3.0); // 33% after rolling to fizzle, 66% success
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spell_extramana_chance") == 0 )
		{
			int spellcastingAbility = std::min(std::max(0, stats[player]->getModifiedProficiency(PRO_SPELLCASTING)
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = (10 - (spellcastingAbility / 10)) * 10;
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("attribute_spell_charm") == 0 )
		{
			int leaderChance = ((statGetCHR(stats[player], players[player]->entity) + 
				stats[player]->getModifiedProficiency(PRO_LEADERSHIP)) / 20) * 5;
			int intChance = (statGetINT(stats[player], players[player]->entity) * 2);
			if ( compendiumTooltipIntro )
			{
				leaderChance = 0;
				intChance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), intChance, leaderChance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_magicstaff") == 0 )
	{
		if ( detailTag.compare("magicstaff_charm_degrade_chance") == 0 )
		{
			int degradeChance = 100;
			if ( item.status > WORN )
			{
				degradeChance = 33;
			}
			snprintf(buf, sizeof(buf), str.c_str(), degradeChance, getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("magicstaff_degrade_chance") == 0 )
		{
			int degradeChance = 33;
			snprintf(buf, sizeof(buf), str.c_str(), degradeChance);
		}
		else if ( detailTag.compare("attribute_spell_charm") == 0 )
		{
			int leaderChance = ((statGetCHR(stats[player], players[player]->entity) +
				stats[player]->getModifiedProficiency(PRO_LEADERSHIP)) / 20) * 10;
			if ( compendiumTooltipIntro )
			{
				leaderChance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), leaderChance);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_scroll") == 0 )
	{
		if ( detailTag.compare("scroll_on_cursed_sideeffect") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_tool_bomb") != std::string::npos )
	{
		if ( detailTag.compare("tool_bomb_base_atk") == 0 )
		{
			int baseDmg = (items[item.type].hasAttribute("BOMB_ATK") ? items[item.type].attributes["BOMB_ATK"] : 0);
			int baseSpellDamage = 0;
			if ( item.type == TOOL_FREEZE_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr, compendiumTooltipIntro);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr, compendiumTooltipIntro);
			}
			snprintf(buf, sizeof(buf), str.c_str(), baseDmg + baseSpellDamage);
		}
		else if ( detailTag.compare("tool_bomb_per_atk") == 0 )
		{
			int perMult = (items[item.type].hasAttribute("BOMB_DMG_PER_MULT") ? items[item.type].attributes["BOMB_DMG_PER_MULT"] : 0);
			int perDmg = std::max(0, statGetPER(
				compendiumTooltipIntro ? nullptr : stats[player], 
				compendiumTooltipIntro ? nullptr : players[player]->entity)) * perMult / 100.0;
			snprintf(buf, sizeof(buf), str.c_str(), perDmg, perMult);
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_tool_spellbot") == 0 || tooltipType.compare("tooltip_tool_sentrybot") == 0
		|| tooltipType.compare("tooltip_tool_gyrobot") == 0 )
	{
		if ( detailTag.compare("tinkerbot_status_bonus") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else if ( detailTag.compare("spellbot_rate_of_fire") == 0 )
		{
			real_t bow = 2.0;
			if ( item.type == TOOL_SPELLBOT )
			{
				if ( item.status >= EXCELLENT )
				{
					bow = 1.2;
				}
				else if ( item.status >= SERVICABLE )
				{
					bow = 1.5;
				}
				else if ( item.status >= WORN )
				{
					bow = 1.8;
				}
				else
				{
					bow = 2;
				}
			}
			real_t rof = fabs((1.0 - (2 / bow)) * 100);
			snprintf(buf, sizeof(buf), str.c_str(), (int)rof);
		}
		else if ( detailTag.compare("tinkerbot_turn_rate") == 0 )
		{
			real_t ratio = 64.0;
			if ( item.status >= EXCELLENT )
			{
				ratio = 2.0;
			}
			else if ( item.status >= SERVICABLE )
			{
				ratio = 4.0;
			}
			else if ( item.status >= WORN )
			{
				ratio = 16.0;
			}
			else
			{
				ratio = 64.0;
			}
			real_t turnRate = (64.0 / ratio) - 1.0;
			snprintf(buf, sizeof(buf), str.c_str(), (int)turnRate);
		}
		else if ( detailTag.compare("gyrobot_info_interact") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemStatusAdjective(item.type, item.status).c_str());
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}
	str = buf;
#endif
}

void ItemTooltips_t::stripOutHighlightBracketText(std::string& str, std::string& bracketText)
{
	for ( auto it = str.begin(); it != str.end(); ++it )
	{
		bool foundBracket = false;
		if ( *it == '[' )
		{
			foundBracket = true;
		}
		else if ( *it == '+' )
		{
			auto next_it = std::next(it);
			if ( next_it != str.end() && *next_it == '[' )
			{
				foundBracket = true;
				bracketText += *it;
				// don't edit original string ---- *it = ' ';
				it = next_it;
			}
		}

		if ( foundBracket )
		{
			while ( *it != '\0' && *it != '\n' )
			{
				bracketText += *it;
				if ( *it == ']' )
				{
					// don't edit original string ---- *it = ' ';
					break;
				}
				// don't edit original string ---- *it = ' ';
				it = std::next(it);
			}
		}
		else if ( it != str.end() && *it != '\n' )
		{
			bracketText += ' ';
		}
		if ( *it == '\n' )
		{
			bracketText += '\n';
		}
	}
}

bool charIsWordSeparator(char c)
{
	if ( c == ' ' || c == '\n' || c == '\r' || c == '\0' )
	{
		return true;
	}
	return false;
}

void ItemTooltips_t::getWordIndexesItemDetails(void* field, std::string& str, std::string& highlightValues, std::string& positiveValues, std::string& negativeValues,
	std::map<int, Uint32>& highlightIndexes, std::map<int, Uint32>& positiveIndexes, std::map<int, Uint32>& negativeIndexes, ItemTooltip_t& tooltip)
{
	positiveIndexes.clear();
	negativeIndexes.clear();
	highlightIndexes.clear();
	((Field*)field)->clearWordsToHighlight();
	int wordIndex = 0;
	bool prevCharWasWordSeparator = false;
	int numLines = 0;
	for ( size_t c = 0; c < str.size(); ++c )
	{
		if ( str[c] == '\n' )
		{
			wordIndex = -1;
			++numLines;
			prevCharWasWordSeparator = true;
			continue;
		}

		if ( prevCharWasWordSeparator && !charIsWordSeparator(str[c]) )
		{
			++wordIndex;
			prevCharWasWordSeparator = false;
			if ( !(c + 1 < str.size() && charIsWordSeparator(str[c+1])) )
			{
				continue;
			}
		}

		if ( charIsWordSeparator(str[c]) )
		{
			prevCharWasWordSeparator = true;
		}
		else
		{
			prevCharWasWordSeparator = false;
			if ( c < positiveValues.size() )
			{
				if ( positiveValues[c] == str[c] )
				{
					positiveIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
			if ( c < negativeValues.size() )
			{
				if ( negativeValues[c] == str[c] )
				{
					negativeIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
			if ( c < highlightValues.size() )
			{
				if ( highlightValues[c] == str[c] )
				{
					highlightIndexes[wordIndex + numLines * Field::TEXT_HIGHLIGHT_WORDS_PER_LINE] = 0;
				}
			}
		}
	}

	for ( auto& p : positiveIndexes )
	{
		Uint32 color = tooltip.positiveTextColor;
		((Field*)field)->addWordToHighlight(p.first, color);
		//messagePlayer(0, "Positives: %d", p.first);
	}
	for ( auto& n : negativeIndexes )
	{
		Uint32 color = tooltip.negativeTextColor;
		((Field*)field)->addWordToHighlight(n.first, color);
		//messagePlayer(0, "Negatives: %d", n.first);
	}
	for ( auto& h : highlightIndexes )
	{
		Uint32 color = tooltip.statusEffectTextColor;
		((Field*)field)->addWordToHighlight(h.first, color);
		//messagePlayer(0, "Highlights: %d", h.first);
	}
}

void ItemTooltips_t::stripOutPositiveNegativeItemDetails(std::string& str, std::string& positiveValues, std::string& negativeValues)
{
	size_t index = 0;
	for ( auto it = str.begin(); it != str.end(); )
	{
		int sign = 0;
		if ( *it == '+' )
		{
			sign = 1;
		}
		else if ( *it == '-' )
		{
			sign = -1;
		}

		if ( sign != 0 )
		{
			if ( std::next(it) != str.end() )
			{
				char peekCharacter = *(std::next(it));
				if ( !(peekCharacter >= '0' && peekCharacter <= '9') )
				{
					sign = 0; // don't highlight +text, only +0 numbers
				}
			}
		}

		if ( sign != 0 )
		{
			bool addSpace = false;
			while ( *it != '\0' && *it != ' ' && *it != '\n' )
			{
				if ( *it == '*' )
				{
					// replace with bullet symbol
					*it = '\x1E';
				}
				if ( sign > 0 )
				{
					positiveValues += *it;
					negativeValues += ' ';
				}
				else
				{
					negativeValues += *it;
					positiveValues += ' ';
				}
				// don't edit original string ---- *it = ' ';
				it = std::next(it);
				addSpace = true;

				if ( it == str.end() )
				{
					break;
				}
			}

			if ( addSpace )
			{
				positiveValues += ' ';
				negativeValues += ' ';
			}
		}
		else
		{
			bool addSpace = true;
			if ( *it == '(' )
			{
				if ( str.find("(?)", std::distance(str.begin(), it)) == std::distance(str.begin(), it) )
				{
					addSpace = false;
					positiveValues += ' ';
					negativeValues += "(?)";
					// don't edit original string ---- *it = ' ';
					for ( size_t i = 0; i < strlen("(?)") - 1; ++i )
					{
						positiveValues += ' ';
						it = std::next(it);
						// don't edit original string ---- *it = ' ';
					}
				}
			}
			else if ( *it == '[' )
			{
				// look for matching brace.
				while ( it != str.end() && *it != ']' && *it != ' ' && *it != '\0' && *it != ' ' && *it != '\n' )
				{
					if ( *it == '*' )
					{
						// replace with bullet symbol
						*it = '\x1E';
					}
					positiveValues += ' ';
					negativeValues += ' ';
					it = std::next(it);
				}
				if ( it != str.end() && (*it == '\0' || *it == '\n') )
				{
					addSpace = false;
				}
			}
			else if ( *it == '^' )
			{
				// cursed line
				it = str.erase(it); // skip the '^'
				while ( it != str.end() && *it != '\0' && *it != '\n' )
				{
					if ( *it == '*' )
					{
						// replace with bullet symbol
						*it = '\x1E';
					}
					positiveValues += ' ';
					negativeValues += *it;
					// don't edit original string ---- *it = ' ';
					it = std::next(it);
				}
				if ( it != str.end() && (*it == '\0' || *it == '\n') )
				{
					addSpace = false;
				}
			}
			else if ( *it == '*' )
			{
				// replace with bullet symbol
				*it = '\x1E';
			}
			else if ( *it == '\0' || *it == '\n' )
			{
				addSpace = false;
			}

			if ( addSpace )
			{
				positiveValues += ' ';
				negativeValues += ' ';
			}
		}

		if ( it != str.end() )
		{
			if ( *it == '\n' )
			{
				positiveValues += '\n';
				negativeValues += '\n';
			}
		}
		else
		{
			break;
		}

		++it;
		index = std::distance(str.begin(), it);
	}
}
#endif // !EDITOR

Uint32 StatueManager_t::statueId = 0;
int StatueManager_t::processStatueExport()
{
	if ( !exportActive )
	{
		return 0;
	}

	Entity* player = uidToEntity(editingPlayerUid);

	if ( exportRotations >= 4 || !player )
	{
		if ( player ) // save the file.
		{
			std::string outputPath = PHYSFS_getRealDir("/data/statues");
			outputPath.append(PHYSFS_getDirSeparator());
			std::string fileName = "data/statues/" + exportFileName;
			outputPath.append(fileName.c_str());

			exportRotations = 0;
			exportFileName = "";
			exportActive = false;

			File* fp = FileIO::open(outputPath.c_str(), "wb");
			if ( !fp )
			{
				return 0;
			}
			rapidjson::StringBuffer os;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
			exportDocument.Accept(writer);
			fp->write(os.GetString(), sizeof(char), os.GetSize());
			FileIO::close(fp);
			return 2; // success
		}
		else
		{
			exportRotations = 0;
			exportFileName = "";
			exportActive = false;
			return 0;
		}
	}

	bool newDocument = false;

	if ( exportFileName == "" ) // find a new filename
	{
		int filenum = 0;
		std::string testPath = "/data/statues/statue" + std::to_string(filenum) + ".json";
		while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
		{
			++filenum;
			testPath = "/data/statues/statue" + std::to_string(filenum) + ".json";
		}
		exportFileName = "statue" + std::to_string(filenum) + ".json";
		newDocument = true;
	}

	if ( newDocument )
	{
		if ( exportDocument.IsObject() )
		{
			exportDocument.RemoveAllMembers();
		}
		exportDocument.SetObject();
		CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(exportDocument, "statue_id", rapidjson::Value(local_rng.rand()));
		CustomHelpers::addMemberToRoot(exportDocument, "height_offset", rapidjson::Value(0));
		rapidjson::Value limbsObject(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(exportDocument, "limbs", limbsObject);
	}

	rapidjson::Value limbsArray(rapidjson::kArrayType);

	std::vector<Entity*> allLimbs;
	allLimbs.push_back(player);

	for ( auto& bodypart : player->bodyparts )
	{
		allLimbs.push_back(bodypart);
	}

	int index = 0;
	for ( auto& limb : allLimbs )
	{
		if ( limb->flags[INVISIBLE] )
		{
			continue;
		}
		rapidjson::Value limbsObj(rapidjson::kObjectType);

		if ( index != 0 )
		{
			limbsObj.AddMember("x", rapidjson::Value(player->x - limb->x), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(player->y - limb->y), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		else
		{
			limbsObj.AddMember("x", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		limbsObj.AddMember("pitch", rapidjson::Value(limb->pitch), exportDocument.GetAllocator());
		limbsObj.AddMember("roll", rapidjson::Value(limb->roll), exportDocument.GetAllocator());
		limbsObj.AddMember("yaw", rapidjson::Value(limb->yaw), exportDocument.GetAllocator());
		limbsObj.AddMember("focalx", rapidjson::Value(limb->focalx), exportDocument.GetAllocator());
		limbsObj.AddMember("focaly", rapidjson::Value(limb->focaly), exportDocument.GetAllocator());
		limbsObj.AddMember("focalz", rapidjson::Value(limb->focalz), exportDocument.GetAllocator());
		limbsObj.AddMember("sprite", rapidjson::Value(limb->sprite), exportDocument.GetAllocator());
		limbsArray.PushBack(limbsObj, exportDocument.GetAllocator());

		++index;
	}

	CustomHelpers::addMemberToSubkey(exportDocument, "limbs", directionKeys[exportRotations], limbsArray);
	++exportRotations;
	return 1;
}

void StatueManager_t::resetStatueEditor()
{
	if ( editingPlayerUid != 0 )
	{
		client_disconnected[1] = true;
	}
	editingPlayerUid = 0;
	StatueManager.activeEditing = false;
}

void StatueManager_t::refreshAllStatues()
{
#ifndef EDITOR
	node_t* nextnode = nullptr;
	for ( node_t* node = map.entities->first; node; node = nextnode )
	{
		nextnode = node->next;
		auto entity = (Entity*)node->element;
		if ( entity->behavior == &actStatue )
		{
			entity->statueInit = 0;
			node_t* nextnode2 = nullptr;
			for ( node_t* node2 = entity->children.first; node2; node2 = nextnode2 )
			{
				nextnode2 = node2->next;
				auto entity2 = (Entity*)node2->element;
				list_RemoveNode(entity2->mynode);
				list_RemoveNode(node2);
			}
		}
	}
#endif // !EDITOR
}

void StatueManager_t::readAllStatues()
{
	std::string baseDir = "data/statues";
	auto files = physfsGetFileNamesInDirectory(baseDir.c_str());
	for ( auto file : files )
	{
		std::string checkFile = baseDir + '/' + file;
		PHYSFS_Stat stat;
		if ( PHYSFS_stat(checkFile.c_str(), &stat) == 0 ) { continue; }

		if ( stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
		{
			auto files2 = physfsGetFileNamesInDirectory(checkFile.c_str());
			for ( auto file2 : files2 )
			{
				std::string checkFile2 = checkFile + '/' + file2;
				if ( PHYSFS_stat(checkFile2.c_str(), &stat) == 0 ) { continue; }

				if ( stat.filetype != PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
				{
					readStatueFromFile(0, checkFile2);
				}
			}
		}
		else
		{
			readStatueFromFile(0, checkFile);
		}
	}
}

void StatueManager_t::readStatueFromFile(int index, std::string filename)
{
	std::string fileName = "/data/statues/statue" + std::to_string(index) + ".json";
	if ( filename != "" )
	{
		fileName = filename;
	}
	if ( PHYSFS_getRealDir(fileName.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(fileName.c_str());
		if (!inputPath.empty()) {
			inputPath.append(PHYSFS_getDirSeparator());
		}
		inputPath.append(fileName);

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("limbs") || !d.HasMember("statue_id") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}
		int version = d["version"].GetInt();
		Uint32 statueId = d["statue_id"].GetUint();
		auto findStatue = allStatues.find(statueId);
		if ( findStatue != allStatues.end() )
		{
			allStatues.erase(findStatue);
		}
		allStatues.insert(std::make_pair(statueId, Statue_t()));
		for ( rapidjson::Value::ConstMemberIterator limb_itr = d["limbs"].MemberBegin(); limb_itr != d["limbs"].MemberEnd(); ++limb_itr )
		{
			auto& statue = allStatues[statueId];
			if ( d.HasMember("height_offset") )
			{
				statue.heightOffset = d["height_offset"].GetDouble();
			}
			for ( rapidjson::Value::ConstValueIterator dir_itr = limb_itr->value.Begin(); dir_itr != limb_itr->value.End(); ++dir_itr )
			{
				const rapidjson::Value& attributes = *dir_itr;
				std::string direction = limb_itr->name.GetString();
				auto& limbVector = statue.limbs[direction];
				limbVector.push_back(Statue_t::StatueLimb_t());
				auto& limb = limbVector[limbVector.size() - 1];
				limb.x = attributes["x"].GetDouble();
				limb.y = attributes["y"].GetDouble();
				limb.z = attributes["z"].GetDouble();
				limb.focalx = attributes["focalx"].GetDouble();
				limb.focaly = attributes["focaly"].GetDouble();
				limb.focalz = attributes["focalz"].GetDouble();
				limb.pitch = attributes["pitch"].GetDouble();
				limb.roll = attributes["roll"].GetDouble();
				limb.yaw = attributes["yaw"].GetDouble();
				limb.sprite = attributes["sprite"].GetInt();
			}
		}

		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}
}

void DebugTimers_t::printAllTimepoints()
{
	int posy = 100;
	for ( auto& keyValue : timepoints )
	{
		printTimepoints(keyValue.first, posy);
		posy += 16;
	}
}

void DebugTimers_t::printTimepoints(std::string key, int& posy)
{
	if ( !font8x8_bmp || intro ) {
		return;
	}
	auto& points = timepoints[key];
	if ( points.empty() ) { return; }
	int starty = posy;
	int index = 0;
	std::string output = "";
	auto previousPoint = points[0];
	for ( auto& point : points )
	{
		double timediff = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(point.second - previousPoint.second).count();
		char outputBuf[1024] = "";
		snprintf(outputBuf, sizeof(outputBuf), "[%d]['%s'] %4.5fms\n", index, point.first.c_str(), timediff);
		output += outputBuf;
		posy += 8;
		if ( index > 0 )
		{
			previousPoint = point;
		}
		++index;
	}
	printTextFormatted(font8x8_bmp, 8, starty, "%s:\n%s", key.c_str(), output.c_str());
}

bool GlyphRenderer_t::readFromFile()
{
	if ( PHYSFS_getRealDir("/data/keyboard_glyph_config.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/keyboard_glyph_config.json");
		inputPath.append("/data/keyboard_glyph_config.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}

		allGlyphs.clear();
		if ( d.HasMember("rendered_glyph_folder") )
		{
			renderedGlyphFolder = d["rendered_glyph_folder"].GetString();
		}
		if ( d.HasMember("base_glyph_folder") )
		{
			baseSourceFolder = d["base_glyph_folder"].GetString();
		}
		if ( d.HasMember("small_key_unpressed_path") )
		{
			baseUnpressedGlyphPath = d["small_key_unpressed_path"].GetString();
		}
		if ( d.HasMember("small_key_pressed_path") )
		{
			basePressedGlyphPath = d["small_key_pressed_path"].GetString();
		}
		int render_offsety = 0;
		if ( d.HasMember("base_render_offset_y") )
		{
			render_offsety = d["base_render_offset_y"].GetInt();
		}
		pressedRenderedPrefix = "Pressed_";
		if ( d.HasMember("pressed_rendered_glyph_prefix") )
		{
			pressedRenderedPrefix = d["pressed_rendered_glyph_prefix"].GetString();
		}
		unpressedRenderedPrefix = "Unpressed_";
		if ( d.HasMember("unpressed_rendered_glyph_prefix") )
		{
			unpressedRenderedPrefix = d["unpressed_rendered_glyph_prefix"].GetString();
		}

		std::string basePath = baseSourceFolder;
		basePath += '/';
		std::string baseRenderedPath = basePath;
		baseRenderedPath += renderedGlyphFolder;
		baseRenderedPath += '/';

		for ( rapidjson::Value::ConstValueIterator glyph_itr = d["glyphs"].Begin(); glyph_itr != d["glyphs"].End(); ++glyph_itr )
		{
			const rapidjson::Value& attributes = *glyph_itr;
			if ( !attributes.HasMember("keyname") )
			{
				printlog("[JSON]: Glyph entry does not have keyname, skipping...");
				continue;
			}
			std::string keyname = attributes["keyname"].GetString();
			int keycode = SDL_GetKeyFromName(keyname.c_str());
			if ( keycode == SDLK_UNKNOWN )
			{
				printlog("[JSON]: Glyph name: %s could not find a keycode, skipping...", keyname.c_str());
				continue;
			}
			allGlyphs[keycode] = GlyphData_t();
			auto& glyphData = allGlyphs[keycode];
			glyphData.keycode = keycode;
			glyphData.keyname = keyname;
			if ( !attributes.HasMember("folder") )
			{
				printlog("[JSON]: Glyph entry does not have base folder entry, skipping...");
				continue;
			}
			glyphData.folder = attributes["folder"].GetString();
			if ( !attributes.HasMember("path") )
			{
				printlog("[JSON]: Glyph entry does not have glyph path name, skipping...");
				continue;
			}
			glyphData.filename = attributes["path"].GetString();
			if ( attributes.HasMember("custom_render_offset_y") )
			{
				if ( attributes["custom_render_offset_y"].GetInt() != 0 )
				{
					glyphData.render_offsety = attributes["custom_render_offset_y"].GetInt();
				}
				else
				{
					glyphData.render_offsety = render_offsety;
				}
			}
			else
			{
				glyphData.render_offsety = render_offsety;
			}
			if ( attributes.HasMember("pressed_glyph_background_path") )
			{
				glyphData.pressedGlyphPath = attributes["pressed_glyph_background_path"].GetString();
			}
			else
			{
				glyphData.pressedGlyphPath = basePressedGlyphPath;
			}
			if ( attributes.HasMember("unpressed_glyph_background_path") )
			{
				glyphData.unpressedGlyphPath = attributes["unpressed_glyph_background_path"].GetString();
			}
			else
			{
				glyphData.unpressedGlyphPath = baseUnpressedGlyphPath;
			}
		}

		for ( auto& keyValue : allGlyphs )
		{
			auto& glyphData = keyValue.second;
            glyphData.fullpath = "";
            glyphData.pressedRenderedFullpath = "";
            glyphData.unpressedRenderedFullpath = "";
            
            glyphData.fullpath = basePath;
            glyphData.fullpath += glyphData.folder;
            glyphData.fullpath += '/';
            glyphData.fullpath += glyphData.filename;

			if ( !PHYSFS_getRealDir(glyphData.fullpath.c_str()) ) // you need single forward '/' slashes for getRealDir to report true
			{
				printlog("[JSON]: Glyph path: %s not detected; won't be able to render!", glyphData.fullpath.c_str());
                glyphData.fullpath = "";
			}

			const std::string renderedPath = baseRenderedPath + glyphData.folder + '/';

			glyphData.unpressedRenderedFullpath = renderedPath;
			glyphData.unpressedRenderedFullpath += unpressedRenderedPrefix;
			glyphData.unpressedRenderedFullpath += glyphData.filename;
			if ( glyphData.unpressedRenderedFullpath[0] == '/' )
			{
				glyphData.unpressedRenderedFullpath.erase((size_t)0, (size_t)1);
			}
            if ( !PHYSFS_getRealDir(glyphData.unpressedRenderedFullpath.c_str()) )
            {
                printlog("[JSON]: Glyph path: %s not detected", glyphData.unpressedRenderedFullpath.c_str());
            }

			glyphData.pressedRenderedFullpath = renderedPath;
			glyphData.pressedRenderedFullpath += pressedRenderedPrefix;
			glyphData.pressedRenderedFullpath += glyphData.filename;
			if ( glyphData.pressedRenderedFullpath[0] == '/' )
			{
				glyphData.pressedRenderedFullpath.erase((size_t)0, (size_t)1);
			}
            if ( !PHYSFS_getRealDir(glyphData.pressedRenderedFullpath.c_str()) )
            {
                printlog("[JSON]: Glyph path: %s not detected", glyphData.pressedRenderedFullpath.c_str());
            }
		}

		printlog("[JSON]: Successfully read json file %s, processed %d glyphs", inputPath.c_str(), allGlyphs.size());
		return true;
	}
	printlog("[JSON]: Error: Could not locate json file %s", "/data/keyboard_glyph_config.json");
	return false;
}

void GlyphRenderer_t::renderGlyphsToPNGs()
{
#if defined(EDITOR) || defined(NINTENDO)
	return;
#else
	printlog("[Glyph Export]: Starting export...");
	int errors = 0;
	for ( auto& keyValue : allGlyphs )
	{
		std::string pressedPath = baseSourceFolder;
		pressedPath += '/';
		pressedPath += keyValue.second.pressedGlyphPath;
		if ( pressedPath[0] == '/' )
		{
			pressedPath.erase((size_t)0, (size_t)1);
		}

		std::string unpressedPath = baseSourceFolder;
		unpressedPath += '/';
		unpressedPath += keyValue.second.unpressedGlyphPath;
		if ( unpressedPath[0] == '/' )
		{
			unpressedPath.erase((size_t)0, (size_t)1);
		}

		auto& glyphData = keyValue.second;

		Image* base = Image::get(unpressedPath.c_str());
		if ( base->getWidth() != 0 )
		{
			// successfully loaded, do unpressed glyph
			SDL_Surface* srcSurf = const_cast<SDL_Surface*>(base->getSurf());
			SDL_Rect pos{ 0, 0, (int)base->getWidth(), (int)base->getHeight() };
			SDL_Surface* sprite = SDL_CreateRGBSurface(0, pos.w, pos.h, 32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
			SDL_SetSurfaceAlphaMod(srcSurf, 255);
			SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);

			std::string keyPath = keyValue.second.fullpath;
			if ( keyPath[0] == '/' )
			{
				keyPath.erase((size_t)0, (size_t)1);
			}
			auto key = Image::get(keyPath.c_str());
			if ( key->getWidth() != 0 )
			{
				// successfully loaded
				SDL_Surface* keySurf = const_cast<SDL_Surface*>(key->getSurf());
				SDL_Rect keyPos{ 0, 0, (int)key->getWidth(), (int)key->getHeight() };
				keyPos.x = pos.w / 2 - keyPos.w / 2;
				keyPos.y = keyValue.second.render_offsety;

				SDL_BlitSurface(keySurf, nullptr, sprite, &keyPos);

				std::string writePath = keyValue.second.unpressedRenderedFullpath;

				if ( writePath[0] == '/' )
				{
					writePath.erase((size_t)0, (size_t)1);
				}

				if ( SDL_SavePNG(sprite, writePath.c_str()) == 0 )
				{
					printlog("[Glyph Export]: Successfully exported unpressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
				else
				{
					printlog("[Glyph Export]: Failed exporting unpressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
			}
			else
			{
				printlog("[Glyph Export]: Failed exporting unpressed glyph: %s", keyValue.second.keyname.c_str());
				++errors;
			}
			if ( sprite )
			{
				SDL_FreeSurface(sprite);
				sprite = nullptr;
			}
		}
		else
		{
			++errors;
		}

		base = Image::get(pressedPath.c_str());
		if ( base->getWidth() != 0 )
		{
			// successfully loaded, do pressed glyph
			SDL_Surface* srcSurf = const_cast<SDL_Surface*>(base->getSurf());
			SDL_Rect pos{ 0, 0, (int)base->getWidth(), (int)base->getHeight() };
			SDL_Surface* sprite = SDL_CreateRGBSurface(0, pos.w, pos.h, 32,
				0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
			SDL_SetSurfaceAlphaMod(srcSurf, 255);
			SDL_SetSurfaceBlendMode(srcSurf, SDL_BLENDMODE_NONE);
			SDL_BlitScaled(srcSurf, nullptr, sprite, &pos);

			std::string keyPath = keyValue.second.fullpath;
			if ( keyPath[0] == '/' )
			{
				keyPath.erase((size_t)0, (size_t)1);
			}
			auto key = Image::get(keyPath.c_str());
			if ( key->getWidth() != 0 )
			{
				// successfully loaded
				SDL_Surface* keySurf = const_cast<SDL_Surface*>(key->getSurf());
				SDL_Rect keyPos{ 0, 0, (int)key->getWidth(), (int)key->getHeight() };
				keyPos.x = pos.w / 2 - keyPos.w / 2;
				keyPos.y = keyValue.second.render_offsety;

				SDL_BlitSurface(keySurf, nullptr, sprite, &keyPos);

				std::string writePath = keyValue.second.pressedRenderedFullpath;
				if ( writePath[0] == '/' )
				{
					writePath.erase((size_t)0, (size_t)1);
				}

				if ( SDL_SavePNG(sprite, writePath.c_str()) == 0 )
				{
					printlog("[Glyph Export]: Successfully exported pressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
				else
				{
					printlog("[Glyph Export]: Failed exporting pressed glyph: %s | path: %s", keyValue.second.keyname.c_str(), writePath.c_str());
				}
			}
			else
			{
				printlog("[Glyph Export]: Failed exporting pressed glyph: %s", keyValue.second.keyname.c_str());
				++errors;
			}
			if ( sprite )
			{
				SDL_FreeSurface(sprite);
				sprite = nullptr;
			}
		}
		else
		{
			++errors;
		}

	}

	printlog("[Glyph Export]: Completed export of %d glyphs with %d errors.", allGlyphs.size(), errors);
#endif
}

void ScriptTextParser_t::readAllScripts()
{
	allEntries.clear();

	std::string baseDir = "/data/scripts";
	auto files = physfsGetFileNamesInDirectory(baseDir.c_str());
	for ( auto file : files )
	{
		std::string checkFile = baseDir + '/' + file;
		PHYSFS_Stat stat;
		if ( PHYSFS_stat(checkFile.c_str(), &stat) == 0 ) { continue; }

		if ( stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
		{
			auto files2 = physfsGetFileNamesInDirectory(checkFile.c_str());
			for ( auto file2 : files2 )
			{
				std::string checkFile2 = checkFile + '/' + file2;
				if ( PHYSFS_stat(checkFile2.c_str(), &stat) == 0 ) { continue; }

				if ( stat.filetype != PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY )
				{
					readFromFile(checkFile2);
				}
			}
		}
		else
		{
			readFromFile(checkFile);
		}
	}
}

bool ScriptTextParser_t::readFromFile(const std::string& filename)
{
	if ( filename.find(".json") == std::string::npos )
	{
		return false;
	}
	if ( PHYSFS_getRealDir(filename.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(filename.c_str());
		inputPath.append(filename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return false;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("script_entries") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return false;
		}

		Uint32 defaultFontColor = 0xFFFFFFFF;
		Uint32 defaultFontOutlineColor = 0;
		Uint32 defaultFontHighlightColor = 0xFFFFFFFF;
		Uint32 defaultFontHighlight2Color = 0xFFFFFFFF;
		if ( d.HasMember("default_attributes") )
		{
			if ( d["default_attributes"].HasMember("font_color") )
			{
				defaultFontColor = makeColor(
					d["default_attributes"]["font_color"]["r"].GetInt(),
					d["default_attributes"]["font_color"]["g"].GetInt(),
					d["default_attributes"]["font_color"]["b"].GetInt(),
					d["default_attributes"]["font_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_outline_color") )
			{
				defaultFontOutlineColor = makeColor(
					d["default_attributes"]["font_outline_color"]["r"].GetInt(),
					d["default_attributes"]["font_outline_color"]["g"].GetInt(),
					d["default_attributes"]["font_outline_color"]["b"].GetInt(),
					d["default_attributes"]["font_outline_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_highlight_color") )
			{
				defaultFontHighlightColor = makeColor(
					d["default_attributes"]["font_highlight_color"]["r"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["g"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["b"].GetInt(),
					d["default_attributes"]["font_highlight_color"]["a"].GetInt());
			}
			if ( d["default_attributes"].HasMember("font_highlight2_color") )
			{
				defaultFontHighlight2Color = makeColor(
					d["default_attributes"]["font_highlight2_color"]["r"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["g"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["b"].GetInt(),
					d["default_attributes"]["font_highlight2_color"]["a"].GetInt());
			}
		}

		for ( rapidjson::Value::ConstMemberIterator entry_itr = d["script_entries"].MemberBegin(); entry_itr != d["script_entries"].MemberEnd(); ++entry_itr )
		{
			std::string key = entry_itr->name.GetString();
			allEntries[key] = Entry_t();
			auto& entry = allEntries[key];
			entry.name = key;
			entry.fontColor = defaultFontColor;
			entry.fontOutlineColor = defaultFontOutlineColor;
			entry.fontHighlightColor = defaultFontHighlightColor;
			entry.fontHighlight2Color = defaultFontHighlight2Color;
			if ( entry_itr->value.HasMember("sign") )
			{
				entry.objectType = OBJ_SIGN;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["sign"].Begin(); text_itr != entry_itr->value["sign"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				if ( entry_itr->value.HasMember("variables") )
				{
					for ( rapidjson::Value::ConstValueIterator var_itr = entry_itr->value["variables"].Begin();
						var_itr != entry_itr->value["variables"].End(); ++var_itr )
					{
						Entry_t::Variable_t variable;
						variable.type = TEXT;
						if ( (*var_itr).HasMember("type") )
						{
							std::string typeTxt = (*var_itr)["type"].GetString();
							if ( typeTxt == "text" )
							{
								variable.type = TEXT;
							}
							else if ( typeTxt == "input_glyph" )
							{
								variable.type = GLYPH;
							}
							else if ( typeTxt == "image" )
							{
								variable.type = IMG;
							}
						}
						if ( (*var_itr).HasMember("value") )
						{
							variable.value = (*var_itr)["value"].GetString();
						}
						if ( (*var_itr).HasMember("sizex") )
						{
							variable.sizex = (*var_itr)["sizex"].GetInt();
						}
						if ( (*var_itr).HasMember("sizey") )
						{
							variable.sizey = (*var_itr)["sizey"].GetInt();
						}
						entry.variables.push_back(variable);
					}
				}
				for ( size_t s = 0; s < entry.rawText.size(); ++s )
				{
					entry.padPerLine.push_back(0);
				}
				if ( entry_itr->value.HasMember("attributes") )
				{
					if ( entry_itr->value["attributes"].HasMember("font") )
					{
						entry.font = entry_itr->value["attributes"]["font"].GetString();
					}
					if ( entry_itr->value["attributes"].HasMember("horizontal_justify") )
					{
						std::string s = entry_itr->value["attributes"]["horizontal_justify"].GetString();
						if ( s == "center" )
						{
							entry.hjustify = Field::justify_t::CENTER;
						}
						else if ( s == "left" )
						{
							entry.hjustify = Field::justify_t::LEFT;
						}
						else if ( s == "right" )
						{
							entry.hjustify = Field::justify_t::RIGHT;
						}
					}
					if ( entry_itr->value["attributes"].HasMember("vertical_justify") )
					{
						std::string s = entry_itr->value["attributes"]["vertical_justify"].GetString();
						if ( s == "center" )
						{
							entry.vjustify = Field::justify_t::CENTER;
						}
						else if ( s == "top" )
						{
							entry.vjustify = Field::justify_t::TOP;
						}
						else if ( s == "bottom" )
						{
							entry.vjustify = Field::justify_t::BOTTOM;
						}
					}
					if ( entry_itr->value["attributes"].HasMember("top_padding") )
					{
						entry.padTopY = entry_itr->value["attributes"]["top_padding"].GetInt();
					}
					if ( entry_itr->value["attributes"].HasMember("line_padding") )
					{
						if ( entry_itr->value["attributes"]["line_padding"].IsInt() )
						{
							for ( auto& s : entry.padPerLine )
							{
								s = entry_itr->value["attributes"]["line_padding"].GetInt();
							}
						}
						else if ( entry_itr->value["attributes"]["line_padding"].IsArray() )
						{
							size_t s = 0;
							for ( auto arr_itr = entry_itr->value["attributes"]["line_padding"].Begin();
								arr_itr != entry_itr->value["attributes"]["line_padding"].End(); ++arr_itr )
							{
								entry.padPerLine[s] = arr_itr->GetInt();
								++s;
								if ( s >= entry.padPerLine.size() )
								{
									break;
								}
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("font_color") )
					{
						entry.fontColor = makeColor(
							entry_itr->value["attributes"]["font_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_outline_color") )
					{
						entry.fontOutlineColor = makeColor(
							entry_itr->value["attributes"]["font_outline_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_outline_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_highlight_color") )
					{
						entry.fontHighlightColor = makeColor(
							entry_itr->value["attributes"]["font_highlight_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_highlight_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("font_highlight2_color") )
					{
						entry.fontHighlight2Color = makeColor(
							entry_itr->value["attributes"]["font_highlight2_color"]["r"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["g"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["b"].GetInt(),
							entry_itr->value["attributes"]["font_highlight2_color"]["a"].GetInt());
					}
					if ( entry_itr->value["attributes"].HasMember("word_highlights") )
					{
						if ( entry_itr->value["attributes"]["word_highlights"].IsArray() )
						{
							int lineNumber = 0;
							for ( auto highlight_itr = entry_itr->value["attributes"]["word_highlights"].Begin();
								highlight_itr != entry_itr->value["attributes"]["word_highlights"].End(); ++highlight_itr )
							{
								for ( auto line_itr = (*highlight_itr).Begin(); line_itr != (*highlight_itr).End(); ++line_itr )
								{
									entry.wordHighlights.push_back(lineNumber + line_itr->GetInt());
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("word_highlights2") )
					{
						if ( entry_itr->value["attributes"]["word_highlights2"].IsArray() )
						{
							int lineNumber = 0;
							for ( auto highlight_itr = entry_itr->value["attributes"]["word_highlights2"].Begin();
								highlight_itr != entry_itr->value["attributes"]["word_highlights2"].End(); ++highlight_itr )
							{
								for ( auto line_itr = (*highlight_itr).Begin(); line_itr != (*highlight_itr).End(); ++line_itr )
								{
									entry.wordHighlights2.push_back(lineNumber + line_itr->GetInt());
								}
								lineNumber += Field::TEXT_HIGHLIGHT_WORDS_PER_LINE;
							}
						}
					}
					if ( entry_itr->value["attributes"].HasMember("inline_img_adjust_x") )
					{
						entry.imageInlineTextAdjustX = entry_itr->value["attributes"]["inline_img_adjust_x"].GetInt();
					}
					if ( entry_itr->value["attributes"].HasMember("video") )
					{
						if ( entry_itr->value["attributes"]["video"].HasMember("path") )
						{
							entry.signVideoContent.path = entry_itr->value["attributes"]["video"]["path"].GetString();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("x") )
						{
							entry.signVideoContent.pos.x = entry_itr->value["attributes"]["video"]["x"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("y") )
						{
							entry.signVideoContent.pos.y = entry_itr->value["attributes"]["video"]["y"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("w") )
						{
							entry.signVideoContent.pos.w = entry_itr->value["attributes"]["video"]["w"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("h") )
						{
							entry.signVideoContent.pos.h = entry_itr->value["attributes"]["video"]["h"].GetInt();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("background_img") )
						{
							entry.signVideoContent.bgPath = entry_itr->value["attributes"]["video"]["background_img"].GetString();
						}
						if ( entry_itr->value["attributes"]["video"].HasMember("background_border") )
						{
							entry.signVideoContent.imgBorder = entry_itr->value["attributes"]["video"]["background_border"].GetInt();
						}
					}
				}
			}
			else if ( entry_itr->value.HasMember("script") )
			{
				entry.objectType = OBJ_SCRIPT;
				entry.formattedText = entry_itr->value["script"].GetString();
			}
			else if ( entry_itr->value.HasMember("message") )
			{
				entry.objectType = OBJ_MESSAGE;
				for ( rapidjson::Value::ConstValueIterator text_itr = entry_itr->value["message"].Begin(); text_itr != entry_itr->value["message"].End(); ++text_itr )
				{
					entry.rawText.push_back(text_itr->GetString());
				}
				if ( entry_itr->value.HasMember("variables") )
				{
					for ( rapidjson::Value::ConstValueIterator var_itr = entry_itr->value["variables"].Begin();
						var_itr != entry_itr->value["variables"].End(); ++var_itr )
					{
						Entry_t::Variable_t variable;
						variable.type = TEXT;
						if ( (*var_itr).HasMember("type") )
						{
							std::string typeTxt = (*var_itr)["type"].GetString();
							if ( typeTxt == "text" )
							{
								variable.type = TEXT;
							}
							else if ( typeTxt == "input_glyph" )
							{
								variable.type = GLYPH;
							}
							else if ( typeTxt == "image" )
							{
								variable.type = IMG;
							}
						}
						if ( (*var_itr).HasMember("value") )
						{
							variable.value = (*var_itr)["value"].GetString();
						}
						entry.variables.push_back(variable);
					}
				}
			}
		}

		printlog("[JSON]: Successfully read json file %s, processed %d script variables", inputPath.c_str(), allEntries.size());
		return true;
	}
	printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
	return false;
}

void ScriptTextParser_t::writeWorldSignsToFile()
{
#ifndef EDITOR
	rapidjson::Document exportDocument;
	exportDocument.SetObject();
	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));
	rapidjson::Value objScriptEntries(rapidjson::kObjectType);

	char suffix = 'a';
	char suffix2 = 'a';
	bool doubleSuffix = false;

	SDL_Color fontColor{ 255, 255, 255, 255 };
	SDL_Color fontOutline{ 29, 16, 11, 255 };
	SDL_Color fontHighlight{ 255, 0, 255, 255 };

	rapidjson::Value objDefaultAttributes(rapidjson::kObjectType);
	{
		rapidjson::Value objFontColor(rapidjson::kObjectType);
		objFontColor.AddMember("r", rapidjson::Value(fontColor.r), exportDocument.GetAllocator());
		objFontColor.AddMember("g", rapidjson::Value(fontColor.g), exportDocument.GetAllocator());
		objFontColor.AddMember("b", rapidjson::Value(fontColor.b), exportDocument.GetAllocator());
		objFontColor.AddMember("a", rapidjson::Value(fontColor.a), exportDocument.GetAllocator());

		rapidjson::Value objFontOutlineColor(rapidjson::kObjectType);
		objFontOutlineColor.AddMember("r", rapidjson::Value(fontOutline.r), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("g", rapidjson::Value(fontOutline.g), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("b", rapidjson::Value(fontOutline.b), exportDocument.GetAllocator());
		objFontOutlineColor.AddMember("a", rapidjson::Value(fontOutline.a), exportDocument.GetAllocator());

		rapidjson::Value objFontHighlightColor(rapidjson::kObjectType);
		objFontHighlightColor.AddMember("r", rapidjson::Value(fontHighlight.r), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("g", rapidjson::Value(fontHighlight.g), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("b", rapidjson::Value(fontHighlight.b), exportDocument.GetAllocator());
		objFontHighlightColor.AddMember("a", rapidjson::Value(fontHighlight.a), exportDocument.GetAllocator());

		rapidjson::Value objFontHighlight2Color(rapidjson::kObjectType);
		objFontHighlight2Color.AddMember("r", rapidjson::Value(fontHighlight.r), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("g", rapidjson::Value(fontHighlight.g), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("b", rapidjson::Value(fontHighlight.b), exportDocument.GetAllocator());
		objFontHighlight2Color.AddMember("a", rapidjson::Value(fontHighlight.a), exportDocument.GetAllocator());

		objDefaultAttributes.AddMember("font_color", objFontColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_outline_color", objFontOutlineColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_highlight_color", objFontHighlightColor, exportDocument.GetAllocator());
		objDefaultAttributes.AddMember("font_highlight2_color", objFontHighlight2Color, exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "default_attributes", objDefaultAttributes);

	for ( auto node = map.entities->first; node != NULL; node = node->next )
	{
		auto entity = (Entity*)node->element;
		if ( entity->behavior == &actFloorDecoration && entity->sprite == 991 /* sign */ )
		{
			std::string key = map.filename;
			size_t find = key.find(".lmp");
			key.erase(find, strlen(".lmp"));
			if ( doubleSuffix ) { key += suffix2; }
			key += suffix;
			if ( suffix == 'z' )
			{
				doubleSuffix = true;
				suffix = 'a';
			}
			suffix += 1;
			
			printlog("Sign '%s': x: %d y: %d", key.c_str(), static_cast<int>(entity->x) >> 4, static_cast<int>(entity->y) >> 4);

			rapidjson::Value objEntry(rapidjson::kObjectType);
			rapidjson::Value arrSignText(rapidjson::kArrayType);

			// assemble the string.
			char buf[256] = "";
			int totalChars = 0;
			for ( int i = 8; i < 60; ++i )
			{
				if ( i == 28 ) // circuit_status
				{
					continue;
				}
				if ( entity->skill[i] != 0 )
				{
					for ( int c = 0; c < 4; ++c )
					{
						buf[totalChars] = static_cast<char>((entity->skill[i] >> (c * 8)) & 0xFF);
						//messagePlayer(0, "%d %d", i, c);
						++totalChars;
					}
				}
			}
			if ( buf[totalChars] != '\0' )
			{
				buf[totalChars] = '\0';
			}
			std::string output = buf;

			std::vector<std::string> signText;
			int line = 0;
			signText.push_back("");
			for ( int i = 0; i < output.size(); ++i )
			{
				if ( i == 0 && output[0] == '#' )
				{
					continue;
				}
				if ( output[i] == '\0' )
				{
					break;
				}
				if ( output[i] == '\\' && (i + 1) < output.size() && output[i + 1] == 'n' )
				{
					++i;
					++line;
					signText.push_back("");
					continue;
				}
				signText[line] += output[i];
			}

			for ( auto& line : signText )
			{
				rapidjson::Value lineString(line.c_str(), exportDocument.GetAllocator());
				arrSignText.PushBack(lineString, exportDocument.GetAllocator());
			}
			objEntry.AddMember("sign", arrSignText, exportDocument.GetAllocator());

			rapidjson::Value objAttributes(rapidjson::kObjectType);
			{
				objAttributes.AddMember("font", rapidjson::StringRef("fonts/pixel_maz_multiline.ttf#16#2"), exportDocument.GetAllocator());
				objAttributes.AddMember("horizontal_justify", rapidjson::StringRef("center"), exportDocument.GetAllocator());
				objAttributes.AddMember("vertical_justify", rapidjson::StringRef("center"), exportDocument.GetAllocator());
				objAttributes.AddMember("line_padding", rapidjson::Value(0), exportDocument.GetAllocator());
				objAttributes.AddMember("top_padding", rapidjson::Value(0), exportDocument.GetAllocator());
				objAttributes.AddMember("inline_img_adjust_x", rapidjson::Value(0), exportDocument.GetAllocator());
				rapidjson::Value objWordHighlights(rapidjson::kArrayType);
				objAttributes.AddMember("word_highlights", objWordHighlights, exportDocument.GetAllocator());
				rapidjson::Value objWordHighlights2(rapidjson::kArrayType);
				objAttributes.AddMember("word_highlights2", objWordHighlights2, exportDocument.GetAllocator());
			}
			objEntry.AddMember("attributes", objAttributes, exportDocument.GetAllocator());

			rapidjson::Value objVariables(rapidjson::kArrayType);
			objEntry.AddMember("variables", objVariables, exportDocument.GetAllocator());

			rapidjson::Value entryName(key.c_str(), exportDocument.GetAllocator());
			objScriptEntries.AddMember(entryName, objEntry, exportDocument.GetAllocator());
		}
	}

	CustomHelpers::addMemberToRoot(exportDocument, "script_entries", objScriptEntries);

	std::string outputPath = PHYSFS_getRealDir("/data/scripts");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/scripts/script.json";
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);
#endif
}

#ifdef USE_THEORA_VIDEO
#include "ui/Image.hpp"
bool VideoManager_t::isInit = false;
void VideoManager_t::drawTexturedQuad(unsigned int texID, int tw, int th, const SDL_Rect& src, const SDL_Rect& dest, float alpha)
{
    Uint32 color = makeColor(255, 255, 255, (uint8_t)(alpha * 255.f));
    const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
	Image::draw(texID, tw, th,
        &src, dest, viewport, color);
}

#ifndef EDITOR
static ConsoleVariable<bool> cvar_doublebufferVideo("/video_doublebuffer", true);
#endif

void VideoManager_t::drawAsFrameCallback(const Widget& widget, SDL_Rect frameSize, SDL_Rect offset, float alpha)
{
	if (!clip) {
		return;
	}

	theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
	if (frame) {
		GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, whichTexture ? textureId1 : textureId2));
        GL_CHECK_ERR(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            clip->getWidth(), clip->getHeight(), GL_RGBA,
            GL_UNSIGNED_BYTE, frame->getBuffer()));
		clip->popFrame();
	}

	float w = clip->getSubFrameWidth();
	float h = clip->getSubFrameHeight();
	float sx = clip->getSubFrameX();
	float sy = clip->getSubFrameY();
	float tw = w;
	float th = h;

	SDL_Rect rect = frameSize;
	if (offset.w <= 0) {
		// use native size of video
		rect.w = w;
		rect.w += offset.w;
		w += offset.w;
	} else {
		// manual scale video
		rect.w = offset.w;
	}

	if (offset.h <= 0) {
		// use native size of video
		rect.h = h;
		rect.h += offset.h;
		h += offset.h;
	} else {
		// manual scale video
		rect.h = offset.h;
	}

	if (offset.x < 0) {
		sx += -offset.x; // shift video to re-center
		offset.x = 0;
	}
	else if (offset.x + rect.w > frameSize.w) {
		rect.w -= (offset.x + rect.w) - frameSize.w; // limit output rect width to frame
	}
	if (offset.y < 0) {
		sy += -offset.y; // shift video to re-center
		offset.y = 0;
	}
	else if (offset.y + rect.h > frameSize.h) {
		rect.h -= (offset.y + rect.h) - frameSize.h; // limit output rect height to frame
	}

	if (frame) {
#ifndef EDITOR
		if (*cvar_doublebufferVideo) {
			whichTexture = (whichTexture == false);
		}
#else
		whichTexture = (whichTexture == false);
#endif
	}

	const SDL_Rect dest{rect.x + offset.x, rect.y + offset.y, rect.w, rect.h};
	const SDL_Rect src{(int)sx, (int)sy, (int)w, (int)h};
	drawTexturedQuad(whichTexture ? textureId1 : textureId2, tw, th, src, dest, alpha);
}

void VideoManager_t::draw()
{
	if (!clip) {
		return;
	}
    
	theoraplayer::VideoFrame* frame = clip->fetchNextFrame();
	if (frame) {
		GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, whichTexture ? textureId1 : textureId2));
        GL_CHECK_ERR(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            clip->getWidth(), clip->getHeight(), GL_RGBA,
            GL_UNSIGNED_BYTE, frame->getBuffer()));
		clip->popFrame();
	}
    
	const int sw = clip->getSubFrameWidth();
    const int sh = clip->getSubFrameHeight();
    const int sx = clip->getSubFrameX();
    const int sy = clip->getSubFrameY();
    const int tw = sw;
    const int th = sh;

	if (frame) {
#ifndef EDITOR
		if (*cvar_doublebufferVideo) {
			whichTexture = (whichTexture == false);
		}
#else
		whichTexture = (whichTexture == false);
#endif
	}

	const SDL_Rect dest{400, 200, 320, 180};
	const SDL_Rect src{sx, sy, sw, sh};
	drawTexturedQuad(whichTexture ? textureId1 : textureId2, tw, th, src, dest, 1.f);
}

unsigned int VideoManager_t::createTexture(int w, int h, unsigned int format)
{
	unsigned int tex = 0;
    GL_CHECK_ERR(glGenTextures(1, &tex));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, tex));
	unsigned char* data = new unsigned char[w * h * 4];
	memset(data, 0, w * h * 4);
    GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	delete[] data;
	return tex;
}

void VideoManager_t::init()
{
	if ( isInit )
	{
		return;
	}
	theoraplayer::init(1);
	isInit = true;
}

void VideoManager_t::destroy()
{
	theoraplayer::destroy();
	isInit = false;
}

void VideoManager_t::deinitManager()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		VideoManager[i].destroyClip();
	}
	destroy();
}

void VideoManager_t::destroyClip()
{
	started = false;
	if ( clip )
	{
		theoraplayer::manager->destroyVideoClip(clip);
		clip = NULL;
	}
	if ( textureId1 != 0 )
	{
        GL_CHECK_ERR(glDeleteTextures(1, &textureId1));
		textureId1 = 0;
	}
	if (textureId2 != 0)
	{
		GL_CHECK_ERR(glDeleteTextures(1, &textureId2));
		textureId2 = 0;
	}
}

#define PRELOAD_VIDEO_TO_RAM

#ifdef PRELOAD_VIDEO_TO_RAM
#include <theoraplayer/MemoryDataSource.h>
#endif

void VideoManager_t::loadfile(const char* filename)
{
	if (!isInit) {
		init();
	}
	if (clip) {
		destroyClip();
	}
	if (!filename || !PHYSFS_getRealDir(filename)) {
		return;
	}
	std::string path = PHYSFS_getRealDir(filename);
	path += PHYSFS_getDirSeparator();
	path += filename;

	auto output_format = theoraplayer::FORMAT_RGBX;

#ifndef EDITOR
	static ConsoleVariable<int> cvar_theoraOutput("/theora_output", 0);
	if (*cvar_theoraOutput > 0) {
		output_format = (theoraplayer::OutputMode)*cvar_theoraOutput;
	}
#endif

#ifdef PRELOAD_VIDEO_TO_RAM
	clip = theoraplayer::manager->createVideoClip(new theoraplayer::MemoryDataSource(path.c_str()), output_format);
#else
	clip = theoraplayer::manager->createVideoClip(path, output_format);
#endif

	if (!clip) {
		return;
	}
	currentfile = filename;
	clip->setAutoRestart(true);
	clip->setPrecachedFramesCount(16);
	textureId1 = createTexture(clip->getWidth(), clip->getHeight(), GL_RGBA);
	textureId2 = createTexture(clip->getWidth(), clip->getHeight(), GL_RGBA);
}

void VideoManager_t::updateCurrentClip(float timeDelta)
{
	if ( !clip )
	{
		started = false;
		return;
	}
	if ( !started )
	{
		// let's wait until the system caches up a few frames on startup
		if ( clip->getReadyFramesCount() < clip->getPrecachedFramesCount() * 0.5f )
		{
			return;
		}
		started = true;
	}
}

void VideoManager_t::update()
{
	init();

	if ( !isInit || !clip )
	{
		return;
	}
	//draw();
	static Uint32 time = SDL_GetTicks();
	Uint32 t = SDL_GetTicks();
	float diff = (t - time) / 1000.0f;
	if ( diff > 0.25f )
	{
		diff = 0.05f; // prevent spikes (usually happen on app load)
	}
	theoraplayer::manager->update(diff);
	updateCurrentClip(diff);
	time = t;
}
#endif

#ifndef EDITOR
void MonsterData_t::loadMonsterDataJSON()
{
	if ( PHYSFS_getRealDir("/data/monster_data.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/monster_data.json");
		inputPath.append("/data/monster_data.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("monsters") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}

		monsterDataEntries.clear();

		const std::string baseIconPath = d["base_path"].GetString();

		for ( auto itr = d["monsters"].MemberBegin(); itr != d["monsters"].MemberEnd(); ++itr )
		{
			std::string monsterTypeName = itr->name.GetString();
			int monsterType = NOTHING;
			for ( int i = 0; i < NUMMONSTERS; ++i )
			{
				if ( monsterTypeName == monstertypename[i] )
				{
					monsterType = i;
					break;
				}
			}

			monsterDataEntries[monsterType] = MonsterDataEntry_t(monsterType);
			auto& entry = monsterDataEntries[monsterType];

			for ( auto entry_itr = itr->value.MemberBegin(); entry_itr != itr->value.MemberEnd(); ++entry_itr )
			{
				std::string key = entry_itr->name.GetString();
				if ( key == "specialNPCs" )
				{
					for ( auto special_itr = entry_itr->value.MemberBegin(); special_itr != entry_itr->value.MemberEnd(); ++special_itr )
					{
						bool foundIcon = false;
						std::string iconPath = "";
						if ( special_itr->value.HasMember("icon") )
						{
							iconPath = special_itr->value["icon"].GetString();
							if ( iconPath.size() > 0 )
							{
								foundIcon = true;
								iconPath = baseIconPath + iconPath;
							}
						}
						if ( special_itr->value.HasMember("models") )
						{
							std::vector<int> models;
							if ( special_itr->value["models"].IsArray() )
							{
								for ( auto array_itr = special_itr->value["models"].Begin(); array_itr != special_itr->value["models"].End(); ++array_itr )
								{
									models.push_back(array_itr->GetInt());
								}
							}
							else if ( special_itr->value["models"].IsInt() )
							{
								models.push_back(special_itr->value["models"].GetInt());
							}
							assert(models.size() > 0);
							int baseModel = models[0];
							if ( special_itr->value.HasMember("base_model") )
							{
								baseModel = special_itr->value["base_model"].GetInt();
							}

							bool noOverrideIcon = false; // special case, handling monsters with unique icons but no unique sprites
							if ( special_itr->value.HasMember("no_override_icon") )
							{
								noOverrideIcon = special_itr->value["no_override_icon"].GetBool();
							}
							entry.specialNPCs[special_itr->name.GetString()] = MonsterDataEntry_t::SpecialNPCEntry_t();
							auto& specialNPC = entry.specialNPCs[special_itr->name.GetString()];
							specialNPC.internalName = special_itr->name.GetString();
							specialNPC.name = special_itr->value["localized_name"].GetString();
							specialNPC.baseModel = baseModel;
							if ( special_itr->value.HasMember("localized_short_name") )
							{
								specialNPC.shortname = special_itr->value["localized_short_name"].GetString();
							}
							if ( foundIcon && noOverrideIcon )
							{
								specialNPC.uniqueIcon = iconPath;
							}
							for ( auto m : models )
							{
								entry.modelIndexes.insert(m);
								if ( foundIcon )
								{
									if ( !noOverrideIcon )
									{
										entry.iconSpritesAndPaths[m].iconPath = iconPath;
										entry.iconSpritesAndPaths[m].key = special_itr->name.GetString();
										entry.keyToSpriteLookup[special_itr->name.GetString()].push_back(m);
									}
								}
								specialNPC.modelIndexes.insert(m);
							}
						}
					}
				}
				else
				{
					bool isPlayerSprite = (key.find("player") != std::string::npos) || monsterType == HUMAN;

					if ( entry_itr->value.HasMember("icon") )
					{
						std::string iconPath = entry_itr->value["icon"].GetString();
						if ( iconPath.size() > 0 )
						{
							iconPath = baseIconPath + iconPath;
						}
						if ( key == "default" )
						{
							entry.defaultIconPath = iconPath;
							if ( entry_itr->value.HasMember("localized_short_name") )
							{
								entry.defaultShortDisplayName = entry_itr->value["localized_short_name"].GetString();
							}
						}
						if ( entry_itr->value.HasMember("models") )
						{
							std::vector<int> models;
							if ( entry_itr->value["models"].IsArray() )
							{
								for ( auto array_itr = entry_itr->value["models"].Begin(); array_itr != entry_itr->value["models"].End(); ++array_itr )
								{
									models.push_back(array_itr->GetInt());
								}
							}
							else if ( entry_itr->value["models"].IsInt() )
							{
								models.push_back(entry_itr->value["models"].GetInt());
							}

							for ( auto m : models )
							{
								if ( isPlayerSprite )
								{
									entry.playerModelIndexes.insert(m);
								}
								entry.modelIndexes.insert(m);
								entry.iconSpritesAndPaths[m].iconPath = iconPath;
								entry.iconSpritesAndPaths[m].key = entry_itr->name.GetString();
								entry.keyToSpriteLookup[entry_itr->name.GetString()].push_back(m);
							}
						}
					}
				}
			}
		}
		// validate data
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			for ( auto sprite : monsterSprites[i] )
			{
				if ( monsterDataEntries[i].modelIndexes.find(sprite) == monsterDataEntries[i].modelIndexes.end() )
				{
					printlog("[JSON]: Error: Could not find monster %s model index: %d", monstertypename[i], sprite);
				}
				if ( Entity::isPlayerHeadSprite(sprite) )
				{
					if ( monsterData.monsterDataEntries[i].playerModelIndexes.find(sprite) == monsterDataEntries[i].playerModelIndexes.end() )
					{
						printlog("[JSON]: Error: Could not find player %s model index: %d", monstertypename[i], sprite);
					}
				}
			}
		}

		printlog("[JSON]: Successfully read json file %s, processed %d monsters", inputPath.c_str(), monsterDataEntries.size());
		return;
	}
	printlog("[JSON]: Error: Could not locate json file %s", "/data/monster_data.json");
}
#endif

#ifndef EDITOR
#ifdef USE_IMGUI
ImVec4 ImGui_t::colorOn = (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f);
ImVec4 ImGui_t::colorOnHovered = (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f);
ImVec4 ImGui_t::colorOnActive = (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f);
ImVec4 ImGui_t::colorBtnDefault = { 0, 0, 0, 0 };
ImVec4 ImGui_t::colorBtnDefaultActive = { 0, 0, 0, 0 };
ImVec4 ImGui_t::colorBtnDefaultHovered = { 0, 0, 0, 0 };
bool ImGui_t::isInit = false;
bool ImGui_t::queueInit = false;
bool ImGui_t::queueDeinit = false;
bool ImGui_t::show_demo_window = false;
bool ImGui_t::disablePlayerControl = false;
SDL_Rect ImGui_t::debugRect{ 0, 0, 0, 0 };

void ImGui_t::init()
{
	if ( isInit )
	{
		return;
	}
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(screen, renderer);
	ImGui_ImplOpenGL3_Init();

	colorBtnDefault = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	colorBtnDefaultActive = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
	colorBtnDefaultHovered = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);

	isInit = true;
	disablePlayerControl = false;
}

void ImGui_t::deinit()
{
	queueInit = false;
	queueDeinit = false;
	disablePlayerControl = false;
	if ( !isInit )
	{
		return;
	}
	isInit = false;
	SDL_ShowCursor(SDL_FALSE);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void ImGui_t::render()
{
	if ( !ImGui_t::isInit )
	{
		return;
	}

	auto& io = ImGui_t::getIO();
	ImGui::Render();
    GL_CHECK_ERR(glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y));
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGui_t::update()
{
	if ( !ImGui_t::isInit )
	{
		return;
	}

	auto& io = ImGui_t::getIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.DisplaySize.x = xres;
	io.DisplaySize.y = yres;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if ( ImGui_t::show_demo_window )
		ImGui::ShowDemoWindow(&ImGui_t::show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	ImVec2 lastWindowSize;
	ImVec2 lastWindowPos;
	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 200, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);

		ImGui::Begin("Dev Menu");

		if ( ImGui::Button("Close Menu") )
		{
			queueDeinit = true;
		}
		ImGui::Checkbox("Demo Window", &ImGui_t::show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Disable Player Control", &ImGui_t::disablePlayerControl);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::SliderInt("debug x", &debugRect.x, -300, 300);
		ImGui::SliderInt("debug y", &debugRect.y, -300, 300);
		ImGui::SliderInt("debug w", &debugRect.w, -300, 300);
		ImGui::SliderInt("debug h", &debugRect.h, -300, 300);

		lastWindowSize = ImGui::GetWindowSize();
		lastWindowPos = ImGui::GetWindowPos();
		ImGui::End();
	}

	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + lastWindowPos.x, 
			main_viewport->WorkPos.y + lastWindowPos.y + 140), 
			ImGuiCond_FirstUseEver);
		ImGui_t::showConsoleCommands();
		ImGui_t::showHUDTimers();
	}
}

void ImGui_t::buttonConsoleCommandHighlight(const char* cmd, bool flag)
{
	flag ? ImGui::PushStyleColor(ImGuiCol_Button, colorOn) : ImGui::PushStyleColor(ImGuiCol_Button, colorBtnDefault);
	flag ? ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorOnActive) : ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorBtnDefaultActive);
	flag ? ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorOnHovered) : ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorBtnDefaultHovered);
	if ( ImGui::Button(cmd) )
	{
		consoleCommand(cmd);
	}
	ImGui::PopStyleColor(3);
}

struct ImGuiHUDTimers_t
{
	std::string sliderName;
	std::string samplesName;
	std::string graphName;
	ImGuiHUDTimers_t(int index)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "Y Zoom G%d", index);
		sliderName = buf;
		snprintf(buf, sizeof(buf), "Samples G%d", index);
		samplesName = buf;
		snprintf(buf, sizeof(buf), "GUI %d ms", index);
		graphName = buf;
	}
	float plotValues[1000] = { 0.f };
	int plotIndex = 0;
	float plotYZoom = 10.f;
	int plotSamples = 50;
	float average = 0.0f;
	void process();
};
std::vector<ImGuiHUDTimers_t> imguiHUDTimers;

void ImGui_t::showHUDTimers()
{
	int ids = 0;

	ImGui::Begin("HUD Timers", nullptr);
	const float windowWidth = ImGui::GetWindowWidth();

	if ( imguiHUDTimers.empty() )
	{
		for ( int i = 0; i < 11; ++i )
		{
			imguiHUDTimers.push_back(ImGuiHUDTimers_t(i));
		}
	}

	for ( int i = 0; i < 11; ++i )
	{
		float milliseconds = 0.0;
		switch ( i )
		{
			case 0:
			{
				auto ms = std::chrono::time_point_cast<std::chrono::microseconds>(DebugStats.gui2);
				milliseconds = ms.time_since_epoch().count() / 1000.0;
				break;
			}
			case 1:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui3 - DebugStats.gui2).count();
				break;
			case 2:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui4 - DebugStats.gui3).count();
				break;
			case 3:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui5 - DebugStats.gui4).count();
				break;
			case 4:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui6 - DebugStats.gui5).count();
				break;
			case 5:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui7 - DebugStats.gui6).count();
				break;
			case 6:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui8 - DebugStats.gui7).count();
				break;
			case 7:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui9 - DebugStats.gui8).count();
				break;
			case 8:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui10 - DebugStats.gui9).count();
				break;
			case 9:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui11 - DebugStats.gui10).count();
				break;
			case 10:
				milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.gui12 - DebugStats.gui11).count();
				break;
			default:
				break;
		}

		{
			
			ImGui::SetNextItemWidth(windowWidth * .25);
			ImGui::SliderFloat(imguiHUDTimers[i].sliderName.c_str(), &imguiHUDTimers[i].plotYZoom, 1, 20);

			ImGui::SameLine();
			ImGui::SetNextItemWidth(windowWidth * .25);
			ImGui::SliderInt(imguiHUDTimers[i].samplesName.c_str(), &imguiHUDTimers[i].plotSamples, 50, 1000);

			imguiHUDTimers[i].plotValues[imguiHUDTimers[i].plotIndex] = milliseconds;
			++imguiHUDTimers[i].plotIndex;
			if ( imguiHUDTimers[i].plotIndex >= imguiHUDTimers[i].plotSamples )
			{
				imguiHUDTimers[i].plotIndex = 0;
			}

			float average = 0.0f;
			int usefulSamples = imguiHUDTimers[i].plotSamples;
			for ( int n = 0; n < std::min(imguiHUDTimers[i].plotSamples, IM_ARRAYSIZE(imguiHUDTimers[i].plotValues)); n++ )
			{
				if ( imguiHUDTimers[i].plotValues[n] == 0.f )
				{
					--usefulSamples;
				}
				average += imguiHUDTimers[i].plotValues[n];
			}
			average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(imguiHUDTimers[i].plotValues));

			char overlay[32];
			sprintf(overlay, "avg %.5fms", average);

			ImGui::PlotLines(imguiHUDTimers[i].graphName.c_str(), imguiHUDTimers[i].plotValues, std::min(imguiHUDTimers[i].plotSamples, IM_ARRAYSIZE(imguiHUDTimers[i].plotValues)), 0, overlay, 0.f, imguiHUDTimers[i].plotYZoom, ImVec2(0, 50.f));
		}
	}
	ImGui::End();
}

void ImGui_t::showConsoleCommands()
{
	int ids = 0;

	ImGui::Begin("Console Commands", nullptr);

	if ( ImGui::Button("/nextlevel") )
	{
		consoleCommand("/nextlevel");
	}
	ImGui::SameLine();
	buttonConsoleCommandHighlight("/god", godmode);
	ImGui::SameLine();
	buttonConsoleCommandHighlight("/entityfreeze", gameloopFreezeEntities);

	buttonConsoleCommandHighlight("/disable_controller_reconnect true", false);
	buttonConsoleCommandHighlight("/splitscreen", splitscreen);
	buttonConsoleCommandHighlight("/culling_max_walls 1", false);
	buttonConsoleCommandHighlight("/culling_max_walls 2", false);

	static int jumpLevel = 0;
	if ( ImGui::Button("/jumplevel") )
	{
		std::string cmd = "/jumplevel ";
		char num[32];
		snprintf(num, sizeof(num), "%d", jumpLevel);
		cmd += num;
		consoleCommand(cmd.c_str());
		jumpLevel = 0;
	}
	ImGui::SameLine();
	if ( ImGui::ArrowButton("jumplvl--", ImGuiDir_::ImGuiDir_Left) )
	{
		jumpLevel -= 5;
	}
	ImGui::SameLine();
	if ( ImGui::ArrowButton("jumplvl-", ImGuiDir_::ImGuiDir_Left) )
	{
		jumpLevel--;
	}
	ImGui::SameLine();
	ImGui::Text("%d", jumpLevel);
	ImGui::SameLine();
	if ( ImGui::ArrowButton("jumplvl+", ImGuiDir_::ImGuiDir_Right) )
	{
		jumpLevel++;
	}
	ImGui::SameLine();
	if ( ImGui::ArrowButton("jumplvl++", ImGuiDir_::ImGuiDir_Right) )
	{
		jumpLevel += 5;
	}

	static int currentItem = 0;
	if ( ImGui::Button("/spawnitem") )
	{
		std::string cmd = "/spawnitem ";
		cmd += items[std::max(0, std::min(currentItem, NUMITEMS - 1))].getIdentifiedName();
		consoleCommand(cmd.c_str());
	}
	ImGui::SameLine();
	const char* combo_preview_value = items[currentItem].getIdentifiedName();
	if ( ImGui::BeginCombo("items", combo_preview_value) )
	{
		for ( int n = 0; n < IM_ARRAYSIZE(items); n++ )
		{
			const bool is_selected = (currentItem == n);
			if ( ImGui::Selectable(items[n].getIdentifiedName(), is_selected) )
				currentItem = n;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if ( is_selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	static int currentItem2 = 0;
	const char* statuses[] = { "Broken", "Decrepit", "Worn", "Servicable", "Excellent" };
	const char* beatitudes[] = { "-5", "-4", "-3", "-2","-1", "0", "1", "2", "3", "4", "5" };
	static int spawnItemBeatitudeIndex = 5;
	static int spawnItemStatusIndex = EXCELLENT;
	if ( ImGui::Button("/spawnitem2") )
	{
		std::string cmd = "/spawnitem2 ";
		char num[32];
		snprintf(num, sizeof(num), "%d", spawnItemBeatitudeIndex - 5);
		cmd += num;
		cmd += " ";
		snprintf(num, sizeof(num), "%d", spawnItemStatusIndex);
		cmd += num;
		cmd += " ";
		cmd += items[std::max(0, std::min(currentItem, NUMITEMS - 1))].getIdentifiedName();
		consoleCommand(cmd.c_str());
	}
	ImGui::SameLine();
	const char* combo_preview_value2 = items[currentItem2].getIdentifiedName();
	if ( ImGui::BeginCombo("items2", combo_preview_value) )
	{
		for ( int n = 0; n < IM_ARRAYSIZE(items); n++ )
		{
			const bool is_selected = (currentItem2 == n);
			if ( ImGui::Selectable(items[n].getIdentifiedName(), is_selected) )
				currentItem2 = n;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if ( is_selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	const float windowWidth = ImGui::GetWindowWidth();
	ImGui::SetNextItemWidth(windowWidth * .25);
	ImGui::Combo("Blessing", &spawnItemBeatitudeIndex, beatitudes, IM_ARRAYSIZE(beatitudes));
	ImGui::SetNextItemWidth(windowWidth * .25);
	ImGui::Combo("Status", &spawnItemStatusIndex, statuses, IM_ARRAYSIZE(statuses));

	ImGui::Separator();

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.t9GUI - DebugStats.t8Status).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("GUI ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.drawWorldT6 - DebugStats.drawWorldT1).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom1", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples1", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Draw ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.t6Messages - DebugStats.t5MainDraw).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom3", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples3", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("doFrames() ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.drawWorldT4 - DebugStats.drawWorldT3).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom3", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples3", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Raycast ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.drawWorldT5 - DebugStats.drawWorldT4).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom4", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples4", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Draw world ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.drawWorldT6 - DebugStats.drawWorldT5).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom5", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples5", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Draw entities ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = -1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.t4Music - DebugStats.t3SteamCallbacks).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom6", &plotYZoom, 1, 144);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples6", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Frame Time ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	{
		float milliseconds = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(DebugStats.drawWorldT3 - DebugStats.drawWorldT2).count();
		static int plotIndex = 0;
		static float plotValues[1000] = { 0.f };

		static float plotYZoom = 10.f;
		static int plotSamples = 50;
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderFloat("Y Zoom2", &plotYZoom, 1, 20);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth * .25);
		ImGui::SliderInt("Samples2", &plotSamples, 50, 1000);

		plotValues[plotIndex] = milliseconds;
		++plotIndex;
		if ( plotIndex >= plotSamples )
		{
			plotIndex = 0;
		}

		float average = 0.0f;
		int usefulSamples = plotSamples;
		for ( int n = 0; n < std::min(plotSamples, IM_ARRAYSIZE(plotValues)); n++ )
		{
			if ( plotValues[n] == 0.f )
			{
				--usefulSamples;
			}
			average += plotValues[n];
		}
		average /= (float)std::min(usefulSamples, IM_ARRAYSIZE(plotValues));

		char overlay[32];
		sprintf(overlay, "avg %.5fms", average);

		ImGui::PlotLines("Occlusion ms", plotValues, std::min(plotSamples, IM_ARRAYSIZE(plotValues)), 0, overlay, 0.f, plotYZoom, ImVec2(0, 50.f));
	}

	ImGui::End();
}
#endif
#endif

#ifndef EDITOR
std::map<int, std::vector<ShopkeeperConsumables_t::StoreSlots_t>> ShopkeeperConsumables_t::entries;
int ShopkeeperConsumables_t::consumableBuyValueMult = 100;
void ShopkeeperConsumables_t::readFromFile()
{
	const std::string filename = "data/shop_consumables.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("store_types") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	consumableBuyValueMult = 100;
	if ( d.HasMember("consumable_buy_value_multiplier") )
	{
		consumableBuyValueMult = d["consumable_buy_value_multiplier"].GetInt();
	}

	entries.clear();
	for ( auto shoptypes = d["store_types"].MemberBegin(); shoptypes != d["store_types"].MemberEnd(); ++shoptypes )
	{
		int shoptype = -1;
		const std::string shopname = shoptypes->name.GetString();
		if ( shopname == "arms_armor" )
		{
			shoptype = SHOP_TYPE_ARMS_ARMOR;
		}
		else if ( shopname == "hats" )
		{
			shoptype = SHOP_TYPE_HAT;
		}
		else if ( shopname == "jewelry" )
		{
			shoptype = SHOP_TYPE_JEWELRY;
		}
		else if ( shopname == "books" )
		{
			shoptype = SHOP_TYPE_BOOKS;
		}
		else if ( shopname == "potions" )
		{
			shoptype = SHOP_TYPE_POTIONS;
		}
		else if ( shopname == "staffs" )
		{
			shoptype = SHOP_TYPE_STAFFS;
		}
		else if ( shopname == "food" )
		{
			shoptype = SHOP_TYPE_FOOD;
		}
		else if ( shopname == "hardware" )
		{
			shoptype = SHOP_TYPE_HARDWARE;
		}
		else if ( shopname == "hunting" )
		{
			shoptype = SHOP_TYPE_HUNTING;
		}
		else if ( shopname == "general" )
		{
			shoptype = SHOP_TYPE_GENERAL;
		}
		if ( shoptype == -1 )
		{
			continue;
		}

		if ( !shoptypes->value.HasMember("slots") )
		{
			continue;
		}

		for ( auto slots = shoptypes->value["slots"].MemberBegin(); slots != shoptypes->value["slots"].MemberEnd(); ++slots )
		{
			auto& slot = slots->value;
			int tradeRequirement = slot["trading_req"].GetInt();

			auto& slotItems = slot["items"];

			entries[shoptype].push_back(StoreSlots_t());
			auto& storeSlotData = entries[shoptype].at(entries[shoptype].size() - 1);

			storeSlotData.slotTradingReq = tradeRequirement;
			for ( auto slot_itr = slotItems.Begin(); slot_itr != slotItems.End(); ++slot_itr )
			{
				storeSlotData.itemEntries.push_back(ItemEntry());
				auto& itemEntry = storeSlotData.itemEntries.at(storeSlotData.itemEntries.size() - 1);

				{
					auto& member = (*slot_itr)["type"];
					bool isArr = member.IsArray();
					std::vector<std::string> strings;
					if ( !isArr )
					{
						strings.push_back(member.GetString());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							strings.push_back(arr->GetString());
						}
					}
					for ( auto& s : strings )
					{
						if ( s == "empty" )
						{
							itemEntry.type.clear();
							break;
						}
						bool found = false;
						for ( int i = 0; i < NUMITEMS; ++i )
						{
							if ( s.compare(itemNameStrings[i + 2]) == 0 )
							{
								itemEntry.type.push_back(static_cast<ItemType>(i));
								found = true;
								break;
							}
						}
						assert(found);
					}
				}
				if ( itemEntry.type.empty() )
				{
					itemEntry.emptyItemEntry = true;
				}
				{
					auto& member = (*slot_itr)["status"];
					bool isArr = member.IsArray();
					std::vector<std::string> strings;
					if ( !isArr )
					{
						strings.push_back(member.GetString());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							strings.push_back(arr->GetString());
						}
					}
					for ( auto& s : strings )
					{
						if ( s == "broken" )
						{
							itemEntry.status.push_back(BROKEN);
						}
						else if ( s == "decrepit" )
						{
							itemEntry.status.push_back(DECREPIT);
						}
						else if ( s == "worn" )
						{
							itemEntry.status.push_back(WORN);
						}
						else if ( s == "serviceable" )
						{
							itemEntry.status.push_back(SERVICABLE);
						}
						else if ( s == "excellent" )
						{
							itemEntry.status.push_back(EXCELLENT);
						}
					}
				}
				{
					auto& member = (*slot_itr)["beatitude"];
					bool isArr = member.IsArray();
					std::vector<int> ints;
					if ( !isArr )
					{
						ints.push_back(member.GetInt());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							ints.push_back(arr->GetInt());
						}
					}
					for ( auto& i : ints )
					{
						itemEntry.beatitude.push_back(i);
					}
				}
				{
					auto& member = (*slot_itr)["count"];
					bool isArr = member.IsArray();
					std::vector<int> ints;
					if ( !isArr )
					{
						ints.push_back(member.GetInt());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							ints.push_back(arr->GetInt());
						}
					}
					for ( auto& i : ints )
					{
						itemEntry.count.push_back(i);
					}
				}
				{
					auto& member = (*slot_itr)["appearance"];
					bool isArr = member.IsArray();
					std::vector<Uint32> ints;
					if ( !member.IsString() )
					{
						if ( !isArr )
						{
							ints.push_back(member.GetUint());
						}
						else
						{
							for ( auto arr = member.Begin(); arr != member.End(); ++arr )
							{
								ints.push_back(arr->GetUint());
							}
						}
						for ( auto& i : ints )
						{
							itemEntry.appearance.push_back(i);
						}
					}
				}
				{
					auto& member = (*slot_itr)["identified"];
					bool isArr = member.IsArray();
					std::vector<bool> bools;
					if ( !isArr )
					{
						bools.push_back(member.GetBool());
					}
					else
					{
						for ( auto arr = member.Begin(); arr != member.End(); ++arr )
						{
							bools.push_back(arr->GetBool());
						}
					}
					for ( auto b : bools )
					{
						itemEntry.identified.push_back(b);
					}
				}
				itemEntry.percentChance = (*slot_itr)["spawn_percent_chance"].GetInt();
				itemEntry.dropChance = (*slot_itr)["drop_percent_chance"].GetInt();
				itemEntry.weightedChance = (*slot_itr)["slot_weighted_chance"].GetInt();
			}
		}
	}

	printlog("[JSON]: Successfully read json file %s, processed %d shop consumables", inputPath.c_str(), entries.size());
}


ClassHotbarConfig_t::ClassHotbar_t ClassHotbarConfig_t::ClassHotbarsDefault[NUMCLASSES];
ClassHotbarConfig_t::ClassHotbar_t ClassHotbarConfig_t::ClassHotbars[NUMCLASSES];

void ClassHotbarConfig_t::writeToFile(HotbarConfigType fileWriteType, HotbarConfigWriteMode writeMode)
{
	std::string outputDir = "/config/";
	if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		outputDir = "/data/";
	}

	if ( !PHYSFS_getRealDir(outputDir.c_str()) )
	{
		printlog("[JSON]: ClassHotbarConfig_t: %s directory not found", outputDir.c_str());
		return;
	}
	std::string outputPath = PHYSFS_getRealDir(outputDir.c_str());
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "config/class_hotbars.json";
	if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		fileName = "data/class_hotbars.json";
	}
	outputPath.append(fileName.c_str());

	rapidjson::Document exportDocument;
	bool writeNewFile = true;
	if ( fileWriteType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		File* fp = FileIO::open(outputPath.c_str(), "rb");
		if ( !fp )
		{
			if ( writeMode == HOTBAR_CONFIG_DELETE )
			{
				printlog("[JSON]: Could not locate json file %s, skipping deletion...", outputPath.c_str());
				return;
			}
			else
			{
				printlog("[JSON]: Could not locate json file %s, creating new file...", outputPath.c_str());
				fp = FileIO::open(outputPath.c_str(), "wb");
				if ( !fp )
				{
					printlog("[JSON]: Error opening json file %s for write!", outputPath.c_str());
					return;
				}
				exportDocument.SetObject();
			}
		}
		else
		{
			char buf[80000];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			exportDocument.ParseStream(is);
			printlog("[JSON]: Loaded existing file %s", outputPath.c_str());
			writeNewFile = false;
		}
	}
	else
	{
		exportDocument.SetObject();
	}

	const int VERSION = 1;

	if ( fileWriteType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		std::string classname = playerClassInternalNames[client_classes[clientnum]];
		if ( writeNewFile )
		{
			CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
			rapidjson::Value allClassesObject(rapidjson::kObjectType);
			CustomHelpers::addMemberToRoot(exportDocument, "classes", allClassesObject);
		}
		else
		{
			exportDocument["version"].SetInt(VERSION);
		}

		if ( !exportDocument["classes"].HasMember(classname.c_str()) )
		{
			if ( writeMode == HOTBAR_CONFIG_DELETE )
			{
				printlog("[JSON]: Custom layout not found for class '%s', skipping deletion...", classname.c_str());
				return;
			}
			exportDocument["classes"].AddMember(rapidjson::Value(classname.c_str(), exportDocument.GetAllocator()),
				rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
		}

		if ( writeMode == HOTBAR_CONFIG_DELETE )
		{
			printlog("[JSON]: Custom layout found for class '%s', removing...", classname.c_str());
			exportDocument["classes"].EraseMember(classname.c_str());
		}
		else
		{
			auto& hotbar_t = players[clientnum]->hotbar;
			std::string layoutname = "classic";
			if ( hotbar_t.useHotbarFaceMenu )
			{
				layoutname = "modern";
			}
			if ( !exportDocument["classes"][classname.c_str()].HasMember(layoutname.c_str()) )
			{
				exportDocument["classes"][classname.c_str()].AddMember(rapidjson::Value(layoutname.c_str(), exportDocument.GetAllocator()),
					rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
			}

			auto& layoutObj = exportDocument["classes"][classname.c_str()][layoutname.c_str()];
			for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
			{
				std::string slotnum = std::to_string(i);
				if ( !layoutObj.HasMember(slotnum.c_str()) )
				{
					layoutObj.AddMember(rapidjson::Value(slotnum.c_str(), exportDocument.GetAllocator()),
						rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
				}

				auto& slot = layoutObj[slotnum.c_str()];
				if ( !slot.HasMember("items") )
				{
					slot.AddMember("items", rapidjson::Value(rapidjson::kArrayType), exportDocument.GetAllocator());
				}
				else
				{
					slot["items"].Clear(); // overwrite new values in array
				}
				//slot.AddMember("categories", rapidjson::Value(rapidjson::kArrayType), exportDocument.GetAllocator());
				if ( hotbar_t.slots()[i].item != 0 )
				{
					if ( Item* item = uidToItem(hotbar_t.slots()[i].item) )
					{
						if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
						{
							std::string itemstr = itemNameStrings[item->type + 2];
							if ( itemstr == "spell_item" )
							{
								if ( spell_t* spell = getSpellFromItem(clientnum, item, false) )
								{
									itemstr = ItemTooltips.spellItems[spell->ID].internalName;
								}
								else
								{
									continue;
								}
							}
							rapidjson::Value itemnamekey(itemstr.c_str(), exportDocument.GetAllocator());
							slot["items"].PushBack(itemnamekey, exportDocument.GetAllocator());
						}
					}
				}
			}
		}
	}
	else if ( fileWriteType == HOTBAR_LAYOUT_DEFAULT_CONFIG )
	{
		CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
		rapidjson::Value allClassesObject(rapidjson::kObjectType);
		CustomHelpers::addMemberToRoot(exportDocument, "classes", allClassesObject);

		int classIndex = -1;
		for ( auto classname : playerClassInternalNames )
		{
			++classIndex;
			rapidjson::Value classObj(rapidjson::kObjectType);
			classObj.AddMember("classic", rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
			classObj.AddMember("modern", rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());

			auto& hotbar_t = players[clientnum]->hotbar;

			std::vector<std::string> layoutTypes = { "classic", "modern" };
			for ( auto layout : layoutTypes )
			{
				if ( layout == "classic" )
				{
					hotbar_t.useHotbarFaceMenu = false;
				}
				else
				{
					hotbar_t.useHotbarFaceMenu = true;
				}
				stats[clientnum]->clearStats();
				client_classes[clientnum] = classIndex;
				initClass(clientnum);

				for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
				{
					std::string slotnum = std::to_string(i);
					classObj[layout.c_str()].AddMember(rapidjson::Value(slotnum.c_str(), exportDocument.GetAllocator()),
						rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());

					auto& slot = classObj[layout.c_str()][slotnum.c_str()];
					slot.AddMember("items", rapidjson::Value(rapidjson::kArrayType), exportDocument.GetAllocator());
					//slot.AddMember("categories", rapidjson::Value(rapidjson::kArrayType), exportDocument.GetAllocator());
					if ( hotbar_t.slots()[i].item != 0 )
					{
						if ( Item* item = uidToItem(hotbar_t.slots()[i].item) )
						{
							if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
							{
								std::string itemstr = itemNameStrings[item->type + 2];
								if ( itemstr == "spell_item" )
								{
									if ( spell_t* spell = getSpellFromItem(clientnum, item, false) )
									{
										itemstr = ItemTooltips.spellItems[spell->ID].internalName;
									}
									else
									{
										continue;
									}
								}
								rapidjson::Value itemnamekey(itemstr.c_str(), exportDocument.GetAllocator());
								slot["items"].PushBack(itemnamekey, exportDocument.GetAllocator());
							}
						}
					}
				}
			}
			CustomHelpers::addMemberToSubkey(exportDocument, "classes", classname, classObj);
		}

		stats[clientnum]->clearStats();
		client_classes[clientnum] = CLASS_BARBARIAN;
		initClass(clientnum);
	}

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", outputPath.c_str());
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", outputPath.c_str());
	return;
}

void ClassHotbarConfig_t::readFromFile(ClassHotbarConfig_t::HotbarConfigType fileReadType)
{
	std::string filename = "data/class_hotbars.json";
	if ( fileReadType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
	{
		filename = "config/class_hotbars.json";
	}
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		if ( fileReadType == HOTBAR_LAYOUT_CUSTOM_CONFIG )
		{
			printlog("[JSON]: Notice: No custom class hotbar layout found '%s'", filename.c_str());
		}
		else
		{
			printlog("[JSON]: Error: Could not find json file %s", filename.c_str());
		}
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	char buf[80000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("classes") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto classes = d["classes"].MemberBegin(); classes != d["classes"].MemberEnd(); ++classes )
	{
		int classIndex = -1;
		for ( auto s : playerClassInternalNames )
		{
			++classIndex;
			if ( s == classes->name.GetString() )
			{
				break;
			}
		}
		if ( !(classIndex >= CLASS_BARBARIAN && classIndex < NUMCLASSES) )
		{
			continue;
		}

		for ( auto layout = classes->value.MemberBegin(); layout != classes->value.MemberEnd(); ++layout )
		{
			if ( strcmp(layout->name.GetString(), "classic") && strcmp(layout->name.GetString(), "modern") )
			{
				continue;
			}
			bool facebarLayout = false;
			if ( !strcmp(layout->name.GetString(), "modern") )
			{
				facebarLayout = true;
			}

			auto& customOrDefaultHotbar = (fileReadType == HOTBAR_LAYOUT_DEFAULT_CONFIG) ? ClassHotbarsDefault[classIndex] : ClassHotbars[classIndex];
			auto& classHotbar = facebarLayout ? customOrDefaultHotbar.layoutModern : customOrDefaultHotbar.layoutClassic;
			classHotbar.hasData = true;
			for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
			{
				std::string slotnum = std::to_string(i);
				auto& slot = layout->value[slotnum.c_str()];
				if ( slot.HasMember("items") )
				{
					if ( slot["items"].IsArray() )
					{
						for ( auto itemArr = slot["items"].Begin(); itemArr != slot["items"].End(); ++itemArr )
						{
							std::string itemString = itemArr->GetString();
							int itemType = WOODEN_SHIELD;
							bool found = false;
							bool spell = false;
							if ( itemString.find("spell_") != std::string::npos )
							{
								spell = true;
								for ( int spellID = 0; spellID < NUM_SPELLS; ++spellID )
								{
									if ( ItemTooltips.spellItems[spellID].internalName == itemString )
									{
										itemType = spellID + 10000; // special id offset
										found = true;
										break;
									}
								}
							}
							else
							{
								for ( int c = 0; c < NUMITEMS; ++c )
								{
									if ( itemString.compare(itemNameStrings[c + 2]) == 0 )
									{
										itemType = c;
										found = true;
										break;
									}
								}
							}
							if ( found )
							{
								auto findVal = std::find(classHotbar.hotbar[i].itemTypes.begin(), classHotbar.hotbar[i].itemTypes.end(),
									itemType);
								if ( findVal == classHotbar.hotbar[i].itemTypes.end() )
								{
									classHotbar.hotbar[i].itemTypes.push_back(itemType);
								}
								else
								{
									*findVal = itemType;
								}
							}
						}
					}
				}
			}
		}
	}
	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
}

void ClassHotbarConfig_t::ClassHotbar_t::ClassHotbarLayout_t::init()
{
	hasData = false;
	hotbar.clear();
	hotbar_alternates.clear();

	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		hotbar.push_back(HotbarEntry_t(i));
	}
	for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
	{
		std::vector<HotbarEntry_t> althotbar;
		for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
		{
			althotbar.push_back(HotbarEntry_t(i));
		}
		hotbar_alternates.push_back(althotbar);
	}
}

void ClassHotbarConfig_t::init()
{
	for ( int c = 0; c < NUMCLASSES; ++c )
	{
		auto& classHotbar = ClassHotbars[c];
		auto& classHotbarDefault = ClassHotbarsDefault[c];
		classHotbar.layoutClassic.init();
		classHotbar.layoutModern.init();
		classHotbarDefault.layoutClassic.init();
		classHotbarDefault.layoutModern.init();
	}

	readFromFile(HOTBAR_LAYOUT_DEFAULT_CONFIG);
	readFromFile(HOTBAR_LAYOUT_CUSTOM_CONFIG);
}

void ClassHotbarConfig_t::assignHotbarSlots(const int player)
{
	int classnum = client_classes[player];
	auto& layoutDefault = players[player]->hotbar.useHotbarFaceMenu ? ClassHotbarsDefault[classnum].layoutModern : ClassHotbarsDefault[classnum].layoutClassic;
	auto& layoutCustom = players[player]->hotbar.useHotbarFaceMenu ? ClassHotbars[classnum].layoutModern : ClassHotbars[classnum].layoutClassic;

	auto& hotbar_t = players[player]->hotbar;
	for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
	{
		hotbar_t.slots()[i].item = 0;
		hotbar_t.slots()[i].resetLastItem();
	}

	std::vector<std::pair<int, HotbarEntry_t*>> itemsAndSlots;

	for ( auto& slot : layoutDefault.hotbar )
	{
		if ( !slot.itemTypes.empty() )
		{
			for ( auto itemType : slot.itemTypes )
			{
				auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
					[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
				if ( it == itemsAndSlots.end() )
				{
					itemsAndSlots.push_back(std::make_pair(itemType, &slot));
				}
				else
				{
					// update existing entry
					it->second = &slot;
				}
			}
		}
	}
	if ( layoutCustom.hasData )
	{
		printlog("[Class Hotbar]: Found custom layout for class '%s'", playerClassInternalNames[classnum].c_str());
		itemsAndSlots.clear();
		for ( auto& slot : layoutCustom.hotbar )
		{
			if ( !slot.itemTypes.empty() )
			{
				for ( auto itemType : slot.itemTypes )
				{
					auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
						[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
					if ( it == itemsAndSlots.end() )
					{
						itemsAndSlots.push_back(std::make_pair(itemType, &slot));
					}
					else
					{
						// update existing entry
						it->second = &slot;
					}
				}
			}
		}
	}

	struct MatchingItem_t
	{
		Item* item = nullptr;
		int slotnum = -1;
		MatchingItem_t(Item* _item, const int _slotnum) :
			item(_item),
			slotnum(_slotnum)
		{};
		MatchingItem_t() {};
	};
	std::map<int, MatchingItem_t> matchingItems;
	for ( node_t* node = stats[player]->inventory.first; node != nullptr; node = node->next )
	{
		Item* item = static_cast<Item*>(node->element);
		if ( item )
		{
			int itemType = item->type;
			if ( itemCategory(item) == SPELL_CAT )
			{
				if ( item->appearance >= 1000 )
				{
					continue; // shaman form spells
				}
				if ( spell_t* spell = getSpellFromItem(player, item, false) )
				{
					itemType = spell->ID + 10000;
				}
				else
				{
					continue;
				}
			}
			auto it = std::find_if(itemsAndSlots.begin(), itemsAndSlots.end(),
				[itemType](const std::pair<int, HotbarEntry_t*>& element) { return element.first == itemType; });
			if ( it != itemsAndSlots.end() )
			{
				// store inventory items in a lookup table
				matchingItems[itemType] = MatchingItem_t(item, it->second->slotnum);
			}
		}
	}

	for ( auto& itemAndSlot : itemsAndSlots )
	{
		// go through each slot, and each item. if item found, place it in hotbar slot
		// if multiple items per slot, last item will override the slot
		if ( matchingItems.find(itemAndSlot.first) != matchingItems.end() )
		{
			if ( matchingItems[itemAndSlot.first].item )
			{
				hotbar_t.slots()[matchingItems[itemAndSlot.first].slotnum].item = matchingItems[itemAndSlot.first].item->uid;
			}
		}
	}
}

LocalAchievements_t LocalAchievements;

void LocalAchievements_t::readFromFile()
{
	LocalAchievements.init();
	Compendium_t::AchievementData_t::achievementsNeedFirstData = false;

	char path[PATH_MAX] = "";
	completePath(path, "savegames/achievements.json", outputdir);

	File* fp = FileIO::open(path, "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", path);
		return;
	}

	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("achievements") || !d.HasMember("statistics") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", path);
		return;
	}

	for ( auto achievement = d["achievements"].MemberBegin(); achievement != d["achievements"].MemberEnd(); ++achievement )
	{
		auto& ach = LocalAchievements.achievements[achievement->name.GetString()];
		ach.name = achievement->name.GetString();
		ach.unlocked = achievement->value["unlocked"].GetBool();
		ach.unlockTime = achievement->value["unlock_time"].GetInt64();

		auto find = Compendium_t::achievements.find(achievement->name.GetString());
		if ( find != Compendium_t::achievements.end() )
		{
			auto& achData = find->second;
			achData.unlocked = ach.unlocked;
			achData.unlockTime = ach.unlockTime;
			if ( ach.unlocked )
			{
				Compendium_t::AchievementData_t::achievementUnlockedLookup.insert(ach.name);
			}
		}
	}

	for ( auto statistic = d["statistics"].MemberBegin(); statistic != d["statistics"].MemberEnd(); ++statistic )
	{
		std::string statStr = statistic->name.GetString();
		const int statNum = stoi(statStr);
		auto& stat = LocalAchievements.statistics[statNum];
		stat.value = statistic->value["progress"].GetInt();
	}

	for ( int statNum = 0; statNum < NUM_STEAM_STATISTICS; ++statNum )
	{
		g_SteamStats[statNum].m_iValue = LocalAchievements.statistics[statNum].value;
	}
	sortAchievementsForDisplay();
}

void LocalAchievements_t::writeToFile()
{
	char path[PATH_MAX] = "";
	completePath(path, "savegames/achievements.json", outputdir);

	rapidjson::Document exportDocument;
	exportDocument.SetObject();

	const int VERSION = 1;

	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
	rapidjson::Value allAchObj(rapidjson::kObjectType);
	for ( auto& ach : Compendium_t::achievements )
	{
		if ( LocalAchievements.achievements.find(ach.first) == LocalAchievements.achievements.end() )
		{
			continue;
		}
		auto& achData = LocalAchievements.achievements[ach.first];

		rapidjson::Value namekey(ach.first.c_str(), exportDocument.GetAllocator());
		allAchObj.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
		auto& obj = allAchObj[ach.first.c_str()];
		obj.AddMember("unlocked", achData.unlocked, exportDocument.GetAllocator());
		obj.AddMember("unlock_time", achData.unlockTime, exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "achievements", allAchObj);

	rapidjson::Value allStatObj(rapidjson::kObjectType);
	for ( int i = 0; i < NUM_STEAM_STATISTICS; ++i )
	{
		if ( LocalAchievements.statistics.find(i) == LocalAchievements.statistics.end() )
		{
			continue;
		}
		auto& statData = LocalAchievements.statistics[i];

		std::string statNum = std::to_string(i);
		rapidjson::Value namekey(statNum.c_str(), exportDocument.GetAllocator());
		allStatObj.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
		auto& obj = allStatObj[statNum.c_str()];
		obj.AddMember("progress", statData.value, exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "statistics", allStatObj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

void LocalAchievements_t::init()
{
	LocalAchievements.achievements.clear();
	for ( auto& ach : Compendium_t::achievements )
	{
		LocalAchievements.achievements[ach.first].unlocked = false;
		LocalAchievements.achievements[ach.first].unlockTime = 0;
		LocalAchievements.achievements[ach.first].name = ach.first;
	}
	LocalAchievements.statistics.clear();
	for ( int i = 0; i < NUM_STEAM_STATISTICS; ++i )
	{
		LocalAchievements.statistics[i].value = 0;
	}
}

void LocalAchievements_t::updateAchievement(const char* name, const bool unlocked)
{
	if ( achievements.find(name) != achievements.end() )
	{
		auto& ach = achievements[name];
		bool oldUnlocked = ach.unlocked;
		ach.unlocked = unlocked;
		if ( ach.unlocked && !oldUnlocked )
		{
			auto t = getTime();
			ach.unlockTime = t;

			UIToastNotificationManager.createAchievementNotification(name);
		}
	}
}

void LocalAchievements_t::updateStatistic(const int stat_num, const int value)
{
	if ( statistics.find(stat_num) != statistics.end() )
	{
		auto& stat = statistics[stat_num];
		stat.value = value;
	}
}

GameplayPreferences_t gameplayPreferences[MAXPLAYERS];

void GameplayPreferences_t::GameplayPreference_t::set(const int _value)
{
	if ( value != _value )
	{
		needsUpdate = true;
	}
	value = _value;
}

void GameplayPreferences_t::requestUpdateFromClient()
{
	if ( player == 0 ) { return; }
	if ( client_disconnected[player] ) { return; }
	if ( !net_packet )
	{
		return;

	}
	strcpy((char*)net_packet->data, "GPPU");
	net_packet->data[4] = (Uint8)player;
	net_packet->address.host = net_clients[player - 1].host;
	net_packet->address.port = net_clients[player - 1].port;
	net_packet->len = 5;
	sendPacketSafe(net_sock, -1, net_packet, player - 1);
}

void GameplayPreferences_t::sendToClients(const int targetPlayer)
{
	if ( targetPlayer == 0 ) { return; }
	if ( client_disconnected[targetPlayer] ) { return; }
	if ( !net_packet )
	{
		return;
	}

	strcpy((char*)net_packet->data, "GPPR");
	net_packet->data[4] = (Uint8)player;
	net_packet->data[5] = (Uint8)GPREF_ENUM_END;
	int index = 0;
	for ( auto& pref : preferences )
	{
		Uint8 data = (pref.value & 0xFF);
		net_packet->data[6 + index] = data;
		++index;
	}
	net_packet->address.host = net_clients[targetPlayer - 1].host;
	net_packet->address.port = net_clients[targetPlayer - 1].port;
	net_packet->len = 6 + index;
	sendPacketSafe(net_sock, -1, net_packet, targetPlayer - 1);
}

void GameplayPreferences_t::receivePacket()
{
	if ( !net_packet )
	{
		return;
	}
	int player = (Uint8)net_packet->data[4];
	if ( player >= 0 && player < MAXPLAYERS )
	{
		auto& playerPrefs = gameplayPreferences[player];
		const int numPrefs = (Uint8)net_packet->data[5];
		for ( int i = 0; i < numPrefs && i < GPREF_ENUM_END; ++i )
		{
			int data = (net_packet->data[6 + i] & 0xFF);
			playerPrefs.preferences[i].value = data;
			playerPrefs.preferences[i].needsUpdate = false;
			//messagePlayer(clientnum, MESSAGE_DEBUG, "%d rcv: %d : %d", player, i, playerPrefs.preferences[i].value);
		}
		playerPrefs.lastUpdateTick = ticks;
	}
}

void GameplayPreferences_t::sendToServer()
{
	if ( multiplayer != CLIENT ) { return; }
	if ( !net_packet )
	{
		return;
	}

	strcpy((char*)net_packet->data, "GPPR");
	net_packet->data[4] = (Uint8)player;
	net_packet->data[5] = (Uint8)GPREF_ENUM_END;
	int index = 0;
	for ( auto& pref : preferences )
	{
		Uint8 data = (pref.value & 0xFF);
		net_packet->data[6 + index] = data;
		++index;
		pref.needsUpdate = false;
	}
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 6 + index;
	sendPacketSafe(net_sock, -1, net_packet, 0);
}

void GameplayPreferences_t::process()
{
	if ( player < 0 )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( &gameplayPreferences[i] == this )
			{
				player = i;
				break;
			}
		}
	}

	if ( players[player]->isLocalPlayer() )
	{
		int index = 0;
		for ( auto& pref : preferences )
		{
			switch ( index )
			{
				case GPREF_ARACHNOPHOBIA:
					pref.set(MainMenu::arachnophobia_filter ? 1 : 0);
					break;
				case GPREF_COLORBLIND:
					pref.set(colorblind ? 1 : 0);
				default:
					break;
			}
			++index;
		}
	}

	if ( multiplayer == CLIENT )
	{
		bool doUpdate = false;
		if ( players[player]->isLocalPlayer() )
		{
			for ( auto& pref : preferences )
			{
				if ( pref.needsUpdate )
				{
					doUpdate = true;
					pref.needsUpdate = false;
				}
			}
			if ( ticks - lastUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
			{
				doUpdate = true;
			}
			if ( doUpdate )
			{
				sendToServer();
			}
			isInit = true;
		}
		if ( doUpdate )
		{
			lastUpdateTick = ticks;
		}
	}
	else if ( multiplayer == SERVER )
	{
		bool doUpdate = false;
		if ( players[player]->isLocalPlayer() )
		{
			isInit = true;
			/*for ( auto& pref : preferences )
			{
			if ( pref.needsUpdate )
			{
			doUpdate = true;
			pref.needsUpdate = false;
			}
			}

			if ( ticks - lastUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
			{
			doUpdate = true;
			}

			if ( doUpdate )
			{
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
			if ( !players[i]->isLocalPlayer() )
			{
			sendToClients(i);
			}
			}
			}*/
		}
		else
		{
			if ( !client_disconnected[player] )
			{
				isInit = true;
				for ( auto& pref : preferences )
				{
					if ( pref.needsUpdate )
					{
						doUpdate = true;
						pref.needsUpdate = false;
					}
				}

				if ( ticks - lastUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
				{
					doUpdate = true;
				}

				if ( doUpdate )
				{
					requestUpdateFromClient();
				}
			}
			else
			{
				if ( isInit )
				{
					for ( auto& pref : preferences )
					{
						pref.reset();
					}
					isInit = false;
				}
			}
		}

		if ( doUpdate )
		{
			lastUpdateTick = ticks;
		}
	}

	if ( multiplayer != CLIENT && player == clientnum )
	{
		isInit = true;
		GameplayPreferences_t::serverProcessGameConfig();
	}
}

GameplayPreferences_t::GameplayPreference_t GameplayPreferences_t::gameConfig[GameplayPreferences_t::GOPT_ENUM_END];
Uint32 GameplayPreferences_t::lastGameConfigUpdateTick = 0;
void GameplayPreferences_t::reset()
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		for ( auto& pref : gameplayPreferences[i].preferences )
		{
			pref.reset();
		}
		gameplayPreferences[i].lastUpdateTick = 0;
		gameplayPreferences[i].isInit = false;
	}
	for ( auto& conf : gameConfig )
	{
		conf.reset();
	}
	lastGameConfigUpdateTick = 0;
}
void GameplayPreferences_t::serverUpdateGameConfig()
{
	lastGameConfigUpdateTick = ticks;
	if ( !net_packet )
	{
		return;
	}
	for ( int i = 1; i < MAXPLAYERS; ++i )
	{
		if ( !players[i]->isLocalPlayer() && !client_disconnected[i] )
		{
			strcpy((char*)net_packet->data, "GOPT");
			net_packet->data[4] = (Uint8)GOPT_ENUM_END;
			int index = 0;
			for ( auto& conf : gameConfig )
			{
				Uint8 data = (conf.value & 0xFF);
				net_packet->data[5 + index] = data;
				++index;
			}
			net_packet->address.host = net_clients[i - 1].host;
			net_packet->address.port = net_clients[i - 1].port;
			net_packet->len = 5 + index;
			sendPacketSafe(net_sock, -1, net_packet, i - 1);
		}
	}
}

void GameplayPreferences_t::receiveGameConfig()
{
	if ( !net_packet ) { return; }
	auto& gameConfig = GameplayPreferences_t::gameConfig;
	const int numConfigs = (Uint8)net_packet->data[4];
	for ( int i = 0; i < numConfigs && i < GOPT_ENUM_END; ++i )
	{
		int data = (net_packet->data[5 + i] & 0xFF);
		gameConfig[i].value = data;
		gameConfig[i].needsUpdate = false;
		//messagePlayer(clientnum, MESSAGE_DEBUG, "GOPT %d rcv: %d", i, gameConfig[i].value);
	}
	lastGameConfigUpdateTick = ticks;
}

void GameplayPreferences_t::serverProcessGameConfig()
{
	bool doUpdate = false;
	for ( int pref = 0; pref < GOPT_ENUM_END; ++pref )
	{
		int value = 0;
		switch ( pref )
		{
			case GOPT_ARACHNOPHOBIA:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						if ( gameplayPreferences[i].preferences[GPREF_ARACHNOPHOBIA].value != 0 )
						{
							value = 1;
						}
					}
				}
				if ( value != oldValue )
				{
					if ( multiplayer == SERVER )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( !client_disconnected[i] )
							{
								if ( value != 0 )
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4333));
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4334));
								}
							}
						}
					}
				}
				break;
			}
			case GOPT_COLORBLIND:
			{
				int oldValue = getGameConfigValue(GameConfigIndexes(pref));
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && gameplayPreferences[i].isInit )
					{
						if ( gameplayPreferences[i].preferences[GPREF_COLORBLIND].value != 0 )
						{
							value = 1;
						}
					}
				}
				if ( value != oldValue )
				{
					if ( multiplayer == SERVER )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( !client_disconnected[i] )
							{
								if ( value != 0 )
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4342));
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, Language::get(4343));
								}
							}
						}
					}
				}
				break;
			}
			default:
				break;
		}
		gameConfig[pref].set(value);
		if ( gameConfig[pref].needsUpdate )
		{
			doUpdate = true;
		}
		gameConfig[pref].needsUpdate = false;
	}

	if ( ticks - lastGameConfigUpdateTick > ((intro ? TICKS_PER_SECOND : (TICKS_PER_SECOND * 15)) + 5) )
	{
		doUpdate = true;
	}

	if ( doUpdate && multiplayer == SERVER )
	{
		serverUpdateGameConfig();
	}

	if ( doUpdate )
	{
		lastGameConfigUpdateTick = ticks;
	}
}
#endif // !EDITOR

EditorEntityData_t editorEntityData;
std::map<int, EditorEntityData_t::EntityColliderData_t> EditorEntityData_t::colliderData;
std::map<std::string, EditorEntityData_t::ColliderDmgProperties_t> EditorEntityData_t::colliderDmgTypes;
std::map<std::string, std::map<int, int>> EditorEntityData_t::colliderRandomGenPool;
void EditorEntityData_t::readFromFile()
{
	const std::string filename = "data/entity_data.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("entities") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	colliderData.clear();
	colliderDmgTypes.clear();
	colliderRandomGenPool.clear();
	auto& entityTypes = d["entities"];
	if ( entityTypes.HasMember("collider_dmg_calcs") )
	{
		for ( auto itr = entityTypes["collider_dmg_calcs"].MemberBegin(); itr != entityTypes["collider_dmg_calcs"].MemberEnd();
			++itr )
		{
			auto& colliderDmg = colliderDmgTypes[itr->name.GetString()];
			colliderDmg.burnable = itr->value["burnable"].GetBool();
			colliderDmg.minotaurPathThroughAndBreak = itr->value["minotaur_path_and_break"].GetBool();
			colliderDmg.meleeAffects = itr->value["melee"].GetBool();
			colliderDmg.magicAffects = itr->value["magic"].GetBool();
			colliderDmg.bombsAttach = itr->value["bombs_attach"].GetBool();
			colliderDmg.boulderDestroys = itr->value["boulder_destroy"].GetBool();
			colliderDmg.showAsWallOnMinimap = itr->value["minimap_appear_as_wall"].GetBool();
			if ( itr->value.HasMember("allow_npc_pathing") )
			{
				colliderDmg.allowNPCPathing = itr->value["allow_npc_pathing"].GetBool();
			}
			if ( itr->value.HasMember("bonus_damage_skills") && itr->value["bonus_damage_skills"].IsArray() )
			{
				for ( auto itr2 = itr->value["bonus_damage_skills"].Begin(); itr2 != itr->value["bonus_damage_skills"].End(); ++itr2 )
				{
					std::string s = itr2->GetString();
					if ( s == "PRO_AXE" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_AXE);
					}
					else if ( s == "PRO_SWORD" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_SWORD);
					}
					else if ( s == "PRO_MACE" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_MACE);
					}
					else if ( s == "PRO_POLEARM" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_POLEARM);
					}
					else if ( s == "PRO_UNARMED" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_UNARMED);
					}
					else if ( s == "PRO_MAGIC" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_MAGIC);
					}
					else if ( s == "PRO_RANGED" )
					{
						colliderDmg.proficiencyBonusDamage.insert(PRO_RANGED);
					}
				}
			}
			if ( itr->value.HasMember("resist_damage_skills") && itr->value["resist_damage_skills"].IsArray() )
			{
				for ( auto itr2 = itr->value["resist_damage_skills"].Begin(); itr2 != itr->value["resist_damage_skills"].End(); ++itr2 )
				{
					std::string s = itr2->GetString();
					if ( s == "PRO_AXE" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_AXE);
					}
					else if ( s == "PRO_SWORD" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_SWORD);
					}
					else if ( s == "PRO_MACE" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_MACE);
					}
					else if ( s == "PRO_POLEARM" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_POLEARM);
					}
					else if ( s == "PRO_UNARMED" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_UNARMED);
					}
					else if ( s == "PRO_MAGIC" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_MAGIC);
					}
					else if ( s == "PRO_RANGED" )
					{
						colliderDmg.proficiencyResistDamage.insert(PRO_RANGED);
					}
				}
			}
		}
	}
	if ( entityTypes.HasMember("collider_dmg_types") )
	{
		for ( auto itr = entityTypes["collider_dmg_types"].MemberBegin(); itr != entityTypes["collider_dmg_types"].MemberEnd();
			++itr )
		{
			auto indexStr = itr->name.GetString();
			int index = std::stoi(indexStr);
			auto& collider = colliderData[index];
			collider.name = itr->value["name"].GetString();
			collider.gib = itr->value["gib_model"].GetInt();
			collider.gib_hit.clear();
			if ( itr->value.HasMember("gib_hit_model") )
			{
				if ( itr->value["gib_hit_model"].IsInt() )
				{
					collider.gib_hit.push_back(itr->value["gib_hit_model"].GetInt());
				}
				else if ( itr->value["gib_hit_model"].IsArray() )
				{
					for ( auto itr2 = itr->value["gib_hit_model"].Begin();
						itr2 != itr->value["gib_hit_model"].End(); ++itr2 )
					{
						if ( itr2->IsInt() )
						{
							collider.gib_hit.push_back(itr2->GetInt());
						}
					}
				}
			}
			if ( itr->value["sfx_break"].IsInt() )
			{
				collider.sfxBreak.push_back(itr->value["sfx_break"].GetInt());
			}
			else if ( itr->value["sfx_break"].IsArray() )
			{
				for ( auto itr2 = itr->value["sfx_break"].Begin();
					itr2 != itr->value["sfx_break"].End(); ++itr2 )
				{
					if ( itr2->IsInt() )
					{
						collider.sfxBreak.push_back(itr2->GetInt());
					}
				}
			}
			collider.sfxHit = itr->value["sfx_hit"].GetInt();
			collider.damageCalculationType = itr->value["damage_calc"].GetString();
			collider.entityLangEntry = itr->value["entity_lang_entry"].GetInt();
			collider.hitMessageLangEntry = itr->value["hit_message"].GetInt();
			collider.breakMessageLangEntry = itr->value["break_message"].GetInt();
			collider.hpbarLookupName = itr->value["hp_bar_lookup_name"].GetString();
			collider.hideMonsters.clear();
			if ( itr->value.HasMember("random_gen_pool") )
			{
				if ( itr->value["random_gen_pool"].IsObject() )
				{
					for ( auto itr2 = itr->value["random_gen_pool"].MemberBegin();
						itr2 != itr->value["random_gen_pool"].MemberEnd(); ++itr2 )
					{
						if ( itr2->name.IsString() )
						{
							EditorEntityData_t::colliderRandomGenPool[itr2->name.GetString()][index] =
								itr2->value.GetInt();
						}
					}
				}
			}
			if ( itr->value.HasMember("events") )
			{
				for ( auto itr2 = itr->value["events"].MemberBegin();
					itr2 != itr->value["events"].MemberEnd(); ++itr2 )
				{
					std::string mapname = itr2->name.GetString();
					for ( auto itr3 = itr2->value.MemberBegin();
						itr3 != itr2->value.MemberEnd(); ++itr3 )
					{
						if ( !strcmp(itr3->name.GetString(), "summon") )
						{
							auto& data = collider.hideMonsters[mapname];
							if ( itr3->value.IsArray() )
							{
								for ( auto val = itr3->value.Begin(); val != itr3->value.End(); ++val )
								{
									if ( val->IsString() )
									{
										for ( int i = 0; i < NUMMONSTERS; ++i )
										{
											if ( !strcmp(val->GetString(), monstertypename[i]) )
											{
												data.push_back(i);
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			collider.overrideProperties.clear();
			if ( itr->value.HasMember("override_editor_props") )
			{
				if ( itr->value["override_editor_props"].IsObject() )
				{
					for ( auto itr2 = itr->value["override_editor_props"].MemberBegin();
						itr2 != itr->value["override_editor_props"].MemberEnd(); ++itr2 )
					{
						if ( itr2->value.IsInt() )
						{
							collider.overrideProperties[itr2->name.GetString()] = itr2->value.GetInt();
						}
					}
				}
			}
		}
	}
}

std::vector<int> Mods::modelsListModifiedIndexes;
std::vector<int> Mods::soundsListModifiedIndexes;
std::vector<std::pair<SDL_Surface**, std::string>> Mods::systemResourceImagesToReload;
std::vector<std::pair<std::string, std::string>> Mods::mountedFilepaths;
std::vector<std::pair<std::string, std::string>> Mods::mountedFilepathsSaved;
std::set<std::string> Mods::mods_loaded_local;
std::set<std::string> Mods::mods_loaded_workshop;
std::list<std::string> Mods::localModFoldernames;
int Mods::numCurrentModsLoaded = -1;
bool Mods::modelsListRequiresReloadUnmodded = false;
bool Mods::soundListRequiresReloadUnmodded = false;
bool Mods::tileListRequireReloadUnmodded = false;
bool Mods::spriteImagesRequireReloadUnmodded = false;
bool Mods::booksRequireReloadUnmodded = false;
bool Mods::musicRequireReloadUnmodded = false;
bool Mods::langRequireReloadUnmodded = false;
bool Mods::monsterLimbsRequireReloadUnmodded = false;
bool Mods::systemImagesReloadUnmodded = false;
bool Mods::customContentLoadedFirstTime = false;
bool Mods::disableSteamAchievements = false;
bool Mods::lobbyDisableSteamAchievements = false;
bool Mods::isLoading = false;
void Mods::updateModCounts()
{
	mods_loaded_local.clear();
	mods_loaded_workshop.clear();
	for ( auto& mod : mountedFilepaths )
	{
		bool found = false;
		if ( mod.first.find("371970") != std::string::npos )
		{
			if ( mod.first.find("workshop") != std::string::npos )
			{
				if ( mod.first.find("content") != std::string::npos )
				{
					found = true;
					Mods::mods_loaded_workshop.insert(mod.first);
				}
			}
		}
		if ( !found )
		{
			Mods::mods_loaded_local.insert(mod.first);
		}
	}
}
#ifdef STEAMWORKS
std::vector<SteamUGCDetails_t*> Mods::workshopSubscribedItemList;
std::vector<std::pair<std::string, uint64>> Mods::workshopLoadedFileIDMap;
std::vector<Mods::WorkshopTags_t> Mods::tag_settings = {
	Mods::WorkshopTags_t("dungeons", "Dungeons"),
	Mods::WorkshopTags_t("textures", "Textures"),
	Mods::WorkshopTags_t("models", "Models"),
	Mods::WorkshopTags_t("gameplay", "Gameplay"),
	Mods::WorkshopTags_t("audio", "Audio"),
	Mods::WorkshopTags_t("misc", "Misc"),
	Mods::WorkshopTags_t("translations", "Translations")
};
int Mods::uploadStatus = 0;
int Mods::uploadErrorStatus = 0;
Uint32 Mods::uploadTicks = 0;
Uint32 Mods::processedOnTick = 0;
PublishedFileId_t Mods::uploadingExistingItem = 0;
int Mods::uploadNumRetries = 3;
bool Mods::forceDownloadCachedImages = false;

std::string Mods::getFolderFullPath(std::string input)
{
	if ( input == "" ) { return ""; }
#ifdef WINDOWS
#ifdef _UNICODE
	wchar_t pathbuffer[PATH_MAX];
	const int len1 = MultiByteToWideChar(CP_ACP, 0, input.c_str(), input.size() + 1, 0, 0);
	auto buf1 = new wchar_t[len1];
	MultiByteToWideChar(CP_ACP, 0, input.c_str(), input.size() + 1, buf1, len1);
	const int pathlen = GetFullPathNameW(buf1, PATH_MAX, pathbuffer, NULL);
	delete[] buf1;
	const int len2 = WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, 0, 0, 0, 0);
	auto buf2 = new char[len2];
	WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, buf2, len2, 0, 0);
	std::string fullpath = buf2;
#else
	char pathbuffer[PATH_MAX];
	GetFullPathNameA(input.c_str(), PATH_MAX, pathbuffer, NULL);
	std::string fullpath = pathbuffer;
#endif
#else
	char pathbuffer[PATH_MAX];
	realpath(input.c_str(), pathbuffer);
	std::string fullpath = pathbuffer;
#endif
	return fullpath;
}
#endif

#ifdef USE_LIBCURL
LibCURL_t LibCURL;

size_t LibCURL_t::write_data_fp(void* ptr, size_t size, size_t nmemb, File* stream) {
	size_t written = stream->write(ptr, size, nmemb);
	return written;
}

size_t LibCURL_t::write_data_string(void* ptr, size_t size, size_t nmemb, std::string* s) {
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)ptr, newLength);
	}
	catch ( std::bad_alloc& e )
	{
		return 0;
	}
	return newLength;
}

void LibCURL_t::download(std::string filename, std::string url)
{
	if ( !bInit )
	{
		init();
	}

	std::string content;
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	//curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);  // redirects
	//curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_string);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &content);

	std::string inputPath = PHYSFS_getRealDir("workshop_cache");
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append("workshop_cache");
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	// Grab image 
	auto result = curl_easy_perform(handle);
	if ( result != CURLE_OK )
	{
		printlog("[CURL]: Error: Could not get file %s", url.c_str());
	}
	else
	{
		char* type = nullptr;
		result = curl_easy_getinfo(handle, CURLINFO::CURLINFO_CONTENT_TYPE, &type);
		if ( result == CURLE_OK && type )
		{
			std::string contentType = type;
			if ( contentType.find("png") != std::string::npos )
			{
				inputPath.append(".png");
			}
			else if ( contentType.find("jpg") != std::string::npos )
			{
				//inputPath.append(".jpg");
				inputPath.append(".png"); // try always png?
			}
			else if ( contentType.find("jpeg") != std::string::npos )
			{
				//inputPath.append(".jpg");
				inputPath.append(".png"); // try always png?
			}
			else
			{
				printlog("[CURL]: Error: Content type was not jpg or png as expected: %s", contentType.c_str());
				return;
			}
		}
		else
		{
			printlog("[CURL]: Error: curl_easy_getinfo failed.");
			return;
		}
	}

	File* fp = FileIO::open(inputPath.c_str(), "wb");
	fp->write(content.c_str(), sizeof(char), content.size());
	if ( !fp )
	{
		printlog("[CURL]: Error: Could not open file %s", inputPath.c_str());
		return;
	}

	FileIO::close(fp);
}
#endif

bool Mods::verifyMapFiles(const char* folder, bool ignoreBaseFolder)
{
	std::map<std::string, int> newMapHashes;
	std::string fullpath;
	if ( !folder )
	{
		fullpath = "maps/";
	}
	else
	{
		fullpath = folder;
		fullpath += PHYSFS_getDirSeparator();
		fullpath += "maps/";
	}
	for ( auto f : directoryContents(fullpath.c_str(), false, true) )
	{
		const std::string mapPath = "maps/" + f;
		auto path = PHYSFS_getRealDir(mapPath.c_str());
		if ( path && ignoreBaseFolder && !strcmp(path, "./") )
		{
			continue;
		}

		map_t m;
		m.tiles = nullptr;
		m.entities = (list_t*)malloc(sizeof(list_t));
		m.entities->first = nullptr;
		m.entities->last = nullptr;
		m.creatures = new list_t;
		m.creatures->first = nullptr;
		m.creatures->last = nullptr;
		m.worldUI = new list_t;
		m.worldUI->first = nullptr;
		m.worldUI->last = nullptr;
		if ( path )
		{
			int maphash = 0;
			const std::string fullMapPath = path + (PHYSFS_getDirSeparator() + mapPath);
			int result = loadMap(fullMapPath.c_str(), &m, m.entities, m.creatures, &maphash);
			if ( result >= 0 ) {
				bool fileExistsInTable = false;
				if ( !verifyMapHash(fullMapPath.c_str(), maphash, &fileExistsInTable) )
				{
					if ( fileExistsInTable || strcmp(path, "./") ) 
					{
						// return false if map exists in map hash table, or if hash check failed and mod folder contains an unknown map
						return false;
					}
				}
			}
		}
		if ( m.entities ) {
			list_FreeAll(m.entities);
			free(m.entities);
		}
		if ( m.creatures ) {
			list_FreeAll(m.creatures);
			delete m.creatures;
		}
		if ( m.worldUI ) {
			list_FreeAll(m.worldUI);
			delete m.worldUI;
		}
		if ( m.tiles ) {
			free(m.tiles);
		}
	}
	return true;
}

void Mods::verifyAchievements(const char* fullpath, bool ignoreBaseFolder)
{
	if ( physfsIsMapLevelListModded() )
	{
		disableSteamAchievements = true;
	}

	if ( PHYSFS_getRealDir("/data/gameplaymodifiers.json") )
	{
		disableSteamAchievements = true;
	}
	else if ( PHYSFS_getRealDir("/data/monstercurve.json") )
	{
		disableSteamAchievements = true;
	}
	else if ( !verifyMapFiles(fullpath, ignoreBaseFolder) )
	{
		disableSteamAchievements = true;
	}
	if ( ItemTooltips_t::itemsJsonHashRead != ItemTooltips_t::kItemsJsonHash )
	{
		disableSteamAchievements = true;
	}
}

bool Mods::isPathInMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry
			return true;
		}
	}
	return false;
}

bool Mods::removePathFromMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry, remove from list.
#ifdef STEAMWORKS
			for ( std::vector<std::pair<std::string, uint64>>::iterator itId = Mods::workshopLoadedFileIDMap.begin();
				itId != Mods::workshopLoadedFileIDMap.end(); ++itId )
			{
				if ( itId->first.compare(line.second) == 0 )
				{
					Mods::workshopLoadedFileIDMap.erase(itId);
					break;
				}
			}
#endif // STEAMWORKS
			Mods::mountedFilepaths.erase(it);
			return true;
		}
	}
	return false;
}

bool Mods::clearAllMountedPaths()
{
	bool success = true;
	char** i;
	for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )
	{
        const std::string xmas = (std::string(datadir) + "/") + holidayThemeDirs[HolidayTheme::THEME_XMAS];
        const std::string halloween = (std::string(datadir) + "/") + holidayThemeDirs[HolidayTheme::THEME_HALLOWEEN];
    
		std::string line = *i;
		if (line.compare(outputdir) != 0 &&
            line.compare(datadir) != 0 &&
            line.compare(halloween) != 0 &&
            line.compare(xmas) != 0 &&
            line.compare("./") != 0) // don't unmount the base directories
		{
			if ( PHYSFS_unmount(*i) == 0 )
			{
				success = false;
				printlog("[%s] unsuccessfully removed from the search path.\n", line.c_str());
			}
			else
			{
				printlog("[%s] is removed from the search path.\n", line.c_str());
			}
		}
	}
	Mods::numCurrentModsLoaded = -1;
	PHYSFS_freeList(*i);
	return success;
}

bool Mods::mountAllExistingPaths()
{
	bool success = true;
	std::vector<std::pair<std::string, std::string>>::iterator it;
	for ( it = Mods::mountedFilepaths.begin(); it != Mods::mountedFilepaths.end(); ++it )
	{
		std::pair<std::string, std::string> itpair = *it;
		if ( PHYSFS_mount(itpair.first.c_str(), NULL, 0) )
		{
			printlog("[%s] is in the search path.\n", itpair.first.c_str());
		}
		else
		{
			printlog("[%s] unsuccessfully added to search path.\n", itpair.first.c_str());
			success = false;
		}
	}
	Mods::numCurrentModsLoaded = Mods::mountedFilepaths.size();
	return success;
}

void Mods::loadModels(int start, int end) {
	start = std::clamp(start, 0, (int)nummodels - 1);
	end = std::clamp(end, 0, (int)nummodels);

	if ( start >= end ) {
		return;
	}

	//messagePlayer(clientnum, Language::get(2354));
#ifndef EDITOR
	printlog(Language::get(2355), start, end);
#endif

	loading = true;
	//createLoadingScreen(5);
	doLoadingScreen();

	std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, sizeof(name));
		if ( c >= start && c < end ) {
			if (polymodels[c].vao) {
				GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
			}
			if (polymodels[c].positions) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
			}
			if (polymodels[c].colors) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
			}
			if (polymodels[c].normals) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
			}
		}
	}

	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done, start, end]() {
		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
	modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
	File* fp = openDataFile(modelsDirectory.c_str(), "rb");
	for ( int c = 0; !fp->eof(); c++ )
	{
		char name[128];
		fp->gets2(name, sizeof(name));
		if ( c >= start && c < end )
		{
			if ( models[c] != NULL )
			{
				if ( models[c]->data )
				{
					free(models[c]->data);
				}
				free(models[c]);
				if ( polymodels[c].faces )
				{
					free(polymodels[c].faces);
					polymodels[c].faces = nullptr;
				}
				models[c] = loadVoxel(name);
			}
		}
	}
	FileIO::close(fp);
	generatePolyModels(start, end, true);
	loading_done = true;
	return 0;
		});
	while ( !loading_done )
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	generateVBOs(start, end);
}

void Mods::unloadMods(bool force)
{
#ifndef EDITOR
	isLoading = true;
	loading = true;
	createLoadingScreen(5);
	doLoadingScreen();

	// start loading
	mountedFilepathsSaved = mountedFilepaths;
	clearAllMountedPaths();
	mountedFilepaths.clear();
	Mods::disableSteamAchievements = false;
    if (force) {
        modelsListModifiedIndexes.clear();
        for (int c = 0; c < nummodels; ++c) {
            modelsListModifiedIndexes.push_back(c);
        }
        for (int c = 0; c < numsounds; ++c) {
            soundsListModifiedIndexes.push_back(c);
        }
        for (const auto& pair : systemResourceImages) {
            Mods::systemResourceImagesToReload.push_back(pair);
        }
		Mods::tileListRequireReloadUnmodded = true;
		Mods::modelsListRequiresReloadUnmodded = true;
		Mods::spriteImagesRequireReloadUnmodded = true;
		Mods::musicRequireReloadUnmodded = true;
		Mods::langRequireReloadUnmodded = true;
		Mods::monsterLimbsRequireReloadUnmodded = true;
		Mods::systemImagesReloadUnmodded = true;
    }
	updateLoadingScreen(10);
	doLoadingScreen();

	// update tiles
	if (Mods::tileListRequireReloadUnmodded)
	{
		physfsReloadTiles(true);
		Mods::tileListRequireReloadUnmodded = false;
	}
	doLoadingScreen();

	// reload sprites
	if (Mods::spriteImagesRequireReloadUnmodded)
	{
		physfsReloadSprites(true);
		Mods::spriteImagesRequireReloadUnmodded = false;
	}
	doLoadingScreen();

	// reload system images
	if (Mods::systemImagesReloadUnmodded)
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = false;
		systemResourceImagesToReload.clear();
	}

	updateLoadingScreen(20);
	doLoadingScreen();

	static int modelsIndexUpdateStart = 1;
	static int modelsIndexUpdateEnd = nummodels;

	// begin async load process
	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done]() {
		initGameDatafilesAsync(true);

		// update sounds
		if (Mods::soundListRequiresReloadUnmodded || !Mods::soundsListModifiedIndexes.empty())
		{
			physfsReloadSounds(true);
			Mods::soundListRequiresReloadUnmodded = false;
		}
		Mods::soundsListModifiedIndexes.clear();
		updateLoadingScreen(30);

		// update models
		if (Mods::modelsListRequiresReloadUnmodded || !Mods::modelsListModifiedIndexes.empty())
		{
			physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd);
			for (int c = 0; c < nummodels; ++c) {
				if (polymodels[c].faces) {
					free(polymodels[c].faces);
					polymodels[c].faces = nullptr;
				}
			}
			free(polymodels);
			polymodels = nullptr;
			generatePolyModels(0, nummodels, false);
			Mods::modelsListRequiresReloadUnmodded = false;
		}
		Mods::modelsListModifiedIndexes.clear();
		updateLoadingScreen(60);

		updateLoadingScreen(70);

		// reload lang file
		if (Mods::langRequireReloadUnmodded)
		{
			Language::reset();
			Language::reloadLanguage();
			Mods::langRequireReloadUnmodded = false;
		}

		// reload monster limb offsets
		if (Mods::monsterLimbsRequireReloadUnmodded)
		{
			physfsReloadMonsterLimbFiles();
			Mods::monsterLimbsRequireReloadUnmodded = false;
		}

		updateLoadingScreen(80);

		loading_done = true;
		return 0;
		});
	while (!loading_done)
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// final loading steps
	initGameDatafiles(true);
	for (int c = 0; c < nummodels; ++c) {
		if (polymodels[c].vao) {
			GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
		}
		if (polymodels[c].positions) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
		}
		if (polymodels[c].colors) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
		}
		if (polymodels[c].normals) {
			GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
		}
	}
	generateVBOs(0, nummodels);

	// reload books
	if ( Mods::booksRequireReloadUnmodded )
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = false;
	}
	consoleCommand("/dumpcache");

	// reload music
	if (Mods::musicRequireReloadUnmodded)
	{
		gamemodsUnloadCustomThemeMusic();
		bool reloadIntroMusic = false;
		physfsReloadMusic(reloadIntroMusic, true);
		if (reloadIntroMusic)
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = false;
	}
	destroyLoadingScreen();
	loading = false;
	isLoading = false;
#endif
}

void Mods::loadMods()
{
#ifndef EDITOR
	Mods::disableSteamAchievements = false;
	Mods::verifyAchievements(nullptr, false);

	isLoading = true;
	loading = true;
	createLoadingScreen(5);
	doLoadingScreen();

	Mods::customContentLoadedFirstTime = true;

	updateLoadingScreen(10);
	doLoadingScreen();

	// process any new model files encountered in the mod load list.
	if ( physfsSearchModelsToUpdate() || !Mods::modelsListModifiedIndexes.empty() )
	{
		int modelsIndexUpdateStart = 1;
		int modelsIndexUpdateEnd = nummodels;
		bool oldModelCache = useModelCache;
		useModelCache = false;
		physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd);
		for (int c = modelsIndexUpdateStart; c < modelsIndexUpdateEnd && c < nummodels; ++c) {
			if (polymodels[c].faces) {
				free(polymodels[c].faces);
				polymodels[c].faces = nullptr;
			}
			if (polymodels[c].vao) {
				GL_CHECK_ERR(glDeleteVertexArrays(1, &polymodels[c].vao));
			}
			if (polymodels[c].positions) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].positions));
			}
			if (polymodels[c].colors) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].colors));
			}
			if (polymodels[c].normals) {
				GL_CHECK_ERR(glDeleteBuffers(1, &polymodels[c].normals));
			}
		}

		// polymodels will get free'd if generating all models in generatePolyModels
		//free(polymodels);
		//polymodels = nullptr;
		generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, true);
		generateVBOs(modelsIndexUpdateStart, modelsIndexUpdateEnd);
		useModelCache = oldModelCache;
		Mods::modelsListRequiresReloadUnmodded = true;
	}

	updateLoadingScreen(20);
	doLoadingScreen();

	if ( physfsSearchSoundsToUpdate() || !Mods::soundsListModifiedIndexes.empty() )
	{
		physfsReloadSounds(false);
		Mods::soundListRequiresReloadUnmodded = true;
	}

	updateLoadingScreen(30);
	doLoadingScreen();

	if ( physfsSearchTilesToUpdate() )
	{
		physfsReloadTiles(false);
		Mods::tileListRequireReloadUnmodded = true;
	}
	else if ( Mods::tileListRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadTiles(true);
		Mods::tileListRequireReloadUnmodded = false;
	}

	updateLoadingScreen(40);
	doLoadingScreen();

	if ( physfsSearchSpritesToUpdate() )
	{
		physfsReloadSprites(false);
		Mods::spriteImagesRequireReloadUnmodded = true;
	}
	else if ( Mods::spriteImagesRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadSprites(true);
		Mods::spriteImagesRequireReloadUnmodded = false;
	}

	updateLoadingScreen(50);
	doLoadingScreen();

	updateLoadingScreen(60);
	doLoadingScreen();

	gamemodsUnloadCustomThemeMusic();

	if ( physfsSearchMusicToUpdate() )
	{
		bool reloadIntroMusic = false;
		physfsReloadMusic(reloadIntroMusic, false);
		if ( reloadIntroMusic )
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = true;
	}
	else if ( Mods::musicRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		// restore old music
		bool reloadIntroMusic = true;
		physfsReloadMusic(reloadIntroMusic, true);
		if ( reloadIntroMusic )
		{
#ifdef SOUND
			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		Mods::musicRequireReloadUnmodded = false;
	}

	updateLoadingScreen(70);
	doLoadingScreen();

	std::string langDirectory = PHYSFS_getRealDir("lang/en.txt");
	if ( langDirectory.compare("./") != 0 )
	{
		if ( Language::reloadLanguage() != 0 )
		{
			printlog("[PhysFS]: Error reloading modified language file in lang/ directory!");
		}
		else
		{
			printlog("[PhysFS]: Found modified language file in lang/ directory, reloading en.txt...");
		}
		Mods::langRequireReloadUnmodded = true;
	}
	else if ( Mods::langRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		Language::reloadLanguage();
		Mods::langRequireReloadUnmodded = false;
	}

	updateLoadingScreen(80);
	doLoadingScreen();

	if ( physfsSearchMonsterLimbFilesToUpdate() )
	{
		physfsReloadMonsterLimbFiles();
		Mods::monsterLimbsRequireReloadUnmodded = true;
	}
	else if ( Mods::monsterLimbsRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadMonsterLimbFiles();
		Mods::monsterLimbsRequireReloadUnmodded = false;
	}

	updateLoadingScreen(85);
	doLoadingScreen();

	if ( physfsSearchSystemImagesToUpdate() )
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = true;
	}
	else if ( Mods::systemImagesReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		physfsReloadSystemImages();
		Mods::systemImagesReloadUnmodded = false;
	}

	updateLoadingScreen(90);
	doLoadingScreen();

	initGameDatafiles(true);

	updateLoadingScreen(95);
	doLoadingScreen();

	std::atomic_bool loading_done{ false };
	auto loading_task = std::async(std::launch::async, [&loading_done]() {
		initGameDatafilesAsync(true);
		loading_done = true;
		return 0;
		});
	while ( !loading_done )
	{
		doLoadingScreen();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if ( physfsSearchBooksToUpdate() )
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = true;
	}
	else if ( Mods::booksRequireReloadUnmodded ) // clean revert if we had loaded mods but can't find any modded ones
	{
		consoleCommand("/dumpcache");
		physfsReloadBooks();
		Mods::booksRequireReloadUnmodded = false;
	}

	loadLights();

	consoleCommand("/dumpcache");

	destroyLoadingScreen();

	loading = false;
	isLoading = false;
#endif
}

void Mods::writeLevelsTxtAndPreview(std::string modFolder)
{
	std::string path = outputdir;
	path.append(PHYSFS_getDirSeparator()).append("mods/").append(modFolder);
	if ( access(path.c_str(), F_OK) == 0 )
	{
		std::string writeFile = modFolder + "/maps/levels.txt";
		PHYSFS_File* physfp = PHYSFS_openWrite(writeFile.c_str());
		if ( physfp != nullptr )
		{
			PHYSFS_writeBytes(physfp, "map: start\n", 11);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "map: minetoswamp\n", 17);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "map: swamptolabyrinth\n", 22);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "map: labyrinthtoruins\n", 22);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "map: boss\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "map: hellboss\n", 14);
			PHYSFS_writeBytes(physfp, "map: hamlet\n", 12);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "map: cavestocitadel\n", 20);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "map: sanctum", 12);
			PHYSFS_close(physfp);
		}
		else
		{
			printlog("[PhysFS]: Failed to open %s/maps/levels.txt for writing.", path.c_str());
		}

		std::string srcImage = datadir;
		srcImage.append("images/system/preview.png");
		std::string dstImage = path + "/preview.png";
		if ( access(srcImage.c_str(), F_OK) == 0 )
		{
			if ( File* fp_read = FileIO::open(srcImage.c_str(), "rb") )
			{
				if ( File* fp_write = FileIO::open(dstImage.c_str(), "wb") )
				{
					char chunk[1024];
					auto len = fp_read->read(chunk, sizeof(chunk[0]), sizeof(chunk));
					while ( len == sizeof(chunk) )
					{
						fp_write->write(chunk, sizeof(chunk[0]), len);
						len = fp_read->read(chunk, sizeof(chunk[0]), sizeof(chunk));
					}
					fp_write->write(chunk, sizeof(chunk[0]), len);
					FileIO::close(fp_write);
				}
				else
				{
					printlog("[PhysFS]: Failed to write preview.png in %s", dstImage.c_str());
				}
				FileIO::close(fp_read);
			}
			else
			{
				printlog("[PhysFS]: Failed to open %s", srcImage.c_str());
			}
		}
		else
		{
			printlog("[PhysFS]: Failed to access %s", srcImage.c_str());
		}
	}
	else
	{
		printlog("[PhysFS]: Failed to write levels.txt in %s", path.c_str());
	}
}

int Mods::createBlankModDirectory(std::string foldername)
{
	std::string baseDir = outputdir;
	baseDir.append(PHYSFS_getDirSeparator()).append("mods").append(PHYSFS_getDirSeparator()).append(foldername);

	if ( access(baseDir.c_str(), F_OK) == 0 )
	{
		// folder already exists!
		return 1;
	}
	else
	{
		if ( PHYSFS_mkdir(foldername.c_str()) )
		{
			std::string dir = foldername;
			std::string folder = "/books";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/editor";
			PHYSFS_mkdir((dir + folder).c_str());

			folder = "/images";
			PHYSFS_mkdir((dir + folder).c_str());
			std::string subfolder = "/sprites";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/system";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/tiles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/ui";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/items";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/images";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/lang";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/maps";
			PHYSFS_mkdir((dir + folder).c_str());
			writeLevelsTxtAndPreview(foldername.c_str());

			folder = "/models";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/creatures";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/decorations";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/doors";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/items";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/particles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/music";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/sound";
			PHYSFS_mkdir((dir + folder).c_str());

			folder = "/data";
			PHYSFS_mkdir((dir + folder).c_str());

			return 0;
		}
	}
	return 2;
}

EquipmentModelOffsets_t EquipmentModelOffsets;

bool EquipmentModelOffsets_t::modelOffsetExists(int monster, int sprite)
{
	auto find = monsterModelsMap.find(monster);
	if ( find != monsterModelsMap.end() )
	{
		auto find2 = find->second.find(sprite);
		if ( find2 != find->second.end() )
		{
			return true;
		}
	}
	return false;
}

EquipmentModelOffsets_t::ModelOffset_t& EquipmentModelOffsets_t::getModelOffset(int monster, int sprite)
{
	return monsterModelsMap[monster][sprite];
}

bool EquipmentModelOffsets_t::expandHelmToFitMask(int monster, int helmSprite, int maskSprite)
{
	if ( modelOffsetExists(monster, maskSprite) )
	{
		auto& maskOffset = getModelOffset(monster, maskSprite);
		if ( maskOffset.oversizedMask )
		{
			if ( modelOffsetExists(monster, helmSprite) )
			{
				auto& helmOffset = getModelOffset(monster, helmSprite);
				if ( helmOffset.expandToFitMask )
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool EquipmentModelOffsets_t::maskHasAdjustmentForExpandedHelm(int monster, int helmSprite, int maskSprite)
{
	if ( modelOffsetExists(monster, maskSprite) )
	{
		auto& maskOffset = getModelOffset(monster, maskSprite);
		if ( maskOffset.adjustToExpandedHelm.find(helmSprite) != maskOffset.adjustToExpandedHelm.end() )
		{
			return true;
		}
		else if ( maskOffset.adjustToExpandedHelm.find(-1) != maskOffset.adjustToExpandedHelm.end() )
		{
			return true;
		}
	}
	return false;
}

EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t EquipmentModelOffsets_t::getExpandHelmOffset(int monster, 
	int helmSprite, int maskSprite)
{
	if ( modelOffsetExists(monster, helmSprite) )
	{
		auto& helmOffset = getModelOffset(monster, helmSprite);
		if ( helmOffset.adjustToOversizeMask.find(maskSprite) != helmOffset.adjustToOversizeMask.end() )
		{
			return helmOffset.adjustToOversizeMask[maskSprite];
		}
		else if ( helmOffset.adjustToOversizeMask.find(-1) != helmOffset.adjustToOversizeMask.end() )
		{
			return helmOffset.adjustToOversizeMask[-1];
		}
	}
	return EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t();
}

EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t EquipmentModelOffsets_t::getMaskOffsetForExpandHelm(int monster, 
	int helmSprite, int maskSprite)
{
	if ( modelOffsetExists(monster, maskSprite) )
	{
		auto& maskOffset = getModelOffset(monster, maskSprite);
		if ( maskOffset.adjustToExpandedHelm.find(helmSprite) != maskOffset.adjustToExpandedHelm.end() )
		{
			return maskOffset.adjustToExpandedHelm[helmSprite];
		}
		else if ( maskOffset.adjustToExpandedHelm.find(-1) != maskOffset.adjustToExpandedHelm.end() )
		{
			return maskOffset.adjustToExpandedHelm[-1];
		}
	}
	return EquipmentModelOffsets_t::ModelOffset_t::AdditionalOffset_t();
}

void EquipmentModelOffsets_t::readFromFile(std::string monsterName, int monsterType)
{
	if ( monsterType == NOTHING )
	{
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			if ( monstertypename[i] == monsterName )
			{
				monsterType = i;
				break;
			}
		}
	}

	if ( monsterType == NOTHING )
	{
		return;
	}

	std::string filename = "models/creatures/";
	filename += monstertypename[monsterType];
	filename += "/model_positions.json";

	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		//printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	static char buf[32000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		return;
	}
	if ( !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	monsterModelsMap[monsterType].clear();

	real_t baseFocalX = 0.0;
	real_t baseFocalY = 0.0;
	real_t baseFocalZ = 0.0;
	if ( d.HasMember("base_offsets") )
	{
		if ( d["base_offsets"].HasMember("focalx") )
		{
			baseFocalX = d["base_offsets"]["focalx"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focaly") )
		{
			baseFocalY = d["base_offsets"]["focaly"].GetDouble();
		}
		if ( d["base_offsets"].HasMember("focalz") )
		{
			baseFocalZ = d["base_offsets"]["focalz"].GetDouble();
		}
	}

	auto& itemsArr = d["items"];
	for ( auto it = itemsArr.Begin(); it != itemsArr.End(); ++it )
	{
		for ( auto it2 = it->MemberBegin(); it2 != it->MemberEnd(); ++it2 )
		{
			std::string itemName = it2->name.GetString();
			if ( ItemTooltips.itemNameStringToItemID.find(itemName) == ItemTooltips.itemNameStringToItemID.end() )
			{
				continue;
			}
			ItemType itemType = (ItemType)ItemTooltips.itemNameStringToItemID[itemName];
			std::vector<int> models;
			if ( it2->value.HasMember("models") )
			{
				if ( it2->value["models"].IsArray() )
				{
					if ( it2->value["models"].Size() == 0 )
					{
						for ( int i = items[itemType].index; i < items[itemType].index + items[itemType].variations; ++i )
						{
							models.push_back(i);
						}
					}
					else
					{
						for ( auto itArr = it2->value["models"].Begin(); itArr != it2->value["models"].End(); ++itArr )
						{
							if ( itArr->IsInt() )
							{
								models.push_back(itArr->GetInt());
							}
						}
					}
				}
			}

			real_t focalx = it2->value["focalx"].GetDouble();
			real_t focaly = it2->value["focaly"].GetDouble();
			real_t focalz = it2->value["focalz"].GetDouble();
			real_t scalex = 0.0;
			if ( it2->value.HasMember("scalex") )
			{
				scalex = it2->value["scalex"].GetDouble();
			}
			real_t scaley = 0.0;
			if ( it2->value.HasMember("scaley") )
			{
				scaley = it2->value["scaley"].GetDouble();
			}
			real_t scalez = 0.0;
			if ( it2->value.HasMember("scalez") )
			{
				scalez = it2->value["scalez"].GetDouble();
			}
			real_t rotation = it2->value["rotation"].GetDouble();
			real_t pitch = it2->value.HasMember("pitch") ?
				it2->value["pitch"].GetDouble() : 0.0;
			int limbsIndex = it2->value["limbs_index"].GetInt();
			bool oversizedMask = it2->value.HasMember("oversize_mask") ? 
				it2->value["oversize_mask"].GetBool() : false;
			bool expandToFitMask = it2->value.HasMember("expand_to_fit_oversize_mask") ?
				it2->value["expand_to_fit_oversize_mask"].GetBool() : false;

			for ( auto index : models )
			{
				auto& entry = monsterModelsMap[monsterType][index];
				entry.focalx = focalx + baseFocalX;
				entry.focaly = focaly + baseFocalY;
				entry.focalz = focalz + baseFocalZ;
				entry.scalex = scalex;
				entry.scaley = scaley;
				entry.scalez = scalez;
				entry.rotation = rotation * (PI / 2);
				entry.pitch = pitch * (PI / 2);
				entry.limbsIndex = limbsIndex;
				entry.expandToFitMask = expandToFitMask;
				entry.oversizedMask = oversizedMask;

				if ( it2->value.HasMember("adjust_on_oversize_mask") )
				{
					auto& itr = it2->value["adjust_on_oversize_mask"];
					for ( auto adjItr = itr.Begin(); adjItr != itr.End(); ++adjItr )
					{
						std::vector<int> models;
						if ( (*adjItr)["mask_sprite"].Size() == 0 )
						{
							models.push_back(-1);
						}
						else
						{
							for ( auto itr2 = (*adjItr)["mask_sprite"].Begin(); itr2 != (*adjItr)["mask_sprite"].End(); ++itr2 )
							{
								models.push_back(itr2->GetInt());
							}
						}
						for ( auto model : models )
						{
							if ( (*adjItr).HasMember("focalx") )
							{
								entry.adjustToOversizeMask[model].focalx = (*adjItr)["focalx"].GetDouble();
							}
							if ( (*adjItr).HasMember("focaly") )
							{
								entry.adjustToOversizeMask[model].focaly = (*adjItr)["focaly"].GetDouble();
							}
							if ( (*adjItr).HasMember("focalz") )
							{
								entry.adjustToOversizeMask[model].focalz = (*adjItr)["focalz"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalex") )
							{
								entry.adjustToOversizeMask[model].scalex = (*adjItr)["scalex"].GetDouble();
							}
							if ( (*adjItr).HasMember("scaley") )
							{
								entry.adjustToOversizeMask[model].scaley = (*adjItr)["scaley"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalez") )
							{
								entry.adjustToOversizeMask[model].scalez = (*adjItr)["scalez"].GetDouble();
							}
						}
					}
				}

				if ( it2->value.HasMember("adjust_on_expand_helm") )
				{
					auto& itr = it2->value["adjust_on_expand_helm"];
					for ( auto adjItr = itr.Begin(); adjItr != itr.End(); ++adjItr )
					{
						std::vector<int> models;
						if ( (*adjItr)["helm_sprite"].Size() == 0 )
						{
							models.push_back(-1);
						}
						else
						{
							for ( auto itr2 = (*adjItr)["helm_sprite"].Begin(); itr2 != (*adjItr)["helm_sprite"].End(); ++itr2 )
							{
								models.push_back(itr2->GetInt());
							}
						}
						for ( auto model : models )
						{
							if ( (*adjItr).HasMember("focalx") )
							{
								entry.adjustToExpandedHelm[model].focalx = (*adjItr)["focalx"].GetDouble();
							}
							if ( (*adjItr).HasMember("focaly") )
							{
								entry.adjustToExpandedHelm[model].focaly = (*adjItr)["focaly"].GetDouble();
							}
							if ( (*adjItr).HasMember("focalz") )
							{
								entry.adjustToExpandedHelm[model].focalz = (*adjItr)["focalz"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalex") )
							{
								entry.adjustToExpandedHelm[model].scalex = (*adjItr)["scalex"].GetDouble();
							}
							if ( (*adjItr).HasMember("scaley") )
							{
								entry.adjustToExpandedHelm[model].scaley = (*adjItr)["scaley"].GetDouble();
							}
							if ( (*adjItr).HasMember("scalez") )
							{
								entry.adjustToExpandedHelm[model].scalez = (*adjItr)["scalez"].GetDouble();
							}
						}
					}
				}
			}
		}
	}

	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
}

#ifndef EDITOR
void GameModeManager_t::CurrentSession_t::ChallengeRun_t::updateKillEvent(Entity* entity)
{
	if ( multiplayer == CLIENT || !isActive() || !entity )
	{
		return;
	}

	if ( gameModeManager.currentSession.challengeRun.numKills < 0 )
	{
		return;
	}
	if ( !(eventType == CHEVENT_KILLS_MONSTERS
		|| eventType == CHEVENT_KILLS_FURNITURE) )
	{
		return;
	}

	if ( entity->behavior == &actMonster && eventType != CHEVENT_KILLS_MONSTERS )
	{
		return;
	}
	if ( entity->behavior == &actFurniture && eventType != CHEVENT_KILLS_FURNITURE )
	{
		return;
	}

	auto& killTotal = gameStatistics[STATISTICS_TOTAL_KILLS];
	killTotal++;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		achievementObserver.playerAchievements[i].totalKillsTickUpdate = true;
	}

	if ( killTotal <= gameModeManager.currentSession.challengeRun.numKills )
	{
		if ( killTotal % 10 == 0 || killTotal == 1 || killTotal == gameModeManager.currentSession.challengeRun.numKills )
		{
			const char* challengeName = "CHALLENGE_MONSTER_KILLS";
			if ( eventType == CHEVENT_KILLS_FURNITURE )
			{
				challengeName = "CHALLENGE_FURNITURE_KILLS";
			}
			UIToastNotificationManager.createStatisticUpdateNotification(challengeName, killTotal, gameModeManager.currentSession.challengeRun.numKills);
			if ( multiplayer == SERVER )
			{
				for ( int i = 1; i < MAXPLAYERS; ++i )
				{
					if ( !client_disconnected[i] && !players[i]->isLocalPlayer() )
					{
						strcpy((char*)net_packet->data, "CHCT");
						SDLNet_Write16((Sint16)killTotal, &net_packet->data[4]);
						SDLNet_Write16((Sint16)gameModeManager.currentSession.challengeRun.numKills, &net_packet->data[6]);
						net_packet->data[8] = eventType;
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 9;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
			}
		}
	}
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::applySettings()
{
	if ( !inUse ) { return; }

	svFlags = setFlags;
	svFlags |= SV_FLAG_HUNGER;
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::setup(std::string _scenario)
{
	reset();
	scenarioStr = _scenario;
	if ( scenarioStr == "" )
	{
		return;
	}

	inUse = loadScenario();
	if ( inUse )
	{
		printlog("[Challenge]: Loaded scenario");
	}
}

void GameModeManager_t::CurrentSession_t::ChallengeRun_t::reset()
{
	if ( inUse )
	{
		printlog("[Challenge]: Resetting");
	}
	inUse = false;

	if ( !baseStats )
	{
		baseStats = new Stat(0);
	}
	if ( !addStats )
	{
		addStats = new Stat(0);
	}

	addStats->clearStats();
	addStats->HP = 0;
	addStats->MAXHP = 0;
	addStats->MP = 0;
	addStats->MAXMP = 0;

	baseStats->clearStats();
	baseStats->HP = 0;
	baseStats->MAXHP = 0;
	baseStats->MP = 0;
	baseStats->MAXMP = 0;

	seed = 0;
	lockedFlags = 0;
	setFlags = 0;
	classnum = -1;
	race = -1;
	customBaseStats = false;
	customAddStats = false;

	globalXPPercent = 100;
	globalGoldPercent = 100;
	playerWeightPercent = 100;
	playerSpeedMax = 12.5;

	eventType = -1;
	winLevel = -1;
	startLevel = -1;
	winCondition = -1;
	numKills = -1;

}

#ifndef NDEBUG
static ConsoleVariable<int> cvar_challengerace("/challengerace", -1);
static ConsoleVariable<int> cvar_challengeclass("/challengeclass", -1);
static ConsoleVariable<int> cvar_challengeevent("/challengeevent", -1);
#endif

bool GameModeManager_t::CurrentSession_t::ChallengeRun_t::loadScenario()
{
	rapidjson::Document d;
	const char* json = scenarioStr.c_str();
	d.Parse(json);

	if ( !d.HasMember("version") || !d.HasMember("seed") || !d.HasMember("lockedFlags") || !d.HasMember("setFlags") )
	{
		printlog("[JSON]: Error: Scenario has no 'version' value in json file, or JSON syntax incorrect!");
		reset();
		return false;
	}

	seed = d["seed"].GetUint();
	seed_word = d["seed_word"].GetString();
	lockedFlags = d["lockedFlags"].GetUint();
	setFlags = d["setFlags"].GetUint();
	lid = d["lid"].GetString();
	lid_version = d["lid_version"].GetInt();

	if ( d.HasMember("base_stats") )
	{
		const rapidjson::Value& stats = d["base_stats"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("enabled") == 0 )
			{
				customBaseStats = itr->value.GetInt() == 0 ? false : true;
			}
			else if ( name.compare("HP") == 0 )
			{
				baseStats->HP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXHP") == 0 )
			{
				baseStats->MAXHP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MP") == 0 )
			{
				baseStats->MP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXMP") == 0 )
			{
				baseStats->MAXMP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("STR") == 0 )
			{
				baseStats->STR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("DEX") == 0 )
			{
				baseStats->DEX = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CON") == 0 )
			{
				baseStats->CON = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("INT") == 0 )
			{
				baseStats->INT = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PER") == 0 )
			{
				baseStats->PER = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CHR") == 0 )
			{
				baseStats->CHR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("EXP") == 0 )
			{
				baseStats->EXP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("LVL") == 0 )
			{
				baseStats->LVL = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("GOLD") == 0 )
			{
				baseStats->GOLD = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PROFICIENCIES") == 0 )
			{
				int index = -1;
				for ( auto arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
				{
					++index;
					if ( index >= NUMPROFICIENCIES )
					{
						break;
					}
					baseStats->setProficiency(index, arr_itr->GetInt());
				}
			}
		}
	}

	if ( d.HasMember("add_stats") )
	{
		const rapidjson::Value& stats = d["add_stats"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("enabled") == 0 )
			{
				customAddStats = itr->value.GetInt() == 0 ? false : true;
			}
			else if ( name.compare("HP") == 0 )
			{
				addStats->HP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXHP") == 0 )
			{
				addStats->MAXHP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MP") == 0 )
			{
				addStats->MP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("MAXMP") == 0 )
			{
				addStats->MAXMP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("STR") == 0 )
			{
				addStats->STR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("DEX") == 0 )
			{
				addStats->DEX = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CON") == 0 )
			{
				addStats->CON = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("INT") == 0 )
			{
				addStats->INT = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PER") == 0 )
			{
				addStats->PER = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("CHR") == 0 )
			{
				addStats->CHR = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("EXP") == 0 )
			{
				addStats->EXP = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("LVL") == 0 )
			{
				addStats->LVL = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("GOLD") == 0 )
			{
				addStats->GOLD = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("PROFICIENCIES") == 0 )
			{
				int index = -1;
				for ( auto arr_itr = itr->value.Begin(); arr_itr != itr->value.End(); ++arr_itr )
				{
					++index;
					if ( index >= NUMPROFICIENCIES )
					{
						break;
					}
					addStats->setProficiency(index, arr_itr->GetInt());
				}
			}
		}
	}

	if ( d.HasMember("character") )
	{
		const rapidjson::Value& stats = d["character"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("class") == 0 )
			{
				classnum = static_cast<Sint32>(itr->value.GetInt());
			}
			else if ( name.compare("race") == 0 )
			{
				race = static_cast<Sint32>(itr->value.GetInt());
			}
		}
	}

	if ( d.HasMember("gameplay") )
	{
		const rapidjson::Value& stats = d["gameplay"];
		for ( auto itr = stats.MemberBegin(); itr != stats.MemberEnd(); ++itr )
		{
			std::string name = itr->name.GetString();
			if ( name.compare("winLevel") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					winLevel = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("event_type") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					eventType = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("numKills") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					numKills = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("startLevel") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					startLevel = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("winCondition") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					winCondition = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("globalXPPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					globalXPPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("globalGoldPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					globalGoldPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("playerWeightPercent") == 0 )
			{
				if ( itr->value.GetInt() >= 0 )
				{
					playerWeightPercent = static_cast<Sint32>(itr->value.GetInt());
				}
			}
			else if ( name.compare("playerSpeedMax") == 0 )
			{
				if ( itr->value.GetFloat() >= 0 )
				{
					playerSpeedMax = static_cast<Sint32>(itr->value.GetFloat());
				}
			}
		}
	}

#ifndef NDEBUG
	if ( *cvar_challengerace >= 0 )
	{
		race = *cvar_challengerace;
	}
	if ( *cvar_challengeclass >= 0 )
	{
		classnum = *cvar_challengeclass;
	}
	if ( *cvar_challengeevent >= 0 )
	{
		eventType = *cvar_challengeevent;
	}
#endif

	return true;
}
#endif

void jsonVecToVec(rapidjson::Value& val, std::vector<std::string>& vec )
{
	for ( auto itr = val.Begin(); itr != val.End(); ++itr )
	{
		if ( itr->IsString() )
		{
			vec.push_back(itr->GetString());
		}
	}
}

void jsonVecToVec(rapidjson::Value& val, std::vector<Sint32>& vec)
{
	for ( auto itr = val.Begin(); itr != val.End(); ++itr )
	{
		if ( itr->IsInt() )
		{
			vec.push_back(itr->GetInt());
		}
	}
}

#ifndef EDITOR
Compendium_t CompendiumEntries;
Item Compendium_t::compendiumItem;
Compendium_t::CompendiumEntityCurrent Compendium_t::compendiumEntityCurrent;
Entity Compendium_t::compendiumItemModel(-1, 0, nullptr, nullptr);
bool Compendium_t::tooltipNeedUpdate = false;
SDL_Rect Compendium_t::tooltipPos;

std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMonsters_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumMonsters_t::contentsMap;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMonsters_t::contents_unfiltered;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumMonsters_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumWorld_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumWorld_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumWorld_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumCodex_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumCodex_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumCodex_t::unlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumItems_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumItems_t::contentsMap;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumItems_t::unlocks;
std::map<int, Compendium_t::CompendiumUnlockStatus> Compendium_t::CompendiumItems_t::itemUnlocks;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::CompendiumMagic_t::contents;
std::map<std::string, std::string> Compendium_t::CompendiumMagic_t::contentsMap;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::AchievementData_t::contents;
std::map<std::string, Compendium_t::CompendiumUnlockStatus> Compendium_t::AchievementData_t::unlocks;
std::map<std::string, std::string> Compendium_t::AchievementData_t::contentsMap;
int Compendium_t::CompendiumMonsters_t::completionPercent = 0;
int Compendium_t::CompendiumCodex_t::completionPercent = 0;
int Compendium_t::CompendiumItems_t::completionPercent = 0;
int Compendium_t::CompendiumMagic_t::completionPercent = 0;
int Compendium_t::CompendiumWorld_t::completionPercent = 0;
int Compendium_t::AchievementData_t::completionPercent = 0;
int Compendium_t::CompendiumMonsters_t::numUnread = 0;
int Compendium_t::CompendiumCodex_t::numUnread = 0;
int Compendium_t::CompendiumItems_t::numUnread = 0;
int Compendium_t::CompendiumMagic_t::numUnread = 0;
int Compendium_t::CompendiumWorld_t::numUnread = 0;
int Compendium_t::AchievementData_t::numUnread = 0;

std::map<int, std::string> Compendium_t::Events_t::monsterIDToString;
std::map<int, std::string> Compendium_t::Events_t::codexIDToString;
std::map<int, std::string> Compendium_t::Events_t::worldIDToString;
std::map<int, std::string> Compendium_t::Events_t::itemIDToString;

void Compendium_t::readContentsLang(std::string name, std::map<std::string, std::vector<std::pair<std::string, std::string>>>& contents,
	std::map<std::string, std::string>& contentsMap)
{
	contents.clear();
	contentsMap.clear();

	std::string filename = "lang/compendium_lang/";
	filename += name;
	filename += ".json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("contents") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d["contents"].Begin(); itr != d["contents"].End(); ++itr )
	{
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			contents["default"].push_back(std::make_pair(itr2->value.GetString(), itr2->name.GetString()));
			if ( name == "contents_monsters"
				&& (!strcmp(itr2->value.GetString(), "crab") || !strcmp(itr2->value.GetString(), "bubbles")) )
			{
				// dont read into map
			}
			else
			{
				contentsMap[itr2->value.GetString()] = itr2->name.GetString();
			}
		}
	}

	if ( d.HasMember("contents_alphabetical") )
	{
		for ( auto itr = d["contents_alphabetical"].Begin(); itr != d["contents_alphabetical"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				contents["alphabetical"].push_back(std::make_pair(itr2->value.GetString(), itr2->name.GetString()));
				if ( name == "contents_monsters"
					&& (!strcmp(itr2->value.GetString(), "crab") || !strcmp(itr2->value.GetString(), "bubbles")) )
				{
					// dont read into map
				}
				else
				{
					contentsMap[itr2->value.GetString()] = itr2->name.GetString();
				}
			}
		}
	}
}

void Compendium_t::CompendiumMonsters_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_monsters", contents_unfiltered, contentsMap);
}

void Compendium_t::CompendiumWorld_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_world", contents, contentsMap);
}

void Compendium_t::CompendiumCodex_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_codex", contents, contentsMap);
}

void Compendium_t::CompendiumItems_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_items", contents, contentsMap);
}

void Compendium_t::CompendiumMagic_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_magic", contents, contentsMap);
}

void Compendium_t::AchievementData_t::readContentsLang()
{
	Compendium_t::readContentsLang("contents_achievements", contents, contentsMap);
}

void Compendium_t::updateTooltip()
{
	bool update = tooltipNeedUpdate;
	tooltipNeedUpdate = false;

	if ( MainMenu::main_menu_frame )
	{
		auto compendiumFrame = MainMenu::main_menu_frame->findFrame("compendium");
		if ( !compendiumFrame ) { return; }
		
		players[MainMenu::getMenuOwner()]->inventoryUI.updateInventoryItemTooltip(compendiumFrame);
		
		if ( update )
		{
			players[MainMenu::getMenuOwner()]->hud.updateFrameTooltip(&compendiumItem, tooltipPos.x, tooltipPos.y, Player::PANEL_JUSTIFY_RIGHT, compendiumFrame);
		}

		if ( Frame* tooltipContainerFrame = compendiumFrame->findFrame("player tooltip container 0") )
		{
			if ( auto prompt = tooltipContainerFrame->findFrame("item_widget") )
			{
				if ( auto tooltip = tooltipContainerFrame->findFrame("player tooltip 0") )
				{
					if ( tooltip->getSize().w == 0 )
					{
						prompt->setOpacity(0.0);
					}
					else
					{
						prompt->setOpacity(tooltip->getOpacity());
					}
					if ( Compendium_t::compendiumItem.type == SPELL_ITEM )
					{
						prompt->setOpacity(0.0);
					}
					SDL_Rect framePos = prompt->getSize();
					framePos.x = tooltip->getSize().x + tooltip->getSize().w - 6 - framePos.w;
					framePos.y = tooltip->getSize().y + tooltip->getSize().h - 10;
					//framePos.x = 802 + 378 / 2 - framePos.w / 2;
					//framePos.y = 110 - framePos.h + 14;
					prompt->setSize(framePos);
				}
			}
		}

	}
}

void Compendium_t::readItemsTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = items.find(key);
		if ( find != items.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
		}
	}
}

void Compendium_t::readItemsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/comp_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	items.clear();
	Compendium_t::Events_t::itemEventLookup.clear();
	Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::Events_t::itemIDToString.clear();
	Compendium_t::CompendiumItems_t::readContentsLang();

	auto& entries = d["items"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = items[name];

		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		for ( auto itr = w["items"].Begin(); itr != w["items"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				CompendiumItems_t::Codex_t::CodexItem_t item;
				item.name = itr2->name.GetString();
				item.rotation = 0;
				if ( itr2->value.HasMember("rotation") )
				{
					item.rotation = itr2->value["rotation"].GetInt();
				}
				obj.items_in_category.push_back(item);
			}
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}

		std::set<std::string> alwaysTrackedEvents = {
			"APPRAISED",
			"RUNS_COLLECTED"
		};

		for ( auto& item : obj.items_in_category )
		{
			const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
			if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
			{
				item.itemID = itemType;
				Compendium_t::Events_t::itemIDToString[itemType] = name;
				if ( ::items[itemType].item_slot != NO_EQUIP )
				{
					alwaysTrackedEvents.insert("BROKEN");
					alwaysTrackedEvents.insert("DEGRADED");
					alwaysTrackedEvents.insert("REPAIRS");
				}

				Compendium_t::Events_t::itemDisplayedEventsList.erase(itemType);
				Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(itemType);
			}
		}

		for ( auto& s : alwaysTrackedEvents )
		{
			auto find = Compendium_t::Events_t::eventIdLookup.find(s);
			if ( find != Compendium_t::Events_t::eventIdLookup.end() )
			{
				auto find2 = Compendium_t::Events_t::events.find(find->second);
				if ( find2 != Compendium_t::Events_t::events.end() )
				{
					for ( auto& item : obj.items_in_category )
					{
						const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
						if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
						{
							Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
						}
					}
				}
			}
		}

		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
							}
						}
					}
				}
			}
		}

		std::set<int> itemsInList;
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							const int itemType = ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[itemType];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(itemType);
							}
						}
					}
				}
			}
		}

		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			for ( auto item : itemsInList )
			{
				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[item];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[item];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() )
						{
							vec2.push_back(customEvents[index]);
						}
						else
						{
							vec2.push_back("");
						}
					}
					else
					{
						vec2.push_back("");
					}
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : items )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_items.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

void Compendium_t::readMagicTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_magic.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMagicTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = magic.find(key);
		if ( find != magic.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
		}
	}
}

void Compendium_t::readMagicFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/comp_magic.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readItemsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	magic.clear();
	//Compendium_t::Events_t::itemEventLookup.clear();
	//Compendium_t::Events_t::eventItemLookup.clear();
	Compendium_t::CompendiumMagic_t::readContentsLang();

	auto& entries = d["items"];
	std::unordered_set<std::string> ignoredSpells;
	std::unordered_set<std::string> ignoredSpellbooks;

	if ( d.HasMember("exclude_spells") )
	{
		for ( auto itr = d["exclude_spells"].Begin(); itr != d["exclude_spells"].End(); ++itr )
		{
			ignoredSpells.insert(itr->GetString());
		}
	}
	if ( d.HasMember("exclude_spellbooks") )
	{
		for ( auto itr = d["exclude_spellbooks"].Begin(); itr != d["exclude_spellbooks"].End(); ++itr )
		{
			ignoredSpellbooks.insert(itr->GetString());
		}
	}

	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = magic[name];

		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		std::set<std::string> objSpellsLookup;
		for ( auto itr = w["items"].Begin(); itr != w["items"].End(); ++itr )
		{
			for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
			{
				CompendiumItems_t::Codex_t::CodexItem_t item;
				item.name = itr2->name.GetString();
				item.rotation = 0;
				if ( itr2->value.HasMember("rotation") )
				{
					item.rotation = itr2->value["rotation"].GetInt();
				}
				if ( item.name.find("spell_") != std::string::npos )
				{
					for ( auto spell : allGameSpells )
					{
						if ( item.name == spell->spell_internal_name )
						{
							item.spellID = spell->ID;
							objSpellsLookup.insert(item.name);
							break;
						}
					}
				}
				else if ( item.name.find("spellbook_") != std::string::npos )
				{
					for ( auto spell : allGameSpells )
					{
						int book = getSpellbookFromSpellID(spell->ID);
						if ( book >= WOODEN_SHIELD && book < NUMITEMS && ::items[book].category == SPELLBOOK )
						{
							if ( item.name == itemNameStrings[book + 2] )
							{
								item.spellID = spell->ID;
								break;
							}
						}
					}
				}
				obj.items_in_category.push_back(item);
			}
		}

		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}

		//std::list<spell_t*> spellsToSort;
		//bool spells = name.find("spells") != std::string::npos;
		//bool spellbooks = name.find("spellbooks") != std::string::npos;
		//if ( spells || spellbooks )
		//{
		//	for ( auto spell : allGameSpells )
		//	{
		//		if ( spells && ignoredSpells.find(spell->spell_internal_name) != ignoredSpells.end() )
		//		{
		//			continue;
		//		}
		//		if ( spellbooks )
		//		{
		//			int book = getSpellbookFromSpellID(spell->ID);
		//			if ( book >= WOODEN_SHIELD && book < NUMITEMS && ::items[book].category == SPELLBOOK )
		//			{
		//				if ( ignoredSpellbooks.find(itemNameStrings[book + 2]) != ignoredSpellbooks.end() )
		//				{
		//					continue;
		//				}
		//			}
		//			else
		//			{
		//				continue;
		//			}
		//		}

		//		if ( name.find("damage") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_DAMAGE)
		//				== ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else if ( name.find("status") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
		//				== ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else if ( name.find("utility") != std::string::npos )
		//		{
		//			if ( ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_DAMAGE)
		//				!= ItemTooltips.spellItems[spell->ID].spellTags.end()
		//				|| ItemTooltips.spellItems[spell->ID].spellTags.find(ItemTooltips_t::SpellTagTypes::SPELL_TAG_STATUS_EFFECT)
		//				!= ItemTooltips.spellItems[spell->ID].spellTags.end() )
		//			{
		//				continue;
		//			}
		//		}
		//		else
		//		{
		//			continue;
		//		}
		//		spellsToSort.push_back(spell);
		//	}

		//	if ( spellbooks )
		//	{
		//		spellsToSort.sort([](const spell_t* lhs, const spell_t* rhs) {
		//			const int bookLeft = getSpellbookFromSpellID(lhs->ID);
		//			const int bookRight = getSpellbookFromSpellID(rhs->ID);

		//			const int bookLevelLeft = ::items[bookLeft].level >= 0 ? ::items[bookLeft].level : 10000; // -1 level sorted to the end
		//			const int bookLevelRight = ::items[bookRight].level >= 0 ? ::items[bookRight].level : 10000;

		//			if ( bookLevelLeft < bookLevelRight ) { return true; }
		//			if ( bookLevelLeft > bookLevelRight ) { return false; }
		//			if ( rhs->difficulty > lhs->difficulty ) { return true; }
		//			if ( rhs->difficulty < lhs->difficulty ) { return false; }

		//			return rhs->ID < lhs->ID;
		//			});
		//	}
		//	else
		//	{
		//		spellsToSort.sort([](const spell_t* lhs, const spell_t* rhs) {
		//			if ( rhs->difficulty > lhs->difficulty ) { return true; }
		//			if ( rhs->difficulty < lhs->difficulty ) { return false; }
		//			
		//			return rhs->ID < lhs->ID;
		//			});
		//	}

		//	for ( auto spell : spellsToSort )
		//	{
		//		CompendiumItems_t::Codex_t::CodexItem_t item;
		//		item.rotation = 0;
		//		if ( spellbooks )
		//		{
		//			int book = getSpellbookFromSpellID(spell->ID);
		//			item.name = itemNameStrings[book + 2];
		//			item.rotation = 180;
		//		}
		//		else
		//		{
		//			item.name = spell->spell_internal_name;
		//		}
		//		item.spellID = spell->ID;
		//		obj.items_in_category.push_back(item);
		//	}
		//}

		std::set<std::string> alwaysTrackedEvents = {
			"APPRAISED",
			"RUNS_COLLECTED"
		};

		for ( auto& item : obj.items_in_category )
		{
			bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
			const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
			if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
			{
				item.itemID = itemType;
				if ( !isSpell )
				{
					Compendium_t::Events_t::itemIDToString[itemType] = name;
					Compendium_t::Events_t::itemDisplayedEventsList.erase(itemType);
					Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(itemType);
				}
				else
				{
					Compendium_t::Events_t::itemIDToString[Compendium_t::Events_t::kEventSpellOffset + item.spellID] = name;
					Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
					Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
				}
				if ( itemType != SPELL_ITEM && ::items[itemType].item_slot != NO_EQUIP )
				{
					alwaysTrackedEvents.insert("BROKEN");
					alwaysTrackedEvents.insert("DEGRADED");
					alwaysTrackedEvents.insert("REPAIRS");
				}
			}
		}

		for ( auto& s : alwaysTrackedEvents )
		{
			auto find = Compendium_t::Events_t::eventIdLookup.find(s);
			if ( find != Compendium_t::Events_t::eventIdLookup.end() )
			{
				auto find2 = Compendium_t::Events_t::events.find(find->second);
				if ( find2 != Compendium_t::Events_t::events.end() )
				{
					for ( auto& item : obj.items_in_category )
					{
						bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
						const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
						if ( itemType == SPELL_ITEM )
						{
							Compendium_t::Events_t::itemEventLookup[Compendium_t::Events_t::kEventSpellOffset + item.spellID].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
						}
						else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
						{
							Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
							Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
						}
					}
				}
			}
		}

		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
							const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType == SPELL_ITEM )
							{
								Compendium_t::Events_t::itemEventLookup[Compendium_t::Events_t::kEventSpellOffset + item.spellID].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
							}
							else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								Compendium_t::Events_t::itemEventLookup[(ItemType)itemType].insert((Compendium_t::EventTags)find2->second.id);
								Compendium_t::Events_t::eventItemLookup[(Compendium_t::EventTags)find2->second.id].insert((ItemType)itemType);
							}
						}
					}
				}
			}
		}

		std::set<int> itemsInList;
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						for ( auto& item : obj.items_in_category )
						{
							bool isSpell = (objSpellsLookup.find(item.name) != objSpellsLookup.end());
							const int itemType = isSpell ? SPELL_ITEM : ItemTooltips.itemNameStringToItemID[item.name];
							if ( itemType == SPELL_ITEM )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventSpellOffset + item.spellID];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(Compendium_t::Events_t::kEventSpellOffset + item.spellID);
							}
							else if ( itemType >= WOODEN_SHIELD && itemType < NUMITEMS )
							{
								auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[itemType];
								if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
									== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
								{
									vec.push_back((Compendium_t::EventTags)find2->second.id);
								}
								itemsInList.insert(itemType);
							}
						}
					}
				}
			}
		}

		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			for ( auto item : itemsInList )
			{
				auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[item];
				int index = -1;
				for ( auto& v : vec )
				{
					++index;
					auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[item];
					if ( v == EventTags::CPDM_CUSTOM_TAG )
					{
						if ( index < customEvents.size() )
						{
							vec2.push_back(customEvents[index]);
						}
						else
						{
							vec2.push_back("");
						}
					}
					else
					{
						vec2.push_back("");
					}
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : magic )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_magic.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

void Compendium_t::readCodexTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_codex.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readCodexTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = codex.find(key);
		if ( find != codex.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.details.clear();
			if ( itr->value.HasMember("details") )
			{
				jsonVecToVec(itr->value["details"], find->second.details);
			}
			for ( auto& line : find->second.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' )
					{
						line[0] = '\x1E';
					}
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' )
							{
								line[c] = '\x1E';
								break;
							}
							else if ( line[c] != ' ' )
							{
								break;
							}
						}
					}
				}
			}

			find->second.linesToHighlight.clear();
			if ( itr->value.HasMember("details_line_highlights") )
			{
				for ( auto itr2 = itr->value["details_line_highlights"].Begin(); itr2 != itr->value["details_line_highlights"].End(); ++itr2 )
				{
					if ( itr2->HasMember("color") )
					{
						Uint8 r, g, b;
						if ( (*itr2)["color"].HasMember("r") )
						{
							r = (*itr2)["color"]["r"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("g") )
						{
							g = (*itr2)["color"]["g"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("b") )
						{
							b = (*itr2)["color"]["b"].GetInt();
						}

						find->second.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else
					{
						find->second.linesToHighlight.push_back(0);
					}
				}
			}
		}
	}
}

void Compendium_t::readCodexFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/codex.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readCodexFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("codex") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	codex.clear();

	Compendium_t::Events_t::eventCodexIDLookup.clear();
	Compendium_t::Events_t::eventCodexLookup.clear();
	Compendium_t::Events_t::codexIDToString.clear();
	Compendium_t::CompendiumCodex_t::readContentsLang();

	auto& entries = d["codex"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = codex[name];

		obj.id = w["event_lookup"].GetInt();
		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		if ( w.HasMember("details") )
		{
			jsonVecToVec(w["details"], obj.details);
		}
		obj.imagePath = w["img"].GetString();
		if ( w.HasMember("enable_tutorial") )
		{
			obj.enableTutorial = w["enable_tutorial"].GetBool();
		}
		if ( w.HasMember("rendered_imgs") )
		{
			jsonVecToVec(w["rendered_imgs"], obj.renderedImagePaths);
		}
		if ( w.HasMember("models") )
		{
			jsonVecToVec(w["models"], obj.models);
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}
		obj.linesToHighlight.clear();
		for ( auto& line : obj.details )
		{
			if ( line.size() > 0 )
			{
				if ( line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				else
				{
					for ( size_t c = 0; c < line.size(); ++c )
					{
						if ( line[c] == '-' )
						{
							line[c] = '\x1E';
							break;
						}
						else if ( line[c] != ' ' )
						{
							break;
						}
					}
				}
			}
		}
		if ( w.HasMember("feature_img") )
		{
			obj.featureImg = w["feature_img"].GetString();
		}
		if ( w.HasMember("details_line_highlights") )
		{
			for ( auto itr = w["details_line_highlights"].Begin(); itr != w["details_line_highlights"].End(); ++itr )
			{
				if ( itr->HasMember("color") )
				{
					Uint8 r, g, b;
					if ( (*itr)["color"].HasMember("r") )
					{
						r = (*itr)["color"]["r"].GetInt();
					}
					if ( (*itr)["color"].HasMember("g") )
					{
						g = (*itr)["color"]["g"].GetInt();
					}
					if ( (*itr)["color"].HasMember("b") )
					{
						b = (*itr)["color"]["b"].GetInt();
					}

					obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
				}
				else
				{
					obj.linesToHighlight.push_back(0);
				}
			}
		}

		Compendium_t::Events_t::eventCodexIDLookup[name] = obj.id;
		Compendium_t::Events_t::codexIDToString[obj.id + Compendium_t::Events_t::kEventCodexOffset] = name;
		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						Compendium_t::Events_t::eventCodexLookup[(Compendium_t::EventTags)find2->second.id].insert(name);
					}
				}
			}
		}

		Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventCodexOffset + obj.id);
		Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventCodexOffset + obj.id);
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
						if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
							== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
						{
							vec.push_back((Compendium_t::EventTags)find2->second.id);
						}
					}
				}
			}
		}
		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
			int index = -1;
			for ( auto& v : vec )
			{
				++index;
				auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventCodexOffset + obj.id];
				if ( v == EventTags::CPDM_CUSTOM_TAG )
				{
					if ( index < customEvents.size() )
					{
						vec2.push_back(customEvents[index]);
					}
					else
					{
						vec2.push_back("");
					}
				}
				else
				{
					vec2.push_back("");
				}
			}
		}
	}
}

void Compendium_t::readWorldTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_world.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readWorldTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = worldObjects.find(key);
		if ( find != worldObjects.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.details.clear();
			if ( itr->value.HasMember("details") )
			{
				jsonVecToVec(itr->value["details"], find->second.details);
			}
			for ( auto& line : find->second.details )
			{
				if ( line.size() > 0 )
				{
					if ( line[0] == '-' )
					{
						line[0] = '\x1E';
					}
					else
					{
						for ( size_t c = 0; c < line.size(); ++c )
						{
							if ( line[c] == '-' )
							{
								line[c] = '\x1E';
								break;
							}
							else if ( line[c] != ' ' )
							{
								break;
							}
						}
					}
				}
			}

			find->second.linesToHighlight.clear();
			if ( itr->value.HasMember("details_line_highlights") )
			{
				for ( auto itr2 = itr->value["details_line_highlights"].Begin(); itr2 != itr->value["details_line_highlights"].End(); ++itr2 )
				{
					if ( itr2->HasMember("color") )
					{
						Uint8 r, g, b;
						if ( (*itr2)["color"].HasMember("r") )
						{
							r = (*itr2)["color"]["r"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("g") )
						{
							g = (*itr2)["color"]["g"].GetInt();
						}
						if ( (*itr2)["color"].HasMember("b") )
						{
							b = (*itr2)["color"]["b"].GetInt();
						}

						find->second.linesToHighlight.push_back(makeColorRGB(r, g, b));
					}
					else
					{
						find->second.linesToHighlight.push_back(0);
					}
				}
			}
		}
	}
}

void Compendium_t::readWorldFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/world.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readWorldFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("world") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	worldObjects.clear();
	Compendium_t::Events_t::eventWorldIDLookup.clear();
	Compendium_t::Events_t::eventWorldLookup.clear();
	Compendium_t::Events_t::worldIDToString.clear();
	Compendium_t::CompendiumWorld_t::readContentsLang();

	auto& entries = d["world"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		auto& w = itr->value;
		auto& obj = worldObjects[name];

		obj.id = w["event_lookup"].GetInt();
		if ( w.HasMember("blurb") )
		{
			jsonVecToVec(w["blurb"], obj.blurb);
		}
		if ( w.HasMember("details") )
		{
			jsonVecToVec(w["details"], obj.details);
		}
		obj.imagePath = w["img"].GetString();
		if ( w.HasMember("models") )
		{
			jsonVecToVec(w["models"], obj.models);
		}
		if ( w.HasMember("lore_points") )
		{
			obj.lorePoints = w["lore_points"].GetInt();
		}
		if ( w.HasMember("feature_img") )
		{
			obj.featureImg = w["feature_img"].GetString();
		}
		obj.linesToHighlight.clear();
		for ( auto& line : obj.details )
		{
			if ( line.size() > 0 )
			{
				if ( line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				else
				{
					for ( size_t c = 0; c < line.size(); ++c )
					{
						if ( line[c] == '-' )
						{
							line[c] = '\x1E';
							break;
						}
						else if ( line[c] != ' ' )
						{
							break;
						}
					}
				}
			}
		}
		if ( w.HasMember("details_line_highlights") )
		{
			for ( auto itr = w["details_line_highlights"].Begin(); itr != w["details_line_highlights"].End(); ++itr )
			{
				if ( itr->HasMember("color") )
				{
					Uint8 r, g, b;
					if ( (*itr)["color"].HasMember("r") )
					{
						r = (*itr)["color"]["r"].GetInt();
					}
					if ( (*itr)["color"].HasMember("g") )
					{
						g = (*itr)["color"]["g"].GetInt();
					}
					if ( (*itr)["color"].HasMember("b") )
					{
						b = (*itr)["color"]["b"].GetInt();
					}

					obj.linesToHighlight.push_back(makeColorRGB(r, g, b));
				}
				else
				{
					obj.linesToHighlight.push_back(0);
				}
			}
		}

		Compendium_t::Events_t::eventWorldIDLookup[name] = obj.id;
		Compendium_t::Events_t::worldIDToString[obj.id + Compendium_t::Events_t::kEventWorldOffset] = name;
		if ( w.HasMember("events") )
		{
			for ( auto itr = w["events"].Begin(); itr != w["events"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						Compendium_t::Events_t::eventWorldLookup[(Compendium_t::EventTags)find2->second.id].insert(name);
					}
				}
			}
		}
		Compendium_t::Events_t::itemDisplayedEventsList.erase(Compendium_t::Events_t::kEventWorldOffset + obj.id);
		Compendium_t::Events_t::itemDisplayedCustomEventsList.erase(Compendium_t::Events_t::kEventWorldOffset + obj.id);
		if ( w.HasMember("events_display") )
		{
			for ( auto itr = w["events_display"].Begin(); itr != w["events_display"].End(); ++itr )
			{
				std::string eventName = itr->GetString();
				auto find = Compendium_t::Events_t::eventIdLookup.find(eventName);
				if ( find != Compendium_t::Events_t::eventIdLookup.end() )
				{
					auto find2 = Compendium_t::Events_t::events.find(find->second);
					if ( find2 != Compendium_t::Events_t::events.end() )
					{
						auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
						if ( std::find(vec.begin(), vec.end(), (Compendium_t::EventTags)find2->second.id)
							== vec.end() || find2->second.id == EventTags::CPDM_CUSTOM_TAG )
						{
							vec.push_back((Compendium_t::EventTags)find2->second.id);
						}
					}
				}
			}
		}
		if ( w.HasMember("custom_events_display") )
		{
			std::vector<std::string> customEvents;
			for ( auto itr = w["custom_events_display"].Begin(); itr != w["custom_events_display"].End(); ++itr )
			{
				customEvents.push_back(itr->GetString());
			}

			auto& vec = Compendium_t::Events_t::itemDisplayedEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
			int index = -1;
			for ( auto& v : vec )
			{
				++index;
				auto& vec2 = Compendium_t::Events_t::itemDisplayedCustomEventsList[Compendium_t::Events_t::kEventWorldOffset + obj.id];
				if ( v == EventTags::CPDM_CUSTOM_TAG )
				{
					if ( index < customEvents.size() )
					{
						vec2.push_back(customEvents[index]);
					}
					else
					{
						vec2.push_back("");
					}
				}
				else
				{
					vec2.push_back("");
				}
			}
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : worldObjects )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.details )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("details", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& color : obj.second.linesToHighlight )
				{
					rapidjson::Value val(rapidjson::kObjectType);
					if ( color == 0 )
					{
					}
					else
					{
						Uint8 r, g, b, a;
						getColor(color, &r, &g, &b, &a);

						rapidjson::Value colorVal(rapidjson::kObjectType);
						colorVal.AddMember("r", r, d.GetAllocator());
						colorVal.AddMember("g", g, d.GetAllocator());
						colorVal.AddMember("b", b, d.GetAllocator());
						colorVal.AddMember("a", a, d.GetAllocator());

						val.AddMember("color", colorVal, d.GetAllocator());
					}
					arr.PushBack(val, d.GetAllocator());
				}
				entry.AddMember("details_line_highlights", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_world.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

std::map<std::string, int> Compendium_t::Events_t::monsterUniqueIDLookup;
std::map<Compendium_t::EventTags, std::set<int>> Compendium_t::Events_t::eventMonsterLookup;
void Compendium_t::readMonstersTranslationsFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "lang/compendium_lang/lang_monsters.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMonstersTranslationsFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	for ( auto itr = d.MemberBegin(); itr != d.MemberEnd(); ++itr )
	{
		std::string key = itr->name.GetString();
		auto find = monsters.find(key);
		if ( find != monsters.end() )
		{
			find->second.blurb.clear();
			if ( itr->value.HasMember("blurb") )
			{
				jsonVecToVec(itr->value["blurb"], find->second.blurb);
			}
			find->second.abilities.clear();
			if ( itr->value.HasMember("abilities") )
			{
				jsonVecToVec(itr->value["abilities"], find->second.abilities);
			}
			find->second.inventory.clear();
			if ( itr->value.HasMember("inventory") )
			{
				jsonVecToVec(itr->value["inventory"], find->second.inventory);
			}
		}
	}
}

void Compendium_t::readMonstersFromFile(bool forceLoadBaseDirectory)
{
	const std::string filename = "data/compendium/monsters.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	if ( forceLoadBaseDirectory )
	{
		inputPath = BASE_DATA_DIR;
	}
	else
	{
		if ( inputPath != BASE_DATA_DIR )
		{
			readMonstersFromFile(true); // force load the base directory first, then modded paths later.
		}
		else
		{
			forceLoadBaseDirectory = true;
		}
	}

	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("monsters") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	monsters.clear();

	Compendium_t::Events_t::monsterUniqueIDLookup.clear();
	Compendium_t::Events_t::eventMonsterLookup.clear();
	Compendium_t::Events_t::monsterIDToString.clear();
	
	Compendium_t::CompendiumMonsters_t::readContentsLang();

	if ( d.HasMember("unique_tags") )
	{
		for ( auto itr = d["unique_tags"].MemberBegin(); itr != d["unique_tags"].MemberEnd(); ++itr )
		{
			Compendium_t::Events_t::monsterUniqueIDLookup[itr->name.GetString()] = itr->value.GetInt();
		}
	}

	for ( int i = 0; i < NUMMONSTERS; ++i )
	{
		int type = i + Compendium_t::Events_t::kEventMonsterOffset;
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_SOLO].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_PARTY].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_MULTIPLAYER].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_BY].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_RECRUITED].insert(type);
		Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILL_XP].insert(type);
		if ( i == GYROBOT )
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GYROBOT_FLIPS].insert(type);
		}

		Compendium_t::Events_t::monsterIDToString[type] = monstertypename[i];
	}
	for ( auto pair : Compendium_t::Events_t::monsterUniqueIDLookup )
	{
		int type = pair.second + Compendium_t::Events_t::kEventMonsterOffset;
		Compendium_t::Events_t::monsterIDToString[type] = pair.first;
		if ( pair.first == "ghost" )
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_SPAWNED].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_TELEPORTS].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_PINGS].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_GHOST_PUSHES].insert(type);
		}
		else
		{
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_SOLO].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_PARTY].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_MULTIPLAYER].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILLED_BY].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_RECRUITED].insert(type);
			Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_KILL_XP].insert(type);

			if ( pair.first == "mysterious shop" )
			{
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_MERCHANT_ORBS].insert(type);
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_SHOP_BOUGHT].insert(type);
				Compendium_t::Events_t::eventMonsterLookup[EventTags::CPDM_SHOP_SPENT].insert(type);
			}
		}
	}

	auto& entries = d["monsters"];
	for ( auto itr = entries.MemberBegin(); itr != entries.MemberEnd(); ++itr )
	{
		std::string name = itr->name.GetString();
		if ( itr->value.HasMember("type") )
		{
			name = itr->value["type"].GetString();
		}
		int monsterType = NOTHING;
		for ( int i = 0; i < NUMMONSTERS; ++i )
		{
			if ( name == monstertypename[i] )
			{
				monsterType = i;
				break;
			}
		}

		if ( monsterType == NOTHING && name != "ghost" ) { continue; }

		auto& m = itr->value;
		auto& monster = monsters[itr->name.GetString()];
		monster.monsterType = monsterType;
		monster.unique_npc = m.HasMember("unique_npc") ? m["unique_npc"].GetString() : "";
		if ( m.HasMember("blurb") )
		{
			jsonVecToVec(m["blurb"], monster.blurb);
		}
		if ( m.HasMember("img") )
		{
			monster.imagePath = m["img"].GetString();
		}
		if ( m.HasMember("lore_points") )
		{
			monster.lorePoints = m["lore_points"].GetInt();
		}
		auto& stats = m["stats"];
		jsonVecToVec(stats["hp"], monster.hp);
		jsonVecToVec(stats["ac"], monster.ac);
		jsonVecToVec(stats["spd"], monster.spd);
		jsonVecToVec(stats["atk"], monster.atk);
		jsonVecToVec(stats["rangeatk"], monster.rangeatk);
		jsonVecToVec(stats["pwr"], monster.pwr);
		if ( stats.HasMember("str") )
		{
			jsonVecToVec(stats["str"], monster.str);
		}
		if ( stats.HasMember("con") )
		{
			jsonVecToVec(stats["con"], monster.con);
		}
		if ( stats.HasMember("dex") )
		{
			jsonVecToVec(stats["dex"], monster.dex);
		}
		if ( stats.HasMember("lvl") )
		{
			jsonVecToVec(stats["lvl"], monster.lvl);
		}
		if ( stats.HasMember("species") )
		{
			auto species = CompendiumMonsters_t::SPECIES_NONE;
			std::string str = stats["species"].GetString();
			if ( str == "humanoid" )
			{
				species = CompendiumMonsters_t::SPECIES_HUMANOID;
			}
			else if ( str == "beast" )
			{
				species = CompendiumMonsters_t::SPECIES_BEAST;
			}
			else if ( str == "beastfolk" )
			{
				species = CompendiumMonsters_t::SPECIES_BEASTFOLK;
			}
			else if ( str == "undead" )
			{
				species = CompendiumMonsters_t::SPECIES_UNDEAD;
			}
			else if ( str == "demonoid" )
			{
				species = CompendiumMonsters_t::SPECIES_DEMONOID;
			}
			else if ( str == "construct" )
			{
				species = CompendiumMonsters_t::SPECIES_CONSTRUCT;
			}
			else if ( str == "elemental" )
			{
				species = CompendiumMonsters_t::SPECIES_ELEMENTAL;
			}
			monster.species = species;
		}
		if ( m.HasMember("abilities") )
		{
			jsonVecToVec(m["abilities"], monster.abilities);
		}
		{
			int i = 0;
			for ( auto itr = m["resistances"].Begin(); itr != m["resistances"].End(); ++itr )
			{
				monster.resistances[i] = itr->GetInt();
				++i;
			}
		}
		if ( m.HasMember("inventory") )
		{
			jsonVecToVec(m["inventory"], monster.inventory);
		}
		if ( m.HasMember("models") )
		{
			jsonVecToVec(m["models"], monster.models);
		}
	}

	/*if ( keystatus[SDLK_g] )
	{
		keystatus[SDLK_g] = 0;
		rapidjson::Document d;
		d.SetObject();
		for ( auto& obj : monsters )
		{
			rapidjson::Value entry(rapidjson::kObjectType);

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.blurb )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("blurb", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.abilities )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("abilities", arr, d.GetAllocator());
			}

			{
				rapidjson::Value arr(rapidjson::kArrayType);
				for ( auto& str : obj.second.inventory )
				{
					arr.PushBack(rapidjson::Value(str.c_str(), d.GetAllocator()), d.GetAllocator());
				}
				entry.AddMember("inventory", arr, d.GetAllocator());
			}

			d.AddMember(rapidjson::Value(obj.first.c_str(), d.GetAllocator()), entry, d.GetAllocator());
		}

		char path[PATH_MAX] = "";
		completePath(path, "lang/compendium_lang/lang_monsters.json", outputdir);

		File* fp = FileIO::open(path, "wb");
		if ( !fp )
		{
			printlog("[JSON]: Error opening json file %s for write!", path);
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);
	}*/
}

Uint32 Compendium_t::lastTickUpdate = 0;
std::map<Compendium_t::EventTags, Compendium_t::Events_t::Event_t> Compendium_t::Events_t::events;
std::map<std::string, Compendium_t::EventTags> Compendium_t::Events_t::eventIdLookup;
std::map<int, std::set<Compendium_t::EventTags>> Compendium_t::Events_t::itemEventLookup;
std::map<Compendium_t::EventTags, std::set<int>> Compendium_t::Events_t::eventItemLookup;
std::map<Compendium_t::EventTags, std::set<std::string>> Compendium_t::Events_t::eventWorldLookup;
std::map<Compendium_t::EventTags, std::set<std::string>> Compendium_t::Events_t::eventCodexLookup;
std::map<std::string, int> Compendium_t::Events_t::eventWorldIDLookup;
std::map<std::string, int> Compendium_t::Events_t::eventCodexIDLookup;
std::map<Compendium_t::EventTags, std::map<int, int>> Compendium_t::Events_t::eventClassIds;
std::map<int, std::vector<Compendium_t::EventTags>> Compendium_t::Events_t::itemDisplayedEventsList;
std::map<int, std::vector<std::string>> Compendium_t::Events_t::itemDisplayedCustomEventsList;
std::map<std::string, std::string> Compendium_t::Events_t::customEventsValues;
std::map<Compendium_t::EventTags, std::map<int, Compendium_t::Events_t::EventVal_t>> Compendium_t::Events_t::playerEvents;
std::map<Compendium_t::EventTags, std::map<int, Compendium_t::Events_t::EventVal_t>> Compendium_t::Events_t::serverPlayerEvents[MAXPLAYERS];
std::map<Compendium_t::EventTags, std::map<std::string, std::string>> Compendium_t::Events_t::eventLangEntries;
std::map<std::string, std::map<std::string, std::string>> Compendium_t::Events_t::eventCustomLangEntries;

void Compendium_t::Events_t::readEventsTranslations()
{
	const std::string filename = "data/compendium/events_text.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	eventLangEntries.clear();
	eventCustomLangEntries.clear();
	for ( auto itr = d["tags"].Begin(); itr != d["tags"].End(); ++itr )
	{
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			auto find = eventIdLookup.find(itr2->name.GetString());
			if ( find != eventIdLookup.end())
			{
				EventTags tag = eventIdLookup[find->first];
				auto& entry = eventLangEntries[tag];
				for ( auto itr3 = itr2->value.MemberBegin(); itr3 != itr2->value.MemberEnd(); ++itr3 )
				{
					entry[itr3->name.GetString()] = itr3->value.GetString();
				}
			}
		}
	}
	if ( d.HasMember("custom_tags") )
	{
		for ( auto itr = d["custom_tags"].MemberBegin(); itr != d["custom_tags"].MemberEnd(); ++itr )
		{
			for ( auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2 )
			{
				eventCustomLangEntries[itr->name.GetString()][itr2->name.GetString()] = itr2->value.GetString();
			}
		}
	}
}

std::string Compendium_t::Events_t::formatEventRecordText(Sint32 value, const char* formatType, int formatVal, std::map<std::string, std::string>& langMap)
{
	std::string resultsFormatting = "%d";
	if ( formatType && !strcmp(formatType, "cycle") )
	{
		std::string fmt = "format";
		fmt += std::to_string(formatVal);

		if ( langMap.find(fmt) != langMap.end() )
		{
			resultsFormatting = langMap[fmt];
		}
	}
	else if ( formatType && itemIDToString.find(formatVal) != itemIDToString.end()
		&& itemIDToString.at(formatVal) == formatType )
	{
		std::string fmt = "format_";
		fmt += formatType;
		if ( langMap.find(fmt) != langMap.end() )
		{
			resultsFormatting = langMap[fmt];
		}
		else if ( langMap.find("format") != langMap.end() )
		{
			resultsFormatting = langMap["format"];
		}
		else
		{
			return std::to_string(value);
		}
	}
	else if ( langMap.find("format") != langMap.end() )
	{
		resultsFormatting = langMap["format"];
	}
	else
	{
		return std::to_string(value);
	}

	std::string output = "";
	for ( size_t c = 0; c < resultsFormatting.size(); ++c )
	{
		if ( resultsFormatting[c] == '%' && ((c + 1) < resultsFormatting.size()) )
		{
			if ( resultsFormatting[c + 1] == 'm' )
			{
				// dist to meters
				float meters = value / (8.f);
				char buf[32];
				if ( meters >= 1000.f )
				{
					float km = meters / 1000.f;
					snprintf(buf, sizeof(buf), "%.2f km", km);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%.1f m", meters);
				}
				output += buf;
			}
			else if ( resultsFormatting[c + 1] == '$' )
			{
				int gold = value;
				// gold amounts
				char buf[32];
				if ( gold >= 1000.f )
				{
					float k = gold / 1000.f;
					snprintf(buf, sizeof(buf), "%.3fk ", k);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%d", gold);
				}
				output += buf;
			}
			else if ( resultsFormatting[c + 1] == 't' )
			{
				if ( langMap.find("format_time") != langMap.end() )
				{
					int numSymbols = 0;
					std::string fmt = langMap["format_time"];
					for ( size_t c = 0; c < fmt.size(); ++c )
					{
						if ( fmt[c] == '%' )
						{
							numSymbols++;
						}
					}
					Uint32 sec = (value / TICKS_PER_SECOND) % 60;
					Uint32 min = ((value / TICKS_PER_SECOND) / 60);
					Uint32 hour = (((value / TICKS_PER_SECOND) / 60) / 60);
					Uint32 day = ((value / TICKS_PER_SECOND) / 60) / 60 / 24;
					char buf[32] = "";
					if ( numSymbols == 2 )
					{
						// mins/secs
						min = std::min(min, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), min, sec);
						output += buf;
					}
					else if ( numSymbols == 3 )
					{
						// hours/mins/secs
						min = min % 60;
						hour = std::min(hour, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), hour, min, sec);
						output += buf;
					}
					else if ( numSymbols == 4 )
					{
						// days also
						min = min % 60;
						hour = hour % 24;
						day = std::min(day, (Uint32)9999);
						snprintf(buf, sizeof(buf), fmt.c_str(), day, hour, min, sec);
						output += buf;
					}
				}
				else
				{
					// ticks to seconds
					float seconds = value / (50.f);
					char buf[32];
					snprintf(buf, sizeof(buf), "%.1f", seconds);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == 'd' )
			{
				output += std::to_string(value);
			}
			else if ( resultsFormatting[c + 1] == 'p' )
			{
				if ( value >= 0 )
				{
					output += '+';
				}
				output += std::to_string(value);
			}
			else if ( resultsFormatting[c + 1] == 'h' )
			{
				real_t regen = value;
				if ( regen > 0.01 )
				{
					real_t nominalRegen = HEAL_TIME;
					regen = nominalRegen / regen;
					char buf[32];
					snprintf(buf, sizeof(buf), "%.f", regen * 100.0);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == 'e' )
			{
				real_t regen = value;
				if ( regen > 0.01 )
				{
					real_t nominalRegen = MAGIC_REGEN_TIME;
					regen = nominalRegen / regen;
					char buf[32];
					snprintf(buf, sizeof(buf), "%.f", regen * 100.0);
					output += buf;
				}
			}
			else if ( resultsFormatting[c + 1] == '%' )
			{
				output += '%';
			}
			else if ( resultsFormatting[c + 1] == 's' )
			{
				if ( formatType )
				{
					if ( !strcmp(formatType, "stats") )
					{
						switch ( formatVal )
						{
						case STAT_STR:
							output += ItemTooltips.getItemStatShortName("STR");
							break;
						case STAT_DEX:
							output += ItemTooltips.getItemStatShortName("DEX");
							break;
						case STAT_CON:
							output += ItemTooltips.getItemStatShortName("CON");
							break;
						case STAT_INT:
							output += ItemTooltips.getItemStatShortName("INT");
							break;
						case STAT_PER:
							output += ItemTooltips.getItemStatShortName("PER");
							break;
						case STAT_CHR:
							output += ItemTooltips.getItemStatShortName("CHR");
							break;
						}
					}
					else if ( !strcmp(formatType, "class") )
					{
						std::string tmp = playerClassLangEntry(formatVal, 0);
						camelCaseString(tmp);
						output += tmp;
					}
					else if ( !strcmp(formatType, "race") )
					{
						std::string tmp = getMonsterLocalizedName(getMonsterFromPlayerRace(formatVal));
						camelCaseString(tmp);
						output += tmp;
					}
					else if ( !strcmp(formatType, "skills") )
					{
						std::string tmp = getSkillLangEntry(formatVal);
						camelCaseString(tmp);
						output += tmp;
					}
				}
			}
			++c;
		}
		else
		{
			output += resultsFormatting[c];
		}
	}
	return output;
}

std::vector<std::pair<std::string, Sint32>> Compendium_t::Events_t::getCustomEventValue(std::string key, 
	std::string compendiumSection, std::string compendiumContentsSelected, int specificClass)
{
	std::vector<std::pair<std::string, Sint32>> results;
	if ( customEventsValues.find(key) == customEventsValues.end() )
	{
		return results;
	}

	rapidjson::Document d;
	d.Parse(customEventsValues[key].c_str());
	if ( !d.IsObject() )
	{
		return results;
	}

	if ( d.HasMember("value") )
	{
		Events_t::Type type = MAX;
		int minValue = INT_MAX;
		int maxValue = 0;
		bool firstResult = true;
		std::string valueType = "";
		if ( d["value"].HasMember("type") )
		{
			valueType = d["value"]["type"].GetString();
			if ( valueType == "max" )
			{
				type = MAX;
			}
			else if ( valueType == "min" )
			{
				type = MIN;
			}
			else if ( valueType == "sum" )
			{
				type = SUM;
			}
			else if ( valueType == "class_sum" )
			{
				type = SUM;
			}
		}

		int specificItemId = -1;
		if ( compendiumSection == "items" || compendiumSection == "magic" )
		{
			specificItemId = specificClass;
			specificClass = -1;
		}
		else
		{
			if ( valueType != "class_sum" )
			{
				specificClass = -1;
			}
		}

		std::map<int, int> mapValueTotals;
		std::string formatType = "";
		int cycleResults = 0;
		bool foundTag = false;
		if ( d["value"].HasMember("format") )
		{
			formatType = d["value"]["format"].GetString();
		}
		if ( d["value"].HasMember("compendium_section") )
		{
			compendiumSection = d["value"]["compendium_section"].GetString();
		}
		if ( d["value"].HasMember("tags") )
		{
			for ( auto itr = d["value"]["tags"].Begin(); itr != d["value"]["tags"].End(); ++itr )
			{
				std::string name = (*itr)["name"].GetString();
				std::string cat = (*itr)["category"].GetString();

				if ( compendiumSection == "items" || compendiumSection == "magic" )
				{
					// items dont use the category header by default, either specific item or current viewed item
				}
				else
				{
					if ( cat == "" )
					{
						cat = compendiumContentsSelected;
					}
				}

				if ( valueType == "sum_items" )
				{
					auto findTag = eventIdLookup.find(name);
					if ( findTag != eventIdLookup.end() )
					{
						auto& playerTags = playerEvents[findTag->second];
						for ( auto itemId : eventItemLookup[findTag->second] )
						{
							if ( cat == "spells" )
							{
								if ( itemId >= kEventSpellOffset )
								{
									int spellID = itemId - kEventSpellOffset;
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "spellbooks" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && ::items[itemId].category == SPELLBOOK )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "equipment" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && ::items[itemId].item_slot != NO_EQUIP )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "armor" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS && 
									::items[itemId].item_slot != NO_EQUIP
									&& ::items[itemId].item_slot != EQUIPPABLE_IN_SLOT_WEAPON )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
							else if ( cat == "weapons" )
							{
								if ( itemId >= WOODEN_SHIELD && itemId < NUMITEMS
									&& ::items[itemId].item_slot == EQUIPPABLE_IN_SLOT_WEAPON )
								{
									auto findVal = playerTags.find(itemId);
									if ( findVal != playerTags.end() )
									{
										mapValueTotals[itemId] += findVal->second.value;
									}
								}
							}
						}
					}
					continue;
				}

				int foundId = -1;
				if ( compendiumSection == "items" || compendiumSection == "magic" )
				{
					if ( cat == "" )
					{
						if ( itemIDToString.find(specificItemId) != itemIDToString.end() )
						{
							foundId = specificItemId;
						}
					}
					else
					{
						auto find = ItemTooltips.itemNameStringToItemID.find(cat);
						if ( find != ItemTooltips.itemNameStringToItemID.end() )
						{
							foundId = find->second;
						}
					}
				}
				else
				{
					auto& eventSectionIDLookup = compendiumSection == "codex" ? eventCodexIDLookup : eventWorldIDLookup;
					auto findCat = eventSectionIDLookup.find(cat);
					if ( findCat != eventSectionIDLookup.end() )
					{
						foundId = findCat->second;
					}
				}
				if ( foundId >= 0 )
				{
					auto findTag = eventIdLookup.find(name);
					if ( findTag != eventIdLookup.end() )
					{
						auto tag = findTag->second;
						if ( playerEvents.find(tag) == playerEvents.end() )
						{
							if ( valueType == "list" )
							{
								results.push_back(std::make_pair("-", 0));
							}
							++cycleResults;
							continue;
						}
						auto& playerTags = playerEvents[tag];
						std::vector<std::pair<int, int>> codexIDs;
						codexIDs.push_back(std::make_pair(-1, foundId));

						bool foundLookup = false;
						if ( compendiumSection == "items" || compendiumSection == "magic" )
						{
							if ( eventItemLookup[tag].find(foundId) != eventItemLookup[tag].end() )
							{
								foundLookup = true;
							}
						}
						else
						{
							auto& eventSectionLookup = compendiumSection == "codex" ? eventCodexLookup : eventWorldLookup;
							if ( eventSectionLookup[tag].find(cat) != eventSectionLookup[tag].end() )
							{
								foundLookup = true;
							}
						}

						if ( foundLookup )
						{
							auto& def = events[tag];
							if ( def.attributes.find("stats") != def.attributes.end() && valueType != "max_class" )
							{
								if ( cat == "str" ) { codexIDs.back().first = STAT_STR; }
								if ( cat == "dex" ) { codexIDs.back().first = STAT_DEX; }
								if ( cat == "con" ) { codexIDs.back().first = STAT_CON; }
								if ( cat == "int" ) { codexIDs.back().first = STAT_INT; }
								if ( cat == "per" ) { codexIDs.back().first = STAT_PER; }
								if ( cat == "chr" ) { codexIDs.back().first = STAT_CHR; }
							}
							else if ( def.attributes.find("class") != def.attributes.end() )
							{
								codexIDs.clear();
								auto findClassTag = eventClassIds.find(tag);
								if ( findClassTag != eventClassIds.end() )
								{
									// iterate through classes
									int startOffsetId = -1;
									if ( def.attributes.find("skills") != def.attributes.end() )
									{
										for ( int i = 0; i < NUMPROFICIENCIES; ++i )
										{
											if ( cat == getSkillStringForCompendium(i) )
											{
												startOffsetId = findClassTag->second[0] + i * kEventClassesMax;
												break;
											}
										}
									}

									for ( auto classId : findClassTag->second )
									{
										if ( startOffsetId >= 0 )
										{
											if ( classId.second >= startOffsetId && classId.second < startOffsetId + kEventClassesMax )
											{
												if ( specificClass >= 0 )
												{
													if ( (classId.first % kEventClassesMax) != specificClass )
													{
														continue;
													}
												}
												codexIDs.push_back(classId);
												codexIDs.back().first = codexIDs.back().first % kEventClassesMax;
											}
										}
										else
										{
											if ( specificClass >= 0 )
											{
												if ( classId.first != specificClass )
												{
													continue;
												}
											}

											codexIDs.push_back(classId);
											if ( def.attributes.find("stats") != def.attributes.end() && valueType == "max_class" )
											{
												// we want to store the stat names rather than the class
												if ( cat == "str" ) { codexIDs.back().first = STAT_STR; }
												if ( cat == "dex" ) { codexIDs.back().first = STAT_DEX; }
												if ( cat == "con" ) { codexIDs.back().first = STAT_CON; }
												if ( cat == "int" ) { codexIDs.back().first = STAT_INT; }
												if ( cat == "per" ) { codexIDs.back().first = STAT_PER; }
												if ( cat == "chr" ) { codexIDs.back().first = STAT_CHR; }
											}
										}
									}
								}
							}
							else if ( def.attributes.find("race") != def.attributes.end() )
							{
								codexIDs.clear();
								auto findClassTag = eventClassIds.find(tag);
								if ( findClassTag != eventClassIds.end() )
								{
									// iterate through classes
									for ( auto classId : findClassTag->second )
									{
										codexIDs.push_back(classId);
									}
								}
							}

							for ( auto& pair : codexIDs )
							{
								int codexID = pair.second;
								int classnum = pair.first;

								if ( formatType == "skills" )
								{
									for ( int i = 0; i < NUMPROFICIENCIES; ++i )
									{
										if ( cat == getSkillStringForCompendium(i) )
										{
											classnum = i;
											break;
										}
									}
								}

								if ( compendiumSection == "codex" )
								{
									if ( codexID < kEventCodexOffset )
									{
										codexID += kEventCodexOffset; // convert to offset
									}
								}
								else if ( compendiumSection == "world" )
								{
									if ( codexID < kEventWorldOffset )
									{
										codexID += kEventWorldOffset; // convert to offset
									}
								}

								auto findVal = playerTags.find(codexID);
								int val = 0;
								if ( findVal != playerTags.end() )
								{
									val = findVal->second.value;
									foundTag = true;
								}

								std::string output = "";
								int numFormats = 0;
								if ( valueType == "sum_category_max" || valueType == "sum_category_min" || valueType == "sum_category_cycle" )
								{
									int categoryValue = foundId;
									if ( formatType == "skills" )
									{
										for ( int i = 0; i < NUMPROFICIENCIES; ++i )
										{
											if ( cat == getSkillStringForCompendium(i) )
											{
												categoryValue = i;
												break;
											}
										}
									}
									mapValueTotals[categoryValue] += val;
								}
								else if ( valueType == "class_max_total" )
								{
									mapValueTotals[classnum] += val;
									continue;
								}
								else if ( formatType == "skills" )
								{
									output = formatEventRecordText(val, d["value"]["format"].GetString(), classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("stats") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "stats", classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("class") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "class", classnum, eventCustomLangEntries[key]);
								}
								else if ( def.attributes.find("race") != def.attributes.end() )
								{
									output = formatEventRecordText(val, "race", classnum, eventCustomLangEntries[key]);
								}
								else if ( valueType == "cycle" )
								{
									if ( findVal != playerTags.end() )
									{
										output = formatEventRecordText(val, valueType.c_str(), cycleResults, eventCustomLangEntries[key]);
									}
									else
									{
										output = "-";
									}
								}

								results.push_back(std::make_pair(output, val));
								maxValue = std::max(maxValue, val);

								if ( findVal != playerTags.end() )
								{
									if ( firstResult )
									{
										minValue = val;
									}
									else
									{
										minValue = std::min(minValue, val);
									}
									firstResult = false;
								}

								++cycleResults;
							}
						}
					}
				}
			}
		}

		Sint32 sum = 0;
		if ( valueType == "cycle" )
		{
			bool filledEntry = false;
			for ( auto& pair : results )
			{
				if ( pair.first != "-" )
				{
					filledEntry = true;
				}
			}
			for ( auto itr = results.begin(); itr != results.end(); )
			{
				if ( filledEntry && itr->first == "-" )
				{
					itr = results.erase(itr); // don't display empty entries if something has data in it
				}
				else
				{
					++itr;
				}
			}
			return results;
		}
		else if ( valueType == "sum_items" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				sum += pair.second;
			}
			std::string output = formatEventRecordText(sum, formatType.c_str(), 0, eventCustomLangEntries[key]);
			results.push_back(std::make_pair(output, maxValue));
			return results;
		}
		else if ( valueType == "sum_category_max" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				maxValue = std::max(maxValue, pair.second);
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == maxValue )
				{
					std::string output = formatEventRecordText(maxValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, maxValue));
				}
			}
			return results;
		}
		else if ( valueType == "sum_category_cycle" )
		{
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				std::string output = formatEventRecordText(pair.second, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
				results.push_back(std::make_pair(output, pair.second));
			}
			return results;
		}
		else if ( valueType == "sum_category_min" )
		{
			results.clear();
			firstResult = true;
			minValue = 0;
			for ( auto& pair : mapValueTotals )
			{
				if ( firstResult )
				{
					minValue = pair.second;
				}
				else
				{
					minValue = std::min(minValue, pair.second);
				}
				firstResult = false;
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == minValue )
				{
					std::string output = formatEventRecordText(minValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, minValue));
				}
			}
			return results;
		}
		else if ( valueType == "class_max_total" )
		{
			maxValue = 0;
			results.clear();
			for ( auto& pair : mapValueTotals )
			{
				maxValue = std::max(maxValue, pair.second);
			}
			for ( auto& pair : mapValueTotals )
			{
				if ( pair.second == maxValue )
				{
					std::string output = formatEventRecordText(maxValue, formatType.c_str(), pair.first, eventCustomLangEntries[key]);
					results.push_back(std::make_pair(output, maxValue));
				}
			}
			return results;
		}
		else if ( valueType == "list" )
		{
			if ( eventCustomLangEntries[key].find("format") != eventCustomLangEntries[key].end() )
			{
				if ( results.size() >= 1 )
				{
					char buf[1024] = "";
					snprintf(buf, sizeof(buf), eventCustomLangEntries[key]["format"].c_str(),
						results[0].first != "-" ? std::to_string(results[0].second).c_str() : "-",
						results[1].first != "-" ? std::to_string(results[1].second).c_str() : "-");
					results.clear();
					results.push_back(std::make_pair(buf, 0));
				}
				else
				{
					char buf[1024] = "";
					snprintf(buf, sizeof(buf), eventCustomLangEntries[key]["format"].c_str(),
						"-",
						"-");
					results.clear();
					results.push_back(std::make_pair(buf, 0));
				}
				return results;
			}
		}
		else
		{
			for ( auto itr = results.begin(); itr != results.end(); )
			{
				if ( type == MAX && itr->second != maxValue )
				{
					itr = results.erase(itr);
				}
				else if ( type == MIN && itr->second != minValue )
				{
					itr = results.erase(itr);
				}
				else
				{
					if ( type == SUM )
					{
						sum += itr->second;
					}
					++itr;
				}
			}
		}
		if ( type == SUM && results.size() > 0 )
		{
			if ( foundTag )
			{
				results.clear();
				results.push_back(std::make_pair(formatEventRecordText(sum, nullptr, 0, eventCustomLangEntries[key]), sum));
			}
			else
			{
				results.clear();
			}
		}
		else if ( (type == MIN || type == MAX) && !foundTag )
		{
			results.clear();
		}
	}

	return results;
}

void Compendium_t::Events_t::readEventsFromFile()
{
	const std::string filename = "data/compendium/events.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	char buf[120000];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("tags") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	events.clear();
	eventIdLookup.clear();
	eventClassIds.clear();
	int classIdIndex = kEventCodexClassOffset;
	int index = -1;
	for ( auto itr = d["tags"].Begin(); itr != d["tags"].End(); ++itr )
	{
		++index;
		for ( auto itr2 = itr->MemberBegin(); itr2 != itr->MemberEnd(); ++itr2 )
		{
			const EventTags id = (EventTags)std::min(index, (int)CPDM_EVENT_TAGS_MAX);
			auto& entry = events[id];
			entry.id = id;
			entry.name = itr2->name.GetString();
			eventIdLookup[entry.name] = id;


			if ( itr2->value.HasMember("type") )
			{
				std::string type = itr2->value["type"].GetString();
				if ( type == "sum" )
				{
					entry.type = SUM;
				}
				else if ( type == "max" )
				{
					entry.type = MAX;
				}
				else if ( type == "bit" )
				{
					entry.type = BITFIELD;
				}
				else if ( type == "min" )
				{
					entry.type = MIN;
				}
			}
			entry.id = index;
			if ( itr2->value.HasMember("client") )
			{
				int tmp = itr2->value["client"].GetInt();
				tmp = std::max(0, tmp);
				tmp = std::min((int)Compendium_t::Events_t::CLIENT_UPDATETYPE_MAX - 1, tmp);
				entry.clienttype = (Compendium_t::Events_t::ClientUpdateType)tmp;
			}
			if ( itr2->value.HasMember("once_per_run") )
			{
				entry.eventTrackingType = itr2->value["once_per_run"].GetBool() ? EventTrackingType::ONCE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("unique_per_run") )
			{
				entry.eventTrackingType = itr2->value["unique_per_run"].GetBool() ? EventTrackingType::UNIQUE_PER_RUN : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("unique_per_floor") )
			{
				entry.eventTrackingType = itr2->value["unique_per_floor"].GetBool() ? EventTrackingType::UNIQUE_PER_FLOOR : EventTrackingType::ALWAYS_UPDATE;
			}
			if ( itr2->value.HasMember("attributes") )
			{
				if ( itr2->value["attributes"].IsArray() )
				{
					for ( auto arr_itr = itr2->value["attributes"].Begin(); arr_itr != itr2->value["attributes"].End(); ++arr_itr )
					{
						if ( arr_itr->IsString() )
						{
							entry.attributes.insert(arr_itr->GetString());
						}
					}
				}
				else if ( itr2->value["attributes"].IsString() )
				{
					entry.attributes.insert(itr2->value["attributes"].GetString());
				}
			}
			if ( entry.attributes.find("class") != entry.attributes.end() || entry.attributes.find("race") != entry.attributes.end() )
			{
				if ( entry.attributes.find("skills") != entry.attributes.end() )
				{
					for ( int skillnum = 0; skillnum < 16; ++skillnum )
					{
						for ( int i = 0; i <= CLASS_HUNTER; ++i )
						{
							int index = i + skillnum * kEventClassesMax;
							eventClassIds[id][index] = (classIdIndex + index);
						}
					}
					classIdIndex += kEventClassesMax * 16;
				}
				else
				{
					for ( int i = 0; i <= CLASS_HUNTER; ++i )
					{
						eventClassIds[id][i] = (classIdIndex + i);
					}
					classIdIndex += kEventClassesMax;
				}
			}
		}
	}

	if ( d.HasMember("custom_tags") )
	{
		for ( auto itr = d["custom_tags"].MemberBegin(); itr != d["custom_tags"].MemberEnd(); ++itr )
		{
			eventIdLookup[itr->name.GetString()] = EventTags::CPDM_CUSTOM_TAG;

			rapidjson::StringBuffer os;
			os.Clear();
			rapidjson::Writer<rapidjson::StringBuffer> writer(os);
			itr->value.Accept(writer);

			customEventsValues[itr->name.GetString()] = os.GetString();
		}
	}
}

void Compendium_t::Events_t::loadItemsSaveData()
{
	const std::string filename = "savegames/compendium_items.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Warning: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	const int bufSize = 360000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("items") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	playerEvents.clear();
	for ( auto itr = d["items"].MemberBegin(); itr != d["items"].MemberEnd(); ++itr )
	{
		auto find = eventIdLookup.find(itr->name.GetString());
		if ( find == eventIdLookup.end() )
		{
			continue;
		}
		const EventTags id = (EventTags)std::min((int)find->second, (int)CPDM_EVENT_TAGS_MAX);
		for ( auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2 )
		{
			int itemType = std::stoi(itr2->name.GetString());
			Sint32 value = itr2->value.GetInt();
			if ( itemType >= kEventMonsterOffset && itemType < kEventMonsterOffset + 1000 )
			{
				eventUpdateMonster(0, id, nullptr, value, true, itemType);
				continue;
			}
			if ( itemType >= kEventWorldOffset && itemType < kEventWorldOffset + 1000 )
			{
				eventUpdateWorld(0, id, nullptr, value, true, itemType - kEventWorldOffset);
				continue;
			}
			if ( itemType >= kEventCodexOffset && itemType <= kEventCodexOffsetMax )
			{
				eventUpdateCodex(0, id, nullptr, value, true, itemType);
				continue;
			}
			if ( itemType < 0 || (itemType >= NUMITEMS && itemType < kEventSpellOffset) )
			{
				continue;
			}
			if ( itemType >= kEventSpellOffset )
			{
				eventUpdate(0, id, SPELL_ITEM, value, true, itemType - kEventSpellOffset);
			}
			else
			{
				eventUpdate(0, id, (ItemType)itemType, value, true);
			}
		}
	}
}

static ConsoleVariable<bool> cvar_compendiumClientSave("/compendium_client_save", false);
static ConsoleCommand ccmd_compendium_dummy_data(
	"/compendium_dummy_data", "Create test compendium data",
	[](int argc, const char** argv) {
		if ( argc < 2 )
		{
			return;
		}
		int playernum = atoi(argv[1]);
		Compendium_t::Events_t::createDummyClientData(playernum);
	});
void Compendium_t::Events_t::createDummyClientData(const int playernum)
{
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }
	for ( int i = 0; i < NUMITEMS; ++i )
	{
		for ( auto& tag : itemEventLookup[i] )
		{
			eventUpdate(playernum, tag, (ItemType)i, 1);
		}
	}
	for ( int i = 0; i < NUM_SPELLS; ++i )
	{
		for ( auto& tag : itemEventLookup[i + kEventSpellOffset] )
		{
			eventUpdate(playernum, tag, SPELL_ITEM, 1, false, i);
		}
	}
	for ( auto& pair : eventMonsterLookup  )
	{
		for ( auto monster : pair.second )
		{
			eventUpdateMonster(playernum, pair.first, nullptr, 1, false, monster);
		}
	}
	for ( auto& pair : eventWorldLookup )
	{
		for ( auto world : pair.second )
		{
			eventUpdateWorld(playernum, pair.first, world.c_str(), 1);
		}
	}
	for ( auto& pair : eventCodexLookup )
	{
		if ( eventClassIds.find(pair.first) != eventClassIds.end() )
		{
			int oldclass = client_classes[playernum];
			for ( int c = 0; c < NUMCLASSES; ++c )
			{
				client_classes[playernum] = c;
				for ( auto world : pair.second )
				{
					eventUpdateCodex(playernum, pair.first, world.c_str(), 1);
				}
			}
			client_classes[playernum] = oldclass;
		}
		else
		{
			for ( auto world : pair.second )
			{
				eventUpdateCodex(playernum, pair.first, world.c_str(), 1);
			}
		}
	}
}

void Compendium_t::readUnlocksSaveData()
{
	CompendiumItems_t::unlocks.clear();
	CompendiumItems_t::itemUnlocks.clear();
	CompendiumWorld_t::unlocks.clear();
	CompendiumCodex_t::unlocks.clear();
	CompendiumMonsters_t::unlocks.clear();
	AchievementData_t::unlocks.clear();

	AchievementData_t::unlocks["experience"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["joker"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["teamwork"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["technique"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	AchievementData_t::unlocks["adventure"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;

	CompendiumWorld_t::unlocks["minehead"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["hall of trials"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["portcullis"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["lever"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumWorld_t::unlocks["door"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumMonsters_t::unlocks["human"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	for ( auto& data : CompendiumEntries.codex )
	{
		CompendiumCodex_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}
	/*CompendiumCodex_t::unlocks["class"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["classes list"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["races"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["stats metastats"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["melee"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["crits"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["flanking"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	CompendiumCodex_t::unlocks["backstabs"] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
	// debug stuff
	/*for ( auto& data : CompendiumEntries.worldObjects )
	{
		CompendiumWorld_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}

	for ( auto& data : CompendiumEntries.monsters )
	{
		CompendiumMonsters_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
	}*/

	for ( auto& data : CompendiumEntries.items )
	{
		for ( auto& entry : data.second.items_in_category )
		{
			// debug stuff
			/*CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
			if ( entry.itemID == TOOL_TORCH
				|| entry.itemID == BRONZE_SWORD
				|| entry.itemID == WOODEN_SHIELD
				|| entry.itemID == BRONZE_AXE
				|| entry.itemID == QUARTERSTAFF
				|| entry.itemID == BRONZE_MACE
				|| (entry.itemID == SPELL_CAT && entry.spellID == SPELL_FORCEBOLT)
				|| entry.itemID == SCROLL_BLANK
				|| entry.itemID == MAGICSTAFF_OPENING
				|| entry.itemID == MAGICSTAFF_LOCKING )
			{
				CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
					? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
					entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}

	for ( auto& data : CompendiumEntries.magic )
	{
		for ( auto& entry : data.second.items_in_category )
		{
			// debug stuff
			/*CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;*/
			if ( entry.itemID == TOOL_TORCH
				|| entry.itemID == BRONZE_SWORD
				|| entry.itemID == WOODEN_SHIELD
				|| entry.itemID == BRONZE_AXE
				|| entry.itemID == QUARTERSTAFF
				|| entry.itemID == BRONZE_MACE
				|| (entry.itemID == SPELL_ITEM && entry.spellID == SPELL_FORCEBOLT)
				|| entry.itemID == SCROLL_BLANK
				|| entry.itemID == MAGICSTAFF_OPENING
				|| entry.itemID == MAGICSTAFF_LOCKING )
			{
				CompendiumItems_t::unlocks[data.first] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				CompendiumItems_t::itemUnlocks[entry.itemID == SPELL_ITEM
					? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
					entry.itemID] = CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}


	const std::string filename = "savegames/compendium_progress.json";
	if ( !PHYSFS_getRealDir(filename.c_str()) )
	{
		printlog("[JSON]: Warning: Could not locate json file %s", filename.c_str());
		return;
	}

	std::string inputPath = PHYSFS_getRealDir(filename.c_str());
	inputPath.append(PHYSFS_getDirSeparator());
	inputPath.append(filename.c_str());

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}

	const int bufSize = 200000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.IsObject() || !d.HasMember("version") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}

	if ( d.HasMember("items") )
	{
		for ( auto itr = d["items"].MemberBegin(); itr != d["items"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumItems_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("items_status") )
	{
		for ( auto itr = d["items_status"].MemberBegin(); itr != d["items_status"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			std::string name = itr->name.GetString();
			int id = std::stoi(name);
			CompendiumItems_t::itemUnlocks[id] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("achievements") )
	{
		for ( auto itr = d["achievements"].MemberBegin(); itr != d["achievements"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			AchievementData_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("world") )
	{
		for ( auto itr = d["world"].MemberBegin(); itr != d["world"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumWorld_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("codex") )
	{
		for ( auto itr = d["codex"].MemberBegin(); itr != d["codex"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumCodex_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}

	if ( d.HasMember("monsters") )
	{
		for ( auto itr = d["monsters"].MemberBegin(); itr != d["monsters"].MemberEnd(); ++itr )
		{
			int val = itr->value.GetInt();
			if ( !(val >= CompendiumUnlockStatus::LOCKED_UNKNOWN
				&& val < CompendiumUnlockStatus::COMPENDIUMUNLOCKSTATUS_MAX) )
			{
				val = 0;
			}
			CompendiumMonsters_t::unlocks[itr->name.GetString()] = static_cast<CompendiumUnlockStatus>(val);
		}
	}
}

void Compendium_t::writeUnlocksSaveData()
{
	char path[PATH_MAX] = "";
	completePath(path, "savegames/compendium_progress.json", outputdir);

	rapidjson::Document exportDocument;
	exportDocument.SetObject();

	const int VERSION = 1;

	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumItems_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumItems_t::itemUnlocks )
	{
		obj.AddMember(rapidjson::Value(std::to_string(pair.first).c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items_status", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : AchievementData_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "achievements", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumWorld_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "world", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumCodex_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "codex", obj);

	obj.RemoveAllMembers();
	for ( auto& pair : CompendiumMonsters_t::unlocks )
	{
		obj.AddMember(rapidjson::Value(pair.first.c_str(), exportDocument.GetAllocator()),
			rapidjson::Value((int)pair.second), exportDocument.GetAllocator());
	}
	CustomHelpers::addMemberToRoot(exportDocument, "monsters", obj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::Writer<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

void Compendium_t::Events_t::writeItemsSaveData()
{
	char path[PATH_MAX] = "";
	if ( *cvar_compendiumClientSave && multiplayer == CLIENT )
	{
		completePath(path, "savegames/compendium_items_mp.json", outputdir);
	}
	else
	{
		completePath(path, "savegames/compendium_items.json", outputdir);
	}

	rapidjson::Document exportDocument;
	exportDocument.SetObject();

	const int VERSION = 1;

	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(VERSION));
	rapidjson::Value itemsObj(rapidjson::kObjectType);
	for ( auto& pair : playerEvents )
	{
		const std::string& key = events[pair.first].name;
		rapidjson::Value namekey(key.c_str(), exportDocument.GetAllocator());
		itemsObj.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), exportDocument.GetAllocator());
		auto& obj = itemsObj[key.c_str()];
		for ( auto& itemsData : pair.second )
		{
			rapidjson::Value itemKey(std::to_string(itemsData.first).c_str(), exportDocument.GetAllocator());
			obj.AddMember(itemKey, itemsData.second.value, exportDocument.GetAllocator());
		}
	}
	CustomHelpers::addMemberToRoot(exportDocument, "items", itemsObj);

	File* fp = FileIO::open(path, "wb");
	if ( !fp )
	{
		printlog("[JSON]: Error opening json file %s for write!", path);
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::Writer<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	printlog("[JSON]: Successfully wrote json file %s", path);
	return;
}

bool Compendium_t::Events_t::EventVal_t::applyValue(const Sint32 val)
{
	bool first = firstValue;
	firstValue = false;
	if ( type == SUM )
	{
		value += val;
		if ( id != CPDM_SINKS_HEALTH_RESTORED )
		{
			if ( (Uint32)value >= 0x7FFFFFFF )
			{
				value = 0x7FFFFFFF;
			}
		}
		return true;
	}
	else if ( type == MAX )
	{
		if ( first )
		{
			value = val;
			return true;
		}
		if ( value == val )
		{
			return false;
		}
		value = std::max(val, value);
		return true;
	}
	else if ( type == MIN )
	{
		if ( first )
		{
			value = val;
			return true;
		}
		if ( value == val )
		{
			return false;
		}
		if ( value == 0 )
		{
			value = val;
			return true;
		}
		value = std::min(val, value);
		return true;
	}
	else if ( type == BITFIELD )
	{
		value |= val;
		return true;
	}
	else
	{
		return false;
	}
}

void onCompendiumLevelExit(const int playernum, const char* level, const bool enteringLvl, const bool died)
{
	if ( !level ) { return; }
	if ( !strcmp(level, "") ) { return; }
	if ( enteringLvl )
	{
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_ENTERED, level, 1);
	}
	else
	{
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_EXITED, level, 1);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_GOLD, level, stats[playernum]->GOLD);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_LVL, level, stats[playernum]->LVL);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MIN_LVL, level, stats[playernum]->LVL);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MIN_COMPLETION, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_MAX_COMPLETION, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);

		if ( stats[playernum]->HP <= 0 || died )
		{
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, level, 1);
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, level, players[playernum]->compendiumProgress.playerAliveTimeTotal);
		Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TOTAL_TIME_SPENT, "minehead",
			players[playernum]->compendiumProgress.playerGameTimeTotal);
	}
}

void Compendium_t::Events_t::updateEventsInMainLoop(const int playernum)
{
	if ( !(players[playernum] && players[playernum]->entity && stats[playernum]) )
	{
		return;
	}

	if ( ticks % TICKS_PER_SECOND == 25 )
	{
		auto entity = players[playernum]->entity;
		auto myStats = stats[playernum];
		{
			real_t resistance = 100.0 * Entity::getDamageTableMultiplier(entity, *myStats, DAMAGE_TABLE_MAGIC);
			resistance /= (Entity::getMagicResistance(myStats) + 1);
			resistance = -(resistance - 100.0);
			eventUpdateCodex(playernum, CPDM_RES_MAX, "res", (int)resistance);
			eventUpdateCodex(playernum, CPDM_CLASS_RES_MAX, "res", (int)resistance);
		}

		{
			{
				Sint32 value = statGetSTR(myStats, entity);
				value -= myStats->STR;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "str", value);
				}
			}
			{
				Sint32 value = statGetDEX(myStats, entity);
				value -= myStats->DEX;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "dex", value);
				}
			}
			{
				Sint32 value = statGetCON(myStats, entity);
				value -= myStats->CON;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "con", value);
				}
			}
			{
				Sint32 value = statGetINT(myStats, entity);
				value -= myStats->INT;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "int", value);
				}
			}
			{
				Sint32 value = statGetPER(myStats, entity);
				value -= myStats->PER;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "per", value);
				}
			}
			{
				Sint32 value = statGetCHR(myStats, entity);
				value -= myStats->CHR;
				if ( value > 0 )
				{
					Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_STAT_MAX, "chr", value);
				}
			}
		}

		{
			bool oldDefending = myStats->defending;
			myStats->defending = false;

			Sint32 ac = AC(myStats);

			Sint32 con = myStats->CON;
			myStats->CON = 0;
			int numBlessings = 0;
			Sint32 acFromArmor = AC(myStats);

			real_t targetACEffectiveness = Entity::getACEffectiveness(entity, myStats, true, nullptr, nullptr, numBlessings);
			int effectiveness = targetACEffectiveness * 100.0;

			myStats->CON = con;
			myStats->defending = oldDefending;

			eventUpdateCodex(playernum, CPDM_CLASS_AC_MAX, "ac", ac);
			eventUpdateCodex(playernum, CPDM_AC_MAX_FROM_BLESS, "ac", numBlessings);
			eventUpdateCodex(playernum, CPDM_AC_EFFECTIVENESS_MAX, "ac", effectiveness);
			eventUpdateCodex(playernum, CPDM_AC_EQUIPMENT_MAX, "ac", acFromArmor);
		}

		{
			eventUpdateCodex(playernum, CPDM_HP_MAX, "hp", myStats->MAXHP);
			eventUpdateCodex(playernum, CPDM_CLASS_HP_MAX, "hp", myStats->MAXHP);

			eventUpdateCodex(playernum, CPDM_MP_MAX, "mp", myStats->MAXMP);
			eventUpdateCodex(playernum, CPDM_CLASS_MP_MAX, "mp", myStats->MAXMP);
		}

		{
			{
				// base PWR INT Bonus
				real_t bonus = getSpellBonusFromCasterINT(entity, myStats) * 100.0;
				real_t val = bonus;
				eventUpdateCodex(playernum, CPDM_CLASS_PWR_MAX, "pwr", (int)val);
			}

			// equip/effect bonus (minus INT)
			{
				real_t val = (getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_NONE) * 100.0);
				// look for damage/healing spell bonus for mitre/magus hat
				val = std::max(val, getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_FIREBALL) * 100.0);
				val = std::max(val, getBonusFromCasterOfSpellElement(entity, myStats, nullptr, SPELL_HEALING) * 100.0);
				real_t bonus = getSpellBonusFromCasterINT(entity, myStats);
				val -= bonus * 100.0;
				eventUpdateCodex(playernum, CPDM_PWR_MAX_EQUIP, "pwr", (int)val);
			}
		}
	}

	if ( ticks % (5 * TICKS_PER_SECOND) == 25 )
	{
		int weight = 0;
		int numDeathBoxes = 0;
		for ( node_t* node = stats[playernum]->inventory.first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( !item )
			{
				continue;
			}
			if ( itemCategory(item) == SPELL_CAT )
			{
				continue;
			}
			if ( item->type == TOOL_PLAYER_LOOT_BAG )
			{
				++numDeathBoxes;
			}
			if ( ::items[item->type].item_slot != NO_EQUIP
				&& itemIsEquipped(item, playernum) )
			{
				weight += item->getWeight();
			}
		}
		Compendium_t::Events_t::eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_WGT_EQUIPPED_MAX, "wgt", weight);
		if ( numDeathBoxes > 0 )
		{
			eventUpdate(playernum, CPDM_DEATHBOX_MOST_CARRIED, TOOL_PLAYER_LOOT_BAG, numDeathBoxes);
		}
	}
}

const char* Compendium_t::compendiumCurrentLevelToWorldString(const int currentlevel, const bool secretlevel)
{
	if ( !secretlevel )
	{
		if ( currentlevel == 0 )
		{
			return "minehead";
		}
		else if ( currentlevel == 5 || currentlevel == 10 || currentlevel == 15
			|| currentlevel == 30 )
		{
			return "transition floor";
		}
		else if ( currentlevel >= 1 && currentlevel <= 4 )
		{
			return "mines";
		}
		else if ( currentlevel >= 6 && currentlevel <= 9 )
		{
			return "swamps";
		}
		else if ( currentlevel >= 11 && currentlevel <= 14 )
		{
			return "labyrinth";
		}
		else if ( currentlevel >= 16 && currentlevel <= 19 )
		{
			return "ruins";
		}
		else if ( currentlevel == 20 )
		{
			return "herx lair";
		}
		else if ( currentlevel >= 21 && currentlevel <= 23 )
		{
			return "hell";
		}
		else if ( currentlevel == 24 )
		{
			return "molten throne";
		}
		else if ( currentlevel == 25 )
		{
			return "hamlet";
		}
		else if ( currentlevel >= 26 && currentlevel <= 29 )
		{
			return "crystal caves";
		}
		else if ( currentlevel >= 31 && currentlevel <= 34 )
		{
			return "arcane citadel";
		}
		else if ( currentlevel == 35 )
		{
			return "citadel sanctum";
		}
	}
	else
	{
		if ( currentlevel == 3 )
		{
			return "gnomish mines";
		}
		else if ( currentlevel == 4 )
		{
			return "minetown";
		}
		else if ( currentlevel == 8 )
		{
			return "temple";
		}
		else if ( currentlevel == 9 )
		{
			return "haunted castle";
		}
		else if ( currentlevel == 12 )
		{
			return "sokoban";
		}
		else if ( currentlevel == 14 )
		{
			return "minotaur maze";
		}
		else if ( currentlevel == 17 )
		{
			return "mystic library";
		}
		else if ( currentlevel == 19 || currentlevel == 20 )
		{
			return "underworld";
		}
		else if ( currentlevel == 6 || currentlevel == 7 )
		{
			return "underworld";
		}
		else if ( currentlevel == 29 )
		{
			return "cockatrice lair";
		}
		else if ( currentlevel == 34 )
		{
			return "brams castle";
		}
	}
	return "";
}

void Compendium_t::Events_t::onEndgameEvent(const int playernum, const bool tutorialend, const bool saveHighscore, const bool died)
{
	if ( players[playernum]->isLocalPlayer() )
	{
		if ( tutorialend )
		{
			if ( stats[playernum]->HP <= 0 )
			{
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, "hall of trials", 1);
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
			}
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		else
		{
			players[playernum]->compendiumProgress.updateFloorEvents();
			if ( victory )
			{
				if ( currentlevel == 35 )
				{
					onCompendiumLevelExit(playernum, "citadel sanctum", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON, "races", 1);
				}
				else if ( currentlevel == 24 )
				{
					onCompendiumLevelExit(playernum, "molten throne", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON_HELL, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_HELL_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_HELL_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON_HELL, "races", 1);
				}
				else if ( currentlevel == 20 )
				{
					onCompendiumLevelExit(playernum, "herx lair", false, died);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_GAMES_WON_CLASSIC, "class", 1);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_CLASSIC_MAX, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_CLASS_LVL_WON_CLASSIC_MIN, "leveling up", stats[playernum]->LVL);
					eventUpdateCodex(playernum, Compendium_t::CPDM_RACE_GAMES_WON_CLASSIC, "races", 1);
				}
			}
			else
			{
				const char* currentWorldString = compendiumCurrentLevelToWorldString(currentlevel, secretlevel);
				if ( strcmp(currentWorldString, "") )
				{
					if ( stats[playernum]->HP <= 0 || died )
					{
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS, currentWorldString, 1);
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
						Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
					}
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, currentWorldString, players[playernum]->compendiumProgress.playerAliveTimeTotal);
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TOTAL_TIME_SPENT, "minehead",
						players[playernum]->compendiumProgress.playerGameTimeTotal);
				}
			}

			if ( multiplayer == SERVER && playernum == 0 )
			{
				for ( int i = 1; i < MAXPLAYERS; ++i )
				{
					sendClientDataOverNet(i);
				}
			}
		}
	}
}

void Player::CompendiumProgress_t::updateFloorEvents()
{
	for ( auto& p1 : floorEvents )
	{
		if ( p1.first >= 0 && p1.first < Compendium_t::EventTags::CPDM_EVENT_TAGS_MAX )
		{
			Compendium_t::EventTags tag = (Compendium_t::EventTags)p1.first;
			for ( auto& p2 : p1.second )
			{
				const char* category = p2.first.c_str();
				for ( auto& p3 : p2.second )
				{
					int eventID = p3.first;
					Sint32 value = p3.second;
					if ( eventID >= Compendium_t::Events_t::kEventCodexOffset && eventID <= Compendium_t::Events_t::kEventCodexOffsetMax )
					{
						Compendium_t::Events_t::eventUpdateCodex(player.playernum, tag, category, value, false);
					}
				}
			}
		}
	}

	floorEvents.clear();
}

void Compendium_t::Events_t::onLevelChangeEvent(const int playernum, const int prevlevel, const bool prevsecretfloor, const std::string prevmapname, const bool died)
{
	if ( intro ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS )
	{
		return;
	}

	if ( players[playernum]->isLocalPlayer() )
	{
		players[playernum]->compendiumProgress.updateFloorEvents();
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			const std::string mapname = map.name;
			if ( mapname.find("Tutorial Hub") == std::string::npos
				&& mapname.find("Tutorial ") != std::string::npos )
			{
				Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TRIALS_ATTEMPTS, "hall of trials", 1);
			}
			if ( prevmapname.find("Tutorial Hub") == std::string::npos
				&& prevmapname.find("Tutorial ") != std::string::npos )
			{
				if ( mapname.find("Tutorial Hub") != std::string::npos )
				{
					// returning to hub from a trial, success
					Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_TRIALS_PASSED, "hall of trials", 1);
				}
			}
			Compendium_t::Events_t::eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_TIME_SPENT, "hall of trials", players[playernum]->compendiumProgress.playerAliveTimeTotal);
		}
		else
		{
			const char* currentWorldString = compendiumCurrentLevelToWorldString(currentlevel, secretlevel);
			if ( strcmp(currentWorldString, "") )
			{
				onCompendiumLevelExit(playernum, currentWorldString, true, died);
			}

			if ( stats[playernum] )
			{
				int numDeathBoxes = 0;
				for ( node_t* node = stats[playernum]->inventory.first; node; node = node->next )
				{
					Item* item = (Item*)node->element;
					if ( !item )
					{
						continue;
					}
					if ( item->type == TOOL_PLAYER_LOOT_BAG )
					{
						++numDeathBoxes;
					}
				}
				if ( numDeathBoxes > 0 )
				{
					eventUpdate(playernum, CPDM_DEATHBOX_TO_EXIT, TOOL_PLAYER_LOOT_BAG, numDeathBoxes);
				}
			}

			const char* prevWorldString = compendiumCurrentLevelToWorldString(prevlevel, prevsecretfloor);
			if ( !prevsecretfloor )
			{
				if ( prevlevel == 0 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel == 5 || prevlevel == 10 || prevlevel == 15
					|| prevlevel == 30 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 1 && prevlevel <= 4 )
				{
					onCompendiumLevelExit(playernum, "mines", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 4 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "mines", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "mines", 
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "mines",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 6 && prevlevel <= 9 )
				{
					onCompendiumLevelExit(playernum, "swamps", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 9 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "swamps", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "swamps",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "swamps",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 11 && prevlevel <= 14 )
				{
					onCompendiumLevelExit(playernum, "labyrinth", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 14 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "labyrinth", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "labyrinth",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "labyrinth",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 16 && prevlevel <= 19 )
				{
					onCompendiumLevelExit(playernum, "ruins", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 19 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "ruins", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "ruins",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "ruins",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel == 20 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 21 && prevlevel <= 23 )
				{
					onCompendiumLevelExit(playernum, "hell", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 23 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "hell", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "hell",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "hell",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel == 24 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel == 25 )
				{
					onCompendiumLevelExit(playernum, prevWorldString, false, died);
				}
				else if ( prevlevel >= 26 && prevlevel <= 29 )
				{
					onCompendiumLevelExit(playernum, "crystal caves", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 29 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "crystal caves", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "crystal caves",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "crystal caves",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
				else if ( prevlevel >= 31 && prevlevel <= 34 )
				{
					onCompendiumLevelExit(playernum, "arcane citadel", false, died);
					bool commitUniqueValue = false;
					if ( prevlevel == 34 )
					{
						eventUpdateWorld(playernum, Compendium_t::CPDM_LEVELS_BIOME_CLEAR, "arcane citadel", 1);
#ifdef USE_PLAYFAB
						playfabUser.biomeLeave();
#endif
						commitUniqueValue = true; // commit at end of biome to save to file
					}
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MIN_COMPLETION, "arcane citadel",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
					eventUpdateWorld(playernum, Compendium_t::CPDM_BIOMES_MAX_COMPLETION, "arcane citadel",
						players[playernum]->compendiumProgress.playerAliveTimeTotal, false, -1, commitUniqueValue);
				}
			}
			else
			{
				onCompendiumLevelExit(playernum, prevWorldString, false, died);
			}
		}
	}

	if ( !players[playernum]->isLocalPlayer() ) { return; }

	if ( multiplayer == SERVER && playernum == 0 )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			sendClientDataOverNet(i);
		}
	}
}

bool allowedCompendiumProgress()
{
	if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_CUSTOM_RUN && gameModeManager.currentSession.challengeRun.isActive()
		&& gameModeManager.currentSession.challengeRun.lid.find("challenge") != std::string::npos )
	{
		return false; // challenge event run
	}
	if ( Mods::disableSteamAchievements || (svFlags & SV_FLAG_CHEATS) )
	{
#ifndef DEBUG_ACHIEVEMENTS
		return false;
#endif
	}
	return true;
}

static ConsoleVariable<bool> cvar_compendiumDebugSave("/compendium_debug_save", false);

void Compendium_t::Events_t::eventUpdate(int playernum, const EventTags tag, const ItemType type, 
	Sint32 value, const bool loadingValue, const int spellID)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
		else if ( def.clienttype == CLIENT_AND_SERVER )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
			}
		}
	}

	int itemType = type;
	if ( type == SPELL_ITEM && spellID >= 0 )
	{
		itemType = kEventSpellOffset + spellID;
	}

	auto find2 = eventItemLookup[tag].find(itemType);
	if ( find2 == eventItemLookup[tag].end() )
	{
		return;
	}

	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(itemType);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][itemType] += value;
	}
	
	if ( loadingValue )
	{
		if ( e.find(itemType) == e.end() )
		{
			e[itemType] = EventVal_t(tag);
		}
		auto& val = e[itemType];
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			// don't update in tutorial
		}
		else
		{
			if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
			{
				if ( clientReceiveUpdateFromServer )
				{
					// server is tracking the total for us, so don't add
					players[playernum]->compendiumProgress.itemEvents[def.name][itemType] = value;
				}
				else
				{
					players[playernum]->compendiumProgress.itemEvents[def.name][itemType] += value;
				}
				value = players[playernum]->compendiumProgress.itemEvents[def.name][itemType];
			}
			if ( e.find(itemType) == e.end() )
			{
				e[itemType] = EventVal_t(tag);
			}
			auto& val = e[itemType];
			if ( val.applyValue(value) )
			{
				if ( *cvar_compendiumDebugSave )
				{
					if ( playernum == clientnum )
					{
						writeItemsSaveData();
					}
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		if ( tag == CPDM_RUNS_COLLECTED )
		{
			{
				Monster monsterUnlock = NOTHING;
				if ( type == TOOL_SENTRYBOT )
				{
					monsterUnlock = SENTRYBOT;
				}
				else if ( type == TOOL_SPELLBOT )
				{
					monsterUnlock = SPELLBOT;
				}
				else if ( type == TOOL_GYROBOT )
				{
					monsterUnlock = GYROBOT;
				}
				else if ( type == TOOL_DUMMYBOT )
				{
					monsterUnlock = DUMMYBOT;
				}
				if ( monsterUnlock != NOTHING )
				{
					int monsterId = Compendium_t::Events_t::kEventMonsterOffset + monsterUnlock;
					auto find = Compendium_t::Events_t::monsterIDToString.find(monsterId);
					if ( find != Compendium_t::Events_t::monsterIDToString.end() )
					{
						auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
						if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
						{
							unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
						}
					}
				}
			}

			bool itemUnlocked = false;
			{
				auto& unlockStatus = Compendium_t::CompendiumItems_t::itemUnlocks[itemType];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					itemUnlocked = true;
				}
			}
			auto find = itemIDToString.find(itemType);
			if ( find != itemIDToString.end() )
			{
				auto& unlockStatus = Compendium_t::CompendiumItems_t::unlocks[find->second];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
				/*else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_VISITED )
				{
					if ( itemUnlocked )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}*/
				else if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::UNLOCKED_VISITED )
				{
					if ( itemUnlocked )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::UNLOCKED_UNVISITED;
					}
				}
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateMonster(int playernum, const EventTags tag, const Entity* entity,
	Sint32 value, const bool loadingValue, const int entryID)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int monsterType = -1;
	std::string monsterStrLookup = "";
	if ( entryID >= 0 )
	{
		monsterType = entryID;
	}
	else if ( entity && entity->behavior == &actDeathGhost )
	{
		if ( monsterUniqueIDLookup.find("ghost") != monsterUniqueIDLookup.end() )
		{
			monsterType = monsterUniqueIDLookup["ghost"];
		}
	}
	else if ( entity && entity->behavior == &actMonster )
	{
		if ( auto stats = entity->getStats() )
		{
			if ( stats->type == GNOME && stats->getAttribute("gnome_type").find("gnome2") != std::string::npos )
			{
				monsterStrLookup = "gnome thief";
			}
			else if ( stats->type == SHOPKEEPER && stats->MISC_FLAGS[STAT_FLAG_MYSTERIOUS_SHOPKEEP] > 0 )
			{
				monsterStrLookup = "mysterious shop";
			}
			else
			{
				monsterStrLookup = stats->getAttribute("special_npc");
			}

			if ( monsterStrLookup != "" && monsterUniqueIDLookup.find(monsterStrLookup) != monsterUniqueIDLookup.end() )
			{
				monsterType = monsterUniqueIDLookup[monsterStrLookup];
			}
			else if ( monsterStrLookup == "" )
			{
				monsterType = stats->type;
			}
		}
	}

	if ( monsterType == -1 )
	{
		return;
	}

	if ( monsterType < kEventMonsterOffset )
	{
		monsterType += kEventMonsterOffset; // convert to offset
	}

	auto find2 = eventMonsterLookup[tag].find(monsterType);
	if ( find2 == eventMonsterLookup[tag].end() )
	{
		return;
	}

	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];
	if ( e.find(monsterType) == e.end() )
	{
		e[monsterType] = EventVal_t(tag);
	}

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(monsterType);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] += value;
	}

	auto& val = e[monsterType];
	if ( loadingValue )
	{
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][monsterType] += value;
			}
			value = players[playernum]->compendiumProgress.itemEvents[def.name][monsterType];
		}

		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		auto find = monsterIDToString.find(monsterType);
		if ( find != monsterIDToString.end() )
		{
			auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
			if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
			{
				unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateWorld(int playernum, const EventTags tag, const char* category, Sint32 value, 
	const bool loadingValue, const int entryID, const bool commitUniqueValue)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }
	if ( multiplayer == SERVER && client_disconnected[playernum] )
	{
		return;
	}

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int worldID = -1;
	if ( entryID >= 0 )
	{
		worldID = entryID;
		bool foundCategory = false;
		for ( auto& cat : eventWorldLookup[tag] )
		{
			auto find = eventWorldIDLookup.find(cat);
			if ( find != eventWorldIDLookup.end() )
			{
				if ( eventWorldIDLookup[cat] == worldID )
				{
					foundCategory = true;
					break;
				}
			}
		}
		if ( !foundCategory )
		{
			return;
		}
	}
	else
	{
		auto find2 = eventWorldLookup[tag].find(category);
		if ( find2 == eventWorldLookup[tag].end() )
		{
			return;
		}
		auto find = eventWorldIDLookup.find(category);
		if ( find != eventWorldIDLookup.end() )
		{
			worldID = find->second;
		}
	}

	if ( worldID == -1 )
	{
		return;
	}

	if ( worldID < kEventWorldOffset )
	{
		worldID += kEventWorldOffset; // convert to offset
	}


	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(worldID);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][worldID] += value;
	}

	if ( loadingValue )
	{
		if ( e.find(worldID) == e.end() )
		{
			e[worldID] = EventVal_t(tag);
		}
		auto& val = e[worldID];
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][worldID] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][worldID] += value;
			}
			if ( commitUniqueValue )
			{
				value = players[playernum]->compendiumProgress.itemEvents[def.name][worldID];
			}
			else
			{
				return;
			}
		}
		if ( e.find(worldID) == e.end() )
		{
			e[worldID] = EventVal_t(tag);
		}
		auto& val = e[worldID];
		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		auto find = worldIDToString.find(worldID);
		if ( find != worldIDToString.end() )
		{
			auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks[find->second];
			if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
			{
				if ( find->second == "merchants guild"
					|| find->second == "magicians guild"
					|| find->second == "hunters guild"
					|| find->second == "the church"
					|| find->second == "masons guild" )
				{
					// dont reveal these, revealed @ hamlet
				}
				else
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
			if ( find->second == "shop" )
			{
				// buying items triggers shopkeep stuff
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + SHOPKEEPER);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "herx lair" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "hamlet" )
			{
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["merchants guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["magicians guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["hunters guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["the church"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				{
					auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["masons guild"];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
			else if ( find->second == "molten throne" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + DEVIL);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}

				auto& unlockStatus = Compendium_t::CompendiumWorld_t::unlocks["brimstone boulder"];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
			else if ( find->second == "citadel sanctum" )
			{
				auto find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH_FIRE);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
				find = monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + LICH_ICE);
				if ( find != monsterIDToString.end() )
				{
					auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
					if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
					{
						unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
					}
				}
			}
		}
	}
}

void Compendium_t::Events_t::eventUpdateCodex(int playernum, const EventTags tag, const char* category, 
	Sint32 value, const bool loadingValue, const int entryID, const bool floorEvent)
{
	if ( !allowedCompendiumProgress() && !loadingValue ) { return; }
	if ( intro && !loadingValue ) { return; }
	if ( playernum < 0 || playernum >= MAXPLAYERS ) { return; }

	if ( multiplayer == SINGLE && playernum != 0 ) { return; }
	if ( multiplayer == SERVER && client_disconnected[playernum] )
	{
		return;
	}

	auto find = events.find(tag);
	if ( find == events.end() )
	{
		return;
	}
	auto& def = find->second;

	bool clientReceiveUpdateFromServer = false;

	if ( !loadingValue )
	{
		if ( gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL
			|| gameModeManager.currentMode == GameModeManager_t::GAME_MODE_TUTORIAL_INIT )
		{
			// don't update in tutorial
			if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN
				|| def.eventTrackingType == EventTrackingType::UNIQUE_PER_FLOOR )
			{
				return; 
			}
			else if ( tag == Compendium_t::CPDM_CLASS_MOVING_TIME
				|| tag == Compendium_t::CPDM_CLASS_SNEAK_TIME )
			{
				return;
			}
			if ( category )
			{
				auto find = CompendiumEntries.codex.find(category);
				if ( find != CompendiumEntries.codex.end() )
				{
					if ( !find->second.enableTutorial )
					{
						return;
					}
				}
			}
		}

		if ( def.clienttype == CLIENT_ONLY )
		{
			if ( multiplayer != SINGLE )
			{
				if ( playernum != clientnum )
				{
					return;
				}
			}
		}
		else if ( def.clienttype == SERVER_ONLY )
		{
			if ( multiplayer == CLIENT )
			{
				if ( playernum == 0 )
				{
					playernum = clientnum; // when a client receives an update from the server
					clientReceiveUpdateFromServer = true;
				}
				else
				{
					return;
				}
			}
		}
	}

	int codexID = -1;
	int baseCodexID = -1;
	if ( entryID >= 0 )
	{
		codexID = entryID;
		bool foundCategory = false;
		if ( def.attributes.find("class") != def.attributes.end()
			|| def.attributes.find("race") != def.attributes.end() )
		{
			auto findClassTag = eventClassIds.find(tag);
			if ( findClassTag != eventClassIds.end() )
			{
				for ( auto& pair : findClassTag->second )
				{
					if ( pair.second == ((codexID < kEventCodexOffset) ? (codexID + kEventCodexOffset) : codexID) )
					{
						foundCategory = true;
						break;
					}
				}
			}
		}
		else
		{
			for ( auto& cat : eventCodexLookup[tag] )
			{
				auto find = eventCodexIDLookup.find(cat);
				if ( find != eventCodexIDLookup.end() )
				{
					if ( (eventCodexIDLookup[cat] + kEventCodexOffset)  == codexID )
					{
						foundCategory = true;
						break;
					}
				}
			}
		}
		if ( !foundCategory )
		{
			return;
		}
	}
	else
	{
		auto find2 = eventCodexLookup[tag].find(category);
		if ( find2 == eventCodexLookup[tag].end() )
		{
			return;
		}
		auto find = eventCodexIDLookup.find(category);
		if ( find != eventCodexIDLookup.end() )
		{
			codexID = find->second;
			baseCodexID = codexID;
			if ( def.attributes.find("class") != def.attributes.end() )
			{
				auto findClassTag = eventClassIds.find(tag);
				if ( findClassTag != eventClassIds.end() )
				{
					// iterate through classes
					int classId = client_classes[playernum];
					if ( def.attributes.find("skills") != def.attributes.end() )
					{
						for ( int i = 0; i < NUMPROFICIENCIES; ++i )
						{
							if ( !strcmp(category, getSkillStringForCompendium(i)) )
							{
								classId = client_classes[playernum] + i * kEventClassesMax;
								break;
							}
						}
					}

					auto findClassId = findClassTag->second.find(classId);
					if ( findClassId != findClassTag->second.end() )
					{
						codexID = findClassId->second;
					}
					else
					{
						codexID = -1;
					}
				}
			}
			else if ( def.attributes.find("race") != def.attributes.end() )
			{
				auto findRaceTag = eventClassIds.find(tag);
				if ( findRaceTag != eventClassIds.end() )
				{
					int race = RACE_HUMAN;
					if ( stats[playernum]->playerRace > 0 && stats[playernum]->stat_appearance == 0 )
					{
						race = stats[playernum]->playerRace;
					}
					auto findRaceId = findRaceTag->second.find(race);
					if ( findRaceId != findRaceTag->second.end() )
					{
						codexID = findRaceId->second;
					}
					else
					{
						codexID = -1;
					}
				}
			}
		}
	}

	if ( codexID == -1 )
	{
		return;
	}

	if ( codexID < kEventCodexOffset )
	{
		codexID += kEventCodexOffset; // convert to offset
	}
	if ( baseCodexID >= 0 )
	{
		if ( baseCodexID < kEventCodexOffset )
		{
			baseCodexID += kEventCodexOffset; // convert to offset
		}
	}


	auto& e = (multiplayer == SERVER && playernum != 0 && !loadingValue) ? serverPlayerEvents[playernum][tag] : playerEvents[tag];
	if ( e.find(codexID) == e.end() )
	{
		e[codexID] = EventVal_t(tag);
	}

	if ( def.eventTrackingType == EventTrackingType::ONCE_PER_RUN && !loadingValue )
	{
		auto find = players[playernum]->compendiumProgress.itemEvents[def.name].find(codexID);
		if ( find != players[playernum]->compendiumProgress.itemEvents[def.name].end() )
		{
			// already present, skip adding
			return;
		}
		players[playernum]->compendiumProgress.itemEvents[def.name][codexID] += value;
	}

	auto& val = e[codexID];
	if ( loadingValue )
	{
		val.value = value; // reading from savefile
		val.firstValue = false;
	}
	else
	{
		if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_RUN )
		{
			if ( clientReceiveUpdateFromServer )
			{
				// server is tracking the total for us, so don't add
				players[playernum]->compendiumProgress.itemEvents[def.name][codexID] = value;
			}
			else
			{
				players[playernum]->compendiumProgress.itemEvents[def.name][codexID] += value;
			}
			value = players[playernum]->compendiumProgress.itemEvents[def.name][codexID];
		}
		else if ( def.eventTrackingType == EventTrackingType::UNIQUE_PER_FLOOR && floorEvent )
		{
			players[playernum]->compendiumProgress.floorEvents[tag][category][codexID] += value;
			return;
		}

		if ( val.applyValue(value) )
		{
			if ( *cvar_compendiumDebugSave )
			{
				if ( playernum == clientnum )
				{
					writeItemsSaveData();
				}
			}
		}
	}

	if ( playernum == clientnum )
	{
		if ( baseCodexID >= 0 )
		{
			auto find = codexIDToString.find(baseCodexID);
			if ( find != codexIDToString.end() )
			{
				auto& unlockStatus = Compendium_t::CompendiumCodex_t::unlocks[find->second];
				if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
				{
					unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
				}
			}
		}
	}
}

Uint8 Compendium_t::Events_t::clientSequence = 0;
int Compendium_t::Events_t::previousCurrentLevel = 0;
bool Compendium_t::Events_t::previousSecretlevel = false;
std::map<int, std::string> Compendium_t::Events_t::clientDataStrings[MAXPLAYERS];
std::map<int, std::map<int, std::string>> Compendium_t::Events_t::clientReceiveData;
void Compendium_t::Events_t::sendClientDataOverNet(const int playernum)
{
	if ( multiplayer == SERVER ) {
		if ( playernum == 0 || client_disconnected[playernum] ) {
			return;
		}

		if ( serverPlayerEvents[playernum].empty() )
		{
			return;
		}

		rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "seq", rapidjson::Value(clientSequence));
		rapidjson::Value data(rapidjson::kObjectType);
		for ( auto& p1 : serverPlayerEvents[playernum] )
		{
			std::string key = std::to_string(p1.first);
			rapidjson::Value namekey(key.c_str(), d.GetAllocator());
			data.AddMember(namekey, rapidjson::Value(rapidjson::kObjectType), d.GetAllocator());
			auto& obj = data[key.c_str()];
			for ( auto& itemsData : p1.second )
			{
				rapidjson::Value itemKey(std::to_string(itemsData.first).c_str(), d.GetAllocator());
				obj.AddMember(itemKey, itemsData.second.value, d.GetAllocator());
			}
		}
		CustomHelpers::addMemberToRoot(d, "item", data);

		rapidjson::StringBuffer os;
		rapidjson::Writer<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		clientDataStrings[playernum][clientSequence] = os.GetString();
		auto& dataStr = clientDataStrings[playernum][clientSequence];

		const size_t len = dataStr.size();
		if ( len == 0 )
		{
			return;
		}

		serverPlayerEvents[playernum].clear();

		// packet header
		memset(net_packet->data, 0, NET_PACKET_SIZE);
		memcpy(net_packet->data, "CMPD", 4);
		Uint8 sequence = 0;
		// encode name
		int chunksize = 256;
		const int numchunks = 1 + (len / chunksize);
		for ( int c = 0; c < len; c += chunksize )
		{
			sequence += 1;

			if ( c + chunksize > len )
			{
				chunksize = len - c;
			}
			net_packet->data[4] = clientSequence;
			net_packet->data[5] = sequence; // chunk index
			net_packet->data[6] = numchunks; // num chunks

			std::string substr = dataStr.substr(c, chunksize);
			stringCopy((char*)net_packet->data + 7, substr.c_str(),
				256 + 1, substr.size());

			net_packet->len = 7 + substr.size();

			net_packet->address.host = net_clients[playernum - 1].host;
			net_packet->address.port = net_clients[playernum - 1].port;
			sendPacketSafe(net_sock, -1, net_packet, playernum - 1);
		}

		++clientSequence;
		if ( clientSequence >= 255 )
		{
			clientSequence = 0;
		}

		//else
		//{
		//	// packet header
		//	memcpy(net_packet->data, "CSCN", 4);
		//	net_packet->data[4] = 0;
		//	net_packet->len = 5;
		//	for ( int i = 1; i < MAXPLAYERS; i++ ) {
		//		if ( client_disconnected[i] ) {
		//			continue;
		//		}
		//		if ( playernum != i )
		//		{
		//			continue;
		//		}
		//		net_packet->address.host = net_clients[i - 1].host;
		//		net_packet->address.port = net_clients[i - 1].port;
		//		sendPacketSafe(net_sock, -1, net_packet, i - 1);
		//	}
		//}
	}
}

void Compendium_t::readModelLimbsFromFile(std::string section)
{
	std::string fullpath = "data/compendium/" + section + "_models/";
	for ( auto f : directoryContents(fullpath.c_str(), false, true) )
	{
		std::string inputPath = fullpath + f;
		std::string path = PHYSFS_getRealDir(inputPath.c_str()) ? PHYSFS_getRealDir(inputPath.c_str()) : "";
		if ( path != "" )
		{
			path += PHYSFS_getDirSeparator();
			inputPath = path + inputPath;
			File* fp = FileIO::open(inputPath.c_str(), "rb");
			if ( !fp )
			{
				printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
				return;
			}
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
			buf[count] = '\0';
			rapidjson::StringStream is(buf);
			FileIO::close(fp);

			rapidjson::Document d;
			d.ParseStream(is);
			if ( !d.IsObject() || !d.HasMember("version") || !d.HasMember("limbs") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
				return;
			}
			int version = d["version"].GetInt();

			std::string filename = f.substr(0, f.find(".json"));
			auto& entry = compendiumObjectLimbs[filename];
			entry.entities.clear();
			entry.baseCamera = CompendiumView_t();

			compendiumObjectMapTiles.erase(filename);
			int w = 0;
			int h = 0;
			int index = 0;
			if ( d.HasMember("map_tiles") )
			{
				auto& m = compendiumObjectMapTiles[filename];
				if ( d["map_tiles"].HasMember("floor") 
					&& d["map_tiles"].HasMember("mid") 
					&& d["map_tiles"].HasMember("top")
					&& d["map_tiles"].HasMember("width") 
					&& d["map_tiles"].HasMember("height") )
				{
					if ( d["map_tiles"]["floor"].IsArray()
						&& d["map_tiles"]["mid"].IsArray()
						&& d["map_tiles"]["top"].IsArray() )
					{
						auto floor = d["map_tiles"]["floor"].GetArray();
						auto mid = d["map_tiles"]["mid"].GetArray();
						auto top = d["map_tiles"]["top"].GetArray();
						w = d["map_tiles"]["width"].GetInt();
						h = d["map_tiles"]["height"].GetInt();
						if ( floor.Size() == mid.Size() &&
							floor.Size() == top.Size() && floor.Size() == (w * h) )
						{
							m.first.width = w;
							m.first.height = h;
							if ( d["map_tiles"].HasMember("ceiling") )
							{
								m.first.ceiling = d["map_tiles"]["ceiling"].GetInt();
							}
							auto& tiles = m.second;
							tiles.resize(w * h * MAPLAYERS);
							for ( int z = 0; z < MAPLAYERS; ++z )
							{
								int x = 0;
								int y = 0;

								auto& arr = (z == 0) ? floor : ((z == 1) ? mid : top);
								for ( auto tile = arr.Begin(); tile != arr.End(); ++tile )
								{
									int index = z + (y * MAPLAYERS) + (x * MAPLAYERS * h);
									tiles[index] = tile->GetInt();

									// fix animated tiles so they always start on the correct index
									constexpr int numTileAtlases = sizeof(AnimatedTile::indices) / sizeof(AnimatedTile::indices[0]);
									if ( animatedtiles[tiles[index]] ) {
										auto find = tileAnimations.find(tiles[index]);
										if ( find == tileAnimations.end() ) {
											// this is not the correct index!
											for ( const auto& pair : tileAnimations ) {
												const auto& animation = pair.second;
												for ( int i = 0; i < numTileAtlases; ++i ) {
													if ( animation.indices[i] == tiles[index] ) {
														tiles[index] = animation.indices[0];
													}
												}
											}
										}
									}

									++x;
									if ( x >= w )
									{
										x = 0;
										++y;
									}
								}

							}
						}
					}
				}
			}
			if ( d.HasMember("camera") )
			{
				entry.baseCamera.inUse = true;
				auto& c = d["camera"];
				if ( c.HasMember("ang_degrees") )
				{
					entry.baseCamera.ang = PI * c["ang_degrees"].GetInt() / 180.0;
				}
				if ( c.HasMember("vang_degrees") )
				{
					entry.baseCamera.vang = PI * c["vang_degrees"].GetInt() / 180.0;
				}
				entry.baseCamera.rotateLimit = false;
				if ( c.HasMember("rotate_limit_degrees_min") )
				{
					entry.baseCamera.rotateLimitMin = PI * c["rotate_limit_degrees_min"].GetInt() / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				if ( c.HasMember("rotate_limit_degrees_max") )
				{
					entry.baseCamera.rotateLimitMax = PI * c["rotate_limit_degrees_max"].GetInt() / 180.0;
					entry.baseCamera.rotateLimit = true;
				}
				if ( c.HasMember("rotate_degrees") )
				{
					entry.baseCamera.rotate = PI * c["rotate_degrees"].GetInt() / 180.0;
				}
				else
				{
					entry.baseCamera.rotate = entry.baseCamera.rotateLimitMax - (entry.baseCamera.rotateLimitMax - entry.baseCamera.rotateLimitMin) / 2;
				}
				if ( c.HasMember("rotate_speed") )
				{
					entry.baseCamera.rotateSpeed = c["rotate_speed"].GetDouble();
				}
				if ( c.HasMember("zoom") )
				{
					entry.baseCamera.zoom = c["zoom"].GetDouble();
				}
				if ( c.HasMember("height") )
				{
					entry.baseCamera.height = c["height"].GetDouble();
				}
				if ( c.HasMember("pan") )
				{
					entry.baseCamera.pan = c["pan"].GetDouble();
				}
			}
			for ( auto itr = d["limbs"].Begin(); itr != d["limbs"].End(); ++itr )
			{
				/*if ( d.HasMember("height_offset") )
				{
					statue.heightOffset = d["height_offset"].GetDouble();
				}*/

				entry.entities.push_back(Entity(-1, 0, nullptr, nullptr));
				auto& limb = entry.entities[entry.entities.size() - 1];
				if ( index > 0 )
				{
					limb.x = entry.entities[0].x - (*itr)["x"].GetDouble();
					limb.y = entry.entities[0].y - (*itr)["y"].GetDouble();
				}
				else
				{
					limb.x = (*itr)["x"].GetDouble();
					limb.y = (*itr)["y"].GetDouble();
					limb.x += 8.0;
					limb.y += 8.0;

					if ( w > 0 && h > 0 )
					{
						// position in center of map
						if ( w % 2 == 1 )
						{
							limb.x += 16.0 * (w / 2);
						}
						else
						{
							limb.x += 16.0 * ((w - 1) / 2);
							limb.x += 8.0;
						}
						if ( h % 2 == 1 )
						{
							limb.y += 16.0 * (h / 2);
						}
						else
						{
							limb.y += 16.0 * ((h - 1) / 2);
							limb.y += 8.0;
						}
					}
				}
				limb.z = (*itr)["z"].GetDouble();
				limb.focalx = (*itr)["focalx"].GetDouble();
				limb.focaly = (*itr)["focaly"].GetDouble();
				limb.focalz = (*itr)["focalz"].GetDouble();
				limb.pitch = (*itr)["pitch"].GetDouble();
				limb.roll = (*itr)["roll"].GetDouble();
				limb.yaw = (*itr)["yaw"].GetDouble();
				if ( (*itr).HasMember("yaw_degrees") )
				{
					limb.yaw += PI * (*itr)["yaw_degrees"].GetInt() / 180.0;
				}
				if ( (*itr).HasMember("scalex") )
				{
					limb.scalex = (*itr)["scalex"].GetDouble();
				}
				if ( (*itr).HasMember("scaley") )
				{
					limb.scaley = (*itr)["scaley"].GetDouble();
				}
				if ( (*itr).HasMember("scalez") )
				{
					limb.scalez = (*itr)["scalez"].GetDouble();
				}
				limb.sprite = (*itr)["sprite"].GetInt();

				++index;
			}

			printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
		}
	}
}

void Compendium_t::exportCurrentMonster(Entity* monster)
{
	if ( !monster )
	{
		return;
	}

	int filenum = 0;
	std::string monsterName = monster->getStats() ? monstertypename[monster->getStats()->type] : "nothing";
	std::string testPath = "/data/compendium/monster_models/" + monsterName + std::to_string(filenum) + ".json";
	while ( PHYSFS_getRealDir(testPath.c_str()) != nullptr && filenum < 1000 )
	{
		++filenum;
		testPath = "/data/compendium/monster_models/" + monsterName + std::to_string(filenum) + ".json";
	}

	std::string exportFileName = monsterName + std::to_string(filenum) + ".json";

	rapidjson::Document exportDocument;
	exportDocument.SetObject();
	CustomHelpers::addMemberToRoot(exportDocument, "version", rapidjson::Value(1));

	rapidjson::Value cameraObject(rapidjson::kObjectType);
	cameraObject.AddMember("ang_degrees", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("vang_degrees", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("zoom", 0.0, exportDocument.GetAllocator());
	cameraObject.AddMember("height", 0.0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_limit_degrees_min", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_limit_degrees_max", 0, exportDocument.GetAllocator());
	cameraObject.AddMember("rotate_speed", 0.0, exportDocument.GetAllocator());
	CustomHelpers::addMemberToRoot(exportDocument, "camera", cameraObject);

	rapidjson::Value limbsObject(rapidjson::kObjectType);

	rapidjson::Value limbsArray(rapidjson::kArrayType);

	std::vector<Entity*> allLimbs;
	allLimbs.push_back(monster);

	for ( auto& bodypart : monster->bodyparts )
	{
		allLimbs.push_back(bodypart);
	}

	int index = 0;
	for ( auto& limb : allLimbs )
	{
		if ( limb->flags[INVISIBLE] )
		{
			continue;
		}
		rapidjson::Value limbsObj(rapidjson::kObjectType);

		if ( index != 0 )
		{
			limbsObj.AddMember("x", rapidjson::Value(monster->x - limb->x), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(monster->y - limb->y), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		else
		{
			limbsObj.AddMember("x", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("y", rapidjson::Value(0), exportDocument.GetAllocator());
			limbsObj.AddMember("z", rapidjson::Value(limb->z), exportDocument.GetAllocator());
		}
		limbsObj.AddMember("pitch", rapidjson::Value(limb->pitch), exportDocument.GetAllocator());
		limbsObj.AddMember("roll", rapidjson::Value(limb->roll), exportDocument.GetAllocator());
		limbsObj.AddMember("yaw", rapidjson::Value(limb->yaw), exportDocument.GetAllocator());
		limbsObj.AddMember("focalx", rapidjson::Value(limb->focalx), exportDocument.GetAllocator());
		limbsObj.AddMember("focaly", rapidjson::Value(limb->focaly), exportDocument.GetAllocator());
		limbsObj.AddMember("focalz", rapidjson::Value(limb->focalz), exportDocument.GetAllocator());
		limbsObj.AddMember("sprite", rapidjson::Value(limb->sprite), exportDocument.GetAllocator());
		limbsObj.AddMember("scalex", rapidjson::Value(limb->scalex), exportDocument.GetAllocator());
		limbsObj.AddMember("scaley", rapidjson::Value(limb->scaley), exportDocument.GetAllocator());
		limbsObj.AddMember("scalez", rapidjson::Value(limb->scalez), exportDocument.GetAllocator());
		limbsArray.PushBack(limbsObj, exportDocument.GetAllocator());

		++index;
	}

	CustomHelpers::addMemberToRoot(exportDocument, "limbs", limbsArray);
	
	std::string outputPath = PHYSFS_getRealDir("/data/compendium/monster_models");
	outputPath.append(PHYSFS_getDirSeparator());
	std::string fileName = "data/compendium/monster_models/" + exportFileName;
	outputPath.append(fileName.c_str());

	File* fp = FileIO::open(outputPath.c_str(), "wb");
	if ( !fp )
	{
		return;
	}
	rapidjson::StringBuffer os;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
	exportDocument.Accept(writer);
	fp->write(os.GetString(), sizeof(char), os.GetSize());
	FileIO::close(fp);

	return;
}

bool Compendium_t::lorePointsFirstLoad = true;
void Compendium_t::updateLorePointCounts()
{
	lorePointsFirstLoad = false;
	lorePointsFromAchievements = 0;
	lorePointsSpent = 0;
	lorePointsAchievementsTotal = 0;
	int completed = 0;
	int total = achievements.size();
	for ( auto& achData : achievements )
	{
		if ( achData.second.unlocked )
		{
			lorePointsFromAchievements += achData.second.lorePoints;
			++completed;
		}
		lorePointsAchievementsTotal += achData.second.lorePoints;
	}
	total = std::max(1, total);
	AchievementData_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumItems_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumItems_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumItems_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.items.find(pair.first);
					if ( find != CompendiumEntries.items.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}

	for ( auto& item : CompendiumEntries.items )
	{
		total += item.second.items_in_category.size();
		for ( auto& entry : item.second.items_in_category )
		{
			int type = entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID;
			auto find = Compendium_t::CompendiumItems_t::itemUnlocks.find(type);
			if ( find != Compendium_t::CompendiumItems_t::itemUnlocks.end() )
			{
				if ( find->second != LOCKED_UNKNOWN )
				{
					completed++;
				}
			}
		}
	}
	total = std::max(1, total);
	CompendiumItems_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumMagic_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumItems_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumItems_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.magic.find(pair.first);
					if ( find != CompendiumEntries.magic.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);

	for ( auto& item : CompendiumEntries.magic )
	{
		total += item.second.items_in_category.size();
		for ( auto& entry : item.second.items_in_category )
		{
			int type = entry.itemID == SPELL_ITEM
				? entry.spellID + Compendium_t::Events_t::kEventSpellOffset :
				entry.itemID;
			auto find = Compendium_t::CompendiumItems_t::itemUnlocks.find(type);
			if ( find != Compendium_t::CompendiumItems_t::itemUnlocks.end() )
			{
				if ( find->second != LOCKED_UNKNOWN )
				{
					completed++;
				}
			}
		}
	}
	total = std::max(1, total);
	CompendiumMagic_t::completionPercent = 100.0 * (completed / (real_t)total);

	completed = 0;
	total = 0;
	int seenEntries = 0;
	for ( auto& pair : CompendiumWorld_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumWorld_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumWorld_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.worldObjects.find(pair.first);
					if ( find != CompendiumEntries.worldObjects.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
				++seenEntries;
			}
		}
	}
	total = std::max(1, total);
	CompendiumWorld_t::completionPercent = 100.0 * (completed / (real_t)total);
	if ( seenEntries >= (total / 2) )
	{
		steamAchievement("BARONY_ACH_ISEENTIT");
	}

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumCodex_t::contents["default"] )
	{
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumCodex_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumCodex_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.codex.find(pair.first);
					if ( find != CompendiumEntries.codex.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);
	CompendiumCodex_t::completionPercent = std::min(100.0, 100.0 * (completed / (real_t)total));

	completed = 0;
	total = 0;
	for ( auto& pair : CompendiumMonsters_t::contents_unfiltered["default"] )
	{
		if ( pair.first == "crab" || pair.first == "bubbles" )
		{
			continue;
		}
		if ( pair.first != "-" )
		{
			total += 2;
		}
		auto unlockStatus = CompendiumMonsters_t::unlocks.find(pair.first);
		if ( unlockStatus != CompendiumMonsters_t::unlocks.end() )
		{
			if ( unlockStatus->second != LOCKED_UNKNOWN )
			{
				if ( unlockStatus->second == UNLOCKED_UNVISITED || unlockStatus->second == UNLOCKED_VISITED )
				{
					auto find = CompendiumEntries.monsters.find(pair.first);
					if ( find != CompendiumEntries.monsters.end() )
					{
						lorePointsSpent += find->second.lorePoints;
						++completed; // 2x completion
					}
				}
				++completed;
			}
		}
	}
	total = std::max(1, total);
	CompendiumMonsters_t::completionPercent = 100.0 * (completed / (real_t)total);

	Compendium_t::PointsAnim_t::countUnreadLastTicks = 0;
	Compendium_t::PointsAnim_t::countUnreadNotifs();
}

real_t Compendium_t::PointsAnim_t::anim = 0.0;
real_t Compendium_t::PointsAnim_t::animNoFunds = 0.0;
Uint32 Compendium_t::PointsAnim_t::noFundsTick = 0;
Uint32 Compendium_t::PointsAnim_t::startTicks = 0;
Sint32 Compendium_t::PointsAnim_t::pointsCurrent = 0;
Sint32 Compendium_t::PointsAnim_t::pointsChange = 0;
Sint32 Compendium_t::PointsAnim_t::txtCurrentPoints = 0;
Sint32 Compendium_t::PointsAnim_t::txtChangePoints = 0;
bool Compendium_t::PointsAnim_t::showChanged = false;
bool Compendium_t::PointsAnim_t::noFundsAnimate = false;
bool Compendium_t::PointsAnim_t::firstLoad = true;
bool Compendium_t::PointsAnim_t::mainMenuAlert = false;
Uint32 Compendium_t::PointsAnim_t::countUnreadLastTicks = 0;

void Compendium_t::PointsAnim_t::countUnreadNotifs()
{
	if ( countUnreadLastTicks == 0 || (ticks - countUnreadLastTicks) > TICKS_PER_SECOND )
	{
		Compendium_t::CompendiumMonsters_t::numUnread = 0;
		Compendium_t::CompendiumCodex_t::numUnread = 0;
		Compendium_t::CompendiumItems_t::numUnread = 0;
		Compendium_t::CompendiumMagic_t::numUnread = 0;
		Compendium_t::CompendiumWorld_t::numUnread = 0;
		Compendium_t::AchievementData_t::numUnread = 0;

		countUnreadLastTicks = ticks;

		int numUnread = 0;
		for ( auto& unlockStatus : CompendiumCodex_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumCodex_t::contentsMap.find(unlockStatus.first) != CompendiumCodex_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumCodex_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumWorld_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumWorld_t::contentsMap.find(unlockStatus.first) != CompendiumWorld_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumWorld_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumItems_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumItems_t::contentsMap.find(unlockStatus.first) != CompendiumItems_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumItems_t::numUnread;
				}
				if ( CompendiumMagic_t::contentsMap.find(unlockStatus.first) != CompendiumMagic_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumMagic_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : CompendiumMonsters_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( CompendiumMonsters_t::contentsMap.find(unlockStatus.first) != CompendiumMonsters_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::CompendiumMonsters_t::numUnread;
				}
			}
		}
		for ( auto& unlockStatus : Compendium_t::AchievementData_t::unlocks )
		{
			if ( unlockStatus.second == Compendium_t::UNLOCKED_UNVISITED
				|| unlockStatus.second == Compendium_t::LOCKED_REVEALED_UNVISITED )
			{
				if ( Compendium_t::AchievementData_t::contentsMap.find(unlockStatus.first) != Compendium_t::AchievementData_t::contentsMap.end() )
				{
					++numUnread;
					++Compendium_t::AchievementData_t::numUnread;
				}
			}
		}

		mainMenuAlert = numUnread > 0;
	}
}

void Compendium_t::PointsAnim_t::tickAnimate()
{
	const auto balance = Compendium_t::lorePointsFromAchievements - Compendium_t::lorePointsSpent;

	noFundsAnimate = false;
	{
		// constant decay for animation
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animNoFunds -= setpointDiffX;
		animNoFunds = std::max(0.0, animNoFunds);

		if ( animNoFunds > 0.001 || (ticks - noFundsTick) < TICKS_PER_SECOND * .8 )
		{
			noFundsAnimate = true;
		}
	}

	bool pauseChangeAnim = false;
	if ( pointsChange != 0 )
	{
		Uint32 duration = pointsChange > 0 ? (3 * TICKS_PER_SECOND) : (TICKS_PER_SECOND / 2);
		if ( ((ticks - startTicks) > duration) )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (anim)) / 10.0;
			anim -= setpointDiffX;
			anim = std::max(0.0, anim);

			if ( anim <= 0.0001 )
			{
				pointsChange = 0;
			}
		}
		else
		{
			pauseChangeAnim = true;

			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - anim)) / 10.0;
			anim += setpointDiffX;
			anim = std::min(1.0, anim);
			anim = 1.0;
		}
	}

	showChanged = false;
	if ( pointsChange != 0 )
	{
		Sint32 displayedChange = anim * pointsChange;
		if ( pauseChangeAnim )
		{
			displayedChange = pointsChange;
		}
		if ( abs(displayedChange) > 0 )
		{
			showChanged = true;
			//changeGoldText->setDisabled(false);
			std::string s = "+";
			if ( pointsChange < 0 )
			{
				s = "";
			}
			s += std::to_string(displayedChange);
			txtChangePoints = displayedChange;
			//changeGoldText->setText(s.c_str());
			Sint32 displayedCurrent = pointsCurrent
				+ (pointsChange - displayedChange);
			//currentGoldText->setText(std::to_string(displayedCurrentGold).c_str());
			txtCurrentPoints = displayedCurrent;
		}
	}

	if ( !showChanged )
	{
		txtChangePoints = 0;
		txtCurrentPoints = balance;
		//changeGoldText->setDisabled(true);
		//changeGoldText->setText(std::to_string(displayedChangeGold).c_str());
		//currentGoldText->setText(std::to_string(balance).c_str());
	}
}

void Compendium_t::PointsAnim_t::noFundsEvent()
{
	playSound(90, 64);
	noFundsTick = ticks;
	animNoFunds = 1.0;
}

void Compendium_t::PointsAnim_t::pointsChangeEvent(Sint32 amount)
{
	bool addedToCurrentTotal = false;
	Uint32 duration = pointsChange > 0 ? (3 * TICKS_PER_SECOND) : (TICKS_PER_SECOND / 2);
	const bool isAnimatingValue = ((ticks - startTicks) > duration);
	const auto balance = Compendium_t::lorePointsFromAchievements - Compendium_t::lorePointsSpent;
	if ( amount < 0 )
	{
		if ( pointsChange < 0
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			if ( balance + amount < 0 )
			{
				pointsChange -= balance;
			}
			else
			{
				pointsChange += amount;
			}
		}
		else
		{
			if ( balance + amount < 0 )
			{
				pointsChange = -balance;
			}
			else
			{
				pointsChange = amount;
			}
		}
	}
	else
	{
		if ( pointsChange > 0
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			pointsChange += amount;
		}
		else
		{
			pointsChange = amount;
		}
	}
	startTicks = ticks;
	anim = 0.0;
	if ( !addedToCurrentTotal )
	{
		pointsCurrent = balance;
	}
}

std::vector<Sint32> Compendium_t::CompendiumMonsters_t::Monster_t::getDisplayStat(const char* name)
{
	std::vector<Sint32> retVal;
	if ( !name )
	{
		return retVal;
	}

	bool ignoreHardcore =
		(monsterType == DUMMYBOT
			|| monsterType == GYROBOT
			|| monsterType == SENTRYBOT
			|| monsterType == SPELLBOT
			|| monsterType == NOTHING
			|| monsterType == HUMAN
			);
	bool hardcore = !intro && (svFlags & SV_FLAG_HARDCORE);

	Stat stats(1000 + monsterType);
	if ( !strcmp(name, "hp") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return hp;
		}
		if ( hp.size() > 0 )
		{
			Sint32 statMin = hp[0];
			Sint32 statMax = statMin;
			if ( hp.size() > 1 )
			{
				statMax = hp[1];
			}

			int statIncrease = ((abs(statMin) / 20 + 1) * 20);
			statMin += statIncrease - (statIncrease / 5);
			statIncrease = ((abs(statMax) / 20 + 1) * 20);
			statMax += statIncrease;
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}
	else if ( !strcmp(name, "spd") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return spd;
		}
		if ( spd.size() > 0 )
		{
			Sint32 statMin = spd[0];
			Sint32 statMax = statMin;
			if ( spd.size() > 1 )
			{
				statMax = spd[1];
			}

			int statIncrease = std::min((abs(statMin) / 4 + 1) * 1, 8);
			statMin += statIncrease - (statIncrease / 2);
			statIncrease = std::min((abs(statMax) / 4 + 1) * 1, 8);
			statMax += statIncrease;
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}
	else if ( !strcmp(name, "ac") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return ac;
		}

		Sint32 statMin = stats.CON;
		Sint32 statMax = stats.CON + stats.RANDOM_CON;
		if ( con.size() > 0 )
		{
			statMin = con[0];
			if ( con.size() > 1 )
			{
				statMax = con[1];
			}
			else
			{
				statMax = con[0] + stats.RANDOM_CON;
			}
		}

		int statIncrease = (abs(statMin) / 5 + 1) * 1;
		int minIncrease = statIncrease - (statIncrease / 2);
		statIncrease = (abs(statMax) / 5 + 1) * 1;
		int maxIncrease = statIncrease;

		if ( ac.size() > 0 )
		{
			retVal.push_back(ac[0] + minIncrease);
			if ( ac.size() > 1 )
			{
				if ( ac[1] + maxIncrease != retVal[0] )
				{
					retVal.push_back(ac[1] + maxIncrease);
				}
			}
			else
			{
				if ( ac[0] + maxIncrease != retVal[0] )
				{
					retVal.push_back(ac[0] + maxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "atk") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return atk;
		}

		Sint32 statMin = stats.STR;
		Sint32 statMax = stats.STR + stats.RANDOM_STR;
		if ( str.size() > 0 )
		{
			statMin = str[0];
			if ( str.size() > 1 )
			{
				statMax = str[1];
			}
			else
			{
				statMax = str[0] + stats.RANDOM_STR;
			}
		}

		int statIncrease = (abs(statMin) / 5 + 1) * 5;
		int minIncrease = statIncrease - (statIncrease / 4);
		statIncrease = (abs(statMax) / 5 + 1) * 5;
		int maxIncrease = statIncrease;

		if ( atk.size() > 0 )
		{
			retVal.push_back(atk[0] + minIncrease);
			if ( atk.size() > 1 )
			{
				if ( atk[1] + maxIncrease != retVal[0] )
				{
					retVal.push_back(atk[1] + maxIncrease);
				}
			}
			else
			{
				if ( atk[0] + maxIncrease != retVal[0] )
				{
					retVal.push_back(atk[0] + maxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "rangeatk") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return rangeatk;
		}

		Sint32 statMinIncrease = 0;
		Sint32 statMaxIncrease = 0;

		{
			Sint32 statMinDEX = stats.DEX;
			Sint32 statMaxDEX = stats.DEX + stats.RANDOM_DEX;
			int statIncrease = std::min((abs(statMinDEX) / 4 + 1) * 1, 8);
			int minIncrease = (statIncrease / 2);
			statIncrease = std::min((abs(statMaxDEX) / 4 + 1) * 1, 8);
			int maxIncrease = statIncrease;

			statMinIncrease += minIncrease;
			statMaxIncrease += maxIncrease;
		}
		{
			Sint32 statMinPER = stats.PER;
			Sint32 statMaxPER = stats.PER + stats.RANDOM_PER;
			int statIncrease = (abs(statMinPER) / 5 + 1) * 5;
			int minIncrease = statIncrease - (statIncrease / 4);
			statIncrease = (abs(statMaxPER) / 5 + 1) * 5;
			int maxIncrease = statIncrease;

			statMinIncrease += minIncrease;
			statMaxIncrease += maxIncrease;
		}

		if ( rangeatk.size() > 0 )
		{
			retVal.push_back(rangeatk[0] + statMinIncrease);
			if ( rangeatk.size() > 1 )
			{
				if ( rangeatk[1] + statMaxIncrease != retVal[0] )
				{
					retVal.push_back(rangeatk[1] + statMaxIncrease);
				}
			}
			else
			{
				if ( rangeatk[0] + statMaxIncrease != retVal[0] )
				{
					retVal.push_back(rangeatk[0] + statMaxIncrease);
				}
			}
		}
	}
	else if ( !strcmp(name, "pwr") )
	{
		return pwr;
	}
	else if ( !strcmp(name, "lvl") )
	{
		if ( !hardcore || ignoreHardcore )
		{
			return lvl;
		}

		if ( lvl.size() > 0 )
		{
			Sint32 statMin = lvl[0];
			Sint32 statMax = statMin;
			if ( lvl.size() > 1 )
			{
				statMax = lvl[1] + 1;
			}
			else
			{
				statMax = statMin + 1;
			}
			retVal.push_back(statMin);
			if ( statMax != statMin )
			{
				retVal.push_back(statMax);
			}
		}
	}

	return retVal;
}
#endif

std::unordered_map<std::string, Compendium_t::AchievementData_t> Compendium_t::achievements;
bool Compendium_t::AchievementData_t::achievementsNeedResort = true;
bool Compendium_t::AchievementData_t::achievementsNeedFirstData = true;
int Compendium_t::lorePointsFromAchievements = 0;
int Compendium_t::lorePointsAchievementsTotal = 0;
int Compendium_t::lorePointsSpent = 0;
std::set<std::pair<std::string, std::string>, Compendium_t::AchievementData_t::Comparator> Compendium_t::AchievementData_t::achievementNamesSorted;
std::map<std::string, std::vector<std::pair<std::string, std::string>>> Compendium_t::AchievementData_t::achievementCategories;
std::map<std::string, Compendium_t::AchievementData_t::CompendiumAchievementsDisplay> Compendium_t::AchievementData_t::achievementsBookDisplay;
std::unordered_set<std::string> Compendium_t::AchievementData_t::achievementUnlockedLookup;
bool Compendium_t::AchievementData_t::sortAlphabetical = false;
std::string Compendium_t::compendium_sorting = "default";
bool Compendium_t::compendium_sorting_hide_undiscovered = false;
bool Compendium_t::compendium_sorting_hide_ach_unlocked = false;