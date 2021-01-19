// Text.cpp

#include <assert.h>

#include "../main.hpp"
#include "../draw.hpp"
#include "Text.hpp"
#include "Font.hpp"

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
	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if (texid) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}
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
	TTF_Font* ttf = font->getTTF();

	SDL_Color colorBlack = { 0, 0, 0, 255 };
	SDL_Color colorWhite = { 255, 255, 255, 255 };

	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}

	if (outlineSize > 0) {
		TTF_SetFontOutline(ttf, outlineSize);
		surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorBlack, xres);
		TTF_SetFontOutline(ttf, 0);
		SDL_Surface* text = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorWhite, xres);
		SDL_Rect rect;
		rect.x = 1; rect.y = 1;
		SDL_BlitSurface(text, NULL, surf, &rect);
		SDL_FreeSurface(text);
	} else {
		TTF_SetFontOutline(ttf, 0);
		surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.c_str(), colorWhite, xres);
	}
	assert(surf);
	if (texid == 0) {
		glGenTextures(1, &texid);
	}

	width = 0;
	height = 0;
	int scan = surf->pitch / surf->format->BytesPerPixel;
	for (int y = 0; y < surf->h; ++y) {
		for (int x = 0; x < surf->w; ++x) {
			if (((Uint32 *)surf->pixels)[x + y * scan] != 0) {
				width = std::max(width, x);
				height = std::max(height, y);
			}
		}
	}
	width += 4;
	height += 4;

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, width, height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Rect dest;
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = width;
	src.h = height;
	dest.x = 0;
	dest.y = 0;
	SDL_BlitSurface(surf, &src, newSurf, &dest); // blit onto a purely RGBA Surface
	SDL_FreeSurface(surf);
	surf = newSurf;

	// load the new surface as a GL texture
	SDL_LockSurface(surf);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	SDL_UnlockSurface(surf);

	rendered = true;
}

void Text::draw(SDL_Rect src, SDL_Rect dest) {
	drawColor(src, dest, 0xffffffff);
}

void Text::drawColor(SDL_Rect src, SDL_Rect dest, const Uint32& color) {
	if (!rendered) {
		render();
	}
	if (!rendered) {
		return;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	src.w = src.w <= 0 ? surf->w : src.w;
	src.h = src.h <= 0 ? surf->h : src.h;
	dest.w = dest.w <= 0 ? surf->w : dest.w;
	dest.h = dest.h <= 0 ? surf->h : dest.h;

	// bind texture
	glBindTexture(GL_TEXTURE_2D, texid);

	// consume color
	real_t r = ((Uint8)(color >> mainsurface->format->Rshift)) / 255.f;
	real_t g = ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f;
	real_t b = ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f;
	real_t a = ((Uint8)(color >> mainsurface->format->Ashift)) / 255.f;
	glColor4f(r, g, b, a);

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src.x / surf->w), 1.0 * ((real_t)src.y / surf->h));
	glVertex2f(dest.x, yres - dest.y);
	glTexCoord2f(1.0 * ((real_t)src.x / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	glVertex2f(dest.x, yres - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	glVertex2f(dest.x + dest.w, yres - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * ((real_t)src.y / surf->h));
	glVertex2f(dest.x + dest.w, yres - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
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