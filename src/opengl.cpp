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
#include "init.hpp"
#include "ui/Image.hpp"

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

    GL_CHECK_ERR(glFrustum(-fW, fW, -fH, fH, zNear, zFar));
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

mat4x4_t* ortho(mat4x4_t* result, float left, float right, float bot, float top, float near, float far) {
    *result = mat4x4(1.f);
    result->x.x = 2.f / (right - left);
    result->y.y = 2.f / (top - bot);
    result->z.z = 2.f / (far - near);
    result->w.x = -1.f;
    result->w.y = -1.f;
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

bool invertMatrix4x4(mat4x4_t* result, const mat4x4_t* m)
{
	float inv[16];

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

	float det = m->x.x * inv[0] + m->x.y * inv[4] + m->x.z * inv[8] + m->x.w * inv[12];

    if (det == 0.f) {
        return false;
    }

	det = 1.f / det;

    float* out = (float*)result;
    for (int i = 0; i < 16; ++i) {
        out[i] = inv[i] * det;
    }

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

	vec4 half(0.5f);
	vec4 w(result.w);
	div_vec4(&result, &result, &w);
	mul_vec4(&result, &result, &half);
	add_vec4(&result, &result, &half);
	result.x = result.x * window->z + window->x;
	result.y = result.y * window->w + window->y;
	return result;
}

ClipResult project_clipped(
    const vec4_t* world,
    const mat4x4_t* model,
    const mat4x4_t* projview,
    const vec4_t* window
) {
    ClipResult clipResult;
    vec4_t& result = clipResult.clipped_coords;
    result = *world; result.w = 1.f;
    clipResult.isBehind = false;

    vec4 copy;
    copy = vec4_copy(&result); mul_mat_vec4(&result, model, &copy);
    copy = vec4_copy(&result); mul_mat_vec4(&result, projview, &copy);

    float w = result.w;
    if ( w < CLIPNEAR ) {
        w = CLIPNEAR;
        result.x *= CLIPFAR;
        result.y *= CLIPFAR;
        if ( result.z < -w ) {
            clipResult.direction = ClipResult::Direction::Behind;
            clipResult.isBehind = true;
        }
    }
    if ( result.x > w ) {
        const float factor = w / result.x;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Right;
    }
    else if ( result.x < -w ) {
        const float factor = -w / result.x;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Left;
    }
    if ( result.y > w ) {
        const float factor = w / result.y;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Top;
    }
    else if ( result.y < -w ) {
        const float factor = -w / result.y;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Bottom;
    }
    if ( result.z > w ) {
        const float factor = w / result.z;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Front;
    }
    else if ( result.z < -w ) {
        const float factor = -w / result.z;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Behind;
        clipResult.isBehind = true;
    }

    vec4 half(0.5f);
    vec4 div(w);
    div_vec4(&result, &result, &div);
    mul_vec4(&result, &result, &half);
    add_vec4(&result, &result, &half);
    result.x = result.x * window->z + window->x;
    result.y = result.y * window->w + window->y;
    return clipResult;
}

ClipResult project_clipped2(
    const vec4_t* world,
    const mat4x4_t* model,
    const mat4x4_t* projview,
    const vec4_t* window
) {
    ClipResult clipResult;
    vec4_t& result = clipResult.clipped_coords;
    result = *world; result.w = 1.f;
    clipResult.isBehind = false;

    vec4 copy;
    copy = vec4_copy(&result); mul_mat_vec4(&result, model, &copy);
    copy = vec4_copy(&result); mul_mat_vec4(&result, projview, &copy);

    float w = result.w;
    if ( w < CLIPNEAR ) {
        w = CLIPNEAR;
        if ( result.z < -w ) {
            clipResult.direction = ClipResult::Direction::Behind;
            clipResult.isBehind = true;
        }
    }
    if ( result.x > w ) {
        const float factor = w / result.x;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Right;
    }
    else if ( result.x < -w ) {
        const float factor = -w / result.x;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Left;
    }
    if ( result.y > w ) {
        const float factor = w / result.y;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Top;
    }
    else if ( result.y < -w ) {
        const float factor = -w / result.y;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Bottom;
    }
    if ( result.z > w ) {
        const float factor = w / result.z;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Front;
    }
    else if ( result.z < -w ) {
        const float factor = -w / result.z;
        pow_vec4(&result, &result, factor);
        clipResult.direction = ClipResult::Direction::Behind;
        clipResult.isBehind = true;
    }

    vec4 half(0.5f);
    vec4 div(w);
    div_vec4(&result, &result, &div);
    mul_vec4(&result, &result, &half);
    add_vec4(&result, &result, &half);
    result.x = result.x * window->z + window->x;
    result.y = result.y * window->w + window->y;
    return clipResult;
}

vec4_t unproject(
	const vec4_t* screenCoords,
	const mat4x4_t* model,
	const mat4x4_t* projview,
	const vec4_t* window
) {
	vec4_t result = *screenCoords;
	result.x = (result.x - window->x) / window->z;
	result.y = (result.y - window->y) / window->w;

	vec4 half(0.5f);
	sub_vec4(&result, &result, &half);
	div_vec4(&result, &result, &half);

    vec4 copy;
    mat4x4_t inv;
    invertMatrix4x4(&inv, projview);
	copy = vec4_copy(&result); mul_mat_vec4(&result, &inv, &copy);
    
    vec4 w(result.w);
    div_vec4(&result, &result, &w);

	return result;
}

/*-------------------------------------------------------------------------------

	glDrawVoxel

	Draws a voxel model at the given world coordinates

-------------------------------------------------------------------------------*/

static void fillSmoothLightmap(int which, map_t& map) {
#ifndef EDITOR
    if ( &map == &CompendiumEntries.compendiumMap )
    {
        return;
    }
#endif

    auto lightmap = lightmaps[which].data();
    auto lightmapSmoothed = lightmapsSmoothed[which].data();
    
    constexpr float epsilon = 1.f;
    constexpr float defaultSmoothRate = 4.f;
#ifndef EDITOR
    static ConsoleVariable<float> cvar_smoothingRate("/lightupdate", defaultSmoothRate);
    const float smoothingRate = *cvar_smoothingRate;
#else
    const float smoothingRate = defaultSmoothRate;
#endif
    const float rate = smoothingRate * (1.f / fpsLimit);
    
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
        
        auto& d = lightmapSmoothed[smoothindex];
        const auto& s = lightmap[index];
        for (int c = 0; c < 4; ++c) {
            auto& dc = *(&d.x + c);
            const auto& sc = *(&s.x + c);
            const auto diff = sc - dc;
            if (fabsf(diff) < epsilon) { dc += diff; }
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

static void loadLightmapTexture(int which, map_t& map) {
    auto lightmapSmoothed = lightmapsSmoothed[which].data();
    
    // allocate lightmap pixel data
    static std::vector<float> pixels;
    pixels.clear();
    pixels.reserve(map.width * map.height * 4);
    
#ifdef EDITOR
    const bool fullbright = false;
#else
    const bool fullbright = (&map == &CompendiumEntries.compendiumMap) ? true :// compendium virtual map is always fullbright
        (conductGameChallenges[CONDUCT_CHEATS_ENABLED] ? *cvar_fullBright : false);
#endif
    
    // build lightmap texture data
    const float div = 1.f / 255.f;
    if (fullbright) {
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                pixels.insert(pixels.end(), {1.f, 1.f, 1.f, 1.f});
            }
        }
    } else {
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
    }
    
    // load lightmap texture data
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE1));
    lightmapTexture[which]->loadFloat(pixels.data(), map.width, map.height, true, false);
    lightmapTexture[which]->bind();
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE0));
}

static void updateChunks();

void beginGraphics() {
    // this runs exactly once each graphics frame.
    updateChunks();
}

#ifndef EDITOR
ConsoleVariable<float> cvar_fogDistance("/fog_distance", 0.f);
ConsoleVariable<Vector4> cvar_fogColor("/fog_color", {0.f, 0.f, 0.f, 0.f});
#endif

