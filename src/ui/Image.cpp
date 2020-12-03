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
}

bool Image::finalize() {
	if (surf) {
		SDL_LockSurface(surf);
		glGenTextures(1, &texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		glTexStorage2D(GL_TEXTURE_2D, 4, GL_RGBA8, surf->w, surf->h);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#ifdef PLATFORM_WINDOWS
			if (cvar_anisotropy.toFloat() > 0.0) {
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cvar_anisotropy.toFloat());
			}
#endif
			glGenerateMipmap(GL_TEXTURE_2D);
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

void Image::createStaticData() {
	// initialize buffer names
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		vbo[static_cast<buffer_t>(i)] = 0;
	}

	// create vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// upload vertex data
	glGenBuffers(1, &vbo[VERTEX_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	// upload texcoord data
	glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), texcoords, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	// upload index data
	glGenBuffers(1, &vbo[INDEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * 3 * sizeof(GLuint), indices, GL_STATIC_DRAW);

	// unbind vertex array
	glBindVertexArray(0);
}

void Image::deleteStaticData() {
	for (int i = 0; i < BUFFER_TYPE_LENGTH; ++i) {
		buffer_t buffer = static_cast<buffer_t>(i);
		if (vbo[buffer]) {
			glDeleteBuffers(1, &vbo[buffer]);
		}
	}
	if (vao) {
		glDeleteVertexArrays(1, &vao);
	}
}

void Image::draw(const SDL_Rect* src, const SDL_Rect& dest) const {
	drawColor(src, dest, 0xffffffff);
}

void Image::drawColor(const SDL_Rect* src, const SDL_Rect& dest, const Uint32& color) const {
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
	glActiveTexture(GL_TEXTURE0);
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
	glVertex2f(dest.x, yres - dest.y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.x + src->w, yres - dest.y - src->h);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.x + src->w, yres - dest.y);
	glEnd();

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
}