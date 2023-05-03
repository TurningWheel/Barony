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
#include "../colors.hpp"
#include "consolecommand.hpp"

/*-------------------------------------------------------------------------------

	drawMinimap

	Draws the game's minimap in the lower right corner of the screen

-------------------------------------------------------------------------------*/

Uint32 minotaur_timer = 0;
std::vector<MinimapPing> minimapPings[MAXPLAYERS];
int minimapPingGimpTimer[MAXPLAYERS] = { 0 };
Uint32 lastMapTick = 0;
SDL_Rect minimaps[MAXPLAYERS];

static TempTexture* minimapTextures[MAXPLAYERS] = { nullptr };
static SDL_Surface* minimapSurfaces[MAXPLAYERS] = { nullptr };

void cleanupMinimapTextures() {
	for (int c = 0; c < MAXPLAYERS; ++c) {
		if (minimapTextures[c]) {
			delete minimapTextures[c];
			minimapTextures[c] = nullptr;
		}
		if (minimapSurfaces[c]) {
			SDL_FreeSurface(minimapSurfaces[c]);
			minimapSurfaces[c] = nullptr;
		}
	}
}


inline real_t getMinimapZoom()
{
	return minimapObjectZoom + 50;
}

void drawMinimap(const int player, SDL_Rect rect, bool drawingSharedMap)
{
	if ( gameplayCustomManager.inUse() ) {
		if ( CustomHelpers::isLevelPartOfSet(
			currentlevel, secretlevel, gameplayCustomManager.minimapDisableFloors) ) {
			return;
		}
	}

	int numplayers = 0;
	for ( int i = 0; i < MAXPLAYERS; ++i ) {
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

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// create a new minimap image
	SDL_Surface* minimapSurface = SDL_CreateRGBSurface(0, mapGCD, mapGCD, 32,
		0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	assert(minimapSurface);
	SDL_LockSurface(minimapSurface);

	std::vector<Entity*> entityPointsOfInterest;
	std::unordered_set<int> customWalls;
	// get special points of interest (exits, items, revealed monsters, etc)
	for ( node_t* node = map.entities->first; node != NULL; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( entity->sprite == 161 || (entity->sprite >= 254 && entity->sprite < 258)
			|| entity->behavior == &actCustomPortal )   // ladder or portal models
		{
			entityPointsOfInterest.push_back(entity);
		}
		else if ( entity->behavior == &actColliderDecoration && entity->isColliderShownAsWallOnMinimap() )
		{
			if ( entity->x >= 0 && entity->y >= 0 && entity->x < map.width << 4 && entity->y < map.height << 4 )
			{
				int x = floor(entity->x / 16);
				int y = floor(entity->y / 16);
				customWalls.insert(x + y * 10000);
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
				entityPointsOfInterest.push_back(entity);
			}
			else if ( entity->isBoulderSprite() )
			{
				entityPointsOfInterest.push_back(entity);
			}
			else if ( entity->behavior == &actItem && entity->sprite >= items[TOOL_PLAYER_LOOT_BAG].index &&
				entity->sprite < (items[TOOL_PLAYER_LOOT_BAG].index + items[TOOL_PLAYER_LOOT_BAG].variations) )
			{
				entityPointsOfInterest.push_back(entity);
			}
			else if ( entity->behavior == &actItem && entity->itemShowOnMap == 1 )
			{
				entityPointsOfInterest.push_back(entity);
			}
			else if ( entity->entityShowOnMap > 0 )
			{
				entityPointsOfInterest.push_back(entity);
			}
		}
		if ( entity->entityShowOnMap > 0 && lastMapTick != ticks )
		{
			// only decrease the entities' shown duration when the global game timer passes a tick
			// (drawMinimap doesn't follow game tick intervals)
			--entity->entityShowOnMap;
		}
	}

	const int xmin = ((int)map.width - mapGCD) / 2;
	const int xmax = map.width - xmin;
	const int ymin = ((int)map.height - mapGCD) / 2;
	const int ymax = map.height - ymin;
	for ( int x = xmin; x < xmax; ++x ) {
		for ( int y = ymin; y < ymax; ++y ) {
			Uint32 color = 0;
			Uint8 backgroundAlpha = 255 * ((100 - minimapTransparencyBackground) / 100.f);
			Uint8 foregroundAlpha = 255 * ((100 - minimapTransparencyForeground) / 100.f);
			auto foundCustomWall = customWalls.find(x + y * 10000);
			if ( x < 0 || y < 0 || x >= map.width || y >= map.height )
			{
				// out-of-bounds
				color = makeColor(0, 64, 64, backgroundAlpha);
			}
			else
			{
				Sint8 mapIndex = minimap[y][x];
				if ( foundCustomWall != customWalls.end() )
				{
					if ( mapIndex == 1 ) { mapIndex = 2; } // force walkable to wall
					else if ( mapIndex == 3 ) { mapIndex = 4; } // force undiscovered walkable to wall
				}

				if ( mapIndex == 0 )
				{
					// unknown / no floor
					color = makeColor(0, 64, 64, backgroundAlpha);
				}
				else if ( mapIndex == 1 )
				{
					// walkable space
					color = makeColor(0, 128, 128, foregroundAlpha);
				}
				else if ( mapIndex == 2 )
				{
					// wall
					color = makeColor(0, 255, 255, foregroundAlpha);
				}
				else if ( mapIndex == 3 )
				{
					// mapped but undiscovered walkable ground
					color = makeColor(64, 64, 64, foregroundAlpha);
				}
				else if ( mapIndex == 4 )
				{
					// mapped but undiscovered wall
					color = makeColor(128, 128, 128, foregroundAlpha);
				}
			}
			putPixel(minimapSurface, x - xmin, y - ymin, color);
		}
	}

	// upload minimap image to an OpenGL texture, if it is different
	if ( minimapSurfaces[player] ) {
		SDL_LockSurface(minimapSurfaces[player]);
	}
	auto m1 = minimapSurface;
	auto m2 = minimapSurfaces[player];
	const auto size1 = m1->w * m1->h * m1->format->BytesPerPixel;
	const auto size2 = m2 ? (m2->w * m2->h * m2->format->BytesPerPixel) : 0;
	if ( size1 != size2 || memcmp(m1, m2, size2) ) {
		if ( !minimapTextures[player] ) {
			minimapTextures[player] = new TempTexture();
		}
		minimapTextures[player]->load(minimapSurface, false, true);
		if ( minimapSurfaces[player] ) {
			SDL_UnlockSurface(minimapSurfaces[player]);
			SDL_FreeSurface(minimapSurfaces[player]);
		}
		SDL_UnlockSurface(minimapSurface);
		minimapSurfaces[player] = minimapSurface;
	}
	else {
		if ( minimapSurfaces[player] ) {
			SDL_UnlockSurface(minimapSurfaces[player]);
		}
		SDL_UnlockSurface(minimapSurface);
		SDL_FreeSurface(minimapSurface);
		minimapSurface = nullptr;
	}
	minimapTextures[player]->bind();

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

	// bind a solid white texture
	auto white = Image::get("images/system/white.png");
	white->bind();

	// build a circle mesh
	static std::vector<std::pair<real_t, real_t>> circle_mesh;
	if ( !circle_mesh.size() ) {
		circle_mesh.emplace_back((real_t)0.0, (real_t)0.0);
		static const int num_circle_vertices = 32;
		for ( int c = 0; c <= num_circle_vertices; ++c ) {
			real_t ang = ((PI * 2.0) / num_circle_vertices) * c;
			circle_mesh.emplace_back((real_t)(sin(ang) / 2.0), (real_t)(cos(ang) / 2.0));
		}
	}

	auto drawCircleMesh = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
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
		for ( auto& pair : circle_mesh ) {
			const real_t sx = pair.first * unitX * (getMinimapZoom() / 100.0) * size;
			const real_t sy = pair.second * unitY * (getMinimapZoom() / 100.0) * size;
			glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
		}
		glEnd();
	};

	std::vector<std::pair<Uint32, std::pair<real_t, real_t>>> deathboxSkulls;

	auto drawSkull = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
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

		auto imgGet = Image::get("*images/ui/HUD/death_skull.png");
		imgGet->bind();

		Uint8 r, g, b, a;
		getColor(color, &r, &g, &b, &a);
		glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

		glBegin(GL_QUADS);

		const real_t sx = unitX * (getMinimapZoom() / 100.0) * size;
		const real_t sy = unitY * (getMinimapZoom() / 100.0) * size;


		SDL_Rect secondsrc;
		SDL_Rect src;
		src.x = 0;
		src.y = 0;
		src.w = imgGet->getSurf()->w;
		src.h = imgGet->getSurf()->h;

		SDL_Rect dest;
		dest.x = x - sx / 2;
		dest.y = y - sy / 2;
		dest.w = sx;
		dest.h = sy;

		SDL_Rect viewport{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY };

		glTexCoord2f(1.0 * ((real_t)src.x / imgGet->getSurf()->w), 1.0 * ((real_t)src.y / imgGet->getSurf()->h));
		glVertex2f(dest.x, viewport.h - dest.y);
		glTexCoord2f(1.0 * ((real_t)src.x / imgGet->getSurf()->w), 1.0 * (((real_t)src.y + src.h) / imgGet->getSurf()->h));
		glVertex2f(dest.x, viewport.h - dest.y - dest.h);
		glTexCoord2f(1.0 * (((real_t)src.x + src.w) / imgGet->getSurf()->w), 1.0 * (((real_t)src.y + src.h) / imgGet->getSurf()->h));
		glVertex2f(dest.x + dest.w, viewport.h - dest.y - dest.h);
		glTexCoord2f(1.0 * (((real_t)src.x + src.w) / imgGet->getSurf()->w), 1.0 * ((real_t)src.y / imgGet->getSurf()->h));
		glVertex2f(dest.x + dest.w, viewport.h - dest.y);
		glEnd();
	};

	// draw special points of interest (exits, items, revealed monsters, etc)
	for ( auto entity : entityPointsOfInterest )
	{
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
						drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(255, 0, 0, 255));
					}
				}
			}
		}
		else
		{
			if ( entity->behavior == &actMonster && entity->monsterAllyIndex < 0 )
			{
				bool warningEffect = false;
				{
					if ( drawingSharedMap )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( !players[i]->isLocalPlayer() || client_disconnected[i] ) { continue; }

							if ( (players[i] && players[i]->entity
								&& players[i]->entity->creatureShadowTaggedThisUid == entity->getUID())
								|| (entity->getStats() && entity->getStats()->EFFECTS[EFF_SHADOW_TAGGED]) )
							{
								warningEffect = true;
								int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
								int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
								drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 191, 191, 255));
								break;
							}
						}
					}
					else
					{
						const int i = player;
						if ( (players[i] && players[i]->entity
							&& players[i]->entity->creatureShadowTaggedThisUid == entity->getUID())
							|| (entity->getStats() && entity->getStats()->EFFECTS[EFF_SHADOW_TAGGED]) )
						{
							warningEffect = true;
							int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
							int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
							drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 191, 191, 255));
						}
					}

					if ( !warningEffect )
					{
						if ( drawingSharedMap )
						{
							for ( int i = 0; i < MAXPLAYERS; ++i )
							{
								if ( !players[i]->isLocalPlayer() || client_disconnected[i] ) { continue; }

								if ( (stats[i]->ring && stats[i]->ring->type == RING_WARNING)
									|| (entity->entityShowOnMap > 0) )
								{
									int beatitude = 0;
									if ( stats[i]->ring && stats[i]->ring->type == RING_WARNING )
									{
										beatitude = stats[i]->ring->beatitude;
										// invert for succ/incubus
										if ( beatitude < 0 && shouldInvertEquipmentBeatitude(stats[i]) )
										{
											beatitude = abs(stats[i]->ring->beatitude);
										}
									}

									bool doEffect = false;
									if ( entity->entityShowOnMap > 0 )
									{
										doEffect = true;
									}
									else if ( stats[i]->ring && players[i] && players[i]->entity
										&& entityDist(players[i]->entity, entity) < 16.0 * std::max(3, (11 + 5 * beatitude)) )
									{
										doEffect = true;
									}
									if ( doEffect )
									{
										int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
										int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
										drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 127, 191, 255));
										warningEffect = true;
										break;
									}
								}
							}
						}
						else
						{
							const int i = player;
							if ( (stats[i]->ring && stats[i]->ring->type == RING_WARNING)
									|| (entity->entityShowOnMap > 0) )
							{
								int beatitude = 0;
								if ( stats[i]->ring && stats[i]->ring->type == RING_WARNING )
								{
									beatitude = stats[i]->ring->beatitude;
									// invert for succ/incubus
									if ( beatitude < 0 && shouldInvertEquipmentBeatitude(stats[i]) )
									{
										beatitude = abs(stats[i]->ring->beatitude);
									}
								}

								bool doEffect = false;
								if ( entity->entityShowOnMap > 0 )
								{
									doEffect = true;
								}
								else if ( stats[i]->ring && players[i] && players[i]->entity
									&& entityDist(players[i]->entity, entity) < 16.0 * std::max(3, (11 + 5 * beatitude)) )
								{
									doEffect = true;
								}
								if ( doEffect )
								{
									int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
									int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
									drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 127, 191, 255));
									warningEffect = true;
								}
							}
						}
					}
					if ( !warningEffect )
					{
						if ( drawingSharedMap )
						{
							for ( int i = 0; i < MAXPLAYERS; ++i )
							{
								if ( !players[i]->isLocalPlayer() || client_disconnected[i] ) { continue; }

								if ( stats[i]->shoes != NULL )
								{
									if ( stats[i]->shoes->type == ARTIFACT_BOOTS )
									{
										if ( (abs(entity->vel_x) > 0.1 || abs(entity->vel_y) > 0.1)
											&& players[i] && players[i]->entity
											&& entityDist(players[i]->entity, entity) < 16.0 * 20 )
										{
											entity->entityShowOnMap = std::max(entity->entityShowOnMap, TICKS_PER_SECOND * 5);
											int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
											int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
											drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 127, 191, 255));
											warningEffect = true;
											break;
										}
									}
								}
							}
						}
						else
						{
							const int i = player;
							if ( stats[i]->shoes != NULL )
							{
								if ( stats[i]->shoes->type == ARTIFACT_BOOTS )
								{
									if ( (abs(entity->vel_x) > 0.1 || abs(entity->vel_y) > 0.1)
										&& players[i] && players[i]->entity
										&& entityDist(players[i]->entity, entity) < 16.0 * 20 )
									{
										entity->entityShowOnMap = std::max(entity->entityShowOnMap, TICKS_PER_SECOND * 5);
										int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
										int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
										drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 127, 191, 255));
										warningEffect = true;
									}
								}
							}
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
					drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(191, 63, 0, 255));
				}
			}
			else if ( entity->behavior == &actItem && entity->sprite >= items[TOOL_PLAYER_LOOT_BAG].index &&
				entity->sprite < (items[TOOL_PLAYER_LOOT_BAG].index + items[TOOL_PLAYER_LOOT_BAG].variations) )
			{
				real_t skullx = std::min<real_t>(std::max(0.0, entity->x / 16), map.width - 1);
				real_t skully = std::min<real_t>(std::max(0.0, entity->y / 16), map.height - 1);
				int playerOwner = entity->sprite - items[TOOL_PLAYER_LOOT_BAG].index;
				Uint32 color = playerColor(playerOwner, colorblind_lobby, false);
				deathboxSkulls.push_back(std::make_pair(color, std::make_pair(skullx, skully)));
			}
			else if ( entity->behavior == &actItem && entity->itemShowOnMap == 1 )
			{
				int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( ticks % 40 - ticks % 20 )
				{
					// item
					drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(240, 228, 66, 255));
				}
			}
			else if ( entity->entityShowOnMap > 0 )
			{
				int x = std::min<int>(std::max<int>(0, entity->x / 16), map.width - 1);
				int y = std::min<int>(std::max<int>(0, entity->y / 16), map.height - 1);
				if ( ticks % 40 - ticks % 20 )
				{
					drawCircleMesh((real_t)x + 0.5, (real_t)y + 0.5, (real_t)1.0, rect, makeColor(255, 168, 200, 255));
				}
			}
		}
	}

	static ConsoleVariable<float> cvar_skullscale("/minimap_skullscale", 1.75);
	for ( auto& skullInfo : deathboxSkulls )
	{
		Uint32 color = skullInfo.first;
		Uint8 r, g, b, a;
		getColor(color, &r, &g, &b, &a);
		const int glowRate = TICKS_PER_SECOND * 2;
		const int intensity = 4;
		if ( ticks % (glowRate) < (glowRate / 2) )
		{
			a = (255 - (glowRate / 2) * intensity) + (ticks % (glowRate)) * intensity;
		}
		else
		{
			a = 255 - ((ticks % (glowRate)) - (glowRate / 2)) * intensity;
		}
		color = makeColor(r, g, b, a);

		drawSkull(skullInfo.second.first, skullInfo.second.second, *cvar_skullscale, rect, color);
		white->bind();
	}

	lastMapTick = ticks;

	// draw player pings
	if ( !minimapPings[player].empty() )
	{
	    int minimapTotalScale = minimapScale;
		const int DEFAULT_PING_TIME = TICKS_PER_SECOND * 2.5;
		const int DEATH_PING_TIME = TICKS_PER_SECOND * 9.5;
		for ( std::vector<MinimapPing>::iterator it = minimapPings[player].begin(); it != minimapPings[player].end();)
		{
			MinimapPing ping = *it;

			int aliveTime = ticks - ping.tickStart;
			if ( (aliveTime < TICKS_PER_SECOND * DEFAULT_PING_TIME && ping.pingType == MinimapPing::PING_DEFAULT)
				|| (aliveTime < TICKS_PER_SECOND * DEATH_PING_TIME && ping.pingType == MinimapPing::PING_DEATH_MARKER) )
			{
				if ( (aliveTime < TICKS_PER_SECOND && (aliveTime % 10 < 5)) || aliveTime >= TICKS_PER_SECOND || ping.radiusPing )
				{
					// draw the ping blinking every 5 ticks if less than 1 second lifetime, otherwise constantly draw.
					Uint8 alpha = ping.radiusPing ? 50 : 255;
					if ( ping.pingType == MinimapPing::PING_DEATH_MARKER )
					{
						if ( aliveTime >= (DEATH_PING_TIME - (TICKS_PER_SECOND * .5)) )
						{
							// start fading ping after 9 seconds, lasting 0.5 seconds.
							real_t alphafade = 1 - (aliveTime - (DEATH_PING_TIME - (TICKS_PER_SECOND * .5))) / static_cast<real_t>(TICKS_PER_SECOND * 0.5);
							alpha = std::max((int)(alphafade * alpha), 0);
						}
					}
					else if ( aliveTime >= (DEFAULT_PING_TIME - (TICKS_PER_SECOND * .5)) )
					{
						// start fading ping after 2 seconds, lasting 0.5 seconds.
						real_t alphafade = 1 - (aliveTime - (DEFAULT_PING_TIME - (TICKS_PER_SECOND * .5))) / static_cast<real_t>(TICKS_PER_SECOND * 0.5);
						alpha = std::max((int)(alphafade * alpha), 0);
					}

					// set color
		            Uint32 color = playerColor(ping.player, colorblind_lobby, false);
			        uint8_t r, g, b, a;
			        getColor(color, &r, &g, &b, &a);
			        color = makeColor(r, g, b, alpha);

					// draw a circle
					if (ping.radiusPing) 
					{
					    static ConsoleVariable<float> pingSize("/map_radiusping_size", 20.f);
					    static ConsoleVariable<float> pingRate("/map_radiusping_rate", 0.4f);
					    int rate = (int)(TICKS_PER_SECOND / *pingRate);
					    real_t scale = (((ticks - ping.tickStart) % rate) / (real_t)rate) * (*pingSize);
			            drawCircleMesh((real_t)ping.x + 0.5, (real_t)ping.y + 0.5, scale, rect, color);
			        } 
					else if ( ping.pingType == MinimapPing::PING_DEATH_MARKER )
					{
						drawSkull((real_t)ping.x + 0.5, (real_t)ping.y + 0.5, *cvar_skullscale, rect, color);
						white->bind();
					}
					else
					{
			            drawCircleMesh((real_t)ping.x + 0.5, (real_t)ping.y + 0.5, (real_t)1.0, rect, color);
			        }
				}
			}


			// prune old pings > 2.5 seconds
			if ( (aliveTime > DEFAULT_PING_TIME && ping.pingType == MinimapPing::PING_DEFAULT)
				|| (aliveTime > DEATH_PING_TIME && ping.pingType == MinimapPing::PING_DEATH_MARKER) )
			{
				if ( ping.player == player )
				{
					if ( ping.pingType == MinimapPing::PING_DEFAULT )
					{
						if ( minimapPingGimpTimer[player] > TICKS_PER_SECOND / 4 )
						{
							minimapPingGimpTimer[player] = TICKS_PER_SECOND / 4; // reduce the gimp timer when one of the player's own pings fades.
						}
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
	for ( int c = 0; c < 2; ++c )
	{
		for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			int drawMonsterAlly = -1;
			int foundplayer = -1;
			if ( c == 1 && entity->behavior != &actPlayer )
			{
				continue; // render nothing but players on second pass
			}
			else if ( c == 0 && entity->behavior == &actPlayer )
			{
				continue; // render anything *but* players on first pass
			}
			else if ( entity->behavior == &actPlayer )
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
				Uint32 color = 0;
				Uint32 color_edge = 0;
				if ( foundplayer >= 0 ) {
					color_edge = uint32ColorWhite;

					bool foundShadowTaggedEntity = false;
					if ( splitscreen )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( players[i] && players[i]->entity
								&& players[i]->entity->creatureShadowTaggedThisUid == entity->getUID() )
							{
								foundShadowTaggedEntity = true;
								break;
							}
						}
					}
					else
					{
						if ( players[player] && players[player]->entity
							&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() )
						{
							foundShadowTaggedEntity = true;
						}
					}
					if ( foundShadowTaggedEntity ) {
						color = uint32ColorPlayerX; // grey
					} else {
						color = playerColor(foundplayer, colorblind_lobby, false);
					}
				} else if ( entity->sprite == 239 ) { // minotaur
					color_edge = uint32ColorBlack;
					if ( !splitscreen )
					{
						if (!players[player] /*|| !players[player]->entity*/) {
							continue;
						}
					}
					if ( ticks % 120 - ticks % 60 ) {
						if ( !minotaur_timer ) {
							playSound(116, 64);
						}
						minotaur_timer = 1;
					} else {
						minotaur_timer = 0;
						continue;
					}
					color = makeColor(255, 0, 0, 255);
				} else {
					color_edge = uint32ColorGray;

					bool foundShadowTaggedEntity = false;
					if ( splitscreen )
					{
						for ( int i = 0; i < MAXPLAYERS; ++i )
						{
							if ( players[i] && players[i]->entity
								&& players[i]->entity->creatureShadowTaggedThisUid == entity->getUID() )
							{
								foundShadowTaggedEntity = true;
								break;
							}
						}
					}
					else
					{
						if ( players[player] && players[player]->entity
							&& players[player]->entity->creatureShadowTaggedThisUid == entity->getUID() )
						{
							foundShadowTaggedEntity = true;
						}
					}

					if ( foundShadowTaggedEntity ) {
						color = uint32ColorPlayerX_Ally; // grey
					} else {
						color = playerColor(drawMonsterAlly, colorblind_lobby, true);
					}
				}

				static ConsoleVariable<bool> cvar_brightTriangles("/minimap_bright_triangles", false);
				static ConsoleVariable<bool> cvar_outlineTriangles("/minimap_outline_triangles", false);

				auto drawTriangle = [](real_t x, real_t y, real_t ang, real_t size, SDL_Rect rect, Uint32 color){
					const int windowLCD = std::min(rect.w, rect.h);
					const int windowGCD = std::max(rect.w, rect.h);
					const int mapLCD = std::min(map.width, map.height);
					const int mapGCD = std::max(map.width, map.height);
					const int xmin = ((int)map.width - mapGCD) / 2;
					const int ymin = ((int)map.height - mapGCD) / 2;
					const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
					const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
           			const real_t zoom = getMinimapZoom() / 100.0 * size;
					x = (x - xmin) * unitX + rect.x;
					y = (y - ymin) * unitY + rect.y;

					const real_t v[][2] = {
						{  1.0,  0.0 },
						{  0.0,  0.0 },
						{ -0.5,  0.5 },

						{  1.0,  0.0 },
						{ -0.5, -0.5 },
						{  0.0,  0.0 },

						{  0.0,  0.0 },
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
						const real_t sx = vx * unitX * zoom;
						const real_t sy = vy * unitY * zoom;
						if (*cvar_brightTriangles) {
							if (c == 0 || c == 3) {
								glColor4f(1.f, 1.f, 1.f, 1.f);
							} else {
								glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
							}
						}
						glVertex2f(x + sx, Frame::virtualScreenY - (y + sy));
					}
					glEnd();
				};

				const real_t size = entity->sprite == 239 ? 2.0 : 1.0;
				const real_t x = entity->x / 16.0;
				const real_t y = entity->y / 16.0;
				const real_t ang = entity->yaw;
				if (*cvar_outlineTriangles) {
            		drawTriangle(x, y, ang, size, rect, color_edge);
            		drawTriangle(x, y, ang, size - 0.25, rect, color);
				} else {
            		drawTriangle(x, y, ang, size, rect, color);
				}
			}
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
	if (srcPlayer < 0 || srcPlayer >= MAXPLAYERS) {
		return;
	}
	if (destPlayer < 0 || destPlayer >= MAXPLAYERS) {
		return;
	}
	if (newPing.player < 0 || newPing.player >= MAXPLAYERS) {
		return;
	}
	if (!players[destPlayer]->isLocalPlayer()) {
		return;
	}

	if ( !minimapPings[destPlayer].empty() )
	{
		int numPlayerPings = 0;
		for ( std::vector<MinimapPing>::iterator it = minimapPings[destPlayer].begin(); it != minimapPings[destPlayer].end();)
		{
			MinimapPing ping = *it;
			if ( ping.player == newPing.player && !newPing.radiusPing && newPing.pingType == MinimapPing::PING_DEFAULT )
			{
				++numPlayerPings;
				// prune pings if too many by same player.
				if ( numPlayerPings > 3 )
				{
					if ( ping.player == srcPlayer )
					{
						// this is the player creating the sound source.
						minimapPingGimpTimer[srcPlayer] = TICKS_PER_SECOND * 2; // 2 second penalty for spam.
					}
					it = minimapPings[destPlayer].erase(it);
					continue;
				}
			}
			++it;
		}
	}
	if ( !minimapPingMute && !newPing.radiusPing && newPing.pingType == MinimapPing::PING_DEFAULT )
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
