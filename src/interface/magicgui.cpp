/*-------------------------------------------------------------------------------

	BARONY
	File: magicgui.cpp
	Desc: contains the functions related to the magic GUI.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "interface.hpp"
#include "../items.hpp"
#include "../magic/magic.hpp"
#include "../player.hpp"
#include "../colors.hpp"

/*-------------------------------------------------------------------------------

	renderMagicGUI

	draws all the magic related gui stuff

-------------------------------------------------------------------------------*/

void renderMagicGUI(int winx, int winy, int winw, int winh)
{
	/*if (!spellList) { //I woke up with this commented out for some reason. TODO: Look into it.
		return; //Can't continue without a spell list!
	}*/
	SDL_Rect pos;
	//pos.x = MAGICSPELL_LIST_X; pos.y = MAGICSPELL_LIST_Y;
	pos.x = 0;
	pos.y = 0;
	pos.w = 0;
	pos.h = 0;

	if (magic_GUI_state == 0)   //TODO: use defines, not numbers.
	{
		//drawImage(magicspellList_bmp, NULL, &pos);
		//TODO: Assemble the interface.
		//First off, calculate how big the end shebang is gonna be.
		//Titlebar
		//One box for each spell up to max spells that can be displayed onscreen at once.
		//Set position such that it renders right in the middle of the game view.
		//Render title bar, increment position and draw all the boxes.
		//Draw arrow selector thingies if needed.
		//Keep track of all needed data to make sure clicking on a box interacts with that spell and the arrow thingies work.
		//Don't forget to keep track of spell list scroll, just like in chests.
		//TODO: Scroll through spells.

		int height = spell_list_titlebar_bmp->h;
		int numspells = 0;
		node_t* node;
		for (node = spellList.first; node != NULL; node = node->next)   //TODO: Create spellList. -- Done?
		{
			numspells++;
		}
		int maxSpellsOnscreen = winh / spell_list_gui_slot_bmp->h;
		numspells = std::min(numspells, maxSpellsOnscreen);
		height += numspells * spell_list_gui_slot_bmp->h;
		//Now calculate the position.
		pos.x = winx + (winw / 2) - (spell_list_gui_slot_bmp->w / 2) + magicspell_list_offset_x;
		pos.y = winy + ((winh / 2) - (height / 2)) + magicspell_list_offset_x;
		magic_gui_pos.x = pos.x;
		magic_gui_pos.y = pos.y;

		drawImage(spell_list_titlebar_bmp, NULL, &pos);
		int text_x = pos.x + (spell_list_titlebar_bmp->w / 2) - ((6 * 8) / 2 /*text characers * font width / 2*/ );
		int text_y = pos.y + (spell_list_titlebar_bmp->h / 2) - (8 / 2 /*font height / 2*/);
		printText(font8x8_bmp, text_x, text_y, language[322]);

		pos.y += spell_list_titlebar_bmp->h;
		int i = 0;
		//Draw all the spell GUI slots.
		node = spellList.first; //This will be needed to grab the name of the spell when its slot is drawn.
		for (i = 0; i < spellscroll; ++i)
		{
			if (node)
			{
				node = node->next;
			}
		}
		for (i = 0; i < numspells; ++i)
		{
			if (node)   //If the node exists (so that there's no crashes midgame...though the node should not be null in the first place. If it is, you have a problem.
			{
				spell_t* spell = (spell_t*)node->element;
				if (spell)
				{
					//If the mouse is over the slot, then draw the highlighted version.
					if (mouseInBounds(pos.x, pos.x + spell_list_gui_slot_bmp->w, pos.y, pos.y + spell_list_gui_slot_bmp->h))
					{
						drawImage(spell_list_gui_slot_highlighted_bmp, NULL, &pos);
					}
					else
					{
						drawImage(spell_list_gui_slot_bmp, NULL, &pos);
					}

					text_x = pos.x + (spell_list_gui_slot_bmp->w / 2) - ((strlen(spell->name) * 8) / 2 /*text characers * font width / 2*/ );
					text_y = pos.y + (spell_list_gui_slot_bmp->h / 2) - (8 / 2 /*font height / 2*/);
					printText(font8x8_bmp, text_x, text_y, spell->name);

					//Advance the position and the node in the spell list.
					pos.y += spell_list_gui_slot_bmp->h;
					node = node->next;
				}
			}
		}
	}
	else if (magic_GUI_state == 1)
	{
		//TODO: Spell editor.
	}
}

