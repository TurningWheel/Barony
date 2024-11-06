/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains all drawing code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.o

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "files.hpp"
#include "hash.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "ui/Frame.hpp"
#ifdef EDITOR
#include "editor.hpp"
#include "mod_tools.hpp"
#endif
#include "items.hpp"
#include "ui/Image.hpp"
#include "interface/consolecommand.hpp"
#include "colors.hpp"
#include "ui/Text.hpp"
#include "ui/GameUI.hpp"

#include <cassert>

#include "ui/Image.hpp"

const std::unordered_map<Mesh::BufferType, int> Mesh::ElementsPerVBO = {
	{Mesh::BufferType::Position, 3},
	{Mesh::BufferType::TexCoord, 2},
	{Mesh::BufferType::Color, 4},
};

framebuffer main_framebuffer;

Mesh framebuffer::mesh{
	{ // positions
		-1.f, -1.f,  0.f,
		 1.f, -1.f,  0.f,
		 1.f,  1.f,  0.f,
        -1.f, -1.f,  0.f,
         1.f,  1.f,  0.f,
		-1.f,  1.f,  0.f,
	},
	{ // texcoords
		0.f,  0.f,
		1.f,  0.f,
		1.f,  1.f,
        0.f,  0.f,
        1.f,  1.f,
		0.f,  1.f,
	},
	{ // colors
		1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
		1.f, 1.f, 1.f, 1.f,
	}
};

Shader framebuffer::shader;
Shader framebuffer::hdrShader;
Shader voxelShader;
Shader voxelBrightShader;
Shader voxelDitheredShader;
Shader voxelBrightDitheredShader;
Shader worldShader;
Shader worldDitheredShader;
Shader worldDarkShader;
Shader skyShader;
Shader spriteShader;
Shader spriteDitheredShader;
Shader spriteBrightShader;
Shader spriteUIShader;
TempTexture* lightmapTexture[MAXPLAYERS + 1];

static Shader gearShader;
static Shader lineShader;
static Mesh lineMesh = {
    {
        1.f, 1.f, 0.f, 1.f,
        1.f, 1.f, 0.f, 1.f,
    }, // positions
    {}, // texcoords
    {} // colors
};

static void buildVoxelShader(
    Shader& shader, const char* name, bool lightmap,
	const char* v, size_t size_v,
	const char* f, size_t size_f)
{
	shader.init(name);
	shader.compile(v, size_v, Shader::Type::Vertex);
	shader.compile(f, size_f, Shader::Type::Fragment);
	shader.bindAttribLocation("iPosition", 0);
	shader.bindAttribLocation("iColor", 1);
	shader.bindAttribLocation("iNormal", 2);
	shader.link();
    if (lightmap) {
        shader.bind();
        GL_CHECK_ERR(glUniform1i(shader.uniform("uLightmap"), 1));
    }
}

static void buildWorldShader(
    Shader& shader, const char* name, bool textures,
    const char* v, size_t size_v,
    const char* f, size_t size_f)
{
    shader.init(name);
    shader.compile(v, size_v, Shader::Type::Vertex);
    shader.compile(f, size_f, Shader::Type::Fragment);
    shader.bindAttribLocation("iPosition", 0);
    shader.bindAttribLocation("iTexCoord", 1);
    shader.bindAttribLocation("iColor", 2);
    shader.link();
    if (textures) {
        shader.bind();
        GL_CHECK_ERR(glUniform1i(shader.uniform("uTextures"), 2));
        GL_CHECK_ERR(glUniform1i(shader.uniform("uLightmap"), 1));
    }
}

static void buildSpriteShader(
    Shader& shader, const char* name, bool lightmap,
    const char* v, size_t size_v,
    const char* f, size_t size_f)
{
    shader.init(name);
    shader.compile(v, size_v, Shader::Type::Vertex);
    shader.compile(f, size_f, Shader::Type::Fragment);
    shader.bindAttribLocation("iPosition", 0);
    shader.bindAttribLocation("iTexCoord", 1);
    shader.bindAttribLocation("iColor", 2);
    shader.link();
    shader.bind();
    GL_CHECK_ERR(glUniform1i(shader.uniform("uTexture"), 0));
    if (lightmap) {
        GL_CHECK_ERR(glUniform1i(shader.uniform("uLightmap"), 1));
    }
}

void createCommonDrawResources() {
    // framebuffer shader:
    
    framebuffer::mesh.init();
    
	static const char fb_vertex_glsl[] =
		"in vec3 iPosition;"
		"in vec2 iTexCoord;"
        "out vec2 TexCoord;"
		"void main() {"
		"gl_Position = vec4(iPosition, 1.0);"
        "TexCoord = iTexCoord;"
		"}";

	static const char fb_fragment_glsl[] =
        "in vec2 TexCoord;"
		"uniform sampler2D uTexture;"
		"uniform float uBrightness;"
        "out vec4 FragColor;"
		"void main() {"
        "vec4 color = texture(uTexture, TexCoord);"
		"FragColor = vec4(color.rgb * uBrightness, color.a);"
		"}";
    
    framebuffer::shader.init("framebuffer");
    framebuffer::shader.compile(fb_vertex_glsl, sizeof(fb_vertex_glsl), Shader::Type::Vertex);
    framebuffer::shader.compile(fb_fragment_glsl, sizeof(fb_fragment_glsl), Shader::Type::Fragment);
    framebuffer::shader.bindAttribLocation("iPosition", 0);
    framebuffer::shader.bindAttribLocation("iTexCoord", 1);
    framebuffer::shader.link();
    framebuffer::shader.bind();
    GL_CHECK_ERR(glUniform1i(framebuffer::shader.uniform("uTexture"), 0));
    
    static const char fb_hdr_fragment_glsl[] =
        "in vec2 TexCoord;"
        "uniform sampler2D uTexture;"
        "uniform vec4 uBrightness;"
        "uniform float uGamma;"
        "uniform float uExposure;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec4 color = texture(uTexture, TexCoord);"
        "vec3 mapped = color.rgb;"
    
        // reinhard tone-mapping
        "mapped = vec3(1.0) - exp(-mapped * uExposure);"
    
        // another kind of reinhard tone mapping (pick one)
        //"mapped = mapped * (uExposure / (1.0 + mapped / uExposure));"

        // luma-based reinhard tone mapping (does not use exposure)
        //"float luma = dot(mapped, vec3(0.2126, 0.7152, 0.0722));"
        //"float toneMappedLuma = luma / (1.0 + luma);"
        //"mapped = mapped * (toneMappedLuma / luma);"
    
        // additional tone-mapping examples
        //https://www.shadertoy.com/view/lslGzl
    
        // gamma correction
        "mapped = pow(mapped, vec3(1.0 / uGamma));"
    
        "FragColor = vec4(mapped, color.a) * uBrightness;"
        "}";
    
    framebuffer::hdrShader.init("hdr framebuffer");
    framebuffer::hdrShader.compile(fb_vertex_glsl, sizeof(fb_vertex_glsl), Shader::Type::Vertex);
    framebuffer::hdrShader.compile(fb_hdr_fragment_glsl, sizeof(fb_hdr_fragment_glsl), Shader::Type::Fragment);
    framebuffer::hdrShader.bindAttribLocation("iPosition", 0);
    framebuffer::hdrShader.bindAttribLocation("iTexCoord", 1);
    framebuffer::hdrShader.link();
    framebuffer::hdrShader.bind();
    GL_CHECK_ERR(glUniform1i(framebuffer::hdrShader.uniform("uTexture"), 0));
    
    // create lightmap textures
    for (int c = 0; c < MAXPLAYERS + 1; ++c) {
        lightmapTexture[c] = new TempTexture();
    }
    
    // voxel shader:
    
    static const char vox_vertex_glsl[] =
        "in vec3 iPosition;"
        "in vec3 iColor;"
        "in vec3 iNormal;"
        "uniform mat4 uProj;"
        "uniform mat4 uView;"
        "uniform mat4 uModel;"
        "out vec3 Color;"
        "out vec4 WorldPos;"
        "out vec3 Normal;"
    
        "void main() {"
        "WorldPos = uModel * vec4(iPosition, 1.0);"
        "gl_Position = uProj * uView * WorldPos;"
        "Color = iColor;"
        "Normal = (uModel * vec4(iNormal, 0.0)).xyz;"
        "}";

    static const char vox_fragment_glsl[] =
        "in vec3 Color;"
        "in vec3 Normal;"
        "in vec4 WorldPos;"
        "uniform mat4 uColorRemap;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform sampler2D uLightmap;"
        "uniform vec2 uMapDims;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec3 Remapped ="
        "    (uColorRemap[0].rgb * Color.r)+"
        "    (uColorRemap[1].rgb * Color.g)+"
        "    (uColorRemap[2].rgb * Color.b);"
        "vec2 TexCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, TexCoord);"
        "FragColor = vec4(Remapped, 1.0) * uLightFactor * (Lightmap + uLightColor) + uColorAdd;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";

	buildVoxelShader(voxelShader, "voxelShader", true,
		vox_vertex_glsl, sizeof(vox_vertex_glsl),
		vox_fragment_glsl, sizeof(vox_fragment_glsl));
    
    static const char vox_bright_fragment_glsl[] =
        "in vec3 Color;"
        "in vec3 Normal;"
        "in vec4 WorldPos;"
        "uniform mat4 uColorRemap;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec3 Remapped ="
        "    (uColorRemap[0].rgb * Color.r)+"
        "    (uColorRemap[1].rgb * Color.g)+"
        "    (uColorRemap[2].rgb * Color.b);"
        "FragColor = vec4(Remapped, 1.0) * uLightFactor * uLightColor + uColorAdd;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";

    buildVoxelShader(voxelBrightShader, "voxelBrightShader", false,
        vox_vertex_glsl, sizeof(vox_vertex_glsl),
        vox_bright_fragment_glsl, sizeof(vox_bright_fragment_glsl));

	static const char vox_dithered_fragment_glsl[] =
		"in vec3 Color;"
        "in vec3 Normal;"
		"in vec4 WorldPos;"
        "uniform float uDitherAmount;"
		"uniform mat4 uColorRemap;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform sampler2D uLightmap;"
		"uniform vec2 uMapDims;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void dither(ivec2 pos, float amount) {"
        "if (amount > 1.0) {"
        "int d = int(amount) - 1;"
        "if ((pos.x & d) == 0 && (pos.y & d) == 0) { discard; }"
        "} else if (amount == 1.0) {"
        "if (((pos.x + pos.y) & 1) == 0) { discard; }"
        "} else if (amount < 1.0) {"
        "int d = int(1.0 / amount) - 1;"
        "if ((pos.x & d) != 0 || (pos.y & d) != 0) { discard; }"
        "}"
        "}"

		"void main() {"
		"dither(ivec2(gl_FragCoord), uDitherAmount);"
		"vec3 Remapped ="
		"    (uColorRemap[0].rgb * Color.r)+"
		"    (uColorRemap[1].rgb * Color.g)+"
		"    (uColorRemap[2].rgb * Color.b);"
		"vec2 TexCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, TexCoord);"
        "FragColor = vec4(Remapped, 1.0) * uLightFactor * (Lightmap + uLightColor) + uColorAdd;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
		"}";

	buildVoxelShader(voxelDitheredShader, "voxelDitheredShader", true,
		vox_vertex_glsl, sizeof(vox_vertex_glsl),
		vox_dithered_fragment_glsl, sizeof(vox_dithered_fragment_glsl));

	static const char vox_bright_dithered_fragment_glsl[] =
		"in vec3 Color;"
		"in vec3 Normal;"
		"in vec4 WorldPos;"
		"uniform float uDitherAmount;"
		"uniform mat4 uColorRemap;"
		"uniform vec4 uLightFactor;"
		"uniform vec4 uLightColor;"
		"uniform vec4 uColorAdd;"
		"uniform vec4 uCameraPos;"
		"uniform sampler2D uLightmap;"
		"uniform vec2 uMapDims;"
		"uniform float uFogDistance;"
		"uniform vec4 uFogColor;"
		"out vec4 FragColor;"

		"void dither(ivec2 pos, float amount) {"
		"if (amount > 1.0) {"
		"int d = int(amount) - 1;"
		"if ((pos.x & d) == 0 && (pos.y & d) == 0) { discard; }"
		"} else if (amount == 1.0) {"
		"if (((pos.x + pos.y) & 1) == 0) { discard; }"
		"} else if (amount < 1.0) {"
		"int d = int(1.0 / amount) - 1;"
		"if ((pos.x & d) != 0 || (pos.y & d) != 0) { discard; }"
		"}"
		"}"

		"void main() {"
		"dither(ivec2(gl_FragCoord), uDitherAmount);"
		"vec3 Remapped ="
		"    (uColorRemap[0].rgb * Color.r)+"
		"    (uColorRemap[1].rgb * Color.g)+"
		"    (uColorRemap[2].rgb * Color.b);"
		"FragColor = vec4(Remapped, 1.0) * uLightFactor * uLightColor + uColorAdd;"

		"if (uFogDistance > 0.0) {"
		"float dist = length(uCameraPos.xyz - WorldPos.xyz);"
		"float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
		"vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
		"FragColor = vec4(mixed, FragColor.a);"
		"}"
		"}";

	buildVoxelShader(voxelBrightDitheredShader, "voxelBrightDitheredShader", true,
		vox_vertex_glsl, sizeof(vox_vertex_glsl),
		vox_bright_dithered_fragment_glsl, sizeof(vox_bright_dithered_fragment_glsl));
    
    // world shader:
    
    static const char world_vertex_glsl[] =
        "in vec3 iPosition;"
        "in vec2 iTexCoord;"
        "in vec3 iColor;"
        "uniform mat4 uProj;"
        "uniform mat4 uView;"
        "out vec2 TexCoord;"
        "out vec3 Color;"
        "out vec4 WorldPos;"
    
        "void main() {"
        "WorldPos = vec4(iPosition, 1.0);"
        "gl_Position = uProj * uView * WorldPos;"
        "TexCoord = iTexCoord;"
        "Color = iColor;"
        "}";
    
    static const char world_fragment_glsl[] =
        "in vec2 TexCoord;"
        "in vec3 Color;"
        "in vec4 WorldPos;"
        "uniform vec4 uLightFactor;"
        "uniform sampler2D uTextures;"
        "uniform sampler2D uLightmap;"
        "uniform vec2 uMapDims;"
        "uniform vec4 uCameraPos;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec2 LightCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, LightCoord);"
        "FragColor = texture(uTextures, TexCoord) * vec4(Color, 1.f) * uLightFactor * Lightmap;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";
    
    buildWorldShader(worldShader, "worldShader", true,
        world_vertex_glsl, sizeof(world_vertex_glsl),
        world_fragment_glsl, sizeof(world_fragment_glsl));
    
    static const char world_dithered_fragment_glsl[] =
        "in vec2 TexCoord;"
        "in vec3 Color;"
        "in vec4 WorldPos;"
        "uniform float uDitherAmount;"
        "uniform vec4 uLightFactor;"
        "uniform sampler2D uTextures;"
        "uniform sampler2D uLightmap;"
        "uniform vec2 uMapDims;"
        "uniform vec4 uCameraPos;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void dither(ivec2 pos, float amount) {"
        "if (amount > 1.0) {"
        "int d = int(amount) - 1;"
        "if ((pos.x & d) == 0 && (pos.y & d) == 0) { discard; }"
        "} else if (amount == 1.0) {"
        "if (((pos.x + pos.y) & 1) == 0) { discard; }"
        "} else if (amount < 1.0) {"
        "int d = int(1.0 / amount) - 1;"
        "if ((pos.x & d) != 0 || (pos.y & d) != 0) { discard; }"
        "}"
        "}"
    
        "void main() {"
        "dither(ivec2(gl_FragCoord), uDitherAmount);"
        "vec2 LightCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, LightCoord);"
        "FragColor = texture(uTextures, TexCoord) * vec4(Color, 1.f) * uLightFactor * Lightmap;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";
    
    buildWorldShader(worldDitheredShader, "worldDitheredShader", true,
        world_vertex_glsl, sizeof(world_vertex_glsl),
        world_dithered_fragment_glsl, sizeof(world_dithered_fragment_glsl));
    
    static const char world_dark_fragment_glsl[] =
        "in vec2 TexCoord;"
        "in vec3 Color;"
        "in vec4 WorldPos;"
        "out vec4 FragColor;"
        "void main() {"
        "FragColor = vec4(0.0, 0.0, 0.0, 1.0);"
        "}";
    
    buildWorldShader(worldDarkShader, "worldDarkShader", false,
        world_vertex_glsl, sizeof(world_vertex_glsl),
        world_dark_fragment_glsl, sizeof(world_dark_fragment_glsl));
    
    // sky shader:
    
    static const char sky_vertex_glsl[] =
        "in vec3 iPosition;"
        "in vec2 iTexCoord;"
        "in vec4 iColor;"
        "uniform mat4 uProj;"
        "uniform mat4 uView;"
        "uniform vec2 uScroll;"
        "out vec2 TexCoord;"
        "out vec2 Scroll;"
        "out vec4 Color;"
    
        "void main() {"
        "mat4 View = uView;"
        "View[3] = vec4(0.f, 0.f, 0.f, 1.f);"
        "gl_Position = uProj * View * vec4(iPosition, 1.0);"
        "TexCoord = iTexCoord;"
        "Color = iColor;"
        "Scroll = (Color.a > 0.75) ? uScroll.xx : uScroll.yy;"
        "}";
    
    static const char sky_fragment_glsl[] =
        "in vec2 TexCoord;"
        "in vec2 Scroll;"
        "in vec4 Color;"
        "uniform vec4 uLightFactor;"
        "uniform sampler2D uTexture;"
        "out vec4 FragColor;"
        "void main() {"
        "FragColor = texture(uTexture, TexCoord + Scroll) * Color * uLightFactor;"
        "}";
    
    skyShader.init("skyShader");
    skyShader.compile(sky_vertex_glsl, sizeof(sky_vertex_glsl), Shader::Type::Vertex);
    skyShader.compile(sky_fragment_glsl, sizeof(sky_fragment_glsl), Shader::Type::Fragment);
    skyShader.bindAttribLocation("iPosition", 0);
    skyShader.bindAttribLocation("iTexCoord", 1);
    skyShader.bindAttribLocation("iColor", 2);
    skyShader.link();
    skyShader.bind();
    GL_CHECK_ERR(glUniform1i(skyShader.uniform("uTexture"), 0));
    
    skyMesh.init();
    
    // sprite shader:
    
    static const char sprite_vertex_glsl[] =
        "in vec3 iPosition;"
        "in vec2 iTexCoord;"
        "uniform mat4 uProj;"
        "uniform mat4 uView;"
        "uniform mat4 uModel;"
        "out vec4 WorldPos;"
        "out vec2 TexCoord;"
    
        "void main() {"
        "WorldPos = uModel * vec4(iPosition, 1.0);"
        "TexCoord = iTexCoord;"
        "gl_Position = uProj * uView * WorldPos;"
        "}";

    static const char sprite_fragment_glsl[] =
        "in vec4 WorldPos;"
        "in vec2 TexCoord;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform sampler2D uTexture;"
        "uniform sampler2D uLightmap;"
        "uniform vec2 uMapDims;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec4 Texture = texture(uTexture, TexCoord);"
        "vec2 LightCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, LightCoord);"
        "FragColor = Texture * uLightFactor * (Lightmap + uLightColor) + uColorAdd;"
        "if (FragColor.a <= 0) discard;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";

    buildSpriteShader(spriteShader, "spriteShader", true,
        sprite_vertex_glsl, sizeof(sprite_vertex_glsl),
        sprite_fragment_glsl, sizeof(sprite_fragment_glsl));
    
    static const char sprite_dithered_fragment_glsl[] =
        "in vec4 WorldPos;"
        "in vec2 TexCoord;"
        "uniform float uDitherAmount;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform sampler2D uTexture;"
        "uniform sampler2D uLightmap;"
        "uniform vec2 uMapDims;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void dither(ivec2 pos, float amount) {"
        "if (amount > 1.0) {"
        "int d = int(amount) - 1;"
        "if ((pos.x & d) == 0 && (pos.y & d) == 0) { discard; }"
        "} else if (amount == 1.0) {"
        "if (((pos.x + pos.y) & 1) == 0) { discard; }"
        "} else if (amount < 1.0) {"
        "int d = int(1.0 / amount) - 1;"
        "if ((pos.x & d) != 0 || (pos.y & d) != 0) { discard; }"
        "}"
        "}"

        "void main() {"
        "dither(ivec2(gl_FragCoord), uDitherAmount);"
        "vec4 Texture = texture(uTexture, TexCoord);"
        "vec2 LightCoord = WorldPos.xz / (uMapDims.xy * 32.0);"
        "vec4 Lightmap = texture(uLightmap, LightCoord);"
        "FragColor = Texture * uLightFactor * (Lightmap + uLightColor) + uColorAdd;"
        "if (FragColor.a <= 0) discard;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";

    buildSpriteShader(spriteDitheredShader, "spriteDitheredShader", true,
        sprite_vertex_glsl, sizeof(sprite_vertex_glsl),
        sprite_dithered_fragment_glsl, sizeof(sprite_dithered_fragment_glsl));

    static const char sprite_bright_fragment_glsl[] =
        "in vec4 WorldPos;"
        "in vec2 TexCoord;"
        "uniform vec4 uLightFactor;"
        "uniform vec4 uLightColor;"
        "uniform vec4 uColorAdd;"
        "uniform vec4 uCameraPos;"
        "uniform sampler2D uTexture;"
        "uniform float uFogDistance;"
        "uniform vec4 uFogColor;"
        "out vec4 FragColor;"
    
        "void main() {"
        "vec4 Texture = texture(uTexture, TexCoord);"
        "FragColor = Texture * uLightFactor * uLightColor + uColorAdd;"
        "if (FragColor.a <= 0) discard;"
        
        "if (uFogDistance > 0.0) {"
        "float dist = length(uCameraPos.xyz - WorldPos.xyz);"
        "float lerp = (min(dist, uFogDistance) / uFogDistance) * uFogColor.a;"
        "vec3 mixed = mix(FragColor.rgb, uFogColor.rgb, lerp);"
        "FragColor = vec4(mixed, FragColor.a);"
        "}"
        "}";

    buildSpriteShader(spriteBrightShader, "spriteBrightShader", false,
        sprite_vertex_glsl, sizeof(sprite_vertex_glsl),
        sprite_bright_fragment_glsl, sizeof(sprite_bright_fragment_glsl));

    buildSpriteShader(spriteUIShader, "spriteUIShader", false,
        sprite_vertex_glsl, sizeof(sprite_vertex_glsl),
        sprite_bright_fragment_glsl, sizeof(sprite_bright_fragment_glsl));
    
    spriteMesh.init();
    
    // 2d lines
    lineMesh.init();
}