static void uploadUniforms(Shader& shader, float* proj, float* view, float* mapDims) {
    shader.bind();
    if (proj) { GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, false, proj)); }
    if (view) { GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uView"), 1, false, view)); }
    if (mapDims) { GL_CHECK_ERR(glUniform2fv(shader.uniform("uMapDims"), 1, mapDims)); }
    
#ifdef EDITOR
    float fogDistance = 0.f;
    float fogColor[4] = { 1.f, 1.f, 1.f, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uFogColor"), 1, fogColor));
    GL_CHECK_ERR(glUniform1f(shader.uniform("uFogDistance"), fogDistance));
#else
    if (shader == spriteUIShader) {
        float fogDistance = 0.f;
        float fogColor[4] = { 1.f, 1.f, 1.f, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uFogColor"), 1, fogColor));
        GL_CHECK_ERR(glUniform1f(shader.uniform("uFogDistance"), fogDistance));
    } else {
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uFogColor"), 1, (float*)&*cvar_fogColor));
        GL_CHECK_ERR(glUniform1f(shader.uniform("uFogDistance"), *cvar_fogDistance));
    }
#endif
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

static void uploadLightUniforms(view_t* camera, Shader& shader, Entity* entity, int mode, bool remap) {
    const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));
    if (mode == REALCOLORS) {
        if (remap) {
            bool doGrayScale = false;
            real_t grayScaleFactor = 0.0;
            if (entity->grayscaleGLRender > 0.001) {
                doGrayScale = true;
                grayScaleFactor = entity->grayscaleGLRender;
            }
            
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
                if ((entity->behavior != &actMonster /*&& entity->noColorChangeAllyLimb < 0.01*/) 
                    || monsterChangesColorWhenAlly(nullptr, entity)) {
                    // certain allies use G/B/R color map
                    remap = mat4x4_t(0.f);
                    remap.x.y = 1.f;
                    remap.y.z = 1.f;
                    remap.z.x = 1.f;

                    //remap.x.x = 0.8f; - desaturate option
                    //remap.y.y = 0.8f;
                    //remap.z.z = 0.8f;
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
            GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uColorRemap"), 1, false, (float*)&remap));
        }

        int player = -1;
        for ( player = 0; player < MAXPLAYERS; ++player ) {
            if ( &cameras[player] == camera ) {
                break;
            }
        }

        bool telepathy =
#ifdef EDITOR
            false;
#else
            entity->monsterEntityRenderAsTelepath && player >= 0 && player < MAXPLAYERS
            && players[player] && players[player]->entity
            && stats[player]->mask&& stats[player]->mask->type == TOOL_BLINDFOLD_TELEPATHY;
#endif
        if ( telepathy ) {
            const GLfloat factor[4] = { 1.f, 1.f, 1.f, 1.f, };
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));

            Vector4 defaultLight{ 0.1f, 0.1f, 0.25f, 1.f };
#ifndef EDITOR
            static ConsoleVariable<Vector4> cvar_lightColor("/telepath_color", defaultLight);
            const auto& light = *cvar_lightColor;
#else
            const auto& light = defaultLight;
#endif
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, (float*)&light));
        } else {
            const GLfloat light[4] = {
                (float)getLightAtModifier,
                (float)getLightAtModifier,
                (float)getLightAtModifier,
                1.f,
            };
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, light));
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, (float*)&entity->lightBonus));
        }

        // highlighting
        bool highlightEntity = false;
        bool highlightEntityFromParent = false;
        highlightEntity = entity->bEntityHighlightedForPlayer(player);
        if (!highlightEntity) {
            Entity* parent = uidToEntity(entity->parent);
            if (parent && parent->bEntityHighlightedForPlayer(player)) {
#ifndef EDITOR
                if ( parent->isInertMimic() )
                {
                    entity->highlightForUIGlow = (0.05 * (entity->ticks % 41));
                }
                else
#endif
                {
                    entity->highlightForUIGlow = parent->highlightForUIGlow;
                }
                highlightEntityFromParent = true;
                highlightEntity = highlightEntityFromParent;
            }
        }
        if (highlightEntity) {
            if (!highlightEntityFromParent) {
                entity->highlightForUIGlow = (0.05 * (entity->ticks % 41));
            }
            float highlight = entity->highlightForUIGlow;
            if (highlight > 1.f) {
                highlight = 1.f - (highlight - 1.f);
            }
            const GLfloat add[4] = {
                (highlight - .5f) * .05f,
                (highlight - .5f) * .05f,
                (highlight - .5f) * .05f,
                0.f };
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, add));
        } else {
            constexpr GLfloat add[4] = { 0.f, 0.f, 0.f, 0.f };
            GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, add));
        }
    } else {
        if (remap) {
            mat4x4_t empty(0.f);
            GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uColorRemap"), 1, false, (float*)&empty));
        }
        
        constexpr GLfloat light[4] = { 0.f, 0.f, 0.f, 0.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, light));
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
        
        GLfloat uidcolors[4];
        Uint32 uid = entity->getUID();
        uidcolors[0] = ((Uint8)(uid)) / 255.f;
        uidcolors[1] = ((Uint8)(uid >> 8)) / 255.f;
        uidcolors[2] = ((Uint8)(uid >> 16)) / 255.f;
        uidcolors[3] = ((Uint8)(uid >> 24)) / 255.f;
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, uidcolors));
    }
}

constexpr Vector4 defaultBrightness = {1.f, 1.f, 1.f, 1.f};
constexpr float defaultGamma = 0.75f;           // default gamma level: 75%
constexpr float defaultExposure = 0.5f;         // default exposure level: 50%
constexpr float defaultAdjustmentRate = 0.1f;   // how fast your eyes adjust
constexpr float defaultLimitHigh = 4.f;         // your aperture can increase to see something 4 times darker.
constexpr float defaultLimitLow = 0.1f;         // your aperture can decrease to see something 10 times brighter.
constexpr float defaultLumaRed = 0.2126f;       // how much to weigh red light for luma (ITU 709)
constexpr float defaultLumaGreen = 0.7152f;     // how much to weigh green light for luma (ITU 709)
constexpr float defaultLumaBlue = 0.0722f;      // how much to weigh blue light for luma (ITU 709)
#ifndef NINTENDO
constexpr bool defaultMultithread = true;       // use multiple workers to collect luminance samples
constexpr float defaultSamples = 16384;         // how many samples (pixels) to gather from the framebuffer for average scene luminance
#else
constexpr bool defaultMultithread = false;
constexpr float defaultSamples = 4096;
#endif
#ifdef EDITOR
bool hdrEnabled = true;
#else
ConsoleVariable<Vector4> cvar_hdrBrightness("/hdr_brightness", defaultBrightness);
static ConsoleVariable<bool> cvar_hdrMultithread("/hdr_multithread", defaultMultithread);
static ConsoleVariable<float> cvar_hdrExposure("/hdr_exposure", defaultExposure);
static ConsoleVariable<float> cvar_hdrGamma("/hdr_gamma", defaultGamma);
static ConsoleVariable<float> cvar_hdrAdjustment("/hdr_adjust_rate", defaultAdjustmentRate);
static ConsoleVariable<float> cvar_hdrLimitHigh("/hdr_limit_high", defaultLimitHigh);
static ConsoleVariable<float> cvar_hdrLimitLow("/hdr_limit_low", defaultLimitLow);
static ConsoleVariable<int> cvar_hdrSamples("/hdr_samples", defaultSamples);
static ConsoleVariable<Vector4> cvar_hdrLuma("/hdr_luma", Vector4{defaultLumaRed, defaultLumaGreen, defaultLumaBlue, 0.f});
bool hdrEnabled = true;
#endif

static int oldViewport[4];

