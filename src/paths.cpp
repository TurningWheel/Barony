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
#include "magic/magic.hpp"
#include "mod_tools.hpp"

int* pathMapFlying = NULL;
int* pathMapGrounded = NULL;
int pathMapZone = 1;

#define STRAIGHTCOST 10
#define DIAGONALCOST 14

class GateGraph
{
	std::unordered_map<int, std::unordered_set<int>> edges;
public:
	int* mapSubzones;
	int numSubzones = 1;
	static const int GATE_GRAPH_GROUNDED = 0;
	static const int GATE_GRAPH_FLYING = 1;
	static const int GATE_GRAPH_NUM_PATHMAPS = 2;
	static const int DIR_EASTWEST = 0;
	static const int DIR_NORTHSOUTH = 1;
	int parentMapType = 0;
	bool bIsInit = false;
	struct GateNode_t
	{
		int x, y, zone1, zone2;
		Uint32 uid = 0;
		int direction = 0;
		GateNode_t(int x, int y, int zone1, int zone2, Uint32 uid, int direction) :
			x(x),
			y(y),
			zone1(zone1),
			zone2(zone2),
			uid(uid),
			direction(direction)
		{};
		GateNode_t()
		{
			zone1 = -1;
			zone2 = -1;
			uid = 0;
		}
	};

	//class CachedPaths_t
	//{
	//	bool bDirty = false;
	//public:
	//	std::unordered_map<int, int> path;
	//	void setDirty(bool _bDirty) { bDirty = _bDirty; }
	//	void setPath(std::unordered_map<int, int>& newPath)
	//	{
	//		path = newPath;
	//		bDirty = false;
	//	}
	//	bool isDirty() { return bDirty; }
	//};
	//std::unordered_map<Uint32, CachedPaths_t> cachedPaths; // key: zone/zone*10000

	std::map<int, GateNode_t> gateNodes;
	std::unordered_map<int, std::unordered_set<int>> connectedZones;
	void buildGraph(const int parentMapType);
	void fillPathMap(int x, int y);
	bool generatePath(Entity* my, int x1, int y1, int x2, int y2);
	void reset()
	{
		if ( mapSubzones )
		{
			free(mapSubzones);
		}
		gateNodes.clear();
		connectedZones.clear();
		edges.clear();
		//cachedPaths.clear();
		numSubzones = 1;
		bIsInit = false;
	}
	~GateGraph()
	{
		reset();
	}
	void addEdge(int a, int b)
	{
		edges[a].insert(b); // doubly linked
		edges[b].insert(a);
	}
	static GateNode_t defaultGate;
	GateNode_t& getGate(int x, int y)
	{
		Uint32 key = x + 10000 * y;
		auto find = gateNodes.find(key);
		if ( find == gateNodes.end() )
		{
			return defaultGate;
		}
		return (*find).second;
	}
	void addGate(int x, int y, int zone1, int zone2, Uint32 uid, int direction)
	{
		Uint32 key = x + 10000 * y;
		gateNodes[key] = GateNode_t(x, y, zone1, zone2, uid, direction);
	}
	bool isConnected(int start, int end)
	{
		if ( start == end ) { return true; }
		if ( start == 0 || end == 0 ) { return false; }

		if ( connectedZones[start].find(end) != connectedZones[start].end() )
		{
			return true;
		}
		if ( connectedZones[end].find(start) != connectedZones[end].end() )
		{
			return true;
		}

		std::queue<int> frontier;
		frontier.push(start);

		std::unordered_set<int> visited;
		visited.insert(start);
		while ( !frontier.empty() )
		{
			int current = frontier.front();
			frontier.pop();
			if ( current == end )
			{
				return true;
			}
			for ( auto neighbour : edges[current] )
			{
				if ( visited.find(neighbour) == visited.end() )
				{
					frontier.push(neighbour);
					visited.insert(neighbour);
				}
			}
		}
		return false;
	}

	std::unordered_set<int> getAccessibleNeighbours(int index, bool ignorePassable)
	{
		std::unordered_set<int> neighbours;
		for ( auto zone : edges[index] )
		{
			if ( ignorePassable )
			{
				neighbours.insert(zone);
				continue;
			}

			for ( auto& pair : gateNodes )
			{
				auto& gate = pair.second;
				// check the physical gate status
				if ( (gate.zone1 == zone && gate.zone2 == index)
					|| (gate.zone2 == zone && gate.zone1 == index) )
				{
					list_t* list = checkTileForEntity(gate.x, gate.y);
					bool isImpassable = false;
					if ( list )
					{
						node_t* node;
						for ( node = list->first; node != NULL; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity )
							{
								if ( entity->behavior == &actGate )
								{
									if ( !entity->flags[PASSABLE] )
									{
										isImpassable = true;
									}
									break;
								}
							}
						}
					}
					if ( !isImpassable )
					{
						neighbours.insert(zone);
					}
				}
			}
		}
		return neighbours;
	}

	std::unordered_map<int, int> getPath(int start, int end)
	{
		Uint32 key = start + end * 10000;
		if ( end < start )
		{
			key = end + start * 10000;
		}
		/*auto cacheFind = cachedPaths.find(key);
		if ( cacheFind != cachedPaths.end() )
		{
			if ( !cacheFind->second.isDirty() )
			{
				messagePlayer(0, MESSAGE_DEBUG, "Found cached path for key %d", key);
				return cacheFind->second.path;
			}
			else
			{
				messagePlayer(0, MESSAGE_DEBUG, "Found dirty path for key %d", key);
			}
		}*/
		//auto& cachedPath = cachedPaths[key];
		//cachedPath.setDirty(false);

		std::queue<int> frontier;
		frontier.push(start);

		std::unordered_map<int, int> visited;
		visited[start] = start;

		while ( !frontier.empty() )
		{
			int current = frontier.front();
			frontier.pop();
			if ( current == end )
			{
				return visited;
			}
			for ( auto neighbour : getAccessibleNeighbours(current, false) )
			{
				if ( visited.find(neighbour) == visited.end() )
				{
					frontier.push(neighbour);
					visited[neighbour] = current;
				}
			}
		}

		return visited;
	}

	void debugPaths();
};
GateGraph gateGraph[GateGraph::GATE_GRAPH_NUM_PATHMAPS];
GateGraph::GateNode_t GateGraph::defaultGate(-1, -1, -1, -1, 0, GateGraph::DIR_EASTWEST);
void updateGatePath(Entity& entity)
{
	return;
	/*int ix = ((int)entity.x >> 4);
	int iy = ((int)entity.y >> 4);

	for ( int i = 0; i < GateGraph::GATE_GRAPH_NUM_PATHMAPS; ++i )
	{
		auto& gate = gateGraph[i].getGate(ix, iy);
		if ( gate.zone1 != -1 )
		{
			int zone1 = std::min(gate.zone1, gate.zone2);
			int zone2 = std::max(gate.zone1, gate.zone2);
			Uint32 key = zone1 + zone2 * 10000;
			for ( auto pair : gateGraph[i].cachedPaths )
			{
				for ( auto node : pair.second.path )
				{

				}
			}
			auto find = gateGraph[i].cachedPaths.find(key);
			if ( find != gateGraph[i].cachedPaths.end() )
			{
				find->second.setDirty(true);
			}
		}
	}*/
}