void destroyCommonDrawResources() {
	framebuffer::mesh.destroy();
	framebuffer::shader.destroy();
    framebuffer::hdrShader.destroy();
    voxelShader.destroy();
    voxelBrightShader.destroy();
	voxelDitheredShader.destroy();
	voxelBrightDitheredShader.destroy();
    worldShader.destroy();
    worldDitheredShader.destroy();
    worldDarkShader.destroy();
    skyShader.destroy();
    skyMesh.destroy();
    spriteShader.destroy();
    spriteDitheredShader.destroy();
    spriteBrightShader.destroy();
    spriteUIShader.destroy();
    spriteMesh.destroy();
    lineShader.destroy();
    lineMesh.destroy();
    gearShader.destroy();
#ifndef EDITOR
	cleanupMinimapTextures();
#endif
    clearChunks();
    for (int c = 0; c < MAXPLAYERS + 1; ++c) {
        delete lightmapTexture[c];
    }
}

void Mesh::init() {
	if (isInitialized()) {
		return;
	}
    
    // NOTE: OpenGL 2.1 does not support vertex array functions
#ifdef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glGenVertexArrays(1, &vao));
    GL_CHECK_ERR(glBindVertexArray(vao));
#endif

	// data buffers
    numVertices = 0;
    GL_CHECK_ERR(glGenBuffers((GLsizei)BufferType::Max, vbo));
	for (unsigned int c = 0; c < (unsigned int)BufferType::Max; ++c) {
        if (data[c].size()) {
            const auto& find = ElementsPerVBO.find((BufferType)c);
            assert(find != ElementsPerVBO.end());
            numVertices = std::max(numVertices, (unsigned int)data[c].size() / find->second);
            GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo[c]));
            GL_CHECK_ERR(glBufferData(GL_ARRAY_BUFFER, data[c].size() * sizeof(float), data[c].data(), GL_STATIC_DRAW));
#ifdef VERTEX_ARRAYS_ENABLED
            GL_CHECK_ERR(glVertexAttribPointer(c, find->second, GL_FLOAT, GL_FALSE, 0, nullptr));
            GL_CHECK_ERR(glEnableVertexAttribArray(c));
#endif
        }
	}
#ifndef VERTEX_ARRAYS_ENABLED
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif

	printlog("initialized mesh with %llu vertices", numVertices);
}

void Mesh::destroy() {
    if (vao) {
        GL_CHECK_ERR(glDeleteVertexArrays(1, &vao));
        vao = 0;
    }
	for (int c = 0; c < (int)BufferType::Max; ++c) {
		if (vbo[c]) {
            GL_CHECK_ERR(glDeleteBuffers(1, &vbo[c]));
			vbo[c] = 0;
		}
	}
}

void Mesh::draw(GLenum type, int numVertices) const {
    // NOTE: OpenGL 2.1 does not support vertex arrays!
#ifdef VERTEX_ARRAYS_ENABLED
	GL_CHECK_ERR(glBindVertexArray(vao));
#endif
    
    if (numVertices == 0) {
        numVertices = this->numVertices;
    }
    
    // bind buffers
#ifndef VERTEX_ARRAYS_ENABLED
    for (unsigned int c = 0; c < (unsigned int)BufferType::Max; ++c) {
        if (data[c].size()) {
            const auto& find = ElementsPerVBO.find((BufferType)c);
            assert(find != ElementsPerVBO.end());
            GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, vbo[c]));
            GL_CHECK_ERR(glVertexAttribPointer(c, find->second, GL_FLOAT, GL_FALSE, 0, nullptr));
            GL_CHECK_ERR(glEnableVertexAttribArray(c));
        }
    }
#endif
    
    // draw elements
    if (numVertices) {
        GL_CHECK_ERR(glDrawArrays(type, 0, numVertices));
    }
    
    // disable buffers
#ifndef VERTEX_ARRAYS_ENABLED
    for (unsigned int c = 0; c < (unsigned int)BufferType::Max; ++c) {
        if (data[c].size()) {
            GL_CHECK_ERR(glDisableVertexAttribArray(c));
        }
    }
    GL_CHECK_ERR(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
}

void framebuffer::init(unsigned int _xsize, unsigned int _ysize, GLint minFilter, GLint magFilter) {
    if (fbo) {
        if (xsize == _xsize && ysize == _ysize) {
            return;
        }
        destroy();
    }
	xsize = _xsize;
	ysize = _ysize;
    
    GL_CHECK_ERR(glGenTextures(1, &fbo_color));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, fbo_color));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
	GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, xsize, ysize, 0, GL_RGBA, GL_HALF_FLOAT, nullptr));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, 0));

    GL_CHECK_ERR(glGenTextures(1, &fbo_depth));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, fbo_depth));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
    GL_CHECK_ERR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
    GL_CHECK_ERR(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8,
        xsize, ysize, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, 0));
    
    GL_CHECK_ERR(glGenFramebuffers(1, &fbo));
    GL_CHECK_ERR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    GL_CHECK_ERR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0));
    GL_CHECK_ERR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0));
    static const GLenum attachments[] = {GL_COLOR_ATTACHMENT0};
    GL_CHECK_ERR(glDrawBuffers(sizeof(attachments) / sizeof(GLenum), attachments));
    GL_CHECK_ERR(glReadBuffer(GL_COLOR_ATTACHMENT0));

    GL_CHECK_ERR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

GLhalf* framebuffer::lock() {
    if (!fbo || mapped) {
        return nullptr;
    }
    
    // map data from the current pixel buffer
    if (pbos[pboindex]) {
		GL_CHECK_ERR(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pboindex]));
		auto result = GL_CHECK_ERR_RET(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
		if (result) {
			mapped = true;
		}
		return (GLhalf*)result;
	} else {
		return nullptr;
	}
}

void framebuffer::unlock() {
    if (!fbo) {
        return;
    }
    
    // unmap pixel pack buffer
	if (mapped) {
		GL_CHECK_ERR(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
		mapped = false;
	}
    
    // select next pbo
    pboindex = (pboindex + 1) % NUM_PBOS;
    
    // start filling a new pixel buffer
    if (pbos[pboindex] == 0) {
        GL_CHECK_ERR(glGenBuffers(1, &pbos[pboindex]));
        GL_CHECK_ERR(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pboindex]));
        GL_CHECK_ERR(glBufferData(GL_PIXEL_PACK_BUFFER, xsize * ysize * 4 * sizeof(GLhalf), nullptr, GL_STREAM_READ));
    }
    GL_CHECK_ERR(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pboindex]));
    GL_CHECK_ERR(glReadPixels(0, 0, xsize, ysize, GL_RGBA, GL_HALF_FLOAT, nullptr));
    GL_CHECK_ERR(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
}

