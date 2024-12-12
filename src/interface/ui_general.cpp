/*-------------------------------------------------------------------------------

	BARONY
	File: ui_general.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "ui.hpp"
#include "../menu.hpp"
#include "../scores.hpp"
#include "../mod_tools.hpp"
#ifdef WINDOWS
#include <shellapi.h>
#endif
#ifdef USE_PLAYFAB
#include "../playfab.hpp"
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
	setDisplayedText(mainCardText.c_str());
	mainCardIsHidden = false;
	mainCardHide = false;
	lastInteractedTick = ticks;

	frame = UIToastNotificationManager.frame->addFrame();
	frame->setColor(makeColor(0, 0, 0, 0));
	frame->setBorder(0);

	createGenericWindowDecorations(*frame);

	headerField = frame->addField("header", 128);
	headerField->setColor(makeColor(255, 255, 0, 255));
	headerField->setFont(smallfont_outline);
	mainField = frame->addField("main", 256);
	mainField->setColor(makeColor(255, 255, 255, 255));
	mainField->setFont(smallfont_outline);
	progressField = frame->addField("progress", 32);
	progressField->setColor(makeColor(255, 255, 255, 255));
	progressField->setFont(smallfont_outline);

	closeButton = frame->addButton("close");
	closeButton->setBorder(0);
	closeButton->setUserData(this);
	closeButton->setBackground("*#images/ui/Shop/Button_X_00.png");
	closeButton->setBackgroundHighlighted("*#images/ui/Shop/Button_XHigh_00.png");
	closeButton->setBackgroundActivated("*#images/ui/Shop/Button_XPress_00.png");
	closeButton->setColor(makeColor(255, 255, 255, 255));
	closeButton->setHighlightColor(makeColor(255, 255, 255, 255));
	closeButton->setText("X");
	closeButton->setFont(smallfont_outline);
	closeButton->setHideGlyphs(true);
	closeButton->setHideKeyboardGlyphs(true);
	closeButton->setHideSelectors(true);
	closeButton->setMenuConfirmControlType(0);
	closeButton->setTextHighlightColor(makeColor(201, 162, 100, 255));
	closeButton->setCallback([](Button& button){
		auto n = static_cast<UIToastNotification*>(button.getUserData());
		n->mainCardHide = true;
		n->dockedCardHide = false;
		n->lastInteractedTick = ticks;
		});

	actionButton = frame->addButton("action");
	actionButton->setBorder(0);
	actionButton->setUserData(this);
	actionButton->setBackground("*#images/ui/GameOver/UI_GameOver_Button_SpawnAsGhost_02.png");
	actionButton->setBackgroundHighlighted("*#images/ui/GameOver/UI_GameOver_Button_SpawnAsGhostHigh_02.png");
	actionButton->setBackgroundActivated("*#images/ui/GameOver/UI_GameOver_Button_SpawnAsGhostPress_02.png");
	actionButton->setColor(makeColor(255, 255, 255, 255));
	actionButton->setHighlightColor(makeColor(255, 255, 255, 255));
	actionButton->setSize(SDL_Rect{108, 0, 150, 34});
	actionButton->setText("X");
	actionButton->setFont(smallfont_outline);
	actionButton->setHideGlyphs(true);
	actionButton->setHideKeyboardGlyphs(true);
	actionButton->setHideSelectors(true);
	actionButton->setMenuConfirmControlType(0);
	actionButton->setTextHighlightColor(makeColor(201, 162, 100, 255));
	actionButton->setCallback([](Button& button) {
		auto n = static_cast<UIToastNotification*>(button.getUserData());
		n->lastInteractedTick = ticks;
		if (n->buttonAction) {
			n->buttonAction();
		}
		});

	frameImage = frame->addImage(SDL_Rect{0, 0, 0, 0}, 0xffffffff, "", "image");
	progressBarBackground = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, makeColorRGB(0, 48, 16), "images/system/white.png", "progressBarBackground");
	progressBar = frame->addImage(SDL_Rect{ 0, 0, 0, 0 }, makeColorRGB(0, 160, 48), "images/system/white.png", "progressBar");
}

void UIToastNotification::draw()
{
	if (!isInit)
	{
		return;
	}

	headerField->setInvisible(true);
	mainField->setInvisible(true);
	progressField->setInvisible(true);
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
	double scaledDuration = (duration / (144.0 / (std::max(1U, fpsLimit))));

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
    SDL_Rect r2 = r; r2.x = 0; r2.y = 0;
	sizeWindowDecorations(*frame, r2);

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
		if (clicking)
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
	r.h = showHeight;
	r.x = Frame::virtualScreenX - r.w + animx;
	r.y = Frame::virtualScreenY - r.h - posy;
	frame->setSize(r);
    SDL_Rect r2 = r; r2.x = 0; r2.y = 0;
    sizeWindowDecorations(*frame, r2);

	if (actionFlags & UI_NOTIFICATION_STATISTIC_UPDATE)
	{
		drawProgressBar(r);
	}

#ifndef NINTENDO
	if (actionFlags & ActionFlags::UI_NOTIFICATION_CLOSE)
	{
		drawCloseButton(r);
	}
#endif

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
					if ( actionFlags & UI_NOTIFICATION_CHALLENGE_UPDATE )
					{
						this->setHeaderText(Language::get(6156));
					}
					else
					{
						this->setHeaderText("Achievement Unlocked!");
						notificationImage = std::string("*#images/achievements/") + this->achievementID + std::string(".png");
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

	const int imgSize = 64;
	const int offset = 8;

	headerField->setInvisible(false);
	headerField->setSize(SDL_Rect{ textx + imgSize + offset, texty, r.w - textx - imgSize, r.h - texty });
	headerField->setHJustify(Field::justify_t::LEFT);
	headerField->setVJustify(Field::justify_t::TOP);
	headerField->setText(headerCardText.c_str());

	mainField->setInvisible(false);
	mainField->setSize(SDL_Rect{ bodyx + imgSize + offset, bodyy, r.w - bodyx - imgSize, r.h - bodyy });
	mainField->setHJustify(Field::justify_t::LEFT);
	mainField->setVJustify(Field::justify_t::CENTER);
	mainField->setText(displayedText.c_str());
	if ( mainField->getNumTextLines() > 1 )
	{
		mainField->setVJustify(Field::justify_t::TOP);
		mainField->setSize(SDL_Rect{ bodyx + imgSize + offset, 29, r.w - bodyx - imgSize, r.h - 29 });
	}

	frameImage->path = notificationImage;
	frameImage->disabled = false;
	frameImage->pos = SDL_Rect{offset, 8, imgSize, imgSize};
}

void UIToastNotification::drawProgressBar(const SDL_Rect& src)
{
	const int height = 16;
	const SDL_Rect r{ src.h, src.h - height - 8, src.w - src.h - 8, height };

	const int percent = (statisticUpdateCurrent * 100) / statisticUpdateMax;
	const int size = (r.w * percent) / 100;

	progressBarBackground->disabled = false;
	progressBarBackground->pos = r;
	if (size) {
		progressBar->disabled = false;
		progressBar->pos = SDL_Rect{ r.x, r.y, size < r.w ? size : r.w, r.h };
	}

	char progress_str[32] = { '\0' };
	const int len = snprintf(progress_str, sizeof(progress_str), "%d / %d",
		statisticUpdateCurrent < statisticUpdateMax ? statisticUpdateCurrent : statisticUpdateMax, statisticUpdateMax);
	progressField->setInvisible(false);
	progressField->setSize(SDL_Rect{r.x, r.y, r.w, r.h + 1});
	progressField->setHJustify(Field::justify_t::CENTER);
	progressField->setVJustify(Field::justify_t::CENTER);
	progressField->setText(progress_str);
}

void UIToastNotification::drawCloseButton(const SDL_Rect& src)
{
	closeButton->setInvisible(false);
	
	const SDL_Rect r{ src.w - 4 - 26, 4, 26, 26 };
	closeButton->setSize(r);
}

void UIToastNotification::drawActionButton(const SDL_Rect& src)
{
	actionButton->setInvisible(false);

	const SDL_Rect r{ 80, src.h - 40, 150, 34 };
	actionButton->setSize(r);
	actionButton->setText(actionText.c_str());
}

UIToastNotificationManager_t UIToastNotificationManager;
static ConsoleVariable<bool> cvar_achievements_warning("/achievements_warning", true);
void UIToastNotificationManager_t::drawNotifications(bool isMoviePlaying, bool beforeFadeout)
{
	if ( !bIsInit )
	{
		return;
	}

	if (achievementsCheck && !intro) {
		if ( gameModeManager.currentSession.challengeRun.isActive()
			&& gameModeManager.currentSession.challengeRun.lid.find("challenge") != std::string::npos )
		{
			achievementsCheck = false;
			if ( *cvar_achievements_warning )
			{
				createAchievementsDisabledNotification();
				if ( auto n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_ACHIEVEMENTS_DISABLED) )
				{
					n->setIdleSeconds(3);
					n->setHeaderText(Language::get(6139));
				}
			}
		}
		else if (conductGameChallenges[CONDUCT_CHEATS_ENABLED]
			|| conductGameChallenges[CONDUCT_LIFESAVING]
			|| conductGameChallenges[CONDUCT_ASSISTANCE_CLAIMED] >= GenericGUIMenu::AssistShrineGUI_t::achievementDisabledLimit
			|| Mods::disableSteamAchievements) {
			achievementsCheck = false;
			if ( *cvar_achievements_warning )
			{
				createAchievementsDisabledNotification();
			}
		}
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
		if (isMoviePlaying || !intro)
		{
			if (!(card.actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE))
			{
				continue;
			}
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
			if (card.isInit && card.frame)
			{
				card.frame->setInvisible(true);
			}
			continue;
		}

		card.init();
		card.draw();
	}

	auto next = allNotifications.begin();
	for ( auto it = allNotifications.begin(); it != allNotifications.end(); it = next )
	{
		++next;
		auto& card = *it;
		if (card.getCardState() == UIToastNotification::CardState::UI_CARD_STATE_REMOVED)
		{
			allNotifications.erase(it);
		}
	}
}

UIToastNotification* UIToastNotificationManager_t::addNotification(const char* image)
{
	if ( !bIsInit )
	{
		init();
	}

	allNotifications.emplace_back(getImage(image));
	auto& notification = allNotifications.back();
	return &notification;
}

void UIToastNotificationManager_t::createEpicLoginNotification()
{
	if (!UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT))
	{
		UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
		n->setHeaderText("Account Status");
		n->setMainText("Logging in...");
		n->setSecondaryText("An error has occurred!");
		n->setActionText("Retry");
		n->showHeight = 112;
		n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON); // unset
		n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->cardType = UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT;
		n->buttonAction = [](){
#ifdef USE_EOS
			EOS.AccountManager.AccountAuthenticationStatus = EOS_EResult::EOS_NotConfigured;
			EOS.initAuth();
#endif

			UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_EOS_ACCOUNT);
			if (n)
			{
				n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON); // unset
				n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
				n->showMainCard();
				n->setDisplayedText(n->getMainText());
				n->updateCardEvent(true, false);
			}
		};
		n->setIdleSeconds(5);
	}
}

void UIToastNotificationManager_t::createEpicCrossplayLoginNotification()
{
	if (!UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT))
	{
		UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
		n->setHeaderText("Crossplay Status");
		n->setMainText("Initializing...");
		n->setSecondaryText("An error has occurred!");
		n->setActionText("Retry");
		n->showHeight = 112;
		n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON); // unset
		n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags &= ~(UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->cardType = UIToastNotification::CardType::UI_CARD_CROSSPLAY_ACCOUNT;
		n->setIdleSeconds(5);
	}
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
		n->setHeaderText("Status Notification");
		n->setMainText("Achievements disabled.");
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
		n->cardType = UIToastNotification::CardType::UI_CARD_ACHIEVEMENTS_DISABLED;
		n->setIdleSeconds(5);
	}
}

void UIToastNotificationManager_t::createGenericNotification(const char* header, const char* text)
{
	UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->cardType = UIToastNotification::CardType::UI_CARD_DEFAULT;
	n->setHeaderText(header);
	n->setMainText(text);
	n->setIdleSeconds(10);
}

void UIToastNotificationManager_t::createLeaderboardNotification(std::string info)
{
#ifdef USE_PLAYFAB
	bool challenge = info.find("_seed_") != std::string::npos;
	if ( !challenge ) { return; }

	char buf[128];
	if ( info.find("oneshot") != std::string::npos )
	{
		std::string prefix = Language::get(6110);
		for ( auto& c : prefix )
		{
			if ( c == '\n' )
			{
				c = ' ';
			}
		}
		snprintf(buf, sizeof(buf), Language::get(6134), prefix.c_str());

		if ( info.find("lid_victory_seed_oneshot") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_ONESHOT] =
				"lid_time_victory_seed_oneshot";
		}
		else if ( info.find("lid_time_victory_seed_oneshot") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_ONESHOT] =
				"lid_time_victory_seed_oneshot";
		}
		else if ( info.find("lid_time_novictory_seed_oneshot") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_ONESHOT] =
				"lid_time_novictory_seed_oneshot";
		}
	}
	else if ( info.find("unlimited") != std::string::npos )
	{
		std::string prefix = Language::get(6111);
		for ( auto& c : prefix )
		{
			if ( c == '\n' )
			{
				c = ' ';
			}
		}
		snprintf(buf, sizeof(buf), Language::get(6134), prefix.c_str());

		if ( info.find("lid_victory_seed_unlimited") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_UNLIMITED] =
				"lid_victory_seed_unlimited";
		}
		else if ( info.find("lid_time_victory_seed_unlimited") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_UNLIMITED] =
				"lid_time_victory_seed_unlimited";
		}
		else if ( info.find("lid_time_novictory_seed_unlimited") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_UNLIMITED] =
				"lid_time_novictory_seed_unlimited";
		}
	}
	else if ( info.find("challenge") != std::string::npos )
	{
		std::string prefix = Language::get(6112);
		for ( auto& c : prefix )
		{
			if ( c == '\n' )
			{
				c = ' ';
			}
		}
		snprintf(buf, sizeof(buf), Language::get(6134), prefix.c_str());

		if ( info.find("lid_victory_seed_challenge") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_CHALLENGE] =
				"lid_victory_seed_challenge";
		}
		else if ( info.find("lid_time_victory_seed_challenge") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_CHALLENGE] =
				"lid_time_victory_seed_challenge";
		}
		else if ( info.find("lid_time_novictory_seed_challenge") != std::string::npos )
		{
			playfabUser.leaderboardSearch.savedSearchesFromNotification[PlayfabUser_t::LeaderboardSearch_t::CHALLENGE_BOARD_CHALLENGE] =
				"lid_time_novictory_seed_challenge";
		}
	}
	else
	{
		return;
	}

	UIToastNotification* n = UIToastNotificationManager.addNotification("*#images/ui/Main Menus/Challenges/seed_attempted64px.png");
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->cardType = UIToastNotification::CardType::UI_CARD_DEFAULT;
	n->showHeight = 112;
	n->setHeaderText(Language::get(6128));
	n->setMainText(buf);
	n->setIdleSeconds(5);
#endif
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

	const char* achievementName = "Unknown Achievement";
	{
		auto it = Compendium_t::achievements.find(name);
		if (it != Compendium_t::achievements.end())
		{
			achievementName = it->second.name.c_str();
		}
	}

	const std::string imgName = std::string("*#images/achievements/") + name + std::string(".png");
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

void UIToastNotificationManager_t::createNewSeedNotification()
{
	UIToastNotification* n = UIToastNotificationManager.addNotification("*#images/ui/Main Menus/Challenges/seed_attempted64px.png");
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->cardType = UIToastNotification::CardType::UI_CARD_DEFAULT;
	n->setHeaderText(Language::get(6169));
	n->setMainText(Language::get(6170));
	n->setIdleSeconds(10);
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
	const char* achievementName = "Unknown Achievement";
	bool challenge = false;
	if ( !strcmp(name, "CHALLENGE_MONSTER_KILLS") )
	{
		challenge = true;
		achievementName = Language::get(6155);
		n = UIToastNotificationManager.addNotification("*#images/ui/Main Menus/Challenges/seed_attempted64px.png");
		n->setHeaderText(unlocked ? Language::get(6156) : Language::get(6154));
	}
	else if ( !strcmp(name, "CHALLENGE_FURNITURE_KILLS") )
	{
		challenge = true;
		achievementName = Language::get(6157);
		n = UIToastNotificationManager.addNotification("*#images/ui/Main Menus/Challenges/seed_attempted64px.png");
		n->setHeaderText(unlocked ? Language::get(6156) : Language::get(6154));
	}
	else
	{
		auto it = Compendium_t::achievements.find(name);
		if ( it != Compendium_t::achievements.end() )
		{
			achievementName = it->second.name.c_str();
		}
		const std::string imgName = unlocked ?
			std::string("*#images/achievements/") + name + std::string(".png"):
			std::string("*#images/achievements/") + name + std::string("_l.png");
		n = UIToastNotificationManager.addNotification(imgName.c_str());
		n->setHeaderText(unlocked ? "Achievement Unlocked!" : "Achievement Updated!");
	}

	std::string achStr = std::string(achievementName);
	truncateMainText(achStr);
	n->setMainText(achStr.c_str());
	n->setAchievementName(name);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_STATISTIC_UPDATE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
	n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
	if ( challenge )
	{
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CHALLENGE_UPDATE);
	}
	n->cardType = UIToastNotification::CardType::UI_CARD_ACHIEVEMENT;
	n->setStatisticCurrentValue(currentValue);
	n->setStatisticMaxValue(maxValue);
	n->setIdleSeconds(5);
}

void createGenericWindowDecorations(Frame& frame) {
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "topleft");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "top");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "topright");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "left");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "center");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "right");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "bottomleft");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "bottom");
	frame.addImage(SDL_Rect{ 0,0,0,0 }, 0xffffffff, "*#images/ui/GenericWindow.png", "bottomright");
}

void sizeWindowDecorations(Frame& frame, SDL_Rect r) {
	{
		auto img = frame.findImage("topleft"); assert(img);
		img->section = SDL_Rect{ 0, 0, 16, 16 };
		img->pos = SDL_Rect{ r.x, r.y, 16, 16 };
	}
	{
		auto img = frame.findImage("top"); assert(img);
		img->section = SDL_Rect{ 16, 0, 16, 16 };
		img->pos = SDL_Rect{ r.x + 16, r.y, r.w - 32, 16 };
	}
	{
		auto img = frame.findImage("topright"); assert(img);
		img->section = SDL_Rect{ 32, 0, 16, 16 };
		img->pos = SDL_Rect{ r.x + r.w - 16, r.y, 16, 16 };
	}
	{
		auto img = frame.findImage("left"); assert(img);
		img->section = SDL_Rect{ 0, 16, 16, 16 };
		img->pos = SDL_Rect{ r.x, r.y + 16, 16, r.h - 32 };
	}
	{
		auto img = frame.findImage("center"); assert(img);
		img->section = SDL_Rect{ 16, 16, 16, 16 };
		img->pos = SDL_Rect{ r.x + 16, r.y + 16, r.w - 32, r.h - 32 };
	}
	{
		auto img = frame.findImage("right"); assert(img);
		img->section = SDL_Rect{ 32, 16, 16, 16 };
		img->pos = SDL_Rect{ r.x + r.w - 16, r.y + 16, 16, r.h - 32 };
	}
	{
		auto img = frame.findImage("bottomleft"); assert(img);
		img->section = SDL_Rect{ 0, 32, 16, 16 };
		img->pos = SDL_Rect{ r.x, r.y + r.h - 16, 16, 16 };
	}
	{
		auto img = frame.findImage("bottom"); assert(img);
		img->section = SDL_Rect{ 16, 32, 16, 16 };
		img->pos = SDL_Rect{ r.x + 16, r.y + r.h - 16, r.w - 32, 16 };
	}
	{
		auto img = frame.findImage("bottomright"); assert(img);
		img->section = SDL_Rect{ 32, 32, 16, 16 };
		img->pos = SDL_Rect{ r.x + r.w - 16, r.y + r.h - 16, 16, 16 };
	}
}

#include "consolecommand.hpp"
#include "../net.hpp"

static ConsoleCommand ccmd_toastTestAchievement("/toast_test_achievement", "",
	[](int argc, const char** argv){
		if (argc > 1) {
			UIToastNotificationManager.createAchievementNotification(argv[1]);
		} else {
			messagePlayer(clientnum, MESSAGE_DEBUG,
				"Give the name of a valid achievement\n"
				"ex: %s <BARONY_ACH_xxx>", argv[0]);
		}
	});

static ConsoleCommand ccmd_toastTestLeaderboard("/toast_test_leaderboard", "",
	[](int argc, const char** argv) {
		if ( argc > 1 ) {
			UIToastNotificationManager.createLeaderboardNotification(argv[1]);
		}
		else
		{
			UIToastNotificationManager.createLeaderboardNotification("");
		}
	});

static ConsoleCommand ccmd_toastTestStatistic("/toast_test_statistic", "",
	[](int argc, const char** argv) {
		if (argc > 1) {
			const int cur = argc > 2 ? (int)strtol(argv[2], nullptr, 10) : 0;
			const int max = argc > 3 ? (int)strtol(argv[3], nullptr, 10) : 10;
			UIToastNotificationManager.createStatisticUpdateNotification(argv[1], cur, max);
		}
		else {
			messagePlayer(clientnum, MESSAGE_DEBUG,
				"Give the name of a valid achievement\n"
				"ex: %s <BARONY_ACH_xxx> [current] [maximum]", argv[0]);
		}
	});

static ConsoleCommand ccmd_toastTestSeeds("/toast_test_seeds", "",
	[](int argc, const char** argv) {
		UIToastNotificationManager.createNewSeedNotification();
	});

static ConsoleCommand ccmd_toastTestAchievementsDisabled("/toast_test_achievements_disabled", "",
	[](int argc, const char** argv) {
		UIToastNotificationManager.createAchievementsDisabledNotification();
	});

static ConsoleCommand ccmd_toastTest("/toast_test", "",
	[](int argc, const char** argv) {
		UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
		n->setHeaderText("Test Notification");
		n->setMainText("This is a test.\nIt means nothing.\n");
		n->setSecondaryText("This is a secondary message.\n");
		n->setActionText("Do a thing");
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE);
		n->cardType = UIToastNotification::CardType::UI_CARD_DEFAULT;
		n->showHeight = 112;
		n->setIdleSeconds(8);
	});
