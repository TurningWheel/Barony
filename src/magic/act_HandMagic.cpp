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
#include "../sound.hpp"
#include "../items.hpp"
#include "magic.hpp"

//The spellcasting animation stages:
#define CIRCLE 0 //One circle
#define THROW 1 //Throw spell!

spellcasting_animation_manager_t cast_animation;
Entity *magicLeftHand = NULL;
Entity *magicRightHand = NULL;

#define HANDMAGIC_INIT my->skill[0]
#define HANDMAGIC_TESTVAR my->skill[1]
#define HANDMAGIC_YAW my->fskill[3]
#define HANDMAGIC_PITCH my->fskill[4]
#define HANDMAGIC_ROLL my->fskill[5]
#define HANDMAGIC_PARTICLESPRAY1 my->skill[3]
#define HANDMAGIC_CIRCLE_RADIUS 0.8
#define HANDMAGIC_CIRCLE_SPEED 0.3

void fireOffSpellAnimation(spellcasting_animation_manager_t* animation_manager, Uint32 caster_uid, spell_t *spell) {
	//This function triggers the spellcasting animation and sets up everything.

	if (!animation_manager) {
		return;
	}
	Entity *caster = uidToEntity(caster_uid);
	if (!caster) {
		return;
	}
	if (!spell) {
		return;
	}
	if (!magicLeftHand) {
		return;
	}
	if (!magicRightHand) {
		return;
	}

	playSoundEntity(caster, 170, 128 );
	Stat *stat = caster->getStats();

	//Save these two very important pieces of data.
	animation_manager->caster = caster->uid;
	animation_manager->spell = spell;
	animation_manager->active = TRUE;
	animation_manager->stage = CIRCLE;

	//Make the HUDWEAPON disappear, or somesuch?
	magicLeftHand->flags[INVISIBLE] = FALSE;
	magicRightHand->flags[INVISIBLE] = FALSE;

	animation_manager->lefthand_angle = 0;
	animation_manager->lefthand_movex = 0;
	animation_manager->lefthand_movey = 0;

	animation_manager->circle_count = 0;
	animation_manager->times_to_circle = (getCostOfSpell(spell) / 10) + 1; //Circle once for every 10 mana the spell costs.
	animation_manager->mana_left = getCostOfSpell(spell);
	if (stat->PROFICIENCIES[PRO_SPELLCASTING] < SPELLCASTING_BEGINNER) { //There's a chance that caster is newer to magic (and thus takes longer to cast a spell).
		int chance = rand()%10;
		if (chance >= stat->PROFICIENCIES[PRO_SPELLCASTING]/15) {
			int amount = (rand()%50)/std::max(stat->PROFICIENCIES[PRO_SPELLCASTING]+statGetINT(stat),1);
			amount = std::min(amount, CASTING_EXTRA_TIMES_CAP);
			animation_manager->times_to_circle += amount;
		}
	}
	animation_manager->consume_interval = (animation_manager->times_to_circle * ((2 * PI) / HANDMAGIC_CIRCLE_SPEED)) / getCostOfSpell(spell);
	animation_manager->consume_timer = animation_manager->consume_interval;
}

void spellcastingAnimationManager_deactivate(spellcasting_animation_manager_t *animation_manager) {
	animation_manager->caster = -1;
	animation_manager->spell = NULL;
	animation_manager->active = FALSE;
	animation_manager->stage = 0;

	//Make the hands invisible (should probably fall away or something, but whatever. That's another project for another day)
	if( magicLeftHand )
		magicLeftHand->flags[INVISIBLE] = TRUE;
	if( magicRightHand )
		magicRightHand->flags[INVISIBLE] = TRUE;
}

void spellcastingAnimationManager_completeSpell(spellcasting_animation_manager_t *animation_manager) {
	castSpell(animation_manager->caster, animation_manager->spell, FALSE, FALSE); //Actually cast the spell.

	spellcastingAnimationManager_deactivate(animation_manager);
}

/*
[12:48:29 PM] Sheridan Kane Rathbun: you can move the entities about by modifying their x, y, z, yaw, pitch, and roll parameters.
[12:48:43 PM] Sheridan Kane Rathbun: everything's relative to the camera since the OVERDRAW flag is on.
[12:49:05 PM] Sheridan Kane Rathbun: so adding x will move it forward, adding y will move it sideways (forget which way) and adding z will move it up and down.
[12:49:46 PM] Sheridan Kane Rathbun: the first step is to get the hands visible on the screen when you cast. worry about moving them when that critical part is done.
*/

