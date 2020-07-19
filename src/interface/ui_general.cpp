/*-------------------------------------------------------------------------------

	BARONY
	File: ui_general.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "ui.hpp"
#include <shellapi.h>
UIToastNotificationManager_t UIToastNotificationManager;

void UIToastNotificationManager_t::drawNotifications()
{
	if ( !bIsInit )
	{
		return;
	}

	int cardPosY = 110; // update the card y values if number of notifications change.
	for ( auto& card : allNotifications )
	{
		card.setPosY(cardPosY);
		SDL_Rect newPosition;
		card.getDimensions(newPosition.x, newPosition.y, newPosition.w, newPosition.h);
		// stack next card higher than the previous
		cardPosY += (newPosition.h + 8);
	}

	for ( auto& card : allNotifications )
	{
		card.init();
		card.draw();
	}
}

UIToastNotification* UIToastNotificationManager_t::addNotification(ImageTypes image)
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

void communityLinkAction()
{
#ifdef STEAMWORKS
	if ( SteamUtils()->IsOverlayEnabled() )
	{
		SteamFriends()->ActivateGameOverlayToWebPage("https://discord.gg/P55tcYD");
	}
	else
	{
#ifdef WINDOWS
		//ShellExecute(NULL, TEXT("open"), TEXT("https://discord.gg/P55tcYD"), NULL, NULL, 0);
#endif // WINDOWS
		UIToastNotification* n = UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_COMMUNITY_LINK);
		if ( n )
		{
			n->showMainCard();
			n->updateCardEvent(false, true);
		}
	}
#endif
}

void UIToastNotificationManager_t::createCommunityNotification()
{
	if ( !UIToastNotificationManager.getNotificationSingle(UIToastNotification::CardType::UI_CARD_COMMUNITY_LINK) )
	{
		UIToastNotification* n = UIToastNotificationManager.addNotification(UIToastNotificationManager_t::GENERIC_TOAST_IMAGE);
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