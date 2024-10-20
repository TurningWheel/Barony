/*-------------------------------------------------------------------------------

	BARONY
	File: shopgui.cpp
	Desc: contains shop (GUI) related functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../shops.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../colors.hpp"
#include "../ui/Field.hpp"
#include "../mod_tools.hpp"
#include "../ui/GameUI.hpp"
#include "../ui/Image.hpp"
#include "../ui/Button.hpp"

#include <assert.h>

bool hideItemFromShopView(Item& item)
{
	if ( item.type == ARTIFACT_ORB_GREEN || item.type == ARTIFACT_ORB_RED || item.type == ARTIFACT_ORB_BLUE )
	{
		return true;
	}
	if ( item.itemHiddenFromShop )
	{
		return true;
	}
	return false;
}

std::string getShopTypeLangEntry(int shopType)
{
	if ( shopType < 10 )
	{
		return Language::get(184 + shopType);
	}
	else if ( shopType == 10 )
	{
		return Language::get(4128);
	}
	return "";
}

bool getShopFreeSlot(const int player, list_t* shopInventory, Item* itemToSell, int& xout, int& yout, Item*& itemToStackInto)
{
	xout = Player::ShopGUI_t::MAX_SHOP_X;
	yout = Player::ShopGUI_t::MAX_SHOP_Y;

	list_t* shopkeeperInv = nullptr;
	if ( player >= 0 )
	{
		shopkeeperInv = shopInv[player];
	}
	else
	{
		shopkeeperInv = shopInventory;
	}
	if ( !itemToSell || !shopkeeperInv )
	{
		return false;
	}

	bool lookForStackableItem = itemToSell->shouldItemStackInShop();
	if ( lookForStackableItem )
	{
		if ( itemTypeIsQuiver(itemToSell->type) )
		{
			lookForStackableItem = false; // don't stack, as we always buy the whole stack
		}
	}
	if ( lookForStackableItem )
	{
		for ( node_t* node = shopkeeperInv->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( !item->playerSoldItemToShop )
				{
					continue;
				}
				if ( !itemCompare(item, itemToSell, false) )
				{
					xout = item->x;
					yout = item->y;
					itemToStackInto = item;
					return true;
				}
			}
		}
	}

	std::unordered_set<int> takenSlots;
	for ( node_t* node = shopkeeperInv->first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item )
		{
			if ( hideItemFromShopView(*item) )
			{
				continue;
			}
			if ( !item->playerSoldItemToShop )
			{
				continue;
			}
			int key = item->x + 100 * item->y;
			if ( item->x >= 0 && item->x < Player::ShopGUI_t::MAX_SHOP_X
				&& item->y >= 0 && item->y < Player::ShopGUI_t::MAX_SHOP_Y )
			{
				takenSlots.insert(key);
			}
		}
	}

	for ( int y = 0; y < Player::ShopGUI_t::MAX_SHOP_Y; ++y )
	{
		for ( int x = 0; x < Player::ShopGUI_t::MAX_SHOP_X; ++x )
		{
			int key = x + 100 * y;
			if ( takenSlots.find(key) == takenSlots.end() )
			{
				xout = x;
				yout = y;
				return true;
			}
		}
	}
	return false;
}

/*-------------------------------------------------------------------------------

	updateShopWindow

	Draws and processes everything related to the shop window

-------------------------------------------------------------------------------*/

void updateShopWindow(const int player)
{
	SDL_Rect pos;
	node_t* node;
	int c;

	if ( player < 0 )
	{
		return;
	}
	if ( !uidToEntity(shopkeeper[player]) )
	{
		closeShop(player);
		return;
	}

	if ( multiplayer != CLIENT && players[player]->isLocalPlayer() )
	{
		Entity* entity = uidToEntity(shopkeeper[player]);
		if (entity)
		{
			Stat* stats = entity->getStats();
			shopkeepername[player] = stats->name;
		}
	}

	bool mysteriousShopkeeper = (shopkeepertype[player] == 10);
	bool mysteriousShopkeeperGreenOrb = false;
	bool mysteriousShopkeeperBlueOrb = false;
	bool mysteriousShopkeeperRedOrb = false;
	if ( mysteriousShopkeeper )
	{
		for ( node_t* node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( item->type == ARTIFACT_ORB_BLUE )
				{
					mysteriousShopkeeperBlueOrb = true;
					item->itemHiddenFromShop = true;
				}
				else if ( item->type == ARTIFACT_ORB_RED )
				{
					mysteriousShopkeeperRedOrb = true;
					item->itemHiddenFromShop = true;
				}
				else if ( item->type == ARTIFACT_ORB_GREEN )
				{
					mysteriousShopkeeperGreenOrb = true;
					item->itemHiddenFromShop = true;
				}
			}
		}
		for ( node = shopInv[player]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_BLUE].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperBlueOrb;
					continue;
				}
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_GREEN].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperGreenOrb;
					continue;
				}
				if ( shopkeeperMysteriousItems[ARTIFACT_ORB_RED].find(item->type) != shopkeeperMysteriousItems[ARTIFACT_ORB_RED].end() )
				{
					item->itemHiddenFromShop = !mysteriousShopkeeperRedOrb;
					continue;
				}
			}
		}
	}

	// chitchat
	if ( (ticks - shoptimer[player]) % 600 == 0 && !mysteriousShopkeeper )
	{
		shopspeech[player] = Language::get(216 + local_rng.rand() % NUMCHITCHAT);
		shoptimer[player]--;
	}

	// draw speech
	char buf[1024] = "";
	//if ( sellitem[clientnum] && shopspeech[player] == Language::get(215) )
	//{
	//	// "I would sell that for %d gold"
	//	snprintf(buf, sizeof(buf), shopspeech[player].c_str(), sellitem[player]->sellValue(player));
	//	if ( players[player]->shopGUI.chatStrFull != buf )
	//	{
	//		players[player]->shopGUI.chatTicks = ticks;
	//		players[player]->shopGUI.chatStrFull = buf;
	//		players[player]->shopGUI.chatStringLength = 0;
	//	}
	//}
	//else
	{
		if ( shopspeech[player] == Language::get(194) // greetings
			|| shopspeech[player] == Language::get(195)
			|| shopspeech[player] == Language::get(196) )
		{
			if ( mysteriousShopkeeper )
			{
				shopspeech[player] = Language::get(3893 + local_rng.rand() % 3);
				if ( players[player]->shopGUI.chatStrFull != shopspeech[player] )
				{
					players[player]->shopGUI.chatTicks = ticks;
					players[player]->shopGUI.chatStrFull = shopspeech[player];
					players[player]->shopGUI.chatStringLength = 0;
				}
			}
			else
			{
				char shopnamebuf[1024];
				snprintf(shopnamebuf, sizeof(shopnamebuf), Language::get(358), shopkeepername[player].c_str(), getShopTypeLangEntry(shopkeepertype[player]).c_str());

				snprintf(buf, sizeof(buf), "%s\n%s", shopspeech[player].c_str(), shopnamebuf); // greetings, welcome to %s's shop
				if ( players[player]->shopGUI.chatStrFull != buf )
				{
					players[player]->shopGUI.chatTicks = ticks;
					players[player]->shopGUI.chatStrFull = buf;
					players[player]->shopGUI.chatStringLength = 0;
				}
			}
		}
		else
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), shopspeech[player].c_str(), 0);
			if ( players[player]->shopGUI.chatStrFull != buf )
			{
				players[player]->shopGUI.chatTicks = ticks;
				players[player]->shopGUI.chatStrFull = buf;
				players[player]->shopGUI.chatStringLength = 0;
			}
		}
	}
}

