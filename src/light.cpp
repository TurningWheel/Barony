/*-------------------------------------------------------------------------------

	BARONY
	File: light.cpp
	Desc: light spawning code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"

/*-------------------------------------------------------------------------------

	lightSphereShadow

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and intensity; casts shadows against walls

	intensity can be from -255 to 255

-------------------------------------------------------------------------------*/

light_t *lightSphereShadow(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity) {
	light_t *light;
	Sint32 i;
	Sint32 u, v, u2, v2;
	double a, b;
	Sint32 dx, dy;
	Sint32 dxabs, dyabs;
	bool wallhit;
	int index, z;

	if( intensity==0 )
		return NULL;
	light = newLight(x,y,radius,intensity);
	intensity = std::min(std::max(-255,intensity),255);

	for( v=y-radius; v<=y+radius; v++ ) {
		for( u=x-radius; u<=x+radius; u++ ) {
			if( u>=0 && v>=0 && u<map.width && v<map.height ) {
				dx = u-x;
				dy = v-y;
				dxabs=abs(dx);
				dyabs=abs(dy);
				a=dyabs*.5;
				b=dxabs*.5;
				u2=u;
				v2=v;
				wallhit=TRUE;
				index = v*MAPLAYERS+u*MAPLAYERS*map.height;
				for( z=0; z<MAPLAYERS; z++ ) {
					if( !map.tiles[index+z] ) {
						wallhit=FALSE;
						break;
					}
				}
				if( wallhit==TRUE )
					continue;
				if( dxabs >= dyabs ) { // the line is more horizontal than vertical
					for( i=0; i<dxabs; i++ ) {
						u2 -= sgn(dx);
						b += dyabs;
						if( b >= dxabs ) {
							b -= dxabs;
							v2 -= sgn(dy);
						}
						if( u2>=0 && u2<map.width && v2>=0 && v2<map.height ) {
							if( map.tiles[OBSTACLELAYER+v2*MAPLAYERS+u2*MAPLAYERS*map.height] ) {
								wallhit=TRUE;
								break;
							}
						}
					}
				} else { // the line is more vertical than horizontal
					for( i=0; i<dyabs; i++ ) {
						v2 -= sgn(dy);
						a += dxabs;
						if( a >= dyabs ) {
							a -= dyabs;
							u2 -= sgn(dx);
						}
						if( u2>=0 && u2<map.width && v2>=0 && v2<map.height ) {
							if( map.tiles[OBSTACLELAYER+v2*MAPLAYERS+u2*MAPLAYERS*map.height] ) {
								wallhit=TRUE;
								break;
							}
						}
					}
				}
				if( wallhit==FALSE || (wallhit==TRUE && u2==u && v2==v) ) {
					light->tiles[(dy+radius)+(dx+radius)*(radius*2+1)] = intensity-intensity*std::min(sqrt(dx*dx+dy*dy)/radius,1.0);
					lightmap[v+u*map.height] += light->tiles[(dy+radius)+(dx+radius)*(radius*2+1)];
				}
			}
		}
	}
	return light;
}

/*-------------------------------------------------------------------------------

	lightSphere

	Adds a circle of light to the lightmap at x and y with the supplied
	radius and intensity; casts no shadows

	intensity can be from -255 to 255

-------------------------------------------------------------------------------*/

light_t *lightSphere(Sint32 x, Sint32 y, Sint32 radius, Sint32 intensity) {
	light_t *light;
	Sint32 u, v;
	Sint32 dx, dy;

	if( intensity==0 )
		return NULL;
	light = newLight(x,y,radius,intensity);
	intensity = std::min(std::max(-255,intensity),255);

	for( v=y-radius; v<=y+radius; v++ ) {
		for( u=x-radius; u<=x+radius; u++ ) {
			if( u>=0 && v>=0 && u<map.width && v<map.height ) {
				dx = u-x;
				dy = v-y;
				light->tiles[(dy+radius)+(dx+radius)*(radius*2+1)] = intensity-intensity*std::min(sqrt(dx*dx+dy*dy)/radius,1.0);
				lightmap[v+u*map.height] += light->tiles[(dy+radius)+(dx+radius)*(radius*2+1)];
			}
		}
	}
	return light;
}