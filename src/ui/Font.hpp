//! @file Font.hpp

#pragma once

#include "../main.hpp"

class Font {
public:
	Font() = default;
	Font(const char* _name);
	Font(const Font&) = delete;
	Font(Font&&) = delete;
	virtual ~Font();

	Font& operator=(const Font&) = delete;
	Font& operator=(Font&&) = delete;

	//! built-in font
	static const char* defaultFont;

	//! get ttf font
	TTF_Font* getTTF() { return font; }

	//! get the size of the given text string in pixels
	//! @param str the utf-8 string to get the size of
	//! @param out_w the integer to hold the width
	//! @param out_h the integer to hold the height
	//! @return 0 on success, non-zero on error
	int sizeText(const char* str, int* out_w, int* out_h) const;

	//! get the height of the font
	//! @return the font height in pixels
	int height() const;

private:
	TTF_Font* font = nullptr;
	int pointSize = 16;
};