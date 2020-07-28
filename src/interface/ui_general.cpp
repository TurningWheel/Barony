/*-------------------------------------------------------------------------------

	BARONY
	File: ui_general.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "ui.hpp"
#ifdef WINDOWS
#include <shellapi.h>
#endif
UIToastNotificationManager_t UIToastNotificationManager;

void UIToastNotificationManager_t::drawNotifications(bool isMoviePlaying, bool beforeFadeout)
{
	if ( !bIsInit )
	{
		return;
	}

	int cardPosY = 110; // update the card y values if number of notifications change.
	for ( auto& card : allNotifications )
	{
		if ((isMoviePlaying || !intro) 
			&& !(card.actionFlags & UIToastNotification::ActionFlags::UI_NOTIFICATION_REMOVABLE))
		{
			continue;
		}

		card.setPosY(cardPosY);
		SDL_Rect newPosition;
		card.getDimensions(newPosition.x, newPosition.y, newPosition.w, newPosition.h);
		// stack next card higher than the previous
		cardPosY += (newPosition.h + 8);
	}

	const int kMaxAchievementCards = 3;
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
			if ( currentNumAchievementCards >= kMaxAchievementCards )
			{
				card.cardForceTickUpdate(); // don't draw, but don't expire these.
				continue;
			}
			++currentNumAchievementCards;
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

UIToastNotification* UIToastNotificationManager_t::addNotification(SDL_Surface* image)
{
	if ( !bIsInit )
	{
		init();
	}

	SDL_Surface* surf = getImage(image);
	allNotifications.push_back(UIToastNotification(surf));
	auto& notification = allNotifications.back();
	return &notification;
}

void openURLTryWithOverlay(std::string url)
{
	bool useSystemBrowser = false;
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

	if ( useSystemBrowser )
	{
#ifdef WINDOWS
		ShellExecute(NULL, TEXT("open"), TEXT(url.c_str()), NULL, NULL, 0);
#endif // WINDOWS
#ifdef APPLE
		//TODO: Mac equivalent.
		system(std::string("open " + url).c_str());
#endif // WINDOWS
#ifdef LINUX
		//TODO: Linux equivalent.
#endif
	}
}

void communityLinkAction()
{
	openURLTryWithOverlay("https://discord.gg/P55tcYD");
	/*UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_COMMUNITY_LINK);
	if ( n )
	{
		n->showMainCard();
		n->updateCardEvent(false, true);
	}*/
}

void UIToastNotificationManager_t::createCommunityNotification()
{
	if ( !UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_COMMUNITY_LINK) )
	{
		UIToastNotification* n = UIToastNotificationManager.addNotification(nullptr);
		n->setHeaderText(std::string("Join the community!"));
		n->setMainText(std::string("Find co-op allies and\nchat in real-time on the\nofficial Barony Discord!"));
		n->setSecondaryText(std::string("Overlay is disabled -\nVisit discord.gg/P55tcYD\nin your browser to join!"));
		n->setActionText(std::string("Join"));
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_ACTION_BUTTON);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_AUTO_HIDE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_CLOSE);
		n->actionFlags |= (UIToastNotification::ActionFlags::UI_NOTIFICATION_RESET_TEXT_TO_MAIN_ON_HIDE);
		n->cardType = UIToastNotification::CardType::UI_CARD_COMMUNITY_LINK;
		n->buttonAction = &communityLinkAction;
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

	SDL_Surface* achievementImage = nullptr;
	{
		std::string imgName = name + std::string(".png");
		auto it = achievementImages.find(imgName.c_str());
		if (it != achievementImages.end())
		{
			achievementImage = it->second;
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

	n = UIToastNotificationManager.addNotification(achievementImage);
	n->setHeaderText(std::string("Achievement Unlocked!"));

	std::string achStr = std::string(achievementName);
	truncateMainText(achStr);
	n->setMainText(achStr);

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

	bool unlocked = (currentValue >= maxValue);

	SDL_Surface* achievementImage = nullptr;
	{
		std::string imgName = name + std::string("_l.png");
		if ( unlocked )
		{
			imgName = name + std::string(".png");
		}
		auto it = achievementImages.find(imgName.c_str());
		if ( it != achievementImages.end() )
		{
			achievementImage = it->second;
		}
	}
	const char* achievementName = "";
	{
		auto it = achievementNames.find(name);
		if ( it != achievementNames.end() )
		{
			achievementName = it->second.c_str();
		}
	}

	n = UIToastNotificationManager.addNotification(achievementImage);
	if ( unlocked )
	{
		n->setHeaderText(std::string("Achievement Unlocked!"));
	}
	else
	{
		n->setHeaderText(std::string("Achievement Updated!"));
	}

	std::string achStr = std::string(achievementName);
	truncateMainText(achStr);
	n->setMainText(achStr);
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