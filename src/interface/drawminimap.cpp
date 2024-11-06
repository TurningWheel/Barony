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

static Mesh circle_mesh;
static Mesh triangle_mesh = {
    {
         1.f,  .0f,  0.f,
        -.5f,  .5f,  0.f,
        -.5f, -.5f,  0.f,
    }, // positions
    {
        
    }, // texcoords
    {
        
    }, // colors
};

// used for drawing icons on the minimap
static Shader minimap_shader;

static const char v_glsl[] =
    "in vec3 iPosition;"
    "uniform mat4 uProj;"
    "uniform mat4 uView;"
    "void main() {"
    "gl_Position = uProj * uView * vec4(iPosition, 1.0);"
    "}";

static const char f_glsl[] =
    "uniform vec4 uColor;"
    "out vec4 FragColor;"
    "void main() {"
    "FragColor = uColor;"
    "}";

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
    circle_mesh.destroy();
    triangle_mesh.destroy();
    minimap_shader.destroy();
}


inline real_t getMinimapZoom()
{
	return minimapObjectZoom + 50;
}

std::map<int, MinimapHighlight_t> minimapHighlights;

void drawMinimap(const int player, SDL_Rect rect, bool drawingSharedMap)
{
	if ( loading )
	{
		return;
	}
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
	const int mapGCD = std::max(map.width, map.height);
	const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
	const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
    
    // build shader if we haven't
    if (!minimap_shader.isInitialized()) {
        minimap_shader.init("minimap shader");
        minimap_shader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        minimap_shader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        minimap_shader.bindAttribLocation("iPosition", 0);
        minimap_shader.link();
    }

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
			if ( entity->behavior == &actMonster )
			{
				if ( entity->monsterAllyIndex < 0 )
				{
					entityPointsOfInterest.push_back(entity);
				}
				else
				{
					for ( int i = 0; i < MAXPLAYERS; ++i )
					{
						if ( achievementObserver.playerAchievements[i].bountyTargets.find(entity->getUID())
							!= achievementObserver.playerAchievements[i].bountyTargets.end() )
						{
							entityPointsOfInterest.push_back(entity);
							break;
						}
					}
				}
			}
			else if ( entity->isDamageableCollider() && entity->colliderHideMonster != 0 )
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
	bool checkedDaedalusEvent = false;
	for ( int x = xmin; x < xmax; ++x ) {
		for ( int y = ymin; y < ymax; ++y ) {
			Uint32 color = 0;
			Uint8 backgroundAlpha = 255 * ((100 - minimapTransparencyBackground) / 100.f);
			Uint8 foregroundAlpha = 255 * ((100 - minimapTransparencyForeground) / 100.f);
			auto foundCustomWall = customWalls.find(x + y * 10000);
			int mapkey = x + y * 10000;
			if ( x < 0 || y < 0 || x >= map.width || y >= map.height )
			{
				// out-of-bounds
				color = makeColor(0, 64, 64, backgroundAlpha);
			}
			else
			{
				auto find = minimapHighlights.find(mapkey);
				if ( find != minimapHighlights.end() )
				{
					if ( find->second.ticks <= ticks )
					{
						if ( map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							if ( !minimap[y][x] )
							{
								minimap[y][x] = 4;
							}
						}
						else if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
						{
							if ( !minimap[y][x] )
							{
								minimap[y][x] = 3;
								if ( !checkedDaedalusEvent )
								{
									for ( auto ent : entityPointsOfInterest )
									{
										if ( ent->behavior == &actLadder || ent->behavior == &actPortal )
										{
											if ( static_cast<int>(ent->x / 16) == x && static_cast<int>(ent->y / 16) == y )
											{
												Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_DAED_EXIT_REVEALS, "daedalus", 1);
												checkedDaedalusEvent = true;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
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


				if ( find != minimapHighlights.end() )
				{
					real_t percent = 100.0;
					if ( find->second.ticks > ticks )
					{
						percent = 0.0;
					}
					else if ( (ticks - find->second.ticks) > TICKS_PER_SECOND )
					{
						percent = std::max(0.0, percent - 4 * ((ticks - find->second.ticks) - TICKS_PER_SECOND));
						if ( percent < 0.01 )
						{
							minimapHighlights.erase(mapkey);
						}
					}
					percent /= 100.0;
					if ( percent > 0.0 )
					{
						Uint8 r, g, b, a;
						int r2, g2, b2, a2;
						getColor(color, &r, &g, &b, &a);
						r2 = std::min(255, std::max(0, (int)(r + 255.0 * percent)));
						g2 = std::min(255, std::max(0, (int)(g - 128.0 * percent)));
						b2 = std::min(255, std::max(0, (int)(b + 255.0 * percent)));
						a2 = std::min(255, std::max(64, (int)(a - 64 * percent)));
						color = makeColor(r2, g2, b2, a2);
					}
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
    
    auto tex = minimapTextures[player];
	Image::draw(tex->texid, tex->w, tex->h, nullptr, rect,
        SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY}, 0xffffffff);

	// build a circle mesh
	auto& circle_positions = circle_mesh.data[(int)Mesh::BufferType::Position];
	if (!circle_positions.size()) {
        circle_positions.insert(circle_positions.end(), {0.f, 0.f, 0.f});
		static const int num_circle_vertices = 32;
		for (int c = num_circle_vertices; c >= 0; --c) {
			float ang = ((PI * 2.f) / num_circle_vertices) * c;
            circle_positions.insert(circle_positions.end(), {sinf(ang) / 2.f, cosf(ang) / 2.f, 0.f});
		}
        circle_mesh.init();
	}

	auto drawCircleMesh = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		const int mapGCD = std::max(map.width, map.height);
		const int xmin = ((int)map.width - mapGCD) / 2;
		const int ymin = ((int)map.height - mapGCD) / 2;
		const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
		const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
		x = (x - xmin) * unitX + rect.x;
		y = (y - ymin) * unitY + rect.y;
        
        // bind shader
        auto& shader = minimap_shader;
        shader.bind();
        
        // upload color
		Uint8 r, g, b, a;
		getColor(color, &r, &g, &b, &a);
		float cv[] = { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
		GL_CHECK_ERR(glUniform4fv(shader.uniform("uColor"), 1, cv));

		vec4_t v;
		mat4x4 m;

		// projection matrix
		mat4x4 proj(1.f);
		(void)ortho(&proj, 0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1.f, 1.f);
		GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));

		// view matrix
		mat4x4 view(1.f);
		v = { (float)x, (float)(Frame::virtualScreenY - y), 0.f, 0.f };
		(void)translate_mat(&m, &view, &v); view = m;
		v = { (float)(unitX * (getMinimapZoom() / 100.0) * size),
			 (float)(unitY * (getMinimapZoom() / 100.0) * size), 0.f, 0.f };
		(void)scale_mat(&m, &view, &v); view = m;
		GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view));

		// draw
		circle_mesh.draw(GL_TRIANGLE_FAN);
	};

	std::vector<std::pair<Uint32, std::pair<real_t, real_t>>> deathboxSkulls;

	auto drawSkull = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		const int mapGCD = std::max(map.width, map.height);
		const int xmin = ((int)map.width - mapGCD) / 2;
		const int ymin = ((int)map.height - mapGCD) / 2;
		const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
		const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
		x = (x - xmin) * unitX + rect.x;
		y = (y - ymin) * unitY + rect.y;

		auto imgGet = Image::get("*images/ui/HUD/death_skull.png");

		const real_t sx = unitX * (getMinimapZoom() / 100.0) * size;
		const real_t sy = unitY * (getMinimapZoom() / 100.0) * size;

        const SDL_Rect src{
            0,
            0,
            imgGet->getSurf()->w,
            imgGet->getSurf()->h,
        };
        const SDL_Rect dest{
            (int)(x - sx / 2.0),
            (int)(y - sy / 2.0),
            (int)(sx),
            (int)(sy),
        };
		const SDL_Rect viewport{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY };
        imgGet->drawColor(&src, dest, viewport, color);
	};

	auto drawBountySkull = [](real_t x, real_t y, real_t size, SDL_Rect rect, Uint32 color) {
		const int mapGCD = std::max(map.width, map.height);
		const int xmin = ((int)map.width - mapGCD) / 2;
		const int ymin = ((int)map.height - mapGCD) / 2;
		const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
		const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
		x = (x - xmin) * unitX + rect.x;
		y = (y - ymin) * unitY + rect.y;

		auto imgGet = Image::get("*images/ui/HUD/death_skull1.png");

		const real_t sx = unitX * (getMinimapZoom() / 100.0) * size;
		const real_t sy = unitY * (getMinimapZoom() / 100.0) * size;

		const SDL_Rect src{
			0,
			0,
			imgGet->getSurf()->w,
			imgGet->getSurf()->h,
		};
		const SDL_Rect dest{
			(int)(x - sx / 2.0),
			(int)(y - sy / 2.0),
			(int)(sx),
			(int)(sy),
		};
		const SDL_Rect viewport{ 0, 0, Frame::virtualScreenX, Frame::virtualScreenY };
		imgGet->drawColor(&src, dest, viewport, color);
	};

	static ConsoleVariable<float> cvar_skullbountyscale("/minimap_skullbountyscale", 1.5);

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
			if ( entity->behavior == &actMonster )
			{
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					//if ( !players[i]->isLocalPlayer() ) { continue; }
					if ( client_disconnected[i] ) { continue; }
					if ( !players[i]->entity ) { continue; }

					bool hat = multiplayer != CLIENT && stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER;
					if ( multiplayer == CLIENT )
					{
						if ( !players[i]->isLocalPlayer() )
						{
							hat = achievementObserver.playerAchievements[i].wearingBountyHat;
						}
						else
						{
							hat = stats[i]->helmet && stats[i]->helmet->type == HAT_BOUNTYHUNTER;
						}
					}
					if ( hat )
					{
						if ( achievementObserver.playerAchievements[i].bountyTargets.find(entity->getUID())
							!= achievementObserver.playerAchievements[i].bountyTargets.end() )
						{
							int x = floor(entity->x / 16);
							int y = floor(entity->y / 16);

							Uint32 color = playerColor(i, colorblind_lobby, false);
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
							drawBountySkull((real_t)x + 0.5, (real_t)y + 0.5, *cvar_skullbountyscale, rect, color);
							break;
						}
					}
				}
			}
			if ( (entity->behavior == &actMonster && entity->monsterAllyIndex < 0)
				|| (entity->isDamageableCollider() && entity->colliderHideMonster != 0) )
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
									if ( stats[i]->shoes->type == ARTIFACT_BOOTS
										&& entity->behavior == &actMonster )
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
								if ( stats[i]->shoes->type == ARTIFACT_BOOTS 
									&& entity->behavior == &actMonster )
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

				if ( colorblind_lobby )
				{
					switch ( playerOwner )
					{
					case 3:
						playerOwner = 1;
						break;
					case 2:
						playerOwner = 0;
						break;
					case 1:
						playerOwner = 2;
						break;
					case 4:
						playerOwner = 3;
					default:
						break;
					}
					color = playerColor(playerOwner, colorblind_lobby, false);
				}

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

	bool blinkPlayers[MAXPLAYERS] = { false };
	{
		struct CalloutPing_t
		{
			real_t x = 0.0;
			real_t y = 0.0;
			Uint32 creationTick = 0;
			int player = 0;
			int targetPlayer = -1;
			Uint32 lifetime = 0;
			Uint32 ticks = 0;
			bool selfCallout = false;
			CalloutPing_t(real_t _x, real_t _y, Uint32 _creationTick, 
				int _player, int _targetPlayer,
				Uint32 _ticks, Uint32 _lifetime, bool _selfCallout) :
				x(_x),
				y(_y),
				creationTick(_creationTick),
				player(_player),
				targetPlayer(_targetPlayer),
				ticks(_ticks),
				lifetime(_lifetime),
				selfCallout(_selfCallout)
			{};
		};
		auto compFunc = [](CalloutPing_t& lhs, CalloutPing_t& rhs)
		{
			return lhs.creationTick < rhs.creationTick;
		};
		std::priority_queue<CalloutPing_t, std::vector<CalloutPing_t>, decltype(compFunc)> priorityQueue(compFunc);
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			blinkPlayers[i] = false;
			for ( auto& callout : CalloutMenu[i].callouts )
			{
				bool selfCallout = false;
				int targetPlayer = -1;
				for ( int j = 0; j < MAXPLAYERS; ++j )
				{
					if ( CalloutRadialMenu::uidMatchesPlayer(j, callout.second.entityUid) )
					{
						selfCallout = true;
						targetPlayer = j;
						break;
					}
				}
				priorityQueue.push(CalloutPing_t(callout.second.x / 16.0, callout.second.y / 16.0, 
					callout.second.creationTick, i, targetPlayer,
					callout.second.ticks,
					callout.second.kParticleLifetime, selfCallout));
			}
		}
		while ( !priorityQueue.empty() )
		{
			auto& ping = priorityQueue.top();

			Uint32 aliveTime = ::ticks - ping.creationTick;
			if ( ping.targetPlayer >= 0 && ping.targetPlayer < MAXPLAYERS )
			{
				blinkPlayers[ping.targetPlayer] = true;
			}
			if ( (aliveTime < (TICKS_PER_SECOND / 2) && (aliveTime % 10 < 5)) || aliveTime >= (TICKS_PER_SECOND / 2) )
			{
				if ( ping.targetPlayer >= 0 && ping.targetPlayer < MAXPLAYERS )
				{
					blinkPlayers[ping.targetPlayer] = false;
					priorityQueue.pop();
					continue;
				}

				// set color
				Uint32 color = playerColor(ping.player, colorblind_lobby, false);
				uint8_t r, g, b, a;
				getColor(color, &r, &g, &b, &a);

				real_t lifePercent = ping.ticks / (real_t)ping.lifetime;
				if ( ping.selfCallout && !drawingSharedMap )
				{
					// fade early for the self callout player, but not others in splitscreen
					lifePercent = ping.ticks / (real_t)((TICKS_PER_SECOND * 4) / 5);
				}
				Uint32 alpha = 255;
				if ( lifePercent >= 0.8 )
				{
					alpha -= std::min((Uint32)255, (Uint32)(255 * (lifePercent - 0.8) / 0.2));
				}
				color = makeColor(r, g, b, alpha);
				drawCircleMesh((real_t)ping.x + 0.5, (real_t)ping.y + 0.5, (real_t)1.0, rect, color);
			}
			priorityQueue.pop();
		}
	}

	auto drawTriangle = [](real_t x, real_t y, real_t ang, real_t size, SDL_Rect rect, Uint32 color, bool outlineOnly) {
		const int mapGCD = std::max(map.width, map.height);
		const int xmin = ((int)map.width - mapGCD) / 2;
		const int ymin = ((int)map.height - mapGCD) / 2;
		const real_t unitX = (real_t)rect.w / (real_t)mapGCD;
		const real_t unitY = (real_t)rect.h / (real_t)mapGCD;
		const real_t zoom = getMinimapZoom() / 100.0 * size;
		x = (x - xmin) * unitX + rect.x;
		y = (y - ymin) * unitY + rect.y;

		// bind shader
		auto& shader = minimap_shader;
		shader.bind();

		// upload color
		Uint8 r, g, b, a;
		getColor(color, &r, &g, &b, &a);
		float cv[] = { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
		GL_CHECK_ERR(glUniform4fv(shader.uniform("uColor"), 1, cv));

		vec4_t v;
		mat4x4 m;

		// projection matrix
		mat4x4 proj(1.f);
		(void)ortho(&proj, 0, Frame::virtualScreenX, 0, Frame::virtualScreenY, -1.f, 1.f);
		GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));

		// view matrix
		mat4x4 view(1.f);
		v = { (float)x, (float)(Frame::virtualScreenY - y), 0.f, 0.f };
		(void)translate_mat(&m, &view, &v); view = m;
		v = { (float)(unitX * zoom), (float)(unitY * zoom), 0.f, 0.f };
		(void)scale_mat(&m, &view, &v); view = m;
		v = { 0.f, 0.f, -1.f, 0.f };
		(void)rotate_mat(&m, &view, ang * 180.f / PI, &v); view = m;
		GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view));

		// draw
		if ( outlineOnly )
		{
			triangle_mesh.draw(GL_LINE_LOOP);
		}
		else
		{
			triangle_mesh.draw();
		}
	};

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
				if ( blinkPlayers[foundplayer] )
				{
					continue; // skip this one since it's blinking from a callout
				}
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
						color = uint32ColorPlayerX; // grey
                        uint8_t r, g, b, a;
                        getColor(color, &r, &g, &b, &a);
                        r /= 2; g /= 2; b /= 2;
                        color = makeColor(r, g, b, a);
					} else {
						color = playerColor(drawMonsterAlly, colorblind_lobby, true);
					}
				}

				static ConsoleVariable<bool> cvar_brightTriangles("/minimap_bright_triangles", false);
				static ConsoleVariable<bool> cvar_outlineTriangles("/minimap_outline_triangles", false);
                
                if (!triangle_mesh.isInitialized()) {
                    triangle_mesh.init();
                }

				const real_t size = entity->sprite == 239 ? 2.0 : 1.0;
				const real_t x = entity->x / 16.0;
				const real_t y = entity->y / 16.0;
				const real_t ang = entity->yaw;
				if (*cvar_outlineTriangles) {
            		drawTriangle(x, y, ang, size, rect, color, true);
				} else {
            		drawTriangle(x, y, ang, size, rect, color, false);
				}
			}
		}
	}

	// draw ghosts
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		Entity* entity = nullptr;
		if ( players[i] && players[i]->ghost.isActive() )
		{
			entity = players[i]->ghost.my;
		}
		if ( !entity )
		{
			continue;
		}
		if ( blinkPlayers[i] )
		{
			continue; // skip this one since it's blinking from a callout
		}
		Uint32 color = playerColor(i, colorblind_lobby, false);
		if ( !triangle_mesh.isInitialized() ) {
			triangle_mesh.init();
		}

		const real_t size = entity->sprite == 239 ? 2.0 : 1.0;
		const real_t x = entity->x / 16.0;
		const real_t y = entity->y / 16.0;
		const real_t ang = entity->yaw;
		drawTriangle(x, y, ang, size, rect, color, true);
	}
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

