/*-------------------------------------------------------------------------------

	BARONY
	File: drawminimap.cpp
	Desc: contains drawMinimap()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../engine/audio/sound.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../collision.hpp"
#include "../mod_tools.hpp"
#include "../ui/Image.hpp"

/*-------------------------------------------------------------------------------

	drawMinimap

	Draws the game's minimap in the lower right corner of the screen

-------------------------------------------------------------------------------*/

Uint32 minotaur_timer = 0;
std::vector<MinimapPing> minimapPings[MAXPLAYERS];
int minimapPingGimpTimer[MAXPLAYERS] = { 0 };
Uint32 lastMapTick = 0;
SDL_Rect minimaps[MAXPLAYERS];

void drawMinimap(const int player, SDL_Rect rect)
{
	if (gameplayCustomManager.inUse()) {
		if (CustomHelpers::isLevelPartOfSet(
		    currentlevel, secretlevel, gameplayCustomManager.minimapDisableFloors)) {
			return;
		}
	}

	int numplayers = 0;
	for (int i = 0; i < MAXPLAYERS; ++i) {
		if ( players[i]->isLocalPlayer() ) {
			++numplayers;
		}
	}

	Input& input = Input::inputs[player];
	const int windowLCD = std::min(rect.w, rect.h);
	const int windowGCD = std::max(rect.w, rect.h);
	const int mapLCD = std::min(map.width, map.height);
	const int mapGCD = std::max(map.width, map.height);
    const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
    const real_t unitY = (real_t)rect.h / (real_t)mapGCD;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0, 0, Frame::virtualScreenX, Frame::virtualScreenY);
	glOrtho(0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// create a new minimap texture
	SDL_Surface* minimapSurface = SDL_CreateRGBSurface(0, mapGCD, mapGCD, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	TempTexture* minimapTexture = new TempTexture();
	SDL_LockSurface(minimapSurface);

	const int xmin = ((int)map.width - mapGCD) / 2;
	const int xmax = map.width - xmin;
	const int ymin = ((int)map.height - mapGCD) / 2;
	const int ymax = map.height - ymin;
	for ( int x = xmin; x < xmax; ++x ) {
		for ( int y = ymin; y < ymax; ++y ) {
		    Uint32 color;
		    Uint8 backgroundAlpha = 255 * ((100 - minimapTransparencyBackground) / 100.f);
		    Uint8 foregroundAlpha = 255 * ((100 - minimapTransparencyForeground) / 100.f);
			if (x < 0 || y < 0 || x >= map.width || y >= map.height)
			{
			    // out-of-bounds
			    color = makeColor(0, 64, 64, backgroundAlpha);
			}
			else if ( minimap[y][x] == 0 )
			{
			    // unknown / no floor
				color = makeColor(0, 64, 64, backgroundAlpha);
			}
			else if ( minimap[y][x] == 1 )
			{
			    // walkable space
				color = makeColor(0, 128, 128, foregroundAlpha);
			}
			else if ( minimap[y][x] == 2 )
			{
			    // wall
				color = makeColor(0, 255, 255, foregroundAlpha);
			}
			else if ( minimap[y][x] == 3 )
			{
			    // mapped but undiscovered walkable ground
				color = makeColor(64, 64, 64, foregroundAlpha);
			}
			else if ( minimap[y][x] == 4 )
			{
			    // mapped but undiscovered wall
				color = makeColor(128, 128, 128, foregroundAlpha);
			}
			putPixel(minimapSurface, x - xmin, y - ymin, color);
		}
	}

	SDL_UnlockSurface(minimapSurface);
	minimapTexture->load(minimapSurface, false, true);

	minimapTexture->bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(rect.x, Frame::virtualScreenY - rect.y);
	glTexCoord2f(0, 1);
	glVertex2f(rect.x, Frame::virtualScreenY - (rect.y + rect.h));
	glTexCoord2f(1, 1);
	glVertex2f(rect.x + rect.w, Frame::virtualScreenY - (rect.y + rect.h));
	glTexCoord2f(1, 0);
	glVertex2f(rect.x + rect.w, Frame::virtualScreenY - rect.y);
	glEnd();
	if (minimapTexture) {
		delete minimapTexture;
		minimapTexture = nullptr;
	}
	if (minimapSurface) {
		SDL_FreeSurface(minimapSurface);
		minimapSurface = nullptr;
	}

	// bind a solid white texture
	auto white = Image::get("images/system/white.png");
	white->bind();

	// build a circle mesh
	static std::vector<std::pair<real_t, real_t>> circle_mesh;
	if (!circle_mesh.size()) {
	    circle_mesh.emplace_back((real_t)0.0, (real_t)0.0);
	    static const int num_circle_vertices = 12;
	    for (int c = 0; c <= num_circle_vertices; ++c) {
	        real_t ang = ((PI * 2.0) / num_circle_vertices) * c;
	        circle_mesh.emplace_back((real_t)(cos(ang) / 2.0), -(real_t)(sin(ang) / 2.0));
	    }
	}

	auto drawCircleMesh = [](real_t x, real_t y, SDL_Rect rect, Uint32 color){
	    const int windowLCD = std::min(rect.w, rect.h);
	    const int windowGCD = std::max(rect.w, rect.h);
	    const int mapLCD = std::min(map.width, map.height);
	    const int mapGCD = std::max(map.width, map.height);
	    const int xmin = ((int)map.width - mapGCD) / 2;
	    const int ymin = ((int)map.height - mapGCD) / 2;
        const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
        const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
        x = (x - xmin) * unitX + rect.x;
        y = (y - ymin) * unitY + rect.y;
		Uint8 r, g, b, a;
		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
        glBegin(GL_TRIANGLE_FAN);
        for (auto& pair : circle_mesh) {
            const real_t sx = pair.first * unitX * (minimapObjectZoom / 100.0);
            const real_t sy = pair.second * unitY * (minimapObjectZoom / 100.0);
            glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
        }
        glEnd();
	};

	// draw special points of interest (exits, items, revealed monsters, etc)
	for ( node_t* node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->sprite == 161 || (entity->sprite >= 254 && entity->sprite < 258)
			|| entity->behavior == &actCustomPortal )   // ladder or portal models
		{
			if ( entity->x >= 0 && entity->y >= 0 && entity->x < map.width << 4 && entity->y < map.height << 4 )
			{
				int x = floor(entity->x / 16);
				int y = floor(entity->y / 16);
				if ( minimap[y][x] || (entity->entityShowOnMap > 0 && !(entity->behavior == &actCustomPortal)) )
				{
					if ( ticks % 40 - ticks % 20 )
					{
					    // exit
			            drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(255, 0, 0, 255));
					}
				}
			}
		}
		else
		{
			if ( entity->skill[28] > 0 ) // mechanism
			{
				if ( entity->behavior == &actCustomPortal
					|| entity->behavior == &actTextSource
					|| entity->behavior == &actFloorDecoration )
				{
					continue;
				}
			}
			if ( entity->behavior == &actMonster && entity->monsterAllyIndex < 0 )
			{
				bool warningEffect = false;
				if ( (players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID())
					|| (entity->getStats() && entity->getStats()->EFFECTS[EFF_SHADOW_TAGGED]) )
				{
					warningEffect = true;
				    int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				    int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
			        drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(191, 191, 191, 255));
				}
				if ( !warningEffect 
					&& ((stats[player]->ring && stats[player]->ring->type == RING_WARNING)
						|| (entity->entityShowOnMap > 0)) )
				{
					int beatitude = 0;
					if ( stats[player]->ring && stats[player]->ring->type == RING_WARNING )
					{
						beatitude = stats[player]->ring->beatitude;
						// invert for succ/incubus
						if ( beatitude < 0 && shouldInvertEquipmentBeatitude(stats[player]) )
						{
							beatitude = abs(stats[player]->ring->beatitude);
						}
					}

					bool doEffect = false;
					if ( entity->entityShowOnMap > 0 )
					{
						doEffect = true;
					}
					else if ( stats[player]->ring && players[player] && players[player]->entity
						&& entityDist(players[player]->entity, entity) < 16.0 * std::max(3, (11 + 5 * beatitude)) )
					{
						doEffect = true;
					}
					if ( doEffect )
					{
				        int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				        int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
			            drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(191, 127, 191, 255));
						warningEffect = true;
					}
				}
				if ( !warningEffect && stats[player]->shoes != NULL )
				{
					if ( stats[player]->shoes->type == ARTIFACT_BOOTS )
					{
						if ( (abs(entity->vel_x) > 0.1 || abs(entity->vel_y) > 0.1)
							&& players[player] && players[player]->entity
							&& entityDist(players[player]->entity, entity) < 16.0 * 20 )
						{
							entity->entityShowOnMap = std::max(entity->entityShowOnMap, TICKS_PER_SECOND * 5);
				            int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				            int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
			                drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(191, 127, 191, 255));
						}
					}
				}
			}
			else if ( entity->isBoulderSprite() )
			{
				int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( minimap[y][x] == 1 || minimap[y][x] == 2 )
				{
				    // boulder
			        drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(191, 63, 0, 255));
				}
			}
			else if ( entity->behavior == &actItem && entity->itemShowOnMap == 1 )
			{
				int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( ticks % 40 - ticks % 20 )
				{
				    // item
			        drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(240, 228, 66, 255));
				}
			}
			else if ( entity->entityShowOnMap > 0 )
			{
				int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( ticks % 40 - ticks % 20 )
				{
			        drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, rect, makeColor(255, 168, 200, 255));
				}
			}
		}
		if ( entity->entityShowOnMap > 0 && lastMapTick != ticks ) 
		{
			// only decrease the entities' shown duration when the global game timer passes a tick
			// (drawMinimap doesn't follow game tick intervals)
			--entity->entityShowOnMap;
		}
	}
	lastMapTick = ticks;

	// draw player pings
	if ( !minimapPings[player].empty() )
	{
	    int minimapTotalScale = minimapScale;
		for ( std::vector<MinimapPing>::iterator it = minimapPings[player].begin(); it != minimapPings[player].end();)
		{
			MinimapPing ping = *it;

			int aliveTime = ticks - ping.tickStart;
			if ( aliveTime < TICKS_PER_SECOND * 2.5 ) // 2.5 second duration.
			{
				if ( (aliveTime < TICKS_PER_SECOND && (aliveTime % 10 < 5)) || aliveTime >= TICKS_PER_SECOND || ping.radiusPing )
				{
					// draw the ping blinking every 5 ticks if less than 1 second lifetime, otherwise constantly draw.
					Uint8 alpha = ping.radiusPing ? 50 : 255;
					if ( aliveTime >= TICKS_PER_SECOND * 2 )
					{
						// start fading ping after 2 seconds, lasting 0.5 seconds.
						real_t alphafade = 1 - (aliveTime - TICKS_PER_SECOND * 2) / static_cast<real_t>(TICKS_PER_SECOND * 0.5);
						alpha = std::max((int)(alphafade * alpha), 0);
					}

					// set color
		            Uint32 color;
			        switch ( ping.player )
			        {
				        case 0:
					        color = makeColor(64, 255, 64, alpha); // green
					        break;
				        case 1:
					        color = makeColor(86, 180, 233, alpha); // sky blue
					        break;
				        case 2:
					        color = makeColor(240, 228, 66, alpha); // yellow
					        break;
				        case 3:
					        color = makeColor(204, 121, 167, alpha); // pink
					        break;
				        default:
					        color = makeColor(192, 192, 192, alpha); // grey
					        break;
			        }

					// draw a circle
			        drawCircleMesh((real_t)ping.x + 0.5, (real_t)ping.y + 0.5, rect, color);
				}
			}


			// prune old pings > 2.5 seconds
			if ( aliveTime > TICKS_PER_SECOND * 2.5 )
			{
				if ( ping.player == player )
				{
					if ( minimapPingGimpTimer[player] > TICKS_PER_SECOND / 4 )
					{
						minimapPingGimpTimer[player] = TICKS_PER_SECOND / 4; // reduce the gimp timer when one of the player's own pings fades.
					}
				}
				it = minimapPings[player].erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// draw minotaur, players, and allies
	for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		int drawMonsterAlly = -1;
		int foundplayer = -1;
		if ( entity->behavior == &actPlayer )
		{
			foundplayer = entity->skill[2];
		}
		else if ( entity->behavior == &actMonster )
		{
			if ( entity->monsterAllyIndex >= 0 )
			{
				drawMonsterAlly = entity->monsterAllyIndex;
			}
		}
		if ( drawMonsterAlly >= 0 || foundplayer >= 0 || entity->sprite == 239)
		{
			Uint32 color = makeColor(255, 0, 0, 255);

			if (entity->sprite == 239)
			{
			    if (players[player] == nullptr)
			    {
			        continue;
			    }
			    if ( ticks % 120 - ticks % 60 )
			    {
				    if ( !minotaur_timer )
				    {
					    playSound(116, 64);
				    }
				    minotaur_timer = 1;
				}
				else
				{
				    minotaur_timer = 0;
				}
			}

			// my player = green, other players = blue
			if ( foundplayer >= 0 ) {
				switch ( foundplayer ) {
					case 0:
						color = makeColor(64, 255, 64, 255); // green
						break;
					case 1:
						color = makeColor(86, 180, 233, 255); // sky blue
						break;
					case 2:
						color = makeColor(240, 228, 66, 255); // yellow
						break;
					case 3:
						color = makeColor(204, 121, 167, 255); // pink
						break;
					default:
						color = makeColor(192, 192, 192, 255); // grey
						break;
				}
				if ( players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() ) {
					color = makeColor(192, 192, 192, 255); // grey
				}
			} else if ( entity->sprite == 239 ) {
				color = makeColor(255, 0, 0, 255);
			} else {
				switch ( drawMonsterAlly ) {
					case 0:
						color = makeColor(32, 127, 32, 255); // green
						break;
					case 1:
						color = makeColor(43, 90, 116, 255); // sky blue
						break;
					case 2:
						color = makeColor(120, 114, 33, 255); // yellow
						break;
					case 3:
						color = makeColor(102, 60, 83, 255); // pink
						break;
					default:
						color = makeColor(96, 96, 96, 255); // grey
						break;
				}
				if ( players[player] && players[player]->entity
					&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() ) {
					color = makeColor(96, 96, 96, 255); // grey
				}
			}

            const real_t x = ((entity->x / 16.0) - xmin) * unitX + rect.x;
            const real_t y = ((entity->y / 16.0) - ymin) * unitY + rect.y;
            const real_t ang = entity->yaw;

            const real_t v[][2] = {
                {  1.0,  0.0 },
                { -0.5, -0.5 },
                { -0.5,  0.5 },
            };
            const int num_vertices = sizeof(v) / sizeof(v[0]);

			Uint8 r, g, b, a;
			getColor(color, &r, &g, &b, &a);
			glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	        glBegin(GL_TRIANGLES);
	        for (int c = 0; c < num_vertices; ++c) {
	            const real_t vx = v[c][0] * cos(ang) - v[c][1] * sin(ang);
	            const real_t vy = v[c][0] * sin(ang) + v[c][1] * cos(ang);
	            const real_t sx = vx * unitX * (minimapObjectZoom / 100.0);
	            const real_t sy = vy * unitY * (minimapObjectZoom / 100.0);
	            glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
	        }
	        glEnd();
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void minimapPingAdd(const int srcPlayer, const int destPlayer, MinimapPing newPing)
{
	int numPlayerPings = 0;

	if ( !players[destPlayer]->isLocalPlayer() )
	{
		return;
	}
	if ( !minimapPings[destPlayer].empty() )
	{
		for ( std::vector<MinimapPing>::iterator it = minimapPings[destPlayer].begin(); it != minimapPings[destPlayer].end();)
		{
			MinimapPing ping = *it;
			if ( ping.player == newPing.player && !newPing.radiusPing )
			{
				++numPlayerPings;
				// prune pings if too many by same player.
				if ( numPlayerPings > 3 )
				{
					if ( ping.player == srcPlayer )
					{
						// this is the player creating the sound source.
						minimapPingGimpTimer[srcPlayer] = TICKS_PER_SECOND * 3; // 3 second penalty for spam.
					}
					it = minimapPings[destPlayer].erase(it);
					continue;
				}
			}
			++it;
		}
	}
	if ( !minimapPingMute && !newPing.radiusPing )
	{
		if ( static_cast<Sint8>(newPing.player) == -1 
			|| (newPing.player == destPlayer) 
			|| (newPing.player >= 0 && newPing.player < MAXPLAYERS && !players[newPing.player]->isLocalPlayer()) )
		{
			// play once? splitscreen host will satisfy newPing.player == destPlayer
			// remote client sending a ping will satisfy !players[newPing.player]->isLocalPlayer()
			playSound(399, 64); 
		}
	}
	minimapPings[destPlayer].insert(minimapPings[destPlayer].begin(), newPing);
}
