/*-------------------------------------------------------------------------------

	BARONY
	File: opengl.cpp
	Desc: contains all drawing functions for opengl

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "items.hpp"
#include "ui/Text.hpp"
#include "ui/GameUI.hpp"
#include "interface/interface.hpp"
#include "interface/consolecommand.hpp"
#include "mod_tools.hpp"
#include "player.hpp"
#include "ui/MainMenu.hpp"

static real_t getLightAtModifier = 1.0;
static real_t getLightAtAdder = 0.0;

#ifndef EDITOR
static ConsoleVariable<bool> cvar_fullBright("/fullbright", false);
#endif

static void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble fW, fH;

	fH = tan(fovY / 360 * PI) * zNear;
	fW = fH * aspect;

	glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

vec4_t vec4_copy(const vec4_t* v) {
	return vec4_t(v->x, v->y, v->z, v->w);
}

vec4_t* mul_mat_vec4(vec4_t* result, const mat4x4_t* m, const vec4_t* v) {
	result->x = m->x.x * v->x + m->y.x * v->y + m->z.x * v->z + m->w.x * v->w;
	result->y = m->x.y * v->x + m->y.y * v->y + m->z.y * v->z + m->w.y * v->w;
	result->z = m->x.z * v->x + m->y.z * v->y + m->z.z * v->z + m->w.z * v->w;
	result->w = m->x.w * v->x + m->y.w * v->y + m->z.w * v->z + m->w.w * v->w;
	return result;
}

vec4_t* add_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->x + b->x;
	result->y = a->y + b->y;
	result->z = a->z + b->z;
	result->w = a->w + b->w;
	return result;
}

vec4_t* sub_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
	result->w = a->w - b->w;
	return result;
}

vec4_t* mul_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->x * b->x;
	result->y = a->y * b->y;
	result->z = a->z * b->z;
	result->w = a->w * b->w;
	return result;
}

vec4_t* div_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->x / b->x;
	result->y = a->y / b->y;
	result->z = a->z / b->z;
	result->w = a->w / b->w;
	return result;
}

vec4_t* pow_vec4(vec4_t* result, const vec4_t* v, float f) {
	result->x = v->x * f;
	result->y = v->y * f;
	result->z = v->z * f;
	result->w = v->w * f;
	return result;
}

float dot_vec4(const vec4_t* a, const vec4_t* b) {
	return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

vec4_t* cross_vec3(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->y * b->z - a->z * b->y;
	result->y = a->z * b->x - a->x * b->z;
	result->z = a->x * b->y - a->y * b->x;
	return result;
}

vec4_t* cross_vec4(vec4_t* result, const vec4_t* a, const vec4_t* b) {
	result->x = a->y * b->z - a->z * b->y;
	result->y = a->z * b->w - a->w * b->z;
	result->z = a->w * b->x - a->x * b->w;
	result->w = a->x * b->y - a->y * b->x;
	return result;
}

float length_vec4(const vec4_t* v) {
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
}

vec4_t* normal_vec4(vec4_t* result, const vec4_t* v) {
	float length = length_vec4(v);
	result->x = v->x / length;
	result->y = v->y / length;
	result->z = v->z / length;
	result->w = v->w / length;
	return result;
}

mat4x4_t* mul_mat(mat4x4_t* result, const mat4x4_t* m1, const mat4x4_t* m2) {
	vec4 v[6];
	(void)add_vec4(
		&result->x,
		add_vec4(&v[0], pow_vec4(&v[1], &m1->x, m2->x.x), pow_vec4(&v[2], &m1->y, m2->x.y)),
		add_vec4(&v[3], pow_vec4(&v[4], &m1->z, m2->x.z), pow_vec4(&v[5], &m1->w, m2->x.w))
	);
	(void)add_vec4(
		&result->y,
		add_vec4(&v[0], pow_vec4(&v[1], &m1->x, m2->y.x), pow_vec4(&v[2], &m1->y, m2->y.y)),
		add_vec4(&v[3], pow_vec4(&v[4], &m1->z, m2->y.z), pow_vec4(&v[5], &m1->w, m2->y.w))
	);
	(void)add_vec4(
		&result->z,
		add_vec4(&v[0], pow_vec4(&v[1], &m1->x, m2->z.x), pow_vec4(&v[2], &m1->y, m2->z.y)),
		add_vec4(&v[3], pow_vec4(&v[4], &m1->z, m2->z.z), pow_vec4(&v[5], &m1->w, m2->z.w))
	);
	(void)add_vec4(
		&result->w,
		add_vec4(&v[0], pow_vec4(&v[1], &m1->x, m2->w.x), pow_vec4(&v[2], &m1->y, m2->w.y)),
		add_vec4(&v[3], pow_vec4(&v[4], &m1->z, m2->w.z), pow_vec4(&v[5], &m1->w, m2->w.w))
	);
	return result;
}

mat4x4_t* translate_mat(mat4x4_t* result, const mat4x4_t* m, const vec4_t* v) {
    vec4_t t[5];
    result->x = m->x;
    result->y = m->y;
    result->z = m->z;
    (void)add_vec4(&result->w, &m->w,
        add_vec4(&t[0],
            add_vec4(&t[1],
                pow_vec4(&t[2], &m->x, v->x),
                pow_vec4(&t[3], &m->y, v->y)),
            pow_vec4(&t[4], &m->z, v->z)));
    return result;
}

mat4x4_t* rotate_mat(mat4x4_t* result, const mat4x4_t* m, float angle, const vec4_t* v) {
    const float a = (angle / 180.f) * PI;
    const float c = cos(a);
    const float s = sin(a);

    vec4_t axis; (void)normal_vec4(&axis, v);
    vec4_t temp; (void)pow_vec4(&temp, &axis, 1.f - c);

    mat4x4_t rotate;
    rotate.x.x = c + temp.x * axis.x;
    rotate.x.y = temp.x * axis.y + s * axis.z;
    rotate.x.z = temp.x * axis.z - s * axis.y;

    rotate.y.x = temp.y * axis.x - s * axis.z;
    rotate.y.y = c + temp.y * axis.y;
    rotate.y.z = temp.y * axis.z + s * axis.x;

    rotate.z.x = temp.z * axis.x + s * axis.y;
    rotate.z.y = temp.z * axis.y - s * axis.x;
    rotate.z.z = c + temp.z * axis.z;

    mat4x4_t t(0.f);
    (void)add_vec4(&result->x,
        add_vec4(&t.w, pow_vec4(&t.x, &m->x, rotate.x.x), pow_vec4(&t.y, &m->y, rotate.x.y)),
        pow_vec4(&t.z, &m->z, rotate.x.z));
    (void)add_vec4(&result->y,
        add_vec4(&t.w, pow_vec4(&t.x, &m->x, rotate.y.x), pow_vec4(&t.y, &m->y, rotate.y.y)),
        pow_vec4(&t.z, &m->z, rotate.y.z));
    (void)add_vec4(&result->z,
        add_vec4(&t.w, pow_vec4(&t.x, &m->x, rotate.z.x), pow_vec4(&t.y, &m->y, rotate.z.y)),
        pow_vec4(&t.z, &m->z, rotate.z.z));
    result->w = m->w;
    return result;
}

mat4x4_t* scale_mat(mat4x4_t* result, const mat4x4_t* m, const vec4_t* v) {
    (void)pow_vec4(&result->x, &m->x, v->x);
    (void)pow_vec4(&result->y, &m->y, v->y);
    (void)pow_vec4(&result->z, &m->z, v->z);
    result->w = m->w;
    return result;
}

mat4x4_t* frustum(mat4x4_t* result, float left, float right, float bot, float top, float near, float far) {
    *result = mat4x4(0.f);
    result->x.x = 2.f / (right - left);
    result->x.z = (right + left) / (right - left);
    result->y.y = 2.f / (top - bot);
    result->y.z = (top + bot) / (top - bot);
    result->z.z = -(far + near) / (far - near);
    result->z.w = -1.f;
    result->w.z = -(2.f * far * near) / (far - near);
    return result;
}

#define perspective fast_perspective

mat4x4_t* slow_perspective(mat4x4_t* result, float fov, float aspect, float near, float far) {
    const float h = tanf((fov / 180.f * (float)PI) / 2.f);
    const float w = h * aspect;
    return frustum(result, -w, w, -h, h, near, far);
}

mat4x4_t* fast_perspective(mat4x4_t* result, float fov, float aspect, float near, float far) {
    const float h = tanf((fov / 180.f * (float)PI) / 2.f);
    const float w = h * aspect;
    
    *result = mat4x4(0.f);
    result->x.x = 1.f / w;
    result->y.y = 1.f / h;
    result->z.z = -(far + near) / (far - near);
    result->z.w = -1.f;
    result->w.z = -(2.f * far * near) / (far - near);
    return result;
}

mat4x4_t* mat_from_array(mat4x4_t* result, float matArray[16])
{
    memcpy((void*)result, (const void*)matArray, sizeof(mat4x4_t));
	return result;
}

bool invertMatrix4x4(const mat4x4_t* m, float invOut[16])
{
	double inv[16], det;
	int i;

	inv[0] = m->y.y * m->z.z * m->w.w -
		m->y.y * m->z.w * m->w.z -
		m->z.y * m->y.z * m->w.w +
		m->z.y * m->y.w * m->w.z +
		m->w.y * m->y.z * m->z.w -
		m->w.y * m->y.w * m->z.z;

	inv[4] = -m->y.x * m->z.z * m->w.w +
		m->y.x * m->z.w * m->w.z +
		m->z.x * m->y.z * m->w.w -
		m->z.x * m->y.w * m->w.z -
		m->w.x * m->y.z * m->z.w +
		m->w.x * m->y.w * m->z.z;

	inv[8] = m->y.x * m->z.y * m->w.w -
		m->y.x * m->z.w * m->w.y -
		m->z.x * m->y.y * m->w.w +
		m->z.x * m->y.w * m->w.y +
		m->w.x * m->y.y * m->z.w -
		m->w.x * m->y.w * m->z.y;

	inv[12] = -m->y.x * m->z.y * m->w.z +
		m->y.x * m->z.z * m->w.y +
		m->z.x * m->y.y * m->w.z -
		m->z.x * m->y.z * m->w.y -
		m->w.x * m->y.y * m->z.z +
		m->w.x * m->y.z * m->z.y;

	inv[1] = -m->x.y * m->z.z * m->w.w +
		m->x.y * m->z.w * m->w.z +
		m->z.y * m->x.z * m->w.w -
		m->z.y * m->x.w * m->w.z -
		m->w.y * m->x.z * m->z.w +
		m->w.y * m->x.w * m->z.z;

	inv[5] = m->x.x * m->z.z * m->w.w -
		m->x.x * m->z.w * m->w.z -
		m->z.x * m->x.z * m->w.w +
		m->z.x * m->x.w * m->w.z +
		m->w.x * m->x.z * m->z.w -
		m->w.x * m->x.w * m->z.z;

	inv[9] = -m->x.x * m->z.y * m->w.w +
		m->x.x * m->z.w * m->w.y +
		m->z.x * m->x.y * m->w.w -
		m->z.x * m->x.w * m->w.y -
		m->w.x * m->x.y * m->z.w +
		m->w.x * m->x.w * m->z.y;

	inv[13] = m->x.x * m->z.y * m->w.z -
		m->x.x * m->z.z * m->w.y -
		m->z.x * m->x.y * m->w.z +
		m->z.x * m->x.z * m->w.y +
		m->w.x * m->x.y * m->z.z -
		m->w.x * m->x.z * m->z.y;

	inv[2] = m->x.y * m->y.z * m->w.w -
		m->x.y * m->y.w * m->w.z -
		m->y.y * m->x.z * m->w.w +
		m->y.y * m->x.w * m->w.z +
		m->w.y * m->x.z * m->y.w -
		m->w.y * m->x.w * m->y.z;

	inv[6] = -m->x.x * m->y.z * m->w.w +
		m->x.x * m->y.w * m->w.z +
		m->y.x * m->x.z * m->w.w -
		m->y.x * m->x.w * m->w.z -
		m->w.x * m->x.z * m->y.w +
		m->w.x * m->x.w * m->y.z;

	inv[10] = m->x.x * m->y.y * m->w.w -
		m->x.x * m->y.w * m->w.y -
		m->y.x * m->x.y * m->w.w +
		m->y.x * m->x.w * m->w.y +
		m->w.x * m->x.y * m->y.w -
		m->w.x * m->x.w * m->y.y;

	inv[14] = -m->x.x * m->y.y * m->w.z +
		m->x.x * m->y.z * m->w.y +
		m->y.x * m->x.y * m->w.z -
		m->y.x * m->x.z * m->w.y -
		m->w.x * m->x.y * m->y.z +
		m->w.x * m->x.z * m->y.y;

	inv[3] = -m->x.y * m->y.z * m->z.w +
		m->x.y * m->y.w * m->z.z +
		m->y.y * m->x.z * m->z.w -
		m->y.y * m->x.w * m->z.z -
		m->z.y * m->x.z * m->y.w +
		m->z.y * m->x.w * m->y.z;

	inv[7] = m->x.x * m->y.z * m->z.w -
		m->x.x * m->y.w * m->z.z -
		m->y.x * m->x.z * m->z.w +
		m->y.x * m->x.w * m->z.z +
		m->z.x * m->x.z * m->y.w -
		m->z.x * m->x.w * m->y.z;

	inv[11] = -m->x.x * m->y.y * m->z.w +
		m->x.x * m->y.w * m->z.y +
		m->y.x * m->x.y * m->z.w -
		m->y.x * m->x.w * m->z.y -
		m->z.x * m->x.y * m->y.w +
		m->z.x * m->x.w * m->y.y;

	inv[15] = m->x.x * m->y.y * m->z.z -
		m->x.x * m->y.z * m->z.y -
		m->y.x * m->x.y * m->z.z +
		m->y.x * m->x.z * m->z.y +
		m->z.x * m->x.y * m->y.z -
		m->z.x * m->x.z * m->y.y;

	det = m->x.x * inv[0] + m->x.y * inv[4] + m->x.z * inv[8] + m->x.w * inv[12];

	if ( det == 0 )
		return false;

	det = 1.0 / det;

	for ( i = 0; i < 16; i++ )
		invOut[i] = inv[i] * det;

	return true;
}

vec4_t project(
	const vec4_t* world,
	const mat4x4_t* model,
	const mat4x4_t* projview,
	const vec4_t* window
) {
	vec4 copy;
	vec4_t result = *world; result.w = 1.f;
	copy = vec4_copy(&result); mul_mat_vec4(&result, model, &copy);
	copy = vec4_copy(&result); mul_mat_vec4(&result, projview, &copy);

	//float invertedProjview[16];
	//invertMatrix4x4(projview, invertedProjview);
	//mat4x4_t invertedProjviewMat;
	//mat_from_array(&invertedProjviewMat, invertedProjview);
	//mul_mat_vec4(&result, &invertedProjviewMat, &vec4_copy(result));

	vec4 half(0.5f);
	vec4 w(result.w);
	div_vec4(&result, &result, &w);
	mul_vec4(&result, &result, &half);
	add_vec4(&result, &result, &half);
	result.x = result.x * window->z + window->x;
	result.y = result.y * window->w + window->y;
	return result;
}

vec4_t unproject(
	const vec4_t* screenCoords,
	const mat4x4_t* model,
	const mat4x4_t* projview,
	const vec4_t* window
) {
	vec4_t result = *screenCoords;
	result.x -= window->x;
	result.y -= window->y;
	result.x /= window->z;
	result.y /= window->w;

	vec4 half(0.5f);
	sub_vec4(&result, &result, &half);
	div_vec4(&result, &result, &half);

	float invertedProjview[16];
	invertMatrix4x4(projview, invertedProjview);
	mat4x4_t invertedProjviewMat;
	mat_from_array(&invertedProjviewMat, invertedProjview);
	vec4 copy = vec4_copy(&result);
	mul_mat_vec4(&result, &invertedProjviewMat, &copy);

	vec4 w(result.w);
	div_vec4(&result, &result, &w);

	return result;
}

/*-------------------------------------------------------------------------------

	getLightForEntity

	Returns a shade factor (0.0-1.0) to shade an entity with, based on
	its surroundings

-------------------------------------------------------------------------------*/