void glBeginCamera(view_t* camera, bool useHDR, map_t& map)
{
    if (!camera) {
        return;
    }
    
    // setup viewport
#ifdef EDITOR
    const bool hdr = useHDR;
    const auto fog_color = Vector4{0.f, 0.f, 0.f, 0.f};
#else
    const bool hdr = useHDR ? *MainMenu::cvar_hdrEnabled : false;
    auto fog_color = *cvar_fogColor;
    fog_color.x *= fog_color.w;
    fog_color.y *= fog_color.w;
    fog_color.z *= fog_color.w;
    fog_color.w = 1.f;
#endif
    
    if (hdr) {
        const int numFbs = sizeof(view_t::fb) / sizeof(view_t::fb[0]);
        const int fbIndex = camera->drawnFrames % numFbs;
        camera->fb[fbIndex].init(camera->winw, camera->winh, GL_LINEAR, GL_LINEAR);
        camera->fb[fbIndex].bindForWriting();
        GL_CHECK_ERR(glClearColor(fog_color.x, fog_color.y, fog_color.z, fog_color.w));
        GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK_ERR(glScissor(0, 0, camera->winw, camera->winh));
    } else {
        GL_CHECK_ERR(glGetIntegerv(GL_VIEWPORT, oldViewport));
        GL_CHECK_ERR(glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh));
        GL_CHECK_ERR(glScissor(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh));
    }
    GL_CHECK_ERR(glEnable(GL_SCISSOR_TEST));
    GL_CHECK_ERR(glEnable(GL_DEPTH_TEST));
    
    const float aspect = (real_t)camera->winw / (real_t)camera->winh;
	const float rotx = camera->vang * 180.f / PI; // get x rotation
	const float roty = (camera->ang - 3.f * PI / 2.f) * 180.f / PI; // get y rotation
	const float rotz = 0.f; // get z rotation
    
    // setup projection + view matrix (shader)
    mat4x4_t proj, view, view2, identity;
    vec4_t translate(-camera->x * 32.f, camera->z, -camera->y * 32.f, 0.f);
    (void)perspective(&proj, fov, aspect, CLIPNEAR, CLIPFAR);
    (void)perspective(&camera->proj_hud, 60.f, aspect, CLIPNEAR, CLIPFAR);
    (void)rotate_mat(&view, &view2, rotx, &identity.x); view2 = view;
    (void)rotate_mat(&view, &view2, roty, &identity.y); view2 = view;
    (void)rotate_mat(&view, &view2, rotz, &identity.z); view2 = view;
    (void)translate_mat(&view, &view2, &translate); view2 = view;
    
    // store proj * view
    (void)mul_mat(&camera->projview, &proj, &view);
    camera->proj = proj;
    
    // set ambient lighting
    if ( camera->globalLightModifierActive ) {
        getLightAtModifier = camera->globalLightModifier;
    }
    else {
        getLightAtModifier = 1.0;
    }
    
    // lightmap dimensions
    vec4_t mapDims;
    mapDims.x = map.width;
    mapDims.y = map.height;
    
    // upload lightmap
    int lightmapIndex = 0;
    for (int c = 0; c < MAXPLAYERS; ++c) {
        if (camera == &cameras[c]) {
            lightmapIndex = c + 1;
            break;
        }
    }
    fillSmoothLightmap(lightmapIndex, map);
    loadLightmapTexture(lightmapIndex, map);
    
	// upload uniforms
    uploadUniforms(voxelShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(voxelBrightShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(voxelDitheredShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(voxelBrightDitheredShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(worldShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(worldDitheredShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(worldDarkShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(skyShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(spriteShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(spriteDitheredShader, (float*)&proj, (float*)&view, (float*)&mapDims);
    uploadUniforms(spriteBrightShader, (float*)&proj, (float*)&view, nullptr);
    uploadUniforms(spriteUIShader, (float*)&proj, (float*)&view, nullptr);
}

#include <thread>
#include <future>

void glEndCamera(view_t* camera, bool useHDR, map_t& map)
{
    if (!camera) {
        return;
    }
    
    GL_CHECK_ERR(glDisable(GL_DEPTH_TEST));
    GL_CHECK_ERR(glDisable(GL_SCISSOR_TEST));
    
#ifdef EDITOR
    const bool hdr = useHDR;
    const bool hdr_multithread = defaultMultithread;
    const float hdr_exposure = defaultExposure;
    const float hdr_gamma = defaultGamma;
    const Vector4& hdr_brightness = defaultBrightness;
    const float hdr_adjustment_rate = defaultAdjustmentRate;
    const float hdr_limit_high = defaultLimitHigh;
    const float hdr_limit_low = defaultLimitLow;
    const int hdr_samples = defaultSamples;
    const Vector4 hdr_luma{defaultLumaRed, defaultLumaGreen, defaultLumaBlue, 0.f};
#else
    const bool hdr = useHDR ? *MainMenu::cvar_hdrEnabled : false;
    const bool hdr_multithread = *cvar_hdrMultithread;
    const float hdr_exposure = *cvar_hdrExposure;
    const float hdr_gamma = *cvar_hdrGamma;
    const Vector4& hdr_brightness = *cvar_hdrBrightness;
    const float hdr_adjustment_rate = *cvar_hdrAdjustment;
    const float hdr_limit_high = *cvar_hdrLimitHigh;
    const float hdr_limit_low = *cvar_hdrLimitLow;
    const int hdr_samples = *cvar_hdrSamples;
    const Vector4 hdr_luma = *cvar_hdrLuma;
#endif
    
    const int numFbs = sizeof(view_t::fb) / sizeof(view_t::fb[0]);
    const int fbIndex = camera->drawnFrames % numFbs;
    
    if (hdr) {
        // update viewport
        camera->fb[fbIndex].unbindForWriting();
        GL_CHECK_ERR(glGetIntegerv(GL_VIEWPORT, oldViewport));
        GL_CHECK_ERR(glViewport(camera->winx, yres - camera->winh - camera->winy, camera->winw, camera->winh));
        
        // calculate luminance
        camera->fb[fbIndex].bindForReading();
        auto pixels = camera->fb[fbIndex].lock();
        if (pixels) {
            // functor for crawling through the framebuffer collecting samples
            auto fn = [](GLhalf* pixels, GLhalf* end, const int step) {
                std::vector<float> v(4);
                if (step > 0) {
                    for (; pixels < end; pixels += step) {
                        const float p[4] = {
                            toFloat32(*(pixels + 0)),
                            toFloat32(*(pixels + 1)),
                            toFloat32(*(pixels + 2)),
                            toFloat32(*(pixels + 3)),
                        };
                        v[0] += p[0];
                        v[1] += p[1];
                        v[2] += p[2];
                        v[3] += p[3];
                    }
                }
                return v;
            };
            
            // collect samples
            std::vector<float> v(4);
            int samplesCollected = 0;
            if (hdr_multithread) {
                // spawn jobs to count samples
                const auto cores = std::thread::hardware_concurrency();
                std::vector<std::future<std::vector<float>>> jobs;
                const int size = camera->winw * camera->winh * 4;
                const int step = ((size / 4) / hdr_samples) * 4;
                const int section = ((size / cores) / 4) * 4;
                auto begin = pixels;
                for (int c = 0; c < cores; ++c, begin += section) {
                    jobs.emplace_back(std::async(std::launch::async, fn,
                        begin, begin + section, step));
                    samplesCollected += section / step;
                }
                
                // add samples together
                for (int c = 0; c < cores; ++c) {
                    auto& job = jobs[c];
                    if (job.valid()) {
                        auto r = job.get();
                        v[0] += r[0];
                        v[1] += r[1];
                        v[2] += r[2];
                        v[3] += r[3];
                    }
                }
            } else {
                // synchronized sample collection
                const int size = camera->winw * camera->winh * 4;
                const int step = ((size / 4) / hdr_samples) * 4;
                v = fn(pixels, pixels + size, step);
                samplesCollected = size / step;
            }
            
            // calculate scene average luminance
            if (samplesCollected) {
                float luminance = v[0] * hdr_luma.x + v[1] * hdr_luma.y + v[2] * hdr_luma.z + v[3] * hdr_luma.w; // dot-product
                luminance = luminance / samplesCollected;
                const float rate = hdr_adjustment_rate / fpsLimit;
                if (camera->luminance > luminance) {
                    camera->luminance -= std::min(rate, camera->luminance - luminance);
                } else if (camera->luminance < luminance) {
                    camera->luminance += std::min(rate, luminance - camera->luminance);
                }
            }
        }
        camera->fb[fbIndex].unlock();
        const float exposure = std::min(std::max(hdr_limit_low, hdr_exposure / camera->luminance), hdr_limit_high);
        const auto& brightness = hdr_brightness;
        const float gamma = hdr_gamma * vidgamma;
        
        // blit framebuffer
        camera->fb[fbIndex].hdrDraw(brightness, gamma, exposure);
        camera->fb[fbIndex].unbindForReading();
        
        // revert viewport
        GL_CHECK_ERR(glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]));
    } else {
        GL_CHECK_ERR(glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]));
    }
    ++camera->drawnFrames;
}

void glDrawVoxel(view_t* camera, Entity* entity, int mode) {
	if (!camera || !entity) {
		return;
	}

	// select model
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
    
    // set GL state
	if (mode == REALCOLORS) {
        GL_CHECK_ERR(glEnable(GL_BLEND));
	}

    int player = -1;
    for ( player = 0; player < MAXPLAYERS; ++player ) {
        if ( &cameras[player] == camera ) {
            break;
        }
    }

    bool telepath =
#ifdef EDITOR
        false;
#else
        (entity->monsterEntityRenderAsTelepath == 1 && !intro 
            && player >= 0 && player < MAXPLAYERS && players[player] && players[player]->entity
            && stats[player]->mask && stats[player]->mask->type == TOOL_BLINDFOLD_TELEPATHY);
#endif

    bool changedDepthRange = false;
	if (entity->flags[OVERDRAW] 
        || telepath
		|| modelindex == FOLLOWER_SELECTED_PARTICLE
		|| modelindex == FOLLOWER_TARGET_PARTICLE ) {
        changedDepthRange = true;
        GL_CHECK_ERR(glDepthRange(0, 0.1));
	}
    
    // bind shader
    auto& dither = entity->dithering[camera];
    auto& shader = !entity->flags[BRIGHT] && !telepath ?
        (dither.value < Entity::Dither::MAX ? voxelDitheredShader : voxelShader) :
        ((entity->flags[INVISIBLE] && entity->flags[INVISIBLE_DITHER] && dither.value < Entity::Dither::MAX) ? voxelBrightDitheredShader : voxelBrightShader);
    shader.bind();
    
    // upload dither amount, if necessary
    if (&shader == &voxelDitheredShader || &shader == &voxelBrightDitheredShader) {
        GL_CHECK_ERR(glUniform1f(shader.uniform("uDitherAmount"),
            (float)((uint32_t)1 << (dither.value - 1)) / (1 << (Entity::Dither::MAX / 2 - 1))));
    }
    
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
        GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, false, (float*)&camera->proj_hud));
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
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m)); // model matrix
    
    // upload light variables
    if (entity->flags[BRIGHT]) {
        mat4x4_t remap(1.f);
        GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uColorRemap"), 1, false, (float*)&remap));
        const float b = std::max(0.5f, camera->luminance * 4.f);
        const GLfloat factor[4] = { 1.f, 1.f, 1.f, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
        const GLfloat light[4] = { b, b, b, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
        const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
        const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));
    } else {
        uploadLightUniforms(camera, shader, entity, mode, true);
    }
    
    // draw mesh
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glBindVertexArray(polymodels[modelindex].vao));
#else
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].vbo));
    GL_CHECK_ERR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(0));
    
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].colors));
    GL_CHECK_ERR(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(1));
    
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, polymodels[modelindex].normals));
    GL_CHECK_ERR(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(2));
