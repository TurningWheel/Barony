/*-------------------------------------------------------------------------------

	BARONY
	File: opengl.cpp
	Desc: contains all drawing functions for opengl

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "items.hpp"

#ifdef WINDOWS
PFNGLGENBUFFERSPROC SDL_glGenBuffers;
PFNGLBINDBUFFERPROC SDL_glBindBuffer;
PFNGLBUFFERDATAPROC SDL_glBufferData;
PFNGLDELETEBUFFERSPROC SDL_glDeleteBuffers;
PFNGLGENVERTEXARRAYSPROC SDL_glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC SDL_glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC SDL_glDeleteVertexArrays;
PFNGLENABLEVERTEXATTRIBARRAYPROC SDL_glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC SDL_glVertexAttribPointer;
#endif

void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble fW, fH;

	fH = tan(fovY / 360 * PI) * zNear;
	fW = fH * aspect;

	glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

/*-------------------------------------------------------------------------------

	getLightForEntity

	Returns a shade factor (0.0-1.0) to shade an entity with, based on
	its surroundings

-------------------------------------------------------------------------------*/

real_t getLightForEntity(real_t x, real_t y)
{
	if ( x < 0 || y < 0 || x >= map.width || y >= map.height )
	{
		return 1.f;
	}
	int u = x;
	int v = y;
	return std::min(std::max(0, lightmapSmoothed[v + u * map.height]), 255) / 255.0;
}

/*-------------------------------------------------------------------------------

	glDrawVoxel

	Draws a voxel model at the given world coordinates

-------------------------------------------------------------------------------*/