real_t getLightForEntity(real_t x, real_t y)
{
	if ( x < 0 || y < 0 || x >= map.width || y >= map.height )
	{
		return 1.f;
	}
	int u = x;
	int v = y;
	constexpr float div = 1.f / 255.f;
    const auto& l = lightmapSmoothed[(v + 1) + (u + 1) * (map.height + 2)];
    const auto light = (l.x + l.y + l.z) / 3.f;
	return std::min(std::max(0.f, light), 255.f) * div;
}

/*-------------------------------------------------------------------------------

	glDrawVoxel

	Draws a voxel model at the given world coordinates

-------------------------------------------------------------------------------*/

static void fillSmoothLightmap() {
    int v = 0;
    int index = 0;
    int smoothindex = 2 + map.height + 1;
    const int size = map.width * map.height;
    for ( ; index < size; ++index, ++v, ++smoothindex )
    {
        if ( v == map.height ) {
            smoothindex += 2;
            v = 0;
        }
        
#ifndef EDITOR
        static ConsoleVariable<float> cvar_smoothingRate("/lightupdate", 1.f);
        const float smoothingRate = *cvar_smoothingRate;
#else
        const float smoothingRate = 1.f;
#endif
        const float rate = smoothingRate * (1.f / fpsLimit) * 4.f;
        
        auto& d = lightmapSmoothed[smoothindex];
        const auto& s = lightmap[index];
        for (int c = 0; c < 4; ++c) {
            auto& dc = *(&d.x + c);
            const auto& sc = *(&s.x + c);
            const auto diff = sc - dc;
            if (fabsf(diff) < 1.f) { dc += diff; }
            else { dc += diff * rate; }
        }
    }
}

static inline bool testTileOccludes(const map_t& map, int index) {
    if (index < 0 || index > map.width * map.height * MAPLAYERS - MAPLAYERS) {
        return true;
    }
    const Uint64& t0 = *(Uint64*)&map.tiles[index];
    const Uint32& t1 = *(Uint32*)&map.tiles[index + 2];
    return (t0 & 0xffffffff00000000) && (t0 & 0x00000000ffffffff) && t1;
}

static void loadLightmapTexture() {
    // allocate lightmap pixel data
    static std::vector<float> pixels;
    pixels.clear();
    pixels.reserve(map.width * map.height * 4);
    
    // build lightmap texture data
    const float div = 1.f / 255.f;
    const int xoff = MAPLAYERS * map.height;
    const int yoff = MAPLAYERS;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0, index = y * yoff; x < map.width; ++x, index += xoff) {
            if (!testTileOccludes(map, index)) {
                float count = 1.f;
                vec4_t t, total = lightmapSmoothed[(y + 1) + (x + 1) * (map.height + 2)];
                if (!testTileOccludes(map, index + yoff)) { (void)add_vec4(&t, &total, &lightmapSmoothed[(y + 2) + (x + 1) * (map.height + 2)]); total = t; ++count; }
                if (!testTileOccludes(map, index + xoff)) { (void)add_vec4(&t, &total, &lightmapSmoothed[(y + 1) + (x + 2) * (map.height + 2)]); total = t; ++count; }
                if (!testTileOccludes(map, index - yoff)) { (void)add_vec4(&t, &total, &lightmapSmoothed[(y + 0) + (x + 1) * (map.height + 2)]); total = t; ++count; }
                if (!testTileOccludes(map, index - xoff)) { (void)add_vec4(&t, &total, &lightmapSmoothed[(y + 1) + (x + 0) * (map.height + 2)]); total = t; ++count; }
                total.x = (total.x / count) * div;
                total.y = (total.y / count) * div;
                total.z = (total.z / count) * div;
                total.w = 1.f;
                pixels.insert(pixels.end(), {total.x, total.y, total.z, total.w});
            } else {
                pixels.insert(pixels.end(), {0.f, 0.f, 0.f, 1.f});
            }
        }
    }
    
    // load lightmap texture data
    lightmapTexture->loadFloat(pixels.data(), map.width, map.height, true, false);
}

static void updateChunks();

void beginGraphics() {
    // this runs exactly once each graphics frame.
    fillSmoothLightmap();
    loadLightmapTexture();
    updateChunks();
}

void uploadUniforms(Shader& shader, float* proj, float* view, float* mapDims) {
    shader.bind();
    if (proj) glUniformMatrix4fv(shader.uniform("uProj"), 1, false, proj);
    if (view) glUniformMatrix4fv(shader.uniform("uView"), 1, false, view);
    if (mapDims) glUniform2fv(shader.uniform("uMapDims"), 1, mapDims);
    if (mapDims) glUniform1i(shader.uniform("uLightmap"), 1); // lightmap uses texture unit 1
    shader.unbind();
}

constexpr float defaultGamma = 1.f;
constexpr float defaultExposure = 0.5f;
constexpr float defaultAdjustmentRate = 2.f;
constexpr float defaultLimitHigh = 4.f;
constexpr float defaultLimitLow = 0.1f;
#ifndef EDITOR
static ConsoleVariable<bool> cvar_hdrEnabled("/hdr_enabled", true);
static ConsoleVariable<float> cvar_hdrExposure("/hdr_exposure", defaultExposure);
static ConsoleVariable<float> cvar_hdrGamma("/hdr_gamma", defaultGamma);
static ConsoleVariable<float> cvar_hdrAdjustment("/hdr_adjust_rate", defaultAdjustmentRate);
static ConsoleVariable<float> cvar_hdrLimitHigh("/hdr_limit_high", defaultLimitHigh);
static ConsoleVariable<float> cvar_hdrLimitLow("/hdr_limit_low", defaultLimitLow);
#endif