/*-------------------------------------------------------------------------------

	heuristic

	Uses the Manhattan Method to calculate the path heuristic from x1, y1
	to x2, y2

-------------------------------------------------------------------------------*/

Uint32 heuristic(int x1, int y1, int x2, int y2)
{
	Uint32 h;
	h = (abs(x2 - x1) + abs(y2 - y1)) * STRAIGHTCOST;
	return h;
}

/*-------------------------------------------------------------------------------

	pathCheckObstacle

	only used during the load process because it's a pretty slow way of
	looking for potential obstacles on a map, and also it checks against
	sprites which only correspond to entities before assignActions is called

-------------------------------------------------------------------------------*/

int pathCheckObstacle(int x, int y, Entity* my, Entity* target)
{
	const int u = std::min(std::max(0, x >> 4), (int)map.width - 1);
	const int v = std::min(std::max(0, y >> 4), (int)map.height - 1);
	const int index = v * MAPLAYERS + u * MAPLAYERS * map.height;

	if ( map.tiles[OBSTACLELAYER + index] || !map.tiles[index] || lavatiles[map.tiles[index]] )
	{
		return 1;
	}

	// entities not passable during this stage normally, hell generation makes entry gates passable
	for ( node_t* node = map.entities->first; node != nullptr; node = node->next )
	{
		Entity* entity = (Entity*)node->element;
		if (entity == my || entity == target)
		{
			continue;
		}
		if ( entity->sprite == 179 ) // collider
		{
			if ( (int)floor(entity->x / 16) == u && (int)floor(entity->y / 16) == v )
			{
				if ( entity->colliderHasCollision != 0 || entity->colliderDiggable != 0 )
				{
					return 1;
				}
				auto find = EditorEntityData_t::colliderData.find(entity->colliderDamageTypes);
				if ( find != EditorEntityData_t::colliderData.end() )
				{
					auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[find->second.damageCalculationType];
					if ( !colliderDmgType.allowNPCPathing )
					{
						return 1;
					}
				}
			}
		}
		else if ( entity->sprite == 14		// fountain
			|| entity->sprite == 15		// sink
			|| (entity->sprite == 19 && !entity->flags[PASSABLE]) // gate
			|| (entity->sprite == 20 && !entity->flags[PASSABLE]) // gate 2
			|| entity->sprite == 39		// head stone
			|| entity->sprite == 44		// boulder?
			|| entity->sprite == 106 	// power crystal
			|| entity->sprite == 108	// stalag column
			|| entity->sprite == 109	// stalagmite
			|| entity->sprite == 110	// stalagmite
			|| entity->sprite == 116	// pedestal
			|| entity->sprite == 124	// column
			|| entity->sprite == 126	// piston
			|| entity->sprite == 169	// statue
			|| entity->sprite == 177	// shrine
			|| entity->sprite == 178	// spell shrine
			|| entity->sprite == 1481	// daedalus shrine
			)
		{
			if ( (int)floor(entity->x / 16) == u && (int)floor(entity->y / 16) == v )
			{
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

typedef std::pair<int, int> pairtype;
struct pair_hash
{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

static std::chrono::high_resolution_clock::time_point pathtime;
static std::chrono::high_resolution_clock::time_point starttime;
static std::chrono::microseconds ms(0);
static Uint32 updatedOnTick = 0;
static ConsoleVariable<int> cvar_pathlimit("/pathlimit", 200);
static ConsoleVariable<bool> cvar_pathing_debug("/pathing_debug", false);
static ConsoleVariable<bool> cvar_pathing_collider_npc("/pathing_collider_npc", true);
int lastGeneratePathTries = 0;
list_t* generatePath(int x1, int y1, int x2, int y2, Entity* my, Entity* target, GeneratePathTypes pathingType, bool lavaIsPassable)
{
	if ( *cvar_pathing_debug )
	{
		pathtime = std::chrono::high_resolution_clock::now();
		if ( updatedOnTick != ticks )
		{
			DebugStats.gui1 = starttime;
			DebugStats.gui2 = {};
			starttime = std::chrono::high_resolution_clock::now();
			updatedOnTick = ticks;
		}
	}
	if (!my)
	{
		if ( *cvar_pathing_debug )
		{
			auto now = std::chrono::high_resolution_clock::now();
			ms = std::chrono::duration_cast<std::chrono::microseconds>(now - pathtime);
			DebugStats.gui2 = DebugStats.gui2 + ms;
		}
		lastGeneratePathTries = 0;
		return NULL;
	}

	x1 = std::min(std::max(0, x1), (int)map.width - 1);
	y1 = std::min(std::max(0, y1), (int)map.height - 1);
	x2 = std::min(std::max(0, x2), (int)map.width - 1);
	y2 = std::min(std::max(0, y2), (int)map.height - 1);

	// get levitation status
	bool levitating = false;
	Stat* stats = my->getStats();
	if ( stats )
	{
		levitating = isLevitating(stats);
	}
	if ( my )
	{
		if ( my->behavior == &actItem || my->behavior == &actArrowTrap || my->behavior == &actBoulderTrap )
		{
			levitating = true;
		}
	}

	// for boulders falling and checking if a player can reach the ladder.
	bool playerCheckPathToExit = (my && my->behavior == &actPlayer
		&& target && (target->behavior == &actLadder || target->behavior == &actPortal));
	bool playerCheckAchievement = (my && my->behavior == &actPlayer
		&& target && (target->behavior == &actBomb || target->behavior == &actPlayerLimb || target->behavior == &actItem || target->behavior == &actSwitch));

	int* pathMap = (int*) calloc(map.width * map.height, sizeof(int));
	int pathMapType = GateGraph::GATE_GRAPH_GROUNDED;
	bool waterWalking = my && my->isWaterWalking();
	bool lavaWalking = my && my->isLavaWalking();
	if ( !loading )
	{
		if ( levitating || playerCheckPathToExit || waterWalking || lavaWalking )
		{
			memcpy(pathMap, pathMapFlying, map.width * map.height * sizeof(int));
			pathMapType = GateGraph::GATE_GRAPH_FLYING;
		}
		else
		{
			memcpy(pathMap, pathMapGrounded, map.width * map.height * sizeof(int));
		}
	}

	int myPathMap = pathMap[y1 + x1 * map.height];
	if ( !loading )
	{
		if ( !myPathMap || myPathMap != pathMap[y2 + x2 * map.height] || !pathMap[y2 + x2 * map.height] || (x1 == x2 && y1 == y2) )
		{
			free(pathMap);
			if ( *cvar_pathing_debug )
			{
				auto now = std::chrono::high_resolution_clock::now();
				ms = std::chrono::duration_cast<std::chrono::microseconds>(now - pathtime);
				DebugStats.gui2 = DebugStats.gui2 + ms;
			}
			lastGeneratePathTries = 0;
			if ( my->behavior == &actMonster
				&& (pathingType == GENERATE_PATH_ALLY_FOLLOW
					|| pathingType == GENERATE_PATH_ALLY_FOLLOW2) )
			{
				monsterAllyFormations.updateOnPathFail(my->getUID(), my);
			}
			return NULL;
		}
		if ( my->behavior == &actMonster )
		{
			if ( gateGraph[pathMapType].bIsInit )
			{
				bool bGatePath = gateGraph[pathMapType].generatePath(my, x1, y1, x2, y2);
				if ( !bGatePath )
				{
					//messagePlayer(0, MESSAGE_DEBUG, "GATE GRAPH: %.4f", out1);
					free(pathMap);
					if ( *cvar_pathing_debug )
					{
						auto now = std::chrono::high_resolution_clock::now();
						ms = std::chrono::duration_cast<std::chrono::microseconds>(now - pathtime);
						DebugStats.gui2 = DebugStats.gui2 + ms;
					}
					lastGeneratePathTries = 0;
					if ( my->behavior == &actMonster
						&& (pathingType == GENERATE_PATH_ALLY_FOLLOW
							|| pathingType == GENERATE_PATH_ALLY_FOLLOW2) )
					{
						monsterAllyFormations.updateOnPathFail(my->getUID(), my);
					}
					return NULL;
				}
			}
		}
	}

	// for boulders falling and checking if a player can reach the ladder.
	// if we're not levitating, we use the flying path map (for water/lava) and here we remove the empty air tiles from the pathMap.
	if ( playerCheckPathToExit && !levitating )
	{
		for ( int y = 0; y < map.height; ++y )
		{
			for ( int x = 0; x < map.width; ++x )
			{
				if ( !map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
				{
					pathMap[y + x * map.height] = 0;
				}
			}
		}
	}
	else if ( !levitating && (waterWalking || lavaWalking) )
	{
		for ( int y = 0; y < map.height; ++y )
		{
			for ( int x = 0; x < map.width; ++x )
			{
				int index = y * MAPLAYERS + x * MAPLAYERS * map.height;
				if ( !map.tiles[index] )
				{
					pathMap[y + x * map.height] = 0;
				}
				else if ( lavatiles[map.tiles[index]] && !lavaWalking )
				{
					pathMap[y + x * map.height] = 0;
				}
				else if ( swimmingtiles[map.tiles[index]] && !waterWalking )
				{
					pathMap[y + x * map.height] = 0;
				}
			}
		}
	}

	Uint32 standingOnTrap = 0; // 0 - not checked.
	for ( auto entityNode = map.entities->first; entityNode != nullptr; entityNode = entityNode->next )
	{
		Entity* entity = (Entity*)entityNode->element;
		if ( entity->flags[PASSABLE] )
		{
			if ( entity->behavior == &actSpearTrap 
				&& (my->getRace() == HUMAN || my->monsterAllyGetPlayerLeader()) )
			{
				// humans/followers know better than that!

				// unless they're standing on a trap...
				if ( standingOnTrap == 0 )
				{
					if ( my->getEntityBonusTrapResist() == 100 )
					{
						standingOnTrap = 1; // 1 - standing on the trap.
						continue;
					}

					std::vector<list_t*> entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 0);
					for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !standingOnTrap; ++it )
					{
						list_t* currentList = *it;
						node_t* node;
						if ( currentList )
						{
							for ( node = currentList->first; node != nullptr && !standingOnTrap; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entity && entity->behavior == &actSpearTrap )
								{
									standingOnTrap = 1; // 1 - standing on the trap.
								}
							}
						}
					}
					if ( standingOnTrap == 0 )
					{
						standingOnTrap = 2; // 2 - have run the check but failed.
					}
				}
				if ( standingOnTrap == 1 )
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
		if ( entity->behavior == &actDoorFrame || entity->behavior == &actDoor || entity->behavior == &actMagicMissile )
		{
			continue;
		}
		if ( playerCheckPathToExit && entity->behavior == &actGate )
		{
			continue;
		}
		if ( entity == target || entity == my )
		{
			continue;
		}
		if ( entity->behavior == &actMonster && (!my->checkEnemy(entity) && !entity->isInertMimic()) )
		{
			continue;
		}
		if ( entity->behavior == &actPlayer && my->monsterAllyIndex >= 0 
			&& (my->monsterTarget == 0 || my->monsterAllyState == ALLY_STATE_MOVETO) )
		{
			continue;
		}
		if ( lavaIsPassable &&
			(entity->sprite == 41
			|| lavatiles[map.tiles[static_cast<int>(entity->y / 16) * MAPLAYERS + static_cast<int>(entity->x / 16) * MAPLAYERS * map.height]]
			|| swimmingtiles[map.tiles[static_cast<int>(entity->y / 16) * MAPLAYERS + static_cast<int>(entity->x / 16) * MAPLAYERS * map.height]])
			)
		{
			//Fix to make ladders generate in hell.
			continue;
		}
		if (playerCheckAchievement && (entity->behavior == &actMonster || entity->behavior == &actPlayer))
		{
			continue;
		}
		if (stats && stats->type == MINOTAUR && (entity->behavior == &actBoulder || entity->behavior == &::actDaedalusShrine || (entity->isDamageableCollider()
			&& (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_MINO))))
		{
			// minotaurs bust through boulders, not an obstacle
			continue;
		}
		else if ( (my && my->behavior == &actMonster) && entity->isDamageableCollider()
			&& (entity->colliderHasCollision & EditorEntityData_t::COLLIDER_COLLISION_FLAG_NPC))
		{
			if ( *cvar_pathing_collider_npc )
			{
				continue;
			}
		}
		int x = std::min<unsigned int>(std::max<int>(0, entity->x / 16), map.width - 1); //TODO: Why are int and double being compared? And why are int and unsigned int being compared?
		int y = std::min<unsigned int>(std::max<int>(0, entity->y / 16), map.height - 1); //TODO: Why are int and double being compared? And why are int and unsigned int being compared?
		pathMap[y + x * map.height] = 0;
	}

    // here begins actual A* code:
    struct queue_type {
        int x, y;
        std::unordered_map<pairtype, pathnode_t, pair_hash>& openSet;
        bool operator>(const queue_type& rhs) const {
            const auto find1 = openSet.find({x, y});
            assert(find1 != openSet.end());
            const auto& lhs_node = find1->second;
            const auto find2 = rhs.openSet.find({rhs.x, rhs.y});
            assert(find2 != openSet.end());
            const auto& rhs_node = find2->second;
            return lhs_node.g + lhs_node.h > rhs_node.g + rhs_node.h;
        }
        queue_type& operator=(const queue_type& rhs) {
            x = rhs.x;
            y = rhs.y;
            return *this;
        }
    };
    std::priority_queue<queue_type, std::vector<queue_type>, std::greater<queue_type>> queue;
	std::unordered_map<pairtype, pathnode_t, pair_hash> openSet, closedSet;

	// create starting node in list
    const auto firstNode = pathnode_t{x1, y1, 0, heuristic(x1, y1, x2, y2), -1, -1};
    openSet.insert({pairtype{firstNode.x, firstNode.y}, firstNode});
	queue.push({x1, y1, openSet});
	int tries = 0;
	int maxtries = *cvar_pathlimit;
	static ConsoleVariable<int> cvar_pathlimit_idlewalk("/pathlimit_idlewalk", 40);
	static ConsoleVariable<int> cvar_pathlimit_allyfollow("/pathlimit_allyfollow", 200);
	static ConsoleVariable<int> cvar_pathlimit_bosses("/pathlimit_bosses", 2000);
	static ConsoleVariable<int> cvar_pathlimit_commandmove("/pathlimit_commandmove", 1000);
	static ConsoleVariable<int> cvar_pathlimit_achievement("/pathlimit_achievement", 1600);
	if (pathingType == GeneratePathTypes::GENERATE_PATH_IDLE_WALK ||
        pathingType == GeneratePathTypes::GENERATE_PATH_MOVEASIDE ||
		pathingType == GeneratePathTypes::GENERATE_PATH_MONSTER_MOVE_BACKWARDS)
	{
		maxtries = *cvar_pathlimit_idlewalk;
	}
	else if (
        pathingType == GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW ||
		pathingType == GeneratePathTypes::GENERATE_PATH_ALLY_FOLLOW2)
	{
		maxtries = *cvar_pathlimit_allyfollow;
		if ( my->behavior == &actMonster )
		{
			if ( Stat* myStats = my->getStats() )
			{
				if ( monsterAllyFormations.getFollowerTryExtendedPathSearch(*my, *myStats) > 0 )
				{
					if ( *cvar_pathing_debug )
					{
						messagePlayer(0, MESSAGE_DEBUG, "Trying extended path range");
						maxtries = *cvar_pathlimit_commandmove;
					}
				}
			}
		}
	}
	else if (
        pathingType == GeneratePathTypes::GENERATE_PATH_PLAYER_ALLY_MOVETO ||
		pathingType == GeneratePathTypes::GENERATE_PATH_INTERACT_MOVE)
	{
		maxtries = *cvar_pathlimit_commandmove;
	}
	else if ( pathingType == GeneratePathTypes::GENERATE_PATH_ACHIEVEMENT )
	{
		maxtries = *cvar_pathlimit_achievement;
	}
	else if ( pathingType == GeneratePathTypes::GENERATE_PATH_BOSS_TRACKING_HUNT
		|| pathingType == GeneratePathTypes::GENERATE_PATH_BOSS_TRACKING_IDLE
		|| (pathingType == GeneratePathTypes::GENERATE_PATH_TO_HUNT_MONSTER_TARGET
			&& my && stats && my->isBossMonster()) )
	{
		maxtries = *cvar_pathlimit_bosses;
	}
	while (!openSet.empty()) {
        if ((tries >= maxtries && !playerCheckPathToExit && !loading) ||
			(tries >= 10000 && (playerCheckPathToExit || loading))) {
            // early abort, this path is taking too long!
            break;
        }
        
        const auto key = queue.top(); queue.pop();
		const auto find = openSet.find({key.x, key.y});
        assert(find != openSet.end());
        const auto pathnode = find->second;
        openSet.erase({key.x, key.y});
        closedSet.insert({{key.x, key.y}, pathnode});
        
		if (pathnode.x == x2 && pathnode.y == y2) {
			// found target, retrace path
			auto path = (list_t*) malloc(sizeof(list_t));
			path->first = nullptr;
			path->last = nullptr;
            auto find = closedSet.find(pairtype{pathnode.x, pathnode.y});
			while (find != closedSet.end()) {
                if (find->second.px == -1 || find->second.py == -1) {
                    // don't bother including the very first node on the path.
                    // the reason is, the starting tile isn't necessary to
                    // begin the path; and if an entity happens to be on
                    // the edge of its starting tile, it will actually
                    // double-back before going to the next one!
                    break;
                }
                auto pathnode = (pathnode_t*)malloc(sizeof(pathnode_t));
                *pathnode = find->second;
                auto node = list_AddNodeFirst(path);
                node->size = sizeof(pathnode_t);
                node->deconstructor = defaultDeconstructor;
                node->element = pathnode;
                find = closedSet.find(pairtype{pathnode->px, pathnode->py});
			}
			free(pathMap);

			if ( *cvar_pathing_debug ) {
				auto now = std::chrono::high_resolution_clock::now();
				ms = std::chrono::duration_cast<std::chrono::microseconds>(now - pathtime);
				DebugStats.gui2 = DebugStats.gui2 + ms;
				messagePlayer(0, MESSAGE_DEBUG, "PASS (%d): path tries: %d", (int)pathingType, tries);
			}
			lastGeneratePathTries = tries;
			if ( my->behavior == &actMonster ) {
				monsterAllyFormations.updateOnPathSucceed(my->getUID(), my);
			}
			return path;
		}

		// expand search
		for (int y = -1; y <= 1; y++) {
			for (int x = -1; x <= 1; x++) {
                const int newx = pathnode.x + x;
                const int newy = pathnode.y + y;
				if (x == 0 && y == 0) {
					continue;
				}
				int z = 0;
				if (!loading) {
					int index;
					index = newy + newx * map.height;
					index = std::min(std::max(0, index),
                        (int)map.width * (int)map.height - 1);
					if (!pathMap[index]) {
						z++;
					}
					if (x && y) {
						index = pathnode.y + newx * map.height;
						index = std::min(std::max(0, index),
                            (int)map.width * (int)map.height - 1);
						if (!pathMap[index]) {
							z++;
						}
						index = newy + x * map.height;
						index = std::min(std::max(0, index),
                            (int)map.width * (int)map.height - 1);
						if (!pathMap[index]) {
							z++;
						}
					}
				}
				else { // if (!loading)
					if (pathCheckObstacle((newx << 4) + 8, (newy << 4) + 8, my, target)) {
						z++;
					}
					if (x && y) {
						if (pathCheckObstacle((pathnode.x << 4) + 8, (newy << 4) + 8, my, target)) {
							z++;
						}
						if (pathCheckObstacle((newx << 4) + 8, (pathnode.y << 4) + 8, my, target)) {
							z++;
						}
					}
				}
				if (!z) {
                    const auto key = pairtype{newx, newy};
					bool alreadyadded = closedSet.find(key) != closedSet.end();
                    auto find = openSet.find(key);
					if (find != openSet.end()) {
                        alreadyadded = true;
						auto& childnode = find->second;
                        if (x && y) {
                            if (childnode.g > pathnode.g + DIAGONALCOST) {
                                childnode.px = pathnode.x;
                                childnode.py = pathnode.y;
                                childnode.g = pathnode.g + DIAGONALCOST;
                            }
                        } else {
                            if (childnode.g > pathnode.g + STRAIGHTCOST) {
                                childnode.px = pathnode.x;
                                childnode.py = pathnode.y;
                                childnode.g = pathnode.g + STRAIGHTCOST;
                            }
                        }
					}
					if (alreadyadded == false) {
						if (enableDebugKeys && *cvar_pathing_debug && keystatus[SDLK_g]) {
							Entity* particle = spawnMagicParticle(my);
							particle->sprite = 576;
							particle->x = (pathnode.x + x) * 16.0 + 8.0;
							particle->y = (pathnode.y + y) * 16.0 + 8.0;
							particle->z = 0;
							particle->scalex = 2.0;
							particle->scaley = 2.0;
							particle->scalez = 2.0;
						}
                        const auto newNode = pathnode_t{newx, newy,
                            (x && y) ? (pathnode.g + DIAGONALCOST) : (pathnode.g + STRAIGHTCOST),
                            heuristic(newx, newy, x2, y2), pathnode.x, pathnode.y};
                        openSet.insert({pairtype{newx, newy}, newNode});
                        queue.push({newx, newy, openSet});
					}
				}
			}
		}
		++tries;
	}
 
    // path failed
	free(pathMap);
	if ( *cvar_pathing_debug ) {
		auto now = std::chrono::high_resolution_clock::now();
		ms = std::chrono::duration_cast<std::chrono::microseconds>(now - pathtime);
		DebugStats.gui2 = DebugStats.gui2 + ms;
		messagePlayer(0, MESSAGE_DEBUG, "FAIL (%d) sprite: %d uid: %d : path tries: %d (%d, %d) to (%d, %d)",
            (int)pathingType, my->sprite, my->getUID(), tries, x1, y1, x2, y2);
	}
	lastGeneratePathTries = tries;
	if (my->behavior == &actMonster) {
		if (pathingType == GENERATE_PATH_ALLY_FOLLOW ||
            pathingType == GENERATE_PATH_ALLY_FOLLOW2) {
            monsterAllyFormations.updateOnPathFail(my->getUID(), my);
        }
	}
	return NULL;
}

/*-------------------------------------------------------------------------------

	generatePathMaps

	Maps out islands in the game map for the path generator

-------------------------------------------------------------------------------*/

void fillPathMap(int* pathMap, int x, int y, int zone);

void generatePathMaps()
{
	int x, y;

	if ( pathMapGrounded )
	{
		free(pathMapGrounded);
	}
	pathMapGrounded = (int*)calloc(map.width * map.height, sizeof(int));
	if ( pathMapFlying )
	{
		free(pathMapFlying);
	}
	pathMapFlying = (int*)calloc(map.width * map.height, sizeof(int));

	pathMapZone = 1;
	for ( y = 0; y < map.height; y++ )
	{
		for ( x = 0; x < map.width; x++ )
		{
			if ( !pathMapGrounded[y + x * map.height] )
			{
				fillPathMap(pathMapGrounded, x, y, pathMapZone);
			}
			if ( !pathMapFlying[y + x * map.height] )
			{
				fillPathMap(pathMapFlying, x, y, pathMapZone);
			}
		}
	}

	for ( int i = 0; i < GateGraph::GATE_GRAPH_NUM_PATHMAPS; ++i )
	{
		auto& graph = gateGraph[i];
		graph.reset();
		graph.buildGraph(i);
		graph.debugPaths();
	}
}

void fillPathMap(int* pathMap, int x, int y, int zone)
{
	bool obstacle = true;

	int index = y * MAPLAYERS + x * MAPLAYERS * map.height;
	if ( !map.tiles[OBSTACLELAYER + index] && map.tiles[index] 
		&& !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) )
	{
		obstacle = false;
	}
	else if ( pathMap == pathMapFlying && !map.tiles[OBSTACLELAYER + index] )
	{
		obstacle = false;
	}
	if ( obstacle == false )
	{
		node_t* node;
		list_t* list = checkTileForEntity(x, y);
		if ( list )
		{
			for ( node = list->first; node != NULL; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity )
				{
					if ( isPathObstacle(entity) )
					{
						obstacle = true;
						break;
					}
					/*else if ( entity->behavior == &actWallBuilder || entity->behavior == &actWallBuster )
					{
						obstacle = false;
						break;
					}*/
				}
			}
			//list_FreeAll(list);
			//free(list);
		}
	}

	if ( obstacle )
	{
		return;
	}

	pathMap[y + x * map.height] = zone;
	bool repeat;
	do
	{
		repeat = false;

		int u, v;
		for ( u = 0; u < map.width; u++ )
		{
			for ( v = 0; v < map.height; v++ )
			{
				if ( pathMap[v + u * map.height] == zone )
				{
					if ( u < map.width - 1 )
					{
						if ( !pathMap[v + (u + 1)*map.height] )
						{
							bool foundObstacle = false;
							bool foundWallModifier = false;
							list_t* list = checkTileForEntity(u + 1, v);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( isPathObstacle(entity) )
										{
											foundObstacle = true;
											break;
										}
										/*else if ( entity->behavior == &actWallBuilder || entity->behavior == &actWallBuster )
										{
											foundWallModifier = true;
											break;
										}*/
									}
								}
								if ( foundWallModifier )
								{
									pathMap[v + (u + 1)*map.height] = zone;
									repeat = true;
								}
								//list_FreeAll(list);
								//free(list);
							}
							if ( !foundWallModifier && !foundObstacle )
							{
								int index = v * MAPLAYERS + (u + 1) * MAPLAYERS * map.height;
								if ( !map.tiles[OBSTACLELAYER + index] && (pathMap == pathMapFlying 
									|| (map.tiles[index] && !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) )) )
								{
									pathMap[v + (u + 1)*map.height] = zone;
									repeat = true;
								}
							}
						}
					}
					if ( u > 0 )
					{
						if ( !pathMap[v + (u - 1)*map.height] )
						{
							bool foundObstacle = false;
							bool foundWallModifier = false;
							list_t* list = checkTileForEntity(u - 1, v);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( isPathObstacle(entity) )
										{
											foundObstacle = true;
											break;
										}
										/*else if ( entity->behavior == &actWallBuilder || entity->behavior == &actWallBuster )
										{
											foundWallModifier = true;
											break;
										}*/
									}
								}
								if ( foundWallModifier )
								{
									pathMap[v + (u - 1)*map.height] = zone;
									repeat = true;
								}
								//list_FreeAll(list);
								//free(list);
							}
							if ( !foundWallModifier && !foundObstacle )
							{
								int index = v * MAPLAYERS + (u - 1) * MAPLAYERS * map.height;
								if ( !map.tiles[OBSTACLELAYER + index] && (pathMap == pathMapFlying 
									|| (map.tiles[index] && !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]])) ) )
								{
									pathMap[v + (u - 1)*map.height] = zone;
									repeat = true;
								}
							}
						}
					}
					if ( v < map.height - 1 )
					{
						if ( !pathMap[(v + 1) + u * map.height] )
						{
							bool foundObstacle = false;
							bool foundWallModifier = false;
							list_t* list = checkTileForEntity(u, v + 1);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( isPathObstacle(entity) )
										{
											foundObstacle = true;
											break;
										}
										/*else if ( entity->behavior == &actWallBuilder || entity->behavior == &actWallBuster )
										{
											foundWallModifier = true;
											break;
										}*/
									}
								}
								if ( foundWallModifier )
								{
									pathMap[(v + 1) + u * map.height] = zone;
									repeat = true;
								}
								//list_FreeAll(list);
								//free(list);
							}
							if ( !foundWallModifier && !foundObstacle )
							{
								int index = (v + 1) * MAPLAYERS + u * MAPLAYERS * map.height;
								if ( !map.tiles[OBSTACLELAYER + index] && (pathMap == pathMapFlying 
									|| (map.tiles[index] && !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]])) ) )
								{
									pathMap[(v + 1) + u * map.height] = zone;
									repeat = true;
								}
							}
						}
					}
					if ( v > 0 )
					{
						if ( !pathMap[(v - 1) + u * map.height] )
						{
							bool foundObstacle = false;
							bool foundWallModifier = false;
							list_t* list = checkTileForEntity(u, v - 1);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( isPathObstacle(entity) )
										{
											foundObstacle = true;
											break;
										}
										/*else if ( entity->behavior == &actWallBuilder || entity->behavior == &actWallBuster )
										{
											foundWallModifier = true;
											break;
										}*/
									}
								}
								if ( foundWallModifier )
								{
									pathMap[(v - 1) + u * map.height] = zone;
									repeat = true;
								}
								//list_FreeAll(list);
								//free(list);
							}
							if ( !foundWallModifier && !foundObstacle )
							{
								int index = (v - 1) * MAPLAYERS + u * MAPLAYERS * map.height;
								if ( !map.tiles[OBSTACLELAYER + index] && (pathMap == pathMapFlying 
									|| (map.tiles[index] && !(swimmingtiles[map.tiles[index]] || lavatiles[map.tiles[index]]) )) )
								{
									pathMap[(v - 1) + u * map.height] = zone;
									repeat = true;
								}
							}
						}
					}
				}
			}
		}
	}
	while ( repeat );
	pathMapZone++;
}



