// Text.cpp

#include <assert.h>

#include "../main.hpp"
#include "../draw.hpp"
#include "Text.hpp"
#include "Font.hpp"
#include "Frame.hpp"
#include "Image.hpp"

constexpr int resolution_factor = 1;

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
        GL_CHECK_ERR(glDeleteTextures(1, &texid));
		texid = 0;
	}
}

size_t getNumTextLines(std::string& str)
{
	int numLines = 1;
	size_t newlines = std::count(str.begin(), str.end(), '\n');

	return numLines + newlines;
}

#ifndef EDITOR
static ConsoleVariable<bool> cvar_text_render_addspace("/text_render_addspace", true);
static ConsoleVariable<bool> cvar_text_delay_dumpcache("/text_delay_dumpcache", false);
#endif

void Text::render() {
	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if (texid) {
        GL_CHECK_ERR(glDeleteTextures(1, &texid));
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
				textColor = (uint32_t)strtoul(rest.substr(0, index).c_str(), nullptr, 16);
				rest = rest.substr(index + 1);
				if ((index = rest.find(fontBreak)) != std::string::npos) {
					outlineColor = (uint32_t)strtoul(rest.substr(0, index).c_str(), nullptr, 16);
				} else {
					outlineColor = (uint32_t)strtoul(rest.c_str(), nullptr, 16);
				}
			} else {
				textColor = (uint32_t)strtoul(rest.c_str(), nullptr, 16);
			}
		} else {
			fontName = rest;
		}
	} else {
		strToRender = rest;
	}

	Font* font = Font::get(fontName.c_str());
	if (!font) {
		assert(0 && "Text tried to render, but font failed to load");
		return;
	}
	TTF_Font* ttf = font->getTTF();

	bool addedSpace = false;

#ifdef NINTENDO
	// fixes weird crash in SDL_ttf when string length < 2
	std::string spaces;
	int num_spaces_needed = std::max(0, 2 - (int)strToRender.size());
	while (num_spaces_needed) {
		spaces.append(" ");
		--num_spaces_needed;
	}
	int spaces_width = 0;
	if (spaces.size()) {
		TTF_SizeUTF8(ttf, spaces.c_str(), &spaces_width, nullptr);
		spaces_width += spaces.size();
		strToRender.append(spaces);
		if (spaces.size() == 2) {
			addedSpace = true;
		}
	}
#else
	const int spaces_width = 0;
#ifndef EDITOR
	if ( *cvar_text_render_addspace ) {
		if ( strToRender == "" ) {
			addedSpace = true;
			strToRender += ' ';
		}
	}
#endif
#endif

	SDL_Color colorText;
	getColor(textColor, &colorText.r, &colorText.g, &colorText.b, &colorText.a);

	SDL_Color colorOutline;
	getColor(outlineColor, &colorOutline.r, &colorOutline.g, &colorOutline.b, &colorOutline.a);

	int outlineSize = font->getOutline();
	if ( outlineSize > 0 ) {
		TTF_SetFontOutline(ttf, outlineSize);
		SDL_ClearError();
		surf = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorOutline);
		if ( !surf )
		{
			printlog("[TTF]: Error: surf = TTF_RenderUTF8_Blended: %s", TTF_GetError());
		}
		TTF_SetFontOutline(ttf, 0);
		SDL_ClearError();
		SDL_Surface* text = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorText);
		if ( !text )
		{
			printlog("[TTF]: Error: text = TTF_RenderUTF8_Blended: %s", TTF_GetError());
		}
		SDL_Rect rect;
		rect.x = outlineSize; rect.y = outlineSize;
		SDL_BlitSurface(text, NULL, surf, &rect);
		SDL_FreeSurface(text);
	}
	else {
		TTF_SetFontOutline(ttf, 0);
		surf = TTF_RenderUTF8_Blended(ttf, strToRender.c_str(), colorText);
	}

	if (!surf) {
		num_text_lines = 1;
	    width = 4;
	    height = font->height(true);
	    return;
	}

	if ( addedSpace )
	{
		width = 4;
		height = surf->h;
	}
	else
	{
		width = std::max(0, surf->w - spaces_width);
		height = surf->h;
#ifndef WINDOWS
		width -= outlineSize;
#endif
	}

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
				bool doFillRow = false;
				for ( int y = 0; y < surf->h; y++ )
				{
					Uint32 pix = getPixel(surf, x, y);
					Uint8 r, g, b, a;
					getColor(pix, &r, &g, &b, &a);
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
        
        // create GL texture object
        GL_CHECK_ERR(glGenTextures(1, &texid));
        GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        
        // check whether we can fit the surf data into the texture
        GLint maxSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
        if (surf->w <= GL_MAX_TEXTURE_SIZE && surf->h <= GL_MAX_TEXTURE_SIZE) {
            // we can fit the texture
            GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h,
                0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels));
        } else {
            // we cannot fit the texture
            GL_CHECK_ERR(glDeleteTextures(1, &texid));
            texid = 0;
        }
        
		SDL_UnlockSurface(surf);
	}

	num_text_lines = countNumTextLines();
}

void Text::draw(const SDL_Rect src, const SDL_Rect dest, const SDL_Rect viewport) const {
	drawColor(src, dest, viewport, 0xffffffff);
}