static int oldViewport[4];

void glBeginCamera(view_t* camera)
{
    if (!camera) {
        return;
    }
    
    // setup viewport
#ifdef EDITOR
    const bool hdr = false;
#else
    const bool hdr = *cvar_hdrEnabled;
#endif
    if (hdr) {
        camera->fb.init(camera->winw, camera->winh, GL_LINEAR, GL_LINEAR);
        camera->fb.bindForWriting();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glScissor(0, 0, camera->winw, camera->winh);
    } else {
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
        glScissor(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);
    }
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    
    const float aspect = (real_t)camera->winw / (real_t)camera->winh;

	// setup projection + view matrix (legacy)
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	perspectiveGL(fov, aspect, CLIPNEAR, CLIPFAR);
	const float rotx = camera->vang * 180.f / PI; // get x rotation
	const float roty = (camera->ang - 3.f * PI / 2.f) * 180.f / PI; // get y rotation
	const float rotz = 0.f; // get z rotation
	glRotatef(rotx, 1.f, 0.f, 0.f); // rotate pitch
	glRotatef(roty, 0.f, 1.f, 0.f); // rotate yaw
	glRotatef(rotz, 0.f, 0.f, 1.f); // rotate roll
	glTranslatef(-camera->x * 32.f, camera->z, -camera->y * 32.f); // translates the scene based on camera position
    
    // setup projection + view matrix (shader)
    mat4x4_t proj, view, view2, identity;
    vec4_t translate(-camera->x * 32.f, camera->z, -camera->y * 32.f, 0.f);
    (void)perspective(&proj, fov, aspect, CLIPNEAR, CLIPFAR);
    (void)rotate_mat(&view, &view2, rotx, &identity.x); view2 = view;
    (void)rotate_mat(&view, &view2, roty, &identity.y); view2 = view;
    (void)rotate_mat(&view, &view2, rotz, &identity.z); view2 = view;
    (void)translate_mat(&view, &view2, &translate); view2 = view;

	// lightmap dimensions
	vec4_t mapDims;
	mapDims.x = map.width;
	mapDims.y = map.height;
    
    // set ambient lighting
    if ( camera->globalLightModifierActive ) {
        getLightAtModifier = camera->globalLightModifier;
    }
    else {
        getLightAtModifier = 1.0;
    }
    
	// upload uniforms
    uploadUniforms(voxelShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(voxelBrightShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(voxelDitheredShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(voxelBrightDitheredShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(worldShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(worldBrightShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(worldDarkShader, (float*)&proj, (float*)&view, nullptr);
}

void glEndCamera(view_t* camera)
{
    if (!camera) {
        return;
    }
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    
#ifdef EDITOR
    const bool hdr = false;
    const float hdr_exposure = defaultExposure;
    const float hdr_gamma = defaultGamma;
    const float hdr_adjustment_rate = defaultAdjustmentRate;
    const float hdr_limit_high = defaultLimitHigh;
    const float hdr_limit_low = defaultLimitLow;
#else
    const bool hdr = *cvar_hdrEnabled;
    const float hdr_exposure = *cvar_hdrExposure;
    const float hdr_gamma = *cvar_hdrGamma;
    const float hdr_adjustment_rate = *cvar_hdrAdjustment;
    const float hdr_limit_high = *cvar_hdrLimitHigh;
    const float hdr_limit_low = *cvar_hdrLimitLow;
#endif
    if (hdr) {
        // update viewport
        camera->fb.unbindForWriting();
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh);

        // prepare to read framebuffer
        camera->fb.bindForReading();
        
        // calculate luminance
        auto pixels = (float*)camera->fb.lock();
        if (pixels) {
            vec4_t v(0.f);
            const int size = camera->winw * camera->winh * 4;
            const auto end = pixels + size;
            const int step = camera->winw / 10;
            for (; pixels < end; pixels += step) {
                (void)add_vec4(&v, &v, (vec4_t*)pixels);
            }
            camera->fb.unlock();
            float luminance = std::max({v.x, v.y, v.z});
            luminance = luminance / (size / step);
            camera->luminance += (luminance - camera->luminance) / (fpsLimit * hdr_adjustment_rate);
        }
        const float exposure = std::min(std::max(hdr_limit_low, hdr_exposure / camera->luminance), hdr_limit_high);
        const float gamma = hdr_gamma;
        
        // blit framebuffer
        camera->fb.hdrBlit(gamma, exposure);
        camera->fb.unbindForReading();
        
        // revert viewport
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    } else {
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    }
}

// hsv values:
// x = [0-360]
// y = [0-100]
// z = [0-100]
// w = [0-1]
static vec4_t* HSVtoRGB(vec4_t* result, const vec4_t* hsv){
    float h = fmodf(hsv->x, 360.f);
    if (h < 0.f) {
        h += 360.f;
    }
    const float s = hsv->y / 100.f;
    const float v = hsv->z / 100.f;
    const float C = s * v;
    const float X = C * (1.f - fabsf(fmodf(h/60.f, 2.f) - 1.f));
    const float m = v - C;
    float r, g, b;
    if (h >= 0 && h < 60) {
        r = C; g = X; b = 0;
    }
    else if(h >= 60 && h < 120) {
        r = X; g = C; b = 0;
    }
    else if(h >= 120 && h < 180) {
        r = 0; g = C; b = X;
    }
    else if(h >= 180 && h < 240) {
        r = 0; g = X; b = C;
    }
    else if(h >= 240 && h < 300) {
        r = X; g = 0; b = C;
    }
    else {
        r = C; g = 0; b = X;
    }
    result->x = r + m;
    result->y = g + m;
    result->z = b + m;
    result->w = hsv->w;
    return result;
}

void glDrawVoxel(view_t* camera, Entity* entity, int mode) {
	if (!entity) {
		return;
	}

	// assign model
    voxel_t* model = nullptr;
    int modelindex = -1;
#ifndef EDITOR
	static ConsoleVariable<int> cvar_forceModel("/forcemodel", -1, "force all voxel models to use a specific index");
	modelindex = *cvar_forceModel;
#endif
	if (modelindex < 0) {
		modelindex = entity->sprite;
	}
	if (modelindex >= 0 && modelindex < nummodels) {
		if (models[modelindex] != NULL) {
			model = models[modelindex];
		} else {
			model = models[0];
		}
	} else {
		model = models[0];
		modelindex = 0;
	}

	if (!model || model == models[0]) {
		return; // don't draw green balls
	}
    
	if (mode == REALCOLORS) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	if (entity->flags[OVERDRAW] || (entity->monsterEntityRenderAsTelepath == 1 && !intro) 
		|| modelindex == FOLLOWER_SELECTED_PARTICLE
		|| modelindex == FOLLOWER_TARGET_PARTICLE ) {
		glDepthRange(0, 0.1);
	}

	bool highlightEntity = false;
	bool highlightEntityFromParent = false;
	int player = -1;
	for (player = 0; player < MAXPLAYERS; ++player) {
		if (&cameras[player] == camera) {
			break;
		}
	}
	highlightEntity = entity->bEntityHighlightedForPlayer(player);
    if (!highlightEntity && (modelindex == 184 || modelindex == 585 || modelindex == 216)) { // lever base/chest lid
		Entity* parent = uidToEntity(entity->parent);
		if (parent && parent->bEntityHighlightedForPlayer(player)) {
			entity->highlightForUIGlow = parent->highlightForUIGlow;
			highlightEntityFromParent = true;
			highlightEntity = highlightEntityFromParent;
		}
	}

	bool doGrayScale = false;
	real_t grayScaleFactor = 0.0;
	if (entity->grayscaleGLRender > 0.001) {
		doGrayScale = true;
		grayScaleFactor = entity->grayscaleGLRender;
	}

	// get shade factor
    real_t s = 1.0;
	if (!entity->flags[BRIGHT]) {
		if (!entity->flags[OVERDRAW]) {
			if (entity->monsterEntityRenderAsTelepath == 1 && !intro) {
				if (camera->globalLightModifierActive) {
					s = camera->globalLightModifierEntities;
				}
			} else {
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
		} else {
			s = getLightForEntity(camera->x, camera->y);
		}
	}

	if ( camera->globalLightModifierActive && entity->monsterEntityRenderAsTelepath == 0 ) {
		s *= camera->globalLightModifier;
	}
    
#ifndef EDITOR
    static ConsoleVariable<bool> cvar_legacyVoxelDraw("/legacyvoxel", false);
	const bool legacyVoxels = *cvar_legacyVoxelDraw;
#else
	const bool legacyVoxels = false;
#endif
    
    if (legacyVoxels) {
        // old rendering (fixed function)
        // setup model matrix
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();
        GLfloat rotx, roty, rotz;
        if (entity->flags[OVERDRAW]) {
            glTranslatef(camera->x * 32, -camera->z, camera->y * 32); // translates the scene based on camera position
            rotx = 0; // get x rotation
            roty = 360.0 - camera->ang * 180.0 / PI; // get y rotation
            rotz = 360.0 - camera->vang * 180.0 / PI; // get z rotation
            glRotatef(roty, 0, 1, 0); // rotate yaw
            glRotatef(rotz, 0, 0, 1); // rotate pitch
            glRotatef(rotx, 1, 0, 0); // rotate roll
        }
        rotx = entity->roll * 180.0 / PI; // get x rotation
        roty = 360.0 - entity->yaw * 180.0 / PI; // get y rotation
        rotz = 360.0 - entity->pitch * 180.0 / PI; // get z rotation
        glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
        glRotatef(roty, 0, 1, 0); // rotate yaw
        glRotatef(rotz, 0, 0, 1); // rotate pitch
        glRotatef(rotx, 1, 0, 0); // rotate roll
        glTranslatef(entity->focalx * 2, -entity->focalz * 2, entity->focaly * 2);
        glScalef(entity->scalex, entity->scalez, entity->scaley);
        
        // OpenGL 2.1 does not support vertex array objects
        //glBindVertexArray(polymodels[modelindex].va);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].vbo);
        glVertexPointer(3, GL_FLOAT, 0, nullptr);
        if (mode == REALCOLORS) {
            glEnableClientState(GL_COLOR_ARRAY);
            /*if (entity->flags[USERFLAG2]) {
                if (entity->behavior == &actMonster && (entity->isPlayerHeadSprite() ||
                    modelindex == 467 || !monsterChangesColorWhenAlly(nullptr, entity))) {
                    if ( doGrayScale ) {
                        //glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].grayscale_colors);
                    } else {
                        glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
                    }
                } else {
                    if ( doGrayScale ) {
                        //glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].grayscale_colors_shifted);
                    } else {
                        //glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors_shifted);
                    }
                }
            } else {
                if (doGrayScale) {
                    //glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].grayscale_colors);
                } else {
                    glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
                }
            }*/
            glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
            glColorPointer(3, GL_FLOAT, 0, nullptr);
            GLfloat params_col[4] = { static_cast<GLfloat>(s), static_cast<GLfloat>(s), static_cast<GLfloat>(s), 1.f };
            if (highlightEntity) {
                glEnable(GL_LIGHTING);
                glEnable(GL_LIGHT1);
                if (!highlightEntityFromParent) {
                    entity->highlightForUIGlow = (0.05 * (entity->ticks % 41));
                }
                real_t highlight = entity->highlightForUIGlow;
                if (highlight > 1.0) {
                    highlight = 1.0 - (highlight - 1.0);
                }
                GLfloat ambient[4] = {
                    static_cast<GLfloat>(.15 + highlight * .15),
                    static_cast<GLfloat>(.15 + highlight * .15),
                    static_cast<GLfloat>(.15 + highlight * .15),
                    1.f };
                glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params_col);
                glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
                glEnable(GL_COLOR_MATERIAL);
            } else {
                glEnable(GL_LIGHTING);
                glEnable(GL_COLOR_MATERIAL);
                glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params_col);
            }
        } else {
            GLfloat uidcolors[4];
            Uint32 uid = entity->getUID();
            uidcolors[0] = ((Uint8)(uid)) / 255.f;
            uidcolors[1] = ((Uint8)(uid >> 8)) / 255.f;
            uidcolors[2] = ((Uint8)(uid >> 16)) / 255.f;
            uidcolors[3] = ((Uint8)(uid >> 24)) / 255.f;
            glColor4f(uidcolors[0], uidcolors[1], uidcolors[2], uidcolors[3]);
        }
        glDrawArrays(GL_TRIANGLES, 0, (int)(3 * polymodels[modelindex].numfaces));
        if (mode == REALCOLORS) {
            glDisable(GL_COLOR_MATERIAL);
            glDisable(GL_LIGHTING);
            if (highlightEntity) {
                glDisable(GL_LIGHT1);
            }
            glDisableClientState(GL_COLOR_ARRAY);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
        //glBindVertexArray(0);
        glPopMatrix();
    } else {
        // new rendering (shader)
        mat4x4_t m, t, i;
        vec4_t v;
        
        // model matrix
        float rotx, roty, rotz;
        if (entity->flags[OVERDRAW]) {
            v = vec4(camera->x * 32, -camera->z, camera->y * 32, 0);
            (void)translate_mat(&m, &t, &v); t = m;
            rotx = 0; // roll
            roty = 360.0 - camera->ang * 180.0 / PI; // yaw
            rotz = 360.0 - camera->vang * 180.0 / PI; // pitch
            (void)rotate_mat(&m, &t, roty, &i.y); t = m; // yaw
            (void)rotate_mat(&m, &t, rotz, &i.z); t = m; // pitch
            (void)rotate_mat(&m, &t, rotx, &i.x); t = m; // roll
        }
        rotx = entity->roll * 180.0 / PI; // roll
        roty = 360.0 - entity->yaw * 180.0 / PI; // yaw
        rotz = 360.0 - entity->pitch * 180.0 / PI; // pitch
        v = vec4(entity->x * 2.f, -entity->z * 2.f - 1, entity->y * 2.f, 0.f);
        (void)translate_mat(&m, &t, &v); t = m;
        (void)rotate_mat(&m, &t, roty, &i.y); t = m; // yaw
        (void)rotate_mat(&m, &t, rotz, &i.z); t = m; // pitch
        (void)rotate_mat(&m, &t, rotx, &i.x); t = m; // roll
        v = vec4(entity->focalx * 2.f, -entity->focalz * 2.f, entity->focaly * 2.f, 0.f);
        (void)translate_mat(&m, &t, &v); t = m;
        v = vec4(entity->scalex, entity->scaley, entity->scalez, 0.f);
        (void)scale_mat(&m, &t, &v); t = m;

#ifndef EDITOR
        const bool fullbright = *cvar_fullBright;
#else
        const bool fullbright = false;
#endif
        const bool useLightmap = mode == REALCOLORS && !entity->flags[BRIGHT] && !fullbright;
        
#ifndef EDITOR
		static ConsoleVariable<bool> cvar_testDithering("/test_dithering", false);
		auto& shader = *cvar_testDithering ?
			(useLightmap ? voxelDitheredShader : voxelBrightDitheredShader):
			(useLightmap ? voxelShader : voxelBrightShader);
#else
		auto& shader = useLightmap ?
			voxelShader : voxelBrightShader;
#endif
		shader.bind();
        
        // upload shader variables
        glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m); // model matrix
        if (mode == REALCOLORS) {
            mat4x4_t remap(1.f);
            if (doGrayScale) {
                remap.x.x = 1.f / 3.f;
                remap.x.y = 1.f / 3.f;
                remap.x.z = 1.f / 3.f;
                remap.y.x = 1.f / 3.f;
                remap.y.y = 1.f / 3.f;
                remap.y.z = 1.f / 3.f;
                remap.z.x = 1.f / 3.f;
                remap.z.y = 1.f / 3.f;
                remap.z.z = 1.f / 3.f;
            }
            else if (entity->flags[USERFLAG2]) {
                if (entity->behavior != &actMonster || (!entity->isPlayerHeadSprite() &&
                    modelindex != 467 && monsterChangesColorWhenAlly(nullptr, entity))) {
                    // certain allies use G/B/R color map
                    remap = mat4x4_t(0.f);
                    remap.x.y = 1.f;
                    remap.y.z = 1.f;
                    remap.z.x = 1.f;
                }
            }
#ifndef EDITOR
            static ConsoleVariable<bool> cvar_rainbowTest("/rainbowtest", false);
            if (*cvar_rainbowTest) {
                remap = mat4x4_t(0.f);
                
                const auto period = TICKS_PER_SECOND * 3; // 3 seconds
                const auto time = (ticks % period) / (real_t)period; // [0-1]
                const auto amp = 360.0;
                
                vec4_t hsv;
                hsv.y = 100.f; // saturation
                hsv.z = 100.f; // value
                hsv.w = 0.f;   // unused
                
                hsv.x = time * amp;
                HSVtoRGB(&remap.x, &hsv); // red
                
                hsv.x = time * amp + 120;
                HSVtoRGB(&remap.y, &hsv); // green
                
                hsv.x = time * amp + 240;
                HSVtoRGB(&remap.z, &hsv); // blue
            }
#endif
            glUniformMatrix4fv(shader.uniform("uColorRemap"), 1, false, (float*)&remap);
            
            const GLfloat light[4] = { (float)getLightAtModifier, (float)getLightAtModifier, (float)getLightAtModifier, 1.f };
            glUniform4fv(shader.uniform("uLightColor"), 1, light);
            
            if (highlightEntity) {
                if (!highlightEntityFromParent) {
                    entity->highlightForUIGlow = (0.05 * (entity->ticks % 41));
                }
                real_t highlight = entity->highlightForUIGlow;
                if (highlight > 1.0) {
                    highlight = 1.0 - (highlight - 1.0);
                }
                GLfloat ambient[4] = {
                    static_cast<GLfloat>((highlight - 0.5) * .1),
                    static_cast<GLfloat>((highlight - 0.5) * .1),
                    static_cast<GLfloat>((highlight - 0.5) * .1),
                    0.f };
                glUniform4fv(shader.uniform("uColorAdd"), 1, ambient);
            } else {
                constexpr GLfloat add[4] = { 0.f, 0.f, 0.f, 0.f };
                glUniform4fv(shader.uniform("uColorAdd"), 1, add);
            }
        } else {
            mat4x4_t empty(0.f);
            glUniformMatrix4fv(shader.uniform("uColorRemap"), 1, false, (float*)&empty);
            
            constexpr GLfloat light[4] = { 0.f, 0.f, 0.f, 0.f };
            glUniform4fv(shader.uniform("uLightColor"), 1, light);
            
            GLfloat uidcolors[4];
            Uint32 uid = entity->getUID();
            uidcolors[0] = ((Uint8)(uid)) / 255.f;
            uidcolors[1] = ((Uint8)(uid >> 8)) / 255.f;
            uidcolors[2] = ((Uint8)(uid >> 16)) / 255.f;
            uidcolors[3] = ((Uint8)(uid >> 24)) / 255.f;
            glUniform4fv(shader.uniform("uColorAdd"), 1, uidcolors);
        }
        
        // draw
        //glDisable(GL_DEPTH_TEST);
        
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        
        glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        
        glDrawArrays(GL_TRIANGLES, 0, (int)(3 * polymodels[modelindex].numfaces));
        
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        
        //glEnable(GL_DEPTH_TEST);
        //shader.unbind();
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDepthRange(0, 1);
}

/*-------------------------------------------------------------------------------

	glDrawSprite

	Draws a 2D sprite to represent an object in 3D

-------------------------------------------------------------------------------*/
SDL_Surface* glTextSurface(std::string text, GLuint* outTextId)
{
	SDL_Surface* image = sprites[0];
	GLuint textureId = texid[(long int)sprites[0]->userdata];
	char textToRetrieve[128];
	strncpy(textToRetrieve, text.c_str(), 127);
	textToRetrieve[std::min(static_cast<int>(strlen(text.c_str())), 127)] = '\0';

	if ( (image = ttfTextHashRetrieve(ttfTextHash, textToRetrieve, ttf12, true)) != NULL )
	{
		textureId = texid[(long int)image->userdata];
	}
	else
	{
		// create the text outline surface
		TTF_SetFontOutline(ttf12, 2);
		SDL_Color sdlColorBlack = { 0, 0, 0, 255 };
		image = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorBlack);

		// create the text surface
		TTF_SetFontOutline(ttf12, 0);
		SDL_Color sdlColorWhite = { 255, 255, 255, 255 };
		SDL_Surface* textSurf = TTF_RenderUTF8_Blended(ttf12, textToRetrieve, sdlColorWhite);

		// combine the surfaces
		SDL_Rect pos;
		pos.x = 2;
		pos.y = 2;
		pos.h = 0;
		pos.w = 0;

		SDL_BlitSurface(textSurf, NULL, image, &pos);
		SDL_FreeSurface(textSurf);
		// load the text outline surface as a GL texture
		allsurfaces[imgref] = image;
		allsurfaces[imgref]->userdata = (void *)((long int)imgref);
		glLoadTexture(allsurfaces[imgref], imgref);
		imgref++;
		// store the surface in the text surface cache
		if ( !ttfTextHashStore(ttfTextHash, textToRetrieve, ttf12, true, image) )
		{
			printlog("warning: failed to store text outline surface with imgref %d\n", imgref - 1);
		}
		textureId = texid[(long int)image->userdata];
	}
	if ( outTextId )
	{
		*outTextId = textureId;
	}
	return image;
}

#ifndef EDITOR
static ConsoleVariable<GLfloat> cvar_enemybarDepthRange("/enemybar_depth_range", 0.5);
#endif

bool glDrawEnemyBarSprite(view_t* camera, int mode, void* enemyHPBarDetails, bool doVisibilityCheckOnly)
{
	if ( !enemyHPBarDetails ) 
	{
		return false;
	}
	auto enemybar = (EnemyHPDamageBarHandler::EnemyHPDetails*)enemyHPBarDetails;
	SDL_Surface* sprite = enemybar->worldSurfaceSprite;
	if ( !sprite || !enemybar->worldTexture )
	{
		return false;
	}

	// assign texture
	TempTexture* tex = enemybar->worldTexture;
	if (!doVisibilityCheckOnly)
	{
		if (mode == REALCOLORS)
		{
			if (tex)
			{
				tex->bind();
			}
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

	// setup projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR);
	GLfloat rotx = camera->vang * 180 / PI; // get x rotation
	GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
	GLfloat rotz = 0; // get z rotation
	glRotatef(rotx, 1, 0, 0); // rotate pitch
	glRotatef(roty, 0, 1, 0); // rotate yaw
	glRotatef(rotz, 0, 0, 1); // rotate roll
	glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position

	GLfloat projectionMatrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}

	// translate sprite and rotate towards camera
	real_t height = enemybar->worldZ - 6;
	glTranslatef(enemybar->worldX * 2,
		-height * 2 - 1, 
		enemybar->worldY * 2);

	real_t tangent = 180 - camera->ang * (180 / PI);
	glRotatef(tangent, 0, 1, 0);

	float scaleFactor = 0.08;
	glScalef(scaleFactor, scaleFactor, scaleFactor);

	/*if ( entity && entity->flags[OVERDRAW] )
	{
	}*/
#ifndef EDITOR
	glDepthRange(0, *cvar_enemybarDepthRange);
#endif // !EDITOR

	// get shade factor
	if ( mode == REALCOLORS )
	{
		glColor4f(1.f, 1.f, 1.f, enemybar->animator.fadeOut / 100.f);
	}
	else
	{
		glColor4ub((Uint8)(enemybar->enemy_uid), (Uint8)(enemybar->enemy_uid >> 8), (Uint8)(enemybar->enemy_uid >> 16), (Uint8)(enemybar->enemy_uid >> 24));
	}

	GLfloat modelViewMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix);

	vec4_t worldCoords[4]; // 0, 0, 0, 1.f is centre of rendered quad
	worldCoords[0].x = enemybar->screenDistance; // top left
	worldCoords[0].y = sprite->h / 2;
	worldCoords[0].z = sprite->w / 2;
	worldCoords[0].w = 1.f;
	worldCoords[1].x = enemybar->screenDistance; // top right
	worldCoords[1].y = sprite->h / 2;
	worldCoords[1].z = -sprite->w / 2;
	worldCoords[1].w = 1.f;
	worldCoords[2].x = enemybar->screenDistance; // bottom left
	worldCoords[2].y = -sprite->h / 2;
	worldCoords[2].z = sprite->w / 2;
	worldCoords[2].w = 1.f;
	worldCoords[3].x = enemybar->screenDistance; // bottom right
	worldCoords[3].y = -sprite->h / 2;
	worldCoords[3].z = -sprite->w / 2;
	worldCoords[3].w = 1.f;

	mat4x4_t projMat4;
	mat_from_array(&projMat4, projectionMatrix);

	mat4x4_t modelMat4;
	mat_from_array(&modelMat4, modelViewMatrix);

	vec4_t window(camera->winx, camera->winy, camera->winw, camera->winh);
	mat4x4_t projViewModel4;
	mul_mat(&projViewModel4, &projMat4, &modelMat4);

	mat4x4_t identityMatrix = mat4x4(1.f);
	bool anyVertexVisible = false;
	if ( enemybar->enemy_hp > 0 ) // don't update if dead target.
	{
		enemybar->glWorldOffsetY = 0.f;
		vec4_t screenCoordinates = project(&worldCoords[0], &identityMatrix, &projViewModel4, &window); // top-left coord
		if ( screenCoordinates.y >= (window.w + window.y) && projViewModel4.w.z >= 0 ) // above camera limit
		{
			float pixelOffset = abs(screenCoordinates.y - (window.w + window.y));
			screenCoordinates.y -= pixelOffset;
			vec4_t worldCoords2 = unproject(&screenCoordinates, &identityMatrix, &projViewModel4, &window); // convert back into worldCoords
			enemybar->glWorldOffsetY = (worldCoords[0].y - worldCoords2.y);
		}
		else if ( false ) // code to check lower bounds of camera - in case needed.
		{
			screenCoordinates = project(&worldCoords[2], &identityMatrix, &projViewModel4, &window); // bottom-left coord
			if ( screenCoordinates.y < (window.y) && projViewModel4.w.z >= 0 ) // below camera limit
			{
				float pixelOffset = abs(window.y - (screenCoordinates.y));
				screenCoordinates.y -= pixelOffset;
				vec4_t worldCoords2 = unproject(&screenCoordinates, &identityMatrix, &projViewModel4, &window); // convert back into worldCoords
				enemybar->glWorldOffsetY = -(worldCoords[2].y - worldCoords2.y);
			}
		}
	}

	if ( abs(enemybar->glWorldOffsetY) <= 0.001 && abs(enemybar->screenDistance) <= 0.001 )
	{
		// rotate up/down pitch towards camera, requires offset to be 0
		real_t tangent2 = camera->vang * 180 / PI; 
		glRotatef(tangent2, 0, 0, 1);
	}

	//	bool visibleX = res[i].x >= window.x && res[i].x < (window.z + window.x) && projViewModel4.w.z >= 0;
	//	bool visibleY = res[i].y >= window.y && res[i].y < (window.w + window.y) && projViewModel4.w.z >= 0;
	//	//printTextFormatted(font16x16_bmp, 8, 8 + i * 16, "%d: visibleX: %d | visibleY: %d", i, visibleX, visibleY);
	//	if ( visibleX && visibleY )
	//	{
	//		anyVertexVisible = true;
	//	}
	//	//SDL_Rect resPos{ res.x, window.w - res.y, 4, 4 };
	//	//drawRect(&resPos, 0xFFFFFF00, 255);
	//}

	int drawOffsetY = (enemybar->worldSurfaceSpriteStatusEffects ? -enemybar->worldSurfaceSpriteStatusEffects->h / 2 : 0);
	drawOffsetY += enemybar->glWorldOffsetY;

	if ( !doVisibilityCheckOnly )
	{
		// draw quad
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(enemybar->screenDistance, GLfloat(sprite->h / 2) - drawOffsetY, GLfloat(sprite->w / 2));
		glTexCoord2f(0, 1);
		glVertex3f(enemybar->screenDistance, GLfloat(-sprite->h / 2) - drawOffsetY, GLfloat(sprite->w / 2));
		glTexCoord2f(1, 1);
		glVertex3f(enemybar->screenDistance, GLfloat(-sprite->h / 2) - drawOffsetY, GLfloat(-sprite->w / 2));
		glTexCoord2f(1, 0);
		glVertex3f(enemybar->screenDistance, GLfloat(sprite->h / 2) - drawOffsetY, GLfloat(-sprite->w / 2));
		glEnd();
	}

	glDepthRange(0, 1);
    
    if (mode == REALCOLORS) {
        glDisable(GL_BLEND);
    }

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
    
    glDisable(GL_ALPHA_TEST);

	//printTextFormatted(font16x16_bmp, 8, 8 + 4 * 16, "Any vertex visible: %d", anyVertexVisible);
	return anyVertexVisible;
}

void glDrawWorldDialogueSprite(view_t* camera, void* worldDialogue, int mode)
{
#ifndef EDITOR
	if ( !worldDialogue )
	{
		return;
	}
	auto dialogue = (Player::WorldUI_t::WorldTooltipDialogue_t::Dialogue_t*)worldDialogue;
	if ( dialogue->alpha <= 0.0 )
	{
		return;
	}
	SDL_Surface* sprite = nullptr;
	if ( !dialogue->dialogueTooltipSurface )
	{
		sprite = dialogue->blitDialogueTooltip();
	}
	else
	{
		sprite = dialogue->dialogueTooltipSurface;
	}
	if ( !sprite )
	{
		return;
	}

	// assign texture
	TempTexture* tex = nullptr;
	tex = new TempTexture();
	if (sprite) {
		tex->load(sprite, false, true);
		if (mode == REALCOLORS)
		{
			tex->bind();
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

	// setup projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR);
	GLfloat rotx = camera->vang * 180 / PI; // get x rotation
	GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
	GLfloat rotz = 0; // get z rotation
	glRotatef(rotx, 1, 0, 0); // rotate pitch
	glRotatef(roty, 0, 1, 0); // rotate yaw
	glRotatef(rotz, 0, 0, 1); // rotate roll
	glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(dialogue->x * 2, -(dialogue->z + dialogue->animZ) * 2 - 1, dialogue->y * 2);
	real_t tangent = 180 - camera->ang * (180 / PI);
	glRotatef(tangent, 0, 1, 0);

	real_t tangent2 = camera->vang * 180 / PI; // face camera pitch
	glRotatef(tangent2, 0, 0, 1);

	float scale = static_cast<float>(dialogue->drawScale);
	if ( splitscreen )
	{
		scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale_splitscreen / 100.f) - 1.f));
	}
	else
	{
		scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale / 100.f) - 1.f));
	}
	glScalef(scale, scale, scale);

	glDepthRange(0, .6);

	// get shade factor
	glColor4f(1.f, 1.f, 1.f, dialogue->alpha);


	// draw quad
	if ( sprite ) {
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(0, sprite->h / 2, sprite->w / 2);
		glTexCoord2f(0, 1);
		glVertex3f(0, -sprite->h / 2, sprite->w / 2);
		glTexCoord2f(1, 1);
		glVertex3f(0, -sprite->h / 2, -sprite->w / 2);
		glTexCoord2f(1, 0);
		glVertex3f(0, sprite->h / 2, -sprite->w / 2);
		glEnd();
	}

	glDepthRange(0, 1);

	if ( tex ) {
		delete tex;
		tex = nullptr;
	}
    if ( mode == REALCOLORS )
    {
        glDisable(GL_BLEND);
    }

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
    
    glDisable(GL_ALPHA_TEST);
#endif
}