bool wholevoxels = false;
void glDrawVoxel(view_t* camera, Entity* entity, int mode)
{
	real_t dx, dy, dz;
	int voxX, voxY, voxZ;
	real_t s = 1;
	//int x = 0;
	//int y = 0;
	Sint32 index;
	Sint32 indexdown[3];
	voxel_t* model;
	int modelindex = 0;
	GLfloat rotx, roty, rotz;
	//GLuint uidcolor;

	if (!entity)
	{
		return;
	}

	// assign model
	if ( entity->sprite >= 0 && entity->sprite < nummodels )
	{
		if ( models[entity->sprite] != NULL )
		{
			model = models[entity->sprite];
		}
		else
		{
			model = models[0];
		}
		modelindex = entity->sprite;
	}
	else
	{
		model = models[0];
		modelindex = 0;
	}

	if ( model == models[0] )
	{
		return; // don't draw green balls
	}

	// model array indexes
	indexdown[0] = model->sizez * model->sizey;
	indexdown[1] = model->sizez;
	indexdown[2] = 1;

	glBindTexture(GL_TEXTURE_2D, 0);

	// setup projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable( GL_DEPTH_TEST );
	if ( !entity->flags[OVERDRAW] )
	{
		rotx = camera->vang * 180 / PI; // get x rotation
		roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	}
	else
	{
		glRotatef(90, 0, 1, 0);
	}

	// setup model matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glPushMatrix();
	rotx = entity->roll * 180 / PI; // get x rotation
	roty = 360 - entity->yaw * 180 / PI; // get y rotation
	rotz = 360 - entity->pitch * 180 / PI; // get z rotation
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	glRotatef(roty, 0, 1, 0); // rotate yaw
	glRotatef(rotz, 0, 0, 1); // rotate pitch
	glRotatef(rotx, 1, 0, 0); // rotate roll
	glTranslatef(entity->focalx * 2, -entity->focalz * 2, entity->focaly * 2);
	glScalef(entity->scalex, entity->scalez, entity->scaley);
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	if ( entity->flags[OVERDRAW] || entity->monsterEntityRenderAsTelepath == 1 )
	{
		glDepthRange(0, 0.1);
	}

	bool highlightEntity = false;
	bool highlightEntityFromParent = false;
	int player = -1;
	for ( player = 0; player < MAXPLAYERS; ++player )
	{
		if ( &cameras[player] == camera )
		{
			break;
		}
	}
	highlightEntity = entity->bEntityHighlightedForPlayer(player);
	if ( !highlightEntity && (entity->sprite == 184 || entity->sprite == 585 || entity->sprite == 216) ) // lever base/chest lid
	{
		Entity* parent = uidToEntity(entity->parent);
		if ( parent && parent->bEntityHighlightedForPlayer(player) )
		{
			entity->highlightForUIGlow = parent->highlightForUIGlow;
			highlightEntityFromParent = true;
			highlightEntity = highlightEntityFromParent;
		}
	}

	// get shade factor
	if (!entity->flags[BRIGHT])
	{
		if ( !entity->flags[OVERDRAW] )
		{
			if ( entity->monsterEntityRenderAsTelepath == 1 )
			{
				if ( globalLightModifierActive )
				{
					s = globalLightTelepathyModifier;
				}
			}
			else
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
		}
		else
		{
			s = getLightForEntity(camera->x, camera->y);
		}
	}

	if ( globalLightModifierActive && entity->monsterEntityRenderAsTelepath == 0 )
	{
		s *= globalLightModifier;
	}

	// Moved glBeign / glEnd outside the loops, to limit the number of calls (helps gl4es on Pandora)
	if ( wholevoxels )
	{
		glBegin( GL_QUADS );
		for ( index = 0, voxX = 0; voxX < model->sizex; voxX++ )
		{
			for ( voxY = 0; voxY < model->sizey; voxY++ )
			{
				for ( voxZ = 0; voxZ < model->sizez; voxZ++, index++ )
				{
					// get the bit color
					if ( model->data[index] == 255 || model->data[index] == 0 )
					{
						continue;
					}
					if ( mode == REALCOLORS )
					{
						glColor3f((model->palette[model->data[index]][0] / 255.0)*s, (model->palette[model->data[index]][1] / 255.0)*s, (model->palette[model->data[index]][2] / 255.0)*s );
					}
					else
					{
						Uint32 uid = entity->getUID();
						glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
					}

					// calculate model offsets
					dx = (real_t)voxX - ((real_t)model->sizex) / 2.f;
					dy = (real_t)voxY - ((real_t)model->sizey) / 2.f;
					dz = ((real_t)model->sizez) / 2.f - (real_t)voxZ;

					// draw front of cube
					bool drawFront = false;
					if ( voxX == model->sizex - 1 )
					{
						drawFront = true;
					}
					else if ( model->data[index + indexdown[0]] == 255 )
					{
						drawFront = true;
					}
					if ( drawFront )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						//glEnd();
					}

					// draw back of cube
					bool drawBack = false;
					if ( voxX == 0 )
					{
						drawBack = true;
					}
					else if ( model->data[index - indexdown[0]] == 255 )
					{
						drawBack = true;
					}
					if ( drawBack )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 0, dz + 0, dy + 0);
						//glEnd();
					}

					// draw right side of cube
					bool drawRight = false;
					if ( voxY == model->sizey - 1 )
					{
						drawRight = true;
					}
					else if ( model->data[index + indexdown[1]] == 255 )
					{
						drawRight = true;
					}
					if ( drawRight )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						//glEnd();
					}

					// draw left side of cube
					bool drawLeft = false;
					if ( voxY == 0 )
					{
						drawLeft = true;
					}
					else if ( model->data[index - indexdown[1]] == 255 )
					{
						drawLeft = true;
					}
					if ( drawLeft )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 0);
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						//glEnd();
					}

					// draw bottom of cube
					bool drawBottom = false;
					if ( voxZ == model->sizez - 1 )
					{
						drawBottom = true;
					}
					else if ( model->data[index + indexdown[2]] == 255 )
					{
						drawBottom = true;
					}
					if ( drawBottom )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 0, dz + 0, dy + 1);
						//glEnd();
					}

					// draw top of cube
					bool drawTop = false;
					if ( voxZ == 0 )
					{
						drawTop = true;
					}
					else if ( model->data[index - indexdown[2]] == 255 )
					{
						drawTop = true;
					}
					if ( drawTop )
					{
						//glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						//glEnd();
					}
				}
			}
		}
		glEnd();
	}
	else
	{
		if ( disablevbos )
		{
			glBegin( GL_TRIANGLES ); //moved outside
			for ( index = 0; index < polymodels[modelindex].numfaces; index++ )
			{
				if ( mode == REALCOLORS )
				{
					if ( entity->flags[USERFLAG2] )
					{
						if ( entity->behavior == &actMonster 
							&& (entity->isPlayerHeadSprite() || entity->sprite == 467 || !monsterChangesColorWhenAlly(nullptr, entity)) )
						{
							// dont invert human heads, or automaton heads.
							glColor3f((polymodels[modelindex].faces[index].r / 255.f)*s, (polymodels[modelindex].faces[index].g / 255.f)*s, (polymodels[modelindex].faces[index].b / 255.f)*s );
						}
						else
						{
							glColor3f((polymodels[modelindex].faces[index].b / 255.f)*s, (polymodels[modelindex].faces[index].r / 255.f)*s, (polymodels[modelindex].faces[index].g / 255.f)*s);
						}
					}
					else
					{
						glColor3f((polymodels[modelindex].faces[index].b / 255.f)*s, (polymodels[modelindex].faces[index].r / 255.f)*s, (polymodels[modelindex].faces[index].g / 255.f)*s );
					}
				}
				else
				{
					Uint32 uid = entity->getUID();
					glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
				}

				polytriangle_t* face = &polymodels[modelindex].faces[index];

				//glBegin( GL_TRIANGLES );
				glVertex3f(face->vertex[0].x, -face->vertex[0].z, face->vertex[0].y);
				glVertex3f(face->vertex[1].x, -face->vertex[1].z, face->vertex[1].y);
				glVertex3f(face->vertex[2].x, -face->vertex[2].z, face->vertex[2].y);
				//glEnd();
			}
			glEnd();
		}
		else
		{
			SDL_glBindVertexArray(polymodels[modelindex].va);
			SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].vbo);
			glVertexPointer( 3, GL_FLOAT, 0, (char*) NULL );  // Set The Vertex Pointer To The Vertex Buffer
			glEnableClientState(GL_VERTEX_ARRAY); // enable the vertex array on the client side
			if ( mode == REALCOLORS )
			{
				glEnableClientState(GL_COLOR_ARRAY); // enable the color array on the client side
				if ( entity->flags[USERFLAG2] )
				{
					if ( entity->behavior == &actMonster && (entity->isPlayerHeadSprite() 
						|| entity->sprite == 467 || !monsterChangesColorWhenAlly(nullptr, entity)) )
					{
						SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
					}
					else
					{
						SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors_shifted);
					}
				}
				else
				{
					SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
				}
				glColorPointer(3, GL_FLOAT, 0, 0);
				GLfloat params_col[4] = { static_cast<GLfloat>(s), static_cast<GLfloat>(s), static_cast<GLfloat>(s), 1.f };
				if ( highlightEntity )
				{
					glEnable(GL_LIGHTING);
					glEnable(GL_LIGHT1);
					if ( !highlightEntityFromParent )
					{
						entity->highlightForUIGlow = (0.05 * (entity->ticks % 41));
					}
					real_t highlight = entity->highlightForUIGlow;
					if ( highlight > 1.0 )
					{
						highlight = 1.0 - (highlight - 1.0);
					}
					GLfloat ambient[4] = { 
						static_cast<GLfloat>(.15 + highlight * .15), 
						static_cast<GLfloat>(.15 + highlight * .15), 
						static_cast<GLfloat>(.15 + highlight * .15),
						1.f };
					glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params_col);
					glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
					glEnable(GL_COLOR_MATERIAL);
				}
				else
				{
					glEnable(GL_LIGHTING);
					glEnable(GL_COLOR_MATERIAL);
					glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params_col);
				}
			}
			else
			{
				GLfloat uidcolors[4];
				Uint32 uid = entity->getUID();
				uidcolors[0] = ((Uint8)(uid)) / 255.f;
				uidcolors[1] = ((Uint8)(uid >> 8)) / 255.f;
				uidcolors[2] = ((Uint8)(uid >> 16)) / 255.f;
				uidcolors[3] = ((Uint8)(uid >> 24)) / 255.f;
				glColor4f(uidcolors[0], uidcolors[1], uidcolors[2], uidcolors[3]);
			}
			glDrawArrays(GL_TRIANGLES, 0, 3 * polymodels[modelindex].numfaces);
			if ( mode == REALCOLORS )
			{
				glDisable(GL_COLOR_MATERIAL);
				glDisable(GL_LIGHTING);
				if ( highlightEntity )
				{
					glDisable(GL_LIGHT1);
				}
				glDisableClientState(GL_COLOR_ARRAY); // disable the color array on the client side
			}
			glDisableClientState(GL_VERTEX_ARRAY); // disable the vertex array on the client side
		}
	}
	glDepthRange(0, 1);
	glPopMatrix();
}

