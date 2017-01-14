/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains all drawing code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "hash.hpp"
#include "entity.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	getPixel

	gets the value of a pixel at the given x,y location in the given
	SDL_Surface

-------------------------------------------------------------------------------*/

Uint32 getPixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to retrieve
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			return *p;
			break;

		case 2:
			return *(Uint16 *)p;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				return p[0] << 16 | p[1] << 8 | p[2];
			} else {
				return p[0] | p[1] << 8 | p[2] << 16;
			}
			break;

		case 4:
			return *(Uint32 *)p;
			break;

		default:
			return 0;	   /* shouldn't happen, but avoids warnings */
	}
}

/*-------------------------------------------------------------------------------

	putPixel

	sets the value of a pixel at the given x,y location in the given
	SDL_Surface

-------------------------------------------------------------------------------*/

void putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to set
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *)p = pixel;
			break;

		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			} else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = pixel;
			break;
	}
}

/*-------------------------------------------------------------------------------

	flipSurface

	flips the contents of an SDL_Surface horizontally, vertically, or both

-------------------------------------------------------------------------------*/

SDL_Surface *flipSurface( SDL_Surface *surface, int flags ) {
	SDL_Surface *flipped = NULL;
	Uint32 pixel;
	int x, rx;
	int y, ry;

	// prepare surface for flipping
	flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );
	if( SDL_MUSTLOCK( surface ) ) {
		SDL_LockSurface( surface );
	}
	if( SDL_MUSTLOCK( flipped ) ) {
		SDL_LockSurface( flipped );
	}

	for( x=0, rx=flipped->w-1; x<flipped->w; x++, rx-- ) {
		for( y=0, ry=flipped->h-1; y<flipped->h; y++, ry-- ) {
			pixel = getPixel( surface, x, y );

			// copy pixel
			if( ( flags & FLIP_VERTICAL ) && ( flags & FLIP_HORIZONTAL ) ) {
				putPixel( flipped, rx, ry, pixel );
			} else if( flags & FLIP_HORIZONTAL ) {
				putPixel( flipped, rx, y, pixel );
			} else if( flags & FLIP_VERTICAL ) {
				putPixel( flipped, x, ry, pixel );
			}
		}
	}

	// restore image
	if( SDL_MUSTLOCK( surface ) ) {
		SDL_UnlockSurface( surface );
	}
	if( SDL_MUSTLOCK( flipped ) ) {
		SDL_UnlockSurface( flipped );
	}

	return flipped;
}

/*-------------------------------------------------------------------------------

	drawCircle

	draws a circle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawCircle( int x, int y, double radius, Uint32 color, Uint8 alpha ) {
	drawArc(x,y,radius,0,360,color,alpha);
}

/*-------------------------------------------------------------------------------

	drawArc

	draws an arc in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawArc( int x, int y, double radius, double angle1, double angle2, Uint32 color, Uint8 alpha ) {
	int c;

	// update projection
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// set line width
	GLint lineWidth;
	glGetIntegerv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(2);

	// draw line
	glColor4f(((Uint8)(color>>mainsurface->format->Rshift))/255.f,((Uint8)(color>>mainsurface->format->Gshift))/255.f,((Uint8)(color>>mainsurface->format->Bshift))/255.f,alpha/255.f);
	glBindTexture(GL_TEXTURE_2D,0);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
	for( c=angle1; c<=angle2; c++) {
		float degInRad = c*PI/180.f;
		glVertex2f(x+ceil(cos(degInRad)*radius)+1,yres-(y+ceil(sin(degInRad)*radius)));
	}
	glEnd();
	glDisable(GL_LINE_SMOOTH);

	// reset line width
	glLineWidth(lineWidth);
}

/*-------------------------------------------------------------------------------

	drawLine

	draws a line in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawLine( int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha ) {
	// update projection
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// set line width
	GLint lineWidth;
	glGetIntegerv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(2);

	// draw line
	glColor4f(((Uint8)(color>>mainsurface->format->Rshift))/255.f,((Uint8)(color>>mainsurface->format->Gshift))/255.f,((Uint8)(color>>mainsurface->format->Bshift))/255.f,alpha/255.f);
	glBindTexture(GL_TEXTURE_2D,0);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINES);
	glVertex2f(x1+1, yres-y1);
	glVertex2f(x2+1, yres-y2);
	glEnd();
	glDisable(GL_LINE_SMOOTH);

	// reset line width
	glLineWidth(lineWidth);
}

/*-------------------------------------------------------------------------------

	drawRect

	draws a rectangle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

int drawRect( SDL_Rect *src, Uint32 color, Uint8 alpha ) {
	SDL_Rect secondsrc;

	// update projection
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// for the use of the whole screen
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=xres;
		secondsrc.h=yres;
		src = &secondsrc;
	}

	// draw quad
	glColor4f(((Uint8)(color>>mainsurface->format->Rshift))/255.f,((Uint8)(color>>mainsurface->format->Gshift))/255.f,((Uint8)(color>>mainsurface->format->Bshift))/255.f,alpha/255.f);
	glBindTexture(GL_TEXTURE_2D,0);
	glBegin(GL_QUADS);
	glVertex2f(src->x, yres-src->y);
	glVertex2f(src->x, yres-src->y-src->h);
	glVertex2f(src->x+src->w, yres-src->y-src->h);
	glVertex2f(src->x+src->w, yres-src->y);
	glEnd();
	return 0;
}


/*-------------------------------------------------------------------------------

	drawBox

	draws the border of a rectangle

-------------------------------------------------------------------------------*/
int drawBox(SDL_Rect *src, Uint32 color, Uint8 alpha) {
	drawLine(src->x, src->y, src->x + src->w, src->y, color, alpha); //Top.
	drawLine(src->x, src->y, src->x, src->y + src->h, color, alpha); //Left.
	drawLine(src->x + src->w, src->y, src->x + src->w, src->y + src->h, color, alpha); //Right.
	drawLine(src->x, src->y + src->h, src->x + src->w, src->y + src->h, color, alpha); //Bottom.
	return 0;
}

