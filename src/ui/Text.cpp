// Text.cpp

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "Main.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Client.hpp"
#include "Text.hpp"

GLuint Text::vao = 0;
GLuint Text::vbo[BUFFER_TYPE_LENGTH] = { 0 };

const GLfloat Text::positions[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLfloat Text::texcoords[8]{
	0.f, 0.f,
	0.f, 1.f,
	1.f, 1.f,
	1.f, 0.f
};

const GLuint Text::indices[6]{
	0, 1, 2,
	0, 2, 3
};

Text::Text(const char* _name) : Asset(_name) {
	loaded = true;
}

Text::~Text() {
	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}
	if (texid) {
		glDeleteTextures(1, &texid);
		texid = 0;
	}
}

void Text::render() {
	Client* client = mainEngine->getLocalClient();
	if (!client) {
		return;
	}
	Renderer* renderer = client->getRenderer();
	if (!renderer) {
		return;
	} else if (!renderer->isInitialized()) {
		return;
	}

	// load font
	String strToRender;
	StringBuf<64> fontName;
	Uint32 fontIndex = 0u;
	if ((fontIndex = name.find(fontBreak)) != String::npos) {
		fontName = name.substr(fontIndex + 1, UINT32_MAX).get();
		strToRender = name.substr(0, fontIndex).get();
	} else {
		fontName = Font::defaultFont;
		strToRender = name.get();
	}
	Font* font = mainEngine->getFontResource().dataForString(fontName.get());
	if (!font) {
		return;
	}
	TTF_Font* ttf = font->getTTF();

	const Sint32 xres = renderer->getXres();
	SDL_Color colorBlack = { 0, 0, 0, 255 };
	SDL_Color colorWhite = { 255, 255, 255, 255 };

	if (surf) {
		SDL_FreeSurface(surf);
		surf = nullptr;
	}

	if (outlineSize > 0) {
		TTF_SetFontOutline(ttf, outlineSize);
		surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.get(), colorBlack, xres);
		TTF_SetFontOutline(ttf, 0);
		SDL_Surface* text = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.get(), colorWhite, xres);
		SDL_Rect rect;
		rect.x = 1; rect.y = 1;
		SDL_BlitSurface(text, NULL, surf, &rect);
		SDL_FreeSurface(text);
	} else {
		TTF_SetFontOutline(ttf, 0);
		surf = TTF_RenderUTF8_Blended_Wrapped(ttf, strToRender.get(), colorWhite, xres);
	}
	assert(surf);
	if (texid == 0) {
		glGenTextures(1, &texid);
	}

	width = 0;
	height = 0;
	int scan = surf->pitch / surf->format->BytesPerPixel;
	for (int y = 0; y < surf->h; ++y) {
		for (int x = 0; x < surf->w; ++x) {
			if (((Uint32 *)surf->pixels)[x + y * scan] != 0) {
				width = max(width, x);
				height = max(height, y);
			}
		}
	}
	width += 4;
	height += 4;

	// translate the original surface to an RGBA surface
	SDL_Surface* newSurf = SDL_CreateRGBSurface(0, width, height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Rect dest;
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = width;
	src.h = height;
	dest.x = 0;
	dest.y = 0;
	SDL_BlitSurface(surf, &src, newSurf, &dest); // blit onto a purely RGBA Surface
	SDL_FreeSurface(surf);
	surf = newSurf;

	// load the new surface as a GL texture
	SDL_LockSurface(surf);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, surf->w, surf->h);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
	SDL_UnlockSurface(surf);

	rendered = true;
}

void Text::createStaticData() {
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

void Text::deleteStaticData() {
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

void Text::draw(Rect<int> src, Rect<int> dest) {
	drawColor(src, dest, glm::vec4(1.f));
}

void Text::drawColor(Rect<int> src, Rect<int> dest, const glm::vec4& color) {
	Client* client = mainEngine->getLocalClient(); assert(client);
	Renderer* renderer = client->getRenderer(); assert(renderer);
	int xres = renderer->getXres();
	int yres = renderer->getYres();

	if (!rendered) {
		render();
	}
	if (!rendered) {
		return;
	}

	// load shader
	Material* mat = mainEngine->getMaterialResource().dataForString("shaders/basic/2D.json");
	if (!mat) {
		return;
	}
	ShaderProgram& shader = mat->getShader().mount();

	glViewport(0, 0, xres, yres);
	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);

	src.w = src.w <= 0 ? surf->w : src.w;
	src.h = src.h <= 0 ? surf->h : src.h;
	dest.w = dest.w <= 0 ? surf->w : dest.w;
	dest.h = dest.h <= 0 ? surf->h : dest.h;

	// create view matrix
	glm::mat4 viewMatrix = glm::ortho(0.f, (float)xres, 0.f, (float)yres, 1.f, -1.f);

	// bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texid);

	// upload uniform variables
	glUniformMatrix4fv(shader.getUniformLocation("gView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform4fv(shader.getUniformLocation("gColor"), 1, glm::value_ptr(color));
	glUniform1i(shader.getUniformLocation("gTexture"), 0);

	// bind vertex array
	glBindVertexArray(vao);

	// upload positions
	GLfloat positions[8] = {
		(GLfloat)dest.x, (GLfloat)(yres - dest.y),
		(GLfloat)dest.x, (GLfloat)(yres - dest.y - dest.h),
		(GLfloat)(dest.x + dest.w), (GLfloat)(yres - dest.y - dest.h),
		(GLfloat)(dest.x + dest.w), (GLfloat)(yres - dest.y)
	};
	glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), positions, GL_DYNAMIC_DRAW);

	// upload texcoords
	GLfloat texcoords[8] = {
		(float)src.x / (float)surf->w, (float)src.y / (float)surf->h,
		(float)src.x / (float)surf->w, (float)(src.y + src.h) / (float)surf->h,
		(float)(src.x + src.w) / (float)surf->w, (float)(src.y + src.h) / (float)surf->h,
		(float)(src.x + src.w) / (float)surf->w, (float)src.y / (float)surf->h
	};
	glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), texcoords, GL_DYNAMIC_DRAW);

	// draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
}

Text* Text::get(const char* str, const char* font) {
	if (!str) {
		return nullptr;
	}
	if (font == nullptr || font[0] == '\0') {
		font = Font::defaultFont;
	}
	size_t len0 = strlen(str);
	size_t len1 = strlen(font);
	String textAndFont;
	textAndFont.alloc((Uint32)(len0 + len1 + 2));
	textAndFont.format("%s%c%s", str, Text::fontBreak, font);
	Text* text = mainEngine->getTextResource().dataForString(textAndFont.get());
	if (text && !text->rendered) {
		text->render();
	}
	return text;
}