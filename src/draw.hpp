/*-------------------------------------------------------------------------------

	BARONY
	File: draw.hpp
	Desc: prototypes for draw.cpp, various drawing functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once
#include <SDL.h>
#include "entity.hpp"

#define FLIP_VERTICAL 1
#define FLIP_HORIZONTAL 2
SDL_Surface* flipSurface(SDL_Surface* surface, int flags);
void drawCircle(int x, int y, real_t radius, Uint32 color, Uint8 alpha);
void drawArc(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawArcInvertedY(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawLine(int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha);
int drawRect(SDL_Rect* src, Uint32 color, Uint8 alpha);
int drawBox(SDL_Rect* src, Uint32 color, Uint8 alpha);
void drawGear(Sint16 x, Sint16 y, real_t size, Sint32 rotation);
void drawImage(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageRing(SDL_Surface* image, SDL_Rect* src, int radius, int thickness, int segments, real_t angStart, real_t angEnd, Uint8 alpha);
void drawImageScaled(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageScaledPartial(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, float percentY);
void drawImageAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint8 alpha);
void drawImageColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
void drawImageFancy(SDL_Surface* image, Uint32 color, real_t angle, SDL_Rect* src, SDL_Rect* pos);
void drawImageRotatedAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, real_t angle, Uint8 alpha);
void drawImageScaledColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
SDL_Surface* scaleSurface(SDL_Surface* Surface, Uint16 Width, Uint16 Height);
void drawSky3D(view_t* camera, SDL_Surface* tex);
void drawLayer(long camx, long camy, int z, map_t* map);
void drawBackground(long camx, long camy);
void drawForeground(long camx, long camy);
void drawClearBuffers();
void raycast(view_t* camera, Sint8 (*minimap)[MINIMAP_MAX_DIMENSION]);
void drawFloors(view_t* camera);
void drawSky(SDL_Surface* srfc);
void drawVoxel(view_t* camera, Entity* entity);
void drawEntities3D(view_t* camera, int mode);
void drawPalette(voxel_t* model);
void drawEntities2D(long camx, long camy);
void drawGrid(long camx, long camy);
void drawEditormap(long camx, long camy);
void drawWindow(int x1, int y1, int x2, int y2);
void drawDepressed(int x1, int y1, int x2, int y2);
void drawWindowFancy(int x1, int y1, int x2, int y2);
SDL_Rect ttfPrintTextColor( TTF_Font* font, int x, int y, Uint32 color, bool outline, const char* str );
SDL_Rect ttfPrintText( TTF_Font* font, int x, int y, const char* str );
SDL_Rect ttfPrintTextFormattedColor( TTF_Font* font, int x, int y, Uint32 color, char const * const fmt, ... );
SDL_Rect ttfPrintTextFormatted( TTF_Font* font, int x, int y, char const * const fmt, ... );
void printTextFormatted( SDL_Surface* font_bmp, int x, int y, char const * const fmt, ... );
void printTextFormattedAlpha(SDL_Surface* font_bmp, int x, int y, Uint8 alpha, char const * const fmt, ...);
void printTextFormattedColor(SDL_Surface* font_bmp, int x, int y, Uint32 color, char const * const fmt, ...);
void printTextFormattedFancy(SDL_Surface* font_bmp, int x, int y, Uint32 color, real_t angle, real_t scale, char* fmt, ...);
void printText( SDL_Surface* font_bmp, int x, int y, const char* str );
void drawSprite(view_t* camera, Entity* entity);
void drawTooltip(SDL_Rect* src, Uint32 optionalColor = 0);
Uint32 getPixel(SDL_Surface* surface, int x, int y);
void putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);
void getColor(Uint32 color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
bool behindCamera(const view_t& camera, real_t x, real_t y);
void occlusionCulling(map_t& map, const view_t& camera);

constexpr Uint32 makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((Uint32)a << 24) | ((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r << 0);
}

constexpr Uint32 makeColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return 0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r << 0);
}

class TempTexture {
private:
	GLuint _texid = 0;
public:
    const GLuint& texid = _texid;

	TempTexture() {
	}

	~TempTexture() {
		if( _texid ) {
			glDeleteTextures(1,&_texid);
			_texid = 0;
		}
	}

	void load(SDL_Surface* surf, bool clamp, bool point) {
		SDL_LockSurface(surf);
		glGenTextures(1,&_texid);
		glBindTexture(GL_TEXTURE_2D, _texid);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		if (clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		if (point) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.f);
			//glGenerateMipmap(GL_TEXTURE_2D);
		}
		SDL_UnlockSurface(surf);
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, _texid);
	}
};

struct framebuffer {
    unsigned int fbo = 0;
    unsigned int fbo_color = 0;
    unsigned int fbo_depth = 0;
    unsigned int xsize = 1280;
    unsigned int ysize = 720;

    void init(unsigned int _xsize, unsigned int _ysize, GLint minFilter, GLint magFilter) {
        xsize = _xsize;
        ysize = _ysize;

	    SDL_glGenFramebuffers(1, &fbo);
	    SDL_glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	    glGenTextures(1, &fbo_color);
	    glBindTexture(GL_TEXTURE_2D, fbo_color);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, xsize, ysize, 0, GL_RGBA, GL_FLOAT, nullptr);
	    SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0);
	    glBindTexture(GL_TEXTURE_2D, 0);

	    glGenTextures(1, &fbo_depth);
	    glBindTexture(GL_TEXTURE_2D, fbo_depth);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, xsize, ysize, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);
	    SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);
	    glBindTexture(GL_TEXTURE_2D, 0);

	    static const GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
	    SDL_glDrawBuffers(sizeof(attachments) / sizeof(GLenum), attachments);
	    glReadBuffer(GL_NONE);

	    SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy() {
	    if (fbo) {
		    SDL_glDeleteFramebuffers(1, &fbo);
		    fbo = 0;
	    }
	    if (fbo_color) {
		    glDeleteTextures(1, &fbo_color);
		    fbo_color = 0;
	    }
	    if (fbo_depth) {
		    glDeleteTextures(1, &fbo_depth);
		    fbo_depth = 0;
	    }
    }

    void bindForWriting() {
	    SDL_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	    SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0);
	    SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);
	    glViewport(0, 0, xsize, ysize);
    }

    void bindForReading() {
	    glBindTexture(GL_TEXTURE_2D, fbo_color);
    }

    static void blit() {
	    glDisable(GL_DEPTH_TEST);
	    glDisable(GL_LIGHTING);
	    glColor4f(1.f, 1.f, 1.f, 1.f);
	    glMatrixMode(GL_PROJECTION);
	    glPushMatrix();
	    glLoadIdentity();
	    glMatrixMode(GL_MODELVIEW);
	    glPushMatrix();
	    glLoadIdentity();
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
	    glTexCoord2f(1.f, 0.f); glVertex2f( 1.f, -1.f);
	    glTexCoord2f(1.f, 1.f); glVertex2f( 1.f,  1.f);
	    glTexCoord2f(0.f, 1.f); glVertex2f(-1.f,  1.f);
	    glEnd();
	    glPopMatrix();
	    glPopMatrix();
    }

    static void unbind() {
	    SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    glViewport(0, 0, xres, yres);
    }
};

