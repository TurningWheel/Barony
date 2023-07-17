//! @file Text.hpp

#pragma once

#include "../main.hpp"

//! Contains some text that was rendered to a texture with a ttf font.
class Text {
friend class Field;
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
	static const char fontBreak = '\b';

	const char*				getName() const { return name.c_str(); }
	const GLuint			getTexID() const { return texid; }
	const SDL_Surface*		getSurf() const { return surf; }
	const unsigned int		getWidth() const { return width; }
	const unsigned int		getHeight()	const { return height; }
	int						getNumTextLines() const { return num_text_lines; }

	//! draws the text
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	//! @param viewport the dimensions of the viewport
	void draw(const SDL_Rect src, const SDL_Rect dest, const SDL_Rect viewport) const;

	//! draws the text with the given color
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	//! @param viewport the dimensions of the viewport
	//! @param color 32-bit encoded color to colorize the text
	void drawColor(const SDL_Rect src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color) const;

	//! get a Text object from the engine. Loads it if it has not been loaded
	//! @param str The Text's string
	//! @param font the Text's font
	//! @param textColor the color of the rendered text
	//! @param outlineColor the color of the rendered outline
	//! @return the Text or nullptr if it could not be retrieved
	static Text* get(const char* str, const char* font, Uint32 textColor, Uint32 outlineColor);

	//! get a text object. loads it if it has not been loaded
	//! @param hash the hash value of the cached text
	//! @return the text object, or nullptr if it was not found
	static Text* get(size_t hash, const char* key);

	//! turn the given parameters into a hash value which can be used to subsequently retrieve cached text more quickly
	//! @param str The Text's string
	//! @param font the Text's font
	//! @param textColor the color of the rendered text
	//! @param outlineColor the color of the rendered outline
	//! @return the hash for the given parameters, and the format string (valid until next call)
	static std::pair<size_t, const char*> hash(const char* str, const char* font, Uint32 textColor, Uint32 outlineColor);

	//! dump engine's text cache
	static void dumpCache();

	//! check if text cache needs to be dumped due to excess size
	static void dumpCacheInMainLoop();

	//! renders the text using its pre-specified parameters.
	//! you usually won't need to call this yourself,
	//! but if for some reason the text object has changed,
	//! you can call this function again to re-render it.
	void render();

	//! add a key value pair to the highlighted word map
	//! @param word the word 'index' in the sentence (first word is 0)
	//! @param color the color to set the word to
	void addWordToHighlight(int word, Uint32 color) { wordsToHighlight[word] = color; }

	//! gets map for highlighted words
	const std::map<int, Uint32>& getWordsToHighlight() const { return wordsToHighlight; }

	//! reset the highlighted word map
	void clearWordsToHighlight() { wordsToHighlight.clear(); }
private:
	std::string name;
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	int width = 0;
	int height = 0;
	int num_text_lines = 0;
	
	// words with index matching the key (first word == 0) will be drawn with the value color
	std::map<int, Uint32> wordsToHighlight; 

	//! get the number of text lines occupied by the text
	//! @return number of lines of text
	int countNumTextLines() const;
};
