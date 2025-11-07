/*-------------------------------------------------------------------------------

	BARONY
	File: actHandMagic.cpp
	Desc: the spellcasting animations

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../entity.hpp"
#include "../interface/interface.hpp"
#include "../engine/audio/sound.hpp"
#include "../items.hpp"
#include "../player.hpp"
#include "magic.hpp"
#include "../net.hpp"
#include "../scores.hpp"
#include "../ui/MainMenu.hpp"
#include "../prng.hpp"
#include "../mod_tools.hpp"
#include "../collision.hpp"

//The spellcasting animation stages:
#define ANIM_SPELL_CIRCLE 0 //One circle
#define ANIM_SPELL_THROW 1 //Throw spell!
#define ANIM_SPELL_TOUCH 2
#define ANIM_SPELL_COMPLETE_SPELL 3
#define ANIM_SPELL_TOUCH_THROW 4
#define ANIM_SPELL_COMPLETE_NOCAST 5
#define ANIM_SPELL_TOUCH_CHARGE 6

spellcasting_animation_manager_t cast_animation[MAXPLAYERS];
bool overDrawDamageNotify = false;

#define HANDMAGIC_INIT my->skill[0]
#define HANDMAGIC_TESTVAR my->skill[1]
#define HANDMAGIC_PLAYERNUM my->skill[2]
#define HANDMAGIC_YAW my->fskill[3]
#define HANDMAGIC_PITCH my->fskill[4]
#define HANDMAGIC_ROLL my->fskill[5]
#define HANDMAGIC_PARTICLESPRAY1 my->skill[3]
#define HANDMAGIC_CIRCLE_RADIUS 0.8
#define HANDMAGIC_CIRCLE_SPEED 0.3
#define HANDMAGIC_TICKS_PER_CIRCLE 20

void spellcasting_animation_manager_t::resetRangefinder()
{
	target_x = 0.0;
	target_y = 0.0;
	caster_x = 0.0;
	caster_y = 0.0;
	targetUid = 0;
	rangefinder = RANGEFINDER_NONE;
}

bool spellcasting_animation_manager_t::hideShieldFromBasicCast()
{
	if ( player < 0 || player >= MAXPLAYERS ) { return false; }
	if ( active )
	{
		if ( stage == ANIM_SPELL_TOUCH || stage == ANIM_SPELL_TOUCH_THROW || stage == ANIM_SPELL_TOUCH_CHARGE )
		{
			return false;
		}
		return true;
	}
	return false;
}

static ConsoleVariable<int> cvar_vibe_spell_x("/vibe_spell_x", 4000);
static ConsoleVariable<int> cvar_vibe_spell_y("/vibe_spell_y", 0);
static ConsoleVariable<int> cvar_vibe_spell_s("/vibe_spell_s", 0);
void spellcasting_animation_manager_t::executeAttackSpell(bool swingweapon)
{
	if ( player < 0 || player >= MAXPLAYERS ) { return; }
	if ( !spellWaitingAttackInput() )
	{
		return;
	}

	if ( stage == ANIM_SPELL_TOUCH )
	{
		if ( swingweapon )
		{
			stage = ANIM_SPELL_TOUCH_CHARGE;
			throw_count = 0;
		}
	}
	else
	{
		if ( !swingweapon )
		{
			if ( rangefinder == RANGEFINDER_TOUCH_FLOOR_TILE )
			{
				stage = ANIM_SPELL_TOUCH_THROW;
				throw_count = 0;
			}
			else if ( (rangefinder == RANGEFINDER_TOUCH 
				|| rangefinder == RANGEFINDER_TOUCH_INTERACT
				|| rangefinder == RANGEFINDER_TOUCH_INTERACT_TEST) && uidToEntity(targetUid) )
			{
				stage = ANIM_SPELL_TOUCH_THROW;
				throw_count = 0;
			}
			else if ( rangefinder == RANGEFINDER_TOUCH_WALL_TILE && wallDir >= 1 )
			{
				stage = ANIM_SPELL_TOUCH_THROW;
				throw_count = 0;
			}
			else
			{
				stage = ANIM_SPELL_TOUCH;
				throw_count = 0;
			}
			/*else
			{
				spellcastingAnimationManager_deactivate(this);
				messagePlayer(player, MESSAGE_COMBAT, Language::get(6496));
				playSoundEntityLocal(players[player]->entity, 163, 64);
			}*/
		}
	}
}

bool spellcasting_animation_manager_t::spellWaitingAttackInput()
{
	if ( active && !active_spellbook && (stage == ANIM_SPELL_TOUCH || stage == ANIM_SPELL_TOUCH_CHARGE) )
	{
		return true;
	}
	return false;
}

bool spellcasting_animation_manager_t::spellIgnoreAttack()
{
	if ( active && !active_spellbook 
		&& (stage == ANIM_SPELL_COMPLETE_NOCAST	|| stage == ANIM_SPELL_COMPLETE_SPELL || stage == ANIM_SPELL_TOUCH_THROW) )
	{
		return true;
	}
	return false;
}

bool rangefinderTargetEnemyType(spell_t& spell, Entity& entity)
{
	if ( spell.ID == SPELL_TELEKINESIS )
	{
		if ( entity.behavior == &actBoulder && !entity.flags[PASSABLE] && entity.z >= -8 )
		{
			return true;
		}
		else if ( entity.behavior == &actItem && !entity.flags[INVISIBLE] )
		{
			return true;
		}
		else if ( entity.behavior == &actGoldBag && !entity.flags[INVISIBLE] )
		{
			return true;
		}
		else if ( entity.behavior == &actMonster && entity.getMonsterTypeFromSprite() == EARTH_ELEMENTAL )
		{
			return true;
		}
		else if ( entity.behavior == &actSwitch ||
			entity.behavior == &actSwitchWithTimer 
			/*|| entity.sprite == 184*/
			)
		{
			return true;
		}
		else if ( entity.behavior == &::actWallButton
			/*|| entity.sprite == 1151
			|| entity.sprite == 1152*/
			)
		{
			return true;
		}
		/*else if ( (entity.behavior == &::actWallLock
			|| (entity.sprite >= 1585 && entity.sprite <= 1592)) )
		{
			return true;
		}*/
		else if ( entity.behavior == &actBell )
		{
			return true;
		}
		return false;
	}
	else if ( spell.ID == SPELL_SABOTAGE
		|| spell.ID == SPELL_HARVEST_TRAP )
	{
		if ( entity.behavior == &actArrowTrap )
		{
			return true;
		}
		else if ( entity.behavior == &actBoulderTrapHole )
		{
			return true;
		}
		else if ( entity.behavior == &actMagicTrapCeiling )
		{
			return true;
		}
		else if ( entity.behavior == &actMagicTrap )
		{
			return true;
		}
		else if ( entity.behavior == &actSpearTrap )
		{
			return true;
		}
		else if ( entity.behavior == &actMonster )
		{
			if ( spell.ID == SPELL_HARVEST_TRAP )
			{
			}
			else
			{
				Monster type = entity.getMonsterTypeFromSprite();
				if ( type == MIMIC || type == AUTOMATON || type == CRYSTALGOLEM || type == MINIMIMIC )
				{
					return true;
				}
			}
		}
		else if ( entity.behavior == &actChest )
		{
			if ( spell.ID == SPELL_HARVEST_TRAP )
			{
			}
			else
			{
				return true;
			}
		}
		return false;
	}
	else if ( spell.ID == SPELL_KINETIC_PUSH )
	{
		if ( entity.behavior == &actBoulder && !entity.flags[PASSABLE] && entity.z >= -8 )
		{
			return true;
		}
		else if ( (entity.behavior == &actMonster && !entity.isInertMimic()) || entity.behavior == &actPlayer )
		{
			return true;
		}
		return false;
	}
	else if ( spell.ID == SPELL_VOID_CHEST )
	{
		return (entity.behavior == &actMonster && entity.getMonsterTypeFromSprite() == MIMIC) || entity.behavior == &actChest;
	}
	else if ( spell.ID == SPELL_BOOBY_TRAP )
	{
		return entity.isDamageableCollider() || entity.behavior == &actFurniture
			|| entity.behavior == &actChest || entity.behavior == &actDoor 
			|| (entity.behavior == &actMonster && entity.getMonsterTypeFromSprite() == MIMIC);
	}
	else if ( spell.ID == SPELL_SPLINTER_GEAR )
	{
		return entity.behavior == &actMonster || entity.behavior == &actPlayer || entity.behavior == &actChest;
	}
	else if ( spell.ID == SPELL_DEFACE )
	{
		return (!entity.flags[INVISIBLE] && entity.behavior == &actHeadstone) || entity.behavior == &actSink;
	}
	else if ( spell.ID == SPELL_DEMESNE_DOOR )
	{
		return (entity.behavior == &actDoorFrame && !entity.flags[INVISIBLE]) /*|| entity.behavior == &actDoor*/;
	}
	else
	{
		return (entity.behavior == &actMonster && !entity.isInertMimic()) || entity.behavior == &actPlayer;
	}
	return false;
}