void shopChangeGoldEvent(const int player, Sint32 amount)
{
	if ( !players[player]->isLocalPlayer() )
	{
		return;
	}

	bool addedToCurrentTotal = false;
	bool isAnimatingValue = ((ticks - players[player]->shopGUI.animGoldStartTicks) > TICKS_PER_SECOND / 2);
	if ( amount < 0 )
	{
		if ( players[player]->shopGUI.playerChangeGold < 0 
			&& !isAnimatingValue 
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			if ( stats[player]->GOLD + amount < 0 )
			{
				players[player]->shopGUI.playerChangeGold -= stats[player]->GOLD;
			}
			else
			{
				players[player]->shopGUI.playerChangeGold += amount;
			}
		}
		else
		{
			if ( stats[player]->GOLD + amount < 0 )
			{
				players[player]->shopGUI.playerChangeGold = -stats[player]->GOLD;
			}
			else
			{
				players[player]->shopGUI.playerChangeGold = amount;
			}
		}
	}
	else
	{
		if ( players[player]->shopGUI.playerChangeGold > 0 
			&& !isAnimatingValue
			&& abs(amount) > 0 )
		{
			addedToCurrentTotal = true;
			players[player]->shopGUI.playerChangeGold += amount;
		}
		else
		{
			players[player]->shopGUI.playerChangeGold = amount;
		}
	}
	players[player]->shopGUI.animGoldStartTicks = ticks;
	players[player]->shopGUI.animGold = 0.0;
	if ( !addedToCurrentTotal )
	{
		players[player]->shopGUI.playerCurrentGold = stats[player]->GOLD;
	}
}

void Player::ShopGUI_t::openShop()
{
	if ( shopFrame )
	{
		bool wasDisabled = shopFrame->isDisabled();
		shopFrame->setDisabled(false);
		if ( wasDisabled )
		{
			animx = 0.0;
			animTooltip = 0.0;
			isInteractable = false;
			bFirstTimeSnapCursor = false;
			lastTooltipModule = Player::GUI_t::MODULE_NONE;
		}
		if ( getSelectedShopX() < 0 || getSelectedShopX() >= MAX_SHOP_X
			|| getSelectedShopY() < 0 || getSelectedShopY() >= MAX_SHOP_Y )
		{
			selectShopSlot(0, 0);
		}
		player.hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		player.inventory_mode = INVENTORY_MODE_ITEM;
		player.gui_mode = GUI_MODE_SHOP;
		bOpen = true;

		bool flipped = player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT;

		if ( auto bgFrame = shopFrame->findFrame("shop base") )
		{
			if ( auto shopName = bgFrame->findField("shop name") )
			{
				if ( shopkeepertype[player.playernum] == 10 ) // mysterious shopkeep
				{
					shopName->setText(Language::get(4129)); // '???'s'

					// opening shop triggers shopkeep compendium reveal
					auto find = Compendium_t::Events_t::monsterUniqueIDLookup.find("mysterious shop");
					if ( find != Compendium_t::Events_t::monsterUniqueIDLookup.end() )
					{
						auto find2 = Compendium_t::Events_t::monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + find->second);
						if ( find2 != Compendium_t::Events_t::monsterIDToString.end() )
						{
							auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find2->second];
							if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
							{
								unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
							}
						}
					}
				}
				else
				{
					char buf[64];
					snprintf(buf, sizeof(buf), Language::get(4127), shopkeepername[player.playernum].c_str());
					shopName->setText(buf);

					// opening shop triggers shopkeep compendium reveal
					auto find = Compendium_t::Events_t::monsterIDToString.find(Compendium_t::Events_t::kEventMonsterOffset + SHOPKEEPER);
					if ( find != Compendium_t::Events_t::monsterIDToString.end() )
					{
						auto& unlockStatus = Compendium_t::CompendiumMonsters_t::unlocks[find->second];
						if ( unlockStatus == Compendium_t::CompendiumUnlockStatus::LOCKED_UNKNOWN )
						{
							unlockStatus = Compendium_t::CompendiumUnlockStatus::LOCKED_REVEALED_UNVISITED;
						}
					}
				}

				SDL_Rect pos = shopName->getSize();
				if ( flipped )
				{
					pos.x = 108;
				}
				else
				{
					pos.x = 228;
				}
				shopName->setSize(pos);
			}
			if ( auto shopType = bgFrame->findField("shop type") )
			{
				SDL_Rect pos = shopType->getSize();
				if ( flipped )
				{
					pos.x = 108;
				}
				else
				{
					pos.x = 228;
				}
				shopType->setSize(pos);
				shopType->setText(getShopTypeLangEntry(shopkeepertype[player.playernum]).c_str());
			}
		}
	}
	if ( inputs.getUIInteraction(player.playernum)->selectedItem )
	{
		inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
		inputs.getUIInteraction(player.playernum)->toggleclick = false;
	}
	inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
	clearItemDisplayed();
	shopChangeGoldEvent(player.playernum, 0);
	buybackView = false;
}

void Player::ShopGUI_t::selectShopSlot(const int x, const int y)
{
	selectedShopSlotX = x;
	selectedShopSlotY = y;
}

void Player::ShopGUI_t::closeShop()
{
	if ( shopFrame )
	{
		shopFrame->setDisabled(true);
	}
	animx = 0.0;
	animTooltip = 0.0;
	isInteractable = false;
	bool wasOpen = bOpen;
	bOpen = false;
	bFirstTimeSnapCursor = false;
	if ( wasOpen )
	{
		if ( inputs.getUIInteraction(player.playernum)->selectedItem )
		{
			inputs.getUIInteraction(player.playernum)->selectedItem = nullptr;
			inputs.getUIInteraction(player.playernum)->toggleclick = false;
		}
		inputs.getUIInteraction(player.playernum)->selectedItemFromChest = 0;
	}
	if ( players[player.playernum]->GUI.activeModule == Player::GUI_t::MODULE_SHOP
		&& !players[player.playernum]->shootmode )
	{
		// reset to inventory mode if still hanging in shop GUI
		players[player.playernum]->hud.compactLayoutMode = Player::HUD_t::COMPACT_LAYOUT_INVENTORY;
		players[player.playernum]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
		if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			players[player.playernum]->GUI.warpControllerToModule(false);
		}
	}
	if ( !players[player.playernum]->shootmode )
	{
		if ( players[player.playernum]->gui_mode == GUI_MODE_SHOP )
		{
			players[player.playernum]->gui_mode = GUI_MODE_INVENTORY;
		}
	}

    if ( shopFrame )
    {
	    if ( auto bgFrame = shopFrame->findFrame("shop base") )
	    {
		    if ( auto discountFrame = bgFrame->findFrame("discount frame") )
		    {
			    if ( auto discountValue = discountFrame->findField("discount") )
			    {
				    discountValue->setText("");
			    }
		    }
	    }
	}
	clearItemDisplayed();
	itemRequiresTitleReflow = true;
	shopChangeGoldEvent(player.playernum, 0);
	buybackView = false;
	chatStrFull = "";
	chatStringLength = 0;
	chatTicks = 0;
}