void Text::drawColor(const SDL_Rect _src, const SDL_Rect _dest, const SDL_Rect viewport, const Uint32& color) const {
	if (!surf || !texid) {
	    return;
	}

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
    
    Image::draw(texid, surf->w, surf->h, &src, dest, viewport, color);
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
static const size_t TEXT_BUDGET = 1 * 1024 * 1024 * 128; // in bytes
static size_t TEXT_VOLUME = 0; // in bytes
static bool bRequireTextDump = false;

static inline void uint32tox(uint32_t value, char* out) {
	for (int i = 28; i >= 0; i -= 4) {
		uint8_t shift = (value >> i) & 0x0F;
		*out = (shift < 10u) ? shift + '0' : shift - 10 + 'a';
		++out;
	}
}

std::pair<size_t, const char*> Text::hash(const char* str, const char* font, Uint32 textColor, Uint32 outlineColor) {
	if (!str) {
		return std::make_pair((size_t)0, (const char*)nullptr);
	}
	if (font == nullptr || font[0] == '\0') {
		font = Font::defaultFont;
	}

	// NOTE the following static buffer makes this function NOT thread safe!!
	// better not try to render more than 64kb of text...
	static char textAndFont[65536] = { '\0' };
	const size_t len0 = strlen(str);
	const size_t len1 = strlen(font);
	const size_t totalLen =
		len0 + sizeof(fontBreak) +
		len1 + sizeof(fontBreak) +
		10 + sizeof(fontBreak) +
		10 + sizeof(fontBreak) +
		sizeof('\0');
	if (totalLen > sizeof(textAndFont)) {
		assert(0 && "Trying to render > 64kb of ttf text");
		return std::make_pair((size_t)0, (const char*)nullptr);
	}

	// build format string
	char* ptr = textAndFont;
	memcpy(ptr, str, len0); ptr += len0;
	*ptr = fontBreak; ++ptr;
	memcpy(ptr, font, len1); ptr += len1;
	*ptr = fontBreak; ++ptr;
	*ptr = '0'; ++ptr;
	*ptr = 'x'; ++ptr;
	uint32tox(textColor, ptr); ptr += 8;
	*ptr = fontBreak; ++ptr;
	*ptr = '0'; ++ptr;
	*ptr = 'x'; ++ptr;
	uint32tox(outlineColor, ptr); ptr += 8;
	*ptr = fontBreak; ++ptr;
	*ptr = '\0'; ++ptr;

	// compute hash
	const auto& hash = hashed_text.hash_function();
	return std::make_pair(hash(textAndFont), textAndFont);
}

Text* Text::get(size_t hash, const char* key) {
	if (!key) {
		return nullptr;
	}

	// search for text using precomputed hash
	auto& map = hashed_text;
	auto bc = map.bucket_count();
	if (bc) {
		const auto& hash_fn = map.hash_function();
		auto chash = !(bc & (bc - 1)) ? hash & (bc - 1) :
			(hash < bc ? hash : hash % bc);
		for (auto it = map.begin(chash); it != map.end(chash); ++it) {
			if (hash == hash_fn(it->first) && it->first == key) {
				return it->second;
			}
		}
	}

	// check if cache is full
	if (TEXT_VOLUME > TEXT_BUDGET) {
#ifdef EDITOR
		dumpCache();
#else
		if (*cvar_text_delay_dumpcache)
		{
			bRequireTextDump = true;
		}
		else
		{
			dumpCache();
		}
#endif
	}

	// text not found, add it to cache
	auto text = new Text(key);
	hashed_text.insert(std::make_pair(key, text));
	TEXT_VOLUME += sizeof(Text) + sizeof(SDL_Surface); // header data
	TEXT_VOLUME += text->getWidth() * text->getHeight() * 4; // 32-bpp pixel data
	TEXT_VOLUME += text->wordsToHighlight.size() * sizeof(int) * sizeof(Uint32); // word highlight map
	TEXT_VOLUME += 1024; // 1-kB buffer

	return text;
}

Text* Text::get(const char* str, const char* font, Uint32 textColor, Uint32 outlineColor) {
	auto h = hash(str, font, textColor, outlineColor);
	return get(h.first, h.second);
}

void Text::dumpCache() {
	printlog("[Text Cache]: dumping...");
	for (auto text : hashed_text) {
		//printlog("%s", text.second->getName());
		delete text.second;
	}
	hashed_text.clear();
	TEXT_VOLUME = 0;
	bRequireTextDump = false;
}

void Text::dumpCacheInMainLoop()
{
	if ( bRequireTextDump )
	{
		dumpCache();
	}
}

#ifndef EDITOR
#include "../net.hpp"
#include "../interface/consolecommand.hpp"
static ConsoleCommand size("/text_cache_size", "measure text cache",
    [](int argc, const char** argv){
    messagePlayer(clientnum, MESSAGE_MISC, "cache size is: %llu bytes (%llu kB)", TEXT_VOLUME, TEXT_VOLUME / 1024);
    });
static ConsoleCommand dump("/text_cache_dump", "dump text cache",
    [](int argc, const char** argv){
    Text::dumpCache();
    messagePlayer(clientnum, MESSAGE_MISC, "dumped cache");
    });
#endif
