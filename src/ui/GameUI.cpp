// GameUI.cpp

#include "GameUI.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../menu.hpp"
#include "../interface/interface.hpp"
#include "../stat.hpp"
#include "../player.hpp"
#include "../draw.hpp"
#include "../items.hpp"

#include <assert.h>

static Frame* playerHud[MAXPLAYERS] = { nullptr };
static Frame* playerInventory[MAXPLAYERS] = { nullptr };
bool newui = false;

void createIngameHud(int player) {
    char name[32];
    snprintf(name, sizeof(name), "player hud %d", player);
    Frame* frame = gui->addFrame(name);

    playerHud[player] = frame;
    int playercount = 0;
    if (multiplayer == SINGLE) {
        for (int c = 0; c < MAXPLAYERS; ++c) {
            if (!client_disconnected[c]) {
                ++playercount;
            }
        }
    } else {
        playercount = 1;
    }

#ifdef NINTENDO
    static const char* bigfont = "rom://fonts/pixelmix.ttf#18";
    static const char* smallfont = "rom://fonts/pixel_maz.ttf#32";
#else
    static const char* bigfont = "fonts/pixelmix.ttf#18";
    static const char* smallfont = "fonts/pixel_maz.ttf#32";
#endif // NINTENDO

    // big empty frame to serve as the root
    if (playercount == 1) {
        frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
    } else if (playercount == 2) {
        int y = (player % 2) * Frame::virtualScreenY / 2;
        frame->setSize(SDL_Rect{0, y, Frame::virtualScreenX, Frame::virtualScreenY / 2});
    } else if (playercount >= 3 || playercount <= 4) {
        int x = (player % 2) * Frame::virtualScreenX / 2;
        int y = (player / 2) * Frame::virtualScreenY / 2;
        frame->setSize(SDL_Rect{x, y, Frame::virtualScreenX / 2, Frame::virtualScreenY / 2});
    }
    frame->setActualSize(SDL_Rect{0, 0, frame->getSize().w, frame->getSize().h});
    frame->setHollow(true);
    frame->setBorder(0);

    // chat
    Frame* chat = frame->addFrame("chat");
    chat->setSize(SDL_Rect{224, 16, 832, 200});
    chat->setActualSize(SDL_Rect{0, 0, 832, 200});
    chat->setFont(bigfont);
    {
        auto e = chat->addEntry("chat", true);
        e->color = 0xff00ffff;
        e->text = "Player 3: Chatlog text is so great what do you think?";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "You have been hit by a dog.";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "Yowza, dog hits smart!";
    }
    {
        auto e = chat->addEntry("message", true);
        e->color = 0xffffffff;
        e->text = "The dog hit smarts.";
    }
    {
        auto e = chat->addEntry("chat", true);
        e->color = 0xffff8800;
        e->text = "Player 2: Chatlog text is the best, I swear my life on it.";
    }

    // other players' statuses
    for (int i = 0, c = 0; c < MAXPLAYERS; ++c) {
        if (c == player) {
            continue;
        }

        // player name
        {
            char name[32];
            snprintf(name, sizeof(name), "Player %d Name", c + 1);
            Field* field = frame->addField(name, sizeof(name));
            Uint32 color = 0xffffffff;
            switch (c) {
            case 0: color = 0xffffffff; break;
            case 1: color = 0xffff8800; break;
            case 2: color = 0xff00ffff; break;
            case 3: color = 0xffff00ff; break;
            }
            field->setSize(SDL_Rect{16, 8 + 56 * i, 320, 64});
            field->setText(name);
            field->setFont(smallfont);
            field->setColor(color);
        }

        // hp bar
        {
            char name[32];
            snprintf(name, sizeof(name), "hp %d", c + 1);
            Frame* hp = frame->addFrame(name);
            hp->setColor(0xffffffff);
            hp->setBorderStyle(Frame::BORDER_BEVEL_HIGH);
            hp->setBorder(2);
            hp->setSize(SDL_Rect{16, 32 + 56 * i, 160, 16});
            hp->setActualSize(SDL_Rect{0, 0, hp->getSize().w, hp->getSize().h});
            auto red = hp->addImage(
                SDL_Rect{4, 4, hp->getSize().w - 8, hp->getSize().h - 8},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
        }

        // mp bar
        {
            char name[32];
            snprintf(name, sizeof(name), "mp %d", c + 1);
            Frame* mp = frame->addFrame(name);
            mp->setColor(0xffffffff);
            mp->setBorderStyle(Frame::BORDER_BEVEL_HIGH);
            mp->setBorder(2);
            mp->setSize(SDL_Rect{16, 48 + 56 * i, 160, 16});
            mp->setActualSize(SDL_Rect{0, 0, mp->getSize().w, mp->getSize().h});
            auto blue = mp->addImage(
                SDL_Rect{4, 4, mp->getSize().w - 8, mp->getSize().h - 8},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
        }

        // position
        ++i;
    }

    // ally hp
    for (int c = 0; c < 8; ++c) {
        // text
        {
            char text[32];
            snprintf(text, sizeof(text), "Ally %d", c + 1);
            Field* field = frame->addField(text, sizeof(text));
            field->setText(text);
            field->setSize(SDL_Rect{frame->getSize().w - 176, 8 + 18 * c, 64, 24});
            field->setFont(smallfont);
            field->setColor(0xff888888);
        }

        // bar
        {
            char name[32];
            snprintf(name, sizeof(name), "ally hp bar %d", c + 1);
            Frame* bar = frame->addFrame(name);
            bar->setSize(SDL_Rect{frame->getSize().w - 128, 14 + 18 * c, 112, 14});
            bar->setActualSize(SDL_Rect{0, 0, bar->getSize().w, bar->getSize().h});
            bar->setColor(0xffaaaaaa);
            auto red = bar->addImage(
                SDL_Rect{2, 2, bar->getSize().w - 4, bar->getSize().h - 4},
                0xff4444dd,
                "images/system/white.png",
                "red"
            );
        }
    }

    // hotbar
    static int num_hotbar_slots = 10;
    for (int c = 0; c < num_hotbar_slots; ++c) {
        Image* img = Image::get("images/system/hotbar_slot.png");
        Uint32 color = 0xffffffff;
        if (c == 2) {
            // selected one is highlighted yellow
            color = 0xff00ffff;
        }
        if (img) {
            int w = img->getWidth();
            int h = img->getHeight();
            int x = frame->getSize().w / 2 - w * num_hotbar_slots / 2 + w * c;
            int y = frame->getSize().h - h - 32;
            frame->addImage(
                SDL_Rect{x, y, w, h},
                color,
                "images/system/hotbar_slot.png",
                "hotbarslot"
            );
        }
    }

    // selected spell
    Image* spellbox_img = Image::get("images/system/hotbar_slot.png");
    if (spellbox_img) {
        int w = spellbox_img->getWidth();
        int h = spellbox_img->getHeight();
        int x = frame->getSize().w / 2 + w * num_hotbar_slots / 2 + 16;
        int y = frame->getSize().h - h - 32;
        frame->addImage(
            SDL_Rect{x, y, w, h},
            0xffffffff,
            "images/system/hotbar_slot.png",
            "spellbox"
        );
    }

    // xp bar
    if (spellbox_img) {
        int w = spellbox_img->getWidth();
        int h = spellbox_img->getHeight();

        Frame* xpbar = frame->addFrame("xpbar");
        xpbar->setSize(SDL_Rect{
            frame->getSize().w / 2 - w * num_hotbar_slots / 2,
            frame->getSize().h - 32,
            w * num_hotbar_slots,
            24
            }
        );
        xpbar->setActualSize(SDL_Rect{
            0,
            0,
            w * num_hotbar_slots,
            24
            }
        );
        xpbar->setColor(0xffaaaaaa);
        {
            auto progress = xpbar->addImage(
                SDL_Rect{2, 2, (xpbar->getSize().w - 4) / 4, xpbar->getSize().h - 4},
                0xffffffff,
                "images/system/white.png"
            );

            Field* text = xpbar->addField("text", 64);
            text->setSize(SDL_Rect{2, 4, xpbar->getSize().w - 4, xpbar->getSize().h - 4});
            text->setText("22 / 100 XP");
            text->setFont(smallfont);
            text->setColor(0xff888888);
            text->setHJustify(Field::justify_t::CENTER);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // hunger icon
    auto hunger = frame->addImage(
        SDL_Rect{16, frame->getSize().h - 160, 64, 64},
        0xffffffff,
        "images/system/Hunger.png",
        "hunger"
    );

    // hp
    {
        Frame* hp = frame->addFrame("hp");
        hp->setColor(0xffffffff);
        hp->setSize(SDL_Rect{16, frame->getSize().h - 96, 40, 24});
        hp->setActualSize(SDL_Rect{0, 0, 40, 24});
        {
            auto red = hp->addImage(
                SDL_Rect{4, 4, 32, 16},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
        }
        Frame* hp_bar = frame->addFrame("hp_bar");
        hp_bar->setColor(0xffffffff);
        hp_bar->setSize(SDL_Rect{56, frame->getSize().h - 96, 256, 24});
        hp_bar->setActualSize(SDL_Rect{0, 0, 256, 24});
        {
            auto red = hp_bar->addImage(
                SDL_Rect{4, 4, hp_bar->getSize().w - 8, 16},
                0xff8888ff,
                "images/system/white.png",
                "red"
            );
            Field* text = hp_bar->addField("text", 64);
            text->setSize(SDL_Rect{6, 3, 64, hp_bar->getSize().h - 4});
            text->setText("HP");
            text->setFont(smallfont);
            text->setColor(0xffffffff);
            text->setHJustify(Field::justify_t::LEFT);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // mp
    {
        Frame* mp = frame->addFrame("mp");
        mp->setColor(0xffffffff);
        mp->setSize(SDL_Rect{16, frame->getSize().h - 68, 40, 24});
        mp->setActualSize(SDL_Rect{0, 0, 40, 24});
        {
            auto blue = mp->addImage(
                SDL_Rect{4, 4, 32, 16},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
        }
        Frame* mp_bar = frame->addFrame("mp_bar");
        mp_bar->setColor(0xffffffff);
        mp_bar->setSize(SDL_Rect{56, frame->getSize().h - 68, 64, 24});
        mp_bar->setActualSize(SDL_Rect{0, 0, 64, 24});
        {
            auto blue = mp_bar->addImage(
                SDL_Rect{4, 4, mp_bar->getSize().w - 8, 16},
                0xffff8888,
                "images/system/white.png",
                "blue"
            );
            Field* text = mp_bar->addField("text", 64);
            text->setSize(SDL_Rect{6, 3, 64, mp_bar->getSize().h - 4});
            text->setText("MP");
            text->setFont(smallfont);
            text->setColor(0xffffffff);
            text->setHJustify(Field::justify_t::LEFT);
            text->setVJustify(Field::justify_t::CENTER);
        }
    }

    // status effect
    auto effect = frame->addImage(
        SDL_Rect{16, frame->getSize().h - 40, 32, 32},
        0xffffffff,
        "images/system/drunk.png",
        "status"
    );
}

void newIngameHud() {
    if (!nohud) {
        // here is where splitscreen
        if (!playerHud[clientnum]) {
            createIngameHud(clientnum);
        }

        // original minimap already works fine, so just reuse it
        if (multiplayer == SINGLE) {
            for (int c = 0; c < MAXPLAYERS; ++c) {
                if (!client_disconnected[c]) {
                    drawMinimap(c);
                }
            }
        } else {
            drawMinimap(0);
        }
    }
}

void createPlayerInventorySlotFrameElements(Frame* slotFrame)
{
	const SDL_Rect slotSize = SDL_Rect{ 0, 0, slotFrame->getSize().w, slotFrame->getSize().h };
	SDL_Rect coloredBackgroundPos = SDL_Rect{ slotSize.x + 2, slotSize.y + 2, slotSize.w - 2, slotSize.h - 2 };

	auto beatitudeFrame = slotFrame->addFrame("beatitude status frame"); // covers unidentified status as well
	beatitudeFrame->setSize(slotSize);
	beatitudeFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	beatitudeFrame->setHollow(true);
	beatitudeFrame->setInvisible(true);
	beatitudeFrame->setDisabled(true);
	beatitudeFrame->addImage(coloredBackgroundPos, 0xFFFFFFFF, "images/system/white.png", "beatitude status bg");

	auto brokenStatusFrame = slotFrame->addFrame("broken status frame");
	brokenStatusFrame->setSize(slotSize);
	brokenStatusFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	brokenStatusFrame->setHollow(true);
	brokenStatusFrame->setInvisible(true);
	brokenStatusFrame->setDisabled(true);
	brokenStatusFrame->addImage(coloredBackgroundPos, SDL_MapRGBA(mainsurface->format, 160, 160, 160, 64), "images/system/white.png", "broken status bg");

	auto itemSpriteFrame = slotFrame->addFrame("item sprite frame");
	itemSpriteFrame->setSize(SDL_Rect{ slotSize.x + 3, slotSize.y + 3, slotSize.w - 3, slotSize.h - 3 });
	itemSpriteFrame->setActualSize(SDL_Rect{ slotSize.x + 3, slotSize.y + 3, slotSize.w - 3, slotSize.h - 3 });
	itemSpriteFrame->setHollow(true);
	itemSpriteFrame->setInvisible(true);
	itemSpriteFrame->setDisabled(true);
	itemSpriteFrame->addImage(slotSize, 0xFFFFFFFF, "images/system/white.png", "item sprite img");

	auto unusableFrame = slotFrame->addFrame("unusable item frame");
	unusableFrame->setSize(slotSize);
	unusableFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	unusableFrame->setHollow(true);
	unusableFrame->setInvisible(true);
	unusableFrame->setDisabled(true);
	unusableFrame->addImage(coloredBackgroundPos, SDL_MapRGBA(mainsurface->format, 64, 64, 64, 144), "images/system/white.png", "unusable item bg");

	static const char* smallfont = "fonts/pixel_maz.ttf#32";

	auto quantityFrame = slotFrame->addFrame("quantity frame");
	quantityFrame->setSize(slotSize);
	quantityFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	quantityFrame->setHollow(true);
	quantityFrame->setInvisible(true);
	Field* qtyText = quantityFrame->addField("quantity text", 32);
	qtyText->setFont(smallfont);
	qtyText->setColor(0xffffffff);
	qtyText->setHJustify(Field::justify_t::RIGHT);
	qtyText->setVJustify(Field::justify_t::BOTTOM);
	qtyText->setText("10");
	qtyText->setSize(SDL_Rect{ 4, 8, quantityFrame->getSize().w, quantityFrame->getSize().h });

	auto equippedIconFrame = slotFrame->addFrame("equipped icon frame");
	equippedIconFrame->setSize(slotSize);
	equippedIconFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	equippedIconFrame->setHollow(true);
	equippedIconFrame->setInvisible(true);
	SDL_Rect equippedImgPos = { 3, slotSize.h - 17, 16, 16 };
	equippedIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Equipped.png", "equipped icon img");

	auto brokenIconFrame = slotFrame->addFrame("broken icon frame");
	brokenIconFrame->setSize(slotSize);
	brokenIconFrame->setActualSize(SDL_Rect{ 0, 0, slotSize.w, slotSize.h });
	brokenIconFrame->setHollow(true);
	brokenIconFrame->setInvisible(true);
	brokenIconFrame->addImage(equippedImgPos, 0xFFFFFFFF, "images/system/Broken.png", "broken icon img");
}

void resetInventorySlotFrames(const int player)
{
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	if ( Frame* inventoryFrame = gui->findFrame(name) )
	{
		if ( Frame* inventorySlotsFrame = inventoryFrame->findFrame("inventory slots") )
		{
			for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
			{
				for ( int y = 0; y < players[player]->inventoryUI.getSizeY(); ++y )
				{
					char slotname[32] = "";
					snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);
					auto slotFrame = inventorySlotsFrame->findFrame(slotname);
					if ( slotFrame )
					{
						slotFrame->setDisabled(true);
					}
				}
			}
		}
		if ( Frame* paperDollSlotsFrame = inventoryFrame->findFrame("paperdoll slots") )
		{
			for ( int x = Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_LEFT; x <= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT; ++x )
			{
				for ( int y = Player::Inventory_t::PaperDollRows::DOLL_ROW_1; y <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5; ++y )
				{
					char slotname[32] = "";
					snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);
					auto slotFrame = paperDollSlotsFrame->findFrame(slotname);
					if ( slotFrame )
					{
						slotFrame->setDisabled(true);
					}
				}
			}
		}
	}
}

bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy)
{
	if ( !gui )
	{
		return false;
	}
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	if ( Frame* inventoryFrame = gui->findFrame(name) )
	{
		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			for ( int y = Player::Inventory_t::DOLL_ROW_1; y < players[player]->inventoryUI.getSizeY(); ++y )
			{
				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);
				auto slotFrame = inventoryFrame->findFrame(slotname);
				if ( !slotFrame )
				{
					continue;
				}

				if ( slotFrame->capturesMouseInRealtimeCoords() )
				{
					outx = x;
					outy = y;
					return true;
				}
			}
		}
	}
	return false;
}

void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr)
{
	if ( !itemPtr || !slotFrame )
	{
		return;
	}

	Item* item = (Item*)itemPtr;

	int player = slotFrame->getOwner();

	slotFrame->setDisabled(false);

	auto spriteImageFrame = slotFrame->findFrame("item sprite frame");
	auto spriteImage = spriteImageFrame->findImage("item sprite img");
	node_t* imagePathsNode = list_Node(&items[item->type].images, item->appearance % items[item->type].variations);
	if ( imagePathsNode )
	{
		string_t* imagePath = static_cast<string_t*>(imagePathsNode->element);
		spriteImage->path = imagePath->data;
		spriteImageFrame->setDisabled(false);
	}

	if ( auto qtyFrame = slotFrame->findFrame("quantity frame") )
	{
		qtyFrame->setDisabled(true);
		if ( item->count > 1 )
		{
			qtyFrame->setDisabled(false);
			if ( auto qtyText = qtyFrame->findField("quantity text") )
			{
				char qtybuf[32] = "";
				snprintf(qtybuf, sizeof(qtybuf), "%d", item->count);
				qtyText->setText(qtybuf);
			}
		}
	}

	bool disableBackgrounds = false;
	if ( !strcmp(slotFrame->getName(), "dragging inventory item") ) // dragging item, no need for colors
	{
		disableBackgrounds = true;
	}
	
	if ( auto beatitudeFrame = slotFrame->findFrame("beatitude status frame") )
	{
		beatitudeFrame->setDisabled(true);
		if ( !disableBackgrounds )
		{
			if ( auto beatitudeImg = beatitudeFrame->findImage("beatitude status bg") )
			{
				if ( !item->identified )
				{
					beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 128, 128, 0, 125);
					beatitudeFrame->setDisabled(false);
				}
				else if ( item->beatitude < 0 )
				{
					beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 128, 0, 0, 125);
					beatitudeFrame->setDisabled(false);
				}
				else if ( item->beatitude > 0 )
				{
					if ( colorblind )
					{
						beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 100, 245, 255, 65);
					}
					else
					{
						beatitudeImg->color = SDL_MapRGBA(mainsurface->format, 0, 255, 0, 65);
					}
					beatitudeFrame->setDisabled(false);
				}
			}
		}
	}

	if ( auto brokenStatusFrame = slotFrame->findFrame("broken status frame") )
	{
		brokenStatusFrame->setDisabled(true);
		if ( item->status == BROKEN )
		{
			brokenStatusFrame->setDisabled(false);
		}
	}

	if ( auto unusableFrame = slotFrame->findFrame("unusable item frame") )
	{
		bool greyedOut = false;
		unusableFrame->setDisabled(true);

		if ( !disableBackgrounds )
		{
			if ( players[player] && players[player]->entity && players[player]->entity->effectShapeshift != NOTHING )
			{
				// shape shifted, disable some items
				if ( !item->usableWhileShapeshifted(stats[player]) )
				{
					greyedOut = true;
				}
			}
			if ( !greyedOut && client_classes[player] == CLASS_SHAMAN
				&& item->type == SPELL_ITEM && !(playerUnlockedShamanSpell(player, item)) )
			{
				greyedOut = true;
			}

			if ( greyedOut )
			{
				unusableFrame->setDisabled(false);
			}
		}
	}

	bool equipped = false;
	bool broken = false;
	if ( itemCategory(item) != SPELL_CAT )
	{
		if ( itemIsEquipped(item, player) )
		{
			equipped = true;
		}
		else if ( item->status == BROKEN )
		{
			broken = true;
		}
	}
	else
	{
		spell_t* spell = getSpellFromItem(player, item);
		if ( players[player]->magic.selectedSpell() == spell
			&& (players[player]->magic.selected_spell_last_appearance == item->appearance || players[player]->magic.selected_spell_last_appearance == -1) )
		{
			equipped = true;
		}
	}

	if ( auto equippedIconFrame = slotFrame->findFrame("equipped icon frame") )
	{
		equippedIconFrame->setDisabled(true);
		if ( equipped )
		{
			equippedIconFrame->setDisabled(false);
		}
	}
	if ( auto brokenIconFrame = slotFrame->findFrame("broken icon frame") )
	{
		brokenIconFrame->setDisabled(true);
		if ( broken )
		{
			brokenIconFrame->setDisabled(false);
		}
	}
}

