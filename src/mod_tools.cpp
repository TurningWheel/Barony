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
#endif

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
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
	stats[0]->appearance = local_rng.rand() % NUMAPPEARANCES;
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
#else
void GameModeManager_t::Tutorial_t::openGameoverWindow()
{
	MainMenu::openGameoverWindow(0, true);
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
	int count = fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
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
	//strcpy(button->label, language[3965]);
	//button->sizex = strlen(language[3965]) * 10 + 8;
	//button->sizey = 20;
	//button->x = centerWindowX - button->sizex / 2;
	//button->y = suby2 - 28 - 24;
	//button->action = &buttonPromptEnterTutorialHub;
	//button->visible = 1;
	//button->focused = 1;

	//button = newButton();
	//strcpy(button->label, language[3966]);
	//button->sizex = strlen(language[3966]) * 12 + 8;
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
	
	ttfPrintTextFormattedColor(ttf12, centerWindowX - strlen(language[3936]) * TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 13, makeColorRGB(255, 255, 0), language[3936]);
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(language[3967]) * TTF12_WIDTH) / 2, suby2 + 8 - TTF12_HEIGHT * 11, language[3967]);
	ttfPrintTextFormatted(ttf12, centerWindowX - (longestline(language[3967]) * TTF12_WIDTH) / 2 - TTF12_WIDTH / 2, suby2 + 8 - TTF12_HEIGHT * 11, language[3968]);*/
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

