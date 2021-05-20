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

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
GameplayCustomManager gameplayCustomManager;
GameModeManager_t gameModeManager;
ItemTooltips_t ItemTooltips;
#ifndef NINTENDO
IRCHandler_t IRCHandler;
#endif // !NINTENDO

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
#else
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

	players[clientnum]->shootmode = false;
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
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
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

	if ( PHYSFS_getRealDir("/data/tutorial_scores.json") )
	{
		std::string inputPath = PHYSFS_getRealDir("/data/tutorial_scores.json");
		inputPath.append("/data/tutorial_scores.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}
		char buf[65536];
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
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

		this->FirstTimePrompt.showFirstTimePrompt = d["first_time_prompt"].GetBool();

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
		printlog("[JSON]: File /data/tutorial_scores.json does not exist, creating...");

		rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "first_time_prompt", rapidjson::Value(true));

		this->FirstTimePrompt.showFirstTimePrompt = true;

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

	if ( !PHYSFS_getRealDir("/data/tutorial_scores.json") )
	{
		printlog("[JSON]: Error file /data/tutorial_scores.json does not exist");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/data/tutorial_scores.json");
	inputPath.append("/data/tutorial_scores.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}
	char buf[65536];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
	buf[count] = '\0';
	rapidjson::StringStream is(buf);
	FileIO::close(fp);

	rapidjson::Document d;
	d.ParseStream(is);

	d["first_time_prompt"].SetBool(this->FirstTimePrompt.showFirstTimePrompt);

	for ( auto it = levels.begin(); it != levels.end(); ++it )
	{
		d["levels"][it->filename.c_str()]["completion_time"].SetUint(it->completionTime);
	}

	writeToFile(d);
}

void GameModeManager_t::Tutorial_t::Menu_t::open()
{
	bWindowOpen = true;
	windowScroll = 0;
	this->selectedMenuItem = -1;

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
	bWindowOpen = true;
	showFirstTimePrompt = false;

	if ( !title_bmp )
	{
		return;
	}

	// create window
	subwindow = 1;
	subx1 = xres / 2 - ((0.75 * title_bmp->w / 2) + 52);
	subx2 = xres / 2 + ((0.75 * title_bmp->w / 2) + 52);
	suby1 = yres / 2 - ((0.75 * title_bmp->h / 2) + 88);
	suby2 = yres / 2 + ((0.75 * title_bmp->h / 2) + 88);
	strcpy(subtext, "");

	Uint32 centerWindowX = subx1 + (subx2 - subx1) / 2;

	button_t* button = newButton();
	strcpy(button->label, language[3965]);
	button->sizex = strlen(language[3965]) * 10 + 8;
	button->sizey = 20;
	button->x = centerWindowX - button->sizex / 2;
	button->y = suby2 - 28 - 24;
	button->action = &buttonPromptEnterTutorialHub;
	button->visible = 1;
	button->focused = 1;

	button = newButton();
	strcpy(button->label, language[3966]);
	button->sizex = strlen(language[3966]) * 12 + 8;
	button->sizey = 20;
	button->x = centerWindowX - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonSkipPrompt;
	button->visible = 1;
	button->focused = 1;
}

void GameModeManager_t::Tutorial_t::FirstTimePrompt_t::drawDialogue()
{
	if ( !bWindowOpen )
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
	
	ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3936]) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 13, SDL_MapRGB(mainsurface->format, 255, 255, 0), language[3936]);
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(language[3967]) * TTF12_WIDTH) / 2, suby2 + 8 - TTF12_HEIGHT * 11, language[3967]);
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(language[3967]) * TTF12_WIDTH) / 2 - TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 11, language[3968]);
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
		int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
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
			messagePlayer(clientnum, "IRC: [@%s]: %s", user.c_str(), formattedMsg.c_str());
		}
		return;
	}
}
#endif // !NINTENDO

void ItemTooltips_t::readItemsFromFile()
{
	if ( !PHYSFS_getRealDir("/items/items.json") )
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

	const int bufSize = 200000;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
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
		t.itemName = item_itr->name.GetString();
		t.itemId = item_itr->value["item_id"].GetInt();
		t.fpIndex = item_itr->value["first_person_model_index"].GetInt();
		t.tpIndex = item_itr->value["third_person_model_index"].GetInt();
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

		tmpItems.push_back(t);
		++itemsRead;
	}

	printlog("[JSON]: Successfully read %d items from '%s'", itemsRead, inputPath.c_str());

	for ( int i = 0; i < NUMITEMS && i < itemsRead; ++i )
	{
		assert(i == tmpItems[i].itemId);
		items[i].level = tmpItems[i].itemLevel;
		items[i].value = tmpItems[i].gold;
		items[i].weight = tmpItems[i].weight;
		items[i].fpindex = tmpItems[i].fpIndex;
		items[i].index = tmpItems[i].tpIndex;
		items[i].tooltip = tmpItems[i].tooltip;
		items[i].attributes.clear();
		items[i].attributes = tmpItems[i].attributes;

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
	}

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