void glDrawWorldUISprite(view_t* camera, Entity* entity, int mode)
{
#ifndef EDITOR
	real_t s = 1;

	if ( !entity || intro )
	{
		return;
	}

	int player = -1;
	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		if ( entity->worldTooltipIgnoreDrawing != 0 )
		{
			return;
		}
		for ( player = 0; player < MAXPLAYERS; ++player )
		{
			if ( &cameras[player] == camera )
			{
				break;
			}
		}
		if ( player >= 0 && player < MAXPLAYERS )
		{
			if ( entity->worldTooltipPlayer != player )
			{
				return;
			}
			if ( entity->worldTooltipActive == 0 && entity->worldTooltipFadeDelay == 0 )
			{
				return;
			}
		}
		else
		{
			return;
		}
		if ( !uidToEntity(entity->parent) )
		{
			return;
		}
	}

	// assign texture
	SDL_Surface* sprite = nullptr;
	TempTexture* tex = nullptr;
	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		Entity* parent = uidToEntity(entity->parent);
		if ( parent && parent->behavior == &actItem 
			&& (multiplayer != CLIENT 
				|| (multiplayer == CLIENT && (parent->itemReceivedDetailsFromServer != 0 || parent->skill[10] != 0))) )
		{
			Item* item = newItemFromEntity(uidToEntity(entity->parent), true);
			if ( !item )
			{
				return;
			}

			sprite = players[player]->worldUI.worldTooltipItem.blitItemWorldTooltip(item);

			free(item);
			item = nullptr;
		}

		tex = new TempTexture();
		if (sprite) {
		    tex->load(sprite, false, true);
		    if ( mode == REALCOLORS )
		    {
			    tex->bind();
		    }
		    else
		    {
			    glBindTexture(GL_TEXTURE_2D, 0);
		    }
		}
		//glBindTexture(GL_TEXTURE_2D, texid[sprite->refcount]);
	}
	else
	{
		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != NULL )
			{
				sprite = sprites[entity->sprite];
			}
			else
			{
				sprite = sprites[0];
			}
		}
		else
		{
			sprite = sprites[0];
		}
	}
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

	// setup projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR);
	GLfloat rotx = camera->vang * 180 / PI; // get x rotation
	GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
	GLfloat rotz = 0; // get z rotation
	glRotatef(rotx, 1, 0, 0); // rotate pitch
	glRotatef(roty, 0, 1, 0); // rotate yaw
	glRotatef(rotz, 0, 0, 1); // rotate roll
	glTranslatef(-camera->x * 32, camera->z, -camera->y * 32); // translates the scene based on camera position

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	if (mode == REALCOLORS)
	{
		glEnable(GL_BLEND);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if ( !entity->flags[OVERDRAW] || entity->flags[OVERDRAW] )
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);

		real_t tangent2 = camera->vang * 180 / PI; // face camera pitch
		glRotatef(tangent2, 0, 0, 1);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}

	float scale = Player::WorldUI_t::WorldTooltipItem_t::WorldItemSettings_t::scaleMod;
	if ( splitscreen )
	{
		scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale_splitscreen / 100.f) - 1.f));
	}
	else
	{
		scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale / 100.f) - 1.f));
	}
	glScalef(static_cast<GLfloat>(entity->scalex + scale), 
		static_cast<GLfloat>(entity->scalez + scale), 
		static_cast<GLfloat>(entity->scaley + scale));

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0.1, 0.2);
	}
	else
	{
		glDepthRange(0, .6);
	}

	// get shade factor
	if ( mode == REALCOLORS )
	{
		if ( !entity->flags[BRIGHT] )
		{
			if ( !entity->flags[OVERDRAW] )
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}

			if ( camera->globalLightModifierActive )
			{
				s *= camera->globalLightModifier;
			}

			glColor4f(s, s, s, 1);
		}
		else
		{
			if ( entity->behavior == &actSpriteWorldTooltip )
			{
				glColor4f(1.f, 1.f, 1.f, entity->worldTooltipAlpha * Player::WorldUI_t::WorldTooltipItem_t::WorldItemSettings_t::opacity);
			}
			else if ( camera->globalLightModifierActive )
			{
				glColor4f(camera->globalLightModifier, camera->globalLightModifier, camera->globalLightModifier, 1.f);
			}
			else
			{
				glColor4f(1.f, 1.f, 1.f, 1.f);
			}
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	if (sprite) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(0, 0);
	    glVertex3f(0, sprite->h / 2, sprite->w / 2);
	    glTexCoord2f(0, 1);
	    glVertex3f(0, -sprite->h / 2, sprite->w / 2);
	    glTexCoord2f(1, 1);
	    glVertex3f(0, -sprite->h / 2, -sprite->w / 2);
	    glTexCoord2f(1, 0);
	    glVertex3f(0, sprite->h / 2, -sprite->w / 2);
	    glEnd();
	}

	glDepthRange(0, 1);

	if ( entity->behavior == &actSpriteWorldTooltip )
	{
		if ( tex ) {
			delete tex;
			tex = nullptr;
		}
	}
    
    if (mode == REALCOLORS)
    {
        glDisable(GL_BLEND);
    }

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
    
    glDisable(GL_ALPHA_TEST);