void framebuffer::destroy() {
	if (mapped) {
		GL_CHECK_ERR(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
		mapped = false;
	}
    if (fbo) {
        GL_CHECK_ERR(glDeleteFramebuffers(1, &fbo));
        fbo = 0;
    }
    if (fbo_color) {
        GL_CHECK_ERR(glDeleteTextures(1, &fbo_color));
        fbo_color = 0;
    }
    if (fbo_depth) {
        GL_CHECK_ERR(glDeleteTextures(1, &fbo_depth));
        fbo_depth = 0;
    }
    for (int c = 0; c < NUM_PBOS; ++c) {
        if (pbos[c]) {
            GL_CHECK_ERR(glDeleteBuffers(1, &pbos[c]));
            pbos[c] = 0;
        }
    }
}

static std::vector<framebuffer*> fbStack;

void framebuffer::bindForWriting() {
    if (fbo) {
        GL_CHECK_ERR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        GL_CHECK_ERR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0));
        GL_CHECK_ERR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0));
        GL_CHECK_ERR(glViewport(0, 0, xsize, ysize));
        fbStack.push_back(this);
    }
}

void framebuffer::bindForReading() const {
    GL_CHECK_ERR(glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, fbo_color));
}

void framebuffer::draw(float brightness) {
	shader.bind();
    GL_CHECK_ERR(glUniform1f(shader.uniform("uBrightness"), brightness));
	mesh.draw();
}

void framebuffer::hdrDraw(const Vector4& brightness, float gamma, float exposure) {
    hdrShader.bind();
    GL_CHECK_ERR(glUniform4fv(hdrShader.uniform("uBrightness"), 1, (float*)&brightness));
    GL_CHECK_ERR(glUniform1f(hdrShader.uniform("uGamma"), gamma));
    GL_CHECK_ERR(glUniform1f(hdrShader.uniform("uExposure"), exposure));
    mesh.draw();
}

void framebuffer::unbindForWriting() {
    if (!fbStack.empty()) {
        fbStack.pop_back();
    }
    if (fbStack.empty()) {
        GL_CHECK_ERR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL_CHECK_ERR(glViewport(0, 0, xres, yres));
    } else {
        auto fb = fbStack.back();
        fbStack.pop_back();
        fb->bindForWriting();
    }
}

void framebuffer::unbindForReading() {
    GL_CHECK_ERR(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
    GL_CHECK_ERR(glBindTexture(GL_TEXTURE_2D, 0));
}

void framebuffer::unbindAll() {
    unbindForWriting();
    unbindForReading();
}

/*-------------------------------------------------------------------------------

	getPixel

	gets the value of a pixel at the given x,y location in the given
	SDL_Surface

-------------------------------------------------------------------------------*/

Uint32 getPixel(SDL_Surface* surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to retrieve
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			return *p;
			break;

		case 2:
			return *(Uint16*)p;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				return p[0] << 16 | p[1] << 8 | p[2];
			}
			else
			{
				return p[0] | p[1] << 8 | p[2] << 16;
			}
			break;

		case 4:
			return *(Uint32*)p;
			break;

		default:
			return 0;	   /* shouldn't happen, but avoids warnings */
	}
}

/*-------------------------------------------------------------------------------

	putPixel

	sets the value of a pixel at the given x,y location in the given
	SDL_Surface

-------------------------------------------------------------------------------*/

void putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
	int bpp = surface->format->BytesPerPixel;
	// Here p is the address to the pixel we want to set
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16*)p = pixel;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}
			else
			{
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32*)p = pixel;
			break;
	}
}

/*-------------------------------------------------------------------------------

	flipSurface

	flips the contents of an SDL_Surface horizontally, vertically, or both

-------------------------------------------------------------------------------*/

SDL_Surface* flipSurface( SDL_Surface* surface, int flags )
{
	SDL_Surface* flipped = NULL;
	Uint32 pixel;
	int x, rx;
	int y, ry;

	// prepare surface for flipping
	flipped = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask );
	if ( SDL_MUSTLOCK( surface ) )
	{
		SDL_LockSurface( surface );
	}
	if ( SDL_MUSTLOCK( flipped ) )
	{
		SDL_LockSurface( flipped );
	}

	for ( x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx-- )
	{
		for ( y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry-- )
		{
			pixel = getPixel( surface, x, y );

			// copy pixel
			if ( ( flags & FLIP_VERTICAL ) && ( flags & FLIP_HORIZONTAL ) )
			{
				putPixel( flipped, rx, ry, pixel );
			}
			else if ( flags & FLIP_HORIZONTAL )
			{
				putPixel( flipped, rx, y, pixel );
			}
			else if ( flags & FLIP_VERTICAL )
			{
				putPixel( flipped, x, ry, pixel );
			}
		}
	}

	// restore image
	if ( SDL_MUSTLOCK( surface ) )
	{
		SDL_UnlockSurface( surface );
	}
	if ( SDL_MUSTLOCK( flipped ) )
	{
		SDL_UnlockSurface( flipped );
	}

	return flipped;
}

/*-------------------------------------------------------------------------------

drawCircle

draws a circle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawCircle( int x, int y, real_t radius, Uint32 color, Uint8 alpha )
{
	drawArc(x, y, radius, 0, 360, color, alpha);
}

/*-------------------------------------------------------------------------------

	drawArc

	draws an arc in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawArc( int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha )
{
	// deprecated
}

/*-------------------------------------------------------------------------------

drawScalingFilledArc

draws an arc with a changing radius

-------------------------------------------------------------------------------*/

static void drawScalingFilledArc(int x, int y, real_t radius1, real_t radius2, real_t angle1, real_t angle2, Uint32 inner_color, Uint32 outer_color)
{
    // initialize shader if needed, then bind
    if (!gearShader.isInitialized()) {
        static const char v_glsl[] =
            "in vec4 iPosition;"
            "void main() {"
            "gl_Position = iPosition;"
            "}";
        
        static const char g_glsl[] =
            "layout (points) in;"
            "layout (triangle_strip, max_vertices = 64) out;"
        
            "uniform mat4 uProj;"
            "uniform mat4 uView;"
            "uniform vec4 uInnerColor;"
            "uniform vec4 uOuterColor;"
            "uniform float uRadius1;"
            "uniform float uRadius2;"
            "uniform float uAngle1;"
            "uniform float uAngle2;"
            "out vec4 Color;"
        
            "void Emit(vec2 position, vec4 color) {"
            "gl_Position = uProj * uView * vec4(position.x, -position.y, 0.0, 1.0);"
            "Color = color;"
            "EmitVertex();"
            "}"
        
            "void main() {"
            "vec2 position = gl_in[0].gl_Position.xy;"
			"float step = 2.0;"
            "for (float c = uAngle2; c > uAngle1; c -= step) {"
            "Emit(position, uInnerColor);"
        
            "float factor1 = (c - uAngle1) / (uAngle2 - uAngle1);"
            "float radius1 = uRadius2 * factor1 + uRadius1 * (1.0 - factor1);"
            "Emit(position + vec2(cos(radians(c)) * radius1, sin(radians(c)) * radius1), uOuterColor);"
        
            "float factor2 = (c - uAngle1 - step) / (uAngle2 - uAngle1);"
            "float radius2 = uRadius2 * factor2 + uRadius1 * (1.0 - factor2);"
            "Emit(position + vec2(cos(radians(c - step)) * radius2, sin(radians(c - step)) * radius2), uOuterColor);"
        
            "EndPrimitive();"
            "}"
            "}";
        
        static const char f_glsl[] =
            "in vec4 Color;"
            "out vec4 FragColor;"
            "void main() {"
            "FragColor = Color;"
            "}";
        
        gearShader.init("gear shader");
        gearShader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        gearShader.compile(g_glsl, sizeof(g_glsl), Shader::Type::Geometry);
        gearShader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        gearShader.bindAttribLocation("iPosition", 0);
        
        //gearShader.setParameter(GL_GEOMETRY_VERTICES_OUT, 64);
        //gearShader.setParameter(GL_GEOMETRY_INPUT_TYPE, GL_POINTS);
        //gearShader.setParameter(GL_GEOMETRY_OUTPUT_TYPE, GL_TRIANGLES);
        gearShader.link();
    }
    gearShader.bind();
    GL_CHECK_ERR(glEnable(GL_BLEND));
    
    // upload radii and angles
    GL_CHECK_ERR(glUniform1f(gearShader.uniform("uRadius1"), (float)radius1));
    GL_CHECK_ERR(glUniform1f(gearShader.uniform("uRadius2"), (float)radius2));
    GL_CHECK_ERR(glUniform1f(gearShader.uniform("uAngle1"), (float)angle1));
    GL_CHECK_ERR(glUniform1f(gearShader.uniform("uAngle2"), (float)angle2));
    
    Uint8 r, g, b, a;
    
    // upload color
    getColor(inner_color, &r, &g, &b, &a);
    float icv[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    GL_CHECK_ERR(glUniform4fv(gearShader.uniform("uInnerColor"), 1, icv));
    getColor(outer_color, &r, &g, &b, &a);
    float ocv[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
    GL_CHECK_ERR(glUniform4fv(gearShader.uniform("uOuterColor"), 1, ocv));
    
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, 0, xres, 0, yres, -1.f, 1.f);
    GL_CHECK_ERR(glUniformMatrix4fv(gearShader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));
    
    // point matrix
    mat4x4 view(1.f);
    v = {(float)x, (float)(yres - y), 0.f, 0.f};
    (void)translate_mat(&m, &view, &v); view = m;
    GL_CHECK_ERR(glUniformMatrix4fv(gearShader.uniform("uView"), 1, GL_FALSE, (float*)&view));
    
    // draw line
    lineMesh.draw(GL_POINTS, 1);
    
    // reset GL state
    GL_CHECK_ERR(glDisable(GL_BLEND));
}

/*-------------------------------------------------------------------------------

drawArcInvertedY, reversing the angle of direction in the y coordinate.

draws an arc in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawArcInvertedY(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha)
{
    // deprecated
}

/*-------------------------------------------------------------------------------

	drawLine

	draws a line in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawLine( int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha )
{
    // read color
    Uint8 r, g, b, a;
    getColor(color, &r, &g, &b, &a);
    if (!alpha) {
        return;
    }
    
    // initialize shader if needed, then bind
    if (!lineShader.isInitialized()) {
        static const char v_glsl[] =
            "in vec4 iPosition;"
            "uniform mat4 uProj;"
            "uniform mat4 uMatrix0;"
            "uniform mat4 uMatrix1;"
            "void main() {"
            "if (gl_VertexID == 0) { gl_Position = uProj * uMatrix0 * iPosition; }"
            "else { gl_Position = uProj * uMatrix1 * iPosition; }"
            "}";
        
        static const char f_glsl[] =
            "uniform vec4 uColor;"
            "out vec4 FragColor;"
            "void main() {"
            "FragColor = uColor;"
            "}";
        
        lineShader.init("line shader");
        lineShader.compile(v_glsl, sizeof(v_glsl), Shader::Type::Vertex);
        lineShader.compile(f_glsl, sizeof(f_glsl), Shader::Type::Fragment);
        lineShader.bindAttribLocation("iPosition", 0);
        lineShader.link();
    }
    lineShader.bind();
    GL_CHECK_ERR(glEnable(GL_BLEND));
    
    // upload color
    float cv[] = {r / 255.f, g / 255.f, b / 255.f, alpha / 255.f};
    GL_CHECK_ERR(glUniform4fv(lineShader.uniform("uColor"), 1, cv));
    
    vec4_t v;
    mat4x4 m;
    
    // projection matrix
    mat4x4 proj(1.f);
    (void)ortho(&proj, 0, xres, 0, yres, -1.f, 1.f);
    GL_CHECK_ERR(glUniformMatrix4fv(lineShader.uniform("uProj"), 1, GL_FALSE, (float*)&proj));
    
    // point 1 matrix
    mat4x4 view0(1.f);
    v = {(float)x1, (float)(yres - y1), 0.f, 0.f};
    (void)translate_mat(&m, &view0, &v); view0 = m;
    GL_CHECK_ERR(glUniformMatrix4fv(lineShader.uniform("uMatrix0"), 1, GL_FALSE, (float*)&view0));
    
    // point 2 matrix
    mat4x4 view1(1.f);
    v = {(float)x2, (float)(yres - y2), 0.f, 0.f};
    (void)translate_mat(&m, &view1, &v); view1 = m;
    GL_CHECK_ERR(glUniformMatrix4fv(lineShader.uniform("uMatrix1"), 1, GL_FALSE, (float*)&view1));
    
    // draw line
    lineMesh.draw(GL_LINES, 2);
    
    // reset GL state
    GL_CHECK_ERR(glDisable(GL_BLEND));
}

/*-------------------------------------------------------------------------------

	drawRect

	draws a rectangle in either an opengl or SDL context

-------------------------------------------------------------------------------*/

int drawRect( SDL_Rect* src, Uint32 color, Uint8 alpha )
{
	SDL_Rect secondsrc;
	if ( src == NULL )
	{
		secondsrc.x = 0;
		secondsrc.y = 0;
		secondsrc.w = xres;
		secondsrc.h = yres;
		src = &secondsrc;
	}
	Uint32 c = (color & 0x00ffffff) | ((Uint32)alpha << 24);
	auto image = Image::get("images/system/white.png");
	image->drawColor(nullptr, *src, SDL_Rect{0, 0, xres, yres}, c);
	return 0;
}


/*-------------------------------------------------------------------------------

	drawBox

	draws the border of a rectangle

-------------------------------------------------------------------------------*/
int drawBox(SDL_Rect* src, Uint32 color, Uint8 alpha)
{
	drawLine(src->x, src->y, src->x + src->w, src->y, color, alpha); //Top.
	drawLine(src->x, src->y, src->x, src->y + src->h, color, alpha); //Left.
	drawLine(src->x + src->w, src->y, src->x + src->w, src->y + src->h, color, alpha); //Right.
	drawLine(src->x, src->y + src->h, src->x + src->w, src->y + src->h, color, alpha); //Bottom.
	return 0;
}

/*-------------------------------------------------------------------------------

	drawGear

	draws a gear (used for turning wheel splash)

-------------------------------------------------------------------------------*/

void drawGear(Sint16 x, Sint16 y, real_t size, Sint32 rotation)
{
	const Uint32 black = makeColor(0, 0, 0, 255);
	const Uint32 color_dark = makeColor(255, 32, 0, 255);
	const Uint32 color = makeColor(255, 76, 49, 255);
	const Uint32 color_bright = makeColor(255, 109, 83, 255);
	const real_t teeth_size = size + size / 3;
	const int num_teeth = 6;
	for ( int c = 0; c < num_teeth; c++ )
	{
		real_t p = 180.0 / (real_t)num_teeth;
		real_t r = (real_t)c * (p * 2.0) + (real_t)rotation;
		real_t t = 4.0;
		drawScalingFilledArc(x, y, size, size,
			r,
			r + p,
            color_bright, color);
		drawScalingFilledArc(x, y, size, teeth_size,
			r + p,
			r + p + t,
            color_bright, color);
		drawScalingFilledArc(x, y, teeth_size, teeth_size,
			r + p + t,
			r + p * 2 - t,
            color_bright, color);
		drawScalingFilledArc(x, y, teeth_size, size,
			r + p * 2 - t,
			r + p * 2,
            color_bright, color);
	}
	for (int c = 0; c < 360; c += 10) {
		drawScalingFilledArc(x, y,
			size * 1 / 3,
			size * 1 / 3,
			c, c + 10,
			color_dark, color);
		drawScalingFilledArc(x, y,
			size * 1 / 6,
			size * 1 / 6,
			c, c + 10,
			black, black);
	}
}

/*-------------------------------------------------------------------------------

	drawImageRotatedAlpha

	blits an image in either an opengl or SDL context, rotating the image
	relative to the screen and taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageRotatedAlpha( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, real_t angle, Uint8 alpha )
{
    if (!image || !pos) {
        return;
    }
    Uint32 color = makeColor(255, 255, 255, alpha);
    Image::draw(texid[(long int)image->userdata], image->w, image->h,
        src, *pos, SDL_Rect{0, 0, xres, yres}, color, angle);
}

/*-------------------------------------------------------------------------------

	drawImageColor

	blits an image in either an opengl or SDL context while colorizing it

-------------------------------------------------------------------------------*/

void drawImageColor( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color )
{
    if (!image || !pos) {
        return;
    }
    Image::draw(texid[(long int)image->userdata], image->w, image->h,
        src, *pos, SDL_Rect{0, 0, xres, yres}, color);
}

/*-------------------------------------------------------------------------------

	drawImageAlpha

	blits an image in either an opengl or SDL context, taking an alpha value

-------------------------------------------------------------------------------*/

void drawImageAlpha( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint8 alpha )
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	drawImage

	blits an image in either an opengl or SDL context

-------------------------------------------------------------------------------*/

void drawImage( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos )
{
    if (!image || !pos) {
        return;
    }
    Image::draw(texid[(long int)image->userdata], image->w, image->h,
        src, *pos, SDL_Rect{0, 0, xres, yres}, 0xffffffff);
}

/*-------------------------------------------------------------------------------

drawImageRing

blits an image in either an opengl or SDL context into a 2d ring.

-------------------------------------------------------------------------------*/

void drawImageRing(SDL_Surface* image, SDL_Rect* src, int radius, int thickness, int segments, real_t angStart, real_t angEnd, Uint8 alpha)
{
    // deprecated
}

/*-------------------------------------------------------------------------------

	drawImageScaled

	blits an image in either an opengl or SDL context, scaling it

-------------------------------------------------------------------------------*/

void drawImageScaled( SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos )
{
    if (!image || !pos) {
        return;
    }
    Image::draw(texid[(long int)image->userdata], image->w, image->h,
        src, *pos, SDL_Rect{0, 0, xres, yres}, 0xffffffff);
}

/*-------------------------------------------------------------------------------

drawImageScaledPartial

blits an image in either an opengl or SDL context, scaling it

-------------------------------------------------------------------------------*/

void drawImageScaledPartial(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, float percentY)
{
    // deprecated
}

/*-------------------------------------------------------------------------------

drawImageScaledColor

blits an image in either an opengl or SDL context while colorizing and scaling it

-------------------------------------------------------------------------------*/

void drawImageScaledColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color)
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	scaleSurface

	Scales an SDL_Surface to the given width and height.

