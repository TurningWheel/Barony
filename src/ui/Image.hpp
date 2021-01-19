//! @file Image.hpp

#pragma once

#include "../main.hpp"

//! An Image is a type of asset that contains all the raw data for a unique 2D image
class Image {
private:
	Image() = default;
	Image(const char* _name);
	Image(const Image&) = delete;
	Image(Image&&) = delete;
	~Image();

	Image& operator=(const Image&) = delete;
	Image& operator=(Image&&) = delete;

	//! finish loading on main thread
	bool finalize();

public:
	//! draws the image
	//! @param src the section of the image to be used for drawing, or nullptr for the whole image
	//! @param dest the location and size by which the image should be drawn
	void draw(const SDL_Rect* src, const SDL_Rect dest) const;

	//! draws the image with the given color
	//! @param src the section of the image to be used for drawing, or nullptr for the whole image
	//! @param dest the location and size by which the image should be drawn
	//! @param color a 32-bit color to mix with the image
	void drawColor(const SDL_Rect* src, const SDL_Rect dest, const Uint32& color) const;

	//! get an Image object from the engine
	//! @param name The Image name
	//! @return the Image or nullptr if it could not be retrieved
	static Image* get(const char* name);

	//! dump engine's image cache
	static void dumpCache();

	const char*				getName() const { return name.c_str(); }
	virtual const bool		isStreamable() const { return true; }
	const GLuint			getTexID() const { return texid; }
	const SDL_Surface*		getSurf() const { return surf; }
	const unsigned int		getWidth() const { return surf ? surf->w : 0U; }
	const unsigned int		getHeight()	const { return surf ? surf->h : 0U; }

private:
	std::string name;
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;

	bool clamp = false;
	bool point = false;

	// static geometry data for rendering the image to a quad:
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
};