#endif
}

void glDrawSprite(view_t* camera, Entity* entity, int mode)
{
	SDL_Surface* sprite;
	//int x, y;
	real_t s = 1;
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

	// setup model matrix
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	if (entity->flags[OVERDRAW])
	{
		glTranslatef(camera->x * 32, -camera->z, camera->y * 32); // translates the scene based on camera position
		float rotx = 0; // get x rotation
		float roty = 360.0 - camera->ang * 180.0 / PI; // get y rotation
		float rotz = 360.0 - camera->vang * 180.0 / PI; // get z rotation
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate pitch
		glRotatef(rotx, 1, 0, 0); // rotate roll
	}
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}

	// assign texture
	if ( entity->sprite >= 0 && entity->sprite < numsprites )
	{
		if ( sprites[entity->sprite] != NULL )
		{
			sprite = sprites[entity->sprite];
		}
		else
		{
			sprite = sprites[0];
		}
	}
	else
	{
		sprite = sprites[0];
	}

	if ( mode == REALCOLORS )
	{
		glBindTexture(GL_TEXTURE_2D, texid[(long int)sprite->userdata]);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if (!entity->flags[OVERDRAW])
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0, 0.1);
	}

	// get shade factor
	if ( mode == REALCOLORS )
	{
		if (!entity->flags[BRIGHT])
		{
			if (!entity->flags[OVERDRAW])
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}

			if ( camera->globalLightModifierActive )
			{
				s *= camera->globalLightModifier;
			}

			glColor4f(s, s, s, 1);
		}
		else
		{
			if ( camera->globalLightModifierActive )
			{
				glColor4f(camera->globalLightModifier, camera->globalLightModifier, camera->globalLightModifier, 1.f);
			}
			else
			{
				glColor4f(1.f, 1.f, 1.f, 1.f);
			}
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, sprite->h / 2, sprite->w / 2);
	glTexCoord2f(0, 1);
	glVertex3f(0, -sprite->h / 2, sprite->w / 2);
	glTexCoord2f(1, 1);
	glVertex3f(0, -sprite->h / 2, -sprite->w / 2);
	glTexCoord2f(1, 0);
	glVertex3f(0, sprite->h / 2, -sprite->w / 2);
	glEnd();
	glDepthRange(0, 1);
	glPopMatrix();
    
    if ( mode == REALCOLORS )
    {
        glDisable(GL_BLEND);
    }
    
    glDisable(GL_ALPHA_TEST);
}

