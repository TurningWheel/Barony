/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains all drawing code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.o

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "files.hpp"
#include "hash.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "ui/Frame.hpp"
#ifndef NINTENDO
#include "editor.hpp"
#endif
#include "items.hpp"
#include "ui/Image.hpp"
#include "interface/consolecommand.hpp"
#include "colors.hpp"
#include "ui/Text.hpp"

#include <cassert>

#include "ui/Image.hpp"

framebuffer main_framebuffer;

Mesh framebuffer::mesh{
	{ // positions
		-1.f, -1.f,  0.f,
		 1.f, -1.f,  0.f,
		 1.f,  1.f,  0.f,
		-1.f,  1.f,  0.f,
	},
	{ // texcoords
		0.f,  0.f,
		1.f,  0.f,
		1.f,  1.f,
		0.f,  1.f,
	},
	{ // colors
		1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
	},
	{ // indices
		0, 1, 2,
		0, 2, 3,
	}
};

Shader framebuffer::shader;

void createCommonDrawResources() {
	static const char vertex_glsl[] =
		"#version 330\n"
		"layout(location=0) in vec3 iPosition;\n"
		"layout(location=1) in vec2 iTexCoord;\n"
		"out vec2 TexCoord;\n"
		"void main() {\n"
		"gl_Position = vec4(iPosition, 1.0);\n"
		"TexCoord = iTexCoord;\n"
		"}";

	static const char fragment_glsl[] =
		"#version 330\n"
		"in vec2 TexCoord;\n"
		"out vec4 Color;\n"
		"uniform sampler2D uTexture;\n"
		"uniform float uGamma;\n"
		"void main() {\n"
		"Color = texture(uTexture, TexCoord) * uGamma;\n"
		"}";

	framebuffer::mesh.init();
	framebuffer::shader.init();
	framebuffer::shader.compile(vertex_glsl, sizeof(vertex_glsl), Shader::Type::Vertex);
	framebuffer::shader.compile(fragment_glsl, sizeof(fragment_glsl), Shader::Type::Fragment);
	framebuffer::shader.link();
	glUniform1i(framebuffer::shader.uniform("uTexture"), 0);
}

void destroyCommonDrawResources() {
	framebuffer::mesh.destroy();
	framebuffer::shader.destroy();
}

void Mesh::init() {
	if (vao) {
		return;
	}
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// data buffers
	glGenBuffers((GLsizei)BufferType::Max, vbo);
	for (unsigned int c = 0; c < (unsigned int)BufferType::Index; ++c) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[c]);
		glBufferData(GL_ARRAY_BUFFER, data[c].size() * sizeof(float), data[c].data(), GL_STATIC_DRAW);
		glVertexAttribPointer(c, ElementsPerVBO[c], GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(c);
	}

	// index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[(int)BufferType::Index]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(unsigned int), index.data(), GL_STATIC_DRAW);
	glBindVertexArray(0);

	printlog("initialized mesh with %llu vertices", index.size());
}

void Mesh::destroy() {
	for (int c = 0; c < (int)BufferType::Max; ++c) {
		if (vbo[c]) {
			glDeleteBuffers(1, &vbo[c]);
			vbo[c] = 0;
		}
	}
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

void Mesh::draw() const {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, index.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void framebuffer::init(unsigned int _xsize, unsigned int _ysize, GLint minFilter, GLint magFilter) {
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

void framebuffer::destroy() {
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

void framebuffer::bindForWriting() {
	SDL_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0);
	SDL_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);
	glViewport(0, 0, xsize, ysize);
}

void framebuffer::bindForReading() const {
	glBindTexture(GL_TEXTURE_2D, fbo_color);
}

void framebuffer::blit(float gamma) {
	shader.bind();
	glUniform1f(shader.uniform("uGamma"), gamma);
	mesh.draw();
	shader.unbind();
}

void framebuffer::unbindForWriting() {
	SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer::unbindForReading() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void framebuffer::unbindAll() {
	SDL_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(0, 0, xres, yres);
}

/*-------------------------------------------------------------------------------

	getPixel

	gets the value of a pixel at the given x,y location in the given
	SDL_Surface

-------------------------------------------------------------------------------*/

Uint32 getPixel(SDL_Surface* surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to retrieve
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			return *p;
			break;

		case 2:
			return *(Uint16*)p;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				return p[0] << 16 | p[1] << 8 | p[2];
			}
			else
			{
				return p[0] | p[1] << 8 | p[2] << 16;
			}
			break;

		case 4:
			return *(Uint32*)p;
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

void putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to set
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16*)p = pixel;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}
			else
			{
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32*)p = pixel;
			break;
	}
}

/*-------------------------------------------------------------------------------

	flipSurface

	flips the contents of an SDL_Surface horizontally, vertically, or both

-------------------------------------------------------------------------------*/

SDL_Surface* flipSurface( SDL_Surface* surface, int flags )
{
	SDL_Surface* flipped = NULL;
	Uint32 pixel;
	int x, rx;
	int y, ry;

	// prepare surface for flipping
	flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );
	if ( SDL_MUSTLOCK( surface ) )
	{
		SDL_LockSurface( surface );
	}
	if ( SDL_MUSTLOCK( flipped ) )
	{
		SDL_LockSurface( flipped );
	}

	for ( x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx-- )
	{
		for ( y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry-- )
		{
			pixel = getPixel( surface, x, y );

			// copy pixel
			if ( ( flags & FLIP_VERTICAL ) && ( flags & FLIP_HORIZONTAL ) )
			{
				putPixel( flipped, rx, ry, pixel );
			}
			else if ( flags & FLIP_HORIZONTAL )
			{
				putPixel( flipped, rx, y, pixel );
			}
			else if ( flags & FLIP_VERTICAL )
			{
				putPixel( flipped, x, ry, pixel );
			}
		}
	}

	// restore image
	if ( SDL_MUSTLOCK( surface ) )
	{
		SDL_UnlockSurface( surface );
	}
	if ( SDL_MUSTLOCK( flipped ) )
	{
		SDL_UnlockSurface( flipped );
	}

	return flipped;
}

/*-------------------------------------------------------------------------------

drawCircle

draws a circle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawCircle( int x, int y, real_t radius, Uint32 color, Uint8 alpha )
{
	drawArc(x, y, radius, 0, 360, color, alpha);
}

/*-------------------------------------------------------------------------------

	drawArc

	draws an arc in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawArc( int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha )
{
	// update projection
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);

	// set color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, alpha / 255.f);
	glBindTexture(GL_TEXTURE_2D, 0);

	// draw arc
	GLint lineWidth;
	glGetIntegerv(GL_LINE_WIDTH, &lineWidth);
	glLineWidth(2);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
	for (real_t c = angle1; c <= angle2; c += (real_t)1)
	{
		real_t degInRad = c * (real_t)PI / (real_t)180;
		glVertex2f(x + ceil(cos(degInRad)*radius) + 1, yres - (y + ceil(sin(degInRad)*radius)));
	}
	glEnd();
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(lineWidth);
}

/*-------------------------------------------------------------------------------

drawScalingFilledArc

draws an arc with a changing radius

-------------------------------------------------------------------------------*/