/*-------------------------------------------------------------------------------

	glDrawSprite

	Draws a 2D sprite to represent an object in 3D

-------------------------------------------------------------------------------*/
SDL_Surface* glTextSurface(std::string text, GLuint* outTextId)
{
	SDL_Surface* image = sprites[0];
	GLuint textureId = texid[sprites[0]->refcount];
	char textToRetrieve[128];
	strncpy(textToRetrieve, text.c_str(), 127);
	textToRetrieve[std::min(static_cast<int>(strlen(text.c_str())), 127)] = '\0';

	if ( (image = ttfTextHashRetrieve(ttfTextHash, textToRetrieve, ttf12, true)) != NULL )
	{
		textureId = texid[image->refcount];
	}
	else
	{
		// create the text outline surface
		TTF_SetFontOutline(ttf12, 2);
		SDL_Color sdlColorBlack = { 0, 0, 0, 255 };
		image = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorBlack);

		// create the text surface
		TTF_SetFontOutline(ttf12, 0);
		SDL_Color sdlColorWhite = { 255, 255, 255, 255 };
		SDL_Surface* textSurf = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorWhite);

		// combine the surfaces
		SDL_Rect pos;
		pos.x = 2;
		pos.y = 2;
		pos.h = 0;
		pos.w = 0;

		SDL_BlitSurface(textSurf, NULL, image, &pos);
		SDL_FreeSurface(textSurf);
		// load the text outline surface as a GL texture
		allsurfaces[imgref] = image;
		allsurfaces[imgref]->refcount = imgref;
		glLoadTexture(allsurfaces[imgref], imgref);
		imgref++;
		// store the surface in the text surface cache
		if ( !ttfTextHashStore(ttfTextHash, textToRetrieve, ttf12, true, image) )
		{
			printlog("warning: failed to store text outline surface with imgref %d\n", imgref - 1);
		}
		textureId = texid[image->refcount];
	}
	if ( outTextId )
	{
		*outTextId = textureId;
	}
	return image;
}