static ConsoleVariable<float> cvar_shrine_reveal_steps("/shrine_reveal_steps", 8.0);
void shrineDaedalusRevealMap(Entity& my)
{
	Entity* exitEntity = nullptr;
	for ( node_t* node = map.entities->first; node; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if ( !entity ) { continue; }

		if ( (entity->behavior == &actLadder && strcmp(map.name, "Hell"))
			|| (entity->behavior == &actPortal && !strcmp(map.name, "Hell")) )
		{
			if ( entity->behavior == &actLadder && entity->skill[3] != 1 )
			{
				exitEntity = entity;
				break;
			}
			if ( entity->behavior == &actPortal && entity->portalNotSecret == 1 )
			{
				exitEntity = entity;
				break;
			}
		}
	}

	if ( !exitEntity )
	{
		return;
	}

	minimapHighlights.clear();

	real_t tangent = atan2(exitEntity->y - my.y, exitEntity->x - my.x);

	if ( Entity* lightball = newEntity(1482, 1, map.entities, nullptr) )
	{
		lightball->x = my.x;
		lightball->y = my.y;
		lightball->z = -2;
		lightball->vel_x = 0.33 * *cvar_shrine_reveal_steps * cos(tangent);
		lightball->vel_y = 0.33 * *cvar_shrine_reveal_steps * sin(tangent);
		lightball->yaw = tangent;
		lightball->parent = my.getUID();
		lightball->behavior = &actMagiclightMoving;
		lightball->skill[0] = TICKS_PER_SECOND * 10;
		lightball->skill[1] = 1;
		lightball->flags[NOUPDATE] = true;
		lightball->flags[PASSABLE] = true;
		lightball->flags[UNCLICKABLE] = true;
		lightball->flags[NOCLIP_WALLS] = true;
		lightball->flags[INVISIBLE] = true;
		lightball->flags[INVISIBLE_DITHER] = true;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		lightball->setUID(-3);
	}

	real_t dist = entityDist(&my, exitEntity);
	std::set<int> visited;
	real_t d = 0.0;
	std::vector<int> checkCoords;
	Uint32 tickOffset = ticks;
	while ( d < dist )
	{
		checkCoords.clear();

		{
			real_t tx = (my.x + d * cos(tangent)) / 16.0;
			real_t ty = (my.y + d * sin(tangent)) / 16.0;
			checkCoords.push_back(static_cast<int>(tx) + 10000 * static_cast<int>(ty));

			tx = (my.x + d * cos(tangent) + (16.0 * cos(tangent + PI / 2))) / 16.0;
			ty = (my.y + d * sin(tangent) + (16.0 * sin(tangent + PI / 2))) / 16.0;
			checkCoords.push_back(static_cast<int>(tx) + 10000 * static_cast<int>(ty));

			tx = (my.x + d * cos(tangent) + (16.0 * cos(tangent - PI / 2))) / 16.0;
			ty = (my.y + d * sin(tangent) + (16.0 * sin(tangent - PI / 2))) / 16.0;
			checkCoords.push_back(static_cast<int>(tx) + 10000 * static_cast<int>(ty));
		}

		for ( auto coord : checkCoords )
		{
			int x = coord % 10000;
			int y = coord / 10000;
			if ( x >= 0 && y >= 0 && x < map.width && y < map.height )
			{
				if ( visited.find(x + 10000 * y) == visited.end() )
				{
					visited.insert(x + 10000 * y);
					minimapHighlights[x + 10000 * y].ticks = tickOffset;
				}
			}
		}

		d += *cvar_shrine_reveal_steps;
		tickOffset += TICKS_PER_SECOND / 25;
	}

	playSoundPlayer(clientnum, 167, 128);
}