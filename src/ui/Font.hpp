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

	//! helper function for sizeText(), except operates on just one line.
	int sizeLine(std::string str, int* out_w, int* out_h, int override_pointsize = 0) const;

	//! helper function for drawTextColor(), except operates on just one line.
	void drawLineColor(std::string str, int x, int y, const Uint32& color, int override_pointsize);

public:
	//! built-in font
	static const char* defaultFont;

	//! tab-width in spaces
	static const int TAB_WIDTH = 4;
	//! The following is basically a prerendered tab replacement...must be maintained to match tab_width. //TODO: Why even have TAB_WIDTH, then?
	static constexpr char* TAB_REPLACE_STRING = "    ";

	const char*		getName() const { return name.c_str(); }
	FONScontext* 	getFontstash() { return fontstash; }
	int				getFontId() { return font_style_normal; } //TODO: Support other styles and stuff? Maybe like "get font ID for regular. Get font ID for italics." Etc.
	int				getPointSize() { return pointSize; } //TODO: All of these extra things we're exposing tells me that maybe Font should render instead (and maybe to a buffer which we cache in Text.hpp...? Or maybe that last surface/texture caching is a waste of memory.)
	int				getOutline() { return outlineSize; }

	//! get the size of the given text string in pixels
	//! implementation note: will split up a string by newlines and query sizeLine() per each substring/line.
	//! @param str the utf-8 string to get the size of
	//! @param out_w the integer to hold the width
	//! @param out_h the integer to hold the height
	//! @param override_pointsize a hack to let us reuse the same font for any point size...
	//! @return 0 on success, non-zero on error
	int sizeText(std::string str, int* out_w, int* out_h, int override_pointsize = 0) const;

	//! get the height of the font
	//! @return the font height in pixels
	int height(int override_pointsize = 0) const;

	//! get the height a multiline string will take up
	//! @param str the utf-8 string to get the height of
	//! @param override_pointsize a hack to let us reuse the same font for any point size...
	//! @return the font height in pixels
	int textHeight(std::string str, int override_pointsize = 0) const;

	//! get the width a multiline string will take up
	//! @param str the utf-8 string to get the width of
	//! @param override_pointsize a hack to let us reuse the same font for any point size...
	//! @return the font width in pixels
	int textWidth(std::string str, int override_pointsize = 0) const;

	//! get a Font object from the engine
	//! @param name The Font name
	//! @return the Font or nullptr if it could not be retrieved
	static Font* get(const char* name);

	//! dump engine's font cache
	static void dumpCache();

	//! draws the text
	//! @param str the utf-8 string to render
	//! @param x the x coordinate from the left corner of the screen to render to.
	//! @param y the y coordinate from the top of the screen to render to.
	//! @param color the RGBA color value to render the text via.
	//! @param override_pointsize a hack to let us reuse the same font for any point size...
	void drawText(std::string str, int x, int y, int override_pointsize = 0);

	//! draws the text with the given color
	//! implementation note: will split up a string by newlines and draw them one at a time via drawLineColor() per each substring/line.
	//TODO: Maybe we can hack Fontstash to support scaling, if we need it?
	//! @param str the utf-8 string to render
	//! @param x the x coordinate from the left corner of the screen to render to.
	//! @param y the y coordinate from the top of the screen to render to.
	//! @param color the RGBA color value to render the text via.
	//! @param override_pointsize a hack to let us reuse the same font for any point size...
	void drawTextColor(std::string str, int x, int y, const Uint32& color, int override_pointsize = 0); //TODO.

private:
	std::string name;
	FONScontext* fontstash = nullptr;
	int font_style_normal = 0;
	int pointSize = 16; //TODO: Split pointsize from the filename and let us set this on the fly?
	int outlineSize = 0;
};