#ifndef EDITOR
static ConsoleVariable<GLfloat> cvar_dmgSpriteDepthRange("/dmg_sprite_depth_range", 0.49);
#endif // !EDITOR

void glDrawSpriteFromImage(view_t* camera, Entity* entity, std::string text, int mode)
{
	if ( text.empty() == true || !entity )
	{
		return;
	}

	Uint32 color = makeColor(255, 255, 255, 255);
	if ( entity->behavior == &actDamageGib && text[0] == '+' )
	{
#ifndef EDITOR
		color = hudColors.characterSheetGreen;
#endif // !EDITOR
	}
	else if ( entity->behavior == &actSpriteNametag )
	{
		color = entity->skill[1];
	}
	auto rendered_text = Text::get(text.c_str(), "fonts/pixel_maz.ttf#32#2",
		color, makeColor(0, 0, 0, 255));
	auto textureId = rendered_text->getTexID();

	// assign texture
	if (mode == REALCOLORS)
	{
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

	// setup model matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	if (entity->flags[OVERDRAW])
	{
		glTranslatef(camera->x * 32, -camera->z, camera->y * 32); // translates the scene based on camera position
		float rotx = 0; // get x rotation
		float roty = 360.0 - camera->ang * 180.0 / PI; // get y rotation
		float rotz = 360.0 - camera->vang * 180.0 / PI; // get z rotation
		glRotatef(roty, 0, 1, 0); // rotate yaw
		glRotatef(rotz, 0, 0, 1); // rotate pitch
		glRotatef(rotx, 1, 0, 0); // rotate roll
	}
	if ( mode == REALCOLORS )
	{
		glEnable(GL_BLEND);
	}

	// translate sprite and rotate towards camera
	//double tangent = atan2( entity->y-camera->y*16, camera->x*16-entity->x ) * (180/PI);
	glTranslatef(entity->x * 2, -entity->z * 2 - 1, entity->y * 2);
	if ( !entity->flags[OVERDRAW] )
	{
		real_t tangent = 180 - camera->ang * (180 / PI);
		glRotatef(tangent, 0, 1, 0);
	}
	else
	{
		real_t tangent = 180;
		glRotatef(tangent, 0, 1, 0);
	}
	glScalef(entity->scalex, entity->scalez, entity->scaley);

	if ( entity->flags[OVERDRAW] )
	{
		glDepthRange(0, 0.1);
	}
	else
	{
		if ( entity->behavior == &actDamageGib )
		{
#ifndef EDITOR
			glDepthRange(0, *cvar_dmgSpriteDepthRange);
#endif // !EDITOR
		}
		else if ( entity->behavior != &actSpriteNametag )
		{
			glDepthRange(0, 0.98);
		}
		else if ( entity->behavior == &actSpriteNametag )
		{
			glDepthRange(0, 0.52);
		}
	}

	// get shade factor
	real_t s;
	if ( mode == REALCOLORS )
	{
		if ( !entity->flags[BRIGHT] )
		{
			if ( !entity->flags[OVERDRAW] )
			{
				s = getLightForEntity(entity->x / 16, entity->y / 16);
			}
			else
			{
				s = getLightForEntity(camera->x, camera->y);
			}
			glColor4f(s, s, s, 1);
		}
		else
		{
			glColor4f(1.f, 1.f, 1.f, 1);
		}
	}
	else
	{
		Uint32 uid = entity->getUID();
		glColor4ub((Uint8)(uid), (Uint8)(uid >> 8), (Uint8)(uid >> 16), (Uint8)(uid >> 24));
	}

	// draw quad
	GLfloat w = static_cast<GLfloat>(rendered_text->getWidth());
	GLfloat h = static_cast<GLfloat>(rendered_text->getHeight());
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(0, h / 2, w / 2);
	glTexCoord2f(0, 1);
	glVertex3f(0, -h / 2, w / 2);
	glTexCoord2f(1, 1);
	glVertex3f(0, -h / 2, -w / 2);
	glTexCoord2f(1, 0);
	glVertex3f(0, h / 2, -w / 2);
	glEnd();

	glDepthRange(0, 1);
	glPopMatrix();
    
    if ( mode == REALCOLORS )
    {
        glDisable(GL_BLEND);
    }
    glDisable(GL_ALPHA_TEST);
}

/*-------------------------------------------------------------------------------

	getLightAt

	returns the light shade factor for the vertex at the given x/y point

-------------------------------------------------------------------------------*/

static vec4_t getLightAt(const int x, const int y)
{
#if !defined(EDITOR) && !defined(NDEBUG)
    static ConsoleVariable<bool> cvar("/fullbright", false);
    if (*cvar)
    {
        return 1.0;
    }
#endif
	const int index = (y + 1) + (x + 1) * (map.height + 2);

	vec4_t l(0.f), r(0.f);
    (void)add_vec4(&r, &l, &lightmapSmoothed[index - 1 - (map.height + 2)]); l = r;
    (void)add_vec4(&r, &l, &lightmapSmoothed[index - (map.height + 2)]); l = r;
    (void)add_vec4(&r, &l, &lightmapSmoothed[index - 1]); l = r;
    (void)add_vec4(&r, &l, &lightmapSmoothed[index]); l = r;
    (void)pow_vec4(&r, &l, getLightAtModifier); l = r;
    static const vec4_t added(getLightAtAdder);
    (void)add_vec4(&r, &l, &added); l = r;
	float div = 1.f / (255.f * 4.f);
	l.x = std::min(std::max(0.f, l.x * div), 1.f);
    l.y = std::min(std::max(0.f, l.y * div), 1.f);
    l.z = std::min(std::max(0.f, l.z * div), 1.f);
    l.w = std::min(std::max(0.f, l.w * div), 1.f);
	return l;
}

/*-------------------------------------------------------------------------------

	glDrawWorld

	Draws the current map from the given camera point

-------------------------------------------------------------------------------*/

static bool shouldDrawClouds(const map_t& map, int* cloudtile = nullptr) {
    bool clouds = false;
    if (cloudtile) {
        *cloudtile = 77; // hell clouds
    }
    if ((!strncmp(map.name, "Hell", 4) || map.skybox != 0) && smoothlighting) {
        clouds = true;
        if (cloudtile) {
            if (strncmp(map.name, "Hell", 4)) {
                // not a hell map, custom clouds
                *cloudtile = map.skybox;
            }
        }
    }
    return clouds;
}

#include "ui/Image.hpp"

std::vector<Chunk> chunks;

void glDrawWorld(view_t* camera, int mode)
{
#ifndef EDITOR
    static ConsoleVariable<bool> cvar_skipDrawWorld("/skipdrawworld", false);
    if (*cvar_skipDrawWorld) {
        return;
    }
#endif

    // determine whether we should draw clouds, and their texture
    int cloudtile;
    const bool clouds = shouldDrawClouds(map, &cloudtile);
    
#ifndef EDITOR
    const bool fullbright = *cvar_fullBright;
#else
    const bool fullbright = false;
#endif
    
    // bind shader
    auto& shader = fullbright ?
        (mode == REALCOLORS ? worldBrightShader : worldDarkShader):
        (mode == REALCOLORS ? worldShader : worldDarkShader);
    shader.bind();
    
    // upload uniforms
    const GLfloat light[4] = { (float)getLightAtModifier, (float)getLightAtModifier, (float)getLightAtModifier, 1.f };
    glUniform4fv(shader.uniform("uLightColor"), 1, light);
    
    // draw tile chunks
    int index = 0;
    const int dim = 4; // size of chunk in tiles
    const int xoff = (map.height / dim) + ((map.height % dim) ? 1 : 0);
    const int yoff = 1;
    for (auto& chunk : chunks) {
        for (int x = chunk.x; x < chunk.x + chunk.w; ++x) {
            for (int y = chunk.y; y < chunk.y + chunk.h; ++y) {
                if (camera->vismap[y + x * map.height]) {
                    if (chunk.isDirty(map)) {
                        const auto ceiling = !shouldDrawClouds(map);
                        chunk.build(map, ceiling, chunk.x, chunk.y, chunk.w, chunk.h);
                        
                        // build neighbor chunks as well (in-case there are shared walls)
                        if (chunk.x + chunk.w < map.width) { // build east chunk
                            auto& nChunk = chunks[index + xoff];
                            nChunk.build(map, ceiling, nChunk.x, nChunk.y, nChunk.w, nChunk.h);
                        }
                        if (chunk.y + chunk.h < map.height) { // build south chunk
                            auto& nChunk = chunks[index + yoff];
                            nChunk.build(map, ceiling, nChunk.x, nChunk.y, nChunk.w, nChunk.h);
                        }
                        if (chunk.x > 0) { // build west chunk
                            auto& nChunk = chunks[index - xoff];
                            nChunk.build(map, ceiling, nChunk.x, nChunk.y, nChunk.w, nChunk.h);
                        }
                        if (chunk.y > 0) { // build north chunk
                            auto& nChunk = chunks[index - yoff];
                            nChunk.build(map, ceiling, nChunk.x, nChunk.y, nChunk.w, nChunk.h);
                        }
                    }
                    chunk.draw();
                    goto next;
                }
            }
        }
    next:
        ++index;
    }
    
    // unbind shader
    shader.unbind();
    
    // draw clouds
    // do this after drawing walls/floors/etc because that way most of it can
    // fail the depth test, improving fill rate.
    if (clouds && mode == REALCOLORS) {
        // setup projection
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        perspectiveGL(fov, (real_t)camera->winw / (real_t)camera->winh, CLIPNEAR, CLIPFAR);
        GLfloat rotx = camera->vang * 180 / PI; // get x rotation
        GLfloat roty = (camera->ang - 3 * PI / 2) * 180 / PI; // get y rotation
        GLfloat rotz = 0; // get z rotation
        glRotatef(rotx, 1, 0, 0); // rotate pitch
        glRotatef(roty, 0, 1, 0); // rotate yaw
        glRotatef(rotz, 0, 0, 1); // rotate roll
        
        // setup model matrix
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        
        const float size = CLIPFAR * 16.f;
        const float htex_size = size / 64.f;
        const float ltex_size = size / 32.f;
        const float high_scroll = (float)(ticks % 60) / 60.f;
        const float low_scroll = (float)(ticks % 120) / 120.f;
        
        // bind cloud texture
        glBindTexture(GL_TEXTURE_2D, texid[(long int)tiles[cloudtile]->userdata]);

        // first (higher) sky layer
        glBegin( GL_QUADS );
        glColor4f(1.f, 1.f, 1.f, (float)getLightAtModifier);
        glTexCoord2f(high_scroll, high_scroll);
        glVertex3f(-size, 65.f, -size);

        glTexCoord2f(htex_size + high_scroll, high_scroll);
        glVertex3f(size, 65.f, -size);

        glTexCoord2f(htex_size + high_scroll, htex_size + high_scroll);
        glVertex3f(size, 65.f, size);

        glTexCoord2f(high_scroll, htex_size + high_scroll);
        glVertex3f(-size, 65.f, size);

        // second (closer) sky layer
        glColor4f(1.f, 1.f, 1.f, (float)getLightAtModifier * .5f);
        glTexCoord2f(low_scroll, low_scroll);
        glVertex3f(-size, 64.f, -size);

        glTexCoord2f(ltex_size + low_scroll, low_scroll);
        glVertex3f(size, 64.f, -size);

        glTexCoord2f(ltex_size + low_scroll, ltex_size + low_scroll);
        glVertex3f(size, 64.f, size);

        glTexCoord2f(low_scroll, ltex_size + low_scroll);
        glVertex3f(-size, 64.f, size);
        glEnd();

        // pop matrices
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}

static int dirty = 1;
static int oldx = 0, oldy = 0;
static unsigned int oldpix = 0;

unsigned int GO_GetPixelU32(int x, int y, view_t& camera)
{
	if(!dirty && (oldx==x) && (oldy==y))
		return oldpix;
    
    main_framebuffer.unbindForWriting();
    
	if(dirty) {
#ifdef PANDORA
		// Pandora fbo
		if((xres==800) && (yres==480)) {
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);
		}
#endif
		// generate object buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glBeginCamera(&camera);
		glDrawWorld(&camera, ENTITYUIDS);
		drawEntities3D(&camera, ENTITYUIDS);
		glEndCamera(&camera);
	}

	GLubyte pixel[4];
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixel);
	oldpix = pixel[0] + (((Uint32)pixel[1]) << 8) + (((Uint32)pixel[2]) << 16) + (((Uint32)pixel[3]) << 24);
#ifdef PANDORA
	if((dirty) && (xres==800) && (yres==480)) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
#else
	main_framebuffer.bindForWriting();
#endif
	dirty = 0;
	return oldpix;
}

