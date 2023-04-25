// Image.cpp

#include "../main.hpp"
#include "../draw.hpp"
#include "Image.hpp"
#include "Frame.hpp"

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
    if (!surf || !texid) {
        return;
    }
	draw(texid, surf->w, surf->h, src, dest, viewport, 0xffffffff);
}

void Image::drawColor(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color) const {
    if (!surf || !texid) {
        return;
    }
    draw(texid, surf->w, surf->h, src, dest, viewport, color);
}

void Image::drawRotated(const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport, const Uint32& color, real_t angle) const {
    if (!surf || !texid) {
        return;
    }
    draw(texid, surf->w, surf->h, src, dest, viewport, color, angle);
}

Mesh Image::mesh = {
    {
        0.f, -1.f, 0.f,
        1.f, -1.f, 0.f,
        1.f,  0.f, 0.f,
        0.f, -1.f, 0.f,
        1.f,  0.f, 0.f,
        0.f,  0.f, 0.f,
    }, // positions
    {
        0.f, 1.f,
        1.f, 1.f,
        1.f, 0.f,
        0.f, 1.f,
        1.f, 0.f,
        0.f, 0.f,
    }, // texcoords
    {
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
    }, // colors
};

Shader Image::shader;

static const char v_glsl[] =
    "#version 120\n"
    "attribute vec3 iPosition;"
    "attribute vec2 iTexCoord;"
    "attribute vec4 iColor;"
    "varying vec2 TexCoord;"
    "varying vec4 Color;"
    "uniform vec4 uColor;"
    "uniform mat4 uProj;"
    "uniform mat4 uView;"
    "uniform mat4 uSection;"
    "void main() {"
    "gl_Position = uProj * uView * vec4(iPosition, 1.0);"
    "TexCoord = (uSection * vec4(iTexCoord, 0.0, 1.0)).xy;"
    "Color = iColor * uColor;"
    "}";

static const char f_glsl[] =
    "#version 120\n"
    "varying vec2 TexCoord;"
    "varying vec4 Color;"
    "uniform sampler2D uTexture;"
    "void main() {"
    "gl_FragColor = texture2D(uTexture, TexCoord) * Color;"
    "}";

void Image::draw(GLuint texid, int textureWidth, int textureHeight,
    const SDL_Rect* src, const SDL_Rect dest,
    const SDL_Rect viewport, const Uint32& color)
{
    // read color
    Uint8 r, g, b, a;
    getColor(color, &r, &g, &b, &a);
    if (!a) {
        return;
    }
    
    // default src
    SDL_Rect _src;
    if (!src) {
        _src = {0, 0, textureWidth, textureHeight};
        src = &_src;
    }
    
    // initialize mesh if needed
    if (!mesh.isInitialized()) {
        mesh.init();
    }
    
    // initialize shader if needed, then bind
    if (!shader.isInitialized()) {
        shader.init("2D image shader");
        shader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        shader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        shader.bindAttribLocation("iPosition", 0);
        shader.bindAttribLocation("iTexCoord", 1);
        shader.bindAttribLocation("iColor", 2);
        shader.link();
        shader.bind();
        glUniform1i(shader.uniform("uTexture"), 0);
    } else {
        shader.bind();
    }
    
    // bind texture
    glBindTexture(GL_TEXTURE_2D, texid);
    if (!drawingGui) {
        glEnable(GL_BLEND);
    }
    
    // upload color
    float cv[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    glUniform4fv(shader.uniform("uColor"), 1, cv);
    
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, viewport.x, viewport.x + viewport.w, viewport.y, viewport.y + viewport.h, -1.f, 1.f);
    glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj);
    
    // view matrix
    mat4x4 view(1.f);
    v = {(float)dest.x, (float)(viewport.h - dest.y), 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    v = {(float)dest.w, (float)dest.h, 0.f, 0.f};
    (void)scale_mat(&m, &view, &v); view = m;
    glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view);
    
    // section matrix
    mat4x4 sect(1.f);
    v = {(float)src->x / textureWidth, (float)src->y / textureHeight, 0.f, 0.f};
    (void)translate_mat(&m, &sect, &v); sect = m;
    v = {(float)src->w / textureWidth, (float)src->h / textureHeight, 0.f, 0.f};
    (void)scale_mat(&m, &sect, &v); sect = m;
    glUniformMatrix4fv(shader.uniform("uSection"), 1, GL_FALSE, (float*)&sect);

    // draw image
    mesh.draw();
    
    // reset GL state
    shader.unbind();
    if (!drawingGui) {
        glDisable(GL_BLEND);
    }
}
                       
void Image::draw(GLuint texid, int textureWidth, int textureHeight,
    const SDL_Rect* src, const SDL_Rect dest, const SDL_Rect viewport,
    const Uint32& color, real_t angle)
{
	// read color
	Uint8 r, g, b, a;
	getColor(color, &r, &g, &b, &a);
	if (!a) {
		return;
	}
    
    // default src
    SDL_Rect _src;
    if (!src) {
        _src = {0, 0, textureWidth, textureHeight};
        src = &_src;
    }
    
    // initialize mesh if needed
    if (!mesh.isInitialized()) {
        mesh.init();
    }
    
    // initialize shader if needed, then bind
    if (!shader.isInitialized()) {
        shader.init("2D image shader");
        shader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        shader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        shader.bindAttribLocation("iPosition", 0);
        shader.bindAttribLocation("iTexCoord", 1);
        shader.bindAttribLocation("iColor", 2);
        shader.link();
        shader.bind();
        glUniform1i(shader.uniform("uTexture"), 0);
    } else {
        shader.bind();
    }
    
    // bind texture
    glBindTexture(GL_TEXTURE_2D, texid);
    if (!drawingGui) {
        glEnable(GL_BLEND);
    }
    
    // upload color
    float cv[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    glUniform4fv(shader.uniform("uColor"), 1, cv);
    
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, viewport.x, viewport.x + viewport.w, viewport.y, viewport.y + viewport.h, -1.f, 1.f);
    glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj);
    
    // view matrix
    mat4x4 view(1.f);
    v = {(float)dest.x, (float)(viewport.h - dest.y), 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    v = {0.f, 0.f, -1.f, 0.f};
    (void)rotate_mat(&m, &view, angle / PI * 180.f, &v); view = m;
    v = {(float)dest.w / 2.f, (float)dest.h / 2.f, 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    v = {(float)dest.w, (float)dest.h, 0.f, 0.f};
    (void)scale_mat(&m, &view, &v); view = m;
    glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view);
    
    // section matrix
    mat4x4 sect(1.f);
    v = {(float)src->x / textureWidth, (float)src->y / textureHeight, 0.f, 0.f};
    (void)translate_mat(&m, &sect, &v); sect = m;
    v = {(float)src->w / textureWidth, (float)src->h / textureHeight, 0.f, 0.f};
    (void)scale_mat(&m, &sect, &v); sect = m;
    glUniformMatrix4fv(shader.uniform("uSection"), 1, GL_FALSE, (float*)&sect);

    // draw image
    mesh.draw();
    
    // reset GL state
    shader.unbind();
    if (!drawingGui) {
        glDisable(GL_BLEND);
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
    mesh.destroy();
    shader.destroy();
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
