//! @file GameUI.hpp

#pragma once

void doFrames();
#include "Frame.hpp"

void newIngameHud();
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr);
void createInventoryTooltipFrame(const int player);
bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy, bool spells);
void resetInventorySlotFrames(const int player);
void createPlayerInventorySlotFrameElements(Frame* slotFrame);
void drawCharacterPreview(const int player, SDL_Rect pos, int fov, real_t offsetyaw);
void loadHUDSettingsJSON();
SDL_Surface* blitEnemyBar(const int player, SDL_Surface* statusEffectSprite);
struct EnemyBarSettings_t
{
	std::unordered_map<std::string, float> heightOffsets;
	std::unordered_map<std::string, float> screenDistanceOffsets;
	std::string getEnemyBarSpriteName(Entity* entity);
	float getHeightOffset(Entity* entity)
	{
		if ( !entity ) { return 0.f; }
		return heightOffsets[getEnemyBarSpriteName(entity)];
	}
	float getScreenDistanceOffset(Entity* entity)
	{
		if ( !entity ) { return 0.f; }
		return screenDistanceOffsets[getEnemyBarSpriteName(entity)];
	}
};
extern EnemyBarSettings_t enemyBarSettings;

// if true, use the new user interface
extern bool newui;
extern bool bUsePreciseFieldTextReflow;
extern bool bUseSelectedSlotCycleAnimation;

void openMinimap(int player);
