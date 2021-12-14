// Text.cpp

#include <assert.h>

#include "../main.hpp"
#include "../draw.hpp"
#include "Text.hpp"
#include "Font.hpp"

GLuint Text::vao = 0;
GLuint Text::vbo[BUFFER_TYPE_LENGTH] = { 0 };
constexpr int resolution_factor = 1;

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
	render();
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

size_t getNumTextLines(std::string& str)
{
	int numLines = 1;
	size_t newlines = std::count(str.begin(), str.end(), '\n');

	return numLines + newlines;
}

void Text::render() {
	if ( surf ) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if ( texid ) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}

	std::string strToRender;
	std::string fontName = Font::defaultFont;
	Uint32 textColor = makeColor(255, 255, 255, 255);
	Uint32 outlineColor = makeColor(0, 0, 0, 255);

	size_t index;
	std::string rest = name;
	if ((index = rest.find(fontBreak)) != std::string::npos) {
		strToRender = rest.substr(0, index);
		rest = rest.substr(index + 1);
		if ((index = rest.find(fontBreak)) != std::string::npos) {
			fontName = rest.substr(0, index);
			rest = rest.substr(index + 1);
			if ((index = rest.find(fontBreak)) != std::string::npos) {
				textColor = strtoul(rest.substr(0, index).c_str(), nullptr, 16);
				rest = rest.substr(index + 1);
				if ((index = rest.find(fontBreak)) != std::string::npos) {
					outlineColor = strtoul(rest.substr(0, index).c_str(), nullptr, 16);
				} else {
					outlineColor = strtoul(rest.c_str(), nullptr, 16);
				}
			} else {
				textColor = strtoul(rest.c_str(), nullptr, 16);
			}
		} else {
			fontName = rest;
		}
	} else {
		strToRender = rest;
	}

#ifdef NINTENDO
	// fixes weird crash in SDL_ttf when string length < 2
	while ( strToRender.size() < 2 ) {
		strToRender.append(" ");
	}
#else
	// fixes nullptr SDL surface when string is empty.
	if ( strToRender.size() < 1 ) {
		strToRender.append(" ");
	}
#endif

	Font* font = Font::get(fontName.c_str());
	if (!font) {
		assert(0 && "Text tried to render, but font failed to load");
		return;
	}
	TTF_Font* ttf = font->getTTF();

	SDL_Color colorText;
	SDL_GetRGBA(textColor, mainsurface->format,
		&colorText.r, &colorText.g, &colorText.b, &colorText.a);

	SDL_Color colorOutline;
	SDL_GetRGBA(outlineColor, mainsurface->format,
		&colorOutline.r, &colorOutline.g, &colorOutline.b, &colorOutline.a);

	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}

	int outlineSize = font->getOutline();
	if ( outlineSize > 0 ) {
		TTF_SetFontOutline(ttf, outlineSize);
		surf = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorOutline);
		TTF_SetFontOutline(ttf, 0);
		SDL_Surface* text = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorText);
		SDL_Rect rect;
		rect.x = outlineSize; rect.y = outlineSize;
		SDL_BlitSurface(text, NULL, surf, &rect);
		SDL_FreeSurface(text);
	}
	else {
		TTF_SetFontOutline(ttf, 0);
		surf = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorText);
	}
	assert(surf);
	if ( texid == 0 ) {
		glGenTextures(1, &texid);
	}

	width = surf->w;
	height = surf->h;

	// Fields break multi-lines anyway, and we're not using TTF_RenderUTF8_Blended_Wrapped()
	// So calculating width/height ourselves is redundant and buggy (it doesn't factor trailing spaces)

	/*width = 0;
	height = 0;

	int numLines = getNumTextLines(strToRender);
	height = surf->h * numLines + std::max(0, numLines - 1) * (2 + 2 * outlineSize);

	int scan = surf->pitch / surf->format->BytesPerPixel;
	for ( int y = 0; y < surf->h; ++y ) {
		for ( int x = 0; x < surf->w; ++x ) {
			// check upper byte (alpha) data. remaining 3 bytes are always 0x00FFFFFF
			if ( (0xFF000000 & ((Uint32 *)surf->pixels)[x + y * scan]) != 0 ) {
				width = std::max(width, x);
			}
		}
	}
	++width;*/

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, width * resolution_factor, height * resolution_factor,
		32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Rect dest{ 0, 0, width * resolution_factor, height * resolution_factor };
	SDL_Rect src{ 0, 0, width, height };
	if (resolution_factor > 1) {
		SDL_BlitScaled(surf, &src, newSurf, &dest);
	} else {
		SDL_BlitSurface(surf, &src, newSurf, &dest);
	}
	SDL_FreeSurface(surf);
	surf = newSurf;

	// load the new surface as a GL texture
	if ( surf )
	{
		SDL_LockSurface(surf);

		/*Uint32 fillColor1 = makeColor(0, 255, 0, 255);
		Uint32 fillColor2 = makeColor(255, 0, 0, 255);
		wordsToHighlight[2] = fillColor1;
		wordsToHighlight[4] = fillColor2;*/
		if ( !wordsToHighlight.empty() )
		{
			int currentWord = 0;
			bool checkForEmptyRow = true;
			bool currentWordHasColor = (wordsToHighlight.find(0) != wordsToHighlight.end());
			for ( int x = 0; x < surf->w; x++ )
			{
				bool isEmptyRow = true && checkForEmptyRow;
				bool foundTextColorThisRow = false;
				bool doFillRow = false;
				for ( int y = 0; y < surf->h; y++ )
				{
					Uint32 pix = getPixel(surf, x, y);
					Uint8 r, g, b, a;
					SDL_GetRGBA(pix, surf->format, &r, &g, &b, &a);
					if ( r == colorText.r && g == colorText.g && b == colorText.b && a == colorText.a )
					{
						if ( !doFillRow )
						{
							checkForEmptyRow = true;
							doFillRow = true;
							--y;
							continue;
						}
						else if ( doFillRow )
						{
							if ( currentWordHasColor )
							{
								putPixel(surf, x, y, wordsToHighlight[currentWord]);
							}
						}
					}
					if ( a != 0 )
					{
						isEmptyRow = false;
					}
				}
				if ( isEmptyRow )
				{
					checkForEmptyRow = false;
					++currentWord;
					currentWordHasColor = (wordsToHighlight.find(currentWord) != wordsToHighlight.end());
				}
			}
		}

		glBindTexture(GL_TEXTURE_2D, texid);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		SDL_UnlockSurface(surf);
		rendered = true;
	}
	else
	{
		rendered = false;
	}

	num_text_lines = countNumTextLines();
}

