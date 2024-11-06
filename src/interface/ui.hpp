/*-------------------------------------------------------------------------------

	BARONY
	File: ui.hpp
	Desc: contains interface related declarations

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "../main.hpp"
#include "../game.hpp"
#include "../files.hpp"
#include "../draw.hpp"
#include "interface.hpp"
#include "../colors.hpp"
#include "../ui/Frame.hpp"

class UIToastNotification
{
	friend class UIToastNotificationManager_t;

public:
	void resetUIPointers()
	{
		frame = nullptr;
		headerField = nullptr;
		mainField = nullptr;
		progressField = nullptr;
		closeButton = nullptr;
		actionButton = nullptr;
		frameImage = nullptr;
		progressBar = nullptr;
		progressBarBackground = nullptr;
	}

	UIToastNotification(const char* image) :
		notificationImage(image)
	{}
	~UIToastNotification() {
		if (frame) {
			frame->removeSelf();
			resetUIPointers();
		}
	}

	Uint32 actionFlags = 0;
	enum ActionFlags : Uint32
	{
		UI_NOTIFICATION_CLOSE = 1,
		UI_NOTIFICATION_AUTO_HIDE = 2,
		UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE = 4,
		UI_NOTIFICATION_REMOVABLE = 8,
		UI_NOTIFICATION_STATISTIC_UPDATE = 16,
		UI_NOTIFICATION_ACTION_BUTTON = 32,
		UI_NOTIFICATION_CHALLENGE_UPDATE = 64
	};
	enum class CardType : Uint32
	{
		UI_CARD_DEFAULT,
		UI_CARD_EOS_ACCOUNT,
		UI_CARD_CROSSPLAY_ACCOUNT,
		UI_CARD_ACHIEVEMENT,
		UI_CARD_ACHIEVEMENTS_DISABLED,
	};
	enum class CardState : Uint32
	{
		UI_CARD_STATE_SHOW,
		UI_CARD_STATE_DOCKED,
		UI_CARD_STATE_UPDATE,
		UI_CARD_STATE_REMOVED
	};

	CardType cardType = CardType::UI_CARD_DEFAULT;

	void setMainText(const char* text) {
		mainCardText = text;
	}
	void setSecondaryText(const char* text) {
		secondaryCardText = text;
	}
	void setHeaderText(const char* text) {
		headerCardText = text;
	}
	void setActionText(const char* text) {
		actionText = text;
	}
	void setDisplayedText(const char* text) {
		displayedText = text;
	}

	const std::string& getDisplayedText()
	{
		return displayedText;
	}

	void setIdleSeconds(Uint32 seconds) {
		idleTicksToHide = seconds * TICKS_PER_SECOND;
	}
	void setStatisticCurrentValue(int value) {
		statisticUpdateCurrent = value;
	}
	void setStatisticMaxValue(int value) {
		statisticUpdateMax = value;
	}
	void setAchievementName(const char* achName) {
		achievementID = achName;
	}
	const char* getMainText() { return mainCardText.c_str(); }
	CardState getCardState() { return cardState; }

	void init();
	void draw();
	void undockCard();
	void hideMainCard();
	void showMainCard();
	void cardForceTickUpdate();
	void updateCardStatisticEvent(int updatedValue);
	void updateCardEvent(bool updateMainText, bool updateSecondaryText);
	void animate(int& xout, int& current_ticks, int duration, int width, bool hideElement, bool& isHidden);

	void drawDockedCard();
	void drawMainCard();
	void drawProgressBar(const SDL_Rect& src);
	void drawCloseButton(const SDL_Rect& src);
	void drawActionButton(const SDL_Rect& src);

	void (*buttonAction)() = nullptr;
	
	int showHeight = 80;

private:
	int posx = 260;
	int posy = 110;
	static const int dockHeight = 32;
	std::string notificationImage;
	bool isInit = false;
	int textx = 8;
	int texty = 8;
	int bodyx = 8;
	int bodyy = 0;

	bool mainCardHide = false;
	bool mainCardIsHidden = false;
	int cardWidth = 332;
	int animx = cardWidth;
	int anim_ticks = 0;
	int anim_duration = 50;

	bool dockedCardHide = true;
	bool dockedCardIsHidden = true;
	int dockedCardWidth = 48;
	int docked_animx = dockedCardWidth;
	int docked_anim_ticks = 0;
	int docked_anim_duration = 25;

	CardState cardState = CardState::UI_CARD_STATE_SHOW;

	bool temporaryCardHide = false;
	bool idleDisappear = true;
	Uint32 lastInteractedTick = 0;
	bool cardUpdateDisplayMainText = false;
	bool cardUpdateDisplaySecondaryText = false;
	Uint32 idleTicksToHide = 10 * TICKS_PER_SECOND;

	std::string displayedText;
	std::string mainCardText;
	std::string secondaryCardText;
	std::string headerCardText;
	std::string actionText;

	int statisticUpdateCurrent = 0;
	int statisticUpdateMax = 0;
	int pendingStatisticUpdateCurrent = -1;
	std::string achievementID = "";

	bool skipDrawingCardThisTick = false;
	bool bQueuedForUndock = false;

	Frame* frame = nullptr;
	Field* headerField = nullptr;
	Field* mainField = nullptr;
	Field* progressField = nullptr;
	Button* closeButton = nullptr;
	Button* actionButton = nullptr;
	Frame::image_t* frameImage = nullptr;
	Frame::image_t* progressBar = nullptr;
	Frame::image_t* progressBarBackground = nullptr;

	bool matchesAchievementName(const char* achName) {
		return (achievementID.compare(achName) == 0);
	}
};

class UIToastNotificationManager_t
{
	friend class UIToastNotification;

	Uint32 undockTicks = 0;
	const Uint32 timeToUndock = 25;
	Uint32 lastUndockTick = 0;
	bool bIsInit = false;
	Frame* frame = nullptr;
	bool achievementsCheck = false;
public:
	UIToastNotificationManager_t() = default;
	~UIToastNotificationManager_t()
	{
        term(true);
	}

	const char* getImage(const char* image)
	{
		if ( image == nullptr )
		{
			return "#images/ui/Toasts/bell1.png";
		}
		return image;
	};

	void init()
	{
		if (bIsInit) {
			return;
		}
		bIsInit = true;
		//communityLink1 = loadImage("images/system/CommunityLink1.png");
		//promoLink1 = loadImage("images/system/Promo1.png");
		frame = gui->addFrame("toasts");
		frame->setHollow(true);
		if (intro) {
			achievementsCheck = true;
		}
	}
	void term(const bool clearNotifications)
	{
		if (!bIsInit) {
			return;
		}
		bIsInit = false;
		if ( clearNotifications )
		{
			allNotifications.clear();
		}
		else
		{
			for ( auto& n : allNotifications )
			{
				n.isInit = false;
				n.resetUIPointers();
			}
		}
		if (frame) {
			frame->removeSelf();
			frame = nullptr;
		}
	}
	void drawNotifications(bool isMoviePlaying, bool beforeFadeout);
	void createEpicLoginNotification();
	void createEpicCrossplayLoginNotification();
	void createAchievementsDisabledNotification();
	void createGenericNotification(const char* header, const char* text);
	void createAchievementNotification(const char* name);
	void createLeaderboardNotification(std::string info);
	void createStatisticUpdateNotification(const char* name, int currentValue, int maxValue);
	void createNewSeedNotification();
	void undockAllCards();

	/// @param image nullptr for default barony icon
	UIToastNotification* addNotification(const char* image);

	UIToastNotification* getNotificationSingle(UIToastNotification::CardType cardType)
	{
		for ( auto& card : allNotifications )
		{
			if ( card.cardType == cardType )
			{
				return &card;
			}
		}
		return nullptr;
	}
	UIToastNotification* getNotificationAchievementSingle(const char* achName)
	{
		for ( auto& card : allNotifications )
		{
			if ( card.cardType == UIToastNotification::CardType::UI_CARD_ACHIEVEMENT )
			{
				if ( card.matchesAchievementName(achName) )
				{
					return &card;
				}
			}
		}
		return nullptr;
	}
	std::list<UIToastNotification> allNotifications;
};
extern UIToastNotificationManager_t UIToastNotificationManager;

void openURLTryWithOverlay(const std::string& url, bool forceSystemBrowser = false);
void createGenericWindowDecorations(Frame& frame);
void sizeWindowDecorations(Frame& frame, SDL_Rect size);
