/*-------------------------------------------------------------------------------

	BARONY
	File: ui_general.cpp
	Desc: contains code for game interface

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/


#include "ui.hpp"
UIToastNotificationManager_t UIToastNotificationManager;

void UIToastNotificationManager_t::drawNotifications()
{
	if ( !bIsInit )
	{
		return;
	}

	for ( auto& card : allNotifications )
	{
		card.draw();
	}
}

void UIToastNotificationManager_t::addNotification(ImageTypes image, std::string headerText, std::string mainText, std::string secondaryText)
{
	if ( !bIsInit )
	{
		init();
	}
	SDL_Surface* surf = getImage(image);
	allNotifications.push_back(UIToastNotification(surf, headerText, mainText, secondaryText));
	auto& notification = allNotifications.back();
	notification.init();
}