void glDrawWorldUISprite(view_t* camera, Entity* entity, int mode)
{
	SDL_Surface* sprite;
	real_t s = 1;

	if ( !entity )
	{
		return;
	}
	if ( !uidToEntity(entity->parent) )
	{
		return;
	}
	int player = -1;
	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		if ( entity->worldTooltipIgnoreDrawing != 0 )
		{
			return;
		}
		for ( player = 0; player < MAXPLAYERS; ++player )
		{
			if ( &cameras[player] == camera )
			{
				break;
			}
		}
		if ( player >= 0 && player < MAXPLAYERS )
		{
			if ( entity->worldTooltipPlayer != player )
			{
				return;
			}
			if ( entity->worldTooltipActive == 0 && entity->worldTooltipFadeDelay == 0 )
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// setup projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable(GL_DEPTH_TEST);
	if ( !entity->flags[OVERDRAW] || entity->flags[OVERDRAW] )
	{
		GLfloat rotx = camera->vang * 180 / PI; // get x rotation
		GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		GLfloat rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	}
	else
	{
		glRotatef(90, 0, 1, 0);
	}

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	// assign texture
	TempTexture* tex = nullptr;
	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		Entity* parent = uidToEntity(entity->parent);
		if ( parent && parent->behavior == &actItem && (multiplayer != CLIENT || (multiplayer == CLIENT && parent->itemReceivedDetailsFromServer != 0)) )
		{
			Item* item = newItemFromEntity(uidToEntity(entity->parent));
			if ( !item )
			{
				return;
			}

			SDL_Rect tooltip;
			tooltip.h = TTF12_HEIGHT * 4 + 8;
			tooltip.w = std::max(13, longestline(item->description())) * TTF12_WIDTH + 8;
			tooltip.w = std::max(20 * TTF12_WIDTH + 8, tooltip.w);
			if ( parent->behavior == &actItem )
			{
				sprite = SDL_CreateRGBSurface(0, tooltip.w, tooltip.h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
				SDL_FillRect(sprite, nullptr, SDL_MapRGBA(mainsurface->format, 0, 0, 0, 255));
				SDL_LockSurface(sprite);

				for ( int x = 0; x < sprite->w; x++ )
				{
					Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 192, 255, 255);
					putPixel(sprite, x, 0, color);
					putPixel(sprite, x, sprite->h - 1, color);
				}
				for ( int y = 0; y < sprite->h; y++ )
				{
					Uint32 color = SDL_MapRGBA(mainsurface->format, 0, 192, 255, 255);
					putPixel(sprite, 0, y, color);
					putPixel(sprite, sprite->w - 1, y, color);
				}
			}
			else
			{
				sprite = SDL_CreateRGBSurface(0, 320, 32, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
				SDL_FillRect(sprite, nullptr, SDL_MapRGBA(mainsurface->format, 0, 0, 0, 0));
				SDL_LockSurface(sprite);
			}
			SDL_UnlockSurface(sprite);

			node_t* node = list_Node(&items[item->type].surfaces, item->appearance % items[item->type].variations);
			if ( !node )
			{
				return;
			}
			SDL_Rect pos;
			pos.w = 48;
			pos.h = 48;
			pos.x = tooltip.w - pos.w - 8;
			pos.y = TTF12_HEIGHT;
			SDL_Surface** itemSurf = static_cast<SDL_Surface**>(node->element);
			SDL_BlitScaled(*itemSurf, nullptr, sprite, &pos);

			GLuint itemTexid = 0;
			SDL_Surface* textSurf = glTextSurface(item->description(), &itemTexid);
			if ( textSurf )
			{
				pos.x = 4;
				pos.y = 0;
				SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
			}
			char buf[256] = "";
			if ( !item->identified )
			{
				textSurf = glTextSurface(language[309], &itemTexid);
				if ( textSurf )
				{
					pos.y += TTF12_HEIGHT;
					SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
				}
			}
			else
			{
				if ( item->beatitude < 0 )
				{
					textSurf = glTextSurface(language[310], &itemTexid);
				}
				else if ( item->beatitude > 0 )
				{
					textSurf = glTextSurface(language[312], &itemTexid);
				}
				else
				{
					textSurf = glTextSurface(language[311], &itemTexid);
				}
				if ( textSurf )
				{
					pos.y += TTF12_HEIGHT;
					SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
				}
			}

			snprintf(buf, 255, language[313], items[item->type].weight);
			textSurf = glTextSurface(buf, &itemTexid);
			if ( textSurf )
			{
				pos.y += TTF12_HEIGHT;
				SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
			}
			snprintf(buf, 255, language[314], item->sellValue(player));
			textSurf = glTextSurface(buf, &itemTexid);
			if ( textSurf )
			{
				pos.y += TTF12_HEIGHT;
				SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
			}
			free(item);
			item = nullptr;
		}
		if ( static_cast<Sint32>(entity->getUID()) == -21 )
		{
			GLuint tmpTextid = 0;
			SDL_Rect pos;
			pos.x = 0;
			pos.y = 0;
			pos.w = 64;
			pos.h = 64;
			if ( parent->behavior == &actItem )
			{
				//SDL_Surface* textSurf = glTextSurface("Press use to pick up!", &tmpTextid);
				//if ( textSurf )
				//{
				//	pos.x = 32 + 16;
				//	pos.y += 32;
				//	if ( multiplayer == CLIENT && parent->itemReceivedDetailsFromServer == 0 )
				//	{
				//		// no details yet.
				//		pos.y -= 24;
				//	}
				//	SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
				//}
			}
			/*else
			{
				SDL_Surface* textSurf = glTextSurface("Press use to interact!", &tmpTextid);
				if ( textSurf )
				{
					pos.x = 32 + 16;
					pos.y += 8;
					SDL_BlitSurface(textSurf, nullptr, sprite, &pos);
					SDL_BlitSurface(selected_glyph_bmp, nullptr, sprite, &pos);
				}
			}*/
		}
		//
		tex = new TempTexture();
		tex->load(sprite, false, true);
		if ( mode == REALCOLORS )
		{
			tex->bind();
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		//glBindTexture(GL_TEXTURE_2D, texid[sprite->refcount]);
	}
	else
	{
		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != NULL )
			{
				sprite = sprites[entity->sprite];
			}
			else
			{
				sprite = sprites[0];
			}
		}
		else
		{
			sprite = sprites[0];
		}
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if ( !entity->flags[OVERDRAW] || entity->flags[OVERDRAW] )
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0.1, 0.2);
	}

	// get shade factor
	if ( mode == REALCOLORS )
	{
		if ( !entity->flags[BRIGHT] )
		{
			if ( !entity->flags[OVERDRAW] )
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}

			if ( globalLightModifierActive )
			{
				s *= globalLightModifier;
			}

			glColor4f(s, s, s, 1);
		}
		else
		{
			if ( entity->behavior == &actSpriteWorldTooltip )
			{
				glColor4f(1.f, 1.f, 1.f, entity->worldTooltipAlpha);
			}
			else if ( globalLightModifierActive )
			{
				glColor4f(globalLightModifier, globalLightModifier, globalLightModifier, 1);
			}
			else
			{
				glColor4f(1.f, 1.f, 1.f, 1);
			}
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, sprite->h / 2, sprite->w / 2);
	glTexCoord2f(0, 1);
	glVertex3f(0, -sprite->h / 2, sprite->w / 2);
	glTexCoord2f(1, 1);
	glVertex3f(0, -sprite->h / 2, -sprite->w / 2);
	glTexCoord2f(1, 0);
	glVertex3f(0, sprite->h / 2, -sprite->w / 2);
	glEnd();
	glDepthRange(0, 1);
	glPopMatrix();

	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		if ( tex ) {
			delete tex;
			tex = nullptr;
		}
		if ( sprite ) {
			SDL_FreeSurface(sprite);
			sprite = nullptr;
		}
	}
}

