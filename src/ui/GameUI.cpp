// GameUI.cpp

#include "GameUI.hpp"
#include "Frame.hpp"
#include "Image.hpp"

static Frame* playerHud = nullptr;

void newIngameHud() {
    if (!playerHud) {
        Frame* frame = gui->addFrame("player hud");
        playerHud = frame;

        frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
        frame->setActualSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
        frame->setHollow(true);
        frame->setBorder(0);

        static int num_hotbar_slots = 10;
        for (int c = 0; c < num_hotbar_slots; ++c) {
            Image* img = Image::get("images/system/hotbar_slot.png");
            if (img) {
                int w = img->getWidth();
                int h = img->getHeight();
                int x = Frame::virtualScreenX / 2 - w * num_hotbar_slots / 2 + w * c;
                int y = Frame::virtualScreenY - h - 10;
                frame->addImage(
                    SDL_Rect{x, y, w, h},
                    0xffffffff,
                    "images/system/hotbar_slot.png",
                    "hotbarslot_img"
                );
            }
        }
    }
}