static void drawScalingFilledArc( int x, int y, real_t radius1, real_t radius2, real_t angle1, real_t angle2, Uint32 outer_color, Uint32 inner_color )
{
	// set state
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

	Uint8 r, g, b, a;

	// draw arc
	glBegin(GL_TRIANGLE_FAN);
	getColor(inner_color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	glVertex2f(x, yres - y);
	getColor(outer_color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	for (real_t c = angle2; c >= angle1; c -= (real_t)1)
	{
		real_t degInRad = c * (real_t)PI / (real_t)180;
		real_t factor = (c - angle1) / (angle2 - angle1);
		real_t radius = radius2 * factor + radius1 * (1 - factor);
		glVertex2f(x + cos(degInRad) * radius, yres - (y + sin(degInRad) * radius));
	}
	glEnd();
}

/*-------------------------------------------------------------------------------

drawArcInvertedY, reversing the angle of direction in the y coordinate.

draws an arc in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawArcInvertedY(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha)
{
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
	glGetIntegerv(GL_LINE_WIDTH, &lineWidth);
	glLineWidth(2);

	// draw line
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, alpha / 255.f);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINE_STRIP);
	for ( c = angle1; c <= angle2; c++ )
	{
		float degInRad = c * PI / 180.f;
		glVertex2f(x + ceil(cos(degInRad)*radius) + 1, yres - (y - ceil(sin(degInRad)*radius)));
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

void drawLine( int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha )
{
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
	glGetIntegerv(GL_LINE_WIDTH, &lineWidth);
	glLineWidth(2);

	// draw line
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, alpha / 255.f);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINES);
	glVertex2f(x1 + 1, yres - y1);
	glVertex2f(x2 + 1, yres - y2);
	glEnd();
	glDisable(GL_LINE_SMOOTH);

	// reset line width
	glLineWidth(lineWidth);
}

/*-------------------------------------------------------------------------------

	drawRect

	draws a rectangle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

int drawRect( SDL_Rect* src, Uint32 color, Uint8 alpha )
{
	SDL_Rect secondsrc;
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = xres;
		secondsrc.h = yres;
		src = &secondsrc;
	}
	Uint32 c = (color & 0x00ffffff) | ((Uint32)alpha << 24);
	auto image = Image::get("images/system/white.png");
	image->drawColor(nullptr, *src, SDL_Rect{0, 0, xres, yres}, c);
	return 0;
}


/*-------------------------------------------------------------------------------

	drawBox

	draws the border of a rectangle

-------------------------------------------------------------------------------*/
int drawBox(SDL_Rect* src, Uint32 color, Uint8 alpha)
{
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

void drawGear(Sint16 x, Sint16 y, real_t size, Sint32 rotation)
{
	const Uint32 black = makeColor(0, 0, 0, 255);
	const Uint32 color_dark = makeColor(255, 32, 0, 255);
	const Uint32 color = makeColor(255, 76, 49, 255);
	const Uint32 color_bright = makeColor(255, 109, 83, 255);
	const real_t teeth_size = size + size / 3;
	const int num_teeth = 6;
	for ( int c = 0; c < num_teeth; c++ )
	{
		real_t p = 180.0 / (real_t)num_teeth;
		real_t r = (real_t)c * (p * 2.0) + (real_t)rotation;
		real_t t = 4.0;
		drawScalingFilledArc(x, y, size, size,
			r,
			r + p,
			color, color_bright);
		drawScalingFilledArc(x, y, size, teeth_size,
			r + p,
			r + p + t,
			color, color_bright);
		drawScalingFilledArc(x, y, teeth_size, teeth_size,
			r + p + t,
			r + p * 2 - t,
			color, color_bright);
		drawScalingFilledArc(x, y, teeth_size, size,
			r + p * 2 - t,
			r + p * 2,
			color, color_bright);
	}
	drawScalingFilledArc(x, y,
		size * 1 / 3,
		size * 1 / 3,
		0, 360,
		color, color_dark);
	drawScalingFilledArc(x, y,
		size * 1 / 6,
		size * 1 / 6,
		0, 360,
		black, black);
}

/*-------------------------------------------------------------------------------

	drawImageRotatedAlpha

	blits an image in either an opengl or SDL context, rotating the image
	relative to the screen and taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageRotatedAlpha( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, real_t angle, Uint8 alpha )
{
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
	glTranslatef(pos->x, yres - pos->y, 0);
	glRotatef(-angle * 180 / PI, 0.f, 0.f, 1.f);

	// for the use of a whole image
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, alpha / 255.1);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(-src->w / 2, src->h / 2);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(-src->w / 2, -src->h / 2);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(src->w / 2, -src->h / 2);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(src->w / 2, src->h / 2);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageColor

	blits an image in either an opengl or SDL context while colorizing it

-------------------------------------------------------------------------------*/

void drawImageColor( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color )
{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a/ 255.f);
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x, yres - pos->y);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageAlpha

	blits an image in either an opengl or SDL context, taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageAlpha( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint8 alpha )
{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, alpha / 255.1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x, yres - pos->y);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImage

	blits an image in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawImage( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos )
{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, 1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x, yres - pos->y);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x + src->w, yres - pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

drawImageRing

blits an image in either an opengl or SDL context into a 2d ring.

-------------------------------------------------------------------------------*/

void drawImageRing(SDL_Surface* image, SDL_Rect* src, int radius, int thickness, int segments, real_t angStart, real_t angEnd, Uint8 alpha)
{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, alpha / 255.f);
	glPushMatrix();

	double s;
	real_t arcAngle = angStart;
	int first = segments / 2;
	real_t distance = std::round((angEnd - angStart) * segments / (2 * PI));
	for ( int i = 0; i < first; ++i ) 
	{
		glBegin(GL_QUAD_STRIP);
		for ( int j = 0; j <= static_cast<int>(distance); ++j )
		{
			s = i % first + 0.01;
			arcAngle = ((j % segments) * 2 * PI / segments) + angStart; // angle of the line.

			real_t arcx1 = (radius + thickness * cos(s * 2 * PI / first)) * cos(arcAngle);
			real_t arcy1 = (radius + thickness * cos(s * 2 * PI / first)) * sin(arcAngle);

			s = (i + 1) % first + 0.01;
			real_t arcx2 = (radius + thickness * cos(s * 2 * PI / first)) * cos(arcAngle);
			real_t arcy2 = (radius + thickness * cos(s * 2 * PI / first)) * sin(arcAngle);
			//glTexCoord2f(1.f, 0.f);
			glVertex2f(src->x + arcx1, yres - src->y + arcy1);
			//glTexCoord2f(0.f, 1.f);
			glVertex2f(src->x + arcx2, yres - src->y + arcy2);
			//s = i % first + 0.01;
			//arcAngle = (((j + 1) % segments) * 2 * PI / segments) + angStart; // angle of the line.
			//real_t arcx3 = (radius + thickness * cos(s * 2 * PI / first)) * cos(arcAngle);
			//real_t arcy3 = (radius + thickness * cos(s * 2 * PI / first)) * sin(arcAngle);

			//s = (i + 1) % first + 0.01;
			//real_t arcx4 = (radius + thickness * cos(s * 2 * PI / first)) * cos(arcAngle);
			//real_t arcy4 = (radius + thickness * cos(s * 2 * PI / first)) * sin(arcAngle);

			//std::vector<std::pair<real_t, real_t>> xycoords;
			//xycoords.push_back(std::make_pair(arcx1, arcy1));
			//xycoords.push_back(std::make_pair(arcx2, arcy2));
			//xycoords.push_back(std::make_pair(arcx3, arcy3));
			//xycoords.push_back(std::make_pair(arcx4, arcy4));
			//std::sort(xycoords.begin(), xycoords.end());
			//if ( xycoords.at(2).second < xycoords.at(3).second )
			//{
			//	glTexCoord2f(1.f, 0.f);
			//	glVertex2f(xres / 2 + xycoords.at(2).first, yres / 2 + xycoords.at(2).second); // lower right.
			//	glTexCoord2f(1.f, 1.f);
			//	glVertex2f(xres / 2 + xycoords.at(3).first, yres / 2 + xycoords.at(3).second); // upper right.
			//}
			//else
			//{
			//	glTexCoord2f(1.f, 0.f);
			//	glVertex2f(xres / 2 + xycoords.at(3).first, yres / 2 + xycoords.at(3).second); // lower right.
			//	glTexCoord2f(1.f, 1.f);
			//	glVertex2f(xres / 2 + xycoords.at(2).first, yres / 2 + xycoords.at(2).second); // upper right.
			//}
			//if ( xycoords.at(0).second < xycoords.at(1).second )
			//{
			//	glTexCoord2f(0.f, 0.f);
			//	glVertex2f(xres / 2 + xycoords.at(0).first, yres / 2 + xycoords.at(0).second); // lower left.
			//	glTexCoord2f(0.f, 1.f);
			//	glVertex2f(xres / 2 + xycoords.at(1).first, yres / 2 + xycoords.at(1).second); // upper left.
			//}
			//else
			//{
			//	glTexCoord2f(0.f, 0.f);
			//	glVertex2f(xres / 2 + xycoords.at(1).first, yres / 2 + xycoords.at(1).second); // lower left.
			//	glTexCoord2f(0.f, 1.f);
			//	glVertex2f(xres / 2 + xycoords.at(0).first, yres / 2 + xycoords.at(0).second); // upper left.
			//}
			

			//glVertex2f(xres / 2 + arcx3, yres / 2 + arcy3);
			//glVertex2f(xres / 2 + arcx4, yres / 2 + arcy4);
		}
		glEnd();
	}
	glPopMatrix();
	// debug lines
	/*real_t x1 = xres / 2 + 300 * cos(angStart);
	real_t y1 = yres / 2 - 300 * sin(angStart);
	real_t x2 = xres / 2 + 300 * cos(angEnd);
	real_t y2 = yres / 2 - 300 * sin(angEnd);
	drawLine(xres / 2, yres / 2, x1, y1, 0xFFFFFFFF, 255);
	drawLine(xres / 2, yres / 2, x2, y2, 0xFFFFFFFF, 255);*/
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	drawImageScaled

	blits an image in either an opengl or SDL context, scaling it

-------------------------------------------------------------------------------*/

void drawImageScaled( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos )
{
	SDL_Rect secondsrc;

	if ( !image )
	{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, 1);
	glPushMatrix();
	glBegin(GL_QUADS);

	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * ((real_t)src->y / image->h));
	glVertex2f(pos->x, yres - pos->y);
	glTexCoord2f(1.0 * ((real_t)src->x / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x, yres - pos->y - pos->h);
	//glVertex2f(pos->x, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * (((real_t)src->y + src->h) / image->h));
	glVertex2f(pos->x + pos->w, yres - pos->y - pos->h);
	//glVertex2f(pos->x + src->w, yres - pos->y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / image->w), 1.0 * ((real_t)src->y / image->h));
	//glVertex2f(pos->x + src->w, yres - pos->y);
	glVertex2f(pos->x + pos->w, yres - pos->y);

	//glTexCoord2f(0.f, 0.f);
	//glVertex2f(pos->x, yres - pos->y);
	//glTexCoord2f(0.f, 1.f);
	//glVertex2f(pos->x, yres - pos->y - pos->h);
	//glTexCoord2f(1.f, 1.f);
	//glVertex2f(pos->x + pos->w, yres - pos->y - pos->h);
	//glTexCoord2f(1.f, 0.f);
	//glVertex2f(pos->x + pos->w, yres - pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

drawImageScaledPartial

blits an image in either an opengl or SDL context, scaling it

-------------------------------------------------------------------------------*/

void drawImageScaledPartial(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, float percentY)
{
	SDL_Rect secondsrc;

	if ( !image )
	{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	glColor4f(1, 1, 1, 1);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 1.f - 1.f * percentY); // top left. 
	glVertex2f(pos->x, yres - pos->y - pos->h + pos->h * percentY);

	glTexCoord2f(0.f, 1.f); // bottom left
	glVertex2f(pos->x, yres - pos->y - pos->h);

	glTexCoord2f(1.f, 1.f); // bottom right
	glVertex2f(pos->x + pos->w, yres - pos->y - pos->h);

	glTexCoord2f(1.f, 1.f - 1.f * percentY); // top right
	glVertex2f(pos->x + pos->w, yres - pos->y - pos->h + pos->h * percentY);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);

	// debug corners
	//Uint32 color = uint32ColorPlayer1; // green
	//drawCircle(pos->x, pos->y + (pos->h - (pos->h * percentY)), 5, color, 255);
	//color = uint32ColorPlayer2; // pink
	//drawCircle(pos->x, pos->y + pos->h, 5, color, 255);
	//color = uint32ColorPlayer3; // sky blue
	//drawCircle(pos->x + pos->w, pos->y + pos->h, 5, color, 255);
	//color = uint32ColorPlayer4; // yellow
	//drawCircle(pos->x + pos->w, pos->y + (pos->h - (pos->h * percentY)), 5, color, 255);
}

/*-------------------------------------------------------------------------------

drawImageScaledColor

blits an image in either an opengl or SDL context while colorizing and scaling it

-------------------------------------------------------------------------------*/

void drawImageScaledColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color)
{
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
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, 0.f);
	glVertex2f(pos->x, yres - pos->y);
	glTexCoord2f(0.f, 1.f);
	glVertex2f(pos->x, yres - pos->y - pos->h);
	glTexCoord2f(1.f, 1.f);
	glVertex2f(pos->x + pos->w, yres - pos->y - pos->h);
	glTexCoord2f(1.f, 0.f);
	glVertex2f(pos->x + pos->w, yres - pos->y);
	glEnd();
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

/*-------------------------------------------------------------------------------

	scaleSurface

	Scales an SDL_Surface to the given width and height.

-------------------------------------------------------------------------------*/

SDL_Surface* scaleSurface(SDL_Surface* Surface, Uint16 Width, Uint16 Height)
{
	Sint32 x, y, o_x, o_y;

	if (!Surface || !Width || !Height)
	{
		return NULL;
	}

	SDL_Surface* _ret = SDL_CreateRGBSurface(Surface->flags, Width, Height, Surface->format->BitsPerPixel, Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);

	real_t _stretch_factor_x = (real_t)Width / (real_t)Surface->w;
	real_t _stretch_factor_y = (real_t)Height / (real_t)Surface->h;

	for (y = 0; y < Surface->h; y++)
		for (x = 0; x < Surface->w; x++)
			for (o_y = 0; o_y < _stretch_factor_y; ++o_y)
				for (o_x = 0; o_x < _stretch_factor_x; ++o_x)
				{
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

void drawImageFancy( SDL_Surface* image, Uint32 color, real_t angle, SDL_Rect* src, SDL_Rect* pos )
{
	SDL_Rect secondsrc;

	if ( !image )
	{
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
	glTranslatef(pos->x, yres - pos->y, 0);
	glRotatef(-angle * 180 / PI, 0.f, 0.f, 1.f);

	// for the use of a whole image
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = image->w;
		secondsrc.h = image->h;
		src = &secondsrc;
	}

	// draw a textured quad
	glBindTexture(GL_TEXTURE_2D, texid[(long int)image->userdata]);
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(((real_t)src->x) / ((real_t)image->w), ((real_t)src->y) / ((real_t)image->h));
	glVertex2f(0, 0);
	glTexCoord2f(((real_t)src->x) / ((real_t)image->w), ((real_t)(src->y + src->h)) / ((real_t)image->h));
	glVertex2f(0, -pos->h);
	glTexCoord2f(((real_t)(src->x + src->w)) / ((real_t)image->w), ((real_t)(src->y + src->h)) / ((real_t)image->h));
	glVertex2f(pos->w, -pos->h);
	glTexCoord2f(((real_t)(src->x + src->w)) / ((real_t)image->w), ((real_t)src->y) / ((real_t)image->h));
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

void drawSky3D( view_t* camera, SDL_Surface* tex )
{
	real_t screenfactor;
	int skyx, skyy;
	SDL_Rect dest;
	SDL_Rect src;

	// move the images differently depending upon the screen size
	screenfactor = xres / 320.0;

	// bitmap offsets
	skyx = -camera->ang * ((320 * screenfactor) / (PI / 2.0));
	skyy = (-114 * screenfactor - camera->vang);

	src.x = -skyx;
	src.y = -skyy;
	src.w = (-skyx) + xres; // clip to the screen width
	src.h = (-skyy) + yres; // clip to the screen height
	dest.x = 0;
	dest.y = 0;
	dest.w = xres;
	dest.h = yres;

	drawImage(tex, &src, &dest);

	// draw the part of the last part of the sky (only appears when angle > 270 deg.)
	if ( skyx < -960 * screenfactor )
	{
		dest.x = 1280 * screenfactor + skyx;
		dest.y = 0;
		dest.w = xres;
		dest.h = yres;
		src.x = 0;
		src.y = -skyy;
		src.w = xres - (-skyx - 1280 * screenfactor);
		src.h = src.y + yres;
		drawImage(tex, &src, &dest);
	}
}

/*-------------------------------------------------------------------------------

	drawLayer / drawBackground / drawForeground

	Draws the world tiles that are viewable at the given camera coordinates

-------------------------------------------------------------------------------*/

void drawLayer(long camx, long camy, int z, map_t* map)
{
	long x, y;
	long minx, miny, maxx, maxy;
	int index;
	SDL_Rect pos;

	minx = std::max<long int>(camx >> TEXTUREPOWER, 0);
	maxx = std::min<long int>((camx >> TEXTUREPOWER) + xres / TEXTURESIZE + 2, map->width); //TODO: Why are long int and unsigned int being compared?
	miny = std::max<long int>(camy >> TEXTUREPOWER, 0);
	maxy = std::min<long int>((camy >> TEXTUREPOWER) + yres / TEXTURESIZE + 2, map->height); //TODO: Why are long int and unsigned int being compared?
	for ( y = miny; y < maxy; y++ )
	{
		for ( x = minx; x < maxx; x++ )
		{
			index = map->tiles[z + y * MAPLAYERS + x * MAPLAYERS * map->height];
			if ( index > 0)
			{
				pos.x = (x << TEXTUREPOWER) - camx;
				pos.y = (y << TEXTUREPOWER) - camy;
				pos.w = TEXTURESIZE;
				pos.h = TEXTURESIZE;
				if ( index >= 0 && index < numtiles )
				{
					if ( tiles[index] != NULL )
					{
						drawImageScaled(tiles[index], NULL, &pos);
					}
					else
					{
						drawImageScaled(sprites[0], NULL, &pos);
					}
				}
				else
				{
					drawImageScaled(sprites[0], NULL, &pos);
				}
			}
		}
	}
}

void drawBackground(long camx, long camy)
{
	long z;
	for ( z = 0; z < OBSTACLELAYER; z++ )
	{
		drawLayer(camx, camy, z, &map);
	}
}

void drawForeground(long camx, long camy)
{
	long z;
	for ( z = OBSTACLELAYER; z < MAPLAYERS; z++ )
	{
		drawLayer(camx, camy, z, &map);
	}
}

/*-------------------------------------------------------------------------------

	drawClearBuffers

	clears the screen and resets zbuffer

-------------------------------------------------------------------------------*/

void drawClearBuffers()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	//drawRect(NULL, 0, 255);
}

/*-------------------------------------------------------------------------------

	raycast

	Performs raycasting from the given camera's position through the
	environment to update minimap

-------------------------------------------------------------------------------*/

void raycast(view_t* camera, Sint8 (*minimap)[MINIMAP_MAX_DIMENSION])
{
	long posx, posy;
	real_t fracx, fracy;
	long inx, iny, inx2, iny2;
	real_t rx, ry;
	real_t d, dstart, dend;
	long sx;
	real_t arx, ary;
	long dincx, dincy;
	real_t dval0, dval1;

	Uint8 light;
	Sint32 z;
	bool zhit[MAPLAYERS], wallhit;

	posx = floor(camera->x);
	posy = floor(camera->y); // integer coordinates
	fracx = camera->x - posx;
	fracy = camera->y - posy; // fraction coordinates

	real_t wfov = (fov * camera->winw / camera->winh) * PI / 180.f;
	dstart = CLIPNEAR / 16.0;

	// ray vector
	rx = cos(camera->ang - wfov / 2.f);
	ry = sin(camera->ang - wfov / 2.f);

	// originally we cast a ray for every column of pixels in the
	// camera viewport. now we just shoot out 300 rays to save time.
	// this makes this function less accurate at distance, but right
	// now it's good enough!
	static const int NUMRAYS = 300;

	// TODO replace this with a simple line algorithm that performs
	// a basic line check from the camera origin through every tile.

	for ( sx = 0; sx < NUMRAYS; sx++ )
	{
		inx = posx;
		iny = posy;
		inx2 = inx;
		iny2 = iny;

		arx = 0;
		if (rx)
		{
			arx = 1.0 / fabs(rx);  // distance increments
		}
		ary = 0;
		if (ry)
		{
			ary = 1.0 / fabs(ry);
		}

		// dval0=dend+1 is there to prevent infinite loops when ray is parallel to axis
		dincx = 0;
		dval0 = 1e32;
		dincy = 0;
		dval1 = 1e32;

		// calculate integer coordinate increments
		// x-axis:
		if (rx < 0)
		{
			dincx = -1;
			dval0 = fracx * arx;
		}
		else if (rx > 0)
		{
			dincx = 1;
			dval0 = (1 - fracx) * arx;
		}

		// y-axis:
		if (ry < 0)
		{
			dincy = -1;
			dval1 = fracy * ary;
		}
		else if (ry > 0)
		{
			dincy = 1;
			dval1 = (1 - fracy) * ary;
		}

		d = 0;
		dend = CLIPFAR / 16;
		do
		{
			inx2 = inx;
			iny2 = iny;

			// move the ray one square forward
			if (dval1 > dval0)
			{
				inx += dincx;
				d = dval0;
				dval0 += arx;
			}
			else
			{
				iny += dincy;
				d = dval1;
				dval1 += ary;
			}

			if ( inx >= 0 && iny >= 0 && inx < map.width && iny < map.height )
			{
				for ( z = 0; z < MAPLAYERS; z++ )
				{
					zhit[z] = false;
					if ( map.tiles[z + iny * MAPLAYERS + inx * MAPLAYERS * map.height] && d > dstart )   // hit something solid
					{
						zhit[z] = true;

						// collect light information
						if ( inx2 >= 0 && iny2 >= 0 && inx2 < map.width && iny2 < map.height )
						{
							if ( map.tiles[z + iny2 * MAPLAYERS + inx2 * MAPLAYERS * map.height] )
							{
								continue;
							}
							light = std::min(std::max(0, lightmap[iny2 + inx2 * map.height]), 255);
						}
						else
						{
							light = 128;
						}

						// update minimap
						if ( d < 16 && z == OBSTACLELAYER )
						{
							if ( light > 0 )
							{
								minimap[iny][inx] = 2;  // wall space
							}
						}
					}
					else if ( z == OBSTACLELAYER )
					{
						// update minimap to show empty region
						if ( inx >= 0 && iny >= 0 && inx < map.width && iny < map.height )
						{
							light = std::min(std::max(0, lightmap[iny + inx * map.height]), 255);
						}
						else
						{
							light = 128;
						}
						if ( d < 16 )
						{
							if ( light > 0 && map.tiles[iny * MAPLAYERS + inx * MAPLAYERS * map.height] )
							{
								minimap[iny][inx] = 1;  // walkable space
							}
							else if ( map.tiles[z + iny * MAPLAYERS + inx * MAPLAYERS * map.height] )
							{
								minimap[iny][inx] = 0;  // no floor
							}
						}
					}
				}
				wallhit = true;
				for ( z = 0; z < MAPLAYERS; z++ )
				{
					if ( zhit[z] == false )
					{
						wallhit = false;
					}
				}
				if ( wallhit == true )
				{
					break;
				}
			}
		}
		while (d < dend);

		// new ray vector for next column
		rx = cos(camera->ang - wfov / 2.f + (wfov / NUMRAYS) * sx);
		ry = sin(camera->ang - wfov / 2.f + (wfov / NUMRAYS) * sx);
	}
}

/*-------------------------------------------------------------------------------

	drawEntities3D

	Draws all entities in the level as either voxel models or sprites

-------------------------------------------------------------------------------*/

void drawEntities3D(view_t* camera, int mode)
{
	node_t* node;
	Entity* entity;
	long x, y;

	static bool draw_ents = true;
	if (keystatus[SDL_SCANCODE_P] && !command && enableDebugKeys) {
	    keystatus[SDL_SCANCODE_P] = 0;
	    draw_ents = (draw_ents==false);
	}
	if (!draw_ents) {
	    return;
	}

	if ( map.entities->first == nullptr )
	{
		return;
	}

	enum SpriteTypes
	{
		SPRITE_ENTITY,
		SPRITE_HPBAR,
		SPRITE_DIALOGUE
	};
	std::vector<std::tuple<real_t, void*, SpriteTypes>> spritesToDraw;

	int currentPlayerViewport = -1;
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( &cameras[c] == camera )
		{
			currentPlayerViewport = c;
			break;
		}
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
	node_t* nextnode = nullptr;
	for ( node = map.entities->first; node != nullptr; node = nextnode )
	{
		entity = (Entity*)node->element;
		nextnode = node->next;
		if ( node->next == nullptr && node->list == map.entities )
		{
			if ( map.worldUI && map.worldUI->first )
			{
				// quick way to attach worldUI to the end of map.entities.
				nextnode = map.worldUI->first;
			}
		}

        if ( !entity->flags[OVERDRAW] )
        {
            const real_t rx = entity->x / 16.0;
            const real_t ry = entity->y / 16.0;
	        if ( behindCamera(*camera, rx, ry) )
	        {
	            continue;
	        }
	    }
		if ( entity->flags[INVISIBLE] )
		{
			continue;
		}
		if ( entity->flags[UNCLICKABLE] && mode == ENTITYUIDS )
		{
			continue;
		}
		if ( entity->flags[GENIUS] )
		{
			// genius entities are not drawn when the camera is inside their bounding box
			if ( camera->x >= (entity->x - entity->sizex) / 16 && camera->x <= (entity->x + entity->sizex) / 16 )
				if ( camera->y >= (entity->y - entity->sizey) / 16 && camera->y <= (entity->y + entity->sizey) / 16 )
				{
					continue;
				}
		}
		if ( entity->flags[OVERDRAW] && splitscreen )
		{
			// need to skip some HUD models in splitscreen.
			if ( currentPlayerViewport >= 0 )
			{
				if ( entity->behavior == &actHudWeapon 
					|| entity->behavior == &actHudArm 
					|| entity->behavior == &actGib
					|| entity->behavior == &actFlame )
				{
					// the gibs are from casting magic in the HUD
					if ( entity->skill[11] != currentPlayerViewport )
					{
						continue;
					}
				}
				else if ( entity->behavior == &actHudAdditional
					|| entity->behavior == &actHudArrowModel
					|| entity->behavior == &actHudShield
					|| entity->behavior == &actLeftHandMagic
					|| entity->behavior == &actRightHandMagic )
				{
					if ( entity->skill[2] != currentPlayerViewport )
					{
						continue;
					}
				}
			}
		}
		x = entity->x / 16;
		y = entity->y / 16;
		if ( x >= 0 && y >= 0 && x < map.width && y < map.height )
		{
		    if ( !entity->flags[OVERDRAW] )
		    {
		        if ( !map.vismap[y + x * map.height] && !entity->monsterEntityRenderAsTelepath )
		        {
		            continue;
		        }
		    }
			if ( entity->flags[SPRITE] == false )
			{
				glDrawVoxel(camera, entity, mode);
			}
			else
			{
				if ( entity->behavior == &actSpriteNametag )
				{
					int playersTag = playerEntityMatchesUid(entity->parent);
					if ( playersTag >= 0 )
					{
						real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
							+ pow(camera->y * 16.0 - entity->y, 2));
						spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
					}
				}
				else if ( entity->behavior == &actSpriteWorldTooltip )
				{
					real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
						+ pow(camera->y * 16.0 - entity->y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
				}
				else if ( entity->behavior == &actDamageGib )
				{
					real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
						+ pow(camera->y * 16.0 - entity->y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
				}
				else
				{
					if ( !entity->flags[OVERDRAW] )
					{
						real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
							+ pow(camera->y * 16.0 - entity->y, 2));
						spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
					}
					else
					{
						glDrawSprite(camera, entity, mode);
					}
				}
			}
		}
		else
		{
			if ( entity->flags[SPRITE] == false )
			{
				glDrawVoxel(camera, entity, mode);
			}
			else if ( entity->behavior == &actSpriteWorldTooltip )
			{
				glDrawWorldUISprite(camera, entity, mode);
			}
			else 
			{
				glDrawSprite(camera, entity, mode);
			}
		}
	}

#ifndef EDITOR
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		for ( auto& enemybar : enemyHPDamageBarHandler[i].HPBars )
		{
			real_t camDist = (pow(camera->x * 16.0 - enemybar.second.worldX, 2)
				+ pow(camera->y * 16.0 - enemybar.second.worldY, 2));
			spritesToDraw.push_back(std::make_tuple(camDist, &enemybar, SPRITE_HPBAR));
		}
		if ( players[i]->worldUI.worldTooltipDialogue.playerDialogue.init && players[i]->worldUI.worldTooltipDialogue.playerDialogue.draw )
		{
			if ( i == currentPlayerViewport )
			{
				real_t camDist = (pow(camera->x * 16.0 - players[i]->worldUI.worldTooltipDialogue.playerDialogue.x, 2)
					+ pow(camera->y * 16.0 - players[i]->worldUI.worldTooltipDialogue.playerDialogue.y, 2));
				spritesToDraw.push_back(std::make_tuple(camDist, &players[i]->worldUI.worldTooltipDialogue.playerDialogue, SPRITE_DIALOGUE));
			}
		}
		for ( auto it = players[i]->worldUI.worldTooltipDialogue.sharedDialogues.begin();
			it != players[i]->worldUI.worldTooltipDialogue.sharedDialogues.end(); ++it )
		{
			if ( it->second.init && it->second.draw )
			{
				if ( i == currentPlayerViewport )
				{
					real_t camDist = (pow(camera->x * 16.0 - it->second.x, 2)
						+ pow(camera->y * 16.0 - it->second.y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, &it->second, SPRITE_DIALOGUE));
				}
			}
		}
	}
#endif

	std::sort(spritesToDraw.begin(), spritesToDraw.end(), 
		[](const std::tuple<real_t, void*, SpriteTypes>& lhs, const std::tuple<real_t, void*, SpriteTypes>& rhs) {
		return lhs > rhs;
	});
	for ( auto& distSpriteType : spritesToDraw )
	{
		if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_ENTITY )
		{
			Entity* entity = (Entity*)std::get<1>(distSpriteType);
			if ( entity->behavior == &actSpriteNametag )
			{
				int playersTag = playerEntityMatchesUid(entity->parent);
				if ( playersTag >= 0 )
				{
					glDrawSpriteFromImage(camera, entity, stats[playersTag]->name, mode);
				}
			}
			else if ( entity->behavior == &actSpriteWorldTooltip )
			{
				glDrawWorldUISprite(camera, entity, mode);
			}
			else if ( entity->behavior == &actDamageGib )
			{
				char buf[16];
				if ( entity->skill[0] < 0 )
				{
					snprintf(buf, sizeof(buf), "+%d", -entity->skill[0]);
					glDrawSpriteFromImage(camera, entity, buf, mode);
				}
				else
				{
					snprintf(buf, sizeof(buf), "%d", entity->skill[0]);
					glDrawSpriteFromImage(camera, entity, buf, mode);
				}
			}
			else
			{
				glDrawSprite(camera, entity, mode);
			}
		}
		else if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_HPBAR )
		{
#ifndef EDITOR
			auto enemybar = (std::pair<Uint32, EnemyHPDamageBarHandler::EnemyHPDetails>*)std::get<1>(distSpriteType);
			glDrawEnemyBarSprite(camera, mode, &enemybar->second, false);
#endif
		}
		else if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_DIALOGUE )
		{
#ifndef EDITOR
			auto dialogue = (Player::WorldUI_t::WorldTooltipDialogue_t::Dialogue_t*)std::get<1>(distSpriteType);
			glDrawWorldDialogueSprite(camera, dialogue, mode);
#endif
		}
	}

	glDisable(GL_SCISSOR_TEST);
	glScissor(0, 0, xres, yres);
}

