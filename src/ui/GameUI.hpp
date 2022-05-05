//! @file GameUI.hpp

#pragma once

void doFrames();
#include "Frame.hpp"
#include "../interface/consolecommand.hpp"
#include <deque>

void doSharedMinimap();
extern Frame* gameUIFrame[MAXPLAYERS];
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr, bool forceUnusable = false);
void createInventoryTooltipFrame(const int player);
bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy, bool spells);
void resetInventorySlotFrames(const int player);
void createPlayerInventorySlotFrameElements(Frame* slotFrame);
void drawCharacterPreview(const int player, SDL_Rect pos, int fov, real_t offsetyaw);
void toggleShopBuybackView(const int player);
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
struct CustomColors_t
{
	Uint32 itemContextMenuHeadingText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionSelectedText = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionImg = 0xFFFFFFFF;
	Uint32 itemContextMenuOptionSelectedImg = 0xFFFFFFFF;
	Uint32 characterSheetNeutral = 0xFFFFFFFF;
	Uint32 characterSheetLightNeutral = 0xFFFFFFFF;
	Uint32 characterSheetGreen = 0xFFFFFFFF;
	Uint32 characterSheetRed = 0xFFFFFFFF;
	Uint32 characterSheetFaintText = 0xFFFFFFFF;
	Uint32 characterSheetOffWhiteText = 0xFFFFFFFF;
	Uint32 characterSheetHeadingText = 0xFFFFFFFF;
};
extern CustomColors_t hudColors;

// if true, use the new user interface
extern bool newui;
extern bool bUsePreciseFieldTextReflow;
extern bool bUseSelectedSlotCycleAnimation;
extern ConsoleVariable<bool> shareMinimap;

void openMinimap(int player);

struct StatusEffectQueueEntry_t
{
	real_t animateX = 0.0;
	real_t animateY = 0.0;
	real_t animateW = 0.0;
	real_t animateH = 0.0;
	int animateSetpointX = 0;
	int animateSetpointY = 0;
	int animateSetpointW = 0;
	int animateSetpointH = 0;
	int animateStartX = 0;
	int animateStartY = 0;
	int animateStartW = 0;
	int animateStartH = 0;
	SDL_Rect pos{ 0, 0, 0, 0 };
	SDL_Rect notificationTargetPosition{ 0, 0, 32, 32 };
	Uint32 lastUpdateTick = 0;
	int effect = -1;
	Uint32 customVariable = 0;
	bool lowDuration = false;
	enum NotificationStates_t : int
	{
		STATE_1,
		STATE_2,
		STATE_3,
		STATE_4,
		STATE_END
	};
	NotificationStates_t notificationState = STATE_1;
	NotificationStates_t notificationStateInit = STATE_1;
	StatusEffectQueueEntry_t(int _effect)
	{
		effect = _effect;
		lastUpdateTick = ticks;
		pos.w = getEffectSpriteNormalWidth();
		pos.h = getEffectSpriteNormalHeight();

		notificationTargetPosition.w = pos.w;
		notificationTargetPosition.h = pos.h;
	}
	int getEffectSpriteNormalWidth();
	int getEffectSpriteNormalHeight();
	void animate();
	void animateNotification(int player);
	void setAnimatePosition(int destx, int desty);
	void setAnimatePosition(int destx, int desty, int destw, int desth);
	real_t getStatusEffectLargestScaling(int player);
	real_t getStatusEffectMidScaling(int player);
};

struct StatusEffectQueue_t
{
    // defined in GameUI.cpp (linkage problems)
	static const int kEffectBread;
	static const int kEffectBloodHunger;
	static const int kEffectAutomatonHunger;
	static const int kSpellEffectOffset;

