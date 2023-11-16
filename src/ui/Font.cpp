// Font.cpp

#include "../main.hpp"
#include "Font.hpp"

const char* Font::defaultFont = "lang/en.ttf#24";

Font::Font(const char* _name) {
	name = _name;
	size_t index = name.find('#');
	std::string path;
	if (index != std::string::npos) {
		size_t nindex = name.find('#', index + 1);
		path = name.substr(0, index);
		if (nindex != std::string::npos) {
			pointSize = std::stoi(name.substr(index + 1, nindex));
			outlineSize = std::stoi(name.substr(nindex + 1, name.length()));
		} else {
			pointSize = std::stoi(name.substr(index + 1, name.length()));
		}
	} else {
		path = name;
	}
	if ( PHYSFS_getRealDir(path.c_str()) )
	{
		std::string realPath = PHYSFS_getRealDir(path.c_str());
		path.insert(0, PHYSFS_getDirSeparator());
		path.insert(0, realPath);
	}
	else {
#ifdef NINTENDO
		path.insert(0, PHYSFS_getDirSeparator());
		path.insert(0, BASE_DATA_DIR);
#endif
	}
	if ((font = TTF_OpenFont(path.c_str(), pointSize)) == NULL) {
		printlog("failed to load '%s': %s", path.c_str(), TTF_GetError());
		return;
	}
	//TTF_SetFontHinting(font, TTF_HINTING_MONO);
	TTF_SetFontKerning(font, 1);
}

Font::~Font() {
	if (font) {
		TTF_CloseFont(font);
	}
}

int Font::sizeText(const char* str, int* out_w, int* out_h) const {
	if (out_w) {
		*out_w = 0;
	}
	if (out_h) {
		*out_h = 0;
	}
	if (font && str) {
		int result = TTF_SizeUTF8(font, str, out_w, out_h);
		if (out_w) {
			*out_w += outlineSize * 2;
		}
		if (out_h) {
			*out_h += outlineSize * 2;
		}
		return result;
	} else {
		return -1;
	}
}

int Font::height(bool withOutline) const {
	if (font) {
		if (withOutline) {
			return TTF_FontHeight(font) + outlineSize * 2;
		} else {
			return TTF_FontHeight(font);
		}
	} else {
		return 0;
	}
}

static std::unordered_map<std::string, Font*> hashed_fonts;
static const int FONT_BUDGET = 50;

Font* Font::get(const char* name) {
	if (!name) {
		return nullptr;
	}
	Font* font = nullptr;
	auto search = hashed_fonts.find(name);
	if (search == hashed_fonts.end()) {
        // NOTE: We have no idea how to size this data because TTF_Font is opaque!!
		if (hashed_fonts.size() > FONT_BUDGET) {
			dumpCache();
		}
		font = new Font(name);
		hashed_fonts.insert(std::make_pair(name, font));
	} else {
		font = search->second;
	}
	return font;
}

void Font::dumpCache() {
	for (auto font : hashed_fonts) {
		delete font.second;
	}
	hashed_fonts.clear();
}

#ifndef EDITOR
#include "../net.hpp"
#include "../interface/consolecommand.hpp"
static ConsoleCommand size("/fonts_cache_size", "measure font cache",
    [](int argc, const char** argv){
    messagePlayer(clientnum, MESSAGE_MISC, "cache size is: %d fonts", (int)hashed_fonts.size());
    });
static ConsoleCommand dump("/fonts_cache_dump", "dump font cache",
    [](int argc, const char** argv){
    Font::dumpCache();
    messagePlayer(clientnum, MESSAGE_MISC, "dumped cache");
    });
#endif