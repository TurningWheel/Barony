/*-------------------------------------------------------------------------------

	BARONY
	File: actgeneral.cpp
	Desc: very small and general behavior functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "engine/audio/sound.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "menu.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "scores.hpp"
#include "mod_tools.hpp"
#include "paths.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following functions describe various entity behaviors. All functions
	take a pointer to the entities that use them as an argument.

-------------------------------------------------------------------------------*/

void actAnimator(Entity* my)
{
	if ( my->skill[4] == 0 )
	{
		my->skill[4] = 1;
		map.tiles[my->skill[0] + (int)my->y * MAPLAYERS + (int)my->x * MAPLAYERS * map.height] -= my->skill[1] - 1;
	}

	if ( (int)floor(my->x) < 0 || (int)floor(my->x) >= map.width || (int)floor(my->y) < 0 || (int)floor(my->y) >= map.height )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	my->skill[3]++;
	if ( my->skill[3] >= 10 )
	{
		my->skill[3] = 0;
		map.tiles[my->skill[0] + (int)floor(my->y)*MAPLAYERS + (int)floor(my->x)*MAPLAYERS * map.height]++;
		my->skill[5]++;
		if (my->skill[5] == my->skill[1])
		{
			my->skill[5] = 0;
			map.tiles[my->skill[0] + (int)floor(my->y)*MAPLAYERS + (int)floor(my->x)*MAPLAYERS * map.height] -= my->skill[1];
		}
	}
}

#define TESTSPRITES

void actRotate(Entity* my)
{
	my->yaw += 0.1;
	my->flags[PASSABLE] = true; // this entity should always be passable

#ifdef TESTSPRITES
	if ( keystatus[SDLK_HOME] )
	{
		keystatus[SDLK_HOME] = 0;
		my->sprite++;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, MESSAGE_MISC, "test sprite: %d", my->sprite);
	}
	if ( keystatus[SDLK_END] )
	{
		keystatus[SDLK_END] = 0;
		my->sprite += 10;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, MESSAGE_MISC, "test sprite: %d", my->sprite);
	}
#endif
}

#define LIQUID_TIMER my->skill[0]
#define LIQUID_INIT my->skill[1]
#define LIQUID_LAVA my->flags[USERFLAG1]
#define LIQUID_LAVANOBUBBLE my->skill[4]

void actLiquid(Entity* my)
{
	// as of 1.0.7 this function is DEPRECATED

	list_RemoveNode(my->mynode);
	return;
}

void actEmpty(Entity* my)
{
	// an empty action
	// used on clients to permit dead reckoning and other interpolation
}

void actFurniture(Entity* my)
{
	if ( !my )
	{
		return;
	}

	if ( !my->flags[BURNABLE] )
	{
		my->flags[BURNABLE] = true;
	}

	my->actFurniture();
}

void Entity::actFurniture()
{

	if ( !furnitureInit )
	{
		auto& rng = entity_rng ? *entity_rng : local_rng;
		if ( furnitureType == FURNITURE_BUNKBED )
		{
			this->createWorldUITooltip();
		}
		furnitureInit = 1;
		if ( furnitureType == FURNITURE_TABLE || furnitureType == FURNITURE_BUNKBED || furnitureType == FURNITURE_BED || furnitureType == FURNITURE_PODIUM )
		{
			furnitureHealth = 15 + rng.rand() % 5;
		}
		else
		{
			furnitureHealth = 4 + rng.rand() % 4;
		}
		furnitureMaxHealth = furnitureHealth;
		furnitureOldHealth = furnitureHealth;
		flags[BURNABLE] = true;
	}
	else
	{
		if ( multiplayer != CLIENT )
		{
			// burning
			if ( flags[BURNING] )
			{
				if ( ticks % 15 == 0 )
				{
					furnitureHealth--;
				}
			}

			furnitureOldHealth = furnitureHealth;

			// furniture mortality :p
			if ( furnitureHealth <= 0 )
			{
				int c;
				for ( c = 0; c < 5; c++ )
				{
					Entity* entity = spawnGib(this);
					entity->flags[INVISIBLE] = false;
					entity->sprite = 187; // Splinter.vox
					entity->x = floor(x / 16) * 16 + 8;
					entity->y = floor(y / 16) * 16 + 8;
					entity->y += -3 + local_rng.rand() % 6;
					entity->x += -3 + local_rng.rand() % 6;
					entity->z = -5 + local_rng.rand() % 10;
					entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = (local_rng.rand() % 10 - 5) / 10.0;
					entity->vel_y = (local_rng.rand() % 10 - 5) / 10.0;
					entity->vel_z = -.5;
					entity->fskill[3] = 0.04;
					serverSpawnGibForClient(entity);
				}
				playSoundEntity(this, 176, 128);
				Entity* entity = uidToEntity(parent);
				if ( entity != NULL )
				{
					entity->itemNotMoving = 0; // drop the item that was on the table
					entity->itemNotMovingClient = 0; // clear the client item gravity flag
					serverUpdateEntitySkill(entity, 18); //update both the above flags.
					serverUpdateEntitySkill(entity, 19);
				}
				list_RemoveNode(mynode);
				return;
			}

			// using
			int i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( selectedEntity[i] == this || client_selected[i] == this )
				{
					if (inrange[i])
					{
						switch ( furnitureType )
						{
							case FURNITURE_CHAIR:
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(476));
								break;
							case FURNITURE_TABLE:
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(477));
								break;
							case FURNITURE_BED:
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(2493));
								break;
							case FURNITURE_BUNKBED:
								if ( i == 0 || i == 2 )
								{
									messagePlayer(i, MESSAGE_INTERACTION, Language::get(2494));
								}
								else
								{
									messagePlayer(i, MESSAGE_INTERACTION, Language::get(2495));
								}
								break;
							case FURNITURE_PODIUM:
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(2496));
								break;
							default:
								messagePlayer(i, MESSAGE_INTERACTION, Language::get(477));
								break;
						}
					}
				}
			}
		}
	}
}

// an easter egg
#define MCAXE_USED my->skill[0]

void actMCaxe(Entity* my)
{
	my->yaw += .05;
	if ( my->yaw > PI * 2 )
	{
		my->yaw -= PI * 2;
	}

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( !MCAXE_USED )
	{
		if ( multiplayer != CLIENT )
		{
			// use
			int i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( selectedEntity[i] == my || client_selected[i] == my )
				{
					if (inrange[i])
					{
						messagePlayer(i, MESSAGE_INTERACTION, Language::get(478 + local_rng.rand() % 5));
						MCAXE_USED = 1;
						serverUpdateEntitySkill(my, 0);
					}
				}
			}
		}

		// bob
		my->z -= sin(my->fskill[0] * PI / 180.f);
		my->fskill[0] += 6;
		if ( my->fskill[0] >= 360 )
		{
			my->fskill[0] -= 360;
		}
		my->z += sin(my->fskill[0] * PI / 180.f);
	}
	else
	{
		my->z += 1;
		if ( my->z > 64 )
		{
			list_RemoveNode(my->mynode);
			return;
		}
	}
}

void actStalagFloor(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] 
		&& (my->sprite == 581 || my->sprite == 582) ) // stop the compiler optimising into a different entity.
	{
		my->flags[BLOCKSIGHT] = false;
	}
	my->actStalagFloor();
}

void Entity::actStalagFloor()
{

}

void actStalagCeiling(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] 
		&& (my->sprite == 583 || my->sprite == 584) ) // stop the compiler optimising into a different entity.
	{
		my->flags[BLOCKSIGHT] = false;
	}
	my->actStalagCeiling();
}

void Entity::actStalagCeiling()
{

}

void actStalagColumn(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] && my->sprite == 580 ) // stop the compiler optimising into a different entity.
	{
		my->flags[BLOCKSIGHT] = false;
	}
	my->actStalagColumn();
}

void Entity::actStalagColumn()
{

}

void actStatue(Entity* my)
{
	if ( my->statueInit == 0 && StatueManager.allStatues.size() > 0 )
	{
		// needs to init.
		if ( StatueManager.allStatues.find(my->statueId) != StatueManager.allStatues.end() )
		{
			my->statueInit = 1;
			if ( my->statueDir >= 0 && my->statueDir < StatueManager.directionKeys.size() )
			{
				int index = 0;
				real_t baseHeight = 0.0;
				std::string directionString = StatueManager.directionKeys[my->statueDir];
				for ( auto& limb : StatueManager.allStatues[my->statueId].limbs[directionString] )
				{
					Entity* childEntity = newEntity(limb.sprite, 1, map.entities, nullptr);
					childEntity->parent = my->getUID();
					childEntity->x = my->x - limb.x;
					childEntity->y = my->y - limb.y;
					childEntity->z = limb.z + StatueManager.allStatues[my->statueId].heightOffset;
					childEntity->focalx = limb.focalx;
					childEntity->focaly = limb.focaly;
					childEntity->focalz = limb.focalz;
					childEntity->pitch = limb.pitch;
					childEntity->roll = limb.roll;
					childEntity->yaw = limb.yaw;
					childEntity->flags[PASSABLE] = true;
					childEntity->grayscaleGLRender = 1.0;
					node_t* tempNode = list_AddNodeLast(&my->children);
					tempNode->element = childEntity; // add the node to the children list.
					tempNode->deconstructor = &emptyDeconstructor;
					tempNode->size = sizeof(Entity*);
					++index;
				}
			}
		}
	}
}

void actStatueAnimator(Entity* my)
{
	if ( !my )
	{
		return;
	}

	if ( StatueManager.processStatueExport() == 1 ) // in progress
	{
		if ( Entity* player = uidToEntity(StatueManager.editingPlayerUid) )
		{
			player->yaw += PI / 2;
			while ( player->yaw >= 2 * PI )
			{
				player->yaw -= 2 * PI;
			}
		}
	}

	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] )
			{
				if ( !StatueManager.activeEditing )
				{
					StatueManager.statueEditorHeightOffset = 0.0;
				}
				StatueManager.activeEditing = !StatueManager.activeEditing;
				messagePlayer(0, MESSAGE_MISC, "Statue editing mode: %d", StatueManager.activeEditing);

				my->skill[0] = StatueManager.activeEditing ? 1 : 0;
			}
		}
	}

	if ( StatueManager.activeEditing )
	{
		if ( !players[1]->entity )
		{
			client_disconnected[1] = false;
			Entity* entity = newEntity(0, 1, map.entities, nullptr);
			entity->behavior = &actPlayer;
			entity->addToCreatureList(map.creatures);

			entity->x = my->x;
			entity->y = my->y;
			entity->z = my->z;

			entity->z -= 15 + StatueManager.statueEditorHeightOffset;
			entity->focalx = limbs[HUMAN][0][0]; // 0
			entity->focaly = limbs[HUMAN][0][1]; // 0
			entity->focalz = limbs[HUMAN][0][2]; // -1.5
			entity->sprite = 113; // head model
			entity->sizex = 4;
			entity->sizey = 4;
			entity->flags[GENIUS] = true;
			entity->flags[BLOCKSIGHT] = true;
			entity->flags[PASSABLE] = true;
			entity->skill[2] = 1; // skill[2] == PLAYER_NUM
			players[1]->entity = entity;
			StatueManager.editingPlayerUid = entity->getUID();
		}
		else
		{
			players[1]->entity->x = my->x;
			players[1]->entity->y = my->y;
			players[1]->entity->z = my->z;
			players[1]->entity->z -= 15 + StatueManager.statueEditorHeightOffset;
		}
	}
}

void actColumn(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] && my->sprite == 629 ) // stop the compiler optimising into a different entity.
	{
		my->flags[BLOCKSIGHT] = false;
	}
	my->actColumn();
}

void Entity::actColumn()
{

}

void actCeilingTile(Entity* my)
{
	if ( !my )
	{
		return;
	}
	if ( !my->flags[PASSABLE] )
	{
		my->flags[PASSABLE] = true;
	}
	if ( my->flags[BLOCKSIGHT] )
	{
		my->flags[BLOCKSIGHT] = false;
	}
}

void actPistonBase(Entity* my)
{
	if ( !my )
	{
		return;
	}
	if ( my->flags[BLOCKSIGHT] && my->sprite == 631 ) // stop the compiler optimising into a different entity.
	{
		my->flags[BLOCKSIGHT] = false;
	}
}

void actPistonCam(Entity* my)
{
	if ( !my )
	{
		return;
	}
	my->actPistonCam();
}

void Entity::actPistonCam()
{
	yaw += pistonCamRotateSpeed;
	while ( yaw > 2 * PI )
	{
		yaw -= 2 * PI;
	}
	while ( yaw < 0 )
	{
		yaw += 2 * PI;
	}
	if ( (pistonCamDir == 0 || pistonCamDir == 2) && pistonCamRotateSpeed > 0 )
	{
		if ( yaw <= PI && yaw >= -pistonCamRotateSpeed + PI )
		{
			yaw = PI;
			pistonCamRotateSpeed = 0;
		}
	}
	--pistonCamTimer;

	if ( pistonCamDir == 0 ) // bottom
	{
		if ( pistonCamTimer <= 0 )
		{
			pistonCamDir = 1; // up
			pistonCamRotateSpeed = 0.2;
			pistonCamTimer = local_rng.rand() % 5 * TICKS_PER_SECOND;
		}
	}
	if ( pistonCamDir == 1 ) // up
	{
		z -= 0.1;
		if ( z < -1.75 )
		{
			z = -1.75;
			pistonCamRotateSpeed *= local_rng.rand() % 2 == 0 ? -1 : 1;
			pistonCamDir = 2; // top
		}
	}
	else if ( pistonCamDir == 2 ) // top
	{
		if ( pistonCamTimer <= 0 )
		{
			pistonCamDir = 3; // down
			pistonCamRotateSpeed = -0.2;
			pistonCamTimer = local_rng.rand() % 5 * TICKS_PER_SECOND;
		}
	}
	else if ( pistonCamDir == 3 ) // down
	{
		z += 0.1;
		if ( z > 1.75 )
		{
			z = 1.75;
			pistonCamRotateSpeed *= local_rng.rand() % 2 == 0 ? -1 : 1;
			pistonCamDir = 0; // down
		}
	}
}

bool Entity::isColliderShownAsWallOnMinimap() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.showAsWallOnMinimap;
}

bool Entity::isColliderWeakToBoulders() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.boulderDestroys;
}

bool Entity::isColliderWeakToSkill(const int proficiency) const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.proficiencyBonusDamage.find(proficiency) != colliderDmgType.proficiencyBonusDamage.end();
}

bool Entity::isColliderResistToSkill(const int proficiency) const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.proficiencyResistDamage.find(proficiency) != colliderDmgType.proficiencyResistDamage.end();
}

bool Entity::isColliderDamageableByMelee() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.meleeAffects;
}

bool Entity::isColliderDamageableByMagic() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.magicAffects;
}

bool Entity::isColliderAttachableToBombs() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
	return colliderDmgType.bombsAttach;
}

bool Entity::isDamageableCollider() const 
{ 
	return behavior == &actColliderDecoration && colliderMaxHP > 0;
}

bool Entity::isColliderWall() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	if ( colliderData.hpbarLookupName.find("_wall") != std::string::npos )
	{
		return true;
	}
	return false;
}