/*-------------------------------------------------------------------------------

	drawGear

	draws a gear (used for turning wheel splash)

-------------------------------------------------------------------------------*/

void drawGear(Sint16 x, Sint16 y, double size, Sint32 rotation) {
	Uint32 color;
	int c;
	Sint16 x1, y1, x2, y2;

	color = SDL_MapRGB(mainsurface->format,255,127,0);
	for( c=0; c<6; c++ ) {
		drawArc(x, y, size, 0+c*60+rotation, 30+c*60+rotation, color, 255);
		drawArc(x, y, (int)ceil(size*1.33), 30+c*60+4+rotation, 60+c*60-4+rotation, color, 255);
		x1 = ceil(size*cos((30+c*60+rotation)*(PI/180)))+x;
		y1 = ceil(size*sin((30+c*60+rotation)*(PI/180)))+y;
		x2 = ceil(size*cos((30+c*60+4+rotation)*(PI/180))*1.33)+x;
		y2 = ceil(size*sin((30+c*60+4+rotation)*(PI/180))*1.33)+y;
		drawLine(x1, y1, x2, y2, color, 255);
		x1 = ceil(size*cos((60+c*60+rotation)*(PI/180)))+x;
		y1 = ceil(size*sin((60+c*60+rotation)*(PI/180)))+y;
		x2 = ceil(size*cos((60+c*60-4+rotation)*(PI/180))*1.33)+x;
		y2 = ceil(size*sin((60+c*60-4+rotation)*(PI/180))*1.33)+y;
		drawLine(x1, y1, x2, y2, color, 255);
	}
	color = SDL_MapRGBA(mainsurface->format,191,63,0,255);
	drawCircle(x, y, size*.66, color, 255);
	color = SDL_MapRGBA(mainsurface->format,127,0,0,255);
	drawCircle(x, y, size*.25, color, 255);
}

/*-------------------------------------------------------------------------------

	drawImageRotatedAlpha

	blits an image in either an opengl or SDL context, rotating the image
	relative to the screen and taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageRotatedAlpha( SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, double angle, Uint8 alpha ) {
	SDL_Rect secondsrc;

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);
	glTranslatef(pos->x,yres-pos->y,0);
	glRotatef(-angle * 180 / PI,0.f,0.f,1.f);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	glColor4f(1,1,1,alpha/255.1);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(-src->w/2, src->h/2);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(-src->w/2, -src->h/2);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(src->w/2, -src->h/2);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(src->w/2, src->h/2);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageColor

	blits an image in either an opengl or SDL context while colorizing it

-------------------------------------------------------------------------------*/

