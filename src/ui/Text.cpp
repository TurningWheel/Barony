// Text.cpp

#include <assert.h>

#include "../main.hpp"
#include "../draw.hpp"
#include "Text.hpp"
#include "Font.hpp"

#include "../external/stb/stb_truetype.h"
#include "../external/fontstash/fontstash.h"
#include "../external/fontstash/glfontstash.h"

Text::Text(const char* _name) {
	name = _name;
}

Text::~Text() {
}

void Text::draw(SDL_Rect src, SDL_Rect dest, int override_pointsize) {
	drawColor(src, dest, 0xffffffff, override_pointsize);
}

void Text::draw(int x, int y, int override_pointsize) {
	drawColor(x, y, 0xffffffff, override_pointsize);
}

void Text::drawColor(int x, int y, const Uint32& color, int override_pointsize) {
	SDL_Rect src;
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	drawColor(src, dst, color, override_pointsize);
}

void Text::drawColor(SDL_Rect src, SDL_Rect dest, const Uint32& color, int override_pointsize) {
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

	FONScontext* fontstash = font->getFontstash();
	if (!fontstash)
	{
		return;
	}

	fonsClearState(fontstash);
	fonsSetFont(fontstash, font->getFontId());
	if (override_pointsize == 0)
	{
		fonsSetSize(fontstash, font->getPointSize());
	}
	else
	{
		fonsSetSize(fontstash, override_pointsize);
	}
	//fonsSetSize(fontstash, 124.0f);
	fonsSetColor(fontstash, color);

	if (0 != font->sizeText(strToRender.c_str(), &width, &height))
	{
		std::cerr << "Failed to get size of text!\n"; //TODO: Delete debug!
		return;
	}

	src.w = src.w <= 0 ? width : src.w;
	src.h = src.h <= 0 ? height : src.h;
	dest.w = dest.w <= 0 ? width : dest.w;
	dest.h = dest.h <= 0 ? height : dest.h;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	//glOrtho(0, SCREEN_WIDTH, 0, SCREEN_HEIGHT, -1, 1);
	glOrtho(0, xres, yres, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	fonsDrawText(fontstash, dest.x, dest.y, strToRender.c_str(), nullptr); //TODO: I think this function had some sort of innate scaling built-in, which this ignores?

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	glEnable(GL_TEXTURE_2D); //Needed for the rest of the game...

}

static std::unordered_map<std::string, Text*> hashed_text;
static const int TEXT_BUDGET = 1000; //TODO: We might not need to cache at all anymore with the new method...maybe instead just have the Font class always render on-demand?

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

	return text;
}

void Text::dumpCache() {
	for (auto text : hashed_text) {
		delete text.second;
	}
	hashed_text.clear();
}