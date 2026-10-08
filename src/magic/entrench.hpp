#pragma once

#include "../json.hpp"

// One portable Entrench object. No world UID, pointer, or arbitrary entity skill array.
struct EntrenchStash
{
	enum Type : Uint32 { NONE, DOOR, FURNITURE, BREAKABLE_COLLIDER };
	Uint32 type = NONE;
	Sint32 sprite = 0;
	double scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
	double focalX = 0.0, focalY = 0.0, focalZ = 0.0;
	Sint32 sizeX = 1, sizeY = 1;
	double z = 0.0, yaw = 0.0, pitch = 0.0, roll = 0.0;
	Uint32 flags = 0;
	Sint32 health = 0, maxHealth = 0;
	Sint32 doorDir = 0, doorLocked = 0, doorStatus = 0;
	Sint32 doorDisableLockpicks = 0, doorDisableOpening = 0, doorLockpickHealth = 0;
	Sint32 doorPreventLockpickExploit = 0, doorForceLockedUnlocked = 0, doorUnlockWhenPowered = 0;
	double doorStartAngle = 0.0;
	Sint32 furnitureType = 0, furnitureDir = 0;
	Sint32 furnitureTableRandomItemChance = 0, furnitureTableSpawnChairs = 0;
	Sint32 colliderModel = 0, colliderRotation = 0, colliderHeightOffset = 0;
	Sint32 colliderXOffset = 0, colliderYOffset = 0, colliderCollision = 0;
	Sint32 colliderSizeX = 0, colliderSizeY = 0, colliderDamageTypes = 0, colliderDiggable = 0;
	Sint32 colliderHideMonster = 0, colliderSpellEvent = 0, colliderSpellCooldown = 0;
	Sint32 colliderSpellEventTrigger = 0, colliderIsMapGenerated = 0;

	struct Content
	{
		enum Kind : Uint32 { ITEM, GOLD };
		Uint32 kind = ITEM;
		Sint32 type = 0, status = 0, beatitude = 0, count = 1;
		Uint32 appearance = 0;
		bool identified = false;
		Sint32 gold = 0;
		Sint32 runeStoredPWR = (-2147483647 - 1), runeCreatorPlayer = -1;
		Uint32 runeInstanceId = 0, runeCreatorIdentity = 0;
		bool familiarInitialized = false;
		Uint32 familiarInstanceId = 0;
		Uint32 wraithLoreInstanceId = 0;
		Sint32 familiarHP = 0, familiarMAXHP = 0, familiarMP = 0, familiarMAXMP = 0;
		Sint32 familiarLVL = 0, familiarEXP = 0;
		Sint32 familiarSTR = 0, familiarDEX = 0, familiarCON = 0;
		Sint32 familiarINT = 0, familiarPER = 0, familiarCHR = 0;
		bool serialize(FileInterface* fp)
		{
			fp->property("kind", kind);
			fp->property("type", type); fp->property("status", status);
			fp->property("beatitude", beatitude); fp->property("count", count);
			fp->property("appearance", appearance); fp->property("identified", identified);
			fp->property("gold", gold);
			fp->property("rune_stored_pwr", runeStoredPWR);
			fp->property("rune_creator_player", runeCreatorPlayer);
			fp->property("rune_instance_id", runeInstanceId);
			fp->property("rune_creator_identity", runeCreatorIdentity);
			fp->property("familiar_initialized", familiarInitialized);
			fp->property("familiar_instance_id", familiarInstanceId);
			fp->property("wraith_lore_instance_id", wraithLoreInstanceId);
			fp->property("familiar_hp", familiarHP); fp->property("familiar_maxhp", familiarMAXHP);
			fp->property("familiar_mp", familiarMP); fp->property("familiar_maxmp", familiarMAXMP);
			fp->property("familiar_lvl", familiarLVL); fp->property("familiar_exp", familiarEXP);
			fp->property("familiar_str", familiarSTR); fp->property("familiar_dex", familiarDEX);
			fp->property("familiar_con", familiarCON); fp->property("familiar_int", familiarINT);
			fp->property("familiar_per", familiarPER); fp->property("familiar_chr", familiarCHR);
			return true;
		}
	};
	std::vector<Content> contents;
	bool valid() const { return type >= DOOR && type <= BREAKABLE_COLLIDER; }
	bool serialize(FileInterface* fp)
	{
		fp->property("type", type); fp->property("sprite", sprite);
		fp->property("scale_x", scaleX); fp->property("scale_y", scaleY); fp->property("scale_z", scaleZ);
		fp->property("focal_x", focalX); fp->property("focal_y", focalY); fp->property("focal_z", focalZ);
		fp->property("size_x", sizeX); fp->property("size_y", sizeY);
		fp->property("z", z); fp->property("yaw", yaw); fp->property("pitch", pitch); fp->property("roll", roll);
		fp->property("flags", flags); fp->property("health", health); fp->property("max_health", maxHealth);
		fp->property("door_dir", doorDir); fp->property("door_locked", doorLocked); fp->property("door_status", doorStatus);
		fp->property("door_disable_lockpicks", doorDisableLockpicks); fp->property("door_disable_opening", doorDisableOpening);
		fp->property("door_lockpick_health", doorLockpickHealth); fp->property("door_prevent_lockpick_exploit", doorPreventLockpickExploit);
		fp->property("door_force_locked_unlocked", doorForceLockedUnlocked); fp->property("door_unlock_when_powered", doorUnlockWhenPowered);
		fp->property("door_start_angle", doorStartAngle);
		fp->property("furniture_type", furnitureType); fp->property("furniture_dir", furnitureDir);
		fp->property("furniture_table_random_item_chance", furnitureTableRandomItemChance);
		fp->property("furniture_table_spawn_chairs", furnitureTableSpawnChairs);
		fp->property("collider_model", colliderModel); fp->property("collider_rotation", colliderRotation);
		fp->property("collider_height_offset", colliderHeightOffset);
		fp->property("collider_x_offset", colliderXOffset); fp->property("collider_y_offset", colliderYOffset);
		fp->property("collider_collision", colliderCollision);
		fp->property("collider_size_x", colliderSizeX); fp->property("collider_size_y", colliderSizeY);
		fp->property("collider_damage_types", colliderDamageTypes); fp->property("collider_diggable", colliderDiggable);
		fp->property("collider_hide_monster", colliderHideMonster);
		fp->property("collider_spell_event", colliderSpellEvent); fp->property("collider_spell_cooldown", colliderSpellCooldown);
		fp->property("collider_spell_event_trigger", colliderSpellEventTrigger);
		fp->property("collider_is_map_generated", colliderIsMapGenerated);
		fp->property("contents", contents);
		return true;
	}
};