bool isPathObstacle(Entity* entity)
{
	if ( entity->behavior == &actHeadstone )
	{
		return true;
	}
	else if(entity->behavior == &actSink )
	{
		return true;
	}
	else if ( entity->behavior == &actFountain )
	{
		return true;
	}
	else if ( entity->behavior == &actPowerCrystal || entity->behavior == &actPowerCrystalBase )
	{
		return true;
	}
	else if ( entity->behavior == &actPedestalBase || entity->behavior == &actPedestalOrb )
	{
		return true;
	}
	else if ( entity->behavior == &actStatue )
	{
		return true;
	}
	else if ( entity->behavior == &actTeleportShrine /*|| entity->behavior == &actSpellShrine*/ )
	{
		return true;
	}
	else if ( entity->behavior == &::actDaedalusShrine )
	{
		return true;
	}
	else if ( entity->behavior == &::actAssistShrine )
	{
		return true;
	}
	else if ( entity->behavior == &actStalagColumn
		|| entity->behavior == &actStalagFloor
		|| entity->behavior == &actColumn )
	{
		return true;
	}
	else if ( entity->behavior == &actPistonBase || entity->behavior == &actPistonCam )
	{
		return true;
	}
	else if ( entity->behavior == &actColliderDecoration && entity->colliderHasCollision != 0
		&& !entity->isDamageableCollider() )
	{
		return true;
	}
	else if ( entity->behavior == &actStalagCeiling )
	{
		if ( entity->z > -8 )
		{
			// not on ceiling layer
			return true;
		}
	}
	return false;
}