#endif
    
    GL_CHECK_ERR(glDrawArrays(GL_TRIANGLES, 0, (int)(3 * polymodels[modelindex].numfaces)));
    
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glBindVertexArray(polymodels[modelindex].vao));
#else
    GL_CHECK_ERR(glDisableVertexAttribArray(0));
    GL_CHECK_ERR(glDisableVertexAttribArray(1));
    GL_CHECK_ERR(glDisableVertexAttribArray(2));
#endif
    
    // reset GL state
    if (entity->flags[OVERDRAW]) {
        GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, false, (float*)&camera->proj));
    }
    if (changedDepthRange) {
        GL_CHECK_ERR(glDepthRange(0, 1));
    }
    if (mode == REALCOLORS) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
    }
}

/*-------------------------------------------------------------------------------

	glDrawSprite

	Draws a 2D sprite to represent an object in 3D

-------------------------------------------------------------------------------*/

Mesh spriteMesh = {
    {
        -.5f, -.5f, 0.f,
         .5f, -.5f, 0.f,
         .5f,  .5f, 0.f,
        -.5f, -.5f, 0.f,
         .5f,  .5f, 0.f,
        -.5f,  .5f, 0.f,
    }, // positions
    {
        0.f, 1.f,
        1.f, 1.f,
        1.f, 0.f,
        0.f, 1.f,
        1.f, 0.f,
        0.f, 0.f,
    }, // texcoords
    {} // colors
};

#ifndef EDITOR
static ConsoleVariable<GLfloat> cvar_enemybarDepthRange("/enemybar_depth_range", 0.5);
static ConsoleVariable<float> cvar_ulight_factor_min("/sprite_ulight_factor_min", 0.5f);
static ConsoleVariable<float> cvar_ulight_factor_mult("/sprite_ulight_factor_mult", 4.f);
#endif

void glDrawEnemyBarSprite(view_t* camera, int mode, int playerViewport, void* enemyHPBarDetails)
{
#ifndef EDITOR
    if (!camera || mode != REALCOLORS || !enemyHPBarDetails) {
		return;
	}
	auto enemybar = (EnemyHPDamageBarHandler::EnemyHPDetails*)enemyHPBarDetails;
	SDL_Surface* sprite = enemybar->worldSurfaceSprite;
	if (!sprite || !enemybar->worldTexture) {
		return;
	}

	// bind texture
	TempTexture* tex = enemybar->worldTexture;
	tex->bind();
    
    // bind shader
    GL_CHECK_ERR(glEnable(GL_BLEND));
    auto& shader = spriteUIShader;
    shader.bind();
    
    vec4_t v;
    mat4x4_t m, t;
    
    // model matrix
    const float height = (float)enemybar->worldZ - 6.f;
    const float drawOffsetY = enemybar->worldSurfaceSpriteStatusEffects ?
        enemybar->worldSurfaceSpriteStatusEffects->h / -2.f : 0.f;
    v = vec4((float)enemybar->worldX * 2.f, -height * 2.f, (float)enemybar->worldY * 2.f, 0.f);
    (void)translate_mat(&t, &m, &v); m = t;
    mat4x4_t i;
    (void)rotate_mat(&t, &m, -90.f - camera->ang * (180.f / PI), &i.y); m = t;
    (void)rotate_mat(&t, &m, -camera->vang * (180.f / PI), &i.x); m = t;
    float scale = 0.08;
    scale += (0.05f * ((*MainMenu::cvar_enemybar_scale / 100.f) - 1.f));
    v = vec4(scale * tex->w, scale * tex->h, scale * enemybar->screenDistance, 0.f);
    (void)scale_mat(&t, &m, &v); m = t;
    
    // don't update if dead target.
    if (enemybar->enemy_hp > 0) {
        // 0, 0, 0, 1.f is centre of rendered quad
        // therefore, the following represents the top
        vec4_t worldCoords;
        worldCoords.x = 0.f;
        worldCoords.y = .5f;
        worldCoords.z = 0.f;
        worldCoords.w = 1.f;
        
        const vec4_t window(camera->winx, camera->winy, camera->winw, camera->winh);
        float topOfWindow = window.w + window.y;
        const float factorY = (float)yres / Frame::virtualScreenY;
        if ( playerViewport >= 0 )
        {
            // sprite height >50 means status effects active, let the effect do the padding
            // otherwise push approx below XP bar, 26 pixels
            topOfWindow += factorY * (sprite->h > 50 ? 0 : -26); 
        }
        float pixelOffset = drawOffsetY;
		vec4_t screenCoordinates = project(&worldCoords, &m, &camera->projview, &window);
        if (screenCoordinates.y >= topOfWindow && screenCoordinates.z >= 0.f) {
            // above camera limit
			pixelOffset += fabs(screenCoordinates.y - topOfWindow);
            
		}

        if ( fabs(pixelOffset) > 0.001 )
        {
		    screenCoordinates.y -= pixelOffset;
            // convert back into worldCoords
		    vec4_t worldCoords2 = unproject(&screenCoordinates, &m, &camera->projview, &window);
            worldCoords2.y -= scale * tex->h * 0.5f;
            m.w = worldCoords2;
        }
	}

    // upload model matrix
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m));

    // update GL state
    GL_CHECK_ERR(glDepthRange(0, *cvar_enemybarDepthRange));
    GL_CHECK_ERR(glEnable(GL_BLEND));
    
    // upload light variables
    const float b = std::max(*MainMenu::cvar_hdrEnabled ? *cvar_ulight_factor_min : 1.f, camera->luminance * *cvar_ulight_factor_mult);
    const GLfloat factor[4] = { 1.f, 1.f, 1.f, (float)enemybar->animator.fadeOut / 100.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
    const GLfloat light[4] = { b, b, b, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
    const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
    const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));

    // draw
    spriteMesh.draw();
    
    // reset GL state
    GL_CHECK_ERR(glDepthRange(0, 1));
    GL_CHECK_ERR(glDisable(GL_BLEND));
