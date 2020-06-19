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

	SDL_Rect newPosition;
	newPosition.x = 0;
	newPosition.y = 0;
	newPosition.w = 0;
	newPosition.h = 0;
	bool hasPrevNotification = allNotifications.size() > 0;
	if ( hasPrevNotification )
	{
		auto& prevNotification = allNotifications.back();
		prevNotification.getDimensions(newPosition.x, newPosition.y, newPosition.w, newPosition.h);
	}

	SDL_Surface* surf = getImage(image);
	allNotifications.push_back(UIToastNotification(surf));
	auto& notification = allNotifications.back();
	if ( hasPrevNotification )
	{
		// stack this higher than the previous
		notification.setPosY(newPosition.y + newPosition.h + 8);
	}
	return &notification;
}