void GateGraph::buildGraph(const int parentMapType)
{
	mapSubzones = (int*)calloc(map.width * map.height, sizeof(int));

	int* parentMap = nullptr;
	this->parentMapType = parentMapType;
	if ( parentMapType == GateGraph::GATE_GRAPH_GROUNDED )
	{
		parentMap = pathMapGrounded;
	}
	else if ( parentMapType == GateGraph::GATE_GRAPH_FLYING )
	{
		parentMap = pathMapFlying;
	}
	else
	{
		printlog("[Gate Graph]: No matching type found: %d", parentMapType);
		return;
	}

	if ( !parentMap )
	{
		printlog("[Gate Graph]: Parent map was NULL: %d", parentMapType);
		return;
	}

	bIsInit = true;
	numSubzones = 1;
	for ( int y = 0; y < map.height; y++ )
	{
		for ( int x = 0; x < map.width; x++ )
		{
			int zone = parentMap[y + x * map.height];
			if ( zone > 0 && !mapSubzones[y + x * map.height] )
			{
				fillPathMap(x, y);
			}
		}
	}

	for ( node_t* entityNode = map.entities->first; entityNode != nullptr; entityNode = entityNode->next )
	{
		Entity* entity = (Entity*)entityNode->element;
		if ( entity->behavior == &actGate )
		{
			int ix = ((int)entity->x >> 4);
			int iy = ((int)entity->y >> 4);
			real_t angle = normaliseAngle2PI(entity->yaw);
			if ( limbAngleWithinRange(angle, .05, PI / 2) || limbAngleWithinRange(angle, .05, 3 * PI / 2) )
			{
				int zone1 = mapSubzones[(iy - 1) + ix * map.height];
				int zone2 = mapSubzones[(iy + 1) + ix * map.height];
				int& middleZone = mapSubzones[iy + ix * map.height];
				middleZone = zone1 + (zone2 * 10000);
				addGate(ix, iy, zone1, zone2, entity->getUID(), DIR_NORTHSOUTH);
				/*printlog("N/S Gate: %d, %d: subzones: %d | %d (i am %d)", 
					ix, iy, zone1, zone2, middleZone);*/
			}
			else
			{
				int zone1 = mapSubzones[iy + (ix - 1) * map.height];
				int zone2 = mapSubzones[iy + (ix + 1) * map.height];
				int& middleZone = mapSubzones[iy + ix * map.height];
				middleZone = zone1 + (zone2 * 10000);
				addGate(ix, iy,	zone1, zone2, entity->getUID(), DIR_EASTWEST);
				/*printlog("E/W Gate: %d, %d: subzones: %d | %d (i am %d)", 
					ix, iy, zone1, zone2, middleZone);*/
			}
		}
	}
	for ( auto& pair : gateNodes )
	{
		addEdge(pair.second.zone1, pair.second.zone2);
	}

	for ( int i = 0; i < numSubzones; ++i )
	{
		for ( int j = 0; j < numSubzones; ++j )
		{
			if ( i == j ) { continue; }
			if ( connectedZones[i].find(j) != connectedZones[i].end() )
			{
				continue;
			}
			if ( connectedZones[j].find(i) != connectedZones[j].end() )
			{
				continue;
			}
			if ( isConnected(i, j) )
			{
				connectedZones[i].insert(j);
				connectedZones[j].insert(i);
			}
		}
	}

	printlog("[Gate Graph]: Map %d Built successfully.", parentMapType);
}

