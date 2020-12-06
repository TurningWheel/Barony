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

class UIToastNotification
{
	double scalex = 0.25;
	double scaley = 0.25;
	int posx = 260;
	int posy = 110;
	int showHeight = 0;
	int dockHeight = 32;
	SDL_Surface* notificationImage = nullptr;
	bool isInit = false;
	int textx = 8;
	int texty = 0;
	int bodyx = 12;
	int bodyy = 16;
	
	SDL_Rect padding;
	int actionButtonOffsetY = 0;
	int actionButtonOffsetW = 0;

	int kImageBorderHeight = 256;
	int kImageBorderWidth = 256;

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

	Uint32 cardState = UI_CARD_STATE_SHOW;
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
public:
	UIToastNotification(SDL_Surface* image) {
		notificationImage = image;
		showHeight = static_cast<int>(kImageBorderHeight * scaley + 16);
		padding.x = 0;
		padding.y = 0;
		padding.w = 0;
		padding.h = 0;
	};
	~UIToastNotification(){};

	Uint32 actionFlags = 0;
	enum ActionFlags : Uint32
	{
		UI_NOTIFICATION_CLOSE = 1,
		UI_NOTIFICATION_ACTION_BUTTON = 2,
		UI_NOTIFICATION_AUTO_HIDE = 4,
		UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE = 8,
		UI_NOTIFICATION_REMOVABLE = 16,
		UI_NOTIFICATION_STATISTIC_UPDATE = 32
	};
	enum CardType : Uint32
	{
		UI_CARD_DEFAULT,
		UI_CARD_EOS_ACCOUNT,
		UI_CARD_CROSSPLAY_ACCOUNT,
		UI_CARD_COMMUNITY_LINK,
		UI_CARD_ACHIEVEMENT,
		UI_CARD_PROMO
	};
	CardType cardType = CardType::UI_CARD_DEFAULT;
	enum CardState : Uint32
	{
		UI_CARD_STATE_SHOW,
		UI_CARD_STATE_DOCKED,
		UI_CARD_STATE_UPDATE,
		UI_CARD_STATE_REMOVED
	};
	void (*buttonAction)() = nullptr;
	bool skipDrawingCardThisTick = false;
	bool bQueuedForUndock = false;
	void getDimensions(int& outPosX, int& outPosY, int& outPosW, int& outPosH)
	{
		outPosX = posx;
		outPosY = posy;
		outPosW = cardWidth;
		outPosH = showHeight;
	}
	void setDimensions(SDL_Rect& pos)
	{
		posx = pos.x;
		posy = pos.y;
		cardWidth = pos.w;
		animx = cardWidth;
		showHeight = pos.h;
	}
	void setPadding(SDL_Rect& pos)
	{
		padding.x = pos.x;
		padding.y = pos.y;
		padding.w = pos.w;
		padding.h = pos.h;
	}
	void setImageDimensions(int width, int height)
	{
		kImageBorderWidth = width;
		kImageBorderHeight = height;
	}
	void setImageScale(double x, double y)
	{
		scalex = x;
		scaley = y;
	}
	void setPosY(int y)
	{
		posy = y;
	}
	void setActionButtonOffsetY(int y)
	{
		actionButtonOffsetY = y;
	}
	void setActionButtonOffsetW(int w)
	{
		actionButtonOffsetW = w;
	}
	void setMainText(const std::string& text)
	{
		mainCardText = text;
	}
	void setSecondaryText(const std::string& text)
	{
		secondaryCardText = text;
	}
	void setHeaderText(const std::string& text)
	{
		headerCardText = text;
	}
	void setDisplayedText(const std::string& text)
	{
		displayedText = text;
	}
	void setActionText(const std::string& text)
	{
		actionText = text;
	}
	void setIdleSeconds(Uint32 seconds)
	{
		idleTicksToHide = seconds * TICKS_PER_SECOND;
	}
	void setStatisticCurrentValue(int value)
	{
		statisticUpdateCurrent = value;
	}
	void setStatisticMaxValue(int value)
	{
		statisticUpdateMax = value;
	}
	void setAchievementName(const char* achName)
	{
		achievementID = achName;
	}
	bool matchesAchievementName(const char* achName)
	{
		return (achievementID.compare(achName) == 0);
	}
	std::string& getMainText() { return mainCardText; };
	CardState getCardState() { return static_cast<CardState>(cardState); }

