//! @file Image.hpp

#pragma once

#include "../main.hpp"
#include "../draw.hpp"
#include "../shader.hpp"

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
	//! @param viewport the viewport dimensions
	void draw(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport) const;

	//! draws the image with the given color
	//! @param src the section of the image to be used for drawing, or nullptr for the whole image
	//! @param dest the location and size by which the image should be drawn
	//! @param viewport the viewport dimensions
	//! @param color a 32-bit color to mix with the image
	void drawColor(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color) const;

	//! draws image with rotation and given color
	//! @param src the section of the image to be used for drawing, or nullptr for the whole image
	//! @param dest the location and size by which the image should be drawn
	//! @param viewport the viewport dimensions
	//! @param color a 32-bit color to mix with the image
	//! @param angle rotation
    void drawRotated(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color, real_t angle) const;

	//! draws arbitrary GL texture on-screen with given color
	//! @param texid GL texture id
	//! @param textureWidth GL texture width
    //! @param textureHeight GL texture height
	//! @param src the section of the image to be used for drawing, or nullptr for the whole image
	//! @param dest the position and size in screen-coordinates by which the image should be drawn
	//! @param viewport the dimensions of the viewport
	//! @param color a 32-bit color to mix with the image
	static void draw(
        GLuint texid, int textureWidth, int textureHeight,
        const SDL_Rect* src, const SDL_Rect dest,
        const SDL_Rect viewport, const Uint32& color);
    
    //! draws arbitrary GL texture on-screen with given color and rotation
    //! @param texid GL texture id
    //! @param textureWidth GL texture width
    //! @param textureHeight GL texture height
    //! @param src the section of the image to be used for drawing, or nullptr for the whole image
    //! @param dest the position and size in screen-coordinates by which the image should be drawn
    //! @param viewport the dimensions of the viewport
    //! @param color a 32-bit color to mix with the image
    //! @param angle rotation
    static void draw(
        GLuint texid, int textureWidth, int textureHeight,
        const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport,
        const Uint32& color, real_t angle);
    
    //! draw image with given color, in a clockwise-fan fashion
    //! @param lerp how much of the image to draw (1.0 is complete, 0.0 is none at all)
    //! @param src the section of the image to be used for drawing, or nullptr for the whole image
    //! @param dest the position and size in screen-coordinates by which the image should be drawn
    //! @param viewport the dimensions of the viewport
    //! @param color a 32-bit color to mix with the image
    void drawClockwise(float lerp,
        const SDL_Rect* src, const SDL_Rect dest,
        const SDL_Rect viewport, const Uint32& color);
    
    //! draws arbitrary GL texture on-screen with given color, in a clockwise-fan fashion
    //! @param texid GL texture id
    //! @param textureWidth GL texture width
    //! @param textureHeight GL texture height
    //! @param lerp how much of the image to draw (1.0 is complete, 0.0 is none at all)
    //! @param src the section of the image to be used for drawing, or nullptr for the whole image
    //! @param dest the position and size in screen-coordinates by which the image should be drawn
    //! @param viewport the dimensions of the viewport
    //! @param color a 32-bit color to mix with the image
    static void drawClockwise(
        GLuint texid, int textureWidth, int textureHeight, float lerp,
        const SDL_Rect* src, const SDL_Rect dest,
        const SDL_Rect viewport, const Uint32& color);

	//! bind this image as the active GL texture
	void bind() const;

	//! get an Image object from the engine. loads it if it has not been loaded
	//! @param name The Image name
	//! @return the Image or nullptr if it could not be retrieved
	static Image* get(const char* name);

	//! get an Image object. loads it if it has not been loaded
	//! @param hash the hash value of the cached image
	//! @return the Image object, or nullptr if it was not found
	static Image* get(size_t hash, const char* key);

	//! turn the given image name into a hash value
	//! @param name the name of the image
	//! @return the hash for the name
	static size_t hash(const char* name);

	//! dump engine's image cache
	static void dumpCache();
	
	const char*				getName() const { return name.c_str(); }
	virtual const bool		isStreamable() const { return true; }
	const GLuint			getTexID() const { return texid; }
	const SDL_Surface*		getSurf() const { return surf; }
	const SDL_Surface*		getOutlineSurf() const { return outlineSurf; }
	const void				setOutlineSurf(SDL_Surface* toSet) { outlineSurf = toSet; }
	const unsigned int		getWidth() const { return surf ? surf->w : 0U; }
	const unsigned int		getHeight()	const { return surf ? surf->h : 0U; }

private:
	std::string name;
	GLuint texid = 0;
	SDL_Surface* surf = nullptr;
	SDL_Surface* outlineSurf = nullptr;
    bool clamp = false;
    bool point = false;
    static Mesh mesh;
    static Mesh clockwiseMesh;
    static Shader shader;
    
    static void setupGL(GLuint texid, const Uint32& color);
};