bool Player::ShopGUI_t::isShopSelected()
{
	if ( !bOpen )
	{
		return false;
	}

	if ( player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
	{
		if ( selectedShopSlotX >= 0 && selectedShopSlotX < MAX_SHOP_X
			&& selectedShopSlotY >= 0 && selectedShopSlotY < MAX_SHOP_Y )
		{
			return true;
		}
	}

	return false;
}

int Player::ShopGUI_t::heightOffsetWhenNotCompact = 172;

void updatePlayerGold(const int player, const int flipped)
{
	auto shopFrame = players[player]->shopGUI.shopFrame;
	if ( !shopFrame )
	{
		return;
	}

	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);
	auto currentGoldText = bgFrame->findField("current gold");
	auto changeGoldText = bgFrame->findField("change gold");
	auto currentGoldLabelText = bgFrame->findField("current gold label");
	if ( flipped )
	{
		std::string s = Language::get(4119);
		if ( flipped )
		{
			size_t found = s.find(':'); // remove colon as text to right of box
			if ( found != std::string::npos )
			{
				s.erase(found);
			}
		}
		currentGoldLabelText->setText(s.c_str());
	}
	else
	{
		currentGoldLabelText->setText(Language::get(4119));
	}
	SDL_Rect changeGoldPos = changeGoldText->getSize();
	const int changePosAnimHeight = 10;
	changeGoldPos.y = bgFrame->getSize().h - 92 - 16;

	real_t& animGold = players[player]->shopGUI.animGold;
	bool pauseChangeGoldAnim = false;

	real_t& animNoDeal = players[player]->shopGUI.animNoDeal;
	Uint32& animNoDealTicks = players[player]->shopGUI.animNoDealTicks;

	if ( players[player]->shopGUI.playerChangeGold != 0 )
	{
		if ( ((ticks - players[player]->shopGUI.animGoldStartTicks) > TICKS_PER_SECOND / 2) )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.1, (animGold)) / 10.0;
			animGold -= setpointDiffX;
			animGold = std::max(0.0, animGold);

			if ( animGold <= 0.0001 )
			{
				players[player]->shopGUI.playerChangeGold = 0;
			}
		}
		else
		{
			pauseChangeGoldAnim = true;

			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animGold)) / 10.0;
			animGold += setpointDiffX;
			animGold = std::min(1.0, animGold);

			changeGoldPos.y += changePosAnimHeight;
			changeGoldPos.y -= animGold * changePosAnimHeight;
		}
	}

	if ( flipped )
	{
		changeGoldPos.x = 26;
		SDL_Rect currentGoldLabelPos = currentGoldLabelText->getSize();
		currentGoldLabelPos.x = 116;
		currentGoldLabelText->setSize(currentGoldLabelPos);
		currentGoldLabelText->setHJustify(Field::justify_t::LEFT);
	}
	else
	{
		changeGoldPos.x = bgFrame->getSize().w - 80 - 18;
		SDL_Rect currentGoldLabelPos = currentGoldLabelText->getSize();
		currentGoldLabelPos.x = bgFrame->getSize().w - 106 - 100 - 12;
		currentGoldLabelText->setSize(currentGoldLabelPos);
		currentGoldLabelText->setHJustify(Field::justify_t::RIGHT);
	}
	changeGoldText->setSize(changeGoldPos);

	{ 
		// constant decay for animation
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * 1.0 / 25.0;
		animNoDeal -= setpointDiffX;
		animNoDeal = std::max(0.0, animNoDeal);

		SDL_Rect currentGoldTextPos = currentGoldText->getSize();
		if ( flipped )
		{
			currentGoldTextPos.x = 26;
		}
		else
		{
			currentGoldTextPos.x = bgFrame->getSize().w - 80 - 18;
		}
		currentGoldTextPos.x += -2 + 2 * (cos(animNoDeal * 4 * PI));
		currentGoldText->setSize(currentGoldTextPos);
		if ( animNoDeal > 0.001 || (ticks - animNoDealTicks) < TICKS_PER_SECOND * .8 )
		{
			currentGoldText->setColor(makeColor(215, 38, 61, 255)); // red
		}
		else
		{
			currentGoldText->setColor(makeColor(201, 162, 100, 255));
		}
	}

	bool showChangedGold = false;
	if ( players[player]->shopGUI.playerChangeGold != 0 )
	{
		Sint32 displayedChangeGold = animGold * players[player]->shopGUI.playerChangeGold;
		if ( pauseChangeGoldAnim )
		{
			displayedChangeGold = players[player]->shopGUI.playerChangeGold;
		}
		if ( abs(displayedChangeGold) > 0 )
		{
			showChangedGold = true;
			changeGoldText->setDisabled(false);
			std::string s = "+";
			if ( players[player]->shopGUI.playerChangeGold < 0 )
			{
				s = "";
			}
			s += std::to_string(displayedChangeGold);
			changeGoldText->setText(s.c_str());
			Sint32 displayedCurrentGold = players[player]->shopGUI.playerCurrentGold 
				+ (players[player]->shopGUI.playerChangeGold - displayedChangeGold);
			currentGoldText->setText(std::to_string(displayedCurrentGold).c_str());
		}
	}
	
	if ( !showChangedGold )
	{
		Sint32 displayedChangeGold = 0;
		changeGoldText->setDisabled(true);
		changeGoldText->setText(std::to_string(displayedChangeGold).c_str());
		currentGoldText->setText(std::to_string(stats[player]->GOLD).c_str());
	}
}

