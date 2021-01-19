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

	//! renders the text using its pre-specified parameters.
	//! this should usually not be called by the user.
	void render();

public:
	//! size of the black text outline
	static const int outlineSize = 0;

	//! special char marks font to be used
	static const char fontBreak = 8;

	const char*				getName() const { return name.c_str(); }
	const GLuint			getTexID() const { return texid; }
	const SDL_Surface*		getSurf() const { return surf; }
	const unsigned int		getWidth() const { return width; }
	const unsigned int		getHeight()	const { return height; }

	//! draws the text
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void draw(SDL_Rect src, SDL_Rect dest);

	//! draws the text with the given color
	//! @param src defines a subsection of the text image to actually draw (width 0 and height 0 uses whole image)
	//! @param dest the position and size of the image on-screen (width 0 and height 0 defaults to 1:1 scale)
	void drawColor(SDL_Rect src, SDL_Rect dest, const Uint32& color);

	//! get a Text object from the engine
	//! @param str The Text's string
	//! @param font the Text's font
	//! @return the Text or nullptr if it could not be retrieved
	static Text* get(const char* str, const char* font);

	//! dump engine's text cache
	static void dumpCache();

private:
	std::string name;
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	//! static geometry data for rendering the image to a quad
	static const GLuint indices[6];
	static const GLfloat positions[8];
	static const GLfloat texcoords[8];
	enum buffer_t {
		VERTEX_BUFFER,
		TEXCOORD_BUFFER,
		INDEX_BUFFER,
		BUFFER_TYPE_LENGTH
	};
	static GLuint vbo[BUFFER_TYPE_LENGTH];
	static GLuint vao;

	int width = 0;
	int height = 0;
	bool rendered = false;
};