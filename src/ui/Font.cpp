// Font.cpp

#include <algorithm> //For std::count
#include <regex> //For std::regex_replace

#include "../main.hpp"
#include "../cppfuncs.hpp"
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

int Font::sizeLine(std::string str, int* out_w, int* out_h, int override_pointsize) const { //TODO: Make this account for tabs and newlines.
	if (out_w) {
		*out_w = 0;
	}
	if (out_h) {
		*out_h = 0;
	}
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
		float textBounds[4];
		float width = fonsTextBounds(fontstash, 0, 0, str.c_str(), nullptr, textBounds);
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

int Font::sizeText(std::string str, int* out_w, int* out_h, int override_pointsize) const { //TODO: Search for all occurrences and change them to pass in a C++ string, not .c_str()
	str = std::regex_replace(str, std::regex("\t"), TAB_REPLACE_STRING); //1. Nuke all tabs, replacing them with their space representation.
	std::vector<std::string> lines = splitStringByDelimeter(str, '\n'); //TODO: 2. Split string on newlines.

	int biggestWidth = 0; //For width, find the longest line in the paragraph.
	int cumulativeHeight = 0; //For height, just keep the running sum... //TODO: Will this return a different value than height()?
	for (std::string line : lines) {
		int lineWidth, lineHeight;
		int returnCode = sizeLine(line, &lineWidth, &lineHeight, override_pointsize);
		if (returnCode != 0)
		{
			return returnCode;
		}
		biggestWidth = std::max(biggestWidth, lineWidth);
		cumulativeHeight += lineHeight;
	}

	if (out_w) {
		*out_w = biggestWidth;
	}
	if (out_h) {
		*out_h = cumulativeHeight;
	}

	return 0;
}

int Font::textWidth(std::string str, int override_pointsize) const {
	int width;

	sizeText(str, &width, nullptr, override_pointsize);

	return width;
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

int Font::textHeight(std::string str, int override_pointsize) const {
	int numNewLines = std::count(str.begin(), str.end(), '\n');

	return numNewLines * height(override_pointsize); //TODO: Do we need to account for a gap between every line??
}

void Font::drawText(std::string str, int x, int y, int override_pointsize) {
	drawTextColor(str, x, y, 0xffffffff, override_pointsize);
}

void Font::drawTextColor(std::string str, int x, int y, const Uint32& color, int override_pointsize) { //TODO: Make this deal with tabs and newlines...
	//Prepare Fontstash for rendering our text.
	if (!fontstash)
	{
		return;
	}
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
	fonsSetColor(fontstash, color);
	fonsSetAlign(fontstash, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);

	//Prepare OpenGL for rendering our text.
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

	//TODO: Do the magic here.
	str = std::regex_replace(str, std::regex("\t"), TAB_REPLACE_STRING); //Nuke all tabs, replacing them with their space representation. //TODO: True tab behavior, where "\t" and " \t" will generate the exact same strings?? //TODO: 2: Refactor out these two calls as "string preprocessing to handle newlines and tabs"? Since, it is used in a couple places...
	std::vector<std::string> lines = splitStringByDelimeter(str, '\n');
	for (std::string line : lines) {
		drawLineColor(line, x, y, color, override_pointsize);
		y += height(override_pointsize);
	}

	//Reset OpenGL state to be in a good state for the rest of the game.
	glBindTexture(GL_TEXTURE_2D, 0); // unbind texture
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glEnable(GL_TEXTURE_2D); //Needed for the rest of the game...
}

void Font::drawLineColor(std::string str, int x, int y, const Uint32& color, int override_pointsize) {
	fonsDrawText(fontstash, x, y, str.c_str(), nullptr);
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