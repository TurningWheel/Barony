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

	std::string path = clippedName;
    if ( PHYSFS_getRealDir(path.c_str()) != nullptr )
    {
        const char* baseDir = PHYSFS_getRealDir(path.c_str());
        path.insert(0, PHYSFS_getDirSeparator());
        path.insert(0, baseDir);
    } else {
#ifdef NINTENDO
        path.insert(0, PHYSFS_getDirSeparator());
        path.insert(0, BASE_DATA_DIR);
#endif
    }
	printlog("loading image '%s'...", path.c_str());
	if ((surf = IMG_Load(path.c_str())) == NULL) {
		printlog("failed to load image '%s'", path.c_str());
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
        GL_CHECK_ERR(glGenTextures(1, &texid));
        GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid));
        GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, point ? GL_NEAREST : GL_LINEAR));
        GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, point ? GL_NEAREST : GL_LINEAR));
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
        GL_CHECK_ERR(glDeleteTextures(1, &texid));
		texid = 0;
	}
	if ( outlineSurf )
	{
		SDL_FreeSurface(outlineSurf);
		outlineSurf = nullptr;
	}
}

void Image::bind() const {
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid));
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
    {}, // colors
};
Mesh Image::clockwiseMesh;

Shader Image::shader;

static const char v_glsl[] =
    "in vec3 iPosition;"
    "in vec2 iTexCoord;"
    "out vec2 TexCoord;"
    "uniform mat4 uProj;"
    "uniform mat4 uView;"
    "uniform mat4 uSection;"
    "void main() {"
    "gl_Position = uProj * uView * vec4(iPosition, 1.0);"
    "TexCoord = (uSection * vec4(iTexCoord, 0.0, 1.0)).xy;"
    "}";

static const char f_glsl[] =
    "in vec2 TexCoord;"
    "uniform sampler2D uTexture;"
    "uniform vec4 uColor;"
    "out vec4 FragColor;"
    "void main() {"
    "FragColor = texture(uTexture, TexCoord) * uColor;"
    "}";