bool Entity::isColliderBreakableContainer() const
{
	if ( !isDamageableCollider() ) { return false; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	if ( colliderData.damageCalculationType.find("breakable") != std::string::npos )
	{
		return true;
	}
	return false;
}

void Entity::colliderOnDestroy()
{
	if ( multiplayer == CLIENT ) { return; }
	flags[PASSABLE] = true;

	Entity* killer = nullptr;
	if ( colliderKillerUid != 0 )
	{
		killer = uidToEntity(colliderKillerUid);
		if ( killer )
		{
			if ( isColliderBreakableContainer() )
			{
				Compendium_t::Events_t::eventUpdateWorld(killer->skill[2], Compendium_t::CPDM_CONTAINER_BROKEN, "containers", 1);
			}
		}
	}

	if ( colliderHideMonster != 0 )
	{
		int type = colliderHideMonster % 1000;
		int numSpawns = type == BAT_SMALL ? 2 : 1;
		int successes = 0;
		for ( int i = 0; i < numSpawns; ++i )
		{
			auto monster = summonMonster((Monster)type, ((int)(x / 16)) * 16 + 8, ((int)(y / 16)) * 16 + 8);
			if ( monster )
			{
				monster->yaw = yaw;
				monster->lookAtEntity(*monster);
				monster->monsterLookDir = yaw;
				if ( Stat* stats = monster->getStats() )
				{
					stats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
					if ( stats->type == GHOUL && currentlevel >= 15 )
					{
						strcpy(stats->name, "enslaved ghoul");
						stats->setAttribute("special_npc", "enslaved ghoul");
					}
					if ( stats->type == AUTOMATON )
					{
						monster->monsterStoreType = 1; // damaged
					}
					++successes;
				}
				//monster->attack(monster->getAttackPose(), 0, nullptr);
			}
		}

		if ( killer )
		{
			if ( killer->behavior == &actPlayer )
			{
				if ( successes >= 1 )
				{
					Compendium_t::Events_t::eventUpdateWorld(killer->skill[2], Compendium_t::CPDM_CONTAINER_MONSTERS, "containers", successes);
				}
				if ( successes == 1 )
				{
					messagePlayer(killer->skill[2], MESSAGE_INTERACTION, Language::get(6234),
						getMonsterLocalizedName((Monster)type).c_str(), Language::get(getColliderLangName()));
				}
				else if ( successes > 1 )
				{
					messagePlayer(killer->skill[2], MESSAGE_INTERACTION, Language::get(6253),
						getMonsterLocalizedPlural((Monster)type).c_str(), Language::get(getColliderLangName()));
				}
			}
		}
	}
	if ( colliderContainedEntity != 0 )
	{
		if ( auto entity = uidToEntity(colliderContainedEntity) )
		{
			if ( entity->behavior == &actItem || entity->behavior == &actGoldBag )
			{
				if ( entity->flags[INVISIBLE] )
				{
					if ( entity->behavior == &actGoldBag )
					{
						entity->vel_x = (0.25 + .025 * (local_rng.rand() % 11)) * cos(entity->yaw);
						entity->vel_y = (0.25 + .025 * (local_rng.rand() % 11)) * sin(entity->yaw);
						entity->vel_z = (-40 - local_rng.rand() % 10) * .01;
						entity->goldBouncing = 0;
						entity->z = 0.0 - (local_rng.rand() % 3);
						entity->flags[INVISIBLE] = false;

						if ( multiplayer == SERVER )
						{
							for ( int i = 1; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									strcpy((char*)net_packet->data, "BREK");
									SDLNet_Write32(static_cast<Uint32>(entity->getUID()), &net_packet->data[4]);
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
						}

						int totalGold = entity->goldAmount;

						// find other matching gold piles
						auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(entity, 2);
						for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
						{
							list_t* currentList = *it;
							node_t* node;
							for ( node = currentList->first; node != nullptr; node = node->next )
							{
								Entity* ent = (Entity*)node->element;
								if ( ent && ent->behavior == &actGoldBag && ent != entity && ent->goldInContainer != 0
									&& ent->goldInContainer == entity->goldInContainer )
								{
									ent->vel_x = (0.25 + .025 * (local_rng.rand() % 11)) * cos(ent->yaw);
									ent->vel_y = (0.25 + .025 * (local_rng.rand() % 11)) * sin(ent->yaw);
									ent->vel_z = (-40 - local_rng.rand() % 10) * .01;
									ent->goldBouncing = 0;
									ent->goldInContainer = 0;
									ent->z = 0.0 - (local_rng.rand() % 3);
									ent->flags[INVISIBLE] = false;

									if ( multiplayer == SERVER )
									{
										for ( int i = 1; i < MAXPLAYERS; ++i )
										{
											if ( !client_disconnected[i] )
											{
												strcpy((char*)net_packet->data, "BREK");
												SDLNet_Write32(static_cast<Uint32>(ent->getUID()), &net_packet->data[4]);
												net_packet->address.host = net_clients[i - 1].host;
												net_packet->address.port = net_clients[i - 1].port;
												net_packet->len = 8;
												sendPacketSafe(net_sock, -1, net_packet, i - 1);
											}
										}
									}

									totalGold += ent->goldAmount;
								}
							}
						}
						if ( totalGold > 0 && killer )
						{
							if ( killer->behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateWorld(killer->skill[2], Compendium_t::CPDM_CONTAINER_GOLD, "containers", totalGold);
							}
						}
						entity->goldInContainer = 0;
					}
					else if ( entity->behavior == &actItem )
					{
						//entity->flags[UPDATENEEDED] = true;
						entity->vel_x = (0.25 + .025 * (local_rng.rand() % 11)) * cos(entity->yaw);
						entity->vel_y = (0.25 + .025 * (local_rng.rand() % 11)) * sin(entity->yaw);
						entity->vel_z = (-40 - local_rng.rand() % 5) * .01;
						entity->itemContainer = 0;
						entity->z = 0.0;
						entity->itemNotMoving = 0;
						entity->itemNotMovingClient = 0;
						entity->flags[USERFLAG1] = false; // enable collision

						if ( multiplayer == SERVER )
						{
							for ( int i = 1; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									strcpy((char*)net_packet->data, "BREK");
									SDLNet_Write32(static_cast<Uint32>(entity->getUID()), &net_packet->data[4]);
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
						}

						if ( killer )
						{
							if ( killer->behavior == &actPlayer )
							{
								Compendium_t::Events_t::eventUpdateWorld(killer->skill[2], Compendium_t::CPDM_CONTAINER_ITEMS, "containers", 1);
							}
						}
					}
				}
			}
		}
	}
}

int Entity::getColliderLangName() const
{
	if ( !isDamageableCollider() ) { return 1; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	return colliderData.entityLangEntry;
}

int Entity::getColliderOnHitLangEntry() const
{
	if ( !isDamageableCollider() ) { return 1; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	return colliderData.hitMessageLangEntry;
}

int Entity::getColliderOnBreakLangEntry() const
{
	if ( !isDamageableCollider() ) { return 1; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	return colliderData.breakMessageLangEntry;
}

int Entity::getColliderSfxOnHit() const
{
	if ( !isDamageableCollider() ) { return 0; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	return colliderData.sfxHit;
}

int Entity::getColliderSfxOnBreak() const
{
	if ( !isDamageableCollider() ) { return 0; }
	auto& colliderData = EditorEntityData_t::colliderData[colliderDamageTypes];
	if ( colliderData.sfxBreak.size() == 0 ) { return 0; }
	return colliderData.sfxBreak[local_rng.rand() % colliderData.sfxBreak.size()];
}

void actColliderDecoration(Entity* my)
{
	if ( !my )
	{
		return;
	}

	if ( !my->colliderInit )
	{
		my->colliderInit = 1;
		if ( my->colliderDiggable != 0 )
		{
			my->colliderHasCollision = 1;
		}
		if ( my->isDamageableCollider() )
		{
			auto& colliderData = EditorEntityData_t::colliderData[my->colliderDamageTypes];
			auto& colliderDmgType = EditorEntityData_t::colliderDmgTypes[colliderData.damageCalculationType];
			if ( colliderDmgType.burnable )
			{
				my->flags[BURNABLE] = true;
			}
			if ( colliderDmgType.minotaurPathThroughAndBreak )
			{
				my->colliderHasCollision |= EditorEntityData_t::COLLIDER_COLLISION_FLAG_MINO;
			}
			if ( colliderDmgType.allowNPCPathing )
			{
				my->colliderHasCollision |= EditorEntityData_t::COLLIDER_COLLISION_FLAG_NPC;
			}
		}
	}

	my->flags[PASSABLE] = (my->colliderHasCollision == 0);
	if ( multiplayer != CLIENT )
	{
		bool checkWallDeletion = false;
		if ( my->colliderHasCollision != 0 )
		{
			if ( my->sprite == 1203 || my->sprite == 1204 )
			{
				if ( my->z > -8.51 && my->z < -8.49 )
				{
					checkWallDeletion = true;
				}
			}
			else if ( my->sprite == 1197 || my->sprite == 1198 )
			{
				if ( my->z > 7.49 || my->z < 7.51 )
				{
					checkWallDeletion = true;
				}
			}
		}
		if ( checkWallDeletion )
		{
			int x = static_cast<int>(my->x) >> 4;
			int y = static_cast<int>(my->y) >> 4;
			if ( !map.tiles[OBSTACLELAYER + y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				//messagePlayer(0, MESSAGE_DEBUG, "[Collider]: Destroyed self at x: %d, y: %d", x, y);
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}



	if ( my->isDamageableCollider() )
	{
		if ( my->ticks == 1 )
		{
			my->createWorldUITooltip();
		}

		if ( multiplayer != CLIENT )
		{
			auto& colliderData = EditorEntityData_t::colliderData[my->colliderDamageTypes];
			if ( my->flags[BURNING] && my->flags[BURNABLE] )
			{
				if ( ticks % 30 == 0 )
				{
					my->colliderCurrentHP--;
					if ( my->colliderCurrentHP <= 0 )
					{
						my->colliderKillerUid = 0;
					}
				}
			}

			my->colliderOldHP = my->colliderCurrentHP;

			if ( my->colliderCurrentHP > 0 )
			{
				if ( my->colliderHideMonster >= 1000 ) // summon a monster when player is near
				{
					Entity* found = nullptr;
					if ( ticks % TICKS_PER_SECOND == 0 )
					{
						auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
						for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !found; ++it )
						{
							list_t* currentList = *it;
							node_t* node;
							for ( node = currentList->first; node != nullptr; node = node->next )
							{
								Entity* entity = (Entity*)node->element;
								if ( entity && (entity->behavior == &actPlayer || (entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader())) )
								{
									real_t tangent = atan2(entity->y - my->y, entity->x - my->x);
									lineTraceTarget(my, my->x, my->y, tangent, 32.0, 0, false, entity);
									if ( hit.entity == entity )
									{
										found = entity;
										break;
									}
								}
							}
						}
					}

					if ( found )
					{
						int type = my->colliderHideMonster % 1000;
						my->colliderHideMonster = 0;
						bool bOldFlag = my->flags[PASSABLE];
						my->flags[PASSABLE] = true;

						int numSpawns = type == BAT_SMALL ? 2 : 1;
						int successes = 0;
						for ( int i = 0; i < numSpawns; ++i )
						{
							auto monster = summonMonster((Monster)type, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8);
							if ( monster )
							{
								monster->yaw = my->yaw;
								monster->lookAtEntity(*found);
								if ( monster->checkEnemy(found) )
								{
									monster->monsterAcquireAttackTarget(*found, MONSTER_STATE_PATH);
								}
								my->colliderCurrentHP = 0;
								my->colliderKillerUid = 0;

								if ( Stat* stats = monster->getStats() )
								{
									stats->MISC_FLAGS[STAT_FLAG_DISABLE_MINIBOSS] = 1;
									if ( stats->type == GHOUL && currentlevel >= 15 )
									{
										strcpy(stats->name, "enslaved ghoul");
										stats->setAttribute("special_npc", "enslaved ghoul");
									}
									if ( stats->type == AUTOMATON )
									{
										monster->monsterStoreType = 1; // damaged
									}
									++successes;
								}
							}
						}

						if ( found->behavior == &actPlayer )
						{
							if ( successes >= 1 )
							{
								Compendium_t::Events_t::eventUpdateWorld(found->skill[2], Compendium_t::CPDM_CONTAINER_MONSTERS, "containers", successes);
							}
							if ( successes == 1 )
							{
								messagePlayer(found->skill[2], MESSAGE_INTERACTION, Language::get(6234),
									getMonsterLocalizedName((Monster)type).c_str(), Language::get(my->getColliderLangName()));
							}
							else if ( successes > 1 )
							{
								messagePlayer(found->skill[2], MESSAGE_INTERACTION, Language::get(6253),
									getMonsterLocalizedPlural((Monster)type).c_str(), Language::get(my->getColliderLangName()));
							}
						}

						my->flags[PASSABLE] = bOldFlag;
					}
				}
			}
			if ( my->colliderCurrentHP <= 0 )
			{
				int sprite = colliderData.gib;
				if ( sprite > 0 )
				{
					createParticleRock(my, sprite);
					if ( multiplayer == SERVER )
					{
						serverSpawnMiscParticles(my, PARTICLE_EFFECT_ABILITY_ROCK, sprite);
					}
				}
				playSoundEntity(my, my->getColliderSfxOnBreak(), 128);
				my->colliderOnDestroy();
				list_RemoveNode(my->mynode);
				return;
			}
		}
	}
}

void Entity::colliderHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster)
{
	auto oldHP = colliderCurrentHP;
	colliderCurrentHP -= damage; //Decrease object health.
	if ( caster )
	{
		if ( colliderCurrentHP <= 0 )
		{
			colliderKillerUid = caster->getUID();
		}
		if ( caster->behavior == &actPlayer )
		{
			if ( colliderCurrentHP <= 0 )
			{
				if ( oldHP > 0 )
				{
					if ( magicProjectile.behavior == &actBomb )
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(3617), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(getColliderLangName()));
					}
					else
					{
						messagePlayer(caster->skill[2], MESSAGE_COMBAT, Language::get(2508), Language::get(getColliderLangName()));
					}
					if ( isColliderWall() )
					{
						Compendium_t::Events_t::eventUpdateWorld(caster->skill[2], Compendium_t::CPDM_BARRIER_DESTROYED, "breakable barriers", 1);
					}
				}
			}
			else
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(3618), items[magicProjectile.skill[21]].getIdentifiedName(), Language::get(getColliderLangName()));
				}
				else
				{
					messagePlayer(caster->skill[2], MESSAGE_COMBAT_BASIC, Language::get(378), Language::get(getColliderLangName()));
				}
			}
			updateEnemyBar(caster, this, Language::get(getColliderLangName()), colliderCurrentHP, colliderMaxHP,
				false, DamageGib::DMG_DEFAULT);
		}
	}

	int sound = 28; //damage.ogg
	if ( getColliderSfxOnHit() > 0 )
	{
		sound = getColliderSfxOnHit();
	}
	playSoundEntity(this, sound, 64);
}

void actFloorDecoration(Entity* my)
{
	if ( !my )
	{
		return;
	}
	if ( !my->flags[PASSABLE] )
	{
		my->flags[PASSABLE] = true;
	}
	if ( my->ticks == 1 && !my->flags[UNCLICKABLE] )
	{
		my->createWorldUITooltip();
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( my->floorDecorationInteractText1 == 0 )
	{
		// no text.
		return;
	}

	// using
	for ( int i = 0; i < MAXPLAYERS; i++ )
	{
		if ( selectedEntity[i] == my || client_selected[i] == my )
		{
			if ( inrange[i] )
			{
				// assemble the string.
				char buf[256] = "";
				int totalChars = 0;
				for ( int i = 8; i < 60; ++i )
				{
					if ( i == 28 ) // circuit_status
					{
						continue;
					}
					if ( my->skill[i] != 0 )
					{
						for ( int c = 0; c < 4; ++c )
						{
							buf[totalChars] = static_cast<char>((my->skill[i] >> (c * 8)) & 0xFF);
							//messagePlayer(0, "%d %d", i, c);
							++totalChars;
						}
					}
				}
				if ( buf[totalChars] != '\0' )
				{
					buf[totalChars] = '\0';
				}
				std::string output = buf;

				if ( buf[0] == '$' )
				{
					// try to replace text with data file entry
					std::string key = "";
					for ( int j = 0; j <= totalChars; ++j )
					{
						char c = buf[j];
						if ( c == '$' ) { continue; }
						if ( charIsWordSeparator(c) ) { break; }
						key += c;
					}
					if ( ScriptTextParser.allEntries.find(key) != ScriptTextParser.allEntries.end() )
					{
						if ( players[i]->isLocalPlayer() )
						{
							if ( players[i]->isLocalPlayerAlive() )
							{
								players[i]->signGUI.openSign(key, my->getUID());
							}
						}
						else if ( multiplayer == SERVER && i > 0 && !client_disconnected[i] )
						{
							strcpy((char*)net_packet->data, "SIGN");
							SDLNet_Write32(my->getUID(), &net_packet->data[4]);
							strcpy((char*)(&net_packet->data[8]), key.c_str());
							net_packet->address.host = net_clients[i - 1].host;
							net_packet->address.port = net_clients[i - 1].port;
							net_packet->len = 8 + strlen(key.c_str()) + 1;
							sendPacketSafe(net_sock, -1, net_packet, i - 1);
						}
						return;
					}
				}

				const bool signpost = output[0] == '#';
				if (signpost) {
					output.erase(0, 1);
				}

				size_t foundInputTag = output.find("@in=");
				while ( foundInputTag != std::string::npos )
				{
					output.erase(foundInputTag, strlen("@in="));
					std::string impulseStr;
					size_t inputTagStrIndex = foundInputTag;
					while ( inputTagStrIndex < output.length()
						&& output.at(inputTagStrIndex) != ' '
						&& output.at(inputTagStrIndex) != '.'
						&& output.at(inputTagStrIndex) != ','
						&& output.at(inputTagStrIndex) != '\0'
						)
					{
						// usage: @in=IN_USE will get the input key for use
						impulseStr = impulseStr + output.at(inputTagStrIndex);
						++inputTagStrIndex;
					}
					output.erase(output.find(impulseStr), impulseStr.length());
					std::string inputFormatted;
					inputFormatted.append("[");
					const char* binding = Input::inputs[0].binding(impulseStr.c_str());
					inputFormatted.append(binding[0] == '\0' ? impulseStr : binding);
					inputFormatted.append("]");
					output.insert(foundInputTag, inputFormatted.c_str());
					foundInputTag = output.find("@in=");
				}

				size_t found = output.find("\\n");
				while ( found != std::string::npos )
				{
					output.erase(found, 2);
					output.insert(found, 1, '\n');
					found = output.find("\\n");
				}
				strcpy(buf, output.c_str());

				if (signpost) {
					players[i]->worldUI.worldTooltipDialogue.createDialogueTooltip(my->getUID(),
						Player::WorldUI_t::WorldTooltipDialogue_t::DIALOGUE_SIGNPOST, buf);
				} else {
					messagePlayer(i, MESSAGE_INSPECTION, buf);
				}
			}
		}
	}
}

void actTextSource(Entity* my)
{
	if ( !my )
	{
		return;
	}
	my->actTextSource();
}

TextSourceScript textSourceScript;

int TextSourceScript::textSourceProcessScriptTag(std::string& input, std::string findTag, Entity& src)
{
	size_t foundScriptTag = input.find(findTag);
	if ( foundScriptTag != std::string::npos )
	{
		if ( !containsOperator(findTag.back()) )
		{
			eraseTag(input, findTag, foundScriptTag);
			// end the function here, return non-error as we found the tag.
			return 0;
		}
		input.erase(foundScriptTag, strlen(findTag.c_str()));
		if ( input.empty() )
		{
			return 0;
		}

		std::string tagValue;

		while ( foundScriptTag < input.length()
			&& input.at(foundScriptTag) != ' '
			&& input.at(foundScriptTag) != '@'
			&& input.at(foundScriptTag) != '\0'
			)
		{
			// usage: Hello @p @d 123 will send to distance 123 units away and send message "Hello player "
			tagValue = tagValue + input.at(foundScriptTag);
			//messagePlayer(clientnum, "%c", output.at(foundDistanceRequirement));
			++foundScriptTag;
		}
		if ( foundScriptTag < input.length() && input.at(foundScriptTag) == ' ' )
		{
			input.erase(input.find(tagValue), tagValue.length() + 1); // clear trailing space
		}
		else
		{
			input.erase(input.find(tagValue), tagValue.length());
		}

		size_t foundMapReference = tagValue.find(",");
		// look for comma or - symbol, e.g @power=15,16 or @power=15-20,16-21
		if ( foundMapReference != std::string::npos )
		{
			if ( findTag.compare("@setvar=") == 0 )
			{
				std::string variableName = tagValue.substr(0, foundMapReference);
				int value = std::stoi(tagValue.substr(foundMapReference + 1, tagValue.length() - foundMapReference));
				if ( scriptVariables.find(variableName) != scriptVariables.end() )
				{
					scriptVariables[variableName] = value;
				}
				else
				{
					scriptVariables.insert(std::make_pair(variableName, value));
				}
				return 0;
			}


			std::string x_str = tagValue.substr(0, foundMapReference);
			std::string y_str = tagValue.substr(foundMapReference + 1, tagValue.length() - foundMapReference);
			int x1 = 0;
			int x2 = 0;
			int y1 = 0;
			int y2 = 0;
			size_t foundMapRange = x_str.find("-");
			if ( foundMapRange != std::string::npos )
			{
				// found map range reference.
				x1 = std::stoi(x_str.substr(0, foundMapRange));
				x2 = std::stoi(x_str.substr(foundMapRange + 1, x_str.length() - foundMapRange));
			}
			else
			{
				x1 = std::stoi(x_str);
				x2 = x1;
			}
			foundMapRange = y_str.find("-");
			if ( foundMapRange != std::string::npos )
			{
				// found map range reference.
				y1 = std::stoi(y_str.substr(0, foundMapRange));
				y2 = std::stoi(y_str.substr(foundMapRange + 1, y_str.length() - foundMapRange));
			}
			else
			{
				y1 = std::stoi(y_str);
				y2 = y1;
			}

			// offset for generated dungeons
			x1 += src.mapGenerationRoomX;
			x2 += src.mapGenerationRoomX;
			y1 += src.mapGenerationRoomY;
			y2 += src.mapGenerationRoomY;

			x1 = std::min(std::max(0, x1), (int)map.width - 1);
			y1 = std::min(std::max(0, y1), (int)map.height - 1);
			x2 = std::min(std::max(x1, x2), (int)map.width - 1);
			y2 = std::min(std::max(y1, y2), (int)map.height - 1);

			return (x1 & 0xFF) + ((x2 & 0xFF) << 8) + ((y1 & 0xFF) << 16) + ((y2 & 0xFF) << 24);
		}
		else
		{
			size_t foundSeperator = tagValue.find(";");
			if ( foundSeperator != std::string::npos )
			{
				std::pair<int, int> param1 = std::make_pair(0, 0);
				std::pair<int, int> param2 = std::make_pair(0, 0);
				std::string first_str = tagValue.substr(0, foundSeperator);
				std::string second_str = tagValue.substr(foundSeperator + 1, tagValue.length() - foundSeperator);
				size_t foundRange = first_str.find("-");
				if ( foundRange != std::string::npos )
				{
					// found range reference.
					param1.first = std::stoi(first_str.substr(0, foundRange));
					param1.second = std::stoi(first_str.substr(foundRange + 1, first_str.length() - foundRange));
				}
				else
				{
					param1.first = std::stoi(first_str);
					param1.second = param1.first;
				}
				foundRange = second_str.find("-");
				if ( foundRange != std::string::npos )
				{
					// found range reference.
					param2.first = std::stoi(second_str.substr(0, foundRange));
					param2.second = std::stoi(second_str.substr(foundRange + 1, second_str.length() - foundRange));
				}
				else
				{
					param2.first = std::stoi(second_str);
					param2.second = param2.first;
				}

				param1.second = std::max(param1.first, param1.second);
				param2.second = std::max(param2.first, param2.second);

				return (param1.first & 0xFF) + ((param1.second & 0xFF) << 8) + ((param2.first & 0xFF) << 16) + ((param2.second & 0xFF) << 24);
			}
			else
			{
				size_t foundRange = tagValue.find("-");
				if ( foundRange != std::string::npos )
				{
					std::pair<int, int> param1 = std::make_pair(0, 0);
					// found range reference.
					std::string first_str = tagValue.substr(0, foundRange);
					std::string second_str = tagValue.substr(foundRange + 1, tagValue.length() - foundRange);

					param1.first = std::stoi(first_str);
					param1.second = std::stoi(second_str);
					param1.second = std::max(param1.first, param1.second);
					return (param1.first & 0xFF) + ((param1.second & 0xFF) << 8);
				}
			}
		}

		if ( !tagValue.compare("all") )
		{
			int x1 = 0;
			int x2 = map.width - 1;
			int y1 = 0;
			int y2 = map.height - 1;
			return (x1 & 0xFF) + ((x2 & 0xFF) << 8) + ((y1 & 0xFF) << 16) + ((y2 & 0xFF) << 24);
		}
		else if ( findTag.compare("@attachto=") == 0 || findTag.compare("@reattachto=") == 0 )
		{
			if ( !tagValue.compare("monsters") )
			{
				return TO_MONSTERS;
			}
			else if ( !tagValue.compare("items") )
			{
				return TO_ITEMS;
			}
			else if ( !tagValue.compare("players") )
			{
				return TO_PLAYERS;
			}
			else
			{
				for ( int i = 0; i < NUMMONSTERS; ++i )
				{
					if ( !tagValue.compare(monstertypename[i]) )
					{
						return TO_NOTHING + i;
					}
				}
				return k_ScriptError;
			}
		}
		else if ( findTag.compare("@triggerif=") == 0 )
		{
			if ( !tagValue.compare("attached#dies") )
			{
				return TRIGGER_ATTACHED_ISREMOVED;
			}
			else if ( !tagValue.compare("attached#exists") )
			{
				return TRIGGER_ATTACHED_EXISTS;
			}
			else if ( !tagValue.compare("attached#invisible") )
			{
				return TRIGGER_ATTACHED_INVIS;
			}
			else if ( !tagValue.compare("attached#visible") )
			{
				return TRIGGER_ATTACHED_VISIBLE;
			}
			else if ( !tagValue.compare("always") )
			{
				return TRIGGER_ATTACHED_ALWAYS;
			}
			else
			{
				return k_ScriptError;
			}
		}

		return std::stoi(tagValue);
	}
	return k_ScriptError;
}

void TextSourceScript::handleTextSourceScript(Entity& src, std::string input)
{
	bool statOnlyUpdateNeeded = false;

	std::vector<std::string> tokens;
	std::string searchString = input;
	size_t findToken = searchString.find("@");
	while ( findToken != std::string::npos )
	{
		std::string token = "@";
		++findToken;
		while ( findToken < searchString.length()
			&& searchString.at(findToken) != ' '
			&& searchString.at(findToken) != '@'
			&& searchString.at(findToken) != '\0'
			)
		{
			token = token + searchString.at(findToken);
			++findToken;
		}
		searchString.erase(searchString.find(token), token.length());
		tokens.push_back(token);
		findToken = searchString.find("@");
	}

	printlog("[SCRIPT]: Starting Execution...");
	std::string executionLog = "[SCRIPT]: Processed tokens:";
	std::vector<Entity*> attachedEntities = textSourceScript.getScriptAttachedEntities(src);

	for ( auto it = tokens.begin(); it != tokens.end(); ++it )
	{
		executionLog.append(" ").append(*it);

		bool processOnAttachedEntity = false;
		size_t foundAttachedTag = (*it).find("@attached.");
		if ( foundAttachedTag != std::string::npos )
		{
			processOnAttachedEntity = true;
			// erase "attached." in token, pass the information on to the tag contents.
			(*it).erase(foundAttachedTag + 1, strlen("attached."));
			size_t foundAttachedTag = input.find("@attached.");
			if ( foundAttachedTag != std::string::npos )
			{
				input.erase(foundAttachedTag + 1, strlen("attached."));
			}
		}

		if ( (*it).find("@reattachto=") != std::string::npos )
		{
			int attachTo = textSourceProcessScriptTag(input, "@reattachto=", src);
			if ( attachTo == k_ScriptError )
			{
				return;
			}
			attachedEntities.clear();
			list_FreeAll(&src.children); // reattach all the entities again.

			textSourceScript.setScriptType(src.textSourceIsScript, textSourceScript.SCRIPT_ATTACHED);
			int x1 = static_cast<int>(src.x / 16); // default to just whatever this script is sitting on.
			int x2 = static_cast<int>(src.x / 16);
			int y1 = static_cast<int>(src.y / 16);
			int y2 = static_cast<int>(src.y / 16);

			// offset for generated dungeons
			x1 += src.mapGenerationRoomX;
			x2 += src.mapGenerationRoomX;
			y1 += src.mapGenerationRoomY;
			y2 += src.mapGenerationRoomY;

			if ( input.find("@attachrange=") != std::string::npos )
			{
				int result = textSourceProcessScriptTag(input, "@attachrange=", src);
				if ( result != k_ScriptError )
				{
					x1 = result & 0xFF;
					x2 = (result >> 8) & 0xFF;
					y1 = (result >> 16) & 0xFF;
					y2 = (result >> 24) & 0xFF;
				}
			}
			textSourceScript.setAttachedToEntityType(src.textSourceIsScript, attachTo);
			for ( node_t* node = map.entities->first; node; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity )
				{
					if ( (entity->behavior == &actMonster && attachTo == TO_MONSTERS)
						|| (entity->behavior == &actPlayer && attachTo == TO_PLAYERS)
						|| (entity->behavior == &actItem && attachTo == TO_ITEMS)
						|| (entity->behavior == &actMonster
							&& attachTo >= TO_NOTHING && attachTo <= TO_DUMMYBOT
							&& entity->getRace() == (attachTo - TO_NOTHING)) )
					{
						// found our entity.
					}
					else
					{
						continue;
					}
					int findx = static_cast<int>(entity->x) >> 4;
					int findy = static_cast<int>(entity->y) >> 4;
					if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
					{
						node_t* node = list_AddNodeLast(&src.children);
						node->deconstructor = &defaultDeconstructor;
						Uint32* entityUid = (Uint32*)(malloc(sizeof(Uint32)));
						node->element = entityUid;
						node->size = sizeof(Uint32);
						*entityUid = entity->getUID();
					}
				}
			}
			attachedEntities = textSourceScript.getScriptAttachedEntities(src);
		}
		else if ( (*it).find("@clrplayer") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@clrplayer", src);
			if ( result != k_ScriptError )
			{
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]] )
						{
							applyToEntities.push_back(entity);
						}
					}
				}
				else
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
						{
							applyToEntities.push_back(players[c]->entity);
						}
					}
				}

				for ( auto entity : applyToEntities )
				{
					Stat* stats = entity->getStats();
					int player = entity->skill[2];
					if ( player >= 0 && players[player]->isLocalPlayer() )
					{
						playerClearInventory(true);
					}
					else
					{
						// other players
						stats->freePlayerEquipment();
						stats->clearStats();
						updateClientInformation(player, true, true, TextSourceScript::CLIENT_UPDATE_ALL);
					}
				}
			}
		}
		else if ( (*it).find("@class=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@class=", src);
			if ( result != k_ScriptError && result >= CLASS_BARBARIAN && result < NUMCLASSES )
			{
				if ( !enabledDLCPack1 && result >= CLASS_CONJURER && result <= CLASS_BREWER )
				{
					result = CLASS_BARBARIAN;
				}
				if ( !enabledDLCPack2 && result >= CLASS_MACHINIST && result <= CLASS_HUNTER )
				{
					result = CLASS_BARBARIAN;
				}

				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]] )
						{
							applyToEntities.push_back(entity);
						}
					}
				}
				else
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
						{
							applyToEntities.push_back(players[c]->entity);
						}
					}
				}

				for ( auto entity : applyToEntities )
				{
					int player = entity->skill[2];
					client_classes[player] = result;
					bool oldIntro = intro;
					intro = true;
					initClass(player);
					intro = oldIntro;

					if (multiplayer == SERVER)
					{
						for (int c = 1; c < MAXPLAYERS; ++c)
						{
							if (!client_disconnected[c] &&!players[c]->isLocalPlayer())
							{
								strcpy((char*)net_packet->data, "SCRC");
								net_packet->data[4] = (Uint8)player;
								net_packet->data[5] = (Uint8)client_classes[player];
								net_packet->address.host = net_clients[c - 1].host;
								net_packet->address.port = net_clients[c - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, c - 1);
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@clrstats") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@clrstats", src);
			if ( result != k_ScriptError )
			{
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actPlayer && entity->behavior != actMonster )
						{
							continue;
						}
						if ( entity->behavior == &actPlayer && !client_disconnected[entity->skill[2]] )
						{
							statOnlyUpdateNeeded = true;
						}
						applyToEntities.push_back(entity);
					}
				}
				else
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
						{
							applyToEntities.push_back(players[c]->entity);
						}
					}
				}

				for ( auto entity : applyToEntities )
				{
					if ( entity->behavior == &actPlayer )
					{
						statOnlyUpdateNeeded = true;
					}
					Stat* stats = entity->getStats();
					if ( stats )
					{
						stats->HP = DEFAULT_HP;
						stats->MAXHP = DEFAULT_HP;
						stats->OLDHP = stats->HP;
						stats->MP = DEFAULT_MP;
						stats->MAXMP = DEFAULT_MP;
						stats->STR = 0;
						stats->DEX = 0;
						stats->CON = 0;
						stats->INT = 0;
						stats->PER = 0;
						stats->CHR = 0;
						stats->LVL = 1;
						stats->GOLD = 0;
						stats->EXP = 0;
					}
				}
			}
		}
		else if ( (*it).find("@hunger=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@hunger=", src);
			if ( result != k_ScriptError )
			{
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actPlayer )
						{
							continue;
						}
						applyToEntities.push_back(entity);
					}
				}
				else
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
						{
							applyToEntities.push_back(players[c]->entity);
						}
					}
				}

				for ( auto entity : applyToEntities )
				{
					int player = -1;
					if ( entity->behavior != &actPlayer )
					{
						continue;
					}
					player = entity->skill[2];
					if ( stats[player] && !client_disconnected[player] )
					{
						stats[player]->HUNGER = std::min(result, 2000);
						if ( !players[player]->isLocalPlayer() )
						{
							updateClientInformation(player, false, false, TextSourceScript::CLIENT_UPDATE_HUNGER);
						}
					}
				}
			}
		}
		else if ( (*it).find("@nextlevel=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@nextlevel=", src);
			if ( result != k_ScriptError )
			{
				loadnextlevel = true;
				Compendium_t::Events_t::previousCurrentLevel = currentlevel;
				Compendium_t::Events_t::previousSecretlevel = secretlevel;
				skipLevelsOnLoad = result;
			}
		}
		else if ( (*it).find("@power=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@power=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				bool foundExisting = false;
				std::unordered_set<int> plateSpots;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* pressurePlate = (Entity*)node->element;
					if ( pressurePlate && pressurePlate->behavior == &actTrapPermanent )
					{
						int findx = static_cast<int>(pressurePlate->x) >> 4;
						int findy = static_cast<int>(pressurePlate->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							if ( pressurePlate->skill[0] == 0 )
							{
								pressurePlate->toggleSwitch();
							}
							plateSpots.insert(static_cast<int>(pressurePlate->x / 16) + map.width * static_cast<int>(pressurePlate->y / 16));
						}
					}
				}
				for ( int x = x1; x <= x2; ++x )
				{
					for ( int y = y1; y <= y2; ++y )
					{
						if ( plateSpots.find(x + map.width * y) == plateSpots.end() )
						{
							Entity* pressurePlate = newEntity(-1, 1, map.entities, nullptr);
							pressurePlate->sizex = 2;
							pressurePlate->sizey = 2;
							pressurePlate->x = x * 16 + 8;
							pressurePlate->y = y * 16 + 8;
							pressurePlate->behavior = &actTrapPermanent;
							pressurePlate->skill[1] = 1;
							pressurePlate->flags[SPRITE] = true;
							pressurePlate->flags[INVISIBLE] = true;
							pressurePlate->flags[PASSABLE] = true;
							pressurePlate->flags[NOUPDATE] = true;
							TileEntityList.addEntity(*pressurePlate); // make sure new nodes are added to the tile list to properly update neighbors.
							pressurePlate->toggleSwitch();
						}
					}
				}
			}
		}
		else if ( (*it).find("@unpower=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@unpower=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* pressurePlate = (Entity*)node->element;
					if ( pressurePlate && pressurePlate->behavior == &actTrapPermanent )
					{
						int findx = static_cast<int>(pressurePlate->x) >> 4;
						int findy = static_cast<int>(pressurePlate->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							if ( pressurePlate->skill[0] != 0 )
							{
								pressurePlate->toggleSwitch();
								pressurePlate->skill[1] = 1; // can't be interacted.
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@wire=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@wire=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				bool foundExisting = false;
				std::unordered_set<int> wireSpots;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* wire = (Entity*)node->element;
					if ( wire && wire->behavior == &actCircuit )
					{
						int findx = static_cast<int>(wire->x) >> 4;
						int findy = static_cast<int>(wire->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							wireSpots.insert(static_cast<int>(wire->x / 16) + map.width * static_cast<int>(wire->y / 16));
						}
					}
				}
				for ( int x = x1; x <= x2; ++x )
				{
					for ( int y = y1; y <= y2; ++y )
					{
						if ( wireSpots.find(x + map.width * y) == wireSpots.end() )
						{
							Entity* wire = newEntity(-1, 1, map.entities, nullptr);
							wire->sizex = 3;
							wire->sizey = 3;
							wire->x = x * 16 + 8;
							wire->y = y * 16 + 8;
							wire->z = 5;
							wire->behavior = &actCircuit;
							wire->flags[PASSABLE] = true;
							wire->flags[INVISIBLE] = true;
							wire->flags[NOUPDATE] = true;
							wire->skill[28] = 1; //It's a depowered powerable.
							TileEntityList.addEntity(*wire); // make sure new nodes are added to the tile list to properly update neighbors.
							wire->updateCircuitNeighbors();
						}
					}
				}
			}
		}
		else if ( (*it).find("@unwire=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@unwire=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				node_t* nextnode = nullptr;
				for ( node_t* node = map.entities->first; node; node = nextnode )
				{
					nextnode = node->next;
					Entity* wire = (Entity*)node->element;
					if ( wire && wire->behavior == &actCircuit )
					{
						int findx = static_cast<int>(wire->x) >> 4;
						int findy = static_cast<int>(wire->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							wire->skill[28] = 1; // unpower the wire
							wire->updateCircuitNeighbors(); // update surrounding stuff
							list_RemoveNode(node); // delete this
						}
					}
				}
			}
		}
		else if ( (*it).find("@freezemonsters=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@freezemonsters=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						int findx = static_cast<int>(entity->x) >> 4;
						int findy = static_cast<int>(entity->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							entity->setEffect(EFF_STUNNED, true, -1, false);
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								entity->setEffect(EFF_STUNNED, true, -1, false);
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@unfreezemonsters=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@unfreezemonsters=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						int findx = static_cast<int>(entity->x) >> 4;
						int findy = static_cast<int>(entity->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							entity->setEffect(EFF_STUNNED, false, 0, false);
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								entity->setEffect(EFF_STUNNED, false, 0, false);
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@seteffect=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@seteffect=", src);
			if ( result != k_ScriptError )
			{
				int effect = result & 0xFF;
				int effectEndRange = (result >> 8) & 0xFF;
				int duration = (result >> 16) & 0xFF;
				int durationEndRange = (result >> 24) & 0xFF;
				duration = duration + local_rng.rand() % (std::max(1, (durationEndRange - duration)));
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster && entity->behavior != &actPlayer )
						{
							continue;
						}
						for ( int i = effect; i <= effectEndRange && i < NUMEFFECTS && i >= 0; ++i )
						{
							entity->setEffect(i, true, duration * TICKS_PER_SECOND, false);
						}
						if ( multiplayer == SERVER )
						{
							entity->serverUpdateEffectsForEntity(true);
						}
					}
				}
			}
		}
		else if ( (*it).find("@clreffect=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@clreffect=", src);
			if ( result != k_ScriptError )
			{
				int effect = result & 0xFF;
				int effectEndRange = (result >> 8) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster && entity->behavior != &actPlayer )
						{
							continue;
						}
						for ( int i = effect; i <= effectEndRange && i < NUMEFFECTS && i >= 0; ++i )
						{
							entity->setEffect(i, false, 0, false);
						}
						if ( multiplayer == SERVER )
						{
							entity->serverUpdateEffectsForEntity(true);
						}
					}
				}
			}
		}
		else if ( (*it).find("@findtarget=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@findtarget=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						Stat* stats = entity->getStats();
						real_t dist = 10000.f;
						real_t sightrange = 256.0;
						Entity* toAttack = nullptr;
						for ( node_t* node = map.creatures->first; node; node = node->next )
						{
							Entity* target = (Entity*)node->element;
							if ( (target->behavior == &actMonster || target->behavior == &actPlayer) && target != entity
								&& entity->checkEnemy(target) )
							{
								int findx = static_cast<int>(target->x) >> 4;
								int findy = static_cast<int>(target->y) >> 4;
								if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
								{
									real_t oldDist = dist;
									dist = sqrt(pow(entity->x - target->x, 2) + pow(entity->y - target->y, 2));
									if ( dist < sightrange && dist <= oldDist )
									{
										double tangent = atan2(target->y - entity->y, target->x - entity->x);
										//lineTrace(entity, entity->x, entity->y, tangent, sightrange, 0, false);
										//if ( hit.entity == target )
										//{
											//my->monsterLookTime = 1;
											//my->monsterMoveTime = local_rng.rand() % 10 + 1;
											entity->monsterLookDir = tangent;
											toAttack = target;
										//}
									}
									else
									{
										dist = oldDist;
									}
								}
							}
						}
						if ( toAttack )
						{
							entity->monsterAcquireAttackTarget(*toAttack, MONSTER_STATE_PATH);
						}
					}
				}
			}
		}
		else if ( (*it).find("@killall=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@killall=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						int findx = static_cast<int>(entity->x) >> 4;
						int findy = static_cast<int>(entity->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							entity->setHP(0);
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								entity->setHP(0);
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@killenemies=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@killenemies=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster || !entity->monsterAllyGetPlayerLeader() )
						{
							continue;
						}
						int findx = static_cast<int>(entity->x) >> 4;
						int findy = static_cast<int>(entity->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							entity->setHP(0);
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								entity->setHP(0);
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@nodropitems=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@nodropitems=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster )
						{
							continue;
						}
						int findx = static_cast<int>(entity->x) >> 4;
						int findy = static_cast<int>(entity->y) >> 4;
						if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
						{
							if ( entity->getStats() )
							{
								entity->getStats()->monsterNoDropItems = 1;
							}
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								if ( entity->getStats() )
								{
									entity->getStats()->monsterNoDropItems = 1;
								}
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@copyNPC=") != std::string::npos )
		{
			std::string profTag = "@copyNPC=";
			int result = textSourceProcessScriptTag(input, profTag, src);
			if ( result != k_ScriptError )
			{
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior != &actMonster && entity->behavior != &actPlayer )
						{
							continue;
						}
						if ( entity->behavior == &actPlayer && client_disconnected[entity->skill[2]] )
						{
							continue;
						}
						applyToEntities.push_back(entity);
					}
				}
				
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					Entity* scriptEntity = (Entity*)node->element;
					if ( scriptEntity && scriptEntity->behavior == &actMonster )
					{
						Stat* scriptStats = scriptEntity->getStats();
						if ( scriptStats && !strcmp(scriptStats->name, "scriptNPC") && (scriptStats->MISC_FLAGS[STAT_FLAG_NPC] & 0xFF) == result )
						{
							// copy stats.
							if ( processOnAttachedEntity )
							{
								for ( auto entity : applyToEntities )
								{
									Stat* stats = entity->getStats();
									if ( stats )
									{
										stats->copyNPCStatsAndInventoryFrom(*scriptStats);
										if ( entity->behavior == &actPlayer )
										{
											statOnlyUpdateNeeded = true;
										}
									}
								}
							}
							else
							{
								for ( int c = 0; c < MAXPLAYERS && !client_disconnected[c] && players[c] && players[c]->entity; ++c )
								{
									stats[c]->copyNPCStatsAndInventoryFrom(*scriptStats);
								}
								statOnlyUpdateNeeded = true;
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@setenemy=") != std::string::npos )
		{
			std::string profTag = "@setenemy=";
			int result = textSourceProcessScriptTag(input, profTag, src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								if ( entity->getStats() )
								{
									entity->getStats()->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ENEMY;
									serverUpdateEntityStatFlag(entity, 20);
								}
							}
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								if ( entity->getStats() )
								{
									entity->getStats()->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ENEMY;
									serverUpdateEntityStatFlag(entity, 20);
								}
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@setally=") != std::string::npos )
		{
			std::string profTag = "@setally=";
			int result = textSourceProcessScriptTag(input, profTag, src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF; 
				int y2 = (result >> 24) & 0xFF;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								if ( entity->getStats() )
								{
									entity->getStats()->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ALLY;
								}
							}
						}
					}
				}
				else
				{
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster && !entity->monsterAllyGetPlayerLeader() )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								if ( entity->getStats() )
								{
									entity->getStats()->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ALLY;
								}
							}
						}
					}
				}
			}
		}
		else if ( (*it).find("@setvar=") != std::string::npos )
		{
			std::string profTag = "@setvar=";
			int result = textSourceProcessScriptTag(input, profTag, src);
			if ( result != k_ScriptError )
			{
				achievementObserver.checkMapScriptsOnVariableSet();
				/*for ( auto it = scriptVariables.begin(); it != scriptVariables.end(); ++it )
				{
					printlog("%s | %d", (*it).first.c_str(), (*it).second);
				}*/
			}
		}
		//else if ( (*it).find("@addtomonster=") != std::string::npos ) // adds entire stack to 1 destination monster
		//{
		//	int result = textSourceProcessScriptTag(input, "@addtomonster=", src);
		//	if ( result != k_ScriptError )
		//	{
		//		int x1 = result & 0xFF;
		//		int x2 = (result >> 8) & 0xFF;
		//		int y1 = (result >> 16) & 0xFF;
		//		int y2 = (result >> 24) & 0xFF;
		//		std::vector<Entity*> applyToEntities;
		//		if ( processOnAttachedEntity && textSourceScript.getAttachedToEntityType(src.textSourceIsScript) == textSourceScript.TO_ITEMS )
		//		{
		//			for ( auto entity : attachedEntities )
		//			{
		//				if ( entity->behavior == &actItem )
		//				{
		//					applyToEntities.push_back(entity);
		//				}
		//			}
		//			Entity* monster = nullptr;
		//			for ( node_t* node = map.creatures->first; node; node = node->next )
		//			{
		//				Entity* entity = (Entity*)node->element;
		//				if ( entity && entity->behavior == &actMonster )
		//				{
		//					int findx = static_cast<int>(entity->x) >> 4;
		//					int findy = static_cast<int>(entity->y) >> 4;
		//					if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
		//					{
		//						monster = entity;
		//					}
		//				}
		//			}
		//			for ( auto entity : applyToEntities )
		//			{
		//				Item* item = newItemFromEntity(entity);
		//				if ( item && monster )
		//				{
		//					monster->addItemToMonsterInventory(item);
		//				}
		//				list_RemoveNode(entity->mynode);
		//				entity = nullptr;
		//			}
		//		}
		//		else
		//		{
		//			// not implemented, needs to be attached to items.
		//		}
		//	}
		//}
		else if ( (*it).find("@addtomonsters=") != std::string::npos ) // adds entire stack between monster(s)
		{
			int result = textSourceProcessScriptTag(input, "@addtomonsters=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity && textSourceScript.getAttachedToEntityType(src.textSourceIsScript) == textSourceScript.TO_ITEMS )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actItem )
						{
							applyToEntities.push_back(entity);
						}
					}
					std::vector<Entity*> monsters;
					for ( node_t* node = map.creatures->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actMonster )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								monsters.push_back(entity);
							}
						}
					}
					for ( auto entity : applyToEntities )
					{
						for ( auto monster : monsters )
						{
							Item* item = newItemFromEntity(entity);
							if ( item )
							{
								monster->addItemToMonsterInventory(item);
							}
						}
						list_RemoveNode(entity->mynode);
						entity = nullptr;
					}
				}
				else
				{
					// not implemented, needs to be attached to items.
				}
			}
		}
		else if ( (*it).find("@equipitems=") != std::string::npos ) // monster try equip items
		{
			int result = textSourceProcessScriptTag(input, "@equipitems=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actMonster )
						{
							applyToEntities.push_back(entity);
						}
					}

					std::vector<Entity*> items;
					if ( attachedEntities.size() > 0 )
					{
						for ( node_t* node = map.entities->first; node; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actItem )
							{
								int findx = static_cast<int>(entity->x) >> 4;
								int findy = static_cast<int>(entity->y) >> 4;
								if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
								{
									items.push_back(entity);
								}
							}
						}
					}
					for ( auto entity : applyToEntities )
					{
						for ( auto i : items )
						{
							Item* item = newItemFromEntity(i);
							Stat* myStats = entity->getStats();
							if ( item && myStats )
							{
								Item** itemSlot = nullptr;
								switch ( ::items[item->type].item_slot )
								{
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_WEAPON:
										itemSlot = &myStats->weapon;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_SHIELD:
										if ( itemCategory(item) == SPELLBOOK )
										{
											itemSlot = &myStats->weapon;
										}
										else
										{
											itemSlot = &myStats->shield;
										}
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_GLOVES:
										itemSlot = &myStats->gloves;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_CLOAK:
										itemSlot = &myStats->cloak;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BOOTS:
										itemSlot = &myStats->shoes;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_BREASTPLATE:
										itemSlot = &myStats->breastplate;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_AMULET:
										itemSlot = &myStats->amulet;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_RING:
										itemSlot = &myStats->ring;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_MASK:
										itemSlot = &myStats->mask;
										break;
									case ItemEquippableSlot::EQUIPPABLE_IN_SLOT_HELM:
										itemSlot = &myStats->helmet;
										break;
									case ItemEquippableSlot::NO_EQUIP:
										break;
									default:
										break;

								}
								if ( itemSlot )
								{
									if ( (*itemSlot) != nullptr )
									{
										copyItem(*itemSlot, item); // set equipped item to this new one.
									}
									else
									{
										(*itemSlot) = newItem(WOODEN_SHIELD, EXCELLENT, 0, 1, 0, false, NULL);
										copyItem(*itemSlot, item); // set equipped item to this new one.
									}
								}
								free(item);
							}
						}
					}
					for ( auto i : items )
					{
						list_RemoveNode(i->mynode);
						i = nullptr;
					}
				}
				else
				{
					// not implemented, needs to be attached to creatures
				}
			}
		}
		else if ( (*it).find("@pickupitems=") != std::string::npos ) // adds entire stack between monster(s)
		{
			int result = textSourceProcessScriptTag(input, "@pickupitems=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity )
				{
					for ( auto entity : attachedEntities )
					{
						if ( (entity->behavior == &actMonster 
							&& textSourceScript.getAttachedToEntityType(src.textSourceIsScript) != textSourceScript.TO_PLAYERS)
							|| (entity->behavior == &actPlayer
								&& textSourceScript.getAttachedToEntityType(src.textSourceIsScript) == textSourceScript.TO_PLAYERS) )
						{
							applyToEntities.push_back(entity);
						}
					}

					std::vector<Entity*> items;
					if ( attachedEntities.size() > 0 )
					{
						for ( node_t* node = map.entities->first; node; node = node->next )
						{
							Entity* entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actItem )
							{
								int findx = static_cast<int>(entity->x) >> 4;
								int findy = static_cast<int>(entity->y) >> 4;
								if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
								{
									items.push_back(entity);
								}
							}
						}
					}
					for ( auto entity : applyToEntities )
					{
						for ( auto i : items )
						{
							Item* item = newItemFromEntity(i);
							if ( item )
							{
								if ( entity->behavior == &actMonster )
								{
									entity->addItemToMonsterInventory(item);
								}
								else if ( entity->behavior == &actPlayer )
								{
									Item* pickedUp = itemPickup(entity->skill[2], item);
									if ( pickedUp )
									{
										if ( players[entity->skill[2]]->isLocalPlayer() )
										{
											// item is the new inventory stack for server, free the picked up items
											free(item);
										}
										else
										{
											free(pickedUp); // item is the picked up items (pickedUp == item)
										}
									}
								}
							}
						}
					}
					for ( auto i : items )
					{
						list_RemoveNode(i->mynode);
					}
				}
				else
				{
					// not implemented, needs to be attached to creatures
				}
			}
		}
		else if ( (*it).find("@addtochest=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(input, "@addtochest=", src);
			if ( result != k_ScriptError )
			{
				int x1 = result & 0xFF;
				int x2 = (result >> 8) & 0xFF;
				int y1 = (result >> 16) & 0xFF;
				int y2 = (result >> 24) & 0xFF;
				std::vector<Entity*> applyToEntities;
				if ( processOnAttachedEntity && textSourceScript.getAttachedToEntityType(src.textSourceIsScript) == textSourceScript.TO_ITEMS )
				{
					for ( auto entity : attachedEntities )
					{
						if ( entity->behavior == &actItem )
						{
							applyToEntities.push_back(entity);
						}
					}
					Entity* chest = nullptr;
					for ( node_t* node = map.entities->first; node; node = node->next )
					{
						Entity* entity = (Entity*)node->element;
						if ( entity && entity->behavior == &actChest )
						{
							int findx = static_cast<int>(entity->x) >> 4;
							int findy = static_cast<int>(entity->y) >> 4;
							if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
							{
								chest = entity;
							}
						}
					}
					if ( chest )
					{
						for ( auto entity : applyToEntities )
						{
							Item* item = newItemFromEntity(entity);
							if ( item )
							{
								chest->addItemToChest(item, true, nullptr);
							}
							list_RemoveNode(entity->mynode);
							entity = nullptr;
						}
					}
				}
				else
				{
					// not implemented, needs to be attached to items.
				}
			}
		}
		else
		{
			for ( int i = 0; i < NUMSTATS; ++i )
			{
				std::string profTag = "@st";
				switch ( i )
				{
					case STAT_STR:
						profTag.append("STR");
						break;
					case STAT_DEX:
						profTag.append("DEX");
						break;
					case STAT_CON:
						profTag.append("INT");
						break;
					case STAT_INT:
						profTag.append("CON");
						break;
					case STAT_PER:
						profTag.append("PER");
						break;
					case STAT_CHR:
						profTag.append("CHR");
						break;
					default:
						break;
				}
				profTag.append("=");
				if ( (*it).find(profTag) != std::string::npos )
				{
					int result = textSourceProcessScriptTag(input, profTag, src);
					if ( result != k_ScriptError )
					{
						std::vector<Entity*> applyToEntities;
						if ( processOnAttachedEntity )
						{
							for ( auto entity : attachedEntities )
							{
								if ( entity->behavior == &actMonster 
									|| (entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]]) )
								{
									applyToEntities.push_back(entity);
									statOnlyUpdateNeeded = (entity->behavior == &actPlayer);
								}
							}
						}
						else
						{
							for ( int c = 0; c < MAXPLAYERS; ++c )
							{
								if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
								{
									applyToEntities.push_back(players[c]->entity);
									statOnlyUpdateNeeded = true;
								}
							}
						}
						for ( auto entity : applyToEntities )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								switch ( i )
								{
									case STAT_STR:
										stats->STR = result;
										break;
									case STAT_DEX:
										stats->DEX = result;
										break;
									case STAT_CON:
										stats->CON = result;
										break;
									case STAT_INT:
										stats->INT = result;
										break;
									case STAT_PER:
										stats->PER = result;
										break;
									case STAT_CHR:
										stats->CHR = result;
										break;
									default:
										break;
								}
							}
						}
					}
				}
			}
			for ( int i = 0; i < NUMSTATS; ++i )
			{
				std::string profTag = "@st";
				switch ( i )
				{
					case STAT_STR:
						profTag.append("STR");
						break;
					case STAT_DEX:
						profTag.append("DEX");
						break;
					case STAT_CON:
						profTag.append("INT");
						break;
					case STAT_INT:
						profTag.append("CON");
						break;
					case STAT_PER:
						profTag.append("PER");
						break;
					case STAT_CHR:
						profTag.append("CHR");
						break;
					default:
						break;
				}
				profTag.append("+");
				if ( (*it).find(profTag) != std::string::npos )
				{
					int result = textSourceProcessScriptTag(input, profTag, src);
					if ( result != k_ScriptError )
					{
						std::vector<Entity*> applyToEntities;
						if ( processOnAttachedEntity )
						{
							for ( auto entity : attachedEntities )
							{
								if ( entity->behavior == &actMonster
									|| (entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]]) )
								{
									applyToEntities.push_back(entity);
									statOnlyUpdateNeeded = (entity->behavior == &actPlayer);
								}
							}
						}
						else
						{
							for ( int c = 0; c < MAXPLAYERS; ++c )
							{
								if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
								{
									applyToEntities.push_back(players[c]->entity);
									statOnlyUpdateNeeded = true;
								}
							}
						}
						for ( auto entity : applyToEntities )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								switch ( i )
								{
									case STAT_STR:
										stats->STR += result;
										break;
									case STAT_DEX:
										stats->DEX += result;
										break;
									case STAT_CON:
										stats->CON += result;
										break;
									case STAT_INT:
										stats->INT += result;
										break;
									case STAT_PER:
										stats->PER += result;
										break;
									case STAT_CHR:
										stats->CHR += result;
										break;
									default:
										break;
								}
							}
						}
					}
				}
			}

			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				std::string profTag = "@pro";
				profTag.append(std::to_string(i).append("="));
				if ( (*it).find(profTag) != std::string::npos )
				{
					int result = textSourceProcessScriptTag(input, profTag, src);
					if ( result != k_ScriptError )
					{
						std::vector<Entity*> applyToEntities;
						if ( processOnAttachedEntity )
						{
							for ( auto entity : attachedEntities )
							{
								if ( entity->behavior == &actMonster
									|| (entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]]) )
								{
									applyToEntities.push_back(entity);
									statOnlyUpdateNeeded = (entity->behavior == &actPlayer);
								}
							}
						}
						else
						{
							for ( int c = 0; c < MAXPLAYERS; ++c )
							{
								if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
								{
									applyToEntities.push_back(players[c]->entity);
									statOnlyUpdateNeeded = true;
								}
							}
						}

						for ( auto entity : applyToEntities )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								stats->setProficiency(i, result);
							}
						}
					}
				}
			}
			for ( int i = 0; i < NUMPROFICIENCIES; ++i )
			{
				std::string profTag = "@pro";
				profTag.append(std::to_string(i).append("+"));
				if ( (*it).find(profTag) != std::string::npos )
				{
					int result = textSourceProcessScriptTag(input, profTag, src);
					if ( result != k_ScriptError )
					{
						std::vector<Entity*> applyToEntities;
						if ( processOnAttachedEntity )
						{
							for ( auto entity : attachedEntities )
							{
								if ( entity->behavior == &actMonster
									|| (entity->behavior == &actPlayer && stats[entity->skill[2]] && !client_disconnected[entity->skill[2]]) )
								{
									applyToEntities.push_back(entity);
									statOnlyUpdateNeeded = (entity->behavior == &actPlayer);
								}
							}
						}
						else
						{
							for ( int c = 0; c < MAXPLAYERS; ++c )
							{
								if ( stats[c] && !client_disconnected[c] && players[c] && players[c]->entity )
								{
									applyToEntities.push_back(players[c]->entity);
									statOnlyUpdateNeeded = true;
								}
							}
						}

						for ( auto entity : applyToEntities )
						{
							Stat* stats = entity->getStats();
							if ( stats )
							{
								stats->setProficiency(i, result);
							}
						}
					}
				}
			}
		}
	}
	printlog("%s", executionLog.c_str());
	if ( statOnlyUpdateNeeded )
	{
		for ( int c = 1; c < MAXPLAYERS; ++c )
		{
			if ( !players[c]->isLocalPlayer() )
			{
				updateClientInformation(c, false, false, TextSourceScript::CLIENT_UPDATE_ALL);
			}
		}
	}
	printlog("[SCRIPT]: Finished running.");
}

