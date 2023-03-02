// Image.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "Image.hpp"
#include "Frame.hpp"

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
	std::string path;
	if (memcmp(clippedName, "rom:", 4) && memcmp(clippedName, "save:", 5)) {
		path = std::string("rom:/") + clippedName;
	} else {
		path = clippedName;
	}
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

	// read color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	if (!a) {
		return;
	}

	// for the use of a whole image
	SDL_Rect secondsrc;
	if (src == nullptr) {
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = surf->w;
		secondsrc.h = surf->h;
		src = &secondsrc;
	}

	if (!drawingGui) {
        glEnable(GL_BLEND);
        
		// setup projection matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);

		// push model matrix
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}

	// bind texture
	glBindTexture(GL_TEXTURE_2D, texid);
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

	if (!drawingGui) {
        glDisable(GL_BLEND);
        
		// pop matrices
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
}

void Image::drawSurface(GLuint texid, SDL_Surface* surf, const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color) {
	if ( !surf ) {
		return;
	}

	// read color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	if ( !a ) {
		return;
	}

	// for the use of a whole image
	SDL_Rect secondsrc;
	if ( src == nullptr ) {
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = surf->w;
		secondsrc.h = surf->h;
		src = &secondsrc;
	}

	if ( !drawingGui ) {
		glEnable(GL_BLEND);

		// setup projection matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);

		// push model matrix
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}

	// bind texture
	glBindTexture(GL_TEXTURE_2D, texid);
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

	if ( !drawingGui ) {
		glDisable(GL_BLEND);

		// pop matrices
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
}

void Image::drawSurfaceRotated(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color, real_t angle)
{
	if ( !surf ) {
		return;
	}

	// read color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	if ( !a ) {
		return;
	}

	if (!drawingGui) {
        glEnable(GL_BLEND);
        
		// setup projection matrix
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(viewport.x, viewport.w, viewport.y, viewport.h, -1, 1);
        glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
	}

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(dest.x, viewport.h - dest.y, 0);
	glRotatef(-angle * 180 / PI, 0.f, 0.f, 1.f);

	// for the use of a whole image
	SDL_Rect secondsrc;
	if ( src == nullptr ) {
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = surf->w;
		secondsrc.h = surf->h;
		src = &secondsrc;
	}

	// bind texture
	glBindTexture(GL_TEXTURE_2D, texid);
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(-dest.w / 2, dest.h / 2);
	glTexCoord2f(1.0 * ((real_t)src->x / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(-dest.w / 2, -dest.h / 2);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * (((real_t)src->y + src->h) / surf->h));
	glVertex2f(dest.w / 2, -dest.h / 2);
	glTexCoord2f(1.0 * (((real_t)src->x + src->w) / surf->w), 1.0 * ((real_t)src->y / surf->h));
	glVertex2f(dest.w / 2, dest.h / 2);
	glEnd();

	// pop matrices
	glPopMatrix();
	if (!drawingGui) {
        glDisable(GL_BLEND);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
}

static std::unordered_map<std::string, Image*> hashed_images;
static const size_t IMAGE_BUDGET = 1 * 1024 * 1024 * 512; // in bytes
static size_t IMAGE_VOLUME = 0; // in bytes

size_t Image::hash(const char* name) {
	if (!name || name[0] == '\0') {
		return 0;
	}
	return hashed_images.hash_function()(name);
}

Image* Image::get(size_t hash, const char* name) {
	if (!name || name[0] == '\0') {
		return nullptr;
	}

	// search for text using precomputed hash
	auto& map = hashed_images;
	auto bc = map.bucket_count();
	if (bc) {
		const auto& key_eq = map.key_eq();
		const auto& hash_fn = map.hash_function();
		auto chash = !(bc & (bc - 1)) ? hash & (bc - 1) :
			(hash < bc ? hash : hash % bc);
		for (auto it = map.begin(chash); it != map.end(chash); ++it) {
			if (hash == hash_fn(it->first) && it->first == name) {
				return it->second;
			}
		}
	}

	// image not found in cache, load it
	if (IMAGE_VOLUME > IMAGE_BUDGET) {
		dumpCache();
	}
	auto image = new Image(name);
	hashed_images.insert(std::make_pair(name, image));
	IMAGE_VOLUME += sizeof(Image) + sizeof(SDL_Surface); // header data
	IMAGE_VOLUME += image->getWidth() * image->getHeight() * 4; // 32-bpp pixel data
	IMAGE_VOLUME += 1024; // 1-kB buffer

	return image;
}

Image* Image::get(const char* name) {
	return get(hash(name), name);
}

void Image::dumpCache() {
	for (auto image : hashed_images) {
		delete image.second;
	}
	hashed_images.clear();
	IMAGE_VOLUME = 0;
}

#ifndef EDITOR
#include "../net.hpp"
#include "../interface/consolecommand.hpp"
static ConsoleCommand size("/images_cache_size", "measure image cache",
    [](int argc, const char** argv){
    messagePlayer(clientnum, MESSAGE_MISC, "cache size is: %llu bytes (%llu kB)", IMAGE_VOLUME, IMAGE_VOLUME / 1024);
    });
static ConsoleCommand dump("/images_cache_dump", "dump image cache",
    [](int argc, const char** argv){
    Image::dumpCache();
    messagePlayer(clientnum, MESSAGE_MISC, "dumped cache");
    });
#endif