void spellcasting_animation_manager_t::setRangeFinderLocation()
{
	Entity* caster = uidToEntity(this->caster);
	rangefinder = RANGEFINDER_NONE;
	if ( !caster )
	{
		return;
	}

	if ( !spell )
	{
		return;
	}

	if ( !spell->rangefinder )
	{
		return;
	}

	rangefinder = spell->rangefinder;

	if ( stage == ANIM_SPELL_TOUCH_THROW )
	{
		return;
	}
	if ( stage == ANIM_SPELL_TOUCH_CHARGE && targetUid != 0 && uidToEntity(targetUid) )
	{
		return;
	}

	if ( rangefinder == RANGEFINDER_TOUCH_INTERACT_TEST )
	{
		targetUid = 0;
		if ( players[player]->worldUI.isEnabled() )
		{
			if ( players[player]->worldUI.tooltipsInRange.size() > 0 )
			{
				for ( node_t* node = map.worldUI->first; node; node = node->next )
				{
					Entity* tooltip = (Entity*)node->element;
					if ( !tooltip || tooltip->behavior != &actSpriteWorldTooltip )
					{
						continue;
					}
					if ( players[player]->worldUI.bTooltipActiveForPlayer(*tooltip) )
					{
						targetUid = tooltip->parent;
						break;
					}
				}
			}
		}
	}

	static ConsoleVariable<float> cvar_rangefinderStartZ("/rangefinder_start_z", -2.5);
	static ConsoleVariable<float> cvar_rangefinderMoveTo("/rangefinder_moveto_z", 0.1);
	static ConsoleVariable<float> cvar_rangefinderStartZLimit("/rangefinder_start_z_limit", 7.5);
	static ConsoleVariable<bool> cvar_rangefinder_cam("/rangefinder_cam", false);
	real_t startx = caster->x;
	real_t starty = caster->y;
	real_t startz = caster->z;
	real_t pitch = caster->pitch;
	real_t yaw = caster->yaw;
	if ( *cvar_rangefinder_cam )
	{
		startx = cameras[player].x * 16.0;
		starty = cameras[player].y * 16.0;
		startz = cameras[player].z + (4.5 - cameras[player].z) / 2.0 + *cvar_rangefinderStartZ;
		pitch = cameras[player].vang;
		yaw = cameras[player].ang;
	}
	if ( pitch < 0 || pitch > PI )
	{
		pitch = 0;
	}

	// draw line from the players height and direction until we hit the ground.
	real_t previousx = startx;
	real_t previousy = starty;
	int index = 0;
	wallDir = 0;

	real_t spellDistance = getSpellPropertyFromID(spell_t::SPELLPROP_MODIFIED_DISTANCE, spell->ID, caster, nullptr, caster);

	if ( rangefinder == RANGEFINDER_TOUCH_WALL_TILE )
	{
		real_t dist = lineTrace(nullptr, startx, starty, yaw, spellDistance, 0, false);
		if ( dist < spellDistance )
		{
			previousx = hit.x;
			previousy = hit.y;
			if ( hit.side == HORIZONTAL )
			{
				if ( yaw >= 3 * PI / 2 || yaw <= PI / 2 )
				{
					wallDir = 3;
					previousx = static_cast<int>((hit.x + 8.0) / 16);
					previousy = static_cast<int>(hit.y / 16);
				}
				else
				{
					wallDir = 1;
					previousx = static_cast<int>((hit.x - 8.0) / 16);
					previousy = static_cast<int>(hit.y / 16);
				}
			}
			else if ( hit.side == VERTICAL )
			{
				if ( yaw >= 0 && yaw < PI )
				{
					wallDir = 4;
					previousx = static_cast<int>(hit.x / 16);
					previousy = static_cast<int>((hit.y + 8.0) / 16);
				}
				else
				{
					wallDir = 2;
					previousx = static_cast<int>(hit.x / 16);
					previousy = static_cast<int>((hit.y - 8.0) / 16);
				}
			}
			else
			{
				wallDir = 0;
				//messagePlayer(0, MESSAGE_DEBUG, "??");
			}
		}
	}
	else
	{
		for ( ; startz < *cvar_rangefinderStartZLimit; startz += abs((*cvar_rangefinderMoveTo) * tan(pitch)) )
		{
			startx += 0.1 * cos(yaw);
			starty += 0.1 * sin(yaw);
			const int index_x = static_cast<int>(startx) >> 4;
			const int index_y = static_cast<int>(starty) >> 4;
			index = (index_y)*MAPLAYERS + (index_x)*MAPLAYERS * map.height;

			if ( index_x < 0 || index_x >= map.width || index_y < 0 || index_y >= map.height )
			{
				break;
			}

			if ( (map.tiles[index] || rangefinder != RANGEFINDER_TARGET) && !map.tiles[OBSTACLELAYER + index] )
			{
				// store the last known good coordinate
				previousx = startx;// + 16 * cos(yaw);
				previousy = starty;// + 16 * sin(yaw);
			}
			if ( map.tiles[OBSTACLELAYER + index] )
			{
				break;
			}
			if ( pow(startx - caster->x, 2) + pow(starty - caster->y, 2) > (spellDistance * spellDistance) )
			{
				// break if distance reached
				break;
			}
		}
	}

	target_x = previousx;
	target_y = previousy;
	caster_x = caster->x;
	caster_y = caster->y;

	if ( rangefinder == RANGEFINDER_TOUCH
		|| rangefinder == RANGEFINDER_TOUCH_INTERACT )
	{
		real_t maxDist = spellDistance + 8.0;
		real_t minDist = 4.0;
		real_t rangeFinderDist = std::max(0.0, sqrt(pow(startx - caster->x, 2) + pow(starty - caster->y, 2)) - 8.0);

		targetUid = 0;

		bool interactBonusWidthEnemies = true;
		bool interactBonusWidthAllies = false;
		list_t* entityList = map.creatures;
		if ( spell->ID == SPELL_TELEKINESIS )
		{
			minDist = 16.0;
		}

		if ( spell->ID == SPELL_BOOBY_TRAP || spell->ID == SPELL_VOID_CHEST || spell->ID == SPELL_TELEKINESIS
			|| spell->ID == SPELL_KINETIC_PUSH || spell->ID == SPELL_SABOTAGE || spell->ID == SPELL_HARVEST_TRAP
			|| spell->ID == SPELL_SPLINTER_GEAR || spell->ID == SPELL_DEFACE || spell->ID == SPELL_SUNDER_MONUMENT
			|| spell->ID == SPELL_DEMESNE_DOOR )
		{
			entityList = map.entities;
		}
		else if ( spell->ID == SPELL_HEAL_OTHER )
		{
			interactBonusWidthAllies = true;
			interactBonusWidthEnemies = false;
		}

		struct EntitySpellTargetLocation
		{
			Entity* entity = nullptr;
			real_t dist = 0.0;
			real_t crossDist = 0.0;
			int wallDir = 0;
		};

		auto compFunc = [](EntitySpellTargetLocation& lhs, EntitySpellTargetLocation& rhs)
		{
			if ( abs(lhs.crossDist - rhs.crossDist) <= 0.00001 )
			{
				return lhs.dist > rhs.dist;
			}
			return lhs.crossDist > rhs.crossDist;
		};
		std::priority_queue<EntitySpellTargetLocation, std::vector<EntitySpellTargetLocation>, decltype(compFunc)> entitiesInRange(compFunc);
		for ( auto node = entityList->first; node; node = node->next ) // TODO - grab a shortened list from somewhere else iterating entities
		{
			if ( Entity* entity = (Entity*)node->element )
			{
				if ( rangefinderTargetEnemyType(*spell, *entity) )
				{
					real_t dist = entityDist(caster, entity);
					if ( dist > minDist && dist < maxDist 
						/*&& rangeFinderDist >= dist*/
						)
					{
						real_t tangent = atan2(entity->y - caster->y, entity->x - caster->x);
						while ( tangent >= 2 * PI )
						{
							tangent -= 2 * PI;
						}
						while ( tangent < 0 )
						{
							tangent += 2 * PI;
						}
						real_t playerYaw = caster->yaw;
						while ( playerYaw >= 2 * PI )
						{
							playerYaw -= 2 * PI;
						}
						while ( playerYaw < 0 )
						{
							playerYaw += 2 * PI;
						}

						real_t interactAngle = PI / 8;
						bool bonusRange = false;
						if ( spell->ID == SPELL_BOOBY_TRAP )
						{
							bonusRange = true;
						}
						else if ( interactBonusWidthAllies && (entity->behavior == &actPlayer
							|| (entity->behavior == &actMonster && entity->monsterAllyGetPlayerLeader())) )
						{
							// monsters have wider interact angle for aim assist
							bonusRange = true;
							
						}
						else if ( interactBonusWidthEnemies && entity->behavior == &actMonster
							&& !(entity->isInertMimic())
							&& ((multiplayer != CLIENT && entity->checkEnemy(caster))
								|| (multiplayer == CLIENT
									&& !entity->monsterAllyGetPlayerLeader() && !monsterally[entity->getMonsterTypeFromSprite()][caster->getStats()->type])) )
						{
							// monsters have wider interact angle for aim assist
							bonusRange = true;
						}
						
						if ( bonusRange )
						{
							interactAngle = (PI / 6);
							if ( dist > STRIKERANGE )
							{
								// for every x units, shrink the selection angle
								int units = static_cast<int>(dist / 16);
								interactAngle -= (units * PI / 64);
								interactAngle = std::max(interactAngle, PI / 32);
							}
						}
						else
						{
							if ( dist > STRIKERANGE )
							{
								// for every x units, shrink the selection angle
								int units = static_cast<int>(dist / 8);
								interactAngle -= (units * PI / 64);
								interactAngle = std::max(interactAngle, PI / 64);
							}
						}

						if ( (abs(tangent - playerYaw) < (interactAngle)) || (abs(tangent - playerYaw) > (2 * PI - interactAngle)) )
						{
							if ( entity->behavior == &actArrowTrap || entity->behavior == &actMagicTrap )
							{
								Entity* ohit = hit.entity;
								real_t dist2 = lineTrace(nullptr, caster->x, caster->y, yaw, dist, 0, false);
								if ( dist2 < dist )
								{
									int tempx = hit.x;
									int tempy = hit.y;
									int wallDir = 0;
									if ( hit.side == HORIZONTAL )
									{
										if ( yaw >= 3 * PI / 2 || yaw <= PI / 2 )
										{
											wallDir = 3;
											tempx = static_cast<int>((hit.x + 8.0) / 16);
											tempy = static_cast<int>(hit.y / 16);
										}
										else
										{
											wallDir = 1;
											tempx = static_cast<int>((hit.x - 8.0) / 16);
											tempy = static_cast<int>(hit.y / 16);
										}
									}
									else if ( hit.side == VERTICAL )
									{
										if ( yaw >= 0 && yaw < PI )
										{
											wallDir = 4;
											tempx = static_cast<int>(hit.x / 16);
											tempy = static_cast<int>((hit.y + 8.0) / 16);
										}
										else
										{
											wallDir = 2;
											tempx = static_cast<int>(hit.x / 16);
											tempy = static_cast<int>((hit.y - 8.0) / 16);
										}
									}
									else
									{
										wallDir = 0;
										//messagePlayer(0, MESSAGE_DEBUG, "??");
									}

									if ( wallDir > 0 )
									{
										if ( tempx == static_cast<int>(entity->x / 16)
											&& tempy == static_cast<int>(entity->y / 16) )
										{
											real_t crossDist = abs(dist2 * sin(tangent - playerYaw));
											entitiesInRange.push(EntitySpellTargetLocation{ entity, dist2, crossDist, wallDir });
										}
										//messagePlayer(0, MESSAGE_DEBUG, "x: %d y: %d | entity: x: %d y: %d",
										//	tempx, tempy, (int)(entity->x / 16), (int)(entity->y / 16));
									}
								}
								hit.entity = ohit;
							}
							else
							{
								Entity* ohit = hit.entity;
								int lineTraceFlags = spell->ID == SPELL_TELEKINESIS ? LINETRACE_TELEKINESIS : 0;
								lineTraceTarget(entity, entity->x, entity->y, tangent + PI, dist, LINETRACE_TELEKINESIS, false, caster);
								if ( hit.entity == caster )
								{
									real_t crossDist = abs(dist * sin(tangent - playerYaw));
									if ( spell->ID == SPELL_TELEKINESIS 
										&& (entity->behavior == &actItem 
											|| entity->behavior == &actGoldBag
											|| entity->behavior == &actSwitch 
											|| entity->behavior == &actSwitchWithTimer) )
									{
										real_t targetTileDist = 4.0 + sqrt(pow(target_x - entity->x, 2) + pow(target_y - entity->y, 2));
										crossDist = sqrt(pow(targetTileDist, 2) + pow(crossDist, 2));
									}
									entitiesInRange.push(EntitySpellTargetLocation{entity, dist, crossDist, 0});
								}
								hit.entity = ohit;
							}
						}
					}
				}
			}
		}

		if ( entitiesInRange.size() )
		{
			targetUid = entitiesInRange.top().entity->getUID();
			wallDir = entitiesInRange.top().wallDir;
			//messagePlayer(0, MESSAGE_DEBUG, "Dist: %.2f, Cross: %.2f", entitiesInRange.top().dist, entitiesInRange.top().crossDist);
		}
	}
}