void ItemTooltips_t::readTooltipsFromFile()
{
	if ( !PHYSFS_getRealDir("/items/item_tooltips.json") )
	{
		printlog("[JSON]: Error: Could not find file: items/items.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/items/item_tooltips.json");
	inputPath.append("/items/item_tooltips.json");

	File* fp = FileIO::open(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		return;
	}

	const int bufSize = 65535;
	char buf[bufSize];
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
	buf[count] = '\0';
	//rapidjson::FileReadStream is(fp, buf, sizeof(buf)); - use this for large chunks.
	rapidjson::StringStream is(buf);

	rapidjson::Document d;
	d.ParseStream(is);
	FileIO::close(fp);
	if ( !d.HasMember("version") || !d.HasMember("tooltips") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	adjectives.clear();
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

	if ( d.HasMember("default_text_colors") )
	{
		defaultHeadingTextColor = SDL_MapRGBA(mainsurface->format, 
			d["default_text_colors"]["heading"]["r"].GetInt(), 
			d["default_text_colors"]["heading"]["g"].GetInt(), 
			d["default_text_colors"]["heading"]["b"].GetInt(),
			d["default_text_colors"]["heading"]["a"].GetInt());
		defaultIconTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["icons"]["r"].GetInt(),
			d["default_text_colors"]["icons"]["g"].GetInt(),
			d["default_text_colors"]["icons"]["b"].GetInt(), 
			d["default_text_colors"]["icons"]["a"].GetInt());
		defaultDescriptionTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["description"]["r"].GetInt(),
			d["default_text_colors"]["description"]["g"].GetInt(), 
			d["default_text_colors"]["description"]["b"].GetInt(),
			d["default_text_colors"]["description"]["a"].GetInt());
		defaultDetailsTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["details"]["r"].GetInt(), 
			d["default_text_colors"]["details"]["g"].GetInt(), 
			d["default_text_colors"]["details"]["b"].GetInt(), 
			d["default_text_colors"]["details"]["a"].GetInt());
		defaultPositiveTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["positive_color"]["r"].GetInt(),
			d["default_text_colors"]["positive_color"]["g"].GetInt(),
			d["default_text_colors"]["positive_color"]["b"].GetInt(),
			d["default_text_colors"]["positive_color"]["a"].GetInt());
		defaultNegativeTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["negative_color"]["r"].GetInt(),
			d["default_text_colors"]["negative_color"]["g"].GetInt(),
			d["default_text_colors"]["negative_color"]["b"].GetInt(),
			d["default_text_colors"]["negative_color"]["a"].GetInt());
		defaultStatusEffectTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["status_effect"]["r"].GetInt(),
			d["default_text_colors"]["status_effect"]["g"].GetInt(),
			d["default_text_colors"]["status_effect"]["b"].GetInt(),
			d["default_text_colors"]["status_effect"]["a"].GetInt());
		defaultFaintTextColor = SDL_MapRGBA(mainsurface->format,
			d["default_text_colors"]["faint_text"]["r"].GetInt(),
			d["default_text_colors"]["faint_text"]["g"].GetInt(),
			d["default_text_colors"]["faint_text"]["b"].GetInt(),
			d["default_text_colors"]["faint_text"]["a"].GetInt());
	}

	templates.clear();
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
				for ( auto lines = template_itr->value.Begin();
					lines != template_itr->value.End(); ++lines )
				{
					templates[template_itr->name.GetString()].push_back(lines->GetString());
				}
			}
		}
	}

	tooltips.clear();

	std::unordered_set<std::string> tagsRead;

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
						color = SDL_MapRGBA(mainsurface->format,
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
				tooltip.minWidth = tooltipType_itr->value["size"]["min_width"].GetInt();
			}
		}

		tooltips[tooltipType_itr->name.GetString()] = tooltip;
	}

	printlog("[JSON]: Successfully read %d item tooltips from '%s'", tooltips.size(), inputPath.c_str());
	/*for ( auto tmp : tagsRead )
	{
		printlog("%s", tmp.c_str());
	}*/
}

