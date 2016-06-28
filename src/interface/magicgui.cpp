/*-------------------------------------------------------------------------------

	BARONY
	File: magicgui.cpp
	Desc: contains the functions related to the magic GUI.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "interface.hpp"
#include "../items.hpp"
#include "../magic/magic.hpp"

/*-------------------------------------------------------------------------------

	renderMagicGUI
	
	draws all the magic related gui stuff

-------------------------------------------------------------------------------*/

void renderMagicGUI(int winx, int winy, int winw, int winh) {
	/*if (!spellList) { //I woke up with this commented out for some reason. TODO: Look into it.
		return; //Can't continue without a spell list!
	}*/
	SDL_Rect pos;
	//pos.x = MAGICSPELL_LIST_X; pos.y = MAGICSPELL_LIST_Y;
	pos.x = 0; pos.y = 0;
	pos.w = 0; pos.h = 0;

	if (magic_GUI_state == 0) { //TODO: use defines, not numbers.
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
		node_t *node;
		for (node = spellList.first; node != NULL; node = node->next) { //TODO: Create spellList. -- Done?
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
		for (i = 0; i < spellscroll; ++i) {
			if (node)
				node = node->next;
		}
		for (i = 0; i < numspells; ++i) {
			if (node) { //If the node exists (so that there's no crashes midgame...though the node should not be null in the first place. If it is, you have a problem.
				spell_t *spell = (spell_t*)node->element;
				if (spell) {
					//If the mouse is over the slot, then draw the highlighted version.
					if (mouseInBounds(pos.x, pos.x + spell_list_gui_slot_bmp->w, pos.y, pos.y + spell_list_gui_slot_bmp->h)) {
						drawImage(spell_list_gui_slot_highlighted_bmp, NULL, &pos);
					} else {
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
	} else if (magic_GUI_state == 1) {
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

void updateMagicGUI() {
	/*if (!spellList) { //I woke up with this commented out for some reason. TODO: Look into it.
		return; //Can't continue without a spell list!
	}*/
	SDL_Rect pos;

	renderMagicGUI(camera.winx, camera.winy, camera.winw, camera.winh);
	if (magic_GUI_state == 0) { //TODO: use defines, not numbers.
		if (mousestatus[SDL_BUTTON_LEFT]) {
			//TODO: Check if a spell was clicked on.
			//TODO: Loop through all spells then run the if check below.
			int height = spell_list_titlebar_bmp->h;
			int numspells = 0;
			node_t *node;
			for (node = spellList.first; node != NULL; node = node->next) {
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
			for (i = 0; i < spellscroll; ++i) {
				if (node)
					node = node->next;
			}

			for (i = 0; i < numspells; ++i) {
				if (node) { //If the node exists (so that there's no crashes midgame...though the node should not be null in the first place. If it is, you have a problem.
					if (mouseInBounds(pos.x, pos.x + spell_list_gui_slot_bmp->w, pos.y, pos.y + spell_list_gui_slot_bmp->h)) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						spell_t *spell = (spell_t*)node->element;
						if (spell) {
							equipSpell(spell, clientnum);
							//selected_spell = spell;
						}
					}
					pos.y += spell_list_gui_slot_bmp->h;
					node = node->next;
				}
			}
		}
	} else if (magic_GUI_state == 1) {
		//TODO: Spell editor.
	}
}

void drawSustainedSpells() {
	if (!channeledSpells[clientnum].first)
		return; //No use continuing if there are no sustained spells.

	SDL_Surface** sprite;

	SDL_Rect pos;
	pos.x = SUST_SPELLS_X;
	if (SUST_SPELLS_RIGHT_ALIGN) {
		//Alright, so, the list should be right-aligned.
		//Meaning, it draws alongside the right side of the screen.
		node_t *node = list_Node(&items[SPELL_ITEM].surfaces, 1); //Use any old sprite icon as a reference to calculate the position.
		if (!node) {
			return;
		}
		SDL_Surface **surface = (SDL_Surface **)node->element;
		pos.x = camera.winw - (*surface)->w - SUST_SPELLS_X;
	}
	pos.y = SUST_SPELLS_Y;

	int count = 0; //This is just for debugging purposes.
	node_t *node = channeledSpells[clientnum].first;
	for(; node; node = node->next, count++) {
		spell_t *spell = (spell_t*)node->element;
		if (!spell) {
			break;
		}
		//Grab the sprite/
		node_t *node = list_Node(&items[SPELL_ITEM].surfaces, spell->ID);
		if (!node) {
			break;
		}
		sprite = (SDL_Surface **)node->element;

		drawImage(*sprite, NULL, &pos);

		if (SUST_SPELLS_DIRECTION == SUST_DIR_HORZ && !SUST_SPELLS_RIGHT_ALIGN) {
			pos.x += sustained_spell_generic_icon->w;
		} else {
			//Vertical.
			pos.y += (*sprite)->h;
		}
	}
}