void Entity::actTextSource()
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( ((textSourceVariables4W >> 16) & 0xFFFF) == 0 ) // store the delay in the 16 leftmost bits.
	{
		textSourceVariables4W |= (textSourceDelay << 16);
	}

	bool powered = false;
	if ( textSourceScript.getScriptType(textSourceIsScript) == textSourceScript.NO_SCRIPT 
		|| textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_POWER )
	{
		powered = (circuit_status == CIRCUIT_ON);
	}
	else if ( textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_ATTACHED_ALWAYS )
	{
		textSourceScript.setScriptType(textSourceIsScript, textSourceScript.SCRIPT_ATTACHED_FIRED);
		powered = true;
	}
	else if ( textSourceScript.getScriptType(textSourceIsScript) == textSourceScript.SCRIPT_ATTACHED )
	{
		int entitiesExisting = 0;
		int entitiesVisible = 0;
		int entitiesInvisible = 0;
		// check if our attached entities still exist.
		for ( node_t* node = children.first; node; node = node->next )
		{
			Uint32 entityUid = *((Uint32*)node->element);
			Entity* child = uidToEntity(entityUid);
			if ( child )
			{
				++entitiesExisting;
				if ( child->flags[INVISIBLE] )
				{
					++entitiesInvisible;
				}
				else
				{
					++entitiesVisible;
				}
			}
		}
		if ( entitiesExisting == 0 )
		{
			if ( textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_ATTACHED_ISREMOVED )
			{
				textSourceScript.setScriptType(textSourceIsScript, textSourceScript.SCRIPT_ATTACHED_FIRED);
				powered = true;
			}
		}
		else
		{
			if ( textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_ATTACHED_EXISTS )
			{
				textSourceScript.setScriptType(textSourceIsScript, textSourceScript.SCRIPT_ATTACHED_FIRED);
				powered = true;
			}
			else if ( textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_ATTACHED_INVIS
				&& entitiesInvisible == entitiesExisting )
			{
				textSourceScript.setScriptType(textSourceIsScript, textSourceScript.SCRIPT_ATTACHED_FIRED);
				powered = true;
			}
			else if ( textSourceScript.getTriggerType(textSourceIsScript) == textSourceScript.TRIGGER_ATTACHED_VISIBLE
				&& entitiesVisible == entitiesExisting )
			{
				textSourceScript.setScriptType(textSourceIsScript, textSourceScript.SCRIPT_ATTACHED_FIRED);
				powered = true;
			}
		}
	}
	else if ( textSourceScript.getScriptType(textSourceIsScript) == textSourceScript.SCRIPT_ATTACHED_FIRED )
	{
		powered = true;
	}

	if ( textSourceScript.getScriptType(textSourceIsScript) != textSourceScript.NO_SCRIPT )
	{
		if ( ticks <= 2 )
		{
			return;
		}
	}

	if ( powered )
	{
		// received power
		if ( textSourceDelay > 0 )
		{
			--textSourceDelay;
			return;
		}
		else
		{
			textSourceDelay = (textSourceVariables4W >> 16) & 0xFFFF;
		}
		if ( (textSourceVariables4W & 0xFF) == 0 )
		{
			textSourceVariables4W |= 1;

			std::string output = textSourceScript.getScriptFromEntity(*this);

			Uint32 color = makeColorRGB((textSourceColorRGB >> 16) & 0xFF, (textSourceColorRGB >> 8) & 0xFF,
				(textSourceColorRGB >> 0) & 0xFF);

			if ( textSourceIsScript != textSourceScript.NO_SCRIPT )
			{
				textSourceScript.handleTextSourceScript(*this, output);
				return;
			}

			size_t foundPlayerRef = output.find("@p");
			if ( foundPlayerRef != std::string::npos )
			{
				output.erase(foundPlayerRef, 2);
				output.insert(foundPlayerRef, "%s");
			}

			size_t foundDistanceRequirement = output.find("@d");
			int distanceRequirement = -1;
			if ( foundDistanceRequirement != std::string::npos )
			{
				output.erase(foundDistanceRequirement, 2);
				std::string distance;
				while ( foundDistanceRequirement < output.length() 
					&& output.at(foundDistanceRequirement) != ' '
					&& output.at(foundDistanceRequirement) != '\0'
					)
				{
					// usage: Hello @p @d 123 will send to distance 123 units away and send message "Hello player "
					distance = distance + output.at(foundDistanceRequirement);
					++foundDistanceRequirement;
				}
				distanceRequirement = std::stoi(distance);
				output.erase(output.find(distance), distance.length());
			}

			size_t foundInputTag = output.find("@in=");
			while ( foundInputTag != std::string::npos )
			{
				output.erase(foundInputTag, strlen("@in="));
				std::string impulseStr;
				size_t inputTagStrIndex = foundInputTag;
				while ( inputTagStrIndex < output.length()
					&& output.at(inputTagStrIndex) != ' '
					&& output.at(inputTagStrIndex) != '.'
					&& output.at(inputTagStrIndex) != ','
					&& output.at(inputTagStrIndex) != '\0'
					)
				{
					// usage: @in=IN_USE will get the input key for use
					impulseStr = impulseStr + output.at(inputTagStrIndex);
					++inputTagStrIndex;
				}
				output.erase(output.find(impulseStr), impulseStr.length());
				std::string inputFormatted;
				inputFormatted.append("[");
				const char* binding = Input::inputs[0].binding(impulseStr.c_str());
				inputFormatted.append(binding[0] == '\0' ? impulseStr : binding);
				inputFormatted.append("]");
				output.insert(foundInputTag, inputFormatted.c_str());
				foundInputTag = output.find("@in=");
			}

			size_t found = output.find("\\n");
			while ( found != std::string::npos )
			{
				output.erase(found, 2);
				output.insert(found, 1, '\n');
				found = output.find("\\n");
			}

			char buf[256] = "";
			strcpy(buf, output.c_str());

			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( !client_disconnected[c] )
				{
					if ( distanceRequirement != -1 && !(players[c] && players[c]->entity && entityDist(this, players[c]->entity) <= distanceRequirement) )
					{
						// not in range.
					}
					else
					{
						if ( foundPlayerRef != std::string::npos && stats[c] )
						{
							messagePlayerColor(c, MESSAGE_MISC, color, buf, stats[c]->name);
						}
						else
						{
							messagePlayerColor(c, MESSAGE_MISC, color, buf);
						}
					}
				}
			}
		}
	}
	else if ( !powered )
	{
		textSourceDelay = (textSourceVariables4W >> 16) & 0xFFFF;
		if ( (textSourceVariables4W & 0xFF) == 1 && ((textSourceVariables4W >> 8) & 0xFF) == 0 )
		{
			textSourceVariables4W -= 1;
		}
	}
}