-------------------------------------------------------------------------------*/

SDL_Surface* scaleSurface(SDL_Surface* Surface, Uint16 Width, Uint16 Height)
{
	Sint32 x, y, o_x, o_y;

	if (!Surface || !Width || !Height)
	{
		return NULL;
	}

	SDL_Surface* _ret = SDL_CreateRGBSurface(Surface->flags, Width, Height, Surface->format->BitsPerPixel, Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);

	real_t _stretch_factor_x = (real_t)Width / (real_t)Surface->w;
	real_t _stretch_factor_y = (real_t)Height / (real_t)Surface->h;

	for (y = 0; y < Surface->h; y++)
		for (x = 0; x < Surface->w; x++)
			for (o_y = 0; o_y < _stretch_factor_y; ++o_y)
				for (o_x = 0; o_x < _stretch_factor_x; ++o_x)
				{
					putPixel(_ret, (Sint32)(_stretch_factor_x * x) + o_x, (Sint32)(_stretch_factor_y * y) + o_y, getPixel(Surface, x, y));
				}

	free(Surface);
	return _ret;
}

/*-------------------------------------------------------------------------------

	drawImageFancy

	blits an image in either an opengl or SDL context, while coloring,
	rotating, and scaling it

-------------------------------------------------------------------------------*/

void drawImageFancy( SDL_Surface* image, Uint32 color, real_t angle, SDL_Rect* src, SDL_Rect* pos )
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	drawSky3D

	Draws the sky as an image whose position depends upon the given camera
	position

-------------------------------------------------------------------------------*/

void drawSky3D( view_t* camera, SDL_Surface* tex )
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	drawLayer / drawBackground / drawForeground

	Draws the world tiles that are viewable at the given camera coordinates

-------------------------------------------------------------------------------*/

void drawLayer(long camx, long camy, int z, map_t* map)
{
	long x, y;
	long minx, miny, maxx, maxy;
	int index;
	SDL_Rect pos;

	minx = std::max<long int>(camx >> TEXTUREPOWER, 0);
	maxx = std::min<long int>((camx >> TEXTUREPOWER) + xres / TEXTURESIZE + 2, map->width); //TODO: Why are long int and unsigned int being compared?
	miny = std::max<long int>(camy >> TEXTUREPOWER, 0);
	maxy = std::min<long int>((camy >> TEXTUREPOWER) + yres / TEXTURESIZE + 2, map->height); //TODO: Why are long int and unsigned int being compared?
	for ( y = miny; y < maxy; y++ )
	{
		for ( x = minx; x < maxx; x++ )
		{
			index = map->tiles[z + y * MAPLAYERS + x * MAPLAYERS * map->height];
			if ( index > 0)
			{
				pos.x = (int)((x << TEXTUREPOWER) - camx);
				pos.y = (int)((y << TEXTUREPOWER) - camy);
				pos.w = TEXTURESIZE;
				pos.h = TEXTURESIZE;
				if ( index >= 0 && index < numtiles )
				{
					if ( tiles[index] != NULL )
					{
						drawImageScaled(tiles[index], NULL, &pos);
					}
					else
					{
						drawImageScaled(sprites[0], NULL, &pos);
					}
				}
				else
				{
					drawImageScaled(sprites[0], NULL, &pos);
				}
			}
		}
	}
}

void drawBackground(long camx, long camy)
{
	for ( int z = 0; z < OBSTACLELAYER; z++ )
	{
		drawLayer(camx, camy, z, &map);
	}
}

void drawForeground(long camx, long camy)
{
	for ( int z = OBSTACLELAYER; z < MAPLAYERS; z++ )
	{
		drawLayer(camx, camy, z, &map);
	}
}

/*-------------------------------------------------------------------------------

	drawClearBuffers

	clears the screen and resets zbuffer

-------------------------------------------------------------------------------*/

