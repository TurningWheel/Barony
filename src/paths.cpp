/*-------------------------------------------------------------------------------

	BARONY
	File: paths.cpp
	Desc: contains functions to generate paths through a level

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "monster.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "items.hpp"
#include "net.hpp"

int *pathMapFlying = NULL;
int *pathMapGrounded = NULL;
int pathMapZone = 1;

#define STRAIGHTCOST 10
#define DIAGONALCOST 14

/*-------------------------------------------------------------------------------

	heuristic

	Uses the Manhattan Method to calculate the path heuristic from x1, y1
	to x2, y2

-------------------------------------------------------------------------------*/

Uint32 heuristic(int x1, int y1, int x2, int y2) {
	Uint32 h;
	h = (abs(x2-x1)+abs(y2-y1))*STRAIGHTCOST;
	return h;
}

/*-------------------------------------------------------------------------------

	heapAdd

	Adds a pathnode to the heap

-------------------------------------------------------------------------------*/

pathnode_t **heapAdd(pathnode_t **heap, pathnode_t *pathnode, long *length) {
	long x;

	*length += 1;
	heap[*length]=pathnode;
	for( x=*length; x>1; x=x>>1 ) {
		if( heap[x>>1]->g+heap[x>>1]->h > pathnode->g+pathnode->h ) {
			pathnode=heap[x];
			heap[x]=heap[x>>1];
			heap[x>>1]=pathnode;
		} else {
			heap[x]=pathnode;
			break;
		}
	}

	return heap;
}

/*-------------------------------------------------------------------------------

	heapRemove

	Removes a pathnode from the heap

-------------------------------------------------------------------------------*/

pathnode_t **heapRemove(pathnode_t **heap, long *length) {
	long u, v=1;
	pathnode_t *pathnode;

	heap[1]=heap[*length];
	*length -= 1;
	while( 1 ) {
		u = v;

		if( (u<<1)+1 <= *length ) {
			if( heap[u<<1]->g+heap[u<<1]->h < heap[u]->g+heap[u]->h )
				v = u<<1;
			if( heap[(u<<1)+1]->g+heap[(u<<1)+1]->h < heap[v]->g+heap[v]->h )
				v = (u<<1)+1;
		} else if( u<<1 <= *length ) {
			if( heap[u<<1]->g+heap[u<<1]->h < heap[u]->g+heap[u]->h )
				v = u<<1;
		}

		if( u != v ) {
			pathnode = heap[u];
			heap[u] = heap[v];
			heap[v] = pathnode;
		} else {
			break;
		}
	}

	return heap;
}

/*-------------------------------------------------------------------------------

	pathCheckObstacle

-------------------------------------------------------------------------------*/

int pathCheckObstacle(long x, long y, Entity *my, Entity *target) {
	int u = std::min(std::max<unsigned int>(0,x>>4),map.width);
	int v = std::min(std::max<unsigned int>(0,y>>4),map.height); //TODO: Why are int and long int being compared?
	int index = v*MAPLAYERS+u*MAPLAYERS*map.height;

	if( map.tiles[OBSTACLELAYER+index] || !map.tiles[index] || lavatiles[map.tiles[index]] )
		return 1;

	node_t *node;
	for( node=map.entities->first; node!=NULL; node=node->next ) {
		Entity *entity = (Entity *)node->element;
		if( entity->sprite==14 || entity->sprite==15 || entity->sprite==19 || entity->sprite==20 || entity->sprite==39 || entity->sprite==44 ) {
			if( (int)floor(entity->x/16)==u && (int)floor(entity->y/16)==v ) {
				return 1;
			}
		}
	}

	return 0;
}

/*-------------------------------------------------------------------------------

	generatePath

	generates a path through the level using the A* pathfinding algorithm.
	Takes a starting point and destination in map coordinates, and returns
	a list of pathnodes which lead from the starting point to the destination.
	If no path connecting the two positions is possible, generatePath returns
	NULL.

-------------------------------------------------------------------------------*/