void spellcastAnimationUpdate(int player, int attackPose, int castTime)
{
	if ( multiplayer == CLIENT )
	{
		// send to server with impulse
		strcpy((char*)net_packet->data, "SANM");
		net_packet->data[4] = player;
		net_packet->data[5] = attackPose;
		SDLNet_Write16(castTime, &net_packet->data[6]);
		net_packet->len = 8;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}

	spellcastAnimationUpdateReceive(player, attackPose, castTime);
}

void spellcastAnimationUpdateReceive(int player, int attackPose, int castTime)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}

	if ( multiplayer == SERVER )
	{
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( !client_disconnected[i] && !players[i]->isLocalPlayer() )
			{
				if ( i != player )
				{
					// forward on packet to other clients
					strcpy((char*)net_packet->data, "SANM");
					net_packet->data[4] = player;
					net_packet->data[5] = attackPose;
					SDLNet_Write16(castTime, &net_packet->data[6]);
					net_packet->len = 8;
					net_packet->address.host = net_clients[i - 1].host;
					net_packet->address.port = net_clients[i - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, i - 1);
				}
			}
		}
	}

	if ( players[player]->entity )
	{
		if ( attackPose == MONSTER_POSE_MAGIC_WINDUP1 )
		{
			players[player]->entity->playerCastTimeAnim = castTime;
			if ( players[player]->entity->skill[9] == MONSTER_POSE_MAGIC_WINDUP1 )
			{
				// add to the counter to not stagger restart
				players[player]->entity->playerCastTimeAnim += players[player]->entity->skill[10];
			}
			else
			{
				players[player]->entity->skill[10] = 0;
			}
			if ( !players[player]->isLocalPlayer() )
			{
				playSoundEntityLocal(players[player]->entity, 170, 128);
			}
			players[player]->entity->skill[9] = MONSTER_POSE_MAGIC_WINDUP1;
		}
		else if ( attackPose == MONSTER_POSE_MAGIC_WINDUP2 )
		{
			if ( !players[player]->isLocalPlayer() )
			{
				if ( players[player]->entity->skill[9] != MONSTER_POSE_MAGIC_WINDUP2 )
				{
					playSoundEntityLocal(players[player]->entity, 759 + local_rng.rand() % 4, 92); // charge up sound
				}
			}
			players[player]->entity->skill[9] = attackPose;
			players[player]->entity->skill[10] = 0;
		}
		else if ( attackPose == MONSTER_POSE_MAGIC_CAST2 || attackPose == 1 )
		{
			if ( attackPose == MONSTER_POSE_MAGIC_CAST2 && !players[player]->isLocalPlayer() )
			{
				if ( players[player]->entity->skill[9] == MONSTER_POSE_MAGIC_WINDUP2 )
				{
					playSoundEntityLocal(players[player]->entity, 163, 64); // fizzle sound
				}
			}
			players[player]->entity->skill[9] = attackPose;
			players[player]->entity->skill[10] = 0;
		}
	}
}

