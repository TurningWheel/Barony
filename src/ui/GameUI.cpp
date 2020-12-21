// GameUI.cpp

#include "GameUI.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"

#include <assert.h>

static Frame* playerHud = nullptr;

void createIngameHud() {
    Frame* frame = gui->addFrame("player hud");
    playerHud = frame;

    // big empty frame to serve as the root
    frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
    frame->setActualSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
    frame->setHollow(true);
    frame->setBorder(0);

    // chat
    Frame* chat = frame->addFrame("chat");
    chat->setSize(SDL_Rect{224, 16, 832, 200});
    chat->setActualSize(SDL_Rect{0, 0, 832, 200});
    chat->setFont("fonts/pixelmix.ttf#18");
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
            text->setFont("fonts/pixel_maz.ttf#32");
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
            text->setFont("fonts/pixel_maz.ttf#32");
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
            text->setFont("fonts/pixel_maz.ttf#32");
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
    if (!playerHud) {
        createIngameHud();
    }
}