void TextSourceScript::updateClientInformation(int player, bool clearInventory, bool clearStats, ClientInformationType updateType)
{
	if ( multiplayer != SERVER )
	{
		return;
	}
	if ( !stats[player] || client_disconnected[player] || players[player]->isLocalPlayer() )
	{
		return;
	}

	if ( updateType == CLIENT_UPDATE_ALL )
	{
		// update client attributes
		strcpy((char*)net_packet->data, "SCRU");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = (Sint8)stats[player]->STR;
		net_packet->data[6] = (Sint8)stats[player]->DEX;
		net_packet->data[7] = (Sint8)stats[player]->CON;
		net_packet->data[8] = (Sint8)stats[player]->INT;
		net_packet->data[9] = (Sint8)stats[player]->PER;
		net_packet->data[10] = (Sint8)stats[player]->CHR;
		net_packet->data[11] = (Uint8)stats[player]->EXP;
		net_packet->data[12] = (Uint8)stats[player]->LVL;
		SDLNet_Write16((Sint16)stats[player]->HP, &net_packet->data[13]);
		SDLNet_Write16((Sint16)stats[player]->MAXHP, &net_packet->data[15]);
		SDLNet_Write16((Sint16)stats[player]->MP, &net_packet->data[17]);
		SDLNet_Write16((Sint16)stats[player]->MAXMP, &net_packet->data[19]);
		SDLNet_Write32((Sint32)stats[player]->GOLD, &net_packet->data[21]);
		if ( clearInventory )
		{
			net_packet->data[25] = 1;
		}
		else
		{
			net_packet->data[25] = 0;
		}
		if ( clearStats )
		{
			net_packet->data[26] = 1;
		}
		else
		{
			net_packet->data[26] = 0;
		}

		for ( int i = 0; i < NUMPROFICIENCIES; ++i )
		{
			net_packet->data[27 + i] = (Uint8)stats[player]->getProficiency(i);
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 27 + NUMPROFICIENCIES;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		serverUpdatePlayerLVL();
	}
	else if ( updateType == CLIENT_UPDATE_CLASS )
	{
		// unused
		/*strcpy((char*)net_packet->data, "SCRC");
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			net_packet->data[4 + i] = client_classes[i];
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 5 + MAXPLAYERS;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);*/
	}
	else if ( updateType == CLIENT_UPDATE_HUNGER )
	{
		serverUpdateHunger(player);
	}
}

void TextSourceScript::playerClearInventory(bool clearStats)
{
	players[clientnum]->hud.reset();
	deinitShapeshiftHotbar(clientnum);
	for ( int c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
	{
		players[clientnum]->hotbar.hotbarShapeshiftInit[c] = false;
	}
	players[clientnum]->magic.clearSelectedSpells(); //So you don't start off with a spell when the game restarts.
	spellcastingAnimationManager_deactivate(&cast_animation[clientnum]);
	stats[clientnum]->freePlayerEquipment();
	list_FreeAll(&stats[clientnum]->inventory);
	players[clientnum]->shootmode = true;
	players[clientnum]->inventoryUI.appraisal.timer = 0;
	players[clientnum]->inventoryUI.appraisal.current_item = 0;

	if ( clearStats )
	{
		stats[clientnum]->clearStats();
	}

	this->hasClearedInventory = true;
}

std::string TextSourceScript::getScriptFromEntity(Entity& src)
{
	// assemble the string.
	char buf[256] = "";
	int totalChars = 0;
	for ( int i = 4; i < 60; ++i )
	{
		if ( i == 28 ) // circuit_status
		{
			continue;
		}
		if ( src.skill[i] != 0 )
		{
			for ( int c = 0; c < 4; ++c )
			{
				buf[totalChars] = static_cast<char>((src.skill[i] >> (c * 8)) & 0xFF);
				++totalChars;
			}
		}
	}
	if ( buf[totalChars] != '\0' )
	{
		buf[totalChars] = '\0';
	}

	if ( buf[0] == '$' )
	{
		// try to replace script with data file entry
		std::string key = "";
		for ( int i = 0; i <= totalChars; ++i )
		{
			char c = buf[i];
			if ( c == '$' ) { continue; }
			if ( charIsWordSeparator(c) ) { break; }
			key += c;
		}
		if ( ScriptTextParser.allEntries.find(key) != ScriptTextParser.allEntries.end() )
		{
			return ScriptTextParser.allEntries[key].formattedText;
		}
	}
	return buf;
}

void TextSourceScript::parseScriptInMapGeneration(Entity& src)
{
	std::string script = getScriptFromEntity(src);

	size_t foundScriptTag = script.find("@script");
	if ( foundScriptTag != std::string::npos )
	{
		if ( (foundScriptTag + strlen("@script")) < script.length()
			&& script.at(foundScriptTag + strlen("@script")) == ' ' )
		{
			script.erase(foundScriptTag, strlen("@script") + 1); // trailing space.
		}
		else
		{
			script.erase(foundScriptTag, strlen("@script"));
		}
		textSourceScript.setScriptType(src.textSourceIsScript, textSourceScript.SCRIPT_NORMAL);
		textSourceScript.setTriggerType(src.textSourceIsScript, textSourceScript.TRIGGER_POWER);
	}
	if ( src.textSourceIsScript == NO_SCRIPT )
	{
		return;
	}

	if ( script.find("@triggerif=") != std::string::npos )
	{
		int result = textSourceProcessScriptTag(script, "@triggerif=", src);
		if ( result != k_ScriptError )
		{
			textSourceScript.setTriggerType(src.textSourceIsScript, static_cast<ScriptTriggeredBy>(result));
		}
	}

	if ( script.find("@attachto=") != std::string::npos )
	{
		int attachTo = textSourceProcessScriptTag(script, "@attachto=", src);
		if ( attachTo == k_ScriptError )
		{
			return;
		}
		textSourceScript.setScriptType(src.textSourceIsScript, textSourceScript.SCRIPT_ATTACHED);
		int x1 = static_cast<int>(src.x / 16); // default to just whatever this script is sitting on.
		int x2 = static_cast<int>(src.x / 16);
		int y1 = static_cast<int>(src.y / 16);
		int y2 = static_cast<int>(src.y / 16);
		// offset for generated dungeons
		x1 += src.mapGenerationRoomX;
		x2 += src.mapGenerationRoomX;
		y1 += src.mapGenerationRoomY;
		y2 += src.mapGenerationRoomY;
		if ( script.find("@attachrange=") != std::string::npos )
		{
			int result = textSourceProcessScriptTag(script, "@attachrange=", src);
			if ( result != k_ScriptError )
			{
				x1 = result & 0xFF;
				x2 = (result >> 8) & 0xFF;
				y1 = (result >> 16) & 0xFF;
				y2 = (result >> 24) & 0xFF;
			}
		}
		textSourceScript.setAttachedToEntityType(src.textSourceIsScript, attachTo);
		for ( node_t* node = map.entities->first; node; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity )
			{
				if ( (entity->behavior == &actMonster && attachTo == TO_MONSTERS)
					|| (entity->behavior == &actPlayer && attachTo == TO_PLAYERS)
					|| (entity->behavior == &actItem && attachTo == TO_ITEMS)
					|| (entity->behavior == &actMonster 
						&& attachTo >= TO_NOTHING && attachTo <= TO_DUMMYBOT 
						&& entity->getRace() == (attachTo - TO_NOTHING)) )
				{
					// found our entity.
				}
				else
				{
					continue;
				}
				int findx = static_cast<int>(entity->x) >> 4;
				int findy = static_cast<int>(entity->y) >> 4;
				if ( findx >= x1 && findx <= x2 && findy >= y1 && findy <= y2 )
				{
					node_t* node = list_AddNodeLast(&src.children);
					node->deconstructor = &defaultDeconstructor;
					Uint32* entityUid = (Uint32*)(malloc(sizeof(Uint32)));
					node->element = entityUid;
					node->size = sizeof(Uint32);
					*entityUid = entity->getUID();
				}
			}
		}
	}
}

void bellAttractMonsters(Entity* my)
{
	if ( !my ) { return; }

	auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, decoyBoxRange);

	for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
	{
		list_t* currentList = *it;
		node_t* node;
		for ( node = currentList->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader() == nullptr )
			{
				if ( (entity->monsterState == MONSTER_STATE_WAIT || entity->monsterTarget == 0) )
				{
					Stat* myStats = entity->getStats();
					if ( !entity->isBossMonster() && !entity->monsterIsTinkeringCreation()
						&& entity->isMobile()
						&& myStats
						&& entityDist(my, entity) > TOUCHRANGE )
					{
						if ( !myStats->EFFECTS[EFF_DISTRACTED_COOLDOWN]
							&& entity->monsterSetPathToLocation(my->x / 16, my->y / 16, 1,
								GeneratePathTypes::GENERATE_PATH_DEFAULT, true) && entity->children.first )
						{
							entity->monsterTarget = my->getUID();
							entity->monsterState = MONSTER_STATE_HUNT; // hunt state
							serverUpdateEntitySkill(entity, 0);
							if ( entity->setEffect(EFF_DISTRACTED_COOLDOWN, true, TICKS_PER_SECOND * 5, false) )
							{
								spawnFloatingSpriteMisc(134, entity->x + (-4 + local_rng.rand() % 9) + cos(entity->yaw) * 2,
									entity->y + (-4 + local_rng.rand() % 9) + sin(entity->yaw) * 2, entity->z + local_rng.rand() % 4);
							}
						}
					}
				}
			}
		}
	}
}