void fireOffSpellAnimation(spellcasting_animation_manager_t* animation_manager, Uint32 caster_uid, spell_t* spell, bool usingSpellbook)
{
	//This function triggers the spellcasting animation and sets up everything.

	if ( !animation_manager )
	{
		return;
	}
	Entity* caster = uidToEntity(caster_uid);
	if (!caster)
	{
		return;
	}
	int player = caster->skill[2];
	if ( !spell )
	{
		return;
	}
	if ( !players[player]->hud.magicLeftHand )
	{
		return;
	}
	if ( !players[player]->hud.magicRightHand )
	{
		return;
	}

	playSoundEntityLocal(caster, 170, 128 );
	Stat* stat = caster->getStats();

	//Save these three very important pieces of data.
	animation_manager->player = caster->skill[2];
	animation_manager->caster = caster->getUID();
	animation_manager->spell = spell;

	if ( !usingSpellbook )
	{
		animation_manager->active = true;
	}
	else
	{
		animation_manager->active_spellbook = true;
	}
	animation_manager->stage = ANIM_SPELL_CIRCLE;

	//Make the HUDWEAPON disappear, or somesuch?
	players[player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = false;
	players[player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = false;
	if ( stat->type != RAT )
	{
		if ( !usingSpellbook )
		{
			players[player]->hud.magicLeftHand->flags[INVISIBLE] = false;
			if ( caster->isInvisible() )
			{
				players[player]->hud.magicLeftHand->flags[INVISIBLE] = true;
				players[player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = true;
			}
		}
		players[player]->hud.magicRightHand->flags[INVISIBLE] = false;
		if ( caster->isInvisible() )
		{
			players[player]->hud.magicRightHand->flags[INVISIBLE] = true;
			players[player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = true;
		}
	}

	animation_manager->lefthand_angle = 0;
	animation_manager->lefthand_movex = 0;
	animation_manager->lefthand_movey = 0;
	int spellCost = getCostOfSpell(spell, caster);
	animation_manager->circle_count = 0;
	animation_manager->throw_count = 0;
	animation_manager->active_count = 0;
	animation_manager->times_to_circle = spell->cast_time * HANDMAGIC_TICKS_PER_CIRCLE;// (spellCost / 10) + 1; //Circle once for every 10 mana the spell costs.
	animation_manager->mana_left = spellCost;
	animation_manager->mana_cost = spellCost;
	animation_manager->consumeMana = true;
	/*if ( spell->ID == SPELL_FORCEBOLT && caster->skillCapstoneUnlockedEntity(PRO_LEGACY_SPELLCASTING) )
	{
		animation_manager->consumeMana = false;
	}*/

	if ( isSpellcasterBeginner(player, caster, spell->skillID) )   //There's a chance that caster is newer to magic (and thus takes longer to cast a spell).
	{
		int chance = local_rng.rand() % 10;
		if (chance >= stat->getModifiedProficiency(spell->skillID) / 15)
		{
			int amount = (local_rng.rand() % 50) / std::max(stat->getModifiedProficiency(spell->skillID) + statGetINT(stat, caster), 1);
			amount = std::min(amount, CASTING_EXTRA_TIMES_CAP);
			animation_manager->times_to_circle += amount * HANDMAGIC_TICKS_PER_CIRCLE;
		}
	}
	if ( usingSpellbook && stat->shield && itemCategory(stat->shield) == SPELLBOOK )
	{
		if ( !playerLearnedSpellbook(player, stat->shield) || (stat->shield->beatitude < 0 && !shouldInvertEquipmentBeatitude(stat)) )
		{
			// for every tier below the spell you are, add 3 circle for 1 tier, or add 2 for every additional tier.
			int casterAbility = std::min(100, std::max(0, stat->getModifiedProficiency(spell->skillID) + statGetINT(stat, caster))) / 20;
			if ( stat->shield->beatitude < 0 )
			{
				casterAbility = 0; // cursed book has cast penalty.
			}
			int difficulty = spell->difficulty / 20;
			if ( difficulty > casterAbility )
			{
				animation_manager->times_to_circle += (std::min(5, 1 + 2 * (difficulty - casterAbility))) * HANDMAGIC_TICKS_PER_CIRCLE;
			}
		}
		else if ( !isSpellcasterBeginner(player, caster, spell->skillID) )
		{
			animation_manager->times_to_circle = std::max(HANDMAGIC_TICKS_PER_CIRCLE, animation_manager->times_to_circle - HANDMAGIC_TICKS_PER_CIRCLE);
			//animation_manager->times_to_circle = (spellCost / 20) + 1; //Circle once for every 20 mana the spell costs.
		}
	}
	animation_manager->times_to_circle = std::max(10, animation_manager->times_to_circle);
	animation_manager->consume_interval = (animation_manager->times_to_circle / std::max(1, spellCost));
	animation_manager->consume_timer = animation_manager->consume_interval;
	animation_manager->setRangeFinderLocation();

	spellcastAnimationUpdate(player, MONSTER_POSE_MAGIC_WINDUP1, animation_manager->times_to_circle + HANDMAGIC_TICKS_PER_CIRCLE);
}

void spellcastingAnimationManager_deactivate(spellcasting_animation_manager_t* animation_manager)
{
	if ( animation_manager->stage == ANIM_SPELL_TOUCH
		|| animation_manager->stage == ANIM_SPELL_TOUCH_CHARGE )
	{
		spellcastAnimationUpdate(animation_manager->player, MONSTER_POSE_MAGIC_CAST2, 0);
	}

	animation_manager->caster = -1;
	animation_manager->spell = NULL;
	animation_manager->active = false;
	animation_manager->active_spellbook = false;
	animation_manager->stage = 0;
	animation_manager->circle_count = 0;
	animation_manager->active_count = 0;
	animation_manager->resetRangefinder();

	if ( animation_manager->player == -1 )
	{
		printlog("spellcastingAnimationManager_deactivate: Error - player was -1!");
		return;
	}
	//Make the hands invisible (should probably fall away or something, but whatever. That's another project for another day)
	if ( players[animation_manager->player]->hud.magicLeftHand )
	{
		players[animation_manager->player]->hud.magicLeftHand->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicLeftHand->flags[INVISIBLE_DITHER] = false;
	}
	if ( players[animation_manager->player]->hud.magicRightHand )
	{
		players[animation_manager->player]->hud.magicRightHand->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicRightHand->flags[INVISIBLE_DITHER] = false;
	}
	if ( players[animation_manager->player]->hud.magicRangefinder )
	{
		players[animation_manager->player]->hud.magicRangefinder->flags[INVISIBLE] = true;
		players[animation_manager->player]->hud.magicRangefinder->flags[INVISIBLE_DITHER] = false;
	}
}

void spellcastingAnimationManager_completeSpell(int player, spellcasting_animation_manager_t* animation_manager, bool deactivate)
{
	if ( animation_manager->stage == ANIM_SPELL_TOUCH_THROW )
	{
		spellcastAnimationUpdate(animation_manager->player, 1, 0);
	}

	if ( animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TARGET
		|| animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE
		|| animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_INTERACT_TEST
		|| animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_WALL_TILE )
	{
		CastSpellProps_t castSpellProps;
		castSpellProps.caster_x = animation_manager->caster_x;
		castSpellProps.caster_y = animation_manager->caster_y;
		castSpellProps.target_x = animation_manager->target_x;
		castSpellProps.target_y = animation_manager->target_y;
		castSpellProps.wallDir = animation_manager->wallDir;
		if ( animation_manager->spell )
		{
			if ( animation_manager->spell->ID == SPELL_MUSHROOM )
			{
				if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_SPORES)) )
				{
					castSpellProps.optionalData = 1;
				}
				if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_SPORE_BOMB)) )
				{
					castSpellProps.optionalData = 2;
				}
			}
			else if ( animation_manager->spell->ID == SPELL_SHRUB )
			{
				if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_THORNS)) )
				{
					castSpellProps.optionalData = 1;
				}
				if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_BLADEVINES)) )
				{
					castSpellProps.optionalData = 2;
				}
			}
		}
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook, &castSpellProps); //Actually cast the spell.
	}
	else if ( animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH
		|| animation_manager->rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_INTERACT )
	{
		CastSpellProps_t castSpellProps;
		castSpellProps.targetUID = animation_manager->targetUid;
		castSpellProps.wallDir = animation_manager->wallDir;
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook, &castSpellProps); //Actually cast the spell.
	}
	else if ( animation_manager->spell && (animation_manager->spell->ID == SPELL_BASTION_MUSHROOM || animation_manager->spell->ID == SPELL_BASTION_ROOTS) )
	{
		CastSpellProps_t castSpellProps;
		if ( animation_manager->spell->ID == SPELL_BASTION_MUSHROOM )
		{
			if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_SPORES)) )
			{
				castSpellProps.optionalData = 1;
			}
			if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_SPORE_BOMB)) )
			{
				castSpellProps.optionalData = 2;
			}
		}
		else if ( animation_manager->spell->ID == SPELL_BASTION_ROOTS )
		{
			if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_THORNS)) )
			{
				castSpellProps.optionalData = 1;
			}
			if ( spellInList(&players[player]->magic.spellList, getSpellFromID(SPELL_BLADEVINES)) )
			{
				castSpellProps.optionalData = 2;
			}
		}
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook, &castSpellProps); //Actually cast the spell.
	}
	else
	{
		castSpell(animation_manager->caster, animation_manager->spell, false, false, animation_manager->active_spellbook); //Actually cast the spell.
	}

	if ( deactivate )
	{
		spellcastingAnimationManager_deactivate(animation_manager);
	}
}

/*
[12:48:29 PM] Sheridan Kane Rathbun: you can move the entities about by modifying their x, y, z, yaw, pitch, and roll parameters.
[12:48:43 PM] Sheridan Kane Rathbun: everything's relative to the camera since the OVERDRAW flag is on.
[12:49:05 PM] Sheridan Kane Rathbun: so adding x will move it forward, adding y will move it sideways (forget which way) and adding z will move it up and down.
[12:49:46 PM] Sheridan Kane Rathbun: the first step is to get the hands visible on the screen when you cast. worry about moving them when that critical part is done.
*/