	Frame* statusEffectFrame = nullptr;
	Frame* statusEffectTooltipFrame = nullptr;
	int player = -1;
	std::deque<StatusEffectQueueEntry_t> effectQueue;
	std::deque<StatusEffectQueueEntry_t> notificationQueue;
	int getBaseEffectPosX();
	int getBaseEffectPosY();
	real_t tooltipOpacitySetpoint = 100;
	real_t tooltipOpacityAnimate = 1.0;
	Uint32 tooltipDeselectedTick = 0;
	int tooltipShowingEffectID = -1;
	int tooltipShowingEffectVariable = -1;
	bool insertEffect(int effectID, int spellID);
	int effectsPerRow = 4;
	bool requiresAnimUpdate = false;
	void updateAllQueuedEffects();
	void animateStatusEffectTooltip(bool showTooltip);
	bool doStatusEffectTooltip(StatusEffectQueueEntry_t& entry, SDL_Rect pos);
	void updateEntryImage(StatusEffectQueueEntry_t& entry, Frame::image_t* img);
	void createStatusEffectTooltip();
	void resetQueue()
	{
		requiresAnimUpdate = true;
		effectQueue.clear();
		notificationQueue.clear();
	}
	void deleteEffect(int effect)
	{
		requiresAnimUpdate = true;
		for ( auto it = effectQueue.begin(); it != effectQueue.end(); )
		{
			if ( (*it).effect == effect )
			{
				it = effectQueue.erase(it);
				continue;
			}
			++it;
		}
	}
	StatusEffectQueue_t(int _player) { player = _player; }
	static void loadStatusEffectsJSON();
	struct EffectDefinitionEntry_t
	{
		int effect_id = -1;
		int spell_id = -1;
		std::string internal_name = "";
		std::string name = "";
		std::string desc = "";
		std::string imgPath = "";
		std::vector<std::string> nameVariations;
		std::vector<std::string> descVariations;
		std::vector<int> useSpellIDForImgVariations;
		std::vector<std::string> imgPathVariations;
		int useSpellIDForImg = -1;
		bool neverDisplay = false;
		int sustainedSpellID = -1;
		std::string& getName(int variation = -1);
		std::string& getDesc(int variation = -1);
		int tooltipWidth = 200;
	};
	struct StatusEffectDefinitions_t
	{
		static std::unordered_map<int, EffectDefinitionEntry_t> allEffects;
		static std::unordered_map<int, EffectDefinitionEntry_t> allSustainedSpells;
		static Uint32 tooltipHeadingColor;
		static Uint32 tooltipDescColor;
		static Uint32 notificationTextColor;
		static std::string notificationFont;
		static void reset()
		{
			allEffects.clear();
			allSustainedSpells.clear();
		}
		static bool effectDefinitionExists(int effectID)
		{
			return (allEffects.find(effectID) != allEffects.end());
		}
		static bool sustainedSpellDefinitionExists(int spellID)
		{
			return (allSustainedSpells.find(spellID) != allSustainedSpells.end());
		}
		static EffectDefinitionEntry_t& getEffect(int effectID)
		{
			return allEffects[effectID];
		}
		static EffectDefinitionEntry_t& getSustainedSpell(int spellID)
		{
			return allSustainedSpells[spellID];
		}
		static std::string getEffectImgPath(EffectDefinitionEntry_t& entry, int variation = -1);
	};
};
extern StatusEffectQueue_t StatusEffectQueue[MAXPLAYERS];

struct SkillSheetFrames_t
{
	Frame* skillsFrame = nullptr;
	Frame* entryFrameLeft = nullptr;
	Frame* entryFrameRight = nullptr;
	Frame* skillDescFrame = nullptr;
	Frame* skillBgImgsFrame = nullptr;
	Frame* scrollAreaOuterFrame = nullptr;
	Frame* scrollArea = nullptr;
	Frame* entryFrames[NUMPROFICIENCIES] = { nullptr };
	Frame* effectFrames[10] = { nullptr };
	Frame* legendFrame = nullptr;
	bool legendTextRequiresReflow = true;
};
extern SkillSheetFrames_t skillSheetEntryFrames[MAXPLAYERS];

extern Frame* minimapFrame;