#define BELL_ACTIVE_TIMER my->skill[0]
#define BELL_HAS_ITEM my->skill[1]
#define BELL_AMBIENCE my->skill[3]
#define BELL_USES my->skill[4]
#define BELL_CURRENT_EVENT my->skill[5]
#define BELL_LAST_TOUCHED_PLAYER my->skill[6]
#define BELL_USE_DELAY my->skill[7]
#define BELL_INIT my->skill[8]
#define BELL_CLAPPER_BROKEN my->skill[9]
#define BELL_BULB_BROKEN my->skill[10]
#define BELL_BUFF_TYPE my->skill[11]
#define BELL_BURNING_TIMER my->skill[12]
#define BELL_PULLED_TO_BREAK my->skill[13]

int getBellDmgOnEntity(Entity* entity)
{
	if ( !entity ) { return 0; }

	Stat* stats = entity->getStats();
	if ( !stats )
	{
		return 0;
	}

	int damage = 80;
	int trapResist = entity->getFollowerBonusTrapResist();
	if ( trapResist != 0 )
	{
		real_t mult = std::max(0.0, 1.0 - (trapResist / 100.0));
		damage *= mult;
	}

	if ( stats->helmet )
	{
		bool shapeshifted = (entity->behavior == &actPlayer && entity->effectShapeshift != NOTHING);

		if ( !shapeshifted
			&& (stats->helmet->type == HELM_MINING || stats->helmet->type == HAT_TOPHAT) )
		{
			if ( stats->helmet->type == HAT_TOPHAT )
			{
				bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats);
				if ( stats->helmet->beatitude >= 0 || cursedItemIsBuff )
				{
					if ( stats->HP <= damage )
					{
						// saved us
						//steamAchievementEntity(entity, "BARONY_ACH_CRUMPLE_ZONES");
					}
					damage = 0;
				}
				stats->helmet->status = BROKEN;
			}
			else if ( stats->helmet->type == HELM_MINING )
			{
				real_t mult = 0.5;
				bool cursedItemIsBuff = shouldInvertEquipmentBeatitude(stats);
				if ( stats->helmet->beatitude >= 0 || cursedItemIsBuff )
				{
					mult -= 0.25 * abs(stats->helmet->beatitude);
					mult = std::max(0.0, mult);
				}
				else
				{
					mult = 1.0;
					mult += 0.25 * abs(stats->helmet->beatitude);
				}

				if ( stats->HP <= damage )
				{
					// saved us
					if ( stats->HP > (damage * mult) )
					{
						//steamAchievementEntity(entity, "BARONY_ACH_CRUMPLE_ZONES");
					}
				}
				damage *= mult;
				if ( stats->helmet->status > BROKEN )
				{
					stats->helmet->status = (Status)((int)stats->helmet->status - 1);
				}
			}

			playSoundEntity(entity, 76, 64);

			if ( entity->behavior == &actPlayer )
			{
				int player = entity->skill[2];
				if ( stats->helmet->status > BROKEN )
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(681), stats->helmet->getName());
				}
				else
				{
					messagePlayer(player, MESSAGE_EQUIPMENT, Language::get(682), stats->helmet->getName());
				}

				if ( multiplayer == SERVER && player > 0 && !players[player]->isLocalPlayer() )
				{
					strcpy((char*)net_packet->data, "ARMR");
					net_packet->data[4] = 0;
					net_packet->data[5] = stats->helmet->status;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
			}
		}
	}

	return damage;
}