void createInventoryTooltipFrame(const int player)
{
	if ( !gui )
	{
		return;
	}

	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	if ( Frame* inventoryFrame = gui->findFrame(name) )
	{
		if ( inventoryFrame->findFrame("inventory mouse tooltip") )
		{
			return;
		}

		auto tooltipFrame = inventoryFrame->addFrame("inventory mouse tooltip");
		tooltipFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipFrame->setActualSize(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, tooltipFrame->getSize().h });
		tooltipFrame->setDisabled(true);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255);
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 },
			color, "images/system/white.png", "tooltip temp background");

		tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 28 },
			color, "images/system/inventory/tooltips/Hover_T00.png", "tooltip top background");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
			color, "images/system/inventory/tooltips/Hover_TL00.png", "tooltip top left");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 28 },
			color, "images/system/inventory/tooltips/Hover_TR00.png", "tooltip top right");

		tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 52 },
			color, "images/system/inventory/tooltips/Hover_C00.png", "tooltip middle background");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
			color, "images/system/inventory/tooltips/Hover_L00.png", "tooltip middle left");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 52 },
			color, "images/system/inventory/tooltips/Hover_R00.png", "tooltip middle right");

		tooltipFrame->addImage(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, 26 },
			color, "images/system/inventory/tooltips/Hover_B00.png", "tooltip bottom background");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
			color, "images/system/inventory/tooltips/Hover_BL00.png", "tooltip bottom left");
		tooltipFrame->addImage(SDL_Rect{ 0, 0, 16, 26 },
			color, "images/system/inventory/tooltips/Hover_BR00.png", "tooltip bottom right");

		auto tooltipTextField = tooltipFrame->addField("inventory mouse tooltip header", 1024);
		tooltipTextField->setText("Nothing");
		tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
		tooltipTextField->setFont("fonts/pixelmix.ttf#14");

		if ( auto attrFrame = tooltipFrame->addFrame("inventory mouse tooltip attributes frame") )
		{
			attrFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

			attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
				0xFFFFFFFF, "images/system/str32.png", "inventory mouse tooltip primary image");
			tooltipTextField = attrFrame->addField("inventory mouse tooltip primary value", 256);
			tooltipTextField->setText("Nothing");
			tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
			tooltipTextField->setFont("fonts/pixelmix.ttf#12");

			attrFrame->addImage(SDL_Rect{ 0, 0, 24, 24 },
				0xFFFFFFFF, "images/system/con32.png", "inventory mouse tooltip secondary image");
			tooltipTextField = attrFrame->addField("inventory mouse tooltip secondary value", 256);
			tooltipTextField->setText("Nothing");
			tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
			tooltipTextField->setFont("fonts/pixelmix.ttf#12");
		}
		if ( auto descFrame = tooltipFrame->addFrame("inventory mouse tooltip description frame") )
		{
			descFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

			tooltipTextField = descFrame->addField("inventory mouse tooltip description", 1024);
			tooltipTextField->setText("Nothing");
			tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
			tooltipTextField->setFont("fonts/pixelmix.ttf#12");
		}
		if ( auto promptFrame = tooltipFrame->addFrame("inventory mouse tooltip prompt frame") )
		{
			promptFrame->setSize(SDL_Rect{ 0, 0, 0, 0 });

			tooltipTextField = promptFrame->addField("inventory mouse tooltip prompt", 1024);
			tooltipTextField->setText("Nothing");
			tooltipTextField->setSize(SDL_Rect{ 0, 0, 0, 0 });
			tooltipTextField->setFont("fonts/pixelmix.ttf#12");
		}
	}
}

