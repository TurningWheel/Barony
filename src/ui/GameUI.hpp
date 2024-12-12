//! @file GameUI.hpp

#pragma once

#include "Frame.hpp"
#include "../stat.hpp"
#include "../game.hpp"
#include "../interface/consolecommand.hpp"
#include <deque>

Frame::result_t doFrames();
void doSharedMinimap();
extern Frame* gameUIFrame[MAXPLAYERS];
void addMessageToLogWindow(int player, string_t* string);
void updateSlotFrameFromItem(Frame* slotFrame, void* itemPtr, bool forceUnusable = false);
void createInventoryTooltipFrame(const int player, 
	Frame* parentFrame,
	Frame*& tooltipContainerFrame,
	Frame*& titleOnlyTooltipFrame,
	Frame*& tooltipFrame,
	Frame*& interactFrame,
	Frame*& promptFrame);
bool getSlotFrameXYFromMousePos(const int player, int& outx, int& outy, bool spells);
void resetInventorySlotFrames(const int player);
void createPlayerInventorySlotFrameElements(Frame* slotFrame);
void drawCharacterPreview(const int player, SDL_Rect pos, int fov, real_t offsetyaw, bool dark = false);
void drawObjectPreview(std::string modelsPath, Entity* object, SDL_Rect pos, real_t offsetyaw, bool dark = false);
void drawSpritesPreview(std::string name, std::string modelsPath, SDL_Rect pos, real_t offsetyaw, bool dark = false);
void drawItemPreview(Entity* item, SDL_Rect pos, real_t offsetyaw, bool dark = false);
extern view_t playerPortraitView[MAXPLAYERS];
void toggleShopBuybackView(const int player);
void loadHUDSettingsJSON();
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
	Uint32 characterSheetLighter1Neutral = 0xFFFFFFFF;
	Uint32 characterSheetDarker1Neutral = 0xFFFFFFFF;
	Uint32 characterSheetGreen = 0xFFFFFFFF;
	Uint32 characterSheetRed = 0xFFFFFFFF;
	Uint32 characterSheetFaintText = 0xFFFFFFFF;
	Uint32 characterSheetOffWhiteText = 0xFFFFFFFF;
	Uint32 characterSheetHeadingText = 0xFFFFFFFF;
	Uint32 characterSheetHighlightText = 0xFFFFFFFF;
	Uint32 characterBaseClassText = 0xFFFFFFFF;
	Uint32 characterDLC1ClassText = 0xFFFFFFFF;
	Uint32 characterDLC2ClassText = 0xFFFFFFFF;
};
extern CustomColors_t hudColors;

extern int GAMEUI_FRAMEDATA_ANIMATING_ITEM;
extern int GAMEUI_FRAMEDATA_ALCHEMY_ITEM;
extern int GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_SLOT; // displaying in main alchemy gui when hovering over recipe
extern int GAMEUI_FRAMEDATA_ALCHEMY_RECIPE_ENTRY; // the recipe icon
extern int GAMEUI_FRAMEDATA_WORLDTOOLTIP_ITEM;
extern int GAMEUI_FRAMEDATA_SHOP_ITEM;