void actLeftHandMagic(Entity* my)
{
	//int c = 0;
	my->flags[INVISIBLE_DITHER] = false;
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if (!HANDMAGIC_INIT)
	{
		HANDMAGIC_INIT = 1;
		HANDMAGIC_TESTVAR = 0;
		my->focalz = -1.5;
	}

	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		players[HANDMAGIC_PLAYERNUM]->hud.magicLeftHand = nullptr;
		spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
		list_RemoveNode(my->mynode);
		return;
	}

	my->mistformGLRender = players[HANDMAGIC_PLAYERNUM]->entity->mistformGLRender;

	//Set the initial values. (For the particle spray)
	my->x = 8;
	my->y = -3;
	my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
	double defaultpitch = (0 - 2.2);
	my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
	my->scalex = 0.5f;
	my->scaley = 0.5f;
	my->scalez = 0.5f;
	my->z -= 0.75;

	//Sprite
	Monster playerRace = players[HANDMAGIC_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HANDMAGIC_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HANDMAGIC_PLAYERNUM]->stat_appearance;
	if ( players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift);
	}
	else if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	if ( playerRace == MONSTER_G )
	{
		my->z -= -1 * .5;
	}
	else if ( playerRace == MONSTER_D )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->z >= 1.5 )
		{
			my->z -= -3.0 * .5;
		}
		else
		{
			my->z -= -2.0 * .5;
		}
	}
	else if ( playerRace == MONSTER_M )
	{
		my->z -= -1.0 * .5;
	}

	bool noGloves = false;
	if ( stats[HANDMAGIC_PLAYERNUM]->gloves == NULL
		|| playerRace == SPIDER
		|| playerRace == RAT
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		noGloves = true;
	}
	else
	{
		if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 659;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 660;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 661;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 662;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 663;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 664;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 666;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 665;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 803;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BONE_BRACERS )
		{
			my->sprite = 2108;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BLACKIRON_GAUNTLETS )
		{
			my->sprite = 2110;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SILVER_GAUNTLETS )
		{
			my->sprite = 2112;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == QUILTED_GLOVES )
		{
			my->sprite = 2114;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CHAIN_GLOVES )
		{
			my->sprite = 2116;
		}
	}


	if ( noGloves )
	{

		switch ( playerRace )
		{
			case SKELETON:
				my->sprite = 773;
				break;
			case INCUBUS:
				my->sprite = 775;
				break;
			case SUCCUBUS:
				my->sprite = 777;
				break;
			case GOBLIN:
				my->sprite = 779;
				break;
			case AUTOMATON:
				my->sprite = 781;
				break;
			case INSECTOID:
				if ( stats[HANDMAGIC_PLAYERNUM]->sex == FEMALE )
				{
					my->sprite = 785;
				}
				else
				{
					my->sprite = 783;
				}
				break;
			case GOATMAN:
				my->sprite = 787;
				break;
			case VAMPIRE:
				my->sprite = 789;
				break;
			case HUMAN:
				if ( playerAppearance / 6 == 0 )
				{
					my->sprite = 656;
				}
				else if ( playerAppearance / 6 == 1 )
				{
					my->sprite = 657;
				}
				else
				{
					my->sprite = 658;
				}
				break;
			case TROLL:
				my->sprite = 856;
				break;
			case SPIDER:
				my->sprite = arachnophobia_filter ? 1006 : 854;
				break;
			case CREATURE_IMP:
				my->sprite = 858;
				break;
			default:
				my->sprite = 656;
				break;
		}
		/*}
		else if ( playerAppearance / 6 == 0 )
		{
			my->sprite = 656;
		}
		else if ( playerAppearance / 6 == 1 )
		{
			my->sprite = 657;
		}
		else
		{
			my->sprite = 658;
		}*/
	}

	Entity*& hudarm = players[HANDMAGIC_PLAYERNUM]->hud.arm;
	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = -hudarm->y;
		//my->z = hudArm->z;
		my->pitch = hudarm->pitch;
		my->roll = -hudarm->roll;
		my->yaw = -players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->focalz = -1.5;
	}

	if ( !cast_animation[HANDMAGIC_PLAYERNUM].active )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		if ( playerRace == RAT )
		{
			my->flags[INVISIBLE] = true;
			my->y = 0;
			my->z += 1;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1 )   // debug cam
		{
			my->flags[INVISIBLE] = true;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->isInvisible() )
		{
			my->flags[INVISIBLE] = true;
			my->flags[INVISIBLE_DITHER] = true;
		}
	}

	if ( (cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		cast_animation[HANDMAGIC_PLAYERNUM].setRangeFinderLocation();
		switch ( cast_animation[HANDMAGIC_PLAYERNUM].stage)
		{
			case ANIM_SPELL_CIRCLE:
				if ( ticks % 5 == 0 && !(players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1) )
				{
					Entity* entity = spawnGib(my);
					entity->flags[INVISIBLE] = false;
					entity->flags[SPRITE] = true;
					entity->flags[NOUPDATE] = true;
					entity->flags[UPDATENEEDED] = false;
					entity->flags[OVERDRAW] = true;
					entity->lightBonus = vec4(0.2f, 0.2f, 0.2f, 0.f);
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->z += 3.0;
					if ( cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook )
					{
						entity->y -= 1.5;
						entity->z += 1;
					}
					entity->yaw = ((local_rng.rand() % 6) * 60) * PI / 180.0;
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * .1;
					entity->vel_y = sin(entity->yaw) * .1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
					entity->skill[11] = HANDMAGIC_PLAYERNUM;
				}
				cast_animation[HANDMAGIC_PLAYERNUM].consume_timer--;
				if ( cast_animation[HANDMAGIC_PLAYERNUM].consume_timer < 0 && cast_animation[HANDMAGIC_PLAYERNUM].mana_left > 0 )
				{
					//Time to consume mana and reset the ticker!
					cast_animation[HANDMAGIC_PLAYERNUM].consume_timer = cast_animation[HANDMAGIC_PLAYERNUM].consume_interval;
					if ( multiplayer == SINGLE && cast_animation[HANDMAGIC_PLAYERNUM].consumeMana )
					{
						int HP = stats[HANDMAGIC_PLAYERNUM]->HP;
						int MP = stats[HANDMAGIC_PLAYERNUM]->MP;
						players[HANDMAGIC_PLAYERNUM]->entity->drainMP(1, false); // don't notify otherwise we'll get spammed each 1 mp

						if ( cast_animation[HANDMAGIC_PLAYERNUM].spell )
						{
							bool sustainedSpell = false;
							auto findSpellDef = ItemTooltips.spellItems.find(cast_animation[HANDMAGIC_PLAYERNUM].spell->ID);
							if ( findSpellDef != ItemTooltips.spellItems.end() )
							{
								sustainedSpell = (findSpellDef->second.spellType == ItemTooltips_t::SpellItemTypes::SPELL_TYPE_SELF_SUSTAIN);
							}
							if ( sustainedSpell )
							{
								players[HANDMAGIC_PLAYERNUM]->mechanics.sustainedSpellIncrementMP(MP - stats[HANDMAGIC_PLAYERNUM]->MP, cast_animation[HANDMAGIC_PLAYERNUM].spell->skillID);
							}
							else
							{
								players[HANDMAGIC_PLAYERNUM]->mechanics.baseSpellIncrementMP(MP - stats[HANDMAGIC_PLAYERNUM]->MP, cast_animation[HANDMAGIC_PLAYERNUM].spell->skillID);
							}
						}

						if ( (HP > stats[HANDMAGIC_PLAYERNUM]->HP) && !overDrawDamageNotify )
						{
							overDrawDamageNotify = true;
							cameravars[HANDMAGIC_PLAYERNUM].shakex += 0.1;
							cameravars[HANDMAGIC_PLAYERNUM].shakey += 10;
							playSoundPlayer(HANDMAGIC_PLAYERNUM, 28, 92);
							Uint32 color = makeColorRGB(255, 255, 0);
							messagePlayerColor(HANDMAGIC_PLAYERNUM, MESSAGE_STATUS, color, Language::get(621));
						}
					}
					--cast_animation[HANDMAGIC_PLAYERNUM].mana_left;
				}

				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle += HANDMAGIC_CIRCLE_SPEED;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex = cos(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey = sin(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				cast_animation[HANDMAGIC_PLAYERNUM].circle_count++;
				if ( cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle >= 2 * PI )   //Completed one loop.
				{
					cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle -= 2 * PI;
				}
				if ( cast_animation[HANDMAGIC_PLAYERNUM].circle_count >= cast_animation[HANDMAGIC_PLAYERNUM].times_to_circle)
					//Finished circling. Time to move on!
				{
					cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle = 0;
					if ( cast_animation[HANDMAGIC_PLAYERNUM].rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH
						|| cast_animation[HANDMAGIC_PLAYERNUM].rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_INTERACT
						|| cast_animation[HANDMAGIC_PLAYERNUM].rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_INTERACT_TEST
						|| cast_animation[HANDMAGIC_PLAYERNUM].rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_FLOOR_TILE
						|| cast_animation[HANDMAGIC_PLAYERNUM].rangefinder == SpellRangefinderType::RANGEFINDER_TOUCH_WALL_TILE )
					{
						cast_animation[HANDMAGIC_PLAYERNUM].stage = ANIM_SPELL_TOUCH;
						if ( *cvar_vibe_spell_s > 0 )
						{
							inputs.rumble(HANDMAGIC_PLAYERNUM, GameController::Haptic_t::RUMBLE_SPELL, *cvar_vibe_spell_x,
								*cvar_vibe_spell_y, *cvar_vibe_spell_s, 0);
						}
						spellcastAnimationUpdate(HANDMAGIC_PLAYERNUM, MONSTER_POSE_MAGIC_WINDUP2, 0);
						playSoundEntityLocal(players[HANDMAGIC_PLAYERNUM]->entity, 759 + local_rng.rand() % 4, 92);
					}
					else
					{
						cast_animation[HANDMAGIC_PLAYERNUM].stage = ANIM_SPELL_THROW;
					}
				}
				break;
			case ANIM_SPELL_THROW:
				//messagePlayer(HANDMAGIC_PLAYERNUM, "IN THROW");
				//TODO: Throw animation! Or something.
				cast_animation[HANDMAGIC_PLAYERNUM].stage = ANIM_SPELL_COMPLETE_SPELL;
				break;
			case ANIM_SPELL_TOUCH_CHARGE:
			{
				my->flags[INVISIBLE] = true;
				my->flags[INVISIBLE_DITHER] = false;

				bool levitating = isLevitating(stats[HANDMAGIC_PLAYERNUM]);

				//Water walking boots
				bool waterwalkingboots = false;
				/*if ( skillCapstoneUnlocked(HANDMAGIC_PLAYERNUM, PRO_LEGACY_SWIMMING) )
				{
					waterwalkingboots = true;
				}*/
				if ( stats[HANDMAGIC_PLAYERNUM]->shoes != NULL )
				{
					if ( stats[HANDMAGIC_PLAYERNUM]->shoes->type == IRON_BOOTS_WATERWALKING )
					{
						waterwalkingboots = true;
					}
				}

				//Check if swimming.
				if ( !waterwalkingboots && !levitating )
				{
					bool swimming = false;
					int x = std::min<int>(std::max<int>(0, floor(players[HANDMAGIC_PLAYERNUM]->entity->x / 16)), map.width - 1);
					int y = std::min<int>(std::max<int>(0, floor(players[HANDMAGIC_PLAYERNUM]->entity->y / 16)), map.height - 1);
					if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						swimming = true;
					}
					if ( swimming )
					{
						//Can't cast spells while swimming if not levitating or water walking.
						messagePlayer(HANDMAGIC_PLAYERNUM, MESSAGE_MISC, Language::get(410));

						playSoundEntityLocal(players[HANDMAGIC_PLAYERNUM]->entity, 163, 64);
						spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
						break;
					}
				}

				auto& anim = cast_animation[HANDMAGIC_PLAYERNUM];
				if ( anim.throw_count == 0 )
				{
					anim.lefthand_movex = 0.f;
					anim.lefthand_movey = 0.f;
					anim.lefthand_angle = 0.0;
					anim.vibrate_x = 0.f;
					anim.vibrate_y = 0.f;
				}
				if ( ticks % 2 == 0 )
				{
					anim.lefthand_movex -= anim.vibrate_x;
					anim.lefthand_movey -= anim.vibrate_y;
					anim.vibrate_x = (local_rng.rand() % 30 - 10) / 150.f;
					anim.vibrate_y = (local_rng.rand() % 30 - 10) / 150.f;
					anim.lefthand_movex += anim.vibrate_x;
					anim.lefthand_movey += anim.vibrate_y;
				}
				anim.lefthand_movex = std::max(-1.f, anim.lefthand_movex - 0.1f);
				anim.throw_count++;
				break;
			}
			case ANIM_SPELL_TOUCH_THROW:
			{
				my->flags[INVISIBLE] = true;
				my->flags[INVISIBLE_DITHER] = false;

				auto& anim = cast_animation[HANDMAGIC_PLAYERNUM];
				if ( anim.throw_count == 0 )
				{
					spellcastingAnimationManager_completeSpell(HANDMAGIC_PLAYERNUM, &cast_animation[HANDMAGIC_PLAYERNUM], false);
					if ( players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 0 )   // debug cam OFF
					{
						cameravars[HANDMAGIC_PLAYERNUM].shakex += .03;
						cameravars[HANDMAGIC_PLAYERNUM].shakey += 4;
					}
					anim.lefthand_movex = 5.f;
					anim.lefthand_angle = 0.0;
				}
				else if ( anim.throw_count < 5 )
				{
					anim.lefthand_angle = std::min(anim.lefthand_angle + 0.025f, 1.f);
				}
				else
				{
					float setpointDiff = std::max(.05f, (1.f - anim.lefthand_angle) / 10.f);
					anim.lefthand_angle += setpointDiff;
					anim.lefthand_angle = std::min(1.f, anim.lefthand_angle);

				}

				if ( anim.throw_count > 0 )
				{
					if ( anim.lefthand_angle >= 1.f )
					{
						anim.lefthand_movex = std::max(-2.5f, anim.lefthand_movex - 0.75f);
					}
					else
					{
						anim.lefthand_movex = std::max(0.0, 2.5f + -2.5f * sin((-PI / 2) + (anim.lefthand_angle) * PI));
					}
				}


				if ( /*anim.throw_count >= 20 ||*/ anim.lefthand_movex <= -2.5f )
				{
					if ( stats[HANDMAGIC_PLAYERNUM]->weapon )
					{
						players[HANDMAGIC_PLAYERNUM]->hud.weaponSwitch = true;
					}
					spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
				}
				anim.throw_count++;
				break;
			}
			case ANIM_SPELL_TOUCH:
			{
				my->flags[INVISIBLE] = true;
				my->flags[INVISIBLE_DITHER] = false;

				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle += HANDMAGIC_CIRCLE_SPEED / 4;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex = cos(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS * 0.25;
				cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey = sin(cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS * 0.25;
				if ( cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle >= 2 * PI )   //Completed one loop.
				{
					cast_animation[HANDMAGIC_PLAYERNUM].lefthand_angle = 0;
					cast_animation[HANDMAGIC_PLAYERNUM].circle_count++;
				}

				bool levitating = isLevitating(stats[HANDMAGIC_PLAYERNUM]);

				//Water walking boots
				bool waterwalkingboots = false;
				/*if ( skillCapstoneUnlocked(HANDMAGIC_PLAYERNUM, PRO_LEGACY_SWIMMING) )
				{
					waterwalkingboots = true;
				}*/
				if ( stats[HANDMAGIC_PLAYERNUM]->shoes != NULL )
				{
					if ( stats[HANDMAGIC_PLAYERNUM]->shoes->type == IRON_BOOTS_WATERWALKING )
					{
						waterwalkingboots = true;
					}
				}

				//Check if swimming.
				if ( !waterwalkingboots && !levitating )
				{
					bool swimming = false;
					int x = std::min<int>(std::max<int>(0, floor(players[HANDMAGIC_PLAYERNUM]->entity->x / 16)), map.width - 1);
					int y = std::min<int>(std::max<int>(0, floor(players[HANDMAGIC_PLAYERNUM]->entity->y / 16)), map.height - 1);
					if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] || lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
					{
						swimming = true;
					}
					if ( swimming )
					{
						//Can't cast spells while swimming if not levitating or water walking.
						messagePlayer(HANDMAGIC_PLAYERNUM, MESSAGE_MISC, Language::get(410));

						playSoundEntityLocal(players[HANDMAGIC_PLAYERNUM]->entity, 163, 64);
						spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
					}
				}
			}
				break;
			case ANIM_SPELL_COMPLETE_NOCAST:
				spellcastingAnimationManager_deactivate(&cast_animation[HANDMAGIC_PLAYERNUM]);
				break;
			case ANIM_SPELL_COMPLETE_SPELL:
			default:
				//messagePlayer(HANDMAGIC_PLAYERNUM, "DEFAULT CASE");
				spellcastingAnimationManager_completeSpell(HANDMAGIC_PLAYERNUM, &cast_animation[HANDMAGIC_PLAYERNUM], true);
				break;
		}
	}
	else
	{
		overDrawDamageNotify = false;
	}

	//Final position code.
	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr)
	{
		return;
	}
	//double defaultpitch = PI / 8.f;
	//double defaultpitch = 0;
	//double defaultpitch = PI / (0-4.f);
	//defaultpitch = (0 - 2.8);
	//my->x = 6 + HUDWEAPON_MOVEX;

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = -hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch;
		my->roll = -hudarm->roll;
		my->yaw = -players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->y = -3;
		my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
		my->z -= 4;
		my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
		my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
		my->roll = HANDMAGIC_ROLL;
		my->focalz = -1.5;

		if ( playerRace == MONSTER_G )
		{
			my->z -= -1 * .5;
		}
		else if ( playerRace == MONSTER_D )
		{
			if ( players[HANDMAGIC_PLAYERNUM]->entity->z >= 1.5 )
			{
				my->z -= -3.0 * .5;
			}
			else
			{
				my->z -= -2.0 * .5;
			}
		}
		else if ( playerRace == MONSTER_M )
		{
			my->z -= -1.0 * .5;
		}
	}

	//my->y = 3 + HUDWEAPON_MOVEY;
	//my->z = (camera.z*.5-players[HANDMAGIC_PLAYERNUM]->z)+7+HUDWEAPON_MOVEZ; //TODO: NOT a PLAYERSWAP
	my->x += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex;
	my->y += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey;

	if ( cast_animation[HANDMAGIC_PLAYERNUM].active )
	{
		if ( cast_animation[HANDMAGIC_PLAYERNUM].stage == ANIM_SPELL_TOUCH
			|| cast_animation[HANDMAGIC_PLAYERNUM].stage == ANIM_SPELL_TOUCH_CHARGE )
		{
			float x = my->x + 2.5;
			float y = -my->y;
			float z = my->z - 0.0;

			Monster playerRace = players[HANDMAGIC_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HANDMAGIC_PLAYERNUM]->playerRace);
			if ( players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift != NOTHING )
			{
				playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift);
				if ( playerRace == RAT )
				{
					y = 0.0;
				}
				else if ( playerRace == SPIDER )
				{
					y = 0.0;
					z -= 2;
				}
			}

			// boosty boost
			Uint32 castLoopDuration = 4 * TICKS_PER_SECOND / 10;
			for ( int i = 1; i < 3 && !(players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1); ++i )
			{
				//if ( i == 1 || i == 3 ) { continue; }
				Uint32 animTick = cast_animation[HANDMAGIC_PLAYERNUM].active_count >= castLoopDuration
					? castLoopDuration
					: cast_animation[HANDMAGIC_PLAYERNUM].active_count;

				Entity* entity = newEntity(1243, 1, map.entities, nullptr); //Particle entity.
				entity->x = x - 0.01 * (5 + local_rng.rand() % 11);
				entity->y = y - 0.01 * (5 + local_rng.rand() % 11);
				entity->z = z - 0.01 * (10 + local_rng.rand() % 21);
				if ( i == 1 )
				{
					entity->y += -2;
					entity->y += -2 * sin(2 * PI * (animTick % 40) / 40.f);
				}
				else
				{
					entity->y += -(-2);
					entity->y -= -2 * sin(2 * PI * (animTick % 40) / 40.f);
				}

				entity->focalz = -2;

				real_t scale = 0.05f;
				scale += (animTick) * 0.025f;
				scale = std::min(scale, 0.5);

				entity->scalex = scale;
				entity->scaley = scale;
				entity->scalez = scale;
				entity->sizex = 1;
				entity->sizey = 1;
				entity->yaw = 0;
				entity->roll = i * 2 * PI / 3;
				entity->pitch = PI + ((animTick % 40) / 40.f) * 2 * PI;
				entity->ditheringDisabled = true;
				entity->flags[PASSABLE] = true;
				entity->flags[NOUPDATE] = true;
				entity->flags[UNCLICKABLE] = true;
				entity->flags[UPDATENEEDED] = false;
				entity->flags[OVERDRAW] = true;
				entity->lightBonus = vec4(0.25f, 0.25f,
					0.25f, 0.f);
				entity->behavior = &actHUDMagicParticle;
				entity->vel_z = 0;
				entity->skill[11] = HANDMAGIC_PLAYERNUM;
				if ( multiplayer != CLIENT )
				{
					entity_uids--;
				}
				entity->setUID(-3);
			}
			++cast_animation[HANDMAGIC_PLAYERNUM].active_count;
		}
	}
	else
	{
		cast_animation[HANDMAGIC_PLAYERNUM].active_count = 0;
	}
}