void createPlayerInventory(const int player)
{
	char name[32];
	snprintf(name, sizeof(name), "player inventory %d", player);
	Frame* frame = gui->addFrame(name);
	playerInventory[player] = frame;
	frame->setSize(SDL_Rect{ players[player]->camera_x1(),
		players[player]->camera_y1(),
		players[player]->camera_width(),
		players[player]->camera_height() });
	frame->setActualSize(frame->getSize());
	frame->setHollow(true);
	frame->setBorder(0);
	frame->setOwner(player);

	createInventoryTooltipFrame(player);

	SDL_Rect basePos{ 0, 0, 105 * 2, 224 * 2 };
	{
		auto bgFrame = frame->addFrame("inventory base");
		bgFrame->setSize(basePos);
		const auto bgSize = bgFrame->getSize();
		bgFrame->setActualSize(SDL_Rect{ 0, 0, bgSize.w, bgSize.h });
		bgFrame->addImage(SDL_Rect{ 0, 0, bgSize.w, bgSize.h },
			SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255),
			"images/system/inventory/HUD_Inventory_Base_00a.png", "inventory base img");
	}

	const int inventorySlotSize = players[player]->inventoryUI.getSlotSize();

	const int baseSlotOffsetX = 4;
	const int baseSlotOffsetY = 0;
	SDL_Rect invSlotsPos{ 0, 202, basePos.w, 242 };
	{
		const auto invSlotsFrame = frame->addFrame("inventory slots");
		invSlotsFrame->setSize(invSlotsPos);
		invSlotsFrame->setActualSize(SDL_Rect{ 0, 0, invSlotsFrame->getSize().w, invSlotsFrame->getSize().h });

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };

		for ( int x = 0; x < players[player]->inventoryUI.getSizeX(); ++x )
		{
			currentSlotPos.x = baseSlotOffsetX + (x * inventorySlotSize);
			for ( int y = 0; y < players[player]->inventoryUI.getSizeY(); ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + (y * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);

				auto slotFrame = invSlotsFrame->addFrame(slotname);
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);
				slotFrame->setActualSize(SDL_Rect{ 0, 0, slotFrame->getSize().w, slotFrame->getSize().h });
				//slotFrame->setDisabled(true);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}
	}

	{
		SDL_Rect dollSlotsPos{ 0, 0, basePos.w, invSlotsPos.y };
		const auto dollSlotsFrame = frame->addFrame("paperdoll slots");
		dollSlotsFrame->setSize(dollSlotsPos);
		dollSlotsFrame->setActualSize(SDL_Rect{ 0, 0, dollSlotsFrame->getSize().w, dollSlotsFrame->getSize().h });

		SDL_Rect currentSlotPos{ baseSlotOffsetX, baseSlotOffsetY, inventorySlotSize, inventorySlotSize };

		for ( int x = Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_LEFT; x <= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT; ++x )
		{
			currentSlotPos.x = baseSlotOffsetX;
			if ( x == Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT )
			{
				currentSlotPos.x = baseSlotOffsetX + (4 * inventorySlotSize); // 4 slots over
			}

			for ( int y = Player::Inventory_t::PaperDollRows::DOLL_ROW_1; y <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5; ++y )
			{
				currentSlotPos.y = baseSlotOffsetY + ((y - Player::Inventory_t::PaperDollRows::DOLL_ROW_1) * inventorySlotSize);

				char slotname[32] = "";
				snprintf(slotname, sizeof(slotname), "slot %d %d", x, y);

				auto slotFrame = dollSlotsFrame->addFrame(slotname);
				SDL_Rect slotPos{ currentSlotPos.x, currentSlotPos.y, inventorySlotSize, inventorySlotSize };
				slotFrame->setSize(slotPos);
				slotFrame->setActualSize(SDL_Rect{ 0, 0, slotFrame->getSize().w, slotFrame->getSize().h });
				//slotFrame->setDisabled(true);

				createPlayerInventorySlotFrameElements(slotFrame);
			}
		}

		{
			auto charFrame = frame->addFrame("inventory character preview");
			auto charSize = dollSlotsPos;
			charSize.x += inventorySlotSize + baseSlotOffsetX + 4;
			charSize.w -= 2 * (inventorySlotSize + baseSlotOffsetX + 4);

			charFrame->setSize(charSize);
			charFrame->setActualSize(SDL_Rect{ 0, 0, charSize.w, charSize.h });
			//charFrame->addImage(SDL_Rect{ 0, 0, charSize.w, charSize.h },
			//	SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255),
			//	"images/system/white.png", "inventory character preview bg");
		}

		/*auto selectedFrame = dollSlotsFrame->addFrame("paperdoll selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setActualSize(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h });
		selectedFrame->setDisabled(true);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "paperdoll selected highlight");*/
	}

	{
		auto selectedFrame = frame->addFrame("inventory selected item");
		selectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		selectedFrame->setActualSize(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h });
		selectedFrame->setDisabled(true);

		Uint32 color = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
		selectedFrame->addImage(SDL_Rect{ 0, 0, selectedFrame->getSize().w, selectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "inventory selected highlight");

		auto oldSelectedFrame = frame->addFrame("inventory old selected item");
		oldSelectedFrame->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		oldSelectedFrame->setActualSize(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h });
		oldSelectedFrame->setDisabled(true);

		color = SDL_MapRGBA(mainsurface->format, 0, 255, 255, 255);
		oldSelectedFrame->addImage(SDL_Rect{ 0, 0, oldSelectedFrame->getSize().w, oldSelectedFrame->getSize().h },
			color, "images/system/hotbar_slot.png", "inventory old selected highlight");
	}

	{
		auto draggingInventoryItem = frame->addFrame("dragging inventory item");
		draggingInventoryItem->setSize(SDL_Rect{ 0, 0, inventorySlotSize, inventorySlotSize });
		draggingInventoryItem->setActualSize(SDL_Rect{ 0, 0, draggingInventoryItem->getSize().w, draggingInventoryItem->getSize().h });
		draggingInventoryItem->setDisabled(true);
		createPlayerInventorySlotFrameElements(draggingInventoryItem);
	}
}

