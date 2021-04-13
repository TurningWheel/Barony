//! @file Text.hpp

#pragma once

#include "../main.hpp"

//! Contains some text that was rendered to a texture with a ttf font.
class Text {
private:
	Text() = default;
	Text(const char* _name);
	Text(const Text&) = delete;
	Text(Text&&) = delete;
	~Text();

	Text& operator=(const Text&) = delete;
	Text& operator=(Text&&) = delete;

public:
	//! special char marks font to be used
	static const char fontBreak = 8;

	const char*				getName() const { return name.c_str(); }
	const unsigned int		getWidth() const { return width; }
	const unsigned int		getHeight()	const { return height; }

	//! draws the text
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void draw(SDL_Rect src, SDL_Rect dest, int override_pointsize = 0);
	void draw(int x, int y, int override_pointsize = 0); //Wrapper to draw converting x and y coords to default rects.

	//! draws the text with the given color
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void drawColor(SDL_Rect src, SDL_Rect dest, const Uint32& color, int override_pointsize = 0);
	void drawColor(int x, int y, const Uint32& color, int override_pointsize = 0); //Wrapper to drawColor converting x and y coords to default rects.

	//! get a Text object from the engine
	//! @param str The Text's string
	//! @param font the Text's font
	//! @return the Text or nullptr if it could not be retrieved
	static Text* get(const char* str, const char* font);

	//! dump engine's text cache
	static void dumpCache();

private:
	std::string name;

	int width = 0;
	int height = 0;
};