void actLeftHandMagic(Entity *my)
{
	//int c = 0;
	if (intro == TRUE) {
		my->flags[INVISIBLE] = TRUE;
		return;
	}

	//Initialize
	if (!HANDMAGIC_INIT) {
		HANDMAGIC_INIT = 1;
		HANDMAGIC_TESTVAR = 0;
		my->focalz = -1.5;
	}

	if( players[clientnum]==NULL ) {
		magicLeftHand = NULL;
		spellcastingAnimationManager_deactivate(&cast_animation);
		list_RemoveNode(my->mynode);
		return;
	}

	//Set the initial values. (For the particle spray)
	my->x = 8;
	my->y = -3;
	my->z = (camera.z *.5 - players[clientnum]->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - camera_shakex2;
	double defaultpitch = (0-6.f) / PI;
	my->pitch = defaultpitch + HANDMAGIC_PITCH - camera_shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;

	//Sprite
	bool noGloves=FALSE;
	if( stats[clientnum]->gloves == NULL ) {
		noGloves=TRUE;
	} else {
		if( stats[clientnum]->gloves->type == GLOVES || stats[clientnum]->gloves->type == GLOVES_DEXTERITY ) {
			my->sprite = 136 + stats[clientnum]->sex;
		} else if( stats[clientnum]->gloves->type == BRACERS || stats[clientnum]->gloves->type == BRACERS_CONSTITUTION ) {
			my->sprite = 327 + stats[clientnum]->sex;
		} else if( stats[clientnum]->gloves->type == GAUNTLETS || stats[clientnum]->gloves->type == GAUNTLETS_STRENGTH ) {
			my->sprite = 144 + stats[clientnum]->sex;
		} else {
			noGloves = TRUE;
		}
	}
	if( noGloves ) {
		if( stats[clientnum]->appearance/6==0 ) {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 122;
			} else {
				my->sprite = 110;
			}
		} else if( stats[clientnum]->appearance/6==1 ) {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 351;
			} else {
				my->sprite = 338;
			}
		} else {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 377;
			} else {
				my->sprite = 364;
			}
		}
	}

	bool wearingring = FALSE;

	//Select model
	if (stats[clientnum]->ring != NULL)
		if (stats[clientnum]->ring->type == RING_INVISIBILITY)
			wearingring = TRUE;
	if (stats[clientnum]->cloak != NULL)
		if (stats[clientnum]->cloak->type == CLOAK_INVISIBILITY)
			wearingring = TRUE;
	if (players[clientnum]->skill[3] == 1 || stats[clientnum]->EFFECTS[EFF_INVISIBLE] == TRUE || wearingring ) { // debug cam or player invisible
		my->flags[INVISIBLE] = TRUE;
	}

	if (cast_animation.active) {
		switch (cast_animation.stage) {
			case CIRCLE:
				if(ticks%5==0) {
					Entity *entity = spawnGib(my);
					entity->flags[INVISIBLE] = FALSE;
					entity->flags[SPRITE] = TRUE;
					entity->flags[NOUPDATE] = TRUE;
					entity->flags[UPDATENEEDED] = FALSE;
					entity->flags[OVERDRAW] = TRUE;
					entity->flags[BRIGHT] = TRUE;
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->yaw = ((rand()%6)*60)*PI/180.0;
					entity->pitch = (rand()%360)*PI/180.0;
					entity->roll = (rand()%360)*PI/180.0;
					entity->vel_x = cos(entity->yaw)*.1;
					entity->vel_y = sin(entity->yaw)*.1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
				}
				cast_animation.consume_timer--;
				if (cast_animation.consume_timer < 0 && cast_animation.mana_left > 0) {
					//Time to consume mana and reset the ticker!
					cast_animation.consume_timer = cast_animation.consume_interval;
					if (multiplayer == SINGLE) {
						players[clientnum]->drainMP(1);
					}
					cast_animation.mana_left--;
				}

				cast_animation.lefthand_angle += HANDMAGIC_CIRCLE_SPEED;
				cast_animation.lefthand_movex = cos(cast_animation.lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				cast_animation.lefthand_movey = sin(cast_animation.lefthand_angle) * HANDMAGIC_CIRCLE_RADIUS;
				if (cast_animation.lefthand_angle >= 2 * PI) { //Completed one loop.
					cast_animation.lefthand_angle = 0;
					cast_animation.circle_count++;
					if (cast_animation.circle_count >= cast_animation.times_to_circle)
						//Finished circling. Time to move on!
						cast_animation.stage++;
				}
				break;
			case THROW:
				//messagePlayer(clientnum, "IN THROW");
				//TODO: Throw animation! Or something.
				cast_animation.stage++;
				break;
			default:
				//messagePlayer(clientnum, "DEFAULT CASE");
				spellcastingAnimationManager_completeSpell(&cast_animation);
				break;
		}
	}

	//Final position code.
	if( players[clientnum] == NULL )
		return;
	//double defaultpitch = PI / 8.f;
	//double defaultpitch = 0;
	//double defaultpitch = PI / (0-4.f);
	defaultpitch = (0-6.f) / PI;
	//my->x = 6 + HUDWEAPON_MOVEX;
	my->x += cast_animation.lefthand_movex;
	//my->y = 3 + HUDWEAPON_MOVEY;
	my->y = -3;
	my->y += cast_animation.lefthand_movey;
	//my->z = (camera.z*.5-players[clientnum]->z)+7+HUDWEAPON_MOVEZ;
	my->z = (camera.z *.5 - players[clientnum]->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - camera_shakex2;
	my->pitch = defaultpitch + HANDMAGIC_PITCH - camera_shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
}

void actRightHandMagic(Entity *my)
{
	if (intro == TRUE) {
		my->flags[INVISIBLE] = TRUE;
		return;
	}

	//Initialize
	if( !HANDMAGIC_INIT ) {
		HANDMAGIC_INIT=1;
		my->focalz = -1.5;
	}

	if( players[clientnum]==NULL ) {
		magicRightHand=NULL;
		list_RemoveNode(my->mynode);
		return;
	}

	my->x = 8;
	my->y = 3;
	my->z = (camera.z *.5 - players[clientnum]->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - camera_shakex2;
	double defaultpitch = (0-6.f) / PI;
	my->pitch = defaultpitch + HANDMAGIC_PITCH - camera_shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;

	//Sprite
	bool noGloves=FALSE;
	if( stats[clientnum]->gloves == NULL ) {
		noGloves=TRUE;
	} else {
		if( stats[clientnum]->gloves->type == GLOVES || stats[clientnum]->gloves->type == GLOVES_DEXTERITY ) {
			my->sprite = 132 + stats[clientnum]->sex;
		} else if( stats[clientnum]->gloves->type == BRACERS || stats[clientnum]->gloves->type == BRACERS_CONSTITUTION ) {
			my->sprite = 323 + stats[clientnum]->sex;
		} else if( stats[clientnum]->gloves->type == GAUNTLETS || stats[clientnum]->gloves->type == GAUNTLETS_STRENGTH ) {
			my->sprite = 140 + stats[clientnum]->sex;
		} else {
			noGloves = TRUE;
		}
	}
	if( noGloves ) {
		if( stats[clientnum]->appearance/6==0 ) {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 121;
			} else {
				my->sprite = 109;
			}
		} else if( stats[clientnum]->appearance/6==1 ) {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 350;
			} else {
				my->sprite = 337;
			}
		} else {
			if( stats[clientnum]->sex==FEMALE ) {
				my->sprite = 376;
			} else {
				my->sprite = 363;
			}
		}
	}

	bool wearingring = FALSE;

	//Select model
	if (stats[clientnum]->ring != NULL)
		if (stats[clientnum]->ring->type == RING_INVISIBILITY)
			wearingring = TRUE;
	if (stats[clientnum]->cloak != NULL)
		if (stats[clientnum]->cloak->type == CLOAK_INVISIBILITY)
			wearingring = TRUE;
	if (players[clientnum]->skill[3] == 1 || stats[clientnum]->EFFECTS[EFF_INVISIBLE] == TRUE || wearingring ) { // debug cam or player invisible
		my->flags[INVISIBLE] = TRUE;
	}

	if (cast_animation.active) {
		switch (cast_animation.stage) {
			case CIRCLE:
				if (ticks%5==0) {
					//messagePlayer(0, "Pingas!");
					Entity *entity = spawnGib(my);
					entity->flags[INVISIBLE] = FALSE;
					entity->flags[SPRITE] = TRUE;
					entity->flags[NOUPDATE] = TRUE;
					entity->flags[UPDATENEEDED] = FALSE;
					entity->flags[OVERDRAW] = TRUE;
					entity->flags[BRIGHT] = TRUE;
					//entity->sizex = 1; //MAKE 'EM SMALL PLEASE!
					//entity->sizey = 1;
					entity->scalex = 0.25f; //MAKE 'EM SMALL PLEASE!
					entity->scaley = 0.25f;
					entity->scalez = 0.25f;
					entity->sprite = 16; //TODO: Originally. 22. 16 -- spark sprite instead?
					entity->yaw = ((rand()%6)*60)*PI/180.0;
					entity->pitch = (rand()%360)*PI/180.0;
					entity->roll = (rand()%360)*PI/180.0;
					entity->vel_x = cos(entity->yaw)*.1;
					entity->vel_y = sin(entity->yaw)*.1;
					entity->vel_z = -.15;
					entity->fskill[3] = 0.01;
				}
				break;
			case THROW:
				break;
			default:
				break;
		}
	}

	//Final position code.
	if( players[clientnum] == NULL )
		return;
	//double defaultpitch = PI / 8.f;
	//double defaultpitch = 0;
	//double defaultpitch = PI / (0-4.f);
	defaultpitch = (0-6.f) / PI;
	//my->x = 6 + HUDWEAPON_MOVEX;
	my->x = 8;
	my->x += cast_animation.lefthand_movex;
	//my->y = 3 + HUDWEAPON_MOVEY;
	my->y = 3;
	my->y -= cast_animation.lefthand_movey;
	//my->z = (camera.z*.5-players[clientnum]->z)+7+HUDWEAPON_MOVEZ;
	my->z = (camera.z *.5 - players[clientnum]->z) + 7;
	my->z -= 4;
	my->yaw = HANDMAGIC_YAW - camera_shakex2;
	my->pitch = defaultpitch + HANDMAGIC_PITCH - camera_shakey2 / 200.f;
	my->roll = HANDMAGIC_ROLL;
}