#endif // !EDITOR
}

void glDrawWorldDialogueSprite(view_t* camera, void* worldDialogue, int mode)
{
#ifndef EDITOR
	if (!camera || !worldDialogue || mode != REALCOLORS) {
		return;
	}
	auto dialogue = (Player::WorldUI_t::WorldTooltipDialogue_t::Dialogue_t*)worldDialogue;
	if (dialogue->alpha <= 0.0) {
		return;
	}
	SDL_Surface* sprite = nullptr;
	if (dialogue->dialogueTooltipSurface) {
        sprite = dialogue->dialogueTooltipSurface;
	} else {
        sprite = dialogue->blitDialogueTooltip();
	}
	if (!sprite) {
		return;
	}

	// bind texture
	TempTexture* tex = nullptr;
	tex = new TempTexture();
	if (sprite) {
		tex->load(sprite, false, true);
        tex->bind();
	}
    
    // depth range
    GL_CHECK_ERR(glDepthRange(0.f, .6f));
    
    // bind shader
    GL_CHECK_ERR(glEnable(GL_BLEND));
    auto& shader = spriteUIShader;
    shader.bind();
    
    vec4_t v;
    mat4x4_t m, t, i;
    
    // scale
    float scale = static_cast<float>(dialogue->drawScale);
    if (splitscreen) {
        scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale_splitscreen / 100.f) - 1.f));
    } else {
        scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale / 100.f) - 1.f));
    }
    
    // model matrix
    v = vec4(dialogue->x * 2, -(dialogue->z + dialogue->animZ) * 2 - 1, dialogue->y * 2, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    (void)rotate_mat(&m, &t, -90.f - camera->ang * (180.f / PI), &i.y); t = m;
    (void)rotate_mat(&m, &t, -camera->vang * (180.f / PI), &i.x); t = m;
    v = vec4(scale * tex->w, scale * tex->h, scale, 0.f);
    (void)scale_mat(&m, &t, &v); t = m;
    
    // limit to top of screen:
    {
        // 0, 0, 0, 1.f is centre of rendered quad
        // therefore, the following represents the top
        vec4_t worldCoords;
        worldCoords.x = 0.f;
        worldCoords.y = .5f;
        worldCoords.z = 0.f;
        worldCoords.w = 1.f;
        
        const vec4_t window(camera->winx, camera->winy, camera->winw, camera->winh);
        const float topOfWindow = window.w + window.y;
        vec4_t screenCoordinates = project(&worldCoords, &m, &camera->projview, &window);
        if (screenCoordinates.y >= topOfWindow && screenCoordinates.z >= 0.f) {
            // above camera limit
            const float pixelOffset = fabs(screenCoordinates.y - topOfWindow);
            screenCoordinates.y -= pixelOffset;
            
            // convert back into worldCoords
            vec4_t worldCoords2 = unproject(&screenCoordinates, &m, &camera->projview, &window);
            worldCoords2.y -= scale * tex->h * 0.5f;
            m.w = worldCoords2;
        }
    }
    
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m)); // model matrix
    
    // upload light variables
    const float b = std::max(*MainMenu::cvar_hdrEnabled ? *cvar_ulight_factor_min : 1.f, camera->luminance * *cvar_ulight_factor_mult);
    const GLfloat factor[4] = { 1.f, 1.f, 1.f, (float)dialogue->alpha };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
    const GLfloat light[4] = { b, b, b, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
    const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
    const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));

    // draw
    spriteMesh.draw();

    // cleanup
	if (tex) {
		delete tex;
		tex = nullptr;
	}
    
    // reset GL state
    GL_CHECK_ERR(glDepthRange(0.f, 1.f));
    GL_CHECK_ERR(glDisable(GL_BLEND));
#endif
}

void glDrawWorldUISprite(view_t* camera, Entity* entity, int mode)
{
#ifndef EDITOR
	if (!camera || !entity || intro) {
		return;
	}
    if (mode != REALCOLORS) {
        return;
    }

    // find player that this UI sprite is drawing for
	int player = -1;
	if ( entity->behavior == &actSpriteWorldTooltip ) {
		if ( entity->worldTooltipIgnoreDrawing != 0 ) {
			return;
		}
		for (player = 0; player < MAXPLAYERS; ++player) {
			if (&cameras[player] == camera) {
				break;
			}
		}
		if (player >= 0 && player < MAXPLAYERS) {
            //if ( CalloutMenu[player].calloutMenuIsOpen() )
            //{
            //    real_t dx, dy;
            //    dx = camera->x * 16.0 - entity->x;
            //    dy = camera->y * 16.0 - entity->y;
            //    if ( sqrt(dx * dx + dy * dy) > 24.0 )
            //    {
            //        return; // too far, ignore drawing
            //    }
            //}
			if (entity->worldTooltipPlayer != player) {
				return;
			}
			if (entity->worldTooltipActive == 0 && entity->worldTooltipFadeDelay == 0) {
				return;
			}
		} else {
			return;
		}
		if (!uidToEntity(entity->parent)) {
			return;
		}
	}

	// bind texture
    TempTexture* tex = nullptr;
	SDL_Surface* sprite = nullptr;
	if (entity->behavior == &actSpriteWorldTooltip)
	{
		Entity* parent = uidToEntity(entity->parent);
		if (parent && parent->behavior == &actItem && (multiplayer != CLIENT
            || (multiplayer == CLIENT && (parent->itemReceivedDetailsFromServer != 0 || parent->skill[10] != 0))))
		{
			Item* item = newItemFromEntity(uidToEntity(entity->parent), true);
			if (!item) {
				return;
			}
			sprite = players[player]->worldUI.worldTooltipItem.blitItemWorldTooltip(item);
			free(item);
		}

		tex = new TempTexture();
		if (sprite) {
		    tex->load(sprite, false, true);
		    tex->bind();
		}
	}
	else {
		if (entity->sprite >= 0 && entity->sprite < numsprites) {
			if (sprites[entity->sprite] != NULL) {
				sprite = sprites[entity->sprite];
			} else {
				sprite = sprites[0];
			}
		} else {
			sprite = sprites[0];
		}
	}
    
    // depth range
    if (entity->flags[OVERDRAW]) {
        GL_CHECK_ERR(glDepthRange(0.1f, 0.2f));
    } else {
        GL_CHECK_ERR(glDepthRange(0.f, 0.6f));
    }
    
    // bind shader
    GL_CHECK_ERR(glEnable(GL_BLEND));
    auto& shader = spriteUIShader;
    shader.bind();
    
    vec4_t v;
    mat4x4_t m, t, i;
    
    // scale
    float scale = Player::WorldUI_t::WorldTooltipItem_t::WorldItemSettings_t::scaleMod;
    if ( splitscreen ) {
        scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale_splitscreen / 100.f) - 1.f));
    } else {
        scale += (0.05f * ((*MainMenu::cvar_worldtooltip_scale / 100.f) - 1.f));
    }
    
    // model matrix
    v = vec4(entity->x * 2, -entity->z * 2 - 1, entity->y * 2, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    (void)rotate_mat(&m, &t, -90.f - camera->ang * (180.f / PI), &i.y); t = m;
    (void)rotate_mat(&m, &t, -camera->vang * (180.f / PI), &i.x); t = m;
    v = vec4((entity->scalex + scale) * tex->w, (entity->scalez + scale) * tex->h, (entity->scalez + scale), 0.f);
    (void)scale_mat(&m, &t, &v); t = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m)); // model matrix
    
    // upload light variables
    const float b = std::max(*MainMenu::cvar_hdrEnabled ? *cvar_ulight_factor_min : 1.f, camera->luminance * *cvar_ulight_factor_mult);
    const GLfloat factor[4] = { 1.f, 1.f, 1.f, (float)entity->worldTooltipAlpha };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
    const GLfloat light[4] = { b, b, b, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
    const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
    const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));
    
    // draw
    spriteMesh.draw();
    
    // cleanup
    if (tex) {
        delete tex;
        tex = nullptr;
    }
    
    // reset GL state
    GL_CHECK_ERR(glDepthRange(0.f, 1.f));
    GL_CHECK_ERR(glDisable(GL_BLEND));
