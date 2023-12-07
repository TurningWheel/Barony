/*-------------------------------------------------------------------------------

	BARONY
	File: light.cpp
	Desc: light spawning code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "light.hpp"
#include "draw.hpp"

/*-------------------------------------------------------------------------------

	lightSphereShadow

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and color; casts shadows against walls

-------------------------------------------------------------------------------*/

light_t* lightSphereShadow(int index, Sint32 x, Sint32 y, Sint32 radius, float r, float g, float b, float exp)
{
	light_t* light = newLight(index, x, y, radius);
    r = r * 255.f;
    g = g * 255.f;
    b = b * 255.f;

	for (int v = y - radius; v <= y + radius; ++v) {
		for (int u = x - radius; u <= x + radius; ++u) {
			if (u >= 0 && v >= 0 && u < map.width && v < map.height) {
				const int dx = u - x;
				const int dy = v - y;
				const int dxabs = abs(dx);
				const int dyabs = abs(dy);
				real_t a0 = dyabs * .5;
				real_t b0 = dxabs * .5;
				int u2 = u;
				int v2 = v;
                
                // check origin is okay
				bool wallhit = true;
				const int mapindex = v * MAPLAYERS + u * MAPLAYERS * map.height;
				for (int z = 0; z < MAPLAYERS; z++) {
					if (!map.tiles[mapindex + z]) {
						wallhit = false;
						break;
					}
				}
				if (wallhit == true) {
					continue;
				}
                
                // line test
                if (dxabs >= dyabs) { // the line is more horizontal than vertical
					for (int i = 0; i < dxabs; ++i) {
						u2 -= sgn(dx);
						b0 += dyabs;
						if (b0 >= dxabs) {
							b0 -= dxabs;
							v2 -= sgn(dy);
						}
						if (u2 >= 0 && u2 < map.width && v2 >= 0 && v2 < map.height) {
							if (map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height]) {
								wallhit = true;
								break;
							}
						}
					}
				}
                else { // the line is more vertical than horizontal
					for (int i = 0; i < dyabs; ++i) {
						v2 -= sgn(dy);
						a0 += dxabs;
						if (a0 >= dyabs) {
							a0 -= dyabs;
							u2 -= sgn(dx);
						}
						if (u2 >= 0 && u2 < map.width && v2 >= 0 && v2 < map.height) {
							if (map.tiles[OBSTACLELAYER + v2 * MAPLAYERS + u2 * MAPLAYERS * map.height]) {
								wallhit = true;
								break;
							}
						}
					}
				}
                
                // light tile if it passed line test
				if (wallhit == false || (wallhit == true && u2 == u && v2 == v)) {
                    const float dist = exp != 1.f ? powf(dx * dx + dy * dy, exp) : dx * dx + dy * dy;
                    const auto falloff = std::min<float>(dist / radius, 1.0f);
                    constexpr float a = 255.f;
                    const auto soff = (dy + radius) + (dx + radius) * (radius * 2 + 1);
                    auto& s = light->tiles[soff];
					s.x += r - r * falloff;
                    s.y += g - g * falloff;
                    s.z += b - b * falloff;
                    s.w += a - a * falloff;
                    const auto doff = v + u * map.height;
                    if (index) {
                        auto& d = lightmaps[index][doff];
                        d.x += s.x;
                        d.y += s.y;
                        d.z += s.z;
                        d.w += s.w;
                    } else {
                        for (int c = 0; c < MAXPLAYERS + 1; ++c) {
                            auto& d = lightmaps[c][doff];
                            d.x += s.x;
                            d.y += s.y;
                            d.z += s.z;
                            d.w += s.w;
                        }
                    }
                }
			}
		}
	}
	return light;
}

/*-------------------------------------------------------------------------------

	lightSphere

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and color; casts no shadows

-------------------------------------------------------------------------------*/