void spawnMagicEffectParticlesBell(Entity* my, Uint32 sprite)
{
	if ( !my ) { return; }
	int baseX = my->x / 16;
	int baseY = my->y / 16;

	real_t posx = baseX * 16.0 + 8;
	real_t posy = baseY * 16.0 + 8;
	real_t z = 8.0;
	const int numParticles = 64;
	for ( int c = 0; c < numParticles; c++ )
	{
		Entity* entity = newEntity(1479, 1, map.entities, nullptr); //Particle entity.
		entity->x = posx + 24.0 * cos(2 * PI * (c / (real_t)numParticles));
		entity->y = posy + 24.0 * sin(2 * PI * (c / (real_t)numParticles));
		entity->z = z;
		entity->scalex = 0.7;
		entity->scaley = 0.7;
		entity->scalez = 0.7;
		entity->sizex = 1;
		entity->sizey = 1;
		entity->yaw = (local_rng.rand() % 360) * PI / 180.f;
		entity->pitch = (local_rng.rand() % 360) * PI / 180.f;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[UNCLICKABLE] = true;
		entity->flags[INVISIBLE] = true;
		entity->flags[INVISIBLE_DITHER] = true;
		entity->lightBonus = vec4(0.25f, 0.25f,
			0.25f, 0.f);
		entity->behavior = &actMagicParticle;
		entity->vel_z = -0.5;
		if ( multiplayer != CLIENT )
		{
			entity_uids--;
		}
		entity->setUID(-3);
	}

	if ( multiplayer == SERVER )
	{
		for ( int c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] || players[c]->isLocalPlayer() )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "MAGB");
			SDLNet_Write32(my->getUID(), &net_packet->data[4]);
			SDLNet_Write32(sprite, &net_packet->data[8]);
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 12;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
}

void bellBreakBulb(Entity* my, bool minotaurBreak)
{
	if ( !my ) { return; }
	if ( BELL_BULB_BROKEN == 1 )
	{
		return;
	}
	BELL_BULB_BROKEN = 1;
	if ( minotaurBreak )
	{
		BELL_LAST_TOUCHED_PLAYER = -1;
	}

	Entity* bell = nullptr;
	for ( node_t* node = my->children.first; node; node = node->next )
	{
		if ( node->element != nullptr )
		{
			Entity* child = (Entity*)node->element;
			if ( child )
			{
				if ( child->sprite == 1475 && !child->flags[INVISIBLE] ) // bell
				{
					bell = child;
				}
			}
		}
	}

	if ( bell )
	{
		bellAttractMonsters(my);
		auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
		int dir = rng.rand() % 9;
		if ( dir == 8 )
		{
			bell->vel_x = 0.0;
			bell->vel_y = 0.0;
		}
		else
		{
			bell->vel_x = 0.25 * cos(my->yaw + dir * PI / 4);
			bell->vel_y = 0.25 * sin(my->yaw + dir * PI / 4);
		}
	}

	serverUpdateEntitySkill(my, 10);
	playSoundEntity(my, 76, 64);

	if ( minotaurBreak )
	{
		playSoundEntity(my, 689, 128);
		playSoundPlayer(clientnum, 689, 32);
		if ( multiplayer == SERVER )
		{
			for ( int i = 1; i < MAXPLAYERS; ++i )
			{
				playSoundPlayer(i, 689, 32);
			}
		}
	}

	if ( BELL_CLAPPER_BROKEN == 0 )
	{
		BELL_CLAPPER_BROKEN = 1;
		serverUpdateEntitySkill(my, 9);
	}
}

void actBell(Entity* my)
{
	if ( !my )
	{
		return;
	}

	enum BellSpriteNotes : int
	{
		NOTE_DOUBLE_EIGHTH = 192,
		NOTE_EIGHTH = 198,
		NOTE_REST = 199,
		NOTE_CRASH = 200
	};

	enum BellEvents : int
	{
		BELL_RING_BUFF = 1,
		BELL_MONSTER,
		BELL_ITEM,
		BELL_CLAPPER_BREAK,
		BELL_CRASH,
		BELL_NOTHING,
		BELL_ENUM_END
	};

	enum BellBuffs
	{
		BUFF_STR,
		BUFF_CON,
		BUFF_PWR,
		BUFF_DEX,
		BUFF_HEAL,
		BUFF_ENUM_END,
	};

	auto& rng = my->entity_rng ? *my->entity_rng : local_rng;
	if ( !BELL_INIT )
	{
		BELL_INIT = 1;

		{
			Entity* childEntity = newEntity(1476, 1, map.entities, nullptr); // clapper
			childEntity->parent = my->getUID();
			childEntity->x = my->x;
			childEntity->y = my->y;
			childEntity->sizex = 2;
			childEntity->sizey = 2;
			childEntity->flags[PASSABLE] = true;
			childEntity->flags[UNCLICKABLE] = false;
			childEntity->flags[NOUPDATE] = true;
			childEntity->z = my->z;
			if ( multiplayer != CLIENT )
			{
				entity_uids--;
			}
			childEntity->setUID(-3);
			node_t* tempNode = list_AddNodeLast(&my->children);
			tempNode->element = childEntity; // add the node to the children list.
			tempNode->deconstructor = &emptyDeconstructor;
			tempNode->size = sizeof(Entity*);
		}
		{
			Entity* childEntity = newEntity(1477, 1, map.entities, nullptr); // headstock
			childEntity->parent = my->getUID();
			childEntity->x = my->x;
			childEntity->y = my->y;
			childEntity->sizex = 4;
			childEntity->sizey = 4;
			childEntity->flags[PASSABLE] = true;
			childEntity->flags[UNCLICKABLE] = false;
			childEntity->flags[NOUPDATE] = true;
			childEntity->z = my->z;
			if ( multiplayer != CLIENT )
			{
				entity_uids--;
			}
			childEntity->setUID(-3);
			node_t* tempNode = list_AddNodeLast(&my->children);
			tempNode->element = childEntity; // add the node to the children list.
			tempNode->deconstructor = &emptyDeconstructor;
			tempNode->size = sizeof(Entity*);
		}
	}

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
		BELL_LAST_TOUCHED_PLAYER = -1;
	}

	my->z = 0;
	static ConsoleVariable<bool> cvar_bell_crash("/bell_crash", false);
