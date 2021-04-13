// Font.cpp

#include "../main.hpp"
#include "Font.hpp"

#include "../external/stb/stb_truetype.h"
#include "../external/fontstash/fontstash.h"
#include "../external/fontstash/glfontstash.h"

#ifdef NINTENDO
const char* Font::defaultFont = "rom://lang/en.ttf#24";
#else // NINTENDO
const char* Font::defaultFont = "lang/en.ttf#24";
#endif // NINTENDO

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
	printf("Creating new font: %s\n", name.c_str());
	fontstash = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
	font_style_normal = fonsAddFont(fontstash, "sans", path.c_str());
	if (font_style_normal == FONS_INVALID)
	{
		printlog("failed to load font '%s'", path.c_str());
	}
}

Font::~Font() {
	if (fontstash) {
		glfonsDelete(fontstash);
	}
}

int Font::sizeText(const char* str, int* out_w, int* out_h) const {
	if (out_w) {
		*out_w = 0;
	}
	if (out_h) {
		*out_h = 0;
	}
	if (fontstash && str) {
		float textBounds[4];
		float width = fonsTextBounds(fontstash, 0, 0, str, nullptr, textBounds);
		//TODO: Make this work!
		if (out_w) {
			*out_w = static_cast<int>(textBounds[2]) - static_cast<int>(textBounds[0]) + outlineSize * 2;
			//*out_w = static_cast<int>(width) + outlineSize * 2; //TODO: This, or text bounds [2] - [0]?
		}
		if (out_h) {
			*out_h = static_cast<int>(textBounds[3]) - static_cast<int>(textBounds[1]) + outlineSize * 2;
			//printf("[%s] [3] = %f, [1] %f, out_h = %d\n", str, textBounds[3], textBounds[1], *out_h);
		}
		return 0;
	} else {
		return -1;
	}
}

int Font::height() const {
	if (fontstash) {
		//TODO: Make this work!
		float height;
		fonsVertMetrics(fontstash, nullptr, nullptr, &height);
		return static_cast<int>(height);
	} else {
		return 0;
	}
}

static std::unordered_map<std::string, Font*> hashed_fonts;
static const int FONT_BUDGET = 100;

Font* Font::get(const char* name) {
	if (!name) {
		return nullptr;
	}
	Font* font = nullptr;
	auto search = hashed_fonts.find(name);
	if (search == hashed_fonts.end()) {
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