light_t* lightSphere(int index, Sint32 x, Sint32 y, Sint32 radius, float r, float g, float b, float exp)
{
	light_t* light = newLight(index, x, y, radius);
    r = r * 255.f;
    g = g * 255.f;
    b = b * 255.f;

	for (int v = y - radius; v <= y + radius; ++v) {
		for (int u = x - radius; u <= x + radius; ++u) {
			if (u >= 0 && v >= 0 && u < map.width && v < map.height) {
				const int dx = u - x;
				const int dy = v - y;
                const float dist = exp != 1.f ? powf(dx * dx + dy * dy, exp) : dx * dx + dy * dy;
                const auto falloff = std::min<float>(dist / radius, 1.0f);
                constexpr float a = 255.f;
                const auto soff = (dy + radius) + (dx + radius) * (radius * 2 + 1);
                auto& s = light->tiles[soff];
                s.x += r - r * falloff;
                s.y += g - g * falloff;
                s.z += b - b * falloff;
                s.w += a - a * falloff;
                const auto doff = v + u * map.height;
                if (index) {
                    auto& d = lightmaps[index][doff];
                    d.x += s.x;
                    d.y += s.y;
                    d.z += s.z;
                    d.w += s.w;
                } else {
                    for (int c = 0; c < MAXPLAYERS + 1; ++c) {
                        auto& d = lightmaps[c][doff];
                        d.x += s.x;
                        d.y += s.y;
                        d.z += s.z;
                        d.w += s.w;
                    }
                }
			}
		}
	}
	return light;
}

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "files.hpp"

std::unordered_map<std::string, LightDef> lightDefs;
bool loadLights(bool forceLoadBaseDirectory) {
    if ( !PHYSFS_getRealDir("/data/lights.json") )
    {
        printlog("[JSON]: Error: Could not find file: data/lights.json");
        return false;
    }

    std::string inputPath = PHYSFS_getRealDir("/data/lights.json");
    if ( forceLoadBaseDirectory )
    {
        inputPath = BASE_DATA_DIR;
    }
    else
    {
        if ( inputPath != BASE_DATA_DIR )
        {
            loadLights(true); // force load the base directory first, then modded paths later.
        }
        else
        {
            forceLoadBaseDirectory = true;
        }
    }

    inputPath.append("/data/lights.json");

    File* fp = FileIO::open(inputPath.c_str(), "rb");
    if ( !fp )
    {
        printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
        return false;
    }

    if ( forceLoadBaseDirectory )
    {
        lightDefs.clear();
    }
    
    char buf[65536];
    int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf));
    buf[count] = '\0';
    rapidjson::StringStream is(buf);
    FileIO::close(fp);

    rapidjson::Document d;
    d.ParseStream(is);
    
    const auto& lights = d["lights"];
    if (lights.IsObject()) {
        for (const auto& it : lights.GetObject()) {
            LightDef def;
            const auto& name = it.name.GetString();
            const auto& radius = it.value["radius"]; def.radius = radius.GetInt();
            const auto& r = it.value["r"]; def.r = r.GetFloat();
            const auto& g = it.value["g"]; def.g = g.GetFloat();
            const auto& b = it.value["b"]; def.b = b.GetFloat();
            const auto& exp = it.value["falloff_exp"]; def.falloff_exp = exp.GetFloat();
            const auto& shadows = it.value["shadows"]; def.shadows = shadows.GetBool();
            lightDefs[name] = def;
        }
    }
    
    return true;
}

#ifndef EDITOR
#include "interface/consolecommand.hpp"
static ConsoleCommand ccmd_reloadLights("/reloadlights", "reload light json",
    [](int argc, const char* argv[]){
    loadLights();
    });
#endif

light_t* addLight(Sint32 x, Sint32 y, const char* name, int range_bonus, int index) {
    if (!name || !name[0]) {
        return nullptr;
    }
    auto find = lightDefs.find(name);
    if (find == lightDefs.end()) {
        return nullptr;
    }
    const auto& def = find->second;
    if (def.shadows) {
        return lightSphereShadow(index, x, y, std::max(def.radius + range_bonus, 1), def.r, def.g, def.b, def.falloff_exp);
    } else {
        return lightSphere(index, x, y, std::max(def.radius + range_bonus, 1), def.r, def.g, def.b, def.falloff_exp);
    }
}