#ifndef NDEBUG
	if ( keystatus[SDLK_KP_5] && enableDebugKeys )
	{
		keystatus[SDLK_KP_5];
		bellBreakBulb(my, true);
		//my->yaw += 0.01;
	}
#endif // !NDEBUG
	my->focalx = 4;
	my->focaly = -6;
	my->focalz = -10.75;

#ifdef USE_FMOD
	if ( BELL_AMBIENCE == 0 )
	{
		BELL_AMBIENCE--;
		my->stopEntitySound();
		my->entity_sound = playSoundEntityLocal(my, 149, 16);
	}
	if ( my->entity_sound )
	{
		bool playing = false;
		my->entity_sound->isPlaying(&playing);
		if ( !playing )
		{
			my->entity_sound = nullptr;
		}
	}
#else
	BELL_AMBIENCE--;
	if ( BELL_AMBIENCE <= 0 )
	{
		BELL_AMBIENCE = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(my, 149, 16);
	}
#endif

	const int pullTimerStart = 100;
#ifndef NDEBUG
	if ( keystatus[SDLK_KP_3] && enableDebugKeys )
	{
		keystatus[SDLK_KP_3] = 0;
		BELL_ACTIVE_TIMER = pullTimerStart; // active pull timer
	}

	if ( keystatus[SDLK_g] && enableDebugKeys && *cvar_bell_crash )
	{
		keystatus[SDLK_g] = 0;
		BELL_CURRENT_EVENT = (BellEvents)(BELL_CURRENT_EVENT + 1);
		if ( BELL_CURRENT_EVENT >= BELL_ENUM_END )
		{
			BELL_CURRENT_EVENT = BELL_RING_BUFF;
		}
		messagePlayer(0, MESSAGE_DEBUG, "Bell event: %d", BELL_CURRENT_EVENT);
	}