list_t *generatePath(int x1, int y1, int x2, int y2, Entity *my, Entity *target) {
	if (!my)
		return NULL;

	pathnode_t *pathnode, *childnode, *parent;
	list_t *openList, *closedList;
	list_t *path;
	node_t *node;
	bool alreadyadded;
	Sint32 x, y, z, h, g;
	pathnode_t **binaryheap;
	long heaplength=0;
	node_t *entityNode = NULL;

	int *pathMap = (int *) calloc(map.width*map.height,sizeof(int));

	bool levitating=FALSE;

	x1 = std::min<unsigned int>(std::max(0,x1),map.width-1);
	y1 = std::min<unsigned int>(std::max(0,y1),map.height-1);
	x2 = std::min<unsigned int>(std::max(0,x2),map.width-1);
	y2 = std::min<unsigned int>(std::max(0,y2),map.height-1); //TODO: Why are int and unsigned int being compared?

	// get levitation status
	Stat *stats = my->getStats();
	if( stats ) {
		if( stats->EFFECTS[EFF_LEVITATING] == TRUE )
			levitating=TRUE;
		if( stats->ring != NULL )
			if( stats->ring->type == RING_LEVITATION )
				levitating = TRUE;
		if( stats->shoes != NULL )
			if( stats->shoes->type == STEEL_BOOTS_LEVITATION )
				levitating = TRUE;
	}
	if( my ) {
		if( my->behavior == &actItem || my->behavior == &actArrowTrap || my->behavior == &actBoulderTrap ) {
			levitating = TRUE;
		}
	}

	if( !loading ) {
		if( levitating )
			memcpy(pathMap,pathMapFlying,map.width*map.height*sizeof(int));
		else
			memcpy(pathMap,pathMapGrounded,map.width*map.height*sizeof(int));
	}

	int myPathMap = pathMap[y1+x1*map.height];
	if( !loading ) {
		if( !myPathMap || myPathMap!=pathMap[y2+x2*map.height] || !pathMap[y2+x2*map.height] || (x1==x2 && y1==y2) ) {
			free(pathMap);
			return NULL;
		}
	}

	for( entityNode=map.entities->first; entityNode!=NULL; entityNode=entityNode->next ) {
		Entity *entity = (Entity *)entityNode->element;
		if( entity->flags[PASSABLE] )
			continue;
		if( entity->behavior==&actDoorFrame || entity->behavior==&actDoor )
			continue;
		if( entity==target || entity==my )
			continue;
		if ( my->checkFriend(target) )
			continue;
		int x = std::min<unsigned int>(std::max<int>(0,entity->x/16),map.width-1); //TODO: Why are int and double being compared? And why are int and unsigned int being compared?
		int y = std::min<unsigned int>(std::max<int>(0,entity->y/16),map.height-1); //TODO: Why are int and double being compared? And why are int and unsigned int being compared?
		pathMap[y+x*map.height] = 0;
	}

	openList = (list_t *) malloc(sizeof(list_t));
	openList->first = NULL;
	openList->last = NULL;
	closedList = (list_t *) malloc(sizeof(list_t));
	closedList->first = NULL;
	closedList->last = NULL;
	binaryheap = (pathnode_t **) malloc(sizeof(pathnode_t *)*map.width*map.height);
	binaryheap[0] = NULL;

	// create starting node in list
	pathnode = newPathnode(openList,x1,y1,NULL,1);
	pathnode->g=0;
	pathnode->h=heuristic(x1,y1,x2,y2);
	heapAdd(binaryheap,pathnode,&heaplength);

	while( openList->first != NULL ) {
		/*pathnode = (pathnode_t *)openList->first->element;
		for( node=openList->first; node!=NULL; node=node->next ) {
			childnode = (pathnode_t *)node->element;
			if( childnode->g+childnode->h < pathnode->g+pathnode->h )
				pathnode=childnode;
		}*/
		pathnode = binaryheap[1];

		x = pathnode->x;
		y = pathnode->y;
		h = pathnode->h;
		g = pathnode->g;
		parent = pathnode->parent;
		list_RemoveNode(pathnode->node);
		heapRemove(binaryheap,&heaplength);
		pathnode = newPathnode(closedList,x,y,parent,1);
		pathnode->h = h;
		pathnode->g = g;
		if( pathnode->x == x2 && pathnode->y == y2 ) {
			// found target, retrace path
			path = (list_t *) malloc(sizeof(list_t));
			path->first = NULL;
			path->last = NULL;
			list_FreeAll(openList);
			free(openList);
			for( childnode=pathnode; childnode!=NULL; childnode=pathnode->parent ) {
				if( childnode->parent==NULL ) {
					parent->parent=NULL;
					break;
				}
				parent = newPathnode(path,childnode->x,childnode->y,childnode->parent,0);
				pathnode=pathnode->parent;
			}
			list_FreeAll(closedList);
			free(closedList);
			free(binaryheap);
			free(pathMap);
			return path;
		}

		// expand search
		for( y=-1; y<=1; y++ ) {
			for( x=-1; x<=1; x++ ) {
				if( x==0 && y==0 )
					continue;
				z = 0;
				if( !loading ) {
					if( !pathMap[(pathnode->y+y)+(pathnode->x+x)*map.height] )
						z++;
					if( x&&y ) {
						if( !pathMap[(pathnode->y)+(pathnode->x+x)*map.height] )
							z++;
						if( !pathMap[(pathnode->y+y)+(pathnode->x)*map.height] )
							z++;
					}
				} else {
					if( pathCheckObstacle(((pathnode->x+x)<<4)+8,((pathnode->y+y)<<4)+8,my,target) )
						z++;
					if( x&&y ) {
						if( pathCheckObstacle(((pathnode->x)<<4)+8,((pathnode->y+y)<<4)+8,my,target) )
							z++;
						if( pathCheckObstacle(((pathnode->x+x)<<4)+8,((pathnode->y)<<4)+8,my,target) )
							z++;
					}
				}
				if( !z ) {
					alreadyadded=FALSE;
					for( node=closedList->first; node!=NULL; node=node->next ) {
						childnode = (pathnode_t *)node->element;
						if( childnode->x==pathnode->x+x && childnode->y==pathnode->y+y ) {
							alreadyadded=TRUE;
							break;
						}
					}
					for( node=openList->first; node!=NULL; node=node->next ) {
						childnode = (pathnode_t *)node->element;
						if( childnode->x==pathnode->x+x && childnode->y==pathnode->y+y ) {
							alreadyadded=TRUE;
							if( x&&y ) {
								if( childnode->g > pathnode->g+DIAGONALCOST ) {
									childnode->parent=pathnode;
									childnode->g=pathnode->g+DIAGONALCOST;
								}
							} else {
								if( childnode->g > pathnode->g+STRAIGHTCOST ) {
									childnode->parent=pathnode;
									childnode->g=pathnode->g+STRAIGHTCOST;
								}
							}
							break;
						}
					}
					if( alreadyadded==FALSE ) {
						if( list_Size(openList)>=1000 ) {
							list_FreeAll(openList);
							list_FreeAll(closedList);
							free(openList);
							free(closedList);
							free(binaryheap);
							free(pathMap);
							return NULL;
						}
						childnode = newPathnode(openList,pathnode->x+x,pathnode->y+y,pathnode,1);
						if( x&&y )
							childnode->g = pathnode->g+DIAGONALCOST;
						else
							childnode->g = pathnode->g+STRAIGHTCOST;
						childnode->h=heuristic(childnode->x,childnode->y,x2,y2);
						heapAdd(binaryheap,childnode,&heaplength);
					}
				}
			}
		}
	}
	list_FreeAll(openList);
	list_FreeAll(closedList);
	free(openList);
	free(closedList);
	free(binaryheap);
	free(pathMap);
	return NULL;
}