#endif
}

#ifndef EDITOR
static ConsoleVariable<GLfloat> cvar_dmgSpriteDepthRange("/dmg_sprite_depth_range", 0.49);
#endif // !EDITOR

void glDrawSprite(view_t* camera, Entity* entity, int mode)
{
    // bind texture
    SDL_Surface* sprite;
    if (entity->sprite >= 0 && entity->sprite < numsprites) {
        if (sprites[entity->sprite] != nullptr) {
            sprite = sprites[entity->sprite];
        } else {
            sprite = sprites[0];
        }
    } else {
        sprite = sprites[0];
    }
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid[(long int)sprite->userdata]));
    
    // set GL state
    if (mode == REALCOLORS) {
        GL_CHECK_ERR(glEnable(GL_BLEND));
    }
    if (entity->flags[OVERDRAW]) {
        GL_CHECK_ERR(glDepthRange(0, 0.1));
    }
    else
    {
        if ( entity->behavior == &actDamageGib ) {
#ifndef EDITOR
            GL_CHECK_ERR(glDepthRange(0.f, *cvar_dmgSpriteDepthRange));
#endif // !EDITOR
        }
    }
    
    // bind shader
    auto& dither = entity->dithering[camera];
    auto& shader = !entity->flags[BRIGHT] ?
        (dither.value < Entity::Dither::MAX ? spriteDitheredShader : spriteShader):
        spriteBrightShader;
    shader.bind();
    
    // upload dither amount, if necessary
    if (&shader == &spriteDitheredShader) {
        GL_CHECK_ERR(glUniform1f(shader.uniform("uDitherAmount"),
            (float)((uint32_t)1 << (dither.value - 1)) / (1 << (Entity::Dither::MAX / 2 - 1))));
    }
    
    vec4_t v;
    mat4x4_t m, t, i;
    
    // model matrix
    if (entity->flags[OVERDRAW]) {
        v = vec4(camera->x * 32, -camera->z, camera->y * 32, 0);
        (void)translate_mat(&m, &t, &v); t = m;
        const float rotx = 0; // roll
        const float roty = 360.0 - camera->ang * 180.0 / PI; // yaw
        const float rotz = 360.0 - camera->vang * 180.0 / PI; // pitch
        (void)rotate_mat(&m, &t, roty, &i.y); t = m; // yaw
        (void)rotate_mat(&m, &t, rotz, &i.z); t = m; // pitch
        (void)rotate_mat(&m, &t, rotx, &i.x); t = m; // roll
        GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, false, (float*)&camera->proj_hud));
    }
    v = vec4(entity->x * 2.f, -entity->z * 2.f - 1, entity->y * 2.f, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    (void)rotate_mat(&m, &t, entity->flags[OVERDRAW] ? -90.f :
        -90.f - camera->ang * (180.f / PI), &i.y); t = m;
    v = vec4(entity->focalx * 2.f, -entity->focalz * 2.f, entity->focaly * 2.f, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    v = vec4(entity->scalex * sprite->w, entity->scaley * sprite->h, entity->scalez, 0.f);
    (void)scale_mat(&m, &t, &v); t = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m)); // model matrix
    
    // upload light variables
    if (entity->flags[BRIGHT]) {
#ifndef EDITOR
        const float b = std::max(*MainMenu::cvar_hdrEnabled ? *cvar_ulight_factor_min : 1.f, camera->luminance * *cvar_ulight_factor_mult);
#else
        const float b = std::max(0.5f, camera->luminance * 4.f);
#endif
        const GLfloat factor[4] = { 1.f, 1.f, 1.f, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
        const GLfloat light[4] = { b, b, b, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
        const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
        const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));
    } else {
        uploadLightUniforms(camera, shader, entity, mode, false);
    }

	// draw
    spriteMesh.draw();
    
    // reset GL state
    if (entity->flags[OVERDRAW]) {
        GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uProj"), 1, false, (float*)&camera->proj));
    }
    if (mode == REALCOLORS) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
    }
    if (entity->flags[OVERDRAW] || entity->behavior == &actDamageGib) {
        GL_CHECK_ERR(glDepthRange(0.f, 1.f));
    }
}

void glDrawSpriteFromImage(view_t* camera, Entity* entity, std::string text, int mode, bool useTextAsImgPath, bool rotate)
{
	if (!camera || !entity || text.empty()) {
		return;
	}

    // set color
	Uint32 color = makeColor(255, 255, 255, 255);
    if ( entity->behavior == &actDamageGib ) {
#ifndef EDITOR
        if ( !EnemyHPDamageBarHandler::bDamageGibTypesEnabled )
        {
            if ( text[0] == '+' )
            {
                color = hudColors.characterSheetGreen;
            }
        }
        else
        {
            color = entity->skill[6];
        }
#endif // !EDITOR
    }
	else if (entity->behavior == &actSpriteNametag) {
		color = entity->skill[1];
	}

    GLfloat w = 0.0;
    GLfloat h = 0.0;
    if ( useTextAsImgPath )
    {
        if ( text.find('*') == std::string::npos )
        {
            text.insert(text.begin(), '*');
        }
        if ( auto imgGet = Image::get(text.c_str()) )
        {
            // bind texture
            GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, imgGet->getTexID()));
            w = imgGet->getWidth();
            h = imgGet->getHeight();
        }
    }
    else
    {
	    auto rendered_text = Text::get(
            text.c_str(), "fonts/pixel_maz.ttf#32#2",
		    color, makeColor(0, 0, 0, 255));
	    auto textureId = rendered_text->getTexID();

        // bind texture
        GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, textureId));
        w = static_cast<GLfloat>(rendered_text->getWidth());
        h = static_cast<GLfloat>(rendered_text->getHeight());
    }
    if (mode == REALCOLORS) {
        GL_CHECK_ERR(glEnable(GL_BLEND));
    }
    
    // set GL state
	if (entity->flags[OVERDRAW]) {
        GL_CHECK_ERR(glDepthRange(0.f, 0.1f));
	} else {
		if (entity->behavior == &actDamageGib) {
#ifndef EDITOR
            GL_CHECK_ERR(glDepthRange(0.f, *cvar_dmgSpriteDepthRange));
#endif // !EDITOR
		}
		else if (entity->behavior != &actSpriteNametag) {
            GL_CHECK_ERR(glDepthRange(0.f, 0.98f));
		}
		else if (entity->behavior == &actSpriteNametag) {
            GL_CHECK_ERR(glDepthRange(0.f, 0.52f));
		}
	}
    
    // bind shader
    auto& shader = spriteUIShader;
    shader.bind();
    
    vec4_t v;
    mat4x4_t m, t, i;
    
    // model matrix
    if (entity->flags[OVERDRAW]) {
        v = vec4(camera->x * 32, -camera->z, camera->y * 32, 0);
        (void)translate_mat(&m, &t, &v); t = m;
        const float rotx = 0; // roll
        const float roty = 360.0 - camera->ang * 180.0 / PI; // yaw
        const float rotz = 360.0 - camera->vang * 180.0 / PI; // pitch
        (void)rotate_mat(&m, &t, roty, &i.y); t = m; // yaw
        (void)rotate_mat(&m, &t, rotz, &i.z); t = m; // pitch
        (void)rotate_mat(&m, &t, rotx, &i.x); t = m; // roll
    }

    v = vec4(entity->x * 2.f, -entity->z * 2.f - 1, entity->y * 2.f, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    if ( rotate )
    {
        float rotx, roty, rotz;
        rotx = entity->roll * 180.0 / PI; // roll
        roty = 360.0 - entity->yaw * 180.0 / PI; // yaw
        rotz = 360.0 - entity->pitch * 180.0 / PI; // pitch
        (void)rotate_mat(&m, &t, roty, &i.y); t = m; // yaw
        (void)rotate_mat(&m, &t, rotz, &i.z); t = m; // pitch
        (void)rotate_mat(&m, &t, rotx, &i.x); t = m; // roll
    }
    else
    {
        (void)rotate_mat(&m, &t, entity->flags[OVERDRAW] ? -90.f :
            -90.f - camera->ang * (180.f / PI), &i.y); t = m;
    }
    v = vec4(entity->focalx * 2.f, -entity->focalz * 2.f, entity->focaly * 2.f, 0.f);
    (void)translate_mat(&m, &t, &v); t = m;
    v = vec4(entity->scalex * w, entity->scaley * h, entity->scalez, 0.f);
    (void)scale_mat(&m, &t, &v); t = m;
    GL_CHECK_ERR(glUniformMatrix4fv(shader.uniform("uModel"), 1, false, (float*)&m)); // model matrix
    
    // upload light variables
#ifndef EDITOR
    const float b = std::max(*MainMenu::cvar_hdrEnabled ? *cvar_ulight_factor_min : 1.f, camera->luminance * *cvar_ulight_factor_mult);
#else
    const float b = std::max(0.5f, camera->luminance * 4.f);
#endif
    const GLfloat factor[4] = { 1.f, 1.f, 1.f, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, factor));
    const GLfloat light[4] = { b, b, b, 1.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightColor"), 1, light));
    const GLfloat empty[4] = { 0.f, 0.f, 0.f, 0.f };
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uColorAdd"), 1, empty));
    const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
    GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));

    // draw
    spriteMesh.draw();
    
    // reset GL state
    GL_CHECK_ERR(glDepthRange(0, 1));
    if (mode == REALCOLORS) {
        GL_CHECK_ERR(glDisable(GL_BLEND));
    }
}

