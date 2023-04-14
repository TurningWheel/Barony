/*-------------------------------------------------------------------------------

	BARONY
	File: light.hpp
	Desc: prototypes for light.cpp, light-related types and prototypes

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once

typedef struct light_t
{
	Sint32 x, y;
	Sint32 radius;
	Uint32 color;
	vec4_t* tiles;

	// a pointer to the light's location in a list
	node_t* node;
} light_t;

light_t* lightSphereShadow(Sint32 x, Sint32 y, Sint32 radius, Uint32 color);
light_t* lightSphere(Sint32 x, Sint32 y, Sint32 radius, Uint32 color);
light_t* newLight(Sint32 x, Sint32 y, Sint32 radius, Uint32 color);