void drawClearBuffers()
{
    GL_CHECK_ERR(glClearColor(0.f, 0.f, 0.f, 1.f));
    GL_CHECK_ERR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

/*-------------------------------------------------------------------------------

	raycast

	Performs raycasting from the given camera's position through the
	environment to update minimap

-------------------------------------------------------------------------------*/

#include <future>

#ifndef EDITOR
#include "net.hpp"
#endif

void raycast(const view_t& camera, Sint8 (*minimap)[MINIMAP_MAX_DIMENSION], bool fillWithColor)
{
    // originally we cast a ray for every column of pixels in the
    // camera viewport. now we just shoot out a few hundred rays to
    // save time. this makes this function less accurate at distance,
    // but that's good enough!
#ifdef EDITOR
    static constexpr int NumRays = 100;
    static constexpr int NumRaysPerJob = 50;
    static constexpr bool DoRaysInParallel = true;
    static constexpr bool WriteOutsSequentially = false;
#else
    static ConsoleVariable<int> cvar_numRays("/raycast_num", 100);
    static ConsoleVariable<int> cvar_numRaysPerJob("/raycast_num_per_job", 50);
    static ConsoleVariable<bool> cvar_parallelRays("/raycast_multithread", false); // note: crashes on nintendo
    static ConsoleVariable<bool> cvar_writeOutsSequentially("/raycast_write_outs_sequentially", false);
    
    static int NumRays;
    NumRays = *cvar_numRays;
    static int NumRaysPerJob;
    NumRaysPerJob = *cvar_numRaysPerJob;
    static bool DoRaysInParallel;
    DoRaysInParallel = *cvar_parallelRays;
    static bool WriteOutsSequentially;
    WriteOutsSequentially = *cvar_writeOutsSequentially;
    
    static bool TimeTest = false;
    static ConsoleCommand ccmd_raycastTime("/raycast_time", "Time the raycast() function", [](int argc, const char** argv){
        TimeTest = true;
    });
#endif
    
    // ray shooting functor
    struct outs_t {
        int x;
        int y;
        int value;
    };
    struct ins_t {
        const int sx;
        const int mw;
        const int mh;
        const view_t camera;
        const Sint32* tiles;
        const vec4_t* lights;
        Sint8 (*minimap)[MINIMAP_MAX_DIMENSION];
		bool fillWithColor;
    };
    auto shoot_ray = [](const ins_t&& ins) -> std::vector<outs_t>{
        std::vector<outs_t> result;
        
        const int& mw = ins.mw;
        const int& mh = ins.mh;
        const view_t& camera = ins.camera;
        const auto& tiles = ins.tiles;
        const auto& lights = ins.lights;
        const auto& minimap = ins.minimap;
        
        const int posx = (int)camera.x;
        const int posy = (int)camera.y; // integer coordinates
		if ( posx == 0 && posy == 0 ) { return result; } // camera not initialized
        const real_t fracx = camera.x - posx;
        const real_t fracy = camera.y - posy; // fraction coordinates

        const real_t wfov = (fov * camera.winw / camera.winh) * PI / 180.f;
        constexpr real_t dstart = CLIPNEAR / 16.0;
        constexpr real_t dend = CLIPFAR / 16;
        
        for (int sx = ins.sx; sx < ins.sx + NumRaysPerJob; ++sx) {
            int inx = posx;
            int iny = posy;
            int inx2 = inx;
            int iny2 = iny;
            
            // new ray vector for next column
            const real_t rx = cos(camera.ang - wfov / 2.f + (wfov / NumRays) * sx);
            const real_t ry = sin(camera.ang - wfov / 2.f + (wfov / NumRays) * sx);
            const real_t arx = rx ? 1.0 / fabs(rx) : 0.0;
            const real_t ary = ry ? 1.0 / fabs(ry) : 0.0;
            
            // dval0=dend+1 is there to prevent infinite loops when ray is parallel to axis
            long dincx = 0;
            long dincy = 0;
            real_t dval0 = 1e32;
            real_t dval1 = 1e32;
            
            // calculate integer coordinate increments
            // x-axis:
            if (rx < 0) {
                dincx = -1;
                dval0 = fracx * arx;
            } else if (rx > 0) {
                dincx = 1;
                dval0 = (1 - fracx) * arx;
            }
            
            // y-axis:
            if (ry < 0) {
                dincy = -1;
                dval1 = fracy * ary;
            } else if (ry > 0) {
                dincy = 1;
                dval1 = (1 - fracy) * ary;
            }
            
            real_t d = 0;
            do {
                // record previous ray position
                inx2 = inx;
                iny2 = iny;
                
                // move the ray one square forward
                if (dval1 > dval0) {
                    inx += dincx;
                    d = dval0;
                    dval0 += arx;
                } else {
                    iny += dincy;
                    d = dval1;
                    dval1 += ary;
                }
                
                // check ray is within map bounds
                if (inx < 0 || iny < 0 || inx >= mw || iny >= mh) {
                    // out of map bounds
                    break;
                }
                
                // check against tiles in each map layer
                bool zhit[MAPLAYERS] = { false };
                for (int z = 0; z < MAPLAYERS; z++) {
                    if (tiles[z + iny * MAPLAYERS + inx * MAPLAYERS * mh] && d > dstart) { // hit something solid
                        zhit[z] = true;
                        
                        // collect light information
                        if (tiles[z + iny2 * MAPLAYERS + inx2 * MAPLAYERS * mh]) {
                            continue;
                        }
                        auto& l = lights[iny2 + inx2 * mh];
                        const auto light = std::max({0.f, l.x, l.y, l.z});
						bool visible = light > 1.f;
						if ( !visible && !ins.fillWithColor )
						{
							// remote players fill in the map within 3x3 area, as they do not have ambient light
							real_t dist = pow(posx - inx2, 2) + pow(posy - iny2, 2);
							if ( dist < 10.0 )
							{
								visible = true;
							}
						}
                        
                        // update minimap
                        if (d < 16 && z == OBSTACLELAYER) {
                            if ( visible ) {
                                // wall space
                                if (WriteOutsSequentially) {
									if ( ins.fillWithColor )
									{
										result.push_back({inx, iny, 2});
									}
									else if ( minimap[iny][inx] != 2 )
									{
										result.push_back({inx, iny, 4});
									}
                                } else {
									if ( ins.fillWithColor )
									{
										minimap[iny][inx] = 2;
									}
									else if ( minimap[iny][inx] != 2 )
									{
										minimap[iny][inx] = 4;
									}
                                }
                            }
                        }
                    } else if (z == OBSTACLELAYER) {
                        // update minimap to show empty region
                        auto& l = lights[iny2 + inx2 * mh];
                        const auto light = std::max({0.f, l.x, l.y, l.z});
						bool visible = light > 1.f;
						if ( !visible && !ins.fillWithColor )
						{
							// remote players fill in the map within 3x3 area, as they do not have ambient light
							real_t dist = pow(posx - inx2, 2) + pow(posy - iny2, 2);
							if ( dist < 10.0 )
							{
								visible = true;
							}
						}

                        if (d < 16) {
                            if ( visible && tiles[iny * MAPLAYERS + inx * MAPLAYERS * mh]) {
                                // walkable space
                                if (WriteOutsSequentially) {
									if ( ins.fillWithColor )
									{
										result.push_back({inx, iny, 1});
									}
									else if ( minimap[iny][inx] != 1 )
									{
										result.push_back({inx, iny, 3});
									}
                                } else {
									if ( ins.fillWithColor )
									{
										minimap[iny][inx] = 1;
									}
									else if ( minimap[iny][inx] != 1 )
									{
										minimap[iny][inx] = 3;
									}
                                }
                            } else if (tiles[z + iny * MAPLAYERS + inx * MAPLAYERS * mh]) {
                                // no floor
                                /*if (WriteOutsSequentially) {
                                    result.push_back({inx, iny, 0});
                                } else {
                                    minimap[iny][inx] = 0;
                                }*/
                            }
                        }
                    }
                }
                
                // if a wall was hit (full column of map layers) stop the ray
                bool wallhit = true;
                for (int z = 0; z < MAPLAYERS; z++) {
                    if (zhit[z] == false) {
                        wallhit = false;
                    }
                }
                if (wallhit == true) {
                    break;
                }
            }
            while (d < dend);
        }
        return result;
    };
    
    auto t = std::chrono::high_resolution_clock::now();

    // shoot the rays
    const vec4_t* lightmap = lightmaps[0].data();
    for (int c = 0; c < MAXPLAYERS; ++c) {
        if (&camera == &cameras[c]) {
            lightmap = lightmaps[c + 1].data();
            break;
        }
    }
    if (DoRaysInParallel) {
        std::vector<std::future<std::vector<outs_t>>> tasks;
        for (int x = 0; x < NumRays; x += NumRaysPerJob) {
            tasks.emplace_back(std::async(std::launch::async, shoot_ray,
                ins_t{x, (int)map.width, (int)map.height, camera, map.tiles, lightmap, minimap, fillWithColor}));
        }
        for (int x = (int)tasks.size() - 1; x >= 0; --x) {
            auto out_list = tasks[x].get();
            for (auto& it : out_list) {
                minimap[it.y][it.x] = it.value;
            }
            tasks.pop_back();
        }
    } else {
        for (int x = 0; x < NumRays; x += NumRaysPerJob) {
            auto out_list = shoot_ray(ins_t{x, (int)map.width, (int)map.height, camera, map.tiles, lightmap, minimap, fillWithColor});
            for (auto& it : out_list) {
                minimap[it.y][it.x] = it.value;
            }
        }
    }
    
#ifndef EDITOR
    if (TimeTest) {
        TimeTest = false;
        auto duration = std::chrono::high_resolution_clock::now() - t;
        auto timer = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        messageLocalPlayers(MESSAGE_DEBUG, "Raycast took ~%llu microseconds", timer);
    }
#endif
}

/*-------------------------------------------------------------------------------

	drawEntities3D

	Draws all entities in the level as either voxel models or sprites

-------------------------------------------------------------------------------*/

Uint32 ditherDisabledTime = 0;
void temporarilyDisableDithering() {
    ditherDisabledTime = ticks;
}

void drawEntities3D(view_t* camera, int mode)
{
#ifndef EDITOR
    static ConsoleVariable<bool> cvar_drawEnts("/draw_entities", true);
	if (!*cvar_drawEnts) {
	    return;
	}
#endif

	if ( map.entities->first == nullptr )
	{
		return;
	}

	enum SpriteTypes
	{
		SPRITE_ENTITY,
		SPRITE_HPBAR,
		SPRITE_DIALOGUE
	};
	std::vector<std::tuple<real_t, void*, SpriteTypes>> spritesToDraw;

	int currentPlayerViewport = -1;
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		if ( &cameras[c] == camera )
		{
			currentPlayerViewport = c;
			break;
		}
	}
    
    const bool ditheringDisabled = ticks - ditherDisabledTime < TICKS_PER_SECOND;

	node_t* nextnode = nullptr;
	for ( node_t* node = map.entities->first; node != nullptr; node = nextnode )
    {
        Entity* entity = (Entity*)node->element;
        nextnode = node->next;
        if ( node->next == nullptr && node->list == map.entities )
        {
            if ( map.worldUI && map.worldUI->first )
            {
                // quick way to attach worldUI to the end of map.entities.
                nextnode = map.worldUI->first;
            }
        }
        
        if ( entity->flags[INVISIBLE] && !entity->flags[INVISIBLE_DITHER] )
        {
            continue;
        }
        if ( entity->flags[UNCLICKABLE] && mode == ENTITYUIDS )
        {
            continue;
        }
        if ( entity->flags[GENIUS] )
        {
			// genius entities are not drawn when the camera is inside their bounding box
#ifndef EDITOR
			if ( entity->behavior == &actDeathGhost )
			{
				// ghost have small collision box
				if ( camera->x >= (entity->x - std::max(4, entity->sizex)) / 16 && camera->x <= (entity->x + std::max(4, entity->sizex)) / 16 )
					if ( camera->y >= (entity->y - std::max(4, entity->sizey)) / 16 && camera->y <= (entity->y + std::max(4, entity->sizey)) / 16 )
					{
						continue;
					}
			}
			else
#endif
			{
				if ( camera->x >= (entity->x - entity->sizex) / 16 && camera->x <= (entity->x + entity->sizex) / 16 )
					if ( camera->y >= (entity->y - entity->sizey) / 16 && camera->y <= (entity->y + entity->sizey) / 16 )
					{
						continue;
					}
			}
        }
        if ( entity->flags[OVERDRAW] && splitscreen )
        {
            // need to skip some HUD models in splitscreen.
            if ( currentPlayerViewport >= 0 )
            {
                if ( entity->behavior == &actHudWeapon
                    || entity->behavior == &actHudArm
                    || entity->behavior == &actGib
                    || entity->behavior == &actFlame
					|| entity->behavior == &actHUDMagicParticle
					|| entity->behavior == &actHUDMagicParticleCircling )
                {
                    // the gibs are from casting magic in the HUD
                    if ( entity->skill[11] != currentPlayerViewport )
                    {
                        continue;
                    }
                }
                else if ( entity->behavior == &actHudAdditional
                         || entity->behavior == &actHudArrowModel
                         || entity->behavior == &actHudShield
                         || entity->behavior == &actLeftHandMagic
                         || entity->behavior == &actRightHandMagic )
                {
                    if ( entity->skill[2] != currentPlayerViewport )
                    {
                        continue;
                    }
                }
            }
        }
        
        // update dithering
        auto& dither = entity->dithering[camera];
        if (ticks != dither.lastUpdateTick) {
            dither.lastUpdateTick = ticks;
            bool decrease = false;
            if ( !entity->flags[OVERDRAW] )
            {
                const int x = entity->x / 16;
                const int y = entity->y / 16;
                if (x >= 0 && y >= 0 && x < map.width && y < map.height)
                {
                    if ( !camera->vismap[y + x * map.height] 
						&& entity->monsterEntityRenderAsTelepath != 1
						&& !(entity->behavior == &actSpriteNametag && entity->ditheringDisabled) )
                    {
                        decrease = true;
                        goto end;
                    }
                }
                const real_t rx = entity->x / 16.0;
                const real_t ry = entity->y / 16.0;
                if ( behindCamera(*camera, rx, ry) )
                {
                    decrease = true;
                    goto end;
                }
            }
            end:
            if (entity->ditheringDisabled) {
                dither.value = decrease ? 0 : Entity::Dither::MAX;
            } else {
                if (ditheringDisabled) {
                    dither.value = decrease ? 0 : Entity::Dither::MAX;
                } else {
					if ( entity->flags[INVISIBLE] && entity->flags[INVISIBLE_DITHER] )
					{
#ifndef EDITOR
						static ConsoleVariable<int> cvar_dither_invisibility("/dither_invisibility", 5);
						dither.value = decrease ? std::max(0, dither.value - 2) :
							std::min(*cvar_dither_invisibility, dither.value + 1);
#else
						dither.value = decrease ? std::max(0, dither.value - 2) :
							dither.value + 1;
#endif
					}
					else
					{
						dither.value = decrease ? std::max(0, dither.value - 2) :
						    std::min(Entity::Dither::MAX, dither.value + 2);
					}
                }
            }
        }
        if (dither.value == 0) {
            continue;
        }

		// don't draw hud weapons if we're being a telepath. they get in the way of world models
		if (entity->flags[OVERDRAW] && currentPlayerViewport >= 0) {
			if (stats[currentPlayerViewport]->EFFECTS[EFF_TELEPATH]) {
				continue;
			}
		}
        
		if ( entity->flags[SPRITE] == false )
		{
            GL_CHECK_ERR(glDrawVoxel(camera, entity, mode));
		}
		else
		{
			if ( entity->behavior == &actSpriteNametag )
			{
                real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
                    + pow(camera->y * 16.0 - entity->y, 2));
                spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
			}
			else if ( entity->behavior == &actSpriteWorldTooltip )
			{
				real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
					+ pow(camera->y * 16.0 - entity->y, 2));
				spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
			}
			else if ( entity->behavior == &actDamageGib )
			{
				if ( currentPlayerViewport != entity->skill[1] ) // skill[1] is player num, don't draw gibs on me
				{
					real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
						+ pow(camera->y * 16.0 - entity->y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
				}
			}
			else
			{
				if ( !entity->flags[OVERDRAW] )
				{
					real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
						+ pow(camera->y * 16.0 - entity->y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
				}
				else
				{
					real_t camDist = (pow(camera->x * 16.0 - entity->x, 2)
						+ pow(camera->y * 16.0 - entity->y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, entity, SPRITE_ENTITY));
				}
			}
		}
	}

#ifndef EDITOR
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		for ( auto& enemybar : enemyHPDamageBarHandler[i].HPBars )
		{
			real_t camDist = (pow(camera->x * 16.0 - enemybar.second.worldX, 2)
				+ pow(camera->y * 16.0 - enemybar.second.worldY, 2));
			spritesToDraw.push_back(std::make_tuple(camDist, &enemybar, SPRITE_HPBAR));
		}
		if ( players[i]->worldUI.worldTooltipDialogue.playerDialogue.init && players[i]->worldUI.worldTooltipDialogue.playerDialogue.draw )
		{
			if ( i == currentPlayerViewport )
			{
				real_t camDist = (pow(camera->x * 16.0 - players[i]->worldUI.worldTooltipDialogue.playerDialogue.x, 2)
					+ pow(camera->y * 16.0 - players[i]->worldUI.worldTooltipDialogue.playerDialogue.y, 2));
				spritesToDraw.push_back(std::make_tuple(camDist, &players[i]->worldUI.worldTooltipDialogue.playerDialogue, SPRITE_DIALOGUE));
			}
		}
		for ( auto it = players[i]->worldUI.worldTooltipDialogue.sharedDialogues.begin();
			it != players[i]->worldUI.worldTooltipDialogue.sharedDialogues.end(); ++it )
		{
			if ( it->second.init && it->second.draw )
			{
				if ( i == currentPlayerViewport )
				{
					real_t camDist = (pow(camera->x * 16.0 - it->second.x, 2)
						+ pow(camera->y * 16.0 - it->second.y, 2));
					spritesToDraw.push_back(std::make_tuple(camDist, &it->second, SPRITE_DIALOGUE));
				}
			}
		}
	}
#endif

	std::sort(spritesToDraw.begin(), spritesToDraw.end(), 
		[](const std::tuple<real_t, void*, SpriteTypes>& lhs, const std::tuple<real_t, void*, SpriteTypes>& rhs) {
		return lhs > rhs;
	});
	for ( auto& distSpriteType : spritesToDraw )
	{
		if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_ENTITY )
		{
			Entity* entity = (Entity*)std::get<1>(distSpriteType);
			if ( entity->behavior == &actSpriteNametag )
			{
				if ( intro ) { continue; } // don't draw on main menu
#ifndef EDITOR
                auto parent = uidToEntity(entity->parent);
                if (parent) {
                    if (multiplayer == CLIENT) {
                        auto stats = parent->behavior == &actPlayer ?
                            parent->getStats() : (parent->clientsHaveItsStats ? parent->clientStats : nullptr);
                        if (stats && stats->name[0]) {
							if ( parent->behavior == &actMonster && entity->skill[0] == clientnum && (!players[clientnum]->entity || parent->monsterAllyIndex != clientnum) )
							{
								// previous ally but we are dead (lost the ally) or someone stole our mon
							}
							else
							{
								if ( parent->getMonsterTypeFromSprite() == SLIME )
								{
									if ( !strcmp(stats->name, getMonsterLocalizedName(SLIME).c_str()) )
									{
										std::string name = stats->name;
										camelCaseString(name);
										glDrawSpriteFromImage(camera, entity, name.c_str(), mode);
									}
									else
									{
										glDrawSpriteFromImage(camera, entity, stats->name, mode);
									}
								}
								else
								{
									glDrawSpriteFromImage(camera, entity, stats->name, mode);
								}
							}
                        }
                    } else {
                        auto stats = parent->getStats();
                        if (stats && stats->name[0]) {
                            auto player = stats->leader_uid ?
                                playerEntityMatchesUid(stats->leader_uid):
                                playerEntityMatchesUid(entity->parent);
                            if (player >= 0 && (!stats->leader_uid || camera == &players[player]->camera())) {
								if ( stats->type == SLIME )
								{
									if ( !strcmp(stats->name, getMonsterLocalizedName(SLIME).c_str()) )
									{
										std::string name = stats->name;
										camelCaseString(name);
										glDrawSpriteFromImage(camera, entity, name.c_str(), mode);
									}
									else
									{
										glDrawSpriteFromImage(camera, entity, stats->name, mode);
									}
								}
								else
								{
									glDrawSpriteFromImage(camera, entity, stats->name, mode);
								}
                            }
                        }
                    }
                }
#else // EDITOR
                glDrawSpriteFromImage(camera, entity, entity->string ? entity->string : "", mode);
#endif
			}
			else if ( entity->behavior == &actSpriteWorldTooltip )
			{
				if ( intro ) { continue; } // don't draw on main menu
				glDrawWorldUISprite(camera, entity, mode);
			}
			else if ( entity->behavior == &actDamageGib )
			{
				if ( intro ) { continue; } // don't draw on main menu
				char buf[16];
				if ( entity->skill[0] < 0 )
				{
					snprintf(buf, sizeof(buf), "+%d", -entity->skill[0]);
					glDrawSpriteFromImage(camera, entity, buf, mode);
				}
				else
				{
					if ( entity->skill[7] == 1 )
					{
						glDrawSpriteFromImage(camera, entity, Language::get(6249), mode);
					}
					else if ( entity->skill[7] == 2 )
					{
						glDrawSprite(camera, entity, mode);
					}
					else
					{
						snprintf(buf, sizeof(buf), "%d", entity->skill[0]);
						glDrawSpriteFromImage(camera, entity, buf, mode);
					}
				}
			}
			else
			{
				glDrawSprite(camera, entity, mode);
			}
		}
		else if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_HPBAR )
		{
#ifndef EDITOR
			if ( intro ) { continue; } // don't draw on main menu
			auto enemybar = (std::pair<Uint32, EnemyHPDamageBarHandler::EnemyHPDetails>*)std::get<1>(distSpriteType);
			glDrawEnemyBarSprite(camera, mode, currentPlayerViewport, &enemybar->second);
#endif
		}
		else if ( std::get<2>(distSpriteType) == SpriteTypes::SPRITE_DIALOGUE )
		{
#ifndef EDITOR
			if ( intro ) { continue; } // don't draw on main menu
			auto dialogue = (Player::WorldUI_t::WorldTooltipDialogue_t::Dialogue_t*)std::get<1>(distSpriteType);
			glDrawWorldDialogueSprite(camera, dialogue, mode);
#endif
		}
	}
}