void actRightHandMagic(Entity* my)
{
	my->flags[INVISIBLE_DITHER] = false;
	if (intro == true)
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if ( !HANDMAGIC_INIT )
	{
		HANDMAGIC_INIT = 1;
		my->focalz = -1.5;
	}

	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		players[HANDMAGIC_PLAYERNUM]->hud.magicRightHand = nullptr;
		list_RemoveNode(my->mynode);
		return;
	}

	my->mistformGLRender = players[HANDMAGIC_PLAYERNUM]->entity->mistformGLRender;

	my->x = 8;
	my->y = 3;
	my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
	double defaultpitch = (0 - 2.2);
	my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
	my->scalex = 0.5f;
	my->scaley = 0.5f;
	my->scalez = 0.5f;
	my->z -= 0.75;

	//Sprite
	Monster playerRace = players[HANDMAGIC_PLAYERNUM]->entity->getMonsterFromPlayerRace(stats[HANDMAGIC_PLAYERNUM]->playerRace);
	int playerAppearance = stats[HANDMAGIC_PLAYERNUM]->stat_appearance;
	if ( players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectShapeshift);
	}
	else if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph != NOTHING )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph > NUMMONSTERS )
		{
			playerRace = HUMAN;
			playerAppearance = players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph - 100;
		}
		else
		{
			playerRace = static_cast<Monster>(players[HANDMAGIC_PLAYERNUM]->entity->effectPolymorph);
		}
	}

	if ( playerRace == MONSTER_G )
	{
		my->z -= -1 * .5;
	}
	else if ( playerRace == MONSTER_D )
	{
		if ( players[HANDMAGIC_PLAYERNUM]->entity->z >= 1.5 )
		{
			my->z -= -3.0 * .5;
		}
		else
		{
			my->z -= -2.0 * .5;
		}
	}
	else if ( playerRace == MONSTER_M )
	{
		my->z -= -1.0 * .5;
	}

	bool noGloves = false;
	if ( stats[HANDMAGIC_PLAYERNUM]->gloves == NULL
		|| playerRace == SPIDER 
		|| playerRace == RAT 
		|| playerRace == CREATURE_IMP
		|| playerRace == TROLL )
	{
		noGloves = true;
	}
	else
	{
		if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GLOVES_DEXTERITY )
		{
			my->sprite = 637;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRACERS_CONSTITUTION )
		{
			my->sprite = 638;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS || stats[HANDMAGIC_PLAYERNUM]->gloves->type == GAUNTLETS_STRENGTH )
		{
			my->sprite = 639;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BRASS_KNUCKLES )
		{
			my->sprite = 640;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == IRON_KNUCKLES )
		{
			my->sprite = 641;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SPIKED_GAUNTLETS )
		{
			my->sprite = 642;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CRYSTAL_GLOVES )
		{
			my->sprite = 591;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == ARTIFACT_GLOVES )
		{
			my->sprite = 590;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SUEDE_GLOVES )
		{
			my->sprite = 802;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BONE_BRACERS )
		{
			my->sprite = 2107;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == BLACKIRON_GAUNTLETS )
		{
			my->sprite = 2109;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == SILVER_GAUNTLETS )
		{
			my->sprite = 2111;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == QUILTED_GLOVES )
		{
			my->sprite = 2113;
		}
		else if ( stats[HANDMAGIC_PLAYERNUM]->gloves->type == CHAIN_GLOVES )
		{
			my->sprite = 2115;
		}
	}

	if ( noGloves )
	{
		switch ( playerRace )
		{
			case SKELETON:
				my->sprite = 774;
				break;
			case INCUBUS:
				my->sprite = 776;
				break;
			case SUCCUBUS:
				my->sprite = 778;
				break;
			case GOBLIN:
				my->sprite = 780;
				break;
			case AUTOMATON:
				my->sprite = 782;
				break;
			case INSECTOID:
				if ( stats[HANDMAGIC_PLAYERNUM]->sex == FEMALE )
				{
					my->sprite = 786;
				}
				else
				{
					my->sprite = 784;
				}
				break;
			case GOATMAN:
				my->sprite = 788;
				break;
			case VAMPIRE:
				my->sprite = 790;
				break;
			case HUMAN:
				if ( playerAppearance / 6 == 0 )
				{
					my->sprite = 634;
				}
				else if ( playerAppearance / 6 == 1 )
				{
					my->sprite = 635;
				}
				else
				{
					my->sprite = 636;
				}
				break;
			case TROLL:
				my->sprite = 855;
				break;
			case SPIDER:
				my->sprite = arachnophobia_filter ? 1005 : 853;
				break;
			case CREATURE_IMP:
				my->sprite = 857;
				break;
			default:
				my->sprite = 634;
				break;
		}
		/*else if ( playerAppearance / 6 == 0 )
		{
			my->sprite = 634;
		}
		else if ( playerAppearance / 6 == 1 )
		{
			my->sprite = 635;
		}
		else
		{
			my->sprite = 636;
		}*/
	}

	if ( !(cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		my->flags[INVISIBLE] = true;
	}
	else
	{
		my->flags[INVISIBLE] = false;

		if ( playerRace == RAT )
		{
			my->flags[INVISIBLE] = true;
			my->y = 0;
			my->z += 1;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1 )   // debug cam
		{
			my->flags[INVISIBLE] = true;
		}
		else if ( players[HANDMAGIC_PLAYERNUM]->entity->isInvisible() )
		{
			my->flags[INVISIBLE] = true;
			my->flags[INVISIBLE_DITHER] = true;
		}
	}

	Entity*& hudarm = players[HANDMAGIC_PLAYERNUM]->hud.arm;

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = hudarm->y;
		//my->z = hudArm->z;
		my->pitch = hudarm->pitch;
		my->roll = hudarm->roll;
		my->yaw = players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->focalz = -1.5;
	}

	if ( (cast_animation[HANDMAGIC_PLAYERNUM].active || cast_animation[HANDMAGIC_PLAYERNUM].active_spellbook) )
	{
		switch ( cast_animation[HANDMAGIC_PLAYERNUM].stage)
		{
			case ANIM_SPELL_CIRCLE:
				if ( ticks % 5 == 0 && !(players[HANDMAGIC_PLAYERNUM]->entity->skill[3] == 1) )
				{
					//messagePlayer(0, "Pingas!");
					Entity* entity = spawnGib(my);
					entity->flags[INVISIBLE] = false;
					entity->flags[SPRITE] = true;
					entity->flags[NOUPDATE] = true;
					entity->flags[UPDATENEEDED] = false;
					entity->flags[OVERDRAW] = true;
					entity->lightBonus = vec4(0.2f, 0.2f, 0.2f, 0.f);
					entity->z += 3.0;
					//entity->sizex = 1; //MAKE 'EM SMALL PLEASE!
					//entity->sizey = 1;
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->yaw = ((local_rng.rand() % 6) * 60) * PI / 180.0;
					entity->pitch = (local_rng.rand() % 360) * PI / 180.0;
					entity->roll = (local_rng.rand() % 360) * PI / 180.0;
					entity->vel_x = cos(entity->yaw) * .1;
					entity->vel_y = sin(entity->yaw) * .1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
					entity->skill[11] = HANDMAGIC_PLAYERNUM;
				}
				break;
			case ANIM_SPELL_THROW:
				break;
			case ANIM_SPELL_TOUCH:
				break;
			default:
				break;
		}
	}

	//Final position code.
	if (players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr)
	{
		return;
	}

	if ( playerRace == SPIDER && hudarm && players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0) )
	{
		my->x = hudarm->x;
		my->y = hudarm->y;
		my->z = hudarm->z;
		my->pitch = hudarm->pitch;
		my->roll = hudarm->roll;
		my->yaw = players[HANDMAGIC_PLAYERNUM]->entity->bodyparts.at(0)->yaw;
		my->scalex = hudarm->scalex;
		my->scaley = hudarm->scaley;
		my->scalez = hudarm->scalez;
		my->focalz = hudarm->focalz;
	}
	else
	{
		my->x = 8;
		my->y = 3;
		my->z = (cameras[HANDMAGIC_PLAYERNUM].z * .5 - players[HANDMAGIC_PLAYERNUM]->entity->z) + 7;
		my->z -= 4;
		my->yaw = HANDMAGIC_YAW - cameravars[HANDMAGIC_PLAYERNUM].shakex2;
		my->pitch = defaultpitch + HANDMAGIC_PITCH - cameravars[HANDMAGIC_PLAYERNUM].shakey2 / 200.f;
		my->roll = HANDMAGIC_ROLL;
		my->focalz = -1.5;

		if ( playerRace == MONSTER_G )
		{
			my->z -= -1 * .5;
		}
		else if ( playerRace == MONSTER_D )
		{
			if ( players[HANDMAGIC_PLAYERNUM]->entity->z >= 1.5 )
			{
				my->z -= -3.0 * .5;
			}
			else
			{
				my->z -= -2.0 * .5;
			}
		}
		else if ( playerRace == MONSTER_M )
		{
			my->z -= -1.0 * .5;
		}
	}

	my->x += cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movex;
	my->y -= cast_animation[HANDMAGIC_PLAYERNUM].lefthand_movey;
}

