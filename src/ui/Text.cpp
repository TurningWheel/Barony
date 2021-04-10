// Text.cpp

#include <assert.h>

#include "../main.hpp"

#include "../external/stb/stb_truetype.h"
#include "../external/fontstash/fontstash.h"
#include "../external/fontstash/glfontstash.h"

#include "../draw.hpp"
#include "Text.hpp"
#include "Font.hpp"



#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  uint32_t rmask = 0xff000000;
  uint32_t gmask = 0x00ff0000;
  uint32_t bmask = 0x0000ff00;
  uint32_t amask = 0x000000ff;
#else // little endian, like x86
  uint32_t rmask = 0x000000ff;
  uint32_t gmask = 0x0000ff00;
  uint32_t bmask = 0x00ff0000;
  uint32_t amask = 0xff000000;
#endif

void convertPixelsToSDLSurfaceAndSaveToBmpFile(unsigned char* data, int width, int height)
{
	//We are given an array of one byte values, basically like GL_ALPHA texture type.
	//uint8_t* expanded_data = new uint8_t[width * height * 4];
	uint8_t* expanded_data = new uint8_t[width * height * 3];
	unsigned j = 0; //Index into the expanded data.
	for (unsigned i = 0; i < width * height; ++i)
	{
		//for (unsigned n = 0; n < 4; ++n)
		for (unsigned n = 0; n < 3; ++n)
		{
			expanded_data[j] = data[i];
			j++;
		}
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)expanded_data, width, height, 24, width * 3, rmask, gmask, bmask, 0);
	if (surf == nullptr)
	{
		printf("Creating surface failed: %s", SDL_GetError());
		exit(-1);
	}

	if (SDL_SaveBMP(surf, "renderedtext.bmp") != 0)
	{
		// Error saving bitmap
		printf("SDL_SaveBMP failed: %s\n", SDL_GetError());
		exit(-1);
	}

	SDL_FreeSurface(surf);
	delete[] expanded_data;
}

GLuint Text::vao = 0;
GLuint Text::vbo[BUFFER_TYPE_LENGTH] = { 0 };

