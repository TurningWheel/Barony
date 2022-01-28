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
#include "ui/MainMenu.hpp"

MonsterStatCustomManager monsterStatCustomManager;
MonsterCurveCustomManager monsterCurveCustomManager;
GameplayCustomManager gameplayCustomManager;
GameModeManager_t gameModeManager;
ItemTooltips_t ItemTooltips;
GlyphRenderer_t GlyphHelper;
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
	// deprecated
	assert(0 && "Make a new gameover window.");
	return;
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

#ifndef EDITOR
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
		}

		t.spellbookInternalName = spell_itr->value["spellbook_internal_name"].GetString();
		t.magicstaffInternalName = spell_itr->value["magicstaff_internal_name"].GetString();

		for ( int i = 0; i < NUMITEMS; ++i )
		{
			if ( items[i].category != SPELLBOOK && items[i].category != MAGICSTAFF )
			{
				continue;
			}
			if ( t.spellbookInternalName == tmpItems[i].itemName )
			{
				t.spellbookId = i;
			}
			if ( t.magicstaffInternalName == tmpItems[i].itemName )
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

	const int bufSize = 120000;
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
		if ( players[player] )
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
	else if ( item.type == SPELL_ITEM )
	{
		spell_t* spell = getSpellFromItem(player, &item);
		if ( spell )
		{
	        if (arachnophobia_filter)
	        {
	            if (spell->ID == SPELL_SPIDER_FORM)
	            {
                    spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_FORM);
	            }
	            else if (spell->ID == SPELL_SPRAY_WEB)
	            {
                    spellImageNode = list_Node(&items[SPELL_ITEM].images, SPELL_CRAB_WEB);
	            }
	            else
	            {
                    spellImageNode = list_Node(&items[SPELL_ITEM].images, spell->ID);
	            }
	        }
	        else
	        {
	            spellImageNode = list_Node(&items[SPELL_ITEM].images, spell->ID);
	        }
		}
		else
		{
			spellImageNode = list_Node(&items[SPELL_ITEM].images, 0);
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
				snprintf(buf, sizeof(buf), str.c_str(), spell->name);
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
			if ( conditionalAttribute == "EFF_REGENERATION" )
			{
				if ( item.type == RING_REGENERATION )
				{
					int healring = std::min(2, std::max(item.beatitude + 1, 1));
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
				auto oldStatus = item.status;
				item.status = DECREPIT;
				int lowVal = item.potionGetEffectHealth();
				item.status = EXCELLENT;
				int highVal = item.potionGetEffectHealth();
				item.status = oldStatus;
				snprintf(buf, sizeof(buf), str.c_str(), lowVal, highVal);
			}
			else if ( item.type == POTION_BOOZE )
			{
				if ( iconIndex == 1 )
				{
					auto oldBeatitude = item.beatitude;
					item.beatitude = std::max((Sint16)0, item.beatitude);
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectDurationMinimum() / TICKS_PER_SECOND, item.potionGetEffectDurationMaximum() / TICKS_PER_SECOND);
					item.beatitude = oldBeatitude;
				}
				else
				{
					snprintf(buf, sizeof(buf), str.c_str(), item.potionGetEffectHealth());
				}
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
				int radius = std::max(3, 11 + 5 * item.beatitude);
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
			int beatitudeBonus = getSpellbookBonusPercent(players[player]->entity, stats[player], &item) - intBonus;

			std::string damageOrHealing = adjectives["spell_strings"]["damage"];
			if ( spellItems[spell->ID].spellTags.find(SpellTagTypes::SPELL_TAG_HEALING)
				!= spellItems[spell->ID].spellTags.end() )
			{
				damageOrHealing = adjectives["spell_strings"]["healing"];
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
		CustomHelpers::addMemberToRoot(exportDocument, "statue_id", rapidjson::Value(rand()));
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

void StatueManager_t::readStatueFromFile(int index)
{
	std::string fileName = "/data/statues/statue" + std::to_string(index) + ".json";
	if ( PHYSFS_getRealDir(fileName.c_str()) )
	{
		std::string inputPath = PHYSFS_getRealDir(fileName.c_str());
		inputPath.append(fileName);

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
			int scancode = SDL_GetScancodeFromName(keyname.c_str());
			if ( scancode == SDL_SCANCODE_UNKNOWN )
			{
				printlog("[JSON]: Glyph name: %s could not find a scancode, skipping...", keyname.c_str());
				continue;
			}
			allGlyphs[scancode] = GlyphData_t();
			auto& glyphData = allGlyphs[scancode];
			glyphData.scancode = scancode;
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
			std::string tmpPath = basePath;
			tmpPath += glyphData.folder;
			tmpPath += '/';
			tmpPath += glyphData.filename;

			if ( !PHYSFS_getRealDir(tmpPath.c_str()) ) // you need single forward '/' slashes for getRealDir to report true
			{
				printlog("[JSON]: Glyph path: %s not detected, skipping...", tmpPath.c_str());
				glyphData.fullpath = "";
				glyphData.pressedRenderedFullpath = "";
				glyphData.unpressedRenderedFullpath = "";
				continue;
			}

			glyphData.fullpath = tmpPath;

			std::string renderedPath = baseRenderedPath;
			renderedPath += glyphData.folder;
			renderedPath += '/';

			glyphData.unpressedRenderedFullpath = renderedPath;
			glyphData.unpressedRenderedFullpath += unpressedRenderedPrefix;
			glyphData.unpressedRenderedFullpath += glyphData.filename;
			if ( glyphData.unpressedRenderedFullpath[0] == '/' )
			{
				glyphData.unpressedRenderedFullpath.erase((size_t)0, (size_t)1);
			}

			glyphData.pressedRenderedFullpath = renderedPath;
			glyphData.pressedRenderedFullpath += pressedRenderedPrefix;
			glyphData.pressedRenderedFullpath += glyphData.filename;
			if ( glyphData.pressedRenderedFullpath[0] == '/' )
			{
				glyphData.pressedRenderedFullpath.erase((size_t)0, (size_t)1);
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
#ifdef EDITOR
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
