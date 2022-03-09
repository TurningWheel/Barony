//! @file GameUI.hpp

#pragma once

void doFrames();
#include "Frame.hpp"
#include <deque>

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
	void animateNotification();
	void setAnimatePosition(int destx, int desty);
	void setAnimatePosition(int destx, int desty, int destw, int desth);
};

struct StatusEffectQueue_t
{
	int player = -1;
	std::deque<StatusEffectQueueEntry_t> effectQueue;
	std::deque<StatusEffectQueueEntry_t> notificationQueue;
	static const int kBaseEffectPosX;
	static const int kBaseEffectPosY;
	static const int kEffectBread = -2;
	static const int kSpellEffectOffset = 10000;
	bool insertEffect(int effectID, int spellID);
	int effectsPerRow = 4;
	bool requiresAnimUpdate = false;
	void updateAllQueuedEffects();
	void updateEntry(StatusEffectQueueEntry_t& entry); // does drawing for now
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
		int useSpellIDForImg = -1;
		bool neverDisplay = false;
	};
	struct StatusEffectDefinitions_t
	{
		static std::unordered_map<int, EffectDefinitionEntry_t> allEffects;
		static std::unordered_map<int, EffectDefinitionEntry_t> allSustainedSpells;
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
extern StatusEffectQueue_t statusEffectQueue;