/*-------------------------------------------------------------------------------

	updateMagicGUI

	Handles all buttons and such.
	If the spell list is open, it "equips" spells the player clicks on.
	If the spell editor is open, well, there's a lot going on there, and it's
	updating all that.

-------------------------------------------------------------------------------*/

void updateMagicGUI()
{
	/*if (!spellList) { //I woke up with this commented out for some reason. TODO: Look into it.
		return; //Can't continue without a spell list!
	}*/
	SDL_Rect pos;

	renderMagicGUI(camera.winx, camera.winy, camera.winw, camera.winh);
	if (magic_GUI_state == 0)   //TODO: use defines, not numbers.
	{
		if (mousestatus[SDL_BUTTON_LEFT])
		{
			//TODO: Check if a spell was clicked on.
			//TODO: Loop through all spells then run the if check below.
			int height = spell_list_titlebar_bmp->h;
			int numspells = 0;
			node_t* node;
			for (node = spellList.first; node != NULL; node = node->next)
			{
				numspells++;
			}
			int maxSpellsOnscreen = camera.winh / spell_list_gui_slot_bmp->h;
			numspells = std::min(numspells, maxSpellsOnscreen);
			height += numspells * spell_list_gui_slot_bmp->h;
			//Now calculate the position.
			pos.x = camera.winx + (camera.winw / 2) - (spell_list_gui_slot_bmp->w / 2) + magicspell_list_offset_x;
			pos.y = camera.winy + ((camera.winh / 2) - (height / 2)) + magicspell_list_offset_x;
			magic_gui_pos.x = pos.x;
			magic_gui_pos.y = pos.y;

			pos.y += spell_list_titlebar_bmp->h;
			int i = 0;
			node = spellList.first; //This will be needed to grab the name of the spell when its slot is drawn.
			for (i = 0; i < spellscroll; ++i)
			{
				if (node)
				{
					node = node->next;
				}
			}

			for (i = 0; i < numspells; ++i)
			{
				if (node)   //If the node exists (so that there's no crashes midgame...though the node should not be null in the first place. If it is, you have a problem.
				{
					if (mouseInBounds(pos.x, pos.x + spell_list_gui_slot_bmp->w, pos.y, pos.y + spell_list_gui_slot_bmp->h))
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						spell_t* spell = (spell_t*)node->element;
						if (spell)
						{
							equipSpell(spell, clientnum);
							//selected_spell = spell;
						}
					}
					pos.y += spell_list_gui_slot_bmp->h;
					node = node->next;
				}
			}
		}
	}
	else if (magic_GUI_state == 1)
	{
		//TODO: Spell editor.
	}
}