std::string& ItemTooltips_t::getItemStatusAdjective(Uint32 itemType, Status status)
{
	if ( items[itemType].category == ARMOR 
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

std::string& ItemTooltips_t::getItemPotionAlchemyAdjective(const int player, Uint32 itemType)
{
	if ( adjectives.find("potion_alchemy_types") == adjectives.end() )
	{
		return defaultString;
	}
	if ( clientLearnedAlchemyIngredients.find(itemType) == clientLearnedAlchemyIngredients.end() )
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
}

std::string& ItemTooltips_t::getItemPotionHarmAllyAdjective(Item& item)
{
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

void ItemTooltips_t::formatItemIcon(const int player, std::string tooltipType, Item& item, std::string& str, int iconIndex)
{
	auto itemTooltip = ItemTooltips.tooltips[tooltipType];

	char buf[128];
	memset(buf, 0, sizeof(buf));

	if ( tooltipType.find("tooltip_armor") != std::string::npos )
	{
		Sint32 AC = item.armorGetAC(stats[player]);
		if ( stats[player] )
		{
			//AC += stats[player]->getPassiveShieldBonus(false);
			/*if ( stats[player]->shield == &item )
			{
				if ( stats[player]->defending )
				{
					AC += stats[player]->getActiveShieldBonus(false);
				}
			}*/
		}
		snprintf(buf, sizeof(buf), str.c_str(), AC);
	}
	else if ( tooltipType.compare("tooltip_mace") == 0
		|| tooltipType.compare("tooltip_sword") == 0 
		|| tooltipType.compare("tooltip_polearm") == 0
		|| tooltipType.find("tooltip_thrown") != std::string::npos )
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.compare("tooltip_axe") == 0 )
	{
		Sint32 atk = item.weaponGetAttack(stats[player]);
		atk += 1;
		snprintf(buf, sizeof(buf), str.c_str(), atk);
	}
	else if ( tooltipType.find("tooltip_potion") != std::string::npos )
	{
		if ( items[item.type].hasAttribute("POTION_TYPE_HEALING") )
		{
			if ( item.type == POTION_HEALING || item.type == POTION_EXTRAHEALING || item.type == POTION_RESTOREMAGIC )
			{
				auto oldStatus = item.status;
				item.status = DECREPIT;
				int lowVal = item.potionGetEffectHealth();
				item.status = EXCELLENT;
				int highVal = item.potionGetEffectHealth();
				item.status = oldStatus;
				snprintf(buf, sizeof(buf), str.c_str(), lowVal, highVal);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth());
			}
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_DMG") )
		{
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDamage());
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_GOOD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum() / TICKS_PER_SECOND, item.potionGetEffectDurationMaximum() / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
		else if ( items[item.type].hasAttribute("POTION_TYPE_BAD_EFFECT") )
		{
			auto oldBeatitude = item.beatitude;
			item.beatitude = std::max((Sint16)0, item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum() / TICKS_PER_SECOND, item.potionGetEffectDurationMaximum() / TICKS_PER_SECOND);
			item.beatitude = oldBeatitude;
		}
	}
	else
	{
		return;
	}
	str = buf;
}

void ItemTooltips_t::formatItemDescription(const int player, std::string tooltipType, Item& item, std::string& str)
{
	return;
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

	//memset(buf, 0, sizeof(buf));
	//if ( tooltipType.find("tooltip_armor") != std::string::npos && str.find("%s") != std::string::npos )
	//{
		//snprintf(buf, sizeof(buf), str.c_str(), adjective.c_str());
		//str = buf;
	//}
}

void ItemTooltips_t::formatItemDetails(const int player, std::string tooltipType, Item& item, std::string& str, std::string detailTag)
{
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

	auto itemTooltip = ItemTooltips.tooltips[tooltipType];

	memset(buf, 0, sizeof(buf));

	if ( tooltipType.find("tooltip_armor") != std::string::npos )
	{
		if ( detailTag.compare("armor_base_ac") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(),
				items[item.type].hasAttribute("AC") ? items[item.type].attributes["AC"] : 0	);
		}
		else if ( detailTag.compare("armor_shield_bonus") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				stats[player]->getPassiveShieldBonus(false),
				getItemProficiencyName(PRO_SHIELD).c_str(),
				stats[player]->getActiveShieldBonus(false),
				getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("on_bless_or_curse") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude, 
				getItemBeatitudeAdjective(item.beatitude).c_str());
		}
		else if ( detailTag.compare("shield_durability") == 0 )
		{
			int skillLVL = stats[player]->PROFICIENCIES[PRO_SHIELD] / 10;
			int durabilityBonus = skillLVL * 10;
			snprintf(buf, sizeof(buf), str.c_str(), durabilityBonus, getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("shield_legendary_durability") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getItemProficiencyName(PRO_SHIELD).c_str());
		}
		else if ( detailTag.compare("knuckle_skill_modifier") == 0 )
		{
			int atk = (stats[player]->PROFICIENCIES[PRO_UNARMED] / 20); // 0 - 5
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemProficiencyName(PRO_UNARMED).c_str());
		}
		else if ( detailTag.compare("knuckle_knockback_modifier") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), 
				items[item.type].hasAttribute("KNOCKBACK") ? items[item.type].attributes["KNOCKBACK"] : 0);
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = stats[player]->PROFICIENCIES[PRO_UNARMED] / 20;
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
		else
		{
			return;
		}
	}
	else if ( tooltipType.find("tooltip_thrown") != std::string::npos )
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
			snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0);
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = stats[player]->PROFICIENCIES[proficiency] / 20;
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * thrownDamageSkillMultipliers[std::min(skillLVL, 5)] - 100),
				getItemProficiencyName(proficiency).c_str());
		}
		else
		{
			return;
		}
	}
	else if ( tooltipType.compare("tooltip_mace") == 0 
		|| tooltipType.compare("tooltip_axe") == 0
		|| tooltipType.compare("tooltip_sword") == 0
		|| tooltipType.compare("tooltip_polearm") == 0 )
	{
		int proficiency = PRO_SWORD;
		if ( tooltipType.compare("tooltip_mace") == 0 )
		{
			proficiency = PRO_MACE;
		}
		else if ( tooltipType.compare("tooltip_axe") == 0 )
		{
			proficiency = PRO_AXE;
		}
		else if ( tooltipType.compare("tooltip_sword") == 0 )
		{
			proficiency = PRO_SWORD;
		}
		else if ( tooltipType.compare("tooltip_polearm") == 0 )
		{
			proficiency = PRO_POLEARM;
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
		else if ( detailTag.compare("weapon_skill_modifier") == 0 )
		{
			int weaponEffectiveness = -25 + (stats[player]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
			snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
		}
		else if ( detailTag.compare("weapon_durability") == 0 )
		{
			int skillLVL = stats[player]->PROFICIENCIES[proficiency] / 20;
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
			snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum() / (60 * TICKS_PER_SECOND),
				item.potionGetEffectDurationMaximum() / (60 * TICKS_PER_SECOND) );
		}
		else if ( detailTag.compare("potion_restoremagic_bonus") == 0 )
		{
			if ( stats[player] && statGetINT(stats[player], players[player]->entity) > 0 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), std::min(30, 2 * statGetINT(stats[player], players[player]->entity)));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_healing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 2 * statGetCON(stats[player], players[player]->entity));
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 0);
			}
		}
		else if ( detailTag.compare("potion_extrahealing_bonus") == 0 )
		{
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 )
			{
				snprintf(buf, sizeof(buf), str.c_str(), 4 * statGetCON(stats[player], players[player]->entity));
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
					item.potionGetEffectDurationRandom() / TICKS_PER_SECOND, getItemBeatitudeAdjective(item.beatitude).c_str());
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
			int skillLVL = stats[player]->PROFICIENCIES[PRO_ALCHEMY] / 20;
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * potionDamageSkillMultipliers[std::min(skillLVL, 5)] - 100), 
				getItemPotionHarmAllyAdjective(item).c_str());
		}
	}
	else
	{
		return;
	}
	str = buf;
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
				*it = ' ';
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
					*it = ' ';
					break;
				}
				*it = ' ';
				it = std::next(it);
			}
		}
		else
		{
			bracketText += ' ';
		}
		if ( *it == '\n' )
		{
			bracketText += '\n';
		}
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
				*it = ' ';
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
					*it = ' ';
					for ( size_t i = 0; i < strlen("(?)") - 1; ++i )
					{
						positiveValues += ' ';
						it = std::next(it);
						*it = ' ';
					}
				}
			}
			else if ( *it == '[' )
			{
				// look for matching brace.
				while ( it != str.end() && *it != ']' && *it != ' ' && *it != '\0' && *it != ' ' && *it != '\n' )
				{
					positiveValues += ' ';
					negativeValues += ' ';
					it = std::next(it);
				}
			}
			else if ( *it == '^' )
			{
				// cursed line
				it = str.erase(it); // skip the '^'
				//it = std::next(it); 
				while ( it != str.end() && *it != '\0' && *it != '\n' )
				{
					positiveValues += ' ';
					negativeValues += *it;
					*it = ' ';
					it = std::next(it);
				}
			}
			/*else if ( *it == adjectives["beatitude_status"]["cursed"][0] )
			{
				if ( str.find(adjectives["beatitude_status"]["cursed"], 
					std::distance(str.begin(), it)) == std::distance(str.begin(), it) )
				{
					addSpace = false;
					positiveValues += ' ';
					negativeValues += adjectives["beatitude_status"]["cursed"];
					*it = ' ';
					for ( int i = 0; i < adjectives["beatitude_status"]["cursed"].size() - 1; ++i )
					{
						positiveValues += ' ';
						it = std::next(it);
						*it = ' ';
					}
				}
			}*/

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