void ItemTooltips_t::readItemsFromFile()
{
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

	const int bufSize = 200000;
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


void ItemTooltips_t::readItemLocalizationsFromFile()
{
	if ( !PHYSFS_getRealDir("/lang/item_names.json") )
	{
		printlog("[JSON]: Error: Could not find file: lang/item_names.json");
		return;
	}

	std::string inputPath = PHYSFS_getRealDir("/lang/item_names.json");
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
		itemNameLocalizations.clear();
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
		spellNameLocalizations.clear();
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

#ifndef EDITOR
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

int ItemTooltips_t::getSpellDamageOrHealAmount(const int player, spell_t* spell, Item* spellbook)
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
					bonus = getSpellbookBonusPercent(players[player]->entity, stats[player], spellbook);
				}
				damage += (damage * (bonus * 0.01 + getBonusFromCasterOfSpellElement(players[player]->entity, stats[player], primaryElement)));
				heal += (heal * (bonus * 0.01 + getBonusFromCasterOfSpellElement(players[player]->entity, stats[player], primaryElement)));
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
	spell_t* spell = getSpellFromItem(player, &item);
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

std::string ItemTooltips_t::getSpellIconText(const int player, Item& item)
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
		spell = getSpellFromItem(player, &item);
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
		if ( (statGetINT(stats[player], players[player]->entity)
			+ stats[player]->PROFICIENCIES[PRO_MAGIC]) >= SKILL_LEVEL_EXPERT )
		{
			numSummons = 2;
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
		snprintf(buf, sizeof(buf), str.c_str(), getSpellDamageOrHealAmount(player, spell, &item));
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
	spell_t* spell = getSpellFromItem(player, &item);
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
	spell_t* spell = getSpellFromItem(player, &item);
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

std::string ItemTooltips_t::getSpellIconPath(const int player, Item& item)
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
		spell_t* spell = getSpellFromItem(player, &item);
		if ( spell )
		{
			spellImageNode = getSpellNodeFromSpellID(spell->ID);
		}
		else
		{
			spellImageNode = getSpellNodeFromSpellID(SPELL_NONE);
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

std::string& ItemTooltips_t::getItemStatShortName(std::string& attribute)
{
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

std::string& ItemTooltips_t::getItemStatFullName(std::string& attribute)
{
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

void ItemTooltips_t::formatItemIcon(const int player, std::string tooltipType, Item& item, std::string& str, int iconIndex, std::string& conditionalAttribute)
{
#ifndef EDITOR
	auto itemTooltip = tooltips[tooltipType];
	static Stat itemDummyStat(0);
	char buf[128];
	memset(buf, 0, sizeof(buf));

	if ( conditionalAttribute.find("magicstaff_") != std::string::npos )
	{
		if ( str == "" )
		{
			str = getSpellIconText(player, item);
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
			str = getSpellIconText(player, item);
		}
		return;
	}
	else if ( conditionalAttribute.find("SPELLBOOK_") != std::string::npos )
	{
		if ( conditionalAttribute == "SPELLBOOK_SPELLINFO_LEARNED" )
		{
			str = getSpellIconText(player, item);
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
			spellBookBonusPercent += getSpellbookBonusPercent(players[player]->entity, stats[player], &item);
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
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr);
			}
			int bonusFromPER = std::max(0, statGetPER(stats[player], players[player]->entity)) * items[item.type].attributes["BOMB_DMG_PER_MULT"];
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
			int spellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(spellID), nullptr);
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
				getItemStatFullName(conditionalAttribute).c_str());
		}
		else if ( conditionalAttribute == "DEX" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute).c_str());
		}
		else if ( conditionalAttribute == "CON" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute).c_str());
		}
		else if ( conditionalAttribute == "INT" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute).c_str());
		}
		else if ( conditionalAttribute == "PER" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute).c_str());
		}
		else if ( conditionalAttribute == "CHR" )
		{
			snprintf(buf, sizeof(buf), str.c_str(), getStatAttributeBonusFromItem(player, item, conditionalAttribute),
				getItemStatFullName(conditionalAttribute).c_str());
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
			std::string attribute("AC");
			snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName(attribute).c_str());
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
		std::string attribute("AC");
		snprintf(buf, sizeof(buf), str.c_str(), AC, getItemStatFullName(attribute).c_str());
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
					const int statBonus = 2 * std::max(0, statGetCON(stats[player], players[player]->entity));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_EXTRAHEALING )
				{
					const int statBonus = 4 * std::max(0, statGetCON(stats[player], players[player]->entity));
					healthVal += statBonus;
				}
				else if ( item.type == POTION_RESTOREMAGIC )
				{
					const int statBonus = std::min(30, 2 * std::max(0, statGetINT(stats[player], players[player]->entity)));
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
			snprintf(buf, sizeof(buf), str.c_str(), item.getScrollLabel());
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

void ItemTooltips_t::formatItemDetails(const int player, std::string tooltipType, Item& item, std::string& str, std::string detailTag)
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
			if ( tooltipType.find("tooltip_offhand") != std::string::npos )
			{
				snprintf(buf, sizeof(buf), str.c_str(),
					stats[player]->getActiveShieldBonus(false),
					getItemProficiencyName(PRO_SHIELD).c_str());
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), 
					stats[player]->getPassiveShieldBonus(false),
					getItemProficiencyName(PRO_SHIELD).c_str(),
					stats[player]->getActiveShieldBonus(false),
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
			int skillLVL = stats[player]->PROFICIENCIES[PRO_SHIELD] / 10;
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
		else if ( detailTag.compare("ring_unarmed_atk") == 0 )
		{
			int atk = 1 + (shouldInvertEquipmentBeatitude(stats[player]) ? abs(item.beatitude) : item.beatitude);
			snprintf(buf, sizeof(buf), str.c_str(), atk, getItemBeatitudeAdjective(item.beatitude).c_str());
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
			snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? (statGetDEX(stats[player], players[player]->entity) / 4) : 0);
		}
		else if ( detailTag.compare("thrown_skill_modifier") == 0 )
		{
			int skillLVL = stats[player]->PROFICIENCIES[proficiency] / 10;
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
				int weaponEffectiveness = -25 + (stats[player]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
			else
			{
				int weaponEffectiveness = -25 + (stats[player]->PROFICIENCIES[proficiency] / 2); // -25% to +25%
				snprintf(buf, sizeof(buf), str.c_str(), weaponEffectiveness, getItemProficiencyName(proficiency).c_str());
			}
		}
		else if ( detailTag.compare("weapon_atk_from_player_stat") == 0 )
		{
			if ( item.type == TOOL_WHIP )
			{
				int atk = (stats[player] ? statGetDEX(stats[player], players[player]->entity) : 0);
				atk += (stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
				atk = std::min(atk / 2, atk);
				snprintf(buf, sizeof(buf), str.c_str(), atk);
			}
			else if ( proficiency == PRO_RANGED )
			{
				snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? statGetDEX(stats[player], players[player]->entity) : 0);
			}
			else
			{
				snprintf(buf, sizeof(buf), str.c_str(), stats[player] ? statGetSTR(stats[player], players[player]->entity) : 0);
			}
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
		else if ( detailTag.compare("weapon_ranged_armor_pierce") == 0 )
		{
			int statChance = std::min(std::max((stats[player] ? statGetPER(stats[player], players[player]->entity) : 0) / 2, 0), 50); // 0 to 50 value.
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
			if ( stats[player] && statGetINT(stats[player], players[player]->entity) > 0 )
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
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 )
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
			if ( stats[player] && statGetCON(stats[player], players[player]->entity) > 0 )
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
			int skillLVL = stats[player]->PROFICIENCIES[PRO_ALCHEMY] / 20;
			snprintf(buf, sizeof(buf), str.c_str(), static_cast<int>(100 * potionDamageSkillMultipliers[std::min(skillLVL, 5)] - 100), 
				getItemPotionHarmAllyAdjective(item).c_str());
		}
	}
	else if ( tooltipType.compare("tooltip_tool_lockpick") == 0 )
	{
		Sint32 PER = statGetPER(stats[player], players[player]->entity);
		if ( detailTag.compare("lockpick_chestsdoors_unlock_chance") == 0 )
		{
			int chance = stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 2; // lockpick chests/doors
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] == SKILL_LEVEL_LEGENDARY )
			{
				chance = 100;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_chests_scrap_chance") == 0 )
		{
			int chance = std::min(100, stats[player]->PROFICIENCIES[PRO_LOCKPICKING] + 50);
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10)))); // disarm arrow traps
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_BASIC )
			{
				chance = 0;
			}
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("lockpick_automaton_disarm") == 0 )
		{
			int chance = 0;
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] >= SKILL_LEVEL_EXPERT )
			{
				chance = 100; // lockpick automatons
			}
			else
			{
				chance = (100 - 100 / (static_cast<int>(stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 20 + 1))); // lockpick automatons
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
		Sint32 PER = statGetPER(stats[player], players[player]->entity);
		if ( detailTag.compare("lockpick_arrow_disarm") == 0 )
		{
			int chance = (100 - 100 / (std::max(1, static_cast<int>(stats[player]->PROFICIENCIES[PRO_LOCKPICKING] / 10)))); // disarm arrow traps
			if ( stats[player]->PROFICIENCIES[PRO_LOCKPICKING] < SKILL_LEVEL_BASIC )
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

			int intBonus = (statGetINT(stats[player], players[player]->entity) * 0.5);
			real_t mult = ((items[item.type].attributes["SPELLBOOK_CAST_BONUS"]) / 100.0);
			intBonus *= mult;
			int beatitudeBonus = (mult * getSpellbookBonusPercent(players[player]->entity, stats[player], &item)) - intBonus;

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

			std::string attribute("INT");
			snprintf(buf, sizeof(buf), str.c_str(),
				intBonus, damageOrHealing.c_str(), getItemStatShortName(attribute).c_str(),
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

			int skillLVL = std::min(100, stats[player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[player], players[player]->entity));
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

			int skillLVL = std::min(100, stats[player]->PROFICIENCIES[PRO_MAGIC] + statGetINT(stats[player], players[player]->entity));
			if ( !playerLearnedSpellbook(player, &item) && (spell && spell->difficulty > skillLVL) )
			{
				str.insert((size_t)0, 1, '^'); // red line character
			}
			Sint32 INT = stats[player] ? statGetINT(stats[player], players[player]->entity) : 0;
			Sint32 skill = stats[player] ? stats[player]->PROFICIENCIES[PRO_MAGIC] : 0;
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
			spell_t* spell = getSpellFromItem(player, &item);
			if ( !spell ) { return; }

			int totalDamage = getSpellDamageOrHealAmount(player, spell, nullptr);
			Sint32 oldINT = stats[player]->INT;
			stats[player]->INT = 0;

			int baseDamage = getSpellDamageOrHealAmount(player, spell, nullptr);
			stats[player]->INT = oldINT;

			real_t bonusPercent = 100.0 * getBonusFromCasterOfSpellElement(players[player]->entity, stats[player]);

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
			}
			std::string attribute("INT");
			snprintf(buf, sizeof(buf), str.c_str(), damageOrHealing.c_str(), baseDamage, damageOrHealing.c_str(), 
				bonusPercent, damageOrHealing.c_str(), getItemStatShortName(attribute).c_str());
		}
		else if ( detailTag.compare("spell_cast_success") == 0 )
		{
			int spellcastingAbility = std::min(std::max(0, stats[player]->PROFICIENCIES[PRO_SPELLCASTING]
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = ((10 - (spellcastingAbility / 10)) * 20 / 3.0); // 33% after rolling to fizzle, 66% success
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("spell_extramana_chance") == 0 )
		{
			int spellcastingAbility = std::min(std::max(0, stats[player]->PROFICIENCIES[PRO_SPELLCASTING]
				+ statGetINT(stats[player], players[player]->entity)), 100);
			int chance = (10 - (spellcastingAbility / 10)) * 10;
			snprintf(buf, sizeof(buf), str.c_str(), chance);
		}
		else if ( detailTag.compare("attribute_spell_charm") == 0 )
		{
			int leaderChance = ((statGetCHR(stats[player], players[player]->entity) + 
				stats[player]->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 5;
			int intChance = (statGetINT(stats[player], players[player]->entity) * 2);
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
				stats[player]->PROFICIENCIES[PRO_LEADERSHIP]) / 20) * 10;
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
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_COLD), nullptr);
			}
			else if ( item.type == TOOL_BOMB )
			{
				baseSpellDamage = getSpellDamageOrHealAmount(-1, getSpellFromID(SPELL_FIREBALL), nullptr);
			}
			snprintf(buf, sizeof(buf), str.c_str(), baseDmg + baseSpellDamage);
		}
		else if ( detailTag.compare("tool_bomb_per_atk") == 0 )
		{
			int perMult = (items[item.type].hasAttribute("BOMB_DMG_PER_MULT") ? items[item.type].attributes["BOMB_DMG_PER_MULT"] : 0);
			int perDmg = std::max(0, statGetPER(stats[player], players[player]->entity)) * perMult / 100.0;
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
								if ( spell_t* spell = getSpellFromItem(clientnum, item) )
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
									if ( spell_t* spell = getSpellFromItem(clientnum, item) )
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
				if ( spell_t* spell = getSpellFromItem(player, item) )
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
		achievementUnlockTime[ach.name] = ach.unlockTime;
		if ( ach.unlocked )
		{
			achievementUnlockedLookup.insert(ach.name);
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
		if ( steamStatAchStringsAndMaxVals[statNum].first != "BARONY_ACH_NONE" )
		{
			auto find = achievementProgress.find(steamStatAchStringsAndMaxVals[statNum].first);
			if ( find == achievementProgress.end() )
			{
				achievementProgress.emplace(std::make_pair(std::string(steamStatAchStringsAndMaxVals[statNum].first), statNum));
			}
			else
			{
				find->second = statNum;
			}
		}
		g_SteamStats[statNum].m_iValue = LocalAchievements.statistics[statNum].value;
	}
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
	for ( auto& ach : achievementNames )
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
	for ( auto& ach : achievementNames )
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
									messagePlayer(i, MESSAGE_HINT, language[4333]);
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, language[4334]);
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
									messagePlayer(i, MESSAGE_HINT, language[4342]);
								}
								else
								{
									messagePlayer(i, MESSAGE_HINT, language[4343]);
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
			collider.sfxBreak = itr->value["sfx_break"].GetInt();
			collider.sfxHit = itr->value["sfx_hit"].GetInt();
			collider.damageCalculationType = itr->value["damage_calc"].GetString();
			collider.entityLangEntry = itr->value["entity_lang_entry"].GetInt();
			collider.hitMessageLangEntry = itr->value["hit_message"].GetInt();
			collider.breakMessageLangEntry = itr->value["break_message"].GetInt();
			collider.hpbarLookupName = itr->value["hp_bar_lookup_name"].GetString();
		}
	}
}
