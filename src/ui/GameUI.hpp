//! @file GameUI.hpp

#pragma once

void doFrames();
#include "Frame.hpp"

void newIngameHud();
void doNewCharacterSheet(int player);
void newPlayerInventory(const int player);
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr);
void resetInventorySlotFrames(const int player);

// if true, use the new user interface
extern bool newui;