/*-------------------------------------------------------------------------------

	BARONY
	File: draw.hpp
	Desc: prototypes for draw.cpp, various drawing functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

vec4_t* add_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b);
vec4_t* sub_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b);
vec4_t* mul_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b);
vec4_t* div_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b);
vec4_t* pow_vec4(vec4_t* result, const vec4_t* v, float f);

class TempTexture {
private:
    GLuint _texid = 0;
public:
    const GLuint& texid = _texid;

    TempTexture() {
        glGenTextures(1, &_texid);
    }

    ~TempTexture() {
        if( _texid ) {
            glDeleteTextures(1,&_texid);
            _texid = 0;
        }
    }
    
    void setParameters(bool clamp, bool point) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, point ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, point ? GL_NEAREST : GL_LINEAR);
        //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.f);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }

    void load(SDL_Surface* surf, bool clamp, bool point) {
        SDL_LockSurface(surf);
        glBindTexture(GL_TEXTURE_2D, _texid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        setParameters(clamp, point);
        SDL_UnlockSurface(surf);
    }
    
    void loadFloat(float* data, int width, int height, bool clamp, bool point) {
        glBindTexture(GL_TEXTURE_2D, _texid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_FLOAT, data);
        setParameters(clamp, point);
    }

    void bind() {
        glBindTexture(GL_TEXTURE_2D, _texid);
    }
};

#include <initializer_list>
#include <cassert>

struct Mesh {
    enum class BufferType : unsigned int {
        Position,    // vec3 float
        TexCoord,    // vec2 float
        Color,       // vec4 float
        Max
    };
    static const std::unordered_map<BufferType, int> ElementsPerVBO;

    Mesh(
        std::initializer_list<float>&& positions,
        std::initializer_list<float>&& texcoords,
        std::initializer_list<float>&& colors) :
        data{{positions}, {texcoords}, {colors}}
        {}

    Mesh(
        const std::initializer_list<float>& positions,
        const std::initializer_list<float>& texcoords,
        const std::initializer_list<float>& colors) :
        data{{positions}, {texcoords}, {colors}}
        {}

    const std::vector<float> data[(int)BufferType::Max];
    const std::vector<unsigned int> index;

    void init();
    void destroy();
    void draw() const;

private:
    unsigned int vao = 0; // vertex array object (mesh handle)
    unsigned int vbo[(int)BufferType::Max]; // vertex buffer objects
};

#include "shader.hpp"

struct framebuffer {
    unsigned int fbo = 0;
    unsigned int fbo_color = 0;
    unsigned int fbo_depth = 0;
    unsigned int xsize = 1280;
    unsigned int ysize = 720;
    
    static constexpr int NUM_PBOS = 2;
    unsigned int pbos[NUM_PBOS];
    unsigned int pboindex = 0;

    void init(unsigned int _xsize, unsigned int _ysize, GLint minFilter, GLint magFilter);
    void destroy();
    void bindForWriting();
    void bindForReading() const;
    
    void* lock();
    void unlock();
    
    void draw(float brightness = 1.f);
    void hdrDraw(float brightness, float gamma, float exposure);
    static void unbindForWriting();
    static void unbindForReading();
    static void unbindAll();

    static Mesh mesh;
    static Shader shader;
    static Shader hdrShader;
};

// view structure
typedef struct view_t
{
    real_t x, y, z;
    real_t ang;
    real_t vang;
    Sint32 winx, winy, winw, winh;
    real_t globalLightModifier = 0.0;
    real_t globalLightModifierEntities = 0.0;
    int globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
    framebuffer fb[1];
    bool* vismap = nullptr;
    float luminance = 0.25f;
    unsigned int drawnFrames = 0;
} view_t;

#define FLIP_VERTICAL 1
#define FLIP_HORIZONTAL 2
SDL_Surface* flipSurface(SDL_Surface* surface, int flags);
void drawCircle(int x, int y, real_t radius, Uint32 color, Uint8 alpha);
void drawArc(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawArcInvertedY(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawLine(int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha);
int drawRect(SDL_Rect* src, Uint32 color, Uint8 alpha);
int drawBox(SDL_Rect* src, Uint32 color, Uint8 alpha);
void drawGear(Sint16 x, Sint16 y, real_t size, Sint32 rotation);
void drawImage(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageRing(SDL_Surface* image, SDL_Rect* src, int radius, int thickness, int segments, real_t angStart, real_t angEnd, Uint8 alpha);
void drawImageScaled(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageScaledPartial(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, float percentY);
void drawImageAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint8 alpha);
void drawImageColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
void drawImageFancy(SDL_Surface* image, Uint32 color, real_t angle, SDL_Rect* src, SDL_Rect* pos);
void drawImageRotatedAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, real_t angle, Uint8 alpha);
void drawImageScaledColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
SDL_Surface* scaleSurface(SDL_Surface* Surface, Uint16 Width, Uint16 Height);
void drawSky3D(view_t* camera, SDL_Surface* tex);
void drawLayer(long camx, long camy, int z, map_t* map);
void drawBackground(long camx, long camy);
void drawForeground(long camx, long camy);
void drawClearBuffers();
void raycast(const view_t& camera, Sint8 (*minimap)[MINIMAP_MAX_DIMENSION]);
void drawFloors(view_t* camera);
void drawSky(SDL_Surface* srfc);
void drawVoxel(view_t* camera, Entity* entity);
void drawEntities3D(view_t* camera, int mode);
void drawPalette(voxel_t* model);
void drawEntities2D(long camx, long camy);
void drawGrid(long camx, long camy);
void drawEditormap(long camx, long camy);
void drawWindow(int x1, int y1, int x2, int y2);
void drawDepressed(int x1, int y1, int x2, int y2);
void drawWindowFancy(int x1, int y1, int x2, int y2);
SDL_Rect ttfPrintTextColor( TTF_Font* font, int x, int y, Uint32 color, bool outline, const char* str );
SDL_Rect ttfPrintText( TTF_Font* font, int x, int y, const char* str );
SDL_Rect ttfPrintTextFormattedColor( TTF_Font* font, int x, int y, Uint32 color, char const * const fmt, ... );
SDL_Rect ttfPrintTextFormatted( TTF_Font* font, int x, int y, char const * const fmt, ... );
void printTextFormatted( SDL_Surface* font_bmp, int x, int y, char const * const fmt, ... );
void printTextFormattedAlpha(SDL_Surface* font_bmp, int x, int y, Uint8 alpha, char const * const fmt, ...);
void printTextFormattedColor(SDL_Surface* font_bmp, int x, int y, Uint32 color, char const * const fmt, ...);
void printTextFormattedFancy(SDL_Surface* font_bmp, int x, int y, Uint32 color, real_t angle, real_t scale, char* fmt, ...);
void printText( SDL_Surface* font_bmp, int x, int y, const char* str );
void drawSprite(view_t* camera, Entity* entity);
void drawTooltip(SDL_Rect* src, Uint32 optionalColor = 0);
Uint32 getPixel(SDL_Surface* surface, int x, int y);
void putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);
void getColor(Uint32 color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
bool behindCamera(const view_t& camera, real_t x, real_t y);
void occlusionCulling(map_t& map, view_t& camera);

constexpr Uint32 makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((Uint32)a << 24) | ((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r << 0);
}

constexpr Uint32 makeColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return 0xff000000 | ((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r << 0);
}

extern framebuffer main_framebuffer;
extern Shader voxelShader;
extern Shader voxelBrightShader;
extern Shader voxelDitheredShader;
extern Shader voxelBrightDitheredShader;
extern Shader worldShader;
extern Shader worldBrightShader;
extern Shader worldDarkShader;
extern TempTexture* lightmapTexture;

#define TRANSPARENT_TILE 246

struct Chunk {
    GLuint vbo_positions = 0;
    GLuint vbo_texcoords = 0;
    GLuint vbo_colors = 0;
    GLint indices = 0;
    
    Chunk() = default;
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    
    Chunk(Chunk&& rhs) {
        vbo_positions = rhs.vbo_positions;
        vbo_texcoords = rhs.vbo_texcoords;
        vbo_colors = rhs.vbo_colors;
        indices = rhs.indices;
        x = rhs.x;
        y = rhs.y;
        w = rhs.w;
        h = rhs.h;
        tiles = rhs.tiles;
        
        rhs.vbo_positions = 0;
        rhs.vbo_texcoords = 0;
        rhs.vbo_colors = 0;
        rhs.indices = 0;
        rhs.x = 0;
        rhs.y = 0;
        rhs.w = 0;
        rhs.h = 0;
        rhs.tiles = nullptr;
    }
    
    Chunk& operator=(Chunk&& rhs) {
        vbo_positions = rhs.vbo_positions;
        vbo_texcoords = rhs.vbo_texcoords;
        vbo_colors = rhs.vbo_colors;
        indices = rhs.indices;
        x = rhs.x;
        y = rhs.y;
        w = rhs.w;
        h = rhs.h;
        tiles = rhs.tiles;
        
        rhs.vbo_positions = 0;
        rhs.vbo_texcoords = 0;
        rhs.vbo_colors = 0;
        rhs.indices = 0;
        rhs.x = 0;
        rhs.y = 0;
        rhs.w = 0;
        rhs.h = 0;
        rhs.tiles = nullptr;
        return *this;
    }
    
    ~Chunk() {
        destroyBuffers();
    }
    
    void build(const map_t& map, bool ceiling, int startX, int startY, int w, int h);
    void buildBuffers(const std::vector<float>& positions, const std::vector<float>& texcoords, const std::vector<float>& colors);
    void destroyBuffers();
    void draw();
    bool isDirty(const map_t& map);
    
    int x = 0, y = 0, w = 0, h = 0;
    Sint32* tiles = nullptr;
};
void clearChunks();
void createChunks();

void createCommonDrawResources();
void destroyCommonDrawResources();

extern view_t cameras[MAXPLAYERS];
extern view_t menucam;

// function prototypes for opengl.c:
#define REALCOLORS 0
#define ENTITYUIDS 1
real_t getLightForEntity(real_t x, real_t y);
void beginGraphics();
void glBeginCamera(view_t* camera);
void glDrawVoxel(view_t* camera, Entity* entity, int mode);
void glDrawSprite(view_t* camera, Entity* entity, int mode);
void glDrawWorldUISprite(view_t* camera, Entity* entity, int mode);
void glDrawWorldDialogueSprite(view_t* camera, void* worldDialogue, int mode);
bool glDrawEnemyBarSprite(view_t* camera, int mode, void* enemyHPBarDetails, bool doVisibilityCheckOnly);
void glDrawSpriteFromImage(view_t* camera, Entity* entity, std::string text, int mode);
void glDrawWorld(view_t* camera, int mode);
void glEndCamera(view_t* camera);
void glEndCamera(view_t* camera);
unsigned int GO_GetPixelU32(int x, int y, view_t& camera);

extern bool hdrEnabled;
