// Image.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "Image.hpp"

GLuint Image::vao = 0;
GLuint Image::vbo[BUFFER_TYPE_LENGTH] = { 0 };

const GLfloat Image::positions[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLfloat Image::texcoords[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLuint Image::indices[6]{
	0, 1, 2,
	0, 2, 3
};

Image::Image(const char* _name) {
	name = _name;

	const char* clippedName = _name;
	do {
		if (clippedName[0] == '#') {
			++clippedName;
			clamp = true;
		}
		else if (clippedName[0] == '*') {
			++clippedName;
			point = true;
		}
		else {
			break;
		}
	} while (1);

#ifdef NINTENDO
	std::string path = std::string("rom:/") + _name;
#else
	std::string path = clippedName;
#endif
	printlog("loading image '%s'...", clippedName);
	if ((surf = IMG_Load(path.c_str())) == NULL) {
		printlog("failed to load image '%s'", clippedName);
		return;
	}

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, surf->w, surf->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_BlitSurface(surf, nullptr, newSurf, nullptr); // blit onto a purely RGBA Surface
	SDL_FreeSurface(surf);
	surf = newSurf;

	(void)finalize();
}

bool Image::finalize() {
	if (surf) {
		SDL_LockSurface(surf);
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		if (clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		if (point) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		SDL_UnlockSurface(surf);

		return true;
	} else {
		return false;
	}
}

Image::~Image() {
	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if (texid) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}
	if ( outlineSurf )
	{
		SDL_FreeSurface(outlineSurf);
		outlineSurf = nullptr;
	}
}

void Image::bind() const {
    glBindTexture(GL_TEXTURE_2D, texid);
}

void Image::draw(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport) const {
	drawColor(src, dest, viewport, 0xffffffff);
}

void Image::drawColor(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color) const {
	if (!surf) {
		return;
	}
	if (!color) {
	    return;
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
	glLoadIdentity();
	glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// for the use of a whole image
	SDL_Rect secondsrc;
	if (src == nullptr) {
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = surf->w;
		secondsrc.h = surf->h;
		src = &secondsrc;
	}

	// bind texture
	glBindTexture(GL_TEXTURE_2D, texid);

	// consume color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x, viewport.h - dest.y);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

void Image::drawSurface(SDL_Surface* surf, const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color)
{
	if ( !surf )
	{
		return;
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
	glLoadIdentity();
	glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// for the use of a whole image
	SDL_Rect secondsrc;
	if ( src == nullptr ) {
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = surf->w;
		secondsrc.h = surf->h;
		src = &secondsrc;
	}

	// consume color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x, viewport.h - dest.y);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x + dest.w, viewport.h - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

static std::unordered_map<std::string, Image*> hashed_images;
static const int IMAGE_BUDGET = 1000;

Image* Image::get(const char* name) {
	if (!name || name[0] == '\0') {
		return nullptr;
	}
	Image* image = nullptr;
	auto search = hashed_images.find(name);
	if (search == hashed_images.end()) {
		if (hashed_images.size() > IMAGE_BUDGET) {
			dumpCache();
		}
		image = new Image(name);
		hashed_images.insert(std::make_pair(name, image));
	} else {
		image = search->second;
	}
	return image;
}

void Image::dumpCache() {
	for (auto image : hashed_images) {
		delete image.second;
	}
	hashed_images.clear();
}
