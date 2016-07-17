/*-------------------------------------------------------------------------------

	BARONY
	File: updatecharactersheet.cpp
	Desc: contains updateCharacterSheet()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "interface.hpp"
#include "../items.hpp"

/*-------------------------------------------------------------------------------

	updateCharacterSheet
	
	Draws the character sheet and processes all interaction with it

-------------------------------------------------------------------------------*/

void updateCharacterSheet() {
	int i = 0;
	int x = 0;
	SDL_Rect pos;
	bool b = FALSE;
	node_t *node = NULL;
	Entity *entity = NULL;
	Item *item = NULL;
	int c;
	
	// draw window
	drawWindowFancy(0, 0, 224, 196);
	pos.x=8; pos.y=8;
	pos.w=208; pos.h=180;
	drawRect(&pos,0,255);
	//drawImage(character_bmp, NULL, &pos);
	//pos.x=0; pos.y=196;
	//pos.w=222; pos.h=392-196;
	//drawTooltip(&pos);
	drawWindowFancy(0, 196, 224, 392);
	drawWindowFancy(attributesleft_bmp->w, 392, 224-attributesleft_bmp->w, 420);

	if( mousestatus[SDL_BUTTON_LEFT] ) {
		if( omousey>=392 && omousey<420 ) {
			if( omousex>=0 && omousex<attributesleft_bmp->w ) {
				mousestatus[SDL_BUTTON_LEFT]=0;
				attributespage = std::max(attributespage-1,0);
				buttonclick=5;
			}
			if( omousex>=224-attributesright_bmp->w && omousex<224 ) {
				mousestatus[SDL_BUTTON_LEFT]=0;
				attributespage = std::min(attributespage+1,2);
				buttonclick=6;
			}
		}
	}
	//Attributes page left button.
	if( buttonclick==5 ) {
		pos.x=0; pos.y=392;
		pos.w=0; pos.h=0;
		drawImage(attributesleft_bmp, NULL, &pos);
	} else {
		pos.x=0; pos.y=392;
		pos.w=0; pos.h=0;
		drawImage(attributesleftunclicked_bmp, NULL, &pos);
	}
	//Attributes page right button.
	if( buttonclick==6 ) {
		pos.x=224-attributesright_bmp->w; pos.y=392;
		pos.w=0; pos.h=0;
		drawImage(attributesright_bmp, NULL, &pos);
	} else {
		pos.x=224-attributesrightunclicked_bmp->w; pos.y=392;
		pos.w=0; pos.h=0;
		drawImage(attributesrightunclicked_bmp, NULL, &pos);
	}

	// character sheet
	double ofov = fov;
	fov = 50;
	if( players[clientnum] != NULL ) {
		if( !softwaremode )
			glClear( GL_DEPTH_BUFFER_BIT );
		//camera.x=players[clientnum]->x/16.0+.5*cos(players[clientnum]->yaw)-.4*sin(players[clientnum]->yaw);
		//camera.y=players[clientnum]->y/16.0+.5*sin(players[clientnum]->yaw)+.4*cos(players[clientnum]->yaw);
		camera_charsheet.x=players[clientnum]->x/16.0+.65;
		camera_charsheet.y=players[clientnum]->y/16.0+.65;
		camera_charsheet.z=players[clientnum]->z*2;
		//camera.ang=atan2(players[clientnum]->y/16.0-camera.y,players[clientnum]->x/16.0-camera.x);
		camera_charsheet.ang=5*PI/4;
		camera_charsheet.vang=PI/20;
		camera_charsheet.winx=8;
		camera_charsheet.winy=8;
		camera_charsheet.winw=208;
		camera_charsheet.winh=180;
		b=players[clientnum]->flags[BRIGHT];
		players[clientnum]->flags[BRIGHT]=TRUE;
		if( !players[clientnum]->flags[INVISIBLE] ) {
			glDrawVoxel(&camera_charsheet,players[clientnum],REALCOLORS);
		}
		players[clientnum]->flags[BRIGHT]=b;
		c=0;
		if( multiplayer!=CLIENT ) {
			for( node=players[clientnum]->children.first; node!=NULL; node=node->next ) {
				if( c==0 ) {
					c++;
					continue;
				}
				entity = (Entity *) node->element;
				if( !entity->flags[INVISIBLE] ) {
					b=entity->flags[BRIGHT];
					entity->flags[BRIGHT]=TRUE;
					glDrawVoxel(&camera_charsheet,entity,REALCOLORS);
					entity->flags[BRIGHT]=b;
				}
				c++;
			}
			for( node=map.entities->first; node!=NULL; node=node->next ) {
				entity = (Entity *) node->element;
				if( (Sint32)entity->uid == -4 ) {
					glDrawSprite(&camera_charsheet,entity,REALCOLORS);
				}
			}
		} else {
			for( node=map.entities->first; node!=NULL; node=node->next ) {
				entity = (Entity *) node->element;
				if( (entity->behavior == &actPlayerLimb && entity->skill[2] == clientnum && !entity->flags[INVISIBLE]) || (Sint32)entity->uid==-4 ) {
					b=entity->flags[BRIGHT];
					entity->flags[BRIGHT]=TRUE;
					if( (Sint32)entity->uid==-4 ) {
						glDrawSprite(&camera_charsheet,entity,REALCOLORS);
					} else {
						glDrawVoxel(&camera_charsheet,entity,REALCOLORS);
					}
					entity->flags[BRIGHT]=b;
				}
			}
		}
	}
	fov = ofov;

	ttfPrintTextFormatted(ttf12,8,202,"%s",stats[clientnum]->name);
	ttfPrintTextFormatted(ttf12,8,214,language[359],stats[clientnum]->LVL,language[1900+client_classes[clientnum]]);
	ttfPrintTextFormatted(ttf12,8,226,language[360],stats[clientnum]->EXP);
	ttfPrintTextFormatted(ttf12,8,238,language[361],currentlevel);

	// attributes
	if( attributespage==0 ) {
		ttfPrintTextFormatted(ttf12,8,262,language[1200],statGetSTR(stats[clientnum]),stats[clientnum]->STR);
		ttfPrintTextFormatted(ttf12,8,274,language[1201],statGetDEX(stats[clientnum]),stats[clientnum]->DEX);
		ttfPrintTextFormatted(ttf12,8,286,language[1202],statGetCON(stats[clientnum]),stats[clientnum]->CON);
		ttfPrintTextFormatted(ttf12,8,298,language[1203],statGetINT(stats[clientnum]),stats[clientnum]->INT);
		ttfPrintTextFormatted(ttf12,8,310,language[1204],statGetPER(stats[clientnum]),stats[clientnum]->PER);
		ttfPrintTextFormatted(ttf12,8,322,language[1205],statGetCHR(stats[clientnum]),stats[clientnum]->CHR);
	} else {
		ttfPrintTextFormatted(ttf12,8,262,language[1883]);

		// skills (page one)
		if( attributespage==1 ) {
			for( c=0; c<NUMPROFICIENCIES/2; c++ ) {
				ttfPrintTextFormatted(ttf12,8,286+c*12,"%s:",language[236+c]);
			}
		}

		// skills (page two)
		else if( attributespage==2 ) {
			for( c=NUMPROFICIENCIES/2; c<NUMPROFICIENCIES; c++ ) {
				ttfPrintTextFormatted(ttf12,8,286+(c-NUMPROFICIENCIES/2)*12,"%s:",language[236+c]);
			}
		}
	}

	// skill levels
	if( attributespage>0 ) {
		for( i=(NUMPROFICIENCIES/2)*(attributespage-1); i<(NUMPROFICIENCIES/2)*attributespage; i++ ) {
			if( stats[clientnum]->PROFICIENCIES[i] == 0 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[363]);
			else if( stats[clientnum]->PROFICIENCIES[i] < 20 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[364]);
			else if( stats[clientnum]->PROFICIENCIES[i] >= 20 && stats[clientnum]->PROFICIENCIES[i] < 40 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[365]);
			else if( stats[clientnum]->PROFICIENCIES[i] >= 40 && stats[clientnum]->PROFICIENCIES[i] < 60 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[366]);
			else if( stats[clientnum]->PROFICIENCIES[i] >= 60 && stats[clientnum]->PROFICIENCIES[i] < 80 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[367]);
			else if( stats[clientnum]->PROFICIENCIES[i] >= 80 && stats[clientnum]->PROFICIENCIES[i] < 100 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[368]);
			else if( stats[clientnum]->PROFICIENCIES[i] >= 100 )
				ttfPrintTextFormatted(ttf12,8,286+(i%(NUMPROFICIENCIES/2))*12,language[369]);
		}
	}

	// armor, gold, and weight
	if( attributespage==0 ) {
		ttfPrintTextFormatted(ttf12,8,346,language[370],stats[clientnum]->GOLD);
		ttfPrintTextFormatted(ttf12,8,358,language[371],AC(stats[clientnum]));
		Uint32 weight=0;
		for( node=stats[clientnum]->inventory.first; node!=NULL; node=node->next ) {
			item = (Item *)node->element;
			weight += items[item->type].weight*item->count;
		}
		weight+=stats[clientnum]->GOLD/100;
		ttfPrintTextFormatted(ttf12,8,370,language[372],weight);
	}
	printTextFormatted(font12x12_bmp,112-30,420-12-8,"%d / 3",attributespage+1); // attributes pane page number
}