void drawSustainedSpells()
{
	SDL_Surface** sprite;
	SDL_Rect pos;
	pos.x = SUST_SPELLS_X;
	pos.y = SUST_SPELLS_Y;
	if (SUST_SPELLS_RIGHT_ALIGN)
	{
		//Alright, so, the list should be right-aligned.
		//Meaning, it draws alongside the right side of the screen.
		node_t* node = list_Node(&items[SPELL_ITEM].surfaces, 1); //Use any old sprite icon as a reference to calculate the position.
		if (!node)
		{
			return;
		}
		SDL_Surface** surface = (SDL_Surface**)node->element;
		pos.x = camera.winw - (*surface)->w - SUST_SPELLS_X;
		//Draw under the skills sheet if inventory open or the sidebar lock has been enabled.
		pos.y = 32 + ( (!shootmode || lock_right_sidebar) ? (NUMPROFICIENCIES * TTF12_HEIGHT) + (TTF12_HEIGHT * 3) : 0); 
	}

	bool isChanneled = false;
	char* tooltipText = nullptr;
	char* currentTooltip = nullptr;
	TTF_Font* fontText = ttf12;
	int fontHeight = TTF12_HEIGHT;
	int fontWidth = TTF12_WIDTH;
	if ( uiscale_skillspage )
	{
		fontText = ttf16;
		fontHeight = TTF16_HEIGHT;
		fontWidth = TTF16_WIDTH;
	}

	for ( int i = 0; showStatusEffectIcons && i < NUMEFFECTS && stats[clientnum]; ++i )
	{
		node_t* effectImageNode = nullptr;
		sprite = nullptr;
		tooltipText = nullptr;
		if ( stats[clientnum]->EFFECTS[i] )
		{
			switch ( i )
			{
				case EFF_SLOW:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_SLOW);
					tooltipText = language[3384];
					break;
				case EFF_BLEEDING:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_BLEED);
					tooltipText = language[3385];
					break;
				case EFF_ASLEEP:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_SLEEP);
					tooltipText = language[3386];
					break;
				case EFF_CONFUSED:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_CONFUSE);
					tooltipText = language[3387];
					break;
				case EFF_PACIFY:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_CHARM_MONSTER);
					tooltipText = language[3388];
					break;
				case EFF_VAMPIRICAURA:
				{
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_VAMPIRIC_AURA);
					tooltipText = language[3389];
					node_t* node = channeledSpells[clientnum].first;
					for ( ; node != nullptr; node = node->next )
					{
						spell_t* spell = (spell_t*)node->element;
						if ( spell && spell->ID == SPELL_VAMPIRIC_AURA )
						{
							tooltipText = language[3390];
							break;
						}
					}
					break;
				}
				case EFF_PARALYZED:
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_LIGHTNING);
					tooltipText = language[3391];
					break;
				case EFF_DRUNK:
					if ( effect_drunk_bmp )
					{
						sprite = &effect_drunk_bmp;
					}
					tooltipText = language[3392];
					break;
				case EFF_POLYMORPH:
					if ( effect_polymorph_bmp )
					{
						sprite = &effect_polymorph_bmp;
					}
					tooltipText = language[3399];
					break;
				case EFF_WITHDRAWAL:
					if ( effect_hungover_bmp )
					{
						sprite = &effect_hungover_bmp;
					}
					tooltipText = language[3393];
					break;
				case EFF_POTION_STR:
					sprite = &str_bmp64u;
					tooltipText = language[3394];
					break;
				case EFF_LEVITATING:
				{
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_LEVITATION);
					node_t* node = channeledSpells[clientnum].first;
					for ( ; node != nullptr; node = node->next )
					{
						spell_t* spell = (spell_t*)node->element;
						if ( spell && spell->ID == SPELL_LEVITATION )
						{
							effectImageNode = nullptr;
							break;
						}
					}
					tooltipText = language[3395];
					break;
				}
				case EFF_INVISIBLE:
				{
					effectImageNode = list_Node(&items[SPELL_ITEM].surfaces, SPELL_INVISIBILITY);
					node_t* node = channeledSpells[clientnum].first;
					for ( ; node != nullptr; node = node->next )
					{
						spell_t* spell = (spell_t*)node->element;
						if ( spell && spell->ID == SPELL_INVISIBILITY )
						{
							effectImageNode = nullptr;
							break;
						}
					}
					tooltipText = language[3396];
					break;
				}
				default:
					effectImageNode = nullptr;
					tooltipText = nullptr;
					break;
			}
		}

		if ( tooltipText && !shootmode && mouseInBounds(pos.x, pos.x + 32,
			pos.y, pos.y + 32) )
		{
			// draw tooltip.
			currentTooltip = tooltipText;
		}

		if ( effectImageNode || sprite )
		{
			if ( !sprite )
			{
				sprite = (SDL_Surface**)effectImageNode->element;
			}

			bool lowDurationFlash = !((ticks % 50) - (ticks % 25));
			bool lowDuration = stats[clientnum]->EFFECTS_TIMERS[i] > 0 && 
				(stats[clientnum]->EFFECTS_TIMERS[i] < TICKS_PER_SECOND * 5);
			if ( i == EFF_VAMPIRICAURA )
			{
				lowDuration = false;
			}
			if ( (lowDuration && !lowDurationFlash) || !lowDuration )
			{
				if ( sprite == &str_bmp64u )
				{
					pos.h = 32;
					pos.w = 32;
					drawImageScaled(*sprite, NULL, &pos);
				}
				else
				{
					drawImage(*sprite, NULL, &pos);
				}
			}

			if ( SUST_SPELLS_DIRECTION == SUST_DIR_HORZ && !SUST_SPELLS_RIGHT_ALIGN )
			{
				pos.x += sustained_spell_generic_icon->w;
			}
			else
			{
				//Vertical.
				pos.y += (*sprite)->h;
			}
			effectImageNode = nullptr;
		}
	}

	if (!channeledSpells[clientnum].first)
	{
		if ( currentTooltip )
		{
			SDL_Rect tooltip;
			std::string str = currentTooltip;
			size_t n = std::count(str.begin(), str.end(), '\n'); // count newlines
			tooltip.w = ((longestline(currentTooltip) + 2) * fontWidth) + 8;
			if ( n > 4 )
			{
				tooltip.h = fontHeight * (n + 1) + 16;
			}
			else
			{
				tooltip.h = fontHeight * (n + 1) + 12;
			}
			tooltip.x = mousex - 16 - tooltip.w;
			tooltip.y = mousey + 8;
			drawTooltip(&tooltip);
			size_t found = str.find('\n');
			if ( found != std::string::npos )
			{
				char buf[256] = "";
				strncpy(buf, str.c_str(), found);
				ttfPrintTextFormatted(fontText, tooltip.x + 4, tooltip.y + 4, buf);
				str = str.substr(found);
				strncpy(buf, str.c_str(), str.length());
				ttfPrintTextFormattedColor(fontText, tooltip.x + 4, tooltip.y + 4, uint32ColorLightBlue(*mainsurface), buf);
			}
			else
			{
				ttfPrintTextFormatted(fontText, tooltip.x + 4, tooltip.y + 4, currentTooltip);
			}
		}
		return;    //No use continuing if there are no sustained spells.
	}


	int count = 0; //This is just for debugging purposes.
	node_t* node = channeledSpells[clientnum].first;
	for (; node; node = node->next, count++)
	{
		tooltipText = nullptr;
		spell_t* spell = (spell_t*)node->element;
		if (!spell)
		{
			break;
		}
		//Grab the sprite/
		if ( spell->ID == SPELL_VAMPIRIC_AURA )
		{
			continue;
		}

		if ( spell->ID == SPELL_INVISIBILITY )
		{
			tooltipText = language[3396];
		}
		else if ( spell->ID == SPELL_LEVITATION )
		{
			tooltipText = language[3395];
		}
		else if ( spell->ID == SPELL_REFLECT_MAGIC )
		{
			tooltipText = language[3397];
		}
		else if ( spell->ID == SPELL_LIGHT )
		{
			tooltipText = language[3398];
		}

		node_t* node = list_Node(&items[SPELL_ITEM].surfaces, spell->ID);
		if (!node)
		{
			break;
		}
		sprite = (SDL_Surface**)node->element;

		drawImage(*sprite, NULL, &pos);

		if ( !shootmode && mouseInBounds(pos.x, pos.x + 32, pos.y, pos.y + 32) && tooltipText )
		{
			// draw tooltip.
			currentTooltip = tooltipText;
		}

		if (SUST_SPELLS_DIRECTION == SUST_DIR_HORZ && !SUST_SPELLS_RIGHT_ALIGN)
		{
			pos.x += sustained_spell_generic_icon->w;
		}
		else
		{
			//Vertical.
			pos.y += (*sprite)->h;
		}
	}

	if ( currentTooltip )
	{
		SDL_Rect tooltip;
		std::string str = currentTooltip;
		size_t n = std::count(str.begin(), str.end(), '\n'); // count newlines
		tooltip.w = ((longestline(currentTooltip) + 1) * fontWidth) + 8;
		if ( n > 4 )
		{
			tooltip.h = fontHeight * (n + 1) + 12;
		}
		else
		{
			tooltip.h = fontHeight * (n + 1) + 8;
		}
		tooltip.x = mousex - 16 - tooltip.w;
		tooltip.y = mousey + 8;
		drawTooltip(&tooltip);
		size_t found = str.find('\n');
		if ( found != std::string::npos )
		{
			char buf[256] = "";
			strncpy(buf, str.c_str(), found);
			ttfPrintTextFormatted(fontText, tooltip.x + 4, tooltip.y + 4, buf);
			str = str.substr(found);
			strncpy(buf, str.c_str(), str.length());
			ttfPrintTextFormattedColor(fontText, tooltip.x + 4, tooltip.y + 4, uint32ColorLightBlue(*mainsurface), buf);
		}
		else
		{
			ttfPrintTextFormatted(fontText, tooltip.x + 4, tooltip.y + 4, currentTooltip);
		}
	}
}