	void draw()
	{
		if ( !isInit )
		{
			return;
		}

		if ( subwindow || fadeout )
		{
			if ( !fadeout && !(actionFlags & UI_NOTIFICATION_AUTO_HIDE) )
			{
				// don't hide or close
			}
			else if ( (fadeout && cardType == UI_CARD_ACHIEVEMENT) )
			{
				// don't hide or close
			}
			else
			{
				temporaryCardHide = (actionFlags & UI_NOTIFICATION_AUTO_HIDE);
				lastInteractedTick = ticks;
				if ( cardState == UI_CARD_STATE_SHOW )
				{
					mainCardHide = true;
					dockedCardHide = false;
				}
			}
		}
		else
		{
			temporaryCardHide = false;
		}

		if ( cardState == UI_CARD_STATE_SHOW || cardState == UI_CARD_STATE_UPDATE )
		{
			drawMainCard();
		}
		else if ( cardState == UI_CARD_STATE_DOCKED )
		{
			drawDockedCard();
		}
	}

	void undockCard()
	{
		dockedCardHide = true;
		mainCardHide = false;
		lastInteractedTick = ticks;
	}

	void drawDockedCard()
	{
		SDL_Rect r;
		r.w = 32;
		r.h = static_cast<int>(kImageBorderHeight * scaley) + padding.h;
		r.x = xres - r.w + docked_animx;
		r.y = yres - r.h - posy;
		drawWindowFancy(r.x, r.y - 8, xres + 16 + docked_animx, r.y + 24);

		if ( !temporaryCardHide && mouseInBounds(clientnum, r.x, xres + docked_animx, r.y - 8, r.y + 24) )
		{
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				undockCard();
			}
		}
		ttfPrintTextColor(ttf16, r.x + 8, r.y + texty,
			SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255), true, "<");

		if ( temporaryCardHide )
		{
			animate(docked_animx, docked_anim_ticks, docked_anim_duration, dockedCardWidth, false, dockedCardIsHidden);
		}
		else
		{
			animate(docked_animx, docked_anim_ticks, docked_anim_duration, dockedCardWidth, dockedCardHide, dockedCardIsHidden);
			if ( dockedCardHide && dockedCardIsHidden )
			{
				cardState = UI_CARD_STATE_SHOW;
			}
		}
	}

	void hideMainCard()
	{
		mainCardHide = true;
		dockedCardHide = false;
		lastInteractedTick = ticks;
	}
	void showMainCard()
	{
		mainCardHide = false;
		dockedCardHide = true;
		lastInteractedTick = ticks;
	}
	void cardForceTickUpdate()
	{
		lastInteractedTick = ticks;
	}
	void updateCardStatisticEvent(int updatedValue)
	{
		pendingStatisticUpdateCurrent = updatedValue;
		updateCardEvent(true, false);
	}
	void updateCardEvent(bool updateMainText, bool updateSecondaryText)
	{
		lastInteractedTick = ticks;
		cardState = UI_CARD_STATE_UPDATE;
		if ( updateMainText )
		{
			cardUpdateDisplayMainText = true;
			cardUpdateDisplaySecondaryText = false;
		}
		if ( updateSecondaryText )
		{
			cardUpdateDisplayMainText = false;
			cardUpdateDisplaySecondaryText = true;
		}
	}

	void drawProgressBar(SDL_Rect& imageDimensions)
	{
		int percent = (int)floor(statisticUpdateCurrent * 100 / static_cast<double>(statisticUpdateMax));

		SDL_Rect progressbar;
		progressbar.x = imageDimensions.x + imageDimensions.w + textx;
		progressbar.h = TTF12_HEIGHT + 2;
		progressbar.w = (xres - 8 + animx) - progressbar.x - 10;
		progressbar.y = imageDimensions.y + imageDimensions.h - progressbar.h;
		drawWindowFancy(progressbar.x - 2, progressbar.y - 2, progressbar.x + progressbar.w + 2, progressbar.y + progressbar.h + 2);

		drawRect(&progressbar, SDL_MapRGB(mainsurface->format, 36, 36, 36), 255);
		progressbar.w = std::min((xres - 8 + animx) - progressbar.x - 4, static_cast<int>(progressbar.w * percent / 100.0));
		drawRect(&progressbar, uint32ColorBaronyBlue(*mainsurface), 92);
		progressbar.w = (xres - 8 + animx) - progressbar.x - TTF12_WIDTH;

		char progress_str[32] = { 0 };
		snprintf(progress_str, sizeof(progress_str), "%d / %d", statisticUpdateCurrent, statisticUpdateMax);
		ttfPrintTextColor(ttf12, progressbar.x + progressbar.w / 2 - (strlen(progress_str) * TTF12_WIDTH) / 2,
			progressbar.y + 4, uint32ColorWhite(*mainsurface), true, progress_str);
	}

	void drawMainCard()
	{
		SDL_Rect r;
		r.w = static_cast<int>(kImageBorderWidth * scalex);
		r.h = static_cast<int>(kImageBorderHeight * scaley) + padding.h;
		r.x = xres - r.w - posx + animx + 12;
		r.y = yres - r.h - posy;
		drawWindowFancy(r.x - 8, r.y - 8, xres - 8 + animx, r.y + r.h + 8);
		drawCloseButton(&r);

		if ( cardType == UI_CARD_PROMO )
		{
			// draw text centred
			Uint32 centrex = r.x + (r.w / 2);
			Uint32 textx = centrex - (headerCardText.length() * TTF12_WIDTH) / 2;
			ttfPrintTextColor(ttf12, textx, r.y + texty + padding.y, SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255), true,
				headerCardText.c_str());

			char c[256] = "";
			strcpy(c, displayedText.c_str());
			textx = centrex - (longestline(c) * TTF12_WIDTH) / 2;
			ttfPrintTextColor(ttf12, textx, r.y + bodyy + padding.y, SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255), true,
				displayedText.c_str());
		}
		else
		{
			ttfPrintTextColor(ttf12, r.x + r.w + textx, r.y + texty, SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255), true,
				headerCardText.c_str());

			ttfPrintTextColor(ttf12, r.x + r.w + bodyx, r.y + bodyy, SDL_MapRGBA(mainsurface->format, 255, 255, 255, 255), true,
				displayedText.c_str());
		}

		if ( cardType == UI_CARD_ACHIEVEMENT )
		{
			drawWindowFancy(r.x - 2, r.y - 2, r.x + r.w + 2, r.y + r.h + 2);
		}

		r.w -= padding.w;
		r.h -= padding.h;
		drawImageScaled(notificationImage, nullptr, &r);
		
		if ( actionFlags & UI_NOTIFICATION_STATISTIC_UPDATE )
		{
			drawProgressBar(r);
		}

		if ( drawActionButton(&r) )
		{
			if ( buttonAction != nullptr )
			{
				(*buttonAction)();
			}
		}

		if ( temporaryCardHide )
		{
			animate(animx, anim_ticks, anim_duration, cardWidth, true, mainCardIsHidden);
		}
		else if ( cardState == UI_CARD_STATE_UPDATE )
		{
			animate(animx, anim_ticks, anim_duration, cardWidth, true, mainCardIsHidden);
			if ( mainCardIsHidden )
			{
				cardState = UI_CARD_STATE_SHOW;
				if ( cardUpdateDisplayMainText )
				{
					setDisplayedText(mainCardText);
				}
				else if ( cardUpdateDisplaySecondaryText )
				{
					setDisplayedText(secondaryCardText);
				}
				cardUpdateDisplayMainText = false;
				cardUpdateDisplaySecondaryText = false;
				if ( cardType == UI_CARD_ACHIEVEMENT && pendingStatisticUpdateCurrent != -1 )
				{
					setStatisticCurrentValue(pendingStatisticUpdateCurrent);
					if ( statisticUpdateCurrent >= statisticUpdateMax )
					{
						this->setHeaderText(std::string("Achievement Unlocked!"));
						{
							std::string imgName = this->achievementID + std::string(".png");
							auto it = achievementImages.find(imgName.c_str());
							if ( it != achievementImages.end() )
							{
								notificationImage = it->second;
							}
						}
					}
					pendingStatisticUpdateCurrent = -1;
				}
			}
		}
		else
		{
			bool oldHiddenStatus = mainCardIsHidden;
			animate(animx, anim_ticks, anim_duration, cardWidth, mainCardHide, mainCardIsHidden);
			if ( mainCardHide && mainCardIsHidden )
			{
				cardState = actionFlags & UI_NOTIFICATION_REMOVABLE ? UI_CARD_STATE_REMOVED : UI_CARD_STATE_DOCKED;
				if ( oldHiddenStatus != mainCardIsHidden && (actionFlags & UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE) )
				{
					setDisplayedText(mainCardText);
				}
			}
			else
			{
				if ( ticks - lastInteractedTick > idleTicksToHide )
				{
					mainCardHide = true;
					dockedCardHide = false;
				}
			}
		}
	}
	void animate(int& xout, int& current_ticks, int duration, int width, bool hideElement, bool& isHidden)
	{
		// scale duration to FPS - tested @ 144hz
		double scaledDuration = (duration / (144.f / std::max(1U, fpsLimit)));

		double t = current_ticks / static_cast<double>(scaledDuration);
		double result = -width * t * t * (3.0f - 2.0f * t); // bezier from 0 to width as t (0-1)
		xout = static_cast<int>(floor(result) + width);
		isHidden = false;
		if ( hideElement )
		{
			current_ticks = std::max(current_ticks - 1, 0);
			if ( current_ticks == 0 )
			{
				isHidden = true;
			}
		}
		else
		{
			current_ticks = std::min(current_ticks + 1, static_cast<int>(scaledDuration));
		}
	}

	bool drawCloseButton(SDL_Rect* src)
	{
		if ( !(actionFlags & ActionFlags::UI_NOTIFICATION_CLOSE) )
		{
			return false;
		}
		SDL_Rect closeBtn;
		closeBtn.x = xres - 8 - 12 - 8 + animx;
		closeBtn.y = src->y - 8 + 4;
		closeBtn.w = 16;
		closeBtn.h = 16;
		if ( !temporaryCardHide && mouseInBounds(clientnum, closeBtn.x, closeBtn.x + closeBtn.w, closeBtn.y, closeBtn.y + closeBtn.h) )
		{
			drawDepressed(closeBtn.x, closeBtn.y, closeBtn.x + closeBtn.w, closeBtn.y + closeBtn.h);
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				ttfPrintText(ttf12, closeBtn.x + 1, closeBtn.y + 2, "x");
				mainCardHide = true;
				dockedCardHide = false;
				lastInteractedTick = ticks;
				return true;
			}
		}
		else
		{
			drawWindow(closeBtn.x, closeBtn.y, closeBtn.x + closeBtn.w, closeBtn.y + closeBtn.h);
		}
		ttfPrintText(ttf12, closeBtn.x + 1, closeBtn.y + 2, "x");
		return false;
	}

	bool drawActionButton(SDL_Rect* src)
	{
		if ( !(actionFlags & ActionFlags::UI_NOTIFICATION_ACTION_BUTTON) )
		{
			return false;
		}

		SDL_Rect actionBtn;
		actionBtn.x = src->x + 4 - actionButtonOffsetW / 2;
		actionBtn.y = src->y + src->h - 4 - TTF12_HEIGHT + actionButtonOffsetY;
		actionBtn.w = src->w - 8 + actionButtonOffsetW;
		actionBtn.h = 20;
		Uint32 textx = actionBtn.x + (actionBtn.w / 2) - ((TTF12_WIDTH * actionText.length()) / 2) - 3;
		if ( actionText.length() == 4 )
		{
			textx += 1;
		}
		if ( !temporaryCardHide && mouseInBounds(clientnum, actionBtn.x, actionBtn.x + actionBtn.w, actionBtn.y, actionBtn.y + actionBtn.h) )
		{
			//drawDepressed(actionBtn.x, actionBtn.y, actionBtn.x + actionBtn.w, actionBtn.y + actionBtn.h);
			drawWindowFancy(actionBtn.x, actionBtn.y, actionBtn.x + actionBtn.w, actionBtn.y + actionBtn.h);
			drawRect(&actionBtn, uint32ColorBaronyBlue(*mainsurface), 32);
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				ttfPrintText(ttf12, textx, actionBtn.y + 5, actionText.c_str());
				lastInteractedTick = ticks;
				return true;
			}
		}
		else
		{
			drawWindowFancy(actionBtn.x, actionBtn.y, actionBtn.x + actionBtn.w, actionBtn.y + actionBtn.h);
			drawRect(&actionBtn, SDL_MapRGB(mainsurface->format, 255, 255, 255), 32);
		}
		ttfPrintText(ttf12, textx, actionBtn.y + 5, actionText.c_str());
		return false;
	}
	void init()
	{
		if ( isInit )
		{
			return;
		}
		displayedText = mainCardText;
		mainCardIsHidden = false;
		mainCardHide = false;
		isInit = true;
		lastInteractedTick = ticks;
	}
};


