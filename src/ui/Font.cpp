// Font.cpp

#include "../main.hpp"
#include "Font.hpp"

const char* Font::defaultFont = "fonts/mono.ttf#16";

Font::Font(const char* _name) {
	name = _name;
	Uint32 index = name.find('#');
	std::string path;
	if (index != std::string::npos) {
		path = name.substr(0, index);
		pointSize = std::stoi(name.substr(index + 1, name.length()));
	} else {
		path = name;
	}
	if ((font = TTF_OpenFont(path.c_str(), pointSize)) == NULL) {
		printlog("failed to load '%s': %s", path.c_str(), TTF_GetError());
		return;
	}
	TTF_SetFontHinting(font, TTF_HINTING_MONO);
	TTF_SetFontKerning(font, 0);
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
		return TTF_SizeUTF8(font, str, out_w, out_h);
	} else {
		return -1;
	}
}

int Font::height() const {
	if (font) {
		return TTF_FontHeight(font);
	} else {
		return 0;
	}
}