void Image::setupGL(GLuint texid, const Uint32& color) {
    // initialize mesh if needed
    if (!mesh.isInitialized()) {
        mesh.init();
    }
    
    // initialize clockwise mesh if needed
    if (!clockwiseMesh.isInitialized()) {
        auto& positions = clockwiseMesh.data[0];
        auto& texcoords = clockwiseMesh.data[1];
        positions.clear();
        texcoords.clear();
        if (!clockwiseMesh.data[0].size()) {
            positions.insert(positions.end(), {0.f, 0.f, 0.f});
            texcoords.insert(texcoords.end(), {.5f, .5f});
            float x = 0.f;
            float y = -.5f;
            constexpr int increment = 25;
            constexpr int num_square_vertices = 200;
            for (int dir = 0, c = 0; c <= num_square_vertices; ++c) {
                positions.insert(positions.end(), {x, -y, 0.f});
                texcoords.insert(texcoords.end(), {x + 0.5f, y + 0.5f});

                if (dir == 0) {
                    y = -.5f;
                    x += 1.01f / increment;
                    if (x >= .5f) {
                        x = .5f;
                        dir = 1;
                    }
                }
                else if (dir == 1) {
                    x = 0.5f;
                    y += 1.01f / increment;
                    if (y >= .5f) {
                        y = .5f;
                        dir = 2;
                    }
                }
                else if (dir == 2) {
                    y = 0.5f;
                    x -= 1.01f / increment;
                    if (x <= -.5f) {
                        x = -.5f;
                        dir = 3;
                    }
                }
                else if (dir == 3) {
                    x = -0.5f;
                    y -= 1.01f / increment;
                    if (y <= -.5f) {
                        y = -.5f;
                        dir = 4;
                    }
                }
                else if (dir == 4) {
                    y = -.5f;
                    x += 1.01 / increment;
                    if (x >= 0.f) {
                        x = 0.f;
                        positions.insert(positions.end(), {x, -y, 0.f});
                        texcoords.insert(texcoords.end(), {x + 0.5f, y + 0.5f});
                        break;
                    }
                }
            }
        }
        clockwiseMesh.init();
    }
    
    // initialize shader if needed, then bind
    if (!shader.isInitialized()) {
        shader.init("2D image shader");
        shader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        shader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        shader.bindAttribLocation("iPosition", 0);
        shader.bindAttribLocation("iTexCoord", 1);
        shader.link();
        shader.bind();
        GL_CHECK_ERR(glUniform1i(shader.uniform("uTexture"), 0));
    } else {
        shader.bind();
    }
    
    // bind texture
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid));
    if (!drawingGui) {
        GL_CHECK_ERR(glEnable(GL_BLEND));
    }
    
    // upload color
    Uint8 r, g, b, a;
    getColor(color, &r, &g, &b, &a);
    float cv[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uColor"), 1, cv));
}

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
    
    // bind shader, etc.
    setupGL(texid, color);
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, viewport.x, viewport.x + viewport.w, viewport.y, viewport.y + viewport.h, -1.f, 1.f);
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));
    
    // view matrix
    mat4x4 view(1.f);
    v = {(float)dest.x, (float)(viewport.h - dest.y), 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    v = {(float)dest.w, (float)dest.h, 0.f, 0.f};
    (void)scale_mat(&m, &view, &v); view = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view));
    
    // section matrix
    mat4x4 sect(1.f);
    v = {(float)src->x / textureWidth, (float)src->y / textureHeight, 0.f, 0.f};
    (void)translate_mat(&m, &sect, &v); sect = m;
    v = {(float)src->w / textureWidth, (float)src->h / textureHeight, 0.f, 0.f};
    (void)scale_mat(&m, &sect, &v); sect = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uSection"), 1, GL_FALSE, (float*)&sect));

    // draw image
    mesh.draw();
    
    // reset GL state
    if (!drawingGui) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
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
    
    // bind shader, etc.
    setupGL(texid, color);
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, viewport.x, viewport.x + viewport.w, viewport.y, viewport.y + viewport.h, -1.f, 1.f);
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));
    
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
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view));
    
    // section matrix
    mat4x4 sect(1.f);
    v = {(float)src->x / textureWidth, (float)src->y / textureHeight, 0.f, 0.f};
    (void)translate_mat(&m, &sect, &v); sect = m;
    v = {(float)src->w / textureWidth, (float)src->h / textureHeight, 0.f, 0.f};
    (void)scale_mat(&m, &sect, &v); sect = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uSection"), 1, GL_FALSE, (float*)&sect));

    // draw image
    mesh.draw();
    
    // reset GL state
    if (!drawingGui) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
    }
}

void Image::drawClockwise(float lerp,
    const SDL_Rect* src, const SDL_Rect dest,
    const SDL_Rect viewport, const Uint32& color)
{
    if (!surf || !texid) {
        return;
    }
    drawClockwise(texid, surf->w, surf->h, lerp, src, dest, viewport, color);
}

void Image::drawClockwise(
    GLuint texid, int textureWidth, int textureHeight, float lerp,
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
    
    // bind shader, etc.
    setupGL(texid, color);
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, viewport.x, viewport.x + viewport.w, viewport.y, viewport.y + viewport.h, -1.f, 1.f);
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));
    
    // view matrix
    mat4x4 view(1.f);
    v = {(float)dest.x, (float)(viewport.h - dest.y), 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    v = {(float)dest.w, (float)dest.h, 0.f, 0.f};
    (void)scale_mat(&m, &view, &v); view = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, GL_FALSE, (float*)&view));
    
    // section matrix
    mat4x4 sect(1.f);
    v = {(float)src->x / textureWidth, (float)src->y / textureHeight, 0.f, 0.f};
    (void)translate_mat(&m, &sect, &v); sect = m;
    v = {(float)src->w / textureWidth, (float)src->h / textureHeight, 0.f, 0.f};
    (void)scale_mat(&m, &sect, &v); sect = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uSection"), 1, GL_FALSE, (float*)&sect));

    // draw image
    GL_CHECK_ERR(glFrontFace(GL_CW)); // we draw clockwise so need to set this.
    const int numVertices = std::min(
        (int)(lerp * clockwiseMesh.data[1].size() / 2 + 1),
        (int)(clockwiseMesh.data[1].size() / 2));
    clockwiseMesh.draw(GL_TRIANGLE_FAN, numVertices);
    GL_CHECK_ERR(glFrontFace(GL_CCW));
    
    // reset GL state
    if (!drawingGui) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
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
    clockwiseMesh.destroy();
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