/*-------------------------------------------------------------------------------

	drawEntities2D

	Draws all entities in the level as sprites while accounting for the given
	camera coordinates

-------------------------------------------------------------------------------*/

void drawEntities2D(long camx, long camy)
{
	node_t* node;
	Entity* entity;
	SDL_Rect pos, box;
	int offsetx = 0;
	int offsety = 0;

	if ( map.entities->first == nullptr )
	{
		return;
	}

	// draw entities
	for ( node = map.entities->first; node != nullptr; node = node->next )
	{
		entity = (Entity*)node->element;
		if ( entity->flags[INVISIBLE] )
		{
			continue;
		}
		pos.x = entity->x * (TEXTURESIZE / 16) - camx;
		pos.y = entity->y * (TEXTURESIZE / 16) - camy;
		pos.w = TEXTURESIZE;
		pos.h = TEXTURESIZE;
		//ttfPrintText(ttf8, 100, 100, inputstr); debug any errant text input in editor

		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != nullptr )
			{
				if ( entity == selectedEntity[0] )
				{
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box, makeColorRGB(255, 0, 0), 255);
					box.w = TEXTURESIZE - 2;
					box.h = TEXTURESIZE - 2;
					box.x = pos.x + 1;
					box.y = pos.y + 1;
					drawRect(&box, makeColorRGB(0, 0, 255), 255);
				}
				
				// if item sprite and the item index is not 0 (NULL), or 1 (RANDOM)
				if ( entity->sprite == 8 && entity->skill[10] > 1 )
				{
					// draw the item sprite in the editor layout
					Item* tmpItem = newItem(static_cast<ItemType>(entity->skill[10] - 2), static_cast<Status>(0), 0, 0, 0, 0, nullptr);
					drawImageScaled(itemSprite(tmpItem), nullptr, &pos);
					free(tmpItem);
				}
				else if ( entity->sprite == 133 )
				{
					pos.y += sprites[entity->sprite]->h / 2;
					pos.x += sprites[entity->sprite]->w / 2;
					switch ( entity->signalInputDirection )
					{
						case 0:
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 0.f, 255);
							break;
						case 1:
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 3 * PI / 2, 255);
							break;
						case 2:
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI, 255);
							break;
						case 3:
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI / 2, 255);
							break;
					}
				}
				else
				{
					// draw sprite normally from sprites list
					drawImageScaled(sprites[entity->sprite], nullptr, &pos);
				}
			}
			else
			{
				if ( entity == selectedEntity[0] )
				{
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box, makeColorRGB(255, 0, 0), 255);
					box.w = TEXTURESIZE - 2;
					box.h = TEXTURESIZE - 2;
					box.x = pos.x + 1;
					box.y = pos.y + 1;
					drawRect(&box, makeColorRGB(0, 0, 255), 255);
				}
				drawImageScaled(sprites[0], nullptr, &pos);
			}
		}
		else
		{
			if ( entity == selectedEntity[0] )
			{
				// draws a box around the sprite
				box.w = TEXTURESIZE;
				box.h = TEXTURESIZE;
				box.x = pos.x;
				box.y = pos.y;
				drawRect(&box, makeColorRGB(255, 0, 0), 255);
				box.w = TEXTURESIZE - 2;
				box.h = TEXTURESIZE - 2;
				box.x = pos.x + 1;
				box.y = pos.y + 1;
				drawRect(&box, makeColorRGB(0, 0, 255), 255);
			}
			drawImageScaled(sprites[0], nullptr, &pos);
		}
	}

	// draw hover text for entities over the top of sprites.
	for ( node = map.entities->first;
		  node != nullptr
#ifndef NINTENDO
			&& (openwindow == 0
			&& savewindow == 0)
#endif
		  ;
		  node = node->next
		)
	{
		entity = (Entity*)node->element;
		if ( entity->flags[INVISIBLE] )
		{
			continue;
		}
		pos.x = entity->x * (TEXTURESIZE / 16) - camx;
		pos.y = entity->y * (TEXTURESIZE / 16) - camy;
		pos.w = TEXTURESIZE;
		pos.h = TEXTURESIZE;
		//ttfPrintText(ttf8, 100, 100, inputstr); debug any errant text input in editor

		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != nullptr )
			{
				if ( entity == selectedEntity[0] )
				{
					int spriteType = checkSpriteType(selectedEntity[0]->sprite);
					char tmpStr[1024] = "";
					char tmpStr2[1024] = "";
					int padx = pos.x + 10;
					int pady = pos.y - 40;
					Uint32 color = makeColorRGB(255, 255, 255);
					Uint32 colorWhite = makeColorRGB(255, 255, 255);
					switch ( spriteType )
					{
						case 1: //monsters
							pady += 10;
							if ( entity->getStats() != nullptr ) {
								strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
								ttfPrintText(ttf8, padx, pady - 10, tmpStr);
								snprintf(tmpStr, sizeof(entity->getStats()->name), "Name: %s", entity->getStats()->name);
								ttfPrintText(ttf8, padx, pady, tmpStr);
								snprintf(tmpStr, 10, "HP: %d", entity->getStats()->MAXHP);
								ttfPrintText(ttf8, padx, pady + 10, tmpStr);
								snprintf(tmpStr, 10, "Level: %d", entity->getStats()->LVL);
								ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							}


							break;
						case 2: //chest
							pady += 5;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady, tmpStr);
							switch ( (int)entity->yaw )
							{
								case 0:
									strcpy(tmpStr, "Facing: EAST");
									break;
								case 1:
									strcpy(tmpStr, "Facing: SOUTH");
									break;
								case 2:
									strcpy(tmpStr, "Facing: WEST");
									break;
								case 3:
									strcpy(tmpStr, "Facing: NORTH");
									break;
								default:
									strcpy(tmpStr, "Facing: Invalid");
									break;

							}
							ttfPrintText(ttf8, padx, pady + 10, tmpStr);

							switch ( entity->skill[9] )
							{
								case 0:
									strcpy(tmpStr, "Type: Random");
									break;
								case 1:
									strcpy(tmpStr, "Type: Garbage");
									break;
								case 2:
									strcpy(tmpStr, "Type: Food");
									break;
								case 3:
									strcpy(tmpStr, "Type: Jewelry");
									break;
								case 4:
									strcpy(tmpStr, "Type: Equipment");
									break;
								case 5:
									strcpy(tmpStr, "Type: Tools");
									break;
								case 6:
									strcpy(tmpStr, "Type: Magical");
									break;
								case 7:
									strcpy(tmpStr, "Type: Potions");
									break;
								case 8:
									strcpy(tmpStr, "Type: Empty");
									break;
								default:
									strcpy(tmpStr, "Type: Random");
									break;
							}
							ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							break;

						case 3: //Items
							pady += 5;
							strcpy(tmpStr, itemNameStrings[selectedEntity[0]->skill[10]]);
							ttfPrintText(ttf8, padx, pady - 20, tmpStr);
							color = makeColorRGB(255, 255, 255);
							pady += 2;

							strcpy(tmpStr, "Status: ");
							ttfPrintTextColor(ttf8, padx, pady - 10, colorWhite, 1, tmpStr);
							switch ( (int)selectedEntity[0]->skill[11] )
							{
								case 1:
									strcpy(tmpStr, "Broken");
									color = makeColorRGB(255, 0, 0);
									break;
								case 2:
									strcpy(tmpStr, "Decrepit");
									color = makeColorRGB(200, 128, 0);
									break;
								case 3:
									strcpy(tmpStr, "Worn");
									color = makeColorRGB(255, 255, 0);
									break;
								case 4:
									strcpy(tmpStr, "Servicable");
									color = makeColorRGB(128, 200, 0);
									break;
								case 5:
									strcpy(tmpStr, "Excellent");
									color = makeColorRGB(0, 255, 0);
									break;
								default:
									strcpy(tmpStr, "?");
									color = makeColorRGB(0, 168, 255);
									break;
							}
							ttfPrintTextColor(ttf8, padx + 56, pady - 10, color, 1, tmpStr);

							strcpy(tmpStr, "Bless: ");
							ttfPrintTextColor(ttf8, padx, pady, colorWhite, 1, tmpStr);
							if ( selectedEntity[0]->skill[12] < 0 )
							{
								snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(255, 0, 0);
							}
							else if ( selectedEntity[0]->skill[12] == 0 )
							{
								snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(255, 255, 255);
							}
							else if ( selectedEntity[0]->skill[12] == 10 )
							{
								strcpy(tmpStr2, "?");
								color = makeColorRGB(0, 168, 255);
							}
							else
							{
								snprintf(tmpStr2, 10, "+%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(0, 255, 0);
							}
							ttfPrintTextColor(ttf8, padx + 48, pady, color, 1, tmpStr2);

							strcpy(tmpStr, "Qty: ");
							ttfPrintTextColor(ttf8, padx, pady + 10, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[13]);
							ttfPrintTextColor(ttf8, padx + 32, pady + 10, colorWhite, 1, tmpStr2);

							pady += 2;
							strcpy(tmpStr, "Identified: ");
							ttfPrintTextColor(ttf8, padx, pady + 20, colorWhite, 1, tmpStr);
							if ( (int)selectedEntity[0]->skill[15] == 0 )
							{
								strcpy(tmpStr2, "No");
								color = makeColorRGB(255, 255, 0);
							}
							else if ( (int)selectedEntity[0]->skill[15] == 1 )
							{
								strcpy(tmpStr2, "Yes");
								color = makeColorRGB(0, 255, 0);
							}
							else
							{
								strcpy(tmpStr2, "?");
								color = makeColorRGB(0, 168, 255);
							}
							ttfPrintTextColor(ttf8, padx + 80, pady + 20, color, 1, tmpStr2);
							break;
						case 4: //summoning trap
							pady += 5;
							offsety = -40;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);

							offsety += 10;
							strcpy(tmpStr, "Type: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							strcpy(tmpStr2, monsterEditorNameStrings[entity->skill[0]]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Qty: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[1]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Time: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[2]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Amount: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[3]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Power to: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							if ( selectedEntity[0]->skill[4] == 1 )
							{
								strcpy(tmpStr2, "Spawn");
							}
							else
							{
								strcpy(tmpStr2, "Disable");
							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Stop Chance: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[5]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);
							break;
						case 5: //power crystal
							pady += 5;
							offsety = -20;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);

							offsety += 10;
							strcpy(tmpStr, "Facing: ");
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);
							offsetx = strlen(tmpStr) * 8 - 8;
							switch ( (int)entity->yaw )
							{
								case 0:
									strcpy(tmpStr2, "EAST");
									break;
								case 1:
									strcpy(tmpStr2, "SOUTH");
									break;
								case 2:
									strcpy(tmpStr2, "WEST");
									break;
								case 3:
									strcpy(tmpStr2, "NORTH");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Nodes: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->crystalNumElectricityNodes);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Rotation: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							switch ( (int)entity->crystalTurnReverse )
							{
								case 0:
									strcpy(tmpStr2, "Clockwise");
									break;
								case 1:
									strcpy(tmpStr2, "Anti-Clockwise");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Spell to Activate: ");
							offsetx = strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							switch ( (int)entity->crystalSpellToActivate )
							{
								case 0:
									strcpy(tmpStr2, "No");
									break;
								case 1:
									strcpy(tmpStr2, "Yes");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);
							break;
						case 16:
						case 13:
						{
							char buf[256] = "";
							int totalChars = 0;
							for ( int i = (spriteType == 16 ? 4 : 8); i < 60; ++i )
							{
								if ( selectedEntity[0]->skill[i] != 0 && i != 28 ) // skill[28] is circuit status.
								{
									for ( int c = 0; c < 4; ++c )
									{
										if ( static_cast<char>((selectedEntity[0]->skill[i] >> (c * 8)) & 0xFF) == '\0'
											&& i != 59 && selectedEntity[0]->skill[i + 1] != 0 )
										{
											// don't add '\0' termination unless the next skill slot is empty as we have more data to read.
										}
										else
										{
											buf[totalChars] = static_cast<char>((selectedEntity[0]->skill[i] >> (c * 8)) & 0xFF);
											++totalChars;
										}
									}
								}
							}
							if ( buf[totalChars] != '\0' )
							{
								buf[totalChars] = '\0';
							}
							int numLines = 0;
							std::vector<std::string> lines;
							lines.push_back(spriteEditorNameStrings[selectedEntity[0]->sprite]);

							strncpy(tmpStr, buf, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 48, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 96, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 144, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 192, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							if ( lines.size() > 3 )
							{
								offsety -= (lines.size() - 2) * 5;
							}

							size_t longestLine = 0;
							for ( auto it : lines )
							{
								longestLine = std::max(longestLine, strlen(it.c_str()));
							}

							SDL_Rect tooltip;
							tooltip.x = padx + offsetx - 4;
							tooltip.w = TTF8_WIDTH * longestLine + 8;
							tooltip.y = pady + offsety - 4;
							tooltip.h = lines.size() * TTF8_HEIGHT + 8;
							if ( lines.size() > 1 )
							{
								drawTooltip(&tooltip);
							}
							for ( auto it : lines )
							{
								ttfPrintText(ttf8, padx + offsetx, pady + offsety, it.c_str());
								offsety += 10;
							}
						}
							break;
						default:
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							break;

					}
				}
				else if ( (omousex / TEXTURESIZE) * 32 == pos.x
						&& (omousey / TEXTURESIZE) * 32 == pos.y
						&& selectedEntity[0] == NULL
#ifndef NINTENDO
						&& hovertext
#endif
						)
				{
					// handle mouseover sprite name tooltip in main editor screen
					int padx = pos.x + 10;
					int pady = pos.y - 20;
					int spriteType = checkSpriteType(entity->sprite);
					//offsety = 0;
					Stat* tmpStats = nullptr;
					if ( spriteType == 1 )
					{
						tmpStats = entity->getStats();
						if ( tmpStats != nullptr )
						{
							if ( strcmp(tmpStats->name, "") != 0 )
							{
								ttfPrintText(ttf8, padx, pady - offsety, tmpStats->name);
								offsety += 10;
							}
							ttfPrintText(ttf8, padx, pady - offsety, spriteEditorNameStrings[entity->sprite]);
							offsety += 10;
						}
					}
					else if ( spriteType == 3 )
					{
						ttfPrintText(ttf8, padx, pady - offsety, itemNameStrings[entity->skill[10]]);
						offsety += 10;
					}
					else
					{
						ttfPrintText(ttf8, padx, pady - offsety, spriteEditorNameStrings[entity->sprite]);
						offsety += 10;
					}
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	drawGrid

	Draws a white line grid for the tile map

-------------------------------------------------------------------------------*/

void drawGrid(long camx, long camy)
{
	long x, y;
	Uint32 color = makeColorRGB(127, 127, 127);
	drawLine(-camx, (map.height << TEXTUREPOWER) - camy, (map.width << TEXTUREPOWER) - camx, (map.height << TEXTUREPOWER) - camy, color, 255);
	drawLine((map.width << TEXTUREPOWER) - camx, -camy, (map.width << TEXTUREPOWER) - camx, (map.height << TEXTUREPOWER) - camy, color, 255);
	for ( y = 0; y < map.height; y++ )
	{
		for ( x = 0; x < map.width; x++ )
		{
			drawLine((x << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, ((x + 1) << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, color, 255);
			drawLine((x << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, (x << TEXTUREPOWER) - camx, ((y + 1) << TEXTUREPOWER) - camy, color, 255);
		}
	}
}

/*-------------------------------------------------------------------------------

	drawEditormap

	Draws a minimap in the upper right corner of the screen to represent
	the screen's position relative to the rest of the level

-------------------------------------------------------------------------------*/

void drawEditormap(long camx, long camy)
{
	SDL_Rect src, osrc;

	src.x = xres - 120;
	src.y = 24;
	src.w = 112;
	src.h = 112;
	drawRect(&src, makeColorRGB(0, 0, 0), 255);

	// initial box dimensions
	src.x = (xres - 120) + (((real_t)camx / TEXTURESIZE) * 112.0) / map.width;
	src.y = 24 + (((real_t)camy / TEXTURESIZE) * 112.0) / map.height;
	src.w = (112.0 / map.width) * ((real_t)xres / TEXTURESIZE);
	src.h = (112.0 / map.height) * ((real_t)yres / TEXTURESIZE);

	// clip at left edge
	if ( src.x < xres - 120 )
	{
		src.w -= (xres - 120) - src.x;
		src.x = xres - 120;
	}

	// clip at right edge
	if ( src.x + src.w > xres - 8 )
	{
		src.w = xres - 8 - src.x;
	}

	// clip at top edge
	if ( src.y < 24 )
	{
		src.h -= 24 - src.y;
		src.y = 24;
	}

	// clip at bottom edge
	if ( src.y + src.h > 136 )
	{
		src.h = 136 - src.y;
	}

	osrc.x = src.x + 1;
	osrc.y = src.y + 1;
	osrc.w = src.w - 2;
	osrc.h = src.h - 2;
	drawRect(&src, makeColorRGB(255, 255, 255), 255);
	drawRect(&osrc, makeColorRGB(0, 0, 0), 255);
}

/*-------------------------------------------------------------------------------

	drawWindow / drawDepressed

	Draws a rectangular box that fills the area inside the given screen
	coordinates

-------------------------------------------------------------------------------*/

void drawWindow(int x1, int y1, int x2, int y2)
{
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2 - x1;
	src.h = y2 - y1;
	drawRect(&src, makeColorRGB(160, 160, 192), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 1;
	src.h = y2 - y1 - 1;
	drawRect(&src, makeColorRGB(96, 96, 128), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 2;
	src.h = y2 - y1 - 2;
	drawRect(&src, makeColorRGB(128, 128, 160), 255);
}

void drawDepressed(int x1, int y1, int x2, int y2)
{
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2 - x1;
	src.h = y2 - y1;
	drawRect(&src, makeColorRGB(96, 96, 128), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 1;
	src.h = y2 - y1 - 1;
	drawRect(&src, makeColorRGB(160, 160, 192), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 2;
	src.h = y2 - y1 - 2;
	drawRect(&src, makeColorRGB(128, 128, 160), 255);
}

void drawWindowFancy(int x1, int y1, int x2, int y2)
{
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
	glBindTexture(GL_TEXTURE_2D, texid[(long int)fancyWindow_bmp->userdata]); // wood texture
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(x1 + 2, yres - y1 - 2);
	glTexCoord2f(0, (y2 - y1 - 4) / (real_t)tiles[30]->h);
	glVertex2f(x1 + 2, yres - y2 + 2);
	glTexCoord2f((x2 - x1 - 4) / (real_t)tiles[30]->w, (y2 - y1 - 4) / (real_t)tiles[30]->h);
	glVertex2f(x2 - 2, yres - y2 + 2);
	glTexCoord2f((x2 - x1 - 4) / (real_t)tiles[30]->w, 0);
	glVertex2f(x2 - 2, yres - y1 - 2);
	glEnd();
}

/*-------------------------------------------------------------------------------

	ttfPrintText / ttfPrintTextColor

	Prints an unformatted utf8 string to the screen, returning the width of
	the message surface in pixels. ttfPrintTextColor() also takes a 32-bit
	color argument

-------------------------------------------------------------------------------*/

SDL_Rect ttfPrintTextColor( TTF_Font* font, int x, int y, Uint32 color, bool outline, const char* str )
{
    const char* filename = "lang/en.ttf#12#1"; // default
    if (outline) {
        if (font == ttf8) {
            filename = "lang/en.ttf#12#1";
            x -= 1;
            y -= 1;
        }
        else if (font == ttf12) {
            filename = "lang/en.ttf#16#2";
            x -= 2;
            y -= 4;
        }
        else if (font == ttf16) {
            filename = "lang/en.ttf#22#2";
            x -= 2;
            y -= 4;
        }
    } else {
        if (font == ttf8) {
            filename = "lang/en.ttf#12#0";
        }
        else if (font == ttf12) {
            filename = "lang/en.ttf#16#0";
        }
        else if (font == ttf16) {
            filename = "lang/en.ttf#22#0";
        }
    }
    char buf[1024];
    char* ptr = buf;
    snprintf(buf, sizeof(buf), str);
    for (int c = 0; ptr[c] != '\0'; ++c) {
        if (ptr[c] == '\n') {
            ptr[c] = '\0';
            auto text = Text::get(ptr, filename, uint32ColorWhite, uint32ColorBlack);
            text->drawColor(SDL_Rect{0, 0, 0, 0}, SDL_Rect{x, y, 0, 0}, SDL_Rect{0, 0, xres, yres}, color);
            y += text->getHeight();
            ptr += c + 1;
        }
    }
    auto text = Text::get(ptr, filename, uint32ColorWhite, uint32ColorBlack);
    text->drawColor(SDL_Rect{0, 0, 0, 0}, SDL_Rect{x, y, 0, 0}, SDL_Rect{0, 0, xres, yres}, color);
    return SDL_Rect{x, y, (int)text->getWidth(), (int)text->getHeight()};
}

static SDL_Rect errorRect = { 0 };

SDL_Rect ttfPrintText( TTF_Font* font, int x, int y, const char* str )
{
	if ( !str )
	{
		return errorRect;
	}
	return ttfPrintTextColor(font, x, y, 0xFFFFFFFF, true, str);
}

/*-------------------------------------------------------------------------------

	ttfPrintTextFormatted / ttfPrintTextFormattedColor

	Prints a formatted utf8 string to the screen using
	ttfPrintText / ttfPrintTextColor

-------------------------------------------------------------------------------*/

SDL_Rect ttfPrintTextFormattedColor( TTF_Font* font, int x, int y, Uint32 color, char const * const fmt, ... )
{
	char str[1024] = { 0 };

	if ( !fmt )
	{
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font, x, y, color, true, str);
}

SDL_Rect ttfPrintTextFormatted( TTF_Font* font, int x, int y, char const * const fmt, ... )
{
	char str[1024] = { 0 };

	if ( !fmt )
	{
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font, x, y, 0xFFFFFFFF, true, str);
}

/*-------------------------------------------------------------------------------

	printText

	Prints unformatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void printText( SDL_Surface* font_bmp, int x, int y, const char* str )
{
	int c;
	int numbytes;
	SDL_Rect src, dest, odest;

	if ( strlen(str) > 2048 )
	{
		printlog("error: buffer overflow in printText\n");
		return;
	}

	// format the string
	numbytes = strlen(str);

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormatted

	Prints formatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void printTextFormatted( SDL_Surface* font_bmp, int x, int y, char const * const fmt, ... )
{
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
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedAlpha

	Prints formatted text to the screen using a font bitmap and taking an
	alpha value.

-------------------------------------------------------------------------------*/

void printTextFormattedAlpha(SDL_Surface* font_bmp, int x, int y, Uint8 alpha, char const * const fmt, ...)
{
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
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImageAlpha( font_bmp, &src, &dest, alpha );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap and taking a
	32-bit color value

-------------------------------------------------------------------------------*/

void printTextFormattedColor(SDL_Surface* font_bmp, int x, int y, Uint32 color, char const * const fmt, ...)
{
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
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImageColor( font_bmp, &src, &dest, color );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap, while coloring,
	rotating, and scaling it

-------------------------------------------------------------------------------*/

void printTextFormattedFancy(SDL_Surface* font_bmp, int x, int y, Uint32 color, real_t angle, real_t scale, char* fmt, ...)
{
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
	real_t newX = x;
	real_t newY = y;
	dest.w = ((real_t)font_bmp->w / 16.f) * scale;
	src.w = font_bmp->w / 16;
	dest.h = ((real_t)font_bmp->h / 16.f) * scale;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	int line = 0;
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			dest.x = newX;
			dest.y = newY;
			drawImageFancy( font_bmp, color, angle, &src, &dest );
			newX += (real_t)dest.w * cos(angle);
			newY += (real_t)dest.h * sin(angle);
		}
		else if ( str[c] == 10 )
		{
			line++;
			dest.x = x + dest.h * cos(angle + PI / 2) * line;
			dest.y = y + dest.h * sin(angle + PI / 2) * line;
		}
	}
}

/*-------------------------------------------------------------------------------

	draws a tooltip

	Draws a tooltip box

-------------------------------------------------------------------------------*/

void drawTooltip(SDL_Rect* src, Uint32 optionalColor)
{
	Uint32 color = makeColorRGB(0, 192, 255);
	if ( optionalColor == 0 )
	{
		drawRect(src, 0, 250);
	}
	else
	{
		color = optionalColor;
	}
	drawLine(src->x, src->y, src->x + src->w, src->y, color, 255);
	drawLine(src->x, src->y + src->h, src->x + src->w, src->y + src->h, color, 255);
	drawLine(src->x, src->y, src->x, src->y + src->h, color, 255);
	drawLine(src->x + src->w, src->y, src->x + src->w, src->y + src->h, color, 255);
}

void getColor(Uint32 color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
	*r = (color & 0x000000ff) >> 0;
	*g = (color & 0x0000ff00) >> 8;
	*b = (color & 0x00ff0000) >> 16;
	*a = (color & 0xff000000) >> 24;
}

bool behindCamera(const view_t& camera, real_t x, real_t y)
{
    const real_t dx = x - camera.x;
    const real_t dy = y - camera.y;
    const real_t len = sqrt(dx*dx + dy*dy);
    if (len < 4) {
        return false;
    }
    const real_t alen = 1.0 / len;

    const real_t v0x = cos(camera.ang);
    const real_t v0y = sin(camera.ang);
    const real_t v1x = dx * alen;
    const real_t v1y = dy * alen;

    const real_t dot = v0x * v1x + v0y * v1y;

    const real_t aspect = (real_t)camera.winw / (real_t)camera.winh;
    const real_t wfov = std::max((real_t)90.0, ((real_t)(fov + 30.0) * aspect)) * PI / 180.0;
    const real_t c = cos(wfov * 0.5);

    return dot < c;
}

bool testTileOccludes(map_t& map, int index) {
    assert(index >= 0 && index <= map.width * map.height * MAPLAYERS - MAPLAYERS);
	for (int z = 0; z < MAPLAYERS; z++) {
		if (!map.tiles[index + z]) {
			return false;
		}
	}
	return true;
}

void occlusionCulling(map_t& map, const view_t& camera)
{
	// cvars
#ifndef EDITOR
    static ConsoleVariable<int> max_distance("/culling_max_distance", CLIPFAR / 16);
    static ConsoleVariable<int> max_walls_hit("/culling_max_walls", 2);
	static ConsoleVariable<bool> diagonalCulling("/culling_expand_diagonal", true);
    static ConsoleVariable<bool> disabled("/skipculling", false);
#else
	static int ed_distance = CLIPFAR / 16;
	static int ed_walls_hit = 2;
	static bool ed_culling = true;
	static bool ed_disabled = false;
	auto* max_distance = &ed_distance;
	auto* max_walls_hit = &ed_walls_hit;
	auto* diagonalCulling = &ed_culling;
    auto* disabled = &ed_disabled;
#endif

	const int size = map.width * map.height;
	
    if (*disabled)
    {
        memset(map.vismap, 1, sizeof(bool) * size);
        return;
    }

    // clear vismap
    const int camx = std::min(std::max(0, (int)camera.x), (int)map.width - 1);
    const int camy = std::min(std::max(0, (int)camera.y), (int)map.height - 1);

    // don't do culling if camera in wall
    if ( map.tiles[OBSTACLELAYER + camy * MAPLAYERS + camx * MAPLAYERS * map.height] != 0 )
	{
		memset(map.vismap, 1, sizeof(bool) * size);
		return;
	}

    memset(map.vismap, 0, sizeof(bool) * size);
    map.vismap[camy + camx * map.height] = true;

    const int hoff = MAPLAYERS;
    const int woff = MAPLAYERS * map.height;

    // do line tests throughout the map
	const int beginx = std::max(0, camx - *max_distance);
	const int beginy = std::max(0, camy - *max_distance);
	const int endx = std::min((int)map.width - 1, camx + *max_distance);
	const int endy = std::min((int)map.height - 1, camy + *max_distance);
	for ( int u = beginx; u <= endx; u++ ) {
		for ( int v = beginy; v <= endy; v++ ) {
			if (map.vismap[v + u * map.height]) {
				continue;
			}
			const int uvindex = v * hoff + u * woff;
			if (testTileOccludes(map, uvindex)) {
				continue;
			}
			if (behindCamera(camera, (real_t)u + 0.5, (real_t)v + 0.5)) {
				continue;
			}
			for (int foo = -1; foo <= 1; ++foo) {
				for (int bar = -1; bar <= 1; ++bar) {
					if (!*diagonalCulling && foo && bar) {
						continue;
					}
					const int x = std::min(std::max(0, camx + foo), (int)map.width - 1);
					const int y = std::min(std::max(0, camy + bar), (int)map.height - 1);
					const int xyindex = y * hoff + x * woff;
					if (testTileOccludes(map, xyindex)) {
						continue;
					}
			        const int dx = u - x;
			        const int dy = v - y;
			        const int sdx = sgn(dx);
			        const int sdy = sgn(dy);
			        const int dxabs = abs(dx);
			        const int dyabs = abs(dy);
			        int wallshit = 0;
			        if (dxabs >= dyabs) { // the line is more horizontal than vertical
			            int a = dxabs >> 1;
			            int index = uvindex;
				        for (int i = 1; i < dxabs; ++i) {
					        index -= woff * sdx;
					        a += dyabs;
					        if (a >= dxabs) {
						        a -= dxabs;
						        index -= hoff * sdy;
					        }
					        if (testTileOccludes(map, index)) {
						        ++wallshit;
						        if (wallshit >= *max_walls_hit) {
						            break;
						        }
					        }
				        }
			        } else { // the line is more vertical than horizontal
			            int a = dyabs >> 1;
			            int index = uvindex;
				        for (int i = 1; i < dyabs; ++i) {
					        index -= hoff * sdy;
					        a += dxabs;
					        if (a >= dyabs) {
						        a -= dyabs;
					            index -= woff * sdx;
					        }
					        if (testTileOccludes(map, index)) {
						        ++wallshit;
						        if (wallshit >= *max_walls_hit) {
						            break;
						        }
					        }
				        }
			        }
			        if (wallshit < *max_walls_hit) {
                        map.vismap[v + u * map.height] = true;
						goto next;
			        }
		        }
	        }
next:;
        }
    }

	// expand vismap one tile in each direction
	const int w = map.width;
	const int w1 = map.width - 1;
	const int h = map.height;
	const int h1 = map.height - 1;
	bool* vmap = (bool*)malloc(sizeof(bool) * size);
    for ( int u = 0; u < w; u++ ) {
        for ( int v = 0; v < h; v++ ) {
            const int index = v + u * h;
	        vmap[index] = map.vismap[index];
		    if (!vmap[index]) {
		        if (v >= 1) {
		            if (map.vismap[index - 1]) {
		                vmap[index] = true;
		                continue;
		            }
		            if (*diagonalCulling) {
		                if (u >= 1 && map.vismap[index - h - 1]) {
		                    vmap[index] = true;
		                    continue;
		                }
		                if (u < w1 && map.vismap[index + h - 1]) {
		                    vmap[index] = true;
		                    continue;
		                }
		            }
		        }
		        if (v < h1) {
		            if (map.vismap[index + 1]) {
		                vmap[index] = true;
		                continue;
		            }
		            if (*diagonalCulling) {
		                if (u >= 1 && map.vismap[index - h + 1]) {
		                    vmap[index] = true;
		                    continue;
		                }
		                if (u < w1 && map.vismap[index + h + 1]) {
		                    vmap[index] = true;
		                    continue;
		                }
		            }
		        }
		        if (u >= 1 && map.vismap[index - h]) {
		            vmap[index] = true;
		            continue;
		        }
		        if (u < w1 && map.vismap[index + h]) {
		            vmap[index] = true;
		            continue;
		        }
		    }
		}
	}
	free(map.vismap);
	map.vismap = vmap;
}