void glDrawSprite(view_t* camera, Entity* entity, int mode)
{
	SDL_Surface* sprite;
	//int x, y;
	real_t s = 1;

	// setup projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable( GL_DEPTH_TEST );
	if (!entity->flags[OVERDRAW])
	{
		GLfloat rotx = camera->vang * 180 / PI; // get x rotation
		GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		GLfloat rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	}
	else
	{
		glRotatef(90, 0, 1, 0);

	}

	// setup model matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glPushMatrix();
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	// assign texture
	if ( entity->sprite >= 0 && entity->sprite < numsprites )
	{
		if ( sprites[entity->sprite] != NULL )
		{
			sprite = sprites[entity->sprite];
		}
		else
		{
			sprite = sprites[0];
		}
	}
	else
	{
		sprite = sprites[0];
	}

	if ( mode == REALCOLORS )
	{
		glBindTexture(GL_TEXTURE_2D, texid[sprite->refcount]);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if (!entity->flags[OVERDRAW])
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0, 0.1);
	}

	// get shade factor
	if ( mode == REALCOLORS )
	{
		if (!entity->flags[BRIGHT])
		{
			if (!entity->flags[OVERDRAW])
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}

			if ( globalLightModifierActive )
			{
				s *= globalLightModifier;
			}

			glColor4f(s, s, s, 1);
		}
		else
		{
			if ( globalLightModifierActive )
			{
				glColor4f(globalLightModifier, globalLightModifier, globalLightModifier, 1);
			}
			else
			{
				glColor4f(1.f, 1.f, 1.f, 1);
			}
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, sprite->h / 2, sprite->w / 2);
	glTexCoord2f(0, 1);
	glVertex3f(0, -sprite->h / 2, sprite->w / 2);
	glTexCoord2f(1, 1);
	glVertex3f(0, -sprite->h / 2, -sprite->w / 2);
	glTexCoord2f(1, 0);
	glVertex3f(0, sprite->h / 2, -sprite->w / 2);
	glEnd();
	glDepthRange(0, 1);
	glPopMatrix();
}

void glDrawSpriteFromImage(view_t* camera, Entity* entity, std::string text, int mode)
{
	//int x, y;
	real_t s = 1;
	SDL_Surface* image = sprites[0];
	GLuint textureId = texid[sprites[0]->refcount];
	char textToRetrieve[128];

	if ( text.compare("") == 0 )
	{
		return;
	}

	// strncpy() does not copy N bytes if a terminating null is encountered first
	// see http://www.cplusplus.com/reference/cstring/strncpy/
	// see https://en.cppreference.com/w/c/string/byte/strncpy
	// GCC throws a warning (intended) when the length argument to strncpy()
	// in any way depends on strlen(src) to discourage this (and related) construct(s).
	strncpy(textToRetrieve, text.c_str(), 22);
	textToRetrieve[std::min(static_cast<int>(strlen(text.c_str())), 22)] = '\0';
	if ( (image = ttfTextHashRetrieve(ttfTextHash, textToRetrieve, ttf12, true)) != NULL )
	{
		textureId = texid[image->refcount];
	}
	else
	{
		// create the text outline surface
		TTF_SetFontOutline(ttf12, 2);
		SDL_Color sdlColorBlack = { 0, 0, 0, 255 };
		image = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorBlack);

		// create the text surface
		TTF_SetFontOutline(ttf12, 0);
		SDL_Color sdlColorWhite = { 255, 255, 255, 255 };
		SDL_Surface* textSurf = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorWhite);

		// combine the surfaces
		SDL_Rect pos;
		pos.x = 2;
		pos.y = 2;
		pos.h = 0;
		pos.w = 0;

		SDL_BlitSurface(textSurf, NULL, image, &pos);
		SDL_FreeSurface(textSurf);
		// load the text outline surface as a GL texture
		allsurfaces[imgref] = image;
		allsurfaces[imgref]->refcount = imgref;
		glLoadTexture(allsurfaces[imgref], imgref);
		imgref++;
		// store the surface in the text surface cache
		if ( !ttfTextHashStore(ttfTextHash, textToRetrieve, ttf12, true, image) )
		{
			printlog("warning: failed to store text outline surface with imgref %d\n", imgref - 1);
		}
		textureId = texid[image->refcount];
	}
	// setup projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable(GL_DEPTH_TEST);
	if ( !entity->flags[OVERDRAW] )
	{
		GLfloat rotx = camera->vang * 180 / PI; // get x rotation
		GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		GLfloat rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	}
	else
	{
		glRotatef(90, 0, 1, 0);
	}


	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	// assign texture
	if ( mode == REALCOLORS )
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if ( !entity->flags[OVERDRAW] )
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0, 0.1);
	}

	// get shade factor
	if ( mode == REALCOLORS )
	{
		if ( !entity->flags[BRIGHT] )
		{
			if ( !entity->flags[OVERDRAW] )
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}
			glColor4f(s, s, s, 1);
		}
		else
		{
			glColor4f(1.f, 1.f, 1.f, 1);
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, image->h / 2, image->w / 2);
	glTexCoord2f(0, 1);
	glVertex3f(0, -image->h / 2, image->w / 2);
	glTexCoord2f(1, 1);
	glVertex3f(0, -image->h / 2, -image->w / 2);
	glTexCoord2f(1, 0);
	glVertex3f(0, image->h / 2, -image->w / 2);
	glEnd();
	glDepthRange(0, 1);
	glPopMatrix();
}

/*-------------------------------------------------------------------------------

	getLightAt

	returns the light shade factor for the vertex at the given x/y point

-------------------------------------------------------------------------------*/

