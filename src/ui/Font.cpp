// Font.cpp

#include <fstream>
using std::ifstream;

#include "../main.hpp"
#include "../external/sdl_stb/sdlStbFont.h"
#include "Font.hpp"

#ifdef NINTENDO
const char* Font::defaultFont = "rom://lang/en.ttf#24";
#else // NINTENDO
const char* Font::defaultFont = "lang/en.ttf#24";
#endif // NINTENDO

static size_t getFileSize(string filepath, ifstream& file)
{
	file.seekg(0, std::ios::end);
	size_t filesize = file.tellg();
	file.seekg(0, std::ios::beg);
	return filesize;
}

Font::FontFile* Font::loadFontFile(string filepath)
{
	ifstream file(filepath, std::ios::binary);
	if (!file)
	{
		printf("ERROR: Failed to load font file \"%s\"\n", filepath.c_str());
		return nullptr;
	}

	size_t filesize = getFileSize(filepath, file);
	if (filesize == 0)
	{
		printf("ERROR: Skipping loading empty font file \"%s\"!\n");
		return nullptr;
	}

	uint8_t* buffer = new uint8_t[filesize];
	if (!file.read(reinterpret_cast<char*>(buffer), filesize))
	{
		printf("ERROR: Failed to read file \"%s\" into buffer!\n");
		return nullptr;
	}

	return new FontFile(filesize, buffer);
}

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

	fontFile = loadFontFile(path);
	if (!fontFile)
	{
		printlog("failed to load font file '%s'", path.c_str());
		return;
	}
	// if ((font = TTF_OpenFont(path.c_str(), pointSize)) == NULL) { //TODO: STB TTF init hookup here.
	// 	printlog("failed to load '%s': %s", path.c_str(), TTF_GetError());
	// 	return;
	// }
	// TTF_SetFontHinting(font, TTF_HINTING_NORMAL);
	// TTF_SetFontKerning(font, 0);
	fontcache = new sdl_stb_font_cache();
	fontcache->faceSize = pointSize;
	fontcache->tabWidthInSpaces = 4;
	fontcache->loadFont(reinterpret_cast<char*>(fontFile->buffer));
	fontcache->bindRenderer(renderer);
}

Font::~Font() {
	// if (font) {
	// 	TTF_CloseFont(font);
	// }
	if (fontcache)
	{
		delete fontcache;
	}
	if (fontFile)
	{
		delete fontFile;
	}
}

int Font::sizeText(const char* str, int* out_w, int* out_h) const {
	if (out_w) {
		*out_w = 0;
	}
	if (out_h) {
		*out_h = 0;
	}
	if (fontcache && str) {
		//int result = TTF_SizeUTF8(font, str, out_w, out_h);
		int w, h;
		fontcache->getTextSize(w, h, str); //TODO: If only height is requested, faster to use h = fontcache->getTextHeight(str)...
		if (out_w) {
			*out_w = w + outlineSize * 2;
		}
		if (out_h) {
			*out_h = h + outlineSize * 2;
		}
		return 0;
	} else {
		return -1;
	}
}

int Font::height() const {
	if (fontcache) {
		//return TTF_FontHeight(font) + outlineSize * 2;
		return fontcache->getTextHeight(string("A")) + outlineSize * 2; //TODO: DIRTY HACK!! Add a function to let us do this without passing in any strings...
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