void Text::draw(const SDL_Rect src, const SDL_Rect dest, const SDL_Rect viewport) const {
	drawColor(src, dest, viewport, 0xffffffff);
}

void Text::drawColor(const SDL_Rect _src, const SDL_Rect _dest, const SDL_Rect viewport, const Uint32& color) const {
	assert(rendered && surf);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
	glLoadIdentity();
	glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	auto src = _src;
	auto dest = _dest;

	if (resolution_factor != 1) {
		src.x *= resolution_factor;
		src.y *= resolution_factor;
		src.w *= resolution_factor;
		src.h *= resolution_factor;
	}

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
	glVertex2f(dest.x, viewport.h - dest.y);
	glTexCoord2f(1.0 * ((real_t)src.x / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	glVertex2f(dest.x, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * (((real_t)src.y + src.h) / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src.x + src.w) / surf->w), 1.0 * ((real_t)src.y / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

int Text::countNumTextLines() const {
	int numLines = 1;
	for (auto c : name) {
		switch (c) {
		case fontBreak: return numLines;
		case '\0': return numLines;
		case '\n': ++numLines; break;
		default: continue;
		}
	}
	return numLines;
}

static std::unordered_map<std::string, Text*> hashed_text;
static const int TEXT_BUDGET = 1000;

Text* Text::get(const char* str, const char* font, Uint32 textColor, Uint32 outlineColor) {
	if (!str) {
		return nullptr;
	}
	if (font == nullptr || font[0] == '\0') {
		font = Font::defaultFont;
	}
	size_t len0 = strlen(str);
	size_t len1 = strlen(font);
	char textAndFont[65536] = { '\0' }; // better not try to render more than 64kb of text...
	size_t totalLen =
		len0 + sizeof(fontBreak) +
		len1 + sizeof(fontBreak) +
		10 + sizeof(fontBreak) +
		10 + sizeof(fontBreak) +
		sizeof('\0');
	if (totalLen > sizeof(textAndFont)) {
		assert(0 && "Trying to render > 64kb of ttf text");
		return nullptr;
	}
	snprintf(textAndFont, sizeof(textAndFont), "%s%c%s%c%#010x%c%#010x%c",
		str, Text::fontBreak,
		font, Text::fontBreak,
		textColor, Text::fontBreak,
		outlineColor, Text::fontBreak);

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