// if true, use the new user interface
extern bool newui;
extern bool bUsePreciseFieldTextReflow;
extern bool bUseSelectedSlotCycleAnimation;
extern ConsoleVariable<bool> shareMinimap;
extern Frame::result_t framesProcResult;
extern ConsoleVariable<bool> framesEatMouse;

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

	enum Dir_t : int
	{
		NONE,
		LEFT,
		UP,
		DOWN,
		RIGHT
	};
	std::map<Dir_t, size_t> navigation;
	size_t index = 0;
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
	static const int kEffectBurning;
	static const int kEffectWanted;
	static const int kEffectWantedInShop;
	static const int kEffectFreeAction;
	static const int kEffectLesserWarning;
	static const int kEffectDisabledHPRegen;
	static const int kEffectResistBurning;
	static const int kEffectResistPoison;
	static const int kEffectSlowDigestion;
	static const int kEffectStrangulation;
	static const int kEffectWarning;
	static const int kEffectWaterBreathing;
	static const int kEffectConflict;
	static const int kEffectWaterWalking;
	static const int kEffectLifesaving;
	static const int kEffectPush;
	static const int kEffectSneak;
	static const int kEffectDrunkGoatman;
	static const int kEffectBountyTarget;
	static const int kEffectInspiration;
	static const int kEffectRetaliation;
	static const int kEffectAssistance;
	
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
	real_t focusedWindowAnim = 0.0;
	bool requiresAnimUpdate = false;
	bool bCompactWidth = false;
	bool bCompactHeight = false;
	SDL_Rect effectsBoundingBox{ 0, 0, 0, 0 };
	int selectedElement = -1;
	void updateAllQueuedEffects();
	void animateStatusEffectTooltip(bool showTooltip);
	bool doStatusEffectTooltip(StatusEffectQueueEntry_t& entry, SDL_Rect pos);
	void updateEntryImage(StatusEffectQueueEntry_t& entry, Frame::image_t* img);
	void createStatusEffectTooltip();
	Frame* getStatusEffectFrame();
	void handleNavigation(std::map<int, StatusEffectQueueEntry_t*>& grid, 
		bool& tooltipShowing, const bool hungerEffectInEffectQueue);
	void resetQueue()
	{
		requiresAnimUpdate = true;
		effectQueue.clear();
		notificationQueue.clear();
	}
	void deleteEffect(int effect)
	{
		for ( auto it = effectQueue.begin(); it != effectQueue.end(); )
		{
			if ( (*it).effect == effect )
			{
				requiresAnimUpdate = true;
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

struct PlayerInventoryFrames_t
{
	Frame* inventoryBgFrame = nullptr;
	Frame* selectedSlotFrame = nullptr;
	Frame* oldSelectedSlotFrame = nullptr;
	Frame* chestFrameSlots = nullptr;
	Frame* dollSlotsFrame = nullptr;
	Frame* invSlotsFrame = nullptr;
	Frame* backpackFrame = nullptr;
	Frame* flourishFrame = nullptr;
	Frame* characterPreview = nullptr;
	Frame* inventoryBaseImagesFrame = nullptr;
	Frame* backpackSlotsFrame = nullptr;
	Frame* chestBgFrame = nullptr;
	Frame* autosortFrame = nullptr;

	Frame::image_t* defaultInvImg = nullptr; //"inventory base img"
	Frame::image_t* compactInvImg = nullptr; //"inventory base compact img"
	Frame::image_t* compactCharImg = nullptr; //"inventory character compact img"
	Frame::image_t* oldSelectedSlotItemImg = nullptr; //"inventory old selected item"
	Frame::image_t* chestBaseImg = nullptr; //"chest base img"
	Frame::image_t* spellBaseImg = nullptr; //"spell base img"
};
extern PlayerInventoryFrames_t playerInventoryFrames[MAXPLAYERS];

extern Frame* minimapFrame; // shared minimap

void openMapWindow(int player);
void openLogWindow(int player);

void capitalizeString(std::string& str);
void uppercaseString(std::string& str);
void camelCaseString(std::string& str);
bool stringStartsWithVowel(std::string& str);

struct MinotaurWarning_t
{
	int state = 0;
	int stateInit = 0;
	real_t animFade = 0.0;
	real_t animBg = 0.0;
	real_t animFlash = 0.0;
	bool animFlashIncrease = true;
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

	Uint32 animTicks = 0;
	SDL_Rect pos{ 0, 0, 0, 0 };
	Uint32 processedOnTick = 0;
	bool started = false;
	bool initialWarningCompleted = false;
	Uint32 minotaurUid = 0;
	bool minotaurSpawned = false;
	bool minotaurDied = false;
	int levelProcessed = 0;
	bool secretlevelProcessed = false;
	void setAnimatePosition(const int destx, const int desty, const int destw, const int desth);
	void setAnimatePosition(int destx, int desty);
	void init();
	void deinit();
};
extern MinotaurWarning_t minotaurWarning[MAXPLAYERS];

struct LevelUpAnimation_t
{
	struct LevelUp_t
	{
		int currentLvl = -1;
		int increaseLvl = -1;
		Uint32 ticksActive = 0;
		Uint32 processedOnTick = 0;
		Uint32 ticksToLive = 6 * TICKS_PER_SECOND;
		struct StatUp_t
		{
			int whichStat = -1;
			int currentStat = -1;
			int increaseStat = -1;
			StatUp_t(const int _numstat, const int _currentStat, const int _increaseStat)
			{
				whichStat = _numstat;
				currentStat = _currentStat;
				increaseStat = _increaseStat;
			};
			void animateNotification(const int player);

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
			real_t animAngle = 0.0;
			real_t animCurrentStat = 0.0;
			real_t animIncreaseStat = 0.0;
			int baseX = 0;
			int baseY = 0;
			Uint32 ticksActive = 0;
			Uint32 processedOnTick = 0;
			SDL_Rect pos{ 0, 0, 0, 0 };
			SDL_Rect notificationTargetPosition{ 0, 0, 24, 24 };
			bool init = false;
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
			void setAnimatePosition(int destx, int desty);
			void setAnimatePosition(int destx, int desty, int destw, int desth);
		};
		std::deque<StatUp_t> statUps;
		LevelUp_t(const int _currentLvl, const int _increaseLvl)
		{
			currentLvl = _currentLvl;
			increaseLvl = _increaseLvl;
		}
		SDL_Rect titleAnimatePos;
		real_t animTitleFade = 0.0;
		real_t fadeout = 0.0;
		bool expired = false;
		bool titleFinishAnim = false;
		void animateTitle(SDL_Rect basePos);
	};
	std::deque<LevelUp_t> lvlUps;
	void addLevelUp(const int currentLvl, const int addLvl, std::vector<LevelUp_t::StatUp_t>& statInfo);
};

extern LevelUpAnimation_t levelUpAnimation[MAXPLAYERS];

void updateLevelUpFrame(const int player);
void updateSkillUpFrame(const int player);
struct SkillUpAnimation_t
{
	struct SkillUp_t
	{
		int whichSkill = -1;
		int currentSkill = -1;
		int increaseSkill = -1;
		int spellID = 0;
		SkillUp_t(const int _numSkill, const int _currentSkill, const int _increaseSkill)
		{
			whichSkill = _numSkill;
			currentSkill = _currentSkill;
			increaseSkill = _increaseSkill;
		};
		SkillUp_t(const int _spellID)
		{
			spellID = _spellID;
			isSpell = true;
		};
		void animateNotification(const int player);

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
		real_t animAngle = 0.0;
		real_t animCurrentStat = 0.0;
		real_t animIncreaseStat = 0.0;
		real_t animBackground = 0.0;
		int baseX = 0;
		int baseY = 0;
		Uint32 ticksActive = 0;
		Uint32 processedOnTick = 0;
		Uint32 preDelayTicks = 5;
		Uint32 ticksToLive = 3 * TICKS_PER_SECOND;
		SDL_Rect pos{ 0, 0, 0, 0 };
		SDL_Rect notificationTargetPosition{ 0, 0, 24, 24 };
		bool init = false;
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
		void setAnimatePosition(int destx, int desty);
		void setAnimatePosition(int destx, int desty, int destw, int desth);
		int getIconNominalSize();

		real_t fadeout = 0.0;
		bool expired = false;
		bool isSpell = false;
	};

	real_t animFrameFadeIn = 1.0;
	std::deque<SkillUp_t> skillUps;
	void addSkillUp(const int _numSkill, const int _currentSkill, const int _increaseSkill);
	void addSpellLearned(const int _spellID);
	size_t getSkillUpIndexToDisplay();
	SkillUp_t& getSkillUpToDisplay();
	static bool soundIndexUsedForNotification(const int index);
};

extern SkillUpAnimation_t skillUpAnimation[MAXPLAYERS];