void updateShopGUIChatter(const int player, const bool flipped)
{
	auto shopFrame = players[player]->shopGUI.shopFrame;
	if ( !shopFrame )
	{
		return;
	}

	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);
	auto chatWindow = bgFrame->findFrame("chatter");
	assert(chatWindow);

	auto tl = chatWindow->findImage("top left img");
	auto tm = chatWindow->findImage("top img");
	auto tr = chatWindow->findImage("top right img");

	auto ml = chatWindow->findImage("middle left img");
	auto mm1 = chatWindow->findImage("middle 1 img");
	auto mm2 = chatWindow->findImage("middle 2 img");
	auto mr = chatWindow->findImage("middle right img");

	auto bl = chatWindow->findImage("bottom left img");
	auto bm = chatWindow->findImage("bottom img");
	auto br = chatWindow->findImage("bottom right img");

	auto pointer = chatWindow->findImage("pointer img");

	auto shopkeeperImg = bgFrame->findImage("shopkeeper img");
	if ( shopkeepertype[player] == 10 )
	{
		shopkeeperImg->path = "images/ui/Shop/shopkeeper2.png";
	}
	else
	{
		shopkeeperImg->path = "images/ui/Shop/shopkeeper.png";
	}
	if ( flipped )
	{
		shopkeeperImg->pos.x = 14;
	}
	else
	{
		shopkeeperImg->pos.x = shopFrame->getSize().w - 14 - 80;
	}

	int width = 288;
	int height = 200;
	const int pointerHeightAddition = 10;
	SDL_Rect chatPos{ bgFrame->getSize().w - 12 - width, 88, width, height + pointerHeightAddition };
	if ( flipped )
	{
		chatPos.x = 6;
	}
	else
	{
		chatPos.x = bgFrame->getSize().w - 12 - width;
	}

	if ( !players[player]->shopGUI.bOpen )
	{
		players[player]->shopGUI.chatStrFull = "";
		players[player]->shopGUI.chatStringLength = 0;
		players[player]->shopGUI.chatTicks = 0;
		chatWindow->setDisabled(true);
		return;
	}

	if ( players[player]->shopGUI.chatStrFull.size() > 1 )
	{
		chatWindow->setDisabled(false);
		auto chatText = chatWindow->findField("chat body");
		if ( players[player]->shopGUI.chatTicks - ticks > 1 )
		{
			players[player]->shopGUI.chatTicks = ticks;

			SDL_Rect textPos{ 22, 18 + pointerHeightAddition + 1, 0, 0 };
			textPos.w = chatPos.w - textPos.x - 14 - 4;
			textPos.h = chatPos.h - 14 - textPos.y;
			chatText->setSize(textPos);

			size_t& currentLen = players[player]->shopGUI.chatStringLength;
			size_t fullLen = players[player]->shopGUI.chatStrFull.size();

			if ( currentLen < fullLen )
			{
				chatText->setText(players[player]->shopGUI.chatStrFull.substr(0U, currentLen + 1).c_str());
				++currentLen;
				/*if ( auto textGet = chatText->getTextObject() )
				{
					if ( textGet->getWidth() >= textPos.w )
					{
						chatText->reflowTextToFit(0); // if we want auto-reflowed text, but shop lines are currently aligned manually in lang file
					}
				}*/
			}
			else if ( currentLen > fullLen )
			{
				currentLen = fullLen;
			}

			if ( Font* actualFont = Font::get(chatText->getFont()) )
			{
				textPos.h = std::min(textPos.h, chatText->getNumTextLines() * actualFont->height(true) + 12);
				chatText->setSize(textPos);
			}
		}

		chatPos.h = std::min(chatPos.h, (chatText->getSize().y + chatText->getSize().h));
		chatWindow->setSize(chatPos);

		const int mainTextAreaY = pointerHeightAddition;
		pointer->pos.y = 0;
		if ( flipped )
		{
			pointer->pos.x = 82;
			pointer->path = "*#images/ui/Shop/Textbox_SpeakerPointer_TL01.png";
		}
		else
		{
			pointer->pos.x = chatPos.w - 96;
			pointer->path = "*#images/ui/Shop/Textbox_SpeakerPointer_TR01.png";
		}

		tl->pos.x = 0;
		tl->pos.y = mainTextAreaY;
		tr->pos.x = width - tr->pos.w;
		tr->pos.y = tl->pos.y + 6;
		tm->pos.x = tl->pos.x + tl->pos.w;
		tm->pos.y = tl->pos.y + 6;
		tm->pos.w = tr->pos.x - (tl->pos.x + tl->pos.w);

		bl->pos.x = tl->pos.x + 6;
		bl->pos.y = chatPos.h - bl->pos.h;
		br->pos.x = width - br->pos.w;
		br->pos.y = chatPos.h - br->pos.h;

		ml->pos.x = tl->pos.x + 6;
		ml->pos.y = tl->pos.y + tl->pos.h;
		ml->pos.h = bl->pos.y - (tl->pos.y + tl->pos.h);

		mr->pos.x = width - mr->pos.w;
		mr->pos.y = tr->pos.y + tr->pos.h;
		mr->pos.h = br->pos.y - (mr->pos.y);

		bm->pos.x = bl->pos.x + bl->pos.w;
		bm->pos.y = br->pos.y + br->pos.h - bm->pos.h;
		bm->pos.w = tm->pos.w;

		mm1->pos.x = tm->pos.x;
		mm1->pos.y = tm->pos.y + tm->pos.h;
		mm1->pos.h = bm->pos.y - mm1->pos.y;
		mm1->pos.w = tm->pos.w;

		mm2->pos.x = ml->pos.x + ml->pos.w;
		mm2->pos.y = ml->pos.y;
		mm2->pos.h = ml->pos.h;
		mm2->pos.w = mr->pos.x - (ml->pos.x + ml->pos.w);
	}
	else
	{
		chatWindow->setDisabled(true);
	}
}

void Player::ShopGUI_t::clearItemDisplayed()
{
	itemPrice = -1;
	itemUnknownPreventPurchase = false;
}