void drawImageColor( SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, Uint32 color ) {
	SDL_Rect secondsrc;

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	double r = ((Uint8)(color>>mainsurface->format->Rshift))/255.f;
	double g = ((Uint8)(color>>mainsurface->format->Gshift))/255.f;
	double b = ((Uint8)(color>>mainsurface->format->Bshift))/255.f;
	double a = ((Uint8)(color>>mainsurface->format->Ashift))/255.f;
	glColor4f(r,g,b,a);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x, yres-pos->y);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageAlpha

	blits an image in either an opengl or SDL context, taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageAlpha( SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos, Uint8 alpha ) {
	SDL_Rect secondsrc;

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	glColor4f(1,1,1,alpha/255.1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x, yres-pos->y);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImage

	blits an image in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawImage( SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos ) {
	SDL_Rect secondsrc;

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	glColor4f(1,1,1,1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x, yres-pos->y);
	glTexCoord2f(1.0*((double)src->x/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*(((double)src->y+src->h)/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y-src->h);
	glTexCoord2f(1.0*(((double)src->x+src->w)/image->w), 1.0*((double)src->y/image->h));
	glVertex2f(pos->x+src->w, yres-pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageScaled

	blits an image in either an opengl or SDL context, scaling it

-------------------------------------------------------------------------------*/

void drawImageScaled( SDL_Surface *image, SDL_Rect *src, SDL_Rect *pos ) {
	SDL_Rect secondsrc;

	if( !image ) {
		return;
	}

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	glColor4f(1,1,1,1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex2f(pos->x, yres-pos->y);
	glTexCoord2f(0.f, 1.f);
	glVertex2f(pos->x, yres-pos->y-pos->h);
	glTexCoord2f(1.f, 1.f);
	glVertex2f(pos->x+pos->w, yres-pos->y-pos->h);
	glTexCoord2f(1.f, 0.f);
	glVertex2f(pos->x+pos->w, yres-pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	scaleSurface

	Scales an SDL_Surface to the given width and height.

-------------------------------------------------------------------------------*/

SDL_Surface* scaleSurface(SDL_Surface *Surface, Uint16 Width, Uint16 Height) {
	Sint32 x,y,o_x,o_y;

	if(!Surface || !Width || !Height) {
		return NULL;
	}

	SDL_Surface *_ret = SDL_CreateRGBSurface(Surface->flags, Width, Height, Surface->format->BitsPerPixel, Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);

	double _stretch_factor_x = (double)Width/(double)Surface->w;
	double _stretch_factor_y = (double)Height/(double)Surface->h;

	for(y = 0; y < Surface->h; y++)
		for(x = 0; x < Surface->w; x++)
			for(o_y = 0; o_y < _stretch_factor_y; ++o_y)
				for(o_x = 0; o_x < _stretch_factor_x; ++o_x) {
					putPixel(_ret, (Sint32)(_stretch_factor_x * x) + o_x, (Sint32)(_stretch_factor_y * y) + o_y, getPixel(Surface, x, y));
				}

	free(Surface);
	return _ret;
}

/*-------------------------------------------------------------------------------

	drawImageFancy

	blits an image in either an opengl or SDL context, while coloring,
	rotating, and scaling it

-------------------------------------------------------------------------------*/

void drawImageFancy( SDL_Surface *image, Uint32 color, double angle, SDL_Rect *src, SDL_Rect *pos ) {
	SDL_Rect secondsrc;

	if( !image ) {
		return;
	}

	// update projection
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);
	glTranslatef(pos->x,yres-pos->y,0);
	glRotatef(-angle * 180 / PI,0.f,0.f,1.f);

	// for the use of a whole image
	if( src==NULL ) {
		secondsrc.x=0;
		secondsrc.y=0;
		secondsrc.w=image->w;
		secondsrc.h=image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[image->refcount]);
	double r = ((Uint8)(color>>mainsurface->format->Rshift))/255.f;
	double g = ((Uint8)(color>>mainsurface->format->Gshift))/255.f;
	double b = ((Uint8)(color>>mainsurface->format->Bshift))/255.f;
	double a = ((Uint8)(color>>mainsurface->format->Ashift))/255.f;
	glColor4f(r,g,b,a);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(((double)src->x)/((double)image->w), ((double)src->y)/((double)image->h));
	glVertex2f(0, 0);
	glTexCoord2f(((double)src->x)/((double)image->w), ((double)(src->y+src->h))/((double)image->h));
	glVertex2f(0, -pos->h);
	glTexCoord2f(((double)(src->x+src->w))/((double)image->w), ((double)(src->y+src->h))/((double)image->h));
	glVertex2f(pos->w, -pos->h);
	glTexCoord2f(((double)(src->x+src->w))/((double)image->w), ((double)src->y)/((double)image->h));
	glVertex2f(pos->w, 0);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawSky3D

	Draws the sky as an image whose position depends upon the given camera
	position

-------------------------------------------------------------------------------*/

void drawSky3D( view_t *camera, SDL_Surface *tex ) {
	double screenfactor;
	int skyx, skyy;
	SDL_Rect dest;
	SDL_Rect src;

	// move the images differently depending upon the screen size
	screenfactor = xres/320.0;

	// bitmap offsets
	skyx = -camera->ang*((320*screenfactor)/(PI/2.0));
	skyy = (-114*screenfactor-camera->vang);

	src.x = -skyx;
	src.y = -skyy;
	src.w = (-skyx)+xres; // clip to the screen width
	src.h = (-skyy)+yres; // clip to the screen height
	dest.x = 0;
	dest.y = 0;
	dest.w = xres;
	dest.h = yres;

	drawImage(tex, &src, &dest);

	// draw the part of the last part of the sky (only appears when angle > 270 deg.)
	if( skyx < -960*screenfactor ) {
		dest.x = 1280*screenfactor+skyx;
		dest.y = 0;
		dest.w = xres;
		dest.h = yres;
		src.x = 0;
		src.y = -skyy;
		src.w = xres-(-skyx-1280*screenfactor);
		src.h = src.y+yres;
		drawImage(tex, &src, &dest);
	}
}

/*-------------------------------------------------------------------------------

	drawLayer / drawBackground / drawForeground

	Draws the world tiles that are viewable at the given camera coordinates

-------------------------------------------------------------------------------*/

void drawLayer(long camx, long camy, int z, map_t *map) {
	long x, y;
	long minx, miny, maxx, maxy;
	int index;
	SDL_Rect pos;

	minx = std::max<long int>(camx>>TEXTUREPOWER,0);
	maxx = std::min<long int>((camx>>TEXTUREPOWER)+xres/TEXTURESIZE+2,map->width); //TODO: Why are long int and unsigned int being compared?
	miny = std::max<long int>(camy>>TEXTUREPOWER,0);
	maxy = std::min<long int>((camy>>TEXTUREPOWER)+yres/TEXTURESIZE+2,map->height); //TODO: Why are long int and unsigned int being compared?
	for( y=miny; y<maxy; y++ ) {
		for( x=minx; x<maxx; x++ ) {
			index = map->tiles[z+y*MAPLAYERS+x*MAPLAYERS*map->height];
			if( index > 0) {
				pos.x = (x<<TEXTUREPOWER)-camx;
				pos.y = (y<<TEXTUREPOWER)-camy;
				pos.w = TEXTURESIZE;
				pos.h = TEXTURESIZE;
				if( index>=0 && index<numtiles ) {
					if( tiles[index] != NULL ) {
						drawImageScaled(tiles[index], NULL, &pos);
					} else {
						drawImageScaled(sprites[0], NULL, &pos);
					}
				} else {
					drawImageScaled(sprites[0], NULL, &pos);
				}
			}
		}
	}
}

void drawBackground(long camx, long camy) {
	long z;
	for( z=0; z<OBSTACLELAYER; z++ ) {
		drawLayer(camx,camy,z,&map);
	}
}

void drawForeground(long camx, long camy) {
	long z;
	for( z=OBSTACLELAYER; z<MAPLAYERS; z++ ) {
		drawLayer(camx,camy,z,&map);
	}
}

/*-------------------------------------------------------------------------------

	drawClearBuffers

	clears the screen and resets zbuffer and vismap

-------------------------------------------------------------------------------*/

void drawClearBuffers() {
	// empty video and input buffers
	if( zbuffer != NULL ) {
		memset( zbuffer, 0, xres*yres*sizeof(double) );
	}
	if( clickmap != NULL ) {
		memset( clickmap, 0, xres*yres*sizeof(Entity *) );
	}
	if( vismap != NULL ) {
		int c, i = map.width*map.height;
		for( c=0; c<i; c++ ) {
			vismap[c] = FALSE;
		}
	}

	// clear the screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	drawRect(NULL,0,255);
}

/*-------------------------------------------------------------------------------

	raycast

	Performs raycasting from the given camera's position through the
	environment to update minimap and vismap

-------------------------------------------------------------------------------*/

void raycast(view_t *camera, int mode) {
	long posx, posy;
	double fracx, fracy;
	long inx, iny, inx2, iny2;
	double rx, ry;
	double d, dstart, dend;
	long sx;
	double arx, ary;
	long dincx, dincy;
	double dval0, dval1;

	Uint8 light;
	Sint32 z;
	bool zhit[MAPLAYERS], wallhit;

	posx=floor(camera->x);
	posy=floor(camera->y); // integer coordinates
	fracx=camera->x-posx;
	fracy=camera->y-posy; // fraction coordinates

	double wfov = (fov * camera->winw / camera->winh) * PI / 180.f;
	dstart = CLIPNEAR/16.0;

	// ray vector
	rx = cos(camera->ang - wfov/2.f);
	ry = sin(camera->ang - wfov/2.f);

	if( posx>=0 && posy>=0 && posx<map.width && posy<map.height ) {
		vismap[posy+posx*map.height]=TRUE;
	}
	for( sx=0; sx<camera->winw; sx++ ) { // for every column of the screen
		inx=posx;
		iny=posy;
		inx2=inx;
		iny2=iny;

		arx=0;
		if(rx) {
			arx = 1.0/fabs(rx);    // distance increments
		}
		ary=0;
		if(ry) {
			ary = 1.0/fabs(ry);
		}

		// dval0=dend+1 is there to prevent infinite loops when ray is parallel to axis
		dincx=0;
		dval0=1e32;
		dincy=0;
		dval1=1e32;

		// calculate integer coordinate increments
		// x-axis:
		if(rx<0) {
			dincx=-1;
			dval0=fracx*arx;
		} else if(rx>0) {
			dincx=1;
			dval0=(1-fracx)*arx;
		}

		// y-axis:
		if(ry<0) {
			dincy=-1;
			dval1=fracy*ary;
		} else if(ry>0) {
			dincy=1;
			dval1=(1-fracy)*ary;
		}

		d=0;
		dend=CLIPFAR/16;
		do {
			inx2=inx;
			iny2=iny;

			// move the ray one square forward
			if(dval1>dval0) {
				inx+=dincx;
				d=dval0;
				dval0+=arx;
			} else {
				iny+=dincy;
				d=dval1;
				dval1+=ary;
			}

			if( inx>=0 && iny>=0 && inx<map.width && iny<map.height ) {
				vismap[iny+inx*map.height]=TRUE;
				for( z=0; z<MAPLAYERS; z++ ) {
					zhit[z]=FALSE;
					if( map.tiles[z+iny*MAPLAYERS+inx*MAPLAYERS*map.height] && d>dstart ) { // hit something solid
						zhit[z]=TRUE;

						// collect light information
						if( inx2>=0 && iny2>=0 && inx2<map.width && iny2<map.height ) {
							if( map.tiles[z+iny2*MAPLAYERS+inx2*MAPLAYERS*map.height] ) {
								continue;
							}
							light = std::min(std::max(0,lightmap[iny2+inx2*map.height]),255);
						} else {
							light = 128;
						}

						// update minimap
						if( mode==REALCOLORS )
							if( d<16 && z==OBSTACLELAYER )
								if( light>0 ) {
									minimap[iny][inx]=2;    // wall space
								}
					} else if( z==OBSTACLELAYER && mode==REALCOLORS ) {
						// update minimap to show empty region
						if( inx>=0 && iny>=0 && inx<map.width && iny<map.height ) {
							light = std::min(std::max(0,lightmap[iny+inx*map.height]),255);
						} else {
							light = 128;
						}
						if( d<16 ) {
							if( light>0 && map.tiles[iny*MAPLAYERS+inx*MAPLAYERS*map.height] ) {
								minimap[iny][inx]=1;    // walkable space
							} else if( map.tiles[z+iny*MAPLAYERS+inx*MAPLAYERS*map.height] ) {
								minimap[iny][inx]=0;    // no floor
							}
						}
					}
				}
				wallhit=TRUE;
				for( z=0; z<MAPLAYERS; z++ )
					if( zhit[z]==FALSE ) {
						wallhit=FALSE;
					}
				if( wallhit==TRUE ) {
					break;
				}
			}
		} while(d<dend);

		// new ray vector for next column
		rx = cos(camera->ang - wfov/2.f + (wfov/camera->winw)*sx);
		ry = sin(camera->ang - wfov/2.f + (wfov/camera->winw)*sx);
	}
}

/*-------------------------------------------------------------------------------

	drawEntities3D

	Draws all entities in the level as either voxel models or sprites

-------------------------------------------------------------------------------*/

void drawEntities3D(view_t *camera, int mode) {
	node_t *node;
	Entity *entity;
	long x, y;

	if( map.entities->first == NULL ) {
		return;
	}

	for( node=map.entities->first; node!=NULL; node=node->next ) {
		entity = (Entity *)node->element;
		if( entity->flags[INVISIBLE] ) {
			continue;
		}
		if( entity->flags[UNCLICKABLE] && mode==ENTITYUIDS ) {
			continue;
		}
		if( entity->flags[GENIUS] ) {
			// genius entities are not drawn when the camera is inside their bounding box
			if( camera->x >= (entity->x-entity->sizex)/16 && camera->x <= (entity->x+entity->sizex)/16 )
				if( camera->y >= (entity->y-entity->sizey)/16 && camera->y <= (entity->y+entity->sizey)/16 ) {
					continue;
				}
		}
		x = entity->x/16;
		y = entity->y/16;
		if( x>=0 && y>=0 && x<map.width && y<map.height ) {
			if( vismap[y+x*map.height] || entity->flags[OVERDRAW] ) {
				if( entity->flags[SPRITE] == FALSE ) {
					glDrawVoxel(camera,entity,mode);
				} else {
					glDrawSprite(camera,entity,mode);
				}
			}
		} else {
			if( entity->flags[SPRITE] == FALSE ) {
				glDrawVoxel(camera,entity,mode);
			} else {
				glDrawSprite(camera,entity,mode);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	drawEntities2D

	Draws all entities in the level as sprites while accounting for the given
	camera coordinates

-------------------------------------------------------------------------------*/

void drawEntities2D(long camx, long camy) {
	node_t *node;
	Entity *entity;
	SDL_Rect pos, box;

	if( map.entities->first == NULL ) {
		return;
	}

	// draw entities
	for( node=map.entities->first; node!=NULL; node=node->next ) {
		entity = (Entity *)node->element;
		if( entity->flags[INVISIBLE] ) {
			continue;
		}
		pos.x = entity->x*(TEXTURESIZE/16)-camx;
		pos.y = entity->y*(TEXTURESIZE/16)-camy;
		pos.w = TEXTURESIZE;
		pos.h = TEXTURESIZE;
		if( entity->sprite >= 0 && entity->sprite<numsprites ) {
			if( sprites[entity->sprite] != NULL ) {
				if( entity == selectedEntity ) {
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box,SDL_MapRGB(mainsurface->format,255,0,0),255);
					box.w = TEXTURESIZE-2;
					box.h = TEXTURESIZE-2;
					box.x = pos.x+1;
					box.y = pos.y+1;
					drawRect(&box,SDL_MapRGB(mainsurface->format,0,0,255),255);
				}
				drawImageScaled(sprites[entity->sprite], NULL, &pos);
			} else {
				if( entity == selectedEntity ) {
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box,SDL_MapRGB(mainsurface->format,255,0,0),255);
					box.w = TEXTURESIZE-2;
					box.h = TEXTURESIZE-2;
					box.x = pos.x+1;
					box.y = pos.y+1;
					drawRect(&box,SDL_MapRGB(mainsurface->format,0,0,255),255);
				}
				drawImageScaled(sprites[0], NULL, &pos);
			}
		} else {
			if( entity == selectedEntity ) {
				// draws a box around the sprite
				box.w = TEXTURESIZE;
				box.h = TEXTURESIZE;
				box.x = pos.x;
				box.y = pos.y;
				drawRect(&box,SDL_MapRGB(mainsurface->format,255,0,0),255);
				box.w = TEXTURESIZE-2;
				box.h = TEXTURESIZE-2;
				box.x = pos.x+1;
				box.y = pos.y+1;
				drawRect(&box,SDL_MapRGB(mainsurface->format,0,0,255),255);
			}
			drawImageScaled(sprites[0], NULL, &pos);
		}
	}
}

/*-------------------------------------------------------------------------------

	drawGrid

	Draws a white line grid for the tile map

-------------------------------------------------------------------------------*/

void drawGrid(long camx, long camy) {
	long x, y;
	Uint32 color;

	color = SDL_MapRGB(mainsurface->format,127,127,127);
	drawLine(-camx,(map.height<<TEXTUREPOWER)-camy,(map.width<<TEXTUREPOWER)-camx,(map.height<<TEXTUREPOWER)-camy,color,255);
	drawLine((map.width<<TEXTUREPOWER)-camx,-camy,(map.width<<TEXTUREPOWER)-camx,(map.height<<TEXTUREPOWER)-camy,color,255);
	for( y=0; y<map.height; y++ ) {
		for( x=0; x<map.width; x++ ) {
			drawLine((x<<TEXTUREPOWER)-camx,(y<<TEXTUREPOWER)-camy,((x+1)<<TEXTUREPOWER)-camx,(y<<TEXTUREPOWER)-camy,color,255);
			drawLine((x<<TEXTUREPOWER)-camx,(y<<TEXTUREPOWER)-camy,(x<<TEXTUREPOWER)-camx,((y+1)<<TEXTUREPOWER)-camy,color,255);
		}
	}
}

/*-------------------------------------------------------------------------------

	drawEditormap

	Draws a minimap in the upper right corner of the screen to represent
	the screen's position relative to the rest of the level

-------------------------------------------------------------------------------*/

void drawEditormap(long camx, long camy) {
	SDL_Rect src, osrc;

	src.x = xres-120;
	src.y = 24;
	src.w = 112;
	src.h = 112;
	drawRect(&src,SDL_MapRGB(mainsurface->format,0,0,0),255);

	// initial box dimensions
	src.x = (xres-120) + (((double)camx/TEXTURESIZE)*112.0)/map.width;
	src.y = 24 + (((double)camy/TEXTURESIZE)*112.0)/map.height;
	src.w = (112.0/map.width)*((double)xres/TEXTURESIZE);
	src.h = (112.0/map.height)*((double)yres/TEXTURESIZE);

	// clip at left edge
	if( src.x < xres-120 ) {
		src.w -= (xres-120)-src.x;
		src.x = xres-120;
	}

	// clip at right edge
	if( src.x+src.w > xres-8 ) {
		src.w = xres-8-src.x;
	}

	// clip at top edge
	if( src.y < 24 ) {
		src.h -= 24-src.y;
		src.y = 24;
	}

	// clip at bottom edge
	if( src.y+src.h > 136 ) {
		src.h = 136-src.y;
	}

	osrc.x = src.x+1;
	osrc.y = src.y+1;
	osrc.w = src.w-2;
	osrc.h = src.h-2;
	drawRect(&src,SDL_MapRGB(mainsurface->format,255,255,255),255);
	drawRect(&osrc,SDL_MapRGB(mainsurface->format,0,0,0),255);
}

/*-------------------------------------------------------------------------------

	drawWindow / drawDepressed

	Draws a rectangular box that fills the area inside the given screen
	coordinates

-------------------------------------------------------------------------------*/

void drawWindow(int x1, int y1, int x2, int y2) {
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2-x1;
	src.h = y2-y1;
	drawRect(&src,SDL_MapRGB(mainsurface->format,160,160,192),255);
	src.x = x1+1;
	src.y = y1+1;
	src.w = x2-x1-1;
	src.h = y2-y1-1;
	drawRect(&src,SDL_MapRGB(mainsurface->format,96,96,128),255);
	src.x = x1+1;
	src.y = y1+1;
	src.w = x2-x1-2;
	src.h = y2-y1-2;
	drawRect(&src,SDL_MapRGB(mainsurface->format,128,128,160),255);
}

void drawDepressed(int x1, int y1, int x2, int y2) {
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2-x1;
	src.h = y2-y1;
	drawRect(&src,SDL_MapRGB(mainsurface->format,96,96,128),255);
	src.x = x1+1;
	src.y = y1+1;
	src.w = x2-x1-1;
	src.h = y2-y1-1;
	drawRect(&src,SDL_MapRGB(mainsurface->format,160,160,192),255);
	src.x = x1+1;
	src.y = y1+1;
	src.w = x2-x1-2;
	src.h = y2-y1-2;
	drawRect(&src,SDL_MapRGB(mainsurface->format,128,128,160),255);
}

void drawWindowFancy(int x1, int y1, int x2, int y2) {
	if (softwaremode) {
		// no fancy stuff in software mode
		drawWindow(x1, y1, x2, y2);
		return;
	}

	// update projection
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// draw quads
	glColor3f(.25, .25, .25);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	glVertex2f(x1, yres - y1);
	glVertex2f(x1, yres - y2);
	glVertex2f(x2, yres - y2);
	glVertex2f(x2, yres - y1);
	glEnd();
	glColor3f(.5, .5, .5);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	glVertex2f(x1 + 1, yres - y1 - 1);
	glVertex2f(x1 + 1, yres - y2 + 1);
	glVertex2f(x2 - 1, yres - y2 + 1);
	glVertex2f(x2 - 1, yres - y1 - 1);
	glEnd();
	glColor3f(.75, .75, .75);
	glBindTexture(GL_TEXTURE_2D, texid[fancyWindow_bmp->refcount]); // wood texture
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x1 + 2, yres - y1 - 2);
	glTexCoord2f(0, (y2 - y1 - 4)/(double)tiles[30]->h);
	glVertex2f(x1 + 2, yres - y2 + 2);
	glTexCoord2f((x2 - x1 - 4)/(double)tiles[30]->w, (y2-y1-4)/(double)tiles[30]->h);
	glVertex2f(x2 - 2, yres - y2 + 2);
	glTexCoord2f((x2 - x1 - 4)/(double)tiles[30]->w, 0);
	glVertex2f(x2 - 2, yres - y1 - 2);
	glEnd();
}

/*-------------------------------------------------------------------------------

	ttfPrintText / ttfPrintTextColor

	Prints an unformatted utf8 string to the screen, returning the width of
	the message surface in pixels. ttfPrintTextColor() also takes a 32-bit
	color argument

-------------------------------------------------------------------------------*/

SDL_Rect errorRect = { 0 };

SDL_Rect ttfPrintTextColor( TTF_Font *font, int x, int y, Uint32 color, bool outline, const char *str ) {
	SDL_Rect pos = { x, y, 0, 0 };
	SDL_Surface *surf;
	int c;

	if( !str ) {
		return errorRect;
	}

	char newStr[1024] = { 0 };
	strcpy(newStr,str);

	// tokenize string
	for( c=0; c<strlen(newStr)+1; c++ ) {
		if( newStr[c]=='\n' || newStr[c]=='\r' ) {
			int offY = 0;
			if( newStr[c]=='\n' ) {
				offY = TTF_FontHeight(font);
			}
			newStr[c]=0;
			ttfPrintTextColor(font,x,y+offY,color,outline,(char *)&newStr[c+1]);
			break;
		} else if( newStr[c]==0 ) {
			break;
		}
	}

	// retrieve text surface
	if( (surf=ttfTextHashRetrieve(ttfTextHash,newStr,font,outline))==NULL ) {
		// create the text outline surface
		if( outline ) {
			if( font==ttf8 ) {
				TTF_SetFontOutline(font, 1);
			} else {
				TTF_SetFontOutline(font, 2);
			}
			SDL_Color sdlColorBlack = { 0, 0, 0, 255 };
			surf = TTF_RenderUTF8_Blended(font, newStr, sdlColorBlack);
		} else {
			int w, h;
			TTF_SizeUTF8(font,str,&w,&h);
			if( font==ttf8 ) {
				surf = SDL_CreateRGBSurface(0,w+2,h+2,
				                            mainsurface->format->BitsPerPixel,
				                            mainsurface->format->Rmask,
				                            mainsurface->format->Gmask,
				                            mainsurface->format->Bmask,
				                            mainsurface->format->Amask
				                           );
			} else {
				surf = SDL_CreateRGBSurface(0,w+4,h+4,
				                            mainsurface->format->BitsPerPixel,
				                            mainsurface->format->Rmask,
				                            mainsurface->format->Gmask,
				                            mainsurface->format->Bmask,
				                            mainsurface->format->Amask
				                           );
			}
		}

		// create the text surface
		TTF_SetFontOutline(font, 0);
		SDL_Color sdlColorWhite = { 255, 255, 255, 255 };
		SDL_Surface *textSurf = TTF_RenderUTF8_Blended(font, newStr, sdlColorWhite);

		// combine the surfaces
		if( font==ttf8 ) {
			pos.x = 1;
			pos.y = 1;
		} else {
			pos.x = 2;
			pos.y = 2;
		}
		SDL_BlitSurface(textSurf,NULL,surf,&pos);

		// load the text outline surface as a GL texture
		allsurfaces[imgref] = surf;
		allsurfaces[imgref]->refcount = imgref;
		glLoadTexture(allsurfaces[imgref],imgref);
		imgref++;

		// store the surface in the text surface cache
		if( !ttfTextHashStore(ttfTextHash,newStr,font,outline,surf) ) {
			printlog("warning: failed to store text outline surface with imgref %d\n",imgref-1);
		}
	}

	// draw the text surface
	if( font==ttf8 ) {
		pos.x = x;
		pos.y = y-3;
	} else {
		pos.x = x+1;
		pos.y = y-4;
	}
	pos.w = surf->w;
	pos.h = surf->h;
	drawImageColor(surf,NULL,&pos,color);
	pos.x = x;
	pos.y = y;

	return pos;
}

SDL_Rect ttfPrintText( TTF_Font *font, int x, int y, const char *str ) {
	if( !str ) {
		return errorRect;
	}
	return ttfPrintTextColor(font,x,y,0xFFFFFFFF,TRUE,str);
}

/*-------------------------------------------------------------------------------

	ttfPrintTextFormatted / ttfPrintTextFormattedColor

	Prints a formatted utf8 string to the screen using
	ttfPrintText / ttfPrintTextColor

-------------------------------------------------------------------------------*/

SDL_Rect ttfPrintTextFormattedColor( TTF_Font *font, int x, int y, Uint32 color, char *fmt, ... ) {
	char str[1024] = { 0 };

	if( !fmt ) {
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font,x,y,color,TRUE,str);
}

SDL_Rect ttfPrintTextFormatted( TTF_Font *font, int x, int y, char *fmt, ... ) {
	char str[1024] = { 0 };

	if( !fmt ) {
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font,x,y,0xFFFFFFFF,TRUE,str);
}

/*-------------------------------------------------------------------------------

	printText

	Prints unformatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void printText( SDL_Surface *font_bmp, int x, int y, char *str ) {
	int c;
	int numbytes;
	SDL_Rect src, dest, odest;

	if( strlen(str) > 2048 ) {
		printlog("error: buffer overflow in printText\n");
		return;
	}

	// format the string
	numbytes = strlen(str);

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w/16;
	src.w = font_bmp->w/16;
	dest.h = font_bmp->h/16;
	src.h = font_bmp->h/16;

	// print the characters in the string
	for( c=0; c<numbytes; c++ ) {
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = (int)((str[c]*src.w)/font_bmp->w)*src.h;
		if( str[c] != 10 && str[c] != 13 ) { // LF/CR
			odest.x=dest.x;
			odest.y=dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x=odest.x+src.w;
			dest.y=odest.y;
		} else if( str[c]==10 ) {
			dest.x=x;
			dest.y+=src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormatted

	Prints formatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void printTextFormatted( SDL_Surface *font_bmp, int x, int y, char *fmt, ... ) {
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest, odest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w/16;
	src.w = font_bmp->w/16;
	dest.h = font_bmp->h/16;
	src.h = font_bmp->h/16;

	// print the characters in the string
	for( c=0; c<numbytes; c++ ) {
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = (int)((str[c]*src.w)/font_bmp->w)*src.h;
		if( str[c] != 10 && str[c] != 13 ) { // LF/CR
			odest.x=dest.x;
			odest.y=dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x=odest.x+src.w;
			dest.y=odest.y;
		} else if( str[c]==10 ) {
			dest.x=x;
			dest.y+=src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedAlpha

	Prints formatted text to the screen using a font bitmap and taking an
	alpha value.

-------------------------------------------------------------------------------*/

void printTextFormattedAlpha(SDL_Surface *font_bmp, int x, int y, Uint8 alpha, char *fmt, ...) {
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest, odest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w/16;
	src.w = font_bmp->w/16;
	dest.h = font_bmp->h/16;
	src.h = font_bmp->h/16;

	// print the characters in the string
	for( c=0; c<numbytes; c++ ) {
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = (int)((str[c]*src.w)/font_bmp->w)*src.h;
		if( str[c] != 10 && str[c] != 13 ) { // LF/CR
			odest.x=dest.x;
			odest.y=dest.y;
			drawImageAlpha( font_bmp, &src, &dest, alpha );
			dest.x=odest.x+src.w;
			dest.y=odest.y;
		} else if( str[c]==10 ) {
			dest.x=x;
			dest.y+=src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap and taking a
	32-bit color value

-------------------------------------------------------------------------------*/

void printTextFormattedColor(SDL_Surface *font_bmp, int x, int y, Uint32 color, char *fmt, ...) {
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest, odest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w/16;
	src.w = font_bmp->w/16;
	dest.h = font_bmp->h/16;
	src.h = font_bmp->h/16;

	// print the characters in the string
	for( c=0; c<numbytes; c++ ) {
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = (int)((str[c]*src.w)/font_bmp->w)*src.h;
		if( str[c] != 10 && str[c] != 13 ) { // LF/CR
			odest.x=dest.x;
			odest.y=dest.y;
			drawImageColor( font_bmp, &src, &dest, color );
			dest.x=odest.x+src.w;
			dest.y=odest.y;
		} else if( str[c]==10 ) {
			dest.x=x;
			dest.y+=src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap, while coloring,
	rotating, and scaling it

-------------------------------------------------------------------------------*/

void printTextFormattedFancy(SDL_Surface *font_bmp, int x, int y, Uint32 color, double angle, double scale, char *fmt, ...) {
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	double newX = x;
	double newY = y;
	dest.w = ((double)font_bmp->w/16.f)*scale;
	src.w = font_bmp->w/16;
	dest.h = ((double)font_bmp->h/16.f)*scale;
	src.h = font_bmp->h/16;

	// print the characters in the string
	int line=0;
	for( c=0; c<numbytes; c++ ) {
		src.x = (str[c]*src.w)%font_bmp->w;
		src.y = (int)((str[c]*src.w)/font_bmp->w)*src.h;
		if( str[c] != 10 && str[c] != 13 ) { // LF/CR
			dest.x = newX;
			dest.y = newY;
			drawImageFancy( font_bmp, color, angle, &src, &dest );
			newX+=(double)dest.w*cos(angle);
			newY+=(double)dest.h*sin(angle);
		} else if( str[c]==10 ) {
			line++;
			dest.x=x+dest.h*cos(angle+PI/2)*line;
			dest.y=y+dest.h*sin(angle+PI/2)*line;
		}
	}
}

/*-------------------------------------------------------------------------------

	draws a tooltip

	Draws a tooltip box

-------------------------------------------------------------------------------*/

void drawTooltip(SDL_Rect *src) {
	drawRect(src,0,250);
	drawLine(src->x,src->y,src->x+src->w,src->y,SDL_MapRGB(mainsurface->format,0,192,255),255);
	drawLine(src->x,src->y+src->h,src->x+src->w,src->y+src->h,SDL_MapRGB(mainsurface->format,0,192,255),255);
	drawLine(src->x,src->y,src->x,src->y+src->h,SDL_MapRGB(mainsurface->format,0,192,255),255);
	drawLine(src->x+src->w,src->y,src->x+src->w,src->y+src->h,SDL_MapRGB(mainsurface->format,0,192,255),255);
}