/*-------------------------------------------------------------------------------

	glDrawWorld

	Draws the current map from the given camera point

-------------------------------------------------------------------------------*/

static bool shouldDrawClouds(const map_t& map, int* cloudtile = nullptr, bool forceCheck = true) {
    bool clouds = false;
#ifdef EDITOR
    const bool fog = false;
#else
    const bool fog = *cvar_fogDistance > 0.f;
#endif
    if (!fog || forceCheck) {
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
    }
    return clouds;
}

#include "ui/Image.hpp"

std::vector<Chunk> chunks;

constexpr float sky_size = CLIPFAR * 16.f;
constexpr float sky_htex_size = sky_size / 64.f;
constexpr float sky_ltex_size = sky_size / 32.f;
Mesh skyMesh = {
    {
        -sky_size, 65.f, -sky_size,
         sky_size, 65.f, -sky_size,
         sky_size, 65.f,  sky_size,
        -sky_size, 65.f, -sky_size,
         sky_size, 65.f,  sky_size,
        -sky_size, 65.f,  sky_size,
        -sky_size, 64.f, -sky_size,
         sky_size, 64.f, -sky_size,
         sky_size, 64.f,  sky_size,
        -sky_size, 64.f, -sky_size,
         sky_size, 64.f,  sky_size,
        -sky_size, 64.f,  sky_size,
    }, // positions
    {
        0.f, 0.f,
        sky_htex_size, 0.f,
        sky_htex_size, sky_htex_size,
        0.f, 0.f,
        sky_htex_size, sky_htex_size,
        0.f, sky_htex_size,
        0.f, 0.f,
        sky_ltex_size, 0.f,
        sky_ltex_size, sky_ltex_size,
        0.f, 0.f,
        sky_ltex_size, sky_ltex_size,
        0.f, sky_ltex_size,
    }, // texcoords
    {
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, .5f,
        1.f, 1.f, 1.f, .5f,
        1.f, 1.f, 1.f, .5f,
        1.f, 1.f, 1.f, .5f,
        1.f, 1.f, 1.f, .5f,
        1.f, 1.f, 1.f, .5f,
    }, // colors
};

#ifndef EDITOR
static ConsoleVariable<bool> cvar_allowChunkRebuild("/allow_chunk_rebuild", true);
#endif

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
    const bool clouds = shouldDrawClouds(map, &cloudtile, false);
    
    // select texture atlas
    constexpr int numTileAtlases = sizeof(AnimatedTile::indices) / sizeof(AnimatedTile::indices[0]);
    const int atlasIndex = (ticks % (numTileAtlases * 10)) / 10;
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE2));
    bindTextureAtlas(atlasIndex);
    GL_CHECK_ERR(glActiveTexture(GL_TEXTURE0));
    
    // upload uniforms for dither shader
    if (mode == REALCOLORS) {
        worldDitheredShader.bind();
        const GLfloat light[4] = { (float)getLightAtModifier, (float)getLightAtModifier, (float)getLightAtModifier, 1.f };
        GL_CHECK_ERR(glUniform4fv(worldDitheredShader.uniform("uLightFactor"), 1, light));
    }
    
    // bind core shader
    auto& shader = mode == REALCOLORS ?
        worldShader : worldDarkShader;
    shader.bind();
    
    // upload uniforms for core shader
    if (&shader != &worldDarkShader) {
        const GLfloat light[4] = { (float)getLightAtModifier, (float)getLightAtModifier, (float)getLightAtModifier, 1.f };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, light));
        const float cameraPos[4] = {(float)camera->x * 32.f, -(float)camera->z, (float)camera->y * 32.f, 1.f};
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uCameraPos"), 1, cameraPos));
    }
    
    const bool ditheringDisabled = ticks - ditherDisabledTime < TICKS_PER_SECOND;
    
    // update chunk dithering & mark chunks for rebuilding
    std::set<std::pair<int, Chunk*>> chunksToBuild;
    for (int index = 0; index < chunks.size(); ++index) {
        auto& chunk = chunks[index];
        auto& dither = chunk.dithering[camera];
        if (ticks != dither.lastUpdateTick) {
            dither.lastUpdateTick = ticks;
            for (int x = chunk.x; x < chunk.x + chunk.w; ++x) {
                for (int y = chunk.y; y < chunk.y + chunk.h; ++y) {
                    if (camera->vismap[y + x * map.height]) {
                        if (ditheringDisabled) {
                            dither.value = Chunk::Dither::MAX;
                        } else {
                            dither.value = std::min(Chunk::Dither::MAX, dither.value + 2);
                        }
                        goto end;
                    }
                }
            }
            if (ditheringDisabled) {
                dither.value = 0;
            } else {
                dither.value = std::max(0, dither.value - 2);
            }
        end:;
        }
        if (dither.value) {
            if (chunk.isDirty(map)) {
                chunksToBuild.emplace(0, &chunk);
            }
        }
    }
    
    // mark chunk neighbors for building (in-case of shared walls)
    const int dim = 4; // size of chunk in tiles
    const int yoff = 1;
    const int xoff = (map.height / dim) + ((map.height % dim) ? 1 : 0);
    for (auto it = chunksToBuild.begin(); it != chunksToBuild.end();) {
        const int index = it->first;
        bool foundDirtyNeighbor = false; // call the police
        for (int x = -xoff; x <= xoff; x += xoff) {
            for (int y = -yoff; y <= yoff; y += yoff) {
                if ((x && y) || (!x && !y)) {
                    continue;
                }
                const int off = index + x + y;
                if (off < 0 || off >= chunks.size()) {
                    continue;
                }
                auto& neighbor = chunks[off];
                if (chunksToBuild.emplace(off, &neighbor).second) {
                    if (neighbor.isDirty(map)) {
                        // if this neighbor chunk needs rebuilding,
                        // it's possible _its_ neighbors need rebuilding too.
                        // therefore, restart the search for adjacent chunks.
                        it = chunksToBuild.begin();
                        foundDirtyNeighbor = true;
                    }
                }
            }
        }
        if (!foundDirtyNeighbor) {
            ++it;
        }
    }
    
