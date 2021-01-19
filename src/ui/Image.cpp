// Image.cpp

#include "../main.hpp"
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

	std::string path = _name;
	printlog("loading image '%s'...", _name);
	if ((surf = IMG_Load(path.c_str())) == NULL) {
		printlog("failed to load image '%s'", _name);
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
}

void Image::draw(const SDL_Rect* src, const SDL_Rect dest) const {
	drawColor(src, dest, 0xffffffff);
}

void Image::drawColor(const SDL_Rect* src, const SDL_Rect dest, const Uint32& color) const {
	if (!surf) {
		return;
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);

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
	real_t r = ((Uint8)(color >> mainsurface->format->Rshift)) / 255.f;
	real_t g = ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f;
	real_t b = ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f;
	real_t a = ((Uint8)(color >> mainsurface->format->Ashift)) / 255.f;
	glColor4f(r, g, b, a);

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x, yres - dest.y);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x, yres - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x + dest.w, yres - dest.y - dest.h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x + dest.w, yres - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

static std::unordered_map<std::string, Image*> hashed_images;
static const int IMAGE_BUDGET = 1000;

Image* Image::get(const char* name) {
	if (!name) {
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