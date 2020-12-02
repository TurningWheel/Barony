// Font.cpp

#include "Main.hpp"
#include "Engine.hpp"
#include "Font.hpp"

const char* Font::defaultFont = "fonts/mono.ttf#16";

Font::Font(const char* _name) : Asset(_name) {
	Uint32 index = name.find('#');
	if (index != String::npos) {
		path = mainEngine->buildPath(name.substr(0, index).get());
		pointSize = name.substr(index + 1, name.length()).toInt();
	} else {
		path = mainEngine->buildPath(name.get());
	}
	if ((font = TTF_OpenFont(path.get(), pointSize)) == NULL) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to load '%s': %s", path.get(), TTF_GetError());
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