const GLfloat Text::positions[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLfloat Text::texcoords[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLuint Text::indices[6]{
	0, 1, 2,
	0, 2, 3
};

Text::Text(const char* _name) {
	name = _name;
}

Text::~Text() {
	// if (surf) {
	// 	SDL_FreeSurface(surf);
	// 	surf = nullptr;
	// }
	// if (texid) { //TODO: Free fontstash stuff? Or, only do this on the Font's end? I think we may only need to do this on the Font's end...
	// 	glDeleteTextures(1, &texid);
	// 	texid = 0;
	// }
}

void Text::render() {
	// load font
	std::string strToRender;
	std::string fontName;
	Uint32 fontIndex = 0u;
	if ((fontIndex = name.find(fontBreak)) != std::string::npos) {
		fontName = name.substr(fontIndex + 1, UINT32_MAX);
		strToRender = name.substr(0, fontIndex).c_str();
	} else {
		fontName = Font::defaultFont;
		strToRender = name.c_str();
	}
	Font* font = Font::get(fontName.c_str());
	if (!font) {
		return;
	}
	//TTF_Font* ttf = font->getTTF();
	//FONScontext* fontstash = font->getFontstash();

	//TODO: It looks like the below caches a text string. Should probably overload Fontstash to let us do that...maybe?

	// SDL_Color colorBlack = { 0, 0, 0, 255 };
	// SDL_Color colorWhite = { 255, 255, 255, 255 };

	// if (surf) {
	// 	SDL_FreeSurface(surf);
	// 	surf = nullptr;
	// }

	// int outlineSize = font->getOutline();
	// if (outlineSize > 0) {
	// 	TTF_SetFontOutline(ttf, outlineSize);
	// 	surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorBlack, xres);
	// 	TTF_SetFontOutline(ttf, 0);
	// 	SDL_Surface* text = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorWhite, xres);
	// 	SDL_Rect rect;
	// 	rect.x = 1; rect.y = 1;
	// 	SDL_BlitSurface(text, NULL, surf, &rect);
	// 	SDL_FreeSurface(text);
	// } else {
	// 	TTF_SetFontOutline(ttf, 0);
	// 	surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorWhite, xres);
	// }
	// assert(surf);
	// if (texid == 0) {
	// 	glGenTextures(1, &texid);
	// }

	//TODO: Do we need this block?
	// width = 0;
	// height = 0;
	// int scan = surf->pitch / surf->format->BytesPerPixel;
	// for (int y = 0; y < surf->h; ++y) {
	// 	for (int x = 0; x < surf->w; ++x) {
	// 		if (((Uint32 *)surf->pixels)[x + y * scan] != 0) {
	// 			width = std::max(width, x);
	// 			height = std::max(height, y);
	// 		}
	// 	}
	// }
	// width += 4;
	// height += 4;

	// //translate the original surface to an RGBA surface
	// SDL_Surface* newSurf = SDL_CreateRGBSurface(0, width, height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	// SDL_Rect dest;
	// SDL_Rect src;
	// src.x = 0;
	// src.y = 0;
	// src.w = width;
	// src.h = height;
	// dest.x = 0;
	// dest.y = 0;
	// SDL_BlitSurface(surf, &src, newSurf, &dest); // blit onto a purely RGBA Surface
	// SDL_FreeSurface(surf); //TODO: Why does this give a heap exception in NX?
	// surf = newSurf;

	// // load the new surface as a GL texture
	// SDL_LockSurface(surf);
	// glBindTexture(GL_TEXTURE_2D, texid);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	// SDL_UnlockSurface(surf);

	rendered = true;
}

void Text::draw(SDL_Rect src, SDL_Rect dest) {
	drawColor(src, dest, 0xffffffff);
}


SDL_Surface* debug_image = nullptr;
GLuint debug_texture_id;

void Text::drawColor(SDL_Rect src, SDL_Rect dest, const Uint32& color) {
	if (!rendered) {
		render();
	}
	if (!rendered) {
		return;
	}
	std::string strToRender;
	std::string fontName;
	Uint32 fontIndex = 0u;
	if ((fontIndex = name.find(fontBreak)) != std::string::npos) {
		fontName = name.substr(fontIndex + 1, UINT32_MAX);
		strToRender = name.substr(0, fontIndex).c_str();
	} else {
		fontName = Font::defaultFont;
		strToRender = name.c_str();
	}
	Font* font = Font::get(fontName.c_str());
	if (!font) {
		return;
	}

	// glEnable(GL_DEBUG_OUTPUT);

	FONScontext* fontstash = font->getFontstash();
	if (!fontstash)
	{
		return;
	}
	fonsClearState(fontstash);
	fonsSetFont(fontstash, font->getFontId());
	//fonsSetSize(fontstash, font->getPointSize());
	fonsSetSize(fontstash, 124.0f);
	unsigned int brown = glfonsRGBA(192,128,0,128);;
	fonsSetColor(fontstash, brown);

	glViewport(0, 0, xres, yres);
			//glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisable(GL_LIGHT1);
	glDisable(GL_COLOR_MATERIAL);
	//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(0, xres, 0, yres, -1, 1);
	glOrtho(0, xres, yres, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
		glDisable(GL_DEPTH_TEST);
		glColor4ub(255,255,255,255);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisable(GL_LIGHT1);
	glDisable(GL_COLOR_MATERIAL);
		glEnable(GL_CULL_FACE);
	//TODO: Enable the BLEND stuff if needed.

	//TODO: Check the OpenGL return codes for errors...
	//TODO: Print to terminal any OpenGL errors or debugging info?

	// { //TODO:  REMOVE DEBUG WHEN DONE!
	// 	if (!debug_image)
	// 	{
	// 		debug_image = SDL_LoadBMP("data/test.bmp");
	// 		if (debug_image == nullptr)
	// 		{
	// 			std::cerr << "Boo! Failed to load debug_image data/test.bmp!\n";
	// 			exit(-1);
	// 		}

	// 		GLenum data_format = GL_BGR;
	// 		glGenTextures(1, &debug_texture_id);
	// 		glBindTexture(GL_TEXTURE_2D, debug_texture_id);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// 		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, debug_image->w, debug_image->h, 0, data_format, GL_UNSIGNED_BYTE, debug_image->pixels);
	// 		SDL_FreeSurface(debug_image);
	// 	}

	// 	glBindTexture(GL_TEXTURE_2D, debug_texture_id);
	// 	glEnable(GL_TEXTURE_2D);

	// 	glBegin(GL_QUADS);

	// 	// glTexCoord2f(0, 0);
	// 	// glVertex2f(50, SCREEN_HEIGHT - 50);
	// 	// glTexCoord2f(0, 1);
	// 	// glVertex2f(50, 50);
	// 	// glTexCoord2f(1, 1);
	// 	// glVertex2f(SCREEN_WIDTH - 50, 50);
	// 	// glTexCoord2f(1, 0);
	// 	// glVertex2f(SCREEN_WIDTH - 50, SCREEN_HEIGHT - 50);

	// 	glTexCoord2f(0, 0);
	// 	glVertex2f(50, 50);
	// 	glTexCoord2f(0, 1);
	// 	glVertex2f(50, yres - 50);
	// 	glTexCoord2f(1, 1);
	// 	glVertex2f(xres - 50, yres - 50);
	// 	glTexCoord2f(1, 0);
	// 	glVertex2f(xres - 50, 50);

	// 	glEnd();
	// }

	int text_width;
	int text_height;
	if (0 != font->sizeText(strToRender.c_str(), &text_width, &text_height))
	{
		std::cerr << "Failed to get size of text!\n"; //TODO: Delete debug!
		return;
	}

	// src.w = src.w <= 0 ? surf->w : src.w;
	// src.h = src.h <= 0 ? surf->h : src.h;
	// dest.w = dest.w <= 0 ? surf->w : dest.w;
	// dest.h = dest.h <= 0 ? surf->h : dest.h;
	src.w = src.w <= 0 ? text_width : src.w;
	src.h = src.h <= 0 ? text_height : src.h;
	dest.w = dest.w <= 0 ? text_width : dest.w;
	dest.h = dest.h <= 0 ? text_height : dest.h;

	// bind texture
	//glBindTexture(GL_TEXTURE_2D, texid);

	// consume color //TODO: Integrate with Fontstash's color...can probably add an option like "bool don't-recolor" or something?
	real_t r = ((Uint8)(color >> mainsurface->format->Rshift)) / 255.f;
	real_t g = ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f;
	real_t b = ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f;
	real_t a = ((Uint8)(color >> mainsurface->format->Ashift)) / 255.f;
	// glColor4f(r, g, b, a);

	// // draw quad
	// glBegin(GL_QUADS);
	// glTexCoord2f(1.0 * ((real_t)src.x / surf->w), 1.0 * ((real_t)src.y / surf->h));
	// glVertex2f(dest.x, yres - dest.y);
	// glTexCoord2f(1.0 * ((real_t)src.x / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	// glVertex2f(dest.x, yres - dest.y - dest.h);
	// glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	// glVertex2f(dest.x + dest.w, yres - dest.y - dest.h);
	// glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * ((real_t)src.y / surf->h));
	// glVertex2f(dest.x + dest.w, yres - dest.y);
	// glEnd();

	// unbind texture
	// glBindTexture(GL_TEXTURE_2D, 0);
	// glColor4f(1.f, 1.f, 1.f, 1.f);

	fonsDrawText(fontstash, 500, 400, strToRender.c_str(), nullptr);
	//glDisable(GL_DEPTH_TEST);
	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glEnable(GL_TEXTURE_2D); //Because it disables this but that breaks things...
	glEnable(GL_LINE_SMOOTH);
	// glDisable(GL_DEBUG_OUTPUT);
}

static std::unordered_map<std::string, Text*> hashed_text;
static const int TEXT_BUDGET = 1000;

Text* Text::get(const char* str, const char* font) {
	if (!str) {
		return nullptr;
	}
	if (font == nullptr || font[0] == '\0') {
		font = Font::defaultFont;
	}
	size_t len0 = strlen(str);
	size_t len1 = strlen(font);
	char textAndFont[65536]; // better not try to render more than 64kb of text...
	size_t totalLen = len0 + len1 + 2;
	if (totalLen > sizeof(textAndFont)) {
		assert(0 && "Trying to render > 64kb of ttf text");
		return nullptr;
	}
	snprintf(textAndFont, totalLen, "%s%c%s", str, Text::fontBreak, font);

	Text* text = nullptr;
	auto search = hashed_text.find(textAndFont);
	if (search == hashed_text.end()) {
		if (hashed_text.size() > TEXT_BUDGET) {
			dumpCache();
		}
		text = new Text(textAndFont);
		hashed_text.insert(std::make_pair(textAndFont, text));
	} else {
		text = search->second;
	}

	if (text && !text->rendered) {
		text->render();
	}
	return text;
}

void Text::dumpCache() {
	for (auto text : hashed_text) {
		delete text.second;
	}
	hashed_text.clear();
}