real_t getLightAt(int x, int y)
{
	real_t l = 0;
	int u, v;

	for ( u = x - 1; u < x + 1; u++ )
	{
		for ( v = y - 1; v < y + 1; v++ )
		{
			if ( u >= 0 && u < map.width && v >= 0 && v < map.height )
			{
				l += std::min(std::max(0, lightmapSmoothed[v + u * map.height]), 255) / 255.0;
			}
		}
	}

	if ( globalLightModifierActive )
	{
		l *= globalLightModifier;
	}

	return l / 4.f;
}

/*-------------------------------------------------------------------------------

	glDrawWorld

	Draws the current map from the given camera point

-------------------------------------------------------------------------------*/

void glDrawWorld(view_t* camera, int mode)
{
	int x, y, z;
	int index;
	real_t s;
	bool clouds = false;
	int cloudtile = 0;
	int mapceilingtile = 50;

	if ( softwaremode == true )
	{
		return;
	}

	if ( (!strncmp(map.name, "Hell", 4) || map.skybox != 0) && smoothlighting )
	{
		clouds = true;
		if ( !strncmp(map.name, "Hell", 4) )
		{
			cloudtile = 77;
		}
		else
		{
			cloudtile = map.skybox;
		}
	}

	for ( int v = 0; v < map.height; v++ )
	{
		for ( int u = 0; u < map.width; u++ )
		{
			int smoothingRate = globalLightSmoothingRate;
			int difference = abs(lightmapSmoothed[v + u * map.height] - lightmap[v + u * map.height]);
			if ( difference > 64 )
			{
				smoothingRate *= 4;
			}
			else if ( difference > 32 )
			{
				smoothingRate *= 2;
			}
			if ( lightmapSmoothed[v + u * map.height] < lightmap[v + u * map.height] )
			{
				lightmapSmoothed[v + u * map.height] = std::min(lightmap[v + u * map.height], lightmapSmoothed[v + u * map.height] + smoothingRate);
			}
			else if ( lightmapSmoothed[v + u * map.height] > lightmap[v + u * map.height] )
			{
				lightmapSmoothed[v + u * map.height] = std::max(lightmap[v + u * map.height], lightmapSmoothed[v + u * map.height] - smoothingRate);
			}
		}
	}

	if ( map.flags[MAP_FLAG_CEILINGTILE] != 0 && map.flags[MAP_FLAG_CEILINGTILE] < numtiles )
	{
		mapceilingtile = map.flags[MAP_FLAG_CEILINGTILE];
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);

	if ( clouds && mode == REALCOLORS )
	{
		// draw sky "box"
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
		perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 16);
		GLfloat rotx = camera->vang * 180 / PI; // get x rotation
		GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		GLfloat rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glEnable( GL_DEPTH_TEST );
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);

		// first (higher) sky layer
		glColor4f(1.f, 1.f, 1.f, .5);
		glBindTexture(GL_TEXTURE_2D, texid[tiles[cloudtile]->refcount]); // sky tile
		glBegin( GL_QUADS );
		glTexCoord2f((real_t)(ticks % 60) / 60, (real_t)(ticks % 60) / 60);
		glVertex3f(-CLIPFAR * 16, 64, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (real_t)(ticks % 60) / 60, (real_t)(ticks % 60) / 60);
		glVertex3f(CLIPFAR * 16, 64, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (real_t)(ticks % 60) / 60, (CLIPFAR) / 2 + (real_t)(ticks % 60) / 60);
		glVertex3f(CLIPFAR * 16, 64, CLIPFAR * 16);

		glTexCoord2f((real_t)(ticks % 60) / 60, (CLIPFAR) / 2 + (real_t)(ticks % 60) / 60);
		glVertex3f(-CLIPFAR * 16, 64, CLIPFAR * 16);
		glEnd();

		// second (closer) sky layer
		glColor4f(1.f, 1.f, 1.f, .5);
		glBindTexture(GL_TEXTURE_2D, texid[tiles[cloudtile]->refcount]); // sky tile
		glBegin( GL_QUADS );
		glTexCoord2f((real_t)(ticks % 240) / 240, (real_t)(ticks % 240) / 240);
		glVertex3f(-CLIPFAR * 16, 32, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (real_t)(ticks % 240) / 240, (real_t)(ticks % 240) / 240);
		glVertex3f(CLIPFAR * 16, 32, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (real_t)(ticks % 240) / 240, (CLIPFAR) / 2 + (real_t)(ticks % 240) / 240);
		glVertex3f(CLIPFAR * 16, 32, CLIPFAR * 16);

		glTexCoord2f((real_t)(ticks % 240) / 240, (CLIPFAR) / 2 + (real_t)(ticks % 240) / 240);
		glVertex3f(-CLIPFAR * 16, 32, CLIPFAR * 16);
		glEnd();
	}

	// setup projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR * 2);
	GLfloat rotx = camera->vang * 180 / PI; // get x rotation
	GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
	GLfloat rotz = 0; // get z rotation
	glRotatef(rotx, 1, 0, 0); // rotate pitch
	glRotatef(roty, 0, 1, 0); // rotate yaw
	glRotatef(rotz, 0, 0, 1); // rotate roll
	glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glEnable( GL_DEPTH_TEST );
	glDepthMask(GL_TRUE);
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	// glBegin / glEnd are also moved outside, 
	// but needs to track the texture used to "flush" current drawing before switching
	GLuint cur_tex = 0, new_tex = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	for ( x = 0; x < map.width; x++ )
	{
		for ( y = 0; y < map.height; y++ )
		{
			if ( x >= (int)camera->x - 3 && x <= (int)camera->x + 3 && y >= (int)camera->y - 3 && y <= (int)camera->y + 3 )
			{
				vismap[y + x * map.height] = true;
			}
			if ( vismap[y + x * map.height] )
			{
				for ( z = 0; z < MAPLAYERS + 1; z++ )
				{
					index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;

					if ( z >= 0 && z < MAPLAYERS )
					{
						// skip "air" tiles
						if ( map.tiles[index] == 0 )
						{
							continue;
						}

						// bind texture
						if ( mode == REALCOLORS )
						{
							if ( map.tiles[index] < 0 || map.tiles[index] >= numtiles )
							{
								new_tex = texid[sprites[0]->refcount];
								//glBindTexture(GL_TEXTURE_2D, texid[sprites[0]->refcount]);
							}
							else
							{
								new_tex = texid[tiles[map.tiles[index]]->refcount];
								//glBindTexture(GL_TEXTURE_2D, texid[tiles[map.tiles[index]]->refcount]);
							}
						}
						else
						{
							new_tex = 0;
							//glBindTexture(GL_TEXTURE_2D, 0);
						}
						// check if the texture has changed (flushing drawing if it's the case)
						if(new_tex != cur_tex)
						{
							glEnd();
							glBindTexture(GL_TEXTURE_2D, new_tex);
							cur_tex=new_tex;
							glBegin(GL_QUADS);
						}

						// draw east wall
						if ( x == map.width - 1 || !map.tiles[index + MAPLAYERS * map.height] )
						{
							if ( smoothlighting && mode == REALCOLORS )
							{
								//glBegin( GL_QUADS );
								if ( z )
								{
									s = getLightAt(x + 1, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									s = getLightAt(x + 1, y);
									glColor3f(s, s, s);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
								}
								else
								{
									s = getLightAt(x + 1, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									glColor3f(0, 0, 0);
									glTexCoord2f(0, 2);
									glVertex3f(x * 32 + 32, z * 32 - 48 - 32, y * 32 + 32);
									s = getLightAt(x + 1, y);
									glColor3f(0, 0, 0);
									glTexCoord2f(1, 2);
									glVertex3f(x * 32 + 32, z * 32 - 48 - 32, y * 32 + 0);
									glColor3f(s, s, s);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
								}
								//glEnd();
							}
							else
							{
								if ( mode == REALCOLORS )
								{
									if ( x < map.width - 1 )
									{
										s = std::min(std::max(0, lightmapSmoothed[y + (x + 1) * map.height]), 255) / 255.0;
										if ( globalLightModifierActive )
										{
											s *= globalLightModifier;
										}
									}
									else
									{
										s = .5;
									}
									glColor3f(s, s, s);
								}
								else
								{
									glColor4ub(0, 0, 0, 0);
								}
								if ( x == map.width - 1 || !map.tiles[z + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] )
								{
									//glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									//glEnd();
								}
							}
						}

						// draw south wall
						if ( y == map.height - 1 || !map.tiles[index + MAPLAYERS] )
						{
							if ( smoothlighting && mode == REALCOLORS )
							{
								//glBegin( GL_QUADS );
								if ( z )
								{
									s = getLightAt(x, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									s = getLightAt(x + 1, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
								}
								else
								{
									s = getLightAt(x, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									glColor3f(0, 0, 0);
									glTexCoord2f(0, 2);
									glVertex3f(x * 32 + 0, z * 32 - 48 - 32, y * 32 + 32);
									s = getLightAt(x + 1, y + 1);
									glColor3f(0, 0, 0);
									glTexCoord2f(1, 2);
									glVertex3f(x * 32 + 32, z * 32 - 48 - 32, y * 32 + 32);
									glColor3f(s, s, s);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
								}
								//glEnd();
							}
							else
							{
								if ( mode == REALCOLORS )
								{
									if ( y < map.height - 1 )
									{
										s = std::min(std::max(0, lightmapSmoothed[(y + 1) + x * map.height]), 255) / 255.0;
										if ( globalLightModifierActive )
										{
											s *= globalLightModifier;
										}
									}
									else
									{
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( y == map.height - 1 || !map.tiles[z + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
								{
									//glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									//glEnd();
								}
							}
						}

						// draw west wall
						if ( x == 0 || !map.tiles[index - MAPLAYERS * map.height] )
						{
							if ( smoothlighting && mode == REALCOLORS )
							{
								//glBegin( GL_QUADS );
								if ( z )
								{
									s = getLightAt(x, y);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									s = getLightAt(x, y + 1);
									glColor3f(s, s, s);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
								}
								else
								{
									s = getLightAt(x, y);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									glColor3f(0, 0, 0);
									glTexCoord2f(0, 2);
									glVertex3f(x * 32 + 0, z * 32 - 48 - 32, y * 32 + 0);
									s = getLightAt(x, y + 1);
									glColor3f(0, 0, 0);
									glTexCoord2f(1, 2);
									glVertex3f(x * 32 + 0, z * 32 - 48 - 32, y * 32 + 32);
									glColor3f(s, s, s);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
								}
								//glEnd();
							}
							else
							{
								if ( mode == REALCOLORS )
								{
									if ( x > 0 )
									{
										s = std::min(std::max(0, lightmapSmoothed[y + (x - 1) * map.height]), 255) / 255.0;
										if ( globalLightModifierActive )
										{
											s *= globalLightModifier;
										}
									}
									else
									{
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( x == 0 || !map.tiles[z + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] )
								{
									//glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									//glEnd();
								}
							}
						}

						// draw north wall
						if ( y == 0 || !map.tiles[index - MAPLAYERS] )
						{
							if ( smoothlighting && mode == REALCOLORS )
							{
								//glBegin( GL_QUADS );
								if ( z )
								{
									s = getLightAt(x + 1, y);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									s = getLightAt(x, y);
									glColor3f(s, s, s);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
								}
								else
								{
									s = getLightAt(x + 1, y);
									glColor3f(s, s, s);
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									glColor3f(0, 0, 0);
									glTexCoord2f(0, 2);
									glVertex3f(x * 32 + 32, z * 32 - 48 - 32, y * 32 + 0);
									s = getLightAt(x, y);
									glColor3f(0, 0, 0);
									glTexCoord2f(1, 2);
									glVertex3f(x * 32 + 0, z * 32 - 48 - 32, y * 32 + 0);
									glColor3f(s, s, s);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
								}
								//glEnd();
							}
							else
							{
								if ( mode == REALCOLORS )
								{
									if ( y > 0 )
									{
										s = std::min(std::max(0, lightmapSmoothed[(y - 1) + x * map.height]), 255) / 255.0;
										if ( globalLightModifierActive )
										{
											s *= globalLightModifier;
										}
									}
									else
									{
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( y == 0 || !map.tiles[z + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] )
								{
									//glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									//glEnd();
								}
							}
						}
					}
					else
					{
						// bind texture
						if ( mode == REALCOLORS )
						{
							new_tex = texid[tiles[mapceilingtile]->refcount];
							//glBindTexture(GL_TEXTURE_2D, texid[tiles[50]->refcount]); // rock tile
							if (cur_tex!=new_tex)
							{
								glEnd();
								cur_tex = new_tex;
								glBindTexture(GL_TEXTURE_2D, new_tex);
								glBegin(GL_QUADS);
							}
						}
						else
						{
							continue;
						}
					}

					if ( smoothlighting && mode == REALCOLORS )
					{
						// draw floor
						if ( z < OBSTACLELAYER )
						{
							if ( !map.tiles[index + 1] )
							{
								//glBegin( GL_QUADS );
								s = getLightAt(x, y);
								glColor3f(s, s, s);
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 0);
								s = getLightAt(x, y + 1);
								glColor3f(s, s, s);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 32);
								s = getLightAt(x + 1, y + 1);
								glColor3f(s, s, s);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 32);
								s = getLightAt(x + 1, y);
								glColor3f(s, s, s);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 0);
								//glEnd();
							}
						}

						// draw ceiling
						else if ( z > OBSTACLELAYER && (!clouds || z < MAPLAYERS) )
						{
							if ( !map.tiles[index - 1] )
							{
								//glBegin( GL_QUADS );
								s = getLightAt(x, y);
								glColor3f(s, s, s);
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 0);
								s = getLightAt(x + 1, y);
								glColor3f(s, s, s);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 0);
								s = getLightAt(x + 1, y + 1);
								glColor3f(s, s, s);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 32);
								s = getLightAt(x, y + 1);
								glColor3f(s, s, s);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 32);
								//glEnd();
							}
						}
					}
					else
					{
						// unsmooth lighting
						if ( mode == REALCOLORS )
						{
							s = std::min(std::max(0, lightmapSmoothed[y + x * map.height]), 255) / 255.0;
							glColor3f(s, s, s);
						}

						// draw floor
						if ( z < OBSTACLELAYER )
						{
							if ( !map.tiles[index + 1] )
							{
								//glBegin( GL_QUADS );
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 0);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 32);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 32);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 0);
								//glEnd();
							}
						}

						// draw ceiling
						else if ( z > OBSTACLELAYER )
						{
							if ( !map.tiles[index - 1] )
							{
								//glBegin( GL_QUADS );
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 0);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 0);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 32);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 32);
								//glEnd();
							}
						}
					}
				}
			}
		}
	}
	glEnd();

	glDisable(GL_SCISSOR_TEST);
	glScissor(0, 0, xres, yres);
}

