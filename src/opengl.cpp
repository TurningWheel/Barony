/*-------------------------------------------------------------------------------

	BARONY
	File: opengl.cpp
	Desc: contains all drawing functions for opengl

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "entity.hpp"

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

/*-------------------------------------------------------------------------------

	getLightForEntity

	Returns a shade factor (0.0-1.0) to shade an entity with, based on
	its surroundings

-------------------------------------------------------------------------------*/

double getLightForEntity(double x, double y) {
	if ( x < 0 || y < 0 || x >= map.width || y >= map.height ) {
		return 1.f;
	}
	int u = x;
	int v = y;
	return std::min(std::max(0, lightmap[v + u * map.height]), 255) / 255.0;
}

/*-------------------------------------------------------------------------------

	glDrawVoxel

	Draws a voxel model at the given world coordinates

-------------------------------------------------------------------------------*/

bool wholevoxels = FALSE;
void glDrawVoxel(view_t* camera, Entity* entity, int mode) {
	double dx, dy, dz;
	int voxX, voxY, voxZ;
	double s = 1;
	//int x = 0;
	//int y = 0;
	Sint32 index;
	Sint32 indexdown[3];
	voxel_t* model;
	int modelindex = 0;
	GLfloat rotx, roty, rotz;
	//GLuint uidcolor;

	// assign model
	if ( entity->sprite >= 0 && entity->sprite < nummodels ) {
		if ( models[entity->sprite] != NULL ) {
			model = models[entity->sprite];
		} else {
			model = models[0];
		}
		modelindex = entity->sprite;
	} else {
		model = models[0];
		modelindex = 0;
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
	gluPerspective(fov, (double)camera->winw / (double)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable( GL_DEPTH_TEST );
	if ( !entity->flags[OVERDRAW] ) {
		rotx = camera->vang * 180 / PI; // get x rotation
		roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	} else {
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
	if ( mode == REALCOLORS ) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	if ( entity->flags[OVERDRAW] ) {
		glDepthRange(0, 0.1);
	}

	// get shade factor
	if (!entity->flags[BRIGHT]) {
		if (!entity->flags[OVERDRAW]) {
			s = getLightForEntity(entity->x / 16, entity->y / 16);
		} else {
			s = getLightForEntity(camera->x, camera->y);
		}
	}

	if ( wholevoxels ) {
		for ( index = 0, voxX = 0; voxX < model->sizex; voxX++ ) {
			for ( voxY = 0; voxY < model->sizey; voxY++ ) {
				for ( voxZ = 0; voxZ < model->sizez; voxZ++, index++ ) {
					// get the bit color
					if ( model->data[index] == 255 || model->data[index] == 0 ) {
						continue;
					}
					if ( mode == REALCOLORS ) {
						glColor3f((model->palette[model->data[index]][0] / 255.0)*s, (model->palette[model->data[index]][1] / 255.0)*s, (model->palette[model->data[index]][2] / 255.0)*s );
					} else {
						glColor4ub((Uint8)(entity->uid), (Uint8)(entity->uid >> 8), (Uint8)(entity->uid >> 16), (Uint8)(entity->uid >> 24));
					}

					// calculate model offsets
					dx = (double)voxX - ((double)model->sizex) / 2.f;
					dy = (double)voxY - ((double)model->sizey) / 2.f;
					dz = ((double)model->sizez) / 2.f - (double)voxZ;

					// draw front of cube
					bool drawFront = FALSE;
					if ( voxX == model->sizex - 1 ) {
						drawFront = TRUE;
					} else if ( model->data[index + indexdown[0]] == 255 ) {
						drawFront = TRUE;
					}
					if ( drawFront ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						glEnd();
					}

					// draw back of cube
					bool drawBack = FALSE;
					if ( voxX == 0 ) {
						drawBack = TRUE;
					} else if ( model->data[index - indexdown[0]] == 255 ) {
						drawBack = TRUE;
					}
					if ( drawBack ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 0, dz + 0, dy + 0);
						glEnd();
					}

					// draw right side of cube
					bool drawRight = FALSE;
					if ( voxY == model->sizey - 1 ) {
						drawRight = TRUE;
					} else if ( model->data[index + indexdown[1]] == 255 ) {
						drawRight = TRUE;
					}
					if ( drawRight ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						glEnd();
					}

					// draw left side of cube
					bool drawLeft = FALSE;
					if ( voxY == 0 ) {
						drawLeft = TRUE;
					} else if ( model->data[index - indexdown[1]] == 255 ) {
						drawLeft = TRUE;
					}
					if ( drawLeft ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 0);
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						glEnd();
					}

					// draw bottom of cube
					bool drawBottom = FALSE;
					if ( voxZ == model->sizez - 1 ) {
						drawBottom = TRUE;
					} else if ( model->data[index + indexdown[2]] == 255 ) {
						drawBottom = TRUE;
					}
					if ( drawBottom ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 0);
						glVertex3f(dx + 1, dz + 0, dy + 1);
						glVertex3f(dx + 0, dz + 0, dy + 1);
						glEnd();
					}

					// draw top of cube
					bool drawTop = FALSE;
					if ( voxZ == 0 ) {
						drawTop = TRUE;
					} else if ( model->data[index - indexdown[2]] == 255 ) {
						drawTop = TRUE;
					}
					if ( drawTop ) {
						glBegin( GL_QUADS );
						glVertex3f(dx + 0, dz + 1, dy + 0);
						glVertex3f(dx + 0, dz + 1, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 1);
						glVertex3f(dx + 1, dz + 1, dy + 0);
						glEnd();
					}
				}
			}
		}
	} else {
		if ( disablevbos ) {
			for ( index = 0; index < polymodels[modelindex].numfaces; index++ ) {
				if ( mode == REALCOLORS ) {
					if ( entity->flags[USERFLAG2] ) {
						glColor3f((polymodels[modelindex].faces[index].r / 255.f)*s, (polymodels[modelindex].faces[index].g / 255.f)*s, (polymodels[modelindex].faces[index].b / 255.f)*s );
					} else {
						glColor3f((polymodels[modelindex].faces[index].b / 255.f)*s, (polymodels[modelindex].faces[index].r / 255.f)*s, (polymodels[modelindex].faces[index].g / 255.f)*s );
					}
				} else {
					glColor4ub((Uint8)(entity->uid), (Uint8)(entity->uid >> 8), (Uint8)(entity->uid >> 16), (Uint8)(entity->uid >> 24));
				}

				polytriangle_t* face = &polymodels[modelindex].faces[index];

				glBegin( GL_TRIANGLES );
				glVertex3f(face->vertex[0].x, -face->vertex[0].z, face->vertex[0].y);
				glVertex3f(face->vertex[1].x, -face->vertex[1].z, face->vertex[1].y);
				glVertex3f(face->vertex[2].x, -face->vertex[2].z, face->vertex[2].y);
				glEnd();
			}
		} else {
			SDL_glBindVertexArray(polymodels[modelindex].va);
			SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].vbo);
			glVertexPointer( 3, GL_FLOAT, 0, (char*) NULL );  // Set The Vertex Pointer To The Vertex Buffer
			glEnableClientState(GL_VERTEX_ARRAY); // enable the vertex array on the client side
			if ( mode == REALCOLORS ) {
				glEnableClientState(GL_COLOR_ARRAY); // enable the color array on the client side
				if ( entity->flags[USERFLAG2] ) {
					SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors_shifted);
				} else {
					SDL_glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
				}
				glColorPointer(3, GL_FLOAT, 0, 0);
				GLfloat params_col[4] = {static_cast<GLfloat>(s), static_cast<GLfloat>(s), static_cast<GLfloat>(s), 1.f};
				glEnable(GL_LIGHTING);
				glEnable(GL_COLOR_MATERIAL);
				glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params_col);
			} else {
				GLfloat uidcolors[4];
				uidcolors[0] = ((Uint8)(entity->uid)) / 255.f;
				uidcolors[1] = ((Uint8)(entity->uid >> 8)) / 255.f;
				uidcolors[2] = ((Uint8)(entity->uid >> 16)) / 255.f;
				uidcolors[3] = ((Uint8)(entity->uid >> 24)) / 255.f;
				glColor4f(uidcolors[0], uidcolors[1], uidcolors[2], uidcolors[3]);
			}
			glDrawArrays(GL_TRIANGLES, 0, 3 * polymodels[modelindex].numfaces);
			if ( mode == REALCOLORS ) {
				glDisable(GL_COLOR_MATERIAL);
				glDisable(GL_LIGHTING);
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

void glDrawSprite(view_t* camera, Entity* entity, int mode) {
	SDL_Surface* sprite;
	//int x, y;
	double s = 1;

	// setup projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	gluPerspective(fov, (double)camera->winw / (double)camera->winh, CLIPNEAR, CLIPFAR * 2);
	glEnable( GL_DEPTH_TEST );
	if (!entity->flags[OVERDRAW]) {
		GLfloat rotx = camera->vang * 180 / PI; // get x rotation
		GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
		GLfloat rotz = 0; // get z rotation
		glRotatef(rotx, 1, 0, 0); // rotate pitch
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate roll
		glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position
	} else {
		glRotatef(90, 0, 1, 0);

	}

	// setup model matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glPushMatrix();
	if ( mode == REALCOLORS ) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	// assign texture
	if ( entity->sprite >= 0 && entity->sprite < numsprites ) {
		if ( sprites[entity->sprite] != NULL ) {
			sprite = sprites[entity->sprite];
		} else {
			sprite = sprites[0];
		}
	} else {
		sprite = sprites[0];
	}
	if ( mode == REALCOLORS ) {
		glBindTexture(GL_TEXTURE_2D, texid[sprite->refcount]);
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if (!entity->flags[OVERDRAW]) {
		double tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	} else {
		double tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] ) {
		glDepthRange(0, 0.1);
	}

	// get shade factor
	if ( mode == REALCOLORS ) {
		if (!entity->flags[BRIGHT]) {
			if (!entity->flags[OVERDRAW]) {
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			} else {
				s = getLightForEntity(camera->x, camera->y);
			}
			glColor4f(s, s, s, 1);
		} else {
			glColor4f(1.f, 1.f, 1.f, 1);
		}
	} else {
		glColor4ub((Uint8)(entity->uid), (Uint8)(entity->uid >> 8), (Uint8)(entity->uid >> 16), (Uint8)(entity->uid >> 24));
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

/*-------------------------------------------------------------------------------

	getLightAt

	returns the light shade factor for the vertex at the given x/y point

-------------------------------------------------------------------------------*/

double getLightAt(int x, int y) {
	double l = 0;
	int u, v;

	for ( u = x - 1; u < x + 1; u++ ) {
		for ( v = y - 1; v < y + 1; v++ ) {
			if ( u >= 0 && u < map.width && v >= 0 && v < map.height ) {
				l += std::min(std::max(0, lightmap[v + u * map.height]), 255) / 255.0;
			}
		}
	}
	return l / 4.f;
}

/*-------------------------------------------------------------------------------

	glDrawWorld

	Draws the current map from the given camera point

-------------------------------------------------------------------------------*/

void glDrawWorld(view_t* camera, int mode) {
	int x, y, z;
	int index;
	double s;
	bool clouds = FALSE;

	if ( softwaremode == TRUE ) {
		return;
	}

	if ( !strncmp(map.name, "Hell", 4) && smoothlighting ) {
		clouds = TRUE;
	}

	if ( clouds && mode == REALCOLORS ) {
		// draw sky "box"
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
		gluPerspective(fov, (double)camera->winw / (double)camera->winh, CLIPNEAR, CLIPFAR * 16);
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
		glBindTexture(GL_TEXTURE_2D, texid[tiles[77]->refcount]); // sky tile
		glBegin( GL_QUADS );
		glTexCoord2f((double)(ticks % 60) / 60, (double)(ticks % 60) / 60);
		glVertex3f(-CLIPFAR * 16, 64, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (double)(ticks % 60) / 60, (double)(ticks % 60) / 60);
		glVertex3f(CLIPFAR * 16, 64, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (double)(ticks % 60) / 60, (CLIPFAR) / 2 + (double)(ticks % 60) / 60);
		glVertex3f(CLIPFAR * 16, 64, CLIPFAR * 16);

		glTexCoord2f((double)(ticks % 60) / 60, (CLIPFAR) / 2 + (double)(ticks % 60) / 60);
		glVertex3f(-CLIPFAR * 16, 64, CLIPFAR * 16);
		glEnd();

		// second (closer) sky layer
		glColor4f(1.f, 1.f, 1.f, .5);
		glBindTexture(GL_TEXTURE_2D, texid[tiles[77]->refcount]); // sky tile
		glBegin( GL_QUADS );
		glTexCoord2f((double)(ticks % 240) / 240, (double)(ticks % 240) / 240);
		glVertex3f(-CLIPFAR * 16, 32, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (double)(ticks % 240) / 240, (double)(ticks % 240) / 240);
		glVertex3f(CLIPFAR * 16, 32, -CLIPFAR * 16);

		glTexCoord2f((CLIPFAR) / 2 + (double)(ticks % 240) / 240, (CLIPFAR) / 2 + (double)(ticks % 240) / 240);
		glVertex3f(CLIPFAR * 16, 32, CLIPFAR * 16);

		glTexCoord2f((double)(ticks % 240) / 240, (CLIPFAR) / 2 + (double)(ticks % 240) / 240);
		glVertex3f(-CLIPFAR * 16, 32, CLIPFAR * 16);
		glEnd();
	}

	// setup projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	gluPerspective(fov, (double)camera->winw / (double)camera->winh, CLIPNEAR, CLIPFAR * 2);
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
	if ( mode == REALCOLORS ) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	for ( x = 0; x < map.width; x++ ) {
		for ( y = 0; y < map.height; y++ ) {
			if ( x >= (int)camera->x - 3 && x <= (int)camera->x + 3 && y >= (int)camera->y - 3 && y <= (int)camera->y + 3 ) {
				vismap[y + x * map.height] = TRUE;
			}
			if ( vismap[y + x * map.height] ) {
				for ( z = 0; z < MAPLAYERS + 1; z++ ) {
					index = z + y * MAPLAYERS + x * MAPLAYERS * map.height;

					if ( z >= 0 && z < MAPLAYERS ) {
						// skip "air" tiles
						if ( map.tiles[index] == 0 ) {
							continue;
						}

						// bind texture
						if ( mode == REALCOLORS ) {
							if ( map.tiles[index] < 0 || map.tiles[index] >= numtiles ) {
								glBindTexture(GL_TEXTURE_2D, texid[sprites[0]->refcount]);
							} else {
								glBindTexture(GL_TEXTURE_2D, texid[tiles[map.tiles[index]]->refcount]);
							}
						} else {
							glBindTexture(GL_TEXTURE_2D, 0);
						}

						// draw east wall
						if ( x == map.width - 1 || !map.tiles[index + MAPLAYERS * map.height] ) {
							if ( smoothlighting && mode == REALCOLORS ) {
								glBegin( GL_QUADS );
								if ( z ) {
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
								} else {
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
								glEnd();
							} else {
								if ( mode == REALCOLORS ) {
									if ( x < map.width - 1 ) {
										s = std::min(std::max(0, lightmap[y + (x + 1) * map.height]), 255) / 255.0;
									} else {
										s = .5;
									}
									glColor3f(s, s, s);
								} else {
									glColor4ub(0, 0, 0, 0);
								}
								if ( x == map.width - 1 || !map.tiles[z + y * MAPLAYERS + (x + 1)*MAPLAYERS * map.height] ) {
									glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									glEnd();
								}
							}
						}

						// draw south wall
						if ( y == map.height - 1 || !map.tiles[index + MAPLAYERS] ) {
							if ( smoothlighting && mode == REALCOLORS ) {
								glBegin( GL_QUADS );
								if ( z ) {
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
								} else {
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
								glEnd();
							} else {
								if ( mode == REALCOLORS ) {
									if ( y < map.height - 1 ) {
										s = std::min(std::max(0, lightmap[(y + 1) + x * map.height]), 255) / 255.0;
									} else {
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( y == map.height - 1 || !map.tiles[z + (y + 1)*MAPLAYERS + x * MAPLAYERS * map.height] ) {
									glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 32);
									glEnd();
								}
							}
						}

						// draw west wall
						if ( x == 0 || !map.tiles[index - MAPLAYERS * map.height] ) {
							if ( smoothlighting && mode == REALCOLORS ) {
								glBegin( GL_QUADS );
								if ( z ) {
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
								} else {
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
								glEnd();
							} else {
								if ( mode == REALCOLORS ) {
									if ( x > 0 ) {
										s = std::min(std::max(0, lightmap[y + (x - 1) * map.height]), 255) / 255.0;
									} else {
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( x == 0 || !map.tiles[z + y * MAPLAYERS + (x - 1)*MAPLAYERS * map.height] ) {
									glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 32);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 32);
									glEnd();
								}
							}
						}

						// draw north wall
						if ( y == 0 || !map.tiles[index - MAPLAYERS] ) {
							if ( smoothlighting && mode == REALCOLORS ) {
								glBegin( GL_QUADS );
								if ( z ) {
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
								} else {
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
								glEnd();
							} else {
								if ( mode == REALCOLORS ) {
									if ( y > 0 ) {
										s = std::min(std::max(0, lightmap[(y - 1) + x * map.height]), 255) / 255.0;
									} else {
										s = .5;
									}
									glColor3f(s, s, s);
								}
								if ( y == 0 || !map.tiles[z + (y - 1)*MAPLAYERS + x * MAPLAYERS * map.height] ) {
									glBegin( GL_QUADS );
									glTexCoord2f(0, 0);
									glVertex3f(x * 32 + 32, z * 32 - 16, y * 32 + 0);
									glTexCoord2f(0, 1);
									glVertex3f(x * 32 + 32, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 1);
									glVertex3f(x * 32 + 0, z * 32 - 48, y * 32 + 0);
									glTexCoord2f(1, 0);
									glVertex3f(x * 32 + 0, z * 32 - 16, y * 32 + 0);
									glEnd();
								}
							}
						}
					} else {
						// bind texture
						if ( mode == REALCOLORS ) {
							glBindTexture(GL_TEXTURE_2D, texid[tiles[50]->refcount]); // rock tile
						} else {
							continue;
						}
					}

					if ( smoothlighting && mode == REALCOLORS ) {
						// draw floor
						if ( z < OBSTACLELAYER ) {
							if ( !map.tiles[index + 1] ) {
								glBegin( GL_QUADS );
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
								glEnd();
							}
						}

						// draw ceiling
						else if ( z > OBSTACLELAYER && (!clouds || z < MAPLAYERS) ) {
							if ( !map.tiles[index - 1] ) {
								glBegin( GL_QUADS );
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
								glEnd();
							}
						}
					} else {
						// unsmooth lighting
						if ( mode == REALCOLORS ) {
							s = std::min(std::max(0, lightmap[y + x * map.height]), 255) / 255.0;
							glColor3f(s, s, s);
						}

						// draw floor
						if ( z < OBSTACLELAYER ) {
							if ( !map.tiles[index + 1] ) {
								glBegin( GL_QUADS );
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 0);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, -16 - 32 * abs(z), y * 32 + 32);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 32);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, -16 - 32 * abs(z), y * 32 + 0);
								glEnd();
							}
						}

						// draw ceiling
						else if ( z > OBSTACLELAYER ) {
							if ( !map.tiles[index - 1] ) {
								glBegin( GL_QUADS );
								glTexCoord2f(0, 0);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 0);
								glTexCoord2f(1, 0);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 0);
								glTexCoord2f(1, 1);
								glVertex3f(x * 32 + 32, 16 + 32 * abs(z - 2), y * 32 + 32);
								glTexCoord2f(0, 1);
								glVertex3f(x * 32 + 0, 16 + 32 * abs(z - 2), y * 32 + 32);
								glEnd();
							}
						}
					}
				}
			}
		}
	}
}

/*GLuint create_shader(const char* filename, GLenum type)
{
	FILE* input = fopen(filename, "rb");
	if( !input ) {
		printlog("Couldn't open shader file \"%s\"", filename);
		return 0;
		//TODO: Error.
	}
	fclose(input);
	const GLchar* source=NULL;
	if (source == NULL) {
		printlog("Error opening %s: ", filename); perror("");
		return 0;
	}
	GLuint res = glCreateShader(type);
	const GLchar* sources[2] = {
#ifdef GL_ES_VERSION_2_0
	"#version 100\n"
	"#define GLES2\n",
#else
	"#version 120\n",
#endif
	source };
	glShaderSource(res, 2, sources, NULL);
	free((void*)source);

	glCompileShader(res);
	GLint compile_ok = GL_FALSE;
	glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
	if (compile_ok == GL_FALSE) {
		printlog("%s:", filename);
		glDeleteShader(res);
		return 0;
	}

	return res;
}
*/
