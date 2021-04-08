//! @file GameUI.hpp

#pragma once

#include "Frame.hpp"

void newIngameHud();
void newPlayerInventory(const int player);
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr);
void resetInventorySlotFrames(const int player);

// if true, use the new user interface
extern bool newui;