class UIToastNotificationManager_t
{
	SDL_Surface* communityLink1 = nullptr;
	SDL_Surface* promoLink1 = nullptr;
	Uint32 undockTicks = 0;
	const Uint32 timeToUndock = 25;
	Uint32 lastUndockTick = 0;
public:
	UIToastNotificationManager_t() {};
	~UIToastNotificationManager_t()
	{ 
		SDL_FreeSurface(communityLink1);
		SDL_FreeSurface(promoLink1);
	};
	SDL_Surface* getImage(SDL_Surface* image)
	{
		if ( image == nullptr )
		{
			return communityLink1;
		}
		return image;
	};

	bool bIsInit = false;
	void init()
	{
		bIsInit = true;
		communityLink1 = loadImage("images/system/CommunityLink1.png");
		promoLink1 = loadImage("images/system/Promo1.png");
	}
	void drawNotifications(bool isMoviePlaying, bool beforeFadeout);
	void createCommunityNotification();
	void createPromoNotification();
	void createAchievementNotification(const char* name);
	void createStatisticUpdateNotification(const char* name, int currentValue, int maxValue);
	void undockAllCards();
	void processUndockingCards();
	/// @param image nullptr for default barony icon
	UIToastNotification* addNotification(SDL_Surface* image);

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
	std::vector<UIToastNotification> allNotifications;
};
extern UIToastNotificationManager_t UIToastNotificationManager;

void openURLTryWithOverlay(std::string url, bool forceSystemBrowser = false);