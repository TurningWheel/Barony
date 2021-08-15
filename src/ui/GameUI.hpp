//! @file GameUI.hpp

#pragma once

void doFrames();
#include "Frame.hpp"

void newIngameHud();
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr);
void createInventoryTooltipFrame(const int player);
bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy);
void resetInventorySlotFrames(const int player);
void createPlayerInventorySlotFrameElements(Frame* slotFrame);
void loadHUDSettingsJSON();

// if true, use the new user interface
extern bool newui;
extern bool bUsePreciseFieldTextReflow;
extern bool bUseSelectedSlotCycleAnimation;
