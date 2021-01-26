// GameUI.cpp

#include "GameUI.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"

#include "../main.hpp"
#include "../game.hpp"
#include "../menu.hpp"
#include "../interface/interface.hpp"

#include <assert.h>

static Frame* playerHud[MAXPLAYERS] = { nullptr };
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

    static const char* bigfont = "fonts/pixelmix.ttf#18";
    static const char* smallfont = "fonts/pixel_maz.ttf#32";

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