void GateGraph::fillPathMap(int x, int y)
{
	int* parentMap = nullptr;
	if ( parentMapType == GateGraph::GATE_GRAPH_GROUNDED )
	{
		parentMap = pathMapGrounded;
	}
	else if ( parentMapType == GateGraph::GATE_GRAPH_FLYING )
	{
		parentMap = pathMapFlying;
	}

	node_t* node;
	list_t* list = checkTileForEntity(x, y);
	if ( list )
	{
		for ( node = list->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity )
			{
				if ( entity->behavior == &actGate )
				{
					return;
				}
			}
		}
	}

	mapSubzones[y + x * map.height] = numSubzones;

	bool repeat;
	do
	{
		repeat = false;

		int u, v;
		for ( u = 0; u < map.width; u++ )
		{
			for ( v = 0; v < map.height; v++ )
			{
				if ( mapSubzones[v + u * map.height] == numSubzones )
				{
					if ( u < map.width - 1 )
					{
						if ( !mapSubzones[v + (u + 1)*map.height] )
						{
							bool foundObstacle = false;
							list_t* list = checkTileForEntity(u + 1, v);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( entity->behavior == &actGate )
										{
											foundObstacle = true;
											break;
										}
									}
								}
							}
							if ( !foundObstacle )
							{
								if ( parentMap[v + (u + 1)*map.height] )
								{
									mapSubzones[v + (u + 1)*map.height] = numSubzones;
									repeat = true;
								}
							}
						}
					}
					if ( u > 0 )
					{
						if ( !mapSubzones[v + (u - 1)*map.height] )
						{
							bool foundObstacle = false;
							list_t* list = checkTileForEntity(u - 1, v);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( entity->behavior == &actGate )
										{
											foundObstacle = true;
											break;
										}
									}
								}
							}
							if ( !foundObstacle )
							{
								if ( parentMap[v + (u - 1)*map.height] )
								{
									mapSubzones[v + (u - 1)*map.height] = numSubzones;
									repeat = true;
								}
							}
						}
					}
					if ( v < map.height - 1 )
					{
						if ( !mapSubzones[(v + 1) + u * map.height] )
						{
							bool foundObstacle = false;
							list_t* list = checkTileForEntity(u, v + 1);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( entity->behavior == &actGate )
										{
											foundObstacle = true;
											break;
										}
									}
								}
							}
							if ( !foundObstacle )
							{
								if ( parentMap[(v + 1) + u*map.height] )
								{
									mapSubzones[(v + 1) + u * map.height] = numSubzones;
									repeat = true;
								}
							}
						}
					}
					if ( v > 0 )
					{
						if ( !mapSubzones[(v - 1) + u * map.height] )
						{
							bool foundObstacle = false;
							list_t* list = checkTileForEntity(u, v - 1);
							if ( list )
							{
								node_t* node;
								for ( node = list->first; node != NULL; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( entity )
									{
										if ( entity->behavior == &actGate )
										{
											foundObstacle = true;
											break;
										}
									}
								}
							}
							if ( !foundObstacle )
							{
								if ( parentMap[(v - 1) + u*map.height] )
								{
									mapSubzones[(v - 1) + u * map.height] = numSubzones;
									repeat = true;
								}
							}
						}
					}
				}
			}
		}
	} while ( repeat );
	++numSubzones;
}