#define HANDMAGIC_RANGEFINDER_ALPHA my->fskill[0]
#define HANDMAGIC_RANGEFINDER_COLOR_R my->fskill[1]
#define HANDMAGIC_RANGEFINDER_COLOR_G my->fskill[2]
#define HANDMAGIC_RANGEFINDER_COLOR_B my->fskill[3]
void actMagicRangefinder(Entity* my)
{
	my->flags[INVISIBLE_DITHER] = false;
	if ( intro == true )
	{
		my->flags[INVISIBLE] = true;
		return;
	}

	//Initialize
	if ( !HANDMAGIC_INIT )
	{
		HANDMAGIC_INIT = 1;
	}

	if ( players[HANDMAGIC_PLAYERNUM] == nullptr || players[HANDMAGIC_PLAYERNUM]->entity == nullptr
		|| (players[HANDMAGIC_PLAYERNUM]->entity && players[HANDMAGIC_PLAYERNUM]->entity->playerCreatedDeathCam != 0) )
	{
		if ( auto indicator = AOEIndicators_t::getIndicator(my->actSpriteUseCustomSurface) )
		{
			indicator->expired = true;
		}
		if ( players[HANDMAGIC_PLAYERNUM] )
		{
			players[HANDMAGIC_PLAYERNUM]->hud.magicRangefinder = nullptr;
		}
		list_RemoveNode(my->mynode);
		return;
	}

	auto& cast_anim = cast_animation[HANDMAGIC_PLAYERNUM];

	if ( !(cast_anim.active || cast_anim.active_spellbook) || !cast_anim.rangefinder 
		|| cast_anim.stage == ANIM_SPELL_TOUCH_THROW
		|| cast_anim.stage == ANIM_SPELL_CIRCLE )
	{
		my->flags[INVISIBLE] = true;
		return;
	}
	if ( my->flags[INVISIBLE] )
	{
		my->bNeedsRenderPositionInit = true;
	}
	my->flags[INVISIBLE] = false;
	my->sprite = 222;
	my->x = cast_anim.target_x;
	my->y = cast_anim.target_y;
	Entity* target = nullptr;
	if ( cast_anim.targetUid != 0 
		&& (cast_anim.rangefinder == RANGEFINDER_TOUCH 
			|| cast_anim.rangefinder == RANGEFINDER_TOUCH_INTERACT_TEST
			|| cast_anim.rangefinder == RANGEFINDER_TOUCH_INTERACT) )
	{
		if ( target = uidToEntity(cast_anim.targetUid) )
		{
			my->x = target->x;
			my->y = target->y;
			if ( target->behavior == &actArrowTrap
				|| target->behavior == &actMagicTrap )
			{
				my->x = static_cast<int>(my->x / 16);
				my->y = static_cast<int>(my->y / 16);
			}
		}
	}
	my->z = 7.499;
	static ConsoleVariable<float> cvar_player_cast_indicator_scale("/player_cast_indicator_scale", 1.0);
	static ConsoleVariable<float> cvar_player_cast_indicator_rotate("/player_cast_indicator_rotate", 0.0);
	static ConsoleVariable<float> cvar_player_cast_indicator_alpha("/player_cast_indicator_alpha", 0.5);
	static ConsoleVariable<float> cvar_player_cast_indicator_alpha_glow("/player_cast_indicator_alpha_glow", 0.0625);
	static ConsoleVariable<float> cvar_player_cast_indicator_r("/player_cast_indicator_r", 1.0);
	static ConsoleVariable<float> cvar_player_cast_indicator_g("/player_cast_indicator_g", 1.0);
	static ConsoleVariable<float> cvar_player_cast_indicator_b("/player_cast_indicator_b", 1.0);
	my->ditheringDisabled = true;
	my->flags[SPRITE] = true;
	my->flags[PASSABLE] = true;
	my->flags[NOUPDATE] = true;
	my->flags[UNCLICKABLE] = true;
	my->flags[BRIGHT] = true;
	my->scalex = *cvar_player_cast_indicator_scale;
	my->scaley = *cvar_player_cast_indicator_scale;
	my->pitch = 0;
	my->roll = -PI / 2;
	if ( cast_anim.rangefinder == RANGEFINDER_TOUCH_WALL_TILE )
	{
		if ( cast_anim.wallDir == 0 )
		{
			my->flags[INVISIBLE] = true;
		}
	}

	if ( (cast_anim.rangefinder == RANGEFINDER_TOUCH && target)
		|| cast_anim.rangefinder == RANGEFINDER_TOUCH_WALL_TILE )
	{
		if ( cast_anim.wallDir > 0 )
		{
			my->roll = PI;
			my->yaw = cast_anim.wallDir * PI / 2;
			my->z = 0.0;

			if ( cast_anim.wallDir == 1 )
			{
				my->x = my->x * 16.0 + 16.001;
				my->y = my->y * 16.0 + 8.0;
			}
			else if ( cast_anim.wallDir == 3 )
			{
				my->x = my->x * 16.0 - 0.001;
				my->y = my->y * 16.0 + 8.0;
			}
			else if ( cast_anim.wallDir == 2 )
			{
				my->x = my->x * 16.0 + 8.0;
				my->y = my->y * 16.0 + 16.001;
			}
			else if ( cast_anim.wallDir == 4 )
			{
				my->x = my->x * 16.0 + 8.0;
				my->y = my->y * 16.0 - 0.001;
			}
		}
	}

	my->flags[ENTITY_SKIP_CULLING] = true;

	HANDMAGIC_RANGEFINDER_COLOR_R = *cvar_player_cast_indicator_r;
	HANDMAGIC_RANGEFINDER_COLOR_G = *cvar_player_cast_indicator_g;
	HANDMAGIC_RANGEFINDER_COLOR_B = *cvar_player_cast_indicator_b;
	HANDMAGIC_RANGEFINDER_ALPHA = *cvar_player_cast_indicator_alpha + 
		*cvar_player_cast_indicator_alpha_glow * sin(2 * PI * (my->ticks % TICKS_PER_SECOND) / (real_t)(TICKS_PER_SECOND));
	my->yaw += *cvar_player_cast_indicator_rotate;

	if ( !AOEIndicators_t::getIndicator(my->actSpriteUseCustomSurface) )
	{
		int size = 20;
		my->actSpriteUseCustomSurface = AOEIndicators_t::createIndicator(4, size, size * 2 + 4, -1);
	}
	if ( auto indicator = AOEIndicators_t::getIndicator(my->actSpriteUseCustomSurface) )
	{
		indicator->cacheType = AOEIndicators_t::CACHE_CASTING;
		indicator->gradient = 6;
		indicator->radiusMin = 8;
		indicator->castingTarget = true;
		indicator->loop = true;
		indicator->framesPerTick = 2;
		indicator->ticksPerUpdate = 4;
		indicator->delayTicks = 0;
	}
}