/*-------------------------------------------------------------------------------

	drawEntities2D

	Draws all entities in the level as sprites while accounting for the given
	camera coordinates

-------------------------------------------------------------------------------*/

void drawEntities2D(long camx, long camy)
{
	// editor only
#ifndef EDITOR
#else
	node_t* node;
	Entity* entity;
	SDL_Rect pos, box;
	int offsetx = 0;
	int offsety = 0;

	if ( map.entities->first == nullptr )
	{
		return;
	}

	// draw entities
	for ( node = map.entities->first; node != nullptr; node = node->next )
	{
		entity = (Entity*)node->element;
		if ( entity->flags[INVISIBLE] )
		{
			continue;
		}
		pos.x = entity->x * (TEXTURESIZE / 16) - camx;
		pos.y = entity->y * (TEXTURESIZE / 16) - camy;
		pos.w = TEXTURESIZE;
		pos.h = TEXTURESIZE;
		//ttfPrintText(ttf8, 100, 100, inputstr); debug any errant text input in editor

		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != nullptr )
			{
				if ( entity == selectedEntity[0] )
				{
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box, makeColorRGB(255, 0, 0), 255);
					box.w = TEXTURESIZE - 2;
					box.h = TEXTURESIZE - 2;
					box.x = pos.x + 1;
					box.y = pos.y + 1;
					drawRect(&box, makeColorRGB(0, 0, 255), 255);
				}
				
				// if item sprite and the item index is not 0 (NULL), or 1 (RANDOM)
				if ( entity->sprite == 8 && entity->skill[10] > 1 )
				{
					// draw the item sprite in the editor layout
					Item* tmpItem = newItem(static_cast<ItemType>(entity->skill[10] - 2), static_cast<Status>(0), 0, 0, 0, 0, nullptr);
					drawImageScaled(itemSprite(tmpItem), nullptr, &pos);
					free(tmpItem);
				}
				else if ( entity->sprite == 133 )
				{
					pos.y += sprites[entity->sprite]->h / 2;
					pos.x += sprites[entity->sprite]->w / 2;
					switch ( entity->signalInputDirection )
					{
						case 0:
							pos.x -= pos.w;
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 0.f, 255);
							break;
						case 1:
							pos.y += pos.h;
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 3 * PI / 2, 255);
							break;
						case 2:
							pos.x += pos.w;
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI, 255);
							break;
						case 3:
							pos.y -= pos.h;
							drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI / 2, 255);
							break;
					}
				}
				else if ( entity->sprite == 185 || entity->sprite == 186 || entity->sprite == 187 )
				{
					pos.y += sprites[entity->sprite]->h / 2;
					pos.x += sprites[entity->sprite]->w / 2;
					switch ( entity->signalInputDirection )
					{
					case 0:
						pos.x -= pos.w;
						drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 0.f, 255);
						break;
					case 1:
						pos.y -= pos.h;
						drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI / 2, 255);
						break;
					case 2:
						pos.x += pos.w;
						drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, PI, 255);
						break;
					case 3:
						pos.y += pos.h;
						drawImageRotatedAlpha(sprites[entity->sprite], nullptr, &pos, 3 * PI / 2, 255);
						break;
					}
				}
				else
				{
					// draw sprite normally from sprites list
					drawImageScaled(sprites[entity->sprite], nullptr, &pos);
				}
			}
			else
			{
				if ( entity == selectedEntity[0] )
				{
					// draws a box around the sprite
					box.w = TEXTURESIZE;
					box.h = TEXTURESIZE;
					box.x = pos.x;
					box.y = pos.y;
					drawRect(&box, makeColorRGB(255, 0, 0), 255);
					box.w = TEXTURESIZE - 2;
					box.h = TEXTURESIZE - 2;
					box.x = pos.x + 1;
					box.y = pos.y + 1;
					drawRect(&box, makeColorRGB(0, 0, 255), 255);
				}
				drawImageScaled(sprites[0], nullptr, &pos);
			}
		}
		else
		{
			if ( entity == selectedEntity[0] )
			{
				// draws a box around the sprite
				box.w = TEXTURESIZE;
				box.h = TEXTURESIZE;
				box.x = pos.x;
				box.y = pos.y;
				drawRect(&box, makeColorRGB(255, 0, 0), 255);
				box.w = TEXTURESIZE - 2;
				box.h = TEXTURESIZE - 2;
				box.x = pos.x + 1;
				box.y = pos.y + 1;
				drawRect(&box, makeColorRGB(0, 0, 255), 255);
			}
			drawImageScaled(sprites[0], nullptr, &pos);
		}
	}

	// draw hover text for entities over the top of sprites.
	for ( node = map.entities->first;
		  node != nullptr
			&& (openwindow == 0
			&& savewindow == 0)
		  ;
		  node = node->next
		)
	{
		entity = (Entity*)node->element;
		if ( entity->flags[INVISIBLE] )
		{
			continue;
		}
		pos.x = entity->x * (TEXTURESIZE / 16) - camx;
		pos.y = entity->y * (TEXTURESIZE / 16) - camy;
		pos.w = TEXTURESIZE;
		pos.h = TEXTURESIZE;
		//ttfPrintText(ttf8, 100, 100, inputstr); debug any errant text input in editor

		if ( entity->sprite >= 0 && entity->sprite < numsprites )
		{
			if ( sprites[entity->sprite] != nullptr )
			{
				if ( entity == selectedEntity[0] )
				{
					int spriteType = checkSpriteType(selectedEntity[0]->sprite);
					char tmpStr[1024] = "";
					char tmpStr2[1024] = "";
					int padx = pos.x + 10;
					int pady = pos.y - 40;
					Uint32 color = makeColorRGB(255, 255, 255);
					Uint32 colorWhite = makeColorRGB(255, 255, 255);
					switch ( spriteType )
					{
						case 1: //monsters
							pady += 10;
							if ( entity->getStats() != nullptr ) {
								strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
								ttfPrintText(ttf8, padx, pady - 10, tmpStr);
								snprintf(tmpStr, sizeof(entity->getStats()->name), "Name: %s", entity->getStats()->name);
								ttfPrintText(ttf8, padx, pady, tmpStr);
								snprintf(tmpStr, 10, "HP: %d", entity->getStats()->MAXHP);
								ttfPrintText(ttf8, padx, pady + 10, tmpStr);
								snprintf(tmpStr, 10, "Level: %d", entity->getStats()->LVL);
								ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							}


							break;
						case 2: //chest
							pady += 5;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady, tmpStr);
							switch ( (int)entity->yaw )
							{
								case 0:
									strcpy(tmpStr, "Facing: EAST");
									break;
								case 1:
									strcpy(tmpStr, "Facing: SOUTH");
									break;
								case 2:
									strcpy(tmpStr, "Facing: WEST");
									break;
								case 3:
									strcpy(tmpStr, "Facing: NORTH");
									break;
								default:
									strcpy(tmpStr, "Facing: Invalid");
									break;

							}
							ttfPrintText(ttf8, padx, pady + 10, tmpStr);

							switch ( entity->skill[9] )
							{
								case 0:
									strcpy(tmpStr, "Type: Random");
									break;
								case 1:
									strcpy(tmpStr, "Type: Garbage");
									break;
								case 2:
									strcpy(tmpStr, "Type: Food");
									break;
								case 3:
									strcpy(tmpStr, "Type: Jewelry");
									break;
								case 4:
									strcpy(tmpStr, "Type: Equipment");
									break;
								case 5:
									strcpy(tmpStr, "Type: Tools");
									break;
								case 6:
									strcpy(tmpStr, "Type: Magical");
									break;
								case 7:
									strcpy(tmpStr, "Type: Potions");
									break;
								case 8:
									strcpy(tmpStr, "Type: Empty");
									break;
								default:
									strcpy(tmpStr, "Type: Random");
									break;
							}
							ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							break;
						case 27: // collider
						{
							pady += 5;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady - 10, tmpStr);
							int model = selectedEntity[0]->colliderDecorationModel;
							if ( EditorEntityData_t::colliderData.find(selectedEntity[0]->colliderDamageTypes)
								!= EditorEntityData_t::colliderData.end() )
							{
								if ( EditorEntityData_t::colliderData[selectedEntity[0]->colliderDamageTypes].hasOverride("model") )
								{
									model = EditorEntityData_t::colliderData[selectedEntity[0]->colliderDamageTypes].getOverride("model");
								}
							}
							snprintf(tmpStr, sizeof(tmpStr), "Model: %s", modelFileNames[model].c_str());
							ttfPrintTextColor(ttf8, padx, pady, makeColorRGB(0, 255, 0), true, tmpStr);

							if ( EditorEntityData_t::colliderData.find(selectedEntity[0]->colliderDamageTypes)
								!= EditorEntityData_t::colliderData.end() )
							{

								auto& colliderData = EditorEntityData_t::colliderData[selectedEntity[0]->colliderDamageTypes];
								snprintf(tmpStr, sizeof(tmpStr), "Collider Type: %s", colliderData.name.c_str());
							}
							else
							{
								snprintf(tmpStr, sizeof(tmpStr), "Collider Type: ???");
							}
							ttfPrintTextColor(ttf8, padx, pady + 10, makeColorRGB(255, 255, 0), true, tmpStr);
							break;
						}
						case 3: //Items
							pady += 5;
							strcpy(tmpStr, itemNameStrings[selectedEntity[0]->skill[10]]);
							ttfPrintText(ttf8, padx, pady - 20, tmpStr);
							color = makeColorRGB(255, 255, 255);
							pady += 2;

							strcpy(tmpStr, "Status: ");
							ttfPrintTextColor(ttf8, padx, pady - 10, colorWhite, 1, tmpStr);
							switch ( (int)selectedEntity[0]->skill[11] )
							{
								case 1:
									strcpy(tmpStr, "Broken");
									color = makeColorRGB(255, 0, 0);
									break;
								case 2:
									strcpy(tmpStr, "Decrepit");
									color = makeColorRGB(200, 128, 0);
									break;
								case 3:
									strcpy(tmpStr, "Worn");
									color = makeColorRGB(255, 255, 0);
									break;
								case 4:
									strcpy(tmpStr, "Servicable");
									color = makeColorRGB(128, 200, 0);
									break;
								case 5:
									strcpy(tmpStr, "Excellent");
									color = makeColorRGB(0, 255, 0);
									break;
								default:
									strcpy(tmpStr, "?");
									color = makeColorRGB(0, 168, 255);
									break;
							}
							ttfPrintTextColor(ttf8, padx + 56, pady - 10, color, 1, tmpStr);

							strcpy(tmpStr, "Bless: ");
							ttfPrintTextColor(ttf8, padx, pady, colorWhite, 1, tmpStr);
							if ( selectedEntity[0]->skill[12] < 0 )
							{
								snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(255, 0, 0);
							}
							else if ( selectedEntity[0]->skill[12] == 0 )
							{
								snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(255, 255, 255);
							}
							else if ( selectedEntity[0]->skill[12] == 10 )
							{
								strcpy(tmpStr2, "?");
								color = makeColorRGB(0, 168, 255);
							}
							else
							{
								snprintf(tmpStr2, 10, "+%d", selectedEntity[0]->skill[12]);
								color = makeColorRGB(0, 255, 0);
							}
							ttfPrintTextColor(ttf8, padx + 48, pady, color, 1, tmpStr2);

							strcpy(tmpStr, "Qty: ");
							ttfPrintTextColor(ttf8, padx, pady + 10, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[13]);
							ttfPrintTextColor(ttf8, padx + 32, pady + 10, colorWhite, 1, tmpStr2);

							pady += 2;
							strcpy(tmpStr, "Identified: ");
							ttfPrintTextColor(ttf8, padx, pady + 20, colorWhite, 1, tmpStr);
							if ( (int)selectedEntity[0]->skill[15] == 0 )
							{
								strcpy(tmpStr2, "No");
								color = makeColorRGB(255, 255, 0);
							}
							else if ( (int)selectedEntity[0]->skill[15] == 1 )
							{
								strcpy(tmpStr2, "Yes");
								color = makeColorRGB(0, 255, 0);
							}
							else
							{
								strcpy(tmpStr2, "?");
								color = makeColorRGB(0, 168, 255);
							}
							ttfPrintTextColor(ttf8, padx + 80, pady + 20, color, 1, tmpStr2);
							break;
						case 4: //summoning trap
							pady += 5;
							offsety = -40;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);

							offsety += 10;
							strcpy(tmpStr, "Type: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							strcpy(tmpStr2, monsterEditorNameStrings[entity->skill[0]]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Qty: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[1]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Time: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[2]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Amount: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[3]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Power to: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							if ( selectedEntity[0]->skill[4] == 1 )
							{
								strcpy(tmpStr2, "Spawn");
							}
							else
							{
								strcpy(tmpStr2, "Disable");
							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Stop Chance: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->skill[5]);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);
							break;
						case 5: //power crystal
							pady += 5;
							offsety = -20;
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);

							offsety += 10;
							strcpy(tmpStr, "Facing: ");
							ttfPrintText(ttf8, padx, pady + offsety, tmpStr);
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							switch ( (int)entity->yaw )
							{
								case 0:
									strcpy(tmpStr2, "EAST");
									break;
								case 1:
									strcpy(tmpStr2, "SOUTH");
									break;
								case 2:
									strcpy(tmpStr2, "WEST");
									break;
								case 3:
									strcpy(tmpStr2, "NORTH");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Nodes: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							snprintf(tmpStr2, 10, "%d", selectedEntity[0]->crystalNumElectricityNodes);
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Rotation: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							switch ( (int)entity->crystalTurnReverse )
							{
								case 0:
									strcpy(tmpStr2, "Clockwise");
									break;
								case 1:
									strcpy(tmpStr2, "Anti-Clockwise");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);

							offsety += 10;
							strcpy(tmpStr, "Spell to Activate: ");
							offsetx = (int)strlen(tmpStr) * 8 - 8;
							ttfPrintTextColor(ttf8, padx, pady + offsety, colorWhite, 1, tmpStr);
							switch ( (int)entity->crystalSpellToActivate )
							{
								case 0:
									strcpy(tmpStr2, "No");
									break;
								case 1:
									strcpy(tmpStr2, "Yes");
									break;
								default:
									strcpy(tmpStr2, "Invalid");
									break;

							}
							ttfPrintText(ttf8, padx + offsetx, pady + offsety, tmpStr2);
							break;
						case 16:
						case 13:
						{
							char buf[256] = "";
							int totalChars = 0;
							for ( int i = (spriteType == 16 ? 4 : 8); i < 60; ++i )
							{
								if ( selectedEntity[0]->skill[i] != 0 && i != 28 ) // skill[28] is circuit status.
								{
									for ( int c = 0; c < 4; ++c )
									{
										if ( static_cast<char>((selectedEntity[0]->skill[i] >> (c * 8)) & 0xFF) == '\0'
											&& i != 59 && selectedEntity[0]->skill[i + 1] != 0 )
										{
											// don't add '\0' termination unless the next skill slot is empty as we have more data to read.
										}
										else
										{
											buf[totalChars] = static_cast<char>((selectedEntity[0]->skill[i] >> (c * 8)) & 0xFF);
											++totalChars;
										}
									}
								}
							}
							if ( buf[totalChars] != '\0' )
							{
								buf[totalChars] = '\0';
							}
							std::vector<std::string> lines;
							lines.push_back(spriteEditorNameStrings[selectedEntity[0]->sprite]);

							strncpy(tmpStr, buf, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 48, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 96, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 144, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							strncpy(tmpStr, buf + 192, 48);
							if ( strcmp(tmpStr, "") )
							{
								lines.push_back(tmpStr);
							}
							if ( lines.size() > 3 )
							{
								offsety -= (lines.size() - 2) * 5;
							}

							size_t longestLine = 0;
							for ( auto it : lines )
							{
								longestLine = std::max(longestLine, strlen(it.c_str()));
							}

							SDL_Rect tooltip;
							tooltip.x = padx + offsetx - 4;
							tooltip.w = TTF8_WIDTH * (int)longestLine + 8;
							tooltip.y = pady + offsety - 4;
							tooltip.h = (int)lines.size() * TTF8_HEIGHT + 8;

							if ( lines.size() <= 1 )
							{
								offsety += 20;
							}

							if ( spriteType == 13 )
							{
								if ( modelFileNames.find(selectedEntity[0]->floorDecorationModel) != modelFileNames.end() )
								{
									snprintf(tmpStr, sizeof(tmpStr), "Model: %s", modelFileNames[selectedEntity[0]->floorDecorationModel].c_str());
									if ( lines.size() > 1 )
									{
										ttfPrintTextColor(ttf8, padx + offsetx, tooltip.y - 16, makeColorRGB(0, 255, 0), true, tmpStr);
									}
									else
									{
										ttfPrintTextColor(ttf8, padx + offsetx, pady + offsety - 10, makeColorRGB(0, 255, 0), true, tmpStr);
									}
								}
								else
								{
									if ( lines.size() > 1 )
									{
										ttfPrintText(ttf8, padx + offsetx, tooltip.y - 16, "Model: Invalid Index");
									}
									else
									{
										ttfPrintText(ttf8, padx + offsetx, pady + offsety - 10, "Model: Invalid Index");
									}
								}
							}

							if ( lines.size() > 1 )
							{
								drawTooltip(&tooltip);
							}
							for ( auto it : lines )
							{
								ttfPrintText(ttf8, padx + offsetx, pady + offsety, it.c_str());
								offsety += 10;
							}
						}
							break;
						default:
							strcpy(tmpStr, spriteEditorNameStrings[selectedEntity[0]->sprite]);
							ttfPrintText(ttf8, padx, pady + 20, tmpStr);
							break;

					}
				}
				else if ( (omousex / TEXTURESIZE) * 32 == pos.x
						&& (omousey / TEXTURESIZE) * 32 == pos.y
						&& selectedEntity[0] == NULL
						&& hovertext
						)
				{
					// handle mouseover sprite name tooltip in main editor screen
					int padx = pos.x + 10;
					int pady = pos.y - 20;
					int spriteType = checkSpriteType(entity->sprite);
					//offsety = 0;
					Stat* tmpStats = nullptr;
					if ( spriteType == 1 )
					{
						tmpStats = entity->getStats();
						if ( tmpStats != nullptr )
						{
							if ( strcmp(tmpStats->name, "") != 0 )
							{
								ttfPrintText(ttf8, padx, pady - offsety, tmpStats->name);
								offsety += 10;
							}
							ttfPrintText(ttf8, padx, pady - offsety, spriteEditorNameStrings[entity->sprite]);
							offsety += 10;
						}
					}
					else if ( spriteType == 3 )
					{
						ttfPrintText(ttf8, padx, pady - offsety, itemNameStrings[entity->skill[10]]);
						offsety += 10;
					}
					else
					{
						ttfPrintText(ttf8, padx, pady - offsety, spriteEditorNameStrings[entity->sprite]);
						offsety += 10;
					}
				}
			}
		}
	}