void Player::ShopGUI_t::setItemDisplayNameAndPrice(Item* item)
{
	if ( !item || item->type == SPELL_ITEM )
	{
		clearItemDisplayed();
	}
	auto bgFrame = shopFrame->findFrame("shop base");
	Field* buyOrSellPrompt = nullptr;
	Frame::image_t* orbImg = nullptr;
	if ( bgFrame )
	{
		auto buyTooltipFrame = shopFrame->findFrame("buy tooltip frame");
		orbImg = buyTooltipFrame->findImage("orb img");
		orbImg->disabled = true;
		buyOrSellPrompt = buyTooltipFrame->findField("buy prompt txt");
	}
	itemUnknownPreventPurchase = false;
	if ( isItemFromShop(item) )
	{
		itemPrice = item->buyValue(player.playernum);
		char buf[1024];

		bool hiddenItemInGUI = false;
		int itemSkillReq = 0;
		if ( item->itemSpecialShopConsumable )
		{
			itemSkillReq = ((int)item->itemRequireTradingSkillInShop) * SHOP_CONSUMABLE_SKILL_REQ_PER_POINT;
			if ( stats[player.playernum]->getModifiedProficiency(PRO_TRADING) + statGetCHR(stats[player.playernum], players[player.playernum]->entity) < itemSkillReq )
			{
				hiddenItemInGUI = true;
				itemUnknownPreventPurchase = true;
			}
		}
		if ( hiddenItemInGUI )
		{
			snprintf(buf, sizeof(buf), Language::get(4251), itemSkillReq);
		}
		else if ( !item->identified )
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
		else
		{
			if ( (item->type == TOOL_SENTRYBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_SPELLBOT
				|| item->type == TOOL_GYROBOT) )
			{
				int health = 100;
				if ( !item->tinkeringBotIsMaxHealth() )
				{
					health = 25 * (item->appearance % 10);
					if ( health == 0 && item->status != BROKEN )
					{
						health = 5;
					}
				}
				snprintf(buf, sizeof(buf), "%s %s (%d%%)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), health);
			}
			else if ( item->type == ENCHANTED_FEATHER && item->identified )
			{
				snprintf(buf, sizeof(buf), "%s %s (%d%%) (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
					item->getName(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY, item->beatitude);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
			}
		}
		if ( itemDesc != buf )
		{
			itemRequiresTitleReflow = true;
		}
		itemDesc = buf;
		if ( buyOrSellPrompt )
		{
			buyOrSellPrompt->setText(Language::get(4113)); // 'buy'
		}
		if ( shopkeepertype[player.playernum] == 10 )
		{
			for ( auto orbCategories : shopkeeperMysteriousItems )
			{
				if ( orbCategories.second.find(item->type) != orbCategories.second.end() )
				{
					ItemType oldType = item->type;
					item->type = (ItemType)orbCategories.first;
					if ( orbImg )
					{
						orbImg->path = getItemSpritePath(player.playernum, *item);
						orbImg->disabled = false;
					}
					item->type = oldType;
					break;
				}
			}
		}
	}
	else
	{
		itemPrice = item->sellValue(player.playernum);
		char buf[1024];
		if ( !item->identified )
		{
			snprintf(buf, sizeof(buf), "%s %s (?)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName());
		}
		else
		{
			if ( (item->type == TOOL_SENTRYBOT || item->type == TOOL_DUMMYBOT || item->type == TOOL_SPELLBOT
				|| item->type == TOOL_GYROBOT) )
			{
				int health = 100;
				if ( !item->tinkeringBotIsMaxHealth() )
				{
					health = 25 * (item->appearance % 10);
					if ( health == 0 && item->status != BROKEN )
					{
						health = 5;
					}
				}
				snprintf(buf, sizeof(buf), "%s %s (%d%%)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), health);
			}
			else if ( item->type == ENCHANTED_FEATHER && item->identified )
			{
				snprintf(buf, sizeof(buf), "%s %s (%d%%) (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(),
					item->getName(), item->appearance % ENCHANTED_FEATHER_MAX_DURABILITY, item->beatitude);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s (%+d)", ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
			}
		}
		if ( itemDesc != buf )
		{
			itemRequiresTitleReflow = true;
		}
		itemDesc = buf;
		if ( buyOrSellPrompt )
		{
			buyOrSellPrompt->setText(Language::get(4114)); // 'sell'
		}
	}

	if ( bgFrame )
	{
		auto buyTooltipFrame = shopFrame->findFrame("buy tooltip frame");
		auto itemSlotFrame = buyTooltipFrame->findFrame("item slot frame");
		int oldQty = item->count;
		if ( !itemTypeIsQuiver(item->type) )
		{
			item->count = 1;
		}
		updateSlotFrameFromItem(itemSlotFrame, item);
		item->count = oldQty;
	}
}

void buttonShopUpdateSelectorOnHighlight(const int player, Button* button)
{
	if ( button->isHighlighted() )
	{
		players[player]->GUI.setHoveringOverModuleButton(Player::GUI_t::MODULE_SHOP);
		if ( players[player]->GUI.activeModule != Player::GUI_t::MODULE_SHOP )
		{
			players[player]->GUI.activateModule(Player::GUI_t::MODULE_SHOP);
		}
		SDL_Rect pos = button->getAbsoluteSize();
		// make sure to adjust absolute size to camera viewport
		pos.x -= players[player]->camera_virtualx1();
		pos.y -= players[player]->camera_virtualy1();
		players[player]->hud.setCursorDisabled(false);
		players[player]->hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, inputs.getVirtualMouse(player)->draw_cursor);
	}
}

void Player::ShopGUI_t::updateShop()
{
	updateShopWindow(player.playernum);

	if ( !shopFrame )
	{
		return;
	}
	shopFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
		players[player.playernum]->camera_virtualy1(),
		shopFrame->getSize().w,
		shopFrame->getSize().h });

	if ( !shopFrame->isDisabled() && bOpen )
	{
		const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
		real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animx)) / 2.0;
		animx += setpointDiffX;
		animx = std::min(1.0, animx);
		if ( animx >= .9999 )
		{
			if ( !bFirstTimeSnapCursor )
			{
				bFirstTimeSnapCursor = true;
				if ( !inputs.getUIInteraction(player.playernum)->selectedItem
					&& player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
				{
					warpMouseToSelectedShopItem(nullptr, (Inputs::SET_CONTROLLER));
				}
			}
			isInteractable = true;
		}
	}
	else
	{
		animx = 0.0;
		isInteractable = false;
	}

	auto shopFramePos = shopFrame->getSize();
	if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_LEFT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			const int fullWidth = shopFramePos.w + 210; // inventory width 210
			shopFramePos.x = -shopFramePos.w + animx * fullWidth;
			/*if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}*/
		}
		else
		{
			shopFramePos.x = player.camera_virtualWidth() - animx * shopFramePos.w;
			/*if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}*/
		}
	}
	else if ( player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
	{
		if ( !player.inventoryUI.bCompactView )
		{
			shopFramePos.x = player.camera_virtualWidth() - animx * shopFramePos.w * 2;
			/*if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= -player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}*/
		}
		else
		{
			shopFramePos.x = -shopFramePos.w + animx * shopFramePos.w;
			/*if ( player.bUseCompactGUIWidth() )
			{
				if ( player.inventoryUI.slideOutPercent >= .0001 )
				{
					isInteractable = false;
				}
				shopFramePos.x -= player.inventoryUI.slideOutWidth * player.inventoryUI.slideOutPercent;
			}*/
		}
	}

	if ( !player.bUseCompactGUIHeight() && !player.bUseCompactGUIWidth() )
	{
		shopFramePos.y = heightOffsetWhenNotCompact;
	}
	else
	{
		shopFramePos.y = 0;
	}

	shopFrame->setSize(shopFramePos);

	bool purchaseItemAction = false;
	bool usingGamepad = inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor;

	if ( !player.isLocalPlayerAlive()
		|| (player.gui_mode != GUI_MODE_SHOP && player.inventory_mode != INVENTORY_MODE_SPELL) 
		// cycleInventoryTab() sets proper gui mode after spell list viewing with INVENTORY_MODE_SPELL
		// this allows us to browse spells while shop is open.
		|| stats[player.playernum]->HP <= 0
		|| !player.entity
		|| player.shootmode )
	{
		::closeShop(player.playernum);
		return;
	}
	if ( bOpen && isInteractable && shopInv[player.playernum] )
	{
		if ( !inputs.getUIInteraction(player.playernum)->selectedItem
			&& !player.GUI.isDropdownActive()
			&& player.GUI.bModuleAccessibleWithMouse(Player::GUI_t::MODULE_SHOP)
			&& !player.inventoryUI.chestGUI.bOpen
			&& !player.inventoryUI.spellPanel.bOpen
			&& player.bControlEnabled && !gamePaused
			&& !player.usingCommand() )
		{
			if ( Input::inputs[player.playernum].binaryToggle("MenuCancel") )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuCancel");
				::closeShop(player.playernum);
				Player::soundCancel();
				return;
			}
			else
			{
				if ( usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuConfirm") )
				{
					purchaseItemAction = true;
					Input::inputs[player.playernum].consumeBinaryToggle("MenuConfirm");
				}
				else if ( !usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuRightClick") )
				{
					purchaseItemAction = true;
					Input::inputs[player.playernum].consumeBinaryToggle("MenuRightClick");
				}
				else if ( usingGamepad && Input::inputs[player.playernum].binaryToggle("MenuPageRightAlt") )
				{
					toggleShopBuybackView(player.playernum);
					Input::inputs[player.playernum].consumeBinaryToggle("MenuPageRightAlt");
				}
			}
		}
	}

	if ( lastTooltipModule != player.GUI.activeModule
		&& (player.GUI.activeModule == Player::GUI_t::MODULE_SHOP
			|| player.GUI.activeModule == Player::GUI_t::MODULE_INVENTORY) )
	{
		animTooltip = 0.0;
		animTooltipTicks = 0;
	}
	lastTooltipModule = player.GUI.activeModule;

	if ( purchaseItemAction && itemPrice >= 0 && player.GUI.activeModule == Player::GUI_t::MODULE_SHOP )
	{
		for ( node_t* node = shopInv[player.playernum]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( buybackView && !item->playerSoldItemToShop )
				{
					continue;
				}
				else if ( !buybackView && item->playerSoldItemToShop )
				{
					continue;
				}
				if ( !hideItemFromShopView(*item)
					&& item->x == getSelectedShopX()
					&& item->y == getSelectedShopY()
					&& getSelectedShopX() >= 0 && getSelectedShopX() < MAX_SHOP_X
					&& getSelectedShopY() >= 0 && getSelectedShopX() < MAX_SHOP_Y )
				{
					int oldQty = item->count;
					bool consumedEntireStack = false;
					if ( buyItemFromShop(player.playernum, item, consumedEntireStack) )
					{
						if ( consumedEntireStack )
						{
							animTooltipTicks = 0;
							animTooltip = 0.0;
							clearItemDisplayed();
						}
					}
					break;
				}
			}
		}
	}

	int buybackItems = 0;
	if ( bOpen && shopInv[player.playernum] )
	{
		for ( node_t* node = shopInv[player.playernum]->first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			if ( item )
			{
				if ( hideItemFromShopView(*item) )
				{
					continue;
				}
				if ( item->playerSoldItemToShop )
				{
					++buybackItems;
				}
			}
		}
	}
	auto bgFrame = shopFrame->findFrame("shop base");
	assert(bgFrame);

	auto bgGrid = bgFrame->findImage("shop grid img");
	if ( buybackView )
	{
		bgGrid->path = "images/ui/Shop/Shop_ItemSlots_Buyback_Areas03.png";
	}
	else
	{
		bgGrid->path = "images/ui/Shop/Shop_ItemSlots_Areas03.png";
	}

	bool flipped = player.inventoryUI.inventoryPanelJustify == Player::PANEL_JUSTIFY_RIGHT;
	if ( flipped )
	{
		bgGrid->pos.x = 302;
		if ( auto bgImg = bgFrame->findImage("shop base img") )
		{
			bgImg->path = "*#images/ui/Shop/Shop_Window_03C_Flipped.png";
		}
		if ( auto slots = shopFrame->findFrame("shop slots") )
		{
			SDL_Rect pos = slots->getSize();
			pos.x = 302;
			slots->setSize(pos);
		}
	}
	else
	{
		bgGrid->pos.x = 12;
		if ( auto bgImg = bgFrame->findImage("shop base img") )
		{
			bgImg->path = "*#images/ui/Shop/Shop_Window_03C.png";
		}
		if ( auto slots = shopFrame->findFrame("shop slots") )
		{
			SDL_Rect pos = slots->getSize();
			pos.x = 12;
			slots->setSize(pos);
		}
	}

	auto buyTooltipFrame = shopFrame->findFrame("buy tooltip frame");
	buyTooltipFrame->setDisabled(false);

	auto displayItemName = buyTooltipFrame->findField("item display name");
	auto displayItemValue = buyTooltipFrame->findField("item display value");
	auto itemTooltipImg = buyTooltipFrame->findImage("tooltip img");
	itemTooltipImg->disabled = false;
	auto itemGoldImg = buyTooltipFrame->findImage("gold img");
	auto itemSlotFrame = buyTooltipFrame->findFrame("item slot frame");
	auto buyPromptText = buyTooltipFrame->findField("buy prompt txt");
	auto buyPromptGlyphFrame = buyTooltipFrame->findFrame("buy prompt frame");
	auto buyPromptGlyph = buyPromptGlyphFrame->findImage("buy prompt glyph");
	auto itemBgImg = buyTooltipFrame->findImage("item bg img");
	itemBgImg->pos.y = itemTooltipImg->pos.y + itemTooltipImg->pos.h / 2 - itemBgImg->pos.h / 2;
	if ( animTooltip <= 0.0001 )
	{
		displayItemName->setDisabled(true);
		displayItemValue->setDisabled(true);
		itemGoldImg->disabled = true;
		itemSlotFrame->setDisabled(true);
		buyPromptText->setDisabled(true);
		buyPromptGlyph->disabled = true;
		buyPromptGlyphFrame->setDisabled(true);
	}

	bool drawGlyphs = !::inputs.getVirtualMouse(player.playernum)->draw_cursor;
	auto closeBtn = bgFrame->findButton("close shop button");
	auto buybackBtn = bgFrame->findButton("buyback button");
	auto buybackPromptTxt = bgFrame->findField("buyback txt");
	auto buybackPromptGlyph = bgFrame->findImage("buyback glyph");
	auto closePromptTxt = bgFrame->findField("close shop prompt");
	auto closePromptGlyph = bgFrame->findImage("close shop glyph");
	char buybackText[32];
	if ( buybackItems > 0 )
	{
		snprintf(buybackText, sizeof(buybackText), "%s (%d)", Language::get(4120), buybackItems);
	}
	else
	{
		snprintf(buybackText, sizeof(buybackText), "%s", Language::get(4120));
	}
	if ( !drawGlyphs )
	{
		closeBtn->setInvisible(false);
		buybackBtn->setInvisible(false);
		buybackBtn->setDisabled(!isInteractable);
		if ( buybackView )
		{
			buybackBtn->setText(Language::get(4124));
		}
		else
		{
			buybackBtn->setText(buybackText);
		}
		closeBtn->setDisabled(!isInteractable);
		if ( isInteractable )
		{
			buttonShopUpdateSelectorOnHighlight(player.playernum, closeBtn);
			buttonShopUpdateSelectorOnHighlight(player.playernum, buybackBtn);
		}
		SDL_Rect closeBtnPos = closeBtn->getSize();
		if ( flipped )
		{
			closeBtnPos.x = 8;
		}
		else
		{
			closeBtnPos.x = bgFrame->getSize().w - 34;
		}
		closeBtn->setSize(closeBtnPos);

		if ( flipped )
		{
			SDL_Rect buybackBtnPos = buybackBtn->getSize();
			buybackBtnPos.x = 14;
			buybackBtn->setSize(buybackBtnPos);
		}
		else
		{
			SDL_Rect buybackBtnPos = buybackBtn->getSize();
			buybackBtnPos.x = bgFrame->getSize().w - 208;
			buybackBtn->setSize(buybackBtnPos);
		}

		closePromptGlyph->disabled = true;
		closePromptTxt->setDisabled(true);
		buybackPromptGlyph->disabled = true;
		buybackPromptTxt->setDisabled(true);
	}
	else
	{
		closeBtn->setInvisible(true);
		buybackBtn->setInvisible(true);
		buybackBtn->setDisabled(true);
		closeBtn->setDisabled(true);
		if ( closeBtn->isSelected() )
		{
			closeBtn->deselect();
		}
		if ( buybackBtn->isSelected() )
		{
			buybackBtn->deselect();
		}
		closePromptTxt->setDisabled(false);
		closePromptTxt->setText(Language::get(4121));
		buybackPromptTxt->setDisabled(false);
		if ( buybackView )
		{
			buybackPromptTxt->setText(Language::get(4124));
		}
		else
		{
			buybackPromptTxt->setText(buybackText);
		}

		if ( flipped )
		{
			SDL_Rect buybackPromptTxtPos = buybackPromptTxt->getSize();
			buybackPromptTxtPos.x = 44;
			buybackPromptTxt->setSize(buybackPromptTxtPos);

			SDL_Rect closePromptTxtPos = closePromptTxt->getSize();
			closePromptTxtPos.x = 44;
			closePromptTxt->setSize(closePromptTxtPos);
		}
		else
		{
			SDL_Rect buybackPromptTxtPos = buybackPromptTxt->getSize();
			buybackPromptTxtPos.x = bgFrame->getSize().w - 178;
			buybackPromptTxt->setSize(buybackPromptTxtPos);

			SDL_Rect closePromptTxtPos = closePromptTxt->getSize();
			closePromptTxtPos.x = bgFrame->getSize().w - 178;
			closePromptTxt->setSize(closePromptTxtPos);
		}

		closePromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuCancel");
		closePromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(closePromptGlyph->path.c_str()) )
		{
			closePromptGlyph->pos.w = imgGet->getWidth();
			closePromptGlyph->pos.h = imgGet->getHeight();
			closePromptGlyph->disabled = false;
		}
		closePromptGlyph->pos.x = closePromptTxt->getSize().x - 4 - closePromptGlyph->pos.w;
		closePromptGlyph->pos.y = closePromptTxt->getSize().y 
			+ closePromptTxt->getSize().h / 2 - closePromptGlyph->pos.h / 2;

		buybackPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuPageRightAlt");
		buybackPromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(buybackPromptGlyph->path.c_str()) )
		{
			buybackPromptGlyph->pos.w = imgGet->getWidth();
			buybackPromptGlyph->pos.h = imgGet->getHeight();
			buybackPromptGlyph->disabled = false;
		}
		buybackPromptGlyph->pos.x = buybackPromptTxt->getSize().x - 4 - buybackPromptGlyph->pos.w;
		buybackPromptGlyph->pos.y = buybackPromptTxt->getSize().y 
			+ buybackPromptTxt->getSize().h / 2 - buybackPromptGlyph->pos.h / 2;
		if ( buybackPromptGlyph->pos.y + buybackPromptGlyph->pos.h >= closePromptGlyph->pos.y )
		{
			// touching below glyph, move this up
			buybackPromptGlyph->pos.y -= 2;
		}
		if ( buybackPromptGlyph->pos.y % 2 == 1 ) // odd numbered pixel, move up
		{
			buybackPromptGlyph->pos.y -= 1;
		}

	}

	if ( itemPrice >= 0 && itemDesc.size() > 1 )
	{
		buyTooltipFrame->setDisabled(false);
		displayItemName->setDisabled(false);
		displayItemValue->setDisabled(false);
		itemTooltipImg->disabled = false;
		itemGoldImg->disabled = false;
		itemSlotFrame->setDisabled(false);

		if ( isInteractable )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (1.0 - animTooltip)) / 2.0;
			animTooltip += setpointDiffX;
			animTooltip = std::min(1.0, animTooltip);
			animTooltipTicks = ticks;
		}

		SDL_Rect buyTooltipFramePos = buyTooltipFrame->getSize();

		SDL_Rect namePos{ 76, 0, 208, 24 };
		displayItemName->setSize(namePos);
		if ( itemUnknownPreventPurchase )
		{
			displayItemName->setColor(hudColors.characterSheetRed);
		}
		else
		{
			displayItemName->setColor(hudColors.characterSheetLightNeutral);
		}
		if ( itemRequiresTitleReflow )
		{
			displayItemName->setText(itemDesc.c_str());
			displayItemName->reflowTextToFit(0);
			if ( displayItemName->getNumTextLines() > 2 )
			{
				// more than 2 lines, append ...
				std::string copiedName = displayItemName->getText();
				auto lastNewline = copiedName.find_last_of('\n');
				copiedName = copiedName.substr(0U, lastNewline);
				copiedName += "...";
				displayItemName->setText(copiedName.c_str());
				displayItemName->reflowTextToFit(0);
				if ( displayItemName->getNumTextLines() > 2 )
				{
					// ... doesn't fit, replace last 3 characters with ...
					copiedName = copiedName.substr(0U, copiedName.size() - 6);
					copiedName += "...";
					displayItemName->setText(copiedName.c_str());
					displayItemName->reflowTextToFit(0);
				}
			}
			itemRequiresTitleReflow = false;
		}
		namePos.h = displayItemName->getNumTextLines() * 24;

		itemBgImg->pos.x = 10;
		if ( displayItemName->getNumTextLines() > 1 )
		{
			itemTooltipImg->path = "images/ui/Shop/Shop_Tooltip_3Row_00.png";
			itemTooltipImg->pos.h = 86;
			buyTooltipFramePos.h = 86;
			itemTooltipImg->pos.y = 0;

			namePos.y = buyTooltipFramePos.h - 77;
		}
		else
		{
			itemTooltipImg->path = "images/ui/Shop/Shop_Tooltip_2Row_00.png";
			itemTooltipImg->pos.h = 66;
			buyTooltipFramePos.h = 66;
			itemTooltipImg->pos.y = 0;

			namePos.y = buyTooltipFramePos.h - 57;
		}
		displayItemName->setSize(namePos);

		itemBgImg->pos.y = itemTooltipImg->pos.y + itemTooltipImg->pos.h / 2 - itemBgImg->pos.h / 2;
		SDL_Rect slotFramePos = itemSlotFrame->getSize();
		slotFramePos.x = itemBgImg->pos.x + itemBgImg->pos.w / 2 - slotFramePos.w / 2 - 1;
		slotFramePos.y = itemBgImg->pos.y + itemBgImg->pos.h / 2 - slotFramePos.h / 2 - 1;
		itemSlotFrame->setSize(slotFramePos);

		char priceFormat[64];
		if ( itemUnknownPreventPurchase )
		{
			snprintf(priceFormat, sizeof(priceFormat), Language::get(4261)); // ??? gold
		}
		else
		{
			snprintf(priceFormat, sizeof(priceFormat), Language::get(4260), itemPrice);
		}

		SDL_Rect valuePos = namePos;
		valuePos.x = 102;
		valuePos.y = buyTooltipFramePos.h - 31;
		valuePos.h = 24;
		displayItemValue->setSize(valuePos);
		displayItemValue->setText(priceFormat);

		displayItemValue->setColor(hudColors.characterSheetLightNeutral);
		if ( !itemUnknownPreventPurchase )
		{
			if ( itemPrice > 0 && itemPrice > stats[player.playernum]->GOLD )
			{
				if ( !strcmp(buyPromptText->getText(), Language::get(4113)) ) // buy prompt
				{
					displayItemValue->setColor(hudColors.characterSheetRed);
				}
			}
		}

		itemGoldImg->pos.x = 76;
		itemGoldImg->pos.y = buyTooltipFramePos.h - 36;

		if ( player.bUseCompactGUIWidth() && player.GUI.activeModule != Player::GUI_t::MODULE_SHOP )
		{
			if ( flipped )
			{
				buyTooltipFramePos.x = shopFrame->getSize().w - (78 + buyTooltipFramePos.w) * animTooltip;
			}
			else
			{
				buyTooltipFramePos.x = -buyTooltipFramePos.w + (86 + buyTooltipFramePos.w) * animTooltip;
			}
			buyTooltipFramePos.y = 8;
		}
		else
		{
			if ( flipped )
			{
				buyTooltipFramePos.x = shopFrame->getSize().w - buyTooltipFramePos.w + 2;
			}
			else
			{
				buyTooltipFramePos.x = 4;
			}
			buyTooltipFramePos.y = bgFrame->getSize().h - (buyTooltipFramePos.h + 4) * animTooltip;
		}
		buyTooltipFrame->setSize(buyTooltipFramePos);

		buyPromptText->setSize(SDL_Rect{ buyTooltipFramePos.w - 60, valuePos.y, 52, 24 });
		buyPromptText->setDisabled(false);

		if ( inputs.hasController(player.playernum) && !inputs.getVirtualMouse(player.playernum)->draw_cursor )
		{
			buyPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuConfirm");
		}
		else
		{
			buyPromptGlyph->path = Input::inputs[player.playernum].getGlyphPathForBinding("MenuRightClick");
		}
		buyPromptGlyph->disabled = true;
		if ( auto imgGet = Image::get(buyPromptGlyph->path.c_str()) )
		{
			buyPromptGlyph->pos.w = imgGet->getWidth();
			buyPromptGlyph->pos.h = imgGet->getHeight();
			buyPromptGlyph->disabled = false;
		}
		buyPromptGlyphFrame->setDisabled(buyPromptGlyph->disabled);
		buyPromptGlyph->pos.x = buyPromptText->getSize().x - 8 - buyPromptGlyph->pos.w;
		buyPromptGlyph->pos.y = buyPromptText->getSize().y + buyPromptText->getSize().h / 2 - buyPromptGlyph->pos.h / 2;
		if ( buyPromptGlyph->pos.y % 2 == 1 )
		{
			buyPromptGlyph->pos.y -= 1;
		}
		buyPromptGlyphFrame->setSize(buyPromptGlyph->pos);
		buyPromptGlyph->pos.x = 0;
		buyPromptGlyph->pos.y = 0;

		/*SDL_Rect buyPromptGlyphFramePos = buyPromptGlyphFrame->getSize();
		int maxPromptHeight = buyTooltipFramePos.h - 6; // if we want to prevent glyph spilling over too much on border
		if ( buyPromptGlyphFramePos.y + buyPromptGlyphFramePos.h > (maxPromptHeight) )
		{
			buyPromptGlyphFramePos.h -= (buyPromptGlyphFramePos.y + buyPromptGlyphFramePos.h) - (maxPromptHeight);
			buyPromptGlyphFrame->setSize(buyPromptGlyphFramePos);
		}*/
	}
	else
	{
		if ( ticks - animTooltipTicks > TICKS_PER_SECOND / 3
			|| usingGamepad
			|| animTooltip < 0.9999 )
		{
			const real_t fpsScale = getFPSScale(50.0); // ported from 50Hz
			real_t setpointDiffX = fpsScale * std::max(.01, (animTooltip)) / 2.0;
			animTooltip -= setpointDiffX;
			animTooltip = std::max(0.0, animTooltip);
		}

		SDL_Rect buyTooltipFramePos = buyTooltipFrame->getSize();
		if ( player.bUseCompactGUIWidth() && player.GUI.activeModule != Player::GUI_t::MODULE_SHOP )
		{
			if ( flipped )
			{
				buyTooltipFramePos.x = shopFrame->getSize().w - (78 + buyTooltipFramePos.w) * animTooltip;
			}
			else
			{
				buyTooltipFramePos.x = -buyTooltipFramePos.w + (86 + buyTooltipFramePos.w) * animTooltip;
			}
			buyTooltipFramePos.y = 8;
		}
		else
		{
			if ( flipped )
			{
				buyTooltipFramePos.x = shopFrame->getSize().w - buyTooltipFramePos.w + 2;
			}
			else
			{
				buyTooltipFramePos.x = 4;
			}
			buyTooltipFramePos.y = bgFrame->getSize().h - (buyTooltipFramePos.h + 4) * animTooltip;
		}
		buyTooltipFrame->setSize(buyTooltipFramePos);
	}
	auto orbImg = buyTooltipFrame->findImage("orb img");
	orbImg->pos.y = buyTooltipFrame->getSize().h - 28;

	// discount values
	auto discountFrame = bgFrame->findFrame("discount frame");
	auto discountLabelText = bgFrame->findField("discount label");
	auto discountValue = discountFrame->findField("discount");
	real_t shopModifier = 0.0;

	if ( flipped )
	{
		SDL_Rect discountPos = discountFrame->getSize();
		discountPos.x = 12;
		discountFrame->setSize(discountPos);

		SDL_Rect discountLabelTextPos = discountLabelText->getSize();
		discountLabelTextPos.x = 116;
		discountLabelText->setSize(discountLabelTextPos);
		discountLabelText->setHJustify(Field::justify_t::LEFT);
	}
	else
	{
		SDL_Rect discountPos = discountFrame->getSize();
		discountPos.x = bgFrame->getSize().w - 112;
		discountFrame->setSize(discountPos);

		SDL_Rect discountLabelTextPos = discountLabelText->getSize();
		discountLabelTextPos.x = bgFrame->getSize().w - 106 - 180 - 12;
		discountLabelText->setSize(discountLabelTextPos);
		discountLabelText->setHJustify(Field::justify_t::RIGHT);
	}
	discountFrame->setDisabled(false);
	discountLabelText->setDisabled(false);
	//discountValue->setDisabled(true);
	if ( animTooltip > 0.0001 )
	{
		discountValue->setDisabled(false);
	}

	if ( (itemPrice >= 0 && itemDesc.size() > 1) || !strcmp(discountValue->getText(), "") )
	{
		if ( player.GUI.activeModule == Player::GUI_t::MODULE_SHOP ) // buy prompt
		{
			std::string s = Language::get(4122);
			if ( flipped )
			{
				size_t found = s.find(':'); // remove colon as text to right of box
				if ( found != std::string::npos )
				{
					s.erase(found);
				}
			}
			discountLabelText->setText(s.c_str());
			shopModifier = 1 / ((50 + stats[player.playernum]->getModifiedProficiency(PRO_TRADING)) / 150.f); // buy value
			//shopModifier /= 1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f;
			shopModifier = std::max(1.0, shopModifier);
			shopModifier = shopModifier * 100.0 - 100.0;
			char buf[32];
			snprintf(buf, sizeof(buf), "%+.f%%", shopModifier);
			discountValue->setText(buf);
		}
		else
		{
			std::string s = Language::get(4123);
			if ( flipped )
			{
				size_t found = s.find(':'); // remove colon as text to right of box
				if ( found != std::string::npos )
				{
					s.erase(found);
				}
			}
			discountLabelText->setText(s.c_str());
			shopModifier = (50 + stats[player.playernum]->getModifiedProficiency(PRO_TRADING)) / 150.f; // sell value
			shopModifier *= 1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f;
			shopModifier = std::min(1.0, shopModifier);
			shopModifier = shopModifier * 100.0 - 100.0;
			char buf[32];
			if ( shopModifier < 0.0 )
			{
				snprintf(buf, sizeof(buf), "%+.f%%", shopModifier);
			}
			else
			{
				snprintf(buf, sizeof(buf), "%.f%%", shopModifier);
			}
			discountValue->setText(buf);
		}
	}
	updatePlayerGold(player.playernum, flipped);
	updateShopGUIChatter(player.playernum, flipped);
}

const bool Player::ShopGUI_t::isItemFromShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->node && item->node->list && shopInv[player.playernum] && item->node->list == shopInv[player.playernum] )
	{
		return true;
	}
	return false;
}

const bool Player::ShopGUI_t::isItemSelectedFromShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->type == SPELL_ITEM )
	{
		return false;
	}

	if ( item->x == getSelectedShopX()
		&& item->y == getSelectedShopY()
		&& isInteractable
		&& isItemFromShop(item) )
	{
		return true;
	}
	return false;
}

const bool Player::ShopGUI_t::isItemSelectedToSellToShop(Item* item) const
{
	if ( !item || !bOpen )
	{
		return false;
	}

	Entity* shopkeeperEntity = uidToEntity(shopkeeper[player.playernum]);
	if ( !shopkeeperEntity )
	{
		return false;
	}

	if ( item->type == SPELL_ITEM )
	{
		return false;
	}

	if ( !isItemFromShop(item) && player.gui_mode == GUI_MODE_SHOP )
	{
		return true;
	}
	return false;
}