#endif

	if ( multiplayer == CLIENT )
	{
		// server encoded current bell event into the timer, separate it out
		Sint32 upperbits = (BELL_ACTIVE_TIMER >> 8) & 0xFF;
		if ( upperbits > 0 )
		{
			BELL_CURRENT_EVENT = upperbits;
		}
		BELL_ACTIVE_TIMER = (BELL_ACTIVE_TIMER & 0xFF);
	}

	Entity* touched = nullptr;
	if ( multiplayer != CLIENT && !my->flags[BURNING] && !my->flags[INVISIBLE] ) // interaction
	{
		if ( my->isInteractWithMonster() )
		{
			Entity* monsterInteracting = uidToEntity(my->interactedByMonster);
			if ( monsterInteracting )
			{
				my->clearMonsterInteract();
				if ( BELL_USE_DELAY <= 0 )
				{
					touched = monsterInteracting;
					if ( auto leader = monsterInteracting->monsterAllyGetPlayerLeader() )
					{
						BELL_LAST_TOUCHED_PLAYER = leader->skill[2];
					}
					else
					{
						BELL_LAST_TOUCHED_PLAYER = -1;
					}
				}
			}
			my->clearMonsterInteract();
		}

		for ( int i = 0; i < MAXPLAYERS; i++ )
		{
			if ( selectedEntity[i] == my || client_selected[i] == my )
			{
				if ( inrange[i] && Player::getPlayerInteractEntity(i) )
				{
					if ( BELL_USE_DELAY <= 0 )
					{
						touched = Player::getPlayerInteractEntity(i);
						BELL_LAST_TOUCHED_PLAYER = i;
						Compendium_t::Events_t::eventUpdateWorld(i, Compendium_t::CPDM_BELL_RUNG_TIMES, "bell", 1);
						break;
					}
				}
			}
		}
		if ( touched )
		{
			if ( BELL_CURRENT_EVENT == BELL_CRASH 
				|| BELL_CURRENT_EVENT == BELL_CLAPPER_BREAK
				|| BELL_CURRENT_EVENT == BELL_NOTHING )
			{
				BELL_CURRENT_EVENT = BELL_NOTHING;
			}
			else
			{
				if ( BELL_CURRENT_EVENT == 0 )
				{
					BELL_USES = 3 + rng.rand() % 2;
					if ( BELL_HAS_ITEM != 0 )
					{
						BELL_CURRENT_EVENT = BELL_ITEM;
						BELL_USES = 2 + rng.rand() % 3;
					}
					else
					{
						if ( rng.rand() % 4 == 0 )
						{
							// bats
							BELL_CURRENT_EVENT = BELL_MONSTER;
							BELL_USES = 2 + rng.rand() % 3;
						}
						else
						{
							BELL_CURRENT_EVENT = BELL_RING_BUFF;
						}
					}
				}
				else
				{
					BELL_CURRENT_EVENT = BELL_RING_BUFF;
					--BELL_USES;

					if ( *cvar_bell_crash && (svFlags & SV_FLAG_CHEATS) )
					{
						BELL_USES = 0;
						BELL_CURRENT_EVENT = BELL_CRASH;
					}
					else if ( BELL_USES <= 0 )
					{
						if ( rng.rand() % 5 == 0 )
						{
							BELL_CURRENT_EVENT = BELL_CRASH;
						}
						else
						{
							BELL_CURRENT_EVENT = BELL_CLAPPER_BREAK;
						}
					}

					if ( BELL_CURRENT_EVENT == BELL_CRASH )
					{
						BELL_PULLED_TO_BREAK = touched->getUID();
					}
				}
			}

			BELL_ACTIVE_TIMER = pullTimerStart;
			BELL_ACTIVE_TIMER |= (BELL_CURRENT_EVENT << 8);
			serverUpdateEntitySkill(my, 0);
			BELL_ACTIVE_TIMER = pullTimerStart;
			if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
			{
				messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6268));
			}

			if ( BELL_CURRENT_EVENT == BELL_RING_BUFF )
			{
				BELL_USE_DELAY = TICKS_PER_SECOND * 9;
			}
			else if ( BELL_CURRENT_EVENT == BELL_NOTHING )
			{
				BELL_USE_DELAY = TICKS_PER_SECOND * 3;
			}
			else
			{
				BELL_USE_DELAY = TICKS_PER_SECOND * 5;
			}
			serverUpdateEntitySkill(my, 7); // use delay
		}
	}
	else if ( multiplayer != CLIENT && my->flags[BURNING] )
	{
		if ( BELL_BURNING_TIMER == 0 )
		{
			BELL_BURNING_TIMER = (1 + local_rng.rand() % 3) * TICKS_PER_SECOND;
			playSoundEntity(my, 512, 64);
		}
		else if ( BELL_BURNING_TIMER > 0 )
		{
			BELL_BURNING_TIMER--;
			if ( BELL_BURNING_TIMER <= 0 )
			{
				BELL_BURNING_TIMER = -1;
				bellBreakBulb(my, true);
				my->flags[BURNING] = false;
				my->flags[INVISIBLE] = true;
				serverUpdateEntitySkill(my, INVISIBLE);
				serverUpdateEntitySkill(my, BURNING);

				if ( BELL_PULLED_TO_BREAK != 0 )
				{
					if ( Entity* puller = uidToEntity(BELL_PULLED_TO_BREAK) )
					{
						if ( puller->behavior == &actPlayer )
						{
							Compendium_t::Events_t::eventUpdateWorld(puller->skill[2], Compendium_t::CPDM_BELL_BROKEN, "bell", 1);
							steamStatisticUpdateClient(puller->skill[2], STEAM_STAT_RUNG_OUT, STEAM_STAT_INT, 1);
						}
					}
				}
			}
		}
	}

	if ( my->flags[INVISIBLE] && my->flags[BURNING] )
	{
		my->flags[BURNING] = false;
		if ( multiplayer != CLIENT )
		{
			serverUpdateEntitySkill(my, BURNING);
		}
	}

	if ( BELL_USE_DELAY > 0 )
	{
		--BELL_USE_DELAY;
	}
	if ( BELL_ACTIVE_TIMER == pullTimerStart )
	{
		playSoundEntityLocal(my, 688, 64);
	}

	if ( BELL_ACTIVE_TIMER > 0 )
	{
		--BELL_ACTIVE_TIMER;
	}
	else
	{
		BELL_ACTIVE_TIMER = 0;
	}

	bool bellEventTriggered = false;
	bool shortRing = (BELL_CURRENT_EVENT != BELL_RING_BUFF);
	bool startBellAnim = false;
	static ConsoleVariable<float> cvar_bell_max_spd("/bell_max_spd", 4.0);
	static ConsoleVariable<int> cvar_bell_clap_rot("/bell_clap_rot", 245);
	static ConsoleVariable<int> cvar_bell_anim_tick("/bell_anim_tick", 90);
	static ConsoleVariable<int> cvar_bell_pull_tick("/bell_pull_tick", 30);
	static ConsoleVariable<int> cvar_bell_dong1("/bell_dong1", 100);
	static ConsoleVariable<int> cvar_bell_dong2("/bell_dong2", 140);
	static ConsoleVariable<int> cvar_bell_dong3("/bell_dong3", 190);
	if ( my->skill[0] == *cvar_bell_anim_tick )
	{
		startBellAnim = true;
	}
	const int pullTimerFirstAnim = *cvar_bell_pull_tick;
	if ( my->skill[0] > pullTimerFirstAnim )
	{
		//const int interval = pullTimerStart - pullTimerFirstAnim;
		//my->focalz += -2 + (interval - (my->skill[0] - pullTimerFirstAnim)) * 2.0 / (interval / 2.0);

		const int interval = pullTimerStart - pullTimerFirstAnim;
		my->focalz += 2.0 * cos(PI * (interval - (my->skill[0] - pullTimerFirstAnim) / (real_t)interval));
	}
	else
	{
		//my->focalz += 2.0 * cos((-my->skill[0] + pullTimerFirstAnim) * PI / (real_t)pullTimerFirstAnim);
		
		const int interval = pullTimerFirstAnim;
		my->focalz += -2.0 + 4.0 * cos(1.5 * PI * (interval - my->skill[0]) / (real_t)interval);
	}

	const real_t baseZ = 0.0;

	Entity* bell = nullptr;
	Entity* clapper = nullptr;
	node_t* nextnode = nullptr;
	static ConsoleVariable<int> cvar_bell_crash_sfx("/bell_crash_sfx", 691);
	for ( node_t* node = my->children.first; node; node = nextnode )
	{
		nextnode = node->next;
		if ( node->element != nullptr )
		{
			Entity* child = (Entity*)node->element;
			if ( child )
			{
				if ( child->sprite == 1475 ) // bell
				{
					if ( child->flags[INVISIBLE] )
					{
						bell = nullptr;
						continue;
					}
					bell = child;
					if ( BELL_BULB_BROKEN )
					{
						child->vel_z += 0.04;
						child->z += child->vel_z;
						child->yaw += 0.15;
						real_t dist = clipMove(&child->x, &child->y, child->vel_x, child->vel_y, child);

						Entity* collided = nullptr;
						if ( multiplayer != CLIENT )
						{
							Entity* puller = uidToEntity(BELL_PULLED_TO_BREAK);
							auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(child, 2);
							for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end() && !collided; ++it )
							{
								list_t* currentList = *it;
								node_t* node;
								for ( node = currentList->first; node != nullptr && !collided; node = node->next )
								{
									Entity* entity = (Entity*)node->element;
									if ( !entity ) { continue; }

									if ( (entity->behavior == &actMonster && !(entity->getRace() == MIMIC)) 
										|| entity->behavior == &actPlayer )
									{
										Stat* stats = entity->getStats();
										if ( stats && entityInsideEntity(entity, child) )
										{
											if ( !(stats->type == MINOTAUR || child->z >= -16.0) )
											{
												continue;
											}

											if ( child->collisionIgnoreTargets.find(entity->getUID()) != child->collisionIgnoreTargets.end() )
											{
												continue;
											}
											child->collisionIgnoreTargets.insert(entity->getUID());

											if ( entity->behavior == &actPlayer )
											{
												const Uint32 color = makeColorRGB(255, 0, 0);
												messagePlayerColor(entity->skill[2], MESSAGE_STATUS, color, Language::get(6276));
												if ( players[entity->skill[2]]->isLocalPlayer() )
												{
													cameravars[entity->skill[2]].shakex += .1;
													cameravars[entity->skill[2]].shakey += 10;
												}
												else
												{
													if ( entity->skill[2] > 0 )
													{
														strcpy((char*)net_packet->data, "SHAK");
														net_packet->data[4] = 10; // turns into .1
														net_packet->data[5] = 10;
														net_packet->address.host = net_clients[entity->skill[2] - 1].host;
														net_packet->address.port = net_clients[entity->skill[2] - 1].port;
														net_packet->len = 6;
														sendPacketSafe(net_sock, -1, net_packet, entity->skill[2] - 1);
													}
												}
											}

											playSoundEntity(entity, 28, 64);
											Entity* gib = spawnGib(entity);
											int dmg = getBellDmgOnEntity(entity);
											Sint32 oldHP = stats->HP;
											entity->modHP(-dmg);
											if ( entity->behavior == &actPlayer )
											{
												if ( stats->HP < oldHP )
												{
													Compendium_t::Events_t::eventUpdateWorld(entity->skill[2], Compendium_t::CPDM_TRAP_DAMAGE, "bell", oldHP - stats->HP);
												}

												if ( !puller )
												{
													int playertarget = entity->skill[2];
													if ( playertarget >= 0 && players[playertarget]->isLocalPlayer() )
													{
														DamageIndicatorHandler.insert(playertarget, child->x, child->y, stats->HP < oldHP);
													}
													else if ( playertarget > 0 && multiplayer == SERVER && !players[playertarget]->isLocalPlayer() )
													{
														strcpy((char*)net_packet->data, "DAMI");
														SDLNet_Write32(child->x, &net_packet->data[4]);
														SDLNet_Write32(child->y, &net_packet->data[8]);
														net_packet->data[12] = (stats->HP < oldHP) ? 1 : 0;
														net_packet->address.host = net_clients[playertarget - 1].host;
														net_packet->address.port = net_clients[playertarget - 1].port;
														net_packet->len = 13;
														sendPacketSafe(net_sock, -1, net_packet, playertarget - 1);
													}
												}
											}
											entity->setObituary(Language::get(6277));
											stats->killer = KilledBy::BELL;

											
											if ( !strcmp(stats->name, "") )
											{
												updateEnemyBar(puller, entity, getMonsterLocalizedName(stats->type).c_str(), stats->HP, stats->MAXHP,
													false, DamageGib::DMG_STRONGEST);
											}
											else
											{
												updateEnemyBar(puller, entity, stats->name, stats->HP, stats->MAXHP,
													false, DamageGib::DMG_STRONGEST);
											}

											if ( stats->HP <= 0 && oldHP > 0 )
											{
												if ( entity->behavior == &actPlayer )
												{
													Compendium_t::Events_t::eventUpdateWorld(entity->skill[2], Compendium_t::CPDM_TRAP_KILLED_BY, "bell", 1);
												}
												if ( puller && puller != entity )
												{
													puller->awardXP(entity, true, true);
													if ( puller->behavior == &actPlayer )
													{
														messagePlayerMonsterEvent(puller->skill[2], makeColorRGB(0, 255, 0), *stats, Language::get(692), Language::get(697), MSG_COMBAT);
														if ( stats->type == GNOME )
														{
															steamAchievementClient(puller->skill[2], "BARONY_ACH_JUBBAITED");
														}
													}
												}
											}

											if ( stats->type == MINOTAUR )
											{
												collided = entity; // break this thing over its head
											}
											break;
										}
									}
								}
							}
						}
						if ( child->z >= -3.5 || collided )
						{
							if ( multiplayer != CLIENT )
							{
								for ( int i = 0; i < 6; ++i )
								{
									Entity* dropped = dropItemMonster(newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 2 + rng.rand() % 3, 0, true, nullptr), child, nullptr, 1);
									if ( dropped )
									{
										dropped->z = child->z + child->focalz;
									}
								}
								playSoundEntity(my, *cvar_bell_crash_sfx, 128);
								playSoundPlayer(clientnum, *cvar_bell_crash_sfx, 32);
								if ( multiplayer == SERVER )
								{
									for ( int i = 1; i < MAXPLAYERS; ++i )
									{
										playSoundPlayer(i, *cvar_bell_crash_sfx, 32);
									}
								}

								spawnDamageGib(child, NOTE_CRASH, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE, true);
							}
							child->flags[INVISIBLE] = true;
							if ( multiplayer == SERVER )
							{
								serverUpdateEntityFlag(child, INVISIBLE);
							}
							bell = nullptr;
						}
						continue;
					}

					child->yaw = my->yaw;
					child->z = baseZ - 22.25;
					child->focalz = 6;
					child->x = my->x - 2 * cos(child->yaw);
					child->y = my->y - 2 * sin(child->yaw);

					Sint32& dongs = child->skill[1];
					real_t& rotation = child->fskill[1];
					Sint32& endDamp = child->skill[5];

					Entity* clapper = nullptr;
					if ( nextnode )
					{
						clapper = (Entity*)nextnode->element;
					}
					if ( startBellAnim )
					{
						// target pitch
						child->fskill[0] = PI / 8;
						child->pitch = 0.0;
						dongs = 0;
						rotation = 0.0;
						child->skill[3] = ticks;
						endDamp = 0;

						if ( clapper && BELL_CLAPPER_BROKEN == 0 )
						{
							clapper->pitch = 0.0;
							clapper->skill[1] = 0; // dongs
							clapper->skill[4] = 0; // rotation
							clapper->skill[5] = 0; // end damp
							clapper->fskill[0] = 0.0;
						}
						if ( clapper )
						{
							clapper->skill[6] = 0; // clapper active
						}
					}

					if ( abs(child->fskill[0]) > 0.0001 )
					{
						real_t oldPitch = child->pitch;
						child->fskill[0] = (1 / (real_t)((1 + std::max(0, (dongs - 1))))) * PI / 8;
						if ( dongs >= 3 || (shortRing && dongs >= 1) )
						{
							endDamp = std::min(endDamp + 1, 100);
							child->fskill[0] *= (float)(100 - endDamp) / 100.f;
						}

						if ( (int)(*cvar_bell_max_spd * rotation) >= *cvar_bell_clap_rot )
						{
							if ( clapper && clapper->skill[6] == 0 )
							{
								if ( BELL_CLAPPER_BROKEN == 0 )
								{
									if ( shortRing )
									{
										playSoundEntityLocal(my, 689, 128);
										playSoundPlayer(clientnum, 689, 32);
									}
								}
								bellEventTriggered = true;
								clapper->skill[6] = 1;
							}
						}

						real_t speed = *cvar_bell_max_spd * rotation;
						child->pitch = child->fskill[0] * sin((speed) * PI / 180.f);
						if ( child->pitch < -0.0 && oldPitch >= 0.0 )
						{
							++dongs;
						}
						if ( rotation < 15.0 )
						{
							rotation += 0.25;
						}
						else
						{
							rotation += 1.0;
						}

						if ( shortRing )
						{
							if ( clapper && BELL_CLAPPER_BROKEN == 0 )
							{
								if ( (ticks - child->skill[3] == *cvar_bell_dong1) )
								{
									spawnDamageGib(child, NOTE_REST, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE);
								}
							}
						}
						else if ( ((ticks - child->skill[3]) == *cvar_bell_dong1) 
							|| ((ticks - child->skill[3]) == *cvar_bell_dong2)
							|| ((ticks - child->skill[3]) == *cvar_bell_dong3) )
						{
							if ( clapper && BELL_CLAPPER_BROKEN == 0 )
							{
								int vol = 128;
								if ( (ticks - child->skill[3]) == *cvar_bell_dong2 )
								{
									vol = 92;
								}
								else if ( ((ticks - child->skill[3]) == *cvar_bell_dong3) )
								{
									vol = 64;
								}
								playSoundEntity(my, 690, vol);
								playSoundPlayer(clientnum, 690, vol / 4);
								spawnDamageGib(child, NOTE_EIGHTH, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE);
							}
						}
					}
					else
					{
						child->pitch = 0.0;
						child->fskill[0] = 0.0;
						dongs = 0;
						rotation = 0.0;
						endDamp = 0;
					}
					while ( child->pitch >= PI )
					{
						child->pitch -= PI;
					}
					while ( child->pitch < -PI )
					{
						child->pitch += PI;
					}
				}
				else if ( child->sprite == 1476 ) // clapper
				{
					if ( child->flags[INVISIBLE] )
					{
						clapper = nullptr;
						continue;
					}
					child->yaw = my->yaw;
					clapper = child;
					if ( BELL_CLAPPER_BROKEN == 1 ) // broken
					{
						if ( child->focalz > 5 )
						{
							child->z = baseZ - 10;
						}
						child->focalz = 0;
						bool onground = false;
						real_t groundheight = 7.5;
						const real_t yawSpeed = 0.2;
						const real_t pitchSpeed = 0.1;
						if ( child->z < groundheight )
						{
							child->vel_z += 0.04;
							child->z += child->vel_z;
							child->pitch += pitchSpeed;
							child->yaw += yawSpeed;
						}
						else
						{
							if ( child->x >= 0 && child->y >= 0 && child->x < map.width << 4 && child->y < map.height << 4 )
							{
								const int tile = map.tiles[(int)(child->y / 16) * MAPLAYERS + (int)(child->x / 16) * MAPLAYERS * map.height];
								if ( tile )
								{
									onground = true;

									child->vel_z *= -.35; // bounce
									if ( child->vel_z > -.35 )
									{
										child->z = groundheight;
										child->vel_z = 0.0;
										child->pitch = -PI / 2;
									}
									else
									{
										// just bounce off the ground.
										child->z = groundheight - .0001;
									}
								}
								else
								{
									// fall (no ground here)
									child->vel_z += 0.04;
									child->z += child->vel_z;
									child->pitch += pitchSpeed;
									child->yaw += yawSpeed;
								}
							}
							else
							{
								// fall (out of bounds)
								child->vel_z += 0.04;
								child->z += child->vel_z;
								child->pitch += pitchSpeed;
								child->yaw += yawSpeed;
							}
						}

						// falling out of the map
						if ( child->z > 128 )
						{
							child->flags[INVISIBLE] = true;
							clapper = nullptr;
						}
						continue;
					}

					child->z = baseZ - 20;
					child->x = my->x - 2 * cos(child->yaw);
					child->y = my->y - 2 * sin(child->yaw);
					child->focalz = 10;
					auto& dongs = child->skill[1];
					Sint32& rotation = child->skill[4];
					Sint32& endDamp = child->skill[5];
					if ( child->skill[6] == 1 )
					{
						child->skill[6] = 2;
						child->fskill[0] = PI / 8;
					}

					if ( abs(child->fskill[0]) > 0.0001 )
					{
						real_t oldPitch = child->pitch;
						child->fskill[0] = -(1 / (real_t)((1 + std::max(0, (dongs - 1))))) * PI / 8;
						if ( dongs >= 3 || (shortRing && dongs >= 1) )
						{
							endDamp = std::min(endDamp + 1, 100);
							child->fskill[0] *= (float)(100 - endDamp) / 100.f;
						}
						child->pitch = child->fskill[0] * sin((*cvar_bell_max_spd * rotation) * PI / 180.f);
						if ( child->pitch < -0.0 && oldPitch >= 0.0 )
						{
							++dongs;
						}
						++rotation;
					}
					else
					{
						child->pitch = 0.0;
						child->fskill[0] = 0.0;
						dongs = 0;
						rotation = 0;
						endDamp = 0;
					}

					while ( child->pitch >= PI )
					{
						child->pitch -= PI;
					}
					while ( child->pitch < -PI )
					{
						child->pitch += PI;
					}
				}
				else if ( child->sprite == 1477 ) // headstock
				{
					child->yaw = my->yaw;
					child->z = baseZ - 21.75;
				}
			}
		}
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( BELL_ACTIVE_TIMER == 1 && BELL_BULB_BROKEN )
	{
		messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6274));
		return;
	}

	if ( bellEventTriggered )
	{
		if ( BELL_BULB_BROKEN )
		{
			messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6274));
			return;
		}
		else if ( BELL_CLAPPER_BROKEN )
		{
			messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6273));
			return;
		}

		if ( BELL_CURRENT_EVENT == BELL_RING_BUFF )
		{
			bellAttractMonsters(my);
			spawnMagicEffectParticlesBell(my, 1479);

			auto entLists = TileEntityList.getEntitiesWithinRadiusAroundEntity(my, 2);
			for ( std::vector<list_t*>::iterator it = entLists.begin(); it != entLists.end(); ++it )
			{
				list_t* currentList = *it;
				node_t* node;
				for ( node = currentList->first; node != nullptr; node = node->next )
				{
					Entity* entity = (Entity*)node->element;
					if ( (entity->behavior == &actMonster && !(entity->getRace() == MIMIC))
						|| entity->behavior == &actPlayer )
					{
						if ( entityDist(my, entity) <= 26.0 )
						{
							const char* lang = nullptr;
							int statusEffect = 0;
							Compendium_t::EventTags tag = Compendium_t::EventTags::CPDM_EVENT_TAGS_MAX;
							switch ( BELL_BUFF_TYPE % BellBuffs::BUFF_ENUM_END )
							{
							case BUFF_STR:
								lang = Language::get(6281);
								statusEffect = EFF_POTION_STR;
								tag = Compendium_t::EventTags::CPDM_BELL_BUFFS_STRENGTH;
								break;
							case BUFF_CON:
								lang = Language::get(6282);
								statusEffect = EFF_CON_BONUS;
								tag = Compendium_t::EventTags::CPDM_BELL_BUFFS_STAMINA;
								break;
							case BUFF_DEX:
								lang = Language::get(6283);
								statusEffect = EFF_AGILITY;
								tag = Compendium_t::EventTags::CPDM_BELL_BUFFS_AGILITY;
								break;
							case BUFF_PWR:
								lang = Language::get(6284);
								statusEffect = EFF_PWR;
								tag = Compendium_t::EventTags::CPDM_BELL_BUFFS_MENTALITY;
								break;
							case BUFF_HEAL:
								statusEffect = SPELL_HEALING;
								tag = Compendium_t::EventTags::CPDM_BELL_BUFFS_HEALS;
								break;
							default:
								break;
							}
							int duration = TICKS_PER_SECOND * 60;
							if ( statusEffect > 0 )
							{
								if ( Stat* stats = entity->getStats() )
								{
									if ( statusEffect == SPELL_HEALING )
									{
										int amount = 15;
										entity->modHP(amount);
										entity->modMP(amount);
										if ( svFlags & SV_FLAG_HUNGER )
										{
											if ( entity->behavior == &actPlayer && stats->playerRace == RACE_INSECTOID && stats->stat_appearance == 0 )
											{
												Sint32 hungerPointPerMana = entity->playerInsectoidHungerValueOfManaPoint(*stats);
												stats->HUNGER += amount * hungerPointPerMana;
												stats->HUNGER = std::min(999, stats->HUNGER);
												serverUpdateHunger(entity->skill[2]);
											}
										}
										playSoundEntity(entity, 168, 128);
										spawnDamageGib(entity, NOTE_DOUBLE_EIGHTH, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE, true);
										spawnMagicEffectParticles(entity->x, entity->y, entity->z, 169);
										if ( entity->behavior == &actPlayer )
										{
											messagePlayerColor(entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6298));
											Compendium_t::Events_t::eventUpdateWorld(entity->skill[2], tag, "bell", 1);
										}
									}
									else if ( entity->setEffect(statusEffect, true, std::max(stats->EFFECTS_TIMERS[statusEffect], duration), false) )
									{
										playSoundEntity(entity, 166, 128);
										spawnDamageGib(entity, NOTE_DOUBLE_EIGHTH, DamageGib::DMG_STRONGEST, DamageGibDisplayType::DMG_GIB_SPRITE, true);
										if ( entity->behavior == &actPlayer )
										{
											messagePlayerColor(entity->skill[2], MESSAGE_STATUS, makeColorRGB(0, 255, 0), Language::get(6280), lang);
											Compendium_t::Events_t::eventUpdateWorld(entity->skill[2], tag, "bell", 1);
										}
									}
								}
							}
						}

					}
				}
			}
		}
		else if ( BELL_CURRENT_EVENT == BELL_CRASH )
		{
			if ( bell && BELL_BULB_BROKEN == 0 )
			{
				messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6275));
				Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_BROKEN, "bell", 1);
				bellBreakBulb(my, false);
				steamStatisticUpdateClient(BELL_LAST_TOUCHED_PLAYER, STEAM_STAT_RUNG_OUT, STEAM_STAT_INT, 1);
			}
		}
		else if ( BELL_CURRENT_EVENT == BELL_CLAPPER_BREAK )
		{
			if ( clapper && BELL_CLAPPER_BROKEN == 0 )
			{
				BELL_CLAPPER_BROKEN = 1;
				serverUpdateEntitySkill(my, 9);
				playSoundEntity(my, 76, 64);
				messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6279));
				bellAttractMonsters(my);
				Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_CLAPPER_BROKEN, "bell", 1);
				steamStatisticUpdateClient(BELL_LAST_TOUCHED_PLAYER, STEAM_STAT_RUNG_OUT, STEAM_STAT_INT, 1);
			}
		}
		else if ( BELL_CURRENT_EVENT == BELL_MONSTER )
		{
			int successes = 0;
			Monster type = BAT_SMALL;
			std::vector<Entity*> enemies;
			for ( int i = 0; i < 4; ++i )
			{
				if ( Entity* monster = summonMonster(type, ((int)(my->x / 16)) * 16 + 8, ((int)(my->y / 16)) * 16 + 8) )
				{
					++successes;
					if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
					{
						if ( players[BELL_LAST_TOUCHED_PLAYER]->entity )
						{
							monster->monsterAcquireAttackTarget(*players[BELL_LAST_TOUCHED_PLAYER]->entity, MONSTER_STATE_PATH);
						}
					}
				}
			}

			if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
			{
				if ( successes > 0 )
				{
					Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_LOOT_BATS, "bell", successes);
				}
				if ( successes == 1 )
				{
					messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6234),
						getMonsterLocalizedName((Monster)type).c_str(), Language::get(6269));
				}
				else if ( successes > 1 )
				{
					messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6253),
						getMonsterLocalizedPlural((Monster)type).c_str(), Language::get(6269));
				}
			}

			if ( rng.rand() % 8 == 0 )
			{
				// sometimes it just falls straight after
				if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
				{
					messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6275));
					Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_BROKEN, "bell", 1);
					steamStatisticUpdateClient(BELL_LAST_TOUCHED_PLAYER, STEAM_STAT_RUNG_OUT, STEAM_STAT_INT, 1);
				}
				bellBreakBulb(my, false);
			}
		}
		else if ( BELL_CURRENT_EVENT == BELL_ITEM )
		{
			if ( BELL_HAS_ITEM != 0 )
			{
				if ( Entity* entity = uidToEntity(BELL_HAS_ITEM) )
				{
					BELL_HAS_ITEM = 0;
					if ( entity->behavior == &actItem )
					{
						playSoundEntityLocal(entity, 47 + local_rng.rand() % 3, 64);
						entity->vel_x = 0.0; //(0.25 + .025 * (local_rng.rand() % 11)) * cos(entity->yaw);
						entity->vel_y = 0.0; //(0.25 + .025 * (local_rng.rand() % 11)) * sin(entity->yaw);
						entity->vel_z = (-2 - local_rng.rand() % 5) * .01;
						entity->itemContainer = 0;
						entity->z = -16;
						entity->itemNotMoving = 0;
						entity->itemNotMovingClient = 0;
						entity->flags[USERFLAG1] = false; // enable collision

						if ( multiplayer == SERVER )
						{
							for ( int i = 1; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									strcpy((char*)net_packet->data, "BELI");
									SDLNet_Write32(static_cast<Uint32>(entity->getUID()), &net_packet->data[4]);
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
						}

						if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
						{
							messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6272));
							Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_LOOT_ITEMS, "bell", 1);
						}
					}
					else if ( entity->behavior == &actGoldBag )
					{
						playSoundEntityLocal(entity, 242 + local_rng.rand() % 4, 64);
						entity->vel_x = 0.0;
						entity->vel_y = 0.0;
						entity->vel_z = (-2 - local_rng.rand() % 5) * .01;
						entity->goldBouncing = 0;
						entity->z = -16;
						entity->flags[INVISIBLE] = false;

						if ( multiplayer == SERVER )
						{
							for ( int i = 1; i < MAXPLAYERS; ++i )
							{
								if ( !client_disconnected[i] )
								{
									strcpy((char*)net_packet->data, "BELI");
									SDLNet_Write32(static_cast<Uint32>(entity->getUID()), &net_packet->data[4]);
									net_packet->address.host = net_clients[i - 1].host;
									net_packet->address.port = net_clients[i - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, i - 1);
								}
							}
						}

						if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
						{
							messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6272));
							Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_LOOT_GOLD, "bell", entity->goldAmount);
						}
					}

					if ( rng.rand() % 16 == 0 )
					{
						// sometimes it just falls straight after
						if ( BELL_LAST_TOUCHED_PLAYER >= 0 )
						{
							messagePlayer(BELL_LAST_TOUCHED_PLAYER, MESSAGE_INTERACTION, Language::get(6275));
							Compendium_t::Events_t::eventUpdateWorld(BELL_LAST_TOUCHED_PLAYER, Compendium_t::CPDM_BELL_BROKEN, "bell", 1);
							steamStatisticUpdateClient(BELL_LAST_TOUCHED_PLAYER, STEAM_STAT_RUNG_OUT, STEAM_STAT_INT, 1);
						}
						bellBreakBulb(my, false);
					}
				}
			}
		}
	}
}