/*-------------------------------------------------------------------------------

	BARONY
	File: updaterightsidebar.cpp
	Desc: contains updateRightSidebar()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

// note: as of some prealpha version I've since forgotten, this module is totally deprecated

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "interface.hpp"
#include "../magic/magic.hpp"
#include "../player.hpp"
#include "../entity.hpp"

void updateRightSidebar()
{
	//TODO: Update this to manage spells & skills.

	SDL_Rect pos;
	//pos.x = MAGICSPELL_LIST_X; pos.y = MAGICSPELL_LIST_Y;
	pos.x = 0;
	pos.y = 0;
	pos.w = 0;
	pos.h = 0;

	int height = rightsidebar_titlebar_img->h;
	int numitems = 2; //Just two for now: The appraisal skill & the spell list.
	//int maxItemsOnscreen = (camera.winh / rightsidebar_titlebar_img->h) - (rightsidebar_titlebar_img->h * 4);
	height += numitems * rightsidebar_slot_img->h;
	rightsidebar_height = height;
	//Now calculate the position.
	pos.x = RIGHTSIDEBAR_X;
	pos.y = RIGHTSIDEBAR_Y;

	//Draw the titlebar itself then the sidebar's text
	drawImage(rightsidebar_titlebar_img, NULL, &pos);
	int text_x = pos.x + (rightsidebar_titlebar_img->w / 2) - ((6 * 8) / 2);
	int text_y = pos.y + (rightsidebar_titlebar_img->h / 2) - (8 / 2);
	printText(font8x8_bmp, text_x, text_y, "Skills");

	pos.y += rightsidebar_titlebar_img->h;
	//Draw all the sidebar slots.

	//TODO: Make it recurse over a skill list or something?
	//If the mouse is over the slot, then draw the highlighted version.
	//TODO: Grey out activated skills. if (appraisal_timer > 0) { draw(slot_unselectable) }
	if (appraisal_timer > 0)
	{
		drawImage(rightsidebar_slot_grayedout_img, NULL, &pos); //The appraisal skill is grayed out while it's timing down. //TODO: Maybe a countdown timer or progress bar?
	}
	else if (mouseInBounds(pos.x, pos.x + rightsidebar_slot_img->w, pos.y, pos.y + rightsidebar_slot_img->h))
	{
		drawImage(rightsidebar_slot_highlighted_img, NULL, &pos);
		if (mousestatus[SDL_BUTTON_LEFT])
		{
			mousestatus[SDL_BUTTON_LEFT] = 0;
			identifygui_active = true;
			identifygui_appraising = true;
			gui_mode = GUI_MODE_INVENTORY;
			if ( removecursegui_active )
			{
				closeRemoveCurseGUI();
			}
			if ( openedChest[clientnum] )
			{
				openedChest[clientnum]->closeChest();
			}

			//Initialize Identify GUI game controller code here.
			initIdentifyGUIControllerCode();
		}
	}
	else
	{
		drawImage(rightsidebar_slot_img, NULL, &pos);
	}

	text_x = pos.x + (rightsidebar_slot_img->w / 2) - ((strlen("Appraisal") * 8) / 2);
	text_y = pos.y + (rightsidebar_slot_img->h / 2) - (8 / 2);
	printText(font8x8_bmp, text_x, text_y, "Appraisal");

	//Advance the position.
	pos.y += rightsidebar_slot_img->h;

	if (!spellList.first)
	{
		//Grayed out. No spells.
		drawImage(rightsidebar_slot_grayedout_img, NULL, &pos);
	}
	else if (mouseInBounds(pos.x, pos.x + rightsidebar_slot_img->w, pos.y, pos.y + rightsidebar_slot_img->h))
	{
		drawImage(rightsidebar_slot_highlighted_img, NULL, &pos);
		if (mousestatus[SDL_BUTTON_LEFT])
		{
			mousestatus[SDL_BUTTON_LEFT] = 0;
			gui_mode = GUI_MODE_MAGIC;
			if (shootmode)
			{
				shootmode = false;
				attributespage = 0;
			}
		}
	}
	else
	{
		//Draw the normal thing.
		drawImage(rightsidebar_slot_img, NULL, &pos);
	}

	text_x = pos.x + (rightsidebar_slot_img->w / 2) - ((strlen("Spell List") * 8) / 2);
	text_y = pos.y + (rightsidebar_slot_img->h / 2) - (8 / 2);
	printText(font8x8_bmp, text_x, text_y, "Spell List");

	//Advance the position.
	pos.y += rightsidebar_slot_img->h;
}