#ifdef EDITOR
    constexpr bool allowChunkRebuild = true;
#else
    const bool allowChunkRebuild = *cvar_allowChunkRebuild;
#endif
    
    // build chunks
    if (allowChunkRebuild) {
        for (auto& pair : chunksToBuild) {
            auto& chunk = *pair.second;
            chunk.build(map, !clouds, chunk.x, chunk.y, chunk.w, chunk.h);
        }
    }
    
    // draw chunks
    for (auto& chunk : chunks) {
        auto& dither = chunk.dithering[camera];
        if (dither.value) {
            if (mode == REALCOLORS) {
                if (dither.value == 10) {
                    worldShader.bind();
                    chunk.draw();
                } else {
                    worldDitheredShader.bind();
                    GL_CHECK_ERR(glUniform1f(worldDitheredShader.uniform("uDitherAmount"),
                        (float)((uint32_t)1 << (dither.value - 1)) / (1 << (Chunk::Dither::MAX / 2 - 1))));
                    chunk.draw();
                }
            } else {
                chunk.draw();
            }
        }
    }
    
    // draw clouds
    // do this after drawing walls/floors/etc because that way most of it can
    // fail the depth test, improving fill rate.
    if (clouds && mode == REALCOLORS) {
        auto& shader = skyShader;
        shader.bind();
        GL_CHECK_ERR(glDepthMask(GL_FALSE));
        GL_CHECK_ERR(glEnable(GL_BLEND));
        
        // upload texture scroll value
        const float scroll[2] = {
            ((float)(ticks % 60) / 60.f),
            ((float)(ticks % 120) / 120.f),
        };
        GL_CHECK_ERR(glUniform2fv(shader.uniform("uScroll"), 1, scroll));
        
        // upload light value
        const float light[4] = {
            (float)getLightAtModifier,
            (float)getLightAtModifier,
            (float)getLightAtModifier,
            1.f,
        };
        GL_CHECK_ERR(glUniform4fv(shader.uniform("uLightFactor"), 1, light));
        
        // bind cloud texture
        GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, texid[(long int)tiles[cloudtile]->userdata]));
        
        // draw sky
        skyMesh.draw();
        
        // reset GL
        GL_CHECK_ERR(glDisable(GL_BLEND));
        GL_CHECK_ERR(glDepthMask(GL_TRUE));
    }
}

static int dirty = 1;
static int oldx = 0, oldy = 0;
static unsigned int oldpix = 0;

unsigned int GO_GetPixelU32(int x, int y, view_t& camera)
{
    if (!dirty && (oldx==x) && (oldy==y)) {
        return oldpix;
    }
    
    if (!hdrEnabled) {
        main_framebuffer.unbindForWriting();
    }
    
	if (dirty) {
        GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 0.f));
        GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		glBeginCamera(&camera, false, map);
		glDrawWorld(&camera, ENTITYUIDS);
		drawEntities3D(&camera, ENTITYUIDS);
		glEndCamera(&camera, false, map);
	}

	GLubyte pixel[4];
    GL_CHECK_ERR(glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixel));
	oldpix = pixel[0] + (((Uint32)pixel[1]) << 8) + (((Uint32)pixel[2]) << 16) + (((Uint32)pixel[3]) << 24);
    if (!hdrEnabled) {
        main_framebuffer.bindForWriting();
    }
	dirty = 0;
	return oldpix;
}

const char* gl_error_string(GLenum err) {
    switch (err) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        assert(0 && "GL_INVALID_ENUM");
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        assert(0 && "GL_INVALID_VALUE");
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        assert(0 && "GL_INVALID_OPERATION");
        return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
        assert(0 && "GL_STACK_OVERFLOW");
        return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
        assert(0 && "GL_STACK_UNDERFLOW");
        return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
        assert(0 && "GL_OUT_OF_MEMORY");
        return "GL_OUT_OF_MEMORY";
    case GL_TABLE_TOO_LARGE:
        assert(0 && "GL_TABLE_TOO_LARGE");
        return "GL_TABLE_TOO_LARGE";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        assert(0 && "GL_INVALID_FRAMEBUFFER_OPERATION");
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        assert(0 && "unknown OpenGL error!");
        return nullptr;
    }
}

void GO_SwapBuffers(SDL_Window* screen)
{
	dirty = 1;
    
    if (!hdrEnabled) {
        main_framebuffer.unbindForWriting();
        main_framebuffer.bindForReading();
        GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 0.f));
        GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        main_framebuffer.draw(vidgamma);
    }
	
    SDL_GL_SwapWindow(screen);
    
#ifndef EDITOR
    // enable HDR if desired
    hdrEnabled = *MainMenu::cvar_hdrEnabled;
#endif
    
    if (!hdrEnabled) {
        main_framebuffer.bindForWriting();
    }
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
    this->x = startX;
    this->y = startY;
    this->w = endX - startX;
    this->h = endY - startY;
    const int sizeOfTiles = this->w * this->h * MAPLAYERS;
    this->tiles.clear();
    this->tiles.resize(sizeOfTiles);
    
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
#ifdef VERTEX_ARRAYS_ENABLED
    if (!vao) {
        GL_CHECK_ERR(glGenVertexArrays(1, &vao));
    }
    GL_CHECK_ERR(glBindVertexArray(vao));
#endif
    if (!vbo_positions) {
        GL_CHECK_ERR(glGenBuffers(1, &vbo_positions));
    }
    if (!vbo_texcoords) {
        GL_CHECK_ERR(glGenBuffers(1, &vbo_texcoords));
    }
    if (!vbo_colors) {
        GL_CHECK_ERR(glGenBuffers(1, &vbo_colors));
    }
    
    // upload positions
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_positions));
    GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(0));
#endif
    
    // upload texcoords
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords));
    GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_DYNAMIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(1));
#endif
    
    // upload colors
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_colors));
    GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_DYNAMIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(2));
#endif
    
#ifndef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
}

void Chunk::destroyBuffers() {
    if (vao) {
        GL_CHECK_ERR(glDeleteVertexArrays(1, &vao));
        vao = 0;
    }
    if (vbo_positions) {
        GL_CHECK_ERR(glDeleteBuffers(1, &vbo_positions));
        vbo_positions = 0;
    }
    if (vbo_texcoords) {
        GL_CHECK_ERR(glDeleteBuffers(1, &vbo_texcoords));
        vbo_texcoords = 0;
    }
    if (vbo_colors) {
        GL_CHECK_ERR(glDeleteBuffers(1, &vbo_colors));
        vbo_colors = 0;
    }
    indices = 0;
}

void Chunk::draw() {
    if (!indices) {
        return;
    }
    
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glBindVertexArray(vao));
#else
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_positions));
    GL_CHECK_ERR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(0));
    
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords));
    GL_CHECK_ERR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(1));
    
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo_colors));
    GL_CHECK_ERR(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK_ERR(glEnableVertexAttribArray(2));
#endif
    
    GL_CHECK_ERR(glDrawArrays(GL_TRIANGLES, 0, indices));
    
#ifndef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glDisableVertexAttribArray(0));
    GL_CHECK_ERR(glDisableVertexAttribArray(1));
    GL_CHECK_ERR(glDisableVertexAttribArray(2));
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
}

bool Chunk::isDirty(const map_t& map) {
    if (tiles.empty()) {
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

static ConsoleCommand ccmd_update_chunks("/updatechunks", "rebuilds all chunks",
    [](int argc, const char* argv[]){
    clearChunks();
    createChunks();
    });
#endif