#endif
}

/*-------------------------------------------------------------------------------

	drawGrid

	Draws a white line grid for the tile map

-------------------------------------------------------------------------------*/

void drawGrid(int camx, int camy)
{
	Uint32 color = makeColorRGB(127, 127, 127);
	drawLine(-camx, (map.height << TEXTUREPOWER) - camy, (map.width << TEXTUREPOWER) - camx, (map.height << TEXTUREPOWER) - camy, color, 255);
	drawLine((map.width << TEXTUREPOWER) - camx, -camy, (map.width << TEXTUREPOWER) - camx, (map.height << TEXTUREPOWER) - camy, color, 255);
	for ( int y = 0; y < map.height; y++ )
	{
		for ( int x = 0; x < map.width; x++ )
		{
			drawLine((x << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, ((x + 1) << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, color, 255);
			drawLine((x << TEXTUREPOWER) - camx, (y << TEXTUREPOWER) - camy, (x << TEXTUREPOWER) - camx, ((y + 1) << TEXTUREPOWER) - camy, color, 255);
		}
	}
}

/*-------------------------------------------------------------------------------

	drawEditormap

	Draws a minimap in the upper right corner of the screen to represent
	the screen's position relative to the rest of the level

-------------------------------------------------------------------------------*/

void drawEditormap(long camx, long camy)
{
	SDL_Rect src, osrc;

	src.x = xres - 120;
	src.y = 24;
	src.w = 112;
	src.h = 112;
	drawRect(&src, makeColorRGB(0, 0, 0), 255);

	// initial box dimensions
	src.x = (xres - 120) + (((real_t)camx / TEXTURESIZE) * 112.0) / map.width;
	src.y = 24 + (((real_t)camy / TEXTURESIZE) * 112.0) / map.height;
	src.w = (112.0 / map.width) * ((real_t)xres / TEXTURESIZE);
	src.h = (112.0 / map.height) * ((real_t)yres / TEXTURESIZE);

	// clip at left edge
	if ( src.x < xres - 120 )
	{
		src.w -= (xres - 120) - src.x;
		src.x = xres - 120;
	}

	// clip at right edge
	if ( src.x + src.w > xres - 8 )
	{
		src.w = xres - 8 - src.x;
	}

	// clip at top edge
	if ( src.y < 24 )
	{
		src.h -= 24 - src.y;
		src.y = 24;
	}

	// clip at bottom edge
	if ( src.y + src.h > 136 )
	{
		src.h = 136 - src.y;
	}

	osrc.x = src.x + 1;
	osrc.y = src.y + 1;
	osrc.w = src.w - 2;
	osrc.h = src.h - 2;
	drawRect(&src, makeColorRGB(255, 255, 255), 255);
	drawRect(&osrc, makeColorRGB(0, 0, 0), 255);
}

/*-------------------------------------------------------------------------------

	drawWindow / drawDepressed

	Draws a rectangular box that fills the area inside the given screen
	coordinates

-------------------------------------------------------------------------------*/

void drawWindow(int x1, int y1, int x2, int y2)
{
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2 - x1;
	src.h = y2 - y1;
	drawRect(&src, makeColorRGB(160, 160, 192), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 1;
	src.h = y2 - y1 - 1;
	drawRect(&src, makeColorRGB(96, 96, 128), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 2;
	src.h = y2 - y1 - 2;
	drawRect(&src, makeColorRGB(128, 128, 160), 255);
}

void drawDepressed(int x1, int y1, int x2, int y2)
{
	SDL_Rect src;

	src.x = x1;
	src.y = y1;
	src.w = x2 - x1;
	src.h = y2 - y1;
	drawRect(&src, makeColorRGB(96, 96, 128), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 1;
	src.h = y2 - y1 - 1;
	drawRect(&src, makeColorRGB(160, 160, 192), 255);
	src.x = x1 + 1;
	src.y = y1 + 1;
	src.w = x2 - x1 - 2;
	src.h = y2 - y1 - 2;
	drawRect(&src, makeColorRGB(128, 128, 160), 255);
}

void drawWindowFancy(int x1, int y1, int x2, int y2)
{
    auto white = Image::get("images/system/white.png");
    auto backdrop = Image::get("images/system/fancyWindow.png");
    
    white->drawColor(nullptr, SDL_Rect{x1, y1, x2 - x1, y2 - y1},
         SDL_Rect{0, 0, xres, yres}, makeColorRGB(63, 63, 63));
    
    white->drawColor(nullptr, SDL_Rect{x1 + 1, y1 + 1, x2 - x1 - 2, y2 - y1 - 2},
         SDL_Rect{0, 0, xres, yres}, makeColorRGB(127, 127, 127));
    
    backdrop->drawColor(nullptr, SDL_Rect{x1 + 2, y1 + 2, x2 - x1 - 4, y2 - y1 - 4},
         SDL_Rect{0, 0, xres, yres}, makeColorRGB(191, 191, 191));
}

/*-------------------------------------------------------------------------------

	ttfPrintText / ttfPrintTextColor

	Prints an unformatted utf8 string to the screen, returning the width of
	the message surface in pixels. ttfPrintTextColor() also takes a 32-bit
	color argument

-------------------------------------------------------------------------------*/

SDL_Rect ttfPrintTextColor( TTF_Font* font, int x, int y, Uint32 color, bool outline, const char* str )
{
    const char* filename = "lang/en.ttf#12#1"; // default
    if (outline) {
        if (font == ttf8) {
            filename = "lang/en.ttf#12#1";
            x -= 1;
            y -= 1;
        }
        else if (font == ttf12) {
            filename = "lang/en.ttf#16#2";
            x -= 2;
            y -= 4;
        }
        else if (font == ttf16) {
            filename = "lang/en.ttf#22#2";
            x -= 2;
            y -= 4;
        }
    } else {
        if (font == ttf8) {
            filename = "lang/en.ttf#12#0";
        }
        else if (font == ttf12) {
            filename = "lang/en.ttf#16#0";
        }
        else if (font == ttf16) {
            filename = "lang/en.ttf#22#0";
        }
    }
	int w = 0;
	int h = 0;
	char buf[1024] = { '\0' };
    char* ptr = buf;
    snprintf(buf, sizeof(buf), "%s", str);
    for (int c = 0; c < sizeof(buf) && ptr[c] != '\0'; ++c) {
        if (ptr[c] == '\n') {
            ptr[c] = '\0';
            auto text = Text::get(ptr, filename, uint32ColorWhite, uint32ColorBlack);
            text->drawColor(SDL_Rect{0, 0, 0, 0}, SDL_Rect{x, y, 0, 0}, SDL_Rect{0, 0, xres, yres}, color);
			w = std::max(w, (int)text->getWidth());
			h = std::max(h, (int)text->getHeight());
            y += text->getHeight();
            ptr += c + 1;
        }
    }
	if (ptr < buf + sizeof(buf)) {
		auto text = Text::get(ptr, filename, uint32ColorWhite, uint32ColorBlack);
		text->drawColor(SDL_Rect{ 0, 0, 0, 0 }, SDL_Rect{ x, y, 0, 0 }, SDL_Rect{ 0, 0, xres, yres }, color);
		w = std::max(w, (int)text->getWidth());
		h = std::max(h, (int)text->getHeight());
	}
    return SDL_Rect{x, y, w, h};
}

static SDL_Rect errorRect = { 0 };

SDL_Rect ttfPrintText( TTF_Font* font, int x, int y, const char* str )
{
	if ( !str )
	{
		return errorRect;
	}
	return ttfPrintTextColor(font, x, y, 0xFFFFFFFF, true, str);
}

/*-------------------------------------------------------------------------------

	ttfPrintTextFormatted / ttfPrintTextFormattedColor

	Prints a formatted utf8 string to the screen using
	ttfPrintText / ttfPrintTextColor

-------------------------------------------------------------------------------*/

SDL_Rect ttfPrintTextFormattedColor( TTF_Font* font, int x, int y, Uint32 color, char const * const fmt, ... )
{
	char str[1024] = { 0 };

	if ( !fmt )
	{
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font, x, y, color, true, str);
}

SDL_Rect ttfPrintTextFormatted( TTF_Font* font, int x, int y, char const * const fmt, ... )
{
	char str[1024] = { 0 };

	if ( !fmt )
	{
		return errorRect;
	}

	// format the string
	va_list argptr;
	va_start( argptr, fmt );
	vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// print the text
	return ttfPrintTextColor(font, x, y, 0xFFFFFFFF, true, str);
}

/*-------------------------------------------------------------------------------

	printText

	Prints unformatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void printText( SDL_Surface* font_bmp, int x, int y, const char* str )
{
	int c;
	int numbytes;
	SDL_Rect src, dest, odest;

	if ( strlen(str) > 2048 )
	{
		printlog("error: buffer overflow in printText\n");
		return;
	}

	// format the string
	numbytes = (int)strlen(str);

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormatted

	Prints formatted text to the screen using a font bitmap

-------------------------------------------------------------------------------*/

void debugPrintText(int x, int y, const SDL_Rect& viewport, char const * const fmt, ...)
{
    const auto font = font16x16_bmp;

	// format the string
	va_list argptr;
	va_start(argptr, fmt);
	char str[1024] = {'\0'};
	int size = vsnprintf(str, sizeof(str), fmt, argptr);
	va_end(argptr);

	// define font dimensions
    SDL_Rect src, dest;
	dest.x = x;
	dest.y = y;
	dest.w = font->w / 16;
	dest.h = font->h / 16;
	src.w = font->w / 16;
	src.h = font->h / 16;

	// print the characters in the string
	for (int c = 0; c < size; ++c) {
		src.x = (str[c] * src.w) % font->w;
		src.y = (int)((str[c] * src.w) / font->w) * src.h;
		if (str[c] != '\n' && str[c] != '\r') {
            SDL_Rect odest;
			odest.x = dest.x;
			odest.y = dest.y;
            Image::draw(texid[(long int)font->userdata], font->w, font->h,
                &src, dest, viewport, 0xffffffff);
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if (str[c] == '\n') {
			dest.x = x;
			dest.y += src.h;
		}
	}
}

void printTextFormatted( SDL_Surface* font_bmp, int x, int y, char const * const fmt, ... )
{
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest, odest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImage( font_bmp, &src, &dest );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedAlpha

	Prints formatted text to the screen using a font bitmap and taking an
	alpha value.

-------------------------------------------------------------------------------*/

void printTextFormattedAlpha(SDL_Surface* font_bmp, int x, int y, Uint8 alpha, char const * const fmt, ...)
{
    // deprecated
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap and taking a
	32-bit color value

-------------------------------------------------------------------------------*/

void printTextFormattedColor(SDL_Surface* font_bmp, int x, int y, Uint32 color, char const * const fmt, ...)
{
	int c;
	int numbytes;
	char str[1024] = { 0 };
	va_list argptr;
	SDL_Rect src, dest, odest;

	// format the string
	va_start( argptr, fmt );
	numbytes = vsnprintf( str, 1023, fmt, argptr );
	va_end( argptr );

	// define font dimensions
	dest.x = x;
	dest.y = y;
	dest.w = font_bmp->w / 16;
	src.w = font_bmp->w / 16;
	dest.h = font_bmp->h / 16;
	src.h = font_bmp->h / 16;

	// print the characters in the string
	for ( c = 0; c < numbytes; c++ )
	{
		src.x = (str[c] * src.w) % font_bmp->w;
		src.y = (int)((str[c] * src.w) / font_bmp->w) * src.h;
		if ( str[c] != 10 && str[c] != 13 )   // LF/CR
		{
			odest.x = dest.x;
			odest.y = dest.y;
			drawImageColor( font_bmp, &src, &dest, color );
			dest.x = odest.x + src.w;
			dest.y = odest.y;
		}
		else if ( str[c] == 10 )
		{
			dest.x = x;
			dest.y += src.h;
		}
	}
}

/*-------------------------------------------------------------------------------

	printTextFormattedColor

	Prints formatted text to the screen using a font bitmap, while coloring,
	rotating, and scaling it

-------------------------------------------------------------------------------*/

void printTextFormattedFancy(SDL_Surface* font_bmp, int x, int y, Uint32 color, real_t angle, real_t scale, char* fmt, ...)
{
	// deprecated
}

/*-------------------------------------------------------------------------------

	draws a tooltip

	Draws a tooltip box

-------------------------------------------------------------------------------*/

void drawTooltip(SDL_Rect* src, Uint32 optionalColor)
{
	Uint32 color = makeColorRGB(0, 192, 255);
	if ( optionalColor == 0 )
	{
		drawRect(src, 0, 250);
	}
	else
	{
		color = optionalColor;
	}
	drawLine(src->x, src->y, src->x + src->w, src->y, color, 255);
	drawLine(src->x, src->y + src->h, src->x + src->w, src->y + src->h, color, 255);
	drawLine(src->x, src->y, src->x, src->y + src->h, color, 255);
	drawLine(src->x + src->w, src->y, src->x + src->w, src->y + src->h, color, 255);
}

void getColor(Uint32 color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
	r ? (*r = (color & 0x000000ff) >> 0) : 0;
	g ? (*g = (color & 0x0000ff00) >> 8) : 0;
	b ? (*b = (color & 0x00ff0000) >> 16) : 0;
	a ? (*a = (color & 0xff000000) >> 24) : 0;
}

bool behindCamera(const view_t& camera, real_t x, real_t y)
{
    const float dx = x - camera.x;
    const float dy = y - camera.y;
    const float len2 = dx*dx + dy*dy;
    if (len2 < 4) {
        return false;
    }
    const float alen = 1.0f / sqrtf(len2); // normalize direction to x/y

	// camera direction vector
	const float a = camera.ang;
    const float v0x = cosf(a);
    const float v0y = sinf(a);

	// direction vector to x/y
    const float v1x = dx * alen;
    const float v1y = dy * alen;

	// dot product of camera direction and normalized direction vector to x/y
    const float dot = v0x * v1x + v0y * v1y;

	// cosine of FOV lets us see if the object is outside the view frustum.
	// however, there is an inaccuracy: the above algorithm only operates in
	// 2D, so when the camera tilts up or down, broadening the WFOV, this is
	// unaccounted for. therefore a margin of error of 30* is added to fov
	const uint32_t error = 30;
    const float aspect = (float)camera.winw / (float)camera.winh;
    const float wfov = std::max(90.f, (float)(fov + error) * aspect) * ((float)PI) / 180.f;
    const float c = cosf(wfov * 0.5f);

    return dot < c;
}

static inline bool testTileOccludes(const map_t& map, int index) {
    assert(index >= 0 && index <= map.width * map.height * MAPLAYERS - MAPLAYERS);
    const Uint64& t0 = *(Uint64*)&map.tiles[index];
    const Uint32& t1 = *(Uint32*)&map.tiles[index + 2];
    return (t0 & 0xffffffff00000000) && (t0 & 0x00000000ffffffff) && t1;
}

void occlusionCulling(map_t& map, view_t& camera)
{
	// cvars
#ifndef EDITOR
    static ConsoleVariable<bool> disabled("/skipculling", false);
	static ConsoleVariable<bool> disableInWalls("/disable_culling_in_walls", false);
#else
	static bool ed_disabled = false;
	static bool ed_disableInWalls = true;
    auto* disabled = &ed_disabled;
	auto* disableInWalls = &ed_disableInWalls;
#endif

	const int size = map.width * map.height;
	
    if (*disabled) {
        memset(camera.vismap, 1, sizeof(bool) * size);
        return;
    }

    const int camx = std::min(std::max(0, (int)camera.x), (int)map.width - 1);
    const int camy = std::min(std::max(0, (int)camera.y), (int)map.height - 1);

    // don't do culling if camera in wall
	if (*disableInWalls) {
		if (map.tiles[OBSTACLELAYER + camy * MAPLAYERS + camx * MAPLAYERS * map.height] != 0) {
			memset(camera.vismap, 1, sizeof(bool) * size);
			return;
		}
	}

    // clear vismap
    memset(camera.vismap, 0, sizeof(bool) * size);
	camera.vismap[camy + camx * map.height] = true;

    const int hoff = MAPLAYERS;
    const int woff = MAPLAYERS * map.height;
    
	// making these static saves a lot of redundant
	// putting up / pulling down of structures in
	// memory, which saves measurable time on more
	// constrained platforms like nintendo.
	// plus this function isn't threaded so who cares.
    static std::vector<std::pair<int, int>> open;
    static std::set<std::pair<int, int>> closed;
	open.clear();
	open.reserve(size);
	closed.clear();
    open.emplace_back(camx, camy);
    closed.emplace(camx, camy);

    // do line tests throughout the map
    while (!open.empty()) {
        const int u = open.back().first;
        const int v = open.back().second;
        const int uvindex = v * hoff + u * woff;
        open.pop_back();
        if (camera.vismap[v + u * map.height]) {
            goto next;
        }
        if (behindCamera(camera, (real_t)u + 0.5, (real_t)v + 0.5)) {
            goto next;
        }
        for (int foo = -1; foo <= 1; ++foo) {
            for (int bar = -1; bar <= 1; ++bar) {
                const int x = std::min(std::max(0, camx + foo), (int)map.width - 1);
                const int y = std::min(std::max(0, camy + bar), (int)map.height - 1);
                const int xyindex = y * hoff + x * woff;
                if (testTileOccludes(map, xyindex)) {
                    continue;
                }
                const int dx = u - x;
                const int dy = v - y;
                const int sdx = sgn(dx);
                const int sdy = sgn(dy);
                const int dxabs = abs(dx);
                const int dyabs = abs(dy);
                bool wallhit = false;
                if (dxabs >= dyabs) { // the line is more horizontal than vertical
                    int a = dxabs >> 1;
                    int index = uvindex;
                    for (int i = 1; i < dxabs; ++i) {
                        index -= woff * sdx;
                        a += dyabs;
                        if (a >= dxabs) {
                            a -= dxabs;
                            index -= hoff * sdy;
                        }
                        if (testTileOccludes(map, index)) {
                            wallhit = true;
                            break;
                        }
                    }
                } else { // the line is more vertical than horizontal
                    int a = dyabs >> 1;
                    int index = uvindex;
                    for (int i = 1; i < dyabs; ++i) {
                        index -= hoff * sdy;
                        a += dxabs;
                        if (a >= dyabs) {
                            a -= dyabs;
                            index -= woff * sdx;
                        }
                        if (testTileOccludes(map, index)) {
                            wallhit = true;
                            break;
                        }
                    }
                }
                if (!wallhit) {
                    camera.vismap[v + u * map.height] = true;
                    goto next;
                }
            }
        }
        
    next:
        // if the vis check succeeded, explore adjacent tiles
        if (camera.vismap[v + u * map.height]) {
            if (u < map.width - 1) { // check tiles to the east
                if (closed.emplace(u + 1, v).second) {
                    if (!testTileOccludes(map, uvindex + woff)) {
                        open.emplace_back(u + 1, v);
                    }
                }
            }
            if (v < map.height - 1) { // check tiles to the south
                if (closed.emplace(u, v + 1).second) {
                    if (!testTileOccludes(map, uvindex + hoff)) {
                        open.emplace_back(u, v + 1);
                    }
                }
            }
            if (u > 0) { // check tiles to the west
                if (closed.emplace(u - 1, v).second) {
                    if (!testTileOccludes(map, uvindex - woff)) {
                        open.emplace_back(u - 1, v);
                    }
                }
            }
            if (v > 0) { // check tiles to the north
                if (closed.emplace(u, v - 1).second) {
                    if (!testTileOccludes(map, uvindex - hoff)) {
                        open.emplace_back(u, v - 1);
                    }
                }
            }
        }
    }

	// expand vismap one tile in each direction
	constexpr int VMAP_MAX_DIMENSION = 128;
	static bool vmap[VMAP_MAX_DIMENSION * VMAP_MAX_DIMENSION];
	assert(size <= sizeof(vmap) / sizeof(vmap[0]));
	const int w = map.width;
	const int w1 = map.width - 1;
	const int h = map.height;
	const int h1 = map.height - 1;
    for (int u = 0; u < w; u++) {
        for (int v = 0; v < h; v++) {
            const int index = v + u * h;
	        vmap[index] = camera.vismap[index];
		    if (!vmap[index]) {
		        if (v >= 1) {
		            if (camera.vismap[index - 1]) {
		                vmap[index] = true;
		                continue;
		            }
                    if (u >= 1 && camera.vismap[index - h - 1]) {
                        vmap[index] = true;
                        continue;
                    }
                    if (u < w1 && camera.vismap[index + h - 1]) {
                        vmap[index] = true;
                        continue;
                    }
		        }
		        if (v < h1) {
		            if (camera.vismap[index + 1]) {
		                vmap[index] = true;
		                continue;
		            }
                    if (u >= 1 && camera.vismap[index - h + 1]) {
                        vmap[index] = true;
                        continue;
                    }
                    if (u < w1 && camera.vismap[index + h + 1]) {
                        vmap[index] = true;
                        continue;
                    }
		        }
		        if (u >= 1 && camera.vismap[index - h]) {
		            vmap[index] = true;
		            continue;
		        }
		        if (u < w1 && camera.vismap[index + h]) {
		            vmap[index] = true;
		            continue;
		        }
		    }
		}
	}
	memcpy(camera.vismap, vmap, size);
}

float foverflow() {
	float f = 1e10;
	for (int i = 0; i < 10; ++i) {
		f = f * f; // this will overflow before the for loop terminates
	}
	return f;
}

float toFloat32(GLhalf value) {
	int s = (value >> 15) & 0x00000001;
	int e = (value >> 10) & 0x0000001f;
	int m = value & 0x000003ff;

	if (e == 0) {
		if (m == 0) {
			// Plus or minus zero
			uif32 result;
			result.i = static_cast<unsigned int>(s << 31);
			return result.f;
		}
		else {
			// Denormalized number -- renormalize it
			while (!(m & 0x00000400)) {
				m <<= 1;
				e -= 1;
			}
			e += 1;
			m &= ~0x00000400;
		}
	}
	else if (e == 31) {
		if (m == 0) {
			// Positive or negative infinity
			uif32 result;
			result.i = static_cast<unsigned int>((s << 31) | 0x7f800000);
			return result.f;
		}
		else {
			// Nan -- preserve sign and significand bits
			uif32 result;
			result.i = static_cast<unsigned int>((s << 31) | 0x7f800000 | (m << 13));
			return result.f;
		}
	}

	// Normalized number
	e = e + (127 - 15);
	m = m << 13;

	// Assemble s, e and m.
	uif32 result;
	result.i = static_cast<unsigned int>((s << 31) | (e << 23) | m);
	return result.f;
}

GLhalf toFloat16(float f) {
	uif32 entry;
	entry.f = f;
	int i = static_cast<int>(entry.i);

	// Our floating point number, f, is represented by the bit
	// pattern in integer i.  Disassemble that bit pattern into
	// the sign, s, the exponent, e, and the significand, m.
	// Shift s into the position where it will go in the
	// resulting half number.
	// Adjust e, accounting for the different exponent bias
	// of float and half (127 versus 15).
	int s = (i >> 16) & 0x00008000;
	int e = ((i >> 23) & 0x000000ff) - (127 - 15);
	int m = i & 0x007fffff;

	// Now reassemble s, e and m into a half:
	if (e <= 0) {
		if (e < -10) {
			// E is less than -10.  The absolute value of f is
			// less than half_MIN (f may be a small normalized
			// float, a denormalized float or a zero).
			// We convert f to a half zero.
			return GLhalf(s);
		}

		// E is between -10 and 0.  F is a normalized float,
		// whose magnitude is less than __half_NRM_MIN.
		// We convert f to a denormalized half.
		m = (m | 0x00800000) >> (1 - e);

		// Round to nearest, round "0.5" up.
		// Rounding may cause the significand to overflow and make
		// our number normalized.  Because of the way a half's bits
		// are laid out, we don't have to treat this case separately;
		// the code below will handle it correctly.
		if (m & 0x00001000) {
			m += 0x00002000;
		}

		// Assemble the half from s, e (zero) and m.
		return GLhalf(s | (m >> 13));
	}
	else if (e == 0xff - (127 - 15)) {
		if (m == 0) {
			// F is an infinity; convert f to a half
			// infinity with the same sign as f.
			return GLhalf(s | 0x7c00);
		}
		else {
			// F is a NAN; we produce a half NAN that preserves
			// the sign bit and the 10 leftmost bits of the
			// significand of f, with one exception: If the 10
			// leftmost bits are all zero, the NAN would turn
			// into an infinity, so we have to set at least one
			// bit in the significand.
			m >>= 13;
			return GLhalf(s | 0x7c00 | m | (m == 0));
		}
	}
	else {
		// E is greater than zero.  F is a normalized float.
		// We try to convert f to a normalized half.
		// Round to nearest, round "0.5" up
		if (m & 0x00001000) {
			m += 0x00002000;
			if (m & 0x00800000) {
				m = 0;      // overflow in significand,
				e += 1;     // adjust exponent
			}
		}

		// Handle exponent overflow
		if (e > 30) {
			foverflow();        // Cause a hardware floating point overflow;

			// if this returns, the half becomes an
			// infinity with the same sign as f.
			return GLhalf(s | 0x7c00);
		}

		// Assemble the half from s, e and m.
		return GLhalf(s | (e << 10) | (m >> 13));
	}
}