static int dirty = 1;
static int oldx = 0, oldy = 0;
static unsigned int oldpix = 0;

unsigned int GO_GetPixelU32(int x, int y, view_t& camera)
{
	if(!dirty && (oldx==x) && (oldy==y))
		return oldpix;

	if(dirty) {
#ifdef PANDORA
		// Pandora fbo
		if((xres==800) && (yres==480)) {
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);
		}
#endif
		// generate object buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glDrawWorld(&camera, ENTITYUIDS);
		drawEntities3D(&camera, ENTITYUIDS);
	}

	GLubyte pixel[4];
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixel);
	oldpix = pixel[0] + (((Uint32)pixel[1]) << 8) + (((Uint32)pixel[2]) << 16) + (((Uint32)pixel[3]) << 24);
#ifdef PANDORA
	if((dirty) && (xres==800) && (yres==480)) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
#endif
	dirty = 0;
	return oldpix;
}

void GO_SwapBuffers(SDL_Window* screen)
{
	dirty = 1;
#ifdef PANDORA
	bool bBlit = !(xres==800 && yres==480);

	int vp_old[4];
	if(bBlit) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glGetIntegerv(GL_VIEWPORT, vp_old);
		glViewport(0, 0, 800, 480);
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho(0, 800, 480, 0, 1, -1);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		glBindTexture(GL_TEXTURE_2D, fbo_tex);
		glColor4f(1,1,1,1);

		glBegin(GL_QUADS);
		 glTexCoord2f(0,yres/1024.0f); glVertex2f(0,0);
		 glTexCoord2f(0, 0); glVertex2f(0,480);
		 glTexCoord2f(xres/1024.0f, 0); glVertex2f(800,480);
		 glTexCoord2f(xres/1024.0f, yres/1024.0f); glVertex2f(800,0);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif
#ifdef APPLE
	SDL_RenderPresent(renderer);
#else
	SDL_GL_SwapWindow(screen);
#endif
#ifdef PANDORA
	if(bBlit) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);
		glViewport(vp_old[0], vp_old[1], vp_old[2], vp_old[3]);
	}
#endif
}