void GO_SwapBuffers(SDL_Window* screen)
{
	dirty = 1;

#ifdef PANDORA
	bool bBlit = !(xres==800 && yres==480);
	int vp_old[4];
	if(bBlit) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glGetIntegerv(GL_VIEWPORT, vp_old);
		glViewport(0, 0, 800, 480);
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho(0, 800, 480, 0, 1, -1);
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		glBindTexture(GL_TEXTURE_2D, fbo_tex);
		glColor4f(1,1,1,1);

		glBegin(GL_QUADS);
		 glTexCoord2f(0,yres/1024.0f); glVertex2f(0,0);
		 glTexCoord2f(0, 0); glVertex2f(0,480);
		 glTexCoord2f(xres/1024.0f, 0); glVertex2f(800,480);
		 glTexCoord2f(xres/1024.0f, yres/1024.0f); glVertex2f(800,0);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if(bBlit) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_fbo);
		glViewport(vp_old[0], vp_old[1], vp_old[2], vp_old[3]);
	}

	return;
#endif
    
    main_framebuffer.unbindForWriting();
	main_framebuffer.bindForReading();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	main_framebuffer.blit(vidgamma);
	SDL_GL_SwapWindow(screen);
	main_framebuffer.bindForWriting();
}

static float chunkTexCoords[3];
static inline void makeTexCoords(float x, float y, float tile) {
#define fdivf(A, B) A / B
    constexpr float dim = 32.f;
    chunkTexCoords[0] = floorf(fmodf(tile, dim) + x) / dim;
    chunkTexCoords[1] = floorf(fdivf(tile, dim) + y) / dim;
}