void GateGraph::debugPaths()
{
	return;
	for ( int i = 0; i < numSubzones; ++i )
	{
		if ( connectedZones.find(i) == connectedZones.end() )
		{
			printlog("[Gate Graph]: Zone %d is isolated.", i);
		}
		else
		{
			for ( int j = 0; j < numSubzones; ++j )
			{
				if ( i == j ) { continue; }
				auto pathMap = getPath(i, j);

				if ( pathMap.find(j) == pathMap.end() )
				{
					printlog("[Gate Graph]: No path from %d to %d", i, j);
				}
				else
				{
					std::vector<int> path;
					int current = j;
					std::string pathStr = "[Gate Graph]: Path from ";
					pathStr += std::to_string(i);
					pathStr += " to ";
					pathStr += std::to_string(j);
					pathStr += ":    ";

					while ( current != i )
					{
						path.push_back(current);
						current = pathMap[current];
					}
					path.push_back(i);
					for ( auto p : path )
					{
						pathStr += std::to_string(p);
						pathStr += ",";
					}
					printlog(pathStr.c_str());
				}
			}
		}
	}
}

bool GateGraph::generatePath(Entity* my, int x1, int y1, int x2, int y2)
{
	int srcZone = mapSubzones[y1 + x1 * map.height];
	int destZone = mapSubzones[y2 + x2 * map.height];
	if ( srcZone < numSubzones && destZone < numSubzones )
	{
		if ( srcZone == destZone ) { return true; }

		if ( !isConnected(srcZone, destZone) )
		{
			return false;
		}

		auto path = getPath(srcZone, destZone);
		if ( path.find(destZone) != path.end() )
		{
			if ( *cvar_pathing_debug )
			{
				messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: Path exists from (%d, %d) to (%d, %d)", x1, y1, x2, y2);
			}
			return true;
		}
		if ( *cvar_pathing_debug )
		{
			messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: No path from (%d, %d) to (%d, %d)", x1, y1, x2, y2);
		}
		return false;
	}
	else
	{
		// on a gate edge
		std::vector<int> srcZonesToTest;
		std::vector<int> destZonesToTest;
		if ( srcZone >= numSubzones )
		{
			GateNode_t& srcGate = getGate(x1, y1);
			if ( srcGate.zone1 != -1 )
			{
				srcZonesToTest.push_back(std::min(srcGate.zone1, srcGate.zone2));
				srcZonesToTest.push_back(std::max(srcGate.zone1, srcGate.zone2));
			}
		}
		else
		{
			srcZonesToTest.push_back(srcZone);
		}
		if ( destZone >= numSubzones )
		{
			GateNode_t& destGate = getGate(x2, y2);
			if ( destGate.zone1 != -1 )
			{
				destZonesToTest.push_back(std::min(destGate.zone1, destGate.zone2));
				destZonesToTest.push_back(std::max(destGate.zone1, destGate.zone2));
			}
		}
		else
		{
			destZonesToTest.push_back(destZone);
		}

		std::unordered_set<int> visited;
		std::unordered_set<int> uniqueZones;

		enum OnGateDir
		{
			NONE = -1,
			EAST,
			SOUTH,
			WEST,
			NORTH
		};
		OnGateDir standingOnGateDirection = NONE;
		Entity* standingOnGateEntity = nullptr;
		auto& gate = getGate(static_cast<int>(my->x / 16), static_cast<int>(my->y / 16));
		if ( gate.zone1 != -1 && gate.x == x1 && gate.y == y1 )
		{
			// we're standing on a gate, matching the start coordinates of path
			if ( standingOnGateEntity = uidToEntity(gate.uid) ) 
			{
				if ( !standingOnGateEntity->flags[PASSABLE] && !entityInsideEntity(my, standingOnGateEntity) )
				{
					if ( gate.direction == DIR_NORTHSOUTH )
					{
						if ( my->y > standingOnGateEntity->y )
						{
							standingOnGateDirection = SOUTH;
							//messagePlayer(0, MESSAGE_DEBUG, "SOUTH");
						}
						else if ( my->y < standingOnGateEntity->y )
						{
							standingOnGateDirection = NORTH;
							//messagePlayer(0, MESSAGE_DEBUG, "NORTH");
						}
					}
					else if ( gate.direction == DIR_EASTWEST )
					{
						if ( my->x > standingOnGateEntity->x )
						{
							standingOnGateDirection = EAST;
							//messagePlayer(0, MESSAGE_DEBUG, "EAST");
						}
						else if ( my->x < standingOnGateEntity->x )
						{
							standingOnGateDirection = WEST;
							//messagePlayer(0, MESSAGE_DEBUG, "WEST");
						}
					}
				}
				else
				{
					standingOnGateEntity = nullptr;
				}
			}
		}

		for ( auto x : srcZonesToTest )
		{
			for ( auto y : destZonesToTest )
			{
				// check the gate
				if ( x == y ) 
				{ 
					if ( standingOnGateEntity )
					{
						if ( standingOnGateDirection == SOUTH || standingOnGateDirection == EAST )
						{
							if ( x == gate.zone2 )
							{
								if ( *cvar_pathing_debug )
								{
									messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: Path exists from (%d, %d) to (%d, %d) (standing on gate in direction: %d", x1, y1, x2, y2, standingOnGateDirection);
								}
								return true;
							}
						}
						else if ( standingOnGateDirection == NORTH || standingOnGateDirection == WEST )
						{
							if ( x == gate.zone1 )
							{
								if ( *cvar_pathing_debug )
								{
									messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: Path exists from (%d, %d) to (%d, %d) (standing on gate in direction: %d", x1, y1, x2, y2, standingOnGateDirection);
								}
								return true;
							}
						}
					}
					uniqueZones.insert(x);
					continue; 
				} 
				int x3 = std::min(x, y);
				int y3 = std::max(x, y);
				if ( visited.find(x3 + y3 * 10000) == visited.end() )
				{
					if ( isConnected(x3, y3) )
					{
						auto path = getPath(x3, y3);
						if ( path.find(y3) != path.end() )
						{
							if ( *cvar_pathing_debug )
							{
								messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: Path exists from (%d, %d) to (%d, %d)", x1, y1, x2, y2);
							}
							return true;
						}
					}
					visited.insert(x3 + y3 * 10000);
					uniqueZones.insert(x);
					uniqueZones.insert(y);
				}
			}
		}

		if ( uniqueZones.size() == 1 )
		{
			if ( *cvar_pathing_debug )
			{
				messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: Path exists from (%d, %d) to (%d, %d), no unique zones", x1, y1, x2, y2);
			}
			return true;
		}
	}

	if ( *cvar_pathing_debug )
	{
		messagePlayer(0, MESSAGE_DEBUG, "[Gate Graph]: No path found (%d-%d) from (%d, %d) to (%d, %d)", srcZone, destZone, x1, y1, x2, y2);
	}
	return false;
}
