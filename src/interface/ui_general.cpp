/*-------------------------------------------------------------------------------

	BARONY
	File: ui_general.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "ui.hpp"
#include "../scores.hpp"
#ifdef WINDOWS
#include <shellapi.h>
#endif

#include "../ui/Button.hpp"
#include "../ui/Field.hpp"

static const char* smallfont_outline = "fonts/pixel_maz_multiline.ttf#16#2";
static const char* smallfont_no_outline = "fonts/pixel_maz_multiline.ttf#16#0";

void UIToastNotification::init()
{
	if (isInit)
	{
		return;
	}
	isInit = true;
	displayedText = mainCardText;
	mainCardIsHidden = false;
	mainCardHide = false;
	lastInteractedTick = ticks;

	frame = UIToastNotificationManager.frame->addFrame();
	frame->setColor(makeColor(0, 32, 160, 255));

	headerField = frame->addField("header", 128);
	headerField->setColor(makeColor(255, 255, 0, 255));
	headerField->setFont(smallfont_outline);
	mainField = frame->addField("main", 256);
	mainField->setColor(makeColor(255, 255, 255, 255));
	mainField->setFont(smallfont_outline);

	closeButton = frame->addButton("close");
	closeButton->setUserData(this);
	closeButton->setCallback([](Button& button){
		auto n = static_cast<UIToastNotification*>(button.getUserData());
		n->mainCardHide = true;
		n->dockedCardHide = false;
		n->lastInteractedTick = ticks;
		});

	actionButton = frame->addButton("action");
	actionButton->setUserData(this);
	actionButton->setCallback([](Button& button) {
		auto n = static_cast<UIToastNotification*>(button.getUserData());
		n->lastInteractedTick = ticks;
		n->buttonAction();
		});

	frameImage = frame->addImage(SDL_Rect{0, 0, 64, 64}, 0xffffffff, "", "image");
	progressBarBackground = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xffffffff, "", "progressBarBackground");
	progressBar = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xffffffff, "", "progressBar");
}

void UIToastNotification::draw()
{
	if (!isInit)
	{
		return;
	}

	headerField->setInvisible(true);
	mainField->setInvisible(true);
	closeButton->setInvisible(true);
	actionButton->setInvisible(true);
	frameImage->disabled = true;
	progressBar->disabled = true;
	progressBarBackground->disabled = true;

	if (subwindow || fadeout)
	{
		if (!fadeout && !(actionFlags & UI_NOTIFICATION_AUTO_HIDE))
		{
			// don't hide or close
		}
		else if ((fadeout && cardType == CardType::UI_CARD_ACHIEVEMENT))
		{
			// don't hide or close
		}
		else
		{
			temporaryCardHide = (actionFlags & UI_NOTIFICATION_AUTO_HIDE);
			lastInteractedTick = ticks;
			if (cardState == CardState::UI_CARD_STATE_SHOW)
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

	frame->setInvisible(false);

	if (cardState == CardState::UI_CARD_STATE_SHOW || cardState == CardState::UI_CARD_STATE_UPDATE)
	{
		drawMainCard();
	}
	else if (cardState == CardState::UI_CARD_STATE_DOCKED)
	{
		drawDockedCard();
	}
	else
	{
		frame->setInvisible(true);
	}
}

void UIToastNotification::undockCard()
{
	dockedCardHide = true;
	mainCardHide = false;
	lastInteractedTick = ticks;
}

void UIToastNotification::hideMainCard()
{
	mainCardHide = true;
	dockedCardHide = false;
	lastInteractedTick = ticks;
}

void UIToastNotification::showMainCard()
{
	mainCardHide = false;
	dockedCardHide = true;
	lastInteractedTick = ticks;
}

void UIToastNotification::cardForceTickUpdate()
{
	lastInteractedTick = ticks;
}

void UIToastNotification::updateCardStatisticEvent(int updatedValue)
{
	pendingStatisticUpdateCurrent = updatedValue;
	updateCardEvent(true, false);
}

void UIToastNotification::updateCardEvent(bool updateMainText, bool updateSecondaryText)
{
	lastInteractedTick = ticks;
	cardState = CardState::UI_CARD_STATE_UPDATE;
	if (updateMainText)
	{
		cardUpdateDisplayMainText = true;
		cardUpdateDisplaySecondaryText = false;
	}
	if (updateSecondaryText)
	{
		cardUpdateDisplayMainText = false;
		cardUpdateDisplaySecondaryText = true;
	}
}

void UIToastNotification::animate(int& xout, int& current_ticks, int duration, int width, bool hideElement, bool& isHidden)
{
	// scale duration to FPS - tested @ 144hz
	double scaledDuration = (duration / (144.f / std::max(1U, fpsLimit)));

	double t = current_ticks / static_cast<double>(scaledDuration);
	double result = -width * t * t * (3.0f - 2.0f * t); // bezier from 0 to width as t (0-1)
	xout = static_cast<int>(floor(result) + width);
	isHidden = false;
	if (hideElement)
	{
		current_ticks = std::max(current_ticks - 1, 0);
		if (current_ticks == 0)
		{
			isHidden = true;
		}
	}
	else
	{
		current_ticks = std::min(current_ticks + 1, static_cast<int>(scaledDuration));
	}
}

void UIToastNotification::drawDockedCard()
{
	SDL_Rect r;
	r.w = dockedCardWidth;
	r.h = dockedCardWidth;
	r.x = Frame::virtualScreenX - r.w + docked_animx;
	r.y = Frame::virtualScreenY - r.h - posy;
	frame->setSize(r);

#if defined(NINTENDO)
	const bool clicking = fingerdown;
	Sint32 mousex = (::fingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::fingery / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::ofingerx / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::ofingery / (float)yres) * (float)Frame::virtualScreenY;
#elif defined(EDITOR)
	const bool clicking = mousestatus[SDL_BUTTON_LEFT];
	Sint32 mousex = (::mousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (::mousey / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (::omousex / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (::omousey / (float)yres) * (float)Frame::virtualScreenY;
#else
	const bool clicking = mousestatus[SDL_BUTTON_LEFT];
	const int mouseowner = intro || gamePaused ? inputs.getPlayerIDAllowedKeyboard() : clientnum;
	Sint32 mousex = (inputs.getMouse(mouseowner, Inputs::X) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 mousey = (inputs.getMouse(mouseowner, Inputs::Y) / (float)yres) * (float)Frame::virtualScreenY;
	Sint32 omousex = (inputs.getMouse(mouseowner, Inputs::OX) / (float)xres) * (float)Frame::virtualScreenX;
	Sint32 omousey = (inputs.getMouse(mouseowner, Inputs::OY) / (float)yres) * (float)Frame::virtualScreenY;
#endif

	if (!temporaryCardHide && rectContainsPoint(r, omousex, omousey))
	{
		if (mousestatus[SDL_BUTTON_LEFT])
		{
			mousestatus[SDL_BUTTON_LEFT] = 0;
			undockCard();
		}
	}

	headerField->setInvisible(false);
	headerField->setText("<");
	headerField->setSize(SDL_Rect{0, 0, r.w, r.h});
	headerField->setJustify(Field::justify_t::CENTER);

	if (temporaryCardHide)
	{
		animate(docked_animx, docked_anim_ticks, docked_anim_duration, dockedCardWidth, false, dockedCardIsHidden);
	}
	else
	{
		animate(docked_animx, docked_anim_ticks, docked_anim_duration, dockedCardWidth, dockedCardHide, dockedCardIsHidden);
		if (dockedCardHide && dockedCardIsHidden)
		{
			cardState = CardState::UI_CARD_STATE_SHOW;
		}
	}
}

void UIToastNotification::drawMainCard()
{
	SDL_Rect r;
	r.w = cardWidth;
	r.h = showHeight + dockHeight;
	r.x = Frame::virtualScreenX - r.w + animx;
	r.y = Frame::virtualScreenY - r.h - posy - (showHeight - dockHeight);
	frame->setSize(r);

	if (actionFlags & UI_NOTIFICATION_STATISTIC_UPDATE)
	{
		drawProgressBar(r);
	}

	if (actionFlags & ActionFlags::UI_NOTIFICATION_CLOSE)
	{
		drawCloseButton(r);
	}

	if (actionFlags & ActionFlags::UI_NOTIFICATION_ACTION_BUTTON)
	{
		drawActionButton(r);
	}

	if (temporaryCardHide)
	{
		animate(animx, anim_ticks, anim_duration, cardWidth, true, mainCardIsHidden);
	}
	else if (cardState == CardState::UI_CARD_STATE_UPDATE)
	{
		animate(animx, anim_ticks, anim_duration, cardWidth, true, mainCardIsHidden);
		if (mainCardIsHidden)
		{
			cardState = CardState::UI_CARD_STATE_SHOW;
			if (cardUpdateDisplayMainText)
			{
				setDisplayedText(mainCardText.c_str());
			}
			else if (cardUpdateDisplaySecondaryText)
			{
				setDisplayedText(secondaryCardText.c_str());
			}
			cardUpdateDisplayMainText = false;
			cardUpdateDisplaySecondaryText = false;
			if (cardType == CardType::UI_CARD_ACHIEVEMENT && pendingStatisticUpdateCurrent != -1)
			{
				setStatisticCurrentValue(pendingStatisticUpdateCurrent);
				if (statisticUpdateCurrent >= statisticUpdateMax)
				{
					this->setHeaderText("Achievement Unlocked!");
					notificationImage = std::string("*#") + this->achievementID + std::string(".png");
				}
				pendingStatisticUpdateCurrent = -1;
			}
		}
	}
	else
	{
		bool oldHiddenStatus = mainCardIsHidden;
		animate(animx, anim_ticks, anim_duration, cardWidth, mainCardHide, mainCardIsHidden);
		if (mainCardHide && mainCardIsHidden)
		{
			cardState = actionFlags & UI_NOTIFICATION_REMOVABLE ?
				CardState::UI_CARD_STATE_REMOVED : CardState::UI_CARD_STATE_DOCKED;
			if (oldHiddenStatus != mainCardIsHidden && (actionFlags & UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE))
			{
				setDisplayedText(mainCardText.c_str());
			}
		}
		else
		{
			if (ticks - lastInteractedTick > idleTicksToHide)
			{
				mainCardHide = true;
				dockedCardHide = false;
			}
		}
	}

	headerField->setInvisible(false);
	headerField->setSize(SDL_Rect{ textx + 64, texty, r.w - textx, r.h - texty });
	headerField->setHJustify(Field::justify_t::LEFT);
	headerField->setVJustify(Field::justify_t::TOP);
	headerField->setText(headerCardText.c_str());

	mainField->setInvisible(false);
	mainField->setSize(SDL_Rect{ bodyx + 64, bodyy, r.w - bodyx, r.h - bodyy });
	mainField->setHJustify(Field::justify_t::LEFT);
	mainField->setVJustify(Field::justify_t::TOP);
	mainField->setText(displayedText.c_str());

	frameImage->path = notificationImage;
	frameImage->disabled = false;
}

void UIToastNotification::drawProgressBar(const SDL_Rect& imageDimensions)
{
	return; //wip

	int percent = (int)floor(statisticUpdateCurrent * 100 / static_cast<double>(statisticUpdateMax));

	SDL_Rect progressbar;
	progressbar.x = imageDimensions.x + imageDimensions.w + textx;
	progressbar.h = TTF12_HEIGHT + 2;
	progressbar.w = (Frame::virtualScreenX - 8 + animx) - progressbar.x - 10;
	progressbar.y = imageDimensions.y + imageDimensions.h - progressbar.h;
	drawWindowFancy(progressbar.x - 2, progressbar.y - 2, progressbar.x + progressbar.w + 2, progressbar.y + progressbar.h + 2);

	drawRect(&progressbar, makeColorRGB(36, 36, 36), 255);
	progressbar.w = std::min((Frame::virtualScreenX - 8 + animx) - progressbar.x - 4, static_cast<int>(progressbar.w * percent / 100.0));
	drawRect(&progressbar, uint32ColorBaronyBlue, 92);
	progressbar.w = (Frame::virtualScreenX - 8 + animx) - progressbar.x - TTF12_WIDTH;

	char progress_str[32] = { '\0' };
	const int len = snprintf(progress_str, sizeof(progress_str), "%d / %d", statisticUpdateCurrent, statisticUpdateMax);
	ttfPrintTextColor(ttf12, progressbar.x + progressbar.w / 2 - (len * TTF12_WIDTH) / 2,
		progressbar.y + 4, uint32ColorWhite, true, progress_str);
}

void UIToastNotification::drawCloseButton(const SDL_Rect& src)
{
	closeButton->setInvisible(false);

	const SDL_Rect r{ src.w - 4 - 16, 4, 16, 16 };
	closeButton->setSize(r);
}

void UIToastNotification::drawActionButton(const SDL_Rect& src)
{
	actionButton->setInvisible(false);

	const SDL_Rect r{ 4, src.h - 20, 16, 16 };
	actionButton->setSize(r);
	actionButton->setText(actionText.c_str());
}

UIToastNotificationManager_t UIToastNotificationManager;

void UIToastNotificationManager_t::drawNotifications(bool isMoviePlaying, bool beforeFadeout)
{
	if ( !bIsInit )
	{
		return;
	}

	frame->setSize(gui->getSize());
	frame->bringToTop();

	int cardPosY = 110; // update the card y values if number of notifications change.
	for ( auto& card : allNotifications )
	{
		card.skipDrawingCardThisTick = false;
	}

	bool bFirstDockedCard = true;

	for ( auto& card : allNotifications )
	{
		if ((isMoviePlaying || !intro) 
			&& !(card.actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE))
		{
			continue;
		}

		bool docked = false;
		if ( !(card.actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE) )
		{
			docked = (card.getCardState() == UIToastNotification::CardState::UI_CARD_STATE_DOCKED);
		}

		card.posy = cardPosY;
		const SDL_Rect newPosition{card.posx, card.posy, card.cardWidth, card.showHeight};
		// stack next card higher than the previous
		if ( docked )
		{
			if ( !bFirstDockedCard )
			{
				card.skipDrawingCardThisTick = true;
			}
			else
			{
				cardPosY += (newPosition.h + 8);
			}
			bFirstDockedCard = false;
		}
		else
		{
			cardPosY += (newPosition.h + 8);
		}
	}

	int maxAchievementCards = 3;
	if ( cardPosY >= Frame::virtualScreenY )
	{
		maxAchievementCards = 1;
	}
	int currentNumAchievementCards = 0;
	for ( auto& card : allNotifications )
	{
		if ( (isMoviePlaying || !intro) 
			&& !(card.actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE))
		{
			continue;
		}

		if ( card.cardType == UIToastNotification::CardType::UI_CARD_ACHIEVEMENT )
		{
			// achievements are drawn AFTER the fadeout
			if ( beforeFadeout )
			{
				// if fading, then skip this card.
				if ( fadealpha > 0 )
				{
					continue;
				}
				// if we aren't fading, then draw BEFORE - makes sure the cursor is available
			}
			else
			{
				if ( fadealpha == 0 )
				{
					continue;
				}
				// if we are fading, then draw AFTER - we don't see the cursor anyway
			}
		}
		else
		{
			// all other cards are drawn BEFORE the fadeout
			if ( !beforeFadeout ) 
			{
				continue;
			}
		}

		if ( card.cardType == UIToastNotification::CardType::UI_CARD_ACHIEVEMENT )
		{
			if ( currentNumAchievementCards >= maxAchievementCards )
			{
				card.cardForceTickUpdate(); // don't draw, but don't expire these.
				continue;
			}
			++currentNumAchievementCards;
		}

		if ( card.skipDrawingCardThisTick )
		{
			continue;
		}

		card.init();
		card.draw();
	}

	for ( size_t c = 0; c < allNotifications.size(); ++c )
	{
		auto& card = allNotifications[c];
		if (card.getCardState() == UIToastNotification::CardState::UI_CARD_STATE_REMOVED)
		{
			allNotifications.erase(allNotifications.begin() + c);
			--c;
		}
	}
}

UIToastNotification* UIToastNotificationManager_t::addNotification(const char* image)
{
	if ( !bIsInit )
	{
		init();
	}

	allNotifications.push_back(UIToastNotification(getImage(image)));
	auto& notification = allNotifications.back();
	return &notification;
}

void openURLTryWithOverlay(const std::string& url, bool forceSystemBrowser)
{
	bool useSystemBrowser = false;
	if ( !forceSystemBrowser )
	{
#ifdef STEAMWORKS
		if ( SteamUtils()->IsOverlayEnabled() )
		{
			SteamFriends()->ActivateGameOverlayToWebPage(url.c_str());
		}
		else
		{
			useSystemBrowser = true;
		}
#else
		useSystemBrowser = true;
#endif
	}
	else
	{
		useSystemBrowser = true;
	}

	if ( useSystemBrowser )
	{
#ifdef WINDOWS
#ifdef _UNICODE
		const int len = MultiByteToWideChar(CP_ACP, 0, url.c_str(), url.size() + 1, 0, 0);
		auto buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, url.c_str(), url.size() + 1, buf, len);
		ShellExecute(NULL, TEXT("open"), buf, NULL, NULL, 0);
		delete[] buf;
#else
		ShellExecute(NULL, TEXT("open"), TEXT(url.c_str()), NULL, NULL, 0);
#endif
#endif // WINDOWS
#ifdef APPLE
		//TODO: Mac equivalent.
		system(std::string("open " + url).c_str());
#endif // APPLE
#ifdef LINUX
		system(std::string("xdg-open " + url).c_str());
#endif // LINUX
#ifdef NINTENDO
		SDL_OpenURL(url.c_str());
#endif // NINTENDO
	}
}

void UIToastNotificationManager_t::createAchievementsDisabledNotification()
{
	if ( !UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_ACHIEVEMENTS_DISABLED) )
	{
		UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
		n->setHeaderText("Notification");
		n->setMainText("Achievements disabled");
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE);
		n->cardType = UIToastNotification::CardType::UI_CARD_ACHIEVEMENTS_DISABLED;
		n->setIdleSeconds(8);
	}
}

void truncateMainText(std::string& str)
{
	if ( str.length() > 24 )
	{
		for ( size_t c, offset = 0;;)
		{
			size_t lastoffset = offset;
			for ( c = lastoffset + 1; c < str.size(); ++c )
			{
				if ( str[c] == ' ' )
				{
					break;
				}
			}
			offset = c;
			if ( offset > 24 && lastoffset )
			{
				str[lastoffset] = '\n';
				break;
			}
			if ( offset >= str.size() )
			{
				break;
			}
		}
	}
}

void UIToastNotificationManager_t::createAchievementNotification(const char* name)
{
	UIToastNotification* n = nullptr;
	if ( n = getNotificationAchievementSingle(name) )
	{
		if ( n->actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_STATISTIC_UPDATE )
		{
			return; // don't create a new achievement card, since we have a statistic update present.
		}
	}

	const char* achievementName = "";
	{
		auto it = achievementNames.find(name);
		if (it != achievementNames.end())
		{
			achievementName = it->second.c_str();
		}
	}

	const std::string imgName = std::string("*#") + name + std::string(".png");
	n = UIToastNotificationManager.addNotification(imgName.c_str());
	n->setHeaderText("Achievement Unlocked!");

	std::string achStr = std::string(achievementName);
	truncateMainText(achStr);
	n->setMainText(achStr.c_str());

	n->setAchievementName(name);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	n->cardType = UIToastNotification::CardType::UI_CARD_ACHIEVEMENT;
	n->setIdleSeconds(5);
}

void UIToastNotificationManager_t::createStatisticUpdateNotification(const char* name, int currentValue, int maxValue)
{
	UIToastNotification* n = nullptr;
	if ( n = getNotificationAchievementSingle(name) )
	{
		n->setStatisticMaxValue(maxValue);
		n->updateCardStatisticEvent(currentValue);
		return;
	}

	const bool unlocked = (currentValue >= maxValue);
	const char* achievementName = "";
	{
		auto it = achievementNames.find(name);
		if ( it != achievementNames.end() )
		{
			achievementName = it->second.c_str();
		}
	}

	const std::string imgName = unlocked ?
		std::string("*#") + name + std::string(".png"):
		std::string("*#") + name + std::string("_l.png");
	n = UIToastNotificationManager.addNotification(imgName.c_str());
	n->setHeaderText(unlocked ? "Achievement Unlocked!" : "Achievement Updated!");

	std::string achStr = std::string(achievementName);
	truncateMainText(achStr);
	n->setMainText(achStr.c_str());
	n->setAchievementName(name);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_STATISTIC_UPDATE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	n->cardType = UIToastNotification::CardType::UI_CARD_ACHIEVEMENT;
	n->setStatisticCurrentValue(currentValue);
	n->setStatisticMaxValue(maxValue);
	n->setIdleSeconds(5);
}