void Chunk::build(const map_t& map, bool ceiling, int startX, int startY, int w, int h) {
    std::vector<float> positions;
    std::vector<float> texcoords;
    std::vector<float> colors;
    
    positions.reserve(1200);
    texcoords.reserve(800);
    colors.reserve(1200);
    
    // determine ceiling texture
    int mapceilingtile = 50;
    if (map.flags[MAP_FLAG_CEILINGTILE] > 0 && map.flags[MAP_FLAG_CEILINGTILE] < numtiles) {
        mapceilingtile = map.flags[MAP_FLAG_CEILINGTILE];
    }
    
    int index2 = 0;
    const int endX = std::min((int)map.width, startX + w);
    const int endY = std::min((int)map.height, startY + h);
    
    // copy tiles
    if (this->tiles) {
        delete[] this->tiles;
    }
    
    this->x = startX;
    this->y = startY;
    this->w = endX - startX;
    this->h = endY - startY;
    const int sizeOfTiles = this->w * this->h * MAPLAYERS;
    this->tiles = new Sint32[sizeOfTiles];
    
    for (int x = startX; x < endX; ++x) {
        for (int y = startY; y < endY; ++y) {
            for (int z = 0; z < MAPLAYERS + 1; ++z) {
                const int index = z + y * MAPLAYERS + x * map.height * MAPLAYERS;

                // build walls
                if (z >= 0 && z < MAPLAYERS) {
                    assert(index2 < sizeOfTiles);
                    this->tiles[index2] = map.tiles[index];
                    ++index2;
                    
                    // skip empty tiles
                    if (map.tiles[index] == 0) {
                        continue;
                    }

                    // skip special transparent tile
                    if (map.tiles[index] == TRANSPARENT_TILE) {
                        continue;
                    }

                    // select texture
                    float tile = mapceilingtile;
                    if (map.tiles[index] >= 0 && map.tiles[index] < numtiles) {
                        if (map.tiles[index] >= 22 && map.tiles[index] < 30) {
                            // water special case
                            tile = 267 + map.tiles[index] - 22;
                        }
                        else if (map.tiles[index] >= 64 && map.tiles[index] < 72) {
                            // lava special case
                            tile = 285 + map.tiles[index] - 64;
                        }
                        else {
                            tile = map.tiles[index];
                        }
                    }
                    
                    // determine lava texture
                    bool lavaTexture = false;
                    if ((tile >= 64 && tile < 72) ||
                        (tile >= 129 && tile < 135) ||
                        (tile >= 136 && tile < 139) ||
                        (tile >= 285 && tile < 293) ||
                        (tile >= 294 && tile < 302)) {
                        lavaTexture = true;
                    }

                    // draw east wall
                    const int easter = index + MAPLAYERS * map.height;
                    if (x == map.width - 1 || !map.tiles[easter] || map.tiles[easter] == TRANSPARENT_TILE) {
                        if (z) { // normal wall
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 0.f});
                        }
                        else { // darkened pit
                            for (int j = z - 1; j <= z; ++j) {
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(0.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(1.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 0.f});
                            }
                        }
                    }

                    // draw south wall
                    const int souther = index + MAPLAYERS;
                    if (y == map.height - 1 || !map.tiles[souther] || map.tiles[souther] == TRANSPARENT_TILE) {
                        if (z) { // normal wall
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 32.f});
                        }
                        else { // darkened pit
                            for (int j = z - 1; j <= z; ++j) {
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(0.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 32.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(1.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 32.f});
                            }
                        }
                    }

                    // draw west wall
                    const int wester = index - MAPLAYERS * map.height;
                    if (x == 0 || !map.tiles[wester] || map.tiles[wester] == TRANSPARENT_TILE) {
                        if (z) { // normal wall
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 32.f});
                        }
                        else { // darkened pit
                            for (int j = z - 1; j <= z; ++j) {
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(0.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 32.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(1.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 32.f});
                            }
                        }
                    }

                    // draw north wall
                    const int norther = index - MAPLAYERS;
                    if (y == 0 || !map.tiles[norther] || map.tiles[norther] == TRANSPARENT_TILE) {
                        if (z) { // normal wall
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, z * 32.f - 16.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 48.f, y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, z * 32.f - 16.f, y * 32.f + 0.f});
                        }
                        else { // darkened pit
                            for (int j = z - 1; j <= z; ++j) {
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(0.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(0.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 32.f, j * 32.f - 16.f, y * 32.f + 0.f});
                                
                                if (j == z - 1) { colors.insert(colors.end(), {0.f, 0.f, 0.f}); }
                                else { colors.insert(colors.end(), {1.f, 1.f, 1.f}); }
                                makeTexCoords(1.f, 1.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 48.f, y * 32.f + 0.f});
                                
                                colors.insert(colors.end(), {1.f, 1.f, 1.f});
                                makeTexCoords(1.f, 0.f, tile);
                                texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                                positions.insert(positions.end(), {x * 32.f + 0.f, j * 32.f - 16.f, y * 32.f + 0.f});
                            }
                        }
                    }
                }
                
                // build floor and ceiling
                {
                    // select floor/ceiling texture
                    float tile = mapceilingtile;
                    if (z >= 0 && z < MAPLAYERS) {
                        if (map.tiles[index] < 0 || map.tiles[index] >= numtiles) {
                            tile = 0;
                        } else {
                            tile = map.tiles[index];
                        }
                    }
                    
                    // determine lava texture
                    bool lavaTexture = false;
                    if ((tile >= 64 && tile < 72) ||
                        (tile >= 129 && tile < 135) ||
                        (tile >= 136 && tile < 139) ||
                        (tile >= 285 && tile < 293) ||
                        (tile >= 294 && tile < 302)) {
                        lavaTexture = true;
                    }
                    
                    // build floor
                    if (z < OBSTACLELAYER) {
                        if (!map.tiles[index + 1] || map.tiles[index + 1] == TRANSPARENT_TILE) {
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, -16.f - 32.f * abs(z), y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, -16.f - 32.f * abs(z), y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, -16.f - 32.f * abs(z), y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, -16.f - 32.f * abs(z), y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, -16.f - 32.f * abs(z), y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, -16.f - 32.f * abs(z), y * 32.f + 0.f});
                        }
                    }
                    
                    // build ceiling
                    else if (z > OBSTACLELAYER && (ceiling || z < MAPLAYERS)) {
                        if (!map.tiles[index - 1] || map.tiles[index - 1] == TRANSPARENT_TILE) {
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, 16.f + 32.f * abs(z - 2), y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, 16.f + 32.f * abs(z - 2), y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, 16.f + 32.f * abs(z - 2), y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 0.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, 16.f + 32.f * abs(z - 2), y * 32.f + 0.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(1.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 32.f, 16.f + 32.f * abs(z - 2), y * 32.f + 32.f});
                            
                            colors.insert(colors.end(), {1.f, 1.f, 1.f});
                            makeTexCoords(0.f, 1.f, tile);
                            texcoords.insert(texcoords.end(), &chunkTexCoords[0], &chunkTexCoords[2]);
                            positions.insert(positions.end(), {x * 32.f + 0.f, 16.f + 32.f * abs(z - 2), y * 32.f + 32.f});
                        }
                    }
                }
            }
        }
    }
    indices = (int)texcoords.size() / 2;
    buildBuffers(positions, texcoords, colors);
    //printlog("built chunk with %d tris", indices);
}

void Chunk::buildBuffers(const std::vector<float>& positions, const std::vector<float>& texcoords, const std::vector<float>& colors) {
    // create buffers
    if (!vbo_positions) {
        glGenBuffers(1, &vbo_positions);
    }
    if (!vbo_texcoords) {
        glGenBuffers(1, &vbo_texcoords);
    }
    if (!vbo_colors) {
        glGenBuffers(1, &vbo_colors);
    }
    
    // upload positions
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDisableVertexAttribArray(0);
    
    // upload texcoords
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDisableVertexAttribArray(1);
    
    // upload colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDisableVertexAttribArray(2);
}

void Chunk::destroyBuffers() {
    if (vbo_positions) {
        glDeleteBuffers(1, &vbo_positions);
        vbo_positions = 0;
    }
    if (vbo_texcoords) {
        glDeleteBuffers(1, &vbo_texcoords);
        vbo_texcoords = 0;
    }
    if (vbo_colors) {
        glDeleteBuffers(1, &vbo_colors);
        vbo_colors = 0;
    }
    if (tiles) {
        delete[] tiles;
        tiles = nullptr;
    }
    indices = 0;
}

void Chunk::draw() {
    if (!indices) {
        return;
    }
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glDrawArrays(GL_TRIANGLES, 0, indices);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool Chunk::isDirty(const map_t& map) {
    if (!tiles) {
        return true;
    }
    for (int u = 0; u < w; ++u) {
        const int off0 = u * h * MAPLAYERS;
        const int off1 = (u + x) * map.height * MAPLAYERS + y * MAPLAYERS;
        const int size = MAPLAYERS * h;
        assert(off0 + size <= w * h * MAPLAYERS);
        assert(off1 + size <= map.width * map.height * MAPLAYERS);
        if (memcmp(&tiles[off0], &map.tiles[off1], size * sizeof(Sint32))) {
            return true;
        }
    }
    return false;
}

void clearChunks() {
    chunks.clear();
}

void createChunks() {
    constexpr int chunkSize = 4;
    chunks.reserve((map.width / chunkSize + 1) * (map.height / chunkSize + 1));
    for (int x = 0; x < map.width; x += chunkSize) {
        for (int y = 0; y < map.height; y += chunkSize) {
            chunks.emplace_back();
            auto& chunk = chunks.back();
            chunk.build(map, !shouldDrawClouds(map), x, y, chunkSize, chunkSize);
        }
    }
}

void updateChunks() {
    static int cachedW = -1;
    static int cachedH = -1;
    if (cachedW != map.width || cachedH != map.height) {
        cachedW = map.width;
        cachedH = map.height;
        clearChunks();
        createChunks();
    }
}

#ifndef EDITOR
static ConsoleCommand ccmd_build_test_chunk("/build_test_chunk", "builds a chunk covering the whole level",
    [](int argc, const char* argv[]){
    chunks.emplace_back();
    auto& chunk = chunks.back();
    chunk.build(map, !shouldDrawClouds(map), 0, 0, map.width, map.height);
    });
#endif