void newPlayerInventory(const int player)
{
	if ( !playerInventory[player] )
	{
		createPlayerInventory(player);
	}
}

void doNewCharacterSheet(int player)
{
#ifdef NINTENDO
    static const char* bigfont = "rom://fonts/pixelmix.ttf#18";
    static const char* smallfont = "rom://fonts/pixel_maz.ttf#32";
#else // NINTENDO
    static const char* bigfont = "fonts/pixelmix.ttf#18";
    static const char* smallfont = "fonts/pixel_maz.ttf#32";
#endif // NINTENDO

    Frame* frame = gui->findFrame("Character sheet");
    if (!frame) {
        const int w = 200;
        frame = gui->addFrame("Character sheet");
        frame->setSize(SDL_Rect{
            players[player]->camera_x2() - w,
            players[player]->camera_y1(),
            w, players[player]->camera_height()});
        frame->setColor(makeColor(154, 154, 154, 255));

        // map button
        {
            Button* b = frame->addButton("minimap button");
            b->setFont(smallfont);
            b->setText("Open Map");
            b->setSize(SDL_Rect{0, 8, frame->getSize().w, 40});
            b->setBorder(2);
            b->setBorderColor(makeColor(128, 128, 128, 255));
            b->setColor(makeColor(192, 192, 192, 255));
            b->setTextColor(makeColor(255, 255, 255, 255));
        }

        // log button
        {
            Button* b = frame->addButton("log button");
            b->setFont(smallfont);
            b->setText("Open Log");
            b->setSize(SDL_Rect{0, 56, frame->getSize().w, 40});
            b->setBorder(2);
            b->setBorderColor(makeColor(128, 128, 128, 255));
            b->setColor(makeColor(192, 192, 192, 255));
            b->setTextColor(makeColor(255, 255, 255, 255));
        }

        // game timer
        {

        }

        // 
        {
        }
    } else {
        frame->setDisabled(players[player]->shootmode);
    }
}

void doFrames() {
    if ( gui ) 
    {
        Frame::result_t gui_result = gui->process();
        gui->draw();
        if ( !gui_result.usable ) {
            return;
        }
    }
}