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
	//fontstash = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);
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

int Font::sizeText(const char* str, int* out_w, int* out_h, int override_pointsize) const {
	if (out_w) {
		*out_w = 0;
	}
	if (out_h) {
		*out_h = 0;
	}
	if (fontstash && str) {
		if (override_pointsize == 0)
		{
			fonsSetSize(fontstash, pointSize);
		}
		else
		{
			fonsSetSize(fontstash, override_pointsize);
		}
		fonsSetAlign(fontstash, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
		float textBounds[4];
		float width = fonsTextBounds(fontstash, 0, 0, str, nullptr, textBounds);
		if (out_w) {
			*out_w = static_cast<int>(textBounds[2]) - static_cast<int>(textBounds[0]) + (outlineSize * 2);
			//*out_w = static_cast<int>(width) + outlineSize * 2; //TODO: This, or text bounds [2] - [0]?
		}
		if (out_h) {
			*out_h = static_cast<int>(textBounds[3]) - static_cast<int>(textBounds[1]) + (outlineSize * 2);
			//printf("[%s] [3] = %f, [1] %f, out_h = %d\n", str, textBounds[3], textBounds[1], *out_h);
		}
		return 0;
	} else {
		return -1;
	}
}

int Font::height(int override_pointsize) const {
	if (fontstash) {
		if (override_pointsize == 0)
		{
			fonsSetSize(fontstash, pointSize);
		}
		else
		{
			fonsSetSize(fontstash, override_pointsize);
		}
		fonsSetAlign(fontstash, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
		float height;
		fonsVertMetrics(fontstash, nullptr, nullptr, &height);
		return static_cast<int>(height);
	} else {
		return 0;
	}
}
void Font::drawText(std::string str, int x, int y, int override_pointsize) {
	drawTextColor(str, x, y, 0xffffffff, override_pointsize);
}

void Font::drawTextColor(std::string str, int x, int y, const Uint32& color, int override_pointsize) {
	fonsClearState(fontstash);
	fonsSetFont(fontstash, font_style_normal);
	if (override_pointsize == 0)
	{
		fonsSetSize(fontstash, pointSize);
	}
	else
	{
		fonsSetSize(fontstash, override_pointsize);
	}
	//fonsSetSize(fontstash, 124.0f);
	fonsSetColor(fontstash, color);
	fonsSetAlign(fontstash, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	//glOrtho(0, xres, 0, yres, -1, 1);
	glOrtho(0, xres / UI_SCALE_EXPERIMENT, yres / UI_SCALE_EXPERIMENT, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	fonsDrawText(fontstash, x, y, str.c_str(), nullptr);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	glEnable(GL_TEXTURE_2D); //Needed for the rest of the game...
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