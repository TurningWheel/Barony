//! @file Font.hpp

#pragma once

#include "../main.hpp"

struct FONScontext;
class Font {
private:
	Font() = default;
	Font(const char* _name);
	Font(const Font&) = delete;
	Font(Font&&) = delete;
	~Font();

	Font& operator=(const Font&) = delete;
	Font& operator=(Font&&) = delete;

public:
	//! built-in font
	static const char* defaultFont;

	const char*		getName() const { return name.c_str(); }
	FONScontext* 	getFontstash() { return fontstash; }
	int				getFontId() { return font_style_normal; } //TODO: Support other styles and stuff? Maybe like "get font ID for regular. Get font ID for italics." Etc.
	int				getPointSize() { return pointSize; } //TODO: All of these extra things we're exposing tells me that maybe Font should render instead (and maybe to a buffer which we cache in Text.hpp...? Or maybe that last surface/texture caching is a waste of memory.)
	int				getOutline() { return outlineSize; }

	//! get the size of the given text string in pixels
	//! @param str the utf-8 string to get the size of
	//! @param out_w the integer to hold the width
	//! @param out_h the integer to hold the height
	//! @return 0 on success, non-zero on error
	int sizeText(const char* str, int* out_w, int* out_h) const;

	//! get the height of the font
	//! @return the font height in pixels
	int height() const;

	//! get a Font object from the engine
	//! @param name The Font name
	//! @return the Font or nullptr if it could not be retrieved
	static Font* get(const char* name);

	//! dump engine's font cache
	static void dumpCache();

private:
	std::string name;
	FONScontext* fontstash = nullptr;
	int font_style_normal = 0;
	int pointSize = 16; //TODO: Split pointsize from the filename and let us set this on the fly?
	int outlineSize = 0;
};