/*-------------------------------------------------------------------------------

	generatePathMaps

	Maps out islands in the game map for the path generator

-------------------------------------------------------------------------------*/

void fillPathMap(int *pathMap, int x, int y, int zone);

void generatePathMaps() {
	int x, y;

	if( pathMapGrounded )
		free(pathMapGrounded);
	pathMapGrounded = (int *) calloc(map.width*map.height,sizeof(int));
	if( pathMapFlying )
		free(pathMapFlying);
	pathMapFlying = (int *) calloc(map.width*map.height,sizeof(int));

	pathMapZone = 1;
	for( y=0; y<map.height; y++ ) {
		for( x=0; x<map.width; x++ ) {
			if( !pathMapGrounded[y+x*map.height] ) {
				fillPathMap(pathMapGrounded,x,y,pathMapZone);
			}
			if( !pathMapFlying[y+x*map.height] ) {
				fillPathMap(pathMapFlying,x,y,pathMapZone);
			}
		}
	}
}

void fillPathMap(int *pathMap, int x, int y, int zone) {
	bool obstacle = TRUE;

	int index = y*MAPLAYERS+x*MAPLAYERS*map.height;
	if( !map.tiles[OBSTACLELAYER+index] && map.tiles[index] && !animatedtiles[map.tiles[index]] ) {
		obstacle = FALSE;
	} else if( pathMap==pathMapFlying && !map.tiles[OBSTACLELAYER+index] ) {
		obstacle = FALSE;
	}
	if( obstacle==FALSE ) {
		node_t *node;
		list_t *list = checkTileForEntity(x,y);
		if( list ) {
			for( node=list->first; node!=NULL; node=node->next ) {
				Entity *entity = (Entity *)node->element;
				if( entity ) {
					if( entity->behavior==&actHeadstone || entity->behavior==&actSink || entity->behavior==&actFountain ) {
						obstacle = TRUE;
						break;
					} else if( entity->behavior==&actWallBuilder || entity->behavior==&actWallBuster ) {
						obstacle = FALSE;
						break;
					}
				}
			}
			list_FreeAll(list);
			free(list);
		}
	}

	if( obstacle )
		return;

	pathMap[y+x*map.height] = zone;
	bool repeat;
	do {
		repeat = FALSE;

		int u, v;
		for( u=0; u<map.width; u++ ) {
			for( v=0; v<map.height; v++ ) {
				if( pathMap[v+u*map.height]==zone ) {
					if( u < map.width-1 ) {
						if( !pathMap[v+(u+1)*map.height] ) {
							bool foundObstacle=FALSE;
							bool foundWallModifier=FALSE;
							list_t *list = checkTileForEntity(u+1,v);
							if( list ) {
								node_t *node;
								for( node=list->first; node!=NULL; node=node->next ) {
									Entity *entity = (Entity *)node->element;
									if( entity ) {
										if( entity->behavior==&actHeadstone || entity->behavior==&actSink || entity->behavior==&actFountain ) {
											foundObstacle=TRUE;
											break;
										} else if( entity->behavior==&actWallBuilder || entity->behavior==&actWallBuster ) {
											foundWallModifier=TRUE;
											break;
										}
									}
								}
								if( foundWallModifier ) {
									pathMap[v+(u+1)*map.height] = zone;
									repeat = TRUE;
								}
								list_FreeAll(list);
								free(list);
							}
							if( !foundWallModifier && !foundObstacle ) {
								int index = v*MAPLAYERS+(u+1)*MAPLAYERS*map.height;
								if( !map.tiles[OBSTACLELAYER+index] && (pathMap==pathMapFlying || (map.tiles[index] && !animatedtiles[map.tiles[index]])) ) {
									pathMap[v+(u+1)*map.height] = zone;
									repeat = TRUE;
								}
							}
						}
					}
					if( u > 0 ) {
						if( !pathMap[v+(u-1)*map.height] ) {
							bool foundObstacle=FALSE;
							bool foundWallModifier=FALSE;
							list_t *list = checkTileForEntity(u-1,v);
							if( list ) {
								node_t *node;
								for( node=list->first; node!=NULL; node=node->next ) {
									Entity *entity = (Entity *)node->element;
									if( entity ) {
										if( entity->behavior==&actHeadstone || entity->behavior==&actSink || entity->behavior==&actFountain ) {
											foundObstacle=TRUE;
											break;
										} else if( entity->behavior==&actWallBuilder || entity->behavior==&actWallBuster ) {
											foundWallModifier=TRUE;
											break;
										}
									}
								}
								if( foundWallModifier ) {
									pathMap[v+(u-1)*map.height] = zone;
									repeat = TRUE;
								}
								list_FreeAll(list);
								free(list);
							}
							if( !foundWallModifier && !foundObstacle ) {
								int index = v*MAPLAYERS+(u-1)*MAPLAYERS*map.height;
								if( !map.tiles[OBSTACLELAYER+index] && (pathMap==pathMapFlying || (map.tiles[index] && !animatedtiles[map.tiles[index]])) ) {
									pathMap[v+(u-1)*map.height] = zone;
									repeat = TRUE;
								}
							}
						}
					}
					if( v < map.height-1 ) {
						if( !pathMap[(v+1)+u*map.height] ) {
							bool foundObstacle=FALSE;
							bool foundWallModifier=FALSE;
							list_t *list = checkTileForEntity(u,v+1);
							if( list ) {
								node_t *node;
								for( node=list->first; node!=NULL; node=node->next ) {
									Entity *entity = (Entity *)node->element;
									if( entity ) {
										if( entity->behavior==&actHeadstone || entity->behavior==&actSink || entity->behavior==&actFountain ) {
											foundObstacle=TRUE;
											break;
										} else if( entity->behavior==&actWallBuilder || entity->behavior==&actWallBuster ) {
											foundWallModifier=TRUE;
											break;
										}
									}
								}
								if( foundWallModifier ) {
									pathMap[(v+1)+u*map.height] = zone;
									repeat = TRUE;
								}
								list_FreeAll(list);
								free(list);
							}
							if( !foundWallModifier && !foundObstacle ) {
								int index = (v+1)*MAPLAYERS+u*MAPLAYERS*map.height;
								if( !map.tiles[OBSTACLELAYER+index] && (pathMap==pathMapFlying || (map.tiles[index] && !animatedtiles[map.tiles[index]])) ) {
									pathMap[(v+1)+u*map.height] = zone;
									repeat = TRUE;
								}
							}
						}
					}
					if( v > 0 ) {
						if( !pathMap[(v-1)+u*map.height] ) {
							bool foundObstacle=FALSE;
							bool foundWallModifier=FALSE;
							list_t *list = checkTileForEntity(u,v-1);
							if( list ) {
								node_t *node;
								for( node=list->first; node!=NULL; node=node->next ) {
									Entity *entity = (Entity *)node->element;
									if( entity ) {
										if( entity->behavior==&actHeadstone || entity->behavior==&actSink || entity->behavior==&actFountain ) {
											foundObstacle=TRUE;
											break;
										} else if( entity->behavior==&actWallBuilder || entity->behavior==&actWallBuster ) {
											foundWallModifier=TRUE;
											break;
										}
									}
								}
								if( foundWallModifier ) {
									pathMap[(v-1)+u*map.height] = zone;
									repeat = TRUE;
								}
								list_FreeAll(list);
								free(list);
							}
							if( !foundWallModifier && !foundObstacle ) {
								int index = (v-1)*MAPLAYERS+u*MAPLAYERS*map.height;
								if( !map.tiles[OBSTACLELAYER+index] && (pathMap==pathMapFlying || (map.tiles[index] && !animatedtiles[map.tiles[index]])) ) {
									pathMap[(v-1)+u*map.height] = zone;
									repeat = TRUE;
								}
							}
						}
					}
				}
			}
		}
	